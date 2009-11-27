
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

// Redefine operator new to do cache-lines alignment
void* operator new(size_t size) throw(std::bad_alloc) {
    void *ptr;
    int res = posix_memalign(&ptr, 64, size);
    if(res != 0)
        std::__throw_bad_alloc();
    else
        return ptr;
}

void operator delete(void *p) {
    free(p);
}
