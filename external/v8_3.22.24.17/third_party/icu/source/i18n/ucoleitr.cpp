/*
******************************************************************************
*   Copyright (C) 2001-2010, International Business Machines
*   Corporation and others.  All Rights Reserved.
******************************************************************************
*
* File ucoleitr.cpp
*
* Modification History:
*
* Date        Name        Description
* 02/15/2001  synwee      Modified all methods to process its own function 
*                         instead of calling the equivalent c++ api (coleitr.h)
******************************************************************************/

#include "unicode/utypes.h"

#if !UCONFIG_NO_COLLATION

#include "unicode/ucoleitr.h"
#include "unicode/ustring.h"
#include "unicode/sortkey.h"
#include "unicode/uobject.h"
#include "ucol_imp.h"
#include "cmemory.h"

U_NAMESPACE_USE

#define BUFFER_LENGTH             100

#define DEFAULT_BUFFER_SIZE 16
#define BUFFER_GROW 8

#define ARRAY_SIZE(array) (sizeof array / sizeof array[0])

#define ARRAY_COPY(dst, src, count) uprv_memcpy((void *) (dst), (void *) (src), (count) * sizeof (src)[0])

#define NEW_ARRAY(type, count) (type *) uprv_malloc((count) * sizeof(type))

#define GROW_ARRAY(array, newSize) uprv_realloc((void *) (array), (newSize) * sizeof (array)[0])

#define DELETE_ARRAY(array) uprv_free((void *) (array))

typedef struct collIterate collIterator;

struct RCEI
{
    uint32_t ce;
    int32_t  low;
    int32_t  high;
};

U_NAMESPACE_BEGIN

struct RCEBuffer
{
    RCEI    defaultBuffer[DEFAULT_BUFFER_SIZE];
    RCEI   *buffer;
    int32_t bufferIndex;
    int32_t bufferSize;

    RCEBuffer();
    ~RCEBuffer();

    UBool empty() const;
    void  put(uint32_t ce, int32_t ixLow, int32_t ixHigh);
    const RCEI *get();
};

RCEBuffer::RCEBuffer()
{
    buffer = defaultBuffer;
    bufferIndex = 0;
    bufferSize = DEFAULT_BUFFER_SIZE;
}

RCEBuffer::~RCEBuffer()
{
    if (buffer != defaultBuffer) {
        DELETE_ARRAY(buffer);
    }
}

UBool RCEBuffer::empty() const
{
    return bufferIndex <= 0;
}

void RCEBuffer::put(uint32_t ce, int32_t ixLow, int32_t ixHigh)
{
    if (bufferIndex >= bufferSize) {
        RCEI *newBuffer = NEW_ARRAY(RCEI, bufferSize + BUFFER_GROW);

        ARRAY_COPY(newBuffer, buffer, bufferSize);

        if (buffer != defaultBuffer) {
            DELETE_ARRAY(buffer);
        }

        buffer = newBuffer;
        bufferSize += BUFFER_GROW;
    }

    buffer[bufferIndex].ce   = ce;
    buffer[bufferIndex].low  = ixLow;
    buffer[bufferIndex].high = ixHigh;

    bufferIndex += 1;
}

const RCEI *RCEBuffer::get()
{
    if (bufferIndex > 0) {
     return &buffer[--bufferIndex];
    }

    return NULL;
}

struct PCEI
{
    uint64_t ce;
    int32_t  low;
    int32_t  high;
};

struct PCEBuffer
{
    PCEI    defaultBuffer[DEFAULT_BUFFER_SIZE];
    PCEI   *buffer;
    int32_t bufferIndex;
    int32_t bufferSize;

    PCEBuffer();
    ~PCEBuffer();

    void  reset();
    UBool empty() const;
    void  put(uint64_t ce, int32_t ixLow, int32_t ixHigh);
    const PCEI *get();
};

PCEBuffer::PCEBuffer()
{
    buffer = defaultBuffer;
    bufferIndex = 0;
    bufferSize = DEFAULT_BUFFER_SIZE;
}

PCEBuffer::~PCEBuffer()
{
    if (buffer != defaultBuffer) {
        DELETE_ARRAY(buffer);
    }
}

void PCEBuffer::reset()
{
    bufferIndex = 0;
}

UBool PCEBuffer::empty() const
{
    return bufferIndex <= 0;
}

void PCEBuffer::put(uint64_t ce, int32_t ixLow, int32_t ixHigh)
{
    if (bufferIndex >= bufferSize) {
        PCEI *newBuffer = NEW_ARRAY(PCEI, bufferSize + BUFFER_GROW);

        ARRAY_COPY(newBuffer, buffer, bufferSize);

        if (buffer != defaultBuffer) {
            DELETE_ARRAY(buffer);
        }

        buffer = newBuffer;
        bufferSize += BUFFER_GROW;
    }

    buffer[bufferIndex].ce   = ce;
    buffer[bufferIndex].low  = ixLow;
    buffer[bufferIndex].high = ixHigh;

    bufferIndex += 1;
}

const PCEI *PCEBuffer::get()
{
    if (bufferIndex > 0) {
     return &buffer[--bufferIndex];
    }

    return NULL;
}

/*
 * This inherits from UObject so that
 * it can be allocated by new and the
 * constructor for PCEBuffer is called.
 */
struct UCollationPCE : public UObject
{
    PCEBuffer          pceBuffer;
    UCollationStrength strength;
    UBool              toShift;
    UBool              isShifted;
    uint32_t           variableTop;

    UCollationPCE(UCollationElements *elems);
    ~UCollationPCE();

    void init(const UCollator *coll);

    virtual UClassID getDynamicClassID() const;
    static UClassID getStaticClassID();
};

UOBJECT_DEFINE_RTTI_IMPLEMENTATION(UCollationPCE)

UCollationPCE::UCollationPCE(UCollationElements *elems)
{
    init(elems->iteratordata_.coll);
}

void UCollationPCE::init(const UCollator *coll)
{
    UErrorCode status = U_ZERO_ERROR;

    strength    = ucol_getStrength(coll);
    toShift     = ucol_getAttribute(coll, UCOL_ALTERNATE_HANDLING, &status) == UCOL_SHIFTED;
    isShifted   = FALSE;
    variableTop = coll->variableTopValue << 16;
}

UCollationPCE::~UCollationPCE()
{
    // nothing to do
}


U_NAMESPACE_END


inline uint64_t processCE(UCollationElements *elems, uint32_t ce)
{
    uint64_t primary = 0, secondary = 0, tertiary = 0, quaternary = 0;

    // This is clean, but somewhat slow...
    // We could apply the mask to ce and then
    // just get all three orders...
    switch(elems->pce->strength) {
    default:
        tertiary = ucol_tertiaryOrder(ce);
        /* note fall-through */

    case UCOL_SECONDARY:
        secondary = ucol_secondaryOrder(ce);
        /* note fall-through */

    case UCOL_PRIMARY:
        primary = ucol_primaryOrder(ce);
    }

    // **** This should probably handle continuations too.  ****
    // **** That means that we need 24 bits for the primary ****
    // **** instead of the 16 that we're currently using.   ****
    // **** So we can lay out the 64 bits as: 24.12.12.16.  ****
    // **** Another complication with continuations is that ****
    // **** the *second* CE is marked as a continuation, so ****
    // **** we always have to peek ahead to know how long   ****
    // **** the primary is...                               ****
    if ((elems->pce->toShift && elems->pce->variableTop > ce && primary != 0)
                || (elems->pce->isShifted && primary == 0)) {

        if (primary == 0) {
            return UCOL_IGNORABLE;
        }

        if (elems->pce->strength >= UCOL_QUATERNARY) {
            quaternary = primary;
        }

        primary = secondary = tertiary = 0;
        elems->pce->isShifted = TRUE;
    } else {
        if (elems->pce->strength >= UCOL_QUATERNARY) {
            quaternary = 0xFFFF;
        }

        elems->pce->isShifted = FALSE;
    }

    return primary << 48 | secondary << 32 | tertiary << 16 | quaternary;
}

U_CAPI void U_EXPORT2
uprv_init_pce(const UCollationElements *elems)
{
    if (elems->pce != NULL) {
        elems->pce->init(elems->iteratordata_.coll);
    }
}



/* public methods ---------------------------------------------------- */

U_CAPI UCollationElements* U_EXPORT2
ucol_openElements(const UCollator  *coll,
                  const UChar      *text,
                        int32_t    textLength,
                        UErrorCode *status)
{
    if (U_FAILURE(*status)) {
        return NULL;
    }

    UCollationElements *result = new UCollationElements;
    if (result == NULL) {
        *status = U_MEMORY_ALLOCATION_ERROR;
        return NULL;
    }

    result->reset_ = TRUE;
    result->isWritable = FALSE;
    result->pce = NULL;

    if (text == NULL) {
        textLength = 0;
    }
    uprv_init_collIterate(coll, text, textLength, &result->iteratordata_, status);

    return result;
}


U_CAPI void U_EXPORT2
ucol_closeElements(UCollationElements *elems)
{
	if (elems != NULL) {
	  collIterate *ci = &elems->iteratordata_;

	  if (ci->extendCEs) {
		  uprv_free(ci->extendCEs);
	  }

	  if (ci->offsetBuffer) {
		  uprv_free(ci->offsetBuffer);
	  }

	  if (elems->isWritable && elems->iteratordata_.string != NULL)
	  {
		uprv_free((UChar *)elems->iteratordata_.string);
	  }

	  if (elems->pce != NULL) {
		  delete elems->pce;
	  }

	  delete elems;
	}
}

U_CAPI void U_EXPORT2
ucol_reset(UCollationElements *elems)
{
    collIterate *ci = &(elems->iteratordata_);
    elems->reset_   = TRUE;
    ci->pos         = ci->string;
    if ((ci->flags & UCOL_ITER_HASLEN) == 0 || ci->endp == NULL) {
        ci->endp      = ci->string + u_strlen(ci->string);
    }
    ci->CEpos       = ci->toReturn = ci->CEs;
    ci->flags       = (ci->flags & UCOL_FORCE_HAN_IMPLICIT) | UCOL_ITER_HASLEN;
    if (ci->coll->normalizationMode == UCOL_ON) {
        ci->flags |= UCOL_ITER_NORM;
    }

    ci->writableBuffer.remove();
    ci->fcdPosition = NULL;

  //ci->offsetReturn = ci->offsetStore = NULL;
	ci->offsetRepeatCount = ci->offsetRepeatValue = 0;
}

U_CAPI void U_EXPORT2
ucol_forceHanImplicit(UCollationElements *elems, UErrorCode *status)
{
    if (U_FAILURE(*status)) {
        return;
    }

    if (elems == NULL) {
        *status = U_ILLEGAL_ARGUMENT_ERROR;
        return;
    }

    elems->iteratordata_.flags |= UCOL_FORCE_HAN_IMPLICIT;
}

U_CAPI int32_t U_EXPORT2
ucol_next(UCollationElements *elems, 
          UErrorCode         *status)
{
    int32_t result;
    if (U_FAILURE(*status)) {
        return UCOL_NULLORDER;
    }

    elems->reset_ = FALSE;

    result = (int32_t)ucol_getNextCE(elems->iteratordata_.coll,
                                     &elems->iteratordata_, 
                                     status);

    if (result == UCOL_NO_MORE_CES) {
        result = UCOL_NULLORDER;
    }
    return result;
}

U_CAPI int64_t U_EXPORT2
ucol_nextProcessed(UCollationElements *elems,
                   int32_t            *ixLow,
                   int32_t            *ixHigh,
                   UErrorCode         *status)
{
    const UCollator *coll = elems->iteratordata_.coll;
    int64_t result = UCOL_IGNORABLE;
    uint32_t low = 0, high = 0;

    if (U_FAILURE(*status)) {
        return UCOL_PROCESSED_NULLORDER;
    }

    if (elems->pce == NULL) {
        elems->pce = new UCollationPCE(elems);
    } else {
        elems->pce->pceBuffer.reset();
    }

    elems->reset_ = FALSE;

    do {
        low = ucol_getOffset(elems);
        uint32_t ce = (uint32_t) ucol_getNextCE(coll, &elems->iteratordata_, status);
        high = ucol_getOffset(elems);

        if (ce == UCOL_NO_MORE_CES) {
             result = UCOL_PROCESSED_NULLORDER;
             break;
        }

        result = processCE(elems, ce);
    } while (result == UCOL_IGNORABLE);

    if (ixLow != NULL) {
        *ixLow = low;
    }

    if (ixHigh != NULL) {
        *ixHigh = high;
    }

    return result;
}

U_CAPI int32_t U_EXPORT2
ucol_previous(UCollationElements *elems,
              UErrorCode         *status)
{
    if(U_FAILURE(*status)) {
        return UCOL_NULLORDER;
    }
    else
    {
        int32_t result;

        if (elems->reset_ && (elems->iteratordata_.pos == elems->iteratordata_.string)) {
            if (elems->iteratordata_.endp == NULL) {
                elems->iteratordata_.endp = elems->iteratordata_.string + 
                                            u_strlen(elems->iteratordata_.string);
                elems->iteratordata_.flags |= UCOL_ITER_HASLEN;
            }
            elems->iteratordata_.pos = elems->iteratordata_.endp;
            elems->iteratordata_.fcdPosition = elems->iteratordata_.endp;
        }

        elems->reset_ = FALSE;

        result = (int32_t)ucol_getPrevCE(elems->iteratordata_.coll,
                                         &(elems->iteratordata_), 
                                         status);

        if (result == UCOL_NO_MORE_CES) {
            result = UCOL_NULLORDER;
        }

        return result;
    }
}

U_CAPI int64_t U_EXPORT2
ucol_previousProcessed(UCollationElements *elems,
                   int32_t            *ixLow,
                   int32_t            *ixHigh,
                   UErrorCode         *status)
{
    const UCollator *coll = elems->iteratordata_.coll;
    int64_t result = UCOL_IGNORABLE;
 // int64_t primary = 0, secondary = 0, tertiary = 0, quaternary = 0;
 // UCollationStrength strength = ucol_getStrength(coll);
 //  UBool toShift   = ucol_getAttribute(coll, UCOL_ALTERNATE_HANDLING, status) ==  UCOL_SHIFTED;
 // uint32_t variableTop = coll->variableTopValue;
    int32_t  low = 0, high = 0;

    if (U_FAILURE(*status)) {
        return UCOL_PROCESSED_NULLORDER;
    }

    if (elems->reset_ && 
        (elems->iteratordata_.pos == elems->iteratordata_.string)) {
        if (elems->iteratordata_.endp == NULL) {
            elems->iteratordata_.endp = elems->iteratordata_.string + 
                                        u_strlen(elems->iteratordata_.string);
            elems->iteratordata_.flags |= UCOL_ITER_HASLEN;
        }

        elems->iteratordata_.pos = elems->iteratordata_.endp;
        elems->iteratordata_.fcdPosition = elems->iteratordata_.endp;
    }

    if (elems->pce == NULL) {
        elems->pce = new UCollationPCE(elems);
    } else {
      //elems->pce->pceBuffer.reset();
    }

    elems->reset_ = FALSE;

    while (elems->pce->pceBuffer.empty()) {
        // buffer raw CEs up to non-ignorable primary
        RCEBuffer rceb;
        uint32_t ce;
        
        // **** do we need to reset rceb, or will it always be empty at this point ****
        do {
            high = ucol_getOffset(elems);
            ce   = ucol_getPrevCE(coll, &elems->iteratordata_, status);
            low  = ucol_getOffset(elems);

            if (ce == UCOL_NO_MORE_CES) {
                if (! rceb.empty()) {
                    break;
                }

                goto finish;
            }

            rceb.put(ce, low, high);
        } while ((ce & UCOL_PRIMARYMASK) == 0);

        // process the raw CEs
        while (! rceb.empty()) {
            const RCEI *rcei = rceb.get();

            result = processCE(elems, rcei->ce);

            if (result != UCOL_IGNORABLE) {
                elems->pce->pceBuffer.put(result, rcei->low, rcei->high);
            }
        }
    }

finish:
    if (elems->pce->pceBuffer.empty()) {
        // **** Is -1 the right value for ixLow, ixHigh? ****
    	if (ixLow != NULL) {
    		*ixLow = -1;
    	}
    	
    	if (ixHigh != NULL) {
    		*ixHigh = -1
    		;
    	}
        return UCOL_PROCESSED_NULLORDER;
    }

    const PCEI *pcei = elems->pce->pceBuffer.get();

    if (ixLow != NULL) {
        *ixLow = pcei->low;
    }

    if (ixHigh != NULL) {
        *ixHigh = pcei->high;
    }

    return pcei->ce;
}

U_CAPI int32_t U_EXPORT2
ucol_getMaxExpansion(const UCollationElements *elems,
                           int32_t            order)
{
    uint8_t result;

#if 0
    UCOL_GETMAXEXPANSION(elems->iteratordata_.coll, (uint32_t)order, result);
#else
    const UCollator *coll = elems->iteratordata_.coll;
    const uint32_t *start;
    const uint32_t *limit;
    const uint32_t *mid;
          uint32_t strengthMask = 0;
          uint32_t mOrder = (uint32_t) order;

    switch (coll->strength) 
    {
    default:
        strengthMask |= UCOL_TERTIARYORDERMASK;
        /* fall through */

    case UCOL_SECONDARY:
        strengthMask |= UCOL_SECONDARYORDERMASK;
        /* fall through */

    case UCOL_PRIMARY:
        strengthMask |= UCOL_PRIMARYORDERMASK;
    }

    mOrder &= strengthMask;
    start = (coll)->endExpansionCE;
    limit = (coll)->lastEndExpansionCE;

    while (start < limit - 1) {
        mid = start + ((limit - start) >> 1);
        if (mOrder <= (*mid & strengthMask)) {
          limit = mid;
        } else {
          start = mid;
        }
    }

    // FIXME: with a masked search, there might be more than one hit,
    // so we need to look forward and backward from the match to find all
    // of the hits...
    if ((*start & strengthMask) == mOrder) {
        result = *((coll)->expansionCESize + (start - (coll)->endExpansionCE));
    } else if ((*limit & strengthMask) == mOrder) {
         result = *(coll->expansionCESize + (limit - coll->endExpansionCE));
   } else if ((mOrder & 0xFFFF) == 0x00C0) {
        result = 2;
   } else {
       result = 1;
   }
#endif

    return result;
}
 
U_CAPI void U_EXPORT2
ucol_setText(      UCollationElements *elems,
             const UChar              *text,
                   int32_t            textLength,
                   UErrorCode         *status)
{
    if (U_FAILURE(*status)) {
        return;
    }

    if (elems->isWritable && elems->iteratordata_.string != NULL)
    {
        uprv_free((UChar *)elems->iteratordata_.string);
    }

    if (text == NULL) {
        textLength = 0;
    }

    elems->isWritable = FALSE;
    
    /* free offset buffer to avoid memory leak before initializing. */
    ucol_freeOffsetBuffer(&(elems->iteratordata_));
    /* Ensure that previously allocated extendCEs is freed before setting to NULL. */
    if (elems->iteratordata_.extendCEs != NULL) {
        uprv_free(elems->iteratordata_.extendCEs);
    }
    uprv_init_collIterate(elems->iteratordata_.coll, text, textLength, 
                          &elems->iteratordata_, status);

    elems->reset_   = TRUE;
}

U_CAPI int32_t U_EXPORT2
ucol_getOffset(const UCollationElements *elems)
{
  const collIterate *ci = &(elems->iteratordata_);

  if (ci->offsetRepeatCount > 0 && ci->offsetRepeatValue != 0) {
      return ci->offsetRepeatValue;
  }

  if (ci->offsetReturn != NULL) {
      return *ci->offsetReturn;
  }

  // while processing characters in normalization buffer getOffset will 
  // return the next non-normalized character. 
  // should be inline with the old implementation since the old codes uses
  // nextDecomp in normalizer which also decomposes the string till the 
  // first base character is found.
  if (ci->flags & UCOL_ITER_INNORMBUF) {
      if (ci->fcdPosition == NULL) {
        return 0;
      }
      return (int32_t)(ci->fcdPosition - ci->string);
  }
  else {
      return (int32_t)(ci->pos - ci->string);
  }
}

U_CAPI void U_EXPORT2
ucol_setOffset(UCollationElements    *elems,
               int32_t           offset,
               UErrorCode            *status)
{
    if (U_FAILURE(*status)) {
        return;
    }

    // this methods will clean up any use of the writable buffer and points to 
    // the original string
    collIterate *ci = &(elems->iteratordata_);
    ci->pos         = ci->string + offset;
    ci->CEpos       = ci->toReturn = ci->CEs;
    if (ci->flags & UCOL_ITER_INNORMBUF) {
        ci->flags = ci->origFlags;
    }
    if ((ci->flags & UCOL_ITER_HASLEN) == 0) {
        ci->endp  = ci->string + u_strlen(ci->string);
        ci->flags |= UCOL_ITER_HASLEN;
    }
    ci->fcdPosition = NULL;
    elems->reset_ = FALSE;

	ci->offsetReturn = NULL;
    ci->offsetStore = ci->offsetBuffer;
	ci->offsetRepeatCount = ci->offsetRepeatValue = 0;
}

U_CAPI int32_t U_EXPORT2
ucol_primaryOrder (int32_t order) 
{
    order &= UCOL_PRIMARYMASK;
    return (order >> UCOL_PRIMARYORDERSHIFT);
}

U_CAPI int32_t U_EXPORT2
ucol_secondaryOrder (int32_t order) 
{
    order &= UCOL_SECONDARYMASK;
    return (order >> UCOL_SECONDARYORDERSHIFT);
}

U_CAPI int32_t U_EXPORT2
ucol_tertiaryOrder (int32_t order) 
{
    return (order & UCOL_TERTIARYMASK);
}


void ucol_freeOffsetBuffer(collIterate *s) {
    if (s != NULL && s->offsetBuffer != NULL) {
        uprv_free(s->offsetBuffer);
        s->offsetBuffer = NULL;
        s->offsetBufferSize = 0;
    }
}

#endif /* #if !UCONFIG_NO_COLLATION */
