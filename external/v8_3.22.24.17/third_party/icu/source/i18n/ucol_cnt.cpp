/*
 *******************************************************************************
 *
 *   Copyright (C) 2001-2008, International Business Machines
 *   Corporation and others.  All Rights Reserved.
 *
 *******************************************************************************
 *   file name:  ucol_cnt.cpp
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

#include "unicode/utypes.h"

#if !UCONFIG_NO_COLLATION

#include "unicode/uchar.h"
#include "ucol_cnt.h"
#include "cmemory.h"

static void uprv_growTable(ContractionTable *tbl, UErrorCode *status) {
    if(tbl->position == tbl->size) {
        uint32_t *newData = (uint32_t *)uprv_realloc(tbl->CEs, 2*tbl->size*sizeof(uint32_t));
        if(newData == NULL) {
            *status = U_MEMORY_ALLOCATION_ERROR;
            return;
        }
        UChar *newCPs = (UChar *)uprv_realloc(tbl->codePoints, 2*tbl->size*sizeof(UChar));
        if(newCPs == NULL) {
            uprv_free(newData);
            *status = U_MEMORY_ALLOCATION_ERROR;
            return;
        }
        tbl->CEs = newData;
        tbl->codePoints = newCPs;
        tbl->size *= 2;
    }
}

U_CAPI CntTable*  U_EXPORT2
/*uprv_cnttab_open(CompactEIntArray *mapping, UErrorCode *status) {*/
uprv_cnttab_open(UNewTrie *mapping, UErrorCode *status) {
    if(U_FAILURE(*status)) {
        return 0;
    }
    CntTable *tbl = (CntTable *)uprv_malloc(sizeof(CntTable));
    if(tbl == NULL) {
        *status = U_MEMORY_ALLOCATION_ERROR;
        return NULL;
    }
    tbl->mapping = mapping;
    tbl->elements = (ContractionTable **)uprv_malloc(INIT_EXP_TABLE_SIZE*sizeof(ContractionTable *));
    if(tbl->elements == NULL) {
        *status = U_MEMORY_ALLOCATION_ERROR;
        uprv_free(tbl);
        return NULL;
    }
    tbl->capacity = INIT_EXP_TABLE_SIZE;
    uprv_memset(tbl->elements, 0, INIT_EXP_TABLE_SIZE*sizeof(ContractionTable *));
    tbl->size = 0;
    tbl->position = 0;
    tbl->CEs = NULL;
    tbl->codePoints = NULL;
    tbl->offsets = NULL;
    tbl->currentTag = NOT_FOUND_TAG;
    return tbl;
}

static ContractionTable *addATableElement(CntTable *table, uint32_t *key, UErrorCode *status) {
    ContractionTable *el = (ContractionTable *)uprv_malloc(sizeof(ContractionTable));
    if(el == NULL) {
        goto outOfMemory;
    }
    el->CEs = (uint32_t *)uprv_malloc(INIT_EXP_TABLE_SIZE*sizeof(uint32_t));
    if(el->CEs == NULL) {
        goto outOfMemory;
    }

    el->codePoints = (UChar *)uprv_malloc(INIT_EXP_TABLE_SIZE*sizeof(UChar));
    if(el->codePoints == NULL) {
        uprv_free(el->CEs);
        goto outOfMemory;
    }

    el->position = 0;
    el->size = INIT_EXP_TABLE_SIZE;
    uprv_memset(el->CEs, 0, INIT_EXP_TABLE_SIZE*sizeof(uint32_t));
    uprv_memset(el->codePoints, 0, INIT_EXP_TABLE_SIZE*sizeof(UChar));

    table->elements[table->size] = el;

    //uhash_put(table->elements, (void *)table->size, el, status);

    *key = table->size++;

    if(table->size == table->capacity) {
        ContractionTable **newElements = (ContractionTable **)uprv_malloc(table->capacity*2*sizeof(ContractionTable *));
        // do realloc
        /*        table->elements = (ContractionTable **)realloc(table->elements, table->capacity*2*sizeof(ContractionTable *));*/
        if(newElements == NULL) {
            uprv_free(el->codePoints);
            uprv_free(el->CEs);
            goto outOfMemory;
        }
        ContractionTable **oldElements = table->elements;
        uprv_memcpy(newElements, oldElements, table->capacity*sizeof(ContractionTable *));
        uprv_memset(newElements+table->capacity, 0, table->capacity*sizeof(ContractionTable *));
        table->capacity *= 2;
        table->elements = newElements;
        uprv_free(oldElements);
    }

    return el;

outOfMemory:
    *status = U_MEMORY_ALLOCATION_ERROR;
    if (el) uprv_free(el);
    return NULL;
}

U_CAPI int32_t  U_EXPORT2
uprv_cnttab_constructTable(CntTable *table, uint32_t mainOffset, UErrorCode *status) {
    int32_t i = 0, j = 0;
    if(U_FAILURE(*status) || table->size == 0) {
        return 0;
    }

    table->position = 0;

    if(table->offsets != NULL) {
        uprv_free(table->offsets);
    }
    table->offsets = (int32_t *)uprv_malloc(table->size*sizeof(int32_t));
    if(table->offsets == NULL) {
        *status = U_MEMORY_ALLOCATION_ERROR;
        return 0;
    }


    /* See how much memory we need */
    for(i = 0; i<table->size; i++) {
        table->offsets[i] = table->position+mainOffset;
        table->position += table->elements[i]->position;
    }

    /* Allocate it */
    if(table->CEs != NULL) {
        uprv_free(table->CEs);
    }
    table->CEs = (uint32_t *)uprv_malloc(table->position*sizeof(uint32_t));
    if(table->CEs == NULL) {
        *status = U_MEMORY_ALLOCATION_ERROR;
        uprv_free(table->offsets);
        table->offsets = NULL;
        return 0;
    }
    uprv_memset(table->CEs, '?', table->position*sizeof(uint32_t));

    if(table->codePoints != NULL) {
        uprv_free(table->codePoints);
    }
    table->codePoints = (UChar *)uprv_malloc(table->position*sizeof(UChar));
    if(table->codePoints == NULL) {
        *status = U_MEMORY_ALLOCATION_ERROR;
        uprv_free(table->offsets);
        table->offsets = NULL;
        uprv_free(table->CEs);
        table->CEs = NULL;
        return 0;
    }
    uprv_memset(table->codePoints, '?', table->position*sizeof(UChar));

    /* Now stuff the things in*/

    UChar *cpPointer = table->codePoints;
    uint32_t *CEPointer = table->CEs;
    for(i = 0; i<table->size; i++) {
        int32_t size = table->elements[i]->position;
        uint8_t ccMax = 0, ccMin = 255, cc = 0;
        for(j = 1; j<size; j++) {
            cc = u_getCombiningClass(table->elements[i]->codePoints[j]);
            if(cc>ccMax) {
                ccMax = cc;
            }
            if(cc<ccMin) {
                ccMin = cc;
            }
            *(cpPointer+j) = table->elements[i]->codePoints[j];
        }
        *cpPointer = ((ccMin==ccMax)?1:0 << 8) | ccMax;

        uprv_memcpy(CEPointer, table->elements[i]->CEs, size*sizeof(uint32_t));
        for(j = 0; j<size; j++) {
            if(isCntTableElement(*(CEPointer+j))) {
                *(CEPointer+j) = constructContractCE(getCETag(*(CEPointer+j)), table->offsets[getContractOffset(*(CEPointer+j))]);
            }
        }
        cpPointer += size;
        CEPointer += size;
    }

    // TODO: this one apparently updates the contraction CEs to point to a real address (relative to the 
    // start of the flat file). However, what is done below is just wrong and it affects building of 
    // tailorings that have constructions in a bad way. At least, one should enumerate the trie. Also,
    // keeping a list of code points that are contractions might be smart, although I'm not sure if it's
    // feasible.
    uint32_t CE;
    for(i = 0; i<=0x10FFFF; i++) {
        /*CE = ucmpe32_get(table->mapping, i);*/
        CE = utrie_get32(table->mapping, i, NULL);
        if(isCntTableElement(CE)) {
            CE = constructContractCE(getCETag(CE), table->offsets[getContractOffset(CE)]);
            /*ucmpe32_set(table->mapping, i, CE);*/
            utrie_set32(table->mapping, i, CE);
        }
    }


    return table->position;
}

static ContractionTable *uprv_cnttab_cloneContraction(ContractionTable *t, UErrorCode *status) {
    ContractionTable *r = (ContractionTable *)uprv_malloc(sizeof(ContractionTable));
    if(r == NULL) {
        goto outOfMemory;
    }

    r->position = t->position;
    r->size = t->size;

    r->codePoints = (UChar *)uprv_malloc(sizeof(UChar)*t->size);
    if(r->codePoints == NULL) {
        goto outOfMemory;
    }
    r->CEs = (uint32_t *)uprv_malloc(sizeof(uint32_t)*t->size);
    if(r->CEs == NULL) {
        uprv_free(r->codePoints);
        goto outOfMemory;
    }
    uprv_memcpy(r->codePoints, t->codePoints, sizeof(UChar)*t->size);
    uprv_memcpy(r->CEs, t->CEs, sizeof(uint32_t)*t->size);

    return r;

outOfMemory:
    *status = U_MEMORY_ALLOCATION_ERROR;
    if (r) uprv_free(r);
    return NULL;
}

U_CAPI CntTable* U_EXPORT2
uprv_cnttab_clone(CntTable *t, UErrorCode *status) {
    if(U_FAILURE(*status)) {
        return NULL;
    }
    int32_t i = 0;
    CntTable *r = (CntTable *)uprv_malloc(sizeof(CntTable));
    /* test for NULL */
    if (r == NULL) {
        goto outOfMemory;
    }
    r->position = t->position;
    r->size = t->size;
    r->capacity = t->capacity;

    r->mapping = t->mapping;

    r->elements = (ContractionTable **)uprv_malloc(t->capacity*sizeof(ContractionTable *));
    /* test for NULL */
    if (r->elements == NULL) {
        goto outOfMemory;
    }
    //uprv_memcpy(r->elements, t->elements, t->capacity*sizeof(ContractionTable *));

    for(i = 0; i<t->size; i++) {
        r->elements[i] = uprv_cnttab_cloneContraction(t->elements[i], status);
    }

    if(t->CEs != NULL) {
        r->CEs = (uint32_t *)uprv_malloc(t->position*sizeof(uint32_t));
        /* test for NULL */
        if (r->CEs == NULL) {
            uprv_free(r->elements);
            goto outOfMemory;
        }
        uprv_memcpy(r->CEs, t->CEs, t->position*sizeof(uint32_t));
    } else {
        r->CEs = NULL;
    }

    if(t->codePoints != NULL) {
        r->codePoints = (UChar *)uprv_malloc(t->position*sizeof(UChar));
        /* test for NULL */
        if (r->codePoints == NULL) {
            uprv_free(r->CEs);
            uprv_free(r->elements);
            goto outOfMemory;
        }
        uprv_memcpy(r->codePoints, t->codePoints, t->position*sizeof(UChar));
    } else {
        r->codePoints = NULL;
    }

    if(t->offsets != NULL) {
        r->offsets = (int32_t *)uprv_malloc(t->size*sizeof(int32_t));
        /* test for NULL */
        if (r->offsets == NULL) {
            uprv_free(r->codePoints);
            uprv_free(r->CEs);
            uprv_free(r->elements);
            goto outOfMemory;
        }
        uprv_memcpy(r->offsets, t->offsets, t->size*sizeof(int32_t));
    } else {
        r->offsets = NULL;
    }

    return r;

outOfMemory:
    *status = U_MEMORY_ALLOCATION_ERROR;
    if (r) uprv_free(r);
    return NULL;
}

U_CAPI void  U_EXPORT2
uprv_cnttab_close(CntTable *table) {
    int32_t i = 0;
    for(i = 0; i<table->size; i++) {
        uprv_free(table->elements[i]->CEs);
        uprv_free(table->elements[i]->codePoints);
        uprv_free(table->elements[i]);
    }
    uprv_free(table->elements);
    uprv_free(table->CEs);
    uprv_free(table->offsets);
    uprv_free(table->codePoints);
    uprv_free(table);
}

/* this is for adding non contractions */
U_CAPI uint32_t  U_EXPORT2
uprv_cnttab_changeLastCE(CntTable *table, uint32_t element, uint32_t value, UErrorCode *status) {
    element &= 0xFFFFFF;

    ContractionTable *tbl = NULL;
    if(U_FAILURE(*status)) {
        return 0;
    }

    if((element == 0xFFFFFF) || (tbl = table->elements[element]) == NULL) {
        return 0;
    }

    tbl->CEs[tbl->position-1] = value;

    return(constructContractCE(table->currentTag, element));
}


/* inserts a part of contraction sequence in table. Sequences behind the offset are moved back. If element is non existent, it creates on. Returns element handle */
U_CAPI uint32_t  U_EXPORT2
uprv_cnttab_insertContraction(CntTable *table, uint32_t element, UChar codePoint, uint32_t value, UErrorCode *status) {

    ContractionTable *tbl = NULL;

    if(U_FAILURE(*status)) {
        return 0;
    }
    element &= 0xFFFFFF;

    if((element == 0xFFFFFF) || (tbl = table->elements[element]) == NULL) {
        tbl = addATableElement(table, &element, status);
        if (U_FAILURE(*status)) {
            return 0;
        }
    }

    uprv_growTable(tbl, status);

    uint32_t offset = 0;


    while(tbl->codePoints[offset] < codePoint && offset<tbl->position) {
        offset++;
    }

    uint32_t i = tbl->position;
    for(i = tbl->position; i > offset; i--) {
        tbl->CEs[i] = tbl->CEs[i-1];
        tbl->codePoints[i] = tbl->codePoints[i-1];
    }

    tbl->CEs[offset] = value;
    tbl->codePoints[offset] = codePoint;

    tbl->position++;

    return(constructContractCE(table->currentTag, element));
}


/* adds more contractions in table. If element is non existant, it creates on. Returns element handle */
U_CAPI uint32_t  U_EXPORT2
uprv_cnttab_addContraction(CntTable *table, uint32_t element, UChar codePoint, uint32_t value, UErrorCode *status) {

    element &= 0xFFFFFF;

    ContractionTable *tbl = NULL;

    if(U_FAILURE(*status)) {
        return 0;
    }

    if((element == 0xFFFFFF) || (tbl = table->elements[element]) == NULL) {
        tbl = addATableElement(table, &element, status);
        if (U_FAILURE(*status)) {
            return 0;
        }
    } 

    uprv_growTable(tbl, status);

    tbl->CEs[tbl->position] = value;
    tbl->codePoints[tbl->position] = codePoint;

    tbl->position++;

    return(constructContractCE(table->currentTag, element));
}

/* sets a part of contraction sequence in table. If element is non existant, it creates on. Returns element handle */
U_CAPI uint32_t  U_EXPORT2
uprv_cnttab_setContraction(CntTable *table, uint32_t element, uint32_t offset, UChar codePoint, uint32_t value, UErrorCode *status) {

    element &= 0xFFFFFF;
    ContractionTable *tbl = NULL;

    if(U_FAILURE(*status)) {
        return 0;
    }

    if((element == 0xFFFFFF) || (tbl = table->elements[element]) == NULL) {
        tbl = addATableElement(table, &element, status);
        if (U_FAILURE(*status)) {
            return 0;
        }
        
    }

    if(offset >= tbl->size) {
        *status = U_INDEX_OUTOFBOUNDS_ERROR;
        return 0;
    }
    tbl->CEs[offset] = value;
    tbl->codePoints[offset] = codePoint;

    //return(offset);
    return(constructContractCE(table->currentTag, element));
}

static ContractionTable *_cnttab_getContractionTable(CntTable *table, uint32_t element) {
    element &= 0xFFFFFF;
    ContractionTable *tbl = NULL;

    if(element != 0xFFFFFF) {
        tbl = table->elements[element]; /* This could also return NULL */
    }
    return tbl;
}

static int32_t _cnttab_findCP(ContractionTable *tbl, UChar codePoint) {
    uint32_t position = 0;
    if(tbl == NULL) {
        return -1;
    }

    while(codePoint > tbl->codePoints[position]) {
        position++;
        if(position > tbl->position) {
            return -1;
        }
    }
    if (codePoint == tbl->codePoints[position]) {
        return position;
    } else {
        return -1;
    }
}

static uint32_t _cnttab_getCE(ContractionTable *tbl, int32_t position) {
    if(tbl == NULL) {
        return UCOL_NOT_FOUND;
    }
    if((uint32_t)position > tbl->position || position == -1) {
        return UCOL_NOT_FOUND;
    } else {
        return tbl->CEs[position];
    }
}

U_CAPI int32_t  U_EXPORT2
uprv_cnttab_findCP(CntTable *table, uint32_t element, UChar codePoint, UErrorCode *status) {

    if(U_FAILURE(*status)) {
        return 0;
    }

    return _cnttab_findCP(_cnttab_getContractionTable(table, element), codePoint);
}

U_CAPI uint32_t  U_EXPORT2
uprv_cnttab_getCE(CntTable *table, uint32_t element, uint32_t position, UErrorCode *status) {
    if(U_FAILURE(*status)) {
        return UCOL_NOT_FOUND;
    }

    return(_cnttab_getCE(_cnttab_getContractionTable(table, element), position));
}

U_CAPI uint32_t  U_EXPORT2
uprv_cnttab_findCE(CntTable *table, uint32_t element, UChar codePoint, UErrorCode *status) {
    if(U_FAILURE(*status)) {
        return UCOL_NOT_FOUND;
    }
    ContractionTable *tbl = _cnttab_getContractionTable(table, element);
    return _cnttab_getCE(tbl, _cnttab_findCP(tbl, codePoint));
}

U_CAPI UBool  U_EXPORT2
uprv_cnttab_isTailored(CntTable *table, uint32_t element, UChar *ztString, UErrorCode *status) {
    if(U_FAILURE(*status)) {
        return FALSE;
    }

    while(*(ztString)!=0) {
        element = uprv_cnttab_findCE(table, element, *(ztString), status);
        if(element == UCOL_NOT_FOUND) {
            return FALSE;
        }
        if(!isCntTableElement(element)) {
            return TRUE;
        }
        ztString++;
    }
    return (UBool)(uprv_cnttab_getCE(table, element, 0, status) != UCOL_NOT_FOUND);
}

U_CAPI uint32_t  U_EXPORT2
uprv_cnttab_changeContraction(CntTable *table, uint32_t element, UChar codePoint, uint32_t newCE, UErrorCode *status) {

    element &= 0xFFFFFF;
    ContractionTable *tbl = NULL;

    if(U_FAILURE(*status)) {
        return 0;
    }

    if((element == 0xFFFFFF) || (tbl = table->elements[element]) == NULL) {
        return 0;
    }

    uint32_t position = 0;

    while(codePoint > tbl->codePoints[position]) {
        position++;
        if(position > tbl->position) {
            return UCOL_NOT_FOUND;
        }
    }
    if (codePoint == tbl->codePoints[position]) {
        tbl->CEs[position] = newCE;
        return element;
    } else {
        return UCOL_NOT_FOUND;
    }
}

#endif /* #if !UCONFIG_NO_COLLATION */
