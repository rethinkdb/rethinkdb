/*
******************************************************************************
*
*   Copyright (C) 1999-2010, International Business Machines
*   Corporation and others.  All Rights Reserved.
*
******************************************************************************
*   file name:  ubidi.c
*   encoding:   US-ASCII
*   tab size:   8 (not used)
*   indentation:4
*
*   created on: 1999jul27
*   created by: Markus W. Scherer, updated by Matitiahu Allouche
*/

#include "cmemory.h"
#include "unicode/utypes.h"
#include "unicode/ustring.h"
#include "unicode/uchar.h"
#include "unicode/ubidi.h"
#include "ubidi_props.h"
#include "ubidiimp.h"
#include "uassert.h"

/*
 * General implementation notes:
 *
 * Throughout the implementation, there are comments like (W2) that refer to
 * rules of the BiDi algorithm in its version 5, in this example to the second
 * rule of the resolution of weak types.
 *
 * For handling surrogate pairs, where two UChar's form one "abstract" (or UTF-32)
 * character according to UTF-16, the second UChar gets the directional property of
 * the entire character assigned, while the first one gets a BN, a boundary
 * neutral, type, which is ignored by most of the algorithm according to
 * rule (X9) and the implementation suggestions of the BiDi algorithm.
 *
 * Later, adjustWSLevels() will set the level for each BN to that of the
 * following character (UChar), which results in surrogate pairs getting the
 * same level on each of their surrogates.
 *
 * In a UTF-8 implementation, the same thing could be done: the last byte of
 * a multi-byte sequence would get the "real" property, while all previous
 * bytes of that sequence would get BN.
 *
 * It is not possible to assign all those parts of a character the same real
 * property because this would fail in the resolution of weak types with rules
 * that look at immediately surrounding types.
 *
 * As a related topic, this implementation does not remove Boundary Neutral
 * types from the input, but ignores them wherever this is relevant.
 * For example, the loop for the resolution of the weak types reads
 * types until it finds a non-BN.
 * Also, explicit embedding codes are neither changed into BN nor removed.
 * They are only treated the same way real BNs are.
 * As stated before, adjustWSLevels() takes care of them at the end.
 * For the purpose of conformance, the levels of all these codes
 * do not matter.
 *
 * Note that this implementation never modifies the dirProps
 * after the initial setup.
 *
 *
 * In this implementation, the resolution of weak types (Wn),
 * neutrals (Nn), and the assignment of the resolved level (In)
 * are all done in one single loop, in resolveImplicitLevels().
 * Changes of dirProp values are done on the fly, without writing
 * them back to the dirProps array.
 *
 *
 * This implementation contains code that allows to bypass steps of the
 * algorithm that are not needed on the specific paragraph
 * in order to speed up the most common cases considerably,
 * like text that is entirely LTR, or RTL text without numbers.
 *
 * Most of this is done by setting a bit for each directional property
 * in a flags variable and later checking for whether there are
 * any LTR characters or any RTL characters, or both, whether
 * there are any explicit embedding codes, etc.
 *
 * If the (Xn) steps are performed, then the flags are re-evaluated,
 * because they will then not contain the embedding codes any more
 * and will be adjusted for override codes, so that subsequently
 * more bypassing may be possible than what the initial flags suggested.
 *
 * If the text is not mixed-directional, then the
 * algorithm steps for the weak type resolution are not performed,
 * and all levels are set to the paragraph level.
 *
 * If there are no explicit embedding codes, then the (Xn) steps
 * are not performed.
 *
 * If embedding levels are supplied as a parameter, then all
 * explicit embedding codes are ignored, and the (Xn) steps
 * are not performed.
 *
 * White Space types could get the level of the run they belong to,
 * and are checked with a test of (flags&MASK_EMBEDDING) to
 * consider if the paragraph direction should be considered in
 * the flags variable.
 *
 * If there are no White Space types in the paragraph, then
 * (L1) is not necessary in adjustWSLevels().
 */

/* to avoid some conditional statements, use tiny constant arrays */
static const Flags flagLR[2]={ DIRPROP_FLAG(L), DIRPROP_FLAG(R) };
static const Flags flagE[2]={ DIRPROP_FLAG(LRE), DIRPROP_FLAG(RLE) };
static const Flags flagO[2]={ DIRPROP_FLAG(LRO), DIRPROP_FLAG(RLO) };

#define DIRPROP_FLAG_LR(level) flagLR[(level)&1]
#define DIRPROP_FLAG_E(level) flagE[(level)&1]
#define DIRPROP_FLAG_O(level) flagO[(level)&1]

/* UBiDi object management -------------------------------------------------- */

U_CAPI UBiDi * U_EXPORT2
ubidi_open(void)
{
    UErrorCode errorCode=U_ZERO_ERROR;
    return ubidi_openSized(0, 0, &errorCode);
}

U_CAPI UBiDi * U_EXPORT2
ubidi_openSized(int32_t maxLength, int32_t maxRunCount, UErrorCode *pErrorCode) {
    UBiDi *pBiDi;

    /* check the argument values */
    if(pErrorCode==NULL || U_FAILURE(*pErrorCode)) {
        return NULL;
    } else if(maxLength<0 || maxRunCount<0) {
        *pErrorCode=U_ILLEGAL_ARGUMENT_ERROR;
        return NULL;    /* invalid arguments */
    }

    /* allocate memory for the object */
    pBiDi=(UBiDi *)uprv_malloc(sizeof(UBiDi));
    if(pBiDi==NULL) {
        *pErrorCode=U_MEMORY_ALLOCATION_ERROR;
        return NULL;
    }

    /* reset the object, all pointers NULL, all flags FALSE, all sizes 0 */
    uprv_memset(pBiDi, 0, sizeof(UBiDi));

    /* get BiDi properties */
    pBiDi->bdp=ubidi_getSingleton();

    /* allocate memory for arrays as requested */
    if(maxLength>0) {
        if( !getInitialDirPropsMemory(pBiDi, maxLength) ||
            !getInitialLevelsMemory(pBiDi, maxLength)
        ) {
            *pErrorCode=U_MEMORY_ALLOCATION_ERROR;
        }
    } else {
        pBiDi->mayAllocateText=TRUE;
    }

    if(maxRunCount>0) {
        if(maxRunCount==1) {
            /* use simpleRuns[] */
            pBiDi->runsSize=sizeof(Run);
        } else if(!getInitialRunsMemory(pBiDi, maxRunCount)) {
            *pErrorCode=U_MEMORY_ALLOCATION_ERROR;
        }
    } else {
        pBiDi->mayAllocateRuns=TRUE;
    }

    if(U_SUCCESS(*pErrorCode)) {
        return pBiDi;
    } else {
        ubidi_close(pBiDi);
        return NULL;
    }
}

/*
 * We are allowed to allocate memory if memory==NULL or
 * mayAllocate==TRUE for each array that we need.
 * We also try to grow memory as needed if we
 * allocate it.
 *
 * Assume sizeNeeded>0.
 * If *pMemory!=NULL, then assume *pSize>0.
 *
 * ### this realloc() may unnecessarily copy the old data,
 * which we know we don't need any more;
 * is this the best way to do this??
 */
U_CFUNC UBool
ubidi_getMemory(BidiMemoryForAllocation *bidiMem, int32_t *pSize, UBool mayAllocate, int32_t sizeNeeded) {
    void **pMemory = (void **)bidiMem;
    /* check for existing memory */
    if(*pMemory==NULL) {
        /* we need to allocate memory */
        if(mayAllocate && (*pMemory=uprv_malloc(sizeNeeded))!=NULL) {
            *pSize=sizeNeeded;
            return TRUE;
        } else {
            return FALSE;
        }
    } else {
        if(sizeNeeded<=*pSize) {
            /* there is already enough memory */
            return TRUE;
        }
        else if(!mayAllocate) {
            /* not enough memory, and we must not allocate */
            return FALSE;
        } else {
            /* we try to grow */
            void *memory;
            /* in most cases, we do not need the copy-old-data part of
             * realloc, but it is needed when adding runs using getRunsMemory()
             * in setParaRunsOnly()
             */
            if((memory=uprv_realloc(*pMemory, sizeNeeded))!=NULL) {
                *pMemory=memory;
                *pSize=sizeNeeded;
                return TRUE;
            } else {
                /* we failed to grow */
                return FALSE;
            }
        }
    }
}

U_CAPI void U_EXPORT2
ubidi_close(UBiDi *pBiDi) {
    if(pBiDi!=NULL) {
        pBiDi->pParaBiDi=NULL;          /* in case one tries to reuse this block */
        if(pBiDi->dirPropsMemory!=NULL) {
            uprv_free(pBiDi->dirPropsMemory);
        }
        if(pBiDi->levelsMemory!=NULL) {
            uprv_free(pBiDi->levelsMemory);
        }
        if(pBiDi->runsMemory!=NULL) {
            uprv_free(pBiDi->runsMemory);
        }
        if(pBiDi->parasMemory!=NULL) {
            uprv_free(pBiDi->parasMemory);
        }
        if(pBiDi->insertPoints.points!=NULL) {
            uprv_free(pBiDi->insertPoints.points);
        }

        uprv_free(pBiDi);
    }
}

/* set to approximate "inverse BiDi" ---------------------------------------- */

U_CAPI void U_EXPORT2
ubidi_setInverse(UBiDi *pBiDi, UBool isInverse) {
    if(pBiDi!=NULL) {
        pBiDi->isInverse=isInverse;
        pBiDi->reorderingMode = isInverse ? UBIDI_REORDER_INVERSE_NUMBERS_AS_L
                                          : UBIDI_REORDER_DEFAULT;
    }
}

U_CAPI UBool U_EXPORT2
ubidi_isInverse(UBiDi *pBiDi) {
    if(pBiDi!=NULL) {
        return pBiDi->isInverse;
    } else {
        return FALSE;
    }
}

/* FOOD FOR THOUGHT: currently the reordering modes are a mixture of
 * algorithm for direct BiDi, algorithm for inverse BiDi and the bizarre
 * concept of RUNS_ONLY which is a double operation.
 * It could be advantageous to divide this into 3 concepts:
 * a) Operation: direct / inverse / RUNS_ONLY
 * b) Direct algorithm: default / NUMBERS_SPECIAL / GROUP_NUMBERS_WITH_R
 * c) Inverse algorithm: default / INVERSE_LIKE_DIRECT / NUMBERS_SPECIAL
 * This would allow combinations not possible today like RUNS_ONLY with
 * NUMBERS_SPECIAL.
 * Also allow to set INSERT_MARKS for the direct step of RUNS_ONLY and
 * REMOVE_CONTROLS for the inverse step.
 * Not all combinations would be supported, and probably not all do make sense.
 * This would need to document which ones are supported and what are the
 * fallbacks for unsupported combinations.
 */
U_CAPI void U_EXPORT2
ubidi_setReorderingMode(UBiDi *pBiDi, UBiDiReorderingMode reorderingMode) {
    if ((pBiDi!=NULL) && (reorderingMode >= UBIDI_REORDER_DEFAULT)
                        && (reorderingMode < UBIDI_REORDER_COUNT)) {
        pBiDi->reorderingMode = reorderingMode;
        pBiDi->isInverse = (UBool)(reorderingMode == UBIDI_REORDER_INVERSE_NUMBERS_AS_L);
    }
}

U_CAPI UBiDiReorderingMode U_EXPORT2
ubidi_getReorderingMode(UBiDi *pBiDi) {
    if (pBiDi!=NULL) {
        return pBiDi->reorderingMode;
    } else {
        return UBIDI_REORDER_DEFAULT;
    }
}

U_CAPI void U_EXPORT2
ubidi_setReorderingOptions(UBiDi *pBiDi, uint32_t reorderingOptions) {
    if (reorderingOptions & UBIDI_OPTION_REMOVE_CONTROLS) {
        reorderingOptions&=~UBIDI_OPTION_INSERT_MARKS;
    }
    if (pBiDi!=NULL) {
        pBiDi->reorderingOptions=reorderingOptions;
    }
}

U_CAPI uint32_t U_EXPORT2
ubidi_getReorderingOptions(UBiDi *pBiDi) {
    if (pBiDi!=NULL) {
        return pBiDi->reorderingOptions;
    } else {
        return 0;
    }
}

U_CAPI UBiDiDirection U_EXPORT2
ubidi_getBaseDirection(const UChar *text,
int32_t length){

    int32_t i;
    UChar32 uchar;
    UCharDirection dir;
    
    if( text==NULL || length<-1 ){
        return UBIDI_NEUTRAL;
    }

    if(length==-1) {
        length=u_strlen(text);
    }

    for( i = 0 ; i < length; ) {
        /* i is incremented by U16_NEXT */
        U16_NEXT(text, i, length, uchar);
        dir = u_charDirection(uchar);
        if( dir == U_LEFT_TO_RIGHT )
                return UBIDI_LTR;
        if( dir == U_RIGHT_TO_LEFT || dir ==U_RIGHT_TO_LEFT_ARABIC )
                return UBIDI_RTL;
    }
    return UBIDI_NEUTRAL;
}

/* perform (P2)..(P3) ------------------------------------------------------- */

/*
 * Get the directional properties for the text,
 * calculate the flags bit-set, and
 * determine the paragraph level if necessary.
 */
static void
getDirProps(UBiDi *pBiDi) {
    const UChar *text=pBiDi->text;
    DirProp *dirProps=pBiDi->dirPropsMemory;    /* pBiDi->dirProps is const */

    int32_t i=0, i1, length=pBiDi->originalLength;
    Flags flags=0;      /* collect all directionalities in the text */
    UChar32 uchar;
    DirProp dirProp=0, paraDirDefault=0;/* initialize to avoid compiler warnings */
    UBool isDefaultLevel=IS_DEFAULT_LEVEL(pBiDi->paraLevel);
    /* for inverse BiDi, the default para level is set to RTL if there is a
       strong R or AL character at either end of the text                           */
    UBool isDefaultLevelInverse=isDefaultLevel && (UBool)
            (pBiDi->reorderingMode==UBIDI_REORDER_INVERSE_LIKE_DIRECT ||
             pBiDi->reorderingMode==UBIDI_REORDER_INVERSE_FOR_NUMBERS_SPECIAL);
    int32_t lastArabicPos=-1;
    int32_t controlCount=0;
    UBool removeBiDiControls = (UBool)(pBiDi->reorderingOptions &
                                       UBIDI_OPTION_REMOVE_CONTROLS);

    typedef enum {
         NOT_CONTEXTUAL,                /* 0: not contextual paraLevel */
         LOOKING_FOR_STRONG,            /* 1: looking for first strong char */
         FOUND_STRONG_CHAR              /* 2: found first strong char       */
    } State;
    State state;
    int32_t paraStart=0;                /* index of first char in paragraph */
    DirProp paraDir;                    /* == CONTEXT_RTL within paragraphs
                                           starting with strong R char      */
    DirProp lastStrongDir=0;            /* for default level & inverse BiDi */
    int32_t lastStrongLTR=0;            /* for STREAMING option             */

    if(pBiDi->reorderingOptions & UBIDI_OPTION_STREAMING) {
        pBiDi->length=0;
        lastStrongLTR=0;
    }
    if(isDefaultLevel) {
        paraDirDefault=pBiDi->paraLevel&1 ? CONTEXT_RTL : 0;
        paraDir=paraDirDefault;
        lastStrongDir=paraDirDefault;
        state=LOOKING_FOR_STRONG;
    } else {
        state=NOT_CONTEXTUAL;
        paraDir=0;
    }
    /* count paragraphs and determine the paragraph level (P2..P3) */
    /*
     * see comment in ubidi.h:
     * the DEFAULT_XXX values are designed so that
     * their bit 0 alone yields the intended default
     */
    for( /* i=0 above */ ; i<length; ) {
        /* i is incremented by U16_NEXT */
        U16_NEXT(text, i, length, uchar);
        flags|=DIRPROP_FLAG(dirProp=(DirProp)ubidi_getCustomizedClass(pBiDi, uchar));
        dirProps[i-1]=dirProp|paraDir;
        if(uchar>0xffff) {  /* set the lead surrogate's property to BN */
            flags|=DIRPROP_FLAG(BN);
            dirProps[i-2]=(DirProp)(BN|paraDir);
        }
        if(state==LOOKING_FOR_STRONG) {
            if(dirProp==L) {
                state=FOUND_STRONG_CHAR;
                if(paraDir) {
                    paraDir=0;
                    for(i1=paraStart; i1<i; i1++) {
                        dirProps[i1]&=~CONTEXT_RTL;
                    }
                }
                continue;
            }
            if(dirProp==R || dirProp==AL) {
                state=FOUND_STRONG_CHAR;
                if(paraDir==0) {
                    paraDir=CONTEXT_RTL;
                    for(i1=paraStart; i1<i; i1++) {
                        dirProps[i1]|=CONTEXT_RTL;
                    }
                }
                continue;
            }
        }
        if(dirProp==L) {
            lastStrongDir=0;
            lastStrongLTR=i;            /* i is index to next character */
        }
        else if(dirProp==R) {
            lastStrongDir=CONTEXT_RTL;
        }
        else if(dirProp==AL) {
            lastStrongDir=CONTEXT_RTL;
            lastArabicPos=i-1;
        }
        else if(dirProp==B) {
            if(pBiDi->reorderingOptions & UBIDI_OPTION_STREAMING) {
                pBiDi->length=i;        /* i is index to next character */
            }
            if(isDefaultLevelInverse && (lastStrongDir==CONTEXT_RTL) &&(paraDir!=lastStrongDir)) {
                for( ; paraStart<i; paraStart++) {
                    dirProps[paraStart]|=CONTEXT_RTL;
                }
            }
            if(i<length) {              /* B not last char in text */
                if(!((uchar==CR) && (text[i]==LF))) {
                    pBiDi->paraCount++;
                }
                if(isDefaultLevel) {
                    state=LOOKING_FOR_STRONG;
                    paraStart=i;        /* i is index to next character */
                    paraDir=paraDirDefault;
                    lastStrongDir=paraDirDefault;
                }
            }
        }
        if(removeBiDiControls && IS_BIDI_CONTROL_CHAR(uchar)) {
            controlCount++;
        }
    }
    if(isDefaultLevelInverse && (lastStrongDir==CONTEXT_RTL) &&(paraDir!=lastStrongDir)) {
        for(i1=paraStart; i1<length; i1++) {
            dirProps[i1]|=CONTEXT_RTL;
        }
    }
    if(isDefaultLevel) {
        pBiDi->paraLevel=GET_PARALEVEL(pBiDi, 0);
    }
    if(pBiDi->reorderingOptions & UBIDI_OPTION_STREAMING) {
        if((lastStrongLTR>pBiDi->length) &&
           (GET_PARALEVEL(pBiDi, lastStrongLTR)==0)) {
            pBiDi->length = lastStrongLTR;
        }
        if(pBiDi->length<pBiDi->originalLength) {
            pBiDi->paraCount--;
        }
    }
    /* The following line does nothing new for contextual paraLevel, but is
       needed for absolute paraLevel.                               */
    flags|=DIRPROP_FLAG_LR(pBiDi->paraLevel);

    if(pBiDi->orderParagraphsLTR && (flags&DIRPROP_FLAG(B))) {
        flags|=DIRPROP_FLAG(L);
    }

    pBiDi->controlCount = controlCount;
    pBiDi->flags=flags;
    pBiDi->lastArabicPos=lastArabicPos;
}

/* perform (X1)..(X9) ------------------------------------------------------- */

/* determine if the text is mixed-directional or single-directional */
static UBiDiDirection
directionFromFlags(UBiDi *pBiDi) {
    Flags flags=pBiDi->flags;
    /* if the text contains AN and neutrals, then some neutrals may become RTL */
    if(!(flags&MASK_RTL || ((flags&DIRPROP_FLAG(AN)) && (flags&MASK_POSSIBLE_N)))) {
        return UBIDI_LTR;
    } else if(!(flags&MASK_LTR)) {
        return UBIDI_RTL;
    } else {
        return UBIDI_MIXED;
    }
}

/*
 * Resolve the explicit levels as specified by explicit embedding codes.
 * Recalculate the flags to have them reflect the real properties
 * after taking the explicit embeddings into account.
 *
 * The BiDi algorithm is designed to result in the same behavior whether embedding
 * levels are externally specified (from "styled text", supposedly the preferred
 * method) or set by explicit embedding codes (LRx, RLx, PDF) in the plain text.
 * That is why (X9) instructs to remove all explicit codes (and BN).
 * However, in a real implementation, this removal of these codes and their index
 * positions in the plain text is undesirable since it would result in
 * reallocated, reindexed text.
 * Instead, this implementation leaves the codes in there and just ignores them
 * in the subsequent processing.
 * In order to get the same reordering behavior, positions with a BN or an
 * explicit embedding code just get the same level assigned as the last "real"
 * character.
 *
 * Some implementations, not this one, then overwrite some of these
 * directionality properties at "real" same-level-run boundaries by
 * L or R codes so that the resolution of weak types can be performed on the
 * entire paragraph at once instead of having to parse it once more and
 * perform that resolution on same-level-runs.
 * This limits the scope of the implicit rules in effectively
 * the same way as the run limits.
 *
 * Instead, this implementation does not modify these codes.
 * On one hand, the paragraph has to be scanned for same-level-runs, but
 * on the other hand, this saves another loop to reset these codes,
 * or saves making and modifying a copy of dirProps[].
 *
 *
 * Note that (Pn) and (Xn) changed significantly from version 4 of the BiDi algorithm.
 *
 *
 * Handling the stack of explicit levels (Xn):
 *
 * With the BiDi stack of explicit levels,
 * as pushed with each LRE, RLE, LRO, and RLO and popped with each PDF,
 * the explicit level must never exceed UBIDI_MAX_EXPLICIT_LEVEL==61.
 *
 * In order to have a correct push-pop semantics even in the case of overflows,
 * there are two overflow counters:
 * - countOver60 is incremented with each LRx at level 60
 * - from level 60, one RLx increases the level to 61
 * - countOver61 is incremented with each LRx and RLx at level 61
 *
 * Popping levels with PDF must work in the opposite order so that level 61
 * is correct at the correct point. Underflows (too many PDFs) must be checked.
 *
 * This implementation assumes that UBIDI_MAX_EXPLICIT_LEVEL is odd.
 */
static UBiDiDirection
resolveExplicitLevels(UBiDi *pBiDi) {
    const DirProp *dirProps=pBiDi->dirProps;
    UBiDiLevel *levels=pBiDi->levels;
    const UChar *text=pBiDi->text;

    int32_t i=0, length=pBiDi->length;
    Flags flags=pBiDi->flags;       /* collect all directionalities in the text */
    DirProp dirProp;
    UBiDiLevel level=GET_PARALEVEL(pBiDi, 0);

    UBiDiDirection direction;
    int32_t paraIndex=0;

    /* determine if the text is mixed-directional or single-directional */
    direction=directionFromFlags(pBiDi);

    /* we may not need to resolve any explicit levels, but for multiple
       paragraphs we want to loop on all chars to set the para boundaries */
    if((direction!=UBIDI_MIXED) && (pBiDi->paraCount==1)) {
        /* not mixed directionality: levels don't matter - trailingWSStart will be 0 */
    } else if((pBiDi->paraCount==1) &&
              (!(flags&MASK_EXPLICIT) ||
               (pBiDi->reorderingMode > UBIDI_REORDER_LAST_LOGICAL_TO_VISUAL))) {
        /* mixed, but all characters are at the same embedding level */
        /* or we are in "inverse BiDi" */
        /* and we don't have contextual multiple paragraphs with some B char */
        /* set all levels to the paragraph level */
        for(i=0; i<length; ++i) {
            levels[i]=level;
        }
    } else {
        /* continue to perform (Xn) */

        /* (X1) level is set for all codes, embeddingLevel keeps track of the push/pop operations */
        /* both variables may carry the UBIDI_LEVEL_OVERRIDE flag to indicate the override status */
        UBiDiLevel embeddingLevel=level, newLevel, stackTop=0;

        UBiDiLevel stack[UBIDI_MAX_EXPLICIT_LEVEL];        /* we never push anything >=UBIDI_MAX_EXPLICIT_LEVEL */
        uint32_t countOver60=0, countOver61=0;  /* count overflows of explicit levels */

        /* recalculate the flags */
        flags=0;

        for(i=0; i<length; ++i) {
            dirProp=NO_CONTEXT_RTL(dirProps[i]);
            switch(dirProp) {
            case LRE:
            case LRO:
                /* (X3, X5) */
                newLevel=(UBiDiLevel)((embeddingLevel+2)&~(UBIDI_LEVEL_OVERRIDE|1)); /* least greater even level */
                if(newLevel<=UBIDI_MAX_EXPLICIT_LEVEL) {
                    stack[stackTop]=embeddingLevel;
                    ++stackTop;
                    embeddingLevel=newLevel;
                    if(dirProp==LRO) {
                        embeddingLevel|=UBIDI_LEVEL_OVERRIDE;
                    }
                    /* we don't need to set UBIDI_LEVEL_OVERRIDE off for LRE
                       since this has already been done for newLevel which is
                       the source for embeddingLevel.
                     */
                } else if((embeddingLevel&~UBIDI_LEVEL_OVERRIDE)==UBIDI_MAX_EXPLICIT_LEVEL) {
                    ++countOver61;
                } else /* (embeddingLevel&~UBIDI_LEVEL_OVERRIDE)==UBIDI_MAX_EXPLICIT_LEVEL-1 */ {
                    ++countOver60;
                }
                flags|=DIRPROP_FLAG(BN);
                break;
            case RLE:
            case RLO:
                /* (X2, X4) */
                newLevel=(UBiDiLevel)(((embeddingLevel&~UBIDI_LEVEL_OVERRIDE)+1)|1); /* least greater odd level */
                if(newLevel<=UBIDI_MAX_EXPLICIT_LEVEL) {
                    stack[stackTop]=embeddingLevel;
                    ++stackTop;
                    embeddingLevel=newLevel;
                    if(dirProp==RLO) {
                        embeddingLevel|=UBIDI_LEVEL_OVERRIDE;
                    }
                    /* we don't need to set UBIDI_LEVEL_OVERRIDE off for RLE
                       since this has already been done for newLevel which is
                       the source for embeddingLevel.
                     */
                } else {
                    ++countOver61;
                }
                flags|=DIRPROP_FLAG(BN);
                break;
            case PDF:
                /* (X7) */
                /* handle all the overflow cases first */
                if(countOver61>0) {
                    --countOver61;
                } else if(countOver60>0 && (embeddingLevel&~UBIDI_LEVEL_OVERRIDE)!=UBIDI_MAX_EXPLICIT_LEVEL) {
                    /* handle LRx overflows from level 60 */
                    --countOver60;
                } else if(stackTop>0) {
                    /* this is the pop operation; it also pops level 61 while countOver60>0 */
                    --stackTop;
                    embeddingLevel=stack[stackTop];
                /* } else { (underflow) */
                }
                flags|=DIRPROP_FLAG(BN);
                break;
            case B:
                stackTop=0;
                countOver60=countOver61=0;
                level=GET_PARALEVEL(pBiDi, i);
                if((i+1)<length) {
                    embeddingLevel=GET_PARALEVEL(pBiDi, i+1);
                    if(!((text[i]==CR) && (text[i+1]==LF))) {
                        pBiDi->paras[paraIndex++]=i+1;
                    }
                }
                flags|=DIRPROP_FLAG(B);
                break;
            case BN:
                /* BN, LRE, RLE, and PDF are supposed to be removed (X9) */
                /* they will get their levels set correctly in adjustWSLevels() */
                flags|=DIRPROP_FLAG(BN);
                break;
            default:
                /* all other types get the "real" level */
                if(level!=embeddingLevel) {
                    level=embeddingLevel;
                    if(level&UBIDI_LEVEL_OVERRIDE) {
                        flags|=DIRPROP_FLAG_O(level)|DIRPROP_FLAG_MULTI_RUNS;
                    } else {
                        flags|=DIRPROP_FLAG_E(level)|DIRPROP_FLAG_MULTI_RUNS;
                    }
                }
                if(!(level&UBIDI_LEVEL_OVERRIDE)) {
                    flags|=DIRPROP_FLAG(dirProp);
                }
                break;
            }

            /*
             * We need to set reasonable levels even on BN codes and
             * explicit codes because we will later look at same-level runs (X10).
             */
            levels[i]=level;
        }
        if(flags&MASK_EMBEDDING) {
            flags|=DIRPROP_FLAG_LR(pBiDi->paraLevel);
        }
        if(pBiDi->orderParagraphsLTR && (flags&DIRPROP_FLAG(B))) {
            flags|=DIRPROP_FLAG(L);
        }

        /* subsequently, ignore the explicit codes and BN (X9) */

        /* again, determine if the text is mixed-directional or single-directional */
        pBiDi->flags=flags;
        direction=directionFromFlags(pBiDi);
    }

    return direction;
}

/*
 * Use a pre-specified embedding levels array:
 *
 * Adjust the directional properties for overrides (->LEVEL_OVERRIDE),
 * ignore all explicit codes (X9),
 * and check all the preset levels.
 *
 * Recalculate the flags to have them reflect the real properties
 * after taking the explicit embeddings into account.
 */
static UBiDiDirection
checkExplicitLevels(UBiDi *pBiDi, UErrorCode *pErrorCode) {
    const DirProp *dirProps=pBiDi->dirProps;
    DirProp dirProp;
    UBiDiLevel *levels=pBiDi->levels;
    const UChar *text=pBiDi->text;

    int32_t i, length=pBiDi->length;
    Flags flags=0;  /* collect all directionalities in the text */
    UBiDiLevel level;
    uint32_t paraIndex=0;

    for(i=0; i<length; ++i) {
        level=levels[i];
        dirProp=NO_CONTEXT_RTL(dirProps[i]);
        if(level&UBIDI_LEVEL_OVERRIDE) {
            /* keep the override flag in levels[i] but adjust the flags */
            level&=~UBIDI_LEVEL_OVERRIDE;     /* make the range check below simpler */
            flags|=DIRPROP_FLAG_O(level);
        } else {
            /* set the flags */
            flags|=DIRPROP_FLAG_E(level)|DIRPROP_FLAG(dirProp);
        }
        if((level<GET_PARALEVEL(pBiDi, i) &&
            !((0==level)&&(dirProp==B))) ||
           (UBIDI_MAX_EXPLICIT_LEVEL<level)) {
            /* level out of bounds */
            *pErrorCode=U_ILLEGAL_ARGUMENT_ERROR;
            return UBIDI_LTR;
        }
        if((dirProp==B) && ((i+1)<length)) {
            if(!((text[i]==CR) && (text[i+1]==LF))) {
                pBiDi->paras[paraIndex++]=i+1;
            }
        }
    }
    if(flags&MASK_EMBEDDING) {
        flags|=DIRPROP_FLAG_LR(pBiDi->paraLevel);
    }

    /* determine if the text is mixed-directional or single-directional */
    pBiDi->flags=flags;
    return directionFromFlags(pBiDi);
}

/******************************************************************
 The Properties state machine table
*******************************************************************

 All table cells are 8 bits:
      bits 0..4:  next state
      bits 5..7:  action to perform (if > 0)

 Cells may be of format "n" where n represents the next state
 (except for the rightmost column).
 Cells may also be of format "s(x,y)" where x represents an action
 to perform and y represents the next state.

*******************************************************************
 Definitions and type for properties state table
*******************************************************************
*/
#define IMPTABPROPS_COLUMNS 14
#define IMPTABPROPS_RES (IMPTABPROPS_COLUMNS - 1)
#define GET_STATEPROPS(cell) ((cell)&0x1f)
#define GET_ACTIONPROPS(cell) ((cell)>>5)
#define s(action, newState) ((uint8_t)(newState+(action<<5)))

static const uint8_t groupProp[] =          /* dirProp regrouped */
{
/*  L   R   EN  ES  ET  AN  CS  B   S   WS  ON  LRE LRO AL  RLE RLO PDF NSM BN  */
    0,  1,  2,  7,  8,  3,  9,  6,  5,  4,  4,  10, 10, 12, 10, 10, 10, 11, 10
};
enum { DirProp_L=0, DirProp_R=1, DirProp_EN=2, DirProp_AN=3, DirProp_ON=4, DirProp_S=5, DirProp_B=6 }; /* reduced dirProp */

/******************************************************************

      PROPERTIES  STATE  TABLE

 In table impTabProps,
      - the ON column regroups ON and WS
      - the BN column regroups BN, LRE, RLE, LRO, RLO, PDF
      - the Res column is the reduced property assigned to a run

 Action 1: process current run1, init new run1
        2: init new run2
        3: process run1, process run2, init new run1
        4: process run1, set run1=run2, init new run2

 Notes:
  1) This table is used in resolveImplicitLevels().
  2) This table triggers actions when there is a change in the Bidi
     property of incoming characters (action 1).
  3) Most such property sequences are processed immediately (in
     fact, passed to processPropertySeq().
  4) However, numbers are assembled as one sequence. This means
     that undefined situations (like CS following digits, until
     it is known if the next char will be a digit) are held until
     following chars define them.
     Example: digits followed by CS, then comes another CS or ON;
              the digits will be processed, then the CS assigned
              as the start of an ON sequence (action 3).
  5) There are cases where more than one sequence must be
     processed, for instance digits followed by CS followed by L:
     the digits must be processed as one sequence, and the CS
     must be processed as an ON sequence, all this before starting
     assembling chars for the opening L sequence.


*/
static const uint8_t impTabProps[][IMPTABPROPS_COLUMNS] =
{
/*                        L ,     R ,    EN ,    AN ,    ON ,     S ,     B ,    ES ,    ET ,    CS ,    BN ,   NSM ,    AL ,  Res */
/* 0 Init        */ {     1 ,     2 ,     4 ,     5 ,     7 ,    15 ,    17 ,     7 ,     9 ,     7 ,     0 ,     7 ,     3 ,  DirProp_ON },
/* 1 L           */ {     1 , s(1,2), s(1,4), s(1,5), s(1,7),s(1,15),s(1,17), s(1,7), s(1,9), s(1,7),     1 ,     1 , s(1,3),   DirProp_L },
/* 2 R           */ { s(1,1),     2 , s(1,4), s(1,5), s(1,7),s(1,15),s(1,17), s(1,7), s(1,9), s(1,7),     2 ,     2 , s(1,3),   DirProp_R },
/* 3 AL          */ { s(1,1), s(1,2), s(1,6), s(1,6), s(1,8),s(1,16),s(1,17), s(1,8), s(1,8), s(1,8),     3 ,     3 ,     3 ,   DirProp_R },
/* 4 EN          */ { s(1,1), s(1,2),     4 , s(1,5), s(1,7),s(1,15),s(1,17),s(2,10),    11 ,s(2,10),     4 ,     4 , s(1,3),  DirProp_EN },
/* 5 AN          */ { s(1,1), s(1,2), s(1,4),     5 , s(1,7),s(1,15),s(1,17), s(1,7), s(1,9),s(2,12),     5 ,     5 , s(1,3),  DirProp_AN },
/* 6 AL:EN/AN    */ { s(1,1), s(1,2),     6 ,     6 , s(1,8),s(1,16),s(1,17), s(1,8), s(1,8),s(2,13),     6 ,     6 , s(1,3),  DirProp_AN },
/* 7 ON          */ { s(1,1), s(1,2), s(1,4), s(1,5),     7 ,s(1,15),s(1,17),     7 ,s(2,14),     7 ,     7 ,     7 , s(1,3),  DirProp_ON },
/* 8 AL:ON       */ { s(1,1), s(1,2), s(1,6), s(1,6),     8 ,s(1,16),s(1,17),     8 ,     8 ,     8 ,     8 ,     8 , s(1,3),  DirProp_ON },
/* 9 ET          */ { s(1,1), s(1,2),     4 , s(1,5),     7 ,s(1,15),s(1,17),     7 ,     9 ,     7 ,     9 ,     9 , s(1,3),  DirProp_ON },
/*10 EN+ES/CS    */ { s(3,1), s(3,2),     4 , s(3,5), s(4,7),s(3,15),s(3,17), s(4,7),s(4,14), s(4,7),    10 , s(4,7), s(3,3),  DirProp_EN },
/*11 EN+ET       */ { s(1,1), s(1,2),     4 , s(1,5), s(1,7),s(1,15),s(1,17), s(1,7),    11 , s(1,7),    11 ,    11 , s(1,3),  DirProp_EN },
/*12 AN+CS       */ { s(3,1), s(3,2), s(3,4),     5 , s(4,7),s(3,15),s(3,17), s(4,7),s(4,14), s(4,7),    12 , s(4,7), s(3,3),  DirProp_AN },
/*13 AL:EN/AN+CS */ { s(3,1), s(3,2),     6 ,     6 , s(4,8),s(3,16),s(3,17), s(4,8), s(4,8), s(4,8),    13 , s(4,8), s(3,3),  DirProp_AN },
/*14 ON+ET       */ { s(1,1), s(1,2), s(4,4), s(1,5),     7 ,s(1,15),s(1,17),     7 ,    14 ,     7 ,    14 ,    14 , s(1,3),  DirProp_ON },
/*15 S           */ { s(1,1), s(1,2), s(1,4), s(1,5), s(1,7),    15 ,s(1,17), s(1,7), s(1,9), s(1,7),    15 , s(1,7), s(1,3),   DirProp_S },
/*16 AL:S        */ { s(1,1), s(1,2), s(1,6), s(1,6), s(1,8),    16 ,s(1,17), s(1,8), s(1,8), s(1,8),    16 , s(1,8), s(1,3),   DirProp_S },
/*17 B           */ { s(1,1), s(1,2), s(1,4), s(1,5), s(1,7),s(1,15),    17 , s(1,7), s(1,9), s(1,7),    17 , s(1,7), s(1,3),   DirProp_B }
};

/*  we must undef macro s because the levels table have a different
 *  structure (4 bits for action and 4 bits for next state.
 */
#undef s

/******************************************************************
 The levels state machine tables
*******************************************************************

 All table cells are 8 bits:
      bits 0..3:  next state
      bits 4..7:  action to perform (if > 0)

 Cells may be of format "n" where n represents the next state
 (except for the rightmost column).
 Cells may also be of format "s(x,y)" where x represents an action
 to perform and y represents the next state.

 This format limits each table to 16 states each and to 15 actions.

*******************************************************************
 Definitions and type for levels state tables
*******************************************************************
*/
#define IMPTABLEVELS_COLUMNS (DirProp_B + 2)
#define IMPTABLEVELS_RES (IMPTABLEVELS_COLUMNS - 1)
#define GET_STATE(cell) ((cell)&0x0f)
#define GET_ACTION(cell) ((cell)>>4)
#define s(action, newState) ((uint8_t)(newState+(action<<4)))

typedef uint8_t ImpTab[][IMPTABLEVELS_COLUMNS];
typedef uint8_t ImpAct[];

/* FOOD FOR THOUGHT: each ImpTab should have its associated ImpAct,
 * instead of having a pair of ImpTab and a pair of ImpAct.
 */
typedef struct ImpTabPair {
    const void * pImpTab[2];
    const void * pImpAct[2];
} ImpTabPair;

/******************************************************************

      LEVELS  STATE  TABLES

 In all levels state tables,
      - state 0 is the initial state
      - the Res column is the increment to add to the text level
        for this property sequence.

 The impAct arrays for each table of a pair map the local action
 numbers of the table to the total list of actions. For instance,
 action 2 in a given table corresponds to the action number which
 appears in entry [2] of the impAct array for that table.
 The first entry of all impAct arrays must be 0.

 Action 1: init conditional sequence
        2: prepend conditional sequence to current sequence
        3: set ON sequence to new level - 1
        4: init EN/AN/ON sequence
        5: fix EN/AN/ON sequence followed by R
        6: set previous level sequence to level 2

 Notes:
  1) These tables are used in processPropertySeq(). The input
     is property sequences as determined by resolveImplicitLevels.
  2) Most such property sequences are processed immediately
     (levels are assigned).
  3) However, some sequences cannot be assigned a final level till
     one or more following sequences are received. For instance,
     ON following an R sequence within an even-level paragraph.
     If the following sequence is R, the ON sequence will be
     assigned basic run level+1, and so will the R sequence.
  4) S is generally handled like ON, since its level will be fixed
     to paragraph level in adjustWSLevels().

*/

static const ImpTab impTabL_DEFAULT =   /* Even paragraph level */
/*  In this table, conditional sequences receive the higher possible level
    until proven otherwise.
*/
{
/*                         L ,     R ,    EN ,    AN ,    ON ,     S ,     B , Res */
/* 0 : init       */ {     0 ,     1 ,     0 ,     2 ,     0 ,     0 ,     0 ,  0 },
/* 1 : R          */ {     0 ,     1 ,     3 ,     3 , s(1,4), s(1,4),     0 ,  1 },
/* 2 : AN         */ {     0 ,     1 ,     0 ,     2 , s(1,5), s(1,5),     0 ,  2 },
/* 3 : R+EN/AN    */ {     0 ,     1 ,     3 ,     3 , s(1,4), s(1,4),     0 ,  2 },
/* 4 : R+ON       */ { s(2,0),     1 ,     3 ,     3 ,     4 ,     4 , s(2,0),  1 },
/* 5 : AN+ON      */ { s(2,0),     1 , s(2,0),     2 ,     5 ,     5 , s(2,0),  1 }
};
static const ImpTab impTabR_DEFAULT =   /* Odd  paragraph level */
/*  In this table, conditional sequences receive the lower possible level
    until proven otherwise.
*/
{
/*                         L ,     R ,    EN ,    AN ,    ON ,     S ,     B , Res */
/* 0 : init       */ {     1 ,     0 ,     2 ,     2 ,     0 ,     0 ,     0 ,  0 },
/* 1 : L          */ {     1 ,     0 ,     1 ,     3 , s(1,4), s(1,4),     0 ,  1 },
/* 2 : EN/AN      */ {     1 ,     0 ,     2 ,     2 ,     0 ,     0 ,     0 ,  1 },
/* 3 : L+AN       */ {     1 ,     0 ,     1 ,     3 ,     5 ,     5 ,     0 ,  1 },
/* 4 : L+ON       */ { s(2,1),     0 , s(2,1),     3 ,     4 ,     4 ,     0 ,  0 },
/* 5 : L+AN+ON    */ {     1 ,     0 ,     1 ,     3 ,     5 ,     5 ,     0 ,  0 }
};
static const ImpAct impAct0 = {0,1,2,3,4,5,6};
static const ImpTabPair impTab_DEFAULT = {{&impTabL_DEFAULT,
                                           &impTabR_DEFAULT},
                                          {&impAct0, &impAct0}};

static const ImpTab impTabL_NUMBERS_SPECIAL =   /* Even paragraph level */
/*  In this table, conditional sequences receive the higher possible level
    until proven otherwise.
*/
{
/*                         L ,     R ,    EN ,    AN ,    ON ,     S ,     B , Res */
/* 0 : init       */ {     0 ,     2 ,    1 ,      1 ,     0 ,     0 ,     0 ,  0 },
/* 1 : L+EN/AN    */ {     0 ,     2 ,    1 ,      1 ,     0 ,     0 ,     0 ,  2 },
/* 2 : R          */ {     0 ,     2 ,    4 ,      4 , s(1,3),     0 ,     0 ,  1 },
/* 3 : R+ON       */ { s(2,0),     2 ,    4 ,      4 ,     3 ,     3 , s(2,0),  1 },
/* 4 : R+EN/AN    */ {     0 ,     2 ,    4 ,      4 , s(1,3), s(1,3),     0 ,  2 }
  };
static const ImpTabPair impTab_NUMBERS_SPECIAL = {{&impTabL_NUMBERS_SPECIAL,
                                                   &impTabR_DEFAULT},
                                                  {&impAct0, &impAct0}};

static const ImpTab impTabL_GROUP_NUMBERS_WITH_R =
/*  In this table, EN/AN+ON sequences receive levels as if associated with R
    until proven that there is L or sor/eor on both sides. AN is handled like EN.
*/
{
/*                         L ,     R ,    EN ,    AN ,    ON ,     S ,     B , Res */
/* 0 init         */ {     0 ,     3 , s(1,1), s(1,1),     0 ,     0 ,     0 ,  0 },
/* 1 EN/AN        */ { s(2,0),     3 ,     1 ,     1 ,     2 , s(2,0), s(2,0),  2 },
/* 2 EN/AN+ON     */ { s(2,0),     3 ,     1 ,     1 ,     2 , s(2,0), s(2,0),  1 },
/* 3 R            */ {     0 ,     3 ,     5 ,     5 , s(1,4),     0 ,     0 ,  1 },
/* 4 R+ON         */ { s(2,0),     3 ,     5 ,     5 ,     4 , s(2,0), s(2,0),  1 },
/* 5 R+EN/AN      */ {     0 ,     3 ,     5 ,     5 , s(1,4),     0 ,     0 ,  2 }
};
static const ImpTab impTabR_GROUP_NUMBERS_WITH_R =
/*  In this table, EN/AN+ON sequences receive levels as if associated with R
    until proven that there is L on both sides. AN is handled like EN.
*/
{
/*                         L ,     R ,    EN ,    AN ,    ON ,     S ,     B , Res */
/* 0 init         */ {     2 ,     0 ,     1 ,     1 ,     0 ,     0 ,     0 ,  0 },
/* 1 EN/AN        */ {     2 ,     0 ,     1 ,     1 ,     0 ,     0 ,     0 ,  1 },
/* 2 L            */ {     2 ,     0 , s(1,4), s(1,4), s(1,3),     0 ,     0 ,  1 },
/* 3 L+ON         */ { s(2,2),     0 ,     4 ,     4 ,     3 ,     0 ,     0 ,  0 },
/* 4 L+EN/AN      */ { s(2,2),     0 ,     4 ,     4 ,     3 ,     0 ,     0 ,  1 }
};
static const ImpTabPair impTab_GROUP_NUMBERS_WITH_R = {
                        {&impTabL_GROUP_NUMBERS_WITH_R,
                         &impTabR_GROUP_NUMBERS_WITH_R},
                        {&impAct0, &impAct0}};


static const ImpTab impTabL_INVERSE_NUMBERS_AS_L =
/*  This table is identical to the Default LTR table except that EN and AN are
    handled like L.
*/
{
/*                         L ,     R ,    EN ,    AN ,    ON ,     S ,     B , Res */
/* 0 : init       */ {     0 ,     1 ,     0 ,     0 ,     0 ,     0 ,     0 ,  0 },
/* 1 : R          */ {     0 ,     1 ,     0 ,     0 , s(1,4), s(1,4),     0 ,  1 },
/* 2 : AN         */ {     0 ,     1 ,     0 ,     0 , s(1,5), s(1,5),     0 ,  2 },
/* 3 : R+EN/AN    */ {     0 ,     1 ,     0 ,     0 , s(1,4), s(1,4),     0 ,  2 },
/* 4 : R+ON       */ { s(2,0),     1 , s(2,0), s(2,0),     4 ,     4 , s(2,0),  1 },
/* 5 : AN+ON      */ { s(2,0),     1 , s(2,0), s(2,0),     5 ,     5 , s(2,0),  1 }
};
static const ImpTab impTabR_INVERSE_NUMBERS_AS_L =
/*  This table is identical to the Default RTL table except that EN and AN are
    handled like L.
*/
{
/*                         L ,     R ,    EN ,    AN ,    ON ,     S ,     B , Res */
/* 0 : init       */ {     1 ,     0 ,     1 ,     1 ,     0 ,     0 ,     0 ,  0 },
/* 1 : L          */ {     1 ,     0 ,     1 ,     1 , s(1,4), s(1,4),     0 ,  1 },
/* 2 : EN/AN      */ {     1 ,     0 ,     1 ,     1 ,     0 ,     0 ,     0 ,  1 },
/* 3 : L+AN       */ {     1 ,     0 ,     1 ,     1 ,     5 ,     5 ,     0 ,  1 },
/* 4 : L+ON       */ { s(2,1),     0 , s(2,1), s(2,1),     4 ,     4 ,     0 ,  0 },
/* 5 : L+AN+ON    */ {     1 ,     0 ,     1 ,     1 ,     5 ,     5 ,     0 ,  0 }
};
static const ImpTabPair impTab_INVERSE_NUMBERS_AS_L = {
                        {&impTabL_INVERSE_NUMBERS_AS_L,
                         &impTabR_INVERSE_NUMBERS_AS_L},
                        {&impAct0, &impAct0}};

static const ImpTab impTabR_INVERSE_LIKE_DIRECT =   /* Odd  paragraph level */
/*  In this table, conditional sequences receive the lower possible level
    until proven otherwise.
*/
{
/*                         L ,     R ,    EN ,    AN ,    ON ,     S ,     B , Res */
/* 0 : init       */ {     1 ,     0 ,     2 ,     2 ,     0 ,     0 ,     0 ,  0 },
/* 1 : L          */ {     1 ,     0 ,     1 ,     2 , s(1,3), s(1,3),     0 ,  1 },
/* 2 : EN/AN      */ {     1 ,     0 ,     2 ,     2 ,     0 ,     0 ,     0 ,  1 },
/* 3 : L+ON       */ { s(2,1), s(3,0),     6 ,     4 ,     3 ,     3 , s(3,0),  0 },
/* 4 : L+ON+AN    */ { s(2,1), s(3,0),     6 ,     4 ,     5 ,     5 , s(3,0),  3 },
/* 5 : L+AN+ON    */ { s(2,1), s(3,0),     6 ,     4 ,     5 ,     5 , s(3,0),  2 },
/* 6 : L+ON+EN    */ { s(2,1), s(3,0),     6 ,     4 ,     3 ,     3 , s(3,0),  1 }
};
static const ImpAct impAct1 = {0,1,11,12};
/* FOOD FOR THOUGHT: in LTR table below, check case "JKL 123abc"
 */
static const ImpTabPair impTab_INVERSE_LIKE_DIRECT = {
                        {&impTabL_DEFAULT,
                         &impTabR_INVERSE_LIKE_DIRECT},
                        {&impAct0, &impAct1}};

static const ImpTab impTabL_INVERSE_LIKE_DIRECT_WITH_MARKS =
/*  The case handled in this table is (visually):  R EN L
*/
{
/*                         L ,     R ,    EN ,    AN ,    ON ,     S ,     B , Res */
/* 0 : init       */ {     0 , s(6,3),     0 ,     1 ,     0 ,     0 ,     0 ,  0 },
/* 1 : L+AN       */ {     0 , s(6,3),     0 ,     1 , s(1,2), s(3,0),     0 ,  4 },
/* 2 : L+AN+ON    */ { s(2,0), s(6,3), s(2,0),     1 ,     2 , s(3,0), s(2,0),  3 },
/* 3 : R          */ {     0 , s(6,3), s(5,5), s(5,6), s(1,4), s(3,0),     0 ,  3 },
/* 4 : R+ON       */ { s(3,0), s(4,3), s(5,5), s(5,6),     4 , s(3,0), s(3,0),  3 },
/* 5 : R+EN       */ { s(3,0), s(4,3),     5 , s(5,6), s(1,4), s(3,0), s(3,0),  4 },
/* 6 : R+AN       */ { s(3,0), s(4,3), s(5,5),     6 , s(1,4), s(3,0), s(3,0),  4 }
};
static const ImpTab impTabR_INVERSE_LIKE_DIRECT_WITH_MARKS =
/*  The cases handled in this table are (visually):  R EN L
                                                     R L AN L
*/
{
/*                         L ,     R ,    EN ,    AN ,    ON ,     S ,     B , Res */
/* 0 : init       */ { s(1,3),     0 ,     1 ,     1 ,     0 ,     0 ,     0 ,  0 },
/* 1 : R+EN/AN    */ { s(2,3),     0 ,     1 ,     1 ,     2 , s(4,0),     0 ,  1 },
/* 2 : R+EN/AN+ON */ { s(2,3),     0 ,     1 ,     1 ,     2 , s(4,0),     0 ,  0 },
/* 3 : L          */ {     3 ,     0 ,     3 , s(3,6), s(1,4), s(4,0),     0 ,  1 },
/* 4 : L+ON       */ { s(5,3), s(4,0),     5 , s(3,6),     4 , s(4,0), s(4,0),  0 },
/* 5 : L+ON+EN    */ { s(5,3), s(4,0),     5 , s(3,6),     4 , s(4,0), s(4,0),  1 },
/* 6 : L+AN       */ { s(5,3), s(4,0),     6 ,     6 ,     4 , s(4,0), s(4,0),  3 }
};
static const ImpAct impAct2 = {0,1,7,8,9,10};
static const ImpTabPair impTab_INVERSE_LIKE_DIRECT_WITH_MARKS = {
                        {&impTabL_INVERSE_LIKE_DIRECT_WITH_MARKS,
                         &impTabR_INVERSE_LIKE_DIRECT_WITH_MARKS},
                        {&impAct0, &impAct2}};

static const ImpTabPair impTab_INVERSE_FOR_NUMBERS_SPECIAL = {
                        {&impTabL_NUMBERS_SPECIAL,
                         &impTabR_INVERSE_LIKE_DIRECT},
                        {&impAct0, &impAct1}};

static const ImpTab impTabL_INVERSE_FOR_NUMBERS_SPECIAL_WITH_MARKS =
/*  The case handled in this table is (visually):  R EN L
*/
{
/*                         L ,     R ,    EN ,    AN ,    ON ,     S ,     B , Res */
/* 0 : init       */ {     0 , s(6,2),     1 ,     1 ,     0 ,     0 ,     0 ,  0 },
/* 1 : L+EN/AN    */ {     0 , s(6,2),     1 ,     1 ,     0 , s(3,0),     0 ,  4 },
/* 2 : R          */ {     0 , s(6,2), s(5,4), s(5,4), s(1,3), s(3,0),     0 ,  3 },
/* 3 : R+ON       */ { s(3,0), s(4,2), s(5,4), s(5,4),     3 , s(3,0), s(3,0),  3 },
/* 4 : R+EN/AN    */ { s(3,0), s(4,2),     4 ,     4 , s(1,3), s(3,0), s(3,0),  4 }
};
static const ImpTabPair impTab_INVERSE_FOR_NUMBERS_SPECIAL_WITH_MARKS = {
                        {&impTabL_INVERSE_FOR_NUMBERS_SPECIAL_WITH_MARKS,
                         &impTabR_INVERSE_LIKE_DIRECT_WITH_MARKS},
                        {&impAct0, &impAct2}};

#undef s

typedef struct {
    const ImpTab * pImpTab;             /* level table pointer          */
    const ImpAct * pImpAct;             /* action map array             */
    int32_t startON;                    /* start of ON sequence         */
    int32_t startL2EN;                  /* start of level 2 sequence    */
    int32_t lastStrongRTL;              /* index of last found R or AL  */
    int32_t state;                      /* current state                */
    UBiDiLevel runLevel;                /* run level before implicit solving */
} LevState;

/*------------------------------------------------------------------------*/

static void
addPoint(UBiDi *pBiDi, int32_t pos, int32_t flag)
  /* param pos:     position where to insert
     param flag:    one of LRM_BEFORE, LRM_AFTER, RLM_BEFORE, RLM_AFTER
  */
{
#define FIRSTALLOC  10
    Point point;
    InsertPoints * pInsertPoints=&(pBiDi->insertPoints);

    if (pInsertPoints->capacity == 0)
    {
        pInsertPoints->points=uprv_malloc(sizeof(Point)*FIRSTALLOC);
        if (pInsertPoints->points == NULL)
        {
            pInsertPoints->errorCode=U_MEMORY_ALLOCATION_ERROR;
            return;
        }
        pInsertPoints->capacity=FIRSTALLOC;
    }
    if (pInsertPoints->size >= pInsertPoints->capacity) /* no room for new point */
    {
        void * savePoints=pInsertPoints->points;
        pInsertPoints->points=uprv_realloc(pInsertPoints->points,
                                           pInsertPoints->capacity*2*sizeof(Point));
        if (pInsertPoints->points == NULL)
        {
            pInsertPoints->points=savePoints;
            pInsertPoints->errorCode=U_MEMORY_ALLOCATION_ERROR;
            return;
        }
        else  pInsertPoints->capacity*=2;
    }
    point.pos=pos;
    point.flag=flag;
    pInsertPoints->points[pInsertPoints->size]=point;
    pInsertPoints->size++;
#undef FIRSTALLOC
}

/* perform rules (Wn), (Nn), and (In) on a run of the text ------------------ */

/*
 * This implementation of the (Wn) rules applies all rules in one pass.
 * In order to do so, it needs a look-ahead of typically 1 character
 * (except for W5: sequences of ET) and keeps track of changes
 * in a rule Wp that affect a later Wq (p<q).
 *
 * The (Nn) and (In) rules are also performed in that same single loop,
 * but effectively one iteration behind for white space.
 *
 * Since all implicit rules are performed in one step, it is not necessary
 * to actually store the intermediate directional properties in dirProps[].
 */

static void
processPropertySeq(UBiDi *pBiDi, LevState *pLevState, uint8_t _prop,
                   int32_t start, int32_t limit) {
    uint8_t cell, oldStateSeq, actionSeq;
    const ImpTab * pImpTab=pLevState->pImpTab;
    const ImpAct * pImpAct=pLevState->pImpAct;
    UBiDiLevel * levels=pBiDi->levels;
    UBiDiLevel level, addLevel;
    InsertPoints * pInsertPoints;
    int32_t start0, k;

    start0=start;                           /* save original start position */
    oldStateSeq=(uint8_t)pLevState->state;
    cell=(*pImpTab)[oldStateSeq][_prop];
    pLevState->state=GET_STATE(cell);       /* isolate the new state */
    actionSeq=(*pImpAct)[GET_ACTION(cell)]; /* isolate the action */
    addLevel=(*pImpTab)[pLevState->state][IMPTABLEVELS_RES];

    if(actionSeq) {
        switch(actionSeq) {
        case 1:                         /* init ON seq */
            pLevState->startON=start0;
            break;

        case 2:                         /* prepend ON seq to current seq */
            start=pLevState->startON;
            break;

        case 3:                         /* L or S after possible relevant EN/AN */
            /* check if we had EN after R/AL */
            if (pLevState->startL2EN >= 0) {
                addPoint(pBiDi, pLevState->startL2EN, LRM_BEFORE);
            }
            pLevState->startL2EN=-1;  /* not within previous if since could also be -2 */
            /* check if we had any relevant EN/AN after R/AL */
            pInsertPoints=&(pBiDi->insertPoints);
            if ((pInsertPoints->capacity == 0) ||
                (pInsertPoints->size <= pInsertPoints->confirmed))
            {
                /* nothing, just clean up */
                pLevState->lastStrongRTL=-1;
                /* check if we have a pending conditional segment */
                level=(*pImpTab)[oldStateSeq][IMPTABLEVELS_RES];
                if ((level & 1) && (pLevState->startON > 0)) {  /* after ON */
                    start=pLevState->startON;   /* reset to basic run level */
                }
                if (_prop == DirProp_S)                /* add LRM before S */
                {
                    addPoint(pBiDi, start0, LRM_BEFORE);
                    pInsertPoints->confirmed=pInsertPoints->size;
                }
                break;
            }
            /* reset previous RTL cont to level for LTR text */
            for (k=pLevState->lastStrongRTL+1; k<start0; k++)
            {
                /* reset odd level, leave runLevel+2 as is */
                levels[k]=(levels[k] - 2) & ~1;
            }
            /* mark insert points as confirmed */
            pInsertPoints->confirmed=pInsertPoints->size;
            pLevState->lastStrongRTL=-1;
            if (_prop == DirProp_S)            /* add LRM before S */
            {
                addPoint(pBiDi, start0, LRM_BEFORE);
                pInsertPoints->confirmed=pInsertPoints->size;
            }
            break;

        case 4:                         /* R/AL after possible relevant EN/AN */
            /* just clean up */
            pInsertPoints=&(pBiDi->insertPoints);
            if (pInsertPoints->capacity > 0)
                /* remove all non confirmed insert points */
                pInsertPoints->size=pInsertPoints->confirmed;
            pLevState->startON=-1;
            pLevState->startL2EN=-1;
            pLevState->lastStrongRTL=limit - 1;
            break;

        case 5:                         /* EN/AN after R/AL + possible cont */
            /* check for real AN */
            if ((_prop == DirProp_AN) && (NO_CONTEXT_RTL(pBiDi->dirProps[start0]) == AN) &&
                (pBiDi->reorderingMode!=UBIDI_REORDER_INVERSE_FOR_NUMBERS_SPECIAL))
            {
                /* real AN */
                if (pLevState->startL2EN == -1) /* if no relevant EN already found */
                {
                    /* just note the righmost digit as a strong RTL */
                    pLevState->lastStrongRTL=limit - 1;
                    break;
                }
                if (pLevState->startL2EN >= 0)  /* after EN, no AN */
                {
                    addPoint(pBiDi, pLevState->startL2EN, LRM_BEFORE);
                    pLevState->startL2EN=-2;
                }
                /* note AN */
                addPoint(pBiDi, start0, LRM_BEFORE);
                break;
            }
            /* if first EN/AN after R/AL */
            if (pLevState->startL2EN == -1) {
                pLevState->startL2EN=start0;
            }
            break;

        case 6:                         /* note location of latest R/AL */
            pLevState->lastStrongRTL=limit - 1;
            pLevState->startON=-1;
            break;

        case 7:                         /* L after R+ON/EN/AN */
            /* include possible adjacent number on the left */
            for (k=start0-1; k>=0 && !(levels[k]&1); k--);
            if(k>=0) {
                addPoint(pBiDi, k, RLM_BEFORE);             /* add RLM before */
                pInsertPoints=&(pBiDi->insertPoints);
                pInsertPoints->confirmed=pInsertPoints->size;   /* confirm it */
            }
            pLevState->startON=start0;
            break;

        case 8:                         /* AN after L */
            /* AN numbers between L text on both sides may be trouble. */
            /* tentatively bracket with LRMs; will be confirmed if followed by L */
            addPoint(pBiDi, start0, LRM_BEFORE);    /* add LRM before */
            addPoint(pBiDi, start0, LRM_AFTER);     /* add LRM after  */
            break;

        case 9:                         /* R after L+ON/EN/AN */
            /* false alert, infirm LRMs around previous AN */
            pInsertPoints=&(pBiDi->insertPoints);
            pInsertPoints->size=pInsertPoints->confirmed;
            if (_prop == DirProp_S)            /* add RLM before S */
            {
                addPoint(pBiDi, start0, RLM_BEFORE);
                pInsertPoints->confirmed=pInsertPoints->size;
            }
            break;

        case 10:                        /* L after L+ON/AN */
            level=pLevState->runLevel + addLevel;
            for(k=pLevState->startON; k<start0; k++) {
                if (levels[k]<level)
                    levels[k]=level;
            }
            pInsertPoints=&(pBiDi->insertPoints);
            pInsertPoints->confirmed=pInsertPoints->size;   /* confirm inserts */
            pLevState->startON=start0;
            break;

        case 11:                        /* L after L+ON+EN/AN/ON */
            level=pLevState->runLevel;
            for(k=start0-1; k>=pLevState->startON; k--) {
                if(levels[k]==level+3) {
                    while(levels[k]==level+3) {
                        levels[k--]-=2;
                    }
                    while(levels[k]==level) {
                        k--;
                    }
                }
                if(levels[k]==level+2) {
                    levels[k]=level;
                    continue;
                }
                levels[k]=level+1;
            }
            break;

        case 12:                        /* R after L+ON+EN/AN/ON */
            level=pLevState->runLevel+1;
            for(k=start0-1; k>=pLevState->startON; k--) {
                if(levels[k]>level) {
                    levels[k]-=2;
                }
            }
            break;

        default:                        /* we should never get here */
            U_ASSERT(FALSE);
            break;
        }
    }
    if((addLevel) || (start < start0)) {
        level=pLevState->runLevel + addLevel;
        for(k=start; k<limit; k++) {
            levels[k]=level;
        }
    }
}

static void
resolveImplicitLevels(UBiDi *pBiDi,
                      int32_t start, int32_t limit,
                      DirProp sor, DirProp eor) {
    const DirProp *dirProps=pBiDi->dirProps;

    LevState levState;
    int32_t i, start1, start2;
    uint8_t oldStateImp, stateImp, actionImp;
    uint8_t gprop, resProp, cell;
    UBool inverseRTL;
    DirProp nextStrongProp=R;
    int32_t nextStrongPos=-1;

    levState.startON = -1;  /* silence gcc flow analysis */

    /* check for RTL inverse BiDi mode */
    /* FOOD FOR THOUGHT: in case of RTL inverse BiDi, it would make sense to
     * loop on the text characters from end to start.
     * This would need a different properties state table (at least different
     * actions) and different levels state tables (maybe very similar to the
     * LTR corresponding ones.
     */
    inverseRTL=(UBool)
        ((start<pBiDi->lastArabicPos) && (GET_PARALEVEL(pBiDi, start) & 1) &&
         (pBiDi->reorderingMode==UBIDI_REORDER_INVERSE_LIKE_DIRECT  ||
          pBiDi->reorderingMode==UBIDI_REORDER_INVERSE_FOR_NUMBERS_SPECIAL));
    /* initialize for levels state table */
    levState.startL2EN=-1;              /* used for INVERSE_LIKE_DIRECT_WITH_MARKS */
    levState.lastStrongRTL=-1;          /* used for INVERSE_LIKE_DIRECT_WITH_MARKS */
    levState.state=0;
    levState.runLevel=pBiDi->levels[start];
    levState.pImpTab=(const ImpTab*)((pBiDi->pImpTabPair)->pImpTab)[levState.runLevel&1];
    levState.pImpAct=(const ImpAct*)((pBiDi->pImpTabPair)->pImpAct)[levState.runLevel&1];
    processPropertySeq(pBiDi, &levState, sor, start, start);
    /* initialize for property state table */
    if(NO_CONTEXT_RTL(dirProps[start])==NSM) {
        stateImp = 1 + sor;
    } else {
        stateImp=0;
    }
    start1=start;
    start2=start;

    for(i=start; i<=limit; i++) {
        if(i>=limit) {
            gprop=eor;
        } else {
            DirProp prop, prop1;
            prop=NO_CONTEXT_RTL(dirProps[i]);
            if(inverseRTL) {
                if(prop==AL) {
                    /* AL before EN does not make it AN */
                    prop=R;
                } else if(prop==EN) {
                    if(nextStrongPos<=i) {
                        /* look for next strong char (L/R/AL) */
                        int32_t j;
                        nextStrongProp=R;   /* set default */
                        nextStrongPos=limit;
                        for(j=i+1; j<limit; j++) {
                            prop1=NO_CONTEXT_RTL(dirProps[j]);
                            if(prop1==L || prop1==R || prop1==AL) {
                                nextStrongProp=prop1;
                                nextStrongPos=j;
                                break;
                            }
                        }
                    }
                    if(nextStrongProp==AL) {
                        prop=AN;
                    }
                }
            }
            gprop=groupProp[prop];
        }
        oldStateImp=stateImp;
        cell=impTabProps[oldStateImp][gprop];
        stateImp=GET_STATEPROPS(cell);      /* isolate the new state */
        actionImp=GET_ACTIONPROPS(cell);    /* isolate the action */
        if((i==limit) && (actionImp==0)) {
            /* there is an unprocessed sequence if its property == eor   */
            actionImp=1;                    /* process the last sequence */
        }
        if(actionImp) {
            resProp=impTabProps[oldStateImp][IMPTABPROPS_RES];
            switch(actionImp) {
            case 1:             /* process current seq1, init new seq1 */
                processPropertySeq(pBiDi, &levState, resProp, start1, i);
                start1=i;
                break;
            case 2:             /* init new seq2 */
                start2=i;
                break;
            case 3:             /* process seq1, process seq2, init new seq1 */
                processPropertySeq(pBiDi, &levState, resProp, start1, start2);
                processPropertySeq(pBiDi, &levState, DirProp_ON, start2, i);
                start1=i;
                break;
            case 4:             /* process seq1, set seq1=seq2, init new seq2 */
                processPropertySeq(pBiDi, &levState, resProp, start1, start2);
                start1=start2;
                start2=i;
                break;
            default:            /* we should never get here */
                U_ASSERT(FALSE);
                break;
            }
        }
    }
    /* flush possible pending sequence, e.g. ON */
    processPropertySeq(pBiDi, &levState, eor, limit, limit);
}

/* perform (L1) and (X9) ---------------------------------------------------- */

/*
 * Reset the embedding levels for some non-graphic characters (L1).
 * This function also sets appropriate levels for BN, and
 * explicit embedding types that are supposed to have been removed
 * from the paragraph in (X9).
 */
static void
adjustWSLevels(UBiDi *pBiDi) {
    const DirProp *dirProps=pBiDi->dirProps;
    UBiDiLevel *levels=pBiDi->levels;
    int32_t i;

    if(pBiDi->flags&MASK_WS) {
        UBool orderParagraphsLTR=pBiDi->orderParagraphsLTR;
        Flags flag;

        i=pBiDi->trailingWSStart;
        while(i>0) {
            /* reset a sequence of WS/BN before eop and B/S to the paragraph paraLevel */
            while(i>0 && (flag=DIRPROP_FLAG_NC(dirProps[--i]))&MASK_WS) {
                if(orderParagraphsLTR&&(flag&DIRPROP_FLAG(B))) {
                    levels[i]=0;
                } else {
                    levels[i]=GET_PARALEVEL(pBiDi, i);
                }
            }

            /* reset BN to the next character's paraLevel until B/S, which restarts above loop */
            /* here, i+1 is guaranteed to be <length */
            while(i>0) {
                flag=DIRPROP_FLAG_NC(dirProps[--i]);
                if(flag&MASK_BN_EXPLICIT) {
                    levels[i]=levels[i+1];
                } else if(orderParagraphsLTR&&(flag&DIRPROP_FLAG(B))) {
                    levels[i]=0;
                    break;
                } else if(flag&MASK_B_S) {
                    levels[i]=GET_PARALEVEL(pBiDi, i);
                    break;
                }
            }
        }
    }
}

#define BIDI_MIN(x, y)   ((x)<(y) ? (x) : (y))
#define BIDI_ABS(x)      ((x)>=0  ? (x) : (-(x)))
static void
setParaRunsOnly(UBiDi *pBiDi, const UChar *text, int32_t length,
                UBiDiLevel paraLevel, UErrorCode *pErrorCode) {
    void *runsOnlyMemory;
    int32_t *visualMap;
    UChar *visualText;
    int32_t saveLength, saveTrailingWSStart;
    const UBiDiLevel *levels;
    UBiDiLevel *saveLevels;
    UBiDiDirection saveDirection;
    UBool saveMayAllocateText;
    Run *runs;
    int32_t visualLength, i, j, visualStart, logicalStart,
            runCount, runLength, addedRuns, insertRemove,
            start, limit, step, indexOddBit, logicalPos,
            index0, index1;
    uint32_t saveOptions;

    pBiDi->reorderingMode=UBIDI_REORDER_DEFAULT;
    if(length==0) {
        ubidi_setPara(pBiDi, text, length, paraLevel, NULL, pErrorCode);
        goto cleanup3;
    }
    /* obtain memory for mapping table and visual text */
    runsOnlyMemory=uprv_malloc(length*(sizeof(int32_t)+sizeof(UChar)+sizeof(UBiDiLevel)));
    if(runsOnlyMemory==NULL) {
        *pErrorCode=U_MEMORY_ALLOCATION_ERROR;
        goto cleanup3;
    }
    visualMap=runsOnlyMemory;
    visualText=(UChar *)&visualMap[length];
    saveLevels=(UBiDiLevel *)&visualText[length];
    saveOptions=pBiDi->reorderingOptions;
    if(saveOptions & UBIDI_OPTION_INSERT_MARKS) {
        pBiDi->reorderingOptions&=~UBIDI_OPTION_INSERT_MARKS;
        pBiDi->reorderingOptions|=UBIDI_OPTION_REMOVE_CONTROLS;
    }
    paraLevel&=1;                       /* accept only 0 or 1 */
    ubidi_setPara(pBiDi, text, length, paraLevel, NULL, pErrorCode);
    if(U_FAILURE(*pErrorCode)) {
        goto cleanup3;
    }
    /* we cannot access directly pBiDi->levels since it is not yet set if
     * direction is not MIXED
     */
    levels=ubidi_getLevels(pBiDi, pErrorCode);
    uprv_memcpy(saveLevels, levels, pBiDi->length*sizeof(UBiDiLevel));
    saveTrailingWSStart=pBiDi->trailingWSStart;
    saveLength=pBiDi->length;
    saveDirection=pBiDi->direction;

    /* FOOD FOR THOUGHT: instead of writing the visual text, we could use
     * the visual map and the dirProps array to drive the second call
     * to ubidi_setPara (but must make provision for possible removal of
     * BiDi controls.  Alternatively, only use the dirProps array via
     * customized classifier callback.
     */
    visualLength=ubidi_writeReordered(pBiDi, visualText, length,
                                      UBIDI_DO_MIRRORING, pErrorCode);
    ubidi_getVisualMap(pBiDi, visualMap, pErrorCode);
    if(U_FAILURE(*pErrorCode)) {
        goto cleanup2;
    }
    pBiDi->reorderingOptions=saveOptions;

    pBiDi->reorderingMode=UBIDI_REORDER_INVERSE_LIKE_DIRECT;
    paraLevel^=1;
    /* Because what we did with reorderingOptions, visualText may be shorter
     * than the original text. But we don't want the levels memory to be
     * reallocated shorter than the original length, since we need to restore
     * the levels as after the first call to ubidi_setpara() before returning.
     * We will force mayAllocateText to FALSE before the second call to
     * ubidi_setpara(), and will restore it afterwards.
     */
    saveMayAllocateText=pBiDi->mayAllocateText;
    pBiDi->mayAllocateText=FALSE;
    ubidi_setPara(pBiDi, visualText, visualLength, paraLevel, NULL, pErrorCode);
    pBiDi->mayAllocateText=saveMayAllocateText;
    ubidi_getRuns(pBiDi, pErrorCode);
    if(U_FAILURE(*pErrorCode)) {
        goto cleanup1;
    }
    /* check if some runs must be split, count how many splits */
    addedRuns=0;
    runCount=pBiDi->runCount;
    runs=pBiDi->runs;
    visualStart=0;
    for(i=0; i<runCount; i++, visualStart+=runLength) {
        runLength=runs[i].visualLimit-visualStart;
        if(runLength<2) {
            continue;
        }
        logicalStart=GET_INDEX(runs[i].logicalStart);
        for(j=logicalStart+1; j<logicalStart+runLength; j++) {
            index0=visualMap[j];
            index1=visualMap[j-1];
            if((BIDI_ABS(index0-index1)!=1) || (saveLevels[index0]!=saveLevels[index1])) {
                addedRuns++;
            }
        }
    }
    if(addedRuns) {
        if(getRunsMemory(pBiDi, runCount+addedRuns)) {
            if(runCount==1) {
                /* because we switch from UBiDi.simpleRuns to UBiDi.runs */
                pBiDi->runsMemory[0]=runs[0];
            }
            runs=pBiDi->runs=pBiDi->runsMemory;
            pBiDi->runCount+=addedRuns;
        } else {
            goto cleanup1;
        }
    }
    /* split runs which are not consecutive in source text */
    for(i=runCount-1; i>=0; i--) {
        runLength= i==0 ? runs[0].visualLimit :
                          runs[i].visualLimit-runs[i-1].visualLimit;
        logicalStart=runs[i].logicalStart;
        indexOddBit=GET_ODD_BIT(logicalStart);
        logicalStart=GET_INDEX(logicalStart);
        if(runLength<2) {
            if(addedRuns) {
                runs[i+addedRuns]=runs[i];
            }
            logicalPos=visualMap[logicalStart];
            runs[i+addedRuns].logicalStart=MAKE_INDEX_ODD_PAIR(logicalPos,
                                            saveLevels[logicalPos]^indexOddBit);
            continue;
        }
        if(indexOddBit) {
            start=logicalStart;
            limit=logicalStart+runLength-1;
            step=1;
        } else {
            start=logicalStart+runLength-1;
            limit=logicalStart;
            step=-1;
        }
        for(j=start; j!=limit; j+=step) {
            index0=visualMap[j];
            index1=visualMap[j+step];
            if((BIDI_ABS(index0-index1)!=1) || (saveLevels[index0]!=saveLevels[index1])) {
                logicalPos=BIDI_MIN(visualMap[start], index0);
                runs[i+addedRuns].logicalStart=MAKE_INDEX_ODD_PAIR(logicalPos,
                                            saveLevels[logicalPos]^indexOddBit);
                runs[i+addedRuns].visualLimit=runs[i].visualLimit;
                runs[i].visualLimit-=BIDI_ABS(j-start)+1;
                insertRemove=runs[i].insertRemove&(LRM_AFTER|RLM_AFTER);
                runs[i+addedRuns].insertRemove=insertRemove;
                runs[i].insertRemove&=~insertRemove;
                start=j+step;
                addedRuns--;
            }
        }
        if(addedRuns) {
            runs[i+addedRuns]=runs[i];
        }
        logicalPos=BIDI_MIN(visualMap[start], visualMap[limit]);
        runs[i+addedRuns].logicalStart=MAKE_INDEX_ODD_PAIR(logicalPos,
                                            saveLevels[logicalPos]^indexOddBit);
    }

  cleanup1:
    /* restore initial paraLevel */
    pBiDi->paraLevel^=1;
  cleanup2:
    /* restore real text */
    pBiDi->text=text;
    pBiDi->length=saveLength;
    pBiDi->originalLength=length;
    pBiDi->direction=saveDirection;
    /* the saved levels should never excess levelsSize, but we check anyway */
    if(saveLength>pBiDi->levelsSize) {
        saveLength=pBiDi->levelsSize;
    }
    uprv_memcpy(pBiDi->levels, saveLevels, saveLength*sizeof(UBiDiLevel));
    pBiDi->trailingWSStart=saveTrailingWSStart;
    /* free memory for mapping table and visual text */
    uprv_free(runsOnlyMemory);
    if(pBiDi->runCount>1) {
        pBiDi->direction=UBIDI_MIXED;
    }
  cleanup3:
    pBiDi->reorderingMode=UBIDI_REORDER_RUNS_ONLY;
}

/* ubidi_setPara ------------------------------------------------------------ */

U_CAPI void U_EXPORT2
ubidi_setPara(UBiDi *pBiDi, const UChar *text, int32_t length,
              UBiDiLevel paraLevel, UBiDiLevel *embeddingLevels,
              UErrorCode *pErrorCode) {
    UBiDiDirection direction;

    /* check the argument values */
    RETURN_VOID_IF_NULL_OR_FAILING_ERRCODE(pErrorCode);
    if(pBiDi==NULL || text==NULL || length<-1 ||
       (paraLevel>UBIDI_MAX_EXPLICIT_LEVEL && paraLevel<UBIDI_DEFAULT_LTR)) {
        *pErrorCode=U_ILLEGAL_ARGUMENT_ERROR;
        return;
    }

    if(length==-1) {
        length=u_strlen(text);
    }

    /* special treatment for RUNS_ONLY mode */
    if(pBiDi->reorderingMode==UBIDI_REORDER_RUNS_ONLY) {
        setParaRunsOnly(pBiDi, text, length, paraLevel, pErrorCode);
        return;
    }

    /* initialize the UBiDi structure */
    pBiDi->pParaBiDi=NULL;          /* mark unfinished setPara */
    pBiDi->text=text;
    pBiDi->length=pBiDi->originalLength=pBiDi->resultLength=length;
    pBiDi->paraLevel=paraLevel;
    pBiDi->direction=UBIDI_LTR;
    pBiDi->paraCount=1;

    pBiDi->dirProps=NULL;
    pBiDi->levels=NULL;
    pBiDi->runs=NULL;
    pBiDi->insertPoints.size=0;         /* clean up from last call */
    pBiDi->insertPoints.confirmed=0;    /* clean up from last call */

    /*
     * Save the original paraLevel if contextual; otherwise, set to 0.
     */
    if(IS_DEFAULT_LEVEL(paraLevel)) {
        pBiDi->defaultParaLevel=paraLevel;
    } else {
        pBiDi->defaultParaLevel=0;
    }

    if(length==0) {
        /*
         * For an empty paragraph, create a UBiDi object with the paraLevel and
         * the flags and the direction set but without allocating zero-length arrays.
         * There is nothing more to do.
         */
        if(IS_DEFAULT_LEVEL(paraLevel)) {
            pBiDi->paraLevel&=1;
            pBiDi->defaultParaLevel=0;
        }
        if(paraLevel&1) {
            pBiDi->flags=DIRPROP_FLAG(R);
            pBiDi->direction=UBIDI_RTL;
        } else {
            pBiDi->flags=DIRPROP_FLAG(L);
            pBiDi->direction=UBIDI_LTR;
        }

        pBiDi->runCount=0;
        pBiDi->paraCount=0;
        pBiDi->pParaBiDi=pBiDi;         /* mark successful setPara */
        return;
    }

    pBiDi->runCount=-1;

    /*
     * Get the directional properties,
     * the flags bit-set, and
     * determine the paragraph level if necessary.
     */
    if(getDirPropsMemory(pBiDi, length)) {
        pBiDi->dirProps=pBiDi->dirPropsMemory;
        getDirProps(pBiDi);
    } else {
        *pErrorCode=U_MEMORY_ALLOCATION_ERROR;
        return;
    }
    /* the processed length may have changed if UBIDI_OPTION_STREAMING */
    length= pBiDi->length;
    pBiDi->trailingWSStart=length;  /* the levels[] will reflect the WS run */
    /* allocate paras memory */
    if(pBiDi->paraCount>1) {
        if(getInitialParasMemory(pBiDi, pBiDi->paraCount)) {
            pBiDi->paras=pBiDi->parasMemory;
            pBiDi->paras[pBiDi->paraCount-1]=length;
        } else {
            *pErrorCode=U_MEMORY_ALLOCATION_ERROR;
            return;
        }
    } else {
        /* initialize paras for single paragraph */
        pBiDi->paras=pBiDi->simpleParas;
        pBiDi->simpleParas[0]=length;
    }

    /* are explicit levels specified? */
    if(embeddingLevels==NULL) {
        /* no: determine explicit levels according to the (Xn) rules */\
        if(getLevelsMemory(pBiDi, length)) {
            pBiDi->levels=pBiDi->levelsMemory;
            direction=resolveExplicitLevels(pBiDi);
        } else {
            *pErrorCode=U_MEMORY_ALLOCATION_ERROR;
            return;
        }
    } else {
        /* set BN for all explicit codes, check that all levels are 0 or paraLevel..UBIDI_MAX_EXPLICIT_LEVEL */
        pBiDi->levels=embeddingLevels;
        direction=checkExplicitLevels(pBiDi, pErrorCode);
        if(U_FAILURE(*pErrorCode)) {
            return;
        }
    }

    /*
     * The steps after (X9) in the UBiDi algorithm are performed only if
     * the paragraph text has mixed directionality!
     */
    pBiDi->direction=direction;
    switch(direction) {
    case UBIDI_LTR:
        /* make sure paraLevel is even */
        pBiDi->paraLevel=(UBiDiLevel)((pBiDi->paraLevel+1)&~1);

        /* all levels are implicitly at paraLevel (important for ubidi_getLevels()) */
        pBiDi->trailingWSStart=0;
        break;
    case UBIDI_RTL:
        /* make sure paraLevel is odd */
        pBiDi->paraLevel|=1;

        /* all levels are implicitly at paraLevel (important for ubidi_getLevels()) */
        pBiDi->trailingWSStart=0;
        break;
    default:
        /*
         *  Choose the right implicit state table
         */
        switch(pBiDi->reorderingMode) {
        case UBIDI_REORDER_DEFAULT:
            pBiDi->pImpTabPair=&impTab_DEFAULT;
            break;
        case UBIDI_REORDER_NUMBERS_SPECIAL:
            pBiDi->pImpTabPair=&impTab_NUMBERS_SPECIAL;
            break;
        case UBIDI_REORDER_GROUP_NUMBERS_WITH_R:
            pBiDi->pImpTabPair=&impTab_GROUP_NUMBERS_WITH_R;
            break;
        case UBIDI_REORDER_INVERSE_NUMBERS_AS_L:
            pBiDi->pImpTabPair=&impTab_INVERSE_NUMBERS_AS_L;
            break;
        case UBIDI_REORDER_INVERSE_LIKE_DIRECT:
            if (pBiDi->reorderingOptions & UBIDI_OPTION_INSERT_MARKS) {
                pBiDi->pImpTabPair=&impTab_INVERSE_LIKE_DIRECT_WITH_MARKS;
            } else {
                pBiDi->pImpTabPair=&impTab_INVERSE_LIKE_DIRECT;
            }
            break;
        case UBIDI_REORDER_INVERSE_FOR_NUMBERS_SPECIAL:
            if (pBiDi->reorderingOptions & UBIDI_OPTION_INSERT_MARKS) {
                pBiDi->pImpTabPair=&impTab_INVERSE_FOR_NUMBERS_SPECIAL_WITH_MARKS;
            } else {
                pBiDi->pImpTabPair=&impTab_INVERSE_FOR_NUMBERS_SPECIAL;
            }
            break;
        default:
            /* we should never get here */
            U_ASSERT(FALSE);
            break;
        }
        /*
         * If there are no external levels specified and there
         * are no significant explicit level codes in the text,
         * then we can treat the entire paragraph as one run.
         * Otherwise, we need to perform the following rules on runs of
         * the text with the same embedding levels. (X10)
         * "Significant" explicit level codes are ones that actually
         * affect non-BN characters.
         * Examples for "insignificant" ones are empty embeddings
         * LRE-PDF, LRE-RLE-PDF-PDF, etc.
         */
        if(embeddingLevels==NULL && pBiDi->paraCount<=1 &&
                                   !(pBiDi->flags&DIRPROP_FLAG_MULTI_RUNS)) {
            resolveImplicitLevels(pBiDi, 0, length,
                                    GET_LR_FROM_LEVEL(GET_PARALEVEL(pBiDi, 0)),
                                    GET_LR_FROM_LEVEL(GET_PARALEVEL(pBiDi, length-1)));
        } else {
            /* sor, eor: start and end types of same-level-run */
            UBiDiLevel *levels=pBiDi->levels;
            int32_t start, limit=0;
            UBiDiLevel level, nextLevel;
            DirProp sor, eor;

            /* determine the first sor and set eor to it because of the loop body (sor=eor there) */
            level=GET_PARALEVEL(pBiDi, 0);
            nextLevel=levels[0];
            if(level<nextLevel) {
                eor=GET_LR_FROM_LEVEL(nextLevel);
            } else {
                eor=GET_LR_FROM_LEVEL(level);
            }

            do {
                /* determine start and limit of the run (end points just behind the run) */

                /* the values for this run's start are the same as for the previous run's end */
                start=limit;
                level=nextLevel;
                if((start>0) && (NO_CONTEXT_RTL(pBiDi->dirProps[start-1])==B)) {
                    /* except if this is a new paragraph, then set sor = para level */
                    sor=GET_LR_FROM_LEVEL(GET_PARALEVEL(pBiDi, start));
                } else {
                    sor=eor;
                }

                /* search for the limit of this run */
                while(++limit<length && levels[limit]==level) {}

                /* get the correct level of the next run */
                if(limit<length) {
                    nextLevel=levels[limit];
                } else {
                    nextLevel=GET_PARALEVEL(pBiDi, length-1);
                }

                /* determine eor from max(level, nextLevel); sor is last run's eor */
                if((level&~UBIDI_LEVEL_OVERRIDE)<(nextLevel&~UBIDI_LEVEL_OVERRIDE)) {
                    eor=GET_LR_FROM_LEVEL(nextLevel);
                } else {
                    eor=GET_LR_FROM_LEVEL(level);
                }

                /* if the run consists of overridden directional types, then there
                   are no implicit types to be resolved */
                if(!(level&UBIDI_LEVEL_OVERRIDE)) {
                    resolveImplicitLevels(pBiDi, start, limit, sor, eor);
                } else {
                    /* remove the UBIDI_LEVEL_OVERRIDE flags */
                    do {
                        levels[start++]&=~UBIDI_LEVEL_OVERRIDE;
                    } while(start<limit);
                }
            } while(limit<length);
        }
        /* check if we got any memory shortage while adding insert points */
        if (U_FAILURE(pBiDi->insertPoints.errorCode))
        {
            *pErrorCode=pBiDi->insertPoints.errorCode;
            return;
        }
        /* reset the embedding levels for some non-graphic characters (L1), (X9) */
        adjustWSLevels(pBiDi);
        break;
    }
    /* add RLM for inverse Bidi with contextual orientation resolving
     * to RTL which would not round-trip otherwise
     */
    if((pBiDi->defaultParaLevel>0) &&
       (pBiDi->reorderingOptions & UBIDI_OPTION_INSERT_MARKS) &&
       ((pBiDi->reorderingMode==UBIDI_REORDER_INVERSE_LIKE_DIRECT) ||
        (pBiDi->reorderingMode==UBIDI_REORDER_INVERSE_FOR_NUMBERS_SPECIAL))) {
        int32_t i, j, start, last;
        DirProp dirProp;
        for(i=0; i<pBiDi->paraCount; i++) {
            last=pBiDi->paras[i]-1;
            if((pBiDi->dirProps[last] & CONTEXT_RTL)==0) {
                continue;           /* LTR paragraph */
            }
            start= i==0 ? 0 : pBiDi->paras[i - 1];
            for(j=last; j>=start; j--) {
                dirProp=NO_CONTEXT_RTL(pBiDi->dirProps[j]);
                if(dirProp==L) {
                    if(j<last) {
                        while(NO_CONTEXT_RTL(pBiDi->dirProps[last])==B) {
                            last--;
                        }
                    }
                    addPoint(pBiDi, last, RLM_BEFORE);
                    break;
                }
                if(DIRPROP_FLAG(dirProp) & MASK_R_AL) {
                    break;
                }
            }
        }
    }

    if(pBiDi->reorderingOptions & UBIDI_OPTION_REMOVE_CONTROLS) {
        pBiDi->resultLength -= pBiDi->controlCount;
    } else {
        pBiDi->resultLength += pBiDi->insertPoints.size;
    }
    pBiDi->pParaBiDi=pBiDi;             /* mark successful setPara */
}

U_CAPI void U_EXPORT2
ubidi_orderParagraphsLTR(UBiDi *pBiDi, UBool orderParagraphsLTR) {
    if(pBiDi!=NULL) {
        pBiDi->orderParagraphsLTR=orderParagraphsLTR;
    }
}

U_CAPI UBool U_EXPORT2
ubidi_isOrderParagraphsLTR(UBiDi *pBiDi) {
    if(pBiDi!=NULL) {
        return pBiDi->orderParagraphsLTR;
    } else {
        return FALSE;
    }
}

U_CAPI UBiDiDirection U_EXPORT2
ubidi_getDirection(const UBiDi *pBiDi) {
    if(IS_VALID_PARA_OR_LINE(pBiDi)) {
        return pBiDi->direction;
    } else {
        return UBIDI_LTR;
    }
}

U_CAPI const UChar * U_EXPORT2
ubidi_getText(const UBiDi *pBiDi) {
    if(IS_VALID_PARA_OR_LINE(pBiDi)) {
        return pBiDi->text;
    } else {
        return NULL;
    }
}

U_CAPI int32_t U_EXPORT2
ubidi_getLength(const UBiDi *pBiDi) {
    if(IS_VALID_PARA_OR_LINE(pBiDi)) {
        return pBiDi->originalLength;
    } else {
        return 0;
    }
}

U_CAPI int32_t U_EXPORT2
ubidi_getProcessedLength(const UBiDi *pBiDi) {
    if(IS_VALID_PARA_OR_LINE(pBiDi)) {
        return pBiDi->length;
    } else {
        return 0;
    }
}

U_CAPI int32_t U_EXPORT2
ubidi_getResultLength(const UBiDi *pBiDi) {
    if(IS_VALID_PARA_OR_LINE(pBiDi)) {
        return pBiDi->resultLength;
    } else {
        return 0;
    }
}

/* paragraphs API functions ------------------------------------------------- */

U_CAPI UBiDiLevel U_EXPORT2
ubidi_getParaLevel(const UBiDi *pBiDi) {
    if(IS_VALID_PARA_OR_LINE(pBiDi)) {
        return pBiDi->paraLevel;
    } else {
        return 0;
    }
}

U_CAPI int32_t U_EXPORT2
ubidi_countParagraphs(UBiDi *pBiDi) {
    if(!IS_VALID_PARA_OR_LINE(pBiDi)) {
        return 0;
    } else {
        return pBiDi->paraCount;
    }
}

U_CAPI void U_EXPORT2
ubidi_getParagraphByIndex(const UBiDi *pBiDi, int32_t paraIndex,
                          int32_t *pParaStart, int32_t *pParaLimit,
                          UBiDiLevel *pParaLevel, UErrorCode *pErrorCode) {
    int32_t paraStart;

    /* check the argument values */
    RETURN_VOID_IF_NULL_OR_FAILING_ERRCODE(pErrorCode);
    RETURN_VOID_IF_NOT_VALID_PARA_OR_LINE(pBiDi, *pErrorCode);
    RETURN_VOID_IF_BAD_RANGE(paraIndex, 0, pBiDi->paraCount, *pErrorCode);

    pBiDi=pBiDi->pParaBiDi;             /* get Para object if Line object */
    if(paraIndex) {
        paraStart=pBiDi->paras[paraIndex-1];
    } else {
        paraStart=0;
    }
    if(pParaStart!=NULL) {
        *pParaStart=paraStart;
    }
    if(pParaLimit!=NULL) {
        *pParaLimit=pBiDi->paras[paraIndex];
    }
    if(pParaLevel!=NULL) {
        *pParaLevel=GET_PARALEVEL(pBiDi, paraStart);
    }
}

U_CAPI int32_t U_EXPORT2
ubidi_getParagraph(const UBiDi *pBiDi, int32_t charIndex,
                          int32_t *pParaStart, int32_t *pParaLimit,
                          UBiDiLevel *pParaLevel, UErrorCode *pErrorCode) {
    uint32_t paraIndex;

    /* check the argument values */
    /* pErrorCode will be checked by the call to ubidi_getParagraphByIndex */
    RETURN_IF_NULL_OR_FAILING_ERRCODE(pErrorCode, -1);
    RETURN_IF_NOT_VALID_PARA_OR_LINE(pBiDi, *pErrorCode, -1);
    pBiDi=pBiDi->pParaBiDi;             /* get Para object if Line object */
    RETURN_IF_BAD_RANGE(charIndex, 0, pBiDi->length, *pErrorCode, -1);

    for(paraIndex=0; charIndex>=pBiDi->paras[paraIndex]; paraIndex++);
    ubidi_getParagraphByIndex(pBiDi, paraIndex, pParaStart, pParaLimit, pParaLevel, pErrorCode);
    return paraIndex;
}

U_CAPI void U_EXPORT2
ubidi_setClassCallback(UBiDi *pBiDi, UBiDiClassCallback *newFn,
                       const void *newContext, UBiDiClassCallback **oldFn,
                       const void **oldContext, UErrorCode *pErrorCode)
{
    RETURN_VOID_IF_NULL_OR_FAILING_ERRCODE(pErrorCode);
    if(pBiDi==NULL) {
        *pErrorCode=U_ILLEGAL_ARGUMENT_ERROR;
        return;
    }
    if( oldFn )
    {
        *oldFn = pBiDi->fnClassCallback;
    }
    if( oldContext )
    {
        *oldContext = pBiDi->coClassCallback;
    }
    pBiDi->fnClassCallback = newFn;
    pBiDi->coClassCallback = newContext;
}

U_CAPI void U_EXPORT2
ubidi_getClassCallback(UBiDi *pBiDi, UBiDiClassCallback **fn, const void **context)
{
    if(pBiDi==NULL) {
        return;
    }
    if( fn )
    {
        *fn = pBiDi->fnClassCallback;
    }
    if( context )
    {
        *context = pBiDi->coClassCallback;
    }
}

U_CAPI UCharDirection U_EXPORT2
ubidi_getCustomizedClass(UBiDi *pBiDi, UChar32 c)
{
    UCharDirection dir;

    if( pBiDi->fnClassCallback == NULL ||
        (dir = (*pBiDi->fnClassCallback)(pBiDi->coClassCallback, c)) == U_BIDI_CLASS_DEFAULT )
    {
        return ubidi_getClass(pBiDi->bdp, c);
    } else {
        return dir;
    }
}

