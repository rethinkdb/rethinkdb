/********************************************************************
 * COPYRIGHT: 
 * Copyright (c) 1997-2004, International Business Machines Corporation and
 * others. All Rights Reserved.
 ********************************************************************/


#ifndef MULTITHREADTEST_H
#define MULTITHREADTEST_H

#include "intltest.h"
#include "mutex.h"



/**
 * Tests actual threading
 **/
class MultithreadTest : public IntlTest
{
public:
    MultithreadTest();
    virtual ~MultithreadTest();
    
    void runIndexedTest( int32_t index, UBool exec, const char* &name, char* par = NULL );

    /**
     * test that threads even work
     **/
    void TestThreads(void);
    /**
     * test that mutexes work 
     **/
    void TestMutex(void);
#if !UCONFIG_NO_FORMATTING
    /**
     * test that intl functions work in a multithreaded context
     **/
    void TestThreadedIntl(void);
#endif
  void TestCollators(void);
  void TestString();
};

#endif

