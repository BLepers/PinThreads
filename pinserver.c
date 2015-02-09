#include "common.h"

static int *core_to_node;
static int **node_to_cores;
static sem_t _sem_init;

void init_server(void) {
   sem_init(&_sem_init, 0, 0);
}

void wait_for_server(void) {
   sem_wait(&_sem_init);
}

static void build_node_to_core() {
   if(core_to_node)
      return;

   int i;
   int nb_cores = get_nprocs();
   int nb_nodes = numa_max_node() + 1;
   core_to_node = calloc(nb_cores, sizeof(*core_to_node));
   node_to_cores = calloc(nb_nodes, sizeof(*node_to_cores));


   for(i = 0; i < nb_nodes; i++) {
      struct bitmask * bm = numa_allocate_cpumask();
      numa_node_to_cpus(i, bm);

      node_to_cores[i] = malloc(nb_cores * sizeof(*node_to_cores[i]));

      int j = 0, index = 0;
      for (j = 0; j < nb_cores; j++) {
         if (numa_bitmask_isbitset(bm, j)) {
            core_to_node[j] = i;
            node_to_cores[i][index] = j;
            index++;
         }
      }
      numa_free_cpumask(bm);
   }
}

static int core_id(int core) {
   int i = 0;
   int node = core_to_node[core];
   while(1) {
      if(node_to_cores[node][i] == core)
         return i;
      i++;
   }
}

void *server(void *data) {
   int s, s2, len;
   struct sockaddr_un local, remote;
   socklen_t sock_size = sizeof(remote);
   char *path = NULL;

   lock_shm();
   if(get_shm()->server_fd == -1) {

      /* allow user to set suffix to facilitate socket retrieval */
      char *suffix = getenv("PINTHREADS_SOCK_SUFFIX");
      if (suffix == NULL)
 	 assert(asprintf(&path, "%s_sock", getenv("PINTHREADS_SHMID")));
      else
 	 assert(asprintf(&path, "%s_%s_sock", getenv("PINTHREADS_SHMID"), suffix));

      if ((s = socket(AF_UNIX, SOCK_STREAM, 0)) == -1) {
         perror("socket");
         exit(1);
      }

      local.sun_family = AF_UNIX;
      strcpy(local.sun_path, path);
      len = strlen(local.sun_path) + sizeof(local.sun_family);
      if (bind(s, (struct sockaddr *)&local, len) == -1) {
         perror("bind");
         exit(1);
      }

      get_shm()->server_fd = s;
   } else {
      s = get_shm()->server_fd;
   }
   unlock_shm();

   sem_post(&_sem_init);

   if (listen(s, 5) == -1) {
      perror("listen");
      exit(1);
   }

   for(;;) {
      if ((s2 = accept(s, &remote, &sock_size)) == -1) {
         perror("accept");
         exit(1);
      }

      int n;
      int i, action;

      n = recv(s2, &action, sizeof(int), 0);
      assert(n == sizeof(int));

      if(action == CHANGE_CORES) {
         int nb_cores;

         VERBOSE("[SERVER] Changing cores\n");

         n = recv(s2, &nb_cores, sizeof(int), 0);
         assert(n == sizeof(int));
         assert(nb_cores == get_shm()->nr_entries_in_cores);

         for(i = 0; i < nb_cores; i++) {
            int core;
            n = recv(s2, &core, sizeof(int), 0);
            assert(n == sizeof(int));
            get_shm()->cores[i] = core;
         }
      } else if(action == GET_CORES) {
         int nr_entries_in_cores = get_shm()->nr_entries_in_cores;

         n = send(s2, &nr_entries_in_cores, sizeof(int), 0);
         assert(n == sizeof(int));

         for(i = 0; i < nr_entries_in_cores; i++) {
            n = send(s2, &(get_shm()->cores[i]), sizeof(int), 0);
            assert(n == sizeof(int));
         }
      } else if(action == CHANGE_NODE) {
         int new_node = 0, old_node = 0;
         n = recv(s2, &old_node, sizeof(int), 0);
         assert(n == sizeof(int));
         n = recv(s2, &new_node, sizeof(int), 0);
         assert(n == sizeof(int));

         int nr_entries_in_cores = get_shm()->nr_entries_in_cores;
         build_node_to_core();
         for(i = 0; i < nr_entries_in_cores; i++) {
            if(core_to_node[get_shm()->cores[i]] == old_node)
               get_shm()->cores[i] = node_to_cores[new_node][core_id(get_shm()->cores[i])];
         }
      } else if (action == GET_ACTIVE) {
	 int active = get_shm()->active;
  	 VERBOSE("[SERVER] Client asks for active, response is %d\n", active);
	 send(s2, &active, sizeof(active), 0);
      }
      else if (action == SET_ACTIVE) {
	 int old_active = get_shm()->active;
	 int new_active = 0;

	 n = recv(s2, &new_active, sizeof(int), 0);
	 assert(n == sizeof(int));

  	 VERBOSE("[SERVER] Client set active at %d (previous = %d)\n", new_active, old_active);
	 get_shm()->active = new_active;
      }

      close(s2);
   }

   return NULL;
}
