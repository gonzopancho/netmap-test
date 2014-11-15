#ifndef _SENDER_
#define _SENDER_

#include <pthread.h>
#include <stdlib.h>     // calloc
#include <stdio.h>      // printf
#include <unistd.h>     // usleep
#include <sys/ioctl.h>  // ioctl
#include "common.h"
#include "cqueue/cqueue.h"

struct sender_data {
  size_t *dropped;
  struct netmap_ring *txring;
  int fd;
};

void *sender(void *threadarg);
int sender_init(struct thread_context *context);
void txring_push(struct netmap_ring *ring, void *data, uint16_t len);

#endif
