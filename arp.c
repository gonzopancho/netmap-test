#include "arp.h"

int arp_is_valid(struct pkt_arp *arp) {
	/*  this is usually checked prior to calling this function anyway
	if (arp->ether_h.ether_type != ETHERTYPE_ARP)
		return 0;
	*/
	if (arp->arp_h.ar_hrd != ARPHRD_ETHER)
		return 0;
	if (arp->arp_h.ar_pro != ETHERTYPE_IP)
		return 0;
	if (arp->arp_h.ar_hln != ETHER_ADDR_LEN)
		return 0;
	if (arp->arp_h.ar_pln != sizeof(struct in_addr))
		return 0;
	if ( (arp->arp_h.ar_op != ARPOP_REQUEST) ||
		 (arp->arp_h.ar_op != ARPOP_REPLY))
		return 0;
	return 1;		
}
