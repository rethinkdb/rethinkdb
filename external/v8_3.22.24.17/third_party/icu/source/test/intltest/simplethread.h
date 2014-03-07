/********************************************************************
 * COPYRIGHT: 
 * Copyright (c) 1997-2009, International Business Machines Corporation and
 * others. All Rights Reserved.
 ********************************************************************/

#ifndef SIMPLETHREAD_H
#define SIMPLETHREAD_H

#include "mutex.h"

class U_EXPORT SimpleThread
{
public:
    SimpleThread();
    virtual  ~SimpleThread();
    int32_t   start(void);        // start the thread
    UBool     isRunning();        // return true if a started thread has exited.

    virtual void run(void) = 0;   // Override this to provide the code to run
                                  //   in the thread.
    void *fImplementation;

public:
    static void sleep(int32_t millis); // probably shouldn't go here but oh well.
    static void errorFunc();      // Empty function, provides a single convenient place
                                  //   to break on errors.
};

#endif

