
#include <errno.h>
#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include "utils.hpp"

void check(const char *str, int error) {
    if (error) {
        if(errno == 0)
            errno = EINVAL;
        perror(str);
        exit(-1);
    }
}

int get_cpu_count() {
    return sysconf(_SC_NPROCESSORS_ONLN);
}

