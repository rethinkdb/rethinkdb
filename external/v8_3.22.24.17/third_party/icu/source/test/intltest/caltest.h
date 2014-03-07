/***********************************************************************
 * Copyright (c) 1997-2009, International Business Machines Corporation
 * and others. All Rights Reserved.
 ***********************************************************************/

#ifndef __CalendarTest__
#define __CalendarTest__
 
#include "unicode/utypes.h"

#if !UCONFIG_NO_FORMATTING

#include "unicode/calendar.h"
#include "unicode/smpdtfmt.h"
#include "caltztst.h"

class CalendarTest: public CalendarTimeZoneTest {
public:
    // IntlTest override
    void runIndexedTest( int32_t index, UBool exec, const char* &name, char* par );
public:
    /**
     * This test confirms the correct behavior of add when incrementing
     * through subsequent days.
     */
    virtual void TestRog(void);
    /**
     * Test the handling of the day of the week, checking for correctness and
     * for correct minimum and maximum values.
     */
    virtual void TestDOW943(void);
    /**
     * test subroutine use by TestDOW943
     */
    void dowTest(UBool lenient);
    /** 
     * Confirm that cloned Calendar objects do not inadvertently share substructures.
     */
    virtual void TestClonesUnique908(void);
    /**
     * Confirm that the Gregorian cutoff value works as advertised.
     */
    virtual void TestGregorianChange768(void);
    /**
     * Confirm the functioning of the field disambiguation algorithm.
     */
    virtual void TestDisambiguation765(void);
    /**
     * Test various API methods for API completeness.
     */
    virtual void TestGenericAPI(void); // New to C++ -- needs to be back ported to Java

    virtual void TestWOY(void);
    
    virtual void TestDebug(void);
 
public: // package
    /**
     * test subroutine used by TestDisambiguation765
     */
    virtual void verify765(const UnicodeString& msg, Calendar* c, int32_t year, int32_t month, int32_t day);
    /**
     * test subroutine used by TestDisambiguation765
     */
    virtual void verify765(const UnicodeString& msg/*, IllegalArgumentException e*/, UErrorCode status);
 
public:
    /**
     * Confirm that the offset between local time and GMT behaves as expected.
     */
    virtual void TestGMTvsLocal4064654(void);
 
public: // package
    /**
     * test subroutine used by TestGMTvsLocal4064654
     */
    virtual void test4064654(int32_t yr, int32_t mo, int32_t dt, int32_t hr, int32_t mn, int32_t sc);
 
public:
    /**
     * The operations of adding and setting should not exhibit pathological
     * dependence on the order of operations.  This test checks for this.
     */
    virtual void TestAddSetOrder621(void);
    /**
     * Confirm that adding to various fields works.
     */
    virtual void TestAdd520(void);
    /**
     * Execute and test adding and rolling in GregorianCalendar extensively.
     */
    virtual void TestAddRollExtensive(void);

public: // package
    // internal utility routine for checking date
    virtual void check520(Calendar* c, 
                            int32_t y, int32_t m, int32_t d, 
                            int32_t hr, int32_t min, int32_t sec, 
                            int32_t ms, UCalendarDateFields field);
 
    virtual void check520(Calendar* c, 
                            int32_t y, int32_t m, int32_t d);

public:
    /**
     * Test that setting of fields works.  In particular, make sure that all instances
     * of GregorianCalendar don't share a static instance of the fields array.
     */
    virtual void TestFieldSet4781(void);
/*    virtual void TestSerialize337();
 
public: // package
    static UnicodeString& PREFIX;
    static UnicodeString& POSTFIX;
    static UnicodeString& FILENAME;
*/ 
public:
    /**
     * Verify that the seconds of a Calendar can be zeroed out through the
     * expected sequence of operations.
     */ 
    virtual void TestSecondsZero121(void);
    /**
     * Verify that a specific sequence of adding and setting works as expected;
     * it should not vary depending on when and whether the get method is
     * called.
     */
    virtual void TestAddSetGet0610(void);
 
public: // package
    // internal routine for checking date
    static UnicodeString value(Calendar* calendar);
 
public:
    /**
     * Verify that various fields on a known date are set correctly.
     */
    virtual void TestFields060(void);
 
public: // package
    static int32_t EXPECTED_FIELDS[];
    static const int32_t EXPECTED_FIELDS_length;
 
public:
    /**
     * Verify that various fields on a known date are set correctly.  In this
     * case, the start of the epoch (January 1 1970).
     */
    virtual void TestEpochStartFields(void);
 
public: // package
    static int32_t EPOCH_FIELDS[];
 
public:
    /**
     * Test that the days of the week progress properly when add is called repeatedly
     * for increments of 24 days.
     */
    virtual void TestDOWProgression(void);
    /**
     * Test newly added fields - DOW_LOCAL and YEAR_WOY
     */
    virtual void TestDOW_LOCALandYEAR_WOY(void);
    // test subroutine used by TestDOW_LOCALandYEAR_WOY
    virtual void doYEAR_WOYLoop(Calendar *cal, 
        SimpleDateFormat *sdf, int32_t times, UErrorCode& status);
    // test subroutine used by TestDOW_LOCALandYEAR_WOY
    virtual void loop_addroll(Calendar *cal, /*SimpleDateFormat *sdf, */
        int times, UCalendarDateFields field, UCalendarDateFields field2, 
        UErrorCode& errorCode);

    void TestYWOY(void);
    void TestJD(void);

    void yearAddTest(Calendar& cal, UErrorCode& status);
 
public: // package
    // test subroutine use by TestDOWProgression
    virtual void marchByDelta(Calendar* cal, int32_t delta);

 public:
    // for other tests' use
    static UnicodeString fieldName(UCalendarDateFields f);
    static UnicodeString calToStr(const Calendar & cal);
    
    // List of non-installed locales with interesting calendars

    /**
     * @return the count of 'other' locales to test
     */
    static int32_t testLocaleCount();

    /**
     * @param i index of 'other' locale to return
     * @return locale ID
     */
    static const char* testLocaleID(int32_t i);

    /**
     * Clone the specified calendar, and determine its earliest supported date
     * by setting the extended year to the minimum value.
     * @param cal Calendar (will be cloned)
     * @param isGregorian output: returns 'TRUE' if the calendar's class is GregorianCalendar
     * @param status error code 
     */
    static UDate minDateOfCalendar(const Calendar& cal, UBool &isGregorian, UErrorCode& status);

    /**
     * Construct a calendar of the specified locale, and determine its earliest supported date
     * by setting the extended year to the minimum value.
     * @param locale locale of calendar to check
     * @param isGregorian output: returns 'TRUE' if the calendar's class is GregorianCalendar
     * @param status error code 
     */
    static UDate minDateOfCalendar(const Locale& locale, UBool &isGregorian, UErrorCode& status);

  // internal - for other test use
 public:
    void Test6703(void);
    void Test3785(void);
    void Test1624(void);
};

#endif /* #if !UCONFIG_NO_FORMATTING */
 
#endif // __CalendarTest__
