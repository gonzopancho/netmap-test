#include "worker.h"


#if 0
int next_receive_queue(struct worker_data *d) {
  return d->current_receive_queue < (d->num_receive_queues -1) ? (d->current_receive_queue + 1) : 0;
}


int receive_queue_insert(struct receive_queue *q, int data) {
  if (q->num_entries == q->max_entries)
    return 0;
  q->data[q->num_entries] = data;
  q->num_entries++;

  return q->num_entries == q->max_entries ? INT32_MAX : 1;
}


int initialize_receive_queue(struct receive_queue *queue,
                              int max_receive_entries) {
  queue->max_entries = max_receive_entries;
  queue->num_entries = 0;
  queue->data = malloc(sizeof(int) * max_receive_entries);
  if (!queue->data)
    return 0;

  return 1;
}

int initialize_worker_data(struct worker_data *data,
                            int num_receive_queues,
                            int max_receive_entries) {
  int i;

  if (num_receive_queues < 2)
    return 0;

  data->num_receive_queues = num_receive_queues;
  data->current_receive_queue = 0;
  data->receive_queues = malloc(sizeof(struct receive_queue) *
                                  num_receive_queues);
  if (!data->receive_queues)
    return 0;

  for (i=0; i<num_receive_queues; i++) {
    if (!initialize_receive_queue(&data->receive_queues[i],
                                  max_receive_entries))
      return 0;
  }

  if (pthread_mutex_init(&data->receive_queue_ready_lock, NULL) != 0)
    return 0;

  if (pthread_cond_init(&data->receive_queue_ready, NULL) != 0)
    return 0;

  return 1;
}

int worker_data_next_receive_queue(struct worker_data *d) {
  if (pthread_mutex_trylock(&d->receive_queue_ready_lock) != 0) {
#ifdef DEBUG_PRINT
    printf("thread[%d]: trylock failed\n", d->thread_id);
#endif
    return 0;
  }

  d->current_receive_queue = next_receive_queue(d);
  if (pthread_cond_signal(&d->receive_queue_ready) != 0) {
    fprintf(stderr, "thread[%d]: pthread_cond_signal failed\n",
            d->thread_id);
    return 0;
  }

  if (pthread_mutex_unlock(&d->receive_queue_ready_lock) != 0)
    return 0;

  return 1;
}

void *worker(void *threadarg) {
  struct worker_data *wd;
  struct receive_queue *q;
  int i;

  wd = (struct worker_data *) threadarg;

  while (1) {
    pthread_mutex_lock(&wd->receive_queue_ready_lock);
    while (wd->receive_queues[wd->current_receive_queue].num_entries == 0) {
      pthread_cond_wait(&wd->receive_queue_ready,
                        &wd->receive_queue_ready_lock);
      // awake now, so read the queue data
#ifdef DEBUG_PRINT
      printf("thread[%d]: reading queue %d\n", wd->thread_id, wd->current_receive_queue);
#endif
      q = &wd->receive_queues[wd->current_receive_queue];
      for (i=0; i<q->num_entries; i++) {
        //do_work(q->data[i]);
      }
      q->num_entries = 0;
#ifdef DEBUG_PRINT
      printf("thread[%d]: done reading queue %d\n", wd->thread_id, wd->current_receive_queue);
#endif
    }
    pthread_mutex_unlock(&wd->receive_queue_ready_lock);
  }

  pthread_exit(NULL);
}
#endif


void *worker(void *threadarg) {
  assert(threadarg);

  struct thread_context *context;
  int rv;

  context = (struct thread_context *)threadarg;
  rv = worker_init(context);

  if (!rv) {
    pthread_exit(NULL);
  }

  printf("worker[%d]: initialized\n", context->thread_id);
  // signal to main() that we are initialized
  atomic_store_explicit(&context->initialized, 1, memory_order_release);

  // TODO: enter event loop
  for (;;) {
    sleep(60);
  }

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
