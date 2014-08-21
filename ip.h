#include <sys/types.h>	// u_char, required for <net/ethernet.h>
#include <net/ethernet.h> //ether_header

#include <netinet/in.h> // IP protos, required for <netinet/ip.h>
#include <netinet/ip.h> // ip

/* assumes no VLAN tags or IP options */
struct pkt_ip {
	struct ether_header ether_h;
	struct ip ip_h;
} __attribute__((__packed__));

int ip_is_valid(struct pkt_ip *ip);
