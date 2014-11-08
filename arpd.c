#include "arpd.h"

void *arpd(void *threadarg) {
  assert(threadarg);

  struct thread_context *context;
  struct arpd_data *data;
  int rv;
  struct ethernet_pkt *arp_request_template;
  struct ethernet_pkt *arp_reply_template;
  struct ethernet_pkt *arp_temp;
  size_t ether_arp_len;

  struct transaction *transaction = NULL;
  struct netmap_slot *slot;
  struct ethernet_pkt *etherpkt;
  struct arp_pkt *arp;
  struct netmap_ring *rxring;
  uint32_t *slots_used;

  context = (struct thread_context *)threadarg;
  data = context->data;
  rxring = data->rxring;

  ether_arp_len = sizeof(struct ether_header) + sizeof(struct arp_pkt);
  if (ether_arp_len < ETHER_MIN_LEN - ETHER_CRC_LEN)
    ether_arp_len = ETHER_MIN_LEN - ETHER_CRC_LEN;

  arp_request_template = calloc(1, ether_arp_len);
  if (!arp_request_template) {
    fprintf(stderr, "Error allocating arp_request_template\n");
    exit(1);
  }
  arp_create_request_template(arp_request_template, data->mac, data->addr); 

  arp_reply_template = calloc(1, ether_arp_len);
  if (!arp_reply_template) {
    fprintf(stderr, "Error allocating arp_reply_template\n");
    exit(1);
  }
  arp_create_reply_template(arp_reply_template, data->mac, data->addr);

  arp_temp = malloc(ether_arp_len);
  if (!arp_temp) {
    fprintf(stderr, "Error allocating arp_temp\n");
    exit(1);
  }

  slots_used = bitmap_new(rxring->num_slots);
  if (!slots_used)
    exit(1);

  rv = arpd_init(context);

  if (!rv) {
    pthread_exit(NULL);
  }

  printf("arpd[%d]: initialized\n", context->thread_id);
  // signal to main() that we are initialized
  atomic_store_explicit(&context->initialized, 1, memory_order_release);

  // main event loop
  for (;;) {
    // read all the incoming packets
    while (tqueue_remove(context->pkt_recv_q, &transaction, 
            (void **) &slot) > 0) {
      etherpkt = (struct ethernet_pkt *) NETMAP_BUF(rxring, slot->buf_idx);
      bitmap_set(slots_used , slot->buf_idx);
      arp = (struct arp_pkt*) etherpkt->data;

      if (!arp_is_valid(arp))
        continue;

      if (arp->arp_h.ar_op == ARP_OP_REQUEST) {
        if (arp->tpa.s_addr != data->addr->s_addr)
          continue;

        /* send a reply for this request */
        memcpy(arp_temp, arp_reply_template, ether_arp_len);
        arp_update_reply(arp_temp, &arp->spa, &arp->sha);
        //transmit_enqueue(txring, arp_temp, ether_arp_len);
      } else {
        // check if replying for an IP that I want
        if (!arp_reply_filter(arp, data->addr))
          continue;

        // TODO: then add to arp cache
      }

      // TODO: send slots_used to dispatcher when transaction is done
    }

    // read all the messages

    // sleep
    sleep(1);
  } // for (;;)

  pthread_exit(NULL);
}

/*! allocate and initialize queues, etc
  \param[in, out] context
  \returns 1 on success, 0 on failure
*/
int arpd_init(struct thread_context *context) {
  assert(context);

  struct arpd_data *data = context->data;

  context->msg_q = squeue_new(data->msg_q_capacity, data->msg_q_elem_size);
  if (!context->msg_q)
    return 0;

  context->pkt_xmit_q = tqueue_new(data->xmit_q_transactions,
                                    data->xmit_q_actions_per_transaction);
  if (!context->pkt_xmit_q) {
    squeue_delete(&context->msg_q);
    return 0;
  }

  context->pkt_recv_q = tqueue_new(data->recv_q_transactions,
                                    data->recv_q_actions_per_transaction);
  if (!context->pkt_recv_q) {
    squeue_delete(&context->msg_q);
    tqueue_delete(&context->pkt_recv_q);
    return 0;
  }

  return 1;
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
