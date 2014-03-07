/*
**********************************************************************
* Copyright (c) 2004, International Business Machines
* Corporation and others.  All Rights Reserved.
**********************************************************************
* Author: Alan Liu
* Created: January 16 2004
* Since: ICU 2.8
**********************************************************************
*/
#include "locbased.h"
#include "cstring.h"

U_NAMESPACE_BEGIN

Locale LocaleBased::getLocale(ULocDataLocaleType type, UErrorCode& status) const {
    const char* id = getLocaleID(type, status);
    return Locale((id != 0) ? id : "");
}

const char* LocaleBased::getLocaleID(ULocDataLocaleType type, UErrorCode& status) const {
    if (U_FAILURE(status)) {
        return NULL;
    }

    switch(type) {
    case ULOC_VALID_LOCALE:
        return valid;
    case ULOC_ACTUAL_LOCALE:
        return actual;
    default:
        status = U_ILLEGAL_ARGUMENT_ERROR;
        return NULL;
    }
}

void LocaleBased::setLocaleIDs(const char* validID, const char* actualID) {
    if (validID != 0) {
        uprv_strcpy(valid, validID);
    }
    if (actualID != 0) {
        uprv_strcpy(actual, actualID);
    }
}

U_NAMESPACE_END
