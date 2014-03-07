/*
*******************************************************************************
* Copyright (C) 2007-2010, International Business Machines Corporation and    *
* others. All Rights Reserved.                                                *
*******************************************************************************
*/

#include "unicode/utypes.h"

#if !UCONFIG_NO_FORMATTING

#include "zstrfmt.h"

#include "unicode/ustring.h"
#include "unicode/putil.h"
#include "unicode/msgfmt.h"
#include "unicode/basictz.h"
#include "unicode/simpletz.h"
#include "unicode/rbtz.h"
#include "unicode/vtzone.h"

#include "uvector.h"
#include "cstring.h"
#include "cmemory.h"
#include "uresimp.h"
#include "zonemeta.h"
#include "olsontz.h"
#include "umutex.h"
#include "ucln_in.h"
#include "uassert.h"
#include "ureslocs.h"

/**
 * global ZoneStringFormatCache stuffs
 */
static UMTX gZSFCacheLock = NULL;
static U_NAMESPACE_QUALIFIER ZSFCache *gZoneStringFormatCache = NULL;

U_CDECL_BEGIN
/**
 * ZoneStringFormatCache cleanup callback func
 */
static UBool U_CALLCONV zoneStringFormat_cleanup(void)
{
    umtx_destroy(&gZSFCacheLock);
    if (gZoneStringFormatCache != NULL) {
        delete gZoneStringFormatCache;
        gZoneStringFormatCache = NULL;
    }
    gZoneStringFormatCache = NULL;
    return TRUE;
}

/**
 * Deleter for ZoneStringInfo
 */
static void U_CALLCONV
deleteZoneStringInfo(void *obj) {
    delete (U_NAMESPACE_QUALIFIER ZoneStringInfo*)obj;
}

/**
 * Deleter for ZoneStrings
 */
static void U_CALLCONV
deleteZoneStrings(void *obj) {
    delete (U_NAMESPACE_QUALIFIER ZoneStrings*)obj;
}
U_CDECL_END

U_NAMESPACE_BEGIN

#define ZID_KEY_MAX 128

static const char gCountriesTag[]       = "Countries";
static const char gZoneStringsTag[]     = "zoneStrings";
static const char gShortGenericTag[]    = "sg";
static const char gShortStandardTag[]   = "ss";
static const char gShortDaylightTag[]   = "sd";
static const char gLongGenericTag[]     = "lg";
static const char gLongStandardTag[]    = "ls";
static const char gLongDaylightTag[]    = "ld";
static const char gExemplarCityTag[]    = "ec";
static const char gCommonlyUsedTag[]    = "cu";
static const char gFallbackFormatTag[]  = "fallbackFormat";
static const char gRegionFormatTag[]    = "regionFormat";

#define MZID_PREFIX_LEN 5
static const char gMetazoneIdPrefix[]   = "meta:";

#define MAX_METAZONES_PER_ZONE 10

static const UChar gDefFallbackPattern[]    = {0x7B, 0x31, 0x7D, 0x20, 0x28, 0x7B, 0x30, 0x7D, 0x29, 0x00}; // "{1} ({0})"
static const UChar gDefRegionPattern[]      = {0x7B, 0x30, 0x7D, 0x00}; // "{0}"
static const UChar gCommonlyUsedTrue[]      = {0x31, 0x00}; // "1"

static const double kDstCheckRange      = (double)184*U_MILLIS_PER_DAY;

static int32_t
getTimeZoneTranslationTypeIndex(TimeZoneTranslationType type) {
    int32_t typeIdx = 0;
    switch (type) {
        case LOCATION:
            typeIdx = ZSIDX_LOCATION;
            break;
        case GENERIC_LONG:
            typeIdx = ZSIDX_LONG_GENERIC;
            break;
        case GENERIC_SHORT:
            typeIdx = ZSIDX_SHORT_GENERIC;
            break;
        case STANDARD_LONG:
            typeIdx = ZSIDX_LONG_STANDARD;
            break;
        case STANDARD_SHORT:
            typeIdx = ZSIDX_SHORT_STANDARD;
            break;
        case DAYLIGHT_LONG:
            typeIdx = ZSIDX_LONG_DAYLIGHT;
            break;
        case DAYLIGHT_SHORT:
            typeIdx = ZSIDX_SHORT_DAYLIGHT;
            break;
    }
    return typeIdx;
}

static int32_t
getTimeZoneTranslationType(TimeZoneTranslationTypeIndex typeIdx) {
    int32_t type = 0;
    switch (typeIdx) {
        case ZSIDX_LOCATION:
            type = LOCATION;
            break;
        case ZSIDX_LONG_GENERIC:
            type = GENERIC_LONG;
            break;
        case ZSIDX_SHORT_GENERIC:
            type = GENERIC_SHORT;
            break;
        case ZSIDX_LONG_STANDARD:
            type = STANDARD_LONG;
            break;
        case ZSIDX_SHORT_STANDARD:
            type = STANDARD_SHORT;
            break;
        case ZSIDX_LONG_DAYLIGHT:
            type = DAYLIGHT_LONG;
            break;
        case ZSIDX_COUNT:
        case ZSIDX_SHORT_DAYLIGHT:
            type = DAYLIGHT_SHORT;
            break;
        default:
            break;
    }
    return type;
}

#define DEFAULT_CHARACTERNODE_CAPACITY 1

// ----------------------------------------------------------------------------
void CharacterNode::clear() {
    uprv_memset(this, 0, sizeof(*this));
}

void CharacterNode::deleteValues() {
    if (fValues == NULL) {
        // Do nothing.
    } else if (!fHasValuesVector) {
        deleteZoneStringInfo(fValues);
    } else {
        delete (UVector *)fValues;
    }
}

void
CharacterNode::addValue(void *value, UErrorCode &status) {
    if (U_FAILURE(status)) {
        deleteZoneStringInfo(value);
        return;
    }
    if (fValues == NULL) {
        fValues = value;
    } else {
        // At least one value already.
        if (!fHasValuesVector) {
            // There is only one value so far, and not in a vector yet.
            // Create a vector and add the old value.
            UVector *values = new UVector(deleteZoneStringInfo, NULL, DEFAULT_CHARACTERNODE_CAPACITY, status);
            if (U_FAILURE(status)) {
                deleteZoneStringInfo(value);
                return;
            }
            values->addElement(fValues, status);
            fValues = values;
            fHasValuesVector = TRUE;
        }
        // Add the new value.
        ((UVector *)fValues)->addElement(value, status);
    }
}

//----------------------------------------------------------------------------
// Virtual destructor to avoid warning
TextTrieMapSearchResultHandler::~TextTrieMapSearchResultHandler(){
}

// ----------------------------------------------------------------------------
TextTrieMap::TextTrieMap(UBool ignoreCase)
: fIgnoreCase(ignoreCase), fNodes(NULL), fNodesCapacity(0), fNodesCount(0), 
  fLazyContents(NULL), fIsEmpty(TRUE) {
}

TextTrieMap::~TextTrieMap() {
    int32_t index;
    for (index = 0; index < fNodesCount; ++index) {
        fNodes[index].deleteValues();
    }
    uprv_free(fNodes);
    if (fLazyContents != NULL) {
        for (int32_t i=0; i<fLazyContents->size(); i+=2) {
            ZoneStringInfo *zsinf = (ZoneStringInfo *)fLazyContents->elementAt(i+1);
            delete zsinf;
        } 
        delete fLazyContents;
    }
}

int32_t TextTrieMap::isEmpty() const {
    // Use a separate field for fIsEmpty because it will remain unchanged once the
    //   Trie is built, while fNodes and fLazyContents change with the lazy init
    //   of the nodes structure.  Trying to test the changing fields has
    //   thread safety complications.
    return fIsEmpty;
}


//  We defer actually building the TextTrieMap node structure until the first time a
//     search is performed.  put() simply saves the parameters in case we do
//     eventually need to build it.
//     
void
TextTrieMap::put(const UnicodeString &key, void *value, ZSFStringPool &sp, UErrorCode &status) {
    fIsEmpty = FALSE;
    if (fLazyContents == NULL) {
        fLazyContents = new UVector(status);
        if (fLazyContents == NULL) {
            status = U_MEMORY_ALLOCATION_ERROR;
        }
    }
    if (U_FAILURE(status)) {
        return;
    }
    UChar *s = const_cast<UChar *>(sp.get(key, status));
    fLazyContents->addElement(s, status);
    fLazyContents->addElement(value, status);
}


void
TextTrieMap::putImpl(const UnicodeString &key, void *value, UErrorCode &status) {
    if (fNodes == NULL) {
        fNodesCapacity = 512;
        fNodes = (CharacterNode *)uprv_malloc(fNodesCapacity * sizeof(CharacterNode));
        fNodes[0].clear();  // Init root node.
        fNodesCount = 1;
    }

    UnicodeString foldedKey;
    const UChar *keyBuffer;
    int32_t keyLength;
    if (fIgnoreCase) {
        // Ok to use fastCopyFrom() because we discard the copy when we return.
        foldedKey.fastCopyFrom(key).foldCase();
        keyBuffer = foldedKey.getBuffer();
        keyLength = foldedKey.length();
    } else {
        keyBuffer = key.getBuffer();
        keyLength = key.length();
    }

    CharacterNode *node = fNodes;
    int32_t index;
    for (index = 0; index < keyLength; ++index) {
        node = addChildNode(node, keyBuffer[index], status);
    }
    node->addValue(value, status);
}

UBool
TextTrieMap::growNodes() {
    if (fNodesCapacity == 0xffff) {
        return FALSE;  // We use 16-bit node indexes.
    }
    int32_t newCapacity = fNodesCapacity + 1000;
    if (newCapacity > 0xffff) {
        newCapacity = 0xffff;
    }
    CharacterNode *newNodes = (CharacterNode *)uprv_malloc(newCapacity * sizeof(CharacterNode));
    if (newNodes == NULL) {
        return FALSE;
    }
    uprv_memcpy(newNodes, fNodes, fNodesCount * sizeof(CharacterNode));
    uprv_free(fNodes);
    fNodes = newNodes;
    fNodesCapacity = newCapacity;
    return TRUE;
}

CharacterNode*
TextTrieMap::addChildNode(CharacterNode *parent, UChar c, UErrorCode &status) {
    if (U_FAILURE(status)) {
        return NULL;
    }
    // Linear search of the sorted list of children.
    uint16_t prevIndex = 0;
    uint16_t nodeIndex = parent->fFirstChild;
    while (nodeIndex > 0) {
        CharacterNode *current = fNodes + nodeIndex;
        UChar childCharacter = current->fCharacter;
        if (childCharacter == c) {
            return current;
        } else if (childCharacter > c) {
            break;
        }
        prevIndex = nodeIndex;
        nodeIndex = current->fNextSibling;
    }

    // Ensure capacity. Grow fNodes[] if needed.
    if (fNodesCount == fNodesCapacity) {
        int32_t parentIndex = (int32_t)(parent - fNodes);
        if (!growNodes()) {
            status = U_MEMORY_ALLOCATION_ERROR;
            return NULL;
        }
        parent = fNodes + parentIndex;
    }

    // Insert a new child node with c in sorted order.
    CharacterNode *node = fNodes + fNodesCount;
    node->clear();
    node->fCharacter = c;
    node->fNextSibling = nodeIndex;
    if (prevIndex == 0) {
        parent->fFirstChild = (uint16_t)fNodesCount;
    } else {
        fNodes[prevIndex].fNextSibling = (uint16_t)fNodesCount;
    }
    ++fNodesCount;
    return node;
}

CharacterNode*
TextTrieMap::getChildNode(CharacterNode *parent, UChar c) const {
    // Linear search of the sorted list of children.
    uint16_t nodeIndex = parent->fFirstChild;
    while (nodeIndex > 0) {
        CharacterNode *current = fNodes + nodeIndex;
        UChar childCharacter = current->fCharacter;
        if (childCharacter == c) {
            return current;
        } else if (childCharacter > c) {
            break;
        }
        nodeIndex = current->fNextSibling;
    }
    return NULL;
}

// Mutex for protecting the lazy creation of the Trie node structure on the first call to search().
static UMTX TextTrieMutex;

// buildTrie() - The Trie node structure is needed.  Create it from the data that was
//               saved at the time the ZoneStringFormatter was created.  The Trie is only
//               needed for parsing operations, which are less common than formatting,
//               and the Trie is big, which is why its creation is deferred until first use.
void TextTrieMap::buildTrie(UErrorCode &status) {
    umtx_lock(&TextTrieMutex);
    if (fLazyContents != NULL) {
        for (int32_t i=0; i<fLazyContents->size(); i+=2) {
            const UChar *key = (UChar *)fLazyContents->elementAt(i);
            void  *val = fLazyContents->elementAt(i+1);
            UnicodeString keyString(TRUE, key, -1);  // Aliasing UnicodeString constructor.
            putImpl(keyString, val, status);
        }
        delete fLazyContents;
        fLazyContents = NULL; 
    }
    umtx_unlock(&TextTrieMutex);
}


void
TextTrieMap::search(const UnicodeString &text, int32_t start,
                  TextTrieMapSearchResultHandler *handler, UErrorCode &status) const {
    UBool trieNeedsInitialization = FALSE;
    UMTX_CHECK(&TextTrieMutex, fLazyContents != NULL, trieNeedsInitialization);
    if (trieNeedsInitialization) {
        TextTrieMap *nonConstThis = const_cast<TextTrieMap *>(this);
        nonConstThis->buildTrie(status);
    }
    if (fNodes == NULL) {
        return;
    }
    search(fNodes, text, start, start, handler, status);
}

void
TextTrieMap::search(CharacterNode *node, const UnicodeString &text, int32_t start,
                  int32_t index, TextTrieMapSearchResultHandler *handler, UErrorCode &status) const {
    if (U_FAILURE(status)) {
        return;
    }
    if (node->hasValues()) {
        if (!handler->handleMatch(index - start, node, status)) {
            return;
        }
        if (U_FAILURE(status)) {
            return;
        }
    }
    UChar32 c = text.char32At(index);
    if (fIgnoreCase) {
        // size of character may grow after fold operation
        UnicodeString tmp(c);
        tmp.foldCase();
        int32_t tmpidx = 0;
        while (tmpidx < tmp.length()) {
            c = tmp.char32At(tmpidx);
            node = getChildNode(node, c);
            if (node == NULL) {
                break;
            }
            tmpidx = tmp.moveIndex32(tmpidx, 1);
        }
    } else {
        node = getChildNode(node, c);
    }
    if (node != NULL) {
        search(node, text, start, index+1, handler, status);
    }
}

// ----------------------------------------------------------------------------
ZoneStringInfo::ZoneStringInfo(const UnicodeString &id, const UnicodeString &str,
                               TimeZoneTranslationType type, ZSFStringPool &sp, UErrorCode &status)
: fType(type) {
    fId = sp.get(id, status);
    fStr = sp.get(str, status);
}

ZoneStringInfo::~ZoneStringInfo() {
}


// ----------------------------------------------------------------------------
ZoneStringSearchResultHandler::ZoneStringSearchResultHandler(UErrorCode &status)
: fResults(status)
{
    clear();
}

ZoneStringSearchResultHandler::~ZoneStringSearchResultHandler() {
    clear();
}

UBool
ZoneStringSearchResultHandler::handleMatch(int32_t matchLength, const CharacterNode *node, UErrorCode &status) {
    if (U_FAILURE(status)) {
        return FALSE;
    }
    if (node->hasValues()) {
        int32_t valuesCount = node->countValues();
        for (int32_t i = 0; i < valuesCount; i++) {
            ZoneStringInfo *zsinfo = (ZoneStringInfo*)node->getValue(i);
            if (zsinfo == NULL) {
                break;
            }
            // Update the results
            UBool foundType = FALSE;
            for (int32_t j = 0; j < fResults.size(); j++) {
                ZoneStringInfo *tmp = (ZoneStringInfo*)fResults.elementAt(j);
                if (zsinfo->fType == tmp->fType) {
                    int32_t lenidx = getTimeZoneTranslationTypeIndex(tmp->fType);
                    if (matchLength > fMatchLen[lenidx]) {
                        // Same type, longer match
                        fResults.setElementAt(zsinfo, j);
                        fMatchLen[lenidx] = matchLength;
                    }
                    foundType = TRUE;
                    break;
                }
            }
            if (!foundType) {
                // not found in the current list
                fResults.addElement(zsinfo, status);
                fMatchLen[getTimeZoneTranslationTypeIndex(zsinfo->fType)] = matchLength;
            }
        }
    }
    return TRUE;
}

int32_t
ZoneStringSearchResultHandler::countMatches(void) {
    return fResults.size();
}

const ZoneStringInfo*
ZoneStringSearchResultHandler::getMatch(int32_t index, int32_t &matchLength) {
    ZoneStringInfo *zsinfo = NULL;
    if (index < fResults.size()) {
        zsinfo = (ZoneStringInfo*)fResults.elementAt(index);
        matchLength = fMatchLen[getTimeZoneTranslationTypeIndex(zsinfo->fType)];
    }
    return zsinfo;
}

void
ZoneStringSearchResultHandler::clear(void) {
    fResults.removeAllElements();
    for (int32_t i = 0; i < (int32_t)(sizeof(fMatchLen)/sizeof(fMatchLen[0])); i++) {
        fMatchLen[i] = 0;
    }
}

// Mutex for protecting the lazy load of a zone ID (or a full load) to ZoneStringFormat structures.
static UMTX ZoneStringFormatMutex;


// ----------------------------------------------------------------------------
ZoneStringFormat::ZoneStringFormat(const UnicodeString* const* strings,
                                   int32_t rowCount, int32_t columnCount, UErrorCode &status)
: fLocale(""),
  fTzidToStrings(NULL),
  fMzidToStrings(NULL),
  fZoneStringsTrie(TRUE),
  fStringPool(status),
  fZoneStringsArray(NULL),
  fMetazoneItem(NULL),
  fZoneItem(NULL),
  fIsFullyLoaded(FALSE)
{
    if (U_FAILURE(status)) {
        return;
    }
    fLocale.setToBogus();
    if (strings == NULL || columnCount <= 0 || rowCount <= 0) {
        status = U_ILLEGAL_ARGUMENT_ERROR;
        return;
    }
    fTzidToStrings = uhash_open(uhash_hashUChars,     // key hash function
                                uhash_compareUChars,  // key comparison function
                                NULL,                 // Value comparison function
                                &status);
    fMzidToStrings = uhash_open(uhash_hashUChars,   
                                uhash_compareUChars,
                                NULL,
                                &status);

    uhash_setValueDeleter(fTzidToStrings, deleteZoneStrings);
    uhash_setValueDeleter(fMzidToStrings, deleteZoneStrings);

    for (int32_t row = 0; row < rowCount; row++) {
        if (strings[row][0].isEmpty()) {
            continue;
        }
        UnicodeString *names = new UnicodeString[ZSIDX_COUNT];
        for (int32_t col = 1; col < columnCount; col++) {
            if (!strings[row][col].isEmpty()) {
                int32_t typeIdx = -1;
                switch (col) {
                    case 1:
                        typeIdx = ZSIDX_LONG_STANDARD;
                        break;
                    case 2:
                        typeIdx = ZSIDX_SHORT_STANDARD;
                        break;
                    case 3:
                        typeIdx = ZSIDX_LONG_DAYLIGHT;
                        break;
                    case 4:
                        typeIdx = ZSIDX_SHORT_DAYLIGHT;
                        break;
                    case 5:
                        typeIdx = ZSIDX_LOCATION;
                        break;
                    case 6:
                        typeIdx = ZSIDX_LONG_GENERIC;
                        break;
                    case 7:
                        typeIdx = ZSIDX_SHORT_GENERIC;
                        break;
                }
                if (typeIdx != -1) {
                    names[typeIdx].setTo(strings[row][col]);

                    // Put the name into the trie
                    int32_t type = getTimeZoneTranslationType((TimeZoneTranslationTypeIndex)typeIdx);
                    ZoneStringInfo *zsinf = new ZoneStringInfo(strings[row][0], 
                                                               strings[row][col], 
                                                               (TimeZoneTranslationType)type, 
                                                               fStringPool,
                                                               status);
                    fZoneStringsTrie.put(strings[row][col], zsinf, fStringPool, status);
                    if (U_FAILURE(status)) {
                        delete zsinf;
                        goto error_cleanup;
                    }
                }
            }
        }
        // Note:  ZoneStrings constructor adopts and delete the names array.
        ZoneStrings *zstrings = new ZoneStrings(names, ZSIDX_COUNT, TRUE, NULL, 0, 0,
                                                fStringPool, status);
        UChar *utzid = const_cast<UChar *>(fStringPool.get(strings[row][0], status));
        uhash_put(fTzidToStrings, utzid, zstrings, &status);
        if (U_FAILURE(status)) {
            delete zstrings;
            goto error_cleanup;
        }
    }
    fStringPool.freeze();
	fIsFullyLoaded = TRUE;
    return;

error_cleanup:
    return;
}

ZoneStringFormat::ZoneStringFormat(const Locale &locale, UErrorCode &status)
: fLocale(locale),
  fTzidToStrings(NULL),
  fMzidToStrings(NULL),
  fZoneStringsTrie(TRUE),
  fStringPool(status),
  fZoneStringsArray(NULL),
  fMetazoneItem(NULL),
  fZoneItem(NULL),
  fIsFullyLoaded(FALSE)
{
    if (U_FAILURE(status)) {
        return;
    }
    fTzidToStrings = uhash_open(uhash_hashUChars,     // key hash function
                                uhash_compareUChars,  // key comparison function
                                NULL,                 // Value comparison function
                                &status);
    fMzidToStrings = uhash_open(uhash_hashUChars,     // key hash function
                                uhash_compareUChars,  // key comparison function
                                NULL,                 // Value comparison function
                                &status);
    uhash_setValueDeleter(fTzidToStrings, deleteZoneStrings);
    uhash_setValueDeleter(fMzidToStrings, deleteZoneStrings);
}

// Load only a single zone
void
ZoneStringFormat::loadZone(const UnicodeString &utzid, UErrorCode &status)
{
	if (fIsFullyLoaded) {
		return;
	}

	if (U_FAILURE(status)) {
		return;
	}

	umtx_lock(&ZoneStringFormatMutex);

	if (fZoneStringsArray == NULL) {
		fZoneStringsArray = ures_open(U_ICUDATA_ZONE, fLocale.getName(), &status);
		fZoneStringsArray = ures_getByKeyWithFallback(fZoneStringsArray, gZoneStringsTag, fZoneStringsArray, &status);
		if (U_FAILURE(status)) {
			// If no locale bundles are available, zoneStrings will be null.
			// We still want to go through the rest of zone strings initialization,
			// because generic location format is generated from tzid for the case.
			// The rest of code should work even zoneStrings is null.
			status = U_ZERO_ERROR;
			ures_close(fZoneStringsArray);
			fZoneStringsArray = NULL;
		}
    }

	// Skip non-canonical IDs
	UnicodeString canonicalID;
	TimeZone::getCanonicalID(utzid, canonicalID, status);
	if (U_FAILURE(status)) {
		// Ignore unknown ID - we should not get here, but just in case.
		//	status = U_ZERO_ERROR;
		umtx_unlock(&ZoneStringFormatMutex);
		return;
	}

    if (U_SUCCESS(status)) {
		if (uhash_count(fTzidToStrings) > 0) {
			ZoneStrings *zstrings = (ZoneStrings*)uhash_get(fTzidToStrings, canonicalID.getTerminatedBuffer());
			if (zstrings != NULL) {
				umtx_unlock(&ZoneStringFormatMutex);
				return;	//	We already about this one
			}
		}
	}

	addSingleZone(canonicalID, status);

	umtx_unlock(&ZoneStringFormatMutex);
}

// Load only a single zone
void
ZoneStringFormat::addSingleZone(UnicodeString &utzid, UErrorCode &status)
{
	if (U_FAILURE(status)) {
		return;
	}

	if (uhash_count(fTzidToStrings) > 0) {
		ZoneStrings *zstrings = (ZoneStrings*)uhash_get(fTzidToStrings, utzid.getTerminatedBuffer());
		if (zstrings != NULL) {
			return;	//	We already about this one
		}
	}

    MessageFormat *fallbackFmt = NULL;
    MessageFormat *regionFmt = NULL;

    fallbackFmt = getFallbackFormat(fLocale, status);
    if (U_FAILURE(status)) {
        goto error_cleanup;
    }
    regionFmt = getRegionFormat(fLocale, status);
    if (U_FAILURE(status)) {
        goto error_cleanup;
    }


	{
		char zidkey[ZID_KEY_MAX+1];
		char tzid[ZID_KEY_MAX+1];
		utzid.extract(0, utzid.length(), zidkey, ZID_KEY_MAX, US_INV);
		utzid.extract(0, utzid.length(), tzid, ZID_KEY_MAX, US_INV);

		const UChar *zstrarray[ZSIDX_COUNT];
		const UChar *mzstrarray[ZSIDX_COUNT];
		UnicodeString mzPartialLoc[MAX_METAZONES_PER_ZONE][4];

		// Replace '/' with ':'
		char *pCity = NULL;
		char *p = zidkey;
		while (*p) {
			if (*p == '/') {
				*p = ':';
				pCity = p + 1;
			}
			p++;
		}
	
		if (fZoneStringsArray != NULL) {
			fZoneItem = ures_getByKeyWithFallback(fZoneStringsArray, zidkey, fZoneItem, &status);
			if (U_FAILURE(status)) {
				// If failed to open the zone item, create only location string
				ures_close(fZoneItem);
				fZoneItem = NULL;
				status = U_ZERO_ERROR;
			}
		}
	
		UnicodeString region;
		getRegion(region);

		zstrarray[ZSIDX_LONG_STANDARD]  = getZoneStringFromBundle(fZoneItem, gLongStandardTag);
		zstrarray[ZSIDX_SHORT_STANDARD] = getZoneStringFromBundle(fZoneItem, gShortStandardTag);
		zstrarray[ZSIDX_LONG_DAYLIGHT]  = getZoneStringFromBundle(fZoneItem, gLongDaylightTag);
		zstrarray[ZSIDX_SHORT_DAYLIGHT] = getZoneStringFromBundle(fZoneItem, gShortDaylightTag);
		zstrarray[ZSIDX_LONG_GENERIC]   = getZoneStringFromBundle(fZoneItem, gLongGenericTag);
		zstrarray[ZSIDX_SHORT_GENERIC]  = getZoneStringFromBundle(fZoneItem, gShortGenericTag);
	
		// Compose location format string
		UnicodeString location;
		UnicodeString country;
		UnicodeString city;
		UnicodeString countryCode;
		ZoneMeta::getCanonicalCountry(utzid, countryCode);
		if (!countryCode.isEmpty()) {
			const UChar* tmpCity = getZoneStringFromBundle(fZoneItem, gExemplarCityTag);
			if (tmpCity != NULL) {
				city.setTo(TRUE, tmpCity, -1);
			} else {
				city.setTo(UnicodeString(pCity, -1, US_INV));
				// Replace '_' with ' '
				for (int32_t i = 0; i < city.length(); i++) {
					if (city.charAt(i) == (UChar)0x5F /*'_'*/) {
						city.setCharAt(i, (UChar)0x20 /*' '*/);
					}
				}
			}
			getLocalizedCountry(countryCode, fLocale, country);
			UnicodeString singleCountry;
			ZoneMeta::getSingleCountry(utzid, singleCountry);
			FieldPosition fpos;
			if (singleCountry.isEmpty()) {
				Formattable params [] = {
					Formattable(city),
					Formattable(country)
				};
				fallbackFmt->format(params, 2, location, fpos, status);
			} else {
				// If the zone is only one zone in the country, do not add city
				Formattable params [] = {
					Formattable(country)
				};
				regionFmt->format(params, 1, location, fpos, status);
			}
			if (U_FAILURE(status)) {
				goto error_cleanup;
			}
	
			zstrarray[ZSIDX_LOCATION] = location.getTerminatedBuffer();
		} else {
			if (uprv_strlen(tzid) > 4 && uprv_strncmp(tzid, "Etc/", 4) == 0) {
				// "Etc/xxx" is not associated with a specific location, so localized
				// GMT format is always used as generic location format.
				zstrarray[ZSIDX_LOCATION] = NULL;
			} else {
				// When a new time zone ID, which is actually associated with a specific
				// location, is added in tzdata, but the current CLDR data does not have
				// the information yet, ICU creates a generic location string based on
				// the ID.  This implementation supports canonical time zone round trip
				// with format pattern "VVVV".  See #6602 for the details.
				UnicodeString loc(utzid);
				int32_t slashIdx = loc.lastIndexOf((UChar)0x2f);
				if (slashIdx == -1) {
					// A time zone ID without slash in the tz database is not
					// associated with a specific location.  For instances,
					// MET, CET, EET and WET fall into this category.
					// In this case, we still use GMT format as fallback.
					zstrarray[ZSIDX_LOCATION] = NULL;
				} else {
					FieldPosition fpos;
					Formattable params[] = {
						Formattable(loc)
					};
					regionFmt->format(params, 1, location, fpos, status);
					if (U_FAILURE(status)) {
						goto error_cleanup;
					}
					zstrarray[ZSIDX_LOCATION] = location.getTerminatedBuffer();
				}
			}
		}
	
		UBool commonlyUsed = isCommonlyUsed(fZoneItem);
	
		// Resolve metazones used by this zone
		int32_t mzPartialLocIdx = 0;
		const UVector *metazoneMappings = ZoneMeta::getMetazoneMappings(utzid);
		if (metazoneMappings != NULL) {
			for (int32_t i = 0; i < metazoneMappings->size(); i++) {
				const OlsonToMetaMappingEntry *mzmap = 
						(const OlsonToMetaMappingEntry*)metazoneMappings->elementAt(i);
				UnicodeString mzid(mzmap->mzid);
				const ZoneStrings *mzStrings = 
					(const ZoneStrings*)uhash_get(fMzidToStrings, mzid.getTerminatedBuffer());
				if (mzStrings == NULL) {
					// If the metazone strings are not yet processed, do it now.
					char mzidkey[ZID_KEY_MAX];
					uprv_strcpy(mzidkey, gMetazoneIdPrefix);
					u_UCharsToChars(mzmap->mzid, mzidkey + MZID_PREFIX_LEN, u_strlen(mzmap->mzid) + 1);
					fMetazoneItem = ures_getByKeyWithFallback(fZoneStringsArray, mzidkey, fMetazoneItem, &status);
					if (U_FAILURE(status)) {
						// No resources available for this metazone
						// Resource bundle will be cleaned up after end of the loop.
						status = U_ZERO_ERROR;
						continue;
					}
					UBool mzCommonlyUsed = isCommonlyUsed(fMetazoneItem);
					mzstrarray[ZSIDX_LONG_STANDARD] = getZoneStringFromBundle(fMetazoneItem, gLongStandardTag);
					mzstrarray[ZSIDX_SHORT_STANDARD] = getZoneStringFromBundle(fMetazoneItem, gShortStandardTag);
					mzstrarray[ZSIDX_LONG_DAYLIGHT] = getZoneStringFromBundle(fMetazoneItem, gLongDaylightTag);
					mzstrarray[ZSIDX_SHORT_DAYLIGHT] = getZoneStringFromBundle(fMetazoneItem, gShortDaylightTag);
					mzstrarray[ZSIDX_LONG_GENERIC] = getZoneStringFromBundle(fMetazoneItem, gLongGenericTag);
					mzstrarray[ZSIDX_SHORT_GENERIC] = getZoneStringFromBundle(fMetazoneItem, gShortGenericTag);
					mzstrarray[ZSIDX_LOCATION] = NULL;
	
					int32_t lastNonNullIdx = ZSIDX_COUNT - 1;
					while (lastNonNullIdx >= 0) {
						if (mzstrarray[lastNonNullIdx] != NULL) {
							break;
						}
						lastNonNullIdx--;
					}
					UnicodeString *strings_mz = NULL;
					ZoneStrings *tmp_mzStrings = NULL;
					if (lastNonNullIdx >= 0) {
						// Create UnicodeString array and put strings to the zone string trie
						strings_mz = new UnicodeString[lastNonNullIdx + 1];
	
						UnicodeString preferredIdForLocale;
						ZoneMeta::getZoneIdByMetazone(mzid, region, preferredIdForLocale);
	
						for (int32_t typeidx = 0; typeidx <= lastNonNullIdx; typeidx++) {
							if (mzstrarray[typeidx] != NULL) {
								strings_mz[typeidx].setTo(TRUE, mzstrarray[typeidx], -1);
	
								// Add a metazone string to the zone string trie
								int32_t type = getTimeZoneTranslationType((TimeZoneTranslationTypeIndex)typeidx);
								ZoneStringInfo *zsinfo = new ZoneStringInfo(
																preferredIdForLocale, 
																strings_mz[typeidx], 
																(TimeZoneTranslationType)type, 
																fStringPool,
																status);
								fZoneStringsTrie.put(strings_mz[typeidx], zsinfo, fStringPool, status);
								if (U_FAILURE(status)) {
									delete []strings_mz;
									goto error_cleanup;
								}
							}
						}
						// Note: ZoneStrings constructor adopts and deletes the strings_mz array.
						tmp_mzStrings = new ZoneStrings(strings_mz, lastNonNullIdx + 1, 
														mzCommonlyUsed, NULL, 0, 0, fStringPool, status);
					} else {
						// Create ZoneStrings with empty contents
						tmp_mzStrings = new ZoneStrings(NULL, 0, FALSE, NULL, 0, 0, fStringPool, status);
					}
	
					UChar *umzid = const_cast<UChar *>(fStringPool.get(mzid, status));
					uhash_put(fMzidToStrings, umzid, tmp_mzStrings, &status);
					if (U_FAILURE(status)) {
						goto error_cleanup;
					}
	
					mzStrings = tmp_mzStrings;
				}
	
				// Compose generic partial location format
				UnicodeString lg;
				UnicodeString sg;
	
				mzStrings->getString(ZSIDX_LONG_GENERIC, lg);
				mzStrings->getString(ZSIDX_SHORT_GENERIC, sg);
	
				if (!lg.isEmpty() || !sg.isEmpty()) {
					UBool addMzPartialLocationNames = TRUE;
					for (int32_t j = 0; j < mzPartialLocIdx; j++) {
						if (mzPartialLoc[j][0] == mzid) {
							// already processed
							addMzPartialLocationNames = FALSE;
							break;
						}
					}
					if (addMzPartialLocationNames) {
						UnicodeString *locationPart = NULL;
						// Check if the zone is the preferred zone for the territory associated with the zone
						UnicodeString preferredID;
						ZoneMeta::getZoneIdByMetazone(mzid, countryCode, preferredID);
						if (utzid == preferredID) {
							// Use country for the location
							locationPart = &country;
						} else {
							// Use city for the location
							locationPart = &city;
						}
						// Reset the partial location string array
						mzPartialLoc[mzPartialLocIdx][0].setTo(mzid);
						mzPartialLoc[mzPartialLocIdx][1].remove();
						mzPartialLoc[mzPartialLocIdx][2].remove();
						mzPartialLoc[mzPartialLocIdx][3].remove();
	
						if (locationPart->length() != 0) {
							FieldPosition fpos;
							if (!lg.isEmpty()) {
								Formattable params [] = {
									Formattable(*locationPart),
									Formattable(lg)
								};
								fallbackFmt->format(params, 2, mzPartialLoc[mzPartialLocIdx][1], fpos, status);
							}
							if (!sg.isEmpty()) {
								Formattable params [] = {
									Formattable(*locationPart),
									Formattable(sg)
								};
								fallbackFmt->format(params, 2, mzPartialLoc[mzPartialLocIdx][2], fpos, status);
								if (mzStrings->isShortFormatCommonlyUsed()) {
									mzPartialLoc[mzPartialLocIdx][3].setTo(TRUE, gCommonlyUsedTrue, -1);
								}
							}
							if (U_FAILURE(status)) {
								goto error_cleanup;
							}
						}
						mzPartialLocIdx++;
					}
				}
			}
		}
		// Collected names for a zone
	
		// Create UnicodeString array for localized zone strings
		int32_t lastIdx = ZSIDX_COUNT - 1;
		while (lastIdx >= 0) {
			if (zstrarray[lastIdx] != NULL) {
				break;
			}
			lastIdx--;
		}
		UnicodeString *strings = NULL;
		int32_t stringsCount = lastIdx + 1;
	
		if (stringsCount > 0) {
			strings = new UnicodeString[stringsCount];
			for (int32_t i = 0; i < stringsCount; i++) {
				if (zstrarray[i] != NULL) {
					strings[i].setTo(zstrarray[i], -1);
	
					// Add names to the trie
					int32_t type = getTimeZoneTranslationType((TimeZoneTranslationTypeIndex)i);
					ZoneStringInfo *zsinfo = new ZoneStringInfo(utzid, 
																strings[i], 
																(TimeZoneTranslationType)type,
																fStringPool,
																status);
					fZoneStringsTrie.put(strings[i], zsinfo, fStringPool, status);
					if (U_FAILURE(status)) {
						delete zsinfo;
						delete[] strings;
						goto error_cleanup;
					}
				}
			}
		}
	
		// Create UnicodeString array for generic partial location strings
		UnicodeString **genericPartialLocationNames = NULL;
		int32_t genericPartialRowCount = mzPartialLocIdx;
		int32_t genericPartialColCount = 4;
	
		if (genericPartialRowCount != 0) {
			genericPartialLocationNames = 
					 (UnicodeString**)uprv_malloc(genericPartialRowCount * sizeof(UnicodeString*));
			if (genericPartialLocationNames == NULL) {
				status = U_MEMORY_ALLOCATION_ERROR;
				delete[] strings;
				goto error_cleanup;
			}
			for (int32_t i = 0; i < genericPartialRowCount; i++) {
				genericPartialLocationNames[i] = new UnicodeString[genericPartialColCount];
				for (int32_t j = 0; j < genericPartialColCount; j++) {
					genericPartialLocationNames[i][j].setTo(mzPartialLoc[i][j]);
					// Add names to the trie
					if ((j == 1 || j == 2) &&!genericPartialLocationNames[i][j].isEmpty()) {
						ZoneStringInfo *zsinfo;
						TimeZoneTranslationType type = (j == 1) ? GENERIC_LONG : GENERIC_SHORT;
						zsinfo = new ZoneStringInfo(utzid, genericPartialLocationNames[i][j], type, 
													fStringPool, status);
						fZoneStringsTrie.put(genericPartialLocationNames[i][j], zsinfo, fStringPool, status);
						if (U_FAILURE(status)) {
							delete[] genericPartialLocationNames[i];
							uprv_free(genericPartialLocationNames);
							delete[] strings;
							goto error_cleanup;
						}
					}
				}
			}
		}
	
		// Finally, create ZoneStrings instance and put it into the tzidToStinrgs map
		ZoneStrings *zstrings = new ZoneStrings(strings, stringsCount, commonlyUsed,
												genericPartialLocationNames, genericPartialRowCount, 
												genericPartialColCount, fStringPool, status);
	
		UChar *uutzid = const_cast<UChar *>(fStringPool.get(utzid, status));
		uhash_put(fTzidToStrings, uutzid, zstrings, &status);
		if (U_FAILURE(status)) {
			delete zstrings;
			goto error_cleanup;
		}
	}

error_cleanup:
    if (fallbackFmt != NULL) {
        delete fallbackFmt;
    }
    if (regionFmt != NULL) {
        delete regionFmt;
    }
    //	fStringPool.freeze();
}

void
ZoneStringFormat::loadFull(UErrorCode &status)
{
    if (U_FAILURE(status)) {
        return;
    }
	if (fIsFullyLoaded) {
		return;
	}

	umtx_lock(&ZoneStringFormatMutex);

	if (fZoneStringsArray == NULL) {
		fZoneStringsArray = ures_open(U_ICUDATA_ZONE, fLocale.getName(), &status);
		fZoneStringsArray = ures_getByKeyWithFallback(fZoneStringsArray, gZoneStringsTag, fZoneStringsArray, &status);
		if (U_FAILURE(status)) {
			// If no locale bundles are available, zoneStrings will be null.
			// We still want to go through the rest of zone strings initialization,
			// because generic location format is generated from tzid for the case.
			// The rest of code should work even zoneStrings is null.
			status = U_ZERO_ERROR;
			ures_close(fZoneStringsArray);
			fZoneStringsArray = NULL;
		}
    }

    StringEnumeration *tzids = NULL;

    tzids = TimeZone::createEnumeration();
    const char *tzid;
    while ((tzid = tzids->next(NULL, status))) {
        if (U_FAILURE(status)) {
            goto error_cleanup;
        }
        // Skip non-canonical IDs
        UnicodeString utzid(tzid, -1, US_INV);
        UnicodeString canonicalID;
        TimeZone::getCanonicalID(utzid, canonicalID, status);
        if (U_FAILURE(status)) {
            // Ignore unknown ID - we should not get here, but just in case.
            status = U_ZERO_ERROR;
            continue;
        }

		if (U_SUCCESS(status)) {
			if (uhash_count(fTzidToStrings) > 0) {
				ZoneStrings *zstrings = (ZoneStrings*)uhash_get(fTzidToStrings, canonicalID.getTerminatedBuffer());
				if (zstrings != NULL) {
					continue;	//	We already about this one
				}
			}
		}

		addSingleZone(canonicalID, status);
		
        if (U_FAILURE(status)) {
			goto error_cleanup;
		}
    }

	fIsFullyLoaded = TRUE;
	
error_cleanup:
    if (tzids != NULL) {
        delete tzids;
    }
    fStringPool.freeze();

	umtx_unlock(&ZoneStringFormatMutex);
}


ZoneStringFormat::~ZoneStringFormat() {
    uhash_close(fTzidToStrings);
    uhash_close(fMzidToStrings);
    ures_close(fZoneItem);
    ures_close(fMetazoneItem);
    ures_close(fZoneStringsArray);
}

SafeZoneStringFormatPtr*
ZoneStringFormat::getZoneStringFormat(const Locale& locale, UErrorCode &status) {
    umtx_lock(&gZSFCacheLock);
    if (gZoneStringFormatCache == NULL) {
        gZoneStringFormatCache = new ZSFCache(10 /* capacity */);
        ucln_i18n_registerCleanup(UCLN_I18N_ZSFORMAT, zoneStringFormat_cleanup);
    }
    umtx_unlock(&gZSFCacheLock);

    return gZoneStringFormatCache->get(locale, status);
}


UnicodeString**
ZoneStringFormat::createZoneStringsArray(UDate date, int32_t &rowCount, int32_t &colCount, UErrorCode &status) const {
    if (U_FAILURE(status)) {
        return NULL;
    }
	ZoneStringFormat *nonConstThis = const_cast<ZoneStringFormat *>(this);
	nonConstThis->loadFull(status);

    UnicodeString **result = NULL;
    rowCount = 0;
    colCount = 0;

    // Collect canonical time zone IDs
    UVector canonicalIDs(uhash_deleteUnicodeString, uhash_compareUnicodeString, status);
    if (U_FAILURE(status)) {
        return NULL;
    }
    StringEnumeration *tzids = TimeZone::createEnumeration();
    const UChar *tzid;
    while ((tzid = tzids->unext(NULL, status))) {
        if (U_FAILURE(status)) {
            delete tzids;
            return NULL;
        }
        UnicodeString utzid(tzid);
        UnicodeString canonicalID;
        TimeZone::getCanonicalID(UnicodeString(tzid), canonicalID, status);
        if (U_FAILURE(status)) {
            // Ignore unknown ID - we should not get here, but just in case.
            status = U_ZERO_ERROR;
            continue;
        }
        if (utzid == canonicalID) {
            canonicalIDs.addElement(new UnicodeString(utzid), status);
            if (U_FAILURE(status)) {
                delete tzids;
                return NULL;
            }
        }
    }
    delete tzids;

    // Allocate array
    result = (UnicodeString**)uprv_malloc(canonicalIDs.size() * sizeof(UnicodeString*));
    if (result == NULL) {
        status = U_MEMORY_ALLOCATION_ERROR;
        return NULL;
    }
    for (int32_t i = 0; i < canonicalIDs.size(); i++) {
        result[i] = new UnicodeString[8];
        UnicodeString *id = (UnicodeString*)canonicalIDs.elementAt(i);
        result[i][0].setTo(*id);
        getLongStandard(*id, date, result[i][1]);
        getShortStandard(*id, date, FALSE, result[i][2]);
        getLongDaylight(*id, date, result[i][3]);
        getShortDaylight(*id, date, FALSE, result[i][4]);
        getGenericLocation(*id, result[i][5]);
        getLongGenericNonLocation(*id, date, result[i][6]);
        getShortGenericNonLocation(*id, date, FALSE, result[i][7]);
    }

    rowCount = canonicalIDs.size();
    colCount = 8;
    return result;
}

UnicodeString&
ZoneStringFormat::getSpecificLongString(const Calendar &cal, UnicodeString &result,
                                        UErrorCode &status) const {
    result.remove();
    if (U_FAILURE(status)) {
        return result;
    }
    UnicodeString tzid;
    cal.getTimeZone().getID(tzid);
    UDate date = cal.getTime(status);
    if (cal.get(UCAL_DST_OFFSET, status) == 0) {
        return getString(tzid, ZSIDX_LONG_STANDARD, date, FALSE /*not used*/, result);
    } else {
        return getString(tzid, ZSIDX_LONG_DAYLIGHT, date, FALSE /*not used*/, result);
    }
}

UnicodeString&
ZoneStringFormat::getSpecificShortString(const Calendar &cal, UBool commonlyUsedOnly,
                                         UnicodeString &result, UErrorCode &status) const {
    result.remove();
    if (U_FAILURE(status)) {
        return result;
    }
    UnicodeString tzid;
    cal.getTimeZone().getID(tzid);
    UDate date = cal.getTime(status);
    if (cal.get(UCAL_DST_OFFSET, status) == 0) {
        return getString(tzid, ZSIDX_SHORT_STANDARD, date, commonlyUsedOnly, result);
    } else {
        return getString(tzid, ZSIDX_SHORT_DAYLIGHT, date, commonlyUsedOnly, result);
    }
}

UnicodeString&
ZoneStringFormat::getGenericLongString(const Calendar &cal, UnicodeString &result,
                                       UErrorCode &status) const {
    return getGenericString(cal, FALSE /*long*/, FALSE /* not used */, result, status);
}

UnicodeString&
ZoneStringFormat::getGenericShortString(const Calendar &cal, UBool commonlyUsedOnly,
                                        UnicodeString &result, UErrorCode &status) const {
    return getGenericString(cal, TRUE /*short*/, commonlyUsedOnly, result, status);
}

UnicodeString&
ZoneStringFormat::getGenericLocationString(const Calendar &cal, UnicodeString &result,
                                           UErrorCode &status) const {
    UnicodeString tzid;
    cal.getTimeZone().getID(tzid);
    UDate date = cal.getTime(status);
    return getString(tzid, ZSIDX_LOCATION, date, FALSE /*not used*/, result);
}

const ZoneStringInfo*
ZoneStringFormat::findSpecificLong(const UnicodeString &text, int32_t start,
                                   int32_t &matchLength, UErrorCode &status) const {
    return find(text, start, STANDARD_LONG | DAYLIGHT_LONG, matchLength, status);
}

const ZoneStringInfo*
ZoneStringFormat::findSpecificShort(const UnicodeString &text, int32_t start,
                                    int32_t &matchLength, UErrorCode &status) const {
    return find(text, start, STANDARD_SHORT | DAYLIGHT_SHORT, matchLength, status);
}

const ZoneStringInfo*
ZoneStringFormat::findGenericLong(const UnicodeString &text, int32_t start,
                                  int32_t &matchLength, UErrorCode &status) const {
    return find(text, start, GENERIC_LONG | STANDARD_LONG | LOCATION, matchLength, status);
}

const ZoneStringInfo*
ZoneStringFormat::findGenericShort(const UnicodeString &text, int32_t start,
                                   int32_t &matchLength, UErrorCode &status) const {
    return find(text, start, GENERIC_SHORT | STANDARD_SHORT | LOCATION, matchLength, status);
}

const ZoneStringInfo*
ZoneStringFormat::findGenericLocation(const UnicodeString &text, int32_t start,
                                      int32_t &matchLength, UErrorCode &status) const {
    return find(text, start, LOCATION, matchLength, status);
}

UnicodeString&
ZoneStringFormat::getString(const UnicodeString &tzid, TimeZoneTranslationTypeIndex typeIdx, UDate date,
                            UBool commonlyUsedOnly, UnicodeString& result) const {
    UErrorCode status = U_ZERO_ERROR;
    result.remove();
	if (!fIsFullyLoaded) {
		// Lazy loading
		ZoneStringFormat *nonConstThis = const_cast<ZoneStringFormat *>(this);
		nonConstThis->loadZone(tzid, status);
	}

    // ICU's own array does not have entries for aliases
    UnicodeString canonicalID;
    TimeZone::getCanonicalID(tzid, canonicalID, status);
    if (U_FAILURE(status)) {
        // Unknown ID, but users might have their own data.
        canonicalID.setTo(tzid);
    }

    if (uhash_count(fTzidToStrings) > 0) {
        ZoneStrings *zstrings = (ZoneStrings*)uhash_get(fTzidToStrings, canonicalID.getTerminatedBuffer());
        if (zstrings != NULL) {
            switch (typeIdx) {
                case ZSIDX_LONG_STANDARD:
                case ZSIDX_LONG_DAYLIGHT:
                case ZSIDX_LONG_GENERIC:
                case ZSIDX_LOCATION:
                    zstrings->getString(typeIdx, result);
                    break;
                case ZSIDX_SHORT_STANDARD:
                case ZSIDX_SHORT_DAYLIGHT:
                case ZSIDX_COUNT: //added to avoid warning
                case ZSIDX_SHORT_GENERIC:
                    if (!commonlyUsedOnly || zstrings->isShortFormatCommonlyUsed()) {
                        zstrings->getString(typeIdx, result);
                    }
                    break;
                default:
                    break;
            }
        }
    }
    if (result.isEmpty() && uhash_count(fMzidToStrings) > 0 && typeIdx != ZSIDX_LOCATION) {
        // Try metazone
        UnicodeString mzid;
        ZoneMeta::getMetazoneID(canonicalID, date, mzid);
        if (!mzid.isEmpty()) {
            ZoneStrings *mzstrings = (ZoneStrings*)uhash_get(fMzidToStrings, mzid.getTerminatedBuffer());
            if (mzstrings != NULL) {
                switch (typeIdx) {
                    case ZSIDX_LONG_STANDARD:
                    case ZSIDX_LONG_DAYLIGHT:
                    case ZSIDX_LONG_GENERIC:
                    case ZSIDX_LOCATION:
                        mzstrings->getString(typeIdx, result);
                        break;
                    case ZSIDX_SHORT_STANDARD:
                    case ZSIDX_SHORT_DAYLIGHT:
                    case ZSIDX_COUNT: //added to avoid warning
                    case ZSIDX_SHORT_GENERIC:
                        if (!commonlyUsedOnly || mzstrings->isShortFormatCommonlyUsed()) {
                            mzstrings->getString(typeIdx, result);
                        }
                        break;
                    default:
                        break;
                }
            }
        }
    }
    return result;
}

UnicodeString&
ZoneStringFormat::getGenericString(const Calendar &cal, UBool isShort, UBool commonlyUsedOnly,
                                   UnicodeString &result, UErrorCode &status) const {
    result.remove();
    UDate time = cal.getTime(status);
    if (U_FAILURE(status)) {
        return result;
    }
    const TimeZone &tz = cal.getTimeZone();
    UnicodeString tzid;
    tz.getID(tzid);

	if (!fIsFullyLoaded) {
		// Lazy loading
		ZoneStringFormat *nonConstThis = const_cast<ZoneStringFormat *>(this);
		nonConstThis->loadZone(tzid, status);
	}

    // ICU's own array does not have entries for aliases
    UnicodeString canonicalID;
    TimeZone::getCanonicalID(tzid, canonicalID, status);
    if (U_FAILURE(status)) {
        // Unknown ID, but users might have their own data.
        status = U_ZERO_ERROR;
        canonicalID.setTo(tzid);
    }

    ZoneStrings *zstrings = NULL;
    if (uhash_count(fTzidToStrings) > 0) {
        zstrings = (ZoneStrings*)uhash_get(fTzidToStrings, canonicalID.getTerminatedBuffer());
        if (zstrings != NULL) {
            if (isShort) {
                if (!commonlyUsedOnly || zstrings->isShortFormatCommonlyUsed()) {
                    zstrings->getString(ZSIDX_SHORT_GENERIC, result);
                }
            } else {
                zstrings->getString(ZSIDX_LONG_GENERIC, result);
            }
        }
    }
    if (result.isEmpty() && uhash_count(fMzidToStrings) > 0) {
        // try metazone
        int32_t raw, sav;
        UnicodeString mzid;
        ZoneMeta::getMetazoneID(canonicalID, time, mzid);
        if (!mzid.isEmpty()) {
            UBool useStandard = FALSE;
            sav = cal.get(UCAL_DST_OFFSET, status);
            if (U_FAILURE(status)) {
                return result;
            }
            if (sav == 0) {
                useStandard = TRUE;
                // Check if the zone actually uses daylight saving time around the time
                TimeZone *tmptz = tz.clone();
                BasicTimeZone *btz = NULL;
                if (dynamic_cast<OlsonTimeZone *>(tmptz) != NULL
                    || dynamic_cast<SimpleTimeZone *>(tmptz) != NULL
                    || dynamic_cast<RuleBasedTimeZone *>(tmptz) != NULL
                    || dynamic_cast<VTimeZone *>(tmptz) != NULL) {
                    btz = (BasicTimeZone*)tmptz;
                }

                if (btz != NULL) {
                    TimeZoneTransition before;
                    UBool beforTrs = btz->getPreviousTransition(time, TRUE, before);
                    if (beforTrs
                            && (time - before.getTime() < kDstCheckRange)
                            && before.getFrom()->getDSTSavings() != 0) {
                        useStandard = FALSE;
                    } else {
                        TimeZoneTransition after;
                        UBool afterTrs = btz->getNextTransition(time, FALSE, after);
                        if (afterTrs
                                && (after.getTime() - time < kDstCheckRange)
                                && after.getTo()->getDSTSavings() != 0) {
                            useStandard = FALSE;
                        }
                    }
                } else {
                    // If not BasicTimeZone... only if the instance is not an ICU's implementation.
                    // We may get a wrong answer in edge case, but it should practically work OK.
                    tmptz->getOffset(time - kDstCheckRange, FALSE, raw, sav, status);
                    if (sav != 0) {
                        useStandard = FALSE;
                    } else {
                        tmptz->getOffset(time + kDstCheckRange, FALSE, raw, sav, status);
                        if (sav != 0){
                            useStandard = FALSE;
                        }
                    }
                    if (U_FAILURE(status)) {
                        delete tmptz;
                        result.remove();
                        return result;
                    }
                }
                delete tmptz;
            }
            if (useStandard) {
                getString(canonicalID, (isShort ? ZSIDX_SHORT_STANDARD : ZSIDX_LONG_STANDARD),
                    time, commonlyUsedOnly, result);

                // Note:
                // In CLDR 1.5.1, a same localization is used for both generic and standard
                // for some metazones in some locales.  This is actually data bugs and should
                // be resolved in later versions of CLDR.  For now, we check if the standard
                // name is different from its generic name below.
                if (!result.isEmpty()) {
                    UnicodeString genericNonLocation;
                    getString(canonicalID, (isShort ? ZSIDX_SHORT_GENERIC : ZSIDX_LONG_GENERIC),
                        time, commonlyUsedOnly, genericNonLocation);
                    if (!genericNonLocation.isEmpty() && result == genericNonLocation) {
                        result.remove();
                    }
                }
            }
            if (result.isEmpty()) {
                ZoneStrings *mzstrings = (ZoneStrings*)uhash_get(fMzidToStrings, mzid.getTerminatedBuffer());
                if (mzstrings != NULL) {
                    if (isShort) {
                        if (!commonlyUsedOnly || mzstrings->isShortFormatCommonlyUsed()) {
                            mzstrings->getString(ZSIDX_SHORT_GENERIC, result);
                        }
                    } else {
                        mzstrings->getString(ZSIDX_LONG_GENERIC, result);
                    }
                }
                if (!result.isEmpty()) {
                    // Check if the offsets at the given time matches the preferred zone's offsets
                    UnicodeString preferredId;
                    UnicodeString region;
                    ZoneMeta::getZoneIdByMetazone(mzid, getRegion(region), preferredId);
                    if (canonicalID != preferredId) {
                        // Check if the offsets at the given time are identical with the preferred zone
                        raw = cal.get(UCAL_ZONE_OFFSET, status);
                        if (U_FAILURE(status)) {
                            result.remove();
                            return result;
                        }
                        TimeZone *preferredZone = TimeZone::createTimeZone(preferredId);
                        int32_t prfRaw, prfSav;
                        // Check offset in preferred time zone with wall time.
                        // With getOffset(time, false, preferredOffsets),
                        // you may get incorrect results because of time overlap at DST->STD
                        // transition.
                        preferredZone->getOffset(time + raw + sav, TRUE, prfRaw, prfSav, status);
                        delete preferredZone;

                        if (U_FAILURE(status)) {
                            result.remove();
                            return result;
                        }
                        if ((raw != prfRaw || sav != prfSav) && zstrings != NULL) {
                            // Use generic partial location string as fallback
                            zstrings->getGenericPartialLocationString(mzid, isShort, commonlyUsedOnly, result);
                        }
                    }
                }
            }
        }
    }
    if (result.isEmpty()) {
        // Use location format as the final fallback
        getString(canonicalID, ZSIDX_LOCATION, time, FALSE /*not used*/, result);
    }

    return result;
}

UnicodeString&
ZoneStringFormat::getGenericPartialLocationString(const UnicodeString &tzid, UBool isShort,
                                                  UDate date, UBool commonlyUsedOnly, UnicodeString &result) const {
    UErrorCode status = U_ZERO_ERROR;
    result.remove();
	if (!fIsFullyLoaded) {
		// Lazy loading
		ZoneStringFormat *nonConstThis = const_cast<ZoneStringFormat *>(this);
		nonConstThis->loadZone(tzid, status);
	}

    if (uhash_count(fTzidToStrings) <= 0) {
        return result;
    }

    UnicodeString canonicalID;
    TimeZone::getCanonicalID(tzid, canonicalID, status);
    if (U_FAILURE(status)) {
        // Unknown ID, so no corresponding meta data.
        return result;
    }

    UnicodeString mzid;
    ZoneMeta::getMetazoneID(canonicalID, date, mzid);

    if (!mzid.isEmpty()) {
        ZoneStrings *zstrings = (ZoneStrings*)uhash_get(fTzidToStrings, canonicalID.getTerminatedBuffer());
        if (zstrings != NULL) {
            zstrings->getGenericPartialLocationString(mzid, isShort, commonlyUsedOnly, result);
        }
    }
    return result;
}

// This method does lazy zone string loading
const ZoneStringInfo*
ZoneStringFormat::find(const UnicodeString &text, int32_t start, int32_t types,
                       int32_t &matchLength, UErrorCode &status) const {

    if (U_FAILURE(status)) {
        return NULL;
    }

	const ZoneStringInfo *	result = subFind(text, start, types, matchLength, status);
	if (fIsFullyLoaded) {
		return result;
	}
	// When zone string data is partially loaded,
	// this method return the result only when
	// the input text is fully consumed.
	if (result != NULL) {
		UnicodeString tmpString;
		matchLength = (result->getString(tmpString)).length();
		if (text.length() - start == matchLength) {
			return result;
		}
	}
	
	// Now load all zone strings
	ZoneStringFormat *nonConstThis = const_cast<ZoneStringFormat *>(this);
	nonConstThis->loadFull(status);
	
	return subFind(text, start, types, matchLength, status);
}


/*
 * Find a prefix matching time zone for the given zone string types.
 * @param text The text contains a time zone string
 * @param start The start index within the text
 * @param types The bit mask representing a set of requested types
 * @return If any zone string matched for the requested types, returns a
 * ZoneStringInfo for the longest match.  If no matches are found for
 * the requested types, returns a ZoneStringInfo for the longest match
 * for any other types.  If nothing matches at all, returns null.
 */
const ZoneStringInfo*
ZoneStringFormat::subFind(const UnicodeString &text, int32_t start, int32_t types,
                       int32_t &matchLength, UErrorCode &status) const {
    matchLength = 0;
    if (U_FAILURE(status)) {
        return NULL;
    }
    if (fZoneStringsTrie.isEmpty()) {
        return NULL;
    }

    const ZoneStringInfo *result = NULL;
    const ZoneStringInfo *fallback = NULL;
    int32_t fallbackMatchLen = 0;

    ZoneStringSearchResultHandler handler(status);
    fZoneStringsTrie.search(text, start, (TextTrieMapSearchResultHandler*)&handler, status);
    if (U_SUCCESS(status)) {
        int32_t numMatches = handler.countMatches();
        for (int32_t i = 0; i < numMatches; i++) {
            int32_t tmpMatchLen = 0; // init. output only param to silence gcc
            const ZoneStringInfo *tmp = handler.getMatch(i, tmpMatchLen);
            if ((types & tmp->fType) != 0) {
                if (result == NULL || matchLength < tmpMatchLen) {
                    result = tmp;
                    matchLength = tmpMatchLen;
                } else if (matchLength == tmpMatchLen) {
                    // Tie breaker - there are some examples that a
                    // long standard name is identical with a location
                    // name - for example, "Uruguay Time".  In this case,
                    // we interpret it as generic, not specific.
                    if (tmp->isGeneric() && !result->isGeneric()) {
                        result = tmp;
                    }
                }
            } else if (result == NULL) {
                if (fallback == NULL || fallbackMatchLen < tmpMatchLen) {
                    fallback = tmp;
                    fallbackMatchLen = tmpMatchLen;
                } else if (fallbackMatchLen == tmpMatchLen) {
                    if (tmp->isGeneric() && !fallback->isGeneric()) {
                        fallback = tmp;
                    }
                }
            }
        }
        if (result == NULL && fallback != NULL) {
            result = fallback;
            matchLength = fallbackMatchLen;
        }
    }
    return result;
}


UnicodeString&
ZoneStringFormat::getRegion(UnicodeString &region) const {
    const char* country = fLocale.getCountry();
    // TODO: Utilize addLikelySubtag in Locale to resolve default region
    // when the implementation is ready.
    region.setTo(UnicodeString(country, -1, US_INV));
    return region;
}

MessageFormat*
ZoneStringFormat::getFallbackFormat(const Locale &locale, UErrorCode &status) {
    if (U_FAILURE(status)) {
        return NULL;
    }
    UnicodeString pattern(TRUE, gDefFallbackPattern, -1);
    UResourceBundle *zoneStringsArray = ures_open(U_ICUDATA_ZONE, locale.getName(), &status);
    zoneStringsArray = ures_getByKeyWithFallback(zoneStringsArray, gZoneStringsTag, zoneStringsArray, &status);
    int32_t len;
    const UChar *flbkfmt = ures_getStringByKeyWithFallback(zoneStringsArray, gFallbackFormatTag, &len, &status);
    if (U_SUCCESS(status)) {
        pattern.setTo(flbkfmt);
    } else {
        status = U_ZERO_ERROR;
    }
    ures_close(zoneStringsArray);

    MessageFormat *fallbackFmt = new MessageFormat(pattern, status);
    return fallbackFmt;
}

MessageFormat*
ZoneStringFormat::getRegionFormat(const Locale& locale, UErrorCode &status) {
    if (U_FAILURE(status)) {
        return NULL;
    }
    UnicodeString pattern(TRUE, gDefRegionPattern, -1);
    UResourceBundle *zoneStringsArray = ures_open(U_ICUDATA_ZONE, locale.getName(), &status);
    zoneStringsArray = ures_getByKeyWithFallback(zoneStringsArray, gZoneStringsTag, zoneStringsArray, &status);
    int32_t len;
    const UChar *regionfmt = ures_getStringByKeyWithFallback(zoneStringsArray, gRegionFormatTag, &len, &status);
    if (U_SUCCESS(status)) {
        pattern.setTo(regionfmt);
    } else {
        status = U_ZERO_ERROR;
    }
    ures_close(zoneStringsArray);

    MessageFormat *regionFmt = new MessageFormat(pattern, status);
    return regionFmt;
}

const UChar*
ZoneStringFormat::getZoneStringFromBundle(const UResourceBundle *zoneitem, const char *key) {
    const UChar *str = NULL;
    if (zoneitem != NULL) {
        UErrorCode status = U_ZERO_ERROR;
        int32_t len;
        str = ures_getStringByKeyWithFallback(zoneitem, key, &len, &status);
        str = fStringPool.adopt(str, status);
        if (U_FAILURE(status)) {
            str = NULL;
        }
    }
    return str;
}

UBool
ZoneStringFormat::isCommonlyUsed(const UResourceBundle *zoneitem) {
    if (zoneitem == NULL) {
        return TRUE;
    }

    UBool commonlyUsed = FALSE;
    UErrorCode status = U_ZERO_ERROR;
    UResourceBundle *cuRes = ures_getByKey(zoneitem, gCommonlyUsedTag, NULL, &status);
    int32_t cuValue = ures_getInt(cuRes, &status);
    if (U_SUCCESS(status)) {
        if (cuValue == 1) {
            commonlyUsed = TRUE;
        }
    }
    ures_close(cuRes);
    return commonlyUsed;
}

UnicodeString&
ZoneStringFormat::getLocalizedCountry(const UnicodeString &countryCode, const Locale &locale, UnicodeString &displayCountry) {
    // We do not want to use display country names only from the target language bundle
    // Note: we should do this in better way.
    displayCountry.remove();
    int32_t ccLen = countryCode.length();
    if (ccLen > 0 && ccLen < ULOC_COUNTRY_CAPACITY) {
        UErrorCode status = U_ZERO_ERROR;
        UResourceBundle *localeBundle = ures_open(NULL, locale.getName(), &status);
        if (U_SUCCESS(status)) {
            const char *bundleLocStr = ures_getLocale(localeBundle, &status);
            if (U_SUCCESS(status) && uprv_strlen(bundleLocStr) > 0) {
                Locale bundleLoc(bundleLocStr);
                if (uprv_strcmp(bundleLocStr, "root") != 0 && 
                    uprv_strcmp(bundleLoc.getLanguage(), locale.getLanguage()) == 0) {
                    // Create a fake locale strings
                    char tmpLocStr[ULOC_COUNTRY_CAPACITY + 3];
                    uprv_strcpy(tmpLocStr, "xx_");
                    u_UCharsToChars(countryCode.getBuffer(), &tmpLocStr[3], ccLen);
                    tmpLocStr[3 + ccLen] = 0;

                    Locale tmpLoc(tmpLocStr);
                    tmpLoc.getDisplayCountry(locale, displayCountry);
                }
            }
        }
        ures_close(localeBundle);
    }
    if (displayCountry.isEmpty()) {
        // Use the country code as the fallback
        displayCountry.setTo(countryCode);
    }
    return displayCountry;
}

// ----------------------------------------------------------------------------
/*
 * ZoneStrings constructor adopts (and promptly copies and deletes) 
 *    the input UnicodeString arrays.
 */
ZoneStrings::ZoneStrings(UnicodeString *strings, 
                         int32_t stringsCount, 
                         UBool commonlyUsed,
                         UnicodeString **genericPartialLocationStrings, 
                         int32_t genericRowCount, 
                         int32_t genericColCount,
                         ZSFStringPool &sp,
                         UErrorCode &status)
:   fStrings(NULL),
    fStringsCount(stringsCount), 
    fIsCommonlyUsed(commonlyUsed),
    fGenericPartialLocationStrings(NULL), 
    fGenericPartialLocationRowCount(genericRowCount), 
    fGenericPartialLocationColCount(genericColCount) 
{
    if (U_FAILURE(status)) {
        return;
    }
    int32_t i, j;
    if (strings != NULL) {
        fStrings = (const UChar **)uprv_malloc(sizeof(const UChar **) * stringsCount);
        if (fStrings == NULL) {
            status = U_MEMORY_ALLOCATION_ERROR;
            return;
        }
        for (i=0; i<fStringsCount; i++) {
            fStrings[i] = sp.get(strings[i], status);
        }
        delete[] strings;
    }
    if (genericPartialLocationStrings != NULL) {
        fGenericPartialLocationStrings = 
            (const UChar ***)uprv_malloc(sizeof(const UChar ***) * genericRowCount);
        if (fGenericPartialLocationStrings == NULL) {
            status = U_MEMORY_ALLOCATION_ERROR;
            return;
        }
        for (i=0; i < fGenericPartialLocationRowCount; i++) {
            fGenericPartialLocationStrings[i] =
                (const UChar **)uprv_malloc(sizeof(const UChar **) * genericColCount);
            if (fGenericPartialLocationStrings[i] == NULL) {
                status = U_MEMORY_ALLOCATION_ERROR;
                continue;   // Continue so that fGenericPartialLocationStrings will not contain uninitialized junk,
            }               //   which would crash the destructor.
            for (j=0; j<genericColCount; j++) {
                fGenericPartialLocationStrings[i][j] = 
                        sp.get(genericPartialLocationStrings[i][j], status);
            }
            delete[] genericPartialLocationStrings[i];
        }
        uprv_free(genericPartialLocationStrings);
    }
}

ZoneStrings::~ZoneStrings() {
    uprv_free(fStrings);
    if (fGenericPartialLocationStrings != NULL) {
        for (int32_t i = 0; i < fGenericPartialLocationRowCount; i++) {
            uprv_free(fGenericPartialLocationStrings[i]);
        }
        uprv_free(fGenericPartialLocationStrings);
    }
}


UnicodeString&
ZoneStrings::getString(int32_t typeIdx, UnicodeString &result) const {
    if (typeIdx >= 0 && typeIdx < fStringsCount) {
        result.setTo(fStrings[typeIdx], -1);
    } else {
        result.remove();
    }
    return result;
}

UnicodeString&
ZoneStrings::getGenericPartialLocationString(const UnicodeString &mzid, UBool isShort,
                                        UBool commonlyUsedOnly, UnicodeString &result) const {
    UBool isSet = FALSE;
    if (fGenericPartialLocationColCount >= 2) {
        for (int32_t i = 0; i < fGenericPartialLocationRowCount; i++) {
            if (mzid.compare(fGenericPartialLocationStrings[i][0], -1) == 0) {
                if (isShort) {
                    if (fGenericPartialLocationColCount >= 3) {
                        if (!commonlyUsedOnly || 
                            fGenericPartialLocationColCount == 3 || 
                            fGenericPartialLocationStrings[i][3][0] != 0) {
                                result.setTo(fGenericPartialLocationStrings[i][2], -1);
                            isSet = TRUE;
                        }
                    }
                } else {
                    result.setTo(fGenericPartialLocationStrings[i][1], -1);
                    isSet = TRUE;
                }
                break;
            }
        }
    }
    if (!isSet) {
        result.remove();
    }
    return result;
}

// --------------------------------------------------------------
SafeZoneStringFormatPtr::SafeZoneStringFormatPtr(ZSFCacheEntry *cacheEntry)
: fCacheEntry(cacheEntry) {
}

SafeZoneStringFormatPtr::~SafeZoneStringFormatPtr() {
    fCacheEntry->delRef();
}

const ZoneStringFormat*
SafeZoneStringFormatPtr::get() const {
    return fCacheEntry->getZoneStringFormat();
}

ZSFCacheEntry::ZSFCacheEntry(const Locale &locale, ZoneStringFormat *zsf, ZSFCacheEntry *next)
: fLocale(locale), fZoneStringFormat(zsf),
 fNext(next), fRefCount(1)
{
}

ZSFCacheEntry::~ZSFCacheEntry () {
    delete fZoneStringFormat;
}

const ZoneStringFormat*
ZSFCacheEntry::getZoneStringFormat(void) {
    return (const ZoneStringFormat*)fZoneStringFormat;
}

void
ZSFCacheEntry::delRef(void) {
    umtx_lock(&gZSFCacheLock);
    --fRefCount;
    umtx_unlock(&gZSFCacheLock);
}

ZSFCache::ZSFCache(int32_t capacity)
: fCapacity(capacity), fFirst(NULL) {
}

ZSFCache::~ZSFCache() {
    ZSFCacheEntry *entry = fFirst;
    while (entry) {
        ZSFCacheEntry *next = entry->fNext;
        delete entry;
        entry = next;
    }
}

SafeZoneStringFormatPtr*
ZSFCache::get(const Locale &locale, UErrorCode &status) {
    SafeZoneStringFormatPtr *result = NULL;

    // Search the cache entry list
    ZSFCacheEntry *entry = NULL;
    ZSFCacheEntry *next, *prev;

    umtx_lock(&gZSFCacheLock);
    entry = fFirst;
    prev = NULL;
    while (entry) {
        next = entry->fNext;
        if (entry->fLocale == locale) {
            // Add reference count
            entry->fRefCount++;

            // move the entry to the top
            if (entry != fFirst) {
                prev->fNext = next;
                entry->fNext = fFirst;
                fFirst = entry;
            }
            break;
        }
        prev = entry;
        entry = next;
    }
    umtx_unlock(&gZSFCacheLock);

    // Create a new ZoneStringFormat
    if (entry == NULL) {
        ZoneStringFormat *zsf = new ZoneStringFormat(locale, status);
        if (U_FAILURE(status)) {
            delete zsf;
            return NULL;
        }
        if (zsf == NULL) {
            status = U_MEMORY_ALLOCATION_ERROR;
            return NULL;
        }

        // Now add the new entry
        umtx_lock(&gZSFCacheLock);
        // Make sure no other threads already created the one for the same locale
        entry = fFirst;
        prev = NULL;
        while (entry) {
            next = entry->fNext;
            if (entry->fLocale == locale) {
                // Add reference count
                entry->fRefCount++;

                // move the entry to the top
                if (entry != fFirst) {
                    prev->fNext = next;
                    entry->fNext = fFirst;
                    fFirst = entry;
                }
                break;
            }
            prev = entry;
            entry = next;
        }
        if (entry == NULL) {
            // Add the new one to the top
            next = fFirst;
            entry = new ZSFCacheEntry(locale, zsf, next);
            fFirst = entry;
        } else {
            delete zsf;
        }
        umtx_unlock(&gZSFCacheLock);
    }

    result = new SafeZoneStringFormatPtr(entry);

    // Now, delete unused cache entries beyond the capacity
    umtx_lock(&gZSFCacheLock);
    entry = fFirst;
    prev = NULL;
    int32_t idx = 1;
    while (entry) {
        next = entry->fNext;
        if (idx >= fCapacity && entry->fRefCount == 0) {
            if (entry == fFirst) {
                fFirst = next;
            } else {
                prev->fNext = next;
            }
            delete entry;
        } else {
            prev = entry;
        }
        entry = next;
        idx++;
    }
    umtx_unlock(&gZSFCacheLock);

    return result;
}


/*
 * Zone String Formatter String Pool Implementation
 *
 *    String pool for (UChar *) strings.  Avoids having repeated copies of the same string.
 */

static const int32_t POOL_CHUNK_SIZE = 2000;
struct ZSFStringPoolChunk: public UMemory {
    ZSFStringPoolChunk    *fNext;                       // Ptr to next pool chunk
    int32_t               fLimit;                       // Index to start of unused area at end of fStrings
    UChar                 fStrings[POOL_CHUNK_SIZE];    //  Strings array
    ZSFStringPoolChunk();
};

ZSFStringPoolChunk::ZSFStringPoolChunk() {
    fNext = NULL;
    fLimit = 0;
}

ZSFStringPool::ZSFStringPool(UErrorCode &status) {
    fChunks = NULL;
    fHash   = NULL;
    if (U_FAILURE(status)) {
        return;
    }
    fChunks = new ZSFStringPoolChunk;
    if (fChunks == NULL) {
        status = U_MEMORY_ALLOCATION_ERROR;
        return;
    }

    fHash   = uhash_open(uhash_hashUChars      /* keyHash */, 
                         uhash_compareUChars   /* keyComp */, 
                         uhash_compareUChars   /* valueComp */, 
                         &status);
    if (U_FAILURE(status)) {
        return;
    }
}


ZSFStringPool::~ZSFStringPool() {
    if (fHash != NULL) {
        uhash_close(fHash);
        fHash = NULL;
    }

    while (fChunks != NULL) {
        ZSFStringPoolChunk *nextChunk = fChunks->fNext;
        delete fChunks;
        fChunks = nextChunk;
    }
}

static const UChar EmptyString = 0;

const UChar *ZSFStringPool::get(const UChar *s, UErrorCode &status) {
    const UChar *pooledString;
    if (U_FAILURE(status)) {
        return &EmptyString;
    }

    pooledString = static_cast<UChar *>(uhash_get(fHash, s));
    if (pooledString != NULL) {
        return pooledString;
    }

    int32_t length = u_strlen(s);
    int32_t remainingLength = POOL_CHUNK_SIZE - fChunks->fLimit;
    if (remainingLength <= length) {
        U_ASSERT(length < POOL_CHUNK_SIZE);
        if (length >= POOL_CHUNK_SIZE) {
            status = U_INTERNAL_PROGRAM_ERROR;
            return &EmptyString;
        }
        ZSFStringPoolChunk *oldChunk = fChunks;
        fChunks = new ZSFStringPoolChunk;
        if (fChunks == NULL) {
            status = U_MEMORY_ALLOCATION_ERROR;
            return &EmptyString;
        }
        fChunks->fNext = oldChunk;
    }
    
    UChar *destString = &fChunks->fStrings[fChunks->fLimit];
    u_strcpy(destString, s);
    fChunks->fLimit += (length + 1);
    uhash_put(fHash, destString, destString, &status);
    return destString;
}        


//
//  ZSFStringPool::adopt()   Put a string into the hash, but do not copy the string data
//                           into the pool's storage.  Used for strings from resource bundles,
//                           which will perisist for the life of the zone string formatter, and
//                           therefore can be used directly without copying.
const UChar *ZSFStringPool::adopt(const UChar * s, UErrorCode &status) {
    const UChar *pooledString;
    if (U_FAILURE(status)) {
        return &EmptyString;
    }
    if (s != NULL) {
        pooledString = static_cast<UChar *>(uhash_get(fHash, s));
        if (pooledString == NULL) {
            UChar *ncs = const_cast<UChar *>(s);
            uhash_put(fHash, ncs, ncs, &status);
        }
    }
    return s;
}

    
const UChar *ZSFStringPool::get(const UnicodeString &s, UErrorCode &status) {
    UnicodeString &nonConstStr = const_cast<UnicodeString &>(s);
    return this->get(nonConstStr.getTerminatedBuffer(), status);
}

/*
 * freeze().   Close the hash table that maps to the pooled strings.
 *             After freezing, the pool can not be searched or added to,
 *             but all existing references to pooled strings remain valid.
 *
 *             The main purpose is to recover the storage used for the hash.
 */
void ZSFStringPool::freeze() {
    uhash_close(fHash);
    fHash = NULL;
}

U_NAMESPACE_END

#endif /* #if !UCONFIG_NO_FORMATTING */
