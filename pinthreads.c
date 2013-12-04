#include "common.h"
#include "parse_args.h"
#include "shm.h"

void usage(char * app_name) {
   fprintf(stderr, "Usage: %s -c <list of cores> <command>\n", app_name);
   fprintf(stderr, "\t-c: list of cores separated by commas or dashes (e.g, -c 0-7,9,15-20)\n");
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

int main(int argc, char **argv){
   char c;
   char *verbose = "0";
   char *cores = NULL;

   while ((c = getopt(argc, argv, "+vc:")) != -1) {
      switch (c) {
         case 'c':
            if(cores) {
               fprintf(stderr, "-c already used !\n");
               exit(EXIT_FAILURE);
            }
            
            cores = strdup(optarg);
            break;
         case 'v':
            verbose = "1";
            break;
         default:
            usage(argv[0]);
      }
   }

   if(!cores) {
      usage(argv[0]);
   }

   if(optind == argc) {
      usage(argv[0]);
   }

   argv +=  optind;  
   
   char *lib = get_lib_path();
   setenv("LD_PRELOAD", lib, 1);
   free(lib);

   char *shm_name = tempnam(".", "shm_");
   struct shared_state *s = init_shm(shm_name, 1);
   s->next_core = 0;
   s->cores = NULL; // each fork will create its own copy of s->cores
   setenv("PINTHREADS_SHMID", shm_name, 1);
   free(shm_name);

   pthread_mutex_init(&s->pin_lock, NULL);
   setenv("PINTHREADS_CORES", cores, 1);
   parse_cores(cores, NULL, &s->nr_entries_in_cores);

   setenv("PINTHREADS_VERBOSE", verbose, 1);

   execvp(argv[0], argv);
   perror("execvp");
   fprintf(stderr,"failed to execute %s\n", argv[0]);

   return EXIT_SUCCESS;
}
