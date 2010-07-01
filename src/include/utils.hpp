
#ifndef __UTILS_HPP__
#define __UTILS_HPP__

#include <errno.h>
#include <stdlib.h>
#include <stdio.h>
#include "corefwd.hpp"

static inline void check(const char *msg, bool err) {
    if (err) {
        if(errno == 0)
            errno = EINVAL;
        perror(msg);
        exit(-1);
    }
}

int get_cpu_count();
long get_available_ram();
long get_total_ram();


void *malloc_aligned(size_t size, size_t alignment = 64);

// Tokenizing strings
const char* tokenize(const char *str, unsigned int size,
                     const char *delims, unsigned int *token_size);
char* tokenize(char *str, unsigned int size,
               const char *delims, unsigned int *token_size);
bool token_eq(const char *str, char *token, int token_size);
bool contains_tokens(char *start, char *end, const char *delims);


// Buffer
template <int _size>
struct buffer_t //: public alloc_mixin_t<tls_small_obj_alloc_accessor<alloc_t>, buffer_t<_size> >
{
    char buf[_size];
    static const int size = _size;
};

#endif // __UTILS_HPP__

