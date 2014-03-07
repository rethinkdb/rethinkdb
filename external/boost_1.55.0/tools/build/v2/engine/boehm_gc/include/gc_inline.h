/* 
 * Copyright 1988, 1989 Hans-J. Boehm, Alan J. Demers
 * Copyright (c) 1991-1995 by Xerox Corporation.  All rights reserved.
 * Copyright (c) 2005 Hewlett-Packard Development Company, L.P.
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
 
/* USE OF THIS FILE IS NOT RECOMMENDED unless GC_all_interior_pointers	*/
/* is not set, or the collector has been built with			*/
/* -DDONT_ADD_BYTE_AT_END, or the specified size includes a pointerfree	*/
/* word at the end.  In the standard collector configuration,		*/
/* the final word of each object may not be scanned.			*/
/* This interface is most useful for compilers that generate C.		*/
/* It is also used internally for thread-local allocation, in which	*/
/* case, the size is suitably adjusted by the caller.			*/
/* Manual use is hereby discouraged.					*/

#include "gc.h"
#include "gc_tiny_fl.h"

#if __GNUC__ >= 3
# define GC_EXPECT(expr, outcome) __builtin_expect(expr,outcome)
  /* Equivalent to (expr), but predict that usually (expr)==outcome. */
#else
# define GC_EXPECT(expr, outcome) (expr)
#endif /* __GNUC__ */

/* The ultimately general inline allocation macro.  Allocate an object	*/
/* of size bytes, putting the resulting pointer in result.  Tiny_fl is	*/
/* a "tiny" free list array, which will be used first, if the size	*/
/* is appropriate.  If bytes is too large, we allocate with 		*/
/* default_expr instead.  If we need to refill the free list, we use	*/
/* GC_generic_malloc_many with the indicated kind.			*/
/* Tiny_fl should be an array of GC_TINY_FREELISTS void * pointers.	*/
/* If num_direct is nonzero, and the individual free list pointers	*/
/* are initialized to (void *)1, then we allocate numdirect granules	*/
/* directly using gmalloc before putting multiple objects into the	*/
/* tiny_fl entry.  If num_direct is zero, then the free lists may also	*/
/* be initialized to (void *)0.						*/
/* We rely on much of this hopefully getting optimized away in the	*/
/* num_direct = 0 case.							*/
/* Particularly if bytes is constant, this should generate a small	*/
/* amount of code.							*/
# define GC_FAST_MALLOC_GRANS(result,granules,tiny_fl,num_direct,\
			      kind,default_expr,init) \
{ \
    if (GC_EXPECT(granules >= GC_TINY_FREELISTS,0)) { \
        result = default_expr; \
    } else { \
	void **my_fl = tiny_fl + granules; \
        void *my_entry=*my_fl; \
	void *next; \
 \
	while (GC_EXPECT((GC_word)my_entry \
				<= num_direct + GC_TINY_FREELISTS + 1, 0)) { \
	    /* Entry contains counter or NULL */ \
	    if ((GC_word)my_entry - 1 < num_direct) { \
		/* Small counter value, not NULL */ \
                *my_fl = (ptr_t)my_entry + granules + 1; \
                result = default_expr; \
		goto out; \
            } else { \
		/* Large counter or NULL */ \
                GC_generic_malloc_many(((granules) == 0? GC_GRANULE_BYTES : \
					  RAW_BYTES_FROM_INDEX(granules)), \
				       kind, my_fl); \
		my_entry = *my_fl; \
                if (my_entry == 0) { \
		    result = GC_oom_fn(bytes); \
		    goto out; \
		} \
	    } \
        } \
        next = *(void **)(my_entry); \
        result = (void *)my_entry; \
        *my_fl = next; \
	init; \
        PREFETCH_FOR_WRITE(next); \
        GC_ASSERT(GC_size(result) >= bytes + EXTRA_BYTES); \
        GC_ASSERT((kind) == PTRFREE || ((GC_word *)result)[1] == 0); \
      out: ; \
   } \
}

# define GC_WORDS_TO_WHOLE_GRANULES(n) \
	GC_WORDS_TO_GRANULES((n) + GC_GRANULE_WORDS - 1)

/* Allocate n words (NOT BYTES).  X is made to point to the result.	*/
/* This should really only be used if GC_all_interior_pointers is	*/
/* not set, or DONT_ADD_BYTE_AT_END is set.  See above.			*/
/* The semantics changed in version 7.0; we no longer lock, and		*/
/* the caller is responsible for supplying a cleared tiny_fl		*/
/* free list array.  For single-threaded applications, this may be	*/
/* a global array.							*/
# define GC_MALLOC_WORDS(result,n,tiny_fl) \
{	\
    size_t grans = WORDS_TO_WHOLE_GRANULES(n); \
    GC_FAST_MALLOC_GRANS(result, grans, tiny_fl, 0, \
			 NORMAL, GC_malloc(grans*GRANULE_BYTES), \
			 *(void **)result = 0); \
}

# define GC_MALLOC_ATOMIC_WORDS(result,n,tiny_fl) \
{	\
    size_t grans = WORDS_TO_WHOLE_GRANULES(n); \
    GC_FAST_MALLOC_GRANS(result, grans, tiny_fl, 0, \
			 PTRFREE, GC_malloc_atomic(grans*GRANULE_BYTES), \
			 /* no initialization */); \
}


/* And once more for two word initialized objects: */
# define GC_CONS(result, first, second, tiny_fl) \
{	\
    size_t grans = WORDS_TO_WHOLE_GRANULES(2); \
    GC_FAST_MALLOC_GRANS(result, grans, tiny_fl, 0, \
			 NORMAL, GC_malloc(grans*GRANULE_BYTES), \
			 *(void **)result = (void *)(first)); \
    ((void **)(result))[1] = (void *)(second);	\
}
