/********************************************************************
 * Copyright (c) 1997-2009, International Business Machines
 * Corporation and others. All Rights Reserved.
 ********************************************************************/

#include "unicode/utypes.h"

#if !UCONFIG_NO_FORMATTING

#include "tsdtfmsy.h"

#include "unicode/dtfmtsym.h"


//--------------------------------------------------------------------
// Time bomb - allows temporary behavior that expires at a given
//             release
//--------------------------------------------------------------------

void IntlTestDateFormatSymbols::runIndexedTest( int32_t index, UBool exec, const char* &name, char* /*par*/ )
{
    if (exec) logln("TestSuite DateFormatSymbols");
    switch (index) {
        TESTCASE(0,TestSymbols);
        TESTCASE(1,TestGetMonths);
        TESTCASE(2,TestGetMonths2);
        TESTCASE(3,TestGetWeekdays2);
        TESTCASE(4,TestGetEraNames);
        default: name = ""; break;
    }
}

/**
 * Test getMonths.
 */
void IntlTestDateFormatSymbols::TestGetMonths()
{
    UErrorCode  status = U_ZERO_ERROR;
    int32_t cnt;
    const UnicodeString* month;
    DateFormatSymbols *symbol;

    symbol=new DateFormatSymbols(Locale::getDefault(), status);

    month=symbol->getMonths(cnt);

    logln((UnicodeString)"size = " + cnt);

    for (int32_t i=0; i<cnt; ++i)
    {
        logln(month[i]);
    }

    delete symbol;
}

void IntlTestDateFormatSymbols::TestGetMonths2()
{
    UErrorCode  status = U_ZERO_ERROR;
    DateFormatSymbols *symbol;

    symbol=new DateFormatSymbols(Locale::getDefault(), status);

    DateFormatSymbols::DtContextType context[] = {DateFormatSymbols::STANDALONE, DateFormatSymbols::FORMAT};
    DateFormatSymbols::DtWidthType width[] = {DateFormatSymbols::WIDE, DateFormatSymbols::ABBREVIATED, DateFormatSymbols::NARROW};

    for (int32_t i = 0; i < 2; i++) {
        for (int32_t j = 0; j < 3; j++) {
            int32_t cnt;
            const UnicodeString * month = symbol->getMonths(cnt,context[i],width[j]);

            logln((UnicodeString)"size = " + cnt);

            for (int32_t k = 0; k < cnt; k++) {
                logln(month[k]);
            }
        }
    }
    delete symbol;
}

void IntlTestDateFormatSymbols::TestGetWeekdays2()
{
    UErrorCode  status = U_ZERO_ERROR;
    DateFormatSymbols *symbol;

    symbol=new DateFormatSymbols(Locale::getDefault(), status);

    DateFormatSymbols::DtContextType context[] = {DateFormatSymbols::STANDALONE, DateFormatSymbols::FORMAT};
    DateFormatSymbols::DtWidthType width[] = {DateFormatSymbols::WIDE, DateFormatSymbols::ABBREVIATED, DateFormatSymbols::NARROW};

    for (int32_t i = 0; i < 2; i++) {
        for (int32_t j = 0; j < 3; j++) {
            int32_t cnt;
            const UnicodeString * wd = symbol->getWeekdays(cnt,context[i],width[j]);

            logln((UnicodeString)"size = " + cnt);

            for (int32_t k = 0; k < cnt; k++) {
                logln(wd[k]);
            }
        }
    }
    delete symbol;
}


void IntlTestDateFormatSymbols::TestGetEraNames()
{
    UErrorCode  status = U_ZERO_ERROR;
    int32_t cnt;
    const UnicodeString* name;
    DateFormatSymbols *symbol;

    symbol=new DateFormatSymbols(Locale::getDefault(), status);

    name=symbol->getEraNames(cnt);

    logln((UnicodeString)"size = " + cnt);

    for (int32_t i=0; i<cnt; ++i)
    {
        logln(name[i]);
    }

    delete symbol;
}

/**
 * Test the API of DateFormatSymbols; primarily a simple get/set set.
 */
void IntlTestDateFormatSymbols::TestSymbols(/* char *par */)
{
    UErrorCode status = U_ZERO_ERROR;

    DateFormatSymbols fr(Locale::getFrench(), status);
    if(U_FAILURE(status)) {
        errcheckln(status, "ERROR: Couldn't create French DateFormatSymbols " + (UnicodeString)u_errorName(status));
        return;
    }

    status = U_ZERO_ERROR;
    DateFormatSymbols fr2(Locale::getFrench(), status);
    if(U_FAILURE(status)) {
        errcheckln(status, "ERROR: Couldn't create French DateFormatSymbols " + (UnicodeString)u_errorName(status));
        return;
    }

    status = U_ZERO_ERROR;
    DateFormatSymbols en(Locale::getEnglish(), status);
    if(U_FAILURE(status)) {
        errcheckln(status, "ERROR: Couldn't create English DateFormatSymbols " + (UnicodeString)u_errorName(status));
        return;
    }

    if(en == fr || ! (en != fr) ) {
        errln("ERROR: English DateFormatSymbols equal to French");
    }

    // just do some VERY basic tests to make sure that get/set work

    int32_t count = 0;
    const UnicodeString *eras = en.getEras(count);
    if(count == 0) {
      errln("ERROR: 0 english eras.. exiting..\n");
      return;
    }
    int32_t eraNamesCount = 0;
    const UnicodeString *eraNames = en.getEraNames(eraNamesCount);
    if(eraNamesCount == 0) {
      errln("ERROR: 0 english eraNames\n");
    } else if ( eraNames[0].length() <= eras[0].length() ) {
      // At least for English we know a wide eraName should be longer than an abbrev era
      errln("ERROR: english eraNames[0] not longer than eras[0]\n");
    }
    int32_t narrowErasCount = 0;
    const UnicodeString *narrowEras = en.getNarrowEras(narrowErasCount);
    if(narrowErasCount == 0) {
      errln("ERROR: 0 english narrowEras\n");
    } else if ( narrowEras[0].length() >= eras[0].length() ) {
      // At least for English we know a narrowEra should be shorter than an abbrev era
      errln("ERROR: english narrowEras[0] not shorter than eras[0]\n");
    }

    fr.setEras(eras, count);
    if( *en.getEras(count) != *fr.getEras(count)) {
      errln("ERROR: setEras() failed");
    }

    const UnicodeString *months = en.getMonths(count);
    fr.setMonths(months, count);
    if( *en.getMonths(count) != *fr.getMonths(count)) {
        errln("ERROR: setMonths() failed");
    }

    const UnicodeString *shortMonths = en.getShortMonths(count);
    fr.setShortMonths(shortMonths, count);
    if( *en.getShortMonths(count) != *fr.getShortMonths(count)) {
        errln("ERROR: setShortMonths() failed");
    }

    const UnicodeString *wideMonths = en.getMonths(count,DateFormatSymbols::FORMAT,DateFormatSymbols::WIDE);
    fr2.setMonths(wideMonths, count, DateFormatSymbols::FORMAT,DateFormatSymbols::WIDE);
    if( *en.getMonths(count,DateFormatSymbols::FORMAT,DateFormatSymbols::WIDE) != 
        *fr2.getMonths(count,DateFormatSymbols::FORMAT,DateFormatSymbols::WIDE )) {
        errln("ERROR: setMonths(FORMAT,WIDE) failed");
    }

    const UnicodeString *abbrMonths = en.getMonths(count,DateFormatSymbols::FORMAT,DateFormatSymbols::ABBREVIATED);
    fr2.setMonths(abbrMonths, count, DateFormatSymbols::FORMAT,DateFormatSymbols::ABBREVIATED);
    if( *en.getMonths(count,DateFormatSymbols::FORMAT,DateFormatSymbols::ABBREVIATED) != 
        *fr2.getMonths(count,DateFormatSymbols::FORMAT,DateFormatSymbols::ABBREVIATED )) {
        errln("ERROR: setMonths(FORMAT,ABBREVIATED) failed");
    }

    const UnicodeString *narrowMonths = en.getMonths(count,DateFormatSymbols::FORMAT,DateFormatSymbols::NARROW);
    fr.setMonths(narrowMonths, count, DateFormatSymbols::FORMAT,DateFormatSymbols::NARROW);
    if( *en.getMonths(count,DateFormatSymbols::FORMAT,DateFormatSymbols::NARROW) != 
        *fr.getMonths(count,DateFormatSymbols::FORMAT,DateFormatSymbols::NARROW )) {
        errln("ERROR: setMonths(FORMAT,NARROW) failed");
    }

    const UnicodeString *standaloneWideMonths = en.getMonths(count,DateFormatSymbols::STANDALONE,DateFormatSymbols::WIDE);
    fr.setMonths(standaloneWideMonths, count, DateFormatSymbols::STANDALONE,DateFormatSymbols::WIDE);
    if( *en.getMonths(count,DateFormatSymbols::STANDALONE,DateFormatSymbols::WIDE) != 
        *fr.getMonths(count,DateFormatSymbols::STANDALONE,DateFormatSymbols::WIDE )) {
        errln("ERROR: setMonths(STANDALONE,WIDE) failed");
    }

    const UnicodeString *standaloneShortMonths = en.getMonths(count,DateFormatSymbols::STANDALONE,DateFormatSymbols::ABBREVIATED);
    fr.setMonths(standaloneShortMonths, count, DateFormatSymbols::STANDALONE,DateFormatSymbols::ABBREVIATED);
    if( *en.getMonths(count,DateFormatSymbols::STANDALONE,DateFormatSymbols::ABBREVIATED) != 
        *fr.getMonths(count,DateFormatSymbols::STANDALONE,DateFormatSymbols::ABBREVIATED )) {
        errln("ERROR: setMonths(STANDALONE,ABBREVIATED) failed");
    }

    const UnicodeString *standaloneNarrowMonths = en.getMonths(count,DateFormatSymbols::STANDALONE,DateFormatSymbols::NARROW);
    fr.setMonths(standaloneNarrowMonths, count, DateFormatSymbols::STANDALONE,DateFormatSymbols::NARROW);
    if( *en.getMonths(count,DateFormatSymbols::STANDALONE,DateFormatSymbols::NARROW) != 
        *fr.getMonths(count,DateFormatSymbols::STANDALONE,DateFormatSymbols::NARROW )) {
        errln("ERROR: setMonths(STANDALONE,NARROW) failed");
    }

    const UnicodeString *weekdays = en.getWeekdays(count);
    fr.setWeekdays(weekdays, count);
    if( *en.getWeekdays(count) != *fr.getWeekdays(count)) {
        errln("ERROR: setWeekdays() failed");
    }

    const UnicodeString *shortWeekdays = en.getShortWeekdays(count);
    fr.setShortWeekdays(shortWeekdays, count);
    if( *en.getShortWeekdays(count) != *fr.getShortWeekdays(count)) {
        errln("ERROR: setShortWeekdays() failed");
    }

    const UnicodeString *wideWeekdays = en.getWeekdays(count,DateFormatSymbols::FORMAT,DateFormatSymbols::WIDE);
    fr2.setWeekdays(wideWeekdays, count, DateFormatSymbols::FORMAT,DateFormatSymbols::WIDE);
    if( *en.getWeekdays(count,DateFormatSymbols::FORMAT,DateFormatSymbols::WIDE) != 
        *fr2.getWeekdays(count,DateFormatSymbols::FORMAT,DateFormatSymbols::WIDE )) {
        errln("ERROR: setWeekdays(FORMAT,WIDE) failed");
    }

    const UnicodeString *abbrWeekdays = en.getWeekdays(count,DateFormatSymbols::FORMAT,DateFormatSymbols::ABBREVIATED);
    fr2.setWeekdays(abbrWeekdays, count, DateFormatSymbols::FORMAT,DateFormatSymbols::ABBREVIATED);
    if( *en.getWeekdays(count,DateFormatSymbols::FORMAT,DateFormatSymbols::ABBREVIATED) != 
        *fr2.getWeekdays(count,DateFormatSymbols::FORMAT,DateFormatSymbols::ABBREVIATED )) {
        errln("ERROR: setWeekdays(FORMAT,ABBREVIATED) failed");
    }

    const UnicodeString *narrowWeekdays = en.getWeekdays(count,DateFormatSymbols::FORMAT,DateFormatSymbols::NARROW);
    fr.setWeekdays(narrowWeekdays, count, DateFormatSymbols::FORMAT,DateFormatSymbols::NARROW);
    if( *en.getWeekdays(count,DateFormatSymbols::FORMAT,DateFormatSymbols::NARROW) != 
        *fr.getWeekdays(count,DateFormatSymbols::FORMAT,DateFormatSymbols::NARROW )) {
        errln("ERROR: setWeekdays(FORMAT,NARROW) failed");
    }

    const UnicodeString *standaloneWideWeekdays = en.getWeekdays(count,DateFormatSymbols::STANDALONE,DateFormatSymbols::WIDE);
    fr.setWeekdays(standaloneWideWeekdays, count, DateFormatSymbols::STANDALONE,DateFormatSymbols::WIDE);
    if( *en.getWeekdays(count,DateFormatSymbols::STANDALONE,DateFormatSymbols::WIDE) != 
        *fr.getWeekdays(count,DateFormatSymbols::STANDALONE,DateFormatSymbols::WIDE )) {
        errln("ERROR: setWeekdays(STANDALONE,WIDE) failed");
    }

    const UnicodeString *standaloneShortWeekdays = en.getWeekdays(count,DateFormatSymbols::STANDALONE,DateFormatSymbols::ABBREVIATED);
    fr.setWeekdays(standaloneShortWeekdays, count, DateFormatSymbols::STANDALONE,DateFormatSymbols::ABBREVIATED);
    if( *en.getWeekdays(count,DateFormatSymbols::STANDALONE,DateFormatSymbols::ABBREVIATED) != 
        *fr.getWeekdays(count,DateFormatSymbols::STANDALONE,DateFormatSymbols::ABBREVIATED )) {
        errln("ERROR: setWeekdays(STANDALONE,ABBREVIATED) failed");
    }

    const UnicodeString *standaloneNarrowWeekdays = en.getWeekdays(count,DateFormatSymbols::STANDALONE,DateFormatSymbols::NARROW);
    fr.setWeekdays(standaloneNarrowWeekdays, count, DateFormatSymbols::STANDALONE,DateFormatSymbols::NARROW);
    if( *en.getWeekdays(count,DateFormatSymbols::STANDALONE,DateFormatSymbols::NARROW) != 
        *fr.getWeekdays(count,DateFormatSymbols::STANDALONE,DateFormatSymbols::NARROW )) {
        errln("ERROR: setWeekdays(STANDALONE,NARROW) failed");
    }

    const UnicodeString *wideQuarters = en.getQuarters(count,DateFormatSymbols::FORMAT, DateFormatSymbols::WIDE);
    fr2.setQuarters(wideQuarters, count, DateFormatSymbols::FORMAT, DateFormatSymbols::WIDE);
    if( *en.getQuarters(count,DateFormatSymbols::FORMAT, DateFormatSymbols::WIDE) != 
        *fr2.getQuarters(count,DateFormatSymbols::FORMAT, DateFormatSymbols::WIDE )) {
        errln("ERROR: setQuarters(FORMAT, WIDE) failed");
    }

    const UnicodeString *abbrQuarters = en.getQuarters(count,DateFormatSymbols::FORMAT, DateFormatSymbols::ABBREVIATED);
    fr2.setQuarters(abbrQuarters, count, DateFormatSymbols::FORMAT, DateFormatSymbols::ABBREVIATED);
    if( *en.getQuarters(count,DateFormatSymbols::FORMAT, DateFormatSymbols::ABBREVIATED) != 
        *fr2.getQuarters(count,DateFormatSymbols::FORMAT ,DateFormatSymbols::ABBREVIATED )) {
        errln("ERROR: setQuarters(FORMAT, ABBREVIATED) failed");
    }

    const UnicodeString *standaloneWideQuarters = en.getQuarters(count,DateFormatSymbols::STANDALONE, DateFormatSymbols::WIDE);
    fr.setQuarters(standaloneWideQuarters, count, DateFormatSymbols::STANDALONE, DateFormatSymbols::WIDE);
    if( *en.getQuarters(count,DateFormatSymbols::STANDALONE, DateFormatSymbols::WIDE) != 
        *fr.getQuarters(count,DateFormatSymbols::STANDALONE, DateFormatSymbols::WIDE )) {
        errln("ERROR: setQuarters(STANDALONE, WIDE) failed");
    }

    const UnicodeString *standaloneShortQuarters = en.getQuarters(count,DateFormatSymbols::STANDALONE, DateFormatSymbols::ABBREVIATED);
    fr.setQuarters(standaloneShortQuarters, count, DateFormatSymbols::STANDALONE, DateFormatSymbols::ABBREVIATED);
    if( *en.getQuarters(count,DateFormatSymbols::STANDALONE, DateFormatSymbols::ABBREVIATED) != 
        *fr.getQuarters(count,DateFormatSymbols::STANDALONE, DateFormatSymbols::ABBREVIATED )) {
        errln("ERROR: setQuarters(STANDALONE, ABBREVIATED) failed");
    }

    const UnicodeString *ampms = en.getAmPmStrings(count);
    fr.setAmPmStrings(ampms, count);
    if( *en.getAmPmStrings(count) != *fr.getAmPmStrings(count)) {
        errln("ERROR: setAmPmStrings() failed");
    }

    int32_t rowCount = 0, columnCount = 0;
    const UnicodeString **strings = en.getZoneStrings(rowCount, columnCount);
    fr.setZoneStrings(strings, rowCount, columnCount);
    const UnicodeString **strings1 = fr.getZoneStrings(rowCount, columnCount);
    for(int32_t i = 0; i < rowCount; i++) {
       for(int32_t j = 0; j < columnCount; j++) {
            if( strings[i][j] != strings1[i][j] ) {
                errln("ERROR: setZoneStrings() failed");
            }
        }
    }

    UnicodeString localPattern, pat1, pat2;
    localPattern = en.getLocalPatternChars(localPattern);
    fr.setLocalPatternChars(localPattern);
    if( en.getLocalPatternChars(pat1) != fr.getLocalPatternChars(pat2)) {
        errln("ERROR: setLocalPatternChars() failed");
    }


    status = U_ZERO_ERROR;
    DateFormatSymbols foo(status);
    DateFormatSymbols bar(foo);

    en = fr;

    if(en != fr) {
        errln("ERROR: Assignment operator failed");
    }
    if(foo != bar) {
        errln("ERROR: Copy Constructor failed");
    }
}

#endif /* #if !UCONFIG_NO_FORMATTING */
