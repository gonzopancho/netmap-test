#ifndef _MESSAGE_
#define _MESSAGE_

#include <assert.h>
#include <stdint.h>   // uintXX_t
#include <stddef.h>   // size_t
#include <limits.h>   // CHAR_BIT
#include "common.h"

#define MSG_BLOCKSIZE 16

enum msg_group {
  MSG_GROUP_GENERAL
};

enum msg_type {
  MSG_TRANSACTION_UPDATE_SINGLE,
  MSG_TRANSACTION_UPDATE,
  MSG_TRANSACTION_UPDATE_DATA
};

struct msg_hdr {
  uint16_t msg_type;
  uint16_t msg_group;
  uint16_t num_blocks;
} __attribute__((__packed__));

struct msg_transaction_update_single {
  struct msg_hdr header;
  uint16_t pad[3]; 
  uint32_t ring_idx;
} __attribute__((__packed__));

struct msg_transaction_update {
  struct msg_hdr header;
  uint16_t pad[5];
} __attribute__((__packed__));

struct msg_transaction_update_data {
  uint32_t data[4];
};


// assumes the squeue is locked
// returns 1 on success, 0 on fail
int send_transaction_update_single(struct thread_context *context, 
                                    uint32_t ring_idx);
int send_transaction_update(struct thread_context *context, uint32_t *bitmap, 
                            size_t nbits);
#endif
