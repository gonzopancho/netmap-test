#ifndef _WORKER_
#define _WORKER_

#include <pthread.h>
#include <stdint.h>     // INT32_MAX
#include <stdlib.h>     // malloc
#include <stdio.h>      // printf
#include <assert.h>     // assert
#include <unistd.h>     // sleep
#include "ethernet.h"
#include "ip4.h"
#include "common.h"
#include "message.h"


struct worker_data {
  int msg_q_capacity;
  int msg_q_elem_size;
  int xmit_q_capacity;
  int xmit_q_elem_size;
  int recv_q_transactions;
  int recv_q_actions_per_transaction;
  struct netmap_ring *rxring;
};

void *worker(void *threadarg);
int worker_init(struct thread_context *context);

#endif
