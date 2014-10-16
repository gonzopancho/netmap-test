#ifndef _RECEIVER_
#define _RECEIVER_

#include <pthread.h>
#include <stdint.h>     // INT32_MAX
#include <stdlib.h>     // malloc
#include <stdio.h>      // printf
#include "common.h"

struct receiver_data {
  int thread_id;
  int netmap_fd;
};


void *receiver(void *threadargs);

#endif
