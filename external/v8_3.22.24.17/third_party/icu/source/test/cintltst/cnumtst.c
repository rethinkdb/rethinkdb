/********************************************************************
 * COPYRIGHT:
 * Copyright (c) 1997-2010, International Business Machines Corporation and
 * others. All Rights Reserved.
 ********************************************************************/
/********************************************************************************
*
* File CNUMTST.C
*
*     Madhu Katragadda              Creation
*
* Modification History:
*
*   Date        Name        Description
*   06/24/99    helena      Integrated Alan's NF enhancements and Java2 bug fixes
*   07/15/99    helena      Ported to HPUX 10/11 CC.
*********************************************************************************
*/

/* C API TEST FOR NUMBER FORMAT */

#include "unicode/utypes.h"

#if !UCONFIG_NO_FORMATTING

#include "unicode/uloc.h"
#include "unicode/umisc.h"
#include "unicode/unum.h"
#include "unicode/ustring.h"

#include "cintltst.h"
#include "cnumtst.h"
#include "cmemory.h"
#include "putilimp.h"

#define LENGTH(arr) (sizeof(arr)/sizeof(arr[0]))

void addNumForTest(TestNode** root);
static void TestTextAttributeCrash(void);
static void TestNBSPInPattern(void);
static void TestInt64Parse(void);

#define TESTCASE(x) addTest(root, &x, "tsformat/cnumtst/" #x)

void addNumForTest(TestNode** root)
{
    TESTCASE(TestNumberFormat);
    TESTCASE(TestSpelloutNumberParse);
    TESTCASE(TestSignificantDigits);
    TESTCASE(TestSigDigRounding);
    TESTCASE(TestNumberFormatPadding);
    TESTCASE(TestInt64Format);
    TESTCASE(TestNonExistentCurrency);
    TESTCASE(TestCurrencyRegression);
    TESTCASE(TestTextAttributeCrash);
    TESTCASE(TestRBNFFormat);
    TESTCASE(TestNBSPInPattern);
    TESTCASE(TestInt64Parse);
}

/** copy src to dst with unicode-escapes for values < 0x20 and > 0x7e, null terminate if possible */
static int32_t ustrToAstr(const UChar* src, int32_t srcLength, char* dst, int32_t dstLength) {
    static const char hex[] = { '0', '1', '2', '3', '4', '5', '6', '7', '8', '9', 'a', 'b', 'c', 'd', 'e', 'f' };

    char *p = dst;
    const char *e = p + dstLength;
    if (srcLength < 0) {
        const UChar* s = src;
        while (*s) {
            ++s;
        }
        srcLength = (int32_t)(s - src);
    }
    while (p < e && --srcLength >= 0) {
        UChar c = *src++;
        if (c == 0xd || c == 0xa || c == 0x9 || (c>= 0x20 && c <= 0x7e)) {
            *p++ = (char) c & 0x7f;
        } else if (e - p >= 6) {
            *p++ = '\\';
            *p++ = 'u';
            *p++ = hex[(c >> 12) & 0xf];
            *p++ = hex[(c >> 8) & 0xf];
            *p++ = hex[(c >> 4) & 0xf];
            *p++ = hex[c & 0xf];
        } else {
            break;
        }
    }
    if (p < e) {
        *p = 0;
    }
    return (int32_t)(p - dst);
}

/* test Parse int 64 */

static void TestInt64Parse()
{

    UErrorCode st = U_ZERO_ERROR;
    UErrorCode* status = &st;
    
    const char* st1 = "009223372036854775808";
    const int size = 21;
    UChar text[21];
    
	
    UNumberFormat* nf;

    int64_t a;

    u_charsToUChars(st1, text, size);
    nf = unum_open(UNUM_DEFAULT, NULL, -1, NULL, NULL, status);

    if(U_FAILURE(*status))
    {
        log_data_err("Error in unum_open() %s \n", myErrorName(*status));
        return;
    }

    log_verbose("About to test unum_parseInt64() with out of range number\n");

    a = unum_parseInt64(nf, text, size, 0, status);
	

    if(!U_FAILURE(*status))
    {
        log_err("Error in unum_parseInt64(): %s \n", myErrorName(*status));
    }
    else
    {
        log_verbose("unum_parseInt64() successful\n");
    }

    unum_close(nf);
    return;
}

/* test Number Format API */
static void TestNumberFormat()
{
    UChar *result=NULL;
    UChar temp1[512];
    UChar temp2[512];

    UChar temp[5];

    UChar prefix[5];
    UChar suffix[5];
    UChar symbol[20];
    int32_t resultlength;
    int32_t resultlengthneeded;
    int32_t parsepos;
    double d1 = -1.0;
    int32_t l1;
    double d = -10456.37;
    double a = 1234.56, a1 = 1235.0;
    int32_t l = 100000000;
    UFieldPosition pos1;
    UFieldPosition pos2;
    int32_t numlocales;
    int32_t i;

    UNumberFormatAttribute attr;
    UNumberFormatSymbol symType = UNUM_DECIMAL_SEPARATOR_SYMBOL;
    int32_t newvalue;
    UErrorCode status=U_ZERO_ERROR;
    UNumberFormatStyle style= UNUM_DEFAULT;
    UNumberFormat *pattern;
    UNumberFormat *def, *fr, *cur_def, *cur_fr, *per_def, *per_fr,
                  *cur_frpattern, *myclone, *spellout_def;

    /* Testing unum_open() with various Numberformat styles and locales*/
    status = U_ZERO_ERROR;
    log_verbose("Testing  unum_open() with default style and locale\n");
    def=unum_open(style, NULL,0,NULL, NULL,&status);

    /* Might as well pack it in now if we can't even get a default NumberFormat... */
    if(U_FAILURE(status))
    {
        log_data_err("Error in creating default NumberFormat using unum_open(): %s (Are you missing data?)\n", myErrorName(status));
        return;
    }

    log_verbose("\nTesting unum_open() with french locale and default style(decimal)\n");
    fr=unum_open(style,NULL,0, "fr_FR",NULL, &status);
    if(U_FAILURE(status))
        log_err("Error: could not create NumberFormat (french): %s\n", myErrorName(status));

    log_verbose("\nTesting unum_open(currency,NULL,status)\n");
    style=UNUM_CURRENCY;
    /* Can't hardcode the result to assume the default locale is "en_US". */
    cur_def=unum_open(style, NULL,0,"en_US", NULL, &status);
    if(U_FAILURE(status))
        log_err("Error: could not create NumberFormat using \n unum_open(currency, NULL, &status) %s\n",
                        myErrorName(status) );

    log_verbose("\nTesting unum_open(currency, frenchlocale, status)\n");
    cur_fr=unum_open(style,NULL,0, "fr_FR", NULL, &status);
    if(U_FAILURE(status))
        log_err("Error: could not create NumberFormat using unum_open(currency, french, &status): %s\n", 
                myErrorName(status));

    log_verbose("\nTesting unum_open(percent, NULL, status)\n");
    style=UNUM_PERCENT;
    per_def=unum_open(style,NULL,0, NULL,NULL, &status);
    if(U_FAILURE(status))
        log_err("Error: could not create NumberFormat using unum_open(percent, NULL, &status): %s\n", myErrorName(status));

    log_verbose("\nTesting unum_open(percent,frenchlocale, status)\n");
    per_fr=unum_open(style, NULL,0,"fr_FR", NULL,&status);
    if(U_FAILURE(status))
        log_err("Error: could not create NumberFormat using unum_open(percent, french, &status): %s\n", myErrorName(status));

    log_verbose("\nTesting unum_open(spellout, NULL, status)");
    style=UNUM_SPELLOUT;
    spellout_def=unum_open(style, NULL, 0, "en_US", NULL, &status);
    if(U_FAILURE(status))
        log_err("Error: could not create NumberFormat using unum_open(spellout, NULL, &status): %s\n", myErrorName(status));

    /* Testing unum_clone(..) */
    log_verbose("\nTesting unum_clone(fmt, status)");
    status = U_ZERO_ERROR;
    myclone = unum_clone(def,&status);
    if(U_FAILURE(status))
        log_err("Error: could not clone unum_clone(def, &status): %s\n", myErrorName(status));
    else
    {
        log_verbose("unum_clone() successful\n");
    }

    /*Testing unum_getAvailable() and unum_countAvailable()*/
    log_verbose("\nTesting getAvailableLocales and countAvailable()\n");
    numlocales=unum_countAvailable();
    if(numlocales < 0)
        log_err("error in countAvailable");
    else{
        log_verbose("unum_countAvialable() successful\n");
        log_verbose("The no: of locales where number formattting is applicable is %d\n", numlocales);
    }
    for(i=0;i<numlocales;i++)
    {
        log_verbose("%s\n", unum_getAvailable(i));
        if (unum_getAvailable(i) == 0)
            log_err("No locale for which number formatting patterns are applicable\n");
        else
            log_verbose("A locale %s for which number formatting patterns are applicable\n",unum_getAvailable(i));
    }


    /*Testing unum_format() and unum_formatdouble()*/
    u_uastrcpy(temp1, "$100,000,000.00");

    log_verbose("\nTesting unum_format() \n");
    resultlength=0;
    pos1.field = 0; /* Integer Section */
    resultlengthneeded=unum_format(cur_def, l, NULL, resultlength, &pos1, &status);
    if(status==U_BUFFER_OVERFLOW_ERROR)
    {
        status=U_ZERO_ERROR;
        resultlength=resultlengthneeded+1;
        result=(UChar*)malloc(sizeof(UChar) * resultlength);
/*        for (i = 0; i < 100000; i++) */
        {
            unum_format(cur_def, l, result, resultlength, &pos1, &status);
        }
    }

    if(U_FAILURE(status))
    {
        log_err("Error in formatting using unum_format(.....): %s\n", myErrorName(status) );
    }
    if(u_strcmp(result, temp1)==0)
        log_verbose("Pass: Number formatting using unum_format() successful\n");
    else
        log_err("Fail: Error in number Formatting using unum_format()\n");
    if(pos1.beginIndex == 1 && pos1.endIndex == 12)
        log_verbose("Pass: Complete number formatting using unum_format() successful\n");
    else
        log_err("Fail: Error in complete number Formatting using unum_format()\nGot: b=%d end=%d\nExpected: b=1 end=12\n",
                pos1.beginIndex, pos1.endIndex);

free(result);
    result = 0;

    log_verbose("\nTesting unum_formatDouble()\n");
    u_uastrcpy(temp1, "($10,456.37)");
    resultlength=0;
    pos2.field = 1; /* Fractional Section */
    resultlengthneeded=unum_formatDouble(cur_def, d, NULL, resultlength, &pos2, &status);
    if(status==U_BUFFER_OVERFLOW_ERROR)
    {
        status=U_ZERO_ERROR;
        resultlength=resultlengthneeded+1;
        result=(UChar*)malloc(sizeof(UChar) * resultlength);
/*        for (i = 0; i < 100000; i++) */
        {
            unum_formatDouble(cur_def, d, result, resultlength, &pos2, &status);
        }
    }
    if(U_FAILURE(status))
    {
        log_err("Error in formatting using unum_formatDouble(.....): %s\n", myErrorName(status));
    }
    if(result && u_strcmp(result, temp1)==0)
        log_verbose("Pass: Number Formatting using unum_formatDouble() Successful\n");
    else
        log_err("FAIL: Error in number formatting using unum_formatDouble()\n");
    if(pos2.beginIndex == 9 && pos2.endIndex == 11)
        log_verbose("Pass: Complete number formatting using unum_format() successful\n");
    else
        log_err("Fail: Error in complete number Formatting using unum_formatDouble()\nGot: b=%d end=%d\nExpected: b=9 end=11",
                pos1.beginIndex, pos1.endIndex);


    /* Testing unum_parse() and unum_parseDouble() */
    log_verbose("\nTesting unum_parseDouble()\n");
/*    for (i = 0; i < 100000; i++)*/
    if (result != NULL)
    {
        parsepos=0;
        d1=unum_parseDouble(cur_def, result, u_strlen(result), &parsepos, &status);
    }
    else {
        log_err("result is NULL\n");
    }
    if(U_FAILURE(status))
    {
        log_err("parse failed. The error is  : %s\n", myErrorName(status));
    }

    if(d1!=d)
        log_err("Fail: Error in parsing\n");
    else
        log_verbose("Pass: parsing successful\n");
    if (result)
        free(result);
    result = 0;


    /* Testing unum_formatDoubleCurrency / unum_parseDoubleCurrency */
    log_verbose("\nTesting unum_formatDoubleCurrency\n");
    u_uastrcpy(temp1, "Y1,235");
    temp1[0] = 0xA5; /* Yen sign */
    u_uastrcpy(temp, "JPY");
    resultlength=0;
    pos2.field = 0; /* integer part */
    resultlengthneeded=unum_formatDoubleCurrency(cur_def, a, temp, NULL, resultlength, &pos2, &status);
    if (status==U_BUFFER_OVERFLOW_ERROR) {
        status=U_ZERO_ERROR;
        resultlength=resultlengthneeded+1;
        result=(UChar*)malloc(sizeof(UChar) * resultlength);
        unum_formatDoubleCurrency(cur_def, a, temp, result, resultlength, &pos2, &status);
    }
    if (U_FAILURE(status)) {
        log_err("Error in formatting using unum_formatDouble(.....): %s\n", myErrorName(status));
    }
    if (result && u_strcmp(result, temp1)==0) {
        log_verbose("Pass: Number Formatting using unum_formatDouble() Successful\n");
    } else {
        log_err("FAIL: Error in number formatting using unum_formatDouble()\n");
    }
    if (pos2.beginIndex == 1 && pos2.endIndex == 6) {
        log_verbose("Pass: Complete number formatting using unum_format() successful\n");
    } else {
        log_err("Fail: Error in complete number Formatting using unum_formatDouble()\nGot: b=%d end=%d\nExpected: b=1 end=6\n",
                pos1.beginIndex, pos1.endIndex);
    }

    log_verbose("\nTesting unum_parseDoubleCurrency\n");
    parsepos=0;
    if (result == NULL) {
        log_err("result is NULL\n");
    }
    else {
        d1=unum_parseDoubleCurrency(cur_def, result, u_strlen(result), &parsepos, temp2, &status);
        if (U_FAILURE(status)) {
            log_err("parse failed. The error is  : %s\n", myErrorName(status));
        }
        /* Note: a==1234.56, but on parse expect a1=1235.0 */
        if (d1!=a1) {
            log_err("Fail: Error in parsing currency, got %f, expected %f\n", d1, a1);
        } else {
            log_verbose("Pass: parsed currency ammount successfully\n");
        }
        if (u_strcmp(temp2, temp)==0) {
            log_verbose("Pass: parsed correct currency\n");
        } else {
            log_err("Fail: parsed incorrect currency\n");
        }
    }

free(result);
    result = 0;


/* performance testing */
    u_uastrcpy(temp1, "$462.12345");
    resultlength=u_strlen(temp1);
/*    for (i = 0; i < 100000; i++) */
    {
        parsepos=0;
        d1=unum_parseDouble(cur_def, temp1, resultlength, &parsepos, &status);
    }
    if(U_FAILURE(status))
    {
        log_err("parse failed. The error is  : %s\n", myErrorName(status));
    }

    /*
     * Note: "for strict standard conformance all operations and constants are now supposed to be 
              evaluated in precision of long double".  So,  we assign a1 before comparing to a double. Bug #7932.
     */
    a1 = 462.12345;

    if(d1!=a1)
        log_err("Fail: Error in parsing\n");
    else
        log_verbose("Pass: parsing successful\n");

free(result);

    u_uastrcpy(temp1, "($10,456.3E1])");
    parsepos=0;
    d1=unum_parseDouble(cur_def, temp1, u_strlen(temp1), &parsepos, &status);
    if(U_SUCCESS(status))
    {
        log_err("Error in unum_parseDouble(..., %s, ...): %s\n", temp1, myErrorName(status));
    }


    log_verbose("\nTesting unum_format()\n");
    status=U_ZERO_ERROR;
    resultlength=0;
    parsepos=0;
    resultlengthneeded=unum_format(per_fr, l, NULL, resultlength, &pos1, &status);
    if(status==U_BUFFER_OVERFLOW_ERROR)
    {
        status=U_ZERO_ERROR;
        resultlength=resultlengthneeded+1;
        result=(UChar*)malloc(sizeof(UChar) * resultlength);
/*        for (i = 0; i < 100000; i++)*/
        {
            unum_format(per_fr, l, result, resultlength, &pos1, &status);
        }
    }
    if(U_FAILURE(status))
    {
        log_err("Error in formatting using unum_format(.....): %s\n", myErrorName(status));
    }


    log_verbose("\nTesting unum_parse()\n");
/*    for (i = 0; i < 100000; i++) */
    {
        parsepos=0;
        l1=unum_parse(per_fr, result, u_strlen(result), &parsepos, &status);
    }
    if(U_FAILURE(status))
    {
        log_err("parse failed. The error is  : %s\n", myErrorName(status));
    }

    if(l1!=l)
        log_err("Fail: Error in parsing\n");
    else
        log_verbose("Pass: parsing successful\n");

free(result);

    /* create a number format using unum_openPattern(....)*/
    log_verbose("\nTesting unum_openPattern()\n");
    u_uastrcpy(temp1, "#,##0.0#;(#,##0.0#)");
    pattern=unum_open(UNUM_IGNORE,temp1, u_strlen(temp1), NULL, NULL,&status);
    if(U_FAILURE(status))
    {
        log_err("error in unum_openPattern(): %s\n", myErrorName(status) );;
    }
    else
        log_verbose("Pass: unum_openPattern() works fine\n");

    /*test for unum_toPattern()*/
    log_verbose("\nTesting unum_toPattern()\n");
    resultlength=0;
    resultlengthneeded=unum_toPattern(pattern, FALSE, NULL, resultlength, &status);
    if(status==U_BUFFER_OVERFLOW_ERROR)
    {
        status=U_ZERO_ERROR;
        resultlength=resultlengthneeded+1;
        result=(UChar*)malloc(sizeof(UChar) * resultlength);
        unum_toPattern(pattern, FALSE, result, resultlength, &status);
    }
    if(U_FAILURE(status))
    {
        log_err("error in extracting the pattern from UNumberFormat: %s\n", myErrorName(status));
    }
    else
    {
        if(u_strcmp(result, temp1)!=0)
            log_err("FAIL: Error in extracting the pattern using unum_toPattern()\n");
        else
            log_verbose("Pass: extracted the pattern correctly using unum_toPattern()\n");
free(result);
    }

    /*Testing unum_getSymbols() and unum_setSymbols()*/
    log_verbose("\nTesting unum_getSymbols and unum_setSymbols()\n");
    /*when we try to change the symbols of french to default we need to apply the pattern as well to fetch correct results */
    resultlength=0;
    resultlengthneeded=unum_toPattern(cur_def, FALSE, NULL, resultlength, &status);
    if(status==U_BUFFER_OVERFLOW_ERROR)
    {
        status=U_ZERO_ERROR;
        resultlength=resultlengthneeded+1;
        result=(UChar*)malloc(sizeof(UChar) * resultlength);
        unum_toPattern(cur_def, FALSE, result, resultlength, &status);
    }
    if(U_FAILURE(status))
    {
        log_err("error in extracting the pattern from UNumberFormat: %s\n", myErrorName(status));
    }

    status=U_ZERO_ERROR;
    cur_frpattern=unum_open(UNUM_IGNORE,result, u_strlen(result), "fr_FR",NULL, &status);
    if(U_FAILURE(status))
    {
        log_err("error in unum_openPattern(): %s\n", myErrorName(status));
    }

free(result);

    /*getting the symbols of cur_def */
    /*set the symbols of cur_frpattern to cur_def */
    for (symType = UNUM_DECIMAL_SEPARATOR_SYMBOL; symType < UNUM_FORMAT_SYMBOL_COUNT; symType++) {
        status=U_ZERO_ERROR;
        unum_getSymbol(cur_def, symType, temp1, sizeof(temp1), &status);
        unum_setSymbol(cur_frpattern, symType, temp1, -1, &status);
        if(U_FAILURE(status))
        {
            log_err("Error in get/set symbols: %s\n", myErrorName(status));
        }
    }

    /*format to check the result */
    resultlength=0;
    resultlengthneeded=unum_format(cur_def, l, NULL, resultlength, &pos1, &status);
    if(status==U_BUFFER_OVERFLOW_ERROR)
    {
        status=U_ZERO_ERROR;
        resultlength=resultlengthneeded+1;
        result=(UChar*)malloc(sizeof(UChar) * resultlength);
        unum_format(cur_def, l, result, resultlength, &pos1, &status);
    }
    if(U_FAILURE(status))
    {
        log_err("Error in formatting using unum_format(.....): %s\n", myErrorName(status));
    }

    if(U_FAILURE(status)){
        log_err("Fail: error in unum_setSymbols: %s\n", myErrorName(status));
    }
    unum_applyPattern(cur_frpattern, FALSE, result, u_strlen(result),NULL,NULL);

    for (symType = UNUM_DECIMAL_SEPARATOR_SYMBOL; symType < UNUM_FORMAT_SYMBOL_COUNT; symType++) {
        status=U_ZERO_ERROR;
        unum_getSymbol(cur_def, symType, temp1, sizeof(temp1), &status);
        unum_getSymbol(cur_frpattern, symType, temp2, sizeof(temp2), &status);
        if(U_FAILURE(status) || u_strcmp(temp1, temp2) != 0)
        {
            log_err("Fail: error in getting symbols\n");
        }
        else
            log_verbose("Pass: get and set symbols successful\n");
    }

    /*format and check with the previous result */

    resultlength=0;
    resultlengthneeded=unum_format(cur_frpattern, l, NULL, resultlength, &pos1, &status);
    if(status==U_BUFFER_OVERFLOW_ERROR)
    {
        status=U_ZERO_ERROR;
        resultlength=resultlengthneeded+1;
        unum_format(cur_frpattern, l, temp1, resultlength, &pos1, &status);
    }
    if(U_FAILURE(status))
    {
        log_err("Error in formatting using unum_format(.....): %s\n", myErrorName(status));
    }
    /* TODO: 
     * This test fails because we have not called unum_applyPattern().
     * Currently, such an applyPattern() does not exist on the C API, and
     * we have jitterbug 411 for it.
     * Since it is close to the 1.5 release, I (markus) am disabling this test just
     * for this release (I added the test itself only last week).
     * For the next release, we need to fix this.
     * Then, remove the uprv_strcmp("1.5", ...) and this comment, and the include "cstring.h" at the beginning of this file.
     */
    if(u_strcmp(result, temp1) != 0) {
        log_err("Formatting failed after setting symbols. result=%s temp1=%s\n", result, temp1);
    }


    /*----------- */

free(result);

    /* Testing unum_get/setSymbol() */
    for(i = 0; i < UNUM_FORMAT_SYMBOL_COUNT; ++i) {
        symbol[0] = (UChar)(0x41 + i);
        symbol[1] = (UChar)(0x61 + i);
        unum_setSymbol(cur_frpattern, (UNumberFormatSymbol)i, symbol, 2, &status);
        if(U_FAILURE(status)) {
            log_err("Error from unum_setSymbol(%d): %s\n", i, myErrorName(status));
            return;
        }
    }
    for(i = 0; i < UNUM_FORMAT_SYMBOL_COUNT; ++i) {
        resultlength = unum_getSymbol(cur_frpattern, (UNumberFormatSymbol)i, symbol, sizeof(symbol)/U_SIZEOF_UCHAR, &status);
        if(U_FAILURE(status)) {
            log_err("Error from unum_getSymbol(%d): %s\n", i, myErrorName(status));
            return;
        }
        if(resultlength != 2 || symbol[0] != 0x41 + i || symbol[1] != 0x61 + i) {
            log_err("Failure in unum_getSymbol(%d): got unexpected symbol\n", i);
        }
    }
    /*try getting from a bogus symbol*/
    unum_getSymbol(cur_frpattern, (UNumberFormatSymbol)i, symbol, sizeof(symbol)/U_SIZEOF_UCHAR, &status);
    if(U_SUCCESS(status)){
        log_err("Error : Expected U_ILLEGAL_ARGUMENT_ERROR for bogus symbol");
    }
    if(U_FAILURE(status)){
        if(status != U_ILLEGAL_ARGUMENT_ERROR){
            log_err("Error: Expected U_ILLEGAL_ARGUMENT_ERROR for bogus symbol, Got %s\n", myErrorName(status));
        }
    }
    status=U_ZERO_ERROR;

    /* Testing unum_getTextAttribute() and unum_setTextAttribute()*/
    log_verbose("\nTesting getting and setting text attributes\n");
    resultlength=5;
    unum_getTextAttribute(cur_fr, UNUM_NEGATIVE_SUFFIX, temp, resultlength, &status);
    if(U_FAILURE(status))
    {
        log_err("Failure in gettting the Text attributes of number format: %s\n", myErrorName(status));
    }
    unum_setTextAttribute(cur_def, UNUM_NEGATIVE_SUFFIX, temp, u_strlen(temp), &status);
    if(U_FAILURE(status))
    {
        log_err("Failure in gettting the Text attributes of number format: %s\n", myErrorName(status));
    }
    unum_getTextAttribute(cur_def, UNUM_NEGATIVE_SUFFIX, suffix, resultlength, &status);
    if(U_FAILURE(status))
    {
        log_err("Failure in gettting the Text attributes of number format: %s\n", myErrorName(status));
    }
    if(u_strcmp(suffix,temp)!=0)
        log_err("Fail:Error in setTextAttribute or getTextAttribute in setting and getting suffix\n");
    else
        log_verbose("Pass: setting and getting suffix works fine\n");
    /*set it back to normal */
    u_uastrcpy(temp,"$");
    unum_setTextAttribute(cur_def, UNUM_NEGATIVE_SUFFIX, temp, u_strlen(temp), &status);

    /*checking some more text setter conditions */
    u_uastrcpy(prefix, "+");
    unum_setTextAttribute(def, UNUM_POSITIVE_PREFIX, prefix, u_strlen(prefix) , &status);
    if(U_FAILURE(status))
    {
        log_err("error in setting the text attributes : %s\n", myErrorName(status));
    }
    unum_getTextAttribute(def, UNUM_POSITIVE_PREFIX, temp, resultlength, &status);
    if(U_FAILURE(status))
    {
        log_err("error in getting the text attributes : %s\n", myErrorName(status));
    }

    if(u_strcmp(prefix, temp)!=0) 
        log_err("ERROR: get and setTextAttributes with positive prefix failed\n");
    else
        log_verbose("Pass: get and setTextAttributes with positive prefix works fine\n");

    u_uastrcpy(prefix, "+");
    unum_setTextAttribute(def, UNUM_NEGATIVE_PREFIX, prefix, u_strlen(prefix), &status);
    if(U_FAILURE(status))
    {
        log_err("error in setting the text attributes : %s\n", myErrorName(status));
    }
    unum_getTextAttribute(def, UNUM_NEGATIVE_PREFIX, temp, resultlength, &status);
    if(U_FAILURE(status))
    {
        log_err("error in getting the text attributes : %s\n", myErrorName(status));
    }
    if(u_strcmp(prefix, temp)!=0) 
        log_err("ERROR: get and setTextAttributes with negative prefix failed\n");
    else
        log_verbose("Pass: get and setTextAttributes with negative prefix works fine\n");

    u_uastrcpy(suffix, "+");
    unum_setTextAttribute(def, UNUM_NEGATIVE_SUFFIX, suffix, u_strlen(suffix) , &status);
    if(U_FAILURE(status))
    {
        log_err("error in setting the text attributes: %s\n", myErrorName(status));
    }

    unum_getTextAttribute(def, UNUM_NEGATIVE_SUFFIX, temp, resultlength, &status);
    if(U_FAILURE(status))
    {
        log_err("error in getting the text attributes : %s\n", myErrorName(status));
    }
    if(u_strcmp(suffix, temp)!=0) 
        log_err("ERROR: get and setTextAttributes with negative suffix failed\n");
    else
        log_verbose("Pass: get and settextAttributes with negative suffix works fine\n");

    u_uastrcpy(suffix, "++");
    unum_setTextAttribute(def, UNUM_POSITIVE_SUFFIX, suffix, u_strlen(suffix) , &status);
    if(U_FAILURE(status))
    {
        log_err("error in setting the text attributes: %s\n", myErrorName(status));
    }

    unum_getTextAttribute(def, UNUM_POSITIVE_SUFFIX, temp, resultlength, &status);
    if(U_FAILURE(status))
    {
        log_err("error in getting the text attributes : %s\n", myErrorName(status));
    }
    if(u_strcmp(suffix, temp)!=0) 
        log_err("ERROR: get and setTextAttributes with negative suffix failed\n");
    else
        log_verbose("Pass: get and settextAttributes with negative suffix works fine\n");

    /*Testing unum_getAttribute and  unum_setAttribute() */
    log_verbose("\nTesting get and set Attributes\n");
    attr=UNUM_GROUPING_SIZE;
    newvalue=unum_getAttribute(def, attr);
    newvalue=2;
    unum_setAttribute(def, attr, newvalue);
    if(unum_getAttribute(def,attr)!=2)
        log_err("Fail: error in setting and getting attributes for UNUM_GROUPING_SIZE\n");
    else
        log_verbose("Pass: setting and getting attributes for UNUM_GROUPING_SIZE works fine\n");

    attr=UNUM_MULTIPLIER;
    newvalue=unum_getAttribute(def, attr);
    newvalue=8;
    unum_setAttribute(def, attr, newvalue);
    if(unum_getAttribute(def,attr) != 8)
        log_err("error in setting and getting attributes for UNUM_MULTIPLIER\n");
    else
        log_verbose("Pass:setting and getting attributes for UNUM_MULTIPLIER works fine\n");

    attr=UNUM_SECONDARY_GROUPING_SIZE;
    newvalue=unum_getAttribute(def, attr);
    newvalue=2;
    unum_setAttribute(def, attr, newvalue);
    if(unum_getAttribute(def,attr) != 2)
        log_err("error in setting and getting attributes for UNUM_SECONDARY_GROUPING_SIZE\n");
    else
        log_verbose("Pass:setting and getting attributes for UNUM_SECONDARY_GROUPING_SIZE works fine\n");

    /*testing set and get Attributes extensively */
    log_verbose("\nTesting get and set attributes extensively\n");
    for(attr=UNUM_PARSE_INT_ONLY; attr<= UNUM_PADDING_POSITION; attr=(UNumberFormatAttribute)((int32_t)attr + 1) )
    {
        newvalue=unum_getAttribute(fr, attr);
        unum_setAttribute(def, attr, newvalue);
        if(unum_getAttribute(def,attr)!=unum_getAttribute(fr, attr))
            log_err("error in setting and getting attributes\n");
        else
            log_verbose("Pass: attributes set and retrieved successfully\n");
    }

    /*testing spellout format to make sure we can use it successfully.*/
    log_verbose("\nTesting spellout format\n");
    if (spellout_def)
    {
        static const int32_t values[] = { 0, -5, 105, 1005, 105050 };
        for (i = 0; i < LENGTH(values); ++i) {
            UChar buffer[128];
            int32_t len;
            int32_t value = values[i];
            status = U_ZERO_ERROR;
            len = unum_format(spellout_def, value, buffer, LENGTH(buffer), NULL, &status);
            if(U_FAILURE(status)) {
                log_err("Error in formatting using unum_format(spellout_fmt, ...): %s\n", myErrorName(status));
            } else {
                int32_t pp = 0;
                int32_t parseResult;
                char logbuf[256];
                ustrToAstr(buffer, len, logbuf, LENGTH(logbuf));
                log_verbose("formatted %d as '%s', length: %d\n", value, logbuf, len);

                parseResult = unum_parse(spellout_def, buffer, len, &pp, &status);
                if (U_FAILURE(status)) {
                    log_err("Error in parsing using unum_format(spellout_fmt, ...): %s\n", myErrorName(status));
                } else if (parseResult != value) {
                    log_err("unum_format result %d != value %d\n", parseResult, value);
                }
            }
        }
    }
    else {
        log_err("Spellout format is unavailable\n");
    }

    {    /* Test for ticket #7079 */
        UNumberFormat* dec_en;
        UChar groupingSep[] = { 0 };
        UChar numPercent[] = { 0x0031, 0x0032, 0x0025, 0 }; /* "12%" */
        double parseResult = 0.0;
        
        status=U_ZERO_ERROR;
        dec_en = unum_open(UNUM_DECIMAL, NULL, 0, "en_US", NULL, &status);
        unum_setAttribute(dec_en, UNUM_LENIENT_PARSE, 0);
        unum_setSymbol(dec_en, UNUM_GROUPING_SEPARATOR_SYMBOL, groupingSep, 0, &status);
        parseResult = unum_parseDouble(dec_en, numPercent, -1, NULL, &status);
        /* Without the fix in #7079, the above call will hang */
        if ( U_FAILURE(status) || parseResult != 12.0 ) {
            log_err("unum_parseDouble with empty groupingSep: status %s, parseResult %f not 12.0\n",
                    myErrorName(status), parseResult);
        } else {
            log_verbose("unum_parseDouble with empty groupingSep: no hang, OK\n");
        }
        unum_close(dec_en);
    }
    
    {   /* Test parse & format of big decimals.  Use a number with too many digits to fit in a double,
                                         to verify that it is taking the pure decimal path. */
        UNumberFormat *fmt;
        const char *bdpattern = "#,##0.#########";   
        const char *numInitial     = "12345678900987654321.1234567896";  
        const char *numFormatted  = "12,345,678,900,987,654,321.12345679";
        const char *parseExpected = "12345678900987654321.12345679";
        int32_t resultSize    = 0;
        int32_t parsePos      = 0;     /* Output parameter for Parse operations. */
        #define DESTCAPACITY 100
        UChar dest[DESTCAPACITY];
        char  desta[DESTCAPACITY];
        UFieldPosition fieldPos = {0};

        /* Format */

        status = U_ZERO_ERROR;
        u_uastrcpy(dest, bdpattern);
        fmt = unum_open(UNUM_PATTERN_DECIMAL, dest, -1, "en", NULL /*parseError*/, &status);
        if (U_FAILURE(status)) log_err("File %s, Line %d, status = %s\n", __FILE__, __LINE__, u_errorName(status));

        resultSize = unum_formatDecimal(fmt, numInitial, -1, dest, DESTCAPACITY, NULL, &status); 
        if (U_FAILURE(status)) {
            log_err("File %s, Line %d, status = %s\n", __FILE__, __LINE__, u_errorName(status));
        }
        u_austrncpy(desta, dest, DESTCAPACITY);
        if (strcmp(numFormatted, desta) != 0) {
            log_err("File %s, Line %d, (expected, acutal) =  (\"%s\", \"%s\")\n",
                    __FILE__, __LINE__, numFormatted, desta);
        }
        if (strlen(numFormatted) != resultSize) {
            log_err("File %s, Line %d, (expected, actual) = (%d, %d)\n", 
                     __FILE__, __LINE__, strlen(numFormatted), resultSize);
        }

        /* Format with a FieldPosition parameter */

        fieldPos.field = 2;   /* Ticket 8034 - need enum constants for the field values. */
                              /*  2 =  kDecimalSeparatorField   */
        resultSize = unum_formatDecimal(fmt, numInitial, -1, dest, DESTCAPACITY, &fieldPos, &status); 
        if (U_FAILURE(status)) {
            log_err("File %s, Line %d, status = %s\n", __FILE__, __LINE__, u_errorName(status));
        }
        u_austrncpy(desta, dest, DESTCAPACITY);
        if (strcmp(numFormatted, desta) != 0) {
            log_err("File %s, Line %d, (expected, acutal) =  (\"%s\", \"%s\")\n",
                    __FILE__, __LINE__, numFormatted, desta);
        }
        if (fieldPos.beginIndex != 26) {  /* index of "." in formatted number */
            log_err("File %s, Line %d, (expected, acutal) =  (%d, %d)\n",
                    __FILE__, __LINE__, 0, fieldPos.beginIndex);
        }
        if (fieldPos.endIndex != 27) {
            log_err("File %s, Line %d, (expected, acutal) =  (%d, %d)\n",
                    __FILE__, __LINE__, 0, fieldPos.endIndex);
        }
        
        /* Parse */

        status = U_ZERO_ERROR;
        u_uastrcpy(dest, numFormatted);   /* Parse the expected output of the formatting test */
        resultSize = unum_parseDecimal(fmt, dest, -1, NULL, desta, DESTCAPACITY, &status);
        if (U_FAILURE(status)) {
            log_err("File %s, Line %d, status = %s\n", __FILE__, __LINE__, u_errorName(status));
        }
        if (strcmp(parseExpected, desta) != 0) {
            log_err("File %s, Line %d, (expected, actual) = (\"%s\", \"%s\")\n",
                    __FILE__, __LINE__, parseExpected, desta);
        }
        if (strlen(parseExpected) != resultSize) {
            log_err("File %s, Line %d, (expected, actual) = (%d, %d)\n",
                    __FILE__, __LINE__, strlen(parseExpected), resultSize);
        }

        /* Parse with a parsePos parameter */
        
        status = U_ZERO_ERROR;
        u_uastrcpy(dest, numFormatted);   /* Parse the expected output of the formatting test */
        parsePos = 3;                 /*      12,345,678,900,987,654,321.12345679         */
                                      /* start parsing at the the third char              */
        resultSize = unum_parseDecimal(fmt, dest, -1, &parsePos, desta, DESTCAPACITY, &status);
        if (U_FAILURE(status)) {
            log_err("File %s, Line %d, status = %s\n", __FILE__, __LINE__, u_errorName(status));
        }
        if (strcmp(parseExpected+2, desta) != 0) {   /*  "345678900987654321.12345679" */
            log_err("File %s, Line %d, (expected, actual) = (\"%s\", \"%s\")\n",
                    __FILE__, __LINE__, parseExpected+2, desta);
        }
        if (strlen(numFormatted) != parsePos) {
            log_err("File %s, Line %d, parsePos (expected, actual) = (\"%d\", \"%d\")\n",
                    __FILE__, __LINE__, strlen(parseExpected), parsePos);
        }

        unum_close(fmt);
    }


    /*closing the NumberFormat() using unum_close(UNumberFormat*)")*/
    unum_close(def);
    unum_close(fr);
    unum_close(cur_def);
    unum_close(cur_fr);
    unum_close(per_def);
    unum_close(per_fr);
    unum_close(spellout_def);
    unum_close(pattern);
    unum_close(cur_frpattern);
    unum_close(myclone);

}

typedef struct {
    const char *  testname;
    const char *  locale;
    const UChar * source;
    int32_t       startPos;
    int32_t       value;
    int32_t       endPos;
    UErrorCode    status;
} SpelloutParseTest;

static const UChar ustr_en0[]   = {0x7A, 0x65, 0x72, 0x6F, 0}; /* zero */
static const UChar ustr_123[]   = {0x31, 0x32, 0x33, 0};       /* 123 */
static const UChar ustr_en123[] = {0x6f, 0x6e, 0x65, 0x20, 0x68, 0x75, 0x6e, 0x64, 0x72, 0x65, 0x64,
                                   0x20, 0x74, 0x77, 0x65, 0x6e, 0x74, 0x79,
                                   0x2d, 0x74, 0x68, 0x72, 0x65, 0x65, 0}; /* one hundred twenty-three */
static const UChar ustr_fr123[] = {0x63, 0x65, 0x6e, 0x74, 0x2d, 0x76, 0x69, 0x6e, 0x67, 0x74, 0x2d,
                                   0x74, 0x72, 0x6f, 0x69, 0x73, 0};       /* cent-vingt-trois */
static const UChar ustr_ja123[] = {0x767e, 0x4e8c, 0x5341, 0x4e09, 0};     /* kanji 100(+)2(*)10(+)3 */

static const SpelloutParseTest spelloutParseTests[] = {
    /* name    loc   src       start val  end status */
    { "en0",   "en", ustr_en0,    0,   0,  4, U_ZERO_ERROR },
    { "en0",   "en", ustr_en0,    2,   0,  2, U_PARSE_ERROR },
    { "en0",   "ja", ustr_en0,    0,   0,  0, U_PARSE_ERROR },
    { "123",   "en", ustr_123,    0, 123,  3, U_ZERO_ERROR },
    { "en123", "en", ustr_en123,  0, 123, 24, U_ZERO_ERROR },
    { "en123", "en", ustr_en123, 12,  23, 24, U_ZERO_ERROR },
    { "en123", "fr", ustr_en123, 16,   0, 16, U_PARSE_ERROR },
    { "fr123", "fr", ustr_fr123,  0, 123, 16, U_ZERO_ERROR },
    { "fr123", "fr", ustr_fr123,  5,  23, 16, U_ZERO_ERROR },
    { "fr123", "en", ustr_fr123,  0,   0,  0, U_PARSE_ERROR },
    { "ja123", "ja", ustr_ja123,  0, 123,  4, U_ZERO_ERROR },
    { "ja123", "ja", ustr_ja123,  1,  23,  4, U_ZERO_ERROR },
    { "ja123", "fr", ustr_ja123,  0,   0,  0, U_PARSE_ERROR },
    { NULL,    NULL, NULL,        0,   0,  0, 0 } /* terminator */
};

static void TestSpelloutNumberParse()
{
    const SpelloutParseTest * testPtr;
    for (testPtr = spelloutParseTests; testPtr->testname != NULL; ++testPtr) {
        UErrorCode status = U_ZERO_ERROR;
        int32_t	value, position = testPtr->startPos;
        UNumberFormat *nf = unum_open(UNUM_SPELLOUT, NULL, 0, testPtr->locale, NULL, &status);
        if (U_FAILURE(status)) {
            log_err_status(status, "unum_open fails for UNUM_SPELLOUT with locale %s, status %s\n", testPtr->locale, myErrorName(status));
            continue;
        }
        value = unum_parse(nf, testPtr->source, -1, &position, &status);
        if ( value != testPtr->value || position != testPtr->endPos || status != testPtr->status ) {
            log_err("unum_parse SPELLOUT, locale %s, testname %s, startPos %d: for value / endPos / status, expected %d / %d / %s, got %d / %d / %s\n",
                    testPtr->locale, testPtr->testname, testPtr->startPos,
                    testPtr->value, testPtr->endPos, myErrorName(testPtr->status),
                    value, position, myErrorName(status) );
        }
        unum_close(nf);
    }
}

static void TestSignificantDigits()
{
    UChar temp[128];
    int32_t resultlengthneeded;
    int32_t resultlength;
    UErrorCode status = U_ZERO_ERROR;
    UChar *result = NULL;
    UNumberFormat* fmt;
    double d = 123456.789;

    u_uastrcpy(temp, "###0.0#");
    fmt=unum_open(UNUM_IGNORE, temp, -1, NULL, NULL,&status);
    if (U_FAILURE(status)) {
        log_err("got unexpected error for unum_open: '%s'\n", u_errorName(status));
        return;
    }
    unum_setAttribute(fmt, UNUM_SIGNIFICANT_DIGITS_USED, TRUE);
    unum_setAttribute(fmt, UNUM_MAX_SIGNIFICANT_DIGITS, 6);

    u_uastrcpy(temp, "123457");
    resultlength=0;
    resultlengthneeded=unum_formatDouble(fmt, d, NULL, resultlength, NULL, &status);
    if(status==U_BUFFER_OVERFLOW_ERROR)
    {
        status=U_ZERO_ERROR;
        resultlength=resultlengthneeded+1;
        result=(UChar*)malloc(sizeof(UChar) * resultlength);
        unum_formatDouble(fmt, d, result, resultlength, NULL, &status);
    }
    if(U_FAILURE(status))
    {
        log_err("Error in formatting using unum_formatDouble(.....): %s\n", myErrorName(status));
        return;
    }
    if(u_strcmp(result, temp)==0)
        log_verbose("Pass: Number Formatting using unum_formatDouble() Successful\n");
    else
        log_err("FAIL: Error in number formatting using unum_formatDouble()\n");
    free(result);
    unum_close(fmt);
}

static void TestSigDigRounding()
{
    UErrorCode status = U_ZERO_ERROR;
    UChar expected[128];
    UChar result[128];
    char		temp1[128];
    char		temp2[128];
    UNumberFormat* fmt;
    double d = 123.4;

    fmt=unum_open(UNUM_DECIMAL, NULL, 0, NULL /* "en_US"*/, NULL, &status);
    if (U_FAILURE(status)) {
        log_data_err("got unexpected error for unum_open: '%s'\n", u_errorName(status));
        return;
    }
    unum_setAttribute(fmt, UNUM_LENIENT_PARSE, FALSE);
    unum_setAttribute(fmt, UNUM_SIGNIFICANT_DIGITS_USED, TRUE);
    unum_setAttribute(fmt, UNUM_MAX_SIGNIFICANT_DIGITS, 2);
    /* unum_setAttribute(fmt, UNUM_MAX_FRACTION_DIGITS, 0); */

    unum_setAttribute(fmt, UNUM_ROUNDING_MODE, UNUM_ROUND_UP);
    unum_setDoubleAttribute(fmt, UNUM_ROUNDING_INCREMENT, 20.0);

    (void)unum_formatDouble(fmt, d, result, sizeof(result) / sizeof(result[0]), NULL, &status);
    if(U_FAILURE(status))
    {
        log_err("Error in formatting using unum_formatDouble(.....): %s\n", myErrorName(status));
        return;
    }

    u_uastrcpy(expected, "140");
    if(u_strcmp(result, expected)!=0)
        log_err("FAIL: Error in unum_formatDouble result %s instead of %s\n", u_austrcpy(temp1, result), u_austrcpy(temp2, expected) );
    
    unum_close(fmt);
}

static void TestNumberFormatPadding()
{
    UChar *result=NULL;
    UChar temp1[512];

    UErrorCode status=U_ZERO_ERROR;
    int32_t resultlength;
    int32_t resultlengthneeded;
    UNumberFormat *pattern;
    double d1;
    double d = -10456.37;
    UFieldPosition pos1;
    int32_t parsepos;

    /* create a number format using unum_openPattern(....)*/
    log_verbose("\nTesting unum_openPattern() with padding\n");
    u_uastrcpy(temp1, "*#,##0.0#*;(#,##0.0#)");
    status=U_ZERO_ERROR;
    pattern=unum_open(UNUM_IGNORE,temp1, u_strlen(temp1), NULL, NULL,&status);
    if(U_SUCCESS(status))
    {
        log_err("error in unum_openPattern(%s): %s\n", temp1, myErrorName(status) );
    }
    else
    {
        unum_close(pattern);
    }

/*    u_uastrcpy(temp1, "*x#,###,###,##0.0#;(*x#,###,###,##0.0#)"); */
    u_uastrcpy(temp1, "*x#,###,###,##0.0#;*x(###,###,##0.0#)");
    status=U_ZERO_ERROR;
    pattern=unum_open(UNUM_IGNORE,temp1, u_strlen(temp1), "en_US",NULL, &status);
    if(U_FAILURE(status))
    {
        log_err_status(status, "error in padding unum_openPattern(%s): %s\n", temp1, myErrorName(status) );;
    }
    else {
        log_verbose("Pass: padding unum_openPattern() works fine\n");

        /*test for unum_toPattern()*/
        log_verbose("\nTesting padding unum_toPattern()\n");
        resultlength=0;
        resultlengthneeded=unum_toPattern(pattern, FALSE, NULL, resultlength, &status);
        if(status==U_BUFFER_OVERFLOW_ERROR)
        {
            status=U_ZERO_ERROR;
            resultlength=resultlengthneeded+1;
            result=(UChar*)malloc(sizeof(UChar) * resultlength);
            unum_toPattern(pattern, FALSE, result, resultlength, &status);
        }
        if(U_FAILURE(status))
        {
            log_err("error in extracting the padding pattern from UNumberFormat: %s\n", myErrorName(status));
        }
        else
        {
            if(u_strcmp(result, temp1)!=0)
                log_err("FAIL: Error in extracting the padding pattern using unum_toPattern()\n");
            else
                log_verbose("Pass: extracted the padding pattern correctly using unum_toPattern()\n");
free(result);
        }
/*        u_uastrcpy(temp1, "(xxxxxxx10,456.37)"); */
        u_uastrcpy(temp1, "xxxxx(10,456.37)");
        resultlength=0;
        pos1.field = 1; /* Fraction field */
        resultlengthneeded=unum_formatDouble(pattern, d, NULL, resultlength, &pos1, &status);
        if(status==U_BUFFER_OVERFLOW_ERROR)
        {
            status=U_ZERO_ERROR;
            resultlength=resultlengthneeded+1;
            result=(UChar*)malloc(sizeof(UChar) * resultlength);
            unum_formatDouble(pattern, d, result, resultlength, NULL, &status);
        }
        if(U_FAILURE(status))
        {
            log_err("Error in formatting using unum_formatDouble(.....) with padding : %s\n", myErrorName(status));
        }
        else
        {
            if(u_strcmp(result, temp1)==0)
                log_verbose("Pass: Number Formatting using unum_formatDouble() padding Successful\n");
            else
                log_data_err("FAIL: Error in number formatting using unum_formatDouble() with padding\n");
            if(pos1.beginIndex == 13 && pos1.endIndex == 15)
                log_verbose("Pass: Complete number formatting using unum_formatDouble() successful\n");
            else
                log_err("Fail: Error in complete number Formatting using unum_formatDouble()\nGot: b=%d end=%d\nExpected: b=13 end=15\n",
                        pos1.beginIndex, pos1.endIndex);


            /* Testing unum_parse() and unum_parseDouble() */
            log_verbose("\nTesting padding unum_parseDouble()\n");
            parsepos=0;
            d1=unum_parseDouble(pattern, result, u_strlen(result), &parsepos, &status);
            if(U_FAILURE(status))
            {
                log_err("padding parse failed. The error is : %s\n", myErrorName(status));
            }

            if(d1!=d)
                log_err("Fail: Error in padding parsing\n");
            else
                log_verbose("Pass: padding parsing successful\n");
free(result);
        }
    }

    unum_close(pattern);
}

static UBool
withinErr(double a, double b, double err) {
    return uprv_fabs(a - b) < uprv_fabs(a * err); 
}

static void TestInt64Format() {
    UChar temp1[512];
    UChar result[512];
    UNumberFormat *fmt;
    UErrorCode status = U_ZERO_ERROR;
    const double doubleInt64Max = (double)U_INT64_MAX;
    const double doubleInt64Min = (double)U_INT64_MIN;
    const double doubleBig = 10.0 * (double)U_INT64_MAX;      
    int32_t val32;
    int64_t val64;
    double  valDouble;
    int32_t parsepos;

    /* create a number format using unum_openPattern(....) */
    log_verbose("\nTesting Int64Format\n");
    u_uastrcpy(temp1, "#.#E0");
    fmt = unum_open(UNUM_IGNORE, temp1, u_strlen(temp1), NULL, NULL, &status);
    if(U_FAILURE(status)) {
        log_err("error in unum_openPattern(): %s\n", myErrorName(status));
    } else {
        unum_setAttribute(fmt, UNUM_MAX_FRACTION_DIGITS, 20);
        unum_formatInt64(fmt, U_INT64_MAX, result, 512, NULL, &status);
        if (U_FAILURE(status)) {
            log_err("error in unum_format(): %s\n", myErrorName(status));
        } else {
            log_verbose("format int64max: '%s'\n", result);
            parsepos = 0;
            val32 = unum_parse(fmt, result, u_strlen(result), &parsepos, &status);
            if (status != U_INVALID_FORMAT_ERROR) {
                log_err("parse didn't report error: %s\n", myErrorName(status));
            } else if (val32 != INT32_MAX) {
                log_err("parse didn't pin return value, got: %d\n", val32);
            }

            status = U_ZERO_ERROR;
            parsepos = 0;
            val64 = unum_parseInt64(fmt, result, u_strlen(result), &parsepos, &status);
            if (U_FAILURE(status)) {
                log_err("parseInt64 returned error: %s\n", myErrorName(status));
            } else if (val64 != U_INT64_MAX) {
                log_err("parseInt64 returned incorrect value, got: %ld\n", val64);
            }

            status = U_ZERO_ERROR;
            parsepos = 0;
            valDouble = unum_parseDouble(fmt, result, u_strlen(result), &parsepos, &status);
            if (U_FAILURE(status)) {
                log_err("parseDouble returned error: %s\n", myErrorName(status));
            } else if (valDouble != doubleInt64Max) {
                log_err("parseDouble returned incorrect value, got: %g\n", valDouble);
            }
        }

        unum_formatInt64(fmt, U_INT64_MIN, result, 512, NULL, &status);
        if (U_FAILURE(status)) {
            log_err("error in unum_format(): %s\n", myErrorName(status));
        } else {
            log_verbose("format int64min: '%s'\n", result);
            parsepos = 0;
            val32 = unum_parse(fmt, result, u_strlen(result), &parsepos, &status);
            if (status != U_INVALID_FORMAT_ERROR) {
                log_err("parse didn't report error: %s\n", myErrorName(status));
            } else if (val32 != INT32_MIN) {
                log_err("parse didn't pin return value, got: %d\n", val32);
            }

            status = U_ZERO_ERROR;
            parsepos = 0;
            val64 = unum_parseInt64(fmt, result, u_strlen(result), &parsepos, &status);
            if (U_FAILURE(status)) {
                log_err("parseInt64 returned error: %s\n", myErrorName(status));
            } else if (val64 != U_INT64_MIN) {
                log_err("parseInt64 returned incorrect value, got: %ld\n", val64);
            }

            status = U_ZERO_ERROR;
            parsepos = 0;
            valDouble = unum_parseDouble(fmt, result, u_strlen(result), &parsepos, &status);
            if (U_FAILURE(status)) {
                log_err("parseDouble returned error: %s\n", myErrorName(status));
            } else if (valDouble != doubleInt64Min) {
                log_err("parseDouble returned incorrect value, got: %g\n", valDouble);
            }
        }

        unum_formatDouble(fmt, doubleBig, result, 512, NULL, &status);
        if (U_FAILURE(status)) {
            log_err("error in unum_format(): %s\n", myErrorName(status));
        } else {
            log_verbose("format doubleBig: '%s'\n", result);
            parsepos = 0;
            val32 = unum_parse(fmt, result, u_strlen(result), &parsepos, &status);
            if (status != U_INVALID_FORMAT_ERROR) {
                log_err("parse didn't report error: %s\n", myErrorName(status));
            } else if (val32 != INT32_MAX) {
                log_err("parse didn't pin return value, got: %d\n", val32);
            }

            status = U_ZERO_ERROR;
            parsepos = 0;
            val64 = unum_parseInt64(fmt, result, u_strlen(result), &parsepos, &status);
            if (status != U_INVALID_FORMAT_ERROR) {
                log_err("parseInt64 didn't report error error: %s\n", myErrorName(status));
            } else if (val64 != U_INT64_MAX) {
                log_err("parseInt64 returned incorrect value, got: %ld\n", val64);
            }

            status = U_ZERO_ERROR;
            parsepos = 0;
            valDouble = unum_parseDouble(fmt, result, u_strlen(result), &parsepos, &status);
            if (U_FAILURE(status)) {
                log_err("parseDouble returned error: %s\n", myErrorName(status));
            } else if (!withinErr(valDouble, doubleBig, 1e-15)) {
                log_err("parseDouble returned incorrect value, got: %g\n", valDouble);
            }
        }
    }
    unum_close(fmt);
}


static void test_fmt(UNumberFormat* fmt, UBool isDecimal) {
    char temp[512];
    UChar buffer[512];
    int32_t BUFSIZE = sizeof(buffer)/sizeof(buffer[0]);
    double vals[] = {
        -.2, 0, .2, 5.5, 15.2, 250, 123456789
    };
    int i;

    for (i = 0; i < sizeof(vals)/sizeof(vals[0]); ++i) {
        UErrorCode status = U_ZERO_ERROR;
        unum_formatDouble(fmt, vals[i], buffer, BUFSIZE, NULL, &status);
        if (U_FAILURE(status)) {
            log_err("failed to format: %g, returned %s\n", vals[i], u_errorName(status));
        } else {
            u_austrcpy(temp, buffer);
            log_verbose("formatting %g returned '%s'\n", vals[i], temp);
        }
    }

    /* check APIs now */
    {
        UErrorCode status = U_ZERO_ERROR;
        UParseError perr;
        u_uastrcpy(buffer, "#,##0.0#");
        unum_applyPattern(fmt, FALSE, buffer, -1, &perr, &status);
        if (isDecimal ? U_FAILURE(status) : (status != U_UNSUPPORTED_ERROR)) {
            log_err("got unexpected error for applyPattern: '%s'\n", u_errorName(status));
        }
    }

    {
        int isLenient = unum_getAttribute(fmt, UNUM_LENIENT_PARSE);
        log_verbose("lenient: 0x%x\n", isLenient);
        if (isDecimal ? (isLenient != -1) : (isLenient == TRUE)) {
            log_err("didn't expect lenient value: %d\n", isLenient);
        }

        unum_setAttribute(fmt, UNUM_LENIENT_PARSE, TRUE);
        isLenient = unum_getAttribute(fmt, UNUM_LENIENT_PARSE);
        if (isDecimal ? (isLenient != -1) : (isLenient == FALSE)) {
            log_err("didn't expect lenient value after set: %d\n", isLenient);
        }
    }

    {
        double val2;
        double val = unum_getDoubleAttribute(fmt, UNUM_LENIENT_PARSE);
        if (val != -1) {
            log_err("didn't expect double attribute\n");
        }
        val = unum_getDoubleAttribute(fmt, UNUM_ROUNDING_INCREMENT);
        if ((val == -1) == isDecimal) {
            log_err("didn't expect -1 rounding increment\n");
        }
        unum_setDoubleAttribute(fmt, UNUM_ROUNDING_INCREMENT, val+.5);
        val2 = unum_getDoubleAttribute(fmt, UNUM_ROUNDING_INCREMENT);
        if (isDecimal && (val2 - val != .5)) {
            log_err("set rounding increment had no effect on decimal format");
        }
    }

    {
        UErrorCode status = U_ZERO_ERROR;
        int len = unum_getTextAttribute(fmt, UNUM_DEFAULT_RULESET, buffer, BUFSIZE, &status);
        if (isDecimal ? (status != U_UNSUPPORTED_ERROR) : U_FAILURE(status)) {
            log_err("got unexpected error for get default ruleset: '%s'\n", u_errorName(status));
        }
        if (U_SUCCESS(status)) {
            u_austrcpy(temp, buffer);
            log_verbose("default ruleset: '%s'\n", temp);
        }

        status = U_ZERO_ERROR;
        len = unum_getTextAttribute(fmt, UNUM_PUBLIC_RULESETS, buffer, BUFSIZE, &status);
        if (isDecimal ? (status != U_UNSUPPORTED_ERROR) : U_FAILURE(status)) {
            log_err("got unexpected error for get public rulesets: '%s'\n", u_errorName(status));
        }
        if (U_SUCCESS(status)) {
            u_austrcpy(temp, buffer);
            log_verbose("public rulesets: '%s'\n", temp);

            /* set the default ruleset to the first one found, and retry */

            if (len > 0) {
                for (i = 0; i < len && temp[i] != ';'; ++i){};
                if (i < len) {
                    buffer[i] = 0;
                    unum_setTextAttribute(fmt, UNUM_DEFAULT_RULESET, buffer, -1, &status);
                    if (U_FAILURE(status)) {
                        log_err("unexpected error setting default ruleset: '%s'\n", u_errorName(status));
                    } else {
                        int len2 = unum_getTextAttribute(fmt, UNUM_DEFAULT_RULESET, buffer, BUFSIZE, &status);
                        if (U_FAILURE(status)) {
                            log_err("could not fetch default ruleset: '%s'\n", u_errorName(status));
                        } else if (len2 != i) {
                            u_austrcpy(temp, buffer);
                            log_err("unexpected ruleset len: %d ex: %d val: %s\n", len2, i, temp);
                        } else {
                            for (i = 0; i < sizeof(vals)/sizeof(vals[0]); ++i) {
                                status = U_ZERO_ERROR;
                                unum_formatDouble(fmt, vals[i], buffer, BUFSIZE, NULL, &status);
                                if (U_FAILURE(status)) {
                                    log_err("failed to format: %g, returned %s\n", vals[i], u_errorName(status));
                                } else {
                                    u_austrcpy(temp, buffer);
                                    log_verbose("formatting %g returned '%s'\n", vals[i], temp);
                                }
                            }
                        }
                    }
                }
            }
        }
    }

    {
        UErrorCode status = U_ZERO_ERROR;
        unum_toPattern(fmt, FALSE, buffer, BUFSIZE, &status);
        if (U_SUCCESS(status)) {
            u_austrcpy(temp, buffer);
            log_verbose("pattern: '%s'\n", temp);
        } else if (status != U_BUFFER_OVERFLOW_ERROR) {
            log_err("toPattern failed unexpectedly: %s\n", u_errorName(status));
        } else {
            log_verbose("pattern too long to display\n");
        }
    }

    {
        UErrorCode status = U_ZERO_ERROR;
        int len = unum_getSymbol(fmt, UNUM_CURRENCY_SYMBOL, buffer, BUFSIZE, &status);
        if (isDecimal ? U_FAILURE(status) : (status != U_UNSUPPORTED_ERROR)) {
            log_err("unexpected error getting symbol: '%s'\n", u_errorName(status));
        }

        unum_setSymbol(fmt, UNUM_CURRENCY_SYMBOL, buffer, len, &status);
        if (isDecimal ? U_FAILURE(status) : (status != U_UNSUPPORTED_ERROR)) {
            log_err("unexpected error setting symbol: '%s'\n", u_errorName(status));
        }
    }
}

static void TestNonExistentCurrency() {
    UNumberFormat *format;
    UErrorCode status = U_ZERO_ERROR;
    UChar currencySymbol[8];
    static const UChar QQQ[] = {0x51, 0x51, 0x51, 0};

    /* Get a non-existent currency and make sure it returns the correct currency code. */
    format = unum_open(UNUM_CURRENCY, NULL, 0, "th_TH@currency=QQQ", NULL, &status);
    if (format == NULL || U_FAILURE(status)) {
        log_data_err("unum_open did not return expected result for non-existent requested currency: '%s' (Are you missing data?)\n", u_errorName(status));
    }
    else {
        unum_getSymbol(format,
                UNUM_CURRENCY_SYMBOL,
                currencySymbol,
                sizeof(currencySymbol)/sizeof(currencySymbol[0]),
                &status);
        if (u_strcmp(currencySymbol, QQQ) != 0) {
            log_err("unum_open set the currency to QQQ\n");
        }
    }
    unum_close(format);
}

static void TestRBNFFormat() {
    UErrorCode status;
    UParseError perr;
    UChar pat[1024];
    UChar tempUChars[512];
    UNumberFormat *formats[5];
    int COUNT = sizeof(formats)/sizeof(formats[0]);
    int i;

    for (i = 0; i < COUNT; ++i) {
        formats[i] = 0;
    }

    /* instantiation */
    status = U_ZERO_ERROR;
    u_uastrcpy(pat, "#,##0.0#;(#,##0.0#)");
    formats[0] = unum_open(UNUM_PATTERN_DECIMAL, pat, -1, "en_US", &perr, &status);
    if (U_FAILURE(status)) {
        log_err_status(status, "unable to open decimal pattern -> %s\n", u_errorName(status));
        return;
    }

    status = U_ZERO_ERROR;
    formats[1] = unum_open(UNUM_SPELLOUT, NULL, 0, "en_US", &perr, &status);
    if (U_FAILURE(status)) {
        log_err_status(status, "unable to open spellout -> %s\n", u_errorName(status));
        return;
    }

    status = U_ZERO_ERROR;
    formats[2] = unum_open(UNUM_ORDINAL, NULL, 0, "en_US", &perr, &status);
    if (U_FAILURE(status)) {
        log_err_status(status, "unable to open ordinal -> %s\n", u_errorName(status));
        return;
    }

    status = U_ZERO_ERROR;
    formats[3] = unum_open(UNUM_DURATION, NULL, 0, "en_US", &perr, &status);
    if (U_FAILURE(status)) {
        log_err_status(status, "unable to open duration %s\n", u_errorName(status));
        return;
    }

    status = U_ZERO_ERROR;
    u_uastrcpy(pat,
        "%standard:\n"
        "-x: minus >>;\n"
        "x.x: << point >>;\n"
        "zero; one; two; three; four; five; six; seven; eight; nine;\n"
        "ten; eleven; twelve; thirteen; fourteen; fifteen; sixteen;\n"
        "seventeen; eighteen; nineteen;\n"
        "20: twenty[->>];\n"
        "30: thirty[->>];\n"
        "40: forty[->>];\n"
        "50: fifty[->>];\n"
        "60: sixty[->>];\n"
        "70: seventy[->>];\n"
        "80: eighty[->>];\n"
        "90: ninety[->>];\n"
        "100: =#,##0=;\n");
    u_uastrcpy(tempUChars,
        "%simple:\n"
        "=%standard=;\n"
        "20: twenty[ and change];\n"
        "30: thirty[ and change];\n"
        "40: forty[ and change];\n"
        "50: fifty[ and change];\n"
        "60: sixty[ and change];\n"
        "70: seventy[ and change];\n"
        "80: eighty[ and change];\n"
        "90: ninety[ and change];\n"
        "100: =#,##0=;\n"
        "%bogus:\n"
        "0.x: tiny;\n"
        "x.x: << point something;\n"
        "=%standard=;\n"
        "20: some reasonable number;\n"
        "100: some substantial number;\n"
        "100,000,000: some huge number;\n");
    /* This is to get around some compiler warnings about char * string length. */
    u_strcat(pat, tempUChars);
    formats[4] = unum_open(UNUM_PATTERN_RULEBASED, pat, -1, "en_US", &perr, &status);
    if (U_FAILURE(status)) {
        log_err_status(status, "unable to open rulebased pattern -> %s\n", u_errorName(status));
    }
    if (U_FAILURE(status)) {
        log_err_status(status, "Something failed with %s\n", u_errorName(status));
        return;
    }

    for (i = 0; i < COUNT; ++i) {
        log_verbose("\n\ntesting format %d\n", i);
        test_fmt(formats[i], (UBool)(i == 0));
    }

    for (i = 0; i < COUNT; ++i) {
        unum_close(formats[i]);
    }
}

static void TestCurrencyRegression(void) {
/* 
 I've found a case where unum_parseDoubleCurrency is not doing what I
expect.  The value I pass in is $1234567890q123460000.00 and this
returns with a status of zero error & a parse pos of 22 (I would
expect a parse error at position 11).

I stepped into DecimalFormat::subparse() and it looks like it parses
the first 10 digits and then stops parsing at the q but doesn't set an
error. Then later in DecimalFormat::parse() the value gets crammed
into a long (which greatly truncates the value).

This is very problematic for me 'cause I try to remove chars that are
invalid but this allows my users to enter bad chars and truncates
their data!
*/

    UChar buf[1024];
    UChar currency[8];
    char acurrency[16];
    double d;
    UNumberFormat *cur;
    int32_t pos;
    UErrorCode status  = U_ZERO_ERROR;
    const int32_t expected = 11;

    currency[0]=0;
    u_uastrcpy(buf, "$1234567890q643210000.00");
    cur = unum_open(UNUM_CURRENCY, NULL,0,"en_US", NULL, &status);
    
    if(U_FAILURE(status)) {
        log_data_err("unum_open failed: %s (Are you missing data?)\n", u_errorName(status));
        return;
    }
    
    status = U_ZERO_ERROR; /* so we can test it later. */
    pos = 0;
    
    d = unum_parseDoubleCurrency(cur,
                         buf,
                         -1,
                         &pos, /* 0 = start */
                         currency,
                         &status);

    u_austrcpy(acurrency, currency);

    if(U_FAILURE(status) || (pos != expected)) {
        log_err("unum_parseDoubleCurrency should have failed with pos %d, but gave: value %.9f, err %s, pos=%d, currency [%s]\n",
            expected, d, u_errorName(status), pos, acurrency);
    } else {
        log_verbose("unum_parseDoubleCurrency failed, value %.9f err %s, pos %d, currency [%s]\n", d, u_errorName(status), pos, acurrency);
    }
    
    unum_close(cur);
}

static void TestTextAttributeCrash(void) {
    UChar ubuffer[64] = {0x0049,0x004E,0x0052,0};
    static const UChar expectedNeg[] = {0x0049,0x004E,0x0052,0x0031,0x0032,0x0033,0x0034,0x002E,0x0035,0};
    static const UChar expectedPos[] = {0x0031,0x0032,0x0033,0x0034,0x002E,0x0035,0};
    int32_t used; 
    UErrorCode status = U_ZERO_ERROR;
    UNumberFormat *nf = unum_open(UNUM_CURRENCY, NULL, 0, "en_US", NULL, &status); 
    if (U_FAILURE(status)) {
        log_data_err("FAILED 1 -> %s (Are you missing data?)\n", u_errorName(status));
        return;
    }
    unum_setTextAttribute(nf, UNUM_CURRENCY_CODE, ubuffer, 3, &status);
    /*
     * the usual negative prefix and suffix seem to be '($' and ')' at this point
     * also crashes if UNUM_NEGATIVE_SUFFIX is substituted for UNUM_NEGATIVE_PREFIX here
     */
    used = unum_getTextAttribute(nf, UNUM_NEGATIVE_PREFIX, ubuffer, 64, &status);
    unum_setTextAttribute(nf, UNUM_NEGATIVE_PREFIX, ubuffer, used, &status);
    if (U_FAILURE(status)) {
        log_err("FAILED 2\n"); exit(1);
    }
    log_verbose("attempting to format...\n");
    used = unum_formatDouble(nf, -1234.5, ubuffer, 64, NULL, &status); 
    if (U_FAILURE(status) || 64 < used) {
        log_err("Failed formatting %s\n", u_errorName(status));
        return;
    }
    if (u_strcmp(expectedNeg, ubuffer) == 0) {
        log_err("Didn't get expected negative result\n");
    }
    used = unum_formatDouble(nf, 1234.5, ubuffer, 64, NULL, &status); 
    if (U_FAILURE(status) || 64 < used) {
        log_err("Failed formatting %s\n", u_errorName(status));
        return;
    }
    if (u_strcmp(expectedPos, ubuffer) == 0) {
        log_err("Didn't get expected positive result\n");
    }
    unum_close(nf);
}

static void TestNBSPPatternRtNum(const char *testcase, UNumberFormat *nf, double myNumber) {
    UErrorCode status = U_ZERO_ERROR;
    UChar myString[20];
    char tmpbuf[200];
    double aNumber = -1.0;
    unum_formatDouble(nf, myNumber, myString, 20, NULL, &status);
    log_verbose("%s: formatted %.2f into %s\n", testcase, myNumber, u_austrcpy(tmpbuf, myString));    
    if(U_FAILURE(status)) {
        log_err("%s: failed format of %.2g with %s\n", testcase, myNumber, u_errorName(status));
        return;
    }
    aNumber = unum_parse(nf, myString, -1, NULL, &status);
    if(U_FAILURE(status)) {
        log_err("%s: failed parse with %s\n", testcase, u_errorName(status));
        return;
    }
    if(uprv_fabs(aNumber-myNumber)>.001) {
        log_err("FAIL: %s: formatted %.2f, parsed into %.2f\n", testcase, myNumber, aNumber);
    } else {
        log_verbose("PASS: %s: formatted %.2f, parsed into %.2f\n", testcase, myNumber, aNumber);
    }
}

static void TestNBSPPatternRT(const char *testcase, UNumberFormat *nf) {
    TestNBSPPatternRtNum(testcase, nf, 12345.);
    TestNBSPPatternRtNum(testcase, nf, -12345.);
}

static void TestNBSPInPattern(void) {
    UErrorCode status = U_ZERO_ERROR;
    UNumberFormat* nf = NULL;
    const char *testcase;
    
    
    testcase="ar_AE UNUM_CURRENCY";
    nf  = unum_open(UNUM_CURRENCY, NULL, -1, "ar_AE", NULL, &status);
    if(U_FAILURE(status) || nf == NULL) {
        log_data_err("%s: unum_open failed with %s (Are you missing data?)\n", testcase, u_errorName(status));
        return;
    }
    TestNBSPPatternRT(testcase, nf);
    
    /* if we don't have CLDR 1.6 data, bring out the problem anyways */
    {
#define SPECIAL_PATTERN "\\u00A4\\u00A4'\\u062f.\\u0625.\\u200f\\u00a0'###0.00"
        UChar pat[200];
        testcase = "ar_AE special pattern: " SPECIAL_PATTERN;
        u_unescape(SPECIAL_PATTERN, pat, sizeof(pat)/sizeof(pat[0]));
        unum_applyPattern(nf, FALSE, pat, -1, NULL, &status);
        if(U_FAILURE(status)) { 
            log_err("%s: unum_applyPattern failed with %s\n", testcase, u_errorName(status));
        } else {
            TestNBSPPatternRT(testcase, nf);
        }
#undef SPECIAL_PATTERN
    }
    unum_close(nf); status = U_ZERO_ERROR;
    
    testcase="ar_AE UNUM_DECIMAL";
    nf  = unum_open(UNUM_DECIMAL, NULL, -1, "ar_AE", NULL, &status);
    if(U_FAILURE(status)) {
        log_err("%s: unum_open failed with %s\n", testcase, u_errorName(status));
    }
    TestNBSPPatternRT(testcase, nf);
    unum_close(nf); status = U_ZERO_ERROR;
    
    testcase="ar_AE UNUM_PERCENT";
    nf  = unum_open(UNUM_PERCENT, NULL, -1, "ar_AE", NULL, &status);
    if(U_FAILURE(status)) {
        log_err("%s: unum_open failed with %s\n", testcase, u_errorName(status));
    }    
    TestNBSPPatternRT(testcase, nf);    
    unum_close(nf); status = U_ZERO_ERROR;
    
    
    
}

#endif /* #if !UCONFIG_NO_FORMATTING */
