#include "../common.h"
#include <sys/wait.h>

void *fake(void *data) {
   return NULL;
}

int main(int argc, char **argv) {
    pthread_t tid;

    if (fork() == 0) {
        sleep(1);
        pthread_create(&tid, NULL, fake, NULL);
    }

    exit(0);
}

