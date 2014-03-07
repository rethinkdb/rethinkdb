/* 
 * Copyright (c) 1991-1994 by Xerox Corporation.  All rights reserved.
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
 * These are checking routines calls to which could be inserted by a
 * preprocessor to validate C pointer arithmetic.
 */

#include "private/gc_pmark.h"

void GC_default_same_obj_print_proc(void * p, void * q)
{
    GC_err_printf("%p and %p are not in the same object\n", p, q);
    ABORT("GC_same_obj test failed");
}

void (*GC_same_obj_print_proc) (void *, void *)
		= GC_default_same_obj_print_proc;

/* Check that p and q point to the same object.  Call		*/
/* *GC_same_obj_print_proc if they don't.			*/
/* Returns the first argument.  (Return value may be hard 	*/
/* to use,due to typing issues.  But if we had a suitable 	*/
/* preprocessor ...)						*/
/* Succeeds if neither p nor q points to the heap.		*/
/* We assume this is performance critical.  (It shouldn't	*/
/* be called by production code, but this can easily make	*/
/* debugging intolerably slow.)					*/
void * GC_same_obj(void *p, void *q)
{
    struct hblk *h;
    hdr *hhdr;
    ptr_t base, limit;
    word sz;
    
    if (!GC_is_initialized) GC_init();
    hhdr = HDR((word)p);
    if (hhdr == 0) {
   	if (divHBLKSZ((word)p) != divHBLKSZ((word)q)
   	    && HDR((word)q) != 0) {
   	    goto fail;
   	}
   	return(p);
    }
    /* If it's a pointer to the middle of a large object, move it	*/
    /* to the beginning.						*/
    if (IS_FORWARDING_ADDR_OR_NIL(hhdr)) {
    	h = HBLKPTR(p) - (word)hhdr;
    	hhdr = HDR(h);
	while (IS_FORWARDING_ADDR_OR_NIL(hhdr)) {
	   h = FORWARDED_ADDR(h, hhdr);
	   hhdr = HDR(h);
	}
	limit = (ptr_t)h + hhdr -> hb_sz;
	if ((ptr_t)p >= limit || (ptr_t)q >= limit || (ptr_t)q < (ptr_t)h ) {
	    goto fail;
	}
	return(p);
    }
    sz = hhdr -> hb_sz;
    if (sz > MAXOBJBYTES) {
      base = (ptr_t)HBLKPTR(p);
      limit = base + sz;
      if ((ptr_t)p >= limit) {
        goto fail;
      }
    } else {
      size_t offset;
      size_t pdispl = HBLKDISPL(p);
      
      offset = pdispl % sz;
      if (HBLKPTR(p) != HBLKPTR(q)) goto fail;
	 	/* W/o this check, we might miss an error if 	*/
	 	/* q points to the first object on a page, and	*/
	 	/* points just before the page.			*/
      base = (ptr_t)p - offset;
      limit = base + sz;
    }
    /* [base, limit) delimits the object containing p, if any.	*/
    /* If p is not inside a valid object, then either q is	*/
    /* also outside any valid object, or it is outside 		*/
    /* [base, limit).						*/
    if ((ptr_t)q >= limit || (ptr_t)q < base) {
    	goto fail;
    }
    return(p);
fail:
    (*GC_same_obj_print_proc)((ptr_t)p, (ptr_t)q);
    return(p);
}

void GC_default_is_valid_displacement_print_proc (void *p)
{
    GC_err_printf("%p does not point to valid object displacement\n", p);
    ABORT("GC_is_valid_displacement test failed");
}

void (*GC_is_valid_displacement_print_proc)(void *) = 
	GC_default_is_valid_displacement_print_proc;

/* Check that if p is a pointer to a heap page, then it points to	*/
/* a valid displacement within a heap object.				*/
/* Uninteresting with GC_all_interior_pointers.				*/
/* Always returns its argument.						*/
/* Note that we don't lock, since nothing relevant about the header	*/
/* should change while we have a valid object pointer to the block.	*/
void * GC_is_valid_displacement(void *p)
{
    hdr *hhdr;
    word pdispl;
    word offset;
    struct hblk *h;
    word sz;
    
    if (!GC_is_initialized) GC_init();
    hhdr = HDR((word)p);
    if (hhdr == 0) return(p);
    h = HBLKPTR(p);
    if (GC_all_interior_pointers) {
	while (IS_FORWARDING_ADDR_OR_NIL(hhdr)) {
	   h = FORWARDED_ADDR(h, hhdr);
	   hhdr = HDR(h);
	}
    }
    if (IS_FORWARDING_ADDR_OR_NIL(hhdr)) {
    	goto fail;
    }
    sz = hhdr -> hb_sz;
    pdispl = HBLKDISPL(p);
    offset = pdispl % sz;
    if ((sz > MAXOBJBYTES && (ptr_t)p >= (ptr_t)h + sz)
	|| !GC_valid_offsets[offset]
	|| (ptr_t)p - offset + sz > (ptr_t)(h + 1)) {
    	goto fail;
    }
    return(p);
fail:
    (*GC_is_valid_displacement_print_proc)((ptr_t)p);
    return(p);
}

void GC_default_is_visible_print_proc(void * p)
{
    GC_err_printf("%p is not a GC visible pointer location\n", p);
    ABORT("GC_is_visible test failed");
}

void (*GC_is_visible_print_proc)(void * p) = GC_default_is_visible_print_proc;

/* Could p be a stack address? */
GC_bool GC_on_stack(ptr_t p)
{
#   ifdef THREADS
	return(TRUE);
#   else
	int dummy;
#   	ifdef STACK_GROWS_DOWN
	    if ((ptr_t)p >= (ptr_t)(&dummy) && (ptr_t)p < GC_stackbottom ) {
	    	return(TRUE);
	    }
#	else
	    if ((ptr_t)p <= (ptr_t)(&dummy) && (ptr_t)p > GC_stackbottom ) {
	    	return(TRUE);
	    }
#	endif
	return(FALSE);
#   endif
}

/* Check that p is visible						*/
/* to the collector as a possibly pointer containing location.		*/
/* If it isn't invoke *GC_is_visible_print_proc.			*/
/* Returns the argument in all cases.  May erroneously succeed		*/
/* in hard cases.  (This is intended for debugging use with		*/
/* untyped allocations.  The idea is that it should be possible, though	*/
/* slow, to add such a call to all indirect pointer stores.)		*/
/* Currently useless for multithreaded worlds.				*/
void * GC_is_visible(void *p)
{
    hdr *hhdr;
    
    if ((word)p & (ALIGNMENT - 1)) goto fail;
    if (!GC_is_initialized) GC_init();
#   ifdef THREADS
	hhdr = HDR((word)p);
        if (hhdr != 0 && GC_base(p) == 0) {
            goto fail;
        } else {
            /* May be inside thread stack.  We can't do much. */
            return(p);
        }
#   else
	/* Check stack first: */
	  if (GC_on_stack(p)) return(p);
	hhdr = HDR((word)p);
    	if (hhdr == 0) {
    	    GC_bool result;
    	    
    	    if (GC_is_static_root(p)) return(p);
    	    /* Else do it again correctly:	*/
#           if (defined(DYNAMIC_LOADING) || defined(MSWIN32) || \
		defined(MSWINCE) || defined(PCR))
    	        GC_register_dynamic_libraries();
    	        result = GC_is_static_root(p);
    	        if (result) return(p);
#	    endif
    	    goto fail;
    	} else {
    	    /* p points to the heap. */
    	    word descr;
    	    ptr_t base = GC_base(p);	/* Should be manually inlined? */
    	    
    	    if (base == 0) goto fail;
    	    if (HBLKPTR(base) != HBLKPTR(p)) hhdr = HDR((word)p);
    	    descr = hhdr -> hb_descr;
    retry:
    	    switch(descr & GC_DS_TAGS) {
    	        case GC_DS_LENGTH:
    	            if ((word)((ptr_t)p - (ptr_t)base) > (word)descr) goto fail;
    	            break;
    	        case GC_DS_BITMAP:
    	            if ((ptr_t)p - (ptr_t)base
    	                 >= WORDS_TO_BYTES(BITMAP_BITS)
    	                 || ((word)p & (sizeof(word) - 1))) goto fail;
    	            if (!((1 << (WORDSZ - ((ptr_t)p - (ptr_t)base) - 1))
    	            	  & descr)) goto fail;
    	            break;
    	        case GC_DS_PROC:
    	            /* We could try to decipher this partially. 	*/
    	            /* For now we just punt.				*/
    	            break;
    	        case GC_DS_PER_OBJECT:
		    if ((signed_word)descr >= 0) {
    	              descr = *(word *)((ptr_t)base + (descr & ~GC_DS_TAGS));
		    } else {
		      ptr_t type_descr = *(ptr_t *)base;
		      descr = *(word *)(type_descr
			      - (descr - (GC_DS_PER_OBJECT
					  - GC_INDIR_PER_OBJ_BIAS)));
		    }
    	            goto retry;
    	    }
    	    return(p);
    	}
#   endif
fail:
    (*GC_is_visible_print_proc)((ptr_t)p);
    return(p);
}


void * GC_pre_incr (void **p, size_t how_much)
{
    void * initial = *p;
    void * result = GC_same_obj((void *)((word)initial + how_much), initial);
    
    if (!GC_all_interior_pointers) {
    	(void) GC_is_valid_displacement(result);
    }
    return (*p = result);
}

void * GC_post_incr (void **p, size_t how_much)
{
    void * initial = *p;
    void * result = GC_same_obj((void *)((word)initial + how_much), initial);
 
    if (!GC_all_interior_pointers) {
    	(void) GC_is_valid_displacement(result);
    }
    *p = result;
    return(initial);
}
