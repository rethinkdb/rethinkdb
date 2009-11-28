
#ifndef __UTILS_HPP__
#define __UTILS_HPP__

#include <stdlib.h>

// TODO: make checking inline
void check(const char *str, int error);

int get_cpu_count();
void *malloc_aligned(size_t size, size_t alignment = 64);

#endif // __UTILS_HPP__

