#include "worker.h"


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
