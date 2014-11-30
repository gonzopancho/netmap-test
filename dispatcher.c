#include "dispatcher.h"

void *dispatcher(void *threadarg) {
  assert(threadarg);

  struct thread_context *context;
  struct thread_context *contexts;
  int rv;
  struct netmap_ring *rxring;
  struct ethernet_pkt *etherpkt;
  struct pollfd pfd;
  struct dispatcher_data *data;
  uint32_t *slots_used, *open_transactions;
  uint32_t i, arpd_idx, num_threads;
  char *buf;
  struct msg_hdr *msg;
  struct ether_addr *mac;

  context = (struct thread_context *)threadarg;
  contexts = context->shared->contexts;
  data = context->data;
  arpd_idx = context->shared->arpd_idx;
  mac = &context->shared->if_info->mac;
  num_threads = context->shared->num_threads;

  struct transaction *transactions[num_threads];
  uint64_t dropped[num_threads];
  for (i=0; i < num_threads; i++) {
    transactions[i] = NULL;
    dropped[i] = 0;
  }

  rv = dispatcher_init(context);
  if (!rv) {
    pthread_exit(NULL);
  }

  rxring = NETMAP_RXRING(data->nifp, 0);
  slots_used = bitmap_new(rxring->num_slots);
  if (!slots_used)
    pthread_exit(NULL);

  open_transactions = bitmap_new(num_threads);
  if (!open_transactions)
    pthread_exit(NULL);

  pfd.fd = data->fd;
  pfd.events = (POLLIN);

  printf("dispatcher[%d]: initialized\n", context->thread_id);
  // signal to main() that we are initialized
  atomic_store_explicit(&context->initialized, 1, memory_order_release);

  for (;;) {
    rv = poll(&pfd, 1, POLL_TIMEOUT);

    // read all packets from the ring
    if (rv > 0) {
      for (; rxring->avail > 0; rxring->avail--) {
        i = rxring->cur;
        rxring->cur = NETMAP_RING_NEXT(rxring, i);
        rxring->reserved++;
        buf = NETMAP_BUF(rxring, rxring->slot[i].buf_idx);
        etherpkt = (struct ethernet_pkt *)(void *)buf;

        // TODO: consider pushing this check to the workers
        if (!ethernet_is_valid(etherpkt, mac)) {
          if (rxring->reserved == 1)
            rxring->reserved = 0;
          continue;
        }

        // TODO: dispatch to n workers instead of just 0
        switch (etherpkt->h.ether_type) {
          case IP4_ETHERTYPE:
            rv = tqueue_insert(contexts[0].pkt_recv_q,
                               &transactions[0], (char *) NULL + i);
            switch (rv) {
              case TQUEUE_SUCCESS:
                bitmap_set(slots_used, i);
                bitmap_set(open_transactions, 0);
                break;
              case TQUEUE_TRANSACTION_FULL:
                bitmap_set(slots_used, i);
                bitmap_clear(open_transactions, 0);
                break;
              case TQUEUE_FULL:
                // just drop packet and do accounting
                dropped[0]++;
                if (rxring->reserved == 1)
                  rxring->reserved = 0;
                break;
            }
            break;
          case ARP_ETHERTYPE:
            rv = tqueue_insert(contexts[arpd_idx].pkt_recv_q,
                               &transactions[arpd_idx], (char *) NULL + i);
            switch (rv) {
              case TQUEUE_SUCCESS:
                tqueue_publish_transaction(contexts[arpd_idx].pkt_recv_q,
                                            &transactions[arpd_idx]);
                bitmap_set(slots_used, i);
                break;
              case TQUEUE_TRANSACTION_FULL:
                bitmap_set(slots_used, i);
                break;
              case TQUEUE_FULL:
                // just drop packet and do accounting
                dropped[arpd_idx]++;
                if (rxring->reserved == 1)
                  rxring->reserved = 0;
                break;
            }
            break;
          default:
            printf("dispatcher[%d]: unknown/unsupported ethertype %hu\n",
                    context->thread_id, etherpkt->h.ether_type);
            if (rxring->reserved == 1)
              rxring->reserved = 0;
        } // switch (ethertype)
      } // for (rxring)

      // publish any open transactions so that the worker can start on it
      for (i=0; i < num_threads; i++) {
        if (bitmap_get(open_transactions, i))
          tqueue_publish_transaction(contexts[i].pkt_recv_q, &transactions[i]);
      }
      bitmap_clearall(open_transactions, num_threads);
    } // if (packets)

    // read the message queue
    rv = squeue_enter(context->msg_q, 1);
    if (!rv)
      continue;
    while ((msg = squeue_get_next_pop_slot(context->msg_q)) != NULL) {
      switch (msg->msg_type) {
        case MSG_TRANSACTION_UPDATE:
          update_slots_used(context->msg_q, slots_used, rxring);
          break;
        case MSG_TRANSACTION_UPDATE_SINGLE:
          update_slots_used_single((void *)msg, slots_used, rxring);
          break;
        default:
          printf("dispatcher: unknown message %hu\n", msg->msg_type);
      }
    }
    squeue_exit(context->msg_q);

  } // for(;;)

  pthread_exit(NULL);
}

/*! allocate and initialize queues, etc
  \param[in, out] context
  \returns 1 on success, 0 on failure
*/
int dispatcher_init(struct thread_context *context) {
  assert(context);

  struct dispatcher_data *data = context->data;

  context->msg_q = squeue_new(data->msg_q_capacity, data->msg_q_elem_size);
  if (!context->msg_q)
    return 0;

  return 1;
}

void update_slots_used_single(void *msg_hdr, uint32_t *bitmap,
                              struct netmap_ring *ring) {
  struct msg_transaction_update_single *msg = msg_hdr;
  bitmap_clear(bitmap, msg->ring_idx);

  // reduce the range of slots in use as indicated by the bitmap
  while (!bitmap_get(bitmap,
            (ring->num_slots + ring->cur - ring->reserved) % ring->num_slots)) {
    ring->reserved--;
    if (!ring->reserved)
      break;
  }
}

void update_slots_used(squeue *q, uint32_t *bitmap, struct netmap_ring *ring) {
  struct msg_transaction_update_data *msg_data;
  uint16_t i;
  uint32_t *mask;
  size_t bitmap_blocks_per_data_block;

  bitmap_blocks_per_data_block = (MSG_BLOCKSIZE * CHAR_BIT) / BITMAP_BLOCKSIZE;
  mask = bitmap_new(ring->num_slots);
  assert(mask);

  // reconsitute the mask
  for (i=0; i < BITMAP_BLOCKS(ring->num_slots); i++) {
    if (i % bitmap_blocks_per_data_block == 0) {
      msg_data = squeue_get_next_pop_slot(q);
      assert(msg_data);
    }
    mask[i] = msg_data->data[i%bitmap_blocks_per_data_block];
  }

  bitmap_clearmask(bitmap, ring->num_slots, mask, ring->num_slots);

  // reduce the range of slots in use as indicated by the bitmap
  while (!bitmap_get(bitmap,
            (ring->num_slots + ring->cur - ring->reserved) % ring->num_slots)) {
    ring->reserved--;
    if (!ring->reserved)
      break;
  }
}
