/* LibMemcached
 * Copyright (C) 2010 Brian Aker
 * All rights reserved.
 *
 * Use and distribution licensed under the BSD license.  See
 * the COPYING file in the parent directory for full text.
 *
 * Summary: Change the verbository level of the memcached server
 *
 */

#ifndef __LIBMEMCACHED_VERBOSITY_H__
#define __LIBMEMCACHED_VERBOSITY_H__

#ifdef __cplusplus
extern "C" {
#endif

LIBMEMCACHED_API
memcached_return_t memcached_verbosity(memcached_st *ptr, uint32_t verbosity);


#ifdef __cplusplus
}
#endif

#endif /* __LIBMEMCACHED_VERBOSITY_H__ */
