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

void arp_print(struct pkt_arp *arp) {
	char sha[6*3];
	char spa[4*4];
	char tha[6*3];
	char tpa[4*4];

	printf("ARP ");
	/*
	switch(arp->arp_h.ar_hrd) {
		case ARPHRD_ETHER:
		case ARPHRD_IEEE802:
		case ARPHRD_ARCNET:
		case ARPHRD_FRELAY:
		case ARPHRD_IEEE11394:
		case ARPHRD_INFINIBAND:
		default:
	}
	*/
	switch(arp->arp_h.ar_op) {
		case ARPOP_REQUEST:
			printf("Request\n");
			break;
		case ARPOP_REPLY:
			printf("Reply\n");
			break;
		case ARPOP_REVREQUEST:
		case ARPOP_REVREPLY:
		case ARPOP_INVREQUEST:
		case ARPOP_INVREPLY:
		default:
			printf("Unknown Operation\n");
	}
	ether_ntoa_r(&arp->sha, sha);
	inet_ntoa_r(arp->spa, spa, sizeof(spa));
	ether_ntoa_r(&arp->tha, tha);
	inet_ntoa_r(arp->tpa, tpa, sizeof(tpa));

	printf("  Sender: %s (%s)\n", spa, sha); 
	printf("  Target: %s (%s)\n", tpa, tha);

	free(sha);
	free(spa);
	free(tha);
	free(tpa);
}
