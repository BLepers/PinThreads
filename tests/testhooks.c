#include "../common.h"

int change_cores(int pid, int nr_cores, int *cores);
int main(int argc, char **argv) {
   int cores[] = { 3, 5, 7 };
   change_cores(atoi(argv[1]), sizeof(cores)/sizeof(*cores), cores);
   return 0;
}
