/********************************************************************
 * COPYRIGHT:
 * Copyright (c) 2005-2009, International Business Machines Corporation and
 * others. All Rights Reserved.
 ********************************************************************/
#include "unicode/utypes.h"

#if !UCONFIG_NO_FORMATTING
#include "unicode/unum.h"
#include "unicode/ucurr.h"
#include "unicode/ustring.h"
#include "cintltst.h"
#include "cstring.h"

static void expectInList(const char *isoCurrency, uint32_t currencyType, UBool isExpected) {
    UErrorCode status = U_ZERO_ERROR;
    const char *foundCurrency = NULL;
    const char *currentCurrency;
    UEnumeration *en = ucurr_openISOCurrencies(currencyType, &status);
    if (U_FAILURE(status)) {
       log_err("Error: ucurr_openISOCurrencies returned %s\n", myErrorName(status));
       return;
    }

    while ((currentCurrency = uenum_next(en, NULL, &status)) != NULL) {
        if (strcmp(isoCurrency, currentCurrency) == 0) {
            foundCurrency = currentCurrency;
            break;
        }
    }

    if ((foundCurrency != NULL) != isExpected) {
       log_err("Error: could not find %s as expected. isExpected = %s type=0x%X\n",
           isoCurrency, isExpected ? "TRUE" : "FALSE", currencyType);
    }
    uenum_close(en);
}

static void TestEnumList(void) {
    expectInList("ADP", UCURR_ALL, TRUE); /* First in list */
    expectInList("ZWD", UCURR_ALL, TRUE); /* Last in list */

    expectInList("USD", UCURR_ALL, TRUE);
    expectInList("USD", UCURR_COMMON, TRUE);
    expectInList("USD", UCURR_UNCOMMON, FALSE);
    expectInList("USD", UCURR_DEPRECATED, FALSE);
    expectInList("USD", UCURR_NON_DEPRECATED, TRUE);
    expectInList("USD", UCURR_COMMON|UCURR_DEPRECATED, FALSE);
    expectInList("USD", UCURR_COMMON|UCURR_NON_DEPRECATED, TRUE);
    expectInList("USD", UCURR_UNCOMMON|UCURR_DEPRECATED, FALSE);
    expectInList("USD", UCURR_UNCOMMON|UCURR_NON_DEPRECATED, FALSE);

    expectInList("USN", UCURR_ALL, TRUE);
    expectInList("USN", UCURR_COMMON, FALSE);
    expectInList("USN", UCURR_UNCOMMON, TRUE);
    expectInList("USN", UCURR_DEPRECATED, FALSE);
    expectInList("USN", UCURR_NON_DEPRECATED, TRUE);
    expectInList("USN", UCURR_COMMON|UCURR_DEPRECATED, FALSE);
    expectInList("USN", UCURR_COMMON|UCURR_NON_DEPRECATED, FALSE);
    expectInList("USN", UCURR_UNCOMMON|UCURR_DEPRECATED, FALSE);
    expectInList("USN", UCURR_UNCOMMON|UCURR_NON_DEPRECATED, TRUE);

    expectInList("DEM", UCURR_ALL, TRUE);
    expectInList("DEM", UCURR_COMMON, TRUE);
    expectInList("DEM", UCURR_UNCOMMON, FALSE);
    expectInList("DEM", UCURR_DEPRECATED, TRUE);
    expectInList("DEM", UCURR_NON_DEPRECATED, FALSE);
    expectInList("DEM", UCURR_COMMON|UCURR_DEPRECATED, TRUE);
    expectInList("DEM", UCURR_COMMON|UCURR_NON_DEPRECATED, FALSE);
    expectInList("DEM", UCURR_UNCOMMON|UCURR_DEPRECATED, FALSE);
    expectInList("DEM", UCURR_UNCOMMON|UCURR_NON_DEPRECATED, FALSE);

    expectInList("XEU", UCURR_ALL, TRUE);
    expectInList("XEU", UCURR_COMMON, FALSE);
    expectInList("XEU", UCURR_UNCOMMON, TRUE);
    expectInList("XEU", UCURR_DEPRECATED, TRUE);
    expectInList("XEU", UCURR_NON_DEPRECATED, FALSE);
    expectInList("XEU", UCURR_COMMON|UCURR_DEPRECATED, FALSE);
    expectInList("XEU", UCURR_COMMON|UCURR_NON_DEPRECATED, FALSE);
    expectInList("XEU", UCURR_UNCOMMON|UCURR_DEPRECATED, TRUE);
    expectInList("XEU", UCURR_UNCOMMON|UCURR_NON_DEPRECATED, FALSE);

}

static void TestEnumListReset(void) {
    UErrorCode status = U_ZERO_ERROR;
    const char *currency1;
    const char *currency2;
    UEnumeration *en = ucurr_openISOCurrencies(UCURR_ALL, &status);
    if (U_FAILURE(status)) {
       log_err("Error: ucurr_openISOCurrencies returned %s\n", myErrorName(status));
       return;
    }

    currency1 = uenum_next(en, NULL, &status);
    uenum_reset(en, &status);
    currency2 = uenum_next(en, NULL, &status);
    if (U_FAILURE(status)) {
       log_err("Error: uenum_next or uenum_reset returned %s\n", myErrorName(status));
       return;
    }
    /* The first item's pointer in the list should be the same between resets. */
    if (currency1 != currency2) {
       log_err("Error: reset doesn't work %s != %s\n", currency1, currency2);
    }
    uenum_close(en);
}

static int32_t checkItemCount(uint32_t currencyType) {
    UErrorCode status = U_ZERO_ERROR;
    int32_t originalCount, count;
    UEnumeration *en = ucurr_openISOCurrencies(currencyType, &status);
    int32_t expectedLen = 3, len;
    if (U_FAILURE(status)) {
       log_err("Error: ucurr_openISOCurrencies returned %s\n", myErrorName(status));
       return -1;
    }

    originalCount = uenum_count(en, &status);
    for (count=0;;count++) {
        const char *str = uenum_next(en, &len, &status);
        if (str == NULL || len != expectedLen || strlen(str) != expectedLen) {
            break;
        }
    }

    if (originalCount != count) {
        log_err("Error: uenum_count returned the wrong value (type = 0x%X). Got: %d Expected %d\n",
           currencyType, count, originalCount);
    }
    if (U_FAILURE(status)) {
        log_err("Error: uenum_next got an error: %s\n", u_errorName(status));
    }
    uenum_close(en);
    return count;
}

static void TestEnumListCount(void) {
    checkItemCount(UCURR_ALL);
    checkItemCount(UCURR_COMMON);
    checkItemCount(UCURR_UNCOMMON);
    checkItemCount(UCURR_DEPRECATED);
    checkItemCount(UCURR_NON_DEPRECATED);
    checkItemCount(UCURR_COMMON|UCURR_DEPRECATED);
    checkItemCount(UCURR_COMMON|UCURR_NON_DEPRECATED);
    checkItemCount(UCURR_UNCOMMON|UCURR_DEPRECATED);
    checkItemCount(UCURR_UNCOMMON|UCURR_NON_DEPRECATED);

    if (checkItemCount(UCURR_DEPRECATED|UCURR_NON_DEPRECATED) != 0) {
        log_err("Error: UCURR_DEPRECATED|UCURR_NON_DEPRECATED should return 0 items\n");
    }
    if (checkItemCount(UCURR_COMMON|UCURR_UNCOMMON) != 0) {
        log_err("Error: UCURR_DEPRECATED|UCURR_NON_DEPRECATED should return 0 items\n");
    }
}

static void TestFractionDigitOverride(void) {
    UErrorCode status = U_ZERO_ERROR;
    UNumberFormat *fmt = unum_open(UNUM_CURRENCY, NULL, 0, "hu_HU", NULL, &status);
    UChar buffer[256];
    UChar expectedBuf[256];
    const char expectedFirst[] = "123\\u00A0Ft";
    const char expectedSecond[] = "123,46\\u00A0Ft";
    const char expectedThird[] = "123,456\\u00A0Ft";
    if (U_FAILURE(status)) {
       log_data_err("Error: unum_open returned %s (Are you missing data?)\n", myErrorName(status));
       return;
    }
    /* Make sure that you can format normal fraction digits. */
    unum_formatDouble(fmt, 123.456, buffer, sizeof(buffer)/sizeof(buffer[0]), NULL, &status);
    u_unescape(expectedFirst, expectedBuf, strlen(expectedFirst)+1);
    if (u_strcmp(buffer, expectedBuf) != 0) {
       log_err("Error: unum_formatDouble didn't return %s\n", expectedFirst);
    }
    /* Make sure that you can format 2 fraction digits. */
    unum_setAttribute(fmt, UNUM_FRACTION_DIGITS, 2);
    unum_formatDouble(fmt, 123.456, buffer, sizeof(buffer)/sizeof(buffer[0]), NULL, &status);
    u_unescape(expectedSecond, expectedBuf, strlen(expectedSecond)+1);
    if (u_strcmp(buffer, expectedBuf) != 0) {
       log_err("Error: unum_formatDouble didn't return %s\n", expectedSecond);
    }
    /* Make sure that you can format more fraction digits. */
    unum_setAttribute(fmt, UNUM_FRACTION_DIGITS, 3);
    unum_formatDouble(fmt, 123.456, buffer, sizeof(buffer)/sizeof(buffer[0]), NULL, &status);
    u_unescape(expectedThird, expectedBuf, strlen(expectedThird)+1);
    if (u_strcmp(buffer, expectedBuf) != 0) {
       log_err("Error: unum_formatDouble didn't return %s\n", expectedThird);
    }
    unum_close(fmt);
}

static void TestPrefixSuffix(void) {
    int32_t	pos;
    UErrorCode status;
    double result1 = 0.0, result2 = 0.0;
    UNumberFormat* parser;
    UChar buffer[4];
    static const UChar TEST_NUMBER[] = {0x0024,0x0031,0x0032,0x002E,0x0030,0x0030,0}; /* $12.00 */
    static const UChar NEG_PREFIX[] = {0x005B,0}; /* "[" */
    static const UChar NEG_SUFFIX[] = {0x005D,0}; /* "]" */


	status = U_ZERO_ERROR;
	parser = unum_open(UNUM_CURRENCY, NULL, -1, "en_US", NULL, &status);
    if (U_FAILURE(status)) {
       log_data_err("Error: unum_open returned %s (Are you missing data?)\n", u_errorName(status));
       return;
    }

	pos = 0;
	status = U_ZERO_ERROR;
	result1 = unum_parseDoubleCurrency(parser, TEST_NUMBER, -1, &pos, buffer, &status);

	unum_setTextAttribute(parser, UNUM_NEGATIVE_SUFFIX, NEG_SUFFIX, -1, &status);
	unum_setTextAttribute(parser, UNUM_NEGATIVE_PREFIX, NEG_PREFIX, -1, &status);
    if (U_FAILURE(status)) {
       log_err("Error: unum_setTextAttribute returned %s\n", u_errorName(status));
       return;
    }

	pos = 0;
	result2 = unum_parseDoubleCurrency(parser, TEST_NUMBER, -1, &pos, buffer, &status);
    if (result1 != result2 || U_FAILURE(status)) {
       log_err("Error: unum_parseDoubleCurrency didn't return the same value for same string %f %f %s\n",
           result1, result2, u_errorName(status));
    }
    unum_close(parser);
}

void addCurrencyTest(TestNode** root);

#define TESTCASE(x) addTest(root, &x, "tsformat/currtest/" #x)

void addCurrencyTest(TestNode** root)
{
    TESTCASE(TestEnumList);
    TESTCASE(TestEnumListReset);
    TESTCASE(TestEnumListCount);
    TESTCASE(TestFractionDigitOverride);
    TESTCASE(TestPrefixSuffix);
}

#endif /* #if !UCONFIG_NO_FORMATTING */
