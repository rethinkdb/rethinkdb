/*
******************************************************************************
*                                                                            *
* Copyright (C) 2001-2009, International Business Machines                   *
*                Corporation and others. All Rights Reserved.                *
*                                                                            *
******************************************************************************
*   file name:  ucln_io.c
*   encoding:   US-ASCII
*   tab size:   8 (not used)
*   indentation:4
*
*   created on: 2006August11
*   created by: George Rhoten
*/

#include "ucln.h"
#include "ucln_io.h"
#include "umutex.h"
#include "uassert.h"

/**  Auto-client */
#define UCLN_TYPE UCLN_IO
#include "ucln_imp.h"

/* Leave this copyright notice here! It needs to go somewhere in this library. */
static const char copyright[] = U_COPYRIGHT_STRING;

static cleanupFunc *gCleanupFunctions[UCLN_IO_COUNT];

static UBool io_cleanup(void)
{
    ECleanupIOType libType = UCLN_IO_START;

    while (++libType<UCLN_IO_COUNT) {
        if (gCleanupFunctions[libType])
        {
            gCleanupFunctions[libType]();
            gCleanupFunctions[libType] = NULL;
        }
    }
#if !UCLN_NO_AUTO_CLEANUP && (defined(UCLN_AUTO_ATEXIT) || defined(UCLN_AUTO_LOCAL))
    ucln_unRegisterAutomaticCleanup();
#endif
    return TRUE;
}

void ucln_io_registerCleanup(ECleanupIOType type,
                               cleanupFunc *func)
{
    U_ASSERT(UCLN_IO_START < type && type < UCLN_IO_COUNT);
    ucln_registerCleanup(UCLN_IO, io_cleanup);
    if (UCLN_IO_START < type && type < UCLN_IO_COUNT)
    {
        gCleanupFunctions[type] = func;
    }

#if !UCLN_NO_AUTO_CLEANUP && (defined(UCLN_AUTO_ATEXIT) || defined(UCLN_AUTO_LOCAL))
    ucln_registerAutomaticCleanup();
#endif
}

