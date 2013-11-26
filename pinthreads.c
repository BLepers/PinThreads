#define _GNU_SOURCE
#include <stdio.h>
#include <sys/sysinfo.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <libgen.h>
#include <assert.h>
#include "parse_args.h"

void usage(char * app_name) {
   fprintf(stderr, "Usage: %s -c <list of cores> <command>\n", app_name);
   fprintf(stderr, "\t-c: list of cores separated by commas or dashes (e.g, -c 0-7,9,15-20)\n");
   exit(EXIT_FAILURE);
}

#define PATH_LEN 512
char *get_lib_path() {
   char buffer[PATH_LEN], *lib, *path;
   int lib_path_len = readlink("/proc/self/exe", buffer, PATH_LEN);
   buffer[lib_path_len + 1] = '\0';
   path = dirname(buffer);
   assert(asprintf(&lib, "%s/pin.so", path));

   FILE * flib = fopen(lib, "r");
   if(!flib) {
      free(lib);
      assert(asprintf(&lib, PREFIX "lib/pinthreads/pin.so"));
   } else {
      fclose(flib);
   }
   return lib;
}

int main(int argc, char **argv){
   char c;
   char *cores = NULL;

   while ((c = getopt(argc, argv, "+c:")) != -1) {
      switch (c) {
         case 'c':
            if(cores) {
               fprintf(stderr, "-c already used !\n");
               exit(EXIT_FAILURE);
            }
            
            cores = malloc(strlen(optarg) + 1);
            strcpy(cores, optarg);

            parse_cores(optarg, NULL, NULL);
            break;
         default:
            usage(argv[0]);
      }
   }

   if(!cores) {
      usage(argv[0]);
   }

   if(optind == argc) {
      usage(argv[0]);
   }

   argv +=  optind;  
   
   char *lib = get_lib_path();
   setenv("PINTHREADS_CORES", cores, 1);
   setenv("LD_PRELOAD", lib, 1);
   free(lib);


   execvp(argv[0], argv);
   perror("execvp");
   fprintf(stderr,"failed to execute %s\n", argv[0]);

   return EXIT_SUCCESS;
}
