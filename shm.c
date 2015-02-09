#include "common.h"
#include "shm.h"

static struct shared_state *shm;

struct shared_state *get_shm(void) {
   return shm;
}

void lock_shm(void) {
   pthread_mutex_lock(&shm->pin_lock);
}

void unlock_shm(void) {
   pthread_mutex_unlock(&shm->pin_lock);
}

int get_next_core() {
   assert(shm);

   pthread_mutex_lock(&shm->pin_lock);
   int ret, core = shm->next_core;
   shm->next_core = (shm->next_core + 1) % (shm->nr_entries_in_cores);
   ret = shm->cores[core];
   pthread_mutex_unlock(&shm->pin_lock);

   return ret;
}

struct shared_state *_create_shm(char *id, int create, struct shared_state *content, int *cores) {
   int       shm_id;
   key_t     mem_key;

   if(shm)
      goto end;

   FILE *shmf = fopen(id, "ab+");
   assert(shmf);

   mem_key = ftok(id, 'a');
   assert(mem_key != -1);

   shm_id = shmget(mem_key, sizeof(*shm) + sizeof(*shm->cores)*content->nr_entries_in_cores, (create?IPC_CREAT:0) | 0666);
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
      int i;
      memcpy(shm, content, sizeof(*content));
      shm->next_core = 0;
      shm->refcount = 0;
      shm->server_fd = -1;
      shm->active = 1;
      for(i = 0; i < content->nr_entries_in_cores; i++)
         shm->cores[i] = cores[i];
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

char *get_shm_size(void) {
   char *size;
   assert(asprintf(&size, "%d", (int)(sizeof(*shm) + sizeof(*shm->cores)*shm->nr_entries_in_cores)));
   return size;
}

struct shared_state *create_shm(char *id, struct shared_state *content, int *cores) {
   return _create_shm(id, 1, content, cores);
}

struct shared_state *restore_shm(char *id, char *size) {
   struct shared_state fake;
   sscanf(size, "%d", &fake.nr_entries_in_cores);
   fake.nr_entries_in_cores = (fake.nr_entries_in_cores - sizeof(*shm))/sizeof(*shm->cores);
   return _create_shm(id, 0, &fake, NULL);
}

void cleanup_shm(char *id) {
   int free_shm = 0;

   pthread_mutex_lock(&shm->pin_lock);
   shm->refcount--;

   assert(shm->refcount >= 0);

   if(shm->refcount == 0) {
      free_shm = 1;

      /* cleanup the socket file with the shm */
      if(shm->server) {
	char *path = NULL;
	char *suffix = getenv("PINTHREADS_SOCK_SUFFIX");
	if (suffix == NULL)
 	  assert(asprintf(&path, "%s_sock", getenv("PINTHREADS_SHMID")));
	else
	  assert(asprintf(&path, "%s_%s_sock", getenv("PINTHREADS_SHMID"), suffix));

        VERBOSE("[SERVER] Delete socket file %s\n", path);
	unlink(path);
	free(path);
      }
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
