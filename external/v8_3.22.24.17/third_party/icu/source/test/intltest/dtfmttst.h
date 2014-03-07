/********************************************************************
 * COPYRIGHT: 
 * Copyright (c) 1997-2010, International Business Machines Corporation and
 * others. All Rights Reserved.
 ********************************************************************/

#ifndef _DATEFORMATTEST_
#define _DATEFORMATTEST_
 
#include "unicode/utypes.h"

#if !UCONFIG_NO_FORMATTING

#include "unicode/datefmt.h"
#include "unicode/smpdtfmt.h"
#include "caltztst.h"

/** 
 * Performs many different tests for DateFormat and SimpleDateFormat
 **/
class DateFormatTest: public CalendarTimeZoneTest {
    // IntlTest override
    void runIndexedTest( int32_t index, UBool exec, const char* &name, char* par );
public:
    /**
     *  "Test written by Wally Wedel and emailed to me."
     *  Test handling of timezone offsets
     **/
    virtual void TestWallyWedel(void);
    /**
     * Test operator==
     */
    virtual void TestEquals(void);
    /**
     * Test the parsing of 2-digit years.
     */
    virtual void TestTwoDigitYearDSTParse(void);
 
public: // package
    // internal utility routine (genrates escape sequences for characters)
    static UnicodeString& escape(UnicodeString& s);
 
public:
    /**
     * Verify that returned field position indices are correct.
     */
    void TestFieldPosition(void);
 
    void TestGeneral();

public: // package
    // internal utility function
    static void getFieldText(DateFormat* df, int32_t field, UDate date, UnicodeString& str);
 
public:
    /**
     * Verify that strings which contain incomplete specifications are parsed
     * correctly.  In some instances, this means not being parsed at all, and
     * returning an appropriate error.
     */
    virtual void TestPartialParse994(void);
 
public: // package
    // internal test subroutine, used by TestPartialParse994
    virtual void tryPat994(SimpleDateFormat* format, const char* pat, const char* str, UDate expected);
 
public:
    /**
     * Verify the behavior of patterns in which digits for different fields run together
     * without intervening separators.
     */
    virtual void TestRunTogetherPattern985(void);
    /**
     * Verify the behavior of patterns in which digits for different fields run together
     * without intervening separators.
     */
    virtual void TestRunTogetherPattern917(void);
 
public: // package
    // internal test subroutine, used by TestRunTogetherPattern917
    virtual void testIt917(SimpleDateFormat* fmt, UnicodeString& str, UDate expected);
 
public:
    /**
     * Verify the handling of Czech June and July, which have the unique attribute that
     * one is a proper prefix substring of the other.
     */
    virtual void TestCzechMonths459(void);
    /**
     * Test the handling of 'D' in patterns.
     */
    virtual void TestLetterDPattern212(void);
    /**
     * Test the day of year pattern.
     */
    virtual void TestDayOfYearPattern195(void);
 
public: // package
    // interl test subroutine, used by TestDayOfYearPattern195
    virtual void tryPattern(SimpleDateFormat& sdf, UDate d, const char* pattern, UDate expected);
 
public:
    /**
     * Test the handling of single quotes in patterns.
     */
    virtual void TestQuotePattern161(void);
    /**
     * Verify the correct behavior when handling invalid input strings.
     */
    virtual void TestBadInput135(void);
 
public:
    /**
     * Verify the correct behavior when parsing an array of inputs against an
     * array of patterns, with known results.  The results are encoded after
     * the input strings in each row.
     */
    virtual void TestBadInput135a(void);
    /**
     * Test the parsing of two-digit years.
     */
    virtual void TestTwoDigitYear(void);
 
public: // package
    // internal test subroutine, used by TestTwoDigitYear
    virtual void parse2DigitYear(DateFormat& fmt, const char* str, UDate expected);
 
public:
    /**
     * Test the formatting of time zones.
     */
    virtual void TestDateFormatZone061(void);
    /**
     * Further test the formatting of time zones.
     */
    virtual void TestDateFormatZone146(void);

    void TestTimeZoneStringsAPI(void);

    void TestGMTParsing(void);

public: // package
    /**
     * Test the formatting of dates in different locales.
     */
    virtual void TestLocaleDateFormat(void);

    virtual void TestDateFormatCalendar(void);

    virtual void TestSpaceParsing(void);

    void TestExactCountFormat(void);

    void TestWhiteSpaceParsing(void);

    void TestInvalidPattern(void);

    void TestGreekMay(void);

    void TestGenericTime(void);

    void TestGenericTimeZoneOrder(void);

    void Test6338(void);

    void Test6726(void);

    void Test6880(void);

    void TestISOEra(void);

    void TestFormalChineseDate(void);

public:
    /**
     * Test host-specific formatting.
     */
    void TestHost(void);

public:
    /**
     * Test patterns added in CLDR 1.4
     */
    void TestEras(void);

    void TestNarrowNames(void);

    void TestStandAloneDays(void);

    void TestStandAloneMonths(void);

    void TestQuarters(void);
    
    void TestZTimeZoneParsing(void);

    void TestRelativeClone(void);
    
    void TestHostClone(void);

    void TestTimeZoneDisplayName(void);

    void TestRoundtripWithCalendar(void);

public:
    /***
     * Test Relative Dates
     */
     void TestRelative(void);
/*   void TestRelativeError(void);
     void TestRelativeOther(void);
*/

 private:
      void TestRelative(int daysdelta, 
                                  const Locale& loc,
                                  const char *expectChars);

 private:
    void expectParse(const char** data, int32_t data_length,
                     const Locale& locale);

    void expect(const char** data, int32_t data_length,
                const Locale& loc);

    void expectFormat(const char **data, int32_t data_length,
                      const Locale &locale);
};

#endif /* #if !UCONFIG_NO_FORMATTING */
 
#endif // _DATEFORMATTEST_
//eof
