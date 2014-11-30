#ifndef _IP4_
#define _IP4_

#include "ethernet.h"
#include <netinet/in.h> // IP protos, required for <netinet/ip.h>
#include <netinet/ip.h> // ip
#include <arpa/inet.h>  // inet_aton_r
#include <stdio.h>      // printf
#include <inttypes.h>   // PRI macros



/* assumes no VLAN tags or IP options */
struct ip4_pkt {
  struct ip ip_h;
  unsigned char data[];
} __attribute__((__packed__));

int ip4_is_valid(struct ip4_pkt *ip);
void ip4_print(struct ip4_pkt *ip);
void ip4_print_line(struct ip4_pkt *ip);
void in_addr_print(struct in_addr *addr);

#endif
