#include "arp.h"

int arp_is_valid(struct arp_pkt *arp) {
	if (arp->arp_h.ar_hrd != ARP_HAF_ETHER)
		return 0;
	if (arp->arp_h.ar_pro != IP4_ETHERTYPE)
		return 0;
	if (arp->arp_h.ar_hln != ETHER_ADDR_LEN)
		return 0;
	if (arp->arp_h.ar_pln != sizeof(struct in_addr))
		return 0;
	if ( (arp->arp_h.ar_op != ARP_OP_REQUEST) && 
		 (arp->arp_h.ar_op != ARP_OP_REPLY))
		return 0;
	return 1;		
}

void arp_receive_handler(struct arp_pkt *arp, struct in_addr *my_ip) {
	switch(arp->arp_h.ar_op) {
		case ARP_OP_REQUEST:
			if(arp->tpa.s_addr == my_ip->s_addr) {
				// send a reply for this request
			} 
			break;
		case ARP_OP_REPLY:
			if (!arp_reply_filter(arp, my_ip)) {
				// check if replying for an IP that I want
				// then add to arp cache
			}
			break;
		default:
			break;
	}
}

int arp_reply_filter (struct arp_pkt *arp, struct in_addr *my_ip) {
	if (arp->tpa.s_addr != my_ip->s_addr)
		return 0;
	if (arp->arp_h.ar_op != ARP_OP_REPLY)
		return 0;
	if (ethernet_addr_is_broadcast(&arp->sha))
		return 0;
	if (ethernet_addr_equals(&arp->sha, &ETHER_ADDR_ZERO))
		return 0;
	return 1;
}

void arp_print(struct arp_pkt *arp) {
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
		case ARP_OP_REQUEST:
			printf("Request\n");
			break;
		case ARP_OP_REPLY:
			printf("Reply\n");
			break;
		case ARP_OP_REVREQUEST:
		case ARP_OP_REVREPLY:
		case ARP_OP_INVREQUEST:
		case ARP_OP_INVREPLY:
		default:
			printf("Unknown Operation\n");
	}
	ether_ntoa_r(&arp->sha, sha);
	inet_ntoa_r(arp->spa, spa, sizeof(spa));
	ether_ntoa_r(&arp->tha, tha);
	inet_ntoa_r(arp->tpa, tpa, sizeof(tpa));

	printf("  Sender: %s (%s)\n", spa, sha); 
	printf("  Target: %s (%s)\n", tpa, tha);
}

void arp_create_request_template(struct ethernet_pkt *pkt, 
								struct ether_addr *src_mac, 
								struct in_addr *src_ip) {
	struct arp_pkt *arp;

	/* ethernet header fields */
	memcpy(pkt->h.ether_dhost, &ETHER_ADDR_BROADCAST, 
			sizeof(ETHER_ADDR_BROADCAST));
	memcpy(pkt->h.ether_shost, src_mac, sizeof(struct ether_addr));
	pkt->h.ether_type = ARP_ETHERTYPE;

	/* arp fields */
	arp = (struct arp_pkt*) pkt->data;
	arp->arp_h.ar_hrd = ARP_HAF_ETHER;
	arp->arp_h.ar_pro = IP4_ETHERTYPE;
	arp->arp_h.ar_hln = ETHER_ADDR_LEN;
	arp->arp_h.ar_pln = sizeof(struct in_addr);
	arp->arp_h.ar_op = ARP_OP_REQUEST;
	memcpy(&arp->sha, src_mac, sizeof(struct ether_addr));
	arp->spa.s_addr = src_ip->s_addr;
	memset(&arp->tha, 0, sizeof(struct ether_addr));
	/* tpa is really the only field that changes in a request */
}

void arp_update_request(struct ethernet_pkt *pkt, 
						struct in_addr *target_ip) {
	struct arp_pkt *arp = (struct arp_pkt*) pkt->data;		
	arp->tpa.s_addr = target_ip->s_addr;
}

void arp_create_reply_template(struct ethernet_pkt *pkt, 
								struct ether_addr *src_mac, 
								struct in_addr *src_ip) {
    struct arp_pkt *arp;

    /* ethernet header fields */
	/* ether_dhost changes in a reply*/
    memcpy(pkt->h.ether_shost, src_mac, sizeof(struct ether_addr));
    pkt->h.ether_type = ARP_ETHERTYPE;

    /* arp fields */
    arp = (struct arp_pkt*) pkt->data;
    arp->arp_h.ar_hrd = ARP_HAF_ETHER;
    arp->arp_h.ar_pro = IP4_ETHERTYPE;
    arp->arp_h.ar_hln = ETHER_ADDR_LEN;
    arp->arp_h.ar_pln = sizeof(struct in_addr);
    arp->arp_h.ar_op = ARP_OP_REPLY;
    memcpy(&arp->sha, src_mac, sizeof(struct ether_addr));
	arp->spa.s_addr = src_ip->s_addr;
	/* tha and tpa change in a reply */
}

void arp_update_reply(struct ethernet_pkt *pkt, 
						struct in_addr *target_ip, 
						struct ether_addr *target_mac) {
	struct arp_pkt *arp = (struct arp_pkt*) pkt->data;
	memcpy(pkt->h.ether_dhost, target_mac, sizeof(struct ether_addr));
	memcpy(&arp->tha, target_mac, sizeof(struct ether_addr));
	arp->tpa.s_addr = target_ip->s_addr;
}

