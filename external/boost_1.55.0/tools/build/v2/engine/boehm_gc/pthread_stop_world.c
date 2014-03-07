#include "private/pthread_support.h"

#if defined(GC_PTHREADS) && !defined(GC_WIN32_THREADS) && \
    !defined(GC_DARWIN_THREADS)

#include <signal.h>
#include <semaphore.h>
#include <errno.h>
#include <unistd.h>
#include "atomic_ops.h"

#if DEBUG_THREADS

#ifndef NSIG
# if defined(MAXSIG)
#  define NSIG (MAXSIG+1)
# elif defined(_NSIG)
#  define NSIG _NSIG
# elif defined(__SIGRTMAX)
#  define NSIG (__SIGRTMAX+1)
# else
  --> please fix it
# endif
#endif

void GC_print_sig_mask()
{
    sigset_t blocked;
    int i;

    if (pthread_sigmask(SIG_BLOCK, NULL, &blocked) != 0)
    	ABORT("pthread_sigmask");
    GC_printf("Blocked: ");
    for (i = 1; i < NSIG; i++) {
        if (sigismember(&blocked, i)) { GC_printf("%d ", i); }
    }
    GC_printf("\n");
}

#endif

/* Remove the signals that we want to allow in thread stopping 	*/
/* handler from a set.						*/
void GC_remove_allowed_signals(sigset_t *set)
{
    if (sigdelset(set, SIGINT) != 0
	  || sigdelset(set, SIGQUIT) != 0
	  || sigdelset(set, SIGABRT) != 0
	  || sigdelset(set, SIGTERM) != 0) {
        ABORT("sigdelset() failed");
    }

#   ifdef MPROTECT_VDB
      /* Handlers write to the thread structure, which is in the heap,	*/
      /* and hence can trigger a protection fault.			*/
      if (sigdelset(set, SIGSEGV) != 0
#	  ifdef SIGBUS
	    || sigdelset(set, SIGBUS) != 0
# 	  endif
	  ) {
        ABORT("sigdelset() failed");
      }
#   endif
}

static sigset_t suspend_handler_mask;

volatile AO_t GC_stop_count;
			/* Incremented at the beginning of GC_stop_world. */

volatile AO_t GC_world_is_stopped = FALSE;
			/* FALSE ==> it is safe for threads to restart, i.e. */
			/* they will see another suspend signal before they  */
			/* are expected to stop (unless they have voluntarily */
			/* stopped).					     */

#ifdef GC_OSF1_THREADS
  GC_bool GC_retry_signals = TRUE;
#else
  GC_bool GC_retry_signals = FALSE;
#endif

/*
 * We use signals to stop threads during GC.
 * 
 * Suspended threads wait in signal handler for SIG_THR_RESTART.
 * That's more portable than semaphores or condition variables.
 * (We do use sem_post from a signal handler, but that should be portable.)
 *
 * The thread suspension signal SIG_SUSPEND is now defined in gc_priv.h.
 * Note that we can't just stop a thread; we need it to save its stack
 * pointer(s) and acknowledge.
 */

#ifndef SIG_THR_RESTART
#  if defined(GC_HPUX_THREADS) || defined(GC_OSF1_THREADS) || defined(GC_NETBSD_THREADS)
#    ifdef _SIGRTMIN
#      define SIG_THR_RESTART _SIGRTMIN + 5
#    else
#      define SIG_THR_RESTART SIGRTMIN + 5
#    endif
#  else
#   define SIG_THR_RESTART SIGXCPU
#  endif
#endif

sem_t GC_suspend_ack_sem;

#ifdef GC_NETBSD_THREADS
# define GC_NETBSD_THREADS_WORKAROUND
  /* It seems to be necessary to wait until threads have restarted.	*/
  /* But it is unclear why that is the case.				*/
  sem_t GC_restart_ack_sem;
#endif

void GC_suspend_handler_inner(ptr_t sig_arg, void *context);

#if defined(IA64) || defined(HP_PA) || defined(M68K)
void GC_suspend_handler(int sig, siginfo_t *info, void *context)
{
  int old_errno = errno;
  GC_with_callee_saves_pushed(GC_suspend_handler_inner, (ptr_t)(word)sig);
  errno = old_errno;
}
#else
/* We believe that in all other cases the full context is already	*/
/* in the signal handler frame.						*/
void GC_suspend_handler(int sig, siginfo_t *info, void *context)
{
  int old_errno = errno;
  GC_suspend_handler_inner((ptr_t)(word)sig, context);
  errno = old_errno;
}
#endif

void GC_suspend_handler_inner(ptr_t sig_arg, void *context)
{
    int sig = (int)(word)sig_arg;
    int dummy;
    pthread_t my_thread = pthread_self();
    GC_thread me;
#   ifdef PARALLEL_MARK
	word my_mark_no = GC_mark_no;
	/* Marker can't proceed until we acknowledge.  Thus this is	*/
	/* guaranteed to be the mark_no correspending to our 		*/
	/* suspension, i.e. the marker can't have incremented it yet.	*/
#   endif
    AO_t my_stop_count = AO_load(&GC_stop_count);

    if (sig != SIG_SUSPEND) ABORT("Bad signal in suspend_handler");

#   if DEBUG_THREADS
      GC_printf("Suspending 0x%x\n", (unsigned)my_thread);
#   endif

    me = GC_lookup_thread(my_thread);
    /* The lookup here is safe, since I'm doing this on behalf  */
    /* of a thread which holds the allocation lock in order	*/
    /* to stop the world.  Thus concurrent modification of the	*/
    /* data structure is impossible.				*/
    if (me -> stop_info.last_stop_count == my_stop_count) {
	/* Duplicate signal.  OK if we are retrying.	*/
	if (!GC_retry_signals) {
	    WARN("Duplicate suspend signal in thread %lx\n",
		 pthread_self());
	}
	return;
    }
#   ifdef SPARC
	me -> stop_info.stack_ptr = GC_save_regs_in_stack();
#   else
	me -> stop_info.stack_ptr = (ptr_t)(&dummy);
#   endif
#   ifdef IA64
	me -> backing_store_ptr = GC_save_regs_in_stack();
#   endif

    /* Tell the thread that wants to stop the world that this   */
    /* thread has been stopped.  Note that sem_post() is  	*/
    /* the only async-signal-safe primitive in LinuxThreads.    */
    sem_post(&GC_suspend_ack_sem);
    me -> stop_info.last_stop_count = my_stop_count;

    /* Wait until that thread tells us to restart by sending    */
    /* this thread a SIG_THR_RESTART signal.			*/
    /* SIG_THR_RESTART should be masked at this point.  Thus there	*/
    /* is no race.						*/
    /* We do not continue until we receive a SIG_THR_RESTART,	*/
    /* but we do not take that as authoritative.  (We may be	*/
    /* accidentally restarted by one of the user signals we 	*/
    /* don't block.)  After we receive the signal, we use a 	*/
    /* primitive and expensive mechanism to wait until it's	*/
    /* really safe to proceed.  Under normal circumstances,	*/
    /* this code should not be executed.			*/
    do {
	sigsuspend (&suspend_handler_mask);
    } while (AO_load_acquire(&GC_world_is_stopped)
	     && AO_load(&GC_stop_count) == my_stop_count);
    /* If the RESTART signal gets lost, we can still lose.  That should be  */
    /* less likely than losing the SUSPEND signal, since we don't do much   */
    /* between the sem_post and sigsuspend.	   			    */
    /* We'd need more handshaking to work around that.			    */
    /* Simply dropping the sigsuspend call should be safe, but is unlikely  */
    /* to be efficient.							    */

#   if DEBUG_THREADS
      GC_printf("Continuing 0x%x\n", (unsigned)my_thread);
#   endif
}

void GC_restart_handler(int sig)
{
    pthread_t my_thread = pthread_self();
    GC_thread me;

    if (sig != SIG_THR_RESTART) ABORT("Bad signal in suspend_handler");

#   ifdef GC_NETBSD_THREADS_WORKAROUND
      sem_post(&GC_restart_ack_sem);
#   endif

    /*
    ** Note: even if we don't do anything useful here,
    ** it would still be necessary to have a signal handler,
    ** rather than ignoring the signals, otherwise
    ** the signals will not be delivered at all, and
    ** will thus not interrupt the sigsuspend() above.
    */

#   if DEBUG_THREADS
      GC_printf("In GC_restart_handler for 0x%x\n", (unsigned)pthread_self());
#   endif
}

# ifdef IA64
#   define IF_IA64(x) x
# else
#   define IF_IA64(x)
# endif
/* We hold allocation lock.  Should do exactly the right thing if the	*/
/* world is stopped.  Should not fail if it isn't.			*/
void GC_push_all_stacks()
{
    GC_bool found_me = FALSE;
    size_t nthreads = 0;
    int i;
    GC_thread p;
    ptr_t lo, hi;
    /* On IA64, we also need to scan the register backing store. */
    IF_IA64(ptr_t bs_lo; ptr_t bs_hi;)
    pthread_t me = pthread_self();
    
    if (!GC_thr_initialized) GC_thr_init();
#   if DEBUG_THREADS
        GC_printf("Pushing stacks from thread 0x%x\n", (unsigned) me);
#   endif
    for (i = 0; i < THREAD_TABLE_SZ; i++) {
      for (p = GC_threads[i]; p != 0; p = p -> next) {
        if (p -> flags & FINISHED) continue;
	++nthreads;
        if (THREAD_EQUAL(p -> id, me)) {
#  	    ifdef SPARC
	        lo = (ptr_t)GC_save_regs_in_stack();
#  	    else
 	        lo = GC_approx_sp();
#           endif
	    found_me = TRUE;
	    IF_IA64(bs_hi = (ptr_t)GC_save_regs_in_stack();)
	} else {
	    lo = p -> stop_info.stack_ptr;
	    IF_IA64(bs_hi = p -> backing_store_ptr;)
	}
        if ((p -> flags & MAIN_THREAD) == 0) {
	    hi = p -> stack_end;
	    IF_IA64(bs_lo = p -> backing_store_end);
        } else {
            /* The original stack. */
            hi = GC_stackbottom;
	    IF_IA64(bs_lo = BACKING_STORE_BASE;)
        }
#	if DEBUG_THREADS
            GC_printf("Stack for thread 0x%x = [%p,%p)\n",
    	              (unsigned)(p -> id), lo, hi);
#	endif
	if (0 == lo) ABORT("GC_push_all_stacks: sp not set!\n");
#       ifdef STACK_GROWS_UP
	  /* We got them backwards! */
          GC_push_all_stack(hi, lo);
#       else
          GC_push_all_stack(lo, hi);
#	endif
#	ifdef IA64
#         if DEBUG_THREADS
            GC_printf("Reg stack for thread 0x%x = [%lx,%lx)\n",
    	              (unsigned)p -> id, bs_lo, bs_hi);
#	  endif
          if (THREAD_EQUAL(p -> id, me)) {
	    /* FIXME:  This may add an unbounded number of entries,	*/
	    /* and hence overflow the mark stack, which is bad.		*/
	    GC_push_all_eager(bs_lo, bs_hi);
	  } else {
	    GC_push_all_stack(bs_lo, bs_hi);
	  }
#	endif
      }
    }
    if (GC_print_stats == VERBOSE) {
	GC_log_printf("Pushed %d thread stacks\n", nthreads);
    }
    if (!found_me && !GC_in_thread_creation)
      ABORT("Collecting from unknown thread.");
}

/* There seems to be a very rare thread stopping problem.  To help us  */
/* debug that, we save the ids of the stopping thread. */
pthread_t GC_stopping_thread;
int GC_stopping_pid;

/* We hold the allocation lock.  Suspend all threads that might	*/
/* still be running.  Return the number of suspend signals that	*/
/* were sent. */
int GC_suspend_all()
{
    int n_live_threads = 0;
    int i;
    GC_thread p;
    int result;
    pthread_t my_thread = pthread_self();
    
    GC_stopping_thread = my_thread;    /* debugging only.      */
    GC_stopping_pid = getpid();                /* debugging only.      */
    for (i = 0; i < THREAD_TABLE_SZ; i++) {
      for (p = GC_threads[i]; p != 0; p = p -> next) {
        if (!THREAD_EQUAL(p -> id, my_thread)) {
            if (p -> flags & FINISHED) continue;
            if (p -> stop_info.last_stop_count == GC_stop_count) continue;
	    if (p -> thread_blocked) /* Will wait */ continue;
            n_live_threads++;
#	    if DEBUG_THREADS
	      GC_printf("Sending suspend signal to 0x%x\n",
			(unsigned)(p -> id));
#	    endif
        
            result = pthread_kill(p -> id, SIG_SUSPEND);
	    switch(result) {
                case ESRCH:
                    /* Not really there anymore.  Possible? */
                    n_live_threads--;
                    break;
                case 0:
                    break;
                default:
                    ABORT("pthread_kill failed");
            }
        }
      }
    }
    return n_live_threads;
}

void GC_stop_world()
{
    int i;
    int n_live_threads;
    int code;

    GC_ASSERT(I_HOLD_LOCK());
#   if DEBUG_THREADS
      GC_printf("Stopping the world from 0x%x\n", (unsigned)pthread_self());
#   endif
       
    /* Make sure all free list construction has stopped before we start. */
    /* No new construction can start, since free list construction is	*/
    /* required to acquire and release the GC lock before it starts,	*/
    /* and we have the lock.						*/
#   ifdef PARALLEL_MARK
      GC_acquire_mark_lock();
      GC_ASSERT(GC_fl_builder_count == 0);
      /* We should have previously waited for it to become zero. */
#   endif /* PARALLEL_MARK */
    AO_store(&GC_stop_count, GC_stop_count+1);
    	/* Only concurrent reads are possible. */
    AO_store_release(&GC_world_is_stopped, TRUE);
    n_live_threads = GC_suspend_all();

      if (GC_retry_signals) {
	  unsigned long wait_usecs = 0;  /* Total wait since retry.	*/
#	  define WAIT_UNIT 3000
#	  define RETRY_INTERVAL 100000
	  for (;;) {
	      int ack_count;

	      sem_getvalue(&GC_suspend_ack_sem, &ack_count);
	      if (ack_count == n_live_threads) break;
	      if (wait_usecs > RETRY_INTERVAL) {
		  int newly_sent = GC_suspend_all();

		  if (GC_print_stats) {
		      GC_log_printf("Resent %d signals after timeout\n",
				newly_sent);
		  }
		  sem_getvalue(&GC_suspend_ack_sem, &ack_count);
		  if (newly_sent < n_live_threads - ack_count) {
		      WARN("Lost some threads during GC_stop_world?!\n",0);
		      n_live_threads = ack_count + newly_sent;
		  }
		  wait_usecs = 0;
	      }
	      usleep(WAIT_UNIT);
	      wait_usecs += WAIT_UNIT;
	  }
      }
    for (i = 0; i < n_live_threads; i++) {
	retry:
	  if (0 != (code = sem_wait(&GC_suspend_ack_sem))) {
	      /* On Linux, sem_wait is documented to always return zero.*/
	      /* But the documentation appears to be incorrect.		*/
	      if (errno == EINTR) {
		/* Seems to happen with some versions of gdb.	*/
		goto retry;
	      }
	      ABORT("sem_wait for handler failed");
	  }
    }
#   ifdef PARALLEL_MARK
      GC_release_mark_lock();
#   endif
    #if DEBUG_THREADS
      GC_printf("World stopped from 0x%x\n", (unsigned)pthread_self());
    #endif
    GC_stopping_thread = 0;  /* debugging only */
}

/* Caller holds allocation lock, and has held it continuously since	*/
/* the world stopped.							*/
void GC_start_world()
{
    pthread_t my_thread = pthread_self();
    register int i;
    register GC_thread p;
    register int n_live_threads = 0;
    register int result;
#   ifdef GC_NETBSD_THREADS_WORKAROUND
      int code;
#   endif

#   if DEBUG_THREADS
      GC_printf("World starting\n");
#   endif

    AO_store(&GC_world_is_stopped, FALSE);
    for (i = 0; i < THREAD_TABLE_SZ; i++) {
      for (p = GC_threads[i]; p != 0; p = p -> next) {
        if (!THREAD_EQUAL(p -> id, my_thread)) {
            if (p -> flags & FINISHED) continue;
	    if (p -> thread_blocked) continue;
            n_live_threads++;
	    #if DEBUG_THREADS
	      GC_printf("Sending restart signal to 0x%x\n",
			(unsigned)(p -> id));
	    #endif
        
            result = pthread_kill(p -> id, SIG_THR_RESTART);
	    switch(result) {
                case ESRCH:
                    /* Not really there anymore.  Possible? */
                    n_live_threads--;
                    break;
                case 0:
                    break;
                default:
                    ABORT("pthread_kill failed");
            }
        }
      }
    }
#   ifdef GC_NETBSD_THREADS_WORKAROUND
      for (i = 0; i < n_live_threads; i++)
	while (0 != (code = sem_wait(&GC_restart_ack_sem)))
	    if (errno != EINTR) {
		GC_err_printf1("sem_wait() returned %ld\n",
			       (unsigned long)code);
		ABORT("sem_wait() for restart handler failed");
	    }
#    endif
#    if DEBUG_THREADS
      GC_printf("World started\n");
#    endif
}

void GC_stop_init() {
    struct sigaction act;
    
    if (sem_init(&GC_suspend_ack_sem, 0, 0) != 0)
        ABORT("sem_init failed");
#   ifdef GC_NETBSD_THREADS_WORKAROUND
      if (sem_init(&GC_restart_ack_sem, 0, 0) != 0)
	ABORT("sem_init failed");
#   endif

    act.sa_flags = SA_RESTART | SA_SIGINFO;
    if (sigfillset(&act.sa_mask) != 0) {
    	ABORT("sigfillset() failed");
    }
    GC_remove_allowed_signals(&act.sa_mask);
    /* SIG_THR_RESTART is set in the resulting mask.		*/
    /* It is unmasked by the handler when necessary. 		*/
    act.sa_sigaction = GC_suspend_handler;
    if (sigaction(SIG_SUSPEND, &act, NULL) != 0) {
    	ABORT("Cannot set SIG_SUSPEND handler");
    }

    act.sa_flags &= ~ SA_SIGINFO;
    act.sa_handler = GC_restart_handler;
    if (sigaction(SIG_THR_RESTART, &act, NULL) != 0) {
    	ABORT("Cannot set SIG_THR_RESTART handler");
    }

    /* Inititialize suspend_handler_mask. It excludes SIG_THR_RESTART. */
      if (sigfillset(&suspend_handler_mask) != 0) ABORT("sigfillset() failed");
      GC_remove_allowed_signals(&suspend_handler_mask);
      if (sigdelset(&suspend_handler_mask, SIG_THR_RESTART) != 0)
	  ABORT("sigdelset() failed");

    /* Check for GC_RETRY_SIGNALS.	*/
      if (0 != GETENV("GC_RETRY_SIGNALS")) {
	  GC_retry_signals = TRUE;
      }
      if (0 != GETENV("GC_NO_RETRY_SIGNALS")) {
	  GC_retry_signals = FALSE;
      }
      if (GC_print_stats && GC_retry_signals) {
          GC_log_printf("Will retry suspend signal if necessary.\n");
      }
}

#endif
