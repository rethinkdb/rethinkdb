
#include "heap_alloc.hpp"

void create_heap(heap_t *heap, size_t object_size, size_t nobjects) {
    heap->heap_size = object_size * nobjects;
    heap->object_size = object_size;
    heap->mem = malloc(heap->heap_size);
    heap->avail = (int*)malloc(sizeof(int) * nobjects);
    for(int i = 0; i < nobjects; i++) {
        heap->avail[i] = i;
    }
    heap->last_avail = nobjects - 1;
}

void destroy_heap(heap_t *heap) {
    free(heap->mem);
    heap->mem = NULL;
    free(heap->avail);
    heap->avail = NULL;
}

void* heap_alloc(heap_t *heap) {
    // TODO: add debug code
    if(heap->last_avail == -1)
        return NULL;
    void *addr = (char*)heap->mem + heap->avail[heap->last_avail] * heap->object_size;
    heap->last_avail--;
    return addr;
}

void heap_free(heap_t *heap, void *obj) {
    // TODO: add debug code
    heap->avail[heap->last_avail + 1] = ((char*)obj - (char*)heap->mem) / heap->object_size;
    heap->last_avail++;
}
