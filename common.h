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

#define VERBOSE(msg, args...) { \
   char * verbose_str = getenv("PINTHREADS_VERBOSE"); \
   int verbose = 0; \
   if(verbose_str) { \
      verbose = atoi(verbose_str); \
   } \
   \
   if(verbose) { \
      printf(msg, ##args); \
   } \
}
#endif
