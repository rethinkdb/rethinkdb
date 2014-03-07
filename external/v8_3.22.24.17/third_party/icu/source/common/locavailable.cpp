/*
*******************************************************************************
*
*   Copyright (C) 1997-2010, International Business Machines
*   Corporation and others.  All Rights Reserved.
*
*******************************************************************************
*   file name:  locavailable.cpp
*   encoding:   US-ASCII
*   tab size:   8 (not used)
*   indentation:4
*
*   created on: 2010feb25
*   created by: Markus W. Scherer
*
*   Code for available locales, separated out from other .cpp files
*   that then do not depend on resource bundle code and res_index bundles.
*/

#include "unicode/utypes.h"
#include "unicode/locid.h"
#include "unicode/uloc.h"
#include "unicode/ures.h"
#include "cmemory.h"
#include "ucln_cmn.h"
#include "umutex.h"
#include "uresimp.h"

// C++ API ----------------------------------------------------------------- ***

static U_NAMESPACE_QUALIFIER Locale*  availableLocaleList = NULL;
static int32_t  availableLocaleListCount;

U_CDECL_BEGIN

static UBool U_CALLCONV locale_available_cleanup(void)
{
    U_NAMESPACE_USE

    if (availableLocaleList) {
        delete []availableLocaleList;
        availableLocaleList = NULL;
    }
    availableLocaleListCount = 0;

    return TRUE;
}

U_CDECL_END

U_NAMESPACE_BEGIN

const Locale* U_EXPORT2
Locale::getAvailableLocales(int32_t& count)
{
    // for now, there is a hardcoded list, so just walk through that list and set it up.
    UBool needInit;
    UMTX_CHECK(NULL, availableLocaleList == NULL, needInit);

    if (needInit) {
        int32_t locCount = uloc_countAvailable();
        Locale *newLocaleList = 0;
        if(locCount) {
           newLocaleList = new Locale[locCount];
        }
        if (newLocaleList == NULL) {
            count = 0;
            return NULL;
        }

        count = locCount;

        while(--locCount >= 0) {
            newLocaleList[locCount].setFromPOSIXID(uloc_getAvailable(locCount));
        }

        umtx_lock(NULL);
        if(availableLocaleList == 0) {
            availableLocaleListCount = count;
            availableLocaleList = newLocaleList;
            newLocaleList = NULL;
            ucln_common_registerCleanup(UCLN_COMMON_LOCALE_AVAILABLE, locale_available_cleanup);
        }
        umtx_unlock(NULL);
        delete []newLocaleList;
    }
    count = availableLocaleListCount;
    return availableLocaleList;
}


U_NAMESPACE_END

// C API ------------------------------------------------------------------- ***

U_NAMESPACE_USE

/* ### Constants **************************************************/

/* These strings describe the resources we attempt to load from
 the locale ResourceBundle data file.*/
static const char _kIndexLocaleName[] = "res_index";
static const char _kIndexTag[]        = "InstalledLocales";

static char** _installedLocales = NULL;
static int32_t _installedLocalesCount = 0;

/* ### Get available **************************************************/

static UBool U_CALLCONV uloc_cleanup(void) {
    char ** temp;

    if (_installedLocales) {
        temp = _installedLocales;
        _installedLocales = NULL;

        _installedLocalesCount = 0;

        uprv_free(temp);
    }
    return TRUE;
}

static void _load_installedLocales()
{
    UBool   localesLoaded;

    UMTX_CHECK(NULL, _installedLocales != NULL, localesLoaded);
    
    if (localesLoaded == FALSE) {
        UResourceBundle *indexLocale = NULL;
        UResourceBundle installed;
        UErrorCode status = U_ZERO_ERROR;
        char ** temp;
        int32_t i = 0;
        int32_t localeCount;
        
        ures_initStackObject(&installed);
        indexLocale = ures_openDirect(NULL, _kIndexLocaleName, &status);
        ures_getByKey(indexLocale, _kIndexTag, &installed, &status);
        
        if(U_SUCCESS(status)) {
            localeCount = ures_getSize(&installed);
            temp = (char **) uprv_malloc(sizeof(char*) * (localeCount+1));
            /* Check for null pointer */
            if (temp != NULL) {
                ures_resetIterator(&installed);
                while(ures_hasNext(&installed)) {
                    ures_getNextString(&installed, NULL, (const char **)&temp[i++], &status);
                }
                temp[i] = NULL;

                umtx_lock(NULL);
                if (_installedLocales == NULL)
                {
                    _installedLocalesCount = localeCount;
                    _installedLocales = temp;
                    temp = NULL;
                    ucln_common_registerCleanup(UCLN_COMMON_ULOC, uloc_cleanup);
                } 
                umtx_unlock(NULL);

                uprv_free(temp);
            }
        }
        ures_close(&installed);
        ures_close(indexLocale);
    }
}

U_CAPI const char* U_EXPORT2
uloc_getAvailable(int32_t offset) 
{
    
    _load_installedLocales();
    
    if (offset > _installedLocalesCount)
        return NULL;
    return _installedLocales[offset];
}

U_CAPI int32_t  U_EXPORT2
uloc_countAvailable()
{
    _load_installedLocales();
    return _installedLocalesCount;
}
