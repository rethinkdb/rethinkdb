/* 
 * Copyright 1988, 1989 Hans-J. Boehm, Alan J. Demers
 * Copyright (c) 1991-1994 by Xerox Corporation.  All rights reserved.
 * Copyright (c) 1996-1999 by Silicon Graphics.  All rights reserved.
 * Copyright (c) 1999-2004 Hewlett-Packard Development Company, L.P.
 *
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
 

# ifndef GC_PRIVATE_H
# define GC_PRIVATE_H

# include <stdlib.h>
# if !(defined( sony_news ) )
#   include <stddef.h>
# endif

#ifdef DGUX
#   include <sys/types.h>
#   include <sys/time.h>
#   include <sys/resource.h>
#endif /* DGUX */

#ifdef BSD_TIME
#   include <sys/types.h>
#   include <sys/time.h>
#   include <sys/resource.h>
#endif /* BSD_TIME */

#ifdef PARALLEL_MARK
#   define AO_REQUIRE_CAS
#endif

#ifndef _GC_H
#   include "../gc.h"
#endif

#ifndef GC_TINY_FL_H
#   include "../gc_tiny_fl.h"
#endif

#ifndef GC_MARK_H
#   include "../gc_mark.h"
#endif

typedef GC_word word;
typedef GC_signed_word signed_word;
typedef unsigned int unsigned32;

typedef int GC_bool;
# define TRUE 1
# define FALSE 0

typedef char * ptr_t;	/* A generic pointer to which we can add	*/
			/* byte displacements and which can be used	*/
			/* for address comparisons.			*/

# ifndef GCCONFIG_H
#   include "gcconfig.h"
# endif

# ifndef HEADERS_H
#   include "gc_hdrs.h"
# endif

#if __GNUC__ >= 3
# define EXPECT(expr, outcome) __builtin_expect(expr,outcome)
  /* Equivalent to (expr), but predict that usually (expr)==outcome. */
# define INLINE inline
#else
# define EXPECT(expr, outcome) (expr)
# define INLINE
#endif /* __GNUC__ */

# ifndef GC_LOCKS_H
#   include "gc_locks.h"
# endif

# ifdef STACK_GROWS_DOWN
#   define COOLER_THAN >
#   define HOTTER_THAN <
#   define MAKE_COOLER(x,y) if ((x)+(y) > (x)) {(x) += (y);} \
			    else {(x) = (ptr_t)ONES;}
#   define MAKE_HOTTER(x,y) (x) -= (y)
# else
#   define COOLER_THAN <
#   define HOTTER_THAN >
#   define MAKE_COOLER(x,y) if ((x)-(y) < (x)) {(x) -= (y);} else {(x) = 0;}
#   define MAKE_HOTTER(x,y) (x) += (y)
# endif

#if defined(AMIGA) && defined(__SASC)
#   define GC_FAR __far
#else
#   define GC_FAR
#endif


/*********************************/
/*                               */
/* Definitions for conservative  */
/* collector                     */
/*                               */
/*********************************/

/*********************************/
/*                               */
/* Easily changeable parameters  */
/*                               */
/*********************************/

/* #define STUBBORN_ALLOC */
		    /* Enable stubborm allocation, and thus a limited	*/
		    /* form of incremental collection w/o dirty bits.	*/

/* #define ALL_INTERIOR_POINTERS */
		    /* Forces all pointers into the interior of an 	*/
		    /* object to be considered valid.  Also causes the	*/
		    /* sizes of all objects to be inflated by at least 	*/
		    /* one byte.  This should suffice to guarantee	*/
		    /* that in the presence of a compiler that does	*/
		    /* not perform garbage-collector-unsafe		*/
		    /* optimizations, all portable, strictly ANSI	*/
		    /* conforming C programs should be safely usable	*/
		    /* with malloc replaced by GC_malloc and free	*/
		    /* calls removed.  There are several disadvantages: */
		    /* 1. There are probably no interesting, portable,	*/
		    /*    strictly ANSI	conforming C programs.		*/
		    /* 2. This option makes it hard for the collector	*/
		    /*    to allocate space that is not ``pointed to''  */
		    /*    by integers, etc.  Under SunOS 4.X with a 	*/
		    /*    statically linked libc, we empiricaly		*/
		    /*    observed that it would be difficult to 	*/
		    /*	  allocate individual objects larger than 100K.	*/
		    /* 	  Even if only smaller objects are allocated,	*/
		    /*    more swap space is likely to be needed.       */
		    /*    Fortunately, much of this will never be	*/
		    /*    touched.					*/
		    /* If you can easily avoid using this option, do.	*/
		    /* If not, try to keep individual objects small.	*/
		    /* This is now really controlled at startup,	*/
		    /* through GC_all_interior_pointers.		*/
		    

#define GC_INVOKE_FINALIZERS() GC_notify_or_invoke_finalizers()

#if !defined(DONT_ADD_BYTE_AT_END)
# define EXTRA_BYTES GC_all_interior_pointers
# define MAX_EXTRA_BYTES 1
#else
# define EXTRA_BYTES 0
# define MAX_EXTRA_BYTES 0
#endif


# ifndef LARGE_CONFIG
#   define MINHINCR 16	 /* Minimum heap increment, in blocks of HBLKSIZE  */
			 /* Must be multiple of largest page size.	   */
#   define MAXHINCR 2048 /* Maximum heap increment, in blocks              */
# else
#   define MINHINCR 64
#   define MAXHINCR 4096
# endif

# define TIME_LIMIT 50	   /* We try to keep pause times from exceeding	 */
			   /* this by much. In milliseconds.		 */

# define BL_LIMIT GC_black_list_spacing
			   /* If we need a block of N bytes, and we have */
			   /* a block of N + BL_LIMIT bytes available, 	 */
			   /* and N > BL_LIMIT,				 */
			   /* but all possible positions in it are 	 */
			   /* blacklisted, we just use it anyway (and	 */
			   /* print a warning, if warnings are enabled). */
			   /* This risks subsequently leaking the block	 */
			   /* due to a false reference.  But not using	 */
			   /* the block risks unreasonable immediate	 */
			   /* heap growth.				 */

/*********************************/
/*                               */
/* Stack saving for debugging	 */
/*                               */
/*********************************/

#ifdef NEED_CALLINFO
    struct callinfo {
	word ci_pc;  	/* Caller, not callee, pc	*/
#	if NARGS > 0
	    word ci_arg[NARGS];	/* bit-wise complement to avoid retention */
#	endif
#	if (NFRAMES * (NARGS + 1)) % 2 == 1
	    /* Likely alignment problem. */
	    word ci_dummy;
#	endif
    };
#endif

#ifdef SAVE_CALL_CHAIN

/* Fill in the pc and argument information for up to NFRAMES of my	*/
/* callers.  Ignore my frame and my callers frame.			*/
void GC_save_callers(struct callinfo info[NFRAMES]);
  
void GC_print_callers(struct callinfo info[NFRAMES]);

#endif


/*********************************/
/*                               */
/* OS interface routines	 */
/*                               */
/*********************************/

#ifdef BSD_TIME
#   undef CLOCK_TYPE
#   undef GET_TIME
#   undef MS_TIME_DIFF
#   define CLOCK_TYPE struct timeval
#   define GET_TIME(x) { struct rusage rusage; \
			 getrusage (RUSAGE_SELF,  &rusage); \
			 x = rusage.ru_utime; }
#   define MS_TIME_DIFF(a,b) ((double) (a.tv_sec - b.tv_sec) * 1000.0 \
                               + (double) (a.tv_usec - b.tv_usec) / 1000.0)
#else /* !BSD_TIME */
# if defined(MSWIN32) || defined(MSWINCE)
#   include <windows.h>
#   include <winbase.h>
#   define CLOCK_TYPE DWORD
#   define GET_TIME(x) x = GetTickCount()
#   define MS_TIME_DIFF(a,b) ((long)((a)-(b)))
# else /* !MSWIN32, !MSWINCE, !BSD_TIME */
#   include <time.h>
#   if !defined(__STDC__) && defined(SPARC) && defined(SUNOS4)
      clock_t clock();	/* Not in time.h, where it belongs	*/
#   endif
#   if defined(FREEBSD) && !defined(CLOCKS_PER_SEC)
#     include <machine/limits.h>
#     define CLOCKS_PER_SEC CLK_TCK
#   endif
#   if !defined(CLOCKS_PER_SEC)
#     define CLOCKS_PER_SEC 1000000
/*
 * This is technically a bug in the implementation.  ANSI requires that
 * CLOCKS_PER_SEC be defined.  But at least under SunOS4.1.1, it isn't.
 * Also note that the combination of ANSI C and POSIX is incredibly gross
 * here. The type clock_t is used by both clock() and times().  But on
 * some machines these use different notions of a clock tick,  CLOCKS_PER_SEC
 * seems to apply only to clock.  Hence we use it here.  On many machines,
 * including SunOS, clock actually uses units of microseconds (which are
 * not really clock ticks).
 */
#   endif
#   define CLOCK_TYPE clock_t
#   define GET_TIME(x) x = clock()
#   define MS_TIME_DIFF(a,b) ((unsigned long) \
		(1000.0*(double)((a)-(b))/(double)CLOCKS_PER_SEC))
# endif /* !MSWIN32 */
#endif /* !BSD_TIME */

/* We use bzero and bcopy internally.  They may not be available.	*/
# if defined(SPARC) && defined(SUNOS4)
#   define BCOPY_EXISTS
# endif
# if defined(M68K) && defined(AMIGA)
#   define BCOPY_EXISTS
# endif
# if defined(M68K) && defined(NEXT)
#   define BCOPY_EXISTS
# endif
# if defined(VAX)
#   define BCOPY_EXISTS
# endif
# if defined(AMIGA)
#   include <string.h>
#   define BCOPY_EXISTS
# endif
# if defined(DARWIN)
#   include <string.h>
#   define BCOPY_EXISTS
# endif

# ifndef BCOPY_EXISTS
#   include <string.h>
#   define BCOPY(x,y,n) memcpy(y, x, (size_t)(n))
#   define BZERO(x,n)  memset(x, 0, (size_t)(n))
# else
#   define BCOPY(x,y,n) bcopy((void *)(x),(void *)(y),(size_t)(n))
#   define BZERO(x,n) bzero((void *)(x),(size_t)(n))
# endif

/*
 * Stop and restart mutator threads.
 */
# ifdef PCR
#     include "th/PCR_ThCtl.h"
#     define STOP_WORLD() \
 	PCR_ThCtl_SetExclusiveMode(PCR_ThCtl_ExclusiveMode_stopNormal, \
 				   PCR_allSigsBlocked, \
 				   PCR_waitForever)
#     define START_WORLD() \
	PCR_ThCtl_SetExclusiveMode(PCR_ThCtl_ExclusiveMode_null, \
 				   PCR_allSigsBlocked, \
 				   PCR_waitForever);
# else
#   if defined(GC_SOLARIS_THREADS) || defined(GC_WIN32_THREADS) \
	|| defined(GC_PTHREADS)
      void GC_stop_world();
      void GC_start_world();
#     define STOP_WORLD() GC_stop_world()
#     define START_WORLD() GC_start_world()
#   else
#     define STOP_WORLD()
#     define START_WORLD()
#   endif
# endif

/* Abandon ship */
# ifdef PCR
#   define ABORT(s) PCR_Base_Panic(s)
# else
#   ifdef SMALL_CONFIG
#	define ABORT(msg) abort()
#   else
	GC_API void GC_abort(const char * msg);
#       define ABORT(msg) GC_abort(msg)
#   endif
# endif

/* Exit abnormally, but without making a mess (e.g. out of memory) */
# ifdef PCR
#   define EXIT() PCR_Base_Exit(1,PCR_waitForever)
# else
#   define EXIT() (void)exit(1)
# endif

/* Print warning message, e.g. almost out of memory.	*/
# define WARN(msg,arg) (*GC_current_warn_proc)("GC Warning: " msg, (GC_word)(arg))
extern GC_warn_proc GC_current_warn_proc;

/* Get environment entry */
#if !defined(NO_GETENV)
#   if defined(EMPTY_GETENV_RESULTS)
	/* Workaround for a reputed Wine bug.	*/
	static inline char * fixed_getenv(const char *name)
	{
	  char * tmp = getenv(name);
	  if (tmp == 0 || strlen(tmp) == 0)
	    return 0;
	  return tmp;
	}
#       define GETENV(name) fixed_getenv(name)
#   else
#       define GETENV(name) getenv(name)
#   endif
#else
#   define GETENV(name) 0
#endif

#if defined(DARWIN)
#	if defined(POWERPC)
#		if CPP_WORDSZ == 32
#                 define GC_THREAD_STATE_T ppc_thread_state_t
#		  define GC_MACH_THREAD_STATE PPC_THREAD_STATE
#		  define GC_MACH_THREAD_STATE_COUNT PPC_THREAD_STATE_COUNT
#		  define GC_MACH_HEADER mach_header
#		  define GC_MACH_SECTION section
#	        else
#                 define GC_THREAD_STATE_T ppc_thread_state64_t
#		  define GC_MACH_THREAD_STATE PPC_THREAD_STATE64
#		  define GC_MACH_THREAD_STATE_COUNT PPC_THREAD_STATE64_COUNT
#		  define GC_MACH_HEADER mach_header_64
#		  define GC_MACH_SECTION section_64
#		endif
#	elif defined(I386) || defined(X86_64)
#               if CPP_WORDSZ == 32
#		  define GC_THREAD_STATE_T x86_thread_state32_t
#		  define GC_MACH_THREAD_STATE x86_THREAD_STATE32
#		  define GC_MACH_THREAD_STATE_COUNT x86_THREAD_STATE32_COUNT
#		  define GC_MACH_HEADER mach_header
#		  define GC_MACH_SECTION section
#               else
#		  define GC_THREAD_STATE_T x86_thread_state64_t
#		  define GC_MACH_THREAD_STATE x86_THREAD_STATE64
#		  define GC_MACH_THREAD_STATE_COUNT x86_THREAD_STATE64_COUNT
#		  define GC_MACH_HEADER mach_header_64
#		  define GC_MACH_SECTION section_64
#               endif
#	else
#		error define GC_THREAD_STATE_T
#		define GC_MACH_THREAD_STATE MACHINE_THREAD_STATE
#		define GC_MACH_THREAD_STATE_COUNT MACHINE_THREAD_STATE_COUNT
#	endif
/* Try to work out the right way to access thread state structure members.
   The structure has changed its definition in different Darwin versions.
   This now defaults to the (older) names without __, thus hopefully,
   not breaking any existing Makefile.direct builds.  */
#       if defined (HAS_PPC_THREAD_STATE___R0) \
	  || defined (HAS_PPC_THREAD_STATE64___R0) \
	  || defined (HAS_X86_THREAD_STATE32___EAX) \
	  || defined (HAS_X86_THREAD_STATE64___RAX)
#         define THREAD_FLD(x) __ ## x
#       else
#         define THREAD_FLD(x) x
#       endif
#endif

/*********************************/
/*                               */
/* Word-size-dependent defines   */
/*                               */
/*********************************/

#if CPP_WORDSZ == 32
#  define WORDS_TO_BYTES(x)   ((x)<<2)
#  define BYTES_TO_WORDS(x)   ((x)>>2)
#  define LOGWL               ((word)5)    /* log[2] of CPP_WORDSZ */
#  define modWORDSZ(n) ((n) & 0x1f)        /* n mod size of word	    */
#  if ALIGNMENT != 4
#	define UNALIGNED
#  endif
#endif

#if CPP_WORDSZ == 64
#  define WORDS_TO_BYTES(x)   ((x)<<3)
#  define BYTES_TO_WORDS(x)   ((x)>>3)
#  define LOGWL               ((word)6)    /* log[2] of CPP_WORDSZ */
#  define modWORDSZ(n) ((n) & 0x3f)        /* n mod size of word	    */
#  if ALIGNMENT != 8
#	define UNALIGNED
#  endif
#endif

/* The first TINY_FREELISTS free lists correspond to the first	*/
/* TINY_FREELISTS multiples of GRANULE_BYTES, i.e. we keep 	*/
/* separate free lists for each multiple of GRANULE_BYTES	*/
/* up to (TINY_FREELISTS-1) * GRANULE_BYTES.  After that they	*/
/* may be spread out further. 					*/
#include "../gc_tiny_fl.h"
#define GRANULE_BYTES GC_GRANULE_BYTES
#define TINY_FREELISTS GC_TINY_FREELISTS

#define WORDSZ ((word)CPP_WORDSZ)
#define SIGNB  ((word)1 << (WORDSZ-1))
#define BYTES_PER_WORD      ((word)(sizeof (word)))
#define ONES                ((word)(signed_word)(-1))
#define divWORDSZ(n) ((n) >> LOGWL)	   /* divide n by size of word      */

#if GRANULE_BYTES == 8
# define BYTES_TO_GRANULES(n) ((n)>>3)
# define GRANULES_TO_BYTES(n) ((n)<<3)
# if CPP_WORDSZ == 64
#   define GRANULES_TO_WORDS(n) (n)
# elif CPP_WORDSZ == 32
#   define GRANULES_TO_WORDS(n) ((n)<<1)
# else
#   define GRANULES_TO_WORDS(n) BYTES_TO_WORDS(GRANULES_TO_BYTES(n))
# endif
#elif GRANULE_BYTES == 16
# define BYTES_TO_GRANULES(n) ((n)>>4)
# define GRANULES_TO_BYTES(n) ((n)<<4)
# if CPP_WORDSZ == 64
#   define GRANULES_TO_WORDS(n) ((n)<<1)
# elif CPP_WORDSZ == 32
#   define GRANULES_TO_WORDS(n) ((n)<<2)
# else
#   define GRANULES_TO_WORDS(n) BYTES_TO_WORDS(GRANULES_TO_BYTES(n))
# endif
#else
# error Bad GRANULE_BYTES value
#endif

/*********************/
/*                   */
/*  Size Parameters  */
/*                   */
/*********************/

/*  heap block size, bytes. Should be power of 2 */

#ifndef HBLKSIZE
# ifdef SMALL_CONFIG
#   define CPP_LOG_HBLKSIZE 10
# else
#   if (CPP_WORDSZ == 32) || (defined(HPUX) && defined(HP_PA))
      /* HPUX/PA seems to use 4K pages with the 64 bit ABI */
#     define CPP_LOG_HBLKSIZE 12
#   else
#     define CPP_LOG_HBLKSIZE 13
#   endif
# endif
#else
# if HBLKSIZE == 512
#   define CPP_LOG_HBLKSIZE 9
# endif
# if HBLKSIZE == 1024
#   define CPP_LOG_HBLKSIZE 10
# endif
# if HBLKSIZE == 2048
#   define CPP_LOG_HBLKSIZE 11
# endif
# if HBLKSIZE == 4096
#   define CPP_LOG_HBLKSIZE 12
# endif
# if HBLKSIZE == 8192
#   define CPP_LOG_HBLKSIZE 13
# endif
# if HBLKSIZE == 16384
#   define CPP_LOG_HBLKSIZE 14
# endif
# ifndef CPP_LOG_HBLKSIZE
    --> fix HBLKSIZE
# endif
# undef HBLKSIZE
#endif
# define CPP_HBLKSIZE (1 << CPP_LOG_HBLKSIZE)
# define LOG_HBLKSIZE   ((size_t)CPP_LOG_HBLKSIZE)
# define HBLKSIZE ((size_t)CPP_HBLKSIZE)


/*  max size objects supported by freelist (larger objects are	*/
/*  allocated directly with allchblk(), by rounding to the next */
/*  multiple of HBLKSIZE.					*/

#define CPP_MAXOBJBYTES (CPP_HBLKSIZE/2)
#define MAXOBJBYTES ((size_t)CPP_MAXOBJBYTES)
#define CPP_MAXOBJWORDS BYTES_TO_WORDS(CPP_MAXOBJBYTES)
#define MAXOBJWORDS ((size_t)CPP_MAXOBJWORDS)
#define CPP_MAXOBJGRANULES BYTES_TO_GRANULES(CPP_MAXOBJBYTES)
#define MAXOBJGRANULES ((size_t)CPP_MAXOBJGRANULES)
		
# define divHBLKSZ(n) ((n) >> LOG_HBLKSIZE)

# define HBLK_PTR_DIFF(p,q) divHBLKSZ((ptr_t)p - (ptr_t)q)
	/* Equivalent to subtracting 2 hblk pointers.	*/
	/* We do it this way because a compiler should	*/
	/* find it hard to use an integer division	*/
	/* instead of a shift.  The bundled SunOS 4.1	*/
	/* o.w. sometimes pessimizes the subtraction to	*/
	/* involve a call to .div.			*/
 
# define modHBLKSZ(n) ((n) & (HBLKSIZE-1))
 
# define HBLKPTR(objptr) ((struct hblk *)(((word) (objptr)) & ~(HBLKSIZE-1)))

# define HBLKDISPL(objptr) (((size_t) (objptr)) & (HBLKSIZE-1))

/* Round up byte allocation requests to integral number of words, etc. */
# define ROUNDED_UP_WORDS(n) \
	BYTES_TO_WORDS((n) + (WORDS_TO_BYTES(1) - 1 + EXTRA_BYTES))
# define ROUNDED_UP_GRANULES(n) \
	BYTES_TO_GRANULES((n) + (GRANULE_BYTES - 1 + EXTRA_BYTES))
# if MAX_EXTRA_BYTES == 0
#  define SMALL_OBJ(bytes) EXPECT((bytes) <= (MAXOBJBYTES), 1)
# else
#  define SMALL_OBJ(bytes) \
	    (EXPECT((bytes) <= (MAXOBJBYTES - MAX_EXTRA_BYTES), 1) || \
	     (bytes) <= (MAXOBJBYTES - EXTRA_BYTES))
    	/* This really just tests bytes <= MAXOBJBYTES - EXTRA_BYTES.	*/
    	/* But we try to avoid looking up EXTRA_BYTES.			*/
# endif
# define ADD_SLOP(bytes) ((bytes) + EXTRA_BYTES)
# ifndef MIN_WORDS
#  define MIN_WORDS 2	/* FIXME: obsolete */
# endif


/*
 * Hash table representation of sets of pages.
 * Implements a map from aligned HBLKSIZE chunks of the address space to one
 * bit each.
 * This assumes it is OK to spuriously set bits, e.g. because multiple
 * addresses are represented by a single location.
 * Used by black-listing code, and perhaps by dirty bit maintenance code.
 */
 
# ifdef LARGE_CONFIG
#   define LOG_PHT_ENTRIES  20  /* Collisions likely at 1M blocks,	*/
				/* which is >= 4GB.  Each table takes	*/
				/* 128KB, some of which may never be	*/
				/* touched.				*/
# else
#   ifdef SMALL_CONFIG
#     define LOG_PHT_ENTRIES  14 /* Collisions are likely if heap grows	*/
				 /* to more than 16K hblks = 64MB.	*/
				 /* Each hash table occupies 2K bytes.   */
#   else /* default "medium" configuration */
#     define LOG_PHT_ENTRIES  16 /* Collisions are likely if heap grows	*/
				 /* to more than 64K hblks >= 256MB.	*/
				 /* Each hash table occupies 8K bytes.  */
				 /* Even for somewhat smaller heaps, 	*/
				 /* say half that, collisions may be an	*/
				 /* issue because we blacklist 		*/
				 /* addresses outside the heap.		*/
#   endif
# endif
# define PHT_ENTRIES ((word)1 << LOG_PHT_ENTRIES)
# define PHT_SIZE (PHT_ENTRIES >> LOGWL)
typedef word page_hash_table[PHT_SIZE];

# define PHT_HASH(addr) ((((word)(addr)) >> LOG_HBLKSIZE) & (PHT_ENTRIES - 1))

# define get_pht_entry_from_index(bl, index) \
		(((bl)[divWORDSZ(index)] >> modWORDSZ(index)) & 1)
# define set_pht_entry_from_index(bl, index) \
		(bl)[divWORDSZ(index)] |= (word)1 << modWORDSZ(index)
# define clear_pht_entry_from_index(bl, index) \
		(bl)[divWORDSZ(index)] &= ~((word)1 << modWORDSZ(index))
/* And a dumb but thread-safe version of set_pht_entry_from_index.	*/
/* This sets (many) extra bits.						*/
# define set_pht_entry_from_index_safe(bl, index) \
		(bl)[divWORDSZ(index)] = ONES
	


/********************************************/
/*                                          */
/*    H e a p   B l o c k s                 */
/*                                          */
/********************************************/

/*  heap block header */
#define HBLKMASK   (HBLKSIZE-1)

#define MARK_BITS_PER_HBLK (HBLKSIZE/GRANULE_BYTES)
	   /* upper bound                                    */
	   /* We allocate 1 bit per allocation granule.	     */
	   /* If MARK_BIT_PER_GRANULE is defined, we use     */
	   /* every nth bit, where n is the number of 	     */
	   /* allocation granules per object.  If	     */
	   /* MARK_BIT_PER_OBJ is defined, we only use the   */
   	   /* initial group of mark bits, and it is safe     */
	   /* to allocate smaller header for large objects.  */

# ifdef USE_MARK_BYTES
#   define MARK_BITS_SZ (MARK_BITS_PER_HBLK + 1)
	/* Unlike the other case, this is in units of bytes.		*/
	/* Since we force doubleword alignment, we need at most one	*/
	/* mark bit per 2 words.  But we do allocate and set one	*/
	/* extra mark bit to avoid an explicit check for the 		*/
	/* partial object at the end of each block.			*/
# else
#   define MARK_BITS_SZ (MARK_BITS_PER_HBLK/CPP_WORDSZ + 1)
# endif

#ifdef PARALLEL_MARK
# include <atomic_ops.h>
  typedef AO_t counter_t;
#else
  typedef size_t counter_t;
#endif

/* We maintain layout maps for heap blocks containing objects of a given */
/* size.  Each entry in this map describes a byte offset and has the	 */
/* following type.							 */
struct hblkhdr {
    struct hblk * hb_next; 	/* Link field for hblk free list	 */
    				/* and for lists of chunks waiting to be */
    				/* reclaimed.				 */
    struct hblk * hb_prev;	/* Backwards link for free list.	*/
    struct hblk * hb_block;	/* The corresponding block.		*/
    unsigned char hb_obj_kind;
    			 /* Kind of objects in the block.  Each kind 	*/
    			 /* identifies a mark procedure and a set of 	*/
    			 /* list headers.  Sometimes called regions.	*/
    unsigned char hb_flags;
#	define IGNORE_OFF_PAGE	1	/* Ignore pointers that do not	*/
					/* point to the first page of 	*/
					/* this object.			*/
#	define WAS_UNMAPPED 2	/* This is a free block, which has	*/
				/* been unmapped from the address 	*/
				/* space.				*/
				/* GC_remap must be invoked on it	*/
				/* before it can be reallocated.	*/
				/* Only set with USE_MUNMAP.		*/
#	define FREE_BLK 4	/* Block is free, i.e. not in use.	*/
    unsigned short hb_last_reclaimed;
    				/* Value of GC_gc_no when block was	*/
    				/* last allocated or swept. May wrap.   */
				/* For a free block, this is maintained */
				/* only for USE_MUNMAP, and indicates	*/
				/* when the header was allocated, or	*/
				/* when the size of the block last	*/
				/* changed.				*/
    size_t hb_sz;  /* If in use, size in bytes, of objects in the block. */
		   /* if free, the size in bytes of the whole block      */
    word hb_descr;   		/* object descriptor for marking.  See	*/
    				/* mark.h.				*/
#   ifdef MARK_BIT_PER_OBJ
      unsigned32 hb_inv_sz;	/* A good upper bound for 2**32/hb_sz.	*/
    				/* For large objects, we use		*/
    				/* LARGE_INV_SZ.			*/
#     define LARGE_INV_SZ (1 << 16)
#   else
      unsigned char hb_large_block;
      short * hb_map;		/* Essentially a table of remainders	*/
      				/* mod BYTES_TO_GRANULES(hb_sz), except	*/
      				/* for large blocks.  See GC_obj_map.	*/
#   endif
    counter_t hb_n_marks;	/* Number of set mark bits, excluding 	*/
    				/* the one always set at the end.	*/
    				/* Currently it is concurrently 	*/
    				/* updated and hence only approximate.  */
    				/* But a zero value does guarantee that	*/
    				/* the block contains no marked		*/
    				/* objects.				*/
    				/* Ensuring this property means that we	*/
    				/* never decrement it to zero during a	*/
    				/* collection, and hence the count may 	*/
    				/* be one too high.  Due to concurrent	*/
    				/* updates, an arbitrary number of	*/
    				/* increments, but not all of them (!)	*/
    				/* may be lost, hence it may in theory	*/
    				/* be much too low.			*/
    				/* The count may also be too high if	*/
    				/* multiple mark threads mark the	*/
    				/* same object due to a race.		*/
    				/* Without parallel marking, the count	*/
    				/* is accurate.				*/
#   ifdef USE_MARK_BYTES
      union {
        char _hb_marks[MARK_BITS_SZ];
			    /* The i'th byte is 1 if the object 	*/
			    /* starting at granule i or object i is	*/
			    /* marked, 0 o.w.				*/
			    /* The mark bit for the "one past the	*/
			    /* end" object is always set to avoid a 	*/
			    /* special case test in the marker.		*/
	word dummy;	/* Force word alignment of mark bytes. */
      } _mark_byte_union;
#     define hb_marks _mark_byte_union._hb_marks
#   else
      word hb_marks[MARK_BITS_SZ];
#   endif /* !USE_MARK_BYTES */
};

# define ANY_INDEX 23	/* "Random" mark bit index for assertions */

/*  heap block body */

# define HBLK_WORDS (HBLKSIZE/sizeof(word))
# define HBLK_GRANULES (HBLKSIZE/GRANULE_BYTES)

/* The number of objects in a block dedicated to a certain size.	*/
/* may erroneously yield zero (instead of one) for large objects.	*/
# define HBLK_OBJS(sz_in_bytes) (HBLKSIZE/(sz_in_bytes))

struct hblk {
    char hb_body[HBLKSIZE];
};

# define HBLK_IS_FREE(hdr) (((hdr) -> hb_flags & FREE_BLK) != 0)

# define OBJ_SZ_TO_BLOCKS(sz) divHBLKSZ(sz + HBLKSIZE-1)
    /* Size of block (in units of HBLKSIZE) needed to hold objects of	*/
    /* given sz (in bytes).						*/

/* Object free list link */
# define obj_link(p) (*(void  **)(p))

# define LOG_MAX_MARK_PROCS 6
# define MAX_MARK_PROCS (1 << LOG_MAX_MARK_PROCS)

/* Root sets.  Logically private to mark_rts.c.  But we don't want the	*/
/* tables scanned, so we put them here.					*/
/* MAX_ROOT_SETS is the maximum number of ranges that can be 	*/
/* registered as static roots. 					*/
# ifdef LARGE_CONFIG
#   define MAX_ROOT_SETS 4096
# else
    /* GCJ LOCAL: MAX_ROOT_SETS increased to permit more shared */
    /* libraries to be loaded.                                  */ 
#   define MAX_ROOT_SETS 1024
# endif

# define MAX_EXCLUSIONS (MAX_ROOT_SETS/4)
/* Maximum number of segments that can be excluded from root sets.	*/

/*
 * Data structure for excluded static roots.
 */
struct exclusion {
    ptr_t e_start;
    ptr_t e_end;
};

/* Data structure for list of root sets.				*/
/* We keep a hash table, so that we can filter out duplicate additions.	*/
/* Under Win32, we need to do a better job of filtering overlaps, so	*/
/* we resort to sequential search, and pay the price.			*/
struct roots {
	ptr_t r_start;
	ptr_t r_end;
#	if !defined(MSWIN32) && !defined(MSWINCE)
	  struct roots * r_next;
#	endif
	GC_bool r_tmp;
	  	/* Delete before registering new dynamic libraries */
};

#if !defined(MSWIN32) && !defined(MSWINCE)
    /* Size of hash table index to roots.	*/
#   define LOG_RT_SIZE 6
#   define RT_SIZE (1 << LOG_RT_SIZE) /* Power of 2, may be != MAX_ROOT_SETS */
#endif

/* Lists of all heap blocks and free lists	*/
/* as well as other random data structures	*/
/* that should not be scanned by the		*/
/* collector.					*/
/* These are grouped together in a struct	*/
/* so that they can be easily skipped by the	*/
/* GC_mark routine.				*/
/* The ordering is weird to make GC_malloc	*/
/* faster by keeping the important fields	*/
/* sufficiently close together that a		*/
/* single load of a base register will do.	*/
/* Scalars that could easily appear to		*/
/* be pointers are also put here.		*/
/* The main fields should precede any 		*/
/* conditionally included fields, so that	*/
/* gc_inl.h will work even if a different set	*/
/* of macros is defined when the client is	*/
/* compiled.					*/

struct _GC_arrays {
  word _heapsize;		/* Heap size in bytes.			*/
  word _max_heapsize;
  word _requested_heapsize;	/* Heap size due to explicit expansion */
  ptr_t _last_heap_addr;
  ptr_t _prev_heap_addr;
  word _large_free_bytes;
	/* Total bytes contained in blocks on large object free */
	/* list.						*/
  word _large_allocd_bytes;
  	/* Total number of bytes in allocated large objects blocks.	*/
  	/* For the purposes of this counter and the next one only, a 	*/
  	/* large object is one that occupies a block of at least	*/
  	/* 2*HBLKSIZE.							*/
  word _max_large_allocd_bytes;
  	/* Maximum number of bytes that were ever allocated in		*/
  	/* large object blocks.  This is used to help decide when it	*/
  	/* is safe to split up a large block.				*/
  word _bytes_allocd_before_gc;
		/* Number of words allocated before this	*/
		/* collection cycle.				*/
# ifndef SEPARATE_GLOBALS
    word _bytes_allocd;
  	/* Number of words allocated during this collection cycle */
# endif
  word _bytes_finalized;
  	/* Approximate number of bytes in objects (and headers)	*/
  	/* That became ready for finalization in the last 	*/
  	/* collection.						*/
  word _non_gc_bytes_at_gc;
  	/* Number of explicitly managed bytes of storage 	*/
  	/* at last collection.					*/
  word _bytes_freed;
  	/* Number of explicitly deallocated bytes of memory	*/
  	/* since last collection.				*/
  word _finalizer_bytes_freed;
  	/* Bytes of memory explicitly deallocated while 	*/
  	/* finalizers were running.  Used to approximate mem.	*/
  	/* explicitly deallocated by finalizers.		*/
  ptr_t _scratch_end_ptr;
  ptr_t _scratch_last_end_ptr;
	/* Used by headers.c, and can easily appear to point to	*/
	/* heap.						*/
  GC_mark_proc _mark_procs[MAX_MARK_PROCS];
  	/* Table of user-defined mark procedures.  There is	*/
	/* a small number of these, which can be referenced	*/
	/* by DS_PROC mark descriptors.  See gc_mark.h.		*/

# ifndef SEPARATE_GLOBALS
    void *_objfreelist[MAXOBJGRANULES+1];
			  /* free list for objects */
    void *_aobjfreelist[MAXOBJGRANULES+1];
			  /* free list for atomic objs 	*/
# endif

  void *_uobjfreelist[MAXOBJGRANULES+1];
			  /* uncollectable but traced objs 	*/
			  /* objects on this and auobjfreelist  */
			  /* are always marked, except during   */
			  /* garbage collections.		*/
# ifdef ATOMIC_UNCOLLECTABLE
    void *_auobjfreelist[MAXOBJGRANULES+1];
# endif
			  /* uncollectable but traced objs 	*/

    word _composite_in_use;
   		/* Number of words in accessible composite	*/
		/* objects.					*/
    word _atomic_in_use;
   		/* Number of words in accessible atomic		*/
		/* objects.					*/
# ifdef USE_MUNMAP
    word _unmapped_bytes;
# endif

    size_t _size_map[MAXOBJBYTES+1];
    	/* Number of words to allocate for a given allocation request in */
    	/* bytes.							 */

# ifdef STUBBORN_ALLOC
    ptr_t _sobjfreelist[MAXOBJGRANULES+1];
# endif
  			  /* free list for immutable objects	*/
# ifdef MARK_BIT_PER_GRANULE
    short * _obj_map[MAXOBJGRANULES+1];
                       /* If not NIL, then a pointer to a map of valid  */
    		       /* object addresses.				*/
		       /* _obj_map[sz_in_granules][i] is 		*/
  		       /* i % sz_in_granules.				*/
  		       /* This is now used purely to replace a 		*/
  		       /* division in the marker by a table lookup.	*/
    		       /* _obj_map[0] is used for large objects and	*/
    		       /* contains all nonzero entries.  This gets us	*/
    		       /* out of the marker fast path without an extra 	*/
    		       /* test.						*/
#   define MAP_LEN BYTES_TO_GRANULES(HBLKSIZE)
# endif
#   define VALID_OFFSET_SZ HBLKSIZE
  char _valid_offsets[VALID_OFFSET_SZ];
				/* GC_valid_offsets[i] == TRUE ==> i 	*/
				/* is registered as a displacement.	*/
  char _modws_valid_offsets[sizeof(word)];
				/* GC_valid_offsets[i] ==>		  */
				/* GC_modws_valid_offsets[i%sizeof(word)] */
# ifdef STUBBORN_ALLOC
    page_hash_table _changed_pages;
        /* Stubborn object pages that were changes since last call to	*/
	/* GC_read_changed.						*/
    page_hash_table _prev_changed_pages;
        /* Stubborn object pages that were changes before last call to	*/
	/* GC_read_changed.						*/
# endif
# if defined(PROC_VDB) || defined(MPROTECT_VDB) || \
     defined(GWW_VDB) || defined(MANUAL_VDB)
    page_hash_table _grungy_pages; /* Pages that were dirty at last 	   */
				     /* GC_read_dirty.			   */
# endif
# if defined(MPROTECT_VDB) || defined(MANUAL_VDB)
    volatile page_hash_table _dirty_pages;	
			/* Pages dirtied since last GC_read_dirty. */
# endif
# if defined(PROC_VDB) || defined(GWW_VDB)
    page_hash_table _written_pages;	/* Pages ever dirtied	*/
# endif
# ifdef LARGE_CONFIG
#   if CPP_WORDSZ > 32
#     define MAX_HEAP_SECTS 4096 	/* overflows at roughly 64 GB	   */
#   else
#     define MAX_HEAP_SECTS 768		/* Separately added heap sections. */
#   endif
# else
#   ifdef SMALL_CONFIG
#     define MAX_HEAP_SECTS 128		/* Roughly 256MB (128*2048*1K)	*/
#   else
#     define MAX_HEAP_SECTS 384		/* Roughly 3GB			*/
#   endif
# endif
  struct HeapSect {
      ptr_t hs_start; size_t hs_bytes;
  } _heap_sects[MAX_HEAP_SECTS];
# if defined(MSWIN32) || defined(MSWINCE)
    ptr_t _heap_bases[MAX_HEAP_SECTS];
    		/* Start address of memory regions obtained from kernel. */
# endif
# ifdef MSWINCE
    word _heap_lengths[MAX_HEAP_SECTS];
    		/* Commited lengths of memory regions obtained from kernel. */
# endif
  struct roots _static_roots[MAX_ROOT_SETS];
# if !defined(MSWIN32) && !defined(MSWINCE)
    struct roots * _root_index[RT_SIZE];
# endif
  struct exclusion _excl_table[MAX_EXCLUSIONS];
  /* Block header index; see gc_headers.h */
  bottom_index * _all_nils;
  bottom_index * _top_index [TOP_SZ];
#ifdef ENABLE_TRACE
  ptr_t _trace_addr;
#endif
#ifdef SAVE_CALL_CHAIN
  struct callinfo _last_stack[NFRAMES];	/* Stack at last garbage collection.*/
  					/* Useful for debugging	mysterious  */
  					/* object disappearances.	    */
  					/* In the multithreaded case, we    */
  					/* currently only save the calling  */
  					/* stack.			    */
#endif
};

GC_API GC_FAR struct _GC_arrays GC_arrays; 

# ifndef SEPARATE_GLOBALS
#   define GC_objfreelist GC_arrays._objfreelist
#   define GC_aobjfreelist GC_arrays._aobjfreelist
#   define GC_bytes_allocd GC_arrays._bytes_allocd
# endif
# define GC_uobjfreelist GC_arrays._uobjfreelist
# ifdef ATOMIC_UNCOLLECTABLE
#   define GC_auobjfreelist GC_arrays._auobjfreelist
# endif
# define GC_sobjfreelist GC_arrays._sobjfreelist
# define GC_valid_offsets GC_arrays._valid_offsets
# define GC_modws_valid_offsets GC_arrays._modws_valid_offsets
# ifdef STUBBORN_ALLOC
#    define GC_changed_pages GC_arrays._changed_pages
#    define GC_prev_changed_pages GC_arrays._prev_changed_pages
# endif
# ifdef MARK_BIT_PER_GRANULE
#   define GC_obj_map GC_arrays._obj_map
# endif
# define GC_last_heap_addr GC_arrays._last_heap_addr
# define GC_prev_heap_addr GC_arrays._prev_heap_addr
# define GC_large_free_bytes GC_arrays._large_free_bytes
# define GC_large_allocd_bytes GC_arrays._large_allocd_bytes
# define GC_max_large_allocd_bytes GC_arrays._max_large_allocd_bytes
# define GC_bytes_finalized GC_arrays._bytes_finalized
# define GC_non_gc_bytes_at_gc GC_arrays._non_gc_bytes_at_gc
# define GC_bytes_freed GC_arrays._bytes_freed
# define GC_finalizer_bytes_freed GC_arrays._finalizer_bytes_freed
# define GC_scratch_end_ptr GC_arrays._scratch_end_ptr
# define GC_scratch_last_end_ptr GC_arrays._scratch_last_end_ptr
# define GC_mark_procs GC_arrays._mark_procs
# define GC_heapsize GC_arrays._heapsize
# define GC_max_heapsize GC_arrays._max_heapsize
# define GC_requested_heapsize GC_arrays._requested_heapsize
# define GC_bytes_allocd_before_gc GC_arrays._bytes_allocd_before_gc
# define GC_heap_sects GC_arrays._heap_sects
# define GC_last_stack GC_arrays._last_stack
#ifdef ENABLE_TRACE
#define GC_trace_addr GC_arrays._trace_addr
#endif
# ifdef USE_MUNMAP
#   define GC_unmapped_bytes GC_arrays._unmapped_bytes
# endif
# if defined(MSWIN32) || defined(MSWINCE)
#   define GC_heap_bases GC_arrays._heap_bases
# endif
# ifdef MSWINCE
#   define GC_heap_lengths GC_arrays._heap_lengths
# endif
# define GC_static_roots GC_arrays._static_roots
# define GC_root_index GC_arrays._root_index
# define GC_excl_table GC_arrays._excl_table
# define GC_all_nils GC_arrays._all_nils
# define GC_top_index GC_arrays._top_index
# if defined(PROC_VDB) || defined(MPROTECT_VDB) || \
     defined(GWW_VDB) || defined(MANUAL_VDB)
#   define GC_grungy_pages GC_arrays._grungy_pages
# endif
# if defined(MPROTECT_VDB) || defined(MANUAL_VDB)
#   define GC_dirty_pages GC_arrays._dirty_pages
# endif
# if defined(PROC_VDB) || defined(GWW_VDB)
#   define GC_written_pages GC_arrays._written_pages
# endif
# define GC_composite_in_use GC_arrays._composite_in_use
# define GC_atomic_in_use GC_arrays._atomic_in_use
# define GC_size_map GC_arrays._size_map

# define beginGC_arrays ((ptr_t)(&GC_arrays))
# define endGC_arrays (((ptr_t)(&GC_arrays)) + (sizeof GC_arrays))

#define USED_HEAP_SIZE (GC_heapsize - GC_large_free_bytes)

/* Object kinds: */
# define MAXOBJKINDS 16

extern struct obj_kind {
   void **ok_freelist;	/* Array of free listheaders for this kind of object */
   			/* Point either to GC_arrays or to storage allocated */
   			/* with GC_scratch_alloc.			     */
   struct hblk **ok_reclaim_list;
   			/* List headers for lists of blocks waiting to be */
   			/* swept.					  */
   			/* Indexed by object size in granules.		  */
   word ok_descriptor;  /* Descriptor template for objects in this	*/
   			/* block.					*/
   GC_bool ok_relocate_descr;
   			/* Add object size in bytes to descriptor 	*/
   			/* template to obtain descriptor.  Otherwise	*/
   			/* template is used as is.			*/
   GC_bool ok_init;   /* Clear objects before putting them on the free list. */
} GC_obj_kinds[MAXOBJKINDS];

# define beginGC_obj_kinds ((ptr_t)(&GC_obj_kinds))
# define endGC_obj_kinds (beginGC_obj_kinds + (sizeof GC_obj_kinds))

/* Variables that used to be in GC_arrays, but need to be accessed by 	*/
/* inline allocation code.  If they were in GC_arrays, the inlined 	*/
/* allocation code would include GC_arrays offsets (as it did), which	*/
/* introduce maintenance problems.					*/

#ifdef SEPARATE_GLOBALS
  word GC_bytes_allocd;
  	/* Number of words allocated during this collection cycle */
  ptr_t GC_objfreelist[MAXOBJGRANULES+1];
			  /* free list for NORMAL objects */
# define beginGC_objfreelist ((ptr_t)(&GC_objfreelist))
# define endGC_objfreelist (beginGC_objfreelist + sizeof(GC_objfreelist))

  ptr_t GC_aobjfreelist[MAXOBJGRANULES+1];
			  /* free list for atomic (PTRFREE) objs 	*/
# define beginGC_aobjfreelist ((ptr_t)(&GC_aobjfreelist))
# define endGC_aobjfreelist (beginGC_aobjfreelist + sizeof(GC_aobjfreelist))
#endif

/* Predefined kinds: */
# define PTRFREE 0
# define NORMAL  1
# define UNCOLLECTABLE 2
# ifdef ATOMIC_UNCOLLECTABLE
#   define AUNCOLLECTABLE 3
#   define STUBBORN 4
#   define IS_UNCOLLECTABLE(k) (((k) & ~1) == UNCOLLECTABLE)
# else
#   define STUBBORN 3
#   define IS_UNCOLLECTABLE(k) ((k) == UNCOLLECTABLE)
# endif

extern unsigned GC_n_kinds;

GC_API word GC_fo_entries;

extern word GC_n_heap_sects;	/* Number of separately added heap	*/
				/* sections.				*/

extern word GC_page_size;

# if defined(MSWIN32) || defined(MSWINCE)
  struct _SYSTEM_INFO;
  extern struct _SYSTEM_INFO GC_sysinfo;
  extern word GC_n_heap_bases;	/* See GC_heap_bases.	*/
# endif

extern word GC_total_stack_black_listed;
			/* Number of bytes on stack blacklist. 	*/

extern word GC_black_list_spacing;
			/* Average number of bytes between blacklisted	*/
			/* blocks. Approximate.				*/
			/* Counts only blocks that are 			*/
			/* "stack-blacklisted", i.e. that are 		*/
			/* problematic in the interior of an object.	*/

extern struct hblk * GC_hblkfreelist[];
				/* List of completely empty heap blocks	*/
				/* Linked through hb_next field of 	*/
				/* header structure associated with	*/
				/* block.				*/

extern GC_bool GC_objects_are_marked;	/* There are marked objects in  */
					/* the heap.			*/

#ifndef SMALL_CONFIG
  extern GC_bool GC_incremental;
			/* Using incremental/generational collection. */
# define TRUE_INCREMENTAL \
	(GC_incremental && GC_time_limit != GC_TIME_UNLIMITED)
	/* True incremental, not just generational, mode */
#else
# define GC_incremental FALSE
			/* Hopefully allow optimizer to remove some code. */
# define TRUE_INCREMENTAL FALSE
#endif

extern GC_bool GC_dirty_maintained;
				/* Dirty bits are being maintained, 	*/
				/* either for incremental collection,	*/
				/* or to limit the root set.		*/

extern word GC_root_size;	/* Total size of registered root sections */

extern GC_bool GC_debugging_started;	/* GC_debug_malloc has been called. */ 

extern long GC_large_alloc_warn_interval;
	/* Interval between unsuppressed warnings.	*/

extern long GC_large_alloc_warn_suppressed;
	/* Number of warnings suppressed so far.	*/

#ifdef THREADS
  extern GC_bool GC_world_stopped;
#endif

/* Operations */
# ifndef abs
#   define abs(x)  ((x) < 0? (-(x)) : (x))
# endif


/*  Marks are in a reserved area in                          */
/*  each heap block.  Each word has one mark bit associated  */
/*  with it. Only those corresponding to the beginning of an */
/*  object are used.                                         */

/* Set mark bit correctly, even if mark bits may be concurrently 	*/
/* accessed.								*/
#ifdef PARALLEL_MARK
# define OR_WORD(addr, bits) \
	{ AO_or((volatile AO_t *)(addr), (AO_t)bits); }
#else
# define OR_WORD(addr, bits) *(addr) |= (bits)
#endif

/* Mark bit operations */

/*
 * Retrieve, set, clear the nth mark bit in a given heap block.
 *
 * (Recall that bit n corresponds to nth object or allocation granule
 * relative to the beginning of the block, including unused words)
 */

#ifdef USE_MARK_BYTES
# define mark_bit_from_hdr(hhdr,n) ((hhdr)->hb_marks[n])
# define set_mark_bit_from_hdr(hhdr,n) ((hhdr)->hb_marks[n]) = 1
# define clear_mark_bit_from_hdr(hhdr,n) ((hhdr)->hb_marks[n]) = 0
#else /* !USE_MARK_BYTES */
# define mark_bit_from_hdr(hhdr,n) (((hhdr)->hb_marks[divWORDSZ(n)] \
			    >> (modWORDSZ(n))) & (word)1)
# define set_mark_bit_from_hdr(hhdr,n) \
			    OR_WORD((hhdr)->hb_marks+divWORDSZ(n), \
				    (word)1 << modWORDSZ(n))
# define clear_mark_bit_from_hdr(hhdr,n) (hhdr)->hb_marks[divWORDSZ(n)] \
				&= ~((word)1 << modWORDSZ(n))
#endif /* !USE_MARK_BYTES */

#ifdef MARK_BIT_PER_OBJ
#  define MARK_BIT_NO(offset, sz) (((unsigned)(offset))/(sz))
	/* Get the mark bit index corresponding to the given byte	*/
	/* offset and size (in bytes). 				        */
#  define MARK_BIT_OFFSET(sz) 1
	/* Spacing between useful mark bits.				*/
#  define IF_PER_OBJ(x) x
#  define FINAL_MARK_BIT(sz) ((sz) > MAXOBJBYTES? 1 : HBLK_OBJS(sz))
	/* Position of final, always set, mark bit.			*/
#else /* MARK_BIT_PER_GRANULE */
#  define MARK_BIT_NO(offset, sz) BYTES_TO_GRANULES((unsigned)(offset))
#  define MARK_BIT_OFFSET(sz) BYTES_TO_GRANULES(sz)
#  define IF_PER_OBJ(x)
#  define FINAL_MARK_BIT(sz) \
	((sz) > MAXOBJBYTES? MARK_BITS_PER_HBLK \
	 		: BYTES_TO_GRANULES(sz * HBLK_OBJS(sz)))
#endif

/* Important internal collector routines */

ptr_t GC_approx_sp(void);
  
GC_bool GC_should_collect(void);
  
void GC_apply_to_all_blocks(void (*fn) (struct hblk *h, word client_data),
    			    word client_data);
  			/* Invoke fn(hbp, client_data) for each 	*/
  			/* allocated heap block.			*/
struct hblk * GC_next_used_block(struct hblk * h);
  			/* Return first in-use block >= h	*/
struct hblk * GC_prev_block(struct hblk * h);
  			/* Return last block <= h.  Returned block	*/
  			/* is managed by GC, but may or may not be in	*/
			/* use.						*/
void GC_mark_init(void);
void GC_clear_marks(void);	/* Clear mark bits for all heap objects. */
void GC_invalidate_mark_state(void);
					/* Tell the marker that	marked 	   */
  					/* objects may point to	unmarked   */
  					/* ones, and roots may point to	   */
  					/* unmarked objects.		   */
  					/* Reset mark stack.		   */
GC_bool GC_mark_stack_empty(void);
GC_bool GC_mark_some(ptr_t cold_gc_frame);
  			/* Perform about one pages worth of marking	*/
  			/* work of whatever kind is needed.  Returns	*/
  			/* quickly if no collection is in progress.	*/
  			/* Return TRUE if mark phase finished.		*/
void GC_initiate_gc(void);
				/* initiate collection.			*/
  				/* If the mark state is invalid, this	*/
  				/* becomes full colleection.  Otherwise */
  				/* it's partial.			*/
void GC_push_all(ptr_t bottom, ptr_t top);
				/* Push everything in a range 		*/
  				/* onto mark stack.			*/
void GC_push_selected(ptr_t bottom, ptr_t top,
		      int (*dirty_fn) (struct hblk *h),
		      void (*push_fn) (ptr_t bottom, ptr_t top) );
				  /* Push all pages h in [b,t) s.t. 	*/
				  /* select_fn(h) != 0 onto mark stack. */
#ifndef SMALL_CONFIG
  void GC_push_conditional (ptr_t b, ptr_t t, GC_bool all);
#else
# define GC_push_conditional(b, t, all) GC_push_all(b, t)
#endif
                                /* Do either of the above, depending	*/
				/* on the third arg.			*/
void GC_push_all_stack (ptr_t b, ptr_t t);
				    /* As above, but consider		*/
				    /*  interior pointers as valid  	*/
void GC_push_all_eager (ptr_t b, ptr_t t);
				    /* Same as GC_push_all_stack, but   */
				    /* ensures that stack is scanned	*/
				    /* immediately, not just scheduled  */
				    /* for scanning.			*/
#ifndef THREADS
  void GC_push_all_stack_partially_eager(ptr_t bottom, ptr_t top,
		  			 ptr_t cold_gc_frame);
			/* Similar to GC_push_all_eager, but only the	*/
			/* part hotter than cold_gc_frame is scanned	*/
			/* immediately.  Needed to ensure that callee-	*/
			/* save registers are not missed.		*/
#else
  /* In the threads case, we push part of the current thread stack	*/
  /* with GC_push_all_eager when we push the registers.  This gets the  */
  /* callee-save registers that may disappear.  The remainder of the	*/
  /* stacks are scheduled for scanning in *GC_push_other_roots, which	*/
  /* is thread-package-specific.					*/
#endif
void GC_push_current_stack(ptr_t cold_gc_frame, void *context);
  			/* Push enough of the current stack eagerly to	*/
  			/* ensure that callee-save registers saved in	*/
  			/* GC frames are scanned.			*/
  			/* In the non-threads case, schedule entire	*/
  			/* stack for scanning.				*/
			/* The second argument is a pointer to the 	*/
			/* (possibly null) thread context, for		*/
			/* (currently hypothetical) more precise	*/
			/* stack scanning.				*/
void GC_push_roots(GC_bool all, ptr_t cold_gc_frame);
  			/* Push all or dirty roots.	*/
extern void (*GC_push_other_roots)(void);
  			/* Push system or application specific roots	*/
  			/* onto the mark stack.  In some environments	*/
  			/* (e.g. threads environments) this is		*/
  			/* predfined to be non-zero.  A client supplied */
  			/* replacement should also call the original	*/
  			/* function.					*/
extern void GC_push_gc_structures(void);
			/* Push GC internal roots.  These are normally	*/
			/* included in the static data segment, and 	*/
			/* Thus implicitly pushed.  But we must do this	*/
			/* explicitly if normal root processing is 	*/
			/* disabled.  Calls the following:		*/
	extern void GC_push_finalizer_structures(void);
	extern void GC_push_stubborn_structures (void);
#	ifdef THREADS
	  extern void GC_push_thread_structures (void);
#	endif
extern void (*GC_start_call_back) (void);
  			/* Called at start of full collections.		*/
  			/* Not called if 0.  Called with allocation 	*/
  			/* lock held.					*/
  			/* 0 by default.				*/
void GC_push_regs_and_stack(ptr_t cold_gc_frame);

void GC_push_regs(void);

void GC_with_callee_saves_pushed(void (*fn)(ptr_t, void *),
				 ptr_t arg);

# if defined(SPARC) || defined(IA64)
  /* Cause all stacked registers to be saved in memory.  Return a	*/
  /* pointer to the top of the corresponding memory stack.		*/
  ptr_t GC_save_regs_in_stack(void);
# endif
			/* Push register contents onto mark stack.	*/
  			/* If NURSERY is defined, the default push	*/
  			/* action can be overridden with GC_push_proc	*/

# ifdef NURSERY
    extern void (*GC_push_proc)(ptr_t);
# endif
# if defined(MSWIN32) || defined(MSWINCE)
  void __cdecl GC_push_one(word p);
# else
  void GC_push_one(word p);
			      /* If p points to an object, mark it    */
                              /* and push contents on the mark stack  */
  			      /* Pointer recognition test always      */
  			      /* accepts interior pointers, i.e. this */
  			      /* is appropriate for pointers found on */
  			      /* stack.				      */
# endif
# if defined(PRINT_BLACK_LIST) || defined(KEEP_BACK_PTRS)
  void GC_mark_and_push_stack(ptr_t p, ptr_t source);
				/* Ditto, omits plausibility test	*/
# else
  void GC_mark_and_push_stack(ptr_t p);
# endif
void GC_push_marked(struct hblk * h, hdr * hhdr);
		/* Push contents of all marked objects in h onto	*/
		/* mark stack.						*/
#ifdef SMALL_CONFIG
# define GC_push_next_marked_dirty(h) GC_push_next_marked(h)
#else
  struct hblk * GC_push_next_marked_dirty(struct hblk * h);
		/* Invoke GC_push_marked on next dirty block above h.	*/
		/* Return a pointer just past the end of this block.	*/
#endif /* !SMALL_CONFIG */
struct hblk * GC_push_next_marked(struct hblk * h);
  		/* Ditto, but also mark from clean pages.	*/
struct hblk * GC_push_next_marked_uncollectable(struct hblk * h);
  		/* Ditto, but mark only from uncollectable pages.	*/
GC_bool GC_stopped_mark(GC_stop_func stop_func);
 			/* Stop world and mark from all roots	*/
  			/* and rescuers.			*/
void GC_clear_hdr_marks(hdr * hhdr);
				    /* Clear the mark bits in a header */
void GC_set_hdr_marks(hdr * hhdr);
 				    /* Set the mark bits in a header */
void GC_set_fl_marks(ptr_t p);
				    /* Set all mark bits associated with */
				    /* a free list.			 */
#ifdef GC_ASSERTIONS
  void GC_check_fl_marks(ptr_t p);
				    /* Check that  all mark bits	*/
  				    /* associated with a free list are  */
  				    /* set.  Abort if not.		*/
#endif
void GC_add_roots_inner(ptr_t b, ptr_t e, GC_bool tmp);
void GC_remove_roots_inner(ptr_t b, ptr_t e);
GC_bool GC_is_static_root(ptr_t p);
  		/* Is the address p in one of the registered static	*/
  		/* root sections?					*/
# if defined(MSWIN32) || defined(_WIN32_WCE_EMULATION)
GC_bool GC_is_tmp_root(ptr_t p);
		/* Is the address p in one of the temporary static	*/
		/* root sections?					*/
# endif
void GC_register_dynamic_libraries(void);
  		/* Add dynamic library data sections to the root set. */
void GC_cond_register_dynamic_libraries(void);
  		/* Remove and reregister dynamic libraries if we're	*/
		/* configured to do that at each GC.			*/

GC_bool GC_register_main_static_data(void);
		/* We need to register the main data segment.  Returns	*/
		/* TRUE unless this is done implicitly as part of	*/
		/* dynamic library registration.			*/
  
/* Machine dependent startup routines */
ptr_t GC_get_main_stack_base(void);	/* Cold end of stack */
#ifdef IA64
  ptr_t GC_get_register_stack_base(void);
  					/* Cold end of register stack.	*/
#endif
void GC_register_data_segments(void);
  
/* Black listing: */
void GC_bl_init(void);
# ifdef PRINT_BLACK_LIST
      void GC_add_to_black_list_normal(word p, ptr_t source);
			/* Register bits as a possible future false	*/
			/* reference from the heap or static data	*/
#     define GC_ADD_TO_BLACK_LIST_NORMAL(bits, source) \
      		if (GC_all_interior_pointers) { \
		  GC_add_to_black_list_stack((word)(bits), (source)); \
		} else { \
  		  GC_add_to_black_list_normal((word)(bits), (source)); \
		}
# else
      void GC_add_to_black_list_normal(word p);
#     define GC_ADD_TO_BLACK_LIST_NORMAL(bits, source) \
      		if (GC_all_interior_pointers) { \
		  GC_add_to_black_list_stack((word)(bits)); \
		} else { \
  		  GC_add_to_black_list_normal((word)(bits)); \
		}
# endif

# ifdef PRINT_BLACK_LIST
    void GC_add_to_black_list_stack(word p, ptr_t source);
#   define GC_ADD_TO_BLACK_LIST_STACK(bits, source) \
	    GC_add_to_black_list_stack((word)(bits), (source))
# else
    void GC_add_to_black_list_stack(word p);
#   define GC_ADD_TO_BLACK_LIST_STACK(bits, source) \
	    GC_add_to_black_list_stack((word)(bits))
# endif
struct hblk * GC_is_black_listed(struct hblk * h, word len);
  			/* If there are likely to be false references	*/
  			/* to a block starting at h of the indicated    */
  			/* length, then return the next plausible	*/
  			/* starting location for h that might avoid	*/
  			/* these false references.			*/
void GC_promote_black_lists(void);
  			/* Declare an end to a black listing phase.	*/
void GC_unpromote_black_lists(void);
  			/* Approximately undo the effect of the above.	*/
  			/* This actually loses some information, but	*/
  			/* only in a reasonably safe way.		*/
word GC_number_stack_black_listed(struct hblk *start, struct hblk *endp1);
  			/* Return the number of (stack) blacklisted	*/
  			/* blocks in the range for statistical		*/
  			/* purposes.					*/
  		 	
ptr_t GC_scratch_alloc(size_t bytes);
  				/* GC internal memory allocation for	*/
  				/* small objects.  Deallocation is not  */
  				/* possible.				*/
  	
/* Heap block layout maps: */			
GC_bool GC_add_map_entry(size_t sz);
  				/* Add a heap block map for objects of	*/
  				/* size sz to obj_map.			*/
  				/* Return FALSE on failure.		*/
void GC_register_displacement_inner(size_t offset);
  				/* Version of GC_register_displacement	*/
  				/* that assumes lock is already held	*/
  				/* and signals are already disabled.	*/

void GC_initialize_offsets(void);
				/* Initialize GC_valid_offsets,		*/
				/* depending on current 		*/
				/* GC_all_interior_pointers settings.	*/
  
/*  hblk allocation: */		
void GC_new_hblk(size_t size_in_granules, int kind);
  				/* Allocate a new heap block, and build */
  				/* a free list in it.			*/				

ptr_t GC_build_fl(struct hblk *h, size_t words, GC_bool clear, ptr_t list);
				/* Build a free list for objects of 	*/
				/* size sz in block h.  Append list to	*/
				/* end of the free lists.  Possibly	*/
				/* clear objects on the list.  Normally	*/
				/* called by GC_new_hblk, but also	*/
				/* called explicitly without GC lock.	*/

struct hblk * GC_allochblk (size_t size_in_bytes, int kind,
		            unsigned flags);
				/* Allocate a heap block, inform	*/
				/* the marker that block is valid	*/
				/* for objects of indicated size.	*/

ptr_t GC_alloc_large (size_t lb, int k, unsigned flags);
			/* Allocate a large block of size lb bytes.	*/
			/* The block is not cleared.			*/
			/* Flags is 0 or IGNORE_OFF_PAGE.		*/
			/* Calls GC_allchblk to do the actual 		*/
			/* allocation, but also triggers GC and/or	*/
			/* heap expansion as appropriate.		*/
			/* Does not update GC_bytes_allocd, but does	*/
			/* other accounting.				*/

ptr_t GC_alloc_large_and_clear(size_t lb, int k, unsigned flags);
			/* As above, but clear block if appropriate	*/
			/* for kind k.					*/

void GC_freehblk(struct hblk * p);
				/* Deallocate a heap block and mark it  */
  				/* as invalid.				*/
  				
/*  Misc GC: */
void GC_init_inner(void);
GC_bool GC_expand_hp_inner(word n);
void GC_start_reclaim(int abort_if_found);
  				/* Restore unmarked objects to free	*/
  				/* lists, or (if abort_if_found is	*/
  				/* TRUE) report them.			*/
  				/* Sweeping of small object pages is	*/
  				/* largely deferred.			*/
void GC_continue_reclaim(size_t sz, int kind);
  				/* Sweep pages of the given size and	*/
  				/* kind, as long as possible, and	*/
  				/* as long as the corr. free list is    */
  				/* empty.  Sz is in granules.		*/
void GC_reclaim_or_delete_all(void);
  				/* Arrange for all reclaim lists to be	*/
  				/* empty.  Judiciously choose between	*/
  				/* sweeping and discarding each page.	*/
GC_bool GC_reclaim_all(GC_stop_func stop_func, GC_bool ignore_old);
  				/* Reclaim all blocks.  Abort (in a	*/
  				/* consistent state) if f returns TRUE. */
ptr_t GC_reclaim_generic(struct hblk * hbp, hdr *hhdr, size_t sz,
			 GC_bool init, ptr_t list, signed_word *count);
			 	/* Rebuild free list in hbp with 	*/
				/* header hhdr, with objects of size sz */
				/* bytes.  Add list to the end of the	*/
				/* free list.  Add the number of	*/
				/* reclaimed bytes to *count.		*/
GC_bool GC_block_empty(hdr * hhdr);
 				/* Block completely unmarked? 	*/
GC_bool GC_never_stop_func(void);
				/* Returns FALSE.		*/
GC_bool GC_try_to_collect_inner(GC_stop_func f);

				/* Collect; caller must have acquired	*/
				/* lock and disabled signals.		*/
				/* Collection is aborted if f returns	*/
				/* TRUE.  Returns TRUE if it completes	*/
				/* successfully.			*/
# define GC_gcollect_inner() \
	(void) GC_try_to_collect_inner(GC_never_stop_func)
void GC_finish_collection(void);
 				/* Finish collection.  Mark bits are	*/
  				/* consistent and lock is still held.	*/
GC_bool GC_collect_or_expand(word needed_blocks, GC_bool ignore_off_page);
  				/* Collect or expand heap in an attempt */
  				/* make the indicated number of free	*/
  				/* blocks available.  Should be called	*/
  				/* until the blocks are available or	*/
  				/* until it fails by returning FALSE.	*/

extern GC_bool GC_is_initialized;	/* GC_init() has been run.	*/

#if defined(MSWIN32) || defined(MSWINCE)
  void GC_deinit(void);
                                /* Free any resources allocated by      */
                                /* GC_init                              */
#endif

void GC_collect_a_little_inner(int n);
  				/* Do n units worth of garbage 		*/
  				/* collection work, if appropriate.	*/
  				/* A unit is an amount appropriate for  */
  				/* HBLKSIZE bytes of allocation.	*/
/* void * GC_generic_malloc(size_t lb, int k); */
  				/* Allocate an object of the given	*/
  				/* kind.  By default, there are only	*/
  				/* a few kinds: composite(pointerfree), */
				/* atomic, uncollectable, etc.		*/
				/* We claim it's possible for clever	*/
				/* client code that understands GC	*/
				/* internals to add more, e.g. to	*/
				/* communicate object layout info	*/
				/* to the collector.			*/
				/* The actual decl is in gc_mark.h.	*/
void * GC_generic_malloc_ignore_off_page(size_t b, int k);
  				/* As above, but pointers past the 	*/
  				/* first page of the resulting object	*/
  				/* are ignored.				*/
void * GC_generic_malloc_inner(size_t lb, int k);
  				/* Ditto, but I already hold lock, etc.	*/
void * GC_generic_malloc_inner_ignore_off_page(size_t lb, int k);
  				/* Allocate an object, where		*/
  				/* the client guarantees that there	*/
  				/* will always be a pointer to the 	*/
  				/* beginning of the object while the	*/
  				/* object is live.			*/
void GC_generic_malloc_many(size_t lb, int k, void **result);
				/* Store a pointer to a list of newly	*/
				/* allocated objects of kind k and size */
				/* lb in *result.			*/
				/* Caler must make sure that *result is	*/
				/* traced even if objects are ptrfree.	*/
ptr_t GC_allocobj(size_t sz, int kind);
  				/* Make the indicated 			*/
  				/* free list nonempty, and return its	*/
  				/* head.  Sz is in granules.		*/

/* Allocation routines that bypass the thread local cache.	*/
/* Used internally.						*/
#ifdef THREAD_LOCAL_ALLOC
  void * GC_core_malloc(size_t);
  void * GC_core_malloc_atomic(size_t);
# ifdef GC_GCJ_SUPPORT
    void *GC_core_gcj_malloc(size_t, void *); 
# endif
#endif /* THREAD_LOCAL_ALLOC */

void GC_free_inner(void * p);
void GC_debug_free_inner(void * p);
  
void GC_init_headers(void);
struct hblkhdr * GC_install_header(struct hblk *h);
  				/* Install a header for block h.	*/
  				/* Return 0 on failure, or the header	*/
  				/* otherwise.				*/
GC_bool GC_install_counts(struct hblk * h, size_t sz);
  				/* Set up forwarding counts for block	*/
  				/* h of size sz.			*/
  				/* Return FALSE on failure.		*/
void GC_remove_header(struct hblk * h);
  				/* Remove the header for block h.	*/
void GC_remove_counts(struct hblk * h, size_t sz);
  				/* Remove forwarding counts for h.	*/
hdr * GC_find_header(ptr_t h); /* Debugging only.		*/
  
void GC_finalize(void);
 			/* Perform all indicated finalization actions	*/
  			/* on unmarked objects.				*/
  			/* Unreachable finalizable objects are enqueued	*/
  			/* for processing by GC_invoke_finalizers.	*/
  			/* Invoked with lock.				*/

void GC_notify_or_invoke_finalizers(void);
			/* If GC_finalize_on_demand is not set, invoke	*/
			/* eligible finalizers. Otherwise:		*/
			/* Call *GC_finalizer_notifier if there are	*/
			/* finalizers to be run, and we haven't called	*/
			/* this procedure yet this GC cycle.		*/

GC_API void * GC_make_closure(GC_finalization_proc fn, void * data);
GC_API void GC_debug_invoke_finalizer(void * obj, void * data);
			/* Auxiliary fns to make finalization work	*/
			/* correctly with displaced pointers introduced	*/
			/* by the debugging allocators.			*/
  			
void GC_add_to_heap(struct hblk *p, size_t bytes);
  			/* Add a HBLKSIZE aligned chunk to the heap.	*/
  
void GC_print_obj(ptr_t p);
  			/* P points to somewhere inside an object with	*/
  			/* debugging info.  Print a human readable	*/
  			/* description of the object to stderr.		*/
extern void (*GC_check_heap)(void);
  			/* Check that all objects in the heap with 	*/
  			/* debugging info are intact.  			*/
  			/* Add any that are not to GC_smashed list.	*/
extern void (*GC_print_all_smashed) (void);
			/* Print GC_smashed if it's not empty.		*/
			/* Clear GC_smashed list.			*/
extern void GC_print_all_errors (void);
			/* Print smashed and leaked objects, if any.	*/
			/* Clear the lists of such objects.		*/
extern void (*GC_print_heap_obj) (ptr_t p);
  			/* If possible print s followed by a more	*/
  			/* detailed description of the object 		*/
  			/* referred to by p.				*/
#if defined(LINUX) && defined(__ELF__) && !defined(SMALL_CONFIG)
  void GC_print_address_map (void);
  			/* Print an address map of the process.		*/
#endif

extern GC_bool GC_have_errors;  /* We saw a smashed or leaked object.	*/
				/* Call error printing routine 		*/
				/* occasionally.			*/

#ifndef SMALL_CONFIG
  extern int GC_print_stats;	/* Nonzero generates basic GC log.	*/
				/* VERBOSE generates add'l messages.	*/
#else
# define GC_print_stats 0
  	/* Will this keep the message character strings from the executable? */
  	/* It should ...						     */
#endif
#define VERBOSE 2

#ifndef NO_DEBUGGING
  extern GC_bool GC_dump_regularly;  /* Generate regular debugging dumps. */
# define COND_DUMP if (GC_dump_regularly) GC_dump();
#else
# define COND_DUMP
#endif

#ifdef KEEP_BACK_PTRS
  extern long GC_backtraces;
  void GC_generate_random_backtrace_no_gc(void);
#endif

extern GC_bool GC_print_back_height;

#ifdef MAKE_BACK_GRAPH
  void GC_print_back_graph_stats(void);
#endif

/* Macros used for collector internal allocation.	*/
/* These assume the collector lock is held.		*/
#ifdef DBG_HDRS_ALL
    extern void * GC_debug_generic_malloc_inner(size_t lb, int k);
    extern void * GC_debug_generic_malloc_inner_ignore_off_page(size_t lb,
								int k);
#   define GC_INTERNAL_MALLOC GC_debug_generic_malloc_inner
#   define GC_INTERNAL_MALLOC_IGNORE_OFF_PAGE \
		 GC_debug_generic_malloc_inner_ignore_off_page
#   ifdef THREADS
#       define GC_INTERNAL_FREE GC_debug_free_inner
#   else
#       define GC_INTERNAL_FREE GC_debug_free
#   endif
#else
#   define GC_INTERNAL_MALLOC GC_generic_malloc_inner
#   define GC_INTERNAL_MALLOC_IGNORE_OFF_PAGE \
		 GC_generic_malloc_inner_ignore_off_page
#   ifdef THREADS
#       define GC_INTERNAL_FREE GC_free_inner
#   else
#       define GC_INTERNAL_FREE GC_free
#   endif
#endif

/* Memory unmapping: */
#ifdef USE_MUNMAP
  void GC_unmap_old(void);
  void GC_merge_unmapped(void);
  void GC_unmap(ptr_t start, size_t bytes);
  void GC_remap(ptr_t start, size_t bytes);
  void GC_unmap_gap(ptr_t start1, size_t bytes1, ptr_t start2, size_t bytes2);
#endif

/* Virtual dirty bit implementation:		*/
/* Each implementation exports the following:	*/
void GC_read_dirty(void);
			/* Retrieve dirty bits.	*/
GC_bool GC_page_was_dirty(struct hblk *h);
  			/* Read retrieved dirty bits.	*/
GC_bool GC_page_was_ever_dirty(struct hblk *h);
  			/* Could the page contain valid heap pointers?	*/
void GC_remove_protection(struct hblk *h, word nblocks,
			  GC_bool pointerfree);
  			/* h is about to be writteni or allocated.  Ensure  */
			/* that it's not write protected by the virtual	    */
			/* dirty bit implementation.			    */
			
void GC_dirty_init(void);
  
/* Slow/general mark bit manipulation: */
GC_API GC_bool GC_is_marked(ptr_t p);
void GC_clear_mark_bit(ptr_t p);
void GC_set_mark_bit(ptr_t p);
  
/* Stubborn objects: */
void GC_read_changed(void);	/* Analogous to GC_read_dirty */
GC_bool GC_page_was_changed(struct hblk * h);
 				/* Analogous to GC_page_was_dirty */
void GC_clean_changing_list(void);
 				/* Collect obsolete changing list entries */
void GC_stubborn_init(void);
  
/* Debugging print routines: */
void GC_print_block_list(void);
void GC_print_hblkfreelist(void);
void GC_print_heap_sects(void);
void GC_print_static_roots(void);
void GC_print_finalization_stats(void);
/* void GC_dump(void); - declared in gc.h */

#ifdef KEEP_BACK_PTRS
   void GC_store_back_pointer(ptr_t source, ptr_t dest);
   void GC_marked_for_finalization(ptr_t dest);
#  define GC_STORE_BACK_PTR(source, dest) GC_store_back_pointer(source, dest)
#  define GC_MARKED_FOR_FINALIZATION(dest) GC_marked_for_finalization(dest)
#else
#  define GC_STORE_BACK_PTR(source, dest) 
#  define GC_MARKED_FOR_FINALIZATION(dest)
#endif

/* Make arguments appear live to compiler */
# ifdef __WATCOMC__
    void GC_noop(void*, ...);
# else
#   ifdef __DMC__
      GC_API void GC_noop(...);
#   else
      GC_API void GC_noop();
#   endif
# endif

void GC_noop1(word);

/* Logging and diagnostic output: 	*/
GC_API void GC_printf (const char * format, ...);
			/* A version of printf that doesn't allocate,	*/
			/* 1K total output length.			*/
			/* (We use sprintf.  Hopefully that doesn't	*/
			/* allocate for long arguments.)  		*/
GC_API void GC_err_printf(const char * format, ...);
GC_API void GC_log_printf(const char * format, ...);
void GC_err_puts(const char *s);
			/* Write s to stderr, don't buffer, don't add	*/
			/* newlines, don't ...				*/

#if defined(LINUX) && !defined(SMALL_CONFIG)
  void GC_err_write(const char *buf, size_t len);
 			/* Write buf to stderr, don't buffer, don't add	*/
  			/* newlines, don't ...				*/
#endif


# ifdef GC_ASSERTIONS
#	define GC_ASSERT(expr) if(!(expr)) {\
		GC_err_printf("Assertion failure: %s:%ld\n", \
				__FILE__, (unsigned long)__LINE__); \
		ABORT("assertion failure"); }
# else 
#	define GC_ASSERT(expr)
# endif

/* Check a compile time assertion at compile time.  The error	*/
/* message for failure is a bit baroque, but ...		*/
#if defined(mips) && !defined(__GNUC__)
/* DOB: MIPSPro C gets an internal error taking the sizeof an array type. 
   This code works correctly (ugliness is to avoid "unused var" warnings) */
# define GC_STATIC_ASSERT(expr) do { if (0) { char j[(expr)? 1 : -1]; j[0]='\0'; j[0]=j[0]; } } while(0)
#else
# define GC_STATIC_ASSERT(expr) sizeof(char[(expr)? 1 : -1])
#endif

# if defined(PARALLEL_MARK) || defined(THREAD_LOCAL_ALLOC)
    /* We need additional synchronization facilities from the thread	*/
    /* support.  We believe these are less performance critical		*/
    /* than the main garbage collector lock; standard pthreads-based	*/
    /* implementations should be sufficient.				*/

    /* The mark lock and condition variable.  If the GC lock is also 	*/
    /* acquired, the GC lock must be acquired first.  The mark lock is	*/
    /* used to both protect some variables used by the parallel		*/
    /* marker, and to protect GC_fl_builder_count, below.		*/
    /* GC_notify_all_marker() is called when				*/ 
    /* the state of the parallel marker changes				*/
    /* in some significant way (see gc_mark.h for details).  The	*/
    /* latter set of events includes incrementing GC_mark_no.		*/
    /* GC_notify_all_builder() is called when GC_fl_builder_count	*/
    /* reaches 0.							*/

     extern void GC_acquire_mark_lock();
     extern void GC_release_mark_lock();
     extern void GC_notify_all_builder();
     /* extern void GC_wait_builder(); */
     extern void GC_wait_for_reclaim();

     extern word GC_fl_builder_count;	/* Protected by mark lock.	*/
# endif /* PARALLEL_MARK || THREAD_LOCAL_ALLOC */
# ifdef PARALLEL_MARK
     extern void GC_notify_all_marker();
     extern void GC_wait_marker();
     extern word GC_mark_no;		/* Protected by mark lock.	*/

     extern void GC_help_marker(word my_mark_no);
		/* Try to help out parallel marker for mark cycle 	*/
		/* my_mark_no.  Returns if the mark cycle finishes or	*/
		/* was already done, or there was nothing to do for	*/
		/* some other reason.					*/
# endif /* PARALLEL_MARK */

# if defined(GC_PTHREADS)
  /* We define the thread suspension signal here, so that we can refer	*/
  /* to it in the dirty bit implementation, if necessary.  Ideally we	*/
  /* would allocate a (real-time ?) signal using the standard mechanism.*/
  /* unfortunately, there is no standard mechanism.  (There is one 	*/
  /* in Linux glibc, but it's not exported.)  Thus we continue to use	*/
  /* the same hard-coded signals we've always used.			*/
#  if !defined(SIG_SUSPEND)
#   if defined(GC_LINUX_THREADS) || defined(GC_DGUX386_THREADS)
#    if defined(SPARC) && !defined(SIGPWR)
       /* SPARC/Linux doesn't properly define SIGPWR in <signal.h>.
        * It is aliased to SIGLOST in asm/signal.h, though.		*/
#      define SIG_SUSPEND SIGLOST
#    else
       /* Linuxthreads itself uses SIGUSR1 and SIGUSR2.			*/
#      define SIG_SUSPEND SIGPWR
#    endif
#   else  /* !GC_LINUX_THREADS */
#     if defined(_SIGRTMIN)
#       define SIG_SUSPEND _SIGRTMIN + 6
#     else
#       define SIG_SUSPEND SIGRTMIN + 6
#     endif       
#   endif
#  endif /* !SIG_SUSPEND */
  
# endif
     
/* Some macros for setjmp that works across signal handlers	*/
/* were possible, and a couple of routines to facilitate	*/
/* catching accesses to bad addresses when that's		*/
/* possible/needed.						*/
#ifdef UNIX_LIKE
# include <setjmp.h>
# if defined(SUNOS5SIGS) && !defined(FREEBSD)
#  include <sys/siginfo.h>
# endif
  /* Define SETJMP and friends to be the version that restores	*/
  /* the signal mask.						*/
# define SETJMP(env) sigsetjmp(env, 1)
# define LONGJMP(env, val) siglongjmp(env, val)
# define JMP_BUF sigjmp_buf
#else
# ifdef ECOS
#   define SETJMP(env)  hal_setjmp(env)
# else
#   define SETJMP(env) setjmp(env)
# endif
# define LONGJMP(env, val) longjmp(env, val)
# define JMP_BUF jmp_buf
#endif

/* Do we need the GC_find_limit machinery to find the end of a 	*/
/* data segment.						*/
# if defined(HEURISTIC2) || defined(SEARCH_FOR_DATA_START)
#   define NEED_FIND_LIMIT
# endif

# if !defined(STACKBOTTOM) && defined(HEURISTIC2)
#   define NEED_FIND_LIMIT
# endif

# if (defined(SVR4) || defined(AUX) || defined(DGUX) \
      || (defined(LINUX) && defined(SPARC))) && !defined(PCR)
#   define NEED_FIND_LIMIT
# endif

#if defined(FREEBSD) && (defined(I386) || defined(X86_64) || defined(powerpc) \
    || defined(__powerpc__))
#  include <machine/trap.h>
#  if !defined(PCR)
#    define NEED_FIND_LIMIT
#  endif
#endif

#if (defined(NETBSD) || defined(OPENBSD)) && defined(__ELF__) \
    && !defined(NEED_FIND_LIMIT)
   /* Used by GC_init_netbsd_elf() in os_dep.c.	*/
#  define NEED_FIND_LIMIT
#endif

#if defined(IA64) && !defined(NEED_FIND_LIMIT)
#  define NEED_FIND_LIMIT
     /* May be needed for register backing store base. */
#endif

# if defined(NEED_FIND_LIMIT) || \
     defined(USE_PROC_FOR_LIBRARIES) && defined(THREADS)
JMP_BUF GC_jmp_buf;

/* Set up a handler for address faults which will longjmp to	*/
/* GC_jmp_buf;							*/
extern void GC_setup_temporary_fault_handler(void);

/* Undo the effect of GC_setup_temporary_fault_handler.		*/
extern void GC_reset_fault_handler(void);

# endif /* Need to handle address faults.	*/

# endif /* GC_PRIVATE_H */
