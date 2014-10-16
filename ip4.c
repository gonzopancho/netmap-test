#include "ip4.h"

int ip4_is_valid(struct ip4_pkt *ip) {
  if (ip->ip_h.ip_hl != 5) // no options
    return 0;
  if (ip->ip_h.ip_v != IPVERSION) // ipv4
    return 0;
  if ((ip->ip_h.ip_p != IPPROTO_TCP) && 
      (ip->ip_h.ip_p != IPPROTO_UDP) && 
      (ip->ip_h.ip_p != IPPROTO_ICMP))
    return 0;

  return 1;   
}

void ip4_print(struct ip4_pkt *ip) {
  char ip_src[4*4];
  char ip_dst[4*4];
  uint16_t fragment;

  printf("IP4\n");
  printf("  version: %u\n", ip->ip_h.ip_v);
  printf("  header len: %u (32bit words)\n", ip->ip_h.ip_hl);
  printf("  TOS: %" PRIu8 "\n", ip->ip_h.ip_tos);
  printf("  total len: %hu\n", ntohs(ip->ip_h.ip_len));
  printf("  identification: %hu\n", ntohs(ip->ip_h.ip_id));
  printf("  fragment flags: ");
  fragment = ntohs(ip->ip_h.ip_off);
  printf("    reserved bit: %u ", fragment & IP_RF ? 1 : 0);
  printf("    DF bit: %u ", fragment & IP_DF ? 1 : 0);
  printf("    MF bit: %u\n", fragment & IP_MF ? 1 : 0);
  printf("  fragment offset: %hu\n", (unsigned short)(fragment & IP_OFFMASK));
  printf("  TTL: %" PRIu8 "\n", ip->ip_h.ip_ttl);
  printf("  protocol: ");

  switch (ip->ip_h.ip_p) {
    case IPPROTO_TCP:
      printf("TCP\n");
      break;
    case IPPROTO_UDP:
      printf("UDP\n");
      break;
    case IPPROTO_ICMP:
      printf("ICMP\n");
      break;
    default:
      printf("Unknown Protocol\n");
  }

  printf("  checksum: %hu\n", ntohs(ip->ip_h.ip_sum));
  printf("  src IP: %s\n", inet_ntoa_r(ip->ip_h.ip_src, ip_src, sizeof(ip_src)));
  printf("  dst IP: %s\n", inet_ntoa_r(ip->ip_h.ip_dst, ip_dst, sizeof(ip_dst)));
}

