/***********************************************************************
 * COPYRIGHT: 
 * Copyright (c) 1997-2010, International Business Machines Corporation
 * and others. All Rights Reserved.
 ***********************************************************************/

#include "unicode/utypes.h"

#if !UCONFIG_NO_FORMATTING

#include "nmfmtrt.h"

#include "unicode/dcfmtsym.h"
#include "unicode/decimfmt.h"
#include "unicode/locid.h"
#include "putilimp.h"

#include <float.h>
#include <stdio.h>    // for sprintf
#include <stdlib.h>
 
// *****************************************************************************
// class NumberFormatRoundTripTest
// *****************************************************************************

UBool NumberFormatRoundTripTest::verbose                  = FALSE;
UBool NumberFormatRoundTripTest::STRING_COMPARE           = TRUE;
UBool NumberFormatRoundTripTest::EXACT_NUMERIC_COMPARE    = FALSE;
UBool NumberFormatRoundTripTest::DEBUG                    = FALSE;
double NumberFormatRoundTripTest::MAX_ERROR               = 1e-14;
double NumberFormatRoundTripTest::max_numeric_error       = 0.0;
double NumberFormatRoundTripTest::min_numeric_error       = 1.0;

#define CASE(id,test) case id: name = #test; if (exec) { logln(#test "---"); logln((UnicodeString)""); test(); } break;

void NumberFormatRoundTripTest::runIndexedTest( int32_t index, UBool exec, const char* &name, char* /*par*/ )
{
    // if (exec) logln((UnicodeString)"TestSuite NumberFormatRoundTripTest");
    switch (index) {
        CASE(0, start)
        default: name = ""; break;
    }
}

UBool 
NumberFormatRoundTripTest::failure(UErrorCode status, const char* msg, UBool possibleDataError)
{
    if(U_FAILURE(status)) {
        if (possibleDataError) {
            dataerrln(UnicodeString("FAIL: ") + msg + " failed, error " + u_errorName(status));
        } else {
            errln(UnicodeString("FAIL: ") + msg + " failed, error " + u_errorName(status));
        }
        return TRUE;
    }

    return FALSE;
}

uint32_t
NumberFormatRoundTripTest::randLong()
{
    // Assume 8-bit (or larger) rand values.  Also assume
    // that the system rand() function is very poor, which it always is.
    uint32_t d;
    uint32_t i;
    char* poke = (char*)&d;
    for (i=0; i < sizeof(uint32_t); ++i)
    {
        poke[i] = (char)(rand() & 0xFF);
    }
    return d;
}

/**
 * Return a random value from -range..+range.
 */
double 
NumberFormatRoundTripTest::randomDouble(double range)
{
    double a = randFraction();
    return (2.0 * range * a) - range;
}

void 
NumberFormatRoundTripTest::start()
{
// test(NumberFormat.getInstance(new Locale("sr", "", "")));

    UErrorCode status = U_ZERO_ERROR;

    NumberFormat *fmt = NULL;

    logln("Default Locale");

    fmt = NumberFormat::createInstance(status);
    if (!failure(status, "NumberFormat::createInstance", TRUE)){
        test(fmt);
    }
    delete fmt;

    fmt = NumberFormat::createCurrencyInstance(status);
    if (!failure(status, "NumberFormat::createCurrencyInstance", TRUE)){
        test(fmt);
    }
    delete fmt;

    fmt = NumberFormat::createPercentInstance(status);
    if (!failure(status, "NumberFormat::createPercentInstance", TRUE)){
        test(fmt);
    }
    delete fmt;


    int32_t locCount = 0;
    const Locale *loc = NumberFormat::getAvailableLocales(locCount);
    if(quick) {
        if(locCount > 5)
            locCount = 5;
        logln("Quick mode: only testing first 5 Locales");
    }
    for(int i = 0; i < locCount; ++i) {
        UnicodeString name;
        logln(loc[i].getDisplayName(name));

        fmt = NumberFormat::createInstance(loc[i], status);
        failure(status, "NumberFormat::createInstance");
        test(fmt);
        delete fmt;
    
        fmt = NumberFormat::createCurrencyInstance(loc[i], status);
        failure(status, "NumberFormat::createCurrencyInstance");
        test(fmt);
        delete fmt;
    
        fmt = NumberFormat::createPercentInstance(loc[i], status);
        failure(status, "NumberFormat::createPercentInstance");
        test(fmt);
        delete fmt;
    }

    logln(UnicodeString("Numeric error ") + min_numeric_error + " to " + max_numeric_error);
}


void 
NumberFormatRoundTripTest::test(NumberFormat *fmt)
{
#if IEEE_754 && !defined(OS400)
    test(fmt, uprv_getNaN());
    test(fmt, uprv_getInfinity());
    test(fmt, -uprv_getInfinity());
#endif

    test(fmt, (int32_t)500);
    test(fmt, (int32_t)0);
    test(fmt, (int32_t)-0);
    test(fmt, 0.0);
    double negZero = 0.0; negZero /= -1.0;
    test(fmt, negZero);
    test(fmt, 9223372036854775808.0);
    test(fmt, -9223372036854775809.0);

    for(int i = 0; i < 10; ++i) {
        test(fmt, randomDouble(1));
        test(fmt, randomDouble(10000));
        test(fmt, uprv_floor((randomDouble(10000))));
        test(fmt, randomDouble(1e50));
        test(fmt, randomDouble(1e-50));
#if !defined(OS390) && !defined(OS400)
        test(fmt, randomDouble(1e100));
#elif IEEE_754
        test(fmt, randomDouble(1e75));    /*OS390 and OS400*/
#endif /* OS390 and OS400 */
        // {sfb} When formatting with a percent instance, numbers very close to
        // DBL_MAX will fail the round trip.  This is because:
        // 1) Format the double into a string --> INF% (since 100 * double > DBL_MAX)
        // 2) Parse the string into a double --> INF
        // 3) Re-format the double --> INF%
        // 4) The strings are equal, so that works.
        // 5) Calculate the proportional error --> INF, so the test will fail
        // I'll get around this by dividing by the multiplier to make sure
        // the double will stay in range.
        //if(fmt->getMultipler() == 1)
        DecimalFormat *df = dynamic_cast<DecimalFormat *>(fmt);
        if(df != NULL)
        {
#if !defined(OS390) && !defined(OS400)
            /* DBL_MAX/2 is here because randomDouble does a *2 in the math */
            test(fmt, randomDouble(DBL_MAX/2.0) / df->getMultiplier());
#elif IEEE_754
            test(fmt, randomDouble(1e75) / df->getMultiplier());   
#else
            test(fmt, randomDouble(1e65) / df->getMultiplier());   /*OS390*/
#endif
        }

#if (defined(_MSC_VER) && _MSC_VER < 1400) || defined(__alpha__) || defined(U_OSF)
        // These machines and compilers don't fully support denormalized doubles,
        test(fmt, randomDouble(1e-292));
        test(fmt, randomDouble(1e-100));
#elif defined(OS390) || defined(OS400)
        // i5/OS (OS400) throws exceptions on denormalized numbers
#   if IEEE_754
        test(fmt, randomDouble(1e-78));
        test(fmt, randomDouble(1e-78));
        // #else we're using something like the old z/OS floating point.
#   endif
#else
        // This is a normal machine that can support IEEE754 denormalized doubles without throwing an error.
        test(fmt, randomDouble(DBL_MIN)); /* Usually 2.2250738585072014e-308 */
        test(fmt, randomDouble(1e-100));
#endif
    }
}

void 
NumberFormatRoundTripTest::test(NumberFormat *fmt, double value)
{
    test(fmt, Formattable(value));
}

void 
NumberFormatRoundTripTest::test(NumberFormat *fmt, int32_t value)
{
    test(fmt, Formattable(value));
}

void 
NumberFormatRoundTripTest::test(NumberFormat *fmt, const Formattable& value)
{
    fmt->setMaximumFractionDigits(999);
    DecimalFormat *df = dynamic_cast<DecimalFormat *>(fmt);
    if(df != NULL) {
        df->setRoundingIncrement(0.0);
    }
    UErrorCode status = U_ZERO_ERROR;
    UnicodeString s, s2, temp;
    if(isDouble(value))
        s = fmt->format(value.getDouble(), s);
    else
        s = fmt->format(value.getLong(), s);

    Formattable n;
    UBool show = verbose;
    if(DEBUG)
        logln(/*value.getString(temp) +*/ " F> " + escape(s));

    fmt->parse(s, n, status);
    failure(status, "fmt->parse");
    if(DEBUG) 
        logln(escape(s) + " P> " /*+ n.getString(temp)*/);

    if(isDouble(n))
        s2 = fmt->format(n.getDouble(), s2);
    else
        s2 = fmt->format(n.getLong(), s2);
    
    if(DEBUG) 
        logln(/*n.getString(temp) +*/ " F> " + escape(s2));

    if(STRING_COMPARE) {
        if (s != s2) {
            errln("*** STRING ERROR \"" + escape(s) + "\" != \"" + escape(s2) + "\"");
            show = TRUE;
        }
    }

    if(EXACT_NUMERIC_COMPARE) {
        if(value != n) {
            errln("*** NUMERIC ERROR");
            show = TRUE;
        }
    }
    else {
        // Compute proportional error
        double error = proportionalError(value, n);

        if(error > MAX_ERROR) {
            errln(UnicodeString("*** NUMERIC ERROR ") + error);
            show = TRUE;
        }

        if (error > max_numeric_error) 
            max_numeric_error = error;
        if (error < min_numeric_error) 
            min_numeric_error = error;
    }

    if (show) {
        errln(/*value.getString(temp) +*/ typeOf(value, temp) + " F> " +
            escape(s) + " P> " + (n.getType() == Formattable::kDouble ? n.getDouble() : (double)n.getLong())
            /*n.getString(temp) */ + typeOf(n, temp) + " F> " +
            escape(s2));
    }
}

double 
NumberFormatRoundTripTest::proportionalError(const Formattable& a, const Formattable& b)
{
    double aa,bb;
    
    if(isDouble(a))
        aa = a.getDouble();
    else
        aa = a.getLong();

    if(isDouble(b))
        bb = b.getDouble();
    else
        bb = b.getLong();

    double error = aa - bb;
    if(aa != 0 && bb != 0) 
        error /= aa;
    
    return uprv_fabs(error);
}

UnicodeString& 
NumberFormatRoundTripTest::typeOf(const Formattable& n, UnicodeString& result)
{
    if(n.getType() == Formattable::kLong) {
        result = UnicodeString(" Long");
    }
    else if(n.getType() == Formattable::kDouble) {
        result = UnicodeString(" Double");
    }
    else if(n.getType() == Formattable::kString) {
        result = UnicodeString(" UnicodeString");
        UnicodeString temp;
    }

    return result;
}


UnicodeString& 
NumberFormatRoundTripTest::escape(UnicodeString& s)
{
    UnicodeString copy(s);
    s.remove();
    for(int i = 0; i < copy.length(); ++i) {
        UChar c = copy[i];
        if(c < 0x00FF) 
            s += c;
        else {
            s += "+U";
            char temp[16];
            sprintf(temp, "%4X", c);        // might not work
            s += temp;
        }
    }
    return s;
}

#endif /* #if !UCONFIG_NO_FORMATTING */
