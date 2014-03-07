/*
*******************************************************************************
* Copyright (C) 1997-2009, International Business Machines Corporation and    *
* others. All Rights Reserved.                                                *
*******************************************************************************
*
* File DTFMTSYM.CPP
*
* Modification History:
*
*   Date        Name        Description
*   02/19/97    aliu        Converted from java.
*   07/21/98    stephen     Added getZoneIndex
*                            Changed weekdays/short weekdays to be one-based
*   06/14/99    stephen     Removed SimpleDateFormat::fgTimeZoneDataSuffix
*   11/16/99    weiv        Added 'Y' and 'e' to fgPatternChars
*   03/27/00    weiv        Keeping resource bundle around!
*   06/30/05    emmons      Added eraNames, narrow month/day, standalone context
*   10/12/05    emmons      Added setters for eraNames, month/day by width/context
*******************************************************************************
*/
#include "unicode/utypes.h"

#if !UCONFIG_NO_FORMATTING
#include "unicode/ustring.h"
#include "unicode/dtfmtsym.h"
#include "unicode/smpdtfmt.h"
#include "unicode/msgfmt.h"
#include "cpputils.h"
#include "ucln_in.h"
#include "umutex.h"
#include "cmemory.h"
#include "cstring.h"
#include "locbased.h"
#include "gregoimp.h"
#include "hash.h"
#include "uresimp.h"
#include "zstrfmt.h"
#include "ureslocs.h"

// *****************************************************************************
// class DateFormatSymbols
// *****************************************************************************

/**
 * These are static arrays we use only in the case where we have no
 * resource data.
 */

#define PATTERN_CHARS_LEN 30

/**
 * Unlocalized date-time pattern characters. For example: 'y', 'd', etc. All
 * locales use the same these unlocalized pattern characters.
 */
static const UChar gPatternChars[] = {
    // GyMdkHmsSEDFwWahKzYeugAZvcLQqV
    0x47, 0x79, 0x4D, 0x64, 0x6B, 0x48, 0x6D, 0x73, 0x53, 0x45,
    0x44, 0x46, 0x77, 0x57, 0x61, 0x68, 0x4B, 0x7A, 0x59, 0x65,
    0x75, 0x67, 0x41, 0x5A, 0x76, 0x63, 0x4c, 0x51, 0x71, 0x56, 0
};

/* length of an array */
#define ARRAY_LENGTH(array) (sizeof(array)/sizeof(array[0]))

//------------------------------------------------------
// Strings of last resort.  These are only used if we have no resource
// files.  They aren't designed for actual use, just for backup.

// These are the month names and abbreviations of last resort.
static const UChar gLastResortMonthNames[13][3] =
{
    {0x0030, 0x0031, 0x0000}, /* "01" */
    {0x0030, 0x0032, 0x0000}, /* "02" */
    {0x0030, 0x0033, 0x0000}, /* "03" */
    {0x0030, 0x0034, 0x0000}, /* "04" */
    {0x0030, 0x0035, 0x0000}, /* "05" */
    {0x0030, 0x0036, 0x0000}, /* "06" */
    {0x0030, 0x0037, 0x0000}, /* "07" */
    {0x0030, 0x0038, 0x0000}, /* "08" */
    {0x0030, 0x0039, 0x0000}, /* "09" */
    {0x0031, 0x0030, 0x0000}, /* "10" */
    {0x0031, 0x0031, 0x0000}, /* "11" */
    {0x0031, 0x0032, 0x0000}, /* "12" */
    {0x0031, 0x0033, 0x0000}  /* "13" */
};

// These are the weekday names and abbreviations of last resort.
static const UChar gLastResortDayNames[8][2] =
{
    {0x0030, 0x0000}, /* "0" */
    {0x0031, 0x0000}, /* "1" */
    {0x0032, 0x0000}, /* "2" */
    {0x0033, 0x0000}, /* "3" */
    {0x0034, 0x0000}, /* "4" */
    {0x0035, 0x0000}, /* "5" */
    {0x0036, 0x0000}, /* "6" */
    {0x0037, 0x0000}  /* "7" */
};

// These are the quarter names and abbreviations of last resort.
static const UChar gLastResortQuarters[4][2] =
{
    {0x0031, 0x0000}, /* "1" */
    {0x0032, 0x0000}, /* "2" */
    {0x0033, 0x0000}, /* "3" */
    {0x0034, 0x0000}, /* "4" */
};

// These are the am/pm and BC/AD markers of last resort.
static const UChar gLastResortAmPmMarkers[2][3] =
{
    {0x0041, 0x004D, 0x0000}, /* "AM" */
    {0x0050, 0x004D, 0x0000}  /* "PM" */
};

static const UChar gLastResortEras[2][3] =
{
    {0x0042, 0x0043, 0x0000}, /* "BC" */
    {0x0041, 0x0044, 0x0000}  /* "AD" */
};


// These are the zone strings of last resort.
static const UChar gLastResortZoneStrings[7][4] =
{
    {0x0047, 0x004D, 0x0054, 0x0000}, /* "GMT" */
    {0x0047, 0x004D, 0x0054, 0x0000}, /* "GMT" */
    {0x0047, 0x004D, 0x0054, 0x0000}, /* "GMT" */
    {0x0047, 0x004D, 0x0054, 0x0000}, /* "GMT" */
    {0x0047, 0x004D, 0x0054, 0x0000}, /* "GMT" */
    {0x0047, 0x004D, 0x0054, 0x0000}, /* "GMT" */
    {0x0047, 0x004D, 0x0054, 0x0000}  /* "GMT" */
};

static const UChar gLastResortGmtFormat[] =
    {0x0047, 0x004D, 0x0054, 0x007B, 0x0030, 0x007D, 0x0000}; /* GMT{0} */

static const UChar gLastResortGmtHourFormats[4][10] =
{
    {0x002D, 0x0048, 0x0048, 0x003A, 0x006D, 0x006D, 0x003A, 0x0073, 0x0073, 0x0000}, /* -HH:mm:ss */
    {0x002D, 0x0048, 0x0048, 0x003A, 0x006D, 0x006D, 0x0000, 0x0000, 0x0000, 0x0000}, /* -HH:mm */
    {0x002B, 0x0048, 0x0048, 0x003A, 0x006D, 0x006D, 0x003A, 0x0073, 0x0073, 0x0000}, /* +HH:mm:ss */
    {0x002B, 0x0048, 0x0048, 0x003A, 0x006D, 0x006D, 0x0000, 0x0000, 0x0000, 0x0000}  /* +HH:mm */
};

/* Sizes for the last resort string arrays */
typedef enum LastResortSize {
    kMonthNum = 13,
    kMonthLen = 3,

    kDayNum = 8,
    kDayLen = 2,

    kAmPmNum = 2,
    kAmPmLen = 3,

    kQuarterNum = 4,
    kQuarterLen = 2,

    kEraNum = 2,
    kEraLen = 3,

    kZoneNum = 5,
    kZoneLen = 4,

    kGmtHourNum = 4,
    kGmtHourLen = 10
} LastResortSize;

U_NAMESPACE_BEGIN

UOBJECT_DEFINE_RTTI_IMPLEMENTATION(DateFormatSymbols)

#define kSUPPLEMENTAL "supplementalData"

/**
 * These are the tags we expect to see in normal resource bundle files associated
 * with a locale and calendar
 */
static const char gErasTag[]="eras";
static const char gMonthNamesTag[]="monthNames";
static const char gDayNamesTag[]="dayNames";
static const char gNamesWideTag[]="wide";
static const char gNamesAbbrTag[]="abbreviated";
static const char gNamesNarrowTag[]="narrow";
static const char gNamesStandaloneTag[]="stand-alone";
static const char gAmPmMarkersTag[]="AmPmMarkers";
static const char gQuartersTag[]="quarters";

static const char gZoneStringsTag[]="zoneStrings";
static const char gGmtFormatTag[]="gmtFormat";
static const char gHourFormatTag[]="hourFormat";

static const char gLocalPatternCharsTag[]="localPatternChars";

static UMTX LOCK;

/**
 * Jitterbug 2974: MSVC has a bug whereby new X[0] behaves badly.
 * Work around this.
 */
static inline UnicodeString* newUnicodeStringArray(size_t count) {
    return new UnicodeString[count ? count : 1];
}

//------------------------------------------------------

DateFormatSymbols::DateFormatSymbols(const Locale& locale,
                                     UErrorCode& status)
    : UObject()
{
  initializeData(locale, NULL,  status);
}

DateFormatSymbols::DateFormatSymbols(UErrorCode& status)
    : UObject()
{
  initializeData(Locale::getDefault(), NULL, status, TRUE);
}


DateFormatSymbols::DateFormatSymbols(const Locale& locale,
                                     const char *type,
                                     UErrorCode& status)
    : UObject()
{
  initializeData(locale, type,  status);
}

DateFormatSymbols::DateFormatSymbols(const char *type, UErrorCode& status)
    : UObject()
{
  initializeData(Locale::getDefault(), type, status, TRUE);
}

DateFormatSymbols::DateFormatSymbols(const DateFormatSymbols& other)
    : UObject(other)
{
    copyData(other);
}

void
DateFormatSymbols::assignArray(UnicodeString*& dstArray,
                               int32_t& dstCount,
                               const UnicodeString* srcArray,
                               int32_t srcCount)
{
    // assignArray() is only called by copyData(), which in turn implements the
    // copy constructor and the assignment operator.
    // All strings in a DateFormatSymbols object are created in one of the following
    // three ways that all allow to safely use UnicodeString::fastCopyFrom():
    // - readonly-aliases from resource bundles
    // - readonly-aliases or allocated strings from constants
    // - safely cloned strings (with owned buffers) from setXYZ() functions
    //
    // Note that this is true for as long as DateFormatSymbols can be constructed
    // only from a locale bundle or set via the cloning API,
    // *and* for as long as all the strings are in *private* fields, preventing
    // a subclass from creating these strings in an "unsafe" way (with respect to fastCopyFrom()).
    dstCount = srcCount;
    dstArray = newUnicodeStringArray(srcCount);
    if(dstArray != NULL) {
        int32_t i;
        for(i=0; i<srcCount; ++i) {
            dstArray[i].fastCopyFrom(srcArray[i]);
        }
    }
}

/**
 * Create a copy, in fZoneStrings, of the given zone strings array.  The
 * member variables fZoneStringsRowCount and fZoneStringsColCount should
 * be set already by the caller.
 */
void
DateFormatSymbols::createZoneStrings(const UnicodeString *const * otherStrings)
{
    int32_t row, col;
    UBool failed = FALSE;

    fZoneStrings = (UnicodeString **)uprv_malloc(fZoneStringsRowCount * sizeof(UnicodeString *));
    if (fZoneStrings != NULL) {
        for (row=0; row<fZoneStringsRowCount; ++row)
        {
            fZoneStrings[row] = newUnicodeStringArray(fZoneStringsColCount);
            if (fZoneStrings[row] == NULL) {
                failed = TRUE;
                break;
            }
            for (col=0; col<fZoneStringsColCount; ++col) {
                // fastCopyFrom() - see assignArray comments
                fZoneStrings[row][col].fastCopyFrom(otherStrings[row][col]);
            }
        }
    }
    // If memory allocation failed, roll back and delete fZoneStrings
    if (failed) {
        for (int i = row; i >= 0; i--) {
            delete[] fZoneStrings[i];
        }
        uprv_free(fZoneStrings);
        fZoneStrings = NULL;
    }
}

/**
 * Copy all of the other's data to this.
 */
void
DateFormatSymbols::copyData(const DateFormatSymbols& other) {
    assignArray(fEras, fErasCount, other.fEras, other.fErasCount);
    assignArray(fEraNames, fEraNamesCount, other.fEraNames, other.fEraNamesCount);
    assignArray(fNarrowEras, fNarrowErasCount, other.fNarrowEras, other.fNarrowErasCount);
    assignArray(fMonths, fMonthsCount, other.fMonths, other.fMonthsCount);
    assignArray(fShortMonths, fShortMonthsCount, other.fShortMonths, other.fShortMonthsCount);
    assignArray(fNarrowMonths, fNarrowMonthsCount, other.fNarrowMonths, other.fNarrowMonthsCount);
    assignArray(fStandaloneMonths, fStandaloneMonthsCount, other.fStandaloneMonths, other.fStandaloneMonthsCount);
    assignArray(fStandaloneShortMonths, fStandaloneShortMonthsCount, other.fStandaloneShortMonths, other.fStandaloneShortMonthsCount);
    assignArray(fStandaloneNarrowMonths, fStandaloneNarrowMonthsCount, other.fStandaloneNarrowMonths, other.fStandaloneNarrowMonthsCount);
    assignArray(fWeekdays, fWeekdaysCount, other.fWeekdays, other.fWeekdaysCount);
    assignArray(fShortWeekdays, fShortWeekdaysCount, other.fShortWeekdays, other.fShortWeekdaysCount);
    assignArray(fNarrowWeekdays, fNarrowWeekdaysCount, other.fNarrowWeekdays, other.fNarrowWeekdaysCount);
    assignArray(fStandaloneWeekdays, fStandaloneWeekdaysCount, other.fStandaloneWeekdays, other.fStandaloneWeekdaysCount);
    assignArray(fStandaloneShortWeekdays, fStandaloneShortWeekdaysCount, other.fStandaloneShortWeekdays, other.fStandaloneShortWeekdaysCount);
    assignArray(fStandaloneNarrowWeekdays, fStandaloneNarrowWeekdaysCount, other.fStandaloneNarrowWeekdays, other.fStandaloneNarrowWeekdaysCount);
    assignArray(fAmPms, fAmPmsCount, other.fAmPms, other.fAmPmsCount);
    assignArray(fQuarters, fQuartersCount, other.fQuarters, other.fQuartersCount);
    assignArray(fShortQuarters, fShortQuartersCount, other.fShortQuarters, other.fShortQuartersCount);
    assignArray(fStandaloneQuarters, fStandaloneQuartersCount, other.fStandaloneQuarters, other.fStandaloneQuartersCount);
    assignArray(fStandaloneShortQuarters, fStandaloneShortQuartersCount, other.fStandaloneShortQuarters, other.fStandaloneShortQuartersCount);
    fGmtFormat = other.fGmtFormat;
    assignArray(fGmtHourFormats, fGmtHourFormatsCount, other.fGmtHourFormats, other.fGmtHourFormatsCount);
 
    if (other.fZoneStrings != NULL) {
        fZoneStringsColCount = other.fZoneStringsColCount;
        fZoneStringsRowCount = other.fZoneStringsRowCount;
        createZoneStrings((const UnicodeString**)other.fZoneStrings);

    } else {
        fZoneStrings = NULL;
        fZoneStringsColCount = 0;
        fZoneStringsRowCount = 0;
    }
    fZSFLocale = other.fZSFLocale;
    // Other zone strings data is created on demand
    fZoneStringFormat = NULL;
    fLocaleZoneStrings = NULL;
    fZSFCachePtr = NULL;
    fZSFLocal = NULL;

    // fastCopyFrom() - see assignArray comments
    fLocalPatternChars.fastCopyFrom(other.fLocalPatternChars);
}

/**
 * Assignment operator.
 */
DateFormatSymbols& DateFormatSymbols::operator=(const DateFormatSymbols& other)
{
    dispose();
    copyData(other);

    return *this;
}

DateFormatSymbols::~DateFormatSymbols()
{
    dispose();
}

void DateFormatSymbols::dispose()
{
    if (fEras)                     delete[] fEras;
    if (fEraNames)                 delete[] fEraNames;
    if (fNarrowEras)               delete[] fNarrowEras;
    if (fMonths)                   delete[] fMonths;
    if (fShortMonths)              delete[] fShortMonths;
    if (fNarrowMonths)             delete[] fNarrowMonths;
    if (fStandaloneMonths)         delete[] fStandaloneMonths;
    if (fStandaloneShortMonths)    delete[] fStandaloneShortMonths;
    if (fStandaloneNarrowMonths)   delete[] fStandaloneNarrowMonths;
    if (fWeekdays)                 delete[] fWeekdays;
    if (fShortWeekdays)            delete[] fShortWeekdays;
    if (fNarrowWeekdays)           delete[] fNarrowWeekdays;
    if (fStandaloneWeekdays)       delete[] fStandaloneWeekdays;
    if (fStandaloneShortWeekdays)  delete[] fStandaloneShortWeekdays;
    if (fStandaloneNarrowWeekdays) delete[] fStandaloneNarrowWeekdays;
    if (fAmPms)                    delete[] fAmPms;
    if (fQuarters)                 delete[] fQuarters;
    if (fShortQuarters)            delete[] fShortQuarters;
    if (fStandaloneQuarters)       delete[] fStandaloneQuarters;
    if (fStandaloneShortQuarters)  delete[] fStandaloneShortQuarters;
    if (fGmtHourFormats)           delete[] fGmtHourFormats;

    disposeZoneStrings();
}

void DateFormatSymbols::disposeZoneStrings()
{
    if (fZoneStrings) {
        for (int32_t row = 0; row < fZoneStringsRowCount; ++row) {
            delete[] fZoneStrings[row];
        }
        uprv_free(fZoneStrings);
    }
    if (fLocaleZoneStrings) {
        for (int32_t row = 0; row < fZoneStringsRowCount; ++row) {
            delete[] fLocaleZoneStrings[row];
        }
        uprv_free(fLocaleZoneStrings);
    }
    if (fZSFLocal) {
        delete fZSFLocal;
    }
    if (fZSFCachePtr) {
        delete fZSFCachePtr;
    }

    fZoneStrings = NULL;
    fLocaleZoneStrings = NULL;
    fZoneStringsRowCount = 0;
    fZoneStringsColCount = 0;

    fZoneStringFormat = NULL;
    fZSFLocal = NULL;
    fZSFCachePtr = NULL;
}

UBool
DateFormatSymbols::arrayCompare(const UnicodeString* array1,
                                const UnicodeString* array2,
                                int32_t count)
{
    if (array1 == array2) return TRUE;
    while (count>0)
    {
        --count;
        if (array1[count] != array2[count]) return FALSE;
    }
    return TRUE;
}

UBool
DateFormatSymbols::operator==(const DateFormatSymbols& other) const
{
    // First do cheap comparisons
    if (this == &other) {
        return TRUE;
    }
    if (fErasCount == other.fErasCount &&
        fEraNamesCount == other.fEraNamesCount &&
        fNarrowErasCount == other.fNarrowErasCount &&
        fMonthsCount == other.fMonthsCount &&
        fShortMonthsCount == other.fShortMonthsCount &&
        fNarrowMonthsCount == other.fNarrowMonthsCount &&
        fStandaloneMonthsCount == other.fStandaloneMonthsCount &&
        fStandaloneShortMonthsCount == other.fStandaloneShortMonthsCount &&
        fStandaloneNarrowMonthsCount == other.fStandaloneNarrowMonthsCount &&
        fWeekdaysCount == other.fWeekdaysCount &&
        fShortWeekdaysCount == other.fShortWeekdaysCount &&
        fNarrowWeekdaysCount == other.fNarrowWeekdaysCount &&
        fStandaloneWeekdaysCount == other.fStandaloneWeekdaysCount &&
        fStandaloneShortWeekdaysCount == other.fStandaloneShortWeekdaysCount &&
        fStandaloneNarrowWeekdaysCount == other.fStandaloneNarrowWeekdaysCount &&
        fAmPmsCount == other.fAmPmsCount &&
        fQuartersCount == other.fQuartersCount &&
        fShortQuartersCount == other.fShortQuartersCount &&
        fStandaloneQuartersCount == other.fStandaloneQuartersCount &&
        fStandaloneShortQuartersCount == other.fStandaloneShortQuartersCount &&
        fGmtHourFormatsCount == other.fGmtHourFormatsCount &&
        fGmtFormat == other.fGmtFormat)
    {
        // Now compare the arrays themselves
        if (arrayCompare(fEras, other.fEras, fErasCount) &&
            arrayCompare(fEraNames, other.fEraNames, fEraNamesCount) &&
            arrayCompare(fNarrowEras, other.fNarrowEras, fNarrowErasCount) &&
            arrayCompare(fMonths, other.fMonths, fMonthsCount) &&
            arrayCompare(fShortMonths, other.fShortMonths, fShortMonthsCount) &&
            arrayCompare(fNarrowMonths, other.fNarrowMonths, fNarrowMonthsCount) &&
            arrayCompare(fStandaloneMonths, other.fStandaloneMonths, fStandaloneMonthsCount) &&
            arrayCompare(fStandaloneShortMonths, other.fStandaloneShortMonths, fStandaloneShortMonthsCount) &&
            arrayCompare(fStandaloneNarrowMonths, other.fStandaloneNarrowMonths, fStandaloneNarrowMonthsCount) &&
            arrayCompare(fWeekdays, other.fWeekdays, fWeekdaysCount) &&
            arrayCompare(fShortWeekdays, other.fShortWeekdays, fShortWeekdaysCount) &&
            arrayCompare(fNarrowWeekdays, other.fNarrowWeekdays, fNarrowWeekdaysCount) &&
            arrayCompare(fStandaloneWeekdays, other.fStandaloneWeekdays, fStandaloneWeekdaysCount) &&
            arrayCompare(fStandaloneShortWeekdays, other.fStandaloneShortWeekdays, fStandaloneShortWeekdaysCount) &&
            arrayCompare(fStandaloneNarrowWeekdays, other.fStandaloneNarrowWeekdays, fStandaloneNarrowWeekdaysCount) &&
            arrayCompare(fAmPms, other.fAmPms, fAmPmsCount) &&
            arrayCompare(fQuarters, other.fQuarters, fQuartersCount) &&
            arrayCompare(fShortQuarters, other.fShortQuarters, fShortQuartersCount) &&
            arrayCompare(fStandaloneQuarters, other.fStandaloneQuarters, fStandaloneQuartersCount) &&
            arrayCompare(fStandaloneShortQuarters, other.fStandaloneShortQuarters, fStandaloneShortQuartersCount) &&
            arrayCompare(fGmtHourFormats, other.fGmtHourFormats, fGmtHourFormatsCount))
        {
            // Compare the contents of fZoneStrings
            if (fZoneStrings == NULL && other.fZoneStrings == NULL) {
                if (fZSFLocale == other.fZSFLocale) {
                    return TRUE;
                }
            } else if (fZoneStrings != NULL && other.fZoneStrings != NULL) {
                if (fZoneStringsRowCount == other.fZoneStringsRowCount
                    && fZoneStringsColCount == other.fZoneStringsColCount) {
                    UBool cmpres = TRUE;
                    for (int32_t i = 0; (i < fZoneStringsRowCount) && cmpres; i++) {
                        cmpres = arrayCompare(fZoneStrings[i], other.fZoneStrings[i], fZoneStringsColCount);
                    }
                    return cmpres;
                }
            }
            return FALSE;
        }
    }
    return FALSE;
}

//------------------------------------------------------

const UnicodeString*
DateFormatSymbols::getEras(int32_t &count) const
{
    count = fErasCount;
    return fEras;
}

const UnicodeString*
DateFormatSymbols::getEraNames(int32_t &count) const
{
    count = fEraNamesCount;
    return fEraNames;
}

const UnicodeString*
DateFormatSymbols::getNarrowEras(int32_t &count) const
{
    count = fNarrowErasCount;
    return fNarrowEras;
}

const UnicodeString*
DateFormatSymbols::getMonths(int32_t &count) const
{
    count = fMonthsCount;
    return fMonths;
}

const UnicodeString*
DateFormatSymbols::getShortMonths(int32_t &count) const
{
    count = fShortMonthsCount;
    return fShortMonths;
}

const UnicodeString*
DateFormatSymbols::getMonths(int32_t &count, DtContextType context, DtWidthType width ) const
{
    UnicodeString *returnValue = NULL;

    switch (context) {
    case FORMAT :
        switch(width) {
        case WIDE :
            count = fMonthsCount;
            returnValue = fMonths;
            break;
        case ABBREVIATED :
            count = fShortMonthsCount;
            returnValue = fShortMonths;
            break;
        case NARROW :
            count = fNarrowMonthsCount;
            returnValue = fNarrowMonths;
            break;
        case DT_WIDTH_COUNT :
            break;
        }
        break;
    case STANDALONE :
        switch(width) {
        case WIDE :
            count = fStandaloneMonthsCount;
            returnValue = fStandaloneMonths;
            break;
        case ABBREVIATED :
            count = fStandaloneShortMonthsCount;
            returnValue = fStandaloneShortMonths;
            break;
        case NARROW :
            count = fStandaloneNarrowMonthsCount;
            returnValue = fStandaloneNarrowMonths;
            break;
        case DT_WIDTH_COUNT :
            break;
        }
        break;
    case DT_CONTEXT_COUNT :
        break;
    }
    return returnValue;
}

const UnicodeString*
DateFormatSymbols::getWeekdays(int32_t &count) const
{
    count = fWeekdaysCount;
    return fWeekdays;
}

const UnicodeString*
DateFormatSymbols::getShortWeekdays(int32_t &count) const
{
    count = fShortWeekdaysCount;
    return fShortWeekdays;
}

const UnicodeString*
DateFormatSymbols::getWeekdays(int32_t &count, DtContextType context, DtWidthType width) const
{
    UnicodeString *returnValue = NULL;
    switch (context) {
    case FORMAT :
        switch(width) {
            case WIDE :
                count = fWeekdaysCount;
                returnValue = fWeekdays;
                break;
            case ABBREVIATED :
                count = fShortWeekdaysCount;
                returnValue = fShortWeekdays;
                break;
            case NARROW :
                count = fNarrowWeekdaysCount;
                returnValue = fNarrowWeekdays;
                break;
            case DT_WIDTH_COUNT :
                break;
        }
        break;
    case STANDALONE :
        switch(width) {
            case WIDE :
                count = fStandaloneWeekdaysCount;
                returnValue = fStandaloneWeekdays;
                break;
            case ABBREVIATED :
                count = fStandaloneShortWeekdaysCount;
                returnValue = fStandaloneShortWeekdays;
                break;
            case NARROW :
                count = fStandaloneNarrowWeekdaysCount;
                returnValue = fStandaloneNarrowWeekdays;
                break;
            case DT_WIDTH_COUNT :
                break;
        }
        break;
    case DT_CONTEXT_COUNT :
        break;
    }
    return returnValue;
}

const UnicodeString*
DateFormatSymbols::getQuarters(int32_t &count, DtContextType context, DtWidthType width ) const
{
    UnicodeString *returnValue = NULL;

    switch (context) {
    case FORMAT :
        switch(width) {
        case WIDE :
            count = fQuartersCount;
            returnValue = fQuarters;
            break;
        case ABBREVIATED :
            count = fShortQuartersCount;
            returnValue = fShortQuarters;
            break;
        case NARROW :
            count = 0;
            returnValue = NULL;
            break;
        case DT_WIDTH_COUNT :
            break;
        }
        break;
    case STANDALONE :
        switch(width) {
        case WIDE :
            count = fStandaloneQuartersCount;
            returnValue = fStandaloneQuarters;
            break;
        case ABBREVIATED :
            count = fStandaloneShortQuartersCount;
            returnValue = fStandaloneShortQuarters;
            break;
        case NARROW :
            count = 0;
            returnValue = NULL;
            break;
        case DT_WIDTH_COUNT :
            break;
        }
        break;
    case DT_CONTEXT_COUNT :
        break;
    }
    return returnValue;
}

const UnicodeString*
DateFormatSymbols::getAmPmStrings(int32_t &count) const
{
    count = fAmPmsCount;
    return fAmPms;
}

//------------------------------------------------------

void
DateFormatSymbols::setEras(const UnicodeString* erasArray, int32_t count)
{
    // delete the old list if we own it
    if (fEras)
        delete[] fEras;

    // we always own the new list, which we create here (we duplicate rather
    // than adopting the list passed in)
    fEras = newUnicodeStringArray(count);
    uprv_arrayCopy(erasArray,fEras,  count);
    fErasCount = count;
}

void
DateFormatSymbols::setEraNames(const UnicodeString* eraNamesArray, int32_t count)
{
    // delete the old list if we own it
    if (fEraNames)
        delete[] fEraNames;

    // we always own the new list, which we create here (we duplicate rather
    // than adopting the list passed in)
    fEraNames = newUnicodeStringArray(count);
    uprv_arrayCopy(eraNamesArray,fEraNames,  count);
    fEraNamesCount = count;
}

void
DateFormatSymbols::setNarrowEras(const UnicodeString* narrowErasArray, int32_t count)
{
    // delete the old list if we own it
    if (fNarrowEras)
        delete[] fNarrowEras;

    // we always own the new list, which we create here (we duplicate rather
    // than adopting the list passed in)
    fNarrowEras = newUnicodeStringArray(count);
    uprv_arrayCopy(narrowErasArray,fNarrowEras,  count);
    fNarrowErasCount = count;
}

void
DateFormatSymbols::setMonths(const UnicodeString* monthsArray, int32_t count)
{
    // delete the old list if we own it
    if (fMonths)
        delete[] fMonths;

    // we always own the new list, which we create here (we duplicate rather
    // than adopting the list passed in)
    fMonths = newUnicodeStringArray(count);
    uprv_arrayCopy( monthsArray,fMonths,count);
    fMonthsCount = count;
}

void
DateFormatSymbols::setShortMonths(const UnicodeString* shortMonthsArray, int32_t count)
{
    // delete the old list if we own it
    if (fShortMonths)
        delete[] fShortMonths;

    // we always own the new list, which we create here (we duplicate rather
    // than adopting the list passed in)
    fShortMonths = newUnicodeStringArray(count);
    uprv_arrayCopy(shortMonthsArray,fShortMonths,  count);
    fShortMonthsCount = count;
}

void
DateFormatSymbols::setMonths(const UnicodeString* monthsArray, int32_t count, DtContextType context, DtWidthType width)
{
    // delete the old list if we own it
    // we always own the new list, which we create here (we duplicate rather
    // than adopting the list passed in)

    switch (context) {
    case FORMAT :
        switch (width) {
        case WIDE :
            if (fMonths)
                delete[] fMonths;
            fMonths = newUnicodeStringArray(count);
            uprv_arrayCopy( monthsArray,fMonths,count);
            fMonthsCount = count;
            break;
        case ABBREVIATED :
            if (fShortMonths)
                delete[] fShortMonths;
            fShortMonths = newUnicodeStringArray(count);
            uprv_arrayCopy( monthsArray,fShortMonths,count);
            fShortMonthsCount = count;
            break;
        case NARROW :
            if (fNarrowMonths)
                delete[] fNarrowMonths;
            fNarrowMonths = newUnicodeStringArray(count);
            uprv_arrayCopy( monthsArray,fNarrowMonths,count);
            fNarrowMonthsCount = count;
            break; 
        case DT_WIDTH_COUNT :
            break;
        }
        break;
    case STANDALONE :
        switch (width) {
        case WIDE :
            if (fStandaloneMonths)
                delete[] fStandaloneMonths;
            fStandaloneMonths = newUnicodeStringArray(count);
            uprv_arrayCopy( monthsArray,fStandaloneMonths,count);
            fStandaloneMonthsCount = count;
            break;
        case ABBREVIATED :
            if (fStandaloneShortMonths)
                delete[] fStandaloneShortMonths;
            fStandaloneShortMonths = newUnicodeStringArray(count);
            uprv_arrayCopy( monthsArray,fStandaloneShortMonths,count);
            fStandaloneShortMonthsCount = count;
            break;
        case NARROW :
           if (fStandaloneNarrowMonths)
                delete[] fStandaloneNarrowMonths;
            fStandaloneNarrowMonths = newUnicodeStringArray(count);
            uprv_arrayCopy( monthsArray,fStandaloneNarrowMonths,count);
            fStandaloneNarrowMonthsCount = count;
            break; 
        case DT_WIDTH_COUNT :
            break;
        }
        break;
    case DT_CONTEXT_COUNT :
        break;
    }
}

void DateFormatSymbols::setWeekdays(const UnicodeString* weekdaysArray, int32_t count)
{
    // delete the old list if we own it
    if (fWeekdays)
        delete[] fWeekdays;

    // we always own the new list, which we create here (we duplicate rather
    // than adopting the list passed in)
    fWeekdays = newUnicodeStringArray(count);
    uprv_arrayCopy(weekdaysArray,fWeekdays,count);
    fWeekdaysCount = count;
}

void
DateFormatSymbols::setShortWeekdays(const UnicodeString* shortWeekdaysArray, int32_t count)
{
    // delete the old list if we own it
    if (fShortWeekdays)
        delete[] fShortWeekdays;

    // we always own the new list, which we create here (we duplicate rather
    // than adopting the list passed in)
    fShortWeekdays = newUnicodeStringArray(count);
    uprv_arrayCopy(shortWeekdaysArray, fShortWeekdays, count);
    fShortWeekdaysCount = count;
}

void
DateFormatSymbols::setWeekdays(const UnicodeString* weekdaysArray, int32_t count, DtContextType context, DtWidthType width)
{
    // delete the old list if we own it
    // we always own the new list, which we create here (we duplicate rather
    // than adopting the list passed in)

    switch (context) {
    case FORMAT :
        switch (width) {
        case WIDE :
            if (fWeekdays)
                delete[] fWeekdays;
            fWeekdays = newUnicodeStringArray(count);
            uprv_arrayCopy(weekdaysArray, fWeekdays, count);
            fWeekdaysCount = count;
            break;
        case ABBREVIATED :
            if (fShortWeekdays)
                delete[] fShortWeekdays;
            fShortWeekdays = newUnicodeStringArray(count);
            uprv_arrayCopy(weekdaysArray, fShortWeekdays, count);
            fShortWeekdaysCount = count;
            break;
        case NARROW :
            if (fNarrowWeekdays)
                delete[] fNarrowWeekdays;
            fNarrowWeekdays = newUnicodeStringArray(count);
            uprv_arrayCopy(weekdaysArray, fNarrowWeekdays, count);
            fNarrowWeekdaysCount = count;
            break; 
        case DT_WIDTH_COUNT :
            break;
        }
        break;
    case STANDALONE :
        switch (width) {
        case WIDE :
            if (fStandaloneWeekdays)
                delete[] fStandaloneWeekdays;
            fStandaloneWeekdays = newUnicodeStringArray(count);
            uprv_arrayCopy(weekdaysArray, fStandaloneWeekdays, count);
            fStandaloneWeekdaysCount = count;
            break;
        case ABBREVIATED :
            if (fStandaloneShortWeekdays)
                delete[] fStandaloneShortWeekdays;
            fStandaloneShortWeekdays = newUnicodeStringArray(count);
            uprv_arrayCopy(weekdaysArray, fStandaloneShortWeekdays, count);
            fStandaloneShortWeekdaysCount = count;
            break;
        case NARROW :
            if (fStandaloneNarrowWeekdays)
                delete[] fStandaloneNarrowWeekdays;
            fStandaloneNarrowWeekdays = newUnicodeStringArray(count);
            uprv_arrayCopy(weekdaysArray, fStandaloneNarrowWeekdays, count);
            fStandaloneNarrowWeekdaysCount = count;
            break; 
        case DT_WIDTH_COUNT :
            break;
        }
        break;
    case DT_CONTEXT_COUNT :
        break;
    }
}

void
DateFormatSymbols::setQuarters(const UnicodeString* quartersArray, int32_t count, DtContextType context, DtWidthType width)
{
    // delete the old list if we own it
    // we always own the new list, which we create here (we duplicate rather
    // than adopting the list passed in)

    switch (context) {
    case FORMAT :
        switch (width) {
        case WIDE :
            if (fQuarters)
                delete[] fQuarters;
            fQuarters = newUnicodeStringArray(count);
            uprv_arrayCopy( quartersArray,fQuarters,count);
            fQuartersCount = count;
            break;
        case ABBREVIATED :
            if (fShortQuarters)
                delete[] fShortQuarters;
            fShortQuarters = newUnicodeStringArray(count);
            uprv_arrayCopy( quartersArray,fShortQuarters,count);
            fShortQuartersCount = count;
            break;
        case NARROW :
        /*
            if (fNarrowQuarters)
                delete[] fNarrowQuarters;
            fNarrowQuarters = newUnicodeStringArray(count);
            uprv_arrayCopy( quartersArray,fNarrowQuarters,count);
            fNarrowQuartersCount = count;
        */
            break; 
        case DT_WIDTH_COUNT :
            break;
        }
        break;
    case STANDALONE :
        switch (width) {
        case WIDE :
            if (fStandaloneQuarters)
                delete[] fStandaloneQuarters;
            fStandaloneQuarters = newUnicodeStringArray(count);
            uprv_arrayCopy( quartersArray,fStandaloneQuarters,count);
            fStandaloneQuartersCount = count;
            break;
        case ABBREVIATED :
            if (fStandaloneShortQuarters)
                delete[] fStandaloneShortQuarters;
            fStandaloneShortQuarters = newUnicodeStringArray(count);
            uprv_arrayCopy( quartersArray,fStandaloneShortQuarters,count);
            fStandaloneShortQuartersCount = count;
            break;
        case NARROW :
        /*
           if (fStandaloneNarrowQuarters)
                delete[] fStandaloneNarrowQuarters;
            fStandaloneNarrowQuarters = newUnicodeStringArray(count);
            uprv_arrayCopy( quartersArray,fStandaloneNarrowQuarters,count);
            fStandaloneNarrowQuartersCount = count;
        */
            break; 
        case DT_WIDTH_COUNT :
            break;
        }
        break;
    case DT_CONTEXT_COUNT :
        break;
    }
}

void
DateFormatSymbols::setAmPmStrings(const UnicodeString* amPmsArray, int32_t count)
{
    // delete the old list if we own it
    if (fAmPms) delete[] fAmPms;

    // we always own the new list, which we create here (we duplicate rather
    // than adopting the list passed in)
    fAmPms = newUnicodeStringArray(count);
    uprv_arrayCopy(amPmsArray,fAmPms,count);
    fAmPmsCount = count;
}

//------------------------------------------------------
const ZoneStringFormat*
DateFormatSymbols::getZoneStringFormat(void) const {
    umtx_lock(&LOCK);
    if (fZoneStringFormat == NULL) {
        ((DateFormatSymbols*)this)->initZoneStringFormat();
    }
    umtx_unlock(&LOCK);
    return fZoneStringFormat;
}

void
DateFormatSymbols::initZoneStringFormat(void) {
    if (fZoneStringFormat == NULL) {
        UErrorCode status = U_ZERO_ERROR;
        if (fZoneStrings) {
            // Create an istance of ZoneStringFormat by the custom zone strings array
            fZSFLocal = new ZoneStringFormat(fZoneStrings, fZoneStringsRowCount,
                fZoneStringsColCount, status);
            if (U_FAILURE(status)) {
                delete fZSFLocal;
            } else {
                fZoneStringFormat = (const ZoneStringFormat*)fZSFLocal;
            }
        } else {
            fZSFCachePtr = ZoneStringFormat::getZoneStringFormat(fZSFLocale, status);
            if (U_FAILURE(status)) {
                delete fZSFCachePtr;
            } else {
                fZoneStringFormat = fZSFCachePtr->get();
            }
        }
    }
}

const UnicodeString**
DateFormatSymbols::getZoneStrings(int32_t& rowCount, int32_t& columnCount) const
{
    const UnicodeString **result = NULL;

    umtx_lock(&LOCK);
    if (fZoneStrings == NULL) {
        if (fLocaleZoneStrings == NULL) {
            ((DateFormatSymbols*)this)->initZoneStringsArray();
        }
        result = (const UnicodeString**)fLocaleZoneStrings;
    } else {
        result = (const UnicodeString**)fZoneStrings;
    }
    rowCount = fZoneStringsRowCount;
    columnCount = fZoneStringsColCount;
    umtx_unlock(&LOCK);

    return result;
}

void
DateFormatSymbols::initZoneStringsArray(void) {
    if (fZoneStrings == NULL && fLocaleZoneStrings == NULL) {
        if (fZoneStringFormat == NULL) {
            initZoneStringFormat();
        }
        if (fZoneStringFormat) {
            UErrorCode status = U_ZERO_ERROR;
            fLocaleZoneStrings = fZoneStringFormat->createZoneStringsArray(uprv_getUTCtime() /* use current time */,
                fZoneStringsRowCount, fZoneStringsColCount, status);
        }
    }
}

void
DateFormatSymbols::setZoneStrings(const UnicodeString* const *strings, int32_t rowCount, int32_t columnCount)
{
    // since deleting a 2-d array is a pain in the butt, we offload that task to
    // a separate function
    disposeZoneStrings();
    // we always own the new list, which we create here (we duplicate rather
    // than adopting the list passed in)
    fZoneStringsRowCount = rowCount;
    fZoneStringsColCount = columnCount;
    createZoneStrings((const UnicodeString**)strings);
}

//------------------------------------------------------

const UChar * U_EXPORT2
DateFormatSymbols::getPatternUChars(void)
{
    return gPatternChars;
}

//------------------------------------------------------

UnicodeString&
DateFormatSymbols::getLocalPatternChars(UnicodeString& result) const
{
    // fastCopyFrom() - see assignArray comments
    return result.fastCopyFrom(fLocalPatternChars);
}

//------------------------------------------------------

void
DateFormatSymbols::setLocalPatternChars(const UnicodeString& newLocalPatternChars)
{
    fLocalPatternChars = newLocalPatternChars;
}

//------------------------------------------------------

static void
initField(UnicodeString **field, int32_t& length, const UResourceBundle *data, UErrorCode &status) {
    if (U_SUCCESS(status)) {
        int32_t strLen = 0;
        length = ures_getSize(data);
        *field = newUnicodeStringArray(length);
        if (*field) {
            for(int32_t i = 0; i<length; i++) {
                const UChar *resStr = ures_getStringByIndex(data, i, &strLen, &status);
                // setTo() - see assignArray comments
                (*(field)+i)->setTo(TRUE, resStr, strLen);
            }
        }
        else {
            length = 0;
            status = U_MEMORY_ALLOCATION_ERROR;
        }
    }
}

static void
initField(UnicodeString **field, int32_t& length, const UChar *data, LastResortSize numStr, LastResortSize strLen, UErrorCode &status) {
    if (U_SUCCESS(status)) {
        length = numStr;
        *field = newUnicodeStringArray((size_t)numStr);
        if (*field) {
            for(int32_t i = 0; i<length; i++) {
                // readonly aliases - all "data" strings are constant
                // -1 as length for variable-length strings (gLastResortDayNames[0] is empty)
                (*(field)+i)->setTo(TRUE, data+(i*((int32_t)strLen)), -1);
            }
        }
        else {
            length = 0;
            status = U_MEMORY_ALLOCATION_ERROR;
        }
    }
}

void
DateFormatSymbols::initializeData(const Locale& locale, const char *type, UErrorCode& status, UBool useLastResortData)
{
    int32_t i;
    int32_t len = 0;
    const UChar *resStr;
    /* In case something goes wrong, initialize all of the data to NULL. */
    fEras = NULL;
    fErasCount = 0;
    fEraNames = NULL;
    fEraNamesCount = 0;
    fNarrowEras = NULL;
    fNarrowErasCount = 0;
    fMonths = NULL;
    fMonthsCount=0;
    fShortMonths = NULL;
    fShortMonthsCount=0;
    fNarrowMonths = NULL;
    fNarrowMonthsCount=0;
    fStandaloneMonths = NULL;
    fStandaloneMonthsCount=0;
    fStandaloneShortMonths = NULL;
    fStandaloneShortMonthsCount=0;
    fStandaloneNarrowMonths = NULL;
    fStandaloneNarrowMonthsCount=0;
    fWeekdays = NULL;
    fWeekdaysCount=0;
    fShortWeekdays = NULL;
    fShortWeekdaysCount=0;
    fNarrowWeekdays = NULL;
    fNarrowWeekdaysCount=0;
    fStandaloneWeekdays = NULL;
    fStandaloneWeekdaysCount=0;
    fStandaloneShortWeekdays = NULL;
    fStandaloneShortWeekdaysCount=0;
    fStandaloneNarrowWeekdays = NULL;
    fStandaloneNarrowWeekdaysCount=0;
    fAmPms = NULL;
    fAmPmsCount=0;
    fQuarters = NULL;
    fQuartersCount = 0;
    fShortQuarters = NULL;
    fShortQuartersCount = 0;
    fStandaloneQuarters = NULL;
    fStandaloneQuartersCount = 0;
    fStandaloneShortQuarters = NULL;
    fStandaloneShortQuartersCount = 0;
    fGmtHourFormats = NULL;
    fGmtHourFormatsCount = 0;
    fZoneStringsRowCount = 0;
    fZoneStringsColCount = 0;
    fZoneStrings = NULL;
    fLocaleZoneStrings = NULL;

    fZoneStringFormat = NULL;
    fZSFLocal = NULL;
    fZSFCachePtr = NULL;

    // We need to preserve the requested locale for
    // lazy ZoneStringFormat instantiation.  ZoneStringFormat
    // is region sensitive, thus, bundle locale bundle's locale
    // is not sufficient.
    fZSFLocale = locale;
      
    if (U_FAILURE(status)) return;

    /**
     * Retrieve the string arrays we need from the resource bundle file.
     * We cast away const here, but that's okay; we won't delete any of
     * these.
     */
    CalendarData calData(locale, type, status);

    /**
     * Use the localeBundle for getting zone GMT formatting patterns
     */
    UResourceBundle *zoneBundle = ures_open(U_ICUDATA_ZONE, locale.getName(), &status);
    UResourceBundle *zoneStringsArray = ures_getByKeyWithFallback(zoneBundle, gZoneStringsTag, NULL, &status);

    // load the first data item
    UResourceBundle *erasMain = calData.getByKey(gErasTag, status);
    UResourceBundle *eras = ures_getByKeyWithFallback(erasMain, gNamesAbbrTag, NULL, &status);
    UErrorCode oldStatus = status;
    UResourceBundle *eraNames = ures_getByKeyWithFallback(erasMain, gNamesWideTag, NULL, &status);
    if ( status == U_MISSING_RESOURCE_ERROR ) { // Workaround because eras/wide was omitted from CLDR 1.3
       status = oldStatus;
       eraNames = ures_getByKeyWithFallback(erasMain, gNamesAbbrTag, NULL, &status);
    }
	// current ICU4J falls back to abbreviated if narrow eras are missing, so we will too
    oldStatus = status;
    UResourceBundle *narrowEras = ures_getByKeyWithFallback(erasMain, gNamesNarrowTag, NULL, &status);
    if ( status == U_MISSING_RESOURCE_ERROR ) {
       status = oldStatus;
       narrowEras = ures_getByKeyWithFallback(erasMain, gNamesAbbrTag, NULL, &status);
    }

    UResourceBundle *lsweekdaysData = NULL; // Data closed by calData
    UResourceBundle *weekdaysData = NULL; // Data closed by calData
    UResourceBundle *narrowWeekdaysData = NULL; // Data closed by calData
    UResourceBundle *standaloneWeekdaysData = NULL; // Data closed by calData
    UResourceBundle *standaloneShortWeekdaysData = NULL; // Data closed by calData
    UResourceBundle *standaloneNarrowWeekdaysData = NULL; // Data closed by calData

    U_LOCALE_BASED(locBased, *this);
    if (U_FAILURE(status))
    {
        if (useLastResortData)
        {
            // Handle the case in which there is no resource data present.
            // We don't have to generate usable patterns in this situation;
            // we just need to produce something that will be semi-intelligible
            // in most locales.

            status = U_USING_FALLBACK_WARNING;

            initField(&fEras, fErasCount, (const UChar *)gLastResortEras, kEraNum, kEraLen, status);
            initField(&fEraNames, fEraNamesCount, (const UChar *)gLastResortEras, kEraNum, kEraLen, status);
            initField(&fNarrowEras, fNarrowErasCount, (const UChar *)gLastResortEras, kEraNum, kEraLen, status);
            initField(&fMonths, fMonthsCount, (const UChar *)gLastResortMonthNames, kMonthNum, kMonthLen,  status);
            initField(&fShortMonths, fShortMonthsCount, (const UChar *)gLastResortMonthNames, kMonthNum, kMonthLen, status);
            initField(&fNarrowMonths, fNarrowMonthsCount, (const UChar *)gLastResortMonthNames, kMonthNum, kMonthLen, status);
            initField(&fStandaloneMonths, fStandaloneMonthsCount, (const UChar *)gLastResortMonthNames, kMonthNum, kMonthLen,  status);
            initField(&fStandaloneShortMonths, fStandaloneShortMonthsCount, (const UChar *)gLastResortMonthNames, kMonthNum, kMonthLen, status);
            initField(&fStandaloneNarrowMonths, fStandaloneNarrowMonthsCount, (const UChar *)gLastResortMonthNames, kMonthNum, kMonthLen, status);
            initField(&fWeekdays, fWeekdaysCount, (const UChar *)gLastResortDayNames, kDayNum, kDayLen, status);
            initField(&fShortWeekdays, fShortWeekdaysCount, (const UChar *)gLastResortDayNames, kDayNum, kDayLen, status);
            initField(&fNarrowWeekdays, fNarrowWeekdaysCount, (const UChar *)gLastResortDayNames, kDayNum, kDayLen, status);
            initField(&fStandaloneWeekdays, fStandaloneWeekdaysCount, (const UChar *)gLastResortDayNames, kDayNum, kDayLen, status);
            initField(&fStandaloneShortWeekdays, fStandaloneShortWeekdaysCount, (const UChar *)gLastResortDayNames, kDayNum, kDayLen, status);
            initField(&fStandaloneNarrowWeekdays, fStandaloneNarrowWeekdaysCount, (const UChar *)gLastResortDayNames, kDayNum, kDayLen, status);
            initField(&fAmPms, fAmPmsCount, (const UChar *)gLastResortAmPmMarkers, kAmPmNum, kAmPmLen, status);
            initField(&fQuarters, fQuartersCount, (const UChar *)gLastResortQuarters, kQuarterNum, kQuarterLen, status);
            initField(&fShortQuarters, fShortQuartersCount, (const UChar *)gLastResortQuarters, kQuarterNum, kQuarterLen, status);
            initField(&fStandaloneQuarters, fStandaloneQuartersCount, (const UChar *)gLastResortQuarters, kQuarterNum, kQuarterLen, status);
            initField(&fStandaloneShortQuarters, fStandaloneShortQuartersCount, (const UChar *)gLastResortQuarters, kQuarterNum, kQuarterLen, status);
            initField(&fGmtHourFormats, fGmtHourFormatsCount, (const UChar *)gLastResortGmtHourFormats, kGmtHourNum, kGmtHourLen, status);
            fGmtFormat.setTo(TRUE, gLastResortGmtFormat, -1);
            fLocalPatternChars.setTo(TRUE, gPatternChars, PATTERN_CHARS_LEN);
        }
        goto cleanup;
    }

    // if we make it to here, the resource data is cool, and we can get everything out
    // of it that we need except for the time-zone and localized-pattern data, which
    // are stored in a separate file
    locBased.setLocaleIDs(ures_getLocaleByType(eras, ULOC_VALID_LOCALE, &status),
                          ures_getLocaleByType(eras, ULOC_ACTUAL_LOCALE, &status));

    initField(&fEras, fErasCount, eras, status);
    initField(&fEraNames, fEraNamesCount, eraNames, status);
    initField(&fNarrowEras, fNarrowErasCount, narrowEras, status);

    initField(&fMonths, fMonthsCount, calData.getByKey2(gMonthNamesTag, gNamesWideTag, status), status);
    initField(&fShortMonths, fShortMonthsCount, calData.getByKey2(gMonthNamesTag, gNamesAbbrTag, status), status);

    initField(&fNarrowMonths, fNarrowMonthsCount, calData.getByKey2(gMonthNamesTag, gNamesNarrowTag, status), status);
    if(status == U_MISSING_RESOURCE_ERROR) {
        status = U_ZERO_ERROR;
        initField(&fNarrowMonths, fNarrowMonthsCount, calData.getByKey3(gMonthNamesTag, gNamesStandaloneTag, gNamesNarrowTag, status), status);
    }
    if ( status == U_MISSING_RESOURCE_ERROR ) { /* If format/narrow not available, use format/abbreviated */
       status = U_ZERO_ERROR;
       initField(&fNarrowMonths, fNarrowMonthsCount, calData.getByKey2(gMonthNamesTag, gNamesAbbrTag, status), status);
    }

    initField(&fStandaloneMonths, fStandaloneMonthsCount, calData.getByKey3(gMonthNamesTag, gNamesStandaloneTag, gNamesWideTag, status), status);
    if ( status == U_MISSING_RESOURCE_ERROR ) { /* If standalone/wide not available, use format/wide */
       status = U_ZERO_ERROR;
       initField(&fStandaloneMonths, fStandaloneMonthsCount, calData.getByKey2(gMonthNamesTag, gNamesWideTag, status), status);
    }
    initField(&fStandaloneShortMonths, fStandaloneShortMonthsCount, calData.getByKey3(gMonthNamesTag, gNamesStandaloneTag, gNamesAbbrTag, status), status);
    if ( status == U_MISSING_RESOURCE_ERROR ) { /* If standalone/abbreviated not available, use format/abbreviated */
       status = U_ZERO_ERROR;
       initField(&fStandaloneShortMonths, fStandaloneShortMonthsCount, calData.getByKey2(gMonthNamesTag, gNamesAbbrTag, status), status);
    }
    initField(&fStandaloneNarrowMonths, fStandaloneNarrowMonthsCount, calData.getByKey3(gMonthNamesTag, gNamesStandaloneTag, gNamesNarrowTag, status), status);
    if ( status == U_MISSING_RESOURCE_ERROR ) { /* if standalone/narrow not availabe, try format/narrow */
       status = U_ZERO_ERROR;
       initField(&fStandaloneNarrowMonths, fStandaloneNarrowMonthsCount, calData.getByKey2(gMonthNamesTag, gNamesNarrowTag, status), status);
       if ( status == U_MISSING_RESOURCE_ERROR ) { /* if still not there, use format/abbreviated */
          status = U_ZERO_ERROR;
          initField(&fStandaloneNarrowMonths, fStandaloneNarrowMonthsCount, calData.getByKey2(gMonthNamesTag, gNamesAbbrTag, status), status);
       }
    }
    initField(&fAmPms, fAmPmsCount, calData.getByKey(gAmPmMarkersTag, status), status);

    initField(&fQuarters, fQuartersCount, calData.getByKey2(gQuartersTag, gNamesWideTag, status), status);
    initField(&fShortQuarters, fShortQuartersCount, calData.getByKey2(gQuartersTag, gNamesAbbrTag, status), status);

    initField(&fStandaloneQuarters, fStandaloneQuartersCount, calData.getByKey3(gQuartersTag, gNamesStandaloneTag, gNamesWideTag, status), status);
    if(status == U_MISSING_RESOURCE_ERROR) {
        status = U_ZERO_ERROR;
        initField(&fStandaloneQuarters, fStandaloneQuartersCount, calData.getByKey2(gQuartersTag, gNamesWideTag, status), status);
    }

    initField(&fStandaloneShortQuarters, fStandaloneShortQuartersCount, calData.getByKey3(gQuartersTag, gNamesStandaloneTag, gNamesAbbrTag, status), status);
    if(status == U_MISSING_RESOURCE_ERROR) {
        status = U_ZERO_ERROR;
        initField(&fStandaloneShortQuarters, fStandaloneShortQuartersCount, calData.getByKey2(gQuartersTag, gNamesAbbrTag, status), status);
    }

    // GMT format patterns
    resStr = ures_getStringByKeyWithFallback(zoneStringsArray, gGmtFormatTag, &len, &status);
    if (len > 0) {
        fGmtFormat.setTo(TRUE, resStr, len);
    }

    resStr = ures_getStringByKeyWithFallback(zoneStringsArray, gHourFormatTag, &len, &status);
    if (len > 0) {
        UChar *sep = u_strchr(resStr, (UChar)0x003B /* ';' */);
        if (sep != NULL) {
            fGmtHourFormats = newUnicodeStringArray(GMT_HOUR_COUNT);
            if (fGmtHourFormats == NULL) {
                status = U_MEMORY_ALLOCATION_ERROR;
            } else {
                fGmtHourFormatsCount = GMT_HOUR_COUNT;
                fGmtHourFormats[GMT_NEGATIVE_HM].setTo(TRUE, sep + 1, -1);
                fGmtHourFormats[GMT_POSITIVE_HM].setTo(FALSE, resStr, (int32_t)(sep - resStr));

                // CLDR 1.5 does not have GMT offset pattern including second field.
                // For now, append "ss" to the end.
                if (fGmtHourFormats[GMT_NEGATIVE_HM].indexOf((UChar)0x003A /* ':' */) != -1) {
                    fGmtHourFormats[GMT_NEGATIVE_HMS] = fGmtHourFormats[GMT_NEGATIVE_HM] + UNICODE_STRING_SIMPLE(":ss");
                } else if (fGmtHourFormats[GMT_NEGATIVE_HM].indexOf((UChar)0x002E /* '.' */) != -1) {
                    fGmtHourFormats[GMT_NEGATIVE_HMS] = fGmtHourFormats[GMT_NEGATIVE_HM] + UNICODE_STRING_SIMPLE(".ss");
                } else {
                    fGmtHourFormats[GMT_NEGATIVE_HMS] = fGmtHourFormats[GMT_NEGATIVE_HM] + UNICODE_STRING_SIMPLE("ss");
                }
                if (fGmtHourFormats[GMT_POSITIVE_HM].indexOf((UChar)0x003A /* ':' */) != -1) {
                    fGmtHourFormats[GMT_POSITIVE_HMS] = fGmtHourFormats[GMT_POSITIVE_HM] + UNICODE_STRING_SIMPLE(":ss");
                } else if (fGmtHourFormats[GMT_POSITIVE_HM].indexOf((UChar)0x002E /* '.' */) != -1) {
                    fGmtHourFormats[GMT_POSITIVE_HMS] = fGmtHourFormats[GMT_POSITIVE_HM] + UNICODE_STRING_SIMPLE(".ss");
                } else {
                    fGmtHourFormats[GMT_POSITIVE_HMS] = fGmtHourFormats[GMT_POSITIVE_HM] + UNICODE_STRING_SIMPLE("ss");
                }
            }
        }
    }

    // ICU 3.8 or later version no longer uses localized date-time pattern characters by default (ticket#5597)
    /*
    // fastCopyFrom()/setTo() - see assignArray comments
    resStr = ures_getStringByKey(fResourceBundle, gLocalPatternCharsTag, &len, &status);
    fLocalPatternChars.setTo(TRUE, resStr, len);
    // If the locale data does not include new pattern chars, use the defaults
    // TODO: Consider making this an error, since this may add conflicting characters.
    if (len < PATTERN_CHARS_LEN) {
        fLocalPatternChars.append(UnicodeString(TRUE, &gPatternChars[len], PATTERN_CHARS_LEN-len));
    }
    */
    fLocalPatternChars.setTo(TRUE, gPatternChars, PATTERN_CHARS_LEN);

    // {sfb} fixed to handle 1-based weekdays
    weekdaysData = calData.getByKey2(gDayNamesTag, gNamesWideTag, status);
    fWeekdaysCount = ures_getSize(weekdaysData);
    fWeekdays = new UnicodeString[fWeekdaysCount+1];
    /* pin the blame on system. If we cannot get a chunk of memory .. the system is dying!*/
    if (fWeekdays == NULL) {
        status = U_MEMORY_ALLOCATION_ERROR;
        goto cleanup;
    }
    // leave fWeekdays[0] empty
    for(i = 0; i<fWeekdaysCount; i++) {
        resStr = ures_getStringByIndex(weekdaysData, i, &len, &status);
        // setTo() - see assignArray comments
        fWeekdays[i+1].setTo(TRUE, resStr, len);
    }
    fWeekdaysCount++;

    lsweekdaysData = calData.getByKey2(gDayNamesTag, gNamesAbbrTag, status);
    fShortWeekdaysCount = ures_getSize(lsweekdaysData);
    fShortWeekdays = new UnicodeString[fShortWeekdaysCount+1];
    /* test for NULL */
    if (fShortWeekdays == 0) {
        status = U_MEMORY_ALLOCATION_ERROR;
        goto cleanup;
    }
    // leave fShortWeekdays[0] empty
    for(i = 0; i<fShortWeekdaysCount; i++) {
        resStr = ures_getStringByIndex(lsweekdaysData, i, &len, &status);
        // setTo() - see assignArray comments
        fShortWeekdays[i+1].setTo(TRUE, resStr, len);
    }
    fShortWeekdaysCount++;

    narrowWeekdaysData = calData.getByKey2(gDayNamesTag, gNamesNarrowTag, status);
    if(status == U_MISSING_RESOURCE_ERROR) {
        status = U_ZERO_ERROR;
        narrowWeekdaysData = calData.getByKey3(gDayNamesTag, gNamesStandaloneTag, gNamesNarrowTag, status);
    }
    if ( status == U_MISSING_RESOURCE_ERROR ) {
       status = U_ZERO_ERROR;
       narrowWeekdaysData = calData.getByKey2(gDayNamesTag, gNamesAbbrTag, status);
    }
    fNarrowWeekdaysCount = ures_getSize(narrowWeekdaysData);
    fNarrowWeekdays = new UnicodeString[fNarrowWeekdaysCount+1];
    /* test for NULL */
    if (fNarrowWeekdays == 0) {
        status = U_MEMORY_ALLOCATION_ERROR;
        goto cleanup;
    }
    // leave fNarrowWeekdays[0] empty
    for(i = 0; i<fNarrowWeekdaysCount; i++) {
        resStr = ures_getStringByIndex(narrowWeekdaysData, i, &len, &status);
        // setTo() - see assignArray comments
        fNarrowWeekdays[i+1].setTo(TRUE, resStr, len);
    }
    fNarrowWeekdaysCount++;

    standaloneWeekdaysData = calData.getByKey3(gDayNamesTag, gNamesStandaloneTag, gNamesWideTag, status);
    if ( status == U_MISSING_RESOURCE_ERROR ) {
       status = U_ZERO_ERROR;
       standaloneWeekdaysData = calData.getByKey2(gDayNamesTag, gNamesWideTag, status);
    }
    fStandaloneWeekdaysCount = ures_getSize(standaloneWeekdaysData);
    fStandaloneWeekdays = new UnicodeString[fStandaloneWeekdaysCount+1];
    /* test for NULL */
    if (fStandaloneWeekdays == 0) {
        status = U_MEMORY_ALLOCATION_ERROR;
        goto cleanup;
    }
    // leave fStandaloneWeekdays[0] empty
    for(i = 0; i<fStandaloneWeekdaysCount; i++) {
        resStr = ures_getStringByIndex(standaloneWeekdaysData, i, &len, &status);
        // setTo() - see assignArray comments
        fStandaloneWeekdays[i+1].setTo(TRUE, resStr, len);
    }
    fStandaloneWeekdaysCount++;

    standaloneShortWeekdaysData = calData.getByKey3(gDayNamesTag, gNamesStandaloneTag, gNamesAbbrTag, status);
    if ( status == U_MISSING_RESOURCE_ERROR ) {
       status = U_ZERO_ERROR;
       standaloneShortWeekdaysData = calData.getByKey2(gDayNamesTag, gNamesAbbrTag, status);
    }
    fStandaloneShortWeekdaysCount = ures_getSize(standaloneShortWeekdaysData);
    fStandaloneShortWeekdays = new UnicodeString[fStandaloneShortWeekdaysCount+1];
    /* test for NULL */
    if (fStandaloneShortWeekdays == 0) {
        status = U_MEMORY_ALLOCATION_ERROR;
        goto cleanup;
    }
    // leave fStandaloneShortWeekdays[0] empty
    for(i = 0; i<fStandaloneShortWeekdaysCount; i++) {
        resStr = ures_getStringByIndex(standaloneShortWeekdaysData, i, &len, &status);
        // setTo() - see assignArray comments
        fStandaloneShortWeekdays[i+1].setTo(TRUE, resStr, len);
    }
    fStandaloneShortWeekdaysCount++;

    standaloneNarrowWeekdaysData = calData.getByKey3(gDayNamesTag, gNamesStandaloneTag, gNamesNarrowTag, status);
    if ( status == U_MISSING_RESOURCE_ERROR ) {
       status = U_ZERO_ERROR;
       standaloneNarrowWeekdaysData = calData.getByKey2(gDayNamesTag, gNamesNarrowTag, status);
       if ( status == U_MISSING_RESOURCE_ERROR ) {
          status = U_ZERO_ERROR;
          standaloneNarrowWeekdaysData = calData.getByKey2(gDayNamesTag, gNamesAbbrTag, status);
       }
    }
    fStandaloneNarrowWeekdaysCount = ures_getSize(standaloneNarrowWeekdaysData);
    fStandaloneNarrowWeekdays = new UnicodeString[fStandaloneNarrowWeekdaysCount+1];
    /* test for NULL */
    if (fStandaloneNarrowWeekdays == 0) {
        status = U_MEMORY_ALLOCATION_ERROR;
        goto cleanup;
    }
    // leave fStandaloneNarrowWeekdays[0] empty
    for(i = 0; i<fStandaloneNarrowWeekdaysCount; i++) {
        resStr = ures_getStringByIndex(standaloneNarrowWeekdaysData, i, &len, &status);
        // setTo() - see assignArray comments
        fStandaloneNarrowWeekdays[i+1].setTo(TRUE, resStr, len);
    }
    fStandaloneNarrowWeekdaysCount++;

cleanup:
    ures_close(eras);
    ures_close(eraNames);
    ures_close(narrowEras);
    ures_close(zoneStringsArray);
    ures_close(zoneBundle);
}

Locale 
DateFormatSymbols::getLocale(ULocDataLocaleType type, UErrorCode& status) const {
    U_LOCALE_BASED(locBased, *this);
    return locBased.getLocale(type, status);
}

U_NAMESPACE_END

#endif /* #if !UCONFIG_NO_FORMATTING */

//eof
