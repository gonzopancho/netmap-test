#include <sys/types.h>
#include <net/if.h>
#include <net/netmap.h>
#include <net/netmap_user.h>
#include <sys/ioctl.h> 
#include <sys/mman.h>
#include <fcntl.h>  // open
#include <stdio.h>  // fprintf, perror
#include <string.h>   // strcpy
#include <poll.h>
#include <unistd.h>   // sleep, close
#include <stdlib.h>   // exit
#include <ifaddrs.h>  // getifaddrs
#include <net/if_dl.h>  // sockaddr_dl
#include "ethernet.h"
#include "arp.h"
#include "ip4.h"


struct if_info {
  struct ether_addr mac;  /* eg ff:ff:ff:ff:ff:ff */
  char *ifname;     /* eg em0 */
  uint32_t mtu;     /* eg 1500 */
  uint16_t link_type;   /* eg ARP_HAF_ETHER */
};

struct inet_info {
  struct in_addr addr;
  struct in_addr network;
  struct in_addr netmask;
  struct in_addr broadcast;
  struct in_addr default_route;
};


void print_nmreq(struct nmreq *req);
void print_netmap_if(struct netmap_if *nif);
void print_ring(struct netmap_ring *ring, uint32_t ridx);
void print_ring2(struct netmap_ring *ring, uint32_t ridx);
void print_buf(char *buf, uint16_t len);
void print_buf2(char *buf, uint16_t len); 
int get_if_hwaddr(const char* if_name, struct ether_addr *addr);
void dispatch(struct ethernet_pkt *pkt, uint16_t len);
int init_if_info(struct if_info *ifi, const char *ifname);
void print_if_info(struct if_info *ifi);
int init_inet_info(struct inet_info *ineti, char *addr, char *netmask, char *default_route);
void print_inet_info(struct inet_info *ineti);
int transmit_enqueue(struct netmap_ring *ring, struct ethernet_pkt *pkt, uint16_t pktlen);


int main() {
  struct netmap_if *nifp;
  struct netmap_ring *rxring;
  struct netmap_ring *txring;
  struct nmreq req;
  struct pollfd pfd;
  struct if_info ifi;
  struct inet_info ineti;
  int fd, retval;
  uint32_t i;
  char *buf;
  void *mem;
  char *ifname = "em0";
  struct ethernet_pkt *etherpkt;
  struct ethernet_pkt *arp_request_template;
  struct ethernet_pkt *arp_reply_template;
  struct ethernet_pkt *arp_temp;
  size_t ether_arp_len;
  struct arp_pkt *arp;

  if (!init_if_info(&ifi, "em0")) {
    fprintf(stderr, "if_info_init failed\n");
    exit(1);
  }

  print_if_info(&ifi);

  if (!init_inet_info(&ineti, "192.168.37.111", "255.255.255.0", "192.168.37.1")) {
    fprintf(stderr, "init_inet_info failed\n");
    exit(1);
  } 

  print_inet_info(&ineti);

  ether_arp_len = sizeof(struct ether_header) + sizeof(struct arp_pkt);
  if (ether_arp_len < ETHER_MIN_LEN - ETHER_CRC_LEN)
    ether_arp_len = ETHER_MIN_LEN - ETHER_CRC_LEN;

  arp_request_template = calloc(1, ether_arp_len);
  if (!arp_request_template) {
    fprintf(stderr, "Error allocating arp_request_template\n");
    exit(1);
  }
  arp_create_request_template(arp_request_template, &ifi.mac, &ineti.addr); 

  arp_reply_template = calloc(1, ether_arp_len);
  if (!arp_reply_template) {
    fprintf(stderr, "Error allocating arp_reply_template\n");
    exit(1);
  }
  arp_create_reply_template(arp_reply_template, &ifi.mac, &ineti.addr);

  arp_temp = malloc(ether_arp_len);
  if (!arp_temp) {
    fprintf(stderr, "Error allocating arp_temp\n");
    exit(1);
  }

  fd = open("/dev/netmap", O_RDWR);
  if (fd < 0) {
    fprintf(stderr, "Error opening /dev/netmap: %d\n", fd);
    close(fd);
    exit(1);
  }
  
  pfd.fd = fd;
  pfd.events = (POLLIN);

  bzero(&req, sizeof(req));
  strncpy(req.nr_name, ifname, sizeof(req.nr_name));
  req.nr_version = NETMAP_API;

  /* register the NIC for netmap mode */
  retval = ioctl(fd, NIOCREGIF, &req);
  if (retval < 0) {
    perror("NIOCREGIF failed");
  }

  /* give the NIC some time to unregister from the host stack */
  sleep(2);

  printf("After registration\n");
  print_nmreq(&req);

  mem = mmap(0, req.nr_memsize, PROT_READ|PROT_WRITE, MAP_SHARED, fd, 0);

  if (mem == MAP_FAILED) {
    perror("mmap failed");
  }

  nifp = NETMAP_IF(mem, req.nr_offset);
  print_netmap_if(nifp);

  rxring = NETMAP_RXRING(nifp, 0);
  txring = NETMAP_TXRING(nifp, 0);
  print_ring(rxring, 0);


  /* main poll loop */
  for(;;) {
    retval = poll(&pfd, 1, INFTIM);
    if (retval < 0) {
      perror("poll failed");
      continue;
    }

    for (; rxring->avail > 0; rxring->avail--) {
      i = rxring->cur;
      rxring->cur = NETMAP_RING_NEXT(rxring, i);
      buf = NETMAP_BUF(rxring, rxring->slot[i].buf_idx);
      etherpkt = (struct ethernet_pkt *)(void *)buf;
      if (!ethernet_is_valid(etherpkt, &ifi.mac)) {
        continue;
      }
  
      if (etherpkt->h.ether_type != ARP_ETHERTYPE)
        continue;
  
      arp = (struct arp_pkt*) etherpkt->data;
      if (!arp_is_valid(arp))
        continue;
    
      if (arp->arp_h.ar_op == ARP_OP_REQUEST) { 
        if(arp->tpa.s_addr != ineti.addr.s_addr)
          continue;
  
        /* send a reply for this request */
        memcpy(arp_temp, arp_reply_template, ether_arp_len);
        arp_update_reply(arp_temp, &arp->spa, &arp->sha);
        transmit_enqueue(txring, arp_temp, ether_arp_len);
        //ioctl(pfd.fd, NIOCTXSYNC, NULL);
      } else {
        if (!arp_reply_filter(arp, &ineti.addr))
          continue;
            // check if replying for an IP that I want
            // then add to arp cache
      }
    } // for rxring
  } // for(;;)

  return 0;
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


void dispatch(struct ethernet_pkt *pkt, uint16_t len) {
  struct arp_pkt *arp;

  switch (pkt->h.ether_type) {
    case IP4_ETHERTYPE:
      break;
    case ARP_ETHERTYPE:
      arp = (struct arp_pkt *)(pkt->data);
      if(arp_is_valid(arp)) {
        print_buf2((char *)pkt, len);
        arp_print(arp);
      }
      break;
    case IP6_ETHERTYPE:
    default:
      printf("DISPATCH: unknown ethertype\n");
  }
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

  ifi->mtu = 1500;
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
