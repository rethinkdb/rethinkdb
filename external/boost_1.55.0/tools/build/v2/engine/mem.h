/*
 * Copyright 2006. Rene Rivera
 * Distributed under the Boost Software License, Version 1.0.
 * (See accompanying file LICENSE_1_0.txt or copy at
 * http://www.boost.org/LICENSE_1_0.txt)
 */

#ifndef BJAM_MEM_H
#define BJAM_MEM_H

#ifdef OPT_BOEHM_GC

    /* Use Boehm GC memory allocator. */
    #include <gc.h>

    #define bjam_malloc_x(s) memset(GC_malloc(s),0,s)
    #define bjam_malloc_atomic_x(s) memset(GC_malloc_atomic(s),0,s)
    #define bjam_calloc_x(n,s) memset(GC_malloc((n)*(s)),0,(n)*(s))
    #define bjam_calloc_atomic_x(n,s) memset(GC_malloc_atomic((n)*(s)),0,(n)*(s))
    #define bjam_realloc_x(p,s) GC_realloc(p,s)
    #define bjam_free_x(p) GC_free(p)
    #define bjam_mem_init_x() GC_init(); GC_enable_incremental()

    #define bjam_malloc_raw_x(s) malloc(s)
    #define bjam_calloc_raw_x(n,s) calloc(n,s)
    #define bjam_realloc_raw_x(p,s) realloc(p,s)
    #define bjam_free_raw_x(p) free(p)

    #ifndef BJAM_NEWSTR_NO_ALLOCATE
    # define BJAM_NEWSTR_NO_ALLOCATE
    #endif

#elif defined( OPT_DUMA )

    /* Use Duma memory debugging library. */
    #include <stdlib.h>

    #define _DUMA_CONFIG_H_
    #define DUMA_NO_GLOBAL_MALLOC_FREE
    #define DUMA_EXPLICIT_INIT
    #define DUMA_NO_THREAD_SAFETY
    #define DUMA_NO_CPP_SUPPORT
    /* #define DUMA_NO_LEAKDETECTION */
    /* #define DUMA_USE_FRAMENO */
    /* #define DUMA_PREFER_ATEXIT */
    /* #define DUMA_OLD_DEL_MACRO */
    /* #define DUMA_NO_HANG_MSG */
    #define DUMA_PAGE_SIZE 4096
    #define DUMA_MIN_ALIGNMENT 1
    /* #define DUMA_GNU_INIT_ATTR 0 */
    typedef unsigned int DUMA_ADDR;
    typedef unsigned int DUMA_SIZE;
    #include <duma.h>

    #define bjam_malloc_x(s) malloc(s)
    #define bjam_calloc_x(n,s) calloc(n,s)
    #define bjam_realloc_x(p,s) realloc(p,s)
    #define bjam_free_x(p) free(p)

    #ifndef BJAM_NEWSTR_NO_ALLOCATE
    # define BJAM_NEWSTR_NO_ALLOCATE
    #endif

#else

    /* Standard C memory allocation. */
    #include <stdlib.h>

    #define bjam_malloc_x(s) malloc(s)
    #define bjam_calloc_x(n,s) calloc(n,s)
    #define bjam_realloc_x(p,s) realloc(p,s)
    #define bjam_free_x(p) free(p)

#endif

#ifndef bjam_malloc_atomic_x
    #define bjam_malloc_atomic_x(s) bjam_malloc_x(s)
#endif
#ifndef bjam_calloc_atomic_x
    #define bjam_calloc_atomic_x(n,s) bjam_calloc_x(n,s)
#endif
#ifndef bjam_mem_init_x
    #define bjam_mem_init_x()
#endif
#ifndef bjam_mem_close_x
    #define bjam_mem_close_x()
#endif
#ifndef bjam_malloc_raw_x
    #define bjam_malloc_raw_x(s) bjam_malloc_x(s)
#endif
#ifndef bjam_calloc_raw_x
    #define bjam_calloc_raw_x(n,s) bjam_calloc_x(n,s)
#endif
#ifndef bjam_realloc_raw_x
    #define bjam_realloc_raw_x(p,s) bjam_realloc_x(p,s)
#endif
#ifndef bjam_free_raw_x
    #define bjam_free_raw_x(p) bjam_free_x(p)
#endif

#ifdef OPT_DEBUG_PROFILE
    /* Profile tracing of memory allocations. */
    #include "debug.h"

    #define BJAM_MALLOC(s) (profile_memory(s), bjam_malloc_x(s))
    #define BJAM_MALLOC_ATOMIC(s) (profile_memory(s), bjam_malloc_atomic_x(s))
    #define BJAM_CALLOC(n,s) (profile_memory(n*s), bjam_calloc_x(n,s))
    #define BJAM_CALLOC_ATOMIC(n,s) (profile_memory(n*s), bjam_calloc_atomic_x(n,s))
    #define BJAM_REALLOC(p,s) (profile_memory(s), bjam_realloc_x(p,s))

    #define BJAM_MALLOC_RAW(s) (profile_memory(s), bjam_malloc_raw_x(s))
    #define BJAM_CALLOC_RAW(n,s) (profile_memory(n*s), bjam_calloc_raw_x(n,s))
    #define BJAM_REALLOC_RAW(p,s) (profile_memory(s), bjam_realloc_raw_x(p,s))
#else
    /* No mem tracing. */
    #define BJAM_MALLOC(s) bjam_malloc_x(s)
    #define BJAM_MALLOC_ATOMIC(s) bjam_malloc_atomic_x(s)
    #define BJAM_CALLOC(n,s) bjam_calloc_x(n,s)
    #define BJAM_CALLOC_ATOMIC(n,s) bjam_calloc_atomic_x(n,s)
    #define BJAM_REALLOC(p,s) bjam_realloc_x(p,s)

    #define BJAM_MALLOC_RAW(s) bjam_malloc_raw_x(s)
    #define BJAM_CALLOC_RAW(n,s) bjam_calloc_raw_x(n,s)
    #define BJAM_REALLOC_RAW(p,s) bjam_realloc_raw_x(p,s)
#endif

#define BJAM_MEM_INIT() bjam_mem_init_x()
#define BJAM_MEM_CLOSE() bjam_mem_close_x()

#define BJAM_FREE(p) bjam_free_x(p)
#define BJAM_FREE_RAW(p) bjam_free_raw_x(p)

#endif
