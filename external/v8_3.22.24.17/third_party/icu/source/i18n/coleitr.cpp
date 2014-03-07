/*
*******************************************************************************
* Copyright (C) 1996-2010, International Business Machines Corporation and    *
* others. All Rights Reserved.                                                *
*******************************************************************************
*/

/*
* File coleitr.cpp
*
* 
*
* Created by: Helena Shih
*
* Modification History:
*
*  Date      Name        Description
*
*  6/23/97   helena      Adding comments to make code more readable.
* 08/03/98   erm         Synched with 1.2 version of CollationElementIterator.java
* 12/10/99   aliu        Ported Thai collation support from Java.
* 01/25/01   swquek      Modified to a C++ wrapper calling C APIs (ucoliter.h)
* 02/19/01   swquek      Removed CollationElementsIterator() since it is 
*                        private constructor and no calls are made to it
*/

#include "unicode/utypes.h"

#if !UCONFIG_NO_COLLATION

#include "unicode/coleitr.h"
#include "unicode/ustring.h"
#include "ucol_imp.h"
#include "cmemory.h"


/* Constants --------------------------------------------------------------- */

U_NAMESPACE_BEGIN

UOBJECT_DEFINE_RTTI_IMPLEMENTATION(CollationElementIterator)

/* CollationElementIterator public constructor/destructor ------------------ */

CollationElementIterator::CollationElementIterator(
                                         const CollationElementIterator& other) 
                                         : UObject(other), isDataOwned_(TRUE)
{
    UErrorCode status = U_ZERO_ERROR;
    m_data_ = ucol_openElements(other.m_data_->iteratordata_.coll, NULL, 0, 
                                &status);

    *this = other;
}

CollationElementIterator::~CollationElementIterator()
{
    if (isDataOwned_) {
        ucol_closeElements(m_data_);
    }
}

/* CollationElementIterator public methods --------------------------------- */

int32_t CollationElementIterator::getOffset() const
{
    return ucol_getOffset(m_data_);
}

/**
* Get the ordering priority of the next character in the string.
* @return the next character's ordering. Returns NULLORDER if an error has 
*         occured or if the end of string has been reached
*/
int32_t CollationElementIterator::next(UErrorCode& status)
{
    return ucol_next(m_data_, &status);
}

UBool CollationElementIterator::operator!=(
                                  const CollationElementIterator& other) const
{
    return !(*this == other);
}

UBool CollationElementIterator::operator==(
                                    const CollationElementIterator& that) const
{
    if (this == &that || m_data_ == that.m_data_) {
        return TRUE;
    }

    // option comparison
    if (m_data_->iteratordata_.coll != that.m_data_->iteratordata_.coll)
    {
        return FALSE;
    }

    // the constructor and setText always sets a length
    // and we only compare the string not the contents of the normalization
    // buffer
    int thislength = (int)(m_data_->iteratordata_.endp - m_data_->iteratordata_.string);
    int thatlength = (int)(that.m_data_->iteratordata_.endp - that.m_data_->iteratordata_.string);
    
    if (thislength != thatlength) {
        return FALSE;
    }

    if (uprv_memcmp(m_data_->iteratordata_.string, 
                    that.m_data_->iteratordata_.string, 
                    thislength * U_SIZEOF_UCHAR) != 0) {
        return FALSE;
    }
    if (getOffset() != that.getOffset()) {
        return FALSE;
    }

    // checking normalization buffer
    if ((m_data_->iteratordata_.flags & UCOL_ITER_HASLEN) == 0) {
        if ((that.m_data_->iteratordata_.flags & UCOL_ITER_HASLEN) != 0) {
            return FALSE;
        }
        // both are in the normalization buffer
        if (m_data_->iteratordata_.pos 
            - m_data_->iteratordata_.writableBuffer.getBuffer()
            != that.m_data_->iteratordata_.pos 
            - that.m_data_->iteratordata_.writableBuffer.getBuffer()) {
            // not in the same position in the normalization buffer
            return FALSE;
        }
    }
    else if ((that.m_data_->iteratordata_.flags & UCOL_ITER_HASLEN) == 0) {
        return FALSE;
    }
    // checking ce position
    return (m_data_->iteratordata_.CEpos - m_data_->iteratordata_.CEs)
            == (that.m_data_->iteratordata_.CEpos 
                                        - that.m_data_->iteratordata_.CEs);
}

/**
* Get the ordering priority of the previous collation element in the string.
* @param status the error code status.
* @return the previous element's ordering. Returns NULLORDER if an error has 
*         occured or if the start of string has been reached.
*/
int32_t CollationElementIterator::previous(UErrorCode& status)
{
    return ucol_previous(m_data_, &status);
}

/**
* Resets the cursor to the beginning of the string.
*/
void CollationElementIterator::reset()
{
    ucol_reset(m_data_);
}

void CollationElementIterator::setOffset(int32_t newOffset, 
                                         UErrorCode& status)
{
    ucol_setOffset(m_data_, newOffset, &status);
}

/**
* Sets the source to the new source string.
*/
void CollationElementIterator::setText(const UnicodeString& source,
                                       UErrorCode& status)
{
    if (U_FAILURE(status)) {
        return;
    }

    int32_t length = source.length();
    UChar *string = NULL;
    if (m_data_->isWritable && m_data_->iteratordata_.string != NULL) {
        uprv_free((UChar *)m_data_->iteratordata_.string);
    }
    m_data_->isWritable = TRUE;
    if (length > 0) {
        string = (UChar *)uprv_malloc(U_SIZEOF_UCHAR * length);
        /* test for NULL */
        if (string == NULL) {
            status = U_MEMORY_ALLOCATION_ERROR;
            return;
        }
        u_memcpy(string, source.getBuffer(), length);
    }
    else {
        string = (UChar *)uprv_malloc(U_SIZEOF_UCHAR);
        /* test for NULL */
        if (string == NULL) {
            status = U_MEMORY_ALLOCATION_ERROR;
            return;
        }
        *string = 0;
    }
    /* Free offsetBuffer before initializing it. */
    ucol_freeOffsetBuffer(&(m_data_->iteratordata_));
    uprv_init_collIterate(m_data_->iteratordata_.coll, string, length, 
        &m_data_->iteratordata_, &status);

    m_data_->reset_   = TRUE;
}

// Sets the source to the new character iterator.
void CollationElementIterator::setText(CharacterIterator& source, 
                                       UErrorCode& status)
{
    if (U_FAILURE(status)) 
        return;

    int32_t length = source.getLength();
    UChar *buffer = NULL;

    if (length == 0) {
        buffer = (UChar *)uprv_malloc(U_SIZEOF_UCHAR);
        /* test for NULL */
        if (buffer == NULL) {
            status = U_MEMORY_ALLOCATION_ERROR;
            return;
        }
        *buffer = 0;
    }
    else {
        buffer = (UChar *)uprv_malloc(U_SIZEOF_UCHAR * length);
        /* test for NULL */
        if (buffer == NULL) {
            status = U_MEMORY_ALLOCATION_ERROR;
            return;
        }
        /* 
        Using this constructor will prevent buffer from being removed when
        string gets removed
        */
        UnicodeString string;
        source.getText(string);
        u_memcpy(buffer, string.getBuffer(), length);
    }

    if (m_data_->isWritable && m_data_->iteratordata_.string != NULL) {
        uprv_free((UChar *)m_data_->iteratordata_.string);
    }
    m_data_->isWritable = TRUE;
    /* Free offsetBuffer before initializing it. */
    ucol_freeOffsetBuffer(&(m_data_->iteratordata_));
    uprv_init_collIterate(m_data_->iteratordata_.coll, buffer, length, 
        &m_data_->iteratordata_, &status);
    m_data_->reset_   = TRUE;
}

int32_t CollationElementIterator::strengthOrder(int32_t order) const
{
    UCollationStrength s = ucol_getStrength(m_data_->iteratordata_.coll);
    // Mask off the unwanted differences.
    if (s == UCOL_PRIMARY) {
        order &= RuleBasedCollator::PRIMARYDIFFERENCEONLY;
    }
    else if (s == UCOL_SECONDARY) {
        order &= RuleBasedCollator::SECONDARYDIFFERENCEONLY;
    }

    return order;
}

/* CollationElementIterator private constructors/destructors --------------- */

/** 
* This is the "real" constructor for this class; it constructs an iterator
* over the source text using the specified collator
*/
CollationElementIterator::CollationElementIterator(
                                               const UnicodeString& sourceText,
                                               const RuleBasedCollator* order,
                                               UErrorCode& status)
                                               : isDataOwned_(TRUE)
{
    if (U_FAILURE(status)) {
        return;
    }

    int32_t length = sourceText.length();
    UChar *string = NULL;

    if (length > 0) {
        string = (UChar *)uprv_malloc(U_SIZEOF_UCHAR * length);
        /* test for NULL */
        if (string == NULL) {
            status = U_MEMORY_ALLOCATION_ERROR;
            return;
        }
        /* 
        Using this constructor will prevent buffer from being removed when
        string gets removed
        */
        u_memcpy(string, sourceText.getBuffer(), length);
    }
    else {
        string = (UChar *)uprv_malloc(U_SIZEOF_UCHAR);
        /* test for NULL */
        if (string == NULL) {
            status = U_MEMORY_ALLOCATION_ERROR;
            return;
        }
        *string = 0;
    }
    m_data_ = ucol_openElements(order->ucollator, string, length, &status);

    /* Test for buffer overflows */
    if (U_FAILURE(status)) {
        return;
    }
    m_data_->isWritable = TRUE;
}

/** 
* This is the "real" constructor for this class; it constructs an iterator over 
* the source text using the specified collator
*/
CollationElementIterator::CollationElementIterator(
                                           const CharacterIterator& sourceText,
                                           const RuleBasedCollator* order,
                                           UErrorCode& status)
                                           : isDataOwned_(TRUE)
{
    if (U_FAILURE(status))
        return;

    // **** should I just drop this test? ****
    /*
    if ( sourceText.endIndex() != 0 )
    {
        // A CollationElementIterator is really a two-layered beast.
        // Internally it uses a Normalizer to munge the source text into a form 
        // where all "composed" Unicode characters (such as \u00FC) are split into a 
        // normal character and a combining accent character.  
        // Afterward, CollationElementIterator does its own processing to handle
        // expanding and contracting collation sequences, ignorables, and so on.
        
        Normalizer::EMode decomp = order->getStrength() == Collator::IDENTICAL
                                ? Normalizer::NO_OP : order->getDecomposition();
          
        text = new Normalizer(sourceText, decomp);
        if (text == NULL)
        status = U_MEMORY_ALLOCATION_ERROR;    
    }
    */
    int32_t length = sourceText.getLength();
    UChar *buffer;
    if (length > 0) {
        buffer = (UChar *)uprv_malloc(U_SIZEOF_UCHAR * length);
        /* test for NULL */
        if (buffer == NULL) {
            status = U_MEMORY_ALLOCATION_ERROR;
            return;
        }
        /* 
        Using this constructor will prevent buffer from being removed when
        string gets removed
        */
        UnicodeString string(buffer, length, length);
        ((CharacterIterator &)sourceText).getText(string);
        const UChar *temp = string.getBuffer();
        u_memcpy(buffer, temp, length);
    }
    else {
        buffer = (UChar *)uprv_malloc(U_SIZEOF_UCHAR);
        /* test for NULL */
        if (buffer == NULL) {
            status = U_MEMORY_ALLOCATION_ERROR;
            return;
        }
        *buffer = 0;
    }
    m_data_ = ucol_openElements(order->ucollator, buffer, length, &status);

    /* Test for buffer overflows */
    if (U_FAILURE(status)) {
        return;
    }
    m_data_->isWritable = TRUE;
}

/* CollationElementIterator protected methods ----------------------------- */

const CollationElementIterator& CollationElementIterator::operator=(
                                         const CollationElementIterator& other)
{
    if (this != &other)
    {
        UCollationElements *ucolelem      = this->m_data_;
        UCollationElements *otherucolelem = other.m_data_;
        collIterate        *coliter       = &(ucolelem->iteratordata_);
        collIterate        *othercoliter  = &(otherucolelem->iteratordata_);
        int                length         = 0;

        // checking only UCOL_ITER_HASLEN is not enough here as we may be in 
        // the normalization buffer
        length = (int)(othercoliter->endp - othercoliter->string);

        ucolelem->reset_         = otherucolelem->reset_;
        ucolelem->isWritable     = TRUE;

        /* create a duplicate of string */
        if (length > 0) {
            coliter->string = (UChar *)uprv_malloc(length * U_SIZEOF_UCHAR);
            if(coliter->string != NULL) {
                uprv_memcpy((UChar *)coliter->string, othercoliter->string,
                    length * U_SIZEOF_UCHAR);
            } else { // Error: couldn't allocate memory. No copying should be done
                length = 0;
            }
        }
        else {
            coliter->string = NULL;
        }

        /* start and end of string */
        coliter->endp = coliter->string + length;

        /* handle writable buffer here */

        if (othercoliter->flags & UCOL_ITER_INNORMBUF) {
            coliter->writableBuffer = othercoliter->writableBuffer;
            coliter->writableBuffer.getTerminatedBuffer();
        }

        /* current position */
        if (othercoliter->pos >= othercoliter->string && 
            othercoliter->pos <= othercoliter->endp)
        {
            coliter->pos = coliter->string + 
                (othercoliter->pos - othercoliter->string);
        }
        else {
            coliter->pos = coliter->writableBuffer.getTerminatedBuffer() + 
                (othercoliter->pos - othercoliter->writableBuffer.getBuffer());
        }

        /* CE buffer */
        int32_t CEsize;
        if (coliter->extendCEs) {
            uprv_memcpy(coliter->CEs, othercoliter->CEs, sizeof(uint32_t) * UCOL_EXPAND_CE_BUFFER_SIZE);
            CEsize = sizeof(othercoliter->extendCEs);
            if (CEsize > 0) {
                othercoliter->extendCEs = (uint32_t *)uprv_malloc(CEsize);
                uprv_memcpy(coliter->extendCEs, othercoliter->extendCEs, CEsize);
            }
            coliter->toReturn = coliter->extendCEs + 
                (othercoliter->toReturn - othercoliter->extendCEs);
            coliter->CEpos    = coliter->extendCEs + CEsize;
        } else {
            CEsize = (int32_t)(othercoliter->CEpos - othercoliter->CEs);
            if (CEsize > 0) {
                uprv_memcpy(coliter->CEs, othercoliter->CEs, CEsize);
            }
            coliter->toReturn = coliter->CEs + 
                (othercoliter->toReturn - othercoliter->CEs);
            coliter->CEpos    = coliter->CEs + CEsize;
        }

        if (othercoliter->fcdPosition != NULL) {
            coliter->fcdPosition = coliter->string + 
                (othercoliter->fcdPosition 
                - othercoliter->string);
        }
        else {
            coliter->fcdPosition = NULL;
        }
        coliter->flags       = othercoliter->flags/*| UCOL_ITER_HASLEN*/;
        coliter->origFlags   = othercoliter->origFlags;
        coliter->coll = othercoliter->coll;
        this->isDataOwned_ = TRUE;
    }

    return *this;
}

U_NAMESPACE_END

#endif /* #if !UCONFIG_NO_COLLATION */

/* eof */
