/********************************************************************
 * COPYRIGHT: 
 * Copyright (c) 1999-2009, International Business Machines Corporation and
 * others. All Rights Reserved.
 ********************************************************************/

#if defined(hpux)
# ifndef _INCLUDE_POSIX_SOURCE
#  define _INCLUDE_POSIX_SOURCE
# endif
#endif

#include "simplethread.h"

#include "unicode/utypes.h"
#include "unicode/ustring.h"
#include "umutex.h"
#include "cmemory.h"
#include "cstring.h"
#include "uparse.h"
#include "unicode/resbund.h"
#include "unicode/udata.h"
#include "unicode/uloc.h"
#include "unicode/locid.h"
#include "putilimp.h"

#include <stdio.h>
#include <string.h>
#include <ctype.h>    // tolower, toupper

#if !defined(U_WINDOWS) && !defined(XP_MAC) && !defined(U_RHAPSODY)
#define POSIX 1
#endif

/* Needed by z/OS to get usleep */
#if defined(OS390)
#define __DOT1 1
#define __UU
#define _XOPEN_SOURCE_EXTENDED 1
#ifndef _XPG4_2
#define _XPG4_2
#endif
#include <unistd.h>
/*#include "platform_xopen_source_extended.h"*/
#endif

#if defined(POSIX) || defined(U_SOLARIS) || defined(U_AIX) || defined(U_HPUX)
#define HAVE_IMP

#if (ICU_USE_THREADS == 1)
#include <pthread.h>
#endif

#if defined(__hpux) && defined(HPUX_CMA)
# if defined(read)  // read being defined as cma_read causes trouble with iostream::read
#  undef read
# endif
#endif

/* Define __EXTENSIONS__ for Solaris and old friends in strict mode. */
#ifndef __EXTENSIONS__
#define __EXTENSIONS__
#endif

#if defined(OS390)
#include <sys/types.h>
#endif

#if !defined(OS390)
#include <signal.h>
#endif

/* Define _XPG4_2 for Solaris and friends. */
#ifndef _XPG4_2
#define _XPG4_2
#endif

/* Define __USE_XOPEN_EXTENDED for Linux and glibc. */
#ifndef __USE_XOPEN_EXTENDED
#define __USE_XOPEN_EXTENDED 
#endif

/* Define _INCLUDE_XOPEN_SOURCE_EXTENDED for HP/UX (11?). */
#ifndef _INCLUDE_XOPEN_SOURCE_EXTENDED
#define _INCLUDE_XOPEN_SOURCE_EXTENDED
#endif

#include <unistd.h>

#endif
/* HPUX */
#ifdef sleep
#undef sleep
#endif


#if (ICU_USE_THREADS==0)
    SimpleThread::SimpleThread()
    {}

    SimpleThread::~SimpleThread()
    {}

    int32_t 
    SimpleThread::start()
    { return -1; }

    void 
    SimpleThread::run()
    {}

    void 
    SimpleThread::sleep(int32_t millis)
    {}

    UBool  
    SimpleThread::isRunning() {
        return FALSE;
    }
#else

#include "unicode/putil.h"

/* for mthreadtest*/
#include "unicode/numfmt.h"
#include "unicode/choicfmt.h"
#include "unicode/msgfmt.h"
#include "unicode/locid.h"
#include "unicode/ucol.h"
#include "unicode/calendar.h"
#include "ucaconf.h"

#ifdef U_WINDOWS
#define HAVE_IMP

#   define VC_EXTRALEAN
#   define WIN32_LEAN_AND_MEAN
#   define NOUSER
#   define NOSERVICE
#   define NOIME
#   define NOMCX
#include <windows.h>
#include <process.h>

//-----------------------------------------------------------------------------------
//
//   class SimpleThread   Windows Implementation
//
//-----------------------------------------------------------------------------------
struct Win32ThreadImplementation
{
    HANDLE         fHandle;
    unsigned int   fThreadID;
};


extern "C" unsigned int __stdcall SimpleThreadProc(void *arg)
{
    ((SimpleThread*)arg)->run();
    return 0;
}

SimpleThread::SimpleThread()
:fImplementation(0)
{
    Win32ThreadImplementation *imp = new Win32ThreadImplementation;
    imp->fHandle = 0;
    fImplementation = imp;
}

SimpleThread::~SimpleThread()
{
    // Destructor.  Because we start the thread running with _beginthreadex(),
    //              we own the Windows HANDLE for the thread and must 
    //              close it here.
    Win32ThreadImplementation *imp = (Win32ThreadImplementation*)fImplementation;
    if (imp != 0) {
        if (imp->fHandle != 0) {
            CloseHandle(imp->fHandle);
            imp->fHandle = 0;
        }
    }
    delete (Win32ThreadImplementation*)fImplementation;
}

int32_t SimpleThread::start()
{
    Win32ThreadImplementation *imp = (Win32ThreadImplementation*)fImplementation;
    if(imp->fHandle != NULL) {
        // The thread appears to have already been started.
        //   This is probably an error on the part of our caller.
        return -1;
    }

    imp->fHandle = (HANDLE) _beginthreadex(
        NULL,                                 // Security    
        0x20000,                              // Stack Size 
        SimpleThreadProc,                     // Function to Run
        (void *)this,                         // Arg List
        0,                                    // initflag.  Start running, not suspended
        &imp->fThreadID                       // thraddr
        );

    if (imp->fHandle == 0) {
        // An error occured
        int err = errno;
        if (err == 0) {
            err = -1;
        }
        return err;
    }
    return 0;
}


UBool  SimpleThread::isRunning() {
    //
    //  Test whether the thread associated with the SimpleThread object is
    //    still actually running.  
    //
    //  NOTE:  on Win64 on Itanium processors, a crashes
    //    occur if the main thread of a process exits concurrently with some
    //    other thread(s) exiting.  To avoid the possibility, we wait until the
    //    OS indicates that all threads have  terminated, rather than waiting
    //    only until the end of the user's Run function has been reached.
    //
    //   I don't know whether the crashes represent a Windows bug, or whether
    //    main() programs are supposed to have to wait for their threads.
    //
    Win32ThreadImplementation *imp = (Win32ThreadImplementation*)fImplementation;
    
    bool      success;
    DWORD     threadExitCode;

    if (imp->fHandle == 0) {
        // No handle, thread must not be running.
        return FALSE;
    }
    success = GetExitCodeThread(imp->fHandle,   &threadExitCode) != 0;
    if (! success) {
        // Can't get status, thread must not be running.
        return FALSE;
    }
    return (threadExitCode == STILL_ACTIVE);
}


void SimpleThread::sleep(int32_t millis)
{
    ::Sleep(millis);
}

//-----------------------------------------------------------------------------------
//
//   class SimpleThread   NULL  Implementation
//
//-----------------------------------------------------------------------------------
#elif defined XP_MAC

// since the Mac has no preemptive threading (at least on MacOS 8), only
// cooperative threading, threads are a no-op.  We have no yield() calls
// anywhere in the ICU, so we are guaranteed to be thread-safe.

#define HAVE_IMP

SimpleThread::SimpleThread()
{}

SimpleThread::~SimpleThread()
{}

int32_t 
SimpleThread::start()
{ return 0; }

void 
SimpleThread::run()
{}

void 
SimpleThread::sleep(int32_t millis)
{}

UBool  
SimpleThread::isRunning() {
    return FALSE;
}

#endif


//-----------------------------------------------------------------------------------
//
//   class SimpleThread   POSIX implementation
//
//        A note on the POSIX vs the Windows implementations of this class..
//        On Windows, the main thread must verify that other threads have finished
//        before exiting, or crashes occasionally occur.  (Seen on Itanium Win64 only)
//        The function SimpleThread::isRunning() is used for this purpose.
//
//        On POSIX, there is NO reliable non-blocking mechanism to determine
//        whether a thread has exited.  pthread_kill(thread, 0) almost works,
//        but the system can recycle thread ids immediately, so seeing that a
//        thread exists with this call could mean that the original thread has
//        finished and a new one started with the same ID.  Useless.
//
//        So we need to do the check with user code, by setting a flag just before
//        the thread function returns.  A technique that is guaranteed to fail
//        on Windows, because it indicates that the thread is done before all
//        system level cleanup has happened.
//
//-----------------------------------------------------------------------------------
#if defined(POSIX)||defined(U_SOLARIS)||defined(U_AIX)||defined(U_HPUX)
#define HAVE_IMP

struct PosixThreadImplementation
{
    pthread_t        fThread;
    UBool            fRunning;
    UBool            fRan;          // True if the thread was successfully started
};

extern "C" void* SimpleThreadProc(void *arg)
{
    // This is the code that is run in the new separate thread.
    SimpleThread *This = (SimpleThread *)arg;
    This->run();      // Run the user code.

    // The user function has returned.  Set the flag indicating that this thread
    // is done.  Need a mutex for memory barrier purposes only, so that other thread
    //   will reliably see that the flag has changed.
    PosixThreadImplementation *imp = (PosixThreadImplementation*)This->fImplementation;
    umtx_lock(NULL);
    imp->fRunning = FALSE;
    umtx_unlock(NULL);
    return 0;
}

SimpleThread::SimpleThread() 
{
    PosixThreadImplementation *imp = new PosixThreadImplementation;
    imp->fRunning   = FALSE;
    imp->fRan       = FALSE;
    fImplementation = imp;
}

SimpleThread::~SimpleThread()
{
    PosixThreadImplementation *imp = (PosixThreadImplementation*)fImplementation;
    if (imp->fRan) {
        pthread_join(imp->fThread, NULL);
    }
    delete imp;
    fImplementation = (void *)0xdeadbeef;
}

int32_t SimpleThread::start()
{
    int32_t        rc;
    static pthread_attr_t attr;
    static UBool attrIsInitialized = FALSE;

    PosixThreadImplementation *imp = (PosixThreadImplementation*)fImplementation;
    imp->fRunning = TRUE;
    imp->fRan     = TRUE;

#ifdef HPUX_CMA
    if (attrIsInitialized == FALSE) {
        rc = pthread_attr_create(&attr);
        attrIsInitialized = TRUE;
    }
    rc = pthread_create(&(imp->fThread),attr,&SimpleThreadProc,(void*)this);
#else
    if (attrIsInitialized == FALSE) {
        rc = pthread_attr_init(&attr);
#if defined(OS390)
        {
            int detachstate = 0;  // jdc30: detach state of zero causes
                                  //threads created with this attr to be in
                                  //an undetached state.  An undetached
                                  //thread will keep its resources after
                                  //termination.
            pthread_attr_setdetachstate(&attr, &detachstate);
        }
#else
        // pthread_attr_setdetachstate(&attr, PTHREAD_CREATE_DETACHED);
        pthread_attr_setdetachstate(&attr, PTHREAD_CREATE_JOINABLE);
#endif
        attrIsInitialized = TRUE;
    }
    rc = pthread_create(&(imp->fThread),&attr,&SimpleThreadProc,(void*)this);
#endif
    
    if (rc != 0) {
        // some kind of error occured, the thread did not start.
        imp->fRan     = FALSE;
        imp->fRunning = FALSE;
    }

    return rc;
}


UBool  
SimpleThread::isRunning() {
    // Note:  Mutex functions are used here not for synchronization, 
    //        but to force memory barriors to exist, to ensure that one thread
    //        can see changes made by another when running on processors
    //        with memory models having weak coherency.
    PosixThreadImplementation *imp = (PosixThreadImplementation*)fImplementation;
    umtx_lock(NULL);
    UBool retVal = imp->fRunning;
    umtx_unlock(NULL);
    return retVal;
}


void SimpleThread::sleep(int32_t millis)
{
#ifdef U_SOLARIS
    sigignore(SIGALRM);
#endif

#ifdef HPUX_CMA
    cma_sleep(millis/100);
#elif defined(U_HPUX) || defined(OS390)
    millis *= 1000;
    while(millis >= 1000000) {
        usleep(999999);
        millis -= 1000000;
    }
    if(millis > 0) {
        usleep(millis);
    }
#else
    usleep(millis * 1000);
#endif
}

#endif
// end POSIX


#ifndef HAVE_IMP
#error  No implementation for threads! Cannot test.
0 = 216; //die
#endif

//-------------------------------------------------------------------------------------------
//
// class ThreadWithStatus - a thread that we can check the status and error condition of
//
//-------------------------------------------------------------------------------------------
class ThreadWithStatus : public SimpleThread
{
public:
    UBool  getError() { return (fErrors > 0); } 
    UBool  getError(UnicodeString& fillinError) { fillinError = fErrorString; return (fErrors > 0); } 
    virtual ~ThreadWithStatus(){}
protected:
    ThreadWithStatus() :  fErrors(0) {}
    void error(const UnicodeString &error) { 
        fErrors++; fErrorString = error; 
        SimpleThread::errorFunc();  
    }
    void error() { error("An error occured."); }
private:
    int32_t fErrors;
    UnicodeString fErrorString;
};

#endif // ICU_USE_THREADS
