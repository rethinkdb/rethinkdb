#include "side_executable.hpp"

#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include "errors.hpp"

void consider_execve_side_executable(int argc, char **argv, const char *command_suffix) {
    if (argc >= 2 && !strcmp(argv[1], command_suffix)) {

        // We use "$argv[0]-$argv[1]" as the command name to search
        // for, and use "$argv[0] $argv[1]" as the command name to
        // pass on as the value of argv[0].

        char *commandbuf = (char *)malloc(strlen(argv[0]) + 1 + strlen(argv[1]) + 1);
        char *argv_0_buf = (char *)malloc(strlen(argv[0]) + 1 + strlen(argv[1]) + 1);
        
        strcpy(commandbuf, argv[0]);
        strcpy(argv_0_buf, argv[0]);
        commandbuf[strlen(argv[0])] = '-';
        argv_0_buf[strlen(argv[0])] = ' ';
        strcpy(commandbuf + strlen(argv[0]) + 1, argv[1]);
        strcpy(argv_0_buf + strlen(argv[0]) + 1, argv[1]);

        // TODO: is this really proper Unix behavior?  Maybe look at
        // how git searches for its subcommands.

        // Drop the first argument from argv.
        argv[1] = argv_0_buf;
        int code = execvp(commandbuf, argv + 1);

        // execvp will not return unless an error has happened.
        check("Tried and failed to find an executable file for '%s'.", code);
    }
}
