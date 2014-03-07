/*
*******************************************************************************
*
*   Copyright (C) 1998-2010, International Business Machines
*   Corporation and others.  All Rights Reserved.
*
*******************************************************************************
*
* Private implementation header for C collation
*   file name:  ucol_imp.h
*   encoding:   US-ASCII
*   tab size:   8 (not used)
*   indentation:4
*
*   created on: 2000dec11
*   created by: Vladimir Weinstein
*
* Modification history
* Date        Name      Comments
* 02/16/2001  synwee    Added UCOL_GETPREVCE for the use in ucoleitr
* 02/27/2001  synwee    Added getMaxExpansion data structure in UCollator
* 03/02/2001  synwee    Added UCOL_IMPLICIT_CE
* 03/12/2001  synwee    Added pointer start to collIterate.
*/

#ifndef UCOL_IMP_H
#define UCOL_IMP_H

#include "unicode/utypes.h"

#define UCA_DATA_TYPE "icu"
#define UCA_DATA_NAME "ucadata"
#define INVC_DATA_TYPE "icu"
#define INVC_DATA_NAME "invuca"

/**
 * Convenience string denoting the Collation data tree
 * @internal ICU 3.0
 */
#define U_ICUDATA_COLL U_ICUDATA_NAME U_TREE_SEPARATOR_STRING "coll"

#if !UCONFIG_NO_COLLATION

#ifdef XP_CPLUSPLUS
#include "unicode/normalizer2.h"
#include "unicode/unistr.h"
#endif
#include "unicode/ucol.h"
#include "utrie.h"
#include "cmemory.h"

/* This is the internal header file which contains important declarations for 
 * the collation framework. 
 * Ready to use collators are stored as binary images. Both UCA and tailorings 
 * share the same binary format. Individual files (currently only UCA) have a 
 * udata header in front of the image and should be opened using udata_open.
 * Tailoring images are currently stored inside resource bundles and are intialized
 * through ucol_open API.
 *
 * The following describes the formats for collation binaries
 * (UCA & tailorings) and for the inverse UCA table.
 * Substructures are described in the collation design document at
 * http://source.icu-project.org/repos/icu/icuhtml/trunk/design/collation/ICU_collation_design.htm
 *
 * -------------------------------------------------------------
 *
 * Here is the format of binary collation image.
 *
 * Physical order of structures:
 * - header (UCATableHeader)
 * - options (UColOptionSet)
 * - expansions (CE[])
 * - contractions (UChar[contractionSize] + CE[contractionSize])
 * - serialized UTrie with mappings of code points to CEs
 * - max expansion tables (CE[endExpansionCECount] + uint8_t[endExpansionCECount])
 * - two bit sets for backward processing in strcoll (identical prefixes)
 *   and for backward CE iteration (each set is uint8_t[UCOL_UNSAFECP_TABLE_SIZE])
 * - UCA constants (UCAConstants)
 * - UCA contractions (UChar[contractionUCACombosSize][contractionUCACombosWidth])
 *
 * UCATableHeader fields:
 *
 * int32_t size; - image size in bytes
 *
 * Offsets to interesting data. All offsets are in bytes.
 * to get the address add to the header address and cast properly.
 * Some offsets are zero if the corresponding structures are empty.
 *
 * Tailoring binaries that only set options and contain no mappings etc.
 * will have all offsets 0 except for the options and expansion offsets,
 * which give the position and length of the options array.
 *
 * uint32_t options; - offset to default collator options (UColOptionSet *),
 *                     a set of 32-bit values. See declaration of UColOptionSet for more details
 *
 * uint32_t UCAConsts; - only used (!=0) in UCA image - structure which holds values for indirect positioning and implicit ranges
 *                       See declaration of UCAConstants structure. This is a set of unsigned 32-bit values used to store 
 *                       important constant values that are defined in the UCA and used for building and runtime.
 *
 * uint32_t contractionUCACombos; - only used (!=0) in UCA image - list of UCA contractions. This is a zero terminated array of UChar[contractionUCACombosWidth],
 *                                  containing contractions from the UCA. These are needed in the build process to copy UCA contractions
 *                                  in case the base contraction symbol is tailored.
 *
 * uint32_t magic; - must contain UCOL_HEADER_MAGIC (formatVersion 2.3)
 *
 * uint32_t mappingPosition;  - offset to UTrie (const uint8_t *mappingPosition). This is a serialized UTrie and should be treated as such. 
 *                              Used as a primary lookup table for collation elements.
 *
 * uint32_t expansion;  - offset to expansion table (uint32_t *expansion). This is an array of expansion CEs. Never 0.
 *
 * uint32_t contractionIndex; - offset to contraction table (UChar *contractionIndex). Used to look up contraction sequences. Contents
 *                              are aligned with the contents of contractionCEs table. 0 if no contractions.
 *
 * uint32_t contractionCEs;  - offset to resulting contraction CEs (uint32_t *contractionCEs). When a contraction is resolved in the
 *                             in the contractionIndex table, the resulting index is used to look up corresponding CE in this table. 
 *                             0 if no contractions.
 * uint32_t contractionSize; - size of contraction table in elements (both Index and CEs). 
 *
 * Tables described below are used for Boyer-Moore searching algorithm - they define the size of longest expansion
 * and last CEs in expansions.
 * uint32_t endExpansionCE; - offset to array of last collation element in expansion (uint32_t *).
 *                            Never 0.
 * uint32_t expansionCESize; - array of maximum expansion sizes (uint8_t *)
 * int32_t  endExpansionCECount; - size of endExpansionCE. See UCOL_GETMAXEXPANSION
 *                                 for the usage model
 *
 * These two offsets point to byte tables that are used in the backup heuristics.
 * uint32_t unsafeCP; - hash table of unsafe code points (uint8_t *). See ucol_unsafeCP function.
 * uint32_t contrEndCP; - hash table of final code points in contractions (uint8_t *). See ucol_contractionEndCP.              
 *
 * int32_t contractionUCACombosSize; - number of UChar[contractionUCACombosWidth] in contractionUCACombos
 *                                     (formatVersion 2.3)
 * UBool jamoSpecial; - Jamo special indicator (uint8_t). If TRUE, Jamos are special, so we cannot use simple Hangul decomposition.
 * UBool isBigEndian; - endianness of this collation binary (formatVersion 2.3)
 * uint8_t charSetFamily; - charset family of this collation binary (formatVersion 2.3)
 * uint8_t contractionUCACombosWidth; - number of UChars per UCA contraction in contractionUCACombos (formatVersion 2.3)
 *
 * Various version fields
 * UVersionInfo version; - version 4 uint8_t
 * UVersionInfo UCAVersion;  - version 4 uint8_t
 * UVersionInfo UCDVersion;  - version 4 uint8_t
 * UVersionInfo formatVersion; - version of the format of the collation binary
 *                               same formatVersion as in ucadata.icu's UDataInfo header
 *                               (formatVersion 2.3)
 *
 * uint32_t offset to the reordering code to lead CE byte remapping table
 * uint32_t offset to the lead CE byte to reordering code mapping table
 *
 * uint8_t reserved[76];  - currently unused
 *
 * -------------------------------------------------------------
 *
 * Inverse UCA is used for constructing collators from rules. It is always an individual file
 * and always has a UDataInfo header. 
 * here is the structure:
 * 
 * uint32_t byteSize; - size of inverse UCA image in bytes
 * uint32_t tableSize; - length of inverse table (number of uint32_t[3] rows)
 * uint32_t contsSize; - size of continuation table (number of UChars in table)
 *
 * uint32_t table; - offset to inverse table (uint32_t *)
 *                   Inverse table contains of rows of 3 uint32_t values. First two values are CE and a possible continuation
 *                   the third value is either a code unit (if there is only one code unit for element) or an index to continuation 
 *                   (number of code units combined with an index).
 *                   table. If more than one codepoint have the same CE, continuation table contains code units separated by FFFF and final
 *                   code unit sequence for a CE is terminated by FFFE.
 * uint32_t conts; - offset to continuation table (uint16_t *). Contains code units that transform to a same CE.
 *
 * UVersionInfo UCAVersion; -  version of the UCA, read from file 4 uint8_t
 * uint8_t padding[8]; - padding 8 uint8_t
 * Header is followed by the table and continuation table.
*/

/* let us know whether reserved fields are reset to zero or junked */
#define UCOL_HEADER_MAGIC 0x20030618

/* UDataInfo for UCA mapping table */
/* dataFormat="UCol"            */
#define UCA_DATA_FORMAT_0 ((uint8_t)0x55)
#define UCA_DATA_FORMAT_1 ((uint8_t)0x43)
#define UCA_DATA_FORMAT_2 ((uint8_t)0x6f)
#define UCA_DATA_FORMAT_3 ((uint8_t)0x6c)

#define UCA_FORMAT_VERSION_0 ((uint8_t)3)
#define UCA_FORMAT_VERSION_1 ((uint8_t)0)
#define UCA_FORMAT_VERSION_2 ((uint8_t)0)
#define UCA_FORMAT_VERSION_3 ((uint8_t)0)

/* UDataInfo for inverse UCA table */
/* dataFormat="InvC"            */
#define INVUCA_DATA_FORMAT_0 ((uint8_t)0x49)
#define INVUCA_DATA_FORMAT_1 ((uint8_t)0x6E)
#define INVUCA_DATA_FORMAT_2 ((uint8_t)0x76)
#define INVUCA_DATA_FORMAT_3 ((uint8_t)0x43)

#define INVUCA_FORMAT_VERSION_0 ((uint8_t)2)
#define INVUCA_FORMAT_VERSION_1 ((uint8_t)1)
#define INVUCA_FORMAT_VERSION_2 ((uint8_t)0)
#define INVUCA_FORMAT_VERSION_3 ((uint8_t)0)

/* This is the size of the stack allocated buffer for sortkey generation and similar operations */
/* if it is too small, heap allocation will occur.*/
/* you can change this value if you need memory - it will affect the performance, though, since we're going to malloc */
#define UCOL_MAX_BUFFER 128
#define UCOL_PRIMARY_MAX_BUFFER 8*UCOL_MAX_BUFFER
#define UCOL_SECONDARY_MAX_BUFFER UCOL_MAX_BUFFER
#define UCOL_TERTIARY_MAX_BUFFER UCOL_MAX_BUFFER
#define UCOL_CASE_MAX_BUFFER UCOL_MAX_BUFFER/4
#define UCOL_QUAD_MAX_BUFFER 2*UCOL_MAX_BUFFER

#define UCOL_NORMALIZATION_GROWTH 2
#define UCOL_NORMALIZATION_MAX_BUFFER UCOL_MAX_BUFFER*UCOL_NORMALIZATION_GROWTH

/* This writable buffer is used if we encounter Thai and need to reorder the string on the fly */
/* Sometimes we already have a writable buffer (like in case of normalized strings). */
/*
you can change this value to any value >= 4 if you need memory -
it will affect the performance, though, since we're going to malloc.
Note 3 is the minimum value for Thai collation and 4 is the
minimum number for special Jamo
*/
#define UCOL_WRITABLE_BUFFER_SIZE 256

/* This is the size of the buffer for expansion CE's */
/* In reality we should not have to deal with expm sequences longer then 16 */
/* you can change this value if you need memory */
/* WARNING THIS BUFFER DOES HAVE MALLOC FALLBACK. If you make it too small, you'll get into performance trouble */
/* Reasonable small value is around 10, if you don't do Arabic or other funky collations that have long expansion sequence */
/* This is the longest expansion sequence we can handle without bombing out */
#define UCOL_EXPAND_CE_BUFFER_SIZE 64

/* This is the size to increase the buffer for expansion CE's */
#define UCOL_EXPAND_CE_BUFFER_EXTEND_SIZE 64


/* Unsafe UChar hash table table size.                                           */
/*  size is 32 bytes for 1 bit for each latin 1 char + some power of two for     */
/*  hashing the rest of the chars.   Size in bytes                               */
#define UCOL_UNSAFECP_TABLE_SIZE 1056
                                    /* mask value down to "some power of two"-1  */
                                    /*  number of bits, not num of bytes.        */
#define UCOL_UNSAFECP_TABLE_MASK 0x1fff


/* flags bits for collIterate.flags       */
/*                                        */
/*  NORM - set for incremental normalize of source string */
#define UCOL_ITER_NORM  1

#define UCOL_ITER_HASLEN 2

                              /* UCOL_ITER_INNORMBUF - set if the "pos" is in          */
                              /*               the writable side buffer, handling      */
                              /*               incrementally normalized characters.    */
#define UCOL_ITER_INNORMBUF 4

                              /* UCOL_ITER_ALLOCATED - set if this iterator has        */
                              /*    malloced storage to expand a buffer.               */
#define UCOL_ITER_ALLOCATED 8
                              /* UCOL_HIRAGANA_Q - note if the codepoint was hiragana  */
#define UCOL_HIRAGANA_Q     16
                              /* UCOL_WAS_HIRAGANA - set to TRUE if there was a Hiragana */
                              /* otherwise set to false                                  */
#define UCOL_WAS_HIRAGANA   32 
                              /* UCOL_USE_ITERATOR - set this if collIterate uses a */
                              /* character iterator instead of simply accessing string */
                              /* by index */
#define UCOL_USE_ITERATOR   64

#define UCOL_FORCE_HAN_IMPLICIT 128

#define NFC_ZERO_CC_BLOCK_LIMIT_  0x300

#ifdef XP_CPLUSPLUS

U_NAMESPACE_BEGIN

typedef struct collIterate : public UMemory {
  const UChar *string; /* Original string */
  /* UChar *start;  Pointer to the start of the source string. Either points to string
                    or to writableBuffer */
  const UChar *endp; /* string end ptr.  Is undefined for null terminated strings */
  const UChar *pos; /* This is position in the string.  Can be to original or writable buf */

  uint32_t *toReturn; /* This is the CE from CEs buffer that should be returned */
  uint32_t *CEpos; /* This is the position to which we have stored processed CEs */

  int32_t *offsetReturn; /* This is the offset to return, if non-NULL */
  int32_t *offsetStore;  /* This is the pointer for storing offsets */
  int32_t offsetRepeatCount;  /* Repeat stored offset if non-zero */
  int32_t offsetRepeatValue;  /* offset value to repeat */

  UnicodeString writableBuffer;
  const UChar *fcdPosition; /* Position in the original string to continue FCD check from. */
  const UCollator *coll;
  const Normalizer2 *nfd;
  uint8_t   flags;
  uint8_t   origFlags;
  uint32_t *extendCEs; /* This is use if CEs is not big enough */
  int32_t extendCEsSize; /* Holds the size of the dynamic CEs buffer */
  uint32_t CEs[UCOL_EXPAND_CE_BUFFER_SIZE]; /* This is where we store CEs */

  int32_t *offsetBuffer;    /* A dynamic buffer to hold offsets */
  int32_t offsetBufferSize; /* The size of the offset buffer */

  UCharIterator *iterator;
  /*int32_t iteratorIndex;*/

  // The offsetBuffer should probably be a UVector32, but helper functions
  // are an improvement over duplicated code.
  void appendOffset(int32_t offset, UErrorCode &errorCode);
} collIterate;

U_NAMESPACE_END

#else

typedef struct collIterate collIterate;

#endif

#define paddedsize(something) ((something)+((((something)%4)!=0)?(4-(something)%4):0))
#define headersize (paddedsize(sizeof(UCATableHeader))+paddedsize(sizeof(UColOptionSet)))

/* 
struct used internally in getSpecial*CE.
data similar to collIterate.
*/
struct collIterateState {
    const UChar *pos; /* This is position in the string.  Can be to original or writable buf */
    const UChar *returnPos;
    const UChar *fcdPosition; /* Position in the original string to continue FCD check from. */
    const UChar *bufferaddress; /* address of the normalization buffer */
    int32_t  buffersize;
    uint8_t   flags;
    uint8_t   origFlags;
    uint32_t   iteratorIndex;
    int32_t    iteratorMove;
};

U_CAPI void U_EXPORT2
uprv_init_collIterate(const UCollator *collator,
                      const UChar *sourceString, int32_t sourceLen,
                      U_NAMESPACE_QUALIFIER collIterate *s, UErrorCode *status);

/* Internal functions for C test code. */
U_CAPI U_NAMESPACE_QUALIFIER collIterate * U_EXPORT2
uprv_new_collIterate(UErrorCode *status);

U_CAPI void U_EXPORT2
uprv_delete_collIterate(U_NAMESPACE_QUALIFIER collIterate *s);

/* @return s->pos == s->endp */
U_CAPI UBool U_EXPORT2
uprv_collIterateAtEnd(U_NAMESPACE_QUALIFIER collIterate *s);

#ifdef XP_CPLUSPLUS

U_NAMESPACE_BEGIN

struct UCollationPCE;
typedef struct UCollationPCE UCollationPCE;

U_NAMESPACE_END

struct UCollationElements : public U_NAMESPACE_QUALIFIER UMemory
{
  /**
  * Struct wrapper for source data
  */
        U_NAMESPACE_QUALIFIER collIterate iteratordata_;
  /**
  * Indicates if this data has been reset.
  */
        UBool              reset_;
  /**
  * Indicates if the data should be deleted.
  */
        UBool              isWritable;

/**
 * Data for getNextProcessed, getPreviousProcessed.
 */
        U_NAMESPACE_QUALIFIER UCollationPCE     *pce;
};

#else
/*opaque type*/
struct UCollationElements;
#endif

U_CAPI void U_EXPORT2
uprv_init_pce(const struct UCollationElements *elems);

#define UCOL_LEVELTERMINATOR 1

/* mask off anything but primary order */
#define UCOL_PRIMARYORDERMASK 0xffff0000
/* mask off anything but secondary order */
#define UCOL_SECONDARYORDERMASK 0x0000ff00
/* mask off anything but tertiary order */
#define UCOL_TERTIARYORDERMASK 0x000000ff
/* primary order shift */
#define UCOL_PRIMARYORDERSHIFT 16
/* secondary order shift */
#define UCOL_SECONDARYORDERSHIFT 8

#define UCOL_BYTE_SIZE_MASK 0xFF

#define UCOL_CASE_BYTE_START 0x80
#define UCOL_CASE_SHIFT_START 7

#define UCOL_IGNORABLE 0

/* get weights from a CE */
#define UCOL_PRIMARYORDER(order) (((order) & UCOL_PRIMARYORDERMASK)>> UCOL_PRIMARYORDERSHIFT)
#define UCOL_SECONDARYORDER(order) (((order) & UCOL_SECONDARYORDERMASK)>> UCOL_SECONDARYORDERSHIFT)
#define UCOL_TERTIARYORDER(order) ((order) & UCOL_TERTIARYORDERMASK)

/**
 * Determine if a character is a Thai vowel (which sorts after
 * its base consonant).
 */
#define UCOL_ISTHAIPREVOWEL(ch) ((((uint32_t)(ch) - 0xe40) <= (0xe44 - 0xe40)) || \
                                 (((uint32_t)(ch) - 0xec0) <= (0xec4 - 0xec0)))

/**
 * Determine if a character is a Thai base consonant
 */
#define UCOL_ISTHAIBASECONSONANT(ch) ((uint32_t)(ch) - 0xe01) <= (0xe2e - 0xe01)

#define UCOL_ISJAMO(ch) ((((uint32_t)(ch) - 0x1100) <= (0x1112 - 0x1100)) || \
                         (((uint32_t)(ch) - 0x1161) <= (0x1175 - 0x1161)) || \
                         (((uint32_t)(ch) - 0x11A8) <= (0x11C2 - 0x11A8)))

/* Han character ranges */
#define UCOL_FIRST_HAN 0x4E00
#define UCOL_LAST_HAN  0x9FFF
#define UCOL_FIRST_HAN_A 0x3400
#define UCOL_LAST_HAN_A  0x4DBF
#define UCOL_FIRST_HAN_COMPAT 0xFAE0
#define UCOL_LAST_HAN_COMPAT  0xFA2F

/* Han extension B is in plane 2 */
#define UCOL_FIRST_HAN_B       0x20000
#define UCOL_LAST_HAN_B        0x2A6DF

/* Hangul range */
#define UCOL_FIRST_HANGUL 0xAC00
#define UCOL_LAST_HANGUL  0xD7AF

/* Jamo ranges */
#define UCOL_FIRST_L_JAMO 0x1100
#define UCOL_FIRST_V_JAMO 0x1161
#define UCOL_FIRST_T_JAMO 0x11A8
#define UCOL_LAST_T_JAMO  0x11F9


#if 0
/* initializes collIterate structure */
/* made as macro to speed up things */
#define init_collIterate(collator, sourceString, sourceLen, s) { \
    (s)->start = (s)->string = (s)->pos = (UChar *)(sourceString); \
    (s)->endp  = (sourceLen) == -1 ? NULL :(UChar *)(sourceString)+(sourceLen); \
    (s)->CEpos = (s)->toReturn = (s)->CEs; \
    (s)->isThai = TRUE; \
    (s)->writableBuffer = (s)->stackWritableBuffer; \
    (s)->writableBufSize = UCOL_WRITABLE_BUFFER_SIZE; \
    (s)->coll = (collator); \
    (s)->fcdPosition = 0;   \
    (s)->flags = 0; \
    if(((collator)->normalizationMode == UCOL_ON)) (s)->flags |= UCOL_ITER_NORM; \
}
#endif



/*
* Macro to get the maximum size of an expansion ending with the argument ce.
* Used in the Boyer Moore algorithm.
* Note for tailoring, the UCA maxexpansion table has been merged.
* Hence we only have to search the tailored collator only.
* @param coll const UCollator pointer
* @param order last collation element of the expansion sequence
* @param result size of the longest expansion with argument collation element
*        as the last element
*/
#define UCOL_GETMAXEXPANSION(coll, order, result) {                          \
  const uint32_t *start;                                                     \
  const uint32_t *limit;                                                     \
  const uint32_t *mid;                                                       \
  start = (coll)->endExpansionCE;                                            \
  limit = (coll)->lastEndExpansionCE;                                        \
  while (start < limit - 1) {                                                \
    mid = start + ((limit - start) >> 1);                                    \
    if ((order) <= *mid) {                                                   \
      limit = mid;                                                           \
    }                                                                        \
    else {                                                                   \
      start = mid;                                                           \
    }                                                                        \
  }                                                                          \
  if (*start == order) {                                                     \
    result = *((coll)->expansionCESize + (start - (coll)->endExpansionCE));  \
  }                                                                          \
  else if (*limit == order) {                                                \
         result = *(coll->expansionCESize + (limit - coll->endExpansionCE)); \
       }                                                                     \
       else if ((order & 0xFFFF) == 0x00C0) {                                \
              result = 2;                                                    \
            }                                                                \
            else {                                                           \
              result = 1;                                                    \
            }                                                                \
}

U_CFUNC
uint32_t ucol_prv_getSpecialCE(const UCollator *coll, UChar ch, uint32_t CE,
                               U_NAMESPACE_QUALIFIER collIterate *source, UErrorCode *status);

U_CFUNC
uint32_t ucol_prv_getSpecialPrevCE(const UCollator *coll, UChar ch, uint32_t CE,
                                   U_NAMESPACE_QUALIFIER collIterate *source, UErrorCode *status);
U_CAPI uint32_t U_EXPORT2 ucol_getNextCE(const UCollator *coll,
                                         U_NAMESPACE_QUALIFIER collIterate *collationSource, UErrorCode *status);
U_CFUNC uint32_t U_EXPORT2 ucol_getPrevCE(const UCollator *coll,
                                          U_NAMESPACE_QUALIFIER collIterate *collationSource,
                                          UErrorCode *status);
/* function used by C++ getCollationKey to prevent restarting the calculation */
U_CFUNC int32_t
ucol_getSortKeyWithAllocation(const UCollator *coll,
                              const UChar *source, int32_t sourceLength,
                              uint8_t **pResult,
                              UErrorCode *pErrorCode);

/* get some memory */
void *ucol_getABuffer(const UCollator *coll, uint32_t size);

/* worker function for generating sortkeys */
U_CFUNC
int32_t U_CALLCONV
ucol_calcSortKey(const    UCollator    *coll,
        const    UChar        *source,
        int32_t        sourceLength,
        uint8_t        **result,
        uint32_t        resultLength,
        UBool allocatePrimary,
        UErrorCode *status);

U_CFUNC
int32_t U_CALLCONV
ucol_calcSortKeySimpleTertiary(const    UCollator    *coll,
        const    UChar        *source,
        int32_t        sourceLength,
        uint8_t        **result,
        uint32_t        resultLength,
        UBool allocatePrimary,
        UErrorCode *status);

U_CFUNC
int32_t 
ucol_getSortKeySize(const UCollator *coll, U_NAMESPACE_QUALIFIER collIterate *s, 
                    int32_t currentSize, UColAttributeValue strength, 
                    int32_t len);
/**
 * Makes a copy of the Collator's rule data. The format is
 * that of .col files.
 *
 * @param length returns the length of the data, in bytes.
 * @param status the error status
 * @return memory, owned by the caller, of size 'length' bytes.
 * @internal INTERNAL USE ONLY
 */
U_CFUNC uint8_t* U_EXPORT2 
ucol_cloneRuleData(const UCollator *coll, int32_t *length, UErrorCode *status);

/**
 * Used to set requested and valid locales on a collator returned by the collator
 * service.
 */
U_CFUNC void U_EXPORT2
ucol_setReqValidLocales(UCollator *coll, char *requestedLocaleToAdopt, char *validLocaleToAdopt, char *actualLocaleToAdopt);

#define UCOL_SPECIAL_FLAG 0xF0000000
#define UCOL_TAG_SHIFT 24
#define UCOL_TAG_MASK 0x0F000000
#define INIT_EXP_TABLE_SIZE 1024
#define UCOL_NOT_FOUND 0xF0000000
#define UCOL_EXPANSION 0xF1000000
#define UCOL_CONTRACTION 0xF2000000
#define UCOL_THAI 0xF3000000
#define UCOL_UNMARKED 0x03
#define UCOL_NEW_TERTIARYORDERMASK 0x0000003f

/* Bit mask for primary collation strength. */
#define UCOL_PRIMARYMASK    0xFFFF0000

/* Bit mask for secondary collation strength. */
#define UCOL_SECONDARYMASK  0x0000FF00

/* Bit mask for tertiary collation strength. */
#define UCOL_TERTIARYMASK   0x000000FF

/**
 * Internal.
 * This indicates the last element in a UCollationElements has been consumed.
 * Compare with the UCOL_NULLORDER, UCOL_NULLORDER is returned if error occurs.
 */
#define UCOL_NO_MORE_CES            0x00010101
#define UCOL_NO_MORE_CES_PRIMARY    0x00010000
#define UCOL_NO_MORE_CES_SECONDARY  0x00000100
#define UCOL_NO_MORE_CES_TERTIARY   0x00000001

#define isSpecial(CE) ((((CE)&UCOL_SPECIAL_FLAG)>>28)==0xF)

#define UCOL_UPPER_CASE 0x80
#define UCOL_MIXED_CASE 0x40
#define UCOL_LOWER_CASE 0x00

#define UCOL_CONTINUATION_MARKER 0xC0
#define UCOL_REMOVE_CONTINUATION 0xFFFFFF3F

#define isContinuation(CE) (((CE) & UCOL_CONTINUATION_MARKER) == UCOL_CONTINUATION_MARKER)
#define isFlagged(CE) (((CE) & 0x80) == 0x80)
#define isLongPrimary(CE) (((CE) & 0xC0) == 0xC0)

#define getCETag(CE) (((CE)&UCOL_TAG_MASK)>>UCOL_TAG_SHIFT)
#define isContraction(CE) (isSpecial((CE)) && (getCETag((CE)) == CONTRACTION_TAG))
#define isPrefix(CE) (isSpecial((CE)) && (getCETag((CE)) == SPEC_PROC_TAG))
#define constructContractCE(tag, CE) (UCOL_SPECIAL_FLAG | ((tag)<<UCOL_TAG_SHIFT) | ((CE)&0xFFFFFF))
#define constructSpecProcCE(CE) (UCOL_SPECIAL_FLAG | (SPEC_PROC_TAG<<UCOL_TAG_SHIFT) | ((CE)&0xFFFFFF))
#define getContractOffset(CE) ((CE)&0xFFFFFF)
#define getExpansionOffset(CE) (((CE)&0x00FFFFF0)>>4)
#define getExpansionCount(CE) ((CE)&0xF)
#define isCEIgnorable(CE) (((CE) & 0xFFFFFFBF) == 0)

/* StringSearch internal use */
#define inNormBuf(coleiter) ((coleiter)->iteratordata_.flags & UCOL_ITER_INNORMBUF)
#define isFCDPointerNull(coleiter) ((coleiter)->iteratordata_.fcdPosition == NULL)
#define hasExpansion(coleiter) ((coleiter)->iteratordata_.CEpos != (coleiter)->iteratordata_.CEs)
#define getExpansionPrefix(coleiter) ((coleiter)->iteratordata_.toReturn - (coleiter)->iteratordata_.CEs)
#define setExpansionPrefix(coleiter, offset) ((coleiter)->iteratordata_.CEs + offset)
#define getExpansionSuffix(coleiter) ((coleiter)->iteratordata_.CEpos - (coleiter)->iteratordata_.toReturn)
#define setExpansionSuffix(coleiter, offset) ((coleiter)->iteratordata_.toReturn = (coleiter)->iteratordata_.CEpos - leftoverces)

/* This is an enum that lists magic special byte values from the fractional UCA.
 * See also http://site.icu-project.org/design/collation/bytes */
/* TODO: all the #defines that refer to special byte values from the UCA should be changed to point here */

enum {
    UCOL_BYTE_ZERO = 0x00,
    UCOL_BYTE_LEVEL_SEPARATOR = 0x01,
    UCOL_BYTE_SORTKEY_GLUE = 0x02,
    UCOL_BYTE_SHIFT_PREFIX = 0x03,
    UCOL_BYTE_UNSHIFTED_MIN = UCOL_BYTE_SHIFT_PREFIX,
    UCOL_BYTE_FIRST_TAILORED = 0x04,
    UCOL_BYTE_COMMON = 0x05,
    UCOL_BYTE_FIRST_UCA = UCOL_BYTE_COMMON,
    /* TODO: Make the following values dynamic since they change with almost every UCA version. */
    UCOL_CODAN_PLACEHOLDER = 0x12,
    UCOL_BYTE_FIRST_NON_LATIN_PRIMARY = 0x5B,
    UCOL_BYTE_UNSHIFTED_MAX = 0xFF
}; 

#if 0
#define UCOL_RESET_TOP_VALUE                0x9F000303
#define UCOL_FIRST_PRIMARY_IGNORABLE        0x00008705
#define UCOL_LAST_PRIMARY_IGNORABLE         0x0000DD05
#define UCOL_LAST_PRIMARY_IGNORABLE_CONT    0x000051C0
#define UCOL_FIRST_SECONDARY_IGNORABLE      0x00000000
#define UCOL_LAST_SECONDARY_IGNORABLE       0x00000500
#define UCOL_FIRST_TERTIARY_IGNORABLE       0x00000000
#define UCOL_LAST_TERTIARY_IGNORABLE        0x00000000
#define UCOL_FIRST_VARIABLE                 0x05070505
#define UCOL_LAST_VARIABLE                  0x179B0505
#define UCOL_FIRST_NON_VARIABLE             0x1A200505
#define UCOL_LAST_NON_VARIABLE              0x7B41058F

#define UCOL_NEXT_TOP_VALUE                 0xE8960303
#define UCOL_NEXT_FIRST_PRIMARY_IGNORABLE   0x00008905
#define UCOL_NEXT_LAST_PRIMARY_IGNORABLE    0x03000303
#define UCOL_NEXT_FIRST_SECONDARY_IGNORABLE 0x00008705
#define UCOL_NEXT_LAST_SECONDARY_IGNORABLE  0x00000500
#define UCOL_NEXT_FIRST_TERTIARY_IGNORABLE  0x00000000
#define UCOL_NEXT_LAST_TERTIARY_IGNORABLE   0x00000000
#define UCOL_NEXT_FIRST_VARIABLE            0x05090505
#define UCOL_NEXT_LAST_VARIABLE             0x1A200505

#define PRIMARY_IMPLICIT_MIN 0xE8000000
#define PRIMARY_IMPLICIT_MAX 0xF0000000
#endif

/* These constants can be changed - sortkey size is affected by them */
#define UCOL_PROPORTION2 0.5
#define UCOL_PROPORTION3 0.667

/* These values come from the UCA */
#define UCOL_COMMON_BOT2 UCOL_BYTE_COMMON
#define UCOL_COMMON_TOP2 0x86u
#define UCOL_TOTAL2 (UCOL_COMMON_TOP2-UCOL_COMMON_BOT2-1) 

#define UCOL_FLAG_BIT_MASK_CASE_SW_OFF 0x80
#define UCOL_FLAG_BIT_MASK_CASE_SW_ON 0x40
#define UCOL_COMMON_TOP3_CASE_SW_OFF 0x85
#define UCOL_COMMON_TOP3_CASE_SW_LOWER 0x45
#define UCOL_COMMON_TOP3_CASE_SW_UPPER 0xC5

/* These values come from the UCA */
#define UCOL_COMMON_BOT3 0x05

#define UCOL_COMMON_BOTTOM3_CASE_SW_UPPER 0x86;
#define UCOL_COMMON_BOTTOM3_CASE_SW_LOWER UCOL_COMMON_BOT3;

#define UCOL_TOP_COUNT2  (UCOL_PROPORTION2*UCOL_TOTAL2)
#define UCOL_BOT_COUNT2  (UCOL_TOTAL2-UCOL_TOP_COUNT2)


#define UCOL_COMMON2 UCOL_COMMON_BOT2
#define UCOL_COMMON3_UPPERFIRST 0xC5
#define UCOL_COMMON3_NORMAL UCOL_COMMON_BOT3

#define UCOL_COMMON4 0xFF

/* constants for case level/case first handling */
/* used to instantiate UCollators fields in ucol_updateInternalState */
#define UCOL_CASE_SWITCH      0xC0
#define UCOL_NO_CASE_SWITCH   0x00

#define UCOL_REMOVE_CASE      0x3F
#define UCOL_KEEP_CASE        0xFF

#define UCOL_CASE_BIT_MASK    0xC0

#define UCOL_TERT_CASE_MASK   0xFF

#define UCOL_ENDOFLATINONERANGE 0xFF
#define UCOL_LATINONETABLELEN   (UCOL_ENDOFLATINONERANGE+50)
#define UCOL_BAIL_OUT_CE      0xFF000000


typedef enum {
    NOT_FOUND_TAG = 0,
    EXPANSION_TAG = 1,       /* This code point results in an expansion */
    CONTRACTION_TAG = 2,     /* Start of a contraction */
    THAI_TAG = 3,            /* Thai character - do the reordering */
    CHARSET_TAG = 4,         /* Charset processing, not yet implemented */
    SURROGATE_TAG = 5,       /* Lead surrogate that is tailored and doesn't start a contraction */
    HANGUL_SYLLABLE_TAG = 6, /* AC00-D7AF*/
    LEAD_SURROGATE_TAG = 7,  /* D800-DBFF*/
    TRAIL_SURROGATE_TAG = 8,     /* DC00-DFFF*/
    CJK_IMPLICIT_TAG = 9,    /* 0x3400-0x4DB5, 0x4E00-0x9FA5, 0xF900-0xFA2D*/
    IMPLICIT_TAG = 10,
    SPEC_PROC_TAG = 11,
    /* ICU 2.1 */
    LONG_PRIMARY_TAG = 12,   /* This is a three byte primary with starting secondaries and tertiaries */
                             /* It fits in a single 32 bit CE and is used instead of expansion to save */
                             /* space without affecting the performance (hopefully) */
                             
    DIGIT_TAG = 13,          /* COllate Digits As Numbers (CODAN) implementation */
    
    CE_TAGS_COUNT
} UColCETags;

/*
 *****************************************************************************************
 * set to zero
 * NON_CHARACTER FDD0 - FDEF, FFFE, FFFF, 1FFFE, 1FFFF, 2FFFE, 2FFFF,...e.g. **FFFE, **FFFF
 ******************************************************************************************
 */

typedef struct {
      uint32_t variableTopValue;
      /*UColAttributeValue*/ int32_t frenchCollation;
      /*UColAttributeValue*/ int32_t alternateHandling; /* attribute for handling variable elements*/
      /*UColAttributeValue*/ int32_t caseFirst;         /* who goes first, lower case or uppercase */
      /*UColAttributeValue*/ int32_t caseLevel;         /* do we have an extra case level */
      /*UColAttributeValue*/ int32_t normalizationMode; /* attribute for normalization */
      /*UColAttributeValue*/ int32_t strength;          /* attribute for strength */
      /*UColAttributeValue*/ int32_t hiraganaQ;         /* attribute for special Hiragana */
      /*UColAttributeValue*/ int32_t numericCollation;  /* attribute for numeric collation */
      uint32_t reserved[15];                 /* for future use */
} UColOptionSet;

typedef struct {
  uint32_t UCA_FIRST_TERTIARY_IGNORABLE[2];       /*0x00000000*/
  uint32_t UCA_LAST_TERTIARY_IGNORABLE[2];        /*0x00000000*/
  uint32_t UCA_FIRST_PRIMARY_IGNORABLE[2];        /*0x00008705*/
  uint32_t UCA_FIRST_SECONDARY_IGNORABLE[2];      /*0x00000000*/
  uint32_t UCA_LAST_SECONDARY_IGNORABLE[2];       /*0x00000500*/
  uint32_t UCA_LAST_PRIMARY_IGNORABLE[2];         /*0x0000DD05*/
  uint32_t UCA_FIRST_VARIABLE[2];                 /*0x05070505*/
  uint32_t UCA_LAST_VARIABLE[2];                  /*0x13CF0505*/
  uint32_t UCA_FIRST_NON_VARIABLE[2];             /*0x16200505*/
  uint32_t UCA_LAST_NON_VARIABLE[2];              /*0x767C0505*/
  uint32_t UCA_RESET_TOP_VALUE[2];                /*0x9F000303*/
  uint32_t UCA_FIRST_IMPLICIT[2];
  uint32_t UCA_LAST_IMPLICIT[2]; 
  uint32_t UCA_FIRST_TRAILING[2];
  uint32_t UCA_LAST_TRAILING[2]; 

#if 0
  uint32_t UCA_NEXT_TOP_VALUE[2];                 /*0xE8960303*/
  uint32_t UCA_NEXT_FIRST_PRIMARY_IGNORABLE;   /*0x00008905*/
  uint32_t UCA_NEXT_LAST_PRIMARY_IGNORABLE;    /*0x03000303*/
  uint32_t UCA_NEXT_FIRST_SECONDARY_IGNORABLE; /*0x00008705*/
  uint32_t UCA_NEXT_LAST_SECONDARY_IGNORABLE;  /*0x00000500*/
  uint32_t UCA_NEXT_FIRST_TERTIARY_IGNORABLE;  /*0x00000000*/
  uint32_t UCA_NEXT_LAST_TERTIARY_IGNORABLE;   /*0x00000000*/
  uint32_t UCA_NEXT_FIRST_VARIABLE;            /*0x05090505*/
  uint32_t UCA_NEXT_LAST_VARIABLE;             /*0x16200505*/
#endif

  uint32_t UCA_PRIMARY_TOP_MIN;
  uint32_t UCA_PRIMARY_IMPLICIT_MIN; /*0xE8000000*/
  uint32_t UCA_PRIMARY_IMPLICIT_MAX; /*0xF0000000*/
  uint32_t UCA_PRIMARY_TRAILING_MIN; /*0xE8000000*/
  uint32_t UCA_PRIMARY_TRAILING_MAX; /*0xF0000000*/
  uint32_t UCA_PRIMARY_SPECIAL_MIN; /*0xE8000000*/
  uint32_t UCA_PRIMARY_SPECIAL_MAX; /*0xF0000000*/
} UCAConstants;

typedef struct {
      int32_t size;
      /* all the offsets are in bytes */
      /* to get the address add to the header address and cast properly */
      uint32_t options; /* these are the default options for the collator */
      uint32_t UCAConsts; /* structure which holds values for indirect positioning and implicit ranges */
      uint32_t contractionUCACombos;        /* this one is needed only for UCA, to copy the appropriate contractions */
      uint32_t magic;            /* magic number - lets us know whether reserved data is reset or junked */
      uint32_t mappingPosition;  /* const uint8_t *mappingPosition; */
      uint32_t expansion;        /* uint32_t *expansion;            */
      uint32_t contractionIndex; /* UChar *contractionIndex;        */
      uint32_t contractionCEs;   /* uint32_t *contractionCEs;       */
      uint32_t contractionSize;  /* needed for various closures */
      /*int32_t latinOneMapping;*/ /* this is now handled in the trie itself *//* fast track to latin1 chars      */

      uint32_t endExpansionCE;      /* array of last collation element in
                                       expansion */
      uint32_t expansionCESize;     /* array of maximum expansion size
                                       corresponding to the expansion
                                       collation elements with last element
                                       in endExpansionCE*/
      int32_t  endExpansionCECount; /* size of endExpansionCE */
      uint32_t unsafeCP;            /* hash table of unsafe code points */
      uint32_t contrEndCP;          /* hash table of final code points  */
                                    /*   in contractions.               */

      int32_t contractionUCACombosSize;     /* number of UCA contraction items. */
                                            /*Length is contractionUCACombosSize*contractionUCACombosWidth*sizeof(UChar) */
      UBool jamoSpecial;                    /* is jamoSpecial */
      UBool isBigEndian;                    /* is this data big endian? from the UDataInfo header*/
      uint8_t charSetFamily;                /* what is the charset family of this data from the UDataInfo header*/
      uint8_t contractionUCACombosWidth;    /* width of UCA combos field */
      UVersionInfo version;
      UVersionInfo UCAVersion;              /* version of the UCA, read from file */
      UVersionInfo UCDVersion;              /* UCD version, obtained by u_getUnicodeVersion */
      UVersionInfo formatVersion;           /* format version from the UDataInfo header */
      uint32_t scriptToLeadByte;            /* offset to script to lead collation byte mapping data */
      uint32_t leadByteToScript;            /* offset to lead collation byte to script mapping data */
      uint8_t reserved[76];                 /* for future use */
} UCATableHeader;

#define U_UNKNOWN_STATE 0
#define U_COLLATOR_STATE 0x01
#define U_STATE_LIMIT 0x02

/* This is the first structure in a state */
/* it should be machine independent */
typedef struct {
  /* this structure is supposed to be readable on all the platforms.*/
  /* first 2 fields hold the size of the structure in a platform independent way */
  uint8_t sizeLo;
  uint8_t sizeHi;
  /* identifying the writing platform */
  uint8_t isBigEndian;
  /* see U_CHARSET_FAMILY values in utypes.h */
  uint8_t charsetFamily;
  /* version of ICU this state structure comes from */
  uint8_t icuVersion[4];
  /* What is the data following this state */
  uint8_t type;
  /* more stuff to come, keep it on 16 byte boundary */
  uint8_t reserved[7];
} UStateStruct;

/* This structure follows UStatusStruct */
/* and contains data specific for the collators */
/* Endianess needs to be decided before accessing this structure */
/* However, it's size IS endianess independent */
typedef struct {
  /* size of this structure */
  uint8_t sizeLo;
  uint8_t sizeHi;
  /* This state is followed by the frozen tailoring */
  uint8_t containsTailoring;
  /* This state is followed by the frozen UCA */
  uint8_t containsUCA;
  /* Version info - the same one */
  uint8_t versionInfo[4];

  /* for charset CEs */
  uint8_t charsetName[32];                 
  /* this is the resolved locale name*/
  uint8_t locale[32];                      

  /* Attributes. Open ended */
  /* all the following will be moved to uint32_t because of portability */
  /* variable top value */
  uint32_t variableTopValue;
  /* attribute for handling variable elements*/
  uint32_t /*UColAttributeValue*/ alternateHandling; 
  /* how to handle secondary weights */
  uint32_t /*UColAttributeValue*/ frenchCollation;
  /* who goes first, lower case or uppercase */
  uint32_t /*UColAttributeValue*/ caseFirst;         
  /* do we have an extra case level */
  uint32_t /*UColAttributeValue*/ caseLevel;         
  /* attribute for normalization */
  uint32_t /*UColAttributeValue*/ normalizationMode; 
  /* attribute for strength */
  uint32_t /*UColAttributeValue*/ strength;
  /* to be immediately 16 byte aligned */
  uint8_t reserved[12];
} UColStateStruct;

#define UCOL_INV_SIZEMASK 0xFFF00000
#define UCOL_INV_OFFSETMASK 0x000FFFFF
#define UCOL_INV_SHIFTVALUE 20

U_CDECL_BEGIN

typedef struct {
  uint32_t byteSize;
  uint32_t tableSize;
  uint32_t contsSize;
  uint32_t table;
  uint32_t conts;
  UVersionInfo UCAVersion;              /* version of the UCA, read from file */
  uint8_t padding[8];
} InverseUCATableHeader;

typedef int32_t U_CALLCONV
SortKeyGenerator(const    UCollator    *coll,
        const    UChar        *source,
        int32_t        sourceLength,
        uint8_t        **result,
        uint32_t        resultLength,
        UBool allocatePrimary,
        UErrorCode *status);

typedef void U_CALLCONV
ResourceCleaner(UCollator *coll);


struct UCollator {
    UColOptionSet  *options;
    SortKeyGenerator *sortKeyGen;
    uint32_t *latinOneCEs;
    char* actualLocale;
    char* validLocale;
    char* requestedLocale;
    const UChar *rules;
    const UChar *ucaRules;
    const UCollator *UCA;
    const UCATableHeader *image;
    UTrie mapping;
    const uint32_t *latinOneMapping;
    const uint32_t *expansion;
    const UChar    *contractionIndex;
    const uint32_t *contractionCEs;

    const uint32_t *endExpansionCE;    /* array of last ces in an expansion ce.
                                          corresponds to expansionCESize */
    const uint32_t *lastEndExpansionCE;/* pointer to the last element in endExpansionCE */
    const uint8_t  *expansionCESize;   /* array of the maximum size of a
                                         expansion ce with the last ce
                                         corresponding to endExpansionCE,
                                         terminated with a null */
    const uint8_t *unsafeCP;           /* unsafe code points hashtable */
    const uint8_t *contrEndCP;         /* Contraction ending chars hash table */
    UChar          minUnsafeCP;        /* Smallest unsafe Code Point. */
    UChar          minContrEndCP;      /* Smallest code point at end of a contraction */

    int32_t rulesLength;
    int32_t latinOneTableLen;

    uint32_t variableTopValue;
    UColAttributeValue frenchCollation;
    UColAttributeValue alternateHandling; /* attribute for handling variable elements*/
    UColAttributeValue caseFirst;         /* who goes first, lower case or uppercase */
    UColAttributeValue caseLevel;         /* do we have an extra case level */
    UColAttributeValue normalizationMode; /* attribute for normalization */
    UColAttributeValue strength;          /* attribute for strength */
    UColAttributeValue hiraganaQ;         /* attribute for Hiragana */
    UColAttributeValue numericCollation;
    UBool variableTopValueisDefault;
    UBool frenchCollationisDefault;
    UBool alternateHandlingisDefault; /* attribute for handling variable elements*/
    UBool caseFirstisDefault;         /* who goes first, lower case or uppercase */
    UBool caseLevelisDefault;         /* do we have an extra case level */
    UBool normalizationModeisDefault; /* attribute for normalization */
    UBool strengthisDefault;          /* attribute for strength */
    UBool hiraganaQisDefault;         /* attribute for Hiragana */
    UBool numericCollationisDefault;
    UBool hasRealData;                /* some collators have only options, like French, no rules */
                                      /* to speed up things, we use the UCA image, but we don't want it */
                                      /* to run around */

    UBool freeOnClose;
    UBool freeOptionsOnClose;
    UBool freeRulesOnClose;
    UBool freeImageOnClose;

    UBool latinOneUse;
    UBool latinOneRegenTable;
    UBool latinOneFailed;

    int8_t tertiaryAddition; /* when switching case, we need to add or subtract different values */
    uint8_t caseSwitch;
    uint8_t tertiaryCommon;
    uint8_t tertiaryMask;
    uint8_t tertiaryTop; /* Upper range when compressing */
    uint8_t tertiaryBottom; /* Upper range when compressing */
    uint8_t tertiaryTopCount;
    uint8_t tertiaryBottomCount;

    UVersionInfo dataVersion;               /* Data info of UCA table */
    int32_t* reorderCodes;
    int32_t reorderCodesLength;
    uint8_t* leadBytePermutationTable;
};

U_CDECL_END

/* various internal functions */

/* do not close UCA returned by ucol_initUCA! */
U_CFUNC
UCollator* ucol_initUCA(UErrorCode *status);

U_CFUNC
UCollator* ucol_initCollator(const UCATableHeader *image, UCollator *fillIn, const UCollator *UCA, UErrorCode *status);

U_CFUNC
void ucol_setOptionsFromHeader(UCollator* result, UColOptionSet * opts, UErrorCode *status);

U_CFUNC
UCollator* ucol_open_internal(const char* loc, UErrorCode* status);

#if 0
U_CFUNC
void ucol_putOptionsToHeader(UCollator* result, UColOptionSet * opts, UErrorCode *status);
#endif

U_CFUNC
void ucol_updateInternalState(UCollator *coll, UErrorCode *status);

U_CFUNC uint32_t U_EXPORT2 ucol_getFirstCE(const UCollator *coll, UChar u, UErrorCode *status);
U_CAPI UBool U_EXPORT2 ucol_isTailored(const UCollator *coll, const UChar u, UErrorCode *status);

U_CAPI const InverseUCATableHeader* U_EXPORT2 ucol_initInverseUCA(UErrorCode *status);

U_CAPI void U_EXPORT2 
uprv_uca_initImplicitConstants(UErrorCode *status);

U_CAPI uint32_t U_EXPORT2
uprv_uca_getImplicitFromRaw(UChar32 cp);

/*U_CFUNC uint32_t U_EXPORT2
uprv_uca_getImplicitPrimary(UChar32 cp);*/

U_CAPI UChar32 U_EXPORT2
uprv_uca_getRawFromImplicit(uint32_t implicit);

U_CAPI UChar32 U_EXPORT2
uprv_uca_getRawFromCodePoint(UChar32 i);

U_CAPI UChar32 U_EXPORT2
uprv_uca_getCodePointFromRaw(UChar32 i);

typedef const UChar* GetCollationRulesFunction(void* context, const char* locale, const char* type, int32_t* pLength, UErrorCode* status);

U_CAPI UCollator* U_EXPORT2
ucol_openRulesForImport( const UChar        *rules,
                         int32_t            rulesLength,
                         UColAttributeValue normalizationMode,
                         UCollationStrength strength,
                         UParseError        *parseError,
                         GetCollationRulesFunction  importFunc,
                         void* context,
                         UErrorCode         *status);

       
U_CAPI void U_EXPORT2 ucol_buildPermutationTable(UCollator *coll, UErrorCode *status);


#ifdef XP_CPLUSPLUS
/*
 *  Test whether a character is potentially "unsafe" for use as a collation
 *  starting point.  Unsafe chars are those with combining class != 0 plus
 *  those that are the 2nd thru nth character in a contraction sequence.
 *
 *  Function is in header file because it's used in both collation and string search,
 *  and needs to be inline for performance.
 */
static inline UBool ucol_unsafeCP(UChar c, const UCollator *coll) {
    int32_t  hash;
    uint8_t  htbyte;

    if (c < coll->minUnsafeCP) {
        return FALSE;
    }

    hash = c;
    if (hash >= UCOL_UNSAFECP_TABLE_SIZE*8) {
        if(UTF_IS_SURROGATE(c)) {
            /*  Lead or trail surrogate             */
            /*  These are always considered unsafe. */
            return TRUE;
        }
        hash = (hash & UCOL_UNSAFECP_TABLE_MASK) + 256;
    }
    htbyte = coll->unsafeCP[hash>>3];
    return ((htbyte >> (hash & 7)) & 1);
}
#endif /* XP_CPLUSPLUS */

/* The offsetBuffer in collIterate might need to be freed to avoid memory leaks. */
void ucol_freeOffsetBuffer(U_NAMESPACE_QUALIFIER collIterate *s); 

#endif /* #if !UCONFIG_NO_COLLATION */

#endif
