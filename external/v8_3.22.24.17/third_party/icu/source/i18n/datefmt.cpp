/*
 *******************************************************************************
 * Copyright (C) 1997-2010, International Business Machines Corporation and    *
 * others. All Rights Reserved.                                                *
 *******************************************************************************
 *
 * File DATEFMT.CPP
 *
 * Modification History:
 *
 *   Date        Name        Description
 *   02/19/97    aliu        Converted from java.
 *   03/31/97    aliu        Modified extensively to work with 50 locales.
 *   04/01/97    aliu        Added support for centuries.
 *   08/12/97    aliu        Fixed operator== to use Calendar::equivalentTo.
 *   07/20/98    stephen     Changed ParsePosition initialization
 ********************************************************************************
 */

#include "unicode/utypes.h"

#if !UCONFIG_NO_FORMATTING

#include "unicode/ures.h"
#include "unicode/datefmt.h"
#include "unicode/smpdtfmt.h"
#include "unicode/dtptngen.h"
#include "reldtfmt.h"

#include "cstring.h"
#include "windtfmt.h"

#if defined( U_DEBUG_CALSVC ) || defined (U_DEBUG_CAL)
#include <stdio.h>
#endif

// *****************************************************************************
// class DateFormat
// *****************************************************************************

U_NAMESPACE_BEGIN

DateFormat::DateFormat()
:   fCalendar(0),
    fNumberFormat(0)
{
}

//----------------------------------------------------------------------

DateFormat::DateFormat(const DateFormat& other)
:   Format(other),
    fCalendar(0),
    fNumberFormat(0)
{
    *this = other;
}

//----------------------------------------------------------------------

DateFormat& DateFormat::operator=(const DateFormat& other)
{
    if (this != &other)
    {
        delete fCalendar;
        delete fNumberFormat;
        if(other.fCalendar) {
          fCalendar = other.fCalendar->clone();
        } else {
          fCalendar = NULL;
        }
        if(other.fNumberFormat) {
          fNumberFormat = (NumberFormat*)other.fNumberFormat->clone();
        } else {
          fNumberFormat = NULL;
        }
    }
    return *this;
}

//----------------------------------------------------------------------

DateFormat::~DateFormat()
{
    delete fCalendar;
    delete fNumberFormat;
}

//----------------------------------------------------------------------

UBool
DateFormat::operator==(const Format& other) const
{
    // This protected comparison operator should only be called by subclasses
    // which have confirmed that the other object being compared against is
    // an instance of a sublcass of DateFormat.  THIS IS IMPORTANT.

    // Format::operator== guarantees that this cast is safe
    DateFormat* fmt = (DateFormat*)&other;

    return (this == fmt) ||
        (Format::operator==(other) &&
         fCalendar&&(fCalendar->isEquivalentTo(*fmt->fCalendar)) &&
         (fNumberFormat && *fNumberFormat == *fmt->fNumberFormat));
}

//----------------------------------------------------------------------

UnicodeString&
DateFormat::format(const Formattable& obj,
                   UnicodeString& appendTo,
                   FieldPosition& fieldPosition,
                   UErrorCode& status) const
{
    if (U_FAILURE(status)) return appendTo;

    // if the type of the Formattable is double or long, treat it as if it were a Date
    UDate date = 0;
    switch (obj.getType())
    {
    case Formattable::kDate:
        date = obj.getDate();
        break;
    case Formattable::kDouble:
        date = (UDate)obj.getDouble();
        break;
    case Formattable::kLong:
        date = (UDate)obj.getLong();
        break;
    default:
        status = U_ILLEGAL_ARGUMENT_ERROR;
        return appendTo;
    }

    // Is this right?
    //if (fieldPosition.getBeginIndex() == fieldPosition.getEndIndex())
    //  status = U_ILLEGAL_ARGUMENT_ERROR;

    return format(date, appendTo, fieldPosition);
}

//----------------------------------------------------------------------

UnicodeString&
DateFormat::format(const Formattable& obj,
                   UnicodeString& appendTo,
                   FieldPositionIterator* posIter,
                   UErrorCode& status) const
{
    if (U_FAILURE(status)) return appendTo;

    // if the type of the Formattable is double or long, treat it as if it were a Date
    UDate date = 0;
    switch (obj.getType())
    {
    case Formattable::kDate:
        date = obj.getDate();
        break;
    case Formattable::kDouble:
        date = (UDate)obj.getDouble();
        break;
    case Formattable::kLong:
        date = (UDate)obj.getLong();
        break;
    default:
        status = U_ILLEGAL_ARGUMENT_ERROR;
        return appendTo;
    }

    // Is this right?
    //if (fieldPosition.getBeginIndex() == fieldPosition.getEndIndex())
    //  status = U_ILLEGAL_ARGUMENT_ERROR;

    return format(date, appendTo, posIter, status);
}

//----------------------------------------------------------------------

// Default implementation for backwards compatibility, subclasses should implement.
UnicodeString&
DateFormat::format(Calendar& /* unused cal */,
                   UnicodeString& appendTo,
                   FieldPositionIterator* /* unused posIter */,
                   UErrorCode& status) const {
    if (U_SUCCESS(status)) {
        status = U_UNSUPPORTED_ERROR;
    }
    return appendTo;
}

//----------------------------------------------------------------------

UnicodeString&
DateFormat::format(UDate date, UnicodeString& appendTo, FieldPosition& fieldPosition) const {
    if (fCalendar != NULL) {
        // Use our calendar instance
        UErrorCode ec = U_ZERO_ERROR;
        fCalendar->setTime(date, ec);
        if (U_SUCCESS(ec)) {
            return format(*fCalendar, appendTo, fieldPosition);
        }
    }
    return appendTo;
}

//----------------------------------------------------------------------

UnicodeString&
DateFormat::format(UDate date, UnicodeString& appendTo, FieldPositionIterator* posIter,
                   UErrorCode& status) const {
    if (fCalendar != NULL) {
        fCalendar->setTime(date, status);
        if (U_SUCCESS(status)) {
            return format(*fCalendar, appendTo, posIter, status);
        }
    }
    return appendTo;
}

//----------------------------------------------------------------------

UnicodeString&
DateFormat::format(UDate date, UnicodeString& appendTo) const
{
    // Note that any error information is just lost.  That's okay
    // for this convenience method.
    FieldPosition fpos(0);
    return format(date, appendTo, fpos);
}

//----------------------------------------------------------------------

UDate
DateFormat::parse(const UnicodeString& text,
                  ParsePosition& pos) const
{
    UDate d = 0; // Error return UDate is 0 (the epoch)
    if (fCalendar != NULL) {
        int32_t start = pos.getIndex();

        // Parse may update TimeZone used by the calendar.
        TimeZone *tzsav = (TimeZone*)fCalendar->getTimeZone().clone();

        fCalendar->clear();
        parse(text, *fCalendar, pos);
        if (pos.getIndex() != start) {
            UErrorCode ec = U_ZERO_ERROR;
            d = fCalendar->getTime(ec);
            if (U_FAILURE(ec)) {
                // We arrive here if fCalendar is non-lenient and there
                // is an out-of-range field.  We don't know which field
                // was illegal so we set the error index to the start.
                pos.setIndex(start);
                pos.setErrorIndex(start);
                d = 0;
            }
        }

        // Restore TimeZone
        fCalendar->adoptTimeZone(tzsav);
    }
    return d;
}

//----------------------------------------------------------------------

UDate
DateFormat::parse(const UnicodeString& text,
                  UErrorCode& status) const
{
    if (U_FAILURE(status)) return 0;

    ParsePosition pos(0);
    UDate result = parse(text, pos);
    if (pos.getIndex() == 0) {
#if defined (U_DEBUG_CAL)
      fprintf(stderr, "%s:%d - - failed to parse  - err index %d\n"
              , __FILE__, __LINE__, pos.getErrorIndex() );
#endif
      status = U_ILLEGAL_ARGUMENT_ERROR;
    }
    return result;
}

//----------------------------------------------------------------------

void
DateFormat::parseObject(const UnicodeString& source,
                        Formattable& result,
                        ParsePosition& pos) const
{
    result.setDate(parse(source, pos));
}

//----------------------------------------------------------------------

DateFormat* U_EXPORT2
DateFormat::createTimeInstance(DateFormat::EStyle style,
                               const Locale& aLocale)
{
    return create(style, kNone, aLocale);
}

//----------------------------------------------------------------------

DateFormat* U_EXPORT2
DateFormat::createDateInstance(DateFormat::EStyle style,
                               const Locale& aLocale)
{
    // +4 to set the correct index for getting data out of
    // LocaleElements.
    if(style != kNone)
    {
        style = (EStyle) (style + kDateOffset);
    }
    return create(kNone, (EStyle) (style), aLocale);
}

//----------------------------------------------------------------------

DateFormat* U_EXPORT2
DateFormat::createDateTimeInstance(EStyle dateStyle,
                                   EStyle timeStyle,
                                   const Locale& aLocale)
{
    if(dateStyle != kNone)
    {
        dateStyle = (EStyle) (dateStyle + kDateOffset);
    }
    return create(timeStyle, dateStyle, aLocale);
}

//----------------------------------------------------------------------

DateFormat* U_EXPORT2
DateFormat::createInstance()
{
    return create(kShort, (EStyle) (kShort + kDateOffset), Locale::getDefault());
}

//----------------------------------------------------------------------

DateFormat* U_EXPORT2
DateFormat::create(EStyle timeStyle, EStyle dateStyle, const Locale& locale)
{
    UErrorCode status = U_ZERO_ERROR;
#ifdef U_WINDOWS
    char buffer[8];
    int32_t count = locale.getKeywordValue("compat", buffer, sizeof(buffer), status);

    // if the locale has "@compat=host", create a host-specific DateFormat...
    if (count > 0 && uprv_strcmp(buffer, "host") == 0) {
        Win32DateFormat *f = new Win32DateFormat(timeStyle, dateStyle, locale, status);

        if (U_SUCCESS(status)) {
            return f;
        }

        delete f;
    }
#endif

    // is it relative?
    if(/*((timeStyle!=UDAT_NONE)&&(timeStyle & UDAT_RELATIVE)) || */((dateStyle!=kNone)&&((dateStyle-kDateOffset) & UDAT_RELATIVE))) {
        RelativeDateFormat *r = new RelativeDateFormat((UDateFormatStyle)timeStyle, (UDateFormatStyle)(dateStyle-kDateOffset), locale, status);
        if(U_SUCCESS(status)) return r;
        delete r;
        status = U_ZERO_ERROR;
    }

    // Try to create a SimpleDateFormat of the desired style.
    SimpleDateFormat *f = new SimpleDateFormat(timeStyle, dateStyle, locale, status);
    if (U_SUCCESS(status)) return f;
    delete f;

    // If that fails, try to create a format using the default pattern and
    // the DateFormatSymbols for this locale.
    status = U_ZERO_ERROR;
    f = new SimpleDateFormat(locale, status);
    if (U_SUCCESS(status)) return f;
    delete f;

    // This should never really happen, because the preceding constructor
    // should always succeed.  If the resource data is unavailable, a last
    // resort object should be returned.
    return 0;
}

//----------------------------------------------------------------------

const Locale* U_EXPORT2
DateFormat::getAvailableLocales(int32_t& count)
{
    // Get the list of installed locales.
    // Even if root has the correct date format for this locale,
    // it's still a valid locale (we don't worry about data fallbacks).
    return Locale::getAvailableLocales(count);
}

//----------------------------------------------------------------------

void
DateFormat::adoptCalendar(Calendar* newCalendar)
{
    delete fCalendar;
    fCalendar = newCalendar;
}

//----------------------------------------------------------------------
void
DateFormat::setCalendar(const Calendar& newCalendar)
{
    Calendar* newCalClone = newCalendar.clone();
    if (newCalClone != NULL) {
        adoptCalendar(newCalClone);
    }
}

//----------------------------------------------------------------------

const Calendar*
DateFormat::getCalendar() const
{
    return fCalendar;
}

//----------------------------------------------------------------------

void
DateFormat::adoptNumberFormat(NumberFormat* newNumberFormat)
{
    delete fNumberFormat;
    fNumberFormat = newNumberFormat;
    newNumberFormat->setParseIntegerOnly(TRUE);
}
//----------------------------------------------------------------------

void
DateFormat::setNumberFormat(const NumberFormat& newNumberFormat)
{
    NumberFormat* newNumFmtClone = (NumberFormat*)newNumberFormat.clone();
    if (newNumFmtClone != NULL) {
        adoptNumberFormat(newNumFmtClone);
    }
}

//----------------------------------------------------------------------

const NumberFormat*
DateFormat::getNumberFormat() const
{
    return fNumberFormat;
}

//----------------------------------------------------------------------

void
DateFormat::adoptTimeZone(TimeZone* zone)
{
    if (fCalendar != NULL) {
        fCalendar->adoptTimeZone(zone);
    }
}
//----------------------------------------------------------------------

void
DateFormat::setTimeZone(const TimeZone& zone)
{
    if (fCalendar != NULL) {
        fCalendar->setTimeZone(zone);
    }
}

//----------------------------------------------------------------------

const TimeZone&
DateFormat::getTimeZone() const
{
    if (fCalendar != NULL) {
        return fCalendar->getTimeZone();
    }
    // If calendar doesn't exists, create default timezone.
    // fCalendar is rarely null
    return *(TimeZone::createDefault());
}

//----------------------------------------------------------------------

void
DateFormat::setLenient(UBool lenient)
{
    if (fCalendar != NULL) {
        fCalendar->setLenient(lenient);
    }
}

//----------------------------------------------------------------------

UBool
DateFormat::isLenient() const
{
    if (fCalendar != NULL) {
        return fCalendar->isLenient();
    }
    // fCalendar is rarely null
    return FALSE;
}

U_NAMESPACE_END

#endif /* #if !UCONFIG_NO_FORMATTING */

//eof
