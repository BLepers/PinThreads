#include "common.h"
#include "shm.h"

static struct shared_state *shm;
int get_next_core(int * cores, int nr_entries_in_cores) {
   assert(shm);

   pthread_mutex_lock(&shm->pin_lock);
   int core = shm->next_core;
   shm->next_core = (shm->next_core + 1) % (nr_entries_in_cores);
   pthread_mutex_unlock(&shm->pin_lock);

   return cores[core];
}

void *init_shm(char *id, int create) {
   int       shm_id;
   key_t     mem_key;

   if(shm)
      goto end;

   FILE *shmf = fopen(id, "ab+");
   assert(shmf);

   mem_key = ftok(id, 'a');
   assert(mem_key != -1);

   shm_id = shmget(mem_key, sizeof(*shm), (create?IPC_CREAT:0) | 0666);
   if (shm_id < 0) {
      fprintf(stderr, "*** shmget error ***\n");
      exit(1);
   }

   shm = shmat(shm_id, NULL, 0);
   if ((long) shm == -1) {
      fprintf(stderr, "*** shmat error ***\n");
      exit(1);
   }

   fclose(shmf);

end:
   return shm;
}
