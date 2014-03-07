/********************************************************************
 * COPYRIGHT: 
 * Copyright (c) 1997-2010, International Business Machines Corporation and
 * others. All Rights Reserved.
 ********************************************************************/
/********************************************************************************
*
* File CBIAPTS.C
*
* Modification History:
*        Name                     Description            
*     Madhu Katragadda              Creation
*********************************************************************************/
/*C API TEST FOR BREAKITERATOR */
/**
* This is an API test.  It doesn't test very many cases, and doesn't
* try to test the full functionality.  It just calls each function in the class and
* verifies that it works on a basic level.
**/

#include "unicode/utypes.h"

#if !UCONFIG_NO_BREAK_ITERATION

#include <stdlib.h>
#include <string.h>
#include "unicode/uloc.h"
#include "unicode/ubrk.h"
#include "unicode/ustring.h"
#include "unicode/ucnv.h"
#include "unicode/utext.h"
#include "cintltst.h"
#include "cbiapts.h"

#define TEST_ASSERT_SUCCESS(status) {if (U_FAILURE(status)) { \
log_data_err("Failure at file %s, line %d, error = %s (Are you missing data?)\n", __FILE__, __LINE__, u_errorName(status));}}

#define TEST_ASSERT(expr) {if ((expr)==FALSE) { \
log_data_err("Test Failure at file %s, line %d (Are you missing data?)\n", __FILE__, __LINE__);}}

static void TestBreakIteratorSafeClone(void);
static void TestBreakIteratorRules(void);
static void TestBreakIteratorRuleError(void);
static void TestBreakIteratorStatusVec(void);
static void TestBreakIteratorUText(void);
static void TestBreakIteratorTailoring(void);

void addBrkIterAPITest(TestNode** root);

void addBrkIterAPITest(TestNode** root)
{
#if !UCONFIG_NO_FILE_IO
    addTest(root, &TestBreakIteratorCAPI, "tstxtbd/cbiapts/TestBreakIteratorCAPI");
    addTest(root, &TestBreakIteratorSafeClone, "tstxtbd/cbiapts/TestBreakIteratorSafeClone");
    addTest(root, &TestBreakIteratorUText, "tstxtbd/cbiapts/TestBreakIteratorUText");
#endif
    addTest(root, &TestBreakIteratorRules, "tstxtbd/cbiapts/TestBreakIteratorRules");
    addTest(root, &TestBreakIteratorRuleError, "tstxtbd/cbiapts/TestBreakIteratorRuleError");
    addTest(root, &TestBreakIteratorStatusVec, "tstxtbd/cbiapts/TestBreakIteratorStatusVec");
    addTest(root, &TestBreakIteratorTailoring, "tstxtbd/cbiapts/TestBreakIteratorTailoring");
}

#define CLONETEST_ITERATOR_COUNT 2

/*
 *   Utility function for converting char * to UChar * strings, to
 *     simplify the test code.   Converted strings are put in heap allocated
 *     storage.   A hook (probably a local in the caller's code) allows all
 *     strings converted with that hook to be freed with a single call.
 */
typedef struct StringStruct {
        struct StringStruct   *link;
        UChar                 str[1];
    } StringStruct;


static UChar* toUChar(const char *src, void **freeHook) {
    /* Structure of the memory that we allocate on the heap */

    int32_t    numUChars;
    int32_t    destSize;
    UChar      stackBuf[2000 + sizeof(void *)/sizeof(UChar)];
    StringStruct  *dest;
    UConverter *cnv;

    UErrorCode status = U_ZERO_ERROR;
    if (src == NULL) {
        return NULL;
    };

    cnv = ucnv_open(NULL, &status);
    if(U_FAILURE(status) || cnv == NULL) {
        return NULL;
    }
    ucnv_reset(cnv);
    numUChars = ucnv_toUChars(cnv,
                  stackBuf,
                  2000,
                  src, -1,
                  &status);

    destSize = (numUChars+1) * sizeof(UChar) + sizeof(struct StringStruct);
    dest = (StringStruct *)malloc(destSize);
    if (dest != NULL) {
        if (status == U_BUFFER_OVERFLOW_ERROR || status == U_STRING_NOT_TERMINATED_WARNING) {
            ucnv_toUChars(cnv, dest->str, numUChars+1, src, -1, &status);
        } else if (status == U_ZERO_ERROR) {
            u_strcpy(dest->str, stackBuf);
        } else {
            free(dest);
            dest = NULL;
        }
    }

    ucnv_reset(cnv); /* be good citizens */
    ucnv_close(cnv);
    if (dest == NULL) {
        return NULL;
    }
    
    dest->link = (StringStruct*)(*freeHook);
    *freeHook = dest;
    return dest->str;
}

static void freeToUCharStrings(void **hook) {
    StringStruct  *s = *(StringStruct **)hook;
    while (s != NULL) {
        StringStruct *next = s->link;
        free(s);
        s = next;
    }
}


static void TestBreakIteratorCAPI()
{
    UErrorCode status = U_ZERO_ERROR;
    UBreakIterator *word, *sentence, *line, *character, *b, *bogus;
    int32_t start,pos,end,to;
    int32_t i;
    int32_t count = 0;

    UChar text[50];

    /* Note:  the adjacent "" are concatenating strings, not adding a \" to the
       string, which is probably what whoever wrote this intended.  Don't fix,
       because it would throw off the hard coded break positions in the following
       tests. */
    u_uastrcpy(text, "He's from Africa. ""Mr. Livingston, I presume?"" Yeah");


/*test ubrk_open()*/
    log_verbose("\nTesting BreakIterator open functions\n");
                                            
    /* Use french for fun */
    word         = ubrk_open(UBRK_WORD, "en_US", text, u_strlen(text), &status);
    if(status == U_FILE_ACCESS_ERROR) {
        log_data_err("Check your data - it doesn't seem to be around\n");
        return;
    } else if(U_FAILURE(status)){
        log_err_status(status, "FAIL: Error in ubrk_open() for word breakiterator: %s\n", myErrorName(status));
    }
    else{
        log_verbose("PASS: Successfully opened  word breakiterator\n");
    }
    
    sentence     = ubrk_open(UBRK_SENTENCE, "en_US", text, u_strlen(text), &status);
    if(U_FAILURE(status)){
        log_err_status(status, "FAIL: Error in ubrk_open() for sentence breakiterator: %s\n", myErrorName(status));
        return;
    }
    else{
        log_verbose("PASS: Successfully opened  sentence breakiterator\n");
    }
    
    line         = ubrk_open(UBRK_LINE, "en_US", text, u_strlen(text), &status);
    if(U_FAILURE(status)){
        log_err("FAIL: Error in ubrk_open() for line breakiterator: %s\n", myErrorName(status));
        return;
    }
    else{
        log_verbose("PASS: Successfully opened  line breakiterator\n");
    }
    
    character     = ubrk_open(UBRK_CHARACTER, "en_US", text, u_strlen(text), &status);
    if(U_FAILURE(status)){
        log_err("FAIL: Error in ubrk_open() for character breakiterator: %s\n", myErrorName(status));
        return;
    }
    else{
        log_verbose("PASS: Successfully opened  character breakiterator\n");
    }
    /*trying to open an illegal iterator*/
    bogus     = ubrk_open((UBreakIteratorType)5, "en_US", text, u_strlen(text), &status);
    if(U_SUCCESS(status)){
        log_err("FAIL: Error in ubrk_open() for BOGUS breakiterator. Expected U_ILLEGAL_ARGUMENT_ERROR\n");
    }
    if(U_FAILURE(status)){
        if(status != U_ILLEGAL_ARGUMENT_ERROR){
            log_err("FAIL: Error in ubrk_open() for BOGUS breakiterator. Expected U_ILLEGAL_ARGUMENT_ERROR\n Got %s\n", myErrorName(status));
        }
    }
    status=U_ZERO_ERROR;


/* ======= Test ubrk_countAvialable() and ubrk_getAvialable() */

    log_verbose("\nTesting ubrk_countAvailable() and ubrk_getAvailable()\n");
    count=ubrk_countAvailable();
    /* use something sensible w/o hardcoding the count */
    if(count < 0){
        log_err("FAIL: Error in ubrk_countAvialable() returned %d\n", count);
    }
    else{
        log_verbose("PASS: ubrk_countAvialable() successful returned %d\n", count);
    }
    for(i=0;i<count;i++)
    {
        log_verbose("%s\n", ubrk_getAvailable(i)); 
        if (ubrk_getAvailable(i) == 0)
            log_err("No locale for which breakiterator is applicable\n");
        else 
            log_verbose("A locale %s for which breakiterator is applicable\n",ubrk_getAvailable(i));
    }

/*========Test ubrk_first(), ubrk_last()...... and other functions*/

    log_verbose("\nTesting the functions for word\n");
    start = ubrk_first(word);
    if(start!=0)
        log_err("error ubrk_start(word) did not return 0\n");
    log_verbose("first (word = %d\n", (int32_t)start);
       pos=ubrk_next(word);
    if(pos!=4)
        log_err("error ubrk_next(word) did not return 4\n");
    log_verbose("next (word = %d\n", (int32_t)pos);
    pos=ubrk_following(word, 4);
    if(pos!=5)
        log_err("error ubrl_following(word,4) did not return 6\n");
    log_verbose("next (word = %d\n", (int32_t)pos);
    end=ubrk_last(word);
    if(end!=49)
        log_err("error ubrk_last(word) did not return 49\n");
    log_verbose("last (word = %d\n", (int32_t)end);
    
    pos=ubrk_previous(word);
    log_verbose("%d   %d\n", end, pos);
     
    pos=ubrk_previous(word);
    log_verbose("%d \n", pos);

    if (ubrk_isBoundary(word, 2) != FALSE) {
        log_err("error ubrk_isBoundary(word, 2) did not return FALSE\n");
    }
    pos=ubrk_current(word);
    if (pos != 4) {
        log_err("error ubrk_current() != 4 after ubrk_isBoundary(word, 2)\n");
    }
    if (ubrk_isBoundary(word, 4) != TRUE) {
        log_err("error ubrk_isBoundary(word, 4) did not return TRUE\n");
    }


    
    log_verbose("\nTesting the functions for character\n");
    ubrk_first(character);
    pos = ubrk_following(character, 5);
    if(pos!=6)
       log_err("error ubrk_following(character,5) did not return 6\n");
    log_verbose("Following (character,5) = %d\n", (int32_t)pos);
    pos=ubrk_following(character, 18);
    if(pos!=19)
       log_err("error ubrk_following(character,18) did not return 19\n");
    log_verbose("Followingcharacter,18) = %d\n", (int32_t)pos);
    pos=ubrk_preceding(character, 22);
    if(pos!=21)
       log_err("error ubrk_preceding(character,22) did not return 21\n");
    log_verbose("preceding(character,22) = %d\n", (int32_t)pos);
    

    log_verbose("\nTesting the functions for line\n");
    pos=ubrk_first(line);
    if(pos != 0)
        log_err("error ubrk_first(line) returned %d, expected 0\n", (int32_t)pos);
    pos = ubrk_next(line);
    pos=ubrk_following(line, 18);
    if(pos!=22)
        log_err("error ubrk_following(line) did not return 22\n");
    log_verbose("following (line) = %d\n", (int32_t)pos);

    
    log_verbose("\nTesting the functions for sentence\n");
    ubrk_first(sentence);
    pos = ubrk_current(sentence);
    log_verbose("Current(sentence) = %d\n", (int32_t)pos);
       pos = ubrk_last(sentence);
    if(pos!=49)
        log_err("error ubrk_last for sentence did not return 49\n");
    log_verbose("Last (sentence) = %d\n", (int32_t)pos);
    ubrk_first(sentence);
    to = ubrk_following( sentence, 0 );
    if (to == 0) log_err("ubrk_following returned 0\n");
    to = ubrk_preceding( sentence, to );
    if (to != 0) log_err("ubrk_preceding didn't return 0\n");
    if (ubrk_first(sentence)!=ubrk_current(sentence)) {
        log_err("error in ubrk_first() or ubrk_current()\n");
    }
    
 
    /*---- */
    /*Testing ubrk_open and ubrk_close()*/
   log_verbose("\nTesting open and close for us locale\n");
    b = ubrk_open(UBRK_WORD, "fr_FR", text, u_strlen(text), &status);
    if (U_FAILURE(status)) {
        log_err("ubrk_open for word returned NULL: %s\n", myErrorName(status));
    }
    ubrk_close(b);

    /* Test setText and setUText */
    {
        UChar s1[] = {0x41, 0x42, 0x20, 0};
        UChar s2[] = {0x41, 0x42, 0x43, 0x44, 0x45, 0};
        UText *ut = NULL;
        UBreakIterator *bb;
        int j;

        log_verbose("\nTesting ubrk_setText() and ubrk_setUText()\n");
        status = U_ZERO_ERROR;
        bb = ubrk_open(UBRK_WORD, "en_US", NULL, 0, &status);
        TEST_ASSERT_SUCCESS(status);
        ubrk_setText(bb, s1, -1, &status);
        TEST_ASSERT_SUCCESS(status);
        ubrk_first(bb);
        j = ubrk_next(bb);
        TEST_ASSERT(j == 2);
        ut = utext_openUChars(ut, s2, -1, &status);
        ubrk_setUText(bb, ut, &status);
        TEST_ASSERT_SUCCESS(status);
        j = ubrk_next(bb);
        TEST_ASSERT(j == 5);

        ubrk_close(bb);
        utext_close(ut);
    }

    ubrk_close(word);
    ubrk_close(sentence);
    ubrk_close(line);
    ubrk_close(character);
}

static void TestBreakIteratorSafeClone(void)
{
    UChar text[51];     /* Keep this odd to test for 64-bit memory alignment */
                        /*  NOTE:  This doesn't reliably force mis-alignment of following items. */ 
    uint8_t buffer [CLONETEST_ITERATOR_COUNT] [U_BRK_SAFECLONE_BUFFERSIZE];
    int32_t bufferSize = U_BRK_SAFECLONE_BUFFERSIZE;

    UBreakIterator * someIterators [CLONETEST_ITERATOR_COUNT];
    UBreakIterator * someClonedIterators [CLONETEST_ITERATOR_COUNT];

    UBreakIterator * brk;
    UErrorCode status = U_ZERO_ERROR;
    int32_t start,pos;
    int32_t i;

    /*Testing ubrk_safeClone */

    /* Note:  the adjacent "" are concatenating strings, not adding a \" to the
       string, which is probably what whoever wrote this intended.  Don't fix,
       because it would throw off the hard coded break positions in the following
       tests. */
    u_uastrcpy(text, "He's from Africa. ""Mr. Livingston, I presume?"" Yeah");

    /* US & Thai - rule-based & dictionary based */
    someIterators[0] = ubrk_open(UBRK_WORD, "en_US", text, u_strlen(text), &status);
    if(!someIterators[0] || U_FAILURE(status)) {
      log_data_err("Couldn't open en_US word break iterator - %s\n", u_errorName(status));
      return;
    }

    someIterators[1] = ubrk_open(UBRK_WORD, "th_TH", text, u_strlen(text), &status);
    if(!someIterators[1] || U_FAILURE(status)) {
      log_data_err("Couldn't open th_TH word break iterator - %s\n", u_errorName(status));
      return;
    }

    /* test each type of iterator */
    for (i = 0; i < CLONETEST_ITERATOR_COUNT; i++)
    {

        /* Check the various error & informational states */

        /* Null status - just returns NULL */
        if (0 != ubrk_safeClone(someIterators[i], buffer[i], &bufferSize, 0))
        {
            log_err("FAIL: Cloned Iterator failed to deal correctly with null status\n");
        }
        /* error status - should return 0 & keep error the same */
        status = U_MEMORY_ALLOCATION_ERROR;
        if (0 != ubrk_safeClone(someIterators[i], buffer[i], &bufferSize, &status) || status != U_MEMORY_ALLOCATION_ERROR)
        {
            log_err("FAIL: Cloned Iterator failed to deal correctly with incoming error status\n");
        }
        status = U_ZERO_ERROR;

        /* Null buffer size pointer - just returns NULL & set error to U_ILLEGAL_ARGUMENT_ERROR*/
        if (0 != ubrk_safeClone(someIterators[i], buffer[i], 0, &status) || status != U_ILLEGAL_ARGUMENT_ERROR)
        {
            log_err("FAIL: Cloned Iterator failed to deal correctly with null bufferSize pointer\n");
        }
        status = U_ZERO_ERROR;
    
        /* buffer size pointer is 0 - fill in pbufferSize with a size */
        bufferSize = 0;
        if (0 != ubrk_safeClone(someIterators[i], buffer[i], &bufferSize, &status) || U_FAILURE(status) || bufferSize <= 0)
        {
            log_err("FAIL: Cloned Iterator failed a sizing request ('preflighting')\n");
        }
        /* Verify our define is large enough  */
        if (U_BRK_SAFECLONE_BUFFERSIZE < bufferSize)
        {
            log_err("FAIL: Pre-calculated buffer size is too small\n");
        }
        /* Verify we can use this run-time calculated size */
        if (0 == (brk = ubrk_safeClone(someIterators[i], buffer[i], &bufferSize, &status)) || U_FAILURE(status))
        {
            log_err("FAIL: Iterator can't be cloned with run-time size\n");
        }
        if (brk)
            ubrk_close(brk);
        /* size one byte too small - should allocate & let us know */
        --bufferSize;
        if (0 == (brk = ubrk_safeClone(someIterators[i], 0, &bufferSize, &status)) || status != U_SAFECLONE_ALLOCATED_WARNING)
        {
            log_err("FAIL: Cloned Iterator failed to deal correctly with too-small buffer size\n");
        }
        if (brk)
            ubrk_close(brk);
        status = U_ZERO_ERROR;
        bufferSize = U_BRK_SAFECLONE_BUFFERSIZE;

        /* Null buffer pointer - return Iterator & set error to U_SAFECLONE_ALLOCATED_ERROR */
        if (0 == (brk = ubrk_safeClone(someIterators[i], 0, &bufferSize, &status)) || status != U_SAFECLONE_ALLOCATED_WARNING)
        {
            log_err("FAIL: Cloned Iterator failed to deal correctly with null buffer pointer\n");
        }
        if (brk)
            ubrk_close(brk);
        status = U_ZERO_ERROR;

        /* Mis-aligned buffer pointer. */
        {
            char  stackBuf[U_BRK_SAFECLONE_BUFFERSIZE+sizeof(void *)];
            void  *p;
            int32_t offset;

            brk = ubrk_safeClone(someIterators[i], &stackBuf[1], &bufferSize, &status);
            if (U_FAILURE(status) || brk == 0) {
                log_err("FAIL: Cloned Iterator failed with misaligned buffer pointer\n");
            }
            if (status == U_SAFECLONE_ALLOCATED_WARNING) {
                log_err("FAIL: Cloned Iterator allocated when using a mis-aligned buffer.\n");
            }
            offset = (int32_t)((char *)&p-(char*)brk);
            if (offset < 0) {
                offset = -offset;
            }
            if (offset % sizeof(void *) != 0) {
                log_err("FAIL: Cloned Iterator failed to align correctly with misaligned buffer pointer\n");
            }
            if (brk)
                ubrk_close(brk);
        }


        /* Null Iterator - return NULL & set U_ILLEGAL_ARGUMENT_ERROR */
        if (0 != ubrk_safeClone(0, buffer[i], &bufferSize, &status) || status != U_ILLEGAL_ARGUMENT_ERROR)
        {
            log_err("FAIL: Cloned Iterator failed to deal correctly with null Iterator pointer\n");
        }
        status = U_ZERO_ERROR;

        /* Do these cloned Iterators work at all - make a first & next call */
        bufferSize = U_BRK_SAFECLONE_BUFFERSIZE;
        someClonedIterators[i] = ubrk_safeClone(someIterators[i], buffer[i], &bufferSize, &status);

        start = ubrk_first(someClonedIterators[i]);
        if(start!=0)
            log_err("error ubrk_start(clone) did not return 0\n");
        pos=ubrk_next(someClonedIterators[i]);
        if(pos!=4)
            log_err("error ubrk_next(clone) did not return 4\n");

        ubrk_close(someClonedIterators[i]);
        ubrk_close(someIterators[i]);
    }
}


/*
//  Open a break iterator from char * rules.  Take care of conversion
//     of the rules and error checking.
*/
static UBreakIterator * testOpenRules(char *rules) {
    UErrorCode      status       = U_ZERO_ERROR;
    UChar          *ruleSourceU  = NULL;
    void           *strCleanUp   = NULL;
    UParseError     parseErr;
    UBreakIterator *bi;

    ruleSourceU = toUChar(rules, &strCleanUp);

    bi = ubrk_openRules(ruleSourceU,  -1,     /*  The rules  */
                        NULL,  -1,            /*  The text to be iterated over. */
                        &parseErr, &status);
    
    if (U_FAILURE(status)) {
        log_data_err("FAIL: ubrk_openRules: ICU Error \"%s\" (Are you missing data?)\n", u_errorName(status));
        bi = 0;
    };
    freeToUCharStrings(&strCleanUp);
    return bi;

}

/*
 *  TestBreakIteratorRules - Verify that a break iterator can be created from
 *                           a set of source rules.
 */
static void TestBreakIteratorRules() {
    /*  Rules will keep together any run of letters not including 'a', OR
     *             keep together 'abc', but only when followed by 'def', OTHERWISE
     *             just return one char at a time.
     */
    char         rules[]  = "abc{666}/def;\n   [\\p{L} - [a]]* {2};  . {1};";
    /*                        0123456789012345678 */
    char         data[]   =  "abcdex abcdefgh-def";     /* the test data string                     */
    char         breaks[] =  "**    **  *    **  *";    /*  * the expected break positions          */
    char         tags[]   =  "01    21  6    21  2";    /*  expected tag values at break positions  */
    int32_t      tagMap[] = {0, 1, 2, 3, 4, 5, 666};

    UChar       *uData;
    void        *freeHook = NULL;
    UErrorCode   status   = U_ZERO_ERROR;
    int32_t      pos;
    int          i;

    UBreakIterator *bi = testOpenRules(rules);
    if (bi == NULL) {return;}
    uData = toUChar(data, &freeHook);
    ubrk_setText(bi,  uData, -1, &status);

    pos = ubrk_first(bi);
    for (i=0; i<sizeof(breaks); i++) {
        if (pos == i && breaks[i] != '*') {
            log_err("FAIL: unexpected break at position %d found\n", pos);
            break;
        }
        if (pos != i && breaks[i] == '*') {
            log_err("FAIL: expected break at position %d not found.\n", i);
            break;
        }
        if (pos == i) {
            int32_t tag, expectedTag;
            tag = ubrk_getRuleStatus(bi);
            expectedTag = tagMap[tags[i]&0xf];
            if (tag != expectedTag) {
                log_err("FAIL: incorrect tag value.  Position = %d;  expected tag %d, got %d",
                    pos, expectedTag, tag);
                break;
            }
            pos = ubrk_next(bi);
        }
    }

    freeToUCharStrings(&freeHook);
    ubrk_close(bi);
}

static void TestBreakIteratorRuleError() {
/*
 *  TestBreakIteratorRuleError -   Try to create a BI from rules with syntax errors,
 *                                 check that the error is reported correctly.
 */
    char            rules[]  = "           #  This is a rule comment on line 1\n"
                               "[:L:];     # this rule is OK.\n"
                               "abcdefg);  # Error, mismatched parens\n";
    UChar          *uRules;
    void           *freeHook = NULL;
    UErrorCode      status   = U_ZERO_ERROR;
    UParseError     parseErr;
    UBreakIterator *bi;

    uRules = toUChar(rules, &freeHook);
    bi = ubrk_openRules(uRules,  -1,          /*  The rules  */
                        NULL,  -1,            /*  The text to be iterated over. */
                        &parseErr, &status);
    if (U_SUCCESS(status)) {
        log_err("FAIL: construction of break iterator succeeded when it should have failed.\n");
        ubrk_close(bi);
    } else {
        if (parseErr.line != 3 || parseErr.offset != 8) {
            log_data_err("FAIL: incorrect error position reported. Got line %d, char %d, expected line 3, char 7 (Are you missing data?)\n",
                parseErr.line, parseErr.offset);
        }
    }
    freeToUCharStrings(&freeHook);
}


/*
*   TestsBreakIteratorStatusVals()   Test the ubrk_getRuleStatusVec() funciton
*/
static void TestBreakIteratorStatusVec() {
    #define RULE_STRING_LENGTH 200
    UChar          rules[RULE_STRING_LENGTH];

    #define TEST_STRING_LENGTH 25
    UChar           testString[TEST_STRING_LENGTH];
    UBreakIterator *bi        = NULL;
    int32_t         pos       = 0;
    int32_t         vals[10];
    int32_t         numVals;
    UErrorCode      status    = U_ZERO_ERROR;

    u_uastrncpy(rules,  "[A-N]{100}; \n"
                             "[a-w]{200}; \n"
                             "[\\p{L}]{300}; \n"
                             "[\\p{N}]{400}; \n"
                             "[0-5]{500}; \n"
                              "!.*;\n", RULE_STRING_LENGTH);
    u_uastrncpy(testString, "ABC", TEST_STRING_LENGTH);


    bi = ubrk_openRules(rules, -1, testString, -1, NULL, &status);
    TEST_ASSERT_SUCCESS(status);
    TEST_ASSERT(bi != NULL);

    /* The TEST_ASSERT above should change too... */
    if (bi != NULL) {
        pos = ubrk_next(bi);
        TEST_ASSERT(pos == 1);

        memset(vals, -1, sizeof(vals));
        numVals = ubrk_getRuleStatusVec(bi, vals, 10, &status);
        TEST_ASSERT_SUCCESS(status);
        TEST_ASSERT(numVals == 2);
        TEST_ASSERT(vals[0] == 100);
        TEST_ASSERT(vals[1] == 300);
        TEST_ASSERT(vals[2] == -1);

        numVals = ubrk_getRuleStatusVec(bi, vals, 0, &status);
        TEST_ASSERT(status == U_BUFFER_OVERFLOW_ERROR);
        TEST_ASSERT(numVals == 2);
    }

    ubrk_close(bi);
}


/*
 *  static void TestBreakIteratorUText(void);
 *
 *         Test that ubrk_setUText() is present and works for a simple case.
 */
static void TestBreakIteratorUText(void) {
    const char *UTF8Str = "\x41\xc3\x85\x5A\x20\x41\x52\x69\x6E\x67";  /* c3 85 is utf-8 for A with a ring on top */
                      /*   0  1   2 34567890  */

    UErrorCode      status = U_ZERO_ERROR;
    UBreakIterator *bi     = NULL;
    int32_t         pos    = 0;


    UText *ut = utext_openUTF8(NULL, UTF8Str, -1, &status);
    TEST_ASSERT_SUCCESS(status);

    bi = ubrk_open(UBRK_WORD, "en_US", NULL, 0, &status);
    if (U_FAILURE(status)) {
        log_err_status(status, "Failure at file %s, line %d, error = %s\n", __FILE__, __LINE__, u_errorName(status));
        return;
    }

    ubrk_setUText(bi, ut, &status);
    if (U_FAILURE(status)) {
        log_err("Failure at file %s, line %d, error = %s\n", __FILE__, __LINE__, u_errorName(status));
        return;
    }

    pos = ubrk_first(bi);
    TEST_ASSERT(pos == 0);

    pos = ubrk_next(bi);
    TEST_ASSERT(pos == 4);

    pos = ubrk_next(bi);
    TEST_ASSERT(pos == 5);

    pos = ubrk_next(bi);
    TEST_ASSERT(pos == 10);

    pos = ubrk_next(bi);
    TEST_ASSERT(pos == UBRK_DONE);
    ubrk_close(bi);
    utext_close(ut);
}

/*
 *  static void TestBreakIteratorTailoring(void);
 *
 *         Test break iterator tailorings from CLDR data.
 */

/* Thai/Lao grapheme break tailoring */
static const UChar thTest[] = { 0x0020, 0x0E40, 0x0E01, 0x0020,
                                0x0E01, 0x0E30, 0x0020, 0x0E01, 0x0E33, 0x0020, 0 };
static const int32_t thTestOffs_enFwd[] = {  1,      3,  4,      6,  7,      9, 10 };
static const int32_t thTestOffs_thFwd[] = {  1,  2,  3,  4,  5,  6,  7,      9, 10 };
static const int32_t thTestOffs_enRev[] = {  9,      7,  6,      4,  3,      1,  0 };
static const int32_t thTestOffs_thRev[] = {  9,      7,  6,  5,  4,  3,  2,  1,  0 };

/* Hebrew line break tailoring, for cldrbug 3028 */
static const UChar heTest[] = { 0x0020, 0x002D, 0x0031, 0x0032, 0x0020,
                                0x0061, 0x002D, 0x006B, 0x0020,
                                0x0061, 0x0300, 0x2010, 0x006B, 0x0020,
                                0x05DE, 0x05D4, 0x002D, 0x0069, 0x0020,
                                0x05D1, 0x05BC, 0x2010, 0x0047, 0x0020, 0 };
static const int32_t heTestOffs_enFwd[] = {  1,  5,  7,  9, 12, 14, 17, 19, 22, 24 };
static const int32_t heTestOffs_heFwd[] = {  1,  5,  7,  9, 12, 14,     19,     24 };
static const int32_t heTestOffs_enRev[] = { 22, 19, 17, 14, 12,  9,  7,  5,  1,  0 };
static const int32_t heTestOffs_heRev[] = {     19,     14, 12,  9,  7,  5,  1,  0 };

/* Finnish line break tailoring, for cldrbug 3029 */
static const UChar fiTest[] = { /* 00 */ 0x0020, 0x002D, 0x0031, 0x0032, 0x0020,
                                /* 05 */ 0x0061, 0x002D, 0x006B, 0x0020,
                                /* 09 */ 0x0061, 0x0300, 0x2010, 0x006B, 0x0020,
                                /* 14 */ 0x0061, 0x0020, 0x002D, 0x006B, 0x0020,
                                /* 19 */ 0x0061, 0x0300, 0x0020, 0x2010, 0x006B, 0x0020, 0 };
static const int32_t fiTestOffs_enFwd[] =  {  1,  5,  7,  9, 12, 14, 16, 17, 19, 22, 23, 25 };
static const int32_t fiTestOffs_fiFwd[] =  {  1,  5,  7,  9, 12, 14, 16,     19, 22,     25 };
static const int32_t fiTestOffs_enRev[] =  { 23, 22, 19, 17, 16, 14, 12,  9,  7,  5,  1,  0 };
static const int32_t fiTestOffs_fiRev[] =  {     22, 19,     16, 14, 12,  9,  7,  5,  1,  0 };

typedef struct {
    const char * locale;
    UBreakIteratorType type;
    const UChar * test;
    const int32_t * offsFwd;
    const int32_t * offsRev;
    int32_t numOffsets;
} RBBITailoringTest;

static const RBBITailoringTest tailoringTests[] = {
    { "en", UBRK_CHARACTER, thTest, thTestOffs_enFwd, thTestOffs_enRev, sizeof(thTestOffs_enFwd)/sizeof(thTestOffs_enFwd[0]) },
    { "th", UBRK_CHARACTER, thTest, thTestOffs_thFwd, thTestOffs_thRev, sizeof(thTestOffs_thFwd)/sizeof(thTestOffs_thFwd[0]) },
    { "en", UBRK_LINE,      heTest, heTestOffs_enFwd, heTestOffs_enRev, sizeof(heTestOffs_enFwd)/sizeof(heTestOffs_enFwd[0]) },
    { "he", UBRK_LINE,      heTest, heTestOffs_heFwd, heTestOffs_heRev, sizeof(heTestOffs_heFwd)/sizeof(heTestOffs_heFwd[0]) },
    { "en", UBRK_LINE,      fiTest, fiTestOffs_enFwd, fiTestOffs_enRev, sizeof(fiTestOffs_enFwd)/sizeof(fiTestOffs_enFwd[0]) },
    { "fi", UBRK_LINE,      fiTest, fiTestOffs_fiFwd, fiTestOffs_fiRev, sizeof(fiTestOffs_fiFwd)/sizeof(fiTestOffs_fiFwd[0]) },
    { NULL, 0, NULL, NULL, NULL, 0 },
};

static void TestBreakIteratorTailoring(void) {
    const RBBITailoringTest * testPtr;
    for (testPtr = tailoringTests; testPtr->locale != NULL; ++testPtr) {
        UErrorCode status = U_ZERO_ERROR;
        UBreakIterator* ubrkiter = ubrk_open(testPtr->type, testPtr->locale, testPtr->test, -1, &status);
        if ( U_SUCCESS(status) ) {
            int32_t offset, offsindx;
            UBool foundError;

            foundError = FALSE;
            for (offsindx = 0; (offset = ubrk_next(ubrkiter)) != UBRK_DONE; ++offsindx) {
                if (!foundError && offsindx >= testPtr->numOffsets) {
                    log_err("FAIL: locale %s, break type %d, ubrk_next expected UBRK_DONE, got %d\n",
                            testPtr->locale, testPtr->type, offset);
                    foundError = TRUE;
                } else if (!foundError && offset != testPtr->offsFwd[offsindx]) {
                    log_err("FAIL: locale %s, break type %d, ubrk_next expected %d, got %d\n",
                            testPtr->locale, testPtr->type, testPtr->offsFwd[offsindx], offset);
                    foundError = TRUE;
                }
            }
            if (!foundError && offsindx < testPtr->numOffsets) {
                log_err("FAIL: locale %s, break type %d, ubrk_next expected %d, got UBRK_DONE\n",
                    	testPtr->locale, testPtr->type, testPtr->offsFwd[offsindx]);
            }

            foundError = FALSE;
            for (offsindx = 0; (offset = ubrk_previous(ubrkiter)) != UBRK_DONE; ++offsindx) {
                if (!foundError && offsindx >= testPtr->numOffsets) {
                    log_err("FAIL: locale %s, break type %d, ubrk_previous expected UBRK_DONE, got %d\n",
                            testPtr->locale, testPtr->type, offset);
                    foundError = TRUE;
                } else if (!foundError && offset != testPtr->offsRev[offsindx]) {
                    log_err("FAIL: locale %s, break type %d, ubrk_previous expected %d, got %d\n",
                            testPtr->locale, testPtr->type, testPtr->offsRev[offsindx], offset);
                    foundError = TRUE;
                }
            }
            if (!foundError && offsindx < testPtr->numOffsets) {
                log_err("FAIL: locale %s, break type %d, ubrk_previous expected %d, got UBRK_DONE\n",
                    	testPtr->locale, testPtr->type, testPtr->offsRev[offsindx]);
            }

            ubrk_close(ubrkiter);
        } else {
            log_err_status(status, "FAIL: locale %s, break type %d, ubrk_open status: %s\n", testPtr->locale, testPtr->type, u_errorName(status));
        }
    }
}

#endif /* #if !UCONFIG_NO_BREAK_ITERATION */
