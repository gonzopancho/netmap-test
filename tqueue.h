/*!
  \file
  \copyright Copyright (c) 2014, Richard Fujiyama
  Licensed under the terms of the New BSD license.
*/

#ifndef _TQUEUE_
#define _TQUEUE_

#include <stdlib.h>     // malloc
#include <assert.h>
#include <stdatomic.h>  // atomics
#include "cqueue/cqueue.h"

typedef struct tqueue tqueue;
typedef struct transaction transaction;

enum tqueue_retval { 
  TQUEUE_FULL = -2,
  TQUEUE_EMPTY = -3,
  TQUEUE_SUCCESS = 1,
  TQUEUE_TRANSACTION_FULL = 2,
  TQUEUE_TRANSACTION_EMPTY = 3
};

/*! initialize and allocate a new transaction queue

  \param[in] num_transactions the maximum number of transactions in the queue
  \param[in] num_actions the number of pointers in each transaction
  \return a pointer to the queue, or NULL on error
*/
tqueue *tqueue_new(size_t num_transactions, size_t num_actions);

/*! deallocates the queue

  \param[in,out] p a pointer to the pointer to the queue to be deallocated.
  On success, *p will be set to NULL.
*/
void tqueue_delete(tqueue **p);

/*! insert a pointer into the transaction queue

  This function is threadsafe for at most one concurrent invocation of 
  insert and remove (ie, spsc).

  \param[in] q the queue to insert the pointer into
  \param[in, out] tp a placeholder to memoize the transaction.
  *tp must initially be set to NULL.
  \params[in] p the pointer to insert
  \return a value in enum tqueue_retval
*/
int tqueue_insert(tqueue *q, transaction **tp, void *p);

/*! publish the current insert transaction for reading
*/
void tqueue_publish_transaction(tqueue *q, transaction **tp);

/*! remove a pointer from the transaction queue

  This function is threadsafe for at most one concurrent invocation of 
  insert and remove (ie, spsc).

  \param[in] q the queue to remove the pointer from
  \param[in, out] tp a placeholder to memoize the transaction
  *tp must initially be set to NULL.
  \params[out] p *p will be set to the removed pointer
  \return a value in enum tqueue_retval
*/
int tqueue_remove(tqueue *q, transaction **tp, void **p);

#endif  // _TQUEUE_

