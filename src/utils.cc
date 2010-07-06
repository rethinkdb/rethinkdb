
#include <assert.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <new>
#include <exception>
#include "config/args.hpp"
#include "config/code.hpp"
#include "utils.hpp"
#include <algorithm>

int get_cpu_count() {
    return sysconf(_SC_NPROCESSORS_ONLN);
}

long get_available_ram() {
    return (long)sysconf(_SC_AVPHYS_PAGES) * (long)sysconf(_SC_PAGESIZE);
}

long get_total_ram() {
    return (long)sysconf(_SC_PHYS_PAGES) * (long)sysconf(_SC_PAGESIZE);
}

// Redefine operator new to do cache-lines alignment
void* operator new(size_t size) throw(std::bad_alloc) {
    // ERROR: You are using builtin operator new. Please use RethinkDB
    // allocator system instead
    assert(0);

    // Provide an implementation in case we do this in release mode.
    void *ptr = NULL;
    int res = posix_memalign(&ptr, 64, size);
    if(res != 0)
        throw std::bad_alloc();
    else
        return ptr;
}

void operator delete(void *p) {
    free(p);
}

void *malloc_aligned(size_t size, size_t alignment) {
    void *ptr = NULL;
    int res = posix_memalign(&ptr, alignment, size);
    if(res != 0)
        return NULL;
    else
        return ptr;
}

// fast non-null terminated string comparison
int sized_strcmp(const char *str1, int len1, const char *str2, int len2) {
    int min_len = std::min(len1, len2);
    int res = memcmp(str1, str2, min_len);
    if (res == 0)
        res = len1-len2;
    return res;
}

void *_gmalloc(size_t size) {
    void *ptr = NULL;
    int res = posix_memalign((void**)&ptr, 64, size);
    if(res != 0)
        return NULL;
    else
        return ptr;
}

void _gfree(void *ptr) {
    free(ptr);
}

