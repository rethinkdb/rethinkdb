/********************************************************************
 * COPYRIGHT: 
 * Copyright (c) 1997-2010, International Business Machines Corporation and
 * others. All Rights Reserved.
 ********************************************************************/
/********************************************************************************
*
* File CDTDPTST.C
*
* Modification History:
*        Name                     Description            
*     Madhu Katragadda               Creation
*********************************************************************************
*/
/* INDEPTH TEST FOR DATE FORMAT */

#include "unicode/utypes.h"

#if !UCONFIG_NO_FORMATTING

#include "unicode/uloc.h"
#include "unicode/udat.h"
#include "unicode/ucal.h"
#include "unicode/unum.h"
#include "unicode/ustring.h"
#include "cintltst.h"
#include "cdtdptst.h"
#include "cformtst.h"

#include "cmemory.h"

void addDtFrDepTest(TestNode** root);

void addDtFrDepTest(TestNode** root)
{
    addTest(root, &TestTwoDigitYearDSTParse, "tsformat/cdtdptst/TestTwoDigitYearDSTParse");
    addTest(root, &TestPartialParse994, "tsformat/cdtdptst/TestPartialParse994");
    addTest(root, &TestRunTogetherPattern985, "tsformat/cdtdptst/TestRunTogetherPattern985");
    addTest(root, &TestCzechMonths459, "tsformat/cdtdptst/TestCzechMonths459");
    addTest(root, &TestQuotePattern161, "tsformat/cdtdptst/TestQuotePattern161");
    
}

/**
 * Test the parsing of 2-digit years.
 */
void TestTwoDigitYearDSTParse()
{
    UDateFormat *fullFmt, *fmt;
    UErrorCode status = U_ZERO_ERROR;
    UChar *pattern;
    UDate d;
    UChar *s;
    int32_t pos;

    ctest_setTimeZone(NULL, &status);

    pattern=(UChar*)malloc(sizeof(UChar) * (strlen("EEE MMM dd HH:mm:ss.SSS zzz yyyy G")+1 ));
    u_uastrcpy(pattern, "EEE MMM dd HH:mm:ss.SSS zzz yyyy G");
    fullFmt= udat_open(UDAT_IGNORE, UDAT_IGNORE,"en_US",NULL,0,pattern, u_strlen(pattern),&status);
    if(U_FAILURE(status))    {
        log_data_err("FAIL: Error in creating a date format using udat_openPattern %s - (Are you missing data?)\n", 
            myErrorName(status) );
    }
    else {
        log_verbose("PASS: creating dateformat using udat_openPattern() succesful\n");
    
        u_uastrcpy(pattern, "dd-MMM-yy h:mm:ss 'o''clock' a z");
        fmt= udat_open(UDAT_IGNORE,UDAT_IGNORE,"en_US", NULL, 0,pattern, u_strlen(pattern), &status);
        
        
        s=(UChar*)malloc(sizeof(UChar) * (strlen("03-Apr-04 2:20:47 o'clock AM PST")+1) );
        u_uastrcpy(s, "03-Apr-04 2:20:47 o'clock AM PST");
        pos=0;
        d = udat_parse(fmt, s, u_strlen(s), &pos, &status);
        if (U_FAILURE(status)) {
            log_err("FAIL: Could not parse \"%s\"\n", austrdup(s));
        } else {
            UCalendar *cal = ucal_open(NULL, 0, uloc_getDefault(), UCAL_TRADITIONAL, &status);
            if (U_FAILURE(status)) {
                log_err_status(status, "FAIL: Could not open calendar: %s\n", u_errorName(status));
            } else {
                int32_t h;
                ucal_setMillis(cal, d, &status);
                h = ucal_get(cal, UCAL_HOUR_OF_DAY, &status);
                if (U_FAILURE(status)) {
                    log_err("FAIL: Some calendar operations failed");
                } else if (h != 2) {
                    log_err("FAIL: Parse of \"%s\" returned HOUR_OF_DAY %d\n",
                            austrdup(s), h);
                }
                ucal_close(cal);
            }
        }
        
        udat_close(fullFmt);
        udat_close(fmt);
        free(s);
    }
    free(pattern);

    ctest_resetTimeZone();
}


/**
 * Verify that strings which contain incomplete specifications are parsed
 * correctly.  In some instances, this means not being parsed at all, and
 * returning an appropriate error.
 */
void TestPartialParse994()
{
    int32_t pos;
    UDateFormat *f;
    UErrorCode status = U_ZERO_ERROR;
    UChar *s;
    UChar *fmtChars;
    UDate d, null;
    null=0;

    /* this is supposed to open default date format, but later on it treats it like it is "en_US" 
       - very bad if you try to run the tests on machine where default locale is NOT "en_US" */
    /* f = udat_open(UDAT_DEFAULT, UDAT_SHORT, NULL, NULL, 0, &status); */
    f = udat_open(UDAT_DEFAULT, UDAT_SHORT, "en_US", NULL, 0,  NULL, 0,&status);
    if(U_FAILURE(status)){
        log_data_err("FAIL: ErrorCode received during test: %s (Are you missing data?)\n", myErrorName(status));
        return;
    }
    s=(UChar*)malloc(sizeof(UChar) * (strlen("01/01/1997 10:11:42 AM")+1) );
    u_uastrcpy(s, "01/01/1997 10:11:42 AM");
    pos=0;
    d = udat_parse(f, s, u_strlen(s), &pos, &status);
    if(U_FAILURE(status)) {
      log_data_err("FAIL: could not parse - exitting");
      return;
    }
    fmtChars = myDateFormat(f, d);
    if(fmtChars) {
      log_verbose("%s\n", fmtChars);
    } else {
      log_data_err("FAIL: could not format \n");
      return;
    }
    tryPat994(f, "yy/MM/dd HH:mm:ss", "97/01/01 10:11:42", d);
    tryPat994(f, "yy/MM/dd HH:mm:ss", "97/01/01 10:", null);
    tryPat994(f, "yy/MM/dd HH:mm:ss", "97/01/01 10", null);
    tryPat994(f, "yy/MM/dd HH:mm:ss", "97/01/01 ", null);
    tryPat994(f, "yy/MM/dd HH:mm:ss", "97/01/01", null);
    udat_close(f);
    free(s);
}
 

 
void tryPat994(UDateFormat* format, const char* pattern, const char* s, UDate expected)
{
    UChar *f;
    UChar *str, *pat;
    UDate date;
    UDate null=0;
    int32_t pos;
    UErrorCode status = U_ZERO_ERROR;
    str=(UChar*)malloc(sizeof(UChar) * (strlen(s) + 1) );
    u_uastrcpy(str, s);
    pat=(UChar*)malloc(sizeof(UChar) * (strlen(pattern) + 1) );
    u_uastrcpy(pat, pattern);
    log_verbose("Pattern : %s ;  String : %s\n", austrdup(pat), austrdup(str));
    udat_applyPattern(format, FALSE, pat, u_strlen(pat));
    pos=0;
    date = udat_parse(format, str, u_strlen(str), &pos, &status);
    if(U_FAILURE(status) || date == null) {
        log_verbose("ParseException: : %s\n", myErrorName(status) );
         if (expected != null) 
             log_err("FAIL: Expected: %s\n", austrdup(myDateFormat(format, expected)) );
        }
    else {
        f=myDateFormat(format, date);
        log_verbose(" parse( %s ) -> %s\n", austrdup(str), austrdup(f));
        if (expected == null || date != expected) 
            log_err("FAIL: Expected null for \"%s\"\n", s);
        if (u_strcmp(f, str) !=0)
            log_err("FAIL: Expected : %s\n", austrdup(str) );
    }
    
    free(str);
    free(pat);
}
 

/**
 * Verify the behavior of patterns in which digits for different fields run together
 * without intervening separators.
 */
void TestRunTogetherPattern985()
{
    int32_t pos;
    UChar *pattern=NULL, *now=NULL, *then=NULL;
    UDateFormat *format;
    UDate date1, date2;
    UErrorCode status = U_ZERO_ERROR;
    pattern=(UChar*)malloc(sizeof(UChar) * (strlen("yyyyMMddHHmmssSSS")+1) );
    u_uastrcpy(pattern, "yyyyMMddHHmmssSSS");
    format = udat_open(UDAT_IGNORE, UDAT_IGNORE, NULL, NULL, 0,pattern, u_strlen(pattern), &status);
    if(U_FAILURE(status)){
        log_data_err("FAIL: Error in date format construction with pattern: %s - (Are you missing data?)\n", myErrorName(status));
        return;
    }
    date1 = ucal_getNow();
    now=myDateFormat(format, date1);
    log_verbose("%s\n", austrdup(now) ); 
    pos = 0;
    date2 = udat_parse(format, now, u_strlen(now), &pos, &status);
    if (date2 == 0) log_verbose("Parse stopped at : %d\n", pos);
    else then=myDateFormat(format, date2);
    log_verbose("%s\n", austrdup(then) );
    if (!(date2 == date1)) log_err("FAIL\n");
    
    udat_close(format);
    free(pattern);
       
}

/**
 * Verify the handling of Czech June and July, which have the unique attribute that
 * one is a proper prefix substring of the other.
 */
void TestCzechMonths459()
{
    int32_t lneed, pos;
    UChar *pattern=NULL, *tzID=NULL;
    UChar *juneStr, *julyStr;
    UDateFormat *fmt;
    UCalendar *cal;
    UDate june, july, d;
    UErrorCode status = U_ZERO_ERROR;
    UChar *date;
    
    ctest_setTimeZone(NULL, &status);
    fmt = udat_open(UDAT_FULL, UDAT_FULL, "cs", NULL, 0, NULL, 0, &status);
    if(U_FAILURE(status)){
        log_data_err("Error in constructing the date format -> %s (Are you missing data?)\n", u_errorName(status));
        ctest_resetTimeZone();
        return;
    }
    lneed=0;
    lneed=udat_toPattern(fmt, TRUE, NULL, lneed, &status);
    if(status==U_BUFFER_OVERFLOW_ERROR){
        status=U_ZERO_ERROR;
        pattern=(UChar*)malloc(sizeof(UChar) * (lneed+1) );
        udat_toPattern(fmt, TRUE, pattern, lneed+1, &status);
    }
    if(U_FAILURE(status)){ log_err("Error in extracting the pattern\n"); }
    tzID=(UChar*)malloc(sizeof(UChar) * 4);
    u_uastrcpy(tzID, "GMT");
    cal=ucal_open(tzID, u_strlen(tzID), "cs", UCAL_GREGORIAN, &status);
    if(U_FAILURE(status)){ log_err("error in ucal_open caldef : %s\n", myErrorName(status));    }
    
    ucal_setDate(cal, 1997, UCAL_JUNE, 15, &status);
    june=ucal_getMillis(cal, &status);
    ucal_setDate(cal, 1997, UCAL_JULY, 15, &status);
    july=ucal_getMillis(cal, &status);

    juneStr = myDateFormat(fmt, june);
    julyStr = myDateFormat(fmt, july);
    pos=0;
    if(juneStr == NULL) {
      log_data_err("Can't load juneStr. Quitting.\n");
      return;
    }
    d = udat_parse(fmt, juneStr, u_strlen(juneStr), &pos, &status);
    date = myDateFormat(fmt, d);

    if(U_SUCCESS(status)){
        UChar* out1 = myDateFormat(fmt, june);
        UChar* out2 = myDateFormat(fmt, d);
        if(u_strcmp(out1, out2) !=0)
            log_err("Error in handling the czech month june\n");
        else
            log_verbose("Pass: Date = %s (czech month June)\n", aescstrdup(date, -1));
    }else{
        log_err("udat_parse failed. Error. %s\n",u_errorName(status));
    }
    pos=0;
    d = udat_parse(fmt, julyStr, u_strlen(julyStr), &pos, &status);
    date = myDateFormat(fmt, d);
    if(u_strcmp(myDateFormat(fmt, july), myDateFormat(fmt, d) ) !=0)
        log_err("Error in handling the czech month july\n");
    else
        log_verbose("Pass: Date = %s (czech month July)\n", aescstrdup(date, -1));
    
    ctest_resetTimeZone();
    udat_close(fmt);
    ucal_close(cal);
    free(pattern);
    free(tzID);
}
 
/**
 * Test the handling of single quotes in patterns.
 */
void TestQuotePattern161()
{
    UDateFormat *format;
    UCalendar *cal;
    UDate currentTime_1;
    UChar *pattern, *tzID, *exp;
    UChar *dateString;
    UErrorCode status = U_ZERO_ERROR;
    const char* expStr = "04/13/1999 at 10:42:28 AM ";

    ctest_setTimeZone(NULL, &status);

    pattern=(UChar*)malloc(sizeof(UChar) * (strlen("MM/dd/yyyy 'at' hh:mm:ss a zzz")+1) );
    u_uastrcpy(pattern, "MM/dd/yyyy 'at' hh:mm:ss a zzz");
    
    /* this is supposed to open default date format, but later on it treats it like it is "en_US" 
       - very bad if you try to run the tests on machine where default locale is NOT "en_US" */
    /* format= udat_openPattern(pattern, u_strlen(pattern), NULL, &status); */
    format= udat_open(UDAT_IGNORE, UDAT_IGNORE,"en_US", NULL, 0,pattern, u_strlen(pattern), &status);
    if(U_FAILURE(status)){
        log_data_err("error in udat_open: %s - (Are you missing data?)\n", myErrorName(status));
        return;
    }
    tzID=(UChar*)malloc(sizeof(UChar) * 4);
    u_uastrcpy(tzID, "PST");
    /* this is supposed to open default date format, but later on it treats it like it is "en_US" 
       - very bad if you try to run the tests on machine where default locale is NOT "en_US" */
    /* cal=ucal_open(tzID, u_strlen(tzID), NULL, UCAL_TRADITIONAL, &status); */
    cal=ucal_open(tzID, u_strlen(tzID), "en_US", UCAL_TRADITIONAL, &status);
    if(U_FAILURE(status)){ log_err("error in ucal_open cal : %s\n", myErrorName(status));    }
    
    ucal_setDateTime(cal, 1999, UCAL_APRIL, 13, 10, 42, 28, &status);
    currentTime_1 = ucal_getMillis(cal, &status);
    
    dateString = myDateFormat(format, currentTime_1);
    exp=(UChar*)malloc(sizeof(UChar) * (strlen(expStr) + 1) );
    u_uastrcpy(exp, expStr);
    
    log_verbose("%s\n", austrdup(dateString) );
    if(u_strncmp(dateString, exp, (int32_t)strlen(expStr)) !=0)
        log_err("Error in formatting a pattern with single quotes\n");

    udat_close(format);
    ucal_close(cal);
    free(exp);
    free(tzID);
    free(pattern);
    
    ctest_resetTimeZone();
}

#endif /* #if !UCONFIG_NO_FORMATTING */
