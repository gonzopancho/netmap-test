#include "ethernet.h"


struct ether_addr ETHER_ADDR_BROADCAST = {
					.octet = { 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF } };
struct ether_addr ETHER_ADDR_ZERO = {
					.octet = { 0x0, 0x0, 0x0, 0x0, 0x0, 0x0 } };

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
	return ethernet_addr_equals(addr, &ETHER_ADDR_BROADCAST);
}

int ethernet_addr_equals(const struct ether_addr *a, 
						const struct ether_addr *b) {
	if (memcmp(a, b, ETHER_ADDR_LEN) == 0)
		return 1;
	else
		return 0;
}
