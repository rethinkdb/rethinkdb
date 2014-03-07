/*
 ******************************************************************************
 *   Copyright (C) 1996-2010, International Business Machines                 *
 *   Corporation and others.  All Rights Reserved.                            *
 ******************************************************************************
 */

/**
 * \file 
 * \brief C++ API: Collation data used to compute minLengthInChars.
 * \internal
 */
 
#ifndef COLL_DATA_H
#define COLL_DATA_H

#include "unicode/utypes.h"

#if !UCONFIG_NO_COLLATION

#include "unicode/uobject.h"
#include "unicode/ucol.h"

U_NAMESPACE_BEGIN

/**
 * The size of the internal buffer for the Collator's short description string.
 * @internal ICU 4.0.1 technology preview
 */
#define KEY_BUFFER_SIZE 64

 /**
  * The size of the internal CE buffer in a <code>CEList</code> object
  * @internal ICU 4.0.1 technology preview
  */
#define CELIST_BUFFER_SIZE 4

/**
 * \def INSTRUMENT_CELIST
 * Define this to enable the <code>CEList</code> objects to collect
 * statistics.
 * @internal ICU 4.0.1 technology preview
 */
//#define INSTRUMENT_CELIST

 /**
  * The size of the initial list in a <code>StringList</code> object.
  * @internal ICU 4.0.1 technology preview
  */
#define STRING_LIST_BUFFER_SIZE 16

/**
 * \def INSTRUMENT_STRING_LIST
 * Define this to enable the <code>StringList</code> objects to
 * collect statistics.
 * @internal ICU 4.0.1 technology preview
 */
//#define INSTRUMENT_STRING_LIST

 /**
  * This object holds a list of CEs generated from a particular
  * <code>UnicodeString</code>
  *
  * @internal ICU 4.0.1 technology preview
  */
class U_I18N_API CEList : public UObject
{
public:
    /**
     * Construct a <code>CEList</code> object.
     *
     * @param coll - the Collator used to collect the CEs.
     * @param string - the string for which to collect the CEs.
     * @param status - will be set if any errors occur. 
     *
     * Note: if on return, status is set to an error code,
     * the only safe thing to do with this object is to call
     * the destructor.
     *
     * @internal ICU 4.0.1 technology preview
     */
    CEList(UCollator *coll, const UnicodeString &string, UErrorCode &status);

    /**
     * The destructor.
     * @internal ICU 4.0.1 technology preview
     */
    ~CEList();

    /**
     * Return the number of CEs in the list.
     *
     * @return the number of CEs in the list.
     *
     * @internal ICU 4.0.1 technology preview
     */
    int32_t size() const;

    /**
     * Get a particular CE from the list.
     *
     * @param index - the index of the CE to return
     *
     * @return the CE, or <code>0</code> if <code>index</code> is out of range
     *
     * @internal ICU 4.0.1 technology preview
     */
    uint32_t get(int32_t index) const;

    /**
     * Check if the CEs in another <code>CEList</code> match the
     * suffix of this list starting at a give offset.
     *
     * @param offset - the offset of the suffix
     * @param other - the other <code>CEList</code>
     *
     * @return <code>TRUE</code> if the CEs match, <code>FALSE</code> otherwise.
     *
     * @internal ICU 4.0.1 technology preview
     */
    UBool matchesAt(int32_t offset, const CEList *other) const; 

    /**
     * The index operator.
     *
     * @param index - the index
     *
     * @return a reference to the given CE in the list
     *
     * @internal ICU 4.0.1 technology preview
     */
    uint32_t &operator[](int32_t index) const;

    /**
     * UObject glue...
     * @internal ICU 4.0.1 technology preview
     */
    virtual UClassID getDynamicClassID() const;
    /**
     * UObject glue...
     * @internal ICU 4.0.1 technology preview
     */
    static UClassID getStaticClassID();

private:
    void add(uint32_t ce, UErrorCode &status);

    uint32_t ceBuffer[CELIST_BUFFER_SIZE];
    uint32_t *ces;
    int32_t listMax;
    int32_t listSize;

#ifdef INSTRUMENT_CELIST
    static int32_t _active;
    static int32_t _histogram[10];
#endif
};

/**
 * StringList
 *
 * This object holds a list of <code>UnicodeString</code> objects.
 *
 * @internal ICU 4.0.1 technology preview
 */
class U_I18N_API StringList : public UObject
{
public:
    /**
     * Construct an empty <code>StringList</code>
     *
     * @param status - will be set if any errors occur. 
     *
     * Note: if on return, status is set to an error code,
     * the only safe thing to do with this object is to call
     * the destructor.
     *
     * @internal ICU 4.0.1 technology preview
     */
    StringList(UErrorCode &status);

    /**
     * The destructor.
     *
     * @internal ICU 4.0.1 technology preview
     */
    ~StringList();

    /**
     * Add a string to the list.
     *
     * @param string - the string to add
     * @param status - will be set if any errors occur. 
     *
     * @internal ICU 4.0.1 technology preview
     */
    void add(const UnicodeString *string, UErrorCode &status);

    /**
     * Add an array of Unicode code points to the list.
     *
     * @param chars - the address of the array of code points
     * @param count - the number of code points in the array
     * @param status - will be set if any errors occur. 
     *
     * @internal ICU 4.0.1 technology preview
     */
    void add(const UChar *chars, int32_t count, UErrorCode &status);

    /**
     * Get a particular string from the list.
     *
     * @param index - the index of the string
     *
     * @return a pointer to the <code>UnicodeString</code> or <code>NULL</code> 
     *         if <code>index</code> is out of bounds.
     *
     * @internal ICU 4.0.1 technology preview
     */
    const UnicodeString *get(int32_t index) const;

    /**
     * Get the number of stings in the list.
     *
     * @return the number of strings in the list.
     *
     * @internal ICU 4.0.1 technology preview
     */
    int32_t size() const;

    /**
     * the UObject glue...
     * @internal ICU 4.0.1 technology preview
     */
    virtual UClassID getDynamicClassID() const;
    /**
     * the UObject glue...
     * @internal ICU 4.0.1 technology preview
     */
    static UClassID getStaticClassID();

private:
    UnicodeString *strings;
    int32_t listMax;
    int32_t listSize;

#ifdef INSTRUMENT_STRING_LIST
    static int32_t _lists;
    static int32_t _strings;
    static int32_t _histogram[101];
#endif
};

/*
 * Forward references to internal classes.
 */
class StringToCEsMap;
class CEToStringsMap;
class CollDataCache;

/**
 * CollData
 *
 * This class holds the Collator-specific data needed to
 * compute the length of the shortest string that can
 * generate a partcular list of CEs.
 *
 * <code>CollData</code> objects are quite expensive to compute. Because
 * of this, they are cached. When you call <code>CollData::open</code> it
 * returns a reference counted cached object. When you call <code>CollData::close</code>
 * the reference count on the object is decremented but the object is not deleted.
 *
 * If you do not need to reuse any unreferenced objects in the cache, you can call
 * <code>CollData::flushCollDataCache</code>. If you no longer need any <code>CollData</code>
 * objects, you can call <code>CollData::freeCollDataCache</code>
 *
 * @internal ICU 4.0.1 technology preview
 */
class U_I18N_API CollData : public UObject
{
public:
    /**
     * Construct a <code>CollData</code> object.
     *
     * @param collator - the collator
     * @param status - will be set if any errors occur. 
     *
     * @return the <code>CollData</code> object. You must call
     *         <code>close</code> when you are done using the object.
     *
     * Note: if on return, status is set to an error code,
     * the only safe thing to do with this object is to call
     * <code>CollData::close</code>.
     *
     * @internal ICU 4.0.1 technology preview
     */
    static CollData *open(UCollator *collator, UErrorCode &status);

    /**
     * Release a <code>CollData</code> object.
     *
     * @param collData - the object
     *
     * @internal ICU 4.0.1 technology preview
     */
    static void close(CollData *collData);

    /**
     * Get the <code>UCollator</code> object used to create this object.
     * The object returned may not be the exact object that was used to
     * create this object, but it will have the same behavior.
     * @internal ICU 4.0.1 technology preview
     */
    UCollator *getCollator() const;

    /**
     * Get a list of all the strings which generate a list
     * of CEs starting with a given CE.
     *
     * @param ce - the CE
     *
     * return a <code>StringList</code> object containing all
     *        the stirngs, or <code>NULL</code> if there are
     *        no such strings.
     *
     * @internal ICU 4.0.1 technology preview.
     */
    const StringList *getStringList(int32_t ce) const;

    /**
     * Get a list of the CEs generated by a partcular stirng.
     *
     * @param string - the string
     *
     * @return a <code>CEList</code> object containt the CEs. You
     *         must call <code>freeCEList</code> when you are finished
     *         using the <code>CEList</code>/
     *
     * @internal ICU 4.0.1 technology preview.
     */
    const CEList *getCEList(const UnicodeString *string) const;

    /**
     * Release a <code>CEList</code> returned by <code>getCEList</code>.
     *
     * @param list - the <code>CEList</code> to free.
     *
     * @internal ICU 4.0.1 technology preview
     */
    void freeCEList(const CEList *list);

    /**
     * Return the length of the shortest string that will generate
     * the given list of CEs.
     *
     * @param ces - the CEs
     * @param offset - the offset of the first CE in the list to use.
     *
     * @return the length of the shortest string.
     *
     * @internal ICU 4.0.1 technology preview
     */
    int32_t minLengthInChars(const CEList *ces, int32_t offset) const;

 
    /**
     * Return the length of the shortest string that will generate
     * the given list of CEs.
     *
     * Note: the algorithm used to do this computation is recursive. To
     * limit the amount of recursion, a "history" list is used to record
     * the best answer starting at a particular offset in the list of CEs.
     * If the same offset is visited again during the recursion, the answer
     * in the history list is used.
     *
     * @param ces - the CEs
     * @param offset - the offset of the first CE in the list to use.
     * @param history - the history list. Must be at least as long as
     *                 the number of cEs in the <code>CEList</code>
     *
     * @return the length of the shortest string.
     *
     * @internal ICU 4.0.1 technology preview
     */
   int32_t minLengthInChars(const CEList *ces, int32_t offset, int32_t *history) const;

   /**
    * UObject glue...
    * @internal ICU 4.0.1 technology preview
    */
    virtual UClassID getDynamicClassID() const;
   /**
    * UObject glue...
    * @internal ICU 4.0.1 technology preview
    */
    static UClassID getStaticClassID();

    /**
     * <code>CollData</code> objects are expensive to compute, and so
     * may be cached. This routine will free the cached objects and delete
     * the cache.
     *
     * WARNING: Don't call this until you are have called <code>close</code>
     * for each <code>CollData</code> object that you have used. also,
     * DO NOT call this if another thread may be calling <code>flushCollDataCache</code>
     * at the same time.
     *
     * @internal 4.0.1 technology preview
     */
    static void freeCollDataCache();

    /**
     * <code>CollData</code> objects are expensive to compute, and so
     * may be cached. This routine will remove any unused <code>CollData</code>
     * objects from the cache.
     *
     * @internal 4.0.1 technology preview
     */
    static void flushCollDataCache();

private:
    friend class CollDataCache;
    friend class CollDataCacheEntry;

    CollData(UCollator *collator, char *cacheKey, int32_t cachekeyLength, UErrorCode &status);
    ~CollData();

    CollData();

    static char *getCollatorKey(UCollator *collator, char *buffer, int32_t bufferLength);

    static CollDataCache *getCollDataCache();

    UCollator      *coll;
    StringToCEsMap *charsToCEList;
    CEToStringsMap *ceToCharsStartingWith;

    char keyBuffer[KEY_BUFFER_SIZE];
    char *key;

    static CollDataCache *collDataCache;

    uint32_t minHan;
    uint32_t maxHan;

    uint32_t jamoLimits[4];
};

U_NAMESPACE_END

#endif // #if !UCONFIG_NO_COLLATION
#endif // #ifndef COLL_DATA_H
