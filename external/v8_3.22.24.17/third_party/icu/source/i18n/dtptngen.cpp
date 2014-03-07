/*
*******************************************************************************
* Copyright (C) 2007-2010, International Business Machines Corporation and
* others. All Rights Reserved.
*******************************************************************************
*
* File DTPTNGEN.CPP
*
*******************************************************************************
*/

#include "unicode/utypes.h"
#if !UCONFIG_NO_FORMATTING

#include "unicode/datefmt.h"
#include "unicode/decimfmt.h"
#include "unicode/dtfmtsym.h"
#include "unicode/dtptngen.h"
#include "unicode/msgfmt.h"
#include "unicode/smpdtfmt.h"
#include "unicode/udat.h"
#include "unicode/udatpg.h"
#include "unicode/uniset.h"
#include "unicode/uloc.h"
#include "unicode/ures.h"
#include "unicode/ustring.h"
#include "unicode/rep.h"
#include "cpputils.h"
#include "ucln_in.h"
#include "mutex.h"
#include "cmemory.h"
#include "cstring.h"
#include "locbased.h"
#include "gregoimp.h"
#include "hash.h"
#include "uresimp.h"
#include "dtptngen_impl.h"

#define LENGTHOF(array) (int32_t)(sizeof(array)/sizeof((array)[0]))

#if U_CHARSET_FAMILY==U_EBCDIC_FAMILY
/**
 * If we are on EBCDIC, use an iterator which will
 * traverse the bundles in ASCII order.
 */
#define U_USE_ASCII_BUNDLE_ITERATOR
#define U_SORT_ASCII_BUNDLE_ITERATOR
#endif

#if defined(U_USE_ASCII_BUNDLE_ITERATOR)

#include "unicode/ustring.h"
#include "uarrsort.h"

struct UResAEntry {
    UChar *key;
    UResourceBundle *item;
};

struct UResourceBundleAIterator {
    UResourceBundle  *bund;
    UResAEntry *entries;
    int32_t num;
    int32_t cursor;
};

/* Must be C linkage to pass function pointer to the sort function */

#if !defined (OS390) && !defined (OS400)
extern "C"
#endif
static int32_t U_CALLCONV
ures_a_codepointSort(const void *context, const void *left, const void *right) {
    //CompareContext *cmp=(CompareContext *)context;
    return u_strcmp(((const UResAEntry *)left)->key,
                    ((const UResAEntry *)right)->key);
}


static void ures_a_open(UResourceBundleAIterator *aiter, UResourceBundle *bund, UErrorCode *status) {
    if(U_FAILURE(*status)) {
        return;
    }
    aiter->bund = bund;
    aiter->num = ures_getSize(aiter->bund);
    aiter->cursor = 0;
#if !defined(U_SORT_ASCII_BUNDLE_ITERATOR)
    aiter->entries = NULL;
#else
    aiter->entries = (UResAEntry*)uprv_malloc(sizeof(UResAEntry)*aiter->num);
    for(int i=0;i<aiter->num;i++) {
        aiter->entries[i].item = ures_getByIndex(aiter->bund, i, NULL, status);
        const char *akey = ures_getKey(aiter->entries[i].item);
        int32_t len = uprv_strlen(akey)+1;
        aiter->entries[i].key = (UChar*)uprv_malloc(len*sizeof(UChar));
        u_charsToUChars(akey, aiter->entries[i].key, len);
    }
    uprv_sortArray(aiter->entries, aiter->num, sizeof(UResAEntry), ures_a_codepointSort, NULL, TRUE, status);
#endif
}

static void ures_a_close(UResourceBundleAIterator *aiter) {
#if defined(U_SORT_ASCII_BUNDLE_ITERATOR)
    for(int i=0;i<aiter->num;i++) {
        uprv_free(aiter->entries[i].key);
        ures_close(aiter->entries[i].item);
    }
#endif
}

static const UChar *ures_a_getNextString(UResourceBundleAIterator *aiter, int32_t *len, const char **key, UErrorCode *err) {
#if !defined(U_SORT_ASCII_BUNDLE_ITERATOR)
    return ures_getNextString(aiter->bund, len, key, err);
#else
    if(U_FAILURE(*err)) return NULL;
    UResourceBundle *item = aiter->entries[aiter->cursor].item;
    const UChar* ret = ures_getString(item, len, err);
    *key = ures_getKey(item);
    aiter->cursor++;
    return ret;
#endif
}


#endif


U_NAMESPACE_BEGIN


// *****************************************************************************
// class DateTimePatternGenerator
// *****************************************************************************
static const UChar Canonical_Items[] = {
    // GyQMwWEdDFHmsSv
    CAP_G, LOW_Y, CAP_Q, CAP_M, LOW_W, CAP_W, CAP_E, LOW_D, CAP_D, CAP_F,
    CAP_H, LOW_M, LOW_S, CAP_S, LOW_V, 0
};

static const dtTypeElem dtTypes[] = {
    // patternChar, field, type, minLen, weight
    {CAP_G, UDATPG_ERA_FIELD, DT_SHORT, 1, 3,},
    {CAP_G, UDATPG_ERA_FIELD, DT_LONG, 4, 0},
    {LOW_Y, UDATPG_YEAR_FIELD, DT_NUMERIC, 1, 20},
    {CAP_Y, UDATPG_YEAR_FIELD, DT_NUMERIC + DT_DELTA, 1, 20},
    {LOW_U, UDATPG_YEAR_FIELD, DT_NUMERIC + 2*DT_DELTA, 1, 20},
    {CAP_Q, UDATPG_QUARTER_FIELD, DT_NUMERIC, 1, 2},
    {CAP_Q, UDATPG_QUARTER_FIELD, DT_SHORT, 3, 0},
    {CAP_Q, UDATPG_QUARTER_FIELD, DT_LONG, 4, 0},
    {LOW_Q, UDATPG_QUARTER_FIELD, DT_NUMERIC + DT_DELTA, 1, 2},
    {LOW_Q, UDATPG_QUARTER_FIELD, DT_SHORT + DT_DELTA, 3, 0},
    {LOW_Q, UDATPG_QUARTER_FIELD, DT_LONG + DT_DELTA, 4, 0},
    {CAP_M, UDATPG_MONTH_FIELD, DT_NUMERIC, 1, 2},
    {CAP_M, UDATPG_MONTH_FIELD, DT_SHORT, 3, 0},
    {CAP_M, UDATPG_MONTH_FIELD, DT_LONG, 4, 0},
    {CAP_M, UDATPG_MONTH_FIELD, DT_NARROW, 5, 0},
    {CAP_L, UDATPG_MONTH_FIELD, DT_NUMERIC + DT_DELTA, 1, 2},
    {CAP_L, UDATPG_MONTH_FIELD, DT_SHORT - DT_DELTA, 3, 0},
    {CAP_L, UDATPG_MONTH_FIELD, DT_LONG - DT_DELTA, 4, 0},
    {CAP_L, UDATPG_MONTH_FIELD, DT_NARROW - DT_DELTA, 5, 0},
    {LOW_W, UDATPG_WEEK_OF_YEAR_FIELD, DT_NUMERIC, 1, 2},
    {CAP_W, UDATPG_WEEK_OF_MONTH_FIELD, DT_NUMERIC + DT_DELTA, 1, 0},
    {CAP_E, UDATPG_WEEKDAY_FIELD, DT_SHORT, 1, 3},
    {CAP_E, UDATPG_WEEKDAY_FIELD, DT_LONG, 4, 0},
    {CAP_E, UDATPG_WEEKDAY_FIELD, DT_NARROW, 5, 0},
    {LOW_C, UDATPG_WEEKDAY_FIELD, DT_NUMERIC + 2*DT_DELTA, 1, 2},
    {LOW_C, UDATPG_WEEKDAY_FIELD, DT_SHORT - 2*DT_DELTA, 3, 0},
    {LOW_C, UDATPG_WEEKDAY_FIELD, DT_LONG - 2*DT_DELTA, 4, 0},
    {LOW_C, UDATPG_WEEKDAY_FIELD, DT_NARROW - 2*DT_DELTA, 5, 0},
    {LOW_E, UDATPG_WEEKDAY_FIELD, DT_NUMERIC + DT_DELTA, 1, 2}, // LOW_E is currently not used in CLDR data, should not be canonical
    {LOW_E, UDATPG_WEEKDAY_FIELD, DT_SHORT - DT_DELTA, 3, 0},
    {LOW_E, UDATPG_WEEKDAY_FIELD, DT_LONG - DT_DELTA, 4, 0},
    {LOW_E, UDATPG_WEEKDAY_FIELD, DT_NARROW - DT_DELTA, 5, 0},
    {LOW_D, UDATPG_DAY_FIELD, DT_NUMERIC, 1, 2},
    {CAP_D, UDATPG_DAY_OF_YEAR_FIELD, DT_NUMERIC + DT_DELTA, 1, 3},
    {CAP_F, UDATPG_DAY_OF_WEEK_IN_MONTH_FIELD, DT_NUMERIC + 2*DT_DELTA, 1, 0},
    {LOW_G, UDATPG_DAY_FIELD, DT_NUMERIC + 3*DT_DELTA, 1, 20}, // really internal use, so we don't care
    {LOW_A, UDATPG_DAYPERIOD_FIELD, DT_SHORT, 1, 0},
    {CAP_H, UDATPG_HOUR_FIELD, DT_NUMERIC + 10*DT_DELTA, 1, 2}, // 24 hour
    {LOW_K, UDATPG_HOUR_FIELD, DT_NUMERIC + 11*DT_DELTA, 1, 2},
    {LOW_H, UDATPG_HOUR_FIELD, DT_NUMERIC, 1, 2}, // 12 hour
    {LOW_K, UDATPG_HOUR_FIELD, DT_NUMERIC + DT_DELTA, 1, 2},
    {LOW_M, UDATPG_MINUTE_FIELD, DT_NUMERIC, 1, 2},
    {LOW_S, UDATPG_SECOND_FIELD, DT_NUMERIC, 1, 2},
    {CAP_S, UDATPG_FRACTIONAL_SECOND_FIELD, DT_NUMERIC + DT_DELTA, 1, 1000},
    {CAP_A, UDATPG_SECOND_FIELD, DT_NUMERIC + 2*DT_DELTA, 1, 1000},
    {LOW_V, UDATPG_ZONE_FIELD, DT_SHORT - 2*DT_DELTA, 1, 0},
    {LOW_V, UDATPG_ZONE_FIELD, DT_LONG - 2*DT_DELTA, 4, 0},
    {LOW_Z, UDATPG_ZONE_FIELD, DT_SHORT, 1, 3},
    {LOW_Z, UDATPG_ZONE_FIELD, DT_LONG, 4, 0},
    {CAP_Z, UDATPG_ZONE_FIELD, DT_SHORT - DT_DELTA, 1, 3},
    {CAP_Z, UDATPG_ZONE_FIELD, DT_LONG - DT_DELTA, 4, 0},
    {CAP_V, UDATPG_ZONE_FIELD, DT_SHORT - DT_DELTA, 1, 3},
    {CAP_V, UDATPG_ZONE_FIELD, DT_LONG - DT_DELTA, 4, 0},
    {0, UDATPG_FIELD_COUNT, 0, 0, 0} , // last row of dtTypes[]
 };

static const char* const CLDR_FIELD_APPEND[] = {
    "Era", "Year", "Quarter", "Month", "Week", "*", "Day-Of-Week", "Day", "*", "*", "*",
    "Hour", "Minute", "Second", "*", "Timezone"
};

static const char* const CLDR_FIELD_NAME[] = {
    "era", "year", "quarter", "month", "week", "*", "weekday", "day", "*", "*", "dayperiod",
    "hour", "minute", "second", "*", "zone"
};

static const char* const Resource_Fields[] = {
    "day", "dayperiod", "era", "hour", "minute", "month", "second", "week",
    "weekday", "year", "zone", "quarter" };

// For appendItems
static const UChar UDATPG_ItemFormat[]= {0x7B, 0x30, 0x7D, 0x20, 0x251C, 0x7B, 0x32, 0x7D, 0x3A,
    0x20, 0x7B, 0x31, 0x7D, 0x2524, 0};  // {0} \u251C{2}: {1}\u2524

static const UChar repeatedPatterns[6]={CAP_G, CAP_E, LOW_Z, LOW_V, CAP_Q, 0}; // "GEzvQ"

static const char DT_DateTimePatternsTag[]="DateTimePatterns";
static const char DT_DateTimeCalendarTag[]="calendar";
static const char DT_DateTimeGregorianTag[]="gregorian";
static const char DT_DateTimeAppendItemsTag[]="appendItems";
static const char DT_DateTimeFieldsTag[]="fields";
static const char DT_DateTimeAvailableFormatsTag[]="availableFormats";
//static const UnicodeString repeatedPattern=UnicodeString(repeatedPatterns);

UOBJECT_DEFINE_RTTI_IMPLEMENTATION(DateTimePatternGenerator)
UOBJECT_DEFINE_RTTI_IMPLEMENTATION(DTSkeletonEnumeration)
UOBJECT_DEFINE_RTTI_IMPLEMENTATION(DTRedundantEnumeration)

DateTimePatternGenerator*  U_EXPORT2
DateTimePatternGenerator::createInstance(UErrorCode& status) {
    return createInstance(Locale::getDefault(), status);
}

DateTimePatternGenerator* U_EXPORT2
DateTimePatternGenerator::createInstance(const Locale& locale, UErrorCode& status) {
    DateTimePatternGenerator *result = new DateTimePatternGenerator(locale, status);
    if (result == NULL) {
        status = U_MEMORY_ALLOCATION_ERROR;
    }
    if (U_FAILURE(status)) {
        delete result;
        result = NULL;
    }
    return result;
}

DateTimePatternGenerator*  U_EXPORT2
DateTimePatternGenerator::createEmptyInstance(UErrorCode& status) {
    DateTimePatternGenerator *result = new DateTimePatternGenerator(status);
    if (result == NULL) {
        status = U_MEMORY_ALLOCATION_ERROR;
    }
    if (U_FAILURE(status)) {
        delete result;
        result = NULL;
    }
    return result;
}

DateTimePatternGenerator::DateTimePatternGenerator(UErrorCode &status) :
    skipMatcher(NULL),
    fAvailableFormatKeyHash(NULL)
{
    fp = new FormatParser();
    dtMatcher = new DateTimeMatcher();
    distanceInfo = new DistanceInfo();
    patternMap = new PatternMap();
    if (fp == NULL || dtMatcher == NULL || distanceInfo == NULL || patternMap == NULL) {
        status = U_MEMORY_ALLOCATION_ERROR;
    }
}

DateTimePatternGenerator::DateTimePatternGenerator(const Locale& locale, UErrorCode &status) :
    skipMatcher(NULL),
    fAvailableFormatKeyHash(NULL)
{
    fp = new FormatParser();
    dtMatcher = new DateTimeMatcher();
    distanceInfo = new DistanceInfo();
    patternMap = new PatternMap();
    if (fp == NULL || dtMatcher == NULL || distanceInfo == NULL || patternMap == NULL) {
        status = U_MEMORY_ALLOCATION_ERROR;
    }
    else {
        initData(locale, status);
    }
}

DateTimePatternGenerator::DateTimePatternGenerator(const DateTimePatternGenerator& other) :
    UObject(),
    skipMatcher(NULL),
    fAvailableFormatKeyHash(NULL)
{
    fp = new FormatParser();
    dtMatcher = new DateTimeMatcher();
    distanceInfo = new DistanceInfo();
    patternMap = new PatternMap();
    *this=other;
}

DateTimePatternGenerator&
DateTimePatternGenerator::operator=(const DateTimePatternGenerator& other) {
    pLocale = other.pLocale;
    fDefaultHourFormatChar = other.fDefaultHourFormatChar;
    *fp = *(other.fp);
    dtMatcher->copyFrom(other.dtMatcher->skeleton);
    *distanceInfo = *(other.distanceInfo);
    dateTimeFormat = other.dateTimeFormat;
    decimal = other.decimal;
    // NUL-terminate for the C API.
    dateTimeFormat.getTerminatedBuffer();
    decimal.getTerminatedBuffer();
    delete skipMatcher;
    if ( other.skipMatcher == NULL ) {
        skipMatcher = NULL;
    }
    else {
        skipMatcher = new DateTimeMatcher(*other.skipMatcher);
    }
    for (int32_t i=0; i< UDATPG_FIELD_COUNT; ++i ) {
        appendItemFormats[i] = other.appendItemFormats[i];
        appendItemNames[i] = other.appendItemNames[i];
        // NUL-terminate for the C API.
        appendItemFormats[i].getTerminatedBuffer();
        appendItemNames[i].getTerminatedBuffer();
    }
    UErrorCode status = U_ZERO_ERROR;
    patternMap->copyFrom(*other.patternMap, status);
    copyHashtable(other.fAvailableFormatKeyHash, status);
    return *this;
}


UBool
DateTimePatternGenerator::operator==(const DateTimePatternGenerator& other) const {
    if (this == &other) {
        return TRUE;
    }
    if ((pLocale==other.pLocale) && (patternMap->equals(*other.patternMap)) &&
        (dateTimeFormat==other.dateTimeFormat) && (decimal==other.decimal)) {
        for ( int32_t i=0 ; i<UDATPG_FIELD_COUNT; ++i ) {
           if ((appendItemFormats[i] != other.appendItemFormats[i]) ||
               (appendItemNames[i] != other.appendItemNames[i]) ) {
               return FALSE;
           }
        }
        return TRUE;
    }
    else {
        return FALSE;
    }
}

UBool
DateTimePatternGenerator::operator!=(const DateTimePatternGenerator& other) const {
    return  !operator==(other);
}

DateTimePatternGenerator::~DateTimePatternGenerator() {
    if (fAvailableFormatKeyHash!=NULL) {
        delete fAvailableFormatKeyHash;
    }

    if (fp != NULL) delete fp;
    if (dtMatcher != NULL) delete dtMatcher;
    if (distanceInfo != NULL) delete distanceInfo;
    if (patternMap != NULL) delete patternMap;
    if (skipMatcher != NULL) delete skipMatcher;
}

void
DateTimePatternGenerator::initData(const Locale& locale, UErrorCode &status) {
    //const char *baseLangName = locale.getBaseName(); // unused

    skipMatcher = NULL;
    fAvailableFormatKeyHash=NULL;
    addCanonicalItems();
    addICUPatterns(locale, status);
    if (U_FAILURE(status)) {
        return;
    }
    addCLDRData(locale, status);
    setDateTimeFromCalendar(locale, status);
    setDecimalSymbols(locale, status);
} // DateTimePatternGenerator::initData

UnicodeString
DateTimePatternGenerator::getSkeleton(const UnicodeString& pattern, UErrorCode&
/*status*/) {
    dtMatcher->set(pattern, fp);
    return dtMatcher->getSkeletonPtr()->getSkeleton();
}

UnicodeString
DateTimePatternGenerator::getBaseSkeleton(const UnicodeString& pattern, UErrorCode& /*status*/) {
    dtMatcher->set(pattern, fp);
    return dtMatcher->getSkeletonPtr()->getBaseSkeleton();
}

void
DateTimePatternGenerator::addICUPatterns(const Locale& locale, UErrorCode& status) {
    UnicodeString dfPattern;
    UnicodeString conflictingString;
    UDateTimePatternConflict conflictingStatus;
    DateFormat* df;

    if (U_FAILURE(status)) {
        return;
    }

    // Load with ICU patterns
    for (int32_t i=DateFormat::kFull; i<=DateFormat::kShort; i++) {
        DateFormat::EStyle style = (DateFormat::EStyle)i;
        df = DateFormat::createDateInstance(style, locale);
        SimpleDateFormat* sdf;
        if (df != NULL && (sdf = dynamic_cast<SimpleDateFormat*>(df)) != NULL) {
            conflictingStatus = addPattern(sdf->toPattern(dfPattern), FALSE, conflictingString, status);
        }
        // TODO Maybe we should return an error when the date format isn't simple.
        delete df;
        if (U_FAILURE(status)) {
            return;
        }

        df = DateFormat::createTimeInstance(style, locale);
        if (df != NULL && (sdf = dynamic_cast<SimpleDateFormat*>(df)) != NULL) {
            conflictingStatus = addPattern(sdf->toPattern(dfPattern), FALSE, conflictingString, status);
            // HACK for hh:ss
            if ( i==DateFormat::kMedium ) {
                hackPattern = dfPattern;
            }
        }
        // TODO Maybe we should return an error when the date format isn't simple.
        delete df;
        if (U_FAILURE(status)) {
            return;
        }
    }
}

void
DateTimePatternGenerator::hackTimes(const UnicodeString& hackPattern, UErrorCode& status)  {
    UDateTimePatternConflict conflictingStatus;
    UnicodeString conflictingString;

    fp->set(hackPattern);
    UnicodeString mmss;
    UBool gotMm=FALSE;
    for (int32_t i=0; i<fp->itemNumber; ++i) {
        UnicodeString field = fp->items[i];
        if ( fp->isQuoteLiteral(field) ) {
            if ( gotMm ) {
               UnicodeString quoteLiteral;
               fp->getQuoteLiteral(quoteLiteral, &i);
               mmss += quoteLiteral;
            }
        }
        else {
            if (fp->isPatternSeparator(field) && gotMm) {
                mmss+=field;
            }
            else {
                UChar ch=field.charAt(0);
                if (ch==LOW_M) {
                    gotMm=TRUE;
                    mmss+=field;
                }
                else {
                    if (ch==LOW_S) {
                        if (!gotMm) {
                            break;
                        }
                        mmss+= field;
                        conflictingStatus = addPattern(mmss, FALSE, conflictingString, status);
                        break;
                    }
                    else {
                        if (gotMm || ch==LOW_Z || ch==CAP_Z || ch==LOW_V || ch==CAP_V) {
                            break;
                        }
                    }
                }
            }
        }
    }
}

#define ULOC_LOCALE_IDENTIFIER_CAPACITY (ULOC_FULLNAME_CAPACITY + 1 + ULOC_KEYWORD_AND_VALUES_CAPACITY)

static const UChar hourFormatChars[] = { CAP_H, LOW_H, CAP_K, LOW_K, 0 }; // HhKk, the hour format characters

void
DateTimePatternGenerator::addCLDRData(const Locale& locale, UErrorCode& err) {
    UResourceBundle *rb, *calTypeBundle, *calBundle;
    UResourceBundle *patBundle, *fieldBundle, *fBundle;
    UnicodeString rbPattern, value, field;
    UnicodeString conflictingPattern;
    UDateTimePatternConflict conflictingStatus;
    const char *key=NULL;
    int32_t i;

    UnicodeString defaultItemFormat(TRUE, UDATPG_ItemFormat, LENGTHOF(UDATPG_ItemFormat)-1);  // Read-only alias.

    err = U_ZERO_ERROR;
    
    fDefaultHourFormatChar = 0;
    for (i=0; i<UDATPG_FIELD_COUNT; ++i ) {
        appendItemNames[i]=CAP_F;
        if (i<10) {
            appendItemNames[i]+=(UChar)(i+0x30);
        }
        else {
            appendItemNames[i]+=(UChar)0x31;
            appendItemNames[i]+=(UChar)(i-10 + 0x30);
        }
        // NUL-terminate for the C API.
        appendItemNames[i].getTerminatedBuffer();
    }

    rb = ures_open(NULL, locale.getName(), &err);
    if (rb == NULL || U_FAILURE(err)) {
        return;
    }
    const char *curLocaleName=ures_getLocaleByType(rb, ULOC_ACTUAL_LOCALE, &err);
    const char * calendarTypeToUse = DT_DateTimeGregorianTag; // initial default
    char         calendarType[ULOC_KEYWORDS_CAPACITY]; // to be filled in with the type to use, if all goes well
    if ( U_SUCCESS(err) ) {
        char    localeWithCalendarKey[ULOC_LOCALE_IDENTIFIER_CAPACITY];
        // obtain a locale that always has the calendar key value that should be used
        (void)ures_getFunctionalEquivalent(localeWithCalendarKey, ULOC_LOCALE_IDENTIFIER_CAPACITY, NULL,
                                            "calendar", "calendar", locale.getName(), NULL, FALSE, &err);
        localeWithCalendarKey[ULOC_LOCALE_IDENTIFIER_CAPACITY-1] = 0; // ensure null termination
        // now get the calendar key value from that locale
        int32_t calendarTypeLen = uloc_getKeywordValue(localeWithCalendarKey, "calendar", calendarType, ULOC_KEYWORDS_CAPACITY, &err);
        if (U_SUCCESS(err) && calendarTypeLen < ULOC_KEYWORDS_CAPACITY) {
            calendarTypeToUse = calendarType;
        }
        err = U_ZERO_ERROR;
    }
    calBundle = ures_getByKeyWithFallback(rb, DT_DateTimeCalendarTag, NULL, &err);
    calTypeBundle = ures_getByKeyWithFallback(calBundle, calendarTypeToUse, NULL, &err);

    key=NULL;
    int32_t dtCount=0;
    patBundle = ures_getByKeyWithFallback(calTypeBundle, DT_DateTimePatternsTag, NULL, &err);
    while (U_SUCCESS(err)) {
        rbPattern = ures_getNextUnicodeString(patBundle, &key, &err);
        dtCount++;
        if (rbPattern.length()==0 ) {
            break;  // no more pattern
        }
        else {
            if (dtCount==9) {
                setDateTimeFormat(rbPattern);
            } else if (dtCount==4) { // short time format
                // set fDefaultHourFormatChar to the hour format character from this pattern
                int32_t tfIdx, tfLen = rbPattern.length();
                UBool ignoreChars = FALSE;
                for (tfIdx = 0; tfIdx < tfLen; tfIdx++) {
                    UChar tfChar = rbPattern.charAt(tfIdx);
                    if ( tfChar == SINGLE_QUOTE ) {
                        ignoreChars = !ignoreChars; // toggle (handle quoted literals & '' for single quote)
                    } else if ( !ignoreChars && u_strchr(hourFormatChars, tfChar) != NULL ) {
                        fDefaultHourFormatChar = tfChar;
                        break;
                    }
                }
            }
        }
    }
    ures_close(patBundle);

    err = U_ZERO_ERROR;
    patBundle = ures_getByKeyWithFallback(calTypeBundle, DT_DateTimeAppendItemsTag, NULL, &err);
    key=NULL;
    UnicodeString itemKey;
    while (U_SUCCESS(err)) {
        rbPattern = ures_getNextUnicodeString(patBundle, &key, &err);
        if (rbPattern.length()==0 ) {
            break;  // no more pattern
        }
        else {
            setAppendItemFormat(getAppendFormatNumber(key), rbPattern);
        }
    }
    ures_close(patBundle);

    key=NULL;
    err = U_ZERO_ERROR;
    fBundle = ures_getByKeyWithFallback(calTypeBundle, DT_DateTimeFieldsTag, NULL, &err);
    for (i=0; i<MAX_RESOURCE_FIELD; ++i) {
        err = U_ZERO_ERROR;
        patBundle = ures_getByKeyWithFallback(fBundle, Resource_Fields[i], NULL, &err);
        fieldBundle = ures_getByKeyWithFallback(patBundle, "dn", NULL, &err);
        rbPattern = ures_getNextUnicodeString(fieldBundle, &key, &err);
        ures_close(fieldBundle);
        ures_close(patBundle);
        if (rbPattern.length()==0 ) {
            continue;
        }
        else {
            setAppendItemName(getAppendNameNumber(Resource_Fields[i]), rbPattern);
        }
    }
    ures_close(fBundle);

    // add available formats
    err = U_ZERO_ERROR;
    initHashtable(err);
    patBundle = ures_getByKeyWithFallback(calTypeBundle, DT_DateTimeAvailableFormatsTag, NULL, &err);
    if (U_SUCCESS(err)) {
        int32_t numberKeys = ures_getSize(patBundle);
        int32_t len;
        const UChar *retPattern;
        key=NULL;
#if defined(U_USE_ASCII_BUNDLE_ITERATOR)
        UResourceBundleAIterator aiter;
        ures_a_open(&aiter, patBundle, &err);
#endif
        for(i=0; i<numberKeys; ++i) {
#if defined(U_USE_ASCII_BUNDLE_ITERATOR)
            retPattern=ures_a_getNextString(&aiter, &len, &key, &err);
#else
            retPattern=ures_getNextString(patBundle, &len, &key, &err);
#endif
            UnicodeString format=UnicodeString(retPattern);
            UnicodeString retKey=UnicodeString(key, -1, US_INV);
            setAvailableFormat(retKey, err);
            // Add pattern with its associated skeleton. Override any duplicate derived from std patterns,
            // but not a previous availableFormats entry:
            conflictingStatus = addPatternWithSkeleton(format, &retKey, TRUE, conflictingPattern, err);
        }
#if defined(U_USE_ASCII_BUNDLE_ITERATOR)
        ures_a_close(&aiter);
#endif
    }
    ures_close(patBundle);
    ures_close(calTypeBundle);
    ures_close(calBundle);
    ures_close(rb);

    err = U_ZERO_ERROR;
    char parentLocale[50];
    int32_t localeNameLen=0;
    uprv_strcpy(parentLocale, curLocaleName);
    while((localeNameLen=uloc_getParent(parentLocale, parentLocale, 50, &err))>=0 ) {
        rb = ures_open(NULL, parentLocale, &err);
        curLocaleName=ures_getLocaleByType(rb, ULOC_ACTUAL_LOCALE, &err);
        uprv_strcpy(parentLocale, curLocaleName);
        calBundle = ures_getByKey(rb, DT_DateTimeCalendarTag, NULL, &err);
        calTypeBundle = ures_getByKey(calBundle, calendarTypeToUse, NULL, &err);
        patBundle = ures_getByKeyWithFallback(calTypeBundle, DT_DateTimeAvailableFormatsTag, NULL, &err);
        if (U_SUCCESS(err)) {
            int32_t numberKeys = ures_getSize(patBundle);
            int32_t len;
            const UChar *retPattern;
            key=NULL;
#if defined(U_USE_ASCII_BUNDLE_ITERATOR)
            UResourceBundleAIterator aiter;
            ures_a_open(&aiter, patBundle, &err);
#endif
            for(i=0; i<numberKeys; ++i) {
#if defined(U_USE_ASCII_BUNDLE_ITERATOR)
                retPattern=ures_a_getNextString(&aiter, &len, &key, &err);
#else
                retPattern=ures_getNextString(patBundle, &len, &key, &err);
#endif
                UnicodeString format=UnicodeString(retPattern);
                UnicodeString retKey=UnicodeString(key, -1, US_INV);
                if ( !isAvailableFormatSet(retKey) ) {
                    setAvailableFormat(retKey, err);
                    // Add pattern with its associated skeleton. Override any duplicate derived from std patterns,
                    // but not a previous availableFormats entry:
                    conflictingStatus = addPatternWithSkeleton(format, &retKey, TRUE, conflictingPattern, err);
                }
            }
#if defined(U_USE_ASCII_BUNDLE_ITERATOR)
            ures_a_close(&aiter);
#endif
        }
        err = U_ZERO_ERROR; // reset; if this locale lacks the necessary data, need to keep checking up to root.
        ures_close(patBundle);
        ures_close(calTypeBundle);
        ures_close(calBundle);
        ures_close(rb);
        if (localeNameLen==0) {
            break;
        }
    }

    if (hackPattern.length()>0) {
        hackTimes(hackPattern, err);
    }
}

void
DateTimePatternGenerator::initHashtable(UErrorCode& err) {
    if (fAvailableFormatKeyHash!=NULL) {
        return;
    }
    if ((fAvailableFormatKeyHash = new Hashtable(FALSE, err))==NULL) {
        err=U_MEMORY_ALLOCATION_ERROR;
        return;
    }
}


void
DateTimePatternGenerator::setAppendItemFormat(UDateTimePatternField field, const UnicodeString& value) {
    appendItemFormats[field] = value;
    // NUL-terminate for the C API.
    appendItemFormats[field].getTerminatedBuffer();
}

const UnicodeString&
DateTimePatternGenerator::getAppendItemFormat(UDateTimePatternField field) const {
    return appendItemFormats[field];
}

void
DateTimePatternGenerator::setAppendItemName(UDateTimePatternField field, const UnicodeString& value) {
    appendItemNames[field] = value;
    // NUL-terminate for the C API.
    appendItemNames[field].getTerminatedBuffer();
}

const UnicodeString&
DateTimePatternGenerator:: getAppendItemName(UDateTimePatternField field) const {
    return appendItemNames[field];
}

void
DateTimePatternGenerator::getAppendName(UDateTimePatternField field, UnicodeString& value) {
    value = SINGLE_QUOTE;
    value += appendItemNames[field];
    value += SINGLE_QUOTE;
}

UnicodeString
DateTimePatternGenerator::getBestPattern(const UnicodeString& patternForm, UErrorCode& status) {
    return getBestPattern(patternForm, UDATPG_MATCH_NO_OPTIONS, status);
}

UnicodeString
DateTimePatternGenerator::getBestPattern(const UnicodeString& patternForm, UDateTimePatternMatchOptions options, UErrorCode& status) {
    const UnicodeString *bestPattern=NULL;
    UnicodeString dtFormat;
    UnicodeString resultPattern;

    int32_t dateMask=(1<<UDATPG_DAYPERIOD_FIELD) - 1;
    int32_t timeMask=(1<<UDATPG_FIELD_COUNT) - 1 - dateMask;

    UnicodeString patternFormCopy = UnicodeString(patternForm);
    patternFormCopy.findAndReplace(UnicodeString(LOW_J), UnicodeString(fDefaultHourFormatChar));

    resultPattern.remove();
    dtMatcher->set(patternFormCopy, fp);
    const PtnSkeleton* specifiedSkeleton=NULL;
    bestPattern=getBestRaw(*dtMatcher, -1, distanceInfo, &specifiedSkeleton);
    if ( distanceInfo->missingFieldMask==0 && distanceInfo->extraFieldMask==0 ) {
        resultPattern = adjustFieldTypes(*bestPattern, specifiedSkeleton, FALSE, options);

        return resultPattern;
    }
    int32_t neededFields = dtMatcher->getFieldMask();
    UnicodeString datePattern=getBestAppending(neededFields & dateMask, options);
    UnicodeString timePattern=getBestAppending(neededFields & timeMask, options);
    if (datePattern.length()==0) {
        if (timePattern.length()==0) {
            resultPattern.remove();
        }
        else {
            return timePattern;
        }
    }
    if (timePattern.length()==0) {
        return datePattern;
    }
    resultPattern.remove();
    status = U_ZERO_ERROR;
    dtFormat=getDateTimeFormat();
    Formattable dateTimeObject[] = { timePattern, datePattern };
    resultPattern = MessageFormat::format(dtFormat, dateTimeObject, 2, resultPattern, status );
    return resultPattern;
}

UnicodeString
DateTimePatternGenerator::replaceFieldTypes(const UnicodeString& pattern,
                                            const UnicodeString& skeleton,
                                            UErrorCode& status) {
    return replaceFieldTypes(pattern, skeleton, UDATPG_MATCH_NO_OPTIONS, status);
}

UnicodeString
DateTimePatternGenerator::replaceFieldTypes(const UnicodeString& pattern,
                                            const UnicodeString& skeleton,
                                            UDateTimePatternMatchOptions options,
                                            UErrorCode& /*status*/) {
    dtMatcher->set(skeleton, fp);
    UnicodeString result = adjustFieldTypes(pattern, NULL, FALSE, options);
    return result;
}

void
DateTimePatternGenerator::setDecimal(const UnicodeString& newDecimal) {
    this->decimal = newDecimal;
    // NUL-terminate for the C API.
    this->decimal.getTerminatedBuffer();
}

const UnicodeString&
DateTimePatternGenerator::getDecimal() const {
    return decimal;
}

void
DateTimePatternGenerator::addCanonicalItems() {
    UnicodeString  conflictingPattern;
    UDateTimePatternConflict conflictingStatus;
    UErrorCode status = U_ZERO_ERROR;

    for (int32_t i=0; i<UDATPG_FIELD_COUNT; i++) {
        conflictingStatus = addPattern(UnicodeString(Canonical_Items[i]), FALSE, conflictingPattern, status);
    }
}

void
DateTimePatternGenerator::setDateTimeFormat(const UnicodeString& dtFormat) {
    dateTimeFormat = dtFormat;
    // NUL-terminate for the C API.
    dateTimeFormat.getTerminatedBuffer();
}

const UnicodeString&
DateTimePatternGenerator::getDateTimeFormat() const {
    return dateTimeFormat;
}

void
DateTimePatternGenerator::setDateTimeFromCalendar(const Locale& locale, UErrorCode& status) {
    const UChar *resStr;
    int32_t resStrLen = 0;

    Calendar* fCalendar = Calendar::createInstance(locale, status);
    CalendarData calData(locale, fCalendar?fCalendar->getType():NULL, status);
    UResourceBundle *dateTimePatterns = calData.getByKey(DT_DateTimePatternsTag, status);
    if (U_FAILURE(status)) return;

    if (ures_getSize(dateTimePatterns) <= DateFormat::kDateTime)
    {
        status = U_INVALID_FORMAT_ERROR;
        return;
    }
    resStr = ures_getStringByIndex(dateTimePatterns, (int32_t)DateFormat::kDateTime, &resStrLen, &status);
    setDateTimeFormat(UnicodeString(TRUE, resStr, resStrLen));

    delete fCalendar;
}

void
DateTimePatternGenerator::setDecimalSymbols(const Locale& locale, UErrorCode& status) {
    DecimalFormatSymbols dfs = DecimalFormatSymbols(locale, status);
    if(U_SUCCESS(status)) {
        decimal = dfs.getSymbol(DecimalFormatSymbols::kDecimalSeparatorSymbol);
        // NUL-terminate for the C API.
        decimal.getTerminatedBuffer();
    }
}

UDateTimePatternConflict
DateTimePatternGenerator::addPattern(
    const UnicodeString& pattern,
    UBool override,
    UnicodeString &conflictingPattern,
    UErrorCode& status)
{
    return addPatternWithSkeleton(pattern, NULL, override, conflictingPattern, status);
}

// For DateTimePatternGenerator::addPatternWithSkeleton -
// If skeletonToUse is specified, then an availableFormats entry is being added. In this case:
// 1. We pass that skeleton to matcher.set instead of having it derive a skeleton from the pattern.
// 2. If the new entry's skeleton or basePattern does match an existing entry but that entry also had a skeleton specified
// (i.e. it was also from availableFormats), then the new entry does not override it regardless of the value of the override
// parameter. This prevents later availableFormats entries from a parent locale overriding earlier ones from the actual
// specified locale. However, availableFormats entries *should* override entries with matching skeleton whose skeleton was
// derived (i.e. entries derived from the standard date/time patters for the specified locale).
// 3. When adding the pattern (patternMap->add), we set a new boolean to indicate that the added entry had a
// specified skeleton (which sets a new field in the PtnElem in the PatternMap).
UDateTimePatternConflict
DateTimePatternGenerator::addPatternWithSkeleton(
    const UnicodeString& pattern,
    const UnicodeString* skeletonToUse,
    UBool override,
    UnicodeString& conflictingPattern,
    UErrorCode& status)
{

    UnicodeString basePattern;
    PtnSkeleton   skeleton;
    UDateTimePatternConflict conflictingStatus = UDATPG_NO_CONFLICT;

    DateTimeMatcher matcher;
    if ( skeletonToUse == NULL ) {
        matcher.set(pattern, fp, skeleton);
        matcher.getBasePattern(basePattern);
    } else {
        matcher.set(*skeletonToUse, fp, skeleton); // this still trims skeleton fields to max len 3, may need to change it.
        matcher.getBasePattern(basePattern); // or perhaps instead: basePattern = *skeletonToUse;
    }
    UBool entryHadSpecifiedSkeleton;
    const UnicodeString *duplicatePattern = patternMap->getPatternFromBasePattern(basePattern, entryHadSpecifiedSkeleton);
    if (duplicatePattern != NULL ) {
        conflictingStatus = UDATPG_BASE_CONFLICT;
        conflictingPattern = *duplicatePattern;
        if (!override || (skeletonToUse != NULL && entryHadSpecifiedSkeleton)) {
            return conflictingStatus;
        }
    }
    const PtnSkeleton* entrySpecifiedSkeleton = NULL;
    duplicatePattern = patternMap->getPatternFromSkeleton(skeleton, &entrySpecifiedSkeleton);
    if (duplicatePattern != NULL ) {
        conflictingStatus = UDATPG_CONFLICT;
        conflictingPattern = *duplicatePattern;
        if (!override || (skeletonToUse != NULL && entrySpecifiedSkeleton != NULL)) {
            return conflictingStatus;
        }
    }
    patternMap->add(basePattern, skeleton, pattern, skeletonToUse != NULL, status);
    if(U_FAILURE(status)) {
        return conflictingStatus;
    }

    return UDATPG_NO_CONFLICT;
}


UDateTimePatternField
DateTimePatternGenerator::getAppendFormatNumber(const char* field) const {
    for (int32_t i=0; i<UDATPG_FIELD_COUNT; ++i ) {
        if (uprv_strcmp(CLDR_FIELD_APPEND[i], field)==0) {
            return (UDateTimePatternField)i;
        }
    }
    return UDATPG_FIELD_COUNT;
}

UDateTimePatternField
DateTimePatternGenerator::getAppendNameNumber(const char* field) const {
    for (int32_t i=0; i<UDATPG_FIELD_COUNT; ++i ) {
        if (uprv_strcmp(CLDR_FIELD_NAME[i],field)==0) {
            return (UDateTimePatternField)i;
        }
    }
    return UDATPG_FIELD_COUNT;
}

const UnicodeString*
DateTimePatternGenerator::getBestRaw(DateTimeMatcher& source,
                                     int32_t includeMask,
                                     DistanceInfo* missingFields,
                                     const PtnSkeleton** specifiedSkeletonPtr) {
    int32_t bestDistance = 0x7fffffff;
    DistanceInfo tempInfo;
    const UnicodeString *bestPattern=NULL;
    const PtnSkeleton* specifiedSkeleton=NULL;

    PatternMapIterator it;
    for (it.set(*patternMap); it.hasNext(); ) {
        DateTimeMatcher trial = it.next();
        if (trial.equals(skipMatcher)) {
            continue;
        }
        int32_t distance=source.getDistance(trial, includeMask, tempInfo);
        if (distance<bestDistance) {
            bestDistance=distance;
            bestPattern=patternMap->getPatternFromSkeleton(*trial.getSkeletonPtr(), &specifiedSkeleton);
            missingFields->setTo(tempInfo);
            if (distance==0) {
                break;
            }
        }
    }

    // If the best raw match had a specified skeleton and that skeleton was requested by the caller,
    // then return it too. This generally happens when the caller needs to pass that skeleton
    // through to adjustFieldTypes so the latter can do a better job.
    if (bestPattern && specifiedSkeletonPtr) {
        *specifiedSkeletonPtr = specifiedSkeleton;
    }
    return bestPattern;
}

UnicodeString
DateTimePatternGenerator::adjustFieldTypes(const UnicodeString& pattern,
                                           const PtnSkeleton* specifiedSkeleton,
                                           UBool fixFractionalSeconds,
                                           UDateTimePatternMatchOptions options) {
    UnicodeString newPattern;
    fp->set(pattern);
    for (int32_t i=0; i < fp->itemNumber; i++) {
        UnicodeString field = fp->items[i];
        if ( fp->isQuoteLiteral(field) ) {

            UnicodeString quoteLiteral;
            fp->getQuoteLiteral(quoteLiteral, &i);
            newPattern += quoteLiteral;
        }
        else {
            if (fp->isPatternSeparator(field)) {
                newPattern+=field;
                continue;
            }
            int32_t canonicalIndex = fp->getCanonicalIndex(field);
            if (canonicalIndex < 0) {
                newPattern+=field;
                continue;  // don't adjust
            }
            const dtTypeElem *row = &dtTypes[canonicalIndex];
            int32_t typeValue = row->field;
            if (fixFractionalSeconds && typeValue == UDATPG_SECOND_FIELD) {
                UnicodeString newField=dtMatcher->skeleton.original[UDATPG_FRACTIONAL_SECOND_FIELD];
                field = field + decimal + newField;
            }
            else {
                if (dtMatcher->skeleton.type[typeValue]!=0) {
                    // Here:
                    // - "reqField" is the field from the originally requested skeleton, with length
                    // "reqFieldLen".
                    // - "field" is the field from the found pattern.
                    //
                    // The adjusted field should consist of characters from the originally requested
                    // skeleton, except in the case of UDATPG_HOUR_FIELD or UDATPG_MONTH_FIELD or
                    // UDATPG_WEEKDAY_FIELD, in which case it should consist of characters from the
                    // found pattern.
                    //
                    // The length of the adjusted field (adjFieldLen) should match that in the originally
                    // requested skeleton, except that in the following cases the length of the adjusted field
                    // should match that in the found pattern (i.e. the length of this pattern field should
                    // not be adjusted):
                    // 1. typeValue is UDATPG_HOUR_FIELD/MINUTE/SECOND and the corresponding bit in options is
                    //    not set (ticket #7180). Note, we may want to implement a similar change for other
                    //    numeric fields (MM, dd, etc.) so the default behavior is to get locale preference for
                    //    field length, but options bits can be used to override this.
                    // 2. There is a specified skeleton for the found pattern and one of the following is true:
                    //    a) The length of the field in the skeleton (skelFieldLen) is equal to reqFieldLen.
                    //    b) The pattern field is numeric and the skeleton field is not, or vice versa.

                    UnicodeString reqField = dtMatcher->skeleton.original[typeValue];
                    int32_t reqFieldLen = reqField.length();
                    if (reqField.charAt(0) == CAP_E && reqFieldLen < 3)
                    	reqFieldLen = 3; // 1-3 for E are equivalent to 3 for c,e
                    int32_t adjFieldLen = reqFieldLen;
                    if ( (typeValue==UDATPG_HOUR_FIELD && (options & UDATPG_MATCH_HOUR_FIELD_LENGTH)==0) ||
                         (typeValue==UDATPG_MINUTE_FIELD && (options & UDATPG_MATCH_MINUTE_FIELD_LENGTH)==0) ||
                         (typeValue==UDATPG_SECOND_FIELD && (options & UDATPG_MATCH_SECOND_FIELD_LENGTH)==0) ) {
                         adjFieldLen = field.length();
                    } else if (specifiedSkeleton) {
                        UnicodeString skelField = specifiedSkeleton->original[typeValue];
                        int32_t skelFieldLen = skelField.length();
                        UBool patFieldIsNumeric = (row->type > 0);
                        UBool skelFieldIsNumeric = (specifiedSkeleton->type[typeValue] > 0);
                        if (skelFieldLen == reqFieldLen || (patFieldIsNumeric && !skelFieldIsNumeric) || (skelFieldIsNumeric && !patFieldIsNumeric)) {
                            // don't adjust the field length in the found pattern
                            adjFieldLen = field.length();
                        }
                    }
                    UChar c = (typeValue!= UDATPG_HOUR_FIELD && typeValue!= UDATPG_MONTH_FIELD && typeValue!= UDATPG_WEEKDAY_FIELD)?
                        reqField.charAt(0): field.charAt(0);
                    field.remove();
                    for (int32_t i=adjFieldLen; i>0; --i) {
                        field+=c;
                    }
                }
                newPattern+=field;
            }
        }
    }
    return newPattern;
}

UnicodeString
DateTimePatternGenerator::getBestAppending(int32_t missingFields, UDateTimePatternMatchOptions options) {
    UnicodeString  resultPattern, tempPattern, formattedPattern;
    UErrorCode err=U_ZERO_ERROR;
    int32_t lastMissingFieldMask=0;
    if (missingFields!=0) {
        resultPattern=UnicodeString();
        const PtnSkeleton* specifiedSkeleton=NULL;
        tempPattern = *getBestRaw(*dtMatcher, missingFields, distanceInfo, &specifiedSkeleton);
        resultPattern = adjustFieldTypes(tempPattern, specifiedSkeleton, FALSE, options);
        if ( distanceInfo->missingFieldMask==0 ) {
            return resultPattern;
        }
        while (distanceInfo->missingFieldMask!=0) { // precondition: EVERY single field must work!
            if ( lastMissingFieldMask == distanceInfo->missingFieldMask ) {
                break;  // cannot find the proper missing field
            }
            if (((distanceInfo->missingFieldMask & UDATPG_SECOND_AND_FRACTIONAL_MASK)==UDATPG_FRACTIONAL_MASK) &&
                ((missingFields & UDATPG_SECOND_AND_FRACTIONAL_MASK) == UDATPG_SECOND_AND_FRACTIONAL_MASK)) {
                resultPattern = adjustFieldTypes(resultPattern, specifiedSkeleton, FALSE, options);
                //resultPattern = tempPattern;
                distanceInfo->missingFieldMask &= ~UDATPG_FRACTIONAL_MASK;
                continue;
            }
            int32_t startingMask = distanceInfo->missingFieldMask;
            tempPattern = *getBestRaw(*dtMatcher, distanceInfo->missingFieldMask, distanceInfo, &specifiedSkeleton);
            tempPattern = adjustFieldTypes(tempPattern, specifiedSkeleton, FALSE, options);
            int32_t foundMask=startingMask& ~distanceInfo->missingFieldMask;
            int32_t topField=getTopBitNumber(foundMask);
            UnicodeString appendName;
            getAppendName((UDateTimePatternField)topField, appendName);
            const Formattable formatPattern[] = {
                resultPattern,
                tempPattern,
                appendName
            };
            UnicodeString emptyStr;
            formattedPattern = MessageFormat::format(appendItemFormats[topField], formatPattern, 3, emptyStr, err);
            lastMissingFieldMask = distanceInfo->missingFieldMask;
        }
    }
    return formattedPattern;
}

int32_t
DateTimePatternGenerator::getTopBitNumber(int32_t foundMask) {
    if ( foundMask==0 ) {
        return 0;
    }
    int32_t i=0;
    while (foundMask!=0) {
        foundMask >>=1;
        ++i;
    }
    if (i-1 >UDATPG_ZONE_FIELD) {
        return UDATPG_ZONE_FIELD;
    }
    else
        return i-1;
}

void
DateTimePatternGenerator::setAvailableFormat(const UnicodeString &key, UErrorCode& err)
{
    fAvailableFormatKeyHash->puti(key, 1, err);
}

UBool
DateTimePatternGenerator::isAvailableFormatSet(const UnicodeString &key) const {
    return (UBool)(fAvailableFormatKeyHash->geti(key) == 1);
}

void
DateTimePatternGenerator::copyHashtable(Hashtable *other, UErrorCode &status) {

    if (other == NULL) {
        return;
    }
    if (fAvailableFormatKeyHash != NULL) {
        delete fAvailableFormatKeyHash;
        fAvailableFormatKeyHash = NULL;
    }
    initHashtable(status);
    if(U_FAILURE(status)){
        return;
    }
    int32_t pos = -1;
    const UHashElement* elem = NULL;
    // walk through the hash table and create a deep clone
    while((elem = other->nextElement(pos))!= NULL){
        const UHashTok otherKeyTok = elem->key;
        UnicodeString* otherKey = (UnicodeString*)otherKeyTok.pointer;
        fAvailableFormatKeyHash->puti(*otherKey, 1, status);
        if(U_FAILURE(status)){
            return;
        }
    }
}

StringEnumeration*
DateTimePatternGenerator::getSkeletons(UErrorCode& status) const {
    StringEnumeration* skeletonEnumerator = new DTSkeletonEnumeration(*patternMap, DT_SKELETON, status);
    return skeletonEnumerator;
}

const UnicodeString&
DateTimePatternGenerator::getPatternForSkeleton(const UnicodeString& skeleton) const {
    PtnElem *curElem;

    if (skeleton.length() ==0) {
        return emptyString;
    }
    curElem = patternMap->getHeader(skeleton.charAt(0));
    while ( curElem != NULL ) {
        if ( curElem->skeleton->getSkeleton()==skeleton ) {
            return curElem->pattern;
        }
        curElem=curElem->next;
    }
    return emptyString;
}

StringEnumeration*
DateTimePatternGenerator::getBaseSkeletons(UErrorCode& status) const {
    StringEnumeration* baseSkeletonEnumerator = new DTSkeletonEnumeration(*patternMap, DT_BASESKELETON, status);
    return baseSkeletonEnumerator;
}

StringEnumeration*
DateTimePatternGenerator::getRedundants(UErrorCode& status) {
    StringEnumeration* output = new DTRedundantEnumeration();
    const UnicodeString *pattern;
    PatternMapIterator it;
    for (it.set(*patternMap); it.hasNext(); ) {
        DateTimeMatcher current = it.next();
        pattern = patternMap->getPatternFromSkeleton(*(it.getSkeleton()));
        if ( isCanonicalItem(*pattern) ) {
            continue;
        }
        if ( skipMatcher == NULL ) {
            skipMatcher = new DateTimeMatcher(current);
        }
        else {
            *skipMatcher = current;
        }
        UnicodeString trial = getBestPattern(current.getPattern(), status);
        if (trial == *pattern) {
            ((DTRedundantEnumeration *)output)->add(*pattern, status);
        }
        if (current.equals(skipMatcher)) {
            continue;
        }
    }
    return output;
}

UBool
DateTimePatternGenerator::isCanonicalItem(const UnicodeString& item) const {
    if ( item.length() != 1 ) {
        return FALSE;
    }
    for (int32_t i=0; i<UDATPG_FIELD_COUNT; ++i) {
        if (item.charAt(0)==Canonical_Items[i]) {
            return TRUE;
        }
    }
    return FALSE;
}


DateTimePatternGenerator*
DateTimePatternGenerator::clone() const {
    return new DateTimePatternGenerator(*this);
}

PatternMap::PatternMap() {
   for (int32_t i=0; i < MAX_PATTERN_ENTRIES; ++i ) {
      boot[i]=NULL;
   }
   isDupAllowed = TRUE;
}

void
PatternMap::copyFrom(const PatternMap& other, UErrorCode& status) {
    this->isDupAllowed = other.isDupAllowed;
    for (int32_t bootIndex=0; bootIndex<MAX_PATTERN_ENTRIES; ++bootIndex ) {
        PtnElem *curElem, *otherElem, *prevElem=NULL;
        otherElem = other.boot[bootIndex];
        while (otherElem!=NULL) {
            if ((curElem = new PtnElem(otherElem->basePattern, otherElem->pattern))==NULL) {
                // out of memory
                status = U_MEMORY_ALLOCATION_ERROR;
                return;
            }
            if ( this->boot[bootIndex]== NULL ) {
                this->boot[bootIndex] = curElem;
            }
            if ((curElem->skeleton=new PtnSkeleton(*(otherElem->skeleton))) == NULL ) {
                // out of memory
                status = U_MEMORY_ALLOCATION_ERROR;
                return;
            }

            if (prevElem!=NULL) {
                prevElem->next=curElem;
            }
            curElem->next=NULL;
            prevElem = curElem;
            otherElem = otherElem->next;
        }

    }
}

PtnElem*
PatternMap::getHeader(UChar baseChar) {
    PtnElem* curElem;

    if ( (baseChar >= CAP_A) && (baseChar <= CAP_Z) ) {
         curElem = boot[baseChar-CAP_A];
    }
    else {
        if ( (baseChar >=LOW_A) && (baseChar <= LOW_Z) ) {
            curElem = boot[26+baseChar-LOW_A];
        }
        else {
            return NULL;
        }
    }
    return curElem;
}

PatternMap::~PatternMap() {
   for (int32_t i=0; i < MAX_PATTERN_ENTRIES; ++i ) {
       if (boot[i]!=NULL ) {
           delete boot[i];
           boot[i]=NULL;
       }
   }
}  // PatternMap destructor

void
PatternMap::add(const UnicodeString& basePattern,
                const PtnSkeleton& skeleton,
                const UnicodeString& value,// mapped pattern value
                UBool skeletonWasSpecified,
                UErrorCode &status) {
    UChar baseChar = basePattern.charAt(0);
    PtnElem *curElem, *baseElem;
    status = U_ZERO_ERROR;

    // the baseChar must be A-Z or a-z
    if ((baseChar >= CAP_A) && (baseChar <= CAP_Z)) {
        baseElem = boot[baseChar-CAP_A];
    }
    else {
        if ((baseChar >=LOW_A) && (baseChar <= LOW_Z)) {
            baseElem = boot[26+baseChar-LOW_A];
         }
         else {
             status = U_ILLEGAL_CHARACTER;
             return;
         }
    }

    if (baseElem == NULL) {
        if ((curElem = new PtnElem(basePattern, value)) == NULL ) {
            // out of memory
            status = U_MEMORY_ALLOCATION_ERROR;
            return;
        }
        if (baseChar >= LOW_A) {
            boot[26 + (baseChar-LOW_A)] = curElem;
        }
        else {
            boot[baseChar-CAP_A] = curElem;
        }
        curElem->skeleton = new PtnSkeleton(skeleton);
        curElem->skeletonWasSpecified = skeletonWasSpecified;
    }
    if ( baseElem != NULL ) {
        curElem = getDuplicateElem(basePattern, skeleton, baseElem);

        if (curElem == NULL) {
            // add new element to the list.
            curElem = baseElem;
            while( curElem -> next != NULL )
            {
                curElem = curElem->next;
            }
            if ((curElem->next = new PtnElem(basePattern, value)) == NULL ) {
                // out of memory
                status = U_MEMORY_ALLOCATION_ERROR;
                return;
            }
            curElem=curElem->next;
            curElem->skeleton = new PtnSkeleton(skeleton);
            curElem->skeletonWasSpecified = skeletonWasSpecified;
        }
        else {
            // Pattern exists in the list already.
            if ( !isDupAllowed ) {
                return;
            }
            // Overwrite the value.
            curElem->pattern = value;
        }
    }
}  // PatternMap::add

// Find the pattern from the given basePattern string.
const UnicodeString *
PatternMap::getPatternFromBasePattern(UnicodeString& basePattern, UBool& skeletonWasSpecified) { // key to search for
   PtnElem *curElem;

   if ((curElem=getHeader(basePattern.charAt(0)))==NULL) {
       return NULL;  // no match
   }

   do  {
       if ( basePattern.compare(curElem->basePattern)==0 ) {
          skeletonWasSpecified = curElem->skeletonWasSpecified;
          return &(curElem->pattern);
       }
       curElem=curElem->next;
   }while (curElem != NULL);

   return NULL;
}  // PatternMap::getFromBasePattern


// Find the pattern from the given skeleton.
// At least when this is called from getBestRaw & addPattern (in which case specifiedSkeletonPtr is non-NULL),
// the comparison should be based on skeleton.original (which is unique and tied to the distance measurement in bestRaw)
// and not skeleton.baseOriginal (which is not unique); otherwise we may pick a different skeleton than the one with the
// optimum distance value in getBestRaw. When this is called from public getRedundants (specifiedSkeletonPtr is NULL),
// for now it will continue to compare based on baseOriginal so as not to change the behavior unnecessarily.
const UnicodeString *
PatternMap::getPatternFromSkeleton(PtnSkeleton& skeleton, const PtnSkeleton** specifiedSkeletonPtr) { // key to search for
   PtnElem *curElem;

   if (specifiedSkeletonPtr) {
       *specifiedSkeletonPtr = NULL;
   }

   // find boot entry
   UChar baseChar='\0';
   for (int32_t i=0; i<UDATPG_FIELD_COUNT; ++i) {
       if (skeleton.baseOriginal[i].length() !=0 ) {
           baseChar = skeleton.baseOriginal[i].charAt(0);
           break;
       }
   }

   if ((curElem=getHeader(baseChar))==NULL) {
       return NULL;  // no match
   }

   do  {
       int32_t i=0;
       if (specifiedSkeletonPtr != NULL) { // called from DateTimePatternGenerator::getBestRaw or addPattern, use original
           for (i=0; i<UDATPG_FIELD_COUNT; ++i) {
               if (curElem->skeleton->original[i].compare(skeleton.original[i]) != 0 )
               {
                   break;
               }
           }
       } else { // called from DateTimePatternGenerator::getRedundants, use baseOriginal
           for (i=0; i<UDATPG_FIELD_COUNT; ++i) {
               if (curElem->skeleton->baseOriginal[i].compare(skeleton.baseOriginal[i]) != 0 )
               {
                   break;
               }
           }
       }
       if (i == UDATPG_FIELD_COUNT) {
           if (specifiedSkeletonPtr && curElem->skeletonWasSpecified) {
               *specifiedSkeletonPtr = curElem->skeleton;
           }
           return &(curElem->pattern);
       }
       curElem=curElem->next;
   }while (curElem != NULL);

   return NULL;
}

UBool
PatternMap::equals(const PatternMap& other) {
    if ( this==&other ) {
        return TRUE;
    }
    for (int32_t bootIndex=0; bootIndex<MAX_PATTERN_ENTRIES; ++bootIndex ) {
        if ( boot[bootIndex]==other.boot[bootIndex] ) {
            continue;
        }
        if ( (boot[bootIndex]==NULL)||(other.boot[bootIndex]==NULL) ) {
            return FALSE;
        }
        PtnElem *otherElem = other.boot[bootIndex];
        PtnElem *myElem = boot[bootIndex];
        while ((otherElem!=NULL) || (myElem!=NULL)) {
            if ( myElem == otherElem ) {
                break;
            }
            if ((otherElem==NULL) || (myElem==NULL)) {
                return FALSE;
            }
            if ( (myElem->basePattern != otherElem->basePattern) ||
                 (myElem->pattern != otherElem->pattern) ) {
                return FALSE;
            }
            if ((myElem->skeleton!=otherElem->skeleton)&&
                !myElem->skeleton->equals(*(otherElem->skeleton))) {
                return FALSE;
            }
            myElem = myElem->next;
            otherElem=otherElem->next;
        }
    }
    return TRUE;
}

// find any key existing in the mapping table already.
// return TRUE if there is an existing key, otherwise return FALSE.
PtnElem*
PatternMap::getDuplicateElem(
            const UnicodeString &basePattern,
            const PtnSkeleton &skeleton,
            PtnElem *baseElem)  {
   PtnElem *curElem;

   if ( baseElem == (PtnElem *)NULL )  {
         return (PtnElem*)NULL;
   }
   else {
         curElem = baseElem;
   }
   do {
     if ( basePattern.compare(curElem->basePattern)==0 ) {
        UBool isEqual=TRUE;
        for (int32_t i=0; i<UDATPG_FIELD_COUNT; ++i) {
            if (curElem->skeleton->type[i] != skeleton.type[i] ) {
                isEqual=FALSE;
                break;
            }
        }
        if (isEqual) {
            return curElem;
        }
     }
     curElem = curElem->next;
   } while( curElem != (PtnElem *)NULL );

   // end of the list
   return (PtnElem*)NULL;

}  // PatternMap::getDuplicateElem

DateTimeMatcher::DateTimeMatcher(void) {
}

DateTimeMatcher::DateTimeMatcher(const DateTimeMatcher& other) {
    copyFrom(other.skeleton);
}


void
DateTimeMatcher::set(const UnicodeString& pattern, FormatParser* fp) {
    PtnSkeleton localSkeleton;
    return set(pattern, fp, localSkeleton);
}

void
DateTimeMatcher::set(const UnicodeString& pattern, FormatParser* fp, PtnSkeleton& skeletonResult) {
    int32_t i;
    for (i=0; i<UDATPG_FIELD_COUNT; ++i) {
        skeletonResult.type[i]=NONE;
    }
    fp->set(pattern);
    for (i=0; i < fp->itemNumber; i++) {
        UnicodeString field = fp->items[i];
        if ( field.charAt(0) == LOW_A ) {
            continue;  // skip 'a'
        }

        if ( fp->isQuoteLiteral(field) ) {
            UnicodeString quoteLiteral;
            fp->getQuoteLiteral(quoteLiteral, &i);
            continue;
        }
        int32_t canonicalIndex = fp->getCanonicalIndex(field);
        if (canonicalIndex < 0 ) {
            continue;
        }
        const dtTypeElem *row = &dtTypes[canonicalIndex];
        int32_t typeValue = row->field;
        skeletonResult.original[typeValue]=field;
        UChar repeatChar = row->patternChar;
        int32_t repeatCount = row->minLen > 3 ? 3: row->minLen;
        while (repeatCount-- > 0) {
            skeletonResult.baseOriginal[typeValue] += repeatChar;
        }
        int16_t subTypeValue = row->type;
        if ( row->type > 0) {
            subTypeValue += field.length();
        }
        skeletonResult.type[typeValue] = subTypeValue;
    }
    copyFrom(skeletonResult);
}

void
DateTimeMatcher::getBasePattern(UnicodeString &result ) {
    result.remove(); // Reset the result first.
    for (int32_t i=0; i<UDATPG_FIELD_COUNT; ++i ) {
        if (skeleton.baseOriginal[i].length()!=0) {
            result += skeleton.baseOriginal[i];
        }
    }
}

UnicodeString
DateTimeMatcher::getPattern() {
    UnicodeString result;

    for (int32_t i=0; i<UDATPG_FIELD_COUNT; ++i ) {
        if (skeleton.original[i].length()!=0) {
            result += skeleton.original[i];
        }
    }
    return result;
}

int32_t
DateTimeMatcher::getDistance(const DateTimeMatcher& other, int32_t includeMask, DistanceInfo& distanceInfo) {
    int32_t result=0;
    distanceInfo.clear();
    for (int32_t i=0; i<UDATPG_FIELD_COUNT; ++i ) {
        int32_t myType = (includeMask&(1<<i))==0 ? 0 : skeleton.type[i];
        int32_t otherType = other.skeleton.type[i];
        if (myType==otherType) {
            continue;
        }
        if (myType==0) {// and other is not
            result += EXTRA_FIELD;
            distanceInfo.addExtra(i);
        }
        else {
            if (otherType==0) {
                result += MISSING_FIELD;
                distanceInfo.addMissing(i);
            }
            else {
                result += abs(myType - otherType);
            }
        }

    }
    return result;
}

void
DateTimeMatcher::copyFrom(const PtnSkeleton& newSkeleton) {
    for (int32_t i=0; i<UDATPG_FIELD_COUNT; ++i) {
        this->skeleton.type[i]=newSkeleton.type[i];
        this->skeleton.original[i]=newSkeleton.original[i];
        this->skeleton.baseOriginal[i]=newSkeleton.baseOriginal[i];
    }
}

void
DateTimeMatcher::copyFrom() {
    // same as clear
    for (int32_t i=0; i<UDATPG_FIELD_COUNT; ++i) {
        this->skeleton.type[i]=0;
        this->skeleton.original[i].remove();
        this->skeleton.baseOriginal[i].remove();
    }
}

UBool
DateTimeMatcher::equals(const DateTimeMatcher* other) const {
    if (other==NULL) {
        return FALSE;
    }
    for (int32_t i=0; i<UDATPG_FIELD_COUNT; ++i) {
        if (this->skeleton.original[i]!=other->skeleton.original[i] ) {
            return FALSE;
        }
    }
    return TRUE;
}

int32_t
DateTimeMatcher::getFieldMask() {
    int32_t result=0;

    for (int32_t i=0; i<UDATPG_FIELD_COUNT; ++i) {
        if (skeleton.type[i]!=0) {
            result |= (1<<i);
        }
    }
    return result;
}

PtnSkeleton*
DateTimeMatcher::getSkeletonPtr() {
    return &skeleton;
}

FormatParser::FormatParser () {
    status = START;
    itemNumber=0;
}


FormatParser::~FormatParser () {
}


// Find the next token with the starting position and length
// Note: the startPos may
FormatParser::TokenStatus
FormatParser::setTokens(const UnicodeString& pattern, int32_t startPos, int32_t *len) {
    int32_t  curLoc = startPos;
    if ( curLoc >= pattern.length()) {
        return DONE;
    }
    // check the current char is between A-Z or a-z
    do {
        UChar c=pattern.charAt(curLoc);
        if ( (c>=CAP_A && c<=CAP_Z) || (c>=LOW_A && c<=LOW_Z) ) {
           curLoc++;
        }
        else {
               startPos = curLoc;
               *len=1;
               return ADD_TOKEN;
        }

        if ( pattern.charAt(curLoc)!= pattern.charAt(startPos) ) {
            break;  // not the same token
        }
    } while(curLoc <= pattern.length());
    *len = curLoc-startPos;
    return ADD_TOKEN;
}

void
FormatParser::set(const UnicodeString& pattern) {
    int32_t startPos=0;
    TokenStatus result=START;
    int32_t len=0;
    itemNumber =0;

    do {
        result = setTokens( pattern, startPos, &len );
        if ( result == ADD_TOKEN )
        {
            items[itemNumber++] = UnicodeString(pattern, startPos, len );
            startPos += len;
        }
        else {
            break;
        }
    } while (result==ADD_TOKEN && itemNumber < MAX_DT_TOKEN);
}

int32_t
FormatParser::getCanonicalIndex(const UnicodeString& s, UBool strict) {
    int32_t len = s.length();
    if (len == 0) {
        return -1;
    }
    UChar ch = s.charAt(0);

    // Verify that all are the same character.
    for (int32_t l = 1; l < len; l++) {
        if (ch != s.charAt(l)) {
            return -1;
        }
    }
    int32_t i = 0;
    int32_t bestRow = -1;
    while (dtTypes[i].patternChar != '\0') {
        if ( dtTypes[i].patternChar != ch ) {
            ++i;
            continue;
        }
        bestRow = i;
        if (dtTypes[i].patternChar != dtTypes[i+1].patternChar) {
            return i;
        }
        if (dtTypes[i+1].minLen <= len) {
            ++i;
            continue;
        }
        return i;
    }
    return strict ? -1 : bestRow;
}

UBool
FormatParser::isQuoteLiteral(const UnicodeString& s) const {
    return (UBool)(s.charAt(0)==SINGLE_QUOTE);
}

// This function aussumes the current itemIndex points to the quote literal.
// Please call isQuoteLiteral prior to this function.
void
FormatParser::getQuoteLiteral(UnicodeString& quote, int32_t *itemIndex) {
    int32_t i=*itemIndex;

    quote.remove();
    if (items[i].charAt(0)==SINGLE_QUOTE) {
        quote += items[i];
        ++i;
    }
    while ( i < itemNumber ) {
        if ( items[i].charAt(0)==SINGLE_QUOTE ) {
            if ( (i+1<itemNumber) && (items[i+1].charAt(0)==SINGLE_QUOTE)) {
                // two single quotes e.g. 'o''clock'
                quote += items[i++];
                quote += items[i++];
                continue;
            }
            else {
                quote += items[i];
                break;
            }
        }
        else {
            quote += items[i];
        }
        ++i;
    }
    *itemIndex=i;
}

UBool
FormatParser::isPatternSeparator(UnicodeString& field) {
    for (int32_t i=0; i<field.length(); ++i ) {
        UChar c= field.charAt(i);
        if ( (c==SINGLE_QUOTE) || (c==BACKSLASH) || (c==SPACE) || (c==COLON) ||
             (c==QUOTATION_MARK) || (c==COMMA) || (c==HYPHEN) ||(items[i].charAt(0)==DOT) ) {
            continue;
        }
        else {
            return FALSE;
        }
    }
    return TRUE;
}

void
DistanceInfo::setTo(DistanceInfo &other) {
    missingFieldMask = other.missingFieldMask;
    extraFieldMask= other.extraFieldMask;
}

PatternMapIterator::PatternMapIterator() {
    bootIndex = 0;
    nodePtr = NULL;
    patternMap=NULL;
    matcher= new DateTimeMatcher();
}


PatternMapIterator::~PatternMapIterator() {
    delete matcher;
}

void
PatternMapIterator::set(PatternMap& newPatternMap) {
    this->patternMap=&newPatternMap;
}

PtnSkeleton*
PatternMapIterator::getSkeleton() {
    if ( nodePtr == NULL ) {
        return NULL;
    }
    else {
        return nodePtr->skeleton;
    }
}

UBool
PatternMapIterator::hasNext() {
    int32_t headIndex=bootIndex;
    PtnElem *curPtr=nodePtr;

    if (patternMap==NULL) {
        return FALSE;
    }
    while ( headIndex < MAX_PATTERN_ENTRIES ) {
        if ( curPtr != NULL ) {
            if ( curPtr->next != NULL ) {
                return TRUE;
            }
            else {
                headIndex++;
                curPtr=NULL;
                continue;
            }
        }
        else {
            if ( patternMap->boot[headIndex] != NULL ) {
                return TRUE;
            }
            else {
                headIndex++;
                continue;
            }
        }

    }
    return FALSE;
}

DateTimeMatcher&
PatternMapIterator::next() {
    while ( bootIndex < MAX_PATTERN_ENTRIES ) {
        if ( nodePtr != NULL ) {
            if ( nodePtr->next != NULL ) {
                nodePtr = nodePtr->next;
                break;
            }
            else {
                bootIndex++;
                nodePtr=NULL;
                continue;
            }
        }
        else {
            if ( patternMap->boot[bootIndex] != NULL ) {
                nodePtr = patternMap->boot[bootIndex];
                break;
            }
            else {
                bootIndex++;
                continue;
            }
        }
    }
    if (nodePtr!=NULL) {
        matcher->copyFrom(*nodePtr->skeleton);
    }
    else {
        matcher->copyFrom();
    }
    return *matcher;
}

PtnSkeleton::PtnSkeleton() {
}


PtnSkeleton::PtnSkeleton(const PtnSkeleton& other) {
    for (int32_t i=0; i<UDATPG_FIELD_COUNT; ++i) {
        this->type[i]=other.type[i];
        this->original[i]=other.original[i];
        this->baseOriginal[i]=other.baseOriginal[i];
    }
}

UBool
PtnSkeleton::equals(const PtnSkeleton& other)  {
    for (int32_t i=0; i<UDATPG_FIELD_COUNT; ++i) {
        if ( (type[i]!= other.type[i]) ||
             (original[i]!=other.original[i]) ||
             (baseOriginal[i]!=other.baseOriginal[i]) ) {
            return FALSE;
        }
    }
    return TRUE;
}

UnicodeString
PtnSkeleton::getSkeleton() {
    UnicodeString result;

    for(int32_t i=0; i< UDATPG_FIELD_COUNT; ++i) {
        if (original[i].length()!=0) {
            result += original[i];
        }
    }
    return result;
}

UnicodeString
PtnSkeleton::getBaseSkeleton() {
    UnicodeString result;

    for(int32_t i=0; i< UDATPG_FIELD_COUNT; ++i) {
        if (baseOriginal[i].length()!=0) {
            result += baseOriginal[i];
        }
    }
    return result;
}

PtnSkeleton::~PtnSkeleton() {
}

PtnElem::PtnElem(const UnicodeString &basePat, const UnicodeString &pat) :
basePattern(basePat),
skeleton(NULL),
pattern(pat),
next(NULL)
{
}

PtnElem::~PtnElem() {

    if (next!=NULL) {
        delete next;
    }
    delete skeleton;
}

DTSkeletonEnumeration::DTSkeletonEnumeration(PatternMap &patternMap, dtStrEnum type, UErrorCode& status) {
    PtnElem  *curElem;
    PtnSkeleton *curSkeleton;
    UnicodeString s;
    int32_t bootIndex;

    pos=0;
    fSkeletons = new UVector(status);
    if (U_FAILURE(status)) {
        delete fSkeletons;
        return;
    }
    for (bootIndex=0; bootIndex<MAX_PATTERN_ENTRIES; ++bootIndex ) {
        curElem = patternMap.boot[bootIndex];
        while (curElem!=NULL) {
            switch(type) {
                case DT_BASESKELETON:
                    s=curElem->basePattern;
                    break;
                case DT_PATTERN:
                    s=curElem->pattern;
                    break;
                case DT_SKELETON:
                    curSkeleton=curElem->skeleton;
                    s=curSkeleton->getSkeleton();
                    break;
            }
            if ( !isCanonicalItem(s) ) {
                fSkeletons->addElement(new UnicodeString(s), status);
                if (U_FAILURE(status)) {
                    delete fSkeletons;
                    fSkeletons = NULL;
                    return;
                }
            }
            curElem = curElem->next;
        }
    }
    if ((bootIndex==MAX_PATTERN_ENTRIES) && (curElem!=NULL) ) {
        status = U_BUFFER_OVERFLOW_ERROR;
    }
}

const UnicodeString*
DTSkeletonEnumeration::snext(UErrorCode& status) {
    if (U_SUCCESS(status) && pos < fSkeletons->size()) {
        return (const UnicodeString*)fSkeletons->elementAt(pos++);
    }
    return NULL;
}

void
DTSkeletonEnumeration::reset(UErrorCode& /*status*/) {
    pos=0;
}

int32_t
DTSkeletonEnumeration::count(UErrorCode& /*status*/) const {
   return (fSkeletons==NULL) ? 0 : fSkeletons->size();
}

UBool
DTSkeletonEnumeration::isCanonicalItem(const UnicodeString& item) {
    if ( item.length() != 1 ) {
        return FALSE;
    }
    for (int32_t i=0; i<UDATPG_FIELD_COUNT; ++i) {
        if (item.charAt(0)==Canonical_Items[i]) {
            return TRUE;
        }
    }
    return FALSE;
}

DTSkeletonEnumeration::~DTSkeletonEnumeration() {
    UnicodeString *s;
    for (int32_t i=0; i<fSkeletons->size(); ++i) {
        if ((s=(UnicodeString *)fSkeletons->elementAt(i))!=NULL) {
            delete s;
        }
    }
    delete fSkeletons;
}

DTRedundantEnumeration::DTRedundantEnumeration() {
    pos=0;
    fPatterns = NULL;
}

void
DTRedundantEnumeration::add(const UnicodeString& pattern, UErrorCode& status) {
    if (U_FAILURE(status)) return;
    if (fPatterns == NULL)  {
        fPatterns = new UVector(status);
        if (U_FAILURE(status)) {
            delete fPatterns;
            fPatterns = NULL;
            return;
       }
    }
    fPatterns->addElement(new UnicodeString(pattern), status);
    if (U_FAILURE(status)) {
        delete fPatterns;
        fPatterns = NULL;
        return;
    }
}

const UnicodeString*
DTRedundantEnumeration::snext(UErrorCode& status) {
    if (U_SUCCESS(status) && pos < fPatterns->size()) {
        return (const UnicodeString*)fPatterns->elementAt(pos++);
    }
    return NULL;
}

void
DTRedundantEnumeration::reset(UErrorCode& /*status*/) {
    pos=0;
}

int32_t
DTRedundantEnumeration::count(UErrorCode& /*status*/) const {
       return (fPatterns==NULL) ? 0 : fPatterns->size();
}

UBool
DTRedundantEnumeration::isCanonicalItem(const UnicodeString& item) {
    if ( item.length() != 1 ) {
        return FALSE;
    }
    for (int32_t i=0; i<UDATPG_FIELD_COUNT; ++i) {
        if (item.charAt(0)==Canonical_Items[i]) {
            return TRUE;
        }
    }
    return FALSE;
}

DTRedundantEnumeration::~DTRedundantEnumeration() {
    UnicodeString *s;
    for (int32_t i=0; i<fPatterns->size(); ++i) {
        if ((s=(UnicodeString *)fPatterns->elementAt(i))!=NULL) {
            delete s;
        }
    }
    delete fPatterns;
}

U_NAMESPACE_END


#endif /* #if !UCONFIG_NO_FORMATTING */

//eof
