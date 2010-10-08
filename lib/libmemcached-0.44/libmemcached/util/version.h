/* LibMemcached
 * Copyright (C) 2010 Brian Aker
 * All rights reserved.
 *
 * Use and distribution licensed under the BSD license.  See
 * the COPYING file in the parent directory for full text.
 *
 * Summary: connect to all hosts, and make sure they meet a minimum version
 *
 */

#ifndef __LIBMEMCACHED_UTIL_VERSION_H__
#define __LIBMEMCACHED_UTIL_VERSION_H__

#ifdef __cplusplus
extern "C" {
#endif

LIBMEMCACHED_API
  bool libmemcached_util_version_check(memcached_st *memc, 
                                       uint8_t major_version,
                                       uint8_t minor_version,
                                       uint8_t micro_version);

#ifdef __cplusplus
}
#endif

#endif /* __LIBMEMCACHED__UTIL_VERSION_H__ */
