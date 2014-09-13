#ifndef _DISPATCHER_
#define _DISPATCHER_

#include <stdlib.h>		// size_t
#include "worker.h"

struct dispatcher_data {
    size_t received;
    size_t dropped;
    int current_receive_queue;
};

int dispatcher_next_receive_queue(struct worker_data *d, int cur);
void *dispatcher(void *threadarg);


#endif
