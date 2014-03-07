/*
 ****************************************************************************
 * Copyright (c) 1997-2010, International Business Machines Corporation and *
 * others. All Rights Reserved.                                             *
 ****************************************************************************
 */

#include "unicode/utypes.h"

#if !UCONFIG_NO_FORMATTING

#include "unicode/utmscale.h"
#include "unicode/ucal.h"

#include "cintltst.h"

#include <stdlib.h>
#include <time.h>

#define LENGTHOF(array) (int32_t)(sizeof(array)/sizeof((array)[0]))

#define LOOP_COUNT 10000

static void TestAPI(void);
static void TestData(void);
static void TestMonkey(void);
static void TestDotNet(void);

void addUtmsTest(TestNode** root);

void addUtmsTest(TestNode** root)
{
    addTest(root, &TestAPI, "tsformat/utmstest/TestAPI");
    addTest(root, &TestData, "tsformat/utmstest/TestData");
    addTest(root, &TestMonkey, "tsformat/utmstest/TestMonkey");
    addTest(root, &TestDotNet, "tsformat/utmstest/TestDotNet");
}

/**
 * Return a random int64_t where U_INT64_MIN <= ran <= U_INT64_MAX.
 */
static uint64_t randomInt64(void)
{
    int64_t ran = 0;
    int32_t i;
    static UBool initialized = FALSE;

    if (!initialized) {
        srand((unsigned)time(NULL));
        initialized = TRUE;
    }

    /* Assume rand has at least 12 bits of precision */
    for (i = 0; i < sizeof(ran); i += 1) {
        ((char*)&ran)[i] = (char)((rand() & 0x0FF0) >> 4);
    }

    return ran;
}

static int64_t ranInt;
static int64_t ranMin;
static int64_t ranMax;

static void initRandom(int64_t min, int64_t max)
{
    uint64_t interval = max - min;

    ranMin = min;
    ranMax = max;
    ranInt = 0;

    /* Verify that we don't have a huge interval. */
    if (interval < (uint64_t)U_INT64_MAX) {
        ranInt = interval;
    }
}

static int64_t randomInRange(void)
{
    int64_t value;

    if (ranInt != 0) {
        value = randomInt64() % ranInt;

        if (value < 0) {
            value = -value;
        }

        value += ranMin;
    } else {
        do {
            value = randomInt64();
        } while (value < ranMin || value > ranMax);
    }

    return value;
}

static void roundTripTest(int64_t value, UDateTimeScale scale)
{
    UErrorCode status = U_ZERO_ERROR;
    int64_t rt = utmscale_toInt64(utmscale_fromInt64(value, scale, &status), scale, &status);

    if (rt != value) {
        log_err("Round-trip error: time scale = %d, value = %lld, round-trip = %lld.\n", scale, value, rt);
    }
}

static void toLimitTest(int64_t toLimit, int64_t fromLimit, UDateTimeScale scale)
{
    UErrorCode status = U_ZERO_ERROR;
    int64_t result = utmscale_toInt64(toLimit, scale, &status);

    if (result != fromLimit) {
        log_err("toLimit failure: scale = %d, toLimit = %lld , utmscale_toInt64(toLimit, scale, &status) = %lld, fromLimit = %lld.\n",
            scale, toLimit, result, fromLimit);
    }
}

static void epochOffsetTest(int64_t epochOffset, int64_t units, UDateTimeScale scale)
{
    UErrorCode status = U_ZERO_ERROR;
    int64_t universal = 0;
    int64_t universalEpoch = epochOffset * units;
    int64_t local = utmscale_toInt64(universalEpoch, scale, &status);

    if (local != 0) {
        log_err("utmscale_toInt64(epochOffset, scale, &status): scale = %d epochOffset = %lld, result = %lld.\n", scale, epochOffset, local);
    }

    local = utmscale_toInt64(0, scale, &status);

    if (local != -epochOffset) {
        log_err("utmscale_toInt64(0, scale): scale = %d, result = %lld.\n", scale, local);
    }

    universal = utmscale_fromInt64(-epochOffset, scale, &status);

    if (universal != 0) {
        log_err("from(-epochOffest, scale): scale = %d, epochOffset = %lld, result = %lld.\n", scale, epochOffset, universal);
    }

    universal = utmscale_fromInt64(0, scale, &status);

    if (universal != universalEpoch) {
        log_err("utmscale_fromInt64(0, scale): scale = %d, result = %lld.\n", scale, universal);
    }
}

static void TestEpochOffsets(void)
{
    UErrorCode status = U_ZERO_ERROR;
    int32_t scale;

    for (scale = 0; scale < UDTS_MAX_SCALE; scale += 1) {
        int64_t units       = utmscale_getTimeScaleValue((UDateTimeScale)scale, UTSV_UNITS_VALUE, &status);
        int64_t epochOffset = utmscale_getTimeScaleValue((UDateTimeScale)scale, UTSV_EPOCH_OFFSET_VALUE, &status);

        epochOffsetTest(epochOffset, units, (UDateTimeScale)scale);
    }
}

static void TestFromLimits(void)
{
    UErrorCode status = U_ZERO_ERROR;
    int32_t scale;

    for (scale = 0; scale < UDTS_MAX_SCALE; scale += 1) {
        int64_t fromMin = utmscale_getTimeScaleValue((UDateTimeScale)scale, UTSV_FROM_MIN_VALUE, &status);
        int64_t fromMax = utmscale_getTimeScaleValue((UDateTimeScale)scale, UTSV_FROM_MAX_VALUE, &status);

        roundTripTest(fromMin, (UDateTimeScale)scale);
        roundTripTest(fromMax, (UDateTimeScale)scale);
    }
}

static void TestToLimits(void)
{
    UErrorCode status = U_ZERO_ERROR;
    int32_t scale;

    for (scale = 0; scale < UDTS_MAX_SCALE; scale += 1) {
        int64_t fromMin = utmscale_getTimeScaleValue((UDateTimeScale)scale, UTSV_FROM_MIN_VALUE, &status);
        int64_t fromMax = utmscale_getTimeScaleValue((UDateTimeScale)scale, UTSV_FROM_MAX_VALUE, &status);
        int64_t toMin   = utmscale_getTimeScaleValue((UDateTimeScale)scale, UTSV_TO_MIN_VALUE, &status);
        int64_t toMax   = utmscale_getTimeScaleValue((UDateTimeScale)scale, UTSV_TO_MAX_VALUE, &status);

        toLimitTest(toMin, fromMin, (UDateTimeScale)scale);
        toLimitTest(toMax, fromMax, (UDateTimeScale)scale);
    }
}

static void TestFromInt64(void)
{
    int32_t scale;
    int64_t result;
    UErrorCode status = U_ZERO_ERROR;

    result = utmscale_fromInt64(0, -1, &status);
    if (status != U_ILLEGAL_ARGUMENT_ERROR) {
        log_err("utmscale_fromInt64(0, -1, status) did not set status to U_ILLEGAL_ARGUMENT_ERROR.\n");
    }

    for (scale = 0; scale < UDTS_MAX_SCALE; scale += 1) {
        int64_t fromMin, fromMax;

        status = U_ZERO_ERROR;
        fromMin = utmscale_getTimeScaleValue((UDateTimeScale)scale, UTSV_FROM_MIN_VALUE, &status);
        fromMax = utmscale_getTimeScaleValue((UDateTimeScale)scale, UTSV_FROM_MAX_VALUE, &status);

        status = U_ZERO_ERROR;
        result = utmscale_fromInt64(0, (UDateTimeScale)scale, &status);
        if (status == U_ILLEGAL_ARGUMENT_ERROR) {
            log_err("utmscale_fromInt64(0, %d, &status) generated U_ILLEGAL_ARGUMENT_ERROR.\n", scale);
        }

        status = U_ZERO_ERROR;
        result = utmscale_fromInt64(fromMin, (UDateTimeScale)scale, &status);
        if (status == U_ILLEGAL_ARGUMENT_ERROR) {
            log_err("utmscale_fromInt64(fromMin, %d, &status) generated U_ILLEGAL_ARGUMENT_ERROR.\n", scale);
        }

        if (fromMin > U_INT64_MIN) {
            status = U_ZERO_ERROR;
            result = utmscale_fromInt64(fromMin - 1, (UDateTimeScale)scale, &status);
            if (status != U_ILLEGAL_ARGUMENT_ERROR) {
                log_err("utmscale_fromInt64(fromMin - 1, %d, &status) did not generate U_ILLEGAL_ARGUMENT_ERROR.\n", scale);
            }
        }

        status = U_ZERO_ERROR;
        result = utmscale_fromInt64(fromMax, (UDateTimeScale)scale, &status);
        if (status == U_ILLEGAL_ARGUMENT_ERROR) {
            log_err("utmscale_fromInt64(fromMax, %d, &status) generated U_ILLEGAL_ARGUMENT_ERROR.\n", scale);
        }

        if (fromMax < U_INT64_MAX) {
            status = U_ZERO_ERROR;
            result = utmscale_fromInt64(fromMax + 1, (UDateTimeScale)scale, &status);
            if (status != U_ILLEGAL_ARGUMENT_ERROR) {
                log_err("utmscale_fromInt64(fromMax + 1, %d, &status) didn't generate U_ILLEGAL_ARGUMENT_ERROR.\n", scale);
            }
        }
    }

    status = U_ZERO_ERROR;
    result = utmscale_fromInt64(0, UDTS_MAX_SCALE, &status);
    if (status != U_ILLEGAL_ARGUMENT_ERROR) {
        log_err("utmscale_fromInt64(0, UDTS_MAX_SCALE, &status) did not generate U_ILLEGAL_ARGUMENT_ERROR.\n");
    }
}

static void TestToInt64(void)
{
    int32_t scale;
    int64_t result;
    UErrorCode status = U_ZERO_ERROR;

    result = utmscale_toInt64(0, -1, &status);
    if (status != U_ILLEGAL_ARGUMENT_ERROR) {
        log_err("utmscale_toInt64(0, -1, &status) did not generate U_ILLEGAL_ARGUMENT_ERROR.\n");
    }

    for (scale = 0; scale < UDTS_MAX_SCALE; scale += 1) {
        int64_t toMin, toMax;

        status = U_ZERO_ERROR;
        toMin = utmscale_getTimeScaleValue((UDateTimeScale)scale, UTSV_TO_MIN_VALUE, &status);
        toMax = utmscale_getTimeScaleValue((UDateTimeScale)scale, UTSV_TO_MAX_VALUE, &status);

        status = U_ZERO_ERROR;
        result = utmscale_toInt64(0, (UDateTimeScale)scale, &status);
        if (status == U_ILLEGAL_ARGUMENT_ERROR) {
            log_err("utmscale_toInt64(0, %d, &status) generated U_ILLEGAL_ARGUMENT_ERROR.\n", scale);
        }

        status = U_ZERO_ERROR;
        result = utmscale_toInt64(toMin, (UDateTimeScale)scale, &status);
        if (status == U_ILLEGAL_ARGUMENT_ERROR) {
            log_err("utmscale_toInt64(toMin, %d, &status) generated U_ILLEGAL_ARGUMENT_ERROR.\n", scale);
        }

        if (toMin > U_INT64_MIN) {
            status = U_ZERO_ERROR;
            result = utmscale_toInt64(toMin - 1, (UDateTimeScale)scale, &status);
            if (status != U_ILLEGAL_ARGUMENT_ERROR) {
                log_err("utmscale_toInt64(toMin - 1, %d, &status) did not generate U_ILLEGAL_ARGUMENT_ERROR.\n", scale);
            }
        }


        status = U_ZERO_ERROR;
        result = utmscale_toInt64(toMax, (UDateTimeScale)scale, &status);
        if (status == U_ILLEGAL_ARGUMENT_ERROR) {
            log_err("utmscale_toInt64(toMax, %d, &status) generated U_ILLEGAL_ARGUMENT_ERROR.\n", scale);
        }

        if (toMax < U_INT64_MAX) {
            status = U_ZERO_ERROR;
            result = utmscale_toInt64(toMax + 1, (UDateTimeScale)scale, &status);
            if (status != U_ILLEGAL_ARGUMENT_ERROR) {
                log_err("utmscale_toInt64(toMax + 1, %d, &status) did not generate U_ILLEGAL_ARGUMENT_ERROR.\n", scale);
            }
        }
    }

    status = U_ZERO_ERROR;
    result = utmscale_toInt64(0, UDTS_MAX_SCALE, &status);
    if (status != U_ILLEGAL_ARGUMENT_ERROR) {
        log_err("utmscale_toInt64(0, UDTS_MAX_SCALE, &status) did not generate U_ILLEGAL_ARGUMENT_ERROR.\n");
    }
}

static void TestAPI(void)
{
    TestFromInt64();
    TestToInt64();
}

static void TestData(void)
{
    TestEpochOffsets();
    TestFromLimits();
    TestToLimits();
}

static void TestMonkey(void)
{
    int32_t scale;
    UErrorCode status = U_ZERO_ERROR;

    for (scale = 0; scale < UDTS_MAX_SCALE; scale += 1) {
        int64_t fromMin = utmscale_getTimeScaleValue((UDateTimeScale)scale, UTSV_FROM_MIN_VALUE, &status);
        int64_t fromMax = utmscale_getTimeScaleValue((UDateTimeScale)scale, UTSV_FROM_MAX_VALUE, &status);
        int32_t i;

        initRandom(fromMin, fromMax);

        for (i = 0; i < LOOP_COUNT; i += 1) {
            int64_t value = randomInRange();

            roundTripTest(value, (UDateTimeScale)scale);
        }
    }
}

struct DotNetDateTimeTicks {
    int32_t year;
    int32_t month;
    int32_t day;
    int64_t ticks;
};
typedef struct DotNetDateTimeTicks DotNetDateTimeTicks;

/*
 * This data was generated by C++.Net code like
 * Console::WriteLine(L"    {{ {0}, 1, 1, INT64_C({1}) }},", year, DateTime(year, 1, 1).Ticks);
 * with the DateTime constructor taking int values for year, month, and date.
 */
static const DotNetDateTimeTicks dotNetDateTimeTicks[]={
    /* year, month, day, ticks */
    { 100, 1, 1, INT64_C(31241376000000000) },
    { 100, 3, 1, INT64_C(31292352000000000) },
    { 200, 1, 1, INT64_C(62798112000000000) },
    { 200, 3, 1, INT64_C(62849088000000000) },
    { 300, 1, 1, INT64_C(94354848000000000) },
    { 300, 3, 1, INT64_C(94405824000000000) },
    { 400, 1, 1, INT64_C(125911584000000000) },
    { 400, 3, 1, INT64_C(125963424000000000) },
    { 500, 1, 1, INT64_C(157469184000000000) },
    { 500, 3, 1, INT64_C(157520160000000000) },
    { 600, 1, 1, INT64_C(189025920000000000) },
    { 600, 3, 1, INT64_C(189076896000000000) },
    { 700, 1, 1, INT64_C(220582656000000000) },
    { 700, 3, 1, INT64_C(220633632000000000) },
    { 800, 1, 1, INT64_C(252139392000000000) },
    { 800, 3, 1, INT64_C(252191232000000000) },
    { 900, 1, 1, INT64_C(283696992000000000) },
    { 900, 3, 1, INT64_C(283747968000000000) },
    { 1000, 1, 1, INT64_C(315253728000000000) },
    { 1000, 3, 1, INT64_C(315304704000000000) },
    { 1100, 1, 1, INT64_C(346810464000000000) },
    { 1100, 3, 1, INT64_C(346861440000000000) },
    { 1200, 1, 1, INT64_C(378367200000000000) },
    { 1200, 3, 1, INT64_C(378419040000000000) },
    { 1300, 1, 1, INT64_C(409924800000000000) },
    { 1300, 3, 1, INT64_C(409975776000000000) },
    { 1400, 1, 1, INT64_C(441481536000000000) },
    { 1400, 3, 1, INT64_C(441532512000000000) },
    { 1500, 1, 1, INT64_C(473038272000000000) },
    { 1500, 3, 1, INT64_C(473089248000000000) },
    { 1600, 1, 1, INT64_C(504595008000000000) },
    { 1600, 3, 1, INT64_C(504646848000000000) },
    { 1700, 1, 1, INT64_C(536152608000000000) },
    { 1700, 3, 1, INT64_C(536203584000000000) },
    { 1800, 1, 1, INT64_C(567709344000000000) },
    { 1800, 3, 1, INT64_C(567760320000000000) },
    { 1900, 1, 1, INT64_C(599266080000000000) },
    { 1900, 3, 1, INT64_C(599317056000000000) },
    { 2000, 1, 1, INT64_C(630822816000000000) },
    { 2000, 3, 1, INT64_C(630874656000000000) },
    { 2100, 1, 1, INT64_C(662380416000000000) },
    { 2100, 3, 1, INT64_C(662431392000000000) },
    { 2200, 1, 1, INT64_C(693937152000000000) },
    { 2200, 3, 1, INT64_C(693988128000000000) },
    { 2300, 1, 1, INT64_C(725493888000000000) },
    { 2300, 3, 1, INT64_C(725544864000000000) },
    { 2400, 1, 1, INT64_C(757050624000000000) },
    { 2400, 3, 1, INT64_C(757102464000000000) },
    { 2500, 1, 1, INT64_C(788608224000000000) },
    { 2500, 3, 1, INT64_C(788659200000000000) },
    { 2600, 1, 1, INT64_C(820164960000000000) },
    { 2600, 3, 1, INT64_C(820215936000000000) },
    { 2700, 1, 1, INT64_C(851721696000000000) },
    { 2700, 3, 1, INT64_C(851772672000000000) },
    { 2800, 1, 1, INT64_C(883278432000000000) },
    { 2800, 3, 1, INT64_C(883330272000000000) },
    { 2900, 1, 1, INT64_C(914836032000000000) },
    { 2900, 3, 1, INT64_C(914887008000000000) },
    { 3000, 1, 1, INT64_C(946392768000000000) },
    { 3000, 3, 1, INT64_C(946443744000000000) },
    { 1, 1, 1, INT64_C(0) },
    { 1601, 1, 1, INT64_C(504911232000000000) },
    { 1899, 12, 31, INT64_C(599265216000000000) },
    { 1904, 1, 1, INT64_C(600527520000000000) },
    { 1970, 1, 1, INT64_C(621355968000000000) },
    { 2001, 1, 1, INT64_C(631139040000000000) },
    { 9900, 3, 1, INT64_C(3123873216000000000) },
    { 9999, 12, 31, INT64_C(3155378112000000000) }
};

/*
 * ICU's Universal Time Scale is designed to be tick-for-tick compatible with
 * .Net System.DateTime. Verify that this is so for the
 * .Net-supported date range (years 1-9999 AD).
 * This requires a proleptic Gregorian calendar because that's what .Net uses.
 * Proleptic: No Julian/Gregorian switchover, or a switchover before
 * any date that we test, that is, before 0001 AD.
 */
static void
TestDotNet() {
    static const UChar utc[] = { 0x45, 0x74, 0x63, 0x2f, 0x47, 0x4d, 0x54, 0 }; /* "Etc/GMT" */
    const int32_t dayMillis = 86400 * INT64_C(1000);    /* 1 day = 86400 seconds */
    const int64_t dayTicks = 86400 * INT64_C(10000000);
    const DotNetDateTimeTicks *dt;
    UCalendar *cal;
    UErrorCode errorCode;
    UDate icuDate;
    int64_t ticks, millis;
    int32_t i;

    /* Open a proleptic Gregorian calendar. */
    errorCode = U_ZERO_ERROR;
    cal = ucal_open(utc, -1, "", UCAL_GREGORIAN, &errorCode);
    ucal_setGregorianChange(cal, -1000000 * (dayMillis * (UDate)1), &errorCode);
    if(U_FAILURE(errorCode)) {
        log_data_err("ucal_open(UTC/proleptic Gregorian) failed: %s - (Are you missing data?)\n", u_errorName(errorCode));
        ucal_close(cal);
        return;
    }
    for(i = 0; i < LENGTHOF(dotNetDateTimeTicks); ++i) {
        /* Test conversion from .Net/Universal time to ICU time. */
        dt = dotNetDateTimeTicks + i;
        millis = utmscale_toInt64(dt->ticks, UDTS_ICU4C_TIME, &errorCode);
        ucal_clear(cal);
        ucal_setDate(cal, dt->year, dt->month - 1, dt->day, &errorCode); /* Java & ICU use January = month 0. */
        icuDate = ucal_getMillis(cal, &errorCode);
        if(millis != icuDate) {
            /* Print days not millis to stay within printf() range. */
            log_err("utmscale_toInt64(ticks[%d], ICU4C)=%dd != %dd=ucal_getMillis(%04d-%02d-%02d)\n",
                    (int)i, (int)(millis/dayMillis), (int)(icuDate/dayMillis), (int)dt->year, (int)dt->month, (int)dt->day);
        }

        /* Test conversion from ICU time to .Net/Universal time. */
        ticks = utmscale_fromInt64((int64_t)icuDate, UDTS_ICU4C_TIME, &errorCode);
        if(ticks != dt->ticks) {
            /* Print days not ticks to stay within printf() range. */
            log_err("utmscale_fromInt64(date[%d], ICU4C)=%dd != %dd=.Net System.DateTime(%04d-%02d-%02d).Ticks\n",
                    (int)i, (int)(ticks/dayTicks), (int)(dt->ticks/dayTicks), (int)dt->year, (int)dt->month, (int)dt->day);
        }
    }

    ucal_close(cal);
}

#endif /* #if !UCONFIG_NO_FORMATTING */
