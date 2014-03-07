/*
******************************************************************************
*
*   Copyright (C) 1997-2010, International Business Machines
*   Corporation and others.  All Rights Reserved.
*
******************************************************************************
*/
//----------------------------------------------------------------------------
// File:     mutex.h
//
// Lightweight C++ wrapper for umtx_ C mutex functions
//
// Author:   Alan Liu  1/31/97
// History:
// 06/04/97   helena         Updated setImplementation as per feedback from 5/21 drop.
// 04/07/1999  srl               refocused as a thin wrapper
//
//----------------------------------------------------------------------------
#ifndef MUTEX_H
#define MUTEX_H

#include "unicode/utypes.h"
#include "unicode/uobject.h"
#include "umutex.h"

U_NAMESPACE_BEGIN

//----------------------------------------------------------------------------
// Code within that accesses shared static or global data should
// should instantiate a Mutex object while doing so. You should make your own 
// private mutex where possible.

// For example:
// 
// UMTX myMutex;
// 
// void Function(int arg1, int arg2)
// {
//    static Object* foo;     // Shared read-write object
//    Mutex mutex(&myMutex);  // or no args for the global lock
//    foo->Method();
//    // When 'mutex' goes out of scope and gets destroyed here, the lock is released
// }
//
// Note:  Do NOT use the form 'Mutex mutex();' as that merely forward-declares a function
//        returning a Mutex. This is a common mistake which silently slips through the
//        compiler!!
//

class U_COMMON_API Mutex : public UMemory {
public:
  inline Mutex(UMTX *mutex = NULL);
  inline ~Mutex();

private:
  UMTX   *fMutex;

  Mutex(const Mutex &other); // forbid copying of this class
  Mutex &operator=(const Mutex &other); // forbid copying of this class
};

inline Mutex::Mutex(UMTX *mutex)
  : fMutex(mutex)
{
  umtx_lock(fMutex);
}

inline Mutex::~Mutex()
{
  umtx_unlock(fMutex);
}

// common code for singletons ---------------------------------------------- ***

/**
 * Function pointer for the instantiator parameter of
 * SimpleSingleton::getInstance() and TriStateSingleton::getInstance().
 * The function creates some object, optionally using the context parameter.
 * The function need not check for U_FAILURE(errorCode).
 */
typedef void *InstantiatorFn(const void *context, UErrorCode &errorCode);

/**
 * Singleton struct with shared instantiation/mutexing code.
 * Simple: Does not remember if a previous instantiation failed.
 * Best used if the instantiation can really only fail with an out-of-memory error,
 * otherwise use a TriStateSingleton.
 * Best used via SimpleSingletonWrapper or similar.
 * Define a static SimpleSingleton instance via the STATIC_SIMPLE_SINGLETON macro.
 */
struct SimpleSingleton {
    void *fInstance;

    /**
     * Returns the singleton instance, or NULL if it could not be created.
     * Calls the instantiator with the context if the instance has not been
     * created yet. In a race condition, the duplicate may not be NULL.
     * The caller must delete the duplicate.
     * The caller need not initialize the duplicate before the call.
     */
    void *getInstance(InstantiatorFn *instantiator, const void *context,
                      void *&duplicate,
                      UErrorCode &errorCode);
    /**
     * Resets the fields. The caller must have deleted the singleton instance.
     * Not mutexed.
     * Call this from a cleanup function.
     */
    void reset() { fInstance=NULL; }
};

#define STATIC_SIMPLE_SINGLETON(name) static SimpleSingleton name={ NULL }

/**
 * Handy wrapper for an SimpleSingleton.
 * Intended for temporary use on the stack, to make the SimpleSingleton easier to deal with.
 * Takes care of the duplicate deletion and type casting.
 */
template<typename T>
class SimpleSingletonWrapper {
public:
    SimpleSingletonWrapper(SimpleSingleton &s) : singleton(s) {}
    void deleteInstance() {
        delete (T *)singleton.fInstance;
        singleton.reset();
    }
    T *getInstance(InstantiatorFn *instantiator, const void *context,
                   UErrorCode &errorCode) {
        void *duplicate;
        T *instance=(T *)singleton.getInstance(instantiator, context, duplicate, errorCode);
        delete (T *)duplicate;
        return instance;
    }
private:
    SimpleSingleton &singleton;
};

/**
 * Singleton struct with shared instantiation/mutexing code.
 * Tri-state: Instantiation succeeded/failed/not attempted yet.
 * Best used via TriStateSingletonWrapper or similar.
 * Define a static TriStateSingleton instance via the STATIC_TRI_STATE_SINGLETON macro.
 */
struct TriStateSingleton {
    void *fInstance;
    UErrorCode fErrorCode;
    int8_t fHaveInstance;

    /**
     * Returns the singleton instance, or NULL if it could not be created.
     * Calls the instantiator with the context if the instance has not been
     * created yet. In a race condition, the duplicate may not be NULL.
     * The caller must delete the duplicate.
     * The caller need not initialize the duplicate before the call.
     * The singleton creation is only attempted once. If it fails,
     * the singleton will then always return NULL.
     */
    void *getInstance(InstantiatorFn *instantiator, const void *context,
                      void *&duplicate,
                      UErrorCode &errorCode);
    /**
     * Resets the fields. The caller must have deleted the singleton instance.
     * Not mutexed.
     * Call this from a cleanup function.
     */
    void reset();
};

#define STATIC_TRI_STATE_SINGLETON(name) static TriStateSingleton name={ NULL, U_ZERO_ERROR, 0 }

/**
 * Handy wrapper for an TriStateSingleton.
 * Intended for temporary use on the stack, to make the TriStateSingleton easier to deal with.
 * Takes care of the duplicate deletion and type casting.
 */
template<typename T>
class TriStateSingletonWrapper {
public:
    TriStateSingletonWrapper(TriStateSingleton &s) : singleton(s) {}
    void deleteInstance() {
        delete (T *)singleton.fInstance;
        singleton.reset();
    }
    T *getInstance(InstantiatorFn *instantiator, const void *context,
                   UErrorCode &errorCode) {
        void *duplicate;
        T *instance=(T *)singleton.getInstance(instantiator, context, duplicate, errorCode);
        delete (T *)duplicate;
        return instance;
    }
private:
    TriStateSingleton &singleton;
};

U_NAMESPACE_END

#endif //_MUTEX_
//eof
