/* Our pthread support normally needs to intercept a number of thread	*/
/* calls.  We arrange to do that here, if appropriate.			*/

#ifndef GC_PTHREAD_REDIRECTS_H

#define GC_PTHREAD_REDIRECTS_H

#if !defined(GC_USE_LD_WRAP) && defined(GC_PTHREADS)
/* We need to intercept calls to many of the threads primitives, so   	 */
/* that we can locate thread stacks and stop the world.                  */
/* Note also that the collector cannot always see thread specific data.  */
/* Thread specific data should generally consist of pointers to          */
/* uncollectable objects (allocated with GC_malloc_uncollectable,        */
/* not the system malloc), which are deallocated using the destructor    */
/* facility in thr_keycreate.  Alternatively, keep a redundant pointer   */
/* to thread specific data on the thread stack. 			 */

# include <pthread.h>
# include <signal.h>

  int GC_pthread_create(pthread_t *new_thread,
                        const pthread_attr_t *attr,
		        void *(*start_routine)(void *), void *arg);
#ifndef GC_DARWIN_THREADS
  int GC_pthread_sigmask(int how, const sigset_t *set, sigset_t *oset);
#endif
  int GC_pthread_join(pthread_t thread, void **retval);
  int GC_pthread_detach(pthread_t thread);

#if defined(GC_OSF1_THREADS) \
    && defined(_PTHREAD_USE_MANGLED_NAMES_) && !defined(_PTHREAD_USE_PTDNAM_)
/* Unless the compiler supports #pragma extern_prefix, the Tru64 UNIX
   <pthread.h> redefines some POSIX thread functions to use mangled names.
   If so, undef them before redefining. */
# undef pthread_create
# undef pthread_join
# undef pthread_detach
#endif

# define pthread_create GC_pthread_create
# define pthread_join GC_pthread_join
# define pthread_detach GC_pthread_detach

#ifndef GC_DARWIN_THREADS
# ifdef pthread_sigmask
#  undef pthread_sigmask
# endif	 /* pthread_sigmask */
# define pthread_sigmask GC_pthread_sigmask
# define dlopen GC_dlopen
#endif

#endif /* GC_xxxxx_THREADS */

#endif /* GC_PTHREAD_REDIRECTS_H */
