//
//********************************************************************
//   Copyright (C) 2002-2005, International Business Machines
//   Corporation and others.  All Rights Reserved.
//********************************************************************
//
// File threadtest.cpp
//

#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include "unicode/utypes.h"
#include "unicode/uclean.h"
#include "umutex.h"
#include "threadtest.h"


//------------------------------------------------------------------------------
//
//   Factory functions for creating different test types.
//
//------------------------------------------------------------------------------
extern  AbstractThreadTest *createStringTest();
extern  AbstractThreadTest *createConvertTest();



//------------------------------------------------------------------------------
//
//   Windows specific code for starting threads
//
//------------------------------------------------------------------------------
#ifdef U_WINDOWS

#include "Windows.h"
#include "process.h"



typedef void (*ThreadFunc)(void *);

class ThreadFuncs           // This class isolates OS dependent threading
{                           //   functions from the rest of ThreadTest program.
public:
    static void            Sleep(int millis) {::Sleep(millis);};
    static void            startThread(ThreadFunc, void *param);
    static unsigned long   getCurrentMillis();
    static void            yield() {::Sleep(0);};
};

void ThreadFuncs::startThread(ThreadFunc func, void *param)
{
    unsigned long x;
    x = _beginthread(func, 0x10000, param);
    if (x == -1)
    {
        fprintf(stderr, "Error starting thread.  Errno = %d\n", errno);
        exit(-1);
    }
}

unsigned long ThreadFuncs::getCurrentMillis()
{
    return (unsigned long)::GetTickCount();
}




// #elif defined (POSIX) 
#else

//------------------------------------------------------------------------------
//
//   UNIX specific code for starting threads
//
//------------------------------------------------------------------------------
#include <pthread.h>
#include <unistd.h>
#include <errno.h>
#include <sched.h>
#include <sys/timeb.h>


extern "C" {


typedef void (*ThreadFunc)(void *);
typedef void *(*pthreadfunc)(void *);

class ThreadFuncs           // This class isolates OS dependent threading
{                           //   functions from the rest of ThreadTest program.
public:
    static void            Sleep(int millis);
    static void            startThread(ThreadFunc, void *param);
    static unsigned long   getCurrentMillis();
    static void            yield() {sched_yield();};
};

void ThreadFuncs::Sleep(int millis)
{
   int seconds = millis/1000;
   if (seconds <= 0) seconds = 1;
   ::sleep(seconds);
}


void ThreadFuncs::startThread(ThreadFunc func, void *param)
{
    unsigned long x;

    pthread_t tId;
    //thread_t tId;
#if defined(_HP_UX) && defined(XML_USE_DCE)
    x = pthread_create( &tId, pthread_attr_default,  (pthreadfunc)func,  param);
#else
    pthread_attr_t attr;
    pthread_attr_init(&attr);
    x = pthread_create( &tId, &attr,  (pthreadfunc)func,  param);
#endif
    if (x == -1)
    {
        fprintf(stderr, "Error starting thread.  Errno = %d\n", errno);
        exit(-1);
    }
}

unsigned long ThreadFuncs::getCurrentMillis() {
    timeb aTime;
    ftime(&aTime);
    return (unsigned long)(aTime.time*1000 + aTime.millitm);
}
}


// #else
// #error This platform is not supported
#endif



//------------------------------------------------------------------------------
//
//  struct runInfo     Holds the info extracted from the command line and data
//                     that is shared by all threads.
//                     There is only one of these, and it is static.
//                     During the test, the threads will access this info without
//                     any synchronization.
//
//------------------------------------------------------------------------------
const int MAXINFILES = 25;
struct RunInfo
{
    bool                quiet;
    bool                verbose;
    int                 numThreads;
    int                 totalTime;
    int                 checkTime;
    AbstractThreadTest *fTest;
    bool                stopFlag;
    bool                exitFlag;
    int32_t             runningThreads;
};


//------------------------------------------------------------------------------
//
//  struct threadInfo  Holds information specific to an individual thread.
//                     One of these is set up for each thread in the test.
//                     The main program monitors the threads by looking
//                     at the status stored in these structs.
//
//------------------------------------------------------------------------------
struct ThreadInfo
{
    bool    fHeartBeat;            // Set true by the thread each time it finishes
                                   //   a test.
    unsigned int     fCycles;      // Number of cycles completed.
    int              fThreadNum;   // Identifying number for this thread.
    ThreadInfo() {
        fHeartBeat = false;
        fCycles = 0;
        fThreadNum = -1;
    }
};


//
//------------------------------------------------------------------------------
//
//  Global Data
//
//------------------------------------------------------------------------------
RunInfo         gRunInfo;
ThreadInfo      *gThreadInfo;
UMTX            gStopMutex;        // Lets main thread suspend test threads.
UMTX            gInfoMutex;        // Synchronize access to data passed between
                                   //  worker threads and the main thread


//----------------------------------------------------------------------
//
//   parseCommandLine   Read through the command line, and save all
//                      of the options in the gRunInfo struct.
//
//                      Display the usage message if the command line
//                      is no good.
//
//                      Probably ought to be a member function of RunInfo.
//
//----------------------------------------------------------------------

void parseCommandLine(int argc, char **argv)
{
    gRunInfo.quiet = false;               // Set up defaults for run.
    gRunInfo.verbose = false;
    gRunInfo.numThreads = 2;
    gRunInfo.totalTime = 0;
    gRunInfo.checkTime = 10;

    try             // Use exceptions for command line syntax errors.
    {
        int argnum = 1;
        while (argnum < argc)
        {
            if      (strcmp(argv[argnum], "-quiet") == 0)
                gRunInfo.quiet = true;
            else if (strcmp(argv[argnum], "-verbose") == 0)
                gRunInfo.verbose = true;
            else if (strcmp(argv[argnum], "--help") == 0 ||
                    (strcmp(argv[argnum],     "?")      == 0)) {throw 1; }
                
            else if (strcmp(argv[argnum], "-threads") == 0)
            {
                ++argnum;
                if (argnum >= argc)
                    throw 1;
                gRunInfo.numThreads = atoi(argv[argnum]);
                if (gRunInfo.numThreads < 0)
                    throw 1;
            }
            else if (strcmp(argv[argnum], "-time") == 0)
            {
                ++argnum;
                if (argnum >= argc)
                    throw 1;
                gRunInfo.totalTime = atoi(argv[argnum]);
                if (gRunInfo.totalTime < 1)
                    throw 1;
            }
            else if (strcmp(argv[argnum], "-ctime") == 0)
            {
                ++argnum;
                if (argnum >= argc)
                    throw 1;
                gRunInfo.checkTime = atoi(argv[argnum]);
                if (gRunInfo.checkTime < 1)
                    throw 1;
            }
            else if (strcmp(argv[argnum], "string") == 0)
            {
                gRunInfo.fTest = createStringTest();
            }
            else if (strcmp(argv[argnum], "convert") == 0)
            {
                gRunInfo.fTest = createConvertTest();
            }
           else  
            {
                fprintf(stderr, "Unrecognized command line option.  Scanning \"%s\"\n",
                    argv[argnum]);
                throw 1;
            }
            argnum++;
        }
        // We've reached the end of the command line parameters.
        // Fail if no test name was specified.
        if (gRunInfo.fTest == NULL) {
            fprintf(stderr, "No test specified.\n");
            throw 1;
        }

    }
    catch (int)
    {
        fprintf(stderr, "usage:  threadtest [-threads nnn] [-time nnn] [-quiet] [-verbose] test-name\n"
            "     -quiet         Suppress periodic status display. \n"
            "     -verbose       Display extra messages. \n"
            "     -threads nnn   Number of threads.  Default is 2. \n"
            "     -time nnn      Total time to run, in seconds.  Default is forever.\n"
            "     -ctime nnn     Time between extra consistency checks, in seconds.  Default 10\n"
            "     testname       string | convert\n"
            );
        exit(1);
    }
}





//----------------------------------------------------------------------
//
//  threadMain   The main function for each of the swarm of test threads.
//               Run in a loop, executing the runOnce() test function each time.
//
//
//----------------------------------------------------------------------

extern "C" {

void threadMain (void *param)
{
    ThreadInfo   *thInfo = (ThreadInfo *)param;

    if (gRunInfo.verbose)
        printf("Thread #%d: starting\n", thInfo->fThreadNum);
    umtx_atomic_inc(&gRunInfo.runningThreads);

    //
    //
    while (true)
    {
        if (gRunInfo.verbose )
            printf("Thread #%d: starting loop\n", thInfo->fThreadNum);

        //
        //  If the main thread is asking us to wait, do so by locking gStopMutex
        //     which will block us, since the main thread will be holding it already.
        // 
        umtx_lock(&gInfoMutex);
        UBool stop = gRunInfo.stopFlag;  // Need mutex for processors with flakey memory models.
        umtx_unlock(&gInfoMutex);

        if (stop) {
            if (gRunInfo.verbose) {
                fprintf(stderr, "Thread #%d: suspending\n", thInfo->fThreadNum);
            }
            umtx_atomic_dec(&gRunInfo.runningThreads);
            while (gRunInfo.stopFlag) {
                umtx_lock(&gStopMutex);
                umtx_unlock(&gStopMutex);
            }
            umtx_atomic_inc(&gRunInfo.runningThreads);
            if (gRunInfo.verbose) {
                fprintf(stderr, "Thread #%d: restarting\n", thInfo->fThreadNum);
            }
        }

        //
        // The real work of the test happens here.
        //
        gRunInfo.fTest->runOnce();

        umtx_lock(&gInfoMutex);
        thInfo->fHeartBeat = true;
        thInfo->fCycles++;
        UBool exitNow = gRunInfo.exitFlag;
        umtx_unlock(&gInfoMutex);

        //
        // If main thread says it's time to exit, break out of the loop.
        //
        if (exitNow) {
            break;
        }
    }
            
    umtx_atomic_dec(&gRunInfo.runningThreads);

    // Returning will kill the thread.
    return;
}

}




//----------------------------------------------------------------------
//
//   main
//
//----------------------------------------------------------------------

int main (int argc, char **argv)
{
    //
    //  Parse the command line options, and create the specified kind of test.
    //
    parseCommandLine(argc, argv);


    //
    //  Fire off the requested number of parallel threads
    //

    if (gRunInfo.numThreads == 0)
        exit(0);

    gRunInfo.exitFlag = FALSE;
    gRunInfo.stopFlag = TRUE;      // Will cause the new threads to block 
    umtx_lock(&gStopMutex);

    gThreadInfo = new ThreadInfo[gRunInfo.numThreads];
    int threadNum;
    for (threadNum=0; threadNum < gRunInfo.numThreads; threadNum++)
    {
        gThreadInfo[threadNum].fThreadNum = threadNum;
        ThreadFuncs::startThread(threadMain, &gThreadInfo[threadNum]);
    }


    unsigned long startTime = ThreadFuncs::getCurrentMillis();
    int elapsedSeconds = 0;
    int timeSinceCheck = 0;

    //
    // Unblock the threads.
    //
    gRunInfo.stopFlag = FALSE;       // Unblocks the worker threads.
    umtx_unlock(&gStopMutex);      

    //
    //  Loop, watching the heartbeat of the worker threads.
    //    Each second,
    //            display "+" if all threads have completed at least one loop
    //            display "." if some thread hasn't since previous "+"
    //    Each "ctime" seconds,
    //            Stop all the worker threads at the top of their loop, then
    //            call the test's check function.
    //
    while (gRunInfo.totalTime == 0 || gRunInfo.totalTime > elapsedSeconds)
    {
        ThreadFuncs::Sleep(1000);      // We sleep while threads do their work ...

        if (gRunInfo.quiet == false && gRunInfo.verbose == false)
        {
            char c = '+';
            int threadNum;
            umtx_lock(&gInfoMutex);
            for (threadNum=0; threadNum < gRunInfo.numThreads; threadNum++)
            {
                if (gThreadInfo[threadNum].fHeartBeat == false)
                {
                    c = '.';
                    break;
                };
            }
            umtx_unlock(&gInfoMutex);
            fputc(c, stdout);
            fflush(stdout);
            if (c == '+')
                for (threadNum=0; threadNum < gRunInfo.numThreads; threadNum++)
                    gThreadInfo[threadNum].fHeartBeat = false;
        }

        //
        // Update running times.
        //
        timeSinceCheck -= elapsedSeconds;
        elapsedSeconds = (ThreadFuncs::getCurrentMillis() - startTime) / 1000;
        timeSinceCheck += elapsedSeconds;

        //
        //  Call back to the test to let it check its internal validity
        //
        if (timeSinceCheck >= gRunInfo.checkTime) {
            if (gRunInfo.verbose) {
                fprintf(stderr, "Main: suspending all threads\n");
            }
            umtx_lock(&gStopMutex);               // Block the worker threads at the top of their loop
            gRunInfo.stopFlag = TRUE;
            for (;;) {
                umtx_lock(&gInfoMutex);
                UBool done = gRunInfo.runningThreads == 0;
                umtx_unlock(&gInfoMutex);
                if (done) { break;}
                ThreadFuncs::yield();
            }


            
            gRunInfo.fTest->check();
            if (gRunInfo.quiet == false && gRunInfo.verbose == false) {
                fputc('C', stdout);
            }

            if (gRunInfo.verbose) {
                fprintf(stderr, "Main: starting all threads.\n");
            }
            gRunInfo.stopFlag = FALSE;       // Unblock the worker threads.
            umtx_unlock(&gStopMutex);      
            timeSinceCheck = 0;
        }
    };

    //
    //  Time's up, we are done.  (We only get here if this was a timed run)
    //  Tell the threads to exit.
    //
    gRunInfo.exitFlag = true;
    for (;;) {
        umtx_lock(&gInfoMutex);
        UBool done = gRunInfo.runningThreads == 0;
        umtx_unlock(&gInfoMutex);
        if (done) { break;}
        ThreadFuncs::yield();
    }

    //
    //  Tally up the total number of cycles completed by each of the threads.
    //
    double totalCyclesCompleted = 0;
    for (threadNum=0; threadNum < gRunInfo.numThreads; threadNum++) {
        totalCyclesCompleted += gThreadInfo[threadNum].fCycles;
    }

    double cyclesPerMinute = totalCyclesCompleted / (double(gRunInfo.totalTime) / double(60));
    printf("\n%8.1f cycles per minute.", cyclesPerMinute);

    //
    //  Memory should be clean coming out
    //
    delete gRunInfo.fTest;
    delete [] gThreadInfo;
    umtx_destroy(&gInfoMutex);
    umtx_destroy(&gStopMutex);
    u_cleanup();

    return 0;
}


