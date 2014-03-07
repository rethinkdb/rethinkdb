/********************************************************************
 * COPYRIGHT: 
 * Copyright (c) 1997-2007, International Business Machines Corporation and
 * others. All Rights Reserved.
 ********************************************************************/

#ifndef __IntlCalendarTest__
#define __IntlCalendarTest__
 
#include "unicode/utypes.h"

#if !UCONFIG_NO_FORMATTING

#include "unicode/calendar.h"
#include "unicode/smpdtfmt.h"
#include "caltztst.h"

class IntlCalendarTest: public CalendarTimeZoneTest {
public:
    // IntlTest override
    void runIndexedTest( int32_t index, UBool exec, const char* &name, char* par );
public:
    void TestTypes(void);

    void TestGregorian(void);

    void TestBuddhist(void);
    void TestBuddhistFormat(void);

    void TestTaiwan(void);

    void TestJapanese(void);
    void TestJapaneseFormat(void);
    void TestJapanese3860(void);
    
    void TestPersian(void);
    void TestPersianFormat(void);

 protected:
    // Test a Gregorian-Like calendar
    void quasiGregorianTest(Calendar& cal, const Locale& gregoLocale, const int32_t *data);
    void simpleTest(const Locale& loc, const UnicodeString& expect, UDate expectDate, UErrorCode& status);
 
public: // package
    // internal routine for checking date
    static UnicodeString value(Calendar* calendar);
 
};


#endif /* #if !UCONFIG_NO_FORMATTING */
 
#endif // __IntlCalendarTest__
