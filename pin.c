#ifndef _GNU_SOURCE
#define _GNU_SOURCE
#endif
#define __USE_GNU
#include <stdio.h>
#include <stdint.h>
#include <pthread.h>
#include <unistd.h>
#include <dlfcn.h>
#include <sys/sysinfo.h>
#include <sys/syscall.h>


static int last_core = 0;
static int nb_cpus;
static void m_init(void);
static int (*old_pthread_create) (pthread_t *thread, const pthread_attr_t *attr, void *(*start_routine) (void *), void *arg);


extern "C" int pthread_create(pthread_t *thread, const pthread_attr_t *attr, void *(*start_routine) (void *), void *arg) {
   if(!old_pthread_create)
      m_init();

   int ret = old_pthread_create(thread, attr, start_routine, arg);

   cpu_set_t mask;
   last_core = (last_core + 1) % (nb_cpus);
   CPU_ZERO(&mask);
   CPU_SET(last_core, &mask);
   pthread_setaffinity_np(*thread, sizeof(mask), &mask);

   printf("-> Set affinity to %d\n", last_core);

   return ret;
}

void
__attribute__((constructor))
m_init(void) {
   old_pthread_create = (int (*)(pthread_t*, const pthread_attr_t*, void* (*)(void*), void*)) dlsym(RTLD_NEXT, "pthread_create");
   nb_cpus = get_nprocs();

   cpu_set_t mask;
   CPU_ZERO(&mask);
   CPU_SET(last_core, &mask);
   sched_setaffinity(syscall(__NR_gettid), sizeof(mask), &mask);
   printf("-> Set affinity to %d\n", last_core);
}
