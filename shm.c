#include "common.h"
#include "shm.h"

static struct shared_state *shm;

void lock_shm(void) {
   pthread_mutex_lock(&shm->pin_lock);
}

void unlock_shm(void) {
   pthread_mutex_unlock(&shm->pin_lock);
}

int get_next_core(int * cores, int nr_entries_in_cores) {
   assert(shm);

   int core = shm->next_core;
   shm->next_core = (shm->next_core + 1) % (nr_entries_in_cores);
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

   if(create) {
      shm->next_core = 0;
      shm->refcount = 0;
      pthread_mutex_init(&shm->pin_lock, NULL);
   } else {
      pthread_mutex_lock(&shm->pin_lock);
      shm->refcount++;
      pthread_mutex_unlock(&shm->pin_lock);
   }

   fclose(shmf);

end:
   return shm;
}

void cleanup_shm(char *id) {
   int free_shm = 0;

   pthread_mutex_lock(&shm->pin_lock);
   shm->refcount--;
   if(shm->refcount <= 0) {
      free_shm = 1;
   }
   pthread_mutex_unlock(&shm->pin_lock);

   shmdt(shm);

   if(free_shm) {
      int       shm_id;
      key_t     mem_key;

      mem_key = ftok(id, 'a');
      shm_id = shmget(mem_key, sizeof(*shm), 0666);
      shmctl(shm_id, IPC_RMID, NULL);
      unlink(id);
   }
}
