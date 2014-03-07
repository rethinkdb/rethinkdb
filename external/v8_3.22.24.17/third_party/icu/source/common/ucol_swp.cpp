/*
*******************************************************************************
*
*   Copyright (C) 2003-2010, International Business Machines
*   Corporation and others.  All Rights Reserved.
*
*******************************************************************************
*   file name:  ucol_swp.c
*   encoding:   US-ASCII
*   tab size:   8 (not used)
*   indentation:4
*
*   created on: 2003sep10
*   created by: Markus W. Scherer
*
*   Swap collation binaries.
*/

#include "unicode/udata.h" /* UDataInfo */
#include "utrie.h"
#include "udataswp.h"
#include "cmemory.h"
#include "ucol_imp.h"
#include "ucol_swp.h"

/* swapping ----------------------------------------------------------------- */

/*
 * This performs data swapping for a folded trie (see utrie.c for details).
 */

U_CAPI int32_t U_EXPORT2
utrie_swap(const UDataSwapper *ds,
           const void *inData, int32_t length, void *outData,
           UErrorCode *pErrorCode) {
    const UTrieHeader *inTrie;
    UTrieHeader trie;
    int32_t size;
    UBool dataIs32;

    if(pErrorCode==NULL || U_FAILURE(*pErrorCode)) {
        return 0;
    }
    if(ds==NULL || inData==NULL || (length>=0 && outData==NULL)) {
        *pErrorCode=U_ILLEGAL_ARGUMENT_ERROR;
        return 0;
    }

    /* setup and swapping */
    if(length>=0 && (uint32_t)length<sizeof(UTrieHeader)) {
        *pErrorCode=U_INDEX_OUTOFBOUNDS_ERROR;
        return 0;
    }

    inTrie=(const UTrieHeader *)inData;
    trie.signature=ds->readUInt32(inTrie->signature);
    trie.options=ds->readUInt32(inTrie->options);
    trie.indexLength=udata_readInt32(ds, inTrie->indexLength);
    trie.dataLength=udata_readInt32(ds, inTrie->dataLength);

    if( trie.signature!=0x54726965 ||
        (trie.options&UTRIE_OPTIONS_SHIFT_MASK)!=UTRIE_SHIFT ||
        ((trie.options>>UTRIE_OPTIONS_INDEX_SHIFT)&UTRIE_OPTIONS_SHIFT_MASK)!=UTRIE_INDEX_SHIFT ||
        trie.indexLength<UTRIE_BMP_INDEX_LENGTH ||
        (trie.indexLength&(UTRIE_SURROGATE_BLOCK_COUNT-1))!=0 ||
        trie.dataLength<UTRIE_DATA_BLOCK_LENGTH ||
        (trie.dataLength&(UTRIE_DATA_GRANULARITY-1))!=0 ||
        ((trie.options&UTRIE_OPTIONS_LATIN1_IS_LINEAR)!=0 && trie.dataLength<(UTRIE_DATA_BLOCK_LENGTH+0x100))
    ) {
        *pErrorCode=U_INVALID_FORMAT_ERROR; /* not a UTrie */
        return 0;
    }

    dataIs32=(UBool)((trie.options&UTRIE_OPTIONS_DATA_IS_32_BIT)!=0);
    size=sizeof(UTrieHeader)+trie.indexLength*2+trie.dataLength*(dataIs32?4:2);

    if(length>=0) {
        UTrieHeader *outTrie;

        if(length<size) {
            *pErrorCode=U_INDEX_OUTOFBOUNDS_ERROR;
            return 0;
        }

        outTrie=(UTrieHeader *)outData;

        /* swap the header */
        ds->swapArray32(ds, inTrie, sizeof(UTrieHeader), outTrie, pErrorCode);

        /* swap the index and the data */
        if(dataIs32) {
            ds->swapArray16(ds, inTrie+1, trie.indexLength*2, outTrie+1, pErrorCode);
            ds->swapArray32(ds, (const uint16_t *)(inTrie+1)+trie.indexLength, trie.dataLength*4,
                                     (uint16_t *)(outTrie+1)+trie.indexLength, pErrorCode);
        } else {
            ds->swapArray16(ds, inTrie+1, (trie.indexLength+trie.dataLength)*2, outTrie+1, pErrorCode);
        }
    }

    return size;
}

#if !UCONFIG_NO_COLLATION

/* Modified copy of the beginning of ucol_swapBinary(). */
U_CAPI UBool U_EXPORT2
ucol_looksLikeCollationBinary(const UDataSwapper *ds,
                              const void *inData, int32_t length) {
    const uint8_t *inBytes;
    const UCATableHeader *inHeader;
    UCATableHeader header;

    if(ds==NULL || inData==NULL || length<-1) {
        return FALSE;
    }

    inBytes=(const uint8_t *)inData;
    inHeader=(const UCATableHeader *)inData;

    /*
     * The collation binary must contain at least the UCATableHeader,
     * starting with its size field.
     * sizeof(UCATableHeader)==42*4 in ICU 2.8
     * check the length against the header size before reading the size field
     */
    uprv_memset(&header, 0, sizeof(header));
    if(length<0) {
        header.size=udata_readInt32(ds, inHeader->size);
    } else if((length<(42*4) || length<(header.size=udata_readInt32(ds, inHeader->size)))) {
        return FALSE;
    }

    header.magic=ds->readUInt32(inHeader->magic);
    if(!(
        header.magic==UCOL_HEADER_MAGIC &&
        inHeader->formatVersion[0]==3 /*&&
        inHeader->formatVersion[1]>=0*/
    )) {
        return FALSE;
    }

    if(inHeader->isBigEndian!=ds->inIsBigEndian || inHeader->charSetFamily!=ds->inCharset) {
        return FALSE;
    }

    return TRUE;
}

/* swap a header-less collation binary, inside a resource bundle or ucadata.icu */
U_CAPI int32_t U_EXPORT2
ucol_swapBinary(const UDataSwapper *ds,
                const void *inData, int32_t length, void *outData,
                UErrorCode *pErrorCode) {
    const uint8_t *inBytes;
    uint8_t *outBytes;

    const UCATableHeader *inHeader;
    UCATableHeader *outHeader;
    UCATableHeader header;

    uint32_t count;

    /* argument checking in case we were not called from ucol_swap() */
    if(pErrorCode==NULL || U_FAILURE(*pErrorCode)) {
        return 0;
    }
    if(ds==NULL || inData==NULL || length<-1 || (length>0 && outData==NULL)) {
        *pErrorCode=U_ILLEGAL_ARGUMENT_ERROR;
        return 0;
    }

    inBytes=(const uint8_t *)inData;
    outBytes=(uint8_t *)outData;

    inHeader=(const UCATableHeader *)inData;
    outHeader=(UCATableHeader *)outData;

    /*
     * The collation binary must contain at least the UCATableHeader,
     * starting with its size field.
     * sizeof(UCATableHeader)==42*4 in ICU 2.8
     * check the length against the header size before reading the size field
     */
    uprv_memset(&header, 0, sizeof(header));
    if(length<0) {
        header.size=udata_readInt32(ds, inHeader->size);
    } else if((length<(42*4) || length<(header.size=udata_readInt32(ds, inHeader->size)))) {
        udata_printError(ds, "ucol_swapBinary(): too few bytes (%d after header) for collation data\n",
                         length);
        *pErrorCode=U_INDEX_OUTOFBOUNDS_ERROR;
        return 0;
    }

    header.magic=ds->readUInt32(inHeader->magic);
    if(!(
        header.magic==UCOL_HEADER_MAGIC &&
        inHeader->formatVersion[0]==3 /*&&
        inHeader->formatVersion[1]>=0*/
    )) {
        udata_printError(ds, "ucol_swapBinary(): magic 0x%08x or format version %02x.%02x is not a collation binary\n",
                         header.magic,
                         inHeader->formatVersion[0], inHeader->formatVersion[1]);
        *pErrorCode=U_UNSUPPORTED_ERROR;
        return 0;
    }

    if(inHeader->isBigEndian!=ds->inIsBigEndian || inHeader->charSetFamily!=ds->inCharset) {
        udata_printError(ds, "ucol_swapBinary(): endianness %d or charset %d does not match the swapper\n",
                         inHeader->isBigEndian, inHeader->charSetFamily);
        *pErrorCode=U_INVALID_FORMAT_ERROR;
        return 0;
    }

    if(length>=0) {
        /* copy everything, takes care of data that needs no swapping */
        if(inBytes!=outBytes) {
            uprv_memcpy(outBytes, inBytes, header.size);
        }

        /* swap the necessary pieces in the order of their occurrence in the data */

        /* read more of the UCATableHeader (the size field was read above) */
        header.options=                 ds->readUInt32(inHeader->options);
        header.UCAConsts=               ds->readUInt32(inHeader->UCAConsts);
        header.contractionUCACombos=    ds->readUInt32(inHeader->contractionUCACombos);
        header.mappingPosition=         ds->readUInt32(inHeader->mappingPosition);
        header.expansion=               ds->readUInt32(inHeader->expansion);
        header.contractionIndex=        ds->readUInt32(inHeader->contractionIndex);
        header.contractionCEs=          ds->readUInt32(inHeader->contractionCEs);
        header.contractionSize=         ds->readUInt32(inHeader->contractionSize);
        header.endExpansionCE=          ds->readUInt32(inHeader->endExpansionCE);
        header.expansionCESize=         ds->readUInt32(inHeader->expansionCESize);
        header.endExpansionCECount=     udata_readInt32(ds, inHeader->endExpansionCECount);
        header.contractionUCACombosSize=udata_readInt32(ds, inHeader->contractionUCACombosSize);
        header.scriptToLeadByte=        ds->readUInt32(inHeader->scriptToLeadByte);
        header.leadByteToScript=        ds->readUInt32(inHeader->leadByteToScript);
        
        /* swap the 32-bit integers in the header */
        ds->swapArray32(ds, inHeader, (int32_t)((const char *)&inHeader->jamoSpecial-(const char *)inHeader),
                           outHeader, pErrorCode);
        ds->swapArray32(ds, &(inHeader->scriptToLeadByte), sizeof(header.scriptToLeadByte) + sizeof(header.leadByteToScript),
                           &(outHeader->scriptToLeadByte), pErrorCode);
        /* set the output platform properties */
        outHeader->isBigEndian=ds->outIsBigEndian;
        outHeader->charSetFamily=ds->outCharset;

        /* swap the options */
        if(header.options!=0) {
            ds->swapArray32(ds, inBytes+header.options, header.expansion-header.options,
                               outBytes+header.options, pErrorCode);
        }

        /* swap the expansions */
        if(header.mappingPosition!=0 && header.expansion!=0) {
            if(header.contractionIndex!=0) {
                /* expansions bounded by contractions */
                count=header.contractionIndex-header.expansion;
            } else {
                /* no contractions: expansions bounded by the main trie */
                count=header.mappingPosition-header.expansion;
            }
            ds->swapArray32(ds, inBytes+header.expansion, (int32_t)count,
                               outBytes+header.expansion, pErrorCode);
        }

        /* swap the contractions */
        if(header.contractionSize!=0) {
            /* contractionIndex: UChar[] */
            ds->swapArray16(ds, inBytes+header.contractionIndex, header.contractionSize*2,
                               outBytes+header.contractionIndex, pErrorCode);

            /* contractionCEs: CEs[] */
            ds->swapArray32(ds, inBytes+header.contractionCEs, header.contractionSize*4,
                               outBytes+header.contractionCEs, pErrorCode);
        }

        /* swap the main trie */
        if(header.mappingPosition!=0) {
            count=header.endExpansionCE-header.mappingPosition;
            utrie_swap(ds, inBytes+header.mappingPosition, (int32_t)count,
                          outBytes+header.mappingPosition, pErrorCode);
        }

        /* swap the max expansion table */
        if(header.endExpansionCECount!=0) {
            ds->swapArray32(ds, inBytes+header.endExpansionCE, header.endExpansionCECount*4,
                               outBytes+header.endExpansionCE, pErrorCode);
        }

        /* expansionCESize, unsafeCP, contrEndCP: uint8_t[], no need to swap */

        /* swap UCA constants */
        if(header.UCAConsts!=0) {
            /*
             * if UCAConsts!=0 then contractionUCACombos because we are swapping
             * the UCA data file, and we know that the UCA contains contractions
             */
            count=header.contractionUCACombos-header.UCAConsts;
            ds->swapArray32(ds, inBytes+header.UCAConsts, header.contractionUCACombos-header.UCAConsts,
                               outBytes+header.UCAConsts, pErrorCode);
        }

        /* swap UCA contractions */
        if(header.contractionUCACombosSize!=0) {
            count=header.contractionUCACombosSize*inHeader->contractionUCACombosWidth*U_SIZEOF_UCHAR;
            ds->swapArray16(ds, inBytes+header.contractionUCACombos, (int32_t)count,
                               outBytes+header.contractionUCACombos, pErrorCode);
        }
        
        /* swap the script to lead bytes */
        if(header.scriptToLeadByte!=0) {
            int indexCount = ds->readUInt16(*((uint16_t*)(inBytes+header.scriptToLeadByte))); // each entry = 2 * uint16
            int dataCount = ds->readUInt16(*((uint16_t*)(inBytes+header.scriptToLeadByte + 2))); // each entry = uint16
            ds->swapArray16(ds, inBytes+header.scriptToLeadByte, 
                                4 + (4 * indexCount) + (2 * dataCount),
                                outBytes+header.scriptToLeadByte, pErrorCode);
        }
        
        /* swap the lead byte to scripts */
        if(header.leadByteToScript!=0) {
            int indexCount = ds->readUInt16(*((uint16_t*)(inBytes+header.leadByteToScript))); // each entry = uint16
            int dataCount = ds->readUInt16(*((uint16_t*)(inBytes+header.leadByteToScript + 2))); // each entry = uint16
            ds->swapArray16(ds, inBytes+header.leadByteToScript, 
                                4 + (2 * indexCount) + (2 * dataCount),
                                outBytes+header.leadByteToScript, pErrorCode);
        }
    }

    return header.size;
}

/* swap ICU collation data like ucadata.icu */
U_CAPI int32_t U_EXPORT2
ucol_swap(const UDataSwapper *ds,
          const void *inData, int32_t length, void *outData,
          UErrorCode *pErrorCode) {
          
    const UDataInfo *pInfo;
    int32_t headerSize, collationSize;

    /* udata_swapDataHeader checks the arguments */
    headerSize=udata_swapDataHeader(ds, inData, length, outData, pErrorCode);
    if(pErrorCode==NULL || U_FAILURE(*pErrorCode)) {
        return 0;
    }

    /* check data format and format version */
    pInfo=(const UDataInfo *)((const char *)inData+4);
    if(!(
        pInfo->dataFormat[0]==0x55 &&   /* dataFormat="UCol" */
        pInfo->dataFormat[1]==0x43 &&
        pInfo->dataFormat[2]==0x6f &&
        pInfo->dataFormat[3]==0x6c &&
        pInfo->formatVersion[0]==3 /*&&
        pInfo->formatVersion[1]>=0*/
    )) {
        udata_printError(ds, "ucol_swap(): data format %02x.%02x.%02x.%02x (format version %02x.%02x) is not a collation file\n",
                         pInfo->dataFormat[0], pInfo->dataFormat[1],
                         pInfo->dataFormat[2], pInfo->dataFormat[3],
                         pInfo->formatVersion[0], pInfo->formatVersion[1]);
        *pErrorCode=U_UNSUPPORTED_ERROR;
        return 0;
    }

    collationSize=ucol_swapBinary(ds,
                        (const char *)inData+headerSize,
                        length>=0 ? length-headerSize : -1,
                        (char *)outData+headerSize,
                        pErrorCode);
    if(U_SUCCESS(*pErrorCode)) {
        return headerSize+collationSize;
    } else {
        return 0;
    }
}

/* swap inverse UCA collation data (invuca.icu) */
U_CAPI int32_t U_EXPORT2
ucol_swapInverseUCA(const UDataSwapper *ds,
                    const void *inData, int32_t length, void *outData,
                    UErrorCode *pErrorCode) {
    const UDataInfo *pInfo;
    int32_t headerSize;

    const uint8_t *inBytes;
    uint8_t *outBytes;

    const InverseUCATableHeader *inHeader;
    InverseUCATableHeader *outHeader;
    InverseUCATableHeader header={ 0,0,0,0,0,{0,0,0,0},{0,0,0,0,0,0,0,0} };

    /* udata_swapDataHeader checks the arguments */
    headerSize=udata_swapDataHeader(ds, inData, length, outData, pErrorCode);
    if(pErrorCode==NULL || U_FAILURE(*pErrorCode)) {
        return 0;
    }

    /* check data format and format version */
    pInfo=(const UDataInfo *)((const char *)inData+4);
    if(!(
        pInfo->dataFormat[0]==0x49 &&   /* dataFormat="InvC" */
        pInfo->dataFormat[1]==0x6e &&
        pInfo->dataFormat[2]==0x76 &&
        pInfo->dataFormat[3]==0x43 &&
        pInfo->formatVersion[0]==2 &&
        pInfo->formatVersion[1]>=1
    )) {
        udata_printError(ds, "ucol_swapInverseUCA(): data format %02x.%02x.%02x.%02x (format version %02x.%02x) is not an inverse UCA collation file\n",
                         pInfo->dataFormat[0], pInfo->dataFormat[1],
                         pInfo->dataFormat[2], pInfo->dataFormat[3],
                         pInfo->formatVersion[0], pInfo->formatVersion[1]);
        *pErrorCode=U_UNSUPPORTED_ERROR;
        return 0;
    }

    inBytes=(const uint8_t *)inData+headerSize;
    outBytes=(uint8_t *)outData+headerSize;

    inHeader=(const InverseUCATableHeader *)inBytes;
    outHeader=(InverseUCATableHeader *)outBytes;

    /*
     * The inverse UCA collation binary must contain at least the InverseUCATableHeader,
     * starting with its size field.
     * sizeof(UCATableHeader)==8*4 in ICU 2.8
     * check the length against the header size before reading the size field
     */
    if(length<0) {
        header.byteSize=udata_readInt32(ds, inHeader->byteSize);
    } else if(
        ((length-headerSize)<(8*4) ||
         (uint32_t)(length-headerSize)<(header.byteSize=udata_readInt32(ds, inHeader->byteSize)))
    ) {
        udata_printError(ds, "ucol_swapInverseUCA(): too few bytes (%d after header) for inverse UCA collation data\n",
                         length);
        *pErrorCode=U_INDEX_OUTOFBOUNDS_ERROR;
        return 0;
    }

    if(length>=0) {
        /* copy everything, takes care of data that needs no swapping */
        if(inBytes!=outBytes) {
            uprv_memcpy(outBytes, inBytes, header.byteSize);
        }

        /* swap the necessary pieces in the order of their occurrence in the data */

        /* read more of the InverseUCATableHeader (the byteSize field was read above) */
        header.tableSize=   ds->readUInt32(inHeader->tableSize);
        header.contsSize=   ds->readUInt32(inHeader->contsSize);
        header.table=       ds->readUInt32(inHeader->table);
        header.conts=       ds->readUInt32(inHeader->conts);

        /* swap the 32-bit integers in the header */
        ds->swapArray32(ds, inHeader, 5*4, outHeader, pErrorCode);

        /* swap the inverse table; tableSize counts uint32_t[3] rows */
        ds->swapArray32(ds, inBytes+header.table, header.tableSize*3*4,
                           outBytes+header.table, pErrorCode);

        /* swap the continuation table; contsSize counts UChars */
        ds->swapArray16(ds, inBytes+header.conts, header.contsSize*U_SIZEOF_UCHAR,
                           outBytes+header.conts, pErrorCode);
    }

    return headerSize+header.byteSize;
}

#endif /* #if !UCONFIG_NO_COLLATION */
