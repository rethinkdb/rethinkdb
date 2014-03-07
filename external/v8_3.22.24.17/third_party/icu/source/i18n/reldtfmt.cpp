/*
*******************************************************************************
* Copyright (C) 2007-2010, International Business Machines Corporation and
* others. All Rights Reserved.
*******************************************************************************
*/

#include "unicode/utypes.h"

#if !UCONFIG_NO_FORMATTING

//#define DEBUG_RELDTFMT

#include <stdio.h>
#include <stdlib.h>

#include "reldtfmt.h"
#include "unicode/msgfmt.h"
#include "unicode/smpdtfmt.h"

#include "gregoimp.h" // for CalendarData
#include "cmemory.h"

U_NAMESPACE_BEGIN


/**
 * An array of URelativeString structs is used to store the resource data loaded out of the bundle.
 */
struct URelativeString {
    int32_t offset;         /** offset of this item, such as, the relative date **/
    int32_t len;            /** length of the string **/
    const UChar* string;    /** string, or NULL if not set **/
};

static const char DT_DateTimePatternsTag[]="DateTimePatterns";


UOBJECT_DEFINE_RTTI_IMPLEMENTATION(RelativeDateFormat)

RelativeDateFormat::RelativeDateFormat(const RelativeDateFormat& other) :
DateFormat(other), fDateFormat(NULL), fTimeFormat(NULL), fCombinedFormat(NULL),
fDateStyle(other.fDateStyle), fTimeStyle(other.fTimeStyle), fLocale(other.fLocale),
fDayMin(other.fDayMin), fDayMax(other.fDayMax),
fDatesLen(other.fDatesLen), fDates(NULL)
{
    if(other.fDateFormat != NULL) {
        fDateFormat = (DateFormat*)other.fDateFormat->clone();
    } else {
        fDateFormat = NULL;
    }
    if (fDatesLen > 0) {
        fDates = (URelativeString*) uprv_malloc(sizeof(fDates[0])*fDatesLen);
        uprv_memcpy(fDates, other.fDates, sizeof(fDates[0])*fDatesLen);
    }
    //fCalendar = other.fCalendar->clone();
/*    
    if(other.fTimeFormat != NULL) {
        fTimeFormat = (DateFormat*)other.fTimeFormat->clone();
    } else {
        fTimeFormat = NULL;
    }
*/
}

RelativeDateFormat::RelativeDateFormat( UDateFormatStyle timeStyle, UDateFormatStyle dateStyle, const Locale& locale, UErrorCode& status)
 : DateFormat(), fDateFormat(NULL), fTimeFormat(NULL), fCombinedFormat(NULL),
fDateStyle(dateStyle), fTimeStyle(timeStyle), fLocale(locale), fDatesLen(0), fDates(NULL)
 {
    if(U_FAILURE(status) ) {
        return;
    }
    
    if(fDateStyle != UDAT_NONE) {
        EStyle newStyle = (EStyle)(fDateStyle & ~UDAT_RELATIVE);
        // Create a DateFormat in the non-relative style requested.
        fDateFormat = createDateInstance(newStyle, locale);
    }
    if(fTimeStyle >= UDAT_FULL && fTimeStyle <= UDAT_SHORT) {
        fTimeFormat = createTimeInstance((EStyle)fTimeStyle, locale);
    } else if(fTimeStyle != UDAT_NONE) {
        // don't support other time styles (e.g. relative styles), for now
        status = U_ILLEGAL_ARGUMENT_ERROR;
        return;
    }
    
    // Initialize the parent fCalendar, so that parse() works correctly.
    initializeCalendar(NULL, locale, status);
    loadDates(status);
}

RelativeDateFormat::~RelativeDateFormat() {
    delete fDateFormat;
    delete fTimeFormat;
    delete fCombinedFormat;
    uprv_free(fDates);
}


Format* RelativeDateFormat::clone(void) const {
    return new RelativeDateFormat(*this);
}

UBool RelativeDateFormat::operator==(const Format& other) const {
    if(DateFormat::operator==(other)) {
        // DateFormat::operator== guarantees following cast is safe
        RelativeDateFormat* that = (RelativeDateFormat*)&other;
        return (fDateStyle==that->fDateStyle   &&
                fTimeStyle==that->fTimeStyle   &&
                fLocale==that->fLocale);
    }
    return FALSE;
}

UnicodeString& RelativeDateFormat::format(  Calendar& cal,
                                UnicodeString& appendTo,
                                FieldPosition& pos) const {
                                
    UErrorCode status = U_ZERO_ERROR;
    UChar emptyStr = 0;
    UnicodeString dateString(&emptyStr);
    
    // calculate the difference, in days, between 'cal' and now.
    int dayDiff = dayDifference(cal, status);

    // look up string
    int32_t len = 0;
    const UChar *theString = getStringForDay(dayDiff, len, status);
    if(U_SUCCESS(status) && (theString!=NULL)) {
        // found a relative string
        dateString.setTo(theString, len);
    }
    
    if(fTimeFormat == NULL || fCombinedFormat == 0) {
        if (dateString.length() > 0) {
            appendTo.append(dateString);
        } else if(fDateFormat != NULL) {
            fDateFormat->format(cal,appendTo,pos);
        }
    } else {
        if (dateString.length() == 0 && fDateFormat != NULL) {
            fDateFormat->format(cal,dateString,pos);
        }
        UnicodeString timeString(&emptyStr);
        FieldPosition timepos = pos;
        fTimeFormat->format(cal,timeString,timepos);
        Formattable timeDateStrings[] = { timeString, dateString };
        fCombinedFormat->format(timeDateStrings, 2, appendTo, pos, status); // pos is ignored by this
        int32_t offset;
        if (pos.getEndIndex() > 0 && (offset = appendTo.indexOf(dateString)) >= 0) {
            // pos.field was found in dateString, offset start & end based on final position of dateString
            pos.setBeginIndex( pos.getBeginIndex() + offset );
            pos.setEndIndex( pos.getEndIndex() + offset );
        } else if (timepos.getEndIndex() > 0 && (offset = appendTo.indexOf(timeString)) >= 0) {
            // pos.field was found in timeString, offset start & end based on final position of timeString
            pos.setBeginIndex( timepos.getBeginIndex() + offset );
            pos.setEndIndex( timepos.getEndIndex() + offset );
        }
    }
    
    return appendTo;
}



UnicodeString&
RelativeDateFormat::format(const Formattable& obj, 
                         UnicodeString& appendTo, 
                         FieldPosition& pos,
                         UErrorCode& status) const
{
    // this is just here to get around the hiding problem
    // (the previous format() override would hide the version of
    // format() on DateFormat that this function correspond to, so we
    // have to redefine it here)
    return DateFormat::format(obj, appendTo, pos, status);
}


void RelativeDateFormat::parse( const UnicodeString& text,
                    Calendar& cal,
                    ParsePosition& pos) const {

    // Can the fDateFormat parse it?
    if(fDateFormat != NULL) {
        ParsePosition aPos(pos);
        fDateFormat->parse(text,cal,aPos);
        if((aPos.getIndex() != pos.getIndex()) && 
            (aPos.getErrorIndex()==-1)) {
                pos=aPos; // copy the sub parse
                return; // parsed subfmt OK
        }
    }
    
    // Linear search the relative strings
    for(int n=0;n<fDatesLen;n++) {
        if(fDates[n].string != NULL &&
            (0==text.compare(pos.getIndex(),
                         fDates[n].len,
                         fDates[n].string))) {
            UErrorCode status = U_ZERO_ERROR;
            
            // Set the calendar to now+offset
            cal.setTime(Calendar::getNow(),status);
            cal.add(UCAL_DATE,fDates[n].offset, status);
            
            if(U_FAILURE(status)) { 
                // failure in setting calendar fields
                pos.setErrorIndex(pos.getIndex()+fDates[n].len);
            } else {
                pos.setIndex(pos.getIndex()+fDates[n].len);
            }
            return;
        }
    }
    
    // parse failed
}

UDate
RelativeDateFormat::parse( const UnicodeString& text,
                         ParsePosition& pos) const {
    // redefined here because the other parse() function hides this function's
    // cunterpart on DateFormat
    return DateFormat::parse(text, pos);
}

UDate
RelativeDateFormat::parse(const UnicodeString& text, UErrorCode& status) const
{
    // redefined here because the other parse() function hides this function's
    // counterpart on DateFormat
    return DateFormat::parse(text, status);
}


const UChar *RelativeDateFormat::getStringForDay(int32_t day, int32_t &len, UErrorCode &status) const {
    if(U_FAILURE(status)) {
        return NULL;
    }
    
    // Is it outside the resource bundle's range?
    if(day < fDayMin || day > fDayMax) {
        return NULL; // don't have it.
    }
    
    // Linear search the held strings
    for(int n=0;n<fDatesLen;n++) {
        if(fDates[n].offset == day) {
            len = fDates[n].len;
            return fDates[n].string;
        }
    }
    
    return NULL;  // not found.
}

UnicodeString&
RelativeDateFormat::toPattern(UnicodeString& result, UErrorCode& status) const
{
    if (!U_FAILURE(status)) {
        result.remove();
        if (fTimeFormat == NULL || fCombinedFormat == 0) {
            if (fDateFormat != NULL) {
                UnicodeString datePattern;
                this->toPatternDate(datePattern, status);
                if (!U_FAILURE(status)) {
                    result.setTo(datePattern);
                }
            }
        } else {
            UnicodeString datePattern, timePattern;
            this->toPatternDate(datePattern, status);
            this->toPatternTime(timePattern, status);
            if (!U_FAILURE(status)) {
                Formattable timeDatePatterns[] = { timePattern, datePattern };
                FieldPosition pos;
                fCombinedFormat->format(timeDatePatterns, 2, result, pos, status);
            }
        }
    }
    return result;
}

UnicodeString&
RelativeDateFormat::toPatternDate(UnicodeString& result, UErrorCode& status) const
{
    if (!U_FAILURE(status)) {
        result.remove();
        if ( fDateFormat ) {
            SimpleDateFormat* sdtfmt = dynamic_cast<SimpleDateFormat*>(fDateFormat);
            if (sdtfmt != NULL) {
                sdtfmt->toPattern(result);
            } else {
                status = U_UNSUPPORTED_ERROR;
            }
        }
    }
    return result;
}

UnicodeString&
RelativeDateFormat::toPatternTime(UnicodeString& result, UErrorCode& status) const
{
    if (!U_FAILURE(status)) {
        result.remove();
        if ( fTimeFormat ) {
            SimpleDateFormat* sdtfmt = dynamic_cast<SimpleDateFormat*>(fTimeFormat);
            if (sdtfmt != NULL) {
                sdtfmt->toPattern(result);
            } else {
                status = U_UNSUPPORTED_ERROR;
            }
        }
    }
    return result;
}

void
RelativeDateFormat::applyPatterns(const UnicodeString& datePattern, const UnicodeString& timePattern, UErrorCode &status)
{
    if (!U_FAILURE(status)) {
        SimpleDateFormat* sdtfmt = NULL;
        SimpleDateFormat* stmfmt = NULL;
        if (fDateFormat && (sdtfmt = dynamic_cast<SimpleDateFormat*>(fDateFormat)) == NULL) {
            status = U_UNSUPPORTED_ERROR;
            return;
        }
        if (fTimeFormat && (stmfmt = dynamic_cast<SimpleDateFormat*>(fTimeFormat)) == NULL) {
            status = U_UNSUPPORTED_ERROR;
            return;
        }
        if ( fDateFormat ) {
            sdtfmt->applyPattern(datePattern);
        }
        if ( fTimeFormat ) {
            stmfmt->applyPattern(timePattern);
        }
    }
}

void RelativeDateFormat::loadDates(UErrorCode &status) {
    CalendarData calData(fLocale, "gregorian", status);
    
    UErrorCode tempStatus = status;
    UResourceBundle *dateTimePatterns = calData.getByKey(DT_DateTimePatternsTag, tempStatus);
    if(U_SUCCESS(tempStatus)) {
        int32_t patternsSize = ures_getSize(dateTimePatterns);
        if (patternsSize > kDateTime) {
            int32_t resStrLen = 0;

            int32_t glueIndex = kDateTime;
            if (patternsSize >= (DateFormat::kDateTimeOffset + DateFormat::kShort + 1)) {
                // Get proper date time format
                switch (fDateStyle) { 
 	            case kFullRelative: 
 	            case kFull: 
 	                glueIndex = kDateTimeOffset + kFull; 
 	                break; 
 	            case kLongRelative: 
 	            case kLong: 
 	                glueIndex = kDateTimeOffset + kLong; 
 	                break; 
 	            case kMediumRelative: 
 	            case kMedium: 
 	                glueIndex = kDateTimeOffset + kMedium; 
 	                break;         
 	            case kShortRelative: 
 	            case kShort: 
 	                glueIndex = kDateTimeOffset + kShort; 
 	                break; 
 	            default: 
 	                break; 
 	            } 
            }

            const UChar *resStr = ures_getStringByIndex(dateTimePatterns, glueIndex, &resStrLen, &tempStatus);
            fCombinedFormat = new MessageFormat(UnicodeString(TRUE, resStr, resStrLen), fLocale, tempStatus);
        }
    }

    UResourceBundle *strings = calData.getByKey3("fields", "day", "relative", status);
    // set up min/max 
    fDayMin=-1;
    fDayMax=1;

    if(U_FAILURE(status)) {
        fDatesLen=0;
        return;
    }

    fDatesLen = ures_getSize(strings);
    fDates = (URelativeString*) uprv_malloc(sizeof(fDates[0])*fDatesLen);

    // Load in each item into the array...
    int n = 0;

    UResourceBundle *subString = NULL;
    
    while(ures_hasNext(strings) && U_SUCCESS(status)) {  // iterate over items
        subString = ures_getNextResource(strings, subString, &status);
        
        if(U_FAILURE(status) || (subString==NULL)) break;
        
        // key = offset #
        const char *key = ures_getKey(subString);
        
        // load the string and length
        int32_t aLen;
        const UChar* aString = ures_getString(subString, &aLen, &status);
        
        if(U_FAILURE(status) || aString == NULL) break;

        // calculate the offset
        int32_t offset = atoi(key);
        
        // set min/max
        if(offset < fDayMin) {
            fDayMin = offset;
        }
        if(offset > fDayMax) {
            fDayMax = offset;
        }
        
        // copy the string pointer
        fDates[n].offset = offset;
        fDates[n].string = aString;
        fDates[n].len = aLen; 

        n++;
    }
    ures_close(subString);
    
    // the fDates[] array could be sorted here, for direct access.
}


// this should to be in DateFormat, instead it was copied from SimpleDateFormat.

Calendar*
RelativeDateFormat::initializeCalendar(TimeZone* adoptZone, const Locale& locale, UErrorCode& status)
{
    if(!U_FAILURE(status)) {
        fCalendar = Calendar::createInstance(adoptZone?adoptZone:TimeZone::createDefault(), locale, status);
    }
    if (U_SUCCESS(status) && fCalendar == NULL) {
        status = U_MEMORY_ALLOCATION_ERROR;
    }
    return fCalendar;
}

int32_t RelativeDateFormat::dayDifference(Calendar &cal, UErrorCode &status) {
    if(U_FAILURE(status)) {
        return 0;
    }
    // TODO: Cache the nowCal to avoid heap allocs? Would be difficult, don't know the calendar type
    Calendar *nowCal = cal.clone();
    nowCal->setTime(Calendar::getNow(), status);

    // For the day difference, we are interested in the difference in the (modified) julian day number
    // which is midnight to midnight.  Using fieldDifference() is NOT correct here, because 
    // 6pm Jan 4th  to 10am Jan 5th should be considered "tomorrow".
    int32_t dayDiff = cal.get(UCAL_JULIAN_DAY, status) - nowCal->get(UCAL_JULIAN_DAY, status);

    delete nowCal;
    return dayDiff;
}

U_NAMESPACE_END

#endif

