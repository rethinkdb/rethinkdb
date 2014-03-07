/* 
 * Copyright 1988, 1989 Hans-J. Boehm, Alan J. Demers
 * Copyright (c) 1991-1994 by Xerox Corporation.  All rights reserved.
 * Copyright (c) 1996-1999 by Silicon Graphics.  All rights reserved.
 * Copyright (c) 1999 by Hewlett-Packard Company. All rights reserved.
 *
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

#ifndef GC_LOCKS_H
#define GC_LOCKS_H

/*
 * Mutual exclusion between allocator/collector routines.
 * Needed if there is more than one allocator thread.
 * DCL_LOCK_STATE declares any local variables needed by LOCK and UNLOCK.
 *
 * Note that I_HOLD_LOCK and I_DONT_HOLD_LOCK are used only positively
 * in assertions, and may return TRUE in the "dont know" case.
 */  
# ifdef THREADS
#  include <atomic_ops.h>

   void GC_noop1(word);
#  ifdef PCR
#    include <base/PCR_Base.h>
#    include <th/PCR_Th.h>
     extern PCR_Th_ML GC_allocate_ml;
#    define DCL_LOCK_STATE \
	 PCR_ERes GC_fastLockRes; PCR_sigset_t GC_old_sig_mask
#    define LOCK() PCR_Th_ML_Acquire(&GC_allocate_ml)
#    define UNLOCK() PCR_Th_ML_Release(&GC_allocate_ml)
#  endif

#  if !defined(AO_HAVE_test_and_set_acquire) && defined(GC_PTHREADS)
#    define USE_PTHREAD_LOCKS
#  endif

#  if defined(GC_WIN32_THREADS) && defined(GC_PTHREADS)
#    define USE_PTHREAD_LOCKS
#  endif

#  if defined(GC_WIN32_THREADS) && !defined(USE_PTHREAD_LOCKS)
#    include <windows.h>
#    define NO_THREAD (DWORD)(-1)
     extern DWORD GC_lock_holder;
     GC_API CRITICAL_SECTION GC_allocate_ml;
#    ifdef GC_ASSERTIONS
#        define UNCOND_LOCK() \
		{ EnterCriticalSection(&GC_allocate_ml); \
		  SET_LOCK_HOLDER(); }
#        define UNCOND_UNLOCK() \
		{ GC_ASSERT(I_HOLD_LOCK()); UNSET_LOCK_HOLDER(); \
	          LeaveCriticalSection(&GC_allocate_ml); }
#    else
#      define UNCOND_LOCK() EnterCriticalSection(&GC_allocate_ml);
#      define UNCOND_UNLOCK() LeaveCriticalSection(&GC_allocate_ml);
#    endif /* !GC_ASSERTIONS */
#    define SET_LOCK_HOLDER() GC_lock_holder = GetCurrentThreadId()
#    define UNSET_LOCK_HOLDER() GC_lock_holder = NO_THREAD
#    define I_HOLD_LOCK() (!GC_need_to_lock \
			   || GC_lock_holder == GetCurrentThreadId())
#    define I_DONT_HOLD_LOCK() (!GC_need_to_lock \
			   || GC_lock_holder != GetCurrentThreadId())
#  elif defined(GC_PTHREADS)
#    include <pthread.h>
     
     /* Posix allows pthread_t to be a struct, though it rarely is.	*/
     /* Unfortunately, we need to use a pthread_t to index a data 	*/
     /* structure.  It also helps if comparisons don't involve a	*/
     /* function call.  Hence we introduce platform-dependent macros	*/
     /* to compare pthread_t ids and to map them to integers.		*/
     /* the mapping to integers does not need to result in different	*/
     /* integers for each thread, though that should be true as much	*/
     /* as possible.							*/
     /* Refine to exclude platforms on which pthread_t is struct */
#    if !defined(GC_WIN32_PTHREADS)
#      define NUMERIC_THREAD_ID(id) ((unsigned long)(id))
#      define THREAD_EQUAL(id1, id2) ((id1) == (id2))
#      define NUMERIC_THREAD_ID_UNIQUE
#    else
#      if defined(GC_WIN32_PTHREADS)
#	 define NUMERIC_THREAD_ID(id) ((unsigned long)(id.p))
	 /* Using documented internal details of win32_pthread library. */
	 /* Faster than pthread_equal(). Should not change with		*/
	 /* future versions of win32_pthread library.                   */
#	 define THREAD_EQUAL(id1, id2) ((id1.p == id2.p) && (id1.x == id2.x))
#        undef NUMERIC_THREAD_ID_UNIQUE
#      else
	 /* Generic definitions that always work, but will result in	*/
	 /* poor performance and weak assertion checking.		*/
#   	 define NUMERIC_THREAD_ID(id) 1l
#	 define THREAD_EQUAL(id1, id2) pthread_equal(id1, id2)
#        undef NUMERIC_THREAD_ID_UNIQUE
#      endif
#    endif
#    define NO_THREAD (-1l)
		/* != NUMERIC_THREAD_ID(pthread_self()) for any thread */

#    if !defined(THREAD_LOCAL_ALLOC) && !defined(USE_PTHREAD_LOCKS)
      /* In the THREAD_LOCAL_ALLOC case, the allocation lock tends to	*/
      /* be held for long periods, if it is held at all.  Thus spinning	*/
      /* and sleeping for fixed periods are likely to result in 	*/
      /* significant wasted time.  We thus rely mostly on queued locks. */
#     define USE_SPIN_LOCK
      extern volatile AO_TS_t GC_allocate_lock;
      extern void GC_lock(void);
	/* Allocation lock holder.  Only set if acquired by client through */
	/* GC_call_with_alloc_lock.					   */
#     ifdef GC_ASSERTIONS
#        define UNCOND_LOCK() \
		{ if (AO_test_and_set_acquire(&GC_allocate_lock) == AO_TS_SET) \
			GC_lock(); \
		  SET_LOCK_HOLDER(); }
#        define UNCOND_UNLOCK() \
		{ GC_ASSERT(I_HOLD_LOCK()); UNSET_LOCK_HOLDER(); \
	          AO_CLEAR(&GC_allocate_lock); }
#     else
#        define UNCOND_LOCK() \
		{ if (AO_test_and_set_acquire(&GC_allocate_lock) == AO_TS_SET) \
			GC_lock(); }
#        define UNCOND_UNLOCK() \
		AO_CLEAR(&GC_allocate_lock)
#     endif /* !GC_ASSERTIONS */
#    else /* THREAD_LOCAL_ALLOC  || USE_PTHREAD_LOCKS */
#      ifndef USE_PTHREAD_LOCKS
#        define USE_PTHREAD_LOCKS
#      endif
#    endif /* THREAD_LOCAL_ALLOC || USE_PTHREAD_LOCK */
#    ifdef USE_PTHREAD_LOCKS
#      include <pthread.h>
       extern pthread_mutex_t GC_allocate_ml;
#      ifdef GC_ASSERTIONS
#        define UNCOND_LOCK() \
		{ GC_lock(); \
		  SET_LOCK_HOLDER(); }
#        define UNCOND_UNLOCK() \
		{ GC_ASSERT(I_HOLD_LOCK()); UNSET_LOCK_HOLDER(); \
	          pthread_mutex_unlock(&GC_allocate_ml); }
#      else /* !GC_ASSERTIONS */
#        if defined(NO_PTHREAD_TRYLOCK)
#          define UNCOND_LOCK() GC_lock();
#        else /* !defined(NO_PTHREAD_TRYLOCK) */
#        define UNCOND_LOCK() \
	   { if (0 != pthread_mutex_trylock(&GC_allocate_ml)) GC_lock(); }
#        endif
#        define UNCOND_UNLOCK() pthread_mutex_unlock(&GC_allocate_ml)
#      endif /* !GC_ASSERTIONS */
#    endif /* USE_PTHREAD_LOCKS */
#    define SET_LOCK_HOLDER() \
		GC_lock_holder = NUMERIC_THREAD_ID(pthread_self())
#    define UNSET_LOCK_HOLDER() GC_lock_holder = NO_THREAD
#    define I_HOLD_LOCK() \
		(!GC_need_to_lock || \
		 GC_lock_holder == NUMERIC_THREAD_ID(pthread_self()))
#    ifndef NUMERIC_THREAD_ID_UNIQUE
#      define I_DONT_HOLD_LOCK() 1  /* Conservatively say yes */
#    else
#      define I_DONT_HOLD_LOCK() \
		(!GC_need_to_lock \
		 || GC_lock_holder != NUMERIC_THREAD_ID(pthread_self()))
#    endif
     extern volatile GC_bool GC_collecting;
#    define ENTER_GC() GC_collecting = 1;
#    define EXIT_GC() GC_collecting = 0;
     extern void GC_lock(void);
     extern unsigned long GC_lock_holder;
#    ifdef GC_ASSERTIONS
      extern unsigned long GC_mark_lock_holder;
#    endif
#  endif /* GC_PTHREADS with linux_threads.c implementation */


# else /* !THREADS */
#   define LOCK()
#   define UNLOCK()
#   define SET_LOCK_HOLDER()
#   define UNSET_LOCK_HOLDER()
#   define I_HOLD_LOCK() TRUE
#   define I_DONT_HOLD_LOCK() TRUE
       		/* Used only in positive assertions or to test whether	*/
       		/* we still need to acaquire the lock.	TRUE works in	*/
       		/* either case.						*/
# endif /* !THREADS */

#if defined(UNCOND_LOCK) && !defined(LOCK) 
     GC_API GC_bool GC_need_to_lock;
     		/* At least two thread running; need to lock.	*/
#    define LOCK() if (GC_need_to_lock) { UNCOND_LOCK(); }
#    define UNLOCK() if (GC_need_to_lock) { UNCOND_UNLOCK(); }
#endif

# ifndef ENTER_GC
#   define ENTER_GC()
#   define EXIT_GC()
# endif

# ifndef DCL_LOCK_STATE
#   define DCL_LOCK_STATE
# endif

#endif /* GC_LOCKS_H */
