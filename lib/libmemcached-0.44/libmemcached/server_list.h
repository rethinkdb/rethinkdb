/* LibMemcached
 * Copyright (C) 2006-2009 Brian Aker
 * All rights reserved.
 *
 * Use and distribution licensed under the BSD license.  See
 * the COPYING file in the parent directory for full text.
 *
 * Summary: Types for libmemcached
 *
 */

#ifndef __LIBMEMCACHED_SERVER_LIST_H__
#define __LIBMEMCACHED_SERVER_LIST_H__

#ifdef __cplusplus
extern "C" {
#endif

/* Server List Public functions */
LIBMEMCACHED_API
  void memcached_server_list_free(memcached_server_list_st ptr);

LIBMEMCACHED_API
  memcached_return_t memcached_server_push(memcached_st *ptr, const memcached_server_list_st list);

LIBMEMCACHED_API
  memcached_server_list_st memcached_server_list_append(memcached_server_list_st ptr,
                                                        const char *hostname,
                                                        in_port_t port,
                                                        memcached_return_t *error);
LIBMEMCACHED_API
  memcached_server_list_st memcached_server_list_append_with_weight(memcached_server_list_st ptr,
                                                                    const char *hostname,
                                                                    in_port_t port,
                                                                    uint32_t weight,
                                                                    memcached_return_t *error);
LIBMEMCACHED_API
  uint32_t memcached_server_list_count(const memcached_server_list_st ptr);

LIBMEMCACHED_LOCAL
  uint32_t memcached_servers_set_count(memcached_server_list_st servers, uint32_t count);

LIBMEMCACHED_LOCAL
  memcached_server_st *memcached_server_list(const memcached_st *);

LIBMEMCACHED_LOCAL
  void memcached_server_list_set(memcached_st *self, memcached_server_list_st list);

#ifdef __cplusplus
} // extern "C"
#endif

#endif /* __LIBMEMCACHED_SERVER_LIST_H__ */
