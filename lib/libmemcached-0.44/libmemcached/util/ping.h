/* LibMemcached
 * Copyright (C) 2010 Brian Aker
 * All rights reserved.
 *
 * Use and distribution licensed under the BSD license.  See
 * the COPYING file in the parent directory for full text.
 *
 * Summary: connects to a host, and makes sure it is alive.
 *
 */

#ifndef __LIBMEMCACHED_PING_H__
#define __LIBMEMCACHED_PING_H__

#ifdef __cplusplus
extern "C" {
#endif

LIBMEMCACHED_API
bool libmemcached_util_ping(const char *hostname, in_port_t port, memcached_return_t *ret);

#ifdef __cplusplus
}
#endif

#endif /* __LIBMEMCACHED_PING_H__ */
