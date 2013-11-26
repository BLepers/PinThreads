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
#include "parse_args.h"

static int* cores;
static int nr_entries_in_cores = 0;

static int next_core = 0;

static void m_init(void);
static int (*old_pthread_create) (pthread_t *thread, const pthread_attr_t *attr, void *(*start_routine) (void *), void *arg);
static int (*old_pthread_setaffinity_np) (pthread_t, size_t, const cpu_set_t *);
static int (*old_sched_setaffinity) (pid_t, size_t, const cpu_set_t*);

static pthread_mutex_t pin_lock = PTHREAD_MUTEX_INITIALIZER;

int pthread_create(pthread_t *thread, const pthread_attr_t *attr, void *(*start_routine) (void *), void *arg) {
   int core;
   int ret;
   cpu_set_t mask;

   pthread_mutex_lock(&pin_lock);
   if(!old_pthread_create)
      m_init();

   ret = old_pthread_create(thread, attr, start_routine, arg);

   core = cores[next_core]; 
   next_core = (next_core + 1) % (nr_entries_in_cores);

   CPU_ZERO(&mask);
   CPU_SET(core, &mask);
   old_pthread_setaffinity_np(*thread, sizeof(mask), &mask);

   printf("-> Set affinity to %d\n", core);

   pthread_mutex_unlock(&pin_lock);

   return ret;
}

int pthread_setaffinity_np(pthread_t thread, size_t cpusetsize, const cpu_set_t *cpuset) {
   printf("-> Ignoring call to pthread_setaffinity_np performed by the application\n");
   return 0;
}

int sched_setaffinity(pid_t pid, size_t cpusetsize, const cpu_set_t *mask) {
   printf("-> Ignoring call to sched_setaffinity performed by the application\n");
   return 0;
}

void __attribute__((constructor)) m_init(void) {
   char * args;

   pthread_mutex_lock(&pin_lock);
   if(old_pthread_create) {
      printf("Already initialized !\n");
      pthread_mutex_unlock(&pin_lock);
      return;
   }

   old_sched_setaffinity = (int (*) (pid_t, size_t, const cpu_set_t*)) dlsym(RTLD_NEXT, "sched_setaffinity");
   old_pthread_setaffinity_np = (int (*) (pthread_t, size_t, const cpu_set_t *)) dlsym(RTLD_NEXT, "pthread_setaffinity_np");
   old_pthread_create = (int (*)(pthread_t*, const pthread_attr_t*, void* (*)(void*), void*)) dlsym(RTLD_NEXT, "pthread_create");

   args = getenv("PINTHREADS_CORES");
   parse_cores(args, &cores, &nr_entries_in_cores);

   int core = cores[next_core]; 
   next_core = (next_core + 1) % (nr_entries_in_cores);

   cpu_set_t mask;
   CPU_ZERO(&mask);
   CPU_SET(core, &mask);
   old_sched_setaffinity(syscall(__NR_gettid), sizeof(mask), &mask);
   printf("-> Set affinity to %d\n", core);

   pthread_mutex_unlock(&pin_lock);
}
