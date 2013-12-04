#include "common.h"
#include "parse_args.h"
#include "shm.h"

static void m_init(void);
static int (*old_pthread_create) (pthread_t *thread, const pthread_attr_t *attr, void *(*start_routine) (void *), void *arg);
static int (*old_pthread_setaffinity_np) (pthread_t, size_t, const cpu_set_t *);
static int (*old_sched_setaffinity) (pid_t, size_t, const cpu_set_t*);
static pid_t (*old_fork)(void);

static pid_t gettid(void) {
   return syscall(__NR_gettid);
}

static void set_affinity(pid_t tid, int cpu_id) {
   cpu_set_t mask;
   CPU_ZERO(&mask);
   CPU_SET(cpu_id, &mask);
   printf("--> Setting tid %d on core %d\n", tid, cpu_id);
   int r = old_sched_setaffinity(tid, sizeof(mask), &mask);
   if (r < 0) {
      fprintf(stderr, "couldn't set affinity for %d\n", cpu_id);
      exit(1);
   }
}

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

pid_t fork(void) {
   pid_t ret = old_fork();
   if(ret == 0) {
      set_affinity(gettid(), get_next_core());
   }

   return ret;
}

void __attribute__((constructor)) m_init(void) {
   if(old_pthread_create) 
      return;

   printf("Init called for pid %d\n", gettid());

   old_sched_setaffinity = (int (*) (pid_t, size_t, const cpu_set_t*)) dlsym(RTLD_NEXT, "sched_setaffinity");
   old_pthread_setaffinity_np = (int (*) (pthread_t, size_t, const cpu_set_t *)) dlsym(RTLD_NEXT, "pthread_setaffinity_np");
   old_pthread_create = (int (*)(pthread_t*, const pthread_attr_t*, void* (*)(void*), void*)) dlsym(RTLD_NEXT, "pthread_create");
   old_fork = (pid_t (*)(void)) dlsym(RTLD_NEXT, "fork");

   struct shared_state *s = init_shm(getenv("PINTHREADS_SHMID"), 0);
   parse_cores(strdup(getenv("PINTHREADS_CORES")), &s->cores, NULL);

   set_affinity(gettid(), get_next_core());
}
