/*
*******************************************************************************
*
*   Copyright (C) 2005-2010, International Business Machines
*   Corporation and others.  All Rights Reserved.
*
*******************************************************************************
*   file name:  swapimpl.cpp
*   encoding:   US-ASCII
*   tab size:   8 (not used)
*   indentation:4
*
*   created on: 2005may05
*   created by: Markus W. Scherer
*
*   Data file swapping functions moved here from the common library
*   because some data is hardcoded in ICU4C and needs not be swapped any more.
*   Moving the functions here simplifies testing (for code coverage) because
*   we need not jump through hoops (like adding snapshots of these files
*   to testdata).
*
*   The declarations for these functions remain in the internal header files
*   in icu/source/common/
*/

#include "unicode/utypes.h"
#include "unicode/putil.h"
#include "unicode/udata.h"

/* Explicit include statement for std_string.h is needed
 * for compilation on certain platforms. (e.g. AIX/VACPP)
 */
#include "unicode/std_string.h"

#include "cmemory.h"
#include "cstring.h"
#include "uinvchar.h"
#include "uassert.h"
#include "uarrsort.h"
#include "ucmndata.h"
#include "udataswp.h"

/* swapping implementations in common */

#include "uresdata.h"
#include "ucnv_io.h"
#include "uprops.h"
#include "ucase.h"
#include "ubidi_props.h"
#include "ucol_swp.h"
#include "ucnv_bld.h"
#include "unormimp.h"
#include "normalizer2impl.h"
#include "sprpimpl.h"
#include "propname.h"
#include "rbbidata.h"
#include "triedict.h"
#include "utrie2.h"

/* swapping implementations in i18n */

#if !UCONFIG_NO_NORMALIZATION
#include "uspoof_impl.h"
#endif


/* definitions */

#define LENGTHOF(array) (int32_t)(sizeof(array)/sizeof((array)[0]))

/* Unicode properties data swapping ----------------------------------------- */

U_CAPI int32_t U_EXPORT2
uprops_swap(const UDataSwapper *ds,
            const void *inData, int32_t length, void *outData,
            UErrorCode *pErrorCode) {
    const UDataInfo *pInfo;
    int32_t headerSize, i;

    int32_t dataIndexes[UPROPS_INDEX_COUNT];
    const int32_t *inData32;

    /* udata_swapDataHeader checks the arguments */
    headerSize=udata_swapDataHeader(ds, inData, length, outData, pErrorCode);
    if(pErrorCode==NULL || U_FAILURE(*pErrorCode)) {
        return 0;
    }

    /* check data format and format version */
    pInfo=(const UDataInfo *)((const char *)inData+4);
    if(!(
        pInfo->dataFormat[0]==0x55 &&   /* dataFormat="UPro" */
        pInfo->dataFormat[1]==0x50 &&
        pInfo->dataFormat[2]==0x72 &&
        pInfo->dataFormat[3]==0x6f &&
        (3<=pInfo->formatVersion[0] && pInfo->formatVersion[0]<=7) &&
        (pInfo->formatVersion[0]>=7 ||
            (pInfo->formatVersion[2]==UTRIE_SHIFT &&
             pInfo->formatVersion[3]==UTRIE_INDEX_SHIFT))
    )) {
        udata_printError(ds, "uprops_swap(): data format %02x.%02x.%02x.%02x (format version %02x) is not a Unicode properties file\n",
                         pInfo->dataFormat[0], pInfo->dataFormat[1],
                         pInfo->dataFormat[2], pInfo->dataFormat[3],
                         pInfo->formatVersion[0]);
        *pErrorCode=U_UNSUPPORTED_ERROR;
        return 0;
    }

    /* the properties file must contain at least the indexes array */
    if(length>=0 && (length-headerSize)<(int32_t)sizeof(dataIndexes)) {
        udata_printError(ds, "uprops_swap(): too few bytes (%d after header) for a Unicode properties file\n",
                         length-headerSize);
        *pErrorCode=U_INDEX_OUTOFBOUNDS_ERROR;
        return 0;
    }

    /* read the indexes */
    inData32=(const int32_t *)((const char *)inData+headerSize);
    for(i=0; i<UPROPS_INDEX_COUNT; ++i) {
        dataIndexes[i]=udata_readInt32(ds, inData32[i]);
    }

    /*
     * comments are copied from the data format description in genprops/store.c
     * indexes[] constants are in uprops.h
     */
    int32_t dataTop;
    if(length>=0) {
        int32_t *outData32;

        /*
         * In formatVersion 7, UPROPS_DATA_TOP_INDEX has the post-header data size.
         * In earlier formatVersions, it is 0 and a lower dataIndexes entry
         * has the top of the last item.
         */
        for(i=UPROPS_DATA_TOP_INDEX; i>0 && (dataTop=dataIndexes[i])==0; --i) {}

        if((length-headerSize)<(4*dataTop)) {
            udata_printError(ds, "uprops_swap(): too few bytes (%d after header) for a Unicode properties file\n",
                             length-headerSize);
            *pErrorCode=U_INDEX_OUTOFBOUNDS_ERROR;
            return 0;
        }

        outData32=(int32_t *)((char *)outData+headerSize);

        /* copy everything for inaccessible data (padding) */
        if(inData32!=outData32) {
            uprv_memcpy(outData32, inData32, 4*dataTop);
        }

        /* swap the indexes[16] */
        ds->swapArray32(ds, inData32, 4*UPROPS_INDEX_COUNT, outData32, pErrorCode);

        /*
         * swap the main properties UTrie
         * PT serialized properties trie, see utrie.h (byte size: 4*(i0-16))
         */
        utrie2_swapAnyVersion(ds,
            inData32+UPROPS_INDEX_COUNT,
            4*(dataIndexes[UPROPS_PROPS32_INDEX]-UPROPS_INDEX_COUNT),
            outData32+UPROPS_INDEX_COUNT,
            pErrorCode);

        /*
         * swap the properties and exceptions words
         * P  const uint32_t props32[i1-i0];
         * E  const uint32_t exceptions[i2-i1];
         */
        ds->swapArray32(ds,
            inData32+dataIndexes[UPROPS_PROPS32_INDEX],
            4*(dataIndexes[UPROPS_EXCEPTIONS_TOP_INDEX]-dataIndexes[UPROPS_PROPS32_INDEX]),
            outData32+dataIndexes[UPROPS_PROPS32_INDEX],
            pErrorCode);

        /*
         * swap the UChars
         * U  const UChar uchars[2*(i3-i2)];
         */
        ds->swapArray16(ds,
            inData32+dataIndexes[UPROPS_EXCEPTIONS_TOP_INDEX],
            4*(dataIndexes[UPROPS_ADDITIONAL_TRIE_INDEX]-dataIndexes[UPROPS_EXCEPTIONS_TOP_INDEX]),
            outData32+dataIndexes[UPROPS_EXCEPTIONS_TOP_INDEX],
            pErrorCode);

        /*
         * swap the additional UTrie
         * i3 additionalTrieIndex; -- 32-bit unit index to the additional trie for more properties
         */
        utrie2_swapAnyVersion(ds,
            inData32+dataIndexes[UPROPS_ADDITIONAL_TRIE_INDEX],
            4*(dataIndexes[UPROPS_ADDITIONAL_VECTORS_INDEX]-dataIndexes[UPROPS_ADDITIONAL_TRIE_INDEX]),
            outData32+dataIndexes[UPROPS_ADDITIONAL_TRIE_INDEX],
            pErrorCode);

        /*
         * swap the properties vectors
         * PV const uint32_t propsVectors[(i6-i4)/i5][i5]==uint32_t propsVectors[i6-i4];
         */
        ds->swapArray32(ds,
            inData32+dataIndexes[UPROPS_ADDITIONAL_VECTORS_INDEX],
            4*(dataIndexes[UPROPS_SCRIPT_EXTENSIONS_INDEX]-dataIndexes[UPROPS_ADDITIONAL_VECTORS_INDEX]),
            outData32+dataIndexes[UPROPS_ADDITIONAL_VECTORS_INDEX],
            pErrorCode);

        // swap the Script_Extensions data
        // SCX const uint16_t scriptExtensions[2*(i7-i6)];
        ds->swapArray16(ds,
            inData32+dataIndexes[UPROPS_SCRIPT_EXTENSIONS_INDEX],
            4*(dataIndexes[UPROPS_RESERVED_INDEX_7]-dataIndexes[UPROPS_SCRIPT_EXTENSIONS_INDEX]),
            outData32+dataIndexes[UPROPS_SCRIPT_EXTENSIONS_INDEX],
            pErrorCode);
    }

    /* i7 reservedIndex7; -- 32-bit unit index to the top of the Script_Extensions data */
    return headerSize+4*dataIndexes[UPROPS_RESERVED_INDEX_7];
}

/* Unicode case mapping data swapping --------------------------------------- */

U_CAPI int32_t U_EXPORT2
ucase_swap(const UDataSwapper *ds,
           const void *inData, int32_t length, void *outData,
           UErrorCode *pErrorCode) {
    const UDataInfo *pInfo;
    int32_t headerSize;

    const uint8_t *inBytes;
    uint8_t *outBytes;

    const int32_t *inIndexes;
    int32_t indexes[16];

    int32_t i, offset, count, size;

    /* udata_swapDataHeader checks the arguments */
    headerSize=udata_swapDataHeader(ds, inData, length, outData, pErrorCode);
    if(pErrorCode==NULL || U_FAILURE(*pErrorCode)) {
        return 0;
    }

    /* check data format and format version */
    pInfo=(const UDataInfo *)((const char *)inData+4);
    if(!(
        pInfo->dataFormat[0]==UCASE_FMT_0 &&    /* dataFormat="cAsE" */
        pInfo->dataFormat[1]==UCASE_FMT_1 &&
        pInfo->dataFormat[2]==UCASE_FMT_2 &&
        pInfo->dataFormat[3]==UCASE_FMT_3 &&
        ((pInfo->formatVersion[0]==1 &&
          pInfo->formatVersion[2]==UTRIE_SHIFT &&
          pInfo->formatVersion[3]==UTRIE_INDEX_SHIFT) ||
         pInfo->formatVersion[0]==2)
    )) {
        udata_printError(ds, "ucase_swap(): data format %02x.%02x.%02x.%02x (format version %02x) is not recognized as case mapping data\n",
                         pInfo->dataFormat[0], pInfo->dataFormat[1],
                         pInfo->dataFormat[2], pInfo->dataFormat[3],
                         pInfo->formatVersion[0]);
        *pErrorCode=U_UNSUPPORTED_ERROR;
        return 0;
    }

    inBytes=(const uint8_t *)inData+headerSize;
    outBytes=(uint8_t *)outData+headerSize;

    inIndexes=(const int32_t *)inBytes;

    if(length>=0) {
        length-=headerSize;
        if(length<16*4) {
            udata_printError(ds, "ucase_swap(): too few bytes (%d after header) for case mapping data\n",
                             length);
            *pErrorCode=U_INDEX_OUTOFBOUNDS_ERROR;
            return 0;
        }
    }

    /* read the first 16 indexes (ICU 3.2/format version 1: UCASE_IX_TOP==16, might grow) */
    for(i=0; i<16; ++i) {
        indexes[i]=udata_readInt32(ds, inIndexes[i]);
    }

    /* get the total length of the data */
    size=indexes[UCASE_IX_LENGTH];

    if(length>=0) {
        if(length<size) {
            udata_printError(ds, "ucase_swap(): too few bytes (%d after header) for all of case mapping data\n",
                             length);
            *pErrorCode=U_INDEX_OUTOFBOUNDS_ERROR;
            return 0;
        }

        /* copy the data for inaccessible bytes */
        if(inBytes!=outBytes) {
            uprv_memcpy(outBytes, inBytes, size);
        }

        offset=0;

        /* swap the int32_t indexes[] */
        count=indexes[UCASE_IX_INDEX_TOP]*4;
        ds->swapArray32(ds, inBytes, count, outBytes, pErrorCode);
        offset+=count;

        /* swap the UTrie */
        count=indexes[UCASE_IX_TRIE_SIZE];
        utrie2_swapAnyVersion(ds, inBytes+offset, count, outBytes+offset, pErrorCode);
        offset+=count;

        /* swap the uint16_t exceptions[] and unfold[] */
        count=(indexes[UCASE_IX_EXC_LENGTH]+indexes[UCASE_IX_UNFOLD_LENGTH])*2;
        ds->swapArray16(ds, inBytes+offset, count, outBytes+offset, pErrorCode);
        offset+=count;

        U_ASSERT(offset==size);
    }

    return headerSize+size;
}

/* Unicode bidi/shaping data swapping --------------------------------------- */

U_CAPI int32_t U_EXPORT2
ubidi_swap(const UDataSwapper *ds,
           const void *inData, int32_t length, void *outData,
           UErrorCode *pErrorCode) {
    const UDataInfo *pInfo;
    int32_t headerSize;

    const uint8_t *inBytes;
    uint8_t *outBytes;

    const int32_t *inIndexes;
    int32_t indexes[16];

    int32_t i, offset, count, size;

    /* udata_swapDataHeader checks the arguments */
    headerSize=udata_swapDataHeader(ds, inData, length, outData, pErrorCode);
    if(pErrorCode==NULL || U_FAILURE(*pErrorCode)) {
        return 0;
    }

    /* check data format and format version */
    pInfo=(const UDataInfo *)((const char *)inData+4);
    if(!(
        pInfo->dataFormat[0]==UBIDI_FMT_0 &&    /* dataFormat="BiDi" */
        pInfo->dataFormat[1]==UBIDI_FMT_1 &&
        pInfo->dataFormat[2]==UBIDI_FMT_2 &&
        pInfo->dataFormat[3]==UBIDI_FMT_3 &&
        ((pInfo->formatVersion[0]==1 &&
          pInfo->formatVersion[2]==UTRIE_SHIFT &&
          pInfo->formatVersion[3]==UTRIE_INDEX_SHIFT) ||
         pInfo->formatVersion[0]==2)
    )) {
        udata_printError(ds, "ubidi_swap(): data format %02x.%02x.%02x.%02x (format version %02x) is not recognized as bidi/shaping data\n",
                         pInfo->dataFormat[0], pInfo->dataFormat[1],
                         pInfo->dataFormat[2], pInfo->dataFormat[3],
                         pInfo->formatVersion[0]);
        *pErrorCode=U_UNSUPPORTED_ERROR;
        return 0;
    }

    inBytes=(const uint8_t *)inData+headerSize;
    outBytes=(uint8_t *)outData+headerSize;

    inIndexes=(const int32_t *)inBytes;

    if(length>=0) {
        length-=headerSize;
        if(length<16*4) {
            udata_printError(ds, "ubidi_swap(): too few bytes (%d after header) for bidi/shaping data\n",
                             length);
            *pErrorCode=U_INDEX_OUTOFBOUNDS_ERROR;
            return 0;
        }
    }

    /* read the first 16 indexes (ICU 3.4/format version 1: UBIDI_IX_TOP==16, might grow) */
    for(i=0; i<16; ++i) {
        indexes[i]=udata_readInt32(ds, inIndexes[i]);
    }

    /* get the total length of the data */
    size=indexes[UBIDI_IX_LENGTH];

    if(length>=0) {
        if(length<size) {
            udata_printError(ds, "ubidi_swap(): too few bytes (%d after header) for all of bidi/shaping data\n",
                             length);
            *pErrorCode=U_INDEX_OUTOFBOUNDS_ERROR;
            return 0;
        }

        /* copy the data for inaccessible bytes */
        if(inBytes!=outBytes) {
            uprv_memcpy(outBytes, inBytes, size);
        }

        offset=0;

        /* swap the int32_t indexes[] */
        count=indexes[UBIDI_IX_INDEX_TOP]*4;
        ds->swapArray32(ds, inBytes, count, outBytes, pErrorCode);
        offset+=count;

        /* swap the UTrie */
        count=indexes[UBIDI_IX_TRIE_SIZE];
        utrie2_swapAnyVersion(ds, inBytes+offset, count, outBytes+offset, pErrorCode);
        offset+=count;

        /* swap the uint32_t mirrors[] */
        count=indexes[UBIDI_IX_MIRROR_LENGTH]*4;
        ds->swapArray32(ds, inBytes+offset, count, outBytes+offset, pErrorCode);
        offset+=count;

        /* just skip the uint8_t jgArray[] */
        count=indexes[UBIDI_IX_JG_LIMIT]-indexes[UBIDI_IX_JG_START];
        offset+=count;

        U_ASSERT(offset==size);
    }

    return headerSize+size;
}

/* Unicode normalization data swapping -------------------------------------- */

#if !UCONFIG_NO_NORMALIZATION

U_CAPI int32_t U_EXPORT2
unorm_swap(const UDataSwapper *ds,
           const void *inData, int32_t length, void *outData,
           UErrorCode *pErrorCode) {
    const UDataInfo *pInfo;
    int32_t headerSize;

    const uint8_t *inBytes;
    uint8_t *outBytes;

    const int32_t *inIndexes;
    int32_t indexes[32];

    int32_t i, offset, count, size;

    /* udata_swapDataHeader checks the arguments */
    headerSize=udata_swapDataHeader(ds, inData, length, outData, pErrorCode);
    if(pErrorCode==NULL || U_FAILURE(*pErrorCode)) {
        return 0;
    }

    /* check data format and format version */
    pInfo=(const UDataInfo *)((const char *)inData+4);
    if(!(
        pInfo->dataFormat[0]==0x4e &&   /* dataFormat="Norm" */
        pInfo->dataFormat[1]==0x6f &&
        pInfo->dataFormat[2]==0x72 &&
        pInfo->dataFormat[3]==0x6d &&
        pInfo->formatVersion[0]==2
    )) {
        udata_printError(ds, "unorm_swap(): data format %02x.%02x.%02x.%02x (format version %02x) is not recognized as unorm.icu\n",
                         pInfo->dataFormat[0], pInfo->dataFormat[1],
                         pInfo->dataFormat[2], pInfo->dataFormat[3],
                         pInfo->formatVersion[0]);
        *pErrorCode=U_UNSUPPORTED_ERROR;
        return 0;
    }

    inBytes=(const uint8_t *)inData+headerSize;
    outBytes=(uint8_t *)outData+headerSize;

    inIndexes=(const int32_t *)inBytes;

    if(length>=0) {
        length-=headerSize;
        if(length<32*4) {
            udata_printError(ds, "unorm_swap(): too few bytes (%d after header) for unorm.icu\n",
                             length);
            *pErrorCode=U_INDEX_OUTOFBOUNDS_ERROR;
            return 0;
        }
    }

    /* read the first 32 indexes (ICU 2.8/format version 2.2: _NORM_INDEX_TOP==32, might grow) */
    for(i=0; i<32; ++i) {
        indexes[i]=udata_readInt32(ds, inIndexes[i]);
    }

    /* calculate the total length of the data */
    size=
        32*4+ /* size of indexes[] */
        indexes[_NORM_INDEX_TRIE_SIZE]+
        indexes[_NORM_INDEX_UCHAR_COUNT]*2+
        indexes[_NORM_INDEX_COMBINE_DATA_COUNT]*2+
        indexes[_NORM_INDEX_FCD_TRIE_SIZE]+
        indexes[_NORM_INDEX_AUX_TRIE_SIZE]+
        indexes[_NORM_INDEX_CANON_SET_COUNT]*2;

    if(length>=0) {
        if(length<size) {
            udata_printError(ds, "unorm_swap(): too few bytes (%d after header) for all of unorm.icu\n",
                             length);
            *pErrorCode=U_INDEX_OUTOFBOUNDS_ERROR;
            return 0;
        }

        /* copy the data for inaccessible bytes */
        if(inBytes!=outBytes) {
            uprv_memcpy(outBytes, inBytes, size);
        }

        offset=0;

        /* swap the indexes[] */
        count=32*4;
        ds->swapArray32(ds, inBytes, count, outBytes, pErrorCode);
        offset+=count;

        /* swap the main UTrie */
        count=indexes[_NORM_INDEX_TRIE_SIZE];
        utrie_swap(ds, inBytes+offset, count, outBytes+offset, pErrorCode);
        offset+=count;

        /* swap the uint16_t extraData[] and the uint16_t combiningTable[] */
        count=(indexes[_NORM_INDEX_UCHAR_COUNT]+indexes[_NORM_INDEX_COMBINE_DATA_COUNT])*2;
        ds->swapArray16(ds, inBytes+offset, count, outBytes+offset, pErrorCode);
        offset+=count;

        /* swap the FCD UTrie */
        count=indexes[_NORM_INDEX_FCD_TRIE_SIZE];
        if(count!=0) {
            utrie_swap(ds, inBytes+offset, count, outBytes+offset, pErrorCode);
            offset+=count;
        }

        /* swap the aux UTrie */
        count=indexes[_NORM_INDEX_AUX_TRIE_SIZE];
        if(count!=0) {
            utrie_swap(ds, inBytes+offset, count, outBytes+offset, pErrorCode);
            offset+=count;
        }

        /* swap the uint16_t combiningTable[] */
        count=indexes[_NORM_INDEX_CANON_SET_COUNT]*2;
        ds->swapArray16(ds, inBytes+offset, count, outBytes+offset, pErrorCode);
        offset+=count;
    }

    return headerSize+size;
}

#endif

/* Swap 'Test' data from gentest */
U_CAPI int32_t U_EXPORT2
test_swap(const UDataSwapper *ds,
           const void *inData, int32_t length, void *outData,
           UErrorCode *pErrorCode) {
    const UDataInfo *pInfo;
    int32_t headerSize;

    const uint8_t *inBytes;
    uint8_t *outBytes;

    int32_t offset;

    /* udata_swapDataHeader checks the arguments */
    headerSize=udata_swapDataHeader(ds, inData, length, outData, pErrorCode);
    if(pErrorCode==NULL || U_FAILURE(*pErrorCode)) {
        udata_printError(ds, "test_swap(): data header swap failed %s\n", u_errorName(*pErrorCode));
        return 0;
    }

    /* check data format and format version */
    pInfo=(const UDataInfo *)((const char *)inData+4);
    if(!(
        pInfo->dataFormat[0]==0x54 &&   /* dataFormat="Norm" */
        pInfo->dataFormat[1]==0x65 &&
        pInfo->dataFormat[2]==0x73 &&
        pInfo->dataFormat[3]==0x74 &&
        pInfo->formatVersion[0]==1
    )) {
        udata_printError(ds, "test_swap(): data format %02x.%02x.%02x.%02x (format version %02x) is not recognized as testdata\n",
                         pInfo->dataFormat[0], pInfo->dataFormat[1],
                         pInfo->dataFormat[2], pInfo->dataFormat[3],
                         pInfo->formatVersion[0]);
        *pErrorCode=U_UNSUPPORTED_ERROR;
        return 0;
    }

    inBytes=(const uint8_t *)inData+headerSize;
    outBytes=(uint8_t *)outData+headerSize;

    int32_t size16 = 2; // 16bit plus padding
    int32_t sizeStr = 5; // 4 char inv-str plus null
    int32_t size = size16 + sizeStr;

    if(length>=0) {
        if(length<size) {
            udata_printError(ds, "test_swap(): too few bytes (%d after header, wanted %d) for all of testdata\n",
                             length, size);
            *pErrorCode=U_INDEX_OUTOFBOUNDS_ERROR;
            return 0;
        }

	offset =0;
	/* swap a 1 entry array */
        ds->swapArray16(ds, inBytes+offset, size16, outBytes+offset, pErrorCode);
	offset+=size16;
	ds->swapInvChars(ds, inBytes+offset, sizeStr, outBytes+offset, pErrorCode);
    }

    return headerSize+size;
}

/* swap any data (except a .dat package) ------------------------------------ */

static const struct {
    uint8_t dataFormat[4];
    UDataSwapFn *swapFn;
} swapFns[]={
    { { 0x52, 0x65, 0x73, 0x42 }, ures_swap },          /* dataFormat="ResB" */
#if !UCONFIG_NO_LEGACY_CONVERSION
    { { 0x63, 0x6e, 0x76, 0x74 }, ucnv_swap },          /* dataFormat="cnvt" */
#endif
#if !UCONFIG_NO_CONVERSION
    { { 0x43, 0x76, 0x41, 0x6c }, ucnv_swapAliases },   /* dataFormat="CvAl" */
#endif
#if !UCONFIG_NO_IDNA
    { { 0x53, 0x50, 0x52, 0x50 }, usprep_swap },        /* dataFormat="SPRP" */
#endif
    /* insert data formats here, descending by expected frequency of occurrence */
    { { 0x55, 0x50, 0x72, 0x6f }, uprops_swap },        /* dataFormat="UPro" */

    { { UCASE_FMT_0, UCASE_FMT_1, UCASE_FMT_2, UCASE_FMT_3 },
                                  ucase_swap },         /* dataFormat="cAsE" */

    { { UBIDI_FMT_0, UBIDI_FMT_1, UBIDI_FMT_2, UBIDI_FMT_3 },
                                  ubidi_swap },         /* dataFormat="BiDi" */

#if !UCONFIG_NO_NORMALIZATION
    { { 0x4e, 0x6f, 0x72, 0x6d }, unorm_swap },         /* dataFormat="Norm" */
    { { 0x4e, 0x72, 0x6d, 0x32 }, unorm2_swap },        /* dataFormat="Nrm2" */
#endif
#if !UCONFIG_NO_COLLATION
    { { 0x55, 0x43, 0x6f, 0x6c }, ucol_swap },          /* dataFormat="UCol" */
    { { 0x49, 0x6e, 0x76, 0x43 }, ucol_swapInverseUCA },/* dataFormat="InvC" */
#endif
#if !UCONFIG_NO_BREAK_ITERATION
    { { 0x42, 0x72, 0x6b, 0x20 }, ubrk_swap },          /* dataFormat="Brk " */
    { { 0x54, 0x72, 0x44, 0x63 }, triedict_swap },      /* dataFormat="TrDc " */
#endif
    { { 0x70, 0x6e, 0x61, 0x6d }, upname_swap },        /* dataFormat="pnam" */
    { { 0x75, 0x6e, 0x61, 0x6d }, uchar_swapNames },    /* dataFormat="unam" */
#if !UCONFIG_NO_NORMALIZATION
    { { 0x43, 0x66, 0x75, 0x20 }, uspoof_swap },         /* dataFormat="Cfu " */
#endif
    { { 0x54, 0x65, 0x73, 0x74 }, test_swap }            /* dataFormat="Test" */
};

U_CAPI int32_t U_EXPORT2
udata_swap(const UDataSwapper *ds,
           const void *inData, int32_t length, void *outData,
           UErrorCode *pErrorCode) {
    char dataFormatChars[4];
    const UDataInfo *pInfo;
    int32_t headerSize, i, swappedLength;

    if(pErrorCode==NULL || U_FAILURE(*pErrorCode)) {
        return 0;
    }

    /*
     * Preflight the header first; checks for illegal arguments, too.
     * Do not swap the header right away because the format-specific swapper
     * will swap it, get the headerSize again, and also use the header
     * information. Otherwise we would have to pass some of the information
     * and not be able to use the UDataSwapFn signature.
     */
    headerSize=udata_swapDataHeader(ds, inData, -1, NULL, pErrorCode);

    /*
     * If we wanted udata_swap() to also handle non-loadable data like a UTrie,
     * then we could check here for further known magic values and structures.
     */
    if(U_FAILURE(*pErrorCode)) {
        return 0; /* the data format was not recognized */
    }

    pInfo=(const UDataInfo *)((const char *)inData+4);

    {
        /* convert the data format from ASCII to Unicode to the system charset */
        UChar u[4]={
             pInfo->dataFormat[0], pInfo->dataFormat[1],
             pInfo->dataFormat[2], pInfo->dataFormat[3]
        };

        if(uprv_isInvariantUString(u, 4)) {
            u_UCharsToChars(u, dataFormatChars, 4);
        } else {
            dataFormatChars[0]=dataFormatChars[1]=dataFormatChars[2]=dataFormatChars[3]='?';
        }
    }

    /* dispatch to the swap function for the dataFormat */
    for(i=0; i<LENGTHOF(swapFns); ++i) {
        if(0==memcmp(swapFns[i].dataFormat, pInfo->dataFormat, 4)) {
            swappedLength=swapFns[i].swapFn(ds, inData, length, outData, pErrorCode);

            if(U_FAILURE(*pErrorCode)) {
                udata_printError(ds, "udata_swap(): failure swapping data format %02x.%02x.%02x.%02x (\"%c%c%c%c\") - %s\n",
                                 pInfo->dataFormat[0], pInfo->dataFormat[1],
                                 pInfo->dataFormat[2], pInfo->dataFormat[3],
                                 dataFormatChars[0], dataFormatChars[1],
                                 dataFormatChars[2], dataFormatChars[3],
                                 u_errorName(*pErrorCode));
            } else if(swappedLength<(length-15)) {
                /* swapped less than expected */
                udata_printError(ds, "udata_swap() warning: swapped only %d out of %d bytes - data format %02x.%02x.%02x.%02x (\"%c%c%c%c\")\n",
                                 swappedLength, length,
                                 pInfo->dataFormat[0], pInfo->dataFormat[1],
                                 pInfo->dataFormat[2], pInfo->dataFormat[3],
                                 dataFormatChars[0], dataFormatChars[1],
                                 dataFormatChars[2], dataFormatChars[3],
                                 u_errorName(*pErrorCode));
            }

            return swappedLength;
        }
    }

    /* the dataFormat was not recognized */
    udata_printError(ds, "udata_swap(): unknown data format %02x.%02x.%02x.%02x (\"%c%c%c%c\")\n",
                     pInfo->dataFormat[0], pInfo->dataFormat[1],
                     pInfo->dataFormat[2], pInfo->dataFormat[3],
                     dataFormatChars[0], dataFormatChars[1],
                     dataFormatChars[2], dataFormatChars[3]);

    *pErrorCode=U_UNSUPPORTED_ERROR;
    return 0;
}
