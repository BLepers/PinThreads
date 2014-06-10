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

static int* cores;
static int nr_entries_in_cores;

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

   lock_shm();
   core = get_next_core(cores, nr_entries_in_cores);
   unlock_shm();

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
      lock_shm();
      set_affinity(ret, get_next_core(cores, nr_entries_in_cores));
      unlock_shm();
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
      lock_shm();
      set_affinity(gettid(), get_next_core(cores, nr_entries_in_cores));
      unlock_shm();
   }

   return ret;
}

void *server(void *data) {
   int s, s2, len;
   struct sockaddr_un local, remote;
   socklen_t sock_size = sizeof(remote);
   char *path = NULL;

   assert(asprintf(&path, "%s_sock", getenv("PINTHREADS_SHMID")));

   if ((s = socket(AF_UNIX, SOCK_STREAM, 0)) == -1) {
      perror("socket");
      exit(1);
   }

   local.sun_family = AF_UNIX;
   strcpy(local.sun_path, path);
   len = strlen(local.sun_path) + sizeof(local.sun_family);
   if (bind(s, (struct sockaddr *)&local, len) == -1) {
      perror("bind");
      exit(1);
   }

   if (listen(s, 5) == -1) {
      perror("listen");
      exit(1);
   }

   for(;;) {
      if ((s2 = accept(s, &remote, &sock_size)) == -1) {
         perror("accept");
         exit(1);
      }

      int n;
      int nb_cores, *_cores, i, action;

      n = recv(s2, &action, sizeof(int), 0);
      assert(n == sizeof(int));

      if(action == CHANGE_CORES) {
         n = recv(s2, &nb_cores, sizeof(int), 0);
         assert(n == sizeof(int));

         _cores = malloc(nb_cores * sizeof(*cores));
         for(i = 0; i < nb_cores; i++) {
            n = recv(s2, &(_cores[i]), sizeof(int), 0);
            assert(n == sizeof(int));
         }

         VERBOSE("[SERVER] Changing cores\n");
         lock_shm();
         free(cores);
         cores = _cores;
         nr_entries_in_cores = nb_cores;
         unlock_shm();
      } else if(action == GET_CORES) {
         n = send(s2, &nr_entries_in_cores, sizeof(int), 0);
         assert(n == sizeof(int));

         for(i = 0; i < nr_entries_in_cores; i++) {
            n = send(s2, &(cores[i]), sizeof(int), 0);
            assert(n == sizeof(int));
         }
      }

      close(s2);
   }

   return NULL;
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

   VERBOSE("Init called for pid %d\n", gettid());


   old_sched_setaffinity = (int (*) (pid_t, size_t, const cpu_set_t*)) dlsym(RTLD_NEXT, "sched_setaffinity");
   old_pthread_setaffinity_np = (int (*) (pthread_t, size_t, const cpu_set_t *)) dlsym(RTLD_NEXT, "pthread_setaffinity_np");
   old_pthread_create = (int (*)(pthread_t*, const pthread_attr_t*, void* (*)(void*), void*)) dlsym(RTLD_NEXT, "pthread_create");
   old_fork = (pid_t (*)(void)) dlsym(RTLD_NEXT, "fork");
   old_clone = (int (*)(int (*)(void *), void *, int flags, void *arg, ...)) dlsym(RTLD_NEXT, "clone");

   init_shm(getenv("PINTHREADS_SHMID"), 0);
   atexit(m_exit);
   signal(SIGTERM, m_signal);
   signal(SIGINT, m_signal);
   signal(SIGSEGV, m_signal);

   if(getenv("PINTHREADS_CORES"))
      parse_cores(strdup(getenv("PINTHREADS_CORES")), &cores, &nr_entries_in_cores, 0);
   else
      parse_cores(strdup(getenv("PINTHREADS_NODES")), &cores, &nr_entries_in_cores, 1);

   if(getenv("PINTHREADS_SERVER")) {
      pthread_t server_thread;
      old_pthread_create(&server_thread, NULL, server, NULL);
   }

   lock_shm();
   set_affinity(gettid(), get_next_core(cores, nr_entries_in_cores));
   unlock_shm();
}
