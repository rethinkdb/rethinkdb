/*
**********************************************************************
* Copyright (C) 1998-2010, International Business Machines Corporation 
* and others.  All Rights Reserved.
**********************************************************************
*
* File cstrtest.c
*
* Modification History:
*
*   Date        Name        Description
*   07/13/2000  Madhu         created
*******************************************************************************
*/

#include "unicode/ustring.h"
#include "unicode/ucnv.h"
#include "cstring.h"
#include "uinvchar.h"
#include "cintltst.h"
#include "cmemory.h"

#define LENGTHOF(array) (int32_t)(sizeof(array)/sizeof((array)[0]))

static void TestAPI(void);
void addCStringTest(TestNode** root);

static void TestInvariant(void);
static void TestCompareInvEbcdicAsAscii(void);

void addCStringTest(TestNode** root) {
    addTest(root, &TestAPI,   "tsutil/cstrtest/TestAPI");
    addTest(root, &TestInvariant,   "tsutil/cstrtest/TestInvariant");
    addTest(root, &TestCompareInvEbcdicAsAscii, "tsutil/cstrtest/TestCompareInvEbcdicAsAscii");
}

static void TestAPI(void)
{
    int32_t intValue=0;
    char src[30]="HELLO THERE", dest[30];
    static const char *const abc="abcdefghijklmnopqrstuvwxyz", *const ABC="ABCDEFGHIJKLMNOPQRSTUVWXYZ";
    const char *temp;
    int32_t i;

    log_verbose("Testing uprv_tolower() and uprv_toupper()\n");
    for(i=0; i<=26; ++i) {
        dest[i]=uprv_tolower(abc[i]);
    }
    if(0!=strcmp(abc, dest)) {
        log_err("uprv_tolower(abc) failed\n");
    }

    for(i=0; i<=26; ++i) {
        dest[i]=uprv_tolower(ABC[i]);
    }
    if(0!=strcmp(abc, dest)) {
        log_err("uprv_tolower(ABC) failed\n");
    }

    for(i=0; i<=26; ++i) {
        dest[i]=uprv_toupper(abc[i]);
    }
    if(0!=strcmp(ABC, dest)) {
        log_err("uprv_toupper(abc) failed\n");
    }

    for(i=0; i<=26; ++i) {
        dest[i]=uprv_toupper(ABC[i]);
    }
    if(0!=strcmp(ABC, dest)) {
        log_err("uprv_toupper(ABC) failed\n");
    }

    log_verbose("Testing the API in cstring\n");
    T_CString_toLowerCase(src);
    if(uprv_strcmp(src, "hello there") != 0){
        log_err("FAIL: *** T_CString_toLowerCase() failed. Expected: \"hello there\", Got: \"%s\"\n", src);
    }
    T_CString_toUpperCase(src);
    if(uprv_strcmp(src, "HELLO THERE") != 0){
        log_err("FAIL: *** T_CString_toUpperCase() failed. Expected: \"HELLO THERE\", Got: \"%s\"\n", src);
    }
     
    intValue=T_CString_stringToInteger("34556", 10);
    if(intValue != 34556){
        log_err("FAIL: ****T_CString_stringToInteger(\"34556\", 10) failed. Expected: 34556, Got: %d\n", intValue);
    }
    intValue=T_CString_stringToInteger("100", 16);
    if(intValue != 256){
        log_err("FAIL: ****T_CString_stringToInteger(\"100\", 16) failed. Expected: 256, Got: %d\n", intValue);
    }
    i = T_CString_integerToString(src, 34556, 10);
    if(uprv_strcmp(src, "34556") != 0 || i != 5){
        log_err("FAIL: ****integerToString(src, 34566, 10); failed. Expected: \"34556\", Got: %s\n", src);
    }
    i = T_CString_integerToString(src, 431, 16);
    if(uprv_stricmp(src, "1AF") != 0 || i != 3){
        log_err("FAIL: ****integerToString(src, 431, 16); failed. Expected: \"1AF\", Got: %s\n", src);
    }
    i = T_CString_int64ToString(src, U_INT64_MAX, 10);
    if(uprv_strcmp(src,  "9223372036854775807") != 0 || i != 19){
        log_err("FAIL: ****integerToString(src, 9223372036854775807, 10); failed. Got: %s\n", src);
    }
    i = T_CString_int64ToString(src, U_INT64_MAX, 16);
    if(uprv_stricmp(src, "7FFFFFFFFFFFFFFF") != 0 || i != 16){
        log_err("FAIL: ****integerToString(src, 7FFFFFFFFFFFFFFF, 16); failed. Got: %s\n", src);
    }

    uprv_strcpy(src, "this is lower case");
    if(T_CString_stricmp(src, "THIS is lower CASE") != 0){
        log_err("FAIL: *****T_CString_stricmp() failed.");
    }
    if((intValue=T_CString_stricmp(NULL, "first string is null") )!= -1){
        log_err("FAIL: T_CString_stricmp() where the first string is null failed. Expected: -1, returned %d\n", intValue);
    }
    if((intValue=T_CString_stricmp("second string is null", NULL)) != 1){
        log_err("FAIL: T_CString_stricmp() where the second string is null failed. Expected: 1, returned %d\n", intValue);
    }
    if((intValue=T_CString_stricmp(NULL, NULL)) != 0){
        log_err("FAIL: T_CString_stricmp(NULL, NULL) failed.  Expected:  0, returned %d\n", intValue);;
    }
    if((intValue=T_CString_stricmp("", "")) != 0){
        log_err("FAIL: T_CString_stricmp(\"\", \"\") failed.  Expected:  0, returned %d\n", intValue);;
    }
    if((intValue=T_CString_stricmp("", "abc")) != -1){
        log_err("FAIL: T_CString_stricmp(\"\", \"abc\") failed.  Expected: -1, returned %d\n", intValue);
    }
    if((intValue=T_CString_stricmp("abc", "")) != 1){
        log_err("FAIL: T_CString_stricmp(\"abc\", \"\") failed.  Expected: 1, returned %d\n", intValue);
    }

    temp=uprv_strdup("strdup");
    if(uprv_strcmp(temp, "strdup") !=0 ){
        log_err("FAIL: uprv_strdup() failed. Expected: \"strdup\", Got: %s\n", temp);
    }
    uprv_free((char *)temp);
  
    uprv_strcpy(src, "this is lower case");
    if(T_CString_strnicmp(src, "THIS", 4 ) != 0){
        log_err("FAIL: *****T_CString_strnicmp() failed.");
    }
    if((intValue=T_CString_strnicmp(NULL, "first string is null", 10) )!= -1){
        log_err("FAIL: T_CString_strnicmp() where the first string is null failed. Expected: -1, returned %d\n", intValue);
    }
    if((intValue=T_CString_strnicmp("second string is null", NULL, 10)) != 1){
        log_err("FAIL: T_CString_strnicmp() where the second string is null failed. Expected: 1, returned %d\n", intValue);
    }
    if((intValue=T_CString_strnicmp(NULL, NULL, 10)) != 0){
        log_err("FAIL: T_CString_strnicmp(NULL, NULL, 10) failed.  Expected:  0, returned %d\n", intValue);;
    }
    if((intValue=T_CString_strnicmp("", "", 10)) != 0){
        log_err("FAIL: T_CString_strnicmp(\"\", \"\") failed.  Expected:  0, returned %d\n", intValue);;
    }
    if((intValue=T_CString_strnicmp("", "abc", 10)) != -1){
        log_err("FAIL: T_CString_stricmp(\"\", \"abc\", 10) failed.  Expected: -1, returned %d\n", intValue);
    }
    if((intValue=T_CString_strnicmp("abc", "", 10)) != 1){
        log_err("FAIL: T_CString_strnicmp(\"abc\", \"\", 10) failed.  Expected: 1, returned %d\n", intValue);
    }
    
}

/* test invariant-character handling */
static void
TestInvariant() {
    /* all invariant graphic chars and some control codes (not \n!) */
    const char invariantChars[]=
        "\t\r \"%&'()*+,-./"
        "0123456789:;<=>?"
        "ABCDEFGHIJKLMNOPQRSTUVWXYZ_"
        "abcdefghijklmnopqrstuvwxyz";

    const UChar invariantUChars[]={
        9, 0xd, 0x20, 0x22, 0x25, 0x26, 0x27, 0x28, 0x29, 0x2a, 0x2b, 0x2c, 0x2d, 0x2e, 0x2f,
        0x30, 0x31, 0x32, 0x33, 0x34, 0x35, 0x36, 0x37, 0x38, 0x39, 0x3a, 0x3b, 0x3c, 0x3d, 0x3e, 0x3f,
        0x41, 0x42, 0x43, 0x44, 0x45, 0x46, 0x47, 0x48, 0x49, 0x4a, 0x4b, 0x4c, 0x4d, 0x4e, 0x4f,
        0x50, 0x51, 0x52, 0x53, 0x54, 0x55, 0x56, 0x57, 0x58, 0x59, 0x5a, 0x5f,
        0x61, 0x62, 0x63, 0x64, 0x65, 0x66, 0x67, 0x68, 0x69, 0x6a, 0x6b, 0x6c, 0x6d, 0x6e, 0x6f,
        0x70, 0x71, 0x72, 0x73, 0x74, 0x75, 0x76, 0x77, 0x78, 0x79, 0x7a, 0
    };

    const char variantChars[]="\n!#$@[\\]^`{|}~";

    const UChar variantUChars[]={
        0x0a, 0x21, 0x23, 0x24, 0x40, 0x5b, 0x5c, 0x5d, 0x5e, 0x60, 0x7b, 0x7c, 0x7d, 0x7e, 0
    };

    const UChar nonASCIIUChars[]={ 0x80, 0xa0, 0x900, 0xff51 };

    UChar us[120];
    char cs[120];

    int32_t i, length;

    /* make sure that all invariant characters convert both ways */
    length=sizeof(invariantChars);
    u_charsToUChars(invariantChars, us, length);
    if(u_strcmp(us, invariantUChars)!=0) {
        log_err("u_charsToUChars(invariantChars) failed\n");
    }

    u_UCharsToChars(invariantUChars, cs, length);
    if(strcmp(cs, invariantChars)!=0) {
        log_err("u_UCharsToChars(invariantUChars) failed\n");
    }


    /*
     * make sure that variant characters convert from source code literals to Unicode
     * but not back to char *
     */
    length=sizeof(variantChars);
    u_charsToUChars(variantChars, us, length);
    if(u_strcmp(us, variantUChars)!=0) {
        log_err("u_charsToUChars(variantChars) failed\n");
    }

#ifdef NDEBUG
    /*
     * Test u_UCharsToChars(variantUChars) only in release mode because it will
     * cause an assertion failure in debug builds.
     */
    u_UCharsToChars(variantUChars, cs, length);
    for(i=0; i<length; ++i) {
        if(cs[i]!=0) {
            log_err("u_UCharsToChars(variantUChars) converted the %d-th character to %02x instead of 00\n", i, cs[i]);
        }
    }
#endif

    /*
     * Verify that invariant characters roundtrip from Unicode to the
     * default converter and back.
     */
    {
        UConverter *cnv;
        UErrorCode errorCode;

        errorCode=U_ZERO_ERROR;
        cnv=ucnv_open(NULL, &errorCode);
        if(U_FAILURE(errorCode)) {
            log_err("unable to open the default converter\n");
        } else {
            length=ucnv_fromUChars(cnv, cs, sizeof(cs), invariantUChars, -1, &errorCode);
            if(U_FAILURE(errorCode)) {
                log_err("ucnv_fromUChars(invariantUChars) failed - %s\n", u_errorName(errorCode));
            } else if(length!=sizeof(invariantChars)-1 || strcmp(cs, invariantChars)!=0) {
                log_err("ucnv_fromUChars(invariantUChars) failed\n");
            }

            errorCode=U_ZERO_ERROR;
            length=ucnv_toUChars(cnv, us, LENGTHOF(us), invariantChars, -1, &errorCode);
            if(U_FAILURE(errorCode)) {
                log_err("ucnv_toUChars(invariantChars) failed - %s\n", u_errorName(errorCode));
            } else if(length!=LENGTHOF(invariantUChars)-1 || u_strcmp(us, invariantUChars)!=0) {
                log_err("ucnv_toUChars(invariantChars) failed\n");
            }

            ucnv_close(cnv);
        }
    }

    /* API tests */
    if(!uprv_isInvariantString(invariantChars, -1)) {
        log_err("uprv_isInvariantString(invariantChars) failed\n");
    }
    if(!uprv_isInvariantUString(invariantUChars, -1)) {
        log_err("uprv_isInvariantUString(invariantUChars) failed\n");
    }
    if(!uprv_isInvariantString(invariantChars+strlen(invariantChars), 1)) {
        log_err("uprv_isInvariantString(\"\\0\") failed\n");
    }

    for(i=0; i<(sizeof(variantChars)-1); ++i) {
        if(uprv_isInvariantString(variantChars+i, 1)) {
            log_err("uprv_isInvariantString(variantChars[%d]) failed\n", i);
        }
        if(uprv_isInvariantUString(variantUChars+i, 1)) {
            log_err("uprv_isInvariantUString(variantUChars[%d]) failed\n", i);
        }
    }

    for(i=0; i<LENGTHOF(nonASCIIUChars); ++i) {
        if(uprv_isInvariantUString(nonASCIIUChars+i, 1)) {
            log_err("uprv_isInvariantUString(nonASCIIUChars[%d]) failed\n", i);
        }
    }
}

static int32_t getSign(int32_t n) {
    if(n<0) {
        return -1;
    } else if(n==0) {
        return 0;
    } else {
        return 1;
    }
}

static void
TestCompareInvEbcdicAsAscii() {
    static const char *const invStrings[][2]={
        /* invariant-character strings in ascending ASCII order */
        /* EBCDIC       native */
        { "",             "" },
        { "\x6c",         "%" },
        { "\xf0",         "0" },
        { "\xf0\xf0",     "00" },
        { "\xf0\xf0\x81", "00a" },
        { "\x7e",         "=" },
        { "\xc1",         "A" },
        { "\xc1\xf0\xf0", "A00" },
        { "\xc1\xf0\xf0", "A00" },
        { "\xc1\xc1",     "AA" },
        { "\xc1\xc1\xf0", "AA0" },
        { "\x6d",         "_" },
        { "\x81",         "a" },
        { "\x81\xf0\xf0", "a00" },
        { "\x81\xf0\xf0", "a00" },
        { "\x81\x81",     "aa" },
        { "\x81\x81\xf0", "aa0" },
        { "\x81\x81\x81", "aaa" },
        { "\x81\x81\x82", "aab" }
    };
    int32_t i;
    for(i=1; i<LENGTHOF(invStrings); ++i) {
        int32_t diff1, diff2;
        /* compare previous vs. current */
        diff1=getSign(uprv_compareInvEbcdicAsAscii(invStrings[i-1][0], invStrings[i][0]));
        if(diff1>0 || (diff1==0 && 0!=uprv_strcmp(invStrings[i-1][0], invStrings[i][0]))) {
            log_err("uprv_compareInvEbcdicAsAscii(%s, %s)=%hd is wrong\n",
                    invStrings[i-1][1], invStrings[i][1], (short)diff1);
        }
        /* compare current vs. previous, should be inverse diff */
        diff2=getSign(uprv_compareInvEbcdicAsAscii(invStrings[i][0], invStrings[i-1][0]));
        if(diff2!=-diff1) {
            log_err("uprv_compareInvEbcdicAsAscii(%s, %s)=%hd is wrong\n",
                    invStrings[i][1], invStrings[i-1][1], (short)diff2);
        }
    }
}
