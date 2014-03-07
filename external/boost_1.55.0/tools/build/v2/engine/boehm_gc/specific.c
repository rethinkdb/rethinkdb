/* 
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

#include "private/gc_priv.h"	/* For configuration, pthreads.h. */
#include "private/thread_local_alloc.h"
				/* To determine type of tsd impl. */
				/* Includes private/specific.h	  */
				/* if needed.			  */

#if defined(USE_CUSTOM_SPECIFIC)

#include "atomic_ops.h"

static tse invalid_tse = {INVALID_QTID, 0, 0, INVALID_THREADID};
			/* A thread-specific data entry which will never    */
			/* appear valid to a reader.  Used to fill in empty */
			/* cache entries to avoid a check for 0.	    */

int PREFIXED(key_create) (tsd ** key_ptr, void (* destructor)(void *)) {
    int i;
    tsd * result = (tsd *)MALLOC_CLEAR(sizeof (tsd));

    /* A quick alignment check, since we need atomic stores */
      GC_ASSERT((unsigned long)(&invalid_tse.next) % sizeof(tse *) == 0);
    if (0 == result) return ENOMEM;
    pthread_mutex_init(&(result -> lock), NULL);
    for (i = 0; i < TS_CACHE_SIZE; ++i) {
	result -> cache[i] = &invalid_tse;
    }
#   ifdef GC_ASSERTIONS
      for (i = 0; i < TS_HASH_SIZE; ++i) {
	GC_ASSERT(result -> hash[i] == 0);
      }
#   endif
    *key_ptr = result;
    return 0;
}

int PREFIXED(setspecific) (tsd * key, void * value) {
    pthread_t self = pthread_self();
    int hash_val = HASH(self);
    volatile tse * entry = (volatile tse *)MALLOC_CLEAR(sizeof (tse));
    
    GC_ASSERT(self != INVALID_THREADID);
    if (0 == entry) return ENOMEM;
    pthread_mutex_lock(&(key -> lock));
    /* Could easily check for an existing entry here.	*/
    entry -> next = key -> hash[hash_val];
    entry -> thread = self;
    entry -> value = value;
    GC_ASSERT(entry -> qtid == INVALID_QTID);
    /* There can only be one writer at a time, but this needs to be	*/
    /* atomic with respect to concurrent readers.			*/ 
    AO_store_release((volatile AO_t *)(key -> hash + hash_val), (AO_t)entry);
    pthread_mutex_unlock(&(key -> lock));
    return 0;
}

/* Remove thread-specific data for this thread.  Should be called on	*/
/* thread exit.								*/
void PREFIXED(remove_specific) (tsd * key) {
    pthread_t self = pthread_self();
    unsigned hash_val = HASH(self);
    tse *entry;
    tse **link = key -> hash + hash_val;

    pthread_mutex_lock(&(key -> lock));
    entry = *link;
    while (entry != NULL && entry -> thread != self) {
	link = &(entry -> next);
        entry = *link;
    }
    /* Invalidate qtid field, since qtids may be reused, and a later 	*/
    /* cache lookup could otherwise find this entry.			*/
        entry -> qtid = INVALID_QTID;
    if (entry != NULL) {
	*link = entry -> next;
	/* Atomic! concurrent accesses still work.	*/
	/* They must, since readers don't lock.		*/
	/* We shouldn't need a volatile access here,	*/
	/* since both this and the preceding write 	*/
	/* should become visible no later than		*/
	/* the pthread_mutex_unlock() call.		*/
    }
    /* If we wanted to deallocate the entry, we'd first have to clear 	*/
    /* any cache entries pointing to it.  That probably requires	*/
    /* additional synchronization, since we can't prevent a concurrent 	*/
    /* cache lookup, which should still be examining deallocated memory.*/
    /* This can only happen if the concurrent access is from another	*/
    /* thread, and hence has missed the cache, but still...		*/

    /* With GC, we're done, since the pointers from the cache will 	*/
    /* be overwritten, all local pointers to the entries will be	*/
    /* dropped, and the entry will then be reclaimed.			*/
    pthread_mutex_unlock(&(key -> lock));
}

/* Note that even the slow path doesn't lock.	*/
void *  PREFIXED(slow_getspecific) (tsd * key, unsigned long qtid,
				    tse * volatile * cache_ptr) {
    pthread_t self = pthread_self();
    unsigned hash_val = HASH(self);
    tse *entry = key -> hash[hash_val];

    GC_ASSERT(qtid != INVALID_QTID);
    while (entry != NULL && entry -> thread != self) {
	entry = entry -> next;
    } 
    if (entry == NULL) return NULL;
    /* Set cache_entry.		*/
        entry -> qtid = qtid;
		/* It's safe to do this asynchronously.  Either value 	*/
		/* is safe, though may produce spurious misses.		*/
		/* We're replacing one qtid with another one for the	*/
		/* same thread.						*/
	*cache_ptr = entry;
		/* Again this is safe since pointer assignments are 	*/
		/* presumed atomic, and either pointer is valid.	*/
    return entry -> value;
}

#ifdef GC_ASSERTIONS

/* Check that that all elements of the data structure associated 	*/
/* with key are marked.							*/
void PREFIXED(check_tsd_marks) (tsd *key)
{
    int i;
    tse *p;

    if (!GC_is_marked(GC_base(key))) {
	ABORT("Unmarked thread-specific-data table");
    }
    for (i = 0; i < TS_HASH_SIZE; ++i) {
        for (p = key -> hash[i]; p != 0; p = p -> next) {
	    if (!GC_is_marked(GC_base(p))) {
		GC_err_printf(
			"Thread-specific-data entry at %p not marked\n",p);
		ABORT("Unmarked tse");
	    }
	}
    }
    for (i = 0; i < TS_CACHE_SIZE; ++i) {
        p = key -> cache[i];
	if (p != &invalid_tse && !GC_is_marked(GC_base(p))) {
	    GC_err_printf(
		"Cached thread-specific-data entry at %p not marked\n",p);
	    ABORT("Unmarked cached tse");
	}
    }
}

#endif

#endif /* USE_CUSTOM_SPECIFIC */
