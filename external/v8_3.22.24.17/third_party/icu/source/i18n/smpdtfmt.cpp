/*
*******************************************************************************
* Copyright (C) 1997-2010, International Business Machines Corporation and    *
* others. All Rights Reserved.                                                *
*******************************************************************************
*
* File SMPDTFMT.CPP
*
* Modification History:
*
*   Date        Name        Description
*   02/19/97    aliu        Converted from java.
*   03/31/97    aliu        Modified extensively to work with 50 locales.
*   04/01/97    aliu        Added support for centuries.
*   07/09/97    helena      Made ParsePosition into a class.
*   07/21/98    stephen     Added initializeDefaultCentury.
*                             Removed getZoneIndex (added in DateFormatSymbols)
*                             Removed subParseLong
*                             Removed chk
*  02/22/99     stephen     Removed character literals for EBCDIC safety
*   10/14/99    aliu        Updated 2-digit year parsing so that only "00" thru
*                           "99" are recognized. {j28 4182066}
*   11/15/99    weiv        Added support for week of year/day of week format
********************************************************************************
*/

#define ZID_KEY_MAX 128

#include "unicode/utypes.h"

#if !UCONFIG_NO_FORMATTING

#include "unicode/smpdtfmt.h"
#include "unicode/dtfmtsym.h"
#include "unicode/ures.h"
#include "unicode/msgfmt.h"
#include "unicode/calendar.h"
#include "unicode/gregocal.h"
#include "unicode/timezone.h"
#include "unicode/decimfmt.h"
#include "unicode/dcfmtsym.h"
#include "unicode/uchar.h"
#include "unicode/ustring.h"
#include "unicode/basictz.h"
#include "unicode/simpletz.h"
#include "unicode/rbtz.h"
#include "unicode/vtzone.h"
#include "olsontz.h"
#include "util.h"
#include "fphdlimp.h"
#include "gregoimp.h"
#include "hebrwcal.h"
#include "cstring.h"
#include "uassert.h"
#include "zstrfmt.h"
#include "cmemory.h"
#include "umutex.h"
#include <float.h>

#if defined( U_DEBUG_CALSVC ) || defined (U_DEBUG_CAL)
#include <stdio.h>
#endif

// *****************************************************************************
// class SimpleDateFormat
// *****************************************************************************

U_NAMESPACE_BEGIN

static const UChar PATTERN_CHAR_BASE = 0x40;

/**
 * Last-resort string to use for "GMT" when constructing time zone strings.
 */
// For time zones that have no names, use strings GMT+minutes and
// GMT-minutes. For instance, in France the time zone is GMT+60.
// Also accepted are GMT+H:MM or GMT-H:MM.
static const UChar gGmt[]      = {0x0047, 0x004D, 0x0054, 0x0000};         // "GMT"
static const UChar gGmtPlus[]  = {0x0047, 0x004D, 0x0054, 0x002B, 0x0000}; // "GMT+"
static const UChar gGmtMinus[] = {0x0047, 0x004D, 0x0054, 0x002D, 0x0000}; // "GMT-"
static const UChar gDefGmtPat[]       = {0x0047, 0x004D, 0x0054, 0x007B, 0x0030, 0x007D, 0x0000}; /* GMT{0} */
static const UChar gDefGmtNegHmsPat[] = {0x002D, 0x0048, 0x0048, 0x003A, 0x006D, 0x006D, 0x003A, 0x0073, 0x0073, 0x0000}; /* -HH:mm:ss */
static const UChar gDefGmtNegHmPat[]  = {0x002D, 0x0048, 0x0048, 0x003A, 0x006D, 0x006D, 0x0000}; /* -HH:mm */
static const UChar gDefGmtPosHmsPat[] = {0x002B, 0x0048, 0x0048, 0x003A, 0x006D, 0x006D, 0x003A, 0x0073, 0x0073, 0x0000}; /* +HH:mm:ss */
static const UChar gDefGmtPosHmPat[]  = {0x002B, 0x0048, 0x0048, 0x003A, 0x006D, 0x006D, 0x0000}; /* +HH:mm */
static const UChar gUt[]       = {0x0055, 0x0054, 0x0000};  // "UT"
static const UChar gUtc[]      = {0x0055, 0x0054, 0x0043, 0x0000};  // "UT"

typedef enum GmtPatSize {
    kGmtLen = 3,
    kGmtPatLen = 6,
    kNegHmsLen = 9,
    kNegHmLen = 6,
    kPosHmsLen = 9,
    kPosHmLen = 6,
    kUtLen = 2,
    kUtcLen = 3
} GmtPatSize;

// Stuff needed for numbering system overrides

typedef enum OvrStrType {
    kOvrStrDate = 0,
    kOvrStrTime = 1,
    kOvrStrBoth = 2
} OvrStrType;

static const UDateFormatField kDateFields[] = {
    UDAT_YEAR_FIELD,
    UDAT_MONTH_FIELD,
    UDAT_DATE_FIELD,
    UDAT_DAY_OF_YEAR_FIELD,
    UDAT_DAY_OF_WEEK_IN_MONTH_FIELD,
    UDAT_WEEK_OF_YEAR_FIELD,
    UDAT_WEEK_OF_MONTH_FIELD,
    UDAT_YEAR_WOY_FIELD,
    UDAT_EXTENDED_YEAR_FIELD,
    UDAT_JULIAN_DAY_FIELD,
    UDAT_STANDALONE_DAY_FIELD,
    UDAT_STANDALONE_MONTH_FIELD,
    UDAT_QUARTER_FIELD,
    UDAT_STANDALONE_QUARTER_FIELD };
static const int8_t kDateFieldsCount = 13;

static const UDateFormatField kTimeFields[] = {
    UDAT_HOUR_OF_DAY1_FIELD,
    UDAT_HOUR_OF_DAY0_FIELD,
    UDAT_MINUTE_FIELD,
    UDAT_SECOND_FIELD,
    UDAT_FRACTIONAL_SECOND_FIELD,
    UDAT_HOUR1_FIELD,
    UDAT_HOUR0_FIELD,
    UDAT_MILLISECONDS_IN_DAY_FIELD,
    UDAT_TIMEZONE_RFC_FIELD };
static const int8_t kTimeFieldsCount = 9;


// This is a pattern-of-last-resort used when we can't load a usable pattern out
// of a resource.
static const UChar gDefaultPattern[] =
{
    0x79, 0x79, 0x79, 0x79, 0x4D, 0x4D, 0x64, 0x64, 0x20, 0x68, 0x68, 0x3A, 0x6D, 0x6D, 0x20, 0x61, 0
};  /* "yyyyMMdd hh:mm a" */

// This prefix is designed to NEVER MATCH real text, in order to
// suppress the parsing of negative numbers.  Adjust as needed (if
// this becomes valid Unicode).
static const UChar SUPPRESS_NEGATIVE_PREFIX[] = {0xAB00, 0};

/**
 * These are the tags we expect to see in normal resource bundle files associated
 * with a locale.
 */
static const char gDateTimePatternsTag[]="DateTimePatterns";

static const UChar gEtcUTC[] = {0x45, 0x74, 0x63, 0x2F, 0x55, 0x54, 0x43, 0x00}; // "Etc/UTC"
static const UChar QUOTE = 0x27; // Single quote

static UMTX LOCK;

UOBJECT_DEFINE_RTTI_IMPLEMENTATION(SimpleDateFormat)

//----------------------------------------------------------------------

SimpleDateFormat::~SimpleDateFormat()
{
    delete fSymbols;
    if (fGMTFormatters) {
        for (int32_t i = 0; i < kNumGMTFormatters; i++) {
            if (fGMTFormatters[i]) {
                delete fGMTFormatters[i];
            }
        }
        uprv_free(fGMTFormatters);

    }
    if (fNumberFormatters) {
        uprv_free(fNumberFormatters);
    }

    while (fOverrideList) {
        NSOverride *cur = fOverrideList;
        fOverrideList = cur->next;
        delete cur->nf;
        uprv_free(cur);
    }
}

//----------------------------------------------------------------------

SimpleDateFormat::SimpleDateFormat(UErrorCode& status)
  :   fLocale(Locale::getDefault()),
      fSymbols(NULL),
      fGMTFormatters(NULL),
      fNumberFormatters(NULL),
      fOverrideList(NULL)
{
    construct(kShort, (EStyle) (kShort + kDateOffset), fLocale, status);
    initializeDefaultCentury();
}

//----------------------------------------------------------------------

SimpleDateFormat::SimpleDateFormat(const UnicodeString& pattern,
                                   UErrorCode &status)
:   fPattern(pattern),
    fLocale(Locale::getDefault()),
    fSymbols(NULL),
    fGMTFormatters(NULL),
    fNumberFormatters(NULL),
    fOverrideList(NULL)
{
    fDateOverride.setToBogus();
    fTimeOverride.setToBogus();
    initializeSymbols(fLocale, initializeCalendar(NULL,fLocale,status), status);
    initialize(fLocale, status);
    initializeDefaultCentury();

}
//----------------------------------------------------------------------

SimpleDateFormat::SimpleDateFormat(const UnicodeString& pattern,
                                   const UnicodeString& override,
                                   UErrorCode &status)
:   fPattern(pattern),
    fLocale(Locale::getDefault()),
    fSymbols(NULL),
    fGMTFormatters(NULL),
    fNumberFormatters(NULL),
    fOverrideList(NULL)
{
    fDateOverride.setTo(override);
    fTimeOverride.setToBogus();
    initializeSymbols(fLocale, initializeCalendar(NULL,fLocale,status), status);
    initialize(fLocale, status);
    initializeDefaultCentury();

    processOverrideString(fLocale,override,kOvrStrBoth,status);

}

//----------------------------------------------------------------------

SimpleDateFormat::SimpleDateFormat(const UnicodeString& pattern,
                                   const Locale& locale,
                                   UErrorCode& status)
:   fPattern(pattern),
    fLocale(locale),
    fGMTFormatters(NULL),
    fNumberFormatters(NULL),
    fOverrideList(NULL)
{

    fDateOverride.setToBogus();
    fTimeOverride.setToBogus();

    initializeSymbols(fLocale, initializeCalendar(NULL,fLocale,status), status);
    initialize(fLocale, status);
    initializeDefaultCentury();
}

//----------------------------------------------------------------------

SimpleDateFormat::SimpleDateFormat(const UnicodeString& pattern,
                                   const UnicodeString& override,
                                   const Locale& locale,
                                   UErrorCode& status)
:   fPattern(pattern),
    fLocale(locale),
    fGMTFormatters(NULL),
    fNumberFormatters(NULL),
    fOverrideList(NULL)
{

    fDateOverride.setTo(override);
    fTimeOverride.setToBogus();

    initializeSymbols(fLocale, initializeCalendar(NULL,fLocale,status), status);
    initialize(fLocale, status);
    initializeDefaultCentury();

    processOverrideString(locale,override,kOvrStrBoth,status);

}

//----------------------------------------------------------------------

SimpleDateFormat::SimpleDateFormat(const UnicodeString& pattern,
                                   DateFormatSymbols* symbolsToAdopt,
                                   UErrorCode& status)
:   fPattern(pattern),
    fLocale(Locale::getDefault()),
    fSymbols(symbolsToAdopt),
    fGMTFormatters(NULL),
    fNumberFormatters(NULL),
    fOverrideList(NULL)
{

    fDateOverride.setToBogus();
    fTimeOverride.setToBogus();

    initializeCalendar(NULL,fLocale,status);
    initialize(fLocale, status);
    initializeDefaultCentury();
}

//----------------------------------------------------------------------

SimpleDateFormat::SimpleDateFormat(const UnicodeString& pattern,
                                   const DateFormatSymbols& symbols,
                                   UErrorCode& status)
:   fPattern(pattern),
    fLocale(Locale::getDefault()),
    fSymbols(new DateFormatSymbols(symbols)),
    fGMTFormatters(NULL),
    fNumberFormatters(NULL),
    fOverrideList(NULL)
{

    fDateOverride.setToBogus();
    fTimeOverride.setToBogus();

    initializeCalendar(NULL, fLocale, status);
    initialize(fLocale, status);
    initializeDefaultCentury();
}

//----------------------------------------------------------------------

// Not for public consumption; used by DateFormat
SimpleDateFormat::SimpleDateFormat(EStyle timeStyle,
                                   EStyle dateStyle,
                                   const Locale& locale,
                                   UErrorCode& status)
:   fLocale(locale),
    fSymbols(NULL),
    fGMTFormatters(NULL),
    fNumberFormatters(NULL),
    fOverrideList(NULL)
{
    construct(timeStyle, dateStyle, fLocale, status);
    if(U_SUCCESS(status)) {
      initializeDefaultCentury();
    }
}

//----------------------------------------------------------------------

/**
 * Not for public consumption; used by DateFormat.  This constructor
 * never fails.  If the resource data is not available, it uses the
 * the last resort symbols.
 */
SimpleDateFormat::SimpleDateFormat(const Locale& locale,
                                   UErrorCode& status)
:   fPattern(gDefaultPattern),
    fLocale(locale),
    fSymbols(NULL),
    fGMTFormatters(NULL),
    fNumberFormatters(NULL),
    fOverrideList(NULL)
{
    if (U_FAILURE(status)) return;
    initializeSymbols(fLocale, initializeCalendar(NULL, fLocale, status),status);
    if (U_FAILURE(status))
    {
        status = U_ZERO_ERROR;
        delete fSymbols;
        // This constructor doesn't fail; it uses last resort data
        fSymbols = new DateFormatSymbols(status);
        /* test for NULL */
        if (fSymbols == 0) {
            status = U_MEMORY_ALLOCATION_ERROR;
            return;
        }
    }

    fDateOverride.setToBogus();
    fTimeOverride.setToBogus();

    initialize(fLocale, status);
    if(U_SUCCESS(status)) {
      initializeDefaultCentury();
    }
}

//----------------------------------------------------------------------

SimpleDateFormat::SimpleDateFormat(const SimpleDateFormat& other)
:   DateFormat(other),
    fSymbols(NULL),
    fGMTFormatters(NULL),
    fNumberFormatters(NULL),
    fOverrideList(NULL)
{
    *this = other;
}

//----------------------------------------------------------------------

SimpleDateFormat& SimpleDateFormat::operator=(const SimpleDateFormat& other)
{
    if (this == &other) {
        return *this;
    }
    DateFormat::operator=(other);

    delete fSymbols;
    fSymbols = NULL;

    if (other.fSymbols)
        fSymbols = new DateFormatSymbols(*other.fSymbols);

    fDefaultCenturyStart         = other.fDefaultCenturyStart;
    fDefaultCenturyStartYear     = other.fDefaultCenturyStartYear;
    fHaveDefaultCentury          = other.fHaveDefaultCentury;

    fPattern = other.fPattern;

    return *this;
}

//----------------------------------------------------------------------

Format*
SimpleDateFormat::clone() const
{
    return new SimpleDateFormat(*this);
}

//----------------------------------------------------------------------

UBool
SimpleDateFormat::operator==(const Format& other) const
{
    if (DateFormat::operator==(other)) {
        // DateFormat::operator== guarantees following cast is safe
        SimpleDateFormat* that = (SimpleDateFormat*)&other;
        return (fPattern             == that->fPattern &&
                fSymbols             != NULL && // Check for pathological object
                that->fSymbols       != NULL && // Check for pathological object
                *fSymbols            == *that->fSymbols &&
                fHaveDefaultCentury  == that->fHaveDefaultCentury &&
                fDefaultCenturyStart == that->fDefaultCenturyStart);
    }
    return FALSE;
}

//----------------------------------------------------------------------

void SimpleDateFormat::construct(EStyle timeStyle,
                                 EStyle dateStyle,
                                 const Locale& locale,
                                 UErrorCode& status)
{
    // called by several constructors to load pattern data from the resources
    if (U_FAILURE(status)) return;

    // We will need the calendar to know what type of symbols to load.
    initializeCalendar(NULL, locale, status);
    if (U_FAILURE(status)) return;

    CalendarData calData(locale, fCalendar?fCalendar->getType():NULL, status);
    UResourceBundle *dateTimePatterns = calData.getByKey(gDateTimePatternsTag, status);
    UResourceBundle *currentBundle;

    if (U_FAILURE(status)) return;

    if (ures_getSize(dateTimePatterns) <= kDateTime)
    {
        status = U_INVALID_FORMAT_ERROR;
        return;
    }

    setLocaleIDs(ures_getLocaleByType(dateTimePatterns, ULOC_VALID_LOCALE, &status),
                 ures_getLocaleByType(dateTimePatterns, ULOC_ACTUAL_LOCALE, &status));

    // create a symbols object from the locale
    initializeSymbols(locale,fCalendar, status);
    if (U_FAILURE(status)) return;
    /* test for NULL */
    if (fSymbols == 0) {
        status = U_MEMORY_ALLOCATION_ERROR;
        return;
    }

    const UChar *resStr,*ovrStr;
    int32_t resStrLen,ovrStrLen = 0;
    fDateOverride.setToBogus();
    fTimeOverride.setToBogus();

    // if the pattern should include both date and time information, use the date/time
    // pattern string as a guide to tell use how to glue together the appropriate date
    // and time pattern strings.  The actual gluing-together is handled by a convenience
    // method on MessageFormat.
    if ((timeStyle != kNone) && (dateStyle != kNone))
    {
        Formattable timeDateArray[2];

        // use Formattable::adoptString() so that we can use fastCopyFrom()
        // instead of Formattable::setString()'s unaware, safe, deep string clone
        // see Jitterbug 2296

        currentBundle = ures_getByIndex(dateTimePatterns, (int32_t)timeStyle, NULL, &status);
        if (U_FAILURE(status)) {
           status = U_INVALID_FORMAT_ERROR;
           return;
        }
        switch (ures_getType(currentBundle)) {
            case URES_STRING: {
               resStr = ures_getString(currentBundle, &resStrLen, &status);
               break;
            }
            case URES_ARRAY: {
               resStr = ures_getStringByIndex(currentBundle, 0, &resStrLen, &status);
               ovrStr = ures_getStringByIndex(currentBundle, 1, &ovrStrLen, &status);
               fTimeOverride.setTo(TRUE, ovrStr, ovrStrLen);
               break;
            }
            default: {
               status = U_INVALID_FORMAT_ERROR;
               ures_close(currentBundle);
               return;
            }
        }
        ures_close(currentBundle);

        UnicodeString *tempus1 = new UnicodeString(TRUE, resStr, resStrLen);
        // NULL pointer check
        if (tempus1 == NULL) {
            status = U_MEMORY_ALLOCATION_ERROR;
            return;
        }
        timeDateArray[0].adoptString(tempus1);

        currentBundle = ures_getByIndex(dateTimePatterns, (int32_t)dateStyle, NULL, &status);
        if (U_FAILURE(status)) {
           status = U_INVALID_FORMAT_ERROR;
           return;
        }
        switch (ures_getType(currentBundle)) {
            case URES_STRING: {
               resStr = ures_getString(currentBundle, &resStrLen, &status);
               break;
            }
            case URES_ARRAY: {
               resStr = ures_getStringByIndex(currentBundle, 0, &resStrLen, &status);
               ovrStr = ures_getStringByIndex(currentBundle, 1, &ovrStrLen, &status);
               fDateOverride.setTo(TRUE, ovrStr, ovrStrLen);
               break;
            }
            default: {
               status = U_INVALID_FORMAT_ERROR;
               ures_close(currentBundle);
               return;
            }
        }
        ures_close(currentBundle);

        UnicodeString *tempus2 = new UnicodeString(TRUE, resStr, resStrLen);
        // Null pointer check
        if (tempus2 == NULL) {
            status = U_MEMORY_ALLOCATION_ERROR;
            return;
        }
        timeDateArray[1].adoptString(tempus2);

        int32_t glueIndex = kDateTime;
        int32_t patternsSize = ures_getSize(dateTimePatterns);
        if (patternsSize >= (kDateTimeOffset + kShort + 1)) {
            // Get proper date time format
            glueIndex = (int32_t)(kDateTimeOffset + (dateStyle - kDateOffset));
        }

        resStr = ures_getStringByIndex(dateTimePatterns, glueIndex, &resStrLen, &status);
        MessageFormat::format(UnicodeString(TRUE, resStr, resStrLen), timeDateArray, 2, fPattern, status);
    }
    // if the pattern includes just time data or just date date, load the appropriate
    // pattern string from the resources
    // setTo() - see DateFormatSymbols::assignArray comments
    else if (timeStyle != kNone) {
        currentBundle = ures_getByIndex(dateTimePatterns, (int32_t)timeStyle, NULL, &status);
        if (U_FAILURE(status)) {
           status = U_INVALID_FORMAT_ERROR;
           return;
        }
        switch (ures_getType(currentBundle)) {
            case URES_STRING: {
               resStr = ures_getString(currentBundle, &resStrLen, &status);
               break;
            }
            case URES_ARRAY: {
               resStr = ures_getStringByIndex(currentBundle, 0, &resStrLen, &status);
               ovrStr = ures_getStringByIndex(currentBundle, 1, &ovrStrLen, &status);
               fDateOverride.setTo(TRUE, ovrStr, ovrStrLen);
               break;
            }
            default: {
               status = U_INVALID_FORMAT_ERROR;
                ures_close(currentBundle);
               return;
            }
        }
        fPattern.setTo(TRUE, resStr, resStrLen);
        ures_close(currentBundle);
    }
    else if (dateStyle != kNone) {
        currentBundle = ures_getByIndex(dateTimePatterns, (int32_t)dateStyle, NULL, &status);
        if (U_FAILURE(status)) {
           status = U_INVALID_FORMAT_ERROR;
           return;
        }
        switch (ures_getType(currentBundle)) {
            case URES_STRING: {
               resStr = ures_getString(currentBundle, &resStrLen, &status);
               break;
            }
            case URES_ARRAY: {
               resStr = ures_getStringByIndex(currentBundle, 0, &resStrLen, &status);
               ovrStr = ures_getStringByIndex(currentBundle, 1, &ovrStrLen, &status);
               fDateOverride.setTo(TRUE, ovrStr, ovrStrLen);
               break;
            }
            default: {
               status = U_INVALID_FORMAT_ERROR;
               ures_close(currentBundle);
               return;
            }
        }
        fPattern.setTo(TRUE, resStr, resStrLen);
        ures_close(currentBundle);
    }

    // and if it includes _neither_, that's an error
    else
        status = U_INVALID_FORMAT_ERROR;

    // finally, finish initializing by creating a Calendar and a NumberFormat
    initialize(locale, status);
}

//----------------------------------------------------------------------

Calendar*
SimpleDateFormat::initializeCalendar(TimeZone* adoptZone, const Locale& locale, UErrorCode& status)
{
    if(!U_FAILURE(status)) {
        fCalendar = Calendar::createInstance(adoptZone?adoptZone:TimeZone::createDefault(), locale, status);
    }
    if (U_SUCCESS(status) && fCalendar == NULL) {
        status = U_MEMORY_ALLOCATION_ERROR;
    }
    return fCalendar;
}

void
SimpleDateFormat::initializeSymbols(const Locale& locale, Calendar* calendar, UErrorCode& status)
{
  if(U_FAILURE(status)) {
    fSymbols = NULL;
  } else {
    // pass in calendar type - use NULL (default) if no calendar set (or err).
    fSymbols = new DateFormatSymbols(locale, calendar?calendar->getType() :NULL , status);
    // Null pointer check
    if (fSymbols == NULL) {
    	status = U_MEMORY_ALLOCATION_ERROR;
    	return;
    }
  }
}

void
SimpleDateFormat::initialize(const Locale& locale,
                             UErrorCode& status)
{
    if (U_FAILURE(status)) return;

    // We don't need to check that the row count is >= 1, since all 2d arrays have at
    // least one row
    fNumberFormat = NumberFormat::createInstance(locale, status);
    if (fNumberFormat != NULL && U_SUCCESS(status))
    {
        // no matter what the locale's default number format looked like, we want
        // to modify it so that it doesn't use thousands separators, doesn't always
        // show the decimal point, and recognizes integers only when parsing

        fNumberFormat->setGroupingUsed(FALSE);
        DecimalFormat* decfmt = dynamic_cast<DecimalFormat*>(fNumberFormat);
        if (decfmt != NULL) {
            decfmt->setDecimalSeparatorAlwaysShown(FALSE);
        }
        fNumberFormat->setParseIntegerOnly(TRUE);
        fNumberFormat->setMinimumFractionDigits(0); // To prevent "Jan 1.00, 1997.00"

        initNumberFormatters(locale,status);

    }
    else if (U_SUCCESS(status))
    {
        status = U_MISSING_RESOURCE_ERROR;
    }
}

/* Initialize the fields we use to disambiguate ambiguous years. Separate
 * so we can call it from readObject().
 */
void SimpleDateFormat::initializeDefaultCentury()
{
  if(fCalendar) {
    fHaveDefaultCentury = fCalendar->haveDefaultCentury();
    if(fHaveDefaultCentury) {
      fDefaultCenturyStart = fCalendar->defaultCenturyStart();
      fDefaultCenturyStartYear = fCalendar->defaultCenturyStartYear();
    } else {
      fDefaultCenturyStart = DBL_MIN;
      fDefaultCenturyStartYear = -1;
    }
  }
}

/* Define one-century window into which to disambiguate dates using
 * two-digit years. Make public in JDK 1.2.
 */
void SimpleDateFormat::parseAmbiguousDatesAsAfter(UDate startDate, UErrorCode& status)
{
    if(U_FAILURE(status)) {
        return;
    }
    if(!fCalendar) {
      status = U_ILLEGAL_ARGUMENT_ERROR;
      return;
    }

    fCalendar->setTime(startDate, status);
    if(U_SUCCESS(status)) {
        fHaveDefaultCentury = TRUE;
        fDefaultCenturyStart = startDate;
        fDefaultCenturyStartYear = fCalendar->get(UCAL_YEAR, status);
    }
}

//----------------------------------------------------------------------

UnicodeString&
SimpleDateFormat::format(Calendar& cal, UnicodeString& appendTo, FieldPosition& pos) const
{
  UErrorCode status = U_ZERO_ERROR;
  FieldPositionOnlyHandler handler(pos);
  return _format(cal, appendTo, handler, status);
}

//----------------------------------------------------------------------

UnicodeString&
SimpleDateFormat::format(Calendar& cal, UnicodeString& appendTo,
                         FieldPositionIterator* posIter, UErrorCode& status) const
{
  FieldPositionIteratorHandler handler(posIter, status);
  return _format(cal, appendTo, handler, status);
}

//----------------------------------------------------------------------

UnicodeString&
SimpleDateFormat::_format(Calendar& cal, UnicodeString& appendTo, FieldPositionHandler& handler,
                          UErrorCode& status) const
{
    Calendar *workCal = &cal;
    TimeZone *backupTZ = NULL;
    if (&cal != fCalendar && uprv_strcmp(cal.getType(), fCalendar->getType()) != 0) {
        // Different calendar type
        // We use the time and time zone from the input calendar, but
        // do not use the input calendar for field calculation.
        UDate t = cal.getTime(status);
        fCalendar->setTime(t, status);
        backupTZ = fCalendar->getTimeZone().clone();
        fCalendar->setTimeZone(cal.getTimeZone());
        workCal = fCalendar;
    }

    UBool inQuote = FALSE;
    UChar prevCh = 0;
    int32_t count = 0;

    // loop through the pattern string character by character
    for (int32_t i = 0; i < fPattern.length() && U_SUCCESS(status); ++i) {
        UChar ch = fPattern[i];

        // Use subFormat() to format a repeated pattern character
        // when a different pattern or non-pattern character is seen
        if (ch != prevCh && count > 0) {
            subFormat(appendTo, prevCh, count, handler, *workCal, status);
            count = 0;
        }
        if (ch == QUOTE) {
            // Consecutive single quotes are a single quote literal,
            // either outside of quotes or between quotes
            if ((i+1) < fPattern.length() && fPattern[i+1] == QUOTE) {
                appendTo += (UChar)QUOTE;
                ++i;
            } else {
                inQuote = ! inQuote;
            }
        }
        else if ( ! inQuote && ((ch >= 0x0061 /*'a'*/ && ch <= 0x007A /*'z'*/)
                    || (ch >= 0x0041 /*'A'*/ && ch <= 0x005A /*'Z'*/))) {
            // ch is a date-time pattern character to be interpreted
            // by subFormat(); count the number of times it is repeated
            prevCh = ch;
            ++count;
        }
        else {
            // Append quoted characters and unquoted non-pattern characters
            appendTo += ch;
        }
    }

    // Format the last item in the pattern, if any
    if (count > 0) {
        subFormat(appendTo, prevCh, count, handler, *workCal, status);
    }

    if (backupTZ != NULL) {
        // Restore the original time zone
        fCalendar->adoptTimeZone(backupTZ);
    }

    return appendTo;
}

//----------------------------------------------------------------------

/* Map calendar field into calendar field level.
 * the larger the level, the smaller the field unit.
 * For example, UCAL_ERA level is 0, UCAL_YEAR level is 10,
 * UCAL_MONTH level is 20.
 * NOTE: if new fields adds in, the table needs to update.
 */
const int32_t
SimpleDateFormat::fgCalendarFieldToLevel[] =
{
    /*GyM*/ 0, 10, 20,
    /*wW*/ 20, 30,
    /*dDEF*/ 30, 20, 30, 30,
    /*ahHm*/ 40, 50, 50, 60,
    /*sS..*/ 70, 80,
    /*z?Y*/ 0, 0, 10,
    /*eug*/ 30, 10, 0,
    /*A*/ 40
};


/* Map calendar field LETTER into calendar field level.
 * the larger the level, the smaller the field unit.
 * NOTE: if new fields adds in, the table needs to update.
 */
const int32_t
SimpleDateFormat::fgPatternCharToLevel[] = {
    //       A   B   C   D   E   F   G   H   I   J   K   L   M   N   O
        -1, 40, -1, -1, 20,  30, 30,  0,  50, -1, -1, 50, 20,  20, -1, -1,
    //   P   Q   R   S   T   U   V   W   X   Y   Z
        -1, 20, -1,  80, -1, -1, 0, 30, -1, 10, 0, -1, -1, -1, -1, -1,
    //       a   b   c   d   e   f   g   h   i   j   k   l   m   n   o
        -1, 40, -1, 30,  30, 30, -1, 0, 50, -1, -1,  50, -1,  60, -1, -1,
    //   p   q   r   s   t   u   v   w   x   y   z
        -1, 20, -1,  70, -1, 10, 0, 20, -1,  10, 0, -1, -1, -1, -1, -1
};


// Map index into pattern character string to Calendar field number.
const UCalendarDateFields
SimpleDateFormat::fgPatternIndexToCalendarField[] =
{
    /*GyM*/ UCAL_ERA, UCAL_YEAR, UCAL_MONTH,
    /*dkH*/ UCAL_DATE, UCAL_HOUR_OF_DAY, UCAL_HOUR_OF_DAY,
    /*msS*/ UCAL_MINUTE, UCAL_SECOND, UCAL_MILLISECOND,
    /*EDF*/ UCAL_DAY_OF_WEEK, UCAL_DAY_OF_YEAR, UCAL_DAY_OF_WEEK_IN_MONTH,
    /*wWa*/ UCAL_WEEK_OF_YEAR, UCAL_WEEK_OF_MONTH, UCAL_AM_PM,
    /*hKz*/ UCAL_HOUR, UCAL_HOUR, UCAL_ZONE_OFFSET,
    /*Yeu*/ UCAL_YEAR_WOY, UCAL_DOW_LOCAL, UCAL_EXTENDED_YEAR,
    /*gAZ*/ UCAL_JULIAN_DAY, UCAL_MILLISECONDS_IN_DAY, UCAL_ZONE_OFFSET,
    /*v*/   UCAL_ZONE_OFFSET,
    /*c*/   UCAL_DOW_LOCAL,
    /*L*/   UCAL_MONTH,
    /*Q*/   UCAL_MONTH,
    /*q*/   UCAL_MONTH,
    /*V*/   UCAL_ZONE_OFFSET,
};

// Map index into pattern character string to DateFormat field number
const UDateFormatField
SimpleDateFormat::fgPatternIndexToDateFormatField[] = {
    /*GyM*/ UDAT_ERA_FIELD, UDAT_YEAR_FIELD, UDAT_MONTH_FIELD,
    /*dkH*/ UDAT_DATE_FIELD, UDAT_HOUR_OF_DAY1_FIELD, UDAT_HOUR_OF_DAY0_FIELD,
    /*msS*/ UDAT_MINUTE_FIELD, UDAT_SECOND_FIELD, UDAT_FRACTIONAL_SECOND_FIELD,
    /*EDF*/ UDAT_DAY_OF_WEEK_FIELD, UDAT_DAY_OF_YEAR_FIELD, UDAT_DAY_OF_WEEK_IN_MONTH_FIELD,
    /*wWa*/ UDAT_WEEK_OF_YEAR_FIELD, UDAT_WEEK_OF_MONTH_FIELD, UDAT_AM_PM_FIELD,
    /*hKz*/ UDAT_HOUR1_FIELD, UDAT_HOUR0_FIELD, UDAT_TIMEZONE_FIELD,
    /*Yeu*/ UDAT_YEAR_WOY_FIELD, UDAT_DOW_LOCAL_FIELD, UDAT_EXTENDED_YEAR_FIELD,
    /*gAZ*/ UDAT_JULIAN_DAY_FIELD, UDAT_MILLISECONDS_IN_DAY_FIELD, UDAT_TIMEZONE_RFC_FIELD,
    /*v*/   UDAT_TIMEZONE_GENERIC_FIELD,
    /*c*/   UDAT_STANDALONE_DAY_FIELD,
    /*L*/   UDAT_STANDALONE_MONTH_FIELD,
    /*Q*/   UDAT_QUARTER_FIELD,
    /*q*/   UDAT_STANDALONE_QUARTER_FIELD,
    /*V*/   UDAT_TIMEZONE_SPECIAL_FIELD,
};

//----------------------------------------------------------------------

/**
 * Append symbols[value] to dst.  Make sure the array index is not out
 * of bounds.
 */
static inline void
_appendSymbol(UnicodeString& dst,
              int32_t value,
              const UnicodeString* symbols,
              int32_t symbolsCount) {
    U_ASSERT(0 <= value && value < symbolsCount);
    if (0 <= value && value < symbolsCount) {
        dst += symbols[value];
    }
}

//---------------------------------------------------------------------
void
SimpleDateFormat::appendGMT(NumberFormat *currentNumberFormat,UnicodeString &appendTo, Calendar& cal, UErrorCode& status) const{
    int32_t offset = cal.get(UCAL_ZONE_OFFSET, status) + cal.get(UCAL_DST_OFFSET, status);
    if (U_FAILURE(status)) {
        return;
    }
    if (isDefaultGMTFormat()) {
        formatGMTDefault(currentNumberFormat,appendTo, offset);
    } else {
        ((SimpleDateFormat*)this)->initGMTFormatters(status);
        if (U_SUCCESS(status)) {
            int32_t type;
            if (offset < 0) {
                offset = -offset;
                type = (offset % U_MILLIS_PER_MINUTE) == 0 ? kGMTNegativeHM : kGMTNegativeHMS;
            } else {
                type = (offset % U_MILLIS_PER_MINUTE) == 0 ? kGMTPositiveHM : kGMTPositiveHMS;
            }
            Formattable param(offset, Formattable::kIsDate);
            FieldPosition fpos(0);
            fGMTFormatters[type]->format(&param, 1, appendTo, fpos, status);
        }
    }
}

int32_t
SimpleDateFormat::parseGMT(const UnicodeString &text, ParsePosition &pos) const {
    if (!isDefaultGMTFormat()) {
        int32_t start = pos.getIndex();

        // Quick check
        UBool prefixMatch = FALSE;
        int32_t prefixLen = fSymbols->fGmtFormat.indexOf((UChar)0x007B /* '{' */);
        if (prefixLen > 0 && text.compare(start, prefixLen, fSymbols->fGmtFormat, 0, prefixLen) == 0) {
            prefixMatch = TRUE;
        }
        if (prefixMatch) {
            // Prefix matched
            UErrorCode status = U_ZERO_ERROR;
            ((SimpleDateFormat*)this)->initGMTFormatters(status);
            if (U_SUCCESS(status)) {
                Formattable parsed;
                int32_t parsedCount;

                // Try negative Hms
                fGMTFormatters[kGMTNegativeHMS]->parseObject(text, parsed, pos);
                if (pos.getErrorIndex() == -1 &&
                    (pos.getIndex() - start) >= fGMTFormatHmsMinLen[kGMTNegativeHMSMinLenIdx]) {
                    parsed.getArray(parsedCount);
                    if (parsedCount == 1 && parsed[0].getType() == Formattable::kDate) {
                        return (int32_t)(-1 * (int64_t)parsed[0].getDate());
                    }
                }

                // Reset ParsePosition
                pos.setIndex(start);
                pos.setErrorIndex(-1);

                // Try positive Hms
                fGMTFormatters[kGMTPositiveHMS]->parseObject(text, parsed, pos);
                if (pos.getErrorIndex() == -1 &&
                    (pos.getIndex() - start) >= fGMTFormatHmsMinLen[kGMTPositiveHMSMinLenIdx]) {
                    parsed.getArray(parsedCount);
                    if (parsedCount == 1 && parsed[0].getType() == Formattable::kDate) {
                        return (int32_t)((int64_t)parsed[0].getDate());
                    }
                }

                // Reset ParsePosition
                pos.setIndex(start);
                pos.setErrorIndex(-1);

                // Try negative Hm
                fGMTFormatters[kGMTNegativeHM]->parseObject(text, parsed, pos);
                if (pos.getErrorIndex() == -1 && pos.getIndex() > start) {
                    parsed.getArray(parsedCount);
                    if (parsedCount == 1 && parsed[0].getType() == Formattable::kDate) {
                        return (int32_t)(-1 * (int64_t)parsed[0].getDate());
                    }
                }

                // Reset ParsePosition
                pos.setIndex(start);
                pos.setErrorIndex(-1);

                // Try positive Hm
                fGMTFormatters[kGMTPositiveHM]->parseObject(text, parsed, pos);
                if (pos.getErrorIndex() == -1 && pos.getIndex() > start) {
                    parsed.getArray(parsedCount);
                    if (parsedCount == 1 && parsed[0].getType() == Formattable::kDate) {
                        return (int32_t)((int64_t)parsed[0].getDate());
                    }
                }

                // Reset ParsePosition
                pos.setIndex(start);
                pos.setErrorIndex(-1);
            }
            // fall through to the default GMT parsing method
        }
    }
    return parseGMTDefault(text, pos);
}

void
SimpleDateFormat::formatGMTDefault(NumberFormat *currentNumberFormat,UnicodeString &appendTo, int32_t offset) const {
    if (offset < 0) {
        appendTo += gGmtMinus;
        offset = -offset; // suppress the '-' sign for text display.
    }else{
        appendTo += gGmtPlus;
    }

    offset /= U_MILLIS_PER_SECOND; // now in seconds
    int32_t sec = offset % 60;
    offset /= 60;
    int32_t min = offset % 60;
    int32_t hour = offset / 60;


    zeroPaddingNumber(currentNumberFormat,appendTo, hour, 2, 2);
    appendTo += (UChar)0x003A /*':'*/;
    zeroPaddingNumber(currentNumberFormat,appendTo, min, 2, 2);
    if (sec != 0) {
        appendTo += (UChar)0x003A /*':'*/;
        zeroPaddingNumber(currentNumberFormat,appendTo, sec, 2, 2);
    }
}

int32_t
SimpleDateFormat::parseGMTDefault(const UnicodeString &text, ParsePosition &pos) const {
    int32_t start = pos.getIndex();
    NumberFormat *currentNumberFormat = getNumberFormatByIndex(UDAT_TIMEZONE_RFC_FIELD);

    if (start + kUtLen + 1 >= text.length()) {
        pos.setErrorIndex(start);
        return 0;
    }

    int32_t cur = start;
    // "GMT"
    if (text.compare(start, kGmtLen, gGmt) == 0) {
        cur += kGmtLen;
    } else if (text.compare(start, kUtLen, gUt) == 0) {
        cur += kUtLen;
    } else {
        pos.setErrorIndex(start);
        return 0;
    }
    // Sign
    UBool negative = FALSE;
    if (text.charAt(cur) == (UChar)0x002D /* minus */) {
        negative = TRUE;
    } else if (text.charAt(cur) != (UChar)0x002B /* plus */) {
        pos.setErrorIndex(cur);
        return 0;
    }
    cur++;

    // Numbers
    int32_t numLen;
    pos.setIndex(cur);

    Formattable number;
    parseInt(text, number, 6, pos, FALSE,currentNumberFormat);
    numLen = pos.getIndex() - cur;

    if (numLen <= 0) {
        pos.setIndex(start);
        pos.setErrorIndex(cur);
        return 0;
    }

    int32_t numVal = number.getLong();

    int32_t hour = 0;
    int32_t min = 0;
    int32_t sec = 0;

    if (numLen <= 2) {
        // H[H][:mm[:ss]]
        hour = numVal;
        cur += numLen;
        if (cur + 2 < text.length() && text.charAt(cur) == (UChar)0x003A /* colon */) {
            cur++;
            pos.setIndex(cur);
            parseInt(text, number, 2, pos, FALSE,currentNumberFormat);
            numLen = pos.getIndex() - cur;
            if (numLen == 2) {
                // got minute field
                min = number.getLong();
                cur += numLen;
                if (cur + 2 < text.length() && text.charAt(cur) == (UChar)0x003A /* colon */) {
                    cur++;
                    pos.setIndex(cur);
                    parseInt(text, number, 2, pos, FALSE,currentNumberFormat);
                    numLen = pos.getIndex() - cur;
                    if (numLen == 2) {
                        // got second field
                        sec = number.getLong();
                    } else {
                        // reset position
                        pos.setIndex(cur - 1);
                        pos.setErrorIndex(-1);
                    }
                }
            } else {
                // reset postion
                pos.setIndex(cur - 1);
                pos.setErrorIndex(-1);
            }
        }
    } else if (numLen == 3 || numLen == 4) {
        // Hmm or HHmm
        hour = numVal / 100;
        min = numVal % 100;
    } else if (numLen == 5 || numLen == 6) {
        // Hmmss or HHmmss
        hour = numVal / 10000;
        min = (numVal % 10000) / 100;
        sec = numVal % 100;
    } else {
        // HHmmss followed by bogus numbers
        pos.setIndex(cur + 6);

        int32_t shift = numLen - 6;
        while (shift > 0) {
            numVal /= 10;
            shift--;
        }
        hour = numVal / 10000;
        min = (numVal % 10000) / 100;
        sec = numVal % 100;
    }

    int32_t offset = ((hour*60 + min)*60 + sec)*1000;
    if (negative) {
        offset = -offset;
    }
    return offset;
}

UBool
SimpleDateFormat::isDefaultGMTFormat() const {
    // GMT pattern
    if (fSymbols->fGmtFormat.length() == 0) {
        // No GMT pattern is set
        return TRUE;
    } else if (fSymbols->fGmtFormat.compare(gDefGmtPat, kGmtPatLen) != 0) {
        return FALSE;
    }
    // Hour patterns
    if (fSymbols->fGmtHourFormats == NULL || fSymbols->fGmtHourFormatsCount != DateFormatSymbols::GMT_HOUR_COUNT) {
        // No Hour pattern is set
        return TRUE;
    } else if ((fSymbols->fGmtHourFormats[DateFormatSymbols::GMT_NEGATIVE_HMS].compare(gDefGmtNegHmsPat, kNegHmsLen) != 0)
        || (fSymbols->fGmtHourFormats[DateFormatSymbols::GMT_NEGATIVE_HM].compare(gDefGmtNegHmPat, kNegHmLen) != 0)
        || (fSymbols->fGmtHourFormats[DateFormatSymbols::GMT_POSITIVE_HMS].compare(gDefGmtPosHmsPat, kPosHmsLen) != 0)
        || (fSymbols->fGmtHourFormats[DateFormatSymbols::GMT_POSITIVE_HM].compare(gDefGmtPosHmPat, kPosHmLen) != 0)) {
        return FALSE;
    }
    return TRUE;
}

void
SimpleDateFormat::formatRFC822TZ(UnicodeString &appendTo, int32_t offset) const {
    UChar sign = 0x002B /* '+' */;
    if (offset < 0) {
        offset = -offset;
        sign = 0x002D /* '-' */;
    }
    appendTo.append(sign);

    int32_t offsetH = offset / U_MILLIS_PER_HOUR;
    offset = offset % U_MILLIS_PER_HOUR;
    int32_t offsetM = offset / U_MILLIS_PER_MINUTE;
    offset = offset % U_MILLIS_PER_MINUTE;
    int32_t offsetS = offset / U_MILLIS_PER_SECOND;

    int32_t num = 0, denom = 0;
    if (offsetS == 0) {
        offset = offsetH*100 + offsetM; // HHmm
        num = offset % 10000;
        denom = 1000;
    } else {
        offset = offsetH*10000 + offsetM*100 + offsetS; // HHmmss
        num = offset % 1000000;
        denom = 100000;
    }
    while (denom >= 1) {
        UChar digit = (UChar)0x0030 + (num / denom);
        appendTo.append(digit);
        num = num % denom;
        denom /= 10;
    }
}

void
SimpleDateFormat::initGMTFormatters(UErrorCode &status) {
    if (U_FAILURE(status)) {
        return;
    }
    umtx_lock(&LOCK);
    if (fGMTFormatters == NULL) {
        fGMTFormatters = (MessageFormat**)uprv_malloc(kNumGMTFormatters * sizeof(MessageFormat*));
        if (fGMTFormatters) {
            for (int32_t i = 0; i < kNumGMTFormatters; i++) {
                const UnicodeString *hourPattern = NULL; //initialized it to avoid warning
                switch (i) {
                    case kGMTNegativeHMS:
                        hourPattern = &(fSymbols->fGmtHourFormats[DateFormatSymbols::GMT_NEGATIVE_HMS]);
                        break;
                    case kGMTNegativeHM:
                        hourPattern = &(fSymbols->fGmtHourFormats[DateFormatSymbols::GMT_NEGATIVE_HM]);
                        break;
                    case kGMTPositiveHMS:
                        hourPattern = &(fSymbols->fGmtHourFormats[DateFormatSymbols::GMT_POSITIVE_HMS]);
                        break;
                    case kGMTPositiveHM:
                        hourPattern = &(fSymbols->fGmtHourFormats[DateFormatSymbols::GMT_POSITIVE_HM]);
                        break;
                }
                fGMTFormatters[i] = new MessageFormat(fSymbols->fGmtFormat, status);
                GregorianCalendar *gcal = new GregorianCalendar(TimeZone::createTimeZone(UnicodeString(gEtcUTC)), status);
                if (U_FAILURE(status)) {
                    break;
                }
                SimpleDateFormat *sdf = (SimpleDateFormat*)this->clone();
                sdf->adoptCalendar(gcal);
                sdf->applyPattern(*hourPattern);
                fGMTFormatters[i]->adoptFormat(0, sdf);

                // For parsing, we only allow Hms patterns to be equal or longer
                // than its length with fixed minutes/seconds digits.
                // See #6880
                if (i == kGMTNegativeHMS || i == kGMTPositiveHMS) {
                    UnicodeString tmp;
                    Formattable tmpParam(60*60*1000, Formattable::kIsDate);
                    FieldPosition fpos(0);
                    fGMTFormatters[i]->format(&tmpParam, 1, tmp, fpos, status);
                    if (U_FAILURE(status)) {
                        break;
                    }
                    if (i == kGMTNegativeHMS) {
                        fGMTFormatHmsMinLen[kGMTNegativeHMSMinLenIdx] = tmp.length();
                    } else {
                        fGMTFormatHmsMinLen[kGMTPositiveHMSMinLenIdx] = tmp.length();
                    }
                }
            }
        } else {
            status = U_MEMORY_ALLOCATION_ERROR;
        }
    }
    umtx_unlock(&LOCK);
}

void
SimpleDateFormat::initNumberFormatters(const Locale &locale,UErrorCode &status) {
    if (U_FAILURE(status)) {
        return;
    }
    if ( fDateOverride.isBogus() && fTimeOverride.isBogus() ) {
        return;
    }
    umtx_lock(&LOCK);
    if (fNumberFormatters == NULL) {
        fNumberFormatters = (NumberFormat**)uprv_malloc(UDAT_FIELD_COUNT * sizeof(NumberFormat*));
        if (fNumberFormatters) {
            for (int32_t i = 0; i < UDAT_FIELD_COUNT; i++) {
                fNumberFormatters[i] = fNumberFormat;
            }
        } else {
            status = U_MEMORY_ALLOCATION_ERROR;
        }
    }
    umtx_unlock(&LOCK);

    processOverrideString(locale,fDateOverride,kOvrStrDate,status);
    processOverrideString(locale,fTimeOverride,kOvrStrTime,status);

}

void
SimpleDateFormat::processOverrideString(const Locale &locale, const UnicodeString &str, int8_t type, UErrorCode &status) {
    if (str.isBogus()) {
        return;
    }
    int32_t start = 0;
    int32_t len;
    UnicodeString nsName;
    UnicodeString ovrField;
    UBool moreToProcess = TRUE;

    while (moreToProcess) {
        int32_t delimiterPosition = str.indexOf(ULOC_KEYWORD_ITEM_SEPARATOR_UNICODE,start);
        if (delimiterPosition == -1) {
            moreToProcess = FALSE;
            len = str.length() - start;
        } else {
            len = delimiterPosition - start;
        }
        UnicodeString currentString(str,start,len);
        int32_t equalSignPosition = currentString.indexOf(ULOC_KEYWORD_ASSIGN_UNICODE,0);
        if (equalSignPosition == -1) { // Simple override string such as "hebrew"
            nsName.setTo(currentString);
            ovrField.setToBogus();
        } else { // Field specific override string such as "y=hebrew"
            nsName.setTo(currentString,equalSignPosition+1);
            ovrField.setTo(currentString,0,1); // We just need the first character.
        }

        int32_t nsNameHash = nsName.hashCode();
        // See if the numbering system is in the override list, if not, then add it.
        NSOverride *cur = fOverrideList;
        NumberFormat *nf = NULL;
        UBool found = FALSE;
        while ( cur && !found ) {
            if ( cur->hash == nsNameHash ) {
                nf = cur->nf;
                found = TRUE;
            }
            cur = cur->next;
        }

        if (!found) {
           cur = (NSOverride *)uprv_malloc(sizeof(NSOverride));
           if (cur) {
               char kw[ULOC_KEYWORD_AND_VALUES_CAPACITY];
               uprv_strcpy(kw,"numbers=");
               nsName.extract(0,len,kw+8,ULOC_KEYWORD_AND_VALUES_CAPACITY-8,US_INV);

               Locale ovrLoc(locale.getLanguage(),locale.getCountry(),locale.getVariant(),kw);
               nf = NumberFormat::createInstance(ovrLoc,status);

               // no matter what the locale's default number format looked like, we want
               // to modify it so that it doesn't use thousands separators, doesn't always
               // show the decimal point, and recognizes integers only when parsing

               if (U_SUCCESS(status)) {
                   nf->setGroupingUsed(FALSE);
                   DecimalFormat* decfmt = dynamic_cast<DecimalFormat*>(nf);
                   if (decfmt != NULL) {
                       decfmt->setDecimalSeparatorAlwaysShown(FALSE);
                   }
                   nf->setParseIntegerOnly(TRUE);
                   nf->setMinimumFractionDigits(0); // To prevent "Jan 1.00, 1997.00"

                   cur->nf = nf;
                   cur->hash = nsNameHash;
                   cur->next = fOverrideList;
                   fOverrideList = cur;
               }
               else {
                   // clean up before returning
                   if (cur != NULL) {
                       uprv_free(cur);
                   }
                  return;
               }

           } else {
               status = U_MEMORY_ALLOCATION_ERROR;
               return;
           }
        }

        // Now that we have an appropriate number formatter, fill in the appropriate spaces in the
        // number formatters table.

        if (ovrField.isBogus()) {
            switch (type) {
                case kOvrStrDate:
                case kOvrStrBoth: {
                    for ( int8_t i=0 ; i<kDateFieldsCount; i++ ) {
                        fNumberFormatters[kDateFields[i]] = nf;
                    }
                    if (type==kOvrStrDate) {
                        break;
                    }
                }
                case kOvrStrTime : {
                    for ( int8_t i=0 ; i<kTimeFieldsCount; i++ ) {
                        fNumberFormatters[kTimeFields[i]] = nf;
                    }
                    break;
                }
            }
        } else {
           UChar ch = ovrField.charAt(0);
           UChar *patternCharPtr = u_strchr(DateFormatSymbols::getPatternUChars(), ch);
           UDateFormatField patternCharIndex;

           // if the pattern character is unrecognized, signal an error and bail out
           if (patternCharPtr == NULL) {
               status = U_INVALID_FORMAT_ERROR;
               return;
           }
           patternCharIndex = (UDateFormatField)(patternCharPtr - DateFormatSymbols::getPatternUChars());

           // Set the number formatter in the table
           fNumberFormatters[patternCharIndex] = nf;
        }

        start = delimiterPosition + 1;
    }
}
//---------------------------------------------------------------------
void
SimpleDateFormat::subFormat(UnicodeString &appendTo,
                            UChar ch,
                            int32_t count,
                            FieldPositionHandler& handler,
                            Calendar& cal,
                            UErrorCode& status) const
{
    if (U_FAILURE(status)) {
        return;
    }

    // this function gets called by format() to produce the appropriate substitution
    // text for an individual pattern symbol (e.g., "HH" or "yyyy")

    UChar *patternCharPtr = u_strchr(DateFormatSymbols::getPatternUChars(), ch);
    UDateFormatField patternCharIndex;
    const int32_t maxIntCount = 10;
    int32_t beginOffset = appendTo.length();
    NumberFormat *currentNumberFormat;

    UBool isHebrewCalendar = !strcmp(cal.getType(),"hebrew");

    // if the pattern character is unrecognized, signal an error and dump out
    if (patternCharPtr == NULL)
    {
        status = U_INVALID_FORMAT_ERROR;
        return;
    }

    patternCharIndex = (UDateFormatField)(patternCharPtr - DateFormatSymbols::getPatternUChars());
    UCalendarDateFields field = fgPatternIndexToCalendarField[patternCharIndex];
    int32_t value = cal.get(field, status);
    if (U_FAILURE(status)) {
        return;
    }

    currentNumberFormat = getNumberFormatByIndex(patternCharIndex);
    switch (patternCharIndex) {

    // for any "G" symbol, write out the appropriate era string
    // "GGGG" is wide era name, "GGGGG" is narrow era name, anything else is abbreviated name
    case UDAT_ERA_FIELD:
        if (count == 5)
           _appendSymbol(appendTo, value, fSymbols->fNarrowEras, fSymbols->fNarrowErasCount);
        else if (count == 4)
           _appendSymbol(appendTo, value, fSymbols->fEraNames, fSymbols->fEraNamesCount);
        else
           _appendSymbol(appendTo, value, fSymbols->fEras, fSymbols->fErasCount);
        break;

    // OLD: for "yyyy", write out the whole year; for "yy", write out the last 2 digits
    // NEW: UTS#35:
//Year  	y  	yy  	yyy  	yyyy  	yyyyy
//AD 1 	1 	01 	001 	0001 	00001
//AD 12 	12 	12 	012 	0012 	00012
//AD 123 	123 	23 	123 	0123 	00123
//AD 1234 	1234 	34 	1234 	1234 	01234
//AD 12345 	12345 	45 	12345 	12345 	12345
    case UDAT_YEAR_FIELD:
    case UDAT_YEAR_WOY_FIELD:
        if(count == 2)
            zeroPaddingNumber(currentNumberFormat, appendTo, value, 2, 2);
        else
            zeroPaddingNumber(currentNumberFormat, appendTo, value, count, maxIntCount);
        break;

    // for "MMMM", write out the whole month name, for "MMM", write out the month
    // abbreviation, for "M" or "MM", write out the month as a number with the
    // appropriate number of digits
    // for "MMMMM", use the narrow form
    case UDAT_MONTH_FIELD:
        if ( isHebrewCalendar ) {
           HebrewCalendar *hc = (HebrewCalendar*)&cal;
           if (hc->isLeapYear(hc->get(UCAL_YEAR,status)) && value == 6 && count >= 3 )
               value = 13; // Show alternate form for Adar II in leap years in Hebrew calendar.
           if (!hc->isLeapYear(hc->get(UCAL_YEAR,status)) && value >= 6 && count < 3 )
               value--; // Adjust the month number down 1 in Hebrew non-leap years, i.e. Adar is 6, not 7.
        }
        if (count == 5)
            _appendSymbol(appendTo, value, fSymbols->fNarrowMonths,
                          fSymbols->fNarrowMonthsCount);
        else if (count == 4)
            _appendSymbol(appendTo, value, fSymbols->fMonths,
                          fSymbols->fMonthsCount);
        else if (count == 3)
            _appendSymbol(appendTo, value, fSymbols->fShortMonths,
                          fSymbols->fShortMonthsCount);
        else
            zeroPaddingNumber(currentNumberFormat,appendTo, value + 1, count, maxIntCount);
        break;

    // for "LLLL", write out the whole month name, for "LLL", write out the month
    // abbreviation, for "L" or "LL", write out the month as a number with the
    // appropriate number of digits
    // for "LLLLL", use the narrow form
    case UDAT_STANDALONE_MONTH_FIELD:
        if (count == 5)
            _appendSymbol(appendTo, value, fSymbols->fStandaloneNarrowMonths,
                          fSymbols->fStandaloneNarrowMonthsCount);
        else if (count == 4)
            _appendSymbol(appendTo, value, fSymbols->fStandaloneMonths,
                          fSymbols->fStandaloneMonthsCount);
        else if (count == 3)
            _appendSymbol(appendTo, value, fSymbols->fStandaloneShortMonths,
                          fSymbols->fStandaloneShortMonthsCount);
        else
            zeroPaddingNumber(currentNumberFormat,appendTo, value + 1, count, maxIntCount);
        break;

    // for "k" and "kk", write out the hour, adjusting midnight to appear as "24"
    case UDAT_HOUR_OF_DAY1_FIELD:
        if (value == 0)
            zeroPaddingNumber(currentNumberFormat,appendTo, cal.getMaximum(UCAL_HOUR_OF_DAY) + 1, count, maxIntCount);
        else
            zeroPaddingNumber(currentNumberFormat,appendTo, value, count, maxIntCount);
        break;

    case UDAT_FRACTIONAL_SECOND_FIELD:
        // Fractional seconds left-justify
        {
            currentNumberFormat->setMinimumIntegerDigits((count > 3) ? 3 : count);
            currentNumberFormat->setMaximumIntegerDigits(maxIntCount);
            if (count == 1) {
                value /= 100;
            } else if (count == 2) {
                value /= 10;
            }
            FieldPosition p(0);
            currentNumberFormat->format(value, appendTo, p);
            if (count > 3) {
                currentNumberFormat->setMinimumIntegerDigits(count - 3);
                currentNumberFormat->format((int32_t)0, appendTo, p);
            }
        }
        break;

    // for "ee" or "e", use local numeric day-of-the-week
    // for "EEEEE" or "eeeee", write out the narrow day-of-the-week name
    // for "EEEE" or "eeee", write out the wide day-of-the-week name
    // for "EEE" or "EE" or "E" or "eee", write out the abbreviated day-of-the-week name
    case UDAT_DOW_LOCAL_FIELD:
        if ( count < 3 ) {
            zeroPaddingNumber(currentNumberFormat,appendTo, value, count, maxIntCount);
            break;
        }
        // fall through to EEEEE-EEE handling, but for that we don't want local day-of-week,
        // we want standard day-of-week, so first fix value to work for EEEEE-EEE.
        value = cal.get(UCAL_DAY_OF_WEEK, status);
        if (U_FAILURE(status)) {
            return;
        }
        // fall through, do not break here
    case UDAT_DAY_OF_WEEK_FIELD:
        if (count == 5)
            _appendSymbol(appendTo, value, fSymbols->fNarrowWeekdays,
                          fSymbols->fNarrowWeekdaysCount);
        else if (count == 4)
            _appendSymbol(appendTo, value, fSymbols->fWeekdays,
                          fSymbols->fWeekdaysCount);
        else
            _appendSymbol(appendTo, value, fSymbols->fShortWeekdays,
                          fSymbols->fShortWeekdaysCount);
        break;

    // for "ccc", write out the abbreviated day-of-the-week name
    // for "cccc", write out the wide day-of-the-week name
    // for "ccccc", use the narrow day-of-the-week name
    case UDAT_STANDALONE_DAY_FIELD:
        if ( count < 3 ) {
            zeroPaddingNumber(currentNumberFormat,appendTo, value, 1, maxIntCount);
            break;
        }
        // fall through to alpha DOW handling, but for that we don't want local day-of-week,
        // we want standard day-of-week, so first fix value.
        value = cal.get(UCAL_DAY_OF_WEEK, status);
        if (U_FAILURE(status)) {
            return;
        }
        if (count == 5)
            _appendSymbol(appendTo, value, fSymbols->fStandaloneNarrowWeekdays,
                          fSymbols->fStandaloneNarrowWeekdaysCount);
        else if (count == 4)
            _appendSymbol(appendTo, value, fSymbols->fStandaloneWeekdays,
                          fSymbols->fStandaloneWeekdaysCount);
        else // count == 3
            _appendSymbol(appendTo, value, fSymbols->fStandaloneShortWeekdays,
                          fSymbols->fStandaloneShortWeekdaysCount);
        break;

    // for and "a" symbol, write out the whole AM/PM string
    case UDAT_AM_PM_FIELD:
        _appendSymbol(appendTo, value, fSymbols->fAmPms,
                      fSymbols->fAmPmsCount);
        break;

    // for "h" and "hh", write out the hour, adjusting noon and midnight to show up
    // as "12"
    case UDAT_HOUR1_FIELD:
        if (value == 0)
            zeroPaddingNumber(currentNumberFormat,appendTo, cal.getLeastMaximum(UCAL_HOUR) + 1, count, maxIntCount);
        else
            zeroPaddingNumber(currentNumberFormat,appendTo, value, count, maxIntCount);
        break;

    // for the "z" symbols, we have to check our time zone data first.  If we have a
    // localized name for the time zone, then "zzzz" / "zzz" indicate whether
    // daylight time is in effect (long/short) and "zz" / "z" do not (long/short).
    // If we don't have a localized time zone name,
    // then the time zone shows up as "GMT+hh:mm" or "GMT-hh:mm" (where "hh:mm" is the
    // offset from GMT) regardless of how many z's were in the pattern symbol
    case UDAT_TIMEZONE_FIELD:
    case UDAT_TIMEZONE_GENERIC_FIELD:
    case UDAT_TIMEZONE_SPECIAL_FIELD:
        {
            UnicodeString zoneString;
            const ZoneStringFormat *zsf = fSymbols->getZoneStringFormat();
            if (zsf) {
                if (patternCharIndex == UDAT_TIMEZONE_FIELD) {
                    if (count < 4) {
                        // "z", "zz", "zzz"
                        zsf->getSpecificShortString(cal, TRUE /*commonly used only*/,
                            zoneString, status);
                    } else {
                        // "zzzz"
                        zsf->getSpecificLongString(cal, zoneString, status);
                    }
                } else if (patternCharIndex == UDAT_TIMEZONE_GENERIC_FIELD) {
                    if (count == 1) {
                        // "v"
                        zsf->getGenericShortString(cal, TRUE /*commonly used only*/,
                            zoneString, status);
                    } else if (count == 4) {
                        // "vvvv"
                        zsf->getGenericLongString(cal, zoneString, status);
                    }
                } else { // patternCharIndex == UDAT_TIMEZONE_SPECIAL_FIELD
                    if (count == 1) {
                        // "V"
                        zsf->getSpecificShortString(cal, FALSE /*ignore commonly used*/,
                            zoneString, status);
                    } else if (count == 4) {
                        // "VVVV"
                        zsf->getGenericLocationString(cal, zoneString, status);
                    }
                }
            }
            if (zoneString.isEmpty()) {
                appendGMT(currentNumberFormat,appendTo, cal, status);
            } else {
                appendTo += zoneString;
            }
        }
        break;

    case UDAT_TIMEZONE_RFC_FIELD: // 'Z' - TIMEZONE_RFC
        if (count < 4) {
            // RFC822 format, must use ASCII digits
            value = (cal.get(UCAL_ZONE_OFFSET, status) + cal.get(UCAL_DST_OFFSET, status));
            formatRFC822TZ(appendTo, value);
        } else {
            // long form, localized GMT pattern
            appendGMT(currentNumberFormat,appendTo, cal, status);
        }
        break;

    case UDAT_QUARTER_FIELD:
        if (count >= 4)
            _appendSymbol(appendTo, value/3, fSymbols->fQuarters,
                          fSymbols->fQuartersCount);
        else if (count == 3)
            _appendSymbol(appendTo, value/3, fSymbols->fShortQuarters,
                          fSymbols->fShortQuartersCount);
        else
            zeroPaddingNumber(currentNumberFormat,appendTo, (value/3) + 1, count, maxIntCount);
        break;

    case UDAT_STANDALONE_QUARTER_FIELD:
        if (count >= 4)
            _appendSymbol(appendTo, value/3, fSymbols->fStandaloneQuarters,
                          fSymbols->fStandaloneQuartersCount);
        else if (count == 3)
            _appendSymbol(appendTo, value/3, fSymbols->fStandaloneShortQuarters,
                          fSymbols->fStandaloneShortQuartersCount);
        else
            zeroPaddingNumber(currentNumberFormat,appendTo, (value/3) + 1, count, maxIntCount);
        break;


    // all of the other pattern symbols can be formatted as simple numbers with
    // appropriate zero padding
    default:
        zeroPaddingNumber(currentNumberFormat,appendTo, value, count, maxIntCount);
        break;
    }

    handler.addAttribute(fgPatternIndexToDateFormatField[patternCharIndex], beginOffset, appendTo.length());
}

//----------------------------------------------------------------------

NumberFormat *
SimpleDateFormat::getNumberFormatByIndex(UDateFormatField index) const {
    if (fNumberFormatters != NULL) {
        return fNumberFormatters[index];
    } else {
        return fNumberFormat;
    }
}

//----------------------------------------------------------------------
void
SimpleDateFormat::zeroPaddingNumber(NumberFormat *currentNumberFormat,UnicodeString &appendTo,
                                    int32_t value, int32_t minDigits, int32_t maxDigits) const
{
    if (currentNumberFormat!=NULL) {
        FieldPosition pos(0);

        currentNumberFormat->setMinimumIntegerDigits(minDigits);
        currentNumberFormat->setMaximumIntegerDigits(maxDigits);
        currentNumberFormat->format(value, appendTo, pos);  // 3rd arg is there to speed up processing
    }
}

//----------------------------------------------------------------------

/**
 * Format characters that indicate numeric fields.  The character
 * at index 0 is treated specially.
 */
static const UChar NUMERIC_FORMAT_CHARS[] = {0x4D, 0x59, 0x79, 0x75, 0x64, 0x65, 0x68, 0x48, 0x6D, 0x73, 0x53, 0x44, 0x46, 0x77, 0x57, 0x6B, 0x4B, 0x00}; /* "MYyudehHmsSDFwWkK" */

/**
 * Return true if the given format character, occuring count
 * times, represents a numeric field.
 */
UBool SimpleDateFormat::isNumeric(UChar formatChar, int32_t count) {
    UnicodeString s(NUMERIC_FORMAT_CHARS);
    int32_t i = s.indexOf(formatChar);
    return (i > 0 || (i == 0 && count < 3));
}

void
SimpleDateFormat::parse(const UnicodeString& text, Calendar& cal, ParsePosition& parsePos) const
{
    UErrorCode status = U_ZERO_ERROR;
    int32_t pos = parsePos.getIndex();
    int32_t start = pos;

    UBool ambiguousYear[] = { FALSE };
    int32_t saveHebrewMonth = -1;
    int32_t count = 0;

    // hack, reset tztype, cast away const
    ((SimpleDateFormat*)this)->tztype = TZTYPE_UNK;

    // For parsing abutting numeric fields. 'abutPat' is the
    // offset into 'pattern' of the first of 2 or more abutting
    // numeric fields.  'abutStart' is the offset into 'text'
    // where parsing the fields begins. 'abutPass' starts off as 0
    // and increments each time we try to parse the fields.
    int32_t abutPat = -1; // If >=0, we are in a run of abutting numeric fields
    int32_t abutStart = 0;
    int32_t abutPass = 0;
    UBool inQuote = FALSE;

    const UnicodeString numericFormatChars(NUMERIC_FORMAT_CHARS);

    TimeZone *backupTZ = NULL;
    Calendar *workCal = &cal;
    if (&cal != fCalendar && uprv_strcmp(cal.getType(), fCalendar->getType()) != 0) {
        // Different calendar type
        // We use the time/zone from the input calendar, but
        // do not use the input calendar for field calculation.
        fCalendar->setTime(cal.getTime(status),status);
        if (U_FAILURE(status)) {
            goto ExitParse;
        }
        backupTZ = fCalendar->getTimeZone().clone();
        fCalendar->setTimeZone(cal.getTimeZone());
        workCal = fCalendar;
    }

    for (int32_t i=0; i<fPattern.length(); ++i) {
        UChar ch = fPattern.charAt(i);

        // Handle alphabetic field characters.
        if (!inQuote && ((ch >= 0x41 && ch <= 0x5A) || (ch >= 0x61 && ch <= 0x7A))) { // [A-Za-z]
            int32_t fieldPat = i;

            // Count the length of this field specifier
            count = 1;
            while ((i+1)<fPattern.length() &&
                   fPattern.charAt(i+1) == ch) {
                ++count;
                ++i;
            }

            if (isNumeric(ch, count)) {
                if (abutPat < 0) {
                    // Determine if there is an abutting numeric field.  For
                    // most fields we can just look at the next characters,
                    // but the 'm' field is either numeric or text,
                    // depending on the count, so we have to look ahead for
                    // that field.
                    if ((i+1)<fPattern.length()) {
                        UBool abutting;
                        UChar nextCh = fPattern.charAt(i+1);
                        int32_t k = numericFormatChars.indexOf(nextCh);
                        if (k == 0) {
                            int32_t j = i+2;
                            while (j<fPattern.length() &&
                                   fPattern.charAt(j) == nextCh) {
                                ++j;
                            }
                            abutting = (j-i) < 4; // nextCount < 3
                        } else {
                            abutting = k > 0;
                        }

                        // Record the start of a set of abutting numeric
                        // fields.
                        if (abutting) {
                            abutPat = fieldPat;
                            abutStart = pos;
                            abutPass = 0;
                        }
                    }
                }
            } else {
                abutPat = -1; // End of any abutting fields
            }

            // Handle fields within a run of abutting numeric fields.  Take
            // the pattern "HHmmss" as an example. We will try to parse
            // 2/2/2 characters of the input text, then if that fails,
            // 1/2/2.  We only adjust the width of the leftmost field; the
            // others remain fixed.  This allows "123456" => 12:34:56, but
            // "12345" => 1:23:45.  Likewise, for the pattern "yyyyMMdd" we
            // try 4/2/2, 3/2/2, 2/2/2, and finally 1/2/2.
            if (abutPat >= 0) {
                // If we are at the start of a run of abutting fields, then
                // shorten this field in each pass.  If we can't shorten
                // this field any more, then the parse of this set of
                // abutting numeric fields has failed.
                if (fieldPat == abutPat) {
                    count -= abutPass++;
                    if (count == 0) {
                        status = U_PARSE_ERROR;
                        goto ExitParse;
                    }
                }

                pos = subParse(text, pos, ch, count,
                               TRUE, FALSE, ambiguousYear, saveHebrewMonth, *workCal, i);

                // If the parse fails anywhere in the run, back up to the
                // start of the run and retry.
                if (pos < 0) {
                    i = abutPat - 1;
                    pos = abutStart;
                    continue;
                }
            }

            // Handle non-numeric fields and non-abutting numeric
            // fields.
            else {
                int32_t s = subParse(text, pos, ch, count,
                               FALSE, TRUE, ambiguousYear, saveHebrewMonth, *workCal, i);

                if (s == -pos-1) {
                    // era not present, in special cases allow this to continue
                    s++;

                    if (i+1 < fPattern.length()) {
                        // move to next pattern character
                        UChar ch = fPattern.charAt(i+1);

                        // check for whitespace
                        if (uprv_isRuleWhiteSpace(ch)) {
                            i++;
                            // Advance over run in pattern
                            while ((i+1)<fPattern.length() &&
                                   uprv_isRuleWhiteSpace(fPattern.charAt(i+1))) {
                                ++i;
                            }
                        }
                    }
                }
                else if (s < 0) {
                    status = U_PARSE_ERROR;
                    goto ExitParse;
                }
                pos = s;
            }
        }

        // Handle literal pattern characters.  These are any
        // quoted characters and non-alphabetic unquoted
        // characters.
        else {

            abutPat = -1; // End of any abutting fields

            // Handle quotes.  Two consecutive quotes is a quote
            // literal, inside or outside of quotes.  Otherwise a
            // quote indicates entry or exit from a quoted region.
            if (ch == QUOTE) {
                // Match a quote literal '' within OR outside of quotes
                if ((i+1)<fPattern.length() && fPattern.charAt(i+1)==ch) {
                    ++i; // Skip over doubled quote
                    // Fall through and treat quote as a literal
                } else {
                    // Enter or exit quoted region
                    inQuote = !inQuote;
                    continue;
                }
            }

            // A run of white space in the pattern matches a run
            // of white space in the input text.
            if (uprv_isRuleWhiteSpace(ch)) {
                // Advance over run in pattern
                while ((i+1)<fPattern.length() &&
                       uprv_isRuleWhiteSpace(fPattern.charAt(i+1))) {
                    ++i;
                }

                // Advance over run in input text
                int32_t s = pos;
                while (pos<text.length() &&
                       ( u_isUWhiteSpace(text.charAt(pos)) || uprv_isRuleWhiteSpace(text.charAt(pos)))) {
                    ++pos;
                }

                // Must see at least one white space char in input
                if (pos > s) {
                    continue;
                }


            } else if (pos<text.length() && text.charAt(pos)==ch) {
                // Match a literal
                ++pos;
                continue;
            }

            // We fall through to this point if the match fails
            status = U_PARSE_ERROR;
            goto ExitParse;
        }
    }

    // At this point the fields of Calendar have been set.  Calendar
    // will fill in default values for missing fields when the time
    // is computed.

    parsePos.setIndex(pos);

    // This part is a problem:  When we call parsedDate.after, we compute the time.
    // Take the date April 3 2004 at 2:30 am.  When this is first set up, the year
    // will be wrong if we're parsing a 2-digit year pattern.  It will be 1904.
    // April 3 1904 is a Sunday (unlike 2004) so it is the DST onset day.  2:30 am
    // is therefore an "impossible" time, since the time goes from 1:59 to 3:00 am
    // on that day.  It is therefore parsed out to fields as 3:30 am.  Then we
    // add 100 years, and get April 3 2004 at 3:30 am.  Note that April 3 2004 is
    // a Saturday, so it can have a 2:30 am -- and it should. [LIU]
    /*
        UDate parsedDate = calendar.getTime();
        if( ambiguousYear[0] && !parsedDate.after(fDefaultCenturyStart) ) {
            calendar.add(Calendar.YEAR, 100);
            parsedDate = calendar.getTime();
        }
    */
    // Because of the above condition, save off the fields in case we need to readjust.
    // The procedure we use here is not particularly efficient, but there is no other
    // way to do this given the API restrictions present in Calendar.  We minimize
    // inefficiency by only performing this computation when it might apply, that is,
    // when the two-digit year is equal to the start year, and thus might fall at the
    // front or the back of the default century.  This only works because we adjust
    // the year correctly to start with in other cases -- see subParse().
    if (ambiguousYear[0] || tztype != TZTYPE_UNK) // If this is true then the two-digit year == the default start year
    {
        // We need a copy of the fields, and we need to avoid triggering a call to
        // complete(), which will recalculate the fields.  Since we can't access
        // the fields[] array in Calendar, we clone the entire object.  This will
        // stop working if Calendar.clone() is ever rewritten to call complete().
        Calendar *copy;
        if (ambiguousYear[0]) {
            copy = cal.clone();
            // Check for failed cloning.
            if (copy == NULL) {
            	status = U_MEMORY_ALLOCATION_ERROR;
            	goto ExitParse;
            }
            UDate parsedDate = copy->getTime(status);
            // {sfb} check internalGetDefaultCenturyStart
            if (fHaveDefaultCentury && (parsedDate < fDefaultCenturyStart)) {
                // We can't use add here because that does a complete() first.
                cal.set(UCAL_YEAR, fDefaultCenturyStartYear + 100);
            }
            delete copy;
        }

        if (tztype != TZTYPE_UNK) {
            copy = cal.clone();
            // Check for failed cloning.
            if (copy == NULL) {
            	status = U_MEMORY_ALLOCATION_ERROR;
            	goto ExitParse;
            }
            const TimeZone & tz = cal.getTimeZone();
            BasicTimeZone *btz = NULL;

            if (dynamic_cast<const OlsonTimeZone *>(&tz) != NULL
                || dynamic_cast<const SimpleTimeZone *>(&tz) != NULL
                || dynamic_cast<const RuleBasedTimeZone *>(&tz) != NULL
                || dynamic_cast<const VTimeZone *>(&tz) != NULL) {
                btz = (BasicTimeZone*)&tz;
            }

            // Get local millis
            copy->set(UCAL_ZONE_OFFSET, 0);
            copy->set(UCAL_DST_OFFSET, 0);
            UDate localMillis = copy->getTime(status);

            // Make sure parsed time zone type (Standard or Daylight)
            // matches the rule used by the parsed time zone.
            int32_t raw, dst;
            if (btz != NULL) {
                if (tztype == TZTYPE_STD) {
                    btz->getOffsetFromLocal(localMillis,
                        BasicTimeZone::kStandard, BasicTimeZone::kStandard, raw, dst, status);
                } else {
                    btz->getOffsetFromLocal(localMillis,
                        BasicTimeZone::kDaylight, BasicTimeZone::kDaylight, raw, dst, status);
                }
            } else {
                // No good way to resolve ambiguous time at transition,
                // but following code work in most case.
                tz.getOffset(localMillis, TRUE, raw, dst, status);
            }

            // Now, compare the results with parsed type, either standard or daylight saving time
            int32_t resolvedSavings = dst;
            if (tztype == TZTYPE_STD) {
                if (dst != 0) {
                    // Override DST_OFFSET = 0 in the result calendar
                    resolvedSavings = 0;
                }
            } else { // tztype == TZTYPE_DST
                if (dst == 0) {
                    if (btz != NULL) {
                        UDate time = localMillis + raw;
                        // We use the nearest daylight saving time rule.
                        TimeZoneTransition beforeTrs, afterTrs;
                        UDate beforeT = time, afterT = time;
                        int32_t beforeSav = 0, afterSav = 0;
                        UBool beforeTrsAvail, afterTrsAvail;

                        // Search for DST rule before or on the time
                        while (TRUE) {
                            beforeTrsAvail = btz->getPreviousTransition(beforeT, TRUE, beforeTrs);
                            if (!beforeTrsAvail) {
                                break;
                            }
                            beforeT = beforeTrs.getTime() - 1;
                            beforeSav = beforeTrs.getFrom()->getDSTSavings();
                            if (beforeSav != 0) {
                                break;
                            }
                        }

                        // Search for DST rule after the time
                        while (TRUE) {
                            afterTrsAvail = btz->getNextTransition(afterT, FALSE, afterTrs);
                            if (!afterTrsAvail) {
                                break;
                            }
                            afterT = afterTrs.getTime();
                            afterSav = afterTrs.getTo()->getDSTSavings();
                            if (afterSav != 0) {
                                break;
                            }
                        }

                        if (beforeTrsAvail && afterTrsAvail) {
                            if (time - beforeT > afterT - time) {
                                resolvedSavings = afterSav;
                            } else {
                                resolvedSavings = beforeSav;
                            }
                        } else if (beforeTrsAvail && beforeSav != 0) {
                            resolvedSavings = beforeSav;
                        } else if (afterTrsAvail && afterSav != 0) {
                            resolvedSavings = afterSav;
                        } else {
                            resolvedSavings = btz->getDSTSavings();
                        }
                    } else {
                        resolvedSavings = tz.getDSTSavings();
                    }
                    if (resolvedSavings == 0) {
                        // final fallback
                        resolvedSavings = U_MILLIS_PER_HOUR;
                    }
                }
            }
            cal.set(UCAL_ZONE_OFFSET, raw);
            cal.set(UCAL_DST_OFFSET, resolvedSavings);
            delete copy;
        }
    }
ExitParse:
    // Set the parsed result if local calendar is used
    // instead of the input calendar
    if (U_SUCCESS(status) && workCal != &cal) {
        cal.setTimeZone(workCal->getTimeZone());
        cal.setTime(workCal->getTime(status), status);
    }

    // Restore the original time zone if required
    if (backupTZ != NULL) {
        fCalendar->adoptTimeZone(backupTZ);
    }

    // If any Calendar calls failed, we pretend that we
    // couldn't parse the string, when in reality this isn't quite accurate--
    // we did parse it; the Calendar calls just failed.
    if (U_FAILURE(status)) {
        parsePos.setErrorIndex(pos);
        parsePos.setIndex(start);
    }
}

UDate
SimpleDateFormat::parse( const UnicodeString& text,
                         ParsePosition& pos) const {
    // redefined here because the other parse() function hides this function's
    // cunterpart on DateFormat
    return DateFormat::parse(text, pos);
}

UDate
SimpleDateFormat::parse(const UnicodeString& text, UErrorCode& status) const
{
    // redefined here because the other parse() function hides this function's
    // counterpart on DateFormat
    return DateFormat::parse(text, status);
}
//----------------------------------------------------------------------

int32_t SimpleDateFormat::matchQuarterString(const UnicodeString& text,
                              int32_t start,
                              UCalendarDateFields field,
                              const UnicodeString* data,
                              int32_t dataCount,
                              Calendar& cal) const
{
    int32_t i = 0;
    int32_t count = dataCount;

    // There may be multiple strings in the data[] array which begin with
    // the same prefix (e.g., Cerven and Cervenec (June and July) in Czech).
    // We keep track of the longest match, and return that.  Note that this
    // unfortunately requires us to test all array elements.
    int32_t bestMatchLength = 0, bestMatch = -1;

    // {sfb} kludge to support case-insensitive comparison
    // {markus 2002oct11} do not just use caseCompareBetween because we do not know
    // the length of the match after case folding
    // {alan 20040607} don't case change the whole string, since the length
    // can change
    // TODO we need a case-insensitive startsWith function
    UnicodeString lcase, lcaseText;
    text.extract(start, INT32_MAX, lcaseText);
    lcaseText.foldCase();

    for (; i < count; ++i)
    {
        // Always compare if we have no match yet; otherwise only compare
        // against potentially better matches (longer strings).

        lcase.fastCopyFrom(data[i]).foldCase();
        int32_t length = lcase.length();

        if (length > bestMatchLength &&
            lcaseText.compareBetween(0, length, lcase, 0, length) == 0)
        {
            bestMatch = i;
            bestMatchLength = length;
        }
    }
    if (bestMatch >= 0)
    {
        cal.set(field, bestMatch * 3);

        // Once we have a match, we have to determine the length of the
        // original source string.  This will usually be == the length of
        // the case folded string, but it may differ (e.g. sharp s).
        lcase.fastCopyFrom(data[bestMatch]).foldCase();

        // Most of the time, the length will be the same as the length
        // of the string from the locale data.  Sometimes it will be
        // different, in which case we will have to figure it out by
        // adding a character at a time, until we have a match.  We do
        // this all in one loop, where we try 'len' first (at index
        // i==0).
        int32_t len = data[bestMatch].length(); // 99+% of the time
        int32_t n = text.length() - start;
        for (i=0; i<=n; ++i) {
            int32_t j=i;
            if (i == 0) {
                j = len;
            } else if (i == len) {
                continue; // already tried this when i was 0
            }
            text.extract(start, j, lcaseText);
            lcaseText.foldCase();
            if (lcase == lcaseText) {
                return start + j;
            }
        }
    }

    return -start;
}

//----------------------------------------------------------------------

int32_t SimpleDateFormat::matchString(const UnicodeString& text,
                              int32_t start,
                              UCalendarDateFields field,
                              const UnicodeString* data,
                              int32_t dataCount,
                              Calendar& cal) const
{
    int32_t i = 0;
    int32_t count = dataCount;

    if (field == UCAL_DAY_OF_WEEK) i = 1;

    // There may be multiple strings in the data[] array which begin with
    // the same prefix (e.g., Cerven and Cervenec (June and July) in Czech).
    // We keep track of the longest match, and return that.  Note that this
    // unfortunately requires us to test all array elements.
    int32_t bestMatchLength = 0, bestMatch = -1;

    // {sfb} kludge to support case-insensitive comparison
    // {markus 2002oct11} do not just use caseCompareBetween because we do not know
    // the length of the match after case folding
    // {alan 20040607} don't case change the whole string, since the length
    // can change
    // TODO we need a case-insensitive startsWith function
    UnicodeString lcase, lcaseText;
    text.extract(start, INT32_MAX, lcaseText);
    lcaseText.foldCase();

    for (; i < count; ++i)
    {
        // Always compare if we have no match yet; otherwise only compare
        // against potentially better matches (longer strings).

        lcase.fastCopyFrom(data[i]).foldCase();
        int32_t length = lcase.length();

        if (length > bestMatchLength &&
            lcaseText.compareBetween(0, length, lcase, 0, length) == 0)
        {
            bestMatch = i;
            bestMatchLength = length;
        }
    }
    if (bestMatch >= 0)
    {
        // Adjustment for Hebrew Calendar month Adar II
        if (!strcmp(cal.getType(),"hebrew") && field==UCAL_MONTH && bestMatch==13) {
            cal.set(field,6);
        }
        else {
            cal.set(field, bestMatch);
        }

        // Once we have a match, we have to determine the length of the
        // original source string.  This will usually be == the length of
        // the case folded string, but it may differ (e.g. sharp s).
        lcase.fastCopyFrom(data[bestMatch]).foldCase();

        // Most of the time, the length will be the same as the length
        // of the string from the locale data.  Sometimes it will be
        // different, in which case we will have to figure it out by
        // adding a character at a time, until we have a match.  We do
        // this all in one loop, where we try 'len' first (at index
        // i==0).
        int32_t len = data[bestMatch].length(); // 99+% of the time
        int32_t n = text.length() - start;
        for (i=0; i<=n; ++i) {
            int32_t j=i;
            if (i == 0) {
                j = len;
            } else if (i == len) {
                continue; // already tried this when i was 0
            }
            text.extract(start, j, lcaseText);
            lcaseText.foldCase();
            if (lcase == lcaseText) {
                return start + j;
            }
        }
    }

    return -start;
}

//----------------------------------------------------------------------

void
SimpleDateFormat::set2DigitYearStart(UDate d, UErrorCode& status)
{
    parseAmbiguousDatesAsAfter(d, status);
}

/**
 * Private member function that converts the parsed date strings into
 * timeFields. Returns -start (for ParsePosition) if failed.
 * @param text the time text to be parsed.
 * @param start where to start parsing.
 * @param ch the pattern character for the date field text to be parsed.
 * @param count the count of a pattern character.
 * @return the new start position if matching succeeded; a negative number
 * indicating matching failure, otherwise.
 */
int32_t SimpleDateFormat::subParse(const UnicodeString& text, int32_t& start, UChar ch, int32_t count,
                           UBool obeyCount, UBool allowNegative, UBool ambiguousYear[], int32_t& saveHebrewMonth, Calendar& cal,
                           int32_t patLoc) const
{
    Formattable number;
    int32_t value = 0;
    int32_t i;
    int32_t ps = 0;
    ParsePosition pos(0);
    UDateFormatField patternCharIndex;
    NumberFormat *currentNumberFormat;
    UnicodeString temp;
    UChar *patternCharPtr = u_strchr(DateFormatSymbols::getPatternUChars(), ch);

#if defined (U_DEBUG_CAL)
    //fprintf(stderr, "%s:%d - [%c]  st=%d \n", __FILE__, __LINE__, (char) ch, start);
#endif

    if (patternCharPtr == NULL) {
        return -start;
    }

    patternCharIndex = (UDateFormatField)(patternCharPtr - DateFormatSymbols::getPatternUChars());
    currentNumberFormat = getNumberFormatByIndex(patternCharIndex);
    UCalendarDateFields field = fgPatternIndexToCalendarField[patternCharIndex];

    // If there are any spaces here, skip over them.  If we hit the end
    // of the string, then fail.
    for (;;) {
        if (start >= text.length()) {
            return -start;
        }
        UChar32 c = text.char32At(start);
        if (!u_isUWhiteSpace(c) || !uprv_isRuleWhiteSpace(c)) {
            break;
        }
        start += UTF_CHAR_LENGTH(c);
    }
    pos.setIndex(start);

    // We handle a few special cases here where we need to parse
    // a number value.  We handle further, more generic cases below.  We need
    // to handle some of them here because some fields require extra processing on
    // the parsed value.
    if (patternCharIndex == UDAT_HOUR_OF_DAY1_FIELD ||
        patternCharIndex == UDAT_HOUR1_FIELD ||
        (patternCharIndex == UDAT_DOW_LOCAL_FIELD && count <= 2) ||
        (patternCharIndex == UDAT_STANDALONE_DAY_FIELD && count <= 2) ||
        (patternCharIndex == UDAT_MONTH_FIELD && count <= 2) ||
        (patternCharIndex == UDAT_STANDALONE_MONTH_FIELD && count <= 2) ||
        (patternCharIndex == UDAT_QUARTER_FIELD && count <= 2) ||
        (patternCharIndex == UDAT_STANDALONE_QUARTER_FIELD && count <= 2) ||
        patternCharIndex == UDAT_YEAR_FIELD ||
        patternCharIndex == UDAT_YEAR_WOY_FIELD ||
        patternCharIndex == UDAT_FRACTIONAL_SECOND_FIELD)
    {
        int32_t parseStart = pos.getIndex();
        // It would be good to unify this with the obeyCount logic below,
        // but that's going to be difficult.
        const UnicodeString* src;

        if (obeyCount) {
            if ((start+count) > text.length()) {
                return -start;
            }

            text.extractBetween(0, start + count, temp);
            src = &temp;
        } else {
            src = &text;
        }

        parseInt(*src, number, pos, allowNegative,currentNumberFormat);

        if (pos.getIndex() == parseStart)
            return -start;
        value = number.getLong();

        // suffix processing
        int32_t txtLoc = pos.getIndex();
        if (value <0 ) {
            txtLoc = checkIntSuffix(text, txtLoc, patLoc+1, TRUE);
            if (txtLoc != pos.getIndex()) {
                value *= -1;
            }
        }
        else {
            txtLoc = checkIntSuffix(text, txtLoc, patLoc+1, FALSE);
        }
        pos.setIndex(txtLoc);

    }

    switch (patternCharIndex) {
    case UDAT_ERA_FIELD:
        if (count == 5) {
            ps = matchString(text, start, UCAL_ERA, fSymbols->fNarrowEras, fSymbols->fNarrowErasCount, cal);
        }
        if (count == 4) {
            ps = matchString(text, start, UCAL_ERA, fSymbols->fEraNames, fSymbols->fEraNamesCount, cal);
        }
        else {
            ps = matchString(text, start, UCAL_ERA, fSymbols->fEras, fSymbols->fErasCount, cal);
        }

        // check return position, if it equals -start, then matchString error
        // special case the return code so we don't necessarily fail out until we
        // verify no year information also
        if (ps == -start)
            ps--;

        return ps;

    case UDAT_YEAR_FIELD:
        // If there are 3 or more YEAR pattern characters, this indicates
        // that the year value is to be treated literally, without any
        // two-digit year adjustments (e.g., from "01" to 2001).  Otherwise
        // we made adjustments to place the 2-digit year in the proper
        // century, for parsed strings from "00" to "99".  Any other string
        // is treated literally:  "2250", "-1", "1", "002".
        if (count <= 2 && (pos.getIndex() - start) == 2
            && u_isdigit(text.charAt(start))
            && u_isdigit(text.charAt(start+1)))
        {
            // Assume for example that the defaultCenturyStart is 6/18/1903.
            // This means that two-digit years will be forced into the range
            // 6/18/1903 to 6/17/2003.  As a result, years 00, 01, and 02
            // correspond to 2000, 2001, and 2002.  Years 04, 05, etc. correspond
            // to 1904, 1905, etc.  If the year is 03, then it is 2003 if the
            // other fields specify a date before 6/18, or 1903 if they specify a
            // date afterwards.  As a result, 03 is an ambiguous year.  All other
            // two-digit years are unambiguous.
          if(fHaveDefaultCentury) { // check if this formatter even has a pivot year
              int32_t ambiguousTwoDigitYear = fDefaultCenturyStartYear % 100;
              ambiguousYear[0] = (value == ambiguousTwoDigitYear);
              value += (fDefaultCenturyStartYear/100)*100 +
                (value < ambiguousTwoDigitYear ? 100 : 0);
            }
        }
        cal.set(UCAL_YEAR, value);

        // Delayed checking for adjustment of Hebrew month numbers in non-leap years.
        if (saveHebrewMonth >= 0) {
            HebrewCalendar *hc = (HebrewCalendar*)&cal;
            if (!hc->isLeapYear(value) && saveHebrewMonth >= 6) {
               cal.set(UCAL_MONTH,saveHebrewMonth);
            } else {
               cal.set(UCAL_MONTH,saveHebrewMonth-1);
            }
            saveHebrewMonth = -1;
        }
        return pos.getIndex();

    case UDAT_YEAR_WOY_FIELD:
        // Comment is the same as for UDAT_Year_FIELDs - look above
        if (count <= 2 && (pos.getIndex() - start) == 2
            && u_isdigit(text.charAt(start))
            && u_isdigit(text.charAt(start+1))
            && fHaveDefaultCentury )
        {
            int32_t ambiguousTwoDigitYear = fDefaultCenturyStartYear % 100;
            ambiguousYear[0] = (value == ambiguousTwoDigitYear);
            value += (fDefaultCenturyStartYear/100)*100 +
                (value < ambiguousTwoDigitYear ? 100 : 0);
        }
        cal.set(UCAL_YEAR_WOY, value);
        return pos.getIndex();

    case UDAT_MONTH_FIELD:
        if (count <= 2) // i.e., M or MM.
        {
            // When parsing month numbers from the Hebrew Calendar, we might need to adjust the month depending on whether
            // or not it was a leap year.  We may or may not yet know what year it is, so might have to delay checking until
            // the year is parsed.
            if (!strcmp(cal.getType(),"hebrew")) {
                HebrewCalendar *hc = (HebrewCalendar*)&cal;
                if (cal.isSet(UCAL_YEAR)) {
                   UErrorCode status = U_ZERO_ERROR;
                   if (!hc->isLeapYear(hc->get(UCAL_YEAR,status)) && value >= 6) {
                       cal.set(UCAL_MONTH, value);
                   } else {
                       cal.set(UCAL_MONTH, value - 1);
                   }
                } else {
                    saveHebrewMonth = value;
                }
            } else {
                // Don't want to parse the month if it is a string
                // while pattern uses numeric style: M or MM.
                // [We computed 'value' above.]
                cal.set(UCAL_MONTH, value - 1);
            }
            return pos.getIndex();
        } else {
            // count >= 3 // i.e., MMM or MMMM
            // Want to be able to parse both short and long forms.
            // Try count == 4 first:
            int32_t newStart = 0;

            if ((newStart = matchString(text, start, UCAL_MONTH,
                                      fSymbols->fMonths, fSymbols->fMonthsCount, cal)) > 0)
                return newStart;
            else // count == 4 failed, now try count == 3
                return matchString(text, start, UCAL_MONTH,
                                   fSymbols->fShortMonths, fSymbols->fShortMonthsCount, cal);
        }

    case UDAT_STANDALONE_MONTH_FIELD:
        if (count <= 2) // i.e., L or LL.
        {
            // Don't want to parse the month if it is a string
            // while pattern uses numeric style: M or MM.
            // [We computed 'value' above.]
            cal.set(UCAL_MONTH, value - 1);
            return pos.getIndex();
        } else {
            // count >= 3 // i.e., LLL or LLLL
            // Want to be able to parse both short and long forms.
            // Try count == 4 first:
            int32_t newStart = 0;

            if ((newStart = matchString(text, start, UCAL_MONTH,
                                      fSymbols->fStandaloneMonths, fSymbols->fStandaloneMonthsCount, cal)) > 0)
                return newStart;
            else // count == 4 failed, now try count == 3
                return matchString(text, start, UCAL_MONTH,
                                   fSymbols->fStandaloneShortMonths, fSymbols->fStandaloneShortMonthsCount, cal);
        }

    case UDAT_HOUR_OF_DAY1_FIELD:
        // [We computed 'value' above.]
        if (value == cal.getMaximum(UCAL_HOUR_OF_DAY) + 1)
            value = 0;
        cal.set(UCAL_HOUR_OF_DAY, value);
        return pos.getIndex();

    case UDAT_FRACTIONAL_SECOND_FIELD:
        // Fractional seconds left-justify
        i = pos.getIndex() - start;
        if (i < 3) {
            while (i < 3) {
                value *= 10;
                i++;
            }
        } else {
            int32_t a = 1;
            while (i > 3) {
                a *= 10;
                i--;
            }
            value = (value + (a>>1)) / a;
        }
        cal.set(UCAL_MILLISECOND, value);
        return pos.getIndex();

    case UDAT_DOW_LOCAL_FIELD:
        if (count <= 2) // i.e., e or ee
        {
            // [We computed 'value' above.]
            cal.set(UCAL_DOW_LOCAL, value);
            return pos.getIndex();
        }
        // else for eee-eeeee fall through to handling of EEE-EEEEE
        // fall through, do not break here
    case UDAT_DAY_OF_WEEK_FIELD:
        {
            // Want to be able to parse both short and long forms.
            // Try count == 4 (EEEE) first:
            int32_t newStart = 0;
            if ((newStart = matchString(text, start, UCAL_DAY_OF_WEEK,
                                      fSymbols->fWeekdays, fSymbols->fWeekdaysCount, cal)) > 0)
                return newStart;
            // EEEE failed, now try EEE
            else if ((newStart = matchString(text, start, UCAL_DAY_OF_WEEK,
                                   fSymbols->fShortWeekdays, fSymbols->fShortWeekdaysCount, cal)) > 0)
                return newStart;
            // EEE failed, now try EEEEE
            else
                return matchString(text, start, UCAL_DAY_OF_WEEK,
                                   fSymbols->fNarrowWeekdays, fSymbols->fNarrowWeekdaysCount, cal);
        }

    case UDAT_STANDALONE_DAY_FIELD:
        {
            if (count <= 2) // c or cc
            {
                // [We computed 'value' above.]
                cal.set(UCAL_DOW_LOCAL, value);
                return pos.getIndex();
            }
            // Want to be able to parse both short and long forms.
            // Try count == 4 (cccc) first:
            int32_t newStart = 0;
            if ((newStart = matchString(text, start, UCAL_DAY_OF_WEEK,
                                      fSymbols->fStandaloneWeekdays, fSymbols->fStandaloneWeekdaysCount, cal)) > 0)
                return newStart;
            else // cccc failed, now try ccc
                return matchString(text, start, UCAL_DAY_OF_WEEK,
                                   fSymbols->fStandaloneShortWeekdays, fSymbols->fStandaloneShortWeekdaysCount, cal);
        }

    case UDAT_AM_PM_FIELD:
        return matchString(text, start, UCAL_AM_PM, fSymbols->fAmPms, fSymbols->fAmPmsCount, cal);

    case UDAT_HOUR1_FIELD:
        // [We computed 'value' above.]
        if (value == cal.getLeastMaximum(UCAL_HOUR)+1)
            value = 0;
        cal.set(UCAL_HOUR, value);
        return pos.getIndex();

    case UDAT_QUARTER_FIELD:
        if (count <= 2) // i.e., Q or QQ.
        {
            // Don't want to parse the month if it is a string
            // while pattern uses numeric style: Q or QQ.
            // [We computed 'value' above.]
            cal.set(UCAL_MONTH, (value - 1) * 3);
            return pos.getIndex();
        } else {
            // count >= 3 // i.e., QQQ or QQQQ
            // Want to be able to parse both short and long forms.
            // Try count == 4 first:
            int32_t newStart = 0;

            if ((newStart = matchQuarterString(text, start, UCAL_MONTH,
                                      fSymbols->fQuarters, fSymbols->fQuartersCount, cal)) > 0)
                return newStart;
            else // count == 4 failed, now try count == 3
                return matchQuarterString(text, start, UCAL_MONTH,
                                   fSymbols->fShortQuarters, fSymbols->fShortQuartersCount, cal);
        }

    case UDAT_STANDALONE_QUARTER_FIELD:
        if (count <= 2) // i.e., q or qq.
        {
            // Don't want to parse the month if it is a string
            // while pattern uses numeric style: q or q.
            // [We computed 'value' above.]
            cal.set(UCAL_MONTH, (value - 1) * 3);
            return pos.getIndex();
        } else {
            // count >= 3 // i.e., qqq or qqqq
            // Want to be able to parse both short and long forms.
            // Try count == 4 first:
            int32_t newStart = 0;

            if ((newStart = matchQuarterString(text, start, UCAL_MONTH,
                                      fSymbols->fStandaloneQuarters, fSymbols->fStandaloneQuartersCount, cal)) > 0)
                return newStart;
            else // count == 4 failed, now try count == 3
                return matchQuarterString(text, start, UCAL_MONTH,
                                   fSymbols->fStandaloneShortQuarters, fSymbols->fStandaloneShortQuartersCount, cal);
        }

    case UDAT_TIMEZONE_FIELD:
    case UDAT_TIMEZONE_RFC_FIELD:
    case UDAT_TIMEZONE_GENERIC_FIELD:
    case UDAT_TIMEZONE_SPECIAL_FIELD:
        {
            int32_t offset = 0;
            UBool parsed = FALSE;

            // Step 1
            // Check if this is a long GMT offset string (either localized or default)
            offset = parseGMT(text, pos);
            if (pos.getIndex() - start > 0) {
                parsed = TRUE;
            }
            if (!parsed) {
                // Step 2
                // Check if this is an RFC822 time zone offset.
                // ICU supports the standard RFC822 format [+|-]HHmm
                // and its extended form [+|-]HHmmSS.
                do {
                    int32_t sign = 0;
                    UChar signChar = text.charAt(start);
                    if (signChar == (UChar)0x002B /* '+' */) {
                        sign = 1;
                    } else if (signChar == (UChar)0x002D /* '-' */) {
                        sign = -1;
                    } else {
                        // Not an RFC822 offset string
                        break;
                    }

                    // Parse digits
                    int32_t orgPos = start + 1;
                    pos.setIndex(orgPos);
                    parseInt(text, number, 6, pos, FALSE,currentNumberFormat);
                    int32_t numLen = pos.getIndex() - orgPos;
                    if (numLen <= 0) {
                        break;
                    }

                    // Followings are possible format (excluding sign char)
                    // HHmmSS
                    // HmmSS
                    // HHmm
                    // Hmm
                    // HH
                    // H
                    int32_t val = number.getLong();
                    int32_t hour = 0, min = 0, sec = 0;
                    switch(numLen) {
                    case 1: // H
                    case 2: // HH
                        hour = val;
                        break;
                    case 3: // Hmm
                    case 4: // HHmm
                        hour = val / 100;
                        min = val % 100;
                        break;
                    case 5: // Hmmss
                    case 6: // HHmmss
                        hour = val / 10000;
                        min = (val % 10000) / 100;
                        sec = val % 100;
                        break;
                    }
                    if (hour > 23 || min > 59 || sec > 59) {
                        // Invalid value range
                        break;
                    }
                    offset = (((hour * 60) + min) * 60 + sec) * 1000 * sign;
                    parsed = TRUE;
                } while (FALSE);

                if (!parsed) {
                    // Failed to parse.  Reset the position.
                    pos.setIndex(start);
                }
            }

            if (parsed) {
                // offset was successfully parsed as either a long GMT string or RFC822 zone offset
                // string.  Create normalized zone ID for the offset.

                UnicodeString tzID(gGmt);
                formatRFC822TZ(tzID, offset);
                //TimeZone *customTZ = TimeZone::createTimeZone(tzID);
                TimeZone *customTZ = new SimpleTimeZone(offset, tzID);    // faster than TimeZone::createTimeZone
                cal.adoptTimeZone(customTZ);

                return pos.getIndex();
            }

            // Step 3
            // At this point, check for named time zones by looking through
            // the locale data from the DateFormatZoneData strings.
            // Want to be able to parse both short and long forms.
            // optimize for calendar's current time zone
            const ZoneStringFormat *zsf = fSymbols->getZoneStringFormat();
            if (zsf) {
                UErrorCode status = U_ZERO_ERROR;
                const ZoneStringInfo *zsinfo = NULL;
                int32_t matchLen;

                switch (patternCharIndex) {
                    case UDAT_TIMEZONE_FIELD: // 'z'
                        if (count < 4) {
                            zsinfo = zsf->findSpecificShort(text, start, matchLen, status);
                        } else {
                            zsinfo = zsf->findSpecificLong(text, start, matchLen, status);
                        }
                        break;
                    case UDAT_TIMEZONE_GENERIC_FIELD: // 'v'
                        if (count == 1) {
                            zsinfo = zsf->findGenericShort(text, start, matchLen, status);
                        } else if (count == 4) {
                            zsinfo = zsf->findGenericLong(text, start, matchLen, status);
                        }
                        break;
                    case UDAT_TIMEZONE_SPECIAL_FIELD: // 'V'
                        if (count == 1) {
                            zsinfo = zsf->findSpecificShort(text, start, matchLen, status);
                        } else if (count == 4) {
                            zsinfo = zsf->findGenericLocation(text, start, matchLen, status);
                        }
                        break;
                    default:
                        break;
                }

                if (U_SUCCESS(status) && zsinfo != NULL) {
                    if (zsinfo->isStandard()) {
                        ((SimpleDateFormat*)this)->tztype = TZTYPE_STD;
                    } else if (zsinfo->isDaylight()) {
                        ((SimpleDateFormat*)this)->tztype = TZTYPE_DST;
                    }
                    UnicodeString tzid;
                    zsinfo->getID(tzid);

                    UnicodeString current;
                    cal.getTimeZone().getID(current);
                    if (tzid != current) {
                        TimeZone *tz = TimeZone::createTimeZone(tzid);
                        cal.adoptTimeZone(tz);
                    }
                    return start + matchLen;
                }
            }
            // Step 4
            // Final attempt - is this standalone GMT/UT/UTC?
            int32_t gmtLen = 0;
            if (text.compare(start, kGmtLen, gGmt) == 0) {
                gmtLen = kGmtLen;
            } else if (text.compare(start, kUtcLen, gUtc) == 0) {
                gmtLen = kUtcLen;
            } else if (text.compare(start, kUtLen, gUt) == 0) {
                gmtLen = kUtLen;
            }
            if (gmtLen > 0) {
                TimeZone *tz = TimeZone::createTimeZone(UnicodeString("Etc/GMT"));
                cal.adoptTimeZone(tz);
                return start + gmtLen;
            }

            // complete failure
            return -start;
        }

    default:
        // Handle "generic" fields
        int32_t parseStart = pos.getIndex();
        const UnicodeString* src;
        if (obeyCount) {
            if ((start+count) > text.length()) {
                return -start;
            }
            text.extractBetween(0, start + count, temp);
            src = &temp;
        } else {
            src = &text;
        }
        parseInt(*src, number, pos, allowNegative,currentNumberFormat);
        if (pos.getIndex() != parseStart) {
            cal.set(field, number.getLong());
            return pos.getIndex();
        }
        return -start;
    }
}

/**
 * Parse an integer using fNumberFormat.  This method is semantically
 * const, but actually may modify fNumberFormat.
 */
void SimpleDateFormat::parseInt(const UnicodeString& text,
                                Formattable& number,
                                ParsePosition& pos,
                                UBool allowNegative,
                                NumberFormat *fmt) const {
    parseInt(text, number, -1, pos, allowNegative,fmt);
}

/**
 * Parse an integer using fNumberFormat up to maxDigits.
 */
void SimpleDateFormat::parseInt(const UnicodeString& text,
                                Formattable& number,
                                int32_t maxDigits,
                                ParsePosition& pos,
                                UBool allowNegative,
                                NumberFormat *fmt) const {
    UnicodeString oldPrefix;
    DecimalFormat* df = NULL;
    if (!allowNegative && (df = dynamic_cast<DecimalFormat*>(fmt)) != NULL) {
        df->getNegativePrefix(oldPrefix);
        df->setNegativePrefix(SUPPRESS_NEGATIVE_PREFIX);
    }
    int32_t oldPos = pos.getIndex();
    fmt->parse(text, number, pos);
    if (df != NULL) {
        df->setNegativePrefix(oldPrefix);
    }

    if (maxDigits > 0) {
        // adjust the result to fit into
        // the maxDigits and move the position back
        int32_t nDigits = pos.getIndex() - oldPos;
        if (nDigits > maxDigits) {
            int32_t val = number.getLong();
            nDigits -= maxDigits;
            while (nDigits > 0) {
                val /= 10;
                nDigits--;
            }
            pos.setIndex(oldPos + maxDigits);
            number.setLong(val);
        }
    }
}

//----------------------------------------------------------------------

void SimpleDateFormat::translatePattern(const UnicodeString& originalPattern,
                                        UnicodeString& translatedPattern,
                                        const UnicodeString& from,
                                        const UnicodeString& to,
                                        UErrorCode& status)
{
  // run through the pattern and convert any pattern symbols from the version
  // in "from" to the corresponding character ion "to".  This code takes
  // quoted strings into account (it doesn't try to translate them), and it signals
  // an error if a particular "pattern character" doesn't appear in "from".
  // Depending on the values of "from" and "to" this can convert from generic
  // to localized patterns or localized to generic.
  if (U_FAILURE(status))
    return;

  translatedPattern.remove();
  UBool inQuote = FALSE;
  for (int32_t i = 0; i < originalPattern.length(); ++i) {
    UChar c = originalPattern[i];
    if (inQuote) {
      if (c == QUOTE)
    inQuote = FALSE;
    }
    else {
      if (c == QUOTE)
    inQuote = TRUE;
      else if ((c >= 0x0061 /*'a'*/ && c <= 0x007A) /*'z'*/
           || (c >= 0x0041 /*'A'*/ && c <= 0x005A /*'Z'*/)) {
    int32_t ci = from.indexOf(c);
    if (ci == -1) {
      status = U_INVALID_FORMAT_ERROR;
      return;
    }
    c = to[ci];
      }
    }
    translatedPattern += c;
  }
  if (inQuote) {
    status = U_INVALID_FORMAT_ERROR;
    return;
  }
}

//----------------------------------------------------------------------

UnicodeString&
SimpleDateFormat::toPattern(UnicodeString& result) const
{
    result = fPattern;
    return result;
}

//----------------------------------------------------------------------

UnicodeString&
SimpleDateFormat::toLocalizedPattern(UnicodeString& result,
                                     UErrorCode& status) const
{
    translatePattern(fPattern, result, DateFormatSymbols::getPatternUChars(), fSymbols->fLocalPatternChars, status);
    return result;
}

//----------------------------------------------------------------------

void
SimpleDateFormat::applyPattern(const UnicodeString& pattern)
{
    fPattern = pattern;
}

//----------------------------------------------------------------------

void
SimpleDateFormat::applyLocalizedPattern(const UnicodeString& pattern,
                                        UErrorCode &status)
{
    translatePattern(pattern, fPattern, fSymbols->fLocalPatternChars, DateFormatSymbols::getPatternUChars(), status);
}

//----------------------------------------------------------------------

const DateFormatSymbols*
SimpleDateFormat::getDateFormatSymbols() const
{
    return fSymbols;
}

//----------------------------------------------------------------------

void
SimpleDateFormat::adoptDateFormatSymbols(DateFormatSymbols* newFormatSymbols)
{
    delete fSymbols;
    fSymbols = newFormatSymbols;
}

//----------------------------------------------------------------------
void
SimpleDateFormat::setDateFormatSymbols(const DateFormatSymbols& newFormatSymbols)
{
    delete fSymbols;
    fSymbols = new DateFormatSymbols(newFormatSymbols);
}


//----------------------------------------------------------------------


void SimpleDateFormat::adoptCalendar(Calendar* calendarToAdopt)
{
  UErrorCode status = U_ZERO_ERROR;
  DateFormat::adoptCalendar(calendarToAdopt);
  delete fSymbols;
  fSymbols=NULL;
  initializeSymbols(fLocale, fCalendar, status);  // we need new symbols
  initializeDefaultCentury();  // we need a new century (possibly)
}


//----------------------------------------------------------------------


UBool
SimpleDateFormat::isFieldUnitIgnored(UCalendarDateFields field) const {
    return isFieldUnitIgnored(fPattern, field);
}


UBool
SimpleDateFormat::isFieldUnitIgnored(const UnicodeString& pattern,
                                     UCalendarDateFields field) {
    int32_t fieldLevel = fgCalendarFieldToLevel[field];
    int32_t level;
    UChar ch;
    UBool inQuote = FALSE;
    UChar prevCh = 0;
    int32_t count = 0;

    for (int32_t i = 0; i < pattern.length(); ++i) {
        ch = pattern[i];
        if (ch != prevCh && count > 0) {
            level = fgPatternCharToLevel[prevCh - PATTERN_CHAR_BASE];
            // the larger the level, the smaller the field unit.
            if ( fieldLevel <= level ) {
                return FALSE;
            }
            count = 0;
        }
        if (ch == QUOTE) {
            if ((i+1) < pattern.length() && pattern[i+1] == QUOTE) {
                ++i;
            } else {
                inQuote = ! inQuote;
            }
        }
        else if ( ! inQuote && ((ch >= 0x0061 /*'a'*/ && ch <= 0x007A /*'z'*/)
                    || (ch >= 0x0041 /*'A'*/ && ch <= 0x005A /*'Z'*/))) {
            prevCh = ch;
            ++count;
        }
    }
    if ( count > 0 ) {
        // last item
        level = fgPatternCharToLevel[prevCh - PATTERN_CHAR_BASE];
            if ( fieldLevel <= level ) {
                return FALSE;
            }
    }
    return TRUE;
}

//----------------------------------------------------------------------

const Locale&
SimpleDateFormat::getSmpFmtLocale(void) const {
    return fLocale;
}

//----------------------------------------------------------------------

int32_t
SimpleDateFormat::checkIntSuffix(const UnicodeString& text, int32_t start,
                                 int32_t patLoc, UBool isNegative) const {
    // local variables
    UnicodeString suf;
    int32_t patternMatch;
    int32_t textPreMatch;
    int32_t textPostMatch;

    // check that we are still in range
    if ( (start > text.length()) ||
         (start < 0) ||
         (patLoc < 0) ||
         (patLoc > fPattern.length())) {
        // out of range, don't advance location in text
        return start;
    }

    // get the suffix
    DecimalFormat* decfmt = dynamic_cast<DecimalFormat*>(fNumberFormat);
    if (decfmt != NULL) {
        if (isNegative) {
            suf = decfmt->getNegativeSuffix(suf);
        }
        else {
            suf = decfmt->getPositiveSuffix(suf);
        }
    }

    // check for suffix
    if (suf.length() <= 0) {
        return start;
    }

    // check suffix will be encountered in the pattern
    patternMatch = compareSimpleAffix(suf,fPattern,patLoc);

    // check if a suffix will be encountered in the text
    textPreMatch = compareSimpleAffix(suf,text,start);

    // check if a suffix was encountered in the text
    textPostMatch = compareSimpleAffix(suf,text,start-suf.length());

    // check for suffix match
    if ((textPreMatch >= 0) && (patternMatch >= 0) && (textPreMatch == patternMatch)) {
        return start;
    }
    else if ((textPostMatch >= 0) && (patternMatch >= 0) && (textPostMatch == patternMatch)) {
        return  start - suf.length();
    }

    // should not get here
    return start;
}

//----------------------------------------------------------------------

int32_t
SimpleDateFormat::compareSimpleAffix(const UnicodeString& affix,
                   const UnicodeString& input,
                   int32_t pos) const {
    int32_t start = pos;
    for (int32_t i=0; i<affix.length(); ) {
        UChar32 c = affix.char32At(i);
        int32_t len = U16_LENGTH(c);
        if (uprv_isRuleWhiteSpace(c)) {
            // We may have a pattern like: \u200F \u0020
            //        and input text like: \u200F \u0020
            // Note that U+200F and U+0020 are RuleWhiteSpace but only
            // U+0020 is UWhiteSpace.  So we have to first do a direct
            // match of the run of RULE whitespace in the pattern,
            // then match any extra characters.
            UBool literalMatch = FALSE;
            while (pos < input.length() &&
                   input.char32At(pos) == c) {
                literalMatch = TRUE;
                i += len;
                pos += len;
                if (i == affix.length()) {
                    break;
                }
                c = affix.char32At(i);
                len = U16_LENGTH(c);
                if (!uprv_isRuleWhiteSpace(c)) {
                    break;
                }
            }

            // Advance over run in pattern
            i = skipRuleWhiteSpace(affix, i);

            // Advance over run in input text
            // Must see at least one white space char in input,
            // unless we've already matched some characters literally.
            int32_t s = pos;
            pos = skipUWhiteSpace(input, pos);
            if (pos == s && !literalMatch) {
                return -1;
            }

            // If we skip UWhiteSpace in the input text, we need to skip it in the pattern.
            // Otherwise, the previous lines may have skipped over text (such as U+00A0) that
            // is also in the affix.
            i = skipUWhiteSpace(affix, i);
        } else {
            if (pos < input.length() &&
                input.char32At(pos) == c) {
                i += len;
                pos += len;
            } else {
                return -1;
            }
        }
    }
    return pos - start;
}

//----------------------------------------------------------------------

int32_t
SimpleDateFormat::skipRuleWhiteSpace(const UnicodeString& text, int32_t pos) const {
    while (pos < text.length()) {
        UChar32 c = text.char32At(pos);
        if (!uprv_isRuleWhiteSpace(c)) {
            break;
        }
        pos += U16_LENGTH(c);
    }
    return pos;
}

//----------------------------------------------------------------------

int32_t
SimpleDateFormat::skipUWhiteSpace(const UnicodeString& text, int32_t pos) const {
    while (pos < text.length()) {
        UChar32 c = text.char32At(pos);
        if (!u_isUWhiteSpace(c)) {
            break;
        }
        pos += U16_LENGTH(c);
    }
    return pos;
}

U_NAMESPACE_END

#endif /* #if !UCONFIG_NO_FORMATTING */

//eof
