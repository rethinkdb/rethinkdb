
/********************************************************************
 * Copyright (c) 1997-2010, International Business Machines
 * Corporation and others. All Rights Reserved.
 ********************************************************************/
 
#ifndef __TimeZoneTest__
#define __TimeZoneTest__

#include "unicode/utypes.h"

#if !UCONFIG_NO_FORMATTING

#include "unicode/simpletz.h" 
#include "caltztst.h"

/** 
 * Various tests for TimeZone
 **/
class TimeZoneTest: public CalendarTimeZoneTest {
    // IntlTest override
    void runIndexedTest( int32_t index, UBool exec, const char* &name, char* par );
public: // package
    static const int32_t millisPerHour;
 
public:
    /**
     * Test the offset of the PRT timezone.
     */
    virtual void TestPRTOffset(void);
    /**
     * Regress a specific bug with a sequence of API calls.
     */
    virtual void TestVariousAPI518(void);
    /**
     * Test the call which retrieves the available IDs.
     */
    virtual void TestGetAvailableIDs913(void);

    /**
     * Generic API testing for API coverage.
     */
    virtual void TestGenericAPI(void);
    /**
     * Test the setStartRule/setEndRule API calls.
     */
    virtual void TestRuleAPI(void);
 
    void findTransition(const TimeZone& tz,
                        UDate min, UDate max);

   /**
     * subtest used by TestRuleAPI
     **/
    void testUsingBinarySearch(const TimeZone& tz,
                               UDate min, UDate max,
                               UDate expectedBoundary);


    /**
     *  Test short zone IDs for compliance
     */ 
    virtual void TestShortZoneIDs(void);


    /**
     *  Test parsing custom zones
     */ 
    virtual void TestCustomParse(void);
    
    /**
     *  Test new getDisplayName() API
     */ 
    virtual void TestDisplayName(void);

    void TestDSTSavings(void);
    void TestAlternateRules(void);

    void TestCountries(void);

    void TestHistorical(void);

    void TestEquivalentIDs(void);

    void TestAliasedNames(void);
    
    void TestFractionalDST(void);

    void TestFebruary(void);

    void TestCanonicalID(void);
    
    virtual void TestDisplayNamesMeta();

    static const UDate INTERVAL;

private:
    // internal functions
    static UnicodeString& formatOffset(int32_t offset, UnicodeString& rv);
    static UnicodeString& formatTZID(int32_t offset, UnicodeString& rv);

    // Some test case data is current date/tzdata version sensitive and producing errors
    // when year/rule are changed.
    static const int32_t REFERENCE_YEAR;
    static const char *REFERENCE_DATA_VERSION;
};

#endif /* #if !UCONFIG_NO_FORMATTING */
 
#endif // __TimeZoneTest__
