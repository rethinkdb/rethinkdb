/* LibMemcached
 * Copyright (C) 2006-2010 Brian Aker
 * All rights reserved.
 *
 * Use and distribution licensed under the BSD license.  See
 * the COPYING file in the parent directory for full text.
 *
 * Summary: Internal functions used by the library. Not for public use!
 *
 */

#ifndef __LIBMEMCACHED_DO_H__
#define __LIBMEMCACHED_DO_H__

#ifdef __cplusplus
extern "C" {
#endif

LIBMEMCACHED_LOCAL
memcached_return_t memcached_do(memcached_server_write_instance_st ptr,
                                const void *commmand,
                                size_t command_length,
                                bool with_flush);

LIBMEMCACHED_LOCAL
memcached_return_t memcached_vdo(memcached_server_write_instance_st ptr,
                                 const struct libmemcached_io_vector_st *vector, size_t count,
                                 bool with_flush);

#ifdef __cplusplus
}
#endif

#endif /* __LIBMEMCACHED_DO_H__ */
