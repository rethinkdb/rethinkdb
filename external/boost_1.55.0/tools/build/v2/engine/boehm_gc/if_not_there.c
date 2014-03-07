/* Conditionally execute a command based if the file argv[1] doesn't exist */
/* Except for execvp, we stick to ANSI C.				   */
# include "private/gcconfig.h"
# include <stdio.h>
# include <stdlib.h>
# include <unistd.h>
#ifdef __DJGPP__
#include <dirent.h>
#endif /* __DJGPP__ */

int main(int argc, char **argv, char **envp)
{
    FILE * f;
#ifdef __DJGPP__
    DIR * d;
#endif /* __DJGPP__ */
    if (argc < 3) goto Usage;
    if ((f = fopen(argv[1], "rb")) != 0
        || (f = fopen(argv[1], "r")) != 0) {
        fclose(f);
        return(0);
    }
#ifdef __DJGPP__
    if ((d = opendir(argv[1])) != 0) {
	    closedir(d);
	    return(0);
    }
#endif
    printf("^^^^Starting command^^^^\n");
    fflush(stdout);
    execvp(argv[2], argv+2);
    exit(1);
    
Usage:
    fprintf(stderr, "Usage: %s file_name command\n", argv[0]);
    return(1);
}

