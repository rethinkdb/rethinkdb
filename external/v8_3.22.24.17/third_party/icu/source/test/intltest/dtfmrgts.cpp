/********************************************************************
 * COPYRIGHT: 
 * Copyright (c) 1997-2010, International Business Machines Corporation and
 * others. All Rights Reserved.
 ********************************************************************/

#include "unicode/utypes.h"

#if !UCONFIG_NO_FORMATTING

#include "dtfmrgts.h"

#include "unicode/timezone.h"
#include "unicode/gregocal.h"
#include "unicode/smpdtfmt.h"
#include "unicode/datefmt.h"
#include "unicode/simpletz.h"
#include "unicode/resbund.h"

// *****************************************************************************
// class DateFormatRegressionTest
// *****************************************************************************

#define CASE(id,test) case id: name = #test; if (exec) { logln(#test "---"); logln((UnicodeString)""); test(); } break;

void 
DateFormatRegressionTest::runIndexedTest( int32_t index, UBool exec, const char* &name, char* /*par*/ )
{
    // if (exec) logln((UnicodeString)"TestSuite DateFormatRegressionTest");
    switch (index) {
        CASE(0,Test4029195)
        CASE(1,Test4052408)
        CASE(2,Test4056591)
        CASE(3,Test4059917)
        CASE(4,Test4060212)
        CASE(5,Test4061287)
        CASE(6,Test4065240)
        CASE(7,Test4071441)
        CASE(8,Test4073003)
        CASE(9,Test4089106)
        CASE(10,Test4100302)
        CASE(11,Test4101483)
        CASE(12,Test4103340)
        CASE(13,Test4103341)
        CASE(14,Test4104136)
        CASE(15,Test4104522)
        CASE(16,Test4106807)
        CASE(17,Test4108407) 
        CASE(18,Test4134203)
        CASE(19,Test4151631)
        CASE(20,Test4151706)
        CASE(21,Test4162071)
        CASE(22,Test4182066)
        CASE(23,Test4210209)
        CASE(24,Test714)
        CASE(25,Test1684)
        CASE(26,Test5554)
        default: name = ""; break;
    }
}

/**
 * @bug 4029195
 */
void DateFormatRegressionTest::Test4029195(void) 
{
    UErrorCode status = U_ZERO_ERROR;

    UDate today = Calendar::getNow();
    logln((UnicodeString) "today: " + today);

    SimpleDateFormat *sdf = (SimpleDateFormat*) DateFormat::createDateInstance();
    if (failure(status, "SimpleDateFormat::createDateInstance")) {
        return;
    }
    UnicodeString pat;
    if(sdf == NULL){
        dataerrln("Error calling DateFormat::createDateTimeInstance");
        return;
    }

    pat = sdf->toPattern(pat);
    logln("pattern: " + pat);
    UnicodeString fmtd;
    FieldPosition pos(FieldPosition::DONT_CARE);
    fmtd = sdf->format(today, fmtd, pos);
    logln("today: " + fmtd);

    sdf->applyPattern("G yyyy DDD");
    UnicodeString todayS;
    todayS = sdf->format(today, todayS, pos);
    logln("today: " + todayS);
    //try {
        today = sdf->parse(todayS, status);
        failure(status, "sdf->parse");
        logln((UnicodeString)"today date: " + today);
    /*} catch(Exception e) {
        logln("Error reparsing date: " + e.getMessage());
    }*/

    //try {
        UnicodeString rt;
        rt = sdf->format(sdf->parse(todayS, status), rt, pos);
        failure(status, "sdf->parse");
        logln("round trip: " + rt);
        if(rt != todayS) 
            errln("Fail: Want " + todayS + " Got " + rt);
    /*}
    catch (ParseException e) {
        errln("Fail: " + e);
        e.printStackTrace();
    }*/

    delete sdf;
}

/**
 * @bug 4052408
 */
void DateFormatRegressionTest::Test4052408(void) 
{

    DateFormat *fmt = DateFormat::createDateTimeInstance(DateFormat::SHORT,
                                                DateFormat::SHORT, Locale::getUS());
    if (fmt == NULL) {
        dataerrln("Error calling DateFormat::createDateTimeInstance");
        return;
    }

    UDate dt = date(97, UCAL_MAY, 3, 8, 55);
    UnicodeString str;
    str = fmt->format(dt, str);
    logln(str);
    
    if(str != "5/3/97 8:55 AM")
        errln("Fail: Test broken; Want 5/3/97 8:55 AM Got " + str);   
    
    UnicodeString expected[] = {
        (UnicodeString) "", //"ERA_FIELD",
        (UnicodeString) "97", //"YEAR_FIELD",
        (UnicodeString) "5", //"MONTH_FIELD",
        (UnicodeString) "3", //"DATE_FIELD",
        (UnicodeString) "", //"HOUR_OF_DAY1_FIELD",
        (UnicodeString) "", //"HOUR_OF_DAY0_FIELD",
        (UnicodeString) "55", //"MINUTE_FIELD",
        (UnicodeString) "", //"SECOND_FIELD",
        (UnicodeString) "", //"MILLISECOND_FIELD",
        (UnicodeString) "", //"DAY_OF_WEEK_FIELD",
        (UnicodeString) "", //"DAY_OF_YEAR_FIELD",
        (UnicodeString) "", //"DAY_OF_WEEK_IN_MONTH_FIELD",
        (UnicodeString) "", //"WEEK_OF_YEAR_FIELD",
        (UnicodeString) "", //"WEEK_OF_MONTH_FIELD",
        (UnicodeString) "AM", //"AM_PM_FIELD",
        (UnicodeString) "8", //"HOUR1_FIELD",
        (UnicodeString) "", //"HOUR0_FIELD",
        (UnicodeString) "" //"TIMEZONE_FIELD"
    };
    
    //Hashtable expected;// = new Hashtable();
    //expected.put(new LongKey(DateFormat.MONTH_FIELD), "5");
    //expected.put(new LongKey(DateFormat.DATE_FIELD), "3");
    //expected.put(new LongKey(DateFormat.YEAR_FIELD), "97");
    //expected.put(new LongKey(DateFormat.HOUR1_FIELD), "8");
    //expected.put(new LongKey(DateFormat.MINUTE_FIELD), "55");
    //expected.put(new LongKey(DateFormat.AM_PM_FIELD), "AM");
    
    //StringBuffer buf = new StringBuffer();
    UnicodeString fieldNames[] = {
        (UnicodeString) "ERA_FIELD",
        (UnicodeString) "YEAR_FIELD",
        (UnicodeString) "MONTH_FIELD",
        (UnicodeString) "DATE_FIELD",
        (UnicodeString) "HOUR_OF_DAY1_FIELD",
        (UnicodeString) "HOUR_OF_DAY0_FIELD",
        (UnicodeString) "MINUTE_FIELD",
        (UnicodeString) "SECOND_FIELD",
        (UnicodeString) "MILLISECOND_FIELD",
        (UnicodeString) "DAY_OF_WEEK_FIELD",
        (UnicodeString) "DAY_OF_YEAR_FIELD",
        (UnicodeString) "DAY_OF_WEEK_IN_MONTH_FIELD",
        (UnicodeString) "WEEK_OF_YEAR_FIELD",
        (UnicodeString) "WEEK_OF_MONTH_FIELD",
        (UnicodeString) "AM_PM_FIELD",
        (UnicodeString) "HOUR1_FIELD",
        (UnicodeString) "HOUR0_FIELD",
        (UnicodeString) "TIMEZONE_FIELD"
    };

    UBool pass = TRUE;
    for(int i = 0; i <= 17; ++i) {
        FieldPosition pos(i);
        UnicodeString buf;
        fmt->format(dt, buf, pos);
        //char[] dst = new char[pos.getEndIndex() - pos.getBeginIndex()];
        UnicodeString dst;
        buf.extractBetween(pos.getBeginIndex(), pos.getEndIndex(), dst);
        UnicodeString str(dst);
        logln((UnicodeString)"" + i + (UnicodeString)": " + fieldNames[i] +
                (UnicodeString)", \"" + str + (UnicodeString)"\", " +
                pos.getBeginIndex() + (UnicodeString)", " +
                pos.getEndIndex());
        UnicodeString exp = expected[i];
        if((exp.length() == 0 && str.length() == 0) || str == exp)
            logln(" ok");
        else {
            errln(UnicodeString(" expected ") + exp);
            pass = FALSE;
        }
        
    }
    if( ! pass) 
        errln("Fail: FieldPosition not set right by DateFormat");
    
    delete fmt;
}

/**
 * @bug 4056591
 * Verify the function of the [s|g]et2DigitYearStart() API.
 */
void DateFormatRegressionTest::Test4056591(void) 
{
    UErrorCode status = U_ZERO_ERROR;

    //try {
        SimpleDateFormat *fmt = new SimpleDateFormat(UnicodeString("yyMMdd"), Locale::getUS(), status);
        if (failure(status, "new SimpleDateFormat", TRUE)) {
            delete fmt;
            return;
        }
        UDate start = date(1809-1900, UCAL_DECEMBER, 25);
        fmt->set2DigitYearStart(start, status);
        failure(status, "fmt->setTwoDigitStartDate");
        if( (fmt->get2DigitYearStart(status) != start) || failure(status, "get2DigitStartDate"))
            errln("get2DigitYearStart broken");
        UDate dates [] = {
            date(1809-1900, UCAL_DECEMBER, 25),
            date(1909-1900, UCAL_DECEMBER, 24),
            date(1809-1900, UCAL_DECEMBER, 26),
            date(1861-1900, UCAL_DECEMBER, 25),
        };

        UnicodeString strings [] = {
            (UnicodeString) "091225",
            (UnicodeString) "091224",
            (UnicodeString) "091226",
            (UnicodeString) "611225"
        };

        /*Object[] DATA = {
            "091225", new Date(1809-1900, Calendar.DECEMBER, 25),
            "091224", new Date(1909-1900, Calendar.DECEMBER, 24),
            "091226", new Date(1809-1900, Calendar.DECEMBER, 26),
            "611225", new Date(1861-1900, Calendar.DECEMBER, 25),
        };*/

        for(int i = 0; i < 4; i++) {
            UnicodeString s = strings[i];
            UDate exp = dates[i];
            UDate got = fmt->parse(s, status);
            failure(status, "fmt->parse");
            logln(s + " -> " + got + "; exp " + exp);
            if(got != exp) 
                errln("set2DigitYearStart broken");
        }
    /*}
    catch (ParseException e) {
        errln("Fail: " + e);
        e.printStackTrace();
    }*/

    delete fmt;
}

/**
 * @bug 4059917
 */
void DateFormatRegressionTest::Test4059917(void) 
{
    UErrorCode status = U_ZERO_ERROR;
    
    SimpleDateFormat *fmt;
    UnicodeString myDate;

    fmt = new SimpleDateFormat( UnicodeString("yyyy/MM/dd"), status );
    if (failure(status, "new SimpleDateFormat", TRUE)) return;
    myDate = "1997/01/01";
    aux917( fmt, myDate );
    
    delete fmt;
    fmt = NULL;
    
    fmt = new SimpleDateFormat( UnicodeString("yyyyMMdd"), status );
    if(failure(status, "new SimpleDateFormat")) return;
    myDate = "19970101";
    aux917( fmt, myDate );
              
    delete fmt;
}

void DateFormatRegressionTest::aux917( SimpleDateFormat *fmt, UnicodeString& str ) {
    //try {
    UnicodeString pat;
    pat = fmt->toPattern(pat);
    logln( "==================" );
    logln( "testIt: pattern=" + pat +
               " string=" + str );
                
        
    Formattable o;
    //Object o;
    ParsePosition pos(0);
    fmt->parseObject( str, o, pos );
    //logln( UnicodeString("Parsed object: ") + o );
    
    UErrorCode status = U_ZERO_ERROR;
    UnicodeString formatted;
    FieldPosition poss(FieldPosition::DONT_CARE);
    formatted = fmt->format( o, formatted, poss, status );
    failure(status, "fmt->format");
    logln( "Formatted string: " + formatted );
    if( formatted != str) 
        errln("Fail: Want " + str + " Got " + formatted);
    /*}
    catch (ParseException e) {
        errln("Fail: " + e);
        e.printStackTrace();
    }*/
}

/**
 * @bug 4060212
 */
void DateFormatRegressionTest::Test4060212(void) 
{
    UnicodeString dateString = "1995-040.05:01:29";

    logln( "dateString= " + dateString );
    logln("Using yyyy-DDD.hh:mm:ss");
    UErrorCode status = U_ZERO_ERROR;
    SimpleDateFormat *formatter = new SimpleDateFormat(UnicodeString("yyyy-DDD.hh:mm:ss"), status);
    if (failure(status, "new SimpleDateFormat", TRUE)) return;
    ParsePosition pos(0);
    UDate myDate = formatter->parse( dateString, pos );
    UnicodeString myString;
    DateFormat *fmt = DateFormat::createDateTimeInstance( DateFormat::FULL,
                                                            DateFormat::LONG);
    if (fmt == NULL) {
        dataerrln("Error calling DateFormat::createDateTimeInstance");
        delete formatter;
        return;
    }

    myString = fmt->format( myDate, myString);
    logln( myString );

    Calendar *cal = new GregorianCalendar(status);
    failure(status, "new GregorianCalendar");
    cal->setTime(myDate, status);
    failure(status, "cal->setTime");
    if ((cal->get(UCAL_DAY_OF_YEAR, status) != 40) || failure(status, "cal->get"))
        errln((UnicodeString) "Fail: Got " + cal->get(UCAL_DAY_OF_YEAR, status) +
                            " Want 40");

    logln("Using yyyy-ddd.hh:mm:ss");
    delete formatter;
    formatter = NULL;
    formatter = new SimpleDateFormat(UnicodeString("yyyy-ddd.hh:mm:ss"), status);
    if(failure(status, "new SimpleDateFormat")) return;
    pos.setIndex(0);
    myDate = formatter->parse( dateString, pos );
    myString = fmt->format( myDate, myString );
    logln( myString );
    cal->setTime(myDate, status);
    failure(status, "cal->setTime");
    if ((cal->get(UCAL_DAY_OF_YEAR, status) != 40) || failure(status, "cal->get"))
        errln((UnicodeString) "Fail: Got " + cal->get(UCAL_DAY_OF_YEAR, status) +
                            " Want 40");

    delete formatter;
    delete fmt;
    delete cal;
}

/**
 * @bug 4061287
 */
void DateFormatRegressionTest::Test4061287(void) 
{
    UErrorCode status = U_ZERO_ERROR;
    
    SimpleDateFormat *df = new SimpleDateFormat(UnicodeString("dd/MM/yyyy"), status);
    if (U_FAILURE(status)) {
        dataerrln("Fail new SimpleDateFormat: %s", u_errorName(status));
        delete df;
        return;
    }
    failure(status, "new SimpleDateFormat");
    //try {
    logln(UnicodeString("") + df->parse("35/01/1971", status));  
    failure(status, "df->parse");
    //logln(df.parse("35/01/1971").toString());
    //}
    /*catch (ParseException e) {
        errln("Fail: " + e);
        e.printStackTrace();
    }*/
    df->setLenient(FALSE);
    UBool ok = FALSE;
    //try {
    logln(UnicodeString("") + df->parse("35/01/1971", status));
    if(U_FAILURE(status))
        ok = TRUE;
    //logln(df.parse("35/01/1971").toString());
    //} catch (ParseException e) {ok=TRUE;}
    if(!ok) 
        errln("Fail: Lenient not working");
    delete df;
}

/**
 * @bug 4065240
 */
void DateFormatRegressionTest::Test4065240(void) 
{
    UDate curDate;
    DateFormat *shortdate, *fulldate;
    UnicodeString strShortDate, strFullDate;
    Locale saveLocale = Locale::getDefault();
    TimeZone *saveZone = TimeZone::createDefault();

    UErrorCode status = U_ZERO_ERROR;
    //try {
        Locale *curLocale = new Locale("de","DE");
        Locale::setDefault(*curLocale, status);
        failure(status, "Locale::setDefault");
        // {sfb} adoptDefault instead of setDefault
        //TimeZone::setDefault(TimeZone::createTimeZone("EST"));
        TimeZone::adoptDefault(TimeZone::createTimeZone("EST"));
        curDate = date(98, 0, 1);
        shortdate = DateFormat::createDateInstance(DateFormat::SHORT);
        if (shortdate == NULL){
            dataerrln("Error calling DateFormat::createDateInstance");
            return;
        }

        fulldate = DateFormat::createDateTimeInstance(DateFormat::LONG, DateFormat::LONG);
        if (fulldate == NULL){
            dataerrln("Error calling DateFormat::createDateTimeInstance");
            return;
        }
        strShortDate = "The current date (short form) is ";
        UnicodeString temp;
        temp = shortdate->format(curDate, temp);
        strShortDate += temp;
        strFullDate = "The current date (long form) is ";
        UnicodeString temp2;
        fulldate->format(curDate, temp2);
        strFullDate += temp2;

        logln(strShortDate);
        logln(strFullDate);

        // {sfb} What to do with resource bundle stuff?????

        // Check to see if the resource is present; if not, we can't test
        ResourceBundle *bundle = new ResourceBundle(
            NULL, *curLocale, status);
        failure(status, "new ResourceBundle");
            //(UnicodeString) "java.text.resources.DateFormatZoneData", curLocale);

        // {sfb} API change to ResourceBundle -- add getLocale()
        /*if (bundle->getLocale().getLanguage(temp) == UnicodeString("de")) {
            // UPDATE THIS AS ZONE NAME RESOURCE FOR <EST> in de_DE is updated
            if (!strFullDate.endsWith(UnicodeString("GMT-05:00")))
                errln("Fail: Want GMT-05:00");
        }
        else {
            logln("*** TEST COULD NOT BE COMPLETED BECAUSE DateFormatZoneData ***");
            logln("*** FOR LOCALE de OR de_DE IS MISSING ***");
        }*/
    //}
    //finally {
    Locale::setDefault(saveLocale, status);
    failure(status, "Locale::setDefault");
    TimeZone::setDefault(*saveZone);
    //}
    delete shortdate;
    delete fulldate;
    delete saveZone;
    delete curLocale;
    delete bundle;
}

/*
  DateFormat.equals is too narrowly defined.  As a result, MessageFormat
  does not work correctly.  DateFormat.equals needs to be written so
  that the Calendar sub-object is not compared using Calendar.equals,
  but rather compared for equivalency.  This may necessitate adding a
  (package private) method to Calendar to test for equivalency.
  
  Currently this bug breaks MessageFormat.toPattern
  */
/**
 * @bug 4071441
 */
void DateFormatRegressionTest::Test4071441(void) 
{
    DateFormat *fmtA = DateFormat::createInstance();
    DateFormat *fmtB = DateFormat::createInstance();

    if (fmtA == NULL || fmtB == NULL){
        dataerrln("Error calling DateFormat::createInstance");
        delete fmtA;
        delete fmtB;
        return;
    }

    // {sfb} Is it OK to cast away const here?
    Calendar *calA = (Calendar*) fmtA->getCalendar();
    Calendar *calB = (Calendar*) fmtB->getCalendar();
    if(!calA || !calB) {
      errln("Couldn't get proper calendars, exiting");
      delete fmtA;
      delete fmtB;
      return;
    }
    UDate epoch = date(0, 0, 0);
    UDate xmas = date(61, UCAL_DECEMBER, 25);

    UErrorCode status = U_ZERO_ERROR;
    calA->setTime(epoch, status);
    failure(status, "calA->setTime");
    calB->setTime(epoch, status);
    failure(status, "calB->setTime");
    if (*calA != *calB)
        errln("Fail: Can't complete test; Calendar instances unequal");
    if (*fmtA != *fmtB)
        errln("Fail: DateFormat unequal when Calendars equal");
    calB->setTime(xmas, status);
    failure(status, "calB->setTime");
    if (*calA == *calB)
        errln("Fail: Can't complete test; Calendar instances equal");
    if (*fmtA != *fmtB)
        errln("Fail: DateFormat unequal when Calendars equivalent");

    logln("DateFormat.equals ok");

    delete fmtA;
    delete fmtB;
}

/* The java.text.DateFormat.parse(String) method expects for the
  US locale a string formatted according to mm/dd/yy and parses it
  correctly.

  When given a string mm/dd/yyyy it only parses up to the first
  two y's, typically resulting in a date in the year 1919.
  
  Please extend the parsing method(s) to handle strings with
  four-digit year values (probably also applicable to various
  other locales.  */
/**
 * @bug 4073003
 */
void DateFormatRegressionTest::Test4073003(void) 
{
    //try {
    UErrorCode ec = U_ZERO_ERROR;
    SimpleDateFormat fmt("dd/MM/yy", Locale::getUK(), ec);
    if (U_FAILURE(ec)) {
        dataerrln("FAIL: SimpleDateFormat constructor - %s", u_errorName(ec));
        return;
    }
        UnicodeString tests [] = { 
            (UnicodeString) "12/25/61", 
            (UnicodeString) "12/25/1961", 
            (UnicodeString) "4/3/2010", 
            (UnicodeString) "4/3/10" 
        };
        UErrorCode status = U_ZERO_ERROR;
        for(int i= 0; i < 4; i+=2) {
            UDate d = fmt.parse(tests[i], status);
            failure(status, "fmt.parse");
            UDate dd = fmt.parse(tests[i+1], status);
            failure(status, "fmt.parse");
            UnicodeString s;
            s = fmt.format(d, s);
            UnicodeString ss;
            ss = fmt.format(dd, ss);
            if (d != dd)
                errln((UnicodeString) "Fail: " + d + " != " + dd);
            if (s != ss)
                errln((UnicodeString)"Fail: " + s + " != " + ss);
            logln("Ok: " + s + " " + d);
        }
}

/**
 * @bug 4089106
 */
void DateFormatRegressionTest::Test4089106(void) 
{
    TimeZone *def = TimeZone::createDefault();
    //try {
        TimeZone *z = new SimpleTimeZone((int)(1.25 * 3600000), "FAKEZONE");
        TimeZone::setDefault(*z);
        UErrorCode status = U_ZERO_ERROR;
        SimpleDateFormat *f = new SimpleDateFormat(status);
        if(U_FAILURE(status)) {
          dataerrln("Couldn't create SimpleDateFormat, error %s", u_errorName(status));
          delete f;
          delete def;
          delete z;
          return;
        }
        failure(status, "new SimpleDateFormat");
        if (f->getTimeZone()!= *z)
            errln("Fail: SimpleTimeZone should use TimeZone.getDefault()");
        
        //}
    //finally {
        TimeZone::setDefault(*def);
    //}

    delete z;
    delete f;
    delete def;
}

/**
 * @bug 4100302
 */

// {sfb} not applicable in C++??

void DateFormatRegressionTest::Test4100302(void) 
{
/*    Locale locales [] =  {
        Locale::CANADA,
        Locale::CANADA_FRENCH,
        Locale::CHINA,
        Locale::CHINESE,
        Locale::ENGLISH,
        Locale::FRANCE,
        Locale::FRENCH,
        Locale::GERMAN,
        Locale::GERMANY,
        Locale::ITALIAN,
        Locale::ITALY,
        Locale::JAPAN,
        Locale::JAPANESE,
        Locale::KOREA,
        Locale::KOREAN,
        Locale::PRC,
        Locale::SIMPLIFIED_CHINESE,
        Locale::TAIWAN,
        Locale::TRADITIONAL_CHINESE,
        Locale::UK,
        Locale::US
        };
    //try {
        UBool pass = TRUE;
        for(int i = 0; i < 21; i++) {

            Format *format = DateFormat::createDateTimeInstance(DateFormat::FULL,
                DateFormat::FULL, locales[i]);
            byte[] bytes;
        
            ByteArrayOutputStream baos = new ByteArrayOutputStream();
            ObjectOutputStream oos = new ObjectOutputStream(baos);
        
            oos.writeObject(format);
            oos.flush();
        
            baos.close();
            bytes = baos.toByteArray();
        
            ObjectInputStream ois =
                new ObjectInputStream(new ByteArrayInputStream(bytes));
        
            if (!format.equals(ois.readObject())) {
                pass = FALSE;
                logln("DateFormat instance for locale " +
                      locales[i] + " is incorrectly serialized/deserialized.");
            } else {
                logln("DateFormat instance for locale " +
                      locales[i] + " is OKAY.");
            }
        }
        if (!pass) errln("Fail: DateFormat serialization/equality bug");      
    }
    catch (IOException e) {
        errln("Fail: " + e);
        e.printStackTrace();
    }
    catch (ClassNotFoundException e) {
        errln("Fail: " + e);
        e.printStackTrace();
    }
*/}

/**
 * @bug 4101483
 */
void DateFormatRegressionTest::Test4101483(void) 
{
    UErrorCode status = U_ZERO_ERROR;
    SimpleDateFormat *sdf = new SimpleDateFormat(UnicodeString("z"), Locale::getUS(), status);
    if (failure(status, "new SimpleDateFormat", TRUE)) return;
    FieldPosition fp(UDAT_TIMEZONE_FIELD);
    //Date d = date(9234567890L);
    UDate d = 9234567890.0;
    //StringBuffer buf = new StringBuffer("");
    UnicodeString buf;
    sdf->format(d, buf, fp);
    //logln(sdf.format(d, buf, fp).toString());
    logln(dateToString(d) + " => " + buf);
    logln("beginIndex = " + fp.getBeginIndex());
    logln("endIndex = " + fp.getEndIndex());
    if (fp.getBeginIndex() == fp.getEndIndex()) 
        errln("Fail: Empty field");

    delete sdf;
}

/**
 * @bug 4103340
 * @bug 4138203
 * This bug really only works in Locale.US, since that's what the locale
 * used for Date.toString() is.  Bug 4138203 reports that it fails on Korean
 * NT; it would actually have failed on any non-US locale.  Now it should
 * work on all locales.
 */
void DateFormatRegressionTest::Test4103340(void) 
{
    UErrorCode status = U_ZERO_ERROR;

    // choose a date that is the FIRST of some month 
    // and some arbitrary time 
    UDate d = date(97, 3, 1, 1, 1, 1); 
    SimpleDateFormat *df = new SimpleDateFormat(UnicodeString("MMMM"), Locale::getUS(), status); 
    if (failure(status, "new SimpleDateFormat", TRUE)) return;

    UnicodeString s;
    s = dateToString(d, s);
    UnicodeString s2;
    FieldPosition pos(FieldPosition::DONT_CARE);
    s2 = df->format(d, s2, pos);
    logln("Date=" + s); 
    logln("DF=" + s2);
    UnicodeString substr;
    s2.extract(0,2, substr);
    if (s.indexOf(substr) == -1)
      errln("Months should match");
    
    delete df;
}

/**
 * @bug 4103341
 */
void DateFormatRegressionTest::Test4103341(void) 
{
    TimeZone *saveZone  =TimeZone::createDefault();
    //try {
        
    // {sfb} changed from setDefault to adoptDefault
    TimeZone::adoptDefault(TimeZone::createTimeZone("CST"));
    UErrorCode status = U_ZERO_ERROR;
    SimpleDateFormat *simple = new SimpleDateFormat(UnicodeString("MM/dd/yyyy HH:mm"), status);
    if(U_FAILURE(status)) {
      dataerrln("Couldn't create SimpleDateFormat, error %s", u_errorName(status));
      delete simple;
      return;
    }
    failure(status, "new SimpleDateFormat");
    TimeZone *temp = TimeZone::createDefault();
    if(simple->getTimeZone() != *temp)
            errln("Fail: SimpleDateFormat not using default zone");
    //}
    //finally {
        TimeZone::adoptDefault(saveZone);
    //}

    delete temp;
    delete simple;
}

/**
 * @bug 4104136
 */
void DateFormatRegressionTest::Test4104136(void) 
{
    UErrorCode status = U_ZERO_ERROR;
    SimpleDateFormat *sdf = new SimpleDateFormat(status); 
    if(U_FAILURE(status)) {
      dataerrln("Couldn't create SimpleDateFormat, error %s", u_errorName(status));
      delete sdf;
      return;
    }
    if(failure(status, "new SimpleDateFormat")) return;
    UnicodeString pattern = "'time' hh:mm"; 
    sdf->applyPattern(pattern); 
    logln("pattern: \"" + pattern + "\""); 

    UnicodeString strings [] = {
        (UnicodeString)"time 10:30",
        (UnicodeString) "time 10:x",
        (UnicodeString) "time 10x"
    };

    ParsePosition ppos [] = {
        ParsePosition(10),
        ParsePosition(0),
        ParsePosition(0)
    };

    UDate dates [] = {
        date(70, UCAL_JANUARY, 1, 10, 30),
        -1,
        -1
    };

    /*Object[] DATA = {
        "time 10:30", new ParsePosition(10), new Date(70, Calendar.JANUARY, 1, 10, 30),
        "time 10:x", new ParsePosition(0), null,
        "time 10x", new ParsePosition(0), null,
    };*/
    
    for(int i = 0; i < 3; i++) {
        UnicodeString text = strings[i];
        ParsePosition finish = ppos[i];
        UDate exp = dates[i];
        
        ParsePosition pos(0);
        UDate d = sdf->parse(text, pos);
        logln(" text: \"" + text + "\""); 
        logln(" index: %d", pos.getIndex()); 
        logln((UnicodeString) " result: " + d);
        if(pos.getIndex() != finish.getIndex())
            errln("Fail: Expected pos " + finish.getIndex());
        if (! ((d == 0 && exp == -1) || (d == exp)))
            errln((UnicodeString) "Fail: Expected result " + exp);
    }

    delete sdf;
}

/**
 * @bug 4104522
 * CANNOT REPRODUCE
 * According to the bug report, this test should throw a
 * StringIndexOutOfBoundsException during the second parse.  However,
 * this is not seen.
 */
void DateFormatRegressionTest::Test4104522(void) 
{
    UErrorCode status = U_ZERO_ERROR;
    
    SimpleDateFormat *sdf = new SimpleDateFormat(status);
    if(U_FAILURE(status)) {
      dataerrln("Couldn't create SimpleDateFormat, error %s", u_errorName(status));
      delete sdf;
      return;
    }
    failure(status, "new SimpleDateFormat");
    UnicodeString pattern = "'time' hh:mm";
    sdf->applyPattern(pattern);
    logln("pattern: \"" + pattern + "\"");

    // works correctly
    ParsePosition pp(0);
    UnicodeString text = "time ";
    UDate dt = sdf->parse(text, pp);
    logln(" text: \"" + text + "\"" +
          " date: " + dt);

    // works wrong
    pp.setIndex(0);
    text = "time";
    dt = sdf->parse(text, pp);
    logln(" text: \"" + text + "\"" +
          " date: " + dt);

    delete sdf;
}

/**
 * @bug 4106807
 */
void DateFormatRegressionTest::Test4106807(void) 
{
    UDate dt; 
    DateFormat *df = DateFormat::createDateTimeInstance(); 
    
    UErrorCode status = U_ZERO_ERROR;
    SimpleDateFormat *sdfs [] = {
        new SimpleDateFormat(UnicodeString("yyyyMMddHHmmss"), status),
        new SimpleDateFormat(UnicodeString("yyyyMMddHHmmss'Z'"), status),
        new SimpleDateFormat(UnicodeString("yyyyMMddHHmmss''"), status),
        new SimpleDateFormat(UnicodeString("yyyyMMddHHmmss'a''a'"), status),
        new SimpleDateFormat(UnicodeString("yyyyMMddHHmmss %"), status)
    };
    if(U_FAILURE(status)) {
      dataerrln("Couldn't create SimpleDateFormat, error %s", u_errorName(status));
      delete sdfs[0];
      delete sdfs[1];
      delete sdfs[2];
      delete sdfs[3];
      delete sdfs[4];
      return;
    }

    failure(status, "new SimpleDateFormat");
    
    UnicodeString strings [] = {
        (UnicodeString) "19980211140000",
        (UnicodeString) "19980211140000",
        (UnicodeString) "19980211140000",
        (UnicodeString) "19980211140000a",
        (UnicodeString) "19980211140000 "
    };

    /*Object[] data = {
        new SimpleDateFormat("yyyyMMddHHmmss"),       "19980211140000",
        new SimpleDateFormat("yyyyMMddHHmmss'Z'"),    "19980211140000",
        new SimpleDateFormat("yyyyMMddHHmmss''"),     "19980211140000",
        new SimpleDateFormat("yyyyMMddHHmmss'a''a'"), "19980211140000a",
        new SimpleDateFormat("yyyyMMddHHmmss %"),     "19980211140000 ",
    };*/
    GregorianCalendar *gc = new GregorianCalendar(status);
    failure(status, "new GregorianCalendar");
    TimeZone *timeZone = TimeZone::createDefault(); 

    TimeZone *gmt = timeZone->clone(); 

    gmt->setRawOffset(0); 

    for(int32_t i = 0; i < 5; i++) {
        SimpleDateFormat *format = sdfs[i];
        UnicodeString dateString = strings[i];
        //try {
            format->setTimeZone(*gmt); 
            dt = format->parse(dateString, status);
            // {sfb} some of these parses will fail purposely
            if(U_FAILURE(status))
                break;
            status = U_ZERO_ERROR;
            UnicodeString fmtd;
            FieldPosition pos(FieldPosition::DONT_CARE);
            fmtd = df->format(dt, fmtd, pos);
            logln(fmtd);
            //logln(df->format(dt)); 
            gc->setTime(dt, status); 
            failure(status, "gc->getTime");
            logln(UnicodeString("") + gc->get(UCAL_ZONE_OFFSET, status));
            failure(status, "gc->get");
            UnicodeString s;
            s = format->format(dt, s, pos);
            logln(s); 
        /*}
        catch (ParseException e) { 
            logln("No way Jose"); 
        }*/ 
    } 

    delete timeZone;
    delete df;
    for(int32_t j = 0; j < 5; j++)
        delete sdfs [j];
     delete gc;
    delete gmt;
}

/*
  Synopsis: Chinese time zone CTT is not recogonized correctly.
  Description: Platform Chinese Windows 95 - ** Time zone set to CST ** 
  */
/**
 * @bug 4108407
 */

// {sfb} what to do with this one ?? 
void DateFormatRegressionTest::Test4108407(void) 
{ 
    /*long l = System.currentTimeMillis(); 
    logln("user.timezone = " + System.getProperty("user.timezone", "?"));
    logln("Time Zone :" + 
                       DateFormat.getDateInstance().getTimeZone().getID()); 
    logln("Default format :" + 
                       DateFormat.getDateInstance().format(new Date(l))); 
    logln("Full format :" + 
                       DateFormat.getDateInstance(DateFormat.FULL).format(new 
                                                                          Date(l))); 
    logln("*** Set host TZ to CST ***");
    logln("*** THE RESULTS OF THIS TEST MUST BE VERIFIED MANUALLY ***");*/
} 

/**
 * @bug 4134203
 * SimpleDateFormat won't parse "GMT"
 */
void DateFormatRegressionTest::Test4134203(void) 
{
    UErrorCode status = U_ZERO_ERROR;
    UnicodeString dateFormat = "MM/dd/yy HH:mm:ss zzz";
    SimpleDateFormat *fmt = new SimpleDateFormat(dateFormat, status);
    if (failure(status, "new SimpleDateFormat", TRUE)) return;
    ParsePosition p0(0);
    UDate d = fmt->parse("01/22/92 04:52:00 GMT", p0);
    logln(dateToString(d));
    if(p0 == ParsePosition(0))
        errln("Fail: failed to parse 'GMT'");
    // In the failure case an exception is thrown by parse();
    // if no exception is thrown, the test passes.

    delete fmt;
}

/**
 * @bug 4151631
 * SimpleDateFormat incorrect handling of 2 single quotes in format()
 */
void DateFormatRegressionTest::Test4151631(void) 
{
    UnicodeString pattern = "'TO_DATE('''dd'-'MM'-'yyyy HH:mm:ss''' , ''DD-MM-YYYY HH:MI:SS'')'";
    logln("pattern=" + pattern);
    UErrorCode status = U_ZERO_ERROR;
    SimpleDateFormat *format = new SimpleDateFormat(pattern, Locale::getUS(), status);
    if (failure(status, "new SimpleDateFormat", TRUE)) return;
    UnicodeString result;
    FieldPosition pos(FieldPosition::DONT_CARE);
    result = format->format(date(1998-1900, UCAL_JUNE, 30, 13, 30, 0), result, pos);
    if (result != "TO_DATE('30-06-1998 13:30:00' , 'DD-MM-YYYY HH:MI:SS')") {
        errln("Fail: result=" + result);
    }
    else {
        logln("Pass: result=" + result);
    }

    delete format;
}

/**
 * @bug 4151706
 * 'z' at end of date format throws index exception in SimpleDateFormat
 * CANNOT REPRODUCE THIS BUG ON 1.2FCS
 */
void DateFormatRegressionTest::Test4151706(void) 
{
    UnicodeString dateString("Thursday, 31-Dec-98 23:00:00 GMT");
    UErrorCode status = U_ZERO_ERROR;
    SimpleDateFormat fmt(UnicodeString("EEEE, dd-MMM-yy HH:mm:ss z"), Locale::getUS(), status);
    if (failure(status, "new SimpleDateFormat", TRUE)) return;
    //try {
        UDate d = fmt.parse(dateString, status);
        failure(status, "fmt->parse");
       // {sfb} what about next two lines?
        //if (d.getTime() != Date.UTC(1998-1900, Calendar.DECEMBER, 31, 23, 0, 0))
        //    errln("Incorrect value: " + d);
    /*} catch (Exception e) {
        errln("Fail: " + e);
    }*/
    UnicodeString temp;
    FieldPosition pos(0);
    logln(dateString + " -> " + fmt.format(d, temp, pos));
}

/**
 * @bug 4162071
 * Cannot reproduce this bug under 1.2 FCS -- it may be a convoluted duplicate
 * of some other bug that has been fixed.
 */
void 
DateFormatRegressionTest::Test4162071(void) 
{
    UnicodeString dateString("Thu, 30-Jul-1999 11:51:14 GMT");
    UnicodeString format("EEE', 'dd-MMM-yyyy HH:mm:ss z"); // RFC 822/1123
    UErrorCode status = U_ZERO_ERROR;
    SimpleDateFormat df(format, Locale::getUS(), status);
    if(U_FAILURE(status)) {
        dataerrln("Couldn't create SimpleDateFormat - %s", u_errorName(status));
        return;
    }
    
    //try {
        UDate x = df.parse(dateString, status);
        if(U_SUCCESS(status))
            logln("Parse format \"" + format + "\" ok");
        else
            errln("Parse format \"" + format + "\" failed.");
        UnicodeString temp;
        FieldPosition pos(0);
        logln(dateString + " -> " + df.format(x, temp, pos));
    //} catch (Exception e) {
    //    errln("Parse format \"" + format + "\" failed.");
    //}
}

/**
 * DateFormat shouldn't parse year "-1" as a two-digit year (e.g., "-1" -> 1999).
 */
void DateFormatRegressionTest::Test4182066(void) {
    UErrorCode status = U_ZERO_ERROR;
    SimpleDateFormat fmt("MM/dd/yy", Locale::getUS(), status);
    SimpleDateFormat dispFmt("MMM dd yyyy GG", Locale::getUS(), status);
    if (U_FAILURE(status)) {
        dataerrln("Couldn't create SimpleDateFormat - %s", u_errorName(status));
        return;
    }

    /* We expect 2-digit year formats to put 2-digit years in the right
     * window.  Out of range years, that is, anything less than "00" or
     * greater than "99", are treated as literal years.  So "1/2/3456"
     * becomes 3456 AD.  Likewise, "1/2/-3" becomes -3 AD == 2 BC.
     */
    const char* STRINGS[] = {
        "02/29/00",
        "01/23/01",
        "04/05/-1",
        "01/23/-9",
        "11/12/1314",
        "10/31/1",
        "09/12/+1",
        "09/12/001",
    };
    int32_t STRINGS_COUNT = (int32_t)(sizeof(STRINGS) / sizeof(STRINGS[0]));
    UDate FAIL_DATE = (UDate) 0;
    UDate DATES[] = {
        date(2000-1900, UCAL_FEBRUARY, 29),
        date(2001-1900, UCAL_JANUARY,  23),
        date(  -1-1900, UCAL_APRIL,     5),
        date(  -9-1900, UCAL_JANUARY,  23),
        date(1314-1900, UCAL_NOVEMBER, 12),
        date(   1-1900, UCAL_OCTOBER,  31),
        FAIL_DATE, // "+1" isn't recognized by US NumberFormat
        date(   1-1900, UCAL_SEPTEMBER,12),
    };

    UnicodeString out;
    UBool pass = TRUE;
    for (int32_t i=0; i<STRINGS_COUNT; ++i) {
        UnicodeString str(STRINGS[i]);
        UDate expected = DATES[i];
        status = U_ZERO_ERROR;
        UDate actual = fmt.parse(str, status);
        if (U_FAILURE(status)) {
            actual = FAIL_DATE;
        }
        UnicodeString actStr;
        if (actual == FAIL_DATE) {
            actStr.append("null");
        } else {
            // Yuck: See j25
            ((DateFormat*)&dispFmt)->format(actual, actStr);
        }

        if (expected == actual) {
            out.append(str + " => " + actStr + "\n");
        } else {
            UnicodeString expStr;
            if (expected == FAIL_DATE) {
                expStr.append("null");
            } else {
                // Yuck: See j25
                ((DateFormat*)&dispFmt)->format(expected, expStr);
            }
            out.append("FAIL: " + str + " => " + actStr
                       + ", expected " + expStr + "\n");
            pass = FALSE;
        }
    }
    if (pass) {
        log(out);
    } else {
        err(out);
    }
}

/**
 * j32 {JDK Bug 4210209 4209272}
 * DateFormat cannot parse Feb 29 2000 when setLenient(false)
 */
void
DateFormatRegressionTest::Test4210209(void) {
    UErrorCode status = U_ZERO_ERROR;
    UnicodeString pattern("MMM d, yyyy");
    SimpleDateFormat sfmt(pattern, Locale::getUS(), status);
    SimpleDateFormat sdisp("MMM dd yyyy GG", Locale::getUS(), status);
    DateFormat& fmt = *(DateFormat*)&sfmt; // Yuck: See j25
    DateFormat& disp = *(DateFormat*)&sdisp; // Yuck: See j25
    if (U_FAILURE(status)) {
        dataerrln("Couldn't create SimpleDateFormat - %s", u_errorName(status));
        return;
    }
    Calendar* calx = (Calendar*)fmt.getCalendar(); // cast away const!
    calx->setLenient(FALSE);
    UDate d = date(2000-1900, UCAL_FEBRUARY, 29);
    UnicodeString s, ss;
    fmt.format(d, s);
    logln(disp.format(d, ss.remove()) + " f> " + pattern +
          " => \"" + s + "\"");
    ParsePosition pos(0);
    d = fmt.parse(s, pos);
    logln(UnicodeString("\"") + s + "\" p> " + pattern +
          " => " + disp.format(d, ss.remove()));
    logln(UnicodeString("Parse pos = ") + pos.getIndex() +
          ", error pos = " + pos.getErrorIndex());
    if (pos.getErrorIndex() != -1) {
        errln(UnicodeString("FAIL: Error index should be -1"));
    }

    // The underlying bug is in GregorianCalendar.  If the following lines
    // succeed, the bug is fixed.  If the bug isn't fixed, they will throw
    // an exception.
    GregorianCalendar cal(status);
    if (U_FAILURE(status)) {
        errln("FAIL: Unable to create Calendar");
        return;
    }
    cal.clear();
    cal.setLenient(FALSE);
    cal.set(2000, UCAL_FEBRUARY, 29); // This should work!
    logln(UnicodeString("Attempt to set Calendar to Feb 29 2000: ") +
                        disp.format(cal.getTime(status), ss.remove()));
    if (U_FAILURE(status)) {
        errln("FAIL: Unable to set Calendar to Feb 29 2000");
    }
}

void DateFormatRegressionTest::Test714(void)
{
    //try {
     UDate d(978103543000.);
    DateFormat *fmt = DateFormat::createDateTimeInstance(DateFormat::NONE,
                                                         DateFormat::MEDIUM,
                                                         Locale::getUS());
    if (fmt == NULL) {
        dataerrln("Error calling DateFormat::createDateTimeInstance");
        return;
    }

    UnicodeString s;
        UnicodeString tests = 
          (UnicodeString) "7:25:43 AM" ;
        UErrorCode status = U_ZERO_ERROR;
        fmt->format (d,s);
        if(U_FAILURE(status))
          {
            errln((UnicodeString) "Fail, errmsg " + u_errorName(status));
            return;
          }
        
        if(s != tests)
        {
          errln((UnicodeString) "Fail: " + s + " != " + tests);
        }
        else
        {
          logln("OK: " + s + " == " + tests);
        }

   delete fmt;
}

class Test1684Data {
public:
  int32_t year;
  int32_t month;
  int32_t date;
  int32_t womyear;
  int32_t wommon;
  int32_t wom;
  int32_t dow;
  UnicodeString data;
  UnicodeString normalized;
    
  Test1684Data(int32_t xyear, int32_t xmonth, int32_t xdate,
               int32_t xwomyear, int32_t xwommon, int32_t xwom, int32_t xdow,
               const char *xdata, const char *xnormalized) :
    year(xyear),
    month(xmonth-1),
    date(xdate),
    womyear(xwomyear),
    wommon(xwommon-1),
    wom(xwom),
    dow(xdow),
    data(xdata,""),
    normalized((xnormalized==NULL)?xdata:xnormalized,"")
  { }
};

void DateFormatRegressionTest::Test1684(void)
{
  //      July 2001            August 2001           January 2002    
  // Su Mo Tu We Th Fr Sa  Su Mo Tu We Th Fr Sa  Su Mo Tu We Th Fr Sa
  //  1  2  3  4  5  6  7            1  2  3  4         1  2  3  4  5
  //  8  9 10 11 12 13 14   5  6  7  8  9 10 11   6  7  8  9 10 11 12
  // 15 16 17 18 19 20 21  12 13 14 15 16 17 18  13 14 15 16 17 18 19
  // 22 23 24 25 26 27 28  19 20 21 22 23 24 25  20 21 22 23 24 25 26
  // 29 30 31              26 27 28 29 30 31     27 28 29 30 31      
  Test1684Data *tests[] = {
    new Test1684Data(2001, 8,  6,  2001,8,2,UCAL_MONDAY,    "2001 08 02 Mon", NULL),
    new Test1684Data(2001, 8,  7,  2001,8,2,UCAL_TUESDAY,   "2001 08 02 Tue", NULL),
    new Test1684Data(2001, 8,  5,/*12,*/ 2001,8,2,UCAL_SUNDAY,    "2001 08 02 Sun", NULL),
    new Test1684Data(2001, 8,6, /*7,  30,*/ 2001,7,6,UCAL_MONDAY,    "2001 07 06 Mon", "2001 08 02 Mon"),
    new Test1684Data(2001, 8,7, /*7,  31,*/ 2001,7,6,UCAL_TUESDAY,   "2001 07 06 Tue", "2001 08 02 Tue"),
    new Test1684Data(2001, 8,  5,  2001,7,6,UCAL_SUNDAY,    "2001 07 06 Sun", "2001 08 02 Sun"),
    new Test1684Data(2001, 7,  30, 2001,8,1,UCAL_MONDAY,    "2001 08 01 Mon", "2001 07 05 Mon"),
    new Test1684Data(2001, 7,  31, 2001,8,1,UCAL_TUESDAY,   "2001 08 01 Tue", "2001 07 05 Tue"),
    new Test1684Data(2001, 7,29, /*8,  5,*/  2001,8,1,UCAL_SUNDAY,    "2001 08 01 Sun", "2001 07 05 Sun"),
    new Test1684Data(2001, 12, 31, 2001,12,6,UCAL_MONDAY,   "2001 12 06 Mon", NULL),
    new Test1684Data(2002, 1,  1,  2002,1,1,UCAL_TUESDAY,   "2002 01 01 Tue", NULL),
    new Test1684Data(2002, 1,  2,  2002,1,1,UCAL_WEDNESDAY, "2002 01 01 Wed", NULL),
    new Test1684Data(2002, 1,  3,  2002,1,1,UCAL_THURSDAY,  "2002 01 01 Thu", NULL),
    new Test1684Data(2002, 1,  4,  2002,1,1,UCAL_FRIDAY,    "2002 01 01 Fri", NULL),
    new Test1684Data(2002, 1,  5,  2002,1,1,UCAL_SATURDAY,  "2002 01 01 Sat", NULL),
    new Test1684Data(2001,12,30, /*2002, 1,  6,*/  2002,1,1,UCAL_SUNDAY,    "2002 01 01 Sun", "2001 12 06 Sun")
  };

#define kTest1684Count  ((int32_t)(sizeof(tests)/sizeof(tests[0])))

  int32_t pass = 0, error = 0, warning = 0;
  int32_t i;

  UErrorCode status = U_ZERO_ERROR;
  UnicodeString pattern("yyyy MM WW EEE","");
  Calendar *cal = new GregorianCalendar(status);
  SimpleDateFormat *sdf = new SimpleDateFormat(pattern,status);
  if (U_FAILURE(status)) {
    dataerrln("Error constructing SimpleDateFormat");
    for(i=0;i<kTest1684Count;i++) {
        delete tests[i];
    }
    delete cal;
    delete sdf;
    return;
  }
  cal->setFirstDayOfWeek(UCAL_SUNDAY);
  cal->setMinimalDaysInFirstWeek(1);

  sdf->adoptCalendar(cal);

  cal = sdf->getCalendar()->clone(); // sdf may have deleted calendar

  if(!cal || !sdf || U_FAILURE(status)) {
    errln(UnicodeString("Error setting up test: ") + u_errorName(status));
  }

  for (i = 0; i < kTest1684Count; ++i) {
    Test1684Data &test = *(tests[i]);
    logln(UnicodeString("#") + i + UnicodeString("\n-----\nTesting round trip of ") + test.year +
        " " + (test.month + 1) +
        " " + test.date +
          " (written as) " + test.data);
    
    cal->clear();
    cal->set(test.year, test.month, test.date);
    UDate ms = cal->getTime(status);
    
    cal->clear();
    cal->set(UCAL_YEAR, test.womyear);
    cal->set(UCAL_MONTH, test.wommon);
    cal->set(UCAL_WEEK_OF_MONTH, test.wom);
    cal->set(UCAL_DAY_OF_WEEK, test.dow);
    UDate ms2 = cal->getTime(status);
            
    if (ms2 != ms) {
      errln((UnicodeString)"\nError: GregorianUCAL_DOM gave " + ms +
            "\n       GregorianUCAL_WOM gave " + ms2);
      error++;
    } else {
      pass++;
    }
    
    ms2 = sdf->parse(test.data, status);
    if(U_FAILURE(status)) {
      errln("parse exception: " + UnicodeString(u_errorName(status)));
    }
    
    if (ms2!=ms) {
      errln((UnicodeString)"\nError: GregorianCalendar gave      " + ms +
            "\n       SimpleDateFormat.parse gave " + ms2);
      error++;
    } else {
      pass++;
    }

    UnicodeString result;
    sdf->format(ms, result);
    if (result != test.normalized) {
      errln("\nWarning: format of '" + test.data + "' gave" +
            "\n                   '" + result + "'" +
            "\n          expected '" + test.normalized + "'");
      warning++;
    } else {
      pass++;
    }
            
    UDate ms3;
    ms3 = sdf->parse(result, status);
    if(U_FAILURE(status)) {
      errln("parse exception 2: " + (UnicodeString)u_errorName(status));
    }
    
    if (ms3!=ms) {
      error++;
      errln((UnicodeString)"\nError: Re-parse of '" + result + "' gave time of " +
          "\n        " + ms3 +
          "\n    not " + ms);
    } else {
      pass++;
            }
  }

  UnicodeString info 
    = UnicodeString("Passed: ") + pass + ", Warnings: " + warning + ", Errors: " + error;
  if (error > 0) {
    errln(info);
  } else {
    logln(info);
  }

  for(i=0;i<kTest1684Count;i++) {
    delete tests[i];
  }
  delete cal;
  delete sdf;
}

void DateFormatRegressionTest::Test5554(void)
{
  UErrorCode status = U_ZERO_ERROR;
  UnicodeString pattern("Z","");
  UnicodeString newfoundland("Canada/Newfoundland", "");
  TimeZone *zone = TimeZone::createTimeZone(newfoundland);
  Calendar *cal = new GregorianCalendar(zone, status);
  SimpleDateFormat *sdf = new SimpleDateFormat(pattern,status);
  if (U_FAILURE(status)) {
    dataerrln("Error constructing SimpleDateFormat");
    delete cal;
    delete sdf;
    return;
  }
  cal->set(2007, 1, 14);
  UDate date = cal->getTime(status);
  if (U_FAILURE(status)) {
    errln("Error getting time to format");
    return;
  };
  sdf->adoptCalendar(cal);
  UnicodeString result;
  UnicodeString correct("-0330", "");
  sdf->format(date, result);
  if (result != correct) {
    errln("\nError: Newfoundland Z of Jan 14, 2007 gave '" + result + "', expected '" + correct + "'");
  }
  delete sdf;
}

#endif /* #if !UCONFIG_NO_FORMATTING */

//eof
