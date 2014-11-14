#ifndef _ARPD_
#define _ARPD_

#include <pthread.h>
#include <stdint.h>     // INT32_MAX
#include <stdlib.h>     // malloc
#include <stdio.h>      // printf
#include <unistd.h>     // sleep
#include "ethernet.h"
#include "arp.h"
#include "common.h"
#include "message.h"


struct arpd_data {
  int msg_q_capacity;
  int msg_q_elem_size;
  int xmit_q_capacity;
  int xmit_q_elem_size;
  int recv_q_transactions;
  int recv_q_actions_per_transaction;
  struct ether_addr *mac;
  struct in_addr *addr;
  struct netmap_ring *rxring;
};

void *arpd(void *threadargs);
int arpd_init(struct thread_context *context);

#endif
