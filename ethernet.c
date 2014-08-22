#include "ethernet.h"

int ethernet_is_valid (const struct ethernet_pkt *pkt, 
					   const struct ether_addr   *me) {

	if ((pkt->h.ether_type != IP4_ETHERTYPE) && 
		(pkt->h.ether_type != ARP_ETHERTYPE))
		return 0;

	if (!ethernet_addr_equals(me, (struct ether_addr*)(pkt->h.ether_dhost)) && 
		!ethernet_addr_is_broadcast((struct ether_addr*)(pkt->h.ether_dhost)))
		return 0;
	return 1;
}

int ethernet_addr_is_broadcast (const struct ether_addr *addr) {
	uint64_t broadcast = UINT64_MAX;
	if (memcmp(addr, &broadcast, ETHER_ADDR_LEN) == 0)
		return 1;
	else
		return 0;
}

int ethernet_addr_equals(const struct ether_addr *a, 
						const struct ether_addr *b) {
	if (memcmp(a, b, ETHER_ADDR_LEN) == 0)
		return 1;
	else
		return 0;
}
