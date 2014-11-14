#include <sys/types.h>
#include <net/if.h>
#include <net/netmap.h>
#include <net/netmap_user.h>
#include <sys/ioctl.h> 
#include <sys/mman.h>
#include <fcntl.h>      // open
#include <stdio.h>      // fprintf, perror
#include <string.h>     // strcpy
#include <unistd.h>     // sleep, close
#include <stdlib.h>     // exit
#include <ifaddrs.h>    // getifaddrs
#include <net/if_dl.h>  // sockaddr_dl
#include <pthread.h>
#include "common.h"
#include "worker.h"
#include "dispatcher.h"
#include "arpd.h"
#include "message.h"

#define NUM_WORKERS 4
#define NUM_THREADS (NUM_WORKERS + 2)


void print_nmreq(struct nmreq *req);
void print_netmap_if(struct netmap_if *nif);
void print_ring(struct netmap_ring *ring, uint32_t ridx);
void print_ring2(struct netmap_ring *ring, uint32_t ridx);
void print_buf(char *buf, uint16_t len);
void print_buf2(char *buf, uint16_t len); 
int get_if_hwaddr(const char* if_name, struct ether_addr *addr);
int init_if_info(struct if_info *ifi, const char *ifname);
void print_if_info(struct if_info *ifi);
int init_inet_info(struct inet_info *ineti, char *addr, char *netmask, char *default_route);
void print_inet_info(struct inet_info *ineti);
int transmit_enqueue(struct netmap_ring *ring, struct ethernet_pkt *pkt, uint16_t pktlen);
int init_netmap(int *fd, char *ifname, void **mem, struct netmap_if **nifp);

int main() {
  struct shared_context shared;
  struct thread_context contexts[NUM_THREADS];
  struct worker_data worker_data[NUM_WORKERS];
  struct arpd_data arpd_data;
  struct dispatcher_data dispatcher_data;

  struct netmap_if *nifp = NULL;
  struct netmap_ring *rxring, *txring;

  struct if_info ifi;
  struct inet_info ineti;
  int fd, retval;
  uint32_t i;
  void *mem = NULL;
  char *ifname = "em0";

  if (!init_if_info(&ifi, "em0")) {
    fprintf(stderr, "if_info_init failed\n");
    exit(1);
  }

  print_if_info(&ifi);

  if (!init_inet_info(&ineti, "192.168.37.111", 
                      "255.255.255.0", "192.168.37.1")) {
    fprintf(stderr, "init_inet_info failed\n");
    exit(1);
  } 

  print_inet_info(&ineti);

  if (!init_netmap(&fd, ifname, &mem, &nifp)) {
    fprintf(stderr, "init_netmap failed\n");
    exit(1);
  }

  rxring = NETMAP_RXRING(nifp, 0);
  txring = NETMAP_TXRING(nifp, 0);

  /* shared variables */
  shared.contexts = contexts;
  shared.num_threads = NUM_THREADS;
  shared.arpd_idx = NUM_WORKERS;
  shared.dispatcher_idx = NUM_WORKERS + 1;

  /* generic context initialization */
  for (i=0; i < NUM_THREADS; i++) {
    contexts[i].num_threads = NUM_THREADS;
    contexts[i].contexts = contexts;
    contexts[i].shared = &shared;
    contexts[i].pkt_xmit_q = NULL;
    contexts[i].pkt_recv_q = NULL;
    contexts[i].msg_q = NULL;
    contexts[i].data = NULL;
    contexts[i].initialized = ATOMIC_VAR_INIT(0);
  }

  /* initialize workers */
  for (i=0; i < NUM_WORKERS; i++) {
    contexts[i].thread_id = i;
    contexts[i].threadfunc = worker;
    contexts[i].thread_type = WORKER;
    contexts[i].data = &worker_data[i];
    worker_data[i].msg_q_capacity = 256;
    worker_data[i].msg_q_elem_size = MSG_BLOCKSIZE;
    worker_data[i].xmit_q_capacity = 1024;
    worker_data[i].xmit_q_elem_size = sizeof(struct xmit_queue_slot) + MTU;
    worker_data[i].recv_q_transactions = 32;
    worker_data[i].recv_q_actions_per_transaction = 32;
    printf("main(): creating worker %d\n", i);
    retval = pthread_create(&contexts[i].thread, NULL, contexts[i].threadfunc,
                            (void *) &contexts[i]);
    if (retval) {
      fprintf(stderr,"ERROR: return code for pthread_create is %d\n", retval);
      exit(-1);
    }
  }

  /* wait for all workers to finish their initialization */
  for (i=0; i < NUM_WORKERS; i++) {
    while (atomic_load_explicit(&contexts[i].initialized, 
                memory_order_acquire) == 0);
  }

  /* initialize arpd */
  i = NUM_WORKERS;
  contexts[i].thread_id = i;
  contexts[i].threadfunc = arpd;
  contexts[i].thread_type = ARPD;
  contexts[i].data = &arpd_data;
  arpd_data.msg_q_capacity = 256;
  arpd_data.msg_q_elem_size = MSG_BLOCKSIZE;
  arpd_data.xmit_q_capacity = 64;
  arpd_data.xmit_q_elem_size = sizeof(struct xmit_queue_slot) + 
                                sizeof(struct arp_pkt);
  arpd_data.recv_q_transactions = 64;
  arpd_data.recv_q_actions_per_transaction = 1;
  arpd_data.mac = &ifi.mac;
  arpd_data.addr = &ineti.addr;
  arpd_data.rxring = rxring;

  printf("main(): creating arpd\n");
  retval = pthread_create(&contexts[i].thread, NULL, contexts[i].threadfunc,
                          (void *) &contexts[i]);
  if (retval) {
    fprintf(stderr, "ERROR: return code for pthread_create is %d\n", retval);
    exit(-1);
  }

  /* wait for arpd to finish initialization */
  while (atomic_load_explicit(&contexts[i].initialized,
          memory_order_acquire) == 0);

  /* initialize dispatcher */
  i = NUM_WORKERS + 1;
  contexts[i].thread_id = i;
  contexts[i].threadfunc = dispatcher;
  contexts[i].thread_type = DISPATCHER;
  contexts[i].data = &dispatcher_data;
  dispatcher_data.msg_q_capacity = 1024;
  dispatcher_data.msg_q_elem_size = MSG_BLOCKSIZE;
  dispatcher_data.fd = fd;
  dispatcher_data.nifp = nifp;
  dispatcher_data.ifi = &ifi;

  printf("main(): creating dispatcher thread\n");
  retval = pthread_create(&contexts[i].thread, NULL, contexts[i].threadfunc,
                          (void *) &contexts[i]);
  if (retval) {
    fprintf(stderr, "ERROR: return code for pthread_create is %d\n", retval);
    exit(-1);
  }

  /* wait for dispatcher to finish initialization */
  while (atomic_load_explicit(&contexts[i].initialized,
          memory_order_acquire) == 0);

  /* wait for all threads to exit */
  sleep(5);
  for (i=0; i < NUM_THREADS; i++) {
    retval = pthread_join(contexts[i].thread, NULL);
    if (retval) {
      fprintf(stderr, "ERROR: pthread_join[%d] returned %d \n", i, retval);
      exit(-2);
    }
  }
  printf("threads finished joining\n");

  pthread_exit(NULL);
}


void print_nmreq(struct nmreq *req) {
  printf("nr_name: %s\n", req->nr_name);
  printf("nr_version: %u\n", req->nr_version);
  printf("nr_offset: %u\n", req->nr_offset);
  printf("nr_memsize: %u\n", req->nr_memsize);
  printf("nr_tx_slots: %u\n", req->nr_tx_slots);
  printf("nr_rx_slots: %u\n", req->nr_rx_slots);
  printf("nr_tx_rings: %hu\n", req->nr_tx_rings);
  printf("nr_rx_rings: %hu\n", req->nr_rx_rings);
  printf("nr_ringid: %hu\n", req->nr_ringid);
  printf("nr_cmd: %hu\n", req->nr_cmd);
  printf("nr_arg1: %hu\n", req->nr_arg1);
  printf("nr_arg2: %hu\n", req->nr_arg2);
  printf("spare2: %u %u %u\n", req->spare2[0], req->spare2[1], req->spare2[2]);
}

void print_netmap_if(struct netmap_if *nif) {
  printf("ni_name: %s\n", nif->ni_name);
  printf("ni_version: %u\n", nif->ni_version);
  printf("ni_rx_rings: %u\n", nif->ni_rx_rings);
  printf("ni_tx_rings: %u\n", nif->ni_tx_rings);
  printf("ring_ofs: %zd\n", nif->ring_ofs[0]);
}

void print_ring(struct netmap_ring *ring, uint32_t ridx) {
  printf("ring: %u\n", ridx);
  printf("buf_ofs: %zd\n", ring->buf_ofs);
  printf("num_slots: %u\n", ring->num_slots);
  printf("avail: %u\n", ring->avail);
  printf("cur: %u\n", ring->cur);
  printf("reserved: %u\n", ring->reserved);
  printf("nr_buf_size: %hu\n", ring->nr_buf_size);
  printf("flags: %hu\n", ring->flags);
  /* printf("ts: unknown\n"); */
}

void print_ring2(struct netmap_ring *ring, uint32_t ridx) {
  printf("ring: %u avail: %u cur: %u res: %u\n", 
        ridx, ring->avail, ring->cur, ring->reserved);
}

void print_buf(char *buf, uint16_t len) {
  uint16_t i;
  printf("*****buf(%hu)*****\n", len);

  for(i = 0; i + 8 < len; i+=8) {
    printf("%.2X %.2X %.2X %.2X %.2X %.2X %.2X %.2X\n", 
          buf[i] & 0xFF, buf[i+1] & 0xFF, buf[i+2] & 0xFF, buf[i+3] & 0xFF,
          buf[i+4] & 0xFF, buf[i+5] & 0xFF, buf[i+6] & 0xFF, buf[i+7] & 0xFF);
  }
  for(; i < len; i++) {
    printf("%.2X ", buf[i] & 0xFF);
  }
  printf("\n");

}

void print_buf2(char *buf, uint16_t len) {
  uint16_t i;
  printf("*****buf(%hu)*****\n", len);
  if ( len > 53)
    len = 53;
  for(i = 0; i < len; i++)
    printf("%.2x ", buf[i] & 0xFF);
  printf("\n"); 
}


/* returns 0 on error, 1 on success */
int get_if_hwaddr(const char* if_name, struct ether_addr *addr) {
  struct ifaddrs *ifas, *ifa;
  struct sockaddr_dl *sdl;

  if (getifaddrs(&ifas) != 0) {
    perror("getifaddrs failed");
    return 0;
  }

  for(ifa = ifas; ifa; ifa = ifa->ifa_next) {
    sdl = (struct sockaddr_dl *)(ifa->ifa_addr);
    if (!sdl || sdl->sdl_family != AF_LINK)
      continue;
    if (memcmp(if_name, sdl->sdl_data, sdl->sdl_nlen) != 0)
      continue;
    memcpy(addr, LLADDR(sdl), sizeof(struct ether_addr));
    break;
  }
  freeifaddrs(ifas);
  return ifa ? 1 : 0;
}


int init_if_info(struct if_info *ifi, const char *ifname) {
  size_t len;
  len = strnlen(ifname, IF_NAMESIZE);

  /* need space for NUL termination */
  if (len == IF_NAMESIZE)
    len++;

  ifi->ifname = malloc(len);
  if (!ifi->ifname)
    return 0;

  strncpy(ifi->ifname, ifname, len);

  ifi->mtu = MTU;
  return get_if_hwaddr(ifi->ifname, &ifi->mac);
}

void print_if_info(struct if_info *ifi) {
  char macstr[18];
  printf("Interface: %s, MAC: %s, MTU: %d\n", 
        ifi->ifname, 
        ether_ntoa_r(&ifi->mac, macstr), 
        ifi->mtu);
}

int init_inet_info(struct inet_info *ineti, char *addr, char *netmask, char *default_route) {
  if (!inet_aton(addr, &ineti->addr)) {
    return 0;
  }
  if (!inet_aton(netmask, &ineti->netmask)) {
    return 0;
  }
  if (!inet_aton(default_route, &ineti->default_route)) {
    return 0;
  }

  ineti->network.s_addr = ineti->addr.s_addr & ineti->netmask.s_addr;
  ineti->broadcast.s_addr = ineti->network.s_addr | (~ineti->netmask.s_addr);
  return 1;
}


void print_inet_info(struct inet_info *ineti) {
  char addr[4*4];
  char netmask[4*4];
  char network[4*4];
  char broadcast[4*4];
  char default_route[4*4];

  inet_ntoa_r(ineti->addr, addr, sizeof(addr));
  inet_ntoa_r(ineti->netmask, netmask, sizeof(netmask));
  inet_ntoa_r(ineti->network, network, sizeof(network));
  inet_ntoa_r(ineti->broadcast, broadcast, sizeof(broadcast));
  inet_ntoa_r(ineti->default_route, default_route, sizeof(default_route));

  printf("IP: %s, NETMASK: %s\n", addr, netmask);
  printf("NETWORK: %s, BROADCAST: %s\n", network, broadcast);
  printf("DEFAULT ROUTE: %s\n", default_route);
}


int transmit_enqueue(struct netmap_ring *ring, struct ethernet_pkt *pkt, 
                      uint16_t pktlen) {
  uint32_t cur;
  struct netmap_slot *slot;
  char *buf;

  if (ring->avail == 0)
    return 0;

  cur = ring->cur;
  slot = &ring->slot[cur];
  slot->flags = NS_REPORT;
  slot->len = pktlen;
  buf = NETMAP_BUF(ring, slot->buf_idx);
  memcpy(buf, pkt, pktlen);
  ring->avail--;
  ring->cur = NETMAP_RING_NEXT(ring, cur);

  return 1;
}


int init_netmap(int *fd, char *ifname, void **mem, struct netmap_if **nifp) {
  struct nmreq req;
  int retval;
  struct netmap_ring *rxring, *txring;

  *fd = open("/dev/netmap", O_RDWR);
  if (*fd < 0) {
    fprintf(stderr, "Error opening /dev/netmap: %d\n", *fd);
    close(*fd);
    return 0;
  }

  bzero(&req, sizeof(req));
  strncpy(req.nr_name, ifname, sizeof(req.nr_name));
  req.nr_version = 4;   // v4 shipped with FreeBSD 10

  /* register the NIC for netmap mode */
  retval = ioctl(*fd, NIOCREGIF, &req);
  if (retval < 0) {
    perror("NIOCREGIF failed");
    return 0;
  }

  /* give the NIC some time to unregister from the host stack */
  sleep(2);

  //printf("After registration\n");
  //print_nmreq(&req);

  *mem = mmap(0, req.nr_memsize, PROT_READ|PROT_WRITE, MAP_SHARED, *fd, 0);

  if (*mem == MAP_FAILED) {
    perror("mmap failed");
    return 0;
  }

  *nifp = NETMAP_IF(*mem, req.nr_offset);
  //print_netmap_if(*nifp);

  rxring = NETMAP_RXRING(*nifp, 0);
  rxring->avail = 0;
  rxring->cur = 0;
  rxring->reserved = 0;

  txring = NETMAP_TXRING(*nifp, 0);
  txring->avail = 0;
  txring->cur = 0;
  txring->reserved = 0;

  return 1;
}

