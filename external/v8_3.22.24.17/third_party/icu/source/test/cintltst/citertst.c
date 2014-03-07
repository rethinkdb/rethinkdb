/********************************************************************
 * COPYRIGHT:
 * Copyright (c) 1997-2011, International Business Machines Corporation and
 * others. All Rights Reserved.
 ********************************************************************/
/********************************************************************************
*
* File CITERTST.C
*
* Modification History:
* Date      Name               Description
*           Madhu Katragadda   Ported for C API
* 02/19/01  synwee             Modified test case for new collation iterator
*********************************************************************************/
/*
 * Collation Iterator tests.
 * (Let me reiterate my position...)
 */

#include "unicode/utypes.h"

#if !UCONFIG_NO_COLLATION

#include "unicode/ucol.h"
#include "unicode/ucoleitr.h"
#include "unicode/uloc.h"
#include "unicode/uchar.h"
#include "unicode/ustring.h"
#include "unicode/putil.h"
#include "callcoll.h"
#include "cmemory.h"
#include "cintltst.h"
#include "citertst.h"
#include "ccolltst.h"
#include "filestrm.h"
#include "cstring.h"
#include "ucol_imp.h"
#include "ucol_tok.h"
#include "uparse.h"
#include <stdio.h>

extern uint8_t ucol_uprv_getCaseBits(const UChar *, uint32_t, UErrorCode *);

void addCollIterTest(TestNode** root)
{
    addTest(root, &TestPrevious, "tscoll/citertst/TestPrevious");
    addTest(root, &TestOffset, "tscoll/citertst/TestOffset");
    addTest(root, &TestSetText, "tscoll/citertst/TestSetText");
    addTest(root, &TestMaxExpansion, "tscoll/citertst/TestMaxExpansion");
    addTest(root, &TestUnicodeChar, "tscoll/citertst/TestUnicodeChar");
    addTest(root, &TestNormalizedUnicodeChar,
                                "tscoll/citertst/TestNormalizedUnicodeChar");
    addTest(root, &TestNormalization, "tscoll/citertst/TestNormalization");
    addTest(root, &TestBug672, "tscoll/citertst/TestBug672");
    addTest(root, &TestBug672Normalize, "tscoll/citertst/TestBug672Normalize");
    addTest(root, &TestSmallBuffer, "tscoll/citertst/TestSmallBuffer");
    addTest(root, &TestCEs, "tscoll/citertst/TestCEs");
    addTest(root, &TestDiscontiguos, "tscoll/citertst/TestDiscontiguos");
    addTest(root, &TestCEBufferOverflow, "tscoll/citertst/TestCEBufferOverflow");
    addTest(root, &TestCEValidity, "tscoll/citertst/TestCEValidity");
    addTest(root, &TestSortKeyValidity, "tscoll/citertst/TestSortKeyValidity");
    addTest(root, &TestSearchCollatorElements, "tscoll/citertst/TestSearchCollatorElements");
}

/* The locales we support */

static const char * LOCALES[] = {"en_AU", "en_BE", "en_CA"};

static void TestBug672() {
    UErrorCode  status = U_ZERO_ERROR;
    UChar       pattern[20];
    UChar       text[50];
    int         i;
    int         result[3][3];

    u_uastrcpy(pattern, "resume");
    u_uastrcpy(text, "Time to resume updating my resume.");

    for (i = 0; i < 3; ++ i) {
        UCollator          *coll = ucol_open(LOCALES[i], &status);
        UCollationElements *pitr = ucol_openElements(coll, pattern, -1,
                                                     &status);
        UCollationElements *titer = ucol_openElements(coll, text, -1,
                                                     &status);
        if (U_FAILURE(status)) {
            log_err_status(status, "ERROR: in creation of either the collator or the collation iterator :%s\n",
                    myErrorName(status));
            return;
        }

        log_verbose("locale tested %s\n", LOCALES[i]);

        while (ucol_next(pitr, &status) != UCOL_NULLORDER &&
               U_SUCCESS(status)) {
        }
        if (U_FAILURE(status)) {
            log_err("ERROR: reversing collation iterator :%s\n",
                    myErrorName(status));
            return;
        }
        ucol_reset(pitr);

        ucol_setOffset(titer, u_strlen(pattern), &status);
        if (U_FAILURE(status)) {
            log_err("ERROR: setting offset in collator :%s\n",
                    myErrorName(status));
            return;
        }
        result[i][0] = ucol_getOffset(titer);
        log_verbose("Text iterator set to offset %d\n", result[i][0]);

        /* Use previous() */
        ucol_previous(titer, &status);
        result[i][1] = ucol_getOffset(titer);
        log_verbose("Current offset %d after previous\n", result[i][1]);

        /* Add one to index */
        log_verbose("Adding one to current offset...\n");
        ucol_setOffset(titer, ucol_getOffset(titer) + 1, &status);
        if (U_FAILURE(status)) {
            log_err("ERROR: setting offset in collator :%s\n",
                    myErrorName(status));
            return;
        }
        result[i][2] = ucol_getOffset(titer);
        log_verbose("Current offset in text = %d\n", result[i][2]);
        ucol_closeElements(pitr);
        ucol_closeElements(titer);
        ucol_close(coll);
    }

    if (uprv_memcmp(result[0], result[1], 3) != 0 ||
        uprv_memcmp(result[1], result[2], 3) != 0) {
        log_err("ERROR: Different locales have different offsets at the same character\n");
    }
}



/*  Running this test with normalization enabled showed up a bug in the incremental
    normalization code. */
static void TestBug672Normalize() {
    UErrorCode  status = U_ZERO_ERROR;
    UChar       pattern[20];
    UChar       text[50];
    int         i;
    int         result[3][3];

    u_uastrcpy(pattern, "resume");
    u_uastrcpy(text, "Time to resume updating my resume.");

    for (i = 0; i < 3; ++ i) {
        UCollator          *coll = ucol_open(LOCALES[i], &status);
        UCollationElements *pitr = NULL;
        UCollationElements *titer = NULL;

        ucol_setAttribute(coll, UCOL_NORMALIZATION_MODE, UCOL_ON, &status);

        pitr = ucol_openElements(coll, pattern, -1, &status);
        titer = ucol_openElements(coll, text, -1, &status);
        if (U_FAILURE(status)) {
            log_err_status(status, "ERROR: in creation of either the collator or the collation iterator :%s\n",
                    myErrorName(status));
            return;
        }

        log_verbose("locale tested %s\n", LOCALES[i]);

        while (ucol_next(pitr, &status) != UCOL_NULLORDER &&
               U_SUCCESS(status)) {
        }
        if (U_FAILURE(status)) {
            log_err("ERROR: reversing collation iterator :%s\n",
                    myErrorName(status));
            return;
        }
        ucol_reset(pitr);

        ucol_setOffset(titer, u_strlen(pattern), &status);
        if (U_FAILURE(status)) {
            log_err("ERROR: setting offset in collator :%s\n",
                    myErrorName(status));
            return;
        }
        result[i][0] = ucol_getOffset(titer);
        log_verbose("Text iterator set to offset %d\n", result[i][0]);

        /* Use previous() */
        ucol_previous(titer, &status);
        result[i][1] = ucol_getOffset(titer);
        log_verbose("Current offset %d after previous\n", result[i][1]);

        /* Add one to index */
        log_verbose("Adding one to current offset...\n");
        ucol_setOffset(titer, ucol_getOffset(titer) + 1, &status);
        if (U_FAILURE(status)) {
            log_err("ERROR: setting offset in collator :%s\n",
                    myErrorName(status));
            return;
        }
        result[i][2] = ucol_getOffset(titer);
        log_verbose("Current offset in text = %d\n", result[i][2]);
        ucol_closeElements(pitr);
        ucol_closeElements(titer);
        ucol_close(coll);
    }

    if (uprv_memcmp(result[0], result[1], 3) != 0 ||
        uprv_memcmp(result[1], result[2], 3) != 0) {
        log_err("ERROR: Different locales have different offsets at the same character\n");
    }
}




/**
 * Test for CollationElementIterator previous and next for the whole set of
 * unicode characters.
 */
static void TestUnicodeChar()
{
    UChar source[0x100];
    UCollator *en_us;
    UCollationElements *iter;
    UErrorCode status = U_ZERO_ERROR;
    UChar codepoint;

    UChar *test;
    en_us = ucol_open("en_US", &status);
    if (U_FAILURE(status)){
       log_err_status(status, "ERROR: in creation of collation data using ucol_open()\n %s\n",
              myErrorName(status));
       return;
    }

    for (codepoint = 1; codepoint < 0xFFFE;)
    {
      test = source;

      while (codepoint % 0xFF != 0)
      {
        if (u_isdefined(codepoint))
          *(test ++) = codepoint;
        codepoint ++;
      }

      if (u_isdefined(codepoint))
        *(test ++) = codepoint;

      if (codepoint != 0xFFFF)
        codepoint ++;

      *test = 0;
      iter=ucol_openElements(en_us, source, u_strlen(source), &status);
      if(U_FAILURE(status)){
          log_err("ERROR: in creation of collation element iterator using ucol_openElements()\n %s\n",
              myErrorName(status));
          ucol_close(en_us);
          return;
      }
      /* A basic test to see if it's working at all */
      log_verbose("codepoint testing %x\n", codepoint);
      backAndForth(iter);
      ucol_closeElements(iter);

      /* null termination test */
      iter=ucol_openElements(en_us, source, -1, &status);
      if(U_FAILURE(status)){
          log_err("ERROR: in creation of collation element iterator using ucol_openElements()\n %s\n",
              myErrorName(status));
          ucol_close(en_us);
          return;
      }
      /* A basic test to see if it's working at all */
      backAndForth(iter);
      ucol_closeElements(iter);
    }

    ucol_close(en_us);
}

/**
 * Test for CollationElementIterator previous and next for the whole set of
 * unicode characters with normalization on.
 */
static void TestNormalizedUnicodeChar()
{
    UChar source[0x100];
    UCollator *th_th;
    UCollationElements *iter;
    UErrorCode status = U_ZERO_ERROR;
    UChar codepoint;

    UChar *test;
    /* thai should have normalization on */
    th_th = ucol_open("th_TH", &status);
    if (U_FAILURE(status)){
        log_err_status(status, "ERROR: in creation of thai collation using ucol_open()\n %s\n",
              myErrorName(status));
        return;
    }

    for (codepoint = 1; codepoint < 0xFFFE;)
    {
      test = source;

      while (codepoint % 0xFF != 0)
      {
        if (u_isdefined(codepoint))
          *(test ++) = codepoint;
        codepoint ++;
      }

      if (u_isdefined(codepoint))
        *(test ++) = codepoint;

      if (codepoint != 0xFFFF)
        codepoint ++;

      *test = 0;
      iter=ucol_openElements(th_th, source, u_strlen(source), &status);
      if(U_FAILURE(status)){
          log_err("ERROR: in creation of collation element iterator using ucol_openElements()\n %s\n",
              myErrorName(status));
            ucol_close(th_th);
          return;
      }

      backAndForth(iter);
      ucol_closeElements(iter);

      iter=ucol_openElements(th_th, source, -1, &status);
      if(U_FAILURE(status)){
          log_err("ERROR: in creation of collation element iterator using ucol_openElements()\n %s\n",
              myErrorName(status));
            ucol_close(th_th);
          return;
      }

      backAndForth(iter);
      ucol_closeElements(iter);
    }

    ucol_close(th_th);
}

/**
* Test the incremental normalization
*/
static void TestNormalization()
{
          UErrorCode          status = U_ZERO_ERROR;
    const char               *str    =
                            "&a < \\u0300\\u0315 < A\\u0300\\u0315 < \\u0316\\u0315B < \\u0316\\u0300\\u0315";
          UCollator          *coll;
          UChar               rule[50];
          int                 rulelen = u_unescape(str, rule, 50);
          int                 count = 0;
    const char                *testdata[] =
                        {"\\u1ED9", "o\\u0323\\u0302",
                        "\\u0300\\u0315", "\\u0315\\u0300",
                        "A\\u0300\\u0315B", "A\\u0315\\u0300B",
                        "A\\u0316\\u0315B", "A\\u0315\\u0316B",
                        "\\u0316\\u0300\\u0315", "\\u0315\\u0300\\u0316",
                        "A\\u0316\\u0300\\u0315B", "A\\u0315\\u0300\\u0316B",
                        "\\u0316\\u0315\\u0300", "A\\u0316\\u0315\\u0300B"};
    int32_t   srclen;
    UChar source[10];
    UCollationElements *iter;

    coll = ucol_openRules(rule, rulelen, UCOL_ON, UCOL_TERTIARY, NULL, &status);
    ucol_setAttribute(coll, UCOL_NORMALIZATION_MODE, UCOL_ON, &status);
    if (U_FAILURE(status)){
        log_err_status(status, "ERROR: in creation of collator using ucol_openRules()\n %s\n",
              myErrorName(status));
        return;
    }

    srclen = u_unescape(testdata[0], source, 10);
    iter = ucol_openElements(coll, source, srclen, &status);
    backAndForth(iter);
    ucol_closeElements(iter);

    srclen = u_unescape(testdata[1], source, 10);
    iter = ucol_openElements(coll, source, srclen, &status);
    backAndForth(iter);
    ucol_closeElements(iter);

    while (count < 12) {
        srclen = u_unescape(testdata[count], source, 10);
        iter = ucol_openElements(coll, source, srclen, &status);

        if (U_FAILURE(status)){
            log_err("ERROR: in creation of collator element iterator\n %s\n",
                  myErrorName(status));
            return;
        }
        backAndForth(iter);
        ucol_closeElements(iter);

        iter = ucol_openElements(coll, source, -1, &status);

        if (U_FAILURE(status)){
            log_err("ERROR: in creation of collator element iterator\n %s\n",
                  myErrorName(status));
            return;
        }
        backAndForth(iter);
        ucol_closeElements(iter);
        count ++;
    }
    ucol_close(coll);
}

/**
 * Test for CollationElementIterator.previous()
 *
 * @bug 4108758 - Make sure it works with contracting characters
 *
 */
static void TestPrevious()
{
    UCollator *coll=NULL;
    UChar rule[50];
    UChar *source;
    UCollator *c1, *c2, *c3;
    UCollationElements *iter;
    UErrorCode status = U_ZERO_ERROR;
    UChar test1[50];
    UChar test2[50];

    u_uastrcpy(test1, "What subset of all possible test cases?");
    u_uastrcpy(test2, "has the highest probability of detecting");
    coll = ucol_open("en_US", &status);

    iter=ucol_openElements(coll, test1, u_strlen(test1), &status);
    log_verbose("English locale testing back and forth\n");
    if(U_FAILURE(status)){
        log_err_status(status, "ERROR: in creation of collation element iterator using ucol_openElements()\n %s\n",
            myErrorName(status));
        ucol_close(coll);
        return;
    }
    /* A basic test to see if it's working at all */
    backAndForth(iter);
    ucol_closeElements(iter);
    ucol_close(coll);

    /* Test with a contracting character sequence */
    u_uastrcpy(rule, "&a,A < b,B < c,C, d,D < z,Z < ch,cH,Ch,CH");
    c1 = ucol_openRules(rule, u_strlen(rule), UCOL_OFF, UCOL_DEFAULT_STRENGTH, NULL, &status);

    log_verbose("Contraction rule testing back and forth with no normalization\n");

    if (c1 == NULL || U_FAILURE(status))
    {
        log_err("Couldn't create a RuleBasedCollator with a contracting sequence\n %s\n",
            myErrorName(status));
        return;
    }
    source=(UChar*)malloc(sizeof(UChar) * 20);
    u_uastrcpy(source, "abchdcba");
    iter=ucol_openElements(c1, source, u_strlen(source), &status);
    if(U_FAILURE(status)){
        log_err("ERROR: in creation of collation element iterator using ucol_openElements()\n %s\n",
            myErrorName(status));
        return;
    }
    backAndForth(iter);
    ucol_closeElements(iter);
    ucol_close(c1);

    /* Test with an expanding character sequence */
    u_uastrcpy(rule, "&a < b < c/abd < d");
    c2 = ucol_openRules(rule, u_strlen(rule), UCOL_OFF, UCOL_DEFAULT_STRENGTH, NULL, &status);
    log_verbose("Expansion rule testing back and forth with no normalization\n");
    if (c2 == NULL || U_FAILURE(status))
    {
        log_err("Couldn't create a RuleBasedCollator with a contracting sequence.\n %s\n",
            myErrorName(status));
        return;
    }
    u_uastrcpy(source, "abcd");
    iter=ucol_openElements(c2, source, u_strlen(source), &status);
    if(U_FAILURE(status)){
        log_err("ERROR: in creation of collation element iterator using ucol_openElements()\n %s\n",
            myErrorName(status));
        return;
    }
    backAndForth(iter);
    ucol_closeElements(iter);
    ucol_close(c2);
    /* Now try both */
    u_uastrcpy(rule, "&a < b < c/aba < d < z < ch");
    c3 = ucol_openRules(rule, u_strlen(rule), UCOL_DEFAULT,  UCOL_DEFAULT_STRENGTH,NULL, &status);
    log_verbose("Expansion/contraction rule testing back and forth with no normalization\n");

    if (c3 == NULL || U_FAILURE(status))
    {
        log_err("Couldn't create a RuleBasedCollator with a contracting sequence.\n %s\n",
            myErrorName(status));
        return;
    }
    u_uastrcpy(source, "abcdbchdc");
    iter=ucol_openElements(c3, source, u_strlen(source), &status);
    if(U_FAILURE(status)){
        log_err("ERROR: in creation of collation element iterator using ucol_openElements()\n %s\n",
            myErrorName(status));
        return;
    }
    backAndForth(iter);
    ucol_closeElements(iter);
    ucol_close(c3);
    source[0] = 0x0e41;
    source[1] = 0x0e02;
    source[2] = 0x0e41;
    source[3] = 0x0e02;
    source[4] = 0x0e27;
    source[5] = 0x61;
    source[6] = 0x62;
    source[7] = 0x63;
    source[8] = 0;

    coll = ucol_open("th_TH", &status);
    log_verbose("Thai locale testing back and forth with normalization\n");
    iter=ucol_openElements(coll, source, u_strlen(source), &status);
    if(U_FAILURE(status)){
        log_err("ERROR: in creation of collation element iterator using ucol_openElements()\n %s\n",
            myErrorName(status));
        return;
    }
    backAndForth(iter);
    ucol_closeElements(iter);
    ucol_close(coll);

    /* prev test */
    source[0] = 0x0061;
    source[1] = 0x30CF;
    source[2] = 0x3099;
    source[3] = 0x30FC;
    source[4] = 0;

    coll = ucol_open("ja_JP", &status);
    log_verbose("Japanese locale testing back and forth with normalization\n");
    iter=ucol_openElements(coll, source, u_strlen(source), &status);
    if(U_FAILURE(status)){
        log_err("ERROR: in creation of collation element iterator using ucol_openElements()\n %s\n",
            myErrorName(status));
        return;
    }
    backAndForth(iter);
    ucol_closeElements(iter);
    ucol_close(coll);

    free(source);
}

/**
 * Test for getOffset() and setOffset()
 */
static void TestOffset()
{
    UErrorCode status= U_ZERO_ERROR;
    UCollator *en_us=NULL;
    UCollationElements *iter, *pristine;
    int32_t offset;
    OrderAndOffset *orders;
    int32_t orderLength=0;
    int     count = 0;
    UChar test1[50];
    UChar test2[50];

    u_uastrcpy(test1, "What subset of all possible test cases?");
    u_uastrcpy(test2, "has the highest probability of detecting");
    en_us = ucol_open("en_US", &status);
    log_verbose("Testing getOffset and setOffset for collations\n");
    iter = ucol_openElements(en_us, test1, u_strlen(test1), &status);
    if(U_FAILURE(status)){
        log_err_status(status, "ERROR: in creation of collation element iterator using ucol_openElements()\n %s\n",
            myErrorName(status));
        ucol_close(en_us);
        return;
    }

    /* testing boundaries */
    ucol_setOffset(iter, 0, &status);
    if (U_FAILURE(status) || ucol_previous(iter, &status) != UCOL_NULLORDER) {
        log_err("Error: After setting offset to 0, we should be at the end "
                "of the backwards iteration");
    }
    ucol_setOffset(iter, u_strlen(test1), &status);
    if (U_FAILURE(status) || ucol_next(iter, &status) != UCOL_NULLORDER) {
        log_err("Error: After setting offset to end of the string, we should "
                "be at the end of the backwards iteration");
    }

    /* Run all the way through the iterator, then get the offset */

    orders = getOrders(iter, &orderLength);

    offset = ucol_getOffset(iter);

    if (offset != u_strlen(test1))
    {
        log_err("offset at end != length %d vs %d\n", offset,
            u_strlen(test1) );
    }

    /* Now set the offset back to the beginning and see if it works */
    pristine=ucol_openElements(en_us, test1, u_strlen(test1), &status);
    if(U_FAILURE(status)){
        log_err("ERROR: in creation of collation element iterator using ucol_openElements()\n %s\n",
            myErrorName(status));
    ucol_close(en_us);
        return;
    }
    status = U_ZERO_ERROR;

    ucol_setOffset(iter, 0, &status);
    if (U_FAILURE(status))
    {
        log_err("setOffset failed. %s\n",    myErrorName(status));
    }
    else
    {
        assertEqual(iter, pristine);
    }

    ucol_closeElements(pristine);
    ucol_closeElements(iter);
    free(orders);

    /* testing offsets in normalization buffer */
    test1[0] = 0x61;
    test1[1] = 0x300;
    test1[2] = 0x316;
    test1[3] = 0x62;
    test1[4] = 0;
    ucol_setAttribute(en_us, UCOL_NORMALIZATION_MODE, UCOL_ON, &status);
    iter = ucol_openElements(en_us, test1, 4, &status);
    if(U_FAILURE(status)){
        log_err("ERROR: in creation of collation element iterator using ucol_openElements()\n %s\n",
            myErrorName(status));
        ucol_close(en_us);
        return;
    }

    count = 0;
    while (ucol_next(iter, &status) != UCOL_NULLORDER &&
        U_SUCCESS(status)) {
        switch (count) {
        case 0:
            if (ucol_getOffset(iter) != 1) {
                log_err("ERROR: Offset of iteration should be 1\n");
            }
            break;
        case 3:
            if (ucol_getOffset(iter) != 4) {
                log_err("ERROR: Offset of iteration should be 4\n");
            }
            break;
        default:
            if (ucol_getOffset(iter) != 3) {
                log_err("ERROR: Offset of iteration should be 3\n");
            }
        }
        count ++;
    }

    ucol_reset(iter);
    count = 0;
    while (ucol_previous(iter, &status) != UCOL_NULLORDER &&
        U_SUCCESS(status)) {
        switch (count) {
        case 0:
        case 1:
            if (ucol_getOffset(iter) != 3) {
                log_err("ERROR: Offset of iteration should be 3\n");
            }
            break;
        case 2:
            if (ucol_getOffset(iter) != 1) {
                log_err("ERROR: Offset of iteration should be 1\n");
            }
            break;
        default:
            if (ucol_getOffset(iter) != 0) {
                log_err("ERROR: Offset of iteration should be 0\n");
            }
        }
        count ++;
    }

    if(U_FAILURE(status)){
        log_err("ERROR: in iterating collation elements %s\n",
            myErrorName(status));
    }

    ucol_closeElements(iter);
    ucol_close(en_us);
}

/**
 * Test for setText()
 */
static void TestSetText()
{
    int32_t c,i;
    UErrorCode status = U_ZERO_ERROR;
    UCollator *en_us=NULL;
    UCollationElements *iter1, *iter2;
    UChar test1[50];
    UChar test2[50];

    u_uastrcpy(test1, "What subset of all possible test cases?");
    u_uastrcpy(test2, "has the highest probability of detecting");
    en_us = ucol_open("en_US", &status);
    log_verbose("testing setText for Collation elements\n");
    iter1=ucol_openElements(en_us, test1, u_strlen(test1), &status);
    if(U_FAILURE(status)){
        log_err_status(status, "ERROR: in creation of collation element iterator1 using ucol_openElements()\n %s\n",
            myErrorName(status));
    ucol_close(en_us);
        return;
    }
    iter2=ucol_openElements(en_us, test2, u_strlen(test2), &status);
    if(U_FAILURE(status)){
        log_err("ERROR: in creation of collation element iterator2 using ucol_openElements()\n %s\n",
            myErrorName(status));
    ucol_close(en_us);
        return;
    }

    /* Run through the second iterator just to exercise it */
    c = ucol_next(iter2, &status);
    i = 0;

    while ( ++i < 10 && (c != UCOL_NULLORDER))
    {
        if (U_FAILURE(status))
        {
            log_err("iter2->next() returned an error. %s\n", myErrorName(status));
            ucol_closeElements(iter2);
            ucol_closeElements(iter1);
    ucol_close(en_us);
            return;
        }

        c = ucol_next(iter2, &status);
    }

    /* Now set it to point to the same string as the first iterator */
    ucol_setText(iter2, test1, u_strlen(test1), &status);
    if (U_FAILURE(status))
    {
        log_err("call to iter2->setText(test1) failed. %s\n", myErrorName(status));
    }
    else
    {
        assertEqual(iter1, iter2);
    }

    /* Now set it to point to a null string with fake length*/
    ucol_setText(iter2, NULL, 2, &status);
    if (U_FAILURE(status))
    {
        log_err("call to iter2->setText(null) failed. %s\n", myErrorName(status));
    }
    else
    {
        if (ucol_next(iter2, &status) != UCOL_NULLORDER) {
            log_err("iter2 with null text expected to return UCOL_NULLORDER\n");
        }
    }

    ucol_closeElements(iter2);
    ucol_closeElements(iter1);
    ucol_close(en_us);
}

/** @bug 4108762
 * Test for getMaxExpansion()
 */
static void TestMaxExpansion()
{
    UErrorCode          status = U_ZERO_ERROR;
    UCollator          *coll   ;/*= ucol_open("en_US", &status);*/
    UChar               ch     = 0;
    UChar32             unassigned = 0xEFFFD;
    UChar               supplementary[2];
    uint32_t            stringOffset = 0;
    UBool               isError = FALSE;
    uint32_t            sorder = 0;
    UCollationElements *iter   ;/*= ucol_openElements(coll, &ch, 1, &status);*/
    uint32_t            temporder = 0;

    UChar rule[256];
    u_uastrcpy(rule, "&a < ab < c/aba < d < z < ch");
    coll = ucol_openRules(rule, u_strlen(rule), UCOL_DEFAULT,
        UCOL_DEFAULT_STRENGTH,NULL, &status);
    if(U_SUCCESS(status) && coll) {
      iter = ucol_openElements(coll, &ch, 1, &status);

      while (ch < 0xFFFF && U_SUCCESS(status)) {
          int      count = 1;
          uint32_t order;
          int32_t  size = 0;

          ch ++;

          ucol_setText(iter, &ch, 1, &status);
          order = ucol_previous(iter, &status);

          /* thai management */
          if (order == 0)
              order = ucol_previous(iter, &status);

          while (U_SUCCESS(status) &&
              ucol_previous(iter, &status) != UCOL_NULLORDER) {
              count ++;
          }

          size = ucol_getMaxExpansion(iter, order);
          if (U_FAILURE(status) || size < count) {
              log_err("Failure at codepoint %d, maximum expansion count < %d\n",
                  ch, count);
          }
      }

      /* testing for exact max expansion */
      ch = 0;
      while (ch < 0x61) {
          uint32_t order;
          int32_t  size;
          ucol_setText(iter, &ch, 1, &status);
          order = ucol_previous(iter, &status);
          size  = ucol_getMaxExpansion(iter, order);
          if (U_FAILURE(status) || size != 1) {
              log_err("Failure at codepoint %d, maximum expansion count < %d\n",
                  ch, 1);
          }
          ch ++;
      }

      ch = 0x63;
      ucol_setText(iter, &ch, 1, &status);
      temporder = ucol_previous(iter, &status);

      if (U_FAILURE(status) || ucol_getMaxExpansion(iter, temporder) != 3) {
          log_err("Failure at codepoint %d, maximum expansion count != %d\n",
                  ch, 3);
      }

      ch = 0x64;
      ucol_setText(iter, &ch, 1, &status);
      temporder = ucol_previous(iter, &status);

      if (U_FAILURE(status) || ucol_getMaxExpansion(iter, temporder) != 1) {
          log_err("Failure at codepoint %d, maximum expansion count != %d\n",
                  ch, 3);
      }

      U16_APPEND(supplementary, stringOffset, 2, unassigned, isError);
      ucol_setText(iter, supplementary, 2, &status);
      sorder = ucol_previous(iter, &status);

      if (U_FAILURE(status) || ucol_getMaxExpansion(iter, sorder) != 2) {
          log_err("Failure at codepoint %d, maximum expansion count < %d\n",
                  ch, 2);
      }

      /* testing jamo */
      ch = 0x1165;

      ucol_setText(iter, &ch, 1, &status);
      temporder = ucol_previous(iter, &status);
      if (U_FAILURE(status) || ucol_getMaxExpansion(iter, temporder) > 3) {
          log_err("Failure at codepoint %d, maximum expansion count > %d\n",
                  ch, 3);
      }

      ucol_closeElements(iter);
      ucol_close(coll);

      /* testing special jamo &a<\u1160 */
      rule[0] = 0x26;
      rule[1] = 0x71;
      rule[2] = 0x3c;
      rule[3] = 0x1165;
      rule[4] = 0x2f;
      rule[5] = 0x71;
      rule[6] = 0x71;
      rule[7] = 0x71;
      rule[8] = 0x71;
      rule[9] = 0;

      coll = ucol_openRules(rule, u_strlen(rule), UCOL_DEFAULT,
          UCOL_DEFAULT_STRENGTH,NULL, &status);
      iter = ucol_openElements(coll, &ch, 1, &status);

      temporder = ucol_previous(iter, &status);
      if (U_FAILURE(status) || ucol_getMaxExpansion(iter, temporder) != 6) {
          log_err("Failure at codepoint %d, maximum expansion count > %d\n",
                  ch, 5);
      }

      ucol_closeElements(iter);
      ucol_close(coll);
    } else {
      log_err_status(status, "Couldn't open collator -> %s\n", u_errorName(status));
    }

}


static void assertEqual(UCollationElements *i1, UCollationElements *i2)
{
    int32_t c1, c2;
    int32_t count = 0;
    UErrorCode status = U_ZERO_ERROR;

    do
    {
        c1 = ucol_next(i1, &status);
        c2 = ucol_next(i2, &status);

        if (c1 != c2)
        {
            log_err("Error in iteration %d assetEqual between\n  %d  and   %d, they are not equal\n", count, c1, c2);
            break;
        }

        count += 1;
    }
    while (c1 != UCOL_NULLORDER);
}

/**
 * Testing iterators with extremely small buffers
 */
static void TestSmallBuffer()
{
    UErrorCode          status = U_ZERO_ERROR;
    UCollator          *coll;
    UCollationElements *testiter,
                       *iter;
    int32_t             count = 0;
    OrderAndOffset     *testorders,
                       *orders;

    UChar teststr[500];
    UChar str[] = {0x300, 0x31A, 0};
    /*
    creating a long string of decomposable characters,
    since by default the writable buffer is of size 256
    */
    while (count < 500) {
        if ((count & 1) == 0) {
            teststr[count ++] = 0x300;
        }
        else {
            teststr[count ++] = 0x31A;
        }
    }

    coll = ucol_open("th_TH", &status);
    if(U_SUCCESS(status) && coll) {
      testiter = ucol_openElements(coll, teststr, 500, &status);
      iter = ucol_openElements(coll, str, 2, &status);

      orders     = getOrders(iter, &count);
      if (count != 2) {
          log_err("Error collation elements size is not 2 for \\u0300\\u031A\n");
      }

      /*
      this will rearrange the string data to 250 characters of 0x300 first then
      250 characters of 0x031A
      */
      testorders = getOrders(testiter, &count);

      if (count != 500) {
          log_err("Error decomposition does not give the right sized collation elements\n");
      }

      while (count != 0) {
          /* UCA collation element for 0x0F76 */
          if ((count > 250 && testorders[-- count].order != orders[1].order) ||
              (count <= 250 && testorders[-- count].order != orders[0].order)) {
              log_err("Error decomposition does not give the right collation element at %d count\n", count);
              break;
          }
      }

      free(testorders);
      free(orders);

      ucol_reset(testiter);

      /* ensures closing of elements done properly to clear writable buffer */
      ucol_next(testiter, &status);
      ucol_next(testiter, &status);
      ucol_closeElements(testiter);
      ucol_closeElements(iter);
      ucol_close(coll);
    } else {
      log_err_status(status, "Couldn't open collator -> %s\n", u_errorName(status));
    }
}

/**
* Sniplets of code from genuca
*/
static int32_t hex2num(char hex) {
    if(hex>='0' && hex <='9') {
        return hex-'0';
    } else if(hex>='a' && hex<='f') {
        return hex-'a'+10;
    } else if(hex>='A' && hex<='F') {
        return hex-'A'+10;
    } else {
        return 0;
    }
}

/**
* Getting codepoints from a string
* @param str character string contain codepoints seperated by space and ended
*        by a semicolon
* @param codepoints array for storage, assuming size > 5
* @return position at the end of the codepoint section
*/
static char *getCodePoints(char *str, UChar *codepoints, UChar *contextCPs) {
    UErrorCode errorCode = U_ZERO_ERROR;
    char *semi = uprv_strchr(str, ';');
    char *pipe = uprv_strchr(str, '|');
    char *s;
    *codepoints = 0;
    *contextCPs = 0;
    if(semi == NULL) {
        log_err("expected semicolon after code point string in FractionalUCA.txt %s\n", str);
        return str;
    }
    if(pipe != NULL) {
        int32_t contextLength;
        *pipe = 0;
        contextLength = u_parseString(str, contextCPs, 99, NULL, &errorCode);
        *pipe = '|';
        if(U_FAILURE(errorCode)) {
            log_err("error parsing precontext string from FractionalUCA.txt %s\n", str);
            return str;
        }
        /* prepend the precontext string to the codepoints */
        u_memcpy(codepoints, contextCPs, contextLength);
        codepoints += contextLength;
        /* start of the code point string */
        s = pipe + 1;
    } else {
        s = str;
    }
    u_parseString(s, codepoints, 99, NULL, &errorCode);
    if(U_FAILURE(errorCode)) {
        log_err("error parsing code point string from FractionalUCA.txt %s\n", str);
        return str;
    }
    return semi + 1;
}

/**
* Sniplets of code from genuca
*/
static int32_t
readElement(char **from, char *to, char separator, UErrorCode *status)
{
    if (U_SUCCESS(*status)) {
        char    buffer[1024];
        int32_t i = 0;
        while (**from != separator) {
            if (**from != ' ') {
                *(buffer+i++) = **from;
            }
            (*from)++;
        }
        (*from)++;
        *(buffer + i) = 0;
        strcpy(to, buffer);
        return i/2;
    }

    return 0;
}

/**
* Sniplets of code from genuca
*/
static uint32_t
getSingleCEValue(char *primary, char *secondary, char *tertiary,
                          UErrorCode *status)
{
    if (U_SUCCESS(*status)) {
        uint32_t  value    = 0;
        char      primsave = '\0';
        char      secsave  = '\0';
        char      tersave  = '\0';
        char     *primend  = primary+4;
        char     *secend   = secondary+2;
        char     *terend   = tertiary+2;
        uint32_t  primvalue;
        uint32_t  secvalue;
        uint32_t  tervalue;

        if (uprv_strlen(primary) > 4) {
            primsave = *primend;
            *primend = '\0';
        }

        if (uprv_strlen(secondary) > 2) {
            secsave = *secend;
            *secend = '\0';
        }

        if (uprv_strlen(tertiary) > 2) {
            tersave = *terend;
            *terend = '\0';
        }

        primvalue = (*primary!='\0')?uprv_strtoul(primary, &primend, 16):0;
        secvalue  = (*secondary!='\0')?uprv_strtoul(secondary, &secend, 16):0;
        tervalue  = (*tertiary!='\0')?uprv_strtoul(tertiary, &terend, 16):0;
        if(primvalue <= 0xFF) {
          primvalue <<= 8;
        }

        value = ((primvalue << UCOL_PRIMARYORDERSHIFT) & UCOL_PRIMARYORDERMASK)
           | ((secvalue << UCOL_SECONDARYORDERSHIFT) & UCOL_SECONDARYORDERMASK)
           | (tervalue & UCOL_TERTIARYORDERMASK);

        if(primsave!='\0') {
            *primend = primsave;
        }
        if(secsave!='\0') {
            *secend = secsave;
        }
        if(tersave!='\0') {
            *terend = tersave;
        }
        return value;
    }
    return 0;
}

/**
* Getting collation elements generated from a string
* @param str character string contain collation elements contained in [] and
*        seperated by space
* @param ce array for storage, assuming size > 20
* @param status error status
* @return position at the end of the codepoint section
*/
static char * getCEs(char *str, uint32_t *ces, UErrorCode *status) {
    char       *pStartCP     = uprv_strchr(str, '[');
    int         count        = 0;
    char       *pEndCP;
    char        primary[100];
    char        secondary[100];
    char        tertiary[100];

    while (*pStartCP == '[') {
        uint32_t primarycount   = 0;
        uint32_t secondarycount = 0;
        uint32_t tertiarycount  = 0;
        uint32_t CEi = 1;
        pEndCP = strchr(pStartCP, ']');
        if(pEndCP == NULL) {
            break;
        }
        pStartCP ++;

        primarycount   = readElement(&pStartCP, primary, ',', status);
        secondarycount = readElement(&pStartCP, secondary, ',', status);
        tertiarycount  = readElement(&pStartCP, tertiary, ']', status);

        /* I want to get the CEs entered right here, including continuation */
        ces[count ++] = getSingleCEValue(primary, secondary, tertiary, status);
        if (U_FAILURE(*status)) {
            break;
        }

        while (2 * CEi < primarycount || CEi < secondarycount ||
               CEi < tertiarycount) {
            uint32_t value = UCOL_CONTINUATION_MARKER; /* Continuation marker */
            if (2 * CEi < primarycount) {
                value |= ((hex2num(*(primary + 4 * CEi)) & 0xF) << 28);
                value |= ((hex2num(*(primary + 4 * CEi + 1)) & 0xF) << 24);
            }

            if (2 * CEi + 1 < primarycount) {
                value |= ((hex2num(*(primary + 4 * CEi + 2)) & 0xF) << 20);
                value |= ((hex2num(*(primary + 4 * CEi + 3)) &0xF) << 16);
            }

            if (CEi < secondarycount) {
                value |= ((hex2num(*(secondary + 2 * CEi)) & 0xF) << 12);
                value |= ((hex2num(*(secondary + 2 * CEi + 1)) & 0xF) << 8);
            }

            if (CEi < tertiarycount) {
                value |= ((hex2num(*(tertiary + 2 * CEi)) & 0x3) << 4);
                value |= (hex2num(*(tertiary + 2 * CEi + 1)) & 0xF);
            }

            CEi ++;
            ces[count ++] = value;
        }

      pStartCP = pEndCP + 1;
    }
    ces[count] = 0;
    return pStartCP;
}

/**
* Getting the FractionalUCA.txt file stream
*/
static FileStream * getFractionalUCA(void)
{
    char        newPath[256];
    char        backupPath[256];
    FileStream *result = NULL;

    /* Look inside ICU_DATA first */
    uprv_strcpy(newPath, ctest_dataSrcDir());
    uprv_strcat(newPath, "unidata" U_FILE_SEP_STRING );
    uprv_strcat(newPath, "FractionalUCA.txt");

    /* As a fallback, try to guess where the source data was located
     *   at the time ICU was built, and look there.
     */
#if defined (U_TOPSRCDIR)
    strcpy(backupPath, U_TOPSRCDIR  U_FILE_SEP_STRING "data");
#else
    {
        UErrorCode errorCode = U_ZERO_ERROR;
        strcpy(backupPath, loadTestData(&errorCode));
        strcat(backupPath, U_FILE_SEP_STRING ".." U_FILE_SEP_STRING ".." U_FILE_SEP_STRING ".." U_FILE_SEP_STRING ".." U_FILE_SEP_STRING "data");
    }
#endif
    strcat(backupPath, U_FILE_SEP_STRING "unidata" U_FILE_SEP_STRING "FractionalUCA.txt");

    result = T_FileStream_open(newPath, "rb");

    if (result == NULL) {
        result = T_FileStream_open(backupPath, "rb");
        if (result == NULL) {
            log_err("Failed to open either %s or %s\n", newPath, backupPath);
        }
    }
    return result;
}

/**
* Testing the CEs returned by the iterator
*/
static void TestCEs() {
    FileStream *file = NULL;
    char        line[2048];
    char       *str;
    UChar       codepoints[10];
    uint32_t    ces[20];
    UErrorCode  status = U_ZERO_ERROR;
    UCollator          *coll = ucol_open("", &status);
    uint32_t lineNo = 0;
    UChar       contextCPs[5];    

    if (U_FAILURE(status)) {
        log_err_status(status, "Error in opening root collator -> %s\n", u_errorName(status));
        return;
    }

    file = getFractionalUCA();

    if (file == NULL) {
        log_err("*** unable to open input FractionalUCA.txt file ***\n");
        return;
    }


    while (T_FileStream_readLine(file, line, sizeof(line)) != NULL) {
        int                 count = 0;
        UCollationElements *iter;
        int32_t            preContextCeLen=0;
        lineNo++;
        /* skip this line if it is empty or a comment or is a return value
        or start of some variable section */
        if(line[0] == 0 || line[0] == '#' || line[0] == '\n' ||
            line[0] == 0x000D || line[0] == '[') {
            continue;
        }

        str = getCodePoints(line, codepoints, contextCPs);

        /* these are 'fake' codepoints in the fractional UCA, and are used just 
         * for positioning of indirect values. They should not go through this
         * test.
         */
        if(*codepoints == 0xFDD0) {
          continue;
        }
        if (*contextCPs != 0) {
            iter = ucol_openElements(coll, contextCPs, -1, &status);
            if (U_FAILURE(status)) {
                log_err("Error in opening collation elements\n");
                break;
            }
            while((ces[preContextCeLen] = ucol_next(iter, &status)) != (uint32_t)UCOL_NULLORDER) {
                preContextCeLen++;
            }
            ucol_closeElements(iter);
        }

        getCEs(str, ces+preContextCeLen, &status);
        if (U_FAILURE(status)) {
            log_err("Error in parsing collation elements in FractionalUCA.txt\n");
            break;
        }
        iter = ucol_openElements(coll, codepoints, -1, &status);
        if (U_FAILURE(status)) {
            log_err("Error in opening collation elements\n");
            break;
        }
        for (;;) {
            uint32_t ce = (uint32_t)ucol_next(iter, &status);
            if (ce == 0xFFFFFFFF) {
                ce = 0;
            }
            /* we now unconditionally reorder Thai/Lao prevowels, so this
             * test would fail if we don't skip here.
             */
            if(UCOL_ISTHAIPREVOWEL(*codepoints) && ce == 0 && count == 0) {
              continue;
            }
            if (ce != ces[count] || U_FAILURE(status)) {
                log_err("Collation elements in FractionalUCA.txt and iterators do not match!\n");
                break;
            }
            if (ces[count] == 0) {
                break;
            }
            count ++;
        }
        ucol_closeElements(iter);
    }

    T_FileStream_close(file);
    ucol_close(coll);
}

/**
* Testing the discontigous contractions
*/
static void TestDiscontiguos() {
    const char               *rulestr    =
                            "&z < AB < X\\u0300 < ABC < X\\u0300\\u0315";
          UChar               rule[50];
          int                 rulelen = u_unescape(rulestr, rule, 50);
    const char               *src[] = {
     "ADB", "ADBC", "A\\u0315B", "A\\u0315BC",
    /* base character blocked */
     "XD\\u0300", "XD\\u0300\\u0315",
    /* non blocking combining character */
     "X\\u0319\\u0300", "X\\u0319\\u0300\\u0315",
     /* blocking combining character */
     "X\\u0314\\u0300", "X\\u0314\\u0300\\u0315",
     /* contraction prefix */
     "ABDC", "AB\\u0315C","X\\u0300D\\u0315", "X\\u0300\\u0319\\u0315",
     "X\\u0300\\u031A\\u0315",
     /* ends not with a contraction character */
     "X\\u0319\\u0300D", "X\\u0319\\u0300\\u0315D", "X\\u0300D\\u0315D",
     "X\\u0300\\u0319\\u0315D", "X\\u0300\\u031A\\u0315D"
    };
    const char               *tgt[] = {
     /* non blocking combining character */
     "A D B", "A D BC", "A \\u0315 B", "A \\u0315 BC",
    /* base character blocked */
     "X D \\u0300", "X D \\u0300\\u0315",
    /* non blocking combining character */
     "X\\u0300 \\u0319", "X\\u0300\\u0315 \\u0319",
     /* blocking combining character */
     "X \\u0314 \\u0300", "X \\u0314 \\u0300\\u0315",
     /* contraction prefix */
     "AB DC", "AB \\u0315 C","X\\u0300 D \\u0315", "X\\u0300\\u0315 \\u0319",
     "X\\u0300 \\u031A \\u0315",
     /* ends not with a contraction character */
     "X\\u0300 \\u0319D", "X\\u0300\\u0315 \\u0319D", "X\\u0300 D\\u0315D",
     "X\\u0300\\u0315 \\u0319D", "X\\u0300 \\u031A\\u0315D"
    };
          int                 size   = 20;
          UCollator          *coll;
          UErrorCode          status    = U_ZERO_ERROR;
          int                 count     = 0;
          UCollationElements *iter;
          UCollationElements *resultiter;

    coll       = ucol_openRules(rule, rulelen, UCOL_OFF, UCOL_DEFAULT_STRENGTH,NULL, &status);
    iter       = ucol_openElements(coll, rule, 1, &status);
    resultiter = ucol_openElements(coll, rule, 1, &status);

    if (U_FAILURE(status)) {
        log_err_status(status, "Error opening collation rules -> %s\n", u_errorName(status));
        return;
    }

    while (count < size) {
        UChar  str[20];
        UChar  tstr[20];
        int    strLen = u_unescape(src[count], str, 20);
        UChar *s;

        ucol_setText(iter, str, strLen, &status);
        if (U_FAILURE(status)) {
            log_err("Error opening collation iterator\n");
            return;
        }

        u_unescape(tgt[count], tstr, 20);
        s = tstr;

        log_verbose("count %d\n", count);

        for (;;) {
            uint32_t  ce;
            UChar    *e = u_strchr(s, 0x20);
            if (e == 0) {
                e = u_strchr(s, 0);
            }
            ucol_setText(resultiter, s, (int32_t)(e - s), &status);
            ce = ucol_next(resultiter, &status);
            if (U_FAILURE(status)) {
                log_err("Error manipulating collation iterator\n");
                return;
            }
            while (ce != UCOL_NULLORDER) {
                if (ce != (uint32_t)ucol_next(iter, &status) ||
                    U_FAILURE(status)) {
                    log_err("Discontiguos contraction test mismatch\n");
                    return;
                }
                ce = ucol_next(resultiter, &status);
                if (U_FAILURE(status)) {
                    log_err("Error getting next collation element\n");
                    return;
                }
            }
            s = e + 1;
            if (*e == 0) {
                break;
            }
        }
        ucol_reset(iter);
        backAndForth(iter);
        count ++;
    }
    ucol_closeElements(resultiter);
    ucol_closeElements(iter);
    ucol_close(coll);
}

static void TestCEBufferOverflow()
{
    UChar               str[UCOL_EXPAND_CE_BUFFER_SIZE + 1];
    UErrorCode          status = U_ZERO_ERROR;
    UChar               rule[10];
    UCollator          *coll;
    UCollationElements *iter;

    u_uastrcpy(rule, "&z < AB");
    coll = ucol_openRules(rule, u_strlen(rule), UCOL_OFF, UCOL_DEFAULT_STRENGTH, NULL,&status);
    if (U_FAILURE(status)) {
        log_err_status(status, "Rule based collator not created for testing ce buffer overflow -> %s\n", u_errorName(status));
        return;
    }

    /* 0xDCDC is a trail surrogate hence deemed unsafe by the heuristic
    test. this will cause an overflow in getPrev */
    str[0] = 0x0041;    /* 'A' */
    /*uprv_memset(str + 1, 0xE0, sizeof(UChar) * UCOL_EXPAND_CE_BUFFER_SIZE);*/
    uprv_memset(str + 1, 0xDC, sizeof(UChar) * UCOL_EXPAND_CE_BUFFER_SIZE);
    str[UCOL_EXPAND_CE_BUFFER_SIZE] = 0x0042;   /* 'B' */
    iter = ucol_openElements(coll, str, UCOL_EXPAND_CE_BUFFER_SIZE + 1,
                             &status);
    if (ucol_previous(iter, &status) == UCOL_NULLORDER ||
        status == U_BUFFER_OVERFLOW_ERROR) {
        log_err("CE buffer should not overflow with long string of trail surrogates\n");
    }
    ucol_closeElements(iter);
    ucol_close(coll);
}

/**
* Checking collation element validity.
*/
#define MAX_CODEPOINTS_TO_SHOW 10
static void showCodepoints(const UChar *codepoints, int length, char * codepointText) {
    int i, lengthToUse = length;
    if (lengthToUse > MAX_CODEPOINTS_TO_SHOW) {
        lengthToUse = MAX_CODEPOINTS_TO_SHOW;
    }
    for (i = 0; i < lengthToUse; ++i) {
        int bytesWritten = sprintf(codepointText, " %04X", *codepoints++);
        if (bytesWritten <= 0) {
            break;
        }
        codepointText += bytesWritten;
    }
    if (i < length) {
        sprintf(codepointText, " ...");
    }
}

static UBool checkCEValidity(const UCollator *coll, const UChar *codepoints,
                             int length)
{
    UErrorCode          status = U_ZERO_ERROR;
    UCollationElements *iter   = ucol_openElements(coll, codepoints, length,
                                                  &status);
    UBool result = FALSE;
    UBool primaryDone = FALSE, secondaryDone = FALSE, tertiaryDone = FALSE;
    const char * collLocale;

    if (U_FAILURE(status)) {
        log_err("Error creating iterator for testing validity\n");
        return FALSE;
    }
    collLocale = ucol_getLocale(coll, ULOC_VALID_LOCALE, &status);
    if (U_FAILURE(status) || collLocale==NULL) {
        status = U_ZERO_ERROR;
        collLocale = "?";
    }

    for (;;) {
        uint32_t ce = ucol_next(iter, &status);
        uint32_t primary, p1, p2, secondary, tertiary;
        if (ce == UCOL_NULLORDER) {
            result = TRUE;
            break;
        }
        if (ce == 0) {
            continue;
        }
        if (ce == 0x02000202) {
            /* special CE for merge-sort character */
            if (*codepoints == 0xFFFE /* && length == 1 */) {
                /*
                 * Note: We should check for length==1 but the token parser appears
                 * to give us trailing NUL characters.
                 * TODO: Ticket #8047: Change TestCEValidity to use ucol_getTailoredSet()
                 *                     rather than the internal collation rule parser
                 */
                continue;
            } else {
                log_err("Special 02/02/02 weight for code point U+%04X [len %d] != U+FFFE\n",
                        (int)*codepoints, (int)length);
                break;
            }
        }
        primary   = UCOL_PRIMARYORDER(ce);
        p1 = primary >> 8;
        p2 = primary & 0xFF;
        secondary = UCOL_SECONDARYORDER(ce);
        tertiary  = UCOL_TERTIARYORDER(ce) & UCOL_REMOVE_CONTINUATION;

        if (!isContinuation(ce)) {
            if ((ce & UCOL_REMOVE_CONTINUATION) == 0) {
                log_err("Empty CE %08lX except for case bits\n", (long)ce);
                break;
            }
            if (p1 == 0) {
                if (p2 != 0) {
                    log_err("Primary 00 xx in %08lX\n", (long)ce);
                    break;
                }
                primaryDone = TRUE;
            } else {
                if (p1 <= 2 || p1 >= 0xF0) {
                    /* Primary first bytes F0..FF are specials. */
                    log_err("Primary first byte of %08lX out of range\n", (long)ce);
                    break;
                }
                if (p2 == 0) {
                    primaryDone = TRUE;
                } else {
                    if (p2 <= 3 || p2 >= 0xFF) {
                        /* Primary second bytes 03 and FF are sort key compression terminators. */
                        log_err("Primary second byte of %08lX out of range\n", (long)ce);
                        break;
                    }
                    primaryDone = FALSE;
                }
            }
            if (secondary == 0) {
                if (primary != 0) {
                    log_err("Primary!=0 secondary==0 in %08lX\n", (long)ce);
                    break;
                }
                secondaryDone = TRUE;
            } else {
                if (secondary <= 2 ||
                    (UCOL_BYTE_COMMON < secondary && secondary <= (UCOL_BYTE_COMMON + 0x80))
                ) {
                    /* Secondary first bytes common+1..+0x80 are used for sort key compression. */
                    log_err("Secondary byte of %08lX out of range\n", (long)ce);
                    break;
                }
                secondaryDone = FALSE;
            }
            if (tertiary == 0) {
                /* We know that ce != 0. */
                log_err("Primary!=0 or secondary!=0 but tertiary==0 in %08lX\n", (long)ce);
                break;
            }
            if (tertiary <= 2) {
                log_err("Tertiary byte of %08lX out of range\n", (long)ce);
                break;
            }
            tertiaryDone = FALSE;
        } else {
            if ((ce & UCOL_REMOVE_CONTINUATION) == 0) {
                log_err("Empty continuation %08lX\n", (long)ce);
                break;
            }
            if (primaryDone && primary != 0) {
                log_err("Primary was done but continues in %08lX\n", (long)ce);
                break;
            }
            if (p1 == 0) {
                if (p2 != 0) {
                    log_err("Primary 00 xx in %08lX\n", (long)ce);
                    break;
                }
                primaryDone = TRUE;
            } else {
                if (p1 <= 2) {
                    log_err("Primary first byte of %08lX out of range\n", (long)ce);
                    break;
                }
                if (p2 == 0) {
                    primaryDone = TRUE;
                } else {
                    if (p2 <= 3) {
                        log_err("Primary second byte of %08lX out of range\n", (long)ce);
                        break;
                    }
                }
            }
            if (secondaryDone && secondary != 0) {
                log_err("Secondary was done but continues in %08lX\n", (long)ce);
                break;
            }
            if (secondary == 0) {
                secondaryDone = TRUE;
            } else {
                if (secondary <= 2) {
                    log_err("Secondary byte of %08lX out of range\n", (long)ce);
                    break;
                }
            }
            if (tertiaryDone && tertiary != 0) {
                log_err("Tertiary was done but continues in %08lX\n", (long)ce);
                break;
            }
            if (tertiary == 0) {
                tertiaryDone = TRUE;
            } else if (tertiary <= 2) {
                log_err("Tertiary byte of %08lX out of range\n", (long)ce);
                break;
            }
        }
    }
    if (!result) {
        char codepointText[5*MAX_CODEPOINTS_TO_SHOW + 5];
        showCodepoints(codepoints, length, codepointText);
        log_err("Locale: %s  Code point string: %s\n", collLocale, codepointText);
    }
    ucol_closeElements(iter);
    return result;
}

static void TestCEValidity()
{
    /* testing UCA collation elements */
    UErrorCode  status      = U_ZERO_ERROR;
    /* en_US has no tailorings */
    UCollator  *coll        = ucol_open("root", &status);
    /* tailored locales */
    char        locale[][11] = {"fr_FR", "ko_KR", "sh_YU", "th_TH", "zh_CN", "zh__PINYIN"};
    const char *loc;
    FileStream *file = NULL;
    char        line[2048];
    UChar       codepoints[11];
    int         count = 0;
    int         maxCount = 0;
    UChar       contextCPs[3];
    UChar32     c;
    UParseError parseError;
    if (U_FAILURE(status)) {
        log_err_status(status, "en_US collator creation failed -> %s\n", u_errorName(status));
        return;
    }
    log_verbose("Testing UCA elements\n");
    file = getFractionalUCA();
    if (file == NULL) {
        log_err("Fractional UCA data can not be opened\n");
        return;
    }

    while (T_FileStream_readLine(file, line, sizeof(line)) != NULL) {
        if(line[0] == 0 || line[0] == '#' || line[0] == '\n' ||
            line[0] == 0x000D || line[0] == '[') {
            continue;
        }

        getCodePoints(line, codepoints, contextCPs);
        checkCEValidity(coll, codepoints, u_strlen(codepoints));
    }

    log_verbose("Testing UCA elements for the whole range of unicode characters\n");
    for (c = 0; c <= 0xffff; ++c) {
        if (u_isdefined(c)) {
            codepoints[0] = (UChar)c;
            checkCEValidity(coll, codepoints, 1);
        }
    }
    for (; c <= 0x10ffff; ++c) {
        if (u_isdefined(c)) {
            int32_t i = 0;
            U16_APPEND_UNSAFE(codepoints, i, c);
            checkCEValidity(coll, codepoints, i);
        }
    }

    ucol_close(coll);

    /* testing tailored collation elements */
    log_verbose("Testing tailored elements\n");
    if(getTestOption(QUICK_OPTION)) {
        maxCount = sizeof(locale)/sizeof(locale[0]);
    } else {
        maxCount = uloc_countAvailable();
    }
    while (count < maxCount) {
        const UChar *rules = NULL,
                    *current = NULL;
        UChar *rulesCopy = NULL;
        int32_t ruleLen = 0;

        uint32_t chOffset = 0;
        uint32_t chLen = 0;
        uint32_t exOffset = 0;
        uint32_t exLen = 0;
        uint32_t prefixOffset = 0;
        uint32_t prefixLen = 0;
        UBool    startOfRules = TRUE;
        UColOptionSet opts;

        UColTokenParser src;
        uint32_t strength = 0;
        uint16_t specs = 0;
        if(getTestOption(QUICK_OPTION)) {
            loc = locale[count];
        } else {
            loc = uloc_getAvailable(count);
            if(!hasCollationElements(loc)) {
                count++;
                continue;
            }
        }

        uprv_memset(&src, 0, sizeof(UColTokenParser));

        log_verbose("Testing CEs for %s\n", loc);

        coll      = ucol_open(loc, &status);
        if (U_FAILURE(status)) {
            log_err("%s collator creation failed\n", loc);
            return;
        }

        src.opts = &opts;
        rules = ucol_getRules(coll, &ruleLen);

        if (ruleLen > 0) {
            rulesCopy = (UChar *)uprv_malloc((ruleLen +
                UCOL_TOK_EXTRA_RULE_SPACE_SIZE) * sizeof(UChar));
            uprv_memcpy(rulesCopy, rules, ruleLen * sizeof(UChar));
            src.current = src.source = rulesCopy;
            src.end = rulesCopy + ruleLen;
            src.extraCurrent = src.end;
            src.extraEnd = src.end + UCOL_TOK_EXTRA_RULE_SPACE_SIZE;

	        /* Note that as a result of tickets 7015 or 6912, ucol_tok_parseNextToken can cause the pointer to
	           the rules copy in src.source to get reallocated, freeing the original pointer in rulesCopy */
            while ((current = ucol_tok_parseNextToken(&src, startOfRules, &parseError,&status)) != NULL) {
              strength = src.parsedToken.strength;
              chOffset = src.parsedToken.charsOffset;
              chLen = src.parsedToken.charsLen;
              exOffset = src.parsedToken.extensionOffset;
              exLen = src.parsedToken.extensionLen;
              prefixOffset = src.parsedToken.prefixOffset;
              prefixLen = src.parsedToken.prefixLen;
              specs = src.parsedToken.flags;

                startOfRules = FALSE;
                uprv_memcpy(codepoints, src.source + chOffset,
                                                       chLen * sizeof(UChar));
                codepoints[chLen] = 0;
                checkCEValidity(coll, codepoints, chLen);
            }
            uprv_free(src.source);
        }

        ucol_close(coll);
        count ++;
    }
    T_FileStream_close(file);
}

static void printSortKeyError(const UChar   *codepoints, int length,
                                    uint8_t *sortkey, int sklen)
{
    int count = 0;
    log_err("Sortkey not valid for ");
    while (length > 0) {
        log_err("0x%04x ", *codepoints);
        length --;
        codepoints ++;
    }
    log_err("\nSortkey : ");
    while (count < sklen) {
        log_err("0x%02x ", sortkey[count]);
        count ++;
    }
    log_err("\n");
}

/**
* Checking sort key validity for all levels
*/
static UBool checkSortKeyValidity(UCollator *coll,
                                  const UChar *codepoints,
                                  int length)
{
    UErrorCode status  = U_ZERO_ERROR;
    UCollationStrength strength[5] = {UCOL_PRIMARY, UCOL_SECONDARY,
                                      UCOL_TERTIARY, UCOL_QUATERNARY,
                                      UCOL_IDENTICAL};
    int        strengthlen = 5;
    int        strengthIndex = 0;
    int        caselevel   = 0;

    while (caselevel < 1) {
        if (caselevel == 0) {
            ucol_setAttribute(coll, UCOL_CASE_LEVEL, UCOL_OFF, &status);
        }
        else {
            ucol_setAttribute(coll, UCOL_CASE_LEVEL, UCOL_ON, &status);
        }

        while (strengthIndex < strengthlen) {
            int        count01 = 0;
            uint32_t   count   = 0;
            uint8_t    sortkey[128];
            uint32_t   sklen;

            ucol_setStrength(coll, strength[strengthIndex]);
            sklen = ucol_getSortKey(coll, codepoints, length, sortkey, 128);
            while (sortkey[count] != 0) {
                if (sortkey[count] == 2 || (sortkey[count] == 3 && count01 > 0 && strengthIndex != 4)) {
                    printSortKeyError(codepoints, length, sortkey, sklen);
                    return FALSE;
                }
                if (sortkey[count] == 1) {
                    count01 ++;
                }
                count ++;
            }

            if (count + 1 != sklen || (count01 != strengthIndex + caselevel)) {
                printSortKeyError(codepoints, length, sortkey, sklen);
                return FALSE;
            }
            strengthIndex ++;
        }
        caselevel ++;
    }
    return TRUE;
}

static void TestSortKeyValidity(void)
{
    /* testing UCA collation elements */
    UErrorCode  status      = U_ZERO_ERROR;
    /* en_US has no tailorings */
    UCollator  *coll        = ucol_open("en_US", &status);
    /* tailored locales */
    char        locale[][6] = {"fr_FR", "ko_KR", "sh_YU", "th_TH", "zh_CN"};
    FileStream *file = NULL;
    char        line[2048];
    UChar       codepoints[10];
    int         count = 0;
    UChar       contextCPs[5];
    UParseError parseError;
    if (U_FAILURE(status)) {
        log_err_status(status, "en_US collator creation failed -> %s\n", u_errorName(status));
        return;
    }
    log_verbose("Testing UCA elements\n");
    file = getFractionalUCA();
    if (file == NULL) {
        log_err("Fractional UCA data can not be opened\n");
        return;
    }

    while (T_FileStream_readLine(file, line, sizeof(line)) != NULL) {
        if(line[0] == 0 || line[0] == '#' || line[0] == '\n' ||
            line[0] == 0x000D || line[0] == '[') {
            continue;
        }

        getCodePoints(line, codepoints, contextCPs);
        if(codepoints[0] == 0xFFFE) {
            /* Skip special merge-sort character U+FFFE which has otherwise illegal 02 weight bytes. */
            continue;
        }
        checkSortKeyValidity(coll, codepoints, u_strlen(codepoints));
    }

    log_verbose("Testing UCA elements for the whole range of unicode characters\n");
    codepoints[0] = 0;

    while (codepoints[0] < 0xFFFF) {
        if (u_isdefined((UChar32)codepoints[0])) {
            checkSortKeyValidity(coll, codepoints, 1);
        }
        codepoints[0] ++;
    }

    ucol_close(coll);

    /* testing tailored collation elements */
    log_verbose("Testing tailored elements\n");
    while (count < 5) {
        const UChar *rules = NULL,
                    *current = NULL;
        UChar *rulesCopy = NULL;
        int32_t ruleLen = 0;

        uint32_t chOffset = 0;
        uint32_t chLen = 0;
        uint32_t exOffset = 0;
        uint32_t exLen = 0;
        uint32_t prefixOffset = 0;
        uint32_t prefixLen = 0;
        UBool    startOfRules = TRUE;
        UColOptionSet opts;

        UColTokenParser src;
        uint32_t strength = 0;
        uint16_t specs = 0;

        uprv_memset(&src, 0, sizeof(UColTokenParser));

        coll      = ucol_open(locale[count], &status);
        if (U_FAILURE(status)) {
            log_err("%s collator creation failed\n", locale[count]);
            return;
        }

        src.opts = &opts;
        rules = ucol_getRules(coll, &ruleLen);

        if (ruleLen > 0) {
            rulesCopy = (UChar *)uprv_malloc((ruleLen +
                UCOL_TOK_EXTRA_RULE_SPACE_SIZE) * sizeof(UChar));
            uprv_memcpy(rulesCopy, rules, ruleLen * sizeof(UChar));
            src.current = src.source = rulesCopy;
            src.end = rulesCopy + ruleLen;
            src.extraCurrent = src.end;
            src.extraEnd = src.end + UCOL_TOK_EXTRA_RULE_SPACE_SIZE;

	        /* Note that as a result of tickets 7015 or 6912, ucol_tok_parseNextToken can cause the pointer to
	           the rules copy in src.source to get reallocated, freeing the original pointer in rulesCopy */
            while ((current = ucol_tok_parseNextToken(&src, startOfRules,&parseError, &status)) != NULL) {
                strength = src.parsedToken.strength;
                chOffset = src.parsedToken.charsOffset;
                chLen = src.parsedToken.charsLen;
                exOffset = src.parsedToken.extensionOffset;
                exLen = src.parsedToken.extensionLen;
                prefixOffset = src.parsedToken.prefixOffset;
                prefixLen = src.parsedToken.prefixLen;
                specs = src.parsedToken.flags;

                startOfRules = FALSE;
                uprv_memcpy(codepoints, src.source + chOffset,
                                                       chLen * sizeof(UChar));
                codepoints[chLen] = 0;
                if(codepoints[0] == 0xFFFE) {
                    /* Skip special merge-sort character U+FFFE which has otherwise illegal 02 weight bytes. */
                    continue;
                }
                checkSortKeyValidity(coll, codepoints, chLen);
            }
            uprv_free(src.source);
        }

        ucol_close(coll);
        count ++;
    }
    T_FileStream_close(file);
}

/**
* TestSearchCollatorElements tests iterator behavior (forwards and backwards) with
* normalization on AND jamo tailoring, among other things.
*/
static const UChar tsceText[] = {   /* Nothing in here should be ignorable */
    0x0020, 0xAC00,                 /* simple LV Hangul */
    0x0020, 0xAC01,                 /* simple LVT Hangul */
    0x0020, 0xAC0F,                 /* LVTT, last jamo expands for search */
    0x0020, 0xAFFF,                 /* LLVVVTT, every jamo expands for search */
    0x0020, 0x1100, 0x1161, 0x11A8, /* 0xAC01 as conjoining jamo */
    0x0020, 0x3131, 0x314F, 0x3131, /* 0xAC01 as compatibility jamo */
    0x0020, 0x1100, 0x1161, 0x11B6, /* 0xAC0F as conjoining jamo; last expands for search */
    0x0020, 0x1101, 0x1170, 0x11B6, /* 0xAFFF as conjoining jamo; all expand for search */
    0x0020, 0x00E6,                 /* small letter ae, expands */
    0x0020, 0x1E4D,                 /* small letter o with tilde and acute, decomposes */
    0x0020
};
enum { kLen_tsceText = sizeof(tsceText)/sizeof(tsceText[0]) };

static const int32_t rootStandardOffsets[] = {
    0,  1,2,
    2,  3,4,4,
    4,  5,6,6,
    6,  7,8,8,
    8,  9,10,11,
    12, 13,14,15,
    16, 17,18,19,
    20, 21,22,23,
    24, 25,26,26,26,
    26, 27,28,28,
    28,
    29
};
enum { kLen_rootStandardOffsets = sizeof(rootStandardOffsets)/sizeof(rootStandardOffsets[0]) };

static const int32_t rootSearchOffsets[] = {
    0,  1,2,
    2,  3,4,4,
    4,  5,6,6,6,
    6,  7,8,8,8,8,8,8,
    8,  9,10,11,
    12, 13,14,15,
    16, 17,18,19,20,
    20, 21,22,22,23,23,23,24,
    24, 25,26,26,26,
    26, 27,28,28,
    28,
    29
};
enum { kLen_rootSearchOffsets = sizeof(rootSearchOffsets)/sizeof(rootSearchOffsets[0]) };

typedef struct {
    const char *    locale;
    const int32_t * offsets;
    int32_t         offsetsLen;
} TSCEItem;

static const TSCEItem tsceItems[] = {
    { "root",                  rootStandardOffsets, kLen_rootStandardOffsets },
    { "root@collation=search", rootSearchOffsets,   kLen_rootSearchOffsets   },
    { NULL,                    NULL,                0                        }
};

static void TestSearchCollatorElements(void)
{
    const TSCEItem * tsceItemPtr;
    for (tsceItemPtr = tsceItems; tsceItemPtr->locale != NULL; tsceItemPtr++) {
        UErrorCode status = U_ZERO_ERROR;
        UCollator* ucol = ucol_open(tsceItemPtr->locale, &status);
        if ( U_SUCCESS(status) ) {
            UCollationElements * uce = ucol_openElements(ucol, tsceText, kLen_tsceText, &status);
            if ( U_SUCCESS(status) ) {
                int32_t offset, element;
                const int32_t * nextOffsetPtr;
                const int32_t * limitOffsetPtr;

                nextOffsetPtr = tsceItemPtr->offsets;
                limitOffsetPtr = tsceItemPtr->offsets + tsceItemPtr->offsetsLen;
                do {
                    offset = ucol_getOffset(uce);
                    element = ucol_next(uce, &status);
                    if ( element == 0 ) {
                        log_err("error, locale %s, ucol_next returned element 0\n", tsceItemPtr->locale );
                    }
                    if ( nextOffsetPtr < limitOffsetPtr ) {
                        if (offset != *nextOffsetPtr) {
                            log_err("error, locale %s, expected ucol_next -> ucol_getOffset %d, got %d\n",
                                                            tsceItemPtr->locale, *nextOffsetPtr, offset );
                            nextOffsetPtr = limitOffsetPtr;
                            break;
                        }
                        nextOffsetPtr++;
                    } else {
                        log_err("error, locale %s, ucol_next returned more elements than expected\n", tsceItemPtr->locale );
                    }
                } while ( U_SUCCESS(status) && element != UCOL_NULLORDER );
                if ( nextOffsetPtr < limitOffsetPtr ) {
                    log_err("error, locale %s, ucol_next returned fewer elements than expected\n", tsceItemPtr->locale );
                }

                ucol_setOffset(uce, kLen_tsceText, &status);
                status = U_ZERO_ERROR;
                nextOffsetPtr = tsceItemPtr->offsets + tsceItemPtr->offsetsLen;
                limitOffsetPtr = tsceItemPtr->offsets;
                do {
                    offset = ucol_getOffset(uce);
                    element = ucol_previous(uce, &status);
                    if ( element == 0 ) {
                        log_err("error, locale %s, ucol_previous returned element 0\n", tsceItemPtr->locale );
                    }
                    if ( nextOffsetPtr > limitOffsetPtr ) {
                        nextOffsetPtr--;
                        if (offset != *nextOffsetPtr) {
                            log_err("error, locale %s, expected ucol_previous -> ucol_getOffset %d, got %d\n",
                                                                tsceItemPtr->locale, *nextOffsetPtr, offset );
                            nextOffsetPtr = limitOffsetPtr;
                            break;
                        }
                   } else {
                        log_err("error, locale %s, ucol_previous returned more elements than expected\n", tsceItemPtr->locale );
                    }
                } while ( U_SUCCESS(status) && element != UCOL_NULLORDER );
                if ( nextOffsetPtr > limitOffsetPtr ) {
                    log_err("error, locale %s, ucol_previous returned fewer elements than expected\n", tsceItemPtr->locale );
                }

                ucol_closeElements(uce);
            } else {
                log_err("error, locale %s, ucol_openElements failed: %s\n", tsceItemPtr->locale, u_errorName(status) );
            }
            ucol_close(ucol);
        } else {
            log_err("error, locale %s, ucol_open failed: %s\n", tsceItemPtr->locale, u_errorName(status) );
        }
    }
}

#endif /* #if !UCONFIG_NO_COLLATION */
