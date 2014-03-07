/*
********************************************************************************
*   Copyright (C) 2005-2006, International Business Machines
*   Corporation and others.  All Rights Reserved.
********************************************************************************
*
* File WINDTTST.H
*
********************************************************************************
*/

#ifndef __WINDTTST
#define __WINDTTST

#include "unicode/utypes.h"

#ifdef U_WINDOWS

#if !UCONFIG_NO_FORMATTING

/**
 * \file 
 * \brief C++ API: Format dates using Windows API.
 */

class TestLog;

class Win32DateTimeTest
{
public:
    static void testLocales(TestLog *log);

private:
    Win32DateTimeTest();
};

#endif /* #if !UCONFIG_NO_FORMATTING */

#endif // #ifdef U_WINDOWS

#endif // __WINDTTST
