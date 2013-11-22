#ifndef _GNU_SOURCE
#define _GNU_SOURCE
#endif
#define __USE_GNU
#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <pthread.h>
#include <unistd.h>
#include <dlfcn.h>
#include <sys/sysinfo.h>
#include <sys/syscall.h>
#include <string.h>
#include <assert.h>

static int* cores;
static int cores_len;
static int nr_entries_in_cores = 0;

static int next_core = 0;

static void m_init(void);
static int (*old_pthread_create) (pthread_t *thread, const pthread_attr_t *attr, void *(*start_routine) (void *), void *arg);

extern "C" int pthread_create(pthread_t *thread, const pthread_attr_t *attr, void *(*start_routine) (void *), void *arg) {
   if(!old_pthread_create)
      m_init();

   int core;  
   int ret = old_pthread_create(thread, attr, start_routine, arg);

   cpu_set_t mask;
   core = cores[next_core]; 
   next_core = (next_core + 1) % (nr_entries_in_cores);

   CPU_ZERO(&mask);
   CPU_SET(core, &mask);
   pthread_setaffinity_np(*thread, sizeof(mask), &mask);

   printf("-> Set affinity to %d\n", core);

   return ret;
}

void __attribute__((constructor)) m_init(void) {
   char * result = NULL;
   char * end_str;
   int ncores = get_nprocs();
   char * args;

   old_pthread_create = (int (*)(pthread_t*, const pthread_attr_t*, void* (*)(void*), void*)) dlsym(RTLD_NEXT, "pthread_create");

   cores_len = ncores;
   cores = (int*) calloc(cores_len, sizeof(int));

   args = getenv("PINTHREADS_CORES");
   result = strtok_r(args, "," , &end_str);
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

   cpu_set_t mask;

   int core = cores[next_core]; 
   next_core = (next_core + 1) % (nr_entries_in_cores);

   CPU_ZERO(&mask);
   CPU_SET(core, &mask);
   sched_setaffinity(syscall(__NR_gettid), sizeof(mask), &mask);
   printf("-> Set affinity to %d\n", core);
}
