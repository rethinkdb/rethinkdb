/********************************************************************
 * COPYRIGHT: 
 * Copyright (c) 1997-2001, International Business Machines Corporation and
 * others. All Rights Reserved.
 ********************************************************************/
/********************************************************************************
*
* File CFORMTST.H
*
* Modification History:
*        Name                     Description            
*     Madhu Katragadda               Creation
*********************************************************************************
*/
/* FormatTest is a medium top level test for everything in the  C FORMAT API */

#ifndef _CFORMATTST
#define _CFORMATTST

#include "unicode/utypes.h"

#if !UCONFIG_NO_FORMATTING

#include "cintltst.h"
#include "unicode/udat.h"


/* Internal fucntion used by all the test format files */
UChar* myDateFormat(UDateFormat *dat, UDate d); 

#endif /* #if !UCONFIG_NO_FORMATTING */

#endif
