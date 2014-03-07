/********************************************************************
 * COPYRIGHT:
 * Copyright (c) 1997-2009, International Business Machines Corporation and
 * others. All Rights Reserved.
 ********************************************************************/
/********************************************************************************
*
* File CJAPTST.C
*
* Modification History:
*        Name                     Description 
*     Madhu Katragadda            Ported for C API
* synwee                          Added TestBase, TestPlainDakutenHandakuten,
*                                 TestSmallLarge, TestKatakanaHiragana,
*                                 TestChooonKigoo
*********************************************************************************/
/**
 * CollationKannaTest is a third level test class.  This tests the locale
 * specific primary, secondary and tertiary rules.  For example, the ignorable
 * character '-' in string "black-bird".  The en_US locale uses the default
 * collation rules as its sorting sequence.
 */

#include <stdlib.h>

#include "unicode/utypes.h"

#if !UCONFIG_NO_COLLATION

#include "unicode/ucol.h"
#include "unicode/uloc.h"
#include "cintltst.h"
#include "ccolltst.h"
#include "callcoll.h"
#include "cjaptst.h"
#include "unicode/ustring.h"
#include "string.h"

static UCollator *myCollation;
const static UChar testSourceCases[][MAX_TOKEN_LEN] = {
    {0xff9E, 0x0000},
    {0x3042, 0x0000},
    {0x30A2, 0x0000},
    {0x3042, 0x3042, 0x0000},
    {0x30A2, 0x30FC, 0x0000},
    {0x30A2, 0x30FC, 0x30C8, 0x0000}                               /*  6 */
};

const static UChar testTargetCases[][MAX_TOKEN_LEN] = {
    {0xFF9F, 0x0000},
    {0x30A2, 0x0000},
    {0x3042, 0x3042, 0x0000},
    {0x30A2, 0x30FC, 0x0000},
    {0x30A2, 0x30FC, 0x30C8, 0x0000},
    {0x3042, 0x3042, 0x3068, 0x0000}                              /*  6 */
};

const static UCollationResult results[] = {
    UCOL_LESS,
    UCOL_EQUAL, /*UCOL_LESS*/   /* Katakanas and Hiraganas are equal on tertiary level(ICU 2.0)*/
    UCOL_LESS,
    UCOL_GREATER, /*UCOL_LESS*/ /* Prolonged sound mark sorts BEFORE equivalent vowel (ICU 2.0)*/
    UCOL_LESS,
    UCOL_LESS, /*UCOL_GREATER*/ /* Prolonged sound mark sorts BEFORE equivalent vowel (ICU 2.0)*//*  6 */
};

const static UChar testBaseCases[][MAX_TOKEN_LEN] = {
  {0x30AB, 0x0000},
  {0x30AB, 0x30AD, 0x0000},
  {0x30AD, 0x0000},
  {0x30AD, 0x30AD, 0x0000}
};

const static UChar testPlainDakutenHandakutenCases[][MAX_TOKEN_LEN] = {
  {0x30CF, 0x30AB, 0x0000},
  {0x30D0, 0x30AB, 0x0000},
  {0x30CF, 0x30AD, 0x0000},
  {0x30D0, 0x30AD, 0x0000}
};

const static UChar testSmallLargeCases[][MAX_TOKEN_LEN] = {
  {0x30C3, 0x30CF, 0x0000},
  {0x30C4, 0x30CF, 0x0000},
  {0x30C3, 0x30D0, 0x0000},
  {0x30C4, 0x30D0, 0x0000}
};

const static UChar testKatakanaHiraganaCases[][MAX_TOKEN_LEN] = {
  {0x3042, 0x30C3, 0x0000},
  {0x30A2, 0x30C3, 0x0000},
  {0x3042, 0x30C4, 0x0000},
  {0x30A2, 0x30C4, 0x0000}
};

const static UChar testChooonKigooCases[][MAX_TOKEN_LEN] = {
  /*0*/ {0x30AB, 0x30FC, 0x3042, 0x0000},
  /*1*/ {0x30AB, 0x30FC, 0x30A2, 0x0000},
  /*2*/ {0x30AB, 0x30A4, 0x3042, 0x0000},
  /*3*/ {0x30AB, 0x30A4, 0x30A2, 0x0000},
  /*6*/ {0x30AD, 0x30FC, 0x3042, 0x0000}, /* Prolonged sound mark sorts BEFORE equivalent vowel (ICU 2.0)*/
  /*7*/ {0x30AD, 0x30FC, 0x30A2, 0x0000}, /* Prolonged sound mark sorts BEFORE equivalent vowel (ICU 2.0)*/
  /*4*/ {0x30AD, 0x30A4, 0x3042, 0x0000},
  /*5*/ {0x30AD, 0x30A4, 0x30A2, 0x0000},
};

void addKannaCollTest(TestNode** root)
{
    addTest(root, &TestTertiary, "tscoll/cjacoll/TestTertiary");
    addTest(root, &TestBase, "tscoll/cjacoll/TestBase");
    addTest(root, &TestPlainDakutenHandakuten, "tscoll/cjacoll/TestPlainDakutenHandakuten");
    addTest(root, &TestSmallLarge, "tscoll/cjacoll/TestSmallLarge");
    addTest(root, &TestKatakanaHiragana, "tscoll/cjacoll/TestKatakanaHiragana");
    addTest(root, &TestChooonKigoo, "tscoll/cjacoll/TestChooonKigoo");
}

static void TestTertiary( )
{
    int32_t i;
    UErrorCode status = U_ZERO_ERROR;
    myCollation = ucol_open("ja_JP", &status);
    if(U_FAILURE(status)){
        log_err_status(status, "ERROR: in creation of rule based collator: %s\n", myErrorName(status));
        return;
    }
    log_verbose("Testing Kanna(Japan) Collation with Tertiary strength\n");
    ucol_setStrength(myCollation, UCOL_TERTIARY);
    ucol_setAttribute(myCollation, UCOL_CASE_LEVEL, UCOL_ON, &status);
    for (i = 0; i < 6 ; i++)
    {
        doTest(myCollation, testSourceCases[i], testTargetCases[i], results[i]);
    }
    ucol_close(myCollation);
}

/* Testing base letters */
static void TestBase()
{
    int32_t i;
    UErrorCode status = U_ZERO_ERROR;
    myCollation = ucol_open("ja_JP", &status);
    if (U_FAILURE(status))
    {
        log_err_status(status, "ERROR: in creation of rule based collator: %s\n",
            myErrorName(status));
        return;
    }

    log_verbose("Testing Japanese Base Characters Collation\n");
    ucol_setStrength(myCollation, UCOL_PRIMARY);
    for (i = 0; i < 3 ; i++)
        doTest(myCollation, testBaseCases[i], testBaseCases[i + 1], UCOL_LESS);

    ucol_close(myCollation);
}

/* Testing plain, Daku-ten, Handaku-ten letters */
static void TestPlainDakutenHandakuten(void)
{
    int32_t i;
    UErrorCode status = U_ZERO_ERROR;
    myCollation = ucol_open("ja_JP", &status);
    if (U_FAILURE(status))
    {
        log_err_status(status, "ERROR: in creation of rule based collator: %s\n",
            myErrorName(status));
        return;
    }

    log_verbose("Testing plain, Daku-ten, Handaku-ten letters Japanese Characters Collation\n");
    ucol_setStrength(myCollation, UCOL_SECONDARY);
    for (i = 0; i < 3 ; i++)
        doTest(myCollation, testPlainDakutenHandakutenCases[i],
        testPlainDakutenHandakutenCases[i + 1], UCOL_LESS);

    ucol_close(myCollation);
}

/*
* Test Small, Large letters
*/
static void TestSmallLarge(void)
{
    int32_t i;
    UErrorCode status = U_ZERO_ERROR;
    myCollation = ucol_open("ja_JP", &status);
    if (U_FAILURE(status))
    {
        log_err_status(status, "ERROR: in creation of rule based collator: %s\n",
            myErrorName(status));
        return;
    }

    log_verbose("Testing Japanese Small and Large Characters Collation\n");
    ucol_setStrength(myCollation, UCOL_TERTIARY);
    ucol_setAttribute(myCollation, UCOL_CASE_LEVEL, UCOL_ON, &status);
    for (i = 0; i < 3 ; i++)
        doTest(myCollation, testSmallLargeCases[i], testSmallLargeCases[i + 1],
        UCOL_LESS);

    ucol_close(myCollation);
}

/*
* Test Katakana, Hiragana letters
*/
static void TestKatakanaHiragana(void)
{
    int32_t i;
    UErrorCode status = U_ZERO_ERROR;
    myCollation = ucol_open("ja_JP", &status);
    if (U_FAILURE(status))
    {
        log_err_status(status, "ERROR: in creation of rule based collator: %s\n",
            myErrorName(status));
        return;
    }

    log_verbose("Testing Japanese Katakana, Hiragana Characters Collation\n");
    ucol_setStrength(myCollation, UCOL_QUATERNARY);
    ucol_setAttribute(myCollation, UCOL_CASE_LEVEL, UCOL_ON, &status);
    for (i = 0; i < 3 ; i++) {
        doTest(myCollation, testKatakanaHiraganaCases[i],
            testKatakanaHiraganaCases[i + 1], UCOL_LESS);
    }

    ucol_close(myCollation);
}

/*
* Test Choo-on kigoo
*/
static void TestChooonKigoo(void)
{
    int32_t i;
    UErrorCode status = U_ZERO_ERROR;
    myCollation = ucol_open("ja_JP", &status);
    if (U_FAILURE(status))
    {
        log_err_status(status, "ERROR: in creation of rule based collator: %s\n",
            myErrorName(status));
        return;
    }

    log_verbose("Testing Japanese Choo-on Kigoo Characters Collation\n");
    ucol_setAttribute(myCollation, UCOL_STRENGTH, UCOL_QUATERNARY, &status);
    ucol_setAttribute(myCollation, UCOL_CASE_LEVEL, UCOL_ON, &status);
    for (i = 0; i < 7 ; i++) {
        doTest(myCollation, testChooonKigooCases[i], testChooonKigooCases[i + 1],
            UCOL_LESS);
    }

    ucol_close(myCollation);
}

#endif /* #if !UCONFIG_NO_COLLATION */
