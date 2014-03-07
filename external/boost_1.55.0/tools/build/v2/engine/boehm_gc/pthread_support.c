/* 
 * Copyright (c) 1994 by Xerox Corporation.  All rights reserved.
 * Copyright (c) 1996 by Silicon Graphics.  All rights reserved.
 * Copyright (c) 1998 by Fergus Henderson.  All rights reserved.
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
/*
 * Support code originally for LinuxThreads, the clone()-based kernel
 * thread package for Linux which is included in libc6.
 *
 * This code no doubt makes some assumptions beyond what is
 * guaranteed by the pthread standard, though it now does
 * very little of that.  It now also supports NPTL, and many
 * other Posix thread implementations.  We are trying to merge
 * all flavors of pthread dupport code into this file.
 */
 /* DG/UX ix86 support <takis@xfree86.org> */
/*
 * Linux_threads.c now also includes some code to support HPUX and
 * OSF1 (Compaq Tru64 Unix, really).  The OSF1 support is based on Eric Benson's
 * patch.
 *
 * Eric also suggested an alternate basis for a lock implementation in
 * his code:
 * + #elif defined(OSF1)
 * +    unsigned long GC_allocate_lock = 0;
 * +    msemaphore GC_allocate_semaphore;
 * + #  define GC_TRY_LOCK() \
 * +    ((msem_lock(&GC_allocate_semaphore, MSEM_IF_NOWAIT) == 0) \
 * +     ? (GC_allocate_lock = 1) \
 * +     : 0)
 * + #  define GC_LOCK_TAKEN GC_allocate_lock
 */

/*#define DEBUG_THREADS 1*/

# include "private/pthread_support.h"

# if defined(GC_PTHREADS) && !defined(GC_WIN32_THREADS)

# if defined(GC_DGUX386_THREADS) && !defined(_POSIX4A_DRAFT10_SOURCE)
#   define _POSIX4A_DRAFT10_SOURCE 1
# endif

# if defined(GC_DGUX386_THREADS) && !defined(_USING_POSIX4A_DRAFT10)
#   define _USING_POSIX4A_DRAFT10 1
# endif

# include <stdlib.h>
# include <pthread.h>
# include <sched.h>
# include <time.h>
# include <errno.h>
# include <unistd.h>
# include <sys/mman.h>
# include <sys/time.h>
# include <sys/types.h>
# include <sys/stat.h>
# include <fcntl.h>
# include <signal.h>

# include "gc_inline.h"

#if defined(GC_DARWIN_THREADS)
# include "private/darwin_semaphore.h"
#else
# include <semaphore.h>
#endif /* !GC_DARWIN_THREADS */

#if defined(GC_DARWIN_THREADS) || defined(GC_FREEBSD_THREADS)
# include <sys/sysctl.h>
#endif /* GC_DARWIN_THREADS */

#if defined(GC_NETBSD_THREADS)
# include <sys/param.h>
# include <sys/sysctl.h>
#endif        /* GC_NETBSD_THREADS */

/* Allocator lock definitions.		*/
#if !defined(USE_SPIN_LOCK)
  pthread_mutex_t GC_allocate_ml = PTHREAD_MUTEX_INITIALIZER;
#endif
unsigned long GC_lock_holder = NO_THREAD;
		/* Used only for assertions, and to prevent	 */
		/* recursive reentry in the system call wrapper. */

#if defined(GC_DGUX386_THREADS)
# include <sys/dg_sys_info.h>
# include <sys/_int_psem.h>
  /* sem_t is an uint in DG/UX */
  typedef unsigned int  sem_t;
#endif /* GC_DGUX386_THREADS */

#ifndef __GNUC__
#   define __inline__
#endif

/* Undefine macros used to redirect pthread primitives. */
# undef pthread_create
# if !defined(GC_DARWIN_THREADS)
#   undef pthread_sigmask
# endif
# undef pthread_join
# undef pthread_detach
# if defined(GC_OSF1_THREADS) && defined(_PTHREAD_USE_MANGLED_NAMES_) \
     && !defined(_PTHREAD_USE_PTDNAM_)
  /* Restore the original mangled names on Tru64 UNIX.  */
#   define pthread_create __pthread_create
#   define pthread_join __pthread_join
#   define pthread_detach __pthread_detach
# endif

#ifdef GC_USE_LD_WRAP
#   define WRAP_FUNC(f) __wrap_##f
#   define REAL_FUNC(f) __real_##f
#else
#   ifdef GC_USE_DLOPEN_WRAP
#     include <dlfcn.h>
#     define WRAP_FUNC(f) f
#     define REAL_FUNC(f) GC_real_##f
      /* We define both GC_f and plain f to be the wrapped function.	*/
      /* In that way plain calls work, as do calls from files that	*/
      /* included gc.h, wich redefined f to GC_f.			*/
      /* FIXME: Needs work for DARWIN and True64 (OSF1) */
      typedef int (* GC_pthread_create_t)(pthread_t *, const pthread_attr_t *,
		      		          void * (*)(void *), void *);
      static GC_pthread_create_t GC_real_pthread_create;
      typedef int (* GC_pthread_sigmask_t)(int, const sigset_t *, sigset_t *);
      static GC_pthread_sigmask_t GC_real_pthread_sigmask;
      typedef int (* GC_pthread_join_t)(pthread_t, void **);
      static GC_pthread_join_t GC_real_pthread_join;
      typedef int (* GC_pthread_detach_t)(pthread_t);
      static GC_pthread_detach_t GC_real_pthread_detach;
#   else
#     define WRAP_FUNC(f) GC_##f
#     if !defined(GC_DGUX386_THREADS)
#       define REAL_FUNC(f) f
#     else /* GC_DGUX386_THREADS */
#       define REAL_FUNC(f) __d10_##f
#     endif /* GC_DGUX386_THREADS */
#   endif
#endif

#if defined(GC_USE_DL_WRAP) || defined(GC_USE_DLOPEN_WRAP)
/* Define GC_ functions as aliases for the plain ones, which will	*/
/* be intercepted.  This allows files which include gc.h, and hence	*/
/* generate referemces to the GC_ symbols, to see the right symbols.	*/
      int GC_pthread_create(pthread_t * t, const pthread_attr_t * a,
		         void * (* fn)(void *), void * arg) {
	  return pthread_create(t, a, fn, arg);
      }
      int GC_pthread_sigmask(int how, const sigset_t *mask, sigset_t *old) {
	  return pthread_sigmask(how, mask, old);
      }
      int GC_pthread_join(pthread_t t, void **res) {
	  return pthread_join(t, res);
      }
      int GC_pthread_detach(pthread_t t) {
	  return pthread_detach(t);
      }
#endif /* Linker-based interception. */

#ifdef GC_USE_DLOPEN_WRAP
  static GC_bool GC_syms_initialized = FALSE;

  void GC_init_real_syms(void)
  {
    void *dl_handle;
#   define LIBPTHREAD_NAME "libpthread.so.0"
#   define LIBPTHREAD_NAME_LEN 16 /* incl. trailing 0 */
    size_t len = LIBPTHREAD_NAME_LEN - 1;
    char namebuf[LIBPTHREAD_NAME_LEN];
    static char *libpthread_name = LIBPTHREAD_NAME;

    if (GC_syms_initialized) return;
#   ifdef RTLD_NEXT
      dl_handle = RTLD_NEXT;
#   else
      dl_handle = dlopen(libpthread_name, RTLD_LAZY);
      if (NULL == dl_handle) {
        while (isdigit(libpthread_name[len-1])) --len;
        if (libpthread_name[len-1] == '.') --len;
        memcpy(namebuf, libpthread_name, len);
        namebuf[len] = '\0';
        dl_handle = dlopen(namebuf, RTLD_LAZY);
      }
      if (NULL == dl_handle) ABORT("Couldn't open libpthread\n");
#   endif
    GC_real_pthread_create = (GC_pthread_create_t)
	    			dlsym(dl_handle, "pthread_create");
    GC_real_pthread_sigmask = (GC_pthread_sigmask_t)
	    			dlsym(dl_handle, "pthread_sigmask");
    GC_real_pthread_join = (GC_pthread_join_t)
	    			dlsym(dl_handle, "pthread_join");
    GC_real_pthread_detach = (GC_pthread_detach_t)
	    			dlsym(dl_handle, "pthread_detach");
    GC_syms_initialized = TRUE;
  }

# define INIT_REAL_SYMS() if (!GC_syms_initialized) GC_init_real_syms();
#else
# define INIT_REAL_SYMS()
#endif

void GC_thr_init(void);

static GC_bool parallel_initialized = FALSE;

GC_bool GC_need_to_lock = FALSE;

void GC_init_parallel(void);

long GC_nprocs = 1;	/* Number of processors.  We may not have	*/
			/* access to all of them, but this is as good	*/
			/* a guess as any ...				*/

#ifdef THREAD_LOCAL_ALLOC
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

#endif /* Thread_local_alloc */

#ifdef PARALLEL_MARK

# ifndef MAX_MARKERS
#   define MAX_MARKERS 16
# endif

static ptr_t marker_sp[MAX_MARKERS] = {0};
#ifdef IA64
  static ptr_t marker_bsp[MAX_MARKERS] = {0};
#endif

void * GC_mark_thread(void * id)
{
  word my_mark_no = 0;

  marker_sp[(word)id] = GC_approx_sp();
# ifdef IA64
    marker_bsp[(word)id] = GC_save_regs_in_stack();
# endif
  for (;; ++my_mark_no) {
    /* GC_mark_no is passed only to allow GC_help_marker to terminate	*/
    /* promptly.  This is important if it were called from the signal	*/
    /* handler or from the GC lock acquisition code.  Under Linux, it's	*/
    /* not safe to call it from a signal handler, since it uses mutexes	*/
    /* and condition variables.  Since it is called only here, the 	*/
    /* argument is unnecessary.						*/
    if (my_mark_no < GC_mark_no || my_mark_no > GC_mark_no + 2) {
	/* resynchronize if we get far off, e.g. because GC_mark_no	*/
	/* wrapped.							*/
	my_mark_no = GC_mark_no;
    }
#   ifdef DEBUG_THREADS
	GC_printf("Starting mark helper for mark number %lu\n", my_mark_no);
#   endif
    GC_help_marker(my_mark_no);
  }
}

extern long GC_markers;		/* Number of mark threads we would	*/
				/* like to have.  Includes the 		*/
				/* initiating thread.			*/

pthread_t GC_mark_threads[MAX_MARKERS];

#define PTHREAD_CREATE REAL_FUNC(pthread_create)

static void start_mark_threads(void)
{
    unsigned i;
    pthread_attr_t attr;

    if (GC_markers > MAX_MARKERS) {
	WARN("Limiting number of mark threads\n", 0);
	GC_markers = MAX_MARKERS;
    }
    if (0 != pthread_attr_init(&attr)) ABORT("pthread_attr_init failed");
	
    if (0 != pthread_attr_setdetachstate(&attr, PTHREAD_CREATE_DETACHED))
	ABORT("pthread_attr_setdetachstate failed");

#   if defined(HPUX) || defined(GC_DGUX386_THREADS)
      /* Default stack size is usually too small: fix it. */
      /* Otherwise marker threads or GC may run out of	  */
      /* space.						  */
#     define MIN_STACK_SIZE (8*HBLKSIZE*sizeof(word))
      {
	size_t old_size;
	int code;

        if (pthread_attr_getstacksize(&attr, &old_size) != 0)
	  ABORT("pthread_attr_getstacksize failed\n");
	if (old_size < MIN_STACK_SIZE) {
	  if (pthread_attr_setstacksize(&attr, MIN_STACK_SIZE) != 0)
		  ABORT("pthread_attr_setstacksize failed\n");
	}
      }
#   endif /* HPUX || GC_DGUX386_THREADS */
    if (GC_print_stats) {
	GC_log_printf("Starting %ld marker threads\n", GC_markers - 1);
    }
    for (i = 0; i < GC_markers - 1; ++i) {
      if (0 != PTHREAD_CREATE(GC_mark_threads + i, &attr,
			      GC_mark_thread, (void *)(word)i)) {
	WARN("Marker thread creation failed, errno = %ld.\n", errno);
      }
    }
}

#endif /* PARALLEL_MARK */

GC_bool GC_thr_initialized = FALSE;

volatile GC_thread GC_threads[THREAD_TABLE_SZ];

void GC_push_thread_structures(void)
{
    GC_ASSERT(I_HOLD_LOCK());
    GC_push_all((ptr_t)(GC_threads), (ptr_t)(GC_threads)+sizeof(GC_threads));
#   if defined(THREAD_LOCAL_ALLOC)
      GC_push_all((ptr_t)(&GC_thread_key),
	  (ptr_t)(&GC_thread_key)+sizeof(&GC_thread_key));
#   endif
}

/* It may not be safe to allocate when we register the first thread.	*/
static struct GC_Thread_Rep first_thread;

/* Add a thread to GC_threads.  We assume it wasn't already there.	*/
/* Caller holds allocation lock.					*/
GC_thread GC_new_thread(pthread_t id)
{
    int hv = NUMERIC_THREAD_ID(id) % THREAD_TABLE_SZ;
    GC_thread result;
    static GC_bool first_thread_used = FALSE;
    
    GC_ASSERT(I_HOLD_LOCK());
    if (!first_thread_used) {
    	result = &first_thread;
    	first_thread_used = TRUE;
    } else {
        result = (struct GC_Thread_Rep *)
        	 GC_INTERNAL_MALLOC(sizeof(struct GC_Thread_Rep), NORMAL);
	GC_ASSERT(result -> flags == 0);
    }
    if (result == 0) return(0);
    result -> id = id;
    result -> next = GC_threads[hv];
    GC_threads[hv] = result;
    GC_ASSERT(result -> flags == 0 && result -> thread_blocked == 0);
    return(result);
}

/* Delete a thread from GC_threads.  We assume it is there.	*/
/* (The code intentionally traps if it wasn't.)			*/
void GC_delete_thread(pthread_t id)
{
    int hv = NUMERIC_THREAD_ID(id) % THREAD_TABLE_SZ;
    register GC_thread p = GC_threads[hv];
    register GC_thread prev = 0;
    
    GC_ASSERT(I_HOLD_LOCK());
    while (!THREAD_EQUAL(p -> id, id)) {
        prev = p;
        p = p -> next;
    }
    if (prev == 0) {
        GC_threads[hv] = p -> next;
    } else {
        prev -> next = p -> next;
    }
#   ifdef GC_DARWIN_THREADS
	mach_port_deallocate(mach_task_self(), p->stop_info.mach_thread);
#   endif
    GC_INTERNAL_FREE(p);
}

/* If a thread has been joined, but we have not yet		*/
/* been notified, then there may be more than one thread 	*/
/* in the table with the same pthread id.			*/
/* This is OK, but we need a way to delete a specific one.	*/
void GC_delete_gc_thread(GC_thread gc_id)
{
    pthread_t id = gc_id -> id;
    int hv = NUMERIC_THREAD_ID(id) % THREAD_TABLE_SZ;
    register GC_thread p = GC_threads[hv];
    register GC_thread prev = 0;

    GC_ASSERT(I_HOLD_LOCK());
    while (p != gc_id) {
        prev = p;
        p = p -> next;
    }
    if (prev == 0) {
        GC_threads[hv] = p -> next;
    } else {
        prev -> next = p -> next;
    }
#   ifdef GC_DARWIN_THREADS
	mach_port_deallocate(mach_task_self(), p->stop_info.mach_thread);
#   endif
    GC_INTERNAL_FREE(p);
}

/* Return a GC_thread corresponding to a given pthread_t.	*/
/* Returns 0 if it's not there.					*/
/* Caller holds  allocation lock or otherwise inhibits 		*/
/* updates.							*/
/* If there is more than one thread with the given id we 	*/
/* return the most recent one.					*/
GC_thread GC_lookup_thread(pthread_t id)
{
    int hv = NUMERIC_THREAD_ID(id) % THREAD_TABLE_SZ;
    register GC_thread p = GC_threads[hv];
    
    while (p != 0 && !THREAD_EQUAL(p -> id, id)) p = p -> next;
    return(p);
}

#ifdef HANDLE_FORK
/* Remove all entries from the GC_threads table, except the	*/
/* one for the current thread.  We need to do this in the child	*/
/* process after a fork(), since only the current thread 	*/
/* survives in the child.					*/
void GC_remove_all_threads_but_me(void)
{
    pthread_t self = pthread_self();
    int hv;
    GC_thread p, next, me;

    for (hv = 0; hv < THREAD_TABLE_SZ; ++hv) {
      me = 0;
      for (p = GC_threads[hv]; 0 != p; p = next) {
	next = p -> next;
	if (THREAD_EQUAL(p -> id, self)) {
	  me = p;
	  p -> next = 0;
	} else {
#	  ifdef THREAD_LOCAL_ALLOC
	    if (!(p -> flags & FINISHED)) {
	      GC_destroy_thread_local(&(p->tlfs));
	    }
#	  endif /* THREAD_LOCAL_ALLOC */
	  if (p != &first_thread) GC_INTERNAL_FREE(p);
	}
      }
      GC_threads[hv] = me;
    }
}
#endif /* HANDLE_FORK */

#ifdef USE_PROC_FOR_LIBRARIES
GC_bool GC_segment_is_thread_stack(ptr_t lo, ptr_t hi)
{
    int i;
    GC_thread p;
    
    GC_ASSERT(I_HOLD_LOCK());
#   ifdef PARALLEL_MARK
      for (i = 0; i < GC_markers; ++i) {
	if (marker_sp[i] > lo & marker_sp[i] < hi) return TRUE;
#       ifdef IA64
	  if (marker_bsp[i] > lo & marker_bsp[i] < hi) return TRUE;
#	endif
      }
#   endif
    for (i = 0; i < THREAD_TABLE_SZ; i++) {
      for (p = GC_threads[i]; p != 0; p = p -> next) {
	if (0 != p -> stack_end) {
#	  ifdef STACK_GROWS_UP
            if (p -> stack_end >= lo && p -> stack_end < hi) return TRUE;
#	  else /* STACK_GROWS_DOWN */
            if (p -> stack_end > lo && p -> stack_end <= hi) return TRUE;
#	  endif
	}
      }
    }
    return FALSE;
}
#endif /* USE_PROC_FOR_LIBRARIES */

#ifdef IA64
/* Find the largest stack_base smaller than bound.  May be used	*/
/* to find the boundary between a register stack and adjacent	*/
/* immediately preceding memory stack.				*/
ptr_t GC_greatest_stack_base_below(ptr_t bound)
{
    int i;
    GC_thread p;
    ptr_t result = 0;
    
    GC_ASSERT(I_HOLD_LOCK());
#   ifdef PARALLEL_MARK
      for (i = 0; i < GC_markers; ++i) {
	if (marker_sp[i] > result && marker_sp[i] < bound)
	  result = marker_sp[i];
      }
#   endif
    for (i = 0; i < THREAD_TABLE_SZ; i++) {
      for (p = GC_threads[i]; p != 0; p = p -> next) {
	if (p -> stack_end > result && p -> stack_end < bound) {
	  result = p -> stack_end;
	}
      }
    }
    return result;
}
#endif /* IA64 */

#ifdef GC_LINUX_THREADS
/* Return the number of processors, or i<= 0 if it can't be determined.	*/
int GC_get_nprocs(void)
{
    /* Should be "return sysconf(_SC_NPROCESSORS_ONLN);" but that	*/
    /* appears to be buggy in many cases.				*/
    /* We look for lines "cpu<n>" in /proc/stat.			*/
#   define STAT_BUF_SIZE 4096
#   define STAT_READ read
	/* If read is wrapped, this may need to be redefined to call 	*/
	/* the real one.						*/
    char stat_buf[STAT_BUF_SIZE];
    int f;
    word result = 1;
	/* Some old kernels only have a single "cpu nnnn ..."	*/
	/* entry in /proc/stat.  We identify those as 		*/
	/* uniprocessors.					*/
    size_t i, len = 0;

    f = open("/proc/stat", O_RDONLY);
    if (f < 0 || (len = STAT_READ(f, stat_buf, STAT_BUF_SIZE)) < 100) {
	WARN("Couldn't read /proc/stat\n", 0);
	return -1;
    }
    for (i = 0; i < len - 100; ++i) {
        if (stat_buf[i] == '\n' && stat_buf[i+1] == 'c'
	    && stat_buf[i+2] == 'p' && stat_buf[i+3] == 'u') {
	    int cpu_no = atoi(stat_buf + i + 4);
	    if (cpu_no >= result) result = cpu_no + 1;
	}
    }
    close(f);
    return result;
}
#endif /* GC_LINUX_THREADS */

/* We hold the GC lock.  Wait until an in-progress GC has finished.	*/
/* Repeatedly RELEASES GC LOCK in order to wait.			*/
/* If wait_for_all is true, then we exit with the GC lock held and no	*/
/* collection in progress; otherwise we just wait for the current GC	*/
/* to finish.								*/
extern GC_bool GC_collection_in_progress(void);
void GC_wait_for_gc_completion(GC_bool wait_for_all)
{
    GC_ASSERT(I_HOLD_LOCK());
    if (GC_incremental && GC_collection_in_progress()) {
	int old_gc_no = GC_gc_no;

	/* Make sure that no part of our stack is still on the mark stack, */
	/* since it's about to be unmapped.				   */
	while (GC_incremental && GC_collection_in_progress()
	       && (wait_for_all || old_gc_no == GC_gc_no)) {
	    ENTER_GC();
	    GC_in_thread_creation = TRUE;
            GC_collect_a_little_inner(1);
	    GC_in_thread_creation = FALSE;
	    EXIT_GC();
	    UNLOCK();
	    sched_yield();
	    LOCK();
	}
    }
}

#ifdef HANDLE_FORK
/* Procedures called before and after a fork.  The goal here is to make */
/* it safe to call GC_malloc() in a forked child.  It's unclear that is	*/
/* attainable, since the single UNIX spec seems to imply that one 	*/
/* should only call async-signal-safe functions, and we probably can't	*/
/* quite guarantee that.  But we give it our best shot.  (That same	*/
/* spec also implies that it's not safe to call the system malloc	*/
/* between fork() and exec().  Thus we're doing no worse than it.	*/

/* Called before a fork()		*/
void GC_fork_prepare_proc(void)
{
    /* Acquire all relevant locks, so that after releasing the locks	*/
    /* the child will see a consistent state in which monitor 		*/
    /* invariants hold.	 Unfortunately, we can't acquire libc locks	*/
    /* we might need, and there seems to be no guarantee that libc	*/
    /* must install a suitable fork handler.				*/
    /* Wait for an ongoing GC to finish, since we can't finish it in	*/
    /* the (one remaining thread in) the child.				*/
      LOCK();
#     if defined(PARALLEL_MARK) || defined(THREAD_LOCAL_ALLOC)
        GC_wait_for_reclaim();
#     endif
      GC_wait_for_gc_completion(TRUE);
#     if defined(PARALLEL_MARK) || defined(THREAD_LOCAL_ALLOC)
        GC_acquire_mark_lock();
#     endif
}

/* Called in parent after a fork()	*/
void GC_fork_parent_proc(void)
{
#   if defined(PARALLEL_MARK) || defined(THREAD_LOCAL_ALLOC)
      GC_release_mark_lock();
#   endif
    UNLOCK();
}

/* Called in child after a fork()	*/
void GC_fork_child_proc(void)
{
    /* Clean up the thread table, so that just our thread is left. */
#   if defined(PARALLEL_MARK) || defined(THREAD_LOCAL_ALLOC)
      GC_release_mark_lock();
#   endif
    GC_remove_all_threads_but_me();
#   ifdef PARALLEL_MARK
      /* Turn off parallel marking in the child, since we are probably 	*/
      /* just going to exec, and we would have to restart mark threads.	*/
        GC_markers = 1;
        GC_parallel = FALSE;
#   endif /* PARALLEL_MARK */
    UNLOCK();
}
#endif /* HANDLE_FORK */

#if defined(GC_DGUX386_THREADS)
/* Return the number of processors, or i<= 0 if it can't be determined. */
int GC_get_nprocs(void)
{
    /* <takis@XFree86.Org> */
    int numCpus;
    struct dg_sys_info_pm_info pm_sysinfo;
    int status =0;

    status = dg_sys_info((long int *) &pm_sysinfo,
	DG_SYS_INFO_PM_INFO_TYPE, DG_SYS_INFO_PM_CURRENT_VERSION);
    if (status < 0)
       /* set -1 for error */
       numCpus = -1;
    else
      /* Active CPUs */
      numCpus = pm_sysinfo.idle_vp_count;

#  ifdef DEBUG_THREADS
    GC_printf("Number of active CPUs in this system: %d\n", numCpus);
#  endif
    return(numCpus);
}
#endif /* GC_DGUX386_THREADS */

#if defined(GC_NETBSD_THREADS)
static int get_ncpu(void)
{
    int mib[] = {CTL_HW,HW_NCPU};
    int res;
    size_t len = sizeof(res);

    sysctl(mib, sizeof(mib)/sizeof(int), &res, &len, NULL, 0);
    return res;
}
#endif	/* GC_NETBSD_THREADS */

# if defined(GC_LINUX_THREADS) && defined(INCLUDE_LINUX_THREAD_DESCR)
__thread int dummy_thread_local;
# endif

/* We hold the allocation lock.	*/
void GC_thr_init(void)
{
#   ifndef GC_DARWIN_THREADS
        int dummy;
#   endif
    GC_thread t;

    if (GC_thr_initialized) return;
    GC_thr_initialized = TRUE;
    
#   ifdef HANDLE_FORK
      /* Prepare for a possible fork.	*/
        pthread_atfork(GC_fork_prepare_proc, GC_fork_parent_proc,
	  	       GC_fork_child_proc);
#   endif /* HANDLE_FORK */
#   if defined(INCLUDE_LINUX_THREAD_DESCR)
      /* Explicitly register the region including the address 		*/
      /* of a thread local variable.  This should included thread	*/
      /* locals for the main thread, except for those allocated		*/
      /* in response to dlopen calls.					*/  
	{
	  ptr_t thread_local_addr = (ptr_t)(&dummy_thread_local);
	  ptr_t main_thread_start, main_thread_end;
          if (!GC_enclosing_mapping(thread_local_addr, &main_thread_start,
				    &main_thread_end)) {
	    ABORT("Failed to find mapping for main thread thread locals");
	  }
	  GC_add_roots_inner(main_thread_start, main_thread_end, FALSE);
	}
#   endif
    /* Add the initial thread, so we can stop it.	*/
      t = GC_new_thread(pthread_self());
#     ifdef GC_DARWIN_THREADS
         t -> stop_info.mach_thread = mach_thread_self();
#     else
         t -> stop_info.stack_ptr = (ptr_t)(&dummy);
#     endif
      t -> flags = DETACHED | MAIN_THREAD;

    GC_stop_init();

    /* Set GC_nprocs.  */
      {
	char * nprocs_string = GETENV("GC_NPROCS");
	GC_nprocs = -1;
	if (nprocs_string != NULL) GC_nprocs = atoi(nprocs_string);
      }
      if (GC_nprocs <= 0) {
#       if defined(GC_HPUX_THREADS)
	  GC_nprocs = pthread_num_processors_np();
#       endif
#	if defined(GC_OSF1_THREADS) || defined(GC_AIX_THREADS) \
	   || defined(GC_SOLARIS_THREADS)
	  GC_nprocs = sysconf(_SC_NPROCESSORS_ONLN);
	  if (GC_nprocs <= 0) GC_nprocs = 1;
#	endif
#       if defined(GC_IRIX_THREADS)
	  GC_nprocs = sysconf(_SC_NPROC_ONLN);
	  if (GC_nprocs <= 0) GC_nprocs = 1;
#       endif
#       if defined(GC_NETBSD_THREADS)
	  GC_nprocs = get_ncpu();
#       endif
#       if defined(GC_DARWIN_THREADS) || defined(GC_FREEBSD_THREADS)
	  int ncpus = 1;
	  size_t len = sizeof(ncpus);
	  sysctl((int[2]) {CTL_HW, HW_NCPU}, 2, &ncpus, &len, NULL, 0);
	  GC_nprocs = ncpus;
#       endif
#	if defined(GC_LINUX_THREADS) || defined(GC_DGUX386_THREADS)
          GC_nprocs = GC_get_nprocs();
#	endif
#       if defined(GC_GNU_THREADS)
	  if (GC_nprocs <= 0) GC_nprocs = 1;
#       endif
      }
      if (GC_nprocs <= 0) {
	WARN("GC_get_nprocs() returned %ld\n", GC_nprocs);
	GC_nprocs = 2;
#	ifdef PARALLEL_MARK
	  GC_markers = 1;
#	endif
      } else {
#	ifdef PARALLEL_MARK
          {
	    char * markers_string = GETENV("GC_MARKERS");
	    if (markers_string != NULL) {
	      GC_markers = atoi(markers_string);
	    } else {
	      GC_markers = GC_nprocs;
	    }
          }
#	endif
      }
#   ifdef PARALLEL_MARK
      if (GC_print_stats) {
          GC_log_printf("Number of processors = %ld, "
		 "number of marker threads = %ld\n", GC_nprocs, GC_markers);
      }
      if (GC_markers == 1) {
	GC_parallel = FALSE;
	if (GC_print_stats) {
	    GC_log_printf(
		"Single marker thread, turning off parallel marking\n");
	}
      } else {
	GC_parallel = TRUE;
	/* Disable true incremental collection, but generational is OK.	*/
	GC_time_limit = GC_TIME_UNLIMITED;
      }
      /* If we are using a parallel marker, actually start helper threads.  */
        if (GC_parallel) start_mark_threads();
#   endif
}


/* Perform all initializations, including those that	*/
/* may require allocation.				*/
/* Called without allocation lock.			*/
/* Must be called before a second thread is created.	*/
/* Did we say it's called without the allocation lock?	*/
void GC_init_parallel(void)
{
    if (parallel_initialized) return;
    parallel_initialized = TRUE;

    /* GC_init() calls us back, so set flag first.	*/
    if (!GC_is_initialized) GC_init();
    /* Initialize thread local free lists if used.	*/
#   if defined(THREAD_LOCAL_ALLOC)
      LOCK();
      GC_init_thread_local(&(GC_lookup_thread(pthread_self())->tlfs));
      UNLOCK();
#   endif
}


#if !defined(GC_DARWIN_THREADS)
int WRAP_FUNC(pthread_sigmask)(int how, const sigset_t *set, sigset_t *oset)
{
    sigset_t fudged_set;
    
    INIT_REAL_SYMS();
    if (set != NULL && (how == SIG_BLOCK || how == SIG_SETMASK)) {
        fudged_set = *set;
        sigdelset(&fudged_set, SIG_SUSPEND);
        set = &fudged_set;
    }
    return(REAL_FUNC(pthread_sigmask)(how, set, oset));
}
#endif /* !GC_DARWIN_THREADS */

/* Wrapper for functions that are likely to block for an appreciable	*/
/* length of time.							*/

struct blocking_data {
    void (*fn)(void *);
    void *arg;
};

static void GC_do_blocking_inner(ptr_t data, void * context) {
    struct blocking_data * d = (struct blocking_data *) data;
    GC_thread me;
    LOCK();
    me = GC_lookup_thread(pthread_self());
    GC_ASSERT(!(me -> thread_blocked));
#   ifdef SPARC
	me -> stop_info.stack_ptr = GC_save_regs_in_stack();
#   elif !defined(GC_DARWIN_THREADS)
	me -> stop_info.stack_ptr = GC_approx_sp();
#   endif
#   ifdef IA64
	me -> backing_store_ptr = GC_save_regs_in_stack();
#   endif
    me -> thread_blocked = TRUE;
    /* Save context here if we want to support precise stack marking */
    UNLOCK();
    (d -> fn)(d -> arg);
    LOCK();   /* This will block if the world is stopped.	*/
    me -> thread_blocked = FALSE;
    UNLOCK();
}

void GC_do_blocking(void (*fn)(void *), void *arg) {
    struct blocking_data my_data;

    my_data.fn = fn;
    my_data.arg = arg;
    GC_with_callee_saves_pushed(GC_do_blocking_inner, (ptr_t)(&my_data));
}
    
struct start_info {
    void *(*start_routine)(void *);
    void *arg;
    word flags;
    sem_t registered;   	/* 1 ==> in our thread table, but 	*/
				/* parent hasn't yet noticed.		*/
};

int GC_unregister_my_thread(void)
{
    GC_thread me;

    LOCK();
    /* Wait for any GC that may be marking from our stack to	*/
    /* complete before we remove this thread.			*/
    GC_wait_for_gc_completion(FALSE);
    me = GC_lookup_thread(pthread_self());
#   if defined(THREAD_LOCAL_ALLOC)
      GC_destroy_thread_local(&(me->tlfs));
#   endif
    if (me -> flags & DETACHED) {
    	GC_delete_thread(pthread_self());
    } else {
	me -> flags |= FINISHED;
    }
#   if defined(THREAD_LOCAL_ALLOC)
      GC_remove_specific(GC_thread_key);
#   endif
    UNLOCK();
    return GC_SUCCESS;
}

/* Called at thread exit.				*/
/* Never called for main thread.  That's OK, since it	*/
/* results in at most a tiny one-time leak.  And 	*/
/* linuxthreads doesn't reclaim the main threads 	*/
/* resources or id anyway.				*/
void GC_thread_exit_proc(void *arg)
{
    GC_unregister_my_thread();
}

int WRAP_FUNC(pthread_join)(pthread_t thread, void **retval)
{
    int result;
    GC_thread thread_gc_id;
    
    INIT_REAL_SYMS();
    LOCK();
    thread_gc_id = GC_lookup_thread(thread);
    /* This is guaranteed to be the intended one, since the thread id	*/
    /* cant have been recycled by pthreads.				*/
    UNLOCK();
    result = REAL_FUNC(pthread_join)(thread, retval);
# if defined (GC_FREEBSD_THREADS)
    /* On FreeBSD, the wrapped pthread_join() sometimes returns (what
       appears to be) a spurious EINTR which caused the test and real code
       to gratuitously fail.  Having looked at system pthread library source
       code, I see how this return code may be generated.  In one path of
       code, pthread_join() just returns the errno setting of the thread
       being joined.  This does not match the POSIX specification or the
       local man pages thus I have taken the liberty to catch this one
       spurious return value properly conditionalized on GC_FREEBSD_THREADS. */
    if (result == EINTR) result = 0;
# endif
    if (result == 0) {
        LOCK();
        /* Here the pthread thread id may have been recycled. */
        GC_delete_gc_thread(thread_gc_id);
        UNLOCK();
    }
    return result;
}

int
WRAP_FUNC(pthread_detach)(pthread_t thread)
{
    int result;
    GC_thread thread_gc_id;
    
    INIT_REAL_SYMS();
    LOCK();
    thread_gc_id = GC_lookup_thread(thread);
    UNLOCK();
    result = REAL_FUNC(pthread_detach)(thread);
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

GC_bool GC_in_thread_creation = FALSE;  /* Protected by allocation lock. */

GC_thread GC_register_my_thread_inner(struct GC_stack_base *sb,
				      pthread_t my_pthread)
{
    GC_thread me;

    GC_in_thread_creation = TRUE; /* OK to collect from unknown thread. */
    me = GC_new_thread(my_pthread);
    GC_in_thread_creation = FALSE;
#   ifdef GC_DARWIN_THREADS
      me -> stop_info.mach_thread = mach_thread_self();
#   else
      me -> stop_info.stack_ptr = sb -> mem_base;
#   endif
    me -> stack_end = sb -> mem_base;
#   ifdef IA64
      me -> backing_store_end = sb -> reg_base;
#   endif /* IA64 */
    return me;
}

int GC_register_my_thread(struct GC_stack_base *sb)
{
    pthread_t my_pthread = pthread_self();
    GC_thread me;

    LOCK();
    me = GC_lookup_thread(my_pthread);
    if (0 == me) {
        me = GC_register_my_thread_inner(sb, my_pthread);
	me -> flags |= DETACHED;
    	  /* Treat as detached, since we do not need to worry about	*/
    	  /* pointer results.						*/
	UNLOCK();
        return GC_SUCCESS;
    } else {
	UNLOCK();
	return GC_DUPLICATE;
    }
}

void * GC_inner_start_routine(struct GC_stack_base *sb, void * arg)
{
    struct start_info * si = arg;
    void * result;
    GC_thread me;
    pthread_t my_pthread;
    void *(*start)(void *);
    void *start_arg;

    my_pthread = pthread_self();
#   ifdef DEBUG_THREADS
        GC_printf("Starting thread 0x%x\n", (unsigned)my_pthread);
        GC_printf("pid = %ld\n", (long) getpid());
        GC_printf("sp = 0x%lx\n", (long) &arg);
#   endif
    LOCK();
    me = GC_register_my_thread_inner(sb, my_pthread);
    me -> flags = si -> flags;
    UNLOCK();
    start = si -> start_routine;
#   ifdef DEBUG_THREADS
	GC_printf("start_routine = %p\n", (void *)start);
#   endif
    start_arg = si -> arg;
    sem_post(&(si -> registered));	/* Last action on si.	*/
    					/* OK to deallocate.	*/
    pthread_cleanup_push(GC_thread_exit_proc, 0);
#   if defined(THREAD_LOCAL_ALLOC)
 	LOCK();
        GC_init_thread_local(&(me->tlfs));
	UNLOCK();
#   endif
    result = (*start)(start_arg);
#   if DEBUG_THREADS
        GC_printf("Finishing thread 0x%x\n", (unsigned)pthread_self());
#   endif
    me -> status = result;
    pthread_cleanup_pop(1);
    /* Cleanup acquires lock, ensuring that we can't exit		*/
    /* while a collection that thinks we're alive is trying to stop     */
    /* us.								*/
    return(result);
}

void * GC_start_routine(void * arg)
{
#   ifdef INCLUDE_LINUX_THREAD_DESCR
      struct GC_stack_base sb;

#     ifdef REDIRECT_MALLOC
      	/* GC_get_stack_base may call pthread_getattr_np, which can 	*/
        /* unfortunately call realloc, which may allocate from an	*/
        /* unregistered thread.  This is unpleasant, since it might	*/ 
        /* force heap growth.						*/
        GC_disable();
#     endif
      if (GC_get_stack_base(&sb) != GC_SUCCESS)
	ABORT("Failed to get thread stack base.");
#     ifdef REDIRECT_MALLOC
        GC_enable();
#     endif
      return GC_inner_start_routine(&sb, arg);
#   else
      return GC_call_with_stack_base(GC_inner_start_routine, arg);
#   endif
}

int
WRAP_FUNC(pthread_create)(pthread_t *new_thread,
		  const pthread_attr_t *attr,
                  void *(*start_routine)(void *), void *arg)
{
    int result;
    int detachstate;
    word my_flags = 0;
    struct start_info * si; 
	/* This is otherwise saved only in an area mmapped by the thread */
	/* library, which isn't visible to the collector.		 */
 
    /* We resist the temptation to muck with the stack size here,	*/
    /* even if the default is unreasonably small.  That's the client's	*/
    /* responsibility.							*/

    INIT_REAL_SYMS();
    LOCK();
    si = (struct start_info *)GC_INTERNAL_MALLOC(sizeof(struct start_info),
						 NORMAL);
    UNLOCK();
    if (!parallel_initialized) GC_init_parallel();
    if (0 == si) return(ENOMEM);
    sem_init(&(si -> registered), 0, 0);
    si -> start_routine = start_routine;
    si -> arg = arg;
    LOCK();
    if (!GC_thr_initialized) GC_thr_init();
#   ifdef GC_ASSERTIONS
      {
	size_t stack_size = 0;
	if (NULL != attr) {
	   pthread_attr_getstacksize(attr, &stack_size);
	}
	if (0 == stack_size) {
	   pthread_attr_t my_attr;
	   pthread_attr_init(&my_attr);
	   pthread_attr_getstacksize(&my_attr, &stack_size);
	}
	/* On Solaris 10, with default attr initialization, 	*/
	/* stack_size remains 0.  Fudge it.			*/
	if (0 == stack_size) {
#	    ifndef SOLARIS
	      WARN("Failed to get stack size for assertion checking\n", 0);
#	    endif
	    stack_size = 1000000;
	}
#       ifdef PARALLEL_MARK
	  GC_ASSERT(stack_size >= (8*HBLKSIZE*sizeof(word)));
#       else
          /* FreeBSD-5.3/Alpha: default pthread stack is 64K, 	*/
	  /* HBLKSIZE=8192, sizeof(word)=8			*/
	  GC_ASSERT(stack_size >= 65536);
#       endif
	/* Our threads may need to do some work for the GC.	*/
	/* Ridiculously small threads won't work, and they	*/
	/* probably wouldn't work anyway.			*/
      }
#   endif
    if (NULL == attr) {
	detachstate = PTHREAD_CREATE_JOINABLE;
    } else { 
        pthread_attr_getdetachstate(attr, &detachstate);
    }
    if (PTHREAD_CREATE_DETACHED == detachstate) my_flags |= DETACHED;
    si -> flags = my_flags;
    UNLOCK();
#   ifdef DEBUG_THREADS
        GC_printf("About to start new thread from thread 0x%x\n",
		  (unsigned)pthread_self());
#   endif
    GC_need_to_lock = TRUE;

    result = REAL_FUNC(pthread_create)(new_thread, attr, GC_start_routine, si);

#   ifdef DEBUG_THREADS
        GC_printf("Started thread 0x%x\n", (unsigned)(*new_thread));
#   endif
    /* Wait until child has been added to the thread table.		*/
    /* This also ensures that we hold onto si until the child is done	*/
    /* with it.  Thus it doesn't matter whether it is otherwise		*/
    /* visible to the collector.					*/
    if (0 == result) {
	while (0 != sem_wait(&(si -> registered))) {
            if (EINTR != errno) ABORT("sem_wait failed");
	}
    }
    sem_destroy(&(si -> registered));
    LOCK();
    GC_INTERNAL_FREE(si);
    UNLOCK();

    return(result);
}

/* Spend a few cycles in a way that can't introduce contention with	*/
/* othre threads.							*/
void GC_pause(void)
{
    int i;
#   if !defined(__GNUC__) || defined(__INTEL_COMPILER)
      volatile word dummy = 0;
#   endif

    for (i = 0; i < 10; ++i) { 
#     if defined(__GNUC__) && !defined(__INTEL_COMPILER)
        __asm__ __volatile__ (" " : : : "memory");
#     else
	/* Something that's unlikely to be optimized away. */
	GC_noop(++dummy);
#     endif
    }
}
    
#define SPIN_MAX 128	/* Maximum number of calls to GC_pause before	*/
			/* give up.					*/

volatile GC_bool GC_collecting = 0;
			/* A hint that we're in the collector and       */
                        /* holding the allocation lock for an           */
                        /* extended period.                             */

#if !defined(USE_SPIN_LOCK) || defined(PARALLEL_MARK)
/* If we don't want to use the below spinlock implementation, either	*/
/* because we don't have a GC_test_and_set implementation, or because 	*/
/* we don't want to risk sleeping, we can still try spinning on 	*/
/* pthread_mutex_trylock for a while.  This appears to be very		*/
/* beneficial in many cases.						*/
/* I suspect that under high contention this is nearly always better	*/
/* than the spin lock.  But it's a bit slower on a uniprocessor.	*/
/* Hence we still default to the spin lock.				*/
/* This is also used to acquire the mark lock for the parallel		*/
/* marker.								*/

/* Here we use a strict exponential backoff scheme.  I don't know 	*/
/* whether that's better or worse than the above.  We eventually 	*/
/* yield by calling pthread_mutex_lock(); it never makes sense to	*/
/* explicitly sleep.							*/

#define LOCK_STATS
#ifdef LOCK_STATS
  unsigned long GC_spin_count = 0;
  unsigned long GC_block_count = 0;
  unsigned long GC_unlocked_count = 0;
#endif

void GC_generic_lock(pthread_mutex_t * lock)
{
#ifndef NO_PTHREAD_TRYLOCK
    unsigned pause_length = 1;
    unsigned i;
    
    if (0 == pthread_mutex_trylock(lock)) {
#       ifdef LOCK_STATS
	    ++GC_unlocked_count;
#       endif
	return;
    }
    for (; pause_length <= SPIN_MAX; pause_length <<= 1) {
	for (i = 0; i < pause_length; ++i) {
	    GC_pause();
	}
        switch(pthread_mutex_trylock(lock)) {
	    case 0:
#		ifdef LOCK_STATS
		    ++GC_spin_count;
#		endif
		return;
	    case EBUSY:
		break;
	    default:
		ABORT("Unexpected error from pthread_mutex_trylock");
        }
    }
#endif /* !NO_PTHREAD_TRYLOCK */
#   ifdef LOCK_STATS
	++GC_block_count;
#   endif
    pthread_mutex_lock(lock);
}

#endif /* !USE_SPIN_LOCK || PARALLEL_MARK */

#if defined(USE_SPIN_LOCK)

/* Reasonably fast spin locks.  Basically the same implementation */
/* as STL alloc.h.  This isn't really the right way to do this.   */
/* but until the POSIX scheduling mess gets straightened out ...  */

volatile AO_TS_t GC_allocate_lock = 0;


void GC_lock(void)
{
#   define low_spin_max 30  /* spin cycles if we suspect uniprocessor */
#   define high_spin_max SPIN_MAX /* spin cycles for multiprocessor */
    static unsigned spin_max = low_spin_max;
    unsigned my_spin_max;
    static unsigned last_spins = 0;
    unsigned my_last_spins;
    int i;

    if (AO_test_and_set_acquire(&GC_allocate_lock) == AO_TS_CLEAR) {
        return;
    }
    my_spin_max = spin_max;
    my_last_spins = last_spins;
    for (i = 0; i < my_spin_max; i++) {
        if (GC_collecting || GC_nprocs == 1) goto yield;
        if (i < my_last_spins/2) {
            GC_pause();
            continue;
        }
        if (AO_test_and_set_acquire(&GC_allocate_lock) == AO_TS_CLEAR) {
	    /*
             * got it!
             * Spinning worked.  Thus we're probably not being scheduled
             * against the other process with which we were contending.
             * Thus it makes sense to spin longer the next time.
	     */
            last_spins = i;
            spin_max = high_spin_max;
            return;
        }
    }
    /* We are probably being scheduled against the other process.  Sleep. */
    spin_max = low_spin_max;
yield:
    for (i = 0;; ++i) {
        if (AO_test_and_set_acquire(&GC_allocate_lock) == AO_TS_CLEAR) {
            return;
        }
#       define SLEEP_THRESHOLD 12
		/* Under Linux very short sleeps tend to wait until	*/
		/* the current time quantum expires.  On old Linux	*/
		/* kernels nanosleep(<= 2ms) just spins under Linux.    */
		/* (Under 2.4, this happens only for real-time		*/
		/* processes.)  We want to minimize both behaviors	*/
		/* here.						*/
        if (i < SLEEP_THRESHOLD) {
            sched_yield();
	} else {
	    struct timespec ts;
	
	    if (i > 24) i = 24;
			/* Don't wait for more than about 15msecs, even	*/
			/* under extreme contention.			*/
	    ts.tv_sec = 0;
	    ts.tv_nsec = 1 << i;
	    nanosleep(&ts, 0);
	}
    }
}

#else  /* !USE_SPINLOCK */
void GC_lock(void)
{
#ifndef NO_PTHREAD_TRYLOCK
    if (1 == GC_nprocs || GC_collecting) {
	pthread_mutex_lock(&GC_allocate_ml);
    } else {
        GC_generic_lock(&GC_allocate_ml);
    }
#else  /* !NO_PTHREAD_TRYLOCK */
    pthread_mutex_lock(&GC_allocate_ml);
#endif /* !NO_PTHREAD_TRYLOCK */
}

#endif /* !USE_SPINLOCK */

#if defined(PARALLEL_MARK) || defined(THREAD_LOCAL_ALLOC)

#ifdef GC_ASSERTIONS
  unsigned long GC_mark_lock_holder = NO_THREAD;
#endif

#if 0
  /* Ugly workaround for a linux threads bug in the final versions      */
  /* of glibc2.1.  Pthread_mutex_trylock sets the mutex owner           */
  /* field even when it fails to acquire the mutex.  This causes        */
  /* pthread_cond_wait to die.  Remove for glibc2.2.                    */
  /* According to the man page, we should use                           */
  /* PTHREAD_ERRORCHECK_MUTEX_INITIALIZER_NP, but that isn't actually   */
  /* defined.                                                           */
  static pthread_mutex_t mark_mutex =
        {0, 0, 0, PTHREAD_MUTEX_ERRORCHECK_NP, {0, 0}};
#else
  static pthread_mutex_t mark_mutex = PTHREAD_MUTEX_INITIALIZER;
#endif

static pthread_cond_t builder_cv = PTHREAD_COND_INITIALIZER;

void GC_acquire_mark_lock(void)
{
/*
    if (pthread_mutex_lock(&mark_mutex) != 0) {
	ABORT("pthread_mutex_lock failed");
    }
*/
    GC_generic_lock(&mark_mutex);
#   ifdef GC_ASSERTIONS
	GC_mark_lock_holder = NUMERIC_THREAD_ID(pthread_self());
#   endif
}

void GC_release_mark_lock(void)
{
    GC_ASSERT(GC_mark_lock_holder == NUMERIC_THREAD_ID(pthread_self()));
#   ifdef GC_ASSERTIONS
	GC_mark_lock_holder = NO_THREAD;
#   endif
    if (pthread_mutex_unlock(&mark_mutex) != 0) {
	ABORT("pthread_mutex_unlock failed");
    }
}

/* Collector must wait for a freelist builders for 2 reasons:		*/
/* 1) Mark bits may still be getting examined without lock.		*/
/* 2) Partial free lists referenced only by locals may not be scanned 	*/
/*    correctly, e.g. if they contain "pointer-free" objects, since the	*/
/*    free-list link may be ignored.					*/
void GC_wait_builder(void)
{
    GC_ASSERT(GC_mark_lock_holder == NUMERIC_THREAD_ID(pthread_self()));
#   ifdef GC_ASSERTIONS
	GC_mark_lock_holder = NO_THREAD;
#   endif
    if (pthread_cond_wait(&builder_cv, &mark_mutex) != 0) {
	ABORT("pthread_cond_wait failed");
    }
    GC_ASSERT(GC_mark_lock_holder == NO_THREAD);
#   ifdef GC_ASSERTIONS
	GC_mark_lock_holder = NUMERIC_THREAD_ID(pthread_self());
#   endif
}

void GC_wait_for_reclaim(void)
{
    GC_acquire_mark_lock();
    while (GC_fl_builder_count > 0) {
	GC_wait_builder();
    }
    GC_release_mark_lock();
}

void GC_notify_all_builder(void)
{
    GC_ASSERT(GC_mark_lock_holder == NUMERIC_THREAD_ID(pthread_self()));
    if (pthread_cond_broadcast(&builder_cv) != 0) {
	ABORT("pthread_cond_broadcast failed");
    }
}

#endif /* PARALLEL_MARK || THREAD_LOCAL_ALLOC */

#ifdef PARALLEL_MARK

static pthread_cond_t mark_cv = PTHREAD_COND_INITIALIZER;

void GC_wait_marker(void)
{
    GC_ASSERT(GC_mark_lock_holder == NUMERIC_THREAD_ID(pthread_self()));
#   ifdef GC_ASSERTIONS
	GC_mark_lock_holder = NO_THREAD;
#   endif
    if (pthread_cond_wait(&mark_cv, &mark_mutex) != 0) {
	ABORT("pthread_cond_wait failed");
    }
    GC_ASSERT(GC_mark_lock_holder == NO_THREAD);
#   ifdef GC_ASSERTIONS
	GC_mark_lock_holder = NUMERIC_THREAD_ID(pthread_self());
#   endif
}

void GC_notify_all_marker(void)
{
    if (pthread_cond_broadcast(&mark_cv) != 0) {
	ABORT("pthread_cond_broadcast failed");
    }
}

#endif /* PARALLEL_MARK */

# endif /* GC_LINUX_THREADS and friends */

