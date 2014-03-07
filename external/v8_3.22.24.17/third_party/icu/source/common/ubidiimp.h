/*
******************************************************************************
*
*   Copyright (C) 1999-2007, International Business Machines
*   Corporation and others.  All Rights Reserved.
*
******************************************************************************
*   file name:  ubidiimp.h
*   encoding:   US-ASCII
*   tab size:   8 (not used)
*   indentation:4
*
*   created on: 1999aug06
*   created by: Markus W. Scherer, updated by Matitiahu Allouche
*/

#ifndef UBIDIIMP_H
#define UBIDIIMP_H

/* set import/export definitions */
#ifdef U_COMMON_IMPLEMENTATION

#include "unicode/utypes.h"
#include "unicode/uchar.h"
#include "ubidi_props.h"

/* miscellaneous definitions ---------------------------------------------- */

typedef uint8_t DirProp;
typedef uint32_t Flags;

/*  Comparing the description of the BiDi algorithm with this implementation
    is easier with the same names for the BiDi types in the code as there.
    See UCharDirection in uchar.h .
*/
enum {
    L=  U_LEFT_TO_RIGHT,
    R=  U_RIGHT_TO_LEFT,
    EN= U_EUROPEAN_NUMBER,
    ES= U_EUROPEAN_NUMBER_SEPARATOR,
    ET= U_EUROPEAN_NUMBER_TERMINATOR,
    AN= U_ARABIC_NUMBER,
    CS= U_COMMON_NUMBER_SEPARATOR,
    B=  U_BLOCK_SEPARATOR,
    S=  U_SEGMENT_SEPARATOR,
    WS= U_WHITE_SPACE_NEUTRAL,
    ON= U_OTHER_NEUTRAL,
    LRE=U_LEFT_TO_RIGHT_EMBEDDING,
    LRO=U_LEFT_TO_RIGHT_OVERRIDE,
    AL= U_RIGHT_TO_LEFT_ARABIC,
    RLE=U_RIGHT_TO_LEFT_EMBEDDING,
    RLO=U_RIGHT_TO_LEFT_OVERRIDE,
    PDF=U_POP_DIRECTIONAL_FORMAT,
    NSM=U_DIR_NON_SPACING_MARK,
    BN= U_BOUNDARY_NEUTRAL,
    dirPropCount
};

/*
 * Sometimes, bit values are more appropriate
 * to deal with directionality properties.
 * Abbreviations in these macro names refer to names
 * used in the BiDi algorithm.
 */
#define DIRPROP_FLAG(dir) (1UL<<(dir))

/* special flag for multiple runs from explicit embedding codes */
#define DIRPROP_FLAG_MULTI_RUNS (1UL<<31)

/* are there any characters that are LTR or RTL? */
#define MASK_LTR (DIRPROP_FLAG(L)|DIRPROP_FLAG(EN)|DIRPROP_FLAG(AN)|DIRPROP_FLAG(LRE)|DIRPROP_FLAG(LRO))
#define MASK_RTL (DIRPROP_FLAG(R)|DIRPROP_FLAG(AL)|DIRPROP_FLAG(RLE)|DIRPROP_FLAG(RLO))
#define MASK_R_AL (DIRPROP_FLAG(R)|DIRPROP_FLAG(AL))

/* explicit embedding codes */
#define MASK_LRX (DIRPROP_FLAG(LRE)|DIRPROP_FLAG(LRO))
#define MASK_RLX (DIRPROP_FLAG(RLE)|DIRPROP_FLAG(RLO))
#define MASK_OVERRIDE (DIRPROP_FLAG(LRO)|DIRPROP_FLAG(RLO))

#define MASK_EXPLICIT (MASK_LRX|MASK_RLX|DIRPROP_FLAG(PDF))
#define MASK_BN_EXPLICIT (DIRPROP_FLAG(BN)|MASK_EXPLICIT)

/* paragraph and segment separators */
#define MASK_B_S (DIRPROP_FLAG(B)|DIRPROP_FLAG(S))

/* all types that are counted as White Space or Neutral in some steps */
#define MASK_WS (MASK_B_S|DIRPROP_FLAG(WS)|MASK_BN_EXPLICIT)
#define MASK_N (DIRPROP_FLAG(ON)|MASK_WS)

/* all types that are included in a sequence of European Terminators for (W5) */
#define MASK_ET_NSM_BN (DIRPROP_FLAG(ET)|DIRPROP_FLAG(NSM)|MASK_BN_EXPLICIT)

/* types that are neutrals or could becomes neutrals in (Wn) */
#define MASK_POSSIBLE_N (DIRPROP_FLAG(CS)|DIRPROP_FLAG(ES)|DIRPROP_FLAG(ET)|MASK_N)

/*
 * These types may be changed to "e",
 * the embedding type (L or R) of the run,
 * in the BiDi algorithm (N2)
 */
#define MASK_EMBEDDING (DIRPROP_FLAG(NSM)|MASK_POSSIBLE_N)

/* the dirProp's L and R are defined to 0 and 1 values in UCharDirection */
#define GET_LR_FROM_LEVEL(level) ((DirProp)((level)&1))

#define IS_DEFAULT_LEVEL(level) ((level)>=0xfe)

/*
 * The following bit is ORed to the property of characters in paragraphs
 * with contextual RTL direction when paraLevel is contextual.
 */
#define CONTEXT_RTL 0x80
#define NO_CONTEXT_RTL(dir) ((dir)&~CONTEXT_RTL)
/*
 * The following is a variant of DIRPROP_FLAG which ignores the CONTEXT_RTL bit.
 */
#define DIRPROP_FLAG_NC(dir) (1UL<<(NO_CONTEXT_RTL(dir)))

#define GET_PARALEVEL(ubidi, index) \
            (UBiDiLevel)((ubidi)->defaultParaLevel ? (ubidi)->dirProps[index]>>7 \
                                                   : (ubidi)->paraLevel)

/* Paragraph type for multiple paragraph support ---------------------------- */
typedef int32_t Para;

#define CR  0x000D
#define LF  0x000A

/* Run structure for reordering --------------------------------------------- */
enum {
    LRM_BEFORE=1,
    LRM_AFTER=2,
    RLM_BEFORE=4,
    RLM_AFTER=8
};

typedef struct Run {
    int32_t logicalStart,   /* first character of the run; b31 indicates even/odd level */
            visualLimit,    /* last visual position of the run +1 */
            insertRemove;   /* if >0, flags for inserting LRM/RLM before/after run,
                               if <0, count of bidi controls within run            */
} Run;

/* in a Run, logicalStart will get this bit set if the run level is odd */
#define INDEX_ODD_BIT (1UL<<31)

#define MAKE_INDEX_ODD_PAIR(index, level) ((index)|((int32_t)(level)<<31))
#define ADD_ODD_BIT_FROM_LEVEL(x, level)  ((x)|=((int32_t)(level)<<31))
#define REMOVE_ODD_BIT(x)                 ((x)&=~INDEX_ODD_BIT)

#define GET_INDEX(x)   ((x)&~INDEX_ODD_BIT)
#define GET_ODD_BIT(x) ((uint32_t)(x)>>31)
#define IS_ODD_RUN(x)  ((UBool)(((x)&INDEX_ODD_BIT)!=0))
#define IS_EVEN_RUN(x) ((UBool)(((x)&INDEX_ODD_BIT)==0))

U_CFUNC UBool
ubidi_getRuns(UBiDi *pBiDi, UErrorCode *pErrorCode);

/** BiDi control code points */
enum {
    ZWNJ_CHAR=0x200c,
    ZWJ_CHAR,
    LRM_CHAR,
    RLM_CHAR,
    LRE_CHAR=0x202a,
    RLE_CHAR,
    PDF_CHAR,
    LRO_CHAR,
    RLO_CHAR
};

#define IS_BIDI_CONTROL_CHAR(c) (((uint32_t)(c)&0xfffffffc)==ZWNJ_CHAR || (uint32_t)((c)-LRE_CHAR)<5)

/* InsertPoints structure for noting where to put BiDi marks ---------------- */

typedef struct Point {
    int32_t pos;            /* position in text */
    int32_t flag;           /* flag for LRM/RLM, before/after */
} Point;

typedef struct InsertPoints {
    int32_t capacity;       /* number of points allocated */
    int32_t size;           /* number of points used */
    int32_t confirmed;      /* number of points confirmed */
    UErrorCode errorCode;   /* for eventual memory shortage */
    Point *points;          /* pointer to array of points */
} InsertPoints;


/* UBiDi structure ----------------------------------------------------------- */

struct UBiDi {
    /* pointer to parent paragraph object (pointer to self if this object is
     * a paragraph object); set to NULL in a newly opened object; set to a
     * real value after a successful execution of ubidi_setPara or ubidi_setLine
     */
    const UBiDi * pParaBiDi;

    const UBiDiProps *bdp;

    /* alias pointer to the current text */
    const UChar *text;

    /* length of the current text */
    int32_t originalLength;

    /* if the UBIDI_OPTION_STREAMING option is set, this is the length
     * of text actually processed by ubidi_setPara, which may be shorter than
     * the original length.
     * Otherwise, it is identical to the original length.
     */
    int32_t length;

    /* if the UBIDI_OPTION_REMOVE_CONTROLS option is set, and/or
     * marks are allowed to be inserted in one of the reordering mode, the
     * length of the result string may be different from the processed length.
     */
    int32_t resultLength;

    /* memory sizes in bytes */
    int32_t dirPropsSize, levelsSize, parasSize, runsSize;

    /* allocated memory */
    DirProp *dirPropsMemory;
    UBiDiLevel *levelsMemory;
    Para *parasMemory;
    Run *runsMemory;

    /* indicators for whether memory may be allocated after ubidi_open() */
    UBool mayAllocateText, mayAllocateRuns;

    /* arrays with one value per text-character */
    const DirProp *dirProps;
    UBiDiLevel *levels;

    /* are we performing an approximation of the "inverse BiDi" algorithm? */
    UBool isInverse;

    /* are we using the basic algorithm or its variation? */
    UBiDiReorderingMode reorderingMode;

    /* UBIDI_REORDER_xxx values must be ordered so that all the regular
     * logical to visual modes come first, and all inverse BiDi modes
     * come last.
     */
    #define UBIDI_REORDER_LAST_LOGICAL_TO_VISUAL    UBIDI_REORDER_NUMBERS_SPECIAL

    /* bitmask for reordering options */
    uint32_t reorderingOptions;

    /* must block separators receive level 0? */
    UBool orderParagraphsLTR;

    /* the paragraph level */
    UBiDiLevel paraLevel;
    /* original paraLevel when contextual */
    /* must be one of UBIDI_DEFAULT_xxx or 0 if not contextual */
    UBiDiLevel defaultParaLevel;

    /* the following is set in ubidi_setPara, used in processPropertySeq */
    const struct ImpTabPair * pImpTabPair;  /* pointer to levels state table pair */

    /* the overall paragraph or line directionality - see UBiDiDirection */
    UBiDiDirection direction;

    /* flags is a bit set for which directional properties are in the text */
    Flags flags;

    /* lastArabicPos is index to the last AL in the text, -1 if none */
    int32_t lastArabicPos;

    /* characters after trailingWSStart are WS and are */
    /* implicitly at the paraLevel (rule (L1)) - levels may not reflect that */
    int32_t trailingWSStart;

    /* fields for paragraph handling */
    int32_t paraCount;                  /* set in getDirProps() */
    Para *paras;                        /* limits of paragraphs, filled in
                            ResolveExplicitLevels() or CheckExplicitLevels() */

    /* for single paragraph text, we only need a tiny array of paras (no malloc()) */
    Para simpleParas[1];

    /* fields for line reordering */
    int32_t runCount;     /* ==-1: runs not set up yet */
    Run *runs;

    /* for non-mixed text, we only need a tiny array of runs (no malloc()) */
    Run simpleRuns[1];

    /* for inverse Bidi with insertion of directional marks */
    InsertPoints insertPoints;

    /* for option UBIDI_OPTION_REMOVE_CONTROLS */
    int32_t controlCount;

    /* for Bidi class callback */
    UBiDiClassCallback *fnClassCallback;    /* action pointer */
    const void *coClassCallback;            /* context pointer */
};

#define IS_VALID_PARA(x) ((x) && ((x)->pParaBiDi==(x)))
#define IS_VALID_PARA_OR_LINE(x) ((x) && ((x)->pParaBiDi==(x) || (((x)->pParaBiDi) && (x)->pParaBiDi->pParaBiDi==(x)->pParaBiDi)))

typedef union {
    DirProp *dirPropsMemory;
    UBiDiLevel *levelsMemory;
    Para *parasMemory;
    Run *runsMemory;
} BidiMemoryForAllocation;

/* Macros for initial checks at function entry */
#define RETURN_IF_NULL_OR_FAILING_ERRCODE(pErrcode, retvalue)   \
        if((pErrcode)==NULL || U_FAILURE(*pErrcode)) return retvalue
#define RETURN_IF_NOT_VALID_PARA(bidi, errcode, retvalue)   \
        if(!IS_VALID_PARA(bidi)) {  \
            errcode=U_INVALID_STATE_ERROR;  \
            return retvalue;                \
        }
#define RETURN_IF_NOT_VALID_PARA_OR_LINE(bidi, errcode, retvalue)   \
        if(!IS_VALID_PARA_OR_LINE(bidi)) {  \
            errcode=U_INVALID_STATE_ERROR;  \
            return retvalue;                \
        }
#define RETURN_IF_BAD_RANGE(arg, start, limit, errcode, retvalue)   \
        if((arg)<(start) || (arg)>=(limit)) {       \
            (errcode)=U_ILLEGAL_ARGUMENT_ERROR;     \
            return retvalue;                        \
        }

#define RETURN_VOID_IF_NULL_OR_FAILING_ERRCODE(pErrcode)   \
        if((pErrcode)==NULL || U_FAILURE(*pErrcode)) return
#define RETURN_VOID_IF_NOT_VALID_PARA(bidi, errcode)   \
        if(!IS_VALID_PARA(bidi)) {  \
            errcode=U_INVALID_STATE_ERROR;  \
            return;                \
        }
#define RETURN_VOID_IF_NOT_VALID_PARA_OR_LINE(bidi, errcode)   \
        if(!IS_VALID_PARA_OR_LINE(bidi)) {  \
            errcode=U_INVALID_STATE_ERROR;  \
            return;                \
        }
#define RETURN_VOID_IF_BAD_RANGE(arg, start, limit, errcode)   \
        if((arg)<(start) || (arg)>=(limit)) {       \
            (errcode)=U_ILLEGAL_ARGUMENT_ERROR;     \
            return;                        \
        }

/* helper function to (re)allocate memory if allowed */
U_CFUNC UBool
ubidi_getMemory(BidiMemoryForAllocation *pMemory, int32_t *pSize, UBool mayAllocate, int32_t sizeNeeded);

/* helper macros for each allocated array in UBiDi */
#define getDirPropsMemory(pBiDi, length) \
        ubidi_getMemory((BidiMemoryForAllocation *)&(pBiDi)->dirPropsMemory, &(pBiDi)->dirPropsSize, \
                        (pBiDi)->mayAllocateText, (length))

#define getLevelsMemory(pBiDi, length) \
        ubidi_getMemory((BidiMemoryForAllocation *)&(pBiDi)->levelsMemory, &(pBiDi)->levelsSize, \
                        (pBiDi)->mayAllocateText, (length))

#define getRunsMemory(pBiDi, length) \
        ubidi_getMemory((BidiMemoryForAllocation *)&(pBiDi)->runsMemory, &(pBiDi)->runsSize, \
                        (pBiDi)->mayAllocateRuns, (length)*sizeof(Run))

/* additional macros used by ubidi_open() - always allow allocation */
#define getInitialDirPropsMemory(pBiDi, length) \
        ubidi_getMemory((BidiMemoryForAllocation *)&(pBiDi)->dirPropsMemory, &(pBiDi)->dirPropsSize, \
                        TRUE, (length))

#define getInitialLevelsMemory(pBiDi, length) \
        ubidi_getMemory((BidiMemoryForAllocation *)&(pBiDi)->levelsMemory, &(pBiDi)->levelsSize, \
                        TRUE, (length))

#define getInitialParasMemory(pBiDi, length) \
        ubidi_getMemory((BidiMemoryForAllocation *)&(pBiDi)->parasMemory, &(pBiDi)->parasSize, \
                        TRUE, (length)*sizeof(Para))

#define getInitialRunsMemory(pBiDi, length) \
        ubidi_getMemory((BidiMemoryForAllocation *)&(pBiDi)->runsMemory, &(pBiDi)->runsSize, \
                        TRUE, (length)*sizeof(Run))

#endif

#endif
