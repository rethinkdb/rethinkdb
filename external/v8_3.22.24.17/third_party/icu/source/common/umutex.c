/*
******************************************************************************
*
*   Copyright (C) 1997-2009, International Business Machines
*   Corporation and others.  All Rights Reserved.
*
******************************************************************************
*
* File umutex.c
*
* Modification History:
*
*   Date        Name        Description
*   04/02/97    aliu        Creation.
*   04/07/99    srl         updated
*   05/13/99    stephen     Changed to umutex (from cmutex).
*   11/22/99    aliu        Make non-global mutex autoinitialize [j151]
******************************************************************************
*/

#include "unicode/utypes.h"
#include "uassert.h"
#include "ucln_cmn.h"

/*
 * ICU Mutex wrappers.  Wrap operating system mutexes, giving the rest of ICU a
 * platform independent set of mutex operations.  For internal ICU use only.
 */

#if defined(U_DARWIN)
#include <AvailabilityMacros.h>
#if (ICU_USE_THREADS == 1) && defined(MAC_OS_X_VERSION_10_4) && defined(MAC_OS_X_VERSION_MIN_REQUIRED) && (MAC_OS_X_VERSION_MIN_REQUIRED >= MAC_OS_X_VERSION_10_4)
#if defined(__STRICT_ANSI__)
#define UPRV_REMAP_INLINE
#define inline
#endif
#include <libkern/OSAtomic.h>
#define USE_MAC_OS_ATOMIC_INCREMENT 1
#if defined(UPRV_REMAP_INLINE)
#undef inline
#undef UPRV_REMAP_INLINE
#endif
#endif
#endif

/* Assume POSIX, and modify as necessary below */
#define POSIX

#if defined(U_WINDOWS)
#undef POSIX
#endif
#if defined(macintosh)
#undef POSIX
#endif
#if defined(OS2)
#undef POSIX
#endif

#if defined(POSIX) && (ICU_USE_THREADS==1)
# include <pthread.h> /* must be first, so that we get the multithread versions of things. */

#endif /* POSIX && (ICU_USE_THREADS==1) */

#ifdef U_WINDOWS
# define WIN32_LEAN_AND_MEAN
# define VC_EXTRALEAN
# define NOUSER
# define NOSERVICE
# define NOIME
# define NOMCX
# include <windows.h>
#endif

#include "umutex.h"
#include "cmemory.h"

/*
 * A note on ICU Mutex Initialization and ICU startup:
 *
 *   ICU mutexes, as used through the rest of the ICU code, are self-initializing.
 *   To make this work, ICU uses the _ICU GLobal Mutex_ to synchronize the lazy init
 *   of other ICU mutexes.  For the global mutex itself, we need some other mechanism
 *   to safely initialize it on first use.  This becomes important when two or more
 *   threads are more or less simultaenously the first to use ICU in a process, and
 *   are racing into the mutex initialization code.
 *
 *
 *   The solution for the global mutex init is platform dependent.
 *   On POSIX systems, plain C-style initialization can be used on a mutex, with the 
 *   macro PTHREAD_MUTEX_INITIALIZER.  The mutex is then ready for use, without
 *   first calling pthread_mutex_init().
 *
 *   Windows has no equivalent statically initialized mutex or CRITICAL SECION.
 *   InitializeCriticalSection() must be called.  If the global mutex does not
 *   appear to be initialized, a thread will create and initialize a new
 *   CRITICAL_SECTION, then use a Windows InterlockedCompareAndExchange to
 *   swap it in as the global mutex while avoid problems with race conditions.
 */ 

/* On WIN32 mutexes are reentrant.  On POSIX platforms they are not, and a deadlock
 *  will occur if a thread attempts to acquire a mutex it already has locked.
 *  ICU mutexes (in debug builds) include checking code that will cause an assertion
 *  failure if a mutex is reentered.  If you are having deadlock problems
 *  on a POSIX machine, debugging may be easier on Windows.
 */


#if (ICU_USE_THREADS == 0)
#define MUTEX_TYPE void *
#define PLATFORM_MUTEX_INIT(m) 
#define PLATFORM_MUTEX_LOCK(m) 
#define PLATFORM_MUTEX_UNLOCK(m) 
#define PLATFORM_MUTEX_DESTROY(m) 
#define PLATFORM_MUTEX_INITIALIZER NULL
#define SYNC_COMPARE_AND_SWAP(dest, oldval, newval) \
            mutexed_compare_and_swap(dest, newval, oldval)


#elif defined(U_WINDOWS)
#define MUTEX_TYPE CRITICAL_SECTION
#define PLATFORM_MUTEX_INIT(m) InitializeCriticalSection(m)
#define PLATFORM_MUTEX_LOCK(m) EnterCriticalSection(m)
#define PLATFORM_MUTEX_UNLOCK(m) LeaveCriticalSection(m)
#define PLATFORM_MUTEX_DESTROY(m) DeleteCriticalSection(m)
#define SYNC_COMPARE_AND_SWAP(dest, oldval, newval) \
            InterlockedCompareExchangePointer(dest, newval, oldval)


#elif defined(POSIX)
#define MUTEX_TYPE pthread_mutex_t   
#define PLATFORM_MUTEX_INIT(m) pthread_mutex_init(m, NULL)
#define PLATFORM_MUTEX_LOCK(m) pthread_mutex_lock(m)
#define PLATFORM_MUTEX_UNLOCK(m) pthread_mutex_unlock(m)
#define PLATFORM_MUTEX_DESTROY(m) pthread_mutex_destroy(m)
#define PLATFORM_MUTEX_INITIALIZER PTHREAD_MUTEX_INITIALIZER
#if (U_HAVE_GCC_ATOMICS == 1)
#define SYNC_COMPARE_AND_SWAP(dest, oldval, newval) \
            __sync_val_compare_and_swap(dest, oldval, newval)
#else
#define SYNC_COMPARE_AND_SWAP(dest, oldval, newval) \
            mutexed_compare_and_swap(dest, newval, oldval)
#endif


#else   
/* Unknown platform.  Note that user can still set mutex functions at run time. */
#define MUTEX_TYPE void *
#define PLATFORM_MUTEX_INIT(m) 
#define PLATFORM_MUTEX_LOCK(m)
#define PLATFORM_MUTEX_UNLOCK(m) 
#define PLATFORM_MUTEX_DESTROY(m) 
#define SYNC_COMPARE_AND_SWAP(dest, oldval, newval) \
            mutexed_compare_and_swap(dest, newval, oldval)

#endif

/*  Forward declarations */
static void *mutexed_compare_and_swap(void **dest, void *newval, void *oldval);
typedef struct ICUMutex ICUMutex;

/*
 * ICUMutex   One of these is set up for each UMTX that is used by other ICU code.
 *            The opaque UMTX points to the corresponding ICUMutex struct.
 *            
 *            Because the total number of ICU mutexes is quite small, no effort has
 *            been made to squeeze every byte out of this struct.
 */
struct ICUMutex {
    UMTX        *owner;             /* Points back to the UMTX corrsponding to this   */
                                    /*    ICUMutex object.                            */
    
    UBool        heapAllocated;     /* Set if this ICUMutex is heap allocated, and    */
                                    /*   will need to be deleted.  The global mutex   */
                                    /*   is static on POSIX platforms; all others     */
                                    /*   will be heap allocated.                      */

    ICUMutex    *next;              /* All ICUMutexes are chained into a list so that  */
                                    /*   they can be found and deleted by u_cleanup(). */

    int32_t      recursionCount;    /* For debugging, detect recursive mutex locks.    */

    MUTEX_TYPE   platformMutex;     /* The underlying OS mutex being wrapped.          */

    UMTX         userMutex;         /* For use with u_setMutexFunctions operations,    */
                                    /*    corresponds to platformMutex.                */
};


/*   The global ICU mutex.
 *   For POSIX platforms, it gets a C style initialization, and is ready to use
 *        at program startup.
 *   For Windows, it will be lazily instantiated on first use.
 */

#if defined(POSIX)
static UMTX  globalUMTX;
static ICUMutex globalMutex = {&globalUMTX, FALSE, NULL, 0, PLATFORM_MUTEX_INITIALIZER, NULL};
static UMTX  globalUMTX = &globalMutex;
#else
static UMTX  globalUMTX = NULL;
#endif

/* Head of the list of all ICU mutexes.
 * Linked list is through ICUMutex::next
 * Modifications to the list are synchronized with the global mutex.
 * The list is used by u_cleanup(), which needs to dispose of all of the ICU mutexes.
 *
 * The statically initialized global mutex on POSIX platforms does not get added to this
 * mutex list, but that's not a problem - the global mutex gets special handling
 * during u_cleanup().
 */
static ICUMutex *mutexListHead;


/*
 *  User mutex implementation functions.  If non-null, call back to these rather than
 *  directly using the system (Posix or Windows) APIs.  See u_setMutexFunctions().
 *    (declarations are in uclean.h)
 */
static UMtxInitFn    *pMutexInitFn    = NULL;
static UMtxFn        *pMutexDestroyFn = NULL;
static UMtxFn        *pMutexLockFn    = NULL;
static UMtxFn        *pMutexUnlockFn  = NULL;
static const void    *gMutexContext   = NULL;


/*
 *   umtx_lock
 */
U_CAPI void  U_EXPORT2
umtx_lock(UMTX *mutex)
{
    ICUMutex *m;

    if (mutex == NULL) {
        mutex = &globalUMTX;
    }
    m = (ICUMutex *)*mutex;
    if (m == NULL) {
        /* See note on lazy initialization, above.  We can get away with it here, with mutexes,
         * where we couldn't with normal user level data.
         */
        umtx_init(mutex);    
        m = (ICUMutex *)*mutex;
    }
    U_ASSERT(m->owner == mutex);

    if (pMutexLockFn != NULL) {
        (*pMutexLockFn)(gMutexContext, &m->userMutex);
    } else {
        PLATFORM_MUTEX_LOCK(&m->platformMutex);
    }

#if defined(U_DEBUG)
    m->recursionCount++;              /* Recursion causes deadlock on Unixes.               */
    U_ASSERT(m->recursionCount == 1); /* Recursion detection works on Windows.              */
                                      /* Assertion failure on non-Windows indicates a       */
                                      /*   problem with the mutex implementation itself.    */
#endif
}



/*
 * umtx_unlock
 */
U_CAPI void  U_EXPORT2
umtx_unlock(UMTX* mutex)
{
    ICUMutex *m;
    if(mutex == NULL) {
        mutex = &globalUMTX;
    }
    m = (ICUMutex *)*mutex;
    if (m == NULL) {
        U_ASSERT(FALSE);  /* This mutex is not initialized.     */
        return; 
    }
    U_ASSERT(m->owner == mutex);

#if defined (U_DEBUG)
    m->recursionCount--;
    U_ASSERT(m->recursionCount == 0);  /* Detect unlock of an already unlocked mutex */
#endif

    if (pMutexUnlockFn) {
        (*pMutexUnlockFn)(gMutexContext, &m->userMutex);
    } else {
        PLATFORM_MUTEX_UNLOCK(&m->platformMutex);
    }
}


/* umtx_ct   Allocate and initialize a new ICUMutex.
 *           If a non-null pointer is supplied, initialize an existing ICU Mutex.
 */
static ICUMutex *umtx_ct(ICUMutex *m) {
    if (m == NULL) {
        m = (ICUMutex *)uprv_malloc(sizeof(ICUMutex));
        m->heapAllocated = TRUE;
    }
    m->next = NULL;    /* List of mutexes is maintained at a higher level.  */
    m->recursionCount = 0;
    m->userMutex = NULL;
    if (pMutexInitFn != NULL) {
        UErrorCode status = U_ZERO_ERROR;
        (*pMutexInitFn)(gMutexContext, &m->userMutex, &status);
        U_ASSERT(U_SUCCESS(status));
    } else {
        PLATFORM_MUTEX_INIT(&m->platformMutex);
    }
    return m;
}


/* umtx_dt   Delete a ICUMutex.  Destroy the underlying OS Platform mutex.
 *           Does not touch the linked list of ICU Mutexes.
 */
static void umtx_dt(ICUMutex *m) {
    if (pMutexDestroyFn != NULL) {
        (*pMutexDestroyFn)(gMutexContext, &m->userMutex);
        m->userMutex = NULL;
    } else {
        PLATFORM_MUTEX_DESTROY(&m->platformMutex);
    }

    if (m->heapAllocated) {
        uprv_free(m);
    }
}
    

U_CAPI void  U_EXPORT2
umtx_init(UMTX *mutex) {
    ICUMutex *m = NULL;
    void *originalValue;

    if (*mutex != NULL) {
        /* Mutex is already initialized.  
         * Multiple umtx_init()s of a UMTX by other ICU code are explicitly permitted.
         */
        return;
    }
#if defined(POSIX)
    if (mutex == &globalUMTX) {
        m = &globalMutex;
    }
#endif

    m = umtx_ct(m);
    originalValue = SYNC_COMPARE_AND_SWAP(mutex, NULL, m);
    if (originalValue != NULL) {
        umtx_dt(m);
        return;
    }

    m->owner = mutex;

    /* Hook the new mutex into the list of all ICU mutexes, so that we can find and
     * delete it for u_cleanup().
     */

    umtx_lock(NULL);
    m->next = mutexListHead;
    mutexListHead = m;
    umtx_unlock(NULL);
    return;
}


/*
 *  umtx_destroy.    Un-initialize a mutex, releasing any underlying resources
 *                   that it may be holding.  Destroying an already destroyed
 *                   mutex has no effect.  Unlike umtx_init(), this function
 *                   is not thread safe;  two threads must not concurrently try to
 *                   destroy the same mutex.
 */                  
U_CAPI void  U_EXPORT2
umtx_destroy(UMTX *mutex) {
    ICUMutex *m;
    
    /* No one should be deleting the global ICU mutex. 
     *   (u_cleanup() does delete it, but does so explicitly, not by passing NULL)
     */
    U_ASSERT(mutex != NULL);
    if (mutex == NULL) { 
        return;
    }
    
    m = (ICUMutex *)*mutex;
    if (m == NULL) {  /* Mutex not initialized, or already destroyed.  */
        return;
    }

    U_ASSERT(m->owner == mutex);
    if (m->owner != mutex) {
        return;
    }

    /* Remove this mutex from the linked list of mutexes.  */
    umtx_lock(NULL);
    if (mutexListHead == m) {
        mutexListHead = m->next;
    } else {
        ICUMutex *prev;
        for (prev = mutexListHead; prev!=NULL && prev->next!=m; prev = prev->next);
            /*  Empty for loop body */
        if (prev != NULL) {
            prev->next = m->next;
        }
    }
    umtx_unlock(NULL);

    umtx_dt(m);        /* Delete the internal ICUMutex   */
    *mutex = NULL;     /* Clear the caller's UMTX        */
}



U_CAPI void U_EXPORT2 
u_setMutexFunctions(const void *context, UMtxInitFn *i, UMtxFn *d, UMtxFn *l, UMtxFn *u,
                    UErrorCode *status) {
    if (U_FAILURE(*status)) {
        return;
    }

    /* Can not set a mutex function to a NULL value  */
    if (i==NULL || d==NULL || l==NULL || u==NULL) {
        *status = U_ILLEGAL_ARGUMENT_ERROR;
        return;
    }

    /* If ICU is not in an initial state, disallow this operation. */
    if (cmemory_inUse()) {
        *status = U_INVALID_STATE_ERROR;
        return;
    }
    
    /* Kill any existing global mutex.  POSIX platforms have a global mutex
     * even before any other part of ICU is initialized.
     */
    umtx_destroy(&globalUMTX);

    /* Swap in the mutex function pointers.  */
    pMutexInitFn    = i;
    pMutexDestroyFn = d;
    pMutexLockFn    = l;
    pMutexUnlockFn  = u;
    gMutexContext   = context;

#if defined (POSIX) 
    /* POSIX platforms must have a pre-initialized global mutex 
     * to allow other mutexes to initialize safely. */
    umtx_init(&globalUMTX);
#endif
}


/*   synchronized compare and swap function, for use when OS or compiler built-in
 *   equivalents aren't available.
 *
 *   This operation relies on the ICU global mutex for synchronization.
 *
 *   There are two cases where this function can be entered when the global mutex is not
 *   yet initialized - at the end  u_cleanup(), and at the end of u_setMutexFunctions, both
 *   of which re-init the global mutex.  But neither function is thread-safe, so the lack of
 *   synchronization at these points doesn't matter.
 */
static void *mutexed_compare_and_swap(void **dest, void *newval, void *oldval) {
    void *temp;
    UBool needUnlock = FALSE;

    if (globalUMTX != NULL) {
        umtx_lock(&globalUMTX);
        needUnlock = TRUE;
    }

    temp = *dest;
    if (temp == oldval) {
        *dest = newval;
    }
    
    if (needUnlock) {
        umtx_unlock(&globalUMTX);
    }
    return temp;
}



/*-----------------------------------------------------------------
 *
 *  Atomic Increment and Decrement
 *     umtx_atomic_inc
 *     umtx_atomic_dec
 *
 *----------------------------------------------------------------*/

/* Pointers to user-supplied inc/dec functions.  Null if no funcs have been set.  */
static UMtxAtomicFn  *pIncFn = NULL;
static UMtxAtomicFn  *pDecFn = NULL;
static const void *gIncDecContext  = NULL;

static UMTX    gIncDecMutex = NULL;

U_CAPI int32_t U_EXPORT2
umtx_atomic_inc(int32_t *p)  {
    int32_t retVal;
    if (pIncFn) {
        retVal = (*pIncFn)(gIncDecContext, p);
    } else {
        #if defined (U_WINDOWS) && ICU_USE_THREADS == 1
            retVal = InterlockedIncrement((LONG*)p);
        #elif defined(USE_MAC_OS_ATOMIC_INCREMENT)
            retVal = OSAtomicIncrement32Barrier(p);
        #elif (U_HAVE_GCC_ATOMICS == 1)
            retVal = __sync_add_and_fetch(p, 1);
        #elif defined (POSIX) && ICU_USE_THREADS == 1
            umtx_lock(&gIncDecMutex);
            retVal = ++(*p);
            umtx_unlock(&gIncDecMutex);
        #else
            /* Unknown Platform, or ICU thread support compiled out. */
            retVal = ++(*p);
        #endif
    }
    return retVal;
}

U_CAPI int32_t U_EXPORT2
umtx_atomic_dec(int32_t *p) {
    int32_t retVal;
    if (pDecFn) {
        retVal = (*pDecFn)(gIncDecContext, p);
    } else {
        #if defined (U_WINDOWS) && ICU_USE_THREADS == 1
            retVal = InterlockedDecrement((LONG*)p);
        #elif defined(USE_MAC_OS_ATOMIC_INCREMENT)
            retVal = OSAtomicDecrement32Barrier(p);
        #elif (U_HAVE_GCC_ATOMICS == 1)
            retVal = __sync_sub_and_fetch(p, 1);
        #elif defined (POSIX) && ICU_USE_THREADS == 1
            umtx_lock(&gIncDecMutex);
            retVal = --(*p);
            umtx_unlock(&gIncDecMutex);
        #else
            /* Unknown Platform, or ICU thread support compiled out. */
            retVal = --(*p);
        #endif
    }
    return retVal;
}



U_CAPI void U_EXPORT2
u_setAtomicIncDecFunctions(const void *context, UMtxAtomicFn *ip, UMtxAtomicFn *dp,
                                UErrorCode *status) {
    if (U_FAILURE(*status)) {
        return;
    }
    /* Can not set a mutex function to a NULL value  */
    if (ip==NULL || dp==NULL) {
        *status = U_ILLEGAL_ARGUMENT_ERROR;
        return;
    }
    /* If ICU is not in an initial state, disallow this operation. */
    if (cmemory_inUse()) {
        *status = U_INVALID_STATE_ERROR;
        return;
    }

    pIncFn = ip;
    pDecFn = dp;
    gIncDecContext = context;

#if !U_RELEASE
    {
        int32_t   testInt = 0;
        U_ASSERT(umtx_atomic_inc(&testInt) == 1);     /* Sanity Check.    Do the functions work at all? */
        U_ASSERT(testInt == 1);
        U_ASSERT(umtx_atomic_dec(&testInt) == 0);
        U_ASSERT(testInt == 0);
    }
#endif
}



/*
 *  Mutex Cleanup Function
 *
 *      Destroy the global mutex(es), and reset the mutex function callback pointers.
 */
U_CFUNC UBool umtx_cleanup(void) {
    ICUMutex *thisMutex = NULL;
    ICUMutex *nextMutex = NULL;

    /* Extra, do-nothing function call to suppress compiler warnings on platforms where
     *   mutexed_compare_and_swap is not otherwise used.  */
    mutexed_compare_and_swap(&globalUMTX, NULL, NULL);

    /* Delete all of the ICU mutexes.  Do the global mutex last because it is used during
     * the umtx_destroy operation of other mutexes.
     */
    for (thisMutex=mutexListHead; thisMutex!=NULL; thisMutex=nextMutex) {
        UMTX *umtx = thisMutex->owner;
        nextMutex = thisMutex->next;
        U_ASSERT(*umtx = (void *)thisMutex);
        if (umtx != &globalUMTX) {
            umtx_destroy(umtx);
        }
    }
    umtx_destroy(&globalUMTX);
        
    pMutexInitFn    = NULL;
    pMutexDestroyFn = NULL;
    pMutexLockFn    = NULL;
    pMutexUnlockFn  = NULL;
    gMutexContext   = NULL;
    pIncFn          = NULL;
    pDecFn          = NULL;
    gIncDecContext  = NULL;
    gIncDecMutex    = NULL;

#if defined (POSIX) 
    /* POSIX platforms must come out of u_cleanup() with a functioning global mutex 
     * to permit the safe resumption of use of ICU in multi-threaded environments. 
     */
    umtx_init(&globalUMTX);
#endif
    return TRUE;
}


