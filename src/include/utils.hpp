
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

//fast string compare
int sized_strcmp(const char *str1, int len1, const char *str2, int len2);

// Buffer
template <int _size>
struct buffer_t : public alloc_mixin_t<tls_small_obj_alloc_accessor<alloc_t>, buffer_t<_size> >
{
    char buf[_size];
    static const int size = _size;
};

#endif // __UTILS_HPP__

