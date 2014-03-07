/***********************************************************************
 * Copyright (c) 1997-2009, International Business Machines Corporation
 * and others. All Rights Reserved.
 ***********************************************************************/

#include "unicode/utypes.h"

#if !UCONFIG_NO_FORMATTING

#include "unicode/datefmt.h"
#include "unicode/smpdtfmt.h"
#include "tsdate.h"
#include "putilimp.h"

#include <float.h>
#include <stdlib.h>
#include <math.h>

const double IntlTestDateFormat::ONEYEAR = 365.25 * ONEDAY; // Approximate

IntlTestDateFormat::~IntlTestDateFormat() {}

/**
 * This test does round-trip testing (format -> parse -> format -> parse -> etc.) of
 * DateFormat.
 */
// par is ignored throughout this file
void IntlTestDateFormat::runIndexedTest( int32_t index, UBool exec, const char* &name, char* /*par*/ )
{
    if (exec) logln("TestSuite DateFormat");
    switch (index) {
        case 0: name = "GenericTest";
            if (exec) {
                logln(name);
                fFormat = DateFormat::createInstance();
                fTestName = "createInstance";
                fLimit = 3;
                testFormat(/* par */);
            }
            break;
        case 1: name = "DefaultLocale";
            if (exec) {
                logln(name);
                testLocale(/*par, */Locale::getDefault(), "Default Locale");
            }
            break;

        case 2: name = "TestAvailableLocales";
            if (exec) {
                logln(name);
                testAvailableLocales(/* par */);
            }
            break;

        case 3: name = "MonsterTest";
            if (exec) {
                logln(name);
                monsterTest(/*par*/);
            }
            break;

        default: name = ""; break;
    }
}

void
IntlTestDateFormat::testLocale(/*char* par, */const Locale& locale, const UnicodeString& localeName)
{
    DateFormat::EStyle timeStyle, dateStyle;
    
    // For patterns including only time information and a timezone, it may take
    // up to three iterations, since the timezone may shift as the year number
    // is determined.  For other patterns, 2 iterations should suffice.
    fLimit = 3;

    for(timeStyle = (DateFormat::EStyle)0; 
        timeStyle < (DateFormat::EStyle)4; 
        timeStyle = (DateFormat::EStyle) (timeStyle+1))
    {
        fTestName = (UnicodeString) "Time test " + (int32_t) timeStyle + " (" + localeName + ")";
        fFormat = DateFormat::createTimeInstance(timeStyle, locale);
        testFormat(/* par */);
    }

    fLimit = 2;

    for(dateStyle = (DateFormat::EStyle)0; 
        dateStyle < (DateFormat::EStyle)4; 
        dateStyle = (DateFormat::EStyle) (dateStyle+1))
    {
        fTestName = (UnicodeString) "Date test " + (int32_t) dateStyle + " (" + localeName + ")";
        fFormat = DateFormat::createDateInstance(dateStyle, locale);
        testFormat(/* par */);
    }

    for(dateStyle = (DateFormat::EStyle)0; 
        dateStyle < (DateFormat::EStyle)4; 
        dateStyle = (DateFormat::EStyle) (dateStyle+1))
    {
        for(timeStyle = (DateFormat::EStyle)0; 
            timeStyle < (DateFormat::EStyle)4; 
            timeStyle = (DateFormat::EStyle) (timeStyle+1))
        {
            fTestName = (UnicodeString) "DateTime test " + (int32_t) dateStyle + "/" + (int32_t) timeStyle + " (" + localeName + ")";
            fFormat = DateFormat::createDateTimeInstance(dateStyle, timeStyle, locale);
            testFormat(/* par */);
        }
    }
}

void IntlTestDateFormat::testFormat(/* char* par */)
{
    if (fFormat == 0)
    {
        dataerrln("FAIL: DateFormat creation failed");
        return;
    }

    describeTest();

    UDate now = Calendar::getNow();
    tryDate(0);
    tryDate(1278161801778.0);
    tryDate(5264498352317.0);   // Sunday, October 28, 2136 8:39:12 AM PST
    tryDate(9516987689250.0);   // In the year 2271
    tryDate(now);
    // Shift 6 months into the future, AT THE SAME TIME OF DAY.
    // This will test the DST handling.
    tryDate(now + 6.0*30*ONEDAY);

    UDate limit = now * 10; // Arbitrary limit
    for (int32_t i=0; i<3; ++i)
        tryDate(uprv_floor(randDouble() * limit));

    delete fFormat;
}

void
IntlTestDateFormat::describeTest()
{
    // Assume it's a SimpleDateFormat and get some info
    SimpleDateFormat *s = (SimpleDateFormat*)fFormat;
    UnicodeString str;
    logln(fTestName + " Pattern " + s->toPattern(str));
}

void IntlTestDateFormat::tryDate(UDate theDate)
{
    const int32_t DEPTH = 10;
    UDate date[DEPTH];
    UnicodeString string[DEPTH];

    int32_t dateMatch = 0;
    int32_t stringMatch = 0;
    UBool dump = FALSE;
#if defined (U_CAL_DEBUG)
    dump = TRUE;
#endif
    int32_t i;

    date[0] = theDate;
    fFormat->format(theDate, string[0]);

    for (i=1; i<DEPTH; ++i)
    {
        UErrorCode status = U_ZERO_ERROR;
        date[i] = fFormat->parse(string[i-1], status);
        if (U_FAILURE(status))
        {
            describeTest();
            errln("**** FAIL: Parse of " + prettify(string[i-1], FALSE) + " failed.");
            dump = TRUE;
            break;
        }
        fFormat->format(date[i], string[i]);
        if (dateMatch == 0 && date[i] == date[i-1])
            dateMatch = i;
        else if (dateMatch > 0 && date[i] != date[i-1])
        {
            describeTest();
            errln("**** FAIL: Date mismatch after match for " + string[i]);
            dump = TRUE;
            break;
        }
        if (stringMatch == 0 && string[i] == string[i-1])
            stringMatch = i;
        else if (stringMatch > 0 && string[i] != string[i-1])
        {
            describeTest();
            errln("**** FAIL: String mismatch after match for " + string[i]);
            dump = TRUE;
            break;
        }
        if (dateMatch > 0 && stringMatch > 0)
            break;
    }
    if (i == DEPTH)
        --i;

    if (stringMatch > fLimit || dateMatch > fLimit)
    {
        describeTest();
        errln((UnicodeString)"**** FAIL: No string and/or date match within " + fLimit
            + " iterations for the Date " + string[0] + "\t(" + theDate + ").");
        dump = TRUE;
    }

    if (dump)
    {
        for (int32_t k=0; k<=i; ++k)
        {
            logln((UnicodeString)"" + k + ": " + date[k] + " F> " +
                  string[k] + " P> ");
        }
    }
}

// Return a random double from 0.01 to 1, inclusive
double IntlTestDateFormat::randDouble()
{
    // Assume 8-bit (or larger) rand values.  Also assume
    // that the system rand() function is very poor, which it always is.
    double d=0.0;
    uint32_t i;
    char* poke = (char*)&d;
    do {
        do {
            for (i=0; i < sizeof(double); ++i)
            {
                poke[i] = (char)(rand() & 0xFF);
            }
        } while (uprv_isNaN(d) || uprv_isInfinite(d));

        if (d < 0.0)
            d = -d;
        if (d > 0.0)
        {
            double e = uprv_floor(log10(d));
            if (e < -2.0)
                d *= uprv_pow10((int32_t)(-e-2));
            else if (e > -1.0)
                d /= uprv_pow10((int32_t)(e+1));
        }
    // While this is not a real normalized number make another one.
    } while (uprv_isNaN(d) || uprv_isInfinite(d)
        || !((-DBL_MAX < d && d < DBL_MAX) || (d < -DBL_MIN && DBL_MIN < d)));
    return d;
}

void IntlTestDateFormat::testAvailableLocales(/* char* par */)
{
    int32_t count = 0;
    const Locale* locales = DateFormat::getAvailableLocales(count);
    logln((UnicodeString)"" + count + " available locales");
    if (locales && count)
    {
        UnicodeString name;
        UnicodeString all;
        for (int32_t i=0; i<count; ++i)
        {
            if (i!=0) all += ", ";
            all += locales[i].getName();
        }
        logln(all);
    }
    else dataerrln((UnicodeString)"**** FAIL: Zero available locales or null array pointer");
}

void IntlTestDateFormat::monsterTest(/*char *par*/)
{
    int32_t count;
    const Locale* locales = DateFormat::getAvailableLocales(count);
    if (locales && count)
    {
        if (quick && count > 3) {
            logln("quick test: testing just 3 locales!");
            count = 3;
        }
        for (int32_t i=0; i<count; ++i)
        {
            UnicodeString name = UnicodeString(locales[i].getName(), "");
            logln((UnicodeString)"Testing " + name + "...");
            testLocale(/*par, */locales[i], name);
        }
    }
}

#endif /* #if !UCONFIG_NO_FORMATTING */
