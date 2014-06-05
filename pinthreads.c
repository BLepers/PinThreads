#include "common.h"
#include "parse_args.h"
#include "shm.h"

void usage(char * app_name) {
   fprintf(stderr, "Usage: %s [-c <list of cores> | -n <list of nodes>] <command>\n", app_name);
   fprintf(stderr, "\t-c: list of cores separated by commas or dashes (e.g, -c 0-7,9,15-20)\n");
   fprintf(stderr, "\t\tYou can use Nx to indicate all cores of a node (e.g., -c 0,N1)\n");
   fprintf(stderr, "\t-n: list of nodes separated by commas or dashes (e.g, -n 0,2-3\n");
   fprintf(stderr, "\t-s: when not specifying any cores/nodes, you might want to evenly distribute threads on nodes (as opposed to maximize locality)\n");
   exit(EXIT_FAILURE);
}

#define PATH_LEN 512
char *get_lib_path() {
   char buffer[PATH_LEN], *lib, *path;
   int lib_path_len = readlink("/proc/self/exe", buffer, PATH_LEN);
   buffer[lib_path_len + 1] = '\0';
   path = dirname(buffer);
   assert(asprintf(&lib, "%s/pin.so", path));

   FILE * flib = fopen(lib, "r");
   if(!flib) {
      free(lib);
      assert(asprintf(&lib, PREFIX "/lib/pinthreads/pin.so"));
   } else {
      fclose(flib);
   }
   return lib;
}

void add_core_to_str(char ** str, int * str_size, int * str_written, int core) {
   do {
      int max = *str_size - *str_written - 1;
      int written = snprintf(*str + *str_written, max, "%d,", core);

      if(written > max) {
         *str_size *= 2;
         *str = (char *) realloc(*str, *str_size * sizeof(char));

         assert(*str);
      }
      else {
         *str_written += written;
         break;
      }
   } while(1);
}

char * build_default_affinity_string (int shuffle) {
   int nr_nodes = numa_num_configured_nodes();
   int nr_cores = numa_num_configured_cpus();

   char * str;
   int str_size = 512;
   int str_written = 0;

   int i;

   struct bitmask ** bm = (struct bitmask**) malloc(sizeof(struct bitmask*) * nr_nodes);

   for (i = 0; i < nr_nodes; i++) {
      bm[i] = numa_allocate_cpumask();
      numa_node_to_cpus(i, bm[i]);
   }

   str = (char*) malloc(str_size * sizeof(char));
   assert(str);

   if(!shuffle) {
      for(i = 0; i < nr_nodes; i++) {
         int j;
         for(j = 0; j < nr_cores; j++) {
            if (numa_bitmask_isbitset(bm[i], j)) {
               add_core_to_str(&str, &str_size, &str_written, j);
            }
         }
      }
   }
   else {
      int next_node = 0;

      for(i = 0; i < nr_cores; i++) {
         int idx = (i / nr_nodes) + 1;
         int found = 0;
         int j = 0;

         do {
            if (numa_bitmask_isbitset(bm[next_node], j)) {
               found++;
            }

            if(found == idx){
               add_core_to_str(&str, &str_size, &str_written, j);
               break;
            }

            j = (j + 1) % nr_cores;
         } while (found != idx);

         next_node = (next_node + 1) % nr_nodes;
      }
   }

   if(str_written) {
      str[str_written - 1] = 0;
   }

   return str;
}

int main(int argc, char **argv){
   char c;
   char *verbose = "0";
   char *verbose_err = "0";
   char *cores = NULL;
   char *nodes = NULL;

   int shuffle = 0;
   while ((c = getopt(argc, argv, "+vVsc:n:")) != -1) {
      switch (c) {
         case 'c':
            if(cores) {
               fprintf(stderr, "-c or -n already used !\n");
               exit(EXIT_FAILURE);
            }

            cores = strdup(optarg);
            break;
         case 'n':
            if(nodes || cores) {
               fprintf(stderr, "-c or -n already used !\n");
               exit(EXIT_FAILURE);
            }

            nodes = strdup(optarg);
            break;
         case 's':
            shuffle = 1;
            break;
         case 'v':
            verbose = "1";
            break;
         case 'V':
            verbose = "1";
            verbose_err = "1";
            break;
         default:
            usage(argv[0]);
      }
   }

   if(!cores && !nodes) {
      printf("Not defined any cores/nodes. Using defaults (shuffle = %d)\n",shuffle);
      cores = build_default_affinity_string(shuffle);
   }

   if(optind == argc) {
      usage(argv[0]);
   }

   argv +=  optind;

   char *lib = get_lib_path();
   setenv("LD_PRELOAD", lib, 1);
   free(lib);

   char *shm_name = tempnam("/tmp/", "shm_"), *uniq_shm_name = NULL;
   assert(asprintf(&uniq_shm_name, "%s_%d", shm_name, gettid()));
   struct shared_state *s = init_shm(uniq_shm_name, 1);
   s->next_core = 0;
   setenv("PINTHREADS_SHMID", uniq_shm_name, 1);
   free(uniq_shm_name);
   free(shm_name);

   pthread_mutex_init(&s->pin_lock, NULL);

   unsetenv("PINTHREADS_CORES");
   unsetenv("PINTHREADS_NODES");

   if(cores) {
      setenv("PINTHREADS_CORES", cores, 1);
      parse_cores(cores, NULL, NULL, 0);
   } else {
      setenv("PINTHREADS_NODES", nodes, 1);
      parse_cores(nodes, NULL, NULL, 1);
   }

   setenv("PINTHREADS_VERBOSE", verbose, 1);
   setenv("PINTHREADS_VERBOSE_STDERR", verbose_err, 1);

   execvp(argv[0], argv);
   perror("execvp");
   fprintf(stderr,"failed to execute %s\n", argv[0]);

   return EXIT_SUCCESS;
}
