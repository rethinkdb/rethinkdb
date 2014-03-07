/*
******************************************************************************
*                                                                            *
* Copyright (C) 1999-2009, International Business Machines                   *
*                Corporation and others. All Rights Reserved.                *
*                                                                            *
******************************************************************************
*   file name:  uresdata.h
*   encoding:   US-ASCII
*   tab size:   8 (not used)
*   indentation:4
*
*   created on: 1999dec08
*   created by: Markus W. Scherer
*   06/24/02    weiv        Added support for resource sharing
*/

#ifndef __RESDATA_H__
#define __RESDATA_H__

#include "unicode/utypes.h"
#include "unicode/udata.h"
#include "unicode/ures.h"
#include "udataswp.h"

/**
 * Numeric constants for internal-only types of resource items.
 * These must use different numeric values than UResType constants
 * because they are used together.
 * Internal types are never returned by ures_getType().
 */
typedef enum {
    /** Include a negative value so that the compiler uses the same int type as for UResType. */
    URES_INTERNAL_NONE=-1,

    /** Resource type constant for tables with 32-bit count, key offsets and values. */
    URES_TABLE32=4,

    /**
     * Resource type constant for tables with 16-bit count, key offsets and values.
     * All values are URES_STRING_V2 strings.
     */
    URES_TABLE16=5,

    /** Resource type constant for 16-bit Unicode strings in formatVersion 2. */
    URES_STRING_V2=6,

    /**
     * Resource type constant for arrays with 16-bit count and values.
     * All values are URES_STRING_V2 strings.
     */
    URES_ARRAY16=9
} UResInternalType;

/*
 * A Resource is a 32-bit value that has 2 bit fields:
 * 31..28   4-bit type, see enum below
 * 27..0    28-bit four-byte-offset or value according to the type
 */
typedef uint32_t Resource;

#define RES_BOGUS 0xffffffff

#define RES_GET_TYPE(res) ((UResType)((res)>>28UL))
#define RES_GET_OFFSET(res) ((res)&0x0fffffff)
#define RES_GET_POINTER(pRoot, res) ((pRoot)+RES_GET_OFFSET(res))

/* get signed and unsigned integer values directly from the Resource handle */
#define RES_GET_INT(res) (((int32_t)((res)<<4L))>>4L)
#define RES_GET_UINT(res) ((res)&0x0fffffff)

#define URES_IS_ARRAY(type) ((type)==URES_ARRAY || (type)==URES_ARRAY16)
#define URES_IS_TABLE(type) ((type)==URES_TABLE || (type)==URES_TABLE16 || (type)==URES_TABLE32)
#define URES_IS_CONTAINER(type) (URES_IS_TABLE(type) || URES_IS_ARRAY(type))

#define URES_MAKE_RESOURCE(type, offset) (((Resource)(type)<<28)|(Resource)(offset))
#define URES_MAKE_EMPTY_RESOURCE(type) ((Resource)(type)<<28)

/* indexes[] value names; indexes are generally 32-bit (Resource) indexes */
enum {
    URES_INDEX_LENGTH,          /* [0] contains URES_INDEX_TOP==the length of indexes[];
                                 *     formatVersion==1: all bits contain the length of indexes[]
                                 *       but the length is much less than 0xff;
                                 *     formatVersion>1:
                                 *       only bits  7..0 contain the length of indexes[],
                                 *            bits 31..8 are reserved and set to 0 */
    URES_INDEX_KEYS_TOP,        /* [1] contains the top of the key strings, */
                                /*     same as the bottom of resources or UTF-16 strings, rounded up */
    URES_INDEX_RESOURCES_TOP,   /* [2] contains the top of all resources */
    URES_INDEX_BUNDLE_TOP,      /* [3] contains the top of the bundle, */
                                /*     in case it were ever different from [2] */
    URES_INDEX_MAX_TABLE_LENGTH,/* [4] max. length of any table */
    URES_INDEX_ATTRIBUTES,      /* [5] attributes bit set, see URES_ATT_* (new in formatVersion 1.2) */
    URES_INDEX_16BIT_TOP,       /* [6] top of the 16-bit units (UTF-16 string v2 UChars, URES_TABLE16, URES_ARRAY16),
                                 *     rounded up (new in formatVersion 2.0, ICU 4.4) */
    URES_INDEX_POOL_CHECKSUM,   /* [7] checksum of the pool bundle (new in formatVersion 2.0, ICU 4.4) */
    URES_INDEX_TOP
};

/*
 * Nofallback attribute, attribute bit 0 in indexes[URES_INDEX_ATTRIBUTES].
 * New in formatVersion 1.2 (ICU 3.6).
 *
 * If set, then this resource bundle is a standalone bundle.
 * If not set, then the bundle participates in locale fallback, eventually
 * all the way to the root bundle.
 * If indexes[] is missing or too short, then the attribute cannot be determined
 * reliably. Dependency checking should ignore such bundles, and loading should
 * use fallbacks.
 */
#define URES_ATT_NO_FALLBACK 1

/*
 * Attributes for bundles that are, or use, a pool bundle.
 * A pool bundle provides key strings that are shared among several other bundles
 * to reduce their total size.
 * New in formatVersion 2 (ICU 4.4).
 */
#define URES_ATT_IS_POOL_BUNDLE 2
#define URES_ATT_USES_POOL_BUNDLE 4

/*
 * File format for .res resource bundle files (formatVersion=2, ICU 4.4)
 *
 * New in formatVersion 2 compared with 1.3: -------------
 *
 * Three new resource types -- String-v2, Table16 and Array16 -- have their
 * values stored in a new array of 16-bit units between the table key strings
 * and the start of the other resources.
 *
 * genrb eliminates duplicates among Unicode string-v2 values.
 * Multiple Unicode strings may use the same offset and string data,
 * or a short string may point to the suffix of a longer string. ("Suffix sharing")
 * For example, one string "abc" may be reused for another string "bc" by pointing
 * to the second character. (Short strings-v2 are NUL-terminated
 * and not preceded by an explicit length value.)
 *
 * It is allowed for all resource types to share values.
 * The swapper code (ures_swap()) has been modified so that it swaps each item
 * exactly once.
 *
 * A resource bundle may use a special pool bundle. Some or all of the table key strings
 * of the using-bundle are omitted, and the key string offsets for such key strings refer
 * to offsets in the pool bundle.
 * The using-bundle's and the pool-bundle's indexes[URES_INDEX_POOL_CHECKSUM] values
 * must match.
 * Two bits in indexes[URES_INDEX_ATTRIBUTES] indicate whether a resource bundle
 * is or uses a pool bundle.
 *
 * Table key strings must be compared in ASCII order, even if they are not
 * stored in ASCII.
 *
 * New in formatVersion 1.3 compared with 1.2: -------------
 *
 * genrb eliminates duplicates among key strings.
 * Multiple table items may share one key string, or one item may point
 * to the suffix of another's key string. ("Suffix sharing")
 * For example, one key "abc" may be reused for another key "bc" by pointing
 * to the second character. (Key strings are NUL-terminated.)
 *
 * -------------
 *
 * An ICU4C resource bundle file (.res) is a binary, memory-mappable file
 * with nested, hierarchical data structures.
 * It physically contains the following:
 *
 *   Resource root; -- 32-bit Resource item, root item for this bundle's tree;
 *                     currently, the root item must be a table or table32 resource item
 *   int32_t indexes[indexes[0]]; -- array of indexes for friendly
 *                                   reading and swapping; see URES_INDEX_* above
 *                                   new in formatVersion 1.1 (ICU 2.8)
 *   char keys[]; -- characters for key strings
 *                   (formatVersion 1.0: up to 65k of characters; 1.1: <2G)
 *                   (minus the space for root and indexes[]),
 *                   which consist of invariant characters (ASCII/EBCDIC) and are NUL-terminated;
 *                   padded to multiple of 4 bytes for 4-alignment of the following data
 *   uint16_t 16BitUnits[]; -- resources that are stored entirely as sequences of 16-bit units
 *                             (new in formatVersion 2/ICU 4.4)
 *                             data is indexed by the offset values in 16-bit resource types,
 *                             with offset 0 pointing to the beginning of this array;
 *                             there is a 0 at offset 0, for empty resources;
 *                             padded to multiple of 4 bytes for 4-alignment of the following data
 *   data; -- data directly and indirectly indexed by the root item;
 *            the structure is determined by walking the tree
 *
 * Each resource bundle item has a 32-bit Resource handle (see typedef above)
 * which contains the item type number in its upper 4 bits (31..28) and either
 * an offset or a direct value in its lower 28 bits (27..0).
 * The order of items is undefined and only determined by walking the tree.
 * Leaves of the tree may be stored first or last or anywhere in between,
 * and it is in theory possible to have unreferenced holes in the file.
 *
 * 16-bit-unit values:
 * Starting with formatVersion 2/ICU 4.4, some resources are stored in a special
 * array of 16-bit units. Each resource value is a sequence of 16-bit units,
 * with no per-resource padding to a 4-byte boundary.
 * 16-bit container types (Table16 and Array16) contain Resource16 values
 * which are offsets to String-v2 resources in the same 16-bit-units array.
 *
 * Direct values:
 * - Empty Unicode strings have an offset value of 0 in the Resource handle itself.
 * - Starting with formatVersion 2/ICU 4.4, an offset value of 0 for
 *   _any_ resource type indicates an empty value.
 * - Integer values are 28-bit values stored in the Resource handle itself;
 *   the interpretation of unsigned vs. signed integers is up to the application.
 *
 * All other types and values use 28-bit offsets to point to the item's data.
 * The offset is an index to the first 32-bit word of the value, relative to the
 * start of the resource data (i.e., the root item handle is at offset 0).
 * To get byte offsets, the offset is multiplied by 4 (or shifted left by 2 bits).
 * All resource item values are 4-aligned.
 *
 * New in formatVersion 2/ICU 4.4: Some types use offsets into the 16-bit-units array,
 * indexing 16-bit units in that array.
 *
 * The structures (memory layouts) for the values for each item type are listed
 * in the table below.
 *
 * Nested, hierarchical structures: -------------
 *
 * Table items contain key-value pairs where the keys are offsets to char * key strings.
 * The values of these pairs are either Resource handles or
 * offsets into the 16-bit-units array, depending on the table type.
 *
 * Array items are simple vectors of Resource handles,
 * or of offsets into the 16-bit-units array, depending on the array type.
 *
 * Table key string offsets: -------
 *
 * Key string offsets are relative to the start of the resource data (of the root handle),
 * i.e., the first string has an offset of 4+sizeof(indexes).
 * (After the 4-byte root handle and after the indexes array.)
 *
 * If the resource bundle uses a pool bundle, then some key strings are stored
 * in the pool bundle rather than in the local bundle itself.
 * - In a Table or Table16, the 16-bit key string offset is local if it is
 *   less than indexes[URES_INDEX_KEYS_TOP]<<2.
 *   Otherwise, subtract indexes[URES_INDEX_KEYS_TOP]<<2 to get the offset into
 *   the pool bundle key strings.
 * - In a Table32, the 32-bit key string offset is local if it is non-negative.
 *   Otherwise, reset bit 31 to get the pool key string offset.
 *
 * Unlike the local offset, the pool key offset is relative to
 * the start of the key strings, not to the start of the bundle.
 *
 * An alias item is special (and new in ICU 2.4): --------------
 *
 * Its memory layout is just like for a UnicodeString, but at runtime it resolves to
 * another resource bundle's item according to the path in the string.
 * This is used to share items across bundles that are in different lookup/fallback
 * chains (e.g., large collation data among zh_TW and zh_HK).
 * This saves space (for large items) and maintenance effort (less duplication of data).
 *
 * --------------------------------------------------------------------------
 *
 * Resource types:
 *
 * Most resources have their values stored at four-byte offsets from the start
 * of the resource data. These values are at least 4-aligned.
 * Some resource values are stored directly in the offset field of the Resource itself.
 * See UResType in unicode/ures.h for enumeration constants for Resource types.
 *
 * Some resources have their values stored as sequences of 16-bit units,
 * at 2-byte offsets from the start of a contiguous 16-bit-unit array between
 * the table key strings and the other resources. (new in formatVersion 2/ICU 4.4)
 * At offset 0 of that array is a 16-bit zero value for empty 16-bit resources.
 * Resource16 values in Table16 and Array16 are 16-bit offsets to String-v2
 * resources, with the offsets relative to the start of the 16-bit-units array.
 *
 * Type Name            Memory layout of values
 *                      (in parentheses: scalar, non-offset values)
 *
 * 0  Unicode String:   int32_t length, UChar[length], (UChar)0, (padding)
 *                  or  (empty string ("") if offset==0)
 * 1  Binary:           int32_t length, uint8_t[length], (padding)
 *                      - the start of the bytes is 16-aligned -
 * 2  Table:            uint16_t count, uint16_t keyStringOffsets[count], (uint16_t padding), Resource[count]
 * 3  Alias:            (physically same value layout as string, new in ICU 2.4)
 * 4  Table32:          int32_t count, int32_t keyStringOffsets[count], Resource[count]
 *                      (new in formatVersion 1.1/ICU 2.8)
 * 5  Table16:          uint16_t count, uint16_t keyStringOffsets[count], Resource16[count]
 *                      (stored in the 16-bit-units array; new in formatVersion 2/ICU 4.4)
 * 6  Unicode String-v2:UChar[length], (UChar)0; length determined by the first UChar:
 *                      - if first is not a trail surrogate, then the length is implicit
 *                        and u_strlen() needs to be called
 *                      - if first<0xdfef then length=first&0x3ff (and skip first)
 *                      - if first<0xdfff then length=((first-0xdfef)<<16) | second UChar
 *                      - if first==0xdfff then length=((second UChar)<<16) | third UChar
 *                      (stored in the 16-bit-units array; new in formatVersion 2/ICU 4.4)
 * 7  Integer:          (28-bit offset is integer value)
 * 8  Array:            int32_t count, Resource[count]
 * 9  Array16:          uint16_t count, Resource16[count]
 *                      (stored in the 16-bit-units array; new in formatVersion 2/ICU 4.4)
 * 14 Integer Vector:   int32_t length, int32_t[length]
 * 15 Reserved:         This value denotes special purpose resources and is for internal use.
 *
 * Note that there are 3 types with data vector values:
 * - Vectors of 8-bit bytes stored as type Binary.
 * - Vectors of 16-bit words stored as type Unicode String or Unicode String-v2
 *                     (no value restrictions, all values 0..ffff allowed!).
 * - Vectors of 32-bit words stored as type Integer Vector.
 */

/*
 * Structure for a single, memory-mapped ResourceBundle.
 */
typedef struct {
    UDataMemory *data;
    const int32_t *pRoot;
    const uint16_t *p16BitUnits;
    const char *poolBundleKeys;
    Resource rootRes;
    int32_t localKeyLimit;
    UBool noFallback; /* see URES_ATT_NO_FALLBACK */
    UBool isPoolBundle;
    UBool usesPoolBundle;
    UBool useNativeStrcmp;
} ResourceData;

/*
 * Read a resource bundle from memory.
 */
U_INTERNAL void U_EXPORT2
res_read(ResourceData *pResData,
         const UDataInfo *pInfo, const void *inBytes, int32_t length,
         UErrorCode *errorCode);

/*
 * Load a resource bundle file.
 * The ResourceData structure must be allocated externally.
 */
U_CFUNC void
res_load(ResourceData *pResData,
         const char *path, const char *name, UErrorCode *errorCode);

/*
 * Release a resource bundle file.
 * This does not release the ResourceData structure itself.
 */
U_CFUNC void
res_unload(ResourceData *pResData);

U_INTERNAL UResType U_EXPORT2
res_getPublicType(Resource res);

/*
 * Return a pointer to a zero-terminated, const UChar* string
 * and set its length in *pLength.
 * Returns NULL if not found.
 */
U_INTERNAL const UChar * U_EXPORT2
res_getString(const ResourceData *pResData, Resource res, int32_t *pLength);

U_INTERNAL const UChar * U_EXPORT2
res_getAlias(const ResourceData *pResData, Resource res, int32_t *pLength);

U_INTERNAL const uint8_t * U_EXPORT2
res_getBinary(const ResourceData *pResData, Resource res, int32_t *pLength);

U_INTERNAL const int32_t * U_EXPORT2
res_getIntVector(const ResourceData *pResData, Resource res, int32_t *pLength);

U_INTERNAL Resource U_EXPORT2
res_getResource(const ResourceData *pResData, const char *key);

U_INTERNAL int32_t U_EXPORT2
res_countArrayItems(const ResourceData *pResData, Resource res);

U_INTERNAL Resource U_EXPORT2
res_getArrayItem(const ResourceData *pResData, Resource array, int32_t indexS);

U_INTERNAL Resource U_EXPORT2
res_getTableItemByIndex(const ResourceData *pResData, Resource table, int32_t indexS, const char ** key);

U_INTERNAL Resource U_EXPORT2
res_getTableItemByKey(const ResourceData *pResData, Resource table, int32_t *indexS, const char* * key);

/*
 * Modifies the contents of *path (replacing separators with NULs),
 * and also moves *path forward while it finds items.
 */
U_CFUNC Resource res_findResource(const ResourceData *pResData, Resource r, char** path, const char** key);

/**
 * Swap an ICU resource bundle. See udataswp.h.
 * @internal
 */
U_CAPI int32_t U_EXPORT2
ures_swap(const UDataSwapper *ds,
          const void *inData, int32_t length, void *outData,
          UErrorCode *pErrorCode);

#endif
