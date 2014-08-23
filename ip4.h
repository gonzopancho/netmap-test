#ifndef _IP4_
#define _IP4_

#include "ethernet.h"
#include <netinet/in.h> // IP protos, required for <netinet/ip.h>
#include <netinet/ip.h> // ip
#include <arpa/inet.h>	// inet_aton_r

/* assumes no VLAN tags or IP options */
struct ip4_pkt {
	struct ether_header ether_h;
	struct ip ip_h;
} __attribute__((__packed__));

int ip4_is_valid(struct ip4_pkt *ip);

#endif
