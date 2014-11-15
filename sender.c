#include "sender.h"

void *sender(void *threadarg) {
  assert(threadarg);

  struct thread_context *context;
  struct thread_context *contexts;
  struct sender_data *data;
  struct xmit_queue_slot *s;
  struct netmap_ring *txring;
  int rv, i, first_transmitter = -1, last_transmitter = -1;
  uint32_t t, num_transmitters = 0, empty_queues;
  uint32_t *transmitters;
  
  context = (struct thread_context *)threadarg;
  contexts = context->shared->contexts;
  data = context->data;
  txring = data->txring;

  rv = sender_init(context);
  if (!rv)
    pthread_exit(NULL);

  transmitters = bitmap_new(context->shared->num_threads);
  if (!transmitters)
    pthread_exit(NULL);

  for (t=0; t < context->shared->num_threads; t++) {
    if ((contexts[t].thread_type == WORKER) ||
        (contexts[t].thread_type == ARPD)) {
      bitmap_set(transmitters, t);
      num_transmitters++;
      if (first_transmitter == -1)
        first_transmitter = t;
      last_transmitter = t;
    }
  }

  if (first_transmitter == -1) {
    printf("sender: no transmitters detected\n");
    pthread_exit(NULL);
  }

  printf("sender[%d]: initialized\n", context->thread_id);
  // signal to main() that we are initialized
  atomic_store_explicit(&context->initialized, 1, memory_order_release);

  for (;;) {
    empty_queues = 0;
    while (txring->avail == 0)
      ioctl(data->fd, NIOCTXSYNC, NULL);

    for (i=first_transmitter; i <= last_transmitter; i++) {
      if (!bitmap_get(transmitters, i))
        continue;

      s = cqueue_spsc_trypop_slot(contexts[i].pkt_xmit_q);
      if (!s) {
        empty_queues++;
        continue;
      }

      txring_push(txring, &s->ether_h, s->len);
      cqueue_spsc_pop_slot_finish(context[i].pkt_xmit_q);

      if (txring->avail == 0)
        ioctl(data->fd, NIOCTXSYNC, NULL);
    } // for transmitters

    if (empty_queues == num_transmitters) { // all xmit queues are empty
      if (!NETMAP_TX_RING_EMPTY(txring))      
        ioctl(data->fd, NIOCTXSYNC, NULL);
      usleep(10000);
    }

  } // for (;;)

  pthread_exit(NULL);
}

/*! allocate and initialize queues, etc
  \param[in, out] context
  \returns 1 on success, 0 on failure
*/
int sender_init(struct thread_context *context) {
  assert(context);

  struct sender_data *data = context->data;
  data->dropped = calloc(context->shared->num_threads, sizeof(*data->dropped));
  if (!data->dropped)
    return 0;

  return 1;
}

void txring_push(struct netmap_ring *ring, void *data, uint16_t len) {
  uint32_t cur;
  struct netmap_slot *slot;
  char *buf;

  assert(ring->avail > 0);

  cur = ring->cur; 
  slot = &ring->slot[cur];
  slot->flags = 0;
  slot->len = len;
  buf = NETMAP_BUF(ring, slot->buf_idx);
  memcpy(buf, data, len);
  ring->avail--;
  ring->cur = NETMAP_RING_NEXT(ring, cur);
}
