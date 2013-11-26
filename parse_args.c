#ifndef _GNU_SOURCE
#define _GNU_SOURCE
#endif
#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <pthread.h>
#include <unistd.h>
#include <string.h>
#include <assert.h>
#include <sys/sysinfo.h>
#include "parse_args.h"

void parse_cores(char *arg, int **_cores, int *_nr_entries_in_cores) {
   int cores_len = 1;
   int *cores = (int*)calloc(cores_len, sizeof(*cores));
   int nr_entries_in_cores = 0;
   char *end_str = NULL;
   int ncores = get_nprocs();

   char *result = strtok_r(arg, "," , &end_str);
   while( result != NULL ) {
      char * end_str2;
      int prev = -1;

      char * result2 = strtok_r(result, "-", &end_str2);
      while(result2 != NULL) {
         if(prev < 0) {
            prev = atoi(result2);
            if(prev < 0 || prev >= ncores){
               fprintf(stderr, "%d is not a valid core number. Must be comprised between 0 and %d\n", prev, ncores-1);
               exit(EXIT_FAILURE);
            }

            /* Add to cores array */
            if(++nr_entries_in_cores > cores_len) {
               cores_len *= 2;
               cores = (int*) realloc (cores, cores_len * sizeof(int));
               assert(cores);
            }

            cores[nr_entries_in_cores-1] = prev;
         }
         else {
            int i;
            int core = atoi(result2);

            if(core < 0 || core >= ncores){
               fprintf(stderr, "%d is not a valid core number. Must be comprised between 0 and %d\n", core, ncores-1);
               exit(EXIT_FAILURE);
            }

            if(prev > core) {
               fprintf(stderr, "%d-%d is not a valid core range\n", prev, core);
               exit(EXIT_FAILURE);
            }

            for(i = prev + 1; i <= core; i++) {
               /* Add to cores array */
               if(++nr_entries_in_cores > cores_len) {
                  cores_len *= 2;
                  cores = (int*) realloc (cores, cores_len * sizeof(int));
                  assert(cores);
               }
               cores[nr_entries_in_cores -1 ] = i;
            }
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
