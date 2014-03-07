/*
*******************************************************************************
*
*   Copyright (C) 1999-2010, International Business Machines
*   Corporation and others.  All Rights Reserved.
*
*******************************************************************************
*   file name:  package.cpp
*   encoding:   US-ASCII
*   tab size:   8 (not used)
*   indentation:4
*
*   created on: 2005aug25
*   created by: Markus W. Scherer
*
*   Read, modify, and write ICU .dat data package files.
*   This is an integral part of the icupkg tool, moved to the toolutil library
*   because parts of tool implementations tend to be later shared by
*   other tools.
*   Subsumes functionality and implementation code from
*   gencmn, decmn, and icuswap tools.
*/

#include "unicode/utypes.h"
#include "unicode/putil.h"
#include "unicode/udata.h"
#include "cstring.h"
#include "uarrsort.h"
#include "ucmndata.h"
#include "udataswp.h"
#include "swapimpl.h"
#include "toolutil.h"
#include "package.h"
#include "cmemory.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>


static const int32_t kItemsChunk = 256; /* How much to increase the filesarray by each time */

// general definitions ----------------------------------------------------- ***

#define LENGTHOF(array) (int32_t)(sizeof(array)/sizeof((array)[0]))

/* UDataInfo cf. udata.h */
static const UDataInfo dataInfo={
    (uint16_t)sizeof(UDataInfo),
    0,

    U_IS_BIG_ENDIAN,
    U_CHARSET_FAMILY,
    (uint8_t)sizeof(UChar),
    0,

    {0x43, 0x6d, 0x6e, 0x44},     /* dataFormat="CmnD" */
    {1, 0, 0, 0},                 /* formatVersion */
    {3, 0, 0, 0}                  /* dataVersion */
};

U_CDECL_BEGIN
static void U_CALLCONV
printPackageError(void *context, const char *fmt, va_list args) {
    vfprintf((FILE *)context, fmt, args);
}
U_CDECL_END

static uint16_t
readSwapUInt16(uint16_t x) {
    return (uint16_t)((x<<8)|(x>>8));
}

// platform types ---------------------------------------------------------- ***

static const char *types="lb?e";

enum { TYPE_L, TYPE_B, TYPE_LE, TYPE_E, TYPE_COUNT };

static inline int32_t
makeTypeEnum(uint8_t charset, UBool isBigEndian) {
    return 2*(int32_t)charset+isBigEndian;
}

static inline int32_t
makeTypeEnum(char type) {
    return
        type == 'l' ? TYPE_L :
        type == 'b' ? TYPE_B :
        type == 'e' ? TYPE_E :
               -1;
}

static inline char
makeTypeLetter(uint8_t charset, UBool isBigEndian) {
    return types[makeTypeEnum(charset, isBigEndian)];
}

static inline char
makeTypeLetter(int32_t typeEnum) {
    return types[typeEnum];
}

static void
makeTypeProps(char type, uint8_t &charset, UBool &isBigEndian) {
    int32_t typeEnum=makeTypeEnum(type);
    charset=(uint8_t)(typeEnum>>1);
    isBigEndian=(UBool)(typeEnum&1);
}

U_CFUNC const UDataInfo *
getDataInfo(const uint8_t *data, int32_t length,
            int32_t &infoLength, int32_t &headerLength,
            UErrorCode *pErrorCode) {
    const DataHeader *pHeader;
    const UDataInfo *pInfo;

    if(pErrorCode==NULL || U_FAILURE(*pErrorCode)) {
        return NULL;
    }
    if( data==NULL ||
        (length>=0 && length<(int32_t)sizeof(DataHeader))
    ) {
        *pErrorCode=U_ILLEGAL_ARGUMENT_ERROR;
        return NULL;
    }

    pHeader=(const DataHeader *)data;
    pInfo=&pHeader->info;
    if( (length>=0 && length<(int32_t)sizeof(DataHeader)) ||
        pHeader->dataHeader.magic1!=0xda ||
        pHeader->dataHeader.magic2!=0x27 ||
        pInfo->sizeofUChar!=2
    ) {
        *pErrorCode=U_UNSUPPORTED_ERROR;
        return NULL;
    }

    if(pInfo->isBigEndian==U_IS_BIG_ENDIAN) {
        headerLength=pHeader->dataHeader.headerSize;
        infoLength=pInfo->size;
    } else {
        headerLength=readSwapUInt16(pHeader->dataHeader.headerSize);
        infoLength=readSwapUInt16(pInfo->size);
    }

    if( headerLength<(int32_t)sizeof(DataHeader) ||
        infoLength<(int32_t)sizeof(UDataInfo) ||
        headerLength<(int32_t)(sizeof(pHeader->dataHeader)+infoLength) ||
        (length>=0 && length<headerLength)
    ) {
        *pErrorCode=U_UNSUPPORTED_ERROR;
        return NULL;
    }

    return pInfo;
}

static int32_t
getTypeEnumForInputData(const uint8_t *data, int32_t length,
                        UErrorCode *pErrorCode) {
    const UDataInfo *pInfo;
    int32_t infoLength, headerLength;

    /* getDataInfo() checks for illegal arguments */
    pInfo=getDataInfo(data, length, infoLength, headerLength, pErrorCode);
    if(pInfo==NULL) {
        return -1;
    }

    return makeTypeEnum(pInfo->charsetFamily, (UBool)pInfo->isBigEndian);
}

// file handling ----------------------------------------------------------- ***

static void
extractPackageName(const char *filename,
                   char pkg[], int32_t capacity) {
    const char *basename;
    int32_t len;

    basename=findBasename(filename);
    len=(int32_t)strlen(basename)-4; /* -4: subtract the length of ".dat" */

    if(len<=0 || 0!=strcmp(basename+len, ".dat")) {
        fprintf(stderr, "icupkg: \"%s\" is not recognized as a package filename (must end with .dat)\n",
                         basename);
        exit(U_ILLEGAL_ARGUMENT_ERROR);
    }

    if(len>=capacity) {
        fprintf(stderr, "icupkg: the package name \"%s\" is too long (>=%ld)\n",
                         basename, (long)capacity);
        exit(U_ILLEGAL_ARGUMENT_ERROR);
    }

    memcpy(pkg, basename, len);
    pkg[len]=0;
}

static int32_t
getFileLength(FILE *f) {
    int32_t length;

    fseek(f, 0, SEEK_END);
    length=(int32_t)ftell(f);
    fseek(f, 0, SEEK_SET);
    return length;
}

/*
 * Turn tree separators and alternate file separators into normal file separators.
 */
#if U_TREE_ENTRY_SEP_CHAR==U_FILE_SEP_CHAR && U_FILE_ALT_SEP_CHAR==U_FILE_SEP_CHAR
#define treeToPath(s)
#else
static void
treeToPath(char *s) {
    char *t;

    for(t=s; *t!=0; ++t) {
        if(*t==U_TREE_ENTRY_SEP_CHAR || *t==U_FILE_ALT_SEP_CHAR) {
            *t=U_FILE_SEP_CHAR;
        }
    }
}
#endif

/*
 * Turn file separators into tree separators.
 */
#if U_TREE_ENTRY_SEP_CHAR==U_FILE_SEP_CHAR && U_FILE_ALT_SEP_CHAR==U_FILE_SEP_CHAR
#define pathToTree(s)
#else
static void
pathToTree(char *s) {
    char *t;

    for(t=s; *t!=0; ++t) {
        if(*t==U_FILE_SEP_CHAR || *t==U_FILE_ALT_SEP_CHAR) {
            *t=U_TREE_ENTRY_SEP_CHAR;
        }
    }
}
#endif

/*
 * Prepend the path (if any) to the name and run the name through treeToName().
 */
static void
makeFullFilename(const char *path, const char *name,
                 char *filename, int32_t capacity) {
    char *s;

    // prepend the path unless NULL or empty
    if(path!=NULL && path[0]!=0) {
        if((int32_t)(strlen(path)+1)>=capacity) {
            fprintf(stderr, "pathname too long: \"%s\"\n", path);
            exit(U_BUFFER_OVERFLOW_ERROR);
        }
        strcpy(filename, path);

        // make sure the path ends with a file separator
        s=strchr(filename, 0);
        if(*(s-1)!=U_FILE_SEP_CHAR && *(s-1)!=U_FILE_ALT_SEP_CHAR) {
            *s++=U_FILE_SEP_CHAR;
        }
    } else {
        s=filename;
    }

    // turn the name into a filename, turn tree separators into file separators
    if((int32_t)((s-filename)+strlen(name))>=capacity) {
        fprintf(stderr, "path/filename too long: \"%s%s\"\n", filename, name);
        exit(U_BUFFER_OVERFLOW_ERROR);
    }
    strcpy(s, name);
    treeToPath(s);
}

static void
makeFullFilenameAndDirs(const char *path, const char *name,
                        char *filename, int32_t capacity) {
    char *sep;
    UErrorCode errorCode;

    makeFullFilename(path, name, filename, capacity);

    // make tree directories
    errorCode=U_ZERO_ERROR;
    sep=strchr(filename, 0)-strlen(name);
    while((sep=strchr(sep, U_FILE_SEP_CHAR))!=NULL) {
        if(sep!=filename) {
            *sep=0;                 // truncate temporarily
            uprv_mkdir(filename, &errorCode);
            if(U_FAILURE(errorCode)) {
                fprintf(stderr, "icupkg: unable to create tree directory \"%s\"\n", filename);
                exit(U_FILE_ACCESS_ERROR);
            }
        }
        *sep++=U_FILE_SEP_CHAR; // restore file separator character
    }
}

static uint8_t *
readFile(const char *path, const char *name, int32_t &length, char &type) {
    char filename[1024];
    FILE *file;
    uint8_t *data;
    UErrorCode errorCode;
    int32_t fileLength, typeEnum;

    makeFullFilename(path, name, filename, (int32_t)sizeof(filename));

    /* open the input file, get its length, allocate memory for it, read the file */
    file=fopen(filename, "rb");
    if(file==NULL) {
        fprintf(stderr, "icupkg: unable to open input file \"%s\"\n", filename);
        exit(U_FILE_ACCESS_ERROR);
    }

    /* get the file length */
    fileLength=getFileLength(file);
    if(ferror(file) || fileLength<=0) {
        fprintf(stderr, "icupkg: empty input file \"%s\"\n", filename);
        fclose(file);
        exit(U_FILE_ACCESS_ERROR);
    }

    /* allocate the buffer, pad to multiple of 16 */
    length=(fileLength+0xf)&~0xf;
    data=(uint8_t *)malloc(length);
    if(data==NULL) {
        fclose(file);
        exit(U_MEMORY_ALLOCATION_ERROR);
    }

    /* read the file */
    if(fileLength!=(int32_t)fread(data, 1, fileLength, file)) {
        fprintf(stderr, "icupkg: error reading \"%s\"\n", filename);
        fclose(file);
        free(data);
        exit(U_FILE_ACCESS_ERROR);
    }

    /* pad the file to a multiple of 16 using the usual padding byte */
    if(fileLength<length) {
        memset(data+fileLength, 0xaa, length-fileLength);
    }

    fclose(file);

    // minimum check for ICU-format data
    errorCode=U_ZERO_ERROR;
    typeEnum=getTypeEnumForInputData(data, length, &errorCode);
    if(typeEnum<0 || U_FAILURE(errorCode)) {
        fprintf(stderr, "icupkg: not an ICU data file: \"%s\"\n", filename);
        free(data);
#if !UCONFIG_NO_LEGACY_CONVERSION
        exit(U_INVALID_FORMAT_ERROR);
#else
        fprintf(stderr, "U_INVALID_FORMAT_ERROR occurred but UCONFIG_NO_LEGACY_CONVERSION is on so this is expected.\n");
        exit(0);
#endif
    }
    type=makeTypeLetter(typeEnum);

    return data;
}

// .dat package file representation ---------------------------------------- ***

U_CDECL_BEGIN

static int32_t U_CALLCONV
compareItems(const void * /*context*/, const void *left, const void *right) {
    U_NAMESPACE_USE

    return (int32_t)strcmp(((Item *)left)->name, ((Item *)right)->name);
}

U_CDECL_END

U_NAMESPACE_BEGIN

Package::Package() {
    inPkgName[0]=0;
    inData=NULL;
    inLength=0;
    inCharset=U_CHARSET_FAMILY;
    inIsBigEndian=U_IS_BIG_ENDIAN;

    itemCount=0;
    itemMax=0;
    items=NULL;

    inStringTop=outStringTop=0;

    matchMode=0;
    findPrefix=findSuffix=NULL;
    findPrefixLength=findSuffixLength=0;
    findNextIndex=-1;

    // create a header for an empty package
    DataHeader *pHeader;
    pHeader=(DataHeader *)header;
    pHeader->dataHeader.magic1=0xda;
    pHeader->dataHeader.magic2=0x27;
    memcpy(&pHeader->info, &dataInfo, sizeof(dataInfo));
    headerLength=(int32_t)(4+sizeof(dataInfo));
    if(headerLength&0xf) {
        /* NUL-pad the header to a multiple of 16 */
        int32_t length=(headerLength+0xf)&~0xf;
        memset(header+headerLength, 0, length-headerLength);
        headerLength=length;
    }
    pHeader->dataHeader.headerSize=(uint16_t)headerLength;
}

Package::~Package() {
    int32_t idx;

    free(inData);

    for(idx=0; idx<itemCount; ++idx) {
        if(items[idx].isDataOwned) {
            free(items[idx].data);
        }
    }

    uprv_free((void*)items);
}

void
Package::readPackage(const char *filename) {
    UDataSwapper *ds;
    const UDataInfo *pInfo;
    UErrorCode errorCode;

    const uint8_t *inBytes;

    int32_t length, offset, i;
    int32_t itemLength, typeEnum;
    char type;

    const UDataOffsetTOCEntry *inEntries;

    extractPackageName(filename, inPkgName, (int32_t)sizeof(inPkgName));

    /* read the file */
    inData=readFile(NULL, filename, inLength, type);
    length=inLength;

    /*
     * swap the header - even if the swapping itself is a no-op
     * because it tells us the header length
     */
    errorCode=U_ZERO_ERROR;
    makeTypeProps(type, inCharset, inIsBigEndian);
    ds=udata_openSwapper(inIsBigEndian, inCharset, U_IS_BIG_ENDIAN, U_CHARSET_FAMILY, &errorCode);
    if(U_FAILURE(errorCode)) {
        fprintf(stderr, "icupkg: udata_openSwapper(\"%s\") failed - %s\n",
                filename, u_errorName(errorCode));
        exit(errorCode);
    }

    ds->printError=printPackageError;
    ds->printErrorContext=stderr;

    headerLength=sizeof(header);
    if(length<headerLength) {
        headerLength=length;
    }
    headerLength=udata_swapDataHeader(ds, inData, headerLength, header, &errorCode);
    if(U_FAILURE(errorCode)) {
        exit(errorCode);
    }

    /* check data format and format version */
    pInfo=(const UDataInfo *)((const char *)inData+4);
    if(!(
        pInfo->dataFormat[0]==0x43 &&   /* dataFormat="CmnD" */
        pInfo->dataFormat[1]==0x6d &&
        pInfo->dataFormat[2]==0x6e &&
        pInfo->dataFormat[3]==0x44 &&
        pInfo->formatVersion[0]==1
    )) {
        fprintf(stderr, "icupkg: data format %02x.%02x.%02x.%02x (format version %02x) is not recognized as an ICU .dat package\n",
                pInfo->dataFormat[0], pInfo->dataFormat[1],
                pInfo->dataFormat[2], pInfo->dataFormat[3],
                pInfo->formatVersion[0]);
        exit(U_UNSUPPORTED_ERROR);
    }
    inIsBigEndian=(UBool)pInfo->isBigEndian;
    inCharset=pInfo->charsetFamily;

    inBytes=(const uint8_t *)inData+headerLength;
    inEntries=(const UDataOffsetTOCEntry *)(inBytes+4);

    /* check that the itemCount fits, then the ToC table, then at least the header of the last item */
    length-=headerLength;
    if(length<4) {
        /* itemCount does not fit */
        offset=0x7fffffff;
    } else {
        itemCount=udata_readInt32(ds, *(const int32_t *)inBytes);
        setItemCapacity(itemCount); /* resize so there's space */
        if(itemCount==0) {
            offset=4;
        } else if(length<(4+8*itemCount)) {
            /* ToC table does not fit */
            offset=0x7fffffff;
        } else {
            /* offset of the last item plus at least 20 bytes for its header */
            offset=20+(int32_t)ds->readUInt32(inEntries[itemCount-1].dataOffset);
        }
    }
    if(length<offset) {
        fprintf(stderr, "icupkg: too few bytes (%ld after header) for a .dat package\n",
                        (long)length);
        exit(U_INDEX_OUTOFBOUNDS_ERROR);
    }
    /* do not modify the package length variable until the last item's length is set */

    if(itemCount>0) {
        char prefix[MAX_PKG_NAME_LENGTH+4];
        char *s, *inItemStrings;
        int32_t inPkgNameLength, prefixLength, stringsOffset;

        if(itemCount>itemMax) {
            fprintf(stderr, "icupkg: too many items, maximum is %d\n", itemMax);
            exit(U_BUFFER_OVERFLOW_ERROR);
        }

        /* swap the item name strings */
        stringsOffset=4+8*itemCount;
        itemLength=(int32_t)(ds->readUInt32(inEntries[0].dataOffset))-stringsOffset;

        // don't include padding bytes at the end of the item names
        while(itemLength>0 && inBytes[stringsOffset+itemLength-1]!=0) {
            --itemLength;
        }

        if((inStringTop+itemLength)>STRING_STORE_SIZE) {
            fprintf(stderr, "icupkg: total length of item name strings too long\n");
            exit(U_BUFFER_OVERFLOW_ERROR);
        }

        inItemStrings=inStrings+inStringTop;
        ds->swapInvChars(ds, inBytes+stringsOffset, itemLength, inItemStrings, &errorCode);
        if(U_FAILURE(errorCode)) {
            fprintf(stderr, "icupkg failed to swap the input .dat package item name strings\n");
            exit(U_INVALID_FORMAT_ERROR);
        }
        inStringTop+=itemLength;

        // reset the Item entries
        memset(items, 0, itemCount*sizeof(Item));

        inPkgNameLength=strlen(inPkgName);
        memcpy(prefix, inPkgName, inPkgNameLength);
        prefixLength=inPkgNameLength;

        /*
         * Get the common prefix of the items.
         * New-style ICU .dat packages use tree separators ('/') between package names,
         * tree names, and item names,
         * while old-style ICU .dat packages (before multi-tree support)
         * use an underscore ('_') between package and item names.
         */
        offset=(int32_t)ds->readUInt32(inEntries[0].nameOffset)-stringsOffset;
        s=inItemStrings+offset;
        if( (int32_t)strlen(s)>=(inPkgNameLength+2) &&
            0==memcmp(s, inPkgName, inPkgNameLength) &&
            s[inPkgNameLength]=='_'
        ) {
            // old-style .dat package
            prefix[prefixLength++]='_';
        } else {
            // new-style .dat package
            prefix[prefixLength++]=U_TREE_ENTRY_SEP_CHAR;
            // if it turns out to not contain U_TREE_ENTRY_SEP_CHAR
            // then the test in the loop below will fail
        }
        prefix[prefixLength]=0;

        /* read the ToC table */
        for(i=0; i<itemCount; ++i) {
            // skip the package part of the item name, error if it does not match the actual package name
            // or if nothing follows the package name
            offset=(int32_t)ds->readUInt32(inEntries[i].nameOffset)-stringsOffset;
            s=inItemStrings+offset;
            if(0!=strncmp(s, prefix, prefixLength) || s[prefixLength]==0) {
                fprintf(stderr, "icupkg: input .dat item name \"%s\" does not start with \"%s\"\n",
                        s, prefix);
                exit(U_UNSUPPORTED_ERROR);
            }
            items[i].name=s+prefixLength;

            // set the item's data
            items[i].data=(uint8_t *)inBytes+ds->readUInt32(inEntries[i].dataOffset);
            if(i>0) {
                items[i-1].length=(int32_t)(items[i].data-items[i-1].data);

                // set the previous item's platform type
                typeEnum=getTypeEnumForInputData(items[i-1].data, items[i-1].length, &errorCode);
                if(typeEnum<0 || U_FAILURE(errorCode)) {
                    fprintf(stderr, "icupkg: not an ICU data file: item \"%s\" in \"%s\"\n", items[i-1].name, filename);
                    exit(U_INVALID_FORMAT_ERROR);
                }
                items[i-1].type=makeTypeLetter(typeEnum);
            }
            items[i].isDataOwned=FALSE;
        }
        // set the last item's length
        items[itemCount-1].length=length-ds->readUInt32(inEntries[itemCount-1].dataOffset);

        // set the last item's platform type
        typeEnum=getTypeEnumForInputData(items[itemCount-1].data, items[itemCount-1].length, &errorCode);
        if(typeEnum<0 || U_FAILURE(errorCode)) {
            fprintf(stderr, "icupkg: not an ICU data file: item \"%s\" in \"%s\"\n", items[i-1].name, filename);
            exit(U_INVALID_FORMAT_ERROR);
        }
        items[itemCount-1].type=makeTypeLetter(typeEnum);

        if(type!=U_ICUDATA_TYPE_LETTER[0]) {
            // sort the item names for the local charset
            sortItems();
        }
    }

    udata_closeSwapper(ds);
}

char
Package::getInType() {
    return makeTypeLetter(inCharset, inIsBigEndian);
}

void
Package::writePackage(const char *filename, char outType, const char *comment) {
    char prefix[MAX_PKG_NAME_LENGTH+4];
    UDataOffsetTOCEntry entry;
    UDataSwapper *dsLocalToOut, *ds[TYPE_COUNT];
    FILE *file;
    Item *pItem;
    char *name;
    UErrorCode errorCode;
    int32_t i, length, prefixLength, maxItemLength, basenameOffset, offset, outInt32;
    uint8_t outCharset;
    UBool outIsBigEndian;

    extractPackageName(filename, prefix, MAX_PKG_NAME_LENGTH);

    // if there is an explicit comment, then use it, else use what's in the current header
    if(comment!=NULL) {
        /* get the header size minus the current comment */
        DataHeader *pHeader;
        int32_t length;

        pHeader=(DataHeader *)header;
        headerLength=4+pHeader->info.size;
        length=(int32_t)strlen(comment);
        if((int32_t)(headerLength+length)>=(int32_t)sizeof(header)) {
            fprintf(stderr, "icupkg: comment too long\n");
            exit(U_BUFFER_OVERFLOW_ERROR);
        }
        memcpy(header+headerLength, comment, length+1);
        headerLength+=length;
        if(headerLength&0xf) {
            /* NUL-pad the header to a multiple of 16 */
            length=(headerLength+0xf)&~0xf;
            memset(header+headerLength, 0, length-headerLength);
            headerLength=length;
        }
        pHeader->dataHeader.headerSize=(uint16_t)headerLength;
    }

    makeTypeProps(outType, outCharset, outIsBigEndian);

    // open (TYPE_COUNT-2) swappers
    // one is a no-op for local type==outType
    // one type (TYPE_LE) is bogus
    errorCode=U_ZERO_ERROR;
    i=makeTypeEnum(outType);
    ds[TYPE_B]= i==TYPE_B ? NULL : udata_openSwapper(TRUE, U_ASCII_FAMILY, outIsBigEndian, outCharset, &errorCode);
    ds[TYPE_L]= i==TYPE_L ? NULL : udata_openSwapper(FALSE, U_ASCII_FAMILY, outIsBigEndian, outCharset, &errorCode);
    ds[TYPE_LE]=NULL;
    ds[TYPE_E]= i==TYPE_E ? NULL : udata_openSwapper(TRUE, U_EBCDIC_FAMILY, outIsBigEndian, outCharset, &errorCode);
    if(U_FAILURE(errorCode)) {
        fprintf(stderr, "icupkg: udata_openSwapper() failed - %s\n", u_errorName(errorCode));
        exit(errorCode);
    }
    for(i=0; i<TYPE_COUNT; ++i) {
        if(ds[i]!=NULL) {
            ds[i]->printError=printPackageError;
            ds[i]->printErrorContext=stderr;
        }
    }

    dsLocalToOut=ds[makeTypeEnum(U_CHARSET_FAMILY, U_IS_BIG_ENDIAN)];

    // create the file and write its contents
    file=fopen(filename, "wb");
    if(file==NULL) {
        fprintf(stderr, "icupkg: unable to create file \"%s\"\n", filename);
        exit(U_FILE_ACCESS_ERROR);
    }

    // swap and write the header
    if(dsLocalToOut!=NULL) {
        udata_swapDataHeader(dsLocalToOut, header, headerLength, header, &errorCode);
        if(U_FAILURE(errorCode)) {
            fprintf(stderr, "icupkg: udata_swapDataHeader(local to out) failed - %s\n", u_errorName(errorCode));
            exit(errorCode);
        }
    }
    length=(int32_t)fwrite(header, 1, headerLength, file);
    if(length!=headerLength) {
        fprintf(stderr, "icupkg: unable to write complete header to file \"%s\"\n", filename);
        exit(U_FILE_ACCESS_ERROR);
    }

    // prepare and swap the package name with a tree separator
    // for prepending to item names
    strcat(prefix, U_TREE_ENTRY_SEP_STRING);
    prefixLength=(int32_t)strlen(prefix);
    if(dsLocalToOut!=NULL) {
        dsLocalToOut->swapInvChars(dsLocalToOut, prefix, prefixLength, prefix, &errorCode);
        if(U_FAILURE(errorCode)) {
            fprintf(stderr, "icupkg: swapInvChars(output package name) failed - %s\n", u_errorName(errorCode));
            exit(errorCode);
        }

        // swap and sort the item names (sorting needs to be done in the output charset)
        dsLocalToOut->swapInvChars(dsLocalToOut, inStrings, inStringTop, inStrings, &errorCode);
        if(U_FAILURE(errorCode)) {
            fprintf(stderr, "icupkg: swapInvChars(item names) failed - %s\n", u_errorName(errorCode));
            exit(errorCode);
        }
        sortItems();
    }

    // create the output item names in sorted order, with the package name prepended to each
    for(i=0; i<itemCount; ++i) {
        length=(int32_t)strlen(items[i].name);
        name=allocString(FALSE, length+prefixLength);
        memcpy(name, prefix, prefixLength);
        memcpy(name+prefixLength, items[i].name, length+1);
        items[i].name=name;
    }

    // calculate offsets for item names and items, pad to 16-align items
    // align only the first item; each item's length is a multiple of 16
    basenameOffset=4+8*itemCount;
    offset=basenameOffset+outStringTop;
    if((length=(offset&15))!=0) {
        length=16-length;
        memset(allocString(FALSE, length-1), 0xaa, length);
        offset+=length;
    }

    // write the table of contents
    // first the itemCount
    outInt32=itemCount;
    if(dsLocalToOut!=NULL) {
        dsLocalToOut->swapArray32(dsLocalToOut, &outInt32, 4, &outInt32, &errorCode);
        if(U_FAILURE(errorCode)) {
            fprintf(stderr, "icupkg: swapArray32(item count) failed - %s\n", u_errorName(errorCode));
            exit(errorCode);
        }
    }
    length=(int32_t)fwrite(&outInt32, 1, 4, file);
    if(length!=4) {
        fprintf(stderr, "icupkg: unable to write complete item count to file \"%s\"\n", filename);
        exit(U_FILE_ACCESS_ERROR);
    }

    // then write the item entries (and collect the maxItemLength)
    maxItemLength=0;
    for(i=0; i<itemCount; ++i) {
        entry.nameOffset=(uint32_t)(basenameOffset+(items[i].name-outStrings));
        entry.dataOffset=(uint32_t)offset;
        if(dsLocalToOut!=NULL) {
            dsLocalToOut->swapArray32(dsLocalToOut, &entry, 8, &entry, &errorCode);
            if(U_FAILURE(errorCode)) {
                fprintf(stderr, "icupkg: swapArray32(item entry %ld) failed - %s\n", (long)i, u_errorName(errorCode));
                exit(errorCode);
            }
        }
        length=(int32_t)fwrite(&entry, 1, 8, file);
        if(length!=8) {
            fprintf(stderr, "icupkg: unable to write complete item entry %ld to file \"%s\"\n", (long)i, filename);
            exit(U_FILE_ACCESS_ERROR);
        }

        length=items[i].length;
        if(length>maxItemLength) {
            maxItemLength=length;
        }
        offset+=length;
    }

    // write the item names
    length=(int32_t)fwrite(outStrings, 1, outStringTop, file);
    if(length!=outStringTop) {
        fprintf(stderr, "icupkg: unable to write complete item names to file \"%s\"\n", filename);
        exit(U_FILE_ACCESS_ERROR);
    }

    // write the items
    for(pItem=items, i=0; i<itemCount; ++pItem, ++i) {
        int32_t type=makeTypeEnum(pItem->type);
        if(ds[type]!=NULL) {
            // swap each item from its platform properties to the desired ones
            udata_swap(
                ds[type],
                pItem->data, pItem->length, pItem->data,
                &errorCode);
            if(U_FAILURE(errorCode)) {
                fprintf(stderr, "icupkg: udata_swap(item %ld) failed - %s\n", (long)i, u_errorName(errorCode));
                exit(errorCode);
            }
        }
        length=(int32_t)fwrite(pItem->data, 1, pItem->length, file);
        if(length!=pItem->length) {
            fprintf(stderr, "icupkg: unable to write complete item %ld to file \"%s\"\n", (long)i, filename);
            exit(U_FILE_ACCESS_ERROR);
        }
    }

    if(ferror(file)) {
        fprintf(stderr, "icupkg: unable to write complete file \"%s\"\n", filename);
        exit(U_FILE_ACCESS_ERROR);
    }

    fclose(file);
    for(i=0; i<TYPE_COUNT; ++i) {
        udata_closeSwapper(ds[i]);
    }
}

int32_t
Package::findItem(const char *name, int32_t length) const {
    int32_t i, start, limit;
    int result;

    /* do a binary search for the string */
    start=0;
    limit=itemCount;
    while(start<limit) {
        i=(start+limit)/2;
        if(length>=0) {
            result=strncmp(name, items[i].name, length);
        } else {
            result=strcmp(name, items[i].name);
        }

        if(result==0) {
            /* found */
            if(length>=0) {
                /*
                 * if we compared just prefixes, then we may need to back up
                 * to the first item with this prefix
                 */
                while(i>0 && 0==strncmp(name, items[i-1].name, length)) {
                    --i;
                }
            }
            return i;
        } else if(result<0) {
            limit=i;
        } else /* result>0 */ {
            start=i+1;
        }
    }

    return ~start; /* not found, return binary-not of the insertion point */
}

void
Package::findItems(const char *pattern) {
    const char *wild;

    if(pattern==NULL || *pattern==0) {
        findNextIndex=-1;
        return;
    }

    findPrefix=pattern;
    findSuffix=NULL;
    findSuffixLength=0;

    wild=strchr(pattern, '*');
    if(wild==NULL) {
        // no wildcard
        findPrefixLength=(int32_t)strlen(pattern);
    } else {
        // one wildcard
        findPrefixLength=(int32_t)(wild-pattern);
        findSuffix=wild+1;
        findSuffixLength=(int32_t)strlen(findSuffix);
        if(NULL!=strchr(findSuffix, '*')) {
            // two or more wildcards
            fprintf(stderr, "icupkg: syntax error (more than one '*') in item pattern \"%s\"\n", pattern);
            exit(U_PARSE_ERROR);
        }
    }

    if(findPrefixLength==0) {
        findNextIndex=0;
    } else {
        findNextIndex=findItem(findPrefix, findPrefixLength);
    }
}

int32_t
Package::findNextItem() {
    const char *name, *middle, *treeSep;
    int32_t idx, nameLength, middleLength;

    if(findNextIndex<0) {
        return -1;
    }

    while(findNextIndex<itemCount) {
        idx=findNextIndex++;
        name=items[idx].name;
        nameLength=(int32_t)strlen(name);
        if(nameLength<(findPrefixLength+findSuffixLength)) {
            // item name too short for prefix & suffix
            continue;
        }
        if(findPrefixLength>0 && 0!=memcmp(findPrefix, name, findPrefixLength)) {
            // left the range of names with this prefix
            break;
        }
        middle=name+findPrefixLength;
        middleLength=nameLength-findPrefixLength-findSuffixLength;
        if(findSuffixLength>0 && 0!=memcmp(findSuffix, name+(nameLength-findSuffixLength), findSuffixLength)) {
            // suffix does not match
            continue;
        }
        // prefix & suffix match

        if(matchMode&MATCH_NOSLASH) {
            treeSep=strchr(middle, U_TREE_ENTRY_SEP_CHAR);
            if(treeSep!=NULL && (treeSep-middle)<middleLength) {
                // the middle (matching the * wildcard) contains a tree separator /
                continue;
            }
        }

        // found a matching item
        return idx;
    }

    // no more items
    findNextIndex=-1;
    return -1;
}

void
Package::setMatchMode(uint32_t mode) {
    matchMode=mode;
}

void
Package::addItem(const char *name) {
    addItem(name, NULL, 0, FALSE, U_ICUDATA_TYPE_LETTER[0]);
}

void
Package::addItem(const char *name, uint8_t *data, int32_t length, UBool isDataOwned, char type) {
    int32_t idx;

    idx=findItem(name);
    if(idx<0) {
        // new item, make space at the insertion point
        ensureItemCapacity();
        // move the following items down
        idx=~idx;
        if(idx<itemCount) {
            memmove(items+idx+1, items+idx, (itemCount-idx)*sizeof(Item));
        }
        ++itemCount;

        // reset this Item entry
        memset(items+idx, 0, sizeof(Item));

        // copy the item's name
        items[idx].name=allocString(TRUE, strlen(name));
        strcpy(items[idx].name, name);
        pathToTree(items[idx].name);
    } else {
        // same-name item found, replace it
        if(items[idx].isDataOwned) {
            free(items[idx].data);
        }

        // keep the item's name since it is the same
    }

    // set the item's data
    items[idx].data=data;
    items[idx].length=length;
    items[idx].isDataOwned=isDataOwned;
    items[idx].type=type;
}

void
Package::addFile(const char *filesPath, const char *name) {
    uint8_t *data;
    int32_t length;
    char type;

    data=readFile(filesPath, name, length, type);
    // readFile() exits the tool if it fails
    addItem(name, data, length, TRUE, type);
}

void
Package::addItems(const Package &listPkg) {
    const Item *pItem;
    int32_t i;

    for(pItem=listPkg.items, i=0; i<listPkg.itemCount; ++pItem, ++i) {
        addItem(pItem->name, pItem->data, pItem->length, FALSE, pItem->type);
    }
}

void
Package::removeItem(int32_t idx) {
    if(idx>=0) {
        // remove the item
        if(items[idx].isDataOwned) {
            free(items[idx].data);
        }

        // move the following items up
        if((idx+1)<itemCount) {
            memmove(items+idx, items+idx+1, (itemCount-(idx+1))*sizeof(Item));
        }
        --itemCount;

        if(idx<=findNextIndex) {
            --findNextIndex;
        }
    }
}

void
Package::removeItems(const char *pattern) {
    int32_t idx;

    findItems(pattern);
    while((idx=findNextItem())>=0) {
        removeItem(idx);
    }
}

void
Package::removeItems(const Package &listPkg) {
    const Item *pItem;
    int32_t i;

    for(pItem=listPkg.items, i=0; i<listPkg.itemCount; ++pItem, ++i) {
        removeItems(pItem->name);
    }
}

void
Package::extractItem(const char *filesPath, const char *outName, int32_t idx, char outType) {
    char filename[1024];
    UDataSwapper *ds;
    FILE *file;
    Item *pItem;
    int32_t fileLength;
    uint8_t itemCharset, outCharset;
    UBool itemIsBigEndian, outIsBigEndian;

    if(idx<0 || itemCount<=idx) {
        return;
    }
    pItem=items+idx;

    // swap the data to the outType
    // outType==0: don't swap
    if(outType!=0 && pItem->type!=outType) {
        // open the swapper
        UErrorCode errorCode=U_ZERO_ERROR;
        makeTypeProps(pItem->type, itemCharset, itemIsBigEndian);
        makeTypeProps(outType, outCharset, outIsBigEndian);
        ds=udata_openSwapper(itemIsBigEndian, itemCharset, outIsBigEndian, outCharset, &errorCode);
        if(U_FAILURE(errorCode)) {
            fprintf(stderr, "icupkg: udata_openSwapper(item %ld) failed - %s\n",
                    (long)idx, u_errorName(errorCode));
            exit(errorCode);
        }

        ds->printError=printPackageError;
        ds->printErrorContext=stderr;

        // swap the item from its platform properties to the desired ones
        udata_swap(ds, pItem->data, pItem->length, pItem->data, &errorCode);
        if(U_FAILURE(errorCode)) {
            fprintf(stderr, "icupkg: udata_swap(item %ld) failed - %s\n", (long)idx, u_errorName(errorCode));
            exit(errorCode);
        }
        udata_closeSwapper(ds);
        pItem->type=outType;
    }

    // create the file and write its contents
    makeFullFilenameAndDirs(filesPath, outName, filename, (int32_t)sizeof(filename));
    file=fopen(filename, "wb");
    if(file==NULL) {
        fprintf(stderr, "icupkg: unable to create file \"%s\"\n", filename);
        exit(U_FILE_ACCESS_ERROR);
    }
    fileLength=(int32_t)fwrite(pItem->data, 1, pItem->length, file);

    if(ferror(file) || fileLength!=pItem->length) {
        fprintf(stderr, "icupkg: unable to write complete file \"%s\"\n", filename);
        exit(U_FILE_ACCESS_ERROR);
    }
    fclose(file);
}

void
Package::extractItem(const char *filesPath, int32_t idx, char outType) {
    extractItem(filesPath, items[idx].name, idx, outType);
}

void
Package::extractItems(const char *filesPath, const char *pattern, char outType) {
    int32_t idx;

    findItems(pattern);
    while((idx=findNextItem())>=0) {
        extractItem(filesPath, idx, outType);
    }
}

void
Package::extractItems(const char *filesPath, const Package &listPkg, char outType) {
    const Item *pItem;
    int32_t i;

    for(pItem=listPkg.items, i=0; i<listPkg.itemCount; ++pItem, ++i) {
        extractItems(filesPath, pItem->name, outType);
    }
}

int32_t
Package::getItemCount() const {
    return itemCount;
}

const Item *
Package::getItem(int32_t idx) const {
    if (0 <= idx && idx < itemCount) {
        return &items[idx];
    }
    return NULL;
}

void
Package::checkDependency(void *context, const char *itemName, const char *targetName) {
    // check dependency: make sure the target item is in the package
    Package *me=(Package *)context;
    if(me->findItem(targetName)<0) {
        me->isMissingItems=TRUE;
        fprintf(stderr, "Item %s depends on missing item %s\n", itemName, targetName);
    }
}

UBool
Package::checkDependencies() {
    isMissingItems=FALSE;
    enumDependencies(this, checkDependency);
    return (UBool)!isMissingItems;
}

void
Package::enumDependencies(void *context, CheckDependency check) {
    int32_t i;

    for(i=0; i<itemCount; ++i) {
        enumDependencies(items+i, context, check);
    }
}

char *
Package::allocString(UBool in, int32_t length) {
    char *p;
    int32_t top;

    if(in) {
        top=inStringTop;
        p=inStrings+top;
    } else {
        top=outStringTop;
        p=outStrings+top;
    }
    top+=length+1;

    if(top>STRING_STORE_SIZE) {
        fprintf(stderr, "icupkg: string storage overflow\n");
        exit(U_BUFFER_OVERFLOW_ERROR);
    }
    if(in) {
        inStringTop=top;
    } else {
        outStringTop=top;
    }
    return p;
}

void
Package::sortItems() {
    UErrorCode errorCode=U_ZERO_ERROR;
    uprv_sortArray(items, itemCount, (int32_t)sizeof(Item), compareItems, NULL, FALSE, &errorCode);
    if(U_FAILURE(errorCode)) {
        fprintf(stderr, "icupkg: sorting item names failed - %s\n", u_errorName(errorCode));
        exit(errorCode);
    }
}

void Package::setItemCapacity(int32_t max) 
{
  if(max<=itemMax) {
    return;
  }
  Item *newItems = (Item*)uprv_malloc(max * sizeof(items[0]));
  Item *oldItems = items;
  if(newItems == NULL) {
    fprintf(stderr, "icupkg: Out of memory trying to allocate %ld bytes for %d items\n", max*sizeof(items[0]), max);
    exit(U_MEMORY_ALLOCATION_ERROR);
  }
  if(items && itemCount>0) {
    uprv_memcpy(newItems, items, itemCount*sizeof(items[0]));
  }
  itemMax = max;
  items = newItems;
  uprv_free(oldItems);
}

void Package::ensureItemCapacity()
{
  if((itemCount+1)>itemMax) {
    setItemCapacity(itemCount+kItemsChunk);
  }
}

U_NAMESPACE_END
