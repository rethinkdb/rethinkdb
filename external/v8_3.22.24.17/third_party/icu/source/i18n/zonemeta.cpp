/*
*******************************************************************************
* Copyright (C) 2007-2010, International Business Machines Corporation and    *
* others. All Rights Reserved.                                                *
*******************************************************************************
*/

#include "unicode/utypes.h"

#if !UCONFIG_NO_FORMATTING

#include "zonemeta.h"

#include "unicode/timezone.h"
#include "unicode/ustring.h"
#include "unicode/putil.h"

#include "umutex.h"
#include "uvector.h"
#include "cmemory.h"
#include "gregoimp.h"
#include "cstring.h"
#include "ucln_in.h"
#include "uassert.h"

static UMTX gZoneMetaLock = NULL;

// Metazone mapping table
static UHashtable *gOlsonToMeta = NULL;
static UBool gOlsonToMetaInitialized = FALSE;

// Country info vectors
static U_NAMESPACE_QUALIFIER UVector *gSingleZoneCountries = NULL;
static U_NAMESPACE_QUALIFIER UVector *gMultiZonesCountries = NULL;
static UBool gCountryInfoVectorsInitialized = FALSE;

U_CDECL_BEGIN


/**
 * Cleanup callback func
 */
static UBool U_CALLCONV zoneMeta_cleanup(void)
{
     umtx_destroy(&gZoneMetaLock);

    if (gOlsonToMeta != NULL) {
        uhash_close(gOlsonToMeta);
        gOlsonToMeta = NULL;
    }
    gOlsonToMetaInitialized = FALSE;

    delete gSingleZoneCountries;
    delete gMultiZonesCountries;
    gCountryInfoVectorsInitialized = FALSE;

    return TRUE;
}

/**
 * Deleter for UChar* string
 */
static void U_CALLCONV
deleteUCharString(void *obj) {
    UChar *entry = (UChar*)obj;
    uprv_free(entry);
}

/**
 * Deleter for UVector
 */
static void U_CALLCONV
deleteUVector(void *obj) {
   delete (U_NAMESPACE_QUALIFIER UVector*) obj;
}

/**
 * Deleter for OlsonToMetaMappingEntry
 */
static void U_CALLCONV
deleteOlsonToMetaMappingEntry(void *obj) {
    U_NAMESPACE_QUALIFIER OlsonToMetaMappingEntry *entry = (U_NAMESPACE_QUALIFIER OlsonToMetaMappingEntry*)obj;
    uprv_free(entry);
}

U_CDECL_END

U_NAMESPACE_BEGIN

#define ZID_KEY_MAX 128

static const char gMetaZones[]          = "metaZones";
static const char gMetazoneInfo[]       = "metazoneInfo";
static const char gMapTimezonesTag[]    = "mapTimezones";

static const char gTimeZoneTypes[]      = "timezoneTypes";
static const char gTypeAliasTag[]       = "typeAlias";
static const char gTypeMapTag[]         = "typeMap";
static const char gTimezoneTag[]        = "timezone";

static const char gWorldTag[]           = "001";

static const UChar gWorld[] = {0x30, 0x30, 0x31, 0x00}; // "001"

static const UChar gDefaultFrom[] = {0x31, 0x39, 0x37, 0x30, 0x2D, 0x30, 0x31, 0x2D, 0x30, 0x31,
                                     0x20, 0x30, 0x30, 0x3A, 0x30, 0x30, 0x00}; // "1970-01-01 00:00"
static const UChar gDefaultTo[]   = {0x39, 0x39, 0x39, 0x39, 0x2D, 0x31, 0x32, 0x2D, 0x33, 0x31,
                                     0x20, 0x32, 0x33, 0x3A, 0x35, 0x39, 0x00}; // "9999-12-31 23:59"

#define ASCII_DIGIT(c) (((c)>=0x30 && (c)<=0x39) ? (c)-0x30 : -1)

/*
 * Convert a date string used by metazone mappings to UDate.
 * The format used by CLDR metazone mapping is "yyyy-MM-dd HH:mm".
 */
static UDate
parseDate (const UChar *text, UErrorCode &status) {
    if (U_FAILURE(status)) {
        return 0;
    }
    int32_t len = u_strlen(text);
    if (len != 16 && len != 10) {
        // It must be yyyy-MM-dd HH:mm (length 16) or yyyy-MM-dd (length 10)
        status = U_INVALID_FORMAT_ERROR;
        return 0;
    }

    int32_t year = 0, month = 0, day = 0, hour = 0, min = 0, n;
    int32_t idx;

    // "yyyy" (0 - 3)
    for (idx = 0; idx <= 3 && U_SUCCESS(status); idx++) {
        n = ASCII_DIGIT((int32_t)text[idx]);
        if (n >= 0) {
            year = 10*year + n;
        } else {
            status = U_INVALID_FORMAT_ERROR;
        }
    }
    // "MM" (5 - 6)
    for (idx = 5; idx <= 6 && U_SUCCESS(status); idx++) {
        n = ASCII_DIGIT((int32_t)text[idx]);
        if (n >= 0) {
            month = 10*month + n;
        } else {
            status = U_INVALID_FORMAT_ERROR;
        }
    }
    // "dd" (8 - 9)
    for (idx = 8; idx <= 9 && U_SUCCESS(status); idx++) {
        n = ASCII_DIGIT((int32_t)text[idx]);
        if (n >= 0) {
            day = 10*day + n;
        } else {
            status = U_INVALID_FORMAT_ERROR;
        }
    }
    if (len == 16) {
        // "HH" (11 - 12)
        for (idx = 11; idx <= 12 && U_SUCCESS(status); idx++) {
            n = ASCII_DIGIT((int32_t)text[idx]);
            if (n >= 0) {
                hour = 10*hour + n;
            } else {
                status = U_INVALID_FORMAT_ERROR;
            }
        }
        // "mm" (14 - 15)
        for (idx = 14; idx <= 15 && U_SUCCESS(status); idx++) {
            n = ASCII_DIGIT((int32_t)text[idx]);
            if (n >= 0) {
                min = 10*min + n;
            } else {
                status = U_INVALID_FORMAT_ERROR;
            }
        }
    }

    if (U_SUCCESS(status)) {
        UDate date = Grego::fieldsToDay(year, month - 1, day) * U_MILLIS_PER_DAY
            + hour * U_MILLIS_PER_HOUR + min * U_MILLIS_PER_MINUTE;
        return date;
    }
    return 0;
}

UnicodeString& U_EXPORT2
ZoneMeta::getCanonicalSystemID(const UnicodeString &tzid, UnicodeString &systemID, UErrorCode& status) {
    int32_t len = tzid.length();
    if ( len >= ZID_KEY_MAX ) {
        status = U_ILLEGAL_ARGUMENT_ERROR;
        systemID.remove();
        return systemID;
    }

    char id[ZID_KEY_MAX];
    const UChar* idChars = tzid.getBuffer();

    u_UCharsToChars(idChars,id,len);
    id[len] = (char) 0; // Make sure it is null terminated.

    // replace '/' with ':'
    char *p = id;
    while (*p++) {
        if (*p == '/') {
            *p = ':';
        }
    }


    UErrorCode tmpStatus = U_ZERO_ERROR;
    UResourceBundle *top = ures_openDirect(NULL, gTimeZoneTypes, &tmpStatus);
    UResourceBundle *rb = ures_getByKey(top, gTypeMapTag, NULL, &tmpStatus);
    ures_getByKey(rb, gTimezoneTag, rb, &tmpStatus);
    ures_getByKey(rb, id, rb, &tmpStatus);
    if (U_SUCCESS(tmpStatus)) {
        // direct map found
        systemID.setTo(tzid);
        ures_close(rb);
        ures_close(top);
        return systemID;
    }

    // If a map element not found, then look for an alias
    tmpStatus = U_ZERO_ERROR;
    ures_getByKey(top, gTypeAliasTag, rb, &tmpStatus);
    ures_getByKey(rb, gTimezoneTag, rb, &tmpStatus);
    const UChar *alias = ures_getStringByKey(rb,id,NULL,&tmpStatus);
    if (U_SUCCESS(tmpStatus)) {
        // alias found
        ures_close(rb);
        ures_close(top);
        systemID.setTo(alias);
        return systemID;
    }

    // Dereference the input ID using the tz data
    const UChar *derefer = TimeZone::dereferOlsonLink(tzid);
    if (derefer == NULL) {
        systemID.remove();
        status = U_ILLEGAL_ARGUMENT_ERROR;
    } else {

        len = u_strlen(derefer);
        u_UCharsToChars(derefer,id,len);
        id[len] = (char) 0; // Make sure it is null terminated.

        // replace '/' with ':'
        char *p = id;
        while (*p++) {
            if (*p == '/') {
                *p = ':';
            }
        }

        // If a dereference turned something up then look for an alias.
        // rb still points to the alias table, so we don't have to go looking
        // for it.
        tmpStatus = U_ZERO_ERROR;
        const UChar *alias = ures_getStringByKey(rb,id,NULL,&tmpStatus);
        if (U_SUCCESS(tmpStatus)) {
            // alias found
            systemID.setTo(alias);
        } else {
            systemID.setTo(derefer);
        }
    }

     ures_close(rb);
     ures_close(top);
     return systemID;
}

UnicodeString& U_EXPORT2
ZoneMeta::getCanonicalCountry(const UnicodeString &tzid, UnicodeString &canonicalCountry) {
    const UChar *region = TimeZone::getRegion(tzid);
    if (u_strcmp(gWorld, region) != 0) {
        canonicalCountry.setTo(region, -1);
    } else {
        canonicalCountry.remove();
    }
    return canonicalCountry;
}

UnicodeString& U_EXPORT2
ZoneMeta::getSingleCountry(const UnicodeString &tzid, UnicodeString &country) {
    // Get canonical country for the zone
    const UChar *region = TimeZone::getRegion(tzid);
    if (u_strcmp(gWorld, region) == 0) {
        // special case - "001"
        country.remove();
        return country;
    }

    // Checking the cached results
    UErrorCode status = U_ZERO_ERROR;
    UBool initialized;
    UMTX_CHECK(&gZoneMetaLock, gCountryInfoVectorsInitialized, initialized);
    if (!initialized) {
        // Create empty vectors
        umtx_lock(&gZoneMetaLock);
        {
            if (!gCountryInfoVectorsInitialized) {
                // No deleters for these UVectors, it's a reference to a resource bundle string.
                gSingleZoneCountries = new UVector(NULL, uhash_compareUChars, status);
                if (gSingleZoneCountries == NULL) {
                    status = U_MEMORY_ALLOCATION_ERROR;
                }
                gMultiZonesCountries = new UVector(NULL, uhash_compareUChars, status);
                if (gMultiZonesCountries == NULL) {
                    status = U_MEMORY_ALLOCATION_ERROR;
                }

                if (U_SUCCESS(status)) {
                    gCountryInfoVectorsInitialized = TRUE;
                } else {
                    delete gSingleZoneCountries;
                    delete gMultiZonesCountries;
                }
            }
        }
        umtx_unlock(&gZoneMetaLock);

        if (U_FAILURE(status)) {
            country.remove();
            return country;
        }
    }

    // Check if it was already cached
    UBool cached = FALSE;
    UBool multiZones = FALSE;
    umtx_lock(&gZoneMetaLock);
    {
        multiZones = cached = gMultiZonesCountries->contains((void*)region);
        if (!multiZones) {
            cached = gSingleZoneCountries->contains((void*)region);
        }
    }
    umtx_unlock(&gZoneMetaLock);

    if (!cached) {
        // We need to go through all zones associated with the region.
        // This is relatively heavy operation.

        U_ASSERT(u_strlen(region) == 2);

        char buf[] = {0, 0, 0};
        u_UCharsToChars(region, buf, 2);

        StringEnumeration *ids = TimeZone::createEnumeration(buf);
        int32_t idsLen = ids->count(status);
        if (U_SUCCESS(status) && idsLen > 1) {
            // multiple zones are available for the region
            UnicodeString canonical, tmp;
            const UnicodeString *id = ids->snext(status);
            getCanonicalSystemID(*id, canonical, status);
            if (U_SUCCESS(status)) {
                // check if there are any other canonical zone in the group
                while ((id = ids->snext(status))!=NULL) {
                    getCanonicalSystemID(*id, tmp, status);
                    if (U_FAILURE(status)) {
                        break;
                    }
                    if (canonical != tmp) {
                        // another canonical zone was found
                        multiZones = TRUE;
                        break;
                    }
                }
            }
        }
        if (U_FAILURE(status)) {
            // no single country by default for any error cases
            multiZones = TRUE;
        }
        delete ids;

        // Cache the result
        umtx_lock(&gZoneMetaLock);
        {
            UErrorCode ec = U_ZERO_ERROR;
            if (multiZones) {
                if (!gMultiZonesCountries->contains((void*)region)) {
                    gMultiZonesCountries->addElement((void*)region, ec);
                }
            } else {
                if (!gSingleZoneCountries->contains((void*)region)) {
                    gSingleZoneCountries->addElement((void*)region, ec);
                }
            }
        }
        umtx_unlock(&gZoneMetaLock);
    }

    if (multiZones) {
        country.remove();
    } else {
        country.setTo(region, -1);
    }
    return country;
}

UnicodeString& U_EXPORT2
ZoneMeta::getMetazoneID(const UnicodeString &tzid, UDate date, UnicodeString &result) {
    UBool isSet = FALSE;
    const UVector *mappings = getMetazoneMappings(tzid);
    if (mappings != NULL) {
        for (int32_t i = 0; i < mappings->size(); i++) {
            OlsonToMetaMappingEntry *mzm = (OlsonToMetaMappingEntry*)mappings->elementAt(i);
            if (mzm->from <= date && mzm->to > date) {
                result.setTo(mzm->mzid, -1);
                isSet = TRUE;
                break;
            }
        }
    }
    if (!isSet) {
        result.remove();
    }
    return result;
}

const UVector* U_EXPORT2
ZoneMeta::getMetazoneMappings(const UnicodeString &tzid) {
    UErrorCode status = U_ZERO_ERROR;
    UChar tzidUChars[ZID_KEY_MAX];
    tzid.extract(tzidUChars, ZID_KEY_MAX, status);
    if (U_FAILURE(status) || status == U_STRING_NOT_TERMINATED_WARNING) {
        return NULL;
    }

    UBool initialized;
    UMTX_CHECK(&gZoneMetaLock, gOlsonToMetaInitialized, initialized);
    if (!initialized) {
        UHashtable *tmpOlsonToMeta = uhash_open(uhash_hashUChars, uhash_compareUChars, NULL, &status);
        if (U_FAILURE(status)) {
            return NULL;
        }
        uhash_setKeyDeleter(tmpOlsonToMeta, deleteUCharString);
        uhash_setValueDeleter(tmpOlsonToMeta, deleteUVector);

        umtx_lock(&gZoneMetaLock);
        {
            if (!gOlsonToMetaInitialized) {
                gOlsonToMeta = tmpOlsonToMeta;
                tmpOlsonToMeta = NULL;
                gOlsonToMetaInitialized = TRUE;
            }
        }
        umtx_unlock(&gZoneMetaLock);

        // OK to call the following multiple times with the same function
        ucln_i18n_registerCleanup(UCLN_I18N_ZONEMETA, zoneMeta_cleanup);
        if (tmpOlsonToMeta != NULL) {
            uhash_close(tmpOlsonToMeta);
        }
    }

    // get the mapping from cache
    const UVector *result = NULL;

    umtx_lock(&gZoneMetaLock);
    {
        result = (UVector*) uhash_get(gOlsonToMeta, tzidUChars);
    }
    umtx_unlock(&gZoneMetaLock);

    if (result != NULL) {
        return result;
    }

    // miss the cache - create new one
    UVector *tmpResult = createMetazoneMappings(tzid);
    if (tmpResult == NULL) {
        // not available
        return NULL;
    }

    // put the new one into the cache
    umtx_lock(&gZoneMetaLock);
    {
        // make sure it's already created
        result = (UVector*) uhash_get(gOlsonToMeta, tzidUChars);
        if (result == NULL) {
            // add the one just created
            int32_t tzidLen = tzid.length() + 1;
            UChar *key = (UChar*)uprv_malloc(tzidLen * sizeof(UChar));
            if (key == NULL) {
                // memory allocation error..  just return NULL
                result = NULL;
                delete tmpResult;
            } else {
                tzid.extract(key, tzidLen, status);
                uhash_put(gOlsonToMeta, key, tmpResult, &status);
                if (U_FAILURE(status)) {
                    // delete the mapping
                    result = NULL;
                    delete tmpResult;
                } else {
                    result = tmpResult;
                }
            }
        } else {
            // another thread already put the one
            delete tmpResult;
        }
    }
    umtx_unlock(&gZoneMetaLock);

    return result;
}

UVector*
ZoneMeta::createMetazoneMappings(const UnicodeString &tzid) {
    UVector *mzMappings = NULL;
    UErrorCode status = U_ZERO_ERROR;

    UnicodeString canonicalID;
    UResourceBundle *rb = ures_openDirect(NULL, gMetaZones, &status);
    ures_getByKey(rb, gMetazoneInfo, rb, &status);
    TimeZone::getCanonicalID(tzid, canonicalID, status);

    if (U_SUCCESS(status)) {
        char tzKey[ZID_KEY_MAX];
        canonicalID.extract(0, canonicalID.length(), tzKey, sizeof(tzKey), US_INV);

        // tzid keys are using ':' as separators
        char *p = tzKey;
        while (*p) {
            if (*p == '/') {
                *p = ':';
            }
            p++;
        }

        ures_getByKey(rb, tzKey, rb, &status);

        if (U_SUCCESS(status)) {
            UResourceBundle *mz = NULL;
            while (ures_hasNext(rb)) {
                mz = ures_getNextResource(rb, mz, &status);

                const UChar *mz_name = ures_getStringByIndex(mz, 0, NULL, &status);
                const UChar *mz_from = gDefaultFrom;
                const UChar *mz_to = gDefaultTo;

                if (ures_getSize(mz) == 3) {
                    mz_from = ures_getStringByIndex(mz, 1, NULL, &status);
                    mz_to   = ures_getStringByIndex(mz, 2, NULL, &status);
                }

                if(U_FAILURE(status)){
                    status = U_ZERO_ERROR;
                    continue;
                }
                // We do not want to use SimpleDateformat to parse boundary dates,
                // because this code could be triggered by the initialization code
                // used by SimpleDateFormat.
                UDate from = parseDate(mz_from, status);
                UDate to = parseDate(mz_to, status);
                if (U_FAILURE(status)) {
                    status = U_ZERO_ERROR;
                    continue;
                }

                OlsonToMetaMappingEntry *entry = (OlsonToMetaMappingEntry*)uprv_malloc(sizeof(OlsonToMetaMappingEntry));
                if (entry == NULL) {
                    status = U_MEMORY_ALLOCATION_ERROR;
                    break;
                }
                entry->mzid = mz_name;
                entry->from = from;
                entry->to = to;

                if (mzMappings == NULL) {
                    mzMappings = new UVector(deleteOlsonToMetaMappingEntry, NULL, status);
                    if (U_FAILURE(status)) {
                        delete mzMappings;
                        deleteOlsonToMetaMappingEntry(entry);
                        uprv_free(entry);
                        break;
                    }
                }

                mzMappings->addElement(entry, status);
                if (U_FAILURE(status)) {
                    break;
                }
            }
            ures_close(mz);
            if (U_FAILURE(status)) {
                if (mzMappings != NULL) {
                    delete mzMappings;
                    mzMappings = NULL;
                }
            }
        }
    }
    ures_close(rb);
    return mzMappings;
}

UnicodeString& U_EXPORT2
ZoneMeta::getZoneIdByMetazone(const UnicodeString &mzid, const UnicodeString &region, UnicodeString &result) {
    UErrorCode status = U_ZERO_ERROR;
    const UChar *tzid = NULL;
    int32_t tzidLen = 0;
    char keyBuf[ZID_KEY_MAX + 1];
    int32_t keyLen = 0;

    if (mzid.length() >= ZID_KEY_MAX) {
        result.remove();
        return result;
    }

    keyLen = mzid.extract(0, mzid.length(), keyBuf, ZID_KEY_MAX, US_INV);

    UResourceBundle *rb = ures_openDirect(NULL, gMetaZones, &status);
    ures_getByKey(rb, gMapTimezonesTag, rb, &status);
    ures_getByKey(rb, keyBuf, rb, &status);

    if (U_SUCCESS(status)) {
        // check region mapping
        if (region.length() == 2 || region.length() == 3) {
            region.extract(0, region.length(), keyBuf, ZID_KEY_MAX, US_INV);
            tzid = ures_getStringByKey(rb, keyBuf, &tzidLen, &status);
            if (status == U_MISSING_RESOURCE_ERROR) {
                status = U_ZERO_ERROR;
            }
        }
        if (U_SUCCESS(status) && tzid == NULL) {
            // try "001"
            tzid = ures_getStringByKey(rb, gWorldTag, &tzidLen, &status);
        }
    }
    ures_close(rb);

    if (tzid == NULL) {
        result.remove();
    } else {
        result.setTo(tzid, tzidLen);
    }

    return result;
}

U_NAMESPACE_END

#endif /* #if !UCONFIG_NO_FORMATTING */
