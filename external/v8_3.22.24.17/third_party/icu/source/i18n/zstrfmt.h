/*
*******************************************************************************
* Copyright (C) 2007-2010, International Business Machines Corporation and    *
* others. All Rights Reserved.                                                *
*******************************************************************************
*/
#ifndef ZSTRFMT_H
#define ZSTRFMT_H

#include "unicode/utypes.h"

#if !UCONFIG_NO_FORMATTING

#include "unicode/unistr.h"
#include "unicode/calendar.h"
#include "uhash.h"
#include "uvector.h"

U_NAMESPACE_BEGIN

/*
 * Character node used by TextTrieMap
 */
struct CharacterNode {
    // No constructor or destructor.
    // We malloc and free an uninitalized array of CharacterNode objects
    // and clear and delete them ourselves.

    void clear();
    void deleteValues();

    void addValue(void *value, UErrorCode &status);
    inline UBool hasValues() const;
    inline int32_t countValues() const;
    inline const void *getValue(int32_t index) const;

    void     *fValues;      // Union of one single value vs. UVector of values.
    UChar    fCharacter;    // UTF-16 code unit.
    uint16_t fFirstChild;   // 0 if no children.
    uint16_t fNextSibling;  // 0 terminates the list.
    UBool    fHasValuesVector;
    UBool    fPadding;

    // No value:   fValues == NULL               and  fHasValuesVector == FALSE
    // One value:  fValues == value              and  fHasValuesVector == FALSE
    // >=2 values: fValues == UVector of values  and  fHasValuesVector == TRUE
};

inline UBool CharacterNode::hasValues() const {
    return (UBool)(fValues != NULL);
}

inline int32_t CharacterNode::countValues() const {
    return
        fValues == NULL ? 0 :
        !fHasValuesVector ? 1 :
        ((const UVector *)fValues)->size();
}

inline const void *CharacterNode::getValue(int32_t index) const {
    if (!fHasValuesVector) {
        return fValues;  // Assume index == 0.
    } else {
        return ((const UVector *)fValues)->elementAt(index);
    }
}

/*
 * Search result handler callback interface used by TextTrieMap search.
 */
class TextTrieMapSearchResultHandler : public UMemory {
public:
    virtual UBool handleMatch(int32_t matchLength,
                              const CharacterNode *node, UErrorCode& status) = 0;
    virtual ~TextTrieMapSearchResultHandler(); //added to avoid warning
};


/*
 * ZSFStringPool   Pool of (UChar *) strings.  Provides for sharing of repeated
 *                 strings within ZoneStringFormats.
 */
struct ZSFStringPoolChunk;
class ZSFStringPool: public UMemory {
  public:
    ZSFStringPool(UErrorCode &status);
    ~ZSFStringPool();

    /* Get the pooled string that is equal to the supplied string s.
     * Copy the string into the pool if it is not already present.
     *
     * Life time of the returned string is that of the pool.
     */
    const UChar *get(const UChar *s, UErrorCode &status);

    /* Get the pooled string that is equal to the supplied string s.
     * Copy the string into the pool if it is not already present.
     */
    const UChar *get(const UnicodeString &s, UErrorCode &status);

    /* Adopt a string into the pool, without copying it.
     * Used for strings from resource bundles, which will persist without copying.
     */
    const UChar *adopt(const UChar *s, UErrorCode &status);

    /* Freeze the string pool.  Discards the hash table that is used
     * for looking up a string.  All pointers to pooled strings remain valid.
     */
    void freeze();

  private:
    ZSFStringPoolChunk   *fChunks;
    UHashtable           *fHash;
};


/**
 * TextTrieMap is a trie implementation for supporting
 * fast prefix match for the string key.
 */
class TextTrieMap : public UMemory {
public:
    TextTrieMap(UBool ignoreCase);
    virtual ~TextTrieMap();

    void put(const UnicodeString &key, void *value, ZSFStringPool &sp, UErrorCode &status);
    void search(const UnicodeString &text, int32_t start,
        TextTrieMapSearchResultHandler *handler, UErrorCode& status) const;
    int32_t isEmpty() const;

private:
    UBool           fIgnoreCase;
    CharacterNode   *fNodes;
    int32_t         fNodesCapacity;
    int32_t         fNodesCount;

    UVector         *fLazyContents;
    UBool           fIsEmpty;      

    UBool growNodes();
    CharacterNode* addChildNode(CharacterNode *parent, UChar c, UErrorCode &status);
    CharacterNode* getChildNode(CharacterNode *parent, UChar c) const;

    void putImpl(const UnicodeString &key, void *value, UErrorCode &status);
    void buildTrie(UErrorCode &status);
    void search(CharacterNode *node, const UnicodeString &text, int32_t start,
        int32_t index, TextTrieMapSearchResultHandler *handler, UErrorCode &status) const;
};


// Name types, these bit flag are used for zone string lookup
enum TimeZoneTranslationType {
    LOCATION        = 0x0001,
    GENERIC_LONG    = 0x0002,
    GENERIC_SHORT   = 0x0004,
    STANDARD_LONG   = 0x0008,
    STANDARD_SHORT  = 0x0010,
    DAYLIGHT_LONG   = 0x0020,
    DAYLIGHT_SHORT  = 0x0040
};

// Name type index, these constants are used for index in the zone strings array.
enum TimeZoneTranslationTypeIndex {
    ZSIDX_LOCATION = 0,
    ZSIDX_LONG_STANDARD,
    ZSIDX_SHORT_STANDARD,
    ZSIDX_LONG_DAYLIGHT,
    ZSIDX_SHORT_DAYLIGHT,
    ZSIDX_LONG_GENERIC,
    ZSIDX_SHORT_GENERIC,

    ZSIDX_COUNT
};

class MessageFormat;


/*
 * ZoneStringInfo is a class holding a localized zone string
 * information.
 */
class ZoneStringInfo : public UMemory {
public:
    virtual ~ZoneStringInfo();

    inline UnicodeString& getID(UnicodeString &result) const;
    inline UnicodeString& getString(UnicodeString &result) const;
    inline UBool isStandard(void) const;
    inline UBool isDaylight(void) const;
    inline UBool isGeneric(void) const;

private:
    friend class ZoneStringFormat;
    friend class ZoneStringSearchResultHandler;

    ZoneStringInfo(const UnicodeString &id, const UnicodeString &str, 
                   TimeZoneTranslationType type, ZSFStringPool &sp, UErrorCode &status);

    const UChar   *fId;
    const UChar   *fStr;
    TimeZoneTranslationType fType;
};

inline UnicodeString& ZoneStringInfo::getID(UnicodeString &result) const {
    return result.setTo(fId, -1);
}

inline UnicodeString& ZoneStringInfo::getString(UnicodeString &result) const {
    return result.setTo(fStr, -1);
}

inline UBool ZoneStringInfo::isStandard(void) const {
    return (fType == STANDARD_LONG || fType == STANDARD_SHORT);
}

inline UBool ZoneStringInfo::isDaylight(void) const {
    return (fType == DAYLIGHT_LONG || fType == DAYLIGHT_SHORT);
}

inline UBool ZoneStringInfo::isGeneric(void) const {
    return (fType == LOCATION || fType == GENERIC_LONG || fType == GENERIC_SHORT);
}

class SafeZoneStringFormatPtr;

class ZoneStringFormat : public UMemory {
public:
    ZoneStringFormat(const UnicodeString* const* strings, int32_t rowCount, int32_t columnCount, UErrorCode &status);
    ZoneStringFormat(const Locale& locale, UErrorCode &status);
    virtual ~ZoneStringFormat();

    /* Gets zone string format from cache if available, create it if not cached. */
    static SafeZoneStringFormatPtr* getZoneStringFormat(const Locale& locale, UErrorCode &status);

    /*
     * Create a snapshot of old zone strings array for the given date
     */
    UnicodeString** createZoneStringsArray(UDate date, int32_t &rowCount, int32_t &colCount, UErrorCode &status) const;

    /* TODO:  There is no implementation for this function.  Delete declaration? */
    const UnicodeString** getZoneStrings(int32_t &rowCount, int32_t &columnCount) const;

    UnicodeString& getSpecificLongString(const Calendar &cal,
        UnicodeString &result, UErrorCode &status) const;

    UnicodeString& getSpecificShortString(const Calendar &cal,
        UBool commonlyUsedOnly, UnicodeString &result, UErrorCode &status) const;

    UnicodeString& getGenericLongString(const Calendar &cal,
        UnicodeString &result, UErrorCode &status) const;

    UnicodeString& getGenericShortString(const Calendar &cal,
        UBool commonlyUsedOnly, UnicodeString &result, UErrorCode &status) const;

    UnicodeString& getGenericLocationString(const Calendar &cal,
        UnicodeString &result, UErrorCode &status) const;

    const ZoneStringInfo* findSpecificLong(const UnicodeString &text, int32_t start,
        int32_t &matchLength, UErrorCode &status) const;
    const ZoneStringInfo* findSpecificShort(const UnicodeString &text, int32_t start,
        int32_t &matchLength, UErrorCode &status) const;
    const ZoneStringInfo* findGenericLong(const UnicodeString &text, int32_t start,
        int32_t &matchLength, UErrorCode &status) const;
    const ZoneStringInfo* findGenericShort(const UnicodeString &text, int32_t start,
        int32_t &matchLength, UErrorCode &status) const;
    const ZoneStringInfo* findGenericLocation(const UnicodeString &text, int32_t start,
        int32_t &matchLength, UErrorCode &status) const;

    // Following APIs are not used by SimpleDateFormat, but public for testing purpose
    inline UnicodeString& getLongStandard(const UnicodeString &tzid, UDate date,
        UnicodeString &result) const;
    inline UnicodeString& getLongDaylight(const UnicodeString &tzid, UDate date,
        UnicodeString &result) const;
    inline UnicodeString& getLongGenericNonLocation(const UnicodeString &tzid, UDate date,
        UnicodeString &result) const;
    inline UnicodeString& getLongGenericPartialLocation(const UnicodeString &tzid, UDate date,
        UnicodeString &result) const;
    inline UnicodeString& getShortStandard(const UnicodeString &tzid, UDate date, UBool commonlyUsedOnly,
        UnicodeString &result) const;
    inline UnicodeString& getShortDaylight(const UnicodeString &tzid, UDate date, UBool commonlyUsedOnly,
        UnicodeString &result) const;
    inline UnicodeString& getShortGenericNonLocation(const UnicodeString &tzid, UDate date, UBool commonlyUsedOnly,
        UnicodeString &result) const;
    inline UnicodeString& getShortGenericPartialLocation(const UnicodeString &tzid, UDate date, UBool commonlyUsedOnly,
        UnicodeString &result) const;
    inline UnicodeString& getGenericLocation(const UnicodeString &tzid, UnicodeString &result) const;

private:
    Locale           fLocale;
    UHashtable      *fTzidToStrings;
    UHashtable      *fMzidToStrings;

    TextTrieMap      fZoneStringsTrie;
    ZSFStringPool    fStringPool;

    UResourceBundle *fZoneStringsArray;
    UResourceBundle *fMetazoneItem;
    UResourceBundle *fZoneItem;

	UBool			 fIsFullyLoaded;

	void loadZone(const UnicodeString &utzid, UErrorCode &status);
	void addSingleZone(UnicodeString &utzid, UErrorCode &status);
	void loadFull(UErrorCode &status);


    /*
     * Private method to get a zone string except generic partial location types.
     */
    UnicodeString& getString(const UnicodeString &tzid, TimeZoneTranslationTypeIndex typeIdx, UDate date,
        UBool commonlyUsedOnly, UnicodeString& result) const;

    /*
     * Private method to get a generic string, with fallback logic involved,
     * that is,
     * 
     * 1. If a generic non-location string is avaiable for the zone, return it.
     * 2. If a generic non-location string is associated with a metazone and 
     *    the zone never use daylight time around the given date, use the standard
     *    string (if available).
     *    
     *    Note: In CLDR1.5.1, the same localization is used for generic and standard.
     *    In this case, we do not use the standard string and do the rest.
     *    
     * 3. If a generic non-location string is associated with a metazone and
     *    the offset at the given time is different from the preferred zone for the
     *    current locale, then return the generic partial location string (if avaiable)
     * 4. If a generic non-location string is not available, use generic location
     *    string.
     */
    UnicodeString& getGenericString(const Calendar &cal, UBool isShort, UBool commonlyUsedOnly,
        UnicodeString &result, UErrorCode &status) const;

    /*
     * Private method to get a generic partial location string
     */
    UnicodeString& getGenericPartialLocationString(const UnicodeString &tzid, UBool isShort,
        UDate date, UBool commonlyUsedOnly, UnicodeString &result) const;

    /*
     * Find a prefix matching time zone for the given zone string types.
     * @param text The text contains a time zone string
     * @param start The start index within the text
     * @param types The bit mask representing a set of requested types
     * @param matchLength Receives the match length
     * @param status
     * @return If any zone string matched for the requested types, returns a
     * ZoneStringInfo for the longest match.  If no matches are found for
     * the requested types, returns a ZoneStringInfo for the longest match
     * for any other types.  If nothing matches at all, returns null.
     */
    const ZoneStringInfo* find(const UnicodeString &text, int32_t start, int32_t types,
        int32_t &matchLength, UErrorCode &status) const;
	const ZoneStringInfo* subFind(const UnicodeString &text, int32_t start, int32_t types,
                       int32_t &matchLength, UErrorCode &status) const;

    UnicodeString& getRegion(UnicodeString &region) const;

    static MessageFormat* getFallbackFormat(const Locale &locale, UErrorCode &status);
    static MessageFormat* getRegionFormat(const Locale &locale, UErrorCode &status);
    const UChar* getZoneStringFromBundle(const UResourceBundle *zoneitem, const char *key);
    static UBool isCommonlyUsed(const UResourceBundle *zoneitem);
    static UnicodeString& getLocalizedCountry(const UnicodeString &countryCode, const Locale &locale,
        UnicodeString &displayCountry);
};

inline UnicodeString&
ZoneStringFormat::getLongStandard(const UnicodeString &tzid, UDate date,
                                  UnicodeString &result) const {
    return getString(tzid, ZSIDX_LONG_STANDARD, date, FALSE /* not used */, result);
}

inline UnicodeString&
ZoneStringFormat::getLongDaylight(const UnicodeString &tzid, UDate date,
                                  UnicodeString &result) const {
    return getString(tzid, ZSIDX_LONG_DAYLIGHT, date, FALSE /* not used */, result);
}

inline UnicodeString&
ZoneStringFormat::getLongGenericNonLocation(const UnicodeString &tzid, UDate date,
                                            UnicodeString &result) const {
    return getString(tzid, ZSIDX_LONG_GENERIC, date, FALSE /* not used */, result);
}

inline UnicodeString&
ZoneStringFormat::getLongGenericPartialLocation(const UnicodeString &tzid, UDate date,
                                                UnicodeString &result) const {
    return getGenericPartialLocationString(tzid, FALSE, date, FALSE /* not used */, result);
}

inline UnicodeString&
ZoneStringFormat::getShortStandard(const UnicodeString &tzid, UDate date, UBool commonlyUsedOnly,
                                   UnicodeString &result) const {
    return getString(tzid, ZSIDX_SHORT_STANDARD, date, commonlyUsedOnly, result);
}

inline UnicodeString&
ZoneStringFormat::getShortDaylight(const UnicodeString &tzid, UDate date, UBool commonlyUsedOnly,
                                   UnicodeString &result) const {
    return getString(tzid, ZSIDX_SHORT_DAYLIGHT, date, commonlyUsedOnly, result);
}

inline UnicodeString&
ZoneStringFormat::getShortGenericNonLocation(const UnicodeString &tzid, UDate date, UBool commonlyUsedOnly,
                                             UnicodeString &result) const {
    return getString(tzid, ZSIDX_SHORT_GENERIC, date, commonlyUsedOnly, result);
}

inline UnicodeString&
ZoneStringFormat::getShortGenericPartialLocation(const UnicodeString &tzid, UDate date, UBool commonlyUsedOnly,
                                                 UnicodeString &result) const {
    return getGenericPartialLocationString(tzid, TRUE, date, commonlyUsedOnly, result);
}

inline UnicodeString&
ZoneStringFormat::getGenericLocation(const UnicodeString &tzid, UnicodeString &result) const {
    return getString(tzid, ZSIDX_LOCATION, 0 /*not used*/, FALSE /*not used*/, result);
}


/*
 * ZoneStrings is a container of localized zone strings used by ZoneStringFormat
 */
class ZoneStrings : public UMemory {
public:
    ZoneStrings(UnicodeString *strings, 
                int32_t        stringsCount, 
                UBool          commonlyUsed,
                UnicodeString **genericPartialLocationStrings, 
                int32_t        genericRowCount, 
                int32_t        genericColCount,
                ZSFStringPool &sp,
                UErrorCode    &status);
    virtual         ~ZoneStrings();

    UnicodeString&   getString(int32_t typeIdx, UnicodeString &result) const;
    inline UBool     isShortFormatCommonlyUsed(void) const;
    UnicodeString&   getGenericPartialLocationString(const UnicodeString &mzid, UBool isShort,
                                        UBool commonlyUsedOnly, UnicodeString &result) const;

private:
    const UChar   **fStrings;
    int32_t         fStringsCount;
    UBool           fIsCommonlyUsed;
    const UChar * **fGenericPartialLocationStrings;
    int32_t         fGenericPartialLocationRowCount;
    int32_t         fGenericPartialLocationColCount;
};

inline UBool
ZoneStrings::isShortFormatCommonlyUsed(void) const {
    return fIsCommonlyUsed;
}

/*
 * ZoneStringSearchResultHandler is an implementation of
 * TextTrieMapSearchHandler.  This class is used by ZoneStringFormat
 * for collecting search results for localized zone strings.
 */
class ZoneStringSearchResultHandler : public TextTrieMapSearchResultHandler {
public:
    ZoneStringSearchResultHandler(UErrorCode &status);
    virtual ~ZoneStringSearchResultHandler();

    virtual UBool handleMatch(int32_t matchLength, const CharacterNode *node, UErrorCode &status);
    int32_t countMatches(void);
    const ZoneStringInfo* getMatch(int32_t index, int32_t &matchLength);
    void clear(void);

private:
    UVector fResults;
    int32_t fMatchLen[ZSIDX_COUNT];
};


/*
 * ZoneStringFormat cache implementation
 */
class ZSFCacheEntry : public UMemory {
public:
    ~ZSFCacheEntry();

    void delRef(void);
    const ZoneStringFormat* getZoneStringFormat(void);

private:
    friend class ZSFCache;

    ZSFCacheEntry(const Locale &locale, ZoneStringFormat *zsf, ZSFCacheEntry *next);

    Locale              fLocale;
    ZoneStringFormat    *fZoneStringFormat;
    ZSFCacheEntry       *fNext;
    int32_t             fRefCount;
};

class SafeZoneStringFormatPtr : public UMemory {
public:
    ~SafeZoneStringFormatPtr();
    const ZoneStringFormat* get() const;

private:
    friend class ZSFCache;

    SafeZoneStringFormatPtr(ZSFCacheEntry *cacheEntry);

    ZSFCacheEntry   *fCacheEntry;
};

class ZSFCache : public UMemory {
public:
    ZSFCache(int32_t capacity);
    ~ZSFCache();

    SafeZoneStringFormatPtr* get(const Locale &locale, UErrorCode &status);

private:
    int32_t         fCapacity;
    ZSFCacheEntry   *fFirst;
};

U_NAMESPACE_END

#endif /* #if !UCONFIG_NO_FORMATTING */

#endif // ZSTRFMT_H
