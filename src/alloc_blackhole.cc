
#include <stdlib.h>
#include "utils.hpp"
#include "alloc_blackhole.hpp"

// TODO: the allocator isn't thread safe. Consider implementing an
// efficient, thread-safe allocator. Since we assign sockets in a
// round-robin fashion (we might want to introduce some randomness for
// safety). This might not be necessary, since local allocators might
// be sufficient because they should fill up evenly.

// TODO: report allocator statistics.

void create_allocator(alloc_blackhole_t *allocator, size_t size) {
    allocator->avail = allocator->heap = malloc(size);
    allocator->size = size;
}

void destroy_allocator(alloc_blackhole_t *allocator) {
    free(allocator->heap);
    allocator->avail = allocator->heap = NULL;
}

size_t get_alignment(alloc_blackhole_t *allocator) {
    return allocator->alignment;
}

void set_alignment(alloc_blackhole_t *allocator, size_t alignment) {
    allocator->alignment = alignment;
}

void* malloc(alloc_blackhole_t *allocator, size_t size) {
    if(allocator->alignment) {
        char *a = (char*)allocator->avail;
        a = a + ((long)a % allocator->alignment);
        allocator->avail = (void*)a;
    }
    check("No more memory to allocate in the black hole.",
          ((char*)allocator->avail - (char*)allocator->heap) + size >= allocator->size);
    void *mem = allocator->avail;
    allocator->avail = (char*)mem + size;
    return mem;
}

void free(alloc_blackhole_t *allocator, void *ptr) {
    // It's a black hole, we don't have to do anything!
}
