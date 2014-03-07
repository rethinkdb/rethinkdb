/*
 * Copyright 1988, 1989 Hans-J. Boehm, Alan J. Demers
 * Copyright (c) 1991-1996 by Xerox Corporation.  All rights reserved.
 * Copyright (c) 1998 by Silicon Graphics.  All rights reserved.
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
 *
 */


# include "private/gc_priv.h"

# include <stdio.h>
# if !defined(MACOS) && !defined(MSWINCE)
#   include <signal.h>
#   include <sys/types.h>
# endif

/*
 * Separate free lists are maintained for different sized objects
 * up to MAXOBJBYTES.
 * The call GC_allocobj(i,k) ensures that the freelist for
 * kind k objects of size i points to a non-empty
 * free list. It returns a pointer to the first entry on the free list.
 * In a single-threaded world, GC_allocobj may be called to allocate
 * an object of (small) size i as follows:
 *
 *            opp = &(GC_objfreelist[i]);
 *            if (*opp == 0) GC_allocobj(i, NORMAL);
 *            ptr = *opp;
 *            *opp = obj_link(ptr);
 *
 * Note that this is very fast if the free list is non-empty; it should
 * only involve the execution of 4 or 5 simple instructions.
 * All composite objects on freelists are cleared, except for
 * their first word.
 */

/*
 *  The allocator uses GC_allochblk to allocate large chunks of objects.
 * These chunks all start on addresses which are multiples of
 * HBLKSZ.   Each allocated chunk has an associated header,
 * which can be located quickly based on the address of the chunk.
 * (See headers.c for details.) 
 * This makes it possible to check quickly whether an
 * arbitrary address corresponds to an object administered by the
 * allocator.
 */

word GC_non_gc_bytes = 0;  /* Number of bytes not intended to be collected */

word GC_gc_no = 0;

#ifndef SMALL_CONFIG
  int GC_incremental = 0;  /* By default, stop the world.	*/
#endif

int GC_parallel = FALSE;   /* By default, parallel GC is off.	*/

int GC_full_freq = 19;	   /* Every 20th collection is a full	*/
			   /* collection, whether we need it 	*/
			   /* or not.			        */

GC_bool GC_need_full_gc = FALSE;
			   /* Need full GC do to heap growth.	*/

#ifdef THREADS
  GC_bool GC_world_stopped = FALSE;
# define IF_THREADS(x) x
#else
# define IF_THREADS(x)
#endif

word GC_used_heap_size_after_full = 0;

char * GC_copyright[] =
{"Copyright 1988,1989 Hans-J. Boehm and Alan J. Demers ",
"Copyright (c) 1991-1995 by Xerox Corporation.  All rights reserved. ",
"Copyright (c) 1996-1998 by Silicon Graphics.  All rights reserved. ",
"Copyright (c) 1999-2001 by Hewlett-Packard Company.  All rights reserved. ",
"THIS MATERIAL IS PROVIDED AS IS, WITH ABSOLUTELY NO WARRANTY",
" EXPRESSED OR IMPLIED.  ANY USE IS AT YOUR OWN RISK.",
"See source code for details." };

# include "version.h"

/* some more variables */

extern signed_word GC_bytes_found; /* Number of reclaimed bytes		*/
				  /* after garbage collection      	*/

GC_bool GC_dont_expand = 0;

word GC_free_space_divisor = 3;

extern GC_bool GC_collection_in_progress();
		/* Collection is in progress, or was abandoned.	*/

int GC_never_stop_func (void) { return(0); }

unsigned long GC_time_limit = TIME_LIMIT;

CLOCK_TYPE GC_start_time;  	/* Time at which we stopped world.	*/
				/* used only in GC_timeout_stop_func.	*/

int GC_n_attempts = 0;		/* Number of attempts at finishing	*/
				/* collection within GC_time_limit.	*/

#if defined(SMALL_CONFIG) || defined(NO_CLOCK)
#   define GC_timeout_stop_func GC_never_stop_func
#else
  int GC_timeout_stop_func (void)
  {
    CLOCK_TYPE current_time;
    static unsigned count = 0;
    unsigned long time_diff;
    
    if ((count++ & 3) != 0) return(0);
    GET_TIME(current_time);
    time_diff = MS_TIME_DIFF(current_time,GC_start_time);
    if (time_diff >= GC_time_limit) {
	if (GC_print_stats) {
	    GC_log_printf("Abandoning stopped marking after ");
	    GC_log_printf("%lu msecs", time_diff);
	    GC_log_printf("(attempt %d)\n", GC_n_attempts);
	}
    	return(1);
    }
    return(0);
  }
#endif /* !SMALL_CONFIG */

/* Return the minimum number of words that must be allocated between	*/
/* collections to amortize the collection cost.				*/
static word min_bytes_allocd()
{
#   ifdef THREADS
 	/* We punt, for now. */
 	signed_word stack_size = 10000;
#   else
        int dummy;
        signed_word stack_size = (ptr_t)(&dummy) - GC_stackbottom;
#   endif
    word total_root_size;  	    /* includes double stack size,	*/
    				    /* since the stack is expensive	*/
    				    /* to scan.				*/
    word scan_size;		/* Estimate of memory to be scanned 	*/
				/* during normal GC.			*/
    
    if (stack_size < 0) stack_size = -stack_size;
    total_root_size = 2 * stack_size + GC_root_size;
    scan_size = 2 * GC_composite_in_use + GC_atomic_in_use
		+ total_root_size;
    if (TRUE_INCREMENTAL) {
        return scan_size / (2 * GC_free_space_divisor);
    } else {
        return scan_size / GC_free_space_divisor;
    }
}

/* Return the number of bytes allocated, adjusted for explicit storage	*/
/* management, etc..  This number is used in deciding when to trigger	*/
/* collections.								*/
word GC_adj_bytes_allocd(void)
{
    signed_word result;
    signed_word expl_managed =
    		(signed_word)GC_non_gc_bytes
		- (signed_word)GC_non_gc_bytes_at_gc;
    
    /* Don't count what was explicitly freed, or newly allocated for	*/
    /* explicit management.  Note that deallocating an explicitly	*/
    /* managed object should not alter result, assuming the client	*/
    /* is playing by the rules.						*/
    result = (signed_word)GC_bytes_allocd
    	     - (signed_word)GC_bytes_freed 
	     + (signed_word)GC_finalizer_bytes_freed
	     - expl_managed;
    if (result > (signed_word)GC_bytes_allocd) {
        result = GC_bytes_allocd;
    	/* probably client bug or unfortunate scheduling */
    }
    result += GC_bytes_finalized;
    	/* We count objects enqueued for finalization as though they	*/
    	/* had been reallocated this round. Finalization is user	*/
    	/* visible progress.  And if we don't count this, we have	*/
    	/* stability problems for programs that finalize all objects.	*/
    if (result < (signed_word)(GC_bytes_allocd >> 3)) {
    	/* Always count at least 1/8 of the allocations.  We don't want	*/
    	/* to collect too infrequently, since that would inhibit	*/
    	/* coalescing of free storage blocks.				*/
    	/* This also makes us partially robust against client bugs.	*/
        return(GC_bytes_allocd >> 3);
    } else {
        return(result);
    }
}


/* Clear up a few frames worth of garbage left at the top of the stack.	*/
/* This is used to prevent us from accidentally treating garbade left	*/
/* on the stack by other parts of the collector as roots.  This 	*/
/* differs from the code in misc.c, which actually tries to keep the	*/
/* stack clear of long-lived, client-generated garbage.			*/
void GC_clear_a_few_frames()
{
#   define NWORDS 64
    word frames[NWORDS];
    int i;
    
    for (i = 0; i < NWORDS; i++) frames[i] = 0;
}

/* Heap size at which we need a collection to avoid expanding past	*/
/* limits used by blacklisting.						*/
static word GC_collect_at_heapsize = (word)(-1);

/* Have we allocated enough to amortize a collection? */
GC_bool GC_should_collect(void)
{
    return(GC_adj_bytes_allocd() >= min_bytes_allocd()
	   || GC_heapsize >= GC_collect_at_heapsize);
}


void GC_notify_full_gc(void)
{
    if (GC_start_call_back != (void (*) (void))0) {
	(*GC_start_call_back)();
    }
}

GC_bool GC_is_full_gc = FALSE;

/* 
 * Initiate a garbage collection if appropriate.
 * Choose judiciously
 * between partial, full, and stop-world collections.
 * Assumes lock held, signals disabled.
 */
void GC_maybe_gc(void)
{
    static int n_partial_gcs = 0;

    if (GC_should_collect()) {
        if (!GC_incremental) {
            GC_gcollect_inner();
            n_partial_gcs = 0;
            return;
        } else {
#   	  ifdef PARALLEL_MARK
	    GC_wait_for_reclaim();
#   	  endif
	  if (GC_need_full_gc || n_partial_gcs >= GC_full_freq) {
	    if (GC_print_stats) {
	        GC_log_printf(
	          "***>Full mark for collection %lu after %ld allocd bytes\n",
     		  (unsigned long)GC_gc_no+1,
		  (long)GC_bytes_allocd);
	    }
	    GC_promote_black_lists();
	    (void)GC_reclaim_all((GC_stop_func)0, TRUE);
	    GC_clear_marks();
            n_partial_gcs = 0;
	    GC_notify_full_gc();
 	    GC_is_full_gc = TRUE;
          } else {
            n_partial_gcs++;
          }
	}
        /* We try to mark with the world stopped.	*/
        /* If we run out of time, this turns into	*/
        /* incremental marking.			*/
#	ifndef NO_CLOCK
          if (GC_time_limit != GC_TIME_UNLIMITED) { GET_TIME(GC_start_time); }
#	endif
        if (GC_stopped_mark(GC_time_limit == GC_TIME_UNLIMITED? 
			    GC_never_stop_func : GC_timeout_stop_func)) {
#           ifdef SAVE_CALL_CHAIN
                GC_save_callers(GC_last_stack);
#           endif
            GC_finish_collection();
        } else {
	    if (!GC_is_full_gc) {
		/* Count this as the first attempt */
	        GC_n_attempts++;
	    }
	}
    }
}


/*
 * Stop the world garbage collection.  Assumes lock held, signals disabled.
 * If stop_func is not GC_never_stop_func, then abort if stop_func returns TRUE.
 * Return TRUE if we successfully completed the collection.
 */
GC_bool GC_try_to_collect_inner(GC_stop_func stop_func)
{
    CLOCK_TYPE start_time, current_time;
    if (GC_dont_gc) return FALSE;
    if (GC_incremental && GC_collection_in_progress()) {
      if (GC_print_stats) {
	GC_log_printf(
	    "GC_try_to_collect_inner: finishing collection in progress\n");
      }
      /* Just finish collection already in progress.	*/
    	while(GC_collection_in_progress()) {
    	    if (stop_func()) return(FALSE);
    	    GC_collect_a_little_inner(1);
    	}
    }
    if (stop_func == GC_never_stop_func) GC_notify_full_gc();
    if (GC_print_stats) {
        GET_TIME(start_time);
	GC_log_printf(
	   "Initiating full world-stop collection %lu after %ld allocd bytes\n",
	   (unsigned long)GC_gc_no+1, (long)GC_bytes_allocd);
    }
    GC_promote_black_lists();
    /* Make sure all blocks have been reclaimed, so sweep routines	*/
    /* don't see cleared mark bits.					*/
    /* If we're guaranteed to finish, then this is unnecessary.		*/
    /* In the find_leak case, we have to finish to guarantee that 	*/
    /* previously unmarked objects are not reported as leaks.		*/
#       ifdef PARALLEL_MARK
	    GC_wait_for_reclaim();
#       endif
 	if ((GC_find_leak || stop_func != GC_never_stop_func)
	    && !GC_reclaim_all(stop_func, FALSE)) {
	    /* Aborted.  So far everything is still consistent.	*/
	    return(FALSE);
	}
    GC_invalidate_mark_state();  /* Flush mark stack.	*/
    GC_clear_marks();
#   ifdef SAVE_CALL_CHAIN
        GC_save_callers(GC_last_stack);
#   endif
    GC_is_full_gc = TRUE;
    if (!GC_stopped_mark(stop_func)) {
      if (!GC_incremental) {
    	/* We're partially done and have no way to complete or use 	*/
    	/* current work.  Reestablish invariants as cheaply as		*/
    	/* possible.							*/
    	GC_invalidate_mark_state();
	GC_unpromote_black_lists();
      } /* else we claim the world is already still consistent.  We'll 	*/
        /* finish incrementally.					*/
      return(FALSE);
    }
    GC_finish_collection();
    if (GC_print_stats) {
        GET_TIME(current_time);
        GC_log_printf("Complete collection took %lu msecs\n",
                  MS_TIME_DIFF(current_time,start_time));
    }
    return(TRUE);
}



/*
 * Perform n units of garbage collection work.  A unit is intended to touch
 * roughly GC_RATE pages.  Every once in a while, we do more than that.
 * This needs to be a fairly large number with our current incremental
 * GC strategy, since otherwise we allocate too much during GC, and the
 * cleanup gets expensive.
 */
# define GC_RATE 10 
# define MAX_PRIOR_ATTEMPTS 1
 	/* Maximum number of prior attempts at world stop marking	*/
 	/* A value of 1 means that we finish the second time, no matter */
 	/* how long it takes.  Doesn't count the initial root scan	*/
 	/* for a full GC.						*/

int GC_deficit = 0;	/* The number of extra calls to GC_mark_some	*/
			/* that we have made.				*/

void GC_collect_a_little_inner(int n)
{
    int i;
    
    if (GC_dont_gc) return;
    if (GC_incremental && GC_collection_in_progress()) {
    	for (i = GC_deficit; i < GC_RATE*n; i++) {
    	    if (GC_mark_some((ptr_t)0)) {
    	        /* Need to finish a collection */
#     		ifdef SAVE_CALL_CHAIN
        	    GC_save_callers(GC_last_stack);
#     		endif
#		ifdef PARALLEL_MARK
		    GC_wait_for_reclaim();
#		endif
		if (GC_n_attempts < MAX_PRIOR_ATTEMPTS
		    && GC_time_limit != GC_TIME_UNLIMITED) {
		  GET_TIME(GC_start_time);
		  if (!GC_stopped_mark(GC_timeout_stop_func)) {
		    GC_n_attempts++;
		    break;
		  }
		} else {
		  (void)GC_stopped_mark(GC_never_stop_func);
		}
    	        GC_finish_collection();
    	        break;
    	    }
    	}
    	if (GC_deficit > 0) GC_deficit -= GC_RATE*n;
	if (GC_deficit < 0) GC_deficit = 0;
    } else {
        GC_maybe_gc();
    }
}

int GC_collect_a_little(void)
{
    int result;
    DCL_LOCK_STATE;

    LOCK();
    GC_collect_a_little_inner(1);
    result = (int)GC_collection_in_progress();
    UNLOCK();
    if (!result && GC_debugging_started) GC_print_all_smashed();
    return(result);
}

/*
 * Assumes lock is held, signals are disabled.
 * We stop the world.
 * If stop_func() ever returns TRUE, we may fail and return FALSE.
 * Increment GC_gc_no if we succeed.
 */
GC_bool GC_stopped_mark(GC_stop_func stop_func)
{
    unsigned i;
    int dummy;
    CLOCK_TYPE start_time, current_time;
	
    if (GC_print_stats)
	GET_TIME(start_time);

#   if defined(REGISTER_LIBRARIES_EARLY)
        GC_cond_register_dynamic_libraries();
#   endif
    STOP_WORLD();
    IF_THREADS(GC_world_stopped = TRUE);
    if (GC_print_stats) {
	GC_log_printf("--> Marking for collection %lu ",
		  (unsigned long)GC_gc_no + 1);
	GC_log_printf("after %lu allocd bytes\n",
	   	   (unsigned long) GC_bytes_allocd);
    }
#   ifdef MAKE_BACK_GRAPH
      if (GC_print_back_height) {
        GC_build_back_graph();
      }
#   endif

    /* Mark from all roots.  */
        /* Minimize junk left in my registers and on the stack */
            GC_clear_a_few_frames();
            GC_noop(0,0,0,0,0,0);
	GC_initiate_gc();
	for(i = 0;;i++) {
	    if ((*stop_func)()) {
		    if (GC_print_stats) {
		    	GC_log_printf("Abandoned stopped marking after ");
			GC_log_printf("%u iterations\n", i);
		    }
		    GC_deficit = i; /* Give the mutator a chance. */
                    IF_THREADS(GC_world_stopped = FALSE);
	            START_WORLD();
	            return(FALSE);
	    }
	    if (GC_mark_some((ptr_t)(&dummy))) break;
	}
	
    GC_gc_no++;
    if (GC_print_stats) {
      GC_log_printf("Collection %lu reclaimed %ld bytes",
		    (unsigned long)GC_gc_no - 1,
	   	    (long)GC_bytes_found);
      GC_log_printf(" ---> heapsize = %lu bytes\n",
      	        (unsigned long) GC_heapsize);
        /* Printf arguments may be pushed in funny places.  Clear the	*/
        /* space.							*/
      GC_log_printf("");
    }

    /* Check all debugged objects for consistency */
        if (GC_debugging_started) {
            (*GC_check_heap)();
        }
    
    IF_THREADS(GC_world_stopped = FALSE);
    START_WORLD();
    if (GC_print_stats) {
      GET_TIME(current_time);
      GC_log_printf("World-stopped marking took %lu msecs\n",
	            MS_TIME_DIFF(current_time,start_time));
    }
    return(TRUE);
}

/* Set all mark bits for the free list whose first entry is q	*/
void GC_set_fl_marks(ptr_t q)
{
   ptr_t p;
   struct hblk * h, * last_h = 0;
   hdr *hhdr;  /* gcc "might be uninitialized" warning is bogus. */
   IF_PER_OBJ(size_t sz;)
   unsigned bit_no;

   for (p = q; p != 0; p = obj_link(p)){
	h = HBLKPTR(p);
	if (h != last_h) {
	  last_h = h; 
	  hhdr = HDR(h);
	  IF_PER_OBJ(sz = hhdr->hb_sz;)
	}
	bit_no = MARK_BIT_NO((ptr_t)p - (ptr_t)h, sz);
	if (!mark_bit_from_hdr(hhdr, bit_no)) {
      	  set_mark_bit_from_hdr(hhdr, bit_no);
          ++hhdr -> hb_n_marks;
        }
   }
}

#ifdef GC_ASSERTIONS
/* Check that all mark bits for the free list whose first entry is q	*/
/* are set.								*/
void GC_check_fl_marks(ptr_t q)
{
   ptr_t p;

   for (p = q; p != 0; p = obj_link(p)){
	if (!GC_is_marked(p)) {
	    GC_err_printf("Unmarked object %p on list %p\n", p, q);
	    ABORT("Unmarked local free list entry.");
	}
   }
}
#endif

/* Clear all mark bits for the free list whose first entry is q	*/
/* Decrement GC_bytes_found by number of bytes on free list.	*/
void GC_clear_fl_marks(ptr_t q)
{
   ptr_t p;
   struct hblk * h, * last_h = 0;
   hdr *hhdr;
   size_t sz;
   unsigned bit_no;

   for (p = q; p != 0; p = obj_link(p)){
	h = HBLKPTR(p);
	if (h != last_h) {
	  last_h = h; 
	  hhdr = HDR(h);
	  sz = hhdr->hb_sz;  /* Normally set only once. */
	}
	bit_no = MARK_BIT_NO((ptr_t)p - (ptr_t)h, sz);
	if (mark_bit_from_hdr(hhdr, bit_no)) {
	  size_t n_marks = hhdr -> hb_n_marks - 1;
      	  clear_mark_bit_from_hdr(hhdr, bit_no);
#	  ifdef PARALLEL_MARK
	    /* Appr. count, don't decrement to zero! */
	    if (0 != n_marks) {
              hhdr -> hb_n_marks = n_marks;
	    }
#	  else
            hhdr -> hb_n_marks = n_marks;
#	  endif
        }
	GC_bytes_found -= sz;
   }
}

#if defined(GC_ASSERTIONS) && defined(THREADS) && defined(THREAD_LOCAL_ALLOC)
extern void GC_check_tls(void);
#endif

/* Finish up a collection.  Assumes lock is held, signals are disabled,	*/
/* but the world is otherwise running.					*/
void GC_finish_collection()
{
    CLOCK_TYPE start_time;
    CLOCK_TYPE finalize_time;
    CLOCK_TYPE done_time;
	
#   if defined(GC_ASSERTIONS) && defined(THREADS) \
       && defined(THREAD_LOCAL_ALLOC) && !defined(DBG_HDRS_ALL)
	/* Check that we marked some of our own data.  		*/
        /* FIXME: Add more checks.				*/
        GC_check_tls();
#   endif

    if (GC_print_stats)
      GET_TIME(start_time);

    GC_bytes_found = 0;
#   if defined(LINUX) && defined(__ELF__) && !defined(SMALL_CONFIG)
	if (getenv("GC_PRINT_ADDRESS_MAP") != 0) {
	  GC_print_address_map();
	}
#   endif
    COND_DUMP;
    if (GC_find_leak) {
      /* Mark all objects on the free list.  All objects should be */
      /* marked when we're done.				   */
	{
	  word size;		/* current object size		*/
	  unsigned kind;
	  ptr_t q;

	  for (kind = 0; kind < GC_n_kinds; kind++) {
	    for (size = 1; size <= MAXOBJGRANULES; size++) {
	      q = GC_obj_kinds[kind].ok_freelist[size];
	      if (q != 0) GC_set_fl_marks(q);
	    }
	  }
	}
	GC_start_reclaim(TRUE);
	  /* The above just checks; it doesn't really reclaim anything. */
    }

    GC_finalize();
#   ifdef STUBBORN_ALLOC
      GC_clean_changing_list();
#   endif

    if (GC_print_stats)
      GET_TIME(finalize_time);

    if (GC_print_back_height) {
#     ifdef MAKE_BACK_GRAPH
	GC_traverse_back_graph();
#     else
#	ifndef SMALL_CONFIG
	  GC_err_printf("Back height not available: "
		        "Rebuild collector with -DMAKE_BACK_GRAPH\n");
#  	endif
#     endif
    }

    /* Clear free list mark bits, in case they got accidentally marked   */
    /* (or GC_find_leak is set and they were intentionally marked).	 */
    /* Also subtract memory remaining from GC_bytes_found count.         */
    /* Note that composite objects on free list are cleared.             */
    /* Thus accidentally marking a free list is not a problem;  only     */
    /* objects on the list itself will be marked, and that's fixed here. */
      {
	word size;		/* current object size		*/
	ptr_t q;	/* pointer to current object	*/
	unsigned kind;

	for (kind = 0; kind < GC_n_kinds; kind++) {
	  for (size = 1; size <= MAXOBJGRANULES; size++) {
	    q = GC_obj_kinds[kind].ok_freelist[size];
	    if (q != 0) GC_clear_fl_marks(q);
	  }
	}
      }


    if (GC_print_stats == VERBOSE)
	GC_log_printf("Bytes recovered before sweep - f.l. count = %ld\n",
	          (long)GC_bytes_found);
    
    /* Reconstruct free lists to contain everything not marked */
        GC_start_reclaim(FALSE);
	if (GC_print_stats) {
	  GC_log_printf("Heap contains %lu pointer-containing "
		        "+ %lu pointer-free reachable bytes\n",
		        (unsigned long)GC_composite_in_use,
		        (unsigned long)GC_atomic_in_use);
	}
        if (GC_is_full_gc)  {
	    GC_used_heap_size_after_full = USED_HEAP_SIZE;
	    GC_need_full_gc = FALSE;
	} else {
	    GC_need_full_gc =
		 USED_HEAP_SIZE - GC_used_heap_size_after_full
		 > min_bytes_allocd();
	}

    if (GC_print_stats == VERBOSE) {
	GC_log_printf(
		  "Immediately reclaimed %ld bytes in heap of size %lu bytes",
	          (long)GC_bytes_found,
	          (unsigned long)GC_heapsize);
#	ifdef USE_MUNMAP
	  GC_log_printf("(%lu unmapped)", (unsigned long)GC_unmapped_bytes);
#	endif
	GC_log_printf("\n");
    }

    /* Reset or increment counters for next cycle */
      GC_n_attempts = 0;
      GC_is_full_gc = FALSE;
      GC_bytes_allocd_before_gc += GC_bytes_allocd;
      GC_non_gc_bytes_at_gc = GC_non_gc_bytes;
      GC_bytes_allocd = 0;
      GC_bytes_freed = 0;
      GC_finalizer_bytes_freed = 0;
      
#   ifdef USE_MUNMAP
      GC_unmap_old();
#   endif
    if (GC_print_stats) {
	GET_TIME(done_time);
	GC_log_printf("Finalize + initiate sweep took %lu + %lu msecs\n",
	              MS_TIME_DIFF(finalize_time,start_time),
	              MS_TIME_DIFF(done_time,finalize_time));
    }
}

/* Externally callable routine to invoke full, stop-world collection */
int GC_try_to_collect(GC_stop_func stop_func)
{
    int result;
    DCL_LOCK_STATE;
    
    if (!GC_is_initialized) GC_init();
    if (GC_debugging_started) GC_print_all_smashed();
    GC_INVOKE_FINALIZERS();
    LOCK();
    ENTER_GC();
    if (!GC_is_initialized) GC_init_inner();
    /* Minimize junk left in my registers */
      GC_noop(0,0,0,0,0,0);
    result = (int)GC_try_to_collect_inner(stop_func);
    EXIT_GC();
    UNLOCK();
    if(result) {
        if (GC_debugging_started) GC_print_all_smashed();
        GC_INVOKE_FINALIZERS();
    }
    return(result);
}

void GC_gcollect(void)
{
    (void)GC_try_to_collect(GC_never_stop_func);
    if (GC_have_errors) GC_print_all_errors();
}

word GC_n_heap_sects = 0;	/* Number of sections currently in heap. */

/*
 * Use the chunk of memory starting at p of size bytes as part of the heap.
 * Assumes p is HBLKSIZE aligned, and bytes is a multiple of HBLKSIZE.
 */
void GC_add_to_heap(struct hblk *p, size_t bytes)
{
    hdr * phdr;
    
    if (GC_n_heap_sects >= MAX_HEAP_SECTS) {
    	ABORT("Too many heap sections: Increase MAXHINCR or MAX_HEAP_SECTS");
    }
    phdr = GC_install_header(p);
    if (0 == phdr) {
    	/* This is extremely unlikely. Can't add it.  This will		*/
    	/* almost certainly result in a	0 return from the allocator,	*/
    	/* which is entirely appropriate.				*/
    	return;
    }
    GC_heap_sects[GC_n_heap_sects].hs_start = (ptr_t)p;
    GC_heap_sects[GC_n_heap_sects].hs_bytes = bytes;
    GC_n_heap_sects++;
    phdr -> hb_sz = bytes;
    phdr -> hb_flags = 0;
    GC_freehblk(p);
    GC_heapsize += bytes;
    if ((ptr_t)p <= (ptr_t)GC_least_plausible_heap_addr
        || GC_least_plausible_heap_addr == 0) {
        GC_least_plausible_heap_addr = (void *)((ptr_t)p - sizeof(word));
        	/* Making it a little smaller than necessary prevents	*/
        	/* us from getting a false hit from the variable	*/
        	/* itself.  There's some unintentional reflection	*/
        	/* here.						*/
    }
    if ((ptr_t)p + bytes >= (ptr_t)GC_greatest_plausible_heap_addr) {
        GC_greatest_plausible_heap_addr = (void *)((ptr_t)p + bytes);
    }
}

# if !defined(NO_DEBUGGING)
void GC_print_heap_sects(void)
{
    unsigned i;
    
    GC_printf("Total heap size: %lu\n", (unsigned long) GC_heapsize);
    for (i = 0; i < GC_n_heap_sects; i++) {
        ptr_t start = GC_heap_sects[i].hs_start;
        size_t len = GC_heap_sects[i].hs_bytes;
        struct hblk *h;
        unsigned nbl = 0;
        
    	GC_printf("Section %d from %p to %p ", i,
    		   start, start + len);
    	for (h = (struct hblk *)start; h < (struct hblk *)(start + len); h++) {
    	    if (GC_is_black_listed(h, HBLKSIZE)) nbl++;
    	}
    	GC_printf("%lu/%lu blacklisted\n", (unsigned long)nbl,
    		   (unsigned long)(len/HBLKSIZE));
    }
}
# endif

void * GC_least_plausible_heap_addr = (void *)ONES;
void * GC_greatest_plausible_heap_addr = 0;

static INLINE ptr_t GC_max(ptr_t x, ptr_t y)
{
    return(x > y? x : y);
}

static INLINE ptr_t GC_min(ptr_t x, ptr_t y)
{
    return(x < y? x : y);
}

void GC_set_max_heap_size(GC_word n)
{
    GC_max_heapsize = n;
}

GC_word GC_max_retries = 0;

/*
 * this explicitly increases the size of the heap.  It is used
 * internally, but may also be invoked from GC_expand_hp by the user.
 * The argument is in units of HBLKSIZE.
 * Tiny values of n are rounded up.
 * Returns FALSE on failure.
 */
GC_bool GC_expand_hp_inner(word n)
{
    word bytes;
    struct hblk * space;
    word expansion_slop;	/* Number of bytes by which we expect the */
    				/* heap to expand soon.			  */

    if (n < MINHINCR) n = MINHINCR;
    bytes = n * HBLKSIZE;
    /* Make sure bytes is a multiple of GC_page_size */
      {
	word mask = GC_page_size - 1;
	bytes += mask;
	bytes &= ~mask;
      }
    
    if (GC_max_heapsize != 0 && GC_heapsize + bytes > GC_max_heapsize) {
        /* Exceeded self-imposed limit */
        return(FALSE);
    }
    space = GET_MEM(bytes);
    if( space == 0 ) {
	if (GC_print_stats) {
	    GC_log_printf("Failed to expand heap by %ld bytes\n",
		          (unsigned long)bytes);
	}
	return(FALSE);
    }
    if (GC_print_stats) {
	GC_log_printf("Increasing heap size by %lu after %lu allocated bytes\n",
	              (unsigned long)bytes,
	              (unsigned long)GC_bytes_allocd);
    }
    expansion_slop = min_bytes_allocd() + 4*MAXHINCR*HBLKSIZE;
    if ((GC_last_heap_addr == 0 && !((word)space & SIGNB))
        || (GC_last_heap_addr != 0 && GC_last_heap_addr < (ptr_t)space)) {
        /* Assume the heap is growing up */
        GC_greatest_plausible_heap_addr =
            (void *)GC_max((ptr_t)GC_greatest_plausible_heap_addr,
                           (ptr_t)space + bytes + expansion_slop);
    } else {
        /* Heap is growing down */
        GC_least_plausible_heap_addr =
            (void *)GC_min((ptr_t)GC_least_plausible_heap_addr,
                           (ptr_t)space - expansion_slop);
    }
#   if defined(LARGE_CONFIG)
      if (((ptr_t)GC_greatest_plausible_heap_addr <= (ptr_t)space + bytes
           || (ptr_t)GC_least_plausible_heap_addr >= (ptr_t)space)
	  && GC_heapsize > 0) {
	/* GC_add_to_heap will fix this, but ... */
	WARN("Too close to address space limit: blacklisting ineffective\n", 0);
      }
#   endif
    GC_prev_heap_addr = GC_last_heap_addr;
    GC_last_heap_addr = (ptr_t)space;
    GC_add_to_heap(space, bytes);
    /* Force GC before we are likely to allocate past expansion_slop */
      GC_collect_at_heapsize =
         GC_heapsize + expansion_slop - 2*MAXHINCR*HBLKSIZE;
#     if defined(LARGE_CONFIG)
        if (GC_collect_at_heapsize < GC_heapsize /* wrapped */)
         GC_collect_at_heapsize = (word)(-1);
#     endif
    return(TRUE);
}

/* Really returns a bool, but it's externally visible, so that's clumsy. */
/* Arguments is in bytes.						*/
int GC_expand_hp(size_t bytes)
{
    int result;
    DCL_LOCK_STATE;
    
    LOCK();
    if (!GC_is_initialized) GC_init_inner();
    result = (int)GC_expand_hp_inner(divHBLKSZ((word)bytes));
    if (result) GC_requested_heapsize += bytes;
    UNLOCK();
    return(result);
}

unsigned GC_fail_count = 0;  
			/* How many consecutive GC/expansion failures?	*/
			/* Reset by GC_allochblk.			*/

GC_bool GC_collect_or_expand(word needed_blocks, GC_bool ignore_off_page)
{
    if (!GC_incremental && !GC_dont_gc &&
	((GC_dont_expand && GC_bytes_allocd > 0) || GC_should_collect())) {
      GC_gcollect_inner();
    } else {
      word blocks_to_get = GC_heapsize/(HBLKSIZE*GC_free_space_divisor)
      			   + needed_blocks;
      
      if (blocks_to_get > MAXHINCR) {
          word slop;
          
	  /* Get the minimum required to make it likely that we		*/
	  /* can satisfy the current request in the presence of black-	*/
	  /* listing.  This will probably be more than MAXHINCR.	*/
          if (ignore_off_page) {
              slop = 4;
          } else {
	      slop = 2*divHBLKSZ(BL_LIMIT);
	      if (slop > needed_blocks) slop = needed_blocks;
	  }
          if (needed_blocks + slop > MAXHINCR) {
              blocks_to_get = needed_blocks + slop;
          } else {
              blocks_to_get = MAXHINCR;
          }
      }
      if (!GC_expand_hp_inner(blocks_to_get)
        && !GC_expand_hp_inner(needed_blocks)) {
      	if (GC_fail_count++ < GC_max_retries) {
      	    WARN("Out of Memory!  Trying to continue ...\n", 0);
	    GC_gcollect_inner();
	} else {
#	    if !defined(AMIGA) || !defined(GC_AMIGA_FASTALLOC)
	      WARN("Out of Memory!  Returning NIL!\n", 0);
#	    endif
	    return(FALSE);
	}
      } else {
          if (GC_fail_count && GC_print_stats) {
	      GC_printf("Memory available again ...\n");
	  }
      }
    }
    return(TRUE);
}

/*
 * Make sure the object free list for size gran (in granules) is not empty.
 * Return a pointer to the first object on the free list.
 * The object MUST BE REMOVED FROM THE FREE LIST BY THE CALLER.
 * Assumes we hold the allocator lock and signals are disabled.
 *
 */
ptr_t GC_allocobj(size_t gran, int kind)
{
    void ** flh = &(GC_obj_kinds[kind].ok_freelist[gran]);
    GC_bool tried_minor = FALSE;
    
    if (gran == 0) return(0);

    while (*flh == 0) {
      ENTER_GC();
      /* Do our share of marking work */
        if(TRUE_INCREMENTAL) GC_collect_a_little_inner(1);
      /* Sweep blocks for objects of this size */
        GC_continue_reclaim(gran, kind);
      EXIT_GC();
      if (*flh == 0) {
        GC_new_hblk(gran, kind);
      }
      if (*flh == 0) {
        ENTER_GC();
	if (GC_incremental && GC_time_limit == GC_TIME_UNLIMITED
	    && ! tried_minor ) {
	    GC_collect_a_little_inner(1);
	    tried_minor = TRUE;
	} else {
          if (!GC_collect_or_expand((word)1,FALSE)) {
	    EXIT_GC();
	    return(0);
	  }
	}
	EXIT_GC();
      }
    }
    /* Successful allocation; reset failure count.	*/
    GC_fail_count = 0;
    
    return(*flh);
}
