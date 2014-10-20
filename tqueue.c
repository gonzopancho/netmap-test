/*!
  \file
  \copyright Copyright (c) 2014, Richard Fujiyama
  Licensed under the terms of the New BSD license.
*/

#include "tqueue.h"

struct transaction {
  size_t write_idx;
  size_t read_idx;
  void *actions[];
};

struct tqueue {
  cqueue_spsc* transactions;
  size_t num_transactions;
  size_t num_actions;
};

// public function definitions
/*!
  Warning: this function relies on internal knowledge of cqueue_spsc
  to optimize num_actions
*/
tqueue *tqueue_new(size_t num_transactions, size_t num_actions) {
  tqueue *q;
  transaction *t;

  size_t transaction_size;

  if (!num_transactions)
    return NULL;

  if (!num_actions)
    return NULL;

  // check overflow
  if (num_actions > (SIZE_MAX - sizeof(transaction)) / sizeof (void *))
    return NULL;
  transaction_size = sizeof(transaction) + num_actions * sizeof(void *);

  q = malloc(sizeof(tqueue));
  if (!q)
    return NULL;

  q->transactions = cqueue_spsc_new(num_transactions, transaction_size);
  if (!q->transactions) {
    free(q);
    return NULL;
  }

  q->num_transactions = num_transactions;
  q->num_actions = (q->transactions->elem_size - sizeof(_Atomic size_t) 
                    - sizeof(transaction)) / sizeof(void *);

  // initialize the transactions
  while ((t = cqueue_spsc_trypush_slot(q->transactions)) != NULL) {
    t->write_idx = 0;
    t->read_idx = 0;
    cqueue_spsc_push_slot_finish(q->transactions);
  }

  while ((t = cqueue_spsc_trypop_slot(q->transactions)) != NULL) {
    cqueue_spsc_pop_slot_finish(q->transactions);
  }

  return q;
}

void tqueue_delete(tqueue **p) {
  tqueue *q = *p;

  if (!q)
    return;

  if (q->transactions)
    cqueue_spsc_delete(&q->transactions);

  free(q);
  *p = NULL;
}

int tqueue_insert(tqueue *q, transaction **tp, void *p) {
  assert(q);

  transaction *t = *tp;

  if (!t) {
    t = cqueue_spsc_trypush_slot(q->transactions);
    if (!t)  // transaction queue full
      return TQUEUE_FULL;
  }

  t->actions[t->write_idx] = p;
  t->write_idx++;

  if (t->write_idx == q->num_actions) { // this transaction is full
    t = NULL;
    cqueue_spsc_push_slot_finish(q->transactions);
    return TQUEUE_TRANSACTION_FULL;
  }

  return TQUEUE_SUCCESS;
}

void tqueue_publish_transaction(tqueue *q, transaction **tp) {
  assert(q);
  *tp = NULL;
  cqueue_spsc_push_slot_finish(q->transactions);
}

int tqueue_remove(tqueue *q, transaction **tp, void **p) {
  assert(q);

  transaction *t = *tp;

  if (!t) {
    t = cqueue_spsc_trypop_slot(q->transactions);
    if (!t)  // transaction queue empty
      return TQUEUE_EMPTY;
  }

  *p = t->actions[t->read_idx];
  t->read_idx++;

  if (t->read_idx == t->write_idx) { // we've read everything
    t->write_idx = 0;
    t->read_idx = 0;
    t = NULL;
    cqueue_spsc_pop_slot_finish(q->transactions);
    return TQUEUE_TRANSACTION_EMPTY;
  }

  return TQUEUE_SUCCESS;
}
