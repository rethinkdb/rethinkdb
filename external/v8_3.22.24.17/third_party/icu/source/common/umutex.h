/*
**********************************************************************
*   Copyright (C) 1997-2008, International Business Machines
*   Corporation and others.  All Rights Reserved.
**********************************************************************
*
* File UMUTEX.H
*
* Modification History:
*
*   Date        Name        Description
*   04/02/97  aliu        Creation.
*   04/07/99  srl         rewrite - C interface, multiple mutices
*   05/13/99  stephen     Changed to umutex (from cmutex)
******************************************************************************
*/

#ifndef UMUTEX_H
#define UMUTEX_H

#include "unicode/utypes.h"
#include "unicode/uclean.h"  


/* APP_NO_THREADS is an old symbol. We'll honour it if present. */
#ifdef APP_NO_THREADS
# define ICU_USE_THREADS 0
#endif

/* ICU_USE_THREADS
 *
 *   Allows thread support (use of mutexes) to be compiled out of ICU.
 *   Default: use threads.
 *   Even with thread support compiled out, applications may override the
 *   (empty) mutex implementation with the u_setMutexFunctions() functions.
 */ 
#ifndef ICU_USE_THREADS
# define ICU_USE_THREADS 1
#endif

/**
 * By default assume that we are on a machine with a weak memory model,
 * and the double check lock won't work reliably.
 */
#if !defined(UMTX_STRONG_MEMORY_MODEL)
#define UMTX_STRONG_MEMORY_MODEL 0
#endif

/**
 * \def UMTX_CHECK
 * Encapsulates a safe check of an expression 
 * for use with double-checked lazy inititialization.
 * On CPUs with weak memory models, this must use memory fence instructions
 * or mutexes.
 * The expression must involve only a  _single_ variable, typically
 *    a possibly null pointer or a boolean that indicates whether some service
 *    is initialized or not.
 * The setting of the variable involved in the test must be the last step of
 *    the initialization process.
 *
 * 
 * @internal
 */
#if UMTX_STRONG_MEMORY_MODEL

#define UMTX_CHECK(pMutex, expression, result) \
    (result)=(expression)

#else

#define UMTX_CHECK(pMutex, expression, result) \
    umtx_lock(pMutex); \
    (result)=(expression); \
    umtx_unlock(pMutex)

#endif

/*
 * Code within ICU that accesses shared static or global data should
 * instantiate a Mutex object while doing so.  The unnamed global mutex
 * is used throughout ICU, so keep locking short and sweet.
 *
 * For example:
 *
 * void Function(int arg1, int arg2)
 * {
 *   static Object* foo;     // Shared read-write object
 *   umtx_lock(NULL);        // Lock the ICU global mutex
 *   foo->Method();
 *   umtx_unlock(NULL);
 * }
 *
 * an alternative C++ mutex API is defined in the file common/mutex.h
 */

/* Lock a mutex. 
 * @param mutex The given mutex to be locked.  Pass NULL to specify
 *              the global ICU mutex.  Recursive locks are an error
 *              and may cause a deadlock on some platforms.
 */
U_CAPI void U_EXPORT2 umtx_lock   ( UMTX* mutex ); 

/* Unlock a mutex. Pass in NULL if you want the single global
   mutex. 
 * @param mutex The given mutex to be unlocked.  Pass NULL to specify
 *              the global ICU mutex.
 */
U_CAPI void U_EXPORT2 umtx_unlock ( UMTX* mutex );

/* Initialize a mutex. Use it this way:
   umtx_init( &aMutex ); 
 * ICU Mutexes do not need explicit initialization before use.  Use of this
 *   function is not necessary.
 * Initialization of an already initialized mutex has no effect, and is safe to do.
 * Initialization of mutexes is thread safe.  Two threads can concurrently 
 *   initialize the same mutex without causing problems.
 * @param mutex The given mutex to be initialized
 */
U_CAPI void U_EXPORT2 umtx_init   ( UMTX* mutex );

/* Destroy a mutex. This will free the resources of a mutex.
 * Use it this way:
 *   umtx_destroy( &aMutex ); 
 * Destroying an already destroyed mutex has no effect, and causes no problems.
 * This function is not thread safe.  Two threads must not attempt to concurrently
 *   destroy the same mutex.
 * @param mutex The given mutex to be destroyed.
 */
U_CAPI void U_EXPORT2 umtx_destroy( UMTX *mutex );



/*
 * Atomic Increment and Decrement of an int32_t value.
 *
 * Return Values:
 *   If the result of the operation is zero, the return zero.
 *   If the result of the operation is not zero, the sign of returned value
 *      is the same as the sign of the result, but the returned value itself may
 *      be different from the result of the operation.
 */
U_CAPI int32_t U_EXPORT2 umtx_atomic_inc(int32_t *);
U_CAPI int32_t U_EXPORT2 umtx_atomic_dec(int32_t *);


#endif /*_CMUTEX*/
/*eof*/



