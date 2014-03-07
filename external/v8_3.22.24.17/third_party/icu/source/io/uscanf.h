/*
******************************************************************************
*
*   Copyright (C) 1998-2004, International Business Machines
*   Corporation and others.  All Rights Reserved.
*
******************************************************************************
*
* File uscanf.h
*
* Modification History:
*
*   Date        Name        Description
*   12/02/98    stephen        Creation.
*   03/13/99    stephen     Modified for new C API.
******************************************************************************
*/

#ifndef USCANF_H
#define USCANF_H

#include "unicode/utypes.h"

#if !UCONFIG_NO_FORMATTING

#include "unicode/ustdio.h"

U_CFUNC int32_t
u_scanf_parse(UFILE     *f,
            const UChar *patternSpecification,
            va_list     ap);

#endif /* #if !UCONFIG_NO_FORMATTING */

#endif

