/********************************************************************
 * COPYRIGHT: 
 * Copyright (c) 1997-2003, International Business Machines Corporation and
 * others. All Rights Reserved.
 ********************************************************************/

#include "ucol_imp.h"

#if !UCONFIG_NO_COLLATION

int TestBufferSize(void); /* keep gcc happy */

int TestBufferSize(void)
{
    return (U_COL_SAFECLONE_BUFFERSIZE < sizeof(UCollator));
}
#endif
