/*
********************************************************************************
*   Copyright (C) 2005-2006, International Business Machines
*   Corporation and others.  All Rights Reserved.
********************************************************************************
*
* File WINNMTST.H
*
********************************************************************************
*/

#ifndef __WINNMTST
#define __WINNMTST

#include "unicode/utypes.h"

#ifdef U_WINDOWS

#if !UCONFIG_NO_FORMATTING

/**
 * \file 
 * \brief C++ API: Format dates using Windows API.
 */

class TestLog;

class Win32NumberTest
{
public:
    static void testLocales(TestLog *log);

private:
    Win32NumberTest();
};

#endif /* #if !UCONFIG_NO_FORMATTING */

#endif // #ifdef U_WINDOWS

#endif // __WINNMTST
