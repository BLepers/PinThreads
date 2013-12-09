#include "common.h"
#include "parse_args.h"

static void check_range(int n0, int n1, int max) {
   if(n0 + 1 < 0 || n0 >= max){
      fprintf(stderr, "%d is not a valid number. Must be comprised between 0 and %d\n", n0 + 1, max-1);
      exit(EXIT_FAILURE);
   }

   if(n0 > n1) {
      fprintf(stderr, "%d-%d is not a valid range\n", n0, n1);
      exit(EXIT_FAILURE);
   }

   if(n1 < 0 || n1 >= max){
      fprintf(stderr, "%d is not a valid number. Must be comprised between 0 and %d\n", n1, max-1);
      exit(EXIT_FAILURE);
   }
}

static void add_core_range(int core0, int core1, int **cores, int *nr_entries_in_cores, int *cores_len) {
   int i;

   check_range(core0, core1, get_nprocs());

   for(i = core0 + 1; i <= core1; i++) {
      /* Add to cores array */
      *nr_entries_in_cores = *nr_entries_in_cores + 1;
      if(*nr_entries_in_cores >= *cores_len) {
         *cores_len *= 2;
         *cores = realloc (*cores, *cores_len * sizeof(**cores));
         assert(*cores);
      }
      (*cores)[*nr_entries_in_cores - 1] = i;
   }
}

static void add_node_range(int node0, int node1, int **cores, int *nr_entries_in_cores, int *cores_len) {
   int i;
   int nprocs = get_nprocs();

   check_range(node0, node1, numa_max_node() + 1);

   for(i = node0 + 1; i <= node1; i++) {
      struct bitmask * bm = numa_allocate_cpumask();
      numa_node_to_cpus(i, bm);

      int j = 0;
      for (j = 0; j < nprocs; j++) {
         if (numa_bitmask_isbitset(bm, j)) {
            add_core_range(j - 1, j, cores, nr_entries_in_cores, cores_len);
         }
      }
      numa_free_cpumask(bm);
   }
}

void parse_cores(char *arg, int **_cores, int *_nr_entries_in_cores, int _is_node) {
   int cores_len = 1;
   int *cores = (int*)calloc(cores_len, sizeof(*cores));
   int nr_entries_in_cores = 0;
   char *end_str = NULL;

   char *result = strtok_r(arg, "," , &end_str);
   while( result != NULL ) {
      char * end_str2;
      int prev = -1;
      int is_node = _is_node;

      char * result2 = strtok_r(result, "-", &end_str2);
      while(result2 != NULL) {
         if(prev < 0) {
            if(result2[0] == 'N') {
               is_node = 1;
               prev = atoi(result2 + 1);
               add_node_range(prev - 1, prev, &cores, &nr_entries_in_cores, &cores_len);
            } else if(_is_node) {
               prev = atoi(result2);
               add_node_range(prev - 1, prev, &cores, &nr_entries_in_cores, &cores_len);
            } else {
               prev = atoi(result2);
               add_core_range(prev - 1, prev, &cores, &nr_entries_in_cores, &cores_len);
            }
         }
         else {
            int core = atoi(result2);
            if(is_node)
               add_node_range(prev, core, &cores, &nr_entries_in_cores, &cores_len);
            else
               add_core_range(prev, core, &cores, &nr_entries_in_cores, &cores_len);
            prev = core;
         }

         result2 = strtok_r(NULL, "-", &end_str2);
      }

      result = strtok_r(NULL, "," , &end_str);
   }

   if(_cores)
      *_cores = cores;
   if(_nr_entries_in_cores)
      *_nr_entries_in_cores = nr_entries_in_cores;
}
