/* 
 * Copyright 1988, 1989 Hans-J. Boehm, Alan J. Demers
 * Copyright (c) 1991-1995 by Xerox Corporation.  All rights reserved.
 * Copyright 1996-1999 by Silicon Graphics.  All rights reserved.
 * Copyright 1999 by Hewlett-Packard Company.  All rights reserved.
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

/* This file assumes the collector has been compiled with GC_GCJ_SUPPORT */
/* and that an ANSI C compiler is available.				 */

/*
 * We allocate objects whose first word contains a pointer to a struct
 * describing the object type.  This struct contains a garbage collector mark
 * descriptor at offset MARK_DESCR_OFFSET.  Alternatively, the objects
 * may be marked by the mark procedure passed to GC_init_gcj_malloc.
 */

#ifndef GC_GCJ_H

#define GC_GCJ_H

#ifndef MARK_DESCR_OFFSET
#  define MARK_DESCR_OFFSET	sizeof(word)
#endif
	/* Gcj keeps GC descriptor as second word of vtable.	This	*/
	/* probably needs to be adjusted for other clients.		*/
	/* We currently assume that this offset is such that:		*/
	/*	- all objects of this kind are large enough to have	*/
	/*	  a value at that offset, and				*/
	/* 	- it is not zero.					*/
	/* These assumptions allow objects on the free list to be 	*/
	/* marked normally.						*/

#ifndef _GC_H
#   include "gc.h"
#endif

/* The following allocators signal an out of memory condition with	*/
/* return GC_oom_fn(bytes);						*/

/* The following function must be called before the gcj allocators	*/
/* can be invoked.							*/
/* mp_index and mp are the index and mark_proc (see gc_mark.h)		*/
/* respectively for the allocated objects.  Mark_proc will be 		*/
/* used to build the descriptor for objects allocated through the	*/
/* debugging interface.  The mark_proc will be invoked on all such 	*/
/* objects with an "environment" value of 1.  The client may choose	*/
/* to use the same mark_proc for some of its generated mark descriptors.*/
/* In that case, it should use a different "environment" value to	*/
/* detect the presence or absence of the debug header.			*/
/* Mp is really of type mark_proc, as defined in gc_mark.h.  We don't 	*/
/* want to include that here for namespace pollution reasons.		*/
extern void GC_init_gcj_malloc(int mp_index, void * /* really mark_proc */mp);

/* Allocate an object, clear it, and store the pointer to the	*/
/* type structure (vtable in gcj).				*/
/* This adds a byte at the end of the object if GC_malloc would.*/
extern void * GC_gcj_malloc(size_t lb, void * ptr_to_struct_containing_descr);
/* The debug versions allocate such that the specified mark_proc	*/
/* is always invoked.							*/
extern void * GC_debug_gcj_malloc(size_t lb,
				  void * ptr_to_struct_containing_descr,
				  GC_EXTRA_PARAMS);

/* Similar to GC_gcj_malloc, but assumes that a pointer to near the	*/
/* beginning of the resulting object is always maintained.		*/
extern void * GC_gcj_malloc_ignore_off_page(size_t lb,
				void * ptr_to_struct_containing_descr);

/* The kind numbers of normal and debug gcj objects.		*/
/* Useful only for debug support, we hope.			*/
extern int GC_gcj_kind;

extern int GC_gcj_debug_kind;

# ifdef GC_DEBUG
#   define GC_GCJ_MALLOC(s,d) GC_debug_gcj_malloc(s,d,GC_EXTRAS)
#   define GC_GCJ_MALLOC_IGNORE_OFF_PAGE(s,d) GC_debug_gcj_malloc(s,d,GC_EXTRAS)
# else
#   define GC_GCJ_MALLOC(s,d) GC_gcj_malloc(s,d)
#   define GC_GCJ_MALLOC_IGNORE_OFF_PAGE(s,d) \
	GC_gcj_malloc_ignore_off_page(s,d)
# endif

#endif /* GC_GCJ_H */
