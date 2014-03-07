/********************************************************************
 * COPYRIGHT:
 * Copyright (c) 1997-2010, International Business Machines
 * Corporation and others. All Rights Reserved.
 ********************************************************************/

#include "unicode/utypes.h"

#if !UCONFIG_NO_FORMATTING

#include "dtfmttst.h"
#include "unicode/timezone.h"
#include "unicode/gregocal.h"
#include "unicode/smpdtfmt.h"
#include "unicode/datefmt.h"
#include "unicode/simpletz.h"
#include "unicode/strenum.h"
#include "unicode/dtfmtsym.h"
#include "cmemory.h"
#include "cstring.h"
#include "caltest.h"  // for fieldName
#include <stdio.h> // for sprintf

#ifdef U_WINDOWS
#include "windttst.h"
#endif

#define ARRAY_SIZE(array) (sizeof array / sizeof array[0])

#define ASSERT_OK(status)  if(U_FAILURE(status)) {errcheckln(status, #status " = %s @ %s:%d", u_errorName(status), __FILE__, __LINE__); return; }

// *****************************************************************************
// class DateFormatTest
// *****************************************************************************

void DateFormatTest::runIndexedTest( int32_t index, UBool exec, const char* &name, char* /*par*/ )
{
    // if (exec) logln((UnicodeString)"TestSuite DateFormatTest");
    switch (index) {
        TESTCASE(0,TestEquals);
        TESTCASE(1,TestTwoDigitYearDSTParse);
        TESTCASE(2,TestFieldPosition);
        TESTCASE(3,TestPartialParse994);
        TESTCASE(4,TestRunTogetherPattern985);
        TESTCASE(5,TestRunTogetherPattern917);
        TESTCASE(6,TestCzechMonths459);
        TESTCASE(7,TestLetterDPattern212);
        TESTCASE(8,TestDayOfYearPattern195);
        TESTCASE(9,TestQuotePattern161);
        TESTCASE(10,TestBadInput135);
        TESTCASE(11,TestBadInput135a);
        TESTCASE(12,TestTwoDigitYear);
        TESTCASE(13,TestDateFormatZone061);
        TESTCASE(14,TestDateFormatZone146);
        TESTCASE(15,TestLocaleDateFormat);
        TESTCASE(16,TestWallyWedel);
        TESTCASE(17,TestDateFormatCalendar);
        TESTCASE(18,TestSpaceParsing);
        TESTCASE(19,TestExactCountFormat);
        TESTCASE(20,TestWhiteSpaceParsing);
        TESTCASE(21,TestInvalidPattern);
        TESTCASE(22,TestGeneral);
        TESTCASE(23,TestGreekMay);
        TESTCASE(24,TestGenericTime);
        TESTCASE(25,TestGenericTimeZoneOrder);
        TESTCASE(26,TestHost);
        TESTCASE(27,TestEras);
        TESTCASE(28,TestNarrowNames);
        TESTCASE(29,TestStandAloneDays);
        TESTCASE(30,TestStandAloneMonths);
        TESTCASE(31,TestQuarters);
        TESTCASE(32,TestZTimeZoneParsing);
        TESTCASE(33,TestRelative);
        TESTCASE(34,TestRelativeClone);
        TESTCASE(35,TestHostClone);
        TESTCASE(36,TestTimeZoneDisplayName);
        TESTCASE(37,TestRoundtripWithCalendar);
        TESTCASE(38,Test6338);
        TESTCASE(39,Test6726);
        TESTCASE(40,TestGMTParsing);
        TESTCASE(41,Test6880);
        TESTCASE(42,TestISOEra);
        TESTCASE(43,TestFormalChineseDate);
        /*
        TESTCASE(43,TestRelativeError);
        TESTCASE(44,TestRelativeOther);
        */
        default: name = ""; break;
    }
}

// Test written by Wally Wedel and emailed to me.
void DateFormatTest::TestWallyWedel()
{
    UErrorCode status = U_ZERO_ERROR;
    /*
     * Instantiate a TimeZone so we can get the ids.
     */
    TimeZone *tz = new SimpleTimeZone(7,"");
    /*
     * Computational variables.
     */
    int32_t offset, hours, minutes, seconds;
    /*
     * Instantiate a SimpleDateFormat set up to produce a full time
     zone name.
     */
    SimpleDateFormat *sdf = new SimpleDateFormat((UnicodeString)"zzzz", status);
    /*
     * A String array for the time zone ids.
     */
    int32_t ids_length;
    StringEnumeration* ids = TimeZone::createEnumeration();
    ids_length = ids->count(status);
    /*
     * How many ids do we have?
     */
    logln("Time Zone IDs size: %d", ids_length);
    /*
     * Column headings (sort of)
     */
    logln("Ordinal ID offset(h:m) name");
    /*
     * Loop through the tzs.
     */
    UDate today = Calendar::getNow();
    Calendar *cal = Calendar::createInstance(status);
    for (int32_t i = 0; i < ids_length; i++) {
        // logln(i + " " + ids[i]);
        const UnicodeString* id = ids->snext(status);
        TimeZone *ttz = TimeZone::createTimeZone(*id);
        // offset = ttz.getRawOffset();
        cal->setTimeZone(*ttz);
        cal->setTime(today, status);
        offset = cal->get(UCAL_ZONE_OFFSET, status) + cal->get(UCAL_DST_OFFSET, status);
        // logln(i + " " + ids[i] + " offset " + offset);
        const char* sign = "+";
        if (offset < 0) {
            sign = "-";
            offset = -offset;
        }
        hours = offset/3600000;
        minutes = (offset%3600000)/60000;
        seconds = (offset%60000)/1000;
        UnicodeString dstOffset = (UnicodeString)"" + sign + (hours < 10 ? "0" : "") +
            (int32_t)hours + ":" + (minutes < 10 ? "0" : "") + (int32_t)minutes;
        if (seconds != 0) {
            dstOffset = dstOffset + ":" + (seconds < 10 ? "0" : "") + seconds;
        }
        /*
         * Instantiate a date so we can display the time zone name.
         */
        sdf->setTimeZone(*ttz);
        /*
         * Format the output.
         */
        UnicodeString fmtOffset;
        FieldPosition pos(0);
        sdf->format(today,fmtOffset, pos);
        // UnicodeString fmtOffset = tzS.toString();
        UnicodeString *fmtDstOffset = 0;
        if (fmtOffset.startsWith("GMT"))
        {
            //fmtDstOffset = fmtOffset->substring(3);
            fmtDstOffset = new UnicodeString();
            fmtOffset.extract(3, fmtOffset.length(), *fmtDstOffset);
        }
        /*
         * Show our result.
         */
        UBool ok = fmtDstOffset == 0 || *fmtDstOffset == dstOffset;
        if (ok)
        {
            logln(UnicodeString() + i + " " + *id + " " + dstOffset +
                  " " + fmtOffset +
                  (fmtDstOffset != 0 ? " ok" : " ?"));
        }
        else
        {
            errln(UnicodeString() + i + " " + *id + " " + dstOffset +
                  " " + fmtOffset + " *** FAIL ***");
        }
        delete ttz;
        delete fmtDstOffset;
    }
    delete cal;
    //  delete ids;   // TODO:  BAD API
    delete ids;
    delete sdf;
    delete tz;
}

// -------------------------------------

/**
 * Test operator==
 */
void
DateFormatTest::TestEquals()
{
    DateFormat* fmtA = DateFormat::createDateTimeInstance(DateFormat::MEDIUM, DateFormat::FULL);
    DateFormat* fmtB = DateFormat::createDateTimeInstance(DateFormat::MEDIUM, DateFormat::FULL);
    if ( fmtA == NULL || fmtB == NULL){
        dataerrln("Error calling DateFormat::createDateTimeInstance");
        delete fmtA;
        delete fmtB;
        return;
    }

    if (!(*fmtA == *fmtB)) errln((UnicodeString)"FAIL");
    delete fmtA;
    delete fmtB;

    TimeZone* test = TimeZone::createTimeZone("PDT");
    delete test;
}

// -------------------------------------

/**
 * Test the parsing of 2-digit years.
 */
void
DateFormatTest::TestTwoDigitYearDSTParse(void)
{
    UErrorCode status = U_ZERO_ERROR;
    SimpleDateFormat* fullFmt = new SimpleDateFormat((UnicodeString)"EEE MMM dd HH:mm:ss.SSS zzz yyyy G", status);
    SimpleDateFormat *fmt = new SimpleDateFormat((UnicodeString)"dd-MMM-yy h:mm:ss 'o''clock' a z", Locale::getEnglish(), status);
    //DateFormat* fmt = DateFormat::createDateTimeInstance(DateFormat::MEDIUM, DateFormat::FULL, Locale::ENGLISH);
    UnicodeString* s = new UnicodeString("03-Apr-04 2:20:47 o'clock AM PST", "");
    TimeZone* defaultTZ = TimeZone::createDefault();
    TimeZone* PST = TimeZone::createTimeZone("PST");
    int32_t defaultOffset = defaultTZ->getRawOffset();
    int32_t PSTOffset = PST->getRawOffset();
    int32_t hour = 2 + (defaultOffset - PSTOffset) / (60*60*1000);
    // hour is the expected hour of day, in units of seconds
    hour = ((hour < 0) ? hour + 24 : hour) * 60*60;

    UnicodeString str;

    if(U_FAILURE(status)) {
        dataerrln("Could not set up test. exitting - %s", u_errorName(status));
        return;
    }

    UDate d = fmt->parse(*s, status);
    logln(*s + " P> " + ((DateFormat*)fullFmt)->format(d, str));
    int32_t y, m, day, hr, min, sec;
    dateToFields(d, y, m, day, hr, min, sec);
    hour += defaultTZ->inDaylightTime(d, status) ? 1 : 0;
    hr = hr*60*60;
    if (hr != hour)
        errln((UnicodeString)"FAIL: Should parse to hour " + hour + " but got " + hr);

    if (U_FAILURE(status))
        errln((UnicodeString)"FAIL: " + (int32_t)status);

    delete s;
    delete fmt;
    delete fullFmt;
    delete PST;
    delete defaultTZ;
}

// -------------------------------------

UChar toHexString(int32_t i) { return (UChar)(i + (i < 10 ? 0x30 : (0x41 - 10))); }

UnicodeString&
DateFormatTest::escape(UnicodeString& s)
{
    UnicodeString buf;
    for (int32_t i=0; i<s.length(); ++i)
    {
        UChar c = s[(int32_t)i];
        if (c <= (UChar)0x7F) buf += c;
        else {
            buf += (UChar)0x5c; buf += (UChar)0x55;
            buf += toHexString((c & 0xF000) >> 12);
            buf += toHexString((c & 0x0F00) >> 8);
            buf += toHexString((c & 0x00F0) >> 4);
            buf += toHexString(c & 0x000F);
        }
    }
    return (s = buf);
}

// -------------------------------------

/**
 * This MUST be kept in sync with DateFormatSymbols.gPatternChars.
 */
static const char* PATTERN_CHARS = "GyMdkHmsSEDFwWahKzYeugAZvcLQqV";

/**
 * A list of the names of all the fields in DateFormat.
 * This MUST be kept in sync with DateFormat.
 */
static const char* DATEFORMAT_FIELD_NAMES[] = {
    "ERA_FIELD",
    "YEAR_FIELD",
    "MONTH_FIELD",
    "DATE_FIELD",
    "HOUR_OF_DAY1_FIELD",
    "HOUR_OF_DAY0_FIELD",
    "MINUTE_FIELD",
    "SECOND_FIELD",
    "MILLISECOND_FIELD",
    "DAY_OF_WEEK_FIELD",
    "DAY_OF_YEAR_FIELD",
    "DAY_OF_WEEK_IN_MONTH_FIELD",
    "WEEK_OF_YEAR_FIELD",
    "WEEK_OF_MONTH_FIELD",
    "AM_PM_FIELD",
    "HOUR1_FIELD",
    "HOUR0_FIELD",
    "TIMEZONE_FIELD",
    "YEAR_WOY_FIELD",
    "DOW_LOCAL_FIELD",
    "EXTENDED_YEAR_FIELD",
    "JULIAN_DAY_FIELD",
    "MILLISECONDS_IN_DAY_FIELD",
    "TIMEZONE_RFC_FIELD",
    "GENERIC_TIMEZONE_FIELD",
    "STAND_ALONE_DAY_FIELD",
    "STAND_ALONE_MONTH_FIELD",
    "QUARTER_FIELD",
    "STAND_ALONE_QUARTER_FIELD",
    "TIMEZONE_SPECIAL_FIELD"
};

static const int32_t DATEFORMAT_FIELD_NAMES_LENGTH =
    sizeof(DATEFORMAT_FIELD_NAMES) / sizeof(DATEFORMAT_FIELD_NAMES[0]);

/**
 * Verify that returned field position indices are correct.
 */
void DateFormatTest::TestFieldPosition() {
    UErrorCode ec = U_ZERO_ERROR;
    int32_t i, j, exp;
    UnicodeString buf;

    // Verify data
    DateFormatSymbols rootSyms(Locale(""), ec);
    assertSuccess("DateFormatSymbols", ec);
    if (U_FAILURE(ec)) {
        return;
    }

    // local pattern chars data is not longer loaded
    // from icu locale bundle
    assertEquals("patternChars", PATTERN_CHARS, rootSyms.getLocalPatternChars(buf));
    assertEquals("patternChars", PATTERN_CHARS, DateFormatSymbols::getPatternUChars());
    assertTrue("DATEFORMAT_FIELD_NAMES", DATEFORMAT_FIELD_NAMES_LENGTH == UDAT_FIELD_COUNT);
    assertTrue("Data", UDAT_FIELD_COUNT == uprv_strlen(PATTERN_CHARS));

    // Create test formatters
    const int32_t COUNT = 4;
    DateFormat* dateFormats[COUNT];
    dateFormats[0] = DateFormat::createDateTimeInstance(DateFormat::kFull, DateFormat::kFull, Locale::getUS());
    dateFormats[1] = DateFormat::createDateTimeInstance(DateFormat::kFull, DateFormat::kFull, Locale::getFrance());
    // Make the pattern "G y M d..."
    buf.remove().append(PATTERN_CHARS);
    for (j=buf.length()-1; j>=0; --j) buf.insert(j, (UChar)32/*' '*/);
    dateFormats[2] = new SimpleDateFormat(buf, Locale::getUS(), ec);
    // Make the pattern "GGGG yyyy MMMM dddd..."
    for (j=buf.length()-1; j>=0; j-=2) {
        for (i=0; i<3; ++i) {
            buf.insert(j, buf.charAt(j));
        }
    }
    dateFormats[3] = new SimpleDateFormat(buf, Locale::getUS(), ec);
    if(U_FAILURE(ec)){
        errln(UnicodeString("Could not create SimpleDateFormat object for locale en_US. Error: " )+ UnicodeString(u_errorName(ec)));
        return;
    }
    UDate aug13 = 871508052513.0;

    // Expected output field values for above DateFormats on aug13
    // Fields are given in order of DateFormat field number
    const char* EXPECTED[] = {
        "", "1997", "August", "13", "", "", "34", "12", "",
        "Wednesday", "", "", "", "", "PM", "2", "", "Pacific Daylight Time", "", "", "", "", "", "", "", "", "", "", "","",

        "", "1997", "ao\\u00FBt", "13", "", "14", "34", "12", "",
        "mercredi", "", "", "", "", "", "", "", "heure avanc\\u00e9e du Pacifique", "", "", "", "", "", "", "",  "", "", "", "", "",

        "AD", "1997", "8", "13", "14", "14", "34", "12", "5",
        "Wed", "225", "2", "33", "2", "PM", "2", "2", "PDT", "1997", "4", "1997", "2450674", "52452513", "-0700", "PT",  "4", "8", "3", "3","PDT",

        "Anno Domini", "1997", "August", "0013", "0014", "0014", "0034", "0012", "5130",
        "Wednesday", "0225", "0002", "0033", "0002", "PM", "0002", "0002", "Pacific Daylight Time", "1997", "Wednesday", "1997", "2450674", "52452513", "GMT-07:00",
        "Pacific Time",  "Wednesday", "August", "3rd quarter", "3rd quarter", "United States (Los Angeles)"
    };

    const int32_t EXPECTED_LENGTH = sizeof(EXPECTED)/sizeof(EXPECTED[0]);

    assertTrue("data size", EXPECTED_LENGTH == COUNT * UDAT_FIELD_COUNT);

    TimeZone* PT = TimeZone::createTimeZone("America/Los_Angeles");
    for (j = 0, exp = 0; j < COUNT; ++j) {
        //  String str;
        DateFormat* df = dateFormats[j];
        df->setTimeZone(*PT);
        SimpleDateFormat* sdtfmt = dynamic_cast<SimpleDateFormat*>(df);
        if (sdtfmt != NULL) {
            logln(" Pattern = " + sdtfmt->toPattern(buf.remove()));
        } else {
            logln(" Pattern = ? (not a SimpleDateFormat)");
        }
        logln((UnicodeString)"  Result = " + df->format(aug13, buf.remove()));

        int32_t expBase = exp; // save for later
        for (i = 0; i < UDAT_FIELD_COUNT; ++i, ++exp) {
            FieldPosition pos(i);
            buf.remove();
            df->format(aug13, buf, pos);
            UnicodeString field;
            buf.extractBetween(pos.getBeginIndex(), pos.getEndIndex(), field);
            assertEquals((UnicodeString)"field #" + i + " " + DATEFORMAT_FIELD_NAMES[i],
                         ctou(EXPECTED[exp]), field);
        }

        // test FieldPositionIterator API
        logln("FieldPositionIterator");
        {
          UErrorCode status = U_ZERO_ERROR;
          FieldPositionIterator posIter;
          FieldPosition fp;

          buf.remove();
          df->format(aug13, buf, &posIter, status);
          while (posIter.next(fp)) {
            int32_t i = fp.getField();
            UnicodeString field;
            buf.extractBetween(fp.getBeginIndex(), fp.getEndIndex(), field);
            assertEquals((UnicodeString)"field #" + i + " " + DATEFORMAT_FIELD_NAMES[i],
                         ctou(EXPECTED[expBase + i]), field);
          }

        }
    }


    // test null posIter
    buf.remove();
    UErrorCode status = U_ZERO_ERROR;
    dateFormats[0]->format(aug13, buf, NULL, status);
    // if we didn't crash, we succeeded.

    for (i=0; i<COUNT; ++i) {
        delete dateFormats[i];
    }
    delete PT;
}

// -------------------------------------

/**
 * General parse/format tests.  Add test cases as needed.
 */
void DateFormatTest::TestGeneral() {
    const char* DATA[] = {
        "yyyy MM dd HH:mm:ss.SSS",

        // Milliseconds are left-justified, since they format as fractions of a second
        "y/M/d H:mm:ss.S", "fp", "2004 03 10 16:36:31.567", "2004/3/10 16:36:31.5", "2004 03 10 16:36:31.500",
        "y/M/d H:mm:ss.SS", "fp", "2004 03 10 16:36:31.567", "2004/3/10 16:36:31.56", "2004 03 10 16:36:31.560",
        "y/M/d H:mm:ss.SSS", "F", "2004 03 10 16:36:31.567", "2004/3/10 16:36:31.567",
        "y/M/d H:mm:ss.SSSS", "pf", "2004/3/10 16:36:31.5679", "2004 03 10 16:36:31.568", "2004/3/10 16:36:31.5680",
    };
    expect(DATA, ARRAY_SIZE(DATA), Locale("en", "", ""));
}

// -------------------------------------

/**
 * Verify that strings which contain incomplete specifications are parsed
 * correctly.  In some instances, this means not being parsed at all, and
 * returning an appropriate error.
 */
void
DateFormatTest::TestPartialParse994()
{
    UErrorCode status = U_ZERO_ERROR;
    SimpleDateFormat* f = new SimpleDateFormat(status);
    if (U_FAILURE(status)) {
        dataerrln("Fail new SimpleDateFormat: %s", u_errorName(status));
        delete f;
        return;
    }
    UDate null = 0;
    tryPat994(f, "yy/MM/dd HH:mm:ss", "97/01/17 10:11:42", date(97, 1 - 1, 17, 10, 11, 42));
    tryPat994(f, "yy/MM/dd HH:mm:ss", "97/01/17 10:", null);
    tryPat994(f, "yy/MM/dd HH:mm:ss", "97/01/17 10", null);
    tryPat994(f, "yy/MM/dd HH:mm:ss", "97/01/17 ", null);
    tryPat994(f, "yy/MM/dd HH:mm:ss", "97/01/17", null);
    if (U_FAILURE(status)) errln((UnicodeString)"FAIL: UErrorCode received during test: " + (int32_t)status);
    delete f;
}

// -------------------------------------

void
DateFormatTest::tryPat994(SimpleDateFormat* format, const char* pat, const char* str, UDate expected)
{
    UErrorCode status = U_ZERO_ERROR;
    UDate null = 0;
    logln(UnicodeString("Pattern \"") + pat + "\"   String \"" + str + "\"");
    //try {
        format->applyPattern(pat);
        UDate date = format->parse(str, status);
        if (U_FAILURE(status) || date == null)
        {
            logln((UnicodeString)"ParseException: " + (int32_t)status);
            if (expected != null) errln((UnicodeString)"FAIL: Expected " + dateToString(expected));
        }
        else
        {
            UnicodeString f;
            ((DateFormat*)format)->format(date, f);
            logln(UnicodeString(" parse(") + str + ") -> " + dateToString(date));
            logln((UnicodeString)" format -> " + f);
            if (expected == null ||
                !(date == expected)) errln((UnicodeString)"FAIL: Expected null");//" + expected);
            if (!(f == str)) errln(UnicodeString("FAIL: Expected ") + str);
        }
    //}
    //catch(ParseException e) {
    //    logln((UnicodeString)"ParseException: " + e.getMessage());
    //    if (expected != null) errln((UnicodeString)"FAIL: Expected " + dateToString(expected));
    //}
    //catch(Exception e) {
    //    errln((UnicodeString)"*** Exception:");
    //    e.printStackTrace();
    //}
}

// -------------------------------------

/**
 * Verify the behavior of patterns in which digits for different fields run together
 * without intervening separators.
 */
void
DateFormatTest::TestRunTogetherPattern985()
{
    UErrorCode status = U_ZERO_ERROR;
    UnicodeString format("yyyyMMddHHmmssSSS");
    UnicodeString now, then;
    //UBool flag;
    SimpleDateFormat *formatter = new SimpleDateFormat(format, status);
    if (U_FAILURE(status)) {
        dataerrln("Fail new SimpleDateFormat: %s", u_errorName(status));
        delete formatter;
        return;
    }
    UDate date1 = Calendar::getNow();
    ((DateFormat*)formatter)->format(date1, now);
    logln(now);
    ParsePosition pos(0);
    UDate date2 = formatter->parse(now, pos);
    if (date2 == 0) then = "Parse stopped at " + pos.getIndex();
    else ((DateFormat*)formatter)->format(date2, then);
    logln(then);
    if (!(date2 == date1)) errln((UnicodeString)"FAIL");
    delete formatter;
    if (U_FAILURE(status)) errln((UnicodeString)"FAIL: UErrorCode received during test: " + (int32_t)status);
}

// -------------------------------------

/**
 * Verify the behavior of patterns in which digits for different fields run together
 * without intervening separators.
 */
void
DateFormatTest::TestRunTogetherPattern917()
{
    UErrorCode status = U_ZERO_ERROR;
    SimpleDateFormat* fmt;
    UnicodeString myDate;
    fmt = new SimpleDateFormat((UnicodeString)"yyyy/MM/dd", status);
    if (U_FAILURE(status)) {
        dataerrln("Fail new SimpleDateFormat: %s", u_errorName(status));
        delete fmt;
        return;
    }
    myDate = "1997/02/03";
    testIt917(fmt, myDate, date(97, 2 - 1, 3));
    delete fmt;
    fmt = new SimpleDateFormat((UnicodeString)"yyyyMMdd", status);
    myDate = "19970304";
    testIt917(fmt, myDate, date(97, 3 - 1, 4));
    delete fmt;
    if (U_FAILURE(status)) errln((UnicodeString)"FAIL: UErrorCode received during test: " + (int32_t)status);
}

// -------------------------------------

void
DateFormatTest::testIt917(SimpleDateFormat* fmt, UnicodeString& str, UDate expected)
{
    UErrorCode status = U_ZERO_ERROR;
    UnicodeString pattern;
    logln((UnicodeString)"pattern=" + fmt->toPattern(pattern) + "   string=" + str);
    Formattable o;
    //try {
        ((Format*)fmt)->parseObject(str, o, status);
    //}
    if (U_FAILURE(status)) return;
    //catch(ParseException e) {
    //    e.printStackTrace();
    //    return;
    //}
    logln((UnicodeString)"Parsed object: " + dateToString(o.getDate()));
    if (!(o.getDate() == expected)) errln((UnicodeString)"FAIL: Expected " + dateToString(expected));
    UnicodeString formatted; ((Format*)fmt)->format(o, formatted, status);
    logln((UnicodeString)"Formatted string: " + formatted);
    if (!(formatted == str)) errln((UnicodeString)"FAIL: Expected " + str);
    if (U_FAILURE(status)) errln((UnicodeString)"FAIL: UErrorCode received during test: " + (int32_t)status);
}

// -------------------------------------

/**
 * Verify the handling of Czech June and July, which have the unique attribute that
 * one is a proper prefix substring of the other.
 */
void
DateFormatTest::TestCzechMonths459()
{
    UErrorCode status = U_ZERO_ERROR;
    DateFormat* fmt = DateFormat::createDateInstance(DateFormat::FULL, Locale("cs", "", ""));
    if (fmt == NULL){
        dataerrln("Error calling DateFormat::createDateInstance()");
        return;
    }

    UnicodeString pattern;
    logln((UnicodeString)"Pattern " + ((SimpleDateFormat*) fmt)->toPattern(pattern));
    UDate june = date(97, UCAL_JUNE, 15);
    UDate july = date(97, UCAL_JULY, 15);
    UnicodeString juneStr; fmt->format(june, juneStr);
    UnicodeString julyStr; fmt->format(july, julyStr);
    //try {
        logln((UnicodeString)"format(June 15 1997) = " + juneStr);
        UDate d = fmt->parse(juneStr, status);
        UnicodeString s; fmt->format(d, s);
        int32_t month,yr,day,hr,min,sec; dateToFields(d,yr,month,day,hr,min,sec);
        logln((UnicodeString)"  -> parse -> " + s + " (month = " + month + ")");
        if (month != UCAL_JUNE) errln((UnicodeString)"FAIL: Month should be June");
        logln((UnicodeString)"format(July 15 1997) = " + julyStr);
        d = fmt->parse(julyStr, status);
        fmt->format(d, s);
        dateToFields(d,yr,month,day,hr,min,sec);
        logln((UnicodeString)"  -> parse -> " + s + " (month = " + month + ")");
        if (month != UCAL_JULY) errln((UnicodeString)"FAIL: Month should be July");
    //}
    //catch(ParseException e) {
    if (U_FAILURE(status))
        errln((UnicodeString)"Exception: " + (int32_t)status);
    //}
    delete fmt;
}

// -------------------------------------

/**
 * Test the handling of 'D' in patterns.
 */
void
DateFormatTest::TestLetterDPattern212()
{
    UErrorCode status = U_ZERO_ERROR;
    UnicodeString dateString("1995-040.05:01:29");
    UnicodeString bigD("yyyy-DDD.hh:mm:ss");
    UnicodeString littleD("yyyy-ddd.hh:mm:ss");
    UDate expLittleD = date(95, 0, 1, 5, 1, 29);
    UDate expBigD = expLittleD + 39 * 24 * 3600000.0;
    expLittleD = expBigD; // Expect the same, with default lenient parsing
    logln((UnicodeString)"dateString= " + dateString);
    SimpleDateFormat *formatter = new SimpleDateFormat(bigD, status);
    if (U_FAILURE(status)) {
        dataerrln("Fail new SimpleDateFormat: %s", u_errorName(status));
        delete formatter;
        return;
    }
    ParsePosition pos(0);
    UDate myDate = formatter->parse(dateString, pos);
    logln((UnicodeString)"Using " + bigD + " -> " + myDate);
    if (myDate != expBigD) errln((UnicodeString)"FAIL: Expected " + dateToString(expBigD));
    delete formatter;
    formatter = new SimpleDateFormat(littleD, status);
    ASSERT_OK(status);
    pos = ParsePosition(0);
    myDate = formatter->parse(dateString, pos);
    logln((UnicodeString)"Using " + littleD + " -> " + dateToString(myDate));
    if (myDate != expLittleD) errln((UnicodeString)"FAIL: Expected " + dateToString(expLittleD));
    delete formatter;
    if (U_FAILURE(status)) errln((UnicodeString)"FAIL: UErrorCode received during test: " + (int32_t)status);
}

// -------------------------------------

/**
 * Test the day of year pattern.
 */
void
DateFormatTest::TestDayOfYearPattern195()
{
    UErrorCode status = U_ZERO_ERROR;
    UDate today = Calendar::getNow();
    int32_t year,month,day,hour,min,sec; dateToFields(today,year,month,day,hour,min,sec);
    UDate expected = date(year, month, day);
    logln((UnicodeString)"Test Date: " + dateToString(today));
    SimpleDateFormat* sdf = (SimpleDateFormat*)DateFormat::createDateInstance();
    if (sdf == NULL){
        dataerrln("Error calling DateFormat::createDateInstance()");
        return;
    }
    tryPattern(*sdf, today, 0, expected);
    tryPattern(*sdf, today, "G yyyy DDD", expected);
    delete sdf;
    if (U_FAILURE(status)) errln((UnicodeString)"FAIL: UErrorCode received during test: " + (int32_t)status);
}

// -------------------------------------

void
DateFormatTest::tryPattern(SimpleDateFormat& sdf, UDate d, const char* pattern, UDate expected)
{
    UErrorCode status = U_ZERO_ERROR;
    if (pattern != 0) sdf.applyPattern(pattern);
    UnicodeString thePat;
    logln((UnicodeString)"pattern: " + sdf.toPattern(thePat));
    UnicodeString formatResult; (*(DateFormat*)&sdf).format(d, formatResult);
    logln((UnicodeString)" format -> " + formatResult);
    // try {
        UDate d2 = sdf.parse(formatResult, status);
        logln((UnicodeString)" parse(" + formatResult + ") -> " + dateToString(d2));
        if (d2 != expected) errln((UnicodeString)"FAIL: Expected " + dateToString(expected));
        UnicodeString format2; (*(DateFormat*)&sdf).format(d2, format2);
        logln((UnicodeString)" format -> " + format2);
        if (!(formatResult == format2)) errln((UnicodeString)"FAIL: Round trip drift");
    //}
    //catch(Exception e) {
    if (U_FAILURE(status))
        errln((UnicodeString)"Error: " + (int32_t)status);
    //}
}

// -------------------------------------

/**
 * Test the handling of single quotes in patterns.
 */
void
DateFormatTest::TestQuotePattern161()
{
    UErrorCode status = U_ZERO_ERROR;
    SimpleDateFormat* formatter = new SimpleDateFormat((UnicodeString)"MM/dd/yyyy 'at' hh:mm:ss a zzz", status);
    if (U_FAILURE(status)) {
        dataerrln("Fail new SimpleDateFormat: %s", u_errorName(status));
        delete formatter;
        return;
    }
    UDate currentTime_1 = date(97, UCAL_AUGUST, 13, 10, 42, 28);
    UnicodeString dateString; ((DateFormat*)formatter)->format(currentTime_1, dateString);
    UnicodeString exp("08/13/1997 at 10:42:28 AM ");
    logln((UnicodeString)"format(" + dateToString(currentTime_1) + ") = " + dateString);
    if (0 != dateString.compareBetween(0, exp.length(), exp, 0, exp.length())) errln((UnicodeString)"FAIL: Expected " + exp);
    delete formatter;
    if (U_FAILURE(status)) errln((UnicodeString)"FAIL: UErrorCode received during test: " + (int32_t)status);
}

// -------------------------------------

/**
 * Verify the correct behavior when handling invalid input strings.
 */
void
DateFormatTest::TestBadInput135()
{
    UErrorCode status = U_ZERO_ERROR;
    DateFormat::EStyle looks[] = {
        DateFormat::SHORT, DateFormat::MEDIUM, DateFormat::LONG, DateFormat::FULL
    };
    int32_t looks_length = (int32_t)(sizeof(looks) / sizeof(looks[0]));
    const char* strings[] = {
        "Mar 15", "Mar 15 1997", "asdf", "3/1/97 1:23:", "3/1/00 1:23:45 AM"
    };
    int32_t strings_length = (int32_t)(sizeof(strings) / sizeof(strings[0]));
    DateFormat *full = DateFormat::createDateTimeInstance(DateFormat::LONG, DateFormat::LONG);
    if(full==NULL) {
      dataerrln("could not create date time instance");
      return;
    }
    UnicodeString expected("March 1, 2000 1:23:45 AM ");
    for (int32_t i = 0; i < strings_length;++i) {
        const char* text = strings[i];
        for (int32_t j = 0; j < looks_length;++j) {
            DateFormat::EStyle dateLook = looks[j];
            for (int32_t k = 0; k < looks_length;++k) {
                DateFormat::EStyle timeLook = looks[k];
                DateFormat *df = DateFormat::createDateTimeInstance(dateLook, timeLook);
                if (df == NULL){
                    dataerrln("Error calling DateFormat::createDateTimeInstance()");
                    continue;
                }
                UnicodeString prefix = UnicodeString(text) + ", " + dateLook + "/" + timeLook + ": ";
                //try {
                    UDate when = df->parse(text, status);
                    if (when == 0 && U_SUCCESS(status)) {
                        errln(prefix + "SHOULD NOT HAPPEN: parse returned 0.");
                        continue;
                    }
                    if (U_SUCCESS(status))
                    {
                        UnicodeString format;
                        full->format(when, format);
                        logln(prefix + "OK: " + format);
                        if (0!=format.compareBetween(0, expected.length(), expected, 0, expected.length()))
                            errln((UnicodeString)"FAIL: Expected " + expected + " got " + format);
                    }
                //}
                //catch(ParseException e) {
                    else
                        status = U_ZERO_ERROR;
                //}
                //catch(StringIndexOutOfBoundsException e) {
                //    errln(prefix + "SHOULD NOT HAPPEN: " + (int)status);
                //}
                delete df;
            }
        }
    }
    delete full;
    if (U_FAILURE(status))
        errln((UnicodeString)"FAIL: UErrorCode received during test: " + (int32_t)status);
}

static const char* const parseFormats[] = {
    "MMMM d, yyyy",
    "MMMM d yyyy",
    "M/d/yy",
    "d MMMM, yyyy",
    "d MMMM yyyy",
    "d MMMM",
    "MMMM d",
    "yyyy",
    "h:mm a MMMM d, yyyy"
};

static const char* const inputStrings[] = {
    "bogus string", 0, 0, 0, 0, 0, 0, 0, 0, 0,
    "April 1, 1997", "April 1, 1997", 0, 0, 0, 0, 0, "April 1", 0, 0,
    "Jan 1, 1970", "January 1, 1970", 0, 0, 0, 0, 0, "January 1", 0, 0,
    "Jan 1 2037", 0, "January 1 2037", 0, 0, 0, 0, "January 1", 0, 0,
    "1/1/70", 0, 0, "1/1/70", 0, 0, 0, 0, "0001", 0,
    "5 May 1997", 0, 0, 0, 0, "5 May 1997", "5 May", 0, "0005", 0,
    "16 May", 0, 0, 0, 0, 0, "16 May", 0, "0016", 0,
    "April 30", 0, 0, 0, 0, 0, 0, "April 30", 0, 0,
    "1998", 0, 0, 0, 0, 0, 0, 0, "1998", 0,
    "1", 0, 0, 0, 0, 0, 0, 0, "0001", 0,
    "3:00 pm Jan 1, 1997", 0, 0, 0, 0, 0, 0, 0, "0003", "3:00 PM January 1, 1997",
};

// -------------------------------------

/**
 * Verify the correct behavior when parsing an array of inputs against an
 * array of patterns, with known results.  The results are encoded after
 * the input strings in each row.
 */
void
DateFormatTest::TestBadInput135a()
{
  UErrorCode status = U_ZERO_ERROR;
  SimpleDateFormat* dateParse = new SimpleDateFormat(status);
  if(U_FAILURE(status)) {
    dataerrln("Failed creating SimpleDateFormat with %s. Quitting test", u_errorName(status));
    delete dateParse;
    return;
  }
  const char* s;
  UDate date;
  const uint32_t PF_LENGTH = (int32_t)(sizeof(parseFormats)/sizeof(parseFormats[0]));
  const uint32_t INPUT_LENGTH = (int32_t)(sizeof(inputStrings)/sizeof(inputStrings[0]));

  dateParse->applyPattern("d MMMM, yyyy");
  dateParse->adoptTimeZone(TimeZone::createDefault());
  s = "not parseable";
  UnicodeString thePat;
  logln(UnicodeString("Trying to parse \"") + s + "\" with " + dateParse->toPattern(thePat));
  //try {
  date = dateParse->parse(s, status);
  if (U_SUCCESS(status))
    errln((UnicodeString)"FAIL: Expected exception during parse");
  //}
  //catch(Exception ex) {
  else
    logln((UnicodeString)"Exception during parse: " + (int32_t)status);
  status = U_ZERO_ERROR;
  //}
  for (uint32_t i = 0; i < INPUT_LENGTH; i += (PF_LENGTH + 1)) {
    ParsePosition parsePosition(0);
    UnicodeString s( inputStrings[i]);
    for (uint32_t index = 0; index < PF_LENGTH;++index) {
      const char* expected = inputStrings[i + 1 + index];
      dateParse->applyPattern(parseFormats[index]);
      dateParse->adoptTimeZone(TimeZone::createDefault());
      //try {
      parsePosition.setIndex(0);
      date = dateParse->parse(s, parsePosition);
      if (parsePosition.getIndex() != 0) {
        UnicodeString s1, s2;
        s.extract(0, parsePosition.getIndex(), s1);
        s.extract(parsePosition.getIndex(), s.length(), s2);
        if (date == 0) {
          errln((UnicodeString)"ERROR: null result fmt=\"" +
                     parseFormats[index] +
                     "\" pos=" + parsePosition.getIndex() + " " +
                     s1 + "|" + s2);
        }
        else {
          UnicodeString result;
          ((DateFormat*)dateParse)->format(date, result);
          logln((UnicodeString)"Parsed \"" + s + "\" using \"" + dateParse->toPattern(thePat) + "\" to: " + result);
          if (expected == 0)
            errln((UnicodeString)"FAIL: Expected parse failure");
          else if (!(result == expected))
            errln(UnicodeString("FAIL: Expected ") + expected);
        }
      }
      else if (expected != 0) {
        errln(UnicodeString("FAIL: Expected ") + expected + " from \"" +
                     s + "\" with \"" + dateParse->toPattern(thePat) + "\"");
      }
      //}
      //catch(Exception ex) {
      if (U_FAILURE(status))
        errln((UnicodeString)"An exception was thrown during parse: " + (int32_t)status);
      //}
    }
  }
  delete dateParse;
  if (U_FAILURE(status))
    errln((UnicodeString)"FAIL: UErrorCode received during test: " + (int32_t)status);
}

// -------------------------------------

/**
 * Test the parsing of two-digit years.
 */
void
DateFormatTest::TestTwoDigitYear()
{
    UErrorCode ec = U_ZERO_ERROR;
    SimpleDateFormat fmt("dd/MM/yy", Locale::getUK(), ec);
    if (U_FAILURE(ec)) {
        dataerrln("FAIL: SimpleDateFormat constructor - %s", u_errorName(ec));
        return;
    }
    parse2DigitYear(fmt, "5/6/17", date(117, UCAL_JUNE, 5));
    parse2DigitYear(fmt, "4/6/34", date(34, UCAL_JUNE, 4));
}

// -------------------------------------

void
DateFormatTest::parse2DigitYear(DateFormat& fmt, const char* str, UDate expected)
{
    UErrorCode status = U_ZERO_ERROR;
    //try {
        UDate d = fmt.parse(str, status);
        UnicodeString thePat;
        logln(UnicodeString("Parsing \"") + str + "\" with " + ((SimpleDateFormat*)&fmt)->toPattern(thePat) +
            "  => " + dateToString(d));
        if (d != expected) errln((UnicodeString)"FAIL: Expected " + expected);
    //}
    //catch(ParseException e) {
        if (U_FAILURE(status))
        errln((UnicodeString)"FAIL: Got exception");
    //}
}

// -------------------------------------

/**
 * Test the formatting of time zones.
 */
void
DateFormatTest::TestDateFormatZone061()
{
    UErrorCode status = U_ZERO_ERROR;
    UDate date;
    DateFormat *formatter;
    date= 859248000000.0;
    logln((UnicodeString)"Date 1997/3/25 00:00 GMT: " + date);
    formatter = new SimpleDateFormat((UnicodeString)"dd-MMM-yyyyy HH:mm", Locale::getUK(), status);
    if(U_FAILURE(status)) {
      dataerrln("Failed creating SimpleDateFormat with %s. Quitting test", u_errorName(status));
      delete formatter;
      return;
    }
    formatter->adoptTimeZone(TimeZone::createTimeZone("GMT"));
    UnicodeString temp; formatter->format(date, temp);
    logln((UnicodeString)"Formatted in GMT to: " + temp);
    //try {
        UDate tempDate = formatter->parse(temp, status);
        logln((UnicodeString)"Parsed to: " + dateToString(tempDate));
        if (tempDate != date) errln((UnicodeString)"FAIL: Expected " + dateToString(date));
    //}
    //catch(Throwable t) {
    if (U_FAILURE(status))
        errln((UnicodeString)"Date Formatter throws: " + (int32_t)status);
    //}
    delete formatter;
}

// -------------------------------------

/**
 * Test the formatting of time zones.
 */
void
DateFormatTest::TestDateFormatZone146()
{
    TimeZone *saveDefault = TimeZone::createDefault();

        //try {
    TimeZone *thedefault = TimeZone::createTimeZone("GMT");
    TimeZone::setDefault(*thedefault);
            // java.util.Locale.setDefault(new java.util.Locale("ar", "", ""));

            // check to be sure... its GMT all right
        TimeZone *testdefault = TimeZone::createDefault();
        UnicodeString testtimezone;
        testdefault->getID(testtimezone);
        if (testtimezone == "GMT")
            logln("Test timezone = " + testtimezone);
        else
            errln("Test timezone should be GMT, not " + testtimezone);

        UErrorCode status = U_ZERO_ERROR;
        // now try to use the default GMT time zone
        GregorianCalendar *greenwichcalendar =
            new GregorianCalendar(1997, 3, 4, 23, 0, status);
        if (U_FAILURE(status)) {
            dataerrln("Fail new GregorianCalendar: %s", u_errorName(status));
        } else {
            //*****************************greenwichcalendar.setTimeZone(TimeZone.getDefault());
            //greenwichcalendar.set(1997, 3, 4, 23, 0);
            // try anything to set hour to 23:00 !!!
            greenwichcalendar->set(UCAL_HOUR_OF_DAY, 23);
            // get time
            UDate greenwichdate = greenwichcalendar->getTime(status);
            // format every way
            UnicodeString DATA [] = {
                UnicodeString("simple format:  "), UnicodeString("04/04/97 23:00 GMT+00:00"),
                    UnicodeString("MM/dd/yy HH:mm z"),
                UnicodeString("full format:    "), UnicodeString("Friday, April 4, 1997 11:00:00 o'clock PM GMT+00:00"),
                    UnicodeString("EEEE, MMMM d, yyyy h:mm:ss 'o''clock' a z"),
                UnicodeString("long format:    "), UnicodeString("April 4, 1997 11:00:00 PM GMT+00:00"),
                    UnicodeString("MMMM d, yyyy h:mm:ss a z"),
                UnicodeString("default format: "), UnicodeString("04-Apr-97 11:00:00 PM"),
                    UnicodeString("dd-MMM-yy h:mm:ss a"),
                UnicodeString("short format:   "), UnicodeString("4/4/97 11:00 PM"),
                    UnicodeString("M/d/yy h:mm a")
            };
            int32_t DATA_length = (int32_t)(sizeof(DATA) / sizeof(DATA[0]));

            for (int32_t i=0; i<DATA_length; i+=3) {
                DateFormat *fmt = new SimpleDateFormat(DATA[i+2], Locale::getEnglish(), status);
                if(failure(status, "new SimpleDateFormat")) break;
                fmt->setCalendar(*greenwichcalendar);
                UnicodeString result;
                result = fmt->format(greenwichdate, result);
                logln(DATA[i] + result);
                if (result != DATA[i+1])
                    errln("FAIL: Expected " + DATA[i+1] + ", got " + result);
                delete fmt;
            }
        }
    //}
    //finally {
        TimeZone::adoptDefault(saveDefault);
    //}
        delete testdefault;
        delete greenwichcalendar;
        delete thedefault;


}

// -------------------------------------

/**
 * Test the formatting of dates in different locales.
 */
void
DateFormatTest::TestLocaleDateFormat() // Bug 495
{
    UDate testDate = date(97, UCAL_SEPTEMBER, 15);
    DateFormat *dfFrench = DateFormat::createDateTimeInstance(DateFormat::FULL,
        DateFormat::FULL, Locale::getFrench());
    DateFormat *dfUS = DateFormat::createDateTimeInstance(DateFormat::FULL,
        DateFormat::FULL, Locale::getUS());
    UnicodeString expectedFRENCH ( "lundi 15 septembre 1997 00:00:00 heure avanc\\u00E9e du Pacifique", -1, US_INV );
    expectedFRENCH = expectedFRENCH.unescape();
    //UnicodeString expectedUS ( "Monday, September 15, 1997 12:00:00 o'clock AM PDT" );
    UnicodeString expectedUS ( "Monday, September 15, 1997 12:00:00 AM Pacific Daylight Time" );
    logln((UnicodeString)"Date set to : " + dateToString(testDate));
    UnicodeString out;
    if (dfUS == NULL || dfFrench == NULL){
        dataerrln("Error calling DateFormat::createDateTimeInstance)");
        delete dfUS;
        delete dfFrench;
        return;
    }

    dfFrench->format(testDate, out);
    logln((UnicodeString)"Date Formated with French Locale " + out);
    if (!(out == expectedFRENCH))
        errln((UnicodeString)"FAIL: Expected " + expectedFRENCH);
    out.truncate(0);
    dfUS->format(testDate, out);
    logln((UnicodeString)"Date Formated with US Locale " + out);
    if (!(out == expectedUS))
        errln((UnicodeString)"FAIL: Expected " + expectedUS);
    delete dfUS;
    delete dfFrench;
}

/**
 * Test DateFormat(Calendar) API
 */
void DateFormatTest::TestDateFormatCalendar() {
    DateFormat *date=0, *time=0, *full=0;
    Calendar *cal=0;
    UnicodeString str;
    ParsePosition pos;
    UDate when;
    UErrorCode ec = U_ZERO_ERROR;

    /* Create a formatter for date fields. */
    date = DateFormat::createDateInstance(DateFormat::kShort, Locale::getUS());
    if (date == NULL) {
        dataerrln("FAIL: createDateInstance failed");
        goto FAIL;
    }

    /* Create a formatter for time fields. */
    time = DateFormat::createTimeInstance(DateFormat::kShort, Locale::getUS());
    if (time == NULL) {
        errln("FAIL: createTimeInstance failed");
        goto FAIL;
    }

    /* Create a full format for output */
    full = DateFormat::createDateTimeInstance(DateFormat::kFull, DateFormat::kFull,
                                              Locale::getUS());
    if (full == NULL) {
        errln("FAIL: createInstance failed");
        goto FAIL;
    }

    /* Create a calendar */
    cal = Calendar::createInstance(Locale::getUS(), ec);
    if (cal == NULL || U_FAILURE(ec)) {
        errln((UnicodeString)"FAIL: Calendar::createInstance failed with " +
              u_errorName(ec));
        goto FAIL;
    }

    /* Parse the date */
    cal->clear();
    str = UnicodeString("4/5/2001", "");
    pos.setIndex(0);
    date->parse(str, *cal, pos);
    if (pos.getIndex() != str.length()) {
        errln((UnicodeString)"FAIL: DateFormat::parse(4/5/2001) failed at " +
              pos.getIndex());
        goto FAIL;
    }

    /* Parse the time */
    str = UnicodeString("5:45 PM", "");
    pos.setIndex(0);
    time->parse(str, *cal, pos);
    if (pos.getIndex() != str.length()) {
        errln((UnicodeString)"FAIL: DateFormat::parse(17:45) failed at " +
              pos.getIndex());
        goto FAIL;
    }

    /* Check result */
    when = cal->getTime(ec);
    if (U_FAILURE(ec)) {
        errln((UnicodeString)"FAIL: cal->getTime() failed with " + u_errorName(ec));
        goto FAIL;
    }
    str.truncate(0);
    full->format(when, str);
    // Thursday, April 5, 2001 5:45:00 PM PDT 986517900000
    if (when == 986517900000.0) {
        logln("Ok: Parsed result: " + str);
    } else {
        errln("FAIL: Parsed result: " + str + ", exp 4/5/2001 5:45 PM");
    }

 FAIL:
    delete date;
    delete time;
    delete full;
    delete cal;
}

/**
 * Test DateFormat's parsing of space characters.  See jitterbug 1916.
 */
void DateFormatTest::TestSpaceParsing() {
    const char* DATA[] = {
        "yyyy MM dd HH:mm:ss",

        // pattern, input, expected parse or NULL if expect parse failure
        "MMMM d yy", " 04 05 06",  NULL, // MMMM wants Apr/April
        NULL,        "04 05 06",   NULL,
        "MM d yy",   " 04 05 06",  "2006 04 05 00:00:00",
        NULL,        "04 05 06",   "2006 04 05 00:00:00",
        "MMMM d yy", " Apr 05 06", "2006 04 05 00:00:00",
        NULL,        "Apr 05 06",  "2006 04 05 00:00:00",
    };
    const int32_t DATA_len = sizeof(DATA)/sizeof(DATA[0]);

    expectParse(DATA, DATA_len, Locale("en"));
}

/**
 * Test handling of "HHmmss" pattern.
 */
void DateFormatTest::TestExactCountFormat() {
    const char* DATA[] = {
        "yyyy MM dd HH:mm:ss",

        // pattern, input, expected parse or NULL if expect parse failure
        "HHmmss", "123456", "1970 01 01 12:34:56",
        NULL,     "12345",  "1970 01 01 01:23:45",
        NULL,     "1234",   NULL,
        NULL,     "00-05",  NULL,
        NULL,     "12-34",  NULL,
        NULL,     "00+05",  NULL,
        "ahhmm",  "PM730",  "1970 01 01 19:30:00",
    };
    const int32_t DATA_len = sizeof(DATA)/sizeof(DATA[0]);

    expectParse(DATA, DATA_len, Locale("en"));
}

/**
 * Test handling of white space.
 */
void DateFormatTest::TestWhiteSpaceParsing() {
    const char* DATA[] = {
        "yyyy MM dd",

        // pattern, input, expected parse or null if expect parse failure

        // Pattern space run should parse input text space run
        "MM   d yy",   " 04 01 03",    "2003 04 01",
        NULL,          " 04  01   03 ", "2003 04 01",
    };
    const int32_t DATA_len = sizeof(DATA)/sizeof(DATA[0]);

    expectParse(DATA, DATA_len, Locale("en"));
}


void DateFormatTest::TestInvalidPattern() {
    UErrorCode ec = U_ZERO_ERROR;
    SimpleDateFormat f(UnicodeString("Yesterday"), ec);
    if (U_FAILURE(ec)) {
        dataerrln("Fail construct SimpleDateFormat: %s", u_errorName(ec));
        return;
    }
    UnicodeString out;
    FieldPosition pos;
    f.format((UDate)0, out, pos);
    logln(out);
    // The bug is that the call to format() will crash.  By not
    // crashing, the test passes.
}

void DateFormatTest::TestGreekMay() {
    UErrorCode ec = U_ZERO_ERROR;
    UDate date = -9896080848000.0;
    SimpleDateFormat fmt("EEEE, dd MMMM yyyy h:mm:ss a", Locale("el", "", ""), ec);
    if (U_FAILURE(ec)) {
        dataerrln("Fail construct SimpleDateFormat: %s", u_errorName(ec));
        return;
    }
    UnicodeString str;
    fmt.format(date, str);
    ParsePosition pos(0);
    UDate d2 = fmt.parse(str, pos);
    if (date != d2) {
        errln("FAIL: unable to parse strings where case-folding changes length");
    }
}

void DateFormatTest::TestStandAloneMonths()
{
    const char *EN_DATA[] = {
        "yyyy MM dd HH:mm:ss",

        "yyyy LLLL dd H:mm:ss", "fp", "2004 03 10 16:36:31", "2004 March 10 16:36:31", "2004 03 10 16:36:31",
        "yyyy LLL dd H:mm:ss",  "fp", "2004 03 10 16:36:31", "2004 Mar 10 16:36:31",   "2004 03 10 16:36:31",
        "yyyy LLLL dd H:mm:ss", "F",  "2004 03 10 16:36:31", "2004 March 10 16:36:31",
        "yyyy LLL dd H:mm:ss",  "pf", "2004 Mar 10 16:36:31", "2004 03 10 16:36:31", "2004 Mar 10 16:36:31",

        "LLLL", "fp", "1970 01 01 0:00:00", "January",   "1970 01 01 0:00:00",
        "LLLL", "fp", "1970 02 01 0:00:00", "February",  "1970 02 01 0:00:00",
        "LLLL", "fp", "1970 03 01 0:00:00", "March",     "1970 03 01 0:00:00",
        "LLLL", "fp", "1970 04 01 0:00:00", "April",     "1970 04 01 0:00:00",
        "LLLL", "fp", "1970 05 01 0:00:00", "May",       "1970 05 01 0:00:00",
        "LLLL", "fp", "1970 06 01 0:00:00", "June",      "1970 06 01 0:00:00",
        "LLLL", "fp", "1970 07 01 0:00:00", "July",      "1970 07 01 0:00:00",
        "LLLL", "fp", "1970 08 01 0:00:00", "August",    "1970 08 01 0:00:00",
        "LLLL", "fp", "1970 09 01 0:00:00", "September", "1970 09 01 0:00:00",
        "LLLL", "fp", "1970 10 01 0:00:00", "October",   "1970 10 01 0:00:00",
        "LLLL", "fp", "1970 11 01 0:00:00", "November",  "1970 11 01 0:00:00",
        "LLLL", "fp", "1970 12 01 0:00:00", "December",  "1970 12 01 0:00:00",

        "LLL", "fp", "1970 01 01 0:00:00", "Jan", "1970 01 01 0:00:00",
        "LLL", "fp", "1970 02 01 0:00:00", "Feb", "1970 02 01 0:00:00",
        "LLL", "fp", "1970 03 01 0:00:00", "Mar", "1970 03 01 0:00:00",
        "LLL", "fp", "1970 04 01 0:00:00", "Apr", "1970 04 01 0:00:00",
        "LLL", "fp", "1970 05 01 0:00:00", "May", "1970 05 01 0:00:00",
        "LLL", "fp", "1970 06 01 0:00:00", "Jun", "1970 06 01 0:00:00",
        "LLL", "fp", "1970 07 01 0:00:00", "Jul", "1970 07 01 0:00:00",
        "LLL", "fp", "1970 08 01 0:00:00", "Aug", "1970 08 01 0:00:00",
        "LLL", "fp", "1970 09 01 0:00:00", "Sep", "1970 09 01 0:00:00",
        "LLL", "fp", "1970 10 01 0:00:00", "Oct", "1970 10 01 0:00:00",
        "LLL", "fp", "1970 11 01 0:00:00", "Nov", "1970 11 01 0:00:00",
        "LLL", "fp", "1970 12 01 0:00:00", "Dec", "1970 12 01 0:00:00",
    };

    const char *CS_DATA[] = {
        "yyyy MM dd HH:mm:ss",

        "yyyy LLLL dd H:mm:ss", "fp", "2004 04 10 16:36:31", "2004 duben 10 16:36:31", "2004 04 10 16:36:31",
        "yyyy MMMM dd H:mm:ss", "fp", "2004 04 10 16:36:31", "2004 dubna 10 16:36:31", "2004 04 10 16:36:31",
        "yyyy LLL dd H:mm:ss",  "fp", "2004 04 10 16:36:31", "2004 4. 10 16:36:31",   "2004 04 10 16:36:31",
        "yyyy LLLL dd H:mm:ss", "F",  "2004 04 10 16:36:31", "2004 duben 10 16:36:31",
        "yyyy MMMM dd H:mm:ss", "F",  "2004 04 10 16:36:31", "2004 dubna 10 16:36:31",
        "yyyy LLLL dd H:mm:ss", "pf", "2004 duben 10 16:36:31", "2004 04 10 16:36:31", "2004 duben 10 16:36:31",
        "yyyy MMMM dd H:mm:ss", "pf", "2004 dubna 10 16:36:31", "2004 04 10 16:36:31", "2004 dubna 10 16:36:31",

        "LLLL", "fp", "1970 01 01 0:00:00", "leden",               "1970 01 01 0:00:00",
        "LLLL", "fp", "1970 02 01 0:00:00", "\\u00FAnor",           "1970 02 01 0:00:00",
        "LLLL", "fp", "1970 03 01 0:00:00", "b\\u0159ezen",         "1970 03 01 0:00:00",
        "LLLL", "fp", "1970 04 01 0:00:00", "duben",               "1970 04 01 0:00:00",
        "LLLL", "fp", "1970 05 01 0:00:00", "kv\\u011Bten",         "1970 05 01 0:00:00",
        "LLLL", "fp", "1970 06 01 0:00:00", "\\u010Derven",         "1970 06 01 0:00:00",
        "LLLL", "fp", "1970 07 01 0:00:00", "\\u010Dervenec",       "1970 07 01 0:00:00",
        "LLLL", "fp", "1970 08 01 0:00:00", "srpen",               "1970 08 01 0:00:00",
        "LLLL", "fp", "1970 09 01 0:00:00", "z\\u00E1\\u0159\\u00ED", "1970 09 01 0:00:00",
        "LLLL", "fp", "1970 10 01 0:00:00", "\\u0159\\u00EDjen",     "1970 10 01 0:00:00",
        "LLLL", "fp", "1970 11 01 0:00:00", "listopad",            "1970 11 01 0:00:00",
        "LLLL", "fp", "1970 12 01 0:00:00", "prosinec",            "1970 12 01 0:00:00",

        "LLL", "fp", "1970 01 01 0:00:00", "1.",  "1970 01 01 0:00:00",
        "LLL", "fp", "1970 02 01 0:00:00", "2.",  "1970 02 01 0:00:00",
        "LLL", "fp", "1970 03 01 0:00:00", "3.",  "1970 03 01 0:00:00",
        "LLL", "fp", "1970 04 01 0:00:00", "4.",  "1970 04 01 0:00:00",
        "LLL", "fp", "1970 05 01 0:00:00", "5.",  "1970 05 01 0:00:00",
        "LLL", "fp", "1970 06 01 0:00:00", "6.",  "1970 06 01 0:00:00",
        "LLL", "fp", "1970 07 01 0:00:00", "7.",  "1970 07 01 0:00:00",
        "LLL", "fp", "1970 08 01 0:00:00", "8.",  "1970 08 01 0:00:00",
        "LLL", "fp", "1970 09 01 0:00:00", "9.",  "1970 09 01 0:00:00",
        "LLL", "fp", "1970 10 01 0:00:00", "10.", "1970 10 01 0:00:00",
        "LLL", "fp", "1970 11 01 0:00:00", "11.", "1970 11 01 0:00:00",
        "LLL", "fp", "1970 12 01 0:00:00", "12.", "1970 12 01 0:00:00",
    };

    expect(EN_DATA, ARRAY_SIZE(EN_DATA), Locale("en", "", ""));
    expect(CS_DATA, ARRAY_SIZE(CS_DATA), Locale("cs", "", ""));
}

void DateFormatTest::TestStandAloneDays()
{
    const char *EN_DATA[] = {
        "yyyy MM dd HH:mm:ss",

        "cccc", "fp", "1970 01 04 0:00:00", "Sunday",    "1970 01 04 0:00:00",
        "cccc", "fp", "1970 01 05 0:00:00", "Monday",    "1970 01 05 0:00:00",
        "cccc", "fp", "1970 01 06 0:00:00", "Tuesday",   "1970 01 06 0:00:00",
        "cccc", "fp", "1970 01 07 0:00:00", "Wednesday", "1970 01 07 0:00:00",
        "cccc", "fp", "1970 01 01 0:00:00", "Thursday",  "1970 01 01 0:00:00",
        "cccc", "fp", "1970 01 02 0:00:00", "Friday",    "1970 01 02 0:00:00",
        "cccc", "fp", "1970 01 03 0:00:00", "Saturday",  "1970 01 03 0:00:00",

        "ccc", "fp", "1970 01 04 0:00:00", "Sun", "1970 01 04 0:00:00",
        "ccc", "fp", "1970 01 05 0:00:00", "Mon", "1970 01 05 0:00:00",
        "ccc", "fp", "1970 01 06 0:00:00", "Tue", "1970 01 06 0:00:00",
        "ccc", "fp", "1970 01 07 0:00:00", "Wed", "1970 01 07 0:00:00",
        "ccc", "fp", "1970 01 01 0:00:00", "Thu", "1970 01 01 0:00:00",
        "ccc", "fp", "1970 01 02 0:00:00", "Fri", "1970 01 02 0:00:00",
        "ccc", "fp", "1970 01 03 0:00:00", "Sat", "1970 01 03 0:00:00",
    };

    const char *CS_DATA[] = {
        "yyyy MM dd HH:mm:ss",

        "cccc", "fp", "1970 01 04 0:00:00", "ned\\u011Ble",       "1970 01 04 0:00:00",
        "cccc", "fp", "1970 01 05 0:00:00", "pond\\u011Bl\\u00ED", "1970 01 05 0:00:00",
        "cccc", "fp", "1970 01 06 0:00:00", "\\u00FAter\\u00FD",   "1970 01 06 0:00:00",
        "cccc", "fp", "1970 01 07 0:00:00", "st\\u0159eda",       "1970 01 07 0:00:00",
        "cccc", "fp", "1970 01 01 0:00:00", "\\u010Dtvrtek",      "1970 01 01 0:00:00",
        "cccc", "fp", "1970 01 02 0:00:00", "p\\u00E1tek",        "1970 01 02 0:00:00",
        "cccc", "fp", "1970 01 03 0:00:00", "sobota",            "1970 01 03 0:00:00",

        "ccc", "fp", "1970 01 04 0:00:00", "ne",      "1970 01 04 0:00:00",
        "ccc", "fp", "1970 01 05 0:00:00", "po",      "1970 01 05 0:00:00",
        "ccc", "fp", "1970 01 06 0:00:00", "\\u00FAt", "1970 01 06 0:00:00",
        "ccc", "fp", "1970 01 07 0:00:00", "st",      "1970 01 07 0:00:00",
        "ccc", "fp", "1970 01 01 0:00:00", "\\u010Dt", "1970 01 01 0:00:00",
        "ccc", "fp", "1970 01 02 0:00:00", "p\\u00E1", "1970 01 02 0:00:00",
        "ccc", "fp", "1970 01 03 0:00:00", "so",      "1970 01 03 0:00:00",
    };

    expect(EN_DATA, ARRAY_SIZE(EN_DATA), Locale("en", "", ""));
    expect(CS_DATA, ARRAY_SIZE(CS_DATA), Locale("cs", "", ""));
}

void DateFormatTest::TestNarrowNames()
{
    const char *EN_DATA[] = {
            "yyyy MM dd HH:mm:ss",

            "yyyy MMMMM dd H:mm:ss", "2004 03 10 16:36:31", "2004 M 10 16:36:31",
            "yyyy LLLLL dd H:mm:ss",  "2004 03 10 16:36:31", "2004 M 10 16:36:31",

            "MMMMM", "1970 01 01 0:00:00", "J",
            "MMMMM", "1970 02 01 0:00:00", "F",
            "MMMMM", "1970 03 01 0:00:00", "M",
            "MMMMM", "1970 04 01 0:00:00", "A",
            "MMMMM", "1970 05 01 0:00:00", "M",
            "MMMMM", "1970 06 01 0:00:00", "J",
            "MMMMM", "1970 07 01 0:00:00", "J",
            "MMMMM", "1970 08 01 0:00:00", "A",
            "MMMMM", "1970 09 01 0:00:00", "S",
            "MMMMM", "1970 10 01 0:00:00", "O",
            "MMMMM", "1970 11 01 0:00:00", "N",
            "MMMMM", "1970 12 01 0:00:00", "D",

            "LLLLL", "1970 01 01 0:00:00", "J",
            "LLLLL", "1970 02 01 0:00:00", "F",
            "LLLLL", "1970 03 01 0:00:00", "M",
            "LLLLL", "1970 04 01 0:00:00", "A",
            "LLLLL", "1970 05 01 0:00:00", "M",
            "LLLLL", "1970 06 01 0:00:00", "J",
            "LLLLL", "1970 07 01 0:00:00", "J",
            "LLLLL", "1970 08 01 0:00:00", "A",
            "LLLLL", "1970 09 01 0:00:00", "S",
            "LLLLL", "1970 10 01 0:00:00", "O",
            "LLLLL", "1970 11 01 0:00:00", "N",
            "LLLLL", "1970 12 01 0:00:00", "D",

            "EEEEE", "1970 01 04 0:00:00", "S",
            "EEEEE", "1970 01 05 0:00:00", "M",
            "EEEEE", "1970 01 06 0:00:00", "T",
            "EEEEE", "1970 01 07 0:00:00", "W",
            "EEEEE", "1970 01 01 0:00:00", "T",
            "EEEEE", "1970 01 02 0:00:00", "F",
            "EEEEE", "1970 01 03 0:00:00", "S",

            "ccccc", "1970 01 04 0:00:00", "S",
            "ccccc", "1970 01 05 0:00:00", "M",
            "ccccc", "1970 01 06 0:00:00", "T",
            "ccccc", "1970 01 07 0:00:00", "W",
            "ccccc", "1970 01 01 0:00:00", "T",
            "ccccc", "1970 01 02 0:00:00", "F",
            "ccccc", "1970 01 03 0:00:00", "S",
        };

        const char *CS_DATA[] = {
            "yyyy MM dd HH:mm:ss",

            "yyyy LLLLL dd H:mm:ss", "2004 04 10 16:36:31", "2004 d 10 16:36:31",
            "yyyy MMMMM dd H:mm:ss", "2004 04 10 16:36:31", "2004 d 10 16:36:31",

            "MMMMM", "1970 01 01 0:00:00", "l",
            "MMMMM", "1970 02 01 0:00:00", "\\u00FA",
            "MMMMM", "1970 03 01 0:00:00", "b",
            "MMMMM", "1970 04 01 0:00:00", "d",
            "MMMMM", "1970 05 01 0:00:00", "k",
            "MMMMM", "1970 06 01 0:00:00", "\\u010D",
            "MMMMM", "1970 07 01 0:00:00", "\\u010D",
            "MMMMM", "1970 08 01 0:00:00", "s",
            "MMMMM", "1970 09 01 0:00:00", "z",
            "MMMMM", "1970 10 01 0:00:00", "\\u0159",
            "MMMMM", "1970 11 01 0:00:00", "l",
            "MMMMM", "1970 12 01 0:00:00", "p",

            "LLLLL", "1970 01 01 0:00:00", "l",
            "LLLLL", "1970 02 01 0:00:00", "\\u00FA",
            "LLLLL", "1970 03 01 0:00:00", "b",
            "LLLLL", "1970 04 01 0:00:00", "d",
            "LLLLL", "1970 05 01 0:00:00", "k",
            "LLLLL", "1970 06 01 0:00:00", "\\u010D",
            "LLLLL", "1970 07 01 0:00:00", "\\u010D",
            "LLLLL", "1970 08 01 0:00:00", "s",
            "LLLLL", "1970 09 01 0:00:00", "z",
            "LLLLL", "1970 10 01 0:00:00", "\\u0159",
            "LLLLL", "1970 11 01 0:00:00", "l",
            "LLLLL", "1970 12 01 0:00:00", "p",

            "EEEEE", "1970 01 04 0:00:00", "N",
            "EEEEE", "1970 01 05 0:00:00", "P",
            "EEEEE", "1970 01 06 0:00:00", "\\u00DA",
            "EEEEE", "1970 01 07 0:00:00", "S",
            "EEEEE", "1970 01 01 0:00:00", "\\u010C",
            "EEEEE", "1970 01 02 0:00:00", "P",
            "EEEEE", "1970 01 03 0:00:00", "S",

            "ccccc", "1970 01 04 0:00:00", "N",
            "ccccc", "1970 01 05 0:00:00", "P",
            "ccccc", "1970 01 06 0:00:00", "\\u00DA",
            "ccccc", "1970 01 07 0:00:00", "S",
            "ccccc", "1970 01 01 0:00:00", "\\u010C",
            "ccccc", "1970 01 02 0:00:00", "P",
            "ccccc", "1970 01 03 0:00:00", "S",
        };

      expectFormat(EN_DATA, ARRAY_SIZE(EN_DATA), Locale("en", "", ""));
      expectFormat(CS_DATA, ARRAY_SIZE(CS_DATA), Locale("cs", "", ""));
}

void DateFormatTest::TestEras()
{
    const char *EN_DATA[] = {
        "yyyy MM dd",

        "MMMM dd yyyy G",    "fp", "1951 07 17", "July 17 1951 AD",          "1951 07 17",
        "MMMM dd yyyy GG",   "fp", "1951 07 17", "July 17 1951 AD",          "1951 07 17",
        "MMMM dd yyyy GGG",  "fp", "1951 07 17", "July 17 1951 AD",          "1951 07 17",
        "MMMM dd yyyy GGGG", "fp", "1951 07 17", "July 17 1951 Anno Domini", "1951 07 17",

        "MMMM dd yyyy G",    "fp", "-438 07 17", "July 17 0439 BC",            "-438 07 17",
        "MMMM dd yyyy GG",   "fp", "-438 07 17", "July 17 0439 BC",            "-438 07 17",
        "MMMM dd yyyy GGG",  "fp", "-438 07 17", "July 17 0439 BC",            "-438 07 17",
        "MMMM dd yyyy GGGG", "fp", "-438 07 17", "July 17 0439 Before Christ", "-438 07 17",
    };

    expect(EN_DATA, ARRAY_SIZE(EN_DATA), Locale("en", "", ""));
}

void DateFormatTest::TestQuarters()
{
    const char *EN_DATA[] = {
        "yyyy MM dd",

        "Q",    "fp", "1970 01 01", "1",           "1970 01 01",
        "QQ",   "fp", "1970 04 01", "02",          "1970 04 01",
        "QQQ",  "fp", "1970 07 01", "Q3",          "1970 07 01",
        "QQQQ", "fp", "1970 10 01", "4th quarter", "1970 10 01",

        "q",    "fp", "1970 01 01", "1",           "1970 01 01",
        "qq",   "fp", "1970 04 01", "02",          "1970 04 01",
        "qqq",  "fp", "1970 07 01", "Q3",          "1970 07 01",
        "qqqq", "fp", "1970 10 01", "4th quarter", "1970 10 01",
    };

    expect(EN_DATA, ARRAY_SIZE(EN_DATA), Locale("en", "", ""));
}

/**
 * Test parsing.  Input is an array that starts with the following
 * header:
 *
 * [0]   = pattern string to parse [i+2] with
 *
 * followed by test cases, each of which is 3 array elements:
 *
 * [i]   = pattern, or NULL to reuse prior pattern
 * [i+1] = input string
 * [i+2] = expected parse result (parsed with pattern [0])
 *
 * If expect parse failure, then [i+2] should be NULL.
 */
void DateFormatTest::expectParse(const char** data, int32_t data_length,
                                 const Locale& loc) {
    const UDate FAIL = (UDate) -1;
    const UnicodeString FAIL_STR("parse failure");
    int32_t i = 0;

    UErrorCode ec = U_ZERO_ERROR;
    SimpleDateFormat fmt("", loc, ec);
    SimpleDateFormat ref(data[i++], loc, ec);
    SimpleDateFormat gotfmt("G yyyy MM dd HH:mm:ss z", loc, ec);
    if (U_FAILURE(ec)) {
        dataerrln("FAIL: SimpleDateFormat constructor - %s", u_errorName(ec));
        return;
    }

    const char* currentPat = NULL;
    while (i<data_length) {
        const char* pattern  = data[i++];
        const char* input    = data[i++];
        const char* expected = data[i++];

        ec = U_ZERO_ERROR;
        if (pattern != NULL) {
            fmt.applyPattern(pattern);
            currentPat = pattern;
        }
        UDate got = fmt.parse(input, ec);
        UnicodeString gotstr(FAIL_STR);
        if (U_FAILURE(ec)) {
            got = FAIL;
        } else {
            gotstr.remove();
            gotfmt.format(got, gotstr);
        }

        UErrorCode ec2 = U_ZERO_ERROR;
        UDate exp = FAIL;
        UnicodeString expstr(FAIL_STR);
        if (expected != NULL) {
            expstr = expected;
            exp = ref.parse(expstr, ec2);
            if (U_FAILURE(ec2)) {
                // This only happens if expected is in wrong format --
                // should never happen once test is debugged.
                errln("FAIL: Internal test error");
                return;
            }
        }

        if (got == exp) {
            logln((UnicodeString)"Ok: " + input + " x " +
                  currentPat + " => " + gotstr);
        } else {
            errln((UnicodeString)"FAIL: " + input + " x " +
                  currentPat + " => " + gotstr + ", expected " +
                  expstr);
        }
    }
}

/**
 * Test formatting and parsing.  Input is an array that starts
 * with the following header:
 *
 * [0]   = pattern string to parse [i+2] with
 *
 * followed by test cases, each of which is 3 array elements:
 *
 * [i]   = pattern, or null to reuse prior pattern
 * [i+1] = control string, either "fp", "pf", or "F".
 * [i+2..] = data strings
 *
 * The number of data strings depends on the control string.
 * Examples:
 * 1. "y/M/d H:mm:ss.SS", "fp", "2004 03 10 16:36:31.567", "2004/3/10 16:36:31.56", "2004 03 10 16:36:31.560",
 * 'f': Format date [i+2] (as parsed using pattern [0]) and expect string [i+3].
 * 'p': Parse string [i+3] and expect date [i+4].
 *
 * 2. "y/M/d H:mm:ss.SSS", "F", "2004 03 10 16:36:31.567", "2004/3/10 16:36:31.567"
 * 'F': Format date [i+2] and expect string [i+3],
 *      then parse string [i+3] and expect date [i+2].
 *
 * 3. "y/M/d H:mm:ss.SSSS", "pf", "2004/3/10 16:36:31.5679", "2004 03 10 16:36:31.567", "2004/3/10 16:36:31.5670",
 * 'p': Parse string [i+2] and expect date [i+3].
 * 'f': Format date [i+3] and expect string [i+4].
 */
void DateFormatTest::expect(const char** data, int32_t data_length,
                            const Locale& loc) {
    int32_t i = 0;
    UErrorCode ec = U_ZERO_ERROR;
    UnicodeString str, str2;
    SimpleDateFormat fmt("", loc, ec);
    SimpleDateFormat ref(data[i++], loc, ec);
    SimpleDateFormat univ("EE G yyyy MM dd HH:mm:ss.SSS z", loc, ec);
    if (U_FAILURE(ec)) {
        dataerrln("Fail construct SimpleDateFormat: %s", u_errorName(ec));
        return;
    }

    UnicodeString currentPat;
    while (i<data_length) {
        const char* pattern  = data[i++];
        if (pattern != NULL) {
            fmt.applyPattern(pattern);
            currentPat = pattern;
        }

        const char* control = data[i++];

        if (uprv_strcmp(control, "fp") == 0) {
            // 'f'
            const char* datestr = data[i++];
            const char* string = data[i++];
            UDate date = ref.parse(ctou(datestr), ec);
            if (!assertSuccess("parse", ec)) return;
            assertEquals((UnicodeString)"\"" + currentPat + "\".format(" + datestr + ")",
                         ctou(string),
                         fmt.format(date, str.remove()));
            // 'p'
            datestr = data[i++];
            date = ref.parse(ctou(datestr), ec);
            if (!assertSuccess("parse", ec)) return;
            UDate parsedate = fmt.parse(ctou(string), ec);
            if (assertSuccess((UnicodeString)"\"" + currentPat + "\".parse(" + string + ")", ec)) {
                assertEquals((UnicodeString)"\"" + currentPat + "\".parse(" + string + ")",
                             univ.format(date, str.remove()),
                             univ.format(parsedate, str2.remove()));
            }
        }

        else if (uprv_strcmp(control, "pf") == 0) {
            // 'p'
            const char* string = data[i++];
            const char* datestr = data[i++];
            UDate date = ref.parse(ctou(datestr), ec);
            if (!assertSuccess("parse", ec)) return;
            UDate parsedate = fmt.parse(ctou(string), ec);
            if (assertSuccess((UnicodeString)"\"" + currentPat + "\".parse(" + string + ")", ec)) {
                assertEquals((UnicodeString)"\"" + currentPat + "\".parse(" + string + ")",
                             univ.format(date, str.remove()),
                             univ.format(parsedate, str2.remove()));
            }
            // 'f'
            string = data[i++];
            assertEquals((UnicodeString)"\"" + currentPat + "\".format(" + datestr + ")",
                         ctou(string),
                         fmt.format(date, str.remove()));
        }

        else if (uprv_strcmp(control, "F") == 0) {
            const char* datestr  = data[i++];
            const char* string   = data[i++];
            UDate date = ref.parse(ctou(datestr), ec);
            if (!assertSuccess("parse", ec)) return;
            assertEquals((UnicodeString)"\"" + currentPat + "\".format(" + datestr + ")",
                         ctou(string),
                         fmt.format(date, str.remove()));

            UDate parsedate = fmt.parse(string, ec);
            if (assertSuccess((UnicodeString)"\"" + currentPat + "\".parse(" + string + ")", ec)) {
                assertEquals((UnicodeString)"\"" + currentPat + "\".parse(" + string + ")",
                             univ.format(date, str.remove()),
                             univ.format(parsedate, str2.remove()));
            }
        }

        else {
            errln((UnicodeString)"FAIL: Invalid control string " + control);
            return;
        }
    }
}

/**
 * Test formatting.  Input is an array that starts
 * with the following header:
 *
 * [0]   = pattern string to parse [i+2] with
 *
 * followed by test cases, each of which is 3 array elements:
 *
 * [i]   = pattern, or null to reuse prior pattern
 * [i+1] = data string a
 * [i+2] = data string b
 *
 * Examples:
 * Format date [i+1] and expect string [i+2].
 *
 * "y/M/d H:mm:ss.SSSS", "2004/3/10 16:36:31.5679", "2004 03 10 16:36:31.567"
 */
void DateFormatTest::expectFormat(const char** data, int32_t data_length,
                            const Locale& loc) {
    int32_t i = 0;
    UErrorCode ec = U_ZERO_ERROR;
    UnicodeString str, str2;
    SimpleDateFormat fmt("", loc, ec);
    SimpleDateFormat ref(data[i++], loc, ec);
    SimpleDateFormat univ("EE G yyyy MM dd HH:mm:ss.SSS z", loc, ec);
    if (U_FAILURE(ec)) {
        dataerrln("Fail construct SimpleDateFormat: %s", u_errorName(ec));
        return;
    }

    UnicodeString currentPat;

    while (i<data_length) {
        const char* pattern  = data[i++];
        if (pattern != NULL) {
            fmt.applyPattern(pattern);
            currentPat = pattern;
        }

        const char* datestr = data[i++];
        const char* string = data[i++];
        UDate date = ref.parse(ctou(datestr), ec);
        if (!assertSuccess("parse", ec)) return;
        assertEquals((UnicodeString)"\"" + currentPat + "\".format(" + datestr + ")",
                        ctou(string),
                        fmt.format(date, str.remove()));
    }
}

void DateFormatTest::TestGenericTime() {
  // any zone pattern should parse any zone
  const Locale en("en");
  const char* ZDATA[] = {
        "yyyy MM dd HH:mm zzz",
        // round trip
        "y/M/d H:mm zzzz", "F", "2004 01 01 01:00 PST", "2004/1/1 1:00 Pacific Standard Time",
        "y/M/d H:mm zzz", "F", "2004 01 01 01:00 PST", "2004/1/1 1:00 PST",
        "y/M/d H:mm vvvv", "F", "2004 01 01 01:00 PST", "2004/1/1 1:00 Pacific Time",
        "y/M/d H:mm v", "F", "2004 01 01 01:00 PST", "2004/1/1 1:00 PT",
        // non-generic timezone string influences dst offset even if wrong for date/time
        "y/M/d H:mm zzz", "pf", "2004/1/1 1:00 PDT", "2004 01 01 01:00 PDT", "2004/1/1 0:00 PST",
        "y/M/d H:mm vvvv", "pf", "2004/1/1 1:00 PDT", "2004 01 01 01:00 PDT", "2004/1/1 0:00 Pacific Time",
        "y/M/d H:mm zzz", "pf", "2004/7/1 1:00 PST", "2004 07 01 02:00 PDT", "2004/7/1 2:00 PDT",
        "y/M/d H:mm vvvv", "pf", "2004/7/1 1:00 PST", "2004 07 01 02:00 PDT", "2004/7/1 2:00 Pacific Time",
        // generic timezone generates dst offset appropriate for local time
        "y/M/d H:mm zzz", "pf", "2004/1/1 1:00 PT", "2004 01 01 01:00 PST", "2004/1/1 1:00 PST",
        "y/M/d H:mm vvvv", "pf", "2004/1/1 1:00 PT", "2004 01 01 01:00 PST", "2004/1/1 1:00 Pacific Time",
        "y/M/d H:mm zzz", "pf", "2004/7/1 1:00 PT", "2004 07 01 01:00 PDT", "2004/7/1 1:00 PDT",
        "y/M/d H:mm vvvv", "pf", "2004/7/1 1:00 PT", "2004 07 01 01:00 PDT", "2004/7/1 1:00 Pacific Time",
        // daylight savings time transition edge cases.
        // time to parse does not really exist, PT interpreted as earlier time
        "y/M/d H:mm zzz", "pf", "2005/4/3 2:30 PT", "2005 04 03 03:30 PDT", "2005/4/3 3:30 PDT",
        "y/M/d H:mm zzz", "pf", "2005/4/3 2:30 PST", "2005 04 03 03:30 PDT", "2005/4/3 3:30 PDT",
        "y/M/d H:mm zzz", "pf", "2005/4/3 2:30 PDT", "2005 04 03 01:30 PST", "2005/4/3 1:30 PST",
        "y/M/d H:mm v", "pf", "2005/4/3 2:30 PT", "2005 04 03 03:30 PDT", "2005/4/3 3:30 PT",
        "y/M/d H:mm v", "pf", "2005/4/3 2:30 PST", "2005 04 03 03:30 PDT", "2005/4/3 3:30 PT",
        "y/M/d H:mm v", "pf", "2005/4/3 2:30 PDT", "2005 04 03 01:30 PST", "2005/4/3 1:30 PT",
        "y/M/d H:mm", "pf", "2005/4/3 2:30", "2005 04 03 03:30 PDT", "2005/4/3 3:30",
        // time to parse is ambiguous, PT interpreted as later time
        "y/M/d H:mm zzz", "pf", "2005/10/30 1:30 PT", "2005 10 30 01:30 PST", "2005/10/30 1:30 PST",
        "y/M/d H:mm v", "pf", "2005/10/30 1:30 PT", "2005 10 30  01:30 PST", "2005/10/30 1:30 PT",
        "y/M/d H:mm", "pf", "2005/10/30 1:30 PT", "2005 10 30 01:30 PST", "2005/10/30 1:30",

        "y/M/d H:mm zzz", "pf", "2004/10/31 1:30 PT", "2004 10 31 01:30 PST", "2004/10/31 1:30 PST",
        "y/M/d H:mm zzz", "pf", "2004/10/31 1:30 PST", "2004 10 31 01:30 PST", "2004/10/31 1:30 PST",
        "y/M/d H:mm zzz", "pf", "2004/10/31 1:30 PDT", "2004 10 31 01:30 PDT", "2004/10/31 1:30 PDT",
        "y/M/d H:mm v", "pf", "2004/10/31 1:30 PT", "2004 10 31 01:30 PST", "2004/10/31 1:30 PT",
        "y/M/d H:mm v", "pf", "2004/10/31 1:30 PST", "2004 10 31 01:30 PST", "2004/10/31 1:30 PT",
        "y/M/d H:mm v", "pf", "2004/10/31 1:30 PDT", "2004 10 31 01:30 PDT", "2004/10/31 1:30 PT",
        "y/M/d H:mm", "pf", "2004/10/31 1:30", "2004 10 31 01:30 PST", "2004/10/31 1:30",
  };
  const int32_t ZDATA_length = sizeof(ZDATA)/ sizeof(ZDATA[0]);
  expect(ZDATA, ZDATA_length, en);

  UErrorCode status = U_ZERO_ERROR;

  logln("cross format/parse tests");
  UnicodeString basepat("yy/MM/dd H:mm ");
  SimpleDateFormat formats[] = {
    SimpleDateFormat(basepat + "vvv", en, status),
    SimpleDateFormat(basepat + "vvvv", en, status),
    SimpleDateFormat(basepat + "zzz", en, status),
    SimpleDateFormat(basepat + "zzzz", en, status)
  };
  if (U_FAILURE(status)) {
    dataerrln("Fail construct SimpleDateFormat: %s", u_errorName(status));
    return;
  }
  const int32_t formats_length = sizeof(formats)/sizeof(formats[0]);

  UnicodeString test;
  SimpleDateFormat univ("yyyy MM dd HH:mm zzz", en, status);
  ASSERT_OK(status);
  const UnicodeString times[] = {
    "2004 01 02 03:04 PST",
    "2004 07 08 09:10 PDT"
  };
  int32_t times_length = sizeof(times)/sizeof(times[0]);
  for (int i = 0; i < times_length; ++i) {
    UDate d = univ.parse(times[i], status);
    logln(UnicodeString("\ntime: ") + d);
    for (int j = 0; j < formats_length; ++j) {
      test.remove();
      formats[j].format(d, test);
      logln("\ntest: '" + test + "'");
      for (int k = 0; k < formats_length; ++k) {
        UDate t = formats[k].parse(test, status);
        if (U_SUCCESS(status)) {
          if (d != t) {
            errln((UnicodeString)"FAIL: format " + k +
                  " incorrectly parsed output of format " + j +
                  " (" + test + "), returned " +
                  dateToString(t) + " instead of " + dateToString(d));
          } else {
            logln((UnicodeString)"OK: format " + k + " parsed ok");
          }
        } else if (status == U_PARSE_ERROR) {
          errln((UnicodeString)"FAIL: format " + k +
                " could not parse output of format " + j +
                " (" + test + ")");
        }
      }
    }
  }
}

void DateFormatTest::TestGenericTimeZoneOrder() {
  // generic times should parse the same no matter what the placement of the time zone string
  // should work for standard and daylight times

  const char* XDATA[] = {
    "yyyy MM dd HH:mm zzz",
    // standard time, explicit daylight/standard
    "y/M/d H:mm zzz", "pf", "2004/1/1 1:00 PT", "2004 01 01 01:00 PST", "2004/1/1 1:00 PST",
    "y/M/d zzz H:mm", "pf", "2004/1/1 PT 1:00", "2004 01 01 01:00 PST", "2004/1/1 PST 1:00",
    "zzz y/M/d H:mm", "pf", "PT 2004/1/1 1:00", "2004 01 01 01:00 PST", "PST 2004/1/1 1:00",

    // standard time, generic
    "y/M/d H:mm vvvv", "pf", "2004/1/1 1:00 PT", "2004 01 01 01:00 PST", "2004/1/1 1:00 Pacific Time",
    "y/M/d vvvv H:mm", "pf", "2004/1/1 PT 1:00", "2004 01 01 01:00 PST", "2004/1/1 Pacific Time 1:00",
    "vvvv y/M/d H:mm", "pf", "PT 2004/1/1 1:00", "2004 01 01 01:00 PST", "Pacific Time 2004/1/1 1:00",

    // dahylight time, explicit daylight/standard
    "y/M/d H:mm zzz", "pf", "2004/7/1 1:00 PT", "2004 07 01 01:00 PDT", "2004/7/1 1:00 PDT",
    "y/M/d zzz H:mm", "pf", "2004/7/1 PT 1:00", "2004 07 01 01:00 PDT", "2004/7/1 PDT 1:00",
    "zzz y/M/d H:mm", "pf", "PT 2004/7/1 1:00", "2004 07 01 01:00 PDT", "PDT 2004/7/1 1:00",

    // daylight time, generic
    "y/M/d H:mm vvvv", "pf", "2004/7/1 1:00 PT", "2004 07 01 01:00 PDT", "2004/7/1 1:00 Pacific Time",
    "y/M/d vvvv H:mm", "pf", "2004/7/1 PT 1:00", "2004 07 01 01:00 PDT", "2004/7/1 Pacific Time 1:00",
    "vvvv y/M/d H:mm", "pf", "PT 2004/7/1 1:00", "2004 07 01 01:00 PDT", "Pacific Time 2004/7/1 1:00",
  };
  const int32_t XDATA_length = sizeof(XDATA)/sizeof(XDATA[0]);
  Locale en("en");
  expect(XDATA, XDATA_length, en);
}

void DateFormatTest::TestZTimeZoneParsing(void) {
    UErrorCode status = U_ZERO_ERROR;
    const Locale en("en");
    UnicodeString test;
    //SimpleDateFormat univ("yyyy-MM-dd'T'HH:mm Z", en, status);
    SimpleDateFormat univ("HH:mm Z", en, status);
    if (failure(status, "construct SimpleDateFormat", TRUE)) return;
    const TimeZone *t = TimeZone::getGMT();
    univ.setTimeZone(*t);

    univ.setLenient(false);
    ParsePosition pp(0);
    struct {
        UnicodeString input;
        UnicodeString expected_result;
    } tests[] = {
        { "11:00 -0200", "13:00 +0000" },
        { "11:00 +0200", "09:00 +0000" },
        { "11:00 +0400", "07:00 +0000" },
        { "11:00 +0530", "05:30 +0000" }
    };

    UnicodeString result;
    int32_t tests_length = sizeof(tests)/sizeof(tests[0]);
    for (int i = 0; i < tests_length; ++i) {
        pp.setIndex(0);
        UDate d = univ.parse(tests[i].input, pp);
        if(pp.getIndex() != tests[i].input.length()){
            errln("setZoneString() did not succeed. Consumed: %i instead of %i",
                  pp.getIndex(), tests[i].input.length());
            return;
        }
        result.remove();
        univ.format(d, result);
        if(result != tests[i].expected_result) {
            errln("Expected " + tests[i].expected_result
                  + " got " + result);
            return;
        }
        logln("SUCCESS: Parsed " + tests[i].input
              + " got " + result
              + " expected " + tests[i].expected_result);
    }
}

void DateFormatTest::TestHost(void)
{
#ifdef U_WINDOWS
    Win32DateTimeTest::testLocales(this);
#endif
}

// Relative Date Tests

void DateFormatTest::TestRelative(int daysdelta,
                                  const Locale& loc,
                                  const char *expectChars) {
    char banner[25];
    sprintf(banner, "%d", daysdelta);
    UnicodeString bannerStr(banner, "");

    UErrorCode status = U_ZERO_ERROR;

    FieldPosition pos(0);
    UnicodeString test;
    Locale en("en");
    DateFormat *fullrelative = DateFormat::createDateInstance(DateFormat::kFullRelative, loc);

    if (fullrelative == NULL) {
        dataerrln("DateFormat::createDateInstance(DateFormat::kFullRelative, %s) returned NULL", loc.getName());
        return;
    }

    DateFormat *full         = DateFormat::createDateInstance(DateFormat::kFull        , loc);

    if (full == NULL) {
        errln("DateFormat::createDateInstance(DateFormat::kFull, %s) returned NULL", loc.getName());
        return;
    }

    DateFormat *en_full =         DateFormat::createDateInstance(DateFormat::kFull,         en);

    if (en_full == NULL) {
        errln("DateFormat::createDateInstance(DateFormat::kFull, en) returned NULL");
        return;
    }

    DateFormat *en_fulltime =         DateFormat::createDateTimeInstance(DateFormat::kFull,DateFormat::kFull,en);

    if (en_fulltime == NULL) {
        errln("DateFormat::createDateTimeInstance(DateFormat::kFull, DateFormat::kFull, en) returned NULL");
        return;
    }

    UnicodeString result;
    UnicodeString normalResult;
    UnicodeString expect;
    UnicodeString parseResult;

    Calendar *c = Calendar::createInstance(status);

    // Today = Today
    c->setTime(Calendar::getNow(), status);
    if(daysdelta != 0) {
        c->add(Calendar::DATE,daysdelta,status);
    }
    ASSERT_OK(status);

    // calculate the expected string
    if(expectChars != NULL) {
        expect = expectChars;
    } else {
        full->format(*c, expect, pos); // expected = normal full
    }

    fullrelative   ->format(*c, result, pos);
    en_full        ->format(*c, normalResult, pos);

    if(result != expect) {
        errln("FAIL: Relative Format ["+bannerStr+"] of "+normalResult+" failed, expected "+expect+" but got " + result);
    } else {
        logln("PASS: Relative Format ["+bannerStr+"] of "+normalResult+" got " + result);
    }


    //verify
    UDate d = fullrelative->parse(result, status);
    ASSERT_OK(status);

    UnicodeString parseFormat; // parse rel->format full
    en_full->format(d, parseFormat, status);

    UnicodeString origFormat;
    en_full->format(*c, origFormat, pos);

    if(parseFormat!=origFormat) {
        errln("FAIL: Relative Parse ["+bannerStr+"] of "+result+" failed, expected "+parseFormat+" but got "+origFormat);
    } else {
        logln("PASS: Relative Parse ["+bannerStr+"] of "+result+" passed, got "+parseFormat);
    }

    delete full;
    delete fullrelative;
    delete en_fulltime;
    delete en_full;
    delete c;
}


void DateFormatTest::TestRelative(void)
{
    Locale en("en");
    TestRelative( 0, en, "Today");
    TestRelative(-1, en, "Yesterday");
    TestRelative( 1, en, "Tomorrow");
    TestRelative( 2, en, NULL);
    TestRelative( -2, en, NULL);
    TestRelative( 3, en, NULL);
    TestRelative( -3, en, NULL);
    TestRelative( 300, en, NULL);
    TestRelative( -300, en, NULL);
}

void DateFormatTest::TestRelativeClone(void)
{
    /*
    Verify that a cloned formatter gives the same results
    and is useable after the original has been deleted.
    */
    UErrorCode status = U_ZERO_ERROR;
    Locale loc("en");
    UDate now = Calendar::getNow();
    DateFormat *full = DateFormat::createDateInstance(DateFormat::kFullRelative, loc);
    if (full == NULL) {
        dataerrln("FAIL: Can't create Relative date instance");
        return;
    }
    UnicodeString result1;
    full->format(now, result1, status);
    Format *fullClone = full->clone();
    delete full;
    full = NULL;

    UnicodeString result2;
    fullClone->format(now, result2, status);
    ASSERT_OK(status);
    if (result1 != result2) {
        errln("FAIL: Clone returned different result from non-clone.");
    }
    delete fullClone;
}

void DateFormatTest::TestHostClone(void)
{
    /*
    Verify that a cloned formatter gives the same results
    and is useable after the original has been deleted.
    */
    // This is mainly important on Windows.
    UErrorCode status = U_ZERO_ERROR;
    Locale loc("en_US@compat=host");
    UDate now = Calendar::getNow();
    DateFormat *full = DateFormat::createDateInstance(DateFormat::kFull, loc);
    if (full == NULL) {
        dataerrln("FAIL: Can't create Relative date instance");
        return;
    }
    UnicodeString result1;
    full->format(now, result1, status);
    Format *fullClone = full->clone();
    delete full;
    full = NULL;

    UnicodeString result2;
    fullClone->format(now, result2, status);
    ASSERT_OK(status);
    if (result1 != result2) {
        errln("FAIL: Clone returned different result from non-clone.");
    }
    delete fullClone;
}

void DateFormatTest::TestTimeZoneDisplayName()
{
    // This test data was ported from ICU4J.  Don't know why the 6th column in there because it's not being
    // used currently.
    const char *fallbackTests[][6]  = {
        { "en", "America/Los_Angeles", "2004-01-15T00:00:00Z", "Z", "-0800", "-8:00" },
        { "en", "America/Los_Angeles", "2004-01-15T00:00:00Z", "ZZZZ", "GMT-08:00", "-8:00" },
        { "en", "America/Los_Angeles", "2004-01-15T00:00:00Z", "z", "PST", "America/Los_Angeles" },
        { "en", "America/Los_Angeles", "2004-01-15T00:00:00Z", "V", "PST", "America/Los_Angeles" },
        { "en", "America/Los_Angeles", "2004-01-15T00:00:00Z", "zzzz", "Pacific Standard Time", "America/Los_Angeles" },
        { "en", "America/Los_Angeles", "2004-07-15T00:00:00Z", "Z", "-0700", "-7:00" },
        { "en", "America/Los_Angeles", "2004-07-15T00:00:00Z", "ZZZZ", "GMT-07:00", "-7:00" },
        { "en", "America/Los_Angeles", "2004-07-15T00:00:00Z", "z", "PDT", "America/Los_Angeles" },
        { "en", "America/Los_Angeles", "2004-07-15T00:00:00Z", "V", "PDT", "America/Los_Angeles" },
        { "en", "America/Los_Angeles", "2004-07-15T00:00:00Z", "zzzz", "Pacific Daylight Time", "America/Los_Angeles" },
        { "en", "America/Los_Angeles", "2004-07-15T00:00:00Z", "v", "PT", "America/Los_Angeles" },
        { "en", "America/Los_Angeles", "2004-07-15T00:00:00Z", "vvvv", "Pacific Time", "America/Los_Angeles" },
        { "en", "America/Los_Angeles", "2004-07-15T00:00:00Z", "VVVV", "United States (Los Angeles)", "America/Los_Angeles" },
        { "en_GB", "America/Los_Angeles", "2004-01-15T12:00:00Z", "z", "PST", "America/Los_Angeles" },
        { "en", "America/Phoenix", "2004-01-15T00:00:00Z", "Z", "-0700", "-7:00" },
        { "en", "America/Phoenix", "2004-01-15T00:00:00Z", "ZZZZ", "GMT-07:00", "-7:00" },
        { "en", "America/Phoenix", "2004-01-15T00:00:00Z", "z", "MST", "America/Phoenix" },
        { "en", "America/Phoenix", "2004-01-15T00:00:00Z", "V", "MST", "America/Phoenix" },
        { "en", "America/Phoenix", "2004-01-15T00:00:00Z", "zzzz", "Mountain Standard Time", "America/Phoenix" },
        { "en", "America/Phoenix", "2004-07-15T00:00:00Z", "Z", "-0700", "-7:00" },
        { "en", "America/Phoenix", "2004-07-15T00:00:00Z", "ZZZZ", "GMT-07:00", "-7:00" },
        { "en", "America/Phoenix", "2004-07-15T00:00:00Z", "z", "MST", "America/Phoenix" },
        { "en", "America/Phoenix", "2004-07-15T00:00:00Z", "V", "MST", "America/Phoenix" },
        { "en", "America/Phoenix", "2004-07-15T00:00:00Z", "zzzz", "Mountain Standard Time", "America/Phoenix" },
        { "en", "America/Phoenix", "2004-07-15T00:00:00Z", "v", "MST", "America/Phoenix" },
        { "en", "America/Phoenix", "2004-07-15T00:00:00Z", "vvvv", "Mountain Standard Time", "America/Phoenix" },
        { "en", "America/Phoenix", "2004-07-15T00:00:00Z", "VVVV", "United States (Phoenix)", "America/Phoenix" },

        { "en", "America/Argentina/Buenos_Aires", "2004-01-15T00:00:00Z", "Z", "-0300", "-3:00" },
        { "en", "America/Argentina/Buenos_Aires", "2004-01-15T00:00:00Z", "ZZZZ", "GMT-03:00", "-3:00" },
        { "en", "America/Argentina/Buenos_Aires", "2004-01-15T00:00:00Z", "z", "GMT-03:00", "-3:00" },
        { "en", "America/Argentina/Buenos_Aires", "2004-01-15T00:00:00Z", "V", "ART", "-3:00" },
        { "en", "America/Argentina/Buenos_Aires", "2004-01-15T00:00:00Z", "zzzz", "Argentina Time", "-3:00" },
        { "en", "America/Argentina/Buenos_Aires", "2004-07-15T00:00:00Z", "Z", "-0300", "-3:00" },
        { "en", "America/Argentina/Buenos_Aires", "2004-07-15T00:00:00Z", "ZZZZ", "GMT-03:00", "-3:00" },
        { "en", "America/Argentina/Buenos_Aires", "2004-07-15T00:00:00Z", "z", "GMT-03:00", "-3:00" },
        { "en", "America/Argentina/Buenos_Aires", "2004-07-15T00:00:00Z", "V", "ART", "-3:00" },
        { "en", "America/Argentina/Buenos_Aires", "2004-07-15T00:00:00Z", "zzzz", "Argentina Time", "-3:00" },
        { "en", "America/Argentina/Buenos_Aires", "2004-07-15T00:00:00Z", "v", "Argentina (Buenos Aires)", "America/Buenos_Aires" },
        { "en", "America/Argentina/Buenos_Aires", "2004-07-15T00:00:00Z", "vvvv", "Argentina Time", "America/Buenos_Aires" },
        { "en", "America/Argentina/Buenos_Aires", "2004-07-15T00:00:00Z", "VVVV", "Argentina (Buenos Aires)", "America/Buenos_Aires" },

        { "en", "America/Buenos_Aires", "2004-01-15T00:00:00Z", "Z", "-0300", "-3:00" },
        { "en", "America/Buenos_Aires", "2004-01-15T00:00:00Z", "ZZZZ", "GMT-03:00", "-3:00" },
        { "en", "America/Buenos_Aires", "2004-01-15T00:00:00Z", "z", "GMT-03:00", "-3:00" },
        { "en", "America/Buenos_Aires", "2004-01-15T00:00:00Z", "V", "ART", "-3:00" },
        { "en", "America/Buenos_Aires", "2004-01-15T00:00:00Z", "zzzz", "Argentina Time", "-3:00" },
        { "en", "America/Buenos_Aires", "2004-07-15T00:00:00Z", "Z", "-0300", "-3:00" },
        { "en", "America/Buenos_Aires", "2004-07-15T00:00:00Z", "ZZZZ", "GMT-03:00", "-3:00" },
        { "en", "America/Buenos_Aires", "2004-07-15T00:00:00Z", "z", "GMT-03:00", "-3:00" },
        { "en", "America/Buenos_Aires", "2004-07-15T00:00:00Z", "V", "ART", "-3:00" },
        { "en", "America/Buenos_Aires", "2004-07-15T00:00:00Z", "zzzz", "Argentina Time", "-3:00" },
        { "en", "America/Buenos_Aires", "2004-07-15T00:00:00Z", "v", "Argentina (Buenos Aires)", "America/Buenos_Aires" },
        { "en", "America/Buenos_Aires", "2004-07-15T00:00:00Z", "vvvv", "Argentina Time", "America/Buenos_Aires" },
        { "en", "America/Buenos_Aires", "2004-07-15T00:00:00Z", "VVVV", "Argentina (Buenos Aires)", "America/Buenos_Aires" },

        { "en", "America/Havana", "2004-01-15T00:00:00Z", "Z", "-0500", "-5:00" },
        { "en", "America/Havana", "2004-01-15T00:00:00Z", "ZZZZ", "GMT-05:00", "-5:00" },
        { "en", "America/Havana", "2004-01-15T00:00:00Z", "z", "GMT-05:00", "-5:00" },
        { "en", "America/Havana", "2004-01-15T00:00:00Z", "V", "CST (Cuba)", "-5:00" },
        { "en", "America/Havana", "2004-01-15T00:00:00Z", "zzzz", "Cuba Standard Time", "-5:00" },
        { "en", "America/Havana", "2004-07-15T00:00:00Z", "Z", "-0400", "-4:00" },
        { "en", "America/Havana", "2004-07-15T00:00:00Z", "ZZZZ", "GMT-04:00", "-4:00" },
        { "en", "America/Havana", "2004-07-15T00:00:00Z", "z", "GMT-04:00", "-4:00" },
        { "en", "America/Havana", "2004-07-15T00:00:00Z", "V", "CDT (Cuba)", "-4:00" },
        { "en", "America/Havana", "2004-07-15T00:00:00Z", "zzzz", "Cuba Daylight Time", "-4:00" },
        { "en", "America/Havana", "2004-07-15T00:00:00Z", "v", "Cuba Time", "America/Havana" },
        { "en", "America/Havana", "2004-07-15T00:00:00Z", "vvvv", "Cuba Time", "America/Havana" },
        { "en", "America/Havana", "2004-07-15T00:00:00Z", "VVVV", "Cuba Time", "America/Havana" },

        { "en", "Australia/ACT", "2004-01-15T00:00:00Z", "Z", "+1100", "+11:00" },
        { "en", "Australia/ACT", "2004-01-15T00:00:00Z", "ZZZZ", "GMT+11:00", "+11:00" },
        { "en", "Australia/ACT", "2004-01-15T00:00:00Z", "z", "GMT+11:00", "+11:00" },
        { "en", "Australia/ACT", "2004-01-15T00:00:00Z", "V", "AEDT", "+11:00" },
        { "en", "Australia/ACT", "2004-01-15T00:00:00Z", "zzzz", "Australian Eastern Daylight Time", "+11:00" },
        { "en", "Australia/ACT", "2004-07-15T00:00:00Z", "Z", "+1000", "+10:00" },
        { "en", "Australia/ACT", "2004-07-15T00:00:00Z", "ZZZZ", "GMT+10:00", "+10:00" },
        { "en", "Australia/ACT", "2004-07-15T00:00:00Z", "z", "GMT+10:00", "+10:00" },
        { "en", "Australia/ACT", "2004-07-15T00:00:00Z", "V", "AEST", "+10:00" },
        { "en", "Australia/ACT", "2004-07-15T00:00:00Z", "zzzz", "Australian Eastern Standard Time", "+10:00" },
        { "en", "Australia/ACT", "2004-07-15T00:00:00Z", "v", "Australia (Sydney)", "Australia/Sydney" },
        { "en", "Australia/ACT", "2004-07-15T00:00:00Z", "vvvv", "Eastern Australia Time", "Australia/Sydney" },
        { "en", "Australia/ACT", "2004-07-15T00:00:00Z", "VVVV", "Australia (Sydney)", "Australia/Sydney" },

        { "en", "Australia/Sydney", "2004-01-15T00:00:00Z", "Z", "+1100", "+11:00" },
        { "en", "Australia/Sydney", "2004-01-15T00:00:00Z", "ZZZZ", "GMT+11:00", "+11:00" },
        { "en", "Australia/Sydney", "2004-01-15T00:00:00Z", "z", "GMT+11:00", "+11:00" },
        { "en", "Australia/Sydney", "2004-01-15T00:00:00Z", "V", "AEDT", "+11:00" },
        { "en", "Australia/Sydney", "2004-01-15T00:00:00Z", "zzzz", "Australian Eastern Daylight Time", "+11:00" },
        { "en", "Australia/Sydney", "2004-07-15T00:00:00Z", "Z", "+1000", "+10:00" },
        { "en", "Australia/Sydney", "2004-07-15T00:00:00Z", "ZZZZ", "GMT+10:00", "+10:00" },
        { "en", "Australia/Sydney", "2004-07-15T00:00:00Z", "z", "GMT+10:00", "+10:00" },
        { "en", "Australia/Sydney", "2004-07-15T00:00:00Z", "V", "AEST", "+10:00" },
        { "en", "Australia/Sydney", "2004-07-15T00:00:00Z", "zzzz", "Australian Eastern Standard Time", "+10:00" },
        { "en", "Australia/Sydney", "2004-07-15T00:00:00Z", "v", "Australia (Sydney)", "Australia/Sydney" },
        { "en", "Australia/Sydney", "2004-07-15T00:00:00Z", "vvvv", "Eastern Australia Time", "Australia/Sydney" },
        { "en", "Australia/Sydney", "2004-07-15T00:00:00Z", "VVVV", "Australia (Sydney)", "Australia/Sydney" },

        { "en", "Europe/London", "2004-01-15T00:00:00Z", "Z", "+0000", "+0:00" },
        { "en", "Europe/London", "2004-01-15T00:00:00Z", "ZZZZ", "GMT+00:00", "+0:00" },
        { "en", "Europe/London", "2004-01-15T00:00:00Z", "z", "GMT", "+0:00" },
        { "en", "Europe/London", "2004-01-15T00:00:00Z", "V", "GMT", "+0:00" },
        { "en", "Europe/London", "2004-01-15T00:00:00Z", "zzzz", "Greenwich Mean Time", "+0:00" },
        { "en", "Europe/London", "2004-07-15T00:00:00Z", "Z", "+0100", "+1:00" },
        { "en", "Europe/London", "2004-07-15T00:00:00Z", "ZZZZ", "GMT+01:00", "+1:00" },
        { "en", "Europe/London", "2004-07-15T00:00:00Z", "z", "GMT+01:00", "Europe/London" },
        { "en", "Europe/London", "2004-07-15T00:00:00Z", "V", "BST", "Europe/London" },
        { "en", "Europe/London", "2004-07-15T00:00:00Z", "zzzz", "British Summer Time", "Europe/London" },
    // icu en.txt has exemplar city for this time zone
        { "en", "Europe/London", "2004-07-15T00:00:00Z", "v", "United Kingdom Time", "Europe/London" },
        { "en", "Europe/London", "2004-07-15T00:00:00Z", "vvvv", "United Kingdom Time", "Europe/London" },
        { "en", "Europe/London", "2004-07-15T00:00:00Z", "VVVV", "United Kingdom Time", "Europe/London" },

        { "en", "Etc/GMT+3", "2004-01-15T00:00:00Z", "Z", "-0300", "-3:00" },
        { "en", "Etc/GMT+3", "2004-01-15T00:00:00Z", "ZZZZ", "GMT-03:00", "-3:00" },
        { "en", "Etc/GMT+3", "2004-01-15T00:00:00Z", "z", "GMT-03:00", "-3:00" },
        { "en", "Etc/GMT+3", "2004-01-15T00:00:00Z", "zzzz", "GMT-03:00", "-3:00" },
        { "en", "Etc/GMT+3", "2004-07-15T00:00:00Z", "Z", "-0300", "-3:00" },
        { "en", "Etc/GMT+3", "2004-07-15T00:00:00Z", "ZZZZ", "GMT-03:00", "-3:00" },
        { "en", "Etc/GMT+3", "2004-07-15T00:00:00Z", "z", "GMT-03:00", "-3:00" },
        { "en", "Etc/GMT+3", "2004-07-15T00:00:00Z", "zzzz", "GMT-03:00", "-3:00" },
        { "en", "Etc/GMT+3", "2004-07-15T00:00:00Z", "v", "GMT-03:00", "-3:00" },
        { "en", "Etc/GMT+3", "2004-07-15T00:00:00Z", "vvvv", "GMT-03:00", "-3:00" },

        // JB#5150
        { "en", "Asia/Calcutta", "2004-01-15T00:00:00Z", "Z", "+0530", "+5:30" },
        { "en", "Asia/Calcutta", "2004-01-15T00:00:00Z", "ZZZZ", "GMT+05:30", "+5:30" },
        { "en", "Asia/Calcutta", "2004-01-15T00:00:00Z", "z", "GMT+05:30", "+5:30" },
        { "en", "Asia/Calcutta", "2004-01-15T00:00:00Z", "V", "IST", "+5:30" },
        { "en", "Asia/Calcutta", "2004-01-15T00:00:00Z", "zzzz", "India Standard Time", "+5:30" },
        { "en", "Asia/Calcutta", "2004-07-15T00:00:00Z", "Z", "+0530", "+5:30" },
        { "en", "Asia/Calcutta", "2004-07-15T00:00:00Z", "ZZZZ", "GMT+05:30", "+5:30" },
        { "en", "Asia/Calcutta", "2004-07-15T00:00:00Z", "z", "GMT+05:30", "+05:30" },
        { "en", "Asia/Calcutta", "2004-07-15T00:00:00Z", "V", "IST", "+05:30" },
        { "en", "Asia/Calcutta", "2004-07-15T00:00:00Z", "zzzz", "India Standard Time", "+5:30" },
        { "en", "Asia/Calcutta", "2004-07-15T00:00:00Z", "v", "India Time", "Asia/Calcutta" },
        { "en", "Asia/Calcutta", "2004-07-15T00:00:00Z", "vvvv", "India Standard Time", "Asia/Calcutta" },

        // ==========

        { "de", "America/Los_Angeles", "2004-01-15T00:00:00Z", "Z", "-0800", "-8:00" },
        { "de", "America/Los_Angeles", "2004-01-15T00:00:00Z", "ZZZZ", "GMT-08:00", "-8:00" },
        { "de", "America/Los_Angeles", "2004-01-15T00:00:00Z", "z", "GMT-08:00", "-8:00" },
        { "de", "America/Los_Angeles", "2004-01-15T00:00:00Z", "zzzz", "GMT-08:00", "-8:00" },
        { "de", "America/Los_Angeles", "2004-07-15T00:00:00Z", "Z", "-0700", "-7:00" },
        { "de", "America/Los_Angeles", "2004-07-15T00:00:00Z", "ZZZZ", "GMT-07:00", "-7:00" },
        { "de", "America/Los_Angeles", "2004-07-15T00:00:00Z", "z", "GMT-07:00", "-7:00" },
        { "de", "America/Los_Angeles", "2004-07-15T00:00:00Z", "zzzz", "GMT-07:00", "-7:00" },
        { "de", "America/Los_Angeles", "2004-07-15T00:00:00Z", "v", "Vereinigte Staaten (Los Angeles)", "America/Los_Angeles" },
        { "de", "America/Los_Angeles", "2004-07-15T00:00:00Z", "vvvv", "Vereinigte Staaten (Los Angeles)", "America/Los_Angeles" },

        { "de", "America/Argentina/Buenos_Aires", "2004-01-15T00:00:00Z", "Z", "-0300", "-3:00" },
        { "de", "America/Argentina/Buenos_Aires", "2004-01-15T00:00:00Z", "ZZZZ", "GMT-03:00", "-3:00" },
        { "de", "America/Argentina/Buenos_Aires", "2004-01-15T00:00:00Z", "z", "GMT-03:00", "-3:00" },
        { "de", "America/Argentina/Buenos_Aires", "2004-01-15T00:00:00Z", "zzzz", "GMT-03:00", "-3:00" },
        { "de", "America/Argentina/Buenos_Aires", "2004-07-15T00:00:00Z", "Z", "-0300", "-3:00" },
        { "de", "America/Argentina/Buenos_Aires", "2004-07-15T00:00:00Z", "ZZZZ", "GMT-03:00", "-3:00" },
        { "de", "America/Argentina/Buenos_Aires", "2004-07-15T00:00:00Z", "z", "GMT-03:00", "-3:00" },
        { "de", "America/Argentina/Buenos_Aires", "2004-07-15T00:00:00Z", "zzzz", "GMT-03:00", "-3:00" },
        { "de", "America/Argentina/Buenos_Aires", "2004-07-15T00:00:00Z", "v", "Argentinien (Buenos Aires)", "America/Buenos_Aires" },
        { "de", "America/Argentina/Buenos_Aires", "2004-07-15T00:00:00Z", "vvvv", "Argentinien (Buenos Aires)", "America/Buenos_Aires" },

        { "de", "America/Buenos_Aires", "2004-01-15T00:00:00Z", "Z", "-0300", "-3:00" },
        { "de", "America/Buenos_Aires", "2004-01-15T00:00:00Z", "ZZZZ", "GMT-03:00", "-3:00" },
        { "de", "America/Buenos_Aires", "2004-01-15T00:00:00Z", "z", "GMT-03:00", "-3:00" },
        { "de", "America/Buenos_Aires", "2004-01-15T00:00:00Z", "zzzz", "GMT-03:00", "-3:00" },
        { "de", "America/Buenos_Aires", "2004-07-15T00:00:00Z", "Z", "-0300", "-3:00" },
        { "de", "America/Buenos_Aires", "2004-07-15T00:00:00Z", "ZZZZ", "GMT-03:00", "-3:00" },
        { "de", "America/Buenos_Aires", "2004-07-15T00:00:00Z", "z", "GMT-03:00", "-3:00" },
        { "de", "America/Buenos_Aires", "2004-07-15T00:00:00Z", "zzzz", "GMT-03:00", "-3:00" },
        { "de", "America/Buenos_Aires", "2004-07-15T00:00:00Z", "v", "Argentinien (Buenos Aires)", "America/Buenos_Aires" },
        { "de", "America/Buenos_Aires", "2004-07-15T00:00:00Z", "vvvv", "Argentinien (Buenos Aires)", "America/Buenos_Aires" },

        { "de", "America/Havana", "2004-01-15T00:00:00Z", "Z", "-0500", "-5:00" },
        { "de", "America/Havana", "2004-01-15T00:00:00Z", "ZZZZ", "GMT-05:00", "-5:00" },
        { "de", "America/Havana", "2004-01-15T00:00:00Z", "z", "GMT-05:00", "-5:00" },
        { "de", "America/Havana", "2004-01-15T00:00:00Z", "zzzz", "GMT-05:00", "-5:00" },
        { "de", "America/Havana", "2004-07-15T00:00:00Z", "Z", "-0400", "-4:00" },
        { "de", "America/Havana", "2004-07-15T00:00:00Z", "ZZZZ", "GMT-04:00", "-4:00" },
        { "de", "America/Havana", "2004-07-15T00:00:00Z", "z", "GMT-04:00", "-4:00" },
        { "de", "America/Havana", "2004-07-15T00:00:00Z", "zzzz", "GMT-04:00", "-4:00" },
        { "de", "America/Havana", "2004-07-15T00:00:00Z", "v", "(Kuba)", "America/Havana" },
        { "de", "America/Havana", "2004-07-15T00:00:00Z", "vvvv", "(Kuba)", "America/Havana" },
        // added to test proper fallback of country name
        { "de_CH", "America/Havana", "2004-07-15T00:00:00Z", "v", "(Kuba)", "America/Havana" },
        { "de_CH", "America/Havana", "2004-07-15T00:00:00Z", "vvvv", "(Kuba)", "America/Havana" },

        { "de", "Australia/ACT", "2004-01-15T00:00:00Z", "Z", "+1100", "+11:00" },
        { "de", "Australia/ACT", "2004-01-15T00:00:00Z", "ZZZZ", "GMT+11:00", "+11:00" },
        { "de", "Australia/ACT", "2004-01-15T00:00:00Z", "z", "GMT+11:00", "+11:00" },
        { "de", "Australia/ACT", "2004-01-15T00:00:00Z", "zzzz", "GMT+11:00", "+11:00" },
        { "de", "Australia/ACT", "2004-07-15T00:00:00Z", "Z", "+1000", "+10:00" },
        { "de", "Australia/ACT", "2004-07-15T00:00:00Z", "ZZZZ", "GMT+10:00", "+10:00" },
        { "de", "Australia/ACT", "2004-07-15T00:00:00Z", "z", "GMT+10:00", "+10:00" },
        { "de", "Australia/ACT", "2004-07-15T00:00:00Z", "zzzz", "GMT+10:00", "+10:00" },
        { "de", "Australia/ACT", "2004-07-15T00:00:00Z", "v", "Australien (Sydney)", "Australia/Sydney" },
        { "de", "Australia/ACT", "2004-07-15T00:00:00Z", "vvvv", "Australien (Sydney)", "Australia/Sydney" },

        { "de", "Australia/Sydney", "2004-01-15T00:00:00Z", "Z", "+1100", "+11:00" },
        { "de", "Australia/Sydney", "2004-01-15T00:00:00Z", "ZZZZ", "GMT+11:00", "+11:00" },
        { "de", "Australia/Sydney", "2004-01-15T00:00:00Z", "z", "GMT+11:00", "+11:00" },
        { "de", "Australia/Sydney", "2004-01-15T00:00:00Z", "zzzz", "GMT+11:00", "+11:00" },
        { "de", "Australia/Sydney", "2004-07-15T00:00:00Z", "Z", "+1000", "+10:00" },
        { "de", "Australia/Sydney", "2004-07-15T00:00:00Z", "ZZZZ", "GMT+10:00", "+10:00" },
        { "de", "Australia/Sydney", "2004-07-15T00:00:00Z", "z", "GMT+10:00", "+10:00" },
        { "de", "Australia/Sydney", "2004-07-15T00:00:00Z", "zzzz", "GMT+10:00", "+10:00" },
        { "de", "Australia/Sydney", "2004-07-15T00:00:00Z", "v", "Australien (Sydney)", "Australia/Sydney" },
        { "de", "Australia/Sydney", "2004-07-15T00:00:00Z", "vvvv", "Australien (Sydney)", "Australia/Sydney" },

        { "de", "Europe/London", "2004-01-15T00:00:00Z", "Z", "+0000", "+0:00" },
        { "de", "Europe/London", "2004-01-15T00:00:00Z", "ZZZZ", "GMT+00:00", "+0:00" },
        { "de", "Europe/London", "2004-01-15T00:00:00Z", "z", "GMT+00:00", "+0:00" },
        { "de", "Europe/London", "2004-01-15T00:00:00Z", "zzzz", "GMT+00:00", "+0:00" },
        { "de", "Europe/London", "2004-07-15T00:00:00Z", "Z", "+0100", "+1:00" },
        { "de", "Europe/London", "2004-07-15T00:00:00Z", "ZZZZ", "GMT+01:00", "+1:00" },
        { "de", "Europe/London", "2004-07-15T00:00:00Z", "z", "GMT+01:00", "+1:00" },
        { "de", "Europe/London", "2004-07-15T00:00:00Z", "zzzz", "GMT+01:00", "+1:00" },
        { "de", "Europe/London", "2004-07-15T00:00:00Z", "v", "(Vereinigtes K\\u00f6nigreich)", "Europe/London" },
        { "de", "Europe/London", "2004-07-15T00:00:00Z", "vvvv", "(Vereinigtes K\\u00f6nigreich)", "Europe/London" },

        { "de", "Etc/GMT+3", "2004-01-15T00:00:00Z", "Z", "-0300", "-3:00" },
        { "de", "Etc/GMT+3", "2004-01-15T00:00:00Z", "ZZZZ", "GMT-03:00", "-3:00" },
        { "de", "Etc/GMT+3", "2004-01-15T00:00:00Z", "z", "GMT-03:00", "-3:00" },
        { "de", "Etc/GMT+3", "2004-01-15T00:00:00Z", "zzzz", "GMT-03:00", "-3:00" },
        { "de", "Etc/GMT+3", "2004-07-15T00:00:00Z", "Z", "-0300", "-3:00" },
        { "de", "Etc/GMT+3", "2004-07-15T00:00:00Z", "ZZZZ", "GMT-03:00", "-3:00" },
        { "de", "Etc/GMT+3", "2004-07-15T00:00:00Z", "z", "GMT-03:00", "-3:00" },
        { "de", "Etc/GMT+3", "2004-07-15T00:00:00Z", "zzzz", "GMT-03:00", "-3:00" },
        { "de", "Etc/GMT+3", "2004-07-15T00:00:00Z", "v", "GMT-03:00", "-3:00" },
        { "de", "Etc/GMT+3", "2004-07-15T00:00:00Z", "vvvv", "GMT-03:00", "-3:00" },

        // JB#5150
        { "de", "Asia/Calcutta", "2004-01-15T00:00:00Z", "Z", "+0530", "+5:30" },
        { "de", "Asia/Calcutta", "2004-01-15T00:00:00Z", "ZZZZ", "GMT+05:30", "+5:30" },
        { "de", "Asia/Calcutta", "2004-01-15T00:00:00Z", "z", "GMT+05:30", "+5:30" },
        { "de", "Asia/Calcutta", "2004-01-15T00:00:00Z", "zzzz", "GMT+05:30", "+5:30" },
        { "de", "Asia/Calcutta", "2004-07-15T00:00:00Z", "Z", "+0530", "+5:30" },
        { "de", "Asia/Calcutta", "2004-07-15T00:00:00Z", "ZZZZ", "GMT+05:30", "+5:30" },
        { "de", "Asia/Calcutta", "2004-07-15T00:00:00Z", "z", "GMT+05:30", "+05:30" },
        { "de", "Asia/Calcutta", "2004-07-15T00:00:00Z", "zzzz", "GMT+05:30", "+5:30" },
        { "de", "Asia/Calcutta", "2004-07-15T00:00:00Z", "v", "(Indien)", "Asia/Calcutta" },
        { "de", "Asia/Calcutta", "2004-07-15T00:00:00Z", "vvvv", "(Indien)", "Asia/Calcutta" },

        // ==========

        { "zh", "America/Los_Angeles", "2004-01-15T00:00:00Z", "Z", "-0800", "-8:00" },
        { "zh", "America/Los_Angeles", "2004-01-15T00:00:00Z", "ZZZZ", "\\u683c\\u6797\\u5c3c\\u6cbb\\u6807\\u51c6\\u65f6\\u95f4-0800", "-8:00" },
        { "zh", "America/Los_Angeles", "2004-01-15T00:00:00Z", "z", "\\u683c\\u6797\\u5c3c\\u6cbb\\u6807\\u51c6\\u65f6\\u95f4-0800", "America/Los_Angeles" },
        { "zh", "America/Los_Angeles", "2004-01-15T00:00:00Z", "zzzz", "\\u592a\\u5e73\\u6d0b\\u6807\\u51c6\\u65f6\\u95f4", "America/Los_Angeles" },
        { "zh", "America/Los_Angeles", "2004-07-15T00:00:00Z", "Z", "-0700", "-7:00" },
        { "zh", "America/Los_Angeles", "2004-07-15T00:00:00Z", "ZZZZ", "\\u683c\\u6797\\u5c3c\\u6cbb\\u6807\\u51c6\\u65f6\\u95f4-0700", "-7:00" },
        { "zh", "America/Los_Angeles", "2004-07-15T00:00:00Z", "z", "\\u683c\\u6797\\u5c3c\\u6cbb\\u6807\\u51c6\\u65f6\\u95f4-0700", "America/Los_Angeles" },
        { "zh", "America/Los_Angeles", "2004-07-15T00:00:00Z", "zzzz", "\\u592a\\u5e73\\u6d0b\\u590f\\u4ee4\\u65f6\\u95f4", "America/Los_Angeles" },
    // icu zh.txt has exemplar city for this time zone
        { "zh", "America/Los_Angeles", "2004-07-15T00:00:00Z", "v", "\\u7f8e\\u56fd (\\u6d1b\\u6749\\u77f6)", "America/Los_Angeles" },
        { "zh", "America/Los_Angeles", "2004-07-15T00:00:00Z", "vvvv", "\\u7f8e\\u56fd\\u592a\\u5e73\\u6d0b\\u65f6\\u95f4", "America/Los_Angeles" },

        { "zh", "America/Argentina/Buenos_Aires", "2004-01-15T00:00:00Z", "Z", "-0300", "-3:00" },
        { "zh", "America/Argentina/Buenos_Aires", "2004-01-15T00:00:00Z", "ZZZZ", "\\u683c\\u6797\\u5c3c\\u6cbb\\u6807\\u51c6\\u65f6\\u95f4-0300", "-3:00" },
        { "zh", "America/Argentina/Buenos_Aires", "2004-01-15T00:00:00Z", "z", "\\u683c\\u6797\\u5c3c\\u6cbb\\u6807\\u51c6\\u65f6\\u95f4-0300", "-3:00" },
        { "zh", "America/Argentina/Buenos_Aires", "2004-01-15T00:00:00Z", "zzzz", "\\u963f\\u6839\\u5ef7\\u6807\\u51c6\\u65f6\\u95f4", "-3:00" },
        { "zh", "America/Argentina/Buenos_Aires", "2004-07-15T00:00:00Z", "Z", "-0300", "-3:00" },
        { "zh", "America/Argentina/Buenos_Aires", "2004-07-15T00:00:00Z", "ZZZZ", "\\u683c\\u6797\\u5c3c\\u6cbb\\u6807\\u51c6\\u65f6\\u95f4-0300", "-3:00" },
        { "zh", "America/Argentina/Buenos_Aires", "2004-07-15T00:00:00Z", "z", "\\u683c\\u6797\\u5c3c\\u6cbb\\u6807\\u51c6\\u65f6\\u95f4-0300", "-3:00" },
        { "zh", "America/Argentina/Buenos_Aires", "2004-07-15T00:00:00Z", "zzzz", "\\u963f\\u6839\\u5ef7\\u6807\\u51c6\\u65f6\\u95f4", "-3:00" },
        { "zh", "America/Argentina/Buenos_Aires", "2004-07-15T00:00:00Z", "v", "\\u963f\\u6839\\u5ef7 (\\u5e03\\u5b9c\\u8bfa\\u65af\\u827e\\u5229\\u65af)", "America/Buenos_Aires" },
        { "zh", "America/Argentina/Buenos_Aires", "2004-07-15T00:00:00Z", "vvvv", "\\u963f\\u6839\\u5ef7\\u6807\\u51c6\\u65f6\\u95f4", "America/Buenos_Aires" },

        { "zh", "America/Buenos_Aires", "2004-01-15T00:00:00Z", "Z", "-0300", "-3:00" },
        { "zh", "America/Buenos_Aires", "2004-01-15T00:00:00Z", "ZZZZ", "\\u683c\\u6797\\u5c3c\\u6cbb\\u6807\\u51c6\\u65f6\\u95f4-0300", "-3:00" },
        { "zh", "America/Buenos_Aires", "2004-01-15T00:00:00Z", "z", "\\u683c\\u6797\\u5c3c\\u6cbb\\u6807\\u51c6\\u65f6\\u95f4-0300", "-3:00" },
        { "zh", "America/Buenos_Aires", "2004-01-15T00:00:00Z", "zzzz", "\\u963f\\u6839\\u5ef7\\u6807\\u51c6\\u65f6\\u95f4", "-3:00" },
        { "zh", "America/Buenos_Aires", "2004-07-15T00:00:00Z", "Z", "-0300", "-3:00" },
        { "zh", "America/Buenos_Aires", "2004-07-15T00:00:00Z", "ZZZZ", "\\u683c\\u6797\\u5c3c\\u6cbb\\u6807\\u51c6\\u65f6\\u95f4-0300", "-3:00" },
        { "zh", "America/Buenos_Aires", "2004-07-15T00:00:00Z", "z", "\\u683c\\u6797\\u5c3c\\u6cbb\\u6807\\u51c6\\u65f6\\u95f4-0300", "-3:00" },
        { "zh", "America/Buenos_Aires", "2004-07-15T00:00:00Z", "zzzz", "\\u963f\\u6839\\u5ef7\\u6807\\u51c6\\u65f6\\u95f4", "-3:00" },
        { "zh", "America/Buenos_Aires", "2004-07-15T00:00:00Z", "v", "\\u963f\\u6839\\u5ef7 (\\u5e03\\u5b9c\\u8bfa\\u65af\\u827e\\u5229\\u65af)", "America/Buenos_Aires" },
        { "zh", "America/Buenos_Aires", "2004-07-15T00:00:00Z", "vvvv", "\\u963f\\u6839\\u5ef7\\u6807\\u51c6\\u65f6\\u95f4", "America/Buenos_Aires" },

        { "zh", "America/Havana", "2004-01-15T00:00:00Z", "Z", "-0500", "-5:00" },
        { "zh", "America/Havana", "2004-01-15T00:00:00Z", "ZZZZ", "\\u683c\\u6797\\u5c3c\\u6cbb\\u6807\\u51c6\\u65f6\\u95f4-0500", "-5:00" },
        { "zh", "America/Havana", "2004-01-15T00:00:00Z", "z", "\\u683c\\u6797\\u5c3c\\u6cbb\\u6807\\u51c6\\u65f6\\u95f4-0500", "-5:00" },
        { "zh", "America/Havana", "2004-01-15T00:00:00Z", "zzzz", "\\u53e4\\u5df4\\u6807\\u51c6\\u65f6\\u95f4", "-5:00" },
        { "zh", "America/Havana", "2004-07-15T00:00:00Z", "Z", "-0400", "-4:00" },
        { "zh", "America/Havana", "2004-07-15T00:00:00Z", "ZZZZ", "\\u683c\\u6797\\u5c3c\\u6cbb\\u6807\\u51c6\\u65f6\\u95f4-0400", "-4:00" },
        { "zh", "America/Havana", "2004-07-15T00:00:00Z", "z", "\\u683c\\u6797\\u5c3c\\u6cbb\\u6807\\u51c6\\u65f6\\u95f4-0400", "-4:00" },
        { "zh", "America/Havana", "2004-07-15T00:00:00Z", "zzzz", "\\u53e4\\u5df4\\u590f\\u4ee4\\u65f6\\u95f4", "-4:00" },
        { "zh", "America/Havana", "2004-07-15T00:00:00Z", "v", "\\u53e4\\u5df4\\u65f6\\u95f4", "America/Havana" },
        { "zh", "America/Havana", "2004-07-15T00:00:00Z", "vvvv", "\\u53e4\\u5df4\\u65f6\\u95f4", "America/Havana" },

        { "zh", "Australia/ACT", "2004-01-15T00:00:00Z", "Z", "+1100", "+11:00" },
        { "zh", "Australia/ACT", "2004-01-15T00:00:00Z", "ZZZZ", "\\u683c\\u6797\\u5c3c\\u6cbb\\u6807\\u51c6\\u65f6\\u95f4+1100", "+11:00" },
        { "zh", "Australia/ACT", "2004-01-15T00:00:00Z", "z", "\\u683c\\u6797\\u5c3c\\u6cbb\\u6807\\u51c6\\u65f6\\u95f4+1100", "+11:00" },
        { "zh", "Australia/ACT", "2004-01-15T00:00:00Z", "zzzz", "\\u6fb3\\u5927\\u5229\\u4e9a\\u4e1c\\u90e8\\u590f\\u4ee4\\u65f6\\u95f4", "+11:00" },
        { "zh", "Australia/ACT", "2004-07-15T00:00:00Z", "Z", "+1000", "+10:00" },
        { "zh", "Australia/ACT", "2004-07-15T00:00:00Z", "ZZZZ", "\\u683c\\u6797\\u5c3c\\u6cbb\\u6807\\u51c6\\u65f6\\u95f4+1000", "+10:00" },
        { "zh", "Australia/ACT", "2004-07-15T00:00:00Z", "z", "\\u683c\\u6797\\u5c3c\\u6cbb\\u6807\\u51c6\\u65f6\\u95f4+1000", "+10:00" },
        { "zh", "Australia/ACT", "2004-07-15T00:00:00Z", "zzzz", "\\u6fb3\\u5927\\u5229\\u4e9a\\u4e1c\\u90e8\\u6807\\u51c6\\u65f6\\u95f4", "+10:00" },
    // icu zh.txt does not have info for this time zone
        { "zh", "Australia/ACT", "2004-07-15T00:00:00Z", "v", "\\u6fb3\\u5927\\u5229\\u4e9a (\\u6089\\u5c3c)", "Australia/Sydney" },
        { "zh", "Australia/ACT", "2004-07-15T00:00:00Z", "vvvv", "\\u6fb3\\u5927\\u5229\\u4e9a\\u4e1c\\u90e8\\u65f6\\u95f4", "Australia/Sydney" },

        { "zh", "Australia/Sydney", "2004-01-15T00:00:00Z", "Z", "+1100", "+11:00" },
        { "zh", "Australia/Sydney", "2004-01-15T00:00:00Z", "ZZZZ", "\\u683c\\u6797\\u5c3c\\u6cbb\\u6807\\u51c6\\u65f6\\u95f4+1100", "+11:00" },
        { "zh", "Australia/Sydney", "2004-01-15T00:00:00Z", "z", "\\u683c\\u6797\\u5c3c\\u6cbb\\u6807\\u51c6\\u65f6\\u95f4+1100", "+11:00" },
        { "zh", "Australia/Sydney", "2004-01-15T00:00:00Z", "zzzz", "\\u6fb3\\u5927\\u5229\\u4e9a\\u4e1c\\u90e8\\u590f\\u4ee4\\u65f6\\u95f4", "+11:00" },
        { "zh", "Australia/Sydney", "2004-07-15T00:00:00Z", "Z", "+1000", "+10:00" },
        { "zh", "Australia/Sydney", "2004-07-15T00:00:00Z", "ZZZZ", "\\u683c\\u6797\\u5c3c\\u6cbb\\u6807\\u51c6\\u65f6\\u95f4+1000", "+10:00" },
        { "zh", "Australia/Sydney", "2004-07-15T00:00:00Z", "z", "\\u683c\\u6797\\u5c3c\\u6cbb\\u6807\\u51c6\\u65f6\\u95f4+1000", "+10:00" },
        { "zh", "Australia/Sydney", "2004-07-15T00:00:00Z", "zzzz", "\\u6fb3\\u5927\\u5229\\u4e9a\\u4e1c\\u90e8\\u6807\\u51c6\\u65f6\\u95f4", "+10:00" },
        { "zh", "Australia/Sydney", "2004-07-15T00:00:00Z", "v", "\\u6fb3\\u5927\\u5229\\u4e9a (\\u6089\\u5c3c)", "Australia/Sydney" },
        { "zh", "Australia/Sydney", "2004-07-15T00:00:00Z", "vvvv", "\\u6fb3\\u5927\\u5229\\u4e9a\\u4e1c\\u90e8\\u65f6\\u95f4", "Australia/Sydney" },

        { "zh", "Europe/London", "2004-01-15T00:00:00Z", "Z", "+0000", "+0:00" },
        { "zh", "Europe/London", "2004-01-15T00:00:00Z", "ZZZZ", "\\u683c\\u6797\\u5c3c\\u6cbb\\u6807\\u51c6\\u65f6\\u95f4+0000", "+0:00" },
        { "zh", "Europe/London", "2004-01-15T00:00:00Z", "z", "\\u683c\\u6797\\u5c3c\\u6cbb\\u6807\\u51c6\\u65f6\\u95f4+0000", "+0:00" },
        { "zh", "Europe/London", "2004-01-15T00:00:00Z", "V", "GMT", "+0:00" },
        { "zh", "Europe/London", "2004-01-15T00:00:00Z", "zzzz", "\\u683C\\u6797\\u5C3C\\u6CBB\\u6807\\u51C6\\u65F6\\u95F4", "+0:00" },
        { "zh", "Europe/London", "2004-07-15T00:00:00Z", "Z", "+0100", "+1:00" },
        { "zh", "Europe/London", "2004-07-15T00:00:00Z", "ZZZZ", "\\u683c\\u6797\\u5c3c\\u6cbb\\u6807\\u51c6\\u65f6\\u95f4+0100", "+1:00" },
        { "zh", "Europe/London", "2004-07-15T00:00:00Z", "z", "\\u683c\\u6797\\u5c3c\\u6cbb\\u6807\\u51c6\\u65f6\\u95f4+0100", "+1:00" },
        { "zh", "Europe/London", "2004-07-15T00:00:00Z", "V", "BST", "+1:00" },
        { "zh", "Europe/London", "2004-07-15T00:00:00Z", "zzzz", "\\u683c\\u6797\\u5c3c\\u6cbb\\u6807\\u51c6\\u65f6\\u95f4+0100", "+1:00" },
        { "zh", "Europe/London", "2004-07-15T00:00:00Z", "v", "\\u82f1\\u56fd\\u65f6\\u95f4", "Europe/London" },
        { "zh", "Europe/London", "2004-07-15T00:00:00Z", "vvvv", "\\u82f1\\u56fd\\u65f6\\u95f4", "Europe/London" },
        { "zh", "Europe/London", "2004-07-15T00:00:00Z", "VVVV", "\\u82f1\\u56fd\\u65f6\\u95f4", "Europe/London" },

        { "zh", "Etc/GMT+3", "2004-01-15T00:00:00Z", "Z", "-0300", "-3:00" },
        { "zh", "Etc/GMT+3", "2004-01-15T00:00:00Z", "ZZZZ", "\\u683c\\u6797\\u5c3c\\u6cbb\\u6807\\u51c6\\u65f6\\u95f4-0300", "-3:00" },
        { "zh", "Etc/GMT+3", "2004-01-15T00:00:00Z", "z", "\\u683c\\u6797\\u5c3c\\u6cbb\\u6807\\u51c6\\u65f6\\u95f4-0300", "-3:00" },
        { "zh", "Etc/GMT+3", "2004-01-15T00:00:00Z", "zzzz", "\\u683c\\u6797\\u5c3c\\u6cbb\\u6807\\u51c6\\u65f6\\u95f4-0300", "-3:00" },
        { "zh", "Etc/GMT+3", "2004-07-15T00:00:00Z", "Z", "-0300", "-3:00" },
        { "zh", "Etc/GMT+3", "2004-07-15T00:00:00Z", "ZZZZ", "\\u683c\\u6797\\u5c3c\\u6cbb\\u6807\\u51c6\\u65f6\\u95f4-0300", "-3:00" },
        { "zh", "Etc/GMT+3", "2004-07-15T00:00:00Z", "z", "\\u683c\\u6797\\u5c3c\\u6cbb\\u6807\\u51c6\\u65f6\\u95f4-0300", "-3:00" },
        { "zh", "Etc/GMT+3", "2004-07-15T00:00:00Z", "zzzz", "\\u683c\\u6797\\u5c3c\\u6cbb\\u6807\\u51c6\\u65f6\\u95f4-0300", "-3:00" },
        { "zh", "Etc/GMT+3", "2004-07-15T00:00:00Z", "v", "\\u683c\\u6797\\u5c3c\\u6cbb\\u6807\\u51c6\\u65f6\\u95f4-0300", "-3:00" },
        { "zh", "Etc/GMT+3", "2004-07-15T00:00:00Z", "vvvv", "\\u683c\\u6797\\u5c3c\\u6cbb\\u6807\\u51c6\\u65f6\\u95f4-0300", "-3:00" },

        // JB#5150
        { "zh", "Asia/Calcutta", "2004-01-15T00:00:00Z", "Z", "+0530", "+5:30" },
        { "zh", "Asia/Calcutta", "2004-01-15T00:00:00Z", "ZZZZ", "\\u683c\\u6797\\u5c3c\\u6cbb\\u6807\\u51c6\\u65f6\\u95f4+0530", "+5:30" },
        { "zh", "Asia/Calcutta", "2004-01-15T00:00:00Z", "z", "\\u683c\\u6797\\u5c3c\\u6cbb\\u6807\\u51c6\\u65f6\\u95f4+0530", "+5:30" },
        { "zh", "Asia/Calcutta", "2004-01-15T00:00:00Z", "zzzz", "\\u5370\\u5ea6\\u6807\\u51c6\\u65f6\\u95f4", "+5:30" },
        { "zh", "Asia/Calcutta", "2004-07-15T00:00:00Z", "Z", "+0530", "+5:30" },
        { "zh", "Asia/Calcutta", "2004-07-15T00:00:00Z", "ZZZZ", "\\u683c\\u6797\\u5c3c\\u6cbb\\u6807\\u51c6\\u65f6\\u95f4+0530", "+5:30" },
        { "zh", "Asia/Calcutta", "2004-07-15T00:00:00Z", "z", "\\u683c\\u6797\\u5c3c\\u6cbb\\u6807\\u51c6\\u65f6\\u95f4+0530", "+05:30" },
        { "zh", "Asia/Calcutta", "2004-07-15T00:00:00Z", "zzzz", "\\u5370\\u5ea6\\u6807\\u51c6\\u65f6\\u95f4", "+5:30" },
        { "zh", "Asia/Calcutta", "2004-07-15T00:00:00Z", "v", "\\u5370\\u5ea6\\u65f6\\u95f4", "Asia/Calcutta" },
        { "zh", "Asia/Calcutta", "2004-07-15T00:00:00Z", "vvvv", "\\u5370\\u5ea6\\u6807\\u51c6\\u65f6\\u95f4", "Asia/Calcutta" },

        // ==========

        { "hi", "America/Los_Angeles", "2004-01-15T00:00:00Z", "Z", "-0800", "-8:00" },
        { "hi", "America/Los_Angeles", "2004-01-15T00:00:00Z", "ZZZZ", "GMT-\\u0966\\u096e:\\u0966\\u0966", "-8:00" },
        { "hi", "America/Los_Angeles", "2004-01-15T00:00:00Z", "z", "GMT-\\u0966\\u096e:\\u0966\\u0966", "-8:00" },
        { "hi", "America/Los_Angeles", "2004-01-15T00:00:00Z", "zzzz", "GMT-\\u0966\\u096e:\\u0966\\u0966", "-8:00" },
        { "hi", "America/Los_Angeles", "2004-07-15T00:00:00Z", "Z", "-0700", "-7:00" },
        { "hi", "America/Los_Angeles", "2004-07-15T00:00:00Z", "ZZZZ", "GMT-\\u0966\\u096d:\\u0966\\u0966", "-7:00" },
        { "hi", "America/Los_Angeles", "2004-07-15T00:00:00Z", "z", "GMT-\\u0966\\u096d:\\u0966\\u0966", "-7:00" },
        { "hi", "America/Los_Angeles", "2004-07-15T00:00:00Z", "zzzz", "GMT-\\u0966\\u096d:\\u0966\\u0966", "-7:00" },
        { "hi", "America/Los_Angeles", "2004-07-15T00:00:00Z", "v", "\\u0938\\u0902\\u092f\\u0941\\u0915\\u094d\\u0924 \\u0930\\u093e\\u091c\\u094d\\u092f \\u0905\\u092e\\u0947\\u0930\\u093f\\u0915\\u093e (\\u0932\\u094b\\u0938 \\u090f\\u0902\\u091c\\u093f\\u0932\\u0947\\u0938)", "America/Los_Angeles" },
        { "hi", "America/Los_Angeles", "2004-07-15T00:00:00Z", "vvvv", "\\u0938\\u0902\\u092f\\u0941\\u0915\\u094d\\u0924 \\u0930\\u093e\\u091c\\u094d\\u092f \\u0905\\u092e\\u0947\\u0930\\u093f\\u0915\\u093e (\\u0932\\u094b\\u0938 \\u090f\\u0902\\u091c\\u093f\\u0932\\u0947\\u0938)", "America/Los_Angeles" },

        { "hi", "America/Argentina/Buenos_Aires", "2004-01-15T00:00:00Z", "Z", "-0300", "-3:00" },
        { "hi", "America/Argentina/Buenos_Aires", "2004-01-15T00:00:00Z", "ZZZZ", "GMT-\\u0966\\u0969:\\u0966\\u0966", "-3:00" },
        { "hi", "America/Argentina/Buenos_Aires", "2004-01-15T00:00:00Z", "z", "GMT-\\u0966\\u0969:\\u0966\\u0966", "-3:00" },
        { "hi", "America/Argentina/Buenos_Aires", "2004-01-15T00:00:00Z", "zzzz", "GMT-\\u0966\\u0969:\\u0966\\u0966", "-3:00" },
        { "hi", "America/Argentina/Buenos_Aires", "2004-07-15T00:00:00Z", "Z", "-0300", "-3:00" },
        { "hi", "America/Argentina/Buenos_Aires", "2004-07-15T00:00:00Z", "ZZZZ", "GMT-\\u0966\\u0969:\\u0966\\u0966", "-3:00" },
        { "hi", "America/Argentina/Buenos_Aires", "2004-07-15T00:00:00Z", "z", "GMT-\\u0966\\u0969:\\u0966\\u0966", "-3:00" },
        { "hi", "America/Argentina/Buenos_Aires", "2004-07-15T00:00:00Z", "zzzz", "GMT-\\u0966\\u0969:\\u0966\\u0966", "-3:00" },
        { "hi", "America/Argentina/Buenos_Aires", "2004-07-15T00:00:00Z", "v", "\\u0905\\u0930\\u094d\\u091c\\u0947\\u0928\\u094d\\u091f\\u0940\\u0928\\u093e (\\u092c\\u094d\\u092f\\u0942\\u0928\\u0938 \\u0906\\u092f\\u0930\\u0938)", "America/Buenos_Aires" },
        { "hi", "America/Argentina/Buenos_Aires", "2004-07-15T00:00:00Z", "vvvv", "\\u0905\\u0930\\u094d\\u091c\\u0947\\u0928\\u094d\\u091f\\u0940\\u0928\\u093e (\\u092c\\u094d\\u092f\\u0942\\u0928\\u0938 \\u0906\\u092f\\u0930\\u0938)", "America/Buenos_Aires" },

        { "hi", "America/Buenos_Aires", "2004-01-15T00:00:00Z", "Z", "-0300", "-3:00" },
        { "hi", "America/Buenos_Aires", "2004-01-15T00:00:00Z", "ZZZZ", "GMT-\\u0966\\u0969:\\u0966\\u0966", "-3:00" },
        { "hi", "America/Buenos_Aires", "2004-01-15T00:00:00Z", "z", "GMT-\\u0966\\u0969:\\u0966\\u0966", "-3:00" },
        { "hi", "America/Buenos_Aires", "2004-01-15T00:00:00Z", "zzzz", "GMT-\\u0966\\u0969:\\u0966\\u0966", "-3:00" },
        { "hi", "America/Buenos_Aires", "2004-07-15T00:00:00Z", "Z", "-0300", "-3:00" },
        { "hi", "America/Buenos_Aires", "2004-07-15T00:00:00Z", "ZZZZ", "GMT-\\u0966\\u0969:\\u0966\\u0966", "-3:00" },
        { "hi", "America/Buenos_Aires", "2004-07-15T00:00:00Z", "z", "GMT-\\u0966\\u0969:\\u0966\\u0966", "-3:00" },
        { "hi", "America/Buenos_Aires", "2004-07-15T00:00:00Z", "zzzz", "GMT-\\u0966\\u0969:\\u0966\\u0966", "-3:00" },
        { "hi", "America/Buenos_Aires", "2004-07-15T00:00:00Z", "v", "\\u0905\\u0930\\u094d\\u091c\\u0947\\u0928\\u094d\\u091f\\u0940\\u0928\\u093e (\\u092c\\u094d\\u092f\\u0942\\u0928\\u0938 \\u0906\\u092f\\u0930\\u0938)", "America/Buenos_Aires" },
        { "hi", "America/Buenos_Aires", "2004-07-15T00:00:00Z", "vvvv", "\\u0905\\u0930\\u094d\\u091c\\u0947\\u0928\\u094d\\u091f\\u0940\\u0928\\u093e (\\u092c\\u094d\\u092f\\u0942\\u0928\\u0938 \\u0906\\u092f\\u0930\\u0938)", "America/Buenos_Aires" },

        { "hi", "America/Havana", "2004-01-15T00:00:00Z", "Z", "-0500", "-5:00" },
        { "hi", "America/Havana", "2004-01-15T00:00:00Z", "ZZZZ", "GMT-\\u0966\\u096b:\\u0966\\u0966", "-5:00" },
        { "hi", "America/Havana", "2004-01-15T00:00:00Z", "z", "GMT-\\u0966\\u096b:\\u0966\\u0966", "-5:00" },
        { "hi", "America/Havana", "2004-01-15T00:00:00Z", "zzzz", "GMT-\\u0966\\u096b:\\u0966\\u0966", "-5:00" },
        { "hi", "America/Havana", "2004-07-15T00:00:00Z", "Z", "-0400", "-4:00" },
        { "hi", "America/Havana", "2004-07-15T00:00:00Z", "ZZZZ", "GMT-\\u0966\\u096a:\\u0966\\u0966", "-4:00" },
        { "hi", "America/Havana", "2004-07-15T00:00:00Z", "z", "GMT-\\u0966\\u096a:\\u0966\\u0966", "-4:00" },
        { "hi", "America/Havana", "2004-07-15T00:00:00Z", "zzzz", "GMT-\\u0966\\u096a:\\u0966\\u0966", "-4:00" },
        { "hi", "America/Havana", "2004-07-15T00:00:00Z", "v", "(\\u0915\\u094d\\u092f\\u0942\\u092c\\u093e)", "America/Havana" },
        { "hi", "America/Havana", "2004-07-15T00:00:00Z", "vvvv", "(\\u0915\\u094d\\u092f\\u0942\\u092c\\u093e)", "America/Havana" },

        { "hi", "Australia/ACT", "2004-01-15T00:00:00Z", "Z", "+1100", "+11:00" },
        { "hi", "Australia/ACT", "2004-01-15T00:00:00Z", "ZZZZ", "GMT+\\u0967\\u0967:\\u0966\\u0966", "+11:00" },
        { "hi", "Australia/ACT", "2004-01-15T00:00:00Z", "z", "GMT+\\u0967\\u0967:\\u0966\\u0966", "+11:00" },
        { "hi", "Australia/ACT", "2004-01-15T00:00:00Z", "zzzz", "GMT+\\u0967\\u0967:\\u0966\\u0966", "+11:00" },
        { "hi", "Australia/ACT", "2004-07-15T00:00:00Z", "Z", "+1000", "+10:00" },
        { "hi", "Australia/ACT", "2004-07-15T00:00:00Z", "ZZZZ", "GMT+\\u0967\\u0966:\\u0966\\u0966", "+10:00" },
        { "hi", "Australia/ACT", "2004-07-15T00:00:00Z", "z", "GMT+\\u0967\\u0966:\\u0966\\u0966", "+10:00" },
        { "hi", "Australia/ACT", "2004-07-15T00:00:00Z", "zzzz", "GMT+\\u0967\\u0966:\\u0966\\u0966", "+10:00" },
        { "hi", "Australia/ACT", "2004-07-15T00:00:00Z", "v", "\\u0911\\u0938\\u094d\\u091f\\u094d\\u0930\\u0947\\u0932\\u093f\\u092f\\u093e (\\u0938\\u093f\\u0921\\u0928\\u0940)", "Australia/Sydney" },
        { "hi", "Australia/ACT", "2004-07-15T00:00:00Z", "vvvv", "\\u0911\\u0938\\u094d\\u091f\\u094d\\u0930\\u0947\\u0932\\u093f\\u092f\\u093e (\\u0938\\u093f\\u0921\\u0928\\u0940)", "Australia/Sydney" },

        { "hi", "Australia/Sydney", "2004-01-15T00:00:00Z", "Z", "+1100", "+11:00" },
        { "hi", "Australia/Sydney", "2004-01-15T00:00:00Z", "ZZZZ", "GMT+\\u0967\\u0967:\\u0966\\u0966", "+11:00" },
        { "hi", "Australia/Sydney", "2004-01-15T00:00:00Z", "z", "GMT+\\u0967\\u0967:\\u0966\\u0966", "+11:00" },
        { "hi", "Australia/Sydney", "2004-01-15T00:00:00Z", "zzzz", "GMT+\\u0967\\u0967:\\u0966\\u0966", "+11:00" },
        { "hi", "Australia/Sydney", "2004-07-15T00:00:00Z", "Z", "+1000", "+10:00" },
        { "hi", "Australia/Sydney", "2004-07-15T00:00:00Z", "ZZZZ", "GMT+\\u0967\\u0966:\\u0966\\u0966", "+10:00" },
        { "hi", "Australia/Sydney", "2004-07-15T00:00:00Z", "z", "GMT+\\u0967\\u0966:\\u0966\\u0966", "+10:00" },
        { "hi", "Australia/Sydney", "2004-07-15T00:00:00Z", "zzzz", "GMT+\\u0967\\u0966:\\u0966\\u0966", "+10:00" },
        { "hi", "Australia/Sydney", "2004-07-15T00:00:00Z", "v", "\\u0911\\u0938\\u094d\\u091f\\u094d\\u0930\\u0947\\u0932\\u093f\\u092f\\u093e (\\u0938\\u093f\\u0921\\u0928\\u0940)", "Australia/Sydney" },
        { "hi", "Australia/Sydney", "2004-07-15T00:00:00Z", "vvvv", "\\u0911\\u0938\\u094d\\u091f\\u094d\\u0930\\u0947\\u0932\\u093f\\u092f\\u093e (\\u0938\\u093f\\u0921\\u0928\\u0940)", "Australia/Sydney" },

        { "hi", "Europe/London", "2004-01-15T00:00:00Z", "Z", "+0000", "+0:00" },
        { "hi", "Europe/London", "2004-01-15T00:00:00Z", "ZZZZ", "GMT+\\u0966\\u0966:\\u0966\\u0966", "+0:00" },
        { "hi", "Europe/London", "2004-01-15T00:00:00Z", "z", "GMT+\\u0966\\u0966:\\u0966\\u0966", "+0:00" },
        { "hi", "Europe/London", "2004-01-15T00:00:00Z", "zzzz", "GMT+\\u0966\\u0966:\\u0966\\u0966", "+0:00" },
        { "hi", "Europe/London", "2004-07-15T00:00:00Z", "Z", "+0100", "+1:00" },
        { "hi", "Europe/London", "2004-07-15T00:00:00Z", "ZZZZ", "GMT+\\u0966\\u0967:\\u0966\\u0966", "+1:00" },
        { "hi", "Europe/London", "2004-07-15T00:00:00Z", "z", "GMT+\\u0966\\u0967:\\u0966\\u0966", "+1:00" },
        { "hi", "Europe/London", "2004-07-15T00:00:00Z", "zzzz", "GMT+\\u0966\\u0967:\\u0966\\u0966", "+1:00" },
        { "hi", "Europe/London", "2004-07-15T00:00:00Z", "v", "(\\u092C\\u094D\\u0930\\u093F\\u0924\\u0928)", "Europe/London" },
        { "hi", "Europe/London", "2004-07-15T00:00:00Z", "vvvv", "(\\u092C\\u094D\\u0930\\u093F\\u0924\\u0928)", "Europe/London" },

        { "hi", "Etc/GMT+3", "2004-01-15T00:00:00Z", "Z", "-0300", "-3:00" },
        { "hi", "Etc/GMT+3", "2004-01-15T00:00:00Z", "ZZZZ", "GMT-\\u0966\\u0969:\\u0966\\u0966", "-3:00" },
        { "hi", "Etc/GMT+3", "2004-01-15T00:00:00Z", "z", "GMT-\\u0966\\u0969:\\u0966\\u0966", "-3:00" },
        { "hi", "Etc/GMT+3", "2004-01-15T00:00:00Z", "zzzz", "GMT-\\u0966\\u0969:\\u0966\\u0966", "-3:00" },
        { "hi", "Etc/GMT+3", "2004-07-15T00:00:00Z", "Z", "-0300", "-3:00" },
        { "hi", "Etc/GMT+3", "2004-07-15T00:00:00Z", "ZZZZ", "GMT-\\u0966\\u0969:\\u0966\\u0966", "-3:00" },
        { "hi", "Etc/GMT+3", "2004-07-15T00:00:00Z", "z", "GMT-\\u0966\\u0969:\\u0966\\u0966", "-3:00" },
        { "hi", "Etc/GMT+3", "2004-07-15T00:00:00Z", "zzzz", "GMT-\\u0966\\u0969:\\u0966\\u0966", "-3:00" },
        { "hi", "Etc/GMT+3", "2004-07-15T00:00:00Z", "v", "GMT-\\u0966\\u0969:\\u0966\\u0966", "-3:00" },
        { "hi", "Etc/GMT+3", "2004-07-15T00:00:00Z", "vvvv", "GMT-\\u0966\\u0969:\\u0966\\u0966", "-3:00" },

        { "hi", "Asia/Calcutta", "2004-01-15T00:00:00Z", "Z", "+0530", "+5:30" },
        { "hi", "Asia/Calcutta", "2004-01-15T00:00:00Z", "ZZZZ", "GMT+\\u0966\\u096B:\\u0969\\u0966", "+5:30" },
        { "hi", "Asia/Calcutta", "2004-01-15T00:00:00Z", "z", "IST", "+5:30" },
        { "hi", "Asia/Calcutta", "2004-01-15T00:00:00Z", "zzzz", "\\u092D\\u093E\\u0930\\u0924\\u0940\\u092F \\u0938\\u092E\\u092F", "+5:30" },
        { "hi", "Asia/Calcutta", "2004-07-15T00:00:00Z", "Z", "+0530", "+5:30" },
        { "hi", "Asia/Calcutta", "2004-07-15T00:00:00Z", "ZZZZ", "GMT+\\u0966\\u096B:\\u0969\\u0966", "+5:30" },
        { "hi", "Asia/Calcutta", "2004-07-15T00:00:00Z", "z", "IST", "+05:30" },
        { "hi", "Asia/Calcutta", "2004-07-15T00:00:00Z", "zzzz", "\\u092D\\u093E\\u0930\\u0924\\u0940\\u092F \\u0938\\u092E\\u092F", "+5:30" },
        { "hi", "Asia/Calcutta", "2004-07-15T00:00:00Z", "v", "IST", "Asia/Calcutta" },
        { "hi", "Asia/Calcutta", "2004-07-15T00:00:00Z", "vvvv", "\\u092D\\u093E\\u0930\\u0924\\u0940\\u092F \\u0938\\u092E\\u092F", "Asia/Calcutta" },

        // ==========

        { "bg", "America/Los_Angeles", "2004-01-15T00:00:00Z", "Z", "-0800", "-8:00" },
        { "bg", "America/Los_Angeles", "2004-01-15T00:00:00Z", "ZZZZ", "\\u0413\\u0440\\u0438\\u0438\\u043D\\u0443\\u0438\\u0447-0800", "-8:00" },
        { "bg", "America/Los_Angeles", "2004-01-15T00:00:00Z", "z", "\\u0413\\u0440\\u0438\\u0438\\u043D\\u0443\\u0438\\u0447-0800", "America/Los_Angeles" },
        { "bg", "America/Los_Angeles", "2004-01-15T00:00:00Z", "V", "PST", "America/Los_Angeles" },
        { "bg", "America/Los_Angeles", "2004-01-15T00:00:00Z", "zzzz", "\\u0422\\u0438\\u0445\\u043E\\u043E\\u043A\\u0435\\u0430\\u043D\\u0441\\u043A\\u0430 \\u0447\\u0430\\u0441\\u043E\\u0432\\u0430 \\u0437\\u043E\\u043D\\u0430", "America/Los_Angeles" },
        { "bg", "America/Los_Angeles", "2004-07-15T00:00:00Z", "Z", "-0700", "-7:00" },
        { "bg", "America/Los_Angeles", "2004-07-15T00:00:00Z", "ZZZZ", "\\u0413\\u0440\\u0438\\u0438\\u043D\\u0443\\u0438\\u0447-0700", "-7:00" },
        { "bg", "America/Los_Angeles", "2004-07-15T00:00:00Z", "z", "\\u0413\\u0440\\u0438\\u0438\\u043D\\u0443\\u0438\\u0447-0700", "America/Los_Angeles" },
        { "bg", "America/Los_Angeles", "2004-07-15T00:00:00Z", "V", "PDT", "America/Los_Angeles" },
        { "bg", "America/Los_Angeles", "2004-07-15T00:00:00Z", "zzzz", "\\u0422\\u0438\\u0445\\u043E\\u043E\\u043A\\u0435\\u0430\\u043D\\u0441\\u043A\\u0430 \\u043B\\u044F\\u0442\\u043D\\u0430 \\u0447\\u0430\\u0441\\u043E\\u0432\\u0430 \\u0437\\u043E\\u043D\\u0430", "America/Los_Angeles" },
    // icu bg.txt has exemplar city for this time zone
        { "bg", "America/Los_Angeles", "2004-07-15T00:00:00Z", "v", "\\u0421\\u0410\\u0429 (\\u041b\\u043e\\u0441 \\u0410\\u043d\\u0436\\u0435\\u043b\\u0438\\u0441)", "America/Los_Angeles" },
        { "bg", "America/Los_Angeles", "2004-07-15T00:00:00Z", "vvvv", "\\u0421\\u0410\\u0429 (\\u041b\\u043e\\u0441 \\u0410\\u043d\\u0436\\u0435\\u043b\\u0438\\u0441)", "America/Los_Angeles" },
        { "bg", "America/Los_Angeles", "2004-07-15T00:00:00Z", "VVVV", "\\u0421\\u0410\\u0429 (\\u041b\\u043e\\u0441 \\u0410\\u043d\\u0436\\u0435\\u043b\\u0438\\u0441)", "America/Los_Angeles" },

        { "bg", "America/Argentina/Buenos_Aires", "2004-01-15T00:00:00Z", "Z", "-0300", "-3:00" },
        { "bg", "America/Argentina/Buenos_Aires", "2004-01-15T00:00:00Z", "ZZZZ", "\\u0413\\u0440\\u0438\\u0438\\u043D\\u0443\\u0438\\u0447-0300", "-3:00" },
        { "bg", "America/Argentina/Buenos_Aires", "2004-01-15T00:00:00Z", "z", "\\u0413\\u0440\\u0438\\u0438\\u043D\\u0443\\u0438\\u0447-0300", "-3:00" },
        { "bg", "America/Argentina/Buenos_Aires", "2004-01-15T00:00:00Z", "zzzz", "\\u0413\\u0440\\u0438\\u0438\\u043D\\u0443\\u0438\\u0447-0300", "-3:00" },
        { "bg", "America/Argentina/Buenos_Aires", "2004-07-15T00:00:00Z", "Z", "-0300", "-3:00" },
        { "bg", "America/Argentina/Buenos_Aires", "2004-07-15T00:00:00Z", "ZZZZ", "\\u0413\\u0440\\u0438\\u0438\\u043D\\u0443\\u0438\\u0447-0300", "-3:00" },
        { "bg", "America/Argentina/Buenos_Aires", "2004-07-15T00:00:00Z", "z", "\\u0413\\u0440\\u0438\\u0438\\u043D\\u0443\\u0438\\u0447-0300", "-3:00" },
        { "bg", "America/Argentina/Buenos_Aires", "2004-07-15T00:00:00Z", "zzzz", "\\u0413\\u0440\\u0438\\u0438\\u043D\\u0443\\u0438\\u0447-0300", "-3:00" },
        { "bg", "America/Argentina/Buenos_Aires", "2004-07-15T00:00:00Z", "v", "\\u0410\\u0440\\u0436\\u0435\\u043d\\u0442\\u0438\\u043d\\u0430 (\\u0411\\u0443\\u0435\\u043D\\u043E\\u0441 \\u0410\\u0439\\u0440\\u0435\\u0441)", "America/Buenos_Aires" },
        { "bg", "America/Argentina/Buenos_Aires", "2004-07-15T00:00:00Z", "vvvv", "\\u0410\\u0440\\u0436\\u0435\\u043d\\u0442\\u0438\\u043d\\u0430 (\\u0411\\u0443\\u0435\\u043D\\u043E\\u0441 \\u0410\\u0439\\u0440\\u0435\\u0441)", "America/Buenos_Aires" },

        { "bg", "America/Buenos_Aires", "2004-01-15T00:00:00Z", "Z", "-0300", "-3:00" },
        { "bg", "America/Buenos_Aires", "2004-01-15T00:00:00Z", "ZZZZ", "\\u0413\\u0440\\u0438\\u0438\\u043D\\u0443\\u0438\\u0447-0300", "-3:00" },
        { "bg", "America/Buenos_Aires", "2004-01-15T00:00:00Z", "z", "\\u0413\\u0440\\u0438\\u0438\\u043D\\u0443\\u0438\\u0447-0300", "-3:00" },
        { "bg", "America/Buenos_Aires", "2004-01-15T00:00:00Z", "zzzz", "\\u0413\\u0440\\u0438\\u0438\\u043D\\u0443\\u0438\\u0447-0300", "-3:00" },
        { "bg", "America/Buenos_Aires", "2004-07-15T00:00:00Z", "Z", "-0300", "-3:00" },
        { "bg", "America/Buenos_Aires", "2004-07-15T00:00:00Z", "ZZZZ", "\\u0413\\u0440\\u0438\\u0438\\u043D\\u0443\\u0438\\u0447-0300", "-3:00" },
        { "bg", "America/Buenos_Aires", "2004-07-15T00:00:00Z", "z", "\\u0413\\u0440\\u0438\\u0438\\u043D\\u0443\\u0438\\u0447-0300", "-3:00" },
        { "bg", "America/Buenos_Aires", "2004-07-15T00:00:00Z", "zzzz", "\\u0413\\u0440\\u0438\\u0438\\u043D\\u0443\\u0438\\u0447-0300", "-3:00" },
    // icu bg.txt does not have info for this time zone
        { "bg", "America/Buenos_Aires", "2004-07-15T00:00:00Z", "v", "\\u0410\\u0440\\u0436\\u0435\\u043d\\u0442\\u0438\\u043d\\u0430 (\\u0411\\u0443\\u0435\\u043D\\u043E\\u0441 \\u0410\\u0439\\u0440\\u0435\\u0441)", "America/Buenos_Aires" },
        { "bg", "America/Buenos_Aires", "2004-07-15T00:00:00Z", "vvvv", "\\u0410\\u0440\\u0436\\u0435\\u043d\\u0442\\u0438\\u043d\\u0430 (\\u0411\\u0443\\u0435\\u043D\\u043E\\u0441 \\u0410\\u0439\\u0440\\u0435\\u0441)", "America/Buenos_Aires" },

        { "bg", "America/Havana", "2004-01-15T00:00:00Z", "Z", "-0500", "-5:00" },
        { "bg", "America/Havana", "2004-01-15T00:00:00Z", "ZZZZ", "\\u0413\\u0440\\u0438\\u0438\\u043D\\u0443\\u0438\\u0447-0500", "-5:00" },
        { "bg", "America/Havana", "2004-01-15T00:00:00Z", "z", "\\u0413\\u0440\\u0438\\u0438\\u043D\\u0443\\u0438\\u0447-0500", "-5:00" },
        { "bg", "America/Havana", "2004-01-15T00:00:00Z", "zzzz", "\\u0413\\u0440\\u0438\\u0438\\u043D\\u0443\\u0438\\u0447-0500", "-5:00" },
        { "bg", "America/Havana", "2004-07-15T00:00:00Z", "Z", "-0400", "-4:00" },
        { "bg", "America/Havana", "2004-07-15T00:00:00Z", "ZZZZ", "\\u0413\\u0440\\u0438\\u0438\\u043D\\u0443\\u0438\\u0447-0400", "-4:00" },
        { "bg", "America/Havana", "2004-07-15T00:00:00Z", "z", "\\u0413\\u0440\\u0438\\u0438\\u043D\\u0443\\u0438\\u0447-0400", "-4:00" },
        { "bg", "America/Havana", "2004-07-15T00:00:00Z", "zzzz", "\\u0413\\u0440\\u0438\\u0438\\u043D\\u0443\\u0438\\u0447-0400", "-4:00" },
        { "bg", "America/Havana", "2004-07-15T00:00:00Z", "v", "(\\u041a\\u0443\\u0431\\u0430)", "America/Havana" },
        { "bg", "America/Havana", "2004-07-15T00:00:00Z", "vvvv", "(\\u041a\\u0443\\u0431\\u0430)", "America/Havana" },

        { "bg", "Australia/ACT", "2004-01-15T00:00:00Z", "Z", "+1100", "+11:00" },
        { "bg", "Australia/ACT", "2004-01-15T00:00:00Z", "ZZZZ", "\\u0413\\u0440\\u0438\\u0438\\u043D\\u0443\\u0438\\u0447+1100", "+11:00" },
        { "bg", "Australia/ACT", "2004-01-15T00:00:00Z", "z", "\\u0413\\u0440\\u0438\\u0438\\u043D\\u0443\\u0438\\u0447+1100", "+11:00" },
        { "bg", "Australia/ACT", "2004-01-15T00:00:00Z", "zzzz", "\\u0413\\u0440\\u0438\\u0438\\u043D\\u0443\\u0438\\u0447+1100", "+11:00" },
        { "bg", "Australia/ACT", "2004-07-15T00:00:00Z", "Z", "+1000", "+10:00" },
        { "bg", "Australia/ACT", "2004-07-15T00:00:00Z", "ZZZZ", "\\u0413\\u0440\\u0438\\u0438\\u043D\\u0443\\u0438\\u0447+1000", "+10:00" },
        { "bg", "Australia/ACT", "2004-07-15T00:00:00Z", "z", "\\u0413\\u0440\\u0438\\u0438\\u043D\\u0443\\u0438\\u0447+1000", "+10:00" },
        { "bg", "Australia/ACT", "2004-07-15T00:00:00Z", "zzzz", "\\u0413\\u0440\\u0438\\u0438\\u043D\\u0443\\u0438\\u0447+1000", "+10:00" },
        { "bg", "Australia/ACT", "2004-07-15T00:00:00Z", "v", "\\u0410\\u0432\\u0441\\u0442\\u0440\\u0430\\u043b\\u0438\\u044f (\\u0421\\u0438\\u0434\\u043D\\u0438)", "Australia/Sydney" },
        { "bg", "Australia/ACT", "2004-07-15T00:00:00Z", "vvvv", "\\u0410\\u0432\\u0441\\u0442\\u0440\\u0430\\u043b\\u0438\\u044f (\\u0421\\u0438\\u0434\\u043D\\u0438)", "Australia/Sydney" },

        { "bg", "Australia/Sydney", "2004-01-15T00:00:00Z", "Z", "+1100", "+11:00" },
        { "bg", "Australia/Sydney", "2004-01-15T00:00:00Z", "ZZZZ", "\\u0413\\u0440\\u0438\\u0438\\u043D\\u0443\\u0438\\u0447+1100", "+11:00" },
        { "bg", "Australia/Sydney", "2004-01-15T00:00:00Z", "z", "\\u0413\\u0440\\u0438\\u0438\\u043D\\u0443\\u0438\\u0447+1100", "+11:00" },
        { "bg", "Australia/Sydney", "2004-01-15T00:00:00Z", "zzzz", "\\u0413\\u0440\\u0438\\u0438\\u043D\\u0443\\u0438\\u0447+1100", "+11:00" },
        { "bg", "Australia/Sydney", "2004-07-15T00:00:00Z", "Z", "+1000", "+10:00" },
        { "bg", "Australia/Sydney", "2004-07-15T00:00:00Z", "ZZZZ", "\\u0413\\u0440\\u0438\\u0438\\u043D\\u0443\\u0438\\u0447+1000", "+10:00" },
        { "bg", "Australia/Sydney", "2004-07-15T00:00:00Z", "z", "\\u0413\\u0440\\u0438\\u0438\\u043D\\u0443\\u0438\\u0447+1000", "+10:00" },
        { "bg", "Australia/Sydney", "2004-07-15T00:00:00Z", "zzzz", "\\u0413\\u0440\\u0438\\u0438\\u043D\\u0443\\u0438\\u0447+1000", "+10:00" },
        { "bg", "Australia/Sydney", "2004-07-15T00:00:00Z", "v", "\\u0410\\u0432\\u0441\\u0442\\u0440\\u0430\\u043b\\u0438\\u044f (\\u0421\\u0438\\u0434\\u043D\\u0438)", "Australia/Sydney" },
        { "bg", "Australia/Sydney", "2004-07-15T00:00:00Z", "vvvv", "\\u0410\\u0432\\u0441\\u0442\\u0440\\u0430\\u043b\\u0438\\u044f (\\u0421\\u0438\\u0434\\u043D\\u0438)", "Australia/Sydney" },

        { "bg", "Europe/London", "2004-01-15T00:00:00Z", "Z", "+0000", "+0:00" },
        { "bg", "Europe/London", "2004-01-15T00:00:00Z", "ZZZZ", "\\u0413\\u0440\\u0438\\u0438\\u043D\\u0443\\u0438\\u0447+0000", "+0:00" },
        { "bg", "Europe/London", "2004-01-15T00:00:00Z", "z", "\\u0413\\u0440\\u0438\\u0438\\u043D\\u0443\\u0438\\u0447+0000", "+0:00" },
        { "bg", "Europe/London", "2004-01-15T00:00:00Z", "zzzz", "\\u0427\\u0430\\u0441\\u043E\\u0432\\u0430 \\u0437\\u043E\\u043D\\u0430 \\u0413\\u0440\\u0438\\u043D\\u0443\\u0438\\u0447", "+0:00" },
        { "bg", "Europe/London", "2004-07-15T00:00:00Z", "Z", "+0100", "+1:00" },
        { "bg", "Europe/London", "2004-07-15T00:00:00Z", "ZZZZ", "\\u0413\\u0440\\u0438\\u0438\\u043D\\u0443\\u0438\\u0447+0100", "+1:00" },
        { "bg", "Europe/London", "2004-07-15T00:00:00Z", "z", "\\u0413\\u0440\\u0438\\u0438\\u043D\\u0443\\u0438\\u0447+0100", "+1:00" },
        { "bg", "Europe/London", "2004-07-15T00:00:00Z", "zzzz", "\\u0413\\u0440\\u0438\\u0438\\u043D\\u0443\\u0438\\u0447+0100", "+1:00" },
        { "bg", "Europe/London", "2004-07-15T00:00:00Z", "v", "(\\u041e\\u0431\\u0435\\u0434\\u0438\\u043d\\u0435\\u043d\\u043e \\u043a\\u0440\\u0430\\u043b\\u0441\\u0442\\u0432\\u043e)", "Europe/London" },
        { "bg", "Europe/London", "2004-07-15T00:00:00Z", "vvvv", "(\\u041e\\u0431\\u0435\\u0434\\u0438\\u043d\\u0435\\u043d\\u043e \\u043a\\u0440\\u0430\\u043b\\u0441\\u0442\\u0432\\u043e)", "Europe/London" },

        { "bg", "Etc/GMT+3", "2004-01-15T00:00:00Z", "Z", "-0300", "-3:00" },
        { "bg", "Etc/GMT+3", "2004-01-15T00:00:00Z", "ZZZZ", "\\u0413\\u0440\\u0438\\u0438\\u043D\\u0443\\u0438\\u0447-0300", "-3:00" },
        { "bg", "Etc/GMT+3", "2004-01-15T00:00:00Z", "z", "\\u0413\\u0440\\u0438\\u0438\\u043D\\u0443\\u0438\\u0447-0300", "-3:00" },
        { "bg", "Etc/GMT+3", "2004-01-15T00:00:00Z", "zzzz", "\\u0413\\u0440\\u0438\\u0438\\u043D\\u0443\\u0438\\u0447-0300", "-3:00" },
        { "bg", "Etc/GMT+3", "2004-07-15T00:00:00Z", "Z", "-0300", "-3:00" },
        { "bg", "Etc/GMT+3", "2004-07-15T00:00:00Z", "ZZZZ", "\\u0413\\u0440\\u0438\\u0438\\u043D\\u0443\\u0438\\u0447-0300", "-3:00" },
        { "bg", "Etc/GMT+3", "2004-07-15T00:00:00Z", "z", "\\u0413\\u0440\\u0438\\u0438\\u043D\\u0443\\u0438\\u0447-0300", "-3:00" },
        { "bg", "Etc/GMT+3", "2004-07-15T00:00:00Z", "zzzz", "\\u0413\\u0440\\u0438\\u0438\\u043D\\u0443\\u0438\\u0447-0300", "-3:00" },
        { "bg", "Etc/GMT+3", "2004-07-15T00:00:00Z", "v", "\\u0413\\u0440\\u0438\\u0438\\u043D\\u0443\\u0438\\u0447-0300", "-3:00" },
        { "bg", "Etc/GMT+3", "2004-07-15T00:00:00Z", "vvvv", "\\u0413\\u0440\\u0438\\u0438\\u043D\\u0443\\u0438\\u0447-0300", "-3:00" },

        // JB#5150
        { "bg", "Asia/Calcutta", "2004-01-15T00:00:00Z", "Z", "+0530", "+5:30" },
        { "bg", "Asia/Calcutta", "2004-01-15T00:00:00Z", "ZZZZ", "\\u0413\\u0440\\u0438\\u0438\\u043D\\u0443\\u0438\\u0447+0530", "+5:30" },
        { "bg", "Asia/Calcutta", "2004-01-15T00:00:00Z", "z", "\\u0413\\u0440\\u0438\\u0438\\u043D\\u0443\\u0438\\u0447+0530", "+5:30" },
        { "bg", "Asia/Calcutta", "2004-01-15T00:00:00Z", "zzzz", "\\u0413\\u0440\\u0438\\u0438\\u043D\\u0443\\u0438\\u0447+0530", "+5:30" },
        { "bg", "Asia/Calcutta", "2004-07-15T00:00:00Z", "Z", "+0530", "+5:30" },
        { "bg", "Asia/Calcutta", "2004-07-15T00:00:00Z", "ZZZZ", "\\u0413\\u0440\\u0438\\u0438\\u043D\\u0443\\u0438\\u0447+0530", "+5:30" },
        { "bg", "Asia/Calcutta", "2004-07-15T00:00:00Z", "z", "\\u0413\\u0440\\u0438\\u0438\\u043D\\u0443\\u0438\\u0447+0530", "+05:30" },
        { "bg", "Asia/Calcutta", "2004-07-15T00:00:00Z", "zzzz", "\\u0413\\u0440\\u0438\\u0438\\u043D\\u0443\\u0438\\u0447+0530", "+5:30" },
        { "bg", "Asia/Calcutta", "2004-07-15T00:00:00Z", "v", "(\\u0418\\u043D\\u0434\\u0438\\u044F)", "Asia/Calcutta" },
        { "bg", "Asia/Calcutta", "2004-07-15T00:00:00Z", "vvvv", "(\\u0418\\u043D\\u0434\\u0438\\u044F)", "Asia/Calcutta" },
    // ==========

        { "ja", "America/Los_Angeles", "2004-01-15T00:00:00Z", "Z", "-0800", "-8:00" },
        { "ja", "America/Los_Angeles", "2004-01-15T00:00:00Z", "ZZZZ", "GMT-08:00", "-8:00" },
        { "ja", "America/Los_Angeles", "2004-01-15T00:00:00Z", "z", "GMT-08:00", "America/Los_Angeles" },
        { "ja", "America/Los_Angeles", "2004-01-15T00:00:00Z", "V", "PST", "America/Los_Angeles" },
        { "ja", "America/Los_Angeles", "2004-01-15T00:00:00Z", "zzzz", "\\u30a2\\u30e1\\u30ea\\u30ab\\u592a\\u5e73\\u6d0b\\u6a19\\u6e96\\u6642", "America/Los_Angeles" },
        { "ja", "America/Los_Angeles", "2004-07-15T00:00:00Z", "Z", "-0700", "-700" },
        { "ja", "America/Los_Angeles", "2004-07-15T00:00:00Z", "ZZZZ", "GMT-07:00", "-7:00" },
        { "ja", "America/Los_Angeles", "2004-07-15T00:00:00Z", "z", "GMT-07:00", "America/Los_Angeles" },
        { "ja", "America/Los_Angeles", "2004-07-15T00:00:00Z", "V", "PDT", "America/Los_Angeles" },
        { "ja", "America/Los_Angeles", "2004-07-15T00:00:00Z", "zzzz", "\\u30a2\\u30e1\\u30ea\\u30ab\\u592a\\u5e73\\u6d0b\\u590f\\u6642\\u9593", "America/Los_Angeles" },
    // icu ja.txt has exemplar city for this time zone
        { "ja", "America/Los_Angeles", "2004-07-15T00:00:00Z", "v", "\\u30A2\\u30E1\\u30EA\\u30AB\\u5408\\u8846\\u56FD (\\u30ed\\u30b5\\u30f3\\u30bc\\u30eb\\u30b9)", "America/Los_Angeles" },
        { "ja", "America/Los_Angeles", "2004-07-15T00:00:00Z", "vvvv", "\\u30A2\\u30E1\\u30EA\\u30AB\\u592A\\u5e73\\u6D0B\\u6642\\u9593", "America/Los_Angeles" },
        { "ja", "America/Los_Angeles", "2004-07-15T00:00:00Z", "VVVV", "\\u30A2\\u30E1\\u30EA\\u30AB\\u5408\\u8846\\u56FD (\\u30ed\\u30b5\\u30f3\\u30bc\\u30eb\\u30b9)", "America/Los_Angeles" },

        { "ja", "America/Argentina/Buenos_Aires", "2004-01-15T00:00:00Z", "Z", "-0300", "-3:00" },
        { "ja", "America/Argentina/Buenos_Aires", "2004-01-15T00:00:00Z", "ZZZZ", "GMT-03:00", "-3:00" },
        { "ja", "America/Argentina/Buenos_Aires", "2004-01-15T00:00:00Z", "z", "GMT-03:00", "-3:00" },
        { "ja", "America/Argentina/Buenos_Aires", "2004-01-15T00:00:00Z", "zzzz", "GMT-03:00", "-3:00" },
        { "ja", "America/Argentina/Buenos_Aires", "2004-07-15T00:00:00Z", "Z", "-0300", "-3:00" },
        { "ja", "America/Argentina/Buenos_Aires", "2004-07-15T00:00:00Z", "ZZZZ", "GMT-03:00", "-3:00" },
        { "ja", "America/Argentina/Buenos_Aires", "2004-07-15T00:00:00Z", "z", "GMT-03:00", "-3:00" },
        { "ja", "America/Argentina/Buenos_Aires", "2004-07-15T00:00:00Z", "zzzz", "GMT-03:00", "-3:00" },
    // icu ja.txt does not have info for this time zone
        { "ja", "America/Argentina/Buenos_Aires", "2004-07-15T00:00:00Z", "v", "\\u30a2\\u30eb\\u30bc\\u30f3\\u30c1\\u30f3 (\\u30D6\\u30A8\\u30CE\\u30B9\\u30A2\\u30A4\\u30EC\\u30B9)", "America/Buenos_Aires" },
        { "ja", "America/Argentina/Buenos_Aires", "2004-07-15T00:00:00Z", "vvvv", "\\u30a2\\u30eb\\u30bc\\u30f3\\u30c1\\u30f3\\u6642\\u9593", "America/Buenos_Aires" },

        { "ja", "America/Buenos_Aires", "2004-01-15T00:00:00Z", "Z", "-0300", "-3:00" },
        { "ja", "America/Buenos_Aires", "2004-01-15T00:00:00Z", "ZZZZ", "GMT-03:00", "-3:00" },
        { "ja", "America/Buenos_Aires", "2004-01-15T00:00:00Z", "z", "GMT-03:00", "-3:00" },
        { "ja", "America/Buenos_Aires", "2004-01-15T00:00:00Z", "zzzz", "GMT-03:00", "-3:00" },
        { "ja", "America/Buenos_Aires", "2004-07-15T00:00:00Z", "Z", "-0300", "-3:00" },
        { "ja", "America/Buenos_Aires", "2004-07-15T00:00:00Z", "ZZZZ", "GMT-03:00", "-3:00" },
        { "ja", "America/Buenos_Aires", "2004-07-15T00:00:00Z", "z", "GMT-03:00", "-3:00" },
        { "ja", "America/Buenos_Aires", "2004-07-15T00:00:00Z", "zzzz", "GMT-03:00", "-3:00" },
        { "ja", "America/Buenos_Aires", "2004-07-15T00:00:00Z", "v", "\\u30a2\\u30eb\\u30bc\\u30f3\\u30c1\\u30f3 (\\u30D6\\u30A8\\u30CE\\u30B9\\u30A2\\u30A4\\u30EC\\u30B9)", "America/Buenos_Aires" },
        { "ja", "America/Buenos_Aires", "2004-07-15T00:00:00Z", "vvvv", "\\u30a2\\u30eb\\u30bc\\u30f3\\u30c1\\u30f3\\u6642\\u9593", "America/Buenos_Aires" },

        { "ja", "America/Havana", "2004-01-15T00:00:00Z", "Z", "-0500", "-5:00" },
        { "ja", "America/Havana", "2004-01-15T00:00:00Z", "ZZZZ", "GMT-05:00", "-5:00" },
        { "ja", "America/Havana", "2004-01-15T00:00:00Z", "z", "GMT-05:00", "-5:00" },
        { "ja", "America/Havana", "2004-01-15T00:00:00Z", "zzzz", "GMT-05:00", "-5:00" },
        { "ja", "America/Havana", "2004-07-15T00:00:00Z", "Z", "-0400", "-4:00" },
        { "ja", "America/Havana", "2004-07-15T00:00:00Z", "ZZZZ", "GMT-04:00", "-4:00" },
        { "ja", "America/Havana", "2004-07-15T00:00:00Z", "z", "GMT-04:00", "-4:00" },
        { "ja", "America/Havana", "2004-07-15T00:00:00Z", "zzzz", "GMT-04:00", "-4:00" },
        { "ja", "America/Havana", "2004-07-15T00:00:00Z", "v", "\\u30ad\\u30e5\\u30fc\\u30d0\\u6642\\u9593", "America/Havana" },
        { "ja", "America/Havana", "2004-07-15T00:00:00Z", "vvvv", "\\u30ad\\u30e5\\u30fc\\u30d0\\u6642\\u9593", "America/Havana" },

        { "ja", "Australia/ACT", "2004-01-15T00:00:00Z", "Z", "+1100", "+11:00" },
        { "ja", "Australia/ACT", "2004-01-15T00:00:00Z", "ZZZZ", "GMT+11:00", "+11:00" },
        { "ja", "Australia/ACT", "2004-01-15T00:00:00Z", "z", "GMT+11:00", "+11:00" },
        { "ja", "Australia/ACT", "2004-01-15T00:00:00Z", "zzzz", "GMT+11:00", "+11:00" },
        { "ja", "Australia/ACT", "2004-07-15T00:00:00Z", "Z", "+1000", "+10:00" },
        { "ja", "Australia/ACT", "2004-07-15T00:00:00Z", "ZZZZ", "GMT+10:00", "+10:00" },
        { "ja", "Australia/ACT", "2004-07-15T00:00:00Z", "z", "GMT+10:00", "+10:00" },
        { "ja", "Australia/ACT", "2004-07-15T00:00:00Z", "zzzz", "GMT+10:00", "+10:00" },
    // icu ja.txt does not have info for this time zone
        { "ja", "Australia/ACT", "2004-07-15T00:00:00Z", "v", "\\u30aa\\u30fc\\u30b9\\u30c8\\u30e9\\u30ea\\u30a2 (\\u30b7\\u30c9\\u30cb\\u30fc)", "Australia/Sydney" },
        { "ja", "Australia/ACT", "2004-07-15T00:00:00Z", "vvvv", "\\u30aa\\u30fc\\u30b9\\u30c8\\u30e9\\u30ea\\u30a2 (\\u30b7\\u30c9\\u30cb\\u30fc)", "Australia/Sydney" },

        { "ja", "Australia/Sydney", "2004-01-15T00:00:00Z", "Z", "+1100", "+11:00" },
        { "ja", "Australia/Sydney", "2004-01-15T00:00:00Z", "ZZZZ", "GMT+11:00", "+11:00" },
        { "ja", "Australia/Sydney", "2004-01-15T00:00:00Z", "z", "GMT+11:00", "+11:00" },
        { "ja", "Australia/Sydney", "2004-01-15T00:00:00Z", "zzzz", "GMT+11:00", "+11:00" },
        { "ja", "Australia/Sydney", "2004-07-15T00:00:00Z", "Z", "+1000", "+10:00" },
        { "ja", "Australia/Sydney", "2004-07-15T00:00:00Z", "ZZZZ", "GMT+10:00", "+10:00" },
        { "ja", "Australia/Sydney", "2004-07-15T00:00:00Z", "z", "GMT+10:00", "+10:00" },
        { "ja", "Australia/Sydney", "2004-07-15T00:00:00Z", "zzzz", "GMT+10:00", "+10:00" },
        { "ja", "Australia/Sydney", "2004-07-15T00:00:00Z", "v", "\\u30aa\\u30fc\\u30b9\\u30c8\\u30e9\\u30ea\\u30a2 (\\u30b7\\u30c9\\u30cb\\u30fc)", "Australia/Sydney" },
        { "ja", "Australia/Sydney", "2004-07-15T00:00:00Z", "vvvv", "\\u30aa\\u30fc\\u30b9\\u30c8\\u30e9\\u30ea\\u30a2 (\\u30b7\\u30c9\\u30cb\\u30fc)", "Australia/Sydney" },

        { "ja", "Europe/London", "2004-01-15T00:00:00Z", "Z", "+0000", "+0:00" },
        { "ja", "Europe/London", "2004-01-15T00:00:00Z", "ZZZZ", "GMT+00:00", "+0:00" },
        { "ja", "Europe/London", "2004-01-15T00:00:00Z", "z", "GMT+00:00", "+0:00" },
        { "ja", "Europe/London", "2004-01-15T00:00:00Z", "V", "GMT", "+0:00" },
        { "ja", "Europe/London", "2004-01-15T00:00:00Z", "zzzz", "\\u30B0\\u30EA\\u30CB\\u30C3\\u30B8\\u6A19\\u6E96\\u6642", "+0:00" },
        { "ja", "Europe/London", "2004-07-15T00:00:00Z", "Z", "+0100", "+1:00" },
        { "ja", "Europe/London", "2004-07-15T00:00:00Z", "ZZZZ", "GMT+01:00", "+1:00" },
        { "ja", "Europe/London", "2004-07-15T00:00:00Z", "z", "GMT+01:00", "+1:00" },
        { "ja", "Europe/London", "2004-07-15T00:00:00Z", "V", "GMT+01:00", "+1:00" },
        { "ja", "Europe/London", "2004-07-15T00:00:00Z", "zzzz", "GMT+01:00", "+1:00" },
        { "ja", "Europe/London", "2004-07-15T00:00:00Z", "v", "\\u30a4\\u30ae\\u30ea\\u30b9\\u6642\\u9593", "Europe/London" },
        { "ja", "Europe/London", "2004-07-15T00:00:00Z", "vvvv", "\\u30a4\\u30ae\\u30ea\\u30b9\\u6642\\u9593", "Europe/London" },
        { "ja", "Europe/London", "2004-07-15T00:00:00Z", "VVVV", "\\u30a4\\u30ae\\u30ea\\u30b9\\u6642\\u9593", "Europe/London" },

        { "ja", "Etc/GMT+3", "2004-01-15T00:00:00Z", "Z", "-0300", "-3:00" },
        { "ja", "Etc/GMT+3", "2004-01-15T00:00:00Z", "ZZZZ", "GMT-03:00", "-3:00" },
        { "ja", "Etc/GMT+3", "2004-01-15T00:00:00Z", "z", "GMT-03:00", "-3:00" },
        { "ja", "Etc/GMT+3", "2004-01-15T00:00:00Z", "zzzz", "GMT-03:00", "-3:00" },
        { "ja", "Etc/GMT+3", "2004-07-15T00:00:00Z", "Z", "-0300", "-3:00" },
        { "ja", "Etc/GMT+3", "2004-07-15T00:00:00Z", "ZZZZ", "GMT-03:00", "-3:00" },
        { "ja", "Etc/GMT+3", "2004-07-15T00:00:00Z", "z", "GMT-03:00", "-3:00" },
        { "ja", "Etc/GMT+3", "2004-07-15T00:00:00Z", "zzzz", "GMT-03:00", "-3:00" },
        { "ja", "Etc/GMT+3", "2004-07-15T00:00:00Z", "v", "GMT-03:00", "-3:00" },
        { "ja", "Etc/GMT+3", "2004-07-15T00:00:00Z", "vvvv", "GMT-03:00", "-3:00" },

        // JB#5150
        { "ja", "Asia/Calcutta", "2004-01-15T00:00:00Z", "Z", "+0530", "+5:30" },
        { "ja", "Asia/Calcutta", "2004-01-15T00:00:00Z", "ZZZZ", "GMT+05:30", "+5:30" },
        { "ja", "Asia/Calcutta", "2004-01-15T00:00:00Z", "z", "GMT+05:30", "+5:30" },
        { "ja", "Asia/Calcutta", "2004-01-15T00:00:00Z", "zzzz", "GMT+05:30", "+5:30" },
        { "ja", "Asia/Calcutta", "2004-07-15T00:00:00Z", "Z", "+0530", "+5:30" },
        { "ja", "Asia/Calcutta", "2004-07-15T00:00:00Z", "ZZZZ", "GMT+05:30", "+5:30" },
        { "ja", "Asia/Calcutta", "2004-07-15T00:00:00Z", "z", "GMT+05:30", "+05:30" },
        { "ja", "Asia/Calcutta", "2004-07-15T00:00:00Z", "zzzz", "GMT+05:30", "+5:30" },
        { "ja", "Asia/Calcutta", "2004-07-15T00:00:00Z", "v", "\\u30A4\\u30F3\\u30C9\\u6642\\u9593", "Asia/Calcutta" },
        { "ja", "Asia/Calcutta", "2004-07-15T00:00:00Z", "vvvv", "\\u30A4\\u30F3\\u30C9\\u6642\\u9593", "Asia/Calcutta" },

    // ==========

        { "si", "America/Los_Angeles", "2004-01-15T00:00:00Z", "Z", "-0800", "-8:00" },
        { "si", "America/Los_Angeles", "2004-01-15T00:00:00Z", "ZZZZ", "GMT-08:00", "-8:00" },
        { "si", "America/Los_Angeles", "2004-01-15T00:00:00Z", "z", "GMT-08:00", "-8:00" },
        { "si", "America/Los_Angeles", "2004-01-15T00:00:00Z", "zzzz", "GMT-08:00", "-8:00" },
        { "si", "America/Los_Angeles", "2004-07-15T00:00:00Z", "Z", "-0700", "-7:00" },
        { "si", "America/Los_Angeles", "2004-07-15T00:00:00Z", "ZZZZ", "GMT-07:00", "-7:00" },
        { "si", "America/Los_Angeles", "2004-07-15T00:00:00Z", "z", "GMT-07:00", "-7:00" },
        { "si", "America/Los_Angeles", "2004-07-15T00:00:00Z", "zzzz", "GMT-07:00", "-7:00" },
        { "si", "America/Los_Angeles", "2004-07-15T00:00:00Z", "v", "US (Los Angeles)", "America/Los_Angeles" },
        { "si", "America/Los_Angeles", "2004-07-15T00:00:00Z", "vvvv", "US (Los Angeles)", "America/Los_Angeles" },

        { "si", "America/Argentina/Buenos_Aires", "2004-01-15T00:00:00Z", "Z", "-0300", "-3:00" },
        { "si", "America/Argentina/Buenos_Aires", "2004-01-15T00:00:00Z", "ZZZZ", "GMT-03:00", "-3:00" },
        { "si", "America/Argentina/Buenos_Aires", "2004-01-15T00:00:00Z", "z", "GMT-03:00", "-3:00" },
        { "si", "America/Argentina/Buenos_Aires", "2004-01-15T00:00:00Z", "zzzz", "GMT-03:00", "-3:00" },
        { "si", "America/Argentina/Buenos_Aires", "2004-07-15T00:00:00Z", "Z", "-0300", "-3:00" },
        { "si", "America/Argentina/Buenos_Aires", "2004-07-15T00:00:00Z", "ZZZZ", "GMT-03:00", "-3:00" },
        { "si", "America/Argentina/Buenos_Aires", "2004-07-15T00:00:00Z", "z", "GMT-03:00", "-3:00" },
        { "si", "America/Argentina/Buenos_Aires", "2004-07-15T00:00:00Z", "zzzz", "GMT-03:00", "-3:00" },
        { "si", "America/Argentina/Buenos_Aires", "2004-07-15T00:00:00Z", "v", "AR (Buenos Aires)", "America/Buenos_Aires" },
        { "si", "America/Argentina/Buenos_Aires", "2004-07-15T00:00:00Z", "vvvv", "AR (Buenos Aires)", "America/Buenos_Aires" },

        { "si", "America/Buenos_Aires", "2004-01-15T00:00:00Z", "Z", "-0300", "-3:00" },
        { "si", "America/Buenos_Aires", "2004-01-15T00:00:00Z", "ZZZZ", "GMT-03:00", "-3:00" },
        { "si", "America/Buenos_Aires", "2004-01-15T00:00:00Z", "z", "GMT-03:00", "-3:00" },
        { "si", "America/Buenos_Aires", "2004-01-15T00:00:00Z", "zzzz", "GMT-03:00", "-3:00" },
        { "si", "America/Buenos_Aires", "2004-07-15T00:00:00Z", "Z", "-0300", "-3:00" },
        { "si", "America/Buenos_Aires", "2004-07-15T00:00:00Z", "ZZZZ", "GMT-03:00", "-3:00" },
        { "si", "America/Buenos_Aires", "2004-07-15T00:00:00Z", "z", "GMT-03:00", "-3:00" },
        { "si", "America/Buenos_Aires", "2004-07-15T00:00:00Z", "zzzz", "GMT-03:00", "-3:00" },
        { "si", "America/Buenos_Aires", "2004-07-15T00:00:00Z", "v", "AR (Buenos Aires)", "America/Buenos_Aires" },
        { "si", "America/Buenos_Aires", "2004-07-15T00:00:00Z", "vvvv", "AR (Buenos Aires)", "America/Buenos_Aires" },

        { "si", "America/Havana", "2004-01-15T00:00:00Z", "Z", "-0500", "-5:00" },
        { "si", "America/Havana", "2004-01-15T00:00:00Z", "ZZZZ", "GMT-05:00", "-5:00" },
        { "si", "America/Havana", "2004-01-15T00:00:00Z", "z", "GMT-05:00", "-5:00" },
        { "si", "America/Havana", "2004-01-15T00:00:00Z", "zzzz", "GMT-05:00", "-5:00" },
        { "si", "America/Havana", "2004-07-15T00:00:00Z", "Z", "-0400", "-4:00" },
        { "si", "America/Havana", "2004-07-15T00:00:00Z", "ZZZZ", "GMT-04:00", "-4:00" },
        { "si", "America/Havana", "2004-07-15T00:00:00Z", "z", "GMT-04:00", "-4:00" },
        { "si", "America/Havana", "2004-07-15T00:00:00Z", "zzzz", "GMT-04:00", "-4:00" },
        { "si", "America/Havana", "2004-07-15T00:00:00Z", "v", "(CU)", "America/Havana" },
        { "si", "America/Havana", "2004-07-15T00:00:00Z", "vvvv", "(CU)", "America/Havana" },

        { "si", "Australia/ACT", "2004-01-15T00:00:00Z", "Z", "+1100", "+11:00" },
        { "si", "Australia/ACT", "2004-01-15T00:00:00Z", "ZZZZ", "GMT+11:00", "+11:00" },
        { "si", "Australia/ACT", "2004-01-15T00:00:00Z", "z", "GMT+11:00", "+11:00" },
        { "si", "Australia/ACT", "2004-01-15T00:00:00Z", "zzzz", "GMT+11:00", "+11:00" },
        { "si", "Australia/ACT", "2004-07-15T00:00:00Z", "Z", "+1000", "+10:00" },
        { "si", "Australia/ACT", "2004-07-15T00:00:00Z", "ZZZZ", "GMT+10:00", "+10:00" },
        { "si", "Australia/ACT", "2004-07-15T00:00:00Z", "z", "GMT+10:00", "+10:00" },
        { "si", "Australia/ACT", "2004-07-15T00:00:00Z", "zzzz", "GMT+10:00", "+10:00" },
        { "si", "Australia/ACT", "2004-07-15T00:00:00Z", "v", "AU (Sydney)", "Australia/Sydney" },
        { "si", "Australia/ACT", "2004-07-15T00:00:00Z", "vvvv", "AU (Sydney)", "Australia/Sydney" },

        { "si", "Australia/Sydney", "2004-01-15T00:00:00Z", "Z", "+1100", "+11:00" },
        { "si", "Australia/Sydney", "2004-01-15T00:00:00Z", "ZZZZ", "GMT+11:00", "+11:00" },
        { "si", "Australia/Sydney", "2004-01-15T00:00:00Z", "z", "GMT+11:00", "+11:00" },
        { "si", "Australia/Sydney", "2004-01-15T00:00:00Z", "zzzz", "GMT+11:00", "+11:00" },
        { "si", "Australia/Sydney", "2004-07-15T00:00:00Z", "Z", "+1000", "+10:00" },
        { "si", "Australia/Sydney", "2004-07-15T00:00:00Z", "ZZZZ", "GMT+10:00", "+10:00" },
        { "si", "Australia/Sydney", "2004-07-15T00:00:00Z", "z", "GMT+10:00", "+10:00" },
        { "si", "Australia/Sydney", "2004-07-15T00:00:00Z", "zzzz", "GMT+10:00", "+10:00" },
        { "si", "Australia/Sydney", "2004-07-15T00:00:00Z", "v", "AU (Sydney)", "Australia/Sydney" },
        { "si", "Australia/Sydney", "2004-07-15T00:00:00Z", "vvvv", "AU (Sydney)", "Australia/Sydney" },

        { "si", "Europe/London", "2004-01-15T00:00:00Z", "Z", "+0000", "+0:00" },
        { "si", "Europe/London", "2004-01-15T00:00:00Z", "ZZZZ", "GMT+00:00", "+0:00" },
        { "si", "Europe/London", "2004-01-15T00:00:00Z", "z", "GMT+00:00", "+0:00" },
        { "si", "Europe/London", "2004-01-15T00:00:00Z", "zzzz", "GMT+00:00", "+0:00" },
        { "si", "Europe/London", "2004-07-15T00:00:00Z", "Z", "+0100", "+1:00" },
        { "si", "Europe/London", "2004-07-15T00:00:00Z", "ZZZZ", "GMT+01:00", "+1:00" },
        { "si", "Europe/London", "2004-07-15T00:00:00Z", "z", "GMT+01:00", "+1:00" },
        { "si", "Europe/London", "2004-07-15T00:00:00Z", "zzzz", "GMT+01:00", "+1:00" },
        { "si", "Europe/London", "2004-07-15T00:00:00Z", "v", "(GB)", "Europe/London" },
        { "si", "Europe/London", "2004-07-15T00:00:00Z", "vvvv", "(GB)", "Europe/London" },

        { "si", "Etc/GMT+3", "2004-01-15T00:00:00Z", "Z", "-0300", "-3:00" },
        { "si", "Etc/GMT+3", "2004-01-15T00:00:00Z", "ZZZZ", "GMT-03:00", "-3:00" },
        { "si", "Etc/GMT+3", "2004-01-15T00:00:00Z", "z", "GMT-03:00", "-3:00" },
        { "si", "Etc/GMT+3", "2004-01-15T00:00:00Z", "zzzz", "GMT-03:00", "-3:00" },
        { "si", "Etc/GMT+3", "2004-07-15T00:00:00Z", "Z", "-0300", "-3:00" },
        { "si", "Etc/GMT+3", "2004-07-15T00:00:00Z", "ZZZZ", "GMT-03:00", "-3:00" },
        { "si", "Etc/GMT+3", "2004-07-15T00:00:00Z", "z", "GMT-03:00", "-3:00" },
        { "si", "Etc/GMT+3", "2004-07-15T00:00:00Z", "zzzz", "GMT-03:00", "-3:00" },
        { "si", "Etc/GMT+3", "2004-07-15T00:00:00Z", "v", "GMT-03:00", "-3:00" },
        { "si", "Etc/GMT+3", "2004-07-15T00:00:00Z", "vvvv", "GMT-03:00", "-3:00" },

        // JB#5150
        { "si", "Asia/Calcutta", "2004-01-15T00:00:00Z", "Z", "+0530", "+5:30" },
        { "si", "Asia/Calcutta", "2004-01-15T00:00:00Z", "ZZZZ", "GMT+05:30", "+5:30" },
        { "si", "Asia/Calcutta", "2004-01-15T00:00:00Z", "z", "GMT+05:30", "+5:30" },
        { "si", "Asia/Calcutta", "2004-01-15T00:00:00Z", "zzzz", "GMT+05:30", "+5:30" },
        { "si", "Asia/Calcutta", "2004-07-15T00:00:00Z", "Z", "+0530", "+5:30" },
        { "si", "Asia/Calcutta", "2004-07-15T00:00:00Z", "ZZZZ", "GMT+05:30", "+5:30" },
        { "si", "Asia/Calcutta", "2004-07-15T00:00:00Z", "z", "GMT+05:30", "+05:30" },
        { "si", "Asia/Calcutta", "2004-07-15T00:00:00Z", "zzzz", "GMT+05:30", "+5:30" },
        { "si", "Asia/Calcutta", "2004-07-15T00:00:00Z", "v", "(IN)", "Asia/Calcutta" },
        { "si", "Asia/Calcutta", "2004-07-15T00:00:00Z", "vvvv", "(IN)", "Asia/Calcutta" },
        { NULL, NULL, NULL, NULL, NULL, NULL },
    };

    UErrorCode status = U_ZERO_ERROR;
    Calendar *cal = GregorianCalendar::createInstance(status);
    if (failure(status, "GregorianCalendar::createInstance", TRUE)) return;
    for (int i = 0; fallbackTests[i][0]; i++) {
        const char **testLine = fallbackTests[i];
        UnicodeString info[5];
        for ( int j = 0 ; j < 5 ; j++ ) {
            info[j] = UnicodeString(testLine[j], -1, US_INV);
        }
        info[4] = info[4].unescape();
        logln("%s;%s;%s;%s", testLine[0], testLine[1], testLine[2], testLine[3]);

        TimeZone *tz = TimeZone::createTimeZone(info[1]);

        if (strcmp(testLine[2], "2004-07-15T00:00:00Z") == 0) {
            cal->set(2004,6,15,0,0,0);
        } else {
            cal->set(2004,0,15,0,0,0);
        }

        SimpleDateFormat fmt(info[3], Locale(testLine[0]),status);
        ASSERT_OK(status);
        cal->adoptTimeZone(tz);
        UnicodeString result;
        FieldPosition pos(0);
        fmt.format(*cal,result,pos);
        if (result != info[4]) {
            errln(info[0] + ";" + info[1] + ";" + info[2] + ";" + info[3] + " expected: '" +
                  info[4] + "' but got: '" + result + "'");
        }
    }
    delete cal;
}

void DateFormatTest::TestRoundtripWithCalendar(void) {
    UErrorCode status = U_ZERO_ERROR;

    TimeZone *tz = TimeZone::createTimeZone("Europe/Paris");
    TimeZone *gmt = TimeZone::createTimeZone("Etc/GMT");

    Calendar *calendars[] = {
        Calendar::createInstance(*tz, Locale("und@calendar=gregorian"), status),
        Calendar::createInstance(*tz, Locale("und@calendar=buddhist"), status),
//        Calendar::createInstance(*tz, Locale("und@calendar=hebrew"), status),
        Calendar::createInstance(*tz, Locale("und@calendar=islamic"), status),
        Calendar::createInstance(*tz, Locale("und@calendar=japanese"), status),
        NULL
    };
    if (U_FAILURE(status)) {
        dataerrln("Failed to initialize calendars: %s", u_errorName(status));
        for (int i = 0; calendars[i] != NULL; i++) {
            delete calendars[i];
        }
        return;
    }

    //FIXME The formatters commented out below are currently failing because of
    // the calendar calculation problem reported by #6691

    // The order of test formatters must match the order of calendars above.
    DateFormat *formatters[] = {
        DateFormat::createDateTimeInstance(DateFormat::kFull, DateFormat::kFull, Locale("en_US")), //calendar=gregorian
        DateFormat::createDateTimeInstance(DateFormat::kFull, DateFormat::kFull, Locale("th_TH")), //calendar=buddhist
//        DateFormat::createDateTimeInstance(DateFormat::kFull, DateFormat::kFull, Locale("he_IL@calendar=hebrew")),
        DateFormat::createDateTimeInstance(DateFormat::kFull, DateFormat::kFull, Locale("ar_EG@calendar=islamic")),
//        DateFormat::createDateTimeInstance(DateFormat::kFull, DateFormat::kFull, Locale("ja_JP@calendar=japanese")),
        NULL
    };

    UDate d = Calendar::getNow();
    UnicodeString buf;
    FieldPosition fpos;
    ParsePosition ppos;

    for (int i = 0; formatters[i] != NULL; i++) {
        buf.remove();
        fpos.setBeginIndex(0);
        fpos.setEndIndex(0);
        calendars[i]->setTime(d, status);

        // Normal case output - the given calendar matches the calendar
        // used by the formatter
        formatters[i]->format(*calendars[i], buf, fpos);
        UnicodeString refStr(buf);

        for (int j = 0; calendars[j] != NULL; j++) {
            if (j == i) {
                continue;
            }
            buf.remove();
            fpos.setBeginIndex(0);
            fpos.setEndIndex(0);
            calendars[j]->setTime(d, status);

            // Even the different calendar type is specified,
            // we should get the same result.
            formatters[i]->format(*calendars[j], buf, fpos);
            if (refStr != buf) {
                errln((UnicodeString)"FAIL: Different format result with a different calendar for the same time -"
                        + "\n Reference calendar type=" + calendars[i]->getType()
                        + "\n Another calendar type=" + calendars[j]->getType()
                        + "\n Expected result=" + refStr
                        + "\n Actual result=" + buf);
            }
        }

        calendars[i]->setTimeZone(*gmt);
        calendars[i]->clear();
        ppos.setErrorIndex(-1);
        ppos.setIndex(0);

        // Normal case parse result - the given calendar matches the calendar
        // used by the formatter
        formatters[i]->parse(refStr, *calendars[i], ppos);

        for (int j = 0; calendars[j] != NULL; j++) {
            if (j == i) {
                continue;
            }
            calendars[j]->setTimeZone(*gmt);
            calendars[j]->clear();
            ppos.setErrorIndex(-1);
            ppos.setIndex(0);

            // Even the different calendar type is specified,
            // we should get the same time and time zone.
            formatters[i]->parse(refStr, *calendars[j], ppos);
            if (calendars[i]->getTime(status) != calendars[j]->getTime(status)
                || calendars[i]->getTimeZone() != calendars[j]->getTimeZone()) {
                UnicodeString tzid;
                errln((UnicodeString)"FAIL: Different parse result with a different calendar for the same string -"
                        + "\n Reference calendar type=" + calendars[i]->getType()
                        + "\n Another calendar type=" + calendars[j]->getType()
                        + "\n Date string=" + refStr
                        + "\n Expected time=" + calendars[i]->getTime(status)
                        + "\n Expected time zone=" + calendars[i]->getTimeZone().getID(tzid)
                        + "\n Actual time=" + calendars[j]->getTime(status)
                        + "\n Actual time zone=" + calendars[j]->getTimeZone().getID(tzid));
            }
        }
        if (U_FAILURE(status)) {
            errln((UnicodeString)"FAIL: " + u_errorName(status));
            break;
        }
    }

    delete tz;
    delete gmt;
    for (int i = 0; calendars[i] != NULL; i++) {
        delete calendars[i];
    }
    for (int i = 0; formatters[i] != NULL; i++) {
        delete formatters[i];
    }
}

/*
void DateFormatTest::TestRelativeError(void)
{
    UErrorCode status;
    Locale en("en");

    DateFormat *en_reltime_reldate =         DateFormat::createDateTimeInstance(DateFormat::kFullRelative,DateFormat::kFullRelative,en);
    if(en_reltime_reldate == NULL) {
        logln("PASS: rel date/rel time failed");
    } else {
        errln("FAIL: rel date/rel time created, should have failed.");
        delete en_reltime_reldate;
    }
}

void DateFormatTest::TestRelativeOther(void)
{
    logln("Nothing in this test. When we get more data from CLDR, put in some tests of -2, +2, etc. ");
}
*/

void DateFormatTest::Test6338(void)
{
    UErrorCode status = U_ZERO_ERROR;

    SimpleDateFormat *fmt1 = new SimpleDateFormat(UnicodeString("y-M-d"), Locale("ar"), status);
    if (failure(status, "new SimpleDateFormat", TRUE)) return;

    UDate dt1 = date(2008-1900, UCAL_JUNE, 10, 12, 00);
    UnicodeString str1;
    str1 = fmt1->format(dt1, str1);
    logln(str1);

    UDate dt11 = fmt1->parse(str1, status);
    failure(status, "fmt->parse");

    UnicodeString str11;
    str11 = fmt1->format(dt11, str11);
    logln(str11);

    if (str1 != str11) {
        errln((UnicodeString)"FAIL: Different dates str1:" + str1
            + " str2:" + str11);
    }
    delete fmt1;

    /////////////////

    status = U_ZERO_ERROR;
    SimpleDateFormat *fmt2 = new SimpleDateFormat(UnicodeString("y M d"), Locale("ar"), status);
    failure(status, "new SimpleDateFormat");

    UDate dt2 = date(2008-1900, UCAL_JUNE, 10, 12, 00);
    UnicodeString str2;
    str2 = fmt2->format(dt2, str2);
    logln(str2);

    UDate dt22 = fmt2->parse(str2, status);
    failure(status, "fmt->parse");

    UnicodeString str22;
    str22 = fmt2->format(dt22, str22);
    logln(str22);

    if (str2 != str22) {
        errln((UnicodeString)"FAIL: Different dates str1:" + str2
            + " str2:" + str22);
    }
    delete fmt2;

    /////////////////

    status = U_ZERO_ERROR;
    SimpleDateFormat *fmt3 = new SimpleDateFormat(UnicodeString("y-M-d"), Locale("en-us"), status);
    failure(status, "new SimpleDateFormat");

    UDate dt3 = date(2008-1900, UCAL_JUNE, 10, 12, 00);
    UnicodeString str3;
    str3 = fmt3->format(dt3, str3);
    logln(str3);

    UDate dt33 = fmt3->parse(str3, status);
    failure(status, "fmt->parse");

    UnicodeString str33;
    str33 = fmt3->format(dt33, str33);
    logln(str33);

    if (str3 != str33) {
        errln((UnicodeString)"FAIL: Different dates str1:" + str3
            + " str2:" + str33);
    }
    delete fmt3;

    /////////////////

    status = U_ZERO_ERROR;
    SimpleDateFormat *fmt4 = new SimpleDateFormat(UnicodeString("y M  d"), Locale("en-us"), status);
    failure(status, "new SimpleDateFormat");

    UDate dt4 = date(2008-1900, UCAL_JUNE, 10, 12, 00);
    UnicodeString str4;
    str4 = fmt4->format(dt4, str4);
    logln(str4);

    UDate dt44 = fmt4->parse(str4, status);
    failure(status, "fmt->parse");

    UnicodeString str44;
    str44 = fmt4->format(dt44, str44);
    logln(str44);

    if (str4 != str44) {
        errln((UnicodeString)"FAIL: Different dates str1:" + str4
            + " str2:" + str44);
    }
    delete fmt4;

}

void DateFormatTest::Test6726(void)
{
    // status
//    UErrorCode status = U_ZERO_ERROR;

    // fmtf, fmtl, fmtm, fmts;
    UnicodeString strf, strl, strm, strs;
    UDate dt = date(2008-1900, UCAL_JUNE, 10, 12, 00);

    Locale loc("ja");
    DateFormat* fmtf = DateFormat::createDateTimeInstance(DateFormat::FULL, DateFormat::FULL, loc);
    DateFormat* fmtl = DateFormat::createDateTimeInstance(DateFormat::LONG, DateFormat::FULL, loc);
    DateFormat* fmtm = DateFormat::createDateTimeInstance(DateFormat::MEDIUM, DateFormat::FULL, loc);
    DateFormat* fmts = DateFormat::createDateTimeInstance(DateFormat::SHORT, DateFormat::FULL, loc);
    if (fmtf == NULL || fmtl == NULL || fmtm == NULL || fmts == NULL) {
        dataerrln("Unable to create DateFormat. got NULL.");
        /* It may not be true that if one is NULL all is NULL.  Just to be safe. */
        delete fmtf;
        delete fmtl;
        delete fmtm;
        delete fmts;

        return;
    }
    strf = fmtf->format(dt, strf);
    strl = fmtl->format(dt, strl);
    strm = fmtm->format(dt, strm);
    strs = fmts->format(dt, strs);


/* Locale data is not yet updated
    if (strf.charAt(13) == UChar(0x20)) {
        errln((UnicodeString)"FAIL: Improper formatted date: " + strf);
    }
    if (strl.charAt(10) == UChar(0x20)) {
        errln((UnicodeString)"FAIL: Improper formatted date: " + strl);
    }
*/
    logln("strm.charAt(10)=%04X wanted 0x20\n", strm.charAt(10));
    if (strm.charAt(10) != UChar(0x0020)) {
      errln((UnicodeString)"FAIL: Improper formatted date: " + strm );
    }
    logln("strs.charAt(10)=%04X wanted 0x20\n", strs.charAt(8));
    if (strs.charAt(8)  != UChar(0x0020)) {
        errln((UnicodeString)"FAIL: Improper formatted date: " + strs);
    }

    delete fmtf;
    delete fmtl;
    delete fmtm;
    delete fmts;

    return;
}

/**
 * Test DateFormat's parsing of default GMT variants.  See ticket#6135
 */
void DateFormatTest::TestGMTParsing() {
    const char* DATA[] = {
        "HH:mm:ss Z",

        // pattern, input, expected output (in quotes)
        "HH:mm:ss Z",       "10:20:30 GMT+03:00",   "10:20:30 +0300",
        "HH:mm:ss Z",       "10:20:30 UT-02:00",    "10:20:30 -0200",
        "HH:mm:ss Z",       "10:20:30 GMT",         "10:20:30 +0000",
        "HH:mm:ss vvvv",    "10:20:30 UT+10:00",    "10:20:30 +1000",
        "HH:mm:ss zzzz",    "10:20:30 UTC",         "10:20:30 +0000",   // standalone "UTC"
        "ZZZZ HH:mm:ss",    "UT 10:20:30",          "10:20:30 +0000",
        "V HH:mm:ss",       "UT+0130 10:20:30",     "10:20:30 +0130",
        "V HH:mm:ss",       "UTC+0130 10:20:30",    NULL,               // UTC+0130 is not a supported pattern
        "HH mm Z ss",       "10 20 GMT-1100 30",    "10:20:30 -1100",
    };
    const int32_t DATA_len = sizeof(DATA)/sizeof(DATA[0]);
    expectParse(DATA, DATA_len, Locale("en"));
}

// Test case for localized GMT format parsing
// with no delimitters in offset format (Chinese locale)
void DateFormatTest::Test6880() {
    UErrorCode status = U_ZERO_ERROR;
    UDate d1, d2, dp1, dp2, dexp1, dexp2;
    UnicodeString s1, s2;

    TimeZone *tz = TimeZone::createTimeZone("Asia/Shanghai");
    GregorianCalendar gcal(*tz, status);
    if (failure(status, "construct GregorianCalendar", TRUE)) return;
    
    gcal.clear();
    gcal.set(1910, UCAL_JULY, 1, 12, 00);   // offset 8:05:52
    d1 = gcal.getTime(status);

    gcal.clear();
    gcal.set(1950, UCAL_JULY, 1, 12, 00);   // offset 8:00
    d2 = gcal.getTime(status);

    gcal.clear();
    gcal.set(1970, UCAL_JANUARY, 1, 12, 00);
    dexp2 = gcal.getTime(status);
    dexp1 = dexp2 - (5*60 + 52)*1000;   // subtract 5m52s

    if (U_FAILURE(status)) {
        errln("FAIL: Gregorian calendar error");
    }

    DateFormat *fmt = DateFormat::createTimeInstance(DateFormat::kFull, Locale("zh"));
    if (fmt == NULL) {
        dataerrln("Unable to create DateFormat. Got NULL.");
        return;
    }
    fmt->adoptTimeZone(tz);

    fmt->format(d1, s1);
    fmt->format(d2, s2);

    dp1 = fmt->parse(s1, status);
    dp2 = fmt->parse(s2, status);

    if (U_FAILURE(status)) {
        errln("FAIL: Parse failure");
    }

    if (dp1 != dexp1) {
        errln("FAIL: Failed to parse " + s1 + " parsed: " + dp1 + " expected: " + dexp1);
    }
    if (dp2 != dexp2) {
        errln("FAIL: Failed to parse " + s2 + " parsed: " + dp2 + " expected: " + dexp2);
    }

    delete fmt;
}

void DateFormatTest::TestISOEra() { 
   
    const char* data[] = { 
    // input, output 
    "BC 4004-10-23T07:00:00Z", "BC 4004-10-23T07:00:00Z", 
    "AD 4004-10-23T07:00:00Z", "AD 4004-10-23T07:00:00Z", 
    "-4004-10-23T07:00:00Z"  , "BC 4005-10-23T07:00:00Z", 
    "4004-10-23T07:00:00Z"   , "AD 4004-10-23T07:00:00Z", 
    }; 
 
    int32_t numData = 8; 
 
    UErrorCode status = U_ZERO_ERROR; 
 
    // create formatter 
    SimpleDateFormat *fmt1 = new SimpleDateFormat(UnicodeString("GGG yyyy-MM-dd'T'HH:mm:ss'Z"), status); 
    failure(status, "new SimpleDateFormat", TRUE); 

    for(int i=0; i < numData; i+=2) { 
        // create input string 
        UnicodeString in = data[i]; 
 
        // parse string to date 
        UDate dt1 = fmt1->parse(in, status); 
        failure(status, "fmt->parse", TRUE); 
 
        // format date back to string 
        UnicodeString out; 
        out = fmt1->format(dt1, out); 
        logln(out); 
 
        // check that roundtrip worked as expected 
        UnicodeString expected = data[i+1]; 
        if (out != expected) { 
            dataerrln((UnicodeString)"FAIL: " + in + " -> " + out + " expected -> " + expected); 
        } 
    } 
 
    delete fmt1; 	 
} 
void DateFormatTest::TestFormalChineseDate() { 
   
    UErrorCode status = U_ZERO_ERROR; 
    UnicodeString pattern ("y\\u5e74M\\u6708d\\u65e5", -1, US_INV );
    pattern = pattern.unescape();
    UnicodeString override ("y=hanidec;M=hans;d=hans", -1, US_INV );
    
    // create formatter 
    SimpleDateFormat *sdf = new SimpleDateFormat(pattern,override,Locale::getChina(),status);
    failure(status, "new SimpleDateFormat with override", TRUE); 

    UDate thedate = date(2009-1900, UCAL_JULY, 28);
    FieldPosition pos(0);
    UnicodeString result;
    sdf->format(thedate,result,pos);
 
    UnicodeString expected = "\\u4e8c\\u3007\\u3007\\u4e5d\\u5e74\\u4e03\\u6708\\u4e8c\\u5341\\u516b\\u65e5"; 
    expected = expected.unescape();
    if (result != expected) { 
        dataerrln((UnicodeString)"FAIL: -> " + result + " expected -> " + expected); 
    } 
 
    UDate parsedate = sdf->parse(expected,status);
    if ( parsedate != thedate ) {
        UnicodeString pat1 ("yyyy-MM-dd'T'HH:mm:ss'Z'", -1, US_INV );
        SimpleDateFormat *usf = new SimpleDateFormat(pat1,Locale::getEnglish(),status);
        UnicodeString parsedres,expres;
        usf->format(parsedate,parsedres,pos);
        usf->format(thedate,expres,pos);
        errln((UnicodeString)"FAIL: parsed -> " + parsedres + " expected -> " + expres); 
        delete usf;
    }
    delete sdf; 	 
} 

#endif /* #if !UCONFIG_NO_FORMATTING */

//eof
