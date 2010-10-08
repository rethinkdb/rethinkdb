/* LibMemcached
 * Copyright (C) 2010 Brian Aker
 * All rights reserved.
 *
 * Use and distribution licensed under the BSD license.  See
 * the COPYING file in the parent directory for full text.
 *
 * Summary: Change the behavior of the memcached connection.
 *
 */

#ifndef __LIBMEMCACHED_RESPONSE_H__
#define __LIBMEMCACHED_RESPONSE_H__

#ifdef __cplusplus
extern "C" {
#endif

/* Read a single response from the server */
LIBMEMCACHED_LOCAL
memcached_return_t memcached_read_one_response(memcached_server_write_instance_st ptr,
                                               char *buffer, size_t buffer_length,
                                               memcached_result_st *result);

LIBMEMCACHED_LOCAL
memcached_return_t memcached_response(memcached_server_write_instance_st ptr,
                                      char *buffer, size_t buffer_length,
                                      memcached_result_st *result);

#ifdef __cplusplus
}
#endif

#endif /* __LIBMEMCACHED_RESPONSE_H__ */
