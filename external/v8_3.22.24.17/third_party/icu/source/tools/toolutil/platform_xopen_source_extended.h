/*
**********************************************************************
* Copyright (c) 2009, International Business Machines
* Corporation and others.  All Rights Reserved.
**********************************************************************
*/
#ifndef _PLATFORM_XOPEN_SOURCE_EXTENDED_H
#define _PLATFORM_XOPEN_SOURCE_EXTENDED_H

#include "unicode/utypes.h"

/*
 * z/OS needs this definition for timeval and to get usleep.
 * We move this definition out of the various source files because
 * there might be some platform issues when this is defined. 
 * See below.
 */
#if !defined(_XOPEN_SOURCE_EXTENDED)
#define _XOPEN_SOURCE_EXTENDED 1
#endif

/*
 * There is an issue with turning on _XOPEN_SOURCE_EXTENDED on certain platforms.
 * A compatibility issue exists between turning on _XOPEN_SOURCE_EXTENDED and using
 * standard C++ string class. As a result, standard C++ string class needs to be
 * turned off for the follwing platforms:
 *  -AIX/VACPP
 *  -Solaris/GCC
 */
#if (defined(U_AIX) && !defined(__GNUC__)) || (defined(U_SOLARIS) && defined(__GNUC__))
#   if _XOPEN_SOURCE_EXTENDED && !defined(U_HAVE_STD_STRING)
#   define U_HAVE_STD_STRING 0
#   endif
#endif

#endif /* _PLATFORM_XOPEN_SOURCE_EXTENDED_H */
