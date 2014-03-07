/* 
 * Copyright 1988, 1989 Hans-J. Boehm, Alan J. Demers
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
/* Boehm, August 9, 1995 6:09 pm PDT */
# include "private/gc_priv.h"

/*
 * We maintain several hash tables of hblks that have had false hits.
 * Each contains one bit per hash bucket;  If any page in the bucket
 * has had a false hit, we assume that all of them have.
 * See the definition of page_hash_table in gc_private.h.
 * False hits from the stack(s) are much more dangerous than false hits
 * from elsewhere, since the former can pin a large object that spans the
 * block, eventhough it does not start on the dangerous block.
 */
 
/*
 * Externally callable routines are:
 
 * GC_add_to_black_list_normal
 * GC_add_to_black_list_stack
 * GC_promote_black_lists
 * GC_is_black_listed
 *
 * All require that the allocator lock is held.
 */

/* Pointers to individual tables.  We replace one table by another by 	*/
/* switching these pointers. 						*/
word * GC_old_normal_bl;
		/* Nonstack false references seen at last full		*/
		/* collection.						*/
word * GC_incomplete_normal_bl;
		/* Nonstack false references seen since last		*/
		/* full collection.					*/
word * GC_old_stack_bl;
word * GC_incomplete_stack_bl;

word GC_total_stack_black_listed;

word GC_black_list_spacing = MINHINCR*HBLKSIZE;  /* Initial rough guess */

void GC_clear_bl(word *);

void GC_default_print_heap_obj_proc(ptr_t p)
{
    ptr_t base = GC_base(p);

    GC_err_printf("start: %p, appr. length: %ld", base,
		  (unsigned long)GC_size(base));
}

void (*GC_print_heap_obj) (ptr_t p) = GC_default_print_heap_obj_proc;

void GC_print_source_ptr(ptr_t p)
{
    ptr_t base = GC_base(p);
    if (0 == base) {
	if (0 == p) {
	    GC_err_printf("in register");
	} else {
	    GC_err_printf("in root set");
	}
    } else {
	GC_err_printf("in object at ");
	(*GC_print_heap_obj)(base);
    }
}

void GC_bl_init(void)
{
    if (!GC_all_interior_pointers) {
      GC_old_normal_bl = (word *)
    			 GC_scratch_alloc((word)(sizeof (page_hash_table)));
      GC_incomplete_normal_bl = (word *)GC_scratch_alloc
    					((word)(sizeof(page_hash_table)));
      if (GC_old_normal_bl == 0 || GC_incomplete_normal_bl == 0) {
        GC_err_printf("Insufficient memory for black list\n");
        EXIT();
      }
      GC_clear_bl(GC_old_normal_bl);
      GC_clear_bl(GC_incomplete_normal_bl);
    }
    GC_old_stack_bl = (word *)GC_scratch_alloc((word)(sizeof(page_hash_table)));
    GC_incomplete_stack_bl = (word *)GC_scratch_alloc
    					((word)(sizeof(page_hash_table)));
    if (GC_old_stack_bl == 0 || GC_incomplete_stack_bl == 0) {
        GC_err_printf("Insufficient memory for black list\n");
        EXIT();
    }
    GC_clear_bl(GC_old_stack_bl);
    GC_clear_bl(GC_incomplete_stack_bl);
}
		
void GC_clear_bl(word *doomed)
{
    BZERO(doomed, sizeof(page_hash_table));
}

void GC_copy_bl(word *old, word *new)
{
    BCOPY(old, new, sizeof(page_hash_table));
}

static word total_stack_black_listed(void);

/* Signal the completion of a collection.  Turn the incomplete black	*/
/* lists into new black lists, etc.					*/			 
void GC_promote_black_lists(void)
{
    word * very_old_normal_bl = GC_old_normal_bl;
    word * very_old_stack_bl = GC_old_stack_bl;
    
    GC_old_normal_bl = GC_incomplete_normal_bl;
    GC_old_stack_bl = GC_incomplete_stack_bl;
    if (!GC_all_interior_pointers) {
      GC_clear_bl(very_old_normal_bl);
    }
    GC_clear_bl(very_old_stack_bl);
    GC_incomplete_normal_bl = very_old_normal_bl;
    GC_incomplete_stack_bl = very_old_stack_bl;
    GC_total_stack_black_listed = total_stack_black_listed();
    if (GC_print_stats == VERBOSE)
  	GC_log_printf("%ld bytes in heap blacklisted for interior pointers\n",
  		      (unsigned long)GC_total_stack_black_listed);
    if (GC_total_stack_black_listed != 0) {
        GC_black_list_spacing =
		HBLKSIZE*(GC_heapsize/GC_total_stack_black_listed);
    }
    if (GC_black_list_spacing < 3 * HBLKSIZE) {
    	GC_black_list_spacing = 3 * HBLKSIZE;
    }
    if (GC_black_list_spacing > MAXHINCR * HBLKSIZE) {
	GC_black_list_spacing = MAXHINCR * HBLKSIZE;
	/* Makes it easier to allocate really huge blocks, which otherwise */
	/* may have problems with nonuniform blacklist distributions.	   */
	/* This way we should always succeed immediately after growing the */ 
	/* heap.							   */
    }
}

void GC_unpromote_black_lists(void)
{
    if (!GC_all_interior_pointers) {
      GC_copy_bl(GC_old_normal_bl, GC_incomplete_normal_bl);
    }
    GC_copy_bl(GC_old_stack_bl, GC_incomplete_stack_bl);
}

/* P is not a valid pointer reference, but it falls inside	*/
/* the plausible heap bounds.					*/
/* Add it to the normal incomplete black list if appropriate.	*/
#ifdef PRINT_BLACK_LIST
  void GC_add_to_black_list_normal(word p, ptr_t source)
#else
  void GC_add_to_black_list_normal(word p)
#endif
{
    if (!(GC_modws_valid_offsets[p & (sizeof(word)-1)])) return;
    {
        word index = PHT_HASH((word)p);
        
        if (HDR(p) == 0 || get_pht_entry_from_index(GC_old_normal_bl, index)) {
#   	    ifdef PRINT_BLACK_LIST
		if (!get_pht_entry_from_index(GC_incomplete_normal_bl, index)) {
	    	  GC_err_printf(
			"Black listing (normal) %p referenced from %p ",
	    	        (ptr_t) p, source);
		  GC_print_source_ptr(source);
		  GC_err_puts("\n");
	    	}
#           endif
            set_pht_entry_from_index(GC_incomplete_normal_bl, index);
        } /* else this is probably just an interior pointer to an allocated */
          /* object, and isn't worth black listing.			    */
    }
}

/* And the same for false pointers from the stack. */
#ifdef PRINT_BLACK_LIST
  void GC_add_to_black_list_stack(word p, ptr_t source)
  ptr_t source;
#else
  void GC_add_to_black_list_stack(word p)
#endif
{
    word index = PHT_HASH((word)p);
        
    if (HDR(p) == 0 || get_pht_entry_from_index(GC_old_stack_bl, index)) {
#   	ifdef PRINT_BLACK_LIST
	    if (!get_pht_entry_from_index(GC_incomplete_stack_bl, index)) {
	    	  GC_err_printf(
			"Black listing (stack) %p referenced from %p ",
	    	        (ptr_t)p, source);
		  GC_print_source_ptr(source);
		  GC_err_puts("\n");
	    }
#       endif
	set_pht_entry_from_index(GC_incomplete_stack_bl, index);
    }
}

/*
 * Is the block starting at h of size len bytes black listed?   If so,
 * return the address of the next plausible r such that (r, len) might not
 * be black listed.  (R may not actually be in the heap.  We guarantee only
 * that every smaller value of r after h is also black listed.)
 * If (h,len) is not black listed, return 0.
 * Knows about the structure of the black list hash tables.
 */
struct hblk * GC_is_black_listed(struct hblk *h, word len)
{
    word index = PHT_HASH((word)h);
    word i;
    word nblocks = divHBLKSZ(len);

    if (!GC_all_interior_pointers) {
      if (get_pht_entry_from_index(GC_old_normal_bl, index)
          || get_pht_entry_from_index(GC_incomplete_normal_bl, index)) {
        return(h+1);
      }
    }
    
    for (i = 0; ; ) {
        if (GC_old_stack_bl[divWORDSZ(index)] == 0
            && GC_incomplete_stack_bl[divWORDSZ(index)] == 0) {
            /* An easy case */
            i += WORDSZ - modWORDSZ(index);
        } else {
          if (get_pht_entry_from_index(GC_old_stack_bl, index)
              || get_pht_entry_from_index(GC_incomplete_stack_bl, index)) {
            return(h+i+1);
          }
          i++;
        }
        if (i >= nblocks) break;
        index = PHT_HASH((word)(h+i));
    }
    return(0);
}


/* Return the number of blacklisted blocks in a given range.	*/
/* Used only for statistical purposes.				*/
/* Looks only at the GC_incomplete_stack_bl.			*/
word GC_number_stack_black_listed(struct hblk *start, struct hblk *endp1)
{
    register struct hblk * h;
    word result = 0;
    
    for (h = start; h < endp1; h++) {
        word index = PHT_HASH((word)h);
        
        if (get_pht_entry_from_index(GC_old_stack_bl, index)) result++;
    }
    return(result);
}


/* Return the total number of (stack) black-listed bytes. */
static word total_stack_black_listed(void)
{
    register unsigned i;
    word total = 0;
    
    for (i = 0; i < GC_n_heap_sects; i++) {
    	struct hblk * start = (struct hblk *) GC_heap_sects[i].hs_start;
    	size_t len = (word) GC_heap_sects[i].hs_bytes;
    	struct hblk * endp1 = start + len/HBLKSIZE;
    	
    	total += GC_number_stack_black_listed(start, endp1);
    }
    return(total * HBLKSIZE);
}

