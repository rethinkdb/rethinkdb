/********************************************************************
 * COPYRIGHT: 
 * Copyright (c) 1998-2010, International Business Machines Corporation and
 * others. All Rights Reserved.
 ********************************************************************/
/*
* File putiltst.c (Tests the API in putil)
*
* Modification History:
*
*   Date          Name        Description
*   07/12/2000    Madhu       Creation 
*******************************************************************************
*/

#include "unicode/utypes.h"
#include "cintltst.h"
#include "cmemory.h"
#include "unicode/putil.h"
#include "unicode/ustring.h"
#include "unicode/icudataver.h"
#include "cstring.h"
#include "putilimp.h"
#include "toolutil.h"
#include "uinvchar.h"
#include <stdio.h>


static UBool compareWithNAN(double x, double y);
static void doAssert(double expect, double got, const char *message);

static void TestPUtilAPI(void){

    double  n1=0.0, y1=0.0, expn1, expy1;
    double  value1 = 0.021;
    char *str=0;
    UBool isTrue=FALSE;

    log_verbose("Testing the API uprv_modf()\n");
    y1 = uprv_modf(value1, &n1);
    expn1=0;
    expy1=0.021;
    if(y1 != expy1   || n1 != expn1){
        log_err("Error in uprv_modf.  Expected IntegralValue=%f, Got=%f, \n Expected FractionalValue=%f, Got=%f\n",
             expn1, n1, expy1, y1);
    }
    if(getTestOption(VERBOSITY_OPTION)){
        log_verbose("[float]  x = %f  n = %f y = %f\n", value1, n1, y1);
    }
    log_verbose("Testing the API uprv_fmod()\n");
    expn1=uprv_fmod(30.50, 15.00);
    doAssert(expn1, 0.5, "uprv_fmod(30.50, 15.00) failed.");

    log_verbose("Testing the API uprv_ceil()\n");
    expn1=uprv_ceil(value1);
    doAssert(expn1, 1, "uprv_ceil(0.021) failed.");

    log_verbose("Testing the API uprv_floor()\n");
    expn1=uprv_floor(value1);
    doAssert(expn1, 0, "uprv_floor(0.021) failed.");

    log_verbose("Testing the API uprv_fabs()\n");
    expn1=uprv_fabs((2.02-1.345));
    doAssert(expn1, 0.675, "uprv_fabs(2.02-1.345) failed.");
    
    log_verbose("Testing the API uprv_fmax()\n");
    doAssert(uprv_fmax(2.4, 1.2), 2.4, "uprv_fmax(2.4, 1.2) failed.");

    log_verbose("Testing the API uprv_fmax() with x value= NaN\n");
    expn1=uprv_fmax(uprv_getNaN(), 1.2);
    doAssert(expn1, uprv_getNaN(), "uprv_fmax(uprv_getNaN(), 1.2) failed. when one parameter is NaN");

    log_verbose("Testing the API uprv_fmin()\n");
    doAssert(uprv_fmin(2.4, 1.2), 1.2, "uprv_fmin(2.4, 1.2) failed.");

    log_verbose("Testing the API uprv_fmin() with x value= NaN\n");
    expn1=uprv_fmin(uprv_getNaN(), 1.2);
    doAssert(expn1, uprv_getNaN(), "uprv_fmin(uprv_getNaN(), 1.2) failed. when one parameter is NaN");

    log_verbose("Testing the API uprv_max()\n");
    doAssert(uprv_max(4, 2), 4, "uprv_max(4, 2) failed.");

    log_verbose("Testing the API uprv_min()\n");
    doAssert(uprv_min(-4, 2), -4, "uprv_min(-4, 2) failed.");

    log_verbose("Testing the API uprv_trunc()\n");
    doAssert(uprv_trunc(12.3456), 12, "uprv_trunc(12.3456) failed.");
    doAssert(uprv_trunc(12.234E2), 1223, "uprv_trunc(12.234E2) failed.");
    doAssert(uprv_trunc(uprv_getNaN()), uprv_getNaN(), "uprv_trunc(uprv_getNaN()) failed. with parameter=NaN");
    doAssert(uprv_trunc(uprv_getInfinity()), uprv_getInfinity(), "uprv_trunc(uprv_getInfinity()) failed. with parameter=Infinity");


    log_verbose("Testing the API uprv_pow10()\n");
    doAssert(uprv_pow10(4), 10000, "uprv_pow10(4) failed.");

    log_verbose("Testing the API uprv_isNegativeInfinity()\n");
    isTrue=uprv_isNegativeInfinity(uprv_getInfinity() * -1);
    if(isTrue != TRUE){
        log_err("ERROR: uprv_isNegativeInfinity failed.\n");
    }
    log_verbose("Testing the API uprv_isPositiveInfinity()\n");
    isTrue=uprv_isPositiveInfinity(uprv_getInfinity());
    if(isTrue != TRUE){
        log_err("ERROR: uprv_isPositiveInfinity failed.\n");
    }
    log_verbose("Testing the API uprv_isInfinite()\n");
    isTrue=uprv_isInfinite(uprv_getInfinity());
    if(isTrue != TRUE){
        log_err("ERROR: uprv_isInfinite failed.\n");
    }

#if 0
    log_verbose("Testing the API uprv_digitsAfterDecimal()....\n");
    doAssert(uprv_digitsAfterDecimal(value1), 3, "uprv_digitsAfterDecimal() failed.");
    doAssert(uprv_digitsAfterDecimal(1.2345E2), 2, "uprv_digitsAfterDecimal(1.2345E2) failed.");
    doAssert(uprv_digitsAfterDecimal(1.2345E-2), 6, "uprv_digitsAfterDecimal(1.2345E-2) failed.");
    doAssert(uprv_digitsAfterDecimal(1.2345E2), 2, "uprv_digitsAfterDecimal(1.2345E2) failed.");
    doAssert(uprv_digitsAfterDecimal(-1.2345E-20), 24, "uprv_digitsAfterDecimal(1.2345E-20) failed.");
    doAssert(uprv_digitsAfterDecimal(1.2345E20), 0, "uprv_digitsAfterDecimal(1.2345E20) failed.");
    doAssert(uprv_digitsAfterDecimal(-0.021), 3, "uprv_digitsAfterDecimal(-0.021) failed.");
    doAssert(uprv_digitsAfterDecimal(23.0), 0, "uprv_digitsAfterDecimal(23.0) failed.");
    doAssert(uprv_digitsAfterDecimal(0.022223333321), 9, "uprv_digitsAfterDecimal(0.022223333321) failed.");
#endif

    log_verbose("Testing the API u_errorName()...\n");
    str=(char*)u_errorName((UErrorCode)0);
    if(strcmp(str, "U_ZERO_ERROR") != 0){
        log_err("ERROR: u_getVersion() failed. Expected: U_ZERO_ERROR Got=%s\n",  str);
    }
    log_verbose("Testing the API u_errorName()...\n");
    str=(char*)u_errorName((UErrorCode)-127);
    if(strcmp(str, "U_USING_DEFAULT_WARNING") != 0){
        log_err("ERROR: u_getVersion() failed. Expected: U_USING_DEFAULT_WARNING Got=%s\n",  str);
    }
    log_verbose("Testing the API u_errorName().. with BOGUS ERRORCODE...\n");
    str=(char*)u_errorName((UErrorCode)200);
    if(strcmp(str, "[BOGUS UErrorCode]") != 0){
        log_err("ERROR: u_getVersion() failed. Expected: [BOGUS UErrorCode] Got=%s\n",  str);
    }

    {
        const char* dataDirectory;
        int32_t dataDirectoryLen;
        UChar *udataDir=0;
        UChar temp[100];
        char *charvalue=0;
        log_verbose("Testing chars to UChars\n");
        
         /* This cannot really work on a japanese system. u_uastrcpy will have different results than */
        /* u_charsToUChars when there is a backslash in the string! */
        /*dataDirectory=u_getDataDirectory();*/

        dataDirectory="directory1";  /*no backslashes*/
        dataDirectoryLen=(int32_t)strlen(dataDirectory);
        udataDir=(UChar*)malloc(sizeof(UChar) * (dataDirectoryLen + 1));
        u_charsToUChars(dataDirectory, udataDir, (dataDirectoryLen + 1));
        u_uastrcpy(temp, dataDirectory);
       
        if(u_strcmp(temp, udataDir) != 0){
            log_err("ERROR: u_charsToUChars failed. Expected %s, Got %s\n", austrdup(temp), austrdup(udataDir));
        }
        log_verbose("Testing UChars to chars\n");
        charvalue=(char*)malloc(sizeof(char) * (u_strlen(udataDir) + 1));

        u_UCharsToChars(udataDir, charvalue, (u_strlen(udataDir)+1));
        if(strcmp(charvalue, dataDirectory) != 0){
            log_err("ERROR: u_UCharsToChars failed. Expected %s, Got %s\n", charvalue, dataDirectory);
        }
        free(charvalue);
        free(udataDir);
    }
   
    log_verbose("Testing uprv_timezone()....\n");
    {
        int32_t tzoffset = uprv_timezone();
        log_verbose("Value returned from uprv_timezone = %d\n",  tzoffset);
        if (tzoffset != 28800) {
            log_verbose("***** WARNING: If testing in the PST timezone, t_timezone should return 28800! *****");
        }
        if ((tzoffset % 1800 != 0)) {
            log_info("Note: t_timezone offset of %ld (for %s : %s) is not a multiple of 30min.", tzoffset, uprv_tzname(0), uprv_tzname(1));
        }
        /*tzoffset=uprv_getUTCtime();*/

    }
}

static void TestVersion()
{
    UVersionInfo versionArray = {0x01, 0x00, 0x02, 0x02};
    UVersionInfo versionArray2 = {0x01, 0x00, 0x02, 0x02};
    char versionString[17]; /* xxx.xxx.xxx.xxx\0 */
    UChar versionUString[] = { 0x0031, 0x002E, 0x0030, 0x002E,
                               0x0032, 0x002E, 0x0038, 0x0000 }; /* 1.0.2.8 */
    UBool isModified = FALSE;
    UVersionInfo version;
    UErrorCode status = U_ZERO_ERROR;

    log_verbose("Testing the API u_versionToString().....\n");
    u_versionToString(versionArray, versionString);
    if(strcmp(versionString, "1.0.2.2") != 0){
        log_err("ERROR: u_versionToString() failed. Expected: 1.0.2.2, Got=%s\n", versionString);
    }
    log_verbose("Testing the API u_versionToString().....with versionArray=NULL\n");
    u_versionToString(NULL, versionString);
    if(strcmp(versionString, "") != 0){
        log_err("ERROR: u_versionToString() failed. with versionArray=NULL. It should just return\n");
    }
    log_verbose("Testing the API u_versionToString().....with versionArray=NULL\n");
    u_versionToString(NULL, versionString);
    if(strcmp(versionString, "") != 0){
        log_err("ERROR: u_versionToString() failed . It should just return\n");
    }
    log_verbose("Testing the API u_versionToString().....with versionString=NULL\n");
    u_versionToString(versionArray, NULL);
    if(strcmp(versionString, "") != 0){
        log_err("ERROR: u_versionToString() failed. with versionArray=NULL  It should just return\n");
    }
    versionArray[0] = 0x0a;
    log_verbose("Testing the API u_versionToString().....\n");
    u_versionToString(versionArray, versionString);
    if(strcmp(versionString, "10.0.2.2") != 0){
        log_err("ERROR: u_versionToString() failed. Expected: 10.0.2.2, Got=%s\n", versionString);
    }
    versionArray[0] = 0xa0;
    u_versionToString(versionArray, versionString);
    if(strcmp(versionString, "160.0.2.2") != 0){
        log_err("ERROR: u_versionToString() failed. Expected: 160.0.2.2, Got=%s\n", versionString);
    }
    versionArray[0] = 0xa0;
    versionArray[1] = 0xa0;
    u_versionToString(versionArray, versionString);
    if(strcmp(versionString, "160.160.2.2") != 0){
        log_err("ERROR: u_versionToString() failed. Expected: 160.160.2.2, Got=%s\n", versionString);
    }
    versionArray[0] = 0x01;
    versionArray[1] = 0x0a;
    u_versionToString(versionArray, versionString);
    if(strcmp(versionString, "1.10.2.2") != 0){
        log_err("ERROR: u_versionToString() failed. Expected: 160.160.2.2, Got=%s\n", versionString);
    }

    log_verbose("Testing the API u_versionFromString() ....\n");
    u_versionFromString(versionArray, "1.3.5.6");
    u_versionToString(versionArray, versionString);
    if(strcmp(versionString, "1.3.5.6") != 0){
        log_err("ERROR: u_getVersion() failed. Expected: 1.3.5.6, Got=%s\n",  versionString);
    }
    log_verbose("Testing the API u_versionFromString() where versionArray=NULL....\n");
    u_versionFromString(NULL, "1.3.5.6");
    u_versionToString(versionArray, versionString);
    if(strcmp(versionString, "1.3.5.6") != 0){
        log_err("ERROR: u_getVersion() failed. Expected: 1.3.5.6, Got=%s\n",  versionString);
    }

    log_verbose("Testing the API u_getVersion().....\n");
    u_getVersion(versionArray);
    u_versionToString(versionArray, versionString);
    if(strcmp(versionString, U_ICU_VERSION) != 0){
        log_err("ERROR: u_getVersion() failed. Got=%s, expected %s\n",  versionString, U_ICU_VERSION);
    }
    /* test unicode */
    log_verbose("Testing u_versionFromUString...\n");
    u_versionFromString(versionArray,"1.0.2.8");
    u_versionFromUString(versionArray2, versionUString);
    u_versionToString(versionArray2, versionString); 
    if(memcmp(versionArray, versionArray2, sizeof(UVersionInfo))) {
       log_err("FAIL: u_versionFromUString produced a different result - not 1.0.2.8 but %s [%x.%x.%x.%x]\n", 
          versionString,
        (int)versionArray2[0],
        (int)versionArray2[1],
        (int)versionArray2[2],
        (int)versionArray2[3]);
    } 
    else {
       log_verbose(" from UString: %s\n", versionString);
    }

    /* Test the data version API for better code coverage */
    u_getDataVersion(version, &status);
    if (U_FAILURE(status)) {
        log_data_err("ERROR: Unable to get data version. %s\n", u_errorName(status));
    } else {
        u_isDataOlder(version, &isModified, &status);
        if (U_FAILURE(status)) {
            log_err("ERROR: Unable to compare data version. %s\n", u_errorName(status));
        }
    }
}

static void TestCompareVersions()
{
   /* use a 1d array to be palatable to java */
   const char *testCases[] = {
      /*  v1          <|=|>       v2  */
    "0.0.0.0",    "=",        "0.0.0.0",
    "3.1.2.0",    ">",        "3.0.9.0",
    "3.2.8.6",    "<",        "3.4",
    "4.0",        ">",        "3.2",
    NULL,        NULL,        NULL
   };
   const char *v1str;
   const char *opstr;
   const char *v2str;
   int32_t op, invop, got, invgot; 
   UVersionInfo v1, v2;
   int32_t j;
   log_verbose("Testing memcmp()\n");
   for(j=0;testCases[j]!=NULL;j+=3) {
    v1str = testCases[j+0];
    opstr = testCases[j+1];
    v2str = testCases[j+2];
    switch(opstr[0]) {
        case '-':
        case '<': op = -1; break;
        case '0':
        case '=': op = 0; break;
        case '+':
        case '>': op = 1; break;
        default:  log_err("Bad operator at j/3=%d\n", (j/3)); return;
    }
    invop = 0-op; /* inverse operation: with v1 and v2 switched */
    u_versionFromString(v1, v1str);
    u_versionFromString(v2, v2str);
    got = memcmp(v1, v2, sizeof(UVersionInfo));
    invgot = memcmp(v2, v1, sizeof(UVersionInfo)); /* Opposite */
    if((got <= 0 && op <= 0) || (got >= 0 && op >= 0)) {
        log_verbose("%d: %s %s %s, OK\n", (j/3), v1str, opstr, v2str);
    } else {
        log_err("%d: %s %s %s: wanted values of the same sign, %d got %d\n", (j/3), v1str, opstr, v2str, op, got);
    }
    if((invgot >= 0 && invop >= 0) || (invgot <= 0 && invop <= 0)) {
        log_verbose("%d: %s (%d) %s, OK (inverse)\n", (j/3), v2str, invop, v1str);
    } else {
        log_err("%d: %s (%d) %s: wanted values of the same sign, %d got %d\n", (j/3), v2str, invop, v1str, invop, invgot);
    }
   }
}



#if 0
static void testIEEEremainder()
{
    double    pinf        = uprv_getInfinity();
    double    ninf        = -uprv_getInfinity();
    double    nan         = uprv_getNaN();
/*    double    pzero       = 0.0;*/
/*    double    nzero       = 0.0;
    nzero *= -1;*/

     /* simple remainder checks*/
    remainderTest(7.0, 2.5, -0.5);
    remainderTest(7.0, -2.5, -0.5);
     /* this should work
     remainderTest(43.7, 2.5, 1.2);
     */

    /* infinity and real*/
    remainderTest(1.0, pinf, 1.0);
    remainderTest(1.0, ninf, 1.0);

    /*test infinity and real*/
    remainderTest(nan, 1.0, nan);
    remainderTest(1.0, nan, nan);
    /*test infinity and nan*/
    remainderTest(ninf, nan, nan);
    remainderTest(pinf, nan, nan);

    /* test infinity and zero */
/*    remainderTest(pinf, pzero, 1.25);
    remainderTest(pinf, nzero, 1.25);
    remainderTest(ninf, pzero, 1.25);
    remainderTest(ninf, nzero, 1.25); */
}

static void remainderTest(double x, double y, double exp)
{
    double result = uprv_IEEEremainder(x,y);

    if(        uprv_isNaN(result) && 
        ! ( uprv_isNaN(x) || uprv_isNaN(y))) {
        log_err("FAIL: got NaN as result without NaN as argument");
        log_err("      IEEEremainder(%f, %f) is %f, expected %f\n", x, y, result, exp);
    }
    else if(!compareWithNAN(result, exp)) {
        log_err("FAIL:  IEEEremainder(%f, %f) is %f, expected %f\n", x, y, result, exp);
    } else{
        log_verbose("OK: IEEEremainder(%f, %f) is %f\n", x, y, result);
    }

}
#endif

static UBool compareWithNAN(double x, double y)
{
  if( uprv_isNaN(x) || uprv_isNaN(y) ) {
    if(!uprv_isNaN(x) || !uprv_isNaN(y) ) {
      return FALSE;
    }
  }
  else if (y != x) { /* no NaN's involved */
    return FALSE;
  }

  return TRUE;
}

static void doAssert(double got, double expect, const char *message)
{
  if(! compareWithNAN(expect, got) ) {
    log_err("ERROR :  %s. Expected : %lf, Got: %lf\n", message, expect, got);
  }
}


#define _CODE_ARR_LEN 8
static const UErrorCode errorCode[_CODE_ARR_LEN] = {
    U_USING_FALLBACK_WARNING,
    U_STRING_NOT_TERMINATED_WARNING,
    U_ILLEGAL_ARGUMENT_ERROR,
    U_STATE_TOO_OLD_ERROR,
    U_BAD_VARIABLE_DEFINITION,
    U_RULE_MASK_ERROR,
    U_UNEXPECTED_TOKEN,
    U_UNSUPPORTED_ATTRIBUTE
};

static const char* str[] = {
    "U_USING_FALLBACK_WARNING",
    "U_STRING_NOT_TERMINATED_WARNING",
    "U_ILLEGAL_ARGUMENT_ERROR",
    "U_STATE_TOO_OLD_ERROR",
    "U_BAD_VARIABLE_DEFINITION",
    "U_RULE_MASK_ERROR",
    "U_UNEXPECTED_TOKEN",
    "U_UNSUPPORTED_ATTRIBUTE"
};

static void TestErrorName(void){
    int32_t code=0;
    const char* errorName ;
    for(;code<U_ERROR_LIMIT;code++){
        errorName = u_errorName((UErrorCode)code);
        if(!errorName || errorName[0] == 0) {
          log_err("Error:  u_errorName(0x%X) failed.\n",code);
        }
    }

    for(code=0;code<_CODE_ARR_LEN; code++){
        errorName = u_errorName(errorCode[code]);
        if(uprv_strcmp(str[code],errorName )!=0){
            log_err("Error : u_errorName failed. Expected: %s Got: %s \n",str[code],errorName);
        }
    }
}

#define AESTRNCPY_SIZE 13

static const char * dump_binline(uint8_t *bytes) {
  static char buf[512];
  int32_t i;
  for(i=0;i<13;i++) {
    sprintf(buf+(i*3), "%02x ", bytes[i]);
  }
  return buf;
}

static void Test_aestrncpy(int32_t line, const uint8_t *expect, const uint8_t *src, int32_t len)
{
  uint8_t str_buf[AESTRNCPY_SIZE] = { 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff };
  uint8_t *ret;

  log_verbose("\n%s:%d: Beginning test of uprv_aestrncpy(dst, src, %d)\n", __FILE__, line, len);
  ret = uprv_aestrncpy(str_buf, src, len);
  if(ret != str_buf) {
    log_err("\n%s:%d: FAIL: uprv_aestrncpy returned %p expected %p\n", __FILE__, line, (void*)ret, (void*)str_buf);
  }
  if(!uprv_memcmp(str_buf, expect, AESTRNCPY_SIZE)) {
    log_verbose("\n%s:%d: OK - compared OK.", __FILE__, line);
    log_verbose("\n%s:%d:         expected: %s", __FILE__, line, dump_binline((uint8_t *)expect));
    log_verbose("\n%s:%d:         got     : %s\n", __FILE__, line, dump_binline(str_buf));
  } else {
    log_err    ("\n%s:%d: FAIL: uprv_aestrncpy output differs", __FILE__, line);
    log_err    ("\n%s:%d:         expected: %s", __FILE__, line, dump_binline((uint8_t *)expect));
    log_err    ("\n%s:%d:         got     : %s\n", __FILE__, line, dump_binline(str_buf));
  }
}

static void TestString(void)
{

  uint8_t str_tst[AESTRNCPY_SIZE] = { 0x81, 0x4b, 0x5c, 0x82, 0x25, 0x00, 0x7f, 0x7f, 0x7f, 0x7f, 0x7f, 0x7f, 0x7f };

  uint8_t str_exp1[AESTRNCPY_SIZE] = { 0x61, 0x2e, 0x2a, 0x62, 0x0a, 0x00, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff };
  uint8_t str_exp2[AESTRNCPY_SIZE] = { 0x61, 0x2e, 0x2a, 0x62, 0x0a, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff };
  uint8_t str_exp3[AESTRNCPY_SIZE] = { 0x61, 0x2e, 0x2a, 0x62, 0x0a, 0x00, 0x00, 0x00, 0xff, 0xff, 0xff, 0xff, 0xff };

  

  /* test #1- copy with -1 length */
  Test_aestrncpy(__LINE__, str_exp1, str_tst, -1);
  Test_aestrncpy(__LINE__, str_exp1, str_tst, 6);
  Test_aestrncpy(__LINE__, str_exp2, str_tst, 5);
  Test_aestrncpy(__LINE__, str_exp3, str_tst, 8);
}

void addPUtilTest(TestNode** root);

static void addToolUtilTests(TestNode** root);

void
addPUtilTest(TestNode** root)
{
    addTest(root, &TestVersion,       "putiltst/TestVersion");
    addTest(root, &TestCompareVersions,       "putiltst/TestCompareVersions");
/*    addTest(root, &testIEEEremainder,  "putiltst/testIEEEremainder"); */
    addTest(root, &TestErrorName, "putiltst/TestErrorName");
    addTest(root, &TestPUtilAPI,       "putiltst/TestPUtilAPI");
    addTest(root, &TestString,    "putiltst/TestString");
    addToolUtilTests(root);
}

/* Tool Util Tests ================ */
#define TOOLUTIL_TESTBUF_SIZE 2048
static char toolutil_testBuf[TOOLUTIL_TESTBUF_SIZE];
static const char *NULLSTR="NULL";

/**
 * Normalize NULL to 'NULL'  for testing
 */
#define STRNULL(x) ((x)?(x):NULLSTR)

static void toolutil_findBasename(void)
{
  struct {
    const char *inBuf;
    const char *expectResult;
  } testCases[] = { 
    {
      U_FILE_SEP_STRING "usr" U_FILE_SEP_STRING "bin" U_FILE_SEP_STRING "pkgdata",
      "pkgdata"
    },
    {
      U_FILE_SEP_STRING "usr" U_FILE_SEP_STRING "bin" U_FILE_SEP_STRING,
      ""
    },
    {
      U_FILE_ALT_SEP_STRING "usr" U_FILE_ALT_SEP_STRING "bin" U_FILE_ALT_SEP_STRING "pkgdata",
      "pkgdata"
    },
    {
      U_FILE_ALT_SEP_STRING "usr" U_FILE_ALT_SEP_STRING "bin" U_FILE_ALT_SEP_STRING,
      ""
    },
  };
  int32_t count=(sizeof(testCases)/sizeof(testCases[0]));
  int32_t i;


  log_verbose("Testing findBaseName()\n");
  for(i=0;i<count;i++) {
    const char *result;
    const char *input = STRNULL(testCases[i].inBuf);
    const char *expect = STRNULL(testCases[i].expectResult);
    log_verbose("Test case [%d/%d]: %s\n", i, count-1, input);
    result = STRNULL(findBasename(testCases[i].inBuf));
    if(result==expect||!strcmp(result,expect)) {
      log_verbose(" -> %s PASS\n", result);
    } else {
      log_err("FAIL: Test case [%d/%d]: %s -> %s but expected %s\n", i, count-1, input, result, expect);
    }
  }
}


static void toolutil_findDirname(void)
{
  int i;
  struct {
    const char *inBuf;
    int32_t outBufLen;
    UErrorCode expectStatus;
    const char *expectResult;
  } testCases[] = { 
    {
      U_FILE_SEP_STRING "usr" U_FILE_SEP_STRING "bin" U_FILE_SEP_STRING "pkgdata",
      200,
      U_ZERO_ERROR,
      U_FILE_SEP_STRING "usr" U_FILE_SEP_STRING "bin",
    },
    {
      U_FILE_SEP_STRING "usr" U_FILE_SEP_STRING "bin" U_FILE_SEP_STRING "pkgdata",
      2,
      U_BUFFER_OVERFLOW_ERROR,
      NULL
    },
    {
      U_FILE_ALT_SEP_STRING "usr" U_FILE_ALT_SEP_STRING "bin" U_FILE_ALT_SEP_STRING "pkgdata",
      200,
      U_ZERO_ERROR,
      U_FILE_ALT_SEP_STRING "usr" U_FILE_ALT_SEP_STRING "bin"
    },
    {
      U_FILE_ALT_SEP_STRING "usr" U_FILE_ALT_SEP_STRING "bin" U_FILE_ALT_SEP_STRING "pkgdata",
      2,
      U_BUFFER_OVERFLOW_ERROR,
      NULL
    },
    {
      U_FILE_ALT_SEP_STRING "usr" U_FILE_ALT_SEP_STRING "bin" U_FILE_SEP_STRING "pkgdata",
      200,
      U_ZERO_ERROR,
      U_FILE_ALT_SEP_STRING "usr" U_FILE_ALT_SEP_STRING "bin"
    },
    {
      U_FILE_ALT_SEP_STRING "usr" U_FILE_SEP_STRING "bin" U_FILE_ALT_SEP_STRING "pkgdata",
      200,
      U_ZERO_ERROR,
      U_FILE_ALT_SEP_STRING "usr" U_FILE_SEP_STRING "bin"
    },
    {
      U_FILE_ALT_SEP_STRING "usr" U_FILE_ALT_SEP_STRING "bin" U_FILE_ALT_SEP_STRING "pkgdata",
      2,
      U_BUFFER_OVERFLOW_ERROR,
      NULL
    },
    {
      U_FILE_ALT_SEP_STRING "vmlinuz",
      200,
      U_ZERO_ERROR,
      U_FILE_ALT_SEP_STRING
    },
    {
      U_FILE_SEP_STRING "vmlinux",
      200,
      U_ZERO_ERROR,
      U_FILE_SEP_STRING
    },
    {
      "pkgdata",
      0,
      U_BUFFER_OVERFLOW_ERROR,
      NULL
    },
    {
      "pkgdata",
      1,
      U_BUFFER_OVERFLOW_ERROR,
      NULL
    },
    {
      "pkgdata",
      2,
      U_ZERO_ERROR,
      "."
    },
    {
      "pkgdata",
      20,
      U_ZERO_ERROR,
      "."
    }
  };
  int32_t count=(sizeof(testCases)/sizeof(testCases[0]));

  log_verbose("Testing findDirname()\n");
  for(i=0;i<count;i++) {
    const char *result;
    const char *input = STRNULL(testCases[i].inBuf);
    const char *expect = STRNULL(testCases[i].expectResult);
    UErrorCode status = U_ZERO_ERROR;
    uprv_memset(toolutil_testBuf, 0x55, TOOLUTIL_TESTBUF_SIZE);
    
    log_verbose("Test case [%d/%d]: %s\n", i, count-1, input);
    result = STRNULL(findDirname(testCases[i].inBuf, toolutil_testBuf, testCases[i].outBufLen, &status));
    log_verbose(" -> %s, \n", u_errorName(status));
    if(status != testCases[i].expectStatus) {
      log_verbose("FAIL: Test case [%d/%d]: %s got error code %s but expected %s\n", i, count-1, input, u_errorName(status), u_errorName(testCases[i].expectStatus));
    }
    if(result==expect||!strcmp(result,expect)) {
      log_verbose(" = -> %s \n", result);
    } else {
      log_err("FAIL: Test case [%d/%d]: %s -> %s but expected %s\n", i, count-1, input, result, expect);
    }
  }
}



static void addToolUtilTests(TestNode** root) {
    addTest(root, &toolutil_findBasename,       "putiltst/toolutil/findBasename");
    addTest(root, &toolutil_findDirname,       "putiltst/toolutil/findDirname");
  /*
    Not yet tested:

    addTest(root, &toolutil_getLongPathname,       "putiltst/toolutil/getLongPathname");
    addTest(root, &toolutil_getCurrentYear,       "putiltst/toolutil/getCurrentYear");
    addTest(root, &toolutil_UToolMemory,       "putiltst/toolutil/UToolMemory");
  */
}
