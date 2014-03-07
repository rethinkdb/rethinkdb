//
//********************************************************************
//   Copyright (C) 2002, International Business Machines
//   Corporation and others.  All Rights Reserved.
//********************************************************************
//
// File threadtest.h
//
#ifndef ABSTRACTTHREADTEST_H
#define ABSTRACTTHREADTEST_H
//------------------------------------------------------------------------------
//
//   class AbstractThreadTest    Base class for threading tests.
//                         Use of this abstract base isolates the part of the
//                         program that nows how to spin up and control threads
//                         from the specific stuff being tested, and (hopefully)
//                         simplifies adding new threading tests for different parts
//                         of ICU.
//
//     Derived classes:    A running test will have exactly one instance of a
//                         derived class, which will persist for the duration of the
//                         test and be shared among all of the threads involved in
//                         the test.
//
//                         The constructor will be called in a single-threaded environment,
//                         and should set up any data that will need to persist for the
//                         duration.
//
//                         runOnce() will be called repeatedly by the working threads of
//                         the test in the full multi-threaded environment.
//
//                         check() will be called periodically in a single threaded
//                         environment, with the worker threads temporarily suspended between
//                         between calls to runOnce().  Do consistency checks here.
//
//------------------------------------------------------------------------------
class AbstractThreadTest {
public:
                     AbstractThreadTest() {};
    virtual         ~AbstractThreadTest() {};
    virtual void     check()   = 0;
    virtual void     runOnce() = 0;
};

#endif // ABSTRACTTHREADTEST_H

