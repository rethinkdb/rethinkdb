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
# include <stdio.h>
# include "private/gc_priv.h"

/* Data structure for list of root sets.				*/
/* We keep a hash table, so that we can filter out duplicate additions.	*/
/* Under Win32, we need to do a better job of filtering overlaps, so	*/
/* we resort to sequential search, and pay the price.			*/
/* This is really declared in gc_priv.h:
struct roots {
	ptr_t r_start;
	ptr_t r_end;
 #	if !defined(MSWIN32) && !defined(MSWINCE)
	  struct roots * r_next;
 #	endif
	GC_bool r_tmp;
	  	-- Delete before registering new dynamic libraries
};

struct roots GC_static_roots[MAX_ROOT_SETS];
*/

int GC_no_dls = 0;	/* Register dynamic library data segments.	*/

static int n_root_sets = 0;

	/* GC_static_roots[0..n_root_sets) contains the valid root sets. */

# if !defined(NO_DEBUGGING)
/* For debugging:	*/
void GC_print_static_roots(void)
{
    register int i;
    size_t total = 0;
    
    for (i = 0; i < n_root_sets; i++) {
        GC_printf("From %p to %p ",
        	  GC_static_roots[i].r_start,
        	  GC_static_roots[i].r_end);
        if (GC_static_roots[i].r_tmp) {
            GC_printf(" (temporary)\n");
        } else {
            GC_printf("\n");
        }
        total += GC_static_roots[i].r_end - GC_static_roots[i].r_start;
    }
    GC_printf("Total size: %ld\n", (unsigned long) total);
    if (GC_root_size != total) {
     	GC_printf("GC_root_size incorrect: %ld!!\n",
     		  (unsigned long) GC_root_size);
    }
}
# endif /* NO_DEBUGGING */

/* Primarily for debugging support:	*/
/* Is the address p in one of the registered static			*/
/* root sections?							*/
GC_bool GC_is_static_root(ptr_t p)
{
    static int last_root_set = MAX_ROOT_SETS;
    register int i;
    
    
    if (last_root_set < n_root_sets
	&& p >= GC_static_roots[last_root_set].r_start
        && p < GC_static_roots[last_root_set].r_end) return(TRUE);
    for (i = 0; i < n_root_sets; i++) {
    	if (p >= GC_static_roots[i].r_start
            && p < GC_static_roots[i].r_end) {
            last_root_set = i;
            return(TRUE);
        }
    }
    return(FALSE);
}

#if !defined(MSWIN32) && !defined(MSWINCE)
/* 
#   define LOG_RT_SIZE 6
#   define RT_SIZE (1 << LOG_RT_SIZE)  -- Power of 2, may be != MAX_ROOT_SETS

    struct roots * GC_root_index[RT_SIZE];
	-- Hash table header.  Used only to check whether a range is
	-- already present.
	-- really defined in gc_priv.h
*/

static INLINE int rt_hash(ptr_t addr)
{
    word result = (word) addr;
#   if CPP_WORDSZ > 8*LOG_RT_SIZE
	result ^= result >> 8*LOG_RT_SIZE;
#   endif
#   if CPP_WORDSZ > 4*LOG_RT_SIZE
	result ^= result >> 4*LOG_RT_SIZE;
#   endif
    result ^= result >> 2*LOG_RT_SIZE;
    result ^= result >> LOG_RT_SIZE;
    result &= (RT_SIZE-1);
    return(result);
}

/* Is a range starting at b already in the table? If so return a	*/
/* pointer to it, else NIL.						*/
struct roots * GC_roots_present(ptr_t b)
{
    int h = rt_hash(b);
    struct roots *p = GC_root_index[h];
    
    while (p != 0) {
        if (p -> r_start == (ptr_t)b) return(p);
        p = p -> r_next;
    }
    return(FALSE);
}

/* Add the given root structure to the index. */
static void add_roots_to_index(struct roots *p)
{
    int h = rt_hash(p -> r_start);
    
    p -> r_next = GC_root_index[h];
    GC_root_index[h] = p;
}

# else /* MSWIN32 || MSWINCE */

#   define add_roots_to_index(p)

# endif




word GC_root_size = 0;

void GC_add_roots(void *b, void *e)
{
    DCL_LOCK_STATE;
    
    if (!GC_is_initialized) GC_init();
    LOCK();
    GC_add_roots_inner((ptr_t)b, (ptr_t)e, FALSE);
    UNLOCK();
}


/* Add [b,e) to the root set.  Adding the same interval a second time	*/
/* is a moderately fast noop, and hence benign.  We do not handle	*/
/* different but overlapping intervals efficiently.  (We do handle	*/
/* them correctly.)							*/
/* Tmp specifies that the interval may be deleted before 		*/
/* reregistering dynamic libraries.					*/ 
void GC_add_roots_inner(ptr_t b, ptr_t e, GC_bool tmp)
{
    struct roots * old;
    
#   if defined(MSWIN32) || defined(MSWINCE)
      /* Spend the time to ensure that there are no overlapping	*/
      /* or adjacent intervals.					*/
      /* This could be done faster with e.g. a			*/
      /* balanced tree.  But the execution time here is		*/
      /* virtually guaranteed to be dominated by the time it	*/
      /* takes to scan the roots.				*/
      {
        register int i;
        
        for (i = 0; i < n_root_sets; i++) {
            old = GC_static_roots + i;
            if (b <= old -> r_end && e >= old -> r_start) {
                if (b < old -> r_start) {
                    old -> r_start = b;
                    GC_root_size += (old -> r_start - b);
                }
                if (e > old -> r_end) {
                    old -> r_end = e;
                    GC_root_size += (e - old -> r_end);
                }
                old -> r_tmp &= tmp;
                break;
            }
        }
        if (i < n_root_sets) {
          /* merge other overlapping intervals */
            struct roots *other;
            
            for (i++; i < n_root_sets; i++) {
              other = GC_static_roots + i;
              b = other -> r_start;
              e = other -> r_end;
              if (b <= old -> r_end && e >= old -> r_start) {
                if (b < old -> r_start) {
                    old -> r_start = b;
                    GC_root_size += (old -> r_start - b);
                }
                if (e > old -> r_end) {
                    old -> r_end = e;
                    GC_root_size += (e - old -> r_end);
                }
                old -> r_tmp &= other -> r_tmp;
                /* Delete this entry. */
                  GC_root_size -= (other -> r_end - other -> r_start);
                  other -> r_start = GC_static_roots[n_root_sets-1].r_start;
                  other -> r_end = GC_static_roots[n_root_sets-1].r_end;
                  n_root_sets--;
              }
            }
          return;
        }
      }
#   else
      old = GC_roots_present(b);
      if (old != 0) {
        if (e <= old -> r_end) /* already there */ return;
        /* else extend */
        GC_root_size += e - old -> r_end;
        old -> r_end = e;
        return;
      }
#   endif
    if (n_root_sets == MAX_ROOT_SETS) {
        ABORT("Too many root sets\n");
    }
    GC_static_roots[n_root_sets].r_start = (ptr_t)b;
    GC_static_roots[n_root_sets].r_end = (ptr_t)e;
    GC_static_roots[n_root_sets].r_tmp = tmp;
#   if !defined(MSWIN32) && !defined(MSWINCE)
      GC_static_roots[n_root_sets].r_next = 0;
#   endif
    add_roots_to_index(GC_static_roots + n_root_sets);
    GC_root_size += e - b;
    n_root_sets++;
}

static GC_bool roots_were_cleared = FALSE;

void GC_clear_roots (void)
{
    DCL_LOCK_STATE;
    
    if (!GC_is_initialized) GC_init();
    LOCK();
    roots_were_cleared = TRUE;
    n_root_sets = 0;
    GC_root_size = 0;
#   if !defined(MSWIN32) && !defined(MSWINCE)
    {
    	register int i;
    	
    	for (i = 0; i < RT_SIZE; i++) GC_root_index[i] = 0;
    }
#   endif
    UNLOCK();
}

/* Internal use only; lock held.	*/
static void GC_remove_root_at_pos(int i) 
{
    GC_root_size -= (GC_static_roots[i].r_end - GC_static_roots[i].r_start);
    GC_static_roots[i].r_start = GC_static_roots[n_root_sets-1].r_start;
    GC_static_roots[i].r_end = GC_static_roots[n_root_sets-1].r_end;
    GC_static_roots[i].r_tmp = GC_static_roots[n_root_sets-1].r_tmp;
    n_root_sets--;
}

#if !defined(MSWIN32) && !defined(MSWINCE)
static void GC_rebuild_root_index(void)
{
    int i;
    	
    for (i = 0; i < RT_SIZE; i++) GC_root_index[i] = 0;
    for (i = 0; i < n_root_sets; i++)
	add_roots_to_index(GC_static_roots + i);
}
#endif

/* Internal use only; lock held.	*/
void GC_remove_tmp_roots(void)
{
    int i;
    
    for (i = 0; i < n_root_sets; ) {
    	if (GC_static_roots[i].r_tmp) {
            GC_remove_root_at_pos(i);
    	} else {
    	    i++;
    }
    }
    #if !defined(MSWIN32) && !defined(MSWINCE)
    GC_rebuild_root_index();
    #endif
}

#if !defined(MSWIN32) && !defined(MSWINCE)
void GC_remove_roots(void *b, void *e)
{
    DCL_LOCK_STATE;
    
    LOCK();
    GC_remove_roots_inner((ptr_t)b, (ptr_t)e);
    UNLOCK();
}

/* Should only be called when the lock is held */
void GC_remove_roots_inner(ptr_t b, ptr_t e)
{
    int i;
    for (i = 0; i < n_root_sets; ) {
    	if (GC_static_roots[i].r_start >= b
	    && GC_static_roots[i].r_end <= e) {
            GC_remove_root_at_pos(i);
    	} else {
    	    i++;
    	}
    }
    GC_rebuild_root_index();
}
#endif /* !defined(MSWIN32) && !defined(MSWINCE) */

#if defined(MSWIN32) || defined(_WIN32_WCE_EMULATION)
/* Workaround for the OS mapping and unmapping behind our back:		*/
/* Is the address p in one of the temporary static root sections?	*/
GC_bool GC_is_tmp_root(ptr_t p)
{
    static int last_root_set = MAX_ROOT_SETS;
    register int i;
    
    if (last_root_set < n_root_sets
	&& p >= GC_static_roots[last_root_set].r_start
        && p < GC_static_roots[last_root_set].r_end)
	return GC_static_roots[last_root_set].r_tmp;
    for (i = 0; i < n_root_sets; i++) {
    	if (p >= GC_static_roots[i].r_start
            && p < GC_static_roots[i].r_end) {
            last_root_set = i;
            return GC_static_roots[i].r_tmp;
        }
    }
    return(FALSE);
}
#endif /* MSWIN32 || _WIN32_WCE_EMULATION */

ptr_t GC_approx_sp(void)
{
    volatile word dummy;

    dummy = 42;	/* Force stack to grow if necessary.	Otherwise the	*/
    		/* later accesses might cause the kernel to think we're	*/
    		/* doing something wrong.				*/
#   ifdef _MSC_VER
#     pragma warning(disable:4172)
#   endif
    return((ptr_t)(&dummy));
#   ifdef _MSC_VER
#     pragma warning(default:4172)
#   endif
}

/*
 * Data structure for excluded static roots.
 * Real declaration is in gc_priv.h.

struct exclusion {
    ptr_t e_start;
    ptr_t e_end;
};

struct exclusion GC_excl_table[MAX_EXCLUSIONS];
					-- Array of exclusions, ascending
					-- address order.
*/

size_t GC_excl_table_entries = 0;	/* Number of entries in use.	  */

/* Return the first exclusion range that includes an address >= start_addr */
/* Assumes the exclusion table contains at least one entry (namely the	   */
/* GC data structures).							   */
struct exclusion * GC_next_exclusion(ptr_t start_addr)
{
    size_t low = 0;
    size_t high = GC_excl_table_entries - 1;
    size_t mid;

    while (high > low) {
	mid = (low + high) >> 1;
	/* low <= mid < high	*/
	if ((word) GC_excl_table[mid].e_end <= (word) start_addr) {
	    low = mid + 1;
	} else {
	    high = mid;
	}
    }
    if ((word) GC_excl_table[low].e_end <= (word) start_addr) return 0;
    return GC_excl_table + low;
}

void GC_exclude_static_roots(void *start, void *finish)
{
    struct exclusion * next;
    size_t next_index, i;

    if (0 == GC_excl_table_entries) {
	next = 0;
    } else {
	next = GC_next_exclusion(start);
    }
    if (0 != next) {
      if ((word)(next -> e_start) < (word) finish) {
	/* incomplete error check. */
	ABORT("exclusion ranges overlap");
      }  
      if ((word)(next -> e_start) == (word) finish) {
        /* extend old range backwards	*/
          next -> e_start = (ptr_t)start;
	  return;
      }
      next_index = next - GC_excl_table;
      for (i = GC_excl_table_entries; i > next_index; --i) {
	GC_excl_table[i] = GC_excl_table[i-1];
      }
    } else {
      next_index = GC_excl_table_entries;
    }
    if (GC_excl_table_entries == MAX_EXCLUSIONS) ABORT("Too many exclusions");
    GC_excl_table[next_index].e_start = (ptr_t)start;
    GC_excl_table[next_index].e_end = (ptr_t)finish;
    ++GC_excl_table_entries;
}

/* Invoke push_conditional on ranges that are not excluded. */
void GC_push_conditional_with_exclusions(ptr_t bottom, ptr_t top, GC_bool all)
{
    struct exclusion * next;
    ptr_t excl_start;

    while (bottom < top) {
        next = GC_next_exclusion(bottom);
	if (0 == next || (excl_start = next -> e_start) >= top) {
	    GC_push_conditional(bottom, top, all);
	    return;
	}
	if (excl_start > bottom) GC_push_conditional(bottom, excl_start, all);
	bottom = next -> e_end;
    }
}

/*
 * In the absence of threads, push the stack contents.
 * In the presence of threads, push enough of the current stack
 * to ensure that callee-save registers saved in collector frames have been
 * seen.
 * FIXME: Merge with per-thread stuff.
 */
void GC_push_current_stack(ptr_t cold_gc_frame, void * context)
{
#   if defined(THREADS)
	if (0 == cold_gc_frame) return;
#       ifdef STACK_GROWS_DOWN
    	  GC_push_all_eager(GC_approx_sp(), cold_gc_frame);
	  /* For IA64, the register stack backing store is handled 	*/
	  /* in the thread-specific code.				*/
#       else
	  GC_push_all_eager( cold_gc_frame, GC_approx_sp() );
#       endif
#   else
#   	ifdef STACK_GROWS_DOWN
    	    GC_push_all_stack_partially_eager( GC_approx_sp(), GC_stackbottom,
					       cold_gc_frame );
#	    ifdef IA64
	      /* We also need to push the register stack backing store. */
	      /* This should really be done in the same way as the	*/
	      /* regular stack.  For now we fudge it a bit.		*/
	      /* Note that the backing store grows up, so we can't use	*/
	      /* GC_push_all_stack_partially_eager.			*/
	      {
		extern word GC_save_regs_ret_val;
			/* Previously set to backing store pointer.	*/
		ptr_t bsp = (ptr_t) GC_save_regs_ret_val;
	        ptr_t cold_gc_bs_pointer;
		if (GC_all_interior_pointers) {
	          cold_gc_bs_pointer = bsp - 2048;
		  if (cold_gc_bs_pointer < BACKING_STORE_BASE) {
		    cold_gc_bs_pointer = BACKING_STORE_BASE;
		  } else {
		    GC_push_all_stack(BACKING_STORE_BASE, cold_gc_bs_pointer);
		  }
		} else {
		  cold_gc_bs_pointer = BACKING_STORE_BASE;
		}
		GC_push_all_eager(cold_gc_bs_pointer, bsp);
		/* All values should be sufficiently aligned that we	*/
		/* dont have to worry about the boundary.		*/
	      }
#	    endif
#       else
	    GC_push_all_stack_partially_eager( GC_stackbottom, GC_approx_sp(),
					       cold_gc_frame );
#       endif
#   endif /* !THREADS */
}

/*
 * Push GC internal roots.  Only called if there is some reason to believe
 * these would not otherwise get registered.
 */
void GC_push_gc_structures(void)
{
    GC_push_finalizer_structures();
#   if defined(THREADS)
      GC_push_thread_structures();
#   endif
}

#ifdef THREAD_LOCAL_ALLOC
  void GC_mark_thread_local_free_lists(void);
#endif

void GC_cond_register_dynamic_libraries(void)
{
# if defined(DYNAMIC_LOADING) || defined(MSWIN32) || defined(MSWINCE) \
     || defined(PCR)
    GC_remove_tmp_roots();
    if (!GC_no_dls) GC_register_dynamic_libraries();
# else
    GC_no_dls = TRUE;
# endif
}

/*
 * Call the mark routines (GC_tl_push for a single pointer, GC_push_conditional
 * on groups of pointers) on every top level accessible pointer.
 * If all is FALSE, arrange to push only possibly altered values.
 * Cold_gc_frame is an address inside a GC frame that
 * remains valid until all marking is complete.
 * A zero value indicates that it's OK to miss some
 * register values.
 */
void GC_push_roots(GC_bool all, ptr_t cold_gc_frame)
{
    int i;
    unsigned kind;

    /*
     * Next push static data.  This must happen early on, since it's
     * not robust against mark stack overflow.
     */
     /* Reregister dynamic libraries, in case one got added.		*/
     /* There is some argument for doing this as late as possible,	*/
     /* especially on win32, where it can change asynchronously.	*/
     /* In those cases, we do it here.  But on other platforms, it's	*/
     /* not safe with the world stopped, so we do it earlier.		*/
#      if !defined(REGISTER_LIBRARIES_EARLY)
         GC_cond_register_dynamic_libraries();
#      endif

     /* Mark everything in static data areas                             */
       for (i = 0; i < n_root_sets; i++) {
         GC_push_conditional_with_exclusions(
			     GC_static_roots[i].r_start,
			     GC_static_roots[i].r_end, all);
       }

     /* Mark all free list header blocks, if those were allocated from	*/
     /* the garbage collected heap.  This makes sure they don't 	*/
     /* disappear if we are not marking from static data.  It also 	*/
     /* saves us the trouble of scanning them, and possibly that of	*/
     /* marking the freelists.						*/
       for (kind = 0; kind < GC_n_kinds; kind++) {
	 void *base = GC_base(GC_obj_kinds[kind].ok_freelist);
	 if (0 != base) {
	   GC_set_mark_bit(base);
	 }
       }
       
     /* Mark from GC internal roots if those might otherwise have	*/
     /* been excluded.							*/
       if (GC_no_dls || roots_were_cleared) {
	   GC_push_gc_structures();
       }

     /* Mark thread local free lists, even if their mark 	*/
     /* descriptor excludes the link field.			*/
     /* If the world is not stopped, this is unsafe.  It is	*/
     /* also unnecessary, since we will do this again with the	*/
     /* world stopped.						*/
#      if defined(THREAD_LOCAL_ALLOC)
         if (GC_world_stopped) GC_mark_thread_local_free_lists();
#      endif

    /*
     * Now traverse stacks, and mark from register contents.
     * These must be done last, since they can legitimately overflow
     * the mark stack.
     * This is usually done by saving the current context on the
     * stack, and then just tracing from the stack.
     */
      GC_push_regs_and_stack(cold_gc_frame);

    if (GC_push_other_roots != 0) (*GC_push_other_roots)();
    	/* In the threads case, this also pushes thread stacks.	*/
        /* Note that without interior pointer recognition lots	*/
    	/* of stuff may have been pushed already, and this	*/
    	/* should be careful about mark stack overflows.	*/
}

