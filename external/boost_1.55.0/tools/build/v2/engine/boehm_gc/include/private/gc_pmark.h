/*
 * Copyright (c) 1991-1994 by Xerox Corporation.  All rights reserved.
 * Copyright (c) 2001 by Hewlett-Packard Company. All rights reserved.
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
 */

/* Private declarations of GC marker data structures and macros */

/*
 * Declarations of mark stack.  Needed by marker and client supplied mark
 * routines.  Transitively include gc_priv.h.
 * (Note that gc_priv.h should not be included before this, since this
 * includes dbg_mlc.h, which wants to include gc_priv.h AFTER defining
 * I_HIDE_POINTERS.)
 */
#ifndef GC_PMARK_H
# define GC_PMARK_H

# if defined(KEEP_BACK_PTRS) || defined(PRINT_BLACK_LIST)
#   include "dbg_mlc.h"
# endif
# ifndef GC_MARK_H
#   include "../gc_mark.h"
# endif
# ifndef GC_PRIVATE_H
#   include "gc_priv.h"
# endif

/* The real declarations of the following is in gc_priv.h, so that	*/
/* we can avoid scanning the following table.				*/
/*
extern mark_proc GC_mark_procs[MAX_MARK_PROCS];
*/

/*
 * Mark descriptor stuff that should remain private for now, mostly
 * because it's hard to export WORDSZ without including gcconfig.h.
 */
# define BITMAP_BITS (WORDSZ - GC_DS_TAG_BITS)
# define PROC(descr) \
	(GC_mark_procs[((descr) >> GC_DS_TAG_BITS) & (GC_MAX_MARK_PROCS-1)])
# define ENV(descr) \
	((descr) >> (GC_DS_TAG_BITS + GC_LOG_MAX_MARK_PROCS))
# define MAX_ENV \
  	(((word)1 << (WORDSZ - GC_DS_TAG_BITS - GC_LOG_MAX_MARK_PROCS)) - 1)


extern unsigned GC_n_mark_procs;

/* Number of mark stack entries to discard on overflow.	*/
#define GC_MARK_STACK_DISCARDS (INITIAL_MARK_STACK_SIZE/8)

typedef struct GC_ms_entry {
    ptr_t mse_start;   /* First word of object */
    GC_word mse_descr;	/* Descriptor; low order two bits are tags,	*/
    			/* identifying the upper 30 bits as one of the	*/
    			/* following:					*/
} mse;

extern size_t GC_mark_stack_size;

extern mse * GC_mark_stack_limit;

#ifdef PARALLEL_MARK
  extern mse * volatile GC_mark_stack_top;
#else
  extern mse * GC_mark_stack_top;
#endif

extern mse * GC_mark_stack;

#ifdef PARALLEL_MARK
    /*
     * Allow multiple threads to participate in the marking process.
     * This works roughly as follows:
     *  The main mark stack never shrinks, but it can grow.
     *
     *	The initiating threads holds the GC lock, and sets GC_help_wanted.
     *  
     *  Other threads:
     *     1) update helper_count (while holding mark_lock.)
     *	   2) allocate a local mark stack
     *     repeatedly:
     *		3) Steal a global mark stack entry by atomically replacing
     *		   its descriptor with 0.
     *		4) Copy it to the local stack.
     *	        5) Mark on the local stack until it is empty, or
     *		   it may be profitable to copy it back.
     *	        6) If necessary, copy local stack to global one,
     *		   holding mark lock.
     *    7) Stop when the global mark stack is empty.
     *    8) decrement helper_count (holding mark_lock).
     *
     * This is an experiment to see if we can do something along the lines
     * of the University of Tokyo SGC in a less intrusive, though probably
     * also less performant, way.
     */
    void GC_do_parallel_mark();
		/* inititate parallel marking.	*/

    extern GC_bool GC_help_wanted;	/* Protected by mark lock	*/
    extern unsigned GC_helper_count;	/* Number of running helpers.	*/
					/* Protected by mark lock	*/
    extern unsigned GC_active_count;	/* Number of active helpers.	*/
					/* Protected by mark lock	*/
					/* May increase and decrease	*/
					/* within each mark cycle.  But	*/
					/* once it returns to 0, it	*/
					/* stays zero for the cycle.	*/
    /* GC_mark_stack_top is also protected by mark lock.	*/
    /*
     * GC_notify_all_marker() is used when GC_help_wanted is first set,
     * when the last helper becomes inactive,
     * when something is added to the global mark stack, and just after
     * GC_mark_no is incremented.
     * This could be split into multiple CVs (and probably should be to
     * scale to really large numbers of processors.)
     */
#endif /* PARALLEL_MARK */

/* Return a pointer to within 1st page of object.  	*/
/* Set *new_hdr_p to corr. hdr.				*/
ptr_t GC_find_start(ptr_t current, hdr *hhdr, hdr **new_hdr_p);

mse * GC_signal_mark_stack_overflow(mse *msp);

/* Push the object obj with corresponding heap block header hhdr onto 	*/
/* the mark stack.							*/
# define PUSH_OBJ(obj, hhdr, mark_stack_top, mark_stack_limit) \
{ \
    register word _descr = (hhdr) -> hb_descr; \
        \
    if (_descr != 0) { \
        mark_stack_top++; \
        if (mark_stack_top >= mark_stack_limit) { \
          mark_stack_top = GC_signal_mark_stack_overflow(mark_stack_top); \
        } \
        mark_stack_top -> mse_start = (obj); \
        mark_stack_top -> mse_descr = _descr; \
    } \
}

/* Push the contents of current onto the mark stack if it is a valid	*/
/* ptr to a currently unmarked object.  Mark it.			*/
/* If we assumed a standard-conforming compiler, we could probably	*/
/* generate the exit_label transparently.				*/
# define PUSH_CONTENTS(current, mark_stack_top, mark_stack_limit, \
		       source, exit_label) \
{ \
    hdr * my_hhdr; \
 \
    HC_GET_HDR(current, my_hhdr, source, exit_label); \
    PUSH_CONTENTS_HDR(current, mark_stack_top, mark_stack_limit, \
		  source, exit_label, my_hhdr, TRUE);	\
exit_label: ; \
}

/* Set mark bit, exit if it was already set.	*/

# ifdef USE_MARK_BITS
#   ifdef PARALLEL_MARK
      /* The following may fail to exit even if the bit was already set.    */
      /* For our uses, that's benign:                                       */
#     define OR_WORD_EXIT_IF_SET(addr, bits, exit_label) \
        { \
          if (!(*(addr) & (mask))) { \
            AO_or((AO_t *)(addr), (mask); \
          } else { \
            goto label; \
          } \
        }
#   else
#     define OR_WORD_EXIT_IF_SET(addr, bits, exit_label) \
        { \
           word old = *(addr); \
           word my_bits = (bits); \
           if (old & my_bits) goto exit_label; \
           *(addr) = (old | my_bits); \
         }
#   endif /* !PARALLEL_MARK */
#   define SET_MARK_BIT_EXIT_IF_SET(hhdr,bit_no,exit_label) \
    { \
        word * mark_word_addr = hhdr -> hb_marks + divWORDSZ(bit_no); \
      \
        OR_WORD_EXIT_IF_SET(mark_word_addr, (word)1 << modWORDSZ(bit_no), \
                            exit_label); \
    }
# endif


#ifdef USE_MARK_BYTES
# if defined(I386) && defined(__GNUC__)
#  define LONG_MULT(hprod, lprod, x, y) { \
	asm("mull %2" : "=a"(lprod), "=d"(hprod) : "g"(y), "0"(x)); \
   }
# else /* No in-line X86 assembly code */
#  define LONG_MULT(hprod, lprod, x, y) { \
	unsigned long long prod = (unsigned long long)x \
				  * (unsigned long long)y; \
	hprod = prod >> 32;  \
	lprod = (unsigned32)prod;  \
   }
# endif

  /* There is a race here, and we may set				*/
  /* the bit twice in the concurrent case.  This can result in the	*/
  /* object being pushed twice.  But that's only a performance issue.	*/
# define SET_MARK_BIT_EXIT_IF_SET(hhdr,bit_no,exit_label) \
    { \
        char * mark_byte_addr = (char *)hhdr -> hb_marks + (bit_no); \
        char mark_byte = *mark_byte_addr; \
          \
	if (mark_byte) goto exit_label; \
	*mark_byte_addr = 1;  \
    } 
#endif /* USE_MARK_BYTES */

#ifdef PARALLEL_MARK
# define INCR_MARKS(hhdr) \
	AO_store(&(hhdr -> hb_n_marks), AO_load(&(hhdr -> hb_n_marks))+1);
#else
# define INCR_MARKS(hhdr) ++(hhdr -> hb_n_marks)
#endif

#ifdef ENABLE_TRACE
# define TRACE(source, cmd) \
	if (GC_trace_addr != 0 && (ptr_t)(source) == GC_trace_addr) cmd
# define TRACE_TARGET(target, cmd) \
	if (GC_trace_addr != 0 && (target) == *(ptr_t *)GC_trace_addr) cmd
#else
# define TRACE(source, cmd)
# define TRACE_TARGET(source, cmd)
#endif
/* If the mark bit corresponding to current is not set, set it, and 	*/
/* push the contents of the object on the mark stack.  Current points	*/
/* to the bginning of the object.  We rely on the fact that the 	*/
/* preceding header calculation will succeed for a pointer past the 	*/
/* forst page of an object, only if it is in fact a valid pointer	*/
/* to the object.  Thus we can omit the otherwise necessary tests	*/
/* here.  Note in particular tha the "displ" value is the displacement	*/
/* from the beggining of the heap block, which may itself be in the	*/
/* interior of a large object.						*/
#ifdef MARK_BIT_PER_GRANULE
# define PUSH_CONTENTS_HDR(current, mark_stack_top, mark_stack_limit, \
		           source, exit_label, hhdr, do_offset_check) \
{ \
    size_t displ = HBLKDISPL(current); /* Displacement in block; in bytes. */\
    /* displ is always within range.  If current doesn't point to	*/ \
    /* first block, then we are in the all_interior_pointers case, and	*/ \
    /* it is safe to use any displacement value.			*/ \
    size_t gran_displ = BYTES_TO_GRANULES(displ); \
    size_t gran_offset = hhdr -> hb_map[gran_displ];	\
    size_t byte_offset = displ & (GRANULE_BYTES - 1); \
    ptr_t base = current;  \
    /* The following always fails for large block references. */ \
    if (EXPECT((gran_offset | byte_offset) != 0, FALSE))  { \
	if (hhdr -> hb_large_block) { \
	  /* gran_offset is bogus.	*/ \
	  size_t obj_displ; \
	  base = (ptr_t)(hhdr -> hb_block); \
	  obj_displ = (ptr_t)(current) - base;  \
	  if (obj_displ != displ) { \
	    GC_ASSERT(obj_displ < hhdr -> hb_sz); \
	    /* Must be in all_interior_pointer case, not first block */ \
	    /* already did validity check on cache miss.	     */ \
	    ; \
	  } else { \
	    if (do_offset_check && !GC_valid_offsets[obj_displ]) { \
	      GC_ADD_TO_BLACK_LIST_NORMAL(current, source); \
	      goto exit_label; \
	    } \
	  } \
	  gran_displ = 0; \
	  GC_ASSERT(hhdr -> hb_sz > HBLKSIZE || \
		    hhdr -> hb_block == HBLKPTR(current)); \
	  GC_ASSERT((ptr_t)(hhdr -> hb_block) <= (ptr_t) current); \
	} else { \
	  size_t obj_displ = GRANULES_TO_BYTES(gran_offset) \
		      	     + byte_offset; \
	  if (do_offset_check && !GC_valid_offsets[obj_displ]) { \
	    GC_ADD_TO_BLACK_LIST_NORMAL(current, source); \
	    goto exit_label; \
	  } \
	  gran_displ -= gran_offset; \
	  base -= obj_displ; \
	} \
    } \
    GC_ASSERT(hhdr == GC_find_header(base)); \
    GC_ASSERT(gran_displ % BYTES_TO_GRANULES(hhdr -> hb_sz) == 0); \
    TRACE(source, GC_log_printf("GC:%d: passed validity tests\n",GC_gc_no)); \
    SET_MARK_BIT_EXIT_IF_SET(hhdr, gran_displ, exit_label); \
    TRACE(source, GC_log_printf("GC:%d: previously unmarked\n",GC_gc_no)); \
    TRACE_TARGET(base, \
	GC_log_printf("GC:%d: marking %p from %p instead\n", GC_gc_no, \
		      base, source)); \
    INCR_MARKS(hhdr); \
    GC_STORE_BACK_PTR((ptr_t)source, base); \
    PUSH_OBJ(base, hhdr, mark_stack_top, mark_stack_limit); \
}
#endif /* MARK_BIT_PER_GRANULE */

#ifdef MARK_BIT_PER_OBJ
# define PUSH_CONTENTS_HDR(current, mark_stack_top, mark_stack_limit, \
		           source, exit_label, hhdr, do_offset_check) \
{ \
    size_t displ = HBLKDISPL(current); /* Displacement in block; in bytes. */\
    unsigned32 low_prod, high_prod, offset_fraction; \
    unsigned32 inv_sz = hhdr -> hb_inv_sz; \
    ptr_t base = current;  \
    LONG_MULT(high_prod, low_prod, displ, inv_sz); \
    /* product is > and within sz_in_bytes of displ * sz_in_bytes * 2**32 */ \
    if (EXPECT(low_prod >> 16 != 0, FALSE))  { \
	    FIXME: fails if offset is a multiple of HBLKSIZE which becomes 0 \
	if (inv_sz == LARGE_INV_SZ) { \
	  size_t obj_displ; \
	  base = (ptr_t)(hhdr -> hb_block); \
	  obj_displ = (ptr_t)(current) - base;  \
	  if (obj_displ != displ) { \
	    GC_ASSERT(obj_displ < hhdr -> hb_sz); \
	    /* Must be in all_interior_pointer case, not first block */ \
	    /* already did validity check on cache miss.	     */ \
	    ; \
	  } else { \
	    if (do_offset_check && !GC_valid_offsets[obj_displ]) { \
	      GC_ADD_TO_BLACK_LIST_NORMAL(current, source); \
	      goto exit_label; \
	    } \
	  } \
	  GC_ASSERT(hhdr -> hb_sz > HBLKSIZE || \
		    hhdr -> hb_block == HBLKPTR(current)); \
	  GC_ASSERT((ptr_t)(hhdr -> hb_block) < (ptr_t) current); \
	} else { \
	  /* Accurate enough if HBLKSIZE <= 2**15.	*/ \
	  GC_ASSERT(HBLKSIZE <= (1 << 15)); \
	  size_t obj_displ = (((low_prod >> 16) + 1) * (hhdr -> hb_sz)) >> 16; \
	  if (do_offset_check && !GC_valid_offsets[obj_displ]) { \
	    GC_ADD_TO_BLACK_LIST_NORMAL(current, source); \
	    goto exit_label; \
	  } \
	  base -= obj_displ; \
	} \
    } \
    /* May get here for pointer to start of block not at	*/ \
    /* beginning of object.  If so, it's valid, and we're fine. */ \
    GC_ASSERT(high_prod >= 0 && high_prod <= HBLK_OBJS(hhdr -> hb_sz)); \
    TRACE(source, GC_log_printf("GC:%d: passed validity tests\n",GC_gc_no)); \
    SET_MARK_BIT_EXIT_IF_SET(hhdr, high_prod, exit_label); \
    TRACE(source, GC_log_printf("GC:%d: previously unmarked\n",GC_gc_no)); \
    TRACE_TARGET(base, \
	GC_log_printf("GC:%d: marking %p from %p instead\n", GC_gc_no, \
		      base, source)); \
    INCR_MARKS(hhdr); \
    GC_STORE_BACK_PTR((ptr_t)source, base); \
    PUSH_OBJ(base, hhdr, mark_stack_top, mark_stack_limit); \
}
#endif /* MARK_BIT_PER_OBJ */

#if defined(PRINT_BLACK_LIST) || defined(KEEP_BACK_PTRS)
#   define PUSH_ONE_CHECKED_STACK(p, source) \
	GC_mark_and_push_stack(p, (ptr_t)(source))
#else
#   define PUSH_ONE_CHECKED_STACK(p, source) \
	GC_mark_and_push_stack(p)
#endif

/*
 * Push a single value onto mark stack. Mark from the object pointed to by p.
 * Invoke FIXUP_POINTER(p) before any further processing.
 * P is considered valid even if it is an interior pointer.
 * Previously marked objects are not pushed.  Hence we make progress even
 * if the mark stack overflows.
 */

# if NEED_FIXUP_POINTER
    /* Try both the raw version and the fixed up one.	*/
#   define GC_PUSH_ONE_STACK(p, source) \
      if ((p) >= (ptr_t)GC_least_plausible_heap_addr 	\
	 && (p) < (ptr_t)GC_greatest_plausible_heap_addr) {	\
	 PUSH_ONE_CHECKED_STACK(p, source);	\
      } \
      FIXUP_POINTER(p); \
      if ((p) >= (ptr_t)GC_least_plausible_heap_addr 	\
	 && (p) < (ptr_t)GC_greatest_plausible_heap_addr) {	\
	 PUSH_ONE_CHECKED_STACK(p, source);	\
      }
# else /* !NEED_FIXUP_POINTER */
#   define GC_PUSH_ONE_STACK(p, source) \
      if ((ptr_t)(p) >= (ptr_t)GC_least_plausible_heap_addr 	\
	 && (ptr_t)(p) < (ptr_t)GC_greatest_plausible_heap_addr) {	\
	 PUSH_ONE_CHECKED_STACK(p, source);	\
      }
# endif


/*
 * As above, but interior pointer recognition as for
 * normal heap pointers.
 */
# define GC_PUSH_ONE_HEAP(p,source) \
    FIXUP_POINTER(p); \
    if ((p) >= (ptr_t)GC_least_plausible_heap_addr 	\
	 && (p) < (ptr_t)GC_greatest_plausible_heap_addr) {	\
	    GC_mark_stack_top = GC_mark_and_push( \
			    (void *)(p), GC_mark_stack_top, \
			    GC_mark_stack_limit, (void * *)(source)); \
    }

/* Mark starting at mark stack entry top (incl.) down to	*/
/* mark stack entry bottom (incl.).  Stop after performing	*/
/* about one page worth of work.  Return the new mark stack	*/
/* top entry.							*/
mse * GC_mark_from(mse * top, mse * bottom, mse *limit);

#define MARK_FROM_MARK_STACK() \
	GC_mark_stack_top = GC_mark_from(GC_mark_stack_top, \
					 GC_mark_stack, \
					 GC_mark_stack + GC_mark_stack_size);

/*
 * Mark from one finalizable object using the specified
 * mark proc. May not mark the object pointed to by 
 * real_ptr. That is the job of the caller, if appropriate.
 * Note that this is called with the mutator running, but
 * with us holding the allocation lock.  This is safe only if the
 * mutator needs tha allocation lock to reveal hidden pointers.
 * FIXME: Why do we need the GC_mark_state test below?
 */
# define GC_MARK_FO(real_ptr, mark_proc) \
{ \
    (*(mark_proc))(real_ptr); \
    while (!GC_mark_stack_empty()) MARK_FROM_MARK_STACK(); \
    if (GC_mark_state != MS_NONE) { \
        GC_set_mark_bit(real_ptr); \
        while (!GC_mark_some((ptr_t)0)) {} \
    } \
}

extern GC_bool GC_mark_stack_too_small;
				/* We need a larger mark stack.  May be	*/
				/* set by client supplied mark routines.*/

typedef int mark_state_t;	/* Current state of marking, as follows:*/
				/* Used to remember where we are during */
				/* concurrent marking.			*/

				/* We say something is dirty if it was	*/
				/* written since the last time we	*/
				/* retrieved dirty bits.  We say it's 	*/
				/* grungy if it was marked dirty in the	*/
				/* last set of bits we retrieved.	*/
				
				/* Invariant I: all roots and marked	*/
				/* objects p are either dirty, or point */
				/* to objects q that are either marked 	*/
				/* or a pointer to q appears in a range	*/
				/* on the mark stack.			*/

# define MS_NONE 0		/* No marking in progress. I holds.	*/
				/* Mark stack is empty.			*/

# define MS_PUSH_RESCUERS 1	/* Rescuing objects are currently 	*/
				/* being pushed.  I holds, except	*/
				/* that grungy roots may point to 	*/
				/* unmarked objects, as may marked	*/
				/* grungy objects above scan_ptr.	*/

# define MS_PUSH_UNCOLLECTABLE 2
				/* I holds, except that marked 		*/
				/* uncollectable objects above scan_ptr */
				/* may point to unmarked objects.	*/
				/* Roots may point to unmarked objects	*/

# define MS_ROOTS_PUSHED 3	/* I holds, mark stack may be nonempty  */

# define MS_PARTIALLY_INVALID 4	/* I may not hold, e.g. because of M.S. */
				/* overflow.  However marked heap	*/
				/* objects below scan_ptr point to	*/
				/* marked or stacked objects.		*/

# define MS_INVALID 5		/* I may not hold.			*/

extern mark_state_t GC_mark_state;

#endif  /* GC_PMARK_H */

