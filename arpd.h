#ifndef _ARPD_
#define _ARPD_

#include <pthread.h>
#include <stdint.h>     // INT32_MAX
#include <stdlib.h>     // malloc
#include <stdio.h>      // printf
#include "ethernet.h"
#include "arp.h"
#include "common.h"

struct arp_data {
  int thread_id;
};

void *arpd(void *threadargs);

#endif
