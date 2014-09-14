#include "arpd.h"

void *arpd(void *threadargs) {
	struct arp_data *arp_data = (struct arp_data *)threadargs;
	printf("arpd(): thread_id: %d\n", arp_data->thread_id);
	pthread_exit(NULL);
}




#if 0
void *arpd(void *threadargs) {
	struct arp_data *arp_data = (struct arp_data *)threadargs;
	size_t ether_arp_len;
    struct ethernet_pkt *arp_request_template;
    struct ethernet_pkt *arp_reply_template;
    struct ethernet_pkt *arp_temp;

	/* FIXME: pass these two in threadargs */
	struct if_info ifi;
    struct inet_info ineti;

	ether_arp_len = sizeof(struct ether_header) + sizeof(struct arp_pkt);
    if (ether_arp_len < ETHER_MIN_LEN - ETHER_CRC_LEN)
        ether_arp_len = ETHER_MIN_LEN - ETHER_CRC_LEN;

    arp_request_template = calloc(1, ether_arp_len);
    if (!arp_request_template) {
        fprintf(stderr, "Error allocating arp_request_template\n");
        exit(1);
    }
    arp_create_request_template(arp_request_template, &ifi.mac, &ineti.addr);

    arp_reply_template = calloc(1, ether_arp_len);
    if (!arp_reply_template) {
        fprintf(stderr, "Error allocating arp_reply_template\n");
        exit(1);
    }
    arp_create_reply_template(arp_reply_template, &ifi.mac, &ineti.addr);

    arp_temp = malloc(ether_arp_len);
    if (!arp_temp) {
        fprintf(stderr, "Error allocating arp_temp\n");
        exit(1);
    }


        for (; rxring->avail > 0; rxring->avail--) {
            i = rxring->cur;
            rxring->cur = NETMAP_RING_NEXT(rxring, i);
            buf = NETMAP_BUF(rxring, rxring->slot[i].buf_idx);
            etherpkt = (struct ethernet_pkt *)(void *)buf;
            if (!ethernet_is_valid(etherpkt, &ifi.mac)) {
                continue;
            }

            if (etherpkt->h.ether_type != ARP_ETHERTYPE)
                continue;

            arp = (struct arp_pkt*) etherpkt->data;
            if (!arp_is_valid(arp))
                continue;

            if (arp->arp_h.ar_op == ARP_OP_REQUEST) {
                if(arp->tpa.s_addr != ineti.addr.s_addr)
                    continue;

                /* send a reply for this request */
                memcpy(arp_temp, arp_reply_template, ether_arp_len);
                arp_update_reply(arp_temp, &arp->spa, &arp->sha);
                transmit_enqueue(txring, arp_temp, ether_arp_len);
                //ioctl(pfd.fd, NIOCTXSYNC, NULL);
            } else {
                if (!arp_reply_filter(arp, &ineti.addr))
                    continue;
                // check if replying for an IP that I want
                // then add to arp cache
            }
        } // for rxring

}
#endif
