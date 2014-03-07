
/********************************************************************
 * COPYRIGHT: 
 * Copyright (c) 1997-2003, International Business Machines Corporation and
 * others. All Rights Reserved.
 ********************************************************************/
/***********************************************************************
************************************************************************
*   Date        Name        Description
*   03/09/2000   Madhu        Creation.
************************************************************************/

#ifndef CPDTRTST_H
#define CPDTRTST_H

#include "unicode/utypes.h"

#if !UCONFIG_NO_TRANSLITERATION

#include "unicode/translit.h"
#include "intltest.h"

/**
 * @test
 * @summary General test of Compound Transliterator
 */
class CompoundTransliteratorTest : public IntlTest {
public:
    void runIndexedTest(int32_t index, UBool exec, const char* &name, char* par=NULL);

    /*Tests the constructors */
    void TestConstruction(void);
    /*Tests the function clone, and operator==()*/
    void TestCloneEqual(void);
    /*Tests the function getCount()*/
    void TestGetCount(void);
    /*Tests the function getTransliterator() and setTransliterators() and adoptTransliterators()*/
    void TestGetSetAdoptTransliterator(void);
    /*Tests the function handleTransliterate()*/
    void TestTransliterate(void);

    //======================================================================
    // Support methods
    //======================================================================

    /**
     * Splits a UnicodeString
     */
    UnicodeString* split(const UnicodeString& str, UChar seperator, int32_t& count);

    void expect(const CompoundTransliterator& t,
                const UnicodeString& source,
                const UnicodeString& expectedResult);

    void expectAux(const UnicodeString& tag,
                   const UnicodeString& summary, UBool pass,
                   const UnicodeString& expectedResult);


};

#endif /* #if !UCONFIG_NO_TRANSLITERATION */

#endif
