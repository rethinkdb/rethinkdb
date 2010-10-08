/* LibMemcached
 * Copyright (C) 2010 Brian Aker
 * All rights reserved.
 *
 * Use and distribution licensed under the BSD license.  See
 * the COPYING file in the parent directory for full text.
 *
 * Summary: Work with fetching results
 *
 */

#ifndef __LIBMEMCACHED_FLUSH_BUFFERS_H__
#define __LIBMEMCACHED_FLUSH_BUFFERS_H__

#ifdef __cplusplus
extern "C" {
#endif

LIBMEMCACHED_API
memcached_return_t memcached_flush_buffers(memcached_st *mem);

#ifdef __cplusplus
}
#endif

#endif /* __LIBMEMCACHED_FLUSH_BUFFERS_H__ */
