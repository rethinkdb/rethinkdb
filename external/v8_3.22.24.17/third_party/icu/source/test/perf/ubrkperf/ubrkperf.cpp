/********************************************************************
 * COPYRIGHT:
 * Copyright (C) 2001-2008 International Business Machines Corporation
 * and others. All Rights Reserved.
 *
 ********************************************************************/
/********************************************************************************
*
* File ubrkperf.cpp
*
* Modification History:
*        Name                     Description
*     Vladimir Weinstein          First Version, based on collperf
*
*********************************************************************************
*/

#include "ubrkperf.h"
#include "uoptions.h"
#include <stdio.h>


#if 0
#ifdef U_DARWIN
#include <ApplicationServices/ApplicationServices.h>
enum{
  kUCTextBreakAllMask = (kUCTextBreakClusterMask | kUCTextBreakWordMask | kUCTextBreakLineMask)
    };
UCTextBreakType breakTypes[4] = {kUCTextBreakCharMask, kUCTextBreakClusterMask, kUCTextBreakWordMask, kUCTextBreakLineMask};
TextBreakLocatorRef breakRef;
UCTextBreakType macBreakType;

void createMACBrkIt() {
  OSStatus status = noErr;
  LocaleRef lref;
  status = LocaleRefFromLocaleString(opt_locale, &lref);
  status = UCCreateTextBreakLocator(lref, 0, kUCTextBreakAllMask, (TextBreakLocatorRef*)&breakRef);
  if(opt_char == TRUE) {
    macBreakType = kUCTextBreakClusterMask;
  } else if(opt_word == TRUE) {
    macBreakType = kUCTextBreakWordMask;
  } else if(opt_line == TRUE) {
    macBreakType = kUCTextBreakLineMask;
  } else if(opt_sentence == TRUE) {
    // error
    // brkit = BreakIterator::createSentenceInstance(opt_locale, status);
  } else {
    // default is character iterator
    macBreakType = kUCTextBreakClusterMask;
      }
}
#endif


void doForwardTest() {
  if (opt_terse == FALSE) {
    printf("Doing the forward test\n");
  }
  int32_t noBreaks = 0;
  int32_t i = 0;
  unsigned long startTime = timeGetTime();
  unsigned long elapsedTime = 0;
  if(opt_icu) {
    createICUBrkIt();
    brkit->setText(text);
    brkit->first();
    if (opt_terse == FALSE) {
      printf("Warmup\n");
    }
    while(brkit->next() != BreakIterator::DONE) {
      noBreaks++;
    }
  
    if (opt_terse == FALSE) {
      printf("Measure\n");
    } 
    startTime = timeGetTime();
    for(i = 0; i < opt_loopCount; i++) {
      brkit->first();  
      while(brkit->next() != BreakIterator::DONE) {
      }
    }

    elapsedTime = timeGetTime()-startTime;
  } else if(opt_mac) {
#ifdef U_DARWIN
    createMACBrkIt();
    UniChar* filePtr = text;
    OSStatus status = noErr;
    UniCharCount startOffset = 0, breakOffset = 0, numUniChars = textSize;
    startOffset = 0;
    //printf("\t---Search forward--\n");
			
    while (startOffset < numUniChars)
    {
	status = UCFindTextBreak(breakRef, macBreakType, kUCTextBreakLeadingEdgeMask, filePtr, numUniChars,
                               startOffset, &breakOffset);
      //require_action(status == noErr, EXIT, printf( "**UCFindTextBreak failed: startOffset %d, status %d\n", (int)startOffset, (int)status));
      //require_action((breakOffset <= numUniChars),EXIT, printf("**UCFindTextBreak breakOffset too big: startOffset %d, breakOffset %d\n", (int)startOffset, (int)breakOffset));
				
      // Output break
      //printf("\t%d\n", (int)breakOffset);
				
      // Increment counters
	noBreaks++;
      startOffset = breakOffset;
    }
    startTime = timeGetTime();
    for(i = 0; i < opt_loopCount; i++) {
      startOffset = 0;
			
      while (startOffset < numUniChars)
	{
	  status = UCFindTextBreak(breakRef, macBreakType, kUCTextBreakLeadingEdgeMask, filePtr, numUniChars,
				   startOffset, &breakOffset);
	  // Increment counters
	  startOffset = breakOffset;
	}
    }
    elapsedTime = timeGetTime()-startTime;
    UCDisposeTextBreakLocator(&breakRef);
#endif


  }


  if (opt_terse == FALSE) {
  int32_t loopTime = (int)(float(1000) * ((float)elapsedTime/(float)opt_loopCount));
      int32_t timePerCU = (int)(float(1000) * ((float)loopTime/(float)textSize));
      int32_t timePerBreak = (int)(float(1000) * ((float)loopTime/(float)noBreaks));
      printf("forward break iteration average loop time %d\n", loopTime);
      printf("number of code units %d average time per code unit %d\n", textSize, timePerCU);
      printf("number of breaks %d average time per break %d\n", noBreaks, timePerBreak);
  } else {
      printf("time=%d\nevents=%d\nsize=%d\n", elapsedTime, noBreaks, textSize);
  }


}




#endif

UPerfFunction* BreakIteratorPerformanceTest::TestICUForward()
{
  return new ICUForward(locale, m_mode_, m_file_, m_fileLen_);
}

UPerfFunction* BreakIteratorPerformanceTest::TestICUIsBound()
{
  return new ICUIsBound(locale, m_mode_, m_file_, m_fileLen_);
}

UPerfFunction* BreakIteratorPerformanceTest::TestDarwinForward()
{
  return NULL;
}

UPerfFunction* BreakIteratorPerformanceTest::TestDarwinIsBound()
{
  return NULL;
}

UPerfFunction* BreakIteratorPerformanceTest::runIndexedTest(int32_t index, UBool exec,
												   const char *&name, 
												   char* par) 
{
    switch (index) {
        TESTCASE(0, TestICUForward);
		TESTCASE(1, TestICUIsBound);
		TESTCASE(2, TestDarwinForward);
		TESTCASE(3, TestDarwinIsBound);
        default: 
            name = ""; 
            return NULL;
    }
    return NULL;
}

UOption options[]={
                      UOPTION_DEF( "mode",        'm', UOPT_REQUIRES_ARG)
                  };


BreakIteratorPerformanceTest::BreakIteratorPerformanceTest(int32_t argc, const char* argv[], UErrorCode& status)
: UPerfTest(argc,argv,status),
m_mode_(NULL),
m_file_(NULL),
m_fileLen_(0)
{

    _remainingArgc = u_parseArgs(_remainingArgc, (char**)argv, (int32_t)(sizeof(options)/sizeof(options[0])), options);


    if(options[0].doesOccur) {
      m_mode_ = options[0].value;
      switch(options[0].value[0]) {
      case 'w' : 
      case 'c' :
      case 's' :
      case 'l' :
        break;
      default:
        status = U_ILLEGAL_ARGUMENT_ERROR;
        break;
      }
    } else {
      status = U_ILLEGAL_ARGUMENT_ERROR;
    }

    m_file_ = getBuffer(m_fileLen_, status);

    if(status== U_ILLEGAL_ARGUMENT_ERROR){
       fprintf(stderr, gUsageString, "ubrkperf");
       fprintf(stderr, "\t-m or --mode        Required mode for breakiterator: char, word, line or sentence\n");

       return;
    }

    if(U_FAILURE(status)){
        fprintf(stderr, "FAILED to create UPerfTest object. Error: %s\n", u_errorName(status));
        return;
    }
}

BreakIteratorPerformanceTest::~BreakIteratorPerformanceTest()
{
}


//----------------------------------------------------------------------------------------
//
//    Main   --  process command line, read in and pre-process the test file,
//                 call other functions to do the actual tests.
//
//----------------------------------------------------------------------------------------
int main(int argc, const char** argv) {
    UErrorCode status = U_ZERO_ERROR;
    BreakIteratorPerformanceTest test(argc, argv, status);
    if(U_FAILURE(status)){
        return status;
    }
    if(test.run()==FALSE){
        fprintf(stderr,"FAILED: Tests could not be run please check the arguments.\n");
        return -1;
    }
    return 0;
}
