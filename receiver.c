#include "receiver.h"

void *receiver(void *threadargs) {
  struct receiver_data *receiver_data = (struct receiver_data *)threadargs;
  printf("receiver(): thread_id %d\n", receiver_data->thread_id);
  pthread_exit(NULL);
}

