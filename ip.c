#include "ip.h"

int ip_is_valid(struct pkt_ip *ip) {
	/*  this is usually checked prior to calling this function anyway
	if (ip->ether_h.ether_type != ETHERTYPE_IP)
		return 0;
	*/
	if (ip->ip_h.ip_hl != 5) // no options
		return 0;
	if (ip->ip_h.ip_v != IPVERSION) // ipv4
		return 0;
	if ( (ip->ip_h.ip_p != IPPROTO_TCP) ||
		(ip->ip_h.ip_p != IPPROTO_UDP) ||
		(ip->ip_h.ip_p != IPPROTO_ICMP) )
		return 0;
	return 1;		
}
