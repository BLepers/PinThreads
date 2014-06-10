#include "../common.h"

void *fake(void *data) {
   return NULL;
}
int main() {
   int i;
   pthread_t t;
   for(i = 0; i < 5; i++) {
      pthread_create(&t, NULL, fake, NULL);
      sleep(10);
   }
   return 0;
}
