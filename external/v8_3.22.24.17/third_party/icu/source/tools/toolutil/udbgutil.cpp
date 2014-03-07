/********************************************************************
 * COPYRIGHT:
 * Copyright (c) 2007-2010, International Business Machines Corporation and
 * others. All Rights Reserved.
 ********************************************************************/

#include "udbgutil.h"
#include <string.h>

/*
To add a new enum type 
      (For example: UShoeSize  with values USHOE_WIDE=0, USHOE_REGULAR, USHOE_NARROW, USHOE_COUNT)

    1. udbgutil.h:  add  UDBG_UShoeSize to the UDebugEnumType enum before UDBG_ENUM_COUNT
      ( The subsequent steps involve this file, udbgutil.cpp )
    2. Find the marker "Add new enum types above this line"
    3. Before that marker, add a #include of any header file you need.
    4. Each enum type has three things in this section:  a #define, a count_, and an array of Fields. 
       It may help to copy and paste a previous definition.
    5. In the case of the USHOE_... strings above, "USHOE_" is common to all values- six characters
         " #define LEN_USHOE 6 "   
       6 characters will strip off "USHOE_" leaving enum values of WIDE, REGULAR, and NARROW.
    6. Define the 'count_' variable, with the number of enum values. If the enum has a _MAX or _COUNT value, 
        that can be helpful for automatically defining the count. Otherwise define it manually.
        " static const int32_t count_UShoeSize = USHOE_COUNT; "
    7. Define the field names, in order.
        " static const Field names_UShoeSize[] =  {
        "  FIELD_NAME_STR( LEN_USHOE, USHOE_WIDE ), 
        "  FIELD_NAME_STR( LEN_USHOE, USHOE_REGULAR ), 
        "  FIELD_NAME_STR( LEN_USHOE, USHOE_NARROW ), 
        " };
      ( The following command  was usedfor converting ucol.h into partially correct entities )
      grep "^[  ]*UCOL" < unicode/ucol.h  | 
         sed -e 's%^[  ]*\([A-Z]*\)_\([A-Z_]*\).*%   FIELD_NAME_STR( LEN_\1, \1_\2 ),%g' 
    8. Now, a bit farther down, add the name of the enum itself to the end of names_UDebugEnumType
          ( UDebugEnumType is an enum, too!)
        names_UDebugEnumType[] { ...  
            " FIELD_NAME_STR( LEN_UDBG, UDBG_UShoeSize ),   "
    9. Find the function _udbg_enumCount  and add the count macro:
            " COUNT_CASE(UShoeSize)
   10. Find the function _udbg_enumFields  and add the field macro:
            " FIELD_CASE(UShoeSize)
   11. verify that your test code, and Java data generation, works properly.
*/

/**
 * Structure representing an enum value
 */
struct Field {
    int32_t prefix;   /**< how many characters to remove in the prefix - i.e. UCHAR_ = 5 */
	const char *str;  /**< The actual string value */
	int32_t num;      /**< The numeric value */
};

/**
 * Calculate the size of an array.
 */
#define DBG_ARRAY_COUNT(x) (sizeof(x)/sizeof(x[0]))

/**
 * Define another field name. Used in an array of Field s
 * @param y the common prefix length (i.e. 6 for "USHOE_" )
 * @param x the actual enum value - it will be copied in both string and symbolic form.
 * @see Field
 */
#define FIELD_NAME_STR(y,x)  { y, #x, x }


// TODO: Currently, this whole functionality goes away with UCONFIG_NO_FORMATTING. Should be split up.
#if !UCONFIG_NO_FORMATTING

// Calendar
#include "unicode/ucal.h"

// 'UCAL_' = 5
#define LEN_UCAL 5 /* UCAL_ */
static const int32_t count_UCalendarDateFields = UCAL_FIELD_COUNT;
static const Field names_UCalendarDateFields[] = 
{
    FIELD_NAME_STR( LEN_UCAL, UCAL_ERA ),
    FIELD_NAME_STR( LEN_UCAL, UCAL_YEAR ),
    FIELD_NAME_STR( LEN_UCAL, UCAL_MONTH ),
    FIELD_NAME_STR( LEN_UCAL, UCAL_WEEK_OF_YEAR ),
    FIELD_NAME_STR( LEN_UCAL, UCAL_WEEK_OF_MONTH ),
    FIELD_NAME_STR( LEN_UCAL, UCAL_DATE ),
    FIELD_NAME_STR( LEN_UCAL, UCAL_DAY_OF_YEAR ),
    FIELD_NAME_STR( LEN_UCAL, UCAL_DAY_OF_WEEK ),
    FIELD_NAME_STR( LEN_UCAL, UCAL_DAY_OF_WEEK_IN_MONTH ),
    FIELD_NAME_STR( LEN_UCAL, UCAL_AM_PM ),
    FIELD_NAME_STR( LEN_UCAL, UCAL_HOUR ),
    FIELD_NAME_STR( LEN_UCAL, UCAL_HOUR_OF_DAY ),
    FIELD_NAME_STR( LEN_UCAL, UCAL_MINUTE ),
    FIELD_NAME_STR( LEN_UCAL, UCAL_SECOND ),
    FIELD_NAME_STR( LEN_UCAL, UCAL_MILLISECOND ),
    FIELD_NAME_STR( LEN_UCAL, UCAL_ZONE_OFFSET ),
    FIELD_NAME_STR( LEN_UCAL, UCAL_DST_OFFSET ),
    FIELD_NAME_STR( LEN_UCAL, UCAL_YEAR_WOY ),
    FIELD_NAME_STR( LEN_UCAL, UCAL_DOW_LOCAL ),
    FIELD_NAME_STR( LEN_UCAL, UCAL_EXTENDED_YEAR ),
    FIELD_NAME_STR( LEN_UCAL, UCAL_JULIAN_DAY ),
    FIELD_NAME_STR( LEN_UCAL, UCAL_MILLISECONDS_IN_DAY ),
    FIELD_NAME_STR( LEN_UCAL, UCAL_IS_LEAP_MONTH ),
};


static const int32_t count_UCalendarMonths = UCAL_UNDECIMBER+1;
static const Field names_UCalendarMonths[] = 
{
  FIELD_NAME_STR( LEN_UCAL, UCAL_JANUARY ),
  FIELD_NAME_STR( LEN_UCAL, UCAL_FEBRUARY ),
  FIELD_NAME_STR( LEN_UCAL, UCAL_MARCH ),
  FIELD_NAME_STR( LEN_UCAL, UCAL_APRIL ),
  FIELD_NAME_STR( LEN_UCAL, UCAL_MAY ),
  FIELD_NAME_STR( LEN_UCAL, UCAL_JUNE ),
  FIELD_NAME_STR( LEN_UCAL, UCAL_JULY ),
  FIELD_NAME_STR( LEN_UCAL, UCAL_AUGUST ),
  FIELD_NAME_STR( LEN_UCAL, UCAL_SEPTEMBER ),
  FIELD_NAME_STR( LEN_UCAL, UCAL_OCTOBER ),
  FIELD_NAME_STR( LEN_UCAL, UCAL_NOVEMBER ),
  FIELD_NAME_STR( LEN_UCAL, UCAL_DECEMBER ),
  FIELD_NAME_STR( LEN_UCAL, UCAL_UNDECIMBER)
};

#include "unicode/udat.h"

#define LEN_UDAT 5 /* "UDAT_" */
static const int32_t count_UDateFormatStyle = UDAT_SHORT+1;
static const Field names_UDateFormatStyle[] = 
{
        FIELD_NAME_STR( LEN_UDAT, UDAT_FULL ),
        FIELD_NAME_STR( LEN_UDAT, UDAT_LONG ),
        FIELD_NAME_STR( LEN_UDAT, UDAT_MEDIUM ),
        FIELD_NAME_STR( LEN_UDAT, UDAT_SHORT ),
        /* end regular */
    /*
     *  negative enums.. leave out for now.
        FIELD_NAME_STR( LEN_UDAT, UDAT_NONE ),
        FIELD_NAME_STR( LEN_UDAT, UDAT_IGNORE ),
     */
};

#endif
 
#include "unicode/uloc.h"

#define LEN_UAR 12 /* "ULOC_ACCEPT_" */
static const int32_t count_UAcceptResult = 3;
static const Field names_UAcceptResult[] = 
{
        FIELD_NAME_STR( LEN_UAR, ULOC_ACCEPT_FAILED ),
        FIELD_NAME_STR( LEN_UAR, ULOC_ACCEPT_VALID ),
        FIELD_NAME_STR( LEN_UAR, ULOC_ACCEPT_FALLBACK ),
};

#if !UCONFIG_NO_COLLATION
#include "unicode/ucol.h"
#define LEN_UCOL 5 /* UCOL_ */
static const int32_t count_UColAttributeValue = UCOL_ATTRIBUTE_VALUE_COUNT;
static const Field names_UColAttributeValue[]  = {
   FIELD_NAME_STR( LEN_UCOL, UCOL_PRIMARY ),
   FIELD_NAME_STR( LEN_UCOL, UCOL_SECONDARY ),
   FIELD_NAME_STR( LEN_UCOL, UCOL_TERTIARY ),
//   FIELD_NAME_STR( LEN_UCOL, UCOL_CE_STRENGTH_LIMIT ),
   FIELD_NAME_STR( LEN_UCOL, UCOL_QUATERNARY ),
   // gap
   FIELD_NAME_STR( LEN_UCOL, UCOL_IDENTICAL ),
//   FIELD_NAME_STR( LEN_UCOL, UCOL_STRENGTH_LIMIT ),
   FIELD_NAME_STR( LEN_UCOL, UCOL_OFF ),
   FIELD_NAME_STR( LEN_UCOL, UCOL_ON ),
   // gap
   FIELD_NAME_STR( LEN_UCOL, UCOL_SHIFTED ),
   FIELD_NAME_STR( LEN_UCOL, UCOL_NON_IGNORABLE ),
   // gap
   FIELD_NAME_STR( LEN_UCOL, UCOL_LOWER_FIRST ),
   FIELD_NAME_STR( LEN_UCOL, UCOL_UPPER_FIRST ),
};

#endif


#include "unicode/icuplug.h"

#define LEN_UPLUG_REASON 13 /* UPLUG_REASON_ */
static const int32_t count_UPlugReason = UPLUG_REASON_COUNT;
static const Field names_UPlugReason[]  = {
   FIELD_NAME_STR( LEN_UPLUG_REASON, UPLUG_REASON_QUERY ),
   FIELD_NAME_STR( LEN_UPLUG_REASON, UPLUG_REASON_LOAD ),
   FIELD_NAME_STR( LEN_UPLUG_REASON, UPLUG_REASON_UNLOAD ),
};

#define LEN_UPLUG_LEVEL 12 /* UPLUG_LEVEL_ */
static const int32_t count_UPlugLevel = UPLUG_LEVEL_COUNT;
static const Field names_UPlugLevel[]  = {
   FIELD_NAME_STR( LEN_UPLUG_LEVEL, UPLUG_LEVEL_INVALID ),
   FIELD_NAME_STR( LEN_UPLUG_LEVEL, UPLUG_LEVEL_UNKNOWN ),
   FIELD_NAME_STR( LEN_UPLUG_LEVEL, UPLUG_LEVEL_LOW ),
   FIELD_NAME_STR( LEN_UPLUG_LEVEL, UPLUG_LEVEL_HIGH ),
};

#define LEN_UDBG 5 /* "UDBG_" */
static const int32_t count_UDebugEnumType = UDBG_ENUM_COUNT;
static const Field names_UDebugEnumType[] = 
{
    FIELD_NAME_STR( LEN_UDBG, UDBG_UDebugEnumType ),
#if !UCONFIG_NO_FORMATTING
    FIELD_NAME_STR( LEN_UDBG, UDBG_UCalendarDateFields ),
    FIELD_NAME_STR( LEN_UDBG, UDBG_UCalendarMonths ),
    FIELD_NAME_STR( LEN_UDBG, UDBG_UDateFormatStyle ),
#endif
    FIELD_NAME_STR( LEN_UDBG, UDBG_UPlugReason ),
    FIELD_NAME_STR( LEN_UDBG, UDBG_UPlugLevel ),
    FIELD_NAME_STR( LEN_UDBG, UDBG_UAcceptResult ),
#if !UCONFIG_NO_COLLATION
    FIELD_NAME_STR( LEN_UDBG, UDBG_UColAttributeValue ),
#endif
};


// --- Add new enum types above this line ---

#define COUNT_CASE(x)  case UDBG_##x: return (actual?count_##x:DBG_ARRAY_COUNT(names_##x));
#define COUNT_FAIL_CASE(x) case UDBG_##x: return -1;

#define FIELD_CASE(x)  case UDBG_##x: return names_##x;
#define FIELD_FAIL_CASE(x) case UDBG_##x: return NULL;

// low level

/**
 * @param type type of item
 * @param actual TRUE: for the actual enum's type (UCAL_FIELD_COUNT, etc), or FALSE for the string count
 */
static int32_t _udbg_enumCount(UDebugEnumType type, UBool actual) {
	switch(type) {
		COUNT_CASE(UDebugEnumType)
#if !UCONFIG_NO_FORMATTING
		COUNT_CASE(UCalendarDateFields)
		COUNT_CASE(UCalendarMonths)
		COUNT_CASE(UDateFormatStyle)
#endif
        COUNT_CASE(UPlugReason)
        COUNT_CASE(UPlugLevel)
        COUNT_CASE(UAcceptResult)
#if !UCONFIG_NO_COLLATION
        COUNT_CASE(UColAttributeValue)
#endif
		// COUNT_FAIL_CASE(UNonExistentEnum)
	default:
		return -1;
	}
}

static const Field* _udbg_enumFields(UDebugEnumType type) {
	switch(type) {
		FIELD_CASE(UDebugEnumType)
#if !UCONFIG_NO_FORMATTING
		FIELD_CASE(UCalendarDateFields)
		FIELD_CASE(UCalendarMonths)
		FIELD_CASE(UDateFormatStyle)
#endif
        FIELD_CASE(UPlugReason)
        FIELD_CASE(UPlugLevel)
        FIELD_CASE(UAcceptResult)
		// FIELD_FAIL_CASE(UNonExistentEnum)
#if !UCONFIG_NO_COLLATION
        FIELD_CASE(UColAttributeValue)
#endif
	default:
		return NULL;
	}
}

// implementation

int32_t  udbg_enumCount(UDebugEnumType type) {
	return _udbg_enumCount(type, FALSE);
}

int32_t  udbg_enumExpectedCount(UDebugEnumType type) {
	return _udbg_enumCount(type, TRUE);
}

const char *  udbg_enumName(UDebugEnumType type, int32_t field) {
	if(field<0 || 
				field>=_udbg_enumCount(type,FALSE)) { // also will catch unsupported items
		return NULL;
	} else {
		const Field *fields = _udbg_enumFields(type);
		if(fields == NULL) {
			return NULL;
		} else {
			return fields[field].str + fields[field].prefix;
		}
	}
}

int32_t  udbg_enumArrayValue(UDebugEnumType type, int32_t field) {
	if(field<0 || 
				field>=_udbg_enumCount(type,FALSE)) { // also will catch unsupported items
		return -1;
	} else {
		const Field *fields = _udbg_enumFields(type);
		if(fields == NULL) {
			return -1;
		} else {
			return fields[field].num;
		}
	}    
}

int32_t udbg_enumByName(UDebugEnumType type, const char *value) {
    if(type<0||type>=_udbg_enumCount(UDBG_UDebugEnumType, TRUE)) {
        return -1; // type out of range
    }
	const Field *fields = _udbg_enumFields(type);
    for(int32_t field = 0;field<_udbg_enumCount(type, FALSE);field++) {
        if(!strcmp(value, fields[field].str + fields[field].prefix)) {
            return fields[field].num;
        }        
    }
    // try with the prefix
    for(int32_t field = 0;field<_udbg_enumCount(type, FALSE);field++) {
        if(!strcmp(value, fields[field].str)) {
            return fields[field].num;
        }        
    }
    // fail
    return -1;
}
