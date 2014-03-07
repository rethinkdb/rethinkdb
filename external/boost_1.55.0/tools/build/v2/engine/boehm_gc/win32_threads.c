#include "private/gc_priv.h"

#if defined(GC_WIN32_THREADS)

#include <windows.h>

#ifdef THREAD_LOCAL_ALLOC
# include "private/thread_local_alloc.h"
#endif /* THREAD_LOCAL_ALLOC */

/* Allocation lock declarations.	*/
#if !defined(USE_PTHREAD_LOCKS)
# if defined(GC_DLL)
    __declspec(dllexport) CRITICAL_SECTION GC_allocate_ml;
# else
    CRITICAL_SECTION GC_allocate_ml;
# endif
  DWORD GC_lock_holder = NO_THREAD;
  	/* Thread id for current holder of allocation lock */
#else
  pthread_mutex_t GC_allocate_ml = PTHREAD_MUTEX_INITIALIZER;
  unsigned long GC_lock_holder = NO_THREAD;
#endif

#ifdef GC_PTHREADS
# include <errno.h>

/* GC_DLL should not normally be defined, especially since we often do turn */
/* on THREAD_LOCAL_ALLOC, which is currently incompatible. 		    */
/* It might be possible to get GC_DLL and DllMain-based	thread registration */
/* to work with Cygwin, but if you try you are on your own.		    */
#ifdef GC_DLL
# error GC_DLL untested with Cygwin
#endif

 /* Cygwin-specific forward decls */
# undef pthread_create 
# undef pthread_sigmask 
# undef pthread_join 
# undef pthread_detach
# undef dlopen 

# ifdef DEBUG_THREADS
#   ifdef CYGWIN32
#     define DEBUG_CYGWIN_THREADS 1
#     define DEBUG_WIN32_PTHREADS 0
#   else
#     define DEBUG_WIN32_PTHREADS 1
#     define DEBUG_CYGWIN_THREADS 0
#   endif
# else
#   define DEBUG_CYGWIN_THREADS 0
#   define DEBUG_WIN32_PTHREADS 0
# endif

  void * GC_pthread_start(void * arg);
  void GC_thread_exit_proc(void *arg);

# include <pthread.h>

#else

# ifdef DEBUG_THREADS
#   define DEBUG_WIN32_THREADS 1
# else
#   define DEBUG_WIN32_THREADS 0
# endif

# undef CreateThread
# undef ExitThread
# undef _beginthreadex
# undef _endthreadex
# undef _beginthread
# ifdef DEBUG_THREADS
#   define DEBUG_WIN32_THREADS 1
# else
#   define DEBUG_WIN32_THREADS 0
# endif

# include <process.h>  /* For _beginthreadex, _endthreadex */

#endif

#if defined(GC_DLL) && !defined(MSWINCE)
  static GC_bool GC_win32_dll_threads = FALSE;
  /* This code operates in two distinct modes, depending on	*/
  /* the setting of GC_win32_dll_threads.  If			*/
  /* GC_win32_dll_threads is set, all threads in the process	*/
  /* are implicitly registered with the GC by DllMain. 		*/
  /* No explicit registration is required, and attempts at	*/
  /* explicit registration are ignored.  This mode is		*/
  /* very different from the Posix operation of the collector.	*/
  /* In this mode access to the thread table is lock-free.	*/
  /* Hence there is a static limit on the number of threads.	*/
  
  /* If GC_win32_dll_threads is FALSE, or the collector is	*/
  /* built without GC_DLL defined, things operate in a way	*/
  /* that is very similar to Posix platforms, and new threads	*/
  /* must be registered with the collector, e.g. by using	*/
  /* preprocessor-based interception of the thread primitives.	*/
  /* In this case, we use a real data structure for the thread	*/
  /* table.  Note that there is no equivalent of linker-based	*/
  /* call interception, since we don't have ELF-like 		*/
  /* facilities.  The Windows analog appears to be "API		*/
  /* hooking", which really seems to be a standard way to 	*/
  /* do minor binary rewriting (?).  I'd prefer not to have	*/
  /* the basic collector rely on such facilities, but an	*/
  /* optional package that intercepts thread calls this way	*/
  /* would probably be nice.					*/

  /* GC_win32_dll_threads must be set at initialization time,	*/
  /* i.e. before any collector or thread calls.  We make it a	*/
  /* "dynamic" option only to avoid multiple library versions.	*/
#else
# define GC_win32_dll_threads FALSE
#endif

/* We have two versions of the thread table.  Which one	*/
/* we us depends on whether or not GC_win32_dll_threads */
/* is set.  Note that before initialization, we don't 	*/
/* add any entries to either table, even if DllMain is	*/
/* called.  The main thread will be added on		*/
/* initialization.					*/

/* The type of the first argument to InterlockedExchange.	*/
/* Documented to be LONG volatile *, but at least gcc likes 	*/
/* this better.							*/
typedef LONG * IE_t;

GC_bool GC_thr_initialized = FALSE;

GC_bool GC_need_to_lock = FALSE;

static GC_bool parallel_initialized = FALSE;

void GC_init_parallel(void);

#ifdef GC_DLL
  /* Turn on GC_win32_dll_threads	*/
  GC_API void GC_use_DllMain(void)
  {
#     ifdef THREAD_LOCAL_ALLOC
	  ABORT("Cannot use thread local allocation with DllMain-based "
		"thread registration.");
	  /* Thread-local allocation really wants to lock at thread	*/
	  /* entry and exit.						*/
#     endif
      GC_ASSERT(!parallel_initialized);
      GC_win32_dll_threads = TRUE;
  }
#else
  GC_API void GC_use_DllMain(void)
  {
      ABORT("GC not configured as DLL");
  }
#endif

DWORD GC_main_thread = 0;

struct GC_Thread_Rep {
  union {
    AO_t tm_in_use; 	/* Updated without lock.		*/
  			/* We assert that unused 		*/
  			/* entries have invalid ids of		*/
  			/* zero and zero stack fields.  	*/
    			/* Used only with GC_win32_dll_threads. */
    struct GC_Thread_Rep * tm_next;
    			/* Hash table link without 		*/
    			/* GC_win32_dll_threads.		*/
    			/* More recently allocated threads	*/
			/* with a given pthread id come 	*/
			/* first.  (All but the first are	*/
			/* guaranteed to be dead, but we may    */
			/* not yet have registered the join.)   */
  } table_management;
# define in_use table_management.tm_in_use
# define next table_management.tm_next
  DWORD id;
  HANDLE handle;
  ptr_t stack_base;	/* The cold end of the stack.   */
			/* 0 ==> entry not valid.	*/
			/* !in_use ==> stack_base == 0	*/
  GC_bool suspended;

# ifdef GC_PTHREADS
    void *status; /* hold exit value until join in case it's a pointer */
    pthread_t pthread_id;
    short flags;		/* Protected by GC lock.	*/
#	define FINISHED 1   	/* Thread has exited.	*/
#	define DETACHED 2	/* Thread is intended to be detached.	*/
#   define KNOWN_FINISHED(t) (((t) -> flags) & FINISHED)
# else
#   define KNOWN_FINISHED(t) 0
# endif
# ifdef THREAD_LOCAL_ALLOC
    struct thread_local_freelists tlfs;
# endif
};

typedef struct GC_Thread_Rep * GC_thread;
typedef volatile struct GC_Thread_Rep * GC_vthread;

/*
 * We assumed that volatile ==> memory ordering, at least among
 * volatiles.  This code should consistently use atomic_ops.
 */

volatile GC_bool GC_please_stop = FALSE;

/*
 * We track thread attachments while the world is supposed to be stopped.
 * Unfortunately, we can't stop them from starting, since blocking in
 * DllMain seems to cause the world to deadlock.  Thus we have to recover
 * If we notice this in the middle of marking.
 */

AO_t GC_attached_thread = FALSE;
/* Return TRUE if an thread was attached since we last asked or	*/
/* since GC_attached_thread was explicitly reset.		*/
GC_bool GC_started_thread_while_stopped(void)
{
  AO_t result;

  if (GC_win32_dll_threads) {
    AO_nop_full();	/* Prior heap reads need to complete earlier. */
    result = AO_load(&GC_attached_thread);
    if (result) {
      AO_store(&GC_attached_thread, FALSE);
    }
    return ((GC_bool)result);
  } else {
    return FALSE;
  }
}

/* Thread table used if GC_win32_dll_threads is set.	*/
/* This is a fixed size array.				*/
/* Since we use runtime conditionals, both versions	*/
/* are always defined.					*/
# ifndef MAX_THREADS
#   define MAX_THREADS 512
#  endif
  /* Things may get quite slow for large numbers of threads,	*/
  /* since we look them up with sequential search.		*/

  volatile struct GC_Thread_Rep dll_thread_table[MAX_THREADS];

  volatile LONG GC_max_thread_index = 0;
  			/* Largest index in dll_thread_table	*/
		        /* that was ever used.			*/

/* And now the version used if GC_win32_dll_threads is not set.	*/
/* This is a chained hash table, with much of the code borrowed	*/
/* From the Posix implementation.				*/
# define THREAD_TABLE_SZ 256	/* Must be power of 2	*/
  GC_thread GC_threads[THREAD_TABLE_SZ];
  

/* Add a thread to GC_threads.  We assume it wasn't already there.	*/
/* Caller holds allocation lock.					*/
/* Unlike the pthreads version, the id field is set by the caller.	*/
GC_thread GC_new_thread(DWORD id)
{
    word hv = ((word)id) % THREAD_TABLE_SZ;
    GC_thread result;
    /* It may not be safe to allocate when we register the first thread. */
    static struct GC_Thread_Rep first_thread;
    static GC_bool first_thread_used = FALSE;
    
    GC_ASSERT(I_HOLD_LOCK());
    if (!first_thread_used) {
    	result = &first_thread;
    	first_thread_used = TRUE;
    } else {
        GC_ASSERT(!GC_win32_dll_threads);
        result = (struct GC_Thread_Rep *)
        	 GC_INTERNAL_MALLOC(sizeof(struct GC_Thread_Rep), NORMAL);
#       ifdef GC_PTHREADS
	  /* result can be NULL -> segfault */
	  GC_ASSERT(result -> flags == 0);
#       endif
    }
    if (result == 0) return(0);
    /* result -> id = id; Done by caller.	*/
    result -> next = GC_threads[hv];
    GC_threads[hv] = result;
#   ifdef GC_PTHREADS
      GC_ASSERT(result -> flags == 0 /* && result -> thread_blocked == 0 */);
#   endif
    return(result);
}

extern LONG WINAPI GC_write_fault_handler(struct _EXCEPTION_POINTERS *exc_info);

#if defined(GWW_VDB) && defined(MPROTECT_VDB)
  extern GC_bool GC_gww_dirty_init(void);
  /* Defined in os_dep.c.  Returns TRUE if GetWriteWatch is available. 	*/
  /* may be called repeatedly.						*/
#endif

GC_bool GC_in_thread_creation = FALSE;  /* Protected by allocation lock. */

/*
 * This may be called from DllMain, and hence operates under unusual
 * constraints.  In particular, it must be lock-free if GC_win32_dll_threads
 * is set.  Always called from the thread being added.
 * If GC_win32_dll_threads is not set, we already hold the allocation lock,
 * except possibly during single-threaded start-up code.
 */
static GC_thread GC_register_my_thread_inner(struct GC_stack_base *sb,
					     DWORD thread_id)
{
  GC_vthread me;

  /* The following should be a noop according to the win32	*/
  /* documentation.  There is empirical evidence that it	*/
  /* isn't.		- HB					*/
# if defined(MPROTECT_VDB)
#   if defined(GWW_VDB)
      if (GC_incremental && !GC_gww_dirty_init())
	SetUnhandledExceptionFilter(GC_write_fault_handler);
#   else
      if (GC_incremental) SetUnhandledExceptionFilter(GC_write_fault_handler);
#   endif
# endif

  if (GC_win32_dll_threads) {
    int i;
    /* It appears to be unsafe to acquire a lock here, since this	*/
    /* code is apparently not preeemptible on some systems.		*/
    /* (This is based on complaints, not on Microsoft's official	*/
    /* documentation, which says this should perform "only simple	*/
    /* initialization tasks".)						*/
    /* Hence we make do with nonblocking synchronization.		*/
    /* It has been claimed that DllMain is really only executed with	*/
    /* a particular system lock held, and thus careful use of locking	*/
    /* around code that doesn't call back into the system libraries	*/
    /* might be OK.  But this hasn't been tested across all win32	*/
    /* variants.							*/
                /* cast away volatile qualifier */
    for (i = 0; InterlockedExchange((IE_t)&dll_thread_table[i].in_use,1) != 0;
	 i++) {
      /* Compare-and-swap would make this cleaner, but that's not 	*/
      /* supported before Windows 98 and NT 4.0.  In Windows 2000,	*/
      /* InterlockedExchange is supposed to be replaced by		*/
      /* InterlockedExchangePointer, but that's not really what I	*/
      /* want here.							*/
      /* FIXME: We should eventually declare Win95 dead and use AO_	*/
      /* primitives here.						*/
      if (i == MAX_THREADS - 1)
        ABORT("too many threads");
    }
    /* Update GC_max_thread_index if necessary.  The following is safe,	*/
    /* and unlike CompareExchange-based solutions seems to work on all	*/
    /* Windows95 and later platforms.					*/
    /* Unfortunately, GC_max_thread_index may be temporarily out of 	*/
    /* bounds, so readers have to compensate.				*/
    while (i > GC_max_thread_index) {
      InterlockedIncrement((IE_t)&GC_max_thread_index);
    }
    if (GC_max_thread_index >= MAX_THREADS) {
      /* We overshot due to simultaneous increments.	*/
      /* Setting it to MAX_THREADS-1 is always safe.	*/
      GC_max_thread_index = MAX_THREADS - 1;
    }
    me = dll_thread_table + i;
  } else /* Not using DllMain */ {
    GC_ASSERT(I_HOLD_LOCK());
    GC_in_thread_creation = TRUE; /* OK to collect from unknown thread. */
    me = GC_new_thread(thread_id);
    GC_in_thread_creation = FALSE;
  }
# ifdef GC_PTHREADS
    /* me can be NULL -> segfault */
    me -> pthread_id = pthread_self();
# endif

  if (!DuplicateHandle(GetCurrentProcess(),
                 	GetCurrentThread(),
		        GetCurrentProcess(),
		        (HANDLE*)&(me -> handle),
		        0,
		        0,
		        DUPLICATE_SAME_ACCESS)) {
	DWORD last_error = GetLastError();
	GC_err_printf("Last error code: %d\n", last_error);
	ABORT("DuplicateHandle failed");
  }
  me -> stack_base = sb -> mem_base;
  /* Up until this point, GC_push_all_stacks considers this thread	*/
  /* invalid.								*/
  /* Up until this point, this entry is viewed as reserved but invalid	*/
  /* by GC_delete_thread.						*/
  me -> id = thread_id;
# if defined(THREAD_LOCAL_ALLOC)
      GC_init_thread_local((GC_tlfs)(&(me->tlfs)));
# endif
  if (me -> stack_base == NULL) 
      ABORT("Bad stack base in GC_register_my_thread_inner");
  if (GC_win32_dll_threads) {
    if (GC_please_stop) {
      AO_store(&GC_attached_thread, TRUE);
      AO_nop_full();  // Later updates must become visible after this.
    }
    /* We'd like to wait here, but can't, since waiting in DllMain 	*/
    /* provokes deadlocks.						*/
    /* Thus we force marking to be restarted instead.			*/
  } else {
    GC_ASSERT(!GC_please_stop);
  	/* Otherwise both we and the thread stopping code would be	*/
  	/* holding the allocation lock.					*/
  }
  return (GC_thread)(me);
}

/*
 * GC_max_thread_index may temporarily be larger than MAX_THREADS.
 * To avoid subscript errors, we check on access.
 */
#ifdef __GNUC__
__inline__
#endif
LONG GC_get_max_thread_index()
{
  LONG my_max = GC_max_thread_index;

  if (my_max >= MAX_THREADS) return MAX_THREADS-1;
  return my_max;
}

/* Return the GC_thread corresponding to a thread id.  May be called 	*/
/* without a lock, but should be called in contexts in which the	*/
/* requested thread cannot be asynchronously deleted, e.g. from the	*/
/* thread itself.							*/
/* This version assumes that either GC_win32_dll_threads is set, or	*/
/* we hold the allocator lock.						*/
/* Also used (for assertion checking only) from thread_local_alloc.c.	*/
GC_thread GC_lookup_thread_inner(DWORD thread_id) {
  if (GC_win32_dll_threads) {
    int i;
    LONG my_max = GC_get_max_thread_index();
    for (i = 0;
       i <= my_max &&
       (!AO_load_acquire(&(dll_thread_table[i].in_use))
	|| dll_thread_table[i].id != thread_id);
       /* Must still be in_use, since nobody else can store our thread_id. */
       i++) {}
    if (i > my_max) {
      return 0;
    } else {
      return (GC_thread)(dll_thread_table + i);
    }
  } else {
    word hv = ((word)thread_id) % THREAD_TABLE_SZ;
    register GC_thread p = GC_threads[hv];
    
    GC_ASSERT(I_HOLD_LOCK());
    while (p != 0 && p -> id != thread_id) p = p -> next;
    return(p);
  }
}

/* A version of the above that acquires the lock if necessary.  Note	*/
/* that the identically named function for pthreads is different, and	*/
/* just assumes we hold the lock.					*/
/* Also used (for assertion checking only) from thread_local_alloc.c.	*/
static GC_thread GC_lookup_thread(DWORD thread_id)
{
  if (GC_win32_dll_threads) {
    return GC_lookup_thread_inner(thread_id);
  } else {
    GC_thread result;
    LOCK();
    result = GC_lookup_thread_inner(thread_id);
    UNLOCK();
    return result;
  }
}

/* If a thread has been joined, but we have not yet		*/
/* been notified, then there may be more than one thread 	*/
/* in the table with the same win32 id.				*/
/* This is OK, but we need a way to delete a specific one.	*/
/* Assumes we hold the allocation lock unless			*/
/* GC_win32_dll_threads is set.					*/
/* If GC_win32_dll_threads is set it should be called from the	*/
/* thread being deleted.					*/
void GC_delete_gc_thread(GC_vthread gc_id)
{
  if (GC_win32_dll_threads) {
    /* This is intended to be lock-free.				*/
    /* It is either called synchronously from the thread being deleted,	*/
    /* or by the joining thread.					*/
    /* In this branch asynchronosu changes to *gc_id are possible.	*/
    CloseHandle(gc_id->handle);
    gc_id -> stack_base = 0;
    gc_id -> id = 0;
#   ifdef CYGWIN32
      gc_id -> pthread_id = 0;
#   endif /* CYGWIN32 */
#   ifdef GC_WIN32_PTHREADS
      gc_id -> pthread_id.p = NULL;
#   endif /* GC_WIN32_PTHREADS */
    AO_store_release(&(gc_id->in_use), FALSE);
  } else {
    /* Cast away volatile qualifier, since we have lock. */
    GC_thread gc_nvid = (GC_thread)gc_id;
    DWORD id = gc_nvid -> id;
    word hv = ((word)id) % THREAD_TABLE_SZ;
    register GC_thread p = GC_threads[hv];
    register GC_thread prev = 0;

    GC_ASSERT(I_HOLD_LOCK());
    while (p != gc_nvid) {
        prev = p;
        p = p -> next;
    }
    if (prev == 0) {
        GC_threads[hv] = p -> next;
    } else {
        prev -> next = p -> next;
    }
    GC_INTERNAL_FREE(p);
  }
}

/* Delete a thread from GC_threads.  We assume it is there.	*/
/* (The code intentionally traps if it wasn't.)			*/
/* Assumes we hold the allocation lock unless			*/
/* GC_win32_dll_threads is set.					*/
/* If GC_win32_dll_threads is set it should be called from the	*/
/* thread being deleted.					*/
void GC_delete_thread(DWORD id)
{
  if (GC_win32_dll_threads) {
    GC_thread t = GC_lookup_thread_inner(id);

    if (0 == t) {
      WARN("Removing nonexistent thread %ld\n", (GC_word)id);
    } else {
      GC_delete_gc_thread(t);
    }
  } else {
    word hv = ((word)id) % THREAD_TABLE_SZ;
    register GC_thread p = GC_threads[hv];
    register GC_thread prev = 0;
    
    GC_ASSERT(I_HOLD_LOCK());
    while (p -> id != id) {
        prev = p;
        p = p -> next;
    }
    if (prev == 0) {
        GC_threads[hv] = p -> next;
    } else {
        prev -> next = p -> next;
    }
    GC_INTERNAL_FREE(p);
  }
}

int GC_register_my_thread(struct GC_stack_base *sb) {
  DWORD t = GetCurrentThreadId();

  if (0 == GC_lookup_thread(t)) {
    /* We lock here, since we want to wait for an ongoing GC.	*/
    LOCK();
    GC_register_my_thread_inner(sb, t);
    UNLOCK();
    return GC_SUCCESS;
  } else {
    return GC_DUPLICATE;
  }
}

int GC_unregister_my_thread(void)
{
    DWORD t = GetCurrentThreadId();

#   if defined(THREAD_LOCAL_ALLOC)
      LOCK();
      {
	GC_thread me = GC_lookup_thread_inner(t);
        GC_destroy_thread_local(&(me->tlfs));
      }
      UNLOCK();
#   endif
    if (GC_win32_dll_threads) {
      /* Should we just ignore this? */
      GC_delete_thread(t);
    } else {
      LOCK();
      GC_delete_thread(t);
      UNLOCK();
    }
    return GC_SUCCESS;
}


#ifdef GC_PTHREADS

/* A quick-and-dirty cache of the mapping between pthread_t	*/
/* and win32 thread id.						*/
#define PTHREAD_MAP_SIZE 512
DWORD GC_pthread_map_cache[PTHREAD_MAP_SIZE];
#define HASH(pthread_id) ((NUMERIC_THREAD_ID(pthread_id) >> 5) % PTHREAD_MAP_SIZE)
	/* It appears pthread_t is really a pointer type ... */
#define SET_PTHREAD_MAP_CACHE(pthread_id, win32_id) \
	GC_pthread_map_cache[HASH(pthread_id)] = (win32_id);
#define GET_PTHREAD_MAP_CACHE(pthread_id) \
	GC_pthread_map_cache[HASH(pthread_id)]

/* Return a GC_thread corresponding to a given pthread_t.	*/
/* Returns 0 if it's not there.					*/
/* We assume that this is only called for pthread ids that	*/
/* have not yet terminated or are still joinable, and		*/
/* cannot be concurrently terminated.				*/
/* Assumes we do NOT hold the allocation lock.			*/
static GC_thread GC_lookup_pthread(pthread_t id)
{
  if (GC_win32_dll_threads) {
    int i;
    LONG my_max = GC_get_max_thread_index();

    for (i = 0;
         i <= my_max &&
         (!AO_load_acquire(&(dll_thread_table[i].in_use))
	  || THREAD_EQUAL(dll_thread_table[i].pthread_id, id));
       /* Must still be in_use, since nobody else can store our thread_id. */
       i++);
    if (i > my_max) return 0;
    return (GC_thread)(dll_thread_table + i);
  } else {
    /* We first try the cache.  If that fails, we use a very slow	*/
    /* approach.							*/
    int hv_guess = GET_PTHREAD_MAP_CACHE(id) % THREAD_TABLE_SZ;
    int hv;
    GC_thread p;

    LOCK();
    for (p = GC_threads[hv_guess]; 0 != p; p = p -> next) {
      if (THREAD_EQUAL(p -> pthread_id, id))
	goto foundit; 
    }
    for (hv = 0; hv < THREAD_TABLE_SZ; ++hv) {
      for (p = GC_threads[hv]; 0 != p; p = p -> next) {
        if (THREAD_EQUAL(p -> pthread_id, id))
	  goto foundit; 
      }
    }
    p = 0;
   foundit:
    UNLOCK();
    return p;
  }
}

#endif /* GC_PTHREADS */

void GC_push_thread_structures(void)
{
  GC_ASSERT(I_HOLD_LOCK());
  if (GC_win32_dll_threads) {
    /* Unlike the other threads implementations, the thread table here	*/
    /* contains no pointers to the collectable heap.  Thus we have	*/
    /* no private structures we need to preserve.			*/
#   ifdef GC_PTHREADS 
    { int i; /* pthreads may keep a pointer in the thread exit value */
      LONG my_max = GC_get_max_thread_index();

      for (i = 0; i <= my_max; i++)
        if (dll_thread_table[i].in_use)
	  GC_push_all((ptr_t)&(dll_thread_table[i].status),
                      (ptr_t)(&(dll_thread_table[i].status)+1));
    }
#   endif
  } else {
    GC_push_all((ptr_t)(GC_threads), (ptr_t)(GC_threads)+sizeof(GC_threads));
  }
# if defined(THREAD_LOCAL_ALLOC)
    GC_push_all((ptr_t)(&GC_thread_key),
      (ptr_t)(&GC_thread_key)+sizeof(&GC_thread_key));
    /* Just in case we ever use our own TLS implementation.	*/
# endif
}

/* Suspend the given thread, if it's still active.	*/
void GC_suspend(GC_thread t)
{
# ifdef MSWINCE
    /* SuspendThread will fail if thread is running kernel code */
      while (SuspendThread(t -> handle) == (DWORD)-1)
	Sleep(10);
# else
    /* Apparently the Windows 95 GetOpenFileName call creates	*/
    /* a thread that does not properly get cleaned up, and		*/
    /* SuspendThread on its descriptor may provoke a crash.		*/
    /* This reduces the probability of that event, though it still	*/
    /* appears there's a race here.					*/
    DWORD exitCode; 
    if (GetExitCodeThread(t -> handle, &exitCode) &&
        exitCode != STILL_ACTIVE) {
      t -> stack_base = 0; /* prevent stack from being pushed */
#     ifndef GC_PTHREADS
        /* this breaks pthread_join on Cygwin, which is guaranteed to  */
        /* only see user pthreads 	 			       */
        AO_store(&(t -> in_use), FALSE);
        CloseHandle(t -> handle);
#     endif
      return;
    }
    if (SuspendThread(t -> handle) == (DWORD)-1)
      ABORT("SuspendThread failed");
# endif
   t -> suspended = TRUE;
}

/* Defined in misc.c */
#ifndef CYGWIN32
  extern CRITICAL_SECTION GC_write_cs;
#endif

void GC_stop_world(void)
{
  DWORD thread_id = GetCurrentThreadId();
  int i;

  if (!GC_thr_initialized) ABORT("GC_stop_world() called before GC_thr_init()");
  GC_ASSERT(I_HOLD_LOCK());

  GC_please_stop = TRUE;
# ifndef CYGWIN32
    EnterCriticalSection(&GC_write_cs);
# endif
  if (GC_win32_dll_threads) {
    /* Any threads being created during this loop will end up setting   */
    /* GC_attached_thread when they start.  This will force marking to  */
    /* restart.								*/
    /* This is not ideal, but hopefully correct.			*/
    GC_attached_thread = FALSE;
    for (i = 0; i <= GC_get_max_thread_index(); i++) {
      GC_vthread t = dll_thread_table + i;
      if (t -> stack_base != 0
	  && t -> id != thread_id) {
	  GC_suspend((GC_thread)t);
      }
    }
  } else {
      GC_thread t;
      int i;

      for (i = 0; i < THREAD_TABLE_SZ; i++) {
        for (t = GC_threads[i]; t != 0; t = t -> next) {
	  if (t -> stack_base != 0
	  && !KNOWN_FINISHED(t)
	  && t -> id != thread_id) {
	    GC_suspend(t);
	  }
	}
      }
  }
# ifndef CYGWIN32
    LeaveCriticalSection(&GC_write_cs);
# endif    
}

void GC_start_world(void)
{
  DWORD thread_id = GetCurrentThreadId();
  int i;
  LONG my_max = GC_get_max_thread_index();

  GC_ASSERT(I_HOLD_LOCK());
  if (GC_win32_dll_threads) {
    for (i = 0; i <= my_max; i++) {
      GC_thread t = (GC_thread)(dll_thread_table + i);
      if (t -> stack_base != 0 && t -> suspended
	  && t -> id != thread_id) {
        if (ResumeThread(t -> handle) == (DWORD)-1)
	  ABORT("ResumeThread failed");
        t -> suspended = FALSE;
      }
    }
  } else {
    GC_thread t;
    int i;

    for (i = 0; i < THREAD_TABLE_SZ; i++) {
      for (t = GC_threads[i]; t != 0; t = t -> next) {
        if (t -> stack_base != 0 && t -> suspended
	    && t -> id != thread_id) {
          if (ResumeThread(t -> handle) == (DWORD)-1)
	    ABORT("ResumeThread failed");
          t -> suspended = FALSE;
        }
      }
    }
  }
  GC_please_stop = FALSE;
}

# ifdef MSWINCE
    /* The VirtualQuery calls below won't work properly on WinCE, but	*/
    /* since each stack is restricted to an aligned 64K region of	*/
    /* virtual memory we can just take the next lowest multiple of 64K.	*/
#   define GC_get_stack_min(s) \
        ((ptr_t)(((DWORD)(s) - 1) & 0xFFFF0000))
# else
    static ptr_t GC_get_stack_min(ptr_t s)
    {
	ptr_t bottom;
	MEMORY_BASIC_INFORMATION info;
	VirtualQuery(s, &info, sizeof(info));
	do {
	    bottom = info.BaseAddress;
	    VirtualQuery(bottom - 1, &info, sizeof(info));
	} while ((info.Protect & PAGE_READWRITE)
		 && !(info.Protect & PAGE_GUARD));
	return(bottom);
    }
# endif

void GC_push_stack_for(GC_thread thread)
{
    int dummy;
    ptr_t sp, stack_min;
    DWORD me = GetCurrentThreadId();

    if (thread -> stack_base) {
      if (thread -> id == me) {
	sp = (ptr_t) &dummy;
      } else {
        CONTEXT context;
        context.ContextFlags = CONTEXT_INTEGER|CONTEXT_CONTROL;
        if (!GetThreadContext(thread -> handle, &context))
	  ABORT("GetThreadContext failed");

        /* Push all registers that might point into the heap.  Frame	*/
        /* pointer registers are included in case client code was	*/
        /* compiled with the 'omit frame pointer' optimisation.		*/
#       define PUSH1(reg) GC_push_one((word)context.reg)
#       define PUSH2(r1,r2) PUSH1(r1), PUSH1(r2)
#       define PUSH4(r1,r2,r3,r4) PUSH2(r1,r2), PUSH2(r3,r4)
#       if defined(I386)
          PUSH4(Edi,Esi,Ebx,Edx), PUSH2(Ecx,Eax), PUSH1(Ebp);
	  sp = (ptr_t)context.Esp;
#	elif defined(X86_64)
	  PUSH4(Rax,Rcx,Rdx,Rbx); PUSH2(Rbp, Rsi); PUSH1(Rdi);
	  PUSH4(R8, R9, R10, R11); PUSH4(R12, R13, R14, R15);
	  sp = (ptr_t)context.Rsp;
#       elif defined(ARM32)
	  PUSH4(R0,R1,R2,R3),PUSH4(R4,R5,R6,R7),PUSH4(R8,R9,R10,R11),PUSH1(R12);
	  sp = (ptr_t)context.Sp;
#       elif defined(SHx)
	  PUSH4(R0,R1,R2,R3), PUSH4(R4,R5,R6,R7), PUSH4(R8,R9,R10,R11);
	  PUSH2(R12,R13), PUSH1(R14);
	  sp = (ptr_t)context.R15;
#       elif defined(MIPS)
	  PUSH4(IntAt,IntV0,IntV1,IntA0), PUSH4(IntA1,IntA2,IntA3,IntT0);
	  PUSH4(IntT1,IntT2,IntT3,IntT4), PUSH4(IntT5,IntT6,IntT7,IntS0);
	  PUSH4(IntS1,IntS2,IntS3,IntS4), PUSH4(IntS5,IntS6,IntS7,IntT8);
	  PUSH4(IntT9,IntK0,IntK1,IntS8);
	  sp = (ptr_t)context.IntSp;
#       elif defined(PPC)
	  PUSH4(Gpr0, Gpr3, Gpr4, Gpr5),  PUSH4(Gpr6, Gpr7, Gpr8, Gpr9);
	  PUSH4(Gpr10,Gpr11,Gpr12,Gpr14), PUSH4(Gpr15,Gpr16,Gpr17,Gpr18);
	  PUSH4(Gpr19,Gpr20,Gpr21,Gpr22), PUSH4(Gpr23,Gpr24,Gpr25,Gpr26);
	  PUSH4(Gpr27,Gpr28,Gpr29,Gpr30), PUSH1(Gpr31);
	  sp = (ptr_t)context.Gpr1;
#       elif defined(ALPHA)
	  PUSH4(IntV0,IntT0,IntT1,IntT2), PUSH4(IntT3,IntT4,IntT5,IntT6);
	  PUSH4(IntT7,IntS0,IntS1,IntS2), PUSH4(IntS3,IntS4,IntS5,IntFp);
	  PUSH4(IntA0,IntA1,IntA2,IntA3), PUSH4(IntA4,IntA5,IntT8,IntT9);
	  PUSH4(IntT10,IntT11,IntT12,IntAt);
	  sp = (ptr_t)context.IntSp;
#       else
#         error "architecture is not supported"
#       endif
      } /* ! current thread */

      stack_min = GC_get_stack_min(thread->stack_base);

      if (sp >= stack_min && sp < thread->stack_base) {
#       if DEBUG_WIN32_PTHREADS || DEBUG_WIN32_THREADS \
           || DEBUG_CYGWIN_THREADS
	  GC_printf("Pushing thread from %p to %p for 0x%x from 0x%x\n",
		    sp, thread -> stack_base, thread -> id, me);
#       endif
        GC_push_all_stack(sp, thread->stack_base);
      } else {
        WARN("Thread stack pointer 0x%lx out of range, pushing everything\n",
	     (unsigned long)(size_t)sp);
        GC_push_all_stack(stack_min, thread->stack_base);
      }
    } /* thread looks live */
}

void GC_push_all_stacks(void)
{
  DWORD me = GetCurrentThreadId();
  GC_bool found_me = FALSE;
  size_t nthreads = 0;
  
  if (GC_win32_dll_threads) {
    int i;
    LONG my_max = GC_get_max_thread_index();

    for (i = 0; i <= my_max; i++) {
      GC_thread t = (GC_thread)(dll_thread_table + i);
      if (t -> in_use) {
        ++nthreads;
        GC_push_stack_for(t);
        if (t -> id == me) found_me = TRUE;
      }
    }
  } else {
    GC_thread t;
    int i;

    for (i = 0; i < THREAD_TABLE_SZ; i++) {
      for (t = GC_threads[i]; t != 0; t = t -> next) {
        ++nthreads;
        if (!KNOWN_FINISHED(t)) GC_push_stack_for(t);
        if (t -> id == me) found_me = TRUE;
      }
    }
  }
  if (GC_print_stats == VERBOSE) {
    GC_log_printf("Pushed %d thread stacks ", nthreads);
    if (GC_win32_dll_threads) {
    	GC_log_printf("based on DllMain thread tracking\n");
    } else {
    	GC_log_printf("\n");
    }
  }
  if (!found_me && !GC_in_thread_creation)
    ABORT("Collecting from unknown thread.");
}

void GC_get_next_stack(char *start, char **lo, char **hi)
{
    int i;
#   define ADDR_LIMIT (char *)(-1L)
    char * current_min = ADDR_LIMIT;

    if (GC_win32_dll_threads) {
      LONG my_max = GC_get_max_thread_index();
  
      for (i = 0; i <= my_max; i++) {
    	ptr_t s = (ptr_t)(dll_thread_table[i].stack_base);

	if (0 != s && s > start && s < current_min) {
	    current_min = s;
	}
      }
    } else {
      for (i = 0; i < THREAD_TABLE_SZ; i++) {
	GC_thread t;

        for (t = GC_threads[i]; t != 0; t = t -> next) {
	  ptr_t s = (ptr_t)(t -> stack_base);

	  if (0 != s && s > start && s < current_min) {
	    current_min = s;
	  }
        }
      }
    }
    *hi = current_min;
    if (current_min == ADDR_LIMIT) {
    	*lo = ADDR_LIMIT;
	return;
    }
    *lo = GC_get_stack_min(current_min);
    if (*lo < start) *lo = start;
}

#ifndef GC_PTHREADS

/* We have no DllMain to take care of new threads.  Thus we	*/
/* must properly intercept thread creation.			*/

typedef struct {
    LPTHREAD_START_ROUTINE start;
    LPVOID param;
} thread_args;

static DWORD WINAPI thread_start(LPVOID arg);

void * GC_win32_start_inner(struct GC_stack_base *sb, LPVOID arg)
{
    void * ret;
    thread_args *args = (thread_args *)arg;

#   if DEBUG_WIN32_THREADS
      GC_printf("thread 0x%x starting...\n", GetCurrentThreadId());
#   endif

    GC_register_my_thread(sb); /* This waits for an in-progress GC. */

    /* Clear the thread entry even if we exit with an exception.	*/
    /* This is probably pointless, since an uncaught exception is	*/
    /* supposed to result in the process being killed.			*/
#ifndef __GNUC__
    __try {
#endif /* __GNUC__ */
	ret = (void *)(size_t)args->start (args->param);
#ifndef __GNUC__
    } __finally {
#endif /* __GNUC__ */
	GC_unregister_my_thread();
	GC_free(args);
#ifndef __GNUC__
    }
#endif /* __GNUC__ */

#   if DEBUG_WIN32_THREADS
      GC_printf("thread 0x%x returned from start routine.\n",
		GetCurrentThreadId());
#   endif
    return ret;
}

DWORD WINAPI GC_win32_start(LPVOID arg)
{
    return (DWORD)(size_t)GC_call_with_stack_base(GC_win32_start_inner, arg);
}

GC_API HANDLE WINAPI GC_CreateThread(
    LPSECURITY_ATTRIBUTES lpThreadAttributes, 
    DWORD dwStackSize, LPTHREAD_START_ROUTINE lpStartAddress, 
    LPVOID lpParameter, DWORD dwCreationFlags, LPDWORD lpThreadId )
{
    HANDLE thread_h = NULL;

    thread_args *args;

    if (!parallel_initialized) GC_init_parallel();
    		/* make sure GC is initialized (i.e. main thread is attached,
		   tls initialized) */

#   if DEBUG_WIN32_THREADS
      GC_printf("About to create a thread from 0x%x\n", GetCurrentThreadId());
#   endif
    if (GC_win32_dll_threads) {
      return CreateThread(lpThreadAttributes, dwStackSize, lpStartAddress,
                        lpParameter, dwCreationFlags, lpThreadId);
    } else {
      args = GC_malloc_uncollectable(sizeof(thread_args)); 
	/* Handed off to and deallocated by child thread.	*/
      if (0 == args) {
	SetLastError(ERROR_NOT_ENOUGH_MEMORY);
        return NULL;
      }

      /* set up thread arguments */
    	args -> start = lpStartAddress;
    	args -> param = lpParameter;

      GC_need_to_lock = TRUE;
      thread_h = CreateThread(lpThreadAttributes,
    			      dwStackSize, GC_win32_start,
    			      args, dwCreationFlags,
    			      lpThreadId);
      if( thread_h == 0 ) GC_free( args );
      return thread_h;
    }
}

void WINAPI GC_ExitThread(DWORD dwExitCode)
{
  GC_unregister_my_thread();
  ExitThread(dwExitCode);
}

uintptr_t GC_beginthreadex(
    void *security, unsigned stack_size,
    unsigned ( __stdcall *start_address )( void * ),
    void *arglist, unsigned initflag, unsigned *thrdaddr)
{
    uintptr_t thread_h = -1L;

    thread_args *args;

    if (!parallel_initialized) GC_init_parallel();
    		/* make sure GC is initialized (i.e. main thread is attached,
		   tls initialized) */
#   if DEBUG_WIN32_THREADS
      GC_printf("About to create a thread from 0x%x\n", GetCurrentThreadId());
#   endif

    if (GC_win32_dll_threads) {
      return _beginthreadex(security, stack_size, start_address,
                            arglist, initflag, thrdaddr);
    } else {
      args = GC_malloc_uncollectable(sizeof(thread_args)); 
	/* Handed off to and deallocated by child thread.	*/
      if (0 == args) {
	SetLastError(ERROR_NOT_ENOUGH_MEMORY);
        return (uintptr_t)(-1);
      }

      /* set up thread arguments */
    	args -> start = (LPTHREAD_START_ROUTINE)start_address;
    	args -> param = arglist;

      GC_need_to_lock = TRUE;
      thread_h = _beginthreadex(security, stack_size,
      		 (unsigned (__stdcall *) (void *))GC_win32_start,
                                args, initflag, thrdaddr);
      if( thread_h == 0 ) GC_free( args );
      return thread_h;
    }
}

void GC_endthreadex(unsigned retval)
{
  GC_unregister_my_thread();
  _endthreadex(retval);
}

#endif /* !GC_PTHREADS */

#ifdef MSWINCE

typedef struct {
    HINSTANCE hInstance;
    HINSTANCE hPrevInstance;
    LPWSTR lpCmdLine;
    int nShowCmd;
} main_thread_args;

DWORD WINAPI main_thread_start(LPVOID arg);

int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance,
		   LPWSTR lpCmdLine, int nShowCmd)
{
    DWORD exit_code = 1;

    main_thread_args args = {
	hInstance, hPrevInstance, lpCmdLine, nShowCmd
    };
    HANDLE thread_h;
    DWORD thread_id;

    /* initialize everything */
    GC_init();

    /* start the main thread */
    thread_h = GC_CreateThread(
	NULL, 0, main_thread_start, &args, 0, &thread_id);

    if (thread_h != NULL)
    {
	WaitForSingleObject (thread_h, INFINITE);
	GetExitCodeThread (thread_h, &exit_code);
	CloseHandle (thread_h);
    }

    GC_deinit();
    DeleteCriticalSection(&GC_allocate_ml);

    return (int) exit_code;
}

DWORD WINAPI main_thread_start(LPVOID arg)
{
    main_thread_args * args = (main_thread_args *) arg;

    return (DWORD) GC_WinMain (args->hInstance, args->hPrevInstance,
			       args->lpCmdLine, args->nShowCmd);
}

# else /* !MSWINCE */

/* Called by GC_init() - we hold the allocation lock.	*/
void GC_thr_init(void) {
    struct GC_stack_base sb;
    int sb_result;

    GC_ASSERT(I_HOLD_LOCK());
    if (GC_thr_initialized) return;
    GC_main_thread = GetCurrentThreadId();
    GC_thr_initialized = TRUE;

    /* Add the initial thread, so we can stop it.	*/
    sb_result = GC_get_stack_base(&sb);
    GC_ASSERT(sb_result == GC_SUCCESS);
    GC_register_my_thread(&sb);
}

#ifdef GC_PTHREADS

struct start_info {
    void *(*start_routine)(void *);
    void *arg;
    GC_bool detached;
};

int GC_pthread_join(pthread_t pthread_id, void **retval) {
    int result;
    int i;
    GC_thread joinee;

#   if DEBUG_CYGWIN_THREADS
      GC_printf("thread 0x%x(0x%x) is joining thread 0x%x.\n",
		(int)pthread_self(), GetCurrentThreadId(), (int)pthread_id);
#   endif
#   if DEBUG_WIN32_PTHREADS
      GC_printf("thread 0x%x(0x%x) is joining thread 0x%x.\n",
		(int)(pthread_self()).p, GetCurrentThreadId(), pthread_id.p);
#   endif

    if (!parallel_initialized) GC_init_parallel();
    /* Thread being joined might not have registered itself yet. */
    /* After the join,thread id may have been recycled.		 */
    /* FIXME: It would be better if this worked more like	 */
    /* pthread_support.c.					 */

    #ifndef GC_WIN32_PTHREADS
      while ((joinee = GC_lookup_pthread(pthread_id)) == 0) Sleep(10);
    #endif

    result = pthread_join(pthread_id, retval);

    #ifdef GC_WIN32_PTHREADS
      /* win32_pthreads id are unique */
      joinee = GC_lookup_pthread(pthread_id);
    #endif

    if (!GC_win32_dll_threads) {
      LOCK();
      GC_delete_gc_thread(joinee);
      UNLOCK();
    } /* otherwise dllmain handles it.	*/

#   if DEBUG_CYGWIN_THREADS
      GC_printf("thread 0x%x(0x%x) completed join with thread 0x%x.\n",
		 (int)pthread_self(), GetCurrentThreadId(), (int)pthread_id);
#   endif
#   if DEBUG_WIN32_PTHREADS
      GC_printf("thread 0x%x(0x%x) completed join with thread 0x%x.\n",
		(int)(pthread_self()).p, GetCurrentThreadId(), pthread_id.p);
#   endif

    return result;
}

/* Cygwin-pthreads calls CreateThread internally, but it's not
 * easily interceptible by us..
 *   so intercept pthread_create instead
 */
int
GC_pthread_create(pthread_t *new_thread,
		  const pthread_attr_t *attr,
                  void *(*start_routine)(void *), void *arg) {
    int result;
    struct start_info * si;

    if (!parallel_initialized) GC_init_parallel();
    		/* make sure GC is initialized (i.e. main thread is attached) */
    if (GC_win32_dll_threads) {
      return pthread_create(new_thread, attr, start_routine, arg);
    }
    
    /* This is otherwise saved only in an area mmapped by the thread */
    /* library, which isn't visible to the collector.		 */
    si = GC_malloc_uncollectable(sizeof(struct start_info)); 
    if (0 == si) return(EAGAIN);

    si -> start_routine = start_routine;
    si -> arg = arg;
    if (attr != 0 &&
        pthread_attr_getdetachstate(attr, &si->detached)
	== PTHREAD_CREATE_DETACHED) {
      si->detached = TRUE;
    }

#   if DEBUG_CYGWIN_THREADS
      GC_printf("About to create a thread from 0x%x(0x%x)\n",
		(int)pthread_self(), GetCurrentThreadId);
#   endif
#   if DEBUG_WIN32_PTHREADS
      GC_printf("About to create a thread from 0x%x(0x%x)\n",
		(int)(pthread_self()).p, GetCurrentThreadId());
#   endif
    GC_need_to_lock = TRUE;
    result = pthread_create(new_thread, attr, GC_pthread_start, si); 

    if (result) { /* failure */
      	GC_free(si);
    } 

    return(result);
}

void * GC_pthread_start_inner(struct GC_stack_base *sb, void * arg)
{
    struct start_info * si = arg;
    void * result;
    void *(*start)(void *);
    void *start_arg;
    DWORD thread_id = GetCurrentThreadId();
    pthread_t pthread_id = pthread_self();
    GC_thread me;
    GC_bool detached;
    int i;

#   if DEBUG_CYGWIN_THREADS
      GC_printf("thread 0x%x(0x%x) starting...\n",(int)pthread_id,
		      				  thread_id);
#   endif
#   if DEBUG_WIN32_PTHREADS
      GC_printf("thread 0x%x(0x%x) starting...\n",(int) pthread_id.p,
      						  thread_id);
#   endif

    GC_ASSERT(!GC_win32_dll_threads);
    /* If a GC occurs before the thread is registered, that GC will	*/
    /* ignore this thread.  That's fine, since it will block trying to  */
    /* acquire the allocation lock, and won't yet hold interesting 	*/
    /* pointers.							*/
    LOCK();
    /* We register the thread here instead of in the parent, so that	*/
    /* we don't need to hold the allocation lock during pthread_create. */
    me = GC_register_my_thread_inner(sb, thread_id);
    SET_PTHREAD_MAP_CACHE(pthread_id, thread_id);
    UNLOCK();

    start = si -> start_routine;
    start_arg = si -> arg;
    if (si-> detached) me -> flags |= DETACHED;
    me -> pthread_id = pthread_id;

    GC_free(si); /* was allocated uncollectable */

    pthread_cleanup_push(GC_thread_exit_proc, (void *)me);
    result = (*start)(start_arg);
    me -> status = result;
    pthread_cleanup_pop(1);

#   if DEBUG_CYGWIN_THREADS
      GC_printf("thread 0x%x(0x%x) returned from start routine.\n",
		(int)pthread_self(),GetCurrentThreadId());
#   endif
#   if DEBUG_WIN32_PTHREADS
      GC_printf("thread 0x%x(0x%x) returned from start routine.\n",
		(int)(pthread_self()).p, GetCurrentThreadId());
#   endif

    return(result);
}

void * GC_pthread_start(void * arg)
{
    return GC_call_with_stack_base(GC_pthread_start_inner, arg);
}

void GC_thread_exit_proc(void *arg)
{
    GC_thread me = (GC_thread)arg;
    int i;

    GC_ASSERT(!GC_win32_dll_threads);
#   if DEBUG_CYGWIN_THREADS
      GC_printf("thread 0x%x(0x%x) called pthread_exit().\n",
		(int)pthread_self(),GetCurrentThreadId());
#   endif
#   if DEBUG_WIN32_PTHREADS
      GC_printf("thread 0x%x(0x%x) called pthread_exit().\n",
		(int)(pthread_self()).p,GetCurrentThreadId());
#   endif

    LOCK();
#   if defined(THREAD_LOCAL_ALLOC)
      GC_destroy_thread_local(&(me->tlfs));
#   endif
    if (me -> flags & DETACHED) {
      GC_delete_thread(GetCurrentThreadId());
    } else {
      /* deallocate it as part of join */
      me -> flags |= FINISHED;
    }
    UNLOCK();
}

#ifndef GC_WIN32_PTHREADS
/* win32 pthread does not support sigmask */
/* nothing required here... */
int GC_pthread_sigmask(int how, const sigset_t *set, sigset_t *oset) {
  if (!parallel_initialized) GC_init_parallel();
  return pthread_sigmask(how, set, oset);
}
#endif

int GC_pthread_detach(pthread_t thread)
{
    int result;
    GC_thread thread_gc_id;
    
    if (!parallel_initialized) GC_init_parallel();
    LOCK();
    thread_gc_id = GC_lookup_pthread(thread);
    UNLOCK();
    result = pthread_detach(thread);
    if (result == 0) {
      LOCK();
      thread_gc_id -> flags |= DETACHED;
      /* Here the pthread thread id may have been recycled. */
      if (thread_gc_id -> flags & FINISHED) {
        GC_delete_gc_thread(thread_gc_id);
      }
      UNLOCK();
    }
    return result;
}

#else /* !GC_PTHREADS */

/*
 * We avoid acquiring locks here, since this doesn't seem to be preemptable.
 * This may run with an uninitialized collector, in which case we don't do much.
 * This implies that no threads other than the main one should be created
 * with an uninitialized collector.  (The alternative of initializing
 * the collector here seems dangerous, since DllMain is limited in what it
 * can do.)
 */
#ifdef GC_DLL
GC_API BOOL WINAPI DllMain(HINSTANCE inst, ULONG reason, LPVOID reserved)
{
  struct GC_stack_base sb;
  DWORD thread_id;
  int sb_result;
  static int entry_count = 0;

  if (parallel_initialized && !GC_win32_dll_threads) return TRUE;

  switch (reason) {
   case DLL_THREAD_ATTACH:
    GC_ASSERT(entry_count == 0 || parallel_initialized);
    ++entry_count; /* and fall through: */
   case DLL_PROCESS_ATTACH:
    /* This may run with the collector uninitialized. */
    thread_id = GetCurrentThreadId();
    if (parallel_initialized && GC_main_thread != thread_id) {
	/* Don't lock here.	*/
        sb_result = GC_get_stack_base(&sb);
        GC_ASSERT(sb_result == GC_SUCCESS);
#       ifdef THREAD_LOCAL_ALLOC
	  ABORT("Cannot initialize thread local cache from DllMain");
#       endif
	GC_register_my_thread_inner(&sb, thread_id);
    } /* o.w. we already did it during GC_thr_init(), called by GC_init() */
    break;

   case DLL_THREAD_DETACH:
    /* We are hopefully running in the context of the exiting thread.	*/
    GC_ASSERT(parallel_initialized);
    if (!GC_win32_dll_threads) return TRUE;
    GC_delete_thread(GetCurrentThreadId());
    break;

   case DLL_PROCESS_DETACH:
    {
      int i;

      if (!GC_win32_dll_threads) return TRUE;
      for (i = 0; i <= GC_get_max_thread_index(); ++i)
      {
          if (AO_load(&(dll_thread_table[i].in_use)))
	    GC_delete_gc_thread(dll_thread_table + i);
      }

      GC_deinit();
      DeleteCriticalSection(&GC_allocate_ml);
    }
    break;

  }
  return TRUE;
}
#endif /* GC_DLL */
#endif /* !GC_PTHREADS */

# endif /* !MSWINCE */

/* Perform all initializations, including those that	*/
/* may require allocation.				*/
/* Called without allocation lock.			*/
/* Must be called before a second thread is created.	*/
void GC_init_parallel(void)
{
    if (parallel_initialized) return;
    parallel_initialized = TRUE;
    /* GC_init() calls us back, so set flag first.	*/
    
    if (!GC_is_initialized) GC_init();
    if (GC_win32_dll_threads) {
      GC_need_to_lock = TRUE;
  	/* Cannot intercept thread creation.  Hence we don't know if other	*/
	/* threads exist.  However, client is not allowed to create other	*/
	/* threads before collector initialization.  Thus it's OK not to	*/
	/* lock before this.							*/
    }
    /* Initialize thread local free lists if used.	*/
#   if defined(THREAD_LOCAL_ALLOC)
      LOCK();
      GC_init_thread_local(&(GC_lookup_thread(GetCurrentThreadId())->tlfs));
      UNLOCK();
#   endif
}

#if defined(USE_PTHREAD_LOCKS)
  /* Support for pthread locking code.		*/
  /* Pthread_mutex_try_lock may not win here,	*/
  /* due to builtinsupport for spinning first?	*/

volatile GC_bool GC_collecting = 0;
			/* A hint that we're in the collector and       */
                        /* holding the allocation lock for an           */
                        /* extended period.                             */

void GC_lock(void)
{
    pthread_mutex_lock(&GC_allocate_ml);
}
#endif /* USE_PTHREAD ... */

# if defined(THREAD_LOCAL_ALLOC)

/* Add thread-local allocation support.  Microsoft uses __declspec(thread) */

/* We must explicitly mark ptrfree and gcj free lists, since the free 	*/
/* list links wouldn't otherwise be found.  We also set them in the 	*/
/* normal free lists, since that involves touching less memory than if	*/
/* we scanned them normally.						*/
void GC_mark_thread_local_free_lists(void)
{
    int i;
    GC_thread p;
    
    for (i = 0; i < THREAD_TABLE_SZ; ++i) {
      for (p = GC_threads[i]; 0 != p; p = p -> next) {
	GC_mark_thread_local_fls_for(&(p->tlfs));
      }
    }
}

#if defined(GC_ASSERTIONS)
    /* Check that all thread-local free-lists are completely marked.	*/
    /* also check that thread-specific-data structures are marked.	*/
    void GC_check_tls(void) {
	int i;
	GC_thread p;
	
	for (i = 0; i < THREAD_TABLE_SZ; ++i) {
	  for (p = GC_threads[i]; 0 != p; p = p -> next) {
	    GC_check_tls_for(&(p->tlfs));
	  }
	}
#       if defined(USE_CUSTOM_SPECIFIC)
	  if (GC_thread_key != 0)
	    GC_check_tsd_marks(GC_thread_key);
#	endif 
    }
#endif /* GC_ASSERTIONS */

#endif /* THREAD_LOCAL_ALLOC ... */

#endif /* GC_WIN32_THREADS */
