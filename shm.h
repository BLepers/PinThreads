#ifndef GET_SHM
#define GET_SHM

#include <pthread.h>

struct shared_state {
   int refcount;
   int next_core;
   pthread_mutex_t pin_lock;
};

int get_next_core(int * cores, int nr_entries_in_cores);
void *init_shm(char *id, int create);
void cleanup_shm(char *id);
void lock_shm(void);
void unlock_shm(void);

#endif
