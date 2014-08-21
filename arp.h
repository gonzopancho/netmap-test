#include <sys/types.h>	// u_char, required for <net/ethernet.h>
#include <net/ethernet.h> //ether_header

#include <sys/socket.h> // sockaddr, required for <net/if_arp.h>
#include <net/if_arp.h> 
#include <netinet/in.h> // in_addr

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
