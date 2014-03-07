/*
******************************************************************************
* Copyright (C) 2001-2010, International Business Machines
*                Corporation and others. All Rights Reserved.
******************************************************************************
*   file name:  ucln_cmn.c
*   encoding:   US-ASCII
*   tab size:   8 (not used)
*   indentation:4
*
*   created on: 2001July05
*   created by: George Rhoten
*/

#include "unicode/utypes.h"
#include "unicode/uclean.h"
#include "utracimp.h"
#include "ustr_imp.h"
#include "ucln_cmn.h"
#include "umutex.h"
#include "ucln.h"
#include "cmemory.h"
#include "uassert.h"

/**  Auto-client for UCLN_COMMON **/
#define UCLN_TYPE UCLN_COMMON
#include "ucln_imp.h"

static cleanupFunc *gCommonCleanupFunctions[UCLN_COMMON_COUNT];
static cleanupFunc *gLibCleanupFunctions[UCLN_COMMON];


/* Enables debugging information about when a library is cleaned up. */
#ifndef UCLN_DEBUG_CLEANUP
#define UCLN_DEBUG_CLEANUP 0
#endif


#if defined(UCLN_DEBUG_CLEANUP)
#include <stdio.h>
#endif

static void ucln_cleanup_internal(ECleanupLibraryType libType) 
{
    if (gLibCleanupFunctions[libType])
    {
        gLibCleanupFunctions[libType]();
        gLibCleanupFunctions[libType] = NULL;
    }
}

U_CAPI void U_EXPORT2 ucln_cleanupOne(ECleanupLibraryType libType)
{
    if(libType==UCLN_COMMON) {
#if UCLN_DEBUG_CLEANUP
        fprintf(stderr, "Cleaning up: UCLN_COMMON with u_cleanup, type %d\n", (int)libType);
#endif
        u_cleanup();
    } else {
#if UCLN_DEBUG_CLEANUP
        fprintf(stderr, "Cleaning up: using ucln_cleanup_internal, type %d\n", (int)libType);
#endif
        ucln_cleanup_internal(libType);
    }
}


U_CFUNC void
ucln_common_registerCleanup(ECleanupCommonType type,
                            cleanupFunc *func)
{
    U_ASSERT(UCLN_COMMON_START < type && type < UCLN_COMMON_COUNT);
    if (UCLN_COMMON_START < type && type < UCLN_COMMON_COUNT)
    {
        gCommonCleanupFunctions[type] = func;
    }
#if !UCLN_NO_AUTO_CLEANUP && (defined(UCLN_AUTO_ATEXIT) || defined(UCLN_AUTO_LOCAL))
    ucln_registerAutomaticCleanup();
#endif
}

U_CAPI void U_EXPORT2
ucln_registerCleanup(ECleanupLibraryType type,
                     cleanupFunc *func)
{
    U_ASSERT(UCLN_START < type && type < UCLN_COMMON);
    if (UCLN_START < type && type < UCLN_COMMON)
    {
        gLibCleanupFunctions[type] = func;
    }
}

U_CFUNC UBool ucln_lib_cleanup(void) {
    ECleanupLibraryType libType = UCLN_START;
    ECleanupCommonType commonFunc = UCLN_COMMON_START;

    for (libType++; libType<UCLN_COMMON; libType++) {
        ucln_cleanup_internal(libType);
    }

    for (commonFunc++; commonFunc<UCLN_COMMON_COUNT; commonFunc++) {
        if (gCommonCleanupFunctions[commonFunc])
        {
            gCommonCleanupFunctions[commonFunc]();
            gCommonCleanupFunctions[commonFunc] = NULL;
        }
    }
#if !UCLN_NO_AUTO_CLEANUP && (defined(UCLN_AUTO_ATEXIT) || defined(UCLN_AUTO_LOCAL))
    ucln_unRegisterAutomaticCleanup();
#endif
    return TRUE;
}
