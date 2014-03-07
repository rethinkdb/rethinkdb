
/*
************************************************************************
* Copyright (c) 2007-2010, International Business Machines
* Corporation and others.  All Rights Reserved.
************************************************************************
*/

/** C++ Utilities to aid in debugging **/

#ifndef _DBGUTIL_H
#define _DBGUTIL_H

#include "unicode/utypes.h"
#include "udbgutil.h"
#include "unicode/unistr.h"

#if !UCONFIG_NO_FORMATTING

U_CAPI const U_NAMESPACE_QUALIFIER UnicodeString& U_EXPORT2
udbg_enumString(UDebugEnumType type, int32_t field);

/**
 * @return enum offset, or UDBG_INVALID_ENUM on error
 */ 
U_CAPI int32_t U_EXPORT2
udbg_enumByString(UDebugEnumType type, const U_NAMESPACE_QUALIFIER UnicodeString& string);

/**
 * Convert a UnicodeString (with ascii digits) into a number.
 * @param s string
 * @return numerical value, or 0 on error
 */
U_CAPI int32_t U_EXPORT2 udbg_stoi(const U_NAMESPACE_QUALIFIER UnicodeString &s);

U_CAPI double U_EXPORT2 udbg_stod(const U_NAMESPACE_QUALIFIER UnicodeString &s);

U_CAPI U_NAMESPACE_QUALIFIER UnicodeString * U_EXPORT2
udbg_escape(const U_NAMESPACE_QUALIFIER UnicodeString &s, U_NAMESPACE_QUALIFIER UnicodeString *dst);

#endif

#endif
