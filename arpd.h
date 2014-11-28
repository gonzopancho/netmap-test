#ifndef _ARPD_
#define _ARPD_

#include <pthread.h>
#include <stdint.h>     // INT32_MAX
#include <stdlib.h>     // malloc
#include <stdio.h>      // printf
#include <unistd.h>     // sleep
#include <string.h>     // memcmp
#include <arpa/inet.h>  // ntohl
#include "ethernet.h"
#include "arp.h"
#include "bitmap.h"
#include "common.h"
#include "message.h"

// uint16_t
#define ARP_CACHE_RETRY_LIMIT 3
// ms
#define ARP_CACHE_RETRY_TIMER 1
// ms 
#define ARP_CACHE_LIFETIME 600000

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
  struct arp_cache *arp_cache;
};

struct arp_cache {
  struct arp_cache_entry *values;
  in_addr_t baseline;
  uint32_t num_entries;
};

struct arp_cache_entry {
  struct ether_addr mac;
  uint16_t retries;
  uint32_t waiters[(MAX_THREADS + BITMAP_BLOCKSIZE - 1)/BITMAP_BLOCKSIZE];
  struct timeval timestamp;
};

void *arpd(void *threadargs);
int arpd_init(struct thread_context *context);
int xmit_queue_init(cqueue_spsc *q,
                    struct in_addr *my_ip, struct ether_addr *my_mac);
int send_arp_request(cqueue_spsc *q, struct in_addr *target_ip);
int send_arp_reply(cqueue_spsc *q,
                    struct in_addr *target_ip, struct ether_addr *target_mac);
struct arp_cache *arp_cache_new(struct thread_context *context);
struct arp_cache_entry *arp_cache_lookup(struct arp_cache *arp_cache,
                                          struct in_addr *key);
void arp_cache_insert(struct arp_cache *arp_cache,
                      struct in_addr *key,
                      struct ether_addr *value);
void arp_cache_print(struct arp_cache *arp_cache);
void handle_arp_reply(struct arp_cache *arp_cache,
                      struct in_addr *key,
                      struct ether_addr *value);
void notify_waiter(uint32_t thread_id, struct in_addr *key,
                    struct ether_addr *value);
void update_arp_cache(struct arp_cache *arp_cache);
int delta_t_exceeds(struct timeval *s, struct timeval *e, uint64_t limit);
#endif
