/* LibMemcached
 * Copyright (C) 2006-2009 Brian Aker
 * All rights reserved.
 *
 * Use and distribution licensed under the BSD license.  See
 * the COPYING file in the parent directory for full text.
 *
 * Summary:
 *
 */

/*
  Common include file for libmemached
*/

#ifndef __LIBMEMCACHED_COMMON_H__
#define __LIBMEMCACHED_COMMON_H__

#include "config.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <strings.h>
#include <ctype.h>
#include <sys/types.h>
#include <unistd.h>
#include <limits.h>
#include <errno.h>
#include <fcntl.h>
#ifdef TIME_WITH_SYS_TIME
# include <sys/time.h>
# include <time.h>
#else
# ifdef HAVE_SYS_TIME_H
#  include <sys/time.h>
# else
#  include <time.h>
# endif
#endif

/* Define this here, which will turn on the visibilty controls while we're
 * building libmemcached.
 */
#define BUILDING_LIBMEMCACHED 1


#include "libmemcached/memcached.h"
#include "libmemcached/watchpoint.h"

typedef struct memcached_server_st * memcached_server_write_instance_st;

typedef memcached_return_t (*memcached_server_execute_fn)(memcached_st *ptr, memcached_server_write_instance_st server, void *context);

LIBMEMCACHED_LOCAL
memcached_server_write_instance_st memcached_server_instance_fetch(memcached_st *ptr, uint32_t server_key);

LIBMEMCACHED_LOCAL
memcached_return_t memcached_server_execute(memcached_st *ptr,
                                            memcached_server_execute_fn callback,
                                            void *context);


/* These are private not to be installed headers */
#include "libmemcached/io.h"
#include "libmemcached/do.h"
#include "libmemcached/internal.h"
#include "libmemcached/libmemcached_probes.h"
#include "libmemcached/memcached/protocol_binary.h"
#include "libmemcached/byteorder.h"
#include "libmemcached/response.h"

/* string value */
struct memcached_continuum_item_st
{
  uint32_t index;
  uint32_t value;
};

/* Yum, Fortran.... can you make the reference? */
typedef enum {
  MEM_NOT= -1,
  MEM_FALSE= false,
  MEM_TRUE= true
} memcached_ternary_t;


#if !defined(__GNUC__) || (__GNUC__ == 2 && __GNUC_MINOR__ < 96)

#define likely(x)       if((x))
#define unlikely(x)     if((x))

#else

#define likely(x)       if(__builtin_expect((x) != 0, 1))
#define unlikely(x)     if(__builtin_expect((x) != 0, 0))
#endif

#define MEMCACHED_BLOCK_SIZE 1024
#define MEMCACHED_DEFAULT_COMMAND_SIZE 350
#define SMALL_STRING_LEN 1024
#define HUGE_STRING_LEN 8196

#ifdef __cplusplus
extern "C" {
#endif

LIBMEMCACHED_LOCAL
memcached_return_t memcached_connect(memcached_server_write_instance_st ptr);

LIBMEMCACHED_LOCAL
memcached_return_t run_distribution(memcached_st *ptr);

#define memcached_server_response_increment(A) (A)->cursor_active++
#define memcached_server_response_decrement(A) (A)->cursor_active--
#define memcached_server_response_reset(A) (A)->cursor_active=0

// These are private 
#define memcached_is_allocated(__object) ((__object)->options.is_allocated)
#define memcached_is_initialized(__object) ((__object)->options.is_initialized)
#define memcached_is_purging(__object) ((__object)->state.is_purging)
#define memcached_is_processing_input(__object) ((__object)->state.is_processing_input)
#define memcached_set_purging(__object, __value) ((__object)->state.is_purging= (__value))
#define memcached_set_processing_input(__object, __value) ((__object)->state.is_processing_input= (__value))
#define memcached_set_initialized(__object, __value) ((__object)->options.is_initialized(= (__value))
#define memcached_set_allocated(__object, __value) ((__object)->options.is_allocated(= (__value))

LIBMEMCACHED_LOCAL
void set_last_disconnected_host(memcached_server_write_instance_st ptr);

LIBMEMCACHED_LOCAL
memcached_return_t memcached_key_test(const char * const *keys,
                                      const size_t *key_length,
                                      size_t number_of_keys);

LIBMEMCACHED_LOCAL
memcached_return_t memcached_purge(memcached_server_write_instance_st ptr);

LIBMEMCACHED_LOCAL
memcached_server_st *memcached_server_create_with(const memcached_st *memc,
                                                  memcached_server_write_instance_st host,
                                                  const char *hostname,
                                                  in_port_t port,
                                                  uint32_t weight,
                                                  memcached_connection_t type);


static inline memcached_return_t memcached_validate_key_length(size_t key_length, bool binary)
{
  unlikely (key_length == 0)
    return MEMCACHED_BAD_KEY_PROVIDED;

  if (binary)
  {
    unlikely (key_length > 0xffff)
      return MEMCACHED_BAD_KEY_PROVIDED;
  }
  else
  {
    unlikely (key_length >= MEMCACHED_MAX_KEY)
      return MEMCACHED_BAD_KEY_PROVIDED;
  }

  return MEMCACHED_SUCCESS;
}

#ifdef TCP_CORK
  #define CORK TCP_CORK
#elif defined TCP_NOPUSH
  #define CORK TCP_NOPUSH
#endif

/*
  test_cork() tries to enable TCP_CORK. IF TCP_CORK is not an option
  on the system it returns false but sets errno to 0. Otherwise on
  failure errno is set.
*/
static inline memcached_ternary_t test_cork(memcached_server_st *ptr, int enable)
{
#ifdef CORK
  if (ptr->type != MEMCACHED_CONNECTION_TCP)
    return MEM_FALSE;

  int err= setsockopt(ptr->fd, IPPROTO_TCP, CORK,
                      &enable, (socklen_t)sizeof(int));
  if (! err)
  {
    return MEM_TRUE;
  }

  perror(strerror(errno));
  ptr->cached_errno= errno;

  return MEM_FALSE;
#else
  (void)ptr;
  (void)enable;

  ptr->cached_errno= 0;

  return MEM_NOT;
#endif
}

static inline void libmemcached_free(const memcached_st *ptr, void *mem)
{
  ptr->allocators.free(ptr, mem, ptr->allocators.context);
}

static inline void *libmemcached_malloc(const memcached_st *ptr, const size_t size)
{
  return ptr->allocators.malloc(ptr, size, ptr->allocators.context);
}

static inline void *libmemcached_realloc(const memcached_st *ptr, void *mem, const size_t size)
{
  return ptr->allocators.realloc(ptr, mem, size, ptr->allocators.context);
}

static inline void *libmemcached_calloc(const memcached_st *ptr, size_t nelem, size_t size)
{
  return ptr->allocators.calloc(ptr, nelem, size, ptr->allocators.context);
}

#ifdef __cplusplus
}
#endif

#endif /* __LIBMEMCACHED_COMMON_H__ */
