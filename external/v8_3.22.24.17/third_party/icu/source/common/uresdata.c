/*
*******************************************************************************
*                                                                             *
* Copyright (C) 1999-2010, International Business Machines Corporation        *
*               and others. All Rights Reserved.                              *
*                                                                             *
*******************************************************************************
*   file name:  uresdata.c
*   encoding:   US-ASCII
*   tab size:   8 (not used)
*   indentation:4
*
*   created on: 1999dec08
*   created by: Markus W. Scherer
* Modification History:
*
*   Date        Name        Description
*   06/20/2000  helena      OS/400 port changes; mostly typecast.
*   06/24/02    weiv        Added support for resource sharing
*/

#include "unicode/utypes.h"
#include "unicode/udata.h"
#include "unicode/ustring.h"
#include "cmemory.h"
#include "cstring.h"
#include "uarrsort.h"
#include "udataswp.h"
#include "ucol_swp.h"
#include "uinvchar.h"
#include "uresdata.h"
#include "uresimp.h"

#define LENGTHOF(array) (int32_t)(sizeof(array)/sizeof((array)[0]))

/*
 * Resource access helpers
 */

/* get a const char* pointer to the key with the keyOffset byte offset from pRoot */
#define RES_GET_KEY16(pResData, keyOffset) \
    ((keyOffset)<(pResData)->localKeyLimit ? \
        (const char *)(pResData)->pRoot+(keyOffset) : \
        (pResData)->poolBundleKeys+(keyOffset)-(pResData)->localKeyLimit)

#define RES_GET_KEY32(pResData, keyOffset) \
    ((keyOffset)>=0 ? \
        (const char *)(pResData)->pRoot+(keyOffset) : \
        (pResData)->poolBundleKeys+((keyOffset)&0x7fffffff))

#define URESDATA_ITEM_NOT_FOUND -1

/* empty resources, returned when the resource offset is 0 */
static const uint16_t gEmpty16=0;
static const int32_t gEmpty32=0;
static const struct {
    int32_t length;
    UChar nul;
    UChar pad;
} gEmptyString={ 0, 0, 0 };

/*
 * All the type-access functions assume that
 * the resource is of the expected type.
 */

static int32_t
_res_findTableItem(const ResourceData *pResData, const uint16_t *keyOffsets, int32_t length,
                   const char *key, const char **realKey) {
    const char *tableKey;
    int32_t mid, start, limit;
    int result;

    /* do a binary search for the key */
    start=0;
    limit=length;
    while(start<limit) {
        mid = (start + limit) / 2;
        tableKey = RES_GET_KEY16(pResData, keyOffsets[mid]);
        if (pResData->useNativeStrcmp) {
            result = uprv_strcmp(key, tableKey);
        } else {
            result = uprv_compareInvCharsAsAscii(key, tableKey);
        }
        if (result < 0) {
            limit = mid;
        } else if (result > 0) {
            start = mid + 1;
        } else {
            /* We found it! */
            *realKey=tableKey;
            return mid;
        }
    }
    return URESDATA_ITEM_NOT_FOUND;  /* not found or table is empty. */
}

static int32_t
_res_findTable32Item(const ResourceData *pResData, const int32_t *keyOffsets, int32_t length,
                     const char *key, const char **realKey) {
    const char *tableKey;
    int32_t mid, start, limit;
    int result;

    /* do a binary search for the key */
    start=0;
    limit=length;
    while(start<limit) {
        mid = (start + limit) / 2;
        tableKey = RES_GET_KEY32(pResData, keyOffsets[mid]);
        if (pResData->useNativeStrcmp) {
            result = uprv_strcmp(key, tableKey);
        } else {
            result = uprv_compareInvCharsAsAscii(key, tableKey);
        }
        if (result < 0) {
            limit = mid;
        } else if (result > 0) {
            start = mid + 1;
        } else {
            /* We found it! */
            *realKey=tableKey;
            return mid;
        }
    }
    return URESDATA_ITEM_NOT_FOUND;  /* not found or table is empty. */
}

/* helper for res_load() ---------------------------------------------------- */

static UBool U_CALLCONV
isAcceptable(void *context,
             const char *type, const char *name,
             const UDataInfo *pInfo) {
    uprv_memcpy(context, pInfo->formatVersion, 4);
    return (UBool)(
        pInfo->size>=20 &&
        pInfo->isBigEndian==U_IS_BIG_ENDIAN &&
        pInfo->charsetFamily==U_CHARSET_FAMILY &&
        pInfo->sizeofUChar==U_SIZEOF_UCHAR &&
        pInfo->dataFormat[0]==0x52 &&   /* dataFormat="ResB" */
        pInfo->dataFormat[1]==0x65 &&
        pInfo->dataFormat[2]==0x73 &&
        pInfo->dataFormat[3]==0x42 &&
        (pInfo->formatVersion[0]==1 || pInfo->formatVersion[0]==2));
}

/* semi-public functions ---------------------------------------------------- */

static void
res_init(ResourceData *pResData,
         UVersionInfo formatVersion, const void *inBytes, int32_t length,
         UErrorCode *errorCode) {
    UResType rootType;

    /* get the root resource */
    pResData->pRoot=(const int32_t *)inBytes;
    pResData->rootRes=(Resource)*pResData->pRoot;
    pResData->p16BitUnits=&gEmpty16;

    /* formatVersion 1.1 must have a root item and at least 5 indexes */
    if(length>=0 && (length/4)<((formatVersion[0]==1 && formatVersion[1]==0) ? 1 : 1+5)) {
        *errorCode=U_INVALID_FORMAT_ERROR;
        res_unload(pResData);
        return;
    }

    /* currently, we accept only resources that have a Table as their roots */
    rootType=RES_GET_TYPE(pResData->rootRes);
    if(!URES_IS_TABLE(rootType)) {
        *errorCode=U_INVALID_FORMAT_ERROR;
        res_unload(pResData);
        return;
    }

    if(formatVersion[0]==1 && formatVersion[1]==0) {
        pResData->localKeyLimit=0x10000;  /* greater than any 16-bit key string offset */
    } else {
        /* bundles with formatVersion 1.1 and later contain an indexes[] array */
        const int32_t *indexes=pResData->pRoot+1;
        int32_t indexLength=indexes[URES_INDEX_LENGTH]&0xff;
        if(indexLength<=URES_INDEX_MAX_TABLE_LENGTH) {
            *errorCode=U_INVALID_FORMAT_ERROR;
            res_unload(pResData);
            return;
        }
        if( length>=0 &&
            (length<((1+indexLength)<<2) ||
             length<(indexes[URES_INDEX_BUNDLE_TOP]<<2))
        ) {
            *errorCode=U_INVALID_FORMAT_ERROR;
            res_unload(pResData);
            return;
        }
        if(indexes[URES_INDEX_KEYS_TOP]>(1+indexLength)) {
            pResData->localKeyLimit=indexes[URES_INDEX_KEYS_TOP]<<2;
        }
        if(indexLength>URES_INDEX_ATTRIBUTES) {
            int32_t att=indexes[URES_INDEX_ATTRIBUTES];
            pResData->noFallback=(UBool)(att&URES_ATT_NO_FALLBACK);
            pResData->isPoolBundle=(UBool)((att&URES_ATT_IS_POOL_BUNDLE)!=0);
            pResData->usesPoolBundle=(UBool)((att&URES_ATT_USES_POOL_BUNDLE)!=0);
        }
        if((pResData->isPoolBundle || pResData->usesPoolBundle) && indexLength<=URES_INDEX_POOL_CHECKSUM) {
            *errorCode=U_INVALID_FORMAT_ERROR;
            res_unload(pResData);
            return;
        }
        if( indexLength>URES_INDEX_16BIT_TOP &&
            indexes[URES_INDEX_16BIT_TOP]>indexes[URES_INDEX_KEYS_TOP]
        ) {
            pResData->p16BitUnits=(const uint16_t *)(pResData->pRoot+indexes[URES_INDEX_KEYS_TOP]);
        }
    }

    if(formatVersion[0]==1 || U_CHARSET_FAMILY==U_ASCII_FAMILY) {
        /*
         * formatVersion 1: compare key strings in native-charset order
         * formatVersion 2 and up: compare key strings in ASCII order
         */
        pResData->useNativeStrcmp=TRUE;
    }
}

U_CAPI void U_EXPORT2
res_read(ResourceData *pResData,
         const UDataInfo *pInfo, const void *inBytes, int32_t length,
         UErrorCode *errorCode) {
    UVersionInfo formatVersion;

    uprv_memset(pResData, 0, sizeof(ResourceData));
    if(U_FAILURE(*errorCode)) {
        return;
    }
    if(!isAcceptable(formatVersion, NULL, NULL, pInfo)) {
        *errorCode=U_INVALID_FORMAT_ERROR;
        return;
    }
    res_init(pResData, formatVersion, inBytes, length, errorCode);
}

U_CFUNC void
res_load(ResourceData *pResData,
         const char *path, const char *name, UErrorCode *errorCode) {
    UVersionInfo formatVersion;

    uprv_memset(pResData, 0, sizeof(ResourceData));

    /* load the ResourceBundle file */
    pResData->data=udata_openChoice(path, "res", name, isAcceptable, formatVersion, errorCode);
    if(U_FAILURE(*errorCode)) {
        return;
    }

    /* get its memory and initialize *pResData */
    res_init(pResData, formatVersion, udata_getMemory(pResData->data), -1, errorCode);
}

U_CFUNC void
res_unload(ResourceData *pResData) {
    if(pResData->data!=NULL) {
        udata_close(pResData->data);
        pResData->data=NULL;
    }
}

static const int8_t gPublicTypes[URES_LIMIT] = {
    URES_STRING,
    URES_BINARY,
    URES_TABLE,
    URES_ALIAS,

    URES_TABLE,     /* URES_TABLE32 */
    URES_TABLE,     /* URES_TABLE16 */
    URES_STRING,    /* URES_STRING_V2 */
    URES_INT,

    URES_ARRAY,
    URES_ARRAY,     /* URES_ARRAY16 */
    URES_NONE,
    URES_NONE,

    URES_NONE,
    URES_NONE,
    URES_INT_VECTOR,
    URES_NONE
};

U_CAPI UResType U_EXPORT2
res_getPublicType(Resource res) {
    return (UResType)gPublicTypes[RES_GET_TYPE(res)];
}

U_CAPI const UChar * U_EXPORT2
res_getString(const ResourceData *pResData, Resource res, int32_t *pLength) {
    const UChar *p;
    uint32_t offset=RES_GET_OFFSET(res);
    int32_t length;
    if(RES_GET_TYPE(res)==URES_STRING_V2) {
        int32_t first;
        p=(const UChar *)(pResData->p16BitUnits+offset);
        first=*p;
        if(!U16_IS_TRAIL(first)) {
            length=u_strlen(p);
        } else if(first<0xdfef) {
            length=first&0x3ff;
            ++p;
        } else if(first<0xdfff) {
            length=((first-0xdfef)<<16)|p[1];
            p+=2;
        } else {
            length=((int32_t)p[1]<<16)|p[2];
            p+=3;
        }
    } else if(res==offset) /* RES_GET_TYPE(res)==URES_STRING */ {
        const int32_t *p32= res==0 ? &gEmptyString.length : pResData->pRoot+res;
        length=*p32++;
        p=(const UChar *)p32;
    } else {
        p=NULL;
        length=0;
    }
    if(pLength) {
        *pLength=length;
    }
    return p;
}

U_CAPI const UChar * U_EXPORT2
res_getAlias(const ResourceData *pResData, Resource res, int32_t *pLength) {
    const UChar *p;
    uint32_t offset=RES_GET_OFFSET(res);
    int32_t length;
    if(RES_GET_TYPE(res)==URES_ALIAS) {
        const int32_t *p32= offset==0 ? &gEmptyString.length : pResData->pRoot+offset;
        length=*p32++;
        p=(const UChar *)p32;
    } else {
        p=NULL;
        length=0;
    }
    if(pLength) {
        *pLength=length;
    }
    return p;
}

U_CAPI const uint8_t * U_EXPORT2
res_getBinary(const ResourceData *pResData, Resource res, int32_t *pLength) {
    const uint8_t *p;
    uint32_t offset=RES_GET_OFFSET(res);
    int32_t length;
    if(RES_GET_TYPE(res)==URES_BINARY) {
        const int32_t *p32= offset==0 ? &gEmpty32 : pResData->pRoot+offset;
        length=*p32++;
        p=(const uint8_t *)p32;
    } else {
        p=NULL;
        length=0;
    }
    if(pLength) {
        *pLength=length;
    }
    return p;
}


U_CAPI const int32_t * U_EXPORT2
res_getIntVector(const ResourceData *pResData, Resource res, int32_t *pLength) {
    const int32_t *p;
    uint32_t offset=RES_GET_OFFSET(res);
    int32_t length;
    if(RES_GET_TYPE(res)==URES_INT_VECTOR) {
        p= offset==0 ? &gEmpty32 : pResData->pRoot+offset;
        length=*p++;
    } else {
        p=NULL;
        length=0;
    }
    if(pLength) {
        *pLength=length;
    }
    return p;
}

U_CAPI int32_t U_EXPORT2
res_countArrayItems(const ResourceData *pResData, Resource res) {
    uint32_t offset=RES_GET_OFFSET(res);
    switch(RES_GET_TYPE(res)) {
    case URES_STRING:
    case URES_STRING_V2:
    case URES_BINARY:
    case URES_ALIAS:
    case URES_INT:
    case URES_INT_VECTOR:
        return 1;
    case URES_ARRAY:
    case URES_TABLE32:
        return offset==0 ? 0 : *(pResData->pRoot+offset);
    case URES_TABLE:
        return offset==0 ? 0 : *((const uint16_t *)(pResData->pRoot+offset));
    case URES_ARRAY16:
    case URES_TABLE16:
        return pResData->p16BitUnits[offset];
    default:
        return 0;
    }
}

U_CAPI Resource U_EXPORT2
res_getTableItemByKey(const ResourceData *pResData, Resource table,
                      int32_t *indexR, const char **key) {
    uint32_t offset=RES_GET_OFFSET(table);
    int32_t length;
    int32_t idx;
    if(key == NULL || *key == NULL) {
        return RES_BOGUS;
    }
    switch(RES_GET_TYPE(table)) {
    case URES_TABLE: {
        const uint16_t *p= offset==0 ? &gEmpty16 : (const uint16_t *)(pResData->pRoot+offset);
        length=*p++;
        *indexR=idx=_res_findTableItem(pResData, p, length, *key, key);
        if(idx>=0) {
            const Resource *p32=(const Resource *)(p+length+(~length&1));
            return p32[idx];
        }
        break;
    }
    case URES_TABLE16: {
        const uint16_t *p=pResData->p16BitUnits+offset;
        length=*p++;
        *indexR=idx=_res_findTableItem(pResData, p, length, *key, key);
        if(idx>=0) {
            return URES_MAKE_RESOURCE(URES_STRING_V2, p[length+idx]);
        }
        break;
    }
    case URES_TABLE32: {
        const int32_t *p= offset==0 ? &gEmpty32 : pResData->pRoot+offset;
        length=*p++;
        *indexR=idx=_res_findTable32Item(pResData, p, length, *key, key);
        if(idx>=0) {
            return (Resource)p[length+idx];
        }
        break;
    }
    default:
        break;
    }
    return RES_BOGUS;
}

U_CAPI Resource U_EXPORT2
res_getTableItemByIndex(const ResourceData *pResData, Resource table,
                        int32_t indexR, const char **key) {
    uint32_t offset=RES_GET_OFFSET(table);
    int32_t length;
    switch(RES_GET_TYPE(table)) {
    case URES_TABLE: {
        const uint16_t *p= offset==0 ? &gEmpty16 : (const uint16_t *)(pResData->pRoot+offset);
        length=*p++;
        if(indexR<length) {
            const Resource *p32=(const Resource *)(p+length+(~length&1));
            if(key!=NULL) {
                *key=RES_GET_KEY16(pResData, p[indexR]);
            }
            return p32[indexR];
        }
        break;
    }
    case URES_TABLE16: {
        const uint16_t *p=pResData->p16BitUnits+offset;
        length=*p++;
        if(indexR<length) {
            if(key!=NULL) {
                *key=RES_GET_KEY16(pResData, p[indexR]);
            }
            return URES_MAKE_RESOURCE(URES_STRING_V2, p[length+indexR]);
        }
        break;
    }
    case URES_TABLE32: {
        const int32_t *p= offset==0 ? &gEmpty32 : pResData->pRoot+offset;
        length=*p++;
        if(indexR<length) {
            if(key!=NULL) {
                *key=RES_GET_KEY32(pResData, p[indexR]);
            }
            return (Resource)p[length+indexR];
        }
        break;
    }
    default:
        break;
    }
    return RES_BOGUS;
}

U_CAPI Resource U_EXPORT2
res_getResource(const ResourceData *pResData, const char *key) {
    const char *realKey=key;
    int32_t idx;
    return res_getTableItemByKey(pResData, pResData->rootRes, &idx, &realKey);
}

U_CAPI Resource U_EXPORT2
res_getArrayItem(const ResourceData *pResData, Resource array, int32_t indexR) {
    uint32_t offset=RES_GET_OFFSET(array);
    switch(RES_GET_TYPE(array)) {
    case URES_ARRAY: {
        const int32_t *p= offset==0 ? &gEmpty32 : pResData->pRoot+offset;
        if(indexR<*p) {
            return (Resource)p[1+indexR];
        }
        break;
    }
    case URES_ARRAY16: {
        const uint16_t *p=pResData->p16BitUnits+offset;
        if(indexR<*p) {
            return URES_MAKE_RESOURCE(URES_STRING_V2, p[1+indexR]);
        }
        break;
    }
    default:
        break;
    }
    return RES_BOGUS;
}

U_CFUNC Resource
res_findResource(const ResourceData *pResData, Resource r, char** path, const char** key) {
  /* we pass in a path. CollationElements/Sequence or zoneStrings/3/2 etc. 
   * iterates over a path and stops when a scalar resource is found. This  
   * CAN be an alias. Path gets set to the part that has not yet been processed. 
   */

  char *pathP = *path, *nextSepP = *path;
  char *closeIndex = NULL;
  Resource t1 = r;
  Resource t2;
  int32_t indexR = 0;
  UResType type = RES_GET_TYPE(t1);

  /* if you come in with an empty path, you'll be getting back the same resource */
  if(!uprv_strlen(pathP)) {
      return r;
  }

  /* one needs to have an aggregate resource in order to search in it */
  if(!URES_IS_CONTAINER(type)) {
      return RES_BOGUS;
  }
  
  while(nextSepP && *pathP && t1 != RES_BOGUS && URES_IS_CONTAINER(type)) {
    /* Iteration stops if: the path has been consumed, we found a non-existing
     * resource (t1 == RES_BOGUS) or we found a scalar resource (including alias)
     */
    nextSepP = uprv_strchr(pathP, RES_PATH_SEPARATOR);
    /* if there are more separators, terminate string 
     * and set path to the remaining part of the string
     */
    if(nextSepP != NULL) {
      *nextSepP = 0; /* overwrite the separator with a NUL to terminate the key */
      *path = nextSepP+1;
    } else {
      *path = uprv_strchr(pathP, 0);
    }

    /* if the resource is a table */
    /* try the key based access */
    if(URES_IS_TABLE(type)) {
      *key = pathP;
      t2 = res_getTableItemByKey(pResData, t1, &indexR, key);
      if(t2 == RES_BOGUS) { 
        /* if we fail to get the resource by key, maybe we got an index */
        indexR = uprv_strtol(pathP, &closeIndex, 10);
        if(closeIndex != pathP) {
          /* if we indeed have an index, try to get the item by index */
          t2 = res_getTableItemByIndex(pResData, t1, indexR, key);
        }
      }
    } else if(URES_IS_ARRAY(type)) {
      indexR = uprv_strtol(pathP, &closeIndex, 10);
      if(closeIndex != pathP) {
        t2 = res_getArrayItem(pResData, t1, indexR);
      } else {
        t2 = RES_BOGUS; /* have an array, but don't have a valid index */
      }
      *key = NULL;
    } else { /* can't do much here, except setting t2 to bogus */
      t2 = RES_BOGUS;
    }
    t1 = t2;
    type = RES_GET_TYPE(t1);
    /* position pathP to next resource key/index */
    pathP = *path;
  }

  return t1;
}

/* resource bundle swapping ------------------------------------------------- */

/*
 * Need to always enumerate the entire item tree,
 * track the lowest address of any item to use as the limit for char keys[],
 * track the highest address of any item to return the size of the data.
 *
 * We should have thought of storing those in the data...
 * It is possible to extend the data structure by putting additional values
 * in places that are inaccessible by ordinary enumeration of the item tree.
 * For example, additional integers could be stored at the beginning or
 * end of the key strings; this could be indicated by a minor version number,
 * and the data swapping would have to know about these values.
 *
 * The data structure does not forbid keys to be shared, so we must swap
 * all keys once instead of each key when it is referenced.
 *
 * These swapping functions assume that a resource bundle always has a length
 * that is a multiple of 4 bytes.
 * Currently, this is trivially true because genrb writes bundle tree leaves
 * physically first, before their branches, so that the root table with its
 * array of resource items (uint32_t values) is always last.
 */

/* definitions for table sorting ------------------------ */

/*
 * row of a temporary array
 *
 * gets platform-endian key string indexes and sorting indexes;
 * after sorting this array by keys, the actual key/value arrays are permutated
 * according to the sorting indexes
 */
typedef struct Row {
    int32_t keyIndex, sortIndex;
} Row;

static int32_t
ures_compareRows(const void *context, const void *left, const void *right) {
    const char *keyChars=(const char *)context;
    return (int32_t)uprv_strcmp(keyChars+((const Row *)left)->keyIndex,
                                keyChars+((const Row *)right)->keyIndex);
}

typedef struct TempTable {
    const char *keyChars;
    Row *rows;
    int32_t *resort;
    uint32_t *resFlags;
    int32_t localKeyLimit;
    uint8_t majorFormatVersion;
} TempTable;

enum {
    STACK_ROW_CAPACITY=200
};

/* The table item key string is not locally available. */
static const char *const gUnknownKey="";

/* resource table key for collation binaries: "%%CollationBin" */
static const UChar gCollationBinKey[]={
    0x25, 0x25,
    0x43, 0x6f, 0x6c, 0x6c, 0x61, 0x74, 0x69, 0x6f, 0x6e,
    0x42, 0x69, 0x6e,
    0
};

/*
 * swap one resource item
 */
static void
ures_swapResource(const UDataSwapper *ds,
                  const Resource *inBundle, Resource *outBundle,
                  Resource res, /* caller swaps res itself */
                  const char *key,
                  TempTable *pTempTable,
                  UErrorCode *pErrorCode) {
    const Resource *p;
    Resource *q;
    int32_t offset, count;

    switch(RES_GET_TYPE(res)) {
    case URES_TABLE16:
    case URES_STRING_V2:
    case URES_INT:
    case URES_ARRAY16:
        /* integer, or points to 16-bit units, nothing to do here */
        return;
    default:
        break;
    }

    /* all other types use an offset to point to their data */
    offset=(int32_t)RES_GET_OFFSET(res);
    if(offset==0) {
        /* special offset indicating an empty item */
        return;
    }
    if(pTempTable->resFlags[offset>>5]&((uint32_t)1<<(offset&0x1f))) {
        /* we already swapped this resource item */
        return;
    } else {
        /* mark it as swapped now */
        pTempTable->resFlags[offset>>5]|=((uint32_t)1<<(offset&0x1f));
    }

    p=inBundle+offset;
    q=outBundle+offset;

    switch(RES_GET_TYPE(res)) {
    case URES_ALIAS:
        /* physically same value layout as string, fall through */
    case URES_STRING:
        count=udata_readInt32(ds, (int32_t)*p);
        /* swap length */
        ds->swapArray32(ds, p, 4, q, pErrorCode);
        /* swap each UChar (the terminating NUL would not change) */
        ds->swapArray16(ds, p+1, 2*count, q+1, pErrorCode);
        break;
    case URES_BINARY:
        count=udata_readInt32(ds, (int32_t)*p);
        /* swap length */
        ds->swapArray32(ds, p, 4, q, pErrorCode);
        /* no need to swap or copy bytes - ures_swap() copied them all */

        /* swap known formats */
#if !UCONFIG_NO_COLLATION
        if( key!=NULL &&  /* the binary is in a table */
            (key!=gUnknownKey ?
                /* its table key string is "%%CollationBin" */
                0==ds->compareInvChars(ds, key, -1,
                                       gCollationBinKey, LENGTHOF(gCollationBinKey)-1) :
                /* its table key string is unknown but it looks like a collation binary */
                ucol_looksLikeCollationBinary(ds, p+1, count))
        ) {
            ucol_swapBinary(ds, p+1, count, q+1, pErrorCode);
        }
#endif
        break;
    case URES_TABLE:
    case URES_TABLE32:
        {
            const uint16_t *pKey16;
            uint16_t *qKey16;

            const int32_t *pKey32;
            int32_t *qKey32;

            Resource item;
            int32_t i, oldIndex;

            if(RES_GET_TYPE(res)==URES_TABLE) {
                /* get table item count */
                pKey16=(const uint16_t *)p;
                qKey16=(uint16_t *)q;
                count=ds->readUInt16(*pKey16);

                pKey32=qKey32=NULL;

                /* swap count */
                ds->swapArray16(ds, pKey16++, 2, qKey16++, pErrorCode);

                offset+=((1+count)+1)/2;
            } else {
                /* get table item count */
                pKey32=(const int32_t *)p;
                qKey32=(int32_t *)q;
                count=udata_readInt32(ds, *pKey32);

                pKey16=qKey16=NULL;

                /* swap count */
                ds->swapArray32(ds, pKey32++, 4, qKey32++, pErrorCode);

                offset+=1+count;
            }

            if(count==0) {
                break;
            }

            p=inBundle+offset; /* pointer to table resources */
            q=outBundle+offset;

            /* recurse */
            for(i=0; i<count; ++i) {
                const char *itemKey=gUnknownKey;
                if(pKey16!=NULL) {
                    int32_t keyOffset=ds->readUInt16(pKey16[i]);
                    if(keyOffset<pTempTable->localKeyLimit) {
                        itemKey=(const char *)outBundle+keyOffset;
                    }
                } else {
                    int32_t keyOffset=udata_readInt32(ds, pKey32[i]);
                    if(keyOffset>=0) {
                        itemKey=(const char *)outBundle+keyOffset;
                    }
                }
                item=ds->readUInt32(p[i]);
                ures_swapResource(ds, inBundle, outBundle, item, itemKey, pTempTable, pErrorCode);
                if(U_FAILURE(*pErrorCode)) {
                    udata_printError(ds, "ures_swapResource(table res=%08x)[%d].recurse(%08x) failed\n",
                                     res, i, item);
                    return;
                }
            }

            if(pTempTable->majorFormatVersion>1 || ds->inCharset==ds->outCharset) {
                /* no need to sort, just swap the offset/value arrays */
                if(pKey16!=NULL) {
                    ds->swapArray16(ds, pKey16, count*2, qKey16, pErrorCode);
                    ds->swapArray32(ds, p, count*4, q, pErrorCode);
                } else {
                    /* swap key offsets and items as one array */
                    ds->swapArray32(ds, pKey32, count*2*4, qKey32, pErrorCode);
                }
                break;
            }

            /*
             * We need to sort tables by outCharset key strings because they
             * sort differently for different charset families.
             * ures_swap() already set pTempTable->keyChars appropriately.
             * First we set up a temporary table with the key indexes and
             * sorting indexes and sort that.
             * Then we permutate and copy/swap the actual values.
             */
            if(pKey16!=NULL) {
                for(i=0; i<count; ++i) {
                    pTempTable->rows[i].keyIndex=ds->readUInt16(pKey16[i]);
                    pTempTable->rows[i].sortIndex=i;
                }
            } else {
                for(i=0; i<count; ++i) {
                    pTempTable->rows[i].keyIndex=udata_readInt32(ds, pKey32[i]);
                    pTempTable->rows[i].sortIndex=i;
                }
            }
            uprv_sortArray(pTempTable->rows, count, sizeof(Row),
                           ures_compareRows, pTempTable->keyChars,
                           FALSE, pErrorCode);
            if(U_FAILURE(*pErrorCode)) {
                udata_printError(ds, "ures_swapResource(table res=%08x).uprv_sortArray(%d items) failed\n",
                                 res, count);
                return;
            }

            /*
             * copy/swap/permutate items
             *
             * If we swap in-place, then the permutation must use another
             * temporary array (pTempTable->resort)
             * before the results are copied to the outBundle.
             */
            /* keys */
            if(pKey16!=NULL) {
                uint16_t *rKey16;

                if(pKey16!=qKey16) {
                    rKey16=qKey16;
                } else {
                    rKey16=(uint16_t *)pTempTable->resort;
                }
                for(i=0; i<count; ++i) {
                    oldIndex=pTempTable->rows[i].sortIndex;
                    ds->swapArray16(ds, pKey16+oldIndex, 2, rKey16+i, pErrorCode);
                }
                if(qKey16!=rKey16) {
                    uprv_memcpy(qKey16, rKey16, 2*count);
                }
            } else {
                int32_t *rKey32;

                if(pKey32!=qKey32) {
                    rKey32=qKey32;
                } else {
                    rKey32=pTempTable->resort;
                }
                for(i=0; i<count; ++i) {
                    oldIndex=pTempTable->rows[i].sortIndex;
                    ds->swapArray32(ds, pKey32+oldIndex, 4, rKey32+i, pErrorCode);
                }
                if(qKey32!=rKey32) {
                    uprv_memcpy(qKey32, rKey32, 4*count);
                }
            }

            /* resources */
            {
                Resource *r;


                if(p!=q) {
                    r=q;
                } else {
                    r=(Resource *)pTempTable->resort;
                }
                for(i=0; i<count; ++i) {
                    oldIndex=pTempTable->rows[i].sortIndex;
                    ds->swapArray32(ds, p+oldIndex, 4, r+i, pErrorCode);
                }
                if(q!=r) {
                    uprv_memcpy(q, r, 4*count);
                }
            }
        }
        break;
    case URES_ARRAY:
        {
            Resource item;
            int32_t i;

            count=udata_readInt32(ds, (int32_t)*p);
            /* swap length */
            ds->swapArray32(ds, p++, 4, q++, pErrorCode);

            /* recurse */
            for(i=0; i<count; ++i) {
                item=ds->readUInt32(p[i]);
                ures_swapResource(ds, inBundle, outBundle, item, NULL, pTempTable, pErrorCode);
                if(U_FAILURE(*pErrorCode)) {
                    udata_printError(ds, "ures_swapResource(array res=%08x)[%d].recurse(%08x) failed\n",
                                     res, i, item);
                    return;
                }
            }

            /* swap items */
            ds->swapArray32(ds, p, 4*count, q, pErrorCode);
        }
        break;
    case URES_INT_VECTOR:
        count=udata_readInt32(ds, (int32_t)*p);
        /* swap length and each integer */
        ds->swapArray32(ds, p, 4*(1+count), q, pErrorCode);
        break;
    default:
        /* also catches RES_BOGUS */
        *pErrorCode=U_UNSUPPORTED_ERROR;
        break;
    }
}

U_CAPI int32_t U_EXPORT2
ures_swap(const UDataSwapper *ds,
          const void *inData, int32_t length, void *outData,
          UErrorCode *pErrorCode) {
    const UDataInfo *pInfo;
    const Resource *inBundle;
    Resource rootRes;
    int32_t headerSize, maxTableLength;

    Row rows[STACK_ROW_CAPACITY];
    int32_t resort[STACK_ROW_CAPACITY];
    TempTable tempTable;

    const int32_t *inIndexes;

    /* the following integers count Resource item offsets (4 bytes each), not bytes */
    int32_t bundleLength, indexLength, keysBottom, keysTop, resBottom, top;

    /* udata_swapDataHeader checks the arguments */
    headerSize=udata_swapDataHeader(ds, inData, length, outData, pErrorCode);
    if(pErrorCode==NULL || U_FAILURE(*pErrorCode)) {
        return 0;
    }

    /* check data format and format version */
    pInfo=(const UDataInfo *)((const char *)inData+4);
    if(!(
        pInfo->dataFormat[0]==0x52 &&   /* dataFormat="ResB" */
        pInfo->dataFormat[1]==0x65 &&
        pInfo->dataFormat[2]==0x73 &&
        pInfo->dataFormat[3]==0x42 &&
        ((pInfo->formatVersion[0]==1 && pInfo->formatVersion[1]>=1) ||  /* formatVersion 1.1+ or 2.x */
         pInfo->formatVersion[0]==2)
    )) {
        udata_printError(ds, "ures_swap(): data format %02x.%02x.%02x.%02x (format version %02x.%02x) is not a resource bundle\n",
                         pInfo->dataFormat[0], pInfo->dataFormat[1],
                         pInfo->dataFormat[2], pInfo->dataFormat[3],
                         pInfo->formatVersion[0], pInfo->formatVersion[1]);
        *pErrorCode=U_UNSUPPORTED_ERROR;
        return 0;
    }
    tempTable.majorFormatVersion=pInfo->formatVersion[0];

    /* a resource bundle must contain at least one resource item */
    if(length<0) {
        bundleLength=-1;
    } else {
        bundleLength=(length-headerSize)/4;

        /* formatVersion 1.1 must have a root item and at least 5 indexes */
        if(bundleLength<(1+5)) {
            udata_printError(ds, "ures_swap(): too few bytes (%d after header) for a resource bundle\n",
                             length-headerSize);
            *pErrorCode=U_INDEX_OUTOFBOUNDS_ERROR;
            return 0;
        }
    }

    inBundle=(const Resource *)((const char *)inData+headerSize);
    rootRes=ds->readUInt32(*inBundle);

    /* formatVersion 1.1 adds the indexes[] array */
    inIndexes=(const int32_t *)(inBundle+1);

    indexLength=udata_readInt32(ds, inIndexes[URES_INDEX_LENGTH])&0xff;
    if(indexLength<=URES_INDEX_MAX_TABLE_LENGTH) {
        udata_printError(ds, "ures_swap(): too few indexes for a 1.1+ resource bundle\n");
        *pErrorCode=U_INDEX_OUTOFBOUNDS_ERROR;
        return 0;
    }
    keysBottom=1+indexLength;
    keysTop=udata_readInt32(ds, inIndexes[URES_INDEX_KEYS_TOP]);
    if(indexLength>URES_INDEX_16BIT_TOP) {
        resBottom=udata_readInt32(ds, inIndexes[URES_INDEX_16BIT_TOP]);
    } else {
        resBottom=keysTop;
    }
    top=udata_readInt32(ds, inIndexes[URES_INDEX_BUNDLE_TOP]);
    maxTableLength=udata_readInt32(ds, inIndexes[URES_INDEX_MAX_TABLE_LENGTH]);

    if(0<=bundleLength && bundleLength<top) {
        udata_printError(ds, "ures_swap(): resource top %d exceeds bundle length %d\n",
                         top, bundleLength);
        *pErrorCode=U_INDEX_OUTOFBOUNDS_ERROR;
        return 0;
    }
    if(keysTop>(1+indexLength)) {
        tempTable.localKeyLimit=keysTop<<2;
    } else {
        tempTable.localKeyLimit=0;
    }

    if(length>=0) {
        Resource *outBundle=(Resource *)((char *)outData+headerSize);

        /* track which resources we have already swapped */
        uint32_t stackResFlags[STACK_ROW_CAPACITY];
        int32_t resFlagsLength;

        /*
         * We need one bit per 4 resource bundle bytes so that we can track
         * every possible Resource for whether we have swapped it already.
         * Multiple Resource words can refer to the same bundle offsets
         * for sharing identical values.
         * We could optimize this by allocating only for locations above
         * where Resource values are stored (above keys & strings).
         */
        resFlagsLength=(length+31)>>5;          /* number of bytes needed */
        resFlagsLength=(resFlagsLength+3)&~3;   /* multiple of 4 bytes for uint32_t */
        if(resFlagsLength<=sizeof(stackResFlags)) {
            tempTable.resFlags=stackResFlags;
        } else {
            tempTable.resFlags=(uint32_t *)uprv_malloc(resFlagsLength);
            if(tempTable.resFlags==NULL) {
                udata_printError(ds, "ures_swap(): unable to allocate memory for tracking resources\n");
                *pErrorCode=U_MEMORY_ALLOCATION_ERROR;
                return 0;
            }
        }
        uprv_memset(tempTable.resFlags, 0, resFlagsLength);

        /* copy the bundle for binary and inaccessible data */
        if(inData!=outData) {
            uprv_memcpy(outBundle, inBundle, 4*top);
        }

        /* swap the key strings, but not the padding bytes (0xaa) after the last string and its NUL */
        udata_swapInvStringBlock(ds, inBundle+keysBottom, 4*(keysTop-keysBottom),
                                    outBundle+keysBottom, pErrorCode);
        if(U_FAILURE(*pErrorCode)) {
            udata_printError(ds, "ures_swap().udata_swapInvStringBlock(keys[%d]) failed\n", 4*(keysTop-keysBottom));
            return 0;
        }

        /* swap the 16-bit units (strings, table16, array16) */
        if(keysTop<resBottom) {
            ds->swapArray16(ds, inBundle+keysTop, (resBottom-keysTop)*4, outBundle+keysTop, pErrorCode);
            if(U_FAILURE(*pErrorCode)) {
                udata_printError(ds, "ures_swap().swapArray16(16-bit units[%d]) failed\n", 2*(resBottom-keysTop));
                return 0;
            }
        }

        /* allocate the temporary table for sorting resource tables */
        tempTable.keyChars=(const char *)outBundle; /* sort by outCharset */
        if(tempTable.majorFormatVersion>1 || maxTableLength<=STACK_ROW_CAPACITY) {
            tempTable.rows=rows;
            tempTable.resort=resort;
        } else {
            tempTable.rows=(Row *)uprv_malloc(maxTableLength*sizeof(Row)+maxTableLength*4);
            if(tempTable.rows==NULL) {
                udata_printError(ds, "ures_swap(): unable to allocate memory for sorting tables (max length: %d)\n",
                                 maxTableLength);
                *pErrorCode=U_MEMORY_ALLOCATION_ERROR;
                if(tempTable.resFlags!=stackResFlags) {
                    uprv_free(tempTable.resFlags);
                }
                return 0;
            }
            tempTable.resort=(int32_t *)(tempTable.rows+maxTableLength);
        }

        /* swap the resources */
        ures_swapResource(ds, inBundle, outBundle, rootRes, NULL, &tempTable, pErrorCode);
        if(U_FAILURE(*pErrorCode)) {
            udata_printError(ds, "ures_swapResource(root res=%08x) failed\n",
                             rootRes);
        }

        if(tempTable.rows!=rows) {
            uprv_free(tempTable.rows);
        }
        if(tempTable.resFlags!=stackResFlags) {
            uprv_free(tempTable.resFlags);
        }

        /* swap the root resource and indexes */
        ds->swapArray32(ds, inBundle, keysBottom*4, outBundle, pErrorCode);
    }

    return headerSize+4*top;
}
