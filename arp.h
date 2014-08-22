#include "ethernet.h"
#include "ip.h"

#include <sys/socket.h> // sockaddr, required for <net/if_arp.h>
#include <net/if_arp.h> 

#include <stdio.h> 		// printf
#include <stdlib.h> 	// free

/* assumes ethernet hw type and IPv4 proto type*/
struct pkt_arp {
	struct ether_header ether_h;
	struct arphdr arp_h; 
	struct ether_addr sha;
	struct in_addr spa;
	struct ether_addr tha;
	struct in_addr tpa;
} __attribute__((__packed__));

int arp_is_valid(struct pkt_arp *arp);
void arp_print(struct pkt_arp *arp);

