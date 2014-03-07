/*
**********************************************************************
*   Copyright (C) 2001-2004, International Business Machines
*   Corporation and others.  All Rights Reserved.
**********************************************************************
*   Date        Name        Description
*   05/23/00    aliu        Creation.
**********************************************************************
*/
#ifndef TRANSRT_H
#define TRANSRT_H

#include "unicode/utypes.h"

#if !UCONFIG_NO_TRANSLITERATION

#include "unicode/translit.h"
#include "intltest.h"

/**
 * @test
 * @summary Round trip test of Transliterator
 */
class TransliteratorRoundTripTest : public IntlTest {

    void runIndexedTest(int32_t index, UBool exec, const char* &name,
                        char* par=NULL);

    void TestKana(void);
    void TestHiragana(void);
    void TestKatakana(void);
    void TestJamo(void);
    void TestHangul(void);
    void TestHan(void);
    void TestGreek(void);
    void TestGreekUNGEGN(void);
    void Testel(void);
    void TestCyrillic(void);
    void TestDevanagariLatin(void);
    void TestInterIndic(void);
    void TestHebrew(void);
    void TestArabic(void);
    void TestDebug(const char* name,const char fromSet[],
                   const char* toSet,const char* exclusions);
};

#endif /* #if !UCONFIG_NO_TRANSLITERATION */

#endif
