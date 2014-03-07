/*
********************************************************************************
*   Copyright (C) 2005-2009, International Business Machines
*   Corporation and others.  All Rights Reserved.
********************************************************************************
*
* File WINUTIL.H
*
********************************************************************************
*/

#ifndef __WINUTIL
#define __WINUTIL

#include "unicode/utypes.h"

#ifdef U_WINDOWS

#if !UCONFIG_NO_FORMATTING

/**
 * \file 
 * \brief C++ API: Format dates using Windows API.
 */

class Win32Utilities
{
public:
    struct LCIDRecord
    {
        int32_t lcid;
        char *localeID;
    };

    static LCIDRecord *getLocales(int32_t &localeCount);
    static void freeLocales(LCIDRecord *records);

private:
    Win32Utilities();
};

#endif /* #if !UCONFIG_NO_FORMATTING */

#endif // #ifdef U_WINDOWS

#endif // __WINUTIL
