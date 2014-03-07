/*
 * Copyright 1988, 1989 Hans-J. Boehm, Alan J. Demers
 * Copyright (c) 1991-1994 by Xerox Corporation.  All rights reserved.
 * Copyright (c) 1996 by Silicon Graphics.  All rights reserved.
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
 */

/*
 * These are extra allocation routines which are likely to be less
 * frequently used than those in malloc.c.  They are separate in the
 * hope that the .o file will be excluded from statically linked
 * executables.  We should probably break this up further.
 */

#include <stdio.h>
#include "private/gc_priv.h"

/* extern ptr_t GC_clear_stack();  /* in misc.c, behaves like identity */
void GC_extend_size_map();      /* in misc.c. */
GC_bool GC_alloc_reclaim_list();	/* in malloc.c */

/* Some externally visible but unadvertised variables to allow access to */
/* free lists from inlined allocators without including gc_priv.h	 */
/* or introducing dependencies on internal data structure layouts.	 */
void ** const GC_objfreelist_ptr = GC_objfreelist;
void ** const GC_aobjfreelist_ptr = GC_aobjfreelist;
void ** const GC_uobjfreelist_ptr = GC_uobjfreelist;
# ifdef ATOMIC_UNCOLLECTABLE
    void ** const GC_auobjfreelist_ptr = GC_auobjfreelist;
# endif


void * GC_generic_or_special_malloc(size_t lb, int knd)
{
    switch(knd) {
#     ifdef STUBBORN_ALLOC
	case STUBBORN:
	    return(GC_malloc_stubborn((size_t)lb));
#     endif
	case PTRFREE:
	    return(GC_malloc_atomic((size_t)lb));
	case NORMAL:
	    return(GC_malloc((size_t)lb));
	case UNCOLLECTABLE:
	    return(GC_malloc_uncollectable((size_t)lb));
#       ifdef ATOMIC_UNCOLLECTABLE
	  case AUNCOLLECTABLE:
	    return(GC_malloc_atomic_uncollectable((size_t)lb));
#	endif /* ATOMIC_UNCOLLECTABLE */
	default:
	    return(GC_generic_malloc(lb,knd));
    }
}


/* Change the size of the block pointed to by p to contain at least   */
/* lb bytes.  The object may be (and quite likely will be) moved.     */
/* The kind (e.g. atomic) is the same as that of the old.	      */
/* Shrinking of large blocks is not implemented well.                 */
void * GC_realloc(void * p, size_t lb)
{
    struct hblk * h;
    hdr * hhdr;
    size_t sz;	 /* Current size in bytes	*/
    size_t orig_sz;	 /* Original sz in bytes	*/
    int obj_kind;

    if (p == 0) return(GC_malloc(lb));	/* Required by ANSI */
    h = HBLKPTR(p);
    hhdr = HDR(h);
    sz = hhdr -> hb_sz;
    obj_kind = hhdr -> hb_obj_kind;
    orig_sz = sz;

    if (sz > MAXOBJBYTES) {
	/* Round it up to the next whole heap block */
	  register word descr;
	  
	  sz = (sz+HBLKSIZE-1) & (~HBLKMASK);
	  hhdr -> hb_sz = sz;
	  descr = GC_obj_kinds[obj_kind].ok_descriptor;
          if (GC_obj_kinds[obj_kind].ok_relocate_descr) descr += sz;
          hhdr -> hb_descr = descr;
#	  ifdef MARK_BIT_PER_OBJ
	    GC_ASSERT(hhdr -> hb_inv_sz == LARGE_INV_SZ);
#	  else
	    GC_ASSERT(hhdr -> hb_large_block &&
		      hhdr -> hb_map[ANY_INDEX] == 1);
#	  endif
	  if (IS_UNCOLLECTABLE(obj_kind)) GC_non_gc_bytes += (sz - orig_sz);
	  /* Extra area is already cleared by GC_alloc_large_and_clear. */
    }
    if (ADD_SLOP(lb) <= sz) {
	if (lb >= (sz >> 1)) {
#	    ifdef STUBBORN_ALLOC
	        if (obj_kind == STUBBORN) GC_change_stubborn(p);
#	    endif
	    if (orig_sz > lb) {
	      /* Clear unneeded part of object to avoid bogus pointer */
	      /* tracing.					      */
	      /* Safe for stubborn objects.			      */
	        BZERO(((ptr_t)p) + lb, orig_sz - lb);
	    }
	    return(p);
	} else {
	    /* shrink */
	      void * result =
	      		GC_generic_or_special_malloc((word)lb, obj_kind);

	      if (result == 0) return(0);
	          /* Could also return original object.  But this 	*/
	          /* gives the client warning of imminent disaster.	*/
	      BCOPY(p, result, lb);
#	      ifndef IGNORE_FREE
	        GC_free(p);
#	      endif
	      return(result);
	}
    } else {
	/* grow */
	  void * result =
	  	GC_generic_or_special_malloc((word)lb, obj_kind);

	  if (result == 0) return(0);
	  BCOPY(p, result, sz);
#	  ifndef IGNORE_FREE
	    GC_free(p);
#	  endif
	  return(result);
    }
}

# if defined(REDIRECT_MALLOC) && !defined(REDIRECT_REALLOC)
#   define REDIRECT_REALLOC GC_realloc
# endif

# ifdef REDIRECT_REALLOC

/* As with malloc, avoid two levels of extra calls here.	*/
# ifdef GC_ADD_CALLER
#   define RA GC_RETURN_ADDR,
# else
#   define RA
# endif
# define GC_debug_realloc_replacement(p, lb) \
	GC_debug_realloc(p, lb, RA "unknown", 0)

void * realloc(void * p, size_t lb)
  {
    return(REDIRECT_REALLOC(p, lb));
  }

# undef GC_debug_realloc_replacement
# endif /* REDIRECT_REALLOC */


/* Allocate memory such that only pointers to near the          */
/* beginning of the object are considered.                      */
/* We avoid holding allocation lock while we clear memory.	*/
void * GC_generic_malloc_ignore_off_page(size_t lb, int k)
{
    void *result;
    size_t lw;
    size_t lb_rounded;
    word n_blocks;
    GC_bool init;
    DCL_LOCK_STATE;
    
    if (SMALL_OBJ(lb))
        return(GC_generic_malloc((word)lb, k));
    lw = ROUNDED_UP_WORDS(lb);
    lb_rounded = WORDS_TO_BYTES(lw);
    n_blocks = OBJ_SZ_TO_BLOCKS(lb_rounded);
    init = GC_obj_kinds[k].ok_init;
    if (GC_have_errors) GC_print_all_errors();
    GC_INVOKE_FINALIZERS();
    LOCK();
    result = (ptr_t)GC_alloc_large(ADD_SLOP(lb), k, IGNORE_OFF_PAGE);
    if (0 != result) {
        if (GC_debugging_started) {
	    BZERO(result, n_blocks * HBLKSIZE);
        } else {
#           ifdef THREADS
	      /* Clear any memory that might be used for GC descriptors */
	      /* before we release the lock.			      */
	        ((word *)result)[0] = 0;
	        ((word *)result)[1] = 0;
	        ((word *)result)[lw-1] = 0;
	        ((word *)result)[lw-2] = 0;
#	    endif
        }
    }
    GC_bytes_allocd += lb_rounded;
    UNLOCK();
    if (0 == result) {
        return((*GC_oom_fn)(lb));
    } else {
    	if (init && !GC_debugging_started) {
	    BZERO(result, n_blocks * HBLKSIZE);
        }
        return(result);
    }
}

void * GC_malloc_ignore_off_page(size_t lb)
{
    return((void *)GC_generic_malloc_ignore_off_page(lb, NORMAL));
}

void * GC_malloc_atomic_ignore_off_page(size_t lb)
{
    return((void *)GC_generic_malloc_ignore_off_page(lb, PTRFREE));
}

/* Increment GC_bytes_allocd from code that doesn't have direct access 	*/
/* to GC_arrays.							*/
void GC_incr_bytes_allocd(size_t n)
{
    GC_bytes_allocd += n;
}

/* The same for GC_bytes_freed.				*/
void GC_incr_bytes_freed(size_t n)
{
    GC_bytes_freed += n;
}

#if defined(THREADS)

extern signed_word GC_bytes_found;   /* Protected by GC lock.  */

#ifdef PARALLEL_MARK
volatile signed_word GC_bytes_allocd_tmp = 0;
                        /* Number of bytes of memory allocated since    */
                        /* we released the GC lock.  Instead of         */
                        /* reacquiring the GC lock just to add this in, */
                        /* we add it in the next time we reacquire      */
                        /* the lock.  (Atomically adding it doesn't     */
                        /* work, since we would have to atomically      */
                        /* update it in GC_malloc, which is too         */
                        /* expensive.)                                   */
#endif /* PARALLEL_MARK */

/* Return a list of 1 or more objects of the indicated size, linked	*/
/* through the first word in the object.  This has the advantage that	*/
/* it acquires the allocation lock only once, and may greatly reduce	*/
/* time wasted contending for the allocation lock.  Typical usage would */
/* be in a thread that requires many items of the same size.  It would	*/
/* keep its own free list in thread-local storage, and call		*/
/* GC_malloc_many or friends to replenish it.  (We do not round up	*/
/* object sizes, since a call indicates the intention to consume many	*/
/* objects of exactly this size.)					*/
/* We assume that the size is a multiple of GRANULE_BYTES.		*/
/* We return the free-list by assigning it to *result, since it is	*/
/* not safe to return, e.g. a linked list of pointer-free objects,	*/
/* since the collector would not retain the entire list if it were 	*/
/* invoked just as we were returning.					*/
/* Note that the client should usually clear the link field.		*/
void GC_generic_malloc_many(size_t lb, int k, void **result)
{
void *op;
void *p;
void **opp;
size_t lw;	/* Length in words.	*/
size_t lg;	/* Length in granules.	*/
signed_word my_bytes_allocd = 0;
struct obj_kind * ok = &(GC_obj_kinds[k]);
DCL_LOCK_STATE;

    GC_ASSERT((lb & (GRANULE_BYTES-1)) == 0);
    if (!SMALL_OBJ(lb)) {
        op = GC_generic_malloc(lb, k);
        if(0 != op) obj_link(op) = 0;
	*result = op;
        return;
    }
    lw = BYTES_TO_WORDS(lb);
    lg = BYTES_TO_GRANULES(lb);
    if (GC_have_errors) GC_print_all_errors();
    GC_INVOKE_FINALIZERS();
    LOCK();
    if (!GC_is_initialized) GC_init_inner();
    /* Do our share of marking work */
      if (GC_incremental && !GC_dont_gc) {
        ENTER_GC();
	GC_collect_a_little_inner(1);
        EXIT_GC();
      }
    /* First see if we can reclaim a page of objects waiting to be */
    /* reclaimed.						   */
    {
	struct hblk ** rlh = ok -> ok_reclaim_list;
	struct hblk * hbp;
	hdr * hhdr;

	rlh += lg;
    	while ((hbp = *rlh) != 0) {
            hhdr = HDR(hbp);
            *rlh = hhdr -> hb_next;
	    GC_ASSERT(hhdr -> hb_sz == lb);
	    hhdr -> hb_last_reclaimed = (unsigned short) GC_gc_no;
#	    ifdef PARALLEL_MARK
		{
		  signed_word my_bytes_allocd_tmp = GC_bytes_allocd_tmp;

		  GC_ASSERT(my_bytes_allocd_tmp >= 0);
		  /* We only decrement it while holding the GC lock.	*/
		  /* Thus we can't accidentally adjust it down in more	*/
		  /* than one thread simultaneously.			*/
		  if (my_bytes_allocd_tmp != 0) {
		    (void)AO_fetch_and_add(
				(volatile AO_t *)(&GC_bytes_allocd_tmp),
				(AO_t)(-my_bytes_allocd_tmp));
		    GC_bytes_allocd += my_bytes_allocd_tmp;
		  }
		}
		GC_acquire_mark_lock();
		++ GC_fl_builder_count;
		UNLOCK();
		GC_release_mark_lock();
#	    endif
	    op = GC_reclaim_generic(hbp, hhdr, lb,
				    ok -> ok_init, 0, &my_bytes_allocd);
            if (op != 0) {
	      /* We also reclaimed memory, so we need to adjust 	*/
	      /* that count.						*/
	      /* This should be atomic, so the results may be		*/
	      /* inaccurate.						*/
	      GC_bytes_found += my_bytes_allocd;
#	      ifdef PARALLEL_MARK
		*result = op;
		(void)AO_fetch_and_add(
				(volatile AO_t *)(&GC_bytes_allocd_tmp),
				(AO_t)(my_bytes_allocd));
		GC_acquire_mark_lock();
		-- GC_fl_builder_count;
		if (GC_fl_builder_count == 0) GC_notify_all_builder();
		GC_release_mark_lock();
		(void) GC_clear_stack(0);
		return;
#	      else
	        GC_bytes_allocd += my_bytes_allocd;
	        goto out;
#	      endif
	    }
#	    ifdef PARALLEL_MARK
	      GC_acquire_mark_lock();
	      -- GC_fl_builder_count;
	      if (GC_fl_builder_count == 0) GC_notify_all_builder();
	      GC_release_mark_lock();
	      LOCK();
	      /* GC lock is needed for reclaim list access.	We	*/
	      /* must decrement fl_builder_count before reaquiring GC	*/
	      /* lock.  Hopefully this path is rare.			*/
#	    endif
    	}
    }
    /* Next try to use prefix of global free list if there is one.	*/
    /* We don't refill it, but we need to use it up before allocating	*/
    /* a new block ourselves.						*/
      opp = &(GC_obj_kinds[k].ok_freelist[lg]);
      if ( (op = *opp) != 0 ) {
	*opp = 0;
        my_bytes_allocd = 0;
        for (p = op; p != 0; p = obj_link(p)) {
          my_bytes_allocd += lb;
          if (my_bytes_allocd >= HBLKSIZE) {
            *opp = obj_link(p);
            obj_link(p) = 0;
            break;
	  }
        }
	GC_bytes_allocd += my_bytes_allocd;
	goto out;
      }
    /* Next try to allocate a new block worth of objects of this size.	*/
    {
	struct hblk *h = GC_allochblk(lb, k, 0);
	if (h != 0) {
	  if (IS_UNCOLLECTABLE(k)) GC_set_hdr_marks(HDR(h));
	  GC_bytes_allocd += HBLKSIZE - HBLKSIZE % lb;
#	  ifdef PARALLEL_MARK
	    GC_acquire_mark_lock();
	    ++ GC_fl_builder_count;
	    UNLOCK();
	    GC_release_mark_lock();
#	  endif

	  op = GC_build_fl(h, lw, ok -> ok_init, 0);
#	  ifdef PARALLEL_MARK
	    *result = op;
	    GC_acquire_mark_lock();
	    -- GC_fl_builder_count;
	    if (GC_fl_builder_count == 0) GC_notify_all_builder();
	    GC_release_mark_lock();
	    (void) GC_clear_stack(0);
	    return;
#	  else
	    goto out;
#	  endif
	}
    }
    
    /* As a last attempt, try allocating a single object.  Note that	*/
    /* this may trigger a collection or expand the heap.		*/
      op = GC_generic_malloc_inner(lb, k);
      if (0 != op) obj_link(op) = 0;
    
  out:
    *result = op;
    UNLOCK();
    (void) GC_clear_stack(0);
}

void * GC_malloc_many(size_t lb)
{
    void *result;
    GC_generic_malloc_many(((lb + EXTRA_BYTES + GRANULE_BYTES-1)
			   & ~(GRANULE_BYTES-1)),
	    		   NORMAL, &result);
    return result;
}

/* Note that the "atomic" version of this would be unsafe, since the	*/
/* links would not be seen by the collector.				*/
# endif

/* Allocate lb bytes of pointerful, traced, but not collectable data */
void * GC_malloc_uncollectable(size_t lb)
{
    void *op;
    void **opp;
    size_t lg;
    DCL_LOCK_STATE;

    if( SMALL_OBJ(lb) ) {
	if (EXTRA_BYTES != 0 && lb != 0) lb--;
	    	  /* We don't need the extra byte, since this won't be	*/
	    	  /* collected anyway.					*/
	lg = GC_size_map[lb];
	opp = &(GC_uobjfreelist[lg]);
	LOCK();
        if( (op = *opp) != 0 ) {
            /* See above comment on signals.	*/
            *opp = obj_link(op);
            obj_link(op) = 0;
            GC_bytes_allocd += GRANULES_TO_BYTES(lg);
            /* Mark bit ws already set on free list.  It will be	*/
	    /* cleared only temporarily during a collection, as a 	*/
	    /* result of the normal free list mark bit clearing.	*/
            GC_non_gc_bytes += GRANULES_TO_BYTES(lg);
            UNLOCK();
        } else {
            UNLOCK();
            op = (ptr_t)GC_generic_malloc((word)lb, UNCOLLECTABLE);
	    /* For small objects, the free lists are completely marked. */
	}
	GC_ASSERT(0 == op || GC_is_marked(op));
        return((void *) op);
    } else {
	hdr * hhdr;
	
	op = (ptr_t)GC_generic_malloc((word)lb, UNCOLLECTABLE);
        if (0 == op) return(0);
	
	GC_ASSERT(((word)op & (HBLKSIZE - 1)) == 0); /* large block */
	hhdr = HDR((struct hbklk *)op);
	/* We don't need the lock here, since we have an undisguised 	*/
	/* pointer.  We do need to hold the lock while we adjust	*/
	/* mark bits.							*/
	lb = hhdr -> hb_sz;
	LOCK();
	set_mark_bit_from_hdr(hhdr, 0);	/* Only object.	*/
	GC_ASSERT(hhdr -> hb_n_marks == 0);
	hhdr -> hb_n_marks = 1;
	UNLOCK();
	return((void *) op);
    }
}

/* Not well tested nor integrated.	*/
/* Debug version is tricky and currently missing.	*/
#include <limits.h>

void * GC_memalign(size_t align, size_t lb) 
{ 
    size_t new_lb;
    size_t offset;
    ptr_t result;

    if (align <= GRANULE_BYTES) return GC_malloc(lb);
    if (align >= HBLKSIZE/2 || lb >= HBLKSIZE/2) {
        if (align > HBLKSIZE) return GC_oom_fn(LONG_MAX-1024) /* Fail */;
	return GC_malloc(lb <= HBLKSIZE? HBLKSIZE : lb);
	    /* Will be HBLKSIZE aligned.	*/
    }
    /* We could also try to make sure that the real rounded-up object size */
    /* is a multiple of align.  That would be correct up to HBLKSIZE.	   */
    new_lb = lb + align - 1;
    result = GC_malloc(new_lb);
    offset = (word)result % align;
    if (offset != 0) {
	offset = align - offset;
        if (!GC_all_interior_pointers) {
	    if (offset >= VALID_OFFSET_SZ) return GC_malloc(HBLKSIZE);
	    GC_register_displacement(offset);
	}
    }
    result = (void *) ((ptr_t)result + offset);
    GC_ASSERT((word)result % align == 0);
    return result;
}

# ifdef ATOMIC_UNCOLLECTABLE
/* Allocate lb bytes of pointerfree, untraced, uncollectable data 	*/
/* This is normally roughly equivalent to the system malloc.		*/
/* But it may be useful if malloc is redefined.				*/
void * GC_malloc_atomic_uncollectable(size_t lb)
{
    void *op;
    void **opp;
    size_t lg;
    DCL_LOCK_STATE;

    if( SMALL_OBJ(lb) ) {
	if (EXTRA_BYTES != 0 && lb != 0) lb--;
	    	  /* We don't need the extra byte, since this won't be	*/
	    	  /* collected anyway.					*/
	lg = GC_size_map[lb];
	opp = &(GC_auobjfreelist[lg]);
	LOCK();
        if( (op = *opp) != 0 ) {
            /* See above comment on signals.	*/
            *opp = obj_link(op);
            obj_link(op) = 0;
            GC_bytes_allocd += GRANULES_TO_BYTES(lg);
	    /* Mark bit was already set while object was on free list. */
            GC_non_gc_bytes += GRANULES_TO_BYTES(lg);
            UNLOCK();
        } else {
            UNLOCK();
            op = (ptr_t)GC_generic_malloc(lb, AUNCOLLECTABLE);
	}
	GC_ASSERT(0 == op || GC_is_marked(op));
        return((void *) op);
    } else {
	hdr * hhdr;
	
	op = (ptr_t)GC_generic_malloc(lb, AUNCOLLECTABLE);
        if (0 == op) return(0);

	GC_ASSERT(((word)op & (HBLKSIZE - 1)) == 0);
	hhdr = HDR((struct hbklk *)op);
	lb = hhdr -> hb_sz;
	
	LOCK();
	set_mark_bit_from_hdr(hhdr, 0);	/* Only object.	*/
	GC_ASSERT(hhdr -> hb_n_marks == 0);
	hhdr -> hb_n_marks = 1;
	UNLOCK();
	return((void *) op);
    }
}

#endif /* ATOMIC_UNCOLLECTABLE */
