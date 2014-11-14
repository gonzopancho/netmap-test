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
int xmit_queue_init(cqueue_spsc *q,
                    struct in_addr *my_ip, struct ether_addr *my_mac);
int send_arp_request(cqueue_spsc *q, struct in_addr *target_ip);
int send_arp_reply(cqueue_spsc *q,
                    struct in_addr *target_ip, struct in_addr *target_mac);

#endif
