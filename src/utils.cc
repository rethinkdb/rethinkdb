
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <new>
#include <exception>
#include "utils.hpp"

int get_cpu_count() {
    return sysconf(_SC_NPROCESSORS_ONLN);
}

long get_available_ram() {
    return (long)sysconf(_SC_AVPHYS_PAGES) * (long)sysconf(_SC_PAGESIZE);
}

long get_total_ram() {
    return (long)sysconf(_SC_PHYS_PAGES) * (long)sysconf(_SC_PAGESIZE); }

// Redefine operator new to do cache-lines alignment
void* operator new(size_t size) throw(std::bad_alloc) {
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
