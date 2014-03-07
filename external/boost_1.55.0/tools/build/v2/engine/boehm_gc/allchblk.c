/* 
 * Copyright 1988, 1989 Hans-J. Boehm, Alan J. Demers
 * Copyright (c) 1991-1994 by Xerox Corporation.  All rights reserved.
 * Copyright (c) 1998-1999 by Silicon Graphics.  All rights reserved.
 * Copyright (c) 1999 by Hewlett-Packard Company. All rights reserved.
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

/* #define DEBUG */
#include <stdio.h>
#include "private/gc_priv.h"

GC_bool GC_use_entire_heap = 0;

/*
 * Free heap blocks are kept on one of several free lists,
 * depending on the size of the block.  Each free list is doubly linked.
 * Adjacent free blocks are coalesced.
 */

 
# define MAX_BLACK_LIST_ALLOC (2*HBLKSIZE)
		/* largest block we will allocate starting on a black   */
		/* listed block.  Must be >= HBLKSIZE.			*/


# define UNIQUE_THRESHOLD 32
	/* Sizes up to this many HBLKs each have their own free list    */
# define HUGE_THRESHOLD 256
	/* Sizes of at least this many heap blocks are mapped to a	*/
	/* single free list.						*/
# define FL_COMPRESSION 8
	/* In between sizes map this many distinct sizes to a single	*/
	/* bin.								*/

# define N_HBLK_FLS (HUGE_THRESHOLD - UNIQUE_THRESHOLD)/FL_COMPRESSION \
				 + UNIQUE_THRESHOLD

struct hblk * GC_hblkfreelist[N_HBLK_FLS+1] = { 0 };

#ifndef USE_MUNMAP

  word GC_free_bytes[N_HBLK_FLS+1] = { 0 };
	/* Number of free bytes on each list.	*/

  /* Is bytes + the number of free bytes on lists n .. N_HBLK_FLS 	*/
  /* > GC_max_large_allocd_bytes?					*/
# ifdef __GNUC__
  __inline__
# endif
  static GC_bool GC_enough_large_bytes_left(word bytes, int n)
  {
    int i;
    for (i = N_HBLK_FLS; i >= n; --i) {
	bytes += GC_free_bytes[i];
	if (bytes > GC_max_large_allocd_bytes) return TRUE;
    }
    return FALSE;
  }

# define INCR_FREE_BYTES(n, b) GC_free_bytes[n] += (b);

# define FREE_ASSERT(e) GC_ASSERT(e)

#else /* USE_MUNMAP */

# define INCR_FREE_BYTES(n, b)
# define FREE_ASSERT(e)

#endif /* USE_MUNMAP */

/* Map a number of blocks to the appropriate large block free list index. */
int GC_hblk_fl_from_blocks(word blocks_needed)
{
    if (blocks_needed <= UNIQUE_THRESHOLD) return (int)blocks_needed;
    if (blocks_needed >= HUGE_THRESHOLD) return N_HBLK_FLS;
    return (int)(blocks_needed - UNIQUE_THRESHOLD)/FL_COMPRESSION
					+ UNIQUE_THRESHOLD;
    
}

# define PHDR(hhdr) HDR(hhdr -> hb_prev)
# define NHDR(hhdr) HDR(hhdr -> hb_next)

# ifdef USE_MUNMAP
#   define IS_MAPPED(hhdr) (((hhdr) -> hb_flags & WAS_UNMAPPED) == 0)
# else  /* !USE_MMAP */
#   define IS_MAPPED(hhdr) 1
# endif /* USE_MUNMAP */

# if !defined(NO_DEBUGGING)
void GC_print_hblkfreelist()
{
    struct hblk * h;
    word total_free = 0;
    hdr * hhdr;
    word sz;
    unsigned i;
    
    for (i = 0; i <= N_HBLK_FLS; ++i) {
      h = GC_hblkfreelist[i];
#     ifdef USE_MUNMAP
        if (0 != h) GC_printf("Free list %ld:\n",
		              (unsigned long)i);
#     else
        if (0 != h) GC_printf("Free list %lu (Total size %lu):\n",
		              i, (unsigned long)GC_free_bytes[i]);
#     endif
      while (h != 0) {
        hhdr = HDR(h);
        sz = hhdr -> hb_sz;
    	GC_printf("\t%p size %lu ", h, (unsigned long)sz);
    	total_free += sz;
        if (GC_is_black_listed(h, HBLKSIZE) != 0) {
             GC_printf("start black listed\n");
        } else if (GC_is_black_listed(h, hhdr -> hb_sz) != 0) {
             GC_printf("partially black listed\n");
        } else {
             GC_printf("not black listed\n");
        }
        h = hhdr -> hb_next;
      }
    }
#   ifndef USE_MUNMAP
      if (total_free != GC_large_free_bytes) {
	GC_printf("GC_large_free_bytes = %lu (INCONSISTENT!!)\n",
		  (unsigned long) GC_large_free_bytes);
      }
#   endif
    GC_printf("Total of %lu bytes on free list\n", (unsigned long)total_free);
}

/* Return the free list index on which the block described by the header */
/* appears, or -1 if it appears nowhere.				 */
int free_list_index_of(hdr *wanted)
{
    struct hblk * h;
    hdr * hhdr;
    int i;
    
    for (i = 0; i <= N_HBLK_FLS; ++i) {
      h = GC_hblkfreelist[i];
      while (h != 0) {
        hhdr = HDR(h);
	if (hhdr == wanted) return i;
        h = hhdr -> hb_next;
      }
    }
    return -1;
}

void GC_dump_regions()
{
    unsigned i;
    ptr_t start, end;
    ptr_t p;
    size_t bytes;
    hdr *hhdr;
    for (i = 0; i < GC_n_heap_sects; ++i) {
	start = GC_heap_sects[i].hs_start;
	bytes = GC_heap_sects[i].hs_bytes;
	end = start + bytes;
	/* Merge in contiguous sections.	*/
	  while (i+1 < GC_n_heap_sects && GC_heap_sects[i+1].hs_start == end) {
	    ++i;
	    end = GC_heap_sects[i].hs_start + GC_heap_sects[i].hs_bytes;
	  }
	GC_printf("***Section from %p to %p\n", start, end);
	for (p = start; p < end;) {
	    hhdr = HDR(p);
	    GC_printf("\t%p ", p);
	    if (IS_FORWARDING_ADDR_OR_NIL(hhdr)) {
		GC_printf("Missing header!!(%d)\n", hhdr);
		p += HBLKSIZE;
		continue;
	    }
	    if (HBLK_IS_FREE(hhdr)) {
                int correct_index = GC_hblk_fl_from_blocks(
					divHBLKSZ(hhdr -> hb_sz));
	        int actual_index;
		
		GC_printf("\tfree block of size 0x%lx bytes",
			  (unsigned long)(hhdr -> hb_sz));
	 	if (IS_MAPPED(hhdr)) {
		    GC_printf("\n");
		} else {
		    GC_printf("(unmapped)\n");
		}
		actual_index = free_list_index_of(hhdr);
		if (-1 == actual_index) {
		    GC_printf("\t\tBlock not on free list %d!!\n",
			      correct_index);
		} else if (correct_index != actual_index) {
		    GC_printf("\t\tBlock on list %d, should be on %d!!\n",
			      actual_index, correct_index);
		}
		p += hhdr -> hb_sz;
	    } else {
		GC_printf("\tused for blocks of size 0x%lx bytes\n",
			  (unsigned long)(hhdr -> hb_sz));
		p += HBLKSIZE * OBJ_SZ_TO_BLOCKS(hhdr -> hb_sz);
	    }
	}
    }
}

# endif /* NO_DEBUGGING */

/* Initialize hdr for a block containing the indicated size and 	*/
/* kind of objects.							*/
/* Return FALSE on failure.						*/
static GC_bool setup_header(hdr * hhdr, struct hblk *block, size_t byte_sz,
			    int kind, unsigned flags)
{
    word descr;
    size_t granules;
    
    /* Set size, kind and mark proc fields */
      hhdr -> hb_sz = byte_sz;
      hhdr -> hb_obj_kind = (unsigned char)kind;
      hhdr -> hb_flags = (unsigned char)flags;
      hhdr -> hb_block = block;
      descr = GC_obj_kinds[kind].ok_descriptor;
      if (GC_obj_kinds[kind].ok_relocate_descr) descr += byte_sz;
      hhdr -> hb_descr = descr;
    
#   ifdef MARK_BIT_PER_OBJ
     /* Set hb_inv_sz as portably as possible. 				*/
     /* We set it to the smallest value such that sz * inv_sz > 2**32    */
     /* This may be more precision than necessary.			*/
      if (byte_sz > MAXOBJBYTES) {
	 hhdr -> hb_inv_sz = LARGE_INV_SZ;
      } else {
	word inv_sz;

#       if CPP_WORDSZ == 64
          inv_sz = ((word)1 << 32)/byte_sz;
	  if (((inv_sz*byte_sz) >> 32) == 0) ++inv_sz;
#	else  /* 32 bit words */
	  GC_ASSERT(byte_sz >= 4);
	  inv_sz = ((unsigned)1 << 31)/byte_sz;
	  inv_sz *= 2;
	  while (inv_sz*byte_sz > byte_sz) ++inv_sz;
#	endif
	hhdr -> hb_inv_sz = inv_sz;
      }
#   else /* MARK_BIT_PER_GRANULE */
      hhdr -> hb_large_block = (unsigned char)(byte_sz > MAXOBJBYTES);
      granules = BYTES_TO_GRANULES(byte_sz);
      if (EXPECT(!GC_add_map_entry(granules), FALSE)) {
        /* Make it look like a valid block. */
        hhdr -> hb_sz = HBLKSIZE;
        hhdr -> hb_descr = 0;
        hhdr -> hb_large_block = TRUE;
        hhdr -> hb_map = 0;
        return FALSE;
      } else {
        size_t index = (hhdr -> hb_large_block? 0 : granules);
        hhdr -> hb_map = GC_obj_map[index];
      }
#   endif /* MARK_BIT_PER_GRANULE */
      
    /* Clear mark bits */
      GC_clear_hdr_marks(hhdr);
      
    hhdr -> hb_last_reclaimed = (unsigned short)GC_gc_no;
    return(TRUE);
}

#define FL_UNKNOWN -1
/*
 * Remove hhdr from the appropriate free list.
 * We assume it is on the nth free list, or on the size
 * appropriate free list if n is FL_UNKNOWN.
 */
void GC_remove_from_fl(hdr *hhdr, int n)
{
    int index;

    GC_ASSERT(((hhdr -> hb_sz) & (HBLKSIZE-1)) == 0);
#   ifndef USE_MUNMAP
      /* We always need index to mainatin free counts.	*/
      if (FL_UNKNOWN == n) {
          index = GC_hblk_fl_from_blocks(divHBLKSZ(hhdr -> hb_sz));
      } else {
	  index = n;
      }
#   endif
    if (hhdr -> hb_prev == 0) {
#	ifdef USE_MUNMAP
	  if (FL_UNKNOWN == n) {
            index = GC_hblk_fl_from_blocks(divHBLKSZ(hhdr -> hb_sz));
	  } else {
	    index = n;
	  }
#	endif
	GC_ASSERT(HDR(GC_hblkfreelist[index]) == hhdr);
	GC_hblkfreelist[index] = hhdr -> hb_next;
    } else {
	hdr *phdr;
	GET_HDR(hhdr -> hb_prev, phdr);
	phdr -> hb_next = hhdr -> hb_next;
    }
    FREE_ASSERT(GC_free_bytes[index] >= hhdr -> hb_sz);
    INCR_FREE_BYTES(index, - (signed_word)(hhdr -> hb_sz));
    if (0 != hhdr -> hb_next) {
	hdr * nhdr;
	GC_ASSERT(!IS_FORWARDING_ADDR_OR_NIL(NHDR(hhdr)));
	GET_HDR(hhdr -> hb_next, nhdr);
	nhdr -> hb_prev = hhdr -> hb_prev;
    }
}

/*
 * Return a pointer to the free block ending just before h, if any.
 */
struct hblk * GC_free_block_ending_at(struct hblk *h)
{
    struct hblk * p = h - 1;
    hdr * phdr;

    GET_HDR(p, phdr);
    while (0 != phdr && IS_FORWARDING_ADDR_OR_NIL(phdr)) {
	p = FORWARDED_ADDR(p,phdr);
	phdr = HDR(p);
    }
    if (0 != phdr) {
        if(HBLK_IS_FREE(phdr)) {
	    return p;
	} else {
	    return 0;
	}
    }
    p = GC_prev_block(h - 1);
    if (0 != p) {
      phdr = HDR(p);
      if (HBLK_IS_FREE(phdr) && (ptr_t)p + phdr -> hb_sz == (ptr_t)h) {
	return p;
      }
    }
    return 0;
}

/*
 * Add hhdr to the appropriate free list.
 * We maintain individual free lists sorted by address.
 */
void GC_add_to_fl(struct hblk *h, hdr *hhdr)
{
    int index = GC_hblk_fl_from_blocks(divHBLKSZ(hhdr -> hb_sz));
    struct hblk *second = GC_hblkfreelist[index];
    hdr * second_hdr;
#   ifdef GC_ASSERTIONS
      struct hblk *next = (struct hblk *)((word)h + hhdr -> hb_sz);
      hdr * nexthdr = HDR(next);
      struct hblk *prev = GC_free_block_ending_at(h);
      hdr * prevhdr = HDR(prev);
      GC_ASSERT(nexthdr == 0 || !HBLK_IS_FREE(nexthdr) || !IS_MAPPED(nexthdr));
      GC_ASSERT(prev == 0 || !HBLK_IS_FREE(prevhdr) || !IS_MAPPED(prevhdr));
#   endif
    GC_ASSERT(((hhdr -> hb_sz) & (HBLKSIZE-1)) == 0);
    GC_hblkfreelist[index] = h;
    INCR_FREE_BYTES(index, hhdr -> hb_sz);
    FREE_ASSERT(GC_free_bytes[index] <= GC_large_free_bytes)
    hhdr -> hb_next = second;
    hhdr -> hb_prev = 0;
    if (0 != second) {
      GET_HDR(second, second_hdr);
      second_hdr -> hb_prev = h;
    }
    hhdr -> hb_flags |= FREE_BLK;
}

#ifdef USE_MUNMAP

/* Unmap blocks that haven't been recently touched.  This is the only way */
/* way blocks are ever unmapped.					  */
void GC_unmap_old(void)
{
    struct hblk * h;
    hdr * hhdr;
    word sz;
    unsigned short last_rec, threshold;
    int i;
#   define UNMAP_THRESHOLD 6
    
    for (i = 0; i <= N_HBLK_FLS; ++i) {
      for (h = GC_hblkfreelist[i]; 0 != h; h = hhdr -> hb_next) {
        hhdr = HDR(h);
	if (!IS_MAPPED(hhdr)) continue;
	threshold = (unsigned short)(GC_gc_no - UNMAP_THRESHOLD);
	last_rec = hhdr -> hb_last_reclaimed;
	if ((last_rec > GC_gc_no || last_rec < threshold)
	    && threshold < GC_gc_no /* not recently wrapped */) {
          sz = hhdr -> hb_sz;
	  GC_unmap((ptr_t)h, sz);
	  hhdr -> hb_flags |= WAS_UNMAPPED;
    	}
      }
    }  
}

/* Merge all unmapped blocks that are adjacent to other free		*/
/* blocks.  This may involve remapping, since all blocks are either	*/
/* fully mapped or fully unmapped.					*/
void GC_merge_unmapped(void)
{
    struct hblk * h, *next;
    hdr * hhdr, *nexthdr;
    word size, nextsize;
    int i;
    
    for (i = 0; i <= N_HBLK_FLS; ++i) {
      h = GC_hblkfreelist[i];
      while (h != 0) {
	GET_HDR(h, hhdr);
	size = hhdr->hb_sz;
	next = (struct hblk *)((word)h + size);
	GET_HDR(next, nexthdr);
	/* Coalesce with successor, if possible */
	  if (0 != nexthdr && HBLK_IS_FREE(nexthdr)) {
	    nextsize = nexthdr -> hb_sz;
	    if (IS_MAPPED(hhdr)) {
	      GC_ASSERT(!IS_MAPPED(nexthdr));
	      /* make both consistent, so that we can merge */
	        if (size > nextsize) {
		  GC_remap((ptr_t)next, nextsize);
		} else {
		  GC_unmap((ptr_t)h, size);
		  hhdr -> hb_flags |= WAS_UNMAPPED;
		}
	    } else if (IS_MAPPED(nexthdr)) {
	      GC_ASSERT(!IS_MAPPED(hhdr));
	      if (size > nextsize) {
		GC_unmap((ptr_t)next, nextsize);
	      } else {
		GC_remap((ptr_t)h, size);
		hhdr -> hb_flags &= ~WAS_UNMAPPED;
		hhdr -> hb_last_reclaimed = nexthdr -> hb_last_reclaimed;
	      }
	    } else {
	      /* Unmap any gap in the middle */
		GC_unmap_gap((ptr_t)h, size, (ptr_t)next, nexthdr -> hb_sz);
	    }
	    /* If they are both unmapped, we merge, but leave unmapped. */
	    GC_remove_from_fl(hhdr, i);
	    GC_remove_from_fl(nexthdr, FL_UNKNOWN);
	    hhdr -> hb_sz += nexthdr -> hb_sz; 
	    GC_remove_header(next);
	    GC_add_to_fl(h, hhdr); 
	    /* Start over at beginning of list */
	    h = GC_hblkfreelist[i];
	  } else /* not mergable with successor */ {
	    h = hhdr -> hb_next;
	  }
      } /* while (h != 0) ... */
    } /* for ... */
}

#endif /* USE_MUNMAP */

/*
 * Return a pointer to a block starting at h of length bytes.
 * Memory for the block is mapped.
 * Remove the block from its free list, and return the remainder (if any)
 * to its appropriate free list.
 * May fail by returning 0.
 * The header for the returned block must be set up by the caller.
 * If the return value is not 0, then hhdr is the header for it.
 */
struct hblk * GC_get_first_part(struct hblk *h, hdr *hhdr,
			        size_t bytes, int index)
{
    word total_size = hhdr -> hb_sz;
    struct hblk * rest;
    hdr * rest_hdr;

    GC_ASSERT((total_size & (HBLKSIZE-1)) == 0);
    GC_remove_from_fl(hhdr, index);
    if (total_size == bytes) return h;
    rest = (struct hblk *)((word)h + bytes);
    rest_hdr = GC_install_header(rest);
    if (0 == rest_hdr) {
	/* FIXME: This is likely to be very bad news ... */
	WARN("Header allocation failed: Dropping block.\n", 0);
	return(0);
    }
    rest_hdr -> hb_sz = total_size - bytes;
    rest_hdr -> hb_flags = 0;
#   ifdef GC_ASSERTIONS
      /* Mark h not free, to avoid assertion about adjacent free blocks. */
        hhdr -> hb_flags &= ~FREE_BLK;
#   endif
    GC_add_to_fl(rest, rest_hdr);
    return h;
}

/*
 * H is a free block.  N points at an address inside it.
 * A new header for n has already been set up.  Fix up h's header
 * to reflect the fact that it is being split, move it to the
 * appropriate free list.
 * N replaces h in the original free list.
 *
 * Nhdr is not completely filled in, since it is about to allocated.
 * It may in fact end up on the wrong free list for its size.
 * (Hence adding it to a free list is silly.  But this path is hopefully
 * rare enough that it doesn't matter.  The code is cleaner this way.)
 */
void GC_split_block(struct hblk *h, hdr *hhdr, struct hblk *n,
		    hdr *nhdr, int index /* Index of free list */)
{
    word total_size = hhdr -> hb_sz;
    word h_size = (word)n - (word)h;
    struct hblk *prev = hhdr -> hb_prev;
    struct hblk *next = hhdr -> hb_next;

    /* Replace h with n on its freelist */
      nhdr -> hb_prev = prev;
      nhdr -> hb_next = next;
      nhdr -> hb_sz = total_size - h_size;
      nhdr -> hb_flags = 0;
      if (0 != prev) {
	HDR(prev) -> hb_next = n;
      } else {
        GC_hblkfreelist[index] = n;
      }
      if (0 != next) {
	HDR(next) -> hb_prev = n;
      }
      INCR_FREE_BYTES(index, -(signed_word)h_size);
      FREE_ASSERT(GC_free_bytes[index] > 0);
#     ifdef GC_ASSERTIONS
	nhdr -> hb_flags &= ~FREE_BLK;
				/* Don't fail test for consecutive	*/
				/* free blocks in GC_add_to_fl.		*/
#     endif
#   ifdef USE_MUNMAP
      hhdr -> hb_last_reclaimed = (unsigned short)GC_gc_no;
#   endif
    hhdr -> hb_sz = h_size;
    GC_add_to_fl(h, hhdr);
    nhdr -> hb_flags |= FREE_BLK;
}
	
struct hblk *
GC_allochblk_nth(size_t sz/* bytes */, int kind, unsigned flags, int n);

/*
 * Allocate (and return pointer to) a heap block
 *   for objects of size sz bytes, searching the nth free list.
 *
 * NOTE: We set obj_map field in header correctly.
 *       Caller is responsible for building an object freelist in block.
 *
 * The client is responsible for clearing the block, if necessary.
 */
struct hblk *
GC_allochblk(size_t sz, int kind, unsigned flags/* IGNORE_OFF_PAGE or 0 */)
{
    word blocks;
    int start_list;
    int i;

    GC_ASSERT((sz & (GRANULE_BYTES - 1)) == 0);
    blocks = OBJ_SZ_TO_BLOCKS(sz);
    start_list = GC_hblk_fl_from_blocks(blocks);
    for (i = start_list; i <= N_HBLK_FLS; ++i) {
	struct hblk * result = GC_allochblk_nth(sz, kind, flags, i);
	if (0 != result) {
	    return result;
	}
    }
    return 0;
}
/*
 * The same, but with search restricted to nth free list.
 * Flags is IGNORE_OFF_PAGE or zero.
 * Unlike the above, sz is in bytes.
 */
struct hblk *
GC_allochblk_nth(size_t sz, int kind, unsigned flags, int n)
{
    struct hblk *hbp;
    hdr * hhdr;		/* Header corr. to hbp */
    			/* Initialized after loop if hbp !=0 	*/
    			/* Gcc uninitialized use warning is bogus.	*/
    struct hblk *thishbp;
    hdr * thishdr;		/* Header corr. to hbp */
    signed_word size_needed;    /* number of bytes in requested objects */
    signed_word size_avail;	/* bytes available in this block	*/

    size_needed = HBLKSIZE * OBJ_SZ_TO_BLOCKS(sz);

    /* search for a big enough block in free list */
	hbp = GC_hblkfreelist[n];
	for(; 0 != hbp; hbp = hhdr -> hb_next) {
	    GET_HDR(hbp, hhdr);
	    size_avail = hhdr->hb_sz;
	    if (size_avail < size_needed) continue;
	    if (size_avail != size_needed
		&& !GC_use_entire_heap
		&& !GC_dont_gc
		&& USED_HEAP_SIZE >= GC_requested_heapsize
		&& !TRUE_INCREMENTAL && GC_should_collect()) {
#		ifdef USE_MUNMAP
		    continue;
#		else
		    /* If we have enough large blocks left to cover any	*/
		    /* previous request for large blocks, we go ahead	*/
		    /* and split.  Assuming a steady state, that should	*/
		    /* be safe.  It means that we can use the full 	*/
		    /* heap if we allocate only small objects.		*/
		    if (!GC_enough_large_bytes_left(GC_large_allocd_bytes, n)) {
		      continue;
		    } 
		    /* If we are deallocating lots of memory from	*/
		    /* finalizers, fail and collect sooner rather	*/
		    /* than later.					*/
		    if (GC_finalizer_bytes_freed > (GC_heapsize >> 4))  {
		      continue;
		    }
#		endif /* !USE_MUNMAP */
	    }
	    /* If the next heap block is obviously better, go on.	*/
	    /* This prevents us from disassembling a single large block */
	    /* to get tiny blocks.					*/
	    {
	      signed_word next_size;
	      
	      thishbp = hhdr -> hb_next;
	      if (thishbp != 0) {
		GET_HDR(thishbp, thishdr);
	        next_size = (signed_word)(thishdr -> hb_sz);
	        if (next_size < size_avail
	          && next_size >= size_needed
	          && !GC_is_black_listed(thishbp, (word)size_needed)) {
	          continue;
	        }
	      }
	    }
	    if ( !IS_UNCOLLECTABLE(kind) &&
	         (kind != PTRFREE || size_needed > MAX_BLACK_LIST_ALLOC)) {
	      struct hblk * lasthbp = hbp;
	      ptr_t search_end = (ptr_t)hbp + size_avail - size_needed;
	      signed_word orig_avail = size_avail;
	      signed_word eff_size_needed = ((flags & IGNORE_OFF_PAGE)?
	      					HBLKSIZE
	      					: size_needed);
	      
	      
	      while ((ptr_t)lasthbp <= search_end
	             && (thishbp = GC_is_black_listed(lasthbp,
	             				      (word)eff_size_needed))
		        != 0) {
	        lasthbp = thishbp;
	      }
	      size_avail -= (ptr_t)lasthbp - (ptr_t)hbp;
	      thishbp = lasthbp;
	      if (size_avail >= size_needed) {
	        if (thishbp != hbp &&
		    0 != (thishdr = GC_install_header(thishbp))) {
		  /* Make sure it's mapped before we mangle it. */
#		    ifdef USE_MUNMAP
		      if (!IS_MAPPED(hhdr)) {
		        GC_remap((ptr_t)hbp, hhdr -> hb_sz);
		        hhdr -> hb_flags &= ~WAS_UNMAPPED;
		      }
#		    endif
	          /* Split the block at thishbp */
		      GC_split_block(hbp, hhdr, thishbp, thishdr, n);
		  /* Advance to thishbp */
		      hbp = thishbp;
		      hhdr = thishdr;
		      /* We must now allocate thishbp, since it may	*/
		      /* be on the wrong free list.			*/
		}
	      } else if (size_needed > (signed_word)BL_LIMIT
	                 && orig_avail - size_needed
			    > (signed_word)BL_LIMIT) {
	        /* Punt, since anything else risks unreasonable heap growth. */
		if (++GC_large_alloc_warn_suppressed
		    >= GC_large_alloc_warn_interval) {
	          WARN("Repeated allocation of very large block "
		       "(appr. size %ld):\n"
		       "\tMay lead to memory leak and poor performance.\n",
		       size_needed);
		  GC_large_alloc_warn_suppressed = 0;
		}
	        size_avail = orig_avail;
	      } else if (size_avail == 0 && size_needed == HBLKSIZE
			 && IS_MAPPED(hhdr)) {
		if (!GC_find_leak) {
	      	  static unsigned count = 0;
	      	  
	      	  /* The block is completely blacklisted.  We need 	*/
	      	  /* to drop some such blocks, since otherwise we spend */
	      	  /* all our time traversing them if pointerfree	*/
	      	  /* blocks are unpopular.				*/
	          /* A dropped block will be reconsidered at next GC.	*/
	          if ((++count & 3) == 0) {
	            /* Allocate and drop the block in small chunks, to	*/
	            /* maximize the chance that we will recover some	*/
	            /* later.						*/
		      word total_size = hhdr -> hb_sz;
	              struct hblk * limit = hbp + divHBLKSZ(total_size);
	              struct hblk * h;
		      struct hblk * prev = hhdr -> hb_prev;
	              
		      GC_large_free_bytes -= total_size;
		      GC_remove_from_fl(hhdr, n);
	              for (h = hbp; h < limit; h++) {
	                if (h == hbp || 0 != (hhdr = GC_install_header(h))) {
	                  (void) setup_header(
	                	  hhdr, h,
	              		  HBLKSIZE,
	              		  PTRFREE, 0); /* Cant fail */
	              	  if (GC_debugging_started) {
	              	    BZERO(h, HBLKSIZE);
	              	  }
	                }
	              }
	            /* Restore hbp to point at free block */
		      hbp = prev;
		      if (0 == hbp) {
			return GC_allochblk_nth(sz, kind, flags, n);
		      }
	   	      hhdr = HDR(hbp);
	          }
		}
	      }
	    }
	    if( size_avail >= size_needed ) {
#		ifdef USE_MUNMAP
		  if (!IS_MAPPED(hhdr)) {
		    GC_remap((ptr_t)hbp, hhdr -> hb_sz);
		    hhdr -> hb_flags &= ~WAS_UNMAPPED;
		  }
#	        endif
		/* hbp may be on the wrong freelist; the parameter n	*/
		/* is important.					*/
		hbp = GC_get_first_part(hbp, hhdr, size_needed, n);
		break;
	    }
	}

    if (0 == hbp) return 0;
	
    /* Add it to map of valid blocks */
    	if (!GC_install_counts(hbp, (word)size_needed)) return(0);
    	/* This leaks memory under very rare conditions. */
    		
    /* Set up header */
        if (!setup_header(hhdr, hbp, sz, kind, flags)) {
            GC_remove_counts(hbp, (word)size_needed);
            return(0); /* ditto */
        }

    /* Notify virtual dirty bit implementation that we are about to write.  */
    /* Ensure that pointerfree objects are not protected if it's avoidable. */
    	GC_remove_protection(hbp, divHBLKSZ(size_needed),
			     (hhdr -> hb_descr == 0) /* pointer-free */);
        
    /* We just successfully allocated a block.  Restart count of	*/
    /* consecutive failures.						*/
    {
	extern unsigned GC_fail_count;
	
	GC_fail_count = 0;
    }

    GC_large_free_bytes -= size_needed;
    
    GC_ASSERT(IS_MAPPED(hhdr));
    return( hbp );
}
 
struct hblk * GC_freehblk_ptr = 0;  /* Search position hint for GC_freehblk */

/*
 * Free a heap block.
 *
 * Coalesce the block with its neighbors if possible.
 *
 * All mark words are assumed to be cleared.
 */
void
GC_freehblk(struct hblk *hbp)
{
struct hblk *next, *prev;
hdr *hhdr, *prevhdr, *nexthdr;
signed_word size;


    GET_HDR(hbp, hhdr);
    size = hhdr->hb_sz;
    size = HBLKSIZE * OBJ_SZ_TO_BLOCKS(size);
    GC_remove_counts(hbp, (word)size);
    hhdr->hb_sz = size;
#   ifdef USE_MUNMAP
      hhdr -> hb_last_reclaimed = (unsigned short)GC_gc_no;
#   endif
    
    /* Check for duplicate deallocation in the easy case */
      if (HBLK_IS_FREE(hhdr)) {
        GC_printf("Duplicate large block deallocation of %p\n", hbp);
	ABORT("Duplicate large block deallocation");
      }

    GC_ASSERT(IS_MAPPED(hhdr));
    hhdr -> hb_flags |= FREE_BLK;
    next = (struct hblk *)((word)hbp + size);
    GET_HDR(next, nexthdr);
    prev = GC_free_block_ending_at(hbp);
    /* Coalesce with successor, if possible */
      if(0 != nexthdr && HBLK_IS_FREE(nexthdr) && IS_MAPPED(nexthdr)) {
	GC_remove_from_fl(nexthdr, FL_UNKNOWN);
	hhdr -> hb_sz += nexthdr -> hb_sz; 
	GC_remove_header(next);
      }
    /* Coalesce with predecessor, if possible. */
      if (0 != prev) {
	prevhdr = HDR(prev);
	if (IS_MAPPED(prevhdr)) {
	  GC_remove_from_fl(prevhdr, FL_UNKNOWN);
	  prevhdr -> hb_sz += hhdr -> hb_sz;
#	  ifdef USE_MUNMAP
	    prevhdr -> hb_last_reclaimed = (unsigned short)GC_gc_no;
#	  endif
	  GC_remove_header(hbp);
	  hbp = prev;
	  hhdr = prevhdr;
	}
      }
    /* FIXME: It is not clear we really always want to do these merges	*/
    /* with -DUSE_MUNMAP, since it updates ages and hence prevents	*/
    /* unmapping. 							*/

    GC_large_free_bytes += size;
    GC_add_to_fl(hbp, hhdr);    
}

