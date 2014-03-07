/********************************************************************
 * Copyright (c) 2001-2011 International Business Machines 
 * Corporation and others. All Rights Reserved.
 ********************************************************************
 * File usrchtst.c
 * Modification History:
 * Name           Date             Description
 * synwee         July 19 2001     creation
 ********************************************************************/

#include "unicode/utypes.h"

#if !UCONFIG_NO_COLLATION && !UCONFIG_NO_BREAK_ITERATION && !UCONFIG_NO_FILE_IO

#include "unicode/usearch.h"
#include "unicode/ustring.h"
#include "ccolltst.h"
#include "cmemory.h"
#include <stdio.h>
#include "usrchdat.c"
#include "unicode/ubrk.h"
#include <assert.h>

static UBool      TOCLOSE_ = TRUE;
static UCollator *EN_US_; 
static UCollator *FR_FR_;
static UCollator *DE_;
static UCollator *ES_;

/**
 * CHECK_BREAK(char *brk)
 *     Test if a break iterator is passed in AND break iteration is disabled. 
 *     Skip the test if so.
 * CHECK_BREAK_BOOL(char *brk)
 *     Same as above, but returns 'TRUE' as a passing result
 */

#if !UCONFIG_NO_BREAK_ITERATION
static UBreakIterator *EN_WORDBREAKER_;
static UBreakIterator *EN_CHARACTERBREAKER_;
#define CHECK_BREAK(x)
#define CHECK_BREAK_BOOL(x)
#else
#define CHECK_BREAK(x)  if(x) { log_info("Skipping test on %s:%d because UCONFIG_NO_BREAK_ITERATION is on\n", __FILE__, __LINE__); return; }
#define CHECK_BREAK_BOOL(x)  if(x) { log_info("Skipping test on %s:%d because UCONFIG_NO_BREAK_ITERATION is on\n", __FILE__, __LINE__); return TRUE; }
#endif

/**
* Opening all static collators and break iterators
*/
static void open(UErrorCode* status)
{
    if (TOCLOSE_) {
        UChar      rules[1024];
        int32_t    rulelength = 0;
        *status = U_ZERO_ERROR;

        EN_US_ = ucol_open("en_US", status);
        if(U_FAILURE(*status)) {
          log_err_status(*status, "Error opening collator\n");
          return;
        }
        FR_FR_ = ucol_open("fr_FR", status);
        DE_ = ucol_open("de_DE", status);
        ES_ = ucol_open("es_ES", status);
    
        u_strcpy(rules, ucol_getRules(DE_, &rulelength));
        u_unescape(EXTRACOLLATIONRULE, rules + rulelength, 1024 - rulelength);
    
        ucol_close(DE_);

        DE_ = ucol_openRules(rules, u_strlen(rules), UCOL_ON, UCOL_TERTIARY,
                             (UParseError *)NULL, status);
        u_strcpy(rules, ucol_getRules(ES_, &rulelength));
        u_unescape(EXTRACOLLATIONRULE, rules + rulelength, 1024 - rulelength);
    
        ucol_close(ES_);
        ES_ = ucol_openRules(rules, u_strlen(rules), UCOL_ON, UCOL_TERTIARY,
                             NULL, status); 
#if !UCONFIG_NO_BREAK_ITERATION
        EN_WORDBREAKER_     = ubrk_open(UBRK_WORD, "en_US", NULL, 0, status);
        EN_CHARACTERBREAKER_ = ubrk_open(UBRK_CHARACTER, "en_US", NULL, 0, 
                                        status);
#endif
        TOCLOSE_ = TRUE;
    }
}

/**
* Start opening all static collators and break iterators
*/
static void TestStart(void)
{
    UErrorCode status = U_ZERO_ERROR;
    open(&status);
    if (U_FAILURE(status)) {
        log_err_status(status, "Unable to open static collators %s\n", u_errorName(status));
        return;
    }
    TOCLOSE_ = FALSE;
}

/**
* Closing all static collators and break iterators
*/
static void close(void)
{
    if (TOCLOSE_) {
        ucol_close(EN_US_);
        ucol_close(FR_FR_);
        ucol_close(DE_);
        ucol_close(ES_);
#if !UCONFIG_NO_BREAK_ITERATION
        ubrk_close(EN_WORDBREAKER_);
        ubrk_close(EN_CHARACTERBREAKER_);
#endif
    }
    TOCLOSE_ = FALSE;
}

/**
* End closing all static collators and break iterators
*/
static void TestEnd(void)
{
    TOCLOSE_ = TRUE;
    close();
    TOCLOSE_ = TRUE;
}

/**
* output UChar strings for printing.
*/
static char *toCharString(const UChar* unichars)
{
    static char result[1024];
    char *temp   = result;
    int   count  = 0;
    int   length = u_strlen(unichars);

    for (; count < length; count ++) {
        UChar ch = unichars[count];
        if (ch >= 0x20 && ch <= 0x7e) {
            *temp ++ = (char)ch;
        }
        else {
            sprintf(temp, "\\u%04x", ch);
            temp += 6; /* \uxxxx */
        }
    }
    *temp = 0;

    return result;
}

/**
* Getting the collator
*/
static UCollator *getCollator(const char *collator) 
{
    if (collator == NULL) {
        return EN_US_;
    }
    if (strcmp(collator, "fr") == 0) {
        return FR_FR_;
    }
    else if (strcmp(collator, "de") == 0) {
        return DE_;
    }
    else if (strcmp(collator, "es") == 0) {
        return ES_;
    }
    else {
        return EN_US_;
    }
}

/**
* Getting the breakiterator
*/
static UBreakIterator *getBreakIterator(const char *breaker) 
{
    if (breaker == NULL) {
        return NULL;
    }
#if !UCONFIG_NO_BREAK_ITERATION
    if (strcmp(breaker, "wordbreaker") == 0) {
        return EN_WORDBREAKER_;
    }
    else {
        return EN_CHARACTERBREAKER_;
    }
#else
    return NULL;
#endif
}

static void TestOpenClose(void) 
{
          UErrorCode      status    = U_ZERO_ERROR;
          UStringSearch  *result;
    const UChar           pattern[] = {0x61, 0x62, 0x63, 0x64, 0x65, 0x66};
    const UChar           text[] = {0x61, 0x62, 0x63, 0x64, 0x65, 0x66, 0x67};
#if !UCONFIG_NO_BREAK_ITERATION
          UBreakIterator *breakiter = ubrk_open(UBRK_WORD, "en_US", 
                                                text, 6, &status);
#endif
    /* testing null arguments */
    result = usearch_open(NULL, 0, NULL, 0, NULL, NULL, &status);
    if (U_SUCCESS(status) || result != NULL) {
        log_err("Error: NULL arguments should produce an error and a NULL result\n");
    }
    status = U_ZERO_ERROR;
    result = usearch_openFromCollator(NULL, 0, NULL, 0, NULL, NULL, &status);
    if (U_SUCCESS(status) || result != NULL) {
        log_err("Error: NULL arguments should produce an error and a NULL result\n");
    }
    
    status = U_ZERO_ERROR;
    result = usearch_open(pattern, 3, NULL, 0, NULL, NULL, &status);
    if (U_SUCCESS(status) || result != NULL) {
        log_err("Error: NULL arguments should produce an error and a NULL result\n");
    }
    status = U_ZERO_ERROR;
    result = usearch_openFromCollator(pattern, 3, NULL, 0, NULL, NULL, 
                                      &status);
    if (U_SUCCESS(status) || result != NULL) {
        log_err("Error: NULL arguments should produce an error and a NULL result\n");
    }
    
    status = U_ZERO_ERROR;
    result = usearch_open(pattern, 3, text, 6, NULL, NULL, &status);
    if (U_SUCCESS(status) || result != NULL) {
        log_err("Error: NULL arguments should produce an error and a NULL result\n");
    }
    status = U_ZERO_ERROR;
    result = usearch_openFromCollator(pattern, 3, text, 6, NULL, NULL, 
                                      &status);
    if (U_SUCCESS(status) || result != NULL) {
        log_err("Error: NULL arguments should produce an error and a NULL result\n");
    }
    
    status = U_ZERO_ERROR;
    result = usearch_open(pattern, 3, text, 6, "en_US", NULL, &status);
    if (U_FAILURE(status) || result == NULL) {
        log_err_status(status, "Error: NULL break iterator is valid for opening search\n");
    }
    else {
        usearch_close(result);
    }
    open(&status);
    if (U_FAILURE(status)) {
        log_err_status(status, "Unable to open static collators %s\n", u_errorName(status));
        return;
    }
    status = U_ZERO_ERROR;
    result = usearch_openFromCollator(pattern, 3, text, 6, EN_US_, NULL, 
                                      &status);
    if (U_FAILURE(status) || result == NULL) {
        if (EN_US_ == NULL) {
            log_data_err("Opening collator failed.\n");
        } else {
            log_err("Error: NULL break iterator is valid for opening search\n");
        }
    }
    else {
        usearch_close(result);
    }


    status = U_ZERO_ERROR;
#if !UCONFIG_NO_BREAK_ITERATION

    result = usearch_open(pattern, 3, text, 6, "en_US", breakiter, &status);
    if (U_FAILURE(status) || result == NULL) {
        log_err_status(status, "Error: Break iterator is valid for opening search\n");
    }
    else {
        usearch_close(result);
    }
    status = U_ZERO_ERROR;
    result = usearch_openFromCollator(pattern, 3, text, 6, EN_US_, breakiter, 
                                      &status);
    if (U_FAILURE(status) || result == NULL) {
        if (EN_US_ == NULL) {
            log_data_err("Opening collator failed.\n");
        } else {
            log_err("Error: Break iterator is valid for opening search\n");
        }
    }
    else {
        usearch_close(result);
    }
    ubrk_close(breakiter);
#endif
    close();
}

static void TestInitialization(void) 
{
          UErrorCode      status = U_ZERO_ERROR;
          UChar           pattern[512];
    const UChar           text[] = {0x61, 0x62, 0x63, 0x64, 0x65, 0x66};
    int32_t i = 0;
    UStringSearch  *result;

    /* simple test on the pattern ce construction */
    pattern[0] = 0x41;
    pattern[1] = 0x42;
    open(&status);
    if (U_FAILURE(status)) {
        log_err_status(status, "Unable to open static collators %s\n", u_errorName(status));
        return;
    }
    result = usearch_openFromCollator(pattern, 2, text, 3, EN_US_, NULL, 
                                      &status);
    if (U_FAILURE(status)) {
        log_err("Error opening search %s\n", u_errorName(status));
    }
    usearch_close(result);

    /* testing if an extremely large pattern will fail the initialization */
    for(i = 0; i < 512; i++) {
      pattern[i] = 0x41;
    }
    /*uprv_memset(pattern, 0x41, 512);*/
    result = usearch_openFromCollator(pattern, 512, text, 3, EN_US_, NULL, 
                                      &status);
    if (U_FAILURE(status)) {
        log_err("Error opening search %s\n", u_errorName(status));
    }
    usearch_close(result);
    close();
}

static UBool assertEqualWithUStringSearch(      UStringSearch *strsrch,
                                          const SearchData     search)
{
    int         count       = 0;
    int         matchlimit  = 0;
    UErrorCode  status      = U_ZERO_ERROR;
    int32_t matchindex  = search.offset[count];
    int32_t     textlength;
    UChar       matchtext[128];

    usearch_setAttribute(strsrch, USEARCH_ELEMENT_COMPARISON, search.elemCompare, &status);
    if (U_FAILURE(status)) {
        log_err("Error setting USEARCH_ELEMENT_COMPARISON attribute %s\n", u_errorName(status));
        return FALSE;
    }   

    if (usearch_getMatchedStart(strsrch) != USEARCH_DONE ||
        usearch_getMatchedLength(strsrch) != 0) {
        log_err("Error with the initialization of match start and length\n");
    }
    /* start of following matches */
    while (U_SUCCESS(status) && matchindex >= 0) {
        uint32_t matchlength = search.size[count];
        usearch_next(strsrch, &status);
        if (matchindex != usearch_getMatchedStart(strsrch) || 
            matchlength != (uint32_t)usearch_getMatchedLength(strsrch)) {
            char *str = toCharString(usearch_getText(strsrch, &textlength));
            log_err("Text: %s\n", str);
            str = toCharString(usearch_getPattern(strsrch, &textlength));
            log_err("Pattern: %s\n", str);
            log_err("Error following match found at idx,len %d,%d; expected %d,%d\n", 
                    usearch_getMatchedStart(strsrch), usearch_getMatchedLength(strsrch),
                    matchindex, matchlength);
            return FALSE;
        }
        count ++;
        
        if (usearch_getMatchedText(strsrch, matchtext, 128, &status) !=
            (int32_t) matchlength || U_FAILURE(status) ||
            memcmp(matchtext, 
                   usearch_getText(strsrch, &textlength) + matchindex,
                   matchlength * sizeof(UChar)) != 0) {
            log_err("Error getting following matched text\n");
        }

        matchindex = search.offset[count];
    }
    usearch_next(strsrch, &status);
    if (usearch_getMatchedStart(strsrch) != USEARCH_DONE ||
        usearch_getMatchedLength(strsrch) != 0) {
        char *str = toCharString(usearch_getText(strsrch, &textlength));
        log_err("Text: %s\n", str);
        str = toCharString(usearch_getPattern(strsrch, &textlength));
        log_err("Pattern: %s\n", str);
        log_err("Error following match found at %d %d\n", 
                    usearch_getMatchedStart(strsrch), 
                    usearch_getMatchedLength(strsrch));
        return FALSE;
    }
    /* start of preceding matches */
    count = count == 0 ? 0 : count - 1;
    matchlimit = count;
    matchindex = search.offset[count];

    while (U_SUCCESS(status) && matchindex >= 0) {
        uint32_t matchlength = search.size[count];
        usearch_previous(strsrch, &status);
        if (matchindex != usearch_getMatchedStart(strsrch) || 
            matchlength != (uint32_t)usearch_getMatchedLength(strsrch)) {
            char *str = toCharString(usearch_getText(strsrch, &textlength));
            log_err("Text: %s\n", str);
            str = toCharString(usearch_getPattern(strsrch, &textlength));
            log_err("Pattern: %s\n", str);
            log_err("Error preceding match found at %d %d\n", 
                    usearch_getMatchedStart(strsrch), 
                    usearch_getMatchedLength(strsrch));
            return FALSE;
        }
        
        if (usearch_getMatchedText(strsrch, matchtext, 128, &status) !=
            (int32_t) matchlength || U_FAILURE(status) ||
            memcmp(matchtext, 
                   usearch_getText(strsrch, &textlength) + matchindex,
                   matchlength * sizeof(UChar)) != 0) {
            log_err("Error getting preceding matched text\n");
        }

        matchindex = count > 0 ? search.offset[count - 1] : -1;
        count --;
    }
    usearch_previous(strsrch, &status);
    if (usearch_getMatchedStart(strsrch) != USEARCH_DONE ||
        usearch_getMatchedLength(strsrch) != 0) {
        char *str = toCharString(usearch_getText(strsrch, &textlength));
        log_err("Text: %s\n", str);
        str = toCharString(usearch_getPattern(strsrch, &textlength));
        log_err("Pattern: %s\n", str);
        log_err("Error preceding match found at %d %d\n", 
                    usearch_getMatchedStart(strsrch), 
                    usearch_getMatchedLength(strsrch));
        return FALSE;
    }

    usearch_setAttribute(strsrch, USEARCH_ELEMENT_COMPARISON, USEARCH_STANDARD_ELEMENT_COMPARISON, &status);
    return TRUE;
}

static UBool assertEqual(const SearchData search)
{
    UErrorCode      status      = U_ZERO_ERROR;
    UChar           pattern[32];
    UChar           text[128];
    UCollator      *collator = getCollator(search.collator);
    UBreakIterator *breaker  = getBreakIterator(search.breaker);
    UStringSearch  *strsrch; 
    
    CHECK_BREAK_BOOL(search.breaker);

    u_unescape(search.text, text, 128);
    u_unescape(search.pattern, pattern, 32);
    ucol_setStrength(collator, search.strength);
    strsrch = usearch_openFromCollator(pattern, -1, text, -1, collator, 
                                       breaker, &status);
    if (U_FAILURE(status)) {
        log_err("Error opening string search %s\n", u_errorName(status));
        return FALSE;
    }   
    
    if (!assertEqualWithUStringSearch(strsrch, search)) {
        ucol_setStrength(collator, UCOL_TERTIARY);
        usearch_close(strsrch);
        return FALSE;
    }
    ucol_setStrength(collator, UCOL_TERTIARY);
    usearch_close(strsrch);
    return TRUE;
}

static UBool assertCanonicalEqual(const SearchData search)
{
    UErrorCode      status      = U_ZERO_ERROR;
    UChar           pattern[32];
    UChar           text[128];
    UCollator      *collator = getCollator(search.collator);
    UBreakIterator *breaker  = getBreakIterator(search.breaker);
    UStringSearch  *strsrch; 
    UBool           result = TRUE;
    
    CHECK_BREAK_BOOL(search.breaker);
    u_unescape(search.text, text, 128);
    u_unescape(search.pattern, pattern, 32);
    ucol_setStrength(collator, search.strength);
    ucol_setAttribute(collator, UCOL_NORMALIZATION_MODE, UCOL_ON, &status);
    strsrch = usearch_openFromCollator(pattern, -1, text, -1, collator, 
                                       breaker, &status);                                   
    usearch_setAttribute(strsrch, USEARCH_CANONICAL_MATCH, USEARCH_ON,
                         &status);
    if (U_FAILURE(status)) {
        log_err("Error opening string search %s\n", u_errorName(status));
        result = FALSE;
        goto bail;
    }   
    
    if (!assertEqualWithUStringSearch(strsrch, search)) {
        ucol_setStrength(collator, UCOL_TERTIARY);
        usearch_close(strsrch);
        result = FALSE;
        goto bail;
    }

bail:
    ucol_setAttribute(collator, UCOL_NORMALIZATION_MODE, UCOL_OFF, &status);
    ucol_setStrength(collator, UCOL_TERTIARY);
    usearch_close(strsrch);
    return result;
}

static UBool assertEqualWithAttribute(const SearchData            search, 
                                            USearchAttributeValue canonical,
                                            USearchAttributeValue overlap)
{
    UErrorCode      status      = U_ZERO_ERROR;
    UChar           pattern[32];
    UChar           text[128];
    UCollator      *collator = getCollator(search.collator);
    UBreakIterator *breaker  = getBreakIterator(search.breaker);
    UStringSearch  *strsrch; 
    
    CHECK_BREAK_BOOL(search.breaker);
    u_unescape(search.text, text, 128);
    u_unescape(search.pattern, pattern, 32);
    ucol_setStrength(collator, search.strength);
    strsrch = usearch_openFromCollator(pattern, -1, text, -1, collator, 
                                       breaker, &status);
    usearch_setAttribute(strsrch, USEARCH_CANONICAL_MATCH, canonical, 
                         &status);
    usearch_setAttribute(strsrch, USEARCH_OVERLAP, overlap, &status);
    
    if (U_FAILURE(status)) {
        log_err("Error opening string search %s\n", u_errorName(status));
        return FALSE;
    }   
    
    if (!assertEqualWithUStringSearch(strsrch, search)) {
            ucol_setStrength(collator, UCOL_TERTIARY);
            usearch_close(strsrch);
            return FALSE;
    }
    ucol_setStrength(collator, UCOL_TERTIARY);
    usearch_close(strsrch);
    return TRUE;
}

static void TestBasic(void) 
{
    int count = 0;
    UErrorCode status = U_ZERO_ERROR;
    open(&status);
    if (U_FAILURE(status)) {
        log_err_status(status, "Unable to open static collators %s\n", u_errorName(status));
        return;
    }
    while (BASIC[count].text != NULL) {
        if (!assertEqual(BASIC[count])) {
            log_err("Error at test number %d\n", count);
        }
        count ++;
    }
    close();
}

static void TestNormExact(void) 
{
    int count = 0;
    UErrorCode status = U_ZERO_ERROR;
    open(&status);
    if (U_FAILURE(status)) {
        log_err_status(status, "Unable to open static collators %s\n", u_errorName(status));
        return;
    }
    ucol_setAttribute(EN_US_, UCOL_NORMALIZATION_MODE, UCOL_ON, &status);
    if (U_FAILURE(status)) {
        log_err("Error setting collation normalization %s\n", 
            u_errorName(status));
    }
    while (BASIC[count].text != NULL) {
        if (!assertEqual(BASIC[count])) {
            log_err("Error at test number %d\n", count);
        }
        count ++;
    }
    count = 0;
    while (NORMEXACT[count].text != NULL) {
        if (!assertEqual(NORMEXACT[count])) {
            log_err("Error at test number %d\n", count);
        }
        count ++;
    }
    ucol_setAttribute(EN_US_, UCOL_NORMALIZATION_MODE, UCOL_OFF, &status);
    count = 0;
    while (NONNORMEXACT[count].text != NULL) {
        if (!assertEqual(NONNORMEXACT[count])) {
            log_err("Error at test number %d\n", count);
        }
        count ++;
    }
    close();
}

static void TestStrength(void) 
{
    int count = 0;
    UErrorCode status = U_ZERO_ERROR;
    open(&status);
    if (U_FAILURE(status)) {
        log_err_status(status, "Unable to open static collators %s\n", u_errorName(status));
        return;
    }
    while (STRENGTH[count].text != NULL) {
        if (!assertEqual(STRENGTH[count])) {
            log_err("Error at test number %d\n", count);
        }
        count ++;
    }
    close();
}

static void TestBreakIterator(void) {
    UErrorCode      status      = U_ZERO_ERROR;
    UStringSearch  *strsrch; 
    UChar           text[128];
    UChar           pattern[32];
    int             count = 0;

    CHECK_BREAK("x");

#if !UCONFIG_NO_BREAK_ITERATION
    open(&status);
    if (U_FAILURE(status)) {
        log_err_status(status, "Unable to open static collators %s\n", u_errorName(status));
        return;
    }
    if (usearch_getBreakIterator(NULL) != NULL) {
        log_err("Expected NULL breakiterator from NULL string search\n");
    }
    u_unescape(BREAKITERATOREXACT[0].text, text, 128);
    u_unescape(BREAKITERATOREXACT[0].pattern, pattern, 32);
    strsrch = usearch_openFromCollator(pattern, -1, text, -1, EN_US_, NULL, 
                                       &status);
    if (U_FAILURE(status)) {
        log_err("Error opening string search %s\n", u_errorName(status));
        goto ENDTESTBREAKITERATOR;
    }
    
    usearch_setBreakIterator(strsrch, NULL, &status);
    if (U_FAILURE(status) || usearch_getBreakIterator(strsrch) != NULL) {
        log_err("Error usearch_getBreakIterator returned wrong object");
        goto ENDTESTBREAKITERATOR;
    }

    usearch_setBreakIterator(strsrch, EN_CHARACTERBREAKER_, &status);
    if (U_FAILURE(status) || 
        usearch_getBreakIterator(strsrch) != EN_CHARACTERBREAKER_) {
        log_err("Error usearch_getBreakIterator returned wrong object");
        goto ENDTESTBREAKITERATOR;
    }
    
    usearch_setBreakIterator(strsrch, EN_WORDBREAKER_, &status);
    if (U_FAILURE(status) || 
        usearch_getBreakIterator(strsrch) != EN_WORDBREAKER_) {
        log_err("Error usearch_getBreakIterator returned wrong object");
        goto ENDTESTBREAKITERATOR;
    }

    usearch_close(strsrch);

    count = 0;
    while (count < 4) {
        /* 0-3 test are fixed */
        const SearchData     *search   = &(BREAKITERATOREXACT[count]);     
              UCollator      *collator = getCollator(search->collator);
              UBreakIterator *breaker  = getBreakIterator(search->breaker);
    
        u_unescape(search->text, text, 128);
        u_unescape(search->pattern, pattern, 32);
        ucol_setStrength(collator, search->strength);
        
        strsrch = usearch_openFromCollator(pattern, -1, text, -1, collator, 
                                           breaker, &status);
        if (U_FAILURE(status) || 
            usearch_getBreakIterator(strsrch) != breaker) {
            log_err("Error setting break iterator\n");
            if (strsrch != NULL) {
                usearch_close(strsrch);
            }
        }
        if (!assertEqualWithUStringSearch(strsrch, *search)) {
            ucol_setStrength(collator, UCOL_TERTIARY);
            usearch_close(strsrch);
            goto ENDTESTBREAKITERATOR;
        }
        search   = &(BREAKITERATOREXACT[count + 1]);
        breaker  = getBreakIterator(search->breaker);
        usearch_setBreakIterator(strsrch, breaker, &status);
        if (U_FAILURE(status) || usearch_getBreakIterator(strsrch) != breaker) {
            log_err("Error setting break iterator\n");
            usearch_close(strsrch);
            goto ENDTESTBREAKITERATOR;
        }
        usearch_reset(strsrch);
        if (!assertEqualWithUStringSearch(strsrch, *search)) {
            log_err("Error at test number %d\n", count);
            usearch_close(strsrch);
            goto ENDTESTBREAKITERATOR;
        }
        usearch_close(strsrch);
        count += 2;
    }
    count = 0;
    while (BREAKITERATOREXACT[count].text != NULL) {
         if (!assertEqual(BREAKITERATOREXACT[count])) {
             log_err("Error at test number %d\n", count);
             goto ENDTESTBREAKITERATOR;
         }
         count ++;
    }
    
ENDTESTBREAKITERATOR:
    close();
#endif
}

static void TestVariable(void) 
{
    int count = 0;
    UErrorCode status = U_ZERO_ERROR;
    open(&status);
    if (U_FAILURE(status)) {
        log_err_status(status, "Unable to open static collators %s\n", u_errorName(status));
        return;
    }
    ucol_setAttribute(EN_US_, UCOL_ALTERNATE_HANDLING, UCOL_SHIFTED, &status);
    if (U_FAILURE(status)) {
        log_err("Error setting collation alternate attribute %s\n", 
            u_errorName(status));
    }
    while (VARIABLE[count].text != NULL) {
        log_verbose("variable %d\n", count);
        if (!assertEqual(VARIABLE[count])) {
            log_err("Error at test number %d\n", count);
        }
        count ++;
    }
    ucol_setAttribute(EN_US_, UCOL_ALTERNATE_HANDLING, 
                      UCOL_NON_IGNORABLE, &status);
    close();
}

static void TestOverlap(void)
{
    int count = 0;
    UErrorCode status = U_ZERO_ERROR;
    open(&status);
    if (U_FAILURE(status)) {
        log_err_status(status, "Unable to open static collators %s\n", u_errorName(status));
        return;
    }
    while (OVERLAP[count].text != NULL) {
        if (!assertEqualWithAttribute(OVERLAP[count], USEARCH_OFF, 
                                      USEARCH_ON)) {
            log_err("Error at overlap test number %d\n", count);
        }
        count ++;
    }    
    count = 0;
    while (NONOVERLAP[count].text != NULL) {
        if (!assertEqual(NONOVERLAP[count])) {
            log_err("Error at non overlap test number %d\n", count);
        }
        count ++;
    }

    count = 0;
    while (count < 1) {
              UChar           pattern[32];
              UChar           text[128];
        const SearchData     *search   = &(OVERLAP[count]);     
              UCollator      *collator = getCollator(search->collator);
              UStringSearch  *strsrch; 
              status   = U_ZERO_ERROR;
    
        u_unescape(search->text, text, 128);
        u_unescape(search->pattern, pattern, 32);
        strsrch = usearch_openFromCollator(pattern, -1, text, -1, collator, 
                                           NULL, &status);
        if(status == U_FILE_ACCESS_ERROR) {
          log_data_err("Is your data around?\n");
          return;
        } else if(U_FAILURE(status)) {
          log_err("Error opening searcher\n");
          return;
        }
        usearch_setAttribute(strsrch, USEARCH_OVERLAP, USEARCH_ON, &status);
        if (U_FAILURE(status) ||
            usearch_getAttribute(strsrch, USEARCH_OVERLAP) != USEARCH_ON) {
            log_err("Error setting overlap option\n");
        }
        if (!assertEqualWithUStringSearch(strsrch, *search)) {
            usearch_close(strsrch);
            return;
        }
        search   = &(NONOVERLAP[count]);
        usearch_setAttribute(strsrch, USEARCH_OVERLAP, USEARCH_OFF, &status);
        if (U_FAILURE(status) ||
            usearch_getAttribute(strsrch, USEARCH_OVERLAP) != USEARCH_OFF) {
            log_err("Error setting overlap option\n");
        }
        usearch_reset(strsrch);
        if (!assertEqualWithUStringSearch(strsrch, *search)) {
            usearch_close(strsrch);
            log_err("Error at test number %d\n", count);
         }

        count ++;
        usearch_close(strsrch);
    }
    close();
}

static void TestCollator(void) 
{
    /* test collator that thinks "o" and "p" are the same thing */
          UChar          rules[32];
          UCollator     *tailored = NULL; 
          UErrorCode     status   = U_ZERO_ERROR;
          UChar          pattern[32];
          UChar          text[128];
          UStringSearch *strsrch; 
          
    text[0] = 0x41;
    text[1] = 0x42;
    text[2] = 0x43;
    text[3] = 0x44;
    text[4] = 0x45;
    pattern[0] = 0x62;
    pattern[1] = 0x63;
    strsrch  = usearch_open(pattern, 2, text, 5, "en_US",  NULL,  &status);
    if(status == U_FILE_ACCESS_ERROR) {
      log_data_err("Is your data around?\n");
      return;
    } else if(U_FAILURE(status)) {
      log_err("Error opening searcher\n");
      return;
    }
    tailored = usearch_getCollator(strsrch);
    if (usearch_next(strsrch, &status) != -1) {
        log_err("Error: Found case insensitive match, when we shouldn't\n");
    }
    ucol_setStrength(tailored, UCOL_PRIMARY);
    usearch_reset(strsrch);
    if (usearch_next(strsrch, &status) != 1) {
        log_err("Error: Found case insensitive match not found\n");
    }
    usearch_close(strsrch);

    open(&status);

    if (usearch_getCollator(NULL) != NULL) {
        log_err("Expected NULL collator from NULL string search\n");
    }
    u_unescape(COLLATOR[0].text, text, 128);
    u_unescape(COLLATOR[0].pattern, pattern, 32);

    strsrch = usearch_openFromCollator(pattern, -1, text, -1, EN_US_, 
                                       NULL, &status);
    if (U_FAILURE(status)) {
        log_err("Error opening string search %s\n", u_errorName(status));
    }
    if (!assertEqualWithUStringSearch(strsrch, COLLATOR[0])) {
        goto ENDTESTCOLLATOR;
    }
    
    u_unescape(TESTCOLLATORRULE, rules, 32);
    tailored = ucol_openRules(rules, -1, UCOL_ON, COLLATOR[1].strength, 
                              NULL, &status);
    if (U_FAILURE(status)) {
        log_err("Error opening rule based collator %s\n", u_errorName(status));
    }

    usearch_setCollator(strsrch, tailored, &status);
    if (U_FAILURE(status) || usearch_getCollator(strsrch) != tailored) {
        log_err("Error setting rule based collator\n");
    }
    usearch_reset(strsrch);
    if (!assertEqualWithUStringSearch(strsrch, COLLATOR[1])) {
        goto ENDTESTCOLLATOR;
    }
        
    usearch_setCollator(strsrch, EN_US_, &status);
    usearch_reset(strsrch);
    if (U_FAILURE(status) || usearch_getCollator(strsrch) != EN_US_) {
        log_err("Error setting rule based collator\n");
    }
    if (!assertEqualWithUStringSearch(strsrch, COLLATOR[0])) {
        goto ENDTESTCOLLATOR;
    }
    
ENDTESTCOLLATOR:
    usearch_close(strsrch);
    if (tailored != NULL) {
        ucol_close(tailored);
    }
    close();
}

static void TestPattern(void)
{
          UStringSearch *strsrch; 
          UChar          pattern[32];
          UChar          bigpattern[512];
          UChar          text[128];
    const UChar         *temp;
          int32_t        templength;
          UErrorCode     status = U_ZERO_ERROR;

    open(&status);
    if (U_FAILURE(status)) {
        log_err_status(status, "Unable to open static collators %s\n", u_errorName(status));
        return;
    }
    if (usearch_getPattern(NULL, &templength) != NULL) {
        log_err("Error NULL string search expected returning NULL pattern\n");
    }
    usearch_setPattern(NULL, pattern, 3, &status);
    if (U_SUCCESS(status)) {
        log_err("Error expected setting pattern in NULL strings search\n");
    }
    status = U_ZERO_ERROR;
    u_unescape(PATTERN[0].text, text, 128);
    u_unescape(PATTERN[0].pattern, pattern, 32);

    ucol_setStrength(EN_US_, PATTERN[0].strength);
    strsrch = usearch_openFromCollator(pattern, -1, text, -1, EN_US_, 
                                       NULL, &status);
    if(status == U_FILE_ACCESS_ERROR) {
      log_data_err("Is your data around?\n");
      return;
    } else if(U_FAILURE(status)) {
      log_err("Error opening searcher\n");
      return;
    }

    status = U_ZERO_ERROR;
    usearch_setPattern(strsrch, NULL, 3, &status);
    if (U_SUCCESS(status)) {
        log_err("Error expected setting NULL pattern in strings search\n");
    }
    status = U_ZERO_ERROR;
    usearch_setPattern(strsrch, pattern, 0, &status);
    if (U_SUCCESS(status)) {
        log_err("Error expected setting pattern with length 0 in strings search\n");
    }
    status = U_ZERO_ERROR;
    if (U_FAILURE(status)) {
        log_err("Error opening string search %s\n", u_errorName(status));
        goto ENDTESTPATTERN;
    }
    temp = usearch_getPattern(strsrch, &templength);
    if (u_strcmp(pattern, temp) != 0) {
        log_err("Error setting pattern\n");
    }
    if (!assertEqualWithUStringSearch(strsrch, PATTERN[0])) {
        goto ENDTESTPATTERN;
    }

    u_unescape(PATTERN[1].pattern, pattern, 32);
    usearch_setPattern(strsrch, pattern, -1, &status);
    temp = usearch_getPattern(strsrch, &templength);
    if (u_strcmp(pattern, temp) != 0) {
        log_err("Error setting pattern\n");
        goto ENDTESTPATTERN;
    }
    usearch_reset(strsrch);
    if (U_FAILURE(status)) {
        log_err("Error setting pattern %s\n", u_errorName(status));
    }
    if (!assertEqualWithUStringSearch(strsrch, PATTERN[1])) {
        goto ENDTESTPATTERN;
    }

    u_unescape(PATTERN[0].pattern, pattern, 32);
    usearch_setPattern(strsrch, pattern, -1, &status);
    temp = usearch_getPattern(strsrch, &templength);
    if (u_strcmp(pattern, temp) != 0) {
        log_err("Error setting pattern\n");
        goto ENDTESTPATTERN;
    }
    usearch_reset(strsrch);
    if (U_FAILURE(status)) {
        log_err("Error setting pattern %s\n", u_errorName(status));
    }
    if (!assertEqualWithUStringSearch(strsrch, PATTERN[0])) {
        goto ENDTESTPATTERN;
    }
    /* enormous pattern size to see if this crashes */
    for (templength = 0; templength != 512; templength ++) {
        bigpattern[templength] = 0x61;
    }
    bigpattern[511] = 0;
    usearch_setPattern(strsrch, bigpattern, -1, &status);
    if (U_FAILURE(status)) {
        log_err("Error setting pattern with size 512, %s \n", 
            u_errorName(status));
    }
ENDTESTPATTERN:
    ucol_setStrength(EN_US_, UCOL_TERTIARY);
    if (strsrch != NULL) {
        usearch_close(strsrch);
    }
    close();
}

static void TestText(void) 
{
          UStringSearch *strsrch; 
          UChar          pattern[32];
          UChar          text[128];
    const UChar         *temp;
          int32_t        templength;
          UErrorCode     status = U_ZERO_ERROR;

    u_unescape(TEXT[0].text, text, 128);
    u_unescape(TEXT[0].pattern, pattern, 32);

    open(&status);
    if (U_FAILURE(status)) {
        log_err_status(status, "Unable to open static collators %s\n", u_errorName(status));
        return;
    }

    if (usearch_getText(NULL, &templength) != NULL) {
        log_err("Error NULL string search should return NULL text\n");
    }

    usearch_setText(NULL, text, 10, &status);
    if (U_SUCCESS(status)) {
        log_err("Error NULL string search should have an error when setting text\n");
    }

    status = U_ZERO_ERROR;
    strsrch = usearch_openFromCollator(pattern, -1, text, -1, EN_US_, 
                                       NULL, &status);

    if (U_FAILURE(status)) {
        log_err("Error opening string search %s\n", u_errorName(status));
        goto ENDTESTPATTERN;
    }
    temp = usearch_getText(strsrch, &templength);
    if (u_strcmp(text, temp) != 0) {
        log_err("Error setting text\n");
    }
    if (!assertEqualWithUStringSearch(strsrch, TEXT[0])) {
        goto ENDTESTPATTERN;
    }

    u_unescape(TEXT[1].text, text, 32);
    usearch_setText(strsrch, text, -1, &status);
    temp = usearch_getText(strsrch, &templength);
    if (u_strcmp(text, temp) != 0) {
        log_err("Error setting text\n");
        goto ENDTESTPATTERN;
    }
    if (U_FAILURE(status)) {
        log_err("Error setting text %s\n", u_errorName(status));
    }
    if (!assertEqualWithUStringSearch(strsrch, TEXT[1])) {
        goto ENDTESTPATTERN;
    }

    u_unescape(TEXT[0].text, text, 32);
    usearch_setText(strsrch, text, -1, &status);
    temp = usearch_getText(strsrch, &templength);
    if (u_strcmp(text, temp) != 0) {
        log_err("Error setting text\n");
        goto ENDTESTPATTERN;
    }
    if (U_FAILURE(status)) {
        log_err("Error setting pattern %s\n", u_errorName(status));
    }
    if (!assertEqualWithUStringSearch(strsrch, TEXT[0])) {
        goto ENDTESTPATTERN;
    }
ENDTESTPATTERN:
    if (strsrch != NULL) {
        usearch_close(strsrch);
    }
    close();
}

static void TestCompositeBoundaries(void) 
{
    int count = 0;
    UErrorCode status = U_ZERO_ERROR;
    open(&status);
    if (U_FAILURE(status)) {
        log_err_status(status, "Unable to open static collators %s\n", u_errorName(status));
        return;
    }
    while (COMPOSITEBOUNDARIES[count].text != NULL) { 
        log_verbose("composite %d\n", count);
        if (!assertEqual(COMPOSITEBOUNDARIES[count])) {
            log_err("Error at test number %d\n", count);
        }
        count ++;
    } 
    close();
}

static void TestGetSetOffset(void)
{
    int            searchDataIndex   = 0;
    UChar          pattern[32];
    UChar          text[128];
    UErrorCode     status  = U_ZERO_ERROR;
    UStringSearch *strsrch;
    memset(pattern, 0, 32*sizeof(UChar));
    memset(text, 0, 128*sizeof(UChar));

    open(&status);
    if (U_FAILURE(status)) {
        log_err_status(status, "Unable to open static collators %s\n", u_errorName(status));
        return;
    }
    if (usearch_getOffset(NULL) != USEARCH_DONE) {
        log_err("usearch_getOffset(NULL) expected USEARCH_DONE\n");
    }
    strsrch = usearch_openFromCollator(pattern, 16, text, 32, EN_US_, NULL, 
                                       &status);
    /* testing out of bounds error */
    usearch_setOffset(strsrch, -1, &status);
    if (U_SUCCESS(status)) {
        log_err("Error expecting set offset error\n");
    }
    usearch_setOffset(strsrch, 128, &status);
    if (U_SUCCESS(status)) {
        log_err("Error expecting set offset error\n");
    }
    while (BASIC[searchDataIndex].text != NULL) {
        int         count       = 0;
        SearchData  search      = BASIC[searchDataIndex ++];
        int32_t matchindex  = search.offset[count];
        int32_t     textlength;
        
        u_unescape(search.text, text, 128);
        u_unescape(search.pattern, pattern, 32);
        status = U_ZERO_ERROR;
        usearch_setText(strsrch, text, -1, &status);
        usearch_setPattern(strsrch, pattern, -1, &status);
        ucol_setStrength(usearch_getCollator(strsrch), search.strength);
        usearch_reset(strsrch);
        while (U_SUCCESS(status) && matchindex >= 0) {
            uint32_t matchlength = search.size[count];
            usearch_next(strsrch, &status);
            if (matchindex != usearch_getMatchedStart(strsrch) || 
                matchlength != (uint32_t)usearch_getMatchedLength(strsrch)) {
                char *str = toCharString(usearch_getText(strsrch, 
                                                         &textlength));
                log_err("Text: %s\n", str);
                str = toCharString(usearch_getPattern(strsrch, &textlength));
                log_err("Pattern: %s\n", str);
                log_err("Error match found at %d %d\n", 
                        usearch_getMatchedStart(strsrch), 
                        usearch_getMatchedLength(strsrch));
                return;
            }
            usearch_setOffset(strsrch, matchindex + matchlength, &status);
            usearch_previous(strsrch, &status);
            if (matchindex != usearch_getMatchedStart(strsrch) || 
                matchlength != (uint32_t)usearch_getMatchedLength(strsrch)) {
                char *str = toCharString(usearch_getText(strsrch, 
                                                         &textlength));
                log_err("Text: %s\n", str);
                str = toCharString(usearch_getPattern(strsrch, &textlength));
                log_err("Pattern: %s\n", str);
                log_err("Error match found at %d %d\n", 
                        usearch_getMatchedStart(strsrch), 
                        usearch_getMatchedLength(strsrch));
                return;
            }
            usearch_setOffset(strsrch, matchindex + matchlength, &status);
            matchindex = search.offset[count + 1] == -1 ? -1 : 
                         search.offset[count + 2];
            if (search.offset[count + 1] != -1) {
                usearch_setOffset(strsrch, search.offset[count + 1] + 1, 
                                  &status);
                if (usearch_getOffset(strsrch) != search.offset[count + 1] + 1) {
                    log_err("Error setting offset\n");
                    return;
                }
            }
            
            count += 2;
        }
        usearch_next(strsrch, &status);
        if (usearch_getMatchedStart(strsrch) != USEARCH_DONE) {
            char *str = toCharString(usearch_getText(strsrch, &textlength));
            log_err("Text: %s\n", str);
            str = toCharString(usearch_getPattern(strsrch, &textlength));
            log_err("Pattern: %s\n", str);
            log_err("Error match found at %d %d\n", 
                        usearch_getMatchedStart(strsrch), 
                        usearch_getMatchedLength(strsrch));
            return;
        }
    }
    ucol_setStrength(usearch_getCollator(strsrch), UCOL_TERTIARY);
    usearch_close(strsrch);
    close();
}

static void TestGetSetAttribute(void) 
{
    UErrorCode      status    = U_ZERO_ERROR;
    UChar           pattern[32];
    UChar           text[128];
    UStringSearch  *strsrch;

    memset(pattern, 0, 32*sizeof(UChar));
    memset(text, 0, 128*sizeof(UChar));
          
    open(&status);
    if (U_FAILURE(status)) {
        log_err_status(status, "Unable to open static collators %s\n", u_errorName(status));
        return;
    }
    if (usearch_getAttribute(NULL, USEARCH_OVERLAP) != USEARCH_DEFAULT ||
        usearch_getAttribute(NULL, USEARCH_CANONICAL_MATCH) != 
                                                         USEARCH_DEFAULT) {
        log_err(
            "Attributes for NULL string search should be USEARCH_DEFAULT\n");
    }
    strsrch = usearch_openFromCollator(pattern, 16, text, 32, EN_US_, NULL, 
                                       &status);
    if (U_FAILURE(status)) {
        log_err("Error opening search %s\n", u_errorName(status));
        return;
    }

    usearch_setAttribute(strsrch, USEARCH_OVERLAP, USEARCH_DEFAULT, &status);
    if (U_FAILURE(status) || 
        usearch_getAttribute(strsrch, USEARCH_OVERLAP) != USEARCH_OFF) {
        log_err("Error setting overlap to the default\n");
    }
    usearch_setAttribute(strsrch, USEARCH_OVERLAP, USEARCH_ON, &status);
    if (U_FAILURE(status) || 
        usearch_getAttribute(strsrch, USEARCH_OVERLAP) != USEARCH_ON) {
        log_err("Error setting overlap true\n");
    }
    usearch_setAttribute(strsrch, USEARCH_OVERLAP, USEARCH_OFF, &status);
    if (U_FAILURE(status) || 
        usearch_getAttribute(strsrch, USEARCH_OVERLAP) != USEARCH_OFF) {
        log_err("Error setting overlap false\n");
    }
    usearch_setAttribute(strsrch, USEARCH_OVERLAP, 
                         USEARCH_ATTRIBUTE_VALUE_COUNT, &status);
    if (U_SUCCESS(status)) {
        log_err("Error setting overlap to illegal value\n");
    }
    status = U_ZERO_ERROR;
    usearch_setAttribute(strsrch, USEARCH_CANONICAL_MATCH, USEARCH_DEFAULT, 
                         &status);
    if (U_FAILURE(status) || 
        usearch_getAttribute(strsrch, USEARCH_CANONICAL_MATCH) != 
                                                        USEARCH_OFF) {
        log_err("Error setting canonical match to the default\n");
    }
    usearch_setAttribute(strsrch, USEARCH_CANONICAL_MATCH, USEARCH_ON, 
                         &status);
    if (U_FAILURE(status) || 
        usearch_getAttribute(strsrch, USEARCH_CANONICAL_MATCH) != 
                                                         USEARCH_ON) {
        log_err("Error setting canonical match true\n");
    }
    usearch_setAttribute(strsrch, USEARCH_CANONICAL_MATCH, USEARCH_OFF, 
                         &status);
    if (U_FAILURE(status) || 
        usearch_getAttribute(strsrch, USEARCH_CANONICAL_MATCH) != 
                                                        USEARCH_OFF) {
        log_err("Error setting canonical match false\n");
    }
    usearch_setAttribute(strsrch, USEARCH_CANONICAL_MATCH, 
                         USEARCH_ATTRIBUTE_VALUE_COUNT, &status);
    if (U_SUCCESS(status)) {
        log_err("Error setting canonical match to illegal value\n");
    }
    status = U_ZERO_ERROR;
    usearch_setAttribute(strsrch, USEARCH_ATTRIBUTE_COUNT, USEARCH_DEFAULT, 
                         &status);
    if (U_SUCCESS(status)) {
        log_err("Error setting illegal attribute success\n");
    }

    usearch_close(strsrch);
    close();
}

static void TestGetMatch(void)
{
    int            count       = 0;
    UErrorCode     status      = U_ZERO_ERROR;
    UChar          text[128];
    UChar          pattern[32];
    SearchData     search      = MATCH[0];
    int32_t    matchindex  = search.offset[count];
    UStringSearch *strsrch;
    int32_t        textlength;
    UChar          matchtext[128];
    
    open(&status);
    if (U_FAILURE(status)) {
        log_err_status(status, "Unable to open static collators %s\n", u_errorName(status));
        return;
    }

    if (usearch_getMatchedStart(NULL) != USEARCH_DONE || 
        usearch_getMatchedLength(NULL) != USEARCH_DONE) {
        log_err(
   "Expected start and length of NULL string search should be USEARCH_DONE\n");
    }

    u_unescape(search.text, text, 128);
    u_unescape(search.pattern, pattern, 32);
    strsrch = usearch_openFromCollator(pattern, -1, text, -1, EN_US_, 
                                       NULL, &status);
    if (U_FAILURE(status)) {
        log_err("Error opening string search %s\n", u_errorName(status));
        if (strsrch != NULL) {
            usearch_close(strsrch);
        }
        return;
    }
    
    while (U_SUCCESS(status) && matchindex >= 0) {
        int32_t matchlength = search.size[count];
        usearch_next(strsrch, &status);
        if (matchindex != usearch_getMatchedStart(strsrch) || 
            matchlength != usearch_getMatchedLength(strsrch)) {
            char *str = toCharString(usearch_getText(strsrch, &textlength));
            log_err("Text: %s\n", str);
            str = toCharString(usearch_getPattern(strsrch, &textlength));
            log_err("Pattern: %s\n", str);
            log_err("Error match found at %d %d\n", 
                    usearch_getMatchedStart(strsrch), 
                    usearch_getMatchedLength(strsrch));
            return;
        }
        count ++;
        
        status = U_ZERO_ERROR;
        if (usearch_getMatchedText(NULL, matchtext, 128, &status) != 
            USEARCH_DONE || U_SUCCESS(status)){
            log_err("Error expecting errors with NULL string search\n");
        }
        status = U_ZERO_ERROR;
        if (usearch_getMatchedText(strsrch, NULL, 0, &status) != 
            (int32_t)matchlength || U_SUCCESS(status)){
            log_err("Error pre-flighting match length\n");
        }
        status = U_ZERO_ERROR;
        if (usearch_getMatchedText(strsrch, matchtext, 0, &status) != 
            (int32_t)matchlength || U_SUCCESS(status)){
            log_err("Error getting match text with buffer size 0\n");
        }
        status = U_ZERO_ERROR;
        if (usearch_getMatchedText(strsrch, matchtext, matchlength, &status) 
            != (int32_t)matchlength || matchtext[matchlength - 1] == 0 ||
            U_FAILURE(status)){
            log_err("Error getting match text with exact size\n");
        }
        status = U_ZERO_ERROR;
        if (usearch_getMatchedText(strsrch, matchtext, 128, &status) !=
            (int32_t) matchlength || U_FAILURE(status) ||
            memcmp(matchtext, 
                   usearch_getText(strsrch, &textlength) + matchindex,
                   matchlength * sizeof(UChar)) != 0 ||
            matchtext[matchlength] != 0) {
            log_err("Error getting matched text\n");
        }

        matchindex = search.offset[count];
    }
    status = U_ZERO_ERROR;
    usearch_next(strsrch, &status);
    if (usearch_getMatchedStart(strsrch)  != USEARCH_DONE || 
        usearch_getMatchedLength(strsrch) != 0) {
        log_err("Error end of match not found\n");
    }
    status = U_ZERO_ERROR;
    if (usearch_getMatchedText(strsrch, matchtext, 128, &status) != 
        USEARCH_DONE) {
        log_err("Error getting null matches\n");
    }
    usearch_close(strsrch);
    close();
}

static void TestSetMatch(void)
{
    int            count       = 0;
    UErrorCode status = U_ZERO_ERROR;
    open(&status);
    if (U_FAILURE(status)) {
        log_err_status(status, "Unable to open static collators %s\n", u_errorName(status));
        return;
    }
    while (MATCH[count].text != NULL) {
        SearchData     search = MATCH[count];
        int            size   = 0;
        int            offsetIndex = 0;
        UChar          text[128];
        UChar          pattern[32];
        UStringSearch *strsrch;
        status = U_ZERO_ERROR;

        if (usearch_first(NULL, &status) != USEARCH_DONE ||
            usearch_last(NULL, &status) != USEARCH_DONE) {
            log_err("Error getting the first and last match of a NULL string search\n");
        }
        u_unescape(search.text, text, 128);
        u_unescape(search.pattern, pattern, 32);
        strsrch = usearch_openFromCollator(pattern, -1, text, -1, EN_US_, 
                                           NULL, &status);
        if (U_FAILURE(status)) {
            log_err("Error opening string search %s\n", u_errorName(status));
            if (strsrch != NULL) {
                usearch_close(strsrch);
            }
            return;
        }

        size = 0;
        while (search.offset[size] != -1) {
            size ++;
        }

        if (usearch_first(strsrch, &status) != search.offset[0] ||
            U_FAILURE(status)) {
            log_err("Error getting first match\n");
        }
        if (usearch_last(strsrch, &status) != search.offset[size -1] ||
            U_FAILURE(status)) {
            log_err("Error getting last match\n");
        }
        
        while (offsetIndex < size) {
            if (offsetIndex + 2 < size) {
                if (usearch_following(strsrch, search.offset[offsetIndex + 2] - 1,
                                      &status) != search.offset[offsetIndex + 2] ||
                    U_FAILURE(status)) {
                    log_err("Error getting following match at index %d\n", 
                            search.offset[offsetIndex + 2] - 1);
                }
            }
            if (offsetIndex + 1 < size) {
                if (usearch_preceding(strsrch, search.offset[offsetIndex + 1] + 
                                               search.size[offsetIndex + 1] + 1, 
                                      &status) != search.offset[offsetIndex + 1] ||
                    U_FAILURE(status)) {
                    log_err("Error getting preceeding match at index %d\n", 
                            search.offset[offsetIndex + 1] + 1);
                }
            }
            offsetIndex += 2;
        }
        status = U_ZERO_ERROR;
        if (usearch_following(strsrch, u_strlen(text), &status) != 
            USEARCH_DONE) {
            log_err("Error expecting out of bounds match\n");
        }
        if (usearch_preceding(strsrch, 0, &status) != USEARCH_DONE) {
            log_err("Error expecting out of bounds match\n");
        }
        count ++;
        usearch_close(strsrch);
    }
    close();
}

static void TestReset(void)
{
    UErrorCode     status    = U_ZERO_ERROR;
    UChar          text[]    = {0x66, 0x69, 0x73, 0x68, 0x20, 
                                0x66, 0x69, 0x73, 0x68};
    UChar          pattern[] = {0x73};
    UStringSearch *strsrch;
    
    open(&status);
    if (U_FAILURE(status)) {
        log_err_status(status, "Unable to open static collators %s\n", u_errorName(status));
        return;
    }
    strsrch = usearch_openFromCollator(pattern, 1, text, 9, 
                                                      EN_US_, NULL, &status);
    if (U_FAILURE(status)) {
        log_err("Error opening string search %s\n", u_errorName(status));
        if (strsrch != NULL) {
            usearch_close(strsrch);
        }
        return;
    }
    usearch_setAttribute(strsrch, USEARCH_OVERLAP, USEARCH_ON, &status);
    usearch_setAttribute(strsrch, USEARCH_CANONICAL_MATCH, USEARCH_ON, 
                         &status);
    usearch_setOffset(strsrch, 9, &status);
    if (U_FAILURE(status)) {
        log_err("Error setting attributes and offsets\n");
    }
    else {
        usearch_reset(strsrch);
        if (usearch_getAttribute(strsrch, USEARCH_OVERLAP) != USEARCH_OFF ||
            usearch_getAttribute(strsrch, USEARCH_CANONICAL_MATCH) != 
                                 USEARCH_OFF ||
            usearch_getOffset(strsrch) != 0 ||
            usearch_getMatchedLength(strsrch) != 0 ||
            usearch_getMatchedStart(strsrch) != USEARCH_DONE) {
            log_err("Error resetting string search\n");
        }
        usearch_previous(strsrch, &status);
        if (usearch_getMatchedStart(strsrch) != 7 ||
            usearch_getMatchedLength(strsrch) != 1) {
            log_err("Error resetting string search\n");
        }
    }
    usearch_close(strsrch);
    close();
}

static void TestSupplementary(void)
{
    int count = 0;
    UErrorCode status = U_ZERO_ERROR;
    open(&status);
    if (U_FAILURE(status)) {
        log_err_status(status, "Unable to open static collators %s\n", u_errorName(status));
        return;
    }
    while (SUPPLEMENTARY[count].text != NULL) {
        if (!assertEqual(SUPPLEMENTARY[count])) {
            log_err("Error at test number %d\n", count);
        }
        count ++;
    }
    close();
}

static void TestContraction(void) 
{
    UChar          rules[128];
    UChar          pattern[128];
    UChar          text[128];
    UCollator     *collator;
    UErrorCode     status = U_ZERO_ERROR;
    int            count = 0;
    UStringSearch *strsrch;
    memset(rules, 0, 128*sizeof(UChar));
    memset(pattern, 0, 128*sizeof(UChar));
    memset(text, 0, 128*sizeof(UChar));

    u_unescape(CONTRACTIONRULE, rules, 128);
    collator = ucol_openRules(rules, u_strlen(rules), UCOL_ON, 
                              UCOL_TERTIARY, NULL, &status); 
    if(status == U_FILE_ACCESS_ERROR) {
      log_data_err("Is your data around?\n");
      return;
    } else if(U_FAILURE(status)) {
      log_err("Error opening collator %s\n", u_errorName(status));
      return;
    }
    strsrch = usearch_openFromCollator(pattern, 1, text, 1, collator, NULL, 
                                       &status);
    if (U_FAILURE(status)) {
        log_err("Error opening string search %s\n", u_errorName(status));
    }   
    
    while (CONTRACTION[count].text != NULL) {
        u_unescape(CONTRACTION[count].text, text, 128);
        u_unescape(CONTRACTION[count].pattern, pattern, 128);
        usearch_setText(strsrch, text, -1, &status);
        usearch_setPattern(strsrch, pattern, -1, &status);
        if (!assertEqualWithUStringSearch(strsrch, CONTRACTION[count])) {
            log_err("Error at test number %d\n", count);
        }
        count ++;
    }
    usearch_close(strsrch);
    ucol_close(collator);
}

static void TestIgnorable(void) 
{
    UChar          rules[128];
    UChar          pattern[128];
    UChar          text[128];
    UCollator     *collator;
    UErrorCode     status = U_ZERO_ERROR;
    UStringSearch *strsrch;
    uint32_t       count = 0;

    memset(rules, 0, 128*sizeof(UChar));
    memset(pattern, 0, 128*sizeof(UChar));
    memset(text, 0, 128*sizeof(UChar));

    u_unescape(IGNORABLERULE, rules, 128);
    collator = ucol_openRules(rules, u_strlen(rules), UCOL_ON, 
                              IGNORABLE[count].strength, NULL, &status); 
    if(status == U_FILE_ACCESS_ERROR) {
      log_data_err("Is your data around?\n");
      return;
    } else if(U_FAILURE(status)) {
        log_err("Error opening collator %s\n", u_errorName(status));
        return;
    }
    strsrch = usearch_openFromCollator(pattern, 1, text, 1, collator, NULL, 
                                       &status);
    if (U_FAILURE(status)) {
        log_err("Error opening string search %s\n", u_errorName(status));
    }   
    
    while (IGNORABLE[count].text != NULL) {
        u_unescape(IGNORABLE[count].text, text, 128);
        u_unescape(IGNORABLE[count].pattern, pattern, 128);
        usearch_setText(strsrch, text, -1, &status);
        usearch_setPattern(strsrch, pattern, -1, &status);
        if (!assertEqualWithUStringSearch(strsrch, IGNORABLE[count])) {
            log_err("Error at test number %d\n", count);
        }
        count ++;
    }
    usearch_close(strsrch);
    ucol_close(collator);
}

static void TestDiacriticMatch(void) 
{
    UChar          pattern[128];
    UChar          text[128];
    UErrorCode     status = U_ZERO_ERROR;
    UStringSearch *strsrch = NULL;
    UCollator *coll = NULL;
    uint32_t       count = 0;
    SearchData search;

    memset(pattern, 0, 128*sizeof(UChar));
    memset(text, 0, 128*sizeof(UChar));
    
    strsrch = usearch_open(pattern, 1, text, 1, uloc_getDefault(), NULL, &status);
	if (U_FAILURE(status)) {
        log_err_status(status, "Error opening string search %s\n", u_errorName(status));
        return;
    }
       
    search = DIACRITICMATCH[count];
    while (search.text != NULL) {
    	if (search.collator != NULL) {
    		coll = ucol_openFromShortString(search.collator, FALSE, NULL, &status);
    	} else {
            /* Always use "en_US" because some of these tests fail in Danish locales. */
    		coll = ucol_open("en_US"/*uloc_getDefault()*/, &status);
    		ucol_setStrength(coll, search.strength);
    	}
    	if (U_FAILURE(status)) {
	        log_err("Error opening string search collator(\"%s\") %s\n", search.collator, u_errorName(status));
	        return;
	    }
    	
    	usearch_setCollator(strsrch, coll, &status);
    	if (U_FAILURE(status)) {
	        log_err("Error setting string search collator %s\n", u_errorName(status));
	        return;
	    }
    
        u_unescape(search.text, text, 128);
        u_unescape(search.pattern, pattern, 128);
        usearch_setText(strsrch, text, -1, &status);
        usearch_setPattern(strsrch, pattern, -1, &status);
        if (!assertEqualWithUStringSearch(strsrch, search)) {
            log_err("Error at test number %d\n", count);
        }
        ucol_close(coll);
        
        search = DIACRITICMATCH[++count];
    }
    usearch_close(strsrch);
}

static void TestCanonical(void)
{
    int count = 0;
    UErrorCode status = U_ZERO_ERROR;
    open(&status);
    if (U_FAILURE(status)) {
        log_err_status(status, "Unable to open static collators %s\n", u_errorName(status));
        return;
    }
    while (BASICCANONICAL[count].text != NULL) {
        if (!assertCanonicalEqual(BASICCANONICAL[count])) {
            log_err("Error at test number %d\n", count);
        }
        count ++;
    }
    close();
}

static void TestNormCanonical(void) 
{
    int count = 0;
    UErrorCode status = U_ZERO_ERROR;
    open(&status);
    if (U_FAILURE(status)) {
        log_err_status(status, "Unable to open static collators %s\n", u_errorName(status));
        return;
    }
    ucol_setAttribute(EN_US_, UCOL_NORMALIZATION_MODE, UCOL_ON, &status);
    count = 0;
    while (NORMCANONICAL[count].text != NULL) {
        if (!assertCanonicalEqual(NORMCANONICAL[count])) {
            log_err("Error at test number %d\n", count);
        }
        count ++;
    }
    ucol_setAttribute(EN_US_, UCOL_NORMALIZATION_MODE, UCOL_OFF, &status);
    close();
}

static void TestStrengthCanonical(void) 
{
    int count = 0;
    UErrorCode status = U_ZERO_ERROR;
    open(&status);
    if (U_FAILURE(status)) {
        log_err_status(status, "Unable to open static collators %s\n", u_errorName(status));
        return;
    }
    while (STRENGTHCANONICAL[count].text != NULL) {
        if (!assertCanonicalEqual(STRENGTHCANONICAL[count])) {
            log_err("Error at test number %d\n", count);
        }
        count ++;
    }
    close();
}

static void TestBreakIteratorCanonical(void) {
    UErrorCode      status      = U_ZERO_ERROR;
    int             count = 0;

    CHECK_BREAK("x");

#if !UCONFIG_NO_BREAK_ITERATION

    open(&status);
    if (U_FAILURE(status)) {
        log_err_status(status, "Unable to open static collators %s\n", u_errorName(status));
        return;
    }
    while (count < 4) {
        /* 0-3 test are fixed */
              UChar           pattern[32];
              UChar           text[128];
        const SearchData     *search   = &(BREAKITERATORCANONICAL[count]);     
              UCollator      *collator = getCollator(search->collator);
              UBreakIterator *breaker  = getBreakIterator(search->breaker);
              UStringSearch  *strsrch; 
    
        u_unescape(search->text, text, 128);
        u_unescape(search->pattern, pattern, 32);
        ucol_setStrength(collator, search->strength);
        
        strsrch = usearch_openFromCollator(pattern, -1, text, -1, collator, 
                                           breaker, &status);
        if(status == U_FILE_ACCESS_ERROR) {
            log_data_err("Is your data around?\n");
            goto ENDTESTBREAKITERATOR;
        } else if(U_FAILURE(status)) {
            log_err("Error opening searcher\n");
            goto ENDTESTBREAKITERATOR;
        }
        usearch_setAttribute(strsrch, USEARCH_CANONICAL_MATCH, USEARCH_ON, 
                             &status);
        if (U_FAILURE(status) || 
            usearch_getBreakIterator(strsrch) != breaker) {
            log_err("Error setting break iterator\n");
            usearch_close(strsrch);
            goto ENDTESTBREAKITERATOR;
        }
        if (!assertEqualWithUStringSearch(strsrch, *search)) {
            ucol_setStrength(collator, UCOL_TERTIARY);
            usearch_close(strsrch);
            goto ENDTESTBREAKITERATOR;
        }
        search   = &(BREAKITERATOREXACT[count + 1]);
        breaker  = getBreakIterator(search->breaker);
        usearch_setBreakIterator(strsrch, breaker, &status);
        if (U_FAILURE(status) || usearch_getBreakIterator(strsrch) != breaker) {
            log_err("Error setting break iterator\n");
            usearch_close(strsrch);
            goto ENDTESTBREAKITERATOR;
        }
        usearch_reset(strsrch);
        usearch_setAttribute(strsrch, USEARCH_CANONICAL_MATCH, USEARCH_ON, 
                             &status);
        if (!assertEqualWithUStringSearch(strsrch, *search)) {
            log_err("Error at test number %d\n", count);
            usearch_close(strsrch);
            goto ENDTESTBREAKITERATOR;
        }
        usearch_close(strsrch);
        count += 2;
    }
    count = 0;
    while (BREAKITERATORCANONICAL[count].text != NULL) {
         if (!assertEqual(BREAKITERATORCANONICAL[count])) {
             log_err("Error at test number %d\n", count);
             goto ENDTESTBREAKITERATOR;
         }
         count ++;
    }
    
ENDTESTBREAKITERATOR:
    close();
#endif
}

static void TestVariableCanonical(void) 
{
    int count = 0;
    UErrorCode status = U_ZERO_ERROR;
    open(&status);
    if (U_FAILURE(status)) {
        log_err_status(status, "Unable to open static collators %s\n", u_errorName(status));
        return;
    }
    ucol_setAttribute(EN_US_, UCOL_ALTERNATE_HANDLING, UCOL_SHIFTED, &status);
    if (U_FAILURE(status)) {
        log_err("Error setting collation alternate attribute %s\n", 
            u_errorName(status));
    }
    while (VARIABLE[count].text != NULL) {
        log_verbose("variable %d\n", count);
        if (!assertCanonicalEqual(VARIABLE[count])) {
            log_err("Error at test number %d\n", count);
        }
        count ++;
    }
    ucol_setAttribute(EN_US_, UCOL_ALTERNATE_HANDLING, 
                      UCOL_NON_IGNORABLE, &status);
    close();
}

static void TestOverlapCanonical(void)
{
    int count = 0;
    UErrorCode status = U_ZERO_ERROR;
    open(&status);
    if (U_FAILURE(status)) {
        log_err_status(status, "Unable to open static collators %s\n", u_errorName(status));
        return;
    }
    while (OVERLAPCANONICAL[count].text != NULL) {
        if (!assertEqualWithAttribute(OVERLAPCANONICAL[count], USEARCH_ON, 
                                      USEARCH_ON)) {
            log_err("Error at overlap test number %d\n", count);
        }
        count ++;
    }    
    count = 0;
    while (NONOVERLAP[count].text != NULL) {
        if (!assertCanonicalEqual(NONOVERLAPCANONICAL[count])) {
            log_err("Error at non overlap test number %d\n", count);
        }
        count ++;
    }

    count = 0;
    while (count < 1) {
              UChar           pattern[32];
              UChar           text[128];
        const SearchData     *search   = &(OVERLAPCANONICAL[count]);     
              UCollator      *collator = getCollator(search->collator);
              UStringSearch  *strsrch; 
              status   = U_ZERO_ERROR;
    
        u_unescape(search->text, text, 128);
        u_unescape(search->pattern, pattern, 32);
        strsrch = usearch_openFromCollator(pattern, -1, text, -1, collator, 
                                           NULL, &status);
        if(status == U_FILE_ACCESS_ERROR) {
          log_data_err("Is your data around?\n");
          return;
        } else if(U_FAILURE(status)) {
          log_err("Error opening searcher\n");
          return;
        }
        usearch_setAttribute(strsrch, USEARCH_CANONICAL_MATCH, USEARCH_ON, 
                             &status);
        usearch_setAttribute(strsrch, USEARCH_OVERLAP, USEARCH_ON, &status);
        if (U_FAILURE(status) ||
            usearch_getAttribute(strsrch, USEARCH_OVERLAP) != USEARCH_ON) {
            log_err("Error setting overlap option\n");
        }
        if (!assertEqualWithUStringSearch(strsrch, *search)) {
            usearch_close(strsrch);
            return;
        }
        search   = &(NONOVERLAPCANONICAL[count]);
        usearch_setAttribute(strsrch, USEARCH_OVERLAP, USEARCH_OFF, &status);
        if (U_FAILURE(status) ||
            usearch_getAttribute(strsrch, USEARCH_OVERLAP) != USEARCH_OFF) {
            log_err("Error setting overlap option\n");
        }
        usearch_reset(strsrch);
        if (!assertEqualWithUStringSearch(strsrch, *search)) {
            usearch_close(strsrch);
            log_err("Error at test number %d\n", count);
         }

        count ++;
        usearch_close(strsrch);
    }
    close();
}

static void TestCollatorCanonical(void) 
{
    /* test collator that thinks "o" and "p" are the same thing */
          UChar          rules[32];
          UCollator     *tailored = NULL; 
          UErrorCode     status = U_ZERO_ERROR;
          UChar          pattern[32];
          UChar          text[128];
          UStringSearch *strsrch; 
          
    open(&status);
    if (U_FAILURE(status)) {
        log_err_status(status, "Unable to open static collators %s\n", u_errorName(status));
        return;
    }
    u_unescape(COLLATORCANONICAL[0].text, text, 128);
    u_unescape(COLLATORCANONICAL[0].pattern, pattern, 32);

    strsrch = usearch_openFromCollator(pattern, -1, text, -1, EN_US_, 
                                       NULL, &status);
    if(status == U_FILE_ACCESS_ERROR) {
      log_data_err("Is your data around?\n");
      return;
    } else if(U_FAILURE(status)) {
      log_err("Error opening searcher\n");
      return;
    }
    usearch_setAttribute(strsrch, USEARCH_CANONICAL_MATCH, USEARCH_ON, 
                         &status);
    if (U_FAILURE(status)) {
        log_err("Error opening string search %s\n", u_errorName(status));
    }
    if (!assertEqualWithUStringSearch(strsrch, COLLATORCANONICAL[0])) {
        goto ENDTESTCOLLATOR;
    }
    
    u_unescape(TESTCOLLATORRULE, rules, 32);
    tailored = ucol_openRules(rules, -1, UCOL_ON, 
                              COLLATORCANONICAL[1].strength, NULL, &status);
    if (U_FAILURE(status)) {
        log_err("Error opening rule based collator %s\n", u_errorName(status));
    }

    usearch_setCollator(strsrch, tailored, &status);
    if (U_FAILURE(status) || usearch_getCollator(strsrch) != tailored) {
        log_err("Error setting rule based collator\n");
    }
    usearch_reset(strsrch);
    usearch_setAttribute(strsrch, USEARCH_CANONICAL_MATCH, USEARCH_ON, 
                         &status);
    if (!assertEqualWithUStringSearch(strsrch, COLLATORCANONICAL[1])) {
        goto ENDTESTCOLLATOR;
    }
        
    usearch_setCollator(strsrch, EN_US_, &status);
    usearch_reset(strsrch);
    usearch_setAttribute(strsrch, USEARCH_CANONICAL_MATCH, USEARCH_ON, 
                         &status);
    if (U_FAILURE(status) || usearch_getCollator(strsrch) != EN_US_) {
        log_err("Error setting rule based collator\n");
    }
    if (!assertEqualWithUStringSearch(strsrch, COLLATORCANONICAL[0])) {
        goto ENDTESTCOLLATOR;
    }
    
ENDTESTCOLLATOR:
    usearch_close(strsrch);
    if (tailored != NULL) {
        ucol_close(tailored);
    }
    close();
}

static void TestPatternCanonical(void)
{
          UStringSearch *strsrch; 
          UChar          pattern[32];
          UChar          text[128];
    const UChar         *temp;
          int32_t        templength;
          UErrorCode     status = U_ZERO_ERROR;

    open(&status);
    if (U_FAILURE(status)) {
        log_err_status(status, "Unable to open static collators %s\n", u_errorName(status));
        return;
    }
    u_unescape(PATTERNCANONICAL[0].text, text, 128);
    u_unescape(PATTERNCANONICAL[0].pattern, pattern, 32);

    ucol_setStrength(EN_US_, PATTERNCANONICAL[0].strength);
    strsrch = usearch_openFromCollator(pattern, -1, text, -1, EN_US_, 
                                       NULL, &status);
    usearch_setAttribute(strsrch, USEARCH_CANONICAL_MATCH, USEARCH_ON, 
                         &status);
    if (U_FAILURE(status)) {
        log_err("Error opening string search %s\n", u_errorName(status));
        goto ENDTESTPATTERN;
    }
    temp = usearch_getPattern(strsrch, &templength);
    if (u_strcmp(pattern, temp) != 0) {
        log_err("Error setting pattern\n");
    }
    if (!assertEqualWithUStringSearch(strsrch, PATTERNCANONICAL[0])) {
        goto ENDTESTPATTERN;
    }

    u_unescape(PATTERNCANONICAL[1].pattern, pattern, 32);
    usearch_setPattern(strsrch, pattern, -1, &status);
    temp = usearch_getPattern(strsrch, &templength);
    if (u_strcmp(pattern, temp) != 0) {
        log_err("Error setting pattern\n");
        goto ENDTESTPATTERN;
    }
    usearch_reset(strsrch);
    usearch_setAttribute(strsrch, USEARCH_CANONICAL_MATCH, USEARCH_ON, 
                         &status);
    if (U_FAILURE(status)) {
        log_err("Error setting pattern %s\n", u_errorName(status));
    }
    if (!assertEqualWithUStringSearch(strsrch, PATTERNCANONICAL[1])) {
        goto ENDTESTPATTERN;
    }

    u_unescape(PATTERNCANONICAL[0].pattern, pattern, 32);
    usearch_setPattern(strsrch, pattern, -1, &status);
    temp = usearch_getPattern(strsrch, &templength);
    if (u_strcmp(pattern, temp) != 0) {
        log_err("Error setting pattern\n");
        goto ENDTESTPATTERN;
    }
    usearch_reset(strsrch);
    usearch_setAttribute(strsrch, USEARCH_CANONICAL_MATCH, USEARCH_ON, 
                         &status);
    if (U_FAILURE(status)) {
        log_err("Error setting pattern %s\n", u_errorName(status));
    }
    if (!assertEqualWithUStringSearch(strsrch, PATTERNCANONICAL[0])) {
        goto ENDTESTPATTERN;
    }
ENDTESTPATTERN:
    ucol_setStrength(EN_US_, UCOL_TERTIARY);
    if (strsrch != NULL) {
        usearch_close(strsrch);
    }
    close();
}

static void TestTextCanonical(void) 
{
          UStringSearch *strsrch; 
          UChar          pattern[32];
          UChar          text[128];
    const UChar         *temp;
          int32_t        templength;
          UErrorCode     status = U_ZERO_ERROR;

    u_unescape(TEXTCANONICAL[0].text, text, 128);
    u_unescape(TEXTCANONICAL[0].pattern, pattern, 32);

    open(&status);
    if (U_FAILURE(status)) {
        log_err_status(status, "Unable to open static collators %s\n", u_errorName(status));
        return;
    }
    strsrch = usearch_openFromCollator(pattern, -1, text, -1, EN_US_, 
                                       NULL, &status);
    usearch_setAttribute(strsrch, USEARCH_CANONICAL_MATCH, USEARCH_ON, 
                         &status);

    if (U_FAILURE(status)) {
        log_err("Error opening string search %s\n", u_errorName(status));
        goto ENDTESTPATTERN;
    }
    temp = usearch_getText(strsrch, &templength);
    if (u_strcmp(text, temp) != 0) {
        log_err("Error setting text\n");
    }
    if (!assertEqualWithUStringSearch(strsrch, TEXTCANONICAL[0])) {
        goto ENDTESTPATTERN;
    }

    u_unescape(TEXTCANONICAL[1].text, text, 32);
    usearch_setText(strsrch, text, -1, &status);
    temp = usearch_getText(strsrch, &templength);
    if (u_strcmp(text, temp) != 0) {
        log_err("Error setting text\n");
        goto ENDTESTPATTERN;
    }
    if (U_FAILURE(status)) {
        log_err("Error setting text %s\n", u_errorName(status));
    }
    if (!assertEqualWithUStringSearch(strsrch, TEXTCANONICAL[1])) {
        goto ENDTESTPATTERN;
    }

    u_unescape(TEXTCANONICAL[0].text, text, 32);
    usearch_setText(strsrch, text, -1, &status);
    temp = usearch_getText(strsrch, &templength);
    if (u_strcmp(text, temp) != 0) {
        log_err("Error setting text\n");
        goto ENDTESTPATTERN;
    }
    if (U_FAILURE(status)) {
        log_err("Error setting pattern %s\n", u_errorName(status));
    }
    if (!assertEqualWithUStringSearch(strsrch, TEXTCANONICAL[0])) {
        goto ENDTESTPATTERN;
    }
ENDTESTPATTERN:
    if (strsrch != NULL) {
        usearch_close(strsrch);
    }
    close();
}

static void TestCompositeBoundariesCanonical(void) 
{
    int count = 0;
    UErrorCode status = U_ZERO_ERROR;
    open(&status);
    if (U_FAILURE(status)) {
        log_err_status(status, "Unable to open static collators %s\n", u_errorName(status));
        return;
    }
    while (COMPOSITEBOUNDARIESCANONICAL[count].text != NULL) { 
        log_verbose("composite %d\n", count);
        if (!assertCanonicalEqual(COMPOSITEBOUNDARIESCANONICAL[count])) {
            log_err("Error at test number %d\n", count);
        }
        count ++;
    } 
    close();
}

static void TestGetSetOffsetCanonical(void)
{
    int            searchDataIndex   = 0;
    UChar          pattern[32];
    UChar          text[128];
    UErrorCode     status  = U_ZERO_ERROR;
    UStringSearch *strsrch;
    UCollator     *collator;

    memset(pattern, 0, 32*sizeof(UChar));
    memset(text, 0, 128*sizeof(UChar));

    open(&status);
    if (U_FAILURE(status)) {
        log_err_status(status, "Unable to open static collators %s\n", u_errorName(status));
        return;
    }
    strsrch = usearch_openFromCollator(pattern, 16, text, 32, EN_US_, NULL, 
                                       &status);

    collator = usearch_getCollator(strsrch);
    ucol_setAttribute(collator, UCOL_NORMALIZATION_MODE, UCOL_ON, &status);

    usearch_setAttribute(strsrch, USEARCH_CANONICAL_MATCH, USEARCH_ON, 
                         &status);

    /* testing out of bounds error */
    usearch_setOffset(strsrch, -1, &status);
    if (U_SUCCESS(status)) {
        log_err("Error expecting set offset error\n");
    }
    usearch_setOffset(strsrch, 128, &status);
    if (U_SUCCESS(status)) {
        log_err("Error expecting set offset error\n");
    }
    while (BASICCANONICAL[searchDataIndex].text != NULL) {
        int         count       = 0;
        SearchData  search      = BASICCANONICAL[searchDataIndex ++];
        int32_t matchindex  = search.offset[count];
        int32_t     textlength;

        if (BASICCANONICAL[searchDataIndex].text == NULL) {
            /* skip the last one */
            break;
        }
        
        u_unescape(search.text, text, 128);
        u_unescape(search.pattern, pattern, 32);
        status = U_ZERO_ERROR;
        usearch_setText(strsrch, text, -1, &status);
        usearch_setPattern(strsrch, pattern, -1, &status);
        while (U_SUCCESS(status) && matchindex >= 0) {
            uint32_t matchlength = search.size[count];
            usearch_next(strsrch, &status);
            if (matchindex != usearch_getMatchedStart(strsrch) || 
                matchlength != (uint32_t)usearch_getMatchedLength(strsrch)) {
                char *str = toCharString(usearch_getText(strsrch, 
                                                         &textlength));
                log_err("Text: %s\n", str);
                str = toCharString(usearch_getPattern(strsrch, &textlength));
                log_err("Pattern: %s\n", str);
                log_err("Error match found at %d %d\n", 
                        usearch_getMatchedStart(strsrch), 
                        usearch_getMatchedLength(strsrch));
                goto bail;
            }
            matchindex = search.offset[count + 1] == -1 ? -1 : 
                         search.offset[count + 2];
            if (search.offset[count + 1] != -1) {
                usearch_setOffset(strsrch, search.offset[count + 1] + 1, 
                                  &status);
                if (usearch_getOffset(strsrch) != search.offset[count + 1] + 1) {
                    log_err("Error setting offset\n");
                    goto bail;
                }
            }
            
            count += 2;
        }
        usearch_next(strsrch, &status);
        if (usearch_getMatchedStart(strsrch) != USEARCH_DONE) {
            char *str = toCharString(usearch_getText(strsrch, &textlength));
            log_err("Text: %s\n", str);
            str = toCharString(usearch_getPattern(strsrch, &textlength));
            log_err("Pattern: %s\n", str);
            log_err("Error match found at %d %d\n", 
                        usearch_getMatchedStart(strsrch), 
                        usearch_getMatchedLength(strsrch));
            goto bail;
        }
    }

bail:
    ucol_setAttribute(collator, UCOL_NORMALIZATION_MODE, UCOL_OFF, &status);
    usearch_close(strsrch);
    close();
}

static void TestSupplementaryCanonical(void)
{
    int count = 0;
    UErrorCode status = U_ZERO_ERROR;
    open(&status);
    if (U_FAILURE(status)) {
        log_err_status(status, "Unable to open static collators %s\n", u_errorName(status));
        return;
    }
    while (SUPPLEMENTARYCANONICAL[count].text != NULL) {
        if (!assertCanonicalEqual(SUPPLEMENTARYCANONICAL[count])) {
            log_err("Error at test number %d\n", count);
        }
        count ++;
    }
    close();
}

static void TestContractionCanonical(void) 
{
    UChar          rules[128];
    UChar          pattern[128];
    UChar          text[128];
    UCollator     *collator = NULL;
    UErrorCode     status = U_ZERO_ERROR;
    int            count = 0;
    UStringSearch *strsrch = NULL;
    memset(rules, 0, 128*sizeof(UChar));
    memset(pattern, 0, 128*sizeof(UChar));
    memset(text, 0, 128*sizeof(UChar));

    u_unescape(CONTRACTIONRULE, rules, 128);
    collator = ucol_openRules(rules, u_strlen(rules), UCOL_ON,
                              UCOL_TERTIARY, NULL, &status); 
    if(status == U_FILE_ACCESS_ERROR) {
      log_data_err("Is your data around?\n");
      return;
    } else if(U_FAILURE(status)) {
      log_err("Error opening collator %s\n", u_errorName(status));
      return;
    }
    strsrch = usearch_openFromCollator(pattern, 1, text, 1, collator, NULL, 
                                       &status);
    usearch_setAttribute(strsrch, USEARCH_CANONICAL_MATCH, USEARCH_ON, 
                         &status);
    if (U_FAILURE(status)) {
        log_err("Error opening string search %s\n", u_errorName(status));
    }   
        
    while (CONTRACTIONCANONICAL[count].text != NULL) {
        u_unescape(CONTRACTIONCANONICAL[count].text, text, 128);
        u_unescape(CONTRACTIONCANONICAL[count].pattern, pattern, 128);
        usearch_setText(strsrch, text, -1, &status);
        usearch_setPattern(strsrch, pattern, -1, &status);
        if (!assertEqualWithUStringSearch(strsrch, 
                                              CONTRACTIONCANONICAL[count])) {
            log_err("Error at test number %d\n", count);
        }
        count ++;
    }
    usearch_close(strsrch);
    ucol_close(collator);
}

static void TestNumeric(void) {
    UCollator     *coll = NULL;
    UStringSearch *strsrch = NULL;
    UErrorCode     status = U_ZERO_ERROR;

    UChar          pattern[128];
    UChar          text[128];
    memset(pattern, 0, 128*sizeof(UChar));
    memset(text, 0, 128*sizeof(UChar));

    coll = ucol_open("", &status);
    if(U_FAILURE(status)) {
        log_data_err("Could not open UCA. Is your data around?\n");
        return;
    }

    ucol_setAttribute(coll, UCOL_NUMERIC_COLLATION, UCOL_ON, &status);

    strsrch = usearch_openFromCollator(pattern, 1, text, 1, coll, NULL, &status);

    if(status != U_UNSUPPORTED_ERROR || U_SUCCESS(status)) {
        log_err("Expected U_UNSUPPORTED_ERROR when trying to instantiate a search object from a CODAN collator, got %s instead\n", u_errorName(status));
        if(strsrch) {
            usearch_close(strsrch);
        }
    }

    ucol_close(coll);

}

/* This test is for ticket 4038 due to incorrect backward searching when certain patterns have a length > 1 */
static void TestForwardBackward(void) {
    UErrorCode status = U_ZERO_ERROR;
    UCollator *coll = NULL;
    UStringSearch *search = NULL;
    UChar usrcstr[32], value[4];
    int32_t pos= -1;
    int32_t expectedPos = 9;
    
    coll = ucol_open("en_GB", &status);
    if (U_FAILURE(status)) {
        log_err_status(status, "ucol_open failed: %s\n", u_errorName(status));
        goto exitTestForwardBackward;
    }
    ucol_setAttribute(coll, UCOL_STRENGTH, UCOL_PRIMARY, &status);
    ucol_setAttribute(coll, UCOL_CASE_LEVEL, UCOL_ON, &status);
    ucol_setAttribute(coll, UCOL_ALTERNATE_HANDLING, UCOL_NON_IGNORABLE, &status);
    
    u_uastrcpy(usrcstr, "QBitArray::bitarr_data"); /* text */
    u_uastrcpy(value, "::");                       /* pattern */
    
    search = usearch_openFromCollator(value, 2, usrcstr, 22, coll, NULL, &status);
    if (U_FAILURE(status)) {
        log_err("usearch_openFromCollator failed: %s\n", u_errorName(status));
        goto exitTestForwardBackward;
    }

    usearch_reset(search);
    /* forward search */
    pos = usearch_first(search, &status);
    if (pos != expectedPos) {
        log_err("Expected search result: %d; Got instead: %d\n", expectedPos, pos);
        goto exitTestForwardBackward;
    }
    
    pos = -1;
    usearch_reset(search);
    /* backward search */
    pos = usearch_last(search, &status);
    if (pos != expectedPos) {
        log_err("Expected search result: %d; Got instead: %d\n", expectedPos, pos);
    }

exitTestForwardBackward :
    if (coll != NULL) {
        ucol_close(coll);
    }
    if (search != NULL) {
        usearch_close(search);
    }
}

#define TEST_ASSERT(x) \
   {if (U_FAILURE(x)) {log_err_status(x, "%s:%d: FAIL: test assertion failure \n", __FILE__, __LINE__);\
   }}

static void TestSearchForNull(void) {
    UCollator *coll;
    UErrorCode ec;
    UStringSearch *search;
    int pos;
    int len;
    int expectedPos;
    int expectedLen;
    int expectedNum;
    int count = 0;
    const UChar zerodigit = 0x0030; /* 0 */
    const UChar nulldigit = 0x0000; /* null */

    /* static const UChar var[(length)+1]=U_DECLARE_UTF16(cs) */
#define PATTERN_LEN 4
#define TEXT_LEN 10

    U_STRING_DECL(_pattern, "IS 0", PATTERN_LEN);
    U_STRING_DECL(_text, "_0IS 0 OK?", TEXT_LEN);
    UChar pattern[PATTERN_LEN + 1], text[TEXT_LEN + 1];

    U_STRING_INIT(_pattern, "IS 0", PATTERN_LEN);
    U_STRING_INIT(_text, "_0IS 0 OK?", TEXT_LEN);
    expectedPos = 2;
    expectedLen = 4;
    expectedNum = 1;

    for (pos = 0; pos < PATTERN_LEN; pos++) {
        if (_pattern[pos] == zerodigit) {
            pattern[pos] = nulldigit;
        } else {
            pattern[pos] = _pattern[pos];
        }
    }
    pattern[PATTERN_LEN] = 0x0000;

    for (pos = 0; pos < TEXT_LEN; pos++) {
        if (_text[pos] == zerodigit) {
            text[pos] = nulldigit;
        } else {
            text[pos] = _text[pos];
        }
    }
    text[TEXT_LEN] = 0x0000;

    ec = U_ZERO_ERROR;

    /* create a US-English collator */
    coll = ucol_open("en_US", &ec);

    /* make sure we didn't fail. */
     TEST_ASSERT (ec);

    ucol_setStrength(coll, UCOL_IDENTICAL);

    /* open a search looking for 0 */
    search = usearch_openFromCollator(pattern, PATTERN_LEN, text,
            TEXT_LEN, coll, NULL, &ec);
     TEST_ASSERT (ec);
    
    if (coll != NULL && search != NULL) {
        pos = usearch_first(search, &ec);
        len = usearch_getMatchedLength(search);
        if (pos != expectedPos) {
            log_err("Expected search result: %d; Got instead: %d\n", expectedPos,
                    pos);
        }

        if (len != expectedLen) {
            log_err("Expected search result length: %d; Got instead: %d\n",
                    expectedLen, len);
        }

        for (pos = usearch_first(search, &ec); pos != USEARCH_DONE; pos
                = usearch_next(search, &ec)) {
            log_verbose("Match at %d\n", pos);
            count += 1;
        }

        if (count != expectedNum) {
            log_err("Expected %d search hits, found %d\n", expectedNum, count);
        }
    }

    ucol_close(coll);
    usearch_close(search);
}

static void TestStrengthIdentical(void)
{
	UCollator *coll;
	UErrorCode ec = U_ZERO_ERROR;
	UStringSearch *search;
	
    UChar pattern[] = {0x05E9, 0x0591, 0x05E9};
    UChar text[]    = {0x05E9, 0x0592, 0x05E9};
    int32_t pLen = sizeof (pattern) / sizeof(pattern[0]);
    int32_t tLen = sizeof(text) / sizeof (text[0]);
	int32_t expectedPos = 0;
	int32_t expectedLen = 3;

	int32_t pos;
	int32_t len;

    /* create a US-English collator */
	coll = ucol_open ("en_US", &ec);

	/* make sure we didn't fail. */
	TEST_ASSERT (ec);

    ucol_setStrength( coll, UCOL_TERTIARY); 

	/* open a search looking for 0 */
	search = usearch_openFromCollator (pattern, pLen, text, tLen, coll, NULL, &ec);
	TEST_ASSERT (ec);

    if (coll != NULL && search != NULL) {
	    pos = usearch_first(search, &ec);
	    len = usearch_getMatchedLength(search);

	    if(pos != expectedPos) {
		    log_err("Expected search result: %d; Got instead: %d\n", expectedPos, pos);
	    }
		
	    if(len != expectedLen) {
		    log_err("Expected search result length: %d; Got instead: %d\n", expectedLen, len);
	    }
	
        /* Now try it at strength == UCOL_IDENTICAL */
        ucol_setStrength(coll, UCOL_IDENTICAL); 
	    usearch_reset(search);

	    pos = usearch_first(search, &ec);
	    len = usearch_getMatchedLength(search);

	    if(pos != -1) {
		    log_err("Expected failure for strentgh = UCOL_IDENTICAL: got %d instead.\n", pos);
	    }
    }

    usearch_close(search);
    ucol_close(coll);
}

/**
* TestUsingSearchCollator
*/

#define ARRAY_LENGTH(array) (sizeof(array)/sizeof(array[0]))

typedef struct {
    const UChar *   pattern;
    const int32_t * offsets;
    int32_t         offsetsLen;
} PatternAndOffsets;

static const UChar scKoText[] = {
       0x0020,
/*01*/ 0xAC00, 0x0020,                         /* simple LV Hangul */
/*03*/ 0xAC01, 0x0020,                         /* simple LVT Hangul */
/*05*/ 0xAC0F, 0x0020,                         /* LVTT, last jamo expands for search */
/*07*/ 0xAFFF, 0x0020,                         /* LLVVVTT, every jamo expands for search */
/*09*/ 0x1100, 0x1161, 0x11A8, 0x0020,         /* 0xAC01 as conjoining jamo */
/*13*/ 0x1100, 0x1161, 0x1100, 0x0020,         /* 0xAC01 as basic conjoining jamo (per search rules) */
/*17*/ 0x3131, 0x314F, 0x3131, 0x0020,         /* 0xAC01 as compatibility jamo */
/*21*/ 0x1100, 0x1161, 0x11B6, 0x0020,         /* 0xAC0F as conjoining jamo; last expands for search */
/*25*/ 0x1100, 0x1161, 0x1105, 0x1112, 0x0020, /* 0xAC0F as basic conjoining jamo; last expands for search */
/*30*/ 0x1101, 0x1170, 0x11B6, 0x0020,         /* 0xAFFF as conjoining jamo; all expand for search */
/*34*/ 0x00E6, 0x0020,                         /* small letter ae, expands */
/*36*/ 0x1E4D, 0x0020,                         /* small letter o with tilde and acute, decomposes */
       0
};

static const UChar scKoPat0[] = { 0xAC01, 0 };
static const UChar scKoPat1[] = { 0x1100, 0x1161, 0x11A8, 0 }; /* 0xAC01 as conjoining jamo */
static const UChar scKoPat2[] = { 0xAC0F, 0 };
static const UChar scKoPat3[] = { 0x1100, 0x1161, 0x1105, 0x1112, 0 }; /* 0xAC0F as basic conjoining jamo */
static const UChar scKoPat4[] = { 0xAFFF, 0 };
static const UChar scKoPat5[] = { 0x1101, 0x1170, 0x11B6, 0 }; /* 0xAFFF as conjoining jamo */

static const int32_t scKoSrchOff01[] = { 3,  9, 13 };
static const int32_t scKoSrchOff23[] = { 5, 21, 25 };
static const int32_t scKoSrchOff45[] = { 7, 30     };

static const PatternAndOffsets scKoSrchPatternsOffsets[] = {
    { scKoPat0, scKoSrchOff01, ARRAY_LENGTH(scKoSrchOff01) },
    { scKoPat1, scKoSrchOff01, ARRAY_LENGTH(scKoSrchOff01) },
    { scKoPat2, scKoSrchOff23, ARRAY_LENGTH(scKoSrchOff23) },
    { scKoPat3, scKoSrchOff23, ARRAY_LENGTH(scKoSrchOff23) },
    { scKoPat4, scKoSrchOff45, ARRAY_LENGTH(scKoSrchOff45) },
    { scKoPat5, scKoSrchOff45, ARRAY_LENGTH(scKoSrchOff45) },
    { NULL,     NULL,          0                           }
};

static const int32_t scKoStndOff01[] = { 3,  9 };
static const int32_t scKoStndOff2[]  = { 5, 21 };
static const int32_t scKoStndOff3[]  = { 25    };
static const int32_t scKoStndOff45[] = { 7, 30 };

static const PatternAndOffsets scKoStndPatternsOffsets[] = {
    { scKoPat0, scKoStndOff01, ARRAY_LENGTH(scKoStndOff01) },
    { scKoPat1, scKoStndOff01, ARRAY_LENGTH(scKoStndOff01) },
    { scKoPat2, scKoStndOff2,  ARRAY_LENGTH(scKoStndOff2)  },
    { scKoPat3, scKoStndOff3,  ARRAY_LENGTH(scKoStndOff3)  },
    { scKoPat4, scKoStndOff45, ARRAY_LENGTH(scKoStndOff45) },
    { scKoPat5, scKoStndOff45, ARRAY_LENGTH(scKoStndOff45) },
    { NULL,     NULL,          0                           }
};

typedef struct {
    const char *  locale;
    const UChar * text;
    const PatternAndOffsets * patternsAndOffsets;
} TUSCItem;

static const TUSCItem tuscItems[] = {
    { "root",                  scKoText, scKoStndPatternsOffsets },
    { "root@collation=search", scKoText, scKoSrchPatternsOffsets },
    { "ko@collation=search",   scKoText, scKoSrchPatternsOffsets },
    { NULL,                    NULL,     NULL                    }
};

static const UChar dummyPat[] = { 0x0061, 0 };

static void TestUsingSearchCollator(void)
{
    const TUSCItem * tuscItemPtr;
    for (tuscItemPtr = tuscItems; tuscItemPtr->locale != NULL; tuscItemPtr++) {
        UErrorCode status = U_ZERO_ERROR;
        UCollator* ucol = ucol_open(tuscItemPtr->locale, &status);
        if ( U_SUCCESS(status) ) {
            UStringSearch* usrch = usearch_openFromCollator(dummyPat, -1, tuscItemPtr->text, -1, ucol, NULL, &status);
            if ( U_SUCCESS(status) ) {
                const PatternAndOffsets * patternsOffsetsPtr;
                for ( patternsOffsetsPtr = tuscItemPtr->patternsAndOffsets; patternsOffsetsPtr->pattern != NULL; patternsOffsetsPtr++) {
                    usearch_setPattern(usrch, patternsOffsetsPtr->pattern, -1, &status);
                    if ( U_SUCCESS(status) ) {
                        int32_t offset;
                        const int32_t * nextOffsetPtr;
                        const int32_t * limitOffsetPtr;

                        usearch_reset(usrch);
                        nextOffsetPtr = patternsOffsetsPtr->offsets;
                        limitOffsetPtr = patternsOffsetsPtr->offsets + patternsOffsetsPtr->offsetsLen;
                        while (TRUE) {
                            offset = usearch_next(usrch, &status);
                            if ( U_FAILURE(status) || offset == USEARCH_DONE ) {
                                break;
                            }
                            if ( nextOffsetPtr < limitOffsetPtr ) {
                                 if (offset != *nextOffsetPtr) {
                                     log_err("error, locale %s, expected usearch_next %d, got %d\n", tuscItemPtr->locale, *nextOffsetPtr, offset);
                                     nextOffsetPtr = limitOffsetPtr;
                                     break;
                                 }
                                 nextOffsetPtr++;
                            } else {
                                log_err("error, locale %s, usearch_next returned more matches than expected\n", tuscItemPtr->locale );
                            }
                        }
                        if ( U_FAILURE(status) ) {
                            log_err("error, locale %s, usearch_next failed: %s\n", tuscItemPtr->locale, u_errorName(status) );
                        } else if ( nextOffsetPtr < limitOffsetPtr ) {
                            log_err("error, locale %s, usearch_next returned fewer matches than expected\n", tuscItemPtr->locale );
                        }

                        status = U_ZERO_ERROR;
                        usearch_reset(usrch);
                        nextOffsetPtr = patternsOffsetsPtr->offsets + patternsOffsetsPtr->offsetsLen;
                        limitOffsetPtr = patternsOffsetsPtr->offsets;
                        while (TRUE) {
                            offset = usearch_previous(usrch, &status);
                            if ( U_FAILURE(status) || offset == USEARCH_DONE ) {
                                break;
                            }
                            if ( nextOffsetPtr > limitOffsetPtr ) {
                                nextOffsetPtr--;
                                if (offset != *nextOffsetPtr) {
                                     log_err("error, locale %s, expected usearch_previous %d, got %d\n", tuscItemPtr->locale, *nextOffsetPtr, offset);
                                     nextOffsetPtr = limitOffsetPtr;
                                      break;
                                }
                            } else {
                                log_err("error, locale %s, usearch_previous returned more matches than expected\n", tuscItemPtr->locale );
                            }
                        }
                        if ( U_FAILURE(status) ) {
                            log_err("error, locale %s, usearch_previous failed: %s\n", tuscItemPtr->locale, u_errorName(status) );
                        } else if ( nextOffsetPtr > limitOffsetPtr ) {
                            log_err("error, locale %s, usearch_previous returned fewer matches than expected\n", tuscItemPtr->locale );
                        }

                    } else {
                        log_err("error, locale %s, usearch_setPattern failed: %s\n", tuscItemPtr->locale, u_errorName(status) );
                    }
                }
                usearch_close(usrch);
            } else {
                log_err("error, locale %s, usearch_openFromCollator failed: %s\n", tuscItemPtr->locale, u_errorName(status) );
            }
            ucol_close(ucol);
        } else {
            log_err("error, locale %s, ucol_open failed: %s\n", tuscItemPtr->locale, u_errorName(status) );
        }
    }
}

/**
* addSearchTest
*/

void addSearchTest(TestNode** root)
{
    addTest(root, &TestStart, "tscoll/usrchtst/TestStart");
    addTest(root, &TestOpenClose, "tscoll/usrchtst/TestOpenClose");
    addTest(root, &TestInitialization, "tscoll/usrchtst/TestInitialization");
    addTest(root, &TestBasic, "tscoll/usrchtst/TestBasic");
    addTest(root, &TestNormExact, "tscoll/usrchtst/TestNormExact");
    addTest(root, &TestStrength, "tscoll/usrchtst/TestStrength");
    addTest(root, &TestBreakIterator, "tscoll/usrchtst/TestBreakIterator");
    addTest(root, &TestVariable, "tscoll/usrchtst/TestVariable");
    addTest(root, &TestOverlap, "tscoll/usrchtst/TestOverlap");
    addTest(root, &TestCollator, "tscoll/usrchtst/TestCollator");
    addTest(root, &TestPattern, "tscoll/usrchtst/TestPattern");
    addTest(root, &TestText, "tscoll/usrchtst/TestText"); 
    addTest(root, &TestCompositeBoundaries, 
                                  "tscoll/usrchtst/TestCompositeBoundaries");
    addTest(root, &TestGetSetOffset, "tscoll/usrchtst/TestGetSetOffset");
    addTest(root, &TestGetSetAttribute, 
                                      "tscoll/usrchtst/TestGetSetAttribute");
    addTest(root, &TestGetMatch, "tscoll/usrchtst/TestGetMatch");
    addTest(root, &TestSetMatch, "tscoll/usrchtst/TestSetMatch");
    addTest(root, &TestReset, "tscoll/usrchtst/TestReset");
    addTest(root, &TestSupplementary, "tscoll/usrchtst/TestSupplementary");
    addTest(root, &TestContraction, "tscoll/usrchtst/TestContraction");
    addTest(root, &TestIgnorable, "tscoll/usrchtst/TestIgnorable");
    addTest(root, &TestCanonical, "tscoll/usrchtst/TestCanonical");
    addTest(root, &TestNormCanonical, "tscoll/usrchtst/TestNormCanonical");
    addTest(root, &TestStrengthCanonical, 
                                    "tscoll/usrchtst/TestStrengthCanonical");
    addTest(root, &TestBreakIteratorCanonical, 
                               "tscoll/usrchtst/TestBreakIteratorCanonical");
    addTest(root, &TestVariableCanonical, 
                                    "tscoll/usrchtst/TestVariableCanonical");
    addTest(root, &TestOverlapCanonical, 
                                     "tscoll/usrchtst/TestOverlapCanonical");
    addTest(root, &TestCollatorCanonical, 
                                    "tscoll/usrchtst/TestCollatorCanonical");
    addTest(root, &TestPatternCanonical, 
                                     "tscoll/usrchtst/TestPatternCanonical");
    addTest(root, &TestTextCanonical, "tscoll/usrchtst/TestTextCanonical"); 
    addTest(root, &TestCompositeBoundariesCanonical, 
                         "tscoll/usrchtst/TestCompositeBoundariesCanonical");
    addTest(root, &TestGetSetOffsetCanonical, 
                                "tscoll/usrchtst/TestGetSetOffsetCanonical");
    addTest(root, &TestSupplementaryCanonical, 
                               "tscoll/usrchtst/TestSupplementaryCanonical");
    addTest(root, &TestContractionCanonical, 
                                 "tscoll/usrchtst/TestContractionCanonical");
    addTest(root, &TestEnd, "tscoll/usrchtst/TestEnd");
    addTest(root, &TestNumeric, "tscoll/usrchtst/TestNumeric");
    addTest(root, &TestDiacriticMatch, "tscoll/usrchtst/TestDiacriticMatch");
    addTest(root, &TestForwardBackward, "tscoll/usrchtst/TestForwardBackward");
	addTest(root, &TestSearchForNull, "tscoll/usrchtst/TestSearchForNull");
    addTest(root, &TestStrengthIdentical, "tscoll/usrchtst/TestStrengthIdentical");
    addTest(root, &TestUsingSearchCollator, "tscoll/usrchtst/TestUsingSearchCollator");
}

#endif /* #if !UCONFIG_NO_COLLATION */
