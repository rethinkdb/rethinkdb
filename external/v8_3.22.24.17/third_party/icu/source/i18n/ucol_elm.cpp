/*
*******************************************************************************
*
*   Copyright (C) 2001-2010, International Business Machines
*   Corporation and others.  All Rights Reserved.
*
*******************************************************************************
*   file name:  ucaelems.cpp
*   encoding:   US-ASCII
*   tab size:   8 (not used)
*   indentation:4
*
*   created 02/22/2001
*   created by: Vladimir Weinstein
*
*   This program reads the Franctional UCA table and generates
*   internal format for UCA table as well as inverse UCA table.
*   It then writes binary files containing the data: ucadata.dat 
*   & invuca.dat
* 
*   date        name       comments
*   03/02/2001  synwee     added setMaxExpansion
*   03/07/2001  synwee     merged UCA's maxexpansion and tailoring's
*/

#include "unicode/utypes.h"

#if !UCONFIG_NO_COLLATION

#include "unicode/uchar.h"
#include "unicode/unistr.h"
#include "unicode/ucoleitr.h"
#include "unicode/normlzr.h"
#include "normalizer2impl.h"
#include "ucol_elm.h"
#include "ucol_tok.h"
#include "ucol_cnt.h"
#include "unicode/caniter.h"
#include "cmemory.h"

U_NAMESPACE_USE

static uint32_t uprv_uca_processContraction(CntTable *contractions, UCAElements *element, uint32_t existingCE, UErrorCode *status);

U_CDECL_BEGIN
static int32_t U_CALLCONV
prefixLookupHash(const UHashTok e) {
    UCAElements *element = (UCAElements *)e.pointer;
    UChar buf[256];
    UHashTok key;
    key.pointer = buf;
    uprv_memcpy(buf, element->cPoints, element->cSize*sizeof(UChar));
    buf[element->cSize] = 0;
    //key.pointer = element->cPoints;
    //element->cPoints[element->cSize] = 0;
    return uhash_hashUChars(key);
}

static int8_t U_CALLCONV
prefixLookupComp(const UHashTok e1, const UHashTok e2) {
    UCAElements *element1 = (UCAElements *)e1.pointer;
    UCAElements *element2 = (UCAElements *)e2.pointer;

    UChar buf1[256];
    UHashTok key1;
    key1.pointer = buf1;
    uprv_memcpy(buf1, element1->cPoints, element1->cSize*sizeof(UChar));
    buf1[element1->cSize] = 0;

    UChar buf2[256];
    UHashTok key2;
    key2.pointer = buf2;
    uprv_memcpy(buf2, element2->cPoints, element2->cSize*sizeof(UChar));
    buf2[element2->cSize] = 0;

    return uhash_compareUChars(key1, key2);
}
U_CDECL_END

static int32_t uprv_uca_addExpansion(ExpansionTable *expansions, uint32_t value, UErrorCode *status) {
    if(U_FAILURE(*status)) {
        return 0;
    }
    if(expansions->CEs == NULL) {
        expansions->CEs = (uint32_t *)uprv_malloc(INIT_EXP_TABLE_SIZE*sizeof(uint32_t));
        /* test for NULL */
        if (expansions->CEs == NULL) {
            *status = U_MEMORY_ALLOCATION_ERROR;
            return 0;
        }
        expansions->size = INIT_EXP_TABLE_SIZE;
        expansions->position = 0;
    }

    if(expansions->position == expansions->size) {
        uint32_t *newData = (uint32_t *)uprv_realloc(expansions->CEs, 2*expansions->size*sizeof(uint32_t));
        if(newData == NULL) {
#ifdef UCOL_DEBUG
            fprintf(stderr, "out of memory for expansions\n");
#endif
            *status = U_MEMORY_ALLOCATION_ERROR;
            return -1;
        }
        expansions->CEs = newData;
        expansions->size *= 2;
    }

    expansions->CEs[expansions->position] = value;
    return(expansions->position++);
}

U_CAPI tempUCATable*  U_EXPORT2
uprv_uca_initTempTable(UCATableHeader *image, UColOptionSet *opts, const UCollator *UCA, UColCETags initTag, UColCETags supplementaryInitTag, UErrorCode *status) {
    MaxJamoExpansionTable *maxjet;
    MaxExpansionTable *maxet;
    tempUCATable *t = (tempUCATable *)uprv_malloc(sizeof(tempUCATable));
    /* test for NULL */
    if (t == NULL) {
        *status = U_MEMORY_ALLOCATION_ERROR;
        return NULL;
    }
    uprv_memset(t, 0, sizeof(tempUCATable));

    maxet  = (MaxExpansionTable *)uprv_malloc(sizeof(MaxExpansionTable));
    if (maxet == NULL) {
        goto allocation_failure;
    }
    uprv_memset(maxet, 0, sizeof(MaxExpansionTable));
    t->maxExpansions       = maxet;

    maxjet = (MaxJamoExpansionTable *)uprv_malloc(sizeof(MaxJamoExpansionTable));
    if (maxjet == NULL) {
        goto allocation_failure;
    }
    uprv_memset(maxjet, 0, sizeof(MaxJamoExpansionTable));
    t->maxJamoExpansions = maxjet;

    t->image = image;
    t->options = opts;

    t->UCA = UCA;
    t->expansions = (ExpansionTable *)uprv_malloc(sizeof(ExpansionTable));
    /* test for NULL */
    if (t->expansions == NULL) {
        goto allocation_failure;
    }
    uprv_memset(t->expansions, 0, sizeof(ExpansionTable));

    t->mapping = utrie_open(NULL, NULL, UCOL_ELM_TRIE_CAPACITY,
        UCOL_SPECIAL_FLAG | (initTag<<24),
        UCOL_SPECIAL_FLAG | (supplementaryInitTag << 24),
        TRUE); // Do your own mallocs for the structure, array and have linear Latin 1
    if (U_FAILURE(*status)) {
        goto allocation_failure;
    }
    t->prefixLookup = uhash_open(prefixLookupHash, prefixLookupComp, NULL, status);
    if (U_FAILURE(*status)) {
        goto allocation_failure;
    }
    uhash_setValueDeleter(t->prefixLookup, uhash_freeBlock);

    t->contractions = uprv_cnttab_open(t->mapping, status);
    if (U_FAILURE(*status)) {
        goto cleanup;
    }

    /* copy UCA's maxexpansion and merge as we go along */
    if (UCA != NULL) {
        /* adding an extra initial value for easier manipulation */
        maxet->size            = (int32_t)(UCA->lastEndExpansionCE - UCA->endExpansionCE) + 2;
        maxet->position        = maxet->size - 1;
        maxet->endExpansionCE  = 
            (uint32_t *)uprv_malloc(sizeof(uint32_t) * maxet->size);
        /* test for NULL */
        if (maxet->endExpansionCE == NULL) {
            goto allocation_failure;
        }
        maxet->expansionCESize =
            (uint8_t *)uprv_malloc(sizeof(uint8_t) * maxet->size);
        /* test for NULL */
        if (maxet->expansionCESize == NULL) {
            goto allocation_failure;
        }
        /* initialized value */
        *(maxet->endExpansionCE)  = 0;
        *(maxet->expansionCESize) = 0;
        uprv_memcpy(maxet->endExpansionCE + 1, UCA->endExpansionCE, 
            sizeof(uint32_t) * (maxet->size - 1));
        uprv_memcpy(maxet->expansionCESize + 1, UCA->expansionCESize, 
            sizeof(uint8_t) * (maxet->size - 1));
    }
    else {
        maxet->size     = 0;
    }
    maxjet->endExpansionCE = NULL;
    maxjet->isV = NULL;
    maxjet->size = 0;
    maxjet->position = 0;
    maxjet->maxLSize = 1;
    maxjet->maxVSize = 1;
    maxjet->maxTSize = 1;

    t->unsafeCP = (uint8_t *)uprv_malloc(UCOL_UNSAFECP_TABLE_SIZE);
    /* test for NULL */
    if (t->unsafeCP == NULL) {
        goto allocation_failure;
    }
    t->contrEndCP = (uint8_t *)uprv_malloc(UCOL_UNSAFECP_TABLE_SIZE);
    /* test for NULL */
    if (t->contrEndCP == NULL) {
        goto allocation_failure;
    }
    uprv_memset(t->unsafeCP, 0, UCOL_UNSAFECP_TABLE_SIZE);
    uprv_memset(t->contrEndCP, 0, UCOL_UNSAFECP_TABLE_SIZE);
    t->cmLookup = NULL;
    return t;

allocation_failure:
    *status = U_MEMORY_ALLOCATION_ERROR;
cleanup:
    uprv_uca_closeTempTable(t);
    return NULL;
}

static tempUCATable* U_EXPORT2
uprv_uca_cloneTempTable(tempUCATable *t, UErrorCode *status) {
    if(U_FAILURE(*status)) {
        return NULL;
    }

    tempUCATable *r = (tempUCATable *)uprv_malloc(sizeof(tempUCATable));
    /* test for NULL */
    if (r == NULL) {
        *status = U_MEMORY_ALLOCATION_ERROR;
        return NULL;
    }
    uprv_memset(r, 0, sizeof(tempUCATable));

    /* mapping */
    if(t->mapping != NULL) {
        /*r->mapping = ucmpe32_clone(t->mapping, status);*/
        r->mapping = utrie_clone(NULL, t->mapping, NULL, 0);
    }

    // a hashing clone function would be very nice. We have none currently...
    // However, we should be good, as closing should not produce any prefixed elements.
    r->prefixLookup = NULL; // prefixes are not used in closing

    /* expansions */
    if(t->expansions != NULL) {
        r->expansions = (ExpansionTable *)uprv_malloc(sizeof(ExpansionTable));
        /* test for NULL */
        if (r->expansions == NULL) {
            *status = U_MEMORY_ALLOCATION_ERROR;
            goto cleanup;
        }
        r->expansions->position = t->expansions->position;
        r->expansions->size = t->expansions->size;
        if(t->expansions->CEs != NULL) {
            r->expansions->CEs = (uint32_t *)uprv_malloc(sizeof(uint32_t)*t->expansions->size);
            /* test for NULL */
            if (r->expansions->CEs == NULL) {
                *status = U_MEMORY_ALLOCATION_ERROR;
                goto cleanup;
            }
            uprv_memcpy(r->expansions->CEs, t->expansions->CEs, sizeof(uint32_t)*t->expansions->position);
        } else {
            r->expansions->CEs = NULL;
        }
    }

    if(t->contractions != NULL) {
        r->contractions = uprv_cnttab_clone(t->contractions, status);
        // Check for cloning failure.
        if (r->contractions == NULL) {
            *status = U_MEMORY_ALLOCATION_ERROR;
            goto cleanup;
        }
        r->contractions->mapping = r->mapping;
    }

    if(t->maxExpansions != NULL) {
        r->maxExpansions = (MaxExpansionTable *)uprv_malloc(sizeof(MaxExpansionTable));
        /* test for NULL */
        if (r->maxExpansions == NULL) {
            *status = U_MEMORY_ALLOCATION_ERROR;
            goto cleanup;
        }
        r->maxExpansions->size = t->maxExpansions->size;
        r->maxExpansions->position = t->maxExpansions->position;
        if(t->maxExpansions->endExpansionCE != NULL) {
            r->maxExpansions->endExpansionCE = (uint32_t *)uprv_malloc(sizeof(uint32_t)*t->maxExpansions->size);
            /* test for NULL */
            if (r->maxExpansions->endExpansionCE == NULL) {
                *status = U_MEMORY_ALLOCATION_ERROR;
                goto cleanup;
            }
            uprv_memset(r->maxExpansions->endExpansionCE, 0xDB, sizeof(uint32_t)*t->maxExpansions->size);
            uprv_memcpy(r->maxExpansions->endExpansionCE, t->maxExpansions->endExpansionCE, t->maxExpansions->position*sizeof(uint32_t));
        } else {
            r->maxExpansions->endExpansionCE = NULL;
        }
        if(t->maxExpansions->expansionCESize != NULL) {
            r->maxExpansions->expansionCESize = (uint8_t *)uprv_malloc(sizeof(uint8_t)*t->maxExpansions->size);
            /* test for NULL */
            if (r->maxExpansions->expansionCESize == NULL) {
                *status = U_MEMORY_ALLOCATION_ERROR;
                goto cleanup;
            }
            uprv_memset(r->maxExpansions->expansionCESize, 0xDB, sizeof(uint8_t)*t->maxExpansions->size);
            uprv_memcpy(r->maxExpansions->expansionCESize, t->maxExpansions->expansionCESize, t->maxExpansions->position*sizeof(uint8_t));
        } else {
            r->maxExpansions->expansionCESize = NULL;
        }
    }

    if(t->maxJamoExpansions != NULL) {
        r->maxJamoExpansions = (MaxJamoExpansionTable *)uprv_malloc(sizeof(MaxJamoExpansionTable));
        /* test for NULL */
        if (r->maxJamoExpansions == NULL) {
            *status = U_MEMORY_ALLOCATION_ERROR;
            goto cleanup;
        }
        r->maxJamoExpansions->size = t->maxJamoExpansions->size;
        r->maxJamoExpansions->position = t->maxJamoExpansions->position;
        r->maxJamoExpansions->maxLSize = t->maxJamoExpansions->maxLSize;
        r->maxJamoExpansions->maxVSize = t->maxJamoExpansions->maxVSize;
        r->maxJamoExpansions->maxTSize = t->maxJamoExpansions->maxTSize;
        if(t->maxJamoExpansions->size != 0) {
            r->maxJamoExpansions->endExpansionCE = (uint32_t *)uprv_malloc(sizeof(uint32_t)*t->maxJamoExpansions->size);
            /* test for NULL */
            if (r->maxJamoExpansions->endExpansionCE == NULL) {
                *status = U_MEMORY_ALLOCATION_ERROR;
                goto cleanup;
            }
            uprv_memcpy(r->maxJamoExpansions->endExpansionCE, t->maxJamoExpansions->endExpansionCE, t->maxJamoExpansions->position*sizeof(uint32_t));
            r->maxJamoExpansions->isV = (UBool *)uprv_malloc(sizeof(UBool)*t->maxJamoExpansions->size);
            /* test for NULL */
            if (r->maxJamoExpansions->isV == NULL) {
                *status = U_MEMORY_ALLOCATION_ERROR;
                goto cleanup;
            }
            uprv_memcpy(r->maxJamoExpansions->isV, t->maxJamoExpansions->isV, t->maxJamoExpansions->position*sizeof(UBool));
        } else {
            r->maxJamoExpansions->endExpansionCE = NULL;
            r->maxJamoExpansions->isV = NULL;
        }
    }

    if(t->unsafeCP != NULL) {
        r->unsafeCP = (uint8_t *)uprv_malloc(UCOL_UNSAFECP_TABLE_SIZE);
        /* test for NULL */
        if (r->unsafeCP == NULL) {
            *status = U_MEMORY_ALLOCATION_ERROR;
            goto cleanup;
        }
        uprv_memcpy(r->unsafeCP, t->unsafeCP, UCOL_UNSAFECP_TABLE_SIZE);
    }

    if(t->contrEndCP != NULL) {
        r->contrEndCP = (uint8_t *)uprv_malloc(UCOL_UNSAFECP_TABLE_SIZE);
        /* test for NULL */
        if (r->contrEndCP == NULL) {
            *status = U_MEMORY_ALLOCATION_ERROR;
            goto cleanup;
        }
        uprv_memcpy(r->contrEndCP, t->contrEndCP, UCOL_UNSAFECP_TABLE_SIZE);
    }

    r->UCA = t->UCA;
    r->image = t->image;
    r->options = t->options;

    return r;
cleanup:
    uprv_uca_closeTempTable(t);
    return NULL;
}


U_CAPI void  U_EXPORT2
uprv_uca_closeTempTable(tempUCATable *t) {
    if(t != NULL) {
        if (t->expansions != NULL) {
            uprv_free(t->expansions->CEs);
            uprv_free(t->expansions);
        }
        if(t->contractions != NULL) {
            uprv_cnttab_close(t->contractions);
        }
        if (t->mapping != NULL) {
            utrie_close(t->mapping);
        }

        if(t->prefixLookup != NULL) {
            uhash_close(t->prefixLookup);
        }

        if (t->maxExpansions != NULL) {
            uprv_free(t->maxExpansions->endExpansionCE);
            uprv_free(t->maxExpansions->expansionCESize);
            uprv_free(t->maxExpansions);
        }

        if (t->maxJamoExpansions->size > 0) {
            uprv_free(t->maxJamoExpansions->endExpansionCE);
            uprv_free(t->maxJamoExpansions->isV);
        }
        uprv_free(t->maxJamoExpansions);

        uprv_free(t->unsafeCP);
        uprv_free(t->contrEndCP);
        
        if (t->cmLookup != NULL) {
            uprv_free(t->cmLookup->cPoints);
            uprv_free(t->cmLookup);
        }

        uprv_free(t);
    }
}

/**
* Looks for the maximum length of all expansion sequences ending with the same
* collation element. The size required for maxexpansion and maxsize is 
* returned if the arrays are too small.
* @param endexpansion the last expansion collation element to be added
* @param expansionsize size of the expansion
* @param maxexpansion data structure to store the maximum expansion data.
* @param status error status
* @returns size of the maxexpansion and maxsize used.
*/
static int uprv_uca_setMaxExpansion(uint32_t           endexpansion,
                                    uint8_t            expansionsize,
                                    MaxExpansionTable *maxexpansion,
                                    UErrorCode        *status)
{
    if (maxexpansion->size == 0) {
        /* we'll always make the first element 0, for easier manipulation */
        maxexpansion->endExpansionCE = 
            (uint32_t *)uprv_malloc(INIT_EXP_TABLE_SIZE * sizeof(int32_t));
        /* test for NULL */
        if (maxexpansion->endExpansionCE == NULL) {
            *status = U_MEMORY_ALLOCATION_ERROR;
            return 0;
        }
        *(maxexpansion->endExpansionCE) = 0;
        maxexpansion->expansionCESize =
            (uint8_t *)uprv_malloc(INIT_EXP_TABLE_SIZE * sizeof(uint8_t));
        /* test for NULL */;
        if (maxexpansion->expansionCESize == NULL) {
            *status = U_MEMORY_ALLOCATION_ERROR;
            return 0;
        }
        *(maxexpansion->expansionCESize) = 0;
        maxexpansion->size     = INIT_EXP_TABLE_SIZE;
        maxexpansion->position = 0;
    }

    if (maxexpansion->position + 1 == maxexpansion->size) {
        uint32_t *neweece = (uint32_t *)uprv_realloc(maxexpansion->endExpansionCE, 
            2 * maxexpansion->size * sizeof(uint32_t));
        if (neweece == NULL) {
            *status = U_MEMORY_ALLOCATION_ERROR;
            return 0;
        }
        maxexpansion->endExpansionCE  = neweece;

        uint8_t  *neweces = (uint8_t *)uprv_realloc(maxexpansion->expansionCESize, 
            2 * maxexpansion->size * sizeof(uint8_t));
        if (neweces == NULL) {
            *status = U_MEMORY_ALLOCATION_ERROR;
            return 0;
        }
        maxexpansion->expansionCESize = neweces;
        maxexpansion->size *= 2;
    }

    uint32_t *pendexpansionce = maxexpansion->endExpansionCE;
    uint8_t  *pexpansionsize  = maxexpansion->expansionCESize;
    int      pos              = maxexpansion->position;

    uint32_t *start = pendexpansionce;
    uint32_t *limit = pendexpansionce + pos;

    /* using binary search to determine if last expansion element is
    already in the array */
    uint32_t *mid;
    int       result = -1;
    while (start < limit - 1) {
        mid = start + ((limit - start) >> 1);
        if (endexpansion <= *mid) {
            limit = mid;
        }
        else {
            start = mid;
        }
    }

    if (*start == endexpansion) {
        result = (int)(start - pendexpansionce);
    }
    else if (*limit == endexpansion) {
        result = (int)(limit - pendexpansionce);
    }

    if (result > -1) {
        /* found the ce in expansion, we'll just modify the size if it is
        smaller */
        uint8_t *currentsize = pexpansionsize + result;
        if (*currentsize < expansionsize) {
            *currentsize = expansionsize;
        }
    }
    else {
        /* we'll need to squeeze the value into the array.
        initial implementation. */
        /* shifting the subarray down by 1 */
        int      shiftsize     = (int)((pendexpansionce + pos) - start);
        uint32_t *shiftpos     = start + 1;
        uint8_t  *sizeshiftpos = pexpansionsize + (shiftpos - pendexpansionce);

        /* okay need to rearrange the array into sorted order */
        if (shiftsize == 0 /*|| *(pendexpansionce + pos) < endexpansion*/) { /* the commented part is actually both redundant and dangerous */
            *(pendexpansionce + pos + 1) = endexpansion;
            *(pexpansionsize + pos + 1)  = expansionsize;
        }
        else {
            uprv_memmove(shiftpos + 1, shiftpos, shiftsize * sizeof(int32_t));
            uprv_memmove(sizeshiftpos + 1, sizeshiftpos, 
                shiftsize * sizeof(uint8_t));
            *shiftpos     = endexpansion;
            *sizeshiftpos = expansionsize;
        }
        maxexpansion->position ++;

#ifdef UCOL_DEBUG
        int   temp;
        UBool found = FALSE;
        for (temp = 0; temp < maxexpansion->position; temp ++) {
            if (pendexpansionce[temp] >= pendexpansionce[temp + 1]) {
                fprintf(stderr, "expansions %d\n", temp);
            }
            if (pendexpansionce[temp] == endexpansion) {
                found =TRUE;
                if (pexpansionsize[temp] < expansionsize) {
                    fprintf(stderr, "expansions size %d\n", temp);
                }
            }
        }
        if (pendexpansionce[temp] == endexpansion) {
            found =TRUE;
            if (pexpansionsize[temp] < expansionsize) {
                fprintf(stderr, "expansions size %d\n", temp);
            }
        }
        if (!found)
            fprintf(stderr, "expansion not found %d\n", temp);
#endif
    }

    return maxexpansion->position;
}

/**
* Sets the maximum length of all jamo expansion sequences ending with the same
* collation element. The size required for maxexpansion and maxsize is 
* returned if the arrays are too small.
* @param ch the jamo codepoint
* @param endexpansion the last expansion collation element to be added
* @param expansionsize size of the expansion
* @param maxexpansion data structure to store the maximum expansion data.
* @param status error status
* @returns size of the maxexpansion and maxsize used.
*/
static int uprv_uca_setMaxJamoExpansion(UChar                  ch,
                                        uint32_t               endexpansion,
                                        uint8_t                expansionsize,
                                        MaxJamoExpansionTable *maxexpansion,
                                        UErrorCode            *status)
{
    UBool isV = TRUE;
    if (((uint32_t)ch - 0x1100) <= (0x1112 - 0x1100)) {
        /* determines L for Jamo, doesn't need to store this since it is never
        at the end of a expansion */
        if (maxexpansion->maxLSize < expansionsize) {
            maxexpansion->maxLSize = expansionsize;
        }
        return maxexpansion->position;
    }

    if (((uint32_t)ch - 0x1161) <= (0x1175 - 0x1161)) {
        /* determines V for Jamo */
        if (maxexpansion->maxVSize < expansionsize) {
            maxexpansion->maxVSize = expansionsize;
        }
    }

    if (((uint32_t)ch - 0x11A8) <= (0x11C2 - 0x11A8)) {
        isV = FALSE;
        /* determines T for Jamo */
        if (maxexpansion->maxTSize < expansionsize) {
            maxexpansion->maxTSize = expansionsize;
        }
    }

    if (maxexpansion->size == 0) {
        /* we'll always make the first element 0, for easier manipulation */
        maxexpansion->endExpansionCE = 
            (uint32_t *)uprv_malloc(INIT_EXP_TABLE_SIZE * sizeof(uint32_t));
        /* test for NULL */;
        if (maxexpansion->endExpansionCE == NULL) {
            *status = U_MEMORY_ALLOCATION_ERROR;
            return 0;
        }
        *(maxexpansion->endExpansionCE) = 0;
        maxexpansion->isV = 
            (UBool *)uprv_malloc(INIT_EXP_TABLE_SIZE * sizeof(UBool));
        /* test for NULL */;
        if (maxexpansion->isV == NULL) {
            *status = U_MEMORY_ALLOCATION_ERROR;
            uprv_free(maxexpansion->endExpansionCE);
            maxexpansion->endExpansionCE = NULL;
            return 0;
        }
        *(maxexpansion->isV) = 0;
        maxexpansion->size     = INIT_EXP_TABLE_SIZE;
        maxexpansion->position = 0;
    }

    if (maxexpansion->position + 1 == maxexpansion->size) {
        maxexpansion->size *= 2;
        maxexpansion->endExpansionCE = (uint32_t *)uprv_realloc(maxexpansion->endExpansionCE, 
            maxexpansion->size * sizeof(uint32_t));
        if (maxexpansion->endExpansionCE == NULL) {
#ifdef UCOL_DEBUG
            fprintf(stderr, "out of memory for maxExpansions\n");
#endif
            *status = U_MEMORY_ALLOCATION_ERROR;
            return 0;
        }
        maxexpansion->isV  = (UBool *)uprv_realloc(maxexpansion->isV, 
            maxexpansion->size * sizeof(UBool));
        if (maxexpansion->isV == NULL) {
#ifdef UCOL_DEBUG
            fprintf(stderr, "out of memory for maxExpansions\n");
#endif
            *status = U_MEMORY_ALLOCATION_ERROR;
            uprv_free(maxexpansion->endExpansionCE);
            maxexpansion->endExpansionCE = NULL;
            return 0;
        }
    }

    uint32_t *pendexpansionce = maxexpansion->endExpansionCE;
    int       pos             = maxexpansion->position;

    while (pos > 0) {
        pos --;
        if (*(pendexpansionce + pos) == endexpansion) {
            return maxexpansion->position;
        }
    }

    *(pendexpansionce + maxexpansion->position) = endexpansion;
    *(maxexpansion->isV + maxexpansion->position) = isV;
    maxexpansion->position ++;

    return maxexpansion->position;
}


static void ContrEndCPSet(uint8_t *table, UChar c) {
    uint32_t    hash;
    uint8_t     *htByte;

    hash = c;
    if (hash >= UCOL_UNSAFECP_TABLE_SIZE*8) {
        hash = (hash & UCOL_UNSAFECP_TABLE_MASK) + 256;
    }
    htByte = &table[hash>>3];
    *htByte |= (1 << (hash & 7));
}


static void unsafeCPSet(uint8_t *table, UChar c) {
    uint32_t    hash;
    uint8_t     *htByte;

    hash = c;
    if (hash >= UCOL_UNSAFECP_TABLE_SIZE*8) {
        if (hash >= 0xd800 && hash <= 0xf8ff) {
            /*  Part of a surrogate, or in private use area.            */
            /*   These don't go in the table                            */
            return;
        }
        hash = (hash & UCOL_UNSAFECP_TABLE_MASK) + 256;
    }
    htByte = &table[hash>>3];
    *htByte |= (1 << (hash & 7));
}

static void
uprv_uca_createCMTable(tempUCATable *t, int32_t noOfCM, UErrorCode *status) {
    t->cmLookup = (CombinClassTable *)uprv_malloc(sizeof(CombinClassTable));
    if (t->cmLookup==NULL) {
        *status = U_MEMORY_ALLOCATION_ERROR;
        return;
    }
    t->cmLookup->cPoints=(UChar *)uprv_malloc(noOfCM*sizeof(UChar));
    if (t->cmLookup->cPoints ==NULL) {
        uprv_free(t->cmLookup);
        t->cmLookup = NULL;
        *status = U_MEMORY_ALLOCATION_ERROR;
        return;
    }

    t->cmLookup->size=noOfCM;
    uprv_memset(t->cmLookup->index, 0, sizeof(t->cmLookup->index));

    return;
}

static void
uprv_uca_copyCMTable(tempUCATable *t, UChar *cm, uint16_t *index) {
    int32_t count=0;

    for (int32_t i=0; i<256; ++i) {
        if (index[i]>0) {
            // cPoints is ordered by combining class value.
            uprv_memcpy(t->cmLookup->cPoints+count, cm+(i<<8), index[i]*sizeof(UChar));
            count += index[i];
        }
        t->cmLookup->index[i]=count;
    }
    return;
}

/* 1. to the UnsafeCP hash table, add all chars with combining class != 0     */
/* 2. build combining marks table for all chars with combining class != 0     */
static void uprv_uca_unsafeCPAddCCNZ(tempUCATable *t, UErrorCode *status) {

    UChar              c;
    uint16_t           fcd;     // Hi byte is lead combining class.
    // lo byte is trailing combing class.
    const uint16_t    *fcdTrieIndex;
    UChar32            fcdHighStart;
    UBool buildCMTable = (t->cmLookup==NULL); // flag for building combining class table
    UChar *cm=NULL;
    uint16_t index[256];
    int32_t count=0;
    fcdTrieIndex = unorm_getFCDTrieIndex(fcdHighStart, status);
    if (U_FAILURE(*status)) {
        return;
    }

    if (buildCMTable) {
        if (cm==NULL) {
            cm = (UChar *)uprv_malloc(sizeof(UChar)*UCOL_MAX_CM_TAB);
            if (cm==NULL) {
                *status = U_MEMORY_ALLOCATION_ERROR;
                return;
            }
        }
        uprv_memset(index, 0, sizeof(index));
    }
    for (c=0; c<0xffff; c++) {
        fcd = unorm_getFCD16(fcdTrieIndex, c);
        if (fcd >= 0x100 ||               // if the leading combining class(c) > 0 ||
            (UTF_IS_LEAD(c) && fcd != 0)) {//    c is a leading surrogate with some FCD data
            if (buildCMTable) {
                uint32_t cClass = fcd & 0xff;
                //uint32_t temp=(cClass<<8)+index[cClass];
                cm[(cClass<<8)+index[cClass]] = c; //
                index[cClass]++;
                count++;
            }
            unsafeCPSet(t->unsafeCP, c);
        }
    }

    // copy to cm table
    if (buildCMTable) {
        uprv_uca_createCMTable(t, count, status);
        if(U_FAILURE(*status)) {
            if (cm!=NULL) {
                uprv_free(cm);
            }
            return;
        }
        uprv_uca_copyCMTable(t, cm, index);
    }

    if(t->prefixLookup != NULL) {
        int32_t i = -1;
        const UHashElement *e = NULL;
        UCAElements *element = NULL;
        UChar NFCbuf[256];
        uint32_t NFCbufLen = 0;
        while((e = uhash_nextElement(t->prefixLookup, &i)) != NULL) {
            element = (UCAElements *)e->value.pointer;
            // codepoints here are in the NFD form. We need to add the
            // first code point of the NFC form to unsafe, because
            // strcoll needs to backup over them.
            NFCbufLen = unorm_normalize(element->cPoints, element->cSize, UNORM_NFC, 0,
                NFCbuf, 256, status);
            unsafeCPSet(t->unsafeCP, NFCbuf[0]);
        }
    }

    if (cm!=NULL) {
        uprv_free(cm);
    }
}

static uint32_t uprv_uca_addPrefix(tempUCATable *t, uint32_t CE,
                                   UCAElements *element, UErrorCode *status)
{
    // currently the longest prefix we're supporting in Japanese is two characters
    // long. Although this table could quite easily mimic complete contraction stuff
    // there is no good reason to make a general solution, as it would require some
    // error prone messing.
    CntTable *contractions = t->contractions;
    UChar32 cp;
    uint32_t cpsize = 0;
    UChar *oldCP = element->cPoints;
    uint32_t oldCPSize = element->cSize;


    contractions->currentTag = SPEC_PROC_TAG;

    // here, we will normalize & add prefix to the table.
    uint32_t j = 0;
#ifdef UCOL_DEBUG
    for(j=0; j<element->cSize; j++) {
        fprintf(stdout, "CP: %04X ", element->cPoints[j]);
    }
    fprintf(stdout, "El: %08X Pref: ", CE);
    for(j=0; j<element->prefixSize; j++) {
        fprintf(stdout, "%04X ", element->prefix[j]);
    }
    fprintf(stdout, "%08X ", element->mapCE);
#endif

    for (j = 1; j<element->prefixSize; j++) {   /* First add NFD prefix chars to unsafe CP hash table */
        // Unless it is a trail surrogate, which is handled algoritmically and
        // shouldn't take up space in the table.
        if(!(UTF_IS_TRAIL(element->prefix[j]))) {
            unsafeCPSet(t->unsafeCP, element->prefix[j]);
        }
    }

    UChar tempPrefix = 0;

    for(j = 0; j < /*nfcSize*/element->prefixSize/2; j++) { // prefixes are going to be looked up backwards
        // therefore, we will promptly reverse the prefix buffer...
        tempPrefix = *(/*nfcBuffer*/element->prefix+element->prefixSize-j-1);
        *(/*nfcBuffer*/element->prefix+element->prefixSize-j-1) = element->prefix[j];
        element->prefix[j] = tempPrefix;
    }

#ifdef UCOL_DEBUG
    fprintf(stdout, "Reversed: ");
    for(j=0; j<element->prefixSize; j++) {
        fprintf(stdout, "%04X ", element->prefix[j]);
    }
    fprintf(stdout, "%08X\n", element->mapCE);
#endif

    // the first codepoint is also unsafe, as it forms a 'contraction' with the prefix
    if(!(UTF_IS_TRAIL(element->cPoints[0]))) {
        unsafeCPSet(t->unsafeCP, element->cPoints[0]);
    }

    // Maybe we need this... To handle prefixes completely in the forward direction...
    //if(element->cSize == 1) {
    //  if(!(UTF_IS_TRAIL(element->cPoints[0]))) {
    //    ContrEndCPSet(t->contrEndCP, element->cPoints[0]);
    //  }
    //}

    element->cPoints = element->prefix;
    element->cSize = element->prefixSize;

    // Add the last char of the contraction to the contraction-end hash table.
    // unless it is a trail surrogate, which is handled algorithmically and
    // shouldn't be in the table
    if(!(UTF_IS_TRAIL(element->cPoints[element->cSize -1]))) {
        ContrEndCPSet(t->contrEndCP, element->cPoints[element->cSize -1]);
    }

    // First we need to check if contractions starts with a surrogate
    UTF_NEXT_CHAR(element->cPoints, cpsize, element->cSize, cp);

    // If there are any Jamos in the contraction, we should turn on special
    // processing for Jamos
    if(UCOL_ISJAMO(element->prefix[0])) {
        t->image->jamoSpecial = TRUE;
    }
    /* then we need to deal with it */
    /* we could aready have something in table - or we might not */

    if(!isPrefix(CE)) {
        /* if it wasn't contraction, we wouldn't end up here*/
        int32_t firstContractionOffset = 0;
        firstContractionOffset = uprv_cnttab_addContraction(contractions, UPRV_CNTTAB_NEWELEMENT, 0, CE, status);
        uint32_t newCE = uprv_uca_processContraction(contractions, element, UCOL_NOT_FOUND, status);
        uprv_cnttab_addContraction(contractions, firstContractionOffset, *element->prefix, newCE, status);
        uprv_cnttab_addContraction(contractions, firstContractionOffset, 0xFFFF, CE, status);
        CE =  constructContractCE(SPEC_PROC_TAG, firstContractionOffset);
    } else { /* we are adding to existing contraction */
        /* there were already some elements in the table, so we need to add a new contraction */
        /* Two things can happen here: either the codepoint is already in the table, or it is not */
        int32_t position = uprv_cnttab_findCP(contractions, CE, *element->prefix, status);
        if(position > 0) {       /* if it is we just continue down the chain */
            uint32_t eCE = uprv_cnttab_getCE(contractions, CE, position, status);
            uint32_t newCE = uprv_uca_processContraction(contractions, element, eCE, status);
            uprv_cnttab_setContraction(contractions, CE, position, *(element->prefix), newCE, status);
        } else {                  /* if it isn't, we will have to create a new sequence */
            uprv_uca_processContraction(contractions, element, UCOL_NOT_FOUND, status);
            uprv_cnttab_insertContraction(contractions, CE, *(element->prefix), element->mapCE, status);
        }
    }

    element->cPoints = oldCP;
    element->cSize = oldCPSize;

    return CE;
}

// Note regarding surrogate handling: We are interested only in the single
// or leading surrogates in a contraction. If a surrogate is somewhere else
// in the contraction, it is going to be handled as a pair of code units,
// as it doesn't affect the performance AND handling surrogates specially
// would complicate code way too much.
static uint32_t uprv_uca_addContraction(tempUCATable *t, uint32_t CE, 
                                        UCAElements *element, UErrorCode *status)
{
    CntTable *contractions = t->contractions;
    UChar32 cp;
    uint32_t cpsize = 0;

    contractions->currentTag = CONTRACTION_TAG;

    // First we need to check if contractions starts with a surrogate
    UTF_NEXT_CHAR(element->cPoints, cpsize, element->cSize, cp);

    if(cpsize<element->cSize) { // This is a real contraction, if there are other characters after the first
        uint32_t j = 0;
        for (j=1; j<element->cSize; j++) {   /* First add contraction chars to unsafe CP hash table */
            // Unless it is a trail surrogate, which is handled algoritmically and 
            // shouldn't take up space in the table.
            if(!(UTF_IS_TRAIL(element->cPoints[j]))) {
                unsafeCPSet(t->unsafeCP, element->cPoints[j]);
            }
        }
        // Add the last char of the contraction to the contraction-end hash table.
        // unless it is a trail surrogate, which is handled algorithmically and 
        // shouldn't be in the table
        if(!(UTF_IS_TRAIL(element->cPoints[element->cSize -1]))) {
            ContrEndCPSet(t->contrEndCP, element->cPoints[element->cSize -1]);
        }

        // If there are any Jamos in the contraction, we should turn on special 
        // processing for Jamos
        if(UCOL_ISJAMO(element->cPoints[0])) {
            t->image->jamoSpecial = TRUE;
        }
        /* then we need to deal with it */
        /* we could aready have something in table - or we might not */
        element->cPoints+=cpsize;
        element->cSize-=cpsize;
        if(!isContraction(CE)) { 
            /* if it wasn't contraction, we wouldn't end up here*/
            int32_t firstContractionOffset = 0;
            firstContractionOffset = uprv_cnttab_addContraction(contractions, UPRV_CNTTAB_NEWELEMENT, 0, CE, status);
            uint32_t newCE = uprv_uca_processContraction(contractions, element, UCOL_NOT_FOUND, status);
            uprv_cnttab_addContraction(contractions, firstContractionOffset, *element->cPoints, newCE, status);
            uprv_cnttab_addContraction(contractions, firstContractionOffset, 0xFFFF, CE, status);
            CE =  constructContractCE(CONTRACTION_TAG, firstContractionOffset);
        } else { /* we are adding to existing contraction */
            /* there were already some elements in the table, so we need to add a new contraction */
            /* Two things can happen here: either the codepoint is already in the table, or it is not */
            int32_t position = uprv_cnttab_findCP(contractions, CE, *element->cPoints, status);
            if(position > 0) {       /* if it is we just continue down the chain */
                uint32_t eCE = uprv_cnttab_getCE(contractions, CE, position, status);
                uint32_t newCE = uprv_uca_processContraction(contractions, element, eCE, status);
                uprv_cnttab_setContraction(contractions, CE, position, *(element->cPoints), newCE, status);
            } else {                  /* if it isn't, we will have to create a new sequence */
                uint32_t newCE = uprv_uca_processContraction(contractions, element, UCOL_NOT_FOUND, status);
                uprv_cnttab_insertContraction(contractions, CE, *(element->cPoints), newCE, status);
            }
        }
        element->cPoints-=cpsize;
        element->cSize+=cpsize;
        /*ucmpe32_set(t->mapping, cp, CE);*/
        utrie_set32(t->mapping, cp, CE);
    } else if(!isContraction(CE)) { /* this is just a surrogate, and there is no contraction */
        /*ucmpe32_set(t->mapping, cp, element->mapCE);*/
        utrie_set32(t->mapping, cp, element->mapCE);
    } else { /* fill out the first stage of the contraction with the surrogate CE */
        uprv_cnttab_changeContraction(contractions, CE, 0, element->mapCE, status);
        uprv_cnttab_changeContraction(contractions, CE, 0xFFFF, element->mapCE, status);
    }
    return CE;
}


static uint32_t uprv_uca_processContraction(CntTable *contractions, UCAElements *element, uint32_t existingCE, UErrorCode *status) {
    int32_t firstContractionOffset = 0;
    //    uint32_t contractionElement = UCOL_NOT_FOUND;

    if(U_FAILURE(*status)) {
        return UCOL_NOT_FOUND;
    }

    /* end of recursion */
    if(element->cSize == 1) {
        if(isCntTableElement(existingCE) && ((UColCETags)getCETag(existingCE) == contractions->currentTag)) {
            uprv_cnttab_changeContraction(contractions, existingCE, 0, element->mapCE, status);
            uprv_cnttab_changeContraction(contractions, existingCE, 0xFFFF, element->mapCE, status);
            return existingCE;
        } else {
            return element->mapCE; /*can't do just that. existingCe might be a contraction, meaning that we need to do another step */
        }
    }

    /* this recursion currently feeds on the only element we have... We will have to copy it in order to accomodate */
    /* for both backward and forward cycles */

    /* we encountered either an empty space or a non-contraction element */
    /* this means we are constructing a new contraction sequence */
    element->cPoints++;
    element->cSize--;
    if(!isCntTableElement(existingCE)) { 
        /* if it wasn't contraction, we wouldn't end up here*/
        firstContractionOffset = uprv_cnttab_addContraction(contractions, UPRV_CNTTAB_NEWELEMENT, 0, existingCE, status);
        uint32_t newCE = uprv_uca_processContraction(contractions, element, UCOL_NOT_FOUND, status);
        uprv_cnttab_addContraction(contractions, firstContractionOffset, *element->cPoints, newCE, status);
        uprv_cnttab_addContraction(contractions, firstContractionOffset, 0xFFFF, existingCE, status);
        existingCE =  constructContractCE(contractions->currentTag, firstContractionOffset);
    } else { /* we are adding to existing contraction */
        /* there were already some elements in the table, so we need to add a new contraction */
        /* Two things can happen here: either the codepoint is already in the table, or it is not */
        int32_t position = uprv_cnttab_findCP(contractions, existingCE, *element->cPoints, status);
        if(position > 0) {       /* if it is we just continue down the chain */
            uint32_t eCE = uprv_cnttab_getCE(contractions, existingCE, position, status);
            uint32_t newCE = uprv_uca_processContraction(contractions, element, eCE, status);
            uprv_cnttab_setContraction(contractions, existingCE, position, *(element->cPoints), newCE, status);
        } else {                  /* if it isn't, we will have to create a new sequence */
            uint32_t newCE = uprv_uca_processContraction(contractions, element, UCOL_NOT_FOUND, status);
            uprv_cnttab_insertContraction(contractions, existingCE, *(element->cPoints), newCE, status);
        }
    }
    element->cPoints--;
    element->cSize++;
    return existingCE;
}

static uint32_t uprv_uca_finalizeAddition(tempUCATable *t, UCAElements *element, UErrorCode *status) {
    uint32_t CE = UCOL_NOT_FOUND;
    // This should add a completely ignorable element to the 
    // unsafe table, so that backward iteration will skip
    // over it when treating contractions.
    uint32_t i = 0;
    if(element->mapCE == 0) {
        for(i = 0; i < element->cSize; i++) {
            if(!UTF_IS_TRAIL(element->cPoints[i])) {
                unsafeCPSet(t->unsafeCP, element->cPoints[i]);
            }
        }
    }
    if(element->cSize > 1) { /* we're adding a contraction */
        uint32_t i = 0;
        UChar32 cp;

        UTF_NEXT_CHAR(element->cPoints, i, element->cSize, cp);
        /*CE = ucmpe32_get(t->mapping, cp);*/
        CE = utrie_get32(t->mapping, cp, NULL);

        CE = uprv_uca_addContraction(t, CE, element, status);
    } else { /* easy case, */
        /*CE = ucmpe32_get(t->mapping, element->cPoints[0]);*/
        CE = utrie_get32(t->mapping, element->cPoints[0], NULL);

        if( CE != UCOL_NOT_FOUND) {
            if(isCntTableElement(CE) /*isContraction(CE)*/) { /* adding a non contraction element (thai, expansion, single) to already existing contraction */
                if(!isPrefix(element->mapCE)) { // we cannot reenter prefix elements - as we are going to create a dead loop
                    // Only expansions and regular CEs can go here... Contractions will never happen in this place
                    uprv_cnttab_setContraction(t->contractions, CE, 0, 0, element->mapCE, status);
                    /* This loop has to change the CE at the end of contraction REDO!*/
                    uprv_cnttab_changeLastCE(t->contractions, CE, element->mapCE, status);
                }
            } else {
                /*ucmpe32_set(t->mapping, element->cPoints[0], element->mapCE);*/
                utrie_set32(t->mapping, element->cPoints[0], element->mapCE);
                if ((element->prefixSize!=0) && (!isSpecial(CE) || (getCETag(CE)!=IMPLICIT_TAG))) {
                    UCAElements *origElem = (UCAElements *)uprv_malloc(sizeof(UCAElements));
                    /* test for NULL */
                    if (origElem== NULL) {
                        *status = U_MEMORY_ALLOCATION_ERROR;
                        return 0;
                    }
                    /* copy the original UCA value */
                    origElem->prefixSize = 0;
                    origElem->prefix = NULL;
                    origElem->cPoints = origElem->uchars;
                    origElem->cPoints[0] = element->cPoints[0];
                    origElem->cSize = 1;
                    origElem->CEs[0]=CE;
                    origElem->mapCE=CE;
                    origElem->noOfCEs=1;
                    uprv_uca_finalizeAddition(t, origElem, status);
                    uprv_free(origElem);
                }
#ifdef UCOL_DEBUG
                fprintf(stderr, "Warning - trying to overwrite existing data %08X for cp %04X with %08X\n", CE, element->cPoints[0], element->CEs[0]);
                //*status = U_ILLEGAL_ARGUMENT_ERROR;
#endif
            }
        } else {
            /*ucmpe32_set(t->mapping, element->cPoints[0], element->mapCE);*/
            utrie_set32(t->mapping, element->cPoints[0], element->mapCE);
        }
    }
    return CE;
}

/* This adds a read element, while testing for existence */
U_CAPI uint32_t  U_EXPORT2
uprv_uca_addAnElement(tempUCATable *t, UCAElements *element, UErrorCode *status) {
    U_NAMESPACE_USE

    ExpansionTable *expansions = t->expansions;

    uint32_t i = 1;
    uint32_t expansion = 0;
    uint32_t CE;

    if(U_FAILURE(*status)) {
        return 0xFFFF;
    }

    element->mapCE = 0; // clear mapCE so that we can catch expansions

    if(element->noOfCEs == 1) {
        element->mapCE = element->CEs[0];      
    } else {     
        /* ICU 2.1 long primaries */
        /* unfortunately, it looks like we have to look for a long primary here */
        /* since in canonical closure we are going to hit some long primaries from */
        /* the first phase, and they will come back as continuations/expansions */
        /* destroying the effect of the previous opitimization */
        /* A long primary is a three byte primary with starting secondaries and tertiaries */
        /* It can appear in long runs of only primary differences (like east Asian tailorings) */
        /* also, it should not be an expansion, as expansions would break with this */
        // This part came in from ucol_bld.cpp
        //if(tok->expansion == 0
        //&& noOfBytes[0] == 3 && noOfBytes[1] == 1 && noOfBytes[2] == 1
        //&& CEparts[1] == (UCOL_BYTE_COMMON << 24) && CEparts[2] == (UCOL_BYTE_COMMON << 24)) {
        /* we will construct a special CE that will go unchanged to the table */
        if(element->noOfCEs == 2 // a two CE expansion 
            && isContinuation(element->CEs[1]) // which  is a continuation
            && (element->CEs[1] & (~(0xFF << 24 | UCOL_CONTINUATION_MARKER))) == 0 // that has only primaries in continuation,
            && (((element->CEs[0]>>8) & 0xFF) == UCOL_BYTE_COMMON) // a common secondary
            && ((element->CEs[0] & 0xFF) == UCOL_BYTE_COMMON) // and a common tertiary
            )
        {
#ifdef UCOL_DEBUG
            fprintf(stdout, "Long primary %04X\n", element->cPoints[0]);
#endif
            element->mapCE = UCOL_SPECIAL_FLAG | (LONG_PRIMARY_TAG<<24) // a long primary special
                | ((element->CEs[0]>>8) & 0xFFFF00) // first and second byte of primary
                | ((element->CEs[1]>>24) & 0xFF);   // third byte of primary
        }
        else {
            expansion = (uint32_t)(UCOL_SPECIAL_FLAG | (EXPANSION_TAG<<UCOL_TAG_SHIFT) 
                | (((uprv_uca_addExpansion(expansions, element->CEs[0], status)+(headersize>>2))<<4)
                   & 0xFFFFF0));

            for(i = 1; i<element->noOfCEs; i++) {
                uprv_uca_addExpansion(expansions, element->CEs[i], status);
            }
            if(element->noOfCEs <= 0xF) {
                expansion |= element->noOfCEs;
            } else {
                uprv_uca_addExpansion(expansions, 0, status);
            }
            element->mapCE = expansion;
            uprv_uca_setMaxExpansion(element->CEs[element->noOfCEs - 1],
                (uint8_t)element->noOfCEs,
                t->maxExpansions,
                status);
            if(UCOL_ISJAMO(element->cPoints[0])) {
                t->image->jamoSpecial = TRUE;
                uprv_uca_setMaxJamoExpansion(element->cPoints[0],
                    element->CEs[element->noOfCEs - 1],
                    (uint8_t)element->noOfCEs,
                    t->maxJamoExpansions,
                    status);
            }
            if (U_FAILURE(*status)) {
                return 0;
            }
        }
    }

    // We treat digits differently - they are "uber special" and should be
    // processed differently if numeric collation is on. 
    UChar32 uniChar = 0;
    //printElement(element);
    if ((element->cSize == 2) && U16_IS_LEAD(element->cPoints[0])){
        uniChar = U16_GET_SUPPLEMENTARY(element->cPoints[0], element->cPoints[1]);
    } else if (element->cSize == 1){
        uniChar = element->cPoints[0];
    }

    // Here, we either have one normal CE OR mapCE is set. Therefore, we stuff only
    // one element to the expansion buffer. When we encounter a digit and we don't 
    // do numeric collation, we will just pick the CE we have and break out of case
    // (see ucol.cpp ucol_prv_getSpecialCE && ucol_prv_getSpecialPrevCE). If we picked
    // a special, further processing will occur. If it's a simple CE, we'll return due
    // to how the loop is constructed.
    if (uniChar != 0 && u_isdigit(uniChar)){
        expansion = (uint32_t)(UCOL_SPECIAL_FLAG | (DIGIT_TAG<<UCOL_TAG_SHIFT) | 1); // prepare the element
        if(element->mapCE) { // if there is an expansion, we'll pick it here
            expansion |= ((uprv_uca_addExpansion(expansions, element->mapCE, status)+(headersize>>2))<<4);
        } else {
            expansion |= ((uprv_uca_addExpansion(expansions, element->CEs[0], status)+(headersize>>2))<<4);
        }
        element->mapCE = expansion;

        // Need to go back to the beginning of the digit string if in the middle!
        if(uniChar <= 0xFFFF) { // supplementaries are always unsafe. API takes UChars
            unsafeCPSet(t->unsafeCP, (UChar)uniChar);
        }
    }

    // here we want to add the prefix structure.
    // I will try to process it as a reverse contraction, if possible.
    // prefix buffer is already reversed.

    if(element->prefixSize!=0) {
        // We keep the seen prefix starter elements in a hashtable
        // we need it to be able to distinguish between the simple
        // codepoints and prefix starters. Also, we need to use it
        // for canonical closure.

        UCAElements *composed = (UCAElements *)uprv_malloc(sizeof(UCAElements));
        /* test for NULL */
        if (composed == NULL) {
            *status = U_MEMORY_ALLOCATION_ERROR;
            return 0;
        }
        uprv_memcpy(composed, element, sizeof(UCAElements));
        composed->cPoints = composed->uchars;
        composed->prefix = composed->prefixChars;

        composed->prefixSize = unorm_normalize(element->prefix, element->prefixSize, UNORM_NFC, 0, composed->prefix, 128, status);


        if(t->prefixLookup != NULL) {
            UCAElements *uCE = (UCAElements *)uhash_get(t->prefixLookup, element);
            if(uCE != NULL) { // there is already a set of code points here
                element->mapCE = uprv_uca_addPrefix(t, uCE->mapCE, element, status);
            } else { // no code points, so this spot is clean
                element->mapCE = uprv_uca_addPrefix(t, UCOL_NOT_FOUND, element, status);
                uCE = (UCAElements *)uprv_malloc(sizeof(UCAElements));
                /* test for NULL */
                if (uCE == NULL) {
                    *status = U_MEMORY_ALLOCATION_ERROR;
                    return 0;
                }
                uprv_memcpy(uCE, element, sizeof(UCAElements));
                uCE->cPoints = uCE->uchars;
                uhash_put(t->prefixLookup, uCE, uCE, status);
            }
            if(composed->prefixSize != element->prefixSize || uprv_memcmp(composed->prefix, element->prefix, element->prefixSize)) {
                // do it!
                composed->mapCE = uprv_uca_addPrefix(t, element->mapCE, composed, status);
            }
        }
        uprv_free(composed);
    }

    // We need to use the canonical iterator here
    // the way we do it is to generate the canonically equivalent strings 
    // for the contraction and then add the sequences that pass FCD check
    if(element->cSize > 1 && !(element->cSize==2 && UTF16_IS_LEAD(element->cPoints[0]) && UTF16_IS_TRAIL(element->cPoints[1]))) { // this is a contraction, we should check whether a composed form should also be included
        UnicodeString source(element->cPoints, element->cSize);
        CanonicalIterator it(source, *status);
        source = it.next();
        while(!source.isBogus()) {
            if(Normalizer::quickCheck(source, UNORM_FCD, *status) != UNORM_NO) {
                element->cSize = source.extract(element->cPoints, 128, *status);
                uprv_uca_finalizeAddition(t, element, status);
            }
            source = it.next();
        }
        CE = element->mapCE;
    } else {
        CE = uprv_uca_finalizeAddition(t, element, status);  
    }

    return CE;
}


/*void uprv_uca_getMaxExpansionJamo(CompactEIntArray       *mapping, */
static void uprv_uca_getMaxExpansionJamo(UNewTrie       *mapping, 
                                         MaxExpansionTable     *maxexpansion,
                                         MaxJamoExpansionTable *maxjamoexpansion,
                                         UBool                  jamospecial,
                                         UErrorCode            *status)
{
    const uint32_t VBASE  = 0x1161;
    const uint32_t TBASE  = 0x11A8;
    const uint32_t VCOUNT = 21;
    const uint32_t TCOUNT = 28;

    uint32_t v = VBASE + VCOUNT - 1;
    uint32_t t = TBASE + TCOUNT - 1;
    uint32_t ce;

    while (v >= VBASE) {
        /*ce = ucmpe32_get(mapping, v);*/
        ce = utrie_get32(mapping, v, NULL);
        if (ce < UCOL_SPECIAL_FLAG) {
            uprv_uca_setMaxExpansion(ce, 2, maxexpansion, status);
        }
        v --;
    }

    while (t >= TBASE)
    {
        /*ce = ucmpe32_get(mapping, t);*/
        ce = utrie_get32(mapping, t, NULL);
        if (ce < UCOL_SPECIAL_FLAG) {
            uprv_uca_setMaxExpansion(ce, 3, maxexpansion, status);
        }
        t --;
    }
    /*  According to the docs, 99% of the time, the Jamo will not be special */
    if (jamospecial) {
        /* gets the max expansion in all unicode characters */
        int     count    = maxjamoexpansion->position;
        uint8_t maxTSize = (uint8_t)(maxjamoexpansion->maxLSize + 
            maxjamoexpansion->maxVSize +
            maxjamoexpansion->maxTSize);
        uint8_t maxVSize = (uint8_t)(maxjamoexpansion->maxLSize + 
            maxjamoexpansion->maxVSize);

        while (count > 0) {
            count --;
            if (*(maxjamoexpansion->isV + count) == TRUE) {
                uprv_uca_setMaxExpansion(
                    *(maxjamoexpansion->endExpansionCE + count), 
                    maxVSize, maxexpansion, status);
            }
            else {
                uprv_uca_setMaxExpansion(
                    *(maxjamoexpansion->endExpansionCE + count), 
                    maxTSize, maxexpansion, status);
            }
        }
    }
}

U_CDECL_BEGIN
static inline uint32_t U_CALLCONV
getFoldedValue(UNewTrie *trie, UChar32 start, int32_t offset)
{
    uint32_t value;
    uint32_t tag;
    UChar32 limit;
    UBool inBlockZero;

    limit=start+0x400;
    while(start<limit) {
        value=utrie_get32(trie, start, &inBlockZero);
        tag = getCETag(value);
        if(inBlockZero == TRUE) {
            start+=UTRIE_DATA_BLOCK_LENGTH;
        } else if(!(isSpecial(value) && (tag == IMPLICIT_TAG || tag == NOT_FOUND_TAG))) {
            /* These are values that are starting in either UCA (IMPLICIT_TAG) or in the 
            * tailorings (NOT_FOUND_TAG). Presence of these tags means that there is 
            * nothing in this position and that it should be skipped.
            */
#ifdef UCOL_DEBUG
            static int32_t count = 1;
            fprintf(stdout, "%i, Folded %08X, value %08X\n", count++, start, value);
#endif
            return (uint32_t)(UCOL_SPECIAL_FLAG | (SURROGATE_TAG<<24) | offset);
        } else {
            ++start;
        }
    }
    return 0;
}
U_CDECL_END

#ifdef UCOL_DEBUG
// This is a debug function to print the contents of a trie.
// It is used in conjuction with the code around utrie_unserialize call
UBool enumRange(const void *context, UChar32 start, UChar32 limit, uint32_t value) {
    if(start<0x10000) {
        fprintf(stdout, "%08X, %08X, %08X\n", start, limit, value);
    } else {
        fprintf(stdout, "%08X=%04X %04X, %08X=%04X %04X, %08X\n", start, UTF16_LEAD(start), UTF16_TRAIL(start), limit, UTF16_LEAD(limit), UTF16_TRAIL(limit), value);
    }
    return TRUE;
}

int32_t 
myGetFoldingOffset(uint32_t data) {
    if(data > UCOL_NOT_FOUND && getCETag(data) == SURROGATE_TAG) {
        return (data&0xFFFFFF);
    } else {
        return 0;
    }
}
#endif

U_CAPI UCATableHeader* U_EXPORT2
uprv_uca_assembleTable(tempUCATable *t, UErrorCode *status) {
    /*CompactEIntArray *mapping = t->mapping;*/
    UNewTrie *mapping = t->mapping;
    ExpansionTable *expansions = t->expansions;
    CntTable *contractions = t->contractions;
    MaxExpansionTable *maxexpansion = t->maxExpansions;

    if(U_FAILURE(*status)) {
        return NULL;
    }

    uint32_t beforeContractions = (uint32_t)((headersize+paddedsize(expansions->position*sizeof(uint32_t)))/sizeof(UChar));

    int32_t contractionsSize = 0;
    contractionsSize = uprv_cnttab_constructTable(contractions, beforeContractions, status);

    /* the following operation depends on the trie data. Therefore, we have to do it before */
    /* the trie is compacted */
    /* sets jamo expansions */
    uprv_uca_getMaxExpansionJamo(mapping, maxexpansion, t->maxJamoExpansions,
        t->image->jamoSpecial, status);

    /*ucmpe32_compact(mapping);*/
    /*UMemoryStream *ms = uprv_mstrm_openNew(8192);*/
    /*int32_t mappingSize = ucmpe32_flattenMem(mapping, ms);*/
    /*const uint8_t *flattened = uprv_mstrm_getBuffer(ms, &mappingSize);*/

    // After setting the jamo expansions, compact the trie and get the needed size
    int32_t mappingSize = utrie_serialize(mapping, NULL, 0, getFoldedValue /*getFoldedValue*/, FALSE, status);

    uint32_t tableOffset = 0;
    uint8_t *dataStart;

    /* TODO: LATIN1 array is now in the utrie - it should be removed from the calculation */

    uint32_t toAllocate =(uint32_t)(headersize+                                    
        paddedsize(expansions->position*sizeof(uint32_t))+
        paddedsize(mappingSize)+
        paddedsize(contractionsSize*(sizeof(UChar)+sizeof(uint32_t)))+
        //paddedsize(0x100*sizeof(uint32_t))  /* Latin1 is now included in the trie */
        /* maxexpansion array */
        + paddedsize(maxexpansion->position * sizeof(uint32_t)) +
        /* maxexpansion size array */
        paddedsize(maxexpansion->position * sizeof(uint8_t)) +
        paddedsize(UCOL_UNSAFECP_TABLE_SIZE) +   /*  Unsafe chars             */
        paddedsize(UCOL_UNSAFECP_TABLE_SIZE));    /*  Contraction Ending chars */


    dataStart = (uint8_t *)uprv_malloc(toAllocate);
    /* test for NULL */
    if (dataStart == NULL) {
        *status = U_MEMORY_ALLOCATION_ERROR;
        return NULL;
    }

    UCATableHeader *myData = (UCATableHeader *)dataStart;
    // Please, do reset all the fields!
    uprv_memset(dataStart, 0, toAllocate);
    // Make sure we know this is reset
    myData->magic = UCOL_HEADER_MAGIC;
    myData->isBigEndian = U_IS_BIG_ENDIAN;
    myData->charSetFamily = U_CHARSET_FAMILY;
    myData->formatVersion[0] = UCA_FORMAT_VERSION_0;
    myData->formatVersion[1] = UCA_FORMAT_VERSION_1;
    myData->formatVersion[2] = UCA_FORMAT_VERSION_2;
    myData->formatVersion[3] = UCA_FORMAT_VERSION_3;
    myData->jamoSpecial = t->image->jamoSpecial;

    // Don't copy stuff from UCA header!
    //uprv_memcpy(myData, t->image, sizeof(UCATableHeader));

    myData->contractionSize = contractionsSize;

    tableOffset += (uint32_t)(paddedsize(sizeof(UCATableHeader)));

    myData->options = tableOffset;
    uprv_memcpy(dataStart+tableOffset, t->options, sizeof(UColOptionSet));
    tableOffset += (uint32_t)(paddedsize(sizeof(UColOptionSet)));

    /* copy expansions */
    /*myData->expansion = (uint32_t *)dataStart+tableOffset;*/
    myData->expansion = tableOffset;
    uprv_memcpy(dataStart+tableOffset, expansions->CEs, expansions->position*sizeof(uint32_t));
    tableOffset += (uint32_t)(paddedsize(expansions->position*sizeof(uint32_t)));

    /* contractions block */
    if(contractionsSize != 0) {
        /* copy contraction index */
        /*myData->contractionIndex = (UChar *)(dataStart+tableOffset);*/
        myData->contractionIndex = tableOffset;
        uprv_memcpy(dataStart+tableOffset, contractions->codePoints, contractionsSize*sizeof(UChar));
        tableOffset += (uint32_t)(paddedsize(contractionsSize*sizeof(UChar)));

        /* copy contraction collation elements */
        /*myData->contractionCEs = (uint32_t *)(dataStart+tableOffset);*/
        myData->contractionCEs = tableOffset;
        uprv_memcpy(dataStart+tableOffset, contractions->CEs, contractionsSize*sizeof(uint32_t));
        tableOffset += (uint32_t)(paddedsize(contractionsSize*sizeof(uint32_t)));
    } else {
        myData->contractionIndex = 0;
        myData->contractionCEs = 0;
    }

    /* copy mapping table */
    /*myData->mappingPosition = dataStart+tableOffset;*/
    /*myData->mappingPosition = tableOffset;*/
    /*uprv_memcpy(dataStart+tableOffset, flattened, mappingSize);*/

    myData->mappingPosition = tableOffset;
    utrie_serialize(mapping, dataStart+tableOffset, toAllocate-tableOffset, getFoldedValue, FALSE, status);
#ifdef UCOL_DEBUG
    // This is debug code to dump the contents of the trie. It needs two functions defined above
    {
        UTrie UCAt = { 0 };
        uint32_t trieWord;
        utrie_unserialize(&UCAt, dataStart+tableOffset, 9999999, status);
        UCAt.getFoldingOffset = myGetFoldingOffset;
        if(U_SUCCESS(*status)) {
            utrie_enum(&UCAt, NULL, enumRange, NULL);
        }
        trieWord = UTRIE_GET32_FROM_LEAD(&UCAt, 0xDC01);
    }
#endif
    tableOffset += paddedsize(mappingSize);


    int32_t i = 0;

    /* copy max expansion table */
    myData->endExpansionCE      = tableOffset;
    myData->endExpansionCECount = maxexpansion->position - 1;
    /* not copying the first element which is a dummy */
    uprv_memcpy(dataStart + tableOffset, maxexpansion->endExpansionCE + 1, 
        (maxexpansion->position - 1) * sizeof(uint32_t));
    tableOffset += (uint32_t)(paddedsize((maxexpansion->position)* sizeof(uint32_t)));
    myData->expansionCESize = tableOffset;
    uprv_memcpy(dataStart + tableOffset, maxexpansion->expansionCESize + 1, 
        (maxexpansion->position - 1) * sizeof(uint8_t));
    tableOffset += (uint32_t)(paddedsize((maxexpansion->position)* sizeof(uint8_t)));

    /* Unsafe chars table.  Finish it off, then copy it. */
    uprv_uca_unsafeCPAddCCNZ(t, status);
    if (t->UCA != 0) {              /* Or in unsafebits from UCA, making a combined table.    */
        for (i=0; i<UCOL_UNSAFECP_TABLE_SIZE; i++) {    
            t->unsafeCP[i] |= t->UCA->unsafeCP[i];
        }
    }
    myData->unsafeCP = tableOffset;
    uprv_memcpy(dataStart + tableOffset, t->unsafeCP, UCOL_UNSAFECP_TABLE_SIZE);
    tableOffset += paddedsize(UCOL_UNSAFECP_TABLE_SIZE);


    /* Finish building Contraction Ending chars hash table and then copy it out.  */
    if (t->UCA != 0) {              /* Or in unsafebits from UCA, making a combined table.    */
        for (i=0; i<UCOL_UNSAFECP_TABLE_SIZE; i++) {    
            t->contrEndCP[i] |= t->UCA->contrEndCP[i];
        }
    }
    myData->contrEndCP = tableOffset;
    uprv_memcpy(dataStart + tableOffset, t->contrEndCP, UCOL_UNSAFECP_TABLE_SIZE);
    tableOffset += paddedsize(UCOL_UNSAFECP_TABLE_SIZE);

    if(tableOffset != toAllocate) {
#ifdef UCOL_DEBUG
        fprintf(stderr, "calculation screwup!!! Expected to write %i but wrote %i instead!!!\n", toAllocate, tableOffset);
#endif
        *status = U_INTERNAL_PROGRAM_ERROR;
        uprv_free(dataStart);
        return 0;
    }

    myData->size = tableOffset;
    /* This should happen upon ressurection */
    /*const uint8_t *mapPosition = (uint8_t*)myData+myData->mappingPosition;*/
    /*uprv_mstrm_close(ms);*/
    return myData;
}


struct enumStruct {
    tempUCATable *t;
    UCollator *tempColl;
    UCollationElements* colEl;
    const Normalizer2Impl *nfcImpl;
    UnicodeSet *closed;
    int32_t noOfClosures;
    UErrorCode *status;
};
U_CDECL_BEGIN
static UBool U_CALLCONV
_enumCategoryRangeClosureCategory(const void *context, UChar32 start, UChar32 limit, UCharCategory type) {

    if (type != U_UNASSIGNED && type != U_PRIVATE_USE_CHAR) { // if the range is assigned - we might ommit more categories later
        UErrorCode *status = ((enumStruct *)context)->status;
        tempUCATable *t = ((enumStruct *)context)->t;
        UCollator *tempColl = ((enumStruct *)context)->tempColl;
        UCollationElements* colEl = ((enumStruct *)context)->colEl;
        UCAElements el;
        UChar decompBuffer[4];
        const UChar *decomp;
        int32_t noOfDec = 0;

        UChar32 u32 = 0;
        UChar comp[2];
        uint32_t len = 0;

        for(u32 = start; u32 < limit; u32++) {
            decomp = ((enumStruct *)context)->nfcImpl->
                getDecomposition(u32, decompBuffer, noOfDec);
            //if((noOfDec = unorm_normalize(comp, len, UNORM_NFD, 0, decomp, 256, status)) > 1
            //|| (noOfDec == 1 && *decomp != (UChar)u32))
            if(decomp != NULL)
            {
                len = 0;
                U16_APPEND_UNSAFE(comp, len, u32);
                if(ucol_strcoll(tempColl, comp, len, decomp, noOfDec) != UCOL_EQUAL) {
#ifdef UCOL_DEBUG
                    fprintf(stderr, "Closure: U+%04X -> ", u32);
                    UChar32 c;
                    int32_t i = 0;
                    while(i < noOfDec) {
                        U16_NEXT(decomp, i, noOfDec, c);
                        fprintf(stderr, "%04X ", c);
                    }
                    fprintf(stderr, "\n");
                    // print CEs for code point vs. decomposition
                    fprintf(stderr, "U+%04X CEs: ", u32);
                    UCollationElements *iter = ucol_openElements(tempColl, comp, len, status);
                    int32_t ce;
                    while((ce = ucol_next(iter, status)) != UCOL_NULLORDER) {
                        fprintf(stderr, "%08X ", ce);
                    }
                    fprintf(stderr, "\nDecomp CEs: ");
                    ucol_setText(iter, decomp, noOfDec, status);
                    while((ce = ucol_next(iter, status)) != UCOL_NULLORDER) {
                        fprintf(stderr, "%08X ", ce);
                    }
                    fprintf(stderr, "\n");
                    ucol_closeElements(iter);
#endif
                    if(((enumStruct *)context)->closed != NULL) {
                        ((enumStruct *)context)->closed->add(u32);
                    }
                    ((enumStruct *)context)->noOfClosures++;
                    el.cPoints = (UChar *)decomp;
                    el.cSize = noOfDec;
                    el.noOfCEs = 0;
                    el.prefix = el.prefixChars;
                    el.prefixSize = 0;

                    UCAElements *prefix=(UCAElements *)uhash_get(t->prefixLookup, &el);
                    el.cPoints = comp;
                    el.cSize = len;
                    el.prefix = el.prefixChars;
                    el.prefixSize = 0;
                    if(prefix == NULL) {
                        el.noOfCEs = 0;
                        ucol_setText(colEl, decomp, noOfDec, status);
                        while((el.CEs[el.noOfCEs] = ucol_next(colEl, status)) != (uint32_t)UCOL_NULLORDER) {
                            el.noOfCEs++;
                        }
                    } else {
                        el.noOfCEs = 1;
                        el.CEs[0] = prefix->mapCE;
                        // This character uses a prefix. We have to add it 
                        // to the unsafe table, as it decomposed form is already
                        // in. In Japanese, this happens for \u309e & \u30fe
                        // Since unsafeCPSet is static in ucol_elm, we are going
                        // to wrap it up in the uprv_uca_unsafeCPAddCCNZ function
                    }
                    uprv_uca_addAnElement(t, &el, status);
                }
            }
        }
    }
    return TRUE;
}
U_CDECL_END

static void
uprv_uca_setMapCE(tempUCATable *t, UCAElements *element, UErrorCode *status) {
    uint32_t expansion = 0;
    int32_t j;

    ExpansionTable *expansions = t->expansions;
    if(element->noOfCEs == 2 // a two CE expansion
        && isContinuation(element->CEs[1]) // which  is a continuation
        && (element->CEs[1] & (~(0xFF << 24 | UCOL_CONTINUATION_MARKER))) == 0 // that has only primaries in continuation,
        && (((element->CEs[0]>>8) & 0xFF) == UCOL_BYTE_COMMON) // a common secondary
        && ((element->CEs[0] & 0xFF) == UCOL_BYTE_COMMON) // and a common tertiary
        ) {
            element->mapCE = UCOL_SPECIAL_FLAG | (LONG_PRIMARY_TAG<<24) // a long primary special
                | ((element->CEs[0]>>8) & 0xFFFF00) // first and second byte of primary
                | ((element->CEs[1]>>24) & 0xFF);   // third byte of primary
        } else {
            expansion = (uint32_t)(UCOL_SPECIAL_FLAG | (EXPANSION_TAG<<UCOL_TAG_SHIFT)
                | (((uprv_uca_addExpansion(expansions, element->CEs[0], status)+(headersize>>2))<<4)
                   & 0xFFFFF0));

            for(j = 1; j<(int32_t)element->noOfCEs; j++) {
                uprv_uca_addExpansion(expansions, element->CEs[j], status);
            }
            if(element->noOfCEs <= 0xF) {
                expansion |= element->noOfCEs;
            } else {
                uprv_uca_addExpansion(expansions, 0, status);
            }
            element->mapCE = expansion;
            uprv_uca_setMaxExpansion(element->CEs[element->noOfCEs - 1],
                (uint8_t)element->noOfCEs,
                t->maxExpansions,
                status);
        }
}

static void
uprv_uca_addFCD4AccentedContractions(tempUCATable *t,
                                      UCollationElements* colEl,
                                      UChar *data,
                                      int32_t len,
                                      UCAElements *el,
                                      UErrorCode *status) {
    UChar decomp[256], comp[256];
    int32_t decLen, compLen;

    decLen = unorm_normalize(data, len, UNORM_NFD, 0, decomp, 256, status);
    compLen = unorm_normalize(data, len, UNORM_NFC, 0, comp, 256, status);
    decomp[decLen] = comp[compLen] = 0;

    el->cPoints = decomp;
    el->cSize = decLen;
    el->noOfCEs = 0;
    el->prefixSize = 0;
    el->prefix = el->prefixChars;

    UCAElements *prefix=(UCAElements *)uhash_get(t->prefixLookup, el);
    el->cPoints = comp;
    el->cSize = compLen;
    el->prefix = el->prefixChars;
    el->prefixSize = 0;
    if(prefix == NULL) {
        el->noOfCEs = 0;
        ucol_setText(colEl, decomp, decLen, status);
        while((el->CEs[el->noOfCEs] = ucol_next(colEl, status)) != (uint32_t)UCOL_NULLORDER) {
            el->noOfCEs++;
        }
        uprv_uca_setMapCE(t, el, status);
        uprv_uca_addAnElement(t, el, status);
    }
}

static void
uprv_uca_addMultiCMContractions(tempUCATable *t,
                                UCollationElements* colEl,
                                tempTailorContext *c,
                                UCAElements *el,
                                UErrorCode *status) {
    CombinClassTable *cmLookup = t->cmLookup;
    UChar  newDecomp[256];
    int32_t maxComp, newDecLen;
    UChar32 fcdHighStart;
    const uint16_t *fcdTrieIndex = unorm_getFCDTrieIndex(fcdHighStart, status);
    if (U_FAILURE(*status)) {
        return;
    }
    int16_t curClass = (unorm_getFCD16(fcdTrieIndex, c->tailoringCM) & 0xff);
    CompData *precomp = c->precomp;
    int32_t  compLen = c->compLen;
    UChar *comp = c->comp;
    maxComp = c->precompLen;

    for (int32_t j=0; j < maxComp; j++) {
        int32_t count=0;
        do {
            if ( count == 0 ) {  // Decompose the saved precomposed char.
                UChar temp[2];
                temp[0]=precomp[j].cp;
                temp[1]=0;
                newDecLen = unorm_normalize(temp, 1, UNORM_NFD, 0,
                            newDecomp, sizeof(newDecomp)/sizeof(UChar), status);
                newDecomp[newDecLen++] = cmLookup->cPoints[c->cmPos];
            }
            else {  // swap 2 combining marks when they are equal.
                uprv_memcpy(newDecomp, c->decomp, sizeof(UChar)*(c->decompLen));
                newDecLen = c->decompLen;
                newDecomp[newDecLen++] = precomp[j].cClass;
            }
            newDecomp[newDecLen] = 0;
            compLen = unorm_normalize(newDecomp, newDecLen, UNORM_NFC, 0,
                              comp, 256, status);
            if (compLen==1) {
                comp[compLen++] = newDecomp[newDecLen++] = c->tailoringCM;
                comp[compLen] = newDecomp[newDecLen] = 0;
                el->cPoints = newDecomp;
                el->cSize = newDecLen;

                UCAElements *prefix=(UCAElements *)uhash_get(t->prefixLookup, el);
                el->cPoints = c->comp;
                el->cSize = compLen;
                el->prefix = el->prefixChars;
                el->prefixSize = 0;
                if(prefix == NULL) {
                    el->noOfCEs = 0;
                    ucol_setText(colEl, newDecomp, newDecLen, status);
                    while((el->CEs[el->noOfCEs] = ucol_next(colEl, status)) != (uint32_t)UCOL_NULLORDER) {
                        el->noOfCEs++;
                    }
                    uprv_uca_setMapCE(t, el, status);
                    uprv_uca_finalizeAddition(t, el, status);

                    // Save the current precomposed char and its class to find any
                    // other combining mark combinations.
                    precomp[c->precompLen].cp=comp[0];
                    precomp[c->precompLen].cClass = curClass;
                    c->precompLen++;
                }
            }
        } while (++count<2 && (precomp[j].cClass == curClass));
    }

}

static void
uprv_uca_addTailCanonicalClosures(tempUCATable *t,
                                  UCollationElements* colEl,
                                  UChar baseCh,
                                  UChar cMark,
                                  UCAElements *el,
                                  UErrorCode *status) {
    CombinClassTable *cmLookup = t->cmLookup;
    UChar32 fcdHighStart;
    const uint16_t *fcdTrieIndex = unorm_getFCDTrieIndex(fcdHighStart, status);
    if (U_FAILURE(*status)) {
        return;
    }
    int16_t maxIndex = (unorm_getFCD16(fcdTrieIndex, cMark) & 0xff );
    UCAElements element;
    uint16_t *index;
    UChar  decomp[256];
    UChar  comp[256];
    CompData precomp[256];   // precomposed array
    int32_t  precompLen = 0; // count for precomp
    int32_t i, len, decompLen, curClass, replacedPos;
    tempTailorContext c;

    if ( cmLookup == NULL ) {
        return;
    }
    index = cmLookup->index;
    int32_t cClass=(unorm_getFCD16(fcdTrieIndex, cMark) & 0xff);
    maxIndex = (int32_t)index[(unorm_getFCD16(fcdTrieIndex, cMark) & 0xff)-1];
    c.comp = comp;
    c.decomp = decomp;
    c.precomp = precomp;
    c.tailoringCM =  cMark;

    if (cClass>0) {
        maxIndex = (int32_t)index[cClass-1];
    }
    else {
        maxIndex=0;
    }
    decomp[0]=baseCh;
    for ( i=0; i<maxIndex ; i++ ) {
        decomp[1] = cmLookup->cPoints[i];
        decomp[2]=0;
        decompLen=2;
        len = unorm_normalize(decomp, decompLen, UNORM_NFC, 0, comp, 256, status);
        if (len==1) {
            // Save the current precomposed char and its class to find any
            // other combining mark combinations.
            precomp[precompLen].cp=comp[0];
            curClass = precomp[precompLen].cClass =
                       index[unorm_getFCD16(fcdTrieIndex, decomp[1]) & 0xff];
            precompLen++;
            replacedPos=0;
            for (decompLen=0; decompLen< (int32_t)el->cSize; decompLen++) {
                decomp[decompLen] = el->cPoints[decompLen];
                if (decomp[decompLen]==cMark) {
                    replacedPos = decompLen;  // record the position for later use
                }
            }
            if ( replacedPos != 0 ) {
                decomp[replacedPos]=cmLookup->cPoints[i];
            }
            decomp[decompLen] = 0;
            len = unorm_normalize(decomp, decompLen, UNORM_NFC, 0, comp, 256, status);
            comp[len++] = decomp[decompLen++] = cMark;
            comp[len] = decomp[decompLen] = 0;
            element.cPoints = decomp;
            element.cSize = decompLen;
            element.noOfCEs = 0;
            element.prefix = el->prefixChars;
            element.prefixSize = 0;

            UCAElements *prefix=(UCAElements *)uhash_get(t->prefixLookup, &element);
            element.cPoints = comp;
            element.cSize = len;
            element.prefix = el->prefixChars;
            element.prefixSize = 0;
            if(prefix == NULL) {
                element.noOfCEs = 0;
                ucol_setText(colEl, decomp, decompLen, status);
                while((element.CEs[element.noOfCEs] = ucol_next(colEl, status)) != (uint32_t)UCOL_NULLORDER) {
                    element.noOfCEs++;
                }
                uprv_uca_setMapCE(t, &element, status);
                uprv_uca_finalizeAddition(t, &element, status);
            }

            // This is a fix for tailoring contractions with accented
            // character at the end of contraction string.
            if ((len>2) && 
                (unorm_getFCD16(fcdTrieIndex, comp[len-2]) & 0xff00)==0) {
                uprv_uca_addFCD4AccentedContractions(t, colEl, comp, len, &element, status);
            }

            if (precompLen >1) {
                c.compLen = len;
                c.decompLen = decompLen;
                c.precompLen = precompLen;
                c.cmPos = i;
                uprv_uca_addMultiCMContractions(t, colEl, &c, &element, status);
                precompLen = c.precompLen;
            }
        }
    }
}

U_CFUNC int32_t U_EXPORT2
uprv_uca_canonicalClosure(tempUCATable *t,
                          UColTokenParser *src,
                          UnicodeSet *closed,
                          UErrorCode *status)
{
    enumStruct context;
    context.closed = closed;
    context.noOfClosures = 0;
    UCAElements el;
    UColToken *tok;
    uint32_t i = 0, j = 0;
    UChar  baseChar, firstCM;
    UChar32 fcdHighStart;
    const uint16_t *fcdTrieIndex = unorm_getFCDTrieIndex(fcdHighStart, status);
    context.nfcImpl=Normalizer2Factory::getNFCImpl(*status);
    if(U_FAILURE(*status)) {
        return 0;
    }

    UCollator *tempColl = NULL;
    tempUCATable *tempTable = uprv_uca_cloneTempTable(t, status);
    // Check for null pointer
    if (U_FAILURE(*status)) {
        return 0;
    }

    UCATableHeader *tempData = uprv_uca_assembleTable(tempTable, status);
    tempColl = ucol_initCollator(tempData, 0, t->UCA, status);
    if ( tempTable->cmLookup != NULL ) {
        t->cmLookup = tempTable->cmLookup;  // copy over to t
        tempTable->cmLookup = NULL;
    }
    uprv_uca_closeTempTable(tempTable);

    if(U_SUCCESS(*status)) {
        tempColl->ucaRules = NULL;
        tempColl->actualLocale = NULL;
        tempColl->validLocale = NULL;
        tempColl->requestedLocale = NULL;
        tempColl->hasRealData = TRUE;
        tempColl->freeImageOnClose = TRUE;
    } else if(tempData != 0) {
        uprv_free(tempData);
    }

    /* produce canonical closure */
    UCollationElements* colEl = ucol_openElements(tempColl, NULL, 0, status);
    // Check for null pointer
    if (U_FAILURE(*status)) {
        return 0;
    }
    context.t = t;
    context.tempColl = tempColl;
    context.colEl = colEl;
    context.status = status;
    u_enumCharTypes(_enumCategoryRangeClosureCategory, &context);

    if ( (src==NULL) || !src->buildCCTabFlag ) {
        ucol_closeElements(colEl);
        ucol_close(tempColl);
        return context.noOfClosures;  // no extra contraction needed to add
    }

    for (i=0; i < src->resultLen; i++) {
        baseChar = firstCM= (UChar)0;
        tok = src->lh[i].first;
        while (tok != NULL && U_SUCCESS(*status)) {
            el.prefix = el.prefixChars;
            el.cPoints = el.uchars;
            if(tok->prefix != 0) {
                el.prefixSize = tok->prefix>>24;
                uprv_memcpy(el.prefix, src->source + (tok->prefix & 0x00FFFFFF), el.prefixSize*sizeof(UChar));

                el.cSize = (tok->source >> 24)-(tok->prefix>>24);
                uprv_memcpy(el.uchars, (tok->source & 0x00FFFFFF)+(tok->prefix>>24) + src->source, el.cSize*sizeof(UChar));
            } else {
                el.prefixSize = 0;
                *el.prefix = 0;

                el.cSize = (tok->source >> 24);
                uprv_memcpy(el.uchars, (tok->source & 0x00FFFFFF) + src->source, el.cSize*sizeof(UChar));
            }
            if(src->UCA != NULL) {
                for(j = 0; j<el.cSize; j++) {
                    int16_t fcd = unorm_getFCD16(fcdTrieIndex, el.cPoints[j]);
                    if ( (fcd & 0xff) == 0 ) {
                        baseChar = el.cPoints[j];  // last base character
                        firstCM=0;  // reset combining mark value
                    }
                    else {
                        if ( (baseChar!=0) && (firstCM==0) ) {
                            firstCM = el.cPoints[j];  // first combining mark
                        }
                    }
                }
            }
            if ( (baseChar!= (UChar)0) && (firstCM != (UChar)0) ) {
                // find all the canonical rules
                uprv_uca_addTailCanonicalClosures(t, colEl, baseChar, firstCM, &el, status);
            }
            tok = tok->next;
        }
    }
    ucol_closeElements(colEl);
    ucol_close(tempColl);

    return context.noOfClosures;
}

#endif /* #if !UCONFIG_NO_COLLATION */
