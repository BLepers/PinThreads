#include "common.h"

int change_cores(int pid, int nr_cores, int *cores) {
    int s, len;
    struct sockaddr_un remote;

    char *path = NULL;
    assert(asprintf(&path, "/tmp/shm_%d_sock", pid));

    if ((s = socket(AF_UNIX, SOCK_STREAM, 0)) == -1) {
        perror("socket");
        return -4;
    }

    remote.sun_family = AF_UNIX;
    strcpy(remote.sun_path, path);
    len = strlen(remote.sun_path) + sizeof(remote.sun_family);
    if (connect(s, (struct sockaddr *)&remote, len) == -1) {
        perror("connect");
        return -1;
    }

    if (send(s, &nr_cores, sizeof(nr_cores), 0) == -1) {
       perror("send");
       return -2;
    }

    int i;
    for(i = 0; i < nr_cores; i++) {
       if (send(s, &(cores[i]), sizeof(cores[i]), 0) == -1) {
          perror("send");
          return -3;
       }
    }
    close(s);
    return 0;
}
