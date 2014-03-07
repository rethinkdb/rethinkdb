/********************************************************************
 * COPYRIGHT: 
 * Copyright (c) 2003-2010, International Business Machines Corporation 
 * and others. All Rights Reserved.
 ********************************************************************
 * Calendar Case Test is a type of CalendarTest which compares the 
 * behavior of a calendar to a certain set of 'test cases', involving
 * conversion between julian-day to fields and vice versa.
 ********************************************************************/

#include "calcasts.h"

#if !UCONFIG_NO_FORMATTING
// ======= 'Main' ===========================

#include "hebrwcal.h" // for Eras
#include "indiancal.h"
#include "coptccal.h"
#include "ethpccal.h"
#include "unicode/datefmt.h"

#define CASE(id,test) case id: name = #test; if (exec) { logln(#test "---"); logln((UnicodeString)""); test(); } break


void CalendarCaseTest::runIndexedTest( int32_t index, UBool exec, const char* &name, char* /*par*/ )
{
    if (exec) logln("TestSuite CalendarCaseTest");
    switch (index) {
    CASE(0,IslamicCivil);
    CASE(1,Hebrew);
    CASE(2,Indian);
    CASE(3,Coptic);
    CASE(4,Ethiopic);
    default: name = ""; break;
    }
}

#undef CASE

// ======= Utility functions =================

void CalendarCaseTest::doTestCases(const TestCase *cases, Calendar *cal) {
  static const int32_t  ONE_SECOND = 1000;
  static const int32_t  ONE_MINUTE = 60*ONE_SECOND;
  static const int32_t  ONE_HOUR   = 60*ONE_MINUTE;
  static const double ONE_DAY    = 24*ONE_HOUR;
  static const double JULIAN_EPOCH = -210866760000000.;   // 1/1/4713 BC 12:00
  int32_t i;
  UErrorCode status = U_ZERO_ERROR;
  cal->adoptTimeZone(TimeZone::getGMT()->clone());
  for(i=0;cases[i].era>=0;i++) {
    UDate t = (JULIAN_EPOCH+(ONE_DAY*cases[i].julian));

    logln("Test case %d:  julianday%f -> date %f\n", i, cases[i].julian, t);

    // Millis -> fields
    cal->setTime(t, status);

    logln(calToStr(*cal));

    checkField(cal, UCAL_ERA, cases[i].era, status);
    checkField(cal, UCAL_YEAR, cases[i].year,status);
    checkField(cal, UCAL_MONTH, cases[i].month - 1,status);
    checkField(cal, UCAL_DATE, cases[i].day,status);
    checkField(cal, UCAL_DAY_OF_WEEK, cases[i].dayOfWeek,status);
    checkField(cal, UCAL_HOUR, cases[i].hour,status);
    checkField(cal, UCAL_MINUTE, cases[i].min,status);
    checkField(cal, UCAL_SECOND, cases[i].sec,status);
    
    // Fields -> millis
    cal->clear();
    
    cal->set(UCAL_ERA, cases[i].era);
    cal->set(UCAL_YEAR, cases[i].year);
    cal->set(UCAL_MONTH, cases[i].month - 1);
    cal->set(UCAL_DATE, cases[i].day);
    cal->set(UCAL_DAY_OF_WEEK, cases[i].dayOfWeek);
    cal->set(UCAL_HOUR, cases[i].hour);
    cal->set(UCAL_MINUTE, cases[i].min);
    cal->set(UCAL_SECOND, cases[i].sec);

    UDate t2 = cal->getTime(status);
    
    if(t != t2) {
      errln("Field->millis: Expected %.0f but got %.0f\n", t, t2);
      logln(calToStr(*cal));
    }
  }
}

UBool CalendarCaseTest::checkField(Calendar *cal, UCalendarDateFields field, int32_t value, UErrorCode &status)
{
  if(U_FAILURE(status)) return FALSE;
  int32_t res = cal->get(field, status);
  if(U_FAILURE(status)) {
    errln((UnicodeString)"Checking field " + fieldName(field) + " and got " + u_errorName(status));
    return FALSE;
  }
  if(res != value) {
    errln((UnicodeString)"FAIL: Checking field " + fieldName(field) + " expected " + value + " and got " + res + UnicodeString("\n"));
    return FALSE;
  } else {
    logln((UnicodeString)"Checking field " + fieldName(field) + " == " + value + UnicodeString("\n"));
  }
  return TRUE;
}

// =========== Test Cases =====================
enum { SUN=UCAL_SUNDAY,
       MON=UCAL_MONDAY,
       TUE=UCAL_TUESDAY,
       WED=UCAL_WEDNESDAY, 
       THU=UCAL_THURSDAY,
       FRI=UCAL_FRIDAY,
       SAT=UCAL_SATURDAY};

void CalendarCaseTest::IslamicCivil()
{
    static const TestCase tests[] = {
        //
        // Most of these test cases were taken from the back of
        // "Calendrical Calculations", with some extras added to help
        // debug a few of the problems that cropped up in development.
        //
        // The months in this table are 1-based rather than 0-based,
        // because it's easier to edit that way.
        //                       Islamic
        //          Julian Day  Era  Year  Month Day  WkDay Hour Min Sec
        { 1507231.5,  0, -1245,   12,   9,  SUN,   0,  0,  0},
        { 1660037.5,  0,  -813,    2,  23,  WED,   0,  0,  0},
        { 1746893.5,  0,  -568,    4,   1,  WED,   0,  0,  0},
        { 1770641.5,  0,  -501,    4,   6,  SUN,   0,  0,  0},
        { 1892731.5,  0,  -157,   10,  17,  WED,   0,  0,  0},
        { 1931579.5,  0,   -47,    6,   3,  MON,   0,  0,  0},
        { 1974851.5,  0,    75,    7,  13,  SAT,   0,  0,  0},
        { 2091164.5,  0,   403,   10,   5,  SUN,   0,  0,  0},
        { 2121509.5,  0,   489,    5,  22,  SUN,   0,  0,  0},
        { 2155779.5,  0,   586,    2,   7,  FRI,   0,  0,  0},
        { 2174029.5,  0,   637,    8,   7,  SAT,   0,  0,  0},
        { 2191584.5,  0,   687,    2,  20,  FRI,   0,  0,  0},
        { 2195261.5,  0,   697,    7,   7,  SUN,   0,  0,  0},
        { 2229274.5,  0,   793,    7,   1,  SUN,   0,  0,  0},
        { 2245580.5,  0,   839,    7,   6,  WED,   0,  0,  0},
        { 2266100.5,  0,   897,    6,   1,  SAT,   0,  0,  0},
        { 2288542.5,  0,   960,    9,  30,  SAT,   0,  0,  0},
        { 2290901.5,  0,   967,    5,  27,  SAT,   0,  0,  0},
        { 2323140.5,  0,  1058,    5,  18,  WED,   0,  0,  0},
        { 2334848.5,  0,  1091,    6,   2,  SUN,   0,  0,  0},
        { 2348020.5,  0,  1128,    8,   4,  FRI,   0,  0,  0},
        { 2366978.5,  0,  1182,    2,   3,  SUN,   0,  0,  0},
        { 2385648.5,  0,  1234,   10,  10,  MON,   0,  0,  0},
        { 2392825.5,  0,  1255,    1,  11,  WED,   0,  0,  0},
        { 2416223.5,  0,  1321,    1,  21,  SUN,   0,  0,  0},
        { 2425848.5,  0,  1348,    3,  19,  SUN,   0,  0,  0},
        { 2430266.5,  0,  1360,    9,   8,  MON,   0,  0,  0},
        { 2430833.5,  0,  1362,    4,  13,  MON,   0,  0,  0},
        { 2431004.5,  0,  1362,   10,   7,  THU,   0,  0,  0},
        { 2448698.5,  0,  1412,    9,  13,  TUE,   0,  0,  0},
        { 2450138.5,  0,  1416,   10,   5,  SUN,   0,  0,  0},
        { 2465737.5,  0,  1460,   10,  12,  WED,   0,  0,  0},
        { 2486076.5,  0,  1518,    3,   5,  SUN,   0,  0,  0},
        { -1,-1,-1,-1,-1,-1,-1,-1,-1 }
    };

    UErrorCode status = U_ZERO_ERROR;
    Calendar *c = Calendar::createInstance("ar@calendar=islamic-civil", status);
    if (failure(status, "Calendar::createInstance", TRUE)) return;
    c->setLenient(TRUE);
    doTestCases(tests, c);

    static const UChar expectedUChars[] = {
        0x0627, 0x0644, 0x062e, 0x0645, 0x064a, 0x0633, 0x060c, 0x0020, 0x0662, 0x0662,
        0x0020, 0x0634, 0x0648, 0x0627, 0x0644, 0x060c, 0x0020, 0x0661, 0x0663, 0x0668,
        0x0669, 0
    };
    UnicodeString result;
    DateFormat *fmt = DateFormat::createDateInstance(DateFormat::kFull, Locale("ar_JO@calendar=islamic-civil"));
    if (fmt == NULL) {
        dataerrln("Error calling DateFormat::createDateInstance");
        delete c;
        return;
    }

    fmt->setTimeZone(*TimeZone::getGMT());
    fmt->format((UDate)2486076.5, result);
    if (result != expectedUChars) {
        errln((UnicodeString)"FAIL: DateFormatting failed. Got " + result + " and expected " + UnicodeString(expectedUChars) + UnicodeString("\n"));
        errln("Maybe the resource aliasing isn't working");
    }
    delete fmt;
    delete c;
}

void CalendarCaseTest::Hebrew() {
    static const int32_t TISHRI  = HebrewCalendar::TISHRI;
    //static const int32_t HESHVAN = HebrewCalendar::HESHVAN;
    //static const int32_t KISLEV  = HebrewCalendar::KISLEV;
    //static const int32_t TEVET   = HebrewCalendar::TEVET;
    //static const int32_t SHEVAT  = HebrewCalendar::SHEVAT;
    //static const int32_t ADAR_1  = HebrewCalendar::ADAR_1;
    //static const int32_t ADAR    = HebrewCalendar::ADAR;
    //static const int32_t NISAN   = HebrewCalendar::NISAN;
    //static const int32_t IYAR    = HebrewCalendar::IYAR;
    //static const int32_t SIVAN   = HebrewCalendar::SIVAN;
    //static const int32_t TAMUZ   = HebrewCalendar::TAMUZ;
    static const int32_t AV      = HebrewCalendar::AV;
    static const int32_t ELUL    = HebrewCalendar::ELUL;

    static const TestCase tests[] = {
        //
        // Most of these test cases were taken from the back of
        // "Calendrical Calculations", with some extras added to help
        // debug a few of the problems that cropped up in development.
        //
        // The months in this table are 1-based rather than 0-based,
        // because it's easier to edit that way.
        //
        //         Julian Day  Era  Year  Month Day  WkDay Hour Min Sec
        {1507231.5,  0,  3174,   12,  10,  SUN,   0,  0,  0},
        {1660037.5,  0,  3593,    3,  25,  WED,   0,  0,  0},
        {1746893.5,  0,  3831,    1,   3,  WED,   0,  0,  0},
        {1770641.5,  0,  3896,    1,   9,  SUN,   0,  0,  0},
        {1892731.5,  0,  4230,    4,  18,  WED,   0,  0,  0},
        {1931579.5,  0,  4336,   10,   4,  MON,   0,  0,  0},
        {1974851.5,  0,  4455,    2,  13,  SAT,   0,  0,  0},
        {2091164.5,  0,  4773,    9,   6,  SUN,   0,  0,  0},
        {2121509.5,  0,  4856,    9,  23,  SUN,   0,  0,  0},
        {2155779.5,  0,  4950,    8,   7,  FRI,   0,  0,  0},
        {2174029.5,  0,  5000,    7,   8,  SAT,   0,  0,  0},
        {2191584.5,  0,  5048,    8,  21,  FRI,   0,  0,  0},
        {2195261.5,  0,  5058,    9,   7,  SUN,   0,  0,  0},
        {2229274.5,  0,  5151,   11,   1,  SUN,   0,  0,  0},
        {2245580.5,  0,  5196,    5,   7,  WED,   0,  0,  0},
        {2266100.5,  0,  5252,    8,   3,  SAT,   0,  0,  0},
        {2288542.5,  0,  5314,    1,   1,  SAT,   0,  0,  0},
        {2290901.5,  0,  5320,    6,  27,  SAT,   0,  0,  0},
        {2323140.5,  0,  5408,   10,  20,  WED,   0,  0,  0},
        {2334551.5,  0,  5440,    1,   1,  THU,   0,  0,  0},
        {2334581.5,  0,  5440,    2,   1,  SAT,   0,  0,  0},
        {2334610.5,  0,  5440,    3,   1,  SUN,   0,  0,  0},
        {2334639.5,  0,  5440,    4,   1,  MON,   0,  0,  0},
        {2334668.5,  0,  5440,    5,   1,  TUE,   0,  0,  0},
        {2334698.5,  0,  5440,    6,   1,  THU,   0,  0,  0},
        {2334728.5,  0,  5440,    7,   1,  SAT,   0,  0,  0},
        {2334757.5,  0,  5440,    8,   1,  SUN,   0,  0,  0},
        {2334787.5,  0,  5440,    9,   1,  TUE,   0,  0,  0},
        {2334816.5,  0,  5440,   10,   1,  WED,   0,  0,  0},
        {2334846.5,  0,  5440,   11,   1,  FRI,   0,  0,  0},
        {2334848.5,  0,  5440,   11,   3,  SUN,   0,  0,  0},
        {2334934.5,  0,  5441,    1,   1,  TUE,   0,  0,  0},
        {2348020.5,  0,  5476,   12,   5,  FRI,   0,  0,  0},
        {2366978.5,  0,  5528,   11,   4,  SUN,   0,  0,  0},
        {2385648.5,  0,  5579,   12,  11,  MON,   0,  0,  0},
        {2392825.5,  0,  5599,    8,  12,  WED,   0,  0,  0},
        {2416223.5,  0,  5663,    8,  22,  SUN,   0,  0,  0},
        {2425848.5,  0,  5689,   12,  19,  SUN,   0,  0,  0},
        {2430266.5,  0,  5702,    1,   8,  MON,   0,  0,  0},
        {2430833.5,  0,  5703,    8,  14,  MON,   0,  0,  0},
        {2431004.5,  0,  5704,    1,   8,  THU,   0,  0,  0},
        {2448698.5,  0,  5752,    7,  12,  TUE,   0,  0,  0},
        {2450138.5,  0,  5756,    7,   5,  SUN,   0,  0,  0},
        {2465737.5,  0,  5799,    2,  12,  WED,   0,  0,  0},
        {2486076.5,  0,  5854,   12,   5,  SUN,   0,  0,  0},

        // Test cases taken from a table of 14 "year types" in the Help file
        // of the application "Hebrew Calendar"
        {2456187.5,  0,  5773,    1,   1,  MON,   0,  0,  0},
        {2459111.5,  0,  5781,    1,   1,  SAT,   0,  0,  0},
        {2453647.5,  0,  5766,    1,   1,  TUE,   0,  0,  0},
        {2462035.5,  0,  5789,    1,   1,  THU,   0,  0,  0},
        {2458756.5,  0,  5780,    1,   1,  MON,   0,  0,  0},
        {2460586.5,  0,  5785,    1,   1,  THU,   0,  0,  0},
        {2463864.5,  0,  5794,    1,   1,  SAT,   0,  0,  0},
        {2463481.5,  0,  5793,    1,   1,  MON,   0,  0,  0},
        {2470421.5,  0,  5812,    1,   1,  THU,   0,  0,  0},
        {2460203.5,  0,  5784,    1,   1,  SAT,   0,  0,  0},
        {2459464.5,  0,  5782,    1,   1,  TUE,   0,  0,  0},
        {2467142.5,  0,  5803,    1,   1,  MON,   0,  0,  0},
        {2455448.5,  0,  5771,    1,   1,  THU,   0,  0,  0},
        // Test cases for JB#2327        
        // http://www.fourmilab.com/documents/calendar/
        // http://www.calendarhome.com/converter/
        //      2452465.5, 2002, JULY, 10, 5762, AV, 1,
        //      2452494.5, 2002, AUGUST, 8, 5762, AV, 30,
        //      2452495.5, 2002, AUGUST, 9, 5762, ELUL, 1,
        //      2452523.5, 2002, SEPTEMBER, 6, 5762, ELUL, 29,
        //      2452524.5, 2002, SEPTEMBER, 7, 5763, TISHRI, 1,
        //         Julian Day  Era  Year  Month Day  WkDay Hour Min Sec
        {2452465.5,  0,  5762,    AV+1,  1,  WED,   0,  0,  0},
        {2452494.5,  0,  5762,    AV+1, 30,  THU,   0,  0,  0},
        {2452495.5,  0,  5762,  ELUL+1,  1,  FRI,   0,  0,  0},
        {2452523.5,  0,  5762,  ELUL+1, 29,  FRI,   0,  0,  0},
        {2452524.5,  0,  5763, TISHRI+1,  1,  SAT,   0,  0,  0},
        { -1,-1,-1,-1,-1,-1,-1,-1,-1 }
    };

    UErrorCode status = U_ZERO_ERROR;
    Calendar *c = Calendar::createInstance("he_HE@calendar=hebrew", status);
    if (failure(status, "Calendar::createInstance", TRUE)) return;
    c->setLenient(TRUE);
    doTestCases(tests, c);


    // Additional test cases for bugs found during development
    //           G.YY/MM/DD  Era  Year  Month Day  WkDay Hour Min Sec
    //{1013, 9, 8, 0,  4774,    1,   1,  TUE,   0,  0,  0},
    //{1239, 9, 1, 0,  5000,    1,   1,  THU,   0,  0,  0},
    //{1240, 9,18, 0,  5001,    1,   1,  TUE,   0,  0,  0},


    delete c;
}

void CalendarCaseTest::Indian() {
    // Months in indian calendar are 0-based. Here taking 1-based names:
    static const int32_t CHAITRA    = IndianCalendar::CHAITRA + 1; 
    static const int32_t VAISAKHA   = IndianCalendar::VAISAKHA + 1;
    static const int32_t JYAISTHA   = IndianCalendar::JYAISTHA + 1;
    static const int32_t ASADHA     = IndianCalendar::ASADHA + 1;
    static const int32_t SRAVANA    = IndianCalendar::SRAVANA + 1 ;
    static const int32_t BHADRA     = IndianCalendar::BHADRA + 1 ;
    static const int32_t ASVINA     = IndianCalendar::ASVINA  + 1 ;
    static const int32_t KARTIKA    = IndianCalendar::KARTIKA + 1 ;
    static const int32_t AGRAHAYANA = IndianCalendar::AGRAHAYANA + 1 ;
    static const int32_t PAUSA      = IndianCalendar::PAUSA + 1 ;
    static const int32_t MAGHA      = IndianCalendar::MAGHA + 1 ;
    static const int32_t PHALGUNA   = IndianCalendar::PHALGUNA + 1 ;


    static const TestCase tests[] = {
    // Test dates generated from:
    // http://www.fourmilab.ch/documents/calendar/ 
    
    // A huge list of test cases to make sure that computeTime and computeFields
    // work properly for a wide range of data in the Indian civil calendar.
    //
    // Julian Day   Era Year     Month         Day  WkDay Hour Min Sec
       {1770641.5,  0,    57,    ASVINA,       10,  SUN,   0,  0,  0},
       {1892731.5,  0,   391,    PAUSA,        18,  WED,   0,  0,  0},
       {1931579.5,  0,   498,    VAISAKHA,     30,  MON,   0,  0,  0},
       {1974851.5,  0,   616,    KARTIKA,      19,  SAT,   0,  0,  0},
       {2091164.5,  0,   935,    VAISAKHA,      5,  SUN,   0,  0,  0},
       {2121509.5,  0,  1018,    JYAISTHA,      3,  SUN,   0,  0,  0},
       {2155779.5,  0,  1112,    CHAITRA,       2,  FRI,   0,  0,  0},
       {2174029.5,  0,  1161,    PHALGUNA,     20,  SAT,   0,  0,  0},
       {2191584.5,  0,  1210,    CHAITRA,      13,  FRI,   0,  0,  0},
       {2195261.5,  0,  1220,    VAISAKHA,      7,  SUN,   0,  0,  0},
       {2229274.5,  0,  1313,    JYAISTHA,     22,  SUN,   0,  0,  0},
       {2245580.5,  0,  1357,    MAGHA,        14,  WED,   0,  0,  0},
       {2266100.5,  0,  1414,    CHAITRA,      20,  SAT,   0,  0,  0},
       {2288542.5,  0,  1475,    BHADRA,       28,  SAT,   0,  0,  0},
       {2290901.5,  0,  1481,    PHALGUNA,     15,  SAT,   0,  0,  0},
       {2323140.5,  0,  1570,    JYAISTHA,     20,  WED,   0,  0,  0},
       {2334551.5,  0,  1601,    BHADRA,       16,  THU,   0,  0,  0},
       {2334581.5,  0,  1601,    ASVINA,       15,  SAT,   0,  0,  0},
       {2334610.5,  0,  1601,    KARTIKA,      14,  SUN,   0,  0,  0},
       {2334639.5,  0,  1601,    AGRAHAYANA,   13,  MON,   0,  0,  0},
       {2334668.5,  0,  1601,    PAUSA,        12,  TUE,   0,  0,  0},
       {2334698.5,  0,  1601,    MAGHA,        12,  THU,   0,  0,  0},
       {2334728.5,  0,  1601,    PHALGUNA,     12,  SAT,   0,  0,  0},
       {2334757.5,  0,  1602,    CHAITRA,      11,  SUN,   0,  0,  0},
       {2334787.5,  0,  1602,    VAISAKHA,     10,  TUE,   0,  0,  0},
       {2334816.5,  0,  1602,    JYAISTHA,      8,  WED,   0,  0,  0},
       {2334846.5,  0,  1602,    ASADHA,        7,  FRI,   0,  0,  0},
       {2334848.5,  0,  1602,    ASADHA,        9,  SUN,   0,  0,  0},
       {2348020.5,  0,  1638,    SRAVANA,       2,  FRI,   0,  0,  0},
       {2334934.5,  0,  1602,    ASVINA,        2,  TUE,   0,  0,  0},
       {2366978.5,  0,  1690,    JYAISTHA,     29,  SUN,   0,  0,  0},
       {2385648.5,  0,  1741,    SRAVANA,      11,  MON,   0,  0,  0},
       {2392825.5,  0,  1761,    CHAITRA,       6,  WED,   0,  0,  0},
       {2416223.5,  0,  1825,    CHAITRA,      29,  SUN,   0,  0,  0},
       {2425848.5,  0,  1851,    BHADRA,        3,  SUN,   0,  0,  0},
       {2430266.5,  0,  1863,    ASVINA,        7,  MON,   0,  0,  0},
       {2430833.5,  0,  1865,    CHAITRA,      29,  MON,   0,  0,  0},
       {2431004.5,  0,  1865,    ASVINA,       15,  THU,   0,  0,  0},
       {2448698.5,  0,  1913,    PHALGUNA,     27,  TUE,   0,  0,  0},
       {2450138.5,  0,  1917,    PHALGUNA,      6,  SUN,   0,  0,  0},
       {2465737.5,  0,  1960,    KARTIKA,      19,  WED,   0,  0,  0},
       {2486076.5,  0,  2016,    ASADHA,       27,  SUN,   0,  0,  0},
        { -1,-1,-1,-1,-1,-1,-1,-1,-1 }
    };

    UErrorCode status = U_ZERO_ERROR;
    Calendar *c = Calendar::createInstance("hi_IN@calendar=indian", status);
    if (failure(status, "Calendar::createInstance", TRUE)) return;
    c->setLenient(TRUE);
    doTestCases(tests, c);

    delete c;
}

void CalendarCaseTest::Coptic() {
    static const TestCase tests[] = {
        //      JD Era  Year  Month  Day WkDay  Hour Min Sec
        {2401442.5,  1,  1579,    2,  20,  WED,    0,  0,  0}, // Gregorian: 20/10/1862
        {2402422.5,  1,  1581,   10,  29,  WED,    0,  0,  0}, // Gregorian: 05/07/1865
        {2402630.5,  1,  1582,    5,  22,  MON,    0,  0,  0}, // Gregorian: 29/01/1866
        {2402708.5,  1,  1582,    8,  10,  TUE,    0,  0,  0}, // Gregorian: 17/04/1866
        {2402971.5,  1,  1583,    4,  28,  SAT,    0,  0,  0}, // Gregorian: 05/01/1867
        {2403344.5,  1,  1584,    5,   5,  MON,    0,  0,  0}, // Gregorian: 13/01/1868
        {1721059.5,  0,   285,    5,   7,  SAT,    0,  0,  0}, // Gregorian: 01/01/0000
        {1721425.5,  0,   284,    5,   8,  MON,    0,  0,  0}, // Gregorian: 01/01/0001
        {1824663.5,  0,     2,   13,   6,  WED,    0,  0,  0}, // Gregorian: 29/08/0283
        {1824664.5,  0,     1,    1,   1,  THU,    0,  0,  0}, // Gregorian: 30/08/0283
        {1825029.5,  1,     1,    1,   1,  FRI,    0,  0,  0}, // Gregorian: 29/08/0284
        {1825394.5,  1,     2,    1,   1,  SAT,    0,  0,  0}, // Gregorian: 29/08/0285
        {1825759.5,  1,     3,    1,   1,  SUN,    0,  0,  0}, // Gregorian: 29/08/0286
        {1826125.5,  1,     4,    1,   1,  TUE,    0,  0,  0}, // Gregorian: 30/08/0287
        {1825028.5,  0,     1,   13,   5,  THU,    0,  0,  0}, // Gregorian: 28/08/0284
        {1825393.5,  1,     1,   13,   5,  FRI,    0,  0,  0}, // Gregorian: 28/08/0285
        {1825758.5,  1,     2,   13,   5,  SAT,    0,  0,  0}, // Gregorian: 28/08/0286
        {1826123.5,  1,     3,   13,   5,  SUN,    0,  0,  0}, // Gregorian: 28/08/0287
        {1826124.5,  1,     3,   13,   6,  MON,    0,  0,  0}, // Gregorian: 29/08/0287
          // above is first coptic leap year
        {1826489.5,  1,     4,   13,   5,  TUE,    0,  0,  0}, // Gregorian: 28/08/0288
        {2299158.5,  1,  1299,    2,   6,  WED,    0,  0,  0}, // Gregorian: 13/10/1582
        {2299159.5,  1,  1299,    2,   7,  THU,    0,  0,  0}, // Gregorian: 14/10/1582
        {2299160.5,  1,  1299,    2,   8,  FRI,    0,  0,  0}, // Gregorian: 15/10/1582
        {2299161.5,  1,  1299,    2,   9,  SAT,    0,  0,  0}, // Gregorian: 16/10/1582

        {2415020.5,  1,  1616,    4,  23,  MON,    0,  0,  0}, // Gregorian: 01/01/1900
        {2453371.5,  1,  1721,    4,  23,  SAT,    0,  0,  0}, // Gregorian: 01/01/2005
        {2555528.5,  1,  2000,   13,   5,  FRI,    0,  0,  0}, // Gregorian: 12/09/2284
        {       -1, -1,    -1,   -1,  -1,   -1,   -1, -1, -1}
    };

    UErrorCode status = U_ZERO_ERROR;
    Calendar *c = Calendar::createInstance("cop_EG@calendar=coptic", status);
    if (failure(status, "Calendar::createInstance", TRUE)) return;

    c->setLenient(TRUE);
    doTestCases(tests, c);

    delete c;
}

void CalendarCaseTest::Ethiopic() {
    static TestCase tests[] = {
        //      JD Era  Year  Month  Day WkDay  Hour Min Sec
        {2401442.5,  1,  1855,    2,  20,  WED,    0,  0,  0}, // Gregorian: 29/10/1862
        {2402422.5,  1,  1857,   10,  29,  WED,    0,  0,  0}, // Gregorian: 05/07/1865
        {2402630.5,  1,  1858,    5,  22,  MON,    0,  0,  0}, // Gregorian: 29/01/1866
        {2402708.5,  1,  1858,    8,  10,  TUE,    0,  0,  0}, // Gregorian: 17/04/1866
        {2402971.5,  1,  1859,    4,  28,  SAT,    0,  0,  0}, // Gregorian: 05/01/1867
        {2403344.5,  1,  1860,    5,   5,  MON,    0,  0,  0}, // Gregorian: 13/01/1868
        {1721059.5,  0,  5492,    5,   7,  SAT,    0,  0,  0}, // Gregorian: 01/01/0000
        {1721425.5,  0,  5493,    5,   8,  MON,    0,  0,  0}, // Gregorian: 01/01/0001
        {1723854.5,  0,  5499,   13,   6,  MON,    0,  0,  0}, // Gregorian: 27/08/0007

        {1723855.5,  0,  5500,    1,   1,  TUE,    0,  0,  0}, // Gregorian: 28/08/0007
        {1724220.5,  1,     1,    1,   1,  WED,    0,  0,  0}, // Gregorian: 27/08/0008
        {1724585.5,  1,     2,    1,   1,  THU,    0,  0,  0}, // Gregorian: 27/08/0009
        {1724950.5,  1,     3,    1,   1,  FRI,    0,  0,  0}, // Gregorian: 27/08/0010

        //{1724536.5,  1,     4,    1,   1,  SUN,    0,  0,  0}, // Gregorian: 28/08/0011
        {1725316.5,  1,     4,    1,   1,  SUN,    0,  0,  0}, // Gregorian: 28/08/0011 - dlf
        {1724219.5,  0,  5500,   13,   5,  TUE,    0,  0,  0}, // Gregorian: 26/08/0008
        {1724584.5,  1,     1,   13,   5,  WED,    0,  0,  0}, // Gregorian: 26/08/0009
        {1724949.5,  1,     2,   13,   5,  THU,    0,  0,  0}, // Gregorian: 26/08/0010
        {1725314.5,  1,     3,   13,   5,  FRI,    0,  0,  0}, // Gregorian: 26/08/0011
        {1725315.5,  1,     3,   13,   6,  SAT,    0,  0,  0}, // Gregorian: 27/08/0011 - first ethiopic leap year
        //{1725560.5,  1,     4,   13,   5,  SUN,    0,  0,  0}, // Gregorian: 26/08/0012 - dlf
        {1725680.5,  1,     4,   13,   5,  SUN,    0,  0,  0}, // Gregorian: 26/08/0012
        {2299158.5,  1,  1575,    2,   6,  WED,    0,  0,  0}, // Gregorian: 13/10/1582  
        {2299159.5,  1,  1575,    2,   7,  THU,    0,  0,  0}, // Gregorian: 14/10/1582  Julian 04/10/1582

        {2299160.5,  1,  1575,    2,   8,  FRI,    0,  0,  0}, // Gregorian: 15/10/1582
        {2299161.5,  1,  1575,    2,   9,  SAT,    0,  0,  0}, // Gregorian: 16/10/1582

        {2415020.5,  1,  1892,    4,  23,  MON,    0,  0,  0}, // Gregorian: 01/01/1900
        {2453371.5,  1,  1997,    4,  23,  SAT,    0,  0,  0}, // Gregorian: 01/01/2005
        {2454719.5,  1,  2000,   13,   5,  WED,    0,  0,  0}, // Gregorian: 10/09/2008
        {       -1, -1,    -1,   -1,  -1,   -1,   -1, -1, -1}
    };

    UErrorCode status = U_ZERO_ERROR;
    Calendar *c = Calendar::createInstance("am_ET@calendar=ethiopic", status);
    if (failure(status, "Calendar::createInstance", TRUE)) return;
    c->setLenient(TRUE);
    doTestCases(tests, c);

    delete c;

    // Testing Amete Alem mode
    int32_t i;
    TestCase *tcase = tests;
    for (i = 0; tcase[i].era >= 0; i++) {
        if (tcase[i].era == 1) {
            tcase[i].era = 0; // Change to Amete Alem era
            tcase[i].year += 5500; // Amete Mihret 1 = Amete Alem 5501
        }
    }
    c = Calendar::createInstance("am_ET@calendar=ethiopic-amete-alem", status);
    if (failure(status, "Calendar::createInstance", TRUE)) return;
    c->setLenient(TRUE);
    doTestCases(tests, c);

    delete c;
}


#endif
