/*
 *******************************************************************************
 * Copyright (C) 2008-2010, Google, International Business Machines Corporation
 * and  others. All Rights Reserved.                                           
 *******************************************************************************
 */

#include "unicode/utypeinfo.h"  // for 'typeid' to work

#include "unicode/tmutfmt.h"

#if !UCONFIG_NO_FORMATTING

#include "cmemory.h"
#include "cstring.h"
#include "hash.h"
#include "uresimp.h"
#include "unicode/msgfmt.h"

#define LEFT_CURLY_BRACKET  ((UChar)0x007B)
#define RIGHT_CURLY_BRACKET ((UChar)0x007D)
#define SPACE             ((UChar)0x0020)
#define DIGIT_ZERO        ((UChar)0x0030)
#define LOW_S             ((UChar)0x0073)
#define LOW_M             ((UChar)0x006D)
#define LOW_I             ((UChar)0x0069)
#define LOW_N             ((UChar)0x006E)
#define LOW_H             ((UChar)0x0068)
#define LOW_W             ((UChar)0x0077)
#define LOW_D             ((UChar)0x0064)
#define LOW_Y             ((UChar)0x0079)
#define LOW_Z             ((UChar)0x007A)
#define LOW_E             ((UChar)0x0065)
#define LOW_R             ((UChar)0x0072)
#define LOW_O             ((UChar)0x006F)
#define LOW_N             ((UChar)0x006E)
#define LOW_T             ((UChar)0x0074)


//TODO: define in compile time
//#define TMUTFMT_DEBUG 1

#ifdef TMUTFMT_DEBUG
#include <iostream>
#endif

U_NAMESPACE_BEGIN



UOBJECT_DEFINE_RTTI_IMPLEMENTATION(TimeUnitFormat)

static const char gUnitsTag[] = "units";
static const char gShortUnitsTag[] = "unitsShort";
static const char gTimeUnitYear[] = "year";
static const char gTimeUnitMonth[] = "month";
static const char gTimeUnitDay[] = "day";
static const char gTimeUnitWeek[] = "week";
static const char gTimeUnitHour[] = "hour";
static const char gTimeUnitMinute[] = "minute";
static const char gTimeUnitSecond[] = "second";
static const char gPluralCountOther[] = "other";

static const UChar DEFAULT_PATTERN_FOR_SECOND[] = {LEFT_CURLY_BRACKET, DIGIT_ZERO, RIGHT_CURLY_BRACKET, SPACE, LOW_S, 0};
static const UChar DEFAULT_PATTERN_FOR_MINUTE[] = {LEFT_CURLY_BRACKET, DIGIT_ZERO, RIGHT_CURLY_BRACKET, SPACE, LOW_M, LOW_I, LOW_N, 0};
static const UChar DEFAULT_PATTERN_FOR_HOUR[] = {LEFT_CURLY_BRACKET, DIGIT_ZERO, RIGHT_CURLY_BRACKET, SPACE, LOW_H, 0};
static const UChar DEFAULT_PATTERN_FOR_WEEK[] = {LEFT_CURLY_BRACKET, DIGIT_ZERO, RIGHT_CURLY_BRACKET, SPACE, LOW_W, 0};
static const UChar DEFAULT_PATTERN_FOR_DAY[] = {LEFT_CURLY_BRACKET, DIGIT_ZERO, RIGHT_CURLY_BRACKET, SPACE, LOW_D, 0};
static const UChar DEFAULT_PATTERN_FOR_MONTH[] = {LEFT_CURLY_BRACKET, DIGIT_ZERO, RIGHT_CURLY_BRACKET, SPACE, LOW_M, 0};
static const UChar DEFAULT_PATTERN_FOR_YEAR[] = {LEFT_CURLY_BRACKET, DIGIT_ZERO, RIGHT_CURLY_BRACKET, SPACE, LOW_Y, 0};

static const UChar PLURAL_COUNT_ZERO[] = {LOW_Z, LOW_E, LOW_R, LOW_O, 0};
static const UChar PLURAL_COUNT_ONE[] = {LOW_O, LOW_N, LOW_E, 0};
static const UChar PLURAL_COUNT_TWO[] = {LOW_T, LOW_W, LOW_O, 0};


TimeUnitFormat::TimeUnitFormat(UErrorCode& status)
:   fNumberFormat(NULL),
    fPluralRules(NULL) {
    create(Locale::getDefault(), kFull, status);
}


TimeUnitFormat::TimeUnitFormat(const Locale& locale, UErrorCode& status)
:   fNumberFormat(NULL),
    fPluralRules(NULL) {
    create(locale, kFull, status);
}


TimeUnitFormat::TimeUnitFormat(const Locale& locale, EStyle style, UErrorCode& status)
:   fNumberFormat(NULL),
    fPluralRules(NULL) {
    create(locale, style, status);
}


TimeUnitFormat::TimeUnitFormat(const TimeUnitFormat& other)
:   MeasureFormat(other),
    fNumberFormat(NULL),
    fPluralRules(NULL),
    fStyle(kFull)
{
    for (TimeUnit::UTimeUnitFields i = TimeUnit::UTIMEUNIT_YEAR;
         i < TimeUnit::UTIMEUNIT_FIELD_COUNT;
         i = (TimeUnit::UTimeUnitFields)(i+1)) {
        fTimeUnitToCountToPatterns[i] = NULL;
    }
    *this = other;
}


TimeUnitFormat::~TimeUnitFormat() {
    delete fNumberFormat;
    fNumberFormat = NULL;
    for (TimeUnit::UTimeUnitFields i = TimeUnit::UTIMEUNIT_YEAR;
         i < TimeUnit::UTIMEUNIT_FIELD_COUNT;
         i = (TimeUnit::UTimeUnitFields)(i+1)) {
        deleteHash(fTimeUnitToCountToPatterns[i]);
        fTimeUnitToCountToPatterns[i] = NULL;
    }
    delete fPluralRules;
    fPluralRules = NULL;
}


Format* 
TimeUnitFormat::clone(void) const {
    return new TimeUnitFormat(*this);
}


TimeUnitFormat& 
TimeUnitFormat::operator=(const TimeUnitFormat& other) {
    if (this == &other) {
        return *this;
    }
    delete fNumberFormat;
    for (TimeUnit::UTimeUnitFields i = TimeUnit::UTIMEUNIT_YEAR;
         i < TimeUnit::UTIMEUNIT_FIELD_COUNT;
         i = (TimeUnit::UTimeUnitFields)(i+1)) {
        deleteHash(fTimeUnitToCountToPatterns[i]);
        fTimeUnitToCountToPatterns[i] = NULL;
    }
    delete fPluralRules;
    if (other.fNumberFormat) {
        fNumberFormat = (NumberFormat*)other.fNumberFormat->clone();
    } else {
        fNumberFormat = NULL;
    }
    fLocale = other.fLocale;
    for (TimeUnit::UTimeUnitFields i = TimeUnit::UTIMEUNIT_YEAR;
         i < TimeUnit::UTIMEUNIT_FIELD_COUNT;
         i = (TimeUnit::UTimeUnitFields)(i+1)) {
        UErrorCode status = U_ZERO_ERROR;
        fTimeUnitToCountToPatterns[i] = initHash(status);
        if (U_SUCCESS(status)) {
            copyHash(other.fTimeUnitToCountToPatterns[i], fTimeUnitToCountToPatterns[i], status);
        } else {
            delete fTimeUnitToCountToPatterns[i];
            fTimeUnitToCountToPatterns[i] = NULL;
        }
    } 
    if (other.fPluralRules) {
        fPluralRules = (PluralRules*)other.fPluralRules->clone();
    } else {
        fPluralRules = NULL;
    }
    fStyle = other.fStyle;
    return *this;
}


UBool 
TimeUnitFormat::operator==(const Format& other) const {
    if (typeid(*this) == typeid(other)) {
        TimeUnitFormat* fmt = (TimeUnitFormat*)&other;
        UBool ret =  ( ((fNumberFormat && fmt->fNumberFormat && *fNumberFormat == *fmt->fNumberFormat)
                            || fNumberFormat == fmt->fNumberFormat ) 
                        && fLocale == fmt->fLocale 
                        && ((fPluralRules && fmt->fPluralRules && *fPluralRules == *fmt->fPluralRules) 
                            || fPluralRules == fmt->fPluralRules) 
                        && fStyle == fmt->fStyle); 
        if (ret) {
            for (TimeUnit::UTimeUnitFields i = TimeUnit::UTIMEUNIT_YEAR;
                 i < TimeUnit::UTIMEUNIT_FIELD_COUNT && ret;
                 i = (TimeUnit::UTimeUnitFields)(i+1)) {
                ret = fTimeUnitToCountToPatterns[i]->equals(*(fmt->fTimeUnitToCountToPatterns[i]));
            }
        }
        return ret;
    }
    return false;
}


UnicodeString& 
TimeUnitFormat::format(const Formattable& obj, UnicodeString& toAppendTo,
                       FieldPosition& pos, UErrorCode& status) const {
    if (U_FAILURE(status)) {
        return toAppendTo;
    }
    if (obj.getType() == Formattable::kObject) {
        const UObject* formatObj = obj.getObject();
        const TimeUnitAmount* amount = dynamic_cast<const TimeUnitAmount*>(formatObj);
        if (amount != NULL){
            Hashtable* countToPattern = fTimeUnitToCountToPatterns[amount->getTimeUnitField()];
            double number;
            const Formattable& amtNumber = amount->getNumber();
            if (amtNumber.getType() == Formattable::kDouble) {
                number = amtNumber.getDouble();
            } else if (amtNumber.getType() == Formattable::kLong) {
                number = amtNumber.getLong();
            } else {
                status = U_ILLEGAL_ARGUMENT_ERROR;
                return toAppendTo;
            }
            UnicodeString count = fPluralRules->select(number);
#ifdef TMUTFMT_DEBUG
            char result[1000];
            count.extract(0, count.length(), result, "UTF-8");
            std::cout << "number: " << number << "; format plural count: " << result << "\n";           
#endif
            MessageFormat* pattern = ((MessageFormat**)countToPattern->get(count))[fStyle];
            Formattable formattable[1];
            formattable[0].setDouble(number);
            return pattern->format(formattable, 1, toAppendTo, pos, status);
        }
    }
    status = U_ILLEGAL_ARGUMENT_ERROR;
    return toAppendTo;
}


void 
TimeUnitFormat::parseObject(const UnicodeString& source, 
                            Formattable& result,
                            ParsePosition& pos) const {
    double resultNumber = -1; 
    UBool withNumberFormat = false;
    TimeUnit::UTimeUnitFields resultTimeUnit = TimeUnit::UTIMEUNIT_FIELD_COUNT;
    int32_t oldPos = pos.getIndex();
    int32_t newPos = -1;
    int32_t longestParseDistance = 0;
    UnicodeString* countOfLongestMatch = NULL;
#ifdef TMUTFMT_DEBUG
    char res[1000];
    source.extract(0, source.length(), res, "UTF-8");
    std::cout << "parse source: " << res << "\n";           
#endif
    // parse by iterating through all available patterns
    // and looking for the longest match.
    for (TimeUnit::UTimeUnitFields i = TimeUnit::UTIMEUNIT_YEAR;
         i < TimeUnit::UTIMEUNIT_FIELD_COUNT;
         i = (TimeUnit::UTimeUnitFields)(i+1)) {
        Hashtable* countToPatterns = fTimeUnitToCountToPatterns[i];
        int32_t elemPos = -1;
        const UHashElement* elem = NULL;
        while ((elem = countToPatterns->nextElement(elemPos)) != NULL){
            const UHashTok keyTok = elem->key;
            UnicodeString* count = (UnicodeString*)keyTok.pointer;
#ifdef TMUTFMT_DEBUG
            count->extract(0, count->length(), res, "UTF-8");
            std::cout << "parse plural count: " << res << "\n";           
#endif
            const UHashTok valueTok = elem->value;
            // the value is a pair of MessageFormat*
            MessageFormat** patterns = (MessageFormat**)valueTok.pointer;
            for (EStyle style = kFull; style < kTotal; style = (EStyle)(style + 1)) {
                MessageFormat* pattern = patterns[style];
                pos.setErrorIndex(-1);
                pos.setIndex(oldPos);
                // see if we can parse
                Formattable parsed;
                pattern->parseObject(source, parsed, pos);
                if (pos.getErrorIndex() != -1 || pos.getIndex() == oldPos) {
                    continue;
                }
    #ifdef TMUTFMT_DEBUG
                std::cout << "parsed.getType: " << parsed.getType() << "\n";
    #endif
                double tmpNumber = 0;
                if (pattern->getArgTypeCount() != 0) {
                    // pattern with Number as beginning, such as "{0} d".
                    // check to make sure that the timeUnit is consistent
                    Formattable& temp = parsed[0];
                    if (temp.getType() == Formattable::kDouble) {
                        tmpNumber = temp.getDouble();
                    } else if (temp.getType() == Formattable::kLong) {
                        tmpNumber = temp.getLong();
                    } else {
                        continue;
                    }
                    UnicodeString select = fPluralRules->select(tmpNumber);
    #ifdef TMUTFMT_DEBUG
                    select.extract(0, select.length(), res, "UTF-8");
                    std::cout << "parse plural select count: " << res << "\n"; 
    #endif
                    if (*count != select) {
                        continue;
                    }
                }
                int32_t parseDistance = pos.getIndex() - oldPos;
                if (parseDistance > longestParseDistance) {
                    if (pattern->getArgTypeCount() != 0) {
                        resultNumber = tmpNumber;
                        withNumberFormat = true;
                    } else {
                        withNumberFormat = false;
                    }
                    resultTimeUnit = i;
                    newPos = pos.getIndex();
                    longestParseDistance = parseDistance;
                    countOfLongestMatch = count;
                }
            }
        }
    }
    /* After find the longest match, parse the number.
     * Result number could be null for the pattern without number pattern.
     * such as unit pattern in Arabic.
     * When result number is null, use plural rule to set the number.
     */
    if (withNumberFormat == false && longestParseDistance != 0) {
        // set the number using plurrual count
        if ( *countOfLongestMatch == PLURAL_COUNT_ZERO ) {
            resultNumber = 0;
        } else if ( *countOfLongestMatch == PLURAL_COUNT_ONE ) {
            resultNumber = 1;
        } else if ( *countOfLongestMatch == PLURAL_COUNT_TWO ) {
            resultNumber = 2;
        } else {
            // should not happen.
            // TODO: how to handle?
            resultNumber = 3;
        }
    }
    if (longestParseDistance == 0) {
        pos.setIndex(oldPos);
        pos.setErrorIndex(0);
    } else {
        UErrorCode status = U_ZERO_ERROR;
        TimeUnitAmount* tmutamt = new TimeUnitAmount(resultNumber, resultTimeUnit, status);
        if (U_SUCCESS(status)) {
            result.adoptObject(tmutamt);
            pos.setIndex(newPos);
            pos.setErrorIndex(-1);
        } else {
            pos.setIndex(oldPos);
            pos.setErrorIndex(0);
        }
    }
}


void
TimeUnitFormat::create(const Locale& locale, EStyle style, UErrorCode& status) {
    if (U_FAILURE(status)) {
        return;
    }
    if (style < kFull || style > kAbbreviate) {
        status = U_ILLEGAL_ARGUMENT_ERROR;
        return;
    }
    fStyle = style;
    fLocale = locale;
    for (TimeUnit::UTimeUnitFields i = TimeUnit::UTIMEUNIT_YEAR;
         i < TimeUnit::UTIMEUNIT_FIELD_COUNT;
         i = (TimeUnit::UTimeUnitFields)(i+1)) {
        fTimeUnitToCountToPatterns[i] = NULL;
    }
    //TODO: format() and parseObj() are const member functions,
    //so, can not do lazy initialization in C++.
    //setup has to be done in constructors.
    //and here, the behavior is not consistent with Java.
    //In Java, create an empty instance does not setup locale as
    //default locale. If it followed by setNumberFormat(),
    //in format(), the locale will set up as the locale in fNumberFormat.
    //But in C++, this sets the locale as the default locale. 
    setup(status);
}

void 
TimeUnitFormat::setup(UErrorCode& err) {
    initDataMembers(err);
    readFromCurrentLocale(kFull, gUnitsTag, err);
    checkConsistency(kFull, gUnitsTag, err);
    readFromCurrentLocale(kAbbreviate, gShortUnitsTag, err);
    checkConsistency(kAbbreviate, gShortUnitsTag, err);
}


void
TimeUnitFormat::initDataMembers(UErrorCode& err){
    if (U_FAILURE(err)) {
        return;
    }
    if (fNumberFormat == NULL) {
        fNumberFormat = NumberFormat::createInstance(fLocale, err);
    }
    delete fPluralRules;
    fPluralRules = PluralRules::forLocale(fLocale, err);
    for (TimeUnit::UTimeUnitFields i = TimeUnit::UTIMEUNIT_YEAR;
         i < TimeUnit::UTIMEUNIT_FIELD_COUNT;
         i = (TimeUnit::UTimeUnitFields)(i+1)) {
        deleteHash(fTimeUnitToCountToPatterns[i]);
        fTimeUnitToCountToPatterns[i] = NULL;
    }
}




void
TimeUnitFormat::readFromCurrentLocale(EStyle style, const char* key, UErrorCode& err) {
    if (U_FAILURE(err)) {
        return;
    }
    // fill timeUnitToCountToPatterns from resource file
    // err is used to indicate wrong status except missing resource.
    // status is an error code used in resource lookup.
    // status does not affect "err".
    UErrorCode status = U_ZERO_ERROR;
    UResourceBundle *rb, *unitsRes;
    rb = ures_open(NULL, fLocale.getName(), &status);
    unitsRes = ures_getByKey(rb, key, NULL, &status);
    if (U_FAILURE(status)) {
        ures_close(unitsRes);
        ures_close(rb);
        return;
    }
    int32_t size = ures_getSize(unitsRes);
    for ( int32_t index = 0; index < size; ++index) {
        // resource of one time unit
        UResourceBundle* oneTimeUnit = ures_getByIndex(unitsRes, index,
                                                       NULL, &status);
        if (U_SUCCESS(status)) {
            const char* timeUnitName = ures_getKey(oneTimeUnit);
            if (timeUnitName == NULL) {
                ures_close(oneTimeUnit);
                continue;
            }
            UResourceBundle* countsToPatternRB = ures_getByKey(unitsRes, 
                                                             timeUnitName, 
                                                             NULL, &status);
            if (countsToPatternRB == NULL || U_FAILURE(status)) {
                ures_close(countsToPatternRB);
                ures_close(oneTimeUnit);
                continue;
            }
            TimeUnit::UTimeUnitFields timeUnitField = TimeUnit::UTIMEUNIT_FIELD_COUNT;
            if ( uprv_strcmp(timeUnitName, gTimeUnitYear) == 0 ) {
                timeUnitField = TimeUnit::UTIMEUNIT_YEAR;
            } else if ( uprv_strcmp(timeUnitName, gTimeUnitMonth) == 0 ) {
                timeUnitField = TimeUnit::UTIMEUNIT_MONTH;
            } else if ( uprv_strcmp(timeUnitName, gTimeUnitDay) == 0 ) {
                timeUnitField = TimeUnit::UTIMEUNIT_DAY;
            } else if ( uprv_strcmp(timeUnitName, gTimeUnitHour) == 0 ) {
                timeUnitField = TimeUnit::UTIMEUNIT_HOUR;
            } else if ( uprv_strcmp(timeUnitName, gTimeUnitMinute) == 0 ) {
                timeUnitField = TimeUnit::UTIMEUNIT_MINUTE;
            } else if ( uprv_strcmp(timeUnitName, gTimeUnitSecond) == 0 ) {
                timeUnitField = TimeUnit::UTIMEUNIT_SECOND;
            } else if ( uprv_strcmp(timeUnitName, gTimeUnitWeek) == 0 ) {
                timeUnitField = TimeUnit::UTIMEUNIT_WEEK;
            } else {
                ures_close(countsToPatternRB);
                ures_close(oneTimeUnit);
                continue;
            }
            Hashtable* countToPatterns = fTimeUnitToCountToPatterns[timeUnitField];
            if (countToPatterns == NULL) {
                countToPatterns = initHash(err);
                if (U_FAILURE(err)) {
                    ures_close(countsToPatternRB);
                    ures_close(oneTimeUnit);
                    delete countToPatterns;
                    break;
                }
            }
            int32_t count = ures_getSize(countsToPatternRB);
            const UChar* pattern;
            const char*  pluralCount;
            int32_t ptLength; 
            for ( int32_t pluralIndex = 0; pluralIndex < count; ++pluralIndex) {
                // resource of count to pattern
                pattern = ures_getNextString(countsToPatternRB, &ptLength,
                                             &pluralCount, &status);
                if (U_FAILURE(status)) {
                    continue;
                }
                MessageFormat* messageFormat = new MessageFormat(pattern, fLocale, err);
                if ( U_SUCCESS(err) ) {
                  if (fNumberFormat != NULL) {
                    messageFormat->setFormat(0, *fNumberFormat);
                  }
                  MessageFormat** formatters = (MessageFormat**)countToPatterns->get(pluralCount);
                  if (formatters == NULL) {
                    formatters = (MessageFormat**)uprv_malloc(kTotal*sizeof(MessageFormat*));
                    formatters[kFull] = NULL;
                    formatters[kAbbreviate] = NULL;
                    countToPatterns->put(pluralCount, formatters, err);
                    if (U_FAILURE(err)) {
                        uprv_free(formatters);
                    }
                  } 
                  if (U_SUCCESS(err)) {
                      //delete formatters[style];
                      formatters[style] = messageFormat;
                  }
                } 
                if (U_FAILURE(err)) {
                    ures_close(countsToPatternRB);
                    ures_close(oneTimeUnit);
                    ures_close(unitsRes);
                    ures_close(rb);
                    delete messageFormat;
                    delete countToPatterns;
                    return;
                }
            }
            if (fTimeUnitToCountToPatterns[timeUnitField] == NULL) {
                fTimeUnitToCountToPatterns[timeUnitField] = countToPatterns;
            }
            ures_close(countsToPatternRB);
        }
        ures_close(oneTimeUnit);
    }
    ures_close(unitsRes);
    ures_close(rb);
}


void 
TimeUnitFormat::checkConsistency(EStyle style, const char* key, UErrorCode& err) {
    if (U_FAILURE(err)) {
        return;
    }
    // there should be patterns for each plural rule in each time unit.
    // For each time unit, 
    //     for each plural rule, following is unit pattern fall-back rule:
    //         ( for example: "one" hour )
    //         look for its unit pattern in its locale tree.
    //         if pattern is not found in its own locale, such as de_DE,
    //         look for the pattern in its parent, such as de,
    //         keep looking till found or till root.
    //         if the pattern is not found in root either,
    //         fallback to plural count "other",
    //         look for the pattern of "other" in the locale tree:
    //         "de_DE" to "de" to "root".
    //         If not found, fall back to value of 
    //         static variable DEFAULT_PATTERN_FOR_xxx, such as "{0} h". 
    //
    // Following is consistency check to create pattern for each
    // plural rule in each time unit using above fall-back rule.
    //
    StringEnumeration* keywords = fPluralRules->getKeywords(err);
    if (U_SUCCESS(err)) {
        const char* pluralCount;
        while ((pluralCount = keywords->next(NULL, err)) != NULL) {
            if ( U_SUCCESS(err) ) {
                for (int32_t i = 0; i < TimeUnit::UTIMEUNIT_FIELD_COUNT; ++i) {
                    // for each time unit, 
                    // get all the patterns for each plural rule in this locale.
                    Hashtable* countToPatterns = fTimeUnitToCountToPatterns[i];
                    if ( countToPatterns == NULL ) {
                        countToPatterns = initHash(err);
                        if (U_FAILURE(err)) {
                            delete countToPatterns;
                            return;
                        }
                        fTimeUnitToCountToPatterns[i] = countToPatterns;
                    }
                    MessageFormat** formatters = (MessageFormat**)countToPatterns->get(pluralCount);
                    if( formatters == NULL || formatters[style] == NULL ) {
                        // look through parents
                        const char* localeName = fLocale.getName();
                        searchInLocaleChain(style, key, localeName,
                                            (TimeUnit::UTimeUnitFields)i, 
                                            pluralCount, pluralCount, 
                                            countToPatterns, err);
                    }
                }
            }
        }
    }
    delete keywords;
}



// srcPluralCount is the original plural count on which the pattern is
// searched for.
// searchPluralCount is the fallback plural count.
// For example, to search for pattern for ""one" hour",
// "one" is the srcPluralCount,
// if the pattern is not found even in root, fallback to 
// using patterns of plural count "other", 
// then, "other" is the searchPluralCount.
void 
TimeUnitFormat::searchInLocaleChain(EStyle style, const char* key, const char* localeName,
                                TimeUnit::UTimeUnitFields srcTimeUnitField,
                                const char* srcPluralCount,
                                const char* searchPluralCount, 
                                Hashtable* countToPatterns,
                                UErrorCode& err) {
    if (U_FAILURE(err)) {
        return;
    }
    UErrorCode status = U_ZERO_ERROR;
    char parentLocale[ULOC_FULLNAME_CAPACITY];
    uprv_strcpy(parentLocale, localeName);
    int32_t locNameLen;
    while ((locNameLen = uloc_getParent(parentLocale, parentLocale,
                                        ULOC_FULLNAME_CAPACITY, &status)) >= 0){
        // look for pattern for srcPluralCount in locale tree
        UResourceBundle *rb, *unitsRes, *countsToPatternRB;
        rb = ures_open(NULL, parentLocale, &status);
        unitsRes = ures_getByKey(rb, key, NULL, &status);
        const char* timeUnitName = getTimeUnitName(srcTimeUnitField, status);
        countsToPatternRB = ures_getByKey(unitsRes, timeUnitName, NULL, &status);
        const UChar* pattern;
        int32_t      ptLength;
        pattern = ures_getStringByKeyWithFallback(countsToPatternRB, searchPluralCount, &ptLength, &status);
        if (U_SUCCESS(status)) {
            //found
            MessageFormat* messageFormat = new MessageFormat(pattern, fLocale, err);
            if (U_SUCCESS(err)) {
                if (fNumberFormat != NULL) {
                    messageFormat->setFormat(0, *fNumberFormat);
                }
                MessageFormat** formatters = (MessageFormat**)countToPatterns->get(srcPluralCount);
                if (formatters == NULL) {
                    formatters = (MessageFormat**)uprv_malloc(kTotal*sizeof(MessageFormat*));
                    formatters[kFull] = NULL;
                    formatters[kAbbreviate] = NULL;
                    countToPatterns->put(srcPluralCount, formatters, err);
                    if (U_FAILURE(err)) {
                        uprv_free(formatters);
                        delete messageFormat;
                    }
                } 
                if (U_SUCCESS(err)) {
                    //delete formatters[style];
                    formatters[style] = messageFormat;
                }
            } else {
                delete messageFormat;
            }
            ures_close(countsToPatternRB);
            ures_close(unitsRes);
            ures_close(rb);
            return;
        }
        ures_close(countsToPatternRB);
        ures_close(unitsRes);
        ures_close(rb);
        status = U_ZERO_ERROR;
        if ( locNameLen ==0 ) {
            break;
        }
    }

    // if no unitsShort resource was found even after fallback to root locale
    // then search the units resource fallback from the current level to root
    if ( locNameLen == 0 && uprv_strcmp(key, gShortUnitsTag) == 0) {
#ifdef TMUTFMT_DEBUG
        std::cout << "loop into searchInLocaleChain since Short-Long-Alternative \n";
#endif
        char pLocale[ULOC_FULLNAME_CAPACITY];
        uprv_strcpy(pLocale, localeName);
        // Add an underscore at the tail of locale name,
        // so that searchInLocaleChain will check the current locale before falling back
        uprv_strcat(pLocale, "_");
        searchInLocaleChain(style, gUnitsTag, pLocale, srcTimeUnitField, srcPluralCount,
                             searchPluralCount, countToPatterns, err);
        if (countToPatterns != NULL) {
            MessageFormat** formatters = (MessageFormat**)countToPatterns->get(srcPluralCount);
            if (formatters != NULL && formatters[style] != NULL) return;
        }
    }

    // if not found the pattern for this plural count at all,
    // fall-back to plural count "other"
    if ( uprv_strcmp(searchPluralCount, gPluralCountOther) == 0 ) {
        // set default fall back the same as the resource in root
        MessageFormat* messageFormat = NULL;
        if ( srcTimeUnitField == TimeUnit::UTIMEUNIT_SECOND ) {
            messageFormat = new MessageFormat(DEFAULT_PATTERN_FOR_SECOND, fLocale, err);
        } else if ( srcTimeUnitField == TimeUnit::UTIMEUNIT_MINUTE ) {
            messageFormat = new MessageFormat(DEFAULT_PATTERN_FOR_MINUTE, fLocale, err);
        } else if ( srcTimeUnitField == TimeUnit::UTIMEUNIT_HOUR ) {
            messageFormat = new MessageFormat(DEFAULT_PATTERN_FOR_HOUR, fLocale, err);
        } else if ( srcTimeUnitField == TimeUnit::UTIMEUNIT_WEEK ) {
            messageFormat = new MessageFormat(DEFAULT_PATTERN_FOR_WEEK, fLocale, err);
        } else if ( srcTimeUnitField == TimeUnit::UTIMEUNIT_DAY ) {
            messageFormat = new MessageFormat(DEFAULT_PATTERN_FOR_DAY, fLocale, err);
        } else if ( srcTimeUnitField == TimeUnit::UTIMEUNIT_MONTH ) {
            messageFormat = new MessageFormat(DEFAULT_PATTERN_FOR_MONTH, fLocale, err);
        } else if ( srcTimeUnitField == TimeUnit::UTIMEUNIT_YEAR ) {
            messageFormat = new MessageFormat(DEFAULT_PATTERN_FOR_YEAR, fLocale, err);
        }
        if (U_SUCCESS(err)) {
            if (fNumberFormat != NULL && messageFormat != NULL) {
                messageFormat->setFormat(0, *fNumberFormat);
            }
            MessageFormat** formatters = (MessageFormat**)countToPatterns->get(srcPluralCount);
            if (formatters == NULL) {
                formatters = (MessageFormat**)uprv_malloc(kTotal*sizeof(MessageFormat*));
                formatters[kFull] = NULL;
                formatters[kAbbreviate] = NULL;
                countToPatterns->put(srcPluralCount, formatters, err);
                if (U_FAILURE(err)) {
                    uprv_free(formatters);
                    delete messageFormat;
                }
            }
            if (U_SUCCESS(err)) {
                //delete formatters[style];
                formatters[style] = messageFormat;
            }
        } else {
            delete messageFormat;
        }
    } else {
        // fall back to rule "other", and search in parents
        searchInLocaleChain(style, key, localeName, srcTimeUnitField, srcPluralCount, 
                            gPluralCountOther, countToPatterns, err);
    }
}

void 
TimeUnitFormat::setLocale(const Locale& locale, UErrorCode& status) {
    if (U_SUCCESS(status) && fLocale != locale) {
        fLocale = locale;
        setup(status);
    }
}


void 
TimeUnitFormat::setNumberFormat(const NumberFormat& format, UErrorCode& status){
    if (U_FAILURE(status) || (fNumberFormat && format == *fNumberFormat)) {
        return;
    }
    delete fNumberFormat;
    fNumberFormat = (NumberFormat*)format.clone();
    // reset the number formatter in the fTimeUnitToCountToPatterns map
    for (TimeUnit::UTimeUnitFields i = TimeUnit::UTIMEUNIT_YEAR;
         i < TimeUnit::UTIMEUNIT_FIELD_COUNT;
         i = (TimeUnit::UTimeUnitFields)(i+1)) {
        int32_t pos = -1;
        const UHashElement* elem = NULL;
        while ((elem = fTimeUnitToCountToPatterns[i]->nextElement(pos)) != NULL){
            const UHashTok keyTok = elem->value;
            MessageFormat** pattern = (MessageFormat**)keyTok.pointer;
            pattern[kFull]->setFormat(0, format);
            pattern[kAbbreviate]->setFormat(0, format);
        }
    }
}


void
TimeUnitFormat::deleteHash(Hashtable* htable) {
    int32_t pos = -1;
    const UHashElement* element = NULL;
    if ( htable ) {
        while ( (element = htable->nextElement(pos)) != NULL ) {
            const UHashTok valueTok = element->value;
            const MessageFormat** value = (const MessageFormat**)valueTok.pointer;
            delete value[kFull];
            delete value[kAbbreviate];
            //delete[] value;
            uprv_free(value);
        }
    }
    delete htable;
}


void
TimeUnitFormat::copyHash(const Hashtable* source, Hashtable* target, UErrorCode& status) {
    if ( U_FAILURE(status) ) {
        return;
    }
    int32_t pos = -1;
    const UHashElement* element = NULL;
    if ( source ) {
        while ( (element = source->nextElement(pos)) != NULL ) {
            const UHashTok keyTok = element->key;
            const UnicodeString* key = (UnicodeString*)keyTok.pointer;
            const UHashTok valueTok = element->value;
            const MessageFormat** value = (const MessageFormat**)valueTok.pointer;
            MessageFormat** newVal = (MessageFormat**)uprv_malloc(kTotal*sizeof(MessageFormat*));
            newVal[0] = (MessageFormat*)value[0]->clone();
            newVal[1] = (MessageFormat*)value[1]->clone();
            target->put(UnicodeString(*key), newVal, status);
            if ( U_FAILURE(status) ) {
                delete newVal[0];
                delete newVal[1];
                uprv_free(newVal);
                return;
            }
        }
    }
}


U_CDECL_BEGIN 

/**
 * set hash table value comparator
 *
 * @param val1  one value in comparison
 * @param val2  the other value in comparison
 * @return      TRUE if 2 values are the same, FALSE otherwise
 */
static UBool U_CALLCONV tmutfmtHashTableValueComparator(UHashTok val1, UHashTok val2);

static UBool
U_CALLCONV tmutfmtHashTableValueComparator(UHashTok val1, UHashTok val2) {
    const MessageFormat** pattern1 = (const MessageFormat**)val1.pointer;
    const MessageFormat** pattern2 = (const MessageFormat**)val2.pointer;
    return *pattern1[0] == *pattern2[0] && *pattern1[1] == *pattern2[1];
}

U_CDECL_END

Hashtable*
TimeUnitFormat::initHash(UErrorCode& status) {
    if ( U_FAILURE(status) ) {
        return NULL;
    }
    Hashtable* hTable;
    if ( (hTable = new Hashtable(TRUE, status)) == NULL ) {
        status = U_MEMORY_ALLOCATION_ERROR;
        return NULL;
    }
    hTable->setValueComparator(tmutfmtHashTableValueComparator);
    return hTable;
}


const char*
TimeUnitFormat::getTimeUnitName(TimeUnit::UTimeUnitFields unitField, 
                                UErrorCode& status) {
    if (U_FAILURE(status)) {
        return NULL;
    }
    switch (unitField) {
      case TimeUnit::UTIMEUNIT_YEAR:
        return gTimeUnitYear;
      case TimeUnit::UTIMEUNIT_MONTH:
        return gTimeUnitMonth;
      case TimeUnit::UTIMEUNIT_DAY:
        return gTimeUnitDay;
      case TimeUnit::UTIMEUNIT_WEEK:
        return gTimeUnitWeek;
      case TimeUnit::UTIMEUNIT_HOUR:
        return gTimeUnitHour;
      case TimeUnit::UTIMEUNIT_MINUTE:
        return gTimeUnitMinute;
      case TimeUnit::UTIMEUNIT_SECOND:
        return gTimeUnitSecond;
      default:
        status = U_ILLEGAL_ARGUMENT_ERROR;
        return NULL;
    }
}

U_NAMESPACE_END

#endif
