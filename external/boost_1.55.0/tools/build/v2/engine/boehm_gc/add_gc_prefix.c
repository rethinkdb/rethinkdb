# include <stdio.h>
# include "version.h"
 
int main(argc, argv, envp)
int argc;
char ** argv;
char ** envp;
{
    int i;
    
    for (i = 1; i < argc; i++) {
      if (GC_ALPHA_VERSION == GC_NOT_ALPHA) {
	printf("gc%d.%d/%s ", GC_VERSION_MAJOR, GC_VERSION_MINOR, argv[i]);
      } else {
	printf("gc%d.%dalpha%d/%s ", GC_VERSION_MAJOR,
	       GC_VERSION_MINOR, GC_ALPHA_VERSION, argv[i]);
      }
    }
    return(0);
}
