/*
 * Copyright 1988, 1989 Hans-J. Boehm, Alan J. Demers
 * Copyright (c) 1991-1994 by Xerox Corporation.  All rights reserved.
 * Copyright (c) 2000 by Hewlett-Packard Company.  All rights reserved.
 *
 * THIS MATERIAL IS PROVIDED AS IS, WITH ABSOLUTELY NO WARRANTY EXPRESSED
 * OR IMPLIED.  ANY USE IS AT YOUR OWN RISK.
 *
 * Permission is hereby granted to use or copy this program
 * for any purpose,  provided the above notices are retained on all copies.
 * Permission to modify the code and to distribute modified code is granted,
 * provided the above notices are retained, and a notice that the code was
 * modified is included with the above copyright notice.
 *
 * This file contains the functions:
 *	ptr_t GC_build_flXXX(h, old_fl)
 *	void GC_new_hblk(size)
 */
/* Boehm, May 19, 1994 2:09 pm PDT */


# include <stdio.h>
# include "private/gc_priv.h"

#ifndef SMALL_CONFIG
/*
 * Build a free list for size 2 (words) cleared objects inside hblk h.
 * Set the last link to
 * be ofl.  Return a pointer tpo the first free list entry.
 */
ptr_t GC_build_fl_clear2(struct hblk *h, ptr_t ofl)
{
    word * p = (word *)(h -> hb_body);
    word * lim = (word *)(h + 1);
    
    p[0] = (word)ofl;
    p[1] = 0;
    p[2] = (word)p;
    p[3] = 0;
    p += 4;
    for (; p < lim; p += 4) {
        p[0] = (word)(p-2);
        p[1] = 0;
        p[2] = (word)p;
        p[3] = 0;
    };
    return((ptr_t)(p-2));
}

/* The same for size 4 cleared objects */
ptr_t GC_build_fl_clear4(struct hblk *h, ptr_t ofl)
{
    word * p = (word *)(h -> hb_body);
    word * lim = (word *)(h + 1);
    
    p[0] = (word)ofl;
    p[1] = 0;
    p[2] = 0;
    p[3] = 0;
    p += 4;
    for (; p < lim; p += 4) {
	PREFETCH_FOR_WRITE((ptr_t)(p+64));
        p[0] = (word)(p-4);
        p[1] = 0;
	CLEAR_DOUBLE(p+2);
    };
    return((ptr_t)(p-4));
}

/* The same for size 2 uncleared objects */
ptr_t GC_build_fl2(struct hblk *h, ptr_t ofl)
{
    word * p = (word *)(h -> hb_body);
    word * lim = (word *)(h + 1);
    
    p[0] = (word)ofl;
    p[2] = (word)p;
    p += 4;
    for (; p < lim; p += 4) {
        p[0] = (word)(p-2);
        p[2] = (word)p;
    };
    return((ptr_t)(p-2));
}

/* The same for size 4 uncleared objects */
ptr_t GC_build_fl4(struct hblk *h, ptr_t ofl)
{
    word * p = (word *)(h -> hb_body);
    word * lim = (word *)(h + 1);
    
    p[0] = (word)ofl;
    p[4] = (word)p;
    p += 8;
    for (; p < lim; p += 8) {
	PREFETCH_FOR_WRITE((ptr_t)(p+64));
        p[0] = (word)(p-4);
        p[4] = (word)p;
    };
    return((ptr_t)(p-4));
}

#endif /* !SMALL_CONFIG */


/* Build a free list for objects of size sz inside heap block h.	*/
/* Clear objects inside h if clear is set.  Add list to the end of	*/
/* the free list we build.  Return the new free list.			*/
/* This could be called without the main GC lock, if we ensure that	*/
/* there is no concurrent collection which might reclaim objects that	*/
/* we have not yet allocated.						*/
ptr_t GC_build_fl(struct hblk *h, size_t sz, GC_bool clear, ptr_t list)
{
  word *p, *prev;
  word *last_object;		/* points to last object in new hblk	*/

  /* Do a few prefetches here, just because its cheap.  	*/
  /* If we were more serious about it, these should go inside	*/
  /* the loops.  But write prefetches usually don't seem to	*/
  /* matter much.						*/
    PREFETCH_FOR_WRITE((ptr_t)h);
    PREFETCH_FOR_WRITE((ptr_t)h + 128);
    PREFETCH_FOR_WRITE((ptr_t)h + 256);
    PREFETCH_FOR_WRITE((ptr_t)h + 378);
  /* Handle small objects sizes more efficiently.  For larger objects 	*/
  /* the difference is less significant.				*/
#  ifndef SMALL_CONFIG
    switch (sz) {
        case 2: if (clear) {
        	    return GC_build_fl_clear2(h, list);
        	} else {
        	    return GC_build_fl2(h, list);
        	}
        case 4: if (clear) {
        	    return GC_build_fl_clear4(h, list);
        	} else {
        	    return GC_build_fl4(h, list);
        	}
        default:
        	break;
    }
#  endif /* !SMALL_CONFIG */
    
  /* Clear the page if necessary. */
    if (clear) BZERO(h, HBLKSIZE);
    
  /* Add objects to free list */
    p = (word *)(h -> hb_body) + sz;	/* second object in *h	*/
    prev = (word *)(h -> hb_body);       	/* One object behind p	*/
    last_object = (word *)((char *)h + HBLKSIZE);
    last_object -= sz;
			    /* Last place for last object to start */

  /* make a list of all objects in *h with head as last object */
    while (p <= last_object) {
      /* current object's link points to last object */
        obj_link(p) = (ptr_t)prev;
	prev = p;
	p += sz;
    }
    p -= sz;			/* p now points to last object */

  /*
   * put p (which is now head of list of objects in *h) as first
   * pointer in the appropriate free list for this size.
   */
      obj_link(h -> hb_body) = list;
      return ((ptr_t)p);
}


/*
 * Allocate a new heapblock for small objects of size gran granules.
 * Add all of the heapblock's objects to the free list for objects
 * of that size.
 * Set all mark bits if objects are uncollectable.
 * Will fail to do anything if we are out of memory.
 */
void GC_new_hblk(size_t gran, int kind)
{
  struct hblk *h;	/* the new heap block			*/
  GC_bool clear = GC_obj_kinds[kind].ok_init;

  /* Ignore gcc "no effect" warning on the following: */
  GC_STATIC_ASSERT((sizeof (struct hblk)) == HBLKSIZE);
  
  if (GC_debugging_started) clear = TRUE;

  /* Allocate a new heap block */
    h = GC_allochblk(GRANULES_TO_BYTES(gran), kind, 0);
    if (h == 0) return;

  /* Mark all objects if appropriate. */
      if (IS_UNCOLLECTABLE(kind)) GC_set_hdr_marks(HDR(h));

  /* Build the free list */
      GC_obj_kinds[kind].ok_freelist[gran] =
	GC_build_fl(h, GRANULES_TO_WORDS(gran), clear,
		    GC_obj_kinds[kind].ok_freelist[gran]);
}

