#ifndef _ARP_
#define _ARP_

#include "ethernet.h"
#include "ip4.h"

#include <sys/socket.h> // sockaddr, required for <net/if_arp.h>
#include <net/if_arp.h> 
#include <stdio.h>      // printf

#if BYTE_ORDER == LITTLE_ENDIAN
#define ARP_HAF_ETHER     0x0100

#define ARP_OP_REQUEST    0x0100
#define ARP_OP_REPLY      0x0200
#define ARP_OP_REVREQUEST 0x0300
#define ARP_OP_REVREPLY   0x0400
#define ARP_OP_INVREQUEST 0x0800
#define ARP_OP_INVREPLY   0x0900
#endif

#if BYTE_ORDER == BIG_ENDIAN
#define ARP_HAF_ETHER     ARPHRD_ETHER

#define ARP_OP_REQUEST    ARPOP_REQUEST
#define ARP_OP_REPLY      ARPOP_REPLY
#define ARP_OP_REVREQUEST ARPOP_REVREQUEST
#define ARP_OP_REVREPLY   ARPOP_REVREPLY
#define ARP_OP_INVREQUEST ARPOP_INVREQUEST
#define ARP_OP_INVREPLY   ARPOP_INVREPLY
#endif


/* assumes ethernet hw type and IPv4 proto type*/
struct arp_pkt {
  struct arphdr arp_h; 
  struct ether_addr sha;
  struct in_addr spa;
  struct ether_addr tha;
  struct in_addr tpa;
} __attribute__((__packed__));

int arp_is_valid(struct arp_pkt *arp);
int arp_reply_filter (struct arp_pkt *arp, struct in_addr *my_ip);
void arp_print(struct arp_pkt *arp);
void arp_create_request_template(struct ethernet_pkt *pkt, 
                struct ether_addr *src_mac, 
                struct in_addr *src_ip);
void arp_update_request(struct ethernet_pkt *pkt, 
            struct in_addr *target_ip);
void arp_create_reply_template(struct ethernet_pkt *pkt, 
                struct ether_addr *src_mac, 
                struct in_addr *src_ip);
void arp_update_reply(struct ethernet_pkt *pkt, 
            struct in_addr *target_ip, 
            struct ether_addr *target_mac);

#endif
