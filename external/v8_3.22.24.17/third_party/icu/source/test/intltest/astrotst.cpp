/********************************************************************
 * COPYRIGHT:
 * Copyright (c) 1996-2010, International Business Machines Corporation and
 * others. All Rights Reserved.
 ********************************************************************/

/* Test CalendarAstronomer for C++ */

#include "unicode/utypes.h"
#include "string.h"
#include "unicode/locid.h"

#if !UCONFIG_NO_FORMATTING

#include "astro.h"
#include "astrotst.h"
#include "gregoimp.h" // for Math
#include "unicode/simpletz.h"


static const double DAY_MS = 24.*60.*60.*1000.;

#define CASE(id,test) case id: name = #test; if (exec) { logln(#test "---"); logln((UnicodeString)""); test(); } break

AstroTest::AstroTest(): astro(NULL), gc(NULL) {
}

void AstroTest::runIndexedTest( int32_t index, UBool exec, const char* &name, char* /*par*/ )
{
    if (exec) logln("TestSuite AstroTest");
    switch (index) {
      // CASE(0,FooTest);
      CASE(0,TestSolarLongitude);
      CASE(1,TestLunarPosition);
      CASE(2,TestCoordinates);
      CASE(3,TestCoverage);
      CASE(4,TestSunriseTimes);
      CASE(5,TestBasics);
      CASE(6,TestMoonAge);
    default: name = ""; break;
    }
}

#undef CASE

#define ASSERT_OK(x)   if(U_FAILURE(x)) { dataerrln("%s:%d: %s\n", __FILE__, __LINE__, u_errorName(x)); return; }


void AstroTest::initAstro(UErrorCode &status) {
  if(U_FAILURE(status)) return;

  if((astro != NULL) || (gc != NULL)) {
    dataerrln("Err: initAstro() called twice!");
    closeAstro(status);
    if(U_SUCCESS(status)) {
      status = U_INTERNAL_PROGRAM_ERROR;
    }
  }

  if(U_FAILURE(status)) return;

  astro = new CalendarAstronomer();
  gc = Calendar::createInstance(TimeZone::getGMT()->clone(), status);
}

void AstroTest::closeAstro(UErrorCode &/*status*/) {
  if(astro != NULL) {
    delete astro;
    astro = NULL;
  }
  if(gc != NULL) {
    delete gc;
    gc = NULL;
  }
}

void AstroTest::TestSolarLongitude(void) {
  UErrorCode status = U_ZERO_ERROR;
  initAstro(status);
  ASSERT_OK(status);

  struct {
    int32_t d[5]; double f ;
  } tests[] = {
    { { 1980, 7, 27, 0, 00 },  124.114347 },
    { { 1988, 7, 27, 00, 00 },  124.187732 }
  };

  logln("");
  for (uint32_t i = 0; i < sizeof(tests)/sizeof(tests[0]); i++) {
    gc->clear();
    gc->set(tests[i].d[0], tests[i].d[1]-1, tests[i].d[2], tests[i].d[3], tests[i].d[4]);

    astro->setDate(gc->getTime(status));

    double longitude = astro->getSunLongitude();
    //longitude = 0;
    CalendarAstronomer::Equatorial result;
    astro->getSunPosition(result);
    logln((UnicodeString)"Sun position is " + result.toString() + (UnicodeString)";  " /* + result.toHmsString()*/ + " Sun longitude is " + longitude );
  }
  closeAstro(status);
  ASSERT_OK(status);
}



void AstroTest::TestLunarPosition(void) {
  UErrorCode status = U_ZERO_ERROR;
  initAstro(status);
  ASSERT_OK(status);

  static const double tests[][7] = {
    { 1979, 2, 26, 16, 00,  0, 0 }
  };
  logln("");

  for (int32_t i = 0; i < (int32_t)(sizeof(tests)/sizeof(tests[0])); i++) {
    gc->clear();
    gc->set((int32_t)tests[i][0], (int32_t)tests[i][1]-1, (int32_t)tests[i][2], (int32_t)tests[i][3], (int32_t)tests[i][4]);
    astro->setDate(gc->getTime(status));

    const CalendarAstronomer::Equatorial& result = astro->getMoonPosition();
    logln((UnicodeString)"Moon position is " + result.toString() + (UnicodeString)";  " /* + result->toHmsString()*/);
  }

  closeAstro(status);
  ASSERT_OK(status);
}



void AstroTest::TestCoordinates(void) {
  UErrorCode status = U_ZERO_ERROR;
  initAstro(status);
  ASSERT_OK(status);

  CalendarAstronomer::Equatorial result;
  astro->eclipticToEquatorial(result, 139.686111 * CalendarAstronomer::PI / 180.0, 4.875278* CalendarAstronomer::PI / 180.0);
  logln((UnicodeString)"result is " + result.toString() + (UnicodeString)";  " /* + result.toHmsString()*/ );
  closeAstro(status);
  ASSERT_OK(status);
}



void AstroTest::TestCoverage(void) {
  UErrorCode status = U_ZERO_ERROR;
  initAstro(status);
  ASSERT_OK(status);
  GregorianCalendar *cal = new GregorianCalendar(1958, UCAL_AUGUST, 15,status);
  UDate then = cal->getTime(status);
  CalendarAstronomer *myastro = new CalendarAstronomer(then);
  ASSERT_OK(status);

  //Latitude:  34 degrees 05' North
  //Longitude:  118 degrees 22' West
  double laLat = 34 + 5./60, laLong = 360 - (118 + 22./60);
  CalendarAstronomer *myastro2 = new CalendarAstronomer(laLong, laLat);

  double eclLat = laLat * CalendarAstronomer::PI / 360;
  double eclLong = laLong * CalendarAstronomer::PI / 360;

  CalendarAstronomer::Ecliptic ecl(eclLat, eclLong);
  CalendarAstronomer::Equatorial eq;
  CalendarAstronomer::Horizon hor;

  logln("ecliptic: " + ecl.toString());
  CalendarAstronomer *myastro3 = new CalendarAstronomer();
  myastro3->setJulianDay((4713 + 2000) * 365.25);

  CalendarAstronomer *astronomers[] = {
    myastro, myastro2, myastro3, myastro2 // check cache
  };

  for (uint32_t i = 0; i < sizeof(astronomers)/sizeof(astronomers[0]); ++i) {
    CalendarAstronomer *anAstro = astronomers[i];

    //logln("astro: " + astro);
    logln((UnicodeString)"   date: " + anAstro->getTime());
    logln((UnicodeString)"   cent: " + anAstro->getJulianCentury());
    logln((UnicodeString)"   gw sidereal: " + anAstro->getGreenwichSidereal());
    logln((UnicodeString)"   loc sidereal: " + anAstro->getLocalSidereal());
    logln((UnicodeString)"   equ ecl: " + (anAstro->eclipticToEquatorial(eq,ecl)).toString());
    logln((UnicodeString)"   equ long: " + (anAstro->eclipticToEquatorial(eq, eclLong)).toString());
    logln((UnicodeString)"   horiz: " + (anAstro->eclipticToHorizon(hor, eclLong)).toString());
    logln((UnicodeString)"   sunrise: " + (anAstro->getSunRiseSet(TRUE)));
    logln((UnicodeString)"   sunset: " + (anAstro->getSunRiseSet(FALSE)));
    logln((UnicodeString)"   moon phase: " + anAstro->getMoonPhase());
    logln((UnicodeString)"   moonrise: " + (anAstro->getMoonRiseSet(TRUE)));
    logln((UnicodeString)"   moonset: " + (anAstro->getMoonRiseSet(FALSE)));
    logln((UnicodeString)"   prev summer solstice: " + (anAstro->getSunTime(CalendarAstronomer::SUMMER_SOLSTICE(), FALSE)));
    logln((UnicodeString)"   next summer solstice: " + (anAstro->getSunTime(CalendarAstronomer::SUMMER_SOLSTICE(), TRUE)));
    logln((UnicodeString)"   prev full moon: " + (anAstro->getMoonTime(CalendarAstronomer::FULL_MOON(), FALSE)));
    logln((UnicodeString)"   next full moon: " + (anAstro->getMoonTime(CalendarAstronomer::FULL_MOON(), TRUE)));
  }

  delete myastro2;
  delete myastro3;
  delete myastro;
  delete cal;

  closeAstro(status);
  ASSERT_OK(status);
}



void AstroTest::TestSunriseTimes(void) {
  UErrorCode status = U_ZERO_ERROR;
  initAstro(status);
  ASSERT_OK(status);

  //  logln("Sunrise/Sunset times for San Jose, California, USA");
  //  CalendarAstronomer *astro2 = new CalendarAstronomer(-121.55, 37.20);
  //  TimeZone *tz = TimeZone::createTimeZone("America/Los_Angeles");

  // We'll use a table generated by the UNSO website as our reference
  // From: http://aa.usno.navy.mil/
  //-Location: W079 25, N43 40
  //-Rise and Set for the Sun for 2001
  //-Zone:  4h West of Greenwich
  int32_t USNO[] = {
    6,59, 19,45,
    6,57, 19,46,
    6,56, 19,47,
    6,54, 19,48,
    6,52, 19,49,
    6,50, 19,51,
    6,48, 19,52,
    6,47, 19,53,
    6,45, 19,54,
    6,43, 19,55,
    6,42, 19,57,
    6,40, 19,58,
    6,38, 19,59,
    6,36, 20, 0,
    6,35, 20, 1,
    6,33, 20, 3,
    6,31, 20, 4,
    6,30, 20, 5,
    6,28, 20, 6,
    6,27, 20, 7,
    6,25, 20, 8,
    6,23, 20,10,
    6,22, 20,11,
    6,20, 20,12,
    6,19, 20,13,
    6,17, 20,14,
    6,16, 20,16,
    6,14, 20,17,
    6,13, 20,18,
    6,11, 20,19,
  };

  logln("Sunrise/Sunset times for Toronto, Canada");
  // long = 79 25", lat = 43 40"
  CalendarAstronomer *astro3 = new CalendarAstronomer(-(79+25/60), 43+40/60);

  // As of ICU4J 2.8 the ICU4J time zones implement pass-through
  // to the underlying JDK.  Because of variation in the
  // underlying JDKs, we have to use a fixed-offset
  // SimpleTimeZone to get consistent behavior between JDKs.
  // The offset we want is [-18000000, 3600000] (raw, dst).
  // [aliu 10/15/03]

  // TimeZone tz = TimeZone.getTimeZone("America/Montreal");
  TimeZone *tz = new SimpleTimeZone(-18000000 + 3600000, "Montreal(FIXED)");

  GregorianCalendar *cal = new GregorianCalendar(tz->clone(), Locale::getUS(), status);
  GregorianCalendar *cal2 = new GregorianCalendar(tz->clone(), Locale::getUS(), status);
  cal->clear();
  cal->set(UCAL_YEAR, 2001);
  cal->set(UCAL_MONTH, UCAL_APRIL);
  cal->set(UCAL_DAY_OF_MONTH, 1);
  cal->set(UCAL_HOUR_OF_DAY, 12); // must be near local noon for getSunRiseSet to work

  DateFormat *df_t  = DateFormat::createTimeInstance(DateFormat::MEDIUM,Locale::getUS());
  DateFormat *df_d  = DateFormat::createDateInstance(DateFormat::MEDIUM,Locale::getUS());
  DateFormat *df_dt = DateFormat::createDateTimeInstance(DateFormat::MEDIUM, DateFormat::MEDIUM, Locale::getUS());
  if(!df_t || !df_d || !df_dt) {
    dataerrln("couldn't create dateformats.");
    return;
  }
  df_t->adoptTimeZone(tz->clone());
  df_d->adoptTimeZone(tz->clone());
  df_dt->adoptTimeZone(tz->clone());

  for (int32_t i=0; i < 30; i++) {
    logln("setDate\n");
    astro3->setDate(cal->getTime(status));
    logln("getRiseSet(TRUE)\n");
    UDate sunrise = astro3->getSunRiseSet(TRUE);
    logln("getRiseSet(FALSE)\n");
    UDate sunset  = astro3->getSunRiseSet(FALSE);
    logln("end of getRiseSet\n");

    cal2->setTime(cal->getTime(status), status);
    cal2->set(UCAL_SECOND,      0);
    cal2->set(UCAL_MILLISECOND, 0);

    cal2->set(UCAL_HOUR_OF_DAY, USNO[4*i+0]);
    cal2->set(UCAL_MINUTE,      USNO[4*i+1]);
    UDate exprise = cal2->getTime(status);
    cal2->set(UCAL_HOUR_OF_DAY, USNO[4*i+2]);
    cal2->set(UCAL_MINUTE,      USNO[4*i+3]);
    UDate expset = cal2->getTime(status);
    // Compute delta of what we got to the USNO data, in seconds
    int32_t deltarise = (int32_t)uprv_fabs((sunrise - exprise) / 1000);
    int32_t deltaset = (int32_t)uprv_fabs((sunset - expset) / 1000);

    // Allow a deviation of 0..MAX_DEV seconds
    // It would be nice to get down to 60 seconds, but at this
    // point that appears to be impossible without a redo of the
    // algorithm using something more advanced than Duffett-Smith.
    int32_t MAX_DEV = 180;
    UnicodeString s1, s2, s3, s4, s5;
    if (deltarise > MAX_DEV || deltaset > MAX_DEV) {
      if (deltarise > MAX_DEV) {
        errln("FAIL: (rise) " + df_d->format(cal->getTime(status),s1) +
              ", Sunrise: " + df_dt->format(sunrise, s2) +
              " (USNO " + df_t->format(exprise,s3) +
              " d=" + deltarise + "s)");
      } else {
        logln(df_d->format(cal->getTime(status),s1) +
              ", Sunrise: " + df_dt->format(sunrise,s2) +
              " (USNO " + df_t->format(exprise,s3) + ")");
      }
      s1.remove(); s2.remove(); s3.remove(); s4.remove(); s5.remove();
      if (deltaset > MAX_DEV) {
        errln("FAIL: (set)  " + df_d->format(cal->getTime(status),s1) +
              ", Sunset:  " + df_dt->format(sunset,s2) +
              " (USNO " + df_t->format(expset,s3) +
              " d=" + deltaset + "s)");
      } else {
        logln(df_d->format(cal->getTime(status),s1) +
              ", Sunset: " + df_dt->format(sunset,s2) +
              " (USNO " + df_t->format(expset,s3) + ")");
      }
    } else {
      logln(df_d->format(cal->getTime(status),s1) +
            ", Sunrise: " + df_dt->format(sunrise,s2) +
            " (USNO " + df_t->format(exprise,s3) + ")" +
            ", Sunset: " + df_dt->format(sunset,s4) +
            " (USNO " + df_t->format(expset,s5) + ")");
    }
    cal->add(UCAL_DATE, 1, status);
  }

  //        CalendarAstronomer a = new CalendarAstronomer(-(71+5/60), 42+37/60);
  //        cal.clear();
  //        cal.set(cal.YEAR, 1986);
  //        cal.set(cal.MONTH, cal.MARCH);
  //        cal.set(cal.DATE, 10);
  //        cal.set(cal.YEAR, 1988);
  //        cal.set(cal.MONTH, cal.JULY);
  //        cal.set(cal.DATE, 27);
  //        a.setDate(cal.getTime());
  //        long r = a.getSunRiseSet2(true);
  delete astro3;
  delete tz;
  delete cal;
  delete cal2;
  delete df_t;
  delete df_d;
  delete df_dt;
  closeAstro(status);
  ASSERT_OK(status);
}



void AstroTest::TestBasics(void) {
  UErrorCode status = U_ZERO_ERROR;
  initAstro(status);
  if (U_FAILURE(status)) {
    dataerrln("Got error: %s", u_errorName(status));
    return;
  }

  // Check that our JD computation is the same as the book's (p. 88)
  GregorianCalendar *cal3 = new GregorianCalendar(TimeZone::getGMT()->clone(), Locale::getUS(), status);
  DateFormat *d3 = DateFormat::createDateTimeInstance(DateFormat::MEDIUM,DateFormat::MEDIUM,Locale::getUS());
  d3->setTimeZone(*TimeZone::getGMT());
  cal3->clear();
  cal3->set(UCAL_YEAR, 1980);
  cal3->set(UCAL_MONTH, UCAL_JULY);
  cal3->set(UCAL_DATE, 2);
  logln("cal3[a]=%.1lf, d=%d\n", cal3->getTime(status), cal3->get(UCAL_JULIAN_DAY,status));
  {
    UnicodeString s;
    logln(UnicodeString("cal3[a] = ") + d3->format(cal3->getTime(status),s));
  }
  cal3->clear();
  cal3->set(UCAL_YEAR, 1980);
  cal3->set(UCAL_MONTH, UCAL_JULY);
  cal3->set(UCAL_DATE, 27);
  logln("cal3=%.1lf, d=%d\n", cal3->getTime(status), cal3->get(UCAL_JULIAN_DAY,status));

  ASSERT_OK(status);
  {
    UnicodeString s;
    logln(UnicodeString("cal3 = ") + d3->format(cal3->getTime(status),s));
  }
  astro->setTime(cal3->getTime(status));
  double jd = astro->getJulianDay() - 2447891.5;
  double exp = -3444.;
  if (jd == exp) {
    UnicodeString s;
    logln(d3->format(cal3->getTime(status),s) + " => " + jd);
  } else {
    UnicodeString s;
    errln("FAIL: " + d3->format(cal3->getTime(status), s) + " => " + jd +
          ", expected " + exp);
  }

  //        cal3.clear();
  //        cal3.set(cal3.YEAR, 1990);
  //        cal3.set(cal3.MONTH, Calendar.JANUARY);
  //        cal3.set(cal3.DATE, 1);
  //        cal3.add(cal3.DATE, -1);
  //        astro.setDate(cal3.getTime());
  //        astro.foo();

  delete cal3;
  delete d3;
  ASSERT_OK(status);
  closeAstro(status);
  ASSERT_OK(status);

}

void AstroTest::TestMoonAge(void){
	UErrorCode status = U_ZERO_ERROR;
	initAstro(status);
	ASSERT_OK(status);
	
	// more testcases are around the date 05/20/2012
	//ticket#3785  UDate ud0 = 1337557623000.0;
	static const double testcase[][10] = {{2012, 5, 20 , 16 , 48, 59},
	                {2012, 5, 20 , 16 , 47, 34},
	                {2012, 5, 21, 00, 00, 00},
	                {2012, 5, 20, 14, 55, 59},
	                {2012, 5, 21, 7, 40, 40},
	                {2023, 9, 25, 10,00, 00},
	                {2008, 7, 7, 15, 00, 33}, 
	                {1832, 9, 24, 2, 33, 41 },
	                {2016, 1, 31, 23, 59, 59},
	                {2099, 5, 20, 14, 55, 59}
	        };
	// Moon phase angle - Got from http://www.moonsystem.to/checkupe.htm
	static const double angle[] = {356.8493418421329, 356.8386760059673, 0.09625415252237701, 355.9986960782416, 3.5714026601303317, 124.26906744384183, 59.80247650195558,
									357.54163205513123, 268.41779281511094, 4.82340276581624};
	static const double precision = CalendarAstronomer::PI/32;
	for (int32_t i = 0; i < (int32_t)(sizeof(testcase)/sizeof(testcase[0])); i++) {
		gc->clear();
		logln((UnicodeString)"CASE["+i+"]: Year "+(int32_t)testcase[i][0]+" Month "+(int32_t)testcase[i][1]+" Day "+
		                                    (int32_t)testcase[i][2]+" Hour "+(int32_t)testcase[i][3]+" Minutes "+(int32_t)testcase[i][4]+
		                                    " Seconds "+(int32_t)testcase[i][5]);
		gc->set((int32_t)testcase[i][0], (int32_t)testcase[i][1]-1, (int32_t)testcase[i][2], (int32_t)testcase[i][3], (int32_t)testcase[i][4], (int32_t)testcase[i][5]);
		astro->setDate(gc->getTime(status));
		double expectedAge = (angle[i]*CalendarAstronomer::PI)/180;
		double got = astro->getMoonAge();
		//logln(testString);
		if(!(got>expectedAge-precision && got<expectedAge+precision)){
			errln((UnicodeString)"FAIL: expected " + expectedAge +
					" got " + got);
		}else{
			logln((UnicodeString)"PASS: expected " + expectedAge +
					" got " + got);
		}
	}
	closeAstro(status);
	ASSERT_OK(status);
}


// TODO: try finding next new moon after  07/28/1984 16:00 GMT


#endif



