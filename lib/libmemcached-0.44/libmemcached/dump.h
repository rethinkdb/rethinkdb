/* LibMemcached
 * Copyright (C) 2006-2009 Brian Aker
 * All rights reserved.
 *
 * Use and distribution licensed under the BSD license.  See
 * the COPYING file in the parent directory for full text.
 *
 * Summary: Simple method for dumping data from Memcached.
 *
 */

#ifndef __LIBMEMCACHED_DUMP_H__
#define __LIBMEMCACHED_DUMP_H__

#ifdef __cplusplus
extern "C" {
#endif

LIBMEMCACHED_API
memcached_return_t memcached_dump(memcached_st *ptr, memcached_dump_fn *function, void *context, uint32_t number_of_callbacks);


#ifdef __cplusplus
}
#endif

#endif /* __LIBMEMCACHED_DUMP_H__ */
