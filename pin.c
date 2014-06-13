#include "common.h"
#include "parse_args.h"
#include "shm.h"

#include <stdarg.h>

static void m_init(void);
static int (*old_pthread_create) (pthread_t *thread, const pthread_attr_t *attr, void *(*start_routine) (void *), void *arg);
static int (*old_pthread_setaffinity_np) (pthread_t, size_t, const cpu_set_t *);
static int (*old_sched_setaffinity) (pid_t, size_t, const cpu_set_t*);
static pid_t (*old_fork)(void);
static int (*old_clone)(int (*)(void *), void *, int, void *, ...);

static void set_affinity(pid_t tid, int cpu_id) {
   cpu_set_t mask;
   CPU_ZERO(&mask);
   CPU_SET(cpu_id, &mask);
   VERBOSE("--> Setting tid %d on core %d\n", tid, cpu_id);
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

   VERBOSE("-> Set affinity to %d\n", core);

   return ret;
}

int pthread_setaffinity_np(pthread_t thread, size_t cpusetsize, const cpu_set_t *cpuset) {
   VERBOSE("-> Ignoring call to pthread_setaffinity_np performed by the application\n");
   return 0;
}

int sched_setaffinity(pid_t pid, size_t cpusetsize, const cpu_set_t *mask) {
   VERBOSE("-> Ignoring call to sched_setaffinity performed by the application\n");
   return 0;
}

pid_t fork(void) {
   pid_t ret = old_fork();
   if(ret > 0) {
      set_affinity(ret, get_next_core());
   }

   return ret;
}

int clone(int (*fn)(void *), void *child_stack, int flags, void *arg, ... ) {
   va_list arg_list;
   int ret;

   va_start(arg_list, arg);

   if((flags & CLONE_CHILD_CLEARTID) || (flags & CLONE_CHILD_CLEARTID)) {
      pid_t *ptid = va_arg(arg_list, pid_t*);
      struct user_desc *tls = va_arg(arg_list, struct user_desc*);
      pid_t *ctid = va_arg(arg_list, pid_t*);

      ret = old_clone(fn, child_stack, flags, arg, ptid, tls, ctid);
   }
   else if (flags & CLONE_SETTLS) {
      pid_t *ptid = va_arg(arg_list, pid_t*);
      struct user_desc *tls = va_arg(arg_list, struct user_desc*);

      ret = old_clone(fn, child_stack, flags, arg, ptid, tls);
   }
   else if(flags & CLONE_PARENT_SETTID) {
      pid_t *ptid = va_arg(arg_list, pid_t*);

      ret = old_clone(fn, child_stack, flags, arg, ptid);
   }
   else {
      ret = old_clone(fn, child_stack, flags, arg);
   }

   va_end(arg_list);

   if(ret > 0) {
      set_affinity(gettid(), get_next_core());
   }

   return ret;
}

void m_exit(void) {
   cleanup_shm(getenv("PINTHREADS_SHMID"));
}

void m_signal(int signal) {
   exit(signal);
}

void __attribute__((constructor)) m_init(void) {
   if(old_pthread_create)
      return;

   restore_shm(getenv("PINTHREADS_SHMID"), getenv("PINTHREADS_SHMSIZE"));

   VERBOSE("Init called for pid %d\n", gettid());

   old_sched_setaffinity = (int (*) (pid_t, size_t, const cpu_set_t*)) dlsym(RTLD_NEXT, "sched_setaffinity");
   old_pthread_setaffinity_np = (int (*) (pthread_t, size_t, const cpu_set_t *)) dlsym(RTLD_NEXT, "pthread_setaffinity_np");
   old_pthread_create = (int (*)(pthread_t*, const pthread_attr_t*, void* (*)(void*), void*)) dlsym(RTLD_NEXT, "pthread_create");
   old_fork = (pid_t (*)(void)) dlsym(RTLD_NEXT, "fork");
   old_clone = (int (*)(int (*)(void *), void *, int flags, void *arg, ...)) dlsym(RTLD_NEXT, "clone");

   atexit(m_exit);
   signal(SIGTERM, m_signal);
   signal(SIGINT, m_signal);
   signal(SIGSEGV, m_signal);

   if(get_shm()->server) {
      pthread_t server_thread;
      init_server();
      old_pthread_create(&server_thread, NULL, server, NULL);
      wait_for_server();
   }

   set_affinity(gettid(), get_next_core());
}
