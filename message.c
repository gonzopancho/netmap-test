#include "message.h"

int send_msg_transaction_update_single(struct thread_context *context,
                                        uint32_t ring_idx) {
  assert(context);
  assert(context->msg_q);

  struct msg_transaction_update_single *msg;
  msg = squeue_push_slot(context->msg_q);
  msg->header.msg_type = MSG_TRANSACTION_UPDATE_SINGLE;
  msg->header.msg_group = MSG_GROUP_GENERAL;
  msg->header.num_blocks = 1;
  msg->ring_idx = ring_idx;
  squeue_exit(context->msg_q);

  return 1;
}

int send_msg_transaction_update(struct thread_context *context,
                                uint32_t *bitmap, size_t nbits) {
  assert(context);
  assert(context->msg_q);

  squeue *q = context->msg_q;
  struct msg_transaction_update_single *msg;
  struct msg_transaction_update_data *msg_data = NULL;
  size_t bitmap_blocks_per_data_block;
  uint16_t i, num_blocks;

  bitmap_blocks_per_data_block = (MSG_BLOCKSIZE * CHAR_BIT) / BITMAP_BLOCKSIZE;
  num_blocks = (BITMAP_BLOCKS(nbits) + bitmap_blocks_per_data_block - 1)
               / bitmap_blocks_per_data_block;
  num_blocks++;

  if (!squeue_enter(q, 1))
    return 0;

  if (num_blocks > q->capacity - q->elem_count) {
    squeue_exit(q);
    return 0;
  }

  msg = squeue_get_next_push_slot(q);
  if (!msg) {
    squeue_exit(q);
    return 0;
  }

  msg->header.msg_type = MSG_TRANSACTION_UPDATE;
  msg->header.msg_group = MSG_GROUP_GENERAL;
  msg->header.num_blocks = num_blocks;

  for (i=0; i < BITMAP_BLOCKS(nbits); i++) {
    if (i % bitmap_blocks_per_data_block == 0) {
      msg_data = squeue_get_next_push_slot(q);
      if (!msg_data) {
        squeue_exit(q);
        return 0;
      }
    }
    msg_data->data[i%bitmap_blocks_per_data_block] = bitmap[i];
  } 

  squeue_exit(q);
  return 1;
}

