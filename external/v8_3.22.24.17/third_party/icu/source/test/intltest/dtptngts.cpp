/********************************************************************
 * COPYRIGHT: 
 * Copyright (c) 2008-2010, International Business Machines Corporation and
 * others. All Rights Reserved.
 ********************************************************************/

#include "unicode/utypes.h"

#if !UCONFIG_NO_FORMATTING

#include <stdio.h>
#include <stdlib.h>
#include "dtptngts.h" 

#include "unicode/calendar.h"
#include "unicode/smpdtfmt.h"
#include "unicode/dtfmtsym.h"
#include "unicode/dtptngen.h"
#include "loctest.h"


// This is an API test, not a unit test.  It doesn't test very many cases, and doesn't
// try to test the full functionality.  It just calls each function in the class and
// verifies that it works on a basic level.

void IntlTestDateTimePatternGeneratorAPI::runIndexedTest( int32_t index, UBool exec, const char* &name, char* /*par*/ )
{
    if (exec) logln("TestSuite DateTimePatternGeneratorAPI");
    switch (index) {
        TESTCASE(0, testAPI);
        TESTCASE(1, testOptions);
        default: name = ""; break;
    }
}

#define MAX_LOCALE   9  

/**
 * Test various generic API methods of DateTimePatternGenerator for API coverage.
 */
void IntlTestDateTimePatternGeneratorAPI::testAPI(/*char *par*/)
{
    UnicodeString patternData[] = {
        UnicodeString("yM"),        // 00
        UnicodeString("yMMM"),      // 01
        UnicodeString("yMd"),       // 02
        UnicodeString("yMMMd"),     // 03
        UnicodeString("Md"),        // 04
        UnicodeString("MMMd"),      // 05
        UnicodeString("yQQQ"),      // 06
        UnicodeString("hhmm"),      // 07
        UnicodeString("HHmm"),      // 08
        UnicodeString("jjmm"),      // 09
        UnicodeString("mmss"),      // 10
        UnicodeString("yyyyMMMM"),  // 11
        UnicodeString("MMMEd"),     // 12
        UnicodeString("Ed"),        // 13
        UnicodeString(),
     };
     
    const char* testLocale[MAX_LOCALE][4] = {
        {"en", "US", "", ""},                   // 0
        {"en", "US", "", "calendar=japanese"},  // 1
        {"de", "DE", "", ""},                   // 2
        {"fi", "", "", ""},                     // 3
        {"ja", "", "", ""},                     // 4
        {"ja", "", "", "calendar=japanese"},    // 5
        {"zh", "Hans", "CN", ""},               // 6
        {"zh", "TW", "", "calendar=roc"},       // 7
        {"ru", "", "", ""},                     // 8
     };
    
    // For Weds, Jan 13, 1999, 23:58:59
    UnicodeString patternResults[] = {
        // en_US                                              // 0 en_US
        UnicodeString("1/1999"),                              // 00: yM
        UnicodeString("Jan 1999"),                            // 01: yMMM
        UnicodeString("1/13/1999"),                           // 02: yMd
        UnicodeString("Jan 13, 1999"),                        // 03: yMMMd
        UnicodeString("1/13"),                                // 04: Md
        UnicodeString("Jan 13"),                              // 05: MMMd
        UnicodeString("Q1 1999"),                             // 06: yQQQ
        UnicodeString("11:58 PM"),                            // 07: hhmm
        UnicodeString("23:58"),                               // 08: HHmm
        UnicodeString("11:58 PM"),                            // 09: jjmm
        UnicodeString("58:59"),                               // 10: mmss
        UnicodeString("January 1999"),                        // 11: yyyyMMMM
        UnicodeString("Wed, Jan 13"),                         // 12: MMMEd -> EEE, MMM d"
        UnicodeString("13 Wed"),                              // 13: Ed    -> d EEE

        // en_US@calendar=japanese                            // 1 en_US@calendar=japanese
        UnicodeString("1/11 Heisei"),                         //  0: yM
        UnicodeString("Jan 11 Heisei"),                       //  1: yMMM
        UnicodeString("H 11-01-13"),                          //  2: yMd
        UnicodeString("H 11 Jan 13"),                         //  3: yMMMd
        UnicodeString("1/13"),                                //  4: Md
        UnicodeString("Jan 13"),                              //  5: MMMd
        UnicodeString("Q1 11 Heisei"),                        //  6: yQQQ
        UnicodeString("11:58 PM"),                            //  7: hhmm
        UnicodeString("23:58"),                               //  8: HHmm
        UnicodeString("11:58 PM"),                            //  9: jjmm
        UnicodeString("58:59"),                               // 10: mmss
        UnicodeString("January 11 Heisei"),                   // 11: yyyyMMMM
        UnicodeString("Wed, Jan 13"),                         // 12: MMMEd -> EEE, MMM d"
        UnicodeString("13 Wed"),                              // 13: Ed    -> d EEE

        // de_DE                                              // 2 de_DE
        UnicodeString("1.1999"),                              // 00: yM
        UnicodeString("Jan 1999"),                            // 01: yMMM
        UnicodeString("13.1.1999"),                           // 02: yMd
        UnicodeString("13. Jan 1999"),                        // 03: yMMMd
        UnicodeString("13.1."),                               // 04: Md
        UnicodeString("13. Jan"),                             // 05: MMMd
        UnicodeString("Q1 1999"),                             // 06: yQQQ
        UnicodeString("11:58 nachm."),                        // 07: hhmm
        UnicodeString("23:58"),                               // 08: HHmm
        UnicodeString("23:58"),                               // 09: jjmm
        UnicodeString("58:59"),                               // 10: mmss
        UnicodeString("Januar 1999"),                         // 11: yyyyMMMM
        UnicodeString("Mi., 13. Jan"),                        // 12: MMMEd -> EEE, d. MMM
        UnicodeString("Mi., 13."),                             // 13: Ed   -> EEE, d.

        // fi                                                 // 3 fi
        UnicodeString("1.1999"),                              // 00: yM (fixed expected result per ticket:6626:)
        UnicodeString("tammi 1999"),                          // 01: yMMM
        UnicodeString("13.1.1999"),                           // 02: yMd
        UnicodeString("13. tammikuuta 1999"),                 // 03: yMMMd
        UnicodeString("13.1."),                               // 04: Md
        UnicodeString("13. tammikuuta"),                      // 05: MMMd
        UnicodeString("1. nelj. 1999"),                       // 06: yQQQ
        UnicodeString("11.58 ip."),                           // 07: hhmm
        UnicodeString("23.58"),                               // 08: HHmm
        UnicodeString("23.58"),                               // 09: jjmm
        UnicodeString("58.59"),                               // 10: mmss
        UnicodeString("tammikuu 1999"),                       // 11: yyyyMMMM
        UnicodeString("ke 13. tammikuuta"),                   // 12: MMMEd -> EEE d. MMM
        UnicodeString("ke 13."),                              // 13: Ed    -> ccc d.

        // ja                                                             // 4 ja
        UnicodeString("1999/1"),                                          // 00: yM    -> y/M
        CharsToUnicodeString("1999\\u5E741\\u6708"),                      // 01: yMMM  -> y\u5E74M\u6708
        UnicodeString("1999/1/13"),                                       // 02: yMd   -> y/M/d
        CharsToUnicodeString("1999\\u5E741\\u670813\\u65E5"),             // 03: yMMMd -> y\u5E74M\u6708d\u65E5
        UnicodeString("1/13"),                                            // 04: Md    -> M/d
        CharsToUnicodeString("1\\u670813\\u65E5"),                        // 05: MMMd  -> M\u6708d\u65E5
        UnicodeString("1999Q1"),                                          // 06: yQQQ  -> yQQQ
        CharsToUnicodeString("\\u5348\\u5F8C11:58"),                      // 07: hhmm
        UnicodeString("23:58"),                                           // 08: HHmm  -> HH:mm
        UnicodeString("23:58"),                                           // 09: jjmm
        UnicodeString("58:59"),                                           // 10: mmss  -> mm:ss
        CharsToUnicodeString("1999\\u5E741\\u6708"),                      // 11: yyyyMMMM  -> y\u5E74M\u6708
        CharsToUnicodeString("1\\u670813\\u65E5(\\u6C34)"),               // 12: MMMEd -> M\u6708d\u65E5(EEE)
        CharsToUnicodeString("13\\u65E5(\\u6C34)"),                       // 13: Ed    -> d\u65E5(EEE)

        // ja@calendar=japanese                                           // 5 ja@calendar=japanese
        CharsToUnicodeString("\\u5E73\\u621011/1"),                       // 00: yM    -> Gy/m
        CharsToUnicodeString("\\u5E73\\u621011\\u5E741\\u6708"),          // 01: yMMM  -> Gy\u5E74M\u6708
        CharsToUnicodeString("\\u5E73\\u621011/1/13"),                    // 02: yMd   -> Gy/m/d
        CharsToUnicodeString("\\u5E73\\u621011\\u5E741\\u670813\\u65E5"), // 03: yMMMd -> Gy\u5E74M\u6708d\u65E5
        UnicodeString("1/13"),                                            // 04: Md    -> M/d
        CharsToUnicodeString("1\\u670813\\u65E5"),                        // 05: MMMd  -> M\u6708d\u65E5
        CharsToUnicodeString("\\u5E73\\u621011/Q1"),                      // 06: yQQQ  -> Gy/QQQ
        CharsToUnicodeString("\\u5348\\u5F8C11:58"),                      // 07: hhmm  ->
        UnicodeString("23:58"),                                           // 08: HHmm  -> HH:mm          (as for ja)
        UnicodeString("23:58"),                                           // 09: jjmm
        UnicodeString("58:59"),                                           // 10: mmss  -> mm:ss          (as for ja)
        CharsToUnicodeString("\\u5E73\\u621011\\u5E741\\u6708"),          // 11: yyyyMMMM  -> Gyyyy\u5E74M\u6708
        CharsToUnicodeString("1\\u670813\\u65E5(\\u6C34)"),               // 12: MMMEd -> M\u6708d\u65E5(EEE)
        CharsToUnicodeString("13\\u65E5(\\u6C34)"),                       // 13: Ed    -> d\u65E5(EEE)

        // zh_Hans_CN                                                     // 6 zh_Hans_CN
        UnicodeString("1999-1", -1, US_INV),                              // 00: yM
        CharsToUnicodeString("1999\\u5E741\\u6708"),                      // 01: yMMM  -> yyyy\u5E74MMM (fixed expected result per ticket:6626:)
        CharsToUnicodeString("1999\\u5E741\\u670813\\u65E5"),             // 02: yMd
        CharsToUnicodeString("1999\\u5E741\\u670813\\u65E5"),             // 03: yMMMd -> yyyy\u5E74MMMd\u65E5 (fixed expected result per ticket:6626:)
        UnicodeString("1-13"),                                            // 04: Md
        CharsToUnicodeString("1\\u670813\\u65E5"),                        // 05: MMMd  -> MMMd\u65E5 (fixed expected result per ticket:6626:)
        CharsToUnicodeString("1999\\u5E741\\u5B63"),                      // 06: yQQQ
        CharsToUnicodeString("\\u4E0B\\u534811:58"),                      // 07: hhmm
        UnicodeString("23:58"),                                           // 08: HHmm
        CharsToUnicodeString("\\u4E0B\\u534811:58"),                      // 09: jjmm
        UnicodeString("58:59"),                                           // 10: mmss
        CharsToUnicodeString("1999\\u5E741\\u6708"),                      // 11: yyyyMMMM  -> yyyy\u5E74MMM
        CharsToUnicodeString("1\\u670813\\u65E5\\u5468\\u4E09"),          // 12: MMMEd -> MMMd\u65E5EEE
        CharsToUnicodeString("13\\u65E5\\u5468\\u4E09"),                  // 13: Ed    -> d\u65E5EEE

        // zh_TW@calendar=roc                                             // 7 zh_TW@calendar=roc 
        CharsToUnicodeString("\\u6C11\\u570B88/1"),                       // 00: yM    -> Gy/M
        CharsToUnicodeString("\\u6C11\\u570B88\\u5E741\\u6708"),          // 01: yMMM  -> Gy\u5E74M\u6708
        CharsToUnicodeString("\\u6C11\\u570B88/1/13"),                    // 02: yMd   -> Gy/M/d
        CharsToUnicodeString("\\u6C11\\u570B88\\u5E741\\u670813\\u65E5"), // 03: yMMMd -> Gy\u5E74M\u6708d\u65E5
        UnicodeString("1/13"),                                            // 04: Md    -> M/d
        CharsToUnicodeString("1\\u670813\\u65E5"),                        // 05: MMMd  ->M\u6708d\u65E5
        CharsToUnicodeString("\\u6C11\\u570B88 1\\u5B63"),                // 06: yQQQ  -> Gy QQQ
        CharsToUnicodeString("\\u4E0B\\u534811:58"),                      // 07: hhmm  ->
        UnicodeString("23:58"),                                           // 08: HHmm  ->
        CharsToUnicodeString("\\u4E0B\\u534811:58"),                      // 09: jjmm
        UnicodeString("58:59"),                                           // 10: mmss  ->
        CharsToUnicodeString("\\u6C11\\u570B88\\u5E741\\u6708"),          // 11: yyyyMMMM  -> Gy\u5E74M\u670
        CharsToUnicodeString("1\\u670813\\u65E5\\u9031\\u4E09"),          // 12: MMMEd -> M\u6708d\u65E5EEE
        CharsToUnicodeString("13\\u65E5(\\u9031\\u4E09)"),                // 13: Ed    -> d\u65E5(EEE)

        // ru                                                             // 8 ru
        UnicodeString("1.1999"),                                          // 00: yM    -> M.y
        CharsToUnicodeString("\\u044F\\u043D\\u0432. 1999"),              // 01: yMMM  -> LLL y
        UnicodeString("13.1.1999"),                                       // 02: yMd   -> d.M.y
        CharsToUnicodeString("13 \\u044F\\u043D\\u0432. 1999\\u00A0\\u0433."), // 03: yMMMd -> d MMM y
        UnicodeString("13.1"),                                            // 04: Md    -> d.M
        CharsToUnicodeString("13 \\u044F\\u043D\\u0432."),                // 05: MMMd  -> d MMM
        CharsToUnicodeString("1999 1-\\u0439 \\u043A\\u0432."),           // 06: yQQQ  -> y QQQ
        UnicodeString("11:58 PM"),                                        // 07: hhmm  -> hh:mm a
        UnicodeString("23:58"),                                           // 08: HHmm  -> HH:mm
        UnicodeString("23:58"),                                           // 09: jjmm  -> HH:mm
        UnicodeString("58:59"),                                           // 10: mmss  -> mm:ss
        CharsToUnicodeString("\\u042F\\u043D\\u0432\\u0430\\u0440\\u044C 1999"), // 11: yyyyMMMM -> LLLL y
        CharsToUnicodeString("\\u0421\\u0440, 13 \\u044F\\u043D\\u0432."), // 12: MMMEd -> ccc, d MMM
        CharsToUnicodeString("\\u0441\\u0440, 13"),                       // 13: Ed    -> EEE, d

        UnicodeString(),
    };

    UnicodeString patternTests2[] = {
        UnicodeString("yyyyMMMdd"),
        UnicodeString("yyyyqqqq"),
        UnicodeString("yMMMdd"),
        UnicodeString("EyyyyMMMdd"),
        UnicodeString("yyyyMMdd"),
        UnicodeString("yyyyMMM"),
        UnicodeString("yyyyMM"),
        UnicodeString("yyMM"),
        UnicodeString("yMMMMMd"),
        UnicodeString("EEEEEMMMMMd"),
        UnicodeString("MMMd"),
        UnicodeString("MMMdhmm"),
        UnicodeString("EMMMdhmms"),
        UnicodeString("MMdhmm"),
        UnicodeString("EEEEMMMdhmms"),
        UnicodeString("yyyyMMMddhhmmss"),
        UnicodeString("EyyyyMMMddhhmmss"),
        UnicodeString("hmm"),
        UnicodeString("hhmm"),
        UnicodeString("hhmmVVVV"),
        UnicodeString(""),
    };
    UnicodeString patternResults2[] = {
        UnicodeString("Oct 14, 1999"),
        UnicodeString("4th quarter 1999"),
        UnicodeString("Oct 14, 1999"),
        UnicodeString("Thu, Oct 14, 1999"),
        UnicodeString("10/14/1999"),
        UnicodeString("Oct 1999"),
        UnicodeString("10/1999"),
        UnicodeString("10/99"),
        UnicodeString("O 14, 1999"),
        UnicodeString("T, O 14"),
        UnicodeString("Oct 14"),
        UnicodeString("Oct 14 6:58 AM"),
        UnicodeString("Thu, Oct 14 6:58:59 AM"),
        UnicodeString("10/14 6:58 AM"),
        UnicodeString("Thursday, Oct 14 6:58:59 AM"),
        UnicodeString("Oct 14, 1999 6:58:59 AM"),
        UnicodeString("Thu, Oct 14, 1999 6:58:59 AM"),
        UnicodeString("6:58 AM"),
        UnicodeString("6:58 AM"),
        UnicodeString("6:58 AM GMT+00:00"),
        UnicodeString(""),
    };
    
    // results for getSkeletons() and getPatternForSkeleton()
    const UnicodeString testSkeletonsResults[] = { 
        UnicodeString("HH:mm"), 
        UnicodeString("MMMMd"), 
        UnicodeString("MMMMMd"), 
    };
          
    const UnicodeString testBaseSkeletonsResults[] = {        
        UnicodeString("Hm"),  
        UnicodeString("MMMd"), 
        UnicodeString("MMMd"),
    };

    UnicodeString newDecimal(" "); // space
    UnicodeString newAppendItemName("hrs.");
    UnicodeString newAppendItemFormat("{1} {0}");
    UnicodeString newDateTimeFormat("{1} {0}");
    UErrorCode status = U_ZERO_ERROR;
    UnicodeString conflictingPattern;
    UDateTimePatternConflict conflictingStatus;

    // ======= Test CreateInstance with default locale
    logln("Testing DateTimePatternGenerator createInstance from default locale");
    
    DateTimePatternGenerator *instFromDefaultLocale=DateTimePatternGenerator::createInstance(status);
    if (U_FAILURE(status)) {
        dataerrln("ERROR: Could not create DateTimePatternGenerator (default) - exitting");
        return;
    }
    else {
        delete instFromDefaultLocale;
    }

    // ======= Test CreateInstance with given locale    
    logln("Testing DateTimePatternGenerator createInstance from French locale");
    status = U_ZERO_ERROR;
    DateTimePatternGenerator *instFromLocale=DateTimePatternGenerator::createInstance(Locale::getFrench(), status);
    if (U_FAILURE(status)) {
        dataerrln("ERROR: Could not create DateTimePatternGenerator (Locale::getFrench()) - exitting");
        return;
    }

    // ======= Test clone DateTimePatternGenerator    
    logln("Testing DateTimePatternGenerator::clone()");
    status = U_ZERO_ERROR;
    

    UnicodeString decimalSymbol = instFromLocale->getDecimal();
    UnicodeString newDecimalSymbol = UnicodeString("*");
    decimalSymbol = instFromLocale->getDecimal();
    instFromLocale->setDecimal(newDecimalSymbol);
    DateTimePatternGenerator *cloneDTPatternGen=instFromLocale->clone();
    decimalSymbol = cloneDTPatternGen->getDecimal();
    if (decimalSymbol != newDecimalSymbol) {
        errln("ERROR: inconsistency is found in cloned object.");
    }
    if ( !(*cloneDTPatternGen == *instFromLocale) ) {
        errln("ERROR: inconsistency is found in cloned object.");
    }
    
    if ( *cloneDTPatternGen != *instFromLocale ) {
        errln("ERROR: inconsistency is found in cloned object.");
    }
    
    delete instFromLocale;
    delete cloneDTPatternGen;
    
    // ======= Test simple use cases    
    logln("Testing simple use cases");
    status = U_ZERO_ERROR;
    Locale deLocale=Locale::getGermany();
    UDate sampleDate=LocaleTest::date(99, 9, 13, 23, 58, 59);
    DateTimePatternGenerator *gen = DateTimePatternGenerator::createInstance(deLocale, status);
    if (U_FAILURE(status)) {
        dataerrln("ERROR: Could not create DateTimePatternGenerator (Locale::getGermany()) - exitting");
        return;
    }
    UnicodeString findPattern = gen->getBestPattern(UnicodeString("MMMddHmm"), status);
    SimpleDateFormat *format = new SimpleDateFormat(findPattern, deLocale, status);
    if (U_FAILURE(status)) {
        dataerrln("ERROR: Could not create SimpleDateFormat (Locale::getGermany())");
        delete gen;
        return;
    }
    TimeZone *zone = TimeZone::createTimeZone(UnicodeString("ECT"));
    if (zone==NULL) {
        dataerrln("ERROR: Could not create TimeZone ECT");
        delete gen;
        delete format;
        return;
    }
    format->setTimeZone(*zone);
    UnicodeString dateReturned, expectedResult;
    dateReturned.remove();
    dateReturned = format->format(sampleDate, dateReturned, status);
    expectedResult=UnicodeString("14. Okt 08:58", -1, US_INV);
    if ( dateReturned != expectedResult ) {
        errln("ERROR: Simple test in getBestPattern with Locale::getGermany()).");
    }
    // add new pattern
    status = U_ZERO_ERROR;
    conflictingStatus = gen->addPattern(UnicodeString("d'. von' MMMM", -1, US_INV), true, conflictingPattern, status); 
    if (U_FAILURE(status)) {
        errln("ERROR: Could not addPattern - d\'. von\' MMMM");
    }
    status = U_ZERO_ERROR;
    UnicodeString testPattern=gen->getBestPattern(UnicodeString("MMMMdd"), status);
    testPattern=gen->getBestPattern(UnicodeString("MMMddHmm"), status);
    format->applyPattern(gen->getBestPattern(UnicodeString("MMMMdHmm"), status));
    dateReturned.remove();
    dateReturned = format->format(sampleDate, dateReturned, status);
    expectedResult=UnicodeString("14. von Oktober 08:58", -1, US_INV);
    if ( dateReturned != expectedResult ) {
        errln(UnicodeString("ERROR: Simple test addPattern failed!: d\'. von\' MMMM   Got: ") + dateReturned + UnicodeString(" Expected: ") + expectedResult);
    }
    delete format;
    
    // get a pattern and modify it
    format = (SimpleDateFormat *)DateFormat::createDateTimeInstance(DateFormat::kFull, DateFormat::kFull, 
                                                                  deLocale);
    format->setTimeZone(*zone);
    UnicodeString pattern;
    pattern = format->toPattern(pattern);
    dateReturned.remove();
    dateReturned = format->format(sampleDate, dateReturned, status);
    expectedResult=CharsToUnicodeString("Donnerstag, 14. Oktober 1999 08:58:59 Mitteleurop\\u00E4ische Sommerzeit");
    if ( dateReturned != expectedResult ) {
        errln("ERROR: Simple test uses full date format.");
        errln(UnicodeString(" Got: ") + dateReturned + UnicodeString(" Expected: ") + expectedResult);
    }
     
    // modify it to change the zone.  
    UnicodeString newPattern = gen->replaceFieldTypes(pattern, UnicodeString("vvvv"), status);
    format->applyPattern(newPattern);
    dateReturned.remove();
    dateReturned = format->format(sampleDate, dateReturned, status);
    expectedResult=UnicodeString("Donnerstag, 14. Oktober 1999 08:58:59 (Frankreich)");
    if ( dateReturned != expectedResult ) {
        errln("ERROR: Simple test modify the timezone!");
        errln(UnicodeString(" Got: ")+ dateReturned + UnicodeString(" Expected: ") + expectedResult);
    }
    
    // setDeciaml(), getDeciaml()
    gen->setDecimal(newDecimal);
    if (newDecimal != gen->getDecimal()) {
        errln("ERROR: unexpected result from setDecimal() and getDecimal()!.\n");
    }
    
    // setAppenItemName() , getAppendItemName()
    gen->setAppendItemName(UDATPG_HOUR_FIELD, newAppendItemName);
    if (newAppendItemName != gen->getAppendItemName(UDATPG_HOUR_FIELD)) {
        errln("ERROR: unexpected result from setAppendItemName() and getAppendItemName()!.\n");
    }
    
    // setAppenItemFormat() , getAppendItemFormat()
    gen->setAppendItemFormat(UDATPG_HOUR_FIELD, newAppendItemFormat);
    if (newAppendItemFormat != gen->getAppendItemFormat(UDATPG_HOUR_FIELD)) {
        errln("ERROR: unexpected result from setAppendItemFormat() and getAppendItemFormat()!.\n");
    }
    
    // setDateTimeFormat() , getDateTimeFormat()
    gen->setDateTimeFormat(newDateTimeFormat);
    if (newDateTimeFormat != gen->getDateTimeFormat()) {
        errln("ERROR: unexpected result from setDateTimeFormat() and getDateTimeFormat()!.\n");
    }
    
    // ======== Test getSkeleton and getBaseSkeleton
    status = U_ZERO_ERROR;
    pattern = UnicodeString("dd-MMM");
    UnicodeString expectedSkeleton = UnicodeString("MMMdd");
    UnicodeString expectedBaseSkeleton = UnicodeString("MMMd");
    UnicodeString retSkeleton = gen->getSkeleton(pattern, status);
    if(U_FAILURE(status) || retSkeleton != expectedSkeleton ) {
         errln("ERROR: Unexpected result from getSkeleton().\n");
         errln(UnicodeString(" Got: ") + retSkeleton + UnicodeString(" Expected: ") + expectedSkeleton );
    }
    retSkeleton = gen->getBaseSkeleton(pattern, status);
    if(U_FAILURE(status) || retSkeleton !=  expectedBaseSkeleton) {
         errln("ERROR: Unexpected result from getBaseSkeleton().\n");
         errln(UnicodeString(" Got: ") + retSkeleton + UnicodeString(" Expected:")+ expectedBaseSkeleton);
    }

    pattern = UnicodeString("dd/MMMM/yy");
    expectedSkeleton = UnicodeString("yyMMMMdd");
    expectedBaseSkeleton = UnicodeString("yMMMd");
    retSkeleton = gen->getSkeleton(pattern, status);
    if(U_FAILURE(status) || retSkeleton != expectedSkeleton ) {
         errln("ERROR: Unexpected result from getSkeleton().\n");
         errln(UnicodeString(" Got: ") + retSkeleton + UnicodeString(" Expected: ") + expectedSkeleton );
    }
    retSkeleton = gen->getBaseSkeleton(pattern, status);
    if(U_FAILURE(status) || retSkeleton !=  expectedBaseSkeleton) {
         errln("ERROR: Unexpected result from getBaseSkeleton().\n");
         errln(UnicodeString(" Got: ") + retSkeleton + UnicodeString(" Expected:")+ expectedBaseSkeleton);
    }
    delete format;
    delete zone;
    delete gen;
    
    {
        // Trac# 6104
        status = U_ZERO_ERROR;
        pattern = UnicodeString("YYYYMMM");
        UnicodeString expR = CharsToUnicodeString("1999\\u5E741\\u6708"); // fixed expected result per ticket:6626:
        Locale loc("ja");
        UDate testDate1= LocaleTest::date(99, 0, 13, 23, 58, 59);
        DateTimePatternGenerator *patGen=DateTimePatternGenerator::createInstance(loc, status);
        if(U_FAILURE(status)) {
            dataerrln("ERROR: Could not create DateTimePatternGenerator");
            return;
        }
        UnicodeString bPattern = patGen->getBestPattern(pattern, status);
        UnicodeString rDate;
        SimpleDateFormat sdf(bPattern, loc, status);
        rDate.remove();
        rDate = sdf.format(testDate1, rDate);

        logln(UnicodeString(" ja locale with skeleton: YYYYMMM  Best Pattern:") + bPattern);
        logln(UnicodeString("  Formatted date:") + rDate);

        if ( expR!= rDate ) {
            errln(UnicodeString("\nERROR: Test Japanese month hack Got: ") + rDate + 
                  UnicodeString(" Expected: ") + expR );
        }
        
        delete patGen;
    }
    {   // Trac# 6104
        Locale loc("zh");
        UnicodeString expR = CharsToUnicodeString("1999\\u5E741\\u6708"); // fixed expected result per ticket:6626:
        UDate testDate1= LocaleTest::date(99, 0, 13, 23, 58, 59);
        DateTimePatternGenerator *patGen=DateTimePatternGenerator::createInstance(loc, status);
        if(U_FAILURE(status)) {
            dataerrln("ERROR: Could not create DateTimePatternGenerator");
            return;
        }
        UnicodeString bPattern = patGen->getBestPattern(pattern, status);
        UnicodeString rDate;
        SimpleDateFormat sdf(bPattern, loc, status);
        rDate.remove();
        rDate = sdf.format(testDate1, rDate);

        logln(UnicodeString(" zh locale with skeleton: YYYYMMM  Best Pattern:") + bPattern);
        logln(UnicodeString("  Formatted date:") + rDate);
        if ( expR!= rDate ) {
            errln(UnicodeString("\nERROR: Test Chinese month hack Got: ") + rDate + 
                  UnicodeString(" Expected: ") + expR );
        }
        delete patGen;   
    }

    {
         // Trac# 6172 duplicate time pattern
         status = U_ZERO_ERROR;
         pattern = UnicodeString("hmv");
         UnicodeString expR = UnicodeString("h:mm a v"); // avail formats has hm -> "h:mm a" (fixed expected result per ticket:6626:)
         Locale loc("en");
         DateTimePatternGenerator *patGen=DateTimePatternGenerator::createInstance(loc, status);
         if(U_FAILURE(status)) {
             dataerrln("ERROR: Could not create DateTimePatternGenerator");
             return;
         }
         UnicodeString bPattern = patGen->getBestPattern(pattern, status);
         logln(UnicodeString(" en locale with skeleton: hmv  Best Pattern:") + bPattern);

         if ( expR!= bPattern ) {
             errln(UnicodeString("\nERROR: Test EN time format Got: ") + bPattern + 
                   UnicodeString(" Expected: ") + expR );
         }
         
         delete patGen;
     }
     
    
    // ======= Test various skeletons.
    logln("Testing DateTimePatternGenerator with various skeleton");
   
    status = U_ZERO_ERROR;
    int32_t localeIndex=0;
    int32_t resultIndex=0;
    UnicodeString resultDate;
    UDate testDate= LocaleTest::date(99, 0, 13, 23, 58, 59);
    while (localeIndex < MAX_LOCALE )
    {       
        int32_t dataIndex=0;
        UnicodeString bestPattern;
        
        Locale loc(testLocale[localeIndex][0], testLocale[localeIndex][1], testLocale[localeIndex][2], testLocale[localeIndex][3]);
        logln("\n\n Locale: %s_%s_%s@%s", testLocale[localeIndex][0], testLocale[localeIndex][1], testLocale[localeIndex][2], testLocale[localeIndex][3]);
        DateTimePatternGenerator *patGen=DateTimePatternGenerator::createInstance(loc, status);
        if(U_FAILURE(status)) {
            dataerrln("ERROR: Could not create DateTimePatternGenerator with locale index:%d . - exitting\n", localeIndex);
            return;
        }
        while (patternData[dataIndex].length() > 0) {
            log(patternData[dataIndex]);
            bestPattern = patGen->getBestPattern(patternData[dataIndex++], status);
            logln(UnicodeString(" -> ") + bestPattern);
            
            SimpleDateFormat sdf(bestPattern, loc, status);
            resultDate.remove();
            resultDate = sdf.format(testDate, resultDate);
            if ( resultDate != patternResults[resultIndex] ) {
                errln(UnicodeString("\nERROR: Test various skeletons[") + (dataIndex-1) + UnicodeString("], localeIndex ") + localeIndex +
                      UnicodeString(". Got: \"") + resultDate + UnicodeString("\" Expected: \"") + patternResults[resultIndex] + "\"" );
            }
            
            resultIndex++;
        }
        delete patGen;
        localeIndex++;
    }
    
    // ======= More tests ticket#6110
    logln("Testing DateTimePatternGenerator with various skeleton");
   
    status = U_ZERO_ERROR;
    localeIndex=0;
    resultIndex=0;
    testDate= LocaleTest::date(99, 9, 13, 23, 58, 59);
    {       
        int32_t dataIndex=0;
        UnicodeString bestPattern;
        logln("\n\n Test various skeletons for English locale...");
        DateTimePatternGenerator *patGen=DateTimePatternGenerator::createInstance(Locale::getEnglish(), status);
        if(U_FAILURE(status)) {
            dataerrln("ERROR: Could not create DateTimePatternGenerator with locale English . - exitting\n");
            return;
        }
        TimeZone *enZone = TimeZone::createTimeZone(UnicodeString("ECT/GMT"));
        if (enZone==NULL) {
            dataerrln("ERROR: Could not create TimeZone ECT");
            delete patGen;
            return;
        }
        SimpleDateFormat *enFormat = (SimpleDateFormat *)DateFormat::createDateTimeInstance(DateFormat::kFull, 
                         DateFormat::kFull, Locale::getEnglish());
        enFormat->setTimeZone(*enZone);
        while (patternTests2[dataIndex].length() > 0) {
            logln(patternTests2[dataIndex]);
            bestPattern = patGen->getBestPattern(patternTests2[dataIndex], status);
            logln(UnicodeString(" -> ") + bestPattern);
            enFormat->applyPattern(bestPattern);
            resultDate.remove();
            resultDate = enFormat->format(testDate, resultDate);
            if ( resultDate != patternResults2[resultIndex] ) {
                errln(UnicodeString("\nERROR: Test various skeletons[") + dataIndex
                    + UnicodeString("]. Got: ") + resultDate + UnicodeString(" Expected: ") + 
                    patternResults2[resultIndex] );
            }
            dataIndex++;
            resultIndex++;
        }
        delete patGen;
        delete enZone;
        delete enFormat;
    }



    // ======= Test random skeleton 
    DateTimePatternGenerator *randDTGen= DateTimePatternGenerator::createInstance(status);
    if (U_FAILURE(status)) {
        dataerrln("ERROR: Could not create DateTimePatternGenerator (Locale::getFrench()) - exitting");
        return;
    }
    UChar newChar;
    int32_t i;
    for (i=0; i<10; ++i) {
        UnicodeString randomSkeleton;
        int32_t len = rand() % 20;
        for (int32_t j=0; j<len; ++j ) {
            while ((newChar = (UChar)(rand()%0x7f))>=(UChar)0x20) {
                randomSkeleton += newChar;
            }
        }
        UnicodeString bestPattern = randDTGen->getBestPattern(randomSkeleton, status);
    }
    delete randDTGen;
    
    // UnicodeString randomString=Unicode
    // ======= Test getStaticClassID()

    logln("Testing getStaticClassID()");
    status = U_ZERO_ERROR;
    DateTimePatternGenerator *test= DateTimePatternGenerator::createInstance(status);
    
    if(test->getDynamicClassID() != DateTimePatternGenerator::getStaticClassID()) {
        errln("ERROR: getDynamicClassID() didn't return the expected value");
    }
    delete test;
    
    // ====== Test createEmptyInstance()
    
    logln("Testing createEmptyInstance()");
    status = U_ZERO_ERROR;
    
    test = DateTimePatternGenerator::createEmptyInstance(status);
    if(U_FAILURE(status)) {
         errln("ERROR: Fail to create an empty instance ! - exitting.\n");
         delete test;
         return;
    }
    
    conflictingStatus = test->addPattern(UnicodeString("MMMMd"), true, conflictingPattern, status); 
    status = U_ZERO_ERROR;
    testPattern=test->getBestPattern(UnicodeString("MMMMdd"), status);
    conflictingStatus = test->addPattern(UnicodeString("HH:mm"), true, conflictingPattern, status); 
    conflictingStatus = test->addPattern(UnicodeString("MMMMMd"), true, conflictingPattern, status); //duplicate pattern
    StringEnumeration *output=NULL;
    output = test->getRedundants(status);
    expectedResult=UnicodeString("MMMMd");
    if (output != NULL) {
        output->reset(status);
        const UnicodeString *dupPattern=output->snext(status);
        if ( (dupPattern==NULL) || (*dupPattern != expectedResult) ) {
            errln("ERROR: Fail in getRedundants !\n");
        }
    }
    
    // ======== Test getSkeletons and getBaseSkeletons
    StringEnumeration* ptrSkeletonEnum = test->getSkeletons(status);
    if(U_FAILURE(status)) {
        errln("ERROR: Fail to get skeletons !\n");
    }
    UnicodeString returnPattern, *ptrSkeleton;
    ptrSkeletonEnum->reset(status);
    int32_t count=ptrSkeletonEnum->count(status);
    for (i=0; i<count; ++i) {
        ptrSkeleton = (UnicodeString *)ptrSkeletonEnum->snext(status);
        returnPattern = test->getPatternForSkeleton(*ptrSkeleton);
        if ( returnPattern != testSkeletonsResults[i] ) {
            errln(UnicodeString("ERROR: Unexpected result from getSkeletons and getPatternForSkeleton\nGot: ") + returnPattern
               + UnicodeString("\nExpected: ") + testSkeletonsResults[i]
               + UnicodeString("\n"));
        }
    }
    StringEnumeration* ptrBaseSkeletonEnum = test->getBaseSkeletons(status);
    if(U_FAILURE(status)) {
        errln("ERROR: Fail to get base skeletons !\n");
    }   
    count=ptrBaseSkeletonEnum->count(status);
    for (i=0; i<count; ++i) {
        ptrSkeleton = (UnicodeString *)ptrBaseSkeletonEnum->snext(status);
        if ( *ptrSkeleton != testBaseSkeletonsResults[i] ) {
            errln("ERROR: Unexpected result from getBaseSkeletons() !\n");
        }
    }

    // ========= DateTimePatternGenerator sample code in Userguide
    // set up the generator
    Locale locale = Locale::getFrench();
    status = U_ZERO_ERROR;
    DateTimePatternGenerator *generator = DateTimePatternGenerator::createInstance( locale, status);
        
    // get a pattern for an abbreviated month and day
    pattern = generator->getBestPattern(UnicodeString("MMMd"), status); 
    SimpleDateFormat formatter(pattern, locale, status); 

    zone = TimeZone::createTimeZone(UnicodeString("GMT"));
    formatter.setTimeZone(*zone);
    // use it to format (or parse)
    UnicodeString formatted;
    formatted = formatter.format(Calendar::getNow(), formatted, status); 
    // for French, the result is "13 sept."
    formatted.remove();
    // cannot use the result from getNow() because the value change evreyday.
    testDate= LocaleTest::date(99, 0, 13, 23, 58, 59);
    formatted = formatter.format(testDate, formatted, status);
    expectedResult=UnicodeString("14 janv.");
    if ( formatted != expectedResult ) {
        errln("ERROR: Userguide sample code result!");
        errln(UnicodeString(" Got: ")+ formatted + UnicodeString(" Expected: ") + expectedResult);
    }

    delete zone;
    delete output;
    delete ptrSkeletonEnum;
    delete ptrBaseSkeletonEnum;
    delete test;
    delete generator;
}

/**
 * Test handling of options
 *
 * For reference, as of ICU 4.3.3,
 *  root/gregorian has
 *      Hm{"H:mm"}
 *      Hms{"H:mm:ss"}
 *      hm{"h:mm a"}
 *      hms{"h:mm:ss a"}
 *  en/gregorian has
 *      Hm{"H:mm"}
 *      Hms{"H:mm:ss"}
 *      hm{"h:mm a"}
 *  be/gregorian has
 *      HHmmss{"HH.mm.ss"}
 *      Hm{"HH.mm"}
 *      hm{"h.mm a"}
 *      hms{"h.mm.ss a"}
 */
typedef struct DTPtnGenOptionsData {
    const char *locale;
    const char *skel;
    const char *expectedPattern;
    UDateTimePatternMatchOptions    options;
} DTPtnGenOptionsData;
void IntlTestDateTimePatternGeneratorAPI::testOptions(/*char *par*/)
{
    DTPtnGenOptionsData testData[] = {
    //   locale  skel   expectedPattern     options
        { "en", "Hmm",  "HH:mm",   UDATPG_MATCH_NO_OPTIONS        },
        { "en", "HHmm", "HH:mm",   UDATPG_MATCH_NO_OPTIONS        },
        { "en", "hhmm", "h:mm a",  UDATPG_MATCH_NO_OPTIONS        },
        { "en", "Hmm",  "HH:mm",   UDATPG_MATCH_HOUR_FIELD_LENGTH },
        { "en", "HHmm", "HH:mm",   UDATPG_MATCH_HOUR_FIELD_LENGTH },
        { "en", "hhmm", "hh:mm a", UDATPG_MATCH_HOUR_FIELD_LENGTH },
        { "be", "Hmm",  "HH.mm",   UDATPG_MATCH_NO_OPTIONS        },
        { "be", "HHmm", "HH.mm",   UDATPG_MATCH_NO_OPTIONS        },
        { "be", "hhmm", "h.mm a",  UDATPG_MATCH_NO_OPTIONS        },
        { "be", "Hmm",  "H.mm",    UDATPG_MATCH_HOUR_FIELD_LENGTH },
        { "be", "HHmm", "HH.mm",   UDATPG_MATCH_HOUR_FIELD_LENGTH },
        { "be", "hhmm", "hh.mm a", UDATPG_MATCH_HOUR_FIELD_LENGTH },
    };
    
    int count = sizeof(testData) / sizeof(testData[0]);
    const DTPtnGenOptionsData * testDataPtr = testData;
    
    for (; count-- > 0; ++testDataPtr) {
        UErrorCode status = U_ZERO_ERROR;

        Locale locale(testDataPtr->locale);
        UnicodeString skel(testDataPtr->skel);
        UnicodeString expectedPattern(testDataPtr->expectedPattern);
        UDateTimePatternMatchOptions options = testDataPtr->options;

        DateTimePatternGenerator * dtpgen = DateTimePatternGenerator::createInstance(locale, status);
        if (U_FAILURE(status)) {
            dataerrln("Unable to create DateTimePatternGenerator instance for locale(%s): %s", locale.getName(), u_errorName(status));
            delete dtpgen;
            continue;
        }
        UnicodeString pattern = dtpgen->getBestPattern(skel, options, status);
        if (pattern.compare(expectedPattern) != 0) {
            errln( UnicodeString("ERROR in getBestPattern, locale ") + UnicodeString(testDataPtr->locale) +
                   UnicodeString(", skeleton ") + skel +
                   ((options)?UnicodeString(", options!=0"):UnicodeString(", options==0")) +
                   UnicodeString(", expected pattern ") + expectedPattern +
                   UnicodeString(", got ") + pattern );
        }
        delete dtpgen;
    }
}

#endif /* #if !UCONFIG_NO_FORMATTING */
