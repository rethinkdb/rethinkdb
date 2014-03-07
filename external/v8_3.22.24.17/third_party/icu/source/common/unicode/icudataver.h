/*
******************************************************************************
*
*   Copyright (C) 2009-2010, International Business Machines
*   Corporation and others.  All Rights Reserved.
*
******************************************************************************
*/

#ifndef __ICU_DATA_VER_H__
#define __ICU_DATA_VER_H__

#include "unicode/utypes.h"

/**
 * @internal ICU 4.4
 */
#define U_ICU_VERSION_BUNDLE "icuver"

/**
 * @internal ICU 4.4
 */
#define U_ICU_STD_BUNDLE "icustd"

/**
 * @internal ICU 4.4
 */
#define U_ICU_DATA_KEY "DataVersion"

/**
 * This function loads up icuver and compares the data version to the wired-in U_ICU_DATA_VERSION.
 * If icuver shows something less than U_ICU_DATA_VERSION it returns TRUE, else FALSE. The version
 * found will be returned in the first fillin parameter (if non-null), and *isModified will be set
 * to TRUE if "icustd" is NOT found. Thus, if the data has been repackaged or modified, "icustd"
 * (standard ICU) will be missing, and the function will alert the caller that the data is not standard.
 * 
 * @param dataVersionFillin icuver data version information to be filled in if not-null
 * @param isModifiedFillin if the data is not standard if not-null
 * @param status stores the error code from the calls to resource bundle
 *
 * @return TRUE if U_ICU_DATA_VERSION is newer than icuver, else FALSE
 * 
 * @internal ICU 4.4
 */
U_INTERNAL UBool U_EXPORT2 u_isDataOlder(UVersionInfo dataVersionFillin, UBool *isModifiedFillin, UErrorCode *status);

/**
 * Retrieves the data version from icuver and stores it in dataVersionFillin.
 * 
 * @param dataVersionFillin icuver data version information to be filled in if not-null
 * @param status stores the error code from the calls to resource bundle
 * 
 * @internal ICU 4.4
 */
U_INTERNAL void U_EXPORT2 u_getDataVersion(UVersionInfo dataVersionFillin, UErrorCode *status);

#endif
