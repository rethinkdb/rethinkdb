/*
 ******************************************************************************
 *   Copyright (C) 1996-2009, International Business Machines                 *
 *   Corporation and others.  All Rights Reserved.                            *
 ******************************************************************************
 */

#include "unicode/utypes.h"

#if !UCONFIG_NO_COLLATION

#include "unicode/unistr.h"
#include "unicode/putil.h"
#include "unicode/usearch.h"

#include "cmemory.h"
#include "unicode/coll.h"
#include "unicode/tblcoll.h"
#include "unicode/coleitr.h"
#include "unicode/ucoleitr.h"

#include "unicode/regex.h"        // TODO: make conditional on regexp being built.

#include "unicode/uniset.h"
#include "unicode/uset.h"
#include "unicode/ustring.h"
#include "hash.h"
#include "uhash.h"
#include "ucln_in.h"
#include "ucol_imp.h"
#include "umutex.h"

#include "unicode/colldata.h"

U_NAMESPACE_BEGIN

#define ARRAY_SIZE(array) (sizeof(array)/sizeof(array[0]))
#define NEW_ARRAY(type, count) (type *) uprv_malloc((count) * sizeof(type))
#define DELETE_ARRAY(array) uprv_free((void *) (array))
#define ARRAY_COPY(dst, src, count) uprv_memcpy((void *) (dst), (void *) (src), (count) * sizeof (src)[0])

UOBJECT_DEFINE_RTTI_IMPLEMENTATION(CEList)

#ifdef INSTRUMENT_CELIST
int32_t CEList::_active = 0;
int32_t CEList::_histogram[10] = {0, 0, 0, 0, 0, 0, 0, 0, 0, 0};
#endif

CEList::CEList(UCollator *coll, const UnicodeString &string, UErrorCode &status)
    : ces(NULL), listMax(CELIST_BUFFER_SIZE), listSize(0)
{
    UCollationElements *elems = ucol_openElements(coll, string.getBuffer(), string.length(), &status);
    UCollationStrength strength = ucol_getStrength(coll);
    UBool toShift = ucol_getAttribute(coll, UCOL_ALTERNATE_HANDLING, &status) ==  UCOL_SHIFTED;
    uint32_t variableTop = ucol_getVariableTop(coll, &status);
    uint32_t strengthMask = 0;
    int32_t order;

    if (U_FAILURE(status)) {
        return;
    }

    // **** only set flag if string has Han(gul) ****
    ucol_forceHanImplicit(elems, &status);

    switch (strength)
    {
    default:
        strengthMask |= UCOL_TERTIARYORDERMASK;
        /* fall through */

    case UCOL_SECONDARY:
        strengthMask |= UCOL_SECONDARYORDERMASK;
        /* fall through */

    case UCOL_PRIMARY:
        strengthMask |= UCOL_PRIMARYORDERMASK;
    }

#ifdef INSTRUMENT_CELIST
    _active += 1;
    _histogram[0] += 1;
#endif

    ces = ceBuffer;

    while ((order = ucol_next(elems, &status)) != UCOL_NULLORDER) {
        UBool cont = isContinuation(order);

        order &= strengthMask;

        if (toShift && variableTop > (uint32_t)order && (order & UCOL_PRIMARYORDERMASK) != 0) {
            if (strength >= UCOL_QUATERNARY) {
                order &= UCOL_PRIMARYORDERMASK;
            } else {
                order = UCOL_IGNORABLE;
            }
        }

        if (order == UCOL_IGNORABLE) {
            continue;
        }

        if (cont) {
            order |= UCOL_CONTINUATION_MARKER;
        }

        add(order, status);
    }

    ucol_closeElements(elems);
}

CEList::~CEList()
{
#ifdef INSTRUMENT_CELIST
    _active -= 1;
#endif

    if (ces != ceBuffer) {
        DELETE_ARRAY(ces);
    }
}

void CEList::add(uint32_t ce, UErrorCode &status)
{
    if (U_FAILURE(status)) {
        return;
    }

    if (listSize >= listMax) {
        int32_t newMax = listMax + CELIST_BUFFER_SIZE;

#ifdef INSTRUMENT_CELIST
        _histogram[listSize / CELIST_BUFFER_SIZE] += 1;
#endif

        uint32_t *newCEs = NEW_ARRAY(uint32_t, newMax);

        if (newCEs == NULL) {
            status = U_MEMORY_ALLOCATION_ERROR;
            return;
        }

        uprv_memcpy(newCEs, ces, listSize * sizeof(uint32_t));

        if (ces != ceBuffer) {
            DELETE_ARRAY(ces);
        }

        ces = newCEs;
        listMax = newMax;
    }

    ces[listSize++] = ce;
}

uint32_t CEList::get(int32_t index) const
{
    if (index >= 0 && index < listSize) {
        return ces[index];
    }

    return UCOL_NULLORDER;
}

uint32_t &CEList::operator[](int32_t index) const
{
    return ces[index];
}

UBool CEList::matchesAt(int32_t offset, const CEList *other) const
{
    if (other == NULL || listSize - offset < other->size()) {
        return FALSE;
    }

    for (int32_t i = offset, j = 0; j < other->size(); i += 1, j += 1) {
        if (ces[i] != (*other)[j]) {
            return FALSE;
        }
    }

    return TRUE;
}

int32_t CEList::size() const
{
    return listSize;
}

UOBJECT_DEFINE_RTTI_IMPLEMENTATION(StringList)

#ifdef INSTRUMENT_STRING_LIST
int32_t StringList::_lists = 0;
int32_t StringList::_strings = 0;
int32_t StringList::_histogram[101] = {0};
#endif

StringList::StringList(UErrorCode &status)
    : strings(NULL), listMax(STRING_LIST_BUFFER_SIZE), listSize(0)
{
    if (U_FAILURE(status)) {
        return;
    }

    strings = new UnicodeString [listMax];

    if (strings == NULL) {
        status = U_MEMORY_ALLOCATION_ERROR;
        return;
    }

#ifdef INSTRUMENT_STRING_LIST
    _lists += 1;
    _histogram[0] += 1;
#endif
}

StringList::~StringList()
{
    delete[] strings;
}

void StringList::add(const UnicodeString *string, UErrorCode &status)
{
    if (U_FAILURE(status)) {
        return;
    }

#ifdef INSTRUMENT_STRING_LIST
    _strings += 1;
#endif

    if (listSize >= listMax) {
        int32_t newMax = listMax + STRING_LIST_BUFFER_SIZE;

        UnicodeString *newStrings = new UnicodeString[newMax];
        if (newStrings == NULL) {
            status = U_MEMORY_ALLOCATION_ERROR;
            return;
        }
        for (int32_t i=0; i<listSize; ++i) {
            newStrings[i] = strings[i];
        }

#ifdef INSTRUMENT_STRING_LIST
        int32_t _h = listSize / STRING_LIST_BUFFER_SIZE;

        if (_h > 100) {
            _h = 100;
        }

        _histogram[_h] += 1;
#endif

        delete[] strings;
        strings = newStrings;
        listMax = newMax;
    }

    // The ctor initialized all the strings in
    // the array to empty strings, so this
    // is the same as copying the source string.
    strings[listSize++].append(*string);
}

void StringList::add(const UChar *chars, int32_t count, UErrorCode &status)
{
    const UnicodeString string(chars, count);

    add(&string, status);
}

const UnicodeString *StringList::get(int32_t index) const
{
    if (index >= 0 && index < listSize) {
        return &strings[index];
    }

    return NULL;
}

int32_t StringList::size() const
{
    return listSize;
}


U_CFUNC void deleteStringList(void *obj);

class CEToStringsMap : public UMemory
{
public:

    CEToStringsMap(UErrorCode &status);
    ~CEToStringsMap();

    void put(uint32_t ce, UnicodeString *string, UErrorCode &status);
    StringList *getStringList(uint32_t ce) const;

private:

    void putStringList(uint32_t ce, StringList *stringList, UErrorCode &status);
    UHashtable *map;
};

CEToStringsMap::CEToStringsMap(UErrorCode &status)
    : map(NULL)
{
    if (U_FAILURE(status)) {
        return;
    }

    map = uhash_open(uhash_hashLong, uhash_compareLong,
                     uhash_compareCaselessUnicodeString,
                     &status);

    if (U_FAILURE(status)) {
        return;
    }

    uhash_setValueDeleter(map, deleteStringList);
}

CEToStringsMap::~CEToStringsMap()
{
    uhash_close(map);
}

void CEToStringsMap::put(uint32_t ce, UnicodeString *string, UErrorCode &status)
{
    StringList *strings = getStringList(ce);

    if (strings == NULL) {
        strings = new StringList(status);

        if (strings == NULL || U_FAILURE(status)) {
            status = U_MEMORY_ALLOCATION_ERROR;
            return;
        }

        putStringList(ce, strings, status);
    }

    strings->add(string, status);
}

StringList *CEToStringsMap::getStringList(uint32_t ce) const
{
    return (StringList *) uhash_iget(map, ce);
}

void CEToStringsMap::putStringList(uint32_t ce, StringList *stringList, UErrorCode &status)
{
    uhash_iput(map, ce, (void *) stringList, &status);
}

U_CFUNC void deleteStringList(void *obj)
{
    StringList *strings = (StringList *) obj;

    delete strings;
}

U_CFUNC void deleteCEList(void *obj);
U_CFUNC void deleteUnicodeStringKey(void *obj);

class StringToCEsMap : public UMemory
{
public:
    StringToCEsMap(UErrorCode &status);
    ~StringToCEsMap();

    void put(const UnicodeString *string, const CEList *ces, UErrorCode &status);
    const CEList *get(const UnicodeString *string);
    void free(const CEList *list);

private:


    UHashtable *map;
};

StringToCEsMap::StringToCEsMap(UErrorCode &status)
    : map(NULL)
{
    if (U_FAILURE(status)) {
        return;
    }

    map = uhash_open(uhash_hashUnicodeString,
                     uhash_compareUnicodeString,
                     uhash_compareLong,
                     &status);

    if (U_FAILURE(status)) {
        return;
    }

    uhash_setValueDeleter(map, deleteCEList);
    uhash_setKeyDeleter(map, deleteUnicodeStringKey);
}

StringToCEsMap::~StringToCEsMap()
{
    uhash_close(map);
}

void StringToCEsMap::put(const UnicodeString *string, const CEList *ces, UErrorCode &status)
{
    uhash_put(map, (void *) string, (void *) ces, &status);
}

const CEList *StringToCEsMap::get(const UnicodeString *string)
{
    return (const CEList *) uhash_get(map, string);
}

U_CFUNC void deleteCEList(void *obj)
{
    CEList *list = (CEList *) obj;

    delete list;
}

U_CFUNC void deleteUnicodeStringKey(void *obj)
{
    UnicodeString *key = (UnicodeString *) obj;

    delete key;
}

class CollDataCacheEntry : public UMemory
{
public:
    CollDataCacheEntry(CollData *theData);
    ~CollDataCacheEntry();

    CollData *data;
    int32_t   refCount;
};

CollDataCacheEntry::CollDataCacheEntry(CollData *theData)
    : data(theData), refCount(1)
{
    // nothing else to do
}

CollDataCacheEntry::~CollDataCacheEntry()
{
    // check refCount?
    delete data;
}

class CollDataCache : public UMemory
{
public:
    CollDataCache(UErrorCode &status);
    ~CollDataCache();

    CollData *get(UCollator *collator, UErrorCode &status);
    void unref(CollData *collData);

    void flush();

private:
    static char *getKey(UCollator *collator, char *keyBuffer, int32_t *charBufferLength);
    static void deleteKey(char *key);

    UMTX lock;
    UHashtable *cache;
};

U_CFUNC void deleteChars(void * /*obj*/)
{
    // char *chars = (char *) obj;
    // All the key strings are owned by the
    // CollData objects and don't need to
    // be freed here.
  //DELETE_ARRAY(chars);
}

U_CFUNC void deleteCollDataCacheEntry(void *obj)
{
    CollDataCacheEntry *entry = (CollDataCacheEntry *) obj;

    delete entry;
}

CollDataCache::CollDataCache(UErrorCode &status)
    : lock(0), cache(NULL)
{
    if (U_FAILURE(status)) {
        return;
    }

    cache = uhash_open(uhash_hashChars, uhash_compareChars, uhash_compareLong, &status);

    if (U_FAILURE(status)) {
        return;
    }

    uhash_setValueDeleter(cache, deleteCollDataCacheEntry);
    uhash_setKeyDeleter(cache, deleteChars);
}

CollDataCache::~CollDataCache()
{
    umtx_lock(&lock);
    uhash_close(cache);
    cache = NULL;
    umtx_unlock(&lock);

    umtx_destroy(&lock);
}

CollData *CollDataCache::get(UCollator *collator, UErrorCode &status)
{
    char keyBuffer[KEY_BUFFER_SIZE];
    int32_t keyLength = KEY_BUFFER_SIZE;
    char *key = getKey(collator, keyBuffer, &keyLength);
    CollData *result = NULL, *newData = NULL;
    CollDataCacheEntry *entry = NULL, *newEntry = NULL;

    umtx_lock(&lock);
    entry = (CollDataCacheEntry *) uhash_get(cache, key);

    if (entry == NULL) {
        umtx_unlock(&lock);

        newData = new CollData(collator, key, keyLength, status);
        newEntry = new CollDataCacheEntry(newData);

        if (U_FAILURE(status) || newData == NULL || newEntry == NULL) {
            status = U_MEMORY_ALLOCATION_ERROR;
            return NULL;
        }

        umtx_lock(&lock);
        entry = (CollDataCacheEntry *) uhash_get(cache, key);

        if (entry == NULL) {
            uhash_put(cache, newData->key, newEntry, &status);
            umtx_unlock(&lock);

            if (U_FAILURE(status)) {
                delete newEntry;
                delete newData;

                return NULL;
            }

            return newData;
        }
    }

    result = entry->data;
    entry->refCount += 1;
    umtx_unlock(&lock);

    if (key != keyBuffer) {
        deleteKey(key);
    }

    if (newEntry != NULL) {
        delete newEntry;
        delete newData;
    }

    return result;
}

void CollDataCache::unref(CollData *collData)
{
    CollDataCacheEntry *entry = NULL;

    umtx_lock(&lock);
    entry = (CollDataCacheEntry *) uhash_get(cache, collData->key);

    if (entry != NULL) {
        entry->refCount -= 1;
    }
    umtx_unlock(&lock);
}

char *CollDataCache::getKey(UCollator *collator, char *keyBuffer, int32_t *keyBufferLength)
{
    UErrorCode status = U_ZERO_ERROR;
    int32_t len = ucol_getShortDefinitionString(collator, NULL, keyBuffer, *keyBufferLength, &status);

    if (len >= *keyBufferLength) {
        *keyBufferLength = (len + 2) & ~1;  // round to even length, leaving room for terminating null
        keyBuffer = NEW_ARRAY(char, *keyBufferLength);
        status = U_ZERO_ERROR;

        len = ucol_getShortDefinitionString(collator, NULL, keyBuffer, *keyBufferLength, &status);
    }

    keyBuffer[len] = '\0';

    return keyBuffer;
}

void CollDataCache::flush()
{
    const UHashElement *element;
    int32_t pos = -1;

    umtx_lock(&lock);
    while ((element = uhash_nextElement(cache, &pos)) != NULL) {
        CollDataCacheEntry *entry = (CollDataCacheEntry *) element->value.pointer;

        if (entry->refCount <= 0) {
            uhash_removeElement(cache, element);
        }
    }
    umtx_unlock(&lock);
}

void CollDataCache::deleteKey(char *key)
{
    DELETE_ARRAY(key);
}

U_CDECL_BEGIN
static UBool coll_data_cleanup(void) {
    CollData::freeCollDataCache();
  return TRUE;
}
U_CDECL_END

UOBJECT_DEFINE_RTTI_IMPLEMENTATION(CollData)

CollData::CollData()
{
    // nothing
}

#define CLONE_COLLATOR

//#define CACHE_CELISTS
CollData::CollData(UCollator *collator, char *cacheKey, int32_t cacheKeyLength, UErrorCode &status)
    : coll(NULL), charsToCEList(NULL), ceToCharsStartingWith(NULL), key(NULL)
{
    // [:c:] == [[:cn:][:cc:][:co:][:cf:][:cs:]]
    // i.e. other, control, private use, format, surrogate
    U_STRING_DECL(test_pattern, "[[:assigned:]-[:c:]]", 20);
    U_STRING_INIT(test_pattern, "[[:assigned:]-[:c:]]", 20);
    USet *charsToTest = uset_openPattern(test_pattern, 20, &status);

    // Han ext. A, Han, Jamo, Hangul, Han Ext. B
    // i.e. all the characers we handle implicitly
    U_STRING_DECL(remove_pattern, "[[\\u3400-\\u9FFF][\\u1100-\\u11F9][\\uAC00-\\uD7AF][\\U00020000-\\U0002A6DF]]", 70);
    U_STRING_INIT(remove_pattern, "[[\\u3400-\\u9FFF][\\u1100-\\u11F9][\\uAC00-\\uD7AF][\\U00020000-\\U0002A6DF]]", 70);
    USet *charsToRemove = uset_openPattern(remove_pattern, 70, &status);

    if (U_FAILURE(status)) {
        return;
    }

    USet *expansions   = uset_openEmpty();
    USet *contractions = uset_openEmpty();
    int32_t itemCount;

#ifdef CACHE_CELISTS
    charsToCEList = new StringToCEsMap(status);

    if (U_FAILURE(status)) {
        goto bail;
    }
#else
    charsToCEList = NULL;
#endif

    ceToCharsStartingWith = new CEToStringsMap(status);

    if (U_FAILURE(status)) {
        goto bail;
    }

    if (cacheKeyLength > KEY_BUFFER_SIZE) {
        key = NEW_ARRAY(char, cacheKeyLength);

        if (key == NULL) {
            status = U_MEMORY_ALLOCATION_ERROR;
            goto bail;
        }
    } else {
        key = keyBuffer;
    }

    ARRAY_COPY(key, cacheKey, cacheKeyLength);

#ifdef CLONE_COLLATOR
    coll = ucol_safeClone(collator, NULL, NULL, &status);

    if (U_FAILURE(status)) {
        goto bail;
    }
#else
    coll = collator;
#endif

    ucol_getContractionsAndExpansions(coll, contractions, expansions, FALSE, &status);

    uset_addAll(charsToTest, contractions);
    uset_addAll(charsToTest, expansions);
    uset_removeAll(charsToTest, charsToRemove);

    itemCount = uset_getItemCount(charsToTest);
    for(int32_t item = 0; item < itemCount; item += 1) {
        UChar32 start = 0, end = 0;
        UChar buffer[16];
        int32_t len = uset_getItem(charsToTest, item, &start, &end,
                                   buffer, 16, &status);

        if (len == 0) {
            for (UChar32 ch = start; ch <= end; ch += 1) {
                UnicodeString *st = new UnicodeString(ch);

                if (st == NULL) {
                    status = U_MEMORY_ALLOCATION_ERROR;
                    break;
                }

                CEList *ceList = new CEList(coll, *st, status);

                ceToCharsStartingWith->put(ceList->get(0), st, status);

#ifdef CACHE_CELISTS
                charsToCEList->put(st, ceList, status);
#else
                delete ceList;
                delete st;
#endif
            }
        } else if (len > 0) {
            UnicodeString *st = new UnicodeString(buffer, len);

            if (st == NULL) {
                status = U_MEMORY_ALLOCATION_ERROR;
                break;
            }

            CEList *ceList = new CEList(coll, *st, status);

            ceToCharsStartingWith->put(ceList->get(0), st, status);

#ifdef CACHE_CELISTS
            charsToCEList->put(st, ceList, status);
#else
            delete ceList;
            delete st;
#endif
        } else {
            // shouldn't happen...
        }

        if (U_FAILURE(status)) {
             break;
        }
    }

bail:
    uset_close(contractions);
    uset_close(expansions);
    uset_close(charsToRemove);
    uset_close(charsToTest);

    if (U_FAILURE(status)) {
        return;
    }

     UChar32 hanRanges[] = {UCOL_FIRST_HAN, UCOL_LAST_HAN, UCOL_FIRST_HAN_COMPAT, UCOL_LAST_HAN_COMPAT, UCOL_FIRST_HAN_A, UCOL_LAST_HAN_A,
                            UCOL_FIRST_HAN_B, UCOL_LAST_HAN_B};
     UChar  jamoRanges[] = {UCOL_FIRST_L_JAMO, UCOL_FIRST_V_JAMO, UCOL_FIRST_T_JAMO, UCOL_LAST_T_JAMO};
     UnicodeString hanString = UnicodeString::fromUTF32(hanRanges, ARRAY_SIZE(hanRanges));
     UnicodeString jamoString(FALSE, jamoRanges, ARRAY_SIZE(jamoRanges));
     CEList hanList(coll, hanString, status);
     CEList jamoList(coll, jamoString, status);
     int32_t j = 0;

     if (U_FAILURE(status)) {
         return;
     }

     for (int32_t c = 0; c < jamoList.size(); c += 1) {
         uint32_t jce = jamoList[c];

         if (! isContinuation(jce)) {
             jamoLimits[j++] = jce;
         }
     }

     jamoLimits[3] += (1 << UCOL_PRIMARYORDERSHIFT);

     minHan = 0xFFFFFFFF;
     maxHan = 0;

     for(int32_t h = 0; h < hanList.size(); h += 2) {
         uint32_t han = (uint32_t) hanList[h];

         if (han < minHan) {
             minHan = han;
         }

         if (han > maxHan) {
             maxHan = han;
         }
     }

     maxHan += (1 << UCOL_PRIMARYORDERSHIFT);
}

CollData::~CollData()
{
#ifdef CLONE_COLLATOR
   ucol_close(coll);
#endif

   if (key != keyBuffer) {
       DELETE_ARRAY(key);
   }

   delete ceToCharsStartingWith;

#ifdef CACHE_CELISTS
   delete charsToCEList;
#endif
}

UCollator *CollData::getCollator() const
{
    return coll;
}

const StringList *CollData::getStringList(int32_t ce) const
{
    return ceToCharsStartingWith->getStringList(ce);
}

const CEList *CollData::getCEList(const UnicodeString *string) const
{
#ifdef CACHE_CELISTS
    return charsToCEList->get(string);
#else
    UErrorCode status = U_ZERO_ERROR;
    const CEList *list = new CEList(coll, *string, status);

    if (U_FAILURE(status)) {
        delete list;
        list = NULL;
    }

    return list;
#endif
}

void CollData::freeCEList(const CEList *list)
{
#ifndef CACHE_CELISTS
    delete list;
#endif
}

int32_t CollData::minLengthInChars(const CEList *ceList, int32_t offset, int32_t *history) const
{
    // find out shortest string for the longest sequence of ces.
    // this can probably be folded with the minLengthCache...

    if (history[offset] >= 0) {
        return history[offset];
    }

    uint32_t ce = ceList->get(offset);
    int32_t maxOffset = ceList->size();
    int32_t shortestLength = INT32_MAX;
    const StringList *strings = ceToCharsStartingWith->getStringList(ce);

    if (strings != NULL) {
        int32_t stringCount = strings->size();

        for (int32_t s = 0; s < stringCount; s += 1) {
            const UnicodeString *string = strings->get(s);
#ifdef CACHE_CELISTS
            const CEList *ceList2 = charsToCEList->get(string);
#else
            UErrorCode status = U_ZERO_ERROR;
            const CEList *ceList2 = new CEList(coll, *string, status);

            if (U_FAILURE(status)) {
                delete ceList2;
                ceList2 = NULL;
            }
#endif

            if (ceList->matchesAt(offset, ceList2)) {
                int32_t clength = ceList2->size();
                int32_t slength = string->length();
                int32_t roffset = offset + clength;
                int32_t rlength = 0;

                if (roffset < maxOffset) {
                    rlength = minLengthInChars(ceList, roffset, history);

                    if (rlength <= 0) {
                    // delete before continue to avoid memory leak.
#ifndef CACHE_CELISTS
                        delete ceList2;
#endif
                        // ignore any dead ends
                        continue;
                    }
                }

                if (shortestLength > slength + rlength) {
                    shortestLength = slength + rlength;
                }
            }

#ifndef CACHE_CELISTS
            delete ceList2;
#endif
        }
    }

    if (shortestLength == INT32_MAX) {
        // No matching strings at this offset. See if
        // the CE is in a range we can handle manually.
        if (ce >= minHan && ce < maxHan) {
            // all han have implicit orders which
            // generate two CEs.
            int32_t roffset = offset + 2;
            int32_t rlength = 0;

          //history[roffset++] = -1;
          //history[roffset++] = 1;

            if (roffset < maxOffset) {
                rlength = minLengthInChars(ceList, roffset, history);
            }

            if (rlength < 0) {
                return -1;
            }

            shortestLength = 1 + rlength;
            goto have_shortest;
        } else if (ce >= jamoLimits[0] && ce < jamoLimits[3]) {
            int32_t roffset = offset;
            int32_t rlength = 0;

            // **** this loop may not handle archaic Hangul correctly ****
            for (int32_t j = 0; roffset < maxOffset && j < 4; j += 1, roffset += 1) {
                uint32_t jce = ceList->get(roffset);

                // Some Jamo have 24-bit primary order; skip the
                // 2nd CE. This should always be OK because if
                // we're still in the loop all we've seen are
                // a series of Jamo in LVT order.
                if (isContinuation(jce)) {
                    continue;
                }

                if (j >= 3 || jce < jamoLimits[j] || jce >= jamoLimits[j + 1]) {
                    break;
                }
            }

            if (roffset == offset) {
                // we started with a non-L Jamo...
                // just say it comes from a single character
                roffset += 1;

                // See if the single Jamo has a 24-bit order.
                if (roffset < maxOffset && isContinuation(ceList->get(roffset))) {
                    roffset += 1;
                }
            }

            if (roffset < maxOffset) {
                rlength = minLengthInChars(ceList, roffset, history);
            }

            if (rlength < 0) {
                return -1;
            }

            shortestLength = 1 + rlength;
            goto have_shortest;
        }

        // Can't handle it manually either. Just move on.
        return -1;
    }

have_shortest:
    history[offset] = shortestLength;

    return shortestLength;
}

int32_t CollData::minLengthInChars(const CEList *ceList, int32_t offset) const
{
    int32_t clength = ceList->size();
    int32_t *history = NEW_ARRAY(int32_t, clength);

    for (int32_t i = 0; i < clength; i += 1) {
        history[i] = -1;
    }

    int32_t minLength = minLengthInChars(ceList, offset, history);

    DELETE_ARRAY(history);

    return minLength;
}

CollData *CollData::open(UCollator *collator, UErrorCode &status)
{
    if (U_FAILURE(status)) {
        return NULL;
    }

    CollDataCache *cache = getCollDataCache();

    return cache->get(collator, status);
}

void CollData::close(CollData *collData)
{
    CollDataCache *cache = getCollDataCache();

    cache->unref(collData);
}

CollDataCache *CollData::collDataCache = NULL;

CollDataCache *CollData::getCollDataCache()
{
    UErrorCode status = U_ZERO_ERROR;
    CollDataCache *cache = NULL;

    UMTX_CHECK(NULL, collDataCache, cache);

    if (cache == NULL) {
        cache = new CollDataCache(status);

        if (U_FAILURE(status)) {
            delete cache;
            return NULL;
        }

        umtx_lock(NULL);
        if (collDataCache == NULL) {
            collDataCache = cache;

            ucln_i18n_registerCleanup(UCLN_I18N_COLL_DATA, coll_data_cleanup);
        }
        umtx_unlock(NULL);

        if (collDataCache != cache) {
            delete cache;
        }
    }

    return collDataCache;
}

void CollData::freeCollDataCache()
{
    CollDataCache *cache = NULL;

    UMTX_CHECK(NULL, collDataCache, cache);

    if (cache != NULL) {
        umtx_lock(NULL);
        if (collDataCache != NULL) {
            collDataCache = NULL;
        } else {
            cache = NULL;
        }
        umtx_unlock(NULL);

        delete cache;
    }
}

void CollData::flushCollDataCache()
{
    CollDataCache *cache = NULL;

    UMTX_CHECK(NULL, collDataCache, cache);

    // **** this will fail if the another ****
    // **** thread deletes the cache here ****
    if (cache != NULL) {
        cache->flush();
    }
}

U_NAMESPACE_END

#endif // #if !UCONFIG_NO_COLLATION
