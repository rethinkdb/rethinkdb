/*
*******************************************************************************
* Copyright (C) 2007-2010, International Business Machines Corporation and    *
* others. All Rights Reserved.                                                *
*******************************************************************************
*/
#ifndef ZONEMETA_H
#define ZONEMETA_H

#include "unicode/utypes.h"

#if !UCONFIG_NO_FORMATTING

#include "unicode/unistr.h"
#include "hash.h"

U_NAMESPACE_BEGIN

typedef struct OlsonToMetaMappingEntry {
    const UChar *mzid; // const because it's a reference to a resource bundle string.
    UDate from;
    UDate to;
} OlsonToMetaMappingEntry;

class UVector;

class U_I18N_API ZoneMeta {
public:
    /**
     * Return the canonical id for this system tzid, which might be the id itself.
     * If the given system tzid is not know, U_ILLEGAL_ARGUMENT_ERROR is set in the status.
     */
    static UnicodeString& U_EXPORT2 getCanonicalSystemID(const UnicodeString &tzid, UnicodeString &systemID, UErrorCode& status);

    /**
     * Return the canonical country code for this tzid.  If we have none, or if the time zone
     * is not associated with a country, return null.
     */
    static UnicodeString& U_EXPORT2 getCanonicalCountry(const UnicodeString &tzid, UnicodeString &canonicalCountry);

    /**
     * Return the country code if this is a 'single' time zone that can fallback to just
     * the country, otherwise return empty string.  (Note, one must also check the locale data
     * to see that there is a localization for the country in order to implement
     * tr#35 appendix J step 5.)
     */
    static UnicodeString& U_EXPORT2 getSingleCountry(const UnicodeString &tzid, UnicodeString &country);

    /**
     * Returns a CLDR metazone ID for the given Olson tzid and time.
     */
    static UnicodeString& U_EXPORT2 getMetazoneID(const UnicodeString &tzid, UDate date, UnicodeString &result);
    /**
     * Returns an Olson ID for the ginve metazone and region
     */
    static UnicodeString& U_EXPORT2 getZoneIdByMetazone(const UnicodeString &mzid, const UnicodeString &region, UnicodeString &result);

    static const UVector* U_EXPORT2 getMetazoneMappings(const UnicodeString &tzid);

private:
    ZoneMeta(); // Prevent construction.
    static UVector* createMetazoneMappings(const UnicodeString &tzid);
};

U_NAMESPACE_END

#endif /* #if !UCONFIG_NO_FORMATTING */
#endif // ZONEMETA_H
