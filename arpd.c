#include "arpd.h"

void *arpd(void *threadarg) {
  assert(threadarg);

  struct thread_context *context;
  struct arpd_data *data;
  int rv;

  struct transaction *transaction = NULL;
  struct ethernet_pkt *etherpkt;
  struct arp_pkt *arp;
  struct netmap_ring *rxring;
  void *ring_idx;
  uint32_t dispatcher_idx;

  context = (struct thread_context *)threadarg;
  data = context->data;
  rxring = data->rxring;
  dispatcher_idx = context->shared->dispatcher_idx;

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
            &ring_idx) > 0) {
      etherpkt = (struct ethernet_pkt *) NETMAP_BUF(rxring, 
                                    rxring->slot[(uint32_t)ring_idx].buf_idx);
      arp = (struct arp_pkt*) etherpkt->data;

      if (!arp_is_valid(arp)) {
        send_transaction_update_single(&context->contexts[dispatcher_idx], 
                                        (uint32_t) ring_idx);
        continue;
      }

      arp_print_line(arp);

      if (arp->arp_h.ar_op == ARP_OP_REQUEST) {
        if (arp->tpa.s_addr != data->addr->s_addr) {
          send_transaction_update_single(&context->contexts[dispatcher_idx],
                                          (uint32_t) ring_idx);
          continue;
        }

        // TODO: send a reply for this request
      } else {
        // check if replying for an IP that I want
        if (!arp_reply_filter(arp, data->addr)) {
          send_transaction_update_single(&context->contexts[dispatcher_idx],
                                          (uint32_t) ring_idx);
          continue;
        }

        // TODO: then add to arp cache
      }

      send_transaction_update_single(&context->contexts[dispatcher_idx],
                                      (uint32_t) ring_idx);
    }

    // TODO: read all the messages

    sleep(1);
  } // for (;;)

  pthread_exit(NULL);
}

/*! allocate and initialize queues, etc
  \param[in, out] context
  \returns 1 on success, 0 on failure
*/
int arpd_init(struct thread_context *context) {
  struct arpd_data *data;
  int rv;

  assert(context);
  data = context->data;

  context->msg_q = squeue_new(data->msg_q_capacity, data->msg_q_elem_size);
  if (!context->msg_q)
    return 0;

  context->pkt_xmit_q = cqueue_spsc_new(data->xmit_q_capacity,
                                        data->xmit_q_elem_size);
  if (!context->pkt_xmit_q) {
    squeue_delete(&context->msg_q);
    return 0;
  }

  rv = xmit_queue_init(context->pkt_xmit_q, data->addr, data->mac);
  if (!rv) {
    squeue_delete(&context->msg_q);
    return 0;
  }

  context->pkt_recv_q = tqueue_new(data->recv_q_transactions,
                                    data->recv_q_actions_per_transaction);
  if (!context->pkt_recv_q) {
    squeue_delete(&context->msg_q);
    cqueue_spsc_delete(&context->pkt_xmit_q);
    return 0;
  }

  return 1;
}

// initialize each slot in the xmit queue with ethernet header and
// arp data that is consistent between requests and replies
int xmit_queue_init(cqueue_spsc *q,
                    struct in_addr *my_ip, struct ether_addr *my_mac) {
  struct xmit_queue_slot *s;
  struct arp_pkt *arp;
  size_t ether_arp_len, i, start_idx;

  ether_arp_len = sizeof(struct ether_header) + sizeof(struct arp_pkt);
  if (ether_arp_len < ETHER_MIN_LEN - ETHER_CRC_LEN)
    ether_arp_len = ETHER_MIN_LEN - ETHER_CRC_LEN;

  start_idx = q->push_idx;
  for (i=0; i < q->capacity; i++) {
    s = cqueue_spsc_push_slot(q);
    if (!s)
      return 0;

    s->len = ether_arp_len;

    /* ethernet header */
    memcpy(s->ether_h.ether_shost, my_mac, sizeof(struct ether_addr));
    s->ether_h.ether_type = ARP_ETHERTYPE;
    // ether_dhost changes in a request or reply

    /* arp */
    arp = (struct arp_pkt *)s->data;
    arp->arp_h.ar_hrd = ARP_HAF_ETHER;
    arp->arp_h.ar_pro = IP4_ETHERTYPE;
    arp->arp_h.ar_hln = ETHER_ADDR_LEN;
    arp->arp_h.ar_pln = sizeof(struct in_addr);
    // ar_op changes for request/reply
    memcpy(&arp->sha, my_mac, sizeof(struct ether_addr));
    arp->spa.s_addr = my_ip->s_addr;
    // tha and tpa change in a request/reply

    cqueue_spsc_push_slot_finish(q);

    // reset the slot used flag without touching the data
    s = cqueue_spsc_pop_slot(q);
    if (!s)
      return 0;
    cqueue_spsc_pop_slot_finish(q);
  }

  // detect if something else pushed while we were initializing
  if (q->push_idx != start_idx)
    return 0;

  return 1;
}

int send_arp_request(cqueue_spsc *q, struct in_addr *target_ip) {
  struct xmit_queue_slot *s;
  struct arp_pkt *request;

  s = cqueue_spsc_push_slot(q);
  if (!s)
    return 0;

  /* ethernet header */
  memcpy(s->ether_h.ether_dhost, &ETHER_ADDR_BROADCAST,
          sizeof(struct ether_addr));

  /* arp */
  request = (struct arp_pkt *)s->data;
  request->arp_h.ar_op = ARP_OP_REQUEST;
  memset(&request->tha, 0, sizeof(struct ether_addr));
  request->tpa.s_addr = target_ip->s_addr;

  cqueue_spsc_push_slot_finish(q);
  return 1;
}

int send_arp_reply(cqueue_spsc *q,
                    struct in_addr *target_ip, struct in_addr *target_mac) {
  struct xmit_queue_slot *s;
  struct arp_pkt *reply;

  s = cqueue_spsc_push_slot(q);
  if (!s)
    return 0;

  /* ethernet header */
  memcpy(s->ether_h.ether_dhost, target_mac, sizeof(struct ether_addr));

  /* arp */
  reply = (struct arp_pkt *)s->data;
  reply->arp_h.ar_op = ARP_OP_REPLY;
  // tha and tpa change in a reply
  memcpy(&reply->tha, target_mac, sizeof(struct ether_addr));
  reply->tpa.s_addr = target_ip->s_addr;

  cqueue_spsc_push_slot_finish(q);
  return 1;
}
