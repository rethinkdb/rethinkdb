/************************************************************************
 * COPYRIGHT: 
 * Copyright (c) 1997-2010, International Business Machines Corporation
 * and others. All Rights Reserved.
 ************************************************************************/

#include "unicode/utypes.h"

#if !UCONFIG_NO_FORMATTING

#include "caltest.h"
#include "unicode/dtfmtsym.h"
#include "unicode/gregocal.h"
#include "hebrwcal.h"
#include "unicode/smpdtfmt.h"
#include "unicode/simpletz.h"
#include "dbgutil.h"
#include "unicode/udat.h"
#include "unicode/ustring.h"

#define mkcstr(U) u_austrcpy(calloc(8, u_strlen(U) + 1), U)

// *****************************************************************************
// class CalendarTest
// *****************************************************************************

UnicodeString CalendarTest::calToStr(const Calendar & cal)
{
  UnicodeString out;
  UErrorCode status = U_ZERO_ERROR;
  int i;
  UDate d;
  for(i = 0;i<UCAL_FIELD_COUNT;i++) {
    out += (UnicodeString("") + fieldName((UCalendarDateFields)i) + "=" +  cal.get((UCalendarDateFields)i, status) + UnicodeString(" "));
  }
  out += "[" + UnicodeString(cal.getType()) + "]";
  
  if(cal.inDaylightTime(status)) {
    out += UnicodeString(" (in DST), zone=");
  }
  else {
    out += UnicodeString(", zone=");
  }
  
  UnicodeString str2;
  out += cal.getTimeZone().getDisplayName(str2);
  d = cal.getTime(status);
  out += UnicodeString(" :","") + d;
  
  return out;
}

void CalendarTest::runIndexedTest( int32_t index, UBool exec, const char* &name, char* /*par*/ )
{
    if (exec) logln("TestSuite TestCalendar");
    switch (index) {
        case 0:
            name = "TestDOW943";
            if (exec) {
                logln("TestDOW943---"); logln("");
                TestDOW943();
            }
            break;
        case 1:
            name = "TestClonesUnique908";
            if (exec) {
                logln("TestClonesUnique908---"); logln("");
                TestClonesUnique908();
            }
            break;
        case 2:
            name = "TestGregorianChange768";
            if (exec) {
                logln("TestGregorianChange768---"); logln("");
                TestGregorianChange768();
            }
            break;
        case 3:
            name = "TestDisambiguation765";
            if (exec) {
                logln("TestDisambiguation765---"); logln("");
                TestDisambiguation765();
            }
            break;
        case 4:
            name = "TestGMTvsLocal4064654";
            if (exec) {
                logln("TestGMTvsLocal4064654---"); logln("");
                TestGMTvsLocal4064654();
            }
            break;
        case 5:
            name = "TestAddSetOrder621";
            if (exec) {
                logln("TestAddSetOrder621---"); logln("");
                TestAddSetOrder621();
            }
            break;
        case 6:
            name = "TestAdd520";
            if (exec) {
                logln("TestAdd520---"); logln("");
                TestAdd520();
            }
            break;
        case 7:
            name = "TestFieldSet4781";
            if (exec) {
                logln("TestFieldSet4781---"); logln("");
                TestFieldSet4781();
            }
            break;
        case 8:
            name = "TestSerialize337";
            if (exec) {
                logln("TestSerialize337---"); logln("");
            //  TestSerialize337();
            }
            break;
        case 9:
            name = "TestSecondsZero121";
            if (exec) {
                logln("TestSecondsZero121---"); logln("");
                TestSecondsZero121();
            }
            break;
        case 10:
            name = "TestAddSetGet0610";
            if (exec) {
                logln("TestAddSetGet0610---"); logln("");
                TestAddSetGet0610();
            }
            break;
        case 11:
            name = "TestFields060";
            if (exec) {
                logln("TestFields060---"); logln("");
                TestFields060();
            }
            break;
        case 12:
            name = "TestEpochStartFields";
            if (exec) {
                logln("TestEpochStartFields---"); logln("");
                TestEpochStartFields();
            }
            break;
        case 13:
            name = "TestDOWProgression";
            if (exec) {
                logln("TestDOWProgression---"); logln("");
                TestDOWProgression();
            }
            break;
        case 14:
            name = "TestGenericAPI";
            if (exec) {
                logln("TestGenericAPI---"); logln("");
                TestGenericAPI();
            }
            break;
        case 15:
            name = "TestAddRollExtensive";
            if (exec) {
                logln("TestAddRollExtensive---"); logln("");
                TestAddRollExtensive();
            }
            break;
        case 16:
            name = "TestDOW_LOCALandYEAR_WOY";
            if (exec) {
                logln("TestDOW_LOCALandYEAR_WOY---"); logln("");
                TestDOW_LOCALandYEAR_WOY();
            }
            break;
        case 17:
            name = "TestWOY";
            if (exec) {
                logln("TestWOY---"); logln("");
                TestWOY();
            }
            break;
        case 18:
            name = "TestRog";
            if (exec) {
                logln("TestRog---"); logln("");
                TestRog();
            }
            break;
        case 19:
           name = "TestYWOY";
            if (exec) {
                logln("TestYWOY---"); logln("");
                TestYWOY();
            }
            break;
        case 20:
          name = "TestJD";
          if(exec) {
            logln("TestJD---"); logln("");
            TestJD();
          }
          break;
        case 21:
          name = "TestDebug";
          if(exec) {
            logln("TestDebug---"); logln("");
            TestDebug();
          }
          break;
        case 22:
          name = "Test6703";
          if(exec) {
            logln("Test6703---"); logln("");
            Test6703();
          }
          break;
        case 23:
          name = "Test3785";
          if(exec) {
            logln("Test3785---"); logln("");
            Test3785();
          }
          break;
        case 24:
          name = "Test1624";
          if(exec) {
            logln("Test1624---"); logln("");
            Test1624();
          }
          break;
        default: name = ""; break;
    }
}

// ---------------------------------------------------------------------------------

UnicodeString CalendarTest::fieldName(UCalendarDateFields f) {
    switch (f) {
#define FIELD_NAME_STR(x) case x: return (#x+5)
      FIELD_NAME_STR( UCAL_ERA );
      FIELD_NAME_STR( UCAL_YEAR );
      FIELD_NAME_STR( UCAL_MONTH );
      FIELD_NAME_STR( UCAL_WEEK_OF_YEAR );
      FIELD_NAME_STR( UCAL_WEEK_OF_MONTH );
      FIELD_NAME_STR( UCAL_DATE );
      FIELD_NAME_STR( UCAL_DAY_OF_YEAR );
      FIELD_NAME_STR( UCAL_DAY_OF_WEEK );
      FIELD_NAME_STR( UCAL_DAY_OF_WEEK_IN_MONTH );
      FIELD_NAME_STR( UCAL_AM_PM );
      FIELD_NAME_STR( UCAL_HOUR );
      FIELD_NAME_STR( UCAL_HOUR_OF_DAY );
      FIELD_NAME_STR( UCAL_MINUTE );
      FIELD_NAME_STR( UCAL_SECOND );
      FIELD_NAME_STR( UCAL_MILLISECOND );
      FIELD_NAME_STR( UCAL_ZONE_OFFSET );
      FIELD_NAME_STR( UCAL_DST_OFFSET );
      FIELD_NAME_STR( UCAL_YEAR_WOY );
      FIELD_NAME_STR( UCAL_DOW_LOCAL );
      FIELD_NAME_STR( UCAL_EXTENDED_YEAR );
      FIELD_NAME_STR( UCAL_JULIAN_DAY );
      FIELD_NAME_STR( UCAL_MILLISECONDS_IN_DAY );
#undef FIELD_NAME_STR
    default:
        return UnicodeString("") + ((int32_t)f);
    }
}

/**
 * Test various API methods for API completeness.
 */
void
CalendarTest::TestGenericAPI()
{
    UErrorCode status = U_ZERO_ERROR;
    UDate d;
    UnicodeString str;
    UBool eq = FALSE,b4 = FALSE,af = FALSE;

    UDate when = date(90, UCAL_APRIL, 15);

    UnicodeString tzid("TestZone");
    int32_t tzoffset = 123400;

    SimpleTimeZone *zone = new SimpleTimeZone(tzoffset, tzid);
    Calendar *cal = Calendar::createInstance(zone->clone(), status);
    if (failure(status, "Calendar::createInstance", TRUE)) return;

    if (*zone != cal->getTimeZone()) errln("FAIL: Calendar::getTimeZone failed");

    Calendar *cal2 = Calendar::createInstance(cal->getTimeZone(), status);
    if (failure(status, "Calendar::createInstance")) return;
    cal->setTime(when, status);
    cal2->setTime(when, status);
    if (failure(status, "Calendar::setTime")) return;
    
    if (!(*cal == *cal2)) errln("FAIL: Calendar::operator== failed");
    if ((*cal != *cal2))  errln("FAIL: Calendar::operator!= failed");
    if (!cal->equals(*cal2, status) ||
        cal->before(*cal2, status) ||
        cal->after(*cal2, status) ||
        U_FAILURE(status)) errln("FAIL: equals/before/after failed");

    logln(UnicodeString("cal=")  +cal->getTime(status)  + UnicodeString(calToStr(*cal)));
    logln(UnicodeString("cal2=")  +cal2->getTime(status)  + UnicodeString(calToStr(*cal2)));
    logln("cal2->setTime(when+1000)");
    cal2->setTime(when + 1000, status);
    logln(UnicodeString("cal2=")  +cal2->getTime(status)  + UnicodeString(calToStr(*cal2)));

    if (failure(status, "Calendar::setTime")) return;
    if (cal->equals(*cal2, status) ||
        cal2->before(*cal, status) ||
        cal->after(*cal2, status) ||
        U_FAILURE(status)) errln("FAIL: equals/before/after failed after setTime(+1000)");

    logln("cal->roll(UCAL_SECOND)");
    cal->roll(UCAL_SECOND, (UBool) TRUE, status);
    logln(UnicodeString("cal=")  +cal->getTime(status)  + UnicodeString(calToStr(*cal)));
    cal->roll(UCAL_SECOND, (int32_t)0, status);
    logln(UnicodeString("cal=")  +cal->getTime(status)  + UnicodeString(calToStr(*cal)));
    if (failure(status, "Calendar::roll")) return;

    if (!(eq=cal->equals(*cal2, status)) ||
        (b4=cal->before(*cal2, status)) ||
        (af=cal->after(*cal2, status)) ||
        U_FAILURE(status)) {
      errln("FAIL: equals[%c]/before[%c]/after[%c] failed after roll 1 second [should be T/F/F]",
            eq?'T':'F',
            b4?'T':'F',
            af?'T':'F');
      logln(UnicodeString("cal=")  +cal->getTime(status)  + UnicodeString(calToStr(*cal)));
      logln(UnicodeString("cal2=")  +cal2->getTime(status)  + UnicodeString(calToStr(*cal2)));
    }

    // Roll back to January
    cal->roll(UCAL_MONTH, (int32_t)(1 + UCAL_DECEMBER - cal->get(UCAL_MONTH, status)), status);
    if (failure(status, "Calendar::roll")) return;
    if (cal->equals(*cal2, status) ||
        cal2->before(*cal, status) ||
        cal->after(*cal2, status) ||
        U_FAILURE(status)) errln("FAIL: equals/before/after failed after rollback to January");

    TimeZone *z = cal->orphanTimeZone();
    if (z->getID(str) != tzid ||
        z->getRawOffset() != tzoffset)
        errln("FAIL: orphanTimeZone failed");

    int32_t i;
    for (i=0; i<2; ++i)
    {
        UBool lenient = ( i > 0 );
        cal->setLenient(lenient);
        if (lenient != cal->isLenient()) errln("FAIL: setLenient/isLenient failed");
        // Later: Check for lenient behavior
    }

    for (i=UCAL_SUNDAY; i<=UCAL_SATURDAY; ++i)
    {
        cal->setFirstDayOfWeek((UCalendarDaysOfWeek)i);
        if (cal->getFirstDayOfWeek() != i) errln("FAIL: set/getFirstDayOfWeek failed");
        UErrorCode aStatus = U_ZERO_ERROR;
        if (cal->getFirstDayOfWeek(aStatus) != i || U_FAILURE(aStatus)) errln("FAIL: getFirstDayOfWeek(status) failed");
    }

    for (i=1; i<=7; ++i)
    {
        cal->setMinimalDaysInFirstWeek((uint8_t)i);
        if (cal->getMinimalDaysInFirstWeek() != i) errln("FAIL: set/getFirstDayOfWeek failed");
    }

    for (i=0; i<UCAL_FIELD_COUNT; ++i)
    {
        if (cal->getMinimum((UCalendarDateFields)i) > cal->getGreatestMinimum((UCalendarDateFields)i))
            errln("FAIL: getMinimum larger than getGreatestMinimum for field " + i);
        if (cal->getLeastMaximum((UCalendarDateFields)i) > cal->getMaximum((UCalendarDateFields)i))
            errln("FAIL: getLeastMaximum larger than getMaximum for field " + i);
        if (cal->getMinimum((UCalendarDateFields)i) >= cal->getMaximum((UCalendarDateFields)i))
            errln("FAIL: getMinimum not less than getMaximum for field " + i);
    }

    cal->adoptTimeZone(TimeZone::createDefault());
    cal->clear();
    cal->set(1984, 5, 24);
    if (cal->getTime(status) != date(84, 5, 24) || U_FAILURE(status))
        errln("FAIL: Calendar::set(3 args) failed");

    cal->clear();
    cal->set(1985, 3, 2, 11, 49);
    if (cal->getTime(status) != date(85, 3, 2, 11, 49) || U_FAILURE(status))
        errln("FAIL: Calendar::set(5 args) failed");

    cal->clear();
    cal->set(1995, 9, 12, 1, 39, 55);
    if (cal->getTime(status) != date(95, 9, 12, 1, 39, 55) || U_FAILURE(status))
        errln("FAIL: Calendar::set(6 args) failed");

    cal->getTime(status);
    if (failure(status, "Calendar::getTime")) return;
    for (i=0; i<UCAL_FIELD_COUNT; ++i)
    {
        switch(i) {
            case UCAL_YEAR: case UCAL_MONTH: case UCAL_DATE:
            case UCAL_HOUR_OF_DAY: case UCAL_MINUTE: case UCAL_SECOND:
            case UCAL_EXTENDED_YEAR:
              if (!cal->isSet((UCalendarDateFields)i)) errln("FAIL: Calendar::isSet F, should be T " + fieldName((UCalendarDateFields)i));
                break;
            default:
              if (cal->isSet((UCalendarDateFields)i)) errln("FAIL: Calendar::isSet = T, should be F  " + fieldName((UCalendarDateFields)i));
        }
        cal->clear((UCalendarDateFields)i);
        if (cal->isSet((UCalendarDateFields)i)) errln("FAIL: Calendar::clear/isSet failed " + fieldName((UCalendarDateFields)i));
    }

    if(cal->getActualMinimum(Calendar::SECOND, status) != 0){
        errln("Calendar is suppose to return 0 for getActualMinimum");
    }

    Calendar *cal3 = Calendar::createInstance(status);
    cal3->roll(Calendar::SECOND, (int32_t)0, status);
    if (failure(status, "Calendar::roll(EDateFields, int32_t, UErrorCode)")) return;

    delete cal;
    delete cal2;
    delete cal3;

    int32_t count;
    const Locale* loc = Calendar::getAvailableLocales(count);
    if (count < 1 || loc == 0)
    {
        dataerrln("FAIL: getAvailableLocales failed");
    }
    else
    {
        for (i=0; i<count; ++i)
        {
            cal = Calendar::createInstance(loc[i], status);
            if (failure(status, "Calendar::createInstance")) return;
            delete cal;
        }
    }

    cal = Calendar::createInstance(TimeZone::createDefault(), Locale::getEnglish(), status);
    if (failure(status, "Calendar::createInstance")) return;
    delete cal;

    cal = Calendar::createInstance(*zone, Locale::getEnglish(), status);
    if (failure(status, "Calendar::createInstance")) return;
    delete cal;

    GregorianCalendar *gc = new GregorianCalendar(*zone, status);
    if (failure(status, "new GregorianCalendar")) return;
    delete gc;

    gc = new GregorianCalendar(Locale::getEnglish(), status);
    if (failure(status, "new GregorianCalendar")) return;
    delete gc;

    gc = new GregorianCalendar(Locale::getEnglish(), status);
    delete gc;

    gc = new GregorianCalendar(*zone, Locale::getEnglish(), status);
    if (failure(status, "new GregorianCalendar")) return;
    delete gc;

    gc = new GregorianCalendar(zone, status);
    if (failure(status, "new GregorianCalendar")) return;
    delete gc;

    gc = new GregorianCalendar(1998, 10, 14, 21, 43, status);
    if (gc->getTime(status) != (d =date(98, 10, 14, 21, 43) )|| U_FAILURE(status))
      errln("FAIL: new GregorianCalendar(ymdhm) failed with " + UnicodeString(u_errorName(status)) + ",  cal="  + gc->getTime(status)  + UnicodeString(calToStr(*gc)) + ", d=" + d);
    else
      logln(UnicodeString("GOOD: cal=")  +gc->getTime(status)  + UnicodeString(calToStr(*gc)) + ", d=" + d);
    delete gc;

    gc = new GregorianCalendar(1998, 10, 14, 21, 43, 55, status);
    if (gc->getTime(status) != (d=date(98, 10, 14, 21, 43, 55)) || U_FAILURE(status))
      errln("FAIL: new GregorianCalendar(ymdhms) failed with " + UnicodeString(u_errorName(status)));

    GregorianCalendar gc2(Locale::getEnglish(), status);
    if (failure(status, "new GregorianCalendar")) return;
    gc2 = *gc;
    if (gc2 != *gc || !(gc2 == *gc)) errln("FAIL: GregorianCalendar assignment/operator==/operator!= failed");
    delete gc;
    delete z;

    /* Code coverage for Calendar class. */
    cal = Calendar::createInstance(status);
    if (failure(status, "Calendar::createInstance")) {
        return;
    }else {
        ((Calendar *)cal)->roll(UCAL_HOUR, (int32_t)100, status);
        ((Calendar *)cal)->clear(UCAL_HOUR);
#if !UCONFIG_NO_SERVICE
        URegistryKey key = cal->registerFactory(NULL, status);
        cal->unregister(key, status);
#endif
    }
    delete cal;

    status = U_ZERO_ERROR;
    cal = Calendar::createInstance(Locale("he_IL@calendar=hebrew"), status);
    if (failure(status, "Calendar::createInstance")) {
        return;
    } else {
        cal->roll(Calendar::MONTH, (int32_t)100, status);
    }

    StringEnumeration *en = Calendar::getKeywordValuesForLocale(NULL, Locale::getDefault(),FALSE, status);
    if (en == NULL || U_FAILURE(status)) {
        dataerrln("FAIL: getKeywordValuesForLocale for Calendar. : %s", u_errorName(status));
    }
    delete en;
    delete cal;
}

// -------------------------------------

/**
 * This test confirms the correct behavior of add when incrementing
 * through subsequent days.
 */
void
CalendarTest::TestRog()
{
    UErrorCode status = U_ZERO_ERROR;
    GregorianCalendar* gc = new GregorianCalendar(status);
    if (failure(status, "new GregorianCalendar", TRUE)) return;
    int32_t year = 1997, month = UCAL_APRIL, date = 1;
    gc->set(year, month, date);
    gc->set(UCAL_HOUR_OF_DAY, 23);
    gc->set(UCAL_MINUTE, 0);
    gc->set(UCAL_SECOND, 0);
    gc->set(UCAL_MILLISECOND, 0);
    for (int32_t i = 0; i < 9; i++, gc->add(UCAL_DATE, 1, status)) {
        if (U_FAILURE(status)) { errln("Calendar::add failed"); return; }
        if (gc->get(UCAL_YEAR, status) != year ||
            gc->get(UCAL_MONTH, status) != month ||
            gc->get(UCAL_DATE, status) != (date + i)) errln("FAIL: Date wrong");
        if (U_FAILURE(status)) { errln("Calendar::get failed"); return; }
    }
    delete gc;
}
 
// -------------------------------------

/**
 * Test the handling of the day of the week, checking for correctness and
 * for correct minimum and maximum values.
 */
void
CalendarTest::TestDOW943()
{
    dowTest(FALSE);
    dowTest(TRUE);
}

void CalendarTest::dowTest(UBool lenient)
{
    UErrorCode status = U_ZERO_ERROR;
    GregorianCalendar* cal = new GregorianCalendar(status);
    if (failure(status, "new GregorianCalendar", TRUE)) return;
    logln("cal - Aug 12, 1997\n");
    cal->set(1997, UCAL_AUGUST, 12);
    cal->getTime(status);
    if (U_FAILURE(status)) { errln("Calendar::getTime failed"); return; }
    logln((lenient?UnicodeString("LENIENT0: "):UnicodeString("nonlenient0: ")) + UnicodeString(calToStr(*cal)));
    cal->setLenient(lenient);
    logln("cal - Dec 1, 1996\n");
    cal->set(1996, UCAL_DECEMBER, 1);
    logln((lenient?UnicodeString("LENIENT: "):UnicodeString("nonlenient: ")) + UnicodeString(calToStr(*cal)));
    int32_t dow = cal->get(UCAL_DAY_OF_WEEK, status);
    if (U_FAILURE(status)) { errln("Calendar::get failed [%s]", u_errorName(status)); return; }
    int32_t min = cal->getMinimum(UCAL_DAY_OF_WEEK);
    int32_t max = cal->getMaximum(UCAL_DAY_OF_WEEK);
    if (dow < min ||
        dow > max) errln(UnicodeString("FAIL: Day of week ") + (int32_t)dow + " out of range");
    if (dow != UCAL_SUNDAY) errln("FAIL: Day of week should be SUNDAY[%d] not %d", UCAL_SUNDAY, dow);
    if (min != UCAL_SUNDAY ||
        max != UCAL_SATURDAY) errln("FAIL: Min/max bad");
    delete cal;
}

// -------------------------------------

/** 
 * Confirm that cloned Calendar objects do not inadvertently share substructures.
 */
void
CalendarTest::TestClonesUnique908()
{
    UErrorCode status = U_ZERO_ERROR;
    Calendar *c = Calendar::createInstance(status);
    if (failure(status, "Calendar::createInstance", TRUE)) return;
    Calendar *d = (Calendar*) c->clone();
    c->set(UCAL_MILLISECOND, 123);
    d->set(UCAL_MILLISECOND, 456);
    if (c->get(UCAL_MILLISECOND, status) != 123 ||
        d->get(UCAL_MILLISECOND, status) != 456) {
        errln("FAIL: Clones share fields");
    }
    if (U_FAILURE(status)) { errln("Calendar::get failed"); return; }
    delete c;
    delete d;
}
 
// -------------------------------------

/**
 * Confirm that the Gregorian cutoff value works as advertised.
 */
void
CalendarTest::TestGregorianChange768()
{
    UBool b;
    UErrorCode status = U_ZERO_ERROR;
    UnicodeString str;
    GregorianCalendar* c = new GregorianCalendar(status);
    if (failure(status, "new GregorianCalendar", TRUE)) return;
    logln(UnicodeString("With cutoff ") + dateToString(c->getGregorianChange(), str));
    b = c->isLeapYear(1800);
    logln(UnicodeString(" isLeapYear(1800) = ") + (b ? "true" : "false"));
    logln(UnicodeString(" (should be FALSE)"));
    if (b) errln("FAIL");
    c->setGregorianChange(date(0, 0, 1), status);
    if (U_FAILURE(status)) { errln("GregorianCalendar::setGregorianChange failed"); return; }
    logln(UnicodeString("With cutoff ") + dateToString(c->getGregorianChange(), str));
    b = c->isLeapYear(1800);
    logln(UnicodeString(" isLeapYear(1800) = ") + (b ? "true" : "false"));
    logln(UnicodeString(" (should be TRUE)"));
    if (!b) errln("FAIL");
    delete c;
}
 
// -------------------------------------

/**
 * Confirm the functioning of the field disambiguation algorithm.
 */
void
CalendarTest::TestDisambiguation765()
{
    UErrorCode status = U_ZERO_ERROR;
    Calendar *c = Calendar::createInstance("en_US", status);
    if (failure(status, "Calendar::createInstance", TRUE)) return;
    c->setLenient(FALSE);
    c->clear();
    c->set(UCAL_YEAR, 1997);
    c->set(UCAL_MONTH, UCAL_JUNE);
    c->set(UCAL_DATE, 3);
    verify765("1997 third day of June = ", c, 1997, UCAL_JUNE, 3);
    c->clear();
    c->set(UCAL_YEAR, 1997);
    c->set(UCAL_DAY_OF_WEEK, UCAL_TUESDAY);
    c->set(UCAL_MONTH, UCAL_JUNE);
    c->set(UCAL_DAY_OF_WEEK_IN_MONTH, 1);
    verify765("1997 first Tuesday in June = ", c, 1997, UCAL_JUNE, 3);
    c->clear();
    c->set(UCAL_YEAR, 1997);
    c->set(UCAL_DAY_OF_WEEK, UCAL_TUESDAY);
    c->set(UCAL_MONTH, UCAL_JUNE);
    c->set(UCAL_DAY_OF_WEEK_IN_MONTH, - 1);
    verify765("1997 last Tuesday in June = ", c, 1997, UCAL_JUNE, 24);

    status = U_ZERO_ERROR;
    c->clear();
    c->set(UCAL_YEAR, 1997);
    c->set(UCAL_DAY_OF_WEEK, UCAL_TUESDAY);
    c->set(UCAL_MONTH, UCAL_JUNE);
    c->set(UCAL_DAY_OF_WEEK_IN_MONTH, 0);
    c->getTime(status);
    verify765("1997 zero-th Tuesday in June = ", status);

    c->clear();
    c->set(UCAL_YEAR, 1997);
    c->set(UCAL_DAY_OF_WEEK, UCAL_TUESDAY);
    c->set(UCAL_MONTH, UCAL_JUNE);
    c->set(UCAL_WEEK_OF_MONTH, 1);
    verify765("1997 Tuesday in week 1 of June = ", c, 1997, UCAL_JUNE, 3);
    c->clear();
    c->set(UCAL_YEAR, 1997);
    c->set(UCAL_DAY_OF_WEEK, UCAL_TUESDAY);
    c->set(UCAL_MONTH, UCAL_JUNE);
    c->set(UCAL_WEEK_OF_MONTH, 5);
    verify765("1997 Tuesday in week 5 of June = ", c, 1997, UCAL_JULY, 1);

    status = U_ZERO_ERROR;
    c->clear();
    c->set(UCAL_YEAR, 1997);
    c->set(UCAL_DAY_OF_WEEK, UCAL_TUESDAY);
    c->set(UCAL_MONTH, UCAL_JUNE);
    c->set(UCAL_WEEK_OF_MONTH, 0);
    c->setMinimalDaysInFirstWeek(1);
    c->getTime(status);
    verify765("1997 Tuesday in week 0 of June = ", status);

    /* Note: The following test used to expect YEAR 1997, WOY 1 to
     * resolve to a date in Dec 1996; that is, to behave as if
     * YEAR_WOY were 1997.  With the addition of a new explicit
     * YEAR_WOY field, YEAR_WOY must itself be set if that is what is
     * desired.  Using YEAR in combination with WOY is ambiguous, and
     * results in the first WOY/DOW day of the year satisfying the
     * given fields (there may be up to two such days). In this case,
     * it propertly resolves to Tue Dec 30 1997, which has a WOY value
     * of 1 (for YEAR_WOY 1998) and a DOW of Tuesday, and falls in the
     * _calendar_ year 1997, as specified. - aliu */
    c->clear();
    c->set(UCAL_YEAR_WOY, 1997); // aliu
    c->set(UCAL_DAY_OF_WEEK, UCAL_TUESDAY);
    c->set(UCAL_WEEK_OF_YEAR, 1);
    verify765("1997 Tuesday in week 1 of yearWOY = ", c, 1996, UCAL_DECEMBER, 31);
    c->clear(); // - add test for YEAR
    c->setMinimalDaysInFirstWeek(1);
    c->set(UCAL_YEAR, 1997);
    c->set(UCAL_DAY_OF_WEEK, UCAL_TUESDAY);
    c->set(UCAL_WEEK_OF_YEAR, 1);
    verify765("1997 Tuesday in week 1 of year = ", c, 1997, UCAL_DECEMBER, 30);
    c->clear();
    c->set(UCAL_YEAR, 1997);
    c->set(UCAL_DAY_OF_WEEK, UCAL_TUESDAY);
    c->set(UCAL_WEEK_OF_YEAR, 10);
    verify765("1997 Tuesday in week 10 of year = ", c, 1997, UCAL_MARCH, 4);
    //try {

    // {sfb} week 0 is no longer a valid week of year
    /*c->clear();
    c->set(Calendar::YEAR, 1997);
    c->set(Calendar::DAY_OF_WEEK, Calendar::TUESDAY);
    //c->set(Calendar::WEEK_OF_YEAR, 0);
    c->set(Calendar::WEEK_OF_YEAR, 1);
    verify765("1997 Tuesday in week 0 of year = ", c, 1996, Calendar::DECEMBER, 24);*/

    //}
    //catch(IllegalArgumentException ex) {
    //    errln("FAIL: Exception seen:");
    //    ex.printStackTrace(log);
    //}
    delete c;
}
 
// -------------------------------------
 
void
CalendarTest::verify765(const UnicodeString& msg, Calendar* c, int32_t year, int32_t month, int32_t day)
{
    UnicodeString str;
    UErrorCode status = U_ZERO_ERROR;
    int32_t y = c->get(UCAL_YEAR, status);
    int32_t m = c->get(UCAL_MONTH, status);
    int32_t d = c->get(UCAL_DATE, status);
    if ( y == year &&
         m == month &&
         d == day) {
        if (U_FAILURE(status)) { errln("FAIL: Calendar::get failed"); return; }
        logln("PASS: " + msg + dateToString(c->getTime(status), str));
        if (U_FAILURE(status)) { errln("Calendar::getTime failed"); return; }
    }
    else {
        errln("FAIL: " + msg + dateToString(c->getTime(status), str) + "; expected " + (int32_t)year + "/" + (int32_t)(month + 1) + "/" + (int32_t)day +
            "; got " + (int32_t)y + "/" + (int32_t)(m + 1) + "/" + (int32_t)d + " for Locale: " + c->getLocaleID(ULOC_ACTUAL_LOCALE,status));
        if (U_FAILURE(status)) { errln("Calendar::getTime failed"); return; }
    }
}
 
// -------------------------------------
 
void
CalendarTest::verify765(const UnicodeString& msg/*, IllegalArgumentException e*/, UErrorCode status)
{
    if (status != U_ILLEGAL_ARGUMENT_ERROR) errln("FAIL: No IllegalArgumentException for " + msg);
    else logln("PASS: " + msg + "IllegalArgument as expected");
}
 
// -------------------------------------

/**
 * Confirm that the offset between local time and GMT behaves as expected.
 */
void
CalendarTest::TestGMTvsLocal4064654()
{
    test4064654(1997, 1, 1, 12, 0, 0);
    test4064654(1997, 4, 16, 18, 30, 0);
}
 
// -------------------------------------
 
void
CalendarTest::test4064654(int32_t yr, int32_t mo, int32_t dt, int32_t hr, int32_t mn, int32_t sc)
{
    UDate date;
    UErrorCode status = U_ZERO_ERROR;
    UnicodeString str;
    Calendar *gmtcal = Calendar::createInstance(status);
    if (failure(status, "Calendar::createInstance", TRUE)) return;
    gmtcal->adoptTimeZone(TimeZone::createTimeZone("Africa/Casablanca"));
    gmtcal->set(yr, mo - 1, dt, hr, mn, sc);
    gmtcal->set(UCAL_MILLISECOND, 0);
    date = gmtcal->getTime(status);
    if (U_FAILURE(status)) { errln("Calendar::getTime failed"); return; }
    logln("date = " + dateToString(date, str));
    Calendar *cal = Calendar::createInstance(status);
    if (U_FAILURE(status)) { errln("Calendar::createInstance failed"); return; }
    cal->setTime(date, status);
    if (U_FAILURE(status)) { errln("Calendar::setTime failed"); return; }
    int32_t offset = cal->getTimeZone().getOffset((uint8_t)cal->get(UCAL_ERA, status),
                                                  cal->get(UCAL_YEAR, status),
                                                  cal->get(UCAL_MONTH, status),
                                                  cal->get(UCAL_DATE, status),
                                                  (uint8_t)cal->get(UCAL_DAY_OF_WEEK, status),
                                                  cal->get(UCAL_MILLISECOND, status), status);
    if (U_FAILURE(status)) { errln("Calendar::get failed"); return; }
    logln("offset for " + dateToString(date, str) + "= " + (offset / 1000 / 60 / 60.0) + "hr");
    int32_t utc = ((cal->get(UCAL_HOUR_OF_DAY, status) * 60 +
                    cal->get(UCAL_MINUTE, status)) * 60 +
                   cal->get(UCAL_SECOND, status)) * 1000 +
        cal->get(UCAL_MILLISECOND, status) - offset;
    if (U_FAILURE(status)) { errln("Calendar::get failed"); return; }
    int32_t expected = ((hr * 60 + mn) * 60 + sc) * 1000;
    if (utc != expected) errln(UnicodeString("FAIL: Discrepancy of ") + (utc - expected) +
                               " millis = " + ((utc - expected) / 1000 / 60 / 60.0) + " hr");
    delete gmtcal;
    delete cal;
}
 
// -------------------------------------
 
/**
 * The operations of adding and setting should not exhibit pathological
 * dependence on the order of operations.  This test checks for this.
 */
void
CalendarTest::TestAddSetOrder621()
{
    UDate d = date(97, 4, 14, 13, 23, 45);
    UErrorCode status = U_ZERO_ERROR;
    Calendar *cal = Calendar::createInstance(status);
    if (failure(status, "Calendar::createInstance", TRUE)) return;

    cal->setTime(d, status);
    if (U_FAILURE(status)) { 
        errln("Calendar::setTime failed"); 
        delete cal;
        return; 
    }
    cal->add(UCAL_DATE, - 5, status);
    if (U_FAILURE(status)) { 
        errln("Calendar::add failed"); 
        delete cal;
        return; 
    }
    cal->set(UCAL_HOUR_OF_DAY, 0);
    cal->set(UCAL_MINUTE, 0);
    cal->set(UCAL_SECOND, 0);
    UnicodeString s;
    dateToString(cal->getTime(status), s);
    if (U_FAILURE(status)) { 
        errln("Calendar::getTime failed"); 
        delete cal;
        return; 
    }
    delete cal;

    cal = Calendar::createInstance(status);
    if (U_FAILURE(status)) { 
        errln("Calendar::createInstance failed"); 
        delete cal;
        return; 
    }
    cal->setTime(d, status);
    if (U_FAILURE(status)) { 
        errln("Calendar::setTime failed"); 
        delete cal;
        return; 
    }
    cal->set(UCAL_HOUR_OF_DAY, 0);
    cal->set(UCAL_MINUTE, 0);
    cal->set(UCAL_SECOND, 0);
    cal->add(UCAL_DATE, - 5, status);
    if (U_FAILURE(status)) { 
        errln("Calendar::add failed"); 
        delete cal;
        return; 
    }
    UnicodeString s2;
    dateToString(cal->getTime(status), s2);
    if (U_FAILURE(status)) { 
        errln("Calendar::getTime failed"); 
        delete cal;
        return; 
    }
    if (s == s2) 
        logln("Pass: " + s + " == " + s2);
    else 
        errln("FAIL: " + s + " != " + s2);
    delete cal;
}
 
// -------------------------------------
 
/**
 * Confirm that adding to various fields works.
 */
void
CalendarTest::TestAdd520()
{
    int32_t y = 1997, m = UCAL_FEBRUARY, d = 1;
    UErrorCode status = U_ZERO_ERROR;
    GregorianCalendar *temp = new GregorianCalendar(y, m, d, status);
    if (failure(status, "new GregorianCalendar", TRUE)) return;
    check520(temp, y, m, d);
    temp->add(UCAL_YEAR, 1, status);
    if (U_FAILURE(status)) { errln("Calendar::add failed"); return; }
    y++;
    check520(temp, y, m, d);
    temp->add(UCAL_MONTH, 1, status);
    if (U_FAILURE(status)) { errln("Calendar::add failed"); return; }
    m++;
    check520(temp, y, m, d);
    temp->add(UCAL_DATE, 1, status);
    if (U_FAILURE(status)) { errln("Calendar::add failed"); return; }
    d++;
    check520(temp, y, m, d);
    temp->add(UCAL_DATE, 2, status);
    if (U_FAILURE(status)) { errln("Calendar::add failed"); return; }
    d += 2;
    check520(temp, y, m, d);
    temp->add(UCAL_DATE, 28, status);
    if (U_FAILURE(status)) { errln("Calendar::add failed"); return; }
    d = 1;++m;
    check520(temp, y, m, d);
    delete temp;
}
 
// -------------------------------------
 
/**
 * Execute adding and rolling in GregorianCalendar extensively,
 */
void
CalendarTest::TestAddRollExtensive()
{
    int32_t maxlimit = 40;
    int32_t y = 1997, m = UCAL_FEBRUARY, d = 1, hr = 1, min = 1, sec = 0, ms = 0;
    UErrorCode status = U_ZERO_ERROR;
    GregorianCalendar *temp = new GregorianCalendar(y, m, d, status);
    if (failure(status, "new GregorianCalendar", TRUE)) return;

    temp->set(UCAL_HOUR, hr);
    temp->set(UCAL_MINUTE, min);
    temp->set(UCAL_SECOND, sec);
    temp->set(UCAL_MILLISECOND, ms);
    temp->setMinimalDaysInFirstWeek(1);

    UCalendarDateFields e;

    logln("Testing GregorianCalendar add...");
    e = UCAL_YEAR;
    while (e < UCAL_FIELD_COUNT) {
        int32_t i;
        int32_t limit = maxlimit;
        status = U_ZERO_ERROR;
        for (i = 0; i < limit; i++) {
            temp->add(e, 1, status);
            if (U_FAILURE(status)) { limit = i; status = U_ZERO_ERROR; }
        }
        for (i = 0; i < limit; i++) {
            temp->add(e, -1, status);
            if (U_FAILURE(status)) { errln("GregorianCalendar::add -1 failed"); return; }
        }
        check520(temp, y, m, d, hr, min, sec, ms, e);

        e = (UCalendarDateFields) ((int32_t) e + 1);
    }

    logln("Testing GregorianCalendar roll...");
    e = UCAL_YEAR;
    while (e < UCAL_FIELD_COUNT) {
        int32_t i;
        int32_t limit = maxlimit;
        status = U_ZERO_ERROR;
        for (i = 0; i < limit; i++) {
            logln(calToStr(*temp) + UnicodeString("  " ) + fieldName(e) + UnicodeString("++") );
            temp->roll(e, 1, status);
            if (U_FAILURE(status)) { 
              logln("caltest.cpp:%d e=%d, i=%d - roll(+) err %s\n",  __LINE__, (int) e, (int) i, u_errorName(status));
              logln(calToStr(*temp));
              limit = i; status = U_ZERO_ERROR; 
            }
        }
        for (i = 0; i < limit; i++) {
            logln("caltest.cpp:%d e=%d, i=%d\n",  __LINE__, (int) e, (int) i);
            logln(calToStr(*temp) + UnicodeString("  " ) + fieldName(e) + UnicodeString("--") );
            temp->roll(e, -1, status);
            if (U_FAILURE(status)) { errln(UnicodeString("GregorianCalendar::roll ") + CalendarTest::fieldName(e) + " count=" + UnicodeString('@'+i) + " by -1 failed with " + u_errorName(status) ); return; }
        }
        check520(temp, y, m, d, hr, min, sec, ms, e);

        e = (UCalendarDateFields) ((int32_t) e + 1);
    }

    delete temp;
}
 
// -------------------------------------
void 
CalendarTest::check520(Calendar* c, 
                        int32_t y, int32_t m, int32_t d, 
                        int32_t hr, int32_t min, int32_t sec, 
                        int32_t ms, UCalendarDateFields field)

{
    UErrorCode status = U_ZERO_ERROR;
    if (c->get(UCAL_YEAR, status) != y ||
        c->get(UCAL_MONTH, status) != m ||
        c->get(UCAL_DATE, status) != d ||
        c->get(UCAL_HOUR, status) != hr ||
        c->get(UCAL_MINUTE, status) != min ||
        c->get(UCAL_SECOND, status) != sec ||
        c->get(UCAL_MILLISECOND, status) != ms) {
        errln(UnicodeString("U_FAILURE for field ") + (int32_t)field + 
                ": Expected y/m/d h:m:s:ms of " +
                y + "/" + (m + 1) + "/" + d + " " +
              hr + ":" + min + ":" + sec + ":" + ms +
              "; got " + c->get(UCAL_YEAR, status) +
              "/" + (c->get(UCAL_MONTH, status) + 1) +
              "/" + c->get(UCAL_DATE, status) +
              " " + c->get(UCAL_HOUR, status) + ":" +
              c->get(UCAL_MINUTE, status) + ":" + 
              c->get(UCAL_SECOND, status) + ":" +
              c->get(UCAL_MILLISECOND, status)
              );

        if (U_FAILURE(status)) { errln("Calendar::get failed"); return; }
    }
    else 
        logln(UnicodeString("Confirmed: ") + y + "/" +
                (m + 1) + "/" + d + " " +
                hr + ":" + min + ":" + sec + ":" + ms);
}
 
// -------------------------------------
void 
CalendarTest::check520(Calendar* c, 
                        int32_t y, int32_t m, int32_t d)

{
    UErrorCode status = U_ZERO_ERROR;
    if (c->get(UCAL_YEAR, status) != y ||
        c->get(UCAL_MONTH, status) != m ||
        c->get(UCAL_DATE, status) != d) {
        errln(UnicodeString("FAILURE: Expected y/m/d of ") +
              y + "/" + (m + 1) + "/" + d + " " +
              "; got " + c->get(UCAL_YEAR, status) +
              "/" + (c->get(UCAL_MONTH, status) + 1) +
              "/" + c->get(UCAL_DATE, status)
              );

        if (U_FAILURE(status)) { errln("Calendar::get failed"); return; }
    }
    else 
        logln(UnicodeString("Confirmed: ") + y + "/" +
                (m + 1) + "/" + d);
}

// -------------------------------------
 
/**
 * Test that setting of fields works.  In particular, make sure that all instances
 * of GregorianCalendar don't share a static instance of the fields array.
 */
void
CalendarTest::TestFieldSet4781()
{
    // try {
        UErrorCode status = U_ZERO_ERROR;
        GregorianCalendar *g = new GregorianCalendar(status);
        if (failure(status, "new GregorianCalendar", TRUE)) return;
        GregorianCalendar *g2 = new GregorianCalendar(status);
        if (U_FAILURE(status)) { errln("Couldn't create GregorianCalendar"); return; }
        g2->set(UCAL_HOUR, 12, status);
        g2->set(UCAL_MINUTE, 0, status);
        g2->set(UCAL_SECOND, 0, status);
        if (U_FAILURE(status)) { errln("Calendar::set failed"); return; }
        if (*g == *g2) logln("Same");
        else logln("Different");
    //}
        //catch(IllegalArgumentException e) {
        //errln("Unexpected exception seen: " + e);
    //}
        delete g;
        delete g2;
}
 
// -------------------------------------

/* We don't support serialization on C++
void
CalendarTest::TestSerialize337()
{
    Calendar cal = Calendar::getInstance();
    UBool ok = FALSE;
    try {
        FileOutputStream f = new FileOutputStream(FILENAME);
        ObjectOutput s = new ObjectOutputStream(f);
        s.writeObject(PREFIX);
        s.writeObject(cal);
        s.writeObject(POSTFIX);
        f.close();
        FileInputStream in = new FileInputStream(FILENAME);
        ObjectInputStream t = new ObjectInputStream(in);
        UnicodeString& pre = (UnicodeString&) t.readObject();
        Calendar c = (Calendar) t.readObject();
        UnicodeString& post = (UnicodeString&) t.readObject();
        in.close();
        ok = pre.equals(PREFIX) &&
            post.equals(POSTFIX) &&
            cal->equals(c);
        File fl = new File(FILENAME);
        fl.delete();
    }
    catch(IOException e) {
        errln("FAIL: Exception received:");
        e.printStackTrace(log);
    }
    catch(ClassNotFoundException e) {
        errln("FAIL: Exception received:");
        e.printStackTrace(log);
    }
    if (!ok) errln("Serialization of Calendar object failed.");
}
 
UnicodeString& CalendarTest::PREFIX = "abc";
 
UnicodeString& CalendarTest::POSTFIX = "def";
 
UnicodeString& CalendarTest::FILENAME = "tmp337.bin";
 */

// -------------------------------------

/**
 * Verify that the seconds of a Calendar can be zeroed out through the
 * expected sequence of operations.
 */ 
void
CalendarTest::TestSecondsZero121()
{
    UErrorCode status = U_ZERO_ERROR;
    Calendar *cal = new GregorianCalendar(status);
    if (failure(status, "new GregorianCalendar", TRUE)) return;
    cal->setTime(Calendar::getNow(), status);
    if (U_FAILURE(status)) { errln("Calendar::setTime failed"); return; }
    cal->set(UCAL_SECOND, 0);
    if (U_FAILURE(status)) { errln("Calendar::set failed"); return; }
    UDate d = cal->getTime(status);
    if (U_FAILURE(status)) { errln("Calendar::getTime failed"); return; }
    UnicodeString s;
    dateToString(d, s);
    if (s.indexOf("DATE_FORMAT_FAILURE") >= 0) {
        dataerrln("Got: \"DATE_FORMAT_FAILURE\".");
    } else if (s.indexOf(":00 ") < 0) {
        errln("Expected to see :00 in " + s);
    }
    delete cal;
}
 
// -------------------------------------
 
/**
 * Verify that a specific sequence of adding and setting works as expected;
 * it should not vary depending on when and whether the get method is
 * called.
 */
void
CalendarTest::TestAddSetGet0610()
{
    UnicodeString EXPECTED_0610("1993/0/5", "");
    UErrorCode status = U_ZERO_ERROR;
    {
        Calendar *calendar = new GregorianCalendar(status);
        if (failure(status, "new GregorianCalendar", TRUE)) return;
        calendar->set(1993, UCAL_JANUARY, 4);
        logln("1A) " + value(calendar));
        calendar->add(UCAL_DATE, 1, status);
        if (U_FAILURE(status)) { errln("Calendar::add failed"); return; }
        UnicodeString v = value(calendar);
        logln("1B) " + v);
        logln("--) 1993/0/5");
        if (!(v == EXPECTED_0610)) errln("Expected " + EXPECTED_0610 + "; saw " + v);
        delete calendar;
    }
    {
        Calendar *calendar = new GregorianCalendar(1993, UCAL_JANUARY, 4, status);
        if (U_FAILURE(status)) { errln("Couldn't create GregorianCalendar"); return; }
        logln("2A) " + value(calendar));
        calendar->add(UCAL_DATE, 1, status);
        if (U_FAILURE(status)) { errln("Calendar::add failed"); return; }
        UnicodeString v = value(calendar);
        logln("2B) " + v);
        logln("--) 1993/0/5");
        if (!(v == EXPECTED_0610)) errln("Expected " + EXPECTED_0610 + "; saw " + v);
        delete calendar;
    }
    {
        Calendar *calendar = new GregorianCalendar(1993, UCAL_JANUARY, 4, status);
        if (U_FAILURE(status)) { errln("Couldn't create GregorianCalendar"); return; }
        logln("3A) " + value(calendar));
        calendar->getTime(status);
        if (U_FAILURE(status)) { errln("Calendar::getTime failed"); return; }
        calendar->add(UCAL_DATE, 1, status);
        if (U_FAILURE(status)) { errln("Calendar::add failed"); return; }
        UnicodeString v = value(calendar);
        logln("3B) " + v);
        logln("--) 1993/0/5");
        if (!(v == EXPECTED_0610)) errln("Expected " + EXPECTED_0610 + "; saw " + v);
        delete calendar;
    }
}
 
// -------------------------------------
 
UnicodeString
CalendarTest::value(Calendar* calendar)
{
    UErrorCode status = U_ZERO_ERROR;
    return UnicodeString("") + (int32_t)calendar->get(UCAL_YEAR, status) +
        "/" + (int32_t)calendar->get(UCAL_MONTH, status) +
        "/" + (int32_t)calendar->get(UCAL_DATE, status) +
        (U_FAILURE(status) ? " FAIL: Calendar::get failed" : "");
}
 
 
// -------------------------------------
 
/**
 * Verify that various fields on a known date are set correctly.
 */
void
CalendarTest::TestFields060()
{
    UErrorCode status = U_ZERO_ERROR;
    int32_t year = 1997;
    int32_t month = UCAL_OCTOBER;
    int32_t dDate = 22;
    GregorianCalendar *calendar = 0;
    calendar = new GregorianCalendar(year, month, dDate, status);
    if (failure(status, "new GregorianCalendar", TRUE)) return;
    for (int32_t i = 0; i < EXPECTED_FIELDS_length;) {
        UCalendarDateFields field = (UCalendarDateFields)EXPECTED_FIELDS[i++];
        int32_t expected = EXPECTED_FIELDS[i++];
        if (calendar->get(field, status) != expected) {
            errln(UnicodeString("Expected field ") + (int32_t)field + " to have value " + (int32_t)expected +
                  "; received " + (int32_t)calendar->get(field, status) + " instead");
            if (U_FAILURE(status)) { errln("Calendar::get failed"); return; }
        }
    }
    delete calendar;
}
 
int32_t CalendarTest::EXPECTED_FIELDS[] = {
    UCAL_YEAR, 1997,
    UCAL_MONTH, UCAL_OCTOBER,
    UCAL_DATE, 22,
    UCAL_DAY_OF_WEEK, UCAL_WEDNESDAY,
    UCAL_DAY_OF_WEEK_IN_MONTH, 4,
    UCAL_DAY_OF_YEAR, 295
};
 
const int32_t CalendarTest::EXPECTED_FIELDS_length = (int32_t)(sizeof(CalendarTest::EXPECTED_FIELDS) /
    sizeof(CalendarTest::EXPECTED_FIELDS[0]));

// -------------------------------------
 
/**
 * Verify that various fields on a known date are set correctly.  In this
 * case, the start of the epoch (January 1 1970).
 */
void
CalendarTest::TestEpochStartFields()
{
    UErrorCode status = U_ZERO_ERROR;
    TimeZone *z = TimeZone::createDefault();
    Calendar *c = Calendar::createInstance(status);
    if (failure(status, "Calendar::createInstance", TRUE)) return;
    UDate d = - z->getRawOffset();
    GregorianCalendar *gc = new GregorianCalendar(status);
    if (U_FAILURE(status)) { errln("Couldn't create GregorianCalendar"); return; }
    gc->setTimeZone(*z);
    gc->setTime(d, status);
    if (U_FAILURE(status)) { errln("Calendar::setTime failed"); return; }
    UBool idt = gc->inDaylightTime(status);
    if (U_FAILURE(status)) { errln("GregorianCalendar::inDaylightTime failed"); return; }
    if (idt) {
        UnicodeString str;
        logln("Warning: Skipping test because " + dateToString(d, str) + " is in DST.");
    }
    else {
        c->setTime(d, status);
        if (U_FAILURE(status)) { errln("Calendar::setTime failed"); return; }
        for (int32_t i = 0; i < UCAL_ZONE_OFFSET;++i) {
            if (c->get((UCalendarDateFields)i, status) != EPOCH_FIELDS[i])
                errln(UnicodeString("Expected field ") + i + " to have value " + EPOCH_FIELDS[i] +
                      "; saw " + c->get((UCalendarDateFields)i, status) + " instead");
            if (U_FAILURE(status)) { errln("Calendar::get failed"); return; }
        }
        if (c->get(UCAL_ZONE_OFFSET, status) != z->getRawOffset())
        {
            errln(UnicodeString("Expected field ZONE_OFFSET to have value ") + z->getRawOffset() +
                  "; saw " + c->get(UCAL_ZONE_OFFSET, status) + " instead");
            if (U_FAILURE(status)) { errln("Calendar::get failed"); return; }
        }
        if (c->get(UCAL_DST_OFFSET, status) != 0)
        {
            errln(UnicodeString("Expected field DST_OFFSET to have value 0") +
                  "; saw " + c->get(UCAL_DST_OFFSET, status) + " instead");
            if (U_FAILURE(status)) { errln("Calendar::get failed"); return; }
        }
    }
    delete c;
    delete z;
    delete gc;
}
 
int32_t CalendarTest::EPOCH_FIELDS[] = {
    1, 1970, 0, 53, 0, 1, 1, 5, 1, 0, 0, 0, 0, 0, 0, - 28800000, 0
};
 
// -------------------------------------
 
/**
 * Test that the days of the week progress properly when add is called repeatedly
 * for increments of 24 days.
 */
void
CalendarTest::TestDOWProgression()
{
    UErrorCode status = U_ZERO_ERROR;
    Calendar *cal = new GregorianCalendar(1972, UCAL_OCTOBER, 26, status);
    if (failure(status, "new GregorianCalendar", TRUE)) return;
    marchByDelta(cal, 24);
    delete cal;
}
 
// -------------------------------------

void
CalendarTest::TestDOW_LOCALandYEAR_WOY()
{
    /* Note: I've commented out the loop_addroll tests for YEAR and
     * YEAR_WOY below because these two fields should NOT behave
     * identically when adding.  YEAR should keep the month/dom
     * invariant.  YEAR_WOY should keep the woy/dow invariant.  I've
     * added a new test that checks for this in place of the old call
     * to loop_addroll. - aliu */
    UErrorCode status = U_ZERO_ERROR;
    int32_t times = 20;
    Calendar *cal=Calendar::createInstance(Locale::getGermany(), status);
    if (failure(status, "Calendar::createInstance", TRUE)) return;
    SimpleDateFormat *sdf=new SimpleDateFormat(UnicodeString("YYYY'-W'ww-ee"), Locale::getGermany(), status);
    if (U_FAILURE(status)) { errcheckln(status, "Couldn't create SimpleDateFormat - %s", u_errorName(status)); return; }

    // ICU no longer use localized date-time pattern characters by default.
    // So we set pattern chars using 'J' instead of 'Y'.
    DateFormatSymbols *dfs = new DateFormatSymbols(Locale::getGermany(), status);
    dfs->setLocalPatternChars(UnicodeString("GyMdkHmsSEDFwWahKzJeugAZvcLQq"));
    sdf->adoptDateFormatSymbols(dfs);
    sdf->applyLocalizedPattern(UnicodeString("JJJJ'-W'ww-ee"), status);
    if (U_FAILURE(status)) { errln("Couldn't apply localized pattern"); return; }

	cal->clear();
    cal->set(1997, UCAL_DECEMBER, 25);
    doYEAR_WOYLoop(cal, sdf, times, status);
    //loop_addroll(cal, /*sdf,*/ times, UCAL_YEAR_WOY, UCAL_YEAR,  status);
    yearAddTest(*cal, status); // aliu
    loop_addroll(cal, /*sdf,*/ times, UCAL_DOW_LOCAL, UCAL_DAY_OF_WEEK, status);
    if (U_FAILURE(status)) { errln("Error in parse/calculate test for 1997"); return; }

    cal->clear();
    cal->set(1998, UCAL_DECEMBER, 25);
    doYEAR_WOYLoop(cal, sdf, times, status);
    //loop_addroll(cal, /*sdf,*/ times, UCAL_YEAR_WOY, UCAL_YEAR,  status);
    yearAddTest(*cal, status); // aliu
    loop_addroll(cal, /*sdf,*/ times, UCAL_DOW_LOCAL, UCAL_DAY_OF_WEEK, status);
    if (U_FAILURE(status)) { errln("Error in parse/calculate test for 1998"); return; }

    cal->clear();
    cal->set(1582, UCAL_OCTOBER, 1);
    doYEAR_WOYLoop(cal, sdf, times, status);
    //loop_addroll(cal, /*sdf,*/ times, Calendar::YEAR_WOY, Calendar::YEAR,  status);
    yearAddTest(*cal, status); // aliu
    loop_addroll(cal, /*sdf,*/ times, UCAL_DOW_LOCAL, UCAL_DAY_OF_WEEK, status);
    if (U_FAILURE(status)) { errln("Error in parse/calculate test for 1582"); return; }
    delete sdf;
    delete cal;

    return;
}

/**
 * Confirm that adding a YEAR and adding a YEAR_WOY work properly for
 * the given Calendar at its current setting.
 */
void CalendarTest::yearAddTest(Calendar& cal, UErrorCode& status) {
    /**
     * When adding the YEAR, the month and day should remain constant.
     * When adding the YEAR_WOY, the WOY and DOW should remain constant. - aliu
     * Examples:
     *  Wed Jan 14 1998 / 1998-W03-03 Add(YEAR_WOY, 1) -> Wed Jan 20 1999 / 1999-W03-03
     *                                Add(YEAR, 1)     -> Thu Jan 14 1999 / 1999-W02-04
     *  Thu Jan 14 1999 / 1999-W02-04 Add(YEAR_WOY, 1) -> Thu Jan 13 2000 / 2000-W02-04
     *                                Add(YEAR, 1)     -> Fri Jan 14 2000 / 2000-W02-05
     *  Sun Oct 31 1582 / 1582-W42-07 Add(YEAR_WOY, 1) -> Sun Oct 23 1583 / 1583-W42-07
     *                                Add(YEAR, 1)     -> Mon Oct 31 1583 / 1583-W44-01
     */
    int32_t y   = cal.get(UCAL_YEAR, status);
    int32_t mon = cal.get(UCAL_MONTH, status);
    int32_t day = cal.get(UCAL_DATE, status);
    int32_t ywy = cal.get(UCAL_YEAR_WOY, status);
    int32_t woy = cal.get(UCAL_WEEK_OF_YEAR, status);
    int32_t dow = cal.get(UCAL_DOW_LOCAL, status);
    UDate t = cal.getTime(status);
    
    if(U_FAILURE(status)){
        errln(UnicodeString("Failed to create Calendar for locale. Error: ") + UnicodeString(u_errorName(status)));
        return;
    }
    UnicodeString str, str2;
    SimpleDateFormat fmt(UnicodeString("EEE MMM dd yyyy / YYYY'-W'ww-ee"), status);
    fmt.setCalendar(cal);

    fmt.format(t, str.remove());
    str += ".add(YEAR, 1)    =>";
    cal.add(UCAL_YEAR, 1, status);
    int32_t y2   = cal.get(UCAL_YEAR, status);
    int32_t mon2 = cal.get(UCAL_MONTH, status);
    int32_t day2 = cal.get(UCAL_DATE, status);
    fmt.format(cal.getTime(status), str);
    if (y2 != (y+1) || mon2 != mon || day2 != day) {
        str += (UnicodeString)", expected year " +
            (y+1) + ", month " + (mon+1) + ", day " + day;
        errln((UnicodeString)"FAIL: " + str);
        logln( UnicodeString(" -> ") + CalendarTest::calToStr(cal) );
    } else {
        logln(str);
    }

    fmt.format(t, str.remove());
    str += ".add(YEAR_WOY, 1)=>";
    cal.setTime(t, status);
    logln( UnicodeString(" <- ") + CalendarTest::calToStr(cal) );
    cal.add(UCAL_YEAR_WOY, 1, status);
    int32_t ywy2 = cal.get(UCAL_YEAR_WOY, status);
    int32_t woy2 = cal.get(UCAL_WEEK_OF_YEAR, status);
    int32_t dow2 = cal.get(UCAL_DOW_LOCAL, status);
    fmt.format(cal.getTime(status), str);
    if (ywy2 != (ywy+1) || woy2 != woy || dow2 != dow) {
        str += (UnicodeString)", expected yearWOY " +
            (ywy+1) + ", woy " + woy + ", dowLocal " + dow;
        errln((UnicodeString)"FAIL: " + str);
        logln( UnicodeString(" -> ") + CalendarTest::calToStr(cal) );
    } else {
        logln(str);
    }
}

// -------------------------------------

void CalendarTest::loop_addroll(Calendar *cal, /*SimpleDateFormat *sdf,*/ int times, UCalendarDateFields field, UCalendarDateFields field2, UErrorCode& errorCode) {
    Calendar *calclone;
    SimpleDateFormat fmt(UnicodeString("EEE MMM dd yyyy / YYYY'-W'ww-ee"), errorCode);
    fmt.setCalendar(*cal);
    int i;

    for(i = 0; i<times; i++) {
        calclone = cal->clone();
        UDate start = cal->getTime(errorCode);
        cal->add(field,1,errorCode);
        if (U_FAILURE(errorCode)) { errln("Error in add"); delete calclone; return; }
        calclone->add(field2,1,errorCode);
        if (U_FAILURE(errorCode)) { errln("Error in add"); delete calclone; return; }
        if(cal->getTime(errorCode) != calclone->getTime(errorCode)) {
            UnicodeString str("FAIL: Results of add differ. "), str2;
            str += fmt.format(start, str2) + " ";
            str += UnicodeString("Add(") + fieldName(field) + ", 1) -> " +
                fmt.format(cal->getTime(errorCode), str2.remove()) + "; ";
            str += UnicodeString("Add(") + fieldName(field2) + ", 1) -> " +
                fmt.format(calclone->getTime(errorCode), str2.remove());
            errln(str);
            delete calclone;
            return;
        }
        delete calclone;
    }

    for(i = 0; i<times; i++) {
        calclone = cal->clone();
        cal->roll(field,(int32_t)1,errorCode);
        if (U_FAILURE(errorCode)) { errln("Error in roll"); delete calclone; return; }
        calclone->roll(field2,(int32_t)1,errorCode);
        if (U_FAILURE(errorCode)) { errln("Error in roll"); delete calclone; return; }
        if(cal->getTime(errorCode) != calclone->getTime(errorCode)) {
            delete calclone;
            errln("Results of roll differ!");
            return;
        }
        delete calclone;
    }
}

// -------------------------------------

void
CalendarTest::doYEAR_WOYLoop(Calendar *cal, SimpleDateFormat *sdf, 
                                    int32_t times, UErrorCode& errorCode) {

    UnicodeString us;
    UDate tst, original;
    Calendar *tstres = new GregorianCalendar(Locale::getGermany(), errorCode);
    for(int i=0; i<times; ++i) {
        sdf->format(Formattable(cal->getTime(errorCode),Formattable::kIsDate), us, errorCode);
        //logln("expected: "+us);
        if (U_FAILURE(errorCode)) { errln("Format error"); return; }
        tst=sdf->parse(us,errorCode);
        if (U_FAILURE(errorCode)) { errln("Parse error"); return; }
        tstres->clear();
        tstres->setTime(tst, errorCode);
        //logln((UnicodeString)"Parsed week of year is "+tstres->get(UCAL_WEEK_OF_YEAR, errorCode));
        if (U_FAILURE(errorCode)) { errln("Set time error"); return; }
        original = cal->getTime(errorCode);
        us.remove();
        sdf->format(Formattable(tst,Formattable::kIsDate), us, errorCode);
        //logln("got: "+us);
        if (U_FAILURE(errorCode)) { errln("Get time error"); return; }
        if(original!=tst) {
            us.remove();
            sdf->format(Formattable(original, Formattable::kIsDate), us, errorCode);
            errln("FAIL: Parsed time doesn't match with regular");
            logln("expected "+us + " " + calToStr(*cal));
            us.remove();
            sdf->format(Formattable(tst, Formattable::kIsDate), us, errorCode);
            logln("got "+us + " " + calToStr(*tstres));
        }
        tstres->clear();
        tstres->set(UCAL_YEAR_WOY, cal->get(UCAL_YEAR_WOY, errorCode));
        tstres->set(UCAL_WEEK_OF_YEAR, cal->get(UCAL_WEEK_OF_YEAR, errorCode));
        tstres->set(UCAL_DOW_LOCAL, cal->get(UCAL_DOW_LOCAL, errorCode));
        if(cal->get(UCAL_YEAR, errorCode) != tstres->get(UCAL_YEAR, errorCode)) {
            errln("FAIL: Different Year!");
            logln((UnicodeString)"Expected "+cal->get(UCAL_YEAR, errorCode));
            logln((UnicodeString)"Got "+tstres->get(UCAL_YEAR, errorCode));
            return;
        }
        if(cal->get(UCAL_DAY_OF_YEAR, errorCode) != tstres->get(UCAL_DAY_OF_YEAR, errorCode)) {
            errln("FAIL: Different Day Of Year!");
            logln((UnicodeString)"Expected "+cal->get(UCAL_DAY_OF_YEAR, errorCode));
            logln((UnicodeString)"Got "+tstres->get(UCAL_DAY_OF_YEAR, errorCode));
            return;
        }
        //logln(calToStr(*cal));
        cal->add(UCAL_DATE, 1, errorCode);
        if (U_FAILURE(errorCode)) { errln("Add error"); return; }
        us.remove();
    }
    delete (tstres);
}
// -------------------------------------
  
void
CalendarTest::marchByDelta(Calendar* cal, int32_t delta)
{
    UErrorCode status = U_ZERO_ERROR;
    Calendar *cur = (Calendar*) cal->clone();
    int32_t initialDOW = cur->get(UCAL_DAY_OF_WEEK, status);
    if (U_FAILURE(status)) { errln("Calendar::get failed"); return; }
    int32_t DOW, newDOW = initialDOW;
    do {
        UnicodeString str;
        DOW = newDOW;
        logln(UnicodeString("DOW = ") + DOW + "  " + dateToString(cur->getTime(status), str));
        if (U_FAILURE(status)) { errln("Calendar::getTime failed"); return; }
        cur->add(UCAL_DAY_OF_WEEK, delta, status);
        if (U_FAILURE(status)) { errln("Calendar::add failed"); return; }
        newDOW = cur->get(UCAL_DAY_OF_WEEK, status);
        if (U_FAILURE(status)) { errln("Calendar::get failed"); return; }
        int32_t expectedDOW = 1 + (DOW + delta - 1) % 7;
        if (newDOW != expectedDOW) {
            errln(UnicodeString("Day of week should be ") + expectedDOW + " instead of " + newDOW +
                  " on " + dateToString(cur->getTime(status), str));
            if (U_FAILURE(status)) { errln("Calendar::getTime failed"); return; }
            return;
        }
    }
    while (newDOW != initialDOW);
    delete cur;
}

#define CHECK(status, msg) \
    if (U_FAILURE(status)) { \
        errcheckln(status, msg); \
        return; \
    }

void CalendarTest::TestWOY(void) {
    /*
      FDW = Mon, MDFW = 4:
         Sun Dec 26 1999, WOY 51
         Mon Dec 27 1999, WOY 52
         Tue Dec 28 1999, WOY 52
         Wed Dec 29 1999, WOY 52
         Thu Dec 30 1999, WOY 52
         Fri Dec 31 1999, WOY 52
         Sat Jan 01 2000, WOY 52 ***
         Sun Jan 02 2000, WOY 52 ***
         Mon Jan 03 2000, WOY 1
         Tue Jan 04 2000, WOY 1
         Wed Jan 05 2000, WOY 1
         Thu Jan 06 2000, WOY 1
         Fri Jan 07 2000, WOY 1
         Sat Jan 08 2000, WOY 1
         Sun Jan 09 2000, WOY 1
         Mon Jan 10 2000, WOY 2

      FDW = Mon, MDFW = 2:
         Sun Dec 26 1999, WOY 52
         Mon Dec 27 1999, WOY 1  ***
         Tue Dec 28 1999, WOY 1  ***
         Wed Dec 29 1999, WOY 1  ***
         Thu Dec 30 1999, WOY 1  ***
         Fri Dec 31 1999, WOY 1  ***
         Sat Jan 01 2000, WOY 1
         Sun Jan 02 2000, WOY 1
         Mon Jan 03 2000, WOY 2
         Tue Jan 04 2000, WOY 2
         Wed Jan 05 2000, WOY 2
         Thu Jan 06 2000, WOY 2
         Fri Jan 07 2000, WOY 2
         Sat Jan 08 2000, WOY 2
         Sun Jan 09 2000, WOY 2
         Mon Jan 10 2000, WOY 3
    */
 
    UnicodeString str;
    UErrorCode status = U_ZERO_ERROR;
    int32_t i;

    GregorianCalendar cal(status);
    SimpleDateFormat fmt(UnicodeString("EEE MMM dd yyyy', WOY' w"), status);
    if (failure(status, "Cannot construct calendar/format", TRUE)) return;

    UCalendarDaysOfWeek fdw = (UCalendarDaysOfWeek) 0;

    //for (int8_t pass=2; pass<=2; ++pass) {
    for (int8_t pass=1; pass<=2; ++pass) {
        switch (pass) {
        case 1:
            fdw = UCAL_MONDAY;
            cal.setFirstDayOfWeek(fdw);
            cal.setMinimalDaysInFirstWeek(4);
            fmt.adoptCalendar(cal.clone());
            break;
        case 2:
            fdw = UCAL_MONDAY;
            cal.setFirstDayOfWeek(fdw);
            cal.setMinimalDaysInFirstWeek(2);
            fmt.adoptCalendar(cal.clone());
            break;
        }

        //for (i=2; i<=6; ++i) {
        for (i=0; i<16; ++i) {
        UDate t, t2;
        int32_t t_y, t_woy, t_dow;
        cal.clear();
        cal.set(1999, UCAL_DECEMBER, 26 + i);
        fmt.format(t = cal.getTime(status), str.remove());
        CHECK(status, "Fail: getTime failed");
        logln(UnicodeString("* ") + str);
        int32_t dow = cal.get(UCAL_DAY_OF_WEEK, status);
        int32_t woy = cal.get(UCAL_WEEK_OF_YEAR, status);
        int32_t year = cal.get(UCAL_YEAR, status);
        int32_t mon = cal.get(UCAL_MONTH, status);
        logln(calToStr(cal));
        CHECK(status, "Fail: get failed");
        int32_t dowLocal = dow - fdw;
        if (dowLocal < 0) dowLocal += 7;
        dowLocal++;
        int32_t yearWoy = year;
        if (mon == UCAL_JANUARY) {
            if (woy >= 52) --yearWoy;
        } else {
            if (woy == 1) ++yearWoy;
        }

        // Basic fields->time check y/woy/dow
        // Since Y/WOY is ambiguous, we do a check of the fields,
        // not of the specific time.
        cal.clear();
        cal.set(UCAL_YEAR, year);
        cal.set(UCAL_WEEK_OF_YEAR, woy);
        cal.set(UCAL_DAY_OF_WEEK, dow);
        t_y = cal.get(UCAL_YEAR, status);
        t_woy = cal.get(UCAL_WEEK_OF_YEAR, status);
        t_dow = cal.get(UCAL_DAY_OF_WEEK, status);
        CHECK(status, "Fail: get failed");
        if (t_y != year || t_woy != woy || t_dow != dow) {
            str = "Fail: y/woy/dow fields->time => ";
            fmt.format(cal.getTime(status), str);
            errln(str);
            logln(calToStr(cal));
            logln("[get!=set] Y%d!=%d || woy%d!=%d || dow%d!=%d\n",
                  t_y, year, t_woy, woy, t_dow, dow);
        } else {
          logln("y/woy/dow fields->time OK");
        }

        // Basic fields->time check y/woy/dow_local
        // Since Y/WOY is ambiguous, we do a check of the fields,
        // not of the specific time.
        cal.clear();
        cal.set(UCAL_YEAR, year);
        cal.set(UCAL_WEEK_OF_YEAR, woy);
        cal.set(UCAL_DOW_LOCAL, dowLocal);
        t_y = cal.get(UCAL_YEAR, status);
        t_woy = cal.get(UCAL_WEEK_OF_YEAR, status);
        t_dow = cal.get(UCAL_DOW_LOCAL, status);
        CHECK(status, "Fail: get failed");
        if (t_y != year || t_woy != woy || t_dow != dowLocal) {
            str = "Fail: y/woy/dow_local fields->time => ";
            fmt.format(cal.getTime(status), str);
            errln(str);
        }

        // Basic fields->time check y_woy/woy/dow
        cal.clear();
        cal.set(UCAL_YEAR_WOY, yearWoy);
        cal.set(UCAL_WEEK_OF_YEAR, woy);
        cal.set(UCAL_DAY_OF_WEEK, dow);
        t2 = cal.getTime(status);
        CHECK(status, "Fail: getTime failed");
        if (t != t2) {
            str = "Fail: y_woy/woy/dow fields->time => ";
            fmt.format(t2, str);
            errln(str);
            logln(calToStr(cal));
            logln("%.f != %.f\n", t, t2);
        } else {
          logln("y_woy/woy/dow OK");
        }

        // Basic fields->time check y_woy/woy/dow_local
        cal.clear();
        cal.set(UCAL_YEAR_WOY, yearWoy);
        cal.set(UCAL_WEEK_OF_YEAR, woy);
        cal.set(UCAL_DOW_LOCAL, dowLocal);
        t2 = cal.getTime(status);
        CHECK(status, "Fail: getTime failed");
        if (t != t2) {
            str = "Fail: y_woy/woy/dow_local fields->time => ";
            fmt.format(t2, str);
            errln(str);
        }

        logln("Testing DOW_LOCAL.. dow%d\n", dow);
        // Make sure DOW_LOCAL disambiguates over DOW
        int32_t wrongDow = dow - 3;
        if (wrongDow < 1) wrongDow += 7;
        cal.setTime(t, status);
        cal.set(UCAL_DAY_OF_WEEK, wrongDow);
        cal.set(UCAL_DOW_LOCAL, dowLocal);
        t2 = cal.getTime(status);
        CHECK(status, "Fail: set/getTime failed");
        if (t != t2) {
            str = "Fail: DOW_LOCAL fields->time => ";
            fmt.format(t2, str);
            errln(str);
            logln(calToStr(cal));
            logln("%.f :   DOW%d, DOW_LOCAL%d -> %.f\n",
                  t, wrongDow, dowLocal, t2);
        }

        // Make sure DOW disambiguates over DOW_LOCAL
        int32_t wrongDowLocal = dowLocal - 3;
        if (wrongDowLocal < 1) wrongDowLocal += 7;
        cal.setTime(t, status);
        cal.set(UCAL_DOW_LOCAL, wrongDowLocal);
        cal.set(UCAL_DAY_OF_WEEK, dow);
        t2 = cal.getTime(status);
        CHECK(status, "Fail: set/getTime failed");
        if (t != t2) {
            str = "Fail: DOW       fields->time => ";
            fmt.format(t2, str);
            errln(str);
        }

        // Make sure YEAR_WOY disambiguates over YEAR
        cal.setTime(t, status);
        cal.set(UCAL_YEAR, year - 2);
        cal.set(UCAL_YEAR_WOY, yearWoy);
        t2 = cal.getTime(status);
        CHECK(status, "Fail: set/getTime failed");
        if (t != t2) {
            str = "Fail: YEAR_WOY  fields->time => ";
            fmt.format(t2, str);
            errln(str);
        }

        // Make sure YEAR disambiguates over YEAR_WOY
        cal.setTime(t, status);
        cal.set(UCAL_YEAR_WOY, yearWoy - 2);
        cal.set(UCAL_YEAR, year);
        t2 = cal.getTime(status);
        CHECK(status, "Fail: set/getTime failed");
        if (t != t2) {
            str = "Fail: YEAR      fields->time => ";
            fmt.format(t2, str);
            errln(str);
        }
    }
    }

    /*
      FDW = Mon, MDFW = 4:
         Sun Dec 26 1999, WOY 51
         Mon Dec 27 1999, WOY 52
         Tue Dec 28 1999, WOY 52
         Wed Dec 29 1999, WOY 52
         Thu Dec 30 1999, WOY 52
         Fri Dec 31 1999, WOY 52
         Sat Jan 01 2000, WOY 52
         Sun Jan 02 2000, WOY 52
    */

    // Roll the DOW_LOCAL within week 52
    for (i=27; i<=33; ++i) {
        int32_t amount;
        for (amount=-7; amount<=7; ++amount) {
            str = "roll(";
            cal.set(1999, UCAL_DECEMBER, i);
            UDate t, t2;
            fmt.format(cal.getTime(status), str);
            CHECK(status, "Fail: getTime failed");
            str += UnicodeString(", ") + amount + ") = ";

            cal.roll(UCAL_DOW_LOCAL, amount, status);
            CHECK(status, "Fail: roll failed");

            t = cal.getTime(status);
            int32_t newDom = i + amount;
            while (newDom < 27) newDom += 7;
            while (newDom > 33) newDom -= 7;
            cal.set(1999, UCAL_DECEMBER, newDom);
            t2 = cal.getTime(status);
            CHECK(status, "Fail: getTime failed");
            fmt.format(t, str);

            if (t != t2) {
                str.append(", exp ");
                fmt.format(t2, str);
                errln(str);
            } else {
                logln(str);
            }
        }
    }
}

void CalendarTest::TestYWOY()
{
   UnicodeString str;
   UErrorCode status = U_ZERO_ERROR;
   
   GregorianCalendar cal(status);
   if (failure(status, "construct GregorianCalendar", TRUE)) return;

   cal.setFirstDayOfWeek(UCAL_SUNDAY);
   cal.setMinimalDaysInFirstWeek(1);

   logln("Setting:  ywoy=2004, woy=1, dow=MONDAY");
   cal.clear();
   cal.set(UCAL_YEAR_WOY,2004);
   cal.set(UCAL_WEEK_OF_YEAR,1);
   cal.set(UCAL_DAY_OF_WEEK, UCAL_MONDAY);
   
   logln(calToStr(cal));
   if(cal.get(UCAL_YEAR, status) != 2003) {
     errln("year not 2003");
   }

   logln("+ setting DOW to THURSDAY");
   cal.clear();
   cal.set(UCAL_YEAR_WOY,2004);
   cal.set(UCAL_WEEK_OF_YEAR,1);
   cal.set(UCAL_DAY_OF_WEEK, UCAL_THURSDAY);
   
   logln(calToStr(cal));
   if(cal.get(UCAL_YEAR, status) != 2004) {
     errln("year not 2004");
   }

   logln("+ setting DOW_LOCAL to 1");
   cal.clear();
   cal.set(UCAL_YEAR_WOY,2004);
   cal.set(UCAL_WEEK_OF_YEAR,1);
   cal.set(UCAL_DAY_OF_WEEK, UCAL_THURSDAY);
   cal.set(UCAL_DOW_LOCAL, 1);
   
   logln(calToStr(cal));
   if(cal.get(UCAL_YEAR, status) != 2003) {
     errln("year not 2003");
   }

   cal.setFirstDayOfWeek(UCAL_MONDAY);
   cal.setMinimalDaysInFirstWeek(4);
   UDate t = 946713600000.;
   cal.setTime(t, status);
   cal.set(UCAL_DAY_OF_WEEK, 4);
   cal.set(UCAL_DOW_LOCAL, 6);
   if(cal.getTime(status) != t) {
     logln(calToStr(cal));
     errln("FAIL:  DOW_LOCAL did not take precedence");
   }

}

void CalendarTest::TestJD()
{
  int32_t jd;
  static const int32_t kEpochStartAsJulianDay = 2440588;
  UErrorCode status = U_ZERO_ERROR;
  GregorianCalendar cal(status);
  if (failure(status, "construct GregorianCalendar", TRUE)) return;
  cal.setTimeZone(*TimeZone::getGMT());
  cal.clear();
  jd = cal.get(UCAL_JULIAN_DAY, status);
  if(jd != kEpochStartAsJulianDay) {
    errln("Wanted JD of %d at time=0, [epoch 1970] but got %d\n", kEpochStartAsJulianDay, jd);
  } else {
    logln("Wanted JD of %d at time=0, [epoch 1970], got %d\n", kEpochStartAsJulianDay, jd);
  }
  
  cal.setTime(Calendar::getNow(), status);
  cal.clear();
  cal.set(UCAL_JULIAN_DAY, kEpochStartAsJulianDay);
  UDate epochTime = cal.getTime(status);
  if(epochTime != 0) {
    errln("Wanted time of 0 at jd=%d, got %.1lf\n", kEpochStartAsJulianDay, epochTime);
  } else {
    logln("Wanted time of 0 at jd=%d, got %.1lf\n", kEpochStartAsJulianDay, epochTime);
  }

}

// make sure the ctestfw utilities are in sync with the Calendar
void CalendarTest::TestDebug()
{
	for(int32_t  t=0;t<=UDBG_ENUM_COUNT;t++) {
		int32_t count = udbg_enumCount((UDebugEnumType)t);
		if(count == -1) {
			logln("enumCount(%d) returned -1", count);
			continue;
		}
	    for(int32_t i=0;i<=count;i++) {
	  	  if(t<=UDBG_HIGHEST_CONTIGUOUS_ENUM && i<count) {
	  		  if( i!=udbg_enumArrayValue((UDebugEnumType)t, i)) {
	  			  errln("FAIL: udbg_enumArrayValue(%d,%d) returned %d, expected %d", t, i, udbg_enumArrayValue((UDebugEnumType)t,i), i);
	  		  }
	  	  } else {
	  		  logln("Testing count+1:");
	  	  }
                  const char *name = udbg_enumName((UDebugEnumType)t,i);
                  if(name==NULL) {
                          if(i==count || t>UDBG_HIGHEST_CONTIGUOUS_ENUM  ) {
                                logln(" null name - expected.\n");
                          } else {
                                errln("FAIL: udbg_enumName(%d,%d) returned NULL", t, i);
                          }
                          name = "(null)";
                  }
		  logln("udbg_enumArrayValue(%d,%d) = %s, returned %d", t, i, 
				  	name, udbg_enumArrayValue((UDebugEnumType)t,i));
	  	  logln("udbg_enumString = " + udbg_enumString((UDebugEnumType)t,i));
	    }
	    if(udbg_enumExpectedCount((UDebugEnumType)t) != count && t<=UDBG_HIGHEST_CONTIGUOUS_ENUM) {
	  	  errln("FAIL: udbg_enumExpectedCount(%d): %d, != UCAL_FIELD_COUNT=%d ", t, udbg_enumExpectedCount((UDebugEnumType)t), count);
	    } else {
	  	  logln("udbg_ucal_fieldCount: %d, UCAL_FIELD_COUNT=udbg_enumCount %d ", udbg_enumExpectedCount((UDebugEnumType)t), count);
	    }
	}
}


#undef CHECK

// List of interesting locales
const char *CalendarTest::testLocaleID(int32_t i)
{
  switch(i) {
  case 0: return "he_IL@calendar=hebrew";
  case 1: return "en_US@calendar=hebrew";
  case 2: return "fr_FR@calendar=hebrew";
  case 3: return "fi_FI@calendar=hebrew";
  case 4: return "nl_NL@calendar=hebrew";
  case 5: return "hu_HU@calendar=hebrew";
  case 6: return "nl_BE@currency=MTL;calendar=islamic";
  case 7: return "th_TH_TRADITIONAL@calendar=gregorian";
  case 8: return "ar_JO@calendar=islamic-civil";
  case 9: return "fi_FI@calendar=islamic";
  case 10: return "fr_CH@calendar=islamic-civil";
  case 11: return "he_IL@calendar=islamic-civil";
  case 12: return "hu_HU@calendar=buddhist";
  case 13: return "hu_HU@calendar=islamic";
  case 14: return "en_US@calendar=japanese";
  default: return NULL;
  }
}

int32_t CalendarTest::testLocaleCount()
{
  static int32_t gLocaleCount = -1;
  if(gLocaleCount < 0) {
    int32_t i;
    for(i=0;testLocaleID(i) != NULL;i++) {
      ;
    }
    gLocaleCount = i;
  }
  return gLocaleCount;
}

static UDate doMinDateOfCalendar(Calendar* adopt, UBool &isGregorian, UErrorCode& status) {
  if(U_FAILURE(status)) return 0.0;
  
  adopt->clear();
  adopt->set(UCAL_EXTENDED_YEAR, adopt->getActualMinimum(UCAL_EXTENDED_YEAR, status));
  UDate ret = adopt->getTime(status);
  isGregorian = dynamic_cast<GregorianCalendar*>(adopt) != NULL;
  delete adopt;
  return ret;
}

UDate CalendarTest::minDateOfCalendar(const Locale& locale, UBool &isGregorian, UErrorCode& status) {
  if(U_FAILURE(status)) return 0.0;
  return doMinDateOfCalendar(Calendar::createInstance(locale, status), isGregorian, status);
}

UDate CalendarTest::minDateOfCalendar(const Calendar& cal, UBool &isGregorian, UErrorCode& status) {
  if(U_FAILURE(status)) return 0.0;
  return doMinDateOfCalendar(cal.clone(), isGregorian, status);
}

void CalendarTest::Test6703()
{
    UErrorCode status = U_ZERO_ERROR;
    Calendar *cal;

    Locale loc1("en@calendar=fubar");
    cal = Calendar::createInstance(loc1, status);
    if (failure(status, "Calendar::createInstance", TRUE)) return;
    delete cal;
  
    status = U_ZERO_ERROR;
    Locale loc2("en");
    cal = Calendar::createInstance(loc2, status);
    if (failure(status, "Calendar::createInstance")) return;
    delete cal;

    status = U_ZERO_ERROR;
    Locale loc3("en@calendar=roc");
    cal = Calendar::createInstance(loc3, status);
    if (failure(status, "Calendar::createInstance")) return;
    delete cal;

    return;
}

void CalendarTest::Test3785()
{
    UErrorCode status = U_ZERO_ERROR; 
    UnicodeString uzone = UNICODE_STRING_SIMPLE("Europe/Paris");
    UnicodeString exp1 = UNICODE_STRING_SIMPLE("Mon 30 Jumada II 1433 AH, 01:47:03");
    UnicodeString exp2 = UNICODE_STRING_SIMPLE("Mon 1 Rajab 1433 AH, 01:47:04");

    LocalUDateFormatPointer df(udat_open(UDAT_NONE, UDAT_NONE, "en@calendar=islamic", uzone.getTerminatedBuffer(), 
                                         uzone.length(), NULL, 0, &status));
    if (df.isNull() || U_FAILURE(status)) return;

    UChar upattern[64];   
    u_uastrcpy(upattern, "EEE d MMMM y G, HH:mm:ss"); 
    udat_applyPattern(df.getAlias(), FALSE, upattern, u_strlen(upattern));

    UChar ubuffer[1024]; 
    UDate ud0 = 1337557623000.0;

    status = U_ZERO_ERROR; 
    udat_format(df.getAlias(), ud0, ubuffer, 1024, NULL, &status); 
    if (U_FAILURE(status)) {
        errln("Error formatting date 1\n");
        return; 
    }
    //printf("formatted: '%s'\n", mkcstr(ubuffer));

    UnicodeString act1(ubuffer);
    if ( act1 != exp1 ) {
        errln("Unexpected result from date 1 format\n"); 
    }
    ud0 += 1000.0; // add one second

    status = U_ZERO_ERROR; 
    udat_format(df.getAlias(), ud0, ubuffer, 1024, NULL, &status); 
    if (U_FAILURE(status)) {
        errln("Error formatting date 2\n");
        return; 
    }
    //printf("formatted: '%s'\n", mkcstr(ubuffer));
    UnicodeString act2(ubuffer);
    if ( act2 != exp2 ) {
        errln("Unexpected result from date 2 format\n");
    }

    return;
}

void CalendarTest::Test1624() {
    UErrorCode status = U_ZERO_ERROR;
    Locale loc("he_IL@calendar=hebrew");
    HebrewCalendar hc(loc,status);

    for (int32_t year = 5600; year < 5800; year++ ) {
    
        for (int32_t month = HebrewCalendar::TISHRI; month <= HebrewCalendar::ELUL; month++) {
            // skip the adar 1 month if year is not a leap year
            if (HebrewCalendar::isLeapYear(year) == FALSE && month == HebrewCalendar::ADAR_1) {
                continue;
            }
            int32_t day = 15;
            hc.set(year,month,day);
            int32_t dayHC = hc.get(UCAL_DATE,status);
            int32_t monthHC = hc.get(UCAL_MONTH,status);
            int32_t yearHC = hc.get(UCAL_YEAR,status);

            if (failure(status, "HebrewCalendar.get()", TRUE)) continue;

            if (dayHC != day) {
                errln(" ==> day %d incorrect, should be: %d\n",dayHC,day);
                break;
            }
            if (monthHC != month) {
                errln(" ==> month %d incorrect, should be: %d\n",monthHC,month);
                break;
            }
            if (yearHC != year) {
                errln(" ==> day %d incorrect, should be: %d\n",yearHC,year);
                break;
            }
        }
    }
    return;
}

#endif /* #if !UCONFIG_NO_FORMATTING */

//eof
