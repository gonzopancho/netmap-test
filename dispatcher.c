#include "dispatcher.h"

void *dispatcher(void *threadarg) {
  assert(threadarg);

  struct thread_context *context;
  int rv, arpd_idx = 0;
  struct netmap_ring *rxring;
  struct ethernet_pkt *etherpkt;
  struct pollfd pfd;
  struct dispatcher_data *data;
  uint32_t *slots_used;
  uint32_t i;
  char *buf;

  context = (struct thread_context *)threadarg;

  struct transaction *transactions[context->num_threads];
  uint64_t dropped[context->num_threads];

  data = context->data;
  rv = dispatcher_init(context);

  if (!rv) {
    pthread_exit(NULL);
  }

  rxring = NETMAP_RXRING(data->nifp, 0);
  slots_used = bitmap_new(rxring->num_slots);
  if (!slots_used)
    exit(1);

  pfd.fd = data->fd;
  pfd.events = (POLLIN);

  for (i=0; i < context->num_threads; i++) {
    transactions[i] = NULL;
    dropped[i] = 0;
    if (context->contexts[i].thread_type == ARPD)
      arpd_idx = i;
  }

  printf("dispatcher[%d]: initialized\n", context->thread_id);
  // signal to main() that we are initialized
  atomic_store_explicit(&context->initialized, 1, memory_order_release);

  for (;;) {
    rv = poll(&pfd, 1, INFTIM);
    if (rv < 0) {
      perror("poll failed");
      continue;
    }
    
    for (; rxring->avail > 0; rxring->avail--) {
      i = rxring->cur;
      rxring->cur = NETMAP_RING_NEXT(rxring, i);
      buf = NETMAP_BUF(rxring, rxring->slot[i].buf_idx);
      etherpkt = (struct ethernet_pkt *)(void *)buf;

      // TODO: consider pushing this check to the workers
      if (!ethernet_is_valid(etherpkt, &data->ifi->mac))
        continue;

      // TODO: dispatch to n workers instead of just 0
      switch (etherpkt->h.ether_type) {
        case IP4_ETHERTYPE:
          rv = tqueue_insert(context->contexts[0].pkt_recv_q,
                             &transactions[0], buf);

          if (rv == TQUEUE_FULL) { 
            // just drop packet and do accounting
            dropped[0]++;
          } else {
            bitmap_set(slots_used, i);
          }

          break;
        case ARP_ETHERTYPE:
          rv = tqueue_insert(context->contexts[arpd_idx].pkt_recv_q,
                             &transactions[arpd_idx], buf);

          if (rv == TQUEUE_FULL) {
            // just drop packet and do accounting
            dropped[arpd_idx]++;
          } else {
            if (rv == TQUEUE_SUCCESS) {
              // don't batch arp pkts for processing
              tqueue_publish_transaction(context->contexts[arpd_idx].pkt_recv_q,
                                          &transactions[arpd_idx]);
            }
            bitmap_set(slots_used, i);
          }

          break;
        default:
          printf("dispatcher[%d]: unknown/unsupported ethertype %hu\n",
                  context->thread_id, etherpkt->h.ether_type);
      }
    } // for rxring

    // TODO: read msg_q
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


#if 0
int dispatcher_next_receive_queue(struct worker_data *d, int cur) {
  int next_queue;
  next_queue = cur < (d->num_receive_queues - 1) ? (cur + 1) : 0;

  return next_queue == d->current_receive_queue ? cur : next_queue;
}
#endif

#if 0
void *dispatcher(void *threadargs) {
  uint32_t i;
  struct dispatcher_data dispatchers[NUM_WORKERS] = {0};

  for(i=0; i<NUM_WORKERS; i++) {
    dispatchers[t].current_receive_queue =
            next_receive_queue(&threadargs[t]);
  }

    while(1) {
        sleep(1);
        retval = poll(&pfd,1, INFTIM);
        if (retval < 0) {
            perror("poll failed");
            continue;
        }

        bytes_read = read(pfd.fd, random_data, sizeof(random_data));
        printf("read %zd bytes\n", bytes_read);
        for(i=0; i<bytes_read; i++) {
            /* dispatch to one of the NUM_THREADS queues */
            /* if NUM_THREADS is a power of 2, x & (NUM_THREADS - 1) is mod */
            t = random_data[i] & (NUM_THREADS-1);

            /* can't write the the queue currently being read, so write to the
                next one */
            queue_num = dispatchers[t].current_receive_queue;
            queue = &threadargs[t].receive_queues[queue_num];
#ifdef DEBUG_PRINT
            printf("dispatcher: writing to thread %ld, queue %d %d\n", t, queue_num, random_data[i]);
#endif
            retval = receive_queue_insert(queue, random_data[i]);
            switch(retval) {
                case 1:
                    /* queue still has space */
                    dispatchers[t].received++;
                    break;
                case INT32_MAX:
                    /* queue is now full, so try and switch it */
                    dispatchers[t].received++;
#ifdef DEBUG_PRINT
                    printf("dispatcher: thread %ld, queue %d is now full\n",
                            t, queue_num);
#endif
                    if (worker_data_next_receive_queue(&threadargs[t])) {
#ifdef DEBUG_PRINT
                        printf("dispatcher: thread %ld current receive queue changed to %d\n", t, threadargs[t].current_receive_queue);
#endif
                    } else {
#ifdef DEBUG_PRINT
                        printf("dispatcher: failed to change receive queue for thread %ld\n", t);
#endif
                    }

                    /* Regardless of whether the worker moved to a new queue,
                        the dispatcher should try to write to a different one.
                        If there are no free queues available, then on the
                        next write attempt the queue will already be full */
                    dispatchers[t].current_receive_queue =
                        dispatcher_next_receive_queue(&threadargs[t],
                                                        queue_num);
                    break;
                case 0:
                    /* Queue was already full...  */
#ifdef DEBUG_PRINT
                    printf("dispatcher: thread %ld, queue %d was already full\n", t, queue_num);
#endif
                    if (worker_data_next_receive_queue(&threadargs[t])) {
                        /* stay one step ahead of the worker */
                        dispatchers[t].current_receive_queue =
                            dispatcher_next_receive_queue(&threadargs[t],
                                                        queue_num);
                    /* we changed queues, so retry adding this byte */
#ifdef DEBUG_PRINT
                        printf("dispatcher: thread %ld current receive queue changed to %d\n", t, threadargs[t].current_receive_queue);
#endif
                        i--;
                        continue;
                    } else {
                        /* couldn't swap the queue, so drop the packet! */
                        dispatchers[t].dropped++;
#ifdef DEBUG_PRINT
                        printf("thread[%ld]: receive queue [%d] dropped data\n",
                            t, queue_num);
#endif
                    }
                    break;
                default:
                    break;
            }
        } // for bytes_read

        for(t=0; t<NUM_THREADS;t++) {
            printf("dispatcher: thread[%ld] received: %zu, dropped: %zu\n",
                    t, dispatchers[t].received, dispatchers[t].dropped);
        }
    } // while(1)
}
#endif

#if 0
void dispatch(struct ethernet_pkt *pkt, uint16_t len) {
    struct arp_pkt *arp;

    switch (pkt->h.ether_type) {
        case IP4_ETHERTYPE:
            break;
        case ARP_ETHERTYPE:
            arp = (struct arp_pkt *)(pkt->data);
            if(arp_is_valid(arp)) {
                print_buf2((char *)pkt, len);
                arp_print(arp);
            }
            break;
        case IP6_ETHERTYPE:
        default:
            printf("DISPATCH: unknown ethertype\n");
    }
}
#endif

