/*
*******************************************************************************
*
*   Copyright (C) 2001-2008, International Business Machines
*   Corporation and others.  All Rights Reserved.
*
*******************************************************************************
*   file name:  ucol_tok.cpp
*   encoding:   US-ASCII
*   tab size:   8 (not used)
*   indentation:4
*
*   created 02/22/2001
*   created by: Vladimir Weinstein
*
* This module maintains a contraction table structure in expanded form
* and provides means to flatten this structure
* 
*/

#ifndef UCOL_CNTTABLE_H
#define UCOL_CNTTABLE_H

#include "unicode/utypes.h"

#if !UCONFIG_NO_COLLATION

#include "utrie.h"
#include "ucol_imp.h"

U_CDECL_BEGIN

#define UPRV_CNTTAB_NEWELEMENT 0xFFFFFF

#define isCntTableElement(CE) (isSpecial((CE)) && \
((getCETag((CE)) == CONTRACTION_TAG)||(getCETag((CE)) == SPEC_PROC_TAG)))

typedef struct ContractionTable ContractionTable;
struct ContractionTable {
    UChar *codePoints;
    uint32_t *CEs;
    uint32_t position;
    uint32_t size;
};

struct CntTable {
    ContractionTable **elements;
    /*CompactEIntArray *mapping;*/
    UNewTrie *mapping;
    UChar *codePoints;
    uint32_t *CEs;
    int32_t *offsets;
    int32_t position;
    int32_t size;
    int32_t capacity;
    UColCETags currentTag;
};

U_CAPI CntTable* U_EXPORT2 
/*uprv_cnttab_open(CompactEIntArray *mapping, UErrorCode *status);*/
uprv_cnttab_open(UNewTrie *mapping, UErrorCode *status);
U_CAPI CntTable* U_EXPORT2 
uprv_cnttab_clone(CntTable *table, UErrorCode *status);
U_CAPI void U_EXPORT2 
uprv_cnttab_close(CntTable *table);

/* construct the table for output */
U_CAPI int32_t U_EXPORT2 
uprv_cnttab_constructTable(CntTable *table, uint32_t mainOffset, UErrorCode *status); 
/* adds more contractions in table. If element is non existant, it creates on. Returns element handle */
U_CAPI uint32_t U_EXPORT2 
uprv_cnttab_addContraction(CntTable *table, uint32_t element, UChar codePoint, uint32_t value, UErrorCode *status);
/* sets a part of contraction sequence in table. If element is non existant, it creates on. Returns element handle */
U_CAPI uint32_t U_EXPORT2 
uprv_cnttab_setContraction(CntTable *table, uint32_t element, uint32_t offset, UChar codePoint, uint32_t value, UErrorCode *status);
/* inserts a part of contraction sequence in table. Sequences behind the offset are moved back. If element is non existant, it creates on. Returns element handle */
U_CAPI uint32_t U_EXPORT2 
uprv_cnttab_insertContraction(CntTable *table, uint32_t element, UChar codePoint, uint32_t value, UErrorCode *status);
/* this is for adding non contractions */
U_CAPI uint32_t U_EXPORT2 
uprv_cnttab_changeLastCE(CntTable *table, uint32_t element, uint32_t value, UErrorCode *status);

U_CAPI int32_t U_EXPORT2 
uprv_cnttab_findCP(CntTable *table, uint32_t element, UChar codePoint, UErrorCode *status);

U_CAPI uint32_t U_EXPORT2 
uprv_cnttab_getCE(CntTable *table, uint32_t element, uint32_t position, UErrorCode *status);

U_CAPI uint32_t U_EXPORT2 
uprv_cnttab_changeContraction(CntTable *table, uint32_t element, UChar codePoint, uint32_t newCE, UErrorCode *status);

U_CAPI uint32_t U_EXPORT2 
uprv_cnttab_findCE(CntTable *table, uint32_t element, UChar codePoint, UErrorCode *status);

U_CAPI UBool U_EXPORT2 
uprv_cnttab_isTailored(CntTable *table, uint32_t element, UChar *ztString, UErrorCode *status);

U_CDECL_END

#endif /* #if !UCONFIG_NO_COLLATION */

#endif
