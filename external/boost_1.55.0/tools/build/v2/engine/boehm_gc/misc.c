/* 
 * Copyright 1988, 1989 Hans-J. Boehm, Alan J. Demers
 * Copyright (c) 1991-1994 by Xerox Corporation.  All rights reserved.
 * Copyright (c) 1999-2001 by Hewlett-Packard Company. All rights reserved.
 *
 * THIS MATERIAL IS PROVIDED AS IS, WITH ABSOLUTELY NO WARRANTY EXPRESSED
 * OR IMPLIED.  ANY USE IS AT YOUR OWN RISK.
 *
 * Permission is hereby granted to use or copy this program
 * for any purpose,  provided the above notices are retained on all copies.
 * Permission to modify the code and to distribute modified code is granted,
 * provided the above notices are retained, and a notice that the code was
 * modified is included with the above copyright notice.
 */
/* Boehm, July 31, 1995 5:02 pm PDT */


#include <stdio.h>
#include <limits.h>
#include <stdarg.h>
#ifndef _WIN32_WCE
#include <signal.h>
#endif

#define I_HIDE_POINTERS	/* To make GC_call_with_alloc_lock visible */
#include "private/gc_pmark.h"

#ifdef GC_SOLARIS_THREADS
# include <sys/syscall.h>
#endif
#if defined(MSWIN32) || defined(MSWINCE)
# define WIN32_LEAN_AND_MEAN
# define NOSERVICE
# include <windows.h>
# include <tchar.h>
#endif

#ifdef UNIX_LIKE
# include <fcntl.h>
# include <sys/types.h>
# include <sys/stat.h>

  int GC_log;  /* Forward decl, so we can set it.	*/
#endif

#ifdef NONSTOP
# include <floss.h>
#endif

#if defined(THREADS) && defined(PCR)
# include "il/PCR_IL.h"
  PCR_Th_ML GC_allocate_ml;
#endif
/* For other platforms with threads, the lock and possibly		*/
/* GC_lock_holder variables are defined in the thread support code.	*/

#if defined(NOSYS) || defined(ECOS)
#undef STACKBASE
#endif

/* Dont unnecessarily call GC_register_main_static_data() in case 	*/
/* dyn_load.c isn't linked in.						*/
#ifdef DYNAMIC_LOADING
# define GC_REGISTER_MAIN_STATIC_DATA() GC_register_main_static_data()
#else
# define GC_REGISTER_MAIN_STATIC_DATA() TRUE
#endif

GC_FAR struct _GC_arrays GC_arrays /* = { 0 } */;


GC_bool GC_debugging_started = FALSE;
	/* defined here so we don't have to load debug_malloc.o */

void (*GC_check_heap) (void) = (void (*) (void))0;
void (*GC_print_all_smashed) (void) = (void (*) (void))0;

void (*GC_start_call_back) (void) = (void (*) (void))0;

ptr_t GC_stackbottom = 0;

#ifdef IA64
  ptr_t GC_register_stackbottom = 0;
#endif

GC_bool GC_dont_gc = 0;

GC_bool GC_dont_precollect = 0;

GC_bool GC_quiet = 0;

#ifndef SMALL_CONFIG
  GC_bool GC_print_stats = 0;
#endif

GC_bool GC_print_back_height = 0;

#ifndef NO_DEBUGGING
  GC_bool GC_dump_regularly = 0;  /* Generate regular debugging dumps. */
#endif

#ifdef KEEP_BACK_PTRS
  long GC_backtraces = 0;	/* Number of random backtraces to 	*/
  				/* generate for each GC.		*/
#endif

#ifdef FIND_LEAK
  int GC_find_leak = 1;
#else
  int GC_find_leak = 0;
#endif

#ifdef ALL_INTERIOR_POINTERS
  int GC_all_interior_pointers = 1;
#else
  int GC_all_interior_pointers = 0;
#endif

long GC_large_alloc_warn_interval = 5;
	/* Interval between unsuppressed warnings.	*/

long GC_large_alloc_warn_suppressed = 0;
	/* Number of warnings suppressed so far.	*/

/*ARGSUSED*/
void * GC_default_oom_fn(size_t bytes_requested)
{
    return(0);
}

void * (*GC_oom_fn) (size_t bytes_requested) = GC_default_oom_fn;

void * GC_project2(void *arg1, void *arg2)
{
  return arg2;
}

/* Set things up so that GC_size_map[i] >= granules(i),		*/
/* but not too much bigger						*/
/* and so that size_map contains relatively few distinct entries 	*/
/* This was originally stolen from Russ Atkinson's Cedar		*/
/* quantization alogrithm (but we precompute it).			*/ 
void GC_init_size_map(void)
{
    int i;

    /* Map size 0 to something bigger.			*/
    /* This avoids problems at lower levels.		*/
      GC_size_map[0] = 1;
    for (i = 1; i <= GRANULES_TO_BYTES(TINY_FREELISTS-1) - EXTRA_BYTES; i++) {
        GC_size_map[i] = ROUNDED_UP_GRANULES(i);
        GC_ASSERT(GC_size_map[i] < TINY_FREELISTS);
    }
    /* We leave the rest of the array to be filled in on demand. */
}

/* Fill in additional entries in GC_size_map, including the ith one */
/* We assume the ith entry is currently 0.				*/
/* Note that a filled in section of the array ending at n always    */
/* has length at least n/4.						*/
void GC_extend_size_map(size_t i)
{
    size_t orig_granule_sz = ROUNDED_UP_GRANULES(i);
    size_t granule_sz = orig_granule_sz;
    size_t byte_sz = GRANULES_TO_BYTES(granule_sz);
    			/* The size we try to preserve.		*/
    			/* Close to i, unless this would	*/
    			/* introduce too many distinct sizes.	*/
    size_t smaller_than_i = byte_sz - (byte_sz >> 3);
    size_t much_smaller_than_i = byte_sz - (byte_sz >> 2);
    size_t low_limit;	/* The lowest indexed entry we 	*/
    			/* initialize.			*/
    size_t j;
    
    if (GC_size_map[smaller_than_i] == 0) {
        low_limit = much_smaller_than_i;
        while (GC_size_map[low_limit] != 0) low_limit++;
    } else {
        low_limit = smaller_than_i + 1;
        while (GC_size_map[low_limit] != 0) low_limit++;
        granule_sz = ROUNDED_UP_GRANULES(low_limit);
        granule_sz += granule_sz >> 3;
        if (granule_sz < orig_granule_sz) granule_sz = orig_granule_sz;
    }
    /* For these larger sizes, we use an even number of granules.	*/
    /* This makes it easier to, for example, construct a 16byte-aligned	*/
    /* allocator even if GRANULE_BYTES is 8.				*/
        granule_sz += 1;
        granule_sz &= ~1;
    if (granule_sz > MAXOBJGRANULES) {
        granule_sz = MAXOBJGRANULES;
    }
    /* If we can fit the same number of larger objects in a block,	*/
    /* do so.							*/ 
    {
        size_t number_of_objs = HBLK_GRANULES/granule_sz;
        granule_sz = HBLK_GRANULES/number_of_objs;
    	granule_sz &= ~1;
    }
    byte_sz = GRANULES_TO_BYTES(granule_sz);
    /* We may need one extra byte;			*/
    /* don't always fill in GC_size_map[byte_sz]	*/
    byte_sz -= EXTRA_BYTES;

    for (j = low_limit; j <= byte_sz; j++) GC_size_map[j] = granule_sz;  
}


/*
 * The following is a gross hack to deal with a problem that can occur
 * on machines that are sloppy about stack frame sizes, notably SPARC.
 * Bogus pointers may be written to the stack and not cleared for
 * a LONG time, because they always fall into holes in stack frames
 * that are not written.  We partially address this by clearing
 * sections of the stack whenever we get control.
 */
word GC_stack_last_cleared = 0;	/* GC_no when we last did this */
# ifdef THREADS
#   define BIG_CLEAR_SIZE 2048	/* Clear this much now and then.	*/
#   define SMALL_CLEAR_SIZE 256 /* Clear this much every time.		*/
# endif
# define CLEAR_SIZE 213  /* Granularity for GC_clear_stack_inner */
# define DEGRADE_RATE 50

ptr_t GC_min_sp;	/* Coolest stack pointer value from which we've */
			/* already cleared the stack.			*/
			
ptr_t GC_high_water;
			/* "hottest" stack pointer value we have seen	*/
			/* recently.  Degrades over time.		*/

word GC_bytes_allocd_at_reset;

#if defined(ASM_CLEAR_CODE)
  extern void *GC_clear_stack_inner(void *, ptr_t);
#else  
/* Clear the stack up to about limit.  Return arg. */
/*ARGSUSED*/
void * GC_clear_stack_inner(void *arg, ptr_t limit)
{
    word dummy[CLEAR_SIZE];
    
    BZERO(dummy, CLEAR_SIZE*sizeof(word));
    if ((ptr_t)(dummy) COOLER_THAN limit) {
        (void) GC_clear_stack_inner(arg, limit);
    }
    /* Make sure the recursive call is not a tail call, and the bzero	*/
    /* call is not recognized as dead code.				*/
    GC_noop1((word)dummy);
    return(arg);
}
#endif

/* Clear some of the inaccessible part of the stack.  Returns its	*/
/* argument, so it can be used in a tail call position, hence clearing  */
/* another frame.							*/
void * GC_clear_stack(void *arg)
{
    ptr_t sp = GC_approx_sp();  /* Hotter than actual sp */
#   ifdef THREADS
        word dummy[SMALL_CLEAR_SIZE];
	static unsigned random_no = 0;
       			 	 /* Should be more random than it is ... */
				 /* Used to occasionally clear a bigger	 */
				 /* chunk.				 */
#   endif
    ptr_t limit;
    
#   define SLOP 400
	/* Extra bytes we clear every time.  This clears our own	*/
	/* activation record, and should cause more frequent		*/
	/* clearing near the cold end of the stack, a good thing.	*/
#   define GC_SLOP 4000
	/* We make GC_high_water this much hotter than we really saw   	*/
	/* saw it, to cover for GC noise etc. above our current frame.	*/
#   define CLEAR_THRESHOLD 100000
	/* We restart the clearing process after this many bytes of	*/
	/* allocation.  Otherwise very heavily recursive programs	*/
	/* with sparse stacks may result in heaps that grow almost	*/
	/* without bounds.  As the heap gets larger, collection 	*/
	/* frequency decreases, thus clearing frequency would decrease, */
	/* thus more junk remains accessible, thus the heap gets	*/
	/* larger ...							*/
# ifdef THREADS
    if (++random_no % 13 == 0) {
	limit = sp;
	MAKE_HOTTER(limit, BIG_CLEAR_SIZE*sizeof(word));
	limit = (ptr_t)((word)limit & ~0xf);
        		/* Make it sufficiently aligned for assembly	*/
        		/* implementations of GC_clear_stack_inner.	*/
	return GC_clear_stack_inner(arg, limit);
    } else {
	BZERO(dummy, SMALL_CLEAR_SIZE*sizeof(word));
	return arg;
    }
# else
    if (GC_gc_no > GC_stack_last_cleared) {
        /* Start things over, so we clear the entire stack again */
        if (GC_stack_last_cleared == 0) GC_high_water = (ptr_t)GC_stackbottom;
        GC_min_sp = GC_high_water;
        GC_stack_last_cleared = GC_gc_no;
        GC_bytes_allocd_at_reset = GC_bytes_allocd;
    }
    /* Adjust GC_high_water */
        MAKE_COOLER(GC_high_water, WORDS_TO_BYTES(DEGRADE_RATE) + GC_SLOP);
        if (sp HOTTER_THAN GC_high_water) {
            GC_high_water = sp;
        }
        MAKE_HOTTER(GC_high_water, GC_SLOP);
    limit = GC_min_sp;
    MAKE_HOTTER(limit, SLOP);
    if (sp COOLER_THAN limit) {
        limit = (ptr_t)((word)limit & ~0xf);
			/* Make it sufficiently aligned for assembly	*/
        		/* implementations of GC_clear_stack_inner.	*/
        GC_min_sp = sp;
        return(GC_clear_stack_inner(arg, limit));
    } else if (GC_bytes_allocd - GC_bytes_allocd_at_reset > CLEAR_THRESHOLD) {
    	/* Restart clearing process, but limit how much clearing we do. */
    	GC_min_sp = sp;
    	MAKE_HOTTER(GC_min_sp, CLEAR_THRESHOLD/4);
    	if (GC_min_sp HOTTER_THAN GC_high_water) GC_min_sp = GC_high_water;
    	GC_bytes_allocd_at_reset = GC_bytes_allocd;
    }  
    return(arg);
# endif
}


/* Return a pointer to the base address of p, given a pointer to a	*/
/* an address within an object.  Return 0 o.w.				*/
void * GC_base(void * p)
{
    ptr_t r;
    struct hblk *h;
    bottom_index *bi;
    hdr *candidate_hdr;
    ptr_t limit;
    
    r = p;
    if (!GC_is_initialized) return 0;
    h = HBLKPTR(r);
    GET_BI(r, bi);
    candidate_hdr = HDR_FROM_BI(bi, r);
    if (candidate_hdr == 0) return(0);
    /* If it's a pointer to the middle of a large object, move it	*/
    /* to the beginning.						*/
	while (IS_FORWARDING_ADDR_OR_NIL(candidate_hdr)) {
	   h = FORWARDED_ADDR(h,candidate_hdr);
	   r = (ptr_t)h;
	   candidate_hdr = HDR(h);
	}
    if (HBLK_IS_FREE(candidate_hdr)) return(0);
    /* Make sure r points to the beginning of the object */
	r = (ptr_t)((word)r & ~(WORDS_TO_BYTES(1) - 1));
        {
	    size_t offset = HBLKDISPL(r);
	    signed_word sz = candidate_hdr -> hb_sz;
	    size_t obj_displ = offset % sz;

	    r -= obj_displ;
            limit = r + sz;
	    if (limit > (ptr_t)(h + 1) && sz <= HBLKSIZE) {
	        return(0);
	    }
	    if ((ptr_t)p >= limit) return(0);
	}
    return((void *)r);
}


/* Return the size of an object, given a pointer to its base.		*/
/* (For small obects this also happens to work from interior pointers,	*/
/* but that shouldn't be relied upon.)					*/
size_t GC_size(void * p)
{
    hdr * hhdr = HDR(p);
    
    return hhdr -> hb_sz;
}

size_t GC_get_heap_size(void)
{
    return GC_heapsize;
}

size_t GC_get_free_bytes(void)
{
    return GC_large_free_bytes;
}

size_t GC_get_bytes_since_gc(void)
{
    return GC_bytes_allocd;
}

size_t GC_get_total_bytes(void)
{
    return GC_bytes_allocd+GC_bytes_allocd_before_gc;
}

GC_bool GC_is_initialized = FALSE;

# if defined(PARALLEL_MARK) || defined(THREAD_LOCAL_ALLOC)
  extern void GC_init_parallel(void);
# endif /* PARALLEL_MARK || THREAD_LOCAL_ALLOC */

void GC_init(void)
{
    DCL_LOCK_STATE;
    
#if defined(GC_WIN32_THREADS) && !defined(GC_PTHREADS)
    if (!GC_is_initialized) {
      BOOL (WINAPI *pfn) (LPCRITICAL_SECTION, DWORD) = NULL;
      HMODULE hK32 = GetModuleHandleA("kernel32.dll");
      if (hK32)
	  pfn = (BOOL (WINAPI *) (LPCRITICAL_SECTION, DWORD))
		GetProcAddress (hK32,
				"InitializeCriticalSectionAndSpinCount");
      if (pfn)
          pfn(&GC_allocate_ml, 4000);
      else
	  InitializeCriticalSection (&GC_allocate_ml);
    }
#endif /* MSWIN32 */

    LOCK();
    GC_init_inner();
    UNLOCK();

#   if defined(PARALLEL_MARK) || defined(THREAD_LOCAL_ALLOC)
	/* Make sure marker threads and started and thread local */
	/* allocation is initialized, in case we didn't get 	 */
	/* called from GC_init_parallel();			 */
        {
	  GC_init_parallel();
	}
#   endif /* PARALLEL_MARK || THREAD_LOCAL_ALLOC */

#   if defined(DYNAMIC_LOADING) && defined(DARWIN)
    {
        /* This must be called WITHOUT the allocation lock held
        and before any threads are created */
        extern void GC_init_dyld();
        GC_init_dyld();
    }
#   endif
}

#if defined(MSWIN32) || defined(MSWINCE)
    CRITICAL_SECTION GC_write_cs;
#endif

#ifdef MSWIN32
    extern void GC_init_win32(void);
#endif

extern void GC_setpagesize();


#ifdef MSWIN32
extern GC_bool GC_no_win32_dlls;
#else
# define GC_no_win32_dlls FALSE
#endif

void GC_exit_check(void)
{
   GC_gcollect();
}

#ifdef SEARCH_FOR_DATA_START
  extern void GC_init_linux_data_start(void);
#endif

#ifdef UNIX_LIKE

extern void GC_set_and_save_fault_handler(void (*handler)(int));

static void looping_handler(sig)
int sig;
{
    GC_err_printf("Caught signal %d: looping in handler\n", sig);
    for(;;);
}

static GC_bool installed_looping_handler = FALSE;

static void maybe_install_looping_handler()
{
    /* Install looping handler before the write fault handler, so we	*/
    /* handle write faults correctly.					*/
      if (!installed_looping_handler && 0 != GETENV("GC_LOOP_ON_ABORT")) {
        GC_set_and_save_fault_handler(looping_handler);
        installed_looping_handler = TRUE;
      }
}

#else /* !UNIX_LIKE */

# define maybe_install_looping_handler()

#endif

void GC_init_inner()
{
#   if !defined(THREADS) && defined(GC_ASSERTIONS)
        word dummy;
#   endif
    word initial_heap_sz = (word)MINHINCR;
    
    if (GC_is_initialized) return;
#   if defined(MSWIN32) || defined(MSWINCE)
      InitializeCriticalSection(&GC_write_cs);
#   endif
#   if (!defined(SMALL_CONFIG))
      if (0 != GETENV("GC_PRINT_STATS")) {
        GC_print_stats = 1;
      } 
      if (0 != GETENV("GC_PRINT_VERBOSE_STATS")) {
        GC_print_stats = VERBOSE;
      } 
#     if defined(UNIX_LIKE)
        {
	  char * file_name = GETENV("GC_LOG_FILE");
          if (0 != file_name) {
	    int log_d = open(file_name, O_CREAT|O_WRONLY|O_APPEND, 0666);
	    if (log_d < 0) {
	      GC_log_printf("Failed to open %s as log file\n", file_name);
	    } else {
	      GC_log = log_d;
	    }
	  }
	}
#     endif
#   endif
#   ifndef NO_DEBUGGING
      if (0 != GETENV("GC_DUMP_REGULARLY")) {
        GC_dump_regularly = 1;
      }
#   endif
#   ifdef KEEP_BACK_PTRS
      {
        char * backtraces_string = GETENV("GC_BACKTRACES");
        if (0 != backtraces_string) {
          GC_backtraces = atol(backtraces_string);
	  if (backtraces_string[0] == '\0') GC_backtraces = 1;
        }
      }
#   endif
    if (0 != GETENV("GC_FIND_LEAK")) {
      GC_find_leak = 1;
      atexit(GC_exit_check);
    }
    if (0 != GETENV("GC_ALL_INTERIOR_POINTERS")) {
      GC_all_interior_pointers = 1;
    }
    if (0 != GETENV("GC_DONT_GC")) {
      GC_dont_gc = 1;
    }
    if (0 != GETENV("GC_PRINT_BACK_HEIGHT")) {
      GC_print_back_height = 1;
    }
    if (0 != GETENV("GC_NO_BLACKLIST_WARNING")) {
      GC_large_alloc_warn_interval = LONG_MAX;
    }
    {
      char * addr_string = GETENV("GC_TRACE");
      if (0 != addr_string) {
#       ifndef ENABLE_TRACE
	  WARN("Tracing not enabled: Ignoring GC_TRACE value\n", 0);
#       else
#	  ifdef STRTOULL
	    long long addr = strtoull(addr_string, NULL, 16);
#	  else
	    long addr = strtoul(addr_string, NULL, 16);
#	  endif
	  if (addr < 0x1000)
	      WARN("Unlikely trace address: 0x%lx\n", (GC_word)addr);
	  GC_trace_addr = (ptr_t)addr;
#	endif
      }
    }
    {
      char * time_limit_string = GETENV("GC_PAUSE_TIME_TARGET");
      if (0 != time_limit_string) {
        long time_limit = atol(time_limit_string);
        if (time_limit < 5) {
	  WARN("GC_PAUSE_TIME_TARGET environment variable value too small "
	       "or bad syntax: Ignoring\n", 0);
        } else {
	  GC_time_limit = time_limit;
        }
      }
    }
    {
      char * interval_string = GETENV("GC_LARGE_ALLOC_WARN_INTERVAL");
      if (0 != interval_string) {
        long interval = atol(interval_string);
        if (interval <= 0) {
	  WARN("GC_LARGE_ALLOC_WARN_INTERVAL environment variable has "
	       "bad value: Ignoring\n", 0);
        } else {
	  GC_large_alloc_warn_interval = interval;
        }
      }
    }
    maybe_install_looping_handler();
    /* Adjust normal object descriptor for extra allocation.	*/
    if (ALIGNMENT > GC_DS_TAGS && EXTRA_BYTES != 0) {
      GC_obj_kinds[NORMAL].ok_descriptor = ((word)(-ALIGNMENT) | GC_DS_LENGTH);
    }
    GC_setpagesize();
    GC_exclude_static_roots(beginGC_arrays, endGC_arrays);
    GC_exclude_static_roots(beginGC_obj_kinds, endGC_obj_kinds);
#   ifdef SEPARATE_GLOBALS
      GC_exclude_static_roots(beginGC_objfreelist, endGC_objfreelist);
      GC_exclude_static_roots(beginGC_aobjfreelist, endGC_aobjfreelist);
#   endif
#   ifdef MSWIN32
 	GC_init_win32();
#   endif
#   if defined(USE_PROC_FOR_LIBRARIES) && defined(GC_LINUX_THREADS)
	WARN("USE_PROC_FOR_LIBRARIES + GC_LINUX_THREADS performs poorly.\n", 0);
	/* If thread stacks are cached, they tend to be scanned in 	*/
	/* entirety as part of the root set.  This wil grow them to	*/
	/* maximum size, and is generally not desirable.		*/
#   endif
#   if defined(SEARCH_FOR_DATA_START)
	GC_init_linux_data_start();
#   endif
#   if (defined(NETBSD) || defined(OPENBSD)) && defined(__ELF__)
	GC_init_netbsd_elf();
#   endif
#   if !defined(THREADS) || defined(GC_PTHREADS) || defined(GC_WIN32_THREADS) \
	|| defined(GC_SOLARIS_THREADS)
      if (GC_stackbottom == 0) {
	GC_stackbottom = GC_get_main_stack_base();
#       if (defined(LINUX) || defined(HPUX)) && defined(IA64)
	  GC_register_stackbottom = GC_get_register_stack_base();
#       endif
      } else {
#       if (defined(LINUX) || defined(HPUX)) && defined(IA64)
	  if (GC_register_stackbottom == 0) {
	    WARN("GC_register_stackbottom should be set with GC_stackbottom\n", 0);
	    /* The following may fail, since we may rely on	 	*/
	    /* alignment properties that may not hold with a user set	*/
	    /* GC_stackbottom.						*/
	    GC_register_stackbottom = GC_get_register_stack_base();
	  }
#	endif
      }
#   endif
    /* Ignore gcc -Wall warnings on the following. */
    GC_STATIC_ASSERT(sizeof (ptr_t) == sizeof(word));
    GC_STATIC_ASSERT(sizeof (signed_word) == sizeof(word));
    GC_STATIC_ASSERT(sizeof (struct hblk) == HBLKSIZE);
#   ifndef THREADS
#     ifdef STACK_GROWS_DOWN
        GC_ASSERT((word)(&dummy) <= (word)GC_stackbottom);
#     else
        GC_ASSERT((word)(&dummy) >= (word)GC_stackbottom);
#     endif
#   endif
#   if !defined(_AUX_SOURCE) || defined(__GNUC__)
      GC_ASSERT((word)(-1) > (word)0);
      /* word should be unsigned */
#   endif
    GC_ASSERT((ptr_t)(word)(-1) > (ptr_t)0);
    	/* Ptr_t comparisons should behave as unsigned comparisons.	*/
    GC_ASSERT((signed_word)(-1) < (signed_word)0);
#   if !defined(SMALL_CONFIG)
      if (GC_incremental || 0 != GETENV("GC_ENABLE_INCREMENTAL")) {
	/* This used to test for !GC_no_win32_dlls.  Why? */
        GC_setpagesize();
	/* For GWW_MPROTECT on Win32, this needs to happen before any	*/
	/* heap memory is allocated.					*/
        GC_dirty_init();
        GC_ASSERT(GC_bytes_allocd == 0)
    	GC_incremental = TRUE;
      }
#   endif /* !SMALL_CONFIG */
    
    /* Add initial guess of root sets.  Do this first, since sbrk(0)	*/
    /* might be used.							*/
      if (GC_REGISTER_MAIN_STATIC_DATA()) GC_register_data_segments();
    GC_init_headers();
    GC_bl_init();
    GC_mark_init();
    {
	char * sz_str = GETENV("GC_INITIAL_HEAP_SIZE");
	if (sz_str != NULL) {
	  initial_heap_sz = atoi(sz_str);
	  if (initial_heap_sz <= MINHINCR * HBLKSIZE) {
	    WARN("Bad initial heap size %s - ignoring it.\n",
		 sz_str);
	  } 
	  initial_heap_sz = divHBLKSZ(initial_heap_sz);
	}
    }
    {
	char * sz_str = GETENV("GC_MAXIMUM_HEAP_SIZE");
	if (sz_str != NULL) {
	  word max_heap_sz = (word)atol(sz_str);
	  if (max_heap_sz < initial_heap_sz * HBLKSIZE) {
	    WARN("Bad maximum heap size %s - ignoring it.\n",
		 sz_str);
	  } 
	  if (0 == GC_max_retries) GC_max_retries = 2;
	  GC_set_max_heap_size(max_heap_sz);
	}
    }
    if (!GC_expand_hp_inner(initial_heap_sz)) {
        GC_err_printf("Can't start up: not enough memory\n");
        EXIT();
    }
    GC_initialize_offsets();
    GC_register_displacement_inner(0L);
#   if defined(GC_LINUX_THREADS) && defined(REDIRECT_MALLOC)
      if (!GC_all_interior_pointers) {
	/* TLS ABI uses pointer-sized offsets for dtv. */
        GC_register_displacement_inner(sizeof(void *));
      }
#   endif
    GC_init_size_map();
#   ifdef PCR
      if (PCR_IL_Lock(PCR_Bool_false, PCR_allSigsBlocked, PCR_waitForever)
          != PCR_ERes_okay) {
          ABORT("Can't lock load state\n");
      } else if (PCR_IL_Unlock() != PCR_ERes_okay) {
          ABORT("Can't unlock load state\n");
      }
      PCR_IL_Unlock();
      GC_pcr_install();
#   endif
    GC_is_initialized = TRUE;
#   if defined(GC_PTHREADS) || defined(GC_WIN32_THREADS)
        GC_thr_init();
#   endif
    COND_DUMP;
    /* Get black list set up and/or incremental GC started */
      if (!GC_dont_precollect || GC_incremental) GC_gcollect_inner();
#   ifdef STUBBORN_ALLOC
    	GC_stubborn_init();
#   endif
#   if defined(GC_LINUX_THREADS) && defined(REDIRECT_MALLOC)
	{
	  extern void GC_init_lib_bounds(void);
	  GC_init_lib_bounds();
	}
#   endif
    /* Convince lint that some things are used */
#   ifdef LINT
      {
          extern char * GC_copyright[];
          extern int GC_read();
          extern void GC_register_finalizer_no_order();
          
          GC_noop(GC_copyright, GC_find_header,
                  GC_push_one, GC_call_with_alloc_lock, GC_read,
                  GC_dont_expand,
#		  ifndef NO_DEBUGGING
		    GC_dump,
#		  endif
                  GC_register_finalizer_no_order);
      }
#   endif
}

void GC_enable_incremental(void)
{
# if !defined(SMALL_CONFIG) && !defined(KEEP_BACK_PTRS)
  /* If we are keeping back pointers, the GC itself dirties all	*/
  /* pages on which objects have been marked, making 		*/
  /* incremental GC pointless.					*/
  if (!GC_find_leak) {
    DCL_LOCK_STATE;
    
    LOCK();
    if (GC_incremental) goto out;
    GC_setpagesize();
    /* if (GC_no_win32_dlls) goto out; Should be win32S test? */
    maybe_install_looping_handler();  /* Before write fault handler! */
    GC_incremental = TRUE;
    if (!GC_is_initialized) {
        GC_init_inner();
    } else {
	GC_dirty_init();
    }
    if (!GC_dirty_maintained) goto out;
    if (GC_dont_gc) {
        /* Can't easily do it. */
        UNLOCK();
    	return;
    }
    if (GC_bytes_allocd > 0) {
    	/* There may be unmarked reachable objects	*/
    	GC_gcollect_inner();
    }   /* else we're OK in assuming everything's	*/
    	/* clean since nothing can point to an	  	*/
    	/* unmarked object.			  	*/
    GC_read_dirty();
out:
    UNLOCK();
  } else {
    GC_init();
  }
# else
    GC_init();
# endif
}


#if defined(MSWIN32) || defined(MSWINCE)
# if defined(_MSC_VER) && defined(_DEBUG)
#  include <crtdbg.h>
# endif
# ifdef OLD_WIN32_LOG_FILE
#   define LOG_FILE _T("gc.log")
# endif

  HANDLE GC_stdout = 0;

  void GC_deinit()
  {
      if (GC_is_initialized) {
  	DeleteCriticalSection(&GC_write_cs);
      }
  }

# ifndef THREADS
#   define GC_need_to_lock 0  /* Not defined without threads */
# endif
  int GC_write(const char *buf, size_t len)
  {
      BOOL tmp;
      DWORD written;
      if (len == 0)
	  return 0;
      if (GC_need_to_lock) EnterCriticalSection(&GC_write_cs);
      if (GC_stdout == INVALID_HANDLE_VALUE) {
          if (GC_need_to_lock) LeaveCriticalSection(&GC_write_cs);
	  return -1;
      } else if (GC_stdout == 0) {
	char * file_name = GETENV("GC_LOG_FILE");
	char logPath[_MAX_PATH + 5];

        if (0 == file_name) {
#         ifdef OLD_WIN32_LOG_FILE
	    strcpy(logPath, LOG_FILE);
#	  else
	    GetModuleFileName(NULL, logPath, _MAX_PATH);
	    strcat(logPath, ".log");
#	  endif
	  file_name = logPath;
	}
	GC_stdout = CreateFile(logPath, GENERIC_WRITE,
        		       FILE_SHARE_READ,
        		       NULL, CREATE_ALWAYS, FILE_FLAG_WRITE_THROUGH,
        		       NULL); 
    	if (GC_stdout == INVALID_HANDLE_VALUE)
	    ABORT("Open of log file failed");
      }
      tmp = WriteFile(GC_stdout, buf, (DWORD)len, &written, NULL);
      if (!tmp)
	  DebugBreak();
#     if defined(_MSC_VER) && defined(_DEBUG)
	  _CrtDbgReport(_CRT_WARN, NULL, 0, NULL, "%.*s", len, buf);
#     endif
      if (GC_need_to_lock) LeaveCriticalSection(&GC_write_cs);
      return tmp ? (int)written : -1;
  }
# undef GC_need_to_lock

#endif

#if defined(OS2) || defined(MACOS)
FILE * GC_stdout = NULL;
FILE * GC_stderr = NULL;
FILE * GC_log = NULL;
int GC_tmp;  /* Should really be local ... */

  void GC_set_files()
  {
      if (GC_stdout == NULL) {
	GC_stdout = stdout;
    }
    if (GC_stderr == NULL) {
	GC_stderr = stderr;
    }
    if (GC_log == NULL) {
	GC_log = stderr;
    }
  }
#endif

#if !defined(OS2) && !defined(MACOS) && !defined(MSWIN32) && !defined(MSWINCE)
  int GC_stdout = 1;
  int GC_stderr = 2;
  int GC_log = 2;
# if !defined(AMIGA)
#   include <unistd.h>
# endif
#endif

#if !defined(MSWIN32) && !defined(MSWINCE) && !defined(OS2) \
    && !defined(MACOS)  && !defined(ECOS) && !defined(NOSYS)
int GC_write(fd, buf, len)
int fd;
const char *buf;
size_t len;
{
     register int bytes_written = 0;
     register int result;
     
     while (bytes_written < len) {
#	ifdef GC_SOLARIS_THREADS
	    result = syscall(SYS_write, fd, buf + bytes_written,
	    			  	    len - bytes_written);
#	else
     	    result = write(fd, buf + bytes_written, len - bytes_written);
#	endif
	if (-1 == result) return(result);
	bytes_written += result;
    }
    return(bytes_written);
}
#endif /* UN*X */

#ifdef ECOS
int GC_write(fd, buf, len)
{
  _Jv_diag_write (buf, len);
  return len;
}
#endif

#ifdef NOSYS
int GC_write(fd, buf, len)
{
  /* No writing.  */
  return len;
}
#endif


#if defined(MSWIN32) || defined(MSWINCE)
    /* FIXME: This is pretty ugly ... */
#   define WRITE(f, buf, len) GC_write(buf, len)
#else
#   if defined(OS2) || defined(MACOS)
#   define WRITE(f, buf, len) (GC_set_files(), \
			       GC_tmp = fwrite((buf), 1, (len), (f)), \
			       fflush(f), GC_tmp)
#   else
#     define WRITE(f, buf, len) GC_write((f), (buf), (len))
#   endif
#endif

#define BUFSZ 1024
#ifdef _MSC_VER
# define vsnprintf _vsnprintf
#endif
/* A version of printf that is unlikely to call malloc, and is thus safer */
/* to call from the collector in case malloc has been bound to GC_malloc. */
/* Floating point arguments ans formats should be avoided, since fp	  */
/* conversion is more likely to allocate.				  */
/* Assumes that no more than BUFSZ-1 characters are written at once.	  */
void GC_printf(const char *format, ...)
{
    va_list args;
    char buf[BUFSZ+1];
    
    va_start(args, format);
    if (GC_quiet) return;
    buf[BUFSZ] = 0x15;
    (void) vsnprintf(buf, BUFSZ, format, args);
    va_end(args);
    if (buf[BUFSZ] != 0x15) ABORT("GC_printf clobbered stack");
    if (WRITE(GC_stdout, buf, strlen(buf)) < 0) ABORT("write to stdout failed");
}

void GC_err_printf(const char *format, ...)
{
    va_list args;
    char buf[BUFSZ+1];
    
    va_start(args, format);
    buf[BUFSZ] = 0x15;
    (void) vsnprintf(buf, BUFSZ, format, args);
    va_end(args);
    if (buf[BUFSZ] != 0x15) ABORT("GC_printf clobbered stack");
    if (WRITE(GC_stderr, buf, strlen(buf)) < 0) ABORT("write to stderr failed");
}

void GC_log_printf(const char *format, ...)
{
    va_list args;
    char buf[BUFSZ+1];
    
    va_start(args, format);
    buf[BUFSZ] = 0x15;
    (void) vsnprintf(buf, BUFSZ, format, args);
    va_end(args);
    if (buf[BUFSZ] != 0x15) ABORT("GC_printf clobbered stack");
    if (WRITE(GC_log, buf, strlen(buf)) < 0) ABORT("write to log failed");
}

void GC_err_puts(const char *s)
{
    if (WRITE(GC_stderr, s, strlen(s)) < 0) ABORT("write to stderr failed");
}

#if defined(LINUX) && !defined(SMALL_CONFIG)
void GC_err_write(buf, len)
const char *buf;
size_t len;
{
    if (WRITE(GC_stderr, buf, len) < 0) ABORT("write to stderr failed");
}
#endif

void GC_default_warn_proc(char *msg, GC_word arg)
{
    GC_err_printf(msg, arg);
}

GC_warn_proc GC_current_warn_proc = GC_default_warn_proc;

GC_warn_proc GC_set_warn_proc(GC_warn_proc p)
{
    GC_warn_proc result;

#   ifdef GC_WIN32_THREADS
      GC_ASSERT(GC_is_initialized);
#   endif
    LOCK();
    result = GC_current_warn_proc;
    GC_current_warn_proc = p;
    UNLOCK();
    return(result);
}

GC_word GC_set_free_space_divisor (GC_word value)
{
    GC_word old = GC_free_space_divisor;
    GC_free_space_divisor = value;
    return old;
}

#ifndef PCR
void GC_abort(const char *msg)
{
#   if defined(MSWIN32)
      (void) MessageBoxA(NULL, msg, "Fatal error in gc", MB_ICONERROR|MB_OK);
#   else
      GC_err_printf("%s\n", msg);
#   endif
    if (GETENV("GC_LOOP_ON_ABORT") != NULL) {
	    /* In many cases it's easier to debug a running process.	*/
	    /* It's arguably nicer to sleep, but that makes it harder	*/
	    /* to look at the thread if the debugger doesn't know much	*/
	    /* about threads.						*/
	    for(;;) {}
    }
#   if defined(MSWIN32) || defined(MSWINCE)
	DebugBreak();
#   else
        (void) abort();
#   endif
}
#endif

void GC_enable()
{
    LOCK();
    GC_dont_gc--;
    UNLOCK();
}

void GC_disable()
{
    LOCK();
    GC_dont_gc++;
    UNLOCK();
}

/* Helper procedures for new kind creation.	*/
void ** GC_new_free_list_inner()
{
    void *result = GC_INTERNAL_MALLOC((MAXOBJGRANULES+1)*sizeof(ptr_t),
		    		      PTRFREE);
    if (result == 0) ABORT("Failed to allocate freelist for new kind");
    BZERO(result, (MAXOBJGRANULES+1)*sizeof(ptr_t));
    return result;
}

void ** GC_new_free_list()
{
    void *result;
    LOCK();
    result = GC_new_free_list_inner();
    UNLOCK();
    return result;
}

unsigned GC_new_kind_inner(void **fl, GC_word descr, int adjust, int clear)
{
    unsigned result = GC_n_kinds++;

    if (GC_n_kinds > MAXOBJKINDS) ABORT("Too many kinds");
    GC_obj_kinds[result].ok_freelist = fl;
    GC_obj_kinds[result].ok_reclaim_list = 0;
    GC_obj_kinds[result].ok_descriptor = descr;
    GC_obj_kinds[result].ok_relocate_descr = adjust;
    GC_obj_kinds[result].ok_init = clear;
    return result;
}

unsigned GC_new_kind(void **fl, GC_word descr, int adjust, int clear)
{
    unsigned result;
    LOCK();
    result = GC_new_kind_inner(fl, descr, adjust, clear);
    UNLOCK();
    return result;
}

unsigned GC_new_proc_inner(GC_mark_proc proc)
{
    unsigned result = GC_n_mark_procs++;

    if (GC_n_mark_procs > MAX_MARK_PROCS) ABORT("Too many mark procedures");
    GC_mark_procs[result] = proc;
    return result;
}

unsigned GC_new_proc(GC_mark_proc proc)
{
    unsigned result;
    LOCK();
    result = GC_new_proc_inner(proc);
    UNLOCK();
    return result;
}

void * GC_call_with_stack_base(GC_stack_base_func fn, void *arg)
{
    int dummy;
    struct GC_stack_base base;

    base.mem_base = (void *)&dummy;
#   ifdef IA64
      base.reg_base = (void *)GC_save_regs_in_stack();
      /* Unnecessarily flushes register stack, 		*/
      /* but that probably doesn't hurt.		*/
#   endif
    return fn(&base, arg);
}

#if !defined(NO_DEBUGGING)

void GC_dump()
{
    GC_printf("***Static roots:\n");
    GC_print_static_roots();
    GC_printf("\n***Heap sections:\n");
    GC_print_heap_sects();
    GC_printf("\n***Free blocks:\n");
    GC_print_hblkfreelist();
    GC_printf("\n***Blocks in use:\n");
    GC_print_block_list();
    GC_printf("\n***Finalization statistics:\n");
    GC_print_finalization_stats();
}

#endif /* NO_DEBUGGING */
