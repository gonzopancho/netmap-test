#ifndef _MESSAGE_
#define _MESSAGE_

#include <assert.h>
#include <stdint.h>   // uintXX_t
#include <stddef.h>   // size_t
#include <limits.h>   // CHAR_BIT
#include "common.h"

#define MSG_BLOCKSIZE 16

typedef enum {
  MSG_GROUP_GENERAL = 0x01
} msg_group;

typedef enum {
  MSG_TRANSACTION_UPDATE_SINGLE = 0x01,
  MSG_TRANSACTION_UPDATE        = 0x02,
  MSG_TRANSACTION_UPDATE_DATA   = 0x03
} msg_type;

struct msg_hdr {
  msg_type msg_type;
  msg_group msg_group;
  uint16_t num_blocks;
} __attribute__((__packed__));

struct msg_transaction_update_single {
  struct msg_hdr header;
  uint16_t pad; 
  uint64_t ring_idx;
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
                                    uint64_t ring_idx);
int send_transaction_update(struct thread_context *context, uint32_t *bitmap, 
                            size_t nbits);
#endif
