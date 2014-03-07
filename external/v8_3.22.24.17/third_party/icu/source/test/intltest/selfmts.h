/********************************************************************
 * COPYRIGHT: 
 * Copyright (c) 1997-2010, International Business Machines Corporation and
 * others. All Rights Reserved.
 * Copyright (C) 2010 , Yahoo! Inc. 
 ********************************************************************/

#ifndef _SELECTFORMATTEST
#define _SELECTFORMATTEST

#include "unicode/utypes.h"
#include "unicode/selfmt.h"


#if !UCONFIG_NO_FORMATTING

#include "intltest.h"

/**
 * Test basic functionality of various API functions
 **/
class SelectFormatTest : public IntlTest {
    void runIndexedTest( int32_t index, UBool exec, const char* &name, char* par = NULL );  

private:
    /**
     * Performs tests on many API functions, see detailed comments in source code
     **/
    void selectFormatAPITest(/* char* par */);
    void selectFormatUnitTest(/* char* par */);
};

#endif /* #if !UCONFIG_NO_FORMATTING */

#endif
