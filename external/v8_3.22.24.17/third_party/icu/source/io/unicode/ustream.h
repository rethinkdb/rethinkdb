/*
**********************************************************************
*   Copyright (C) 2001-2010 International Business Machines
*   Corporation and others.  All Rights Reserved.
**********************************************************************
*  FILE NAME : ustream.h
*
*   Modification History:
*
*   Date        Name        Description
*   06/25/2001  grhoten     Move iostream from unistr.h
******************************************************************************
*/

#ifndef USTREAM_H
#define USTREAM_H

#include "unicode/unistr.h"

/**
 * \file
 * \brief C++ API: Unicode iostream like API
 *
 * At this time, this API is very limited. It contains
 * operator<< and operator>> for UnicodeString manipulation with the
 * C++ I/O stream API.
 */

#if U_IOSTREAM_SOURCE >= 199711
#if (__GNUC__ == 2)
#include <iostream>
#else
#include <istream>
#include <ostream>
#endif

U_NAMESPACE_BEGIN

/**
 * Write the contents of a UnicodeString to a C++ ostream. This functions writes
 * the characters in a UnicodeString to an ostream. The UChars in the
 * UnicodeString are converted to the char based ostream with the default
 * converter.
 * @stable 3.0
 */
U_IO_API std::ostream & U_EXPORT2 operator<<(std::ostream& stream, const UnicodeString& s);

/**
 * Write the contents from a C++ istream to a UnicodeString. The UChars in the
 * UnicodeString are converted from the char based istream with the default
 * converter.
 * @stable 3.0
 */
U_IO_API std::istream & U_EXPORT2 operator>>(std::istream& stream, UnicodeString& s);
U_NAMESPACE_END

#elif U_IOSTREAM_SOURCE >= 198506
/* <istream.h> and <ostream.h> don't exist. */
#include <iostream.h>

U_NAMESPACE_BEGIN
U_IO_API ostream & U_EXPORT2 operator<<(ostream& stream, const UnicodeString& s);

U_IO_API istream & U_EXPORT2 operator>>(istream& stream, UnicodeString& s);
U_NAMESPACE_END

#endif

/* No operator for UChar because it can conflict with wchar_t  */

#endif
