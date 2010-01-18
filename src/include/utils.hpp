
#ifndef __UTILS_HPP__
#define __UTILS_HPP__

#include <errno.h>
#include <stdlib.h>
#include <stdio.h>

static void check(const char *msg, bool err) {
    if (err) {
        if(errno == 0)
            errno = EINVAL;
        perror(msg);
        exit(-1);
    }
}

int get_cpu_count();
void *malloc_aligned(size_t size, size_t alignment = 64);

// Tokenizing strings
const char* tokenize(const char *str, unsigned int size,
                     const char *delims, unsigned int *token_size);
char* tokenize(char *str, unsigned int size,
               const char *delims, unsigned int *token_size);

#endif // __UTILS_HPP__

