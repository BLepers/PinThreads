#ifndef COMMON_H
#define COMMON_H

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
#include <libgen.h>
#include <sys/types.h>
#include <sys/ipc.h>
#include <sys/shm.h>
#include <numa.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <sys/file.h>
#include <signal.h>

#define VERBOSE(msg, args...) { \
   char * verbose_str = getenv("PINTHREADS_VERBOSE"); \
   char * verbose_str_err = getenv("PINTHREADS_VERBOSE_STDERR"); \
   int verbose = 0, verbose_err = 0; \
   if(verbose_str) { \
      verbose = atoi(verbose_str); \
   } \
   if(verbose_str_err) { \
      verbose_err = atoi(verbose_str_err); \
   } \
   if(verbose) { \
      if(verbose_err) { \
         fprintf(stderr, msg, ##args); \
      } else { \
         printf(msg, ##args); \
      } \
   } \
}

static inline pid_t gettid(void) {
   return syscall(__NR_gettid);
}

#endif
