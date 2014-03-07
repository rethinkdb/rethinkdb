/*
******************************************************************************
*
*   Copyright (C) 2002-2010, International Business Machines
*   Corporation and others.  All Rights Reserved.
*
******************************************************************************
*   file name:  custrtst.c
*   encoding:   US-ASCII
*   tab size:   8 (not used)
*   indentation:4
*
*   created on: 2002oct09
*   created by: Markus W. Scherer
*
*   Tests of ustring.h Unicode string API functions.
*/

#include "unicode/ustring.h"
#include "unicode/ucnv.h"
#include "unicode/uiter.h"
#include "cintltst.h"
#include <string.h>

#define LENGTHOF(array) (sizeof(array)/sizeof((array)[0]))

/* get the sign of an integer */
#define _SIGN(value) ((value)==0 ? 0 : ((int32_t)(value)>>31)|1)

/* test setup --------------------------------------------------------------- */

static void setUpDataTable(void);
static void TestStringCopy(void);
static void TestStringFunctions(void);
static void TestStringSearching(void);
static void TestSurrogateSearching(void);
static void TestUnescape(void);
static void TestCountChar32(void);
static void TestUCharIterator(void);
static void TestUNormIterator(void);
static void TestBadUNormIterator(void);

void addUStringTest(TestNode** root);

void addUStringTest(TestNode** root)
{
    addTest(root, &TestStringCopy, "tsutil/custrtst/TestStringCopy");
    addTest(root, &TestStringFunctions, "tsutil/custrtst/TestStringFunctions");
    addTest(root, &TestStringSearching, "tsutil/custrtst/TestStringSearching");
    addTest(root, &TestSurrogateSearching, "tsutil/custrtst/TestSurrogateSearching");
    addTest(root, &TestUnescape, "tsutil/custrtst/TestUnescape");
    addTest(root, &TestCountChar32, "tsutil/custrtst/TestCountChar32");
    addTest(root, &TestUCharIterator, "tsutil/custrtst/TestUCharIterator");
    addTest(root, &TestUNormIterator, "tsutil/custrtst/TestUNormIterator");
    addTest(root, &TestBadUNormIterator, "tsutil/custrtst/TestBadUNormIterator");
}

/* test data for TestStringFunctions ---------------------------------------- */

UChar*** dataTable = NULL;

static const char* raw[3][4] = {

    /* First String */
    {   "English_",  "French_",   "Croatian_", "English_"},
    /* Second String */
    {   "United States",    "France",   "Croatia",  "Unites States"},

   /* Concatenated string */
    {   "English_United States", "French_France", "Croatian_Croatia", "English_United States"}
};

static void setUpDataTable()
{
    int32_t i,j;
    if(dataTable == NULL) {
        dataTable = (UChar***)calloc(sizeof(UChar**),3);

            for (i = 0; i < 3; i++) {
              dataTable[i] = (UChar**)calloc(sizeof(UChar*),4);
                for (j = 0; j < 4; j++){
                    dataTable[i][j] = (UChar*) malloc(sizeof(UChar)*(strlen(raw[i][j])+1));
                    u_uastrcpy(dataTable[i][j],raw[i][j]);
                }
            }
    }
}

static void cleanUpDataTable()
{
    int32_t i,j;
    if(dataTable != NULL) {
        for (i=0; i<3; i++) {
            for(j = 0; j<4; j++) {
                free(dataTable[i][j]);
            }
            free(dataTable[i]);
        }
        free(dataTable);
    }
    dataTable = NULL;
}

/*Tests  for u_strcat(),u_strcmp(), u_strlen(), u_strcpy(),u_strncat(),u_strncmp(),u_strncpy, u_uastrcpy(),u_austrcpy(), u_uastrncpy(); */
static void TestStringFunctions()
{
    int32_t i,j,k;
    UChar temp[512];
    UChar nullTemp[512];
    char test[512];
    char tempOut[512];

    setUpDataTable();

    log_verbose("Testing u_strlen()\n");
    if( u_strlen(dataTable[0][0])!= u_strlen(dataTable[0][3]) || u_strlen(dataTable[0][0]) == u_strlen(dataTable[0][2]))
        log_err("There is an error in u_strlen()");

    log_verbose("Testing u_memcpy() and u_memcmp()\n");

    for(i=0;i<3;++i)
    {
        for(j=0;j<4;++j)
        {
            log_verbose("Testing  %s\n", u_austrcpy(tempOut, dataTable[i][j]));
            temp[0] = 0;
            temp[7] = 0xA4; /* Mark the end */
            u_memcpy(temp,dataTable[i][j], 7);

            if(temp[7] != 0xA4)
                log_err("an error occured in u_memcpy()\n");
            if(u_memcmp(temp, dataTable[i][j], 7)!=0)
                log_err("an error occured in u_memcpy() or u_memcmp()\n");
        }
    }
    if(u_memcmp(dataTable[0][0], dataTable[1][1], 7)==0)
        log_err("an error occured in u_memcmp()\n");

    log_verbose("Testing u_memset()\n");
    nullTemp[0] = 0;
    nullTemp[7] = 0;
    u_memset(nullTemp, 0xa4, 7);
    for (i = 0; i < 7; i++) {
        if(nullTemp[i] != 0xa4) {
            log_err("an error occured in u_memset()\n");
        }
    }
    if(nullTemp[7] != 0) {
        log_err("u_memset() went too far\n");
    }

    u_memset(nullTemp, 0, 7);
    nullTemp[7] = 0xa4;
    temp[7] = 0;
    u_memcpy(temp,nullTemp, 7);
    if(u_memcmp(temp, nullTemp, 7)!=0 || temp[7]!=0)
        log_err("an error occured in u_memcpy() or u_memcmp()\n");


    log_verbose("Testing u_memmove()\n");
    for (i = 0; i < 7; i++) {
        temp[i] = (UChar)i;
    }
    u_memmove(temp + 1, temp, 7);
    if(temp[0] != 0) {
        log_err("an error occured in u_memmove()\n");
    }
    for (i = 1; i <= 7; i++) {
        if(temp[i] != (i - 1)) {
            log_err("an error occured in u_memmove()\n");
        }
    }

    log_verbose("Testing u_strcpy() and u_strcmp()\n");

    for(i=0;i<3;++i)
    {
        for(j=0;j<4;++j)
        {
            log_verbose("Testing  %s\n", u_austrcpy(tempOut, dataTable[i][j]));
            temp[0] = 0;
            u_strcpy(temp,dataTable[i][j]);

            if(u_strcmp(temp,dataTable[i][j])!=0)
                log_err("something threw an error in u_strcpy() or u_strcmp()\n");
        }
    }
    if(u_strcmp(dataTable[0][0], dataTable[1][1])==0)
        log_err("an error occured in u_memcmp()\n");

    log_verbose("testing u_strcat()\n");
    i=0;
    for(j=0; j<2;++j)
    {
        u_uastrcpy(temp, "");
        u_strcpy(temp,dataTable[i][j]);
        u_strcat(temp,dataTable[i+1][j]);
        if(u_strcmp(temp,dataTable[i+2][j])!=0)
            log_err("something threw an error in u_strcat()\n");

    }
    log_verbose("Testing u_strncmp()\n");
    for(i=0,j=0;j<4; ++j)
    {
        k=u_strlen(dataTable[i][j]);
        if(u_strncmp(dataTable[i][j],dataTable[i+2][j],k)!=0)
            log_err("Something threw an error in u_strncmp\n");
    }
    if(u_strncmp(dataTable[0][0], dataTable[1][1], 7)==0)
        log_err("an error occured in u_memcmp()\n");


    log_verbose("Testing u_strncat\n");
    for(i=0,j=0;j<4; ++j)
    {
        k=u_strlen(dataTable[i][j]);

        u_uastrcpy(temp,"");

        if(u_strcmp(u_strncat(temp,dataTable[i+2][j],k),dataTable[i][j])!=0)
            log_err("something threw an error in u_strncat or u_uastrcpy()\n");

    }

    log_verbose("Testing u_strncpy() and u_uastrcpy()\n");
    for(i=2,j=0;j<4; ++j)
    {
        k=u_strlen(dataTable[i][j]);
        u_strncpy(temp, dataTable[i][j],k);
        temp[k] = 0xa4;

        if(u_strncmp(temp, dataTable[i][j],k)!=0)
            log_err("something threw an error in u_strncpy()\n");

        if(temp[k] != 0xa4)
            log_err("something threw an error in u_strncpy()\n");

        u_memset(temp, 0x3F, (sizeof(temp) / sizeof(UChar)) - 1);
        u_uastrncpy(temp, raw[i][j], k-1);
        if(u_strncmp(temp, dataTable[i][j],k-1)!=0)
            log_err("something threw an error in u_uastrncpy(k-1)\n");

        if(temp[k-1] != 0x3F)
            log_err("something threw an error in u_uastrncpy(k-1)\n");

        u_memset(temp, 0x3F, (sizeof(temp) / sizeof(UChar)) - 1);
        u_uastrncpy(temp, raw[i][j], k+1);
        if(u_strcmp(temp, dataTable[i][j])!=0)
            log_err("something threw an error in u_uastrncpy(k+1)\n");

        if(temp[k] != 0)
            log_err("something threw an error in u_uastrncpy(k+1)\n");

        u_memset(temp, 0x3F, (sizeof(temp) / sizeof(UChar)) - 1);
        u_uastrncpy(temp, raw[i][j], k);
        if(u_strncmp(temp, dataTable[i][j], k)!=0)
            log_err("something threw an error in u_uastrncpy(k)\n");

        if(temp[k] != 0x3F)
            log_err("something threw an error in u_uastrncpy(k)\n");
    }

    log_verbose("Testing u_strchr() and u_memchr()\n");

    for(i=2,j=0;j<4;j++)
    {
        UChar saveVal = dataTable[i][j][0];
        UChar *findPtr = u_strchr(dataTable[i][j], 0x005F);
        int32_t dataSize = (int32_t)(u_strlen(dataTable[i][j]) + 1);

        log_verbose("%s ", u_austrcpy(tempOut, findPtr));

        if (findPtr == NULL || *findPtr != 0x005F) {
            log_err("u_strchr can't find '_' in the string\n");
        }

        findPtr = u_strchr32(dataTable[i][j], 0x005F);
        if (findPtr == NULL || *findPtr != 0x005F) {
            log_err("u_strchr32 can't find '_' in the string\n");
        }

        findPtr = u_strchr(dataTable[i][j], 0);
        if (findPtr != (&(dataTable[i][j][dataSize - 1]))) {
            log_err("u_strchr can't find NULL in the string\n");
        }

        findPtr = u_strchr32(dataTable[i][j], 0);
        if (findPtr != (&(dataTable[i][j][dataSize - 1]))) {
            log_err("u_strchr32 can't find NULL in the string\n");
        }

        findPtr = u_memchr(dataTable[i][j], 0, dataSize);
        if (findPtr != (&(dataTable[i][j][dataSize - 1]))) {
            log_err("u_memchr can't find NULL in the string\n");
        }

        findPtr = u_memchr32(dataTable[i][j], 0, dataSize);
        if (findPtr != (&(dataTable[i][j][dataSize - 1]))) {
            log_err("u_memchr32 can't find NULL in the string\n");
        }

        dataTable[i][j][0] = 0;
        /* Make sure we skip over the NULL termination */
        findPtr = u_memchr(dataTable[i][j], 0x005F, dataSize);
        if (findPtr == NULL || *findPtr != 0x005F) {
            log_err("u_memchr can't find '_' in the string\n");
        }

        findPtr = u_memchr32(dataTable[i][j], 0x005F, dataSize);
        if (findPtr == NULL || *findPtr != 0x005F) {
            log_err("u_memchr32 can't find '_' in the string\n");
        }
        findPtr = u_memchr32(dataTable[i][j], 0xFFFD, dataSize);
        if (findPtr != NULL) {
            log_err("Should have found NULL when the character is not there.\n");
        }
        dataTable[i][j][0] = saveVal;   /* Put it back for the other tests */
    }

    /*
     * test that u_strchr32()
     * does not find surrogate code points when they are part of matched pairs
     * (= part of supplementary code points)
     * Jitterbug 1542
     */
    {
        static const UChar s[]={
            /*   0       1       2       3       4       5       6       7       8  9 */
            0x0061, 0xd841, 0xdc02, 0xd841, 0x0062, 0xdc02, 0xd841, 0xdc02, 0x0063, 0
        };

        if(u_strchr32(s, 0xd841)!=(s+3) || u_strchr32(s, 0xdc02)!=(s+5)) {
            log_err("error: u_strchr32(surrogate) finds a partial supplementary code point\n");
        }
        if(u_memchr32(s, 0xd841, 9)!=(s+3) || u_memchr32(s, 0xdc02, 9)!=(s+5)) {
            log_err("error: u_memchr32(surrogate) finds a partial supplementary code point\n");
        }
    }

    log_verbose("Testing u_austrcpy()");
    u_austrcpy(test,dataTable[0][0]);
    if(strcmp(test,raw[0][0])!=0)
        log_err("There is an error in u_austrcpy()");


    log_verbose("Testing u_strtok_r()");
    {
        const char tokString[] = "  ,  1 2 3  AHHHHH! 5.5 6 7    ,        8\n";
        const char *tokens[] = {",", "1", "2", "3", "AHHHHH!", "5.5", "6", "7", "8\n"};
        UChar delimBuf[sizeof(test)];
        UChar currTokenBuf[sizeof(tokString)];
        UChar *state;
        uint32_t currToken = 0;
        UChar *ptr;

        u_uastrcpy(temp, tokString);
        u_uastrcpy(delimBuf, " ");

        ptr = u_strtok_r(temp, delimBuf, &state);
        u_uastrcpy(delimBuf, " ,");
        while (ptr != NULL) {
            u_uastrcpy(currTokenBuf, tokens[currToken]);
            if (u_strcmp(ptr, currTokenBuf) != 0) {
                log_err("u_strtok_r mismatch at %d. Got: %s, Expected: %s\n", currToken, ptr, tokens[currToken]);
            }
            ptr = u_strtok_r(NULL, delimBuf, &state);
            currToken++;
        }

        if (currToken != sizeof(tokens)/sizeof(tokens[0])) {
            log_err("Didn't get correct number of tokens\n");
        }
        state = delimBuf;       /* Give it an "invalid" saveState */
        u_uastrcpy(currTokenBuf, "");
        if (u_strtok_r(currTokenBuf, delimBuf, &state) != NULL) {
            log_err("Didn't get NULL for empty string\n");
        }
        if (state != NULL) {
            log_err("State should be NULL for empty string\n");
        }
        state = delimBuf;       /* Give it an "invalid" saveState */
        u_uastrcpy(currTokenBuf, ", ,");
        if (u_strtok_r(currTokenBuf, delimBuf, &state) != NULL) {
            log_err("Didn't get NULL for a string of delimiters\n");
        }
        if (state != NULL) {
            log_err("State should be NULL for a string of delimiters\n");
        }

        state = delimBuf;       /* Give it an "invalid" saveState */
        u_uastrcpy(currTokenBuf, "q, ,");
        if (u_strtok_r(currTokenBuf, delimBuf, &state) == NULL) {
            log_err("Got NULL for a string that does not begin with delimiters\n");
        }
        if (u_strtok_r(NULL, delimBuf, &state) != NULL) {
            log_err("Didn't get NULL for a string that ends in delimiters\n");
        }
        if (state != NULL) {
            log_err("State should be NULL for empty string\n");
        }

        state = delimBuf;       /* Give it an "invalid" saveState */
        u_uastrcpy(currTokenBuf, tokString);
        u_uastrcpy(temp, tokString);
        u_uastrcpy(delimBuf, "q");  /* Give it a delimiter that it can't find. */
        ptr = u_strtok_r(currTokenBuf, delimBuf, &state);
        if (ptr == NULL || u_strcmp(ptr, temp) != 0) {
            log_err("Should have recieved the same string when there are no delimiters\n");
        }
        if (u_strtok_r(NULL, delimBuf, &state) != NULL) {
            log_err("Should not have found another token in a one token string\n");
        }
    }

    /* test u_strcmpCodePointOrder() */
    {
        /* these strings are in ascending order */
        static const UChar strings[][4]={
            { 0x61, 0 },                    /* U+0061 */
            { 0x20ac, 0xd801, 0 },          /* U+20ac U+d801 */
            { 0x20ac, 0xd800, 0xdc00, 0 },  /* U+20ac U+10000 */
            { 0xd800, 0 },                  /* U+d800 */
            { 0xd800, 0xff61, 0 },          /* U+d800 U+ff61 */
            { 0xdfff, 0 },                  /* U+dfff */
            { 0xff61, 0xdfff, 0 },          /* U+ff61 U+dfff */
            { 0xff61, 0xd800, 0xdc02, 0 },  /* U+ff61 U+10002 */
            { 0xd800, 0xdc02, 0 },          /* U+10002 */
            { 0xd84d, 0xdc56, 0 }           /* U+23456 */
        };

        UCharIterator iter1, iter2;
        int32_t len1, len2, r1, r2;

        for(i=0; i<(sizeof(strings)/sizeof(strings[0])-1); ++i) {
            if(u_strcmpCodePointOrder(strings[i], strings[i+1])>=0) {
                log_err("error: u_strcmpCodePointOrder() fails for string %d and the following one\n", i);
            }
            if(u_strncmpCodePointOrder(strings[i], strings[i+1], 10)>=0) {
                log_err("error: u_strncmpCodePointOrder() fails for string %d and the following one\n", i);
            }

            /* There are at least 2 UChars in each string - verify that strncmp()==memcmp(). */
            if(u_strncmpCodePointOrder(strings[i], strings[i+1], 2)!=u_memcmpCodePointOrder(strings[i], strings[i+1], 2)) {
                log_err("error: u_strncmpCodePointOrder(2)!=u_memcmpCodePointOrder(2) for string %d and the following one\n", i);
            }

            /* test u_strCompare(TRUE) */
            len1=u_strlen(strings[i]);
            len2=u_strlen(strings[i+1]);
            if( u_strCompare(strings[i], -1, strings[i+1], -1, TRUE)>=0 ||
                u_strCompare(strings[i], -1, strings[i+1], len2, TRUE)>=0 ||
                u_strCompare(strings[i], len1, strings[i+1], -1, TRUE)>=0 ||
                u_strCompare(strings[i], len1, strings[i+1], len2, TRUE)>=0
            ) {
                log_err("error: u_strCompare(code point order) fails for string %d and the following one\n", i);
            }

            /* test u_strCompare(FALSE) */
            r1=u_strCompare(strings[i], -1, strings[i+1], -1, FALSE);
            r2=u_strcmp(strings[i], strings[i+1]);
            if(_SIGN(r1)!=_SIGN(r2)) {
                log_err("error: u_strCompare(code unit order)!=u_strcmp() for string %d and the following one\n", i);
            }

            /* test u_strCompareIter() */
            uiter_setString(&iter1, strings[i], len1);
            uiter_setString(&iter2, strings[i+1], len2);
            if(u_strCompareIter(&iter1, &iter2, TRUE)>=0) {
                log_err("error: u_strCompareIter(code point order) fails for string %d and the following one\n", i);
            }
            r1=u_strCompareIter(&iter1, &iter2, FALSE);
            if(_SIGN(r1)!=_SIGN(u_strcmp(strings[i], strings[i+1]))) {
                log_err("error: u_strCompareIter(code unit order)!=u_strcmp() for string %d and the following one\n", i);
            }
        }
    }

    cleanUpDataTable();
}

static void TestStringSearching()
{
    const UChar testString[] = {0x0061, 0x0062, 0x0063, 0x0064, 0x0064, 0x0061, 0};
    const UChar testSurrogateString[] = {0xdbff, 0x0061, 0x0062, 0xdbff, 0xdfff, 0x0063, 0x0064, 0x0064, 0xdbff, 0xdfff, 0xdb00, 0xdf00, 0x0061, 0};
    const UChar surrMatchSet1[] = {0xdbff, 0xdfff, 0};
    const UChar surrMatchSet2[] = {0x0061, 0x0062, 0xdbff, 0xdfff, 0};
    const UChar surrMatchSet3[] = {0xdb00, 0xdf00, 0xdbff, 0xdfff, 0};
    const UChar surrMatchSet4[] = {0x0000};
    const UChar surrMatchSetBad[] = {0xdbff, 0x0061, 0};
    const UChar surrMatchSetBad2[] = {0x0061, 0xdbff, 0};
    const UChar surrMatchSetBad3[] = {0xdbff, 0x0061, 0x0062, 0xdbff, 0xdfff, 0};   /* has partial surrogate */
    const UChar
        empty[] = { 0 },
        a[] = { 0x61, 0 },
        ab[] = { 0x61, 0x62, 0 },
        ba[] = { 0x62, 0x61, 0 },
        abcd[] = { 0x61, 0x62, 0x63, 0x64, 0 },
        cd[] = { 0x63, 0x64, 0 },
        dc[] = { 0x64, 0x63, 0 },
        cdh[] = { 0x63, 0x64, 0x68, 0 },
        f[] = { 0x66, 0 },
        fg[] = { 0x66, 0x67, 0 },
        gf[] = { 0x67, 0x66, 0 };

    log_verbose("Testing u_strpbrk()");

    if (u_strpbrk(testString, a) != &testString[0]) {
        log_err("u_strpbrk couldn't find first letter a.\n");
    }
    if (u_strpbrk(testString, dc) != &testString[2]) {
        log_err("u_strpbrk couldn't find d or c.\n");
    }
    if (u_strpbrk(testString, cd) != &testString[2]) {
        log_err("u_strpbrk couldn't find c or d.\n");
    }
    if (u_strpbrk(testString, cdh) != &testString[2]) {
        log_err("u_strpbrk couldn't find c, d or h.\n");
    }
    if (u_strpbrk(testString, f) != NULL) {
        log_err("u_strpbrk didn't return NULL for \"f\".\n");
    }
    if (u_strpbrk(testString, fg) != NULL) {
        log_err("u_strpbrk didn't return NULL for \"fg\".\n");
    }
    if (u_strpbrk(testString, gf) != NULL) {
        log_err("u_strpbrk didn't return NULL for \"gf\".\n");
    }
    if (u_strpbrk(testString, empty) != NULL) {
        log_err("u_strpbrk didn't return NULL for \"\".\n");
    }

    log_verbose("Testing u_strpbrk() with surrogates");

    if (u_strpbrk(testSurrogateString, a) != &testSurrogateString[1]) {
        log_err("u_strpbrk couldn't find first letter a.\n");
    }
    if (u_strpbrk(testSurrogateString, dc) != &testSurrogateString[5]) {
        log_err("u_strpbrk couldn't find d or c.\n");
    }
    if (u_strpbrk(testSurrogateString, cd) != &testSurrogateString[5]) {
        log_err("u_strpbrk couldn't find c or d.\n");
    }
    if (u_strpbrk(testSurrogateString, cdh) != &testSurrogateString[5]) {
        log_err("u_strpbrk couldn't find c, d or h.\n");
    }
    if (u_strpbrk(testSurrogateString, f) != NULL) {
        log_err("u_strpbrk didn't return NULL for \"f\".\n");
    }
    if (u_strpbrk(testSurrogateString, fg) != NULL) {
        log_err("u_strpbrk didn't return NULL for \"fg\".\n");
    }
    if (u_strpbrk(testSurrogateString, gf) != NULL) {
        log_err("u_strpbrk didn't return NULL for \"gf\".\n");
    }
    if (u_strpbrk(testSurrogateString, surrMatchSet1) != &testSurrogateString[3]) {
        log_err("u_strpbrk couldn't find \"0xdbff, 0xdfff\".\n");
    }
    if (u_strpbrk(testSurrogateString, surrMatchSet2) != &testSurrogateString[1]) {
        log_err("u_strpbrk couldn't find \"0xdbff, a, b, 0xdbff, 0xdfff\".\n");
    }
    if (u_strpbrk(testSurrogateString, surrMatchSet3) != &testSurrogateString[3]) {
        log_err("u_strpbrk couldn't find \"0xdb00, 0xdf00, 0xdbff, 0xdfff\".\n");
    }
    if (u_strpbrk(testSurrogateString, surrMatchSet4) != NULL) {
        log_err("u_strpbrk should have returned NULL for empty string.\n");
    }
    if (u_strpbrk(testSurrogateString, surrMatchSetBad) != &testSurrogateString[0]) {
        log_err("u_strpbrk should have found bad surrogate.\n");
    }

    log_verbose("Testing u_strcspn()");

    if (u_strcspn(testString, a) != 0) {
        log_err("u_strcspn couldn't find first letter a.\n");
    }
    if (u_strcspn(testString, dc) != 2) {
        log_err("u_strcspn couldn't find d or c.\n");
    }
    if (u_strcspn(testString, cd) != 2) {
        log_err("u_strcspn couldn't find c or d.\n");
    }
    if (u_strcspn(testString, cdh) != 2) {
        log_err("u_strcspn couldn't find c, d or h.\n");
    }
    if (u_strcspn(testString, f) != u_strlen(testString)) {
        log_err("u_strcspn didn't return NULL for \"f\".\n");
    }
    if (u_strcspn(testString, fg) != u_strlen(testString)) {
        log_err("u_strcspn didn't return NULL for \"fg\".\n");
    }
    if (u_strcspn(testString, gf) != u_strlen(testString)) {
        log_err("u_strcspn didn't return NULL for \"gf\".\n");
    }

    log_verbose("Testing u_strcspn() with surrogates");

    if (u_strcspn(testSurrogateString, a) != 1) {
        log_err("u_strcspn couldn't find first letter a.\n");
    }
    if (u_strcspn(testSurrogateString, dc) != 5) {
        log_err("u_strcspn couldn't find d or c.\n");
    }
    if (u_strcspn(testSurrogateString, cd) != 5) {
        log_err("u_strcspn couldn't find c or d.\n");
    }
    if (u_strcspn(testSurrogateString, cdh) != 5) {
        log_err("u_strcspn couldn't find c, d or h.\n");
    }
    if (u_strcspn(testSurrogateString, f) != u_strlen(testSurrogateString)) {
        log_err("u_strcspn didn't return NULL for \"f\".\n");
    }
    if (u_strcspn(testSurrogateString, fg) != u_strlen(testSurrogateString)) {
        log_err("u_strcspn didn't return NULL for \"fg\".\n");
    }
    if (u_strcspn(testSurrogateString, gf) != u_strlen(testSurrogateString)) {
        log_err("u_strcspn didn't return NULL for \"gf\".\n");
    }
    if (u_strcspn(testSurrogateString, surrMatchSet1) != 3) {
        log_err("u_strcspn couldn't find \"0xdbff, 0xdfff\".\n");
    }
    if (u_strcspn(testSurrogateString, surrMatchSet2) != 1) {
        log_err("u_strcspn couldn't find \"a, b, 0xdbff, 0xdfff\".\n");
    }
    if (u_strcspn(testSurrogateString, surrMatchSet3) != 3) {
        log_err("u_strcspn couldn't find \"0xdb00, 0xdf00, 0xdbff, 0xdfff\".\n");
    }
    if (u_strcspn(testSurrogateString, surrMatchSet4) != u_strlen(testSurrogateString)) {
        log_err("u_strcspn should have returned strlen for empty string.\n");
    }


    log_verbose("Testing u_strspn()");

    if (u_strspn(testString, a) != 1) {
        log_err("u_strspn couldn't skip first letter a.\n");
    }
    if (u_strspn(testString, ab) != 2) {
        log_err("u_strspn couldn't skip a or b.\n");
    }
    if (u_strspn(testString, ba) != 2) {
        log_err("u_strspn couldn't skip a or b.\n");
    }
    if (u_strspn(testString, f) != 0) {
        log_err("u_strspn didn't return 0 for \"f\".\n");
    }
    if (u_strspn(testString, dc) != 0) {
        log_err("u_strspn couldn't find first letter a (skip d or c).\n");
    }
    if (u_strspn(testString, abcd) != u_strlen(testString)) {
        log_err("u_strspn couldn't skip over the whole string.\n");
    }
    if (u_strspn(testString, empty) != 0) {
        log_err("u_strspn should have returned 0 for empty string.\n");
    }

    log_verbose("Testing u_strspn() with surrogates");
    if (u_strspn(testSurrogateString, surrMatchSetBad) != 2) {
        log_err("u_strspn couldn't skip 0xdbff or a.\n");
    }
    if (u_strspn(testSurrogateString, surrMatchSetBad2) != 2) {
        log_err("u_strspn couldn't skip 0xdbff or a.\n");
    }
    if (u_strspn(testSurrogateString, f) != 0) {
        log_err("u_strspn couldn't skip d or c (skip first letter).\n");
    }
    if (u_strspn(testSurrogateString, dc) != 0) {
        log_err("u_strspn couldn't skip d or c (skip first letter).\n");
    }
    if (u_strspn(testSurrogateString, cd) != 0) {
        log_err("u_strspn couldn't skip d or c (skip first letter).\n");
    }
    if (u_strspn(testSurrogateString, testSurrogateString) != u_strlen(testSurrogateString)) {
        log_err("u_strspn couldn't skip whole string.\n");
    }
    if (u_strspn(testSurrogateString, surrMatchSet1) != 0) {
        log_err("u_strspn couldn't skip \"0xdbff, 0xdfff\" (get first letter).\n");
    }
    if (u_strspn(testSurrogateString, surrMatchSetBad3) != 5) {
        log_err("u_strspn couldn't skip \"0xdbff, a, b, 0xdbff, 0xdfff\".\n");
    }
    if (u_strspn(testSurrogateString, surrMatchSet4) != 0) {
        log_err("u_strspn should have returned 0 for empty string.\n");
    }
}

/*
 * All binary Unicode string searches should behave the same for equivalent input.
 * See Jitterbug 2145.
 * There are some new functions, too - just test them all.
 */
static void
TestSurrogateSearching() {
    static const UChar s[]={
        /* 0       1       2     3       4     5       6     7       8       9    10 11 */
        0x61, 0xd801, 0xdc02, 0x61, 0xdc02, 0x61, 0xd801, 0x61, 0xd801, 0xdc02, 0x61, 0
    }, sub_a[]={
        0x61, 0
    }, sub_b[]={
        0x62, 0
    }, sub_lead[]={
        0xd801, 0
    }, sub_trail[]={
        0xdc02, 0
    }, sub_supp[]={
        0xd801, 0xdc02, 0
    }, sub_supp2[]={
        0xd801, 0xdc03, 0
    }, sub_a_lead[]={
        0x61, 0xd801, 0
    }, sub_trail_a[]={
        0xdc02, 0x61, 0
    }, sub_aba[]={
        0x61, 0x62, 0x61, 0
    };
    static const UChar a=0x61, b=0x62, lead=0xd801, trail=0xdc02, nul=0;
    static const UChar32 supp=0x10402, supp2=0x10403, ill=0x123456;

    const UChar *first, *last;

    /* search for NUL code point: find end of string */
    first=s+u_strlen(s);

    if(
        first!=u_strchr(s, nul) ||
        first!=u_strchr32(s, nul) ||
        first!=u_memchr(s, nul, LENGTHOF(s)) ||
        first!=u_memchr32(s, nul, LENGTHOF(s)) ||
        first!=u_strrchr(s, nul) ||
        first!=u_strrchr32(s, nul) ||
        first!=u_memrchr(s, nul, LENGTHOF(s)) ||
        first!=u_memrchr32(s, nul, LENGTHOF(s))
    ) {
        log_err("error: one of the u_str[|mem][r]chr[32](s, nul) does not find the terminator of s\n");
    }

    /* search for empty substring: find beginning of string */
    if(
        s!=u_strstr(s, &nul) ||
        s!=u_strFindFirst(s, -1, &nul, -1) ||
        s!=u_strFindFirst(s, -1, &nul, 0) ||
        s!=u_strFindFirst(s, LENGTHOF(s), &nul, -1) ||
        s!=u_strFindFirst(s, LENGTHOF(s), &nul, 0) ||
        s!=u_strrstr(s, &nul) ||
        s!=u_strFindLast(s, -1, &nul, -1) ||
        s!=u_strFindLast(s, -1, &nul, 0) ||
        s!=u_strFindLast(s, LENGTHOF(s), &nul, -1) ||
        s!=u_strFindLast(s, LENGTHOF(s), &nul, 0)
    ) {
        log_err("error: one of the u_str[str etc](s, \"\") does not find s itself\n");
    }

    /* find 'a' in s[1..10[ */
    first=s+3;
    last=s+7;
    if(
        first!=u_strchr(s+1, a) ||
        first!=u_strchr32(s+1, a) ||
        first!=u_memchr(s+1, a, 9) ||
        first!=u_memchr32(s+1, a, 9) ||
        first!=u_strstr(s+1, sub_a) ||
        first!=u_strFindFirst(s+1, -1, sub_a, -1) ||
        first!=u_strFindFirst(s+1, -1, &a, 1) ||
        first!=u_strFindFirst(s+1, 9, sub_a, -1) ||
        first!=u_strFindFirst(s+1, 9, &a, 1) ||
        (s+10)!=u_strrchr(s+1, a) ||
        (s+10)!=u_strrchr32(s+1, a) ||
        last!=u_memrchr(s+1, a, 9) ||
        last!=u_memrchr32(s+1, a, 9) ||
        (s+10)!=u_strrstr(s+1, sub_a) ||
        (s+10)!=u_strFindLast(s+1, -1, sub_a, -1) ||
        (s+10)!=u_strFindLast(s+1, -1, &a, 1) ||
        last!=u_strFindLast(s+1, 9, sub_a, -1) ||
        last!=u_strFindLast(s+1, 9, &a, 1)
    ) {
        log_err("error: one of the u_str[chr etc]('a') does not find the correct place\n");
    }

    /* do not find 'b' in s[1..10[ */
    if(
        NULL!=u_strchr(s+1, b) ||
        NULL!=u_strchr32(s+1, b) ||
        NULL!=u_memchr(s+1, b, 9) ||
        NULL!=u_memchr32(s+1, b, 9) ||
        NULL!=u_strstr(s+1, sub_b) ||
        NULL!=u_strFindFirst(s+1, -1, sub_b, -1) ||
        NULL!=u_strFindFirst(s+1, -1, &b, 1) ||
        NULL!=u_strFindFirst(s+1, 9, sub_b, -1) ||
        NULL!=u_strFindFirst(s+1, 9, &b, 1) ||
        NULL!=u_strrchr(s+1, b) ||
        NULL!=u_strrchr32(s+1, b) ||
        NULL!=u_memrchr(s+1, b, 9) ||
        NULL!=u_memrchr32(s+1, b, 9) ||
        NULL!=u_strrstr(s+1, sub_b) ||
        NULL!=u_strFindLast(s+1, -1, sub_b, -1) ||
        NULL!=u_strFindLast(s+1, -1, &b, 1) ||
        NULL!=u_strFindLast(s+1, 9, sub_b, -1) ||
        NULL!=u_strFindLast(s+1, 9, &b, 1)
    ) {
        log_err("error: one of the u_str[chr etc]('b') incorrectly finds something\n");
    }

    /* do not find a non-code point in s[1..10[ */
    if(
        NULL!=u_strchr32(s+1, ill) ||
        NULL!=u_memchr32(s+1, ill, 9) ||
        NULL!=u_strrchr32(s+1, ill) ||
        NULL!=u_memrchr32(s+1, ill, 9)
    ) {
        log_err("error: one of the u_str[chr etc](illegal code point) incorrectly finds something\n");
    }

    /* find U+d801 in s[1..10[ */
    first=s+6;
    if(
        first!=u_strchr(s+1, lead) ||
        first!=u_strchr32(s+1, lead) ||
        first!=u_memchr(s+1, lead, 9) ||
        first!=u_memchr32(s+1, lead, 9) ||
        first!=u_strstr(s+1, sub_lead) ||
        first!=u_strFindFirst(s+1, -1, sub_lead, -1) ||
        first!=u_strFindFirst(s+1, -1, &lead, 1) ||
        first!=u_strFindFirst(s+1, 9, sub_lead, -1) ||
        first!=u_strFindFirst(s+1, 9, &lead, 1) ||
        first!=u_strrchr(s+1, lead) ||
        first!=u_strrchr32(s+1, lead) ||
        first!=u_memrchr(s+1, lead, 9) ||
        first!=u_memrchr32(s+1, lead, 9) ||
        first!=u_strrstr(s+1, sub_lead) ||
        first!=u_strFindLast(s+1, -1, sub_lead, -1) ||
        first!=u_strFindLast(s+1, -1, &lead, 1) ||
        first!=u_strFindLast(s+1, 9, sub_lead, -1) ||
        first!=u_strFindLast(s+1, 9, &lead, 1)
    ) {
        log_err("error: one of the u_str[chr etc](U+d801) does not find the correct place\n");
    }

    /* find U+dc02 in s[1..10[ */
    first=s+4;
    if(
        first!=u_strchr(s+1, trail) ||
        first!=u_strchr32(s+1, trail) ||
        first!=u_memchr(s+1, trail, 9) ||
        first!=u_memchr32(s+1, trail, 9) ||
        first!=u_strstr(s+1, sub_trail) ||
        first!=u_strFindFirst(s+1, -1, sub_trail, -1) ||
        first!=u_strFindFirst(s+1, -1, &trail, 1) ||
        first!=u_strFindFirst(s+1, 9, sub_trail, -1) ||
        first!=u_strFindFirst(s+1, 9, &trail, 1) ||
        first!=u_strrchr(s+1, trail) ||
        first!=u_strrchr32(s+1, trail) ||
        first!=u_memrchr(s+1, trail, 9) ||
        first!=u_memrchr32(s+1, trail, 9) ||
        first!=u_strrstr(s+1, sub_trail) ||
        first!=u_strFindLast(s+1, -1, sub_trail, -1) ||
        first!=u_strFindLast(s+1, -1, &trail, 1) ||
        first!=u_strFindLast(s+1, 9, sub_trail, -1) ||
        first!=u_strFindLast(s+1, 9, &trail, 1)
    ) {
        log_err("error: one of the u_str[chr etc](U+dc02) does not find the correct place\n");
    }

    /* find U+10402 in s[1..10[ */
    first=s+1;
    last=s+8;
    if(
        first!=u_strchr32(s+1, supp) ||
        first!=u_memchr32(s+1, supp, 9) ||
        first!=u_strstr(s+1, sub_supp) ||
        first!=u_strFindFirst(s+1, -1, sub_supp, -1) ||
        first!=u_strFindFirst(s+1, -1, sub_supp, 2) ||
        first!=u_strFindFirst(s+1, 9, sub_supp, -1) ||
        first!=u_strFindFirst(s+1, 9, sub_supp, 2) ||
        last!=u_strrchr32(s+1, supp) ||
        last!=u_memrchr32(s+1, supp, 9) ||
        last!=u_strrstr(s+1, sub_supp) ||
        last!=u_strFindLast(s+1, -1, sub_supp, -1) ||
        last!=u_strFindLast(s+1, -1, sub_supp, 2) ||
        last!=u_strFindLast(s+1, 9, sub_supp, -1) ||
        last!=u_strFindLast(s+1, 9, sub_supp, 2)
    ) {
        log_err("error: one of the u_str[chr etc](U+10402) does not find the correct place\n");
    }

    /* do not find U+10402 in a single UChar */
    if(
        NULL!=u_memchr32(s+1, supp, 1) ||
        NULL!=u_strFindFirst(s+1, 1, sub_supp, -1) ||
        NULL!=u_strFindFirst(s+1, 1, sub_supp, 2) ||
        NULL!=u_memrchr32(s+1, supp, 1) ||
        NULL!=u_strFindLast(s+1, 1, sub_supp, -1) ||
        NULL!=u_strFindLast(s+1, 1, sub_supp, 2) ||
        NULL!=u_memrchr32(s+2, supp, 1) ||
        NULL!=u_strFindLast(s+2, 1, sub_supp, -1) ||
        NULL!=u_strFindLast(s+2, 1, sub_supp, 2)
    ) {
        log_err("error: one of the u_str[chr etc](U+10402) incorrectly finds a supplementary c.p. in a single UChar\n");
    }

    /* do not find U+10403 in s[1..10[ */
    if(
        NULL!=u_strchr32(s+1, supp2) ||
        NULL!=u_memchr32(s+1, supp2, 9) ||
        NULL!=u_strstr(s+1, sub_supp2) ||
        NULL!=u_strFindFirst(s+1, -1, sub_supp2, -1) ||
        NULL!=u_strFindFirst(s+1, -1, sub_supp2, 2) ||
        NULL!=u_strFindFirst(s+1, 9, sub_supp2, -1) ||
        NULL!=u_strFindFirst(s+1, 9, sub_supp2, 2) ||
        NULL!=u_strrchr32(s+1, supp2) ||
        NULL!=u_memrchr32(s+1, supp2, 9) ||
        NULL!=u_strrstr(s+1, sub_supp2) ||
        NULL!=u_strFindLast(s+1, -1, sub_supp2, -1) ||
        NULL!=u_strFindLast(s+1, -1, sub_supp2, 2) ||
        NULL!=u_strFindLast(s+1, 9, sub_supp2, -1) ||
        NULL!=u_strFindLast(s+1, 9, sub_supp2, 2)
    ) {
        log_err("error: one of the u_str[chr etc](U+10403) incorrectly finds something\n");
    }

    /* find <0061 d801> in s[1..10[ */
    first=s+5;
    if(
        first!=u_strstr(s+1, sub_a_lead) ||
        first!=u_strFindFirst(s+1, -1, sub_a_lead, -1) ||
        first!=u_strFindFirst(s+1, -1, sub_a_lead, 2) ||
        first!=u_strFindFirst(s+1, 9, sub_a_lead, -1) ||
        first!=u_strFindFirst(s+1, 9, sub_a_lead, 2) ||
        first!=u_strrstr(s+1, sub_a_lead) ||
        first!=u_strFindLast(s+1, -1, sub_a_lead, -1) ||
        first!=u_strFindLast(s+1, -1, sub_a_lead, 2) ||
        first!=u_strFindLast(s+1, 9, sub_a_lead, -1) ||
        first!=u_strFindLast(s+1, 9, sub_a_lead, 2)
    ) {
        log_err("error: one of the u_str[str etc](<0061 d801>) does not find the correct place\n");
    }

    /* find <dc02 0061> in s[1..10[ */
    first=s+4;
    if(
        first!=u_strstr(s+1, sub_trail_a) ||
        first!=u_strFindFirst(s+1, -1, sub_trail_a, -1) ||
        first!=u_strFindFirst(s+1, -1, sub_trail_a, 2) ||
        first!=u_strFindFirst(s+1, 9, sub_trail_a, -1) ||
        first!=u_strFindFirst(s+1, 9, sub_trail_a, 2) ||
        first!=u_strrstr(s+1, sub_trail_a) ||
        first!=u_strFindLast(s+1, -1, sub_trail_a, -1) ||
        first!=u_strFindLast(s+1, -1, sub_trail_a, 2) ||
        first!=u_strFindLast(s+1, 9, sub_trail_a, -1) ||
        first!=u_strFindLast(s+1, 9, sub_trail_a, 2)
    ) {
        log_err("error: one of the u_str[str etc](<dc02 0061>) does not find the correct place\n");
    }

    /* do not find "aba" in s[1..10[ */
    if(
        NULL!=u_strstr(s+1, sub_aba) ||
        NULL!=u_strFindFirst(s+1, -1, sub_aba, -1) ||
        NULL!=u_strFindFirst(s+1, -1, sub_aba, 3) ||
        NULL!=u_strFindFirst(s+1, 9, sub_aba, -1) ||
        NULL!=u_strFindFirst(s+1, 9, sub_aba, 3) ||
        NULL!=u_strrstr(s+1, sub_aba) ||
        NULL!=u_strFindLast(s+1, -1, sub_aba, -1) ||
        NULL!=u_strFindLast(s+1, -1, sub_aba, 3) ||
        NULL!=u_strFindLast(s+1, 9, sub_aba, -1) ||
        NULL!=u_strFindLast(s+1, 9, sub_aba, 3)
    ) {
        log_err("error: one of the u_str[str etc](\"aba\") incorrectly finds something\n");
    }
}

static void TestStringCopy()
{
    UChar temp[40];
    UChar *result=0;
    UChar subString[5];
    UChar uchars[]={0x61, 0x62, 0x63, 0x00};
    char  charOut[40];
    char  chars[]="abc";    /* needs default codepage */

    log_verbose("Testing u_uastrncpy() and u_uastrcpy()");

    u_uastrcpy(temp, "abc");
    if(u_strcmp(temp, uchars) != 0) {
        log_err("There is an error in u_uastrcpy() Expected %s Got %s\n", austrdup(uchars), austrdup(temp));
    }

    temp[0] = 0xFB; /* load garbage into it */
    temp[1] = 0xFB;
    temp[2] = 0xFB;
    temp[3] = 0xFB;

    u_uastrncpy(temp, "abcabcabc", 3);
    if(u_strncmp(uchars, temp, 3) != 0){
        log_err("There is an error in u_uastrncpy() Expected %s Got %s\n", austrdup(uchars), austrdup(temp));
    }
    if(temp[3] != 0xFB) {
        log_err("u_uastrncpy wrote past it's bounds. Expected undisturbed byte at 3\n");
    }

    charOut[0] = (char)0x7B; /* load garbage into it */
    charOut[1] = (char)0x7B;
    charOut[2] = (char)0x7B;
    charOut[3] = (char)0x7B;

    temp[0] = 0x0061;
    temp[1] = 0x0062;
    temp[2] = 0x0063;
    temp[3] = 0x0061;
    temp[4] = 0x0062;
    temp[5] = 0x0063;
    temp[6] = 0x0000;

    u_austrncpy(charOut, temp, 3);
    if(strncmp(chars, charOut, 3) != 0){
        log_err("There is an error in u_austrncpy() Expected %s Got %s\n", austrdup(uchars), austrdup(temp));
    }
    if(charOut[3] != (char)0x7B) {
        log_err("u_austrncpy wrote past it's bounds. Expected undisturbed byte at 3\n");
    }

    /*Testing u_strchr()*/
    log_verbose("Testing u_strchr\n");
    temp[0]=0x42;
    temp[1]=0x62;
    temp[2]=0x62;
    temp[3]=0x63;
    temp[4]=0xd841;
    temp[5]=0xd841;
    temp[6]=0xdc02;
    temp[7]=0;
    result=u_strchr(temp, (UChar)0x62);
    if(result != temp+1){
        log_err("There is an error in u_strchr() Expected match at position 1 Got %ld (pointer 0x%lx)\n", result-temp, result);
    }
    /*Testing u_strstr()*/
    log_verbose("Testing u_strstr\n");
    subString[0]=0x62;
    subString[1]=0x63;
    subString[2]=0;
    result=u_strstr(temp, subString);
    if(result != temp+2){
        log_err("There is an error in u_strstr() Expected match at position 2 Got %ld (pointer 0x%lx)\n", result-temp, result);
    }
    result=u_strstr(temp, subString+2); /* subString+2 is an empty string */
    if(result != temp){
        log_err("There is an error in u_strstr() Expected match at position 0 Got %ld (pointer 0x%lx)\n", result-temp, result);
    }
    result=u_strstr(subString, temp);
    if(result != NULL){
        log_err("There is an error in u_strstr() Expected NULL \"not found\" Got non-NULL \"found\" result\n");
    }

    /*Testing u_strchr32*/
    log_verbose("Testing u_strchr32\n");
    result=u_strchr32(temp, (UChar32)0x62);
    if(result != temp+1){
        log_err("There is an error in u_strchr32() Expected match at position 1 Got %ld (pointer 0x%lx)\n", result-temp, result);
    }
    result=u_strchr32(temp, (UChar32)0xfb);
    if(result != NULL){
        log_err("There is an error in u_strchr32() Expected NULL \"not found\" Got non-NULL \"found\" result\n");
    }
    result=u_strchr32(temp, (UChar32)0x20402);
    if(result != temp+5){
        log_err("There is an error in u_strchr32() Expected match at position 5 Got %ld (pointer 0x%lx)\n", result-temp, result);
    }

    temp[7]=0xfc00;
    result=u_memchr32(temp, (UChar32)0x20402, 7);
    if(result != temp+5){
        log_err("There is an error in u_memchr32() Expected match at position 5 Got %ld (pointer 0x%lx)\n", result-temp, result);
    }
    result=u_memchr32(temp, (UChar32)0x20402, 6);
    if(result != NULL){
        log_err("There is an error in u_memchr32() Expected no match Got %ld (pointer 0x%lx)\n", result-temp, result);
    }
    result=u_memchr32(temp, (UChar32)0x20402, 1);
    if(result != NULL){
        log_err("There is an error in u_memchr32() Expected no match Got %ld (pointer 0x%lx)\n", result-temp, result);
    }
    result=u_memchr32(temp, (UChar32)0xfc00, 8);
    if(result != temp+7){
        log_err("There is an error in u_memchr32() Expected match at position 7 Got %ld (pointer 0x%lx)\n", result-temp, result);
    }
}

/* test u_unescape() and u_unescapeAt() ------------------------------------- */

static void
TestUnescape() {
    static UChar buffer[200];
    
    static const char* input =
        "Sch\\u00f6nes Auto: \\u20ac 11240.\\fPrivates Zeichen: \\U00102345\\e\\cC\\n \\x1b\\x{263a}";

    static const UChar expect[]={
        0x53, 0x63, 0x68, 0xf6, 0x6e, 0x65, 0x73, 0x20, 0x41, 0x75, 0x74, 0x6f, 0x3a, 0x20,
        0x20ac, 0x20, 0x31, 0x31, 0x32, 0x34, 0x30, 0x2e, 0x0c,
        0x50, 0x72, 0x69, 0x76, 0x61, 0x74, 0x65, 0x73, 0x20,
        0x5a, 0x65, 0x69, 0x63, 0x68, 0x65, 0x6e, 0x3a, 0x20, 0xdbc8, 0xdf45, 0x1b, 0x03, 0x0a, 0x20, 0x1b, 0x263A, 0
    };
    static const int32_t explength = sizeof(expect)/sizeof(expect[0])-1;
    int32_t length;

    /* test u_unescape() */
    length=u_unescape(input, buffer, sizeof(buffer)/sizeof(buffer[0]));
    if(length!=explength || u_strcmp(buffer, expect)!=0) {
        log_err("failure in u_unescape(): length %d!=%d and/or incorrect result string\n", length,
                explength);
    }

    /* try preflighting */
    length=u_unescape(input, NULL, sizeof(buffer)/sizeof(buffer[0]));
    if(length!=explength || u_strcmp(buffer, expect)!=0) {
        log_err("failure in u_unescape(preflighting): length %d!=%d\n", length, explength);
    }

    /* ### TODO: test u_unescapeAt() */
}

/* test code point counting functions --------------------------------------- */

/* reference implementation of u_strHasMoreChar32Than() */
static int32_t
_refStrHasMoreChar32Than(const UChar *s, int32_t length, int32_t number) {
    int32_t count=u_countChar32(s, length);
    return count>number;
}

/* compare the real function against the reference */
static void
_testStrHasMoreChar32Than(const UChar *s, int32_t i, int32_t length, int32_t number) {
    if(u_strHasMoreChar32Than(s, length, number)!=_refStrHasMoreChar32Than(s, length, number)) {
        log_err("u_strHasMoreChar32Than(s+%d, %d, %d)=%hd is wrong\n",
                i, length, number, u_strHasMoreChar32Than(s, length, number));
    }
}

static void
TestCountChar32() {
    static const UChar string[]={
        0x61, 0x62, 0xd800, 0xdc00,
        0xd801, 0xdc01, 0x63, 0xd802,
        0x64, 0xdc03, 0x65, 0x66,
        0xd804, 0xdc04, 0xd805, 0xdc05,
        0x67
    };
    UChar buffer[100];
    int32_t i, length, number;

    /* test u_strHasMoreChar32Than() with length>=0 */
    length=LENGTHOF(string);
    while(length>=0) {
        for(i=0; i<=length; ++i) {
            for(number=-1; number<=((length-i)+2); ++number) {
                _testStrHasMoreChar32Than(string+i, i, length-i, number);
            }
        }
        --length;
    }

    /* test u_strHasMoreChar32Than() with NUL-termination (length=-1) */
    length=LENGTHOF(string);
    u_memcpy(buffer, string, length);
    while(length>=0) {
        buffer[length]=0;
        for(i=0; i<=length; ++i) {
            for(number=-1; number<=((length-i)+2); ++number) {
                _testStrHasMoreChar32Than(string+i, i, -1, number);
            }
        }
        --length;
    }

    /* test u_strHasMoreChar32Than() with NULL string (bad input) */
    for(length=-1; length<=1; ++length) {
        for(i=0; i<=length; ++i) {
            for(number=-2; number<=2; ++number) {
                _testStrHasMoreChar32Than(NULL, 0, length, number);
            }
        }
    }
}

/* UCharIterator ------------------------------------------------------------ */

/*
 * Compare results from two iterators, should be same.
 * Assume that the text is not empty and that
 * iteration start==0 and iteration limit==length.
 */
static void
compareIterators(UCharIterator *iter1, const char *n1,
                 UCharIterator *iter2, const char *n2) {
    int32_t i, pos1, pos2, middle, length;
    UChar32 c1, c2;

    /* compare lengths */
    length=iter1->getIndex(iter1, UITER_LENGTH);
    pos2=iter2->getIndex(iter2, UITER_LENGTH);
    if(length!=pos2) {
        log_err("%s->getIndex(length)=%d != %d=%s->getIndex(length)\n", n1, length, pos2, n2);
        return;
    }

    /* set into the middle */
    middle=length/2;

    pos1=iter1->move(iter1, middle, UITER_ZERO);
    if(pos1!=middle) {
        log_err("%s->move(from 0 to middle %d)=%d does not move to the middle\n", n1, middle, pos1);
        return;
    }

    pos2=iter2->move(iter2, middle, UITER_ZERO);
    if(pos2!=middle) {
        log_err("%s->move(from 0 to middle %d)=%d does not move to the middle\n", n2, middle, pos2);
        return;
    }

    /* test current() */
    c1=iter1->current(iter1);
    c2=iter2->current(iter2);
    if(c1!=c2) {
        log_err("%s->current()=U+%04x != U+%04x=%s->current() at middle=%d\n", n1, c1, c2, n2, middle);
        return;
    }

    /* move forward 3 UChars */
    for(i=0; i<3; ++i) {
        c1=iter1->next(iter1);
        c2=iter2->next(iter2);
        if(c1!=c2) {
            log_err("%s->next()=U+%04x != U+%04x=%s->next() at %d (started in middle)\n", n1, c1, c2, n2, iter1->getIndex(iter1, UITER_CURRENT));
            return;
        }
    }

    /* move backward 5 UChars */
    for(i=0; i<5; ++i) {
        c1=iter1->previous(iter1);
        c2=iter2->previous(iter2);
        if(c1!=c2) {
            log_err("%s->previous()=U+%04x != U+%04x=%s->previous() at %d (started in middle)\n", n1, c1, c2, n2, iter1->getIndex(iter1, UITER_CURRENT));
            return;
        }
    }

    /* iterate forward from the beginning */
    pos1=iter1->move(iter1, 0, UITER_START);
    if(pos1<0) {
        log_err("%s->move(start) failed\n", n1);
        return;
    }
    if(!iter1->hasNext(iter1)) {
        log_err("%s->hasNext() at the start returns FALSE\n", n1);
        return;
    }

    pos2=iter2->move(iter2, 0, UITER_START);
    if(pos2<0) {
        log_err("%s->move(start) failed\n", n2);
        return;
    }
    if(!iter2->hasNext(iter2)) {
        log_err("%s->hasNext() at the start returns FALSE\n", n2);
        return;
    }

    do {
        c1=iter1->next(iter1);
        c2=iter2->next(iter2);
        if(c1!=c2) {
            log_err("%s->next()=U+%04x != U+%04x=%s->next() at %d\n", n1, c1, c2, n2, iter1->getIndex(iter1, UITER_CURRENT));
            return;
        }
    } while(c1>=0);

    if(iter1->hasNext(iter1)) {
        log_err("%s->hasNext() at the end returns TRUE\n", n1);
        return;
    }
    if(iter2->hasNext(iter2)) {
        log_err("%s->hasNext() at the end returns TRUE\n", n2);
        return;
    }

    /* back to the middle */
    pos1=iter1->move(iter1, middle, UITER_ZERO);
    if(pos1!=middle) {
        log_err("%s->move(from end to middle %d)=%d does not move to the middle\n", n1, middle, pos1);
        return;
    }

    pos2=iter2->move(iter2, middle, UITER_ZERO);
    if(pos2!=middle) {
        log_err("%s->move(from end to middle %d)=%d does not move to the middle\n", n2, middle, pos2);
        return;
    }

    /* move to index 1 */
    pos1=iter1->move(iter1, 1, UITER_ZERO);
    if(pos1!=1) {
        log_err("%s->move(from middle %d to 1)=%d does not move to 1\n", n1, middle, pos1);
        return;
    }

    pos2=iter2->move(iter2, 1, UITER_ZERO);
    if(pos2!=1) {
        log_err("%s->move(from middle %d to 1)=%d does not move to 1\n", n2, middle, pos2);
        return;
    }

    /* iterate backward from the end */
    pos1=iter1->move(iter1, 0, UITER_LIMIT);
    if(pos1<0) {
        log_err("%s->move(limit) failed\n", n1);
        return;
    }
    if(!iter1->hasPrevious(iter1)) {
        log_err("%s->hasPrevious() at the end returns FALSE\n", n1);
        return;
    }

    pos2=iter2->move(iter2, 0, UITER_LIMIT);
    if(pos2<0) {
        log_err("%s->move(limit) failed\n", n2);
        return;
    }
    if(!iter2->hasPrevious(iter2)) {
        log_err("%s->hasPrevious() at the end returns FALSE\n", n2);
        return;
    }

    do {
        c1=iter1->previous(iter1);
        c2=iter2->previous(iter2);
        if(c1!=c2) {
            log_err("%s->previous()=U+%04x != U+%04x=%s->previous() at %d\n", n1, c1, c2, n2, iter1->getIndex(iter1, UITER_CURRENT));
            return;
        }
    } while(c1>=0);

    if(iter1->hasPrevious(iter1)) {
        log_err("%s->hasPrevious() at the start returns TRUE\n", n1);
        return;
    }
    if(iter2->hasPrevious(iter2)) {
        log_err("%s->hasPrevious() at the start returns TRUE\n", n2);
        return;
    }
}

/*
 * Test the iterator's getState() and setState() functions.
 * iter1 and iter2 must be set up for the same iterator type and the same string
 * but may be physically different structs (different addresses).
 *
 * Assume that the text is not empty and that
 * iteration start==0 and iteration limit==length.
 * It must be 2<=middle<=length-2.
 */
static void
testIteratorState(UCharIterator *iter1, UCharIterator *iter2, const char *n, int32_t middle) {
    UChar32 u[4];

    UErrorCode errorCode;
    UChar32 c;
    uint32_t state;
    int32_t i, j;

    /* get four UChars from the middle of the string */
    iter1->move(iter1, middle-2, UITER_ZERO);
    for(i=0; i<4; ++i) {
        c=iter1->next(iter1);
        if(c<0) {
            /* the test violates the assumptions, see comment above */
            log_err("test error: %s[%d]=%d\n", n, middle-2+i, c);
            return;
        }
        u[i]=c;
    }

    /* move to the middle and get the state */
    iter1->move(iter1, -2, UITER_CURRENT);
    state=uiter_getState(iter1);

    /* set the state into the second iterator and compare the results */
    errorCode=U_ZERO_ERROR;
    uiter_setState(iter2, state, &errorCode);
    if(U_FAILURE(errorCode)) {
        log_err("%s->setState(0x%x) failed: %s\n", n, state, u_errorName(errorCode));
        return;
    }

    c=iter2->current(iter2);
    if(c!=u[2]) {
        log_err("%s->current(at %d)=U+%04x!=U+%04x\n", n, middle, c, u[2]);
    }

    c=iter2->previous(iter2);
    if(c!=u[1]) {
        log_err("%s->previous(at %d)=U+%04x!=U+%04x\n", n, middle-1, c, u[1]);
    }

    iter2->move(iter2, 2, UITER_CURRENT);
    c=iter2->next(iter2);
    if(c!=u[3]) {
        log_err("%s->next(at %d)=U+%04x!=U+%04x\n", n, middle+1, c, u[3]);
    }

    iter2->move(iter2, -3, UITER_CURRENT);
    c=iter2->previous(iter2);
    if(c!=u[0]) {
        log_err("%s->previous(at %d)=U+%04x!=U+%04x\n", n, middle-2, c, u[0]);
    }

    /* move the second iterator back to the middle */
    iter2->move(iter2, 1, UITER_CURRENT);
    iter2->next(iter2);

    /* check that both are in the middle */
    i=iter1->getIndex(iter1, UITER_CURRENT);
    j=iter2->getIndex(iter2, UITER_CURRENT);
    if(i!=middle) {
        log_err("%s->getIndex(current)=%d!=%d as expected\n", n, i, middle);
    }
    if(i!=j) {
        log_err("%s->getIndex(current)=%d!=%d after setState()\n", n, j, i);
    }

    /* compare lengths */
    i=iter1->getIndex(iter1, UITER_LENGTH);
    j=iter2->getIndex(iter2, UITER_LENGTH);
    if(i!=j) {
        log_err("%s->getIndex(length)=%d!=%d before/after setState()\n", n, i, j);
    }
}

static void
TestUCharIterator() {
    static const UChar text[]={
        0x61, 0x62, 0x63, 0xd801, 0xdffd, 0x78, 0x79, 0x7a, 0
    };
    char bytes[40];

    UCharIterator iter, iter1, iter2;
    UConverter *cnv;
    UErrorCode errorCode;
    int32_t length;

    /* simple API/code coverage - test NOOP UCharIterator */
    uiter_setString(&iter, NULL, 0);
    if( iter.current(&iter)!=-1 || iter.next(&iter)!=-1 || iter.previous(&iter)!=-1 ||
        iter.move(&iter, 1, UITER_CURRENT) || iter.getIndex(&iter, UITER_CURRENT)!=0 ||
        iter.hasNext(&iter) || iter.hasPrevious(&iter)
    ) {
        log_err("NOOP UCharIterator behaves unexpectedly\n");
    }

    /* test get/set state */
    length=LENGTHOF(text)-1;
    uiter_setString(&iter1, text, -1);
    uiter_setString(&iter2, text, length);
    testIteratorState(&iter1, &iter2, "UTF16IteratorState", length/2);
    testIteratorState(&iter1, &iter2, "UTF16IteratorStatePlus1", length/2+1);

    /* compare the same string between UTF-16 and UTF-8 UCharIterators ------ */
    errorCode=U_ZERO_ERROR;
    u_strToUTF8(bytes, sizeof(bytes), &length, text, -1, &errorCode);
    if(U_FAILURE(errorCode)) {
        log_err("u_strToUTF8() failed, %s\n", u_errorName(errorCode));
        return;
    }

    uiter_setString(&iter1, text, -1);
    uiter_setUTF8(&iter2, bytes, length);
    compareIterators(&iter1, "UTF16Iterator", &iter2, "UTF8Iterator");

    /* try again with length=-1 */
    uiter_setUTF8(&iter2, bytes, -1);
    compareIterators(&iter1, "UTF16Iterator", &iter2, "UTF8Iterator_1");

    /* test get/set state */
    length=LENGTHOF(text)-1;
    uiter_setUTF8(&iter1, bytes, -1);
    testIteratorState(&iter1, &iter2, "UTF8IteratorState", length/2);
    testIteratorState(&iter1, &iter2, "UTF8IteratorStatePlus1", length/2+1);

    /* compare the same string between UTF-16 and UTF-16BE UCharIterators --- */
    errorCode=U_ZERO_ERROR;
    cnv=ucnv_open("UTF-16BE", &errorCode);
    length=ucnv_fromUChars(cnv, bytes, sizeof(bytes), text, -1, &errorCode);
    ucnv_close(cnv);
    if(U_FAILURE(errorCode)) {
        log_err("ucnv_fromUChars(UTF-16BE) failed, %s\n", u_errorName(errorCode));
        return;
    }

    /* terminate with a _pair_ of 0 bytes - a UChar NUL in UTF-16BE (length is known to be ok) */
    bytes[length]=bytes[length+1]=0;

    uiter_setString(&iter1, text, -1);
    uiter_setUTF16BE(&iter2, bytes, length);
    compareIterators(&iter1, "UTF16Iterator", &iter2, "UTF16BEIterator");

    /* try again with length=-1 */
    uiter_setUTF16BE(&iter2, bytes, -1);
    compareIterators(&iter1, "UTF16Iterator", &iter2, "UTF16BEIterator_1");

    /* try again after moving the bytes up one, and with length=-1 */
    memmove(bytes+1, bytes, length+2);
    uiter_setUTF16BE(&iter2, bytes+1, -1);
    compareIterators(&iter1, "UTF16Iterator", &iter2, "UTF16BEIteratorMoved1");

    /* ### TODO test other iterators: CharacterIterator, Replaceable */
}

#if UCONFIG_NO_COLLATION

static void
TestUNormIterator() {
    /* test nothing */
}

static void
TestBadUNormIterator(void) {
    /* test nothing, as well */
}

#else

#include "unicode/unorm.h"
#include "unorm_it.h"

/*
 * Compare results from two iterators, should be same.
 * Assume that the text is not empty and that
 * iteration start==0 and iteration limit==length.
 *
 * Modified version of compareIterators() but does not assume that indexes
 * are available.
 */
static void
compareIterNoIndexes(UCharIterator *iter1, const char *n1,
                     UCharIterator *iter2, const char *n2,
                     int32_t middle) {
    uint32_t state;
    int32_t i;
    UChar32 c1, c2;
    UErrorCode errorCode;

    /* code coverage for unorm_it.c/unormIteratorGetIndex() */
    if(
        iter2->getIndex(iter2, UITER_START)!=0 ||
        iter2->getIndex(iter2, UITER_LENGTH)!=UITER_UNKNOWN_INDEX
    ) {
        log_err("UNormIterator.getIndex() failed\n");
    }

    /* set into the middle */
    iter1->move(iter1, middle, UITER_ZERO);
    iter2->move(iter2, middle, UITER_ZERO);

    /* test current() */
    c1=iter1->current(iter1);
    c2=iter2->current(iter2);
    if(c1!=c2) {
        log_err("%s->current()=U+%04x != U+%04x=%s->current() at middle=%d\n", n1, c1, c2, n2, middle);
        return;
    }

    /* move forward 3 UChars */
    for(i=0; i<3; ++i) {
        c1=iter1->next(iter1);
        c2=iter2->next(iter2);
        if(c1!=c2) {
            log_err("%s->next()=U+%04x != U+%04x=%s->next() at %d (started in middle)\n", n1, c1, c2, n2, iter1->getIndex(iter1, UITER_CURRENT));
            return;
        }
    }

    /* move backward 5 UChars */
    for(i=0; i<5; ++i) {
        c1=iter1->previous(iter1);
        c2=iter2->previous(iter2);
        if(c1!=c2) {
            log_err("%s->previous()=U+%04x != U+%04x=%s->previous() at %d (started in middle)\n", n1, c1, c2, n2, iter1->getIndex(iter1, UITER_CURRENT));
            return;
        }
    }

    /* iterate forward from the beginning */
    iter1->move(iter1, 0, UITER_START);
    if(!iter1->hasNext(iter1)) {
        log_err("%s->hasNext() at the start returns FALSE\n", n1);
        return;
    }

    iter2->move(iter2, 0, UITER_START);
    if(!iter2->hasNext(iter2)) {
        log_err("%s->hasNext() at the start returns FALSE\n", n2);
        return;
    }

    do {
        c1=iter1->next(iter1);
        c2=iter2->next(iter2);
        if(c1!=c2) {
            log_err("%s->next()=U+%04x != U+%04x=%s->next() at %d\n", n1, c1, c2, n2, iter1->getIndex(iter1, UITER_CURRENT));
            return;
        }
    } while(c1>=0);

    if(iter1->hasNext(iter1)) {
        log_err("%s->hasNext() at the end returns TRUE\n", n1);
        return;
    }
    if(iter2->hasNext(iter2)) {
        log_err("%s->hasNext() at the end returns TRUE\n", n2);
        return;
    }

    /* iterate backward */
    do {
        c1=iter1->previous(iter1);
        c2=iter2->previous(iter2);
        if(c1!=c2) {
            log_err("%s->previous()=U+%04x != U+%04x=%s->previous() at %d\n", n1, c1, c2, n2, iter1->getIndex(iter1, UITER_CURRENT));
            return;
        }
    } while(c1>=0);

    /* back to the middle */
    iter1->move(iter1, middle, UITER_ZERO);
    iter2->move(iter2, middle, UITER_ZERO);

    /* try get/set state */
    while((state=uiter_getState(iter2))==UITER_NO_STATE) {
        if(!iter2->hasNext(iter2)) {
            log_err("%s has no known state from middle=%d to the end\n", n2, middle);
            return;
        }
        iter2->next(iter2);
    }

    errorCode=U_ZERO_ERROR;

    c2=iter2->current(iter2);
    iter2->move(iter2, 0, UITER_ZERO);
    uiter_setState(iter2, state, &errorCode);
    c1=iter2->current(iter2);
    if(U_FAILURE(errorCode) || c1!=c2) {
        log_err("%s->current() differs across get/set state, U+%04x vs. U+%04x\n", n2, c2, c1);
        return;
    }

    c2=iter2->previous(iter2);
    iter2->move(iter2, 0, UITER_ZERO);
    uiter_setState(iter2, state, &errorCode);
    c1=iter2->previous(iter2);
    if(U_FAILURE(errorCode) || c1!=c2) {
        log_err("%s->previous() differs across get/set state, U+%04x vs. U+%04x\n", n2, c2, c1);
        return;
    }

    /* iterate backward from the end */
    iter1->move(iter1, 0, UITER_LIMIT);
    if(!iter1->hasPrevious(iter1)) {
        log_err("%s->hasPrevious() at the end returns FALSE\n", n1);
        return;
    }

    iter2->move(iter2, 0, UITER_LIMIT);
    if(!iter2->hasPrevious(iter2)) {
        log_err("%s->hasPrevious() at the end returns FALSE\n", n2);
        return;
    }

    do {
        c1=iter1->previous(iter1);
        c2=iter2->previous(iter2);
        if(c1!=c2) {
            log_err("%s->previous()=U+%04x != U+%04x=%s->previous() at %d\n", n1, c1, c2, n2, iter1->getIndex(iter1, UITER_CURRENT));
            return;
        }
    } while(c1>=0);

    if(iter1->hasPrevious(iter1)) {
        log_err("%s->hasPrevious() at the start returns TRUE\n", n1);
        return;
    }
    if(iter2->hasPrevious(iter2)) {
        log_err("%s->hasPrevious() at the start returns TRUE\n", n2);
        return;
    }
}

/* n2 must have a digit 1 at the end, will be incremented with the normalization mode */
static void
testUNormIteratorWithText(const UChar *text, int32_t textLength, int32_t middle,
                          const char *name1, const char *n2) {
    UChar buffer[600];
    char name2[40];

    UCharIterator iter1, iter2, *iter;
    UNormIterator *uni;

    UNormalizationMode mode;
    UErrorCode errorCode;
    int32_t length;

    /* open a normalizing iterator */
    errorCode=U_ZERO_ERROR;
    uni=unorm_openIter(NULL, 0, &errorCode);
    if(U_FAILURE(errorCode)) {
        log_err("unorm_openIter() fails: %s\n", u_errorName(errorCode));
        return;
    }

    /* set iterator 2 to the original text */
    uiter_setString(&iter2, text, textLength);

    strcpy(name2, n2);

    /* test the normalizing iterator for each mode */
    for(mode=UNORM_NONE; mode<UNORM_MODE_COUNT; ++mode) {
        length=unorm_normalize(text, textLength, mode, 0, buffer, LENGTHOF(buffer), &errorCode);
        if(U_FAILURE(errorCode)) {
            log_data_err("unorm_normalize(mode %d) failed: %s - (Are you missing data?)\n", mode, u_errorName(errorCode));
            break;
        }

        /* set iterator 1 to the normalized text  */
        uiter_setString(&iter1, buffer, length);

        /* set the normalizing iterator to use iter2 */
        iter=unorm_setIter(uni, &iter2, mode, &errorCode);
        if(U_FAILURE(errorCode)) {
            log_err("unorm_setIter(mode %d) failed: %s\n", mode, u_errorName(errorCode));
            break;
        }

        compareIterNoIndexes(&iter1, name1, iter, name2, middle);
        ++name2[strlen(name2)-1];
    }

    unorm_closeIter(uni);
}

static void
TestUNormIterator() {
    static const UChar text[]={ /* must contain <00C5 0327> see u_strchr() below */
        0x61,                                                   /* 'a' */
        0xe4, 0x61, 0x308,                                      /* variations of 'a'+umlaut */
        0xc5, 0x327, 0x41, 0x30a, 0x327, 0x41, 0x327, 0x30a,    /* variations of 'A'+ring+cedilla */
        0xfb03, 0xfb00, 0x69, 0x66, 0x66, 0x69, 0x66, 0xfb01    /* variations of 'ffi' */
    };
    static const UChar surrogateText[]={
        0x6e, 0xd900, 0x6a, 0xdc00, 0xd900, 0xdc00, 0x61
    };

    UChar longText[600];
    int32_t i, middle, length;

    length=LENGTHOF(text);
    testUNormIteratorWithText(text, length, length/2, "UCharIter", "UNormIter1");
    testUNormIteratorWithText(text, length, length, "UCharIterEnd", "UNormIterEnd1");

    /* test again, this time with an insane string to cause internal buffer overflows */
    middle=(int32_t)(u_strchr(text, 0x327)-text); /* see comment at text[] */
    memcpy(longText, text, middle*U_SIZEOF_UCHAR);
    for(i=0; i<150; ++i) {
        longText[middle+i]=0x30a; /* insert many rings between 'A-ring' and cedilla */
    }
    memcpy(longText+middle+i, text+middle, (LENGTHOF(text)-middle)*U_SIZEOF_UCHAR);
    length=LENGTHOF(text)+i;

    /* append another copy of this string for more overflows */
    memcpy(longText+length, longText, length*U_SIZEOF_UCHAR);
    length*=2;

    /* the first test of the following two starts at length/4, inside the sea of combining rings */
    testUNormIteratorWithText(longText, length, length/4, "UCharIterLong", "UNormIterLong1");
    testUNormIteratorWithText(longText, length, length, "UCharIterLongEnd", "UNormIterLongEnd1");

    length=LENGTHOF(surrogateText);
    testUNormIteratorWithText(surrogateText, length, length/4, "UCharIterSurr", "UNormIterSurr1");
    testUNormIteratorWithText(surrogateText, length, length, "UCharIterSurrEnd", "UNormIterSurrEnd1");
}

static void
TestBadUNormIterator(void) {
#if !UCONFIG_NO_NORMALIZATION
    UErrorCode status = U_ILLEGAL_ESCAPE_SEQUENCE;
    UNormIterator *uni;

    unorm_setIter(NULL, NULL, UNORM_NONE, &status);
    if (status != U_ILLEGAL_ESCAPE_SEQUENCE) {
        log_err("unorm_setIter changed the error code to: %s\n", u_errorName(status));
    }
    status = U_ZERO_ERROR;
    unorm_setIter(NULL, NULL, UNORM_NONE, &status);
    if (status != U_ILLEGAL_ARGUMENT_ERROR) {
        log_err("unorm_setIter didn't react correctly to bad arguments: %s\n", u_errorName(status));
    }
    status = U_ZERO_ERROR;
    uni=unorm_openIter(NULL, 0, &status);
    if(U_FAILURE(status)) {
        log_err("unorm_openIter() fails: %s\n", u_errorName(status));
        return;
    }
    unorm_setIter(uni, NULL, UNORM_NONE, &status);
    unorm_closeIter(uni);
#endif
}

#endif
