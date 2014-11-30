#include "worker.h"


void *worker(void *threadarg) {
  assert(threadarg);

  struct thread_context *context;
  struct thread_context *contexts;
  struct thread_context *dispatcher;
  struct worker_data *data;
  int rv;

  struct transaction *transaction = NULL;
  struct ethernet_pkt *etherpkt;
  struct ip4_pkt *ip;
  struct netmap_ring *rxring;
  void *ring_idx;
  uint32_t *slots_read;

  context = (struct thread_context *)threadarg;
  contexts = context->shared->contexts;
  data = context->data;
  dispatcher = &contexts[context->shared->dispatcher_idx];

  rxring = data->rxring;
  slots_read = bitmap_new(rxring->num_slots);
  if (!slots_read)
    pthread_exit(NULL);

  rv = worker_init(context);
  if (!rv) {
    pthread_exit(NULL);
  }

  printf("worker[%d]: initialized\n", context->thread_id);
  // signal to main() that we are initialized
  atomic_store_explicit(&context->initialized, 1, memory_order_release);

  for (;;) {
    // read all the incoming packets
    while ((rv = tqueue_remove(context->pkt_recv_q, &transaction, &ring_idx))
           != TQUEUE_EMPTY) {
      etherpkt = (struct ethernet_pkt *) NETMAP_BUF(rxring,
                                    rxring->slot[(uint32_t)ring_idx].buf_idx);
      switch (etherpkt->h.ether_type) {
        case IP4_ETHERTYPE:
          ip = (struct ip4_pkt*) etherpkt->data;
          ip4_print_line(ip);
          break;
        default:
          printf("worker[%d]: unknown ethertype\n", context->thread_id);
      }
      bitmap_set(slots_read, (uint32_t)ring_idx);
      if (rv == TQUEUE_TRANSACTION_EMPTY) {
        send_msg_transaction_update(dispatcher, slots_read, rxring->num_slots);
        bitmap_clearall(slots_read, rxring->num_slots);
      }
    } // while (packets)

    sleep(1);
  } // for (;;)

  // TODO: read all the messages

  pthread_exit(NULL);
}


/*! allocate and initialize queues, etc
  \param[in, out] context
  \returns 1 on success, 0 on failure
*/
int worker_init(struct thread_context *context) {
  assert(context);

  struct worker_data *data = context->data;

  context->msg_q = squeue_new(data->msg_q_capacity, data->msg_q_elem_size);
  if (!context->msg_q)
    return 0;

  context->pkt_xmit_q = cqueue_spsc_new(data->xmit_q_capacity,
                                        data->xmit_q_elem_size);
  if (!context->pkt_xmit_q) {
    squeue_delete(&context->msg_q);
    return 0;
  }

  context->pkt_recv_q = tqueue_new(data->recv_q_transactions,
                                    data->recv_q_actions_per_transaction);
  if (!context->pkt_recv_q) {
    squeue_delete(&context->msg_q);
    cqueue_spsc_delete(&context->pkt_xmit_q);
    return 0;
  }

  return 1;
}
