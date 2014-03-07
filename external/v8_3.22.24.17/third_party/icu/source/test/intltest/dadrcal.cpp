/********************************************************************
 * COPYRIGHT: 
 * Copyright (c) 1997-2010, International Business Machines Corporation and
 * others. All Rights Reserved.
 ********************************************************************/

/***********************************************************************
 * Modification history
 * Date        Name        Description
 * 07/09/2007  srl         Copied from dadrcoll.cpp
 ***********************************************************************/

#include "unicode/utypes.h"

#if !UCONFIG_NO_FORMATTING

#include "unicode/tstdtmod.h"
#include "tsdate.h"
#include "dadrcal.h"
#include "unicode/calendar.h"
#include "intltest.h"
#include <string.h>
#include "unicode/schriter.h"
#include "unicode/regex.h"
#include "unicode/smpdtfmt.h"
#include "dbgutil.h"

#include <stdio.h>

DataDrivenCalendarTest::DataDrivenCalendarTest() {
    UErrorCode status = U_ZERO_ERROR;
    driver = TestDataModule::getTestDataModule("calendar", *this, status);
}

DataDrivenCalendarTest::~DataDrivenCalendarTest() {
    delete driver;
}

void DataDrivenCalendarTest::runIndexedTest(int32_t index, UBool exec,
        const char* &name, char* /*par */) {
    if (driver != NULL) {
        if (exec) {
            //  logln("Begin ");
        }
        const DataMap *info= NULL;
        UErrorCode status= U_ZERO_ERROR;
        TestData *testData = driver->createTestData(index, status);
        if (U_SUCCESS(status)) {
            name = testData->getName();
            if (testData->getInfo(info, status)) {
                log(info->getString("Description", status));
            }
            if (exec) {
                log(name);
                logln("---");
                logln("");

                processTest(testData);
            }
            delete testData;
        } else {
            name = "";
        }
    } else {
        dataerrln("format/DataDriven*Test data (calendar.res) not initialized!");
        name = "";
    }

}

void DataDrivenCalendarTest::testOps(TestData *testData,
        const DataMap * /*settings*/) {
    UErrorCode status = U_ZERO_ERROR;
    UBool useDate = FALSE; // TODO
    UnicodeString kMILLIS("MILLIS="); // TODO: static
    UDate fromDate = 0; // TODO
    UDate toDate = 0;
    
    const DataMap *currentCase= NULL;
    char toCalLoc[256] = "";

    // TODO: static strings?
    const UnicodeString kADD("add", "");
    const UnicodeString kROLL("roll", "");

    // Get 'from' time 
    CalendarFieldsSet fromSet, toSet, paramsSet, diffSet;
    SimpleDateFormat fmt(UnicodeString("EEE MMM dd yyyy / YYYY'-W'ww-ee"),
            status);
    if (U_FAILURE(status)) {
        dataerrln("FAIL: Couldn't create SimpleDateFormat: %s",
                u_errorName(status));
        return;
    }
    // Start the processing
    int n = 0;
    while (testData->nextCase(currentCase, status)) {
        ++n;
        Calendar *toCalendar= NULL;
        Calendar *fromCalendar= NULL;

        // load parameters
        char theCase[200];
        sprintf(theCase, "[case %d]", n);
        UnicodeString caseString(theCase, "");
        // build to calendar
        //             Headers { "locale","from","operation","params","to" }
        // #1 locale
        const char *param = "locale";
        UnicodeString locale;
        UnicodeString testSetting = currentCase->getString(param, status);
        if (U_FAILURE(status)) {
            errln(caseString+": Unable to get param '"+param+"' "
                    + UnicodeString(" - "));
            continue;
        }
        testSetting.extract(0, testSetting.length(), toCalLoc, (const char*)0);
        fromCalendar = Calendar::createInstance(toCalLoc, status);
        if (U_FAILURE(status)) {
            errln(caseString+": Unable to instantiate calendar for "
                    +testSetting);
            continue;
        }

        fromSet.clear();
        // #2 'from' info
        param = "from";
        UnicodeString from = testSetting=currentCase->getString(param, status);
        if (U_FAILURE(status)) {
            errln(caseString+": Unable to get parameter '"+param+"' "
                    + UnicodeString(" - "));
            continue;
        }
                
        if(from.startsWith(kMILLIS)){
        	UnicodeString millis = UnicodeString(from, kMILLIS.length());
        	useDate = TRUE;
        	fromDate = udbg_stod(millis);
        } else if(fromSet.parseFrom(testSetting, status)<0 || U_FAILURE(status)){
        	errln(caseString+": Failed to parse '"+param+"' parameter: "
        	                    +testSetting);
        	            continue;
        }
        
        // #4 'operation' info
        param = "operation";
        UnicodeString operation = testSetting=currentCase->getString(param,
                status);
        if (U_FAILURE(status)) {
            errln(caseString+": Unable to get parameter '"+param+"' "
                    + UnicodeString(" - "));
            continue;
        }
        if (U_FAILURE(status)) {
            errln(caseString+": Failed to parse '"+param+"' parameter: "
                    +testSetting);
            continue;
        }

        paramsSet.clear();
        // #3 'params' info
        param = "params";
        UnicodeString params = testSetting
                =currentCase->getString(param, status);
        if (U_FAILURE(status)) {
            errln(caseString+": Unable to get parameter '"+param+"' "
                    + UnicodeString(" - "));
            continue;
        }
        paramsSet.parseFrom(testSetting, status); // parse with inheritance.
        if (U_FAILURE(status)) {
            errln(caseString+": Failed to parse '"+param+"' parameter: "
                    +testSetting);
            continue;
        }

        toSet.clear();
        // #4 'to' info
        param = "to";
        UnicodeString to = testSetting=currentCase->getString(param, status);
        if (U_FAILURE(status)) {
            errln(caseString+": Unable to get parameter '"+param+"' "
                    + UnicodeString(" - "));
            continue;
        }
        if(to.startsWith(kMILLIS)){
        	UnicodeString millis = UnicodeString(to, kMILLIS.length());
            useDate = TRUE;
            toDate = udbg_stod(millis);
        } else if(toSet.parseFrom(testSetting, &fromSet, status)<0 || U_FAILURE(status)){
            errln(caseString+": Failed to parse '"+param+"' parameter: "
                   +testSetting);
            continue;
        }
        
        UnicodeString caseContentsString = locale+":  from "+from+": "
                +operation +" [[[ "+params+" ]]]   >>> "+to;
        logln(caseString+": "+caseContentsString);

        // ------
        // now, do it.

        /// prepare calendar
        if(useDate){
        	fromCalendar->setTime(fromDate, status);
        	if (U_FAILURE(status)) {
        	        	            errln(caseString+" FAIL: Failed to set time on Source calendar: "
        	        	                    + u_errorName(status));
        	        	            return;
        	        	        }
        } else {
        	fromSet.setOnCalendar(fromCalendar, status);
        	        if (U_FAILURE(status)) {
        	            errln(caseString+" FAIL: Failed to set on Source calendar: "
        	                    + u_errorName(status));
        	            return;
        	        }
        }
        
        diffSet.clear();
        // Is the calendar sane after being set?
        if (!fromSet.matches(fromCalendar, diffSet, status)) {
            UnicodeString diffs = diffSet.diffFrom(fromSet, status);
            errln((UnicodeString)"FAIL: "+caseString
                    +", SET SOURCE calendar was not set: Differences: "+ diffs
                    +"', status: "+ u_errorName(status));
        } else if (U_FAILURE(status)) {
            errln("FAIL: "+caseString+" SET SOURCE calendar Failed to match: "
                    +u_errorName(status));
        } else {
            logln("PASS: "+caseString+" SET SOURCE calendar match.");
        }
        
        // to calendar - copy of from calendar
        toCalendar = fromCalendar->clone();

        /// perform op
        for (int q=0; q<UCAL_FIELD_COUNT; q++) {
            if (paramsSet.isSet((UCalendarDateFields)q)) {
                if (operation == kROLL) {
                    toCalendar->roll((UCalendarDateFields)q,
                            paramsSet.get((UCalendarDateFields)q), status);
                } else if (operation == kADD) {
                    toCalendar->add((UCalendarDateFields)q,
                            paramsSet.get((UCalendarDateFields)q), status);
                } else {
                    errln(caseString+ " FAIL: unknown operation "+ operation);
                }
                logln(operation + " of "+ paramsSet.get((UCalendarDateFields)q)
                        +" -> "+u_errorName(status));
            }
        }
        if (U_FAILURE(status)) {
            errln(caseString+" FAIL: after "+operation+" of "+params+" -> "
                    +u_errorName(status));
            continue;
        }

        // now - what's the result?
        diffSet.clear();

        if(useDate){
        	if(!(toCalendar->getTime(status)==toDate) || U_FAILURE(status)){
        		errln("FAIL: "+caseString+" Match operation had an error: "
        		                    +u_errorName(status));
        	}else{
        		logln(caseString + " SUCCESS: got=expected="+toDate);
        		logln("PASS: "+caseString+" matched!");
        	}
        } else if (!toSet.matches(toCalendar, diffSet, status)) {
            UnicodeString diffs = diffSet.diffFrom(toSet, status);
            errln((UnicodeString)"FAIL: "+caseString+" - , "+caseContentsString
                    +" Differences: "+ diffs +"', status: "
                    + u_errorName(status));
        }else if (U_FAILURE(status)) {
            errln("FAIL: "+caseString+" Match operation had an error: "
                    +u_errorName(status));
        }else {
            logln("PASS: "+caseString+" matched!");
        }

        delete fromCalendar;
        delete toCalendar;
    }
}

void DataDrivenCalendarTest::testConvert(int32_t n,
        const CalendarFieldsSet &fromSet, Calendar *fromCalendar,
        const CalendarFieldsSet &toSet, Calendar *toCalendar, UBool forward) {
    UErrorCode status = U_ZERO_ERROR;
    UnicodeString thisString = (UnicodeString)"#"+n+" "+(forward ? "forward"
            : "reverse")+" "+fromCalendar->getType()+"->"+toCalendar->getType();

    fromCalendar->clear();

    fromSet.setOnCalendar(fromCalendar, status);
    if (U_FAILURE(status)) {
        errln("FAIL: Failed to set on Source calendar: %s", u_errorName(status));
        return;
    }

    CalendarFieldsSet diffSet;

    diffSet.clear();
    // Is the calendar sane at the first?
    if (!fromSet.matches(fromCalendar, diffSet, status)) {
        UnicodeString diffs = diffSet.diffFrom(fromSet, status);
        errln((UnicodeString)"FAIL: "+thisString
                +", SOURCE calendar was not set: Differences: "+ diffs
                +"', status: "+ u_errorName(status));
    } else if (U_FAILURE(status)) {
        errln("FAIL: "+thisString+" SOURCE calendar Failed to match: "
                +u_errorName(status));
    } else {
        logln("PASS: "+thisString+" SOURCE calendar match.");
    }

    //logln("Set Source calendar: " + from);

    UDate fromTime = fromCalendar->getTime(status);
    if (U_FAILURE(status)) {
        errln("FAIL: Failed to get Source time: %s", u_errorName(status));
        return;
    }

    diffSet.clear();
    // Is the calendar sane after being set?
    if (!fromSet.matches(fromCalendar, diffSet, status)) {
        UnicodeString diffs = diffSet.diffFrom(fromSet, status);
        errln((UnicodeString)"FAIL: "+thisString
                +", SET SOURCE calendar was not set: Differences: "+ diffs
                +"', status: "+ u_errorName(status));
    } else if (U_FAILURE(status)) {
        errln("FAIL: "+thisString+" SET SOURCE calendar Failed to match: "
                +u_errorName(status));
    } else {
        logln("PASS: "+thisString+" SET SOURCE calendar match.");
    }

    toCalendar->clear();
    toCalendar->setTime(fromTime, status);
    if (U_FAILURE(status)) {
        errln("FAIL: Failed to set Target time: %s", u_errorName(status));
        return;
    }

    diffSet.clear();
    if (!toSet.matches(toCalendar, diffSet, status)) {
        UnicodeString diffs = diffSet.diffFrom(toSet, status);
        errln((UnicodeString)"FAIL: "+thisString+", Differences: "+ diffs
                +"', status: "+ u_errorName(status));
        SimpleDateFormat fmt(UnicodeString("EEE MMM dd yyyy G"), status);
        UnicodeString fromString;
        fmt.format(fromTime, fromString);
        logln("Source Time: "+fromString+", Source Calendar: "
                +fromCalendar->getType());
    } else if (U_FAILURE(status)) {
        errln("FAIL: "+thisString+" Failed to match: "+u_errorName(status));
    } else {
        logln("PASS: "+thisString+" match.");
    }
}

void DataDrivenCalendarTest::testConvert(TestData *testData,
        const DataMap *settings, UBool forward) {
    UErrorCode status = U_ZERO_ERROR;
    Calendar *toCalendar= NULL;
    const DataMap *currentCase= NULL;
    char toCalLoc[256] = "";
    char fromCalLoc[256] = "";
    // build to calendar
    UnicodeString testSetting = settings->getString("ToCalendar", status);
    if (U_SUCCESS(status)) {
        testSetting.extract(0, testSetting.length(), toCalLoc, (const char*)0);
        toCalendar = Calendar::createInstance(toCalLoc, status);
        if (U_FAILURE(status)) {
            dataerrln(UnicodeString("Unable to instantiate ToCalendar for ")+testSetting);
            return;
        }
    }

    CalendarFieldsSet fromSet, toSet, diffSet;
    SimpleDateFormat fmt(UnicodeString("EEE MMM dd yyyy / YYYY'-W'ww-ee"),
            status);
    if (U_FAILURE(status)) {
        errcheckln(status, "FAIL: Couldn't create SimpleDateFormat: %s",
                u_errorName(status));
        return;
    }
    // Start the processing
    int n = 0;
    while (testData->nextCase(currentCase, status)) {
        ++n;
        Calendar *fromCalendar= NULL;
        UnicodeString locale = currentCase->getString("locale", status);
        if (U_SUCCESS(status)) {
            locale.extract(0, locale.length(), fromCalLoc, (const char*)0); // default codepage.  Invariant codepage doesn't have '@'!
            fromCalendar = Calendar::createInstance(fromCalLoc, status);
            if (U_FAILURE(status)) {
                errln("Unable to instantiate fromCalendar for "+locale);
                return;
            }
        } else {
            errln("No 'locale' line.");
            continue;
        }

        fromSet.clear();
        toSet.clear();

        UnicodeString from = currentCase->getString("from", status);
        if (U_FAILURE(status)) {
            errln("No 'from' line.");
            continue;
        }
        fromSet.parseFrom(from, status);
        if (U_FAILURE(status)) {
            errln("Failed to parse 'from' parameter: "+from);
            continue;
        }
        UnicodeString to = currentCase->getString("to", status);
        if (U_FAILURE(status)) {
            errln("No 'to' line.");
            continue;
        }
        toSet.parseFrom(to, &fromSet, status);
        if (U_FAILURE(status)) {
            errln("Failed to parse 'to' parameter: "+to);
            continue;
        }

        // now, do it.
        if (forward) {
            logln((UnicodeString)"#"+n+" "+locale+"/"+from+" >>> "+toCalLoc+"/"
                    +to);
            testConvert(n, fromSet, fromCalendar, toSet, toCalendar, forward);
        } else {
            logln((UnicodeString)"#"+n+" "+locale+"/"+from+" <<< "+toCalLoc+"/"
                    +to);
            testConvert(n, toSet, toCalendar, fromSet, fromCalendar, forward);
        }

        delete fromCalendar;
    }
    delete toCalendar;
}

void DataDrivenCalendarTest::processTest(TestData *testData) {
    //Calendar *cal= NULL;
    //const UChar *arguments= NULL;
    //int32_t argLen = 0;
    char testType[256];
    const DataMap *settings= NULL;
    //const UChar *type= NULL;
    UErrorCode status = U_ZERO_ERROR;
    UnicodeString testSetting;
    int n = 0;
    while (testData->nextSettings(settings, status)) {
        status = U_ZERO_ERROR;
        // try to get a locale
        testSetting = settings->getString("Type", status);
        if (U_SUCCESS(status)) {
            if ((++n)>0) {
                logln("---");
            }
            logln(testSetting + "---");
            testSetting.extract(0, testSetting.length(), testType, "");
        } else {
            errln("Unable to extract 'Type'. Skipping..");
            continue;
        }

        if (!strcmp(testType, "convert_fwd")) {
            testConvert(testData, settings, true);
        } else if (!strcmp(testType, "convert_rev")) {
            testConvert(testData, settings, false);
        } else if (!strcmp(testType, "ops")) {
            testOps(testData, settings);
        } else {
            errln("Unknown type: %s", testType);
        }
    }
}

#endif
