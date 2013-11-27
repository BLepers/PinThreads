#ifndef GET_SHM
#define GET_SHM

#include <pthread.h>

struct shared_state {
   int next_core;
   pthread_mutex_t pin_lock;
   int *cores;
   int nr_entries_in_cores;
};

int get_next_core(void);
void *init_shm(char *id, int create);

#endif
