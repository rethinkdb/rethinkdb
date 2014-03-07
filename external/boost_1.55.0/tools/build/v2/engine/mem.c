/*
Copyright Rene Rivera 2006.
Distributed under the Boost Software License, Version 1.0.
(See accompanying file LICENSE_1_0.txt or copy at
http://www.boost.org/LICENSE_1_0.txt)
*/

#include "jam.h"

#ifdef OPT_BOEHM_GC

    /* Compile the Boehm GC as one big chunk of code. It's much easier
    this way, than trying to make radical changes to the bjam build
    scripts. */

    #define ATOMIC_UNCOLLECTABLE
    #define NO_EXECUTE_PERMISSION
    #define ALL_INTERIOR_POINTERS

    #define LARGE_CONFIG
    /*
    #define NO_SIGNALS
    #define SILENT
    */
    #ifndef GC_DEBUG
    #define NO_DEBUGGING
    #endif

    #ifdef __GLIBC__
    #define __USE_GNU
    #endif

    #include "boehm_gc/reclaim.c"
    #include "boehm_gc/allchblk.c"
    #include "boehm_gc/misc.c"
    #include "boehm_gc/alloc.c"
    #include "boehm_gc/mach_dep.c"
    #include "boehm_gc/os_dep.c"
    #include "boehm_gc/mark_rts.c"
    #include "boehm_gc/headers.c"
    #include "boehm_gc/mark.c"
    #include "boehm_gc/obj_map.c"
    #include "boehm_gc/pcr_interface.c"
    #include "boehm_gc/blacklst.c"
    #include "boehm_gc/new_hblk.c"
    #include "boehm_gc/real_malloc.c"
    #include "boehm_gc/dyn_load.c"
    #include "boehm_gc/dbg_mlc.c"
    #include "boehm_gc/malloc.c"
    #include "boehm_gc/stubborn.c"
    #include "boehm_gc/checksums.c"
    #include "boehm_gc/pthread_support.c"
    #include "boehm_gc/pthread_stop_world.c"
    #include "boehm_gc/darwin_stop_world.c"
    #include "boehm_gc/typd_mlc.c"
    #include "boehm_gc/ptr_chck.c"
    #include "boehm_gc/mallocx.c"
    #include "boehm_gc/gcj_mlc.c"
    #include "boehm_gc/specific.c"
    #include "boehm_gc/gc_dlopen.c"
    #include "boehm_gc/backgraph.c"
    #include "boehm_gc/win32_threads.c"

    /* Needs to be last. */
    #include "boehm_gc/finalize.c"

#elif defined(OPT_DUMA)

    #ifdef OS_NT
        #define WIN32
    #endif
    #include "duma/duma.c"
    #include "duma/print.c"

#endif
