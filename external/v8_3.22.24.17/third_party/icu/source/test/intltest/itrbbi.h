/*
**********************************************************************
* Copyright (C) 1998-2001, International Business Machines Corporation 
* and others.  All Rights Reserved.
**********************************************************************
************************************************************************
*   Date        Name        Description
*   12/14/99    Madhu        Creation.
************************************************************************/
/**
 * IntlTestRBBI is the top level test class for the RuleBasedBreakIterator tests
 */

#ifndef INTLTESTRBBI_H
#define INTLTESTRBBI_H

#include "unicode/utypes.h"

#if !UCONFIG_NO_BREAK_ITERATION

#include "intltest.h"


class IntlTestRBBI: public IntlTest {
public:
    void runIndexedTest( int32_t index, UBool exec, const char* &name, char* par = NULL );
};

#endif /* #if !UCONFIG_NO_BREAK_ITERATION */

#endif
