#include "common.h"

int get_socket(int pid) {
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
    return s;
}

int change_cores(int pid, int nr_cores, int *cores) {
    int s = get_socket(pid);
    if(s < 0)
       return s;

    int action = CHANGE_CORES;
    if (send(s, &action, sizeof(action), 0) == -1) {
       perror("send");
       return -2;
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

int get_cores_list(int pid, int *nr_cores, int **cores) {
    int s = get_socket(pid);
    if(s < 0)
       return s;

    int action = GET_CORES;
    if (send(s, &action, sizeof(action), 0) == -1) {
       perror("send");
       return -2;
    }

    if (recv(s, nr_cores, sizeof(*nr_cores), 0) == -1) {
       perror("recv");
       return -2;
    }

    int i;
    *cores = malloc(*nr_cores * sizeof(**cores));

    for(i = 0; i < *nr_cores; i++) {
       if (recv(s, &((*cores)[i]), sizeof((*cores)[i]), 0) == -1) {
          perror("recv");
          return -3;
       }
    }
    close(s);
    return 0;
}

int switch_node(int pid, int old_node, int new_node) {
    int s = get_socket(pid);
    if(s < 0)
       return s;

    int action = CHANGE_NODE;
    if (send(s, &action, sizeof(action), 0) == -1) {
       perror("send");
       return -2;
    }

    if (send(s, &old_node, sizeof(old_node), 0) == -1) {
       perror("send");
       return -2;
    }
    if (send(s, &new_node, sizeof(new_node), 0) == -1) {
       perror("send");
       return -2;
    }

    close(s);
    return 0;
}
