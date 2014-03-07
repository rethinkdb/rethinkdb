/* 
 * Copyright (c) 2000-2005 by Hewlett-Packard Company.  All rights reserved.
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
#include "private/gc_priv.h"

# if defined(THREAD_LOCAL_ALLOC)

#include "private/thread_local_alloc.h"
#include "gc_inline.h"

# include <stdlib.h>

#if defined(USE_COMPILER_TLS)
  __thread
#elif defined(USE_WIN32_COMPILER_TLS)
  __declspec(thread)
#endif
GC_key_t GC_thread_key;

static GC_bool keys_initialized;

/* Return a single nonempty freelist fl to the global one pointed to 	*/
/* by gfl.	*/

static void return_single_freelist(void *fl, void **gfl)
{
    void *q, **qptr;

    if (*gfl == 0) {
      *gfl = fl;
    } else {
      GC_ASSERT(GC_size(fl) == GC_size(*gfl));
      /* Concatenate: */
	for (qptr = &(obj_link(fl)), q = *qptr;
	     (word)q >= HBLKSIZE; qptr = &(obj_link(q)), q = *qptr);
	GC_ASSERT(0 == q);
	*qptr = *gfl;
	*gfl = fl;
    }
}

/* Recover the contents of the freelist array fl into the global one gfl.*/
/* We hold the allocator lock.						*/
static void return_freelists(void **fl, void **gfl)
{
    int i;

    for (i = 1; i < TINY_FREELISTS; ++i) {
	if ((word)(fl[i]) >= HBLKSIZE) {
	  return_single_freelist(fl[i], gfl+i);
	}
	/* Clear fl[i], since the thread structure may hang around.	*/
	/* Do it in a way that is likely to trap if we access it.	*/
	fl[i] = (ptr_t)HBLKSIZE;
    }
    /* The 0 granule freelist really contains 1 granule objects.	*/
#   ifdef GC_GCJ_SUPPORT
      if (fl[0] == ERROR_FL) return;
#   endif
    if ((word)(fl[0]) >= HBLKSIZE) {
        return_single_freelist(fl[0], gfl+1);
    }
}

/* Each thread structure must be initialized.	*/
/* This call must be made from the new thread.	*/
/* Caller holds allocation lock.		*/
void GC_init_thread_local(GC_tlfs p)
{
    int i;

    if (!keys_initialized) {
	if (0 != GC_key_create(&GC_thread_key, 0)) {
	    ABORT("Failed to create key for local allocator");
        }
	keys_initialized = TRUE;
    }
    if (0 != GC_setspecific(GC_thread_key, p)) {
	ABORT("Failed to set thread specific allocation pointers");
    }
    for (i = 1; i < TINY_FREELISTS; ++i) {
	p -> ptrfree_freelists[i] = (void *)1;
	p -> normal_freelists[i] = (void *)1;
#	ifdef GC_GCJ_SUPPORT
	  p -> gcj_freelists[i] = (void *)1;
#	endif
    }   
    /* Set up the size 0 free lists.	*/
    /* We now handle most of them like regular free lists, to ensure	*/
    /* That explicit deallocation works.  However, allocation of a	*/
    /* size 0 "gcj" object is always an error.				*/
    p -> ptrfree_freelists[0] = (void *)1;
    p -> normal_freelists[0] = (void *)1;
#   ifdef GC_GCJ_SUPPORT
        p -> gcj_freelists[0] = ERROR_FL;
#   endif
}

#ifdef GC_GCJ_SUPPORT
  extern void ** GC_gcjobjfreelist;
#endif

/* We hold the allocator lock.	*/
void GC_destroy_thread_local(GC_tlfs p)
{
    /* We currently only do this from the thread itself or from	*/
    /* the fork handler for a child process.			*/
#   ifndef HANDLE_FORK
      GC_ASSERT(GC_getspecific(GC_thread_key) == (void *)p);
#   endif
    return_freelists(p -> ptrfree_freelists, GC_aobjfreelist);
    return_freelists(p -> normal_freelists, GC_objfreelist);
#   ifdef GC_GCJ_SUPPORT
   	return_freelists(p -> gcj_freelists, GC_gcjobjfreelist);
#   endif
}

#if defined(GC_ASSERTIONS) && defined(GC_PTHREADS) && !defined(CYGWIN32) \
    && !defined(GC_WIN32_PTHREADS)
# include <pthread.h>
  extern char * GC_lookup_thread(pthread_t id);
#endif

#if defined(GC_ASSERTIONS) && defined(GC_WIN32_THREADS)
  extern char * GC_lookup_thread(int id);
#endif

void * GC_malloc(size_t bytes)
{
    size_t granules = ROUNDED_UP_GRANULES(bytes);
    void *tsd;
    void *result;
    void **tiny_fl;

#   if defined(REDIRECT_MALLOC) && !defined(USE_PTHREAD_SPECIFIC)
      GC_key_t k = GC_thread_key;
      if (EXPECT(0 == k, 0)) {
	/* We haven't yet run GC_init_parallel.  That means	*/
	/* we also aren't locking, so this is fairly cheap.	*/
	return GC_core_malloc(bytes);
      }
      tsd = GC_getspecific(k);
#   else
      GC_ASSERT(GC_is_initialized);
      tsd = GC_getspecific(GC_thread_key);
#   endif
#   if defined(REDIRECT_MALLOC) && defined(USE_PTHREAD_SPECIFIC)
      if (EXPECT(NULL == tsd, 0)) {
	return GC_core_malloc(bytes);
      }
#   endif
#   ifdef GC_ASSERTIONS
      /* We can't check tsd correctly, since we don't have access to 	*/
      /* the right declarations.  But we can check that it's close.	*/
      LOCK();
      {
#	if defined(GC_WIN32_THREADS)
	  char * me = (char *)GC_lookup_thread_inner(GetCurrentThreadId());
#       else
	  char * me = GC_lookup_thread(pthread_self());
#	endif
        GC_ASSERT((char *)tsd > me && (char *)tsd < me + 1000);
      }
      UNLOCK();
#   endif
    tiny_fl = ((GC_tlfs)tsd) -> normal_freelists;
    GC_FAST_MALLOC_GRANS(result, granules, tiny_fl, DIRECT_GRANULES,
		         NORMAL, GC_core_malloc(bytes), obj_link(result)=0);
    return result;
}

void * GC_malloc_atomic(size_t bytes)
{
    size_t granules = ROUNDED_UP_GRANULES(bytes);
    void *result;
    void **tiny_fl;

    GC_ASSERT(GC_is_initialized);
    tiny_fl = ((GC_tlfs)GC_getspecific(GC_thread_key))
		        		-> ptrfree_freelists;
    GC_FAST_MALLOC_GRANS(result, bytes, tiny_fl, DIRECT_GRANULES,
		         PTRFREE, GC_core_malloc_atomic(bytes), 0/* no init */);
    return result;
}

#ifdef GC_GCJ_SUPPORT

#include "include/gc_gcj.h"

#ifdef GC_ASSERTIONS
  extern GC_bool GC_gcj_malloc_initialized;
#endif

extern int GC_gcj_kind;

/* Gcj-style allocation without locks is extremely tricky.  The 	*/
/* fundamental issue is that we may end up marking a free list, which	*/
/* has freelist links instead of "vtable" pointers.  That is usually	*/
/* OK, since the next object on the free list will be cleared, and	*/
/* will thus be interpreted as containg a zero descriptor.  That's fine	*/
/* if the object has not yet been initialized.  But there are		*/
/* interesting potential races.						*/
/* In the case of incremental collection, this seems hopeless, since	*/
/* the marker may run asynchronously, and may pick up the pointer to  	*/
/* the next freelist entry (which it thinks is a vtable pointer), get	*/
/* suspended for a while, and then see an allocated object instead	*/
/* of the vtable.  This made be avoidable with either a handshake with	*/
/* the collector or, probably more easily, by moving the free list	*/
/* links to the second word of each object.  The latter isn't a		*/
/* universal win, since on architecture like Itanium, nonzero offsets	*/
/* are not necessarily free.  And there may be cache fill order issues.	*/
/* For now, we punt with incremental GC.  This probably means that	*/
/* incremental GC should be enabled before we fork a second thread.	*/
void * GC_gcj_malloc(size_t bytes,
		     void * ptr_to_struct_containing_descr)
{
  if (GC_EXPECT(GC_incremental, 0)) {
    return GC_core_gcj_malloc(bytes, ptr_to_struct_containing_descr);
  } else {
    size_t granules = ROUNDED_UP_GRANULES(bytes);
    void *result;
    void **tiny_fl = ((GC_tlfs)GC_getspecific(GC_thread_key))
		        		-> gcj_freelists;
    GC_ASSERT(GC_gcj_malloc_initialized);
    GC_FAST_MALLOC_GRANS(result, bytes, tiny_fl, DIRECT_GRANULES,
		         GC_gcj_kind,
			 GC_core_gcj_malloc(bytes,
				 	    ptr_to_struct_containing_descr),
			 {AO_compiler_barrier();
			  *(void **)result = ptr_to_struct_containing_descr;});
    	/* This forces the initialization of the "method ptr".		*/
        /* This is necessary to ensure some very subtle properties	*/
    	/* required if a GC is run in the middle of such an allocation.	*/
    	/* Here we implicitly also assume atomicity for the free list.	*/
        /* and method pointer assignments.				*/
	/* We must update the freelist before we store the pointer.	*/
	/* Otherwise a GC at this point would see a corrupted		*/
	/* free list.							*/
	/* A real memory barrier is not needed, since the 		*/
	/* action of stopping this thread will cause prior writes	*/
	/* to complete.							*/
	/* We assert that any concurrent marker will stop us.		*/
	/* Thus it is impossible for a mark procedure to see the 	*/
	/* allocation of the next object, but to see this object 	*/
	/* still containing a free list pointer.  Otherwise the 	*/
	/* marker, by misinterpreting the freelist link as a vtable	*/
        /* pointer, might find a random "mark descriptor" in the next	*/
        /* object.							*/
    return result;
  }
}

#endif /* GC_GCJ_SUPPORT */

/* The thread support layer must arrange to mark thread-local	*/
/* free lists explicitly, since the link field is often 	*/
/* invisible to the marker.  It knows hoe to find all threads;	*/
/* we take care of an individual thread freelist structure.	*/
void GC_mark_thread_local_fls_for(GC_tlfs p)
{
    ptr_t q;
    int j;
    
    for (j = 1; j < TINY_FREELISTS; ++j) {
      q = p -> ptrfree_freelists[j];
      if ((word)q > HBLKSIZE) GC_set_fl_marks(q);
      q = p -> normal_freelists[j];
      if ((word)q > HBLKSIZE) GC_set_fl_marks(q);
#     ifdef GC_GCJ_SUPPORT
        q = p -> gcj_freelists[j];
        if ((word)q > HBLKSIZE) GC_set_fl_marks(q);
#     endif /* GC_GCJ_SUPPORT */
    }
}

#if defined(GC_ASSERTIONS)
    /* Check that all thread-local free-lists in p are completely marked.	*/
    void GC_check_tls_for(GC_tlfs p)
    {
	ptr_t q;
	int j;
	
	for (j = 1; j < TINY_FREELISTS; ++j) {
	  q = p -> ptrfree_freelists[j];
	  if ((word)q > HBLKSIZE) GC_check_fl_marks(q);
	  q = p -> normal_freelists[j];
	  if ((word)q > HBLKSIZE) GC_check_fl_marks(q);
#	  ifdef GC_GCJ_SUPPORT
	    q = p -> gcj_freelists[j];
	    if ((word)q > HBLKSIZE) GC_check_fl_marks(q);
#	  endif /* GC_GCJ_SUPPORT */
	}
    }
#endif /* GC_ASSERTIONS */

# else  /* !THREAD_LOCAL_ALLOC  */

#   define GC_destroy_thread_local(t)

# endif /* !THREAD_LOCAL_ALLOC */

