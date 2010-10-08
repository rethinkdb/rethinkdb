#include "common.h"

void _libmemcached_free(const memcached_st *ptr, void *mem, void *context)
{
  (void) ptr;
  (void) context;
  free(mem);
}

void *_libmemcached_malloc(const memcached_st *ptr, size_t size, void *context)
{
  (void) ptr;
  (void) context;
  return malloc(size);
}

void *_libmemcached_realloc(const memcached_st *ptr, void *mem, size_t size, void *context)
{
  (void) ptr;
  (void) context;
  return realloc(mem, size);
}

void *_libmemcached_calloc(const memcached_st *ptr, size_t nelem, size_t size, void *context)
{
  if (ptr->allocators.malloc != _libmemcached_malloc)
  {
     void *ret = _libmemcached_malloc(ptr, nelem * size, context);
     if (ret != NULL) 
       memset(ret, 0, nelem * size);

     return ret;
  }

  return calloc(nelem, size);
}

static const struct _allocators_st global_default_allocator= {
  .calloc= _libmemcached_calloc,
  .context= NULL,
  .free= _libmemcached_free,
  .malloc= _libmemcached_malloc,
  .realloc= _libmemcached_realloc
};

struct _allocators_st memcached_allocators_return_default(void)
{
  return global_default_allocator;
}

memcached_return_t memcached_set_memory_allocators(memcached_st *ptr,
                                                   memcached_malloc_fn mem_malloc,
                                                   memcached_free_fn mem_free,
                                                   memcached_realloc_fn mem_realloc,
                                                   memcached_calloc_fn mem_calloc,
                                                   void *context)
{
  /* All should be set, or none should be set */
  if (mem_malloc == NULL && mem_free == NULL && mem_realloc == NULL && mem_calloc == NULL) 
  {
    ptr->allocators= memcached_allocators_return_default();
  }
  else if (mem_malloc == NULL || mem_free == NULL || mem_realloc == NULL || mem_calloc == NULL)
  {
    return MEMCACHED_FAILURE;
  }
  else
  {
    ptr->allocators.malloc= mem_malloc;
    ptr->allocators.free= mem_free;
    ptr->allocators.realloc= mem_realloc;
    ptr->allocators.calloc= mem_calloc;
    ptr->allocators.context= context;
  }

  return MEMCACHED_SUCCESS;
}

void *memcached_get_memory_allocators_context(const memcached_st *ptr)
{
  return ptr->allocators.context;
}

void memcached_get_memory_allocators(const memcached_st *ptr,
                                     memcached_malloc_fn *mem_malloc,
                                     memcached_free_fn *mem_free,
                                     memcached_realloc_fn *mem_realloc,
                                     memcached_calloc_fn *mem_calloc)
{
   *mem_malloc= ptr->allocators.malloc;
   *mem_free= ptr->allocators.free;
   *mem_realloc= ptr->allocators.realloc;
   *mem_calloc= ptr->allocators.calloc;
}
