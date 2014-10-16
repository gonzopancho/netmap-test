#ifndef _ETHERNET_
#define _ETHERNET_

#include <sys/types.h>      // u_char, required for <net/ethernet.h>
#include <net/ethernet.h>   // ether_header, ether_ntoa_r
#include <stdint.h>         // UINT64_MAX
#include <string.h>         // memcmp


#if BYTE_ORDER == LITTLE_ENDIAN
#define ARP_ETHERTYPE   0x0608
#define IP4_ETHERTYPE   0x0008
#define IP6_ETHERTYPE   0xDD86
#endif
#if BYTE_ORDER == BIG_ENDIAN
#define ARP_ETHERTYPE   ETHERTYPE_ARP
#define IP4_ETHERTYPE   ETHERTYPE_IP
#define IP6_ETHERTYPE   ETHERTYPE_IPV6
#endif


extern struct ether_addr ETHER_ADDR_BROADCAST;
extern struct ether_addr ETHER_ADDR_ZERO;

// technically a frame, but for the sake of consistency
struct ethernet_pkt {
  struct ether_header h;
  unsigned char data[];
} __attribute__((__packed__));


int ethernet_is_valid (const struct ethernet_pkt *pkt,
                       const struct ether_addr   *me);
int ethernet_addr_is_broadcast (const struct ether_addr *addr);
int ethernet_addr_equals(const struct ether_addr *a,
                        const struct ether_addr *b);

#endif
