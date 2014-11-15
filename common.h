#ifndef _COMMON_
#define _COMMON_
#include <sys/types.h>      // u_char, required for <net/ethernet.h>
#include <net/ethernet.h>   // ether_header, ether_ntoa_r
#include <netinet/in.h>     // IP protos, required for <netinet/ip.h>
#include <net/if.h>         // IFNAMSIZ, required for <net/netmap.h>
#include <net/netmap.h>
#include <net/netmap_user.h>
#include "tqueue.h"
#include "squeue/squeue.h"
#include "bitmap.h"
#include "cqueue/cqueue.h"

#ifndef MEMCPY_ALIGNMENT
#define MEMCPY_ALIGNMENT 32
#endif

#ifndef MTU
#define MTU 1500
#endif

typedef enum {
  UNINITIALIZED,
  WORKER,
  ARPD,
  DISPATCHER,
  SENDER
} thread_type;


struct if_info {
  struct ether_addr mac;  /* eg ff:ff:ff:ff:ff:ff */
  char *ifname;           /* eg em0 */
  uint32_t mtu;           /* eg 1500 */
  uint16_t link_type;     /* eg ARP_HAF_ETHER */
};

struct inet_info {
  struct in_addr addr;
  struct in_addr network;
  struct in_addr netmask;
  struct in_addr broadcast;
  struct in_addr default_route;
};

struct shared_context {
  struct thread_context *contexts;
  uint32_t num_threads;
  uint32_t dispatcher_idx;
  uint32_t arpd_idx;
};

struct thread_context {
  pthread_t thread;
  int thread_id;
  int thread_type;
  uint32_t num_threads;
  struct thread_context *contexts;
  struct shared_context *shared;
  cqueue_spsc *pkt_xmit_q;
  tqueue *pkt_recv_q;
  squeue *msg_q;
  void *data;
  void *(*threadfunc)(void *);
  _Atomic int initialized;
};

struct xmit_queue_slot {
  char pad[MEMCPY_ALIGNMENT - sizeof(_Atomic size_t) - sizeof(uint16_t)];
  uint16_t len;
  struct ether_header ether_h;
  unsigned char data[];
} __attribute__((__packed__));

#endif
