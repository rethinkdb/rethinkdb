/***********************************************************************
 * COPYRIGHT: 
 * Copyright (c) 1997-2009, International Business Machines Corporation
 * and others. All Rights Reserved.
 ***********************************************************************/

#include "unicode/utypes.h"

#if !UCONFIG_NO_FORMATTING

#include "unicode/decimfmt.h"
#include "tsnmfmt.h"
#include "putilimp.h"
#include <float.h>
#include <stdlib.h>

IntlTestNumberFormat::~IntlTestNumberFormat() {}

static const char * formattableTypeName(Formattable::Type t)
{
  switch(t) {
  case Formattable::kDate: return "kDate";
  case Formattable::kDouble: return "kDouble";
  case Formattable::kLong: return "kLong";
  case Formattable::kString: return "kString";
  case Formattable::kArray: return "kArray";
  case Formattable::kInt64: return "kInt64";
  default: return "??unknown??";
  }
}

/**
 * This test does round-trip testing (format -> parse -> format -> parse -> etc.) of
 * NumberFormat.
 */
void IntlTestNumberFormat::runIndexedTest( int32_t index, UBool exec, const char* &name, char* /*par*/ )
{

    if (exec) logln((UnicodeString)"TestSuite NumberFormat");
    switch (index) {
        case 0: name = "createInstance"; 
            if (exec)
            {
                logln(name);
                fStatus = U_ZERO_ERROR;
                fFormat = NumberFormat::createInstance(fStatus);
                testFormat(/*par*/);
            }
            break;

        case 1: name = "DefaultLocale";
            if (exec) testLocale(/*par, */Locale::getDefault(), name);
            break;

        case 2: name = "testAvailableLocales"; 
            if (exec) {
                logln(name);
                testAvailableLocales(/*par*/);
            }
            break;

        case 3: name = "monsterTest";
            if (exec) {
                logln(name);
                monsterTest(/*par*/);
            }
            break;

        default: name = ""; break;
    }
}

void
IntlTestNumberFormat::testLocale(/* char* par, */const Locale& locale, const UnicodeString& localeName)
{
    const char* name;
    
    fLocale = locale;
    name = "Number test";
    logln((UnicodeString)name + " (" + localeName + ")");
    fStatus = U_ZERO_ERROR;
    fFormat = NumberFormat::createInstance(locale, fStatus);
    testFormat(/* par */);

    name = "Currency test";
    logln((UnicodeString)name + " (" + localeName + ")");
    fStatus = U_ZERO_ERROR;
    fFormat = NumberFormat::createCurrencyInstance(locale, fStatus);
    testFormat(/* par */);

    name = "Percent test";
    logln((UnicodeString)name + " (" + localeName + ")");
    fStatus = U_ZERO_ERROR;
    fFormat = NumberFormat::createPercentInstance(locale, fStatus);
    testFormat(/* par */);
}

double IntlTestNumberFormat::randDouble()
{
    // Assume 8-bit (or larger) rand values.  Also assume
    // that the system rand() function is very poor, which it always is.
    // Call srand(currentTime) in intltest to make it truly random.
    double d;
    uint32_t i;
    char* poke = (char*)&d;
    do {
        for (i=0; i < sizeof(double); ++i)
        {
            poke[i] = (char)(rand() & 0xFF);
        }
    } while (uprv_isNaN(d) || uprv_isInfinite(d)
        || !((-DBL_MAX < d && d < DBL_MAX) || (d < -DBL_MIN && DBL_MIN < d)));

    return d;
}

/*
 * Return a random uint32_t
 **/
uint32_t IntlTestNumberFormat::randLong()
{
    // Assume 8-bit (or larger) rand values.  Also assume
    // that the system rand() function is very poor, which it always is.
    // Call srand(currentTime) in intltest to make it truly random.
    uint32_t d;
    uint32_t i;
    char* poke = (char*)&d;
    for (i=0; i < sizeof(uint32_t); ++i)
    {
        poke[i] = (char)(rand() & 0xFF);
    }
    return d;
}


/* Make sure that we don't get something too large and multiply into infinity.
   @param smallerThanMax the requested maximum value smaller than DBL_MAX */
double IntlTestNumberFormat::getSafeDouble(double smallerThanMax) {
    double it;
    double high = (DBL_MAX/smallerThanMax)/10.0;
    double low = -high;
    do {
        it = randDouble();
    } while (low > it || it > high);
    return it;
}

void
IntlTestNumberFormat::testFormat(/* char* par */)
{
    if (U_FAILURE(fStatus))
    { 
        dataerrln((UnicodeString)"**** FAIL: createXxxInstance failed. - " + u_errorName(fStatus));
        if (fFormat != 0)
            errln("**** FAIL: Non-null format returned by createXxxInstance upon failure.");
        delete fFormat;
        fFormat = 0;
        return;
    }
                    
    if (fFormat == 0)
    {
        errln((UnicodeString)"**** FAIL: Null format returned by createXxxInstance.");
        return;
    }

    UnicodeString str;

    // Assume it's a DecimalFormat and get some info
    DecimalFormat *s = (DecimalFormat*)fFormat;
    logln((UnicodeString)"  Pattern " + s->toPattern(str));

#if defined(OS390) || defined(OS400)
    tryIt(-2.02147304840132e-68);
    tryIt(3.88057859588817e-68); // Test rounding when only some digits are shown because exponent is close to -maxfrac
    tryIt(-2.64651110485945e+65); // Overflows to +INF when shown as a percent
    tryIt(9.29526819488338e+64); // Ok -- used to fail?
#else
    tryIt(-2.02147304840132e-100);
    tryIt(3.88057859588817e-096); // Test rounding when only some digits are shown because exponent is close to -maxfrac
    tryIt(-2.64651110485945e+306); // Overflows to +INF when shown as a percent
    tryIt(9.29526819488338e+250); // Ok -- used to fail?
#endif

    // These PASS now, with the sprintf/atof based format-parse.

    // These fail due to round-off
    // The least significant digit drops by one during each format-parse cycle.
    // Both numbers DON'T have a round-off problem when multiplied by 100! (shown as %)
#ifdef OS390
    tryIt(-9.18228054496402e+64);
    tryIt(-9.69413034454191e+64);
#else
    tryIt(-9.18228054496402e+255);
    tryIt(-9.69413034454191e+273);
#endif

#ifndef OS390
    tryIt(1.234e-200);
    tryIt(-2.3e-168);

    tryIt(uprv_getNaN());
    tryIt(uprv_getInfinity());
    tryIt(-uprv_getInfinity());
#endif

    tryIt((int32_t)251887531);
    tryIt(5e-20 / 9);
    tryIt(5e20 / 9);
    tryIt(1.234e-50);
    tryIt(9.99999999999996);
    tryIt(9.999999999999996);

    tryIt((int32_t)INT32_MIN);
    tryIt((int32_t)INT32_MAX);
    tryIt((double)INT32_MIN);
    tryIt((double)INT32_MAX);
    tryIt((double)INT32_MIN - 1.0);
    tryIt((double)INT32_MAX + 1.0);

    tryIt(5.0 / 9.0 * 1e-20);
    tryIt(4.0 / 9.0 * 1e-20);
    tryIt(5.0 / 9.0 * 1e+20);
    tryIt(4.0 / 9.0 * 1e+20);

    tryIt(2147483647.);
    tryIt((int32_t)0);
    tryIt(0.0);
    tryIt((int32_t)1);
    tryIt((int32_t)10);
    tryIt((int32_t)100);
    tryIt((int32_t)-1);
    tryIt((int32_t)-10);
    tryIt((int32_t)-100);
    tryIt((int32_t)-1913860352);

    for (int32_t z=0; z<10; ++z)
    {
        double d = randFraction() * 2e10 - 1e10;
        tryIt(d);
    }

    double it = getSafeDouble(100000.0);

    tryIt(0.0);
    tryIt(it);
    tryIt((int32_t)0);
    tryIt(uprv_floor(it));
    tryIt((int32_t)randLong());

    // try again
    it = getSafeDouble(100.0);
    tryIt(it);
    tryIt(uprv_floor(it));
    tryIt((int32_t)randLong());

    // try again with very large numbers
    it = getSafeDouble(100000000000.0);
    tryIt(it);

    // try again with very large numbers
    // and without going outside of the int32_t range
    it = randFraction() * INT32_MAX;
    tryIt(it);
    tryIt((int32_t)uprv_floor(it));

    delete fFormat;
}

void 
IntlTestNumberFormat::tryIt(double aNumber)
{
    const int32_t DEPTH = 10;
    Formattable number[DEPTH];
    UnicodeString string[DEPTH];

    int32_t numberMatch = 0;
    int32_t stringMatch = 0;
    UnicodeString errMsg;
    int32_t i;
    for (i=0; i<DEPTH; ++i)
    {
        errMsg.truncate(0); // if non-empty, we failed this iteration
        UErrorCode status = U_ZERO_ERROR;
        string[i] = "(n/a)"; // "format was never done" value
        if (i == 0) {
            number[i].setDouble(aNumber);
        } else {
            fFormat->parse(string[i-1], number[i], status);
            if (U_FAILURE(status)) {
                number[i].setDouble(1234.5); // "parse failed" value
                errMsg = "**** FAIL: Parse of " + prettify(string[i-1]) + " failed.";
                --i; // don't show empty last line: "1234.5 F> (n/a) P>"
                break;
            }
        }
        // Convert from long to double
        if (number[i].getType() == Formattable::kLong)
            number[i].setDouble(number[i].getLong());
        else if (number[i].getType() == Formattable::kInt64)
            number[i].setDouble((double)number[i].getInt64());
        else if (number[i].getType() != Formattable::kDouble)
        {
            errMsg = ("**** FAIL: Parse of " + prettify(string[i-1])
                + " returned non-numeric Formattable, type " + UnicodeString(formattableTypeName(number[i].getType()))
                + ", Locale=" + UnicodeString(fLocale.getName())
                + ", longValue=" + number[i].getLong());
            break;
        }
        string[i].truncate(0);
        fFormat->format(number[i].getDouble(), string[i]);
        if (i > 0)
        {
            if (numberMatch == 0 && number[i] == number[i-1])
                numberMatch = i;
            else if (numberMatch > 0 && number[i] != number[i-1])
            {
                errMsg = ("**** FAIL: Numeric mismatch after match.");
                break;
            }
            if (stringMatch == 0 && string[i] == string[i-1])
                stringMatch = i;
            else if (stringMatch > 0 && string[i] != string[i-1])
            {
                errMsg = ("**** FAIL: String mismatch after match.");
                break;
            }
        }
        if (numberMatch > 0 && stringMatch > 0)
            break;
    }
    if (i == DEPTH)
        --i;

    if (stringMatch > 2 || numberMatch > 2)
    {
        errMsg = ("**** FAIL: No string and/or number match within 2 iterations.");
    }

    if (errMsg.length() != 0)
    {
        for (int32_t k=0; k<=i; ++k)
        {
            logln((UnicodeString)"" + k + ": " + number[k].getDouble() + " F> " +
                  prettify(string[k]) + " P> ");
        }
        errln(errMsg);
    }
}

void 
IntlTestNumberFormat::tryIt(int32_t aNumber)
{
    Formattable number(aNumber);
    UnicodeString stringNum;
    UErrorCode status = U_ZERO_ERROR;

    fFormat->format(number, stringNum, status);
    if (U_FAILURE(status))
    {
        errln("**** FAIL: Formatting " + aNumber);
        return;
    }
    fFormat->parse(stringNum, number, status);
    if (U_FAILURE(status))
    {
        errln("**** FAIL: Parse of " + prettify(stringNum) + " failed.");
        return;
    }
    if (number.getType() != Formattable::kLong)
    {
        errln("**** FAIL: Parse of " + prettify(stringNum)
            + " returned non-long Formattable, type " + UnicodeString(formattableTypeName(number.getType()))
            + ", Locale=" + UnicodeString(fLocale.getName())
            + ", doubleValue=" + number.getDouble()
            + ", longValue=" + number.getLong()
            + ", origValue=" + aNumber
            );
    }
    if (number.getLong() != aNumber) {
        errln("**** FAIL: Parse of " + prettify(stringNum) + " failed. Got:" + number.getLong()
            + " Expected:" + aNumber);
    }
}

void IntlTestNumberFormat::testAvailableLocales(/* char* par */)
{
    int32_t count = 0;
    const Locale* locales = NumberFormat::getAvailableLocales(count);
    logln((UnicodeString)"" + count + " available locales");
    if (locales && count)
    {
        UnicodeString name;
        UnicodeString all;
        for (int32_t i=0; i<count; ++i)
        {
            if (i!=0)
                all += ", ";
            all += locales[i].getName();
        }
        logln(all);
    }
    else
        dataerrln((UnicodeString)"**** FAIL: Zero available locales or null array pointer");
}

void IntlTestNumberFormat::monsterTest(/* char* par */)
{
    const char *SEP = "============================================================\n";
    int32_t count;
    const Locale* allLocales = NumberFormat::getAvailableLocales(count);
    Locale* locales = (Locale*)allLocales;
    Locale quickLocales[6];
    if (allLocales && count)
    {
        if (quick && count > 6) {
            logln("quick test: testing just 6 locales!");
            count = 6;
            locales = quickLocales;
            locales[0] = allLocales[0];
            locales[1] = allLocales[1];
            locales[2] = allLocales[2];
            // In a quick test, make sure we test locales that use
            // currency prefix, currency suffix, and choice currency
            // logic.  Otherwise bugs in these areas can slip through.
            locales[3] = Locale("ar", "AE", "");
            locales[4] = Locale("cs", "CZ", "");
            locales[5] = Locale("en", "IN", "");
        }
        for (int32_t i=0; i<count; ++i)
        {
            UnicodeString name(locales[i].getName(), "");
            logln(SEP);
            testLocale(/* par, */locales[i], name);
        }
    }

    logln(SEP);
}

#endif /* #if !UCONFIG_NO_FORMATTING */
