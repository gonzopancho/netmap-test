#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <signal.h>
#include <inttypes.h>   // PRIu64
#include "tqueue.h"

#define NUM_THREADS 2

typedef enum {
  PRODUCER,
  CONSUMER
} thread_type;


struct thread_args {
  char pad1[LEVEL1_DCACHE_LINESIZE/2];
  uint64_t limit;
  tqueue *q;
  int id;
  thread_type t;
  uint64_t passes;
  char pad2[LEVEL1_DCACHE_LINESIZE/2];
};

static struct thread_args targs[NUM_THREADS];
static pthread_t threads[NUM_THREADS];

void *producer(void *targ);
void *consumer(void *targ);

int main(int argc, char** argv) {
  int passes, i;
  tqueue *q;

  if (argc != 2) {
    printf("Error: %s requires an int parameter that specifies the number of passes\n", argv[0]);
    exit(EXIT_FAILURE);
  }

  passes = atoi(argv[1]);
  if (passes < 1) {
    printf("Error: %s requires an int parameter that specifies the number of passes\n", argv[0]);
    exit(EXIT_FAILURE);
  }

  q = tqueue_new(32, 32);
  if (!q) {
      printf("Error: tqueue_new failed\n");
      exit(EXIT_FAILURE);
  }

  for (i=0; i < NUM_THREADS; i++) {
    targs[i].id = i;
    targs[i].limit = passes;
    targs[i].passes = 0;
    targs[i].t = PRODUCER;
    targs[i].q = q;
    if (i == NUM_THREADS-1) {
      targs[i].t = CONSUMER;
      pthread_create(&threads[i], NULL, &consumer, &targs[i]);
    } else {
      pthread_create(&threads[i], NULL, &producer, &targs[i]);
    }
  }

  for (i=0; i < NUM_THREADS; i++)
    pthread_join(threads[i], NULL);

  exit(EXIT_SUCCESS);
}

void *producer(void *targ) {
  struct thread_args *args = targ;
  uint64_t data[11] = {1,2,3,4,5,6,7,8,9,0,0};
  int rv;
  transaction *t = NULL;

  data[10] = args->limit;
  for (uint64_t i=1; i < args->limit; i++) {
    while ((rv = tqueue_insert(args->q, &t, &data[i%10])) < 1);
    args->passes++;
  }

  while ((rv = tqueue_insert(args->q, &t, &data[10])) < 1);
  tqueue_publish_transaction(args->q);
  args->passes++;

  while(1);
  pthread_exit(NULL);
}

void *consumer(void *targ) {
  struct thread_args *args = targ;
  uint64_t *data; 
  transaction *t = NULL;
  int rv;

  while(1) {
    rv = tqueue_remove(args->q, &t, (void**)&data);
    if (rv < 1)
      continue;

    args->passes++;
    if (*data == args->limit) {
      printf("thread[%d]: got limit\n", args->id);
      break;
    }
  }

  for (int i=0; i < NUM_THREADS; i++)
    printf("thread[%d]: %" PRIu64 " passes\n", targs[i].id, targs[i].passes);

  pthread_kill(pthread_self(), SIGTERM);

  pthread_exit(NULL);
}
