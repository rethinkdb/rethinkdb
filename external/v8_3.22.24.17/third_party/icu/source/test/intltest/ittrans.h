/********************************************************************
 * COPYRIGHT: 
 * Copyright (c) 2001, International Business Machines Corporation and
 * others. All Rights Reserved.
 ********************************************************************/
/************************************************************************
*   Date        Name        Description
*   12/14/99    Madhu        Creation.
************************************************************************/
/**
 * IntlTestRBBI is the top level test class for the RuleBasedBreakIterator tests
 */

#ifndef INTLTRANSLIT_H
#define INTLTRANSLIT_H

#include "unicode/utypes.h"

#if !UCONFIG_NO_TRANSLITERATION

#include "intltest.h"


class IntlTestTransliterator: public IntlTest {
public:
    void runIndexedTest( int32_t index, UBool exec, const char* &name, char* par = NULL );
};

#endif /* #if !UCONFIG_NO_TRANSLITERATION */

#endif
