/*
******************************************************************************
*                                                                            *
* Copyright (C) 2001-2009, International Business Machines                   *
*                Corporation and others. All Rights Reserved.                *
*                                                                            *
******************************************************************************
*   file name:  ucln_cmn.h
*   encoding:   US-ASCII
*   tab size:   8 (not used)
*   indentation:4
*
*   created on: 2001July05
*   created by: George Rhoten
*/

#ifndef __UCLN_CMN_H__
#define __UCLN_CMN_H__

#include "unicode/utypes.h"
#include "ucln.h"

/*
Please keep the order of enums declared in same order
as the functions are suppose to be called.
It's usually best to have child dependencies called first. */
typedef enum ECleanupI18NType {
    UCLN_I18N_START = -1,
    UCLN_I18N_TRANSLITERATOR,
    UCLN_I18N_REGEX,
    UCLN_I18N_ISLAMIC_CALENDAR,
    UCLN_I18N_CHINESE_CALENDAR,
    UCLN_I18N_HEBREW_CALENDAR,
    UCLN_I18N_ASTRO_CALENDAR,
    UCLN_I18N_CALENDAR,
    UCLN_I18N_ZONEMETA,
    UCLN_I18N_ZSFORMAT,
    UCLN_I18N_TIMEZONE,
    UCLN_I18N_PLURAL_RULE,
    UCLN_I18N_CURRENCY,
    UCLN_I18N_NUMFMT,
    UCLN_I18N_USEARCH,
    UCLN_I18N_COLLATOR,
    UCLN_I18N_UCOL,
    UCLN_I18N_UCOL_RES,
    UCLN_I18N_UCOL_BLD,
    UCLN_I18N_CSDET,
    UCLN_I18N_COLL_DATA,
    UCLN_I18N_COUNT /* This must be last */
} ECleanupI18NType;

/* Main library cleanup registration function. */
/* See common/ucln.h for details on adding a cleanup function. */
U_CFUNC void U_EXPORT2 ucln_i18n_registerCleanup(ECleanupI18NType type,
                                                 cleanupFunc *func);

U_CFUNC UBool utrans_transliterator_cleanup(void);

#endif
