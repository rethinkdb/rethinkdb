/* LibMemcached
 * Copyright (C) 2006-2009 Brian Aker
 * All rights reserved.
 *
 * Use and distribution licensed under the BSD license.  See
 * the COPYING file in the parent directory for full text.
 *
 * Summary:
 *
 */

#ifndef __LIBMEMCACHED_BYTEORDER_H__
#define __LIBMEMCACHED_BYTEORDER_H__

#include "config.h"

#if HAVE_SYS_TYPES_H
#include <sys/types.h>
#endif


/* Define this here, which will turn on the visibilty controls while we're
 * building libmemcached.
 */
#define BUILDING_LIBMEMCACHED 1

#include "libmemcached/memcached.h"

#ifndef HAVE_HTONLL
#define ntohll(a) memcached_ntohll(a)
#define htonll(a) memcached_htonll(a)

LIBMEMCACHED_LOCAL
uint64_t memcached_ntohll(uint64_t);
LIBMEMCACHED_LOCAL
uint64_t memcached_htonll(uint64_t);
#endif

#ifdef linux
/* /usr/include/netinet/in.h defines macros from ntohs() to _bswap_nn to
 * optimize the conversion functions, but the prototypes generate warnings
 * from gcc. The conversion methods isn't the bottleneck for my app, so
 * just remove the warnings by undef'ing the optimization ..
 */
#undef ntohs
#undef ntohl
#undef htons
#undef htonl
#endif

#endif /*__LIBMEMCACHED_BYTEORDER_H__ */
