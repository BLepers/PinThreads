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
#include <signal.h>
#include <errno.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <semaphore.h>
#include "shm.h"
#include "pinserver.h"

#define VERBOSE(msg, args...) { \
   if(get_shm()->verbose) { \
      if(get_shm()->verbose_err) { \
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
