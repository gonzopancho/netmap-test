#ifndef _COMMON_
#define _COMMON_
#include <sys/types.h>          // u_char, required for <net/ethernet.h>
#include <net/ethernet.h>       // ether_header, ether_ntoa_r
#include <netinet/in.h> 		// IP protos, required for <netinet/ip.h>


struct if_info {
    struct ether_addr mac;  /* eg ff:ff:ff:ff:ff:ff */
    char *ifname;           /* eg em0 */
    uint32_t mtu;           /* eg 1500 */
    uint16_t link_type;     /* eg ARP_HAF_ETHER */
};

struct inet_info {
    struct in_addr addr;
    struct in_addr network;
    struct in_addr netmask;
    struct in_addr broadcast;
    struct in_addr default_route;
};


#endif
