/*
******************************************************************************
*
*   Copyright (C) 2009, International Business Machines
*   Corporation and others.  All Rights Reserved.
*
******************************************************************************
*/

#include "unicode/utypes.h"
#include "unicode/icudataver.h"
#include "unicode/uversion.h"
#include "unicode/ures.h"
#include "uresimp.h" /* for ures_getVersionByKey */
#include "cmemory.h"

/*
 * Determines if icustd is in the data.
 */
static UBool hasICUSTDBundle();

static UBool hasICUSTDBundle() {
    UErrorCode status = U_ZERO_ERROR;
    UBool result = TRUE;
    
    UResourceBundle *icustdbundle = ures_openDirect(NULL, U_ICU_STD_BUNDLE, &status);
    if (U_SUCCESS(status)) {
        result = TRUE;
    } else {
        result = FALSE;
    }
    
    ures_close(icustdbundle);
    
    return result;
}

U_CAPI void U_EXPORT2 u_getDataVersion(UVersionInfo dataVersionFillin, UErrorCode *status) {
    UResourceBundle *icudatares = NULL;
    
    if (U_FAILURE(*status)) {
        return;
    }
    
    if (dataVersionFillin != NULL) {
        icudatares = ures_openDirect(NULL, U_ICU_VERSION_BUNDLE , status);
        if (U_SUCCESS(*status)) {
            ures_getVersionByKey(icudatares, U_ICU_DATA_KEY, dataVersionFillin, status);
        }
        ures_close(icudatares);
    }
}

U_CAPI UBool U_EXPORT2 u_isDataOlder(UVersionInfo dataVersionFillin, UBool *isModifiedFillin, UErrorCode *status) {
    UBool result = TRUE;
    UVersionInfo dataVersion;
    UVersionInfo wiredVersion;
    
    if (U_FAILURE(*status)) {
        return result;
    }
    
    u_getDataVersion(dataVersion, status);
    if (U_SUCCESS(*status)) {
        u_versionFromString(wiredVersion, U_ICU_DATA_VERSION);
        
        if (uprv_memcmp(dataVersion, wiredVersion, sizeof(UVersionInfo)) >= 0) {
            result = FALSE;
        }
        
        if (dataVersionFillin != NULL) {
            uprv_memcpy(dataVersionFillin, dataVersion, sizeof(UVersionInfo));
        }
        
        if (hasICUSTDBundle()) {
            *isModifiedFillin = FALSE;
        } else {
            *isModifiedFillin = TRUE;
        }
    }
    
    return result;
}
