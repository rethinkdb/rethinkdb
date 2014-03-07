/*
*******************************************************************************
*
*   Copyright (C) 2000-2010, International Business Machines
*   Corporation and others.  All Rights Reserved.
*
*******************************************************************************
*
* File reslist.c
*
* Modification History:
*
*   Date        Name        Description
*   02/21/00    weiv        Creation.
*******************************************************************************
*/

#include <assert.h>
#include <stdio.h>
#include "reslist.h"
#include "unewdata.h"
#include "unicode/ures.h"
#include "unicode/putil.h"
#include "errmsg.h"

#include "uarrsort.h"
#include "uinvchar.h"

/*
 * Align binary data at a 16-byte offset from the start of the resource bundle,
 * to be safe for any data type it may contain.
 */
#define BIN_ALIGNMENT 16

static UBool gIncludeCopyright = FALSE;
static UBool gUsePoolBundle = FALSE;
static int32_t gFormatVersion = 2;

static UChar gEmptyString = 0;

/* How do we store string values? */
enum {
    STRINGS_UTF16_V1,   /* formatVersion 1: int length + UChars + NUL + padding to 4 bytes */
    STRINGS_UTF16_V2    /* formatVersion 2: optional length in 1..3 UChars + UChars + NUL */
};

enum {
    MAX_IMPLICIT_STRING_LENGTH = 40  /* do not store the length explicitly for such strings */
};

/*
 * res_none() returns the address of kNoResource,
 * for use in non-error cases when no resource is to be added to the bundle.
 * (NULL is used in error cases.)
 */
static const struct SResource kNoResource = { URES_NONE };

static UDataInfo dataInfo= {
    sizeof(UDataInfo),
    0,

    U_IS_BIG_ENDIAN,
    U_CHARSET_FAMILY,
    sizeof(UChar),
    0,

    {0x52, 0x65, 0x73, 0x42},     /* dataFormat="ResB" */
    {1, 3, 0, 0},                 /* formatVersion */
    {1, 4, 0, 0}                  /* dataVersion take a look at version inside parsed resb*/
};

static const UVersionInfo gFormatVersions[3] = {  /* indexed by a major-formatVersion integer */
    { 0, 0, 0, 0 },
    { 1, 3, 0, 0 },
    { 2, 0, 0, 0 }
};

static uint8_t calcPadding(uint32_t size) {
    /* returns space we need to pad */
    return (uint8_t) ((size % sizeof(uint32_t)) ? (sizeof(uint32_t) - (size % sizeof(uint32_t))) : 0);

}

void setIncludeCopyright(UBool val){
    gIncludeCopyright=val;
}

UBool getIncludeCopyright(void){
    return gIncludeCopyright;
}

void setFormatVersion(int32_t formatVersion) {
    gFormatVersion = formatVersion;
}

void setUsePoolBundle(UBool use) {
    gUsePoolBundle = use;
}

static void
bundle_compactStrings(struct SRBRoot *bundle, UErrorCode *status);

/* Writing Functions */

/*
 * type_write16() functions write resource values into f16BitUnits
 * and determine the resource item word, if possible.
 */
static void
res_write16(struct SRBRoot *bundle, struct SResource *res,
            UErrorCode *status);

/*
 * type_preWrite() functions calculate ("preflight") and advance the *byteOffset
 * by the size of their data in the binary file and
 * determine the resource item word.
 * Most type_preWrite() functions may add any number of bytes, but res_preWrite()
 * will always pad it to a multiple of 4.
 * The resource item type may be a related subtype of the fType.
 *
 * The type_preWrite() and type_write() functions start and end at the same
 * byteOffset values.
 * Prewriting allows bundle_write() to determine the root resource item word,
 * before actually writing the bundle contents to the file,
 * which is necessary because the root item is stored at the beginning.
 */
static void
res_preWrite(uint32_t *byteOffset,
             struct SRBRoot *bundle, struct SResource *res,
             UErrorCode *status);

/*
 * type_write() functions write their data to mem and update the byteOffset
 * in parallel.
 * (A kingdom for C++ and polymorphism...)
 */
static void
res_write(UNewDataMemory *mem, uint32_t *byteOffset,
          struct SRBRoot *bundle, struct SResource *res,
          UErrorCode *status);

static uint16_t *
reserve16BitUnits(struct SRBRoot *bundle, int32_t length, UErrorCode *status) {
    if (U_FAILURE(*status)) {
        return NULL;
    }
    if ((bundle->f16BitUnitsLength + length) > bundle->f16BitUnitsCapacity) {
        uint16_t *newUnits;
        int32_t capacity = 2 * bundle->f16BitUnitsCapacity + length + 1024;
        capacity &= ~1;  /* ensures padding fits if f16BitUnitsLength needs it */
        newUnits = (uint16_t *)uprv_malloc(capacity * 2);
        if (newUnits == NULL) {
            *status = U_MEMORY_ALLOCATION_ERROR;
            return NULL;
        }
        if (bundle->f16BitUnitsLength > 0) {
            uprv_memcpy(newUnits, bundle->f16BitUnits, bundle->f16BitUnitsLength * 2);
        } else {
            newUnits[0] = 0;
            bundle->f16BitUnitsLength = 1;
        }
        uprv_free(bundle->f16BitUnits);
        bundle->f16BitUnits = newUnits;
        bundle->f16BitUnitsCapacity = capacity;
    }
    return bundle->f16BitUnits + bundle->f16BitUnitsLength;
}

static int32_t
makeRes16(uint32_t resWord) {
    uint32_t type, offset;
    if (resWord == 0) {
        return 0;  /* empty string */
    }
    type = RES_GET_TYPE(resWord);
    offset = RES_GET_OFFSET(resWord);
    if (type == URES_STRING_V2 && offset <= 0xffff) {
        return (int32_t)offset;
    }
    return -1;
}

static int32_t
mapKey(struct SRBRoot *bundle, int32_t oldpos) {
    const KeyMapEntry *map = bundle->fKeyMap;
    int32_t i, start, limit;

    /* do a binary search for the old, pre-bundle_compactKeys() key offset */
    start = bundle->fPoolBundleKeysCount;
    limit = start + bundle->fKeysCount;
    while (start < limit - 1) {
        i = (start + limit) / 2;
        if (oldpos < map[i].oldpos) {
            limit = i;
        } else {
            start = i;
        }
    }
    assert(oldpos == map[start].oldpos);
    return map[start].newpos;
}

static uint16_t
makeKey16(struct SRBRoot *bundle, int32_t key) {
    if (key >= 0) {
        return (uint16_t)key;
    } else {
        return (uint16_t)(key + bundle->fLocalKeyLimit);  /* offset in the pool bundle */
    }
}

/*
 * Only called for UTF-16 v1 strings and duplicate UTF-16 v2 strings.
 * For unique UTF-16 v2 strings, res_write16() sees fRes != RES_BOGUS
 * and exits early.
 */
static void
string_write16(struct SRBRoot *bundle, struct SResource *res, UErrorCode *status) {
    struct SResource *same;
    if ((same = res->u.fString.fSame) != NULL) {
        /* This is a duplicate. */
        if (same->fRes == RES_BOGUS) {
            /* The original has not been visited yet. */
            string_write16(bundle, same, status);
        }
        res->fRes = same->fRes;
        res->fWritten = same->fWritten;
    }
}

static void
array_write16(struct SRBRoot *bundle, struct SResource *res,
              UErrorCode *status) {
    struct SResource *current;
    int32_t res16 = 0;

    if (U_FAILURE(*status)) {
        return;
    }
    if (res->u.fArray.fCount == 0 && gFormatVersion > 1) {
        res->fRes = URES_MAKE_EMPTY_RESOURCE(URES_ARRAY);
        res->fWritten = TRUE;
        return;
    }
    for (current = res->u.fArray.fFirst; current != NULL; current = current->fNext) {
        res_write16(bundle, current, status);
        res16 |= makeRes16(current->fRes);
    }
    if (U_SUCCESS(*status) && res->u.fArray.fCount <= 0xffff && res16 >= 0 && gFormatVersion > 1) {
        uint16_t *p16 = reserve16BitUnits(bundle, 1 + res->u.fArray.fCount, status);
        if (U_SUCCESS(*status)) {
            res->fRes = URES_MAKE_RESOURCE(URES_ARRAY16, bundle->f16BitUnitsLength);
            *p16++ = (uint16_t)res->u.fArray.fCount;
            for (current = res->u.fArray.fFirst; current != NULL; current = current->fNext) {
                *p16++ = (uint16_t)makeRes16(current->fRes);
            }
            bundle->f16BitUnitsLength += 1 + res->u.fArray.fCount;
            res->fWritten = TRUE;
        }
    }
}

static void
table_write16(struct SRBRoot *bundle, struct SResource *res,
              UErrorCode *status) {
    struct SResource *current;
    int32_t maxKey = 0, maxPoolKey = 0x80000000;
    int32_t res16 = 0;
    UBool hasLocalKeys = FALSE, hasPoolKeys = FALSE;

    if (U_FAILURE(*status)) {
        return;
    }
    if (res->u.fTable.fCount == 0 && gFormatVersion > 1) {
        res->fRes = URES_MAKE_EMPTY_RESOURCE(URES_TABLE);
        res->fWritten = TRUE;
        return;
    }
    /* Find the smallest table type that fits the data. */
    for (current = res->u.fTable.fFirst; current != NULL; current = current->fNext) {
        int32_t key;
        res_write16(bundle, current, status);
        if (bundle->fKeyMap == NULL) {
            key = current->fKey;
        } else {
            key = current->fKey = mapKey(bundle, current->fKey);
        }
        if (key >= 0) {
            hasLocalKeys = TRUE;
            if (key > maxKey) {
                maxKey = key;
            }
        } else {
            hasPoolKeys = TRUE;
            if (key > maxPoolKey) {
                maxPoolKey = key;
            }
        }
        res16 |= makeRes16(current->fRes);
    }
    if (U_FAILURE(*status)) {
        return;
    }
    if(res->u.fTable.fCount > (uint32_t)bundle->fMaxTableLength) {
        bundle->fMaxTableLength = res->u.fTable.fCount;
    }
    maxPoolKey &= 0x7fffffff;
    if (res->u.fTable.fCount <= 0xffff &&
        (!hasLocalKeys || maxKey < bundle->fLocalKeyLimit) &&
        (!hasPoolKeys || maxPoolKey < (0x10000 - bundle->fLocalKeyLimit))
    ) {
        if (res16 >= 0 && gFormatVersion > 1) {
            uint16_t *p16 = reserve16BitUnits(bundle, 1 + res->u.fTable.fCount * 2, status);
            if (U_SUCCESS(*status)) {
                /* 16-bit count, key offsets and values */
                res->fRes = URES_MAKE_RESOURCE(URES_TABLE16, bundle->f16BitUnitsLength);
                *p16++ = (uint16_t)res->u.fTable.fCount;
                for (current = res->u.fTable.fFirst; current != NULL; current = current->fNext) {
                    *p16++ = makeKey16(bundle, current->fKey);
                }
                for (current = res->u.fTable.fFirst; current != NULL; current = current->fNext) {
                    *p16++ = (uint16_t)makeRes16(current->fRes);
                }
                bundle->f16BitUnitsLength += 1 + res->u.fTable.fCount * 2;
                res->fWritten = TRUE;
            }
        } else {
            /* 16-bit count, 16-bit key offsets, 32-bit values */
            res->u.fTable.fType = URES_TABLE;
        }
    } else {
        /* 32-bit count, key offsets and values */
        res->u.fTable.fType = URES_TABLE32;
    }
}

static void
res_write16(struct SRBRoot *bundle, struct SResource *res,
            UErrorCode *status) {
    if (U_FAILURE(*status) || res == NULL) {
        return;
    }
    if (res->fRes != RES_BOGUS) {
        /*
         * The resource item word was already precomputed, which means
         * no further data needs to be written.
         * This might be an integer, or an empty or UTF-16 v2 string,
         * an empty binary, etc.
         */
        return;
    }
    switch (res->fType) {
    case URES_STRING:
        string_write16(bundle, res, status);
        break;
    case URES_ARRAY:
        array_write16(bundle, res, status);
        break;
    case URES_TABLE:
        table_write16(bundle, res, status);
        break;
    default:
        /* Only a few resource types write 16-bit units. */
        break;
    }
}

/*
 * Only called for UTF-16 v1 strings.
 * For UTF-16 v2 strings, res_preWrite() sees fRes != RES_BOGUS
 * and exits early.
 */
static void
string_preWrite(uint32_t *byteOffset,
                struct SRBRoot *bundle, struct SResource *res,
                UErrorCode *status) {
    /* Write the UTF-16 v1 string. */
    res->fRes = URES_MAKE_RESOURCE(URES_STRING, *byteOffset >> 2);
    *byteOffset += 4 + (res->u.fString.fLength + 1) * U_SIZEOF_UCHAR;
}

static void
bin_preWrite(uint32_t *byteOffset,
             struct SRBRoot *bundle, struct SResource *res,
             UErrorCode *status) {
    uint32_t pad       = 0;
    uint32_t dataStart = *byteOffset + sizeof(res->u.fBinaryValue.fLength);

    if (dataStart % BIN_ALIGNMENT) {
        pad = (BIN_ALIGNMENT - dataStart % BIN_ALIGNMENT);
        *byteOffset += pad;  /* pad == 4 or 8 or 12 */
    }
    res->fRes = URES_MAKE_RESOURCE(URES_BINARY, *byteOffset >> 2);
    *byteOffset += 4 + res->u.fBinaryValue.fLength;
}

static void
array_preWrite(uint32_t *byteOffset,
               struct SRBRoot *bundle, struct SResource *res,
               UErrorCode *status) {
    struct SResource *current;

    if (U_FAILURE(*status)) {
        return;
    }
    for (current = res->u.fArray.fFirst; current != NULL; current = current->fNext) {
        res_preWrite(byteOffset, bundle, current, status);
    }
    res->fRes = URES_MAKE_RESOURCE(URES_ARRAY, *byteOffset >> 2);
    *byteOffset += (1 + res->u.fArray.fCount) * 4;
}

static void
table_preWrite(uint32_t *byteOffset,
               struct SRBRoot *bundle, struct SResource *res,
               UErrorCode *status) {
    struct SResource *current;

    if (U_FAILURE(*status)) {
        return;
    }
    for (current = res->u.fTable.fFirst; current != NULL; current = current->fNext) {
        res_preWrite(byteOffset, bundle, current, status);
    }
    if (res->u.fTable.fType == URES_TABLE) {
        /* 16-bit count, 16-bit key offsets, 32-bit values */
        res->fRes = URES_MAKE_RESOURCE(URES_TABLE, *byteOffset >> 2);
        *byteOffset += 2 + res->u.fTable.fCount * 6;
    } else {
        /* 32-bit count, key offsets and values */
        res->fRes = URES_MAKE_RESOURCE(URES_TABLE32, *byteOffset >> 2);
        *byteOffset += 4 + res->u.fTable.fCount * 8;
    }
}

static void
res_preWrite(uint32_t *byteOffset,
             struct SRBRoot *bundle, struct SResource *res,
             UErrorCode *status) {
    if (U_FAILURE(*status) || res == NULL) {
        return;
    }
    if (res->fRes != RES_BOGUS) {
        /*
         * The resource item word was already precomputed, which means
         * no further data needs to be written.
         * This might be an integer, or an empty or UTF-16 v2 string,
         * an empty binary, etc.
         */
        return;
    }
    switch (res->fType) {
    case URES_STRING:
        string_preWrite(byteOffset, bundle, res, status);
        break;
    case URES_ALIAS:
        res->fRes = URES_MAKE_RESOURCE(URES_ALIAS, *byteOffset >> 2);
        *byteOffset += 4 + (res->u.fString.fLength + 1) * U_SIZEOF_UCHAR;
        break;
    case URES_INT_VECTOR:
        if (res->u.fIntVector.fCount == 0 && gFormatVersion > 1) {
            res->fRes = URES_MAKE_EMPTY_RESOURCE(URES_INT_VECTOR);
            res->fWritten = TRUE;
        } else {
            res->fRes = URES_MAKE_RESOURCE(URES_INT_VECTOR, *byteOffset >> 2);
            *byteOffset += (1 + res->u.fIntVector.fCount) * 4;
        }
        break;
    case URES_BINARY:
        bin_preWrite(byteOffset, bundle, res, status);
        break;
    case URES_INT:
        break;
    case URES_ARRAY:
        array_preWrite(byteOffset, bundle, res, status);
        break;
    case URES_TABLE:
        table_preWrite(byteOffset, bundle, res, status);
        break;
    default:
        *status = U_INTERNAL_PROGRAM_ERROR;
        break;
    }
    *byteOffset += calcPadding(*byteOffset);
}

/*
 * Only called for UTF-16 v1 strings. For UTF-16 v2 strings,
 * res_write() sees fWritten and exits early.
 */
static void string_write(UNewDataMemory *mem, uint32_t *byteOffset,
                         struct SRBRoot *bundle, struct SResource *res,
                         UErrorCode *status) {
    /* Write the UTF-16 v1 string. */
    int32_t length = res->u.fString.fLength;
    udata_write32(mem, length);
    udata_writeUString(mem, res->u.fString.fChars, length + 1);
    *byteOffset += 4 + (length + 1) * U_SIZEOF_UCHAR;
    res->fWritten = TRUE;
}

static void alias_write(UNewDataMemory *mem, uint32_t *byteOffset,
                        struct SRBRoot *bundle, struct SResource *res,
                        UErrorCode *status) {
    int32_t length = res->u.fString.fLength;
    udata_write32(mem, length);
    udata_writeUString(mem, res->u.fString.fChars, length + 1);
    *byteOffset += 4 + (length + 1) * U_SIZEOF_UCHAR;
}

static void array_write(UNewDataMemory *mem, uint32_t *byteOffset,
                        struct SRBRoot *bundle, struct SResource *res,
                        UErrorCode *status) {
    uint32_t  i;

    struct SResource *current = NULL;

    if (U_FAILURE(*status)) {
        return;
    }
    for (i = 0, current = res->u.fArray.fFirst; current != NULL; ++i, current = current->fNext) {
        res_write(mem, byteOffset, bundle, current, status);
    }
    assert(i == res->u.fArray.fCount);

    udata_write32(mem, res->u.fArray.fCount);
    for (current = res->u.fArray.fFirst; current != NULL; current = current->fNext) {
        udata_write32(mem, current->fRes);
    }
    *byteOffset += (1 + res->u.fArray.fCount) * 4;
}

static void intvector_write(UNewDataMemory *mem, uint32_t *byteOffset,
                            struct SRBRoot *bundle, struct SResource *res,
                            UErrorCode *status) {
    uint32_t i = 0;
    udata_write32(mem, res->u.fIntVector.fCount);
    for(i = 0; i<res->u.fIntVector.fCount; i++) {
      udata_write32(mem, res->u.fIntVector.fArray[i]);
    }
    *byteOffset += (1 + res->u.fIntVector.fCount) * 4;
}

static void bin_write(UNewDataMemory *mem, uint32_t *byteOffset,
                      struct SRBRoot *bundle, struct SResource *res,
                      UErrorCode *status) {
    uint32_t pad       = 0;
    uint32_t dataStart = *byteOffset + sizeof(res->u.fBinaryValue.fLength);

    if (dataStart % BIN_ALIGNMENT) {
        pad = (BIN_ALIGNMENT - dataStart % BIN_ALIGNMENT);
        udata_writePadding(mem, pad);  /* pad == 4 or 8 or 12 */
        *byteOffset += pad;
    }

    udata_write32(mem, res->u.fBinaryValue.fLength);
    if (res->u.fBinaryValue.fLength > 0) {
        udata_writeBlock(mem, res->u.fBinaryValue.fData, res->u.fBinaryValue.fLength);
    }
    *byteOffset += 4 + res->u.fBinaryValue.fLength;
}

static void table_write(UNewDataMemory *mem, uint32_t *byteOffset,
                        struct SRBRoot *bundle, struct SResource *res,
                        UErrorCode *status) {
    struct SResource *current;
    uint32_t i;

    if (U_FAILURE(*status)) {
        return;
    }
    for (i = 0, current = res->u.fTable.fFirst; current != NULL; ++i, current = current->fNext) {
        assert(i < res->u.fTable.fCount);
        res_write(mem, byteOffset, bundle, current, status);
    }
    assert(i == res->u.fTable.fCount);

    if(res->u.fTable.fType == URES_TABLE) {
        udata_write16(mem, (uint16_t)res->u.fTable.fCount);
        for (current = res->u.fTable.fFirst; current != NULL; current = current->fNext) {
            udata_write16(mem, makeKey16(bundle, current->fKey));
        }
        *byteOffset += (1 + res->u.fTable.fCount)* 2;
        if ((res->u.fTable.fCount & 1) == 0) {
            /* 16-bit count and even number of 16-bit key offsets need padding before 32-bit resource items */
            udata_writePadding(mem, 2);
            *byteOffset += 2;
        }
    } else /* URES_TABLE32 */ {
        udata_write32(mem, res->u.fTable.fCount);
        for (current = res->u.fTable.fFirst; current != NULL; current = current->fNext) {
            udata_write32(mem, (uint32_t)current->fKey);
        }
        *byteOffset += (1 + res->u.fTable.fCount)* 4;
    }
    for (current = res->u.fTable.fFirst; current != NULL; current = current->fNext) {
        udata_write32(mem, current->fRes);
    }
    *byteOffset += res->u.fTable.fCount * 4;
}

void res_write(UNewDataMemory *mem, uint32_t *byteOffset,
               struct SRBRoot *bundle, struct SResource *res,
               UErrorCode *status) {
    uint8_t paddingSize;

    if (U_FAILURE(*status) || res == NULL) {
        return;
    }
    if (res->fWritten) {
        assert(res->fRes != RES_BOGUS);
        return;
    }
    switch (res->fType) {
    case URES_STRING:
        string_write    (mem, byteOffset, bundle, res, status);
        break;
    case URES_ALIAS:
        alias_write     (mem, byteOffset, bundle, res, status);
        break;
    case URES_INT_VECTOR:
        intvector_write (mem, byteOffset, bundle, res, status);
        break;
    case URES_BINARY:
        bin_write       (mem, byteOffset, bundle, res, status);
        break;
    case URES_INT:
        break;  /* fRes was set by int_open() */
    case URES_ARRAY:
        array_write     (mem, byteOffset, bundle, res, status);
        break;
    case URES_TABLE:
        table_write     (mem, byteOffset, bundle, res, status);
        break;
    default:
        *status = U_INTERNAL_PROGRAM_ERROR;
        break;
    }
    paddingSize = calcPadding(*byteOffset);
    if (paddingSize > 0) {
        udata_writePadding(mem, paddingSize);
        *byteOffset += paddingSize;
    }
    res->fWritten = TRUE;
}

void bundle_write(struct SRBRoot *bundle,
                  const char *outputDir, const char *outputPkg,
                  char *writtenFilename, int writtenFilenameLen,
                  UErrorCode *status) {
    UNewDataMemory *mem        = NULL;
    uint32_t        byteOffset = 0;
    uint32_t        top, size;
    char            dataName[1024];
    int32_t         indexes[URES_INDEX_TOP];

    bundle_compactKeys(bundle, status);
    /*
     * Add padding bytes to fKeys so that fKeysTop is 4-aligned.
     * Safe because the capacity is a multiple of 4.
     */
    while (bundle->fKeysTop & 3) {
        bundle->fKeys[bundle->fKeysTop++] = (char)0xaa;
    }
    /*
     * In URES_TABLE, use all local key offsets that fit into 16 bits,
     * and use the remaining 16-bit offsets for pool key offsets
     * if there are any.
     * If there are no local keys, then use the whole 16-bit space
     * for pool key offsets.
     * Note: This cannot be changed without changing the major formatVersion.
     */
    if (bundle->fKeysBottom < bundle->fKeysTop) {
        if (bundle->fKeysTop <= 0x10000) {
            bundle->fLocalKeyLimit = bundle->fKeysTop;
        } else {
            bundle->fLocalKeyLimit = 0x10000;
        }
    } else {
        bundle->fLocalKeyLimit = 0;
    }

    bundle_compactStrings(bundle, status);
    res_write16(bundle, bundle->fRoot, status);
    if (bundle->f16BitUnitsLength & 1) {
        bundle->f16BitUnits[bundle->f16BitUnitsLength++] = 0xaaaa;  /* pad to multiple of 4 bytes */
    }
    /* all keys have been mapped */
    uprv_free(bundle->fKeyMap);
    bundle->fKeyMap = NULL;

    byteOffset = bundle->fKeysTop + bundle->f16BitUnitsLength * 2;
    res_preWrite(&byteOffset, bundle, bundle->fRoot, status);

    /* total size including the root item */
    top = byteOffset;

    if (U_FAILURE(*status)) {
        return;
    }

    if (writtenFilename && writtenFilenameLen) {
        *writtenFilename = 0;
    }

    if (writtenFilename) {
       int32_t off = 0, len = 0;
       if (outputDir) {
           len = (int32_t)uprv_strlen(outputDir);
           if (len > writtenFilenameLen) {
               len = writtenFilenameLen;
           }
           uprv_strncpy(writtenFilename, outputDir, len);
       }
       if (writtenFilenameLen -= len) {
           off += len;
           writtenFilename[off] = U_FILE_SEP_CHAR;
           if (--writtenFilenameLen) {
               ++off;
               if(outputPkg != NULL)
               {
                   uprv_strcpy(writtenFilename+off, outputPkg);
                   off += (int32_t)uprv_strlen(outputPkg);
                   writtenFilename[off] = '_';
                   ++off;
               }

               len = (int32_t)uprv_strlen(bundle->fLocale);
               if (len > writtenFilenameLen) {
                   len = writtenFilenameLen;
               }
               uprv_strncpy(writtenFilename + off, bundle->fLocale, len);
               if (writtenFilenameLen -= len) {
                   off += len;
                   len = 5;
                   if (len > writtenFilenameLen) {
                       len = writtenFilenameLen;
                   }
                   uprv_strncpy(writtenFilename +  off, ".res", len);
               }
           }
       }
    }

    if(outputPkg)
    {
        uprv_strcpy(dataName, outputPkg);
        uprv_strcat(dataName, "_");
        uprv_strcat(dataName, bundle->fLocale);
    }
    else
    {
        uprv_strcpy(dataName, bundle->fLocale);
    }

    uprv_memcpy(dataInfo.formatVersion, gFormatVersions + gFormatVersion, sizeof(UVersionInfo));

    mem = udata_create(outputDir, "res", dataName, &dataInfo, (gIncludeCopyright==TRUE)? U_COPYRIGHT_STRING:NULL, status);
    if(U_FAILURE(*status)){
        return;
    }

    /* write the root item */
    udata_write32(mem, bundle->fRoot->fRes);

    /*
     * formatVersion 1.1 (ICU 2.8):
     * write int32_t indexes[] after root and before the strings
     * to make it easier to parse resource bundles in icuswap or from Java etc.
     */
    uprv_memset(indexes, 0, sizeof(indexes));
    indexes[URES_INDEX_LENGTH]=             bundle->fIndexLength;
    indexes[URES_INDEX_KEYS_TOP]=           bundle->fKeysTop>>2;
    indexes[URES_INDEX_RESOURCES_TOP]=      (int32_t)(top>>2);
    indexes[URES_INDEX_BUNDLE_TOP]=         indexes[URES_INDEX_RESOURCES_TOP];
    indexes[URES_INDEX_MAX_TABLE_LENGTH]=   bundle->fMaxTableLength;

    /*
     * formatVersion 1.2 (ICU 3.6):
     * write indexes[URES_INDEX_ATTRIBUTES] with URES_ATT_NO_FALLBACK set or not set
     * the memset() above initialized all indexes[] to 0
     */
    if (bundle->noFallback) {
        indexes[URES_INDEX_ATTRIBUTES]=URES_ATT_NO_FALLBACK;
    }
    /*
     * formatVersion 2.0 (ICU 4.4):
     * more compact string value storage, optional pool bundle
     */
    if (URES_INDEX_16BIT_TOP < bundle->fIndexLength) {
        indexes[URES_INDEX_16BIT_TOP] = (bundle->fKeysTop>>2) + (bundle->f16BitUnitsLength>>1);
    }
    if (URES_INDEX_POOL_CHECKSUM < bundle->fIndexLength) {
        if (bundle->fIsPoolBundle) {
            indexes[URES_INDEX_ATTRIBUTES] |= URES_ATT_IS_POOL_BUNDLE | URES_ATT_NO_FALLBACK;
            indexes[URES_INDEX_POOL_CHECKSUM] =
                (int32_t)computeCRC((char *)(bundle->fKeys + bundle->fKeysBottom),
                                    (uint32_t)(bundle->fKeysTop - bundle->fKeysBottom),
                                    0);
        } else if (gUsePoolBundle) {
            indexes[URES_INDEX_ATTRIBUTES] |= URES_ATT_USES_POOL_BUNDLE;
            indexes[URES_INDEX_POOL_CHECKSUM] = bundle->fPoolChecksum;
        }
    }

    /* write the indexes[] */
    udata_writeBlock(mem, indexes, bundle->fIndexLength*4);

    /* write the table key strings */
    udata_writeBlock(mem, bundle->fKeys+bundle->fKeysBottom,
                          bundle->fKeysTop-bundle->fKeysBottom);

    /* write the v2 UTF-16 strings, URES_TABLE16 and URES_ARRAY16 */
    udata_writeBlock(mem, bundle->f16BitUnits, bundle->f16BitUnitsLength*2);

    /* write all of the bundle contents: the root item and its children */
    byteOffset = bundle->fKeysTop + bundle->f16BitUnitsLength * 2;
    res_write(mem, &byteOffset, bundle, bundle->fRoot, status);
    assert(byteOffset == top);

    size = udata_finish(mem, status);
    if(top != size) {
        fprintf(stderr, "genrb error: wrote %u bytes but counted %u\n",
                (int)size, (int)top);
        *status = U_INTERNAL_PROGRAM_ERROR;
    }
}

/* Opening Functions */

/* gcc 4.2 complained "no previous prototype for res_open" without this prototype... */
struct SResource* res_open(struct SRBRoot *bundle, const char *tag,
                           const struct UString* comment, UErrorCode* status);

struct SResource* res_open(struct SRBRoot *bundle, const char *tag,
                           const struct UString* comment, UErrorCode* status){
    struct SResource *res;
    int32_t key = bundle_addtag(bundle, tag, status);
    if (U_FAILURE(*status)) {
        return NULL;
    }

    res = (struct SResource *) uprv_malloc(sizeof(struct SResource));
    if (res == NULL) {
        *status = U_MEMORY_ALLOCATION_ERROR;
        return NULL;
    }
    uprv_memset(res, 0, sizeof(struct SResource));
    res->fKey = key;
    res->fRes = RES_BOGUS;

    ustr_init(&res->fComment);
    if(comment != NULL){
        ustr_cpy(&res->fComment, comment, status);
        if (U_FAILURE(*status)) {
            res_close(res);
            return NULL;
        }
    }
    return res;
}

struct SResource* res_none() {
    return (struct SResource*)&kNoResource;
}

struct SResource* table_open(struct SRBRoot *bundle, const char *tag, const struct UString* comment, UErrorCode *status) {
    struct SResource *res = res_open(bundle, tag, comment, status);
    if (U_FAILURE(*status)) {
        return NULL;
    }
    res->fType = URES_TABLE;
    res->u.fTable.fRoot = bundle;
    return res;
}

struct SResource* array_open(struct SRBRoot *bundle, const char *tag, const struct UString* comment, UErrorCode *status) {
    struct SResource *res = res_open(bundle, tag, comment, status);
    if (U_FAILURE(*status)) {
        return NULL;
    }
    res->fType = URES_ARRAY;
    return res;
}

static int32_t U_CALLCONV
string_hash(const UHashTok key) {
    const struct SResource *res = (struct SResource *)key.pointer;
    return uhash_hashUCharsN(res->u.fString.fChars, res->u.fString.fLength);
}

static UBool U_CALLCONV
string_comp(const UHashTok key1, const UHashTok key2) {
    const struct SResource *res1 = (struct SResource *)key1.pointer;
    const struct SResource *res2 = (struct SResource *)key2.pointer;
    return 0 == u_strCompare(res1->u.fString.fChars, res1->u.fString.fLength,
                             res2->u.fString.fChars, res2->u.fString.fLength,
                             FALSE);
}

struct SResource *string_open(struct SRBRoot *bundle, char *tag, const UChar *value, int32_t len, const struct UString* comment, UErrorCode *status) {
    struct SResource *res = res_open(bundle, tag, comment, status);
    if (U_FAILURE(*status)) {
        return NULL;
    }
    res->fType = URES_STRING;

    if (len == 0 && gFormatVersion > 1) {
        res->u.fString.fChars = &gEmptyString;
        res->fRes = 0;
        res->fWritten = TRUE;
        return res;
    }

    res->u.fString.fLength = len;

    if (gFormatVersion > 1) {
        /* check for duplicates */
        res->u.fString.fChars  = (UChar *)value;
        if (bundle->fStringSet == NULL) {
            UErrorCode localStatus = U_ZERO_ERROR;  /* if failure: just don't detect dups */
            bundle->fStringSet = uhash_open(string_hash, string_comp, string_comp, &localStatus);
        } else {
            res->u.fString.fSame = uhash_get(bundle->fStringSet, res);
        }
    }
    if (res->u.fString.fSame == NULL) {
        /* this is a new string */
        res->u.fString.fChars = (UChar *) uprv_malloc(sizeof(UChar) * (len + 1));

        if (res->u.fString.fChars == NULL) {
            *status = U_MEMORY_ALLOCATION_ERROR;
            uprv_free(res);
            return NULL;
        }

        uprv_memcpy(res->u.fString.fChars, value, sizeof(UChar) * len);
        res->u.fString.fChars[len] = 0;
        if (bundle->fStringSet != NULL) {
            /* put it into the set for finding duplicates */
            uhash_put(bundle->fStringSet, res, res, status);
        }

        if (bundle->fStringsForm != STRINGS_UTF16_V1) {
            if (len <= MAX_IMPLICIT_STRING_LENGTH && !U16_IS_TRAIL(value[0]) && len == u_strlen(value)) {
                /*
                 * This string will be stored without an explicit length.
                 * Runtime will detect !U16_IS_TRAIL(value[0]) and call u_strlen().
                 */
                res->u.fString.fNumCharsForLength = 0;
            } else if (len <= 0x3ee) {
                res->u.fString.fNumCharsForLength = 1;
            } else if (len <= 0xfffff) {
                res->u.fString.fNumCharsForLength = 2;
            } else {
                res->u.fString.fNumCharsForLength = 3;
            }
            bundle->f16BitUnitsLength += res->u.fString.fNumCharsForLength + len + 1;  /* +1 for the NUL */
        }
    } else {
        /* this is a duplicate of fSame */
        struct SResource *same = res->u.fString.fSame;
        res->u.fString.fChars = same->u.fString.fChars;
    }
    return res;
}

/* TODO: make alias_open and string_open use the same code */
struct SResource *alias_open(struct SRBRoot *bundle, char *tag, UChar *value, int32_t len, const struct UString* comment, UErrorCode *status) {
    struct SResource *res = res_open(bundle, tag, comment, status);
    if (U_FAILURE(*status)) {
        return NULL;
    }
    res->fType = URES_ALIAS;
    if (len == 0 && gFormatVersion > 1) {
        res->u.fString.fChars = &gEmptyString;
        res->fRes = URES_MAKE_EMPTY_RESOURCE(URES_ALIAS);
        res->fWritten = TRUE;
        return res;
    }

    res->u.fString.fLength = len;
    res->u.fString.fChars  = (UChar *) uprv_malloc(sizeof(UChar) * (len + 1));
    if (res->u.fString.fChars == NULL) {
        *status = U_MEMORY_ALLOCATION_ERROR;
        uprv_free(res);
        return NULL;
    }
    uprv_memcpy(res->u.fString.fChars, value, sizeof(UChar) * (len + 1));
    return res;
}


struct SResource* intvector_open(struct SRBRoot *bundle, char *tag, const struct UString* comment, UErrorCode *status) {
    struct SResource *res = res_open(bundle, tag, comment, status);
    if (U_FAILURE(*status)) {
        return NULL;
    }
    res->fType = URES_INT_VECTOR;

    res->u.fIntVector.fCount = 0;
    res->u.fIntVector.fArray = (uint32_t *) uprv_malloc(sizeof(uint32_t) * RESLIST_MAX_INT_VECTOR);
    if (res->u.fIntVector.fArray == NULL) {
        *status = U_MEMORY_ALLOCATION_ERROR;
        uprv_free(res);
        return NULL;
    }
    return res;
}

struct SResource *int_open(struct SRBRoot *bundle, char *tag, int32_t value, const struct UString* comment, UErrorCode *status) {
    struct SResource *res = res_open(bundle, tag, comment, status);
    if (U_FAILURE(*status)) {
        return NULL;
    }
    res->fType = URES_INT;
    res->u.fIntValue.fValue = value;
    res->fRes = URES_MAKE_RESOURCE(URES_INT, value & 0x0FFFFFFF);
    res->fWritten = TRUE;
    return res;
}

struct SResource *bin_open(struct SRBRoot *bundle, const char *tag, uint32_t length, uint8_t *data, const char* fileName, const struct UString* comment, UErrorCode *status) {
    struct SResource *res = res_open(bundle, tag, comment, status);
    if (U_FAILURE(*status)) {
        return NULL;
    }
    res->fType = URES_BINARY;

    res->u.fBinaryValue.fLength = length;
    res->u.fBinaryValue.fFileName = NULL;
    if(fileName!=NULL && uprv_strcmp(fileName, "") !=0){
        res->u.fBinaryValue.fFileName = (char*) uprv_malloc(sizeof(char) * (uprv_strlen(fileName)+1));
        uprv_strcpy(res->u.fBinaryValue.fFileName,fileName);
    }
    if (length > 0) {
        res->u.fBinaryValue.fData   = (uint8_t *) uprv_malloc(sizeof(uint8_t) * length);

        if (res->u.fBinaryValue.fData == NULL) {
            *status = U_MEMORY_ALLOCATION_ERROR;
            uprv_free(res);
            return NULL;
        }

        uprv_memcpy(res->u.fBinaryValue.fData, data, length);
    }
    else {
        res->u.fBinaryValue.fData = NULL;
        if (gFormatVersion > 1) {
            res->fRes = URES_MAKE_EMPTY_RESOURCE(URES_BINARY);
            res->fWritten = TRUE;
        }
    }

    return res;
}

struct SRBRoot *bundle_open(const struct UString* comment, UBool isPoolBundle, UErrorCode *status) {
    struct SRBRoot *bundle;

    if (U_FAILURE(*status)) {
        return NULL;
    }

    bundle = (struct SRBRoot *) uprv_malloc(sizeof(struct SRBRoot));
    if (bundle == NULL) {
        *status = U_MEMORY_ALLOCATION_ERROR;
        return 0;
    }
    uprv_memset(bundle, 0, sizeof(struct SRBRoot));

    bundle->fKeys = (char *) uprv_malloc(sizeof(char) * KEY_SPACE_SIZE);
    bundle->fRoot = table_open(bundle, NULL, comment, status);
    if (bundle->fKeys == NULL || bundle->fRoot == NULL || U_FAILURE(*status)) {
        if (U_SUCCESS(*status)) {
            *status = U_MEMORY_ALLOCATION_ERROR;
        }
        bundle_close(bundle, status);
        return NULL;
    }

    bundle->fLocale   = NULL;
    bundle->fKeysCapacity = KEY_SPACE_SIZE;
    /* formatVersion 1.1: start fKeysTop after the root item and indexes[] */
    bundle->fIsPoolBundle = isPoolBundle;
    if (gUsePoolBundle || isPoolBundle) {
        bundle->fIndexLength = URES_INDEX_POOL_CHECKSUM + 1;
    } else if (gFormatVersion >= 2) {
        bundle->fIndexLength = URES_INDEX_16BIT_TOP + 1;
    } else /* formatVersion 1 */ {
        bundle->fIndexLength = URES_INDEX_ATTRIBUTES + 1;
    }
    bundle->fKeysBottom = (1 /* root */ + bundle->fIndexLength) * 4;
    uprv_memset(bundle->fKeys, 0, bundle->fKeysBottom);
    bundle->fKeysTop = bundle->fKeysBottom;

    if (gFormatVersion == 1) {
        bundle->fStringsForm = STRINGS_UTF16_V1;
    } else {
        bundle->fStringsForm = STRINGS_UTF16_V2;
    }

    return bundle;
}

/* Closing Functions */
static void table_close(struct SResource *table) {
    struct SResource *current = NULL;
    struct SResource *prev    = NULL;

    current = table->u.fTable.fFirst;

    while (current != NULL) {
        prev    = current;
        current = current->fNext;

        res_close(prev);
    }

    table->u.fTable.fFirst = NULL;
}

static void array_close(struct SResource *array) {
    struct SResource *current = NULL;
    struct SResource *prev    = NULL;
    
    if(array==NULL){
        return;
    }
    current = array->u.fArray.fFirst;
    
    while (current != NULL) {
        prev    = current;
        current = current->fNext;

        res_close(prev);
    }
    array->u.fArray.fFirst = NULL;
}

static void string_close(struct SResource *string) {
    if (string->u.fString.fChars != NULL &&
        string->u.fString.fChars != &gEmptyString &&
        string->u.fString.fSame == NULL
    ) {
        uprv_free(string->u.fString.fChars);
        string->u.fString.fChars =NULL;
    }
}

static void alias_close(struct SResource *alias) {
    if (alias->u.fString.fChars != NULL) {
        uprv_free(alias->u.fString.fChars);
        alias->u.fString.fChars =NULL;
    }
}

static void intvector_close(struct SResource *intvector) {
    if (intvector->u.fIntVector.fArray != NULL) {
        uprv_free(intvector->u.fIntVector.fArray);
        intvector->u.fIntVector.fArray =NULL;
    }
}

static void int_close(struct SResource *intres) {
    /* Intentionally left blank */
}

static void bin_close(struct SResource *binres) {
    if (binres->u.fBinaryValue.fData != NULL) {
        uprv_free(binres->u.fBinaryValue.fData);
        binres->u.fBinaryValue.fData = NULL;
    }
}

void res_close(struct SResource *res) {
    if (res != NULL) {
        switch(res->fType) {
        case URES_STRING:
            string_close(res);
            break;
        case URES_ALIAS:
            alias_close(res);
            break;
        case URES_INT_VECTOR:
            intvector_close(res);
            break;
        case URES_BINARY:
            bin_close(res);
            break;
        case URES_INT:
            int_close(res);
            break;
        case URES_ARRAY:
            array_close(res);
            break;
        case URES_TABLE:
            table_close(res);
            break;
        default:
            /* Shouldn't happen */
            break;
        }

        ustr_deinit(&res->fComment);
        uprv_free(res);
    }
}

void bundle_close(struct SRBRoot *bundle, UErrorCode *status) {
    res_close(bundle->fRoot);
    uprv_free(bundle->fLocale);
    uprv_free(bundle->fKeys);
    uprv_free(bundle->fKeyMap);
    uhash_close(bundle->fStringSet);
    uprv_free(bundle->f16BitUnits);
    uprv_free(bundle);
}

void bundle_closeString(struct SRBRoot *bundle, struct SResource *string) {
    if (bundle->fStringSet != NULL) {
        uhash_remove(bundle->fStringSet, string);
    }
    string_close(string);
}

/* Adding Functions */
void table_add(struct SResource *table, struct SResource *res, int linenumber, UErrorCode *status) {
    struct SResource *current = NULL;
    struct SResource *prev    = NULL;
    struct SResTable *list;
    const char *resKeyString;

    if (U_FAILURE(*status)) {
        return;
    }
    if (res == &kNoResource) {
        return;
    }

    /* remember this linenumber to report to the user if there is a duplicate key */
    res->line = linenumber;

    /* here we need to traverse the list */
    list = &(table->u.fTable);
    ++(list->fCount);

    /* is list still empty? */
    if (list->fFirst == NULL) {
        list->fFirst = res;
        res->fNext   = NULL;
        return;
    }

    resKeyString = list->fRoot->fKeys + res->fKey;

    current = list->fFirst;

    while (current != NULL) {
        const char *currentKeyString = list->fRoot->fKeys + current->fKey;
        int diff;
        /*
         * formatVersion 1: compare key strings in native-charset order
         * formatVersion 2 and up: compare key strings in ASCII order
         */
        if (gFormatVersion == 1 || U_CHARSET_FAMILY == U_ASCII_FAMILY) {
            diff = uprv_strcmp(currentKeyString, resKeyString);
        } else {
            diff = uprv_compareInvCharsAsAscii(currentKeyString, resKeyString);
        }
        if (diff < 0) {
            prev    = current;
            current = current->fNext;
        } else if (diff > 0) {
            /* we're either in front of list, or in middle */
            if (prev == NULL) {
                /* front of the list */
                list->fFirst = res;
            } else {
                /* middle of the list */
                prev->fNext = res;
            }

            res->fNext = current;
            return;
        } else {
            /* Key already exists! ERROR! */
            error(linenumber, "duplicate key '%s' in table, first appeared at line %d", currentKeyString, current->line);
            *status = U_UNSUPPORTED_ERROR;
            return;
        }
    }

    /* end of list */
    prev->fNext = res;
    res->fNext  = NULL;
}

void array_add(struct SResource *array, struct SResource *res, UErrorCode *status) {
    if (U_FAILURE(*status)) {
        return;
    }

    if (array->u.fArray.fFirst == NULL) {
        array->u.fArray.fFirst = res;
        array->u.fArray.fLast  = res;
    } else {
        array->u.fArray.fLast->fNext = res;
        array->u.fArray.fLast        = res;
    }

    (array->u.fArray.fCount)++;
}

void intvector_add(struct SResource *intvector, int32_t value, UErrorCode *status) {
    if (U_FAILURE(*status)) {
        return;
    }

    *(intvector->u.fIntVector.fArray + intvector->u.fIntVector.fCount) = value;
    intvector->u.fIntVector.fCount++;
}

/* Misc Functions */

void bundle_setlocale(struct SRBRoot *bundle, UChar *locale, UErrorCode *status) {

    if(U_FAILURE(*status)) {
        return;
    }

    if (bundle->fLocale!=NULL) {
        uprv_free(bundle->fLocale);
    }

    bundle->fLocale= (char*) uprv_malloc(sizeof(char) * (u_strlen(locale)+1));

    if(bundle->fLocale == NULL) {
        *status = U_MEMORY_ALLOCATION_ERROR;
        return;
    }

    /*u_strcpy(bundle->fLocale, locale);*/
    u_UCharsToChars(locale, bundle->fLocale, u_strlen(locale)+1);

}

static const char *
getKeyString(const struct SRBRoot *bundle, int32_t key) {
    if (key < 0) {
        return bundle->fPoolBundleKeys + (key & 0x7fffffff);
    } else {
        return bundle->fKeys + key;
    }
}

const char *
res_getKeyString(const struct SRBRoot *bundle, const struct SResource *res, char temp[8]) {
    if (res->fKey == -1) {
        return NULL;
    }
    return getKeyString(bundle, res->fKey);
}

const char *
bundle_getKeyBytes(struct SRBRoot *bundle, int32_t *pLength) {
    *pLength = bundle->fKeysTop - bundle->fKeysBottom;
    return bundle->fKeys + bundle->fKeysBottom;
}

int32_t
bundle_addKeyBytes(struct SRBRoot *bundle, const char *keyBytes, int32_t length, UErrorCode *status) {
    int32_t keypos;

    if (U_FAILURE(*status)) {
        return -1;
    }
    if (length < 0 || (keyBytes == NULL && length != 0)) {
        *status = U_ILLEGAL_ARGUMENT_ERROR;
        return -1;
    }
    if (length == 0) {
        return bundle->fKeysTop;
    }

    keypos = bundle->fKeysTop;
    bundle->fKeysTop += length;
    if (bundle->fKeysTop >= bundle->fKeysCapacity) {
        /* overflow - resize the keys buffer */
        bundle->fKeysCapacity += KEY_SPACE_SIZE;
        bundle->fKeys = uprv_realloc(bundle->fKeys, bundle->fKeysCapacity);
        if(bundle->fKeys == NULL) {
            *status = U_MEMORY_ALLOCATION_ERROR;
            return -1;
        }
    }

    uprv_memcpy(bundle->fKeys + keypos, keyBytes, length);

    return keypos;
}

int32_t
bundle_addtag(struct SRBRoot *bundle, const char *tag, UErrorCode *status) {
    int32_t keypos;

    if (U_FAILURE(*status)) {
        return -1;
    }

    if (tag == NULL) {
        /* no error: the root table and array items have no keys */
        return -1;
    }

    keypos = bundle_addKeyBytes(bundle, tag, (int32_t)(uprv_strlen(tag) + 1), status);
    if (U_SUCCESS(*status)) {
        ++bundle->fKeysCount;
    }
    return keypos;
}

static int32_t
compareInt32(int32_t lPos, int32_t rPos) {
    /*
     * Compare possibly-negative key offsets. Don't just return lPos - rPos
     * because that is prone to negative-integer underflows.
     */
    if (lPos < rPos) {
        return -1;
    } else if (lPos > rPos) {
        return 1;
    } else {
        return 0;
    }
}

static int32_t U_CALLCONV
compareKeySuffixes(const void *context, const void *l, const void *r) {
    const struct SRBRoot *bundle=(const struct SRBRoot *)context;
    int32_t lPos = ((const KeyMapEntry *)l)->oldpos;
    int32_t rPos = ((const KeyMapEntry *)r)->oldpos;
    const char *lStart = getKeyString(bundle, lPos);
    const char *lLimit = lStart;
    const char *rStart = getKeyString(bundle, rPos);
    const char *rLimit = rStart;
    int32_t diff;
    while (*lLimit != 0) { ++lLimit; }
    while (*rLimit != 0) { ++rLimit; }
    /* compare keys in reverse character order */
    while (lStart < lLimit && rStart < rLimit) {
        diff = (int32_t)(uint8_t)*--lLimit - (int32_t)(uint8_t)*--rLimit;
        if (diff != 0) {
            return diff;
        }
    }
    /* sort equal suffixes by descending key length */
    diff = (int32_t)(rLimit - rStart) - (int32_t)(lLimit - lStart);
    if (diff != 0) {
        return diff;
    }
    /* Sort pool bundle keys first (negative oldpos), and otherwise keys in parsing order. */
    return compareInt32(lPos, rPos);
}

static int32_t U_CALLCONV
compareKeyNewpos(const void *context, const void *l, const void *r) {
    return compareInt32(((const KeyMapEntry *)l)->newpos, ((const KeyMapEntry *)r)->newpos);
}

static int32_t U_CALLCONV
compareKeyOldpos(const void *context, const void *l, const void *r) {
    return compareInt32(((const KeyMapEntry *)l)->oldpos, ((const KeyMapEntry *)r)->oldpos);
}

void
bundle_compactKeys(struct SRBRoot *bundle, UErrorCode *status) {
    KeyMapEntry *map;
    char *keys;
    int32_t i;
    int32_t keysCount = bundle->fPoolBundleKeysCount + bundle->fKeysCount;
    if (U_FAILURE(*status) || bundle->fKeysCount == 0 || bundle->fKeyMap != NULL) {
        return;
    }
    map = (KeyMapEntry *)uprv_malloc(keysCount * sizeof(KeyMapEntry));
    if (map == NULL) {
        *status = U_MEMORY_ALLOCATION_ERROR;
        return;
    }
    keys = (char *)bundle->fPoolBundleKeys;
    for (i = 0; i < bundle->fPoolBundleKeysCount; ++i) {
        map[i].oldpos =
            (int32_t)(keys - bundle->fPoolBundleKeys) | 0x80000000;  /* negative oldpos */
        map[i].newpos = 0;
        while (*keys != 0) { ++keys; }  /* skip the key */
        ++keys;  /* skip the NUL */
    }
    keys = bundle->fKeys + bundle->fKeysBottom;
    for (; i < keysCount; ++i) {
        map[i].oldpos = (int32_t)(keys - bundle->fKeys);
        map[i].newpos = 0;
        while (*keys != 0) { ++keys; }  /* skip the key */
        ++keys;  /* skip the NUL */
    }
    /* Sort the keys so that each one is immediately followed by all of its suffixes. */
    uprv_sortArray(map, keysCount, (int32_t)sizeof(KeyMapEntry),
                   compareKeySuffixes, bundle, FALSE, status);
    /*
     * Make suffixes point into earlier, longer strings that contain them
     * and mark the old, now unused suffix bytes as deleted.
     */
    if (U_SUCCESS(*status)) {
        keys = bundle->fKeys;
        for (i = 0; i < keysCount;) {
            /*
             * This key is not a suffix of the previous one;
             * keep this one and delete the following ones that are
             * suffixes of this one.
             */
            const char *key;
            const char *keyLimit;
            int32_t j = i + 1;
            map[i].newpos = map[i].oldpos;
            if (j < keysCount && map[j].oldpos < 0) {
                /* Key string from the pool bundle, do not delete. */
                i = j;
                continue;
            }
            key = getKeyString(bundle, map[i].oldpos);
            for (keyLimit = key; *keyLimit != 0; ++keyLimit) {}
            for (; j < keysCount && map[j].oldpos >= 0; ++j) {
                const char *k;
                char *suffix;
                const char *suffixLimit;
                int32_t offset;
                suffix = keys + map[j].oldpos;
                for (suffixLimit = suffix; *suffixLimit != 0; ++suffixLimit) {}
                offset = (int32_t)(keyLimit - key) - (suffixLimit - suffix);
                if (offset < 0) {
                    break;  /* suffix cannot be longer than the original */
                }
                /* Is it a suffix of the earlier, longer key? */
                for (k = keyLimit; suffix < suffixLimit && *--k == *--suffixLimit;) {}
                if (suffix == suffixLimit && *k == *suffixLimit) {
                    map[j].newpos = map[i].oldpos + offset;  /* yes, point to the earlier key */
                    /* mark the suffix as deleted */
                    while (*suffix != 0) { *suffix++ = 1; }
                    *suffix = 1;
                } else {
                    break;  /* not a suffix, restart from here */
                }
            }
            i = j;
        }
        /*
         * Re-sort by newpos, then modify the key characters array in-place
         * to squeeze out unused bytes, and readjust the newpos offsets.
         */
        uprv_sortArray(map, keysCount, (int32_t)sizeof(KeyMapEntry),
                       compareKeyNewpos, NULL, FALSE, status);
        if (U_SUCCESS(*status)) {
            int32_t oldpos, newpos, limit;
            oldpos = newpos = bundle->fKeysBottom;
            limit = bundle->fKeysTop;
            /* skip key offsets that point into the pool bundle rather than this new bundle */
            for (i = 0; i < keysCount && map[i].newpos < 0; ++i) {}
            if (i < keysCount) {
                while (oldpos < limit) {
                    if (keys[oldpos] == 1) {
                        ++oldpos;  /* skip unused bytes */
                    } else {
                        /* adjust the new offsets for keys starting here */
                        while (i < keysCount && map[i].newpos == oldpos) {
                            map[i++].newpos = newpos;
                        }
                        /* move the key characters to their new position */
                        keys[newpos++] = keys[oldpos++];
                    }
                }
                assert(i == keysCount);
            }
            bundle->fKeysTop = newpos;
            /* Re-sort once more, by old offsets for binary searching. */
            uprv_sortArray(map, keysCount, (int32_t)sizeof(KeyMapEntry),
                           compareKeyOldpos, NULL, FALSE, status);
            if (U_SUCCESS(*status)) {
                /* key size reduction by limit - newpos */
                bundle->fKeyMap = map;
                map = NULL;
            }
        }
    }
    uprv_free(map);
}

static int32_t U_CALLCONV
compareStringSuffixes(const void *context, const void *l, const void *r) {
    struct SResource *left = *((struct SResource **)l);
    struct SResource *right = *((struct SResource **)r);
    const UChar *lStart = left->u.fString.fChars;
    const UChar *lLimit = lStart + left->u.fString.fLength;
    const UChar *rStart = right->u.fString.fChars;
    const UChar *rLimit = rStart + right->u.fString.fLength;
    int32_t diff;
    /* compare keys in reverse character order */
    while (lStart < lLimit && rStart < rLimit) {
        diff = (int32_t)*--lLimit - (int32_t)*--rLimit;
        if (diff != 0) {
            return diff;
        }
    }
    /* sort equal suffixes by descending string length */
    return right->u.fString.fLength - left->u.fString.fLength;
}

static int32_t U_CALLCONV
compareStringLengths(const void *context, const void *l, const void *r) {
    struct SResource *left = *((struct SResource **)l);
    struct SResource *right = *((struct SResource **)r);
    int32_t diff;
    /* Make "is suffix of another string" compare greater than a non-suffix. */
    diff = (int)(left->u.fString.fSame != NULL) - (int)(right->u.fString.fSame != NULL);
    if (diff != 0) {
        return diff;
    }
    /* sort by ascending string length */
    return left->u.fString.fLength - right->u.fString.fLength;
}

static int32_t
string_writeUTF16v2(struct SRBRoot *bundle, struct SResource *res, int32_t utf16Length) {
    int32_t length = res->u.fString.fLength;
    res->fRes = URES_MAKE_RESOURCE(URES_STRING_V2, utf16Length);
    res->fWritten = TRUE;
    switch(res->u.fString.fNumCharsForLength) {
    case 0:
        break;
    case 1:
        bundle->f16BitUnits[utf16Length++] = (uint16_t)(0xdc00 + length);
        break;
    case 2:
        bundle->f16BitUnits[utf16Length] = (uint16_t)(0xdfef + (length >> 16));
        bundle->f16BitUnits[utf16Length + 1] = (uint16_t)length;
        utf16Length += 2;
        break;
    case 3:
        bundle->f16BitUnits[utf16Length] = 0xdfff;
        bundle->f16BitUnits[utf16Length + 1] = (uint16_t)(length >> 16);
        bundle->f16BitUnits[utf16Length + 2] = (uint16_t)length;
        utf16Length += 3;
        break;
    default:
        break;  /* will not occur */
    }
    u_memcpy(bundle->f16BitUnits + utf16Length, res->u.fString.fChars, length + 1);
    return utf16Length + length + 1;
}

static void
bundle_compactStrings(struct SRBRoot *bundle, UErrorCode *status) {
    if (U_FAILURE(*status)) {
        return;
    }
    switch(bundle->fStringsForm) {
    case STRINGS_UTF16_V2:
        if (bundle->f16BitUnitsLength > 0) {
            struct SResource **array;
            int32_t count = uhash_count(bundle->fStringSet);
            int32_t i, pos;
            /*
             * Allocate enough space for the initial NUL and the UTF-16 v2 strings,
             * and some extra for URES_TABLE16 and URES_ARRAY16 values.
             * Round down to an even number.
             */
            int32_t utf16Length = (bundle->f16BitUnitsLength + 20000) & ~1;
            bundle->f16BitUnits = (UChar *)uprv_malloc(utf16Length * U_SIZEOF_UCHAR);
            array = (struct SResource **)uprv_malloc(count * sizeof(struct SResource **));
            if (bundle->f16BitUnits == NULL || array == NULL) {
                uprv_free(bundle->f16BitUnits);
                bundle->f16BitUnits = NULL;
                uprv_free(array);
                *status = U_MEMORY_ALLOCATION_ERROR;
                return;
            }
            bundle->f16BitUnitsCapacity = utf16Length;
            /* insert the initial NUL */
            bundle->f16BitUnits[0] = 0;
            utf16Length = 1;
            ++bundle->f16BitUnitsLength;
            for (pos = -1, i = 0; i < count; ++i) {
                array[i] = (struct SResource *)uhash_nextElement(bundle->fStringSet, &pos)->key.pointer;
            }
            /* Sort the strings so that each one is immediately followed by all of its suffixes. */
            uprv_sortArray(array, count, (int32_t)sizeof(struct SResource **),
                           compareStringSuffixes, NULL, FALSE, status);
            /*
             * Make suffixes point into earlier, longer strings that contain them.
             * Temporarily use fSame and fSuffixOffset for suffix strings to
             * refer to the remaining ones.
             */
            if (U_SUCCESS(*status)) {
                for (i = 0; i < count;) {
                    /*
                     * This string is not a suffix of the previous one;
                     * write this one and subsume the following ones that are
                     * suffixes of this one.
                     */
                    struct SResource *res = array[i];
                    const UChar *strLimit = res->u.fString.fChars + res->u.fString.fLength;
                    int32_t j;
                    for (j = i + 1; j < count; ++j) {
                        struct SResource *suffixRes = array[j];
                        const UChar *s;
                        const UChar *suffix = suffixRes->u.fString.fChars;
                        const UChar *suffixLimit = suffix + suffixRes->u.fString.fLength;
                        int32_t offset = res->u.fString.fLength - suffixRes->u.fString.fLength;
                        if (offset < 0) {
                            break;  /* suffix cannot be longer than the original */
                        }
                        /* Is it a suffix of the earlier, longer key? */
                        for (s = strLimit; suffix < suffixLimit && *--s == *--suffixLimit;) {}
                        if (suffix == suffixLimit && *s == *suffixLimit) {
                            if (suffixRes->u.fString.fNumCharsForLength == 0) {
                                /* yes, point to the earlier string */
                                suffixRes->u.fString.fSame = res;
                                suffixRes->u.fString.fSuffixOffset = offset;
                            } else {
                                /* write the suffix by itself if we need explicit length */
                            }
                        } else {
                            break;  /* not a suffix, restart from here */
                        }
                    }
                    i = j;
                }
            }
            /*
             * Re-sort the strings by ascending length (except suffixes last)
             * to optimize for URES_TABLE16 and URES_ARRAY16:
             * Keep as many as possible within reach of 16-bit offsets.
             */
            uprv_sortArray(array, count, (int32_t)sizeof(struct SResource **),
                           compareStringLengths, NULL, FALSE, status);
            if (U_SUCCESS(*status)) {
                /* Write the non-suffix strings. */
                for (i = 0; i < count && array[i]->u.fString.fSame == NULL; ++i) {
                    utf16Length = string_writeUTF16v2(bundle, array[i], utf16Length);
                }
                /* Write the suffix strings. Make each point to the real string. */
                for (; i < count; ++i) {
                    struct SResource *res = array[i];
                    struct SResource *same = res->u.fString.fSame;
                    res->fRes = same->fRes + same->u.fString.fNumCharsForLength + res->u.fString.fSuffixOffset;
                    res->u.fString.fSame = NULL;
                    res->fWritten = TRUE;
                }
            }
            assert(utf16Length <= bundle->f16BitUnitsLength);
            bundle->f16BitUnitsLength = utf16Length;
            uprv_free(array);
        }
        break;
    default:
        break;
    }
}
