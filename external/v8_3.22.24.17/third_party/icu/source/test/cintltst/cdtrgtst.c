/********************************************************************
 * COPYRIGHT: 
 * Copyright (c) 1997-2010, International Business Machines Corporation and
 * others. All Rights Reserved.
 ********************************************************************/
/********************************************************************************
*
* File CDTRGTST.C
*
*     Madhu Katragadda            Ported for C API
* Modification History:
*   Date        Name        Description
*   07/15/99    helena      Ported to HPUX 10/11 CC.
*********************************************************************************
*/
/* REGRESSION TEST FOR DATE FORMAT */

#include "unicode/utypes.h"

#if !UCONFIG_NO_FORMATTING

#include "unicode/uloc.h"
#include "unicode/udat.h"
#include "unicode/ucal.h"
#include "unicode/unum.h"
#include "unicode/ustring.h"
#include "cintltst.h"
#include "cdtrgtst.h"
#include "cmemory.h"

void addDateForRgrTest(TestNode** root);

void addDateForRgrTest(TestNode** root)
{
    addTest(root, &Test4029195, "tsformat/cdtrgtst/Test4029195");
    addTest(root, &Test4056591, "tsformat/cdtrgtst/Test4056591");
    addTest(root, &Test4059917, "tsformat/cdtrgtst/Test4059917");
    addTest(root, &Test4060212, "tsformat/cdtrgtst/Test4060212");
    addTest(root, &Test4061287, "tsformat/cdtrgtst/Test4061287");
    addTest(root, &Test4073003, "tsformat/cdtrgtst/Test4073003");
    addTest(root, &Test4162071, "tsformat/cdtrgtst/Test4162071");
    addTest(root, &Test714,     "tsformat/cdtrgtst/Test714");
    addTest(root, &Test_GEec,   "tsformat/cdtrgtst/Test_GEec"); /* tests for format chars GEec, jitterbugs 5726 6072 6585 */
}

/**
 * @bug 4029195
 */
void Test4029195() 
{
    int32_t resultlength, resultlengthneeded;
    UChar  *fmdt, *todayS, *rt;
    UChar *pat=NULL;
    UChar *temp;
    UDate today, d1;
    UDateFormat *df;
    int32_t parsepos;
    UErrorCode status = U_ZERO_ERROR;

    log_verbose("Testing date format and parse function in regression test\n");
    today = ucal_getNow();
    
    df = udat_open(UDAT_DEFAULT,UDAT_DEFAULT ,"en_US", NULL, 0, NULL, 0, &status);
    if(U_FAILURE(status))
    {
        log_data_err("FAIL: error in creating the dateformat using default date and time style : %s (Are you missing data?)\n", myErrorName(status));
        return;
    }
    resultlength=0;
    resultlengthneeded=udat_toPattern(df, TRUE, NULL, resultlength, &status);
    if(status==U_BUFFER_OVERFLOW_ERROR)
    {
        status=U_ZERO_ERROR;
        resultlength=resultlengthneeded + 1;
        pat=(UChar*)malloc(sizeof(UChar) * resultlength);
        udat_toPattern(df, TRUE, pat, resultlength, &status);
    }
    
    log_verbose("pattern: %s\n", austrdup(pat));

    
    fmdt = myFormatit(df, today);
    if(fmdt) {
      log_verbose("today: %s\n", austrdup(fmdt));
    } else {
      log_data_err("ERROR: couldn't format, exitting test");
      return;
    }
    
    temp=(UChar*)malloc(sizeof(UChar) * 10);
    u_uastrcpy(temp, "M yyyy dd");
    udat_applyPattern(df, TRUE, temp, u_strlen(temp));
    
    todayS =myFormatit(df, today);
    log_verbose("After teh pattern is applied\n today: %s\n", austrdup(todayS) );
    parsepos=0;
    d1=udat_parse(df, todayS, u_strlen(todayS), &parsepos, &status);
    if(U_FAILURE(status))
    {
        log_err("FAIL: Error in parsing using udat_parse(.....): %s\n", myErrorName(status));
    }
            
    rt =myFormatit(df, d1);
    log_verbose("today: %s\n", austrdup(rt) );
     
    log_verbose("round trip: %s\n", austrdup(rt) );
      
    if(u_strcmp(rt, todayS)!=0) {
            log_err("Fail: Want  %s  Got  %s\n", austrdup(todayS), austrdup(rt) );
    }
    else
        log_verbose("Pass: parse and format working fine\n");
    udat_close(df);
    free(temp);
    if(pat != NULL) {
        free(pat);
    }
}


/**
 * @bug 4056591
 * Verify the function of the [s|g]et2DigitYearStart() API.
 */
void Test4056591() 
{
    int i;
    UCalendar *cal;
    UDateFormat *def;    
    UDate start,exp,got;
    UChar s[10];
    UChar *gotdate, *expdate;
    UChar pat[10];
    UDate d[4];
    UErrorCode status = U_ZERO_ERROR;
    const char* strings[] = {
             "091225",
             "091224",
             "611226",
             "991227"
        };

    log_verbose("Testing s[get] 2 digit year start regressively\n");
    cal=ucal_open(NULL, 0, "en_US", UCAL_GREGORIAN, &status);
    if(U_FAILURE(status)){
        log_data_err("error in ucal_open caldef : %s - (Are you missing data?)\n", myErrorName(status));
        return;
    }
    ucal_setDateTime(cal, 1809, UCAL_DECEMBER, 25, 17, 40, 30, &status);
    d[0]=ucal_getMillis(cal, &status);
    if(U_FAILURE(status)){
            log_err("Error: failure in get millis: %s\n", myErrorName(status));
    }
    ucal_setDateTime(cal, 1909, UCAL_DECEMBER, 24, 17, 40, 30, &status);
    d[1]=ucal_getMillis(cal, &status);
    ucal_setDateTime(cal, 1861, UCAL_DECEMBER, 26, 17, 40, 30, &status);
    d[2]=ucal_getMillis(cal, &status);
    ucal_setDateTime(cal, 1999, UCAL_DECEMBER, 27, 17, 40, 30, &status);
    d[3]=ucal_getMillis(cal, &status);

    
    u_uastrcpy(pat, "yyMMdd");
    def = udat_open(UDAT_IGNORE,UDAT_IGNORE,NULL, NULL, 0, pat, u_strlen(pat), &status);
    if(U_FAILURE(status))
    {
        log_err_status(status, "FAIL: error in creating the dateformat using u_openPattern(): %s\n", myErrorName(status));
        return;
    }
    start = 1800;
    udat_set2DigitYearStart(def, start, &status);
    if(U_FAILURE(status))
        log_err("ERROR: in setTwoDigitStartDate: %s\n", myErrorName(status));
    if( (udat_get2DigitYearStart(def, &status) != start))
        log_err("ERROR: get2DigitYearStart broken\n");
        

    for(i = 0; i < 4; ++i) {
        u_uastrcpy(s, strings[i]);
        exp = d[i];
        got = udat_parse(def, s, u_strlen(s), 0, &status);
        gotdate=myFormatit(def, got);
        expdate=myFormatit(def, exp);

        if (gotdate == NULL || expdate == NULL) {
            log_err("myFormatit failed!\n");
        }
        else if(u_strcmp(gotdate, expdate) !=0){
            log_err("set2DigitYearStart broken for %s \n  got: %s, expected: %s\n", austrdup(s),
                austrdup(gotdate), austrdup(expdate) );
        }
    }
    
    udat_close(def);
    ucal_close(cal);
}


/**
 * SimpleDateFormat does not properly parse date strings without delimiters
 * @bug 4059917
 */
void Test4059917() 
{
    UDateFormat* def;
    UChar *myDate;
    UErrorCode status = U_ZERO_ERROR;
    UChar pattern[11];
    UChar tzID[4];

    log_verbose("Testing apply pattern and to pattern regressively\n");
    u_uastrcpy(tzID, "PST");
    u_uastrcpy(pattern, "yyyy/MM/dd");
    log_verbose("%s\n", austrdup(pattern) );
    def = udat_open(UDAT_IGNORE,UDAT_IGNORE,NULL,tzID,-1,pattern, u_strlen(pattern),&status);
    if(U_FAILURE(status))
    {
        log_data_err("FAIL: error in creating the dateformat using openPattern: %s - (Are you missing data?)\n", myErrorName(status));
        return;
    }
    myDate=(UChar*)malloc(sizeof(UChar) * 11);
    u_uastrcpy(myDate, "1970/01/12");
        
    aux917( def, myDate );
    udat_close(def);
    
    u_uastrcpy(pattern, "yyyyMMdd");
    def = udat_open(UDAT_IGNORE,UDAT_IGNORE,NULL,tzID,-1,pattern, u_strlen(pattern),&status);
    if(U_FAILURE(status))
    {
        log_err("FAIL: error in creating the dateformat using openPattern: %s\n", myErrorName(status));
        return;
    }
    u_uastrcpy(myDate, "19700112");
    aux917( def, myDate );
    udat_close(def);    
    free(myDate);
}

void aux917( UDateFormat *fmt, UChar* str) 
{    
    int32_t resultlength, resultlengthneeded;
    UErrorCode status = U_ZERO_ERROR;
    UChar* formatted=NULL;
    UChar *pat=NULL;
    UDate d1=1000000000.0;
   
    resultlength=0;
    resultlengthneeded=udat_toPattern(fmt, TRUE, NULL, resultlength, &status);
    if(status==U_BUFFER_OVERFLOW_ERROR)
    {
        status=U_ZERO_ERROR;
        resultlength=resultlengthneeded + 1;
        pat=(UChar*)malloc(sizeof(UChar) * (resultlength));
        udat_toPattern(fmt, TRUE, pat, resultlength, &status);
    }
    if(U_FAILURE(status)){
        log_err("failure in retrieving the pattern: %s\n", myErrorName(status));
    }
    log_verbose("pattern: %s\n", austrdup(pat) );
       
    status = U_ZERO_ERROR;
    formatted = myFormatit(fmt, d1);
    if( u_strcmp(formatted,str)!=0) {
        log_err("Fail: Want %s Got: %s\n", austrdup(str),  austrdup(formatted) );
    }
    free(pat);
}

/**
 * @bug 4060212
 */
void Test4060212() 
{
    int32_t pos;
    UCalendar *cal;
    UDateFormat *formatter, *fmt;
    UErrorCode status = U_ZERO_ERROR;
    UDate myDate;
    UChar *myString;
    UChar dateString[30], pattern[20], tzID[4];
    u_uastrcpy(dateString, "1995-040.05:01:29 -8");
    u_uastrcpy(pattern, "yyyy-DDD.hh:mm:ss z");

    log_verbose( "dateString= %s Using yyyy-DDD.hh:mm:ss\n", austrdup(dateString) );
    status = U_ZERO_ERROR;
    u_uastrcpy(tzID, "PST");

    formatter = udat_open(UDAT_IGNORE,UDAT_IGNORE,"en_US",tzID,-1,pattern, u_strlen(pattern), &status);
    pos=0;
    myDate = udat_parse(formatter, dateString, u_strlen(dateString), &pos, &status);
    
    
    fmt = udat_open(UDAT_FULL,UDAT_LONG ,NULL, tzID, -1, NULL, 0, &status);
    if(U_FAILURE(status))
    {
        log_data_err("FAIL: error in creating the dateformat using default date and time style: %s - (Are you missing data?)\n", 
                        myErrorName(status) );
        return;
    }
    myString = myFormatit(fmt, myDate);
    cal=ucal_open(tzID, u_strlen(tzID), "en_US", UCAL_GREGORIAN, &status);
    if(U_FAILURE(status)){
        log_err("FAIL: error in ucal_open caldef : %s\n", myErrorName(status));
    }
    ucal_setMillis(cal, myDate, &status);
    if ((ucal_get(cal, UCAL_DAY_OF_YEAR, &status) != 40)){
        log_err("Fail: Got  %d Expected 40\n", ucal_get(cal, UCAL_DAY_OF_YEAR, &status));
    }
    
    udat_close(formatter);
    ucal_close(cal);
    udat_close(fmt);
    
}

/**
 * @bug 4061287
 */
void Test4061287() 
{
    UBool ok;
    int32_t pos;
    UDateFormat *df;
    UErrorCode status = U_ZERO_ERROR;
    UDate myDate;
    UChar pattern[21], dateString[11];
    
    u_uastrcpy(dateString, "35/13/1971");
    u_uastrcpy(pattern, "dd/mm/yyyy");
    status = U_ZERO_ERROR;
    log_verbose("Testing parsing by changing the attribute lenient\n");
    df = udat_open(UDAT_IGNORE,UDAT_IGNORE,NULL,NULL,0,pattern, u_strlen(pattern),&status);
    if(U_FAILURE(status)){
        log_data_err("ERROR: failure in open pattern of test4061287: %s - (Are you missing data?)\n", myErrorName(status));
        return;
    }

    pos=0;
    
    udat_setLenient(df, FALSE);
    ok=udat_isLenient(df);
    if(ok==TRUE)
        log_err("setLenient nor working\n");
    ok = FALSE;
    myDate = udat_parse(df, dateString, u_strlen(dateString), &pos, &status);
    if(U_FAILURE(status))
        ok = TRUE;
    if(ok!=TRUE) 
        log_err("Fail: Lenient not working: does lenient parsing in spite of setting Leninent as FALSE ");

    udat_close(df);
    
}



/* The java.text.DateFormat.parse(String) method expects for the
  US locale a string formatted according to mm/dd/yy and parses it
  correctly.

  When given a string mm/dd/yyyy it only parses up to the first
  two y's, typically resulting in a date in the year 1919.
  
  Please extend the parsing method(s) to handle strings with
  four-digit year values (probably also applicable to various
  other locales.  */
/**
 * @bug 4073003
 */
void Test4073003() 
{
    int32_t pos,i;
    UDate d,dd;
    UChar *datestr;
    UChar temp[15];
    UErrorCode status = U_ZERO_ERROR;
    UDateFormat *fmt;
    UChar *result, *result2;
    const char* tests [] = { 
                "12/25/61", 
                "12/25/1961", 
                "4/3/1999", 
                "4/3/99" 
        };
    
    fmt= udat_open(UDAT_SHORT,UDAT_SHORT ,NULL, NULL, 0, NULL, 0, &status);
    if(U_FAILURE(status))
    {
        log_data_err("FAIL: error in creating the dateformat using short date and time style: %s (Are you missing data?)\n", 
            myErrorName(status));
        return;
    }
    u_uastrcpy(temp, "m/D/yy");
    udat_applyPattern(fmt, FALSE, temp, u_strlen(temp));

    for(i= 0; i < 4; i+=2) {
        status=U_ZERO_ERROR;
        datestr=(UChar*)malloc(sizeof(UChar) * (strlen(tests[i])+1));
        u_uastrcpy(datestr, tests[i]);
        
        pos=0;
        d = udat_parse(fmt, datestr, u_strlen(datestr), &pos, &status);
        if(U_FAILURE(status)){
            log_err("ERROR : in test 4073003: %s\n", myErrorName(status));
        }
        
        free(datestr);
        datestr=(UChar*)malloc(sizeof(UChar) * (strlen(tests[i+1])+1));
        u_uastrcpy(datestr, tests[i+1]);
    
        pos=0;
        status=U_ZERO_ERROR;
        dd = udat_parse(fmt, datestr, u_strlen(datestr), &pos, &status);
        if(U_FAILURE(status)){
            log_err("ERROR : in test 4073003: %s\n", myErrorName(status));
        }
        free(datestr);
        
        result =myFormatit(fmt, d);
        result2 =myFormatit(fmt, dd);
        if(!result || !result2) {
            log_data_err("Fail: could not format - exitting test\n");
            return;
        }
        if (u_strcmp(result, result2)!=0){
            log_err("Fail: %s != %s\n", austrdup(result), austrdup(result2) );
        }
        else{
            log_verbose("Ok: %s == %s\n", austrdup(result), austrdup(result2) );
        }
   
    }
    udat_close(fmt);
}

/**
 * @bug 4162071
 **/
void Test4162071() 
{
    int32_t pos;
    UDate x;
    UErrorCode status = U_ZERO_ERROR;
    UDateFormat *df;
    UChar datestr[30];
    UChar format[50];
    u_uastrcpy(datestr, "Thu, 30-Jul-1999 11:51:14 GMT");
    u_uastrcpy(format, "EEE', 'dd-MMM-yyyy HH:mm:ss z"); /*  RFC 822/1123 */
    status = U_ZERO_ERROR;
    /* Can't hardcode the result to assume the default locale is "en_US". */
    df = udat_open(UDAT_IGNORE,UDAT_IGNORE,"en_US",NULL,0,format, u_strlen(format),&status);
    if(U_FAILURE(status)){
        log_data_err("ERROR: couldn't create date format: %s\n", myErrorName(status));
        return;
    }
    pos=0;
    x = udat_parse(df, datestr, u_strlen(datestr), &pos, &status);
    if(U_FAILURE(status)){
        log_data_err("ERROR : parse format  %s fails : %s\n", austrdup(format), myErrorName(status));
    }
    else{
        log_verbose("Parse format \"%s \" ok.\n", austrdup(format) );
    }
    /*log_verbose("date= %s\n", austrdup(myFormatit(df, x)) );*/
    udat_close(df);
}

void Test714(void)
{
    UDate d=978103543000.0;
    UChar temp[20];
    UErrorCode status = U_ZERO_ERROR;
    UDateFormat *fmt;
    UChar *result;
    const char* expect =  "7:25:43 AM";
    
    ctest_setTimeZone(NULL, &status);

    fmt= udat_open(UDAT_MEDIUM,UDAT_NONE ,"en_US_CA", NULL, -1, NULL, 0, &status);
    if(U_FAILURE(status))
    {
        log_data_err("FAIL: error in creating the dateformat using medium time style and NO date style: %s (Are you missing data?)\n", 
            myErrorName(status));
        return;
    }
    result =myFormatit(fmt, d);
    if(!result) {
      log_data_err("Fail: could not format - exitting test\n");
      return;
    }
    u_uastrcpy(temp, expect);
    if (u_strcmp(result, temp)!=0){
      log_err("Fail: %s != %s\n", austrdup(result), expect);
    }
    else{
      log_verbose("Ok: %s == %s\n", austrdup(result), expect );
    }
        
    udat_close(fmt);

    ctest_resetTimeZone();
}

enum { DATE_TEXT_MAX_CHARS = 64 };
static const UChar zonePST[] = { 0x50,0x53,0x54,0 }; /* "PST" */
static const UDate july022008 = 1215000001979.0; /* 02 July 2008 5:00:01.979 AM PDT (near ICU 4.0 release date :-) */
static const double dayMillisec = 8.64e+07;

static const UChar dMyGGGPattern[]   = { 0x64,0x64,0x20,0x4D,0x4D,0x4D,0x20,0x79,0x79,0x79,0x79,0x20,0x47,0x47,0x47,0 };           /* "dd MMM yyyy GGG" */
static const UChar dMyGGGGGPattern[] = { 0x64,0x64,0x20,0x4D,0x4D,0x4D,0x20,0x79,0x79,0x79,0x79,0x20,0x47,0x47,0x47,0x47,0x47,0 }; /* "dd MMM yyyy GGGGG" */
static const UChar dMyGGGText[]      = { 0x30,0x32,0x20,0x4A,0x75,0x6C,0x20,0x32,0x30,0x30,0x38,0x20,0x41,0x44,0 };                /* "02 Jul 2008 AD" */
static const UChar dMyGGGGGText[]    = { 0x30,0x32,0x20,0x4A,0x75,0x6C,0x20,0x32,0x30,0x30,0x38,0x20,0x41,0 };                     /* "02 Jul 2008 A" */
static const UChar edMyPattern[]     = { 0x65,0x20,0x64,0x64,0x20,0x4D,0x4D,0x4D,0x20,0x79,0x79,0x79,0x79,0 };                     /* "e dd MMM yyyy" */
static const UChar eedMyPattern[]    = { 0x65,0x65,0x20,0x64,0x64,0x20,0x4D,0x4D,0x4D,0x20,0x79,0x79,0x79,0x79,0 };                /* "ee dd MMM yyyy" */
static const UChar cdMyPattern[]     = { 0x63,0x20,0x64,0x64,0x20,0x4D,0x4D,0x4D,0x20,0x79,0x79,0x79,0x79,0 };                     /* "c dd MMM yyyy" */
static const UChar ccdMyPattern[]    = { 0x63,0x63,0x20,0x64,0x64,0x20,0x4D,0x4D,0x4D,0x20,0x79,0x79,0x79,0x79,0 };                /* "cc dd MMM yyyy" */
static const UChar edMyText[]        = { 0x34,0x20,0x30,0x32,0x20,0x4A,0x75,0x6C,0x20,0x32,0x30,0x30,0x38,0 };                     /* "4 02 Jul 2008" */
static const UChar eedMyText[]       = { 0x30,0x34,0x20,0x30,0x32,0x20,0x4A,0x75,0x6C,0x20,0x32,0x30,0x30,0x38,0 };                /* "04 02 Jul 2008" */
static const UChar eeedMyPattern[]   = { 0x65,0x65,0x65,0x20,0x64,0x64,0x20,0x4D,0x4D,0x4D,0x20,0x79,0x79,0x79,0x79,0 };           /* "eee dd MMM yyyy" */
static const UChar EEEdMyPattern[]   = { 0x45,0x45,0x45,0x20,0x64,0x64,0x20,0x4D,0x4D,0x4D,0x20,0x79,0x79,0x79,0x79,0 };           /* "EEE dd MMM yyyy" */
static const UChar EEdMyPattern[]    = { 0x45,0x45,0x20,0x64,0x64,0x20,0x4D,0x4D,0x4D,0x20,0x79,0x79,0x79,0x79,0 };                /* "EE dd MMM yyyy" */
static const UChar eeedMyText[]      = { 0x57,0x65,0x64,0x20,0x30,0x32,0x20,0x4A,0x75,0x6C,0x20,0x32,0x30,0x30,0x38,0 };           /* "Wed 02 Jul 2008" */
static const UChar eeeedMyPattern[]  = { 0x65,0x65,0x65,0x65,0x20,0x64,0x64,0x20,0x4D,0x4D,0x4D,0x20,0x79,0x79,0x79,0x79,0 };      /* "eeee dd MMM yyyy" */
static const UChar eeeedMyText[]     = { 0x57,0x65,0x64,0x6E,0x65,0x73,0x64,0x61,0x79,0x20,0x30,0x32,0x20,0x4A,0x75,0x6C,0x20,0x32,0x30,0x30,0x38,0 }; /* "Wednesday 02 Jul 2008" */
static const UChar eeeeedMyPattern[] = { 0x65,0x65,0x65,0x65,0x65,0x20,0x64,0x64,0x20,0x4D,0x4D,0x4D,0x20,0x79,0x79,0x79,0x79,0 }; /* "eeeee dd MMM yyyy" */
static const UChar eeeeedMyText[]    = { 0x57,0x20,0x30,0x32,0x20,0x4A,0x75,0x6C,0x20,0x32,0x30,0x30,0x38,0 };                     /* "W 02 Jul 2008" */
static const UChar ewYPattern[]      = { 0x65,0x20,0x77,0x77,0x20,0x59,0x59,0x59,0x59,0 };                                         /* "e ww YYYY" */
static const UChar cwYPattern[]      = { 0x63,0x20,0x77,0x77,0x20,0x59,0x59,0x59,0x59,0 };                                         /* "c ww YYYY" */
static const UChar ewYText[]         = { 0x34,0x20,0x32,0x37,0x20,0x32,0x30,0x30,0x38,0 };                                         /* "4 27 2008" */
static const UChar HHmmssPattern[]   = { 0x48,0x48,0x3A,0x6D,0x6D,0x3A,0x73,0x73,0 };                                              /* "HH:mm:ss" */
static const UChar HHmmssText[]      = { 0x30,0x35,0x3A,0x30,0x30,0x3A,0x30,0x31,0 };                                              /* "05:00:01" */
static const UChar ssSPattern[]      = { 0x73,0x73,0x2E,0x53,0 };                                                                  /* "ss.S" */
static const UChar ssSText[]         = { 0x30,0x31,0x2E,0x39,0 };                                                                  /* "01.9" */
static const UChar ssSSPattern[]     = { 0x73,0x73,0x2E,0x53,0x53,0 };                                                             /* "ss.SS" */
static const UChar ssSSText[]        = { 0x30,0x31,0x2E,0x39,0x37,0 };                                                             /* "01.97" */

typedef struct {
    const UChar * pattern;
    const UChar * text;
    const char *  label;
} DatePatternAndText;
static const DatePatternAndText datePatternsAndText[] = {
    { dMyGGGPattern,   dMyGGGText,   "dd MMM yyyy GGG"   },
    { dMyGGGGGPattern, dMyGGGGGText, "dd MMM yyyy GGGGG" },
    { edMyPattern,     edMyText,     "e dd MMM yyyy"     },
    { eedMyPattern,    eedMyText,    "ee dd MMM yyyy"    },
    { cdMyPattern,     edMyText,     "c dd MMM yyyy"     },
    { ccdMyPattern,    edMyText,     "cc dd MMM yyyy"    },
    { eeedMyPattern,   eeedMyText,   "eee dd MMM yyyy"   },
    { EEEdMyPattern,   eeedMyText,   "EEE dd MMM yyyy"   },
    { EEdMyPattern,    eeedMyText,   "EE dd MMM yyyy"    },
    { eeeedMyPattern,  eeeedMyText,  "eeee dd MMM yyyy"  },
    { eeeeedMyPattern, eeeeedMyText, "eeeee dd MMM yyyy" },
    { ewYPattern,      ewYText,      "e ww YYYY"         },
    { cwYPattern,      ewYText,      "c ww YYYY"         },
    { HHmmssPattern,   HHmmssText,   "* HH:mm:ss"        }, /* '*' at start means don't check value from parse (won't be july022008) */
    { ssSPattern,      ssSText,      "* ss.S"            },
    { ssSSPattern,     ssSSText,     "* ss.SS"           },
    { NULL,            NULL,         NULL                }
};
void Test_GEec(void)
{
    UErrorCode    status = U_ZERO_ERROR;
    UDateFormat * dtfmt = udat_open(UDAT_LONG, UDAT_LONG, "en", zonePST, -1, NULL, 0, &status);
    if ( U_SUCCESS(status) ) {
        const DatePatternAndText *patTextPtr;
        for (patTextPtr = datePatternsAndText; patTextPtr->pattern != NULL; ++patTextPtr) {
            UChar   dmyGnText[DATE_TEXT_MAX_CHARS];
            char    byteText[3*DATE_TEXT_MAX_CHARS];
            int32_t dmyGnTextLen;
            UDate   dateResult;

            udat_applyPattern(dtfmt, FALSE, patTextPtr->pattern, -1);
            dmyGnTextLen = udat_format(dtfmt, july022008, dmyGnText, DATE_TEXT_MAX_CHARS, NULL, &status);
            if ( U_FAILURE(status) ) {
                log_err("FAIL: udat_format with %s: %s\n", patTextPtr->label, myErrorName(status) );
                status = U_ZERO_ERROR;
            } else if ( u_strcmp(dmyGnText, patTextPtr->text) != 0 ) {
                log_err("FAIL: udat_format with %s: wrong UChar[] result %s\n", patTextPtr->label, u_austrcpy(byteText,dmyGnText) );
            }

            dateResult = udat_parse(dtfmt, patTextPtr->text, -1, NULL, &status); /* no time, dateResult != july022008 by some hours */
            if ( U_FAILURE(status) ) {
                log_err("FAIL: udat_parse with %s: %s\n", patTextPtr->label, myErrorName(status) );
                status = U_ZERO_ERROR;
            } else if ( patTextPtr->label[0] != '*' && july022008 - dateResult > dayMillisec ) {
                log_err("FAIL: udat_parse with %s: wrong UDate result\n", patTextPtr->label );
            }
        }
        udat_close(dtfmt);
    } else {
        log_data_err("FAIL: udat_open fails: %s (Are you missing data?)\n", myErrorName(status));
    }
}

/*INTERNAL FUNCTION USED */

UChar* myFormatit(UDateFormat* datdef, UDate d1)
{
    UChar *result1=NULL;
    int32_t resultlength, resultlengthneeded;
    UErrorCode status = U_ZERO_ERROR;
    
    resultlength=0;
    resultlengthneeded=udat_format(datdef, d1, NULL, resultlength, NULL, &status);
    if(status==U_BUFFER_OVERFLOW_ERROR)
    {
        status=U_ZERO_ERROR;
        resultlength=resultlengthneeded+1;
        /*result1=(UChar*)malloc(sizeof(UChar) * resultlength);*/ /*this leaks*/
        result1=(UChar*)ctst_malloc(sizeof(UChar) * resultlength); /*this won't*/
        udat_format(datdef, d1, result1, resultlength, NULL, &status);
    }
    if(U_FAILURE(status))
    {
        log_err("FAIL: Error in formatting using udat_format(.....): %s\n", myErrorName(status));
        return 0;
    }
    
    
    return result1;

}

#endif /* #if !UCONFIG_NO_FORMATTING */

/*eof*/
