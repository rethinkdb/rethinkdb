/* LibMemcached
 * Copyright (C) 2010 Brian Aker
 * All rights reserved.
 *
 * Use and distribution licensed under the BSD license.  See
 * the COPYING file in the parent directory for full text.
 *
 * Summary: returns a human readable string for the error message
 *
 */

#ifndef __LIBMEMCACHED_QUIT_H__
#define __LIBMEMCACHED_QUIT_H__

#ifdef __cplusplus
extern "C" {
#endif

LIBMEMCACHED_API
void memcached_quit(memcached_st *ptr);

LIBMEMCACHED_LOCAL
void memcached_quit_server(memcached_server_st *ptr, bool io_death);

#ifdef __cplusplus
}
#endif

#endif /* __LIBMEMCACHED_QUIT_H__ */
