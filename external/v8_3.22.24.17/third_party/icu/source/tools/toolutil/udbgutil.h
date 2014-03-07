/*
************************************************************************
* Copyright (c) 2008-2010, International Business Machines
* Corporation and others.  All Rights Reserved.
************************************************************************
*/

/** C Utilities to aid in debugging **/

#ifndef _UDBGUTIL_H
#define _UDBGUTIL_H

#include "unicode/utypes.h"


enum UDebugEnumType {
    UDBG_UDebugEnumType = 0, /* Self-referential, strings for UDebugEnumType. Count=ENUM_COUNT. */
#if !UCONFIG_NO_FORMATTING
    UDBG_UCalendarDateFields, /* UCalendarDateFields. Count=UCAL_FIELD_COUNT.  Unsupported if UCONFIG_NO_FORMATTING. */
    UDBG_UCalendarMonths, /* UCalendarMonths. Count= (UCAL_UNDECIMBER+1) */
    UDBG_UDateFormatStyle, /* Count = UDAT_SHORT=1 */
#endif
    UDBG_UPlugReason,   /* Count = UPLUG_REASON_COUNT */
    UDBG_UPlugLevel,    /* COUNT = UPLUG_LEVEL_COUNT */
    UDBG_UAcceptResult, /* Count = ULOC_ACCEPT_FALLBACK+1=3 */
    
    /* All following enums may be discontiguous. */ 
    
#if !UCONFIG_NO_COLLATION
    UDBG_UColAttributeValue,  /* UCOL_ATTRIBUTE_VALUE_COUNT */
#endif
    UDBG_ENUM_COUNT,
    UDBG_HIGHEST_CONTIGUOUS_ENUM = UDBG_UAcceptResult,  /**< last enum in this list with contiguous (testable) values. */
    UDBG_INVALID_ENUM = -1 /** Invalid enum value **/
};

typedef enum UDebugEnumType UDebugEnumType;

/**
 * @param type the type of enum
 * Print how many enums are contained for this type. 
 * Should be equal to the appropriate _COUNT constant or there is an error. Return -1 if unsupported.
 */
U_CAPI int32_t U_EXPORT2 udbg_enumCount(UDebugEnumType type);

/**
 * Convert an enum to a string
 * @param type type of enum
 * @param field field number
 * @return string of the format "ERA", "YEAR", etc, or NULL if out of range or unsupported
 */
U_CAPI const char * U_EXPORT2 udbg_enumName(UDebugEnumType type, int32_t field);

/**
 * for consistency checking
 * @param type the type of enum
 * Print how many enums should be contained for this type. 
 * This is equal to the appropriate _COUNT constant or there is an error. Returns -1 if unsupported.
 */
U_CAPI int32_t U_EXPORT2 udbg_enumExpectedCount(UDebugEnumType type);

/**
 * For consistency checking, returns the expected enum ordinal value for the given index value. 
 * @param type which type
 * @param field field number
 * @return should be equal to 'field' or -1 if out of range.
 */
U_CAPI int32_t U_EXPORT2 udbg_enumArrayValue(UDebugEnumType type, int32_t field);

/**
 * Locate the specified field value by name. 
 * @param type which type
 * @param name name of string (case sensitive)
 * @return should be a field value or -1 if not found.
 */
U_CAPI int32_t U_EXPORT2 udbg_enumByName(UDebugEnumType type, const char *name);

#endif
