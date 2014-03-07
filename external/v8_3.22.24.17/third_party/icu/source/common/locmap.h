/*
******************************************************************************
*
*   Copyright (C) 1996-2004, International Business Machines
*   Corporation and others.  All Rights Reserved.
*
******************************************************************************
*
* File locmap.h      : Locale Mapping Classes
* 
*
* Created by: Helena Shih
*
* Modification History:
*
*  Date        Name        Description
*  3/11/97     aliu        Added setId().
*  4/20/99     Madhu       Added T_convertToPosix()
* 09/18/00     george      Removed the memory leaks.
* 08/23/01     george      Convert to C
*============================================================================
*/

#ifndef LOCMAP_H
#define LOCMAP_H

#include "unicode/utypes.h"

#define LANGUAGE_LCID(hostID) (uint16_t)(0x03FF & hostID)

U_CAPI const char *uprv_convertToPosix(uint32_t hostid, UErrorCode* status);

/* Don't call this function directly. Use uloc_getLCID instead. */
U_CAPI uint32_t uprv_convertToLCID(const char *langID, const char* posixID, UErrorCode* status);

#endif /* LOCMAP_H */

