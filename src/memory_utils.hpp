#ifndef MEMORY_HPP_
#define MEMORY_HPP_

#include <stddef.h>

void *raw_malloc_aligned(size_t size, size_t alignment);
void raw_free_aligned(void *ptr);

#ifndef _WIN32
void *raw_malloc_page_aligned(size_t size);
#endif

/* Calls `malloc()` and checks its return value to crash if the allocation fails. */
void *rmalloc(size_t size);
/* Calls `realloc()` and checks its return value to crash if the allocation fails. */
void *rrealloc(void *ptr, size_t size);

#endif  // MEMORY_HPP_
