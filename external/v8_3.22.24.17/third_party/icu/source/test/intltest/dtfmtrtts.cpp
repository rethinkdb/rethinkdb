/***********************************************************************
 * COPYRIGHT: 
 * Copyright (c) 1997-2010, International Business Machines Corporation
 * and others. All Rights Reserved.
 ***********************************************************************/
 
#include "unicode/utypes.h"

#if !UCONFIG_NO_FORMATTING

#include "unicode/datefmt.h"
#include "unicode/smpdtfmt.h"
#include "unicode/gregocal.h"
#include "dtfmtrtts.h"
#include "caltest.h"

#include <stdio.h>
#include <string.h>

// *****************************************************************************
// class DateFormatRoundTripTest
// *****************************************************************************

// Useful for turning up subtle bugs: Change the following to TRUE, recompile,
// and run while at lunch.
// Warning -- makes test run infinite loop!!!
#ifndef INFINITE
#define INFINITE 0
#endif

static const UVersionInfo ICU_452 = {4,5,2,0};

// Define this to test just a single locale
//#define TEST_ONE_LOC  "cs_CZ"

// If SPARSENESS is > 0, we don't run each exhaustive possibility.
// There are 24 total possible tests per each locale.  A SPARSENESS
// of 12 means we run half of them.  A SPARSENESS of 23 means we run
// 1 of them.  SPARSENESS _must_ be in the range 0..23.
int32_t DateFormatRoundTripTest::SPARSENESS = 0;
int32_t DateFormatRoundTripTest::TRIALS = 4;
int32_t DateFormatRoundTripTest::DEPTH = 5;

DateFormatRoundTripTest::DateFormatRoundTripTest() : dateFormat(0) {
}

DateFormatRoundTripTest::~DateFormatRoundTripTest() {
    delete dateFormat;
}

#define CASE(id,test) case id: name = #test; if (exec) { logln(#test "---"); logln((UnicodeString)""); test(); } break;

void 
DateFormatRoundTripTest::runIndexedTest( int32_t index, UBool exec, const char* &name, char* par )
{
    optionv = (par && *par=='v');
    switch (index) {
        CASE(0,TestDateFormatRoundTrip)
        CASE(1, TestCentury)
        default: name = ""; break;
    }
}

UBool 
DateFormatRoundTripTest::failure(UErrorCode status, const char* msg)
{
    if(U_FAILURE(status)) {
        errln(UnicodeString("FAIL: ") + msg + " failed, error " + u_errorName(status));
        return TRUE;
    }

    return FALSE;
}

UBool 
DateFormatRoundTripTest::failure(UErrorCode status, const char* msg, const UnicodeString& str)
{
    if(U_FAILURE(status)) {
        UnicodeString escaped;
        escape(str,escaped);
        errln(UnicodeString("FAIL: ") + msg + " failed, error " + u_errorName(status) + ", str=" + escaped);
        return TRUE;
    }

    return FALSE;
}

void DateFormatRoundTripTest::TestCentury()
{
    UErrorCode status = U_ZERO_ERROR;
    Locale locale("es_PA");
    UnicodeString pattern = "MM/dd/yy hh:mm:ss a z";
    SimpleDateFormat fmt(pattern, locale, status);
    if (U_FAILURE(status)) {
        dataerrln("Fail: construct SimpleDateFormat: %s", u_errorName(status));
        return;
    }
    UDate date[] = {-55018555891590.05, 0, 0};
    UnicodeString result[2];

    fmt.format(date[0], result[0]);
    date[1] = fmt.parse(result[0], status);
    fmt.format(date[1], result[1]);
    date[2] = fmt.parse(result[1], status);

    /* This test case worked OK by accident before.  date[1] != date[0],
     * because we use -80/+20 year window for 2-digit year parsing.
     * (date[0] is in year 1926, date[1] is in year 2026.)  result[1] set
     * by the first format call returns "07/13/26 07:48:28 p.m. PST",
     * which is correct, because DST was not used in year 1926 in zone
     * America/Los_Angeles.  When this is parsed, date[1] becomes a time
     * in 2026, which is "07/13/26 08:48:28 p.m. PDT".  There was a zone
     * offset calculation bug that observed DST in 1926, which was resolved.
     * Before the bug was resolved, result[0] == result[1] was true,
     * but after the bug fix, the expected result is actually
     * result[0] != result[1]. -Yoshito
     */
    /* TODO: We need to review this code and clarify what we really
     * want to test here.
     */
    //if (date[1] != date[2] || result[0] != result[1]) {
    if (date[1] != date[2]) {
        errln("Round trip failure: \"%S\" (%f), \"%S\" (%f)", result[0].getBuffer(), date[1], result[1].getBuffer(), date[2]);
    }
}

// ==

void DateFormatRoundTripTest::TestDateFormatRoundTrip() 
{
    UErrorCode status = U_ZERO_ERROR;

    getFieldCal = Calendar::createInstance(status);
    if (U_FAILURE(status)) {
        dataerrln("Fail: Calendar::createInstance: %s", u_errorName(status));
        return;
    }


    int32_t locCount = 0;
    const Locale *avail = DateFormat::getAvailableLocales(locCount);
    logln("DateFormat available locales: %d", locCount);
    if(quick) {
        SPARSENESS = 18;
        logln("Quick mode: only testing SPARSENESS = 18");
    }
    TimeZone *tz = TimeZone::createDefault();
    UnicodeString temp;
    logln("Default TimeZone:             " + tz->getID(temp));
    delete tz;

#ifdef TEST_ONE_LOC // define this to just test ONE locale.
    Locale loc(TEST_ONE_LOC);
    test(loc);
#if INFINITE
    for(;;) {
      test(loc);
    }
#endif

#else
# if INFINITE
    // Special infinite loop test mode for finding hard to reproduce errors
    Locale loc = Locale::getDefault();
    logln("ENTERING INFINITE TEST LOOP FOR Locale: " + loc.getDisplayName(temp));
    for(;;) 
        test(loc);
# else
    test(Locale::getDefault());

#if 1
    // installed locales
    for (int i=0; i < locCount; ++i) {
            test(avail[i]);
    }
#endif

#if 1
    // special locales
    int32_t jCount = CalendarTest::testLocaleCount();
    for (int32_t j=0; j < jCount; ++j) {
        test(Locale(CalendarTest::testLocaleID(j)));
    }
#endif

# endif
#endif

    delete getFieldCal;
}

static const char *styleName(DateFormat::EStyle s)
{
    switch(s)
    {
    case DateFormat::SHORT: return "SHORT";
    case DateFormat::MEDIUM: return "MEDIUM";
    case DateFormat::LONG: return "LONG";
    case DateFormat::FULL: return "FULL";
//  case DateFormat::DEFAULT: return "DEFAULT";
    case DateFormat::DATE_OFFSET: return "DATE_OFFSET";
    case DateFormat::NONE: return "NONE";
    case DateFormat::DATE_TIME: return "DATE_TIME";
    default: return "Unknown";
    }
}

void DateFormatRoundTripTest::test(const Locale& loc) 
{
    UnicodeString temp;
#if !INFINITE
    logln("Locale: " + loc.getDisplayName(temp));
#endif

    // Total possibilities = 24
    //  4 date
    //  4 time
    //  16 date-time
    UBool TEST_TABLE [24];//= new boolean[24];
    int32_t i = 0;
    for(i = 0; i < 24; ++i) 
        TEST_TABLE[i] = TRUE;

    // If we have some sparseness, implement it here.  Sparseness decreases
    // test time by eliminating some tests, up to 23.
    for(i = 0; i < SPARSENESS; ) {
        int random = (int)(randFraction() * 24);
        if (random >= 0 && random < 24 && TEST_TABLE[i]) {
            TEST_TABLE[i] = FALSE;
            ++i;
        }
    }

    int32_t itable = 0;
    int32_t style = 0;
    for(style = DateFormat::FULL; style <= DateFormat::SHORT; ++style) {
        if(TEST_TABLE[itable++]) {
            logln("Testing style " + UnicodeString(styleName((DateFormat::EStyle)style)));
            DateFormat *df = DateFormat::createDateInstance((DateFormat::EStyle)style, loc);
            if(df == NULL) {
              errln(UnicodeString("Could not DF::createDateInstance ") + UnicodeString(styleName((DateFormat::EStyle)style)) +      " Locale: " + loc.getDisplayName(temp));
            } else {
              test(df, loc);
              delete df;
            }
        }
    }
    
    for(style = DateFormat::FULL; style <= DateFormat::SHORT; ++style) {
        if (TEST_TABLE[itable++]) {
          logln("Testing style " + UnicodeString(styleName((DateFormat::EStyle)style)));
            DateFormat *df = DateFormat::createTimeInstance((DateFormat::EStyle)style, loc);
            if(df == NULL) {
              errln(UnicodeString("Could not DF::createTimeInstance ") + UnicodeString(styleName((DateFormat::EStyle)style)) + " Locale: " + loc.getDisplayName(temp));
            } else {
              test(df, loc, TRUE);
              delete df;
            }
        }
    }
    
    for(int32_t dstyle = DateFormat::FULL; dstyle <= DateFormat::SHORT; ++dstyle) {
        for(int32_t tstyle = DateFormat::FULL; tstyle <= DateFormat::SHORT; ++tstyle) {
            if(TEST_TABLE[itable++]) {
                logln("Testing dstyle" + UnicodeString(styleName((DateFormat::EStyle)dstyle)) + ", tstyle" + UnicodeString(styleName((DateFormat::EStyle)tstyle)) );
                DateFormat *df = DateFormat::createDateTimeInstance((DateFormat::EStyle)dstyle, (DateFormat::EStyle)tstyle, loc);
                if(df == NULL) {
                    dataerrln(UnicodeString("Could not DF::createDateTimeInstance ") + UnicodeString(styleName((DateFormat::EStyle)dstyle)) + ", tstyle" + UnicodeString(styleName((DateFormat::EStyle)tstyle))    + "Locale: " + loc.getDisplayName(temp));
                } else {
                    test(df, loc);
                    delete df;
                }
            }
        }
    }
}

void DateFormatRoundTripTest::test(DateFormat *fmt, const Locale &origLocale, UBool timeOnly) 
{
    UnicodeString pat;
    if(fmt->getDynamicClassID() != SimpleDateFormat::getStaticClassID()) {
        errln("DateFormat wasn't a SimpleDateFormat");
        return;
    }
    
    UBool isGregorian = FALSE;
    UErrorCode minStatus = U_ZERO_ERROR;
    UDate minDate = CalendarTest::minDateOfCalendar(*fmt->getCalendar(), isGregorian, minStatus);
    if(U_FAILURE(minStatus)) {
      errln((UnicodeString)"Failure getting min date for " + origLocale.getName());
      return;
    } 
    //logln(UnicodeString("Min date is ") + fullFormat(minDate)  + " for " + origLocale.getName());

    pat = ((SimpleDateFormat*)fmt)->toPattern(pat);

    // NOTE TO MAINTAINER
    // This indexOf check into the pattern needs to be refined to ignore
    // quoted characters.  Currently, this isn't a problem with the locale
    // patterns we have, but it may be a problem later.

    UBool hasEra = (pat.indexOf(UnicodeString("G")) != -1);
    UBool hasZoneDisplayName = (pat.indexOf(UnicodeString("z")) != -1) || (pat.indexOf(UnicodeString("v")) != -1) 
        || (pat.indexOf(UnicodeString("V")) != -1);

    // Because patterns contain incomplete data representing the Date,
    // we must be careful of how we do the roundtrip.  We start with
    // a randomly generated Date because they're easier to generate.
    // From this we get a string.  The string is our real starting point,
    // because this string should parse the same way all the time.  Note
    // that it will not necessarily parse back to the original date because
    // of incompleteness in patterns.  For example, a time-only pattern won't
    // parse back to the same date.

    //try {
        for(int i = 0; i < TRIALS; ++i) {
            UDate *d                = new UDate    [DEPTH];
            UnicodeString *s    = new UnicodeString[DEPTH];

            if(isGregorian == TRUE) {
              d[0] = generateDate();
            } else {
              d[0] = generateDate(minDate);
            }

            UErrorCode status = U_ZERO_ERROR;

            // We go through this loop until we achieve a match or until
            // the maximum loop count is reached.  We record the points at
            // which the date and the string starts to match.  Once matching
            // starts, it should continue.
            int loop;
            int dmatch = 0; // d[dmatch].getTime() == d[dmatch-1].getTime()
            int smatch = 0; // s[smatch].equals(s[smatch-1])
            for(loop = 0; loop < DEPTH; ++loop) {
                if (loop > 0)  {
                    d[loop] = fmt->parse(s[loop-1], status);
                    failure(status, "fmt->parse", s[loop-1]+" in locale: " + origLocale.getName());
                    status = U_ZERO_ERROR; /* any error would have been reported */
                }

                s[loop] = fmt->format(d[loop], s[loop]);
                
                // For displaying which date is being tested
                //logln(s[loop] + " = " + fullFormat(d[loop]));
                
                if(s[loop].length() == 0) {
                  errln("FAIL: fmt->format gave 0-length string in " + pat + " with number " + d[loop] + " in locale " + origLocale.getName());
                }

                if(loop > 0) {
                    if(smatch == 0) {
                        UBool match = s[loop] == s[loop-1];
                        if(smatch == 0) {
                            if(match) 
                                smatch = loop;
                        }
                        else if( ! match) 
                            errln("FAIL: String mismatch after match");
                    }

                    if(dmatch == 0) {
                        // {sfb} watch out here, this might not work
                        UBool match = d[loop]/*.getTime()*/ == d[loop-1]/*.getTime()*/;
                        if(dmatch == 0) {
                            if(match) 
                                dmatch = loop;
                        }
                        else if( ! match) 
                            errln("FAIL: Date mismatch after match");
                    }

                    if(smatch != 0 && dmatch != 0) 
                        break;
                }
            }
            // At this point loop == DEPTH if we've failed, otherwise loop is the
            // max(smatch, dmatch), that is, the index at which we have string and
            // date matching.

            // Date usually matches in 2.  Exceptions handled below.
            int maxDmatch = 2;
            int maxSmatch = 1;
            if (dmatch > maxDmatch) {
                // Time-only pattern with zone information and a starting date in PST.
                if(timeOnly && hasZoneDisplayName) {
                    int32_t startRaw, startDst;
                    fmt->getTimeZone().getOffset(d[0], FALSE, startRaw, startDst, status);
                    failure(status, "TimeZone::getOffset");
                    // if the start offset is greater than the offset on Jan 1, 1970
                    // in PST, then need one more round trip.  There are two cases
                    // fall into this category.  The start date is 1) DST or
                    // 2) LMT (GMT-07:52:58).
                    if (startRaw + startDst > -28800000) {
                        maxDmatch = 3;
                        maxSmatch = 2;
                    }
                }
            }

            // String usually matches in 1.  Exceptions are checked for here.
            if(smatch > maxSmatch) { // Don't compute unless necessary
                UBool in0;
                // Starts in BC, with no era in pattern
                if( ! hasEra && getField(d[0], UCAL_ERA) == GregorianCalendar::BC)
                    maxSmatch = 2;
                // Starts in DST, no year in pattern
                else if((in0=fmt->getTimeZone().inDaylightTime(d[0], status)) && ! failure(status, "gettingDaylightTime") &&
                         pat.indexOf(UnicodeString("yyyy")) == -1)
                    maxSmatch = 2;
                // If we start not in DST, but transition into DST
                else if (!in0 &&
                         fmt->getTimeZone().inDaylightTime(d[1], status) && !failure(status, "gettingDaylightTime"))
                    maxSmatch = 2;
                // Two digit year with no time zone change,
                // unless timezone isn't used or we aren't close to the DST changover
                else if (pat.indexOf(UnicodeString("y")) != -1
                        && pat.indexOf(UnicodeString("yyyy")) == -1
                        && getField(d[0], UCAL_YEAR)
                            != getField(d[dmatch], UCAL_YEAR)
                        && !failure(status, "error status [smatch>maxSmatch]")
                        && ((hasZoneDisplayName
                         && (fmt->getTimeZone().inDaylightTime(d[0], status)
                                == fmt->getTimeZone().inDaylightTime(d[dmatch], status)
                            || getField(d[0], UCAL_MONTH) == UCAL_APRIL
                            || getField(d[0], UCAL_MONTH) == UCAL_OCTOBER))
                         || !hasZoneDisplayName)
                         )
                {
                    maxSmatch = 2;
                }
                // If zone display name is used, fallback format might be used before 1970
                else if (hasZoneDisplayName && d[0] < 0) {
                    maxSmatch = 2;
                }
            }

            if(dmatch > maxDmatch || smatch > maxSmatch) { // Special case for Japanese and Islamic (could have large negative years)
              const char *type = fmt->getCalendar()->getType();
              if(!strcmp(type,"japanese") || (!strcmp(type,"buddhist"))) {
                maxSmatch = 4;
                maxDmatch = 4;
              }
            }

            // Use @v to see verbose results on successful cases
            UBool fail = (dmatch > maxDmatch || smatch > maxSmatch);
            if (optionv || fail) {
                if (fail) {
                    errln(UnicodeString("\nFAIL: Pattern: ") + pat +
                          " in Locale: " + origLocale.getName());
                } else {
                    errln(UnicodeString("\nOk: Pattern: ") + pat +
                          " in Locale: " + origLocale.getName());
                }
                
                logln("Date iters until match=%d (max allowed=%d), string iters until match=%d (max allowed=%d)",
                      dmatch,maxDmatch, smatch, maxSmatch);

                for(int j = 0; j <= loop && j < DEPTH; ++j) {
                    UnicodeString temp;
                    FieldPosition pos(FieldPosition::DONT_CARE);
                    errln((j>0?" P> ":"    ") + fullFormat(d[j]) + " F> " +
                          escape(s[j], temp) + UnicodeString(" d=") + d[j] + 
                          (j > 0 && d[j]/*.getTime()*/==d[j-1]/*.getTime()*/?" d==":"") +
                          (j > 0 && s[j] == s[j-1]?" s==":""));
                }
            }
            delete[] d;
            delete[] s;
        }
    /*}
    catch (ParseException e) {
        errln("Exception: " + e.getMessage());
        logln(e.toString());
    }*/
}

const UnicodeString& DateFormatRoundTripTest::fullFormat(UDate d) {
    UErrorCode ec = U_ZERO_ERROR;
    if (dateFormat == 0) {
        dateFormat = new SimpleDateFormat((UnicodeString)"EEE MMM dd HH:mm:ss.SSS zzz yyyy G", ec);
        if (U_FAILURE(ec) || dateFormat == 0) {
            fgStr = "[FAIL: SimpleDateFormat constructor]";
            delete dateFormat;
            dateFormat = 0;
            return fgStr;
        }
    }
    fgStr.truncate(0);
    dateFormat->format(d, fgStr);
    return fgStr;
}

/**
 * Return a field of the given date
 */
int32_t DateFormatRoundTripTest::getField(UDate d, int32_t f) {
    // Should be synchronized, but we're single threaded so it's ok
    UErrorCode status = U_ZERO_ERROR;
    getFieldCal->setTime(d, status);
    failure(status, "getfieldCal->setTime");
    int32_t ret = getFieldCal->get((UCalendarDateFields)f, status);
    failure(status, "getfieldCal->get");
    return ret;
}

UnicodeString& DateFormatRoundTripTest::escape(const UnicodeString& src, UnicodeString& dst ) 
{
    dst.remove();
    for (int32_t i = 0; i < src.length(); ++i) {
        UChar c = src[i];
        if(c < 0x0080) 
            dst += c;
        else {
            dst += UnicodeString("[");
            char buf [8];
            sprintf(buf, "%#x", c);
            dst += UnicodeString(buf);
            dst += UnicodeString("]");
        }
    }

    return dst;
}

#define U_MILLIS_PER_YEAR (365.25 * 24 * 60 * 60 * 1000)

UDate DateFormatRoundTripTest::generateDate(UDate minDate)
{
  // Bring range in conformance to generateDate() below.
  if(minDate < (U_MILLIS_PER_YEAR * -(4000-1970))) {
    minDate = (U_MILLIS_PER_YEAR * -(4000-1970));
  }
  for(int i=0;i<8;i++) {
    double a = randFraction();
    
    // Range from (min) to  (8000-1970) AD
    double dateRange = (0.0 - minDate) + (U_MILLIS_PER_YEAR + (8000-1970));
    
    a *= dateRange;

    // Now offset from minDate
    a += minDate;
   
    // Last sanity check
    if(a>=minDate) {
      return a;
    }
  }
  return minDate;
}

UDate DateFormatRoundTripTest::generateDate() 
{
    double a = randFraction();
    
    // Now 'a' ranges from 0..1; scale it to range from 0 to 8000 years
    a *= 8000;
    
    // Range from (4000-1970) BC to (8000-1970) AD
    a -= 4000;
    
    // Now scale up to ms
    a *= 365.25 * 24 * 60 * 60 * 1000;

    //return new Date((long)a);
    return a;
}

#endif /* #if !UCONFIG_NO_FORMATTING */

//eof
