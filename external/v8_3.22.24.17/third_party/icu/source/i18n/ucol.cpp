/*
*******************************************************************************
*   Copyright (C) 1996-2011, International Business Machines
*   Corporation and others.  All Rights Reserved.
*******************************************************************************
*   file name:  ucol.cpp
*   encoding:   US-ASCII
*   tab size:   8 (not used)
*   indentation:4
*
* Modification history
* Date        Name      Comments
* 1996-1999   various members of ICU team maintained C API for collation framework
* 02/16/2001  synwee    Added internal method getPrevSpecialCE
* 03/01/2001  synwee    Added maxexpansion functionality.
* 03/16/2001  weiv      Collation framework is rewritten in C and made UCA compliant
*/

#include "unicode/utypes.h"

#if !UCONFIG_NO_COLLATION

#include "unicode/coleitr.h"
#include "unicode/unorm.h"
#include "unicode/udata.h"
#include "unicode/ustring.h"

#include "ucol_imp.h"
#include "bocsu.h"

#include "normalizer2impl.h"
#include "unorm_it.h"
#include "umutex.h"
#include "cmemory.h"
#include "ucln_in.h"
#include "cstring.h"
#include "utracimp.h"
#include "putilimp.h"
#include "uassert.h"

#ifdef UCOL_DEBUG
#include <stdio.h>
#endif

U_NAMESPACE_USE

#define LENGTHOF(array) (int32_t)(sizeof(array)/sizeof((array)[0]))

#define LAST_BYTE_MASK_           0xFF
#define SECOND_LAST_BYTE_SHIFT_   8

#define ZERO_CC_LIMIT_            0xC0

// this is static pointer to the normalizer fcdTrieIndex
// it is always the same between calls to u_cleanup
// and therefore writing to it is not synchronized.
// It is cleaned in ucol_cleanup
static const uint16_t *fcdTrieIndex=NULL;
// Code points at fcdHighStart and above have a zero FCD value.
static UChar32 fcdHighStart = 0;

// These are values from UCA required for
// implicit generation and supressing sort key compression
// they should regularly be in the UCA, but if one
// is running without UCA, it could be a problem
static const int32_t maxRegularPrimary  = 0x7A;
static const int32_t minImplicitPrimary = 0xE0;
static const int32_t maxImplicitPrimary = 0xE4;

U_CDECL_BEGIN
static UBool U_CALLCONV
ucol_cleanup(void)
{
    fcdTrieIndex = NULL;
    return TRUE;
}

static int32_t U_CALLCONV
_getFoldingOffset(uint32_t data) {
    return (int32_t)(data&0xFFFFFF);
}

U_CDECL_END

// init FCD data
static inline
UBool initializeFCD(UErrorCode *status) {
    if (fcdTrieIndex != NULL) {
        return TRUE;
    } else {
        // The result is constant, until the library is reloaded.
        fcdTrieIndex = unorm_getFCDTrieIndex(fcdHighStart, status);
        ucln_i18n_registerCleanup(UCLN_I18N_UCOL, ucol_cleanup);
        return U_SUCCESS(*status);
    }
}

static
inline void IInit_collIterate(const UCollator *collator, const UChar *sourceString,
                              int32_t sourceLen, collIterate *s,
                              UErrorCode *status)
{
    (s)->string = (s)->pos = sourceString;
    (s)->origFlags = 0;
    (s)->flags = 0;
    if (sourceLen >= 0) {
        s->flags |= UCOL_ITER_HASLEN;
        (s)->endp = (UChar *)sourceString+sourceLen;
    }
    else {
        /* change to enable easier checking for end of string for fcdpositon */
        (s)->endp = NULL;
    }
    (s)->extendCEs = NULL;
    (s)->extendCEsSize = 0;
    (s)->CEpos = (s)->toReturn = (s)->CEs;
    (s)->offsetBuffer = NULL;
    (s)->offsetBufferSize = 0;
    (s)->offsetReturn = (s)->offsetStore = NULL;
    (s)->offsetRepeatCount = (s)->offsetRepeatValue = 0;
    (s)->coll = (collator);
    (s)->nfd = Normalizer2Factory::getNFDInstance(*status);
    (s)->fcdPosition = 0;
    if(collator->normalizationMode == UCOL_ON) {
        (s)->flags |= UCOL_ITER_NORM;
    }
    if(collator->hiraganaQ == UCOL_ON && collator->strength >= UCOL_QUATERNARY) {
        (s)->flags |= UCOL_HIRAGANA_Q;
    }
    (s)->iterator = NULL;
    //(s)->iteratorIndex = 0;
}

U_CAPI void  U_EXPORT2
uprv_init_collIterate(const UCollator *collator, const UChar *sourceString,
                             int32_t sourceLen, collIterate *s,
                             UErrorCode *status) {
    /* Out-of-line version for use from other files. */
    IInit_collIterate(collator, sourceString, sourceLen, s, status);
}

U_CAPI collIterate * U_EXPORT2 
uprv_new_collIterate(UErrorCode *status) {
    if(U_FAILURE(*status)) {
        return NULL;
    }
    collIterate *s = new collIterate;
    if(s == NULL) {
        *status = U_MEMORY_ALLOCATION_ERROR;
        return NULL;
    }
    return s;
}

U_CAPI void U_EXPORT2 
uprv_delete_collIterate(collIterate *s) {
    delete s;
}

U_CAPI UBool U_EXPORT2
uprv_collIterateAtEnd(collIterate *s) {
    return s == NULL || s->pos == s->endp;
}

/**
* Backup the state of the collIterate struct data
* @param data collIterate to backup
* @param backup storage
*/
static
inline void backupState(const collIterate *data, collIterateState *backup)
{
    backup->fcdPosition = data->fcdPosition;
    backup->flags       = data->flags;
    backup->origFlags   = data->origFlags;
    backup->pos         = data->pos;
    backup->bufferaddress = data->writableBuffer.getBuffer();
    backup->buffersize    = data->writableBuffer.length();
    backup->iteratorMove = 0;
    backup->iteratorIndex = 0;
    if(data->iterator != NULL) {
        //backup->iteratorIndex = data->iterator->getIndex(data->iterator, UITER_CURRENT);
        backup->iteratorIndex = data->iterator->getState(data->iterator);
        // no we try to fixup if we're using a normalizing iterator and we get UITER_NO_STATE
        if(backup->iteratorIndex == UITER_NO_STATE) {
            while((backup->iteratorIndex = data->iterator->getState(data->iterator)) == UITER_NO_STATE) {
                backup->iteratorMove++;
                data->iterator->move(data->iterator, -1, UITER_CURRENT);
            }
            data->iterator->move(data->iterator, backup->iteratorMove, UITER_CURRENT);
        }
    }
}

/**
* Loads the state into the collIterate struct data
* @param data collIterate to backup
* @param backup storage
* @param forwards boolean to indicate if forwards iteration is used,
*        false indicates backwards iteration
*/
static
inline void loadState(collIterate *data, const collIterateState *backup,
                      UBool        forwards)
{
    UErrorCode status = U_ZERO_ERROR;
    data->flags       = backup->flags;
    data->origFlags   = backup->origFlags;
    if(data->iterator != NULL) {
        //data->iterator->move(data->iterator, backup->iteratorIndex, UITER_ZERO);
        data->iterator->setState(data->iterator, backup->iteratorIndex, &status);
        if(backup->iteratorMove != 0) {
            data->iterator->move(data->iterator, backup->iteratorMove, UITER_CURRENT);
        }
    }
    data->pos         = backup->pos;

    if ((data->flags & UCOL_ITER_INNORMBUF) &&
        data->writableBuffer.getBuffer() != backup->bufferaddress) {
        /*
        this is when a new buffer has been reallocated and we'll have to
        calculate the new position.
        note the new buffer has to contain the contents of the old buffer.
        */
        if (forwards) {
            data->pos = data->writableBuffer.getTerminatedBuffer() +
                                         (data->pos - backup->bufferaddress);
        }
        else {
            /* backwards direction */
            int32_t temp = backup->buffersize -
                                  (int32_t)(data->pos - backup->bufferaddress);
            data->pos = data->writableBuffer.getTerminatedBuffer() + (data->writableBuffer.length() - temp);
        }
    }
    if ((data->flags & UCOL_ITER_INNORMBUF) == 0) {
        /*
        this is alittle tricky.
        if we are initially not in the normalization buffer, even if we
        normalize in the later stage, the data in the buffer will be
        ignored, since we skip back up to the data string.
        however if we are already in the normalization buffer, any
        further normalization will pull data into the normalization
        buffer and modify the fcdPosition.
        since we are keeping the data in the buffer for use, the
        fcdPosition can not be reverted back.
        arrgghh....
        */
        data->fcdPosition = backup->fcdPosition;
    }
}

static UBool
reallocCEs(collIterate *data, int32_t newCapacity) {
    uint32_t *oldCEs = data->extendCEs;
    if(oldCEs == NULL) {
        oldCEs = data->CEs;
    }
    int32_t length = data->CEpos - oldCEs;
    uint32_t *newCEs = (uint32_t *)uprv_malloc(newCapacity * 4);
    if(newCEs == NULL) {
        return FALSE;
    }
    uprv_memcpy(newCEs, oldCEs, length * 4);
    uprv_free(data->extendCEs);
    data->extendCEs = newCEs;
    data->extendCEsSize = newCapacity;
    data->CEpos = newCEs + length;
    return TRUE;
}

static UBool
increaseCEsCapacity(collIterate *data) {
    int32_t oldCapacity;
    if(data->extendCEs != NULL) {
        oldCapacity = data->extendCEsSize;
    } else {
        oldCapacity = LENGTHOF(data->CEs);
    }
    return reallocCEs(data, 2 * oldCapacity);
}

static UBool
ensureCEsCapacity(collIterate *data, int32_t minCapacity) {
    int32_t oldCapacity;
    if(data->extendCEs != NULL) {
        oldCapacity = data->extendCEsSize;
    } else {
        oldCapacity = LENGTHOF(data->CEs);
    }
    if(minCapacity <= oldCapacity) {
        return TRUE;
    }
    oldCapacity *= 2;
    return reallocCEs(data, minCapacity > oldCapacity ? minCapacity : oldCapacity);
}

void collIterate::appendOffset(int32_t offset, UErrorCode &errorCode) {
    if(U_FAILURE(errorCode)) {
        return;
    }
    int32_t length = offsetStore == NULL ? 0 : (int32_t)(offsetStore - offsetBuffer);
    if(length >= offsetBufferSize) {
        int32_t newCapacity = 2 * offsetBufferSize + UCOL_EXPAND_CE_BUFFER_SIZE;
        int32_t *newBuffer = reinterpret_cast<int32_t *>(uprv_malloc(newCapacity * 4));
        if(newBuffer == NULL) {
            errorCode = U_MEMORY_ALLOCATION_ERROR;
            return;
        }
        if(length > 0) {
            uprv_memcpy(newBuffer, offsetBuffer, length * 4);
        }
        uprv_free(offsetBuffer);
        offsetBuffer = newBuffer;
        offsetStore = offsetBuffer + length;
        offsetBufferSize = newCapacity;
    }
    *offsetStore++ = offset;
}

/*
* collIter_eos()
*     Checks for a collIterate being positioned at the end of
*     its source string.
*
*/
static
inline UBool collIter_eos(collIterate *s) {
    if(s->flags & UCOL_USE_ITERATOR) {
      return !(s->iterator->hasNext(s->iterator));
    }
    if ((s->flags & UCOL_ITER_HASLEN) == 0 && *s->pos != 0) {
        // Null terminated string, but not at null, so not at end.
        //   Whether in main or normalization buffer doesn't matter.
        return FALSE;
    }

    // String with length.  Can't be in normalization buffer, which is always
    //  null termintated.
    if (s->flags & UCOL_ITER_HASLEN) {
        return (s->pos == s->endp);
    }

    // We are at a null termination, could be either normalization buffer or main string.
    if ((s->flags & UCOL_ITER_INNORMBUF) == 0) {
        // At null at end of main string.
        return TRUE;
    }

    // At null at end of normalization buffer.  Need to check whether there there are
    //   any characters left in the main buffer.
    if(s->origFlags & UCOL_USE_ITERATOR) {
      return !(s->iterator->hasNext(s->iterator));
    } else if ((s->origFlags & UCOL_ITER_HASLEN) == 0) {
        // Null terminated main string.  fcdPosition is the 'return' position into main buf.
        return (*s->fcdPosition == 0);
    }
    else {
        // Main string with an end pointer.
        return s->fcdPosition == s->endp;
    }
}

/*
* collIter_bos()
*     Checks for a collIterate being positioned at the start of
*     its source string.
*
*/
static
inline UBool collIter_bos(collIterate *source) {
  // if we're going backwards, we need to know whether there is more in the
  // iterator, even if we are in the side buffer
  if(source->flags & UCOL_USE_ITERATOR || source->origFlags & UCOL_USE_ITERATOR) {
    return !source->iterator->hasPrevious(source->iterator);
  }
  if (source->pos <= source->string ||
      ((source->flags & UCOL_ITER_INNORMBUF) &&
      *(source->pos - 1) == 0 && source->fcdPosition == NULL)) {
    return TRUE;
  }
  return FALSE;
}

/*static
inline UBool collIter_SimpleBos(collIterate *source) {
  // if we're going backwards, we need to know whether there is more in the
  // iterator, even if we are in the side buffer
  if(source->flags & UCOL_USE_ITERATOR || source->origFlags & UCOL_USE_ITERATOR) {
    return !source->iterator->hasPrevious(source->iterator);
  }
  if (source->pos == source->string) {
    return TRUE;
  }
  return FALSE;
}*/
    //return (data->pos == data->string) ||


/****************************************************************************/
/* Following are the open/close functions                                   */
/*                                                                          */
/****************************************************************************/

static UCollator*
ucol_initFromBinary(const uint8_t *bin, int32_t length,
                const UCollator *base,
                UCollator *fillIn,
                UErrorCode *status)
{
    UCollator *result = fillIn;
    if(U_FAILURE(*status)) {
        return NULL;
    }
    /*
    if(base == NULL) {
        // we don't support null base yet
        *status = U_ILLEGAL_ARGUMENT_ERROR;
        return NULL;
    }
    */
    // We need these and we could be running without UCA
    uprv_uca_initImplicitConstants(status);
    UCATableHeader *colData = (UCATableHeader *)bin;
    // do we want version check here? We're trying to figure out whether collators are compatible
    if((base && (uprv_memcmp(colData->UCAVersion, base->image->UCAVersion, sizeof(UVersionInfo)) != 0 ||
        uprv_memcmp(colData->UCDVersion, base->image->UCDVersion, sizeof(UVersionInfo)) != 0)) ||
        colData->version[0] != UCOL_BUILDER_VERSION)
    {
        *status = U_COLLATOR_VERSION_MISMATCH;
        return NULL;
    }
    else {
        if((uint32_t)length > (paddedsize(sizeof(UCATableHeader)) + paddedsize(sizeof(UColOptionSet)))) {
            result = ucol_initCollator((const UCATableHeader *)bin, result, base, status);
            if(U_FAILURE(*status)){
                return NULL;
            }
            result->hasRealData = TRUE;
        }
        else {
            if(base) {
                result = ucol_initCollator(base->image, result, base, status);
                ucol_setOptionsFromHeader(result, (UColOptionSet *)(bin+((const UCATableHeader *)bin)->options), status);
                if(U_FAILURE(*status)){
                    return NULL;
                }
                result->hasRealData = FALSE;
            }
            else {
                *status = U_USELESS_COLLATOR_ERROR;
                return NULL;
            }
        }
        result->freeImageOnClose = FALSE;
    }
    result->actualLocale = NULL;
    result->validLocale = NULL;
    result->requestedLocale = NULL;
    result->rules = NULL;
    result->rulesLength = 0;
    result->freeRulesOnClose = FALSE;
    result->ucaRules = NULL;
    return result;
}

U_CAPI UCollator* U_EXPORT2
ucol_openBinary(const uint8_t *bin, int32_t length,
                const UCollator *base,
                UErrorCode *status)
{
    return ucol_initFromBinary(bin, length, base, NULL, status);
}

U_CAPI int32_t U_EXPORT2
ucol_cloneBinary(const UCollator *coll,
                 uint8_t *buffer, int32_t capacity,
                 UErrorCode *status)
{
    int32_t length = 0;
    if(U_FAILURE(*status)) {
        return length;
    }
    if(capacity < 0) {
        *status = U_ILLEGAL_ARGUMENT_ERROR;
        return length;
    }
    if(coll->hasRealData == TRUE) {
        length = coll->image->size;
        if(length <= capacity) {
            uprv_memcpy(buffer, coll->image, length);
        } else {
            *status = U_BUFFER_OVERFLOW_ERROR;
        }
    } else {
        length = (int32_t)(paddedsize(sizeof(UCATableHeader))+paddedsize(sizeof(UColOptionSet)));
        if(length <= capacity) {
            /* build the UCATableHeader with minimal entries */
            /* do not copy the header from the UCA file because its values are wrong! */
            /* uprv_memcpy(result, UCA->image, sizeof(UCATableHeader)); */

            /* reset everything */
            uprv_memset(buffer, 0, length);

            /* set the tailoring-specific values */
            UCATableHeader *myData = (UCATableHeader *)buffer;
            myData->size = length;

            /* offset for the options, the only part of the data that is present after the header */
            myData->options = sizeof(UCATableHeader);

            /* need to always set the expansion value for an upper bound of the options */
            myData->expansion = myData->options + sizeof(UColOptionSet);

            myData->magic = UCOL_HEADER_MAGIC;
            myData->isBigEndian = U_IS_BIG_ENDIAN;
            myData->charSetFamily = U_CHARSET_FAMILY;

            /* copy UCA's version; genrb will override all but the builder version with tailoring data */
            uprv_memcpy(myData->version, coll->image->version, sizeof(UVersionInfo));

            uprv_memcpy(myData->UCAVersion, coll->image->UCAVersion, sizeof(UVersionInfo));
            uprv_memcpy(myData->UCDVersion, coll->image->UCDVersion, sizeof(UVersionInfo));
            uprv_memcpy(myData->formatVersion, coll->image->formatVersion, sizeof(UVersionInfo));
            myData->jamoSpecial = coll->image->jamoSpecial;

            /* copy the collator options */
            uprv_memcpy(buffer+paddedsize(sizeof(UCATableHeader)), coll->options, sizeof(UColOptionSet));
        } else {
            *status = U_BUFFER_OVERFLOW_ERROR;
        }
    }
    return length;
}

U_CAPI UCollator* U_EXPORT2
ucol_safeClone(const UCollator *coll, void *stackBuffer, int32_t * pBufferSize, UErrorCode *status)
{
    UCollator * localCollator;
    int32_t bufferSizeNeeded = (int32_t)sizeof(UCollator);
    char *stackBufferChars = (char *)stackBuffer;
    int32_t imageSize = 0;
    int32_t rulesSize = 0;
    int32_t rulesPadding = 0;
    uint8_t *image;
    UChar *rules;
    UBool colAllocated = FALSE;
    UBool imageAllocated = FALSE;

    if (status == NULL || U_FAILURE(*status)){
        return 0;
    }
    if ((stackBuffer && !pBufferSize) || !coll){
       *status = U_ILLEGAL_ARGUMENT_ERROR;
        return 0;
    }
    if (coll->rules && coll->freeRulesOnClose) {
        rulesSize = (int32_t)(coll->rulesLength + 1)*sizeof(UChar);
        rulesPadding = (int32_t)(bufferSizeNeeded % sizeof(UChar));
        bufferSizeNeeded += rulesSize + rulesPadding;
    }

    if (stackBuffer && *pBufferSize <= 0){ /* 'preflighting' request - set needed size into *pBufferSize */
        *pBufferSize =  bufferSizeNeeded;
        return 0;
    }

    /* Pointers on 64-bit platforms need to be aligned
     * on a 64-bit boundry in memory.
     */
    if (U_ALIGNMENT_OFFSET(stackBuffer) != 0) {
        int32_t offsetUp = (int32_t)U_ALIGNMENT_OFFSET_UP(stackBufferChars);
        if (*pBufferSize > offsetUp) {
            *pBufferSize -= offsetUp;
            stackBufferChars += offsetUp;
        }
        else {
            /* prevent using the stack buffer but keep the size > 0 so that we do not just preflight */
            *pBufferSize = 1;
        }
    }
    stackBuffer = (void *)stackBufferChars;

    if (stackBuffer == NULL || *pBufferSize < bufferSizeNeeded) {
        /* allocate one here...*/
        stackBufferChars = (char *)uprv_malloc(bufferSizeNeeded);
        // Null pointer check.
        if (stackBufferChars == NULL) {
            *status = U_MEMORY_ALLOCATION_ERROR;
            return NULL;
        }
        colAllocated = TRUE;
        if (U_SUCCESS(*status)) {
            *status = U_SAFECLONE_ALLOCATED_WARNING;
        }
    }
    localCollator = (UCollator *)stackBufferChars;
    rules = (UChar *)(stackBufferChars + sizeof(UCollator) + rulesPadding);
    {
        UErrorCode tempStatus = U_ZERO_ERROR;
        imageSize = ucol_cloneBinary(coll, NULL, 0, &tempStatus);
    }
    if (coll->freeImageOnClose) {
        image = (uint8_t *)uprv_malloc(imageSize);
        // Null pointer check
        if (image == NULL) {
            *status = U_MEMORY_ALLOCATION_ERROR;
            return NULL;
        }
        ucol_cloneBinary(coll, image, imageSize, status);
        imageAllocated = TRUE;
    }
    else {
        image = (uint8_t *)coll->image;
    }
    localCollator = ucol_initFromBinary(image, imageSize, coll->UCA, localCollator, status);
    if (U_FAILURE(*status)) {
        return NULL;
    }

    if (coll->rules) {
        if (coll->freeRulesOnClose) {
            localCollator->rules = u_strcpy(rules, coll->rules);
            //bufferEnd += rulesSize;
        }
        else {
            localCollator->rules = coll->rules;
        }
        localCollator->freeRulesOnClose = FALSE;
        localCollator->rulesLength = coll->rulesLength;
    }

    int32_t i;
    for(i = 0; i < UCOL_ATTRIBUTE_COUNT; i++) {
        ucol_setAttribute(localCollator, (UColAttribute)i, ucol_getAttribute(coll, (UColAttribute)i, status), status);
    }
    // zero copies of pointers
    localCollator->actualLocale = NULL;
    localCollator->validLocale = NULL;
    localCollator->requestedLocale = NULL;
    localCollator->ucaRules = coll->ucaRules; // There should only be one copy here.
    localCollator->freeOnClose = colAllocated;
    localCollator->freeImageOnClose = imageAllocated;
    return localCollator;
}

U_CAPI void U_EXPORT2
ucol_close(UCollator *coll)
{
    UTRACE_ENTRY_OC(UTRACE_UCOL_CLOSE);
    UTRACE_DATA1(UTRACE_INFO, "coll = %p", coll);
    if(coll != NULL) {
        // these are always owned by each UCollator struct,
        // so we always free them
        if(coll->validLocale != NULL) {
            uprv_free(coll->validLocale);
        }
        if(coll->actualLocale != NULL) {
            uprv_free(coll->actualLocale);
        }
        if(coll->requestedLocale != NULL) {
            uprv_free(coll->requestedLocale);
        }
        if(coll->latinOneCEs != NULL) {
            uprv_free(coll->latinOneCEs);
        }
        if(coll->options != NULL && coll->freeOptionsOnClose) {
            uprv_free(coll->options);
        }
        if(coll->rules != NULL && coll->freeRulesOnClose) {
            uprv_free((UChar *)coll->rules);
        }
        if(coll->image != NULL && coll->freeImageOnClose) {
            uprv_free((UCATableHeader *)coll->image);
        }
        if(coll->leadBytePermutationTable != NULL) {
            uprv_free(coll->leadBytePermutationTable);
        }
        if(coll->reorderCodes != NULL) {
            uprv_free(coll->reorderCodes);
        }

        /* Here, it would be advisable to close: */
        /* - UData for UCA (unless we stuff it in the root resb */
        /* Again, do we need additional housekeeping... HMMM! */
        UTRACE_DATA1(UTRACE_INFO, "coll->freeOnClose: %d", coll->freeOnClose);
        if(coll->freeOnClose){
            /* for safeClone, if freeOnClose is FALSE,
            don't free the other instance data */
            uprv_free(coll);
        }
    }
    UTRACE_EXIT();
}

/* This one is currently used by genrb & tests. After constructing from rules (tailoring),*/
/* you should be able to get the binary chunk to write out...  Doesn't look very full now */
U_CFUNC uint8_t* U_EXPORT2
ucol_cloneRuleData(const UCollator *coll, int32_t *length, UErrorCode *status)
{
    uint8_t *result = NULL;
    if(U_FAILURE(*status)) {
        return NULL;
    }
    if(coll->hasRealData == TRUE) {
        *length = coll->image->size;
        result = (uint8_t *)uprv_malloc(*length);
        /* test for NULL */
        if (result == NULL) {
            *status = U_MEMORY_ALLOCATION_ERROR;
            return NULL;
        }
        uprv_memcpy(result, coll->image, *length);
    } else {
        *length = (int32_t)(paddedsize(sizeof(UCATableHeader))+paddedsize(sizeof(UColOptionSet)));
        result = (uint8_t *)uprv_malloc(*length);
        /* test for NULL */
        if (result == NULL) {
            *status = U_MEMORY_ALLOCATION_ERROR;
            return NULL;
        }

        /* build the UCATableHeader with minimal entries */
        /* do not copy the header from the UCA file because its values are wrong! */
        /* uprv_memcpy(result, UCA->image, sizeof(UCATableHeader)); */

        /* reset everything */
        uprv_memset(result, 0, *length);

        /* set the tailoring-specific values */
        UCATableHeader *myData = (UCATableHeader *)result;
        myData->size = *length;

        /* offset for the options, the only part of the data that is present after the header */
        myData->options = sizeof(UCATableHeader);

        /* need to always set the expansion value for an upper bound of the options */
        myData->expansion = myData->options + sizeof(UColOptionSet);

        myData->magic = UCOL_HEADER_MAGIC;
        myData->isBigEndian = U_IS_BIG_ENDIAN;
        myData->charSetFamily = U_CHARSET_FAMILY;

        /* copy UCA's version; genrb will override all but the builder version with tailoring data */
        uprv_memcpy(myData->version, coll->image->version, sizeof(UVersionInfo));

        uprv_memcpy(myData->UCAVersion, coll->image->UCAVersion, sizeof(UVersionInfo));
        uprv_memcpy(myData->UCDVersion, coll->image->UCDVersion, sizeof(UVersionInfo));
        uprv_memcpy(myData->formatVersion, coll->image->formatVersion, sizeof(UVersionInfo));
        myData->jamoSpecial = coll->image->jamoSpecial;

        /* copy the collator options */
        uprv_memcpy(result+paddedsize(sizeof(UCATableHeader)), coll->options, sizeof(UColOptionSet));
    }
    return result;
}

void ucol_setOptionsFromHeader(UCollator* result, UColOptionSet * opts, UErrorCode *status) {
    if(U_FAILURE(*status)) {
        return;
    }
    result->caseFirst = (UColAttributeValue)opts->caseFirst;
    result->caseLevel = (UColAttributeValue)opts->caseLevel;
    result->frenchCollation = (UColAttributeValue)opts->frenchCollation;
    result->normalizationMode = (UColAttributeValue)opts->normalizationMode;
    if(result->normalizationMode == UCOL_ON && !initializeFCD(status)) {
        return;
    }
    result->strength = (UColAttributeValue)opts->strength;
    result->variableTopValue = opts->variableTopValue;
    result->alternateHandling = (UColAttributeValue)opts->alternateHandling;
    result->hiraganaQ = (UColAttributeValue)opts->hiraganaQ;
    result->numericCollation = (UColAttributeValue)opts->numericCollation;
    result->caseFirstisDefault = TRUE;
    result->caseLevelisDefault = TRUE;
    result->frenchCollationisDefault = TRUE;
    result->normalizationModeisDefault = TRUE;
    result->strengthisDefault = TRUE;
    result->variableTopValueisDefault = TRUE;
    result->alternateHandlingisDefault = TRUE;
    result->hiraganaQisDefault = TRUE;
    result->numericCollationisDefault = TRUE;

    ucol_updateInternalState(result, status);

    result->options = opts;
}


/**
* Approximate determination if a character is at a contraction end.
* Guaranteed to be TRUE if a character is at the end of a contraction,
* otherwise it is not deterministic.
* @param c character to be determined
* @param coll collator
*/
static
inline UBool ucol_contractionEndCP(UChar c, const UCollator *coll) {
    if (c < coll->minContrEndCP) {
        return FALSE;
    }

    int32_t  hash = c;
    uint8_t  htbyte;
    if (hash >= UCOL_UNSAFECP_TABLE_SIZE*8) {
        if (U16_IS_TRAIL(c)) {
            return TRUE;
        }
        hash = (hash & UCOL_UNSAFECP_TABLE_MASK) + 256;
    }
    htbyte = coll->contrEndCP[hash>>3];
    return (((htbyte >> (hash & 7)) & 1) == 1);
}



/*
*   i_getCombiningClass()
*        A fast, at least partly inline version of u_getCombiningClass()
*        This is a candidate for further optimization.  Used heavily
*        in contraction processing.
*/
static
inline uint8_t i_getCombiningClass(UChar32 c, const UCollator *coll) {
    uint8_t sCC = 0;
    if ((c >= 0x300 && ucol_unsafeCP(c, coll)) || c > 0xFFFF) {
        sCC = u_getCombiningClass(c);
    }
    return sCC;
}

UCollator* ucol_initCollator(const UCATableHeader *image, UCollator *fillIn, const UCollator *UCA, UErrorCode *status) {
    UChar c;
    UCollator *result = fillIn;
    if(U_FAILURE(*status) || image == NULL) {
        return NULL;
    }

    if(result == NULL) {
        result = (UCollator *)uprv_malloc(sizeof(UCollator));
        if(result == NULL) {
            *status = U_MEMORY_ALLOCATION_ERROR;
            return result;
        }
        result->freeOnClose = TRUE;
    } else {
        result->freeOnClose = FALSE;
    }

    result->image = image;
    result->mapping.getFoldingOffset = _getFoldingOffset;
    const uint8_t *mapping = (uint8_t*)result->image+result->image->mappingPosition;
    utrie_unserialize(&result->mapping, mapping, result->image->endExpansionCE - result->image->mappingPosition, status);
    if(U_FAILURE(*status)) {
        if(result->freeOnClose == TRUE) {
            uprv_free(result);
            result = NULL;
        }
        return result;
    }

    result->latinOneMapping = UTRIE_GET32_LATIN1(&result->mapping);
    result->contractionCEs = (uint32_t*)((uint8_t*)result->image+result->image->contractionCEs);
    result->contractionIndex = (UChar*)((uint8_t*)result->image+result->image->contractionIndex);
    result->expansion = (uint32_t*)((uint8_t*)result->image+result->image->expansion);
    result->rules = NULL;
    result->rulesLength = 0;
    result->freeRulesOnClose = FALSE;
    result->reorderCodes = NULL;
    result->reorderCodesLength = 0;
    result->leadBytePermutationTable = NULL;

    /* get the version info from UCATableHeader and populate the Collator struct*/
    result->dataVersion[0] = result->image->version[0]; /* UCA Builder version*/
    result->dataVersion[1] = result->image->version[1]; /* UCA Tailoring rules version*/
    result->dataVersion[2] = 0;
    result->dataVersion[3] = 0;

    result->unsafeCP = (uint8_t *)result->image + result->image->unsafeCP;
    result->minUnsafeCP = 0;
    for (c=0; c<0x300; c++) {  // Find the smallest unsafe char.
        if (ucol_unsafeCP(c, result)) break;
    }
    result->minUnsafeCP = c;

    result->contrEndCP = (uint8_t *)result->image + result->image->contrEndCP;
    result->minContrEndCP = 0;
    for (c=0; c<0x300; c++) {  // Find the Contraction-ending char.
        if (ucol_contractionEndCP(c, result)) break;
    }
    result->minContrEndCP = c;

    /* max expansion tables */
    result->endExpansionCE = (uint32_t*)((uint8_t*)result->image +
                                         result->image->endExpansionCE);
    result->lastEndExpansionCE = result->endExpansionCE +
                                 result->image->endExpansionCECount - 1;
    result->expansionCESize = (uint8_t*)result->image +
                                               result->image->expansionCESize;


    //result->errorCode = *status;

    result->latinOneCEs = NULL;

    result->latinOneRegenTable = FALSE;
    result->latinOneFailed = FALSE;
    result->UCA = UCA;

    /* Normally these will be set correctly later. This is the default if you use UCA or the default. */
    result->ucaRules = NULL;
    result->actualLocale = NULL;
    result->validLocale = NULL;
    result->requestedLocale = NULL;
    result->hasRealData = FALSE; // real data lives in .dat file...
    result->freeImageOnClose = FALSE;

    /* set attributes */
    ucol_setOptionsFromHeader(
        result,
        (UColOptionSet*)((uint8_t*)result->image+result->image->options),
        status);
    result->freeOptionsOnClose = FALSE;

    return result;
}

/* new Mark's code */

/**
 * For generation of Implicit CEs
 * @author Davis
 *
 * Cleaned up so that changes can be made more easily.
 * Old values:
# First Implicit: E26A792D
# Last Implicit: E3DC70C0
# First CJK: E0030300
# Last CJK: E0A9DD00
# First CJK_A: E0A9DF00
# Last CJK_A: E0DE3100
 */
/* Following is a port of Mark's code for new treatment of implicits.
 * It is positioned here, since ucol_initUCA need to initialize the
 * variables below according to the data in the fractional UCA.
 */

/**
 * Function used to:
 * a) collapse the 2 different Han ranges from UCA into one (in the right order), and
 * b) bump any non-CJK characters by 10FFFF.
 * The relevant blocks are:
 * A:    4E00..9FFF; CJK Unified Ideographs
 *       F900..FAFF; CJK Compatibility Ideographs
 * B:    3400..4DBF; CJK Unified Ideographs Extension A
 *       20000..XX;  CJK Unified Ideographs Extension B (and others later on)
 * As long as
 *   no new B characters are allocated between 4E00 and FAFF, and
 *   no new A characters are outside of this range,
 * (very high probability) this simple code will work.
 * The reordered blocks are:
 * Block1 is CJK
 * Block2 is CJK_COMPAT_USED
 * Block3 is CJK_A
 * (all contiguous)
 * Any other CJK gets its normal code point
 * Any non-CJK gets +10FFFF
 * When we reorder Block1, we make sure that it is at the very start,
 * so that it will use a 3-byte form.
 * Warning: the we only pick up the compatibility characters that are
 * NOT decomposed, so that block is smaller!
 */

// CONSTANTS
static const UChar32
    NON_CJK_OFFSET = 0x110000,
    UCOL_MAX_INPUT = 0x220001; // 2 * Unicode range + 2

/**
 * Precomputed by initImplicitConstants()
 */
static int32_t
    final3Multiplier = 0,
    final4Multiplier = 0,
    final3Count = 0,
    final4Count = 0,
    medialCount = 0,
    min3Primary = 0,
    min4Primary = 0,
    max4Primary = 0,
    minTrail = 0,
    maxTrail = 0,
    max3Trail = 0,
    max4Trail = 0,
    min4Boundary = 0;

static const UChar32
    // 4E00;<CJK Ideograph, First>;Lo;0;L;;;;;N;;;;;
    // 9FCB;<CJK Ideograph, Last>;Lo;0;L;;;;;N;;;;;
    CJK_BASE = 0x4E00,
    CJK_LIMIT = 0x9FCB+1,
    // Unified CJK ideographs in the compatibility ideographs block.
    CJK_COMPAT_USED_BASE = 0xFA0E,
    CJK_COMPAT_USED_LIMIT = 0xFA2F+1,
    // 3400;<CJK Ideograph Extension A, First>;Lo;0;L;;;;;N;;;;;
    // 4DB5;<CJK Ideograph Extension A, Last>;Lo;0;L;;;;;N;;;;;
    CJK_A_BASE = 0x3400,
    CJK_A_LIMIT = 0x4DB5+1,
    // 20000;<CJK Ideograph Extension B, First>;Lo;0;L;;;;;N;;;;;
    // 2A6D6;<CJK Ideograph Extension B, Last>;Lo;0;L;;;;;N;;;;;
    CJK_B_BASE = 0x20000,
    CJK_B_LIMIT = 0x2A6D6+1,
    // 2A700;<CJK Ideograph Extension C, First>;Lo;0;L;;;;;N;;;;;
    // 2B734;<CJK Ideograph Extension C, Last>;Lo;0;L;;;;;N;;;;;
    CJK_C_BASE = 0x2A700,
    CJK_C_LIMIT = 0x2B734+1,
    // 2B740;<CJK Ideograph Extension D, First>;Lo;0;L;;;;;N;;;;;
    // 2B81D;<CJK Ideograph Extension D, Last>;Lo;0;L;;;;;N;;;;;
    CJK_D_BASE = 0x2B740,
    CJK_D_LIMIT = 0x2B81D+1;
    // when adding to this list, look for all occurrences (in project)
    // of CJK_C_BASE and CJK_C_LIMIT, etc. to check for code that needs changing!!!!

static UChar32 swapCJK(UChar32 i) {
    if (i < CJK_A_BASE) {
        // non-CJK
    } else if (i < CJK_A_LIMIT) {
        // Extension A has lower code points than the original Unihan+compat
        // but sorts higher.
        return i - CJK_A_BASE
                + (CJK_LIMIT - CJK_BASE)
                + (CJK_COMPAT_USED_LIMIT - CJK_COMPAT_USED_BASE);
    } else if (i < CJK_BASE) {
        // non-CJK
    } else if (i < CJK_LIMIT) {
        return i - CJK_BASE;
    } else if (i < CJK_COMPAT_USED_BASE) {
        // non-CJK
    } else if (i < CJK_COMPAT_USED_LIMIT) {
        return i - CJK_COMPAT_USED_BASE
                + (CJK_LIMIT - CJK_BASE);
    } else if (i < CJK_B_BASE) {
        // non-CJK
    } else if (i < CJK_B_LIMIT) {
        return i; // non-BMP-CJK
    } else if (i < CJK_C_BASE) {
        // non-CJK
    } else if (i < CJK_C_LIMIT) {
        return i; // non-BMP-CJK
    } else if (i < CJK_D_BASE) {
        // non-CJK
    } else if (i < CJK_D_LIMIT) {
        return i; // non-BMP-CJK
    }
    return i + NON_CJK_OFFSET; // non-CJK
}

U_CAPI UChar32 U_EXPORT2
uprv_uca_getRawFromCodePoint(UChar32 i) {
    return swapCJK(i)+1;
}

U_CAPI UChar32 U_EXPORT2
uprv_uca_getCodePointFromRaw(UChar32 i) {
    i--;
    UChar32 result = 0;
    if(i >= NON_CJK_OFFSET) {
        result = i - NON_CJK_OFFSET;
    } else if(i >= CJK_B_BASE) {
        result = i;
    } else if(i < CJK_A_LIMIT + (CJK_LIMIT - CJK_BASE) + (CJK_COMPAT_USED_LIMIT - CJK_COMPAT_USED_BASE)) { // rest of CJKs, compacted
        if(i < CJK_LIMIT - CJK_BASE) {
            result = i + CJK_BASE;
        } else if(i < (CJK_LIMIT - CJK_BASE) + (CJK_COMPAT_USED_LIMIT - CJK_COMPAT_USED_BASE)) {
            result = i + CJK_COMPAT_USED_BASE - (CJK_LIMIT - CJK_BASE);
        } else {
            result = i + CJK_A_BASE - (CJK_LIMIT - CJK_BASE) - (CJK_COMPAT_USED_LIMIT - CJK_COMPAT_USED_BASE);
        }
    } else {
        result = -1;
    }
    return result;
}

// GET IMPLICIT PRIMARY WEIGHTS
// Return value is left justified primary key
U_CAPI uint32_t U_EXPORT2
uprv_uca_getImplicitFromRaw(UChar32 cp) {
    /*
    if (cp < 0 || cp > UCOL_MAX_INPUT) {
        throw new IllegalArgumentException("Code point out of range " + Utility.hex(cp));
    }
    */
    int32_t last0 = cp - min4Boundary;
    if (last0 < 0) {
        int32_t last1 = cp / final3Count;
        last0 = cp % final3Count;

        int32_t last2 = last1 / medialCount;
        last1 %= medialCount;

        last0 = minTrail + last0*final3Multiplier; // spread out, leaving gap at start
        last1 = minTrail + last1; // offset
        last2 = min3Primary + last2; // offset
        /*
        if (last2 >= min4Primary) {
            throw new IllegalArgumentException("4-byte out of range: " + Utility.hex(cp) + ", " + Utility.hex(last2));
        }
        */
        return (last2 << 24) + (last1 << 16) + (last0 << 8);
    } else {
        int32_t last1 = last0 / final4Count;
        last0 %= final4Count;

        int32_t last2 = last1 / medialCount;
        last1 %= medialCount;

        int32_t last3 = last2 / medialCount;
        last2 %= medialCount;

        last0 = minTrail + last0*final4Multiplier; // spread out, leaving gap at start
        last1 = minTrail + last1; // offset
        last2 = minTrail + last2; // offset
        last3 = min4Primary + last3; // offset
        /*
        if (last3 > max4Primary) {
            throw new IllegalArgumentException("4-byte out of range: " + Utility.hex(cp) + ", " + Utility.hex(last3));
        }
        */
        return (last3 << 24) + (last2 << 16) + (last1 << 8) + last0;
    }
}

static uint32_t U_EXPORT2
uprv_uca_getImplicitPrimary(UChar32 cp) {
   //fprintf(stdout, "Incoming: %04x\n", cp);
    //if (DEBUG) System.out.println("Incoming: " + Utility.hex(cp));

    cp = swapCJK(cp);
    cp++;
    // we now have a range of numbers from 0 to 21FFFF.

    //if (DEBUG) System.out.println("CJK swapped: " + Utility.hex(cp));
    //fprintf(stdout, "CJK swapped: %04x\n", cp);

    return uprv_uca_getImplicitFromRaw(cp);
}

/**
 * Converts implicit CE into raw integer ("code point")
 * @param implicit
 * @return -1 if illegal format
 */
U_CAPI UChar32 U_EXPORT2
uprv_uca_getRawFromImplicit(uint32_t implicit) {
    UChar32 result;
    UChar32 b3 = implicit & 0xFF;
    UChar32 b2 = (implicit >> 8) & 0xFF;
    UChar32 b1 = (implicit >> 16) & 0xFF;
    UChar32 b0 = (implicit >> 24) & 0xFF;

    // simple parameter checks
    if (b0 < min3Primary || b0 > max4Primary
        || b1 < minTrail || b1 > maxTrail)
        return -1;
    // normal offsets
    b1 -= minTrail;

    // take care of the final values, and compose
    if (b0 < min4Primary) {
        if (b2 < minTrail || b2 > max3Trail || b3 != 0)
            return -1;
        b2 -= minTrail;
        UChar32 remainder = b2 % final3Multiplier;
        if (remainder != 0)
            return -1;
        b0 -= min3Primary;
        b2 /= final3Multiplier;
        result = ((b0 * medialCount) + b1) * final3Count + b2;
    } else {
        if (b2 < minTrail || b2 > maxTrail
            || b3 < minTrail || b3 > max4Trail)
            return -1;
        b2 -= minTrail;
        b3 -= minTrail;
        UChar32 remainder = b3 % final4Multiplier;
        if (remainder != 0)
            return -1;
        b3 /= final4Multiplier;
        b0 -= min4Primary;
        result = (((b0 * medialCount) + b1) * medialCount + b2) * final4Count + b3 + min4Boundary;
    }
    // final check
    if (result < 0 || result > UCOL_MAX_INPUT)
        return -1;
    return result;
}


static inline int32_t divideAndRoundUp(int a, int b) {
    return 1 + (a-1)/b;
}

/* this function is either called from initUCA or from genUCA before
 * doing canonical closure for the UCA.
 */

/**
 * Set up to generate implicits.
 * Maintenance Note:  this function may end up being called more than once, due
 *                    to threading races during initialization.  Make sure that
 *                    none of the Constants is ever transiently assigned an
 *                    incorrect value.
 * @param minPrimary
 * @param maxPrimary
 * @param minTrail final byte
 * @param maxTrail final byte
 * @param gap3 the gap we leave for tailoring for 3-byte forms
 * @param gap4 the gap we leave for tailoring for 4-byte forms
 */
static void initImplicitConstants(int minPrimary, int maxPrimary,
                                    int minTrailIn, int maxTrailIn,
                                    int gap3, int primaries3count,
                                    UErrorCode *status) {
    // some simple parameter checks
    if ((minPrimary < 0 || minPrimary >= maxPrimary || maxPrimary > 0xFF)
        || (minTrailIn < 0 || minTrailIn >= maxTrailIn || maxTrailIn > 0xFF)
        || (primaries3count < 1))
    {
        *status = U_ILLEGAL_ARGUMENT_ERROR;
        return;
    };

    minTrail = minTrailIn;
    maxTrail = maxTrailIn;

    min3Primary = minPrimary;
    max4Primary = maxPrimary;
    // compute constants for use later.
    // number of values we can use in trailing bytes
    // leave room for empty values between AND above, e.g. if gap = 2
    // range 3..7 => +3 -4 -5 -6 -7: so 1 value
    // range 3..8 => +3 -4 -5 +6 -7 -8: so 2 values
    // range 3..9 => +3 -4 -5 +6 -7 -8 -9: so 2 values
    final3Multiplier = gap3 + 1;
    final3Count = (maxTrail - minTrail + 1) / final3Multiplier;
    max3Trail = minTrail + (final3Count - 1) * final3Multiplier;

    // medials can use full range
    medialCount = (maxTrail - minTrail + 1);
    // find out how many values fit in each form
    int32_t threeByteCount = medialCount * final3Count;
    // now determine where the 3/4 boundary is.
    // we use 3 bytes below the boundary, and 4 above
    int32_t primariesAvailable = maxPrimary - minPrimary + 1;
    int32_t primaries4count = primariesAvailable - primaries3count;


    int32_t min3ByteCoverage = primaries3count * threeByteCount;
    min4Primary = minPrimary + primaries3count;
    min4Boundary = min3ByteCoverage;
    // Now expand out the multiplier for the 4 bytes, and redo.

    int32_t totalNeeded = UCOL_MAX_INPUT - min4Boundary;
    int32_t neededPerPrimaryByte = divideAndRoundUp(totalNeeded, primaries4count);
    int32_t neededPerFinalByte = divideAndRoundUp(neededPerPrimaryByte, medialCount * medialCount);
    int32_t gap4 = (maxTrail - minTrail - 1) / neededPerFinalByte;
    if (gap4 < 1) {
        *status = U_ILLEGAL_ARGUMENT_ERROR;
        return;
    }
    final4Multiplier = gap4 + 1;
    final4Count = neededPerFinalByte;
    max4Trail = minTrail + (final4Count - 1) * final4Multiplier;
}

    /**
     * Supply parameters for generating implicit CEs
     */
U_CAPI void U_EXPORT2
uprv_uca_initImplicitConstants(UErrorCode *status) {
    // 13 is the largest 4-byte gap we can use without getting 2 four-byte forms.
    //initImplicitConstants(minPrimary, maxPrimary, 0x04, 0xFE, 1, 1, status);
    initImplicitConstants(minImplicitPrimary, maxImplicitPrimary, 0x04, 0xFE, 1, 1, status);
}


/*    collIterNormalize     Incremental Normalization happens here.                       */
/*                          pick up the range of chars identifed by FCD,                  */
/*                          normalize it into the collIterate's writable buffer,          */
/*                          switch the collIterate's state to use the writable buffer.    */
/*                                                                                        */
static
void collIterNormalize(collIterate *collationSource)
{
    UErrorCode  status = U_ZERO_ERROR;
    const UChar *srcP = collationSource->pos - 1;      /*  Start of chars to normalize    */
    const UChar *endP = collationSource->fcdPosition;  /* End of region to normalize+1    */

    collationSource->nfd->normalize(UnicodeString(FALSE, srcP, (int32_t)(endP - srcP)),
                                    collationSource->writableBuffer,
                                    status);
    if (U_FAILURE(status)) {
#ifdef UCOL_DEBUG
        fprintf(stderr, "collIterNormalize(), NFD failed, status = %s\n", u_errorName(status));
#endif
        return;
    }

    collationSource->pos        = collationSource->writableBuffer.getTerminatedBuffer();
    collationSource->origFlags  = collationSource->flags;
    collationSource->flags     |= UCOL_ITER_INNORMBUF;
    collationSource->flags     &= ~(UCOL_ITER_NORM | UCOL_ITER_HASLEN | UCOL_USE_ITERATOR);
}


// This function takes the iterator and extracts normalized stuff up to the next boundary
// It is similar in the end results to the collIterNormalize, but for the cases when we
// use an iterator
/*static
inline void normalizeIterator(collIterate *collationSource) {
  UErrorCode status = U_ZERO_ERROR;
  UBool wasNormalized = FALSE;
  //int32_t iterIndex = collationSource->iterator->getIndex(collationSource->iterator, UITER_CURRENT);
  uint32_t iterIndex = collationSource->iterator->getState(collationSource->iterator);
  int32_t normLen = unorm_next(collationSource->iterator, collationSource->writableBuffer,
    (int32_t)collationSource->writableBufSize, UNORM_FCD, 0, TRUE, &wasNormalized, &status);
  if(status == U_BUFFER_OVERFLOW_ERROR || normLen == (int32_t)collationSource->writableBufSize) {
    // reallocate and terminate
    if(!u_growBufferFromStatic(collationSource->stackWritableBuffer,
                               &collationSource->writableBuffer,
                               (int32_t *)&collationSource->writableBufSize, normLen + 1,
                               0)
    ) {
    #ifdef UCOL_DEBUG
        fprintf(stderr, "normalizeIterator(), out of memory\n");
    #endif
        return;
    }
    status = U_ZERO_ERROR;
    //collationSource->iterator->move(collationSource->iterator, iterIndex, UITER_ZERO);
    collationSource->iterator->setState(collationSource->iterator, iterIndex, &status);
    normLen = unorm_next(collationSource->iterator, collationSource->writableBuffer,
    (int32_t)collationSource->writableBufSize, UNORM_FCD, 0, TRUE, &wasNormalized, &status);
  }
  // Terminate the buffer - we already checked that it is big enough
  collationSource->writableBuffer[normLen] = 0;
  if(collationSource->writableBuffer != collationSource->stackWritableBuffer) {
      collationSource->flags |= UCOL_ITER_ALLOCATED;
  }
  collationSource->pos        = collationSource->writableBuffer;
  collationSource->origFlags  = collationSource->flags;
  collationSource->flags     |= UCOL_ITER_INNORMBUF;
  collationSource->flags     &= ~(UCOL_ITER_NORM | UCOL_ITER_HASLEN | UCOL_USE_ITERATOR);
}*/


/* Incremental FCD check and normalize                                                    */
/*   Called from getNextCE when normalization state is suspect.                           */
/*   When entering, the state is known to be this:                                        */
/*      o   We are working in the main buffer of the collIterate, not the side            */
/*          writable buffer.  When in the side buffer, normalization mode is always off,  */
/*          so we won't get here.                                                         */
/*      o   The leading combining class from the current character is 0 or                */
/*          the trailing combining class of the previous char was zero.                   */
/*          True because the previous call to this function will have always exited       */
/*          that way, and we get called for every char where cc might be non-zero.        */
static
inline UBool collIterFCD(collIterate *collationSource) {
    const UChar *srcP, *endP;
    uint8_t     leadingCC;
    uint8_t     prevTrailingCC = 0;
    uint16_t    fcd;
    UBool       needNormalize = FALSE;

    srcP = collationSource->pos-1;

    if (collationSource->flags & UCOL_ITER_HASLEN) {
        endP = collationSource->endp;
    } else {
        endP = NULL;
    }

    // Get the trailing combining class of the current character.  If it's zero,
    //   we are OK.
    /* trie access */
    fcd = unorm_nextFCD16(fcdTrieIndex, fcdHighStart, srcP, endP);
    if (fcd != 0) {
        prevTrailingCC = (uint8_t)(fcd & LAST_BYTE_MASK_);

        if (prevTrailingCC != 0) {
            // The current char has a non-zero trailing CC.  Scan forward until we find
            //   a char with a leading cc of zero.
            while (endP == NULL || srcP != endP)
            {
                const UChar *savedSrcP = srcP;

                /* trie access */
                fcd = unorm_nextFCD16(fcdTrieIndex, fcdHighStart, srcP, endP);
                leadingCC = (uint8_t)(fcd >> SECOND_LAST_BYTE_SHIFT_);
                if (leadingCC == 0) {
                    srcP = savedSrcP;      // Hit char that is not part of combining sequence.
                                           //   back up over it.  (Could be surrogate pair!)
                    break;
                }

                if (leadingCC < prevTrailingCC) {
                    needNormalize = TRUE;
                }

                prevTrailingCC = (uint8_t)(fcd & LAST_BYTE_MASK_);
            }
        }
    }

    collationSource->fcdPosition = (UChar *)srcP;

    return needNormalize;
}

/****************************************************************************/
/* Following are the CE retrieval functions                                 */
/*                                                                          */
/****************************************************************************/

static uint32_t getImplicit(UChar32 cp, collIterate *collationSource);
static uint32_t getPrevImplicit(UChar32 cp, collIterate *collationSource);

/* there should be a macro version of this function in the header file */
/* This is the first function that tries to fetch a collation element  */
/* If it's not succesfull or it encounters a more difficult situation  */
/* some more sofisticated and slower functions are invoked             */
static
inline uint32_t ucol_IGetNextCE(const UCollator *coll, collIterate *collationSource, UErrorCode *status) {
    uint32_t order = 0;
    if (collationSource->CEpos > collationSource->toReturn) {       /* Are there any CEs from previous expansions? */
        order = *(collationSource->toReturn++);                         /* if so, return them */
        if(collationSource->CEpos == collationSource->toReturn) {
            collationSource->CEpos = collationSource->toReturn = collationSource->extendCEs ? collationSource->extendCEs : collationSource->CEs;
        }
        return order;
    }

    UChar ch = 0;
    collationSource->offsetReturn = NULL;

    do {
        for (;;)                           /* Loop handles case when incremental normalize switches   */
        {                                  /*   to or from the side buffer / original string, and we  */
            /*   need to start again to get the next character.        */

            if ((collationSource->flags & (UCOL_ITER_HASLEN | UCOL_ITER_INNORMBUF | UCOL_ITER_NORM | UCOL_HIRAGANA_Q | UCOL_USE_ITERATOR)) == 0)
            {
                // The source string is null terminated and we're not working from the side buffer,
                //   and we're not normalizing.  This is the fast path.
                //   (We can be in the side buffer for Thai pre-vowel reordering even when not normalizing.)
                ch = *collationSource->pos++;
                if (ch != 0) {
                    break;
                }
                else {
                    return UCOL_NO_MORE_CES;
                }
            }

            if (collationSource->flags & UCOL_ITER_HASLEN) {
                // Normal path for strings when length is specified.
                //   (We can't be in side buffer because it is always null terminated.)
                if (collationSource->pos >= collationSource->endp) {
                    // Ran off of the end of the main source string.  We're done.
                    return UCOL_NO_MORE_CES;
                }
                ch = *collationSource->pos++;
            }
            else if(collationSource->flags & UCOL_USE_ITERATOR) {
                UChar32 iterCh = collationSource->iterator->next(collationSource->iterator);
                if(iterCh == U_SENTINEL) {
                    return UCOL_NO_MORE_CES;
                }
                ch = (UChar)iterCh;
            }
            else
            {
                // Null terminated string.
                ch = *collationSource->pos++;
                if (ch == 0) {
                    // Ran off end of buffer.
                    if ((collationSource->flags & UCOL_ITER_INNORMBUF) == 0) {
                        // Ran off end of main string. backing up one character.
                        collationSource->pos--;
                        return UCOL_NO_MORE_CES;
                    }
                    else
                    {
                        // Hit null in the normalize side buffer.
                        // Usually this means the end of the normalized data,
                        // except for one odd case: a null followed by combining chars,
                        //   which is the case if we are at the start of the buffer.
                        if (collationSource->pos == collationSource->writableBuffer.getBuffer()+1) {
                            break;
                        }

                        //  Null marked end of side buffer.
                        //   Revert to the main string and
                        //   loop back to top to try again to get a character.
                        collationSource->pos   = collationSource->fcdPosition;
                        collationSource->flags = collationSource->origFlags;
                        continue;
                    }
                }
            }

            if(collationSource->flags&UCOL_HIRAGANA_Q) {
                /* Codepoints \u3099-\u309C are both Hiragana and Katakana. Set the flag
                 * based on whether the previous codepoint was Hiragana or Katakana.
                 */
                if(((ch>=0x3040 && ch<=0x3096) || (ch >= 0x309d && ch <= 0x309f)) ||
                        ((collationSource->flags & UCOL_WAS_HIRAGANA) && (ch >= 0x3099 && ch <= 0x309C))) {
                    collationSource->flags |= UCOL_WAS_HIRAGANA;
                } else {
                    collationSource->flags &= ~UCOL_WAS_HIRAGANA;
                }
            }

            // We've got a character.  See if there's any fcd and/or normalization stuff to do.
            //    Note that UCOL_ITER_NORM flag is always zero when we are in the side buffer.
            if ((collationSource->flags & UCOL_ITER_NORM) == 0) {
                break;
            }

            if (collationSource->fcdPosition >= collationSource->pos) {
                // An earlier FCD check has already covered the current character.
                // We can go ahead and process this char.
                break;
            }

            if (ch < ZERO_CC_LIMIT_ ) {
                // Fast fcd safe path.  Trailing combining class == 0.  This char is OK.
                break;
            }

            if (ch < NFC_ZERO_CC_BLOCK_LIMIT_) {
                // We need to peek at the next character in order to tell if we are FCD
                if ((collationSource->flags & UCOL_ITER_HASLEN) && collationSource->pos >= collationSource->endp) {
                    // We are at the last char of source string.
                    //  It is always OK for FCD check.
                    break;
                }

                // Not at last char of source string (or we'll check against terminating null).  Do the FCD fast test
                if (*collationSource->pos < NFC_ZERO_CC_BLOCK_LIMIT_) {
                    break;
                }
            }


            // Need a more complete FCD check and possible normalization.
            if (collIterFCD(collationSource)) {
                collIterNormalize(collationSource);
            }
            if ((collationSource->flags & UCOL_ITER_INNORMBUF) == 0) {
                //  No normalization was needed.  Go ahead and process the char we already had.
                break;
            }

            // Some normalization happened.  Next loop iteration will pick up a char
            //   from the normalization buffer.

        }   // end for (;;)


        if (ch <= 0xFF) {
            /*  For latin-1 characters we never need to fall back to the UCA table        */
            /*    because all of the UCA data is replicated in the latinOneMapping array  */
            order = coll->latinOneMapping[ch];
            if (order > UCOL_NOT_FOUND) {
                order = ucol_prv_getSpecialCE(coll, ch, order, collationSource, status);
            }
        }
        else
        {
            // Always use UCA for Han, Hangul
            // (Han extension A is before main Han block)
            // **** Han compatibility chars ?? ****
            if ((collationSource->flags & UCOL_FORCE_HAN_IMPLICIT) != 0 &&
                (ch >= UCOL_FIRST_HAN_A && ch <= UCOL_LAST_HANGUL)) {
                if (ch > UCOL_LAST_HAN && ch < UCOL_FIRST_HANGUL) {
                    // between the two target ranges; do normal lookup
                    // **** this range is YI, Modifier tone letters, ****
                    // **** Latin-D, Syloti Nagari, Phagas-pa.       ****
                    // **** Latin-D might be tailored, so we need to ****
                    // **** do the normal lookup for these guys.     ****
                    order = UTRIE_GET32_FROM_LEAD(&coll->mapping, ch);
                } else {
                    // in one of the target ranges; use UCA
                    order = UCOL_NOT_FOUND;
                }
            } else {
                order = UTRIE_GET32_FROM_LEAD(&coll->mapping, ch);
            }

            if(order > UCOL_NOT_FOUND) {                                       /* if a CE is special                */
                order = ucol_prv_getSpecialCE(coll, ch, order, collationSource, status);    /* and try to get the special CE     */
            }

            if(order == UCOL_NOT_FOUND && coll->UCA) {   /* We couldn't find a good CE in the tailoring */
                /* if we got here, the codepoint MUST be over 0xFF - so we look directly in the trie */
                order = UTRIE_GET32_FROM_LEAD(&coll->UCA->mapping, ch);

                if(order > UCOL_NOT_FOUND) { /* UCA also gives us a special CE */
                    order = ucol_prv_getSpecialCE(coll->UCA, ch, order, collationSource, status);
                }
            }
        }
    } while ( order == UCOL_IGNORABLE && ch >= UCOL_FIRST_HANGUL && ch <= UCOL_LAST_HANGUL );

    if(order == UCOL_NOT_FOUND) {
        order = getImplicit(ch, collationSource);
    }
    return order; /* return the CE */
}

/* ucol_getNextCE, out-of-line version for use from other files.   */
U_CAPI uint32_t  U_EXPORT2
ucol_getNextCE(const UCollator *coll, collIterate *collationSource, UErrorCode *status) {
    return ucol_IGetNextCE(coll, collationSource, status);
}


/**
* Incremental previous normalization happens here. Pick up the range of chars
* identifed by FCD, normalize it into the collIterate's writable buffer,
* switch the collIterate's state to use the writable buffer.
* @param data collation iterator data
*/
static
void collPrevIterNormalize(collIterate *data)
{
    UErrorCode status  = U_ZERO_ERROR;
    const UChar *pEnd   = data->pos;  /* End normalize + 1 */
    const UChar *pStart;

    /* Start normalize */
    if (data->fcdPosition == NULL) {
        pStart = data->string;
    }
    else {
        pStart = data->fcdPosition + 1;
    }

    int32_t normLen =
        data->nfd->normalize(UnicodeString(FALSE, pStart, (int32_t)((pEnd - pStart) + 1)),
                             data->writableBuffer,
                             status).
        length();
    if(U_FAILURE(status)) {
        return;
    }
    /*
    this puts the null termination infront of the normalized string instead
    of the end
    */
    data->writableBuffer.insert(0, (UChar)0);

    /*
     * The usual case at this point is that we've got a base
     * character followed by marks that were normalized. If
     * fcdPosition is NULL, that means that we backed up to
     * the beginning of the string and there's no base character.
     *
     * Forward processing will usually normalize when it sees
     * the first mark, so that mark will get it's natural offset
     * and the rest will get the offset of the character following
     * the marks. The base character will also get its natural offset.
     *
     * We write the offset of the base character, if there is one,
     * followed by the offset of the first mark and then the offsets
     * of the rest of the marks.
     */
    int32_t firstMarkOffset = 0;
    int32_t trailOffset     = (int32_t)(data->pos - data->string + 1);
    int32_t trailCount      = normLen - 1;

    if (data->fcdPosition != NULL) {
        int32_t baseOffset = (int32_t)(data->fcdPosition - data->string);
        UChar   baseChar   = *data->fcdPosition;

        firstMarkOffset = baseOffset + 1;

        /*
         * If the base character is the start of a contraction, forward processing
         * will normalize the marks while checking for the contraction, which means
         * that the offset of the first mark will the same as the other marks.
         *
         * **** THIS IS PROBABLY NOT A COMPLETE TEST ****
         */
        if (baseChar >= 0x100) {
            uint32_t baseOrder = UTRIE_GET32_FROM_LEAD(&data->coll->mapping, baseChar);

            if (baseOrder == UCOL_NOT_FOUND && data->coll->UCA) {
                baseOrder = UTRIE_GET32_FROM_LEAD(&data->coll->UCA->mapping, baseChar);
            }

            if (baseOrder > UCOL_NOT_FOUND && getCETag(baseOrder) == CONTRACTION_TAG) {
                firstMarkOffset = trailOffset;
            }
        }

        data->appendOffset(baseOffset, status);
    }

    data->appendOffset(firstMarkOffset, status);

    for (int32_t i = 0; i < trailCount; i += 1) {
        data->appendOffset(trailOffset, status);
    }

    data->offsetRepeatValue = trailOffset;

    data->offsetReturn = data->offsetStore - 1;
    if (data->offsetReturn == data->offsetBuffer) {
        data->offsetStore = data->offsetBuffer;
    }

    data->pos        = data->writableBuffer.getTerminatedBuffer() + 1 + normLen;
    data->origFlags  = data->flags;
    data->flags     |= UCOL_ITER_INNORMBUF;
    data->flags     &= ~(UCOL_ITER_NORM | UCOL_ITER_HASLEN);
}


/**
* Incremental FCD check for previous iteration and normalize. Called from
* getPrevCE when normalization state is suspect.
* When entering, the state is known to be this:
* o  We are working in the main buffer of the collIterate, not the side
*    writable buffer. When in the side buffer, normalization mode is always
*    off, so we won't get here.
* o  The leading combining class from the current character is 0 or the
*    trailing combining class of the previous char was zero.
*    True because the previous call to this function will have always exited
*    that way, and we get called for every char where cc might be non-zero.
* @param data collation iterate struct
* @return normalization status, TRUE for normalization to be done, FALSE
*         otherwise
*/
static
inline UBool collPrevIterFCD(collIterate *data)
{
    const UChar *src, *start;
    uint8_t     leadingCC;
    uint8_t     trailingCC = 0;
    uint16_t    fcd;
    UBool       result = FALSE;

    start = data->string;
    src = data->pos + 1;

    /* Get the trailing combining class of the current character. */
    fcd = unorm_prevFCD16(fcdTrieIndex, fcdHighStart, start, src);

    leadingCC = (uint8_t)(fcd >> SECOND_LAST_BYTE_SHIFT_);

    if (leadingCC != 0) {
        /*
        The current char has a non-zero leading combining class.
        Scan backward until we find a char with a trailing cc of zero.
        */
        for (;;)
        {
            if (start == src) {
                data->fcdPosition = NULL;
                return result;
            }

            fcd = unorm_prevFCD16(fcdTrieIndex, fcdHighStart, start, src);

            trailingCC = (uint8_t)(fcd & LAST_BYTE_MASK_);

            if (trailingCC == 0) {
                break;
            }

            if (leadingCC < trailingCC) {
                result = TRUE;
            }

            leadingCC = (uint8_t)(fcd >> SECOND_LAST_BYTE_SHIFT_);
        }
    }

    data->fcdPosition = (UChar *)src;

    return result;
}

/** gets a code unit from the string at a given offset
 *  Handles both normal and iterative cases.
 *  No error checking - caller beware!
 */
static inline
UChar peekCodeUnit(collIterate *source, int32_t offset) {
    if(source->pos != NULL) {
        return *(source->pos + offset);
    } else if(source->iterator != NULL) {
        UChar32 c;
        if(offset != 0) {
            source->iterator->move(source->iterator, offset, UITER_CURRENT);
            c = source->iterator->next(source->iterator);
            source->iterator->move(source->iterator, -offset-1, UITER_CURRENT);
        } else {
            c = source->iterator->current(source->iterator);
        }
        return c >= 0 ? (UChar)c : 0xfffd;  // If the caller works properly, we should never see c<0.
    } else {
        return 0xfffd;
    }
}

// Code point version. Treats the offset as a _code point_ delta.
// We cannot use U16_FWD_1_UNSAFE and similar because we might not have well-formed UTF-16.
// We cannot use U16_FWD_1 and similar because we do not know the start and limit of the buffer.
static inline
UChar32 peekCodePoint(collIterate *source, int32_t offset) {
    UChar32 c;
    if(source->pos != NULL) {
        const UChar *p = source->pos;
        if(offset >= 0) {
            // Skip forward over (offset-1) code points.
            while(--offset >= 0) {
                if(U16_IS_LEAD(*p++) && U16_IS_TRAIL(*p)) {
                    ++p;
                }
            }
            // Read the code point there.
            c = *p++;
            UChar trail;
            if(U16_IS_LEAD(c) && U16_IS_TRAIL(trail = *p)) {
                c = U16_GET_SUPPLEMENTARY(c, trail);
            }
        } else /* offset<0 */ {
            // Skip backward over (offset-1) code points.
            while(++offset < 0) {
                if(U16_IS_TRAIL(*--p) && U16_IS_LEAD(*(p - 1))) {
                    --p;
                }
            }
            // Read the code point before that.
            c = *--p;
            UChar lead;
            if(U16_IS_TRAIL(c) && U16_IS_LEAD(lead = *(p - 1))) {
                c = U16_GET_SUPPLEMENTARY(lead, c);
            }
        }
    } else if(source->iterator != NULL) {
        if(offset >= 0) {
            // Skip forward over (offset-1) code points.
            int32_t fwd = offset;
            while(fwd-- > 0) {
                uiter_next32(source->iterator);
            }
            // Read the code point there.
            c = uiter_current32(source->iterator);
            // Return to the starting point, skipping backward over (offset-1) code points.
            while(offset-- > 0) {
                uiter_previous32(source->iterator);
            }
        } else /* offset<0 */ {
            // Read backward, reading offset code points, remember only the last-read one.
            int32_t back = offset;
            do {
                c = uiter_previous32(source->iterator);
            } while(++back < 0);
            // Return to the starting position, skipping forward over offset code points.
            do {
                uiter_next32(source->iterator);
            } while(++offset < 0);
        }
    } else {
        c = U_SENTINEL;
    }
    return c;
}

/**
* Determines if we are at the start of the data string in the backwards
* collation iterator
* @param data collation iterator
* @return TRUE if we are at the start
*/
static
inline UBool isAtStartPrevIterate(collIterate *data) {
    if(data->pos == NULL && data->iterator != NULL) {
        return !data->iterator->hasPrevious(data->iterator);
    }
    //return (collIter_bos(data)) ||
    return (data->pos == data->string) ||
              ((data->flags & UCOL_ITER_INNORMBUF) &&
              *(data->pos - 1) == 0 && data->fcdPosition == NULL);
}

static
inline void goBackOne(collIterate *data) {
# if 0
    // somehow, it looks like we need to keep iterator synced up
    // at all times, as above.
    if(data->pos) {
        data->pos--;
    }
    if(data->iterator) {
        data->iterator->previous(data->iterator);
    }
#endif
    if(data->iterator && (data->flags & UCOL_USE_ITERATOR)) {
        data->iterator->previous(data->iterator);
    }
    if(data->pos) {
        data->pos --;
    }
}

/**
* Inline function that gets a simple CE.
* So what it does is that it will first check the expansion buffer. If the
* expansion buffer is not empty, ie the end pointer to the expansion buffer
* is different from the string pointer, we return the collation element at the
* return pointer and decrement it.
* For more complicated CEs it resorts to getComplicatedCE.
* @param coll collator data
* @param data collation iterator struct
* @param status error status
*/
static
inline uint32_t ucol_IGetPrevCE(const UCollator *coll, collIterate *data,
                               UErrorCode *status)
{
    uint32_t result = (uint32_t)UCOL_NULLORDER;

    if (data->offsetReturn != NULL) {
        if (data->offsetRepeatCount > 0) {
                data->offsetRepeatCount -= 1;
        } else {
            if (data->offsetReturn == data->offsetBuffer) {
                data->offsetReturn = NULL;
                data->offsetStore  = data->offsetBuffer;
            } else {
                data->offsetReturn -= 1;
            }
        }
    }

    if ((data->extendCEs && data->toReturn > data->extendCEs) ||
            (!data->extendCEs && data->toReturn > data->CEs))
    {
        data->toReturn -= 1;
        result = *(data->toReturn);
        if (data->CEs == data->toReturn || data->extendCEs == data->toReturn) {
            data->CEpos = data->toReturn;
        }
    }
    else {
        UChar ch = 0;

        do {
            /*
            Loop handles case when incremental normalize switches to or from the
            side buffer / original string, and we need to start again to get the
            next character.
            */
            for (;;) {
                if (data->flags & UCOL_ITER_HASLEN) {
                    /*
                    Normal path for strings when length is specified.
                    Not in side buffer because it is always null terminated.
                    */
                    if (data->pos <= data->string) {
                        /* End of the main source string */
                        return UCOL_NO_MORE_CES;
                    }
                    data->pos --;
                    ch = *data->pos;
                }
                // we are using an iterator to go back. Pray for us!
                else if (data->flags & UCOL_USE_ITERATOR) {
                  UChar32 iterCh = data->iterator->previous(data->iterator);
                  if(iterCh == U_SENTINEL) {
                    return UCOL_NO_MORE_CES;
                  } else {
                    ch = (UChar)iterCh;
                  }
                }
                else {
                    data->pos --;
                    ch = *data->pos;
                    /* we are in the side buffer. */
                    if (ch == 0) {
                        /*
                        At the start of the normalize side buffer.
                        Go back to string.
                        Because pointer points to the last accessed character,
                        hence we have to increment it by one here.
                        */
                        data->flags = data->origFlags;
                        data->offsetRepeatValue = 0;

                         if (data->fcdPosition == NULL) {
                            data->pos = data->string;
                            return UCOL_NO_MORE_CES;
                        }
                        else {
                            data->pos   = data->fcdPosition + 1;
                        }

                       continue;
                    }
                }

                if(data->flags&UCOL_HIRAGANA_Q) {
                  if(ch>=0x3040 && ch<=0x309f) {
                    data->flags |= UCOL_WAS_HIRAGANA;
                  } else {
                    data->flags &= ~UCOL_WAS_HIRAGANA;
                  }
                }

                /*
                * got a character to determine if there's fcd and/or normalization
                * stuff to do.
                * if the current character is not fcd.
                * if current character is at the start of the string
                * Trailing combining class == 0.
                * Note if pos is in the writablebuffer, norm is always 0
                */
                if (ch < ZERO_CC_LIMIT_ ||
                  // this should propel us out of the loop in the iterator case
                    (data->flags & UCOL_ITER_NORM) == 0 ||
                    (data->fcdPosition != NULL && data->fcdPosition <= data->pos)
                    || data->string == data->pos) {
                    break;
                }

                if (ch < NFC_ZERO_CC_BLOCK_LIMIT_) {
                    /* if next character is FCD */
                    if (data->pos == data->string) {
                        /* First char of string is always OK for FCD check */
                        break;
                    }

                    /* Not first char of string, do the FCD fast test */
                    if (*(data->pos - 1) < NFC_ZERO_CC_BLOCK_LIMIT_) {
                        break;
                    }
                }

                /* Need a more complete FCD check and possible normalization. */
                if (collPrevIterFCD(data)) {
                    collPrevIterNormalize(data);
                }

                if ((data->flags & UCOL_ITER_INNORMBUF) == 0) {
                    /*  No normalization. Go ahead and process the char. */
                    break;
                }

                /*
                Some normalization happened.
                Next loop picks up a char from the normalization buffer.
                */
            }

            /* attempt to handle contractions, after removal of the backwards
            contraction
            */
            if (ucol_contractionEndCP(ch, coll) && !isAtStartPrevIterate(data)) {
                result = ucol_prv_getSpecialPrevCE(coll, ch, UCOL_CONTRACTION, data, status);
            } else {
                if (ch <= 0xFF) {
                    result = coll->latinOneMapping[ch];
                }
                else {
                    // Always use UCA for [3400..9FFF], [AC00..D7AF]
                    // **** [FA0E..FA2F] ?? ****
                    if ((data->flags & UCOL_FORCE_HAN_IMPLICIT) != 0 &&
                        (ch >= 0x3400 && ch <= 0xD7AF)) {
                        if (ch > 0x9FFF && ch < 0xAC00) {
                            // between the two target ranges; do normal lookup
                            // **** this range is YI, Modifier tone letters, ****
                            // **** Latin-D, Syloti Nagari, Phagas-pa.       ****
                            // **** Latin-D might be tailored, so we need to ****
                            // **** do the normal lookup for these guys.     ****
                             result = UTRIE_GET32_FROM_LEAD(&coll->mapping, ch);
                        } else {
                            result = UCOL_NOT_FOUND;
                        }
                    } else {
                        result = UTRIE_GET32_FROM_LEAD(&coll->mapping, ch);
                    }
                }
                if (result > UCOL_NOT_FOUND) {
                    result = ucol_prv_getSpecialPrevCE(coll, ch, result, data, status);
                }
                if (result == UCOL_NOT_FOUND) { // Not found in master list
                    if (!isAtStartPrevIterate(data) &&
                        ucol_contractionEndCP(ch, data->coll))
                    {
                        result = UCOL_CONTRACTION;
                    } else {
                        if(coll->UCA) {
                            result = UTRIE_GET32_FROM_LEAD(&coll->UCA->mapping, ch);
                        }
                    }

                    if (result > UCOL_NOT_FOUND) {
                        if(coll->UCA) {
                            result = ucol_prv_getSpecialPrevCE(coll->UCA, ch, result, data, status);
                        }
                    }
                }
            }
        } while ( result == UCOL_IGNORABLE && ch >= UCOL_FIRST_HANGUL && ch <= UCOL_LAST_HANGUL );

        if(result == UCOL_NOT_FOUND) {
            result = getPrevImplicit(ch, data);
        }
    }

    return result;
}


/*   ucol_getPrevCE, out-of-line version for use from other files.  */
U_CFUNC uint32_t  U_EXPORT2
ucol_getPrevCE(const UCollator *coll, collIterate *data,
                        UErrorCode *status) {
    return ucol_IGetPrevCE(coll, data, status);
}


/* this should be connected to special Jamo handling */
U_CFUNC uint32_t  U_EXPORT2
ucol_getFirstCE(const UCollator *coll, UChar u, UErrorCode *status) {
    collIterate colIt;
    IInit_collIterate(coll, &u, 1, &colIt, status);
    if(U_FAILURE(*status)) {
        return 0;
    }
    return ucol_IGetNextCE(coll, &colIt, status);
}

/**
* Inserts the argument character into the end of the buffer pushing back the
* null terminator.
* @param data collIterate struct data
* @param ch character to be appended
* @return the position of the new addition
*/
static
inline const UChar * insertBufferEnd(collIterate *data, UChar ch)
{
    int32_t oldLength = data->writableBuffer.length();
    return data->writableBuffer.append(ch).getTerminatedBuffer() + oldLength;
}

/**
* Inserts the argument string into the end of the buffer pushing back the
* null terminator.
* @param data collIterate struct data
* @param string to be appended
* @param length of the string to be appended
* @return the position of the new addition
*/
static
inline const UChar * insertBufferEnd(collIterate *data, const UChar *str, int32_t length)
{
    int32_t oldLength = data->writableBuffer.length();
    return data->writableBuffer.append(str, length).getTerminatedBuffer() + oldLength;
}

/**
* Special normalization function for contraction in the forwards iterator.
* This normalization sequence will place the current character at source->pos
* and its following normalized sequence into the buffer.
* The fcd position, pos will be changed.
* pos will now point to positions in the buffer.
* Flags will be changed accordingly.
* @param data collation iterator data
*/
static
inline void normalizeNextContraction(collIterate *data)
{
    int32_t     strsize;
    UErrorCode  status     = U_ZERO_ERROR;
    /* because the pointer points to the next character */
    const UChar *pStart    = data->pos - 1;
    const UChar *pEnd;

    if ((data->flags & UCOL_ITER_INNORMBUF) == 0) {
        data->writableBuffer.setTo(*(pStart - 1));
        strsize               = 1;
    }
    else {
        strsize = data->writableBuffer.length();
    }

    pEnd = data->fcdPosition;

    data->writableBuffer.append(
        data->nfd->normalize(UnicodeString(FALSE, pStart, (int32_t)(pEnd - pStart)), status));
    if(U_FAILURE(status)) {
        return;
    }

    data->pos        = data->writableBuffer.getTerminatedBuffer() + strsize;
    data->origFlags  = data->flags;
    data->flags     |= UCOL_ITER_INNORMBUF;
    data->flags     &= ~(UCOL_ITER_NORM | UCOL_ITER_HASLEN);
}

/**
* Contraction character management function that returns the next character
* for the forwards iterator.
* Does nothing if the next character is in buffer and not the first character
* in it.
* Else it checks next character in data string to see if it is normalizable.
* If it is not, the character is simply copied into the buffer, else
* the whole normalized substring is copied into the buffer, including the
* current character.
* @param data collation element iterator data
* @return next character
*/
static
inline UChar getNextNormalizedChar(collIterate *data)
{
    UChar  nextch;
    UChar  ch;
    // Here we need to add the iterator code. One problem is the way
    // end of string is handled. If we just return next char, it could
    // be the sentinel. Most of the cases already check for this, but we
    // need to be sure.
    if ((data->flags & (UCOL_ITER_NORM | UCOL_ITER_INNORMBUF)) == 0 ) {
         /* if no normalization and not in buffer. */
      if(data->flags & UCOL_USE_ITERATOR) {
         return (UChar)data->iterator->next(data->iterator);
      } else {
         return *(data->pos ++);
      }
    }

    //if (data->flags & UCOL_ITER_NORM && data->flags & UCOL_USE_ITERATOR) {
      //normalizeIterator(data);
    //}

    UBool  innormbuf = (UBool)(data->flags & UCOL_ITER_INNORMBUF);
    if ((innormbuf && *data->pos != 0) ||
        (data->fcdPosition != NULL && !innormbuf &&
        data->pos < data->fcdPosition)) {
        /*
        if next character is in normalized buffer, no further normalization
        is required
        */
        return *(data->pos ++);
    }

    if (data->flags & UCOL_ITER_HASLEN) {
        /* in data string */
        if (data->pos + 1 == data->endp) {
            return *(data->pos ++);
        }
    }
    else {
        if (innormbuf) {
          // inside the normalization buffer, but at the end
          // (since we encountered zero). This means, in the
          // case we're using char iterator, that we need to
          // do another round of normalization.
          //if(data->origFlags & UCOL_USE_ITERATOR) {
            // we need to restore original flags,
            // otherwise, we'll lose them
            //data->flags = data->origFlags;
            //normalizeIterator(data);
            //return *(data->pos++);
          //} else {
            /*
            in writable buffer, at this point fcdPosition can not be
            pointing to the end of the data string. see contracting tag.
            */
          if(data->fcdPosition) {
            if (*(data->fcdPosition + 1) == 0 ||
                data->fcdPosition + 1 == data->endp) {
                /* at the end of the string, dump it into the normalizer */
                data->pos = insertBufferEnd(data, *(data->fcdPosition)) + 1;
                // Check if data->pos received a null pointer
                if (data->pos == NULL) {
                    return (UChar)-1; // Return to indicate error.
                }
                return *(data->fcdPosition ++);
            }
            data->pos = data->fcdPosition;
          } else if(data->origFlags & UCOL_USE_ITERATOR) {
            // if we are here, we're using a normalizing iterator.
            // we should just continue further.
            data->flags = data->origFlags;
            data->pos = NULL;
            return (UChar)data->iterator->next(data->iterator);
          }
          //}
        }
        else {
            if (*(data->pos + 1) == 0) {
                return *(data->pos ++);
            }
        }
    }

    ch = *data->pos ++;
    nextch = *data->pos;

    /*
    * if the current character is not fcd.
    * Trailing combining class == 0.
    */
    if ((data->fcdPosition == NULL || data->fcdPosition < data->pos) &&
        (nextch >= NFC_ZERO_CC_BLOCK_LIMIT_ ||
         ch >= NFC_ZERO_CC_BLOCK_LIMIT_)) {
            /*
            Need a more complete FCD check and possible normalization.
            normalize substring will be appended to buffer
            */
        if (collIterFCD(data)) {
            normalizeNextContraction(data);
            return *(data->pos ++);
        }
        else if (innormbuf) {
            /* fcdposition shifted even when there's no normalization, if we
            don't input the rest into this, we'll get the wrong position when
            we reach the end of the writableBuffer */
            int32_t length = (int32_t)(data->fcdPosition - data->pos + 1);
            data->pos = insertBufferEnd(data, data->pos - 1, length);
            // Check if data->pos received a null pointer
            if (data->pos == NULL) {
                return (UChar)-1; // Return to indicate error.
            }
            return *(data->pos ++);
        }
    }

    if (innormbuf) {
        /*
        no normalization is to be done hence only one character will be
        appended to the buffer.
        */
        data->pos = insertBufferEnd(data, ch) + 1;
        // Check if data->pos received a null pointer
        if (data->pos == NULL) {
            return (UChar)-1; // Return to indicate error.
        }
    }

    /* points back to the pos in string */
    return ch;
}



/**
* Function to copy the buffer into writableBuffer and sets the fcd position to
* the correct position
* @param source data string source
* @param buffer character buffer
*/
static
inline void setDiscontiguosAttribute(collIterate *source, const UnicodeString &buffer)
{
    /* okay confusing part here. to ensure that the skipped characters are
    considered later, we need to place it in the appropriate position in the
    normalization buffer and reassign the pos pointer. simple case if pos
    reside in string, simply copy to normalization buffer and
    fcdposition = pos, pos = start of normalization buffer. if pos in
    normalization buffer, we'll insert the copy infront of pos and point pos
    to the start of the normalization buffer. why am i doing these copies?
    well, so that the whole chunk of codes in the getNextCE, ucol_prv_getSpecialCE does
    not require any changes, which be really painful. */
    if (source->flags & UCOL_ITER_INNORMBUF) {
        int32_t replaceLength = source->pos - source->writableBuffer.getBuffer();
        source->writableBuffer.replace(0, replaceLength, buffer);
    }
    else {
        source->fcdPosition  = source->pos;
        source->origFlags    = source->flags;
        source->flags       |= UCOL_ITER_INNORMBUF;
        source->flags       &= ~(UCOL_ITER_NORM | UCOL_ITER_HASLEN | UCOL_USE_ITERATOR);
        source->writableBuffer = buffer;
    }

    source->pos = source->writableBuffer.getTerminatedBuffer();
}

/**
* Function to get the discontiguos collation element within the source.
* Note this function will set the position to the appropriate places.
* @param coll current collator used
* @param source data string source
* @param constart index to the start character in the contraction table
* @return discontiguos collation element offset
*/
static
uint32_t getDiscontiguous(const UCollator *coll, collIterate *source,
                                const UChar *constart)
{
    /* source->pos currently points to the second combining character after
       the start character */
          const UChar *temppos      = source->pos;
          UnicodeString buffer;
    const UChar   *tempconstart = constart;
          uint8_t  tempflags    = source->flags;
          UBool    multicontraction = FALSE;
          collIterateState discState;

          backupState(source, &discState);

    buffer.setTo(peekCodePoint(source, -1));
    for (;;) {
        UChar    *UCharOffset;
        UChar     schar,
                  tchar;
        uint32_t  result;

        if (((source->flags & UCOL_ITER_HASLEN) && source->pos >= source->endp)
            || (peekCodeUnit(source, 0) == 0  &&
            //|| (*source->pos == 0  &&
                ((source->flags & UCOL_ITER_INNORMBUF) == 0 ||
                 source->fcdPosition == NULL ||
                 source->fcdPosition == source->endp ||
                 *(source->fcdPosition) == 0 ||
                 u_getCombiningClass(*(source->fcdPosition)) == 0)) ||
                 /* end of string in null terminated string or stopped by a
                 null character, note fcd does not always point to a base
                 character after the discontiguos change */
                 u_getCombiningClass(peekCodePoint(source, 0)) == 0) {
                 //u_getCombiningClass(*(source->pos)) == 0) {
            //constart = (UChar *)coll->image + getContractOffset(CE);
            if (multicontraction) {
                source->pos    = temppos - 1;
                setDiscontiguosAttribute(source, buffer);
                return *(coll->contractionCEs +
                                    (tempconstart - coll->contractionIndex));
            }
            constart = tempconstart;
            break;
        }

        UCharOffset = (UChar *)(tempconstart + 1); /* skip the backward offset*/
        schar = getNextNormalizedChar(source);

        while (schar > (tchar = *UCharOffset)) {
            UCharOffset++;
        }

        if (schar != tchar) {
            /* not the correct codepoint. we stuff the current codepoint into
            the discontiguos buffer and try the next character */
            buffer.append(schar);
            continue;
        }
        else {
            if (u_getCombiningClass(schar) ==
                u_getCombiningClass(peekCodePoint(source, -2))) {
                buffer.append(schar);
                continue;
            }
            result = *(coll->contractionCEs +
                                      (UCharOffset - coll->contractionIndex));
        }

        if (result == UCOL_NOT_FOUND) {
          break;
        } else if (isContraction(result)) {
            /* this is a multi-contraction*/
            tempconstart = (UChar *)coll->image + getContractOffset(result);
            if (*(coll->contractionCEs + (constart - coll->contractionIndex))
                != UCOL_NOT_FOUND) {
                multicontraction = TRUE;
                temppos       = source->pos + 1;
            }
        } else {
            setDiscontiguosAttribute(source, buffer);
            return result;
        }
    }

    /* no problems simply reverting just like that,
    if we are in string before getting into this function, points back to
    string hence no problem.
    if we are in normalization buffer before getting into this function,
    since we'll never use another normalization within this function, we
    know that fcdposition points to a base character. the normalization buffer
    never change, hence this revert works. */
    loadState(source, &discState, TRUE);
    goBackOne(source);

    //source->pos   = temppos - 1;
    source->flags = tempflags;
    return *(coll->contractionCEs + (constart - coll->contractionIndex));
}

/* now uses Mark's getImplicitPrimary code */
static
inline uint32_t getImplicit(UChar32 cp, collIterate *collationSource) {
    uint32_t r = uprv_uca_getImplicitPrimary(cp);
    *(collationSource->CEpos++) = ((r & 0x0000FFFF)<<16) | 0x000000C0;
    collationSource->offsetRepeatCount += 1;
    return (r & UCOL_PRIMARYMASK) | 0x00000505; // This was 'order'
}

/**
* Inserts the argument character into the front of the buffer replacing the
* front null terminator.
* @param data collation element iterator data
* @param ch character to be appended
*/
static
inline void insertBufferFront(collIterate *data, UChar ch)
{
    data->pos = data->writableBuffer.setCharAt(0, ch).insert(0, (UChar)0).getTerminatedBuffer() + 2;
}

/**
* Special normalization function for contraction in the previous iterator.
* This normalization sequence will place the current character at source->pos
* and its following normalized sequence into the buffer.
* The fcd position, pos will be changed.
* pos will now point to positions in the buffer.
* Flags will be changed accordingly.
* @param data collation iterator data
*/
static
inline void normalizePrevContraction(collIterate *data, UErrorCode *status)
{
    const UChar *pEnd = data->pos + 1;         /* End normalize + 1 */
    const UChar *pStart;

    UnicodeString endOfBuffer;
    if (data->flags & UCOL_ITER_HASLEN) {
        /*
        normalization buffer not used yet, we'll pull down the next
        character into the end of the buffer
        */
        endOfBuffer.setTo(*pEnd);
    }
    else {
        endOfBuffer.setTo(data->writableBuffer, 1);  // after the leading NUL
    }

    if (data->fcdPosition == NULL) {
        pStart = data->string;
    }
    else {
        pStart = data->fcdPosition + 1;
    }
    int32_t normLen =
        data->nfd->normalize(UnicodeString(FALSE, pStart, (int32_t)(pEnd - pStart)),
                             data->writableBuffer,
                             *status).
        length();
    if(U_FAILURE(*status)) {
        return;
    }
    /*
    this puts the null termination infront of the normalized string instead
    of the end
    */
    data->pos =
        data->writableBuffer.insert(0, (UChar)0).append(endOfBuffer).getTerminatedBuffer() +
        1 + normLen;
    data->origFlags  = data->flags;
    data->flags     |= UCOL_ITER_INNORMBUF;
    data->flags     &= ~(UCOL_ITER_NORM | UCOL_ITER_HASLEN);
}

/**
* Contraction character management function that returns the previous character
* for the backwards iterator.
* Does nothing if the previous character is in buffer and not the first
* character in it.
* Else it checks previous character in data string to see if it is
* normalizable.
* If it is not, the character is simply copied into the buffer, else
* the whole normalized substring is copied into the buffer, including the
* current character.
* @param data collation element iterator data
* @return previous character
*/
static
inline UChar getPrevNormalizedChar(collIterate *data, UErrorCode *status)
{
    UChar  prevch;
    UChar  ch;
    const UChar *start;
    UBool  innormbuf = (UBool)(data->flags & UCOL_ITER_INNORMBUF);
    if ((data->flags & (UCOL_ITER_NORM | UCOL_ITER_INNORMBUF)) == 0 ||
        (innormbuf && *(data->pos - 1) != 0)) {
        /*
        if no normalization.
        if previous character is in normalized buffer, no further normalization
        is required
        */
      if(data->flags & UCOL_USE_ITERATOR) {
        data->iterator->move(data->iterator, -1, UITER_CURRENT);
        return (UChar)data->iterator->next(data->iterator);
      } else {
        return *(data->pos - 1);
      }
    }

    start = data->pos;
    if ((data->fcdPosition==NULL)||(data->flags & UCOL_ITER_HASLEN)) {
        /* in data string */
        if ((start - 1) == data->string) {
            return *(start - 1);
        }
        start --;
        ch     = *start;
        prevch = *(start - 1);
    }
    else {
        /*
        in writable buffer, at this point fcdPosition can not be NULL.
        see contracting tag.
        */
        if (data->fcdPosition == data->string) {
            /* at the start of the string, just dump it into the normalizer */
            insertBufferFront(data, *(data->fcdPosition));
            data->fcdPosition = NULL;
            return *(data->pos - 1);
        }
        start  = data->fcdPosition;
        ch     = *start;
        prevch = *(start - 1);
    }
    /*
    * if the current character is not fcd.
    * Trailing combining class == 0.
    */
    if (data->fcdPosition > start &&
       (ch >= NFC_ZERO_CC_BLOCK_LIMIT_ || prevch >= NFC_ZERO_CC_BLOCK_LIMIT_))
    {
        /*
        Need a more complete FCD check and possible normalization.
        normalize substring will be appended to buffer
        */
        const UChar *backuppos = data->pos;
        data->pos = start;
        if (collPrevIterFCD(data)) {
            normalizePrevContraction(data, status);
            return *(data->pos - 1);
        }
        data->pos = backuppos;
        data->fcdPosition ++;
    }

    if (innormbuf) {
    /*
    no normalization is to be done hence only one character will be
    appended to the buffer.
    */
        insertBufferFront(data, ch);
        data->fcdPosition --;
    }

    return ch;
}

/* This function handles the special CEs like contractions, expansions, surrogates, Thai */
/* It is called by getNextCE */

/* The following should be even */
#define UCOL_MAX_DIGITS_FOR_NUMBER 254

uint32_t ucol_prv_getSpecialCE(const UCollator *coll, UChar ch, uint32_t CE, collIterate *source, UErrorCode *status) {
    collIterateState entryState;
    backupState(source, &entryState);
    UChar32 cp = ch;

    for (;;) {
        // This loop will repeat only in the case of contractions, and only when a contraction
        //   is found and the first CE resulting from that contraction is itself a special
        //   (an expansion, for example.)  All other special CE types are fully handled the
        //   first time through, and the loop exits.

        const uint32_t *CEOffset = NULL;
        switch(getCETag(CE)) {
        case NOT_FOUND_TAG:
            /* This one is not found, and we'll let somebody else bother about it... no more games */
            return CE;
        case SPEC_PROC_TAG:
            {
                // Special processing is getting a CE that is preceded by a certain prefix
                // Currently this is only needed for optimizing Japanese length and iteration marks.
                // When we encouter a special processing tag, we go backwards and try to see if
                // we have a match.
                // Contraction tables are used - so the whole process is not unlike contraction.
                // prefix data is stored backwards in the table.
                const UChar *UCharOffset;
                UChar schar, tchar;
                collIterateState prefixState;
                backupState(source, &prefixState);
                loadState(source, &entryState, TRUE);
                goBackOne(source); // We want to look at the point where we entered - actually one
                // before that...

                for(;;) {
                    // This loop will run once per source string character, for as long as we
                    //  are matching a potential contraction sequence

                    // First we position ourselves at the begining of contraction sequence
                    const UChar *ContractionStart = UCharOffset = (UChar *)coll->image+getContractOffset(CE);
                    if (collIter_bos(source)) {
                        CE = *(coll->contractionCEs + (UCharOffset - coll->contractionIndex));
                        break;
                    }
                    schar = getPrevNormalizedChar(source, status);
                    goBackOne(source);

                    while(schar > (tchar = *UCharOffset)) { /* since the contraction codepoints should be ordered, we skip all that are smaller */
                        UCharOffset++;
                    }

                    if (schar == tchar) {
                        // Found the source string char in the table.
                        //  Pick up the corresponding CE from the table.
                        CE = *(coll->contractionCEs +
                            (UCharOffset - coll->contractionIndex));
                    }
                    else
                    {
                        // Source string char was not in the table.
                        //   We have not found the prefix.
                        CE = *(coll->contractionCEs +
                            (ContractionStart - coll->contractionIndex));
                    }

                    if(!isPrefix(CE)) {
                        // The source string char was in the contraction table, and the corresponding
                        //   CE is not a prefix CE.  We found the prefix, break
                        //   out of loop, this CE will end up being returned.  This is the normal
                        //   way out of prefix handling when the source actually contained
                        //   the prefix.
                        break;
                    }
                }
                if(CE != UCOL_NOT_FOUND) { // we found something and we can merilly continue
                    loadState(source, &prefixState, TRUE);
                    if(source->origFlags & UCOL_USE_ITERATOR) {
                        source->flags = source->origFlags;
                    }
                } else { // prefix search was a failure, we have to backup all the way to the start
                    loadState(source, &entryState, TRUE);
                }
                break;
            }
        case CONTRACTION_TAG:
            {
                /* This should handle contractions */
                collIterateState state;
                backupState(source, &state);
                uint32_t firstCE = *(coll->contractionCEs + ((UChar *)coll->image+getContractOffset(CE) - coll->contractionIndex)); //UCOL_NOT_FOUND;
                const UChar *UCharOffset;
                UChar schar, tchar;

                for (;;) {
                    /* This loop will run once per source string character, for as long as we     */
                    /*  are matching a potential contraction sequence                  */

                    /* First we position ourselves at the begining of contraction sequence */
                    const UChar *ContractionStart = UCharOffset = (UChar *)coll->image+getContractOffset(CE);

                    if (collIter_eos(source)) {
                        // Ran off the end of the source string.
                        CE = *(coll->contractionCEs + (UCharOffset - coll->contractionIndex));
                        // So we'll pick whatever we have at the point...
                        if (CE == UCOL_NOT_FOUND) {
                            // back up the source over all the chars we scanned going into this contraction.
                            CE = firstCE;
                            loadState(source, &state, TRUE);
                            if(source->origFlags & UCOL_USE_ITERATOR) {
                                source->flags = source->origFlags;
                            }
                        }
                        break;
                    }

                    uint8_t maxCC = (uint8_t)(*(UCharOffset)&0xFF); /*get the discontiguos stuff */ /* skip the backward offset, see above */
                    uint8_t allSame = (uint8_t)(*(UCharOffset++)>>8);

                    schar = getNextNormalizedChar(source);
                    while(schar > (tchar = *UCharOffset)) { /* since the contraction codepoints should be ordered, we skip all that are smaller */
                        UCharOffset++;
                    }

                    if (schar == tchar) {
                        // Found the source string char in the contraction table.
                        //  Pick up the corresponding CE from the table.
                        CE = *(coll->contractionCEs +
                            (UCharOffset - coll->contractionIndex));
                    }
                    else
                    {
                        // Source string char was not in contraction table.
                        //   Unless we have a discontiguous contraction, we have finished
                        //   with this contraction.
                        // in order to do the proper detection, we
                        // need to see if we're dealing with a supplementary
                        /* We test whether the next two char are surrogate pairs.
                        * This test is done if the iterator is not NULL.
                        * If there is no surrogate pair, the iterator
                        * goes back one if needed. */
                        UChar32 miss = schar;
                        if (source->iterator) {
                            UChar32 surrNextChar; /* the next char in the iteration to test */
                            int32_t prevPos; /* holds the previous position before move forward of the source iterator */
                            if(U16_IS_LEAD(schar) && source->iterator->hasNext(source->iterator)) {
                                prevPos = source->iterator->index;
                                surrNextChar = getNextNormalizedChar(source);
                                if (U16_IS_TRAIL(surrNextChar)) {
                                    miss = U16_GET_SUPPLEMENTARY(schar, surrNextChar);
                                } else if (prevPos < source->iterator->index){
                                    goBackOne(source);
                                }
                            }
                        } else if (U16_IS_LEAD(schar)) {
                            miss = U16_GET_SUPPLEMENTARY(schar, getNextNormalizedChar(source));
                        }

                        uint8_t sCC;
                        if (miss < 0x300 ||
                            maxCC == 0 ||
                            (sCC = i_getCombiningClass(miss, coll)) == 0 ||
                            sCC>maxCC ||
                            (allSame != 0 && sCC == maxCC) ||
                            collIter_eos(source))
                        {
                            //  Contraction can not be discontiguous.
                            goBackOne(source);  // back up the source string by one,
                            //  because  the character we just looked at was
                            //  not part of the contraction.   */
                            if(U_IS_SUPPLEMENTARY(miss)) {
                                goBackOne(source);
                            }
                            CE = *(coll->contractionCEs +
                                (ContractionStart - coll->contractionIndex));
                        } else {
                            //
                            // Contraction is possibly discontiguous.
                            //   Scan more of source string looking for a match
                            //
                            UChar tempchar;
                            /* find the next character if schar is not a base character
                            and we are not yet at the end of the string */
                            tempchar = getNextNormalizedChar(source);
                            // probably need another supplementary thingie here
                            goBackOne(source);
                            if (i_getCombiningClass(tempchar, coll) == 0) {
                                goBackOne(source);
                                if(U_IS_SUPPLEMENTARY(miss)) {
                                    goBackOne(source);
                                }
                                /* Spit out the last char of the string, wasn't tasty enough */
                                CE = *(coll->contractionCEs +
                                    (ContractionStart - coll->contractionIndex));
                            } else {
                                CE = getDiscontiguous(coll, source, ContractionStart);
                            }
                        }
                    } // else after if(schar == tchar)

                    if(CE == UCOL_NOT_FOUND) {
                        /* The Source string did not match the contraction that we were checking.  */
                        /*  Back up the source position to undo the effects of having partially    */
                        /*   scanned through what ultimately proved to not be a contraction.       */
                        loadState(source, &state, TRUE);
                        CE = firstCE;
                        break;
                    }

                    if(!isContraction(CE)) {
                        // The source string char was in the contraction table, and the corresponding
                        //   CE is not a contraction CE.  We completed the contraction, break
                        //   out of loop, this CE will end up being returned.  This is the normal
                        //   way out of contraction handling when the source actually contained
                        //   the contraction.
                        break;
                    }


                    // The source string char was in the contraction table, and the corresponding
                    //   CE is IS  a contraction CE.  We will continue looping to check the source
                    //   string for the remaining chars in the contraction.
                    uint32_t tempCE = *(coll->contractionCEs + (ContractionStart - coll->contractionIndex));
                    if(tempCE != UCOL_NOT_FOUND) {
                        // We have scanned a a section of source string for which there is a
                        //  CE from the contraction table.  Remember the CE and scan position, so
                        //  that we can return to this point if further scanning fails to
                        //  match a longer contraction sequence.
                        firstCE = tempCE;

                        goBackOne(source);
                        backupState(source, &state);
                        getNextNormalizedChar(source);

                        // Another way to do this is:
                        //collIterateState tempState;
                        //backupState(source, &tempState);
                        //goBackOne(source);
                        //backupState(source, &state);
                        //loadState(source, &tempState, TRUE);

                        // The problem is that for incomplete contractions we have to remember the previous
                        // position. Before, the only thing I needed to do was state.pos--;
                        // After iterator introduction and especially after introduction of normalizing
                        // iterators, it became much more difficult to decrease the saved state.
                        // I'm not yet sure which of the two methods above is faster.
                    }
                } // for(;;)
                break;
            } // case CONTRACTION_TAG:
        case LONG_PRIMARY_TAG:
            {
                *(source->CEpos++) = ((CE & 0xFF)<<24)|UCOL_CONTINUATION_MARKER;
                CE = ((CE & 0xFFFF00) << 8) | (UCOL_BYTE_COMMON << 8) | UCOL_BYTE_COMMON;
                source->offsetRepeatCount += 1;
                return CE;
            }
        case EXPANSION_TAG:
            {
                /* This should handle expansion. */
                /* NOTE: we can encounter both continuations and expansions in an expansion! */
                /* I have to decide where continuations are going to be dealt with */
                uint32_t size;
                uint32_t i;    /* general counter */

                CEOffset = (uint32_t *)coll->image+getExpansionOffset(CE); /* find the offset to expansion table */
                size = getExpansionCount(CE);
                CE = *CEOffset++;
              //source->offsetRepeatCount = -1;

                if(size != 0) { /* if there are less than 16 elements in expansion, we don't terminate */
                    for(i = 1; i<size; i++) {
                        *(source->CEpos++) = *CEOffset++;
                        source->offsetRepeatCount += 1;
                    }
                } else { /* else, we do */
                    while(*CEOffset != 0) {
                        *(source->CEpos++) = *CEOffset++;
                        source->offsetRepeatCount += 1;
                    }
                }

                return CE;
            }
        case DIGIT_TAG:
            {
                /*
                We do a check to see if we want to collate digits as numbers; if so we generate
                a custom collation key. Otherwise we pull out the value stored in the expansion table.
                */
                //uint32_t size;
                uint32_t i;    /* general counter */

                if (source->coll->numericCollation == UCOL_ON){
                    collIterateState digitState = {0,0,0,0,0,0,0,0,0};
                    UChar32 char32 = 0;
                    int32_t digVal = 0;

                    uint32_t digIndx = 0;
                    uint32_t endIndex = 0;
                    uint32_t trailingZeroIndex = 0;

                    uint8_t collateVal = 0;

                    UBool nonZeroValReached = FALSE;

                    uint8_t numTempBuf[UCOL_MAX_DIGITS_FOR_NUMBER/2 + 3]; // I just need a temporary place to store my generated CEs.
                    /*
                         We parse the source string until we hit a char that's NOT a digit.
                        Use this u_charDigitValue. This might be slow because we have to
                        handle surrogates...
                    */
            /*
                    if (U16_IS_LEAD(ch)){
                      if (!collIter_eos(source)) {
                        backupState(source, &digitState);
                        UChar trail = getNextNormalizedChar(source);
                        if(U16_IS_TRAIL(trail)) {
                          char32 = U16_GET_SUPPLEMENTARY(ch, trail);
                        } else {
                          loadState(source, &digitState, TRUE);
                          char32 = ch;
                        }
                      } else {
                        char32 = ch;
                      }
                    } else {
                      char32 = ch;
                    }
                    digVal = u_charDigitValue(char32);
            */
                    digVal = u_charDigitValue(cp); // if we have arrived here, we have
                    // already processed possible supplementaries that trigered the digit tag -
                    // all supplementaries are marked in the UCA.
                    /*
                        We  pad a zero in front of the first element anyways. This takes
                        care of the (probably) most common case where people are sorting things followed
                        by a single digit
                    */
                    digIndx++;
                    for(;;){
                        // Make sure we have enough space. No longer needed;
                        // at this point digIndx now has a max value of UCOL_MAX_DIGITS_FOR_NUMBER
                        // (it has been pre-incremented) so we just ensure that numTempBuf is big enough
                        // (UCOL_MAX_DIGITS_FOR_NUMBER/2 + 3).

                        // Skipping over leading zeroes.
                        if (digVal != 0) {
                            nonZeroValReached = TRUE;
                        }
                        if (nonZeroValReached) {
                            /*
                            We parse the digit string into base 100 numbers (this fits into a byte).
                            We only add to the buffer in twos, thus if we are parsing an odd character,
                            that serves as the 'tens' digit while the if we are parsing an even one, that
                            is the 'ones' digit. We dumped the parsed base 100 value (collateVal) into
                            a buffer. We multiply each collateVal by 2 (to give us room) and add 5 (to avoid
                            overlapping magic CE byte values). The last byte we subtract 1 to ensure it is less
                            than all the other bytes.
                            */

                            if (digIndx % 2 == 1){
                                collateVal += (uint8_t)digVal;

                                // We don't enter the low-order-digit case unless we've already seen
                                // the high order, or for the first digit, which is always non-zero.
                                if (collateVal != 0)
                                    trailingZeroIndex = 0;

                                numTempBuf[(digIndx/2) + 2] = collateVal*2 + 6;
                                collateVal = 0;
                            }
                            else{
                                // We drop the collation value into the buffer so if we need to do
                                // a "front patch" we don't have to check to see if we're hitting the
                                // last element.
                                collateVal = (uint8_t)(digVal * 10);

                                // Check for trailing zeroes.
                                if (collateVal == 0)
                                {
                                    if (!trailingZeroIndex)
                                        trailingZeroIndex = (digIndx/2) + 2;
                                }
                                else
                                    trailingZeroIndex = 0;

                                numTempBuf[(digIndx/2) + 2] = collateVal*2 + 6;
                            }
                            digIndx++;
                        }

                        // Get next character.
                        if (!collIter_eos(source)){
                            ch = getNextNormalizedChar(source);
                            if (U16_IS_LEAD(ch)){
                                if (!collIter_eos(source)) {
                                    backupState(source, &digitState);
                                    UChar trail = getNextNormalizedChar(source);
                                    if(U16_IS_TRAIL(trail)) {
                                        char32 = U16_GET_SUPPLEMENTARY(ch, trail);
                                    } else {
                                        loadState(source, &digitState, TRUE);
                                        char32 = ch;
                                    }
                                }
                            } else {
                                char32 = ch;
                            }

                            if ((digVal = u_charDigitValue(char32)) == -1 || digIndx > UCOL_MAX_DIGITS_FOR_NUMBER){
                                // Resetting position to point to the next unprocessed char. We
                                // overshot it when doing our test/set for numbers.
                                if (char32 > 0xFFFF) { // For surrogates.
                                    loadState(source, &digitState, TRUE);
                                    //goBackOne(source);
                                }
                                goBackOne(source);
                                break;
                            }
                        } else {
                            break;
                        }
                    }

                    if (nonZeroValReached == FALSE){
                        digIndx = 2;
                        numTempBuf[2] = 6;
                    }

                    endIndex = trailingZeroIndex ? trailingZeroIndex : ((digIndx/2) + 2) ;
                    if (digIndx % 2 != 0){
                        /*
                        We missed a value. Since digIndx isn't even, stuck too many values into the buffer (this is what
                        we get for padding the first byte with a zero). "Front-patch" now by pushing all nybbles forward.
                        Doing it this way ensures that at least 50% of the time (statistically speaking) we'll only be doing a
                        single pass and optimizes for strings with single digits. I'm just assuming that's the more common case.
                        */

                        for(i = 2; i < endIndex; i++){
                            numTempBuf[i] =     (((((numTempBuf[i] - 6)/2) % 10) * 10) +
                                (((numTempBuf[i+1])-6)/2) / 10) * 2 + 6;
                        }
                        --digIndx;
                    }

                    // Subtract one off of the last byte.
                    numTempBuf[endIndex-1] -= 1;

                    /*
                    We want to skip over the first two slots in the buffer. The first slot
                    is reserved for the header byte UCOL_CODAN_PLACEHOLDER. The second slot is for the
                    sign/exponent byte: 0x80 + (decimalPos/2) & 7f.
                    */
                    numTempBuf[0] = UCOL_CODAN_PLACEHOLDER;
                    numTempBuf[1] = (uint8_t)(0x80 + ((digIndx/2) & 0x7F));

                    // Now transfer the collation key to our collIterate struct.
                    // The total size for our collation key is endIndx bumped up to the next largest even value divided by two.
                    //size = ((endIndex+1) & ~1)/2;
                    CE = (((numTempBuf[0] << 8) | numTempBuf[1]) << UCOL_PRIMARYORDERSHIFT) | //Primary weight
                        (UCOL_BYTE_COMMON << UCOL_SECONDARYORDERSHIFT) | // Secondary weight
                        UCOL_BYTE_COMMON; // Tertiary weight.
                    i = 2; // Reset the index into the buffer.
                    while(i < endIndex)
                    {
                        uint32_t primWeight = numTempBuf[i++] << 8;
                        if ( i < endIndex)
                            primWeight |= numTempBuf[i++];
                        *(source->CEpos++) = (primWeight << UCOL_PRIMARYORDERSHIFT) | UCOL_CONTINUATION_MARKER;
                    }

                } else {
                    // no numeric mode, we'll just switch to whatever we stashed and continue
                    CEOffset = (uint32_t *)coll->image+getExpansionOffset(CE); /* find the offset to expansion table */
                    CE = *CEOffset++;
                    break;
                }
                return CE;
            }
            /* various implicits optimization */
        case IMPLICIT_TAG:        /* everything that is not defined otherwise */
            /* UCA is filled with these. Tailorings are NOT_FOUND */
            return getImplicit(cp, source);
        case CJK_IMPLICIT_TAG:    /* 0x3400-0x4DB5, 0x4E00-0x9FA5, 0xF900-0xFA2D*/
            // TODO: remove CJK_IMPLICIT_TAG completely - handled by the getImplicit
            return getImplicit(cp, source);
        case HANGUL_SYLLABLE_TAG: /* AC00-D7AF*/
            {
                static const uint32_t
                    SBase = 0xAC00, LBase = 0x1100, VBase = 0x1161, TBase = 0x11A7;
                //const uint32_t LCount = 19;
                static const uint32_t VCount = 21;
                static const uint32_t TCount = 28;
                //const uint32_t NCount = VCount * TCount;   // 588
                //const uint32_t SCount = LCount * NCount;   // 11172
                uint32_t L = ch - SBase;

                // divide into pieces

                uint32_t T = L % TCount; // we do it in this order since some compilers can do % and / in one operation
                L /= TCount;
                uint32_t V = L % VCount;
                L /= VCount;

                // offset them

                L += LBase;
                V += VBase;
                T += TBase;

                // return the first CE, but first put the rest into the expansion buffer
                if (!source->coll->image->jamoSpecial) { // FAST PATH

                    *(source->CEpos++) = UTRIE_GET32_FROM_LEAD(&coll->mapping, V);
                    if (T != TBase) {
                        *(source->CEpos++) = UTRIE_GET32_FROM_LEAD(&coll->mapping, T);
                    }

                    return UTRIE_GET32_FROM_LEAD(&coll->mapping, L);

                } else { // Jamo is Special
                    // Since Hanguls pass the FCD check, it is
                    // guaranteed that we won't be in
                    // the normalization buffer if something like this happens

                    // However, if we are using a uchar iterator and normalization
                    // is ON, the Hangul that lead us here is going to be in that
                    // normalization buffer. Here we want to restore the uchar
                    // iterator state and pull out of the normalization buffer
                    if(source->iterator != NULL && source->flags & UCOL_ITER_INNORMBUF) {
                        source->flags = source->origFlags; // restore the iterator
                        source->pos = NULL;
                    }

                    // Move Jamos into normalization buffer
                    UChar *buffer = source->writableBuffer.getBuffer(4);
                    int32_t bufferLength;
                    buffer[0] = (UChar)L;
                    buffer[1] = (UChar)V;
                    if (T != TBase) {
                        buffer[2] = (UChar)T;
                        bufferLength = 3;
                    } else {
                        bufferLength = 2;
                    }
                    source->writableBuffer.releaseBuffer(bufferLength);

                    // Indicate where to continue in main input string after exhausting the writableBuffer
                    source->fcdPosition       = source->pos;

                    source->pos   = source->writableBuffer.getTerminatedBuffer();
                    source->origFlags   = source->flags;
                    source->flags       |= UCOL_ITER_INNORMBUF;
                    source->flags       &= ~(UCOL_ITER_NORM | UCOL_ITER_HASLEN);

                    return(UCOL_IGNORABLE);
                }
            }
        case SURROGATE_TAG:
            /* we encountered a leading surrogate. We shall get the CE by using the following code unit */
            /* two things can happen here: next code point can be a trailing surrogate - we will use it */
            /* to retrieve the CE, or it is not a trailing surrogate (or the string is done). In that case */
            /* we treat it like an unassigned code point. */
            {
                UChar trail;
                collIterateState state;
                backupState(source, &state);
                if (collIter_eos(source) || !(U16_IS_TRAIL((trail = getNextNormalizedChar(source))))) {
                    // we chould have stepped one char forward and it might have turned that it
                    // was not a trail surrogate. In that case, we have to backup.
                    loadState(source, &state, TRUE);
                    return UCOL_NOT_FOUND;
                } else {
                    /* TODO: CE contain the data from the previous CE + the mask. It should at least be unmasked */
                    CE = UTRIE_GET32_FROM_OFFSET_TRAIL(&coll->mapping, CE&0xFFFFFF, trail);
                    if(CE == UCOL_NOT_FOUND) { // there are tailored surrogates in this block, but not this one.
                        // We need to backup
                        loadState(source, &state, TRUE);
                        return CE;
                    }
                    // calculate the supplementary code point value, if surrogate was not tailored
                    cp = ((((uint32_t)ch)<<10UL)+(trail)-(((uint32_t)0xd800<<10UL)+0xdc00-0x10000));
                }
            }
            break;
        case LEAD_SURROGATE_TAG:  /* D800-DBFF*/
            UChar nextChar;
            if( source->flags & UCOL_USE_ITERATOR) {
                if(U_IS_TRAIL(nextChar = (UChar)source->iterator->current(source->iterator))) {
                    cp = U16_GET_SUPPLEMENTARY(ch, nextChar);
                    source->iterator->next(source->iterator);
                    return getImplicit(cp, source);
                }
            } else if((((source->flags & UCOL_ITER_HASLEN) == 0 ) || (source->pos<source->endp)) &&
                      U_IS_TRAIL((nextChar=*source->pos))) {
                cp = U16_GET_SUPPLEMENTARY(ch, nextChar);
                source->pos++;
                return getImplicit(cp, source);
            }
            return UCOL_NOT_FOUND;
        case TRAIL_SURROGATE_TAG: /* DC00-DFFF*/
            return UCOL_NOT_FOUND; /* broken surrogate sequence */
        case CHARSET_TAG:
            /* not yet implemented */
            /* probably after 1.8 */
            return UCOL_NOT_FOUND;
        default:
            *status = U_INTERNAL_PROGRAM_ERROR;
            CE=0;
            break;
    }
    if (CE <= UCOL_NOT_FOUND) break;
  }
  return CE;
}


/* now uses Mark's getImplicitPrimary code */
static
inline uint32_t getPrevImplicit(UChar32 cp, collIterate *collationSource) {
    uint32_t r = uprv_uca_getImplicitPrimary(cp);

    *(collationSource->CEpos++) = (r & UCOL_PRIMARYMASK) | 0x00000505;
    collationSource->toReturn = collationSource->CEpos;

    // **** doesn't work if using iterator ****
    if (collationSource->flags & UCOL_ITER_INNORMBUF) {
        collationSource->offsetRepeatCount = 1;
    } else {
        int32_t firstOffset = (int32_t)(collationSource->pos - collationSource->string);

        UErrorCode errorCode = U_ZERO_ERROR;
        collationSource->appendOffset(firstOffset, errorCode);
        collationSource->appendOffset(firstOffset + 1, errorCode);

        collationSource->offsetReturn = collationSource->offsetStore - 1;
        *(collationSource->offsetBuffer) = firstOffset;
        if (collationSource->offsetReturn == collationSource->offsetBuffer) {
            collationSource->offsetStore = collationSource->offsetBuffer;
        }
    }

    return ((r & 0x0000FFFF)<<16) | 0x000000C0;
}

/**
 * This function handles the special CEs like contractions, expansions,
 * surrogates, Thai.
 * It is called by both getPrevCE
 */
uint32_t ucol_prv_getSpecialPrevCE(const UCollator *coll, UChar ch, uint32_t CE,
                          collIterate *source,
                          UErrorCode *status)
{
    const uint32_t *CEOffset    = NULL;
          UChar    *UCharOffset = NULL;
          UChar    schar;
    const UChar    *constart    = NULL;
          uint32_t size;
          UChar    buffer[UCOL_MAX_BUFFER];
          uint32_t *endCEBuffer;
          UChar   *strbuffer;
          int32_t noChars = 0;
          int32_t CECount = 0;

    for(;;)
    {
        /* the only ces that loops are thai and contractions */
        switch (getCETag(CE))
        {
        case NOT_FOUND_TAG:  /* this tag always returns */
            return CE;

        case SPEC_PROC_TAG:
            {
                // Special processing is getting a CE that is preceded by a certain prefix
                // Currently this is only needed for optimizing Japanese length and iteration marks.
                // When we encouter a special processing tag, we go backwards and try to see if
                // we have a match.
                // Contraction tables are used - so the whole process is not unlike contraction.
                // prefix data is stored backwards in the table.
                const UChar *UCharOffset;
                UChar schar, tchar;
                collIterateState prefixState;
                backupState(source, &prefixState);
                for(;;) {
                    // This loop will run once per source string character, for as long as we
                    //  are matching a potential contraction sequence

                    // First we position ourselves at the begining of contraction sequence
                    const UChar *ContractionStart = UCharOffset = (UChar *)coll->image+getContractOffset(CE);

                    if (collIter_bos(source)) {
                        CE = *(coll->contractionCEs + (UCharOffset - coll->contractionIndex));
                        break;
                    }
                    schar = getPrevNormalizedChar(source, status);
                    goBackOne(source);

                    while(schar > (tchar = *UCharOffset)) { /* since the contraction codepoints should be ordered, we skip all that are smaller */
                        UCharOffset++;
                    }

                    if (schar == tchar) {
                        // Found the source string char in the table.
                        //  Pick up the corresponding CE from the table.
                        CE = *(coll->contractionCEs +
                            (UCharOffset - coll->contractionIndex));
                    }
                    else
                    {
                        // if there is a completely ignorable code point in the middle of
                        // a prefix, we need to act as if it's not there
                        // assumption: 'real' noncharacters (*fffe, *ffff, fdd0-fdef are set to zero)
                        // lone surrogates cannot be set to zero as it would break other processing
                        uint32_t isZeroCE = UTRIE_GET32_FROM_LEAD(&coll->mapping, schar);
                        // it's easy for BMP code points
                        if(isZeroCE == 0) {
                            continue;
                        } else if(U16_IS_SURROGATE(schar)) {
                            // for supplementary code points, we have to check the next one
                            // situations where we are going to ignore
                            // 1. beginning of the string: schar is a lone surrogate
                            // 2. schar is a lone surrogate
                            // 3. schar is a trail surrogate in a valid surrogate sequence
                            //    that is explicitly set to zero.
                            if (!collIter_bos(source)) {
                                UChar lead;
                                if(!U16_IS_SURROGATE_LEAD(schar) && U16_IS_LEAD(lead = getPrevNormalizedChar(source, status))) {
                                    isZeroCE = UTRIE_GET32_FROM_LEAD(&coll->mapping, lead);
                                    if(isSpecial(isZeroCE) && getCETag(isZeroCE) == SURROGATE_TAG) {
                                        uint32_t finalCE = UTRIE_GET32_FROM_OFFSET_TRAIL(&coll->mapping, isZeroCE&0xFFFFFF, schar);
                                        if(finalCE == 0) {
                                            // this is a real, assigned completely ignorable code point
                                            goBackOne(source);
                                            continue;
                                        }
                                    }
                                } else {
                                    // lone surrogate, treat like unassigned
                                    return UCOL_NOT_FOUND;
                                }
                            } else {
                                // lone surrogate at the beggining, treat like unassigned
                                return UCOL_NOT_FOUND;
                            }
                        }
                        // Source string char was not in the table.
                        //   We have not found the prefix.
                        CE = *(coll->contractionCEs +
                            (ContractionStart - coll->contractionIndex));
                    }

                    if(!isPrefix(CE)) {
                        // The source string char was in the contraction table, and the corresponding
                        //   CE is not a prefix CE.  We found the prefix, break
                        //   out of loop, this CE will end up being returned.  This is the normal
                        //   way out of prefix handling when the source actually contained
                        //   the prefix.
                        break;
                    }
                }
                loadState(source, &prefixState, TRUE);
                break;
            }

        case CONTRACTION_TAG: {
            /* to ensure that the backwards and forwards iteration matches, we
            take the current region of most possible match and pass it through
            the forward iteration. this will ensure that the obstinate problem of
            overlapping contractions will not occur.
            */
            schar = peekCodeUnit(source, 0);
            constart = (UChar *)coll->image + getContractOffset(CE);
            if (isAtStartPrevIterate(source)
                /* commented away contraction end checks after adding the checks
                in getPrevCE  */) {
                    /* start of string or this is not the end of any contraction */
                    CE = *(coll->contractionCEs +
                        (constart - coll->contractionIndex));
                    break;
            }
            strbuffer = buffer;
            UCharOffset = strbuffer + (UCOL_MAX_BUFFER - 1);
            *(UCharOffset --) = 0;
            noChars = 0;
            // have to swap thai characters
            while (ucol_unsafeCP(schar, coll)) {
                *(UCharOffset) = schar;
                noChars++;
                UCharOffset --;
                schar = getPrevNormalizedChar(source, status);
                goBackOne(source);
                // TODO: when we exhaust the contraction buffer,
                // it needs to get reallocated. The problem is
                // that the size depends on the string which is
                // not iterated over. However, since we're travelling
                // backwards, we already had to set the iterator at
                // the end - so we might as well know where we are?
                if (UCharOffset + 1 == buffer) {
                    /* we have exhausted the buffer */
                    int32_t newsize = 0;
                    if(source->pos) { // actually dealing with a position
                        newsize = (int32_t)(source->pos - source->string + 1);
                    } else { // iterator
                        newsize = 4 * UCOL_MAX_BUFFER;
                    }
                    strbuffer = (UChar *)uprv_malloc(sizeof(UChar) *
                        (newsize + UCOL_MAX_BUFFER));
                    /* test for NULL */
                    if (strbuffer == NULL) {
                        *status = U_MEMORY_ALLOCATION_ERROR;
                        return UCOL_NO_MORE_CES;
                    }
                    UCharOffset = strbuffer + newsize;
                    uprv_memcpy(UCharOffset, buffer,
                        UCOL_MAX_BUFFER * sizeof(UChar));
                    UCharOffset --;
                }
                if ((source->pos && (source->pos == source->string ||
                    ((source->flags & UCOL_ITER_INNORMBUF) &&
                    *(source->pos - 1) == 0 && source->fcdPosition == NULL)))
                    || (source->iterator && !source->iterator->hasPrevious(source->iterator))) {
                        break;
                }
            }
            /* adds the initial base character to the string */
            *(UCharOffset) = schar;
            noChars++;

            int32_t offsetBias;

            // **** doesn't work if using iterator ****
            if (source->flags & UCOL_ITER_INNORMBUF) {
                offsetBias = -1;
            } else {
                offsetBias = (int32_t)(source->pos - source->string);
            }

            /* a new collIterate is used to simplify things, since using the current
            collIterate will mean that the forward and backwards iteration will
            share and change the same buffers. we don't want to get into that. */
            collIterate temp;
            int32_t rawOffset;

            IInit_collIterate(coll, UCharOffset, noChars, &temp, status);
            if(U_FAILURE(*status)) {
                return UCOL_NULLORDER;
            }
            temp.flags &= ~UCOL_ITER_NORM;
            temp.flags |= source->flags & UCOL_FORCE_HAN_IMPLICIT;

            rawOffset = (int32_t)(temp.pos - temp.string); // should always be zero?
            CE = ucol_IGetNextCE(coll, &temp, status);

            if (source->extendCEs) {
                endCEBuffer = source->extendCEs + source->extendCEsSize;
                CECount = (int32_t)((source->CEpos - source->extendCEs)/sizeof(uint32_t));
            } else {
                endCEBuffer = source->CEs + UCOL_EXPAND_CE_BUFFER_SIZE;
                CECount = (int32_t)((source->CEpos - source->CEs)/sizeof(uint32_t));
            }

            while (CE != UCOL_NO_MORE_CES) {
                *(source->CEpos ++) = CE;

                if (offsetBias >= 0) {
                    source->appendOffset(rawOffset + offsetBias, *status);
                }

                CECount++;
                if (source->CEpos == endCEBuffer) {
                    /* ran out of CE space, reallocate to new buffer.
                    If reallocation fails, reset pointers and bail out,
                    there's no guarantee of the right character position after
                    this bail*/
                    if (!increaseCEsCapacity(source)) {
                        *status = U_MEMORY_ALLOCATION_ERROR;
                        break;
                    }

                    endCEBuffer = source->extendCEs + source->extendCEsSize;
                }

                if ((temp.flags & UCOL_ITER_INNORMBUF) != 0) {
                    rawOffset = (int32_t)(temp.fcdPosition - temp.string);
                } else {
                    rawOffset = (int32_t)(temp.pos - temp.string);
                }

                CE = ucol_IGetNextCE(coll, &temp, status);
            }

            if (strbuffer != buffer) {
                uprv_free(strbuffer);
            }
            if (U_FAILURE(*status)) {
                return (uint32_t)UCOL_NULLORDER;
            }

            if (source->offsetRepeatValue != 0) {
                if (CECount > noChars) {
                    source->offsetRepeatCount += temp.offsetRepeatCount;
                } else {
                    // **** does this really skip the right offsets? ****
                    source->offsetReturn -= (noChars - CECount);
                }
            }

            if (offsetBias >= 0) {
                source->offsetReturn = source->offsetStore - 1;
                if (source->offsetReturn == source->offsetBuffer) {
                    source->offsetStore = source->offsetBuffer;
                }
            }

            source->toReturn = source->CEpos - 1;
            if (source->toReturn == source->CEs) {
                source->CEpos = source->CEs;
            }

            return *(source->toReturn);
        }
        case LONG_PRIMARY_TAG:
            {
                *(source->CEpos++) = ((CE & 0xFFFF00) << 8) | (UCOL_BYTE_COMMON << 8) | UCOL_BYTE_COMMON;
                *(source->CEpos++) = ((CE & 0xFF)<<24)|UCOL_CONTINUATION_MARKER;
                source->toReturn = source->CEpos - 1;

                if (source->flags & UCOL_ITER_INNORMBUF) {
                    source->offsetRepeatCount = 1;
                } else {
                    int32_t firstOffset = (int32_t)(source->pos - source->string);

                    source->appendOffset(firstOffset, *status);
                    source->appendOffset(firstOffset + 1, *status);

                    source->offsetReturn = source->offsetStore - 1;
                    *(source->offsetBuffer) = firstOffset;
                    if (source->offsetReturn == source->offsetBuffer) {
                        source->offsetStore = source->offsetBuffer;
                    }
                }


                return *(source->toReturn);
            }

        case EXPANSION_TAG: /* this tag always returns */
            {
            /*
            This should handle expansion.
            NOTE: we can encounter both continuations and expansions in an expansion!
            I have to decide where continuations are going to be dealt with
            */
            int32_t firstOffset = (int32_t)(source->pos - source->string);

            // **** doesn't work if using iterator ****
            if (source->offsetReturn != NULL) {
                if (! (source->flags & UCOL_ITER_INNORMBUF) && source->offsetReturn == source->offsetBuffer) {
                    source->offsetStore = source->offsetBuffer;
                }else {
                  firstOffset = -1;
                }
            }

            /* find the offset to expansion table */
            CEOffset = (uint32_t *)coll->image + getExpansionOffset(CE);
            size     = getExpansionCount(CE);
            if (size != 0) {
                /*
                if there are less than 16 elements in expansion, we don't terminate
                */
                uint32_t count;

                for (count = 0; count < size; count++) {
                    *(source->CEpos ++) = *CEOffset++;

                    if (firstOffset >= 0) {
                        source->appendOffset(firstOffset + 1, *status);
                    }
                }
            } else {
                /* else, we do */
                while (*CEOffset != 0) {
                    *(source->CEpos ++) = *CEOffset ++;

                    if (firstOffset >= 0) {
                        source->appendOffset(firstOffset + 1, *status);
                    }
                }
            }

            if (firstOffset >= 0) {
                source->offsetReturn = source->offsetStore - 1;
                *(source->offsetBuffer) = firstOffset;
                if (source->offsetReturn == source->offsetBuffer) {
                    source->offsetStore = source->offsetBuffer;
                }
            } else {
                source->offsetRepeatCount += size - 1;
            }

            source->toReturn = source->CEpos - 1;
            // in case of one element expansion, we
            // want to immediately return CEpos
            if(source->toReturn == source->CEs) {
                source->CEpos = source->CEs;
            }

            return *(source->toReturn);
            }

        case DIGIT_TAG:
            {
                /*
                We do a check to see if we want to collate digits as numbers; if so we generate
                a custom collation key. Otherwise we pull out the value stored in the expansion table.
                */
                uint32_t i;    /* general counter */

                if (source->coll->numericCollation == UCOL_ON){
                    uint32_t digIndx = 0;
                    uint32_t endIndex = 0;
                    uint32_t leadingZeroIndex = 0;
                    uint32_t trailingZeroCount = 0;

                    uint8_t collateVal = 0;

                    UBool nonZeroValReached = FALSE;

                    uint8_t numTempBuf[UCOL_MAX_DIGITS_FOR_NUMBER/2 + 2]; // I just need a temporary place to store my generated CEs.
                    /*
                    We parse the source string until we hit a char that's NOT a digit.
                    Use this u_charDigitValue. This might be slow because we have to
                    handle surrogates...
                    */
                    /*
                    We need to break up the digit string into collection elements of UCOL_MAX_DIGITS_FOR_NUMBER or less,
                    with any chunks smaller than that being on the right end of the digit string - i.e. the first collation
                    element we process when going backward. To determine how long that chunk might be, we may need to make
                    two passes through the loop that collects digits - one to see how long the string is (and how much is
                    leading zeros) to determine the length of that right-hand chunk, and a second (if the whole string has
                    more than UCOL_MAX_DIGITS_FOR_NUMBER non-leading-zero digits) to actually process that collation
                    element chunk after resetting the state to the initialState at the right side of the digit string.
                    */
                    uint32_t ceLimit = 0;
                    UChar initial_ch = ch;
                    collIterateState initialState = {0,0,0,0,0,0,0,0,0};
                    backupState(source, &initialState);

                    for(;;) {
                        collIterateState state = {0,0,0,0,0,0,0,0,0};
                        UChar32 char32 = 0;
                        int32_t digVal = 0;

                        if (U16_IS_TRAIL (ch)) {
                            if (!collIter_bos(source)){
                                UChar lead = getPrevNormalizedChar(source, status);
                                if(U16_IS_LEAD(lead)) {
                                    char32 = U16_GET_SUPPLEMENTARY(lead,ch);
                                    goBackOne(source);
                                } else {
                                    char32 = ch;
                                }
                            } else {
                                char32 = ch;
                            }
                        } else {
                            char32 = ch;
                        }
                        digVal = u_charDigitValue(char32);

                        for(;;) {
                            // Make sure we have enough space. No longer needed;
                            // at this point the largest value of digIndx when we need to save data in numTempBuf
                            // is UCOL_MAX_DIGITS_FOR_NUMBER-1 (digIndx is post-incremented) so we just ensure
                            // that numTempBuf is big enough (UCOL_MAX_DIGITS_FOR_NUMBER/2 + 2).

                            // Skip over trailing zeroes, and keep a count of them.
                            if (digVal != 0)
                                nonZeroValReached = TRUE;

                            if (nonZeroValReached) {
                                /*
                                We parse the digit string into base 100 numbers (this fits into a byte).
                                We only add to the buffer in twos, thus if we are parsing an odd character,
                                that serves as the 'tens' digit while the if we are parsing an even one, that
                                is the 'ones' digit. We dumped the parsed base 100 value (collateVal) into
                                a buffer. We multiply each collateVal by 2 (to give us room) and add 5 (to avoid
                                overlapping magic CE byte values). The last byte we subtract 1 to ensure it is less
                                than all the other bytes.

                                Since we're doing in this reverse we want to put the first digit encountered into the
                                ones place and the second digit encountered into the tens place.
                                */

                                if ((digIndx + trailingZeroCount) % 2 == 1) {
                                    // High-order digit case (tens place)
                                    collateVal += (uint8_t)(digVal * 10);

                                    // We cannot set leadingZeroIndex unless it has been set for the
                                    // low-order digit. Therefore, all we can do for the high-order
                                    // digit is turn it off, never on.
                                    // The only time we will have a high digit without a low is for
                                    // the very first non-zero digit, so no zero check is necessary.
                                    if (collateVal != 0)
                                        leadingZeroIndex = 0;

                                    // The first pass through, digIndx may exceed the limit, but in that case
                                    // we no longer care about numTempBuf contents since they will be discarded
                                    if ( digIndx < UCOL_MAX_DIGITS_FOR_NUMBER ) {
                                        numTempBuf[(digIndx/2) + 2] = collateVal*2 + 6;
                                    }
                                    collateVal = 0;
                                } else {
                                    // Low-order digit case (ones place)
                                    collateVal = (uint8_t)digVal;

                                    // Check for leading zeroes.
                                    if (collateVal == 0) {
                                        if (!leadingZeroIndex)
                                            leadingZeroIndex = (digIndx/2) + 2;
                                    } else
                                        leadingZeroIndex = 0;

                                    // No need to write to buffer; the case of a last odd digit
                                    // is handled below.
                                }
                                ++digIndx;
                            } else
                                ++trailingZeroCount;

                            if (!collIter_bos(source)) {
                                ch = getPrevNormalizedChar(source, status);
                                //goBackOne(source);
                                if (U16_IS_TRAIL(ch)) {
                                    backupState(source, &state);
                                    if (!collIter_bos(source)) {
                                        goBackOne(source);
                                        UChar lead = getPrevNormalizedChar(source, status);

                                        if(U16_IS_LEAD(lead)) {
                                            char32 = U16_GET_SUPPLEMENTARY(lead,ch);
                                        } else {
                                            loadState(source, &state, FALSE);
                                            char32 = ch;
                                        }
                                    }
                                } else
                                    char32 = ch;

                                if ((digVal = u_charDigitValue(char32)) == -1 || (ceLimit > 0 && (digIndx + trailingZeroCount) >= ceLimit)) {
                                    if (char32 > 0xFFFF) {// For surrogates.
                                        loadState(source, &state, FALSE);
                                    }
                                    // Don't need to "reverse" the goBackOne call,
                                    // as this points to the next position to process..
                                    //if (char32 > 0xFFFF) // For surrogates.
                                    //getNextNormalizedChar(source);
                                    break;
                                }

                                goBackOne(source);
                            }else
                                break;
                        }

                        if (digIndx + trailingZeroCount <= UCOL_MAX_DIGITS_FOR_NUMBER) {
                            // our collation element is not too big, go ahead and finish with it
                            break;
                        }
                        // our digit string is too long for a collation element;
                        // set the limit for it, reset the state and begin again
                        ceLimit = (digIndx + trailingZeroCount) % UCOL_MAX_DIGITS_FOR_NUMBER;
                        if ( ceLimit == 0 ) {
                            ceLimit = UCOL_MAX_DIGITS_FOR_NUMBER;
                        }
                        ch = initial_ch;
                        loadState(source, &initialState, FALSE);
                        digIndx = endIndex = leadingZeroIndex = trailingZeroCount = 0;
                        collateVal = 0;
                        nonZeroValReached = FALSE;
                    }

                    if (! nonZeroValReached) {
                        digIndx = 2;
                        trailingZeroCount = 0;
                        numTempBuf[2] = 6;
                    }

                    if ((digIndx + trailingZeroCount) % 2 != 0) {
                        numTempBuf[((digIndx)/2) + 2] = collateVal*2 + 6;
                        digIndx += 1;       // The implicit leading zero
                    }
                    if (trailingZeroCount % 2 != 0) {
                        // We had to consume one trailing zero for the low digit
                        // of the least significant byte
                        digIndx += 1;       // The trailing zero not in the exponent
                        trailingZeroCount -= 1;
                    }

                    endIndex = leadingZeroIndex ? leadingZeroIndex : ((digIndx/2) + 2) ;

                    // Subtract one off of the last byte. Really the first byte here, but it's reversed...
                    numTempBuf[2] -= 1;

                    /*
                    We want to skip over the first two slots in the buffer. The first slot
                    is reserved for the header byte UCOL_CODAN_PLACEHOLDER. The second slot is for the
                    sign/exponent byte: 0x80 + (decimalPos/2) & 7f.
                    The exponent must be adjusted by the number of leading zeroes, and the number of
                    trailing zeroes.
                    */
                    numTempBuf[0] = UCOL_CODAN_PLACEHOLDER;
                    uint32_t exponent = (digIndx+trailingZeroCount)/2;
                    if (leadingZeroIndex)
                        exponent -= ((digIndx/2) + 2 - leadingZeroIndex);
                    numTempBuf[1] = (uint8_t)(0x80 + (exponent & 0x7F));

                    // Now transfer the collation key to our collIterate struct.
                    // The total size for our collation key is half of endIndex, rounded up.
                    int32_t size = (endIndex+1)/2;
                    if(!ensureCEsCapacity(source, size)) {
                        return UCOL_NULLORDER;
                    }
                    *(source->CEpos++) = (((numTempBuf[0] << 8) | numTempBuf[1]) << UCOL_PRIMARYORDERSHIFT) | //Primary weight
                        (UCOL_BYTE_COMMON << UCOL_SECONDARYORDERSHIFT) | // Secondary weight
                        UCOL_BYTE_COMMON; // Tertiary weight.
                    i = endIndex - 1; // Reset the index into the buffer.
                    while(i >= 2) {
                        uint32_t primWeight = numTempBuf[i--] << 8;
                        if ( i >= 2)
                            primWeight |= numTempBuf[i--];
                        *(source->CEpos++) = (primWeight << UCOL_PRIMARYORDERSHIFT) | UCOL_CONTINUATION_MARKER;
                    }

                    source->toReturn = source->CEpos -1;
                    return *(source->toReturn);
                } else {
                    CEOffset = (uint32_t *)coll->image + getExpansionOffset(CE);
                    CE = *(CEOffset++);
                    break;
                }
            }

        case HANGUL_SYLLABLE_TAG: /* AC00-D7AF*/
            {
                static const uint32_t
                    SBase = 0xAC00, LBase = 0x1100, VBase = 0x1161, TBase = 0x11A7;
                //const uint32_t LCount = 19;
                static const uint32_t VCount = 21;
                static const uint32_t TCount = 28;
                //const uint32_t NCount = VCount * TCount;   /* 588 */
                //const uint32_t SCount = LCount * NCount;   /* 11172 */

                uint32_t L = ch - SBase;
                /*
                divide into pieces.
                we do it in this order since some compilers can do % and / in one
                operation
                */
                uint32_t T = L % TCount;
                L /= TCount;
                uint32_t V = L % VCount;
                L /= VCount;

                /* offset them */
                L += LBase;
                V += VBase;
                T += TBase;

                int32_t firstOffset = (int32_t)(source->pos - source->string);
                source->appendOffset(firstOffset, *status);

                /*
                 * return the first CE, but first put the rest into the expansion buffer
                 */
                if (!source->coll->image->jamoSpecial) {
                    *(source->CEpos++) = UTRIE_GET32_FROM_LEAD(&coll->mapping, L);
                    *(source->CEpos++) = UTRIE_GET32_FROM_LEAD(&coll->mapping, V);
                    source->appendOffset(firstOffset + 1, *status);

                    if (T != TBase) {
                        *(source->CEpos++) = UTRIE_GET32_FROM_LEAD(&coll->mapping, T);
                        source->appendOffset(firstOffset + 1, *status);
                    }

                    source->toReturn = source->CEpos - 1;

                    source->offsetReturn = source->offsetStore - 1;
                    if (source->offsetReturn == source->offsetBuffer) {
                        source->offsetStore = source->offsetBuffer;
                    }

                    return *(source->toReturn);
                } else {
                    // Since Hanguls pass the FCD check, it is
                    // guaranteed that we won't be in
                    // the normalization buffer if something like this happens

                    // Move Jamos into normalization buffer
                    UChar *tempbuffer = source->writableBuffer.getBuffer(5);
                    int32_t tempbufferLength, jamoOffset;
                    tempbuffer[0] = 0;
                    tempbuffer[1] = (UChar)L;
                    tempbuffer[2] = (UChar)V;
                    if (T != TBase) {
                        tempbuffer[3] = (UChar)T;
                        tempbufferLength = 4;
                    } else {
                        tempbufferLength = 3;
                    }
                    source->writableBuffer.releaseBuffer(tempbufferLength);

                    // Indicate where to continue in main input string after exhausting the writableBuffer
                    if (source->pos  == source->string) {
                        jamoOffset = 0;
                        source->fcdPosition = NULL;
                    } else {
                        jamoOffset = source->pos - source->string;
                        source->fcdPosition       = source->pos-1;
                    }
                    
					// Append offsets for the additional chars
					// (not the 0, and not the L whose offsets match the original Hangul)
                    int32_t jamoRemaining = tempbufferLength - 2;
                    jamoOffset++; // appended offsets should match end of original Hangul
                    while (jamoRemaining-- > 0) {
                        source->appendOffset(jamoOffset, *status);
                    }

                    source->offsetRepeatValue = jamoOffset;

                    source->offsetReturn = source->offsetStore - 1;
                    if (source->offsetReturn == source->offsetBuffer) {
                        source->offsetStore = source->offsetBuffer;
                    }

                    source->pos               = source->writableBuffer.getTerminatedBuffer() + tempbufferLength;
                    source->origFlags         = source->flags;
                    source->flags            |= UCOL_ITER_INNORMBUF;
                    source->flags            &= ~(UCOL_ITER_NORM | UCOL_ITER_HASLEN);

                    return(UCOL_IGNORABLE);
                }
            }

        case IMPLICIT_TAG:        /* everything that is not defined otherwise */
            return getPrevImplicit(ch, source);

            // TODO: Remove CJK implicits as they are handled by the getImplicitPrimary function
        case CJK_IMPLICIT_TAG:    /* 0x3400-0x4DB5, 0x4E00-0x9FA5, 0xF900-0xFA2D*/
            return getPrevImplicit(ch, source);

        case SURROGATE_TAG:  /* This is a surrogate pair */
            /* essentially an engaged lead surrogate. */
            /* if you have encountered it here, it means that a */
            /* broken sequence was encountered and this is an error */
            return UCOL_NOT_FOUND;

        case LEAD_SURROGATE_TAG:  /* D800-DBFF*/
            return UCOL_NOT_FOUND; /* broken surrogate sequence */

        case TRAIL_SURROGATE_TAG: /* DC00-DFFF*/
            {
                UChar32 cp = 0;
                UChar  prevChar;
                const UChar *prev;
                if (isAtStartPrevIterate(source)) {
                    /* we are at the start of the string, wrong place to be at */
                    return UCOL_NOT_FOUND;
                }
                if (source->pos != source->writableBuffer.getBuffer()) {
                    prev     = source->pos - 1;
                } else {
                    prev     = source->fcdPosition;
                }
                prevChar = *prev;

                /* Handles Han and Supplementary characters here.*/
                if (U16_IS_LEAD(prevChar)) {
                    cp = ((((uint32_t)prevChar)<<10UL)+(ch)-(((uint32_t)0xd800<<10UL)+0xdc00-0x10000));
                    source->pos = prev;
                } else {
                    return UCOL_NOT_FOUND; /* like unassigned */
                }

                return getPrevImplicit(cp, source);
            }

            /* UCA is filled with these. Tailorings are NOT_FOUND */
            /* not yet implemented */
        case CHARSET_TAG:  /* this tag always returns */
            /* probably after 1.8 */
            return UCOL_NOT_FOUND;

        default:           /* this tag always returns */
            *status = U_INTERNAL_PROGRAM_ERROR;
            CE=0;
            break;
        }

        if (CE <= UCOL_NOT_FOUND) {
            break;
        }
    }

    return CE;
}

/* This should really be a macro        */
/* However, it is used only when stack buffers are not sufficiently big, and then we're messed up performance wise */
/* anyway */
static
uint8_t *reallocateBuffer(uint8_t **secondaries, uint8_t *secStart, uint8_t *second, uint32_t *secSize, uint32_t newSize, UErrorCode *status) {
#ifdef UCOL_DEBUG
    fprintf(stderr, ".");
#endif
    uint8_t *newStart = NULL;
    uint32_t offset = (uint32_t)(*secondaries-secStart);

    if(secStart==second) {
        newStart=(uint8_t*)uprv_malloc(newSize);
        if(newStart==NULL) {
            *status = U_MEMORY_ALLOCATION_ERROR;
            return NULL;
        }
        uprv_memcpy(newStart, secStart, *secondaries-secStart);
    } else {
        newStart=(uint8_t*)uprv_realloc(secStart, newSize);
        if(newStart==NULL) {
            *status = U_MEMORY_ALLOCATION_ERROR;
            /* Since we're reallocating, return original reference so we don't loose it. */
            return secStart;
        }
    }
    *secondaries=newStart+offset;
    *secSize=newSize;
    return newStart;
}


/* This should really be a macro                                                                      */
/* This function is used to reverse parts of a buffer. We need this operation when doing continuation */
/* secondaries in French                                                                              */
/*
void uprv_ucol_reverse_buffer(uint8_t *start, uint8_t *end) {
  uint8_t temp;
  while(start<end) {
    temp = *start;
    *start++ = *end;
    *end-- = temp;
  }
}
*/

#define uprv_ucol_reverse_buffer(TYPE, start, end) { \
  TYPE tempA; \
while((start)<(end)) { \
    tempA = *(start); \
    *(start)++ = *(end); \
    *(end)-- = tempA; \
} \
}

/****************************************************************************/
/* Following are the sortkey generation functions                           */
/*                                                                          */
/****************************************************************************/

/**
 * Merge two sort keys.
 * This is useful, for example, to combine sort keys from first and last names
 * to sort such pairs.
 * Merged sort keys consider on each collation level the first part first entirely,
 * then the second one.
 * It is possible to merge multiple sort keys by consecutively merging
 * another one with the intermediate result.
 *
 * The length of the merge result is the sum of the lengths of the input sort keys
 * minus 1.
 *
 * @param src1 the first sort key
 * @param src1Length the length of the first sort key, including the zero byte at the end;
 *        can be -1 if the function is to find the length
 * @param src2 the second sort key
 * @param src2Length the length of the second sort key, including the zero byte at the end;
 *        can be -1 if the function is to find the length
 * @param dest the buffer where the merged sort key is written,
 *        can be NULL if destCapacity==0
 * @param destCapacity the number of bytes in the dest buffer
 * @return the length of the merged sort key, src1Length+src2Length-1;
 *         can be larger than destCapacity, or 0 if an error occurs (only for illegal arguments),
 *         in which cases the contents of dest is undefined
 *
 * @draft
 */
U_CAPI int32_t U_EXPORT2
ucol_mergeSortkeys(const uint8_t *src1, int32_t src1Length,
                   const uint8_t *src2, int32_t src2Length,
                   uint8_t *dest, int32_t destCapacity) {
    int32_t destLength;
    uint8_t b;

    /* check arguments */
    if( src1==NULL || src1Length<-2 || src1Length==0 || (src1Length>0 && src1[src1Length-1]!=0) ||
        src2==NULL || src2Length<-2 || src2Length==0 || (src2Length>0 && src2[src2Length-1]!=0) ||
        destCapacity<0 || (destCapacity>0 && dest==NULL)
    ) {
        /* error, attempt to write a zero byte and return 0 */
        if(dest!=NULL && destCapacity>0) {
            *dest=0;
        }
        return 0;
    }

    /* check lengths and capacity */
    if(src1Length<0) {
        src1Length=(int32_t)uprv_strlen((const char *)src1)+1;
    }
    if(src2Length<0) {
        src2Length=(int32_t)uprv_strlen((const char *)src2)+1;
    }

    destLength=src1Length+src2Length-1;
    if(destLength>destCapacity) {
        /* the merged sort key does not fit into the destination */
        return destLength;
    }

    /* merge the sort keys with the same number of levels */
    while(*src1!=0 && *src2!=0) { /* while both have another level */
        /* copy level from src1 not including 00 or 01 */
        while((b=*src1)>=2) {
            ++src1;
            *dest++=b;
        }

        /* add a 02 merge separator */
        *dest++=2;

        /* copy level from src2 not including 00 or 01 */
        while((b=*src2)>=2) {
            ++src2;
            *dest++=b;
        }

        /* if both sort keys have another level, then add a 01 level separator and continue */
        if(*src1==1 && *src2==1) {
            ++src1;
            ++src2;
            *dest++=1;
        }
    }

    /*
     * here, at least one sort key is finished now, but the other one
     * might have some contents left from containing more levels;
     * that contents is just appended to the result
     */
    if(*src1!=0) {
        /* src1 is not finished, therefore *src2==0, and src1 is appended */
        src2=src1;
    }
    /* append src2, "the other, unfinished sort key" */
    uprv_strcpy((char *)dest, (const char *)src2);

    /* trust that neither sort key contained illegally embedded zero bytes */
    return destLength;
}

/* sortkey API */
U_CAPI int32_t U_EXPORT2
ucol_getSortKey(const    UCollator    *coll,
        const    UChar        *source,
        int32_t        sourceLength,
        uint8_t        *result,
        int32_t        resultLength)
{
    UTRACE_ENTRY(UTRACE_UCOL_GET_SORTKEY);
    if (UTRACE_LEVEL(UTRACE_VERBOSE)) {
        UTRACE_DATA3(UTRACE_VERBOSE, "coll=%p, source string = %vh ", coll, source,
            ((sourceLength==-1 && source!=NULL) ? u_strlen(source) : sourceLength));
    }

    UErrorCode status = U_ZERO_ERROR;
    int32_t keySize   = 0;

    if(source != NULL) {
        // source == NULL is actually an error situation, but we would need to
        // have an error code to return it. Until we introduce a new
        // API, it stays like this

        /* this uses the function pointer that is set in updateinternalstate */
        /* currently, there are two funcs: */
        /*ucol_calcSortKey(...);*/
        /*ucol_calcSortKeySimpleTertiary(...);*/

        keySize = coll->sortKeyGen(coll, source, sourceLength, &result, resultLength, FALSE, &status);
        //if (U_FAILURE(status) && status != U_BUFFER_OVERFLOW_ERROR && result && resultLength > 0) {
            // That's not good. Something unusual happened.
            // We don't know how much we initialized before we failed.
            // NULL terminate for safety.
            // We have no way say that we have generated a partial sort key.
            //result[0] = 0;
            //keySize = 0;
        //}
    }
    UTRACE_DATA2(UTRACE_VERBOSE, "Sort Key = %vb", result, keySize);
    UTRACE_EXIT_STATUS(status);
    return keySize;
}

/* this function is called by the C++ API for sortkey generation */
U_CFUNC int32_t
ucol_getSortKeyWithAllocation(const UCollator *coll,
                              const UChar *source, int32_t sourceLength,
                              uint8_t **pResult,
                              UErrorCode *pErrorCode) {
    *pResult = 0;
    return coll->sortKeyGen(coll, source, sourceLength, pResult, 0, TRUE, pErrorCode);
}

#define UCOL_FSEC_BUF_SIZE 256

// Is this primary weight compressible?
// Returns false for multi-lead-byte scripts (digits, Latin, Han, implicit).
// TODO: This should use per-lead-byte flags from FractionalUCA.txt.
static inline UBool
isCompressible(const UCollator * /*coll*/, uint8_t primary1) {
    return UCOL_BYTE_FIRST_NON_LATIN_PRIMARY <= primary1 && primary1 <= maxRegularPrimary;
}

/* This function tries to get the size of a sortkey. It will be invoked if the size of resulting buffer is 0  */
/* or if we run out of space while making a sortkey and want to return ASAP                                   */
int32_t ucol_getSortKeySize(const UCollator *coll, collIterate *s, int32_t currentSize, UColAttributeValue strength, int32_t len) {
    UErrorCode status = U_ZERO_ERROR;
    //const UCAConstants *UCAconsts = (UCAConstants *)((uint8_t *)coll->UCA->image + coll->image->UCAConsts);
    uint8_t compareSec   = (uint8_t)((strength >= UCOL_SECONDARY)?0:0xFF);
    uint8_t compareTer   = (uint8_t)((strength >= UCOL_TERTIARY)?0:0xFF);
    uint8_t compareQuad  = (uint8_t)((strength >= UCOL_QUATERNARY)?0:0xFF);
    UBool  compareIdent = (strength == UCOL_IDENTICAL);
    UBool  doCase = (coll->caseLevel == UCOL_ON);
    UBool  shifted = (coll->alternateHandling == UCOL_SHIFTED);
    //UBool  qShifted = shifted  && (compareQuad == 0);
    UBool  doHiragana = (coll->hiraganaQ == UCOL_ON) && (compareQuad == 0);
    UBool  isFrenchSec = (coll->frenchCollation == UCOL_ON) && (compareSec == 0);
    uint8_t fSecsBuff[UCOL_FSEC_BUF_SIZE];
    uint8_t *fSecs = fSecsBuff;
    uint32_t fSecsLen = 0, fSecsMaxLen = UCOL_FSEC_BUF_SIZE;
    uint8_t *frenchStartPtr = NULL, *frenchEndPtr = NULL;

    uint32_t variableTopValue = coll->variableTopValue;
    uint8_t UCOL_COMMON_BOT4 = (uint8_t)((coll->variableTopValue>>8)+1);
    if(doHiragana) {
        UCOL_COMMON_BOT4++;
        /* allocate one more space for hiragana */
    }
    uint8_t UCOL_BOT_COUNT4 = (uint8_t)(0xFF - UCOL_COMMON_BOT4);

    uint32_t order = UCOL_NO_MORE_CES;
    uint8_t primary1 = 0;
    uint8_t primary2 = 0;
    uint8_t secondary = 0;
    uint8_t tertiary = 0;
    int32_t caseShift = 0;
    uint32_t c2 = 0, c3 = 0, c4 = 0; /* variables for compression */

    uint8_t caseSwitch = coll->caseSwitch;
    uint8_t tertiaryMask = coll->tertiaryMask;
    uint8_t tertiaryCommon = coll->tertiaryCommon;

    UBool wasShifted = FALSE;
    UBool notIsContinuation = FALSE;
    uint8_t leadPrimary = 0;


    for(;;) {
        order = ucol_IGetNextCE(coll, s, &status);
        if(order == UCOL_NO_MORE_CES) {
            break;
        }

        if(order == 0) {
            continue;
        }

        notIsContinuation = !isContinuation(order);


        if(notIsContinuation) {
            tertiary = (uint8_t)((order & UCOL_BYTE_SIZE_MASK));
        } else {
            tertiary = (uint8_t)((order & UCOL_REMOVE_CONTINUATION));
        }
        secondary = (uint8_t)((order >>= 8) & UCOL_BYTE_SIZE_MASK);
        primary2 = (uint8_t)((order >>= 8) & UCOL_BYTE_SIZE_MASK);
        primary1 = (uint8_t)(order >> 8);

        /* no need to permute since the actual code values don't matter
        if (coll->leadBytePermutationTable != NULL && notIsContinuation) {
            primary1 = coll->leadBytePermutationTable[primary1];
        }
        */

        if((shifted && ((notIsContinuation && order <= variableTopValue && primary1 > 0)
                      || (!notIsContinuation && wasShifted)))
            || (wasShifted && primary1 == 0)) { /* amendment to the UCA says that primary ignorables */
                /* and other ignorables should be removed if following a shifted code point */
                if(primary1 == 0) { /* if we were shifted and we got an ignorable code point */
                    /* we should just completely ignore it */
                    continue;
                }
                if(compareQuad == 0) {
                    if(c4 > 0) {
                        currentSize += (c2/UCOL_BOT_COUNT4)+1;
                        c4 = 0;
                    }
                    currentSize++;
                    if(primary2 != 0) {
                        currentSize++;
                    }
                }
                wasShifted = TRUE;
        } else {
            wasShifted = FALSE;
            /* Note: This code assumes that the table is well built i.e. not having 0 bytes where they are not supposed to be. */
            /* Usually, we'll have non-zero primary1 & primary2, except in cases of a-z and friends, when primary2 will   */
            /* calculate sortkey size */
            if(primary1 != UCOL_IGNORABLE) {
                if(notIsContinuation) {
                    if(leadPrimary == primary1) {
                        currentSize++;
                    } else {
                        if(leadPrimary != 0) {
                            currentSize++;
                        }
                        if(primary2 == UCOL_IGNORABLE) {
                            /* one byter, not compressed */
                            currentSize++;
                            leadPrimary = 0;
                        } else if(isCompressible(coll, primary1)) {
                            /* compress */
                            leadPrimary = primary1;
                            currentSize+=2;
                        } else {
                            leadPrimary = 0;
                            currentSize+=2;
                        }
                    }
                } else { /* we are in continuation, so we're gonna add primary to the key don't care about compression */
                    currentSize++;
                    if(primary2 != UCOL_IGNORABLE) {
                        currentSize++;
                    }
                }
            }

            if(secondary > compareSec) { /* I think that != 0 test should be != IGNORABLE */
                if(!isFrenchSec){
                    if (secondary == UCOL_COMMON2 && notIsContinuation) {
                        c2++;
                    } else {
                        if(c2 > 0) {
                            if (secondary > UCOL_COMMON2) { // not necessary for 4th level.
                                currentSize += (c2/(uint32_t)UCOL_TOP_COUNT2)+1;
                            } else {
                                currentSize += (c2/(uint32_t)UCOL_BOT_COUNT2)+1;
                            }
                            c2 = 0;
                        }
                        currentSize++;
                    }
                } else {
                    fSecs[fSecsLen++] = secondary;
                    if(fSecsLen == fSecsMaxLen) {
                        uint8_t *fSecsTemp;
                        if(fSecs == fSecsBuff) {
                            fSecsTemp = (uint8_t *)uprv_malloc(2*fSecsLen);
                        } else {
                            fSecsTemp = (uint8_t *)uprv_realloc(fSecs, 2*fSecsLen);
                        }
                        if(fSecsTemp == NULL) {
                            status = U_MEMORY_ALLOCATION_ERROR;
                            return 0;
                        }
                        fSecs = fSecsTemp;
                        fSecsMaxLen *= 2;
                    }
                    if(notIsContinuation) {
                        if (frenchStartPtr != NULL) {
                            /* reverse secondaries from frenchStartPtr up to frenchEndPtr */
                            uprv_ucol_reverse_buffer(uint8_t, frenchStartPtr, frenchEndPtr);
                            frenchStartPtr = NULL;
                        }
                    } else {
                        if (frenchStartPtr == NULL) {
                            frenchStartPtr = fSecs+fSecsLen-2;
                        }
                        frenchEndPtr = fSecs+fSecsLen-1;
                    }
                }
            }

            if(doCase && (primary1 > 0 || strength >= UCOL_SECONDARY)) {
                // do the case level if we need to do it. We don't want to calculate
                // case level for primary ignorables if we have only primary strength and case level
                // otherwise we would break well formedness of CEs
                if (caseShift  == 0) {
                    currentSize++;
                    caseShift = UCOL_CASE_SHIFT_START;
                }
                if((tertiary&0x3F) > 0 && notIsContinuation) {
                    caseShift--;
                    if((tertiary &0xC0) != 0) {
                        if (caseShift  == 0) {
                            currentSize++;
                            caseShift = UCOL_CASE_SHIFT_START;
                        }
                        caseShift--;
                    }
                }
            } else {
                if(notIsContinuation) {
                    tertiary ^= caseSwitch;
                }
            }

            tertiary &= tertiaryMask;
            if(tertiary > compareTer) { /* I think that != 0 test should be != IGNORABLE */
                if (tertiary == tertiaryCommon && notIsContinuation) {
                    c3++;
                } else {
                    if(c3 > 0) {
                        if((tertiary > tertiaryCommon && tertiaryCommon == UCOL_COMMON3_NORMAL)
                            || (tertiary <= tertiaryCommon && tertiaryCommon == UCOL_COMMON3_UPPERFIRST)) {
                                currentSize += (c3/(uint32_t)coll->tertiaryTopCount)+1;
                        } else {
                            currentSize += (c3/(uint32_t)coll->tertiaryBottomCount)+1;
                        }
                        c3 = 0;
                    }
                    currentSize++;
                }
            }

            if(/*qShifted*/(compareQuad==0)  && notIsContinuation) {
                if(s->flags & UCOL_WAS_HIRAGANA) { // This was Hiragana and we need to note it
                    if(c4>0) { // Close this part
                        currentSize += (c4/UCOL_BOT_COUNT4)+1;
                        c4 = 0;
                    }
                    currentSize++; // Add the Hiragana
                } else { // This wasn't Hiragana, so we can continue adding stuff
                    c4++;
                }
            }
        }
    }

    if(!isFrenchSec){
        if(c2 > 0) {
            currentSize += (c2/(uint32_t)UCOL_BOT_COUNT2)+((c2%(uint32_t)UCOL_BOT_COUNT2 != 0)?1:0);
        }
    } else {
        uint32_t i = 0;
        if(frenchStartPtr != NULL) {
            uprv_ucol_reverse_buffer(uint8_t, frenchStartPtr, frenchEndPtr);
        }
        for(i = 0; i<fSecsLen; i++) {
            secondary = *(fSecs+fSecsLen-i-1);
            /* This is compression code. */
            if (secondary == UCOL_COMMON2) {
                ++c2;
            } else {
                if(c2 > 0) {
                    if (secondary > UCOL_COMMON2) { // not necessary for 4th level.
                        currentSize += (c2/(uint32_t)UCOL_TOP_COUNT2)+((c2%(uint32_t)UCOL_TOP_COUNT2 != 0)?1:0);
                    } else {
                        currentSize += (c2/(uint32_t)UCOL_BOT_COUNT2)+((c2%(uint32_t)UCOL_BOT_COUNT2 != 0)?1:0);
                    }
                    c2 = 0;
                }
                currentSize++;
            }
        }
        if(c2 > 0) {
            currentSize += (c2/(uint32_t)UCOL_BOT_COUNT2)+((c2%(uint32_t)UCOL_BOT_COUNT2 != 0)?1:0);
        }
        if(fSecs != fSecsBuff) {
            uprv_free(fSecs);
        }
    }

    if(c3 > 0) {
        currentSize += (c3/(uint32_t)coll->tertiaryBottomCount) + ((c3%(uint32_t)coll->tertiaryBottomCount != 0)?1:0);
    }

    if(c4 > 0  && compareQuad == 0) {
        currentSize += (c4/(uint32_t)UCOL_BOT_COUNT4)+((c4%(uint32_t)UCOL_BOT_COUNT4 != 0)?1:0);
    }

    if(compareIdent) {
        currentSize += u_lengthOfIdenticalLevelRun(s->string, len);
    }
    return currentSize;
}

static
inline void doCaseShift(uint8_t **cases, uint32_t &caseShift) {
    if (caseShift  == 0) {
        *(*cases)++ = UCOL_CASE_BYTE_START;
        caseShift = UCOL_CASE_SHIFT_START;
    }
}

// Adds a value to the buffer if it's safe to add. Increments the number of added values, so that we
// know how many values we wanted to add, even if we didn't add them all
static
inline void addWithIncrement(uint8_t *&primaries, uint8_t *limit, uint32_t &size, const uint8_t value) {
    size++;
    if(primaries < limit) {
        *(primaries)++ = value;
    }
}

// Packs the secondary buffer when processing French locale. Adds the terminator.
static
inline uint8_t *packFrench(uint8_t *primaries, uint8_t *primEnd, uint8_t *secondaries, uint32_t *secsize, uint8_t *frenchStartPtr, uint8_t *frenchEndPtr) {
    uint8_t secondary;
    int32_t count2 = 0;
    uint32_t i = 0, size = 0;
    // we use i here since the key size already accounts for terminators, so we'll discard the increment
    addWithIncrement(primaries, primEnd, i, UCOL_LEVELTERMINATOR);
    /* If there are any unresolved continuation secondaries, reverse them here so that we can reverse the whole secondary thing */
    if(frenchStartPtr != NULL) {
        uprv_ucol_reverse_buffer(uint8_t, frenchStartPtr, frenchEndPtr);
    }
    for(i = 0; i<*secsize; i++) {
        secondary = *(secondaries-i-1);
        /* This is compression code. */
        if (secondary == UCOL_COMMON2) {
            ++count2;
        } else {
            if (count2 > 0) {
                if (secondary > UCOL_COMMON2) { // not necessary for 4th level.
                    while (count2 > UCOL_TOP_COUNT2) {
                        addWithIncrement(primaries, primEnd, size, (uint8_t)(UCOL_COMMON_TOP2 - UCOL_TOP_COUNT2));
                        count2 -= (uint32_t)UCOL_TOP_COUNT2;
                    }
                    addWithIncrement(primaries, primEnd, size, (uint8_t)(UCOL_COMMON_TOP2 - (count2-1)));
                } else {
                    while (count2 > UCOL_BOT_COUNT2) {
                        addWithIncrement(primaries, primEnd, size, (uint8_t)(UCOL_COMMON_BOT2 + UCOL_BOT_COUNT2));
                        count2 -= (uint32_t)UCOL_BOT_COUNT2;
                    }
                    addWithIncrement(primaries, primEnd, size, (uint8_t)(UCOL_COMMON_BOT2 + (count2-1)));
                }
                count2 = 0;
            }
            addWithIncrement(primaries, primEnd, size, secondary);
        }
    }
    if (count2 > 0) {
        while (count2 > UCOL_BOT_COUNT2) {
            addWithIncrement(primaries, primEnd, size, (uint8_t)(UCOL_COMMON_BOT2 + UCOL_BOT_COUNT2));
            count2 -= (uint32_t)UCOL_BOT_COUNT2;
        }
        addWithIncrement(primaries, primEnd, size, (uint8_t)(UCOL_COMMON_BOT2 + (count2-1)));
    }
    *secsize = size;
    return primaries;
}

#define DEFAULT_ERROR_SIZE_FOR_CALCSORTKEY 0

/* This is the sortkey work horse function */
U_CFUNC int32_t U_CALLCONV
ucol_calcSortKey(const    UCollator    *coll,
        const    UChar        *source,
        int32_t        sourceLength,
        uint8_t        **result,
        uint32_t        resultLength,
        UBool allocateSKBuffer,
        UErrorCode *status)
{
    //const UCAConstants *UCAconsts = (UCAConstants *)((uint8_t *)coll->UCA->image + coll->image->UCAConsts);

    uint32_t i = 0; /* general purpose counter */

    /* Stack allocated buffers for buffers we use */
    uint8_t prim[UCOL_PRIMARY_MAX_BUFFER], second[UCOL_SECONDARY_MAX_BUFFER], tert[UCOL_TERTIARY_MAX_BUFFER], caseB[UCOL_CASE_MAX_BUFFER], quad[UCOL_QUAD_MAX_BUFFER];

    uint8_t *primaries = *result, *secondaries = second, *tertiaries = tert, *cases = caseB, *quads = quad;

    if(U_FAILURE(*status)) {
        return 0;
    }

    if(primaries == NULL && allocateSKBuffer == TRUE) {
        primaries = *result = prim;
        resultLength = UCOL_PRIMARY_MAX_BUFFER;
    }

    uint32_t secSize = UCOL_SECONDARY_MAX_BUFFER, terSize = UCOL_TERTIARY_MAX_BUFFER,
      caseSize = UCOL_CASE_MAX_BUFFER, quadSize = UCOL_QUAD_MAX_BUFFER;

    uint32_t sortKeySize = 1; /* it is always \0 terminated */

    UnicodeString normSource;

    int32_t len = (sourceLength == -1 ? u_strlen(source) : sourceLength);

    UColAttributeValue strength = coll->strength;

    uint8_t compareSec   = (uint8_t)((strength >= UCOL_SECONDARY)?0:0xFF);
    uint8_t compareTer   = (uint8_t)((strength >= UCOL_TERTIARY)?0:0xFF);
    uint8_t compareQuad  = (uint8_t)((strength >= UCOL_QUATERNARY)?0:0xFF);
    UBool  compareIdent = (strength == UCOL_IDENTICAL);
    UBool  doCase = (coll->caseLevel == UCOL_ON);
    UBool  isFrenchSec = (coll->frenchCollation == UCOL_ON) && (compareSec == 0);
    UBool  shifted = (coll->alternateHandling == UCOL_SHIFTED);
    //UBool  qShifted = shifted && (compareQuad == 0);
    UBool  doHiragana = (coll->hiraganaQ == UCOL_ON) && (compareQuad == 0);

    uint32_t variableTopValue = coll->variableTopValue;
    // TODO: UCOL_COMMON_BOT4 should be a function of qShifted. If we have no
    // qShifted, we don't need to set UCOL_COMMON_BOT4 so high.
    uint8_t UCOL_COMMON_BOT4 = (uint8_t)((coll->variableTopValue>>8)+1);
    uint8_t UCOL_HIRAGANA_QUAD = 0;
    if(doHiragana) {
        UCOL_HIRAGANA_QUAD=UCOL_COMMON_BOT4++;
        /* allocate one more space for hiragana, value for hiragana */
    }
    uint8_t UCOL_BOT_COUNT4 = (uint8_t)(0xFF - UCOL_COMMON_BOT4);

    /* support for special features like caselevel and funky secondaries */
    uint8_t *frenchStartPtr = NULL;
    uint8_t *frenchEndPtr = NULL;
    uint32_t caseShift = 0;

    sortKeySize += ((compareSec?0:1) + (compareTer?0:1) + (doCase?1:0) + /*(qShifted?1:0)*/(compareQuad?0:1) + (compareIdent?1:0));

    /* If we need to normalize, we'll do it all at once at the beginning! */
    const Normalizer2 *norm2;
    if(compareIdent) {
        norm2 = Normalizer2Factory::getNFDInstance(*status);
    } else if(coll->normalizationMode != UCOL_OFF) {
        norm2 = Normalizer2Factory::getFCDInstance(*status);
    } else {
        norm2 = NULL;
    }
    if(norm2 != NULL) {
        normSource.setTo(FALSE, source, len);
        int32_t qcYesLength = norm2->spanQuickCheckYes(normSource, *status);
        if(qcYesLength != len) {
            UnicodeString unnormalized = normSource.tempSubString(qcYesLength);
            normSource.truncate(qcYesLength);
            norm2->normalizeSecondAndAppend(normSource, unnormalized, *status);
            source = normSource.getBuffer();
            len = normSource.length();
        }
    }
    collIterate s;
    IInit_collIterate(coll, source, len, &s, status);
    if(U_FAILURE(*status)) {
        return 0;
    }
    s.flags &= ~UCOL_ITER_NORM;  // source passed the FCD test or else was normalized.

    if(resultLength == 0 || primaries == NULL) {
        return ucol_getSortKeySize(coll, &s, sortKeySize, strength, len);
    }
    uint8_t *primarySafeEnd = primaries + resultLength - 1;
    if(strength > UCOL_PRIMARY) {
        primarySafeEnd--;
    }

    uint32_t minBufferSize = UCOL_MAX_BUFFER;

    uint8_t *primStart = primaries;
    uint8_t *secStart = secondaries;
    uint8_t *terStart = tertiaries;
    uint8_t *caseStart = cases;
    uint8_t *quadStart = quads;

    uint32_t order = 0;

    uint8_t primary1 = 0;
    uint8_t primary2 = 0;
    uint8_t secondary = 0;
    uint8_t tertiary = 0;
    uint8_t caseSwitch = coll->caseSwitch;
    uint8_t tertiaryMask = coll->tertiaryMask;
    int8_t tertiaryAddition = coll->tertiaryAddition;
    uint8_t tertiaryTop = coll->tertiaryTop;
    uint8_t tertiaryBottom = coll->tertiaryBottom;
    uint8_t tertiaryCommon = coll->tertiaryCommon;
    uint8_t caseBits = 0;

    UBool finished = FALSE;
    UBool wasShifted = FALSE;
    UBool notIsContinuation = FALSE;

    uint32_t prevBuffSize = 0;

    uint32_t count2 = 0, count3 = 0, count4 = 0;
    uint8_t leadPrimary = 0;

    for(;;) {
        for(i=prevBuffSize; i<minBufferSize; ++i) {

            order = ucol_IGetNextCE(coll, &s, status);
            if(order == UCOL_NO_MORE_CES) {
                finished = TRUE;
                break;
            }

            if(order == 0) {
                continue;
            }

            notIsContinuation = !isContinuation(order);

            if(notIsContinuation) {
                tertiary = (uint8_t)(order & UCOL_BYTE_SIZE_MASK);
            } else {
                tertiary = (uint8_t)((order & UCOL_REMOVE_CONTINUATION));
            }

            secondary = (uint8_t)((order >>= 8) & UCOL_BYTE_SIZE_MASK);
            primary2 = (uint8_t)((order >>= 8) & UCOL_BYTE_SIZE_MASK);
            primary1 = (uint8_t)(order >> 8);

            uint8_t originalPrimary1 = primary1;
            if(notIsContinuation && coll->leadBytePermutationTable != NULL) {
                primary1 = coll->leadBytePermutationTable[primary1];
            }

            if((shifted && ((notIsContinuation && order <= variableTopValue && primary1 > 0)
                           || (!notIsContinuation && wasShifted)))
                || (wasShifted && primary1 == 0)) /* amendment to the UCA says that primary ignorables */
            {
                /* and other ignorables should be removed if following a shifted code point */
                if(primary1 == 0) { /* if we were shifted and we got an ignorable code point */
                    /* we should just completely ignore it */
                    continue;
                }
                if(compareQuad == 0) {
                    if(count4 > 0) {
                        while (count4 > UCOL_BOT_COUNT4) {
                            *quads++ = (uint8_t)(UCOL_COMMON_BOT4 + UCOL_BOT_COUNT4);
                            count4 -= UCOL_BOT_COUNT4;
                        }
                        *quads++ = (uint8_t)(UCOL_COMMON_BOT4 + (count4-1));
                        count4 = 0;
                    }
                    /* We are dealing with a variable and we're treating them as shifted */
                    /* This is a shifted ignorable */
                    if(primary1 != 0) { /* we need to check this since we could be in continuation */
                        *quads++ = primary1;
                    }
                    if(primary2 != 0) {
                        *quads++ = primary2;
                    }
                }
                wasShifted = TRUE;
            } else {
                wasShifted = FALSE;
                /* Note: This code assumes that the table is well built i.e. not having 0 bytes where they are not supposed to be. */
                /* Usually, we'll have non-zero primary1 & primary2, except in cases of a-z and friends, when primary2 will   */
                /* regular and simple sortkey calc */
                if(primary1 != UCOL_IGNORABLE) {
                    if(notIsContinuation) {
                        if(leadPrimary == primary1) {
                            *primaries++ = primary2;
                        } else {
                            if(leadPrimary != 0) {
                                *primaries++ = (uint8_t)((primary1 > leadPrimary) ? UCOL_BYTE_UNSHIFTED_MAX : UCOL_BYTE_UNSHIFTED_MIN);
                            }
                            if(primary2 == UCOL_IGNORABLE) {
                                /* one byter, not compressed */
                                *primaries++ = primary1;
                                leadPrimary = 0;
                            } else if(isCompressible(coll, originalPrimary1)) {
                                /* compress */
                                *primaries++ = leadPrimary = primary1;
                                if(primaries <= primarySafeEnd) {
                                    *primaries++ = primary2;
                                }
                            } else {
                                leadPrimary = 0;
                                *primaries++ = primary1;
                                if(primaries <= primarySafeEnd) {
                                    *primaries++ = primary2;
                                }
                            }
                        }
                    } else { /* we are in continuation, so we're gonna add primary to the key don't care about compression */
                        *primaries++ = primary1;
                        if((primary2 != UCOL_IGNORABLE) && (primaries <= primarySafeEnd)) {
                                *primaries++ = primary2; /* second part */
                        }
                    }
                }

                if(secondary > compareSec) {
                    if(!isFrenchSec) {
                        /* This is compression code. */
                        if (secondary == UCOL_COMMON2 && notIsContinuation) {
                            ++count2;
                        } else {
                            if (count2 > 0) {
                                if (secondary > UCOL_COMMON2) { // not necessary for 4th level.
                                    while (count2 > UCOL_TOP_COUNT2) {
                                        *secondaries++ = (uint8_t)(UCOL_COMMON_TOP2 - UCOL_TOP_COUNT2);
                                        count2 -= (uint32_t)UCOL_TOP_COUNT2;
                                    }
                                    *secondaries++ = (uint8_t)(UCOL_COMMON_TOP2 - (count2-1));
                                } else {
                                    while (count2 > UCOL_BOT_COUNT2) {
                                        *secondaries++ = (uint8_t)(UCOL_COMMON_BOT2 + UCOL_BOT_COUNT2);
                                        count2 -= (uint32_t)UCOL_BOT_COUNT2;
                                    }
                                    *secondaries++ = (uint8_t)(UCOL_COMMON_BOT2 + (count2-1));
                                }
                                count2 = 0;
                            }
                            *secondaries++ = secondary;
                        }
                    } else {
                        *secondaries++ = secondary;
                        /* Do the special handling for French secondaries */
                        /* We need to get continuation elements and do intermediate restore */
                        /* abc1c2c3de with french secondaries need to be edc1c2c3ba NOT edc3c2c1ba */
                        if(notIsContinuation) {
                            if (frenchStartPtr != NULL) {
                                /* reverse secondaries from frenchStartPtr up to frenchEndPtr */
                                uprv_ucol_reverse_buffer(uint8_t, frenchStartPtr, frenchEndPtr);
                                frenchStartPtr = NULL;
                            }
                        } else {
                            if (frenchStartPtr == NULL) {
                                frenchStartPtr = secondaries - 2;
                            }
                            frenchEndPtr = secondaries-1;
                        }
                    }
                }

                if(doCase && (primary1 > 0 || strength >= UCOL_SECONDARY)) {
                    // do the case level if we need to do it. We don't want to calculate
                    // case level for primary ignorables if we have only primary strength and case level
                    // otherwise we would break well formedness of CEs
                    doCaseShift(&cases, caseShift);
                    if(notIsContinuation) {
                        caseBits = (uint8_t)(tertiary & 0xC0);

                        if(tertiary != 0) {
                            if(coll->caseFirst == UCOL_UPPER_FIRST) {
                                if((caseBits & 0xC0) == 0) {
                                    *(cases-1) |= 1 << (--caseShift);
                                } else {
                                    *(cases-1) |= 0 << (--caseShift);
                                    /* second bit */
                                    doCaseShift(&cases, caseShift);
                                    *(cases-1) |= ((caseBits>>6)&1) << (--caseShift);
                                }
                            } else {
                                if((caseBits & 0xC0) == 0) {
                                    *(cases-1) |= 0 << (--caseShift);
                                } else {
                                    *(cases-1) |= 1 << (--caseShift);
                                    /* second bit */
                                    doCaseShift(&cases, caseShift);
                                    *(cases-1) |= ((caseBits>>7)&1) << (--caseShift);
                                }
                            }
                        }

                    }
                } else {
                    if(notIsContinuation) {
                        tertiary ^= caseSwitch;
                    }
                }

                tertiary &= tertiaryMask;
                if(tertiary > compareTer) {
                    /* This is compression code. */
                    /* sequence size check is included in the if clause */
                    if (tertiary == tertiaryCommon && notIsContinuation) {
                        ++count3;
                    } else {
                        if(tertiary > tertiaryCommon && tertiaryCommon == UCOL_COMMON3_NORMAL) {
                            tertiary += tertiaryAddition;
                        } else if(tertiary <= tertiaryCommon && tertiaryCommon == UCOL_COMMON3_UPPERFIRST) {
                            tertiary -= tertiaryAddition;
                        }
                        if (count3 > 0) {
                            if ((tertiary > tertiaryCommon)) {
                                while (count3 > coll->tertiaryTopCount) {
                                    *tertiaries++ = (uint8_t)(tertiaryTop - coll->tertiaryTopCount);
                                    count3 -= (uint32_t)coll->tertiaryTopCount;
                                }
                                *tertiaries++ = (uint8_t)(tertiaryTop - (count3-1));
                            } else {
                                while (count3 > coll->tertiaryBottomCount) {
                                    *tertiaries++ = (uint8_t)(tertiaryBottom + coll->tertiaryBottomCount);
                                    count3 -= (uint32_t)coll->tertiaryBottomCount;
                                }
                                *tertiaries++ = (uint8_t)(tertiaryBottom + (count3-1));
                            }
                            count3 = 0;
                        }
                        *tertiaries++ = tertiary;
                    }
                }

                if(/*qShifted*/(compareQuad==0)  && notIsContinuation) {
                    if(s.flags & UCOL_WAS_HIRAGANA) { // This was Hiragana and we need to note it
                        if(count4>0) { // Close this part
                            while (count4 > UCOL_BOT_COUNT4) {
                                *quads++ = (uint8_t)(UCOL_COMMON_BOT4 + UCOL_BOT_COUNT4);
                                count4 -= UCOL_BOT_COUNT4;
                            }
                            *quads++ = (uint8_t)(UCOL_COMMON_BOT4 + (count4-1));
                            count4 = 0;
                        }
                        *quads++ = UCOL_HIRAGANA_QUAD; // Add the Hiragana
                    } else { // This wasn't Hiragana, so we can continue adding stuff
                        count4++;
                    }
                }
            }

            if(primaries > primarySafeEnd) { /* We have stepped over the primary buffer */
                if(allocateSKBuffer == FALSE) { /* need to save our butts if we cannot reallocate */
                    IInit_collIterate(coll, (UChar *)source, len, &s, status);
                    if(U_FAILURE(*status)) {
                        sortKeySize = DEFAULT_ERROR_SIZE_FOR_CALCSORTKEY;
                        finished = TRUE;
                        break;
                    }
                    s.flags &= ~UCOL_ITER_NORM;
                    sortKeySize = ucol_getSortKeySize(coll, &s, sortKeySize, strength, len);
                    *status = U_BUFFER_OVERFLOW_ERROR;
                    finished = TRUE;
                    break;
                } else { /* It's much nicer if we can actually reallocate */
                    int32_t sks = sortKeySize+(int32_t)((primaries - primStart)+(secondaries - secStart)+(tertiaries - terStart)+(cases-caseStart)+(quads-quadStart));
                    primStart = reallocateBuffer(&primaries, *result, prim, &resultLength, 2*sks, status);
                    if(U_SUCCESS(*status)) {
                        *result = primStart;
                        primarySafeEnd = primStart + resultLength - 1;
                        if(strength > UCOL_PRIMARY) {
                            primarySafeEnd--;
                        }
                    } else {
                        /* We ran out of memory!? We can't recover. */
                        sortKeySize = DEFAULT_ERROR_SIZE_FOR_CALCSORTKEY;
                        finished = TRUE;
                        break;
                    }
                }
            }
        }
        if(finished) {
            break;
        } else {
            prevBuffSize = minBufferSize;

            uint32_t frenchStartOffset = 0, frenchEndOffset = 0;
            if (frenchStartPtr != NULL) {
                frenchStartOffset = (uint32_t)(frenchStartPtr - secStart);
                frenchEndOffset = (uint32_t)(frenchEndPtr - secStart);
            }
            secStart = reallocateBuffer(&secondaries, secStart, second, &secSize, 2*secSize, status);
            terStart = reallocateBuffer(&tertiaries, terStart, tert, &terSize, 2*terSize, status);
            caseStart = reallocateBuffer(&cases, caseStart, caseB, &caseSize, 2*caseSize, status);
            quadStart = reallocateBuffer(&quads, quadStart, quad, &quadSize, 2*quadSize, status);
            if(U_FAILURE(*status)) {
                /* We ran out of memory!? We can't recover. */
                sortKeySize = DEFAULT_ERROR_SIZE_FOR_CALCSORTKEY;
                break;
            }
            if (frenchStartPtr != NULL) {
                frenchStartPtr = secStart + frenchStartOffset;
                frenchEndPtr = secStart + frenchEndOffset;
            }
            minBufferSize *= 2;
        }
    }

    /* Here, we are generally done with processing */
    /* bailing out would not be too productive */

    if(U_SUCCESS(*status)) {
        sortKeySize += (uint32_t)(primaries - primStart);
        /* we have done all the CE's, now let's put them together to form a key */
        if(compareSec == 0) {
            if (count2 > 0) {
                while (count2 > UCOL_BOT_COUNT2) {
                    *secondaries++ = (uint8_t)(UCOL_COMMON_BOT2 + UCOL_BOT_COUNT2);
                    count2 -= (uint32_t)UCOL_BOT_COUNT2;
                }
                *secondaries++ = (uint8_t)(UCOL_COMMON_BOT2 + (count2-1));
            }
            uint32_t secsize = (uint32_t)(secondaries-secStart);
            if(!isFrenchSec) { // Regular situation, we know the length of secondaries
                sortKeySize += secsize;
                if(sortKeySize <= resultLength) {
                    *(primaries++) = UCOL_LEVELTERMINATOR;
                    uprv_memcpy(primaries, secStart, secsize);
                    primaries += secsize;
                } else {
                    if(allocateSKBuffer == TRUE) { /* need to save our butts if we cannot reallocate */
                        primStart = reallocateBuffer(&primaries, *result, prim, &resultLength, 2*sortKeySize, status);
                        if(U_SUCCESS(*status)) {
                            *result = primStart;
                            *(primaries++) = UCOL_LEVELTERMINATOR;
                            uprv_memcpy(primaries, secStart, secsize);
                            primaries += secsize;
                        }
                        else {
                            /* We ran out of memory!? We can't recover. */
                            sortKeySize = DEFAULT_ERROR_SIZE_FOR_CALCSORTKEY;
                            goto cleanup;
                        }
                    } else {
                        *status = U_BUFFER_OVERFLOW_ERROR;
                    }
                }
            } else { // French secondary is on. We will need to pack French. packFrench will add the level terminator
                uint8_t *newPrim = packFrench(primaries, primStart+resultLength, secondaries, &secsize, frenchStartPtr, frenchEndPtr);
                sortKeySize += secsize;
                if(sortKeySize <= resultLength) { // if we managed to pack fine
                    primaries = newPrim; // update the primary pointer
                } else { // overflow, need to reallocate and redo
                    if(allocateSKBuffer == TRUE) { /* need to save our butts if we cannot reallocate */
                        primStart = reallocateBuffer(&primaries, *result, prim, &resultLength, 2*sortKeySize, status);
                        if(U_SUCCESS(*status)) {
                            primaries = packFrench(primaries, primStart+resultLength, secondaries, &secsize, frenchStartPtr, frenchEndPtr);
                        }
                        else {
                            /* We ran out of memory!? We can't recover. */
                            sortKeySize = DEFAULT_ERROR_SIZE_FOR_CALCSORTKEY;
                            goto cleanup;
                        }
                    } else {
                        *status = U_BUFFER_OVERFLOW_ERROR;
                    }
                }
            }
        }

        if(doCase) {
            uint32_t casesize = (uint32_t)(cases - caseStart);
            sortKeySize += casesize;
            if(sortKeySize <= resultLength) {
                *(primaries++) = UCOL_LEVELTERMINATOR;
                uprv_memcpy(primaries, caseStart, casesize);
                primaries += casesize;
            } else {
                if(allocateSKBuffer == TRUE) {
                    primStart = reallocateBuffer(&primaries, *result, prim, &resultLength, 2*sortKeySize, status);
                    if(U_SUCCESS(*status)) {
                        *result = primStart;
                        *(primaries++) = UCOL_LEVELTERMINATOR;
                        uprv_memcpy(primaries, caseStart, casesize);
                    }
                    else {
                        /* We ran out of memory!? We can't recover. */
                        sortKeySize = DEFAULT_ERROR_SIZE_FOR_CALCSORTKEY;
                        goto cleanup;
                    }
                } else {
                    *status = U_BUFFER_OVERFLOW_ERROR;
                }
            }
        }

        if(compareTer == 0) {
            if (count3 > 0) {
                if (coll->tertiaryCommon != UCOL_COMMON_BOT3) {
                    while (count3 >= coll->tertiaryTopCount) {
                        *tertiaries++ = (uint8_t)(tertiaryTop - coll->tertiaryTopCount);
                        count3 -= (uint32_t)coll->tertiaryTopCount;
                    }
                    *tertiaries++ = (uint8_t)(tertiaryTop - count3);
                } else {
                    while (count3 > coll->tertiaryBottomCount) {
                        *tertiaries++ = (uint8_t)(tertiaryBottom + coll->tertiaryBottomCount);
                        count3 -= (uint32_t)coll->tertiaryBottomCount;
                    }
                    *tertiaries++ = (uint8_t)(tertiaryBottom + (count3-1));
                }
            }
            uint32_t tersize = (uint32_t)(tertiaries - terStart);
            sortKeySize += tersize;
            if(sortKeySize <= resultLength) {
                *(primaries++) = UCOL_LEVELTERMINATOR;
                uprv_memcpy(primaries, terStart, tersize);
                primaries += tersize;
            } else {
                if(allocateSKBuffer == TRUE) {
                    primStart = reallocateBuffer(&primaries, *result, prim, &resultLength, 2*sortKeySize, status);
                    if(U_SUCCESS(*status)) {
                        *result = primStart;
                        *(primaries++) = UCOL_LEVELTERMINATOR;
                        uprv_memcpy(primaries, terStart, tersize);
                    }
                    else {
                        /* We ran out of memory!? We can't recover. */
                        sortKeySize = DEFAULT_ERROR_SIZE_FOR_CALCSORTKEY;
                        goto cleanup;
                    }
                } else {
                    *status = U_BUFFER_OVERFLOW_ERROR;
                }
            }

            if(compareQuad == 0/*qShifted == TRUE*/) {
                if(count4 > 0) {
                    while (count4 > UCOL_BOT_COUNT4) {
                        *quads++ = (uint8_t)(UCOL_COMMON_BOT4 + UCOL_BOT_COUNT4);
                        count4 -= UCOL_BOT_COUNT4;
                    }
                    *quads++ = (uint8_t)(UCOL_COMMON_BOT4 + (count4-1));
                }
                uint32_t quadsize = (uint32_t)(quads - quadStart);
                sortKeySize += quadsize;
                if(sortKeySize <= resultLength) {
                    *(primaries++) = UCOL_LEVELTERMINATOR;
                    uprv_memcpy(primaries, quadStart, quadsize);
                    primaries += quadsize;
                } else {
                    if(allocateSKBuffer == TRUE) {
                        primStart = reallocateBuffer(&primaries, *result, prim, &resultLength, 2*sortKeySize, status);
                        if(U_SUCCESS(*status)) {
                            *result = primStart;
                            *(primaries++) = UCOL_LEVELTERMINATOR;
                            uprv_memcpy(primaries, quadStart, quadsize);
                        }
                        else {
                            /* We ran out of memory!? We can't recover. */
                            sortKeySize = DEFAULT_ERROR_SIZE_FOR_CALCSORTKEY;
                            goto cleanup;
                        }
                    } else {
                        *status = U_BUFFER_OVERFLOW_ERROR;
                    }
                }
            }

            if(compareIdent) {
                sortKeySize += u_lengthOfIdenticalLevelRun(s.string, len);
                if(sortKeySize <= resultLength) {
                    *(primaries++) = UCOL_LEVELTERMINATOR;
                    primaries += u_writeIdenticalLevelRun(s.string, len, primaries);
                } else {
                    if(allocateSKBuffer == TRUE) {
                        primStart = reallocateBuffer(&primaries, *result, prim, &resultLength, sortKeySize, status);
                        if(U_SUCCESS(*status)) {
                            *result = primStart;
                            *(primaries++) = UCOL_LEVELTERMINATOR;
                            u_writeIdenticalLevelRun(s.string, len, primaries);
                        }
                        else {
                            /* We ran out of memory!? We can't recover. */
                            sortKeySize = DEFAULT_ERROR_SIZE_FOR_CALCSORTKEY;
                            goto cleanup;
                        }
                    } else {
                        *status = U_BUFFER_OVERFLOW_ERROR;
                    }
                }
            }
        }
        *(primaries++) = '\0';
    }

    if(allocateSKBuffer == TRUE) {
        *result = (uint8_t*)uprv_malloc(sortKeySize);
        /* test for NULL */
        if (*result == NULL) {
            *status = U_MEMORY_ALLOCATION_ERROR;
            goto cleanup;
        }
        uprv_memcpy(*result, primStart, sortKeySize);
        if(primStart != prim) {
            uprv_free(primStart);
        }
    }

cleanup:
    if (allocateSKBuffer == FALSE && resultLength > 0 && U_FAILURE(*status) && *status != U_BUFFER_OVERFLOW_ERROR) {
        /* NULL terminate for safety */
        **result = 0;
    }
    if(terStart != tert) {
        uprv_free(terStart);
        uprv_free(secStart);
        uprv_free(caseStart);
        uprv_free(quadStart);
    }
    
    /* To avoid memory leak, free the offset buffer if necessary. */
    ucol_freeOffsetBuffer(&s);

    return sortKeySize;
}


U_CFUNC int32_t U_CALLCONV
ucol_calcSortKeySimpleTertiary(const    UCollator    *coll,
        const    UChar        *source,
        int32_t        sourceLength,
        uint8_t        **result,
        uint32_t        resultLength,
        UBool allocateSKBuffer,
        UErrorCode *status)
{
    U_ALIGN_CODE(16);

    //const UCAConstants *UCAconsts = (UCAConstants *)((uint8_t *)coll->UCA->image + coll->image->UCAConsts);
    uint32_t i = 0; /* general purpose counter */

    /* Stack allocated buffers for buffers we use */
    uint8_t prim[UCOL_PRIMARY_MAX_BUFFER], second[UCOL_SECONDARY_MAX_BUFFER], tert[UCOL_TERTIARY_MAX_BUFFER];

    uint8_t *primaries = *result, *secondaries = second, *tertiaries = tert;

    if(U_FAILURE(*status)) {
        return 0;
    }

    if(primaries == NULL && allocateSKBuffer == TRUE) {
        primaries = *result = prim;
        resultLength = UCOL_PRIMARY_MAX_BUFFER;
    }

    uint32_t secSize = UCOL_SECONDARY_MAX_BUFFER, terSize = UCOL_TERTIARY_MAX_BUFFER;

    uint32_t sortKeySize = 3; /* it is always \0 terminated plus separators for secondary and tertiary */

    UnicodeString normSource;

    int32_t len =  sourceLength;

    /* If we need to normalize, we'll do it all at once at the beginning! */
    if(coll->normalizationMode != UCOL_OFF) {
        normSource.setTo(len < 0, source, len);
        const Normalizer2 *norm2 = Normalizer2Factory::getFCDInstance(*status);
        int32_t qcYesLength = norm2->spanQuickCheckYes(normSource, *status);
        if(qcYesLength != normSource.length()) {
            UnicodeString unnormalized = normSource.tempSubString(qcYesLength);
            normSource.truncate(qcYesLength);
            norm2->normalizeSecondAndAppend(normSource, unnormalized, *status);
            source = normSource.getBuffer();
            len = normSource.length();
        }
    }
    collIterate s;
    IInit_collIterate(coll, (UChar *)source, len, &s, status);
    if(U_FAILURE(*status)) {
        return 0;
    }
    s.flags &= ~UCOL_ITER_NORM;  // source passed the FCD test or else was normalized.

    if(resultLength == 0 || primaries == NULL) {
        return ucol_getSortKeySize(coll, &s, sortKeySize, coll->strength, len);
    }

    uint8_t *primarySafeEnd = primaries + resultLength - 2;

    uint32_t minBufferSize = UCOL_MAX_BUFFER;

    uint8_t *primStart = primaries;
    uint8_t *secStart = secondaries;
    uint8_t *terStart = tertiaries;

    uint32_t order = 0;

    uint8_t primary1 = 0;
    uint8_t primary2 = 0;
    uint8_t secondary = 0;
    uint8_t tertiary = 0;
    uint8_t caseSwitch = coll->caseSwitch;
    uint8_t tertiaryMask = coll->tertiaryMask;
    int8_t tertiaryAddition = coll->tertiaryAddition;
    uint8_t tertiaryTop = coll->tertiaryTop;
    uint8_t tertiaryBottom = coll->tertiaryBottom;
    uint8_t tertiaryCommon = coll->tertiaryCommon;

    uint32_t prevBuffSize = 0;

    UBool finished = FALSE;
    UBool notIsContinuation = FALSE;

    uint32_t count2 = 0, count3 = 0;
    uint8_t leadPrimary = 0;

    for(;;) {
        for(i=prevBuffSize; i<minBufferSize; ++i) {

            order = ucol_IGetNextCE(coll, &s, status);

            if(order == 0) {
                continue;
            }

            if(order == UCOL_NO_MORE_CES) {
                finished = TRUE;
                break;
            }

            notIsContinuation = !isContinuation(order);

            if(notIsContinuation) {
                tertiary = (uint8_t)((order & tertiaryMask));
            } else {
                tertiary = (uint8_t)((order & UCOL_REMOVE_CONTINUATION));
            }

            secondary = (uint8_t)((order >>= 8) & UCOL_BYTE_SIZE_MASK);
            primary2 = (uint8_t)((order >>= 8) & UCOL_BYTE_SIZE_MASK);
            primary1 = (uint8_t)(order >> 8);

            uint8_t originalPrimary1 = primary1;
            if (coll->leadBytePermutationTable != NULL && notIsContinuation) {
                primary1 = coll->leadBytePermutationTable[primary1];
            }

            /* Note: This code assumes that the table is well built i.e. not having 0 bytes where they are not supposed to be. */
            /* Usually, we'll have non-zero primary1 & primary2, except in cases of a-z and friends, when primary2 will   */
            /* be zero with non zero primary1. primary3 is different than 0 only for long primaries - see above.               */
            /* regular and simple sortkey calc */
            if(primary1 != UCOL_IGNORABLE) {
                if(notIsContinuation) {
                    if(leadPrimary == primary1) {
                        *primaries++ = primary2;
                    } else {
                        if(leadPrimary != 0) {
                            *primaries++ = (uint8_t)((primary1 > leadPrimary) ? UCOL_BYTE_UNSHIFTED_MAX : UCOL_BYTE_UNSHIFTED_MIN);
                        }
                        if(primary2 == UCOL_IGNORABLE) {
                            /* one byter, not compressed */
                            *primaries++ = primary1;
                            leadPrimary = 0;
                        } else if(isCompressible(coll, originalPrimary1)) {
                            /* compress */
                            *primaries++ = leadPrimary = primary1;
                            *primaries++ = primary2;
                        } else {
                            leadPrimary = 0;
                            *primaries++ = primary1;
                            *primaries++ = primary2;
                        }
                    }
                } else { /* we are in continuation, so we're gonna add primary to the key don't care about compression */
                    *primaries++ = primary1;
                    if(primary2 != UCOL_IGNORABLE) {
                        *primaries++ = primary2; /* second part */
                    }
                }
            }

            if(secondary > 0) { /* I think that != 0 test should be != IGNORABLE */
                /* This is compression code. */
                if (secondary == UCOL_COMMON2 && notIsContinuation) {
                    ++count2;
                } else {
                    if (count2 > 0) {
                        if (secondary > UCOL_COMMON2) { // not necessary for 4th level.
                            while (count2 > UCOL_TOP_COUNT2) {
                                *secondaries++ = (uint8_t)(UCOL_COMMON_TOP2 - UCOL_TOP_COUNT2);
                                count2 -= (uint32_t)UCOL_TOP_COUNT2;
                            }
                            *secondaries++ = (uint8_t)(UCOL_COMMON_TOP2 - (count2-1));
                        } else {
                            while (count2 > UCOL_BOT_COUNT2) {
                                *secondaries++ = (uint8_t)(UCOL_COMMON_BOT2 + UCOL_BOT_COUNT2);
                                count2 -= (uint32_t)UCOL_BOT_COUNT2;
                            }
                            *secondaries++ = (uint8_t)(UCOL_COMMON_BOT2 + (count2-1));
                        }
                        count2 = 0;
                    }
                    *secondaries++ = secondary;
                }
            }

            if(notIsContinuation) {
                tertiary ^= caseSwitch;
            }

            if(tertiary > 0) {
                /* This is compression code. */
                /* sequence size check is included in the if clause */
                if (tertiary == tertiaryCommon && notIsContinuation) {
                    ++count3;
                } else {
                    if(tertiary > tertiaryCommon && tertiaryCommon == UCOL_COMMON3_NORMAL) {
                        tertiary += tertiaryAddition;
                    } else if (tertiary <= tertiaryCommon && tertiaryCommon == UCOL_COMMON3_UPPERFIRST) {
                        tertiary -= tertiaryAddition;
                    }
                    if (count3 > 0) {
                        if ((tertiary > tertiaryCommon)) {
                            while (count3 > coll->tertiaryTopCount) {
                                *tertiaries++ = (uint8_t)(tertiaryTop - coll->tertiaryTopCount);
                                count3 -= (uint32_t)coll->tertiaryTopCount;
                            }
                            *tertiaries++ = (uint8_t)(tertiaryTop - (count3-1));
                        } else {
                            while (count3 > coll->tertiaryBottomCount) {
                                *tertiaries++ = (uint8_t)(tertiaryBottom + coll->tertiaryBottomCount);
                                count3 -= (uint32_t)coll->tertiaryBottomCount;
                            }
                            *tertiaries++ = (uint8_t)(tertiaryBottom + (count3-1));
                        }
                        count3 = 0;
                    }
                    *tertiaries++ = tertiary;
                }
            }

            if(primaries > primarySafeEnd) { /* We have stepped over the primary buffer */
                if(allocateSKBuffer == FALSE) { /* need to save our butts if we cannot reallocate */
                    IInit_collIterate(coll, (UChar *)source, len, &s, status);
                    if(U_FAILURE(*status)) {
                        sortKeySize = DEFAULT_ERROR_SIZE_FOR_CALCSORTKEY;
                        finished = TRUE;
                        break;
                    }
                    s.flags &= ~UCOL_ITER_NORM;
                    sortKeySize = ucol_getSortKeySize(coll, &s, sortKeySize, coll->strength, len);
                    *status = U_BUFFER_OVERFLOW_ERROR;
                    finished = TRUE;
                    break;
                } else { /* It's much nicer if we can actually reallocate */
                    int32_t sks = sortKeySize+(int32_t)((primaries - primStart)+(secondaries - secStart)+(tertiaries - terStart));
                    primStart = reallocateBuffer(&primaries, *result, prim, &resultLength, 2*sks, status);
                    if(U_SUCCESS(*status)) {
                        *result = primStart;
                        primarySafeEnd = primStart + resultLength - 2;
                    } else {
                        /* We ran out of memory!? We can't recover. */
                        sortKeySize = DEFAULT_ERROR_SIZE_FOR_CALCSORTKEY;
                        finished = TRUE;
                        break;
                    }
                }
            }
        }
        if(finished) {
            break;
        } else {
            prevBuffSize = minBufferSize;
            secStart = reallocateBuffer(&secondaries, secStart, second, &secSize, 2*secSize, status);
            terStart = reallocateBuffer(&tertiaries, terStart, tert, &terSize, 2*terSize, status);
            minBufferSize *= 2;
            if(U_FAILURE(*status)) { // if we cannot reallocate buffers, we can at least give the sortkey size
                /* We ran out of memory!? We can't recover. */
                sortKeySize = DEFAULT_ERROR_SIZE_FOR_CALCSORTKEY;
                break;
            }
        }
    }

    if(U_SUCCESS(*status)) {
        sortKeySize += (uint32_t)(primaries - primStart);
        /* we have done all the CE's, now let's put them together to form a key */
        if (count2 > 0) {
            while (count2 > UCOL_BOT_COUNT2) {
                *secondaries++ = (uint8_t)(UCOL_COMMON_BOT2 + UCOL_BOT_COUNT2);
                count2 -= (uint32_t)UCOL_BOT_COUNT2;
            }
            *secondaries++ = (uint8_t)(UCOL_COMMON_BOT2 + (count2-1));
        }
        uint32_t secsize = (uint32_t)(secondaries-secStart);
        sortKeySize += secsize;
        if(sortKeySize <= resultLength) {
            *(primaries++) = UCOL_LEVELTERMINATOR;
            uprv_memcpy(primaries, secStart, secsize);
            primaries += secsize;
        } else {
            if(allocateSKBuffer == TRUE) {
                primStart = reallocateBuffer(&primaries, *result, prim, &resultLength, 2*sortKeySize, status);
                if(U_SUCCESS(*status)) {
                    *(primaries++) = UCOL_LEVELTERMINATOR;
                    *result = primStart;
                    uprv_memcpy(primaries, secStart, secsize);
                }
                else {
                    /* We ran out of memory!? We can't recover. */
                    sortKeySize = DEFAULT_ERROR_SIZE_FOR_CALCSORTKEY;
                    goto cleanup;
                }
            } else {
                *status = U_BUFFER_OVERFLOW_ERROR;
            }
        }

        if (count3 > 0) {
            if (coll->tertiaryCommon != UCOL_COMMON3_NORMAL) {
                while (count3 >= coll->tertiaryTopCount) {
                    *tertiaries++ = (uint8_t)(tertiaryTop - coll->tertiaryTopCount);
                    count3 -= (uint32_t)coll->tertiaryTopCount;
                }
                *tertiaries++ = (uint8_t)(tertiaryTop - count3);
            } else {
                while (count3 > coll->tertiaryBottomCount) {
                    *tertiaries++ = (uint8_t)(tertiaryBottom + coll->tertiaryBottomCount);
                    count3 -= (uint32_t)coll->tertiaryBottomCount;
                }
                *tertiaries++ = (uint8_t)(tertiaryBottom + (count3-1));
            }
        }
        uint32_t tersize = (uint32_t)(tertiaries - terStart);
        sortKeySize += tersize;
        if(sortKeySize <= resultLength) {
            *(primaries++) = UCOL_LEVELTERMINATOR;
            uprv_memcpy(primaries, terStart, tersize);
            primaries += tersize;
        } else {
            if(allocateSKBuffer == TRUE) {
                primStart = reallocateBuffer(&primaries, *result, prim, &resultLength, 2*sortKeySize, status);
                if(U_SUCCESS(*status)) {
                    *result = primStart;
                    *(primaries++) = UCOL_LEVELTERMINATOR;
                    uprv_memcpy(primaries, terStart, tersize);
                }
                else {
                    /* We ran out of memory!? We can't recover. */
                    sortKeySize = DEFAULT_ERROR_SIZE_FOR_CALCSORTKEY;
                    goto cleanup;
                }
            } else {
                *status = U_BUFFER_OVERFLOW_ERROR;
            }
        }

        *(primaries++) = '\0';
    }

    if(allocateSKBuffer == TRUE) {
        *result = (uint8_t*)uprv_malloc(sortKeySize);
        /* test for NULL */
        if (*result == NULL) {
            *status = U_MEMORY_ALLOCATION_ERROR;
            goto cleanup;
        }
        uprv_memcpy(*result, primStart, sortKeySize);
        if(primStart != prim) {
            uprv_free(primStart);
        }
    }

cleanup:
    if (allocateSKBuffer == FALSE && resultLength > 0 && U_FAILURE(*status) && *status != U_BUFFER_OVERFLOW_ERROR) {
        /* NULL terminate for safety */
        **result = 0;
    }
    if(terStart != tert) {
        uprv_free(terStart);
        uprv_free(secStart);
    }
    
    /* To avoid memory leak, free the offset buffer if necessary. */
    ucol_freeOffsetBuffer(&s);
    
    return sortKeySize;
}

static inline
UBool isShiftedCE(uint32_t CE, uint32_t LVT, UBool *wasShifted) {
    UBool notIsContinuation = !isContinuation(CE);
    uint8_t primary1 = (uint8_t)((CE >> 24) & 0xFF);
    if((LVT && ((notIsContinuation && (CE & 0xFFFF0000)<= LVT && primary1 > 0)
               || (!notIsContinuation && *wasShifted)))
        || (*wasShifted && primary1 == 0)) /* amendment to the UCA says that primary ignorables */
    {
        // The stuff below should probably be in the sortkey code... maybe not...
        if(primary1 != 0) { /* if we were shifted and we got an ignorable code point */
            /* we should just completely ignore it */
            *wasShifted = TRUE;
            //continue;
        }
        //*wasShifted = TRUE;
        return TRUE;
    } else {
        *wasShifted = FALSE;
        return FALSE;
    }
}
static inline
void terminatePSKLevel(int32_t level, int32_t maxLevel, int32_t &i, uint8_t *dest) {
    if(level < maxLevel) {
        dest[i++] = UCOL_LEVELTERMINATOR;
    } else {
        dest[i++] = 0;
    }
}

/** enumeration of level identifiers for partial sort key generation */
enum {
  UCOL_PSK_PRIMARY = 0,
    UCOL_PSK_SECONDARY = 1,
    UCOL_PSK_CASE = 2,
    UCOL_PSK_TERTIARY = 3,
    UCOL_PSK_QUATERNARY = 4,
    UCOL_PSK_QUIN = 5,      /** This is an extra level, not used - but we have three bits to blow */
    UCOL_PSK_IDENTICAL = 6,
    UCOL_PSK_NULL = 7,      /** level for the end of sort key. Will just produce zeros */
    UCOL_PSK_LIMIT
};

/** collation state enum. *_SHIFT value is how much to shift right
 *  to get the state piece to the right. *_MASK value should be
 *  ANDed with the shifted state. This data is stored in state[1]
 *  field.
 */
enum {
    UCOL_PSK_LEVEL_SHIFT = 0,      /** level identificator. stores an enum value from above */
    UCOL_PSK_LEVEL_MASK = 7,       /** three bits */
    UCOL_PSK_BYTE_COUNT_OR_FRENCH_DONE_SHIFT = 3, /** number of bytes of primary or quaternary already written */
    UCOL_PSK_BYTE_COUNT_OR_FRENCH_DONE_MASK = 1,
    /** can be only 0 or 1, since we get up to two bytes from primary or quaternary
     *  This field is also used to denote that the French secondary level is finished
     */
    UCOL_PSK_WAS_SHIFTED_SHIFT = 4,/** was the last value shifted */
    UCOL_PSK_WAS_SHIFTED_MASK = 1, /** can be 0 or 1 (Boolean) */
    UCOL_PSK_USED_FRENCH_SHIFT = 5,/** how many French bytes have we already written */
    UCOL_PSK_USED_FRENCH_MASK = 3, /** up to 4 bytes. See comment just below */
    /** When we do French we need to reverse secondary values. However, continuations
     *  need to stay the same. So if you had abc1c2c3de, you need to have edc1c2c3ba
     */
    UCOL_PSK_BOCSU_BYTES_SHIFT = 7,
    UCOL_PSK_BOCSU_BYTES_MASK = 3,
    UCOL_PSK_CONSUMED_CES_SHIFT = 9,
    UCOL_PSK_CONSUMED_CES_MASK = 0x7FFFF
};

// macro calculating the number of expansion CEs available
#define uprv_numAvailableExpCEs(s) (s).CEpos - (s).toReturn


/** main sortkey part procedure. On the first call,
 *  you should pass in a collator, an iterator, empty state
 *  state[0] == state[1] == 0, a buffer to hold results
 *  number of bytes you need and an error code pointer.
 *  Make sure your buffer is big enough to hold the wanted
 *  number of sortkey bytes. I don't check.
 *  The only meaningful status you can get back is
 *  U_BUFFER_OVERFLOW_ERROR, which basically means that you
 *  have been dealt a raw deal and that you probably won't
 *  be able to use partial sortkey generation for this
 *  particular combination of string and collator. This
 *  is highly unlikely, but you should still check the error code.
 *  Any other status means that you're not in a sane situation
 *  anymore. After the first call, preserve state values and
 *  use them on subsequent calls to obtain more bytes of a sortkey.
 *  Use until the number of bytes written is smaller than the requested
 *  number of bytes. Generated sortkey is not compatible with the
 *  one generated by ucol_getSortKey, as we don't do any compression.
 *  However, levels are still terminated by a 1 (one) and the sortkey
 *  is terminated by a 0 (zero). Identical level is the same as in the
 *  regular sortkey - internal bocu-1 implementation is used.
 *  For curious, although you cannot do much about this, here is
 *  the structure of state words.
 *  state[0] - iterator state. Depends on the iterator implementation,
 *             but allows the iterator to continue where it stopped in
 *             the last iteration.
 *  state[1] - collation processing state. Here is the distribution
 *             of the bits:
 *   0, 1, 2 - level of the sortkey - primary, secondary, case, tertiary
 *             quaternary, quin (we don't use this one), identical and
 *             null (producing only zeroes - first one to terminate the
 *             sortkey and subsequent to fill the buffer).
 *   3       - byte count. Number of bytes written on the primary level.
 *   4       - was shifted. Whether the previous iteration finished in the
 *             shifted state.
 *   5, 6    - French continuation bytes written. See the comment in the enum
 *   7,8     - Bocsu bytes used. Number of bytes from a bocu sequence on
 *             the identical level.
 *   9..31   - CEs consumed. Number of getCE or next32 operations performed
 *             since thes last successful update of the iterator state.
 */
U_CAPI int32_t U_EXPORT2
ucol_nextSortKeyPart(const UCollator *coll,
                     UCharIterator *iter,
                     uint32_t state[2],
                     uint8_t *dest, int32_t count,
                     UErrorCode *status)
{
    /* error checking */
    if(status==NULL || U_FAILURE(*status)) {
        return 0;
    }
    UTRACE_ENTRY(UTRACE_UCOL_NEXTSORTKEYPART);
    if( coll==NULL || iter==NULL ||
        state==NULL ||
        count<0 || (count>0 && dest==NULL)
    ) {
        *status=U_ILLEGAL_ARGUMENT_ERROR;
        UTRACE_EXIT_STATUS(status);
        return 0;
    }

    UTRACE_DATA6(UTRACE_VERBOSE, "coll=%p, iter=%p, state=%d %d, dest=%p, count=%d",
                  coll, iter, state[0], state[1], dest, count);

    if(count==0) {
        /* nothing to do */
        UTRACE_EXIT_VALUE(0);
        return 0;
    }
    /** Setting up situation according to the state we got from the previous iteration */
    // The state of the iterator from the previous invocation
    uint32_t iterState = state[0];
    // Has the last iteration ended in the shifted state
    UBool wasShifted = ((state[1] >> UCOL_PSK_WAS_SHIFTED_SHIFT) & UCOL_PSK_WAS_SHIFTED_MASK)?TRUE:FALSE;
    // What is the current level of the sortkey?
    int32_t level= (state[1] >> UCOL_PSK_LEVEL_SHIFT) & UCOL_PSK_LEVEL_MASK;
    // Have we written only one byte from a two byte primary in the previous iteration?
    // Also on secondary level - have we finished with the French secondary?
    int32_t byteCountOrFrenchDone = (state[1] >> UCOL_PSK_BYTE_COUNT_OR_FRENCH_DONE_SHIFT) & UCOL_PSK_BYTE_COUNT_OR_FRENCH_DONE_MASK;
    // number of bytes in the continuation buffer for French
    int32_t usedFrench = (state[1] >> UCOL_PSK_USED_FRENCH_SHIFT) & UCOL_PSK_USED_FRENCH_MASK;
    // Number of bytes already written from a bocsu sequence. Since
    // the longes bocsu sequence is 4 long, this can be up to 3.
    int32_t bocsuBytesUsed = (state[1] >> UCOL_PSK_BOCSU_BYTES_SHIFT) & UCOL_PSK_BOCSU_BYTES_MASK;
    // Number of elements that need to be consumed in this iteration because
    // the iterator returned UITER_NO_STATE at the end of the last iteration,
    // so we had to save the last valid state.
    int32_t cces = (state[1] >> UCOL_PSK_CONSUMED_CES_SHIFT) & UCOL_PSK_CONSUMED_CES_MASK;

    /** values that depend on the collator attributes */
    // strength of the collator.
    int32_t strength = ucol_getAttribute(coll, UCOL_STRENGTH, status);
    // maximal level of the partial sortkey. Need to take whether case level is done
    int32_t maxLevel = 0;
    if(strength < UCOL_TERTIARY) {
        if(ucol_getAttribute(coll, UCOL_CASE_LEVEL, status) == UCOL_ON) {
            maxLevel = UCOL_PSK_CASE;
        } else {
            maxLevel = strength;
        }
    } else {
        if(strength == UCOL_TERTIARY) {
            maxLevel = UCOL_PSK_TERTIARY;
        } else if(strength == UCOL_QUATERNARY) {
            maxLevel = UCOL_PSK_QUATERNARY;
        } else { // identical
            maxLevel = UCOL_IDENTICAL;
        }
    }
    // value for the quaternary level if Hiragana is encountered. Used for JIS X 4061 collation
    uint8_t UCOL_HIRAGANA_QUAD =
      (ucol_getAttribute(coll, UCOL_HIRAGANA_QUATERNARY_MODE, status) == UCOL_ON)?0xFE:0xFF;
    // Boundary value that decides whether a CE is shifted or not
    uint32_t LVT = (coll->alternateHandling == UCOL_SHIFTED)?(coll->variableTopValue<<16):0;
    // Are we doing French collation?
    UBool doingFrench = (ucol_getAttribute(coll, UCOL_FRENCH_COLLATION, status) == UCOL_ON);

    /** initializing the collation state */
    UBool notIsContinuation = FALSE;
    uint32_t CE = UCOL_NO_MORE_CES;

    collIterate s;
    IInit_collIterate(coll, NULL, -1, &s, status);
    if(U_FAILURE(*status)) {
        UTRACE_EXIT_STATUS(*status);
        return 0;
    }
    s.iterator = iter;
    s.flags |= UCOL_USE_ITERATOR;
    // This variable tells us whether we have produced some other levels in this iteration
    // before we moved to the identical level. In that case, we need to switch the
    // type of the iterator.
    UBool doingIdenticalFromStart = FALSE;
    // Normalizing iterator
    // The division for the array length may truncate the array size to
    // a little less than UNORM_ITER_SIZE, but that size is dimensioned too high
    // for all platforms anyway.
    UAlignedMemory stackNormIter[UNORM_ITER_SIZE/sizeof(UAlignedMemory)];
    UNormIterator *normIter = NULL;
    // If the normalization is turned on for the collator and we are below identical level
    // we will use a FCD normalizing iterator
    if(ucol_getAttribute(coll, UCOL_NORMALIZATION_MODE, status) == UCOL_ON && level < UCOL_PSK_IDENTICAL) {
        normIter = unorm_openIter(stackNormIter, sizeof(stackNormIter), status);
        s.iterator = unorm_setIter(normIter, iter, UNORM_FCD, status);
        s.flags &= ~UCOL_ITER_NORM;
        if(U_FAILURE(*status)) {
            UTRACE_EXIT_STATUS(*status);
            return 0;
        }
    } else if(level == UCOL_PSK_IDENTICAL) {
        // for identical level, we need a NFD iterator. We need to instantiate it here, since we
        // will be updating the state - and this cannot be done on an ordinary iterator.
        normIter = unorm_openIter(stackNormIter, sizeof(stackNormIter), status);
        s.iterator = unorm_setIter(normIter, iter, UNORM_NFD, status);
        s.flags &= ~UCOL_ITER_NORM;
        if(U_FAILURE(*status)) {
            UTRACE_EXIT_STATUS(*status);
            return 0;
        }
        doingIdenticalFromStart = TRUE;
    }

    // This is the tentative new state of the iterator. The problem
    // is that the iterator might return an undefined state, in
    // which case we should save the last valid state and increase
    // the iterator skip value.
    uint32_t newState = 0;

    // First, we set the iterator to the last valid position
    // from the last iteration. This was saved in state[0].
    if(iterState == 0) {
        /* initial state */
        if(level == UCOL_PSK_SECONDARY && doingFrench && !byteCountOrFrenchDone) {
            s.iterator->move(s.iterator, 0, UITER_LIMIT);
        } else {
            s.iterator->move(s.iterator, 0, UITER_START);
        }
    } else {
        /* reset to previous state */
        s.iterator->setState(s.iterator, iterState, status);
        if(U_FAILURE(*status)) {
            UTRACE_EXIT_STATUS(*status);
            return 0;
        }
    }



    // This variable tells us whether we can attempt to update the state
    // of iterator. Situations where we don't want to update iterator state
    // are the existence of expansion CEs that are not yet processed, and
    // finishing the case level without enough space in the buffer to insert
    // a level terminator.
    UBool canUpdateState = TRUE;

    // Consume all the CEs that were consumed at the end of the previous
    // iteration without updating the iterator state. On identical level,
    // consume the code points.
    int32_t counter = cces;
    if(level < UCOL_PSK_IDENTICAL) {
        while(counter-->0) {
            // If we're doing French and we are on the secondary level,
            // we go backwards.
            if(level == UCOL_PSK_SECONDARY && doingFrench) {
                CE = ucol_IGetPrevCE(coll, &s, status);
            } else {
                CE = ucol_IGetNextCE(coll, &s, status);
            }
            if(CE==UCOL_NO_MORE_CES) {
                /* should not happen */
                *status=U_INTERNAL_PROGRAM_ERROR;
                UTRACE_EXIT_STATUS(*status);
                return 0;
            }
            if(uprv_numAvailableExpCEs(s)) {
                canUpdateState = FALSE;
            }
        }
    } else {
        while(counter-->0) {
            uiter_next32(s.iterator);
        }
    }

    // French secondary needs to know whether the iterator state of zero came from previous level OR
    // from a new invocation...
    UBool wasDoingPrimary = FALSE;
    // destination buffer byte counter. When this guy
    // gets to count, we're done with the iteration
    int32_t i = 0;
    // used to count the zero bytes written after we
    // have finished with the sort key
    int32_t j = 0;


    // Hm.... I think we're ready to plunge in. Basic story is as following:
    // we have a fall through case based on level. This is used for initial
    // positioning on iteration start. Every level processor contains a
    // for(;;) which will be broken when we exhaust all the CEs. Other
    // way to exit is a goto saveState, which happens when we have filled
    // out our buffer.
    switch(level) {
    case UCOL_PSK_PRIMARY:
        wasDoingPrimary = TRUE;
        for(;;) {
            if(i==count) {
                goto saveState;
            }
            // We should save the state only if we
            // are sure that we are done with the
            // previous iterator state
            if(canUpdateState && byteCountOrFrenchDone == 0) {
                newState = s.iterator->getState(s.iterator);
                if(newState != UITER_NO_STATE) {
                    iterState = newState;
                    cces = 0;
                }
            }
            CE = ucol_IGetNextCE(coll, &s, status);
            cces++;
            if(CE==UCOL_NO_MORE_CES) {
                // Add the level separator
                terminatePSKLevel(level, maxLevel, i, dest);
                byteCountOrFrenchDone=0;
                // Restart the iteration an move to the
                // second level
                s.iterator->move(s.iterator, 0, UITER_START);
                cces = 0;
                level = UCOL_PSK_SECONDARY;
                break;
            }
            if(!isContinuation(CE)){
                if(coll->leadBytePermutationTable != NULL){
                    CE = (coll->leadBytePermutationTable[CE>>24] << 24) | (CE & 0x00FFFFFF);
                }
            }
            if(!isShiftedCE(CE, LVT, &wasShifted)) {
                CE >>= UCOL_PRIMARYORDERSHIFT; /* get primary */
                if(CE != 0) {
                    if(byteCountOrFrenchDone == 0) {
                        // get the second byte of primary
                        dest[i++]=(uint8_t)(CE >> 8);
                    } else {
                        byteCountOrFrenchDone = 0;
                    }
                    if((CE &=0xff)!=0) {
                        if(i==count) {
                            /* overflow */
                            byteCountOrFrenchDone = 1;
                            cces--;
                            goto saveState;
                        }
                        dest[i++]=(uint8_t)CE;
                    }
                }
            }
            if(uprv_numAvailableExpCEs(s)) {
                canUpdateState = FALSE;
            } else {
                canUpdateState = TRUE;
            }
        }
        /* fall through to next level */
    case UCOL_PSK_SECONDARY:
        if(strength >= UCOL_SECONDARY) {
            if(!doingFrench) {
                for(;;) {
                    if(i == count) {
                        goto saveState;
                    }
                    // We should save the state only if we
                    // are sure that we are done with the
                    // previous iterator state
                    if(canUpdateState) {
                        newState = s.iterator->getState(s.iterator);
                        if(newState != UITER_NO_STATE) {
                            iterState = newState;
                            cces = 0;
                        }
                    }
                    CE = ucol_IGetNextCE(coll, &s, status);
                    cces++;
                    if(CE==UCOL_NO_MORE_CES) {
                        // Add the level separator
                        terminatePSKLevel(level, maxLevel, i, dest);
                        byteCountOrFrenchDone = 0;
                        // Restart the iteration an move to the
                        // second level
                        s.iterator->move(s.iterator, 0, UITER_START);
                        cces = 0;
                        level = UCOL_PSK_CASE;
                        break;
                    }
                    if(!isShiftedCE(CE, LVT, &wasShifted)) {
                        CE >>= 8; /* get secondary */
                        if(CE != 0) {
                            dest[i++]=(uint8_t)CE;
                        }
                    }
                    if(uprv_numAvailableExpCEs(s)) {
                        canUpdateState = FALSE;
                    } else {
                        canUpdateState = TRUE;
                    }
                }
            } else { // French secondary processing
                uint8_t frenchBuff[UCOL_MAX_BUFFER];
                int32_t frenchIndex = 0;
                // Here we are going backwards.
                // If the iterator is at the beggining, it should be
                // moved to end.
                if(wasDoingPrimary) {
                    s.iterator->move(s.iterator, 0, UITER_LIMIT);
                    cces = 0;
                }
                for(;;) {
                    if(i == count) {
                        goto saveState;
                    }
                    if(canUpdateState) {
                        newState = s.iterator->getState(s.iterator);
                        if(newState != UITER_NO_STATE) {
                            iterState = newState;
                            cces = 0;
                        }
                    }
                    CE = ucol_IGetPrevCE(coll, &s, status);
                    cces++;
                    if(CE==UCOL_NO_MORE_CES) {
                        // Add the level separator
                        terminatePSKLevel(level, maxLevel, i, dest);
                        byteCountOrFrenchDone = 0;
                        // Restart the iteration an move to the next level
                        s.iterator->move(s.iterator, 0, UITER_START);
                        level = UCOL_PSK_CASE;
                        break;
                    }
                    if(isContinuation(CE)) { // if it's a continuation, we want to save it and
                        // reverse when we get a first non-continuation CE.
                        CE >>= 8;
                        frenchBuff[frenchIndex++] = (uint8_t)CE;
                    } else if(!isShiftedCE(CE, LVT, &wasShifted)) {
                        CE >>= 8; /* get secondary */
                        if(!frenchIndex) {
                            if(CE != 0) {
                                dest[i++]=(uint8_t)CE;
                            }
                        } else {
                            frenchBuff[frenchIndex++] = (uint8_t)CE;
                            frenchIndex -= usedFrench;
                            usedFrench = 0;
                            while(i < count && frenchIndex) {
                                dest[i++] = frenchBuff[--frenchIndex];
                                usedFrench++;
                            }
                        }
                    }
                    if(uprv_numAvailableExpCEs(s)) {
                        canUpdateState = FALSE;
                    } else {
                        canUpdateState = TRUE;
                    }
                }
            }
        } else {
            level = UCOL_PSK_CASE;
        }
        /* fall through to next level */
    case UCOL_PSK_CASE:
        if(ucol_getAttribute(coll, UCOL_CASE_LEVEL, status) == UCOL_ON) {
            uint32_t caseShift = UCOL_CASE_SHIFT_START;
            uint8_t caseByte = UCOL_CASE_BYTE_START;
            uint8_t caseBits = 0;

            for(;;) {
                U_ASSERT(caseShift <= UCOL_CASE_SHIFT_START);
                if(i == count) {
                    goto saveState;
                }
                // We should save the state only if we
                // are sure that we are done with the
                // previous iterator state
                if(canUpdateState) {
                    newState = s.iterator->getState(s.iterator);
                    if(newState != UITER_NO_STATE) {
                        iterState = newState;
                        cces = 0;
                    }
                }
                CE = ucol_IGetNextCE(coll, &s, status);
                cces++;
                if(CE==UCOL_NO_MORE_CES) {
                    // On the case level we might have an unfinished
                    // case byte. Add one if it's started.
                    if(caseShift != UCOL_CASE_SHIFT_START) {
                        dest[i++] = caseByte;
                    }
                    cces = 0;
                    // We have finished processing CEs on this level.
                    // However, we don't know if we have enough space
                    // to add a case level terminator.
                    if(i < count) {
                        // Add the level separator
                        terminatePSKLevel(level, maxLevel, i, dest);
                        // Restart the iteration and move to the
                        // next level
                        s.iterator->move(s.iterator, 0, UITER_START);
                        level = UCOL_PSK_TERTIARY;
                    } else {
                        canUpdateState = FALSE;
                    }
                    break;
                }

                if(!isShiftedCE(CE, LVT, &wasShifted)) {
                    if(!isContinuation(CE) && ((CE & UCOL_PRIMARYMASK) != 0 || strength > UCOL_PRIMARY)) {
                        // do the case level if we need to do it. We don't want to calculate
                        // case level for primary ignorables if we have only primary strength and case level
                        // otherwise we would break well formedness of CEs
                        CE = (uint8_t)(CE & UCOL_BYTE_SIZE_MASK);
                        caseBits = (uint8_t)(CE & 0xC0);
                        // this copies the case level logic from the
                        // sort key generation code
                        if(CE != 0) {
                            if (caseShift == 0) {
                                dest[i++] = caseByte;
                                caseShift = UCOL_CASE_SHIFT_START;
                                caseByte = UCOL_CASE_BYTE_START;
                            }
                            if(coll->caseFirst == UCOL_UPPER_FIRST) {
                                if((caseBits & 0xC0) == 0) {
                                    caseByte |= 1 << (--caseShift);
                                } else {
                                    caseByte |= 0 << (--caseShift);
                                    /* second bit */
                                    if(caseShift == 0) {
                                        dest[i++] = caseByte;
                                        caseShift = UCOL_CASE_SHIFT_START;
                                        caseByte = UCOL_CASE_BYTE_START;
                                    }
                                    caseByte |= ((caseBits>>6)&1) << (--caseShift);
                                }
                            } else {
                                if((caseBits & 0xC0) == 0) {
                                    caseByte |= 0 << (--caseShift);
                                } else {
                                    caseByte |= 1 << (--caseShift);
                                    /* second bit */
                                    if(caseShift == 0) {
                                        dest[i++] = caseByte;
                                        caseShift = UCOL_CASE_SHIFT_START;
                                        caseByte = UCOL_CASE_BYTE_START;
                                    }
                                    caseByte |= ((caseBits>>7)&1) << (--caseShift);
                                }
                            }
                        }

                    }
                }
                // Not sure this is correct for the case level - revisit
                if(uprv_numAvailableExpCEs(s)) {
                    canUpdateState = FALSE;
                } else {
                    canUpdateState = TRUE;
                }
            }
        } else {
            level = UCOL_PSK_TERTIARY;
        }
        /* fall through to next level */
    case UCOL_PSK_TERTIARY:
        if(strength >= UCOL_TERTIARY) {
            for(;;) {
                if(i == count) {
                    goto saveState;
                }
                // We should save the state only if we
                // are sure that we are done with the
                // previous iterator state
                if(canUpdateState) {
                    newState = s.iterator->getState(s.iterator);
                    if(newState != UITER_NO_STATE) {
                        iterState = newState;
                        cces = 0;
                    }
                }
                CE = ucol_IGetNextCE(coll, &s, status);
                cces++;
                if(CE==UCOL_NO_MORE_CES) {
                    // Add the level separator
                    terminatePSKLevel(level, maxLevel, i, dest);
                    byteCountOrFrenchDone = 0;
                    // Restart the iteration an move to the
                    // second level
                    s.iterator->move(s.iterator, 0, UITER_START);
                    cces = 0;
                    level = UCOL_PSK_QUATERNARY;
                    break;
                }
                if(!isShiftedCE(CE, LVT, &wasShifted)) {
                    notIsContinuation = !isContinuation(CE);

                    if(notIsContinuation) {
                        CE = (uint8_t)(CE & UCOL_BYTE_SIZE_MASK);
                        CE ^= coll->caseSwitch;
                        CE &= coll->tertiaryMask;
                    } else {
                        CE = (uint8_t)((CE & UCOL_REMOVE_CONTINUATION));
                    }

                    if(CE != 0) {
                        dest[i++]=(uint8_t)CE;
                    }
                }
                if(uprv_numAvailableExpCEs(s)) {
                    canUpdateState = FALSE;
                } else {
                    canUpdateState = TRUE;
                }
            }
        } else {
            // if we're not doing tertiary
            // skip to the end
            level = UCOL_PSK_NULL;
        }
        /* fall through to next level */
    case UCOL_PSK_QUATERNARY:
        if(strength >= UCOL_QUATERNARY) {
            for(;;) {
                if(i == count) {
                    goto saveState;
                }
                // We should save the state only if we
                // are sure that we are done with the
                // previous iterator state
                if(canUpdateState) {
                    newState = s.iterator->getState(s.iterator);
                    if(newState != UITER_NO_STATE) {
                        iterState = newState;
                        cces = 0;
                    }
                }
                CE = ucol_IGetNextCE(coll, &s, status);
                cces++;
                if(CE==UCOL_NO_MORE_CES) {
                    // Add the level separator
                    terminatePSKLevel(level, maxLevel, i, dest);
                    //dest[i++] = UCOL_LEVELTERMINATOR;
                    byteCountOrFrenchDone = 0;
                    // Restart the iteration an move to the
                    // second level
                    s.iterator->move(s.iterator, 0, UITER_START);
                    cces = 0;
                    level = UCOL_PSK_QUIN;
                    break;
                }
                if(CE==0)
                    continue;
                if(isShiftedCE(CE, LVT, &wasShifted)) {
                    CE >>= 16; /* get primary */
                    if(CE != 0) {
                        if(byteCountOrFrenchDone == 0) {
                            dest[i++]=(uint8_t)(CE >> 8);
                        } else {
                            byteCountOrFrenchDone = 0;
                        }
                        if((CE &=0xff)!=0) {
                            if(i==count) {
                                /* overflow */
                                byteCountOrFrenchDone = 1;
                                goto saveState;
                            }
                            dest[i++]=(uint8_t)CE;
                        }
                    }
                } else {
                    notIsContinuation = !isContinuation(CE);
                    if(notIsContinuation) {
                        if(s.flags & UCOL_WAS_HIRAGANA) { // This was Hiragana and we need to note it
                            dest[i++] = UCOL_HIRAGANA_QUAD;
                        } else {
                            dest[i++] = 0xFF;
                        }
                    }
                }
                if(uprv_numAvailableExpCEs(s)) {
                    canUpdateState = FALSE;
                } else {
                    canUpdateState = TRUE;
                }
            }
        } else {
            // if we're not doing quaternary
            // skip to the end
            level = UCOL_PSK_NULL;
        }
        /* fall through to next level */
    case UCOL_PSK_QUIN:
        level = UCOL_PSK_IDENTICAL;
        /* fall through to next level */
    case UCOL_PSK_IDENTICAL:
        if(strength >= UCOL_IDENTICAL) {
            UChar32 first, second;
            int32_t bocsuBytesWritten = 0;
            // We always need to do identical on
            // the NFD form of the string.
            if(normIter == NULL) {
                // we arrived from the level below and
                // normalization was not turned on.
                // therefore, we need to make a fresh NFD iterator
                normIter = unorm_openIter(stackNormIter, sizeof(stackNormIter), status);
                s.iterator = unorm_setIter(normIter, iter, UNORM_NFD, status);
            } else if(!doingIdenticalFromStart) {
                // there is an iterator, but we did some other levels.
                // therefore, we have a FCD iterator - need to make
                // a NFD one.
                // normIter being at the beginning does not guarantee
                // that the underlying iterator is at the beginning
                iter->move(iter, 0, UITER_START);
                s.iterator = unorm_setIter(normIter, iter, UNORM_NFD, status);
            }
            // At this point we have a NFD iterator that is positioned
            // in the right place
            if(U_FAILURE(*status)) {
                UTRACE_EXIT_STATUS(*status);
                return 0;
            }
            first = uiter_previous32(s.iterator);
            // maybe we're at the start of the string
            if(first == U_SENTINEL) {
                first = 0;
            } else {
                uiter_next32(s.iterator);
            }

            j = 0;
            for(;;) {
                if(i == count) {
                    if(j+1 < bocsuBytesWritten) {
                        bocsuBytesUsed = j+1;
                    }
                    goto saveState;
                }

                // On identical level, we will always save
                // the state if we reach this point, since
                // we don't depend on getNextCE for content
                // all the content is in our buffer and we
                // already either stored the full buffer OR
                // otherwise we won't arrive here.
                newState = s.iterator->getState(s.iterator);
                if(newState != UITER_NO_STATE) {
                    iterState = newState;
                    cces = 0;
                }

                uint8_t buff[4];
                second = uiter_next32(s.iterator);
                cces++;

                // end condition for identical level
                if(second == U_SENTINEL) {
                    terminatePSKLevel(level, maxLevel, i, dest);
                    level = UCOL_PSK_NULL;
                    break;
                }
                bocsuBytesWritten = u_writeIdenticalLevelRunTwoChars(first, second, buff);
                first = second;

                j = 0;
                if(bocsuBytesUsed != 0) {
                    while(bocsuBytesUsed-->0) {
                        j++;
                    }
                }

                while(i < count && j < bocsuBytesWritten) {
                    dest[i++] = buff[j++];
                }
            }

        } else {
            level = UCOL_PSK_NULL;
        }
        /* fall through to next level */
    case UCOL_PSK_NULL:
        j = i;
        while(j<count) {
            dest[j++]=0;
        }
        break;
    default:
        *status = U_INTERNAL_PROGRAM_ERROR;
        UTRACE_EXIT_STATUS(*status);
        return 0;
    }

saveState:
    // Now we need to return stuff. First we want to see whether we have
    // done everything for the current state of iterator.
    if(byteCountOrFrenchDone
        || canUpdateState == FALSE
        || (newState = s.iterator->getState(s.iterator)) == UITER_NO_STATE)
    {
        // Any of above mean that the previous transaction
        // wasn't finished and that we should store the
        // previous iterator state.
        state[0] = iterState;
    } else {
        // The transaction is complete. We will continue in the next iteration.
        state[0] = s.iterator->getState(s.iterator);
        cces = 0;
    }
    // Store the number of bocsu bytes written.
    if((bocsuBytesUsed & UCOL_PSK_BOCSU_BYTES_MASK) != bocsuBytesUsed) {
        *status = U_INDEX_OUTOFBOUNDS_ERROR;
    }
    state[1] = (bocsuBytesUsed & UCOL_PSK_BOCSU_BYTES_MASK) << UCOL_PSK_BOCSU_BYTES_SHIFT;

    // Next we put in the level of comparison
    state[1] |= ((level & UCOL_PSK_LEVEL_MASK) << UCOL_PSK_LEVEL_SHIFT);

    // If we are doing French, we need to store whether we have just finished the French level
    if(level == UCOL_PSK_SECONDARY && doingFrench) {
        state[1] |= (((state[0] == 0) & UCOL_PSK_BYTE_COUNT_OR_FRENCH_DONE_MASK) << UCOL_PSK_BYTE_COUNT_OR_FRENCH_DONE_SHIFT);
    } else {
        state[1] |= ((byteCountOrFrenchDone & UCOL_PSK_BYTE_COUNT_OR_FRENCH_DONE_MASK) << UCOL_PSK_BYTE_COUNT_OR_FRENCH_DONE_SHIFT);
    }

    // Was the latest CE shifted
    if(wasShifted) {
        state[1] |= 1 << UCOL_PSK_WAS_SHIFTED_SHIFT;
    }
    // Check for cces overflow
    if((cces & UCOL_PSK_CONSUMED_CES_MASK) != cces) {
        *status = U_INDEX_OUTOFBOUNDS_ERROR;
    }
    // Store cces
    state[1] |= ((cces & UCOL_PSK_CONSUMED_CES_MASK) << UCOL_PSK_CONSUMED_CES_SHIFT);

    // Check for French overflow
    if((usedFrench & UCOL_PSK_USED_FRENCH_MASK) != usedFrench) {
        *status = U_INDEX_OUTOFBOUNDS_ERROR;
    }
    // Store number of bytes written in the French secondary continuation sequence
    state[1] |= ((usedFrench & UCOL_PSK_USED_FRENCH_MASK) << UCOL_PSK_USED_FRENCH_SHIFT);


    // If we have used normalizing iterator, get rid of it
    if(normIter != NULL) {
        unorm_closeIter(normIter);
    }

    /* To avoid memory leak, free the offset buffer if necessary. */
    ucol_freeOffsetBuffer(&s);
    
    // Return number of meaningful sortkey bytes.
    UTRACE_DATA4(UTRACE_VERBOSE, "dest = %vb, state=%d %d",
                  dest,i, state[0], state[1]);
    UTRACE_EXIT_VALUE(i);
    return i;
}

/**
 * Produce a bound for a given sortkey and a number of levels.
 */
U_CAPI int32_t U_EXPORT2
ucol_getBound(const uint8_t       *source,
        int32_t             sourceLength,
        UColBoundMode       boundType,
        uint32_t            noOfLevels,
        uint8_t             *result,
        int32_t             resultLength,
        UErrorCode          *status)
{
    // consistency checks
    if(status == NULL || U_FAILURE(*status)) {
        return 0;
    }
    if(source == NULL) {
        *status = U_ILLEGAL_ARGUMENT_ERROR;
        return 0;
    }

    int32_t sourceIndex = 0;
    // Scan the string until we skip enough of the key OR reach the end of the key
    do {
        sourceIndex++;
        if(source[sourceIndex] == UCOL_LEVELTERMINATOR) {
            noOfLevels--;
        }
    } while (noOfLevels > 0
        && (source[sourceIndex] != 0 || sourceIndex < sourceLength));

    if((source[sourceIndex] == 0 || sourceIndex == sourceLength)
        && noOfLevels > 0) {
            *status = U_SORT_KEY_TOO_SHORT_WARNING;
    }


    // READ ME: this code assumes that the values for boundType
    // enum will not changes. They are set so that the enum value
    // corresponds to the number of extra bytes each bound type
    // needs.
    if(result != NULL && resultLength >= sourceIndex+boundType) {
        uprv_memcpy(result, source, sourceIndex);
        switch(boundType) {
            // Lower bound just gets terminated. No extra bytes
        case UCOL_BOUND_LOWER: // = 0
            break;
            // Upper bound needs one extra byte
        case UCOL_BOUND_UPPER: // = 1
            result[sourceIndex++] = 2;
            break;
            // Upper long bound needs two extra bytes
        case UCOL_BOUND_UPPER_LONG: // = 2
            result[sourceIndex++] = 0xFF;
            result[sourceIndex++] = 0xFF;
            break;
        default:
            *status = U_ILLEGAL_ARGUMENT_ERROR;
            return 0;
        }
        result[sourceIndex++] = 0;

        return sourceIndex;
    } else {
        return sourceIndex+boundType+1;
    }
}

/****************************************************************************/
/* Following are the functions that deal with the properties of a collator  */
/* there are new APIs and some compatibility APIs                           */
/****************************************************************************/

static inline void
ucol_addLatinOneEntry(UCollator *coll, UChar ch, uint32_t CE,
                    int32_t *primShift, int32_t *secShift, int32_t *terShift)
{
    uint8_t primary1 = 0, primary2 = 0, secondary = 0, tertiary = 0;
    UBool reverseSecondary = FALSE;
    UBool continuation = isContinuation(CE);
    if(!continuation) {
        tertiary = (uint8_t)((CE & coll->tertiaryMask));
        tertiary ^= coll->caseSwitch;
        reverseSecondary = TRUE;
    } else {
        tertiary = (uint8_t)((CE & UCOL_REMOVE_CONTINUATION));
        tertiary &= UCOL_REMOVE_CASE;
        reverseSecondary = FALSE;
    }

    secondary = (uint8_t)((CE >>= 8) & UCOL_BYTE_SIZE_MASK);
    primary2 = (uint8_t)((CE >>= 8) & UCOL_BYTE_SIZE_MASK);
    primary1 = (uint8_t)(CE >> 8);

    if(primary1 != 0) {
        if (coll->leadBytePermutationTable != NULL && !continuation) {
            primary1 = coll->leadBytePermutationTable[primary1];
        }

        coll->latinOneCEs[ch] |= (primary1 << *primShift);
        *primShift -= 8;
    }
    if(primary2 != 0) {
        if(*primShift < 0) {
            coll->latinOneCEs[ch] = UCOL_BAIL_OUT_CE;
            coll->latinOneCEs[coll->latinOneTableLen+ch] = UCOL_BAIL_OUT_CE;
            coll->latinOneCEs[2*coll->latinOneTableLen+ch] = UCOL_BAIL_OUT_CE;
            return;
        }
        coll->latinOneCEs[ch] |= (primary2 << *primShift);
        *primShift -= 8;
    }
    if(secondary != 0) {
        if(reverseSecondary && coll->frenchCollation == UCOL_ON) { // reverse secondary
            coll->latinOneCEs[coll->latinOneTableLen+ch] >>= 8; // make space for secondary
            coll->latinOneCEs[coll->latinOneTableLen+ch] |= (secondary << 24);
        } else { // normal case
            coll->latinOneCEs[coll->latinOneTableLen+ch] |= (secondary << *secShift);
        }
        *secShift -= 8;
    }
    if(tertiary != 0) {
        coll->latinOneCEs[2*coll->latinOneTableLen+ch] |= (tertiary << *terShift);
        *terShift -= 8;
    }
}

static inline UBool
ucol_resizeLatinOneTable(UCollator *coll, int32_t size, UErrorCode *status) {
    uint32_t *newTable = (uint32_t *)uprv_malloc(size*sizeof(uint32_t)*3);
    if(newTable == NULL) {
      *status = U_MEMORY_ALLOCATION_ERROR;
      coll->latinOneFailed = TRUE;
      return FALSE;
    }
    int32_t sizeToCopy = ((size<coll->latinOneTableLen)?size:coll->latinOneTableLen)*sizeof(uint32_t);
    uprv_memset(newTable, 0, size*sizeof(uint32_t)*3);
    uprv_memcpy(newTable, coll->latinOneCEs, sizeToCopy);
    uprv_memcpy(newTable+size, coll->latinOneCEs+coll->latinOneTableLen, sizeToCopy);
    uprv_memcpy(newTable+2*size, coll->latinOneCEs+2*coll->latinOneTableLen, sizeToCopy);
    coll->latinOneTableLen = size;
    uprv_free(coll->latinOneCEs);
    coll->latinOneCEs = newTable;
    return TRUE;
}

static UBool
ucol_setUpLatinOne(UCollator *coll, UErrorCode *status) {
    UBool result = TRUE;
    if(coll->latinOneCEs == NULL) {
        coll->latinOneCEs = (uint32_t *)uprv_malloc(sizeof(uint32_t)*UCOL_LATINONETABLELEN*3);
        if(coll->latinOneCEs == NULL) {
            *status = U_MEMORY_ALLOCATION_ERROR;
            return FALSE;
        }
        coll->latinOneTableLen = UCOL_LATINONETABLELEN;
    }
    UChar ch = 0;
    UCollationElements *it = ucol_openElements(coll, &ch, 1, status);
    // Check for null pointer 
    if (U_FAILURE(*status)) {
        return FALSE;
    }
    uprv_memset(coll->latinOneCEs, 0, sizeof(uint32_t)*coll->latinOneTableLen*3);

    int32_t primShift = 24, secShift = 24, terShift = 24;
    uint32_t CE = 0;
    int32_t contractionOffset = UCOL_ENDOFLATINONERANGE+1;

    // TODO: make safe if you get more than you wanted...
    for(ch = 0; ch <= UCOL_ENDOFLATINONERANGE; ch++) {
        primShift = 24; secShift = 24; terShift = 24;
        if(ch < 0x100) {
            CE = coll->latinOneMapping[ch];
        } else {
            CE = UTRIE_GET32_FROM_LEAD(&coll->mapping, ch);
            if(CE == UCOL_NOT_FOUND && coll->UCA) {
                CE = UTRIE_GET32_FROM_LEAD(&coll->UCA->mapping, ch);
            }
        }
        if(CE < UCOL_NOT_FOUND) {
            ucol_addLatinOneEntry(coll, ch, CE, &primShift, &secShift, &terShift);
        } else {
            switch (getCETag(CE)) {
            case EXPANSION_TAG:
            case DIGIT_TAG:
                ucol_setText(it, &ch, 1, status);
                while((int32_t)(CE = ucol_next(it, status)) != UCOL_NULLORDER) {
                    if(primShift < 0 || secShift < 0 || terShift < 0) {
                        coll->latinOneCEs[ch] = UCOL_BAIL_OUT_CE;
                        coll->latinOneCEs[coll->latinOneTableLen+ch] = UCOL_BAIL_OUT_CE;
                        coll->latinOneCEs[2*coll->latinOneTableLen+ch] = UCOL_BAIL_OUT_CE;
                        break;
                    }
                    ucol_addLatinOneEntry(coll, ch, CE, &primShift, &secShift, &terShift);
                }
                break;
            case CONTRACTION_TAG:
                // here is the trick
                // F2 is contraction. We do something very similar to contractions
                // but have two indices, one in the real contraction table and the
                // other to where we stuffed things. This hopes that we don't have
                // many contractions (this should work for latin-1 tables).
                {
                    if((CE & 0x00FFF000) != 0) {
                        *status = U_UNSUPPORTED_ERROR;
                        goto cleanup_after_failure;
                    }

                    const UChar *UCharOffset = (UChar *)coll->image+getContractOffset(CE);

                    CE |= (contractionOffset & 0xFFF) << 12; // insert the offset in latin-1 table

                    coll->latinOneCEs[ch] = CE;
                    coll->latinOneCEs[coll->latinOneTableLen+ch] = CE;
                    coll->latinOneCEs[2*coll->latinOneTableLen+ch] = CE;

                    // We're going to jump into contraction table, pick the elements
                    // and use them
                    do {
                        CE = *(coll->contractionCEs +
                            (UCharOffset - coll->contractionIndex));
                        if(CE > UCOL_NOT_FOUND && getCETag(CE) == EXPANSION_TAG) {
                            uint32_t size;
                            uint32_t i;    /* general counter */
                            uint32_t *CEOffset = (uint32_t *)coll->image+getExpansionOffset(CE); /* find the offset to expansion table */
                            size = getExpansionCount(CE);
                            //CE = *CEOffset++;
                            if(size != 0) { /* if there are less than 16 elements in expansion, we don't terminate */
                                for(i = 0; i<size; i++) {
                                    if(primShift < 0 || secShift < 0 || terShift < 0) {
                                        coll->latinOneCEs[(UChar)contractionOffset] = UCOL_BAIL_OUT_CE;
                                        coll->latinOneCEs[coll->latinOneTableLen+(UChar)contractionOffset] = UCOL_BAIL_OUT_CE;
                                        coll->latinOneCEs[2*coll->latinOneTableLen+(UChar)contractionOffset] = UCOL_BAIL_OUT_CE;
                                        break;
                                    }
                                    ucol_addLatinOneEntry(coll, (UChar)contractionOffset, *CEOffset++, &primShift, &secShift, &terShift);
                                }
                            } else { /* else, we do */
                                while(*CEOffset != 0) {
                                    if(primShift < 0 || secShift < 0 || terShift < 0) {
                                        coll->latinOneCEs[(UChar)contractionOffset] = UCOL_BAIL_OUT_CE;
                                        coll->latinOneCEs[coll->latinOneTableLen+(UChar)contractionOffset] = UCOL_BAIL_OUT_CE;
                                        coll->latinOneCEs[2*coll->latinOneTableLen+(UChar)contractionOffset] = UCOL_BAIL_OUT_CE;
                                        break;
                                    }
                                    ucol_addLatinOneEntry(coll, (UChar)contractionOffset, *CEOffset++, &primShift, &secShift, &terShift);
                                }
                            }
                            contractionOffset++;
                        } else if(CE < UCOL_NOT_FOUND) {
                            ucol_addLatinOneEntry(coll, (UChar)contractionOffset++, CE, &primShift, &secShift, &terShift);
                        } else {
                            coll->latinOneCEs[(UChar)contractionOffset] = UCOL_BAIL_OUT_CE;
                            coll->latinOneCEs[coll->latinOneTableLen+(UChar)contractionOffset] = UCOL_BAIL_OUT_CE;
                            coll->latinOneCEs[2*coll->latinOneTableLen+(UChar)contractionOffset] = UCOL_BAIL_OUT_CE;
                            contractionOffset++;
                        }
                        UCharOffset++;
                        primShift = 24; secShift = 24; terShift = 24;
                        if(contractionOffset == coll->latinOneTableLen) { // we need to reallocate
                            if(!ucol_resizeLatinOneTable(coll, 2*coll->latinOneTableLen, status)) {
                                goto cleanup_after_failure;
                            }
                        }
                    } while(*UCharOffset != 0xFFFF);
                }
                break;;
            case SPEC_PROC_TAG:
                {
                    // 0xB7 is a precontext character defined in UCA5.1, a special
                    // handle is implemeted in order to save LatinOne table for
                    // most locales.
                    if (ch==0xb7) {
                        ucol_addLatinOneEntry(coll, ch, CE, &primShift, &secShift, &terShift);
                    }
                    else {
                        goto cleanup_after_failure;
                    }
                }
                break;
            default:
                goto cleanup_after_failure;
            }
        }
    }
    // compact table
    if(contractionOffset < coll->latinOneTableLen) {
        if(!ucol_resizeLatinOneTable(coll, contractionOffset, status)) {
            goto cleanup_after_failure;
        }
    }
    ucol_closeElements(it);
    return result;

cleanup_after_failure:
    // status should already be set before arriving here.
    coll->latinOneFailed = TRUE;
    ucol_closeElements(it);
    return FALSE;
}

void ucol_updateInternalState(UCollator *coll, UErrorCode *status) {
    if(U_SUCCESS(*status)) {
        if(coll->caseFirst == UCOL_UPPER_FIRST) {
            coll->caseSwitch = UCOL_CASE_SWITCH;
        } else {
            coll->caseSwitch = UCOL_NO_CASE_SWITCH;
        }

        if(coll->caseLevel == UCOL_ON || coll->caseFirst == UCOL_OFF) {
            coll->tertiaryMask = UCOL_REMOVE_CASE;
            coll->tertiaryCommon = UCOL_COMMON3_NORMAL;
            coll->tertiaryAddition = (int8_t)UCOL_FLAG_BIT_MASK_CASE_SW_OFF; /* Should be 0x80 */
            coll->tertiaryTop = UCOL_COMMON_TOP3_CASE_SW_OFF;
            coll->tertiaryBottom = UCOL_COMMON_BOT3;
        } else {
            coll->tertiaryMask = UCOL_KEEP_CASE;
            coll->tertiaryAddition = UCOL_FLAG_BIT_MASK_CASE_SW_ON;
            if(coll->caseFirst == UCOL_UPPER_FIRST) {
                coll->tertiaryCommon = UCOL_COMMON3_UPPERFIRST;
                coll->tertiaryTop = UCOL_COMMON_TOP3_CASE_SW_UPPER;
                coll->tertiaryBottom = UCOL_COMMON_BOTTOM3_CASE_SW_UPPER;
            } else {
                coll->tertiaryCommon = UCOL_COMMON3_NORMAL;
                coll->tertiaryTop = UCOL_COMMON_TOP3_CASE_SW_LOWER;
                coll->tertiaryBottom = UCOL_COMMON_BOTTOM3_CASE_SW_LOWER;
            }
        }

        /* Set the compression values */
        uint8_t tertiaryTotal = (uint8_t)(coll->tertiaryTop - UCOL_COMMON_BOT3-1);
        coll->tertiaryTopCount = (uint8_t)(UCOL_PROPORTION3*tertiaryTotal); /* we multilply double with int, but need only int */
        coll->tertiaryBottomCount = (uint8_t)(tertiaryTotal - coll->tertiaryTopCount);

        if(coll->caseLevel == UCOL_OFF && coll->strength == UCOL_TERTIARY
            && coll->frenchCollation == UCOL_OFF && coll->alternateHandling == UCOL_NON_IGNORABLE)
        {
            coll->sortKeyGen = ucol_calcSortKeySimpleTertiary;
        } else {
            coll->sortKeyGen = ucol_calcSortKey;
        }
        if(coll->caseLevel == UCOL_OFF && coll->strength <= UCOL_TERTIARY && coll->numericCollation == UCOL_OFF
            && coll->alternateHandling == UCOL_NON_IGNORABLE && !coll->latinOneFailed)
        {
            if(coll->latinOneCEs == NULL || coll->latinOneRegenTable) {
                if(ucol_setUpLatinOne(coll, status)) { // if we succeed in building latin1 table, we'll use it
                    //fprintf(stderr, "F");
                    coll->latinOneUse = TRUE;
                } else {
                    coll->latinOneUse = FALSE;
                }
                if(*status == U_UNSUPPORTED_ERROR) {
                    *status = U_ZERO_ERROR;
                }
            } else { // latin1Table exists and it doesn't need to be regenerated, just use it
                coll->latinOneUse = TRUE;
            }
        } else {
            coll->latinOneUse = FALSE;
        }
    }
}

U_CAPI uint32_t  U_EXPORT2
ucol_setVariableTop(UCollator *coll, const UChar *varTop, int32_t len, UErrorCode *status) {
    if(U_FAILURE(*status) || coll == NULL) {
        return 0;
    }
    if(len == -1) {
        len = u_strlen(varTop);
    }
    if(len == 0) {
        *status = U_ILLEGAL_ARGUMENT_ERROR;
        return 0;
    }

    collIterate s;
    IInit_collIterate(coll, varTop, len, &s, status);
    if(U_FAILURE(*status)) {
        return 0;
    }

    uint32_t CE = ucol_IGetNextCE(coll, &s, status);

    /* here we check if we have consumed all characters */
    /* you can put in either one character or a contraction */
    /* you shouldn't put more... */
    if(s.pos != s.endp || CE == UCOL_NO_MORE_CES) {
        *status = U_CE_NOT_FOUND_ERROR;
        return 0;
    }

    uint32_t nextCE = ucol_IGetNextCE(coll, &s, status);

    if(isContinuation(nextCE) && (nextCE & UCOL_PRIMARYMASK) != 0) {
        *status = U_PRIMARY_TOO_LONG_ERROR;
        return 0;
    }
    if(coll->variableTopValue != (CE & UCOL_PRIMARYMASK)>>16) {
        coll->variableTopValueisDefault = FALSE;
        coll->variableTopValue = (CE & UCOL_PRIMARYMASK)>>16;
    }
    
    /* To avoid memory leak, free the offset buffer if necessary. */
    ucol_freeOffsetBuffer(&s);

    return CE & UCOL_PRIMARYMASK;
}

U_CAPI uint32_t U_EXPORT2 ucol_getVariableTop(const UCollator *coll, UErrorCode *status) {
    if(U_FAILURE(*status) || coll == NULL) {
        return 0;
    }
    return coll->variableTopValue<<16;
}

U_CAPI void  U_EXPORT2
ucol_restoreVariableTop(UCollator *coll, const uint32_t varTop, UErrorCode *status) {
    if(U_FAILURE(*status) || coll == NULL) {
        return;
    }

    if(coll->variableTopValue != (varTop & UCOL_PRIMARYMASK)>>16) {
        coll->variableTopValueisDefault = FALSE;
        coll->variableTopValue = (varTop & UCOL_PRIMARYMASK)>>16;
    }
}
/* Attribute setter API */
U_CAPI void  U_EXPORT2
ucol_setAttribute(UCollator *coll, UColAttribute attr, UColAttributeValue value, UErrorCode *status) {
    if(U_FAILURE(*status) || coll == NULL) {
      return;
    }
    UColAttributeValue oldFrench = coll->frenchCollation;
    UColAttributeValue oldCaseFirst = coll->caseFirst;
    switch(attr) {
    case UCOL_NUMERIC_COLLATION: /* sort substrings of digits as numbers */
        if(value == UCOL_ON) {
            coll->numericCollation = UCOL_ON;
            coll->numericCollationisDefault = FALSE;
        } else if (value == UCOL_OFF) {
            coll->numericCollation = UCOL_OFF;
            coll->numericCollationisDefault = FALSE;
        } else if (value == UCOL_DEFAULT) {
            coll->numericCollationisDefault = TRUE;
            coll->numericCollation = (UColAttributeValue)coll->options->numericCollation;
        } else {
            *status = U_ILLEGAL_ARGUMENT_ERROR;
        }
        break;
    case UCOL_HIRAGANA_QUATERNARY_MODE: /* special quaternary values for Hiragana */
        if(value == UCOL_ON) {
            coll->hiraganaQ = UCOL_ON;
            coll->hiraganaQisDefault = FALSE;
        } else if (value == UCOL_OFF) {
            coll->hiraganaQ = UCOL_OFF;
            coll->hiraganaQisDefault = FALSE;
        } else if (value == UCOL_DEFAULT) {
            coll->hiraganaQisDefault = TRUE;
            coll->hiraganaQ = (UColAttributeValue)coll->options->hiraganaQ;
        } else {
            *status = U_ILLEGAL_ARGUMENT_ERROR;
        }
        break;
    case UCOL_FRENCH_COLLATION: /* attribute for direction of secondary weights*/
        if(value == UCOL_ON) {
            coll->frenchCollation = UCOL_ON;
            coll->frenchCollationisDefault = FALSE;
        } else if (value == UCOL_OFF) {
            coll->frenchCollation = UCOL_OFF;
            coll->frenchCollationisDefault = FALSE;
        } else if (value == UCOL_DEFAULT) {
            coll->frenchCollationisDefault = TRUE;
            coll->frenchCollation = (UColAttributeValue)coll->options->frenchCollation;
        } else {
            *status = U_ILLEGAL_ARGUMENT_ERROR  ;
        }
        break;
    case UCOL_ALTERNATE_HANDLING: /* attribute for handling variable elements*/
        if(value == UCOL_SHIFTED) {
            coll->alternateHandling = UCOL_SHIFTED;
            coll->alternateHandlingisDefault = FALSE;
        } else if (value == UCOL_NON_IGNORABLE) {
            coll->alternateHandling = UCOL_NON_IGNORABLE;
            coll->alternateHandlingisDefault = FALSE;
        } else if (value == UCOL_DEFAULT) {
            coll->alternateHandlingisDefault = TRUE;
            coll->alternateHandling = (UColAttributeValue)coll->options->alternateHandling ;
        } else {
            *status = U_ILLEGAL_ARGUMENT_ERROR  ;
        }
        break;
    case UCOL_CASE_FIRST: /* who goes first, lower case or uppercase */
        if(value == UCOL_LOWER_FIRST) {
            coll->caseFirst = UCOL_LOWER_FIRST;
            coll->caseFirstisDefault = FALSE;
        } else if (value == UCOL_UPPER_FIRST) {
            coll->caseFirst = UCOL_UPPER_FIRST;
            coll->caseFirstisDefault = FALSE;
        } else if (value == UCOL_OFF) {
            coll->caseFirst = UCOL_OFF;
            coll->caseFirstisDefault = FALSE;
        } else if (value == UCOL_DEFAULT) {
            coll->caseFirst = (UColAttributeValue)coll->options->caseFirst;
            coll->caseFirstisDefault = TRUE;
        } else {
            *status = U_ILLEGAL_ARGUMENT_ERROR  ;
        }
        break;
    case UCOL_CASE_LEVEL: /* do we have an extra case level */
        if(value == UCOL_ON) {
            coll->caseLevel = UCOL_ON;
            coll->caseLevelisDefault = FALSE;
        } else if (value == UCOL_OFF) {
            coll->caseLevel = UCOL_OFF;
            coll->caseLevelisDefault = FALSE;
        } else if (value == UCOL_DEFAULT) {
            coll->caseLevel = (UColAttributeValue)coll->options->caseLevel;
            coll->caseLevelisDefault = TRUE;
        } else {
            *status = U_ILLEGAL_ARGUMENT_ERROR  ;
        }
        break;
    case UCOL_NORMALIZATION_MODE: /* attribute for normalization */
        if(value == UCOL_ON) {
            coll->normalizationMode = UCOL_ON;
            coll->normalizationModeisDefault = FALSE;
            initializeFCD(status);
        } else if (value == UCOL_OFF) {
            coll->normalizationMode = UCOL_OFF;
            coll->normalizationModeisDefault = FALSE;
        } else if (value == UCOL_DEFAULT) {
            coll->normalizationModeisDefault = TRUE;
            coll->normalizationMode = (UColAttributeValue)coll->options->normalizationMode;
            if(coll->normalizationMode == UCOL_ON) {
                initializeFCD(status);
            }
        } else {
            *status = U_ILLEGAL_ARGUMENT_ERROR  ;
        }
        break;
    case UCOL_STRENGTH:         /* attribute for strength */
        if (value == UCOL_DEFAULT) {
            coll->strengthisDefault = TRUE;
            coll->strength = (UColAttributeValue)coll->options->strength;
        } else if (value <= UCOL_IDENTICAL) {
            coll->strengthisDefault = FALSE;
            coll->strength = value;
        } else {
            *status = U_ILLEGAL_ARGUMENT_ERROR  ;
        }
        break;
    case UCOL_ATTRIBUTE_COUNT:
    default:
        *status = U_ILLEGAL_ARGUMENT_ERROR;
        break;
    }
    if(oldFrench != coll->frenchCollation || oldCaseFirst != coll->caseFirst) {
        coll->latinOneRegenTable = TRUE;
    } else {
        coll->latinOneRegenTable = FALSE;
    }
    ucol_updateInternalState(coll, status);
}

U_CAPI UColAttributeValue  U_EXPORT2
ucol_getAttribute(const UCollator *coll, UColAttribute attr, UErrorCode *status) {
    if(U_FAILURE(*status) || coll == NULL) {
      return UCOL_DEFAULT;
    }
    switch(attr) {
    case UCOL_NUMERIC_COLLATION:
      return coll->numericCollation;
    case UCOL_HIRAGANA_QUATERNARY_MODE:
      return coll->hiraganaQ;
    case UCOL_FRENCH_COLLATION: /* attribute for direction of secondary weights*/
        return coll->frenchCollation;
    case UCOL_ALTERNATE_HANDLING: /* attribute for handling variable elements*/
        return coll->alternateHandling;
    case UCOL_CASE_FIRST: /* who goes first, lower case or uppercase */
        return coll->caseFirst;
    case UCOL_CASE_LEVEL: /* do we have an extra case level */
        return coll->caseLevel;
    case UCOL_NORMALIZATION_MODE: /* attribute for normalization */
        return coll->normalizationMode;
    case UCOL_STRENGTH:         /* attribute for strength */
        return coll->strength;
    case UCOL_ATTRIBUTE_COUNT:
    default:
        *status = U_ILLEGAL_ARGUMENT_ERROR;
        break;
    }
    return UCOL_DEFAULT;
}

U_CAPI void U_EXPORT2
ucol_setStrength(    UCollator                *coll,
            UCollationStrength        strength)
{
    UErrorCode status = U_ZERO_ERROR;
    ucol_setAttribute(coll, UCOL_STRENGTH, strength, &status);
}

U_CAPI UCollationStrength U_EXPORT2
ucol_getStrength(const UCollator *coll)
{
    UErrorCode status = U_ZERO_ERROR;
    return ucol_getAttribute(coll, UCOL_STRENGTH, &status);
}

U_INTERNAL int32_t U_EXPORT2 
ucol_getReorderCodes(const UCollator *coll,
                    int32_t *dest,
                    int32_t destCapacity,
                    UErrorCode *pErrorCode) {
    if (U_FAILURE(*pErrorCode)) {
        return 0;
    }
    
    if (destCapacity < 0 || (destCapacity > 0 && dest == NULL)) {
        *pErrorCode = U_ILLEGAL_ARGUMENT_ERROR;
        return 0;
    }
    
    if (coll->reorderCodesLength > destCapacity) {
        *pErrorCode = U_BUFFER_OVERFLOW_ERROR;
        return coll->reorderCodesLength;
    }
    for (int32_t i = 0; i < coll->reorderCodesLength; i++) {
        dest[i] = coll->reorderCodes[i];
    }
    return coll->reorderCodesLength;
}

U_INTERNAL void U_EXPORT2 
ucol_setReorderCodes(UCollator *coll,
                    const int32_t *reorderCodes,
                    int32_t reorderCodesLength,
                    UErrorCode *pErrorCode) {
    if (U_FAILURE(*pErrorCode)) {
        return;
    }

    if (reorderCodesLength < 0 || (reorderCodesLength > 0 && reorderCodes == NULL)) {
        *pErrorCode = U_ILLEGAL_ARGUMENT_ERROR;
        return;
    }
    
    uprv_free(coll->reorderCodes);
    coll->reorderCodes = NULL;
    coll->reorderCodesLength = 0;
    if (reorderCodesLength == 0) {
        uprv_free(coll->leadBytePermutationTable);
        coll->leadBytePermutationTable = NULL;
        return;
    }
    coll->reorderCodes = (int32_t*) uprv_malloc(reorderCodesLength * sizeof(int32_t));
    if (coll->reorderCodes == NULL) {
        *pErrorCode = U_MEMORY_ALLOCATION_ERROR;
        return;
    }
    for (int32_t i = 0; i < reorderCodesLength; i++) {
        coll->reorderCodes[i] = reorderCodes[i];
    }
    coll->reorderCodesLength = reorderCodesLength;
    ucol_buildPermutationTable(coll, pErrorCode);
    if (U_FAILURE(*pErrorCode)) {
        uprv_free(coll->reorderCodes);
        coll->reorderCodes = NULL;
        coll->reorderCodesLength = 0;
    }    
}


/****************************************************************************/
/* Following are misc functions                                             */
/* there are new APIs and some compatibility APIs                           */
/****************************************************************************/

U_CAPI void U_EXPORT2
ucol_getVersion(const UCollator* coll,
                UVersionInfo versionInfo)
{
    /* RunTime version  */
    uint8_t rtVersion = UCOL_RUNTIME_VERSION;
    /* Builder version*/
    uint8_t bdVersion = coll->image->version[0];

    /* Charset Version. Need to get the version from cnv files
     * makeconv should populate cnv files with version and
     * an api has to be provided in ucnv.h to obtain this version
     */
    uint8_t csVersion = 0;

    /* combine the version info */
    uint16_t cmbVersion = (uint16_t)((rtVersion<<11) | (bdVersion<<6) | (csVersion));

    /* Tailoring rules */
    versionInfo[0] = (uint8_t)(cmbVersion>>8);
    versionInfo[1] = (uint8_t)cmbVersion;
    versionInfo[2] = coll->image->version[1];
    if(coll->UCA) {
        /* Include the minor number when getting the UCA version. (major & 1f) << 3 | (minor & 7) */
        versionInfo[3] = (coll->UCA->image->UCAVersion[0] & 0x1f) << 3 | (coll->UCA->image->UCAVersion[1] & 0x07);
    } else {
        versionInfo[3] = 0;
    }
}


/* This internal API checks whether a character is tailored or not */
U_CAPI UBool  U_EXPORT2
ucol_isTailored(const UCollator *coll, const UChar u, UErrorCode *status) {
    if(U_FAILURE(*status) || coll == NULL || coll == coll->UCA) {
        return FALSE;
    }

    uint32_t CE = UCOL_NOT_FOUND;
    const UChar *ContractionStart = NULL;
    if(u < 0x100) { /* latin-1 */
        CE = coll->latinOneMapping[u];
        if(coll->UCA && CE == coll->UCA->latinOneMapping[u]) {
            return FALSE;
        }
    } else { /* regular */
        CE = UTRIE_GET32_FROM_LEAD(&coll->mapping, u);
    }

    if(isContraction(CE)) {
        ContractionStart = (UChar *)coll->image+getContractOffset(CE);
        CE = *(coll->contractionCEs + (ContractionStart- coll->contractionIndex));
    }

    return (UBool)(CE != UCOL_NOT_FOUND);
}


/****************************************************************************/
/* Following are the string compare functions                               */
/*                                                                          */
/****************************************************************************/


/*  ucol_checkIdent    internal function.  Does byte level string compare.   */
/*                     Used by strcoll if strength == identical and strings  */
/*                     are otherwise equal.                                  */
/*                                                                           */
/*                     Comparison must be done on NFD normalized strings.    */
/*                     FCD is not good enough.                               */

static
UCollationResult    ucol_checkIdent(collIterate *sColl, collIterate *tColl, UBool normalize, UErrorCode *status)
{
    // When we arrive here, we can have normal strings or UCharIterators. Currently they are both
    // of same type, but that doesn't really mean that it will stay that way.
    int32_t            comparison;

    if (sColl->flags & UCOL_USE_ITERATOR) {
        // The division for the array length may truncate the array size to
        // a little less than UNORM_ITER_SIZE, but that size is dimensioned too high
        // for all platforms anyway.
        UAlignedMemory stackNormIter1[UNORM_ITER_SIZE/sizeof(UAlignedMemory)];
        UAlignedMemory stackNormIter2[UNORM_ITER_SIZE/sizeof(UAlignedMemory)];
        UNormIterator *sNIt = NULL, *tNIt = NULL;
        sNIt = unorm_openIter(stackNormIter1, sizeof(stackNormIter1), status);
        tNIt = unorm_openIter(stackNormIter2, sizeof(stackNormIter2), status);
        sColl->iterator->move(sColl->iterator, 0, UITER_START);
        tColl->iterator->move(tColl->iterator, 0, UITER_START);
        UCharIterator *sIt = unorm_setIter(sNIt, sColl->iterator, UNORM_NFD, status);
        UCharIterator *tIt = unorm_setIter(tNIt, tColl->iterator, UNORM_NFD, status);
        comparison = u_strCompareIter(sIt, tIt, TRUE);
        unorm_closeIter(sNIt);
        unorm_closeIter(tNIt);
    } else {
        int32_t sLen      = (sColl->flags & UCOL_ITER_HASLEN) ? (int32_t)(sColl->endp - sColl->string) : -1;
        const UChar *sBuf = sColl->string;
        int32_t tLen      = (tColl->flags & UCOL_ITER_HASLEN) ? (int32_t)(tColl->endp - tColl->string) : -1;
        const UChar *tBuf = tColl->string;

        if (normalize) {
            *status = U_ZERO_ERROR;
            // Note: We could use Normalizer::compare() or similar, but for short strings
            // which may not be in FCD it might be faster to just NFD them.
            // Note: spanQuickCheckYes() + normalizeSecondAndAppend() rather than
            // NFD'ing immediately might be faster for long strings,
            // but string comparison is usually done on relatively short strings.
            sColl->nfd->normalize(UnicodeString((sColl->flags & UCOL_ITER_HASLEN) == 0, sBuf, sLen),
                                  sColl->writableBuffer,
                                  *status);
            tColl->nfd->normalize(UnicodeString((tColl->flags & UCOL_ITER_HASLEN) == 0, tBuf, tLen),
                                  tColl->writableBuffer,
                                  *status);
            if(U_FAILURE(*status)) {
                return UCOL_LESS;
            }
            comparison = sColl->writableBuffer.compareCodePointOrder(tColl->writableBuffer);
        } else {
            comparison = u_strCompare(sBuf, sLen, tBuf, tLen, TRUE);
        }
    }

    if (comparison < 0) {
        return UCOL_LESS;
    } else if (comparison == 0) {
        return UCOL_EQUAL;
    } else /* comparison > 0 */ {
        return UCOL_GREATER;
    }
}

/*  CEBuf - A struct and some inline functions to handle the saving    */
/*          of CEs in a buffer within ucol_strcoll                     */

#define UCOL_CEBUF_SIZE 512
typedef struct ucol_CEBuf {
    uint32_t    *buf;
    uint32_t    *endp;
    uint32_t    *pos;
    uint32_t     localArray[UCOL_CEBUF_SIZE];
} ucol_CEBuf;


static
inline void UCOL_INIT_CEBUF(ucol_CEBuf *b) {
    (b)->buf = (b)->pos = (b)->localArray;
    (b)->endp = (b)->buf + UCOL_CEBUF_SIZE;
}

static
void ucol_CEBuf_Expand(ucol_CEBuf *b, collIterate *ci, UErrorCode *status) {
    uint32_t  oldSize;
    uint32_t  newSize;
    uint32_t  *newBuf;

    ci->flags |= UCOL_ITER_ALLOCATED;
    oldSize = (uint32_t)(b->pos - b->buf);
    newSize = oldSize * 2;
    newBuf = (uint32_t *)uprv_malloc(newSize * sizeof(uint32_t));
    if(newBuf == NULL) {
        *status = U_MEMORY_ALLOCATION_ERROR;
    }
    else {
        uprv_memcpy(newBuf, b->buf, oldSize * sizeof(uint32_t));
        if (b->buf != b->localArray) {
            uprv_free(b->buf);
        }
        b->buf = newBuf;
        b->endp = b->buf + newSize;
        b->pos  = b->buf + oldSize;
    }
}

static
inline void UCOL_CEBUF_PUT(ucol_CEBuf *b, uint32_t ce, collIterate *ci, UErrorCode *status) {
    if (b->pos == b->endp) {
        ucol_CEBuf_Expand(b, ci, status);
    }
    if (U_SUCCESS(*status)) {
        *(b)->pos++ = ce;
    }
}

/* This is a trick string compare function that goes in and uses sortkeys to compare */
/* It is used when compare gets in trouble and needs to bail out                     */
static UCollationResult ucol_compareUsingSortKeys(collIterate *sColl,
                                                  collIterate *tColl,
                                                  UErrorCode *status)
{
    uint8_t sourceKey[UCOL_MAX_BUFFER], targetKey[UCOL_MAX_BUFFER];
    uint8_t *sourceKeyP = sourceKey;
    uint8_t *targetKeyP = targetKey;
    int32_t sourceKeyLen = UCOL_MAX_BUFFER, targetKeyLen = UCOL_MAX_BUFFER;
    const UCollator *coll = sColl->coll;
    const UChar *source = NULL;
    const UChar *target = NULL;
    int32_t result = UCOL_EQUAL;
    UnicodeString sourceString, targetString;
    int32_t sourceLength;
    int32_t targetLength;

    if(sColl->flags & UCOL_USE_ITERATOR) {
        sColl->iterator->move(sColl->iterator, 0, UITER_START);
        tColl->iterator->move(tColl->iterator, 0, UITER_START);
        UChar32 c;
        while((c=sColl->iterator->next(sColl->iterator))>=0) {
            sourceString.append((UChar)c);
        }
        while((c=tColl->iterator->next(tColl->iterator))>=0) {
            targetString.append((UChar)c);
        }
        source = sourceString.getBuffer();
        sourceLength = sourceString.length();
        target = targetString.getBuffer();
        targetLength = targetString.length();
    } else { // no iterators
        sourceLength = (sColl->flags&UCOL_ITER_HASLEN)?(int32_t)(sColl->endp-sColl->string):-1;
        targetLength = (tColl->flags&UCOL_ITER_HASLEN)?(int32_t)(tColl->endp-tColl->string):-1;
        source = sColl->string;
        target = tColl->string;
    }



    sourceKeyLen = ucol_getSortKey(coll, source, sourceLength, sourceKeyP, sourceKeyLen);
    if(sourceKeyLen > UCOL_MAX_BUFFER) {
        sourceKeyP = (uint8_t*)uprv_malloc(sourceKeyLen*sizeof(uint8_t));
        if(sourceKeyP == NULL) {
            *status = U_MEMORY_ALLOCATION_ERROR;
            goto cleanup_and_do_compare;
        }
        sourceKeyLen = ucol_getSortKey(coll, source, sourceLength, sourceKeyP, sourceKeyLen);
    }

    targetKeyLen = ucol_getSortKey(coll, target, targetLength, targetKeyP, targetKeyLen);
    if(targetKeyLen > UCOL_MAX_BUFFER) {
        targetKeyP = (uint8_t*)uprv_malloc(targetKeyLen*sizeof(uint8_t));
        if(targetKeyP == NULL) {
            *status = U_MEMORY_ALLOCATION_ERROR;
            goto cleanup_and_do_compare;
        }
        targetKeyLen = ucol_getSortKey(coll, target, targetLength, targetKeyP, targetKeyLen);
    }

    result = uprv_strcmp((const char*)sourceKeyP, (const char*)targetKeyP);

cleanup_and_do_compare:
    if(sourceKeyP != NULL && sourceKeyP != sourceKey) {
        uprv_free(sourceKeyP);
    }

    if(targetKeyP != NULL && targetKeyP != targetKey) {
        uprv_free(targetKeyP);
    }

    if(result<0) {
        return UCOL_LESS;
    } else if(result>0) {
        return UCOL_GREATER;
    } else {
        return UCOL_EQUAL;
    }
}


static UCollationResult
ucol_strcollRegular(collIterate *sColl, collIterate *tColl, UErrorCode *status)
{
    U_ALIGN_CODE(16);

    const UCollator *coll = sColl->coll;


    // setting up the collator parameters
    UColAttributeValue strength = coll->strength;
    UBool initialCheckSecTer = (strength  >= UCOL_SECONDARY);

    UBool checkSecTer = initialCheckSecTer;
    UBool checkTertiary = (strength  >= UCOL_TERTIARY);
    UBool checkQuad = (strength  >= UCOL_QUATERNARY);
    UBool checkIdent = (strength == UCOL_IDENTICAL);
    UBool checkCase = (coll->caseLevel == UCOL_ON);
    UBool isFrenchSec = (coll->frenchCollation == UCOL_ON) && checkSecTer;
    UBool shifted = (coll->alternateHandling == UCOL_SHIFTED);
    UBool qShifted = shifted && checkQuad;
    UBool doHiragana = (coll->hiraganaQ == UCOL_ON) && checkQuad;

    if(doHiragana && shifted) {
        return (ucol_compareUsingSortKeys(sColl, tColl, status));
    }
    uint8_t caseSwitch = coll->caseSwitch;
    uint8_t tertiaryMask = coll->tertiaryMask;

    // This is the lowest primary value that will not be ignored if shifted
    uint32_t LVT = (shifted)?(coll->variableTopValue<<16):0;

    UCollationResult result = UCOL_EQUAL;
    UCollationResult hirResult = UCOL_EQUAL;

    // Preparing the CE buffers. They will be filled during the primary phase
    ucol_CEBuf   sCEs;
    ucol_CEBuf   tCEs;
    UCOL_INIT_CEBUF(&sCEs);
    UCOL_INIT_CEBUF(&tCEs);

    uint32_t secS = 0, secT = 0;
    uint32_t sOrder=0, tOrder=0;

    // Non shifted primary processing is quite simple
    if(!shifted) {
        for(;;) {

            // We fetch CEs until we hit a non ignorable primary or end.
            do {
                // We get the next CE
                sOrder = ucol_IGetNextCE(coll, sColl, status);
                // Stuff it in the buffer
                UCOL_CEBUF_PUT(&sCEs, sOrder, sColl, status);
                // And keep just the primary part.
                sOrder &= UCOL_PRIMARYMASK;
            } while(sOrder == 0);

            // see the comments on the above block
            do {
                tOrder = ucol_IGetNextCE(coll, tColl, status);
                UCOL_CEBUF_PUT(&tCEs, tOrder, tColl, status);
                tOrder &= UCOL_PRIMARYMASK;
            } while(tOrder == 0);

            // if both primaries are the same
            if(sOrder == tOrder) {
                // and there are no more CEs, we advance to the next level
                if(sOrder == UCOL_NO_MORE_CES_PRIMARY) {
                    break;
                }
                if(doHiragana && hirResult == UCOL_EQUAL) {
                    if((sColl->flags & UCOL_WAS_HIRAGANA) != (tColl->flags & UCOL_WAS_HIRAGANA)) {
                        hirResult = ((sColl->flags & UCOL_WAS_HIRAGANA) > (tColl->flags & UCOL_WAS_HIRAGANA))
                            ? UCOL_LESS:UCOL_GREATER;
                    }
                }
            } else {
                // only need to check one for continuation
                // if one is then the other must be or the preceding CE would be a prefix of the other
                if (coll->leadBytePermutationTable != NULL && !isContinuation(sOrder)) {
                    sOrder = (coll->leadBytePermutationTable[sOrder>>24] << 24) | (sOrder & 0x00FFFFFF);
                    tOrder = (coll->leadBytePermutationTable[tOrder>>24] << 24) | (tOrder & 0x00FFFFFF);
                }
                // if two primaries are different, we are done
                result = (sOrder < tOrder) ?  UCOL_LESS: UCOL_GREATER;
                goto commonReturn;
            }
        } // no primary difference... do the rest from the buffers
    } else { // shifted - do a slightly more complicated processing :)
        for(;;) {
            UBool sInShifted = FALSE;
            UBool tInShifted = FALSE;
            // This version of code can be refactored. However, it seems easier to understand this way.
            // Source loop. Sam as the target loop.
            for(;;) {
                sOrder = ucol_IGetNextCE(coll, sColl, status);
                if(sOrder == UCOL_NO_MORE_CES) {
                    UCOL_CEBUF_PUT(&sCEs, sOrder, sColl, status);
                    break;
                } else if(sOrder == 0 || (sInShifted && (sOrder & UCOL_PRIMARYMASK) == 0)) {
                    /* UCA amendment - ignore ignorables that follow shifted code points */
                    continue;
                } else if(isContinuation(sOrder)) {
                    if((sOrder & UCOL_PRIMARYMASK) > 0) { /* There is primary value */
                        if(sInShifted) {
                            sOrder = (sOrder & UCOL_PRIMARYMASK) | 0xC0; /* preserve interesting continuation */
                            UCOL_CEBUF_PUT(&sCEs, sOrder, sColl, status);
                            continue;
                        } else {
                            UCOL_CEBUF_PUT(&sCEs, sOrder, sColl, status);
                            break;
                        }
                    } else { /* Just lower level values */
                        if(sInShifted) {
                            continue;
                        } else {
                            UCOL_CEBUF_PUT(&sCEs, sOrder, sColl, status);
                            continue;
                        }
                    }
                } else { /* regular */
                    if(coll->leadBytePermutationTable != NULL){
                        sOrder = (coll->leadBytePermutationTable[sOrder>>24] << 24) | (sOrder & 0x00FFFFFF);
                    }
                    if((sOrder & UCOL_PRIMARYMASK) > LVT) {
                        UCOL_CEBUF_PUT(&sCEs, sOrder, sColl, status);
                        break;
                    } else {
                        if((sOrder & UCOL_PRIMARYMASK) > 0) {
                            sInShifted = TRUE;
                            sOrder &= UCOL_PRIMARYMASK;
                            UCOL_CEBUF_PUT(&sCEs, sOrder, sColl, status);
                            continue;
                        } else {
                            UCOL_CEBUF_PUT(&sCEs, sOrder, sColl, status);
                            sInShifted = FALSE;
                            continue;
                        }
                    }
                }
            }
            sOrder &= UCOL_PRIMARYMASK;
            sInShifted = FALSE;

            for(;;) {
                tOrder = ucol_IGetNextCE(coll, tColl, status);
                if(tOrder == UCOL_NO_MORE_CES) {
                    UCOL_CEBUF_PUT(&tCEs, tOrder, tColl, status);
                    break;
                } else if(tOrder == 0 || (tInShifted && (tOrder & UCOL_PRIMARYMASK) == 0)) {
                    /* UCA amendment - ignore ignorables that follow shifted code points */
                    continue;
                } else if(isContinuation(tOrder)) {
                    if((tOrder & UCOL_PRIMARYMASK) > 0) { /* There is primary value */
                        if(tInShifted) {
                            tOrder = (tOrder & UCOL_PRIMARYMASK) | 0xC0; /* preserve interesting continuation */
                            UCOL_CEBUF_PUT(&tCEs, tOrder, tColl, status);
                            continue;
                        } else {
                            UCOL_CEBUF_PUT(&tCEs, tOrder, tColl, status);
                            break;
                        }
                    } else { /* Just lower level values */
                        if(tInShifted) {
                            continue;
                        } else {
                            UCOL_CEBUF_PUT(&tCEs, tOrder, tColl, status);
                            continue;
                        }
                    }
                } else { /* regular */
                    if(coll->leadBytePermutationTable != NULL){
                        tOrder = (coll->leadBytePermutationTable[tOrder>>24] << 24) | (tOrder & 0x00FFFFFF);
                    }
                    if((tOrder & UCOL_PRIMARYMASK) > LVT) {
                        UCOL_CEBUF_PUT(&tCEs, tOrder, tColl, status);
                        break;
                    } else {
                        if((tOrder & UCOL_PRIMARYMASK) > 0) {
                            tInShifted = TRUE;
                            tOrder &= UCOL_PRIMARYMASK;
                            UCOL_CEBUF_PUT(&tCEs, tOrder, tColl, status);
                            continue;
                        } else {
                            UCOL_CEBUF_PUT(&tCEs, tOrder, tColl, status);
                            tInShifted = FALSE;
                            continue;
                        }
                    }
                }
            }
            tOrder &= UCOL_PRIMARYMASK;
            tInShifted = FALSE;

            if(sOrder == tOrder) {
                /*
                if(doHiragana && hirResult == UCOL_EQUAL) {
                if((sColl.flags & UCOL_WAS_HIRAGANA) != (tColl.flags & UCOL_WAS_HIRAGANA)) {
                hirResult = ((sColl.flags & UCOL_WAS_HIRAGANA) > (tColl.flags & UCOL_WAS_HIRAGANA))
                ? UCOL_LESS:UCOL_GREATER;
                }
                }
                */
                if(sOrder == UCOL_NO_MORE_CES_PRIMARY) {
                    break;
                } else {
                    sOrder = 0;
                    tOrder = 0;
                    continue;
                }
            } else {
                result = (sOrder < tOrder) ? UCOL_LESS : UCOL_GREATER;
                goto commonReturn;
            }
        } /* no primary difference... do the rest from the buffers */
    }

    /* now, we're gonna reexamine collected CEs */
    uint32_t    *sCE;
    uint32_t    *tCE;

    /* This is the secondary level of comparison */
    if(checkSecTer) {
        if(!isFrenchSec) { /* normal */
            sCE = sCEs.buf;
            tCE = tCEs.buf;
            for(;;) {
                while (secS == 0) {
                    secS = *(sCE++) & UCOL_SECONDARYMASK;
                }

                while(secT == 0) {
                    secT = *(tCE++) & UCOL_SECONDARYMASK;
                }

                if(secS == secT) {
                    if(secS == UCOL_NO_MORE_CES_SECONDARY) {
                        break;
                    } else {
                        secS = 0; secT = 0;
                        continue;
                    }
                } else {
                    result = (secS < secT) ? UCOL_LESS : UCOL_GREATER;
                    goto commonReturn;
                }
            }
        } else { /* do the French */
            uint32_t *sCESave = NULL;
            uint32_t *tCESave = NULL;
            sCE = sCEs.pos-2; /* this could also be sCEs-- if needs to be optimized */
            tCE = tCEs.pos-2;
            for(;;) {
                while (secS == 0 && sCE >= sCEs.buf) {
                    if(sCESave == NULL) {
                        secS = *(sCE--);
                        if(isContinuation(secS)) {
                            while(isContinuation(secS = *(sCE--)))
                                ;
                            /* after this, secS has the start of continuation, and sCEs points before that */
                            sCESave = sCE; /* we save it, so that we know where to come back AND that we need to go forward */
                            sCE+=2;  /* need to point to the first continuation CP */
                            /* However, now you can just continue doing stuff */
                        }
                    } else {
                        secS = *(sCE++);
                        if(!isContinuation(secS)) { /* This means we have finished with this cont */
                            sCE = sCESave;            /* reset the pointer to before continuation */
                            sCESave = NULL;
                            secS = 0;  /* Fetch a fresh CE before the continuation sequence. */
                            continue;
                        }
                    }
                    secS &= UCOL_SECONDARYMASK; /* remove the continuation bit */
                }

                while(secT == 0 && tCE >= tCEs.buf) {
                    if(tCESave == NULL) {
                        secT = *(tCE--);
                        if(isContinuation(secT)) {
                            while(isContinuation(secT = *(tCE--)))
                                ;
                            /* after this, secS has the start of continuation, and sCEs points before that */
                            tCESave = tCE; /* we save it, so that we know where to come back AND that we need to go forward */
                            tCE+=2;  /* need to point to the first continuation CP */
                            /* However, now you can just continue doing stuff */
                        }
                    } else {
                        secT = *(tCE++);
                        if(!isContinuation(secT)) { /* This means we have finished with this cont */
                            tCE = tCESave;          /* reset the pointer to before continuation */
                            tCESave = NULL;
                            secT = 0;  /* Fetch a fresh CE before the continuation sequence. */
                            continue;
                        }
                    }
                    secT &= UCOL_SECONDARYMASK; /* remove the continuation bit */
                }

                if(secS == secT) {
                    if(secS == UCOL_NO_MORE_CES_SECONDARY || (sCE < sCEs.buf && tCE < tCEs.buf)) {
                        break;
                    } else {
                        secS = 0; secT = 0;
                        continue;
                    }
                } else {
                    result = (secS < secT) ? UCOL_LESS : UCOL_GREATER;
                    goto commonReturn;
                }
            }
        }
    }

    /* doing the case bit */
    if(checkCase) {
        sCE = sCEs.buf;
        tCE = tCEs.buf;
        for(;;) {
            while((secS & UCOL_REMOVE_CASE) == 0) {
                if(!isContinuation(*sCE++)) {
                    secS =*(sCE-1);
                    if(((secS & UCOL_PRIMARYMASK) != 0) || strength > UCOL_PRIMARY) {
                        // primary ignorables should not be considered on the case level when the strength is primary
                        // otherwise, the CEs stop being well-formed
                        secS &= UCOL_TERT_CASE_MASK;
                        secS ^= caseSwitch;
                    } else {
                        secS = 0;
                    }
                } else {
                    secS = 0;
                }
            }

            while((secT & UCOL_REMOVE_CASE) == 0) {
                if(!isContinuation(*tCE++)) {
                    secT = *(tCE-1);
                    if(((secT & UCOL_PRIMARYMASK) != 0) || strength > UCOL_PRIMARY) {
                        // primary ignorables should not be considered on the case level when the strength is primary
                        // otherwise, the CEs stop being well-formed
                        secT &= UCOL_TERT_CASE_MASK;
                        secT ^= caseSwitch;
                    } else {
                        secT = 0;
                    }
                } else {
                    secT = 0;
                }
            }

            if((secS & UCOL_CASE_BIT_MASK) < (secT & UCOL_CASE_BIT_MASK)) {
                result = UCOL_LESS;
                goto commonReturn;
            } else if((secS & UCOL_CASE_BIT_MASK) > (secT & UCOL_CASE_BIT_MASK)) {
                result = UCOL_GREATER;
                goto commonReturn;
            }

            if((secS & UCOL_REMOVE_CASE) == UCOL_NO_MORE_CES_TERTIARY || (secT & UCOL_REMOVE_CASE) == UCOL_NO_MORE_CES_TERTIARY ) {
                break;
            } else {
                secS = 0;
                secT = 0;
            }
        }
    }

    /* Tertiary level */
    if(checkTertiary) {
        secS = 0;
        secT = 0;
        sCE = sCEs.buf;
        tCE = tCEs.buf;
        for(;;) {
            while((secS & UCOL_REMOVE_CASE) == 0) {
                secS = *(sCE++) & tertiaryMask;
                if(!isContinuation(secS)) {
                    secS ^= caseSwitch;
                } else {
                    secS &= UCOL_REMOVE_CASE;
                }
            }

            while((secT & UCOL_REMOVE_CASE)  == 0) {
                secT = *(tCE++) & tertiaryMask;
                if(!isContinuation(secT)) {
                    secT ^= caseSwitch;
                } else {
                    secT &= UCOL_REMOVE_CASE;
                }
            }

            if(secS == secT) {
                if((secS & UCOL_REMOVE_CASE) == 1) {
                    break;
                } else {
                    secS = 0; secT = 0;
                    continue;
                }
            } else {
                result = (secS < secT) ? UCOL_LESS : UCOL_GREATER;
                goto commonReturn;
            }
        }
    }


    if(qShifted /*checkQuad*/) {
        UBool sInShifted = TRUE;
        UBool tInShifted = TRUE;
        secS = 0;
        secT = 0;
        sCE = sCEs.buf;
        tCE = tCEs.buf;
        for(;;) {
            while((secS == 0 && secS != UCOL_NO_MORE_CES) || (isContinuation(secS) && !sInShifted)) {
                secS = *(sCE++);
                if(isContinuation(secS)) {
                    if(!sInShifted) {
                        continue;
                    }
                } else if(secS > LVT || (secS & UCOL_PRIMARYMASK) == 0) { /* non continuation */
                    secS = UCOL_PRIMARYMASK;
                    sInShifted = FALSE;
                } else {
                    sInShifted = TRUE;
                }
            }
            secS &= UCOL_PRIMARYMASK;


            while((secT == 0 && secT != UCOL_NO_MORE_CES) || (isContinuation(secT) && !tInShifted)) {
                secT = *(tCE++);
                if(isContinuation(secT)) {
                    if(!tInShifted) {
                        continue;
                    }
                } else if(secT > LVT || (secT & UCOL_PRIMARYMASK) == 0) {
                    secT = UCOL_PRIMARYMASK;
                    tInShifted = FALSE;
                } else {
                    tInShifted = TRUE;
                }
            }
            secT &= UCOL_PRIMARYMASK;

            if(secS == secT) {
                if(secS == UCOL_NO_MORE_CES_PRIMARY) {
                    break;
                } else {
                    secS = 0; secT = 0;
                    continue;
                }
            } else {
                result = (secS < secT) ? UCOL_LESS : UCOL_GREATER;
                goto commonReturn;
            }
        }
    } else if(doHiragana && hirResult != UCOL_EQUAL) {
        // If we're fine on quaternaries, we might be different
        // on Hiragana. This, however, might fail us in shifted.
        result = hirResult;
        goto commonReturn;
    }

    /*  For IDENTICAL comparisons, we use a bitwise character comparison */
    /*  as a tiebreaker if all else is equal.                                */
    /*  Getting here  should be quite rare - strings are not identical -     */
    /*     that is checked first, but compared == through all other checks.  */
    if(checkIdent)
    {
        //result = ucol_checkIdent(&sColl, &tColl, coll->normalizationMode == UCOL_ON);
        result = ucol_checkIdent(sColl, tColl, TRUE, status);
    }

commonReturn:
    if ((sColl->flags | tColl->flags) & UCOL_ITER_ALLOCATED) {
        if (sCEs.buf != sCEs.localArray ) {
            uprv_free(sCEs.buf);
        }
        if (tCEs.buf != tCEs.localArray ) {
            uprv_free(tCEs.buf);
        }
    }

    return result;
}

static UCollationResult
ucol_strcollRegular(const UCollator *coll,
                    const UChar *source, int32_t sourceLength,
                    const UChar *target, int32_t targetLength,
                    UErrorCode *status) {
    collIterate sColl, tColl;
    // Preparing the context objects for iterating over strings
    IInit_collIterate(coll, source, sourceLength, &sColl, status);
    IInit_collIterate(coll, target, targetLength, &tColl, status);
    if(U_FAILURE(*status)) {
        return UCOL_LESS;
    }
    return ucol_strcollRegular(&sColl, &tColl, status);
}

static inline uint32_t
ucol_getLatinOneContraction(const UCollator *coll, int32_t strength,
                          uint32_t CE, const UChar *s, int32_t *index, int32_t len)
{
    const UChar *UCharOffset = (UChar *)coll->image+getContractOffset(CE&0xFFF);
    int32_t latinOneOffset = (CE & 0x00FFF000) >> 12;
    int32_t offset = 1;
    UChar schar = 0, tchar = 0;

    for(;;) {
        if(len == -1) {
            if(s[*index] == 0) { // end of string
                return(coll->latinOneCEs[strength*coll->latinOneTableLen+latinOneOffset]);
            } else {
                schar = s[*index];
            }
        } else {
            if(*index == len) {
                return(coll->latinOneCEs[strength*coll->latinOneTableLen+latinOneOffset]);
            } else {
                schar = s[*index];
            }
        }

        while(schar > (tchar = *(UCharOffset+offset))) { /* since the contraction codepoints should be ordered, we skip all that are smaller */
            offset++;
        }

        if (schar == tchar) {
            (*index)++;
            return(coll->latinOneCEs[strength*coll->latinOneTableLen+latinOneOffset+offset]);
        }
        else
        {
            if(schar & 0xFF00 /*> UCOL_ENDOFLATIN1RANGE*/) {
                return UCOL_BAIL_OUT_CE;
            }
            // skip completely ignorables
            uint32_t isZeroCE = UTRIE_GET32_FROM_LEAD(&coll->mapping, schar);
            if(isZeroCE == 0) { // we have to ignore completely ignorables
                (*index)++;
                continue;
            }

            return(coll->latinOneCEs[strength*coll->latinOneTableLen+latinOneOffset]);
        }
    }
}


/**
 * This is a fast strcoll, geared towards text in Latin-1.
 * It supports contractions of size two, French secondaries
 * and case switching. You can use it with strengths primary
 * to tertiary. It does not support shifted and case level.
 * It relies on the table build by setupLatin1Table. If it
 * doesn't understand something, it will go to the regular
 * strcoll.
 */
static UCollationResult
ucol_strcollUseLatin1( const UCollator    *coll,
              const UChar        *source,
              int32_t            sLen,
              const UChar        *target,
              int32_t            tLen,
              UErrorCode *status)
{
    U_ALIGN_CODE(16);
    int32_t strength = coll->strength;

    int32_t sIndex = 0, tIndex = 0;
    UChar sChar = 0, tChar = 0;
    uint32_t sOrder=0, tOrder=0;

    UBool endOfSource = FALSE;

    uint32_t *elements = coll->latinOneCEs;

    UBool haveContractions = FALSE; // if we have contractions in our string
                                    // we cannot do French secondary

    // Do the primary level
    for(;;) {
        while(sOrder==0) { // this loop skips primary ignorables
            // sOrder=getNextlatinOneCE(source);
            if(sLen==-1) {   // handling zero terminated strings
                sChar=source[sIndex++];
                if(sChar==0) {
                    endOfSource = TRUE;
                    break;
                }
            } else {        // handling strings with known length
                if(sIndex==sLen) {
                    endOfSource = TRUE;
                    break;
                }
                sChar=source[sIndex++];
            }
            if(sChar&0xFF00) { // if we encounter non-latin-1, we bail out (sChar > 0xFF, but this is faster on win32)
                //fprintf(stderr, "R");
                return ucol_strcollRegular(coll, source, sLen, target, tLen, status);
            }
            sOrder = elements[sChar];
            if(sOrder >= UCOL_NOT_FOUND) { // if we got a special
                // specials can basically be either contractions or bail-out signs. If we get anything
                // else, we'll bail out anywasy
                if(getCETag(sOrder) == CONTRACTION_TAG) {
                    sOrder = ucol_getLatinOneContraction(coll, UCOL_PRIMARY, sOrder, source, &sIndex, sLen);
                    haveContractions = TRUE; // if there are contractions, we cannot do French secondary
                    // However, if there are contractions in the table, but we always use just one char,
                    // we might be able to do French. This should be checked out.
                }
                if(sOrder >= UCOL_NOT_FOUND /*== UCOL_BAIL_OUT_CE*/) {
                    //fprintf(stderr, "S");
                    return ucol_strcollRegular(coll, source, sLen, target, tLen, status);
                }
            }
        }

        while(tOrder==0) {  // this loop skips primary ignorables
            // tOrder=getNextlatinOneCE(target);
            if(tLen==-1) {    // handling zero terminated strings
                tChar=target[tIndex++];
                if(tChar==0) {
                    if(endOfSource) { // this is different than source loop,
                        // as we already know that source loop is done here,
                        // so we can either finish the primary loop if both
                        // strings are done or anounce the result if only
                        // target is done. Same below.
                        goto endOfPrimLoop;
                    } else {
                        return UCOL_GREATER;
                    }
                }
            } else {          // handling strings with known length
                if(tIndex==tLen) {
                    if(endOfSource) {
                        goto endOfPrimLoop;
                    } else {
                        return UCOL_GREATER;
                    }
                }
                tChar=target[tIndex++];
            }
            if(tChar&0xFF00) { // if we encounter non-latin-1, we bail out (sChar > 0xFF, but this is faster on win32)
                //fprintf(stderr, "R");
                return ucol_strcollRegular(coll, source, sLen, target, tLen, status);
            }
            tOrder = elements[tChar];
            if(tOrder >= UCOL_NOT_FOUND) {
                // Handling specials, see the comments for source
                if(getCETag(tOrder) == CONTRACTION_TAG) {
                    tOrder = ucol_getLatinOneContraction(coll, UCOL_PRIMARY, tOrder, target, &tIndex, tLen);
                    haveContractions = TRUE;
                }
                if(tOrder >= UCOL_NOT_FOUND /*== UCOL_BAIL_OUT_CE*/) {
                    //fprintf(stderr, "S");
                    return ucol_strcollRegular(coll, source, sLen, target, tLen, status);
                }
            }
        }
        if(endOfSource) { // source is finished, but target is not, say the result.
            return UCOL_LESS;
        }

        if(sOrder == tOrder) { // if we have same CEs, we continue the loop
            sOrder = 0; tOrder = 0;
            continue;
        } else {
            // compare current top bytes
            if(((sOrder^tOrder)&0xFF000000)!=0) {
                // top bytes differ, return difference
                if(sOrder < tOrder) {
                    return UCOL_LESS;
                } else if(sOrder > tOrder) {
                    return UCOL_GREATER;
                }
                // instead of return (int32_t)(sOrder>>24)-(int32_t)(tOrder>>24);
                // since we must return enum value
            }

            // top bytes match, continue with following bytes
            sOrder<<=8;
            tOrder<<=8;
        }
    }

endOfPrimLoop:
    // after primary loop, we definitely know the sizes of strings,
    // so we set it and use simpler loop for secondaries and tertiaries
    sLen = sIndex; tLen = tIndex;
    if(strength >= UCOL_SECONDARY) {
        // adjust the table beggining
        elements += coll->latinOneTableLen;
        endOfSource = FALSE;

        if(coll->frenchCollation == UCOL_OFF) { // non French
            // This loop is a simplified copy of primary loop
            // at this point we know that whole strings are latin-1, so we don't
            // check for that. We also know that we only have contractions as
            // specials.
            sIndex = 0; tIndex = 0;
            for(;;) {
                while(sOrder==0) {
                    if(sIndex==sLen) {
                        endOfSource = TRUE;
                        break;
                    }
                    sChar=source[sIndex++];
                    sOrder = elements[sChar];
                    if(sOrder > UCOL_NOT_FOUND) {
                        sOrder = ucol_getLatinOneContraction(coll, UCOL_SECONDARY, sOrder, source, &sIndex, sLen);
                    }
                }

                while(tOrder==0) {
                    if(tIndex==tLen) {
                        if(endOfSource) {
                            goto endOfSecLoop;
                        } else {
                            return UCOL_GREATER;
                        }
                    }
                    tChar=target[tIndex++];
                    tOrder = elements[tChar];
                    if(tOrder > UCOL_NOT_FOUND) {
                        tOrder = ucol_getLatinOneContraction(coll, UCOL_SECONDARY, tOrder, target, &tIndex, tLen);
                    }
                }
                if(endOfSource) {
                    return UCOL_LESS;
                }

                if(sOrder == tOrder) {
                    sOrder = 0; tOrder = 0;
                    continue;
                } else {
                    // see primary loop for comments on this
                    if(((sOrder^tOrder)&0xFF000000)!=0) {
                        if(sOrder < tOrder) {
                            return UCOL_LESS;
                        } else if(sOrder > tOrder) {
                            return UCOL_GREATER;
                        }
                    }
                    sOrder<<=8;
                    tOrder<<=8;
                }
            }
        } else { // French
            if(haveContractions) { // if we have contractions, we have to bail out
                // since we don't really know how to handle them here
                return ucol_strcollRegular(coll, source, sLen, target, tLen, status);
            }
            // For French, we go backwards
            sIndex = sLen; tIndex = tLen;
            for(;;) {
                while(sOrder==0) {
                    if(sIndex==0) {
                        endOfSource = TRUE;
                        break;
                    }
                    sChar=source[--sIndex];
                    sOrder = elements[sChar];
                    // don't even look for contractions
                }

                while(tOrder==0) {
                    if(tIndex==0) {
                        if(endOfSource) {
                            goto endOfSecLoop;
                        } else {
                            return UCOL_GREATER;
                        }
                    }
                    tChar=target[--tIndex];
                    tOrder = elements[tChar];
                    // don't even look for contractions
                }
                if(endOfSource) {
                    return UCOL_LESS;
                }

                if(sOrder == tOrder) {
                    sOrder = 0; tOrder = 0;
                    continue;
                } else {
                    // see the primary loop for comments
                    if(((sOrder^tOrder)&0xFF000000)!=0) {
                        if(sOrder < tOrder) {
                            return UCOL_LESS;
                        } else if(sOrder > tOrder) {
                            return UCOL_GREATER;
                        }
                    }
                    sOrder<<=8;
                    tOrder<<=8;
                }
            }
        }
    }

endOfSecLoop:
    if(strength >= UCOL_TERTIARY) {
        // tertiary loop is the same as secondary (except no French)
        elements += coll->latinOneTableLen;
        sIndex = 0; tIndex = 0;
        endOfSource = FALSE;
        for(;;) {
            while(sOrder==0) {
                if(sIndex==sLen) {
                    endOfSource = TRUE;
                    break;
                }
                sChar=source[sIndex++];
                sOrder = elements[sChar];
                if(sOrder > UCOL_NOT_FOUND) {
                    sOrder = ucol_getLatinOneContraction(coll, UCOL_TERTIARY, sOrder, source, &sIndex, sLen);
                }
            }
            while(tOrder==0) {
                if(tIndex==tLen) {
                    if(endOfSource) {
                        return UCOL_EQUAL; // if both strings are at the end, they are equal
                    } else {
                        return UCOL_GREATER;
                    }
                }
                tChar=target[tIndex++];
                tOrder = elements[tChar];
                if(tOrder > UCOL_NOT_FOUND) {
                    tOrder = ucol_getLatinOneContraction(coll, UCOL_TERTIARY, tOrder, target, &tIndex, tLen);
                }
            }
            if(endOfSource) {
                return UCOL_LESS;
            }
            if(sOrder == tOrder) {
                sOrder = 0; tOrder = 0;
                continue;
            } else {
                if(((sOrder^tOrder)&0xff000000)!=0) {
                    if(sOrder < tOrder) {
                        return UCOL_LESS;
                    } else if(sOrder > tOrder) {
                        return UCOL_GREATER;
                    }
                }
                sOrder<<=8;
                tOrder<<=8;
            }
        }
    }
    return UCOL_EQUAL;
}


U_CAPI UCollationResult U_EXPORT2
ucol_strcollIter( const UCollator    *coll,
                 UCharIterator *sIter,
                 UCharIterator *tIter,
                 UErrorCode         *status)
{
    if(!status || U_FAILURE(*status)) {
        return UCOL_EQUAL;
    }

    UTRACE_ENTRY(UTRACE_UCOL_STRCOLLITER);
    UTRACE_DATA3(UTRACE_VERBOSE, "coll=%p, sIter=%p, tIter=%p", coll, sIter, tIter);

    if (sIter == tIter) {
        UTRACE_EXIT_VALUE_STATUS(UCOL_EQUAL, *status)
        return UCOL_EQUAL;
    }
    if(sIter == NULL || tIter == NULL || coll == NULL) {
        *status = U_ILLEGAL_ARGUMENT_ERROR;
        UTRACE_EXIT_VALUE_STATUS(UCOL_EQUAL, *status)
        return UCOL_EQUAL;
    }

    UCollationResult result = UCOL_EQUAL;

    // Preparing the context objects for iterating over strings
    collIterate sColl, tColl;
    IInit_collIterate(coll, NULL, -1, &sColl, status);
    IInit_collIterate(coll, NULL, -1, &tColl, status);
    if(U_FAILURE(*status)) {
        UTRACE_EXIT_VALUE_STATUS(UCOL_EQUAL, *status)
        return UCOL_EQUAL;
    }
    // The division for the array length may truncate the array size to
    // a little less than UNORM_ITER_SIZE, but that size is dimensioned too high
    // for all platforms anyway.
    UAlignedMemory stackNormIter1[UNORM_ITER_SIZE/sizeof(UAlignedMemory)];
    UAlignedMemory stackNormIter2[UNORM_ITER_SIZE/sizeof(UAlignedMemory)];
    UNormIterator *sNormIter = NULL, *tNormIter = NULL;

    sColl.iterator = sIter;
    sColl.flags |= UCOL_USE_ITERATOR;
    tColl.flags |= UCOL_USE_ITERATOR;
    tColl.iterator = tIter;

    if(ucol_getAttribute(coll, UCOL_NORMALIZATION_MODE, status) == UCOL_ON) {
        sNormIter = unorm_openIter(stackNormIter1, sizeof(stackNormIter1), status);
        sColl.iterator = unorm_setIter(sNormIter, sIter, UNORM_FCD, status);
        sColl.flags &= ~UCOL_ITER_NORM;

        tNormIter = unorm_openIter(stackNormIter2, sizeof(stackNormIter2), status);
        tColl.iterator = unorm_setIter(tNormIter, tIter, UNORM_FCD, status);
        tColl.flags &= ~UCOL_ITER_NORM;
    }

    UChar32 sChar = U_SENTINEL, tChar = U_SENTINEL;

    while((sChar = sColl.iterator->next(sColl.iterator)) ==
        (tChar = tColl.iterator->next(tColl.iterator))) {
            if(sChar == U_SENTINEL) {
                result = UCOL_EQUAL;
                goto end_compare;
            }
    }

    if(sChar == U_SENTINEL) {
        tChar = tColl.iterator->previous(tColl.iterator);
    }

    if(tChar == U_SENTINEL) {
        sChar = sColl.iterator->previous(sColl.iterator);
    }

    sChar = sColl.iterator->previous(sColl.iterator);
    tChar = tColl.iterator->previous(tColl.iterator);

    if (ucol_unsafeCP((UChar)sChar, coll) || ucol_unsafeCP((UChar)tChar, coll))
    {
        // We are stopped in the middle of a contraction.
        // Scan backwards through the == part of the string looking for the start of the contraction.
        //   It doesn't matter which string we scan, since they are the same in this region.
        do
        {
            sChar = sColl.iterator->previous(sColl.iterator);
            tChar = tColl.iterator->previous(tColl.iterator);
        }
        while (sChar != U_SENTINEL && ucol_unsafeCP((UChar)sChar, coll));
    }


    if(U_SUCCESS(*status)) {
        result = ucol_strcollRegular(&sColl, &tColl, status);
    }

end_compare:
    if(sNormIter || tNormIter) {
        unorm_closeIter(sNormIter);
        unorm_closeIter(tNormIter);
    }

    UTRACE_EXIT_VALUE_STATUS(result, *status)
    return result;
}


/*                                                                      */
/* ucol_strcoll     Main public API string comparison function          */
/*                                                                      */
U_CAPI UCollationResult U_EXPORT2
ucol_strcoll( const UCollator    *coll,
              const UChar        *source,
              int32_t            sourceLength,
              const UChar        *target,
              int32_t            targetLength)
{
    U_ALIGN_CODE(16);

    UTRACE_ENTRY(UTRACE_UCOL_STRCOLL);
    if (UTRACE_LEVEL(UTRACE_VERBOSE)) {
        UTRACE_DATA3(UTRACE_VERBOSE, "coll=%p, source=%p, target=%p", coll, source, target);
        UTRACE_DATA2(UTRACE_VERBOSE, "source string = %vh ", source, sourceLength);
        UTRACE_DATA2(UTRACE_VERBOSE, "target string = %vh ", target, targetLength);
    }

    if(source == NULL || target == NULL) {
        // do not crash, but return. Should have
        // status argument to return error.
        UTRACE_EXIT_VALUE(UCOL_EQUAL);
        return UCOL_EQUAL;
    }

    /* Quick check if source and target are same strings. */
    /* They should either both be NULL terminated or the explicit length should be set on both. */
    if (source==target && sourceLength==targetLength) {
        UTRACE_EXIT_VALUE(UCOL_EQUAL);
        return UCOL_EQUAL;
    }

    /* Scan the strings.  Find:                                                             */
    /*    The length of any leading portion that is equal                                   */
    /*    Whether they are exactly equal.  (in which case we just return)                   */
    const UChar    *pSrc    = source;
    const UChar    *pTarg   = target;
    int32_t        equalLength;

    if (sourceLength == -1 && targetLength == -1) {
        // Both strings are null terminated.
        //    Scan through any leading equal portion.
        while (*pSrc == *pTarg && *pSrc != 0) {
            pSrc++;
            pTarg++;
        }
        if (*pSrc == 0 && *pTarg == 0) {
            UTRACE_EXIT_VALUE(UCOL_EQUAL);
            return UCOL_EQUAL;
        }
        equalLength = (int32_t)(pSrc - source);
    }
    else
    {
        // One or both strings has an explicit length.
        const UChar    *pSrcEnd = source + sourceLength;
        const UChar    *pTargEnd = target + targetLength;

        // Scan while the strings are bitwise ==, or until one is exhausted.
        for (;;) {
            if (pSrc == pSrcEnd || pTarg == pTargEnd) {
                break;
            }
            if ((*pSrc == 0 && sourceLength == -1) || (*pTarg == 0 && targetLength == -1)) {
                break;
            }
            if (*pSrc != *pTarg) {
                break;
            }
            pSrc++;
            pTarg++;
        }
        equalLength = (int32_t)(pSrc - source);

        // If we made it all the way through both strings, we are done.  They are ==
        if ((pSrc ==pSrcEnd  || (pSrcEnd <pSrc  && *pSrc==0))  &&   /* At end of src string, however it was specified. */
            (pTarg==pTargEnd || (pTargEnd<pTarg && *pTarg==0)))     /* and also at end of dest string                  */
        {
            UTRACE_EXIT_VALUE(UCOL_EQUAL);
            return UCOL_EQUAL;
        }
    }
    if (equalLength > 0) {
        /* There is an identical portion at the beginning of the two strings.        */
        /*   If the identical portion ends within a contraction or a comibining      */
        /*   character sequence, back up to the start of that sequence.              */
        
        // These values should already be set by the code above.
        //pSrc  = source + equalLength;        /* point to the first differing chars   */
        //pTarg = target + equalLength;
        if ((pSrc  != source+sourceLength && ucol_unsafeCP(*pSrc, coll)) ||
            (pTarg != target+targetLength && ucol_unsafeCP(*pTarg, coll)))
        {
            // We are stopped in the middle of a contraction.
            // Scan backwards through the == part of the string looking for the start of the contraction.
            //   It doesn't matter which string we scan, since they are the same in this region.
            do
            {
                equalLength--;
                pSrc--;
            }
            while (equalLength>0 && ucol_unsafeCP(*pSrc, coll));
        }

        source += equalLength;
        target += equalLength;
        if (sourceLength > 0) {
            sourceLength -= equalLength;
        }
        if (targetLength > 0) {
            targetLength -= equalLength;
        }
    }

    UErrorCode status = U_ZERO_ERROR;
    UCollationResult returnVal;
    if(!coll->latinOneUse || (sourceLength > 0 && *source&0xff00) || (targetLength > 0 && *target&0xff00)) {
        returnVal = ucol_strcollRegular(coll, source, sourceLength, target, targetLength, &status);
    } else {
        returnVal = ucol_strcollUseLatin1(coll, source, sourceLength, target, targetLength, &status);
    }
    UTRACE_EXIT_VALUE(returnVal);
    return returnVal;
}

/* convenience function for comparing strings */
U_CAPI UBool U_EXPORT2
ucol_greater(    const    UCollator        *coll,
        const    UChar            *source,
        int32_t            sourceLength,
        const    UChar            *target,
        int32_t            targetLength)
{
    return (ucol_strcoll(coll, source, sourceLength, target, targetLength)
        == UCOL_GREATER);
}

/* convenience function for comparing strings */
U_CAPI UBool U_EXPORT2
ucol_greaterOrEqual(    const    UCollator    *coll,
            const    UChar        *source,
            int32_t        sourceLength,
            const    UChar        *target,
            int32_t        targetLength)
{
    return (ucol_strcoll(coll, source, sourceLength, target, targetLength)
        != UCOL_LESS);
}

/* convenience function for comparing strings */
U_CAPI UBool U_EXPORT2
ucol_equal(        const    UCollator        *coll,
            const    UChar            *source,
            int32_t            sourceLength,
            const    UChar            *target,
            int32_t            targetLength)
{
    return (ucol_strcoll(coll, source, sourceLength, target, targetLength)
        == UCOL_EQUAL);
}

U_CAPI void U_EXPORT2
ucol_getUCAVersion(const UCollator* coll, UVersionInfo info) {
    if(coll && coll->UCA) {
        uprv_memcpy(info, coll->UCA->image->UCAVersion, sizeof(UVersionInfo));
    }
}

#endif /* #if !UCONFIG_NO_COLLATION */
