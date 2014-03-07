/*
************************************************************************
* Copyright (c) 2007-2008, International Business Machines
* Corporation and others.  All Rights Reserved.
************************************************************************
*/

#include "fldset.h"
#include "intltest.h"

#if !UCONFIG_NO_FORMATTING
#include "unicode/regex.h"


FieldsSet::FieldsSet() {
    // NOTREACHED
}

FieldsSet::FieldsSet(int32_t fieldCount) {
    construct((UDebugEnumType)-1, fieldCount);
}

FieldsSet::FieldsSet(UDebugEnumType field) {
    construct(field, udbg_enumCount(field));
}

FieldsSet::~FieldsSet() {
    
}

int32_t FieldsSet::fieldCount() const {
    return fFieldCount;
}

void FieldsSet::construct(UDebugEnumType field, int32_t fieldCount) {
    fEnum = field;
    if(fieldCount > U_FIELDS_SET_MAX) {
        fieldCount = U_FIELDS_SET_MAX;
    }
    fFieldCount = fieldCount;
    clear();
}

UnicodeString FieldsSet::diffFrom(const FieldsSet& other, UErrorCode& status) const {
    UnicodeString str;
    if(!isSameType(other)) {
        status = U_ILLEGAL_ARGUMENT_ERROR;
        return UnicodeString("U_ILLEGAL_ARGUMENT_ERROR: FieldsSet of a different type!");
    }
    for (int i=0; i<fieldCount(); i++) {
        if (isSet((UCalendarDateFields)i)) {
            int32_t myVal = get(i);
            int32_t theirVal = other.get(i);
            
            if(fEnum != -1) {
                const UnicodeString& fieldName = udbg_enumString(
                        fEnum, i);

                str = str + fieldName + UnicodeString("=")+myVal+UnicodeString(" not ")+theirVal+UnicodeString(", ");
            } else {
                str = str + UnicodeString("some field") + "=" + myVal+" not " + theirVal+", ";
            }
        }
    }
    return str;
}

static UnicodeString *split(const UnicodeString &src, UChar ch, int32_t &splits)
{
    int32_t offset = -1;

    splits = 1;
    while((offset = src.indexOf(ch, offset + 1)) >= 0) {
        splits += 1;
    }

    UnicodeString *result = new UnicodeString[splits];

    int32_t start = 0;
    int32_t split = 0;
    int32_t end;

    while((end = src.indexOf(ch, start)) >= 0) {
        src.extractBetween(start, end, result[split++]);
        start = end + 1;
    }

    src.extractBetween(start, src.length(), result[split]);

    return result;
}

int32_t FieldsSet::parseFrom(const UnicodeString& str, const 
        FieldsSet* inheritFrom, UErrorCode& status) {

    int goodFields = 0;

    if(U_FAILURE(status)) {
        return -1;
    }

    int32_t destCount = 0;
    UnicodeString *dest = split(str, 0x002C /* ',' */, destCount);

    for(int i = 0; i < destCount; i += 1) {
        int32_t dc = 0;
        UnicodeString *kv = split(dest[i], 0x003D /* '=' */, dc);

        if(dc != 2) {
            it_errln(UnicodeString("dc == ") + dc + UnicodeString("?"));
        }

        int32_t field = handleParseName(inheritFrom, kv[0], kv[1], status);

        if(U_FAILURE(status)) {
            char ch[256];
            const UChar *u = kv[0].getBuffer();
            int32_t len = kv[0].length();
            u_UCharsToChars(u, ch, len);
            ch[len] = 0; /* include terminating \0 */
            it_errln(UnicodeString("Parse Failed: Field ") + UnicodeString(ch) + UnicodeString(", err ") + UnicodeString(u_errorName(status)));
            return -1;
        }

        if(field != -1) {
            handleParseValue(inheritFrom, field, kv[1], status);

            if(U_FAILURE(status)) {
                char ch[256];
                const UChar *u = kv[1].getBuffer();
                int32_t len = kv[1].length();
                u_UCharsToChars(u, ch, len);
                ch[len] = 0; /* include terminating \0 */
                it_errln(UnicodeString("Parse Failed: Value ") + UnicodeString(ch) + UnicodeString(", err ") + UnicodeString(u_errorName(status)));
                return -1;
            }

            goodFields += 1;
        }

        delete[] kv;
    }

    delete[] dest;

    return goodFields;
}

UBool FieldsSet::isSameType(const FieldsSet& other) const {
    return((&other==this)||
           ((other.fFieldCount==fFieldCount) && (other.fEnum==fEnum)));  
}

void FieldsSet::clear() {
    for (int i=0; i<fieldCount(); i++) {
        fValue[i]=-1;
        fIsSet[i]=FALSE;
    }
}

void FieldsSet::clear(int32_t field) {
    if (field<0|| field>=fieldCount()) {
        return;
    }
    fValue[field] = -1;
    fIsSet[field] = FALSE;
}
void FieldsSet::set(int32_t field, int32_t amount) {
    if (field<0|| field>=fieldCount()) {
        return;
    }
    fValue[field] = amount;
    fIsSet[field] = TRUE;
}

UBool FieldsSet::isSet(int32_t field) const {
    if (field<0|| field>=fieldCount()) {
        return FALSE;
    }
    return fIsSet[field];
}
int32_t FieldsSet::get(int32_t field) const {
    if (field<0|| field>=fieldCount()) {
        return -1;
    }
    return fValue[field];
}


int32_t FieldsSet::handleParseName(const FieldsSet* /* inheritFrom */, const UnicodeString& name, const UnicodeString& /* substr*/ , UErrorCode& status) {
    if(fEnum > -1) {
        int32_t which = udbg_enumByString(fEnum, name);
        if(which == UDBG_INVALID_ENUM) {
            status = U_UNSUPPORTED_ERROR;
        }
        return which;
    } else {
        status = U_UNSUPPORTED_ERROR;
        return -1;
    }
}

void FieldsSet::parseValueDefault(const FieldsSet* inheritFrom, int32_t field, const UnicodeString& substr, UErrorCode& status) {
    int32_t value = -1;
    if(substr.length()==0) { // inherit requested
        // inherit
        if((inheritFrom == NULL) || !inheritFrom->isSet((UCalendarDateFields)field)) {
            // couldn't inherit from field 
            it_errln(UnicodeString("Parse Failed: Couldn't inherit field ") + field + UnicodeString(" [") + UnicodeString(udbg_enumName(fEnum, field)) + UnicodeString("]"));
            status = U_ILLEGAL_ARGUMENT_ERROR;
            return;
        }
        value = inheritFrom->get((UCalendarDateFields)field);
    } else {        
        value = udbg_stoi(substr);
    }
    set(field, value);
}

void FieldsSet::parseValueEnum(UDebugEnumType type, const FieldsSet* inheritFrom, int32_t field, const UnicodeString& substr, UErrorCode& status) {
    int32_t value = udbg_enumByString(type, substr);
    if(value>=0) {
        set(field, value);
    } else {
        // fallback
        parseValueDefault(inheritFrom,field,substr,status);
    }
}

void FieldsSet::handleParseValue(const FieldsSet* inheritFrom, int32_t field, const UnicodeString& substr, UErrorCode& status) {
    parseValueDefault(inheritFrom, field, substr, status);
}

/// CAL FIELDS


CalendarFieldsSet::CalendarFieldsSet() :
FieldsSet(UDBG_UCalendarDateFields) {
    // base class will call clear.
}

CalendarFieldsSet::~CalendarFieldsSet() {
}

void CalendarFieldsSet::handleParseValue(const FieldsSet* inheritFrom, int32_t field, const UnicodeString& substr, UErrorCode& status) {
    if(field==UCAL_MONTH) {
        parseValueEnum(UDBG_UCalendarMonths, inheritFrom, field, substr, status);
        // will fallback to default.
    } else {
        parseValueDefault(inheritFrom, field, substr, status);
    }
}

/**
 * set the specified fields on this calendar. Doesn't clear first. Returns any errors the caller 
 */
void CalendarFieldsSet::setOnCalendar(Calendar *cal, UErrorCode& /*status*/) const {
    for (int i=0; i<UDAT_FIELD_COUNT; i++) {
        if (isSet((UCalendarDateFields)i)) {
            int32_t value = get((UCalendarDateFields)i);
            cal->set((UCalendarDateFields)i, value);
        }
    }
}

/**
 * return true if the calendar matches in these fields
 */
UBool CalendarFieldsSet::matches(Calendar *cal, CalendarFieldsSet &diffSet,
        UErrorCode& status) const {
    UBool match = TRUE;
    if (U_FAILURE(status)) {
        return FALSE;
    }
    for (int i=0; i<UDAT_FIELD_COUNT; i++) {
        if (isSet((UCalendarDateFields)i)) {
            int32_t calVal = cal->get((UCalendarDateFields)i, status);
            if (U_FAILURE(status))
                return FALSE;
            if (calVal != get((UCalendarDateFields)i)) {
                match = FALSE;
                diffSet.set((UCalendarDateFields)i, calVal);
                //fprintf(stderr, "match failed: %s#%d=%d != %d\n",udbg_enumName(UDBG_UCalendarDateFields,i),i,cal->get((UCalendarDateFields)i,status), get((UCalendarDateFields)i));;
            }
        }
    }
    return match;
}


/**
 * DateTimeStyleSet has two 'fields' -- date, and time.
 */
enum DateTimeStyleSetFields {
    DTS_DATE = 0,  /** Field one: the date (long, medium, short, etc). */
    DTS_TIME,      /** Field two: the time (long, medium, short, etc). */
    DTS_COUNT      /** The number of fields */
};

/**
 * DateTimeSet 
 * */
DateTimeStyleSet::DateTimeStyleSet() :
    FieldsSet(DTS_COUNT) {
}

DateTimeStyleSet::~DateTimeStyleSet() {
    
}

UDateFormatStyle DateTimeStyleSet::getDateStyle() const {
    if(!isSet(DTS_DATE)) {
        return UDAT_NONE;
    } else {
        return (UDateFormatStyle)get(DTS_DATE);
    }
}


UDateFormatStyle DateTimeStyleSet::getTimeStyle() const {
    if(!isSet(DTS_TIME)) {
        return UDAT_NONE;
    } else {
        return (UDateFormatStyle)get(DTS_TIME);
    }
}

void DateTimeStyleSet::handleParseValue(const FieldsSet* inheritFrom, int32_t field, const UnicodeString& substr, UErrorCode& status) {
    UnicodeString kRELATIVE_("RELATIVE_");
    if(substr.startsWith(kRELATIVE_)) {
        UnicodeString relativeas(substr,kRELATIVE_.length());
        parseValueEnum(UDBG_UDateFormatStyle, inheritFrom, field, relativeas, status);
        // fix relative value
        if(isSet(field) && U_SUCCESS(status)) {
            set(field, get(field) | UDAT_RELATIVE);
        }
    } else {
        parseValueEnum(UDBG_UDateFormatStyle, inheritFrom, field, substr, status);
    }
}

int32_t DateTimeStyleSet::handleParseName(const FieldsSet* /* inheritFrom */, const UnicodeString& name, const UnicodeString& /* substr */, UErrorCode& status) {
    UnicodeString kDATE("DATE"); // TODO: static
    UnicodeString kTIME("TIME"); // TODO: static
    if(name == kDATE ) { 
        return DTS_DATE;
    } else if(name == kTIME) {
        return DTS_TIME;
    } else {
        status = U_ILLEGAL_ARGUMENT_ERROR;
        return -1;   
    }
}

#endif /*!UCONFIG_NO_FORMAT*/
