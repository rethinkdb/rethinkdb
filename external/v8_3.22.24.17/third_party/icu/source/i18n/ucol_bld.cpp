/*
*******************************************************************************
*
*   Copyright (C) 2001-2010, International Business Machines
*   Corporation and others.  All Rights Reserved.
*
*******************************************************************************
*   file name:  ucol_bld.cpp
*   encoding:   US-ASCII
*   tab size:   8 (not used)
*   indentation:4
*
*   created 02/22/2001
*   created by: Vladimir Weinstein
*
* This module builds a collator based on the rule set.
*
*/

#include "unicode/utypes.h"

#if !UCONFIG_NO_COLLATION

#include "unicode/ucoleitr.h"
#include "unicode/udata.h"
#include "unicode/uchar.h"
#include "unicode/uniset.h"
#include "unicode/uscript.h"
#include "unicode/ustring.h"
#include "normalizer2impl.h"
#include "ucol_bld.h"
#include "ucol_elm.h"
#include "ucol_cnt.h"
#include "ucln_in.h"
#include "umutex.h"
#include "cmemory.h"
#include "cstring.h"

U_NAMESPACE_BEGIN

static const InverseUCATableHeader* _staticInvUCA = NULL;
static UDataMemory* invUCA_DATA_MEM = NULL;

U_CDECL_BEGIN
static UBool U_CALLCONV
isAcceptableInvUCA(void * /*context*/,
                   const char * /*type*/, const char * /*name*/,
                   const UDataInfo *pInfo)
{
    /* context, type & name are intentionally not used */
    if( pInfo->size>=20 &&
        pInfo->isBigEndian==U_IS_BIG_ENDIAN &&
        pInfo->charsetFamily==U_CHARSET_FAMILY &&
        pInfo->dataFormat[0]==INVUCA_DATA_FORMAT_0 &&   /* dataFormat="InvC" */
        pInfo->dataFormat[1]==INVUCA_DATA_FORMAT_1 &&
        pInfo->dataFormat[2]==INVUCA_DATA_FORMAT_2 &&
        pInfo->dataFormat[3]==INVUCA_DATA_FORMAT_3 &&
        pInfo->formatVersion[0]==INVUCA_FORMAT_VERSION_0 &&
        pInfo->formatVersion[1]>=INVUCA_FORMAT_VERSION_1 //&&
        //pInfo->formatVersion[1]==INVUCA_FORMAT_VERSION_1 &&
        //pInfo->formatVersion[2]==INVUCA_FORMAT_VERSION_2 &&
        //pInfo->formatVersion[3]==INVUCA_FORMAT_VERSION_3 &&
        )
    {
        UVersionInfo UCDVersion;
        u_getUnicodeVersion(UCDVersion);
        return (pInfo->dataVersion[0]==UCDVersion[0] &&
            pInfo->dataVersion[1]==UCDVersion[1]);
            //pInfo->dataVersion[1]==invUcaDataInfo.dataVersion[1] &&
            //pInfo->dataVersion[2]==invUcaDataInfo.dataVersion[2] &&
            //pInfo->dataVersion[3]==invUcaDataInfo.dataVersion[3]) {
    } else {
        return FALSE;
    }
}
U_CDECL_END

/*
* Takes two CEs (lead and continuation) and
* compares them as CEs should be compared:
* primary vs. primary, secondary vs. secondary
* tertiary vs. tertiary
*/
static int32_t compareCEs(uint32_t source0, uint32_t source1, uint32_t target0, uint32_t target1) {
    uint32_t s1 = source0, s2, t1 = target0, t2;
    if(isContinuation(source1)) {
        s2 = source1;
    } else {
        s2 = 0;
    }
    if(isContinuation(target1)) {
        t2 = target1;
    } else {
        t2 = 0;
    }

    uint32_t s = 0, t = 0;
    if(s1 == t1 && s2 == t2) {
        return 0;
    }
    s = (s1 & 0xFFFF0000)|((s2 & 0xFFFF0000)>>16);
    t = (t1 & 0xFFFF0000)|((t2 & 0xFFFF0000)>>16);
    if(s < t) {
        return -1;
    } else if(s > t) {
        return 1;
    } else {
        s = (s1 & 0x0000FF00) | (s2 & 0x0000FF00)>>8;
        t = (t1 & 0x0000FF00) | (t2 & 0x0000FF00)>>8;
        if(s < t) {
            return -1;
        } else if(s > t) {
            return 1;
        } else {
            s = (s1 & 0x000000FF)<<8 | (s2 & 0x000000FF);
            t = (t1 & 0x000000FF)<<8 | (t2 & 0x000000FF);
            if(s < t) {
                return -1;
            } else {
                return 1;
            }
        }
    }
}

static
int32_t ucol_inv_findCE(const UColTokenParser *src, uint32_t CE, uint32_t SecondCE) {
    uint32_t bottom = 0, top = src->invUCA->tableSize;
    uint32_t i = 0;
    uint32_t first = 0, second = 0;
    uint32_t *CETable = (uint32_t *)((uint8_t *)src->invUCA+src->invUCA->table);
    int32_t res = 0;

    while(bottom < top-1) {
        i = (top+bottom)/2;
        first = *(CETable+3*i);
        second = *(CETable+3*i+1);
        res = compareCEs(first, second, CE, SecondCE);
        if(res > 0) {
            top = i;
        } else if(res < 0) {
            bottom = i;
        } else {
            break;
        }
    }

    /* weiv:                                                  */
    /* in searching for elements, I have removed the failure  */
    /* The reason for this is that the builder does not rely  */
    /* on search mechanism telling it that it didn't find an  */
    /* element. However, indirect positioning relies on being */
    /* able to find the elements around any CE, even if it is */
    /* not defined in the UCA. */
    return i;
    /*
    if((first == CE && second == SecondCE)) {
    return i;
    } else {
    return -1;
    }
    */
}

static const uint32_t strengthMask[UCOL_CE_STRENGTH_LIMIT] = {
    0xFFFF0000,
    0xFFFFFF00,
    0xFFFFFFFF
};

U_CAPI int32_t U_EXPORT2 ucol_inv_getNextCE(const UColTokenParser *src,
                                            uint32_t CE, uint32_t contCE,
                                            uint32_t *nextCE, uint32_t *nextContCE,
                                            uint32_t strength)
{
    uint32_t *CETable = (uint32_t *)((uint8_t *)src->invUCA+src->invUCA->table);
    int32_t iCE;

    iCE = ucol_inv_findCE(src, CE, contCE);

    if(iCE<0) {
        *nextCE = UCOL_NOT_FOUND;
        return -1;
    }

    CE &= strengthMask[strength];
    contCE &= strengthMask[strength];

    *nextCE = CE;
    *nextContCE = contCE;

    while((*nextCE  & strengthMask[strength]) == CE
        && (*nextContCE  & strengthMask[strength]) == contCE)
    {
        *nextCE = (*(CETable+3*(++iCE)));
        *nextContCE = (*(CETable+3*(iCE)+1));
    }

    return iCE;
}

U_CFUNC int32_t U_EXPORT2 ucol_inv_getPrevCE(const UColTokenParser *src,
                                            uint32_t CE, uint32_t contCE,
                                            uint32_t *prevCE, uint32_t *prevContCE,
                                            uint32_t strength)
{
    uint32_t *CETable = (uint32_t *)((uint8_t *)src->invUCA+src->invUCA->table);
    int32_t iCE;

    iCE = ucol_inv_findCE(src, CE, contCE);

    if(iCE<0) {
        *prevCE = UCOL_NOT_FOUND;
        return -1;
    }

    CE &= strengthMask[strength];
    contCE &= strengthMask[strength];

    *prevCE = CE;
    *prevContCE = contCE;

    while((*prevCE  & strengthMask[strength]) == CE
        && (*prevContCE  & strengthMask[strength])== contCE
        && iCE > 0) /* this condition should prevent falling off the edge of the world */
    {
        /* here, we end up in a singularity - zero */
        *prevCE = (*(CETable+3*(--iCE)));
        *prevContCE = (*(CETable+3*(iCE)+1));
    }

    return iCE;
}

U_CFUNC uint32_t U_EXPORT2 ucol_getCEStrengthDifference(uint32_t CE, uint32_t contCE,
                                                       uint32_t prevCE, uint32_t prevContCE)
{
    if(prevCE == CE && prevContCE == contCE) {
        return UCOL_IDENTICAL;
    }
    if((prevCE & strengthMask[UCOL_PRIMARY]) != (CE & strengthMask[UCOL_PRIMARY])
        || (prevContCE & strengthMask[UCOL_PRIMARY]) != (contCE & strengthMask[UCOL_PRIMARY]))
    {
        return UCOL_PRIMARY;
    }
    if((prevCE & strengthMask[UCOL_SECONDARY]) != (CE & strengthMask[UCOL_SECONDARY])
        || (prevContCE & strengthMask[UCOL_SECONDARY]) != (contCE & strengthMask[UCOL_SECONDARY]))
    {
        return UCOL_SECONDARY;
    }
    return UCOL_TERTIARY;
}


/*static
inline int32_t ucol_inv_getPrevious(UColTokenParser *src, UColTokListHeader *lh, uint32_t strength) {

    uint32_t CE = lh->baseCE;
    uint32_t SecondCE = lh->baseContCE;

    uint32_t *CETable = (uint32_t *)((uint8_t *)src->invUCA+src->invUCA->table);
    uint32_t previousCE, previousContCE;
    int32_t iCE;

    iCE = ucol_inv_findCE(src, CE, SecondCE);

    if(iCE<0) {
        return -1;
    }

    CE &= strengthMask[strength];
    SecondCE &= strengthMask[strength];

    previousCE = CE;
    previousContCE = SecondCE;

    while((previousCE  & strengthMask[strength]) == CE && (previousContCE  & strengthMask[strength])== SecondCE) {
        previousCE = (*(CETable+3*(--iCE)));
        previousContCE = (*(CETable+3*(iCE)+1));
    }
    lh->previousCE = previousCE;
    lh->previousContCE = previousContCE;

    return iCE;
}*/

static
inline int32_t ucol_inv_getNext(UColTokenParser *src, UColTokListHeader *lh, uint32_t strength) {
    uint32_t CE = lh->baseCE;
    uint32_t SecondCE = lh->baseContCE;

    uint32_t *CETable = (uint32_t *)((uint8_t *)src->invUCA+src->invUCA->table);
    uint32_t nextCE, nextContCE;
    int32_t iCE;

    iCE = ucol_inv_findCE(src, CE, SecondCE);

    if(iCE<0) {
        return -1;
    }

    CE &= strengthMask[strength];
    SecondCE &= strengthMask[strength];

    nextCE = CE;
    nextContCE = SecondCE;

    while((nextCE  & strengthMask[strength]) == CE
        && (nextContCE  & strengthMask[strength]) == SecondCE)
    {
        nextCE = (*(CETable+3*(++iCE)));
        nextContCE = (*(CETable+3*(iCE)+1));
    }

    lh->nextCE = nextCE;
    lh->nextContCE = nextContCE;

    return iCE;
}

static void ucol_inv_getGapPositions(UColTokenParser *src, UColTokListHeader *lh, UErrorCode *status) {
    /* reset all the gaps */
    int32_t i = 0;
    uint32_t *CETable = (uint32_t *)((uint8_t *)src->invUCA+src->invUCA->table);
    uint32_t st = 0;
    uint32_t t1, t2;
    int32_t pos;

    UColToken *tok = lh->first;
    uint32_t tokStrength = tok->strength;

    for(i = 0; i<3; i++) {
        lh->gapsHi[3*i] = 0;
        lh->gapsHi[3*i+1] = 0;
        lh->gapsHi[3*i+2] = 0;
        lh->gapsLo[3*i] = 0;
        lh->gapsLo[3*i+1] = 0;
        lh->gapsLo[3*i+2] = 0;
        lh->numStr[i] = 0;
        lh->fStrToken[i] = NULL;
        lh->lStrToken[i] = NULL;
        lh->pos[i] = -1;
    }

    UCAConstants *consts = (UCAConstants *)((uint8_t *)src->UCA->image + src->UCA->image->UCAConsts);

    if((lh->baseCE & 0xFF000000)>= (consts->UCA_PRIMARY_IMPLICIT_MIN<<24) && (lh->baseCE & 0xFF000000) <= (consts->UCA_PRIMARY_IMPLICIT_MAX<<24) ) { /* implicits - */
        //if(lh->baseCE >= PRIMARY_IMPLICIT_MIN && lh->baseCE < PRIMARY_IMPLICIT_MAX ) { /* implicits - */
        lh->pos[0] = 0;
        t1 = lh->baseCE;
        t2 = lh->baseContCE & UCOL_REMOVE_CONTINUATION;
        lh->gapsLo[0] = (t1 & UCOL_PRIMARYMASK) | (t2 & UCOL_PRIMARYMASK) >> 16;
        lh->gapsLo[1] = (t1 & UCOL_SECONDARYMASK) << 16 | (t2 & UCOL_SECONDARYMASK) << 8;
        lh->gapsLo[2] = (UCOL_TERTIARYORDER(t1)) << 24 | (UCOL_TERTIARYORDER(t2)) << 16;
        uint32_t primaryCE = (t1 & UCOL_PRIMARYMASK) | ((t2 & UCOL_PRIMARYMASK) >> 16);
        primaryCE = uprv_uca_getImplicitFromRaw(uprv_uca_getRawFromImplicit(primaryCE)+1);

        t1 = (primaryCE & UCOL_PRIMARYMASK) | 0x0505;
        t2 = (primaryCE << 16) & UCOL_PRIMARYMASK; // | UCOL_CONTINUATION_MARKER;

        lh->gapsHi[0] = (t1 & UCOL_PRIMARYMASK) | (t2 & UCOL_PRIMARYMASK) >> 16;
        lh->gapsHi[1] = (t1 & UCOL_SECONDARYMASK) << 16 | (t2 & UCOL_SECONDARYMASK) << 8;
        lh->gapsHi[2] = (UCOL_TERTIARYORDER(t1)) << 24 | (UCOL_TERTIARYORDER(t2)) << 16;
    } else if(lh->indirect == TRUE && lh->nextCE != 0) {
        //} else if(lh->baseCE == UCOL_RESET_TOP_VALUE && lh->baseContCE == 0) {
        lh->pos[0] = 0;
        t1 = lh->baseCE;
        t2 = lh->baseContCE&UCOL_REMOVE_CONTINUATION;
        lh->gapsLo[0] = (t1 & UCOL_PRIMARYMASK) | (t2 & UCOL_PRIMARYMASK) >> 16;
        lh->gapsLo[1] = (t1 & UCOL_SECONDARYMASK) << 16 | (t2 & UCOL_SECONDARYMASK) << 8;
        lh->gapsLo[2] = (UCOL_TERTIARYORDER(t1)) << 24 | (UCOL_TERTIARYORDER(t2)) << 16;
        t1 = lh->nextCE;
        t2 = lh->nextContCE&UCOL_REMOVE_CONTINUATION;
        lh->gapsHi[0] = (t1 & UCOL_PRIMARYMASK) | (t2 & UCOL_PRIMARYMASK) >> 16;
        lh->gapsHi[1] = (t1 & UCOL_SECONDARYMASK) << 16 | (t2 & UCOL_SECONDARYMASK) << 8;
        lh->gapsHi[2] = (UCOL_TERTIARYORDER(t1)) << 24 | (UCOL_TERTIARYORDER(t2)) << 16;
    } else {
        for(;;) {
            if(tokStrength < UCOL_CE_STRENGTH_LIMIT) {
                if((lh->pos[tokStrength] = ucol_inv_getNext(src, lh, tokStrength)) >= 0) {
                    lh->fStrToken[tokStrength] = tok;
                } else { /* The CE must be implicit, since it's not in the table */
                    /* Error */
                    *status = U_INTERNAL_PROGRAM_ERROR;
                }
            }

            while(tok != NULL && tok->strength >= tokStrength) {
                if(tokStrength < UCOL_CE_STRENGTH_LIMIT) {
                    lh->lStrToken[tokStrength] = tok;
                }
                tok = tok->next;
            }
            if(tokStrength < UCOL_CE_STRENGTH_LIMIT-1) {
                /* check if previous interval is the same and merge the intervals if it is so */
                if(lh->pos[tokStrength] == lh->pos[tokStrength+1]) {
                    lh->fStrToken[tokStrength] = lh->fStrToken[tokStrength+1];
                    lh->fStrToken[tokStrength+1] = NULL;
                    lh->lStrToken[tokStrength+1] = NULL;
                    lh->pos[tokStrength+1] = -1;
                }
            }
            if(tok != NULL) {
                tokStrength = tok->strength;
            } else {
                break;
            }
        }
        for(st = 0; st < 3; st++) {
            if((pos = lh->pos[st]) >= 0) {
                t1 = *(CETable+3*(pos));
                t2 = *(CETable+3*(pos)+1);
                lh->gapsHi[3*st] = (t1 & UCOL_PRIMARYMASK) | (t2 & UCOL_PRIMARYMASK) >> 16;
                lh->gapsHi[3*st+1] = (t1 & UCOL_SECONDARYMASK) << 16 | (t2 & UCOL_SECONDARYMASK) << 8;
                //lh->gapsHi[3*st+2] = (UCOL_TERTIARYORDER(t1)) << 24 | (UCOL_TERTIARYORDER(t2)) << 16;
                lh->gapsHi[3*st+2] = (t1&0x3f) << 24 | (t2&0x3f) << 16;
                //pos--;
                //t1 = *(CETable+3*(pos));
                //t2 = *(CETable+3*(pos)+1);
                t1 = lh->baseCE;
                t2 = lh->baseContCE;
                lh->gapsLo[3*st] = (t1 & UCOL_PRIMARYMASK) | (t2 & UCOL_PRIMARYMASK) >> 16;
                lh->gapsLo[3*st+1] = (t1 & UCOL_SECONDARYMASK) << 16 | (t2 & UCOL_SECONDARYMASK) << 8;
                lh->gapsLo[3*st+2] = (t1&0x3f) << 24 | (t2&0x3f) << 16;
            }
        }
    }
}


#define ucol_countBytes(value, noOfBytes)   \
{                               \
    uint32_t mask = 0xFFFFFFFF;   \
    (noOfBytes) = 0;              \
    while(mask != 0) {            \
    if(((value) & mask) != 0) { \
    (noOfBytes)++;            \
    }                           \
    mask >>= 8;                 \
    }                             \
}

static uint32_t ucol_getNextGenerated(ucolCEGenerator *g, UErrorCode *status) {
    if(U_SUCCESS(*status)) {
        g->current = ucol_nextWeight(g->ranges, &g->noOfRanges);
    }
    return g->current;
}

static uint32_t ucol_getSimpleCEGenerator(ucolCEGenerator *g, UColToken *tok, uint32_t strength, UErrorCode *status) {
    /* TODO: rename to enum names */
    uint32_t high, low, count=1;
    uint32_t maxByte = (strength == UCOL_TERTIARY)?0x3F:0xFF;

    if(strength == UCOL_SECONDARY) {
        low = UCOL_COMMON_TOP2<<24;
        high = 0xFFFFFFFF;
        count = 0xFF - UCOL_COMMON_TOP2;
    } else {
        low = UCOL_BYTE_COMMON << 24; //0x05000000;
        high = 0x40000000;
        count = 0x40 - UCOL_BYTE_COMMON;
    }

    if(tok->next != NULL && tok->next->strength == strength) {
        count = tok->next->toInsert;
    }

    g->noOfRanges = ucol_allocWeights(low, high, count, maxByte, g->ranges);
    g->current = UCOL_BYTE_COMMON<<24;

    if(g->noOfRanges == 0) {
        *status = U_INTERNAL_PROGRAM_ERROR;
    }
    return g->current;
}

static uint32_t ucol_getCEGenerator(ucolCEGenerator *g, uint32_t* lows, uint32_t* highs, UColToken *tok, uint32_t fStrength, UErrorCode *status) {
    uint32_t strength = tok->strength;
    uint32_t low = lows[fStrength*3+strength];
    uint32_t high = highs[fStrength*3+strength];
    uint32_t maxByte = 0;
    if(strength == UCOL_TERTIARY) {
        maxByte = 0x3F;
    } else if(strength == UCOL_PRIMARY) {
        maxByte = 0xFE;
    } else {
        maxByte = 0xFF;
    }

    uint32_t count = tok->toInsert;

    if(low >= high && strength > UCOL_PRIMARY) {
        int32_t s = strength;
        for(;;) {
            s--;
            if(lows[fStrength*3+s] != highs[fStrength*3+s]) {
                if(strength == UCOL_SECONDARY) {
                    if (low < UCOL_COMMON_TOP2<<24 ) {
                       // Override if low range is less than UCOL_COMMON_TOP2.
		        low = UCOL_COMMON_TOP2<<24;
                    }
                    high = 0xFFFFFFFF;
                } else {
                    // Override if low range is less than UCOL_COMMON_BOT3.
		    if ( low < UCOL_COMMON_BOT3<<24 ) {
                        low = UCOL_COMMON_BOT3<<24;
		    }
                    high = 0x40000000;
                }
                break;
            }
            if(s<0) {
                *status = U_INTERNAL_PROGRAM_ERROR;
                return 0;
            }
        }
    }

    if(low < 0x02000000) {
        // We must not use CE weight byte 02, so we set it as the minimum lower bound.
        // See http://site.icu-project.org/design/collation/bytes
        low = 0x02000000;
    }

    if(strength == UCOL_SECONDARY) { /* similar as simple */
        if(low >= (UCOL_COMMON_BOT2<<24) && low < (uint32_t)(UCOL_COMMON_TOP2<<24)) {
            low = UCOL_COMMON_TOP2<<24;
        }
        if(high > (UCOL_COMMON_BOT2<<24) && high < (uint32_t)(UCOL_COMMON_TOP2<<24)) {
            high = UCOL_COMMON_TOP2<<24;
        }
        if(low < (UCOL_COMMON_BOT2<<24)) {
            g->noOfRanges = ucol_allocWeights(UCOL_BYTE_UNSHIFTED_MIN<<24, high, count, maxByte, g->ranges);
            g->current = ucol_nextWeight(g->ranges, &g->noOfRanges);
            //g->current = UCOL_COMMON_BOT2<<24;
            return g->current;
        }
    }

    g->noOfRanges = ucol_allocWeights(low, high, count, maxByte, g->ranges);
    if(g->noOfRanges == 0) {
        *status = U_INTERNAL_PROGRAM_ERROR;
    }
    g->current = ucol_nextWeight(g->ranges, &g->noOfRanges);
    return g->current;
}

static
uint32_t u_toLargeKana(const UChar *source, const uint32_t sourceLen, UChar *resBuf, const uint32_t resLen, UErrorCode *status) {
    uint32_t i = 0;
    UChar c;

    if(U_FAILURE(*status)) {
        return 0;
    }

    if(sourceLen > resLen) {
        *status = U_MEMORY_ALLOCATION_ERROR;
        return 0;
    }

    for(i = 0; i < sourceLen; i++) {
        c = source[i];
        if(0x3041 <= c && c <= 0x30FA) { /* Kana range */
            switch(c - 0x3000) {
            case 0x41: case 0x43: case 0x45: case 0x47: case 0x49: case 0x63: case 0x83: case 0x85: case 0x8E:
            case 0xA1: case 0xA3: case 0xA5: case 0xA7: case 0xA9: case 0xC3: case 0xE3: case 0xE5: case 0xEE:
                c++;
                break;
            case 0xF5:
                c = 0x30AB;
                break;
            case 0xF6:
                c = 0x30B1;
                break;
            }
        }
        resBuf[i] = c;
    }
    return sourceLen;
}

static
uint32_t u_toSmallKana(const UChar *source, const uint32_t sourceLen, UChar *resBuf, const uint32_t resLen, UErrorCode *status) {
    uint32_t i = 0;
    UChar c;

    if(U_FAILURE(*status)) {
        return 0;
    }

    if(sourceLen > resLen) {
        *status = U_MEMORY_ALLOCATION_ERROR;
        return 0;
    }

    for(i = 0; i < sourceLen; i++) {
        c = source[i];
        if(0x3041 <= c && c <= 0x30FA) { /* Kana range */
            switch(c - 0x3000) {
            case 0x42: case 0x44: case 0x46: case 0x48: case 0x4A: case 0x64: case 0x84: case 0x86: case 0x8F:
            case 0xA2: case 0xA4: case 0xA6: case 0xA8: case 0xAA: case 0xC4: case 0xE4: case 0xE6: case 0xEF:
                c--;
                break;
            case 0xAB:
                c = 0x30F5;
                break;
            case 0xB1:
                c = 0x30F6;
                break;
            }
        }
        resBuf[i] = c;
    }
    return sourceLen;
}

static
uint8_t ucol_uprv_getCaseBits(const UCollator *UCA, const UChar *src, uint32_t len, UErrorCode *status) {
    uint32_t i = 0;
    UChar n[128];
    uint32_t nLen = 0;
    uint32_t uCount = 0, lCount = 0;

    collIterate s;
    uint32_t order = 0;

    if(U_FAILURE(*status)) {
        return UCOL_LOWER_CASE;
    }

    nLen = unorm_normalize(src, len, UNORM_NFKD, 0, n, 128, status);
    if(U_SUCCESS(*status)) {
        for(i = 0; i < nLen; i++) {
            uprv_init_collIterate(UCA, &n[i], 1, &s, status);
            order = ucol_getNextCE(UCA, &s, status);
            if(isContinuation(order)) {
                *status = U_INTERNAL_PROGRAM_ERROR;
                return UCOL_LOWER_CASE;
            }
            if((order&UCOL_CASE_BIT_MASK)== UCOL_UPPER_CASE) {
                uCount++;
            } else {
                if(u_islower(n[i])) {
                    lCount++;
                } else if(U_SUCCESS(*status)) {
                    UChar sk[1], lk[1];
                    u_toSmallKana(&n[i], 1, sk, 1, status);
                    u_toLargeKana(&n[i], 1, lk, 1, status);
                    if(sk[0] == n[i] && lk[0] != n[i]) {
                        lCount++;
                    }
                }
            }
        }
    }

    if(uCount != 0 && lCount != 0) {
        return UCOL_MIXED_CASE;
    } else if(uCount != 0) {
        return UCOL_UPPER_CASE;
    } else {
        return UCOL_LOWER_CASE;
    }
}


U_CFUNC void ucol_doCE(UColTokenParser *src, uint32_t *CEparts, UColToken *tok, UErrorCode *status) {
    /* this one makes the table and stuff */
    uint32_t noOfBytes[3];
    uint32_t i;

    for(i = 0; i<3; i++) {
        ucol_countBytes(CEparts[i], noOfBytes[i]);
    }

    /* Here we have to pack CEs from parts */

    uint32_t CEi = 0;
    uint32_t value = 0;

    while(2*CEi<noOfBytes[0] || CEi<noOfBytes[1] || CEi<noOfBytes[2]) {
        if(CEi > 0) {
            value = UCOL_CONTINUATION_MARKER; /* Continuation marker */
        } else {
            value = 0;
        }

        if(2*CEi<noOfBytes[0]) {
            value |= ((CEparts[0]>>(32-16*(CEi+1))) & 0xFFFF) << 16;
        }
        if(CEi<noOfBytes[1]) {
            value |= ((CEparts[1]>>(32-8*(CEi+1))) & 0xFF) << 8;
        }
        if(CEi<noOfBytes[2]) {
            value |= ((CEparts[2]>>(32-8*(CEi+1))) & 0x3F);
        }
        tok->CEs[CEi] = value;
        CEi++;
    }
    if(CEi == 0) { /* totally ignorable */
        tok->noOfCEs = 1;
        tok->CEs[0] = 0;
    } else { /* there is at least something */
        tok->noOfCEs = CEi;
    }


    // we want to set case bits here and now, not later.
    // Case bits handling
    if(tok->CEs[0] != 0) { // case bits should be set only for non-ignorables
        tok->CEs[0] &= 0xFFFFFF3F; // Clean the case bits field
        int32_t cSize = (tok->source & 0xFF000000) >> 24;
        UChar *cPoints = (tok->source & 0x00FFFFFF) + src->source;

        if(cSize > 1) {
            // Do it manually
            tok->CEs[0] |= ucol_uprv_getCaseBits(src->UCA, cPoints, cSize, status);
        } else {
            // Copy it from the UCA
            uint32_t caseCE = ucol_getFirstCE(src->UCA, cPoints[0], status);
            tok->CEs[0] |= (caseCE & 0xC0);
        }
    }

#if UCOL_DEBUG==2
    fprintf(stderr, "%04X str: %i, [%08X, %08X, %08X]: tok: ", tok->debugSource, tok->strength, CEparts[0] >> (32-8*noOfBytes[0]), CEparts[1] >> (32-8*noOfBytes[1]), CEparts[2]>> (32-8*noOfBytes[2]));
    for(i = 0; i<tok->noOfCEs; i++) {
        fprintf(stderr, "%08X ", tok->CEs[i]);
    }
    fprintf(stderr, "\n");
#endif
}

U_CFUNC void ucol_initBuffers(UColTokenParser *src, UColTokListHeader *lh, UErrorCode *status) {
    ucolCEGenerator Gens[UCOL_CE_STRENGTH_LIMIT];
    uint32_t CEparts[UCOL_CE_STRENGTH_LIMIT];

    UColToken *tok = lh->last;
    uint32_t t[UCOL_STRENGTH_LIMIT];

    uprv_memset(t, 0, UCOL_STRENGTH_LIMIT*sizeof(uint32_t));

    tok->toInsert = 1;
    t[tok->strength] = 1;

    while(tok->previous != NULL) {
        if(tok->previous->strength < tok->strength) { /* going up */
            t[tok->strength] = 0;
            t[tok->previous->strength]++;
        } else if(tok->previous->strength > tok->strength) { /* going down */
            t[tok->previous->strength] = 1;
        } else {
            t[tok->strength]++;
        }
        tok=tok->previous;
        tok->toInsert = t[tok->strength];
    }

    tok->toInsert = t[tok->strength];
    ucol_inv_getGapPositions(src, lh, status);

#if UCOL_DEBUG
    fprintf(stderr, "BaseCE: %08X %08X\n", lh->baseCE, lh->baseContCE);
    int32_t j = 2;
    for(j = 2; j >= 0; j--) {
        fprintf(stderr, "gapsLo[%i] [%08X %08X %08X]\n", j, lh->gapsLo[j*3], lh->gapsLo[j*3+1], lh->gapsLo[j*3+2]);
        fprintf(stderr, "gapsHi[%i] [%08X %08X %08X]\n", j, lh->gapsHi[j*3], lh->gapsHi[j*3+1], lh->gapsHi[j*3+2]);
    }
    tok=&lh->first[UCOL_TOK_POLARITY_POSITIVE];

    do {
        fprintf(stderr,"%i", tok->strength);
        tok = tok->next;
    } while(tok != NULL);
    fprintf(stderr, "\n");

    tok=&lh->first[UCOL_TOK_POLARITY_POSITIVE];

    do {
        fprintf(stderr,"%i", tok->toInsert);
        tok = tok->next;
    } while(tok != NULL);
#endif

    tok = lh->first;
    uint32_t fStrength = UCOL_IDENTICAL;
    uint32_t initStrength = UCOL_IDENTICAL;


    CEparts[UCOL_PRIMARY] = (lh->baseCE & UCOL_PRIMARYMASK) | (lh->baseContCE & UCOL_PRIMARYMASK) >> 16;
    CEparts[UCOL_SECONDARY] = (lh->baseCE & UCOL_SECONDARYMASK) << 16 | (lh->baseContCE & UCOL_SECONDARYMASK) << 8;
    CEparts[UCOL_TERTIARY] = (UCOL_TERTIARYORDER(lh->baseCE)) << 24 | (UCOL_TERTIARYORDER(lh->baseContCE)) << 16;

    while (tok != NULL && U_SUCCESS(*status)) {
        fStrength = tok->strength;
        if(fStrength < initStrength) {
            initStrength = fStrength;
            if(lh->pos[fStrength] == -1) {
                while(lh->pos[fStrength] == -1 && fStrength > 0) {
                    fStrength--;
                }
                if(lh->pos[fStrength] == -1) {
                    *status = U_INTERNAL_PROGRAM_ERROR;
                    return;
                }
            }
            if(initStrength == UCOL_TERTIARY) { /* starting with tertiary */
                CEparts[UCOL_PRIMARY] = lh->gapsLo[fStrength*3];
                CEparts[UCOL_SECONDARY] = lh->gapsLo[fStrength*3+1];
                /*CEparts[UCOL_TERTIARY] = ucol_getCEGenerator(&Gens[2], lh->gapsLo[fStrength*3+2], lh->gapsHi[fStrength*3+2], tok, UCOL_TERTIARY); */
                CEparts[UCOL_TERTIARY] = ucol_getCEGenerator(&Gens[UCOL_TERTIARY], lh->gapsLo, lh->gapsHi, tok, fStrength, status);
            } else if(initStrength == UCOL_SECONDARY) { /* secondaries */
                CEparts[UCOL_PRIMARY] = lh->gapsLo[fStrength*3];
                /*CEparts[1] = ucol_getCEGenerator(&Gens[1], lh->gapsLo[fStrength*3+1], lh->gapsHi[fStrength*3+1], tok, 1);*/
                CEparts[UCOL_SECONDARY] = ucol_getCEGenerator(&Gens[UCOL_SECONDARY], lh->gapsLo, lh->gapsHi, tok, fStrength,  status);
                CEparts[UCOL_TERTIARY] = ucol_getSimpleCEGenerator(&Gens[UCOL_TERTIARY], tok, UCOL_TERTIARY, status);
            } else { /* primaries */
                /*CEparts[UCOL_PRIMARY] = ucol_getCEGenerator(&Gens[0], lh->gapsLo[0], lh->gapsHi[0], tok, UCOL_PRIMARY);*/
                CEparts[UCOL_PRIMARY] = ucol_getCEGenerator(&Gens[UCOL_PRIMARY], lh->gapsLo, lh->gapsHi, tok, fStrength,  status);
                CEparts[UCOL_SECONDARY] = ucol_getSimpleCEGenerator(&Gens[UCOL_SECONDARY], tok, UCOL_SECONDARY, status);
                CEparts[UCOL_TERTIARY] = ucol_getSimpleCEGenerator(&Gens[UCOL_TERTIARY], tok, UCOL_TERTIARY, status);
            }
        } else {
            if(tok->strength == UCOL_TERTIARY) {
                CEparts[UCOL_TERTIARY] = ucol_getNextGenerated(&Gens[UCOL_TERTIARY], status);
            } else if(tok->strength == UCOL_SECONDARY) {
                CEparts[UCOL_SECONDARY] = ucol_getNextGenerated(&Gens[UCOL_SECONDARY], status);
                CEparts[UCOL_TERTIARY] = ucol_getSimpleCEGenerator(&Gens[UCOL_TERTIARY], tok, UCOL_TERTIARY, status);
            } else if(tok->strength == UCOL_PRIMARY) {
                CEparts[UCOL_PRIMARY] = ucol_getNextGenerated(&Gens[UCOL_PRIMARY], status);
                CEparts[UCOL_SECONDARY] = ucol_getSimpleCEGenerator(&Gens[UCOL_SECONDARY], tok, UCOL_SECONDARY, status);
                CEparts[UCOL_TERTIARY] = ucol_getSimpleCEGenerator(&Gens[UCOL_TERTIARY], tok, UCOL_TERTIARY, status);
            }
        }
        ucol_doCE(src, CEparts, tok, status);
        tok = tok->next;
    }
}

U_CFUNC void ucol_createElements(UColTokenParser *src, tempUCATable *t, UColTokListHeader *lh, UErrorCode *status) {
    UCAElements el;
    UColToken *tok = lh->first;
    UColToken *expt = NULL;
    uint32_t i = 0, j = 0;
    UChar32 fcdHighStart;
    const uint16_t *fcdTrieIndex = unorm_getFCDTrieIndex(fcdHighStart, status);

    while(tok != NULL && U_SUCCESS(*status)) {
        /* first, check if there are any expansions */
        /* if there are expansions, we need to do a little bit more processing */
        /* since parts of expansion can be tailored, while others are not */
        if(tok->expansion != 0) {
            uint32_t len = tok->expansion >> 24;
            uint32_t currentSequenceLen = len;
            uint32_t expOffset = tok->expansion & 0x00FFFFFF;
            //uint32_t exp = currentSequenceLen | expOffset;
            UColToken exp;
            exp.source = currentSequenceLen | expOffset;
            exp.rulesToParseHdl = &(src->source);

            while(len > 0) {
                currentSequenceLen = len;
                while(currentSequenceLen > 0) {
                    exp.source = (currentSequenceLen << 24) | expOffset;
                    if((expt = (UColToken *)uhash_get(src->tailored, &exp)) != NULL && expt->strength != UCOL_TOK_RESET) { /* expansion is tailored */
                        uint32_t noOfCEsToCopy = expt->noOfCEs;
                        for(j = 0; j<noOfCEsToCopy; j++) {
                            tok->expCEs[tok->noOfExpCEs + j] = expt->CEs[j];
                        }
                        tok->noOfExpCEs += noOfCEsToCopy;
                        // Smart people never try to add codepoints and CEs.
                        // For some odd reason, it won't work.
                        expOffset += currentSequenceLen; //noOfCEsToCopy;
                        len -= currentSequenceLen; //noOfCEsToCopy;
                        break;
                    } else {
                        currentSequenceLen--;
                    }
                }
                if(currentSequenceLen == 0) { /* couldn't find any tailored subsequence */
                    /* will have to get one from UCA */
                    /* first, get the UChars from the rules */
                    /* then pick CEs out until there is no more and stuff them into expansion */
                    collIterate s;
                    uint32_t order = 0;
                    uprv_init_collIterate(src->UCA, expOffset + src->source, 1, &s, status);

                    for(;;) {
                        order = ucol_getNextCE(src->UCA, &s, status);
                        if(order == UCOL_NO_MORE_CES) {
                            break;
                        }
                        tok->expCEs[tok->noOfExpCEs++] = order;
                    }
                    expOffset++;
                    len--;
                }
            }
        } else {
            tok->noOfExpCEs = 0;
        }

        /* set the ucaelement with obtained values */
        el.noOfCEs = tok->noOfCEs + tok->noOfExpCEs;
        /* copy CEs */
        for(i = 0; i<tok->noOfCEs; i++) {
            el.CEs[i] = tok->CEs[i];
        }
        for(i = 0; i<tok->noOfExpCEs; i++) {
            el.CEs[i+tok->noOfCEs] = tok->expCEs[i];
        }

        /* copy UChars */
        // We kept prefix and source kind of together, as it is a kind of a contraction.
        // However, now we have to slice the prefix off the main thing -
        el.prefix = el.prefixChars;
        el.cPoints = el.uchars;
        if(tok->prefix != 0) { // we will just copy the prefix here, and adjust accordingly in the
            // addPrefix function in ucol_elm. The reason is that we need to add both composed AND
            // decomposed elements to the unsaf table.
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
            for(i = 0; i<el.cSize; i++) {
                if(UCOL_ISJAMO(el.cPoints[i])) {
                    t->image->jamoSpecial = TRUE;
                }
            }
            if (!src->buildCCTabFlag && el.cSize > 0) {
                // Check the trailing canonical combining class (tccc) of the last character.
                const UChar *s = el.cPoints + el.cSize;
                uint16_t fcd = unorm_prevFCD16(fcdTrieIndex, fcdHighStart, el.cPoints, s);
                if ((fcd & 0xff) != 0) {
                    src->buildCCTabFlag = TRUE;
                }
            }
        }

        /* and then, add it */
#if UCOL_DEBUG==2
        fprintf(stderr, "Adding: %04X with %08X\n", el.cPoints[0], el.CEs[0]);
#endif
        uprv_uca_addAnElement(t, &el, status);

#if UCOL_DEBUG_DUPLICATES
        if(*status != U_ZERO_ERROR) {
            fprintf(stderr, "replaced CE for %04X with CE for %04X\n", el.cPoints[0], tok->debugSource);
            *status = U_ZERO_ERROR;
        }
#endif

        tok = tok->next;
    }
}

U_CDECL_BEGIN
static UBool U_CALLCONV
_processUCACompleteIgnorables(const void *context, UChar32 start, UChar32 limit, uint32_t value) {
    UErrorCode status = U_ZERO_ERROR;
    tempUCATable *t = (tempUCATable *)context;
    if(value == 0) {
        while(start < limit) {
            uint32_t CE = utrie_get32(t->mapping, start, NULL);
            if(CE == UCOL_NOT_FOUND) {
                UCAElements el;
                el.isThai = FALSE;
                el.prefixSize = 0;
                el.prefixChars[0] = 0;
                el.prefix = el.prefixChars;
                el.cPoints = el.uchars;

                el.cSize = 0;
                UTF_APPEND_CHAR(el.uchars, el.cSize, 1024, start);

                el.noOfCEs = 1;
                el.CEs[0] = 0;
                uprv_uca_addAnElement(t, &el, &status);

            }
            start++;
        }
    }
    if(U_FAILURE(status)) {
        return FALSE;
    } else {
        return TRUE;
    }
}
U_CDECL_END

static void
ucol_uprv_bld_copyRangeFromUCA(UColTokenParser *src, tempUCATable *t,
                               UChar32 start, UChar32 end,
                               UErrorCode *status)
{
    //UChar decomp[256];
    uint32_t CE = UCOL_NOT_FOUND;
    UChar32 u = 0;
    UCAElements el;
    el.isThai = FALSE;
    el.prefixSize = 0;
    el.prefixChars[0] = 0;
    collIterate colIt;

    if(U_SUCCESS(*status)) {
        for(u = start; u<=end; u++) {
            if((CE = utrie_get32(t->mapping, u, NULL)) == UCOL_NOT_FOUND
                /* this test is for contractions that are missing the starting element. */
                || ((isCntTableElement(CE)) &&
                (uprv_cnttab_getCE(t->contractions, CE, 0, status) == UCOL_NOT_FOUND))
                )
            {
                el.cSize = 0;
                U16_APPEND_UNSAFE(el.uchars, el.cSize, u);
                //decomp[0] = (UChar)u;
                //el.uchars[0] = (UChar)u;
                el.cPoints = el.uchars;
                //el.cSize = 1;
                el.noOfCEs = 0;
                el.prefix = el.prefixChars;
                el.prefixSize = 0;
                //uprv_init_collIterate(src->UCA, decomp, 1, &colIt);
                // We actually want to check whether this element is a special
                // If it is an implicit element (hangul, CJK - we want to copy the
                // special, not the resolved CEs) - for hangul, copying resolved
                // would just make things the same (there is an expansion and it
                // takes approximately the same amount of time to resolve as
                // falling back to the UCA).
                /*
                UTRIE_GET32(src->UCA->mapping, u, CE);
                tag = getCETag(CE);
                if(tag == HANGUL_SYLLABLE_TAG || tag == CJK_IMPLICIT_TAG
                || tag == IMPLICIT_TAG || tag == TRAIL_SURROGATE_TAG
                || tag == LEAD_SURROGATE_TAG) {
                el.CEs[el.noOfCEs++] = CE;
                } else {
                */
                // It turns out that it does not make sense to keep implicits
                // unresolved. The cost of resolving them is big enough so that
                // it doesn't make any difference whether we have to go to the UCA
                // or not.
                {
                    uprv_init_collIterate(src->UCA, el.uchars, el.cSize, &colIt, status);
                    while(CE != UCOL_NO_MORE_CES) {
                        CE = ucol_getNextCE(src->UCA, &colIt, status);
                        if(CE != UCOL_NO_MORE_CES) {
                            el.CEs[el.noOfCEs++] = CE;
                        }
                    }
                }
                uprv_uca_addAnElement(t, &el, status);
            }
        }
    }
}

U_CFUNC UCATableHeader *
ucol_assembleTailoringTable(UColTokenParser *src, UErrorCode *status) {
    U_NAMESPACE_USE

    uint32_t i = 0;
    if(U_FAILURE(*status)) {
        return NULL;
    }
    /*
    2.  Eliminate the negative lists by doing the following for each non-null negative list:
    o   if previousCE(baseCE, strongestN) != some ListHeader X's baseCE,
    create new ListHeader X
    o   reverse the list, add to the end of X's positive list. Reset the strength of the
    first item you add, based on the stronger strength levels of the two lists.
    */
    /*
    3.  For each ListHeader with a non-null positive list:
    */
    /*
    o   Find all character strings with CEs between the baseCE and the
    next/previous CE, at the strength of the first token. Add these to the
    tailoring.
    ? That is, if UCA has ...  x <<< X << x' <<< X' < y ..., and the
    tailoring has & x < z...
    ? Then we change the tailoring to & x  <<< X << x' <<< X' < z ...
    */
    /* It is possible that this part should be done even while constructing list */
    /* The problem is that it is unknown what is going to be the strongest weight */
    /* So we might as well do it here */

    /*
    o   Allocate CEs for each token in the list, based on the total number N of the
    largest level difference, and the gap G between baseCE and nextCE at that
    level. The relation * between the last item and nextCE is the same as the
    strongest strength.
    o   Example: baseCE < a << b <<< q << c < d < e * nextCE(X,1)
    ? There are 3 primary items: a, d, e. Fit them into the primary gap.
    Then fit b and c into the secondary gap between a and d, then fit q
    into the tertiary gap between b and c.

    o   Example: baseCE << b <<< q << c * nextCE(X,2)
    ? There are 2 secondary items: b, c. Fit them into the secondary gap.
    Then fit q into the tertiary gap between b and c.
    o   When incrementing primary values, we will not cross high byte
    boundaries except where there is only a single-byte primary. That is to
    ensure that the script reordering will continue to work.
    */
    UCATableHeader *image = (UCATableHeader *)uprv_malloc(sizeof(UCATableHeader));
    /* test for NULL */
    if (image == NULL) {
        *status = U_MEMORY_ALLOCATION_ERROR;
        return NULL;
    }
    uprv_memcpy(image, src->UCA->image, sizeof(UCATableHeader));

    for(i = 0; i<src->resultLen; i++) {
        /* now we need to generate the CEs */
        /* We stuff the initial value in the buffers, and increase the appropriate buffer */
        /* According to strength                                                          */
        if(U_SUCCESS(*status)) {
            if(src->lh[i].first) { // if there are any elements
                // due to the way parser works, subsequent tailorings
                // may remove all the elements from a sequence, therefore
                // leaving an empty tailoring sequence.
                ucol_initBuffers(src, &src->lh[i], status);
            }
        }
        if(U_FAILURE(*status)) {
            uprv_free(image);
            return NULL;
        }
    }

    if(src->varTop != NULL) { /* stuff the variable top value */
        src->opts->variableTopValue = (*(src->varTop->CEs))>>16;
        /* remove it from the list */
        if(src->varTop->listHeader->first == src->varTop) { /* first in list */
            src->varTop->listHeader->first = src->varTop->next;
        }
        if(src->varTop->listHeader->last == src->varTop) { /* first in list */
            src->varTop->listHeader->last = src->varTop->previous;
        }
        if(src->varTop->next != NULL) {
            src->varTop->next->previous = src->varTop->previous;
        }
        if(src->varTop->previous != NULL) {
            src->varTop->previous->next = src->varTop->next;
        }
    }


    tempUCATable *t = uprv_uca_initTempTable(image, src->opts, src->UCA, NOT_FOUND_TAG, NOT_FOUND_TAG, status);
    if(U_FAILURE(*status)) {
        uprv_free(image);
        return NULL;
    }


    /* After this, we have assigned CE values to all regular CEs      */
    /* now we will go through list once more and resolve expansions,  */
    /* make UCAElements structs and add them to table                 */
    for(i = 0; i<src->resultLen; i++) {
        /* now we need to generate the CEs */
        /* We stuff the initial value in the buffers, and increase the appropriate buffer */
        /* According to strength                                                          */
        if(U_SUCCESS(*status)) {
            ucol_createElements(src, t, &src->lh[i], status);
        }
    }

    UCAElements el;
    el.isThai = FALSE;
    el.prefixSize = 0;
    el.prefixChars[0] = 0;

    /* add latin-1 stuff */
    ucol_uprv_bld_copyRangeFromUCA(src, t, 0, 0xFF, status);

    /* add stuff for copying */
    if(src->copySet != NULL) {
        int32_t i = 0;
        UnicodeSet *set = (UnicodeSet *)src->copySet;
        for(i = 0; i < set->getRangeCount(); i++) {
            ucol_uprv_bld_copyRangeFromUCA(src, t, set->getRangeStart(i), set->getRangeEnd(i), status);
        }
    }

    if(U_SUCCESS(*status)) {
        /* copy contractions from the UCA - this is felt mostly for cyrillic*/

        uint32_t tailoredCE = UCOL_NOT_FOUND;
        //UChar *conts = (UChar *)((uint8_t *)src->UCA->image + src->UCA->image->UCAConsts+sizeof(UCAConstants));
        UChar *conts = (UChar *)((uint8_t *)src->UCA->image + src->UCA->image->contractionUCACombos);
        UCollationElements *ucaEl = ucol_openElements(src->UCA, NULL, 0, status);
        // Check for null pointer
        if (ucaEl == NULL) {
        	*status = U_MEMORY_ALLOCATION_ERROR;
        	return NULL;
        }
        while(*conts != 0) {
            /*tailoredCE = ucmpe32_get(t->mapping, *conts);*/
            tailoredCE = utrie_get32(t->mapping, *conts, NULL);
            if(tailoredCE != UCOL_NOT_FOUND) {
                UBool needToAdd = TRUE;
                if(isCntTableElement(tailoredCE)) {
                    if(uprv_cnttab_isTailored(t->contractions, tailoredCE, conts+1, status) == TRUE) {
                        needToAdd = FALSE;
                    }
                }
                if (!needToAdd && isPrefix(tailoredCE) && *(conts+1)==0) {
                    UCAElements elm;
                    elm.cPoints = el.uchars;
                    elm.noOfCEs = 0;
                    elm.uchars[0] = *conts;
                    elm.uchars[1] = 0;
                    elm.cSize = 1;
                    elm.prefixChars[0] = *(conts+2);
                    elm.isThai = FALSE;
                    elm.prefix = elm.prefixChars;
                    elm.prefixSize = 1;
                    UCAElements *prefixEnt=(UCAElements *)uhash_get(t->prefixLookup, &elm);
                    if ((prefixEnt==NULL) || *(prefixEnt->prefix)!=*(conts+2)) {
                        needToAdd = TRUE;
                    }
                }
                if(src->removeSet != NULL && uset_contains(src->removeSet, *conts)) {
                    needToAdd = FALSE;
                }

                if(needToAdd == TRUE) { // we need to add if this contraction is not tailored.
                    if (*(conts+1) != 0) {  // contractions
                        el.prefix = el.prefixChars;
                        el.prefixSize = 0;
                        el.cPoints = el.uchars;
                        el.noOfCEs = 0;
                        el.uchars[0] = *conts;
                        el.uchars[1] = *(conts+1);
                        if(*(conts+2)!=0) {
                            el.uchars[2] = *(conts+2);
                            el.cSize = 3;
                        } else {
                            el.cSize = 2;
                        }
                        ucol_setText(ucaEl, el.uchars, el.cSize, status);
                    }
                    else { // pre-context character
                        UChar str[4] = { 0 };
                        int32_t len=0;
                        int32_t preKeyLen=0;
                        
                        el.cPoints = el.uchars;
                        el.noOfCEs = 0;
                        el.uchars[0] = *conts;
                        el.uchars[1] = 0;
                        el.cSize = 1;
                        el.prefixChars[0] = *(conts+2);
                        el.prefix = el.prefixChars;
                        el.prefixSize = 1;
                        if (el.prefixChars[0]!=0) {
                            // get CE of prefix character first
                            str[0]=el.prefixChars[0];
                            str[1]=0;
                            ucol_setText(ucaEl, str, 1, status);
                            while ((int32_t)(el.CEs[el.noOfCEs] = ucol_next(ucaEl, status))
                                    != UCOL_NULLORDER) {
                                preKeyLen++;  // count number of keys for prefix character
                            }
                            str[len++] = el.prefixChars[0];
                        }

                        str[len++] = el.uchars[0];
                        str[len]=0;
                        ucol_setText(ucaEl, str, len, status);
                        // Skip the keys for prefix character, then copy the rest to el.
                        while ((preKeyLen-->0) && 
                               (int32_t)(el.CEs[el.noOfCEs] = ucol_next(ucaEl, status)) != UCOL_NULLORDER) {
                            continue;
                        }
                           
                    }
                    while ((int32_t)(el.CEs[el.noOfCEs] = ucol_next(ucaEl, status)) != UCOL_NULLORDER) {
                        el.noOfCEs++;
                    }
                    uprv_uca_addAnElement(t, &el, status);
                }

            } else if(src->removeSet != NULL && uset_contains(src->removeSet, *conts)) {
                ucol_uprv_bld_copyRangeFromUCA(src, t, *conts, *conts, status);
            }
            conts+=3;
        }
        ucol_closeElements(ucaEl);
    }

    // Add completely ignorable elements
    utrie_enum(&t->UCA->mapping, NULL, _processUCACompleteIgnorables, t);

    // add tailoring characters related canonical closures
    uprv_uca_canonicalClosure(t, src, NULL, status);

    /* still need to produce compatibility closure */

    UCATableHeader *myData = uprv_uca_assembleTable(t, status);

    uprv_uca_closeTempTable(t);
    uprv_free(image);

    return myData;
}

U_CDECL_BEGIN
static UBool U_CALLCONV
ucol_bld_cleanup(void)
{
    udata_close(invUCA_DATA_MEM);
    invUCA_DATA_MEM = NULL;
    _staticInvUCA = NULL;
    return TRUE;
}
U_CDECL_END

U_CAPI const InverseUCATableHeader * U_EXPORT2
ucol_initInverseUCA(UErrorCode *status)
{
    if(U_FAILURE(*status)) return NULL;

    UBool needsInit;
    UMTX_CHECK(NULL, (_staticInvUCA == NULL), needsInit);

    if(needsInit) {
        InverseUCATableHeader *newInvUCA = NULL;
        UDataMemory *result = udata_openChoice(U_ICUDATA_COLL, INVC_DATA_TYPE, INVC_DATA_NAME, isAcceptableInvUCA, NULL, status);

        if(U_FAILURE(*status)) {
            if (result) {
                udata_close(result);
            }
            // This is not needed, as we are talking about
            // memory we got from UData
            //uprv_free(newInvUCA);
        }

        if(result != NULL) { /* It looks like sometimes we can fail to find the data file */
            newInvUCA = (InverseUCATableHeader *)udata_getMemory(result);
            UCollator *UCA = ucol_initUCA(status);
            // UCA versions of UCA and inverse UCA should match
            if(uprv_memcmp(newInvUCA->UCAVersion, UCA->image->UCAVersion, sizeof(UVersionInfo)) != 0) {
                *status = U_INVALID_FORMAT_ERROR;
                udata_close(result);
                return NULL;
            }

            umtx_lock(NULL);
            if(_staticInvUCA == NULL) {
                invUCA_DATA_MEM = result;
                _staticInvUCA = newInvUCA;
                result = NULL;
                newInvUCA = NULL;
            }
            umtx_unlock(NULL);

            if(newInvUCA != NULL) {
                udata_close(result);
                // This is not needed, as we are talking about
                // memory we got from UData
                //uprv_free(newInvUCA);
            }
            else {
                ucln_i18n_registerCleanup(UCLN_I18N_UCOL_BLD, ucol_bld_cleanup);
            }
        }
    }
    return _staticInvUCA;
}

/* This is the data that is used for non-script reordering codes. These _must_ be kept
 * in order that they are to be applied as defaults and in synch with the UColReorderCode enum.
 */
static const char* ReorderingTokenNames[] = {
    "SPACE",
    "PUNCT",
    "SYMBOL",
    "CURRENCY",
    "DIGIT",
    NULL
};

static void toUpper(const char* src, char* dst, uint32_t length) {
   for (uint32_t i = 0; *src != '\0' && i < length - 1; ++src, ++dst, ++i) {
       *dst = toupper(*src);
   }
   *dst = '\0';
}

U_INTERNAL int32_t U_EXPORT2 
ucol_findReorderingEntry(const char* name) {
    char buffer[32];
    toUpper(name, buffer, 32);
    for (uint32_t entry = 0; ReorderingTokenNames[entry] != NULL; entry++) {
        if (uprv_strcmp(buffer, ReorderingTokenNames[entry]) == 0) {
            return entry + UCOL_REORDER_CODE_FIRST;
        }
    }
    return USCRIPT_INVALID_CODE;
}

U_NAMESPACE_END

#endif /* #if !UCONFIG_NO_COLLATION */
