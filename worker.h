#ifndef _WORKER_
#define _WORKER_

#include <pthread.h>
#include <stdint.h>     // INT32_MAX
#include <stdlib.h>		// malloc
#include <stdio.h>      // printf


struct receive_queue {
    int num_entries;
    int max_entries;
    int *data;
};


struct worker_data {
    int thread_id;
    int num_receive_queues;
    int current_receive_queue;
    struct receive_queue *receive_queues;
    pthread_mutex_t receive_queue_ready_lock;
    pthread_cond_t receive_queue_ready;
};

int next_receive_queue(struct worker_data *d);
int receive_queue_insert(struct receive_queue *q, int data);
int initialize_receive_queue(struct receive_queue *queue,
                                int max_receive_entries);
int initialize_worker_data(struct worker_data *data,
                            int num_receive_queues,
                            int max_receive_entries);
int worker_data_next_receive_queue(struct worker_data *d);
void *worker(void *threadarg);

#endif
