/*
 * Copyright 1988, 1989 Hans-J. Boehm, Alan J. Demers
 * Copyright (c) 1991-1996 by Xerox Corporation.  All rights reserved.
 * Copyright (c) 1996-1999 by Silicon Graphics.  All rights reserved.
 * Copyright (C) 2007 Free Software Foundation, Inc

 * THIS MATERIAL IS PROVIDED AS IS, WITH ABSOLUTELY NO WARRANTY EXPRESSED
 * OR IMPLIED.  ANY USE IS AT YOUR OWN RISK.
 *
 * Permission is hereby granted to use or copy this program
 * for any purpose,  provided the above notices are retained on all copies.
 * Permission to modify the code and to distribute modified code is granted,
 * provided the above notices are retained, and a notice that the code was
 * modified is included with the above copyright notice.
 */
/* Boehm, February 1, 1996 1:19 pm PST */
# define I_HIDE_POINTERS
# include "private/gc_pmark.h"

# ifdef FINALIZE_ON_DEMAND
    int GC_finalize_on_demand = 1;
# else
    int GC_finalize_on_demand = 0;
# endif

# ifdef JAVA_FINALIZATION
    int GC_java_finalization = 1;
# else
    int GC_java_finalization = 0;
# endif

/* Type of mark procedure used for marking from finalizable object.	*/
/* This procedure normally does not mark the object, only its		*/
/* descendents.								*/
typedef void finalization_mark_proc(/* ptr_t finalizable_obj_ptr */); 

# define HASH3(addr,size,log_size) \
    ((((word)(addr) >> 3) ^ ((word)(addr) >> (3+(log_size)))) \
    & ((size) - 1))
#define HASH2(addr,log_size) HASH3(addr, 1 << log_size, log_size)

struct hash_chain_entry {
    word hidden_key;
    struct hash_chain_entry * next;
};

unsigned GC_finalization_failures = 0;
	/* Number of finalization requests that failed for lack of memory. */

static struct disappearing_link {
    struct hash_chain_entry prolog;
#   define dl_hidden_link prolog.hidden_key
				/* Field to be cleared.		*/
#   define dl_next(x) (struct disappearing_link *)((x) -> prolog.next)
#   define dl_set_next(x,y) (x) -> prolog.next = (struct hash_chain_entry *)(y)

    word dl_hidden_obj;		/* Pointer to object base	*/
} **dl_head = 0;

static signed_word log_dl_table_size = -1;
			/* Binary log of				*/
			/* current size of array pointed to by dl_head.	*/
			/* -1 ==> size is 0.				*/

word GC_dl_entries = 0;	/* Number of entries currently in disappearing	*/
			/* link table.					*/

static struct finalizable_object {
    struct hash_chain_entry prolog;
#   define fo_hidden_base prolog.hidden_key
				/* Pointer to object base.	*/
				/* No longer hidden once object */
				/* is on finalize_now queue.	*/
#   define fo_next(x) (struct finalizable_object *)((x) -> prolog.next)
#   define fo_set_next(x,y) (x) -> prolog.next = (struct hash_chain_entry *)(y)
    GC_finalization_proc fo_fn;	/* Finalizer.			*/
    ptr_t fo_client_data;
    word fo_object_size;	/* In bytes.			*/
    finalization_mark_proc * fo_mark_proc;	/* Mark-through procedure */
} **fo_head = 0;

struct finalizable_object * GC_finalize_now = 0;
	/* LIst of objects that should be finalized now.	*/

static signed_word log_fo_table_size = -1;

word GC_fo_entries = 0;

void GC_push_finalizer_structures(void)
{
    GC_push_all((ptr_t)(&dl_head), (ptr_t)(&dl_head) + sizeof(word));
    GC_push_all((ptr_t)(&fo_head), (ptr_t)(&fo_head) + sizeof(word));
    GC_push_all((ptr_t)(&GC_finalize_now),
		(ptr_t)(&GC_finalize_now) + sizeof(word));
}

/* Double the size of a hash table. *size_ptr is the log of its current	*/
/* size.  May be a noop.						*/
/* *table is a pointer to an array of hash headers.  If we succeed, we	*/
/* update both *table and *log_size_ptr.				*/
/* Lock is held.  Signals are disabled.					*/
void GC_grow_table(struct hash_chain_entry ***table,
		   signed_word *log_size_ptr)
{
    register word i;
    register struct hash_chain_entry *p;
    signed_word log_old_size = *log_size_ptr;
    signed_word log_new_size = log_old_size + 1;
    word old_size = ((log_old_size == -1)? 0: (1 << log_old_size));
    word new_size = (word)1 << log_new_size;
    /* FIXME: Power of 2 size often gets rounded up to one more page. */
    struct hash_chain_entry **new_table = (struct hash_chain_entry **)
    	GC_INTERNAL_MALLOC_IGNORE_OFF_PAGE(
    		(size_t)new_size * sizeof(struct hash_chain_entry *), NORMAL);
    
    if (new_table == 0) {
    	if (table == 0) {
    	    ABORT("Insufficient space for initial table allocation");
    	} else {
    	    return;
    	}
    }
    for (i = 0; i < old_size; i++) {
      p = (*table)[i];
      while (p != 0) {
        ptr_t real_key = (ptr_t)REVEAL_POINTER(p -> hidden_key);
        struct hash_chain_entry *next = p -> next;
        size_t new_hash = HASH3(real_key, new_size, log_new_size);
        
        p -> next = new_table[new_hash];
        new_table[new_hash] = p;
        p = next;
      }
    }
    *log_size_ptr = log_new_size;
    *table = new_table;
}

int GC_register_disappearing_link(void * * link)
{
    ptr_t base;
    
    base = (ptr_t)GC_base((void *)link);
    if (base == 0)
    	ABORT("Bad arg to GC_register_disappearing_link");
    return(GC_general_register_disappearing_link(link, base));
}

int GC_general_register_disappearing_link(void * * link, void * obj)
{
    struct disappearing_link *curr_dl;
    size_t index;
    struct disappearing_link * new_dl;
    DCL_LOCK_STATE;
    
    if ((word)link & (ALIGNMENT-1))
    	ABORT("Bad arg to GC_general_register_disappearing_link");
#   ifdef THREADS
    	LOCK();
#   endif
    if (log_dl_table_size == -1
        || GC_dl_entries > ((word)1 << log_dl_table_size)) {
    	GC_grow_table((struct hash_chain_entry ***)(&dl_head),
    		      &log_dl_table_size);
	if (GC_print_stats) {
	    GC_log_printf("Grew dl table to %u entries\n",
	    	      (1 << log_dl_table_size));
	}
    }
    index = HASH2(link, log_dl_table_size);
    curr_dl = dl_head[index];
    for (curr_dl = dl_head[index]; curr_dl != 0; curr_dl = dl_next(curr_dl)) {
        if (curr_dl -> dl_hidden_link == HIDE_POINTER(link)) {
            curr_dl -> dl_hidden_obj = HIDE_POINTER(obj);
#	    ifdef THREADS
                UNLOCK();
#	    endif
            return(1);
        }
    }
    new_dl = (struct disappearing_link *)
    	GC_INTERNAL_MALLOC(sizeof(struct disappearing_link),NORMAL);
    if (0 == new_dl) {
#     ifdef THREADS
	UNLOCK();
#     endif
      new_dl = (struct disappearing_link *)
	      GC_oom_fn(sizeof(struct disappearing_link));
      if (0 == new_dl) {
	GC_finalization_failures++;
	return(0);
      }
      /* It's not likely we'll make it here, but ... */
#     ifdef THREADS
	LOCK();
#     endif
    }
    new_dl -> dl_hidden_obj = HIDE_POINTER(obj);
    new_dl -> dl_hidden_link = HIDE_POINTER(link);
    dl_set_next(new_dl, dl_head[index]);
    dl_head[index] = new_dl;
    GC_dl_entries++;
#   ifdef THREADS
        UNLOCK();
#   endif
    return(0);
}

int GC_unregister_disappearing_link(void * * link)
{
    struct disappearing_link *curr_dl, *prev_dl;
    size_t index;
    DCL_LOCK_STATE;
    
    LOCK();
    index = HASH2(link, log_dl_table_size);
    if (((word)link & (ALIGNMENT-1))) goto out;
    prev_dl = 0; curr_dl = dl_head[index];
    while (curr_dl != 0) {
        if (curr_dl -> dl_hidden_link == HIDE_POINTER(link)) {
            if (prev_dl == 0) {
                dl_head[index] = dl_next(curr_dl);
            } else {
                dl_set_next(prev_dl, dl_next(curr_dl));
            }
            GC_dl_entries--;
            UNLOCK();
#	    ifdef DBG_HDRS_ALL
	      dl_set_next(curr_dl, 0);
#	    else
              GC_free((void *)curr_dl);
#	    endif
            return(1);
        }
        prev_dl = curr_dl;
        curr_dl = dl_next(curr_dl);
    }
out:
    UNLOCK();
    return(0);
}

/* Possible finalization_marker procedures.  Note that mark stack	*/
/* overflow is handled by the caller, and is not a disaster.		*/
GC_API void GC_normal_finalize_mark_proc(ptr_t p)
{
    hdr * hhdr = HDR(p);
    
    PUSH_OBJ(p, hhdr, GC_mark_stack_top,
	     &(GC_mark_stack[GC_mark_stack_size]));
}

/* This only pays very partial attention to the mark descriptor.	*/
/* It does the right thing for normal and atomic objects, and treats	*/
/* most others as normal.						*/
GC_API void GC_ignore_self_finalize_mark_proc(ptr_t p)
{
    hdr * hhdr = HDR(p);
    word descr = hhdr -> hb_descr;
    ptr_t q, r;
    ptr_t scan_limit;
    ptr_t target_limit = p + hhdr -> hb_sz - 1;
    
    if ((descr & GC_DS_TAGS) == GC_DS_LENGTH) {
       scan_limit = p + descr - sizeof(word);
    } else {
       scan_limit = target_limit + 1 - sizeof(word);
    }
    for (q = p; q <= scan_limit; q += ALIGNMENT) {
    	r = *(ptr_t *)q;
    	if (r < p || r > target_limit) {
    	    GC_PUSH_ONE_HEAP(r, q);
    	}
    }
}

/*ARGSUSED*/
GC_API void GC_null_finalize_mark_proc(ptr_t p)
{
}

/* Possible finalization_marker procedures.  Note that mark stack	*/
/* overflow is handled by the caller, and is not a disaster.		*/

/* GC_unreachable_finalize_mark_proc is an alias for normal marking,	*/
/* but it is explicitly tested for, and triggers different		*/
/* behavior.  Objects registered in this way are not finalized		*/
/* if they are reachable by other finalizable objects, eve if those	*/
/* other objects specify no ordering.					*/
GC_API void GC_unreachable_finalize_mark_proc(ptr_t p)
{
    GC_normal_finalize_mark_proc(p);
}



/* Register a finalization function.  See gc.h for details.	*/
/* in the nonthreads case, we try to avoid disabling signals,	*/
/* since it can be expensive.  Threads packages typically	*/
/* make it cheaper.						*/
/* The last parameter is a procedure that determines		*/
/* marking for finalization ordering.  Any objects marked	*/
/* by that procedure will be guaranteed to not have been	*/
/* finalized when this finalizer is invoked.			*/
GC_API void GC_register_finalizer_inner(void * obj,
					GC_finalization_proc fn, void *cd,
					GC_finalization_proc *ofn, void **ocd,
					finalization_mark_proc mp)
{
    ptr_t base;
    struct finalizable_object * curr_fo, * prev_fo;
    size_t index;
    struct finalizable_object *new_fo;
    hdr *hhdr;
    DCL_LOCK_STATE;

#   ifdef THREADS
	LOCK();
#   endif
    if (log_fo_table_size == -1
        || GC_fo_entries > ((word)1 << log_fo_table_size)) {
    	GC_grow_table((struct hash_chain_entry ***)(&fo_head),
    		      &log_fo_table_size);
	if (GC_print_stats) {
	    GC_log_printf("Grew fo table to %u entries\n",
	    	          (1 << log_fo_table_size));
	}
    }
    /* in the THREADS case signals are disabled and we hold allocation	*/
    /* lock; otherwise neither is true.  Proceed carefully.		*/
    base = (ptr_t)obj;
    index = HASH2(base, log_fo_table_size);
    prev_fo = 0; curr_fo = fo_head[index];
    while (curr_fo != 0) {
        GC_ASSERT(GC_size(curr_fo) >= sizeof(struct finalizable_object));
        if (curr_fo -> fo_hidden_base == HIDE_POINTER(base)) {
            /* Interruption by a signal in the middle of this	*/
            /* should be safe.  The client may see only *ocd	*/
            /* updated, but we'll declare that to be his	*/
            /* problem.						*/
            if (ocd) *ocd = (void *) (curr_fo -> fo_client_data);
            if (ofn) *ofn = curr_fo -> fo_fn;
            /* Delete the structure for base. */
                if (prev_fo == 0) {
                  fo_head[index] = fo_next(curr_fo);
                } else {
                  fo_set_next(prev_fo, fo_next(curr_fo));
                }
            if (fn == 0) {
                GC_fo_entries--;
                  /* May not happen if we get a signal.  But a high	*/
                  /* estimate will only make the table larger than	*/
                  /* necessary.						*/
#		if !defined(THREADS) && !defined(DBG_HDRS_ALL)
                  GC_free((void *)curr_fo);
#		endif
            } else {
                curr_fo -> fo_fn = fn;
                curr_fo -> fo_client_data = (ptr_t)cd;
                curr_fo -> fo_mark_proc = mp;
		/* Reinsert it.  We deleted it first to maintain	*/
		/* consistency in the event of a signal.		*/
		if (prev_fo == 0) {
                  fo_head[index] = curr_fo;
                } else {
                  fo_set_next(prev_fo, curr_fo);
                }
            }
#	    ifdef THREADS
                UNLOCK();
#	    endif
            return;
        }
        prev_fo = curr_fo;
        curr_fo = fo_next(curr_fo);
    }
    if (ofn) *ofn = 0;
    if (ocd) *ocd = 0;
    if (fn == 0) {
#	ifdef THREADS
            UNLOCK();
#	endif
        return;
    }
    GET_HDR(base, hhdr);
    if (0 == hhdr) {
      /* We won't collect it, hence finalizer wouldn't be run. */
#     ifdef THREADS
          UNLOCK();
#     endif
      return;
    }
    new_fo = (struct finalizable_object *)
    	GC_INTERNAL_MALLOC(sizeof(struct finalizable_object),NORMAL);
    GC_ASSERT(GC_size(new_fo) >= sizeof(struct finalizable_object));
    if (EXPECT(0 == new_fo, FALSE)) {
#     ifdef THREADS
	UNLOCK();
#     endif
      new_fo = (struct finalizable_object *)
	      GC_oom_fn(sizeof(struct finalizable_object));
      if (0 == new_fo) {
	GC_finalization_failures++;
	return;
      }
      /* It's not likely we'll make it here, but ... */
#     ifdef THREADS
	LOCK();
#     endif
    }
    new_fo -> fo_hidden_base = (word)HIDE_POINTER(base);
    new_fo -> fo_fn = fn;
    new_fo -> fo_client_data = (ptr_t)cd;
    new_fo -> fo_object_size = hhdr -> hb_sz;
    new_fo -> fo_mark_proc = mp;
    fo_set_next(new_fo, fo_head[index]);
    GC_fo_entries++;
    fo_head[index] = new_fo;
#   ifdef THREADS
        UNLOCK();
#   endif
}

void GC_register_finalizer(void * obj,
			       GC_finalization_proc fn, void * cd,
			       GC_finalization_proc *ofn, void ** ocd)
{
    GC_register_finalizer_inner(obj, fn, cd, ofn,
    				ocd, GC_normal_finalize_mark_proc);
}

void GC_register_finalizer_ignore_self(void * obj,
			       GC_finalization_proc fn, void * cd,
			       GC_finalization_proc *ofn, void ** ocd)
{
    GC_register_finalizer_inner(obj, fn, cd, ofn,
    				ocd, GC_ignore_self_finalize_mark_proc);
}

void GC_register_finalizer_no_order(void * obj,
			       GC_finalization_proc fn, void * cd,
			       GC_finalization_proc *ofn, void ** ocd)
{
    GC_register_finalizer_inner(obj, fn, cd, ofn,
    				ocd, GC_null_finalize_mark_proc);
}

static GC_bool need_unreachable_finalization = FALSE;
	/* Avoid the work if this isn't used.	*/

void GC_register_finalizer_unreachable(void * obj,
			       GC_finalization_proc fn, void * cd,
			       GC_finalization_proc *ofn, void ** ocd)
{
    need_unreachable_finalization = TRUE;
    GC_ASSERT(GC_java_finalization);
    GC_register_finalizer_inner(obj, fn, cd, ofn,
    				ocd, GC_unreachable_finalize_mark_proc);
}

#ifndef NO_DEBUGGING
void GC_dump_finalization(void)
{
    struct disappearing_link * curr_dl;
    struct finalizable_object * curr_fo;
    ptr_t real_ptr, real_link;
    int dl_size = (log_dl_table_size == -1 ) ? 0 : (1 << log_dl_table_size);
    int fo_size = (log_fo_table_size == -1 ) ? 0 : (1 << log_fo_table_size);
    int i;

    GC_printf("Disappearing links:\n");
    for (i = 0; i < dl_size; i++) {
      for (curr_dl = dl_head[i]; curr_dl != 0; curr_dl = dl_next(curr_dl)) {
        real_ptr = (ptr_t)REVEAL_POINTER(curr_dl -> dl_hidden_obj);
        real_link = (ptr_t)REVEAL_POINTER(curr_dl -> dl_hidden_link);
        GC_printf("Object: %p, Link:%p\n", real_ptr, real_link);
      }
    }
    GC_printf("Finalizers:\n");
    for (i = 0; i < fo_size; i++) {
      for (curr_fo = fo_head[i]; curr_fo != 0; curr_fo = fo_next(curr_fo)) {
        real_ptr = (ptr_t)REVEAL_POINTER(curr_fo -> fo_hidden_base);
        GC_printf("Finalizable object: %p\n", real_ptr);
      }
    }
}
#endif

/* Called with world stopped.  Cause disappearing links to disappear,	*/
/* and invoke finalizers.						*/
void GC_finalize(void)
{
    struct disappearing_link * curr_dl, * prev_dl, * next_dl;
    struct finalizable_object * curr_fo, * prev_fo, * next_fo;
    ptr_t real_ptr, real_link;
    size_t i;
    size_t dl_size = (log_dl_table_size == -1 ) ? 0 : (1 << log_dl_table_size);
    size_t fo_size = (log_fo_table_size == -1 ) ? 0 : (1 << log_fo_table_size);
    
  /* Make disappearing links disappear */
    for (i = 0; i < dl_size; i++) {
      curr_dl = dl_head[i];
      prev_dl = 0;
      while (curr_dl != 0) {
        real_ptr = (ptr_t)REVEAL_POINTER(curr_dl -> dl_hidden_obj);
        real_link = (ptr_t)REVEAL_POINTER(curr_dl -> dl_hidden_link);
        if (!GC_is_marked(real_ptr)) {
            *(word *)real_link = 0;
            next_dl = dl_next(curr_dl);
            if (prev_dl == 0) {
                dl_head[i] = next_dl;
            } else {
                dl_set_next(prev_dl, next_dl);
            }
            GC_clear_mark_bit((ptr_t)curr_dl);
            GC_dl_entries--;
            curr_dl = next_dl;
        } else {
            prev_dl = curr_dl;
            curr_dl = dl_next(curr_dl);
        }
      }
    }
  /* Mark all objects reachable via chains of 1 or more pointers	*/
  /* from finalizable objects.						*/
    GC_ASSERT(GC_mark_state == MS_NONE);
    for (i = 0; i < fo_size; i++) {
      for (curr_fo = fo_head[i]; curr_fo != 0; curr_fo = fo_next(curr_fo)) {
        GC_ASSERT(GC_size(curr_fo) >= sizeof(struct finalizable_object));
        real_ptr = (ptr_t)REVEAL_POINTER(curr_fo -> fo_hidden_base);
        if (!GC_is_marked(real_ptr)) {
	    GC_MARKED_FOR_FINALIZATION(real_ptr);
            GC_MARK_FO(real_ptr, curr_fo -> fo_mark_proc);
            if (GC_is_marked(real_ptr)) {
                WARN("Finalization cycle involving %lx\n", real_ptr);
            }
        }
      }
    }
  /* Enqueue for finalization all objects that are still		*/
  /* unreachable.							*/
    GC_bytes_finalized = 0;
    for (i = 0; i < fo_size; i++) {
      curr_fo = fo_head[i];
      prev_fo = 0;
      while (curr_fo != 0) {
        real_ptr = (ptr_t)REVEAL_POINTER(curr_fo -> fo_hidden_base);
        if (!GC_is_marked(real_ptr)) {
	    if (!GC_java_finalization) {
              GC_set_mark_bit(real_ptr);
	    }
            /* Delete from hash table */
              next_fo = fo_next(curr_fo);
              if (prev_fo == 0) {
                fo_head[i] = next_fo;
              } else {
                fo_set_next(prev_fo, next_fo);
              }
              GC_fo_entries--;
            /* Add to list of objects awaiting finalization.	*/
              fo_set_next(curr_fo, GC_finalize_now);
              GC_finalize_now = curr_fo;
              /* unhide object pointer so any future collections will	*/
              /* see it.						*/
              curr_fo -> fo_hidden_base = 
              		(word) REVEAL_POINTER(curr_fo -> fo_hidden_base);
              GC_bytes_finalized +=
                 	curr_fo -> fo_object_size
              		+ sizeof(struct finalizable_object);
	    GC_ASSERT(GC_is_marked(GC_base((ptr_t)curr_fo)));
            curr_fo = next_fo;
        } else {
            prev_fo = curr_fo;
            curr_fo = fo_next(curr_fo);
        }
      }
    }

  if (GC_java_finalization) {
    /* make sure we mark everything reachable from objects finalized
       using the no_order mark_proc */
      for (curr_fo = GC_finalize_now; 
  	 curr_fo != NULL; curr_fo = fo_next(curr_fo)) {
  	real_ptr = (ptr_t)curr_fo -> fo_hidden_base;
  	if (!GC_is_marked(real_ptr)) {
  	    if (curr_fo -> fo_mark_proc == GC_null_finalize_mark_proc) {
  	        GC_MARK_FO(real_ptr, GC_normal_finalize_mark_proc);
  	    }
	    if (curr_fo -> fo_mark_proc != GC_unreachable_finalize_mark_proc) {
		GC_set_mark_bit(real_ptr);
	    }
  	}
      }

    /* now revive finalize-when-unreachable objects reachable from
       other finalizable objects */
      if (need_unreachable_finalization) {
        curr_fo = GC_finalize_now;
        prev_fo = 0;
        while (curr_fo != 0) {
	  next_fo = fo_next(curr_fo);
	  if (curr_fo -> fo_mark_proc == GC_unreachable_finalize_mark_proc) {
	    real_ptr = (ptr_t)curr_fo -> fo_hidden_base;
	    if (!GC_is_marked(real_ptr)) {
	      GC_set_mark_bit(real_ptr);
	    } else {
	      if (prev_fo == 0)
		GC_finalize_now = next_fo;
	      else
		fo_set_next(prev_fo, next_fo);

              curr_fo -> fo_hidden_base =
              		(word) HIDE_POINTER(curr_fo -> fo_hidden_base);
              GC_bytes_finalized -=
                 	curr_fo -> fo_object_size + sizeof(struct finalizable_object);

	      i = HASH2(real_ptr, log_fo_table_size);
	      fo_set_next (curr_fo, fo_head[i]);
	      GC_fo_entries++;
	      fo_head[i] = curr_fo;
	      curr_fo = prev_fo;
	    }
	  }
	  prev_fo = curr_fo;
	  curr_fo = next_fo;
        }
      }
  }

  /* Remove dangling disappearing links. */
    for (i = 0; i < dl_size; i++) {
      curr_dl = dl_head[i];
      prev_dl = 0;
      while (curr_dl != 0) {
        real_link = GC_base((ptr_t)REVEAL_POINTER(curr_dl -> dl_hidden_link));
        if (real_link != 0 && !GC_is_marked(real_link)) {
            next_dl = dl_next(curr_dl);
            if (prev_dl == 0) {
                dl_head[i] = next_dl;
            } else {
                dl_set_next(prev_dl, next_dl);
            }
            GC_clear_mark_bit((ptr_t)curr_dl);
            GC_dl_entries--;
            curr_dl = next_dl;
        } else {
            prev_dl = curr_dl;
            curr_dl = dl_next(curr_dl);
        }
      }
    }
}

#ifndef JAVA_FINALIZATION_NOT_NEEDED

/* Enqueue all remaining finalizers to be run - Assumes lock is
 * held, and signals are disabled */
void GC_enqueue_all_finalizers(void)
{
    struct finalizable_object * curr_fo, * prev_fo, * next_fo;
    ptr_t real_ptr;
    register int i;
    int fo_size;
    
    fo_size = (log_fo_table_size == -1 ) ? 0 : (1 << log_fo_table_size);
    GC_bytes_finalized = 0;
    for (i = 0; i < fo_size; i++) {
        curr_fo = fo_head[i];
        prev_fo = 0;
      while (curr_fo != 0) {
          real_ptr = (ptr_t)REVEAL_POINTER(curr_fo -> fo_hidden_base);
          GC_MARK_FO(real_ptr, GC_normal_finalize_mark_proc);
          GC_set_mark_bit(real_ptr);
 
          /* Delete from hash table */
          next_fo = fo_next(curr_fo);
          if (prev_fo == 0) {
              fo_head[i] = next_fo;
          } else {
              fo_set_next(prev_fo, next_fo);
          }
          GC_fo_entries--;

          /* Add to list of objects awaiting finalization.	*/
          fo_set_next(curr_fo, GC_finalize_now);
          GC_finalize_now = curr_fo;

          /* unhide object pointer so any future collections will	*/
          /* see it.						*/
          curr_fo -> fo_hidden_base = 
        		(word) REVEAL_POINTER(curr_fo -> fo_hidden_base);

          GC_bytes_finalized +=
           	curr_fo -> fo_object_size + sizeof(struct finalizable_object);
          curr_fo = next_fo;
        }
    }

    return;
}

/* Invoke all remaining finalizers that haven't yet been run. 
 * This is needed for strict compliance with the Java standard, 
 * which can make the runtime guarantee that all finalizers are run.
 * Unfortunately, the Java standard implies we have to keep running
 * finalizers until there are no more left, a potential infinite loop.
 * YUCK.
 * Note that this is even more dangerous than the usual Java
 * finalizers, in that objects reachable from static variables
 * may have been finalized when these finalizers are run.
 * Finalizers run at this point must be prepared to deal with a
 * mostly broken world.
 * This routine is externally callable, so is called without 
 * the allocation lock. 
 */
GC_API void GC_finalize_all(void)
{
    DCL_LOCK_STATE;

    LOCK();
    while (GC_fo_entries > 0) {
      GC_enqueue_all_finalizers();
      UNLOCK();
      GC_INVOKE_FINALIZERS();
      LOCK();
    }
    UNLOCK();
}
#endif

/* Returns true if it is worth calling GC_invoke_finalizers. (Useful if	*/
/* finalizers can only be called from some kind of `safe state' and	*/
/* getting into that safe state is expensive.)				*/
int GC_should_invoke_finalizers(void)
{
    return GC_finalize_now != 0;
}

/* Invoke finalizers for all objects that are ready to be finalized.	*/
/* Should be called without allocation lock.				*/
int GC_invoke_finalizers(void)
{
    struct finalizable_object * curr_fo;
    int count = 0;
    word bytes_freed_before;
    DCL_LOCK_STATE;
    
    while (GC_finalize_now != 0) {
#	ifdef THREADS
	    LOCK();
#	endif
	if (count == 0) {
	    bytes_freed_before = GC_bytes_freed;
	    /* Don't do this outside, since we need the lock. */
	}
    	curr_fo = GC_finalize_now;
#	ifdef THREADS
 	    if (curr_fo != 0) GC_finalize_now = fo_next(curr_fo);
	    UNLOCK();
	    if (curr_fo == 0) break;
#	else
	    GC_finalize_now = fo_next(curr_fo);
#	endif
 	fo_set_next(curr_fo, 0);
    	(*(curr_fo -> fo_fn))((ptr_t)(curr_fo -> fo_hidden_base),
    			      curr_fo -> fo_client_data);
    	curr_fo -> fo_client_data = 0;
	++count;
#	ifdef UNDEFINED
	    /* This is probably a bad idea.  It throws off accounting if */
	    /* nearly all objects are finalizable.  O.w. it shouldn't	 */
	    /* matter.							 */
    	    GC_free((void *)curr_fo);
#	endif
    }
    /* bytes_freed_before is initialized whenever count != 0 */
    if (count != 0 && bytes_freed_before != GC_bytes_freed) {
        LOCK();
	GC_finalizer_bytes_freed += (GC_bytes_freed - bytes_freed_before);
	UNLOCK();
    }
    return count;
}

void (* GC_finalizer_notifier)() = (void (*) (void))0;

static GC_word last_finalizer_notification = 0;

void GC_notify_or_invoke_finalizers(void)
{
    /* This is a convenient place to generate backtraces if appropriate, */
    /* since that code is not callable with the allocation lock.	 */
#   if defined(KEEP_BACK_PTRS) || defined(MAKE_BACK_GRAPH)
      static word last_back_trace_gc_no = 1;	/* Skip first one. */

      if (GC_gc_no > last_back_trace_gc_no) {
	word i;
  
#	ifdef KEEP_BACK_PTRS
	  LOCK();
  	  /* Stops when GC_gc_no wraps; that's OK.	*/
	  last_back_trace_gc_no = (word)(-1);  /* disable others. */
	  for (i = 0; i < GC_backtraces; ++i) {
	      /* FIXME: This tolerates concurrent heap mutation,	*/
	      /* which may cause occasional mysterious results.		*/
	      /* We need to release the GC lock, since GC_print_callers	*/
	      /* acquires it.  It probably shouldn't.			*/
	      UNLOCK();
	      GC_generate_random_backtrace_no_gc();
	      LOCK();
	  }
	  last_back_trace_gc_no = GC_gc_no;
	  UNLOCK();
#	endif
#       ifdef MAKE_BACK_GRAPH
	  if (GC_print_back_height)
            GC_print_back_graph_stats();
#	endif
      }
#   endif
    if (GC_finalize_now == 0) return;
    if (!GC_finalize_on_demand) {
	(void) GC_invoke_finalizers();
#	ifndef THREADS
	  GC_ASSERT(GC_finalize_now == 0);
#	endif	/* Otherwise GC can run concurrently and add more */
	return;
    }
    if (GC_finalizer_notifier != (void (*) (void))0
	&& last_finalizer_notification != GC_gc_no) {
	last_finalizer_notification = GC_gc_no;
	GC_finalizer_notifier();
    }
}

void * GC_call_with_alloc_lock(GC_fn_type fn, void * client_data)
{
    void * result;
    DCL_LOCK_STATE;
    
#   ifdef THREADS
      LOCK();
      /* FIXME - This looks wrong!! */
      SET_LOCK_HOLDER();
#   endif
    result = (*fn)(client_data);
#   ifdef THREADS
#     ifndef GC_ASSERTIONS
        UNSET_LOCK_HOLDER();
#     endif /* o.w. UNLOCK() does it implicitly */
      UNLOCK();
#   endif
    return(result);
}

#if !defined(NO_DEBUGGING)

void GC_print_finalization_stats(void)
{
    struct finalizable_object *fo = GC_finalize_now;
    size_t ready = 0;

    GC_printf("%u finalization table entries; %u disappearing links\n",
	       GC_fo_entries, GC_dl_entries);
    for (; 0 != fo; fo = fo_next(fo)) ++ready;
    GC_printf("%u objects are eligible for immediate finalization\n", ready);
}

#endif /* NO_DEBUGGING */
