/*
*******************************************************************************
*
*   Copyright (C) 2008-2010, International Business Machines
*   Corporation and others.  All Rights Reserved.
*
*******************************************************************************
*   file name:  mutex.cpp
*   encoding:   US-ASCII
*   tab size:   8 (not used)
*   indentation:4
*/

#include "unicode/utypes.h"
#include "mutex.h"

U_NAMESPACE_BEGIN

void *SimpleSingleton::getInstance(InstantiatorFn *instantiator, const void *context,
                                   void *&duplicate,
                                   UErrorCode &errorCode) {
    duplicate=NULL;
    if(U_FAILURE(errorCode)) {
        return NULL;
    }
    void *instance;
    UMTX_CHECK(NULL, fInstance, instance);
    if(instance!=NULL) {
        return instance;
    } else {
        instance=instantiator(context, errorCode);
        Mutex mutex;
        if(fInstance==NULL && U_SUCCESS(errorCode)) {
            fInstance=instance;
        } else {
            duplicate=instance;
        }
        return fInstance;
    }
}

void *TriStateSingleton::getInstance(InstantiatorFn *instantiator, const void *context,
                                     void *&duplicate,
                                     UErrorCode &errorCode) {
    duplicate=NULL;
    if(U_FAILURE(errorCode)) {
        return NULL;
    }
    int8_t haveInstance;
    UMTX_CHECK(NULL, fHaveInstance, haveInstance);
    if(haveInstance>0) {
        return fInstance;  // instance was created
    } else if(haveInstance<0) {
        errorCode=fErrorCode;  // instance creation failed
        return NULL;
    } else /* haveInstance==0 */ {
        void *instance=instantiator(context, errorCode);
        Mutex mutex;
        if(fHaveInstance==0) {
            if(U_SUCCESS(errorCode)) {
                fInstance=instance;
                instance=NULL;
                fHaveInstance=1;
            } else {
                fErrorCode=errorCode;
                fHaveInstance=-1;
            }
        } else {
            errorCode=fErrorCode;
        }
        duplicate=instance;
        return fInstance;
    }
}

void TriStateSingleton::reset() {
    fInstance=NULL;
    fErrorCode=U_ZERO_ERROR;
    fHaveInstance=0;
}

#if UCONFIG_NO_SERVICE

/* If UCONFIG_NO_SERVICE, then there is no invocation of Mutex elsewhere in
   common, so add one here to force an export */
static Mutex *aMutex = 0;

/* UCONFIG_NO_SERVICE */
#endif

U_NAMESPACE_END
