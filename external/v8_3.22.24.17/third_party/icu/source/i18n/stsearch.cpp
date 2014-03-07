/*
**********************************************************************
*   Copyright (C) 2001-2008 IBM and others. All rights reserved.
**********************************************************************
*   Date        Name        Description
*  03/22/2000   helena      Creation.
**********************************************************************
*/

#include "unicode/utypes.h"

#if !UCONFIG_NO_COLLATION && !UCONFIG_NO_BREAK_ITERATION

#include "unicode/stsearch.h"
#include "usrchimp.h"
#include "cmemory.h"

U_NAMESPACE_BEGIN

UOBJECT_DEFINE_RTTI_IMPLEMENTATION(StringSearch)

// public constructors and destructors -----------------------------------

StringSearch::StringSearch(const UnicodeString &pattern,
                           const UnicodeString &text,
                           const Locale        &locale,
                                 BreakIterator *breakiter,
                                 UErrorCode    &status) :
                           SearchIterator(text, breakiter),
                           m_collator_(),
                           m_pattern_(pattern)
{
    if (U_FAILURE(status)) {
        m_strsrch_ = NULL;
        return;
    }

    m_strsrch_ = usearch_open(m_pattern_.getBuffer(), m_pattern_.length(),
                              m_text_.getBuffer(), m_text_.length(),
                              locale.getName(), (UBreakIterator *)breakiter,
                              &status);
    uprv_free(m_search_);
    m_search_ = NULL;

    // !!! dlf m_collator_ is an odd beast.  basically it is an aliasing
    // wrapper around the internal collator and rules, which (here) are
    // owned by this stringsearch object.  this means 1) it's destructor
    // _should not_ delete the ucollator or rules, and 2) changes made
    // to the exposed collator (setStrength etc) _should_ modify the
    // ucollator.  thus the collator is not a copy-on-write alias, and it
    // needs to distinguish itself not merely from 'stand alone' colators
    // but also from copy-on-write ones.  it needs additional state, which
    // setUCollator should set.

    if (U_SUCCESS(status)) {
        // Alias the collator
        m_collator_.setUCollator((UCollator *)m_strsrch_->collator);
        // m_search_ has been created by the base SearchIterator class
        m_search_        = m_strsrch_->search;
    }
}

StringSearch::StringSearch(const UnicodeString     &pattern,
                           const UnicodeString     &text,
                                 RuleBasedCollator *coll,
                                 BreakIterator     *breakiter,
                                 UErrorCode        &status) :
                           SearchIterator(text, breakiter),
                           m_collator_(),
                           m_pattern_(pattern)
{
    if (U_FAILURE(status)) {
        m_strsrch_ = NULL;
        return;
    }
    if (coll == NULL) {
        status     = U_ILLEGAL_ARGUMENT_ERROR;
        m_strsrch_ = NULL;
        return;
    }
    m_strsrch_ = usearch_openFromCollator(m_pattern_.getBuffer(),
                                          m_pattern_.length(),
                                          m_text_.getBuffer(),
                                          m_text_.length(), coll->ucollator,
                                          (UBreakIterator *)breakiter,
                                          &status);
    uprv_free(m_search_);
    m_search_ = NULL;

    if (U_SUCCESS(status)) {
        // Alias the collator
        m_collator_.setUCollator((UCollator *)m_strsrch_->collator);
        // m_search_ has been created by the base SearchIterator class
        m_search_ = m_strsrch_->search;
    }
}

StringSearch::StringSearch(const UnicodeString     &pattern,
                                 CharacterIterator &text,
                           const Locale            &locale,
                                 BreakIterator     *breakiter,
                                 UErrorCode        &status) :
                           SearchIterator(text, breakiter),
                           m_collator_(),
                           m_pattern_(pattern)
{
    if (U_FAILURE(status)) {
        m_strsrch_ = NULL;
        return;
    }
    m_strsrch_ = usearch_open(m_pattern_.getBuffer(), m_pattern_.length(),
                              m_text_.getBuffer(), m_text_.length(),
                              locale.getName(), (UBreakIterator *)breakiter,
                              &status);
    uprv_free(m_search_);
    m_search_ = NULL;

    if (U_SUCCESS(status)) {
        // Alias the collator
        m_collator_.setUCollator((UCollator *)m_strsrch_->collator);
        // m_search_ has been created by the base SearchIterator class
        m_search_ = m_strsrch_->search;
    }
}

StringSearch::StringSearch(const UnicodeString     &pattern,
                                 CharacterIterator &text,
                                 RuleBasedCollator *coll,
                                 BreakIterator     *breakiter,
                                 UErrorCode        &status) :
                           SearchIterator(text, breakiter),
                           m_collator_(),
                           m_pattern_(pattern)
{
    if (U_FAILURE(status)) {
        m_strsrch_ = NULL;
        return;
    }
    if (coll == NULL) {
        status     = U_ILLEGAL_ARGUMENT_ERROR;
        m_strsrch_ = NULL;
        return;
    }
    m_strsrch_ = usearch_openFromCollator(m_pattern_.getBuffer(),
                                          m_pattern_.length(),
                                          m_text_.getBuffer(),
                                          m_text_.length(), coll->ucollator,
                                          (UBreakIterator *)breakiter,
                                          &status);
    uprv_free(m_search_);
    m_search_ = NULL;

    if (U_SUCCESS(status)) {
        // Alias the collator
        m_collator_.setUCollator((UCollator *)m_strsrch_->collator);
        // m_search_ has been created by the base SearchIterator class
        m_search_ = m_strsrch_->search;
    }
}

StringSearch::StringSearch(const StringSearch &that) :
                       SearchIterator(that.m_text_, that.m_breakiterator_),
                       m_collator_(),
                       m_pattern_(that.m_pattern_)
{
    UErrorCode status = U_ZERO_ERROR;

    // Free m_search_ from the superclass
    uprv_free(m_search_);
    m_search_ = NULL;

    if (that.m_strsrch_ == NULL) {
        // This was not a good copy
        m_strsrch_ = NULL;
    }
    else {
        // Make a deep copy
        m_strsrch_ = usearch_openFromCollator(m_pattern_.getBuffer(),
                                              m_pattern_.length(),
                                              m_text_.getBuffer(),
                                              m_text_.length(),
                                              that.m_strsrch_->collator,
                                             (UBreakIterator *)that.m_breakiterator_,
                                              &status);
        if (U_SUCCESS(status)) {
            // Alias the collator
            m_collator_.setUCollator((UCollator *)m_strsrch_->collator);
            // m_search_ has been created by the base SearchIterator class
            m_search_        = m_strsrch_->search;
        }
    }
}

StringSearch::~StringSearch()
{
    if (m_strsrch_ != NULL) {
        usearch_close(m_strsrch_);
        m_search_ = NULL;
    }
}

StringSearch *
StringSearch::clone() const {
    return new StringSearch(*this);
}

// operator overloading ---------------------------------------------
StringSearch & StringSearch::operator=(const StringSearch &that)
{
    if ((*this) != that) {
        UErrorCode status = U_ZERO_ERROR;
        m_text_          = that.m_text_;
        m_breakiterator_ = that.m_breakiterator_;
        m_pattern_       = that.m_pattern_;
        // all m_search_ in the parent class is linked up with m_strsrch_
        usearch_close(m_strsrch_);
        m_strsrch_ = usearch_openFromCollator(m_pattern_.getBuffer(),
                                              m_pattern_.length(),
                                              m_text_.getBuffer(),
                                              m_text_.length(),
                                              that.m_strsrch_->collator,
                                              NULL, &status);
        // Check null pointer
        if (m_strsrch_ != NULL) {
	        // Alias the collator
	        m_collator_.setUCollator((UCollator *)m_strsrch_->collator);
	        m_search_ = m_strsrch_->search;
        }
    }
    return *this;
}

UBool StringSearch::operator==(const SearchIterator &that) const
{
    if (this == &that) {
        return TRUE;
    }
    if (SearchIterator::operator ==(that)) {
        StringSearch &thatsrch = (StringSearch &)that;
        return (this->m_pattern_ == thatsrch.m_pattern_ &&
                this->m_strsrch_->collator == thatsrch.m_strsrch_->collator);
    }
    return FALSE;
}

// public get and set methods ----------------------------------------

void StringSearch::setOffset(int32_t position, UErrorCode &status)
{
    // status checked in usearch_setOffset
    usearch_setOffset(m_strsrch_, position, &status);
}

int32_t StringSearch::getOffset(void) const
{
    return usearch_getOffset(m_strsrch_);
}

void StringSearch::setText(const UnicodeString &text, UErrorCode &status)
{
    if (U_SUCCESS(status)) {
        m_text_ = text;
        usearch_setText(m_strsrch_, text.getBuffer(), text.length(), &status);
    }
}

void StringSearch::setText(CharacterIterator &text, UErrorCode &status)
{
    if (U_SUCCESS(status)) {
        text.getText(m_text_);
        usearch_setText(m_strsrch_, m_text_.getBuffer(), m_text_.length(), &status);
    }
}

RuleBasedCollator * StringSearch::getCollator() const
{
    return (RuleBasedCollator *)&m_collator_;
}

void StringSearch::setCollator(RuleBasedCollator *coll, UErrorCode &status)
{
    if (U_SUCCESS(status)) {
        usearch_setCollator(m_strsrch_, coll->getUCollator(), &status);
        // Alias the collator
        m_collator_.setUCollator((UCollator *)m_strsrch_->collator);
    }
}

void StringSearch::setPattern(const UnicodeString &pattern,
                                    UErrorCode    &status)
{
    if (U_SUCCESS(status)) {
        m_pattern_ = pattern;
        usearch_setPattern(m_strsrch_, m_pattern_.getBuffer(), m_pattern_.length(),
                           &status);
    }
}

const UnicodeString & StringSearch::getPattern() const
{
    return m_pattern_;
}

// public methods ----------------------------------------------------

void StringSearch::reset()
{
    usearch_reset(m_strsrch_);
}

SearchIterator * StringSearch::safeClone(void) const
{
    UErrorCode status = U_ZERO_ERROR;
    StringSearch *result = new StringSearch(m_pattern_, m_text_,
                                            (RuleBasedCollator *)&m_collator_,
                                            m_breakiterator_,
                                            status);
    /* test for NULL */
    if (result == 0) {
        status = U_MEMORY_ALLOCATION_ERROR;
        return 0;
    }
    result->setOffset(getOffset(), status);
    result->setMatchStart(m_strsrch_->search->matchedIndex);
    result->setMatchLength(m_strsrch_->search->matchedLength);
    if (U_FAILURE(status)) {
        return NULL;
    }
    return result;
}

// protected method -------------------------------------------------

int32_t StringSearch::handleNext(int32_t position, UErrorCode &status)
{
    // values passed here are already in the pre-shift position
    if (U_SUCCESS(status)) {
        if (m_strsrch_->pattern.CELength == 0) {
            m_search_->matchedIndex =
                                    m_search_->matchedIndex == USEARCH_DONE ?
                                    getOffset() : m_search_->matchedIndex + 1;
            m_search_->matchedLength = 0;
            ucol_setOffset(m_strsrch_->textIter, m_search_->matchedIndex,
                           &status);
            if (m_search_->matchedIndex == m_search_->textLength) {
                m_search_->matchedIndex = USEARCH_DONE;
            }
        }
        else {
            // looking at usearch.cpp, this part is shifted out to
            // StringSearch instead of SearchIterator because m_strsrch_ is
            // not accessible in SearchIterator
#if 0
            if (position + m_strsrch_->pattern.defaultShiftSize
                > m_search_->textLength) {
                setMatchNotFound();
                return USEARCH_DONE;
            }
#endif
            if (m_search_->matchedLength <= 0) {
                // the flipping direction issue has already been handled
                // in next()
                // for boundary check purposes. this will ensure that the
                // next match will not preceed the current offset
                // note search->matchedIndex will always be set to something
                // in the code
                m_search_->matchedIndex = position - 1;
            }

            ucol_setOffset(m_strsrch_->textIter, position, &status);
            
#if 0
            for (;;) {
                if (m_search_->isCanonicalMatch) {
                    // can't use exact here since extra accents are allowed.
                    usearch_handleNextCanonical(m_strsrch_, &status);
                }
                else {
                    usearch_handleNextExact(m_strsrch_, &status);
                }
                if (U_FAILURE(status)) {
                    return USEARCH_DONE;
                }
                if (m_breakiterator_ == NULL
#if !UCONFIG_NO_BREAK_ITERATION
                    ||
                    m_search_->matchedIndex == USEARCH_DONE ||
                    (m_breakiterator_->isBoundary(m_search_->matchedIndex) &&
                     m_breakiterator_->isBoundary(m_search_->matchedIndex +
                                                  m_search_->matchedLength))
#endif
                ) {
                    if (m_search_->matchedIndex == USEARCH_DONE) {
                        ucol_setOffset(m_strsrch_->textIter,
                                       m_search_->textLength, &status);
                    }
                    else {
                        ucol_setOffset(m_strsrch_->textIter,
                                       m_search_->matchedIndex, &status);
                    }
                    return m_search_->matchedIndex;
                }
            }
#else
            // if m_strsrch_->breakIter is always the same as m_breakiterator_
            // then we don't need to check the match boundaries here because
            // usearch_handleNextXXX will already have done it.
            if (m_search_->isCanonicalMatch) {
            	// *could* actually use exact here 'cause no extra accents allowed...
            	usearch_handleNextCanonical(m_strsrch_, &status);
            } else {
            	usearch_handleNextExact(m_strsrch_, &status);
            }
            
            if (U_FAILURE(status)) {
            	return USEARCH_DONE;
            }
            
            if (m_search_->matchedIndex == USEARCH_DONE) {
            	ucol_setOffset(m_strsrch_->textIter, m_search_->textLength, &status);
            } else {
            	ucol_setOffset(m_strsrch_->textIter, m_search_->matchedIndex, &status);
            }
            
            return m_search_->matchedIndex;
#endif
        }
    }
    return USEARCH_DONE;
}

int32_t StringSearch::handlePrev(int32_t position, UErrorCode &status)
{
    // values passed here are already in the pre-shift position
    if (U_SUCCESS(status)) {
        if (m_strsrch_->pattern.CELength == 0) {
            m_search_->matchedIndex =
                  (m_search_->matchedIndex == USEARCH_DONE ? getOffset() :
                   m_search_->matchedIndex);
            if (m_search_->matchedIndex == 0) {
                setMatchNotFound();
            }
            else {
                m_search_->matchedIndex --;
                ucol_setOffset(m_strsrch_->textIter, m_search_->matchedIndex,
                               &status);
                m_search_->matchedLength = 0;
            }
        }
        else {
            // looking at usearch.cpp, this part is shifted out to
            // StringSearch instead of SearchIterator because m_strsrch_ is
            // not accessible in SearchIterator
#if 0
            if (!m_search_->isOverlap &&
                position - m_strsrch_->pattern.defaultShiftSize < 0) {
                setMatchNotFound();
                return USEARCH_DONE;
            }
            
            for (;;) {
                if (m_search_->isCanonicalMatch) {
                    // can't use exact here since extra accents are allowed.
                    usearch_handlePreviousCanonical(m_strsrch_, &status);
                }
                else {
                    usearch_handlePreviousExact(m_strsrch_, &status);
                }
                if (U_FAILURE(status)) {
                    return USEARCH_DONE;
                }
                if (m_breakiterator_ == NULL
#if !UCONFIG_NO_BREAK_ITERATION
                    ||
                    m_search_->matchedIndex == USEARCH_DONE ||
                    (m_breakiterator_->isBoundary(m_search_->matchedIndex) &&
                     m_breakiterator_->isBoundary(m_search_->matchedIndex +
                                                  m_search_->matchedLength))
#endif
                ) {
                    return m_search_->matchedIndex;
                }
            }
#else
            ucol_setOffset(m_strsrch_->textIter, position, &status);
            
            if (m_search_->isCanonicalMatch) {
            	// *could* use exact match here since extra accents *not* allowed!
            	usearch_handlePreviousCanonical(m_strsrch_, &status);
            } else {
            	usearch_handlePreviousExact(m_strsrch_, &status);
            }
            
            if (U_FAILURE(status)) {
            	return USEARCH_DONE;
            }
            
            return m_search_->matchedIndex;
#endif
        }

        return m_search_->matchedIndex;
    }
    return USEARCH_DONE;
}

U_NAMESPACE_END

#endif /* #if !UCONFIG_NO_COLLATION */
