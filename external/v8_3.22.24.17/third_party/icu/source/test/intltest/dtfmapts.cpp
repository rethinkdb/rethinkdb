/***********************************************************************
 * COPYRIGHT: 
 * Copyright (c) 1997-2010, International Business Machines Corporation
 * and others. All Rights Reserved.
 ***********************************************************************/

#include "unicode/utypes.h"

#if !UCONFIG_NO_FORMATTING

#include "dtfmapts.h"

#include "unicode/datefmt.h"
#include "unicode/smpdtfmt.h"
#include "unicode/decimfmt.h"
#include "unicode/choicfmt.h"
#include "unicode/msgfmt.h"


// This is an API test, not a unit test.  It doesn't test very many cases, and doesn't
// try to test the full functionality.  It just calls each function in the class and
// verifies that it works on a basic level.

void IntlTestDateFormatAPI::runIndexedTest( int32_t index, UBool exec, const char* &name, char* /*par*/ )
{
    if (exec) logln("TestSuite DateFormatAPI");
    switch (index) {
        case 0: name = "DateFormat API test"; 
                if (exec) {
                    logln("DateFormat API test---"); logln("");
                    UErrorCode status = U_ZERO_ERROR;
                    Locale saveLocale;
                    Locale::setDefault(Locale::getEnglish(), status);
                    if(U_FAILURE(status)) {
                        errln("ERROR: Could not set default locale, test may not give correct results");
                    }
                    testAPI(/*par*/);
                    Locale::setDefault(saveLocale, status);
                }
                break;

        case 1: name = "TestEquals"; 
                if (exec) {
                    logln("TestEquals---"); logln("");
                    TestEquals();
                }
                break;

        case 2: name = "TestNameHiding"; 
                if (exec) {
                    logln("TestNameHiding---"); logln("");
                    TestNameHiding();
                }
                break;

        case 3: name = "TestCoverage";
                if (exec) {
                    logln("TestCoverage---"); logln("");
                    TestCoverage();
                }
                break;

        default: name = ""; break;
    }
}

/**
 * Add better code coverage.
 */
void IntlTestDateFormatAPI::TestCoverage(void)
{
    const char *LOCALES[] = {
            "zh_CN@calendar=chinese",
            "cop_EG@calendar=coptic",
            "hi_IN@calendar=indian",
            "am_ET@calendar=ethiopic"
    };
    int32_t numOfLocales = 4;

    for (int32_t i = 0; i < numOfLocales; i++) {
        DateFormat *df = DateFormat::createDateTimeInstance(DateFormat::kMedium, DateFormat::kMedium, Locale(LOCALES[i]));
        if (df == NULL){
            dataerrln("Error creating DateFormat instances.");
            return;
        }
        delete df;
    }
}
/**
 * Test that the equals method works correctly.
 */
void IntlTestDateFormatAPI::TestEquals(void)
{
    UErrorCode status = U_ZERO_ERROR;
    // Create two objects at different system times
    DateFormat *a = DateFormat::createInstance();
    UDate start = Calendar::getNow();
    while (Calendar::getNow() == start) ; // Wait for time to change
    DateFormat *b = DateFormat::createInstance();

    if (a == NULL || b == NULL){
        dataerrln("Error calling DateFormat::createInstance()");
        delete a;
        delete b;
        return;
    }

    if (!(*a == *b))
        errln("FAIL: DateFormat objects created at different times are unequal.");

    SimpleDateFormat *sdtfmt = dynamic_cast<SimpleDateFormat *>(b);
    if (sdtfmt != NULL)
    {
        double ONE_YEAR = 365*24*60*60*1000.0;
        sdtfmt->set2DigitYearStart(start + 50*ONE_YEAR, status);
        if (U_FAILURE(status))
            errln("FAIL: setTwoDigitStartDate failed.");
        else if (*a == *b)
            errln("FAIL: DateFormat objects with different two digit start dates are equal.");
    }
    delete a;
    delete b;
}

/**
 * This test checks various generic API methods in DateFormat to achieve 100%
 * API coverage.
 */
void IntlTestDateFormatAPI::testAPI(/* char* par */)
{
    UErrorCode status = U_ZERO_ERROR;

// ======= Test constructors

    logln("Testing DateFormat constructors");

    DateFormat *def = DateFormat::createInstance();
    DateFormat *fr = DateFormat::createTimeInstance(DateFormat::FULL, Locale::getFrench());
    DateFormat *it = DateFormat::createDateInstance(DateFormat::MEDIUM, Locale::getItalian());
    DateFormat *de = DateFormat::createDateTimeInstance(DateFormat::LONG, DateFormat::LONG, Locale::getGerman());

    if (def == NULL || fr == NULL || it == NULL || de == NULL){
        dataerrln("Error creating DateFormat instances.");
    }

// ======= Test equality
if (fr != NULL && def != NULL)
{
    logln("Testing equality operator");
    
    if( *fr == *it ) {
        errln("ERROR: == failed");
    }
}

// ======= Test various format() methods
if (fr != NULL && it != NULL && de != NULL)
{
    logln("Testing various format() methods");

    UDate d = 837039928046.0;
    Formattable fD(d, Formattable::kIsDate);

    UnicodeString res1, res2, res3;
    FieldPosition pos1(0), pos2(0);
    
    status = U_ZERO_ERROR;
    res1 = fr->format(d, res1, pos1, status);
    if(U_FAILURE(status)) {
        errln("ERROR: format() failed (French)");
    }
    logln( (UnicodeString) "" + d + " formatted to " + res1);

    res2 = it->format(d, res2, pos2);
    logln( (UnicodeString) "" + d + " formatted to " + res2);

    res3 = de->format(d, res3);
    logln( (UnicodeString) "" + d + " formatted to " + res3);
}

// ======= Test parse()
if (def != NULL)
{
    logln("Testing parse()");

    UnicodeString text("02/03/76 2:50 AM, CST");
    Formattable result1;
    UDate result2, result3;
    ParsePosition pos(0), pos01(0);
    def->parseObject(text, result1, pos);
    if(result1.getType() != Formattable::kDate) {
        errln("ERROR: parseObject() failed for " + text);
    }
    logln(text + " parsed into " + result1.getDate());

    status = U_ZERO_ERROR;
    result2 = def->parse(text, status);
    if(U_FAILURE(status)) {
        errln("ERROR: parse() failed, stopping testing");
        return;
    }
    logln(text + " parsed into " + result2);

    result3 = def->parse(text, pos01);
    logln(text + " parsed into " + result3);
}

// ======= Test getters and setters
if (fr != NULL && it != NULL && de != NULL)
{
    logln("Testing getters and setters");

    int32_t count = 0;
    const Locale *locales = DateFormat::getAvailableLocales(count);
    logln((UnicodeString) "Got " + count + " locales" );
    for(int32_t i = 0; i < count; i++) {
        UnicodeString name;
        name = locales[i].getName();
        logln(name);
    }

    fr->setLenient(it->isLenient());
    if(fr->isLenient() != it->isLenient()) {
        errln("ERROR: setLenient() failed");
    }

    const Calendar *cal = def->getCalendar();
    Calendar *newCal = cal->clone();
    de->adoptCalendar(newCal);  
    it->setCalendar(*newCal);
    if( *(de->getCalendar()) != *(it->getCalendar())) {
        errln("ERROR: adopt or set Calendar() failed");
    }

    const NumberFormat *nf = def->getNumberFormat();
    NumberFormat *newNf = (NumberFormat*) nf->clone();
    de->adoptNumberFormat(newNf);   
    it->setNumberFormat(*newNf);
    if( *(de->getNumberFormat()) != *(it->getNumberFormat())) {
        errln("ERROR: adopt or set NumberFormat() failed");
    }

    const TimeZone& tz = def->getTimeZone();
    TimeZone *newTz = tz.clone();
    de->adoptTimeZone(newTz);   
    it->setTimeZone(*newTz);
    if( de->getTimeZone() != it->getTimeZone()) {
        errln("ERROR: adopt or set TimeZone() failed");
    }
}
// ======= Test getStaticClassID()

    logln("Testing getStaticClassID()");

    status = U_ZERO_ERROR;
    DateFormat *test = new SimpleDateFormat(status);
    if(U_FAILURE(status)) {
        dataerrln("ERROR: Couldn't create a DateFormat - %s", u_errorName(status));
    }

    if(test->getDynamicClassID() != SimpleDateFormat::getStaticClassID()) {
        errln("ERROR: getDynamicClassID() didn't return the expected value");
    }

    delete test;
    delete def;
    delete fr;
    delete it;
    delete de;
}

/**
 * Test hiding of parse() and format() APIs in the Format hierarchy.
 * We test the entire hierarchy, even though this test is located in
 * the DateFormat API test.
 */
void
IntlTestDateFormatAPI::TestNameHiding(void) {

    // N.B.: This test passes if it COMPILES, since it's a test of
    // compile-time name hiding.

    UErrorCode status = U_ZERO_ERROR;
    Formattable dateObj(0, Formattable::kIsDate);
    Formattable numObj(3.1415926535897932384626433832795);
    Formattable obj;
    UnicodeString str;
    FieldPosition fpos;
    ParsePosition ppos;

    // DateFormat calling Format API
    {
        logln("DateFormat");
        DateFormat *dateFmt = DateFormat::createInstance();
        if (dateFmt) {
            dateFmt->format(dateObj, str, status);
            dateFmt->format(dateObj, str, fpos, status);
            delete dateFmt;
        } else {
            dataerrln("FAIL: Can't create DateFormat");
        }
    }

    // SimpleDateFormat calling Format & DateFormat API
    {
        logln("SimpleDateFormat");
        status = U_ZERO_ERROR;
        SimpleDateFormat sdf(status);
        // Format API
        sdf.format(dateObj, str, status);
        sdf.format(dateObj, str, fpos, status);
        // DateFormat API
        sdf.format((UDate)0, str, fpos);
        sdf.format((UDate)0, str);
        sdf.parse(str, status);
        sdf.parse(str, ppos);
        sdf.getNumberFormat();
    }

    // NumberFormat calling Format API
    {
        logln("NumberFormat");
        status = U_ZERO_ERROR;
        NumberFormat *fmt = NumberFormat::createInstance(status);
        if (fmt) {
            fmt->format(numObj, str, status);
            fmt->format(numObj, str, fpos, status);
            delete fmt;
        } else {
            dataerrln("FAIL: Can't create NumberFormat()");
        }
    }

    // DecimalFormat calling Format & NumberFormat API
    {
        logln("DecimalFormat");
        status = U_ZERO_ERROR;
        DecimalFormat fmt(status);
        if(U_SUCCESS(status)) {
          // Format API
          fmt.format(numObj, str, status);
          fmt.format(numObj, str, fpos, status);
          // NumberFormat API
          fmt.format(2.71828, str);
          fmt.format((int32_t)1234567, str);
          fmt.format(1.41421, str, fpos);
          fmt.format((int32_t)9876543, str, fpos);
          fmt.parse(str, obj, ppos);
          fmt.parse(str, obj, status);
        } else {
          errcheckln(status, "FAIL: Couldn't instantiate DecimalFormat, error %s. Quitting test", u_errorName(status));
        }
    }

    // ChoiceFormat calling Format & NumberFormat API
    {
        logln("ChoiceFormat");
        status = U_ZERO_ERROR;
        ChoiceFormat fmt("0#foo|1#foos|2#foos", status);
        // Format API
        fmt.format(numObj, str, status);
        fmt.format(numObj, str, fpos, status);
        // NumberFormat API
        fmt.format(2.71828, str);
        fmt.format((int32_t)1234567, str);
        fmt.format(1.41421, str, fpos);
        fmt.format((int32_t)9876543, str, fpos);
        fmt.parse(str, obj, ppos);
        fmt.parse(str, obj, status);
    }

    // MessageFormat calling Format API
    {
        logln("MessageFormat");
        status = U_ZERO_ERROR;
        MessageFormat fmt("", status);
        // Format API
        // We use dateObj, which MessageFormat should reject.
        // We're testing name hiding, not the format method.
        fmt.format(dateObj, str, status);
        fmt.format(dateObj, str, fpos, status);
    }
}

#endif /* #if !UCONFIG_NO_FORMATTING */
