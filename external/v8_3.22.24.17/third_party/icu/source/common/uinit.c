/*
******************************************************************************
*                                                                            *
* Copyright (C) 2001-2010, International Business Machines                   *
*                Corporation and others. All Rights Reserved.                *
*                                                                            *
******************************************************************************
*   file name:  uinit.c
*   encoding:   US-ASCII
*   tab size:   8 (not used)
*   indentation:4
*
*   created on: 2001July05
*   created by: George Rhoten
*/

#include "unicode/utypes.h"
#include "unicode/icuplug.h"
#include "unicode/uclean.h"
#include "cmemory.h"
#include "icuplugimp.h"
#include "uassert.h"
#include "ucln.h"
#include "ucln_cmn.h"
#include "ucnv_io.h"
#include "umutex.h"
#include "utracimp.h"

static UBool gICUInitialized = FALSE;
static UMTX  gICUInitMutex   = NULL;


/************************************************
 The cleanup order is important in this function.
 Please be sure that you have read ucln.h
 ************************************************/
U_CAPI void U_EXPORT2
u_cleanup(void)
{
    UTRACE_ENTRY_OC(UTRACE_U_CLEANUP);
    umtx_lock(NULL);     /* Force a memory barrier, so that we are sure to see   */
    umtx_unlock(NULL);   /*   all state left around by any other threads.        */

    ucln_lib_cleanup();

    umtx_destroy(&gICUInitMutex);
    umtx_cleanup();
    cmemory_cleanup();       /* undo any heap functions set by u_setMemoryFunctions(). */
    gICUInitialized = FALSE;
    UTRACE_EXIT();           /* Must be before utrace_cleanup(), which turns off tracing. */
/*#if U_ENABLE_TRACING*/
    utrace_cleanup();
/*#endif*/
}

/*
 * ICU Initialization Function. Need not be called.
 */
U_CAPI void U_EXPORT2
u_init(UErrorCode *status) {
    UTRACE_ENTRY_OC(UTRACE_U_INIT);
    /* initialize plugins */
    uplug_init(status);

    umtx_lock(&gICUInitMutex);
    if (gICUInitialized || U_FAILURE(*status)) {
        umtx_unlock(&gICUInitMutex);
        UTRACE_EXIT_STATUS(*status);
        return;
    }

    /*
     * 2005-may-02
     *
     * ICU4C 3.4 (jitterbug 4497) hardcodes the data for Unicode character
     * properties for APIs that want to be fast.
     * Therefore, we need not load them here nor check for errors.
     * Instead, we load the converter alias table to see if any ICU data
     * is available.
     * Users should really open the service objects they need and check
     * for errors there, to make sure that the actual items they need are
     * available.
     */
#if !UCONFIG_NO_CONVERSION
    ucnv_io_countKnownConverters(status);
#endif

    gICUInitialized = TRUE;    /* TODO:  don't set if U_FAILURE? */
    umtx_unlock(&gICUInitMutex);
    UTRACE_EXIT_STATUS(*status);
}
