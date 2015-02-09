#ifndef PINSERVER
#define PINSERVER 1

#define GET_CORES 0
#define CHANGE_CORES 1
#define CHANGE_NODE 2
#define GET_ACTIVE 3
#define SET_ACTIVE 4

void init_server(void);
void wait_for_server(void);
void *server(void *data);

#endif
