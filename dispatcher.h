#ifndef _DISPATCHER_
#define _DISPATCHER_

#include <stdlib.h>   // size_t
#include <poll.h>     // pollfd
#include "common.h"
#include "worker.h"
#include "ethernet.h"
#include "message.h"

#define POLL_TIMEOUT 10

struct worker_stats {
  size_t received;
  size_t dropped;
  int current_receive_queue;
};

struct dispatcher_data {
  int msg_q_capacity;
  int msg_q_elem_size;
  int fd;
  struct netmap_if *nifp;
  struct if_info *ifi;
  struct worker_stats *worker_stats;
};

void *dispatcher(void *threadarg);
int dispatcher_init(struct thread_context *context);
void update_slots_used_single(void *msg_hdr, uint32_t *bitmap,
                              struct netmap_ring *ring);
#endif
