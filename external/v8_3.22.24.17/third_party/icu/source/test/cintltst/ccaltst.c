/********************************************************************
 * Copyright (c) 1997-2010, International Business Machines
 * Corporation and others. All Rights Reserved.
 ********************************************************************
 *
 * File CCALTST.C
 *
 * Modification History:
 *        Name                     Description            
 *     Madhu Katragadda               Creation
 ********************************************************************
 */

/* C API AND FUNCTIONALITY TEST FOR CALENDAR (ucol.h)*/

#include "unicode/utypes.h"

#if !UCONFIG_NO_FORMATTING

#include <stdlib.h>
#include <string.h>

#include "unicode/uloc.h"
#include "unicode/ucal.h"
#include "unicode/udat.h"
#include "unicode/ustring.h"
#include "cintltst.h"
#include "ccaltst.h"
#include "cformtst.h"
#include "cstring.h"
#include "ulist.h"

void TestGregorianChange(void);

void addCalTest(TestNode** root);

void addCalTest(TestNode** root)
{

    addTest(root, &TestCalendar, "tsformat/ccaltst/TestCalendar");
    addTest(root, &TestGetSetDateAPI, "tsformat/ccaltst/TestGetSetDateAPI");
    addTest(root, &TestFieldGetSet, "tsformat/ccaltst/TestFieldGetSet");
    addTest(root, &TestAddRollExtensive, "tsformat/ccaltst/TestAddRollExtensive");
    addTest(root, &TestGetLimits, "tsformat/ccaltst/TestGetLimits");
    addTest(root, &TestDOWProgression, "tsformat/ccaltst/TestDOWProgression");
    addTest(root, &TestGMTvsLocal, "tsformat/ccaltst/TestGMTvsLocal");
    addTest(root, &TestGregorianChange, "tsformat/ccaltst/TestGregorianChange");
    addTest(root, &TestGetKeywordValuesForLocale, "tsformat/ccaltst/TestGetKeywordValuesForLocale");
    addTest(root, &TestWeekend, "tsformat/ccaltst/TestWeekend");
}

/* "GMT" */
static const UChar fgGMTID [] = { 0x0047, 0x004d, 0x0054, 0x0000 };

/* "PST" */
static const UChar PST[] = {0x50, 0x53, 0x54, 0x00}; /* "PST" */

static const UChar EUROPE_PARIS[] = {0x45, 0x75, 0x72, 0x6F, 0x70, 0x65, 0x2F, 0x50, 0x61, 0x72, 0x69, 0x73, 0x00}; /* "Europe/Paris" */

static const UChar AMERICA_LOS_ANGELES[] = {0x41, 0x6D, 0x65, 0x72, 0x69, 0x63, 0x61, 0x2F,
    0x4C, 0x6F, 0x73, 0x5F, 0x41, 0x6E, 0x67, 0x65, 0x6C, 0x65, 0x73, 0x00}; /* America/Los_Angeles */

typedef struct {
    const char *    locale;
    UCalendarType   calType;
    const char *    expectedResult;
} UCalGetTypeTest;

static const UCalGetTypeTest ucalGetTypeTests[] = {
    { "en_US",                   UCAL_GREGORIAN, "gregorian" },
    { "ja_JP@calendar=japanese", UCAL_DEFAULT,   "japanese"  },
    { "th_TH",                   UCAL_GREGORIAN, "gregorian" },
    { "th_TH",                   UCAL_DEFAULT,   "buddhist"  },
    { "th-TH-u-ca-gregory",      UCAL_DEFAULT,   "gregorian" },
    { "ja_JP@calendar=japanese", UCAL_GREGORIAN, "gregorian" },
    { "",                        UCAL_GREGORIAN, "gregorian" },
    { NULL,                      UCAL_GREGORIAN, "gregorian" },
    { NULL, 0, NULL } /* terminator */
};    
    
static void TestCalendar()
{
    UCalendar *caldef = 0, *caldef2 = 0, *calfr = 0, *calit = 0, *calfrclone = 0;
    UEnumeration* uenum = NULL;
    int32_t count, count2, i,j;
    UChar tzID[4];
    UChar *tzdname = 0;
    UErrorCode status = U_ZERO_ERROR;
    UDate now;
    UDateFormat *datdef = 0;
    UChar *result = 0;
    int32_t resultlength, resultlengthneeded;
    char tempMsgBuf[256];
    UChar zone1[32], zone2[32];
    const char *tzver = 0;
    UChar canonicalID[64];
    UBool isSystemID = FALSE;
    const UCalGetTypeTest * ucalGetTypeTestPtr;

#ifdef U_USE_UCAL_OBSOLETE_2_8
    /*Testing countAvailableTimeZones*/
    int32_t offset=0;
    log_verbose("\nTesting ucal_countAvailableTZIDs\n");
    count=ucal_countAvailableTZIDs(offset);
    log_verbose("The number of timezone id's present with offset 0 are %d:\n", count);
    if(count < 5) /* Don't hard code an exact == test here! */
        log_err("FAIL: error in the ucal_countAvailableTZIDs - got %d expected at least 5 total\n", count);

    /*Testing getAvailableTZIDs*/
    log_verbose("\nTesting ucal_getAvailableTZIDs");
    for(i=0;i<count;i++){
        ucal_getAvailableTZIDs(offset, i, &status);
        if(U_FAILURE(status)){
            log_err("FAIL: ucal_getAvailableTZIDs returned %s\n", u_errorName(status));
        }
        log_verbose("%s\n", u_austrcpy(tempMsgBuf, ucal_getAvailableTZIDs(offset, i, &status)));
    }
    /*get Illegal TZID where index >= count*/
    ucal_getAvailableTZIDs(offset, i, &status);
    if(status != U_INDEX_OUTOFBOUNDS_ERROR){
        log_err("FAIL:for TZID index >= count Expected INDEX_OUTOFBOUNDS_ERROR Got %s\n", u_errorName(status));
    }
    status=U_ZERO_ERROR;
#endif
    
    /*Test ucal_openTimeZones & ucal_openCountryTimeZones*/
    for (j=0; j<2; ++j) {
        const char* api = (j==0) ? "ucal_openTimeZones()" :
            "ucal_openCountryTimeZones(US)";
        uenum = (j==0) ? ucal_openTimeZones(&status) :
            ucal_openCountryTimeZones("US", &status);
        if (U_FAILURE(status)) {
            log_err("FAIL: %s failed with %s", api,
                    u_errorName(status));
        } else {
            const char* id;
            int32_t len;
            count = uenum_count(uenum, &status);
            log_verbose("%s returned %d timezone id's:\n", api, count);
            if (count < 5) { /* Don't hard code an exact == test here! */
                log_data_err("FAIL: in %s, got %d, expected at least 5 -> %s (Are you missing data?)\n", api, count, u_errorName(status));
            }
            uenum_reset(uenum, &status);    
            if (U_FAILURE(status)){
                log_err("FAIL: uenum_reset for %s returned %s\n",
                        api, u_errorName(status));
            }
            for (i=0; i<count; i++) {
                id = uenum_next(uenum, &len, &status);
                if (U_FAILURE(status)){
                    log_err("FAIL: uenum_next for %s returned %s\n",
                            api, u_errorName(status));
                } else {
                    log_verbose("%s\n", id);
                }
            }
            /* Next one should be NULL */
            id = uenum_next(uenum, &len, &status);
            if (id != NULL) {
                log_err("FAIL: uenum_next for %s returned %s, expected NULL\n",
                        api, id);
            }
        }
        uenum_close(uenum);
    }

    /*Test ucal_getDSTSavings*/
    status = U_ZERO_ERROR;
    i = ucal_getDSTSavings(fgGMTID, &status);
    if (U_FAILURE(status)) {
        log_err("FAIL: ucal_getDSTSavings(GMT) => %s\n",
                u_errorName(status));
    } else if (i != 0) {
        log_data_err("FAIL: ucal_getDSTSavings(GMT) => %d, expect 0 (Are you missing data?)\n", i);
    }
    i = ucal_getDSTSavings(PST, &status);
    if (U_FAILURE(status)) {
        log_err("FAIL: ucal_getDSTSavings(PST) => %s\n",
                u_errorName(status));
    } else if (i != 1*60*60*1000) {
        log_err("FAIL: ucal_getDSTSavings(PST) => %d, expect %d\n", i, 1*60*60*1000);
    }

    /*Test ucal_set/getDefaultTimeZone*/
    status = U_ZERO_ERROR;
    i = ucal_getDefaultTimeZone(zone1, sizeof(zone1)/sizeof(zone1[0]), &status);
    if (U_FAILURE(status)) {
        log_err("FAIL: ucal_getDefaultTimeZone() => %s\n",
                u_errorName(status));
    } else {
        ucal_setDefaultTimeZone(EUROPE_PARIS, &status);
        if (U_FAILURE(status)) {
            log_err("FAIL: ucal_setDefaultTimeZone(Europe/Paris) => %s\n",
                    u_errorName(status));
        } else {
            i = ucal_getDefaultTimeZone(zone2, sizeof(zone2)/sizeof(zone2[0]), &status);
            if (U_FAILURE(status)) {
                log_err("FAIL: ucal_getDefaultTimeZone() => %s\n",
                        u_errorName(status));
            } else {
                if (u_strcmp(zone2, EUROPE_PARIS) != 0) {
                    log_data_err("FAIL: ucal_getDefaultTimeZone() did not return Europe/Paris (Are you missing data?)\n");
                }
            }
        }
        status = U_ZERO_ERROR;
        ucal_setDefaultTimeZone(zone1, &status);
    }
    
    /*Test ucal_getTZDataVersion*/
    status = U_ZERO_ERROR;
    tzver = ucal_getTZDataVersion(&status);
    if (U_FAILURE(status)) {
        log_err_status(status, "FAIL: ucal_getTZDataVersion() => %s\n", u_errorName(status));
    } else if (uprv_strlen(tzver) != 5 /*4 digits + 1 letter*/) {
        log_err("FAIL: Bad version string was returned by ucal_getTZDataVersion\n");
    } else {
        log_verbose("PASS: ucal_getTZDataVersion returned %s\n", tzver);
    }
    
    /*Testing ucal_getCanonicalTimeZoneID*/
    status = U_ZERO_ERROR;
    resultlength = ucal_getCanonicalTimeZoneID(PST, -1,
        canonicalID, sizeof(canonicalID)/sizeof(UChar), &isSystemID, &status);
    if (U_FAILURE(status)) {
        log_err("FAIL: error in ucal_getCanonicalTimeZoneID : %s\n", u_errorName(status));
    } else {
        if (u_strcmp(AMERICA_LOS_ANGELES, canonicalID) != 0) {
            log_data_err("FAIL: ucal_getCanonicalTimeZoneID(%s) returned %s : expected - %s (Are you missing data?)\n",
                PST, canonicalID, AMERICA_LOS_ANGELES);
        }
        if (!isSystemID) {
            log_data_err("FAIL: ucal_getCanonicalTimeZoneID(%s) set %d to isSystemID (Are you missing data?)\n",
                PST, isSystemID);
        }
    }

    /*Testing the  ucal_open() function*/
    status = U_ZERO_ERROR;
    log_verbose("\nTesting the ucal_open()\n");
    u_uastrcpy(tzID, "PST");
    caldef=ucal_open(tzID, u_strlen(tzID), "en_US", UCAL_TRADITIONAL, &status);
    if(U_FAILURE(status)){
        log_data_err("FAIL: error in ucal_open caldef : %s\n - (Are you missing data?)", u_errorName(status));
    }
    
    caldef2=ucal_open(tzID, u_strlen(tzID), "en_US", UCAL_TRADITIONAL, &status);
    if(U_FAILURE(status)){
        log_data_err("FAIL: error in ucal_open caldef : %s - (Are you missing data?)\n", u_errorName(status));
    }
    u_strcpy(tzID, fgGMTID);
    calfr=ucal_open(tzID, u_strlen(tzID), "fr_FR", UCAL_TRADITIONAL, &status);
    if(U_FAILURE(status)){
        log_data_err("FAIL: error in ucal_open calfr : %s - (Are you missing data?)\n", u_errorName(status));
    }
    calit=ucal_open(tzID, u_strlen(tzID), "it_IT", UCAL_TRADITIONAL, &status);
    if(U_FAILURE(status))    {
        log_data_err("FAIL: error in ucal_open calit : %s - (Are you missing data?)\n", u_errorName(status));
    }

    /*Testing the  clone() function*/
    calfrclone = ucal_clone(calfr, &status);
    if(U_FAILURE(status)){
        log_data_err("FAIL: error in ucal_clone calfr : %s - (Are you missing data?)\n", u_errorName(status));
    }
    
    /*Testing udat_getAvailable() and udat_countAvailable()*/ 
    log_verbose("\nTesting getAvailableLocales and countAvailable()\n");
    count=ucal_countAvailable();
    /* use something sensible w/o hardcoding the count */
    if(count > 0) {
        log_verbose("PASS: ucal_countAvailable() works fine\n");
        log_verbose("The no: of locales for which calendars are avilable are %d\n", count);
    } else {
        log_data_err("FAIL: Error in countAvailable()\n");
    }

    for(i=0;i<count;i++) {
       log_verbose("%s\n", ucal_getAvailable(i)); 
    }
    

    /*Testing the equality between calendar's*/
    log_verbose("\nTesting ucal_equivalentTo()\n");
    if(caldef && caldef2 && calfr && calit) { 
      if(ucal_equivalentTo(caldef, caldef2) == FALSE || ucal_equivalentTo(caldef, calfr)== TRUE || 
        ucal_equivalentTo(caldef, calit)== TRUE || ucal_equivalentTo(calfr, calfrclone) == FALSE) {
          log_data_err("FAIL: Error. equivalentTo test failed (Are you missing data?)\n");
      } else {
          log_verbose("PASS: equivalentTo test passed\n");
      }
    }
    

    /*Testing the current time and date using ucal_getnow()*/
    log_verbose("\nTesting the ucal_getNow function to check if it is fetching tbe current time\n");
    now=ucal_getNow();
    /* open the date format and format the date to check the output */
    datdef=udat_open(UDAT_FULL,UDAT_FULL ,NULL, NULL, 0,NULL,0,&status);
    if(U_FAILURE(status)){
        log_data_err("FAIL: error in creating the dateformat : %s (Are you missing data?)\n", u_errorName(status));
        return;
    }
    log_verbose("PASS: The current date and time fetched is %s\n", u_austrcpy(tempMsgBuf, myDateFormat(datdef, now)) );
    
    
    /*Testing the TimeZoneDisplayName */
    log_verbose("\nTesting the fetching of time zone display name\n");
    /*for the US locale */
    resultlength=0;
    resultlengthneeded=ucal_getTimeZoneDisplayName(caldef, UCAL_DST, "en_US", NULL, resultlength, &status);
    
    if(status==U_BUFFER_OVERFLOW_ERROR)
    {
        status=U_ZERO_ERROR;
        resultlength=resultlengthneeded+1;
        result=(UChar*)malloc(sizeof(UChar) * resultlength);
        ucal_getTimeZoneDisplayName(caldef, UCAL_DST, "en_US", result, resultlength, &status);
    }
    if(U_FAILURE(status))    {
        log_err("FAIL: Error in getting the timezone display name : %s\n", u_errorName(status));
    }
    else{    
        log_verbose("PASS: getting the time zone display name successful : %s, %d needed \n",
            u_errorName(status), resultlengthneeded);
    }
    

#define expectPDT "Pacific Daylight Time"

    tzdname=(UChar*)malloc(sizeof(UChar) * (sizeof(expectPDT)+1));
    u_uastrcpy(tzdname, expectPDT);
    if(u_strcmp(tzdname, result)==0){
        log_verbose("PASS: got the correct time zone display name %s\n", u_austrcpy(tempMsgBuf, result) );
    }
    else{
        log_err("FAIL: got the wrong time zone(DST) display name %s, wanted %s\n", austrdup(result) , expectPDT);
    }
    
    ucal_getTimeZoneDisplayName(caldef, UCAL_SHORT_DST, "en_US", result, resultlength, &status);
    u_uastrcpy(tzdname, "PDT");
    if(u_strcmp(tzdname, result) != 0){
        log_err("FAIL: got the wrong time zone(SHORT_DST) display name %s, wanted %s\n", austrdup(result), austrdup(tzdname));
    }

    ucal_getTimeZoneDisplayName(caldef, UCAL_STANDARD, "en_US", result, resultlength, &status);
    u_uastrcpy(tzdname, "Pacific Standard Time");
    if(u_strcmp(tzdname, result) != 0){
        log_err("FAIL: got the wrong time zone(STANDARD) display name %s, wanted %s\n", austrdup(result), austrdup(tzdname));
    }

    ucal_getTimeZoneDisplayName(caldef, UCAL_SHORT_STANDARD, "en_US", result, resultlength, &status);
    u_uastrcpy(tzdname, "PST");
    if(u_strcmp(tzdname, result) != 0){
        log_err("FAIL: got the wrong time zone(SHORT_STANDARD) display name %s, wanted %s\n", austrdup(result), austrdup(tzdname));
    }
    
    
    /*testing the setAttributes and getAttributes of a UCalendar*/
    log_verbose("\nTesting the getAttributes and set Attributes\n");
    count=ucal_getAttribute(calit, UCAL_LENIENT);
    count2=ucal_getAttribute(calfr, UCAL_LENIENT);
    ucal_setAttribute(calit, UCAL_LENIENT, 0);
    ucal_setAttribute(caldef, UCAL_LENIENT, count2);
    if( ucal_getAttribute(calit, UCAL_LENIENT) !=0 || 
        ucal_getAttribute(calfr, UCAL_LENIENT)!=ucal_getAttribute(caldef, UCAL_LENIENT) )
        log_err("FAIL: there is an error in getAttributes or setAttributes\n");
    else
        log_verbose("PASS: attribute set and got successfully\n");
        /*set it back to orginal value */
    log_verbose("Setting it back to normal\n");
    ucal_setAttribute(calit, UCAL_LENIENT, count);
    if(ucal_getAttribute(calit, UCAL_LENIENT)!=count)
        log_err("FAIL: Error in setting the attribute back to normal\n");
    
    /*setting the first day of the week to other values  */
    count=ucal_getAttribute(calit, UCAL_FIRST_DAY_OF_WEEK);
    for (i=1; i<=7; ++i) {
        ucal_setAttribute(calit, UCAL_FIRST_DAY_OF_WEEK,i);
        if (ucal_getAttribute(calit, UCAL_FIRST_DAY_OF_WEEK) != i) 
            log_err("FAIL: set/getFirstDayOfWeek failed\n");
    }
    /*get bogus Attribute*/
    count=ucal_getAttribute(calit, (UCalendarAttribute)99); /* BOGUS_ATTRIBUTE */
    if(count != -1){
        log_err("FAIL: get/bogus attribute should return -1\n");
    }

    /*set it back to normal */
    ucal_setAttribute(calit, UCAL_FIRST_DAY_OF_WEEK,count);
    /*setting minimal days of the week to other values */
    count=ucal_getAttribute(calit, UCAL_MINIMAL_DAYS_IN_FIRST_WEEK);
    for (i=1; i<=7; ++i) {
        ucal_setAttribute(calit, UCAL_MINIMAL_DAYS_IN_FIRST_WEEK,i);
        if (ucal_getAttribute(calit, UCAL_MINIMAL_DAYS_IN_FIRST_WEEK) != i) 
            log_err("FAIL: set/getMinimalDaysInFirstWeek failed\n");
    }
    /*set it back to normal */
    ucal_setAttribute(calit, UCAL_MINIMAL_DAYS_IN_FIRST_WEEK,count);
    
    
    /*testing if the UCalendar's timezone is currently in day light saving's time*/
    log_verbose("\nTesting if the UCalendar is currently in daylight saving's time\n");
    ucal_setDateTime(caldef, 1999, UCAL_MARCH, 3, 10, 45, 20, &status); 
    ucal_inDaylightTime(caldef, &status );
    if(U_FAILURE(status))    {
        log_err("Error in ucal_inDaylightTime: %s\n", u_errorName(status));
    }
    if(!ucal_inDaylightTime(caldef, &status))
        log_verbose("PASS: It is not in daylight saving's time\n");
    else
        log_err("FAIL: It is not in daylight saving's time\n");

    /*closing the UCalendar*/
    ucal_close(caldef);
    ucal_close(caldef2);
    ucal_close(calfr);
    ucal_close(calit);
    ucal_close(calfrclone);
    
    /*testing ucal_getType, and ucal_open with UCAL_GREGORIAN*/
    for (ucalGetTypeTestPtr = ucalGetTypeTests; ucalGetTypeTestPtr->expectedResult != NULL; ++ucalGetTypeTestPtr) {
        const char * localeToDisplay = (ucalGetTypeTestPtr->locale != NULL)? ucalGetTypeTestPtr->locale: "<NULL>";
        status = U_ZERO_ERROR;
        caldef = ucal_open(NULL, 0, ucalGetTypeTestPtr->locale, ucalGetTypeTestPtr->calType, &status);
        if ( U_SUCCESS(status) ) {
            const char * calType = ucal_getType(caldef, &status);
            if ( U_SUCCESS(status) && calType != NULL ) {
                if ( strcmp( calType, ucalGetTypeTestPtr->expectedResult ) != 0 ) {
                    log_err("FAIL: ucal_open %s type %d does not return %s calendar\n", localeToDisplay,
                                                ucalGetTypeTestPtr->calType, ucalGetTypeTestPtr->expectedResult);
                }
            } else {
                log_err("FAIL: ucal_open %s type %d, then ucal_getType fails\n", localeToDisplay, ucalGetTypeTestPtr->calType);
            }
            ucal_close(caldef);
        } else {
            log_err("FAIL: ucal_open %s type %d fails\n", localeToDisplay, ucalGetTypeTestPtr->calType);
        }
    }

    /*closing the UDateFormat used */
    udat_close(datdef);
    free(result);
    free(tzdname);
}

/*------------------------------------------------------*/
/*Testing the getMillis, setMillis, setDate and setDateTime functions extensively*/

static void TestGetSetDateAPI()
{
    UCalendar *caldef = 0, *caldef2 = 0;
    UChar tzID[4];
    UDate d1;
    int32_t hour;
    int32_t zoneOffset;
    UDateFormat *datdef = 0;
    UErrorCode status=U_ZERO_ERROR;
    UDate d2= 837039928046.0;
    UChar temp[30];

    log_verbose("\nOpening the calendars()\n");
    u_strcpy(tzID, fgGMTID);
    /*open the calendars used */
    caldef=ucal_open(tzID, u_strlen(tzID), "en_US", UCAL_TRADITIONAL, &status);
    caldef2=ucal_open(tzID, u_strlen(tzID), "en_US", UCAL_TRADITIONAL, &status);
    /*open the dateformat */
    /* this is supposed to open default date format, but later on it treats it like it is "en_US" 
       - very bad if you try to run the tests on machine where default locale is NOT "en_US" */
    /*datdef=udat_open(UDAT_DEFAULT,UDAT_DEFAULT ,NULL,fgGMTID,-1, &status);*/
    datdef=udat_open(UDAT_DEFAULT,UDAT_DEFAULT ,"en_US",fgGMTID,-1,NULL,0, &status);
    if(U_FAILURE(status))
    {
        log_data_err("error in creating the dateformat : %s (Are you missing data?)\n", u_errorName(status));
        return;
    }
    

    /*Testing getMillis and setMillis */
    log_verbose("\nTesting the date and time fetched in millis for a calendar using getMillis\n");
    d1=ucal_getMillis(caldef, &status);
    if(U_FAILURE(status)){
        log_err("Error in getMillis : %s\n", u_errorName(status));
    }
    
    /*testing setMillis */
    log_verbose("\nTesting the set date and time function using setMillis\n");
    ucal_setMillis(caldef, d2, &status);
    if(U_FAILURE(status)){
        log_err("Error in setMillis : %s\n", u_errorName(status));
    }

    /*testing if the calendar date is set properly or not  */
    d1=ucal_getMillis(caldef, &status);
    if(u_strcmp(myDateFormat(datdef, d1), myDateFormat(datdef, d2))!=0)
        log_err("error in setMillis or getMillis\n");
    /*-------------------*/
    
    
    
    ctest_setTimeZone(NULL, &status);

    /*testing ucal_setTimeZone() function*/
    log_verbose("\nTesting if the function ucal_setTimeZone() works fine\n");
    ucal_setMillis(caldef2, d2, &status); 
    if(U_FAILURE(status)){
        log_err("Error in getMillis : %s\n", u_errorName(status));;
    }
    hour=ucal_get(caldef2, UCAL_HOUR_OF_DAY, &status);
        
    u_uastrcpy(tzID, "PST");
    ucal_setTimeZone(caldef2,tzID, 3, &status);
    if(U_FAILURE(status)){
        log_err("Error in setting the time zone using ucal_setTimeZone(): %s\n", u_errorName(status));
    }
    else
        log_verbose("ucal_setTimeZone worked fine\n");
    if(hour == ucal_get(caldef2, UCAL_HOUR_OF_DAY, &status))
        log_err("FAIL: Error setting the time zone doesn't change the represented time\n");
    else if((hour-8 + 1) != ucal_get(caldef2, UCAL_HOUR_OF_DAY, &status)) /*because it is not in daylight savings time */
        log_err("FAIL: Error setTimeZone doesn't change the represented time correctly with 8 hour offset\n");
    else
        log_verbose("PASS: setTimeZone works fine\n");
        
    /*testing setTimeZone roundtrip */
    log_verbose("\nTesting setTimeZone() roundtrip\n");
    u_strcpy(tzID, fgGMTID);
    ucal_setTimeZone(caldef2, tzID, 3, &status);
    if(U_FAILURE(status)){
        log_err("Error in setting the time zone using ucal_setTimeZone(): %s\n", u_errorName(status));
    }
    if(d2==ucal_getMillis(caldef2, &status))
        log_verbose("PASS: setTimeZone roundtrip test passed\n");
    else
        log_err("FAIL: setTimeZone roundtrip test failed\n");

    zoneOffset = ucal_get(caldef2, UCAL_ZONE_OFFSET, &status);
    if(U_FAILURE(status)){
        log_err("Error in getting the time zone using ucal_get() after using ucal_setTimeZone(): %s\n", u_errorName(status));
    }
    else if (zoneOffset != 0) {
        log_err("Error in getting the time zone using ucal_get() after using ucal_setTimeZone() offset=%d\n", zoneOffset);
    }

    ucal_setTimeZone(caldef2, NULL, -1, &status);
    if(U_FAILURE(status)){
        log_err("Error in setting the time zone using ucal_setTimeZone(): %s\n", u_errorName(status));
    }
    if(ucal_getMillis(caldef2, &status))
        log_verbose("PASS: setTimeZone roundtrip test passed\n");
    else
        log_err("FAIL: setTimeZone roundtrip test failed\n");

    zoneOffset = ucal_get(caldef2, UCAL_ZONE_OFFSET, &status);
    if(U_FAILURE(status)){
        log_err("Error in getting the time zone using ucal_get() after using ucal_setTimeZone(): %s\n", u_errorName(status));
    }
    else if (zoneOffset != -28800000) {
        log_err("Error in getting the time zone using ucal_get() after using ucal_setTimeZone() offset=%d\n", zoneOffset);
    }

    ctest_resetTimeZone();

/*----------------------------*     */
    
    

    /*Testing  if setDate works fine  */
    log_verbose("\nTesting the ucal_setDate() function \n");
    u_uastrcpy(temp, "Dec 17, 1971 11:05:28 PM");
    ucal_setDate(caldef,1971, UCAL_DECEMBER, 17, &status);
    if(U_FAILURE(status)){
        log_err("error in setting the calendar date : %s\n", u_errorName(status));
    }
    /*checking if the calendar date is set properly or not  */
    d1=ucal_getMillis(caldef, &status);
    if(u_strcmp(myDateFormat(datdef, d1), temp)==0)
        log_verbose("PASS:setDate works fine\n");
    else
        log_err("FAIL:Error in setDate()\n");

    
    /* Testing setDate Extensively with various input values */
    log_verbose("\nTesting ucal_setDate() extensively\n");
    ucal_setDate(caldef, 1999, UCAL_JANUARY, 10, &status);
    verify1("1999  10th day of January  is :", caldef, datdef, 1999, UCAL_JANUARY, 10);
    ucal_setDate(caldef, 1999, UCAL_DECEMBER, 3, &status);
    verify1("1999 3rd day of December  is :", caldef, datdef, 1999, UCAL_DECEMBER, 3);
    ucal_setDate(caldef, 2000, UCAL_MAY, 3, &status);
    verify1("2000 3rd day of May is :", caldef, datdef, 2000, UCAL_MAY, 3);
    ucal_setDate(caldef, 1999, UCAL_AUGUST, 32, &status);
    verify1("1999 32th day of August is :", caldef, datdef, 1999, UCAL_SEPTEMBER, 1);
    ucal_setDate(caldef, 1999, UCAL_MARCH, 0, &status); 
    verify1("1999 0th day of March is :", caldef, datdef, 1999, UCAL_FEBRUARY, 28);
    ucal_setDate(caldef, 0, UCAL_MARCH, 12, &status);
    
    /*--------------------*/

    /*Testing if setDateTime works fine */
    log_verbose("\nTesting the ucal_setDateTime() function \n");
    u_uastrcpy(temp, "May 3, 1972 4:30:42 PM");
    ucal_setDateTime(caldef,1972, UCAL_MAY, 3, 16, 30, 42, &status);
    if(U_FAILURE(status)){
        log_err("error in setting the calendar date : %s\n", u_errorName(status));
    }
    /*checking  if the calendar date is set properly or not  */
    d1=ucal_getMillis(caldef, &status);
    if(u_strcmp(myDateFormat(datdef, d1), temp)==0)
        log_verbose("PASS: setDateTime works fine\n");
    else
        log_err("FAIL: Error in setDateTime\n");

    
    
    /*Testing setDateTime extensively with various input values*/
    log_verbose("\nTesting ucal_setDateTime() function extensively\n");
    ucal_setDateTime(caldef, 1999, UCAL_OCTOBER, 10, 6, 45, 30, &status);
    verify2("1999  10th day of October  at 6:45:30 is :", caldef, datdef, 1999, UCAL_OCTOBER, 10, 6, 45, 30, 0 );
    ucal_setDateTime(caldef, 1999, UCAL_MARCH, 3, 15, 10, 55, &status);
    verify2("1999 3rd day of March   at 15:10:55 is :", caldef, datdef, 1999, UCAL_MARCH, 3, 3, 10, 55, 1);
    ucal_setDateTime(caldef, 1999, UCAL_MAY, 3, 25, 30, 45, &status);
    verify2("1999 3rd day of May at 25:30:45 is :", caldef, datdef, 1999, UCAL_MAY, 4, 1, 30, 45, 0);
    ucal_setDateTime(caldef, 1999, UCAL_AUGUST, 32, 22, 65, 40, &status);
    verify2("1999 32th day of August at 22:65:40 is :", caldef, datdef, 1999, UCAL_SEPTEMBER, 1, 11, 5, 40,1);
    ucal_setDateTime(caldef, 1999, UCAL_MARCH, 12, 0, 0, 0,&status);
    verify2("1999 12th day of March at 0:0:0 is :", caldef, datdef, 1999, UCAL_MARCH, 12, 0, 0, 0, 0);
    ucal_setDateTime(caldef, 1999, UCAL_MARCH, 12, -10, -10,0, &status);
    verify2("1999 12th day of March is at -10:-10:0 :", caldef, datdef, 1999, UCAL_MARCH, 11, 1, 50, 0, 1);
    
    
    
    /*close caldef and datdef*/
    ucal_close(caldef);
    ucal_close(caldef2);
    udat_close(datdef);
}

/*----------------------------------------------------------- */
/**
 * Confirm the functioning of the calendar field related functions.
 */
static void TestFieldGetSet()
{
    UCalendar *cal = 0;
    UChar tzID[4];
    UDateFormat *datdef = 0;
    UDate d1;
    UErrorCode status=U_ZERO_ERROR;
    log_verbose("\nFetching pointer to UCalendar using the ucal_open()\n");
    u_strcpy(tzID, fgGMTID);
    /*open the calendar used */
    cal=ucal_open(tzID, u_strlen(tzID), "en_US", UCAL_TRADITIONAL, &status);
    if (U_FAILURE(status)) {
        log_data_err("ucal_open failed: %s - (Are you missing data?)\n", u_errorName(status));
        return; 
    }
    datdef=udat_open(UDAT_SHORT,UDAT_SHORT ,NULL,fgGMTID,-1,NULL, 0, &status);
    if(U_FAILURE(status))
    {
        log_data_err("error in creating the dateformat : %s (Are you missing data?)\n", u_errorName(status));
    }
    
    /*Testing ucal_get()*/
    log_verbose("\nTesting the ucal_get() function of Calendar\n");
    ucal_setDateTime(cal, 1999, UCAL_MARCH, 12, 5, 25, 30, &status);
    if(U_FAILURE(status)){
        log_data_err("error in the setDateTime() : %s (Are you missing data?)\n", u_errorName(status));
    }
    if(ucal_get(cal, UCAL_YEAR, &status)!=1999 || ucal_get(cal, UCAL_MONTH, &status)!=2 || 
        ucal_get(cal, UCAL_DATE, &status)!=12 || ucal_get(cal, UCAL_HOUR, &status)!=5)
        log_data_err("error in ucal_get() -> %s (Are you missing data?)\n", u_errorName(status));    
    else if(ucal_get(cal, UCAL_DAY_OF_WEEK_IN_MONTH, &status)!=2 || ucal_get(cal, UCAL_DAY_OF_WEEK, &status)!=6
        || ucal_get(cal, UCAL_WEEK_OF_MONTH, &status)!=2 || ucal_get(cal, UCAL_WEEK_OF_YEAR, &status)!= 10)
        log_err("FAIL: error in ucal_get()\n");
    else
        log_verbose("PASS: ucal_get() works fine\n");

    /*Testing the ucal_set() , ucal_clear() functions of calendar*/
    log_verbose("\nTesting the set, and clear  field functions of calendar\n");
    ucal_setAttribute(cal, UCAL_LENIENT, 0);
    ucal_clear(cal);
    ucal_set(cal, UCAL_YEAR, 1997);
    ucal_set(cal, UCAL_MONTH, UCAL_JUNE);
    ucal_set(cal, UCAL_DATE, 3);
    verify1("1997 third day of June = ", cal, datdef, 1997, UCAL_JUNE, 3);
    ucal_clear(cal);
    ucal_set(cal, UCAL_YEAR, 1997);
    ucal_set(cal, UCAL_DAY_OF_WEEK, UCAL_TUESDAY);
    ucal_set(cal, UCAL_MONTH, UCAL_JUNE);
    ucal_set(cal, UCAL_DAY_OF_WEEK_IN_MONTH, 1);
    verify1("1997 first Tuesday in June = ", cal, datdef, 1997, UCAL_JUNE, 3);
    ucal_clear(cal);
    ucal_set(cal, UCAL_YEAR, 1997);
    ucal_set(cal, UCAL_DAY_OF_WEEK, UCAL_TUESDAY);
    ucal_set(cal, UCAL_MONTH, UCAL_JUNE);
    ucal_set(cal, UCAL_DAY_OF_WEEK_IN_MONTH, - 1);
    verify1("1997 last Tuesday in June = ", cal, datdef,1997,   UCAL_JUNE, 24);
    /*give undesirable input    */
    status = U_ZERO_ERROR;
    ucal_clear(cal);
    ucal_set(cal, UCAL_YEAR, 1997);
    ucal_set(cal, UCAL_DAY_OF_WEEK, UCAL_TUESDAY);
    ucal_set(cal, UCAL_MONTH, UCAL_JUNE);
    ucal_set(cal, UCAL_DAY_OF_WEEK_IN_MONTH, 0);
    d1 = ucal_getMillis(cal, &status);
    if (status != U_ILLEGAL_ARGUMENT_ERROR) { 
        log_err("FAIL: U_ILLEGAL_ARGUMENT_ERROR was not returned for : 1997 zero-th Tuesday in June\n");
    } else {
        log_verbose("PASS: U_ILLEGAL_ARGUMENT_ERROR as expected\n");
    }
    status = U_ZERO_ERROR;
    ucal_clear(cal);
    ucal_set(cal, UCAL_YEAR, 1997);
    ucal_set(cal, UCAL_DAY_OF_WEEK, UCAL_TUESDAY);
    ucal_set(cal, UCAL_MONTH, UCAL_JUNE);
    ucal_set(cal, UCAL_WEEK_OF_MONTH, 1);
    verify1("1997 Tuesday in week 1 of June = ", cal,datdef, 1997, UCAL_JUNE, 3);
    ucal_clear(cal);
    ucal_set(cal, UCAL_YEAR, 1997);
    ucal_set(cal, UCAL_DAY_OF_WEEK, UCAL_TUESDAY);
    ucal_set(cal, UCAL_MONTH, UCAL_JUNE);
    ucal_set(cal, UCAL_WEEK_OF_MONTH, 5);
    verify1("1997 Tuesday in week 5 of June = ", cal,datdef, 1997, UCAL_JULY, 1);
    status = U_ZERO_ERROR;
    ucal_clear(cal);
    ucal_set(cal, UCAL_YEAR, 1997);
    ucal_set(cal, UCAL_DAY_OF_WEEK, UCAL_TUESDAY);
    ucal_set(cal, UCAL_MONTH, UCAL_JUNE);
    ucal_set(cal, UCAL_WEEK_OF_MONTH, 0);
    ucal_setAttribute(cal, UCAL_MINIMAL_DAYS_IN_FIRST_WEEK,1);
    d1 = ucal_getMillis(cal,&status);
    if (status != U_ILLEGAL_ARGUMENT_ERROR){ 
        log_err("FAIL: U_ILLEGAL_ARGUMENT_ERROR was not returned for : 1997 Tuesday zero-th week in June\n");
    } else {
        log_verbose("PASS: U_ILLEGAL_ARGUMENT_ERROR as expected\n");
    }
    status = U_ZERO_ERROR;
    ucal_clear(cal);
    ucal_set(cal, UCAL_YEAR_WOY, 1997);
    ucal_set(cal, UCAL_DAY_OF_WEEK, UCAL_TUESDAY);
    ucal_set(cal, UCAL_WEEK_OF_YEAR, 1);
    verify1("1997 Tuesday in week 1 of year = ", cal, datdef,1996, UCAL_DECEMBER, 31);
    ucal_clear(cal);
    ucal_set(cal, UCAL_YEAR, 1997);
    ucal_set(cal, UCAL_DAY_OF_WEEK, UCAL_TUESDAY);
    ucal_set(cal, UCAL_WEEK_OF_YEAR, 10);
    verify1("1997 Tuesday in week 10 of year = ", cal,datdef, 1997, UCAL_MARCH, 4);
    ucal_clear(cal);
    ucal_set(cal, UCAL_YEAR, 1999);
    ucal_set(cal, UCAL_DAY_OF_YEAR, 1);
    verify1("1999 1st day of the year =", cal, datdef, 1999, UCAL_JANUARY, 1);
    ucal_set(cal, UCAL_MONTH, -3);
    d1 = ucal_getMillis(cal,&status);
    if (status != U_ILLEGAL_ARGUMENT_ERROR){ 
        log_err("FAIL: U_ILLEGAL_ARGUMENT_ERROR was not returned for : 1999 -3th month\n");
    } else {
        log_verbose("PASS: U_ILLEGAL_ARGUMENT_ERROR as expected\n");
    }

    ucal_setAttribute(cal, UCAL_LENIENT, 1);
    
    ucal_set(cal, UCAL_MONTH, -3);
    verify1("1999 -3th month should be", cal, datdef, 1998, UCAL_OCTOBER, 1);


    /*testing isSet and clearField()*/
    if(!ucal_isSet(cal, UCAL_WEEK_OF_YEAR))
        log_err("FAIL: error in isSet\n");
    else
        log_verbose("PASS: isSet working fine\n");
    ucal_clearField(cal, UCAL_WEEK_OF_YEAR);
    if(ucal_isSet(cal, UCAL_WEEK_OF_YEAR))
        log_err("FAIL: there is an error in clearField or isSet\n");
    else
        log_verbose("PASS :clearField working fine\n");

    /*-------------------------------*/

    ucal_close(cal);
    udat_close(datdef);
}
 

 
/* ------------------------------------- */
/**
 * Execute adding and rolling in Calendar extensively,
 */
static void TestAddRollExtensive()
{
    UCalendar *cal = 0;
    int32_t i,limit;
    UChar tzID[4];
    UCalendarDateFields e;
    int32_t y,m,d,hr,min,sec,ms;
    int32_t maxlimit = 40;
    UErrorCode status = U_ZERO_ERROR;
    y = 1997; m = UCAL_FEBRUARY; d = 1; hr = 1; min = 1; sec = 0; ms = 0;
   
    log_verbose("Testing add and roll extensively\n");
    
    u_uastrcpy(tzID, "PST");
    /*open the calendar used */
    cal=ucal_open(tzID, u_strlen(tzID), "en_US", UCAL_GREGORIAN, &status);;
    if (U_FAILURE(status)) {
        log_data_err("ucal_open() failed : %s - (Are you missing data?)\n", u_errorName(status)); 
        return; 
    }

    ucal_set(cal, UCAL_YEAR, y);
    ucal_set(cal, UCAL_MONTH, m);
    ucal_set(cal, UCAL_DATE, d);
    ucal_setAttribute(cal, UCAL_MINIMAL_DAYS_IN_FIRST_WEEK,1);

    /* Confirm that adding to various fields works.*/
    log_verbose("\nTesting to confirm that adding to various fields works with ucal_add()\n");
    checkDate(cal, y, m, d);
    ucal_add(cal,UCAL_YEAR, 1, &status);
    if (U_FAILURE(status)) { log_err("ucal_add failed: %s\n", u_errorName(status)); return; }
    y++;
    checkDate(cal, y, m, d);
    ucal_add(cal,UCAL_MONTH, 12, &status);
    if (U_FAILURE(status)) { log_err("ucal_add failed: %s\n", u_errorName(status) ); return; }
    y+=1;
    checkDate(cal, y, m, d);
    ucal_add(cal,UCAL_DATE, 1, &status);
    if (U_FAILURE(status)) { log_err("ucal_add failed: %s\n", u_errorName(status) ); return; }
    d++;
    checkDate(cal, y, m, d);
    ucal_add(cal,UCAL_DATE, 2, &status);
    if (U_FAILURE(status)) { log_err("ucal_add failed: %s\n", u_errorName(status) ); return; }
    d += 2;
    checkDate(cal, y, m, d);
    ucal_add(cal,UCAL_DATE, 28, &status);
    if (U_FAILURE(status)) { log_err("ucal_add failed: %s\n", u_errorName(status) ); return; }
    ++m;
    checkDate(cal, y, m, d);
    ucal_add(cal, (UCalendarDateFields)-1, 10, &status);
    if(status==U_ILLEGAL_ARGUMENT_ERROR)
        log_verbose("Pass: Illegal argument error as expected\n");
    else{
        log_err("Fail: No, illegal argument error as expected. Got....: %s\n", u_errorName(status));
    }
    status=U_ZERO_ERROR;
    

    /*confirm that applying roll to various fields works fine*/
    log_verbose("\nTesting to confirm that ucal_roll() works\n");
    ucal_roll(cal, UCAL_DATE, -1, &status);
    if (U_FAILURE(status)) { log_err("ucal_roll failed: %s\n", u_errorName(status) ); return; }
    d -=1;
    checkDate(cal, y, m, d);
    ucal_roll(cal, UCAL_MONTH, -2, &status);
    if (U_FAILURE(status)) { log_err("ucal_roll failed: %s\n", u_errorName(status) ); return; }
    m -=2;
    checkDate(cal, y, m, d);
    ucal_roll(cal, UCAL_DATE, 1, &status);
    if (U_FAILURE(status)) { log_err("ucal_roll failed: %s\n", u_errorName(status) ); return; }
    d +=1;
    checkDate(cal, y, m, d);
    ucal_roll(cal, UCAL_MONTH, -12, &status);
    if (U_FAILURE(status)) { log_err("ucal_roll failed: %s\n", u_errorName(status) ); return; }
    checkDate(cal, y, m, d);
    ucal_roll(cal, UCAL_YEAR, -1, &status);
    if (U_FAILURE(status)) { log_err("ucal_roll failed: %s\n", u_errorName(status) ); return; }
    y -=1;
    checkDate(cal, y, m, d);
    ucal_roll(cal, UCAL_DATE, 29, &status);
    if (U_FAILURE(status)) { log_err("ucal_roll failed: %s\n", u_errorName(status) ); return; }
    d = 2;
    checkDate(cal, y, m, d);
    ucal_roll(cal, (UCalendarDateFields)-1, 10, &status);
    if(status==U_ILLEGAL_ARGUMENT_ERROR)
        log_verbose("Pass: illegal arguement error as expected\n");
    else{
        log_err("Fail: no illegal argument error got..: %s\n", u_errorName(status));
        return;
    }
    status=U_ZERO_ERROR;
    ucal_clear(cal);
    ucal_setDateTime(cal, 1999, UCAL_FEBRUARY, 28, 10, 30, 45,  &status);
    if(U_FAILURE(status)){
        log_err("error is setting the datetime: %s\n", u_errorName(status));
    }
    ucal_add(cal, UCAL_MONTH, 1, &status);
    checkDate(cal, 1999, UCAL_MARCH, 28);
    ucal_add(cal, UCAL_MILLISECOND, 1000, &status);
    checkDateTime(cal, 1999, UCAL_MARCH, 28, 10, 30, 46, 0, UCAL_MILLISECOND);

    ucal_close(cal);
/*--------------- */
    status=U_ZERO_ERROR;
    /* Testing add and roll extensively */
    log_verbose("\nTesting the ucal_add() and ucal_roll() functions extensively\n");
    y = 1997; m = UCAL_FEBRUARY; d = 1; hr = 1; min = 1; sec = 0; ms = 0;
    cal=ucal_open(tzID, u_strlen(tzID), "en_US", UCAL_TRADITIONAL, &status);
    if (U_FAILURE(status)) {
        log_err("ucal_open failed: %s\n", u_errorName(status));
        return; 
    }
    ucal_set(cal, UCAL_YEAR, y);
    ucal_set(cal, UCAL_MONTH, m);
    ucal_set(cal, UCAL_DATE, d);
    ucal_set(cal, UCAL_HOUR, hr);
    ucal_set(cal, UCAL_MINUTE, min);
    ucal_set(cal, UCAL_SECOND,sec);
    ucal_set(cal, UCAL_MILLISECOND, ms);
    ucal_setAttribute(cal, UCAL_MINIMAL_DAYS_IN_FIRST_WEEK,1);
    status=U_ZERO_ERROR;

    log_verbose("\nTesting UCalendar add...\n");
    for(e = UCAL_YEAR;e < UCAL_FIELD_COUNT; e=(UCalendarDateFields)((int32_t)e + 1)) {
        limit = maxlimit;
        status = U_ZERO_ERROR;
        for (i = 0; i < limit; i++) {
            ucal_add(cal, e, 1, &status);
            if (U_FAILURE(status)) { limit = i; status = U_ZERO_ERROR; }
        }
        for (i = 0; i < limit; i++) {
            ucal_add(cal, e, -1, &status);
            if (U_FAILURE(status)) { 
                log_err("ucal_add -1 failed: %s\n", u_errorName(status));
                return; 
            }
        }
        checkDateTime(cal, y, m, d, hr, min, sec, ms, e);
    }
    log_verbose("\nTesting calendar ucal_roll()...\n");
    for(e = UCAL_YEAR;e < UCAL_FIELD_COUNT; e=(UCalendarDateFields)((int32_t)e + 1)) {
        limit = maxlimit;
        status = U_ZERO_ERROR;
        for (i = 0; i < limit; i++) {
            ucal_roll(cal, e, 1, &status);
            if (U_FAILURE(status)) {
                limit = i;
                status = U_ZERO_ERROR;
            }
        }
        for (i = 0; i < limit; i++) {
            ucal_roll(cal, e, -1, &status);
            if (U_FAILURE(status)) { 
                log_err("ucal_roll -1 failed: %s\n", u_errorName(status));
                return; 
            }
        }
        checkDateTime(cal, y, m, d, hr, min, sec, ms, e);
    }

    ucal_close(cal);
}

/*------------------------------------------------------ */
/*Testing the Limits for various Fields of Calendar*/
static void TestGetLimits()
{
    UCalendar *cal = 0;
    int32_t min, max, gr_min, le_max, ac_min, ac_max, val;
    UChar tzID[4];
    UErrorCode status = U_ZERO_ERROR;
    
    
    u_uastrcpy(tzID, "PST");
    /*open the calendar used */
    cal=ucal_open(tzID, u_strlen(tzID), "en_US", UCAL_GREGORIAN, &status);;
    if (U_FAILURE(status)) {
        log_data_err("ucal_open() for gregorian calendar failed in TestGetLimits: %s - (Are you missing data?)\n", u_errorName(status));
        return; 
    }
    
    log_verbose("\nTesting the getLimits function for various fields\n");
    
    
    
    ucal_setDate(cal, 1999, UCAL_MARCH, 5, &status); /* Set the date to be March 5, 1999 */
    val = ucal_get(cal, UCAL_DAY_OF_WEEK, &status);
    min = ucal_getLimit(cal, UCAL_DAY_OF_WEEK, UCAL_MINIMUM, &status);
    max = ucal_getLimit(cal, UCAL_DAY_OF_WEEK, UCAL_MAXIMUM, &status);
    if ( (min != UCAL_SUNDAY || max != UCAL_SATURDAY ) && (min > val && val > max)  && (val != UCAL_FRIDAY)){
           log_err("FAIL: Min/max bad\n");
           log_err("FAIL: Day of week %d out of range\n", val);
           log_err("FAIL: FAIL: Day of week should be SUNDAY Got %d\n", val);
    }
    else
        log_verbose("getLimits successful\n");

    val = ucal_get(cal, UCAL_DAY_OF_WEEK_IN_MONTH, &status);
    min = ucal_getLimit(cal, UCAL_DAY_OF_WEEK_IN_MONTH, UCAL_MINIMUM, &status);
    max = ucal_getLimit(cal, UCAL_DAY_OF_WEEK_IN_MONTH, UCAL_MAXIMUM, &status);
    if ( (min != 0 || max != 5 ) && (min > val && val > max)  && (val != 1)){
           log_err("FAIL: Min/max bad\n");
           log_err("FAIL: Day of week in month %d out of range\n", val);
           log_err("FAIL: FAIL: Day of week in month should be SUNDAY Got %d\n", val);
        
    }
    else
        log_verbose("getLimits successful\n");
    
    min=ucal_getLimit(cal, UCAL_MONTH, UCAL_MINIMUM, &status);
    max=ucal_getLimit(cal, UCAL_MONTH, UCAL_MAXIMUM, &status);
    gr_min=ucal_getLimit(cal, UCAL_MONTH, UCAL_GREATEST_MINIMUM, &status);
    le_max=ucal_getLimit(cal, UCAL_MONTH, UCAL_LEAST_MAXIMUM, &status);
    ac_min=ucal_getLimit(cal, UCAL_MONTH, UCAL_ACTUAL_MINIMUM, &status);
    ac_max=ucal_getLimit(cal, UCAL_MONTH, UCAL_ACTUAL_MAXIMUM, &status);
    if(U_FAILURE(status)){
        log_err("Error in getLimits: %s\n", u_errorName(status));
    }
    if(min!=0 || max!=11 || gr_min!=0 || le_max!=11 || ac_min!=0 || ac_max!=11)
        log_err("There is and error in getLimits in fetching the values\n");
    else
        log_verbose("getLimits successful\n");

    ucal_setDateTime(cal, 1999, UCAL_MARCH, 5, 4, 10, 35, &status);
    val=ucal_get(cal, UCAL_HOUR_OF_DAY, &status);
    min=ucal_getLimit(cal, UCAL_HOUR_OF_DAY, UCAL_MINIMUM, &status);
    max=ucal_getLimit(cal, UCAL_HOUR_OF_DAY, UCAL_MAXIMUM, &status);
    gr_min=ucal_getLimit(cal, UCAL_MINUTE, UCAL_GREATEST_MINIMUM, &status);
    le_max=ucal_getLimit(cal, UCAL_MINUTE, UCAL_LEAST_MAXIMUM, &status);
    ac_min=ucal_getLimit(cal, UCAL_MINUTE, UCAL_ACTUAL_MINIMUM, &status);
    ac_max=ucal_getLimit(cal, UCAL_SECOND, UCAL_ACTUAL_MAXIMUM, &status);
    if( (min!=0 || max!= 11 || gr_min!=0 || le_max!=60 || ac_min!=0 || ac_max!=60) &&
        (min>val && val>max) && val!=4){
                
        log_err("FAIL: Min/max bad\n");
        log_err("FAIL: Hour of Day %d out of range\n", val);
        log_err("FAIL: HOUR_OF_DAY should be 4 Got %d\n", val);
    }
    else
        log_verbose("getLimits successful\n");    

    
    /*get BOGUS_LIMIT type*/
    val=ucal_getLimit(cal, UCAL_SECOND, (UCalendarLimitType)99, &status);
    if(val != -1){
        log_err("FAIL: ucal_getLimit() with BOGUS type should return -1\n");
    }
    status=U_ZERO_ERROR;


    ucal_close(cal);
}



/* ------------------------------------- */
 
/**
 * Test that the days of the week progress properly when add is called repeatedly
 * for increments of 24 days.
 */
static void TestDOWProgression()
{
    int32_t initialDOW, DOW, newDOW, expectedDOW;
    UCalendar *cal = 0;
    UDateFormat *datfor = 0;
    UDate date1;
    int32_t delta=24;
    UErrorCode status = U_ZERO_ERROR;
    UChar tzID[4];
    char tempMsgBuf[256];
    u_strcpy(tzID, fgGMTID);
    /*open the calendar used */
    cal=ucal_open(tzID, u_strlen(tzID), "en_US", UCAL_TRADITIONAL, &status);;
    if (U_FAILURE(status)) {
        log_data_err("ucal_open failed: %s - (Are you missing data?)\n", u_errorName(status));
        return; 
    }

    datfor=udat_open(UDAT_MEDIUM,UDAT_MEDIUM ,NULL, fgGMTID,-1,NULL, 0, &status);
    if(U_FAILURE(status)){
        log_data_err("error in creating the dateformat : %s (Are you missing data?)\n", u_errorName(status));
    }
    

    ucal_setDate(cal, 1999, UCAL_JANUARY, 1, &status);

    log_verbose("\nTesting the DOW progression\n");
    
    initialDOW = ucal_get(cal, UCAL_DAY_OF_WEEK, &status);
    if (U_FAILURE(status)) { log_data_err("ucal_get() failed: %s (Are you missing data?)\n", u_errorName(status) ); return; }
    newDOW = initialDOW;
    do {
        DOW = newDOW;
        log_verbose("DOW = %d...\n", DOW);
        date1=ucal_getMillis(cal, &status);
        if(U_FAILURE(status)){ log_err("ucal_getMiilis() failed: %s\n", u_errorName(status)); return;}
        log_verbose("%s\n", u_austrcpy(tempMsgBuf, myDateFormat(datfor, date1)));
        
        ucal_add(cal,UCAL_DAY_OF_WEEK, delta, &status);
        if (U_FAILURE(status)) { log_err("ucal_add() failed: %s\n", u_errorName(status)); return; }
        
        newDOW = ucal_get(cal, UCAL_DAY_OF_WEEK, &status);
        if (U_FAILURE(status)) { log_err("ucal_get() failed: %s\n", u_errorName(status)); return; }
        expectedDOW = 1 + (DOW + delta - 1) % 7;
        date1=ucal_getMillis(cal, &status);
        if(U_FAILURE(status)){ log_err("ucal_getMiilis() failed: %s\n", u_errorName(status)); return;}
        if (newDOW != expectedDOW) {
            log_err("Day of week should be %d instead of %d on %s", expectedDOW, newDOW, 
                u_austrcpy(tempMsgBuf, myDateFormat(datfor, date1)) );    
            return; 
        }
    }
    while (newDOW != initialDOW);
    
    ucal_close(cal);
    udat_close(datfor);
}
 
/* ------------------------------------- */
 
/**
 * Confirm that the offset between local time and GMT behaves as expected.
 */
static void TestGMTvsLocal()
{
    log_verbose("\nTesting the offset between the GMT and local time\n");
    testZones(1999, 1, 1, 12, 0, 0);
    testZones(1999, 4, 16, 18, 30, 0);
    testZones(1998, 12, 17, 19, 0, 0);
}
 
/* ------------------------------------- */
 
static void testZones(int32_t yr, int32_t mo, int32_t dt, int32_t hr, int32_t mn, int32_t sc)
{
    int32_t offset,utc, expected;
    UCalendar *gmtcal = 0, *cal = 0;
    UDate date1;
    double temp;
    UDateFormat *datfor = 0;
    UErrorCode status = U_ZERO_ERROR;
    UChar tzID[4];
    char tempMsgBuf[256];

    u_strcpy(tzID, fgGMTID);
    gmtcal=ucal_open(tzID, 3, "en_US", UCAL_TRADITIONAL, &status);;
    if (U_FAILURE(status)) {
        log_data_err("ucal_open failed: %s - (Are you missing data?)\n", u_errorName(status)); 
        return; 
    }
    u_uastrcpy(tzID, "PST");
    cal = ucal_open(tzID, 3, "en_US", UCAL_TRADITIONAL, &status);
    if (U_FAILURE(status)) {
        log_err("ucal_open failed: %s\n", u_errorName(status));
        return; 
    }
    
    datfor=udat_open(UDAT_MEDIUM,UDAT_MEDIUM ,NULL, fgGMTID,-1,NULL, 0, &status);
    if(U_FAILURE(status)){
        log_data_err("error in creating the dateformat : %s (Are you missing data?)\n", u_errorName(status));
    }
   
    ucal_setDateTime(gmtcal, yr, mo - 1, dt, hr, mn, sc, &status);
    if (U_FAILURE(status)) {
        log_data_err("ucal_setDateTime failed: %s (Are you missing data?)\n", u_errorName(status));
        return; 
    }
    ucal_set(gmtcal, UCAL_MILLISECOND, 0);
    date1 = ucal_getMillis(gmtcal, &status);
    if (U_FAILURE(status)) {
        log_err("ucal_getMillis failed: %s\n", u_errorName(status));
        return;
    }
    log_verbose("date = %s\n", u_austrcpy(tempMsgBuf, myDateFormat(datfor, date1)) );

    
    ucal_setMillis(cal, date1, &status);
    if (U_FAILURE(status)) {
        log_err("ucal_setMillis() failed: %s\n", u_errorName(status));
        return;
    }

    offset = ucal_get(cal, UCAL_ZONE_OFFSET, &status);
    offset += ucal_get(cal, UCAL_DST_OFFSET, &status);
   
    if (U_FAILURE(status)) {
        log_err("ucal_get() failed: %s\n", u_errorName(status));
        return;
    }
    temp=(double)((double)offset / 1000.0 / 60.0 / 60.0);
    /*printf("offset for %s %f hr\n", austrdup(myDateFormat(datfor, date1)), temp);*/
       
    utc = ((ucal_get(cal, UCAL_HOUR_OF_DAY, &status) * 60 +
                    ucal_get(cal, UCAL_MINUTE, &status)) * 60 +
                   ucal_get(cal, UCAL_SECOND, &status)) * 1000 +
                    ucal_get(cal, UCAL_MILLISECOND, &status) - offset;
    if (U_FAILURE(status)) {
        log_err("ucal_get() failed: %s\n", u_errorName(status));
        return;
    }
    
    expected = ((hr * 60 + mn) * 60 + sc) * 1000;
    if (utc != expected) {
        temp=(double)(utc - expected)/ 1000 / 60 / 60.0;
        log_err("FAIL: Discrepancy of %d  millis = %fhr\n", utc-expected, temp );
    }
    else
        log_verbose("PASS: the offset between local and GMT is correct\n");
    ucal_close(gmtcal);
    ucal_close(cal);
    udat_close(datfor);
}
 
/* ------------------------------------- */




/* INTERNAL FUNCTIONS USED */
/*------------------------------------------------------------------------------------------- */
 
/* ------------------------------------- */
static void checkDateTime(UCalendar* c, 
                        int32_t y, int32_t m, int32_t d, 
                        int32_t hr, int32_t min, int32_t sec, 
                        int32_t ms, UCalendarDateFields field)

{
    UErrorCode status = U_ZERO_ERROR;
    if (ucal_get(c, UCAL_YEAR, &status) != y ||
        ucal_get(c, UCAL_MONTH, &status) != m ||
        ucal_get(c, UCAL_DATE, &status) != d ||
        ucal_get(c, UCAL_HOUR, &status) != hr ||
        ucal_get(c, UCAL_MINUTE, &status) != min ||
        ucal_get(c, UCAL_SECOND, &status) != sec ||
        ucal_get(c, UCAL_MILLISECOND, &status) != ms) {
        log_err("U_FAILURE for field  %d, Expected y/m/d h:m:s:ms of %d/%d/%d %d:%d:%d:%d  got %d/%d/%d %d:%d:%d:%d\n",
            (int32_t)field, y, m + 1, d, hr, min, sec, ms, 
                    ucal_get(c, UCAL_YEAR, &status),
                    ucal_get(c, UCAL_MONTH, &status) + 1,
                    ucal_get(c, UCAL_DATE, &status),
                    ucal_get(c, UCAL_HOUR, &status),
                    ucal_get(c, UCAL_MINUTE, &status) + 1,
                    ucal_get(c, UCAL_SECOND, &status),
                    ucal_get(c, UCAL_MILLISECOND, &status) );

                    if (U_FAILURE(status)){
                    log_err("ucal_get failed: %s\n", u_errorName(status)); 
                    return; 
                    }
    
     }
    else 
        log_verbose("Confirmed: %d/%d/%d %d:%d:%d:%d\n", y, m + 1, d, hr, min, sec, ms);
        
}
 
/* ------------------------------------- */
static void checkDate(UCalendar* c, int32_t y, int32_t m, int32_t d)
{
    UErrorCode status = U_ZERO_ERROR;
    if (ucal_get(c,UCAL_YEAR, &status) != y ||
        ucal_get(c, UCAL_MONTH, &status) != m ||
        ucal_get(c, UCAL_DATE, &status) != d) {
        
        log_err("FAILURE: Expected y/m/d of %d/%d/%d  got %d/%d/%d\n", y, m + 1, d, 
                    ucal_get(c, UCAL_YEAR, &status),
                    ucal_get(c, UCAL_MONTH, &status) + 1,
                    ucal_get(c, UCAL_DATE, &status) );
        
        if (U_FAILURE(status)) { 
            log_err("ucal_get failed: %s\n", u_errorName(status));
            return; 
        }
    }
    else 
        log_verbose("Confirmed: %d/%d/%d\n", y, m + 1, d);


}

/* ------------------------------------- */
 
/* ------------------------------------- */
 
static void verify1(const char* msg, UCalendar* c, UDateFormat* dat, int32_t year, int32_t month, int32_t day)
{
    UDate d1;
    UErrorCode status = U_ZERO_ERROR;
    if (ucal_get(c, UCAL_YEAR, &status) == year &&
        ucal_get(c, UCAL_MONTH, &status) == month &&
        ucal_get(c, UCAL_DATE, &status) == day) {
        if (U_FAILURE(status)) { 
            log_err("FAIL: Calendar::get failed: %s\n", u_errorName(status));
            return; 
        }
        log_verbose("PASS: %s\n", msg);
        d1=ucal_getMillis(c, &status);
        if (U_FAILURE(status)) { 
            log_err("ucal_getMillis failed: %s\n", u_errorName(status));
            return;
        }
    /*log_verbose(austrdup(myDateFormat(dat, d1)) );*/
    }
    else {
        log_err("FAIL: %s\n", msg);
        d1=ucal_getMillis(c, &status);
        if (U_FAILURE(status)) { 
            log_err("ucal_getMillis failed: %s\n", u_errorName(status) );
            return;
        }
        log_err("Got %s  Expected %d/%d/%d \n", austrdup(myDateFormat(dat, d1)), year, month + 1, day );
        return;
    }

        
}
 
/* ------------------------------------ */
static void verify2(const char* msg, UCalendar* c, UDateFormat* dat, int32_t year, int32_t month, int32_t day, 
                                                                     int32_t hour, int32_t min, int32_t sec, int32_t am_pm)
{
    UDate d1;
    UErrorCode status = U_ZERO_ERROR;
    char tempMsgBuf[256];

    if (ucal_get(c, UCAL_YEAR, &status) == year &&
        ucal_get(c, UCAL_MONTH, &status) == month &&
        ucal_get(c, UCAL_DATE, &status) == day &&
        ucal_get(c, UCAL_HOUR, &status) == hour &&
        ucal_get(c, UCAL_MINUTE, &status) == min &&
        ucal_get(c, UCAL_SECOND, &status) == sec &&
        ucal_get(c, UCAL_AM_PM, &status) == am_pm ){
        if (U_FAILURE(status)) { 
            log_err("FAIL: Calendar::get failed: %s\n", u_errorName(status));
            return; 
        }
        log_verbose("PASS: %s\n", msg); 
        d1=ucal_getMillis(c, &status);
        if (U_FAILURE(status)) { 
            log_err("ucal_getMillis failed: %s\n", u_errorName(status));
            return;
        }
        log_verbose("%s\n" , u_austrcpy(tempMsgBuf, myDateFormat(dat, d1)) );
    }
    else {
        log_err("FAIL: %s\n", msg);
        d1=ucal_getMillis(c, &status);
        if (U_FAILURE(status)) { 
            log_err("ucal_getMillis failed: %s\n", u_errorName(status)); 
            return;
        }
        log_err("Got %s Expected %d/%d/%d/ %d:%d:%d  %s\n", austrdup(myDateFormat(dat, d1)), 
            year, month + 1, day, hour, min, sec, (am_pm==0) ? "AM": "PM");
        
        return;
    }

        
}

void TestGregorianChange() {
    static const UChar utc[] = { 0x45, 0x74, 0x63, 0x2f, 0x47, 0x4d, 0x54, 0 }; /* "Etc/GMT" */
    const int32_t dayMillis = 86400 * INT64_C(1000);    /* 1 day = 86400 seconds */
    UCalendar *cal;
    UDate date;
    UErrorCode errorCode = U_ZERO_ERROR;

    /* Test ucal_setGregorianChange() on a Gregorian calendar. */
    errorCode = U_ZERO_ERROR;
    cal = ucal_open(utc, -1, "", UCAL_GREGORIAN, &errorCode);
    if(U_FAILURE(errorCode)) {
        log_data_err("ucal_open(UTC) failed: %s - (Are you missing data?)\n", u_errorName(errorCode));
        return;
    }
    ucal_setGregorianChange(cal, -365 * (dayMillis * (UDate)1), &errorCode);
    if(U_FAILURE(errorCode)) {
        log_err("ucal_setGregorianChange(1969) failed: %s\n", u_errorName(errorCode));
    } else {
        date = ucal_getGregorianChange(cal, &errorCode);
        if(U_FAILURE(errorCode) || date != -365 * (dayMillis * (UDate)1)) {
            log_err("ucal_getGregorianChange() failed: %s, date = %f\n", u_errorName(errorCode), date);
        }
    }
    ucal_close(cal);

    /* Test ucal_setGregorianChange() on a non-Gregorian calendar where it should fail. */
    errorCode = U_ZERO_ERROR;
    cal = ucal_open(utc, -1, "th@calendar=buddhist", UCAL_TRADITIONAL, &errorCode);
    if(U_FAILURE(errorCode)) {
        log_err("ucal_open(UTC, non-Gregorian) failed: %s\n", u_errorName(errorCode));
        return;
    }
    ucal_setGregorianChange(cal, -730 * (dayMillis * (UDate)1), &errorCode);
    if(errorCode != U_UNSUPPORTED_ERROR) {
        log_err("ucal_setGregorianChange(non-Gregorian calendar) did not yield U_UNSUPPORTED_ERROR but %s\n",
                u_errorName(errorCode));
    }
    errorCode = U_ZERO_ERROR;
    date = ucal_getGregorianChange(cal, &errorCode);
    if(errorCode != U_UNSUPPORTED_ERROR) {
        log_err("ucal_getGregorianChange(non-Gregorian calendar) did not yield U_UNSUPPORTED_ERROR but %s\n",
                u_errorName(errorCode));
    }
    ucal_close(cal);
}

static void TestGetKeywordValuesForLocale() {
#define PREFERRED_SIZE 15
#define MAX_NUMBER_OF_KEYWORDS 4
    const char *PREFERRED[PREFERRED_SIZE][MAX_NUMBER_OF_KEYWORDS+1] = {
            { "root",        "gregorian", NULL, NULL, NULL },
            { "und",         "gregorian", NULL, NULL, NULL },
            { "en_US",       "gregorian", NULL, NULL, NULL },
            { "en_029",      "gregorian", NULL, NULL, NULL },
            { "th_TH",       "buddhist", "gregorian", NULL, NULL },
            { "und_TH",      "buddhist", "gregorian", NULL, NULL },
            { "en_TH",       "buddhist", "gregorian", NULL, NULL },
            { "he_IL",       "gregorian", "hebrew", "islamic", "islamic-civil" },
            { "ar_EG",       "gregorian", "coptic", "islamic", "islamic-civil" },
            { "ja",          "gregorian", "japanese", NULL, NULL },
            { "ps_Guru_IN",  "gregorian", "indian", NULL, NULL },
            { "th@calendar=gregorian", "buddhist", "gregorian", NULL, NULL },
            { "en@calendar=islamic",   "gregorian", NULL, NULL, NULL },
            { "zh_TW",       "gregorian", "roc", "chinese", NULL },
            { "ar_IR",       "gregorian", "persian", "islamic", "islamic-civil" },
    };
    const int32_t EXPECTED_SIZE[PREFERRED_SIZE] = { 1, 1, 1, 1, 2, 2, 2, 4, 4, 2, 2, 2, 1, 3, 4 };
    UErrorCode status = U_ZERO_ERROR;
    int32_t i, size, j;
    UEnumeration *all, *pref;
    const char *loc = NULL;
    UBool matchPref, matchAll;
    const char *value;
    int32_t valueLength;
    UList *ALLList = NULL;
    
    UEnumeration *ALL = ucal_getKeywordValuesForLocale("calendar", uloc_getDefault(), FALSE, &status);
    if (U_SUCCESS(status)) {
        for (i = 0; i < PREFERRED_SIZE; i++) {
            pref = NULL;
            all = NULL;
            loc = PREFERRED[i][0];
            pref = ucal_getKeywordValuesForLocale("calendar", loc, TRUE, &status);
            matchPref = FALSE;
            matchAll = FALSE;
            
            value = NULL;
            valueLength = 0;
            
            if (U_SUCCESS(status) && uenum_count(pref, &status) == EXPECTED_SIZE[i]) {
                matchPref = TRUE;
                for (j = 0; j < EXPECTED_SIZE[i]; j++) {
                    if ((value = uenum_next(pref, &valueLength, &status)) != NULL && U_SUCCESS(status)) {
                        if (uprv_strcmp(value, PREFERRED[i][j+1]) != 0) {
                            matchPref = FALSE;
                            break;
                        }
                    } else {
                        matchPref = FALSE;
                        log_err("ERROR getting keyword value for locale \"%s\"\n", loc);
                        break;
                    }
                }
            }
            
            if (!matchPref) {
                log_err("FAIL: Preferred values for locale \"%s\" does not match expected.\n", loc);
                break;
            }
            uenum_close(pref);
            
            all = ucal_getKeywordValuesForLocale("calendar", loc, FALSE, &status);
            
            size = uenum_count(all, &status);
            
            if (U_SUCCESS(status) && size == uenum_count(ALL, &status)) {
                matchAll = TRUE;
                ALLList = ulist_getListFromEnum(ALL);
                for (j = 0; j < size; j++) {
                    if ((value = uenum_next(all, &valueLength, &status)) != NULL && U_SUCCESS(status)) {
                        if (!ulist_containsString(ALLList, value, uprv_strlen(value))) {
                            log_err("Locale %s have %s not in ALL\n", loc, value);
                            matchAll = FALSE;
                            break;
                        }
                    } else {
                        matchAll = FALSE;
                        log_err("ERROR getting \"all\" keyword value for locale \"%s\"\n", loc);
                        break;
                    }
                }
            }
            if (!matchAll) {
                log_err("FAIL: All values for locale \"%s\" does not match expected.\n", loc);
            }
            
            uenum_close(all);
        }
    } else {
        log_err_status(status, "Failed to get ALL keyword values for default locale %s: %s.\n", uloc_getDefault(), u_errorName(status));
    }
    uenum_close(ALL);
}

/*
 * Weekend tests, ported from
 * icu4j/trunk/main/tests/core/src/com/ibm/icu/dev/test/calendar/IBMCalendarTest.java
 * and extended a bit. Notes below from IBMCalendarTest.java ...
 * This test tests for specific locale data. This is probably okay
 * as far as US data is concerned, but if the Arabic/Yemen data
 * changes, this test will have to be updated.
 */

typedef struct {
    int32_t year;
    int32_t month;
    int32_t day;
    int32_t hour;
    int32_t millisecOffset;
    UBool   isWeekend;
} TestWeekendDates;
typedef struct {
    const char * locale;
    const        TestWeekendDates * dates;
    int32_t      numDates;
} TestWeekendDatesList;

static const TestWeekendDates weekendDates_en_US[] = {
    { 2000, UCAL_MARCH, 17, 23,  0, 0 }, /* Fri 23:00        */
    { 2000, UCAL_MARCH, 18,  0, -1, 0 }, /* Fri 23:59:59.999 */
    { 2000, UCAL_MARCH, 18,  0,  0, 1 }, /* Sat 00:00        */
    { 2000, UCAL_MARCH, 18, 15,  0, 1 }, /* Sat 15:00        */
    { 2000, UCAL_MARCH, 19, 23,  0, 1 }, /* Sun 23:00        */
    { 2000, UCAL_MARCH, 20,  0, -1, 1 }, /* Sun 23:59:59.999 */
    { 2000, UCAL_MARCH, 20,  0,  0, 0 }, /* Mon 00:00        */
    { 2000, UCAL_MARCH, 20,  8,  0, 0 }, /* Mon 08:00        */
};
static const TestWeekendDates weekendDates_ar_YE[] = {
    { 2000, UCAL_MARCH, 15, 23,  0, 0 }, /* Wed 23:00        */
    { 2000, UCAL_MARCH, 16,  0, -1, 0 }, /* Wed 23:59:59.999 */
    { 2000, UCAL_MARCH, 16,  0,  0, 1 }, /* Thu 00:00        */
    { 2000, UCAL_MARCH, 16, 15,  0, 1 }, /* Thu 15:00        */
    { 2000, UCAL_MARCH, 17, 23,  0, 1 }, /* Fri 23:00        */
    { 2000, UCAL_MARCH, 18,  0, -1, 1 }, /* Fri 23:59:59.999 */
    { 2000, UCAL_MARCH, 18,  0,  0, 0 }, /* Sat 00:00        */
    { 2000, UCAL_MARCH, 18,  8,  0, 0 }, /* Sat 08:00        */
};
static const TestWeekendDatesList testDates[] = {
    { "en_US", weekendDates_en_US, sizeof(weekendDates_en_US)/sizeof(weekendDates_en_US[0]) },
    { "ar_YE", weekendDates_ar_YE, sizeof(weekendDates_ar_YE)/sizeof(weekendDates_ar_YE[0]) },
};

typedef struct {
    UCalendarDaysOfWeek  dayOfWeek;
    UCalendarWeekdayType dayType;
    int32_t              transition; /* transition time if dayType is UCAL_WEEKEND_ONSET or UCAL_WEEKEND_CEASE; else must be 0 */
} TestDaysOfWeek;
typedef struct {
    const char *           locale;
    const TestDaysOfWeek * days;
    int32_t                numDays;
} TestDaysOfWeekList;

static const TestDaysOfWeek daysOfWeek_en_US[] = {
    { UCAL_MONDAY,   UCAL_WEEKDAY,       0        },
    { UCAL_FRIDAY,   UCAL_WEEKDAY,       0        },
    { UCAL_SATURDAY, UCAL_WEEKEND,       0        },
    { UCAL_SUNDAY,   UCAL_WEEKEND_CEASE, 86400000 },
};
static const TestDaysOfWeek daysOfWeek_ar_YE[] = { /* Thursday:Friday */
    { UCAL_WEDNESDAY,UCAL_WEEKDAY,       0        },
    { UCAL_SATURDAY, UCAL_WEEKDAY,       0        },
    { UCAL_THURSDAY, UCAL_WEEKEND,       0        },
    { UCAL_FRIDAY,   UCAL_WEEKEND_CEASE, 86400000 },
};
static const TestDaysOfWeekList testDays[] = {
    { "en_US", daysOfWeek_en_US, sizeof(daysOfWeek_en_US)/sizeof(daysOfWeek_en_US[0]) },
    { "ar_YE", daysOfWeek_ar_YE, sizeof(daysOfWeek_ar_YE)/sizeof(daysOfWeek_ar_YE[0]) },
};

static const UChar logDateFormat[] = { 0x0045,0x0045,0x0045,0x0020,0x004D,0x004D,0x004D,0x0020,0x0064,0x0064,0x0020,0x0079,
                                       0x0079,0x0079,0x0079,0x0020,0x0047,0x0020,0x0048,0x0048,0x003A,0x006D,0x006D,0x003A,
                                       0x0073,0x0073,0x002E,0x0053,0x0053,0x0053,0 }; /* "EEE MMM dd yyyy G HH:mm:ss.SSS" */
enum { kFormattedDateMax = 2*sizeof(logDateFormat)/sizeof(logDateFormat[0]) };

static void TestWeekend() {
    const TestWeekendDatesList * testDatesPtr = testDates;
    const TestDaysOfWeekList *   testDaysPtr = testDays;
    int32_t count, subCount;

    UErrorCode fmtStatus = U_ZERO_ERROR;
    UDateFormat * fmt = udat_open(UDAT_NONE, UDAT_NONE, "en", NULL, 0, NULL, 0, &fmtStatus);
    if (U_SUCCESS(fmtStatus)) {
        udat_applyPattern(fmt, FALSE, logDateFormat, -1);
    }
	for (count = sizeof(testDates)/sizeof(testDates[0]); count-- > 0; ++testDatesPtr) {
        UErrorCode status = U_ZERO_ERROR;
		UCalendar * cal = ucal_open(NULL, 0, testDatesPtr->locale, UCAL_GREGORIAN, &status);
		log_verbose("locale: %s\n", testDatesPtr->locale);
		if (U_SUCCESS(status)) {
			const TestWeekendDates * weekendDatesPtr = testDatesPtr->dates;
			for (subCount = testDatesPtr->numDates; subCount--; ++weekendDatesPtr) {
				UDate dateToTest;
				UBool isWeekend;
				char  fmtDateBytes[kFormattedDateMax] = "<could not format test date>"; /* initialize for failure */

				ucal_clear(cal);
				ucal_setDateTime(cal, weekendDatesPtr->year, weekendDatesPtr->month, weekendDatesPtr->day,
								 weekendDatesPtr->hour, 0, 0, &status);
				dateToTest = ucal_getMillis(cal, &status) + weekendDatesPtr->millisecOffset;
				isWeekend = ucal_isWeekend(cal, dateToTest, &status);
				if (U_SUCCESS(fmtStatus)) {
				    UChar fmtDate[kFormattedDateMax];
				    (void)udat_format(fmt, dateToTest, fmtDate, kFormattedDateMax, NULL, &fmtStatus);
				    if (U_SUCCESS(fmtStatus)) {
						u_austrncpy(fmtDateBytes, fmtDate, kFormattedDateMax);
						fmtDateBytes[kFormattedDateMax-1] = 0;
				    } else {
				    	fmtStatus = U_ZERO_ERROR;
				    }
				}
				if ( U_FAILURE(status) ) {
					log_err("FAIL: locale %s date %s isWeekend() status %s\n", testDatesPtr->locale, fmtDateBytes, u_errorName(status) );
					status = U_ZERO_ERROR;
				} else if ( (isWeekend!=0) != (weekendDatesPtr->isWeekend!=0) ) {
					log_err("FAIL: locale %s date %s isWeekend %d, expected the opposite\n", testDatesPtr->locale, fmtDateBytes, isWeekend );
				} else {
					log_verbose("OK:   locale %s date %s isWeekend %d\n", testDatesPtr->locale, fmtDateBytes, isWeekend );
				}
			}
			ucal_close(cal);
		} else {
			log_data_err("FAIL: ucal_open for locale %s failed: %s - (Are you missing data?)\n", testDatesPtr->locale, u_errorName(status) );
		}
	}
    if (U_SUCCESS(fmtStatus)) {
        udat_close(fmt);
    }

    for (count = sizeof(testDays)/sizeof(testDays[0]); count-- > 0; ++testDaysPtr) {
        UErrorCode status = U_ZERO_ERROR;
        UCalendar * cal = ucal_open(NULL, 0, testDaysPtr->locale, UCAL_GREGORIAN, &status);
        log_verbose("locale: %s\n", testDaysPtr->locale);
        if (U_SUCCESS(status)) {
            const TestDaysOfWeek * daysOfWeekPtr = testDaysPtr->days;
            for (subCount = testDaysPtr->numDays; subCount--; ++daysOfWeekPtr) {
                int32_t transition = 0;
                UCalendarWeekdayType dayType = ucal_getDayOfWeekType(cal, daysOfWeekPtr->dayOfWeek, &status);
                if ( dayType == UCAL_WEEKEND_ONSET || dayType == UCAL_WEEKEND_CEASE ) {
                    transition = ucal_getWeekendTransition(cal, daysOfWeekPtr->dayOfWeek, &status); 
                }
                if ( U_FAILURE(status) ) {
					log_err("FAIL: locale %s DOW %d getDayOfWeekType() status %s\n", testDaysPtr->locale, daysOfWeekPtr->dayOfWeek, u_errorName(status) );
					status = U_ZERO_ERROR;
                } else if ( dayType != daysOfWeekPtr->dayType || transition != daysOfWeekPtr->transition ) {
					log_err("FAIL: locale %s DOW %d type %d, expected %d\n", testDaysPtr->locale, daysOfWeekPtr->dayOfWeek, dayType, daysOfWeekPtr->dayType );
                } else {
					log_verbose("OK:   locale %s DOW %d type %d\n", testDaysPtr->locale, daysOfWeekPtr->dayOfWeek, dayType );
                }
            }
            ucal_close(cal);
        } else {
            log_data_err("FAIL: ucal_open for locale %s failed: %s - (Are you missing data?)\n", testDaysPtr->locale, u_errorName(status) );
        }
    }
}

#endif /* #if !UCONFIG_NO_FORMATTING */
