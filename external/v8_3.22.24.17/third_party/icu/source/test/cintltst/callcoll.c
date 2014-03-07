/********************************************************************
 * COPYRIGHT: 
 * Copyright (c) 1997-2010, International Business Machines Corporation and
 * others. All Rights Reserved.
 ********************************************************************/
/*******************************************************************************
*
* File CALLCOLL.C
*
* Modification History:
*        Name                     Description            
*     Madhu Katragadda              Ported for C API
********************************************************************************
*/

/*
 * Important: This file is included into intltest/allcoll.cpp so that the
 * test data is shared. This makes it easier to maintain the test data,
 * especially since the Unicode data must be portable and quoted character
 * literals will not work.
 * If it is included, then there will be a #define INCLUDE_CALLCOLL_C
 * that must prevent the actual code in here from being part of the
 * allcoll.cpp compilation.
 */

/**
 * CollationDummyTest is a third level test class.  This tests creation of 
 * a customized collator object.  For example, number 1 to be sorted 
 * equlivalent to word 'one'. 
 */

#include <string.h>
#include <stdlib.h>

#include "unicode/utypes.h"

#if !UCONFIG_NO_COLLATION

#include "unicode/ucol.h"
#include "unicode/uloc.h"
#include "unicode/ures.h"
#include "unicode/udata.h"
#include "unicode/ucoleitr.h"
#include "unicode/ustring.h"
#include "unicode/uclean.h"
#include "unicode/putil.h"
#include "unicode/uenum.h"

#include "cintltst.h"
#include "ccolltst.h"
#include "callcoll.h"
#include "calldata.h"
#include "cstring.h"
#include "cmemory.h"

/* set to 1 to test offsets in backAndForth() */
#define TEST_OFFSETS 0

/* perform test with strength PRIMARY */
static void TestPrimary(void);

/* perform test with strength SECONDARY */
static void TestSecondary(void);

/* perform test with strength tertiary */
static void TestTertiary(void);

/*perform tests with strength Identical */
static void TestIdentical(void);

/* perform extra tests */
static void TestExtra(void);

/* Test jitterbug 581 */
static void TestJB581(void);

/* Test jitterbug 1401 */
static void TestJB1401(void);

/* Test [variable top] in the rule syntax */
static void TestVariableTop(void);

/* Test surrogates */
static void TestSurrogates(void);

static void TestInvalidRules(void);

static void TestJitterbug1098(void);

static void TestFCDCrash(void);

static void TestJ5298(void);

const UCollationResult results[] = {
    UCOL_LESS,
    UCOL_LESS, /*UCOL_GREATER,*/
    UCOL_LESS,
    UCOL_LESS,
    UCOL_LESS,
    UCOL_LESS,
    UCOL_LESS,
    UCOL_GREATER,
    UCOL_GREATER,
    UCOL_LESS,                                     /*  10 */
    UCOL_GREATER,
    UCOL_LESS,
    UCOL_GREATER,
    UCOL_GREATER,
    UCOL_LESS,
    UCOL_LESS,
    UCOL_LESS,
    /*  test primary > 17 */
    UCOL_EQUAL,
    UCOL_EQUAL,
    UCOL_EQUAL,                                    /*  20 */
    UCOL_LESS,
    UCOL_LESS,
    UCOL_EQUAL,
    UCOL_EQUAL,
    UCOL_EQUAL,
    UCOL_LESS,
    /*  test secondary > 26 */
    UCOL_EQUAL,
    UCOL_EQUAL,
    UCOL_EQUAL,
    UCOL_EQUAL,
    UCOL_EQUAL,                                    /*  30 */
    UCOL_EQUAL,
    UCOL_LESS,
    UCOL_EQUAL,                                     /*  34 */
    UCOL_EQUAL,
    UCOL_EQUAL,
    UCOL_LESS                                        /* 37 */
};


static
void uprv_appendByteToHexString(char *dst, uint8_t val) {
  uint32_t len = (uint32_t)uprv_strlen(dst);
  *(dst+len) = T_CString_itosOffset((val >> 4));
  *(dst+len+1) = T_CString_itosOffset((val & 0xF));
  *(dst+len+2) = 0;
}

/* this function makes a string with representation of a sortkey */
static char* U_EXPORT2 sortKeyToString(const UCollator *coll, const uint8_t *sortkey, char *buffer, uint32_t *len) {
    int32_t strength = UCOL_PRIMARY;
    uint32_t res_size = 0;
    UBool doneCase = FALSE;
    UErrorCode errorCode = U_ZERO_ERROR;

    char *current = buffer;
    const uint8_t *currentSk = sortkey;

    uprv_strcpy(current, "[");

    while(strength <= UCOL_QUATERNARY && strength <= ucol_getStrength(coll)) {
        if(strength > UCOL_PRIMARY) {
            uprv_strcat(current, " . ");
        }
        while(*currentSk != 0x01 && *currentSk != 0x00) { /* print a level */
            uprv_appendByteToHexString(current, *currentSk++);
            uprv_strcat(current, " ");
        }
        if(ucol_getAttribute(coll, UCOL_CASE_LEVEL, &errorCode) == UCOL_ON && strength == UCOL_SECONDARY && doneCase == FALSE) {
            doneCase = TRUE;
        } else if(ucol_getAttribute(coll, UCOL_CASE_LEVEL, &errorCode) == UCOL_OFF || doneCase == TRUE || strength != UCOL_SECONDARY) {
            strength ++;
        }
        if (*currentSk) {
            uprv_appendByteToHexString(current, *currentSk++); /* This should print '01' */
        }
        if(strength == UCOL_QUATERNARY && ucol_getAttribute(coll, UCOL_ALTERNATE_HANDLING, &errorCode) == UCOL_NON_IGNORABLE) {
            break;
        }
    }

    if(ucol_getStrength(coll) == UCOL_IDENTICAL) {
        uprv_strcat(current, " . ");
        while(*currentSk != 0) {
            uprv_appendByteToHexString(current, *currentSk++);
            uprv_strcat(current, " ");
        }

        uprv_appendByteToHexString(current, *currentSk++);
    }
    uprv_strcat(current, "]");

    if(res_size > *len) {
        return NULL;
    }

    return buffer;
}

void addAllCollTest(TestNode** root)
{
    addTest(root, &TestPrimary, "tscoll/callcoll/TestPrimary");
    addTest(root, &TestSecondary, "tscoll/callcoll/TestSecondary");
    addTest(root, &TestTertiary, "tscoll/callcoll/TestTertiary");
    addTest(root, &TestIdentical, "tscoll/callcoll/TestIdentical");
    addTest(root, &TestExtra, "tscoll/callcoll/TestExtra");
    addTest(root, &TestJB581, "tscoll/callcoll/TestJB581");
    addTest(root, &TestVariableTop, "tscoll/callcoll/TestVariableTop");
    addTest(root, &TestSurrogates, "tscoll/callcoll/TestSurrogates");
    addTest(root, &TestInvalidRules, "tscoll/callcoll/TestInvalidRules");
    addTest(root, &TestJB1401, "tscoll/callcoll/TestJB1401");
    addTest(root, &TestJitterbug1098, "tscoll/callcoll/TestJitterbug1098");
    addTest(root, &TestFCDCrash, "tscoll/callcoll/TestFCDCrash");
    addTest(root, &TestJ5298, "tscoll/callcoll/TestJ5298");
}

UBool hasCollationElements(const char *locName) {

  UErrorCode status = U_ZERO_ERROR;

  UResourceBundle *loc = ures_open(U_ICUDATA_NAME U_TREE_SEPARATOR_STRING "coll", locName, &status);;

  if(U_SUCCESS(status)) {
    status = U_ZERO_ERROR;
    loc = ures_getByKey(loc, "collations", loc, &status);
    ures_close(loc);
    if(status == U_ZERO_ERROR) { /* do the test - there are real elements */
      return TRUE;
    }
  }
  return FALSE;
}

static UCollationResult compareUsingPartials(UCollator *coll, const UChar source[], int32_t sLen, const UChar target[], int32_t tLen, int32_t pieceSize, UErrorCode *status) {
  int32_t partialSKResult = 0;
  UCharIterator sIter, tIter;
  uint32_t sState[2], tState[2];
  int32_t sSize = pieceSize, tSize = pieceSize;
  /*int32_t i = 0;*/
  uint8_t sBuf[16384], tBuf[16384];
  if(pieceSize > 16384) {
    log_err("Partial sortkey size buffer too small. Please consider increasing the buffer!\n");
    *status = U_BUFFER_OVERFLOW_ERROR;
    return UCOL_EQUAL;
  }
  *status = U_ZERO_ERROR;
  sState[0] = 0; sState[1] = 0;
  tState[0] = 0; tState[1] = 0;
  while(sSize == pieceSize && tSize == pieceSize && partialSKResult == 0) {
    uiter_setString(&sIter, source, sLen);
    uiter_setString(&tIter, target, tLen);
    sSize = ucol_nextSortKeyPart(coll, &sIter, sState, sBuf, pieceSize, status);
    tSize = ucol_nextSortKeyPart(coll, &tIter, tState, tBuf, pieceSize, status);
    
    if(sState[0] != 0 || tState[0] != 0) {
      /*log_verbose("State != 0 : %08X %08X\n", sState[0], tState[0]);*/
    }
    /*log_verbose("%i ", i++);*/
    
    partialSKResult = memcmp(sBuf, tBuf, pieceSize);
  }

  if(partialSKResult < 0) {
      return UCOL_LESS;
  } else if(partialSKResult > 0) {
    return UCOL_GREATER;
  } else {
    return UCOL_EQUAL;
  }
}

static void doTestVariant(UCollator* myCollation, const UChar source[], const UChar target[], UCollationResult result)
{
    int32_t sortklen1, sortklen2, sortklenmax, sortklenmin;
    int temp=0, gSortklen1=0,gSortklen2=0;
    UCollationResult compareResult, compareResulta, keyResult, compareResultIter = result;
    uint8_t *sortKey1, *sortKey2, *sortKey1a, *sortKey2a;
    uint32_t sLen = u_strlen(source);
    uint32_t tLen = u_strlen(target);
    char buffer[256];
    uint32_t len;
    UErrorCode status = U_ZERO_ERROR;
    UColAttributeValue norm = ucol_getAttribute(myCollation, UCOL_NORMALIZATION_MODE, &status);

    UCharIterator sIter, tIter;
    uiter_setString(&sIter, source, sLen);
    uiter_setString(&tIter, target, tLen);
    compareResultIter = ucol_strcollIter(myCollation, &sIter, &tIter, &status);
    if(compareResultIter != result) {
        log_err("different results in iterative comparison for UTF-16 encoded strings. %s, %s\n", aescstrdup(source,-1), aescstrdup(target,-1));
    }

    /* convert the strings to UTF-8 and do try comparing with char iterator */
    if(getTestOption(QUICK_OPTION) <= 0) { /*!QUICK*/
      char utf8Source[256], utf8Target[256];
      int32_t utf8SourceLen = 0, utf8TargetLen = 0;
      u_strToUTF8(utf8Source, 256, &utf8SourceLen, source, sLen, &status);
      if(U_FAILURE(status)) { /* probably buffer is not big enough */
        log_verbose("Src UTF-8 buffer too small! Will not compare!\n");
      } else {
        u_strToUTF8(utf8Target, 256, &utf8TargetLen, target, tLen, &status);
        if(U_SUCCESS(status)) { /* probably buffer is not big enough */
          UCollationResult compareResultUTF8 = result, compareResultUTF8Norm = result;
          /*UCharIterator sIter, tIter;*/
          /*log_verbose("Strings converted to UTF-8:%s, %s\n", aescstrdup(source,-1), aescstrdup(target,-1));*/
          uiter_setUTF8(&sIter, utf8Source, utf8SourceLen);
          uiter_setUTF8(&tIter, utf8Target, utf8TargetLen);
       /*uiter_setString(&sIter, source, sLen);
      uiter_setString(&tIter, target, tLen);*/
          compareResultUTF8 = ucol_strcollIter(myCollation, &sIter, &tIter, &status);
          ucol_setAttribute(myCollation, UCOL_NORMALIZATION_MODE, UCOL_ON, &status);
          sIter.move(&sIter, 0, UITER_START);
          tIter.move(&tIter, 0, UITER_START);
          compareResultUTF8Norm = ucol_strcollIter(myCollation, &sIter, &tIter, &status);
          ucol_setAttribute(myCollation, UCOL_NORMALIZATION_MODE, norm, &status);
          if(compareResultUTF8 != compareResultIter) {
            log_err("different results in iterative comparison for UTF-16 and UTF-8 encoded strings. %s, %s\n", aescstrdup(source,-1), aescstrdup(target,-1));
          }
          if(compareResultUTF8 != compareResultUTF8Norm) {
            log_err("different results in iterative when normalization is turned on with UTF-8 strings. %s, %s\n", aescstrdup(source,-1), aescstrdup(target,-1));
          }
        } else {
          log_verbose("Target UTF-8 buffer too small! Did not compare!\n");
        }
        if(U_FAILURE(status)) {
          log_verbose("UTF-8 strcoll failed! Ignoring result\n");
        }
      }
    }

    /* testing the partial sortkeys */
    if(1) { /*!QUICK*/
      int32_t i = 0;
      int32_t partialSizes[] = { 3, 1, 2, 4, 8, 20, 80 }; /* just size 3 in the quick mode */
      int32_t partialSizesSize = 1;
      if(getTestOption(QUICK_OPTION) <= 0) {
        partialSizesSize = 7;
      }
      /*log_verbose("partial sortkey test piecesize=");*/
      for(i = 0; i < partialSizesSize; i++) {
        UCollationResult partialSKResult = result, partialNormalizedSKResult = result;
        /*log_verbose("%i ", partialSizes[i]);*/

        partialSKResult = compareUsingPartials(myCollation, source, sLen, target, tLen, partialSizes[i], &status);
        if(partialSKResult != result) {
          log_err("Partial sortkey comparison returned wrong result (%i exp. %i): %s, %s (size %i)\n", 
            partialSKResult, result,
            aescstrdup(source,-1), aescstrdup(target,-1), partialSizes[i]);
        }

        if(getTestOption(QUICK_OPTION) <= 0 && norm != UCOL_ON) {
          /*log_verbose("N ");*/
          ucol_setAttribute(myCollation, UCOL_NORMALIZATION_MODE, UCOL_ON, &status);
          partialNormalizedSKResult = compareUsingPartials(myCollation, source, sLen, target, tLen, partialSizes[i], &status);
          ucol_setAttribute(myCollation, UCOL_NORMALIZATION_MODE, norm, &status);
          if(partialSKResult != partialNormalizedSKResult) {
            log_err("Partial sortkey comparison gets different result when normalization is on: %s, %s (size %i)\n", 
              aescstrdup(source,-1), aescstrdup(target,-1), partialSizes[i]);
          }
        }
      }
      /*log_verbose("\n");*/
    }

    
    compareResult  = ucol_strcoll(myCollation, source, sLen, target, tLen);
    compareResulta = ucol_strcoll(myCollation, source, -1,   target, -1); 
    if (compareResult != compareResulta) {
        log_err("ucol_strcoll result from null terminated and explicit length strings differs.\n");
    }

    sortklen1=ucol_getSortKey(myCollation, source, sLen,  NULL, 0);
    sortklen2=ucol_getSortKey(myCollation, target, tLen,  NULL, 0);

    sortklenmax = (sortklen1>sortklen2?sortklen1:sortklen2);
    sortklenmin = (sortklen1<sortklen2?sortklen1:sortklen2);

    sortKey1 =(uint8_t*)malloc(sizeof(uint8_t) * (sortklenmax+1));
    sortKey1a=(uint8_t*)malloc(sizeof(uint8_t) * (sortklenmax+1));
    ucol_getSortKey(myCollation, source, sLen, sortKey1,  sortklen1+1);
    ucol_getSortKey(myCollation, source, -1,   sortKey1a, sortklen1+1);
    
    sortKey2 =(uint8_t*)malloc(sizeof(uint8_t) * (sortklenmax+1));
    sortKey2a=(uint8_t*)malloc(sizeof(uint8_t) * (sortklenmax+1));
    ucol_getSortKey(myCollation, target, tLen, sortKey2,  sortklen2+1);
    ucol_getSortKey(myCollation, target, -1,   sortKey2a, sortklen2+1);

    /* Check that sort key generated with null terminated string is identical  */
    /*  to that generted with a length specified.                              */
    if (uprv_strcmp((const char *)sortKey1, (const char *)sortKey1a) != 0 ||
        uprv_strcmp((const char *)sortKey2, (const char *)sortKey2a) != 0 ) {
        log_err("Sort Keys from null terminated and explicit length strings differ.\n");
    }

    /*memcmp(sortKey1, sortKey2,sortklenmax);*/
    temp= uprv_strcmp((const char *)sortKey1, (const char *)sortKey2);
    gSortklen1 = uprv_strlen((const char *)sortKey1)+1;
    gSortklen2 = uprv_strlen((const char *)sortKey2)+1;
    if(sortklen1 != gSortklen1){
        log_err("SortKey length does not match Expected: %i Got: %i\n",sortklen1, gSortklen1);
        log_verbose("Generated sortkey: %s\n", sortKeyToString(myCollation, sortKey1, buffer, &len));
    }
    if(sortklen2!= gSortklen2){
        log_err("SortKey length does not match Expected: %i Got: %i\n", sortklen2, gSortklen2);
        log_verbose("Generated sortkey: %s\n", sortKeyToString(myCollation, sortKey2, buffer, &len));
    }

    if(temp < 0) {
        keyResult=UCOL_LESS;
    }
    else if(temp > 0) {
        keyResult= UCOL_GREATER;
    }
    else {
        keyResult = UCOL_EQUAL;
    }
    reportCResult( source, target, sortKey1, sortKey2, compareResult, keyResult, compareResultIter, result );
    free(sortKey1);
    free(sortKey2);
    free(sortKey1a);
    free(sortKey2a);

}

void doTest(UCollator* myCollation, const UChar source[], const UChar target[], UCollationResult result)
{
  if(myCollation) {
    doTestVariant(myCollation, source, target, result);
    if(result == UCOL_LESS) {
      doTestVariant(myCollation, target, source, UCOL_GREATER);
    } else if(result == UCOL_GREATER) {
      doTestVariant(myCollation, target, source, UCOL_LESS);
    } else {
      doTestVariant(myCollation, target, source, UCOL_EQUAL);
    }
  } else {
    log_data_err("No collator! Any data around?\n");
  }
}


/**
 * Return an integer array containing all of the collation orders
 * returned by calls to next on the specified iterator
 */
OrderAndOffset* getOrders(UCollationElements *iter, int32_t *orderLength)
{
    UErrorCode status;
    int32_t order;
    int32_t maxSize = 100;
    int32_t size = 0;
    int32_t offset = ucol_getOffset(iter);
    OrderAndOffset *temp;
    OrderAndOffset *orders =(OrderAndOffset *)malloc(sizeof(OrderAndOffset) * maxSize);
    status= U_ZERO_ERROR;


    while ((order=ucol_next(iter, &status)) != UCOL_NULLORDER)
    {
        if (size == maxSize)
        {
            maxSize *= 2;
            temp = (OrderAndOffset *)malloc(sizeof(OrderAndOffset) * maxSize);

            memcpy(temp, orders, size * sizeof(OrderAndOffset));
            free(orders);
            orders = temp;

        }

        orders[size].order  = order;
        orders[size].offset = offset;

        offset = ucol_getOffset(iter);
        size += 1;
    }

    if (maxSize > size && size > 0)
    {
        temp = (OrderAndOffset *)malloc(sizeof(OrderAndOffset) * size);

        memcpy(temp, orders, size * sizeof(OrderAndOffset));
        free(orders);
        orders = temp;


    }

    *orderLength = size;
    return orders;
}


void 
backAndForth(UCollationElements *iter)
{
    /* Run through the iterator forwards and stick it into an array */
    int32_t index, o;
    UErrorCode status = U_ZERO_ERROR;
    int32_t orderLength = 0;
    OrderAndOffset *orders = getOrders(iter, &orderLength);


    /* Now go through it backwards and make sure we get the same values */
    index = orderLength;
    ucol_reset(iter);

    /* synwee : changed */
    while ((o = ucol_previous(iter, &status)) != UCOL_NULLORDER) {
#if TEST_OFFSETS
      int32_t offset = 
#endif
        ucol_getOffset(iter);

      index -= 1;
      if (o != orders[index].order) {
        if (o == 0)
          index ++;
        else {
          while (index > 0 && orders[-- index].order == 0) {
            /* nothing... */
          }

          if (o != orders[index].order) {
              log_err("Mismatched order at index %d: 0x%8.8X vs. 0x%8.8X\n", index,
                orders[index].order, o);
            goto bail;
          }
        }
      }

#if TEST_OFFSETS
      if (offset != orders[index].offset) {
        log_err("Mismatched offset at index %d: %d vs. %d\n", index,
            orders[index].offset, offset);
        goto bail;
      }
#endif

    }

    while (index != 0 && orders[index - 1].order == 0) {
      index -= 1;
    }

    if (index != 0) {
        log_err("Didn't get back to beginning - index is %d\n", index);

        ucol_reset(iter);
        log_err("\nnext: ");

        if ((o = ucol_next(iter, &status)) != UCOL_NULLORDER) {
            log_err("Error at %x\n", o);
        }

        log_err("\nprev: ");

        if ((o = ucol_previous(iter, &status)) != UCOL_NULLORDER) {
            log_err("Error at %x\n", o);
        }

        log_verbose("\n");
    }

bail:
    free(orders);
}

void genericOrderingTestWithResult(UCollator *coll, const char * const s[], uint32_t size, UCollationResult result) {
  UChar t1[2048] = {0};
  UChar t2[2048] = {0};
  UCollationElements *iter;
  UErrorCode status = U_ZERO_ERROR;

  uint32_t i = 0, j = 0;
  log_verbose("testing sequence:\n");
  for(i = 0; i < size; i++) {
    log_verbose("%s\n", s[i]);
  }

  iter = ucol_openElements(coll, t1, u_strlen(t1), &status);
  if (U_FAILURE(status)) {
    log_err("Creation of iterator failed\n");
  }
  for(i = 0; i < size-1; i++) {
    for(j = i+1; j < size; j++) {
      u_unescape(s[i], t1, 2048);
      u_unescape(s[j], t2, 2048);
      doTest(coll, t1, t2, result);
      /* synwee : added collation element iterator test */
      ucol_setText(iter, t1, u_strlen(t1), &status);
      backAndForth(iter);
      ucol_setText(iter, t2, u_strlen(t2), &status);
      backAndForth(iter);
    }
  }
  ucol_closeElements(iter);
}

void genericOrderingTest(UCollator *coll, const char * const s[], uint32_t size) {
  genericOrderingTestWithResult(coll, s, size, UCOL_LESS);
}

void genericLocaleStarter(const char *locale, const char * const s[], uint32_t size) {
  UErrorCode status = U_ZERO_ERROR;
  UCollator *coll = ucol_open(locale, &status);

  log_verbose("Locale starter for %s\n", locale);

  if(U_SUCCESS(status)) {
    genericOrderingTest(coll, s, size);
  } else if(status == U_FILE_ACCESS_ERROR) {
    log_data_err("Is your data around?\n");
    return;
  } else {
    log_err("Unable to open collator for locale %s\n", locale);
  }
  ucol_close(coll);
}

void genericLocaleStarterWithResult(const char *locale, const char * const s[], uint32_t size, UCollationResult result) {
  UErrorCode status = U_ZERO_ERROR;
  UCollator *coll = ucol_open(locale, &status);

  log_verbose("Locale starter for %s\n", locale);

  if(U_SUCCESS(status)) {
    genericOrderingTestWithResult(coll, s, size, result);
  } else if(status == U_FILE_ACCESS_ERROR) {
    log_data_err("Is your data around?\n");
    return;
  } else {
    log_err("Unable to open collator for locale %s\n", locale);
  }
  ucol_close(coll);
}

/* currently not used with options */
void genericRulesStarterWithOptionsAndResult(const char *rules, const char * const s[], uint32_t size, const UColAttribute *attrs, const UColAttributeValue *values, uint32_t attsize, UCollationResult result) {
  UErrorCode status = U_ZERO_ERROR;
  UChar rlz[RULE_BUFFER_LEN] = { 0 };
  uint32_t rlen = u_unescape(rules, rlz, RULE_BUFFER_LEN);
  uint32_t i;

  UCollator *coll = ucol_openRules(rlz, rlen, UCOL_DEFAULT, UCOL_DEFAULT,NULL, &status);

  log_verbose("Rules starter for %s\n", rules);

  if(U_SUCCESS(status)) {
    log_verbose("Setting attributes\n");
    for(i = 0; i < attsize; i++) {
      ucol_setAttribute(coll, attrs[i], values[i], &status);
    }

    genericOrderingTestWithResult(coll, s, size, result);
  } else {
    log_err_status(status, "Unable to open collator with rules %s\n", rules);
  }
  ucol_close(coll);
}

void genericLocaleStarterWithOptionsAndResult(const char *locale, const char * const s[], uint32_t size, const UColAttribute *attrs, const UColAttributeValue *values, uint32_t attsize, UCollationResult result) {
  UErrorCode status = U_ZERO_ERROR;
  uint32_t i;

  UCollator *coll = ucol_open(locale, &status);

  log_verbose("Locale starter for %s\n", locale);

  if(U_SUCCESS(status)) {

    log_verbose("Setting attributes\n");
    for(i = 0; i < attsize; i++) {
      ucol_setAttribute(coll, attrs[i], values[i], &status);
    }

    genericOrderingTestWithResult(coll, s, size, result);
  } else {
    log_err_status(status, "Unable to open collator for locale %s\n", locale);
  }
  ucol_close(coll);
}

void genericLocaleStarterWithOptions(const char *locale, const char * const s[], uint32_t size, const UColAttribute *attrs, const UColAttributeValue *values, uint32_t attsize) {
  genericLocaleStarterWithOptionsAndResult(locale, s, size, attrs, values, attsize, UCOL_LESS);
}

void genericRulesStarterWithResult(const char *rules, const char * const s[], uint32_t size, UCollationResult result) {
  UErrorCode status = U_ZERO_ERROR;
  UChar rlz[RULE_BUFFER_LEN] = { 0 };
  uint32_t rlen = u_unescape(rules, rlz, RULE_BUFFER_LEN);

  UCollator *coll = NULL;
  coll = ucol_openRules(rlz, rlen, UCOL_DEFAULT, UCOL_DEFAULT,NULL, &status);
  log_verbose("Rules starter for %s\n", rules);

  if(U_SUCCESS(status)) {
    genericOrderingTestWithResult(coll, s, size, result);
    ucol_close(coll);
  } else if(status == U_FILE_ACCESS_ERROR) {
    log_data_err("Is your data around?\n");
  } else {
    log_err("Unable to open collator with rules %s\n", rules);
  }
}

void genericRulesStarter(const char *rules, const char * const s[], uint32_t size) {
  genericRulesStarterWithResult(rules, s, size, UCOL_LESS);
}

static void TestTertiary()
{
    int32_t len,i;
    UCollator *myCollation;
    UErrorCode status=U_ZERO_ERROR;
    static const char str[]="& C < ch, cH, Ch, CH & Five, 5 & Four, 4 & one, 1 & Ampersand; '&' & Two, 2 ";
    UChar rules[sizeof(str)];
    len = strlen(str);
    u_uastrcpy(rules, str);

    myCollation=ucol_openRules(rules, len, UCOL_OFF, UCOL_DEFAULT_STRENGTH, NULL, &status);
    if(U_FAILURE(status)){
        log_err_status(status, "ERROR: in creation of rule based collator :%s\n", myErrorName(status));
        return;
    }
   
    ucol_setStrength(myCollation, UCOL_TERTIARY);
    for (i = 0; i < 17 ; i++)
    {
        doTest(myCollation, testSourceCases[i], testTargetCases[i], results[i]);
    }
    ucol_close(myCollation);
    myCollation = 0;
}

static void TestPrimary( )
{
    int32_t len,i;
    UCollator *myCollation;
    UErrorCode status=U_ZERO_ERROR;
    static const char str[]="& C < ch, cH, Ch, CH & Five, 5 & Four, 4 & one, 1 & Ampersand; '&' & Two, 2 ";   
    UChar rules[sizeof(str)];
    len = strlen(str);
    u_uastrcpy(rules, str);

    myCollation=ucol_openRules(rules, len, UCOL_OFF, UCOL_DEFAULT_STRENGTH,NULL, &status);
    if(U_FAILURE(status)){
        log_err_status(status, "ERROR: in creation of rule based collator :%s\n", myErrorName(status));
        return;
    }
    ucol_setStrength(myCollation, UCOL_PRIMARY);
    
    for (i = 17; i < 26 ; i++)
    {
        
        doTest(myCollation, testSourceCases[i], testTargetCases[i], results[i]);
    }
    ucol_close(myCollation);
    myCollation = 0;
}

static void TestSecondary()
{
    int32_t i;
    int32_t len;
    UCollator *myCollation;
    UErrorCode status=U_ZERO_ERROR;
    static const char str[]="& C < ch, cH, Ch, CH & Five, 5 & Four, 4 & one, 1 & Ampersand; '&' & Two, 2 ";
    UChar rules[sizeof(str)];
    len = strlen(str);
    u_uastrcpy(rules, str);

    myCollation=ucol_openRules(rules, len, UCOL_OFF, UCOL_DEFAULT_STRENGTH,NULL, &status);
    if(U_FAILURE(status)){
        log_err_status(status, "ERROR: in creation of rule based collator :%s\n", myErrorName(status));
        return;
    }
    ucol_setStrength(myCollation, UCOL_SECONDARY);
    for (i = 26; i < 34 ; i++)
    {
        doTest(myCollation, testSourceCases[i], testTargetCases[i], results[i]);
    }
    ucol_close(myCollation);
    myCollation = 0;
}

static void TestIdentical()
{
    int32_t i;
    int32_t len;
    UCollator *myCollation;
    UErrorCode status=U_ZERO_ERROR;
    static const char str[]="& C < ch, cH, Ch, CH & Five, 5 & Four, 4 & one, 1 & Ampersand; '&' & Two, 2 ";
    UChar rules[sizeof(str)];
    len = strlen(str);
    u_uastrcpy(rules, str);

    myCollation=ucol_openRules(rules, len, UCOL_OFF, UCOL_IDENTICAL, NULL,&status);
    if(U_FAILURE(status)){
        log_err_status(status, "ERROR: in creation of rule based collator :%s\n", myErrorName(status));
        return;
    }
    for(i= 34; i<37; i++)
    {
        doTest(myCollation, testSourceCases[i], testTargetCases[i], results[i]);
    }
    ucol_close(myCollation);
    myCollation = 0;
}

static void TestExtra()
{
    int32_t i, j;
    int32_t len;
    UCollator *myCollation;
    UErrorCode status = U_ZERO_ERROR;
    static const char str[]="& C < ch, cH, Ch, CH & Five, 5 & Four, 4 & one, 1 & Ampersand; '&' & Two, 2 ";
    UChar rules[sizeof(str)];
    len = strlen(str);
    u_uastrcpy(rules, str);

    myCollation=ucol_openRules(rules, len, UCOL_OFF, UCOL_DEFAULT_STRENGTH,NULL, &status);
    if(U_FAILURE(status)){
        log_err_status(status, "ERROR: in creation of rule based collator :%s\n", myErrorName(status));
        return;
    }
    ucol_setStrength(myCollation, UCOL_TERTIARY);
    for (i = 0; i < COUNT_TEST_CASES-1 ; i++)
    {
        for (j = i + 1; j < COUNT_TEST_CASES; j += 1)
        {
        
            doTest(myCollation, testCases[i], testCases[j], UCOL_LESS);
        }
    }
    ucol_close(myCollation);
    myCollation = 0;
}

static void TestJB581(void)
{
    int32_t     bufferLen   = 0;
    UChar       source      [100];
    UChar       target      [100];
    UCollationResult result     = UCOL_EQUAL;
    uint8_t     sourceKeyArray  [100];
    uint8_t     targetKeyArray  [100]; 
    int32_t     sourceKeyOut    = 0, 
                targetKeyOut    = 0;
    UCollator   *myCollator = 0;
    UErrorCode status = U_ZERO_ERROR;

    /*u_uastrcpy(source, "This is a test.");*/
    /*u_uastrcpy(target, "THISISATEST.");*/
    u_uastrcpy(source, "THISISATEST.");
    u_uastrcpy(target, "Thisisatest.");

    myCollator = ucol_open("en_US", &status);
    if (U_FAILURE(status)){
        log_err_status(status, "ERROR: Failed to create the collator : %s\n", u_errorName(status));
        return;
    }
    result = ucol_strcoll(myCollator, source, -1, target, -1);
    /* result is 1, secondary differences only for ignorable space characters*/
    if (result != 1)
    {
        log_err("Comparing two strings with only secondary differences in C failed.\n");
    }
    /* To compare them with just primary differences */
    ucol_setStrength(myCollator, UCOL_PRIMARY);
    result = ucol_strcoll(myCollator, source, -1, target, -1);
    /* result is 0 */
    if (result != 0)
    {
        log_err("Comparing two strings with no differences in C failed.\n");
    }
    /* Now, do the same comparison with keys */
    sourceKeyOut = ucol_getSortKey(myCollator, source, -1, sourceKeyArray, 100);
    targetKeyOut = ucol_getSortKey(myCollator, target, -1, targetKeyArray, 100);
    bufferLen = ((targetKeyOut > 100) ? 100 : targetKeyOut);
    if (memcmp(sourceKeyArray, targetKeyArray, bufferLen) != 0)
    {
        log_err("Comparing two strings with sort keys in C failed.\n");
    }
    ucol_close(myCollator);
}

static void TestJB1401(void)
{
    UCollator     *myCollator = 0;
    UErrorCode     status = U_ZERO_ERROR;
    static UChar   NFD_UnsafeStartChars[] = {
        0x0f73,          /* Tibetan Vowel Sign II */
        0x0f75,          /* Tibetan Vowel Sign UU */
        0x0f81,          /* Tibetan Vowel Sign Reversed II */
            0
    };
    int            i;

    
    myCollator = ucol_open("en_US", &status);
    if (U_FAILURE(status)){
        log_err_status(status, "ERROR: Failed to create the collator : %s\n", u_errorName(status));
        return;
    }
    ucol_setAttribute(myCollator, UCOL_NORMALIZATION_MODE, UCOL_ON, &status);
    if (U_FAILURE(status)){
        log_err("ERROR: Failed to set normalization mode ON for collator.\n");
        return;
    }

    for (i=0; ; i++) {
        UChar    c;
        UChar    X[4];
        UChar    Y[20];
        UChar    Z[20];

        /*  Get the next funny character to be tested, and set up the
         *  three test strings X, Y, Z, consisting of an A-grave + test char,
         *    in original form, NFD, and then NFC form.
         */
        c = NFD_UnsafeStartChars[i];
        if (c==0) {break;}

        X[0]=0xC0; X[1]=c; X[2]=0;   /* \u00C0 is A Grave*/
        
        unorm_normalize(X, -1, UNORM_NFD, 0, Y, 20, &status);
        unorm_normalize(Y, -1, UNORM_NFC, 0, Z, 20, &status);
        if (U_FAILURE(status)){
            log_err("ERROR: Failed to normalize test of character %x\n", c);
            return;
        }

        /* Collation test.  All three strings should be equal.
         *   doTest does both strcoll and sort keys, with params in both orders.
         */
        doTest(myCollator, X, Y, UCOL_EQUAL);
        doTest(myCollator, X, Z, UCOL_EQUAL);
        doTest(myCollator, Y, Z, UCOL_EQUAL);

        /* Run collation element iterators over the three strings.  Results should be same for each.
         */
        {
            UCollationElements *ceiX, *ceiY, *ceiZ;
            int32_t             ceX,   ceY,   ceZ;
            int                 j;

            ceiX = ucol_openElements(myCollator, X, -1, &status);
            ceiY = ucol_openElements(myCollator, Y, -1, &status);
            ceiZ = ucol_openElements(myCollator, Z, -1, &status);
            if (U_FAILURE(status)) {
                log_err("ERROR: uucol_openElements failed.\n");
                return;
            }

            for (j=0;; j++) {
                ceX = ucol_next(ceiX, &status);
                ceY = ucol_next(ceiY, &status);
                ceZ = ucol_next(ceiZ, &status);
                if (U_FAILURE(status)) {
                    log_err("ERROR: ucol_next failed for iteration #%d.\n", j);
                    break;
                }
                if (ceX != ceY || ceY != ceZ) {
                    log_err("ERROR: ucol_next failed for iteration #%d.\n", j);
                    break;
                }
                if (ceX == UCOL_NULLORDER) {
                    break;
                }
            }
            ucol_closeElements(ceiX);
            ucol_closeElements(ceiY);
            ucol_closeElements(ceiZ);
        }
    }
    ucol_close(myCollator);
}



/**
* Tests the [variable top] tag in rule syntax. Since the default [alternate]
* tag has the value shifted, any codepoints before [variable top] should give
* a primary ce of 0.
*/
static void TestVariableTop(void)
{
    static const char       str[]          = "&z = [variable top]";
          int         len          = strlen(str);
          UChar      rules[sizeof(str)];
          UCollator  *myCollation;
          UCollator  *enCollation;
          UErrorCode  status       = U_ZERO_ERROR;
          UChar       source[1];
          UChar       ch;
          uint8_t     result[20];
          uint8_t     expected[20];

    u_uastrcpy(rules, str);

    enCollation = ucol_open("en_US", &status);
    if (U_FAILURE(status)) {
        log_err_status(status, "ERROR: in creation of collator :%s\n", 
                myErrorName(status));
        return;
    }
    myCollation = ucol_openRules(rules, len, UCOL_OFF, 
                                 UCOL_PRIMARY,NULL, &status);
    if (U_FAILURE(status)) {
        ucol_close(enCollation);
        log_err("ERROR: in creation of rule based collator :%s\n", 
                myErrorName(status));
        return;
    }

    ucol_setStrength(enCollation, UCOL_PRIMARY);
    ucol_setAttribute(enCollation, UCOL_ALTERNATE_HANDLING, UCOL_SHIFTED,
                      &status);
    ucol_setAttribute(myCollation, UCOL_ALTERNATE_HANDLING, UCOL_SHIFTED,
                      &status);
        
    if (ucol_getAttribute(myCollation, UCOL_ALTERNATE_HANDLING, &status) !=
        UCOL_SHIFTED || U_FAILURE(status)) {
        log_err("ERROR: ALTERNATE_HANDLING value can not be set to SHIFTED\n");
    }

    uprv_memset(expected, 0, 20);

    /* space is supposed to be a variable */
    source[0] = ' ';
    len = ucol_getSortKey(enCollation, source, 1, result, 
                          sizeof(result));

    if (uprv_memcmp(expected, result, len) != 0) {
        log_err("ERROR: SHIFTED alternate does not return 0 for primary of space\n");
    }

    ch = 'a';
    while (ch < 'z') {
        source[0] = ch;
        len = ucol_getSortKey(myCollation, source, 1, result,
                              sizeof(result));
        if (uprv_memcmp(expected, result, len) != 0) {
            log_err("ERROR: SHIFTED alternate does not return 0 for primary of %c\n", 
                    ch);
        }
        ch ++;
    }
  
    ucol_close(enCollation);
    ucol_close(myCollation);
    enCollation = NULL;
    myCollation = NULL;
}

/**
  * Tests surrogate support.
  * NOTE: This test used \\uD801\\uDC01 pair, which is now assigned to Desseret
  * Therefore, another (unassigned) code point was used for this test.
  */
static void TestSurrogates(void)
{
    static const char       str[]          = 
                              "&z<'\\uD800\\uDC00'<'\\uD800\\uDC0A\\u0308'<A";
          int         len          = strlen(str);
          int         rlen         = 0;
          UChar      rules[sizeof(str)];
          UCollator  *myCollation;
          UCollator  *enCollation;
          UErrorCode  status       = U_ZERO_ERROR;
          UChar       source[][4]    = 
          {{'z', 0, 0}, {0xD800, 0xDC00, 0}, {0xD800, 0xDC0A, 0x0308, 0}, {0xD800, 0xDC02}};
          UChar       target[][4]    = 
          {{0xD800, 0xDC00, 0}, {0xD800, 0xDC0A, 0x0308, 0}, {'A', 0, 0}, {0xD800, 0xDC03}};
          int         count        = 0;
          uint8_t enresult[20], myresult[20];
          int enlen, mylen;
          
    /* tests for open rules with surrogate rules */
    rlen = u_unescape(str, rules, len);
    
    enCollation = ucol_open("en_US", &status);
    if (U_FAILURE(status)) {
        log_err_status(status, "ERROR: in creation of collator :%s\n", 
                myErrorName(status));
        return;
    }
    myCollation = ucol_openRules(rules, rlen, UCOL_OFF, 
                                 UCOL_TERTIARY,NULL, &status);
    if (U_FAILURE(status)) {
        ucol_close(enCollation);
        log_err("ERROR: in creation of rule based collator :%s\n", 
                myErrorName(status));
        return;
    }

    /* 
    this test is to verify the supplementary sort key order in the english 
    collator
    */
    log_verbose("start of english collation supplementary characters test\n");
    while (count < 2) {
        doTest(enCollation, source[count], target[count], UCOL_LESS);
        count ++;
    }
    doTest(enCollation, source[count], target[count], UCOL_GREATER);
        
    log_verbose("start of tailored collation supplementary characters test\n");
    count = 0;
    /* tests getting collation elements for surrogates for tailored rules */
    while (count < 4) {
        doTest(myCollation, source[count], target[count], UCOL_LESS);
        count ++;
    }

    /* tests that \uD800\uDC02 still has the same value, not changed */
    enlen = ucol_getSortKey(enCollation, source[3], 2, enresult, 20);
    mylen = ucol_getSortKey(myCollation, source[3], 2, myresult, 20);
    if (enlen != mylen ||
        uprv_memcmp(enresult, myresult, enlen) != 0) {
        log_verbose("Failed : non-tailored supplementary characters should have the same value\n");
    }

    ucol_close(enCollation);
    ucol_close(myCollation);
    enCollation = NULL;
    myCollation = NULL;
}

/*
 *### TODO: Add more invalid rules to test all different scenarios.
 *
 */
static void 
TestInvalidRules(){
#define MAX_ERROR_STATES 2

    static const char* rulesArr[MAX_ERROR_STATES] = {
        "& C < ch, cH, Ch[this should fail]<d",
        "& C < ch, cH, & Ch[variable top]"
    };
    static const char* preContextArr[MAX_ERROR_STATES] = {
        "his should fail",
        "& C < ch, cH, ",

    };
    static const char* postContextArr[MAX_ERROR_STATES] = {
        "<d",
        " Ch[variable t"
    };
    int i;

    for(i = 0;i<MAX_ERROR_STATES;i++){
        UChar rules[1000]       = { '\0' };
        UChar preContextExp[1000]  = { '\0' };
        UChar postContextExp[1000] = { '\0' };
        UParseError parseError;
        UErrorCode status = U_ZERO_ERROR;
        UCollator* coll=0;
        u_charsToUChars(rulesArr[i],rules,uprv_strlen(rulesArr[i])+1);
        u_charsToUChars(preContextArr[i],preContextExp,uprv_strlen(preContextArr[i])+1);
        u_charsToUChars(postContextArr[i],postContextExp,uprv_strlen(postContextArr[i])+1);
        /* clean up stuff in parseError */
        u_memset(parseError.preContext,0x0000,U_PARSE_CONTEXT_LEN);      
        u_memset(parseError.postContext,0x0000,U_PARSE_CONTEXT_LEN);
        /* open the rules and test */
        coll = ucol_openRules(rules,u_strlen(rules),UCOL_OFF,UCOL_DEFAULT_STRENGTH,&parseError,&status);
        if(u_strcmp(parseError.preContext,preContextExp)!=0){
            log_err_status(status, "preContext in UParseError for ucol_openRules does not match\n");
        }
        if(u_strcmp(parseError.postContext,postContextExp)!=0){
            log_err_status(status, "postContext in UParseError for ucol_openRules does not match\n");
        }
    }  
}

static void
TestJitterbug1098(){
    UChar rule[1000];
    UCollator* c1 = NULL;
    UErrorCode status = U_ZERO_ERROR;
    UParseError parseError;
    char preContext[200]={0};
    char postContext[200]={0};
    int i=0;
    const char* rules[] = {
         "&''<\\\\",
         "&\\'<\\\\",
         "&\\\"<'\\'",
         "&'\"'<\\'",
         '\0'

    };
    const UCollationResult results1098[] = {
        UCOL_LESS,
        UCOL_LESS, 
        UCOL_LESS,
        UCOL_LESS,
    };
    const UChar input[][2]= {
        {0x0027,0x005c},
        {0x0027,0x005c},
        {0x0022,0x005c},
        {0x0022,0x0027},
    };
    UChar X[2] ={0};
    UChar Y[2] ={0};
    u_memset(parseError.preContext,0x0000,U_PARSE_CONTEXT_LEN);      
    u_memset(parseError.postContext,0x0000,U_PARSE_CONTEXT_LEN);
    for(;rules[i]!=0;i++){
        u_uastrcpy(rule, rules[i]);
        c1 = ucol_openRules(rule, u_strlen(rule), UCOL_OFF, UCOL_DEFAULT_STRENGTH, &parseError, &status);
        if(U_FAILURE(status)){
            log_err_status(status, "Could not parse the rules syntax. Error: %s\n", u_errorName(status));

            if (status == U_PARSE_ERROR) {
                u_UCharsToChars(parseError.preContext,preContext,20);
                u_UCharsToChars(parseError.postContext,postContext,20);
                log_verbose("\n\tPre-Context: %s \n\tPost-Context:%s \n",preContext,postContext);
            }

            return;
        }
        X[0] = input[i][0];
        Y[0] = input[i][1];
        doTest(c1,X,Y,results1098[i]);
        ucol_close(c1);
    }
}

static void
TestFCDCrash(void) {
    static const char *test[] = {
    "Gr\\u00F6\\u00DFe",
    "Grossist"
    };

    UErrorCode status = U_ZERO_ERROR;
    UCollator *coll = ucol_open("es", &status);
    if(U_FAILURE(status)) {
        log_err_status(status, "Couldn't open collator -> %s\n", u_errorName(status));
        return;
    }
    ucol_close(coll);
    coll = NULL;
    ctest_resetICU();
    coll = ucol_open("de_DE", &status);
    if(U_FAILURE(status)) {
        log_err_status(status, "Couldn't open collator -> %s\n", u_errorName(status));
        return;
    }
    ucol_setAttribute(coll, UCOL_NORMALIZATION_MODE, UCOL_ON, &status);
    genericOrderingTest(coll, test, 2);
    ucol_close(coll);
}

/*static UBool
find(UEnumeration* list, const char* str, UErrorCode* status){
    const char* value = NULL;
    int32_t length=0;
    if(U_FAILURE(*status)){
        return FALSE;
    }
    uenum_reset(list, status);
    while( (value= uenum_next(list, &length, status))!=NULL){
        if(strcmp(value, str)==0){
            return TRUE;
        }
    }
    return FALSE;
}*/

static void TestJ5298(void)
{
    UErrorCode status = U_ZERO_ERROR;
    char input[256], output[256];
    UBool isAvailable;
    int32_t i = 0;
    UEnumeration* values = NULL;
    const char *keywordValue = NULL;
    log_verbose("Number of collator locales returned : %i \n", ucol_countAvailable());
    values = ucol_getKeywordValues("collation", &status);
    for (i = 0; i < ucol_countAvailable(); i++) {
        uenum_reset(values, &status);
        while ((keywordValue = uenum_next(values, NULL, &status)) != NULL) {
            strcpy(input, ucol_getAvailable(i));
            if (strcmp(keywordValue, "standard") != 0) {
                strcat(input, "@collation=");
                strcat(input, keywordValue);
            }

            ucol_getFunctionalEquivalent(output, 256, "collation", input, &isAvailable, &status);
            if (strcmp(input, output) == 0) { /* Unique locale, print it out */
                log_verbose("%s, \n", output);
            }
        }
    }
    uenum_close(values);
    log_verbose("\n");
}
#endif /* #if !UCONFIG_NO_COLLATION */
