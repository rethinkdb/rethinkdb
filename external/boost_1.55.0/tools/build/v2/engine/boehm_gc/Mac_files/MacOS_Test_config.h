/*
	MacOS_Test_config.h
	
	Configuration flags for Macintosh development systems.
	
	Test version.
	
 	<Revision History>

 	11/16/95  pcb  Updated compilation flags to reflect latest 4.6 Makefile.
	
	by Patrick C. Beard.
 */
/* Boehm, November 17, 1995 12:05 pm PST */

#ifdef __MWERKS__

// for CodeWarrior Pro with Metrowerks Standard Library (MSL).
// #define MSL_USE_PRECOMPILED_HEADERS 0
#include <ansi_prefix.mac.h>
#ifndef __STDC__
#define __STDC__ 0
#endif

#endif

// these are defined again in gc_priv.h.
#undef TRUE
#undef FALSE

#define ALL_INTERIOR_POINTERS	// follows interior pointers.
//#define SILENT				// want collection messages.
//#define DONT_ADD_BYTE_AT_END	// no padding.
//#define SMALL_CONFIG			// whether to a smaller heap.
#define NO_SIGNALS				// signals aren't real on the Macintosh.
#define USE_TEMPORARY_MEMORY	// use Macintosh temporary memory.

// CFLAGS= -O -DNO_SIGNALS -DALL_INTERIOR_POINTERS -DSILENT
//
//LIBGC_CFLAGS= -O -DNO_SIGNALS -DSILENT \
//    -DREDIRECT_MALLOC=GC_malloc_uncollectable \
//    -DDONT_ADD_BYTE_AT_END -DALL_INTERIOR_POINTERS
//   Flags for building libgc.a -- the last two are required.
//
// Setjmp_test may yield overly optimistic results when compiled
// without optimization.
// -DSILENT disables statistics printing, and improves performance.
// -DCHECKSUMS reports on erroneously clear dirty bits, and unexpectedly
//   altered stubborn objects, at substantial performance cost.
//   Use only for incremental collector debugging.
// -DFIND_LEAK causes the collector to assume that all inaccessible
//   objects should have been explicitly deallocated, and reports exceptions.
//   Finalization and the test program are not usable in this mode.
// -DSOLARIS_THREADS enables support for Solaris (thr_) threads.
//   (Clients should also define SOLARIS_THREADS and then include
//   gc.h before performing thr_ or GC_ operations.)
//   This is broken on nonSPARC machines.
// -DALL_INTERIOR_POINTERS allows all pointers to the interior
//   of objects to be recognized.  (See gc_priv.h for consequences.)
// -DSMALL_CONFIG tries to tune the collector for small heap sizes,
//   usually causing it to use less space in such situations.
//   Incremental collection no longer works in this case.
// -DLARGE_CONFIG tunes the collector for unusually large heaps.
//   Necessary for heaps larger than about 500 MB on most machines.
//   Recommended for heaps larger than about 64 MB.
// -DDONT_ADD_BYTE_AT_END is meaningful only with
//   -DALL_INTERIOR_POINTERS.  Normally -DALL_INTERIOR_POINTERS
//   causes all objects to be padded so that pointers just past the end of
//   an object can be recognized.  This can be expensive.  (The padding
//   is normally more than one byte due to alignment constraints.)
//   -DDONT_ADD_BYTE_AT_END disables the padding.
// -DNO_SIGNALS does not disable signals during critical parts of
//   the GC process.  This is no less correct than many malloc 
//   implementations, and it sometimes has a significant performance
//   impact.  However, it is dangerous for many not-quite-ANSI C
//   programs that call things like printf in asynchronous signal handlers.
// -DGC_OPERATOR_NEW_ARRAY declares that the C++ compiler supports the
//   new syntax "operator new[]" for allocating and deleting arrays.
//   See gc_cpp.h for details.  No effect on the C part of the collector.
//   This is defined implicitly in a few environments.
// -DREDIRECT_MALLOC=X causes malloc, realloc, and free to be defined
//   as aliases for X, GC_realloc, and GC_free, respectively.
//   Calloc is redefined in terms of the new malloc.  X should
//   be either GC_malloc or GC_malloc_uncollectable.
//   The former is occasionally useful for working around leaks in code
//   you don't want to (or can't) look at.  It may not work for
//   existing code, but it often does.  Neither works on all platforms,
//   since some ports use malloc or calloc to obtain system memory.
//   (Probably works for UNIX, and win32.)
// -DNO_DEBUG removes GC_dump and the debugging routines it calls.
//   Reduces code size slightly at the expense of debuggability.
