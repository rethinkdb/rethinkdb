
#ifndef __ALLOC_BLACKHOLE_HPP__
#define __ALLOC_BLACKHOLE_HPP__

#include <unistd.h>

struct alloc_blackhole_t {
    void *heap;
    size_t size;
    void *avail;
    size_t alignment;
};

void create_allocator(alloc_blackhole_t *allocator, size_t size);
void destroy_allocator(alloc_blackhole_t *allocator);

size_t get_alignment(alloc_blackhole_t *allocator);
void set_alignment(alloc_blackhole_t *allocator, size_t alignment);

void* malloc(alloc_blackhole_t *allocator, size_t size);
void free(alloc_blackhole_t *allocator, void *ptr);

#endif // __ALLOC_BLACKHOLE_HPP__

