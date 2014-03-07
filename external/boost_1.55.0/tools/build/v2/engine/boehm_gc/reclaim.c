/* 
 * Copyright 1988, 1989 Hans-J. Boehm, Alan J. Demers
 * Copyright (c) 1991-1996 by Xerox Corporation.  All rights reserved.
 * Copyright (c) 1996-1999 by Silicon Graphics.  All rights reserved.
 * Copyright (c) 1999-2004 Hewlett-Packard Development Company, L.P.
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

#include <stdio.h>
#include "private/gc_priv.h"

signed_word GC_bytes_found = 0;
			/* Number of bytes of memory reclaimed     */

#if defined(PARALLEL_MARK) || defined(THREAD_LOCAL_ALLOC)
  word GC_fl_builder_count = 0;
	/* Number of threads currently building free lists without 	*/
	/* holding GC lock.  It is not safe to collect if this is 	*/
	/* nonzero.							*/
#endif /* PARALLEL_MARK */

/* We defer printing of leaked objects until we're done with the GC	*/
/* cycle, since the routine for printing objects needs to run outside	*/
/* the collector, e.g. without the allocation lock.			*/
#define MAX_LEAKED 40
ptr_t GC_leaked[MAX_LEAKED];
unsigned GC_n_leaked = 0;

GC_bool GC_have_errors = FALSE;

void GC_add_leaked(ptr_t leaked)
{
    if (GC_n_leaked < MAX_LEAKED) {
      GC_have_errors = TRUE;
      GC_leaked[GC_n_leaked++] = leaked;
      /* Make sure it's not reclaimed this cycle */
        GC_set_mark_bit(leaked);
    }
}

static GC_bool printing_errors = FALSE;
/* Print all objects on the list after printing any smashed objs. 	*/
/* Clear both lists.							*/
void GC_print_all_errors ()
{
    unsigned i;

    LOCK();
    if (printing_errors) {
	UNLOCK();
	return;
    }
    printing_errors = TRUE;
    UNLOCK();
    if (GC_debugging_started) GC_print_all_smashed();
    for (i = 0; i < GC_n_leaked; ++i) {
	ptr_t p = GC_leaked[i];
	if (HDR(p) -> hb_obj_kind == PTRFREE) {
	    GC_err_printf("Leaked atomic object at ");
	} else {
	    GC_err_printf("Leaked composite object at ");
	}
	GC_print_heap_obj(p);
	GC_err_printf("\n");
	GC_free(p);
	GC_leaked[i] = 0;
    }
    GC_n_leaked = 0;
    printing_errors = FALSE;
}


/*
 * reclaim phase
 *
 */


/*
 * Test whether a block is completely empty, i.e. contains no marked
 * objects.  This does not require the block to be in physical
 * memory.
 */
 
GC_bool GC_block_empty(hdr *hhdr)
{
    return (hhdr -> hb_n_marks == 0);
}

GC_bool GC_block_nearly_full(hdr *hhdr)
{
    return (hhdr -> hb_n_marks > 7 * HBLK_OBJS(hhdr -> hb_sz)/8);
}

/* FIXME: This should perhaps again be specialized for USE_MARK_BYTES	*/
/* and USE_MARK_BITS cases.						*/

/*
 * Restore unmarked small objects in h of size sz to the object
 * free list.  Returns the new list.
 * Clears unmarked objects.  Sz is in bytes.
 */
/*ARGSUSED*/
ptr_t GC_reclaim_clear(struct hblk *hbp, hdr *hhdr, size_t sz,
		       ptr_t list, signed_word *count)
{
    word bit_no = 0;
    word *p, *q, *plim;
    signed_word n_bytes_found = 0;
    
    GC_ASSERT(hhdr == GC_find_header((ptr_t)hbp));
    GC_ASSERT(sz == hhdr -> hb_sz);
    GC_ASSERT((sz & (BYTES_PER_WORD-1)) == 0);
    p = (word *)(hbp->hb_body);
    plim = (word *)(hbp->hb_body + HBLKSIZE - sz);

    /* go through all words in block */
	while( p <= plim )  {
	    if( mark_bit_from_hdr(hhdr, bit_no) ) {
	        p = (word *)((ptr_t)p + sz);
	    } else {
		n_bytes_found += sz;
		/* object is available - put on list */
		    obj_link(p) = list;
		    list = ((ptr_t)p);
		/* Clear object, advance p to next object in the process */
	            q = (word *)((ptr_t)p + sz);
#		    ifdef USE_MARK_BYTES
		      GC_ASSERT(!(sz & 1)
				&& !((word)p & (2 * sizeof(word) - 1)));
		      p[1] = 0;
                      p += 2;
                      while (p < q) {
			CLEAR_DOUBLE(p);
			p += 2;
		      }
#		    else
                      p++; /* Skip link field */
                      while (p < q) {
			*p++ = 0;
		      }
#		    endif
	    }
	    bit_no += MARK_BIT_OFFSET(sz);
	}
    *count += n_bytes_found;
    return(list);
}

/* The same thing, but don't clear objects: */
/*ARGSUSED*/
ptr_t GC_reclaim_uninit(struct hblk *hbp, hdr *hhdr, size_t sz,
			ptr_t list, signed_word *count)
{
    word bit_no = 0;
    word *p, *plim;
    signed_word n_bytes_found = 0;
    
    GC_ASSERT(sz == hhdr -> hb_sz);
    p = (word *)(hbp->hb_body);
    plim = (word *)((ptr_t)hbp + HBLKSIZE - sz);

    /* go through all words in block */
	while( p <= plim )  {
	    if( !mark_bit_from_hdr(hhdr, bit_no) ) {
		n_bytes_found += sz;
		/* object is available - put on list */
		    obj_link(p) = list;
		    list = ((ptr_t)p);
	    }
	    p = (word *)((ptr_t)p + sz);
	    bit_no += MARK_BIT_OFFSET(sz);
	}
    *count += n_bytes_found;
    return(list);
}

/* Don't really reclaim objects, just check for unmarked ones: */
/*ARGSUSED*/
void GC_reclaim_check(struct hblk *hbp, hdr *hhdr, word sz)
{
    word bit_no = 0;
    ptr_t p, plim;
    
    GC_ASSERT(sz == hhdr -> hb_sz);
    p = hbp->hb_body;
    plim = p + HBLKSIZE - sz;

    /* go through all words in block */
	while( p <= plim )  {
	    if( !mark_bit_from_hdr(hhdr, bit_no) ) {
		GC_add_leaked(p);
	    }
	    p += sz;
	    bit_no += MARK_BIT_OFFSET(sz);
	}
}


/*
 * Generic procedure to rebuild a free list in hbp.
 * Also called directly from GC_malloc_many.
 * Sz is now in bytes.
 */
ptr_t GC_reclaim_generic(struct hblk * hbp, hdr *hhdr, size_t sz,
			 GC_bool init, ptr_t list, signed_word *count)
{
    ptr_t result = list;

    GC_ASSERT(GC_find_header((ptr_t)hbp) == hhdr);
    GC_remove_protection(hbp, 1, (hhdr)->hb_descr == 0 /* Pointer-free? */);
    if (init) {
      result = GC_reclaim_clear(hbp, hhdr, sz, list, count);
    } else {
      GC_ASSERT((hhdr)->hb_descr == 0 /* Pointer-free block */);
      result = GC_reclaim_uninit(hbp, hhdr, sz, list, count);
    } 
    if (IS_UNCOLLECTABLE(hhdr -> hb_obj_kind)) GC_set_hdr_marks(hhdr);
    return result;
}

/*
 * Restore unmarked small objects in the block pointed to by hbp
 * to the appropriate object free list.
 * If entirely empty blocks are to be completely deallocated, then
 * caller should perform that check.
 */
void GC_reclaim_small_nonempty_block(struct hblk *hbp,
				     int report_if_found, signed_word *count)
{
    hdr *hhdr = HDR(hbp);
    size_t sz = hhdr -> hb_sz;
    int kind = hhdr -> hb_obj_kind;
    struct obj_kind * ok = &GC_obj_kinds[kind];
    void **flh = &(ok -> ok_freelist[BYTES_TO_GRANULES(sz)]);
    
    hhdr -> hb_last_reclaimed = (unsigned short) GC_gc_no;

    if (report_if_found) {
	GC_reclaim_check(hbp, hhdr, sz);
    } else {
        *flh = GC_reclaim_generic(hbp, hhdr, sz,
				  (ok -> ok_init || GC_debugging_started),
	 			  *flh, &GC_bytes_found);
    }
}

/*
 * Restore an unmarked large object or an entirely empty blocks of small objects
 * to the heap block free list.
 * Otherwise enqueue the block for later processing
 * by GC_reclaim_small_nonempty_block.
 * If report_if_found is TRUE, then process any block immediately, and
 * simply report free objects; do not actually reclaim them.
 */
void GC_reclaim_block(struct hblk *hbp, word report_if_found)
{
    hdr * hhdr = HDR(hbp);
    size_t sz = hhdr -> hb_sz;	/* size of objects in current block	*/
    struct obj_kind * ok = &GC_obj_kinds[hhdr -> hb_obj_kind];
    struct hblk ** rlh;

    if( sz > MAXOBJBYTES ) {  /* 1 big object */
        if( !mark_bit_from_hdr(hhdr, 0) ) {
	    if (report_if_found) {
	      GC_add_leaked((ptr_t)hbp);
	    } else {
	      size_t blocks = OBJ_SZ_TO_BLOCKS(sz);
	      if (blocks > 1) {
	        GC_large_allocd_bytes -= blocks * HBLKSIZE;
	      }
	      GC_bytes_found += sz;
	      GC_freehblk(hbp);
	    }
	} else {
	    if (hhdr -> hb_descr != 0) {
	      GC_composite_in_use += sz;
	    } else {
	      GC_atomic_in_use += sz;
	    }
	}
    } else {
        GC_bool empty = GC_block_empty(hhdr);
#	ifdef PARALLEL_MARK
	  /* Count can be low or one too high because we sometimes	*/
	  /* have to ignore decrements.  Objects can also potentially   */
	  /* be repeatedly marked by each marker.			*/
	  /* Here we assume two markers, but this is extremely		*/
	  /* unlikely to fail spuriously with more.  And if it does, it	*/
	  /* should be looked at.					*/
	  GC_ASSERT(hhdr -> hb_n_marks <= 2 * (HBLKSIZE/sz + 1) + 16);
#	else
	  GC_ASSERT(sz * hhdr -> hb_n_marks <= HBLKSIZE);
#	endif
	if (hhdr -> hb_descr != 0) {
	  GC_composite_in_use += sz * hhdr -> hb_n_marks;
	} else {
	  GC_atomic_in_use += sz * hhdr -> hb_n_marks;
	}
        if (report_if_found) {
    	  GC_reclaim_small_nonempty_block(hbp, (int)report_if_found,
					  &GC_bytes_found);
        } else if (empty) {
          GC_bytes_found += HBLKSIZE;
          GC_freehblk(hbp);
        } else if (TRUE != GC_block_nearly_full(hhdr)){
          /* group of smaller objects, enqueue the real work */
          rlh = &(ok -> ok_reclaim_list[BYTES_TO_GRANULES(sz)]);
          hhdr -> hb_next = *rlh;
          *rlh = hbp;
        } /* else not worth salvaging. */
	/* We used to do the nearly_full check later, but we 	*/
	/* already have the right cache context here.  Also	*/
	/* doing it here avoids some silly lock contention in	*/
	/* GC_malloc_many.					*/
    }
}

#if !defined(NO_DEBUGGING)
/* Routines to gather and print heap block info 	*/
/* intended for debugging.  Otherwise should be called	*/
/* with lock.						*/

struct Print_stats
{
	size_t number_of_blocks;
	size_t total_bytes;
};

#ifdef USE_MARK_BYTES

/* Return the number of set mark bits in the given header	*/
int GC_n_set_marks(hdr *hhdr)
{
    int result = 0;
    int i;
    size_t sz = hhdr -> hb_sz;
    int offset = MARK_BIT_OFFSET(sz);
    int limit = FINAL_MARK_BIT(sz);

    for (i = 0; i < limit; i += offset) {
        result += hhdr -> hb_marks[i];
    }
    GC_ASSERT(hhdr -> hb_marks[limit]);
    return(result);
}

#else

/* Number of set bits in a word.  Not performance critical.	*/
static int set_bits(word n)
{
    word m = n;
    int result = 0;
    
    while (m > 0) {
    	if (m & 1) result++;
    	m >>= 1;
    }
    return(result);
}

/* Return the number of set mark bits in the given header	*/
int GC_n_set_marks(hdr *hhdr)
{
    int result = 0;
    int i;
    int n_mark_words;
#   ifdef MARK_BIT_PER_OBJ
      int n_objs = HBLK_OBJS(hhdr -> hb_sz);
    
      if (0 == n_objs) n_objs = 1;
      n_mark_words = divWORDSZ(n_objs + WORDSZ - 1);
#   else /* MARK_BIT_PER_GRANULE */
      n_mark_words = MARK_BITS_SZ;
#   endif
    for (i = 0; i < n_mark_words - 1; i++) {
        result += set_bits(hhdr -> hb_marks[i]);
    }
#   ifdef MARK_BIT_PER_OBJ
      result += set_bits((hhdr -> hb_marks[n_mark_words - 1])
		         << (n_mark_words * WORDSZ - n_objs));
#   else
      result += set_bits(hhdr -> hb_marks[n_mark_words - 1]);
#   endif
    return(result - 1);
}

#endif /* !USE_MARK_BYTES  */

/*ARGSUSED*/
void GC_print_block_descr(struct hblk *h, word /* struct PrintStats */ raw_ps)
{
    hdr * hhdr = HDR(h);
    size_t bytes = hhdr -> hb_sz;
    struct Print_stats *ps;
    unsigned n_marks = GC_n_set_marks(hhdr);
    
    if (hhdr -> hb_n_marks != n_marks) {
      GC_printf("(%u:%u,%u!=%u)", hhdr -> hb_obj_kind,
    			          bytes,
    			          hhdr -> hb_n_marks, n_marks);
    } else {
      GC_printf("(%u:%u,%u)", hhdr -> hb_obj_kind,
    			      bytes, n_marks);
    }
    bytes += HBLKSIZE-1;
    bytes &= ~(HBLKSIZE-1);

    ps = (struct Print_stats *)raw_ps;
    ps->total_bytes += bytes;
    ps->number_of_blocks++;
}

void GC_print_block_list()
{
    struct Print_stats pstats;

    GC_printf("(kind(0=ptrfree,1=normal,2=unc.):size_in_bytes, #_marks_set)\n");
    pstats.number_of_blocks = 0;
    pstats.total_bytes = 0;
    GC_apply_to_all_blocks(GC_print_block_descr, (word)&pstats);
    GC_printf("\nblocks = %lu, bytes = %lu\n",
    	      (unsigned long)pstats.number_of_blocks,
    	      (unsigned long)pstats.total_bytes);
}

/* Currently for debugger use only: */
void GC_print_free_list(int kind, size_t sz_in_granules)
{
    struct obj_kind * ok = &GC_obj_kinds[kind];
    ptr_t flh = ok -> ok_freelist[sz_in_granules];
    struct hblk *lastBlock = 0;
    int n = 0;

    while (flh){
        struct hblk *block = HBLKPTR(flh);
        if (block != lastBlock){
            GC_printf("\nIn heap block at 0x%x:\n\t", block);
            lastBlock = block;
        }
        GC_printf("%d: 0x%x;", ++n, flh);
        flh = obj_link(flh);
    }
}

#endif /* NO_DEBUGGING */

/*
 * Clear all obj_link pointers in the list of free objects *flp.
 * Clear *flp.
 * This must be done before dropping a list of free gcj-style objects,
 * since may otherwise end up with dangling "descriptor" pointers.
 * It may help for other pointer-containing objects.
 */
void GC_clear_fl_links(void **flp)
{
    void *next = *flp;

    while (0 != next) {
       *flp = 0;
       flp = &(obj_link(next));
       next = *flp;
    }
}

/*
 * Perform GC_reclaim_block on the entire heap, after first clearing
 * small object free lists (if we are not just looking for leaks).
 */
void GC_start_reclaim(GC_bool report_if_found)
{
    unsigned kind;
    
#   if defined(PARALLEL_MARK) || defined(THREAD_LOCAL_ALLOC)
      GC_ASSERT(0 == GC_fl_builder_count);
#   endif
    /* Reset in use counters.  GC_reclaim_block recomputes them. */
      GC_composite_in_use = 0;
      GC_atomic_in_use = 0;
    /* Clear reclaim- and free-lists */
      for (kind = 0; kind < GC_n_kinds; kind++) {
        void **fop;
        void **lim;
        struct hblk ** rlp;
        struct hblk ** rlim;
        struct hblk ** rlist = GC_obj_kinds[kind].ok_reclaim_list;
	GC_bool should_clobber = (GC_obj_kinds[kind].ok_descriptor != 0);
        
        if (rlist == 0) continue;	/* This kind not used.	*/
        if (!report_if_found) {
            lim = &(GC_obj_kinds[kind].ok_freelist[MAXOBJGRANULES+1]);
	    for( fop = GC_obj_kinds[kind].ok_freelist; fop < lim; fop++ ) {
	      if (*fop != 0) {
		if (should_clobber) {
		  GC_clear_fl_links(fop);
		} else {
	          *fop = 0;
		}
	      }
	    }
	} /* otherwise free list objects are marked, 	*/
	  /* and its safe to leave them			*/
	rlim = rlist + MAXOBJGRANULES+1;
	for( rlp = rlist; rlp < rlim; rlp++ ) {
	    *rlp = 0;
	}
      }
    

  /* Go through all heap blocks (in hblklist) and reclaim unmarked objects */
  /* or enqueue the block for later processing.				   */
    GC_apply_to_all_blocks(GC_reclaim_block, (word)report_if_found);

# ifdef EAGER_SWEEP
    /* This is a very stupid thing to do.  We make it possible anyway,	*/
    /* so that you can convince yourself that it really is very stupid.	*/
    GC_reclaim_all((GC_stop_func)0, FALSE);
# endif
# if defined(PARALLEL_MARK) || defined(THREAD_LOCAL_ALLOC)
    GC_ASSERT(0 == GC_fl_builder_count);
# endif
    
}

/*
 * Sweep blocks of the indicated object size and kind until either the
 * appropriate free list is nonempty, or there are no more blocks to
 * sweep.
 */
void GC_continue_reclaim(size_t sz /* granules */, int kind)
{
    hdr * hhdr;
    struct hblk * hbp;
    struct obj_kind * ok = &(GC_obj_kinds[kind]);
    struct hblk ** rlh = ok -> ok_reclaim_list;
    void **flh = &(ok -> ok_freelist[sz]);
    
    if (rlh == 0) return;	/* No blocks of this kind.	*/
    rlh += sz;
    while ((hbp = *rlh) != 0) {
        hhdr = HDR(hbp);
        *rlh = hhdr -> hb_next;
        GC_reclaim_small_nonempty_block(hbp, FALSE, &GC_bytes_found);
        if (*flh != 0) break;
    }
}

/*
 * Reclaim all small blocks waiting to be reclaimed.
 * Abort and return FALSE when/if (*stop_func)() returns TRUE.
 * If this returns TRUE, then it's safe to restart the world
 * with incorrectly cleared mark bits.
 * If ignore_old is TRUE, then reclaim only blocks that have been 
 * recently reclaimed, and discard the rest.
 * Stop_func may be 0.
 */
GC_bool GC_reclaim_all(GC_stop_func stop_func, GC_bool ignore_old)
{
    word sz;
    unsigned kind;
    hdr * hhdr;
    struct hblk * hbp;
    struct obj_kind * ok;
    struct hblk ** rlp;
    struct hblk ** rlh;
    CLOCK_TYPE start_time;
    CLOCK_TYPE done_time;
	
    if (GC_print_stats == VERBOSE)
	GET_TIME(start_time);
    
    for (kind = 0; kind < GC_n_kinds; kind++) {
    	ok = &(GC_obj_kinds[kind]);
    	rlp = ok -> ok_reclaim_list;
    	if (rlp == 0) continue;
    	for (sz = 1; sz <= MAXOBJGRANULES; sz++) {
    	    rlh = rlp + sz;
    	    while ((hbp = *rlh) != 0) {
    	        if (stop_func != (GC_stop_func)0 && (*stop_func)()) {
    	            return(FALSE);
    	        }
        	hhdr = HDR(hbp);
        	*rlh = hhdr -> hb_next;
        	if (!ignore_old || hhdr -> hb_last_reclaimed == GC_gc_no - 1) {
        	    /* It's likely we'll need it this time, too	*/
        	    /* It's been touched recently, so this	*/
        	    /* shouldn't trigger paging.		*/
        	    GC_reclaim_small_nonempty_block(hbp, FALSE, &GC_bytes_found);
        	}
            }
        }
    }
    if (GC_print_stats == VERBOSE) {
	GET_TIME(done_time);
	GC_log_printf("Disposing of reclaim lists took %lu msecs\n",
	          MS_TIME_DIFF(done_time,start_time));
    }
    return(TRUE);
}
