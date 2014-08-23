#ifndef _IP_
#define _IP_

#include "ethernet.h"
#include <netinet/in.h> // IP protos, required for <netinet/ip.h>
#include <netinet/ip.h> // ip
#include <arpa/inet.h>	// inet_aton_r

/* assumes no VLAN tags or IP options */
struct pkt_ip {
	struct ether_header ether_h;
	struct ip ip_h;
} __attribute__((__packed__));

int ip_is_valid(struct pkt_ip *ip);

#endif
