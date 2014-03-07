/*
*******************************************************************************
* Copyright (C) 2007-2010, International Business Machines Corporation and    *
* others. All Rights Reserved.                                                *
*******************************************************************************
*/
#include "unicode/utypes.h"

#if !UCONFIG_NO_FORMATTING

#include "tzfmttst.h"

#include "simplethread.h"
#include "unicode/timezone.h"
#include "unicode/simpletz.h"
#include "unicode/calendar.h"
#include "unicode/strenum.h"
#include "unicode/smpdtfmt.h"
#include "unicode/uchar.h"
#include "unicode/basictz.h"
#include "cstring.h"

static const char* PATTERNS[] = {"z", "zzzz", "Z", "ZZZZ", "v", "vvvv", "V", "VVVV"};
static const int NUM_PATTERNS = sizeof(PATTERNS)/sizeof(const char*);

void
TimeZoneFormatTest::runIndexedTest( int32_t index, UBool exec, const char* &name, char* /*par*/ )
{
    if (exec) {
        logln("TestSuite TimeZoneFormatTest");
    }
    switch (index) {
        TESTCASE(0, TestTimeZoneRoundTrip);
        TESTCASE(1, TestTimeRoundTrip);
        default: name = ""; break;
    }
}

void
TimeZoneFormatTest::TestTimeZoneRoundTrip(void) {
    UErrorCode status = U_ZERO_ERROR;

    SimpleTimeZone unknownZone(-31415, (UnicodeString)"Etc/Unknown");
    int32_t badDstOffset = -1234;
    int32_t badZoneOffset = -2345;

    int32_t testDateData[][3] = {
        {2007, 1, 15},
        {2007, 6, 15},
        {1990, 1, 15},
        {1990, 6, 15},
        {1960, 1, 15},
        {1960, 6, 15},
    };

    Calendar *cal = Calendar::createInstance(TimeZone::createTimeZone((UnicodeString)"UTC"), status);
    if (U_FAILURE(status)) {
        dataerrln("Calendar::createInstance failed: %s", u_errorName(status));
        return;
    }

    // Set up rule equivalency test range
    UDate low, high;
    cal->set(1900, UCAL_JANUARY, 1);
    low = cal->getTime(status);
    cal->set(2040, UCAL_JANUARY, 1);
    high = cal->getTime(status);
    if (U_FAILURE(status)) {
        errln("getTime failed");
        return;
    }

    // Set up test dates
    UDate DATES[(sizeof(testDateData)/sizeof(int32_t))/3];
    const int32_t nDates = (sizeof(testDateData)/sizeof(int32_t))/3;
    cal->clear();
    for (int32_t i = 0; i < nDates; i++) {
        cal->set(testDateData[i][0], testDateData[i][1], testDateData[i][2]);
        DATES[i] = cal->getTime(status);
        if (U_FAILURE(status)) {
            errln("getTime failed");
            return;
        }
    }

    // Set up test locales
    const Locale testLocales[] = {
        Locale("en"),
        Locale("en_CA"),
        Locale("fr"),
        Locale("zh_Hant")
    };

    const Locale *LOCALES;
    int32_t nLocales;

    if (quick) {
        LOCALES = testLocales;
        nLocales = sizeof(testLocales)/sizeof(Locale);
    } else {
        LOCALES = Locale::getAvailableLocales(nLocales);
    }

    StringEnumeration *tzids = TimeZone::createEnumeration();
    if (U_FAILURE(status)) {
        errln("tzids->count failed");
        return;
    }

    int32_t inRaw, inDst;
    int32_t outRaw, outDst;

    // Run the roundtrip test
    for (int32_t locidx = 0; locidx < nLocales; locidx++) {
        for (int32_t patidx = 0; patidx < NUM_PATTERNS; patidx++) {

            SimpleDateFormat *sdf = new SimpleDateFormat((UnicodeString)PATTERNS[patidx], LOCALES[locidx], status);
            if (U_FAILURE(status)) {
                errcheckln(status, (UnicodeString)"new SimpleDateFormat failed for pattern " +
                    PATTERNS[patidx] + " for locale " + LOCALES[locidx].getName() + " - " + u_errorName(status));
                status = U_ZERO_ERROR;
                continue;
            }

            tzids->reset(status);
            const UnicodeString *tzid;
            while ((tzid = tzids->snext(status))) {
                TimeZone *tz = TimeZone::createTimeZone(*tzid);

                for (int32_t datidx = 0; datidx < nDates; datidx++) {
                    UnicodeString tzstr;
                    FieldPosition fpos(0);
                    // Format
                    sdf->setTimeZone(*tz);
                    sdf->format(DATES[datidx], tzstr, fpos);

                    // Before parse, set unknown zone to SimpleDateFormat instance
                    // just for making sure that it does not depends on the time zone
                    // originally set.
                    sdf->setTimeZone(unknownZone);

                    // Parse
                    ParsePosition pos(0);
                    Calendar *outcal = Calendar::createInstance(unknownZone, status);
                    if (U_FAILURE(status)) {
                        errln("Failed to create an instance of calendar for receiving parse result.");
                        status = U_ZERO_ERROR;
                        continue;
                    }
                    outcal->set(UCAL_DST_OFFSET, badDstOffset);
                    outcal->set(UCAL_ZONE_OFFSET, badZoneOffset);

                    sdf->parse(tzstr, *outcal, pos);

                    // Check the result
                    const TimeZone &outtz = outcal->getTimeZone();
                    UnicodeString outtzid;
                    outtz.getID(outtzid);

                    tz->getOffset(DATES[datidx], false, inRaw, inDst, status);
                    if (U_FAILURE(status)) {
                        errln((UnicodeString)"Failed to get offsets from time zone" + *tzid);
                        status = U_ZERO_ERROR;
                    }
                    outtz.getOffset(DATES[datidx], false, outRaw, outDst, status);
                    if (U_FAILURE(status)) {
                        errln((UnicodeString)"Failed to get offsets from time zone" + outtzid);
                        status = U_ZERO_ERROR;
                    }

                    if (uprv_strcmp(PATTERNS[patidx], "VVVV") == 0) {
                        // Location: time zone rule must be preserved except
                        // zones not actually associated with a specific location.
                        // Time zones in this category do not have "/" in its ID.
                        UnicodeString canonical;
                        TimeZone::getCanonicalID(*tzid, canonical, status);
                        if (U_FAILURE(status)) {
                            // Uknown ID - we should not get here
                            errln((UnicodeString)"Unknown ID " + *tzid);
                            status = U_ZERO_ERROR;
                        } else if (outtzid != canonical) {
                            // Canonical ID did not match - check the rules
                            if (!((BasicTimeZone*)&outtz)->hasEquivalentTransitions((BasicTimeZone&)*tz, low, high, TRUE, status)) {
                                if (canonical.indexOf((UChar)0x27 /*'/'*/) == -1) {
                                    // Exceptional cases, such as CET, EET, MET and WET
                                    logln("Canonical round trip failed (as expected); tz=" + *tzid
                                            + ", locale=" + LOCALES[locidx].getName() + ", pattern=" + PATTERNS[patidx]
                                            + ", time=" + DATES[datidx] + ", str=" + tzstr
                                            + ", outtz=" + outtzid);
                                } else {
                                    errln("Canonical round trip failed; tz=" + *tzid
                                        + ", locale=" + LOCALES[locidx].getName() + ", pattern=" + PATTERNS[patidx]
                                        + ", time=" + DATES[datidx] + ", str=" + tzstr
                                        + ", outtz=" + outtzid);
                                }
                                if (U_FAILURE(status)) {
                                    errln("hasEquivalentTransitions failed");
                                    status = U_ZERO_ERROR;
                                }
                            }
                        }

                    } else {
                        // Check if localized GMT format or RFC format is used.
                        int32_t numDigits = 0;
                        for (int n = 0; n < tzstr.length(); n++) {
                            if (u_isdigit(tzstr.charAt(n))) {
                                numDigits++;
                            }
                        }
                        if (numDigits >= 3) {
                            // Localized GMT or RFC: total offset (raw + dst) must be preserved.
                            int32_t inOffset = inRaw + inDst;
                            int32_t outOffset = outRaw + outDst;
                            if (inOffset != outOffset) {
                                errln((UnicodeString)"Offset round trip failed; tz=" + *tzid
                                    + ", locale=" + LOCALES[locidx].getName() + ", pattern=" + PATTERNS[patidx]
                                    + ", time=" + DATES[datidx] + ", str=" + tzstr
                                    + ", inOffset=" + inOffset + ", outOffset=" + outOffset);
                            }
                        } else {
                            // Specific or generic: raw offset must be preserved.
                            if (inRaw != outRaw) {
                                errln((UnicodeString)"Raw offset round trip failed; tz=" + *tzid
                                    + ", locale=" + LOCALES[locidx].getName() + ", pattern=" + PATTERNS[patidx]
                                    + ", time=" + DATES[datidx] + ", str=" + tzstr
                                    + ", inRawOffset=" + inRaw + ", outRawOffset=" + outRaw);
                            }
                        }
                    }
                    delete outcal;
                }
                delete tz;
            }
            delete sdf;
        }
    }
    delete cal;
    delete tzids;
}

struct LocaleData {
    int32_t index;
    int32_t testCounts;
    UDate *times;
    const Locale* locales; // Static
    int32_t nLocales; // Static
    UBool quick; // Static
    UDate START_TIME; // Static
    UDate END_TIME; // Static
    int32_t numDone;
};

class TestTimeRoundTripThread: public SimpleThread {
public:
    TestTimeRoundTripThread(IntlTest& tlog, LocaleData &ld, int32_t i)
        : log(tlog), data(ld), index(i) {}
    virtual void run() {
        UErrorCode status = U_ZERO_ERROR;
        UBool REALLY_VERBOSE = FALSE;

        // Whether each pattern is ambiguous at DST->STD local time overlap
        UBool AMBIGUOUS_DST_DECESSION[] = { FALSE, FALSE, FALSE, FALSE, TRUE, TRUE, FALSE, TRUE };
        // Whether each pattern is ambiguous at STD->STD/DST->DST local time overlap
        UBool AMBIGUOUS_NEGATIVE_SHIFT[] = { TRUE, TRUE, FALSE, FALSE, TRUE, TRUE, TRUE, TRUE };

        // Workaround for #6338
        //UnicodeString BASEPATTERN("yyyy-MM-dd'T'HH:mm:ss.SSS");
        UnicodeString BASEPATTERN("yyyy.MM.dd HH:mm:ss.SSS");

        // timer for performance analysis
        UDate timer;
        UDate testTimes[4];
        UBool expectedRoundTrip[4];
        int32_t testLen = 0;

        StringEnumeration *tzids = TimeZone::createEnumeration();
        if (U_FAILURE(status)) {
            log.errln("tzids->count failed");
            return;
        }

        int32_t locidx = -1;
        UDate times[NUM_PATTERNS];
        for (int32_t i = 0; i < NUM_PATTERNS; i++) {
            times[i] = 0;
        }

        int32_t testCounts = 0;

        while (true) {
            umtx_lock(NULL); // Lock to increment the index
            for (int32_t i = 0; i < NUM_PATTERNS; i++) {
                data.times[i] += times[i];
                data.testCounts += testCounts;
            }
            if (data.index < data.nLocales) {
                locidx = data.index;
                data.index++;
            } else {
                locidx = -1;
            }
            umtx_unlock(NULL); // Unlock for other threads to use

            if (locidx == -1) {
                log.logln((UnicodeString) "Thread " + index + " is done.");
                break;
            }

            log.logln((UnicodeString) "\nThread " + index + ": Locale: " + UnicodeString(data.locales[locidx].getName()));

            for (int32_t patidx = 0; patidx < NUM_PATTERNS; patidx++) {
                log.logln((UnicodeString) "    Pattern: " + PATTERNS[patidx]);
                times[patidx] = 0;

                UnicodeString pattern(BASEPATTERN);
                pattern.append(" ").append(PATTERNS[patidx]);

                SimpleDateFormat *sdf = new SimpleDateFormat(pattern, data.locales[locidx], status);
                if (U_FAILURE(status)) {
                    log.errcheckln(status, (UnicodeString) "new SimpleDateFormat failed for pattern " + 
                        pattern + " for locale " + data.locales[locidx].getName() + " - " + u_errorName(status));
                    status = U_ZERO_ERROR;
                    continue;
                }

                tzids->reset(status);
                const UnicodeString *tzid;

                timer = Calendar::getNow();

                while ((tzid = tzids->snext(status))) {
                    UnicodeString canonical;
                    TimeZone::getCanonicalID(*tzid, canonical, status);
                    if (U_FAILURE(status)) {
                        // Unknown ID - we should not get here
                        status = U_ZERO_ERROR;
                        continue;
                    }
                    if (*tzid != canonical) {
                        // Skip aliases
                        continue;
                    }
                    BasicTimeZone *tz = (BasicTimeZone*) TimeZone::createTimeZone(*tzid);
                    sdf->setTimeZone(*tz);

                    UDate t = data.START_TIME;
                    TimeZoneTransition tzt;
                    UBool tztAvail = FALSE;
                    UBool middle = TRUE;

                    while (t < data.END_TIME) {
                        if (!tztAvail) {
                            testTimes[0] = t;
                            expectedRoundTrip[0] = TRUE;
                            testLen = 1;
                        } else {
                            int32_t fromOffset = tzt.getFrom()->getRawOffset() + tzt.getFrom()->getDSTSavings();
                            int32_t toOffset = tzt.getTo()->getRawOffset() + tzt.getTo()->getDSTSavings();
                            int32_t delta = toOffset - fromOffset;
                            if (delta < 0) {
                                UBool isDstDecession = tzt.getFrom()->getDSTSavings() > 0 && tzt.getTo()->getDSTSavings() == 0;
                                testTimes[0] = t + delta - 1;
                                expectedRoundTrip[0] = TRUE;
                                testTimes[1] = t + delta;
                                expectedRoundTrip[1] = isDstDecession ? !AMBIGUOUS_DST_DECESSION[patidx] : !AMBIGUOUS_NEGATIVE_SHIFT[patidx];
                                testTimes[2] = t - 1;
                                expectedRoundTrip[2] = isDstDecession ? !AMBIGUOUS_DST_DECESSION[patidx] : !AMBIGUOUS_NEGATIVE_SHIFT[patidx];
                                testTimes[3] = t;
                                expectedRoundTrip[3] = TRUE;
                                testLen = 4;
                            } else {
                                testTimes[0] = t - 1;
                                expectedRoundTrip[0] = TRUE;
                                testTimes[1] = t;
                                expectedRoundTrip[1] = TRUE;
                                testLen = 2;
                            }
                        }
                        for (int32_t testidx = 0; testidx < testLen; testidx++) {
                            if (data.quick) {
                                // reduce regular test time
                                if (!expectedRoundTrip[testidx]) {
                                    continue;
                                }
                            }

                            testCounts++;

                            UnicodeString text;
                            FieldPosition fpos(0);
                            sdf->format(testTimes[testidx], text, fpos);

                            UDate parsedDate = sdf->parse(text, status);
                            if (U_FAILURE(status)) {
                                log.errln((UnicodeString) "Parse failure for text=" + text + ", tzid=" + *tzid + ", locale=" + data.locales[locidx].getName()
                                        + ", pattern=" + PATTERNS[patidx] + ", time=" + testTimes[testidx]);
                                status = U_ZERO_ERROR;
                                continue;
                            }
                            if (parsedDate != testTimes[testidx]) {
                                UnicodeString msg = (UnicodeString) "Time round trip failed for " + "tzid=" + *tzid + ", locale=" + data.locales[locidx].getName() + ", pattern=" + PATTERNS[patidx]
                                        + ", text=" + text + ", time=" + testTimes[testidx] + ", restime=" + parsedDate + ", diff=" + (parsedDate - testTimes[testidx]);
                                if (expectedRoundTrip[testidx]) {
                                    log.errln((UnicodeString) "FAIL: " + msg);
                                } else if (REALLY_VERBOSE) {
                                    log.logln(msg);
                                }
                            }
                        }
                        tztAvail = tz->getNextTransition(t, FALSE, tzt);
                        if (!tztAvail) {
                            break;
                        }
                        if (middle) {
                            // Test the date in the middle of two transitions.
                            t += (int64_t) ((tzt.getTime() - t) / 2);
                            middle = FALSE;
                            tztAvail = FALSE;
                        } else {
                            t = tzt.getTime();
                        }
                    }
                    delete tz;
                }
                times[patidx] += (Calendar::getNow() - timer);
                delete sdf;
            }
            umtx_lock(NULL);
            data.numDone++;
            umtx_unlock(NULL);
        }
        delete tzids;
    }
private:
    IntlTest& log;
    LocaleData& data;
    int32_t index;
};

void
TimeZoneFormatTest::TestTimeRoundTrip(void) {
    int32_t nThreads = threadCount;
    const Locale *LOCALES;
    int32_t nLocales;
    int32_t testCounts = 0;

    UErrorCode status = U_ZERO_ERROR;
    Calendar *cal = Calendar::createInstance(TimeZone::createTimeZone((UnicodeString) "UTC"), status);
    if (U_FAILURE(status)) {
        dataerrln("Calendar::createInstance failed: %s", u_errorName(status));
        return;
    }

    const char* testAllProp = getProperty("TimeZoneRoundTripAll");
    UBool bTestAll = (testAllProp && uprv_strcmp(testAllProp, "true") == 0);

    UDate START_TIME, END_TIME;
    if (bTestAll || !quick) {
        cal->set(1900, UCAL_JANUARY, 1);
    } else {
        cal->set(1990, UCAL_JANUARY, 1);
    }
    START_TIME = cal->getTime(status);

    cal->set(2015, UCAL_JANUARY, 1);
    END_TIME = cal->getTime(status);

    if (U_FAILURE(status)) {
        errln("getTime failed");
        return;
    }

    UDate times[NUM_PATTERNS];
    for (int32_t i = 0; i < NUM_PATTERNS; i++) {
        times[i] = 0;
    }

    // Set up test locales
    const Locale locales1[] = {Locale("en")};
    const Locale locales2[] = {
        Locale("ar_EG"), Locale("bg_BG"), Locale("ca_ES"), Locale("da_DK"), Locale("de"),
        Locale("de_DE"), Locale("el_GR"), Locale("en"), Locale("en_AU"), Locale("en_CA"),
        Locale("en_US"), Locale("es"), Locale("es_ES"), Locale("es_MX"), Locale("fi_FI"),
        Locale("fr"), Locale("fr_CA"), Locale("fr_FR"), Locale("he_IL"), Locale("hu_HU"),
        Locale("it"), Locale("it_IT"), Locale("ja"), Locale("ja_JP"), Locale("ko"),
        Locale("ko_KR"), Locale("nb_NO"), Locale("nl_NL"), Locale("nn_NO"), Locale("pl_PL"),
        Locale("pt"), Locale("pt_BR"), Locale("pt_PT"), Locale("ru_RU"), Locale("sv_SE"),
        Locale("th_TH"), Locale("tr_TR"), Locale("zh"), Locale("zh_Hans"), Locale("zh_Hans_CN"),
        Locale("zh_Hant"), Locale("zh_Hant_TW")
    };

    if (bTestAll) {
        LOCALES = Locale::getAvailableLocales(nLocales);
    } else if (quick) {
        LOCALES = locales1;
        nLocales = sizeof(locales1)/sizeof(Locale);
    } else {
        LOCALES = locales2;
        nLocales = sizeof(locales2)/sizeof(Locale);
    }

    LocaleData data;
    data.index = 0;
    data.testCounts = testCounts;
    data.times = times;
    data.locales = LOCALES;
    data.nLocales = nLocales;
    data.quick = quick;
    data.START_TIME = START_TIME;
    data.END_TIME = END_TIME;
    data.numDone = 0;

#if (ICU_USE_THREADS==0)
    TestTimeRoundTripThread fakeThread(*this, data, 0);
    fakeThread.run();
#else
    TestTimeRoundTripThread **threads = new TestTimeRoundTripThread*[threadCount];
    int32_t i;
    for (i = 0; i < nThreads; i++) {
        threads[i] = new TestTimeRoundTripThread(*this, data, i);
        if (threads[i]->start() != 0) {
            errln("Error starting thread %d", i);
        }
    }

    UBool done = false;
    while (true) {
        umtx_lock(NULL);
        if (data.numDone == nLocales) {
            done = true;
        }
        umtx_unlock(NULL);
        if (done)
            break;
        SimpleThread::sleep(1000);
    }

    for (i = 0; i < nThreads; i++) {
        delete threads[i];
    }
    delete [] threads;

#endif
    UDate total = 0;
    logln("### Elapsed time by patterns ###");
    for (int32_t i = 0; i < NUM_PATTERNS; i++) {
        logln(UnicodeString("") + data.times[i] + "ms (" + PATTERNS[i] + ")");
        total += data.times[i];
    }
    logln((UnicodeString) "Total: " + total + "ms");
    logln((UnicodeString) "Iteration: " + data.testCounts);

    delete cal;
}

#endif /* #if !UCONFIG_NO_FORMATTING */
