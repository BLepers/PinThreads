#include "common.h"
#include "shm.h"

static void m_init(void);
static int (*old_pthread_create) (pthread_t *thread, const pthread_attr_t *attr, void *(*start_routine) (void *), void *arg);
static int (*old_pthread_setaffinity_np) (pthread_t, size_t, const cpu_set_t *);
static int (*old_sched_setaffinity) (pid_t, size_t, const cpu_set_t*);


int pthread_create(pthread_t *thread, const pthread_attr_t *attr, void *(*start_routine) (void *), void *arg) {
   int core;
   int ret;
   cpu_set_t mask;

   ret = old_pthread_create(thread, attr, start_routine, arg);

   core = get_next_core();
   CPU_ZERO(&mask);
   CPU_SET(core, &mask);
   old_pthread_setaffinity_np(*thread, sizeof(mask), &mask);

   fprintf(stderr, "-> Set affinity to %d\n", core);

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
   int core;
   cpu_set_t mask;


   if(old_pthread_create) 
      return;

   old_sched_setaffinity = (int (*) (pid_t, size_t, const cpu_set_t*)) dlsym(RTLD_NEXT, "sched_setaffinity");
   old_pthread_setaffinity_np = (int (*) (pthread_t, size_t, const cpu_set_t *)) dlsym(RTLD_NEXT, "pthread_setaffinity_np");
   old_pthread_create = (int (*)(pthread_t*, const pthread_attr_t*, void* (*)(void*), void*)) dlsym(RTLD_NEXT, "pthread_create");

   init_shm(getenv("PINTHREAD_SHMID"), 0);

   core = get_next_core(); 
   CPU_ZERO(&mask);
   CPU_SET(core, &mask);
   old_sched_setaffinity(syscall(__NR_gettid), sizeof(mask), &mask);
   fprintf(stderr, "-> Set affinity to %d\n", core);
}
