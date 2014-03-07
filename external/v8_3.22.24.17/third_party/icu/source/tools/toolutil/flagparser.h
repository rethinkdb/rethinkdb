/*
*******************************************************************************
*
*   Copyright (C) 2009-2010, International Business Machines
*   Corporation and others.  All Rights Reserved.
*
*******************************************************************************
*   file name:  flagparser.h
*   encoding:   US-ASCII
*   tab size:   8 (not used)
*   indentation:4
*
*   created on: 2009jan08
*   created by: Michael Ow
*
* Tiny flag file parser using ICU and intended for use in ICU tests and in build tools.
* Not suitable for production use. Not supported.
* Not conformant. Not efficient.
* But very small.
*/

#ifndef __FLAGPARSER_H__
#define __FLAGPARSER_H__

#include "unicode/utypes.h"

U_CAPI int32_t U_EXPORT2
parseFlagsFile(const char *fileName, char **flagBuffer, int32_t flagBufferSize, int32_t numOfFlags, UErrorCode *status);

#endif
