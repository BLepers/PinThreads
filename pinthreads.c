#include <stdio.h>
#include <sys/sysinfo.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>

void usage(char * app_name) {
   fprintf(stderr, "Usage: %s -c <list of cores> <command>\n", app_name);
   fprintf(stderr, "\t-c: list of cores separated by commas or dashes (e.g, -c 0-7,9,15-20)\n");
   exit(EXIT_FAILURE);
}

int main(int argc, char **argv){
   int ncores = get_nprocs();
   char* cores = NULL;
   char c;

   while ((c = getopt(argc, argv, "+c:")) != -1) {
      char * result = NULL;
      char * end_str;

      switch (c) {
         case 'c':
            if(cores) {
               fprintf(stderr, "-c already used !\n");
               exit(EXIT_FAILURE);
            }
            cores = malloc(strlen(optarg) + 1);
            strcpy(cores, optarg);

            result = strtok_r( optarg, "," , &end_str);
            while( result != NULL ) {
               char * end_str2;
               int prev = -1;

               char * result2 = strtok_r(result, "-", &end_str2);
               while(result2 != NULL) {
                  if(prev < 0) {
                     prev = atoi(result2);
                     if(prev < 0 || prev >= ncores){
                        fprintf(stderr, "%d is not a valid core number. Must be comprised between 0 and %d\n", prev, ncores-1);
                        exit(EXIT_FAILURE);
                     }
                  }
                  else {
                     int core = atoi(result2);
                     if(core < 0 || core >= ncores){
                        fprintf(stderr, "%d is not a valid core number. Must be comprised between 0 and %d\n", core, ncores-1);
                        exit(EXIT_FAILURE);
                     }

                     if(prev > core) {
                        fprintf(stderr, "%d-%d is not a valid core range\n", prev, core);
                        exit(EXIT_FAILURE);
                     }

                  }
                  
                  result2 = strtok_r(NULL, "-", &end_str2);
               }
               result = strtok_r(NULL, "," , &end_str);
            }
            break;
         default:
            usage(argv[0]);
      }
   }

   if(! cores) {
      usage(argv[0]);
   }

   if(optind == argc) {
      usage(argv[0]);
   }

   argv +=  optind;  
   
   setenv("PINTHREADS_CORES", cores, 1);
   setenv("LD_PRELOAD", "/usr/local/lib/pinthreads/pin.so", 1);

   execvp(argv[0], argv);
   perror("execvp");
   fprintf(stderr,"failed to execute %s\n", argv[0]);

   return EXIT_SUCCESS;
}
