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

#ifndef CLIENTS_MS_ATOMIC_H
#define CLIENTS_MS_ATOMIC_H

#if defined(__SUNPRO_C)
# define _KERNEL
# include <atomic.h>
# if SIZEOF_SIZE_T == 8
#  define  atomic_add_size(X, Y) atomic_add_64((X), (Y))
#  define  atomic_add_size_nv(X, Y) atomic_add_64((X), (Y))
#  define  atomic_dec_size(X, Y) atomic_add_64((X), (Y))
#  define  atomic_dec_size_nv(X, Y) atomic_add_64((X), (Y))
# else
#  define  atomic_add_size(X, Y) atomic_add_32((X), (Y))
#  define  atomic_add_size_nv(X, Y) atomic_add_32((X), (Y))
#  define  atomic_dec_size(X, Y) atomic_add_32((X), (Y))
#  define  atomic_dec_size_nv(X, Y) atomic_add_32((X), (Y))
# endif
# undef _KERNEL
#else
# define atomic_add_8(X, Y)  __sync_fetch_and_add((X), (Y))
# define atomic_add_16(X, Y) __sync_fetch_and_add((X), (Y))
# define atomic_add_32(X, Y) __sync_fetch_and_add((X), (Y))
# define atomic_add_size(X, Y) __sync_fetch_and_add((X), (Y))
# define atomic_dec_8(X)  __sync_fetch_and_sub((X), 1)
# define atomic_dec_16(X) __sync_fetch_and_sub((X), 1)
# define atomic_dec_32(X) __sync_fetch_and_sub((X), 1)
# define atomic_dec_size(X) __sync_fetch_and_sub((X), 1)
/* The same as above, but these return the new value instead of void */
# define atomic_add_8_nv(X, Y)  __sync_fetch_and_add((X), (Y))
# define atomic_add_16_nv(X, Y) __sync_fetch_and_add((X), (Y))
# define atomic_add_32_nv(X, Y) __sync_fetch_and_add((X), (Y))
# define atomic_add_size_nv(X, Y) __sync_fetch_and_add((X), (Y))
# define atomic_dec_8_nv(X)  __sync_fetch_and_sub((X), 1)
# define atomic_dec_16_nv(X) __sync_fetch_and_sub((X), 1)
# define atomic_dec_32_nv(X) __sync_fetch_and_sub((X), 1)
# define atomic_dec_size_nv(X) __sync_fetch_and_sub((X), 1)
#endif /* defined(__SUNPRO_C) */

#endif /* CLIENTS_MS_ATOMIC_H */
