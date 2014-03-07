/*
***************************************************************************
*   Copyright (C) 1999-2010 International Business Machines Corporation   *
*   and others. All rights reserved.                                      *
***************************************************************************
*/

#include "unicode/utypes.h"

#if !UCONFIG_NO_BREAK_ITERATION

#include "unicode/utypes.h"
#include "rbbidata.h"
#include "rbbirb.h"
#include "utrie.h"
#include "udatamem.h"
#include "cmemory.h"
#include "cstring.h"
#include "umutex.h"

#include "uassert.h"


//-----------------------------------------------------------------------------------
//
//   Trie access folding function.  Copied as-is from properties code in uchar.c
//
//-----------------------------------------------------------------------------------
U_CDECL_BEGIN
static int32_t U_CALLCONV
getFoldingOffset(uint32_t data) {
    /* if bit 15 is set, then the folding offset is in bits 14..0 of the 16-bit trie result */
    if(data&0x8000) {
        return (int32_t)(data&0x7fff);
    } else {
        return 0;
    }
}
U_CDECL_END

U_NAMESPACE_BEGIN

//-----------------------------------------------------------------------------
//
//    Constructors.
//
//-----------------------------------------------------------------------------
RBBIDataWrapper::RBBIDataWrapper(const RBBIDataHeader *data, UErrorCode &status) {
    init(data, status);
}

RBBIDataWrapper::RBBIDataWrapper(const RBBIDataHeader *data, enum EDontAdopt, UErrorCode &status) {
    init(data, status);
    fDontFreeData = TRUE;
}

RBBIDataWrapper::RBBIDataWrapper(UDataMemory* udm, UErrorCode &status) {
    const RBBIDataHeader *d = (const RBBIDataHeader *)
        // ((char *)&(udm->pHeader->info) + udm->pHeader->info.size);
        // taking into consideration the padding added in by udata_write
        ((char *)(udm->pHeader) + udm->pHeader->dataHeader.headerSize);
    init(d, status);
    fUDataMem = udm;
}

//-----------------------------------------------------------------------------
//
//    init().   Does most of the work of construction, shared between the
//              constructors.
//
//-----------------------------------------------------------------------------
void RBBIDataWrapper::init(const RBBIDataHeader *data, UErrorCode &status) {
    if (U_FAILURE(status)) {
        return;
    }
    fHeader = data;
    if (fHeader->fMagic != 0xb1a0 || fHeader->fFormatVersion[0] != 3) 
    {
        status = U_INVALID_FORMAT_ERROR;
        return;
    }
    // Note: in ICU version 3.2 and earlier, there was a formatVersion 1
    //       that is no longer supported.  At that time fFormatVersion was
    //       an int32_t field, rather than an array of 4 bytes.

    fDontFreeData = FALSE;
    fUDataMem     = NULL;
    fReverseTable = NULL;
    fSafeFwdTable = NULL;
    fSafeRevTable = NULL;
    if (data->fFTableLen != 0) {
        fForwardTable = (RBBIStateTable *)((char *)data + fHeader->fFTable);
    }
    if (data->fRTableLen != 0) {
        fReverseTable = (RBBIStateTable *)((char *)data + fHeader->fRTable);
    }
    if (data->fSFTableLen != 0) {
        fSafeFwdTable = (RBBIStateTable *)((char *)data + fHeader->fSFTable);
    }
    if (data->fSRTableLen != 0) {
        fSafeRevTable = (RBBIStateTable *)((char *)data + fHeader->fSRTable);
    }


    utrie_unserialize(&fTrie,
                       (uint8_t *)data + fHeader->fTrie,
                       fHeader->fTrieLen,
                       &status);
    if (U_FAILURE(status)) {
        return;
    }
    fTrie.getFoldingOffset=getFoldingOffset;


    fRuleSource   = (UChar *)((char *)data + fHeader->fRuleSource);
    fRuleString.setTo(TRUE, fRuleSource, -1);
    U_ASSERT(data->fRuleSourceLen > 0);

    fRuleStatusTable = (int32_t *)((char *)data + fHeader->fStatusTable);
    fStatusMaxIdx    = data->fStatusTableLen / sizeof(int32_t);

    fRefCount = 1;

#ifdef RBBI_DEBUG
    char *debugEnv = getenv("U_RBBIDEBUG");
    if (debugEnv && uprv_strstr(debugEnv, "data")) {this->printData();}
#endif
}


//-----------------------------------------------------------------------------
//
//    Destructor.     Don't call this - use removeReference() instead.
//
//-----------------------------------------------------------------------------
RBBIDataWrapper::~RBBIDataWrapper() {
    U_ASSERT(fRefCount == 0);
    if (fUDataMem) {
        udata_close(fUDataMem);
    } else if (!fDontFreeData) {
        uprv_free((void *)fHeader);
    }
}



//-----------------------------------------------------------------------------
//
//   Operator ==    Consider two RBBIDataWrappers to be equal if they
//                  refer to the same underlying data.  Although
//                  the data wrappers are normally shared between
//                  iterator instances, it's possible to independently
//                  open the same data twice, and get two instances, which
//                  should still be ==.
//
//-----------------------------------------------------------------------------
UBool RBBIDataWrapper::operator ==(const RBBIDataWrapper &other) const {
    if (fHeader == other.fHeader) {
        return TRUE;
    }
    if (fHeader->fLength != other.fHeader->fLength) {
        return FALSE;
    }
    if (uprv_memcmp(fHeader, other.fHeader, fHeader->fLength) == 0) {
        return TRUE;
    }
    return FALSE;
}

int32_t  RBBIDataWrapper::hashCode() {
    return fHeader->fFTableLen;
}



//-----------------------------------------------------------------------------
//
//    Reference Counting.   A single RBBIDataWrapper object is shared among
//                          however many RulesBasedBreakIterator instances are
//                          referencing the same data.
//
//-----------------------------------------------------------------------------
void RBBIDataWrapper::removeReference() {
    if (umtx_atomic_dec(&fRefCount) == 0) {
        delete this;
    }
}


RBBIDataWrapper *RBBIDataWrapper::addReference() {
   umtx_atomic_inc(&fRefCount);
   return this;
}



//-----------------------------------------------------------------------------
//
//  getRuleSourceString
//
//-----------------------------------------------------------------------------
const UnicodeString &RBBIDataWrapper::getRuleSourceString() const {
    return fRuleString;
}


//-----------------------------------------------------------------------------
//
//  print   -  debugging function to dump the runtime data tables.
//
//-----------------------------------------------------------------------------
#ifdef RBBI_DEBUG
void  RBBIDataWrapper::printTable(const char *heading, const RBBIStateTable *table) {
    uint32_t   c;
    uint32_t   s;

    RBBIDebugPrintf("   %s\n", heading);

    RBBIDebugPrintf("State |  Acc  LA TagIx");
    for (c=0; c<fHeader->fCatCount; c++) {RBBIDebugPrintf("%3d ", c);}
    RBBIDebugPrintf("\n------|---------------"); for (c=0;c<fHeader->fCatCount; c++) {
        RBBIDebugPrintf("----");
    }
    RBBIDebugPrintf("\n");

    if (table == NULL) {
        RBBIDebugPrintf("         N U L L   T A B L E\n\n");
        return;
    }
    for (s=0; s<table->fNumStates; s++) {
        RBBIStateTableRow *row = (RBBIStateTableRow *)
                                  (table->fTableData + (table->fRowLen * s));
        RBBIDebugPrintf("%4d  |  %3d %3d %3d ", s, row->fAccepting, row->fLookAhead, row->fTagIdx);
        for (c=0; c<fHeader->fCatCount; c++)  {
            RBBIDebugPrintf("%3d ", row->fNextState[c]);
        }
        RBBIDebugPrintf("\n");
    }
    RBBIDebugPrintf("\n");
}
#endif


#ifdef RBBI_DEBUG
void  RBBIDataWrapper::printData() {
    RBBIDebugPrintf("RBBI Data at %p\n", (void *)fHeader);
    RBBIDebugPrintf("   Version = {%d %d %d %d}\n", fHeader->fFormatVersion[0], fHeader->fFormatVersion[1],
                                                    fHeader->fFormatVersion[2], fHeader->fFormatVersion[3]);
    RBBIDebugPrintf("   total length of data  = %d\n", fHeader->fLength);
    RBBIDebugPrintf("   number of character categories = %d\n\n", fHeader->fCatCount);

    printTable("Forward State Transition Table", fForwardTable);
    printTable("Reverse State Transition Table", fReverseTable);
    printTable("Safe Forward State Transition Table", fSafeFwdTable);
    printTable("Safe Reverse State Transition Table", fSafeRevTable);

    RBBIDebugPrintf("\nOrignal Rules source:\n");
    for (int32_t c=0; fRuleSource[c] != 0; c++) {
        RBBIDebugPrintf("%c", fRuleSource[c]);
    }
    RBBIDebugPrintf("\n\n");
}
#endif


U_NAMESPACE_END
U_NAMESPACE_USE

//-----------------------------------------------------------------------------
//
//  ubrk_swap   -  byte swap and char encoding swap of RBBI data
//
//-----------------------------------------------------------------------------

U_CAPI int32_t U_EXPORT2
ubrk_swap(const UDataSwapper *ds, const void *inData, int32_t length, void *outData,
           UErrorCode *status) {

    if (status == NULL || U_FAILURE(*status)) {
        return 0;
    }
    if(ds==NULL || inData==NULL || length<-1 || (length>0 && outData==NULL)) {
        *status=U_ILLEGAL_ARGUMENT_ERROR;
        return 0;
    }

    //
    //  Check that the data header is for for break data.
    //    (Header contents are defined in genbrk.cpp)
    //
    const UDataInfo *pInfo = (const UDataInfo *)((const char *)inData+4);
    if(!(  pInfo->dataFormat[0]==0x42 &&   /* dataFormat="Brk " */
           pInfo->dataFormat[1]==0x72 &&
           pInfo->dataFormat[2]==0x6b &&
           pInfo->dataFormat[3]==0x20 &&
           pInfo->formatVersion[0]==3  )) {
        udata_printError(ds, "ubrk_swap(): data format %02x.%02x.%02x.%02x (format version %02x) is not recognized\n",
                         pInfo->dataFormat[0], pInfo->dataFormat[1],
                         pInfo->dataFormat[2], pInfo->dataFormat[3],
                         pInfo->formatVersion[0]);
        *status=U_UNSUPPORTED_ERROR;
        return 0;
    }

    //
    // Swap the data header.  (This is the generic ICU Data Header, not the RBBI Specific
    //                         RBBIDataHeader).  This swap also conveniently gets us
    //                         the size of the ICU d.h., which lets us locate the start
    //                         of the RBBI specific data.
    //
    int32_t headerSize=udata_swapDataHeader(ds, inData, length, outData, status);


    //
    // Get the RRBI Data Header, and check that it appears to be OK.
    //
    //    Note:  ICU 3.2 and earlier, RBBIDataHeader::fDataFormat was actually 
    //           an int32_t with a value of 1.  Starting with ICU 3.4,
    //           RBBI's fDataFormat matches the dataFormat field from the
    //           UDataInfo header, four int8_t bytes.  The value is {3,1,0,0}
    //
    const uint8_t  *inBytes =(const uint8_t *)inData+headerSize;
    RBBIDataHeader *rbbiDH = (RBBIDataHeader *)inBytes;
    if (ds->readUInt32(rbbiDH->fMagic) != 0xb1a0 || 
        rbbiDH->fFormatVersion[0] != 3 ||
        ds->readUInt32(rbbiDH->fLength)  <  sizeof(RBBIDataHeader)) 
    {
        udata_printError(ds, "ubrk_swap(): RBBI Data header is invalid.\n");
        *status=U_UNSUPPORTED_ERROR;
        return 0;
    }

    //
    // Prefight operation?  Just return the size
    //
    int32_t breakDataLength = ds->readUInt32(rbbiDH->fLength);
    int32_t totalSize = headerSize + breakDataLength;
    if (length < 0) {
        return totalSize;
    }

    //
    // Check that length passed in is consistent with length from RBBI data header.
    //
    if (length < totalSize) {
        udata_printError(ds, "ubrk_swap(): too few bytes (%d after ICU Data header) for break data.\n",
                            breakDataLength);
        *status=U_INDEX_OUTOFBOUNDS_ERROR;
        return 0;
        }


    //
    // Swap the Data.  Do the data itself first, then the RBBI Data Header, because
    //                 we need to reference the header to locate the data, and an
    //                 inplace swap of the header leaves it unusable.
    //
    uint8_t         *outBytes = (uint8_t *)outData + headerSize;
    RBBIDataHeader  *outputDH = (RBBIDataHeader *)outBytes;

    int32_t   tableStartOffset;
    int32_t   tableLength;

    //
    // If not swapping in place, zero out the output buffer before starting.
    //    Individual tables and other data items within are aligned to 8 byte boundaries
    //    when originally created.  Any unused space between items needs to be zero.
    //
    if (inBytes != outBytes) {
        uprv_memset(outBytes, 0, breakDataLength);
    }

    //
    // Each state table begins with several 32 bit fields.  Calculate the size
    //   in bytes of these.
    //
    int32_t         topSize = offsetof(RBBIStateTable, fTableData);

    // Forward state table.  
    tableStartOffset = ds->readUInt32(rbbiDH->fFTable);
    tableLength      = ds->readUInt32(rbbiDH->fFTableLen);

    if (tableLength > 0) {
        ds->swapArray32(ds, inBytes+tableStartOffset, topSize, 
                            outBytes+tableStartOffset, status);
        ds->swapArray16(ds, inBytes+tableStartOffset+topSize, tableLength-topSize,
                            outBytes+tableStartOffset+topSize, status);
    }
    
    // Reverse state table.  Same layout as forward table, above.
    tableStartOffset = ds->readUInt32(rbbiDH->fRTable);
    tableLength      = ds->readUInt32(rbbiDH->fRTableLen);

    if (tableLength > 0) {
        ds->swapArray32(ds, inBytes+tableStartOffset, topSize, 
                            outBytes+tableStartOffset, status);
        ds->swapArray16(ds, inBytes+tableStartOffset+topSize, tableLength-topSize,
                            outBytes+tableStartOffset+topSize, status);
    }

    // Safe Forward state table.  Same layout as forward table, above.
    tableStartOffset = ds->readUInt32(rbbiDH->fSFTable);
    tableLength      = ds->readUInt32(rbbiDH->fSFTableLen);

    if (tableLength > 0) {
        ds->swapArray32(ds, inBytes+tableStartOffset, topSize, 
                            outBytes+tableStartOffset, status);
        ds->swapArray16(ds, inBytes+tableStartOffset+topSize, tableLength-topSize,
                            outBytes+tableStartOffset+topSize, status);
    }

    // Safe Reverse state table.  Same layout as forward table, above.
    tableStartOffset = ds->readUInt32(rbbiDH->fSRTable);
    tableLength      = ds->readUInt32(rbbiDH->fSRTableLen);

    if (tableLength > 0) {
        ds->swapArray32(ds, inBytes+tableStartOffset, topSize, 
                            outBytes+tableStartOffset, status);
        ds->swapArray16(ds, inBytes+tableStartOffset+topSize, tableLength-topSize,
                            outBytes+tableStartOffset+topSize, status);
    }

    // Trie table for character categories
    utrie_swap(ds, inBytes+ds->readUInt32(rbbiDH->fTrie), ds->readUInt32(rbbiDH->fTrieLen),
                            outBytes+ds->readUInt32(rbbiDH->fTrie), status);

    // Source Rules Text.  It's UChar data
    ds->swapArray16(ds, inBytes+ds->readUInt32(rbbiDH->fRuleSource), ds->readUInt32(rbbiDH->fRuleSourceLen),
                        outBytes+ds->readUInt32(rbbiDH->fRuleSource), status);

    // Table of rule status values.  It's all int_32 values
    ds->swapArray32(ds, inBytes+ds->readUInt32(rbbiDH->fStatusTable), ds->readUInt32(rbbiDH->fStatusTableLen),
                        outBytes+ds->readUInt32(rbbiDH->fStatusTable), status);

    // And, last, the header.
    //   It is all int32_t values except for fFormataVersion, which is an array of four bytes.
    //   Swap the whole thing as int32_t, then re-swap the one field.
    //
    ds->swapArray32(ds, inBytes, sizeof(RBBIDataHeader), outBytes, status);
    ds->swapArray32(ds, outputDH->fFormatVersion, 4, outputDH->fFormatVersion, status);

    return totalSize;
}


#endif /* #if !UCONFIG_NO_BREAK_ITERATION */
