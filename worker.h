#ifndef _WORKER_
#define _WORKER_

#include <pthread.h>
#include <stdint.h>     // INT32_MAX
#include <stdlib.h>     // malloc
#include <stdio.h>      // printf
#include <assert.h>     // assert
#include <unistd.h>     // sleep
#include "common.h"

#if 0
struct receive_queue {
  int num_entries;
  int max_entries;
  int *data;
};


struct worker_data {
  int thread_id;
  int num_receive_queues;
  int current_receive_queue;
  struct receive_queue *receive_queues;
  pthread_mutex_t receive_queue_ready_lock;
  pthread_cond_t receive_queue_ready;
};

int next_receive_queue(struct worker_data *d);
int receive_queue_insert(struct receive_queue *q, int data);
int initialize_receive_queue(struct receive_queue *queue,
                              int max_receive_entries);
int initialize_worker_data(struct worker_data *data,
                            int num_receive_queues,
                            int max_receive_entries);
int worker_data_next_receive_queue(struct worker_data *d);
void *worker(void *threadarg);
#endif


struct worker_data {
  int msg_q_capacity;
  int msg_q_elem_size;
  int xmit_q_transactions;
  int xmit_q_actions_per_transaction;
  int recv_q_transactions;
  int recv_q_actions_per_transaction;
};

void *worker(void *threadarg);
int worker_init(struct thread_context *context);

#endif
