/********************************************************************
 * COPYRIGHT: 
 * Copyright (c) 1997-2010, International Business Machines Corporation and
 * others. All Rights Reserved.
 ********************************************************************
 *
 * File CMSGTST.C
 *
 * Modification History:
 *        Name                     Description
 *     Madhu Katragadda              Creation
 ********************************************************************/
/* C API TEST FOR MESSAGE FORMAT */

#include "unicode/utypes.h"

#if !UCONFIG_NO_FORMATTING

#include <stdlib.h>
#include <string.h>
#include <stdarg.h>
#include "unicode/uloc.h"
#include "unicode/umsg.h"
#include "unicode/udat.h"
#include "unicode/umsg.h"
#include "unicode/ustring.h"
#include "cintltst.h"
#include "cmsgtst.h"
#include "cformtst.h"

static const char* const txt_testCasePatterns[] = {
   "Quotes '', '{', a {0,number,integer} '{'0}",
   "Quotes '', '{', a {0,number,integer} '{'0}",
   "You deposited {0,number,integer} times an amount of {1,number,currency} on {2,date,short}",
    "'{'2,time,full}, for {1, number }, {0,number,integer} is {2,time,full} and full date is {2,date,full}",
   "'{'1,number,percent} for {0,number,integer} is {1,number,percent}",
};

static const char* const txt_testResultStrings[] = {
    "Quotes ', {, a 1 {0}",
    "Quotes ', {, a 1 {0}",
    "You deposited 1 times an amount of $3,456.00 on 1/12/70",
    "{2,time,full}, for 3,456, 1 is 5:46:40 AM Pacific Standard Time and full date is Monday, January 12, 1970",
    "{1,number,percent} for 1 is 345,600%"
};

const int32_t cnt_testCases = 5;
static UChar* testCasePatterns[5];

static UChar* testResultStrings[5];

static UBool strings_initialized = FALSE;

/* function used to create the test patterns for testing Message formatting */
static void InitStrings( void )
{
    int32_t i;
    if (strings_initialized)
        return;

    for (i=0; i < cnt_testCases; i++ ) {
        uint32_t strSize = (uint32_t)strlen(txt_testCasePatterns[i]) + 1;
        testCasePatterns[i]=(UChar*)malloc(sizeof(UChar) * strSize);
        u_uastrncpy(testCasePatterns[i], txt_testCasePatterns[i], strSize);
    }
    for (i=0; i < cnt_testCases; i++ ) {
        uint32_t strSize = (uint32_t)strlen(txt_testResultStrings[i]) + 1;
        testResultStrings[i] = (UChar*)malloc(sizeof(UChar) * strSize);
        u_uastrncpy(testResultStrings[i], txt_testResultStrings[i], strSize);
    }

    strings_initialized = TRUE;
}

static void FreeStrings( void )
{
    int32_t i;
    if (!strings_initialized)
        return;

    for (i=0; i < cnt_testCases; i++ ) {
        free(testCasePatterns[i]);
    }
    for (i=0; i < cnt_testCases; i++ ) {
        free(testResultStrings[i]);
    }
    strings_initialized = FALSE;
}

/* Platform dependent test to detect if this type will return NULL when interpreted as a pointer. */
static UBool returnsNullForType(int firstParam, ...) {
    UBool isNULL;
    va_list marker;
    va_start(marker, firstParam);
    isNULL = (UBool)(va_arg(marker, void*) == NULL);
    va_end(marker);
    return isNULL;
}

/* Test u_formatMessage() with various test patterns() */
static void MessageFormatTest( void ) 
{
    UChar *str;
    UChar* result;
    int32_t resultLengthOut,resultlength,i, patternlength;
    UErrorCode status = U_ZERO_ERROR;
    UDate d1=1000000000.0;

    ctest_setTimeZone(NULL, &status);

    str=(UChar*)malloc(sizeof(UChar) * 7);
    u_uastrncpy(str, "MyDisk", 7);
    resultlength=1;
    result=(UChar*)malloc(sizeof(UChar) * 1);
    log_verbose("Testing u_formatMessage()\n");
    InitStrings();
    for (i = 0; i < cnt_testCases; i++) {
        status=U_ZERO_ERROR;
        patternlength=u_strlen(testCasePatterns[i]);
        resultLengthOut=u_formatMessage( "en_US",testCasePatterns[i], patternlength, result, resultlength, 
            &status, 1, 3456.00, d1);
        if(status== U_BUFFER_OVERFLOW_ERROR)
        {
            status=U_ZERO_ERROR;
            resultlength=resultLengthOut+1;
            result=(UChar*)realloc(result,sizeof(UChar) * resultlength);
            u_formatMessage( "en_US",testCasePatterns[i], patternlength, result, resultlength, 
                &status, 1, 3456.00, d1);
        }
        if(U_FAILURE(status)){
            log_data_err("ERROR: failure in message format on testcase %d:  %s (Are you missing data?)\n", i, myErrorName(status) );
            continue;
        }
        if(u_strcmp(result, testResultStrings[i])==0){
            log_verbose("PASS: MessagFormat successful on testcase : %d\n", i);
        }
        else{
            log_err("FAIL: Error in MessageFormat on testcase : %d\n GOT %s EXPECTED %s\n", i, 
                austrdup(result), austrdup(testResultStrings[i]) );
        }
    }
    free(result);
    result = NULL;
    free(str);
    {

         for (i = 0; i < cnt_testCases; i++) {
            UParseError parseError;
            status=U_ZERO_ERROR;
            patternlength=u_strlen(testCasePatterns[i]);
            resultlength=0;
            resultLengthOut=u_formatMessageWithError( "en_US",testCasePatterns[i], patternlength, result, resultlength, 
                &parseError,&status, 1, 3456.00, d1);
            if(status== U_BUFFER_OVERFLOW_ERROR)
            {
                status=U_ZERO_ERROR;
                resultlength=resultLengthOut+1;
                result=(UChar*)malloc(sizeof(UChar) * resultlength);
                u_formatMessage( "en_US",testCasePatterns[i], patternlength, result, resultlength, 
                    &status, 1, 3456.00, d1);
            }
            if(U_FAILURE(status)){
                log_data_err("ERROR: failure in message format on testcase %d:  %s (Are you missing data?)\n", i, myErrorName(status) );
                continue;
            }
            if(u_strcmp(result, testResultStrings[i])==0){
                log_verbose("PASS: MessagFormat successful on testcase : %d\n", i);
            }
            else{
                log_err("FAIL: Error in MessageFormat on testcase : %d\n GOT %s EXPECTED %s\n", i, 
                    austrdup(result), austrdup(testResultStrings[i]) );
            }
            free(result);
            result=NULL;
        }
    }
    {
        UErrorCode ec = U_ZERO_ERROR;
        int32_t patternLength = u_strlen(testCasePatterns[0]);
            
        UMessageFormat formatter = umsg_open(testCasePatterns[0],patternLength,"en_US",NULL,&ec);

        if(U_FAILURE(ec)){
            log_data_err("umsg_open() failed for testCasePattens[%d]. -> %s (Are you missing data?)\n",i, u_errorName(ec));
            return;
        }
        for(i = 0;i<cnt_testCases; i++){
            UParseError parseError;
            int32_t resultLength =0,count=0;
            int32_t one=0;
            int32_t two=0;
            UDate d2=0;
    
            result=NULL;
            patternLength = u_strlen(testCasePatterns[i]);
            
            umsg_applyPattern(formatter,testCasePatterns[i],patternLength,&parseError,&ec);
            if(U_FAILURE(ec)){
                log_err("umsg_applyPattern() failed for testCasePattens[%d].\n",i);
                return;
            }
            /* pre-flight */
            resultLength = umsg_format(formatter,result,resultLength,&ec,1,3456.00,d1);
            if(ec==U_BUFFER_OVERFLOW_ERROR){
                ec=U_ZERO_ERROR;
                result = (UChar*) malloc(U_SIZEOF_UCHAR*resultLength+2);
                resultLength =  umsg_format(formatter,result,resultLength+2,&ec,1,3456.00,d1);
                if(U_FAILURE(ec)){
                      log_err("ERROR: failure in message format on testcase %d:  %s\n", i, u_errorName(status) );
                      free(result);
                      return;
                }
            
                if(u_strcmp(result, testResultStrings[i])==0){
                    log_verbose("PASS: MessagFormat successful on testcase : %d\n", i);
                }
                else{
                    log_err("FAIL: Error in MessageFormat on testcase : %d\n GOT %s EXPECTED %s\n", i, 
                        austrdup(result), austrdup(testResultStrings[i]) );
                }

#if defined (U_DARWIN)  /* add platforms here .. */
                log_verbose("Skipping potentially crashing test for mismatched varargs.\n");
#else
                log_verbose("Note: the next is a platform dependent test. If it crashes, add an exclusion for your platform near %s:%d\n", __FILE__, __LINE__); 

                if (returnsNullForType(1, (double)2.0)) {
                    /* HP/UX and possibly other platforms don't properly check for this case.
                    We pass in a UDate, but the function expects a UDate *.  When va_arg is used,
                    most compilers will return NULL, but HP-UX won't do that and will return 2
                    in this case.  This is a platform dependent test.  It crashes on some systems.
            
                    If you get a crash here, see the definition of returnsNullForType.

                    This relies upon "undefined" behavior, as indicated by C99 7.15.1.1 paragraph 2
                    */
                    umsg_parse(formatter,result,resultLength,&count,&ec,one,two,d2);
                    if(ec!=U_ILLEGAL_ARGUMENT_ERROR){
                        log_err("FAIL: Did not get expected error for umsg_parse(). Expected: U_ILLEGAL_ARGUMENT_ERROR Got: %s \n",u_errorName(ec));
                    }else{
                        ec = U_ZERO_ERROR;
                    }
                }
                else {
                    log_verbose("Warning: Returning NULL for a mismatched va_arg type isn't supported on this platform.\n", i);
                }
#endif

                umsg_parse(formatter,result,resultLength,&count,&ec,&one,&two,&d2);
                if(U_FAILURE(ec)){
                    log_err("umsg_parse could not parse the pattern. Error: %s.\n",u_errorName(ec));
                }
                free(result);
            }else{
                log_err("FAIL: Expected U_BUFFER_OVERFLOW error while preflighting got: %s for testCasePatterns[%d]",u_errorName(ec),i);
            }
        }
        umsg_close(formatter);
    }
    FreeStrings();

    ctest_resetTimeZone();
}


/*test u_formatMessage() with sample patterns */
static void TestSampleMessageFormat(void)
{
    UChar *str;
    UChar *result;
    UChar pattern[100], expected[100];
    int32_t resultLengthOut, resultlength;
    UDate d = 837039928046.0;
    UErrorCode status = U_ZERO_ERROR;

    ctest_setTimeZone(NULL, &status);

    str=(UChar*)malloc(sizeof(UChar) * 15);
    u_uastrcpy(str, "abc");    
    
    u_uastrcpy(pattern, "There are {0} files on {1,date}");
    u_uastrcpy(expected, "There are abc files on Jul 10, 1996");
    result=(UChar*)malloc(sizeof(UChar) * 1);
    log_verbose("\nTesting a sample for Message format test#1\n");
    resultlength=1;
    resultLengthOut=u_formatMessage( "en_US", pattern, u_strlen(pattern), result, resultlength, &status, str, d);
    if(status==U_BUFFER_OVERFLOW_ERROR)
    {
        status=U_ZERO_ERROR;
        resultlength=resultLengthOut+1;
        result=(UChar*)realloc(result, sizeof(UChar) * resultlength);
        u_formatMessage( "en_US", pattern, u_strlen(pattern), result, resultlength, &status, str, d);
    }
    if(U_FAILURE(status)){
        log_data_err("Error: failure in message format on test#1: %s (Are you missing data?)\n", myErrorName(status));
    }
    else if(u_strcmp(result, expected)==0)
        log_verbose("PASS: MessagFormat successful on test#1\n");
    else{
        log_err("FAIL: Error in MessageFormat on test#1 \n GOT: %s EXPECTED: %s\n", 
            austrdup(result), austrdup(expected) );
    }


    log_verbose("\nTesting message format with another pattern test#2\n");
    u_uastrcpy(pattern, "The disk \"{0}\" contains {1,number,integer} file(s)");
    u_uastrcpy(expected, "The disk \"MyDisk\" contains 23 file(s)");
    u_uastrcpy(str, "MyDisk");

    resultLengthOut=u_formatMessage( "en_US", 
        pattern, 
        u_strlen(pattern),
        result,
        resultlength,
        &status, 
        str,
        235);
    if(status==U_BUFFER_OVERFLOW_ERROR)
    {
        status=U_ZERO_ERROR;
        resultlength=resultLengthOut+1;
        result=(UChar*)realloc(result, sizeof(UChar) * (resultlength+1));
        u_formatMessage( "en_US", pattern, u_strlen(pattern), result, resultlength, &status, str, 23);
    }
    if(U_FAILURE(status)){
        log_data_err("Error: failure in message format on test#2 : %s (Are you missing data?)\n", myErrorName(status));
    }
    else if(u_strcmp(result, expected)==0)
        log_verbose("PASS: MessagFormat successful on test#2\n");
    else{
        log_err("FAIL: Error in MessageFormat on test#2\n GOT: %s EXPECTED: %s\n", 
            austrdup(result), austrdup(expected) );
    }



    log_verbose("\nTesting message format with another pattern test#3\n");
    u_uastrcpy(pattern, "You made a {0} of {1,number,currency}");
    u_uastrcpy(expected, "You made a deposit of $500.00");
    u_uastrcpy(str, "deposit");
    resultlength=0;
    resultLengthOut=u_formatMessage( "en_US", pattern, u_strlen(pattern), NULL, resultlength, &status, str, 500.00);
    if(status==U_BUFFER_OVERFLOW_ERROR)
    {
        status=U_ZERO_ERROR;
        resultlength=resultLengthOut+1;
        result=(UChar*)realloc(result, sizeof(UChar) * resultlength);
        u_formatMessage( "en_US", pattern, u_strlen(pattern), result, resultlength, &status, str, 500.00);
    }
    if(U_FAILURE(status)){
        log_data_err("Error: failure in message format on test#3 : %s (Are you missing data?)\n", myErrorName(status));
    }
    else if(u_strcmp(result, expected)==0)
        log_verbose("PASS: MessagFormat successful on test#3\n");
    else{
        log_err("FAIL: Error in MessageFormat on test#3\n GOT: %s EXPECTED %s\n", austrdup(result), 
            austrdup(expected) );
    }
    
    free(result);
    free(str);

    ctest_resetTimeZone();
}

/* Test umsg_format() and umsg_parse() , format and parse sequence and round trip */
static void TestNewFormatAndParseAPI(void)
{

    UChar *result, tzID[4], str[25];
    UChar pattern[100];
    UChar expected[100];
    int32_t resultLengthOut, resultlength;
    UCalendar *cal;
    UDate d1,d;
    UDateFormat *def1;
    UErrorCode status = U_ZERO_ERROR;
    int32_t value = 0;
    UChar ret[30];
    UParseError parseError;
    UMessageFormat* fmt = NULL;
    int32_t count=0;

    ctest_setTimeZone(NULL, &status);

    log_verbose("Testing format and parse with parse error\n");

    u_uastrcpy(str, "disturbance in force");
    u_uastrcpy(tzID, "PST");
    cal=ucal_open(tzID, u_strlen(tzID), "en_US", UCAL_TRADITIONAL, &status);
    if(U_FAILURE(status)){
        log_data_err("error in ucal_open caldef : %s - (Are you missing data?)\n", myErrorName(status) );
        return;
    }
    ucal_setDateTime(cal, 1999, UCAL_MARCH, 18, 0, 0, 0, &status);
    d1=ucal_getMillis(cal, &status);
    if(U_FAILURE(status)){
            log_err("Error: failure in get millis: %s\n", myErrorName(status) );
            return;
    }
    
    log_verbose("\nTesting with pattern test#4");
    u_uastrcpy(pattern, "On {0, date, long}, there was a {1} on planet {2,number,integer}");
    u_uastrcpy(expected, "On March 18, 1999, there was a disturbance in force on planet 7"); 
    resultlength=1;
    fmt = umsg_open(pattern,u_strlen(pattern),"en_US",&parseError,&status);
    if(U_FAILURE(status)){
        log_data_err("error in umsg_open  : %s (Are you missing data?)\n", u_errorName(status) );
        return;
    }
    result=(UChar*)malloc(sizeof(UChar) * resultlength);
    
    resultLengthOut=umsg_format(fmt ,result, resultlength,&status, d1, str, 7);
    if(status==U_BUFFER_OVERFLOW_ERROR)
    {
        status=U_ZERO_ERROR;
        resultlength=resultLengthOut+1;
        result=(UChar*)realloc(result, sizeof(UChar) * resultlength);
        u_formatMessageWithError( "en_US", pattern, u_strlen(pattern), result, resultlength,&parseError, &status, d1, str, 7);
        
    }
    if(U_FAILURE(status)){
        log_err("ERROR: failure in message format test#4: %s\n", myErrorName(status));
    }
    if(u_strcmp(result, expected)==0)
        log_verbose("PASS: MessagFormat successful on test#4\n");
    else{
        log_err("FAIL: Error in MessageFormat on test#4\n GOT: %s EXPECTED: %s\n", austrdup(result),
            austrdup(expected) );
    }
    
    
    /*try to parse this and check*/
    log_verbose("\nTesting the parse Message test#5\n");

    umsg_parse(fmt, result, u_strlen(result),&count,&status, &d, ret, &value);
    if(U_FAILURE(status)){
        log_err("ERROR: error in parsing: test#5: %s\n", myErrorName(status));
    }
    if(value!=7 && u_strcmp(str,ret)!=0)
        log_err("FAIL: Error in parseMessage on test#5 \n");
    else
        log_verbose("PASS: parseMessage successful on test#5\n");
        
    def1 = udat_open(UDAT_DEFAULT,UDAT_DEFAULT ,NULL, NULL, 0, NULL,0,&status);
    if(U_FAILURE(status))
    {
        log_err("error in creating the dateformat using short date and time style:\n %s\n", myErrorName(status));
    }else{

        if(u_strcmp(myDateFormat(def1, d), myDateFormat(def1, d1))==0)
            log_verbose("PASS: parseMessage successful test#5\n");
        else{
            log_err("FAIL: parseMessage didn't parse the date successfully\n GOT: %s EXPECTED %s\n", 
                austrdup(myDateFormat(def1,d)), austrdup(myDateFormat(def1,d1)) );
        }
    }
    umsg_close(fmt);
    udat_close(def1);
    ucal_close(cal);

    free(result);

    ctest_resetTimeZone();
}

/* Test u_formatMessageWithError() and u_parseMessageWithError() , format and parse sequence and round trip */
static void TestSampleFormatAndParseWithError(void)
{

    UChar *result, *tzID, *str;
    UChar pattern[100];

    UChar expected[100];
    int32_t resultLengthOut, resultlength;
    UCalendar *cal;
    UDate d1,d;
    UDateFormat *def1;
    UErrorCode status = U_ZERO_ERROR;
    int32_t value = 0;
    UChar ret[30];
    UParseError parseError;

    ctest_setTimeZone(NULL, &status);

    log_verbose("Testing format and parse with parse error\n");

    str=(UChar*)malloc(sizeof(UChar) * 25);
    u_uastrcpy(str, "disturbance in force");
    tzID=(UChar*)malloc(sizeof(UChar) * 4);
    u_uastrcpy(tzID, "PST");
    cal=ucal_open(tzID, u_strlen(tzID), "en_US", UCAL_TRADITIONAL, &status);
    if(U_FAILURE(status)){
        log_data_err("error in ucal_open caldef : %s - (Are you missing data?)\n", myErrorName(status) );
    }
    ucal_setDateTime(cal, 1999, UCAL_MARCH, 18, 0, 0, 0, &status);
    d1=ucal_getMillis(cal, &status);
    if(U_FAILURE(status)){
            log_data_err("Error: failure in get millis: %s - (Are you missing data?)\n", myErrorName(status) );
    }
    
    log_verbose("\nTesting with pattern test#4");
    u_uastrcpy(pattern, "On {0, date, long}, there was a {1} on planet {2,number,integer}");
    u_uastrcpy(expected, "On March 18, 1999, there was a disturbance in force on planet 7"); 
    resultlength=1;
    result=(UChar*)malloc(sizeof(UChar) * resultlength);
    resultLengthOut=u_formatMessageWithError( "en_US", pattern, u_strlen(pattern), result, resultlength,&parseError, &status, d1, str, 7);
    if(status==U_BUFFER_OVERFLOW_ERROR)
    {
        status=U_ZERO_ERROR;
        resultlength=resultLengthOut+1;
        result=(UChar*)realloc(result, sizeof(UChar) * resultlength);
        u_formatMessageWithError( "en_US", pattern, u_strlen(pattern), result, resultlength,&parseError, &status, d1, str, 7);
        
    }
    if(U_FAILURE(status)){
        log_data_err("ERROR: failure in message format test#4: %s (Are you missing data?)\n", myErrorName(status));
    }
    else if(u_strcmp(result, expected)==0)
        log_verbose("PASS: MessagFormat successful on test#4\n");
    else{
        log_err("FAIL: Error in MessageFormat on test#4\n GOT: %s EXPECTED: %s\n", austrdup(result),
            austrdup(expected) );
    }
    
    
    /*try to parse this and check*/
    log_verbose("\nTesting the parse Message test#5\n");

    u_parseMessageWithError("en_US", pattern, u_strlen(pattern), result, u_strlen(result), &parseError,&status, &d, ret, &value);
    if(U_FAILURE(status)){
        log_data_err("ERROR: error in parsing: test#5: %s (Are you missing data?)\n", myErrorName(status));
    }
    else if(value!=7 && u_strcmp(str,ret)!=0)
        log_err("FAIL: Error in parseMessage on test#5 \n");
    else
        log_verbose("PASS: parseMessage successful on test#5\n");
        
    def1 = udat_open(UDAT_DEFAULT,UDAT_DEFAULT ,NULL, NULL, 0, NULL,0,&status);
    if(U_FAILURE(status))
    {
        log_data_err("error in creating the dateformat using short date and time style: %s (Are you missing data?)\n", myErrorName(status));
    }else{

        if(u_strcmp(myDateFormat(def1, d), myDateFormat(def1, d1))==0)
            log_verbose("PASS: parseMessage successful test#5\n");
        else{
            log_err("FAIL: parseMessage didn't parse the date successfully\n GOT: %s EXPECTED %s\n", 
                austrdup(myDateFormat(def1,d)), austrdup(myDateFormat(def1,d1)) );
        }
    }
    udat_close(def1);
    ucal_close(cal);

    free(result);
    free(str);
    free(tzID);
    
    ctest_resetTimeZone();
}

/* Test u_formatMessage() and u_parseMessage() , format and parse sequence and round trip */
static void TestSampleFormatAndParse(void)
{

    UChar *result, *tzID, *str;
    UChar pattern[100];
    UChar expected[100];
    int32_t resultLengthOut, resultlength;
    UCalendar *cal;
    UDate d1,d;
    UDateFormat *def1;
    UErrorCode status = U_ZERO_ERROR;
    int32_t value = 0;
    UChar ret[30];

    ctest_setTimeZone(NULL, &status);

    log_verbose("Testing format and parse\n");

    str=(UChar*)malloc(sizeof(UChar) * 25);
    u_uastrcpy(str, "disturbance in force");
    tzID=(UChar*)malloc(sizeof(UChar) * 4);
    u_uastrcpy(tzID, "PST");
    cal=ucal_open(tzID, u_strlen(tzID), "en_US", UCAL_TRADITIONAL, &status);
    if(U_FAILURE(status)){
        log_data_err("error in ucal_open caldef : %s - (Are you missing data?)\n", myErrorName(status) );
    }
    ucal_setDateTime(cal, 1999, UCAL_MARCH, 18, 0, 0, 0, &status);
    d1=ucal_getMillis(cal, &status);
    if(U_FAILURE(status)){
            log_data_err("Error: failure in get millis: %s - (Are you missing data?)\n", myErrorName(status) );
    }
    
    log_verbose("\nTesting with pattern test#4");
    u_uastrcpy(pattern, "On {0, date, long}, there was a {1} on planet {2,number,integer}");
    u_uastrcpy(expected, "On March 18, 1999, there was a disturbance in force on planet 7"); 
    resultlength=1;
    result=(UChar*)malloc(sizeof(UChar) * resultlength);
    resultLengthOut=u_formatMessage( "en_US", pattern, u_strlen(pattern), result, resultlength, &status, d1, str, 7);
    if(status==U_BUFFER_OVERFLOW_ERROR)
    {
        status=U_ZERO_ERROR;
        resultlength=resultLengthOut+1;
        result=(UChar*)realloc(result, sizeof(UChar) * resultlength);
        u_formatMessage( "en_US", pattern, u_strlen(pattern), result, resultlength, &status, d1, str, 7);
        
    }
    if(U_FAILURE(status)){
        log_data_err("ERROR: failure in message format test#4: %s (Are you missing data?)\n", myErrorName(status));
    }
    else if(u_strcmp(result, expected)==0)
        log_verbose("PASS: MessagFormat successful on test#4\n");
    else{
        log_err("FAIL: Error in MessageFormat on test#4\n GOT: %s EXPECTED: %s\n", austrdup(result),
            austrdup(expected) );
    }
    
    
    /*try to parse this and check*/
    log_verbose("\nTesting the parse Message test#5\n");

    u_parseMessage("en_US", pattern, u_strlen(pattern), result, u_strlen(result), &status, &d, ret, &value);
    if(U_FAILURE(status)){
        log_data_err("ERROR: error in parsing: test#5: %s (Are you missing data?)\n", myErrorName(status));
    }
    else if(value!=7 && u_strcmp(str,ret)!=0)
        log_err("FAIL: Error in parseMessage on test#5 \n");
    else
        log_verbose("PASS: parseMessage successful on test#5\n");
        
    def1 = udat_open(UDAT_DEFAULT,UDAT_DEFAULT ,NULL, NULL, 0, NULL,0,&status);
    if(U_FAILURE(status))
    {
        log_data_err("error in creating the dateformat using short date and time style: %s (Are you missing data?)\n", myErrorName(status));
    }else{

        if(u_strcmp(myDateFormat(def1, d), myDateFormat(def1, d1))==0)
            log_verbose("PASS: parseMessage successful test#5\n");
        else{
            log_err("FAIL: parseMessage didn't parse the date successfully\n GOT: %s EXPECTED %s\n", 
                austrdup(myDateFormat(def1,d)), austrdup(myDateFormat(def1,d1)) );
        }
    }
    udat_close(def1);
    ucal_close(cal);

    free(result);
    free(str);
    free(tzID);
    
    ctest_resetTimeZone();
}

/* Test message format with a Select option */
static void TestMsgFormatSelect(void)
{
    UChar* str;
    UChar* str1;
    UErrorCode status = U_ZERO_ERROR;
    UChar *result;
    UChar pattern[100];
    UChar expected[100];
    int32_t resultlength,resultLengthOut;

    str=(UChar*)malloc(sizeof(UChar) * 25);
    u_uastrcpy(str, "Kirti");
    str1=(UChar*)malloc(sizeof(UChar) * 25);
    u_uastrcpy(str1, "female");
    log_verbose("Testing message format with Select test #1\n:");
    u_uastrcpy(pattern, "{0} est {1, select, female {all\\u00E9e} other {all\\u00E9}} \\u00E0 Paris.");
    u_uastrcpy(expected, "Kirti est all\\u00E9e \\u00E0 Paris.");
    resultlength=0;
    resultLengthOut=u_formatMessage( "fr", pattern, u_strlen(pattern), NULL, resultlength, &status, str , str1);
    if(status==U_BUFFER_OVERFLOW_ERROR)
    {
        status=U_ZERO_ERROR;
        resultlength=resultLengthOut+1;
        result=(UChar*)malloc(sizeof(UChar) * resultlength);
        u_formatMessage( "fr", pattern, u_strlen(pattern), result, resultlength, &status, str , str1);
        if(u_strcmp(result, expected)==0)
            log_verbose("PASS: MessagFormat successful on Select test#1\n");
        else{
            log_err("FAIL: Error in MessageFormat on Select test#1\n GOT %s EXPECTED %s\n", austrdup(result),
                austrdup(expected) );
        }
        free(result);
    }
    if(U_FAILURE(status)){
        log_data_err("ERROR: failure in message format on Select test#1 : %s \n", myErrorName(status));
    }
    free(str);
    free(str1);

    /*Test a nested pattern*/
    str=(UChar*)malloc(sizeof(UChar) * 25);
    u_uastrcpy(str, "Noname");
    str1=(UChar*)malloc(sizeof(UChar) * 25);
    u_uastrcpy(str1, "other");
    log_verbose("Testing message format with Select test #2\n:");
    u_uastrcpy(pattern, "{0} est {1, select, female {{2,number,integer} all\\u00E9e} other {all\\u00E9}} \\u00E0 Paris.");
    u_uastrcpy(expected, "Noname est all\\u00E9 \\u00E0 Paris.");
    resultlength=0;
    resultLengthOut=u_formatMessage( "fr", pattern, u_strlen(pattern), NULL, resultlength, &status, str , str1,6);
    if(status==U_BUFFER_OVERFLOW_ERROR)
    {
        status=U_ZERO_ERROR;
        resultlength=resultLengthOut+1;
        result=(UChar*)malloc(sizeof(UChar) * resultlength);
        u_formatMessage( "fr", pattern, u_strlen(pattern), result, resultlength, &status, str , str1);
        if(u_strcmp(result, expected)==0)
            log_verbose("PASS: MessagFormat successful on Select test#2\n");
        else{
            log_err("FAIL: Error in MessageFormat on Select test#2\n GOT %s EXPECTED %s\n", austrdup(result),
                austrdup(expected) );
        }
        free(result);
    }
    if(U_FAILURE(status)){
        log_data_err("ERROR: failure in message format on Select test#2 : %s \n", myErrorName(status));
    }
    free(str);
    free(str1);
}

/* test message format with a choice option */
static void TestMsgFormatChoice(void)
{
    UChar* str;
    UErrorCode status = U_ZERO_ERROR;
    UChar *result;
    UChar pattern[100];
    UChar expected[100];
    int32_t resultlength,resultLengthOut;
    
    str=(UChar*)malloc(sizeof(UChar) * 25);
    u_uastrcpy(str, "MyDisk");
    log_verbose("Testing message format with choice test #6\n:");
    /*There {0,choice,0#are no files|1#is one file|1<are {0,number,integer} files}.*/
    u_uastrcpy(pattern, "The disk {1} contains {0,choice,0#no files|1#one file|1<{0,number,integer} files}");
    u_uastrcpy(expected, "The disk MyDisk contains 100 files");
    resultlength=0;
    resultLengthOut=u_formatMessage( "en_US", pattern, u_strlen(pattern), NULL, resultlength, &status, 100., str);
    if(status==U_BUFFER_OVERFLOW_ERROR)
    {
        status=U_ZERO_ERROR;
        resultlength=resultLengthOut+1;
        result=(UChar*)malloc(sizeof(UChar) * resultlength);
        u_formatMessage( "en_US", pattern, u_strlen(pattern), result, resultlength, &status, 100., str);
        if(u_strcmp(result, expected)==0)
            log_verbose("PASS: MessagFormat successful on test#6\n");
        else{
            log_err("FAIL: Error in MessageFormat on test#6\n GOT %s EXPECTED %s\n", austrdup(result),
                austrdup(expected) );
        }
        free(result);
    }
    if(U_FAILURE(status)){
        log_data_err("ERROR: failure in message format on test#6 : %s (Are you missing data?)\n", myErrorName(status));
    }

    log_verbose("Testing message format with choice test #7\n:");
    u_uastrcpy(expected, "The disk MyDisk contains no files");
    resultlength=0;
    resultLengthOut=u_formatMessage( "en_US", pattern, u_strlen(pattern), NULL, resultlength, &status, 0., str);
    if(status==U_BUFFER_OVERFLOW_ERROR)
    {
        status=U_ZERO_ERROR;
        resultlength=resultLengthOut+1;
        result=(UChar*)malloc(sizeof(UChar) * resultlength);
        u_formatMessage( "en_US", pattern, u_strlen(pattern), result, resultlength, &status, 0., str);

        if(u_strcmp(result, expected)==0)
            log_verbose("PASS: MessagFormat successful on test#7\n");
        else{
            log_err("FAIL: Error in MessageFormat on test#7\n GOT: %s EXPECTED %s\n", austrdup(result), 
                austrdup(expected) );
        }
        free(result);
    }
    if(U_FAILURE(status)){
        log_data_err("ERROR: failure in message format on test#7 : %s (Are you missing data?)\n", myErrorName(status));
    }

    log_verbose("Testing message format with choice test #8\n:");
    u_uastrcpy(expected, "The disk MyDisk contains one file");
    resultlength=0;
    resultLengthOut=u_formatMessage( "en_US", pattern, u_strlen(pattern), NULL, resultlength, &status, 1., str);
    if(status==U_BUFFER_OVERFLOW_ERROR)
    {
        status=U_ZERO_ERROR;
        resultlength=resultLengthOut+1;
        result=(UChar*)malloc(sizeof(UChar) * resultlength);
        u_formatMessage( "en_US", pattern, u_strlen(pattern), result, resultlength, &status, 1., str);

        if(u_strcmp(result, expected)==0)
            log_verbose("PASS: MessagFormat successful on test#8\n");
        else{
            log_err("FAIL: Error in MessageFormat on test#8\n GOT %s EXPECTED: %s\n", austrdup(result), 
                austrdup(expected) );
        }

        free(result);
    }
    if(U_FAILURE(status)){
        log_data_err("ERROR: failure in message format on test#8 : %s (Are you missing data?)\n", myErrorName(status));
    }

    free(str);

}

/*test u_parseMessage() with various test patterns */
static void TestParseMessage(void)
{
    UChar pattern[100];
    UChar source[100];
    UErrorCode status = U_ZERO_ERROR;
    int32_t value;
    UChar str[10];
    UChar res[10];
        
    log_verbose("\nTesting a sample for parse Message test#9\n");
    
    u_uastrcpy(source, "You deposited an amount of $500.00");
    u_uastrcpy(pattern, "You {0} an amount of {1,number,currency}");
    u_uastrcpy(res,"deposited");
        
    u_parseMessage( "en_US", pattern, u_strlen(pattern), source, u_strlen(source), &status, str, &value);
    if(U_FAILURE(status)){
        log_data_err("ERROR: failure in parse Message on test#9: %s (Are you missing data?)\n", myErrorName(status));
    }
    else if(value==500.00  && u_strcmp(str,res)==0)
        log_verbose("PASS: parseMessage successful on test#9\n");
    else
        log_err("FAIL: Error in parseMessage on test#9 \n");

    
    
    log_verbose("\nTesting a sample for parse Message test#10\n");
    
    u_uastrcpy(source, "There are 123 files on MyDisk created");
    u_uastrcpy(pattern, "There are {0,number,integer} files on {1} created");
    u_uastrcpy(res,"MyDisk");
        
    u_parseMessage( "en_US", pattern, u_strlen(pattern), source, u_strlen(source), &status, &value, str);
    if(U_FAILURE(status)){
        log_data_err("ERROR: failure in parse Message on test#10: %s (Are you missing data?)\n", myErrorName(status));
    }
    else if(value==123.00 && u_strcmp(str,res)==0)
        log_verbose("PASS: parseMessage successful on test#10\n");
    else
        log_err("FAIL: Error in parseMessage on test#10 \n");
}

static int32_t CallFormatMessage(const char* locale, UChar* testCasePattern, int32_t patternLength, 
                       UChar* result, int32_t resultLength, UErrorCode *status, ...)
{
    int32_t len = 0;
    va_list ap;
    va_start(ap, status);
    len = u_vformatMessage(locale, testCasePattern, patternLength, result, resultLength, ap, status);
    va_end(ap);
    return len;
}

/* Test u_vformatMessage() with various test patterns. */
static void TestMessageFormatWithValist( void ) 
{

    UChar *str;
    UChar* result;
    int32_t resultLengthOut,resultlength,i, patternlength;
    UErrorCode status = U_ZERO_ERROR;
    UDate d1=1000000000.0;

    ctest_setTimeZone(NULL, &status);

    str=(UChar*)malloc(sizeof(UChar) * 7);
    u_uastrcpy(str, "MyDisk");
    resultlength=1;
    result=(UChar*)malloc(sizeof(UChar) * 1);
    log_verbose("Testing u_formatMessage90\n");
    InitStrings();
    for (i = 0; i < cnt_testCases; i++) {
        status=U_ZERO_ERROR;
        patternlength=u_strlen(testCasePatterns[i]);
        resultLengthOut=CallFormatMessage( "en_US",testCasePatterns[i], patternlength, result, resultlength, 
            &status, 1, 3456.00, d1);
        if(status== U_BUFFER_OVERFLOW_ERROR)
        {
            status=U_ZERO_ERROR;
            resultlength=resultLengthOut+1;
            result=(UChar*)realloc(result,sizeof(UChar) * resultlength);
            CallFormatMessage( "en_US",testCasePatterns[i], patternlength, result, resultlength, 
                &status, 1, 3456.00, d1);
        }
        if(U_FAILURE(status)){
            log_data_err("ERROR: failure in message format on testcase %d:  %s (Are you missing data?)\n", i, myErrorName(status) );
        }
        else if(u_strcmp(result, testResultStrings[i])==0){
            log_verbose("PASS: MessagFormat successful on testcase : %d\n", i);
        }
        else{
            log_err("FAIL: Error in MessageFormat on testcase : %d\n GOT %s EXPECTED %s\n", i, 
                austrdup(result), austrdup(testResultStrings[i]) );
        }
    }
    free(result);
    free(str);
    FreeStrings();

    ctest_resetTimeZone();
}

static void CallParseMessage(const char* locale, UChar* pattern, int32_t patternLength, 
                       UChar* source, int32_t sourceLength, UErrorCode *status, ...)
{
    va_list ap;
    va_start(ap, status);
    u_vparseMessage(locale, pattern, patternLength, source, sourceLength, ap, status);
    va_end(ap);
}

/*test u_vparseMessage() with various test patterns */
static void TestParseMessageWithValist(void)
{
    UChar pattern[100];
    UChar source[100];
    UErrorCode status = U_ZERO_ERROR;
    int32_t value;
    UChar str[10];
    UChar res[10];

    log_verbose("\nTesting a sample for parse Message test#9\n");

    u_uastrcpy(source, "You deposited an amount of $500.00");
    u_uastrcpy(pattern, "You {0} an amount of {1,number,currency}");
    u_uastrcpy(res,"deposited");

    CallParseMessage( "en_US", pattern, u_strlen(pattern), source, u_strlen(source), &status, str, &value);
    if(U_FAILURE(status)){
        log_data_err("ERROR: failure in parse Message on test#9: %s (Are you missing data?)\n", myErrorName(status));
    }
    else if(value==500.00  && u_strcmp(str,res)==0)
        log_verbose("PASS: parseMessage successful on test#9\n");
    else
        log_err("FAIL: Error in parseMessage on test#9\n");


    log_verbose("\nTesting a sample for parse Message test#10\n");

    u_uastrcpy(source, "There are 123 files on MyDisk created");
    u_uastrcpy(pattern, "There are {0,number,integer} files on {1} created");
    u_uastrcpy(res,"MyDisk");

    CallParseMessage( "en_US", pattern, u_strlen(pattern), source, u_strlen(source), &status, &value, str);
    if(U_FAILURE(status)){
        log_data_err("ERROR: failure in parse Message on test#10: %s (Are you missing data?)\n", myErrorName(status));
    }
    else if(value==123.00 && u_strcmp(str,res)==0)
        log_verbose("PASS: parseMessage successful on test#10\n");
    else
        log_err("FAIL: Error in parseMessage on test#10 \n");    
}

/**
 * Regression test for ICU4C Jitterbug 904
 */
static void TestJ904(void) {
    UChar pattern[256];
    UChar result[256];
    UChar string[16];
    char cresult[256];
    int32_t length;
    UErrorCode status = U_ZERO_ERROR;
    const char* PAT = "Number {1,number,#0.000}, String {0}, Date {2,date,12:mm:ss.SSS}";
    const char* EXP = "Number 0,143, String foo, Date 12:34:56.789";

    ctest_setTimeZone(NULL, &status);

    u_uastrcpy(string, "foo");
    /* Slight hack here -- instead of date pattern HH:mm:ss.SSS, use
     * 12:mm:ss.SSS.  Why?  So this test generates the same output --
     * "12:34:56.789" -- regardless of time zone (as long as we aren't
     * in one of the 30 minute offset zones!). */
    u_uastrcpy(pattern, PAT);
    length = u_formatMessage("nl", pattern, u_strlen(pattern),
                             result, 256, &status,
                             string, 1/7.0,
                             789.0+1000*(56+60*(34+60*12)));

    u_austrncpy(cresult, result, sizeof(cresult));

    /* This test passes if it DOESN'T CRASH.  However, we test the
     * output anyway.  If the string doesn't match in the date part,
     * check to see that the machine doesn't have an unusual time zone
     * offset, that is, one with a non-zero minutes/seconds offset
     * from GMT -- see above. */
    if (strcmp(cresult, EXP) == 0) {
        log_verbose("Ok: \"%s\"\n", cresult);
    } else {
        log_data_err("FAIL: got \"%s\", expected \"%s\" -> %s (Are you missing data?)\n", cresult, EXP, u_errorName(status));
    }

    ctest_resetTimeZone();
}

static void OpenMessageFormatTest(void)
{
    UMessageFormat *f1, *f2, *f3;
    UChar pattern[256];
    UChar result[256];
    char cresult[256];
    UParseError parseError;
    const char* locale = "hi_IN";
    char* retLoc;
    const char* PAT = "Number {1,number,#0.000}, String {0}, Date {2,date,12:mm:ss.SSS}";
    int32_t length=0;
    UErrorCode status = U_ZERO_ERROR;

    u_uastrncpy(pattern, PAT, sizeof(pattern)/sizeof(pattern[0]));

    /* Test umsg_open                   */
    f1 = umsg_open(pattern,length,NULL,NULL,&status);
    
    if(U_FAILURE(status))
    {
        log_err("umsg_open failed with pattern %s. Error: \n", PAT, u_errorName(status));
        return;
    }
   
    /* Test umsg_open with parse error  */
    status = U_ZERO_ERROR;
    f2 = umsg_open(pattern,length,NULL,&parseError,&status);
    
    if(U_FAILURE(status))
    {
        log_err("umsg_open with parseError failed with pattern %s. Error: %s\n", PAT, u_errorName(status));
        return;
    }
    
    /* Test umsg_clone                  */
    status = U_ZERO_ERROR;
    f3 = umsg_clone(f1,&status);
    if(U_FAILURE(status))
    {
        log_err("umsg_clone failed. Error %s \n", u_errorName(status));
    }
   
    /* Test umsg_setLocale              */
    umsg_setLocale(f1,locale);
    /* Test umsg_getLocale              */
    retLoc = (char*)umsg_getLocale(f1);
    if(strcmp(retLoc,locale)!=0)
    {
        log_err("umsg_setLocale and umsg_getLocale methods failed. Expected:%s Got: %s \n", locale, retLoc);
    }

    /* Test umsg_applyPattern           */
    status = U_ZERO_ERROR;
    umsg_applyPattern(f1,pattern,(int32_t)strlen(PAT),NULL,&status);
    if(U_FAILURE(status))
    {
        log_data_err("umsg_applyPattern failed. Error %s (Are you missing data?)\n",u_errorName(status));
    }

    /* Test umsg_toPattern              */
    umsg_toPattern(f1,result,256,&status);
    if(U_FAILURE(status) ){
        log_data_err("umsg_toPattern method failed. Error: %s (Are you missing data?)\n",u_errorName(status));
    } else {
        if(u_strcmp(result,pattern)!=0){
            u_UCharsToChars(result,cresult,256);
            log_err("umsg_toPattern method failed. Expected: %s Got: %s \n",PAT,cresult);
        }
    }
    /* umsg_format umsg_parse */

    umsg_close(f1);
    umsg_close(f2);
    umsg_close(f3);
}

static void MessageLength(void)
{
    UErrorCode status = U_ZERO_ERROR;
    const char patChars[] = {"123{0}456{0}"};
    const char expectedChars[] = {"123abc"};
    UChar pattern[sizeof(patChars)];
    UChar arg[] = {0x61,0x62,0x63,0};
    UChar result[128] = {0};
    UChar expected[sizeof(expectedChars)];

    u_uastrncpy(pattern, patChars, sizeof(pattern)/sizeof(pattern[0]));
    u_uastrncpy(expected, expectedChars, sizeof(expected)/sizeof(expected[0]));

    u_formatMessage("en_US", pattern, 6, result, sizeof(result)/sizeof(result[0]), &status, arg);
    if (U_FAILURE(status)) {
        log_err("u_formatMessage method failed. Error: %s \n",u_errorName(status));
    }
    if (u_strcmp(result, expected) != 0) {
        log_err("u_formatMessage didn't return expected result\n");
    }
}

static void TestErrorChaining(void) {
    UErrorCode status = U_USELESS_COLLATOR_ERROR;

    umsg_open(NULL, 0, NULL, NULL, &status);
    umsg_applyPattern(NULL, NULL, 0, NULL, &status);
    umsg_toPattern(NULL, NULL, 0, &status);
    umsg_clone(NULL, &status);
    umsg_format(NULL, NULL, 0, &status);
    umsg_parse(NULL, NULL, 0, NULL, &status);
    umsg_close(NULL);

    /* All of this code should have done nothing. */
    if (status != U_USELESS_COLLATOR_ERROR) {
        log_err("Status got changed to %s\n", u_errorName(status));
    }

    status = U_ZERO_ERROR;
    umsg_open(NULL, 0, NULL, NULL, &status);
    if (status != U_ILLEGAL_ARGUMENT_ERROR) {
        log_err("Status should be U_ILLEGAL_ARGUMENT_ERROR instead of %s\n", u_errorName(status));
    }
    status = U_ZERO_ERROR;
    umsg_applyPattern(NULL, NULL, 0, NULL, &status);
    if (status != U_ILLEGAL_ARGUMENT_ERROR) {
        log_err("Status should be U_ILLEGAL_ARGUMENT_ERROR instead of %s\n", u_errorName(status));
    }
    status = U_ZERO_ERROR;
    umsg_toPattern(NULL, NULL, 0, &status);
    if (status != U_ILLEGAL_ARGUMENT_ERROR) {
        log_err("Status should be U_ILLEGAL_ARGUMENT_ERROR instead of %s\n", u_errorName(status));
    }
    status = U_ZERO_ERROR;
    umsg_clone(NULL, &status);
    if (status != U_ILLEGAL_ARGUMENT_ERROR) {
        log_err("Status should be U_ILLEGAL_ARGUMENT_ERROR instead of %s\n", u_errorName(status));
    }
}

void addMsgForTest(TestNode** root);

void addMsgForTest(TestNode** root)
{
    addTest(root, &OpenMessageFormatTest, "tsformat/cmsgtst/OpenMessageFormatTest");
    addTest(root, &MessageFormatTest, "tsformat/cmsgtst/MessageFormatTest");
    addTest(root, &TestSampleMessageFormat, "tsformat/cmsgtst/TestSampleMessageFormat");
    addTest(root, &TestSampleFormatAndParse, "tsformat/cmsgtst/TestSampleFormatAndParse");
    addTest(root, &TestSampleFormatAndParseWithError, "tsformat/cmsgtst/TestSampleFormatAndParseWithError");
    addTest(root, &TestNewFormatAndParseAPI, "tsformat/cmsgtst/TestNewFormatAndParseAPI");
    addTest(root, &TestMsgFormatChoice, "tsformat/cmsgtst/TestMsgFormatChoice");
    addTest(root, &TestParseMessage, "tsformat/cmsgtst/TestParseMessage");
    addTest(root, &TestMessageFormatWithValist, "tsformat/cmsgtst/TestMessageFormatWithValist");
    addTest(root, &TestParseMessageWithValist, "tsformat/cmsgtst/TestParseMessageWithValist");
    addTest(root, &TestJ904, "tsformat/cmsgtst/TestJ904");
    addTest(root, &MessageLength, "tsformat/cmsgtst/MessageLength");
    addTest(root, &TestErrorChaining, "tsformat/cmsgtst/TestErrorChaining");
    addTest(root, &TestMsgFormatSelect, "tsformat/cmsgtst/TestMsgFormatSelect");
}

#endif /* #if !UCONFIG_NO_FORMATTING */
