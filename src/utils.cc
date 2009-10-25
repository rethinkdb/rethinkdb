
#include <errno.h>
#include <stdlib.h>
#include <stdio.h>
#include "utils.hpp"

void check(const char *str, int error) {
    if (error) {
        if(errno == 0)
            errno = EINVAL;
        perror(str);
        exit(-1);
    }
}

