/************************************************************************
 * COPYRIGHT:
 * Copyright (c) 1997-2010, International Business Machines Corporation
 * and others. All Rights Reserved.
 ************************************************************************/

#ifndef _NUMBERFORMATTEST_
#define _NUMBERFORMATTEST_

#include "unicode/utypes.h"

#if !UCONFIG_NO_FORMATTING

#include "unicode/numfmt.h"
#include "unicode/decimfmt.h"
#include "caltztst.h"

/**
 * Performs various in-depth test on NumberFormat
 **/
class NumberFormatTest: public CalendarTimeZoneTest {

    // IntlTest override
    void runIndexedTest( int32_t index, UBool exec, const char* &name, char* par );
 public:

    /**
     * Test APIs (to increase code coverage)
     */
    void TestAPI(void);

    void TestCoverage(void);

    /**
     * Test the handling of quotes
     **/
    void TestQuotes(void);
    /**
     * Test patterns with exponential representation
     **/
    void TestExponential(void);
    /**
     * Test handling of patterns with currency symbols
     **/
    void TestCurrencySign(void);
    /**
     * Test different format patterns
     **/
    void TestPatterns(void);
    /**
     * API coverage for DigitList
     **/
    //void TestDigitList(void);

    /**
     * Test localized currency patterns.
     */
    void TestCurrency(void);

    /**
     * Test the Currency object handling, new as of ICU 2.2.
     */
    void TestCurrencyObject(void);

    void TestCurrencyPatterns(void);

    /**
     * Do rudimentary testing of parsing.
     */
    void TestParse(void);
    /**
     * Test proper rounding by the format method.
     */
    void TestRounding487(void);

    // New tests for alphaWorks upgrade
    void TestExponent(void);

    void TestScientific(void);

    void TestScientific2(void);

    void TestScientificGrouping(void);

    void TestInt64(void);

    void TestSurrogateSupport(void);

    /**
     * Test the functioning of the secondary grouping value.
     */
    void TestSecondaryGrouping(void);

    void TestWhiteSpaceParsing(void);

    void TestComplexCurrency(void);

    void TestPad(void);
    void TestPatterns2(void);

    /**
     * Test currency registration.
     */
    void TestRegCurrency(void);

    void TestCurrencyNames(void);

    void TestCurrencyAmount(void);

    void TestCurrencyUnit(void);

    void TestSymbolsWithBadLocale(void);

    void TestAdoptDecimalFormatSymbols(void);

    void TestPerMill(void);

    void TestIllegalPatterns(void);

    void TestCases(void);

    void TestJB3832(void);

    void TestHost(void);

    void TestHostClone(void);

    void TestCurrencyFormat(void);

    /* Port of ICU4J rounding test. */
    void TestRounding(void);

    void TestNonpositiveMultiplier(void);

    void TestNumberingSystems();


    void TestSpaceParsing();
    void TestMultiCurrencySign();
    void TestCurrencyFormatForMixParsing();
    void TestDecimalFormatCurrencyParse();
    void TestCurrencyIsoPluralFormat();
    void TestCurrencyParsing();
    void TestParseCurrencyInUCurr();
    void TestFormatAttributes();
    void TestFieldPositionIterator();

    void TestDecimal();
    void TestCurrencyFractionDigits();

    void TestExponentParse();

 private:

    static UBool equalValue(const Formattable& a, const Formattable& b);

    void expectPositions(FieldPositionIterator& iter, int32_t *values, int32_t tupleCount,
                         const UnicodeString& str);

    void expectPosition(FieldPosition& pos, int32_t id, int32_t start, int32_t limit,
                        const UnicodeString& str);

    void expect2(NumberFormat& fmt, const Formattable& n, const UnicodeString& str);

    void expect3(NumberFormat& fmt, const Formattable& n, const UnicodeString& str);

    void expect2(NumberFormat& fmt, const Formattable& n, const char* str) {
        expect2(fmt, n, UnicodeString(str, ""));
    }

    void expect2(NumberFormat* fmt, const Formattable& n, const UnicodeString& str, UErrorCode ec);

    void expect2(NumberFormat* fmt, const Formattable& n, const char* str, UErrorCode ec) {
        expect2(fmt, n, UnicodeString(str, ""), ec);
    }

    void expect(NumberFormat& fmt, const UnicodeString& str, const Formattable& n);

    void expect(NumberFormat& fmt, const char *str, const Formattable& n) {
        expect(fmt, UnicodeString(str, ""), n);
    }

    void expect(NumberFormat& fmt, const Formattable& n,
                const UnicodeString& exp, UBool rt=TRUE);

    void expect(NumberFormat& fmt, const Formattable& n,
                const char *exp, UBool rt=TRUE) {
        expect(fmt, n, UnicodeString(exp, ""), rt);
    }

    void expect(NumberFormat* fmt, const Formattable& n,
                const UnicodeString& exp, UErrorCode);

    void expect(NumberFormat* fmt, const Formattable& n,
                const char *exp, UErrorCode errorCode) {
        expect(fmt, n, UnicodeString(exp, ""), errorCode);
    }

    void expectCurrency(NumberFormat& nf, const Locale& locale,
                        double value, const UnicodeString& string);

    void expectPad(DecimalFormat& fmt, const UnicodeString& pat,
                   int32_t pos, int32_t width, UChar pad);

    void expectPad(DecimalFormat& fmt, const char *pat,
                   int32_t pos, int32_t width, UChar pad) {
        expectPad(fmt, UnicodeString(pat, ""), pos, width, pad);
    }

    void expectPad(DecimalFormat& fmt, const UnicodeString& pat,
                   int32_t pos, int32_t width, const UnicodeString& pad);

    void expectPad(DecimalFormat& fmt, const char *pat,
                   int32_t pos, int32_t width, const UnicodeString& pad) {
        expectPad(fmt, UnicodeString(pat, ""), pos, width, pad);
    }

    void expectPat(DecimalFormat& fmt, const UnicodeString& exp);

    void expectPat(DecimalFormat& fmt, const char *exp) {
        expectPat(fmt, UnicodeString(exp, ""));
    }

    void expectPad(DecimalFormat& fmt, const UnicodeString& pat,
                   int32_t pos);

    void expectPad(DecimalFormat& fmt, const char *pat,
                   int32_t pos) {
        expectPad(fmt, pat, pos, 0, (UChar)0);
    }

    void expect_rbnf(NumberFormat& fmt, const UnicodeString& str, const Formattable& n);

    void expect_rbnf(NumberFormat& fmt, const Formattable& n,
                const UnicodeString& exp, UBool rt=TRUE);

    // internal utility routine
    static UnicodeString& escape(UnicodeString& s);

    enum { ILLEGAL = -1 };

    // internal subtest used by TestRounding487
    void roundingTest(NumberFormat& nf, double x, int32_t maxFractionDigits, const char* expected);

    // internal rounding checking for TestRounding
    void checkRounding(DecimalFormat* df, double base, int iterations, double increment);

    double checkRound(DecimalFormat* df, double iValue, double lastParsed);
};

#endif /* #if !UCONFIG_NO_FORMATTING */

#endif // _NUMBERFORMATTEST_
