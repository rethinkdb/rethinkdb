/* LibMemcached
 * Copyright (C) 2006-2009 Brian Aker
 * All rights reserved.
 *
 * Use and distribution licensed under the BSD license.  See
 * the COPYING file in the parent directory for full text.
 *
 * Summary: Change any of the possible callbacks.
 *
 */

#ifndef __LIBMEMCACHED_CALLBACK_H__
#define __LIBMEMCACHED_CALLBACK_H__

struct memcached_callback_st {
  memcached_execute_fn *callback;
  void *context;
  uint32_t number_of_callback;
};

#ifdef __cplusplus
extern "C" {
#endif

LIBMEMCACHED_API
memcached_return_t memcached_callback_set(memcached_st *ptr,
                                          const memcached_callback_t flag,
                                          void *data);
LIBMEMCACHED_API
void *memcached_callback_get(memcached_st *ptr,
                             const memcached_callback_t flag,
                             memcached_return_t *error);

#ifdef __cplusplus
}
#endif

#endif /* __LIBMEMCACHED_CALLBACK_H__ */
