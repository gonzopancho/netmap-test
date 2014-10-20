#ifndef _DISPATCHER_
#define _DISPATCHER_

#include <stdlib.h>   // size_t
#include "common.h"
#include "worker.h"

#if 0
struct dispatcher_data {
  int thread_id;
  struct worker_stats *worker_stats;
};
#endif

struct worker_stats {
  size_t received;
  size_t dropped;
  int current_receive_queue;
};

struct dispatcher_data {
  int msg_q_capacity;
  int msg_q_elem_size;
  struct worker_stats *worker_stats;
};

void *dispatcher(void *threadarg);
int dispatcher_init(struct thread_context *context);

#endif
