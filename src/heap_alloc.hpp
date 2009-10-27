
#ifndef __HEAP_ALLOC_HPP__
#define __HEAP_ALLOC_HPP__

#include <stdlib.h>

struct heap_t {
    void *mem;
    size_t heap_size;
    size_t object_size;
    int *avail;
    int last_avail;
};

void create_heap(heap_t *heap, size_t object_size, size_t nobjects);
void destroy_heap(heap_t *heap);
void* heap_alloc(heap_t *heap);
void heap_free(heap_t *heap, void *obj);


#endif // __HEAP_ALLOC_HPP__

