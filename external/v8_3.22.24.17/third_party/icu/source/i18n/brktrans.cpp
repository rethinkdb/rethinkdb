/*
**********************************************************************
*   Copyright (C) 2008-2010, International Business Machines
*   Corporation and others.  All Rights Reserved.
**********************************************************************
*   Date        Name        Description
*   05/11/2008  Andy Heninger  Port from Java
**********************************************************************
*/

#include "unicode/utypes.h"

#if  !UCONFIG_NO_TRANSLITERATION && !UCONFIG_NO_BREAK_ITERATION

#include "unicode/unifilt.h"
#include "unicode/uchar.h"
#include "unicode/uniset.h"
#include "unicode/brkiter.h"
#include "brktrans.h"
#include "unicode/uchar.h"
#include "cmemory.h"
#include "uprops.h"
#include "uinvchar.h"
#include "util.h"
#include "uvectr32.h"

U_NAMESPACE_BEGIN

UOBJECT_DEFINE_RTTI_IMPLEMENTATION(BreakTransliterator)

static const UChar SPACE       = 32;  // ' '


/**
 * Constructs a transliterator with the default delimiters '{' and
 * '}'.
 */
BreakTransliterator::BreakTransliterator(UnicodeFilter* adoptedFilter) :
    Transliterator(UNICODE_STRING("Any-BreakInternal", 17), adoptedFilter),
    fInsertion(SPACE) {
        bi = NULL;
        UErrorCode status = U_ZERO_ERROR;
        boundaries = new UVector32(status);
    }


/**
 * Destructor.
 */
BreakTransliterator::~BreakTransliterator() {
    delete bi;
    bi = NULL;
    delete boundaries;
    boundaries = NULL;
}

/**
 * Copy constructor.
 */
BreakTransliterator::BreakTransliterator(const BreakTransliterator& o) :
    Transliterator(o) {
        bi = NULL;
        if (o.bi != NULL) {
            bi = o.bi->clone();
        }
        fInsertion = o.fInsertion;
        UErrorCode status = U_ZERO_ERROR;
        boundaries = new UVector32(status);
    }


/**
 * Transliterator API.
 */
Transliterator* BreakTransliterator::clone(void) const {
    return new BreakTransliterator(*this);
}

/**
 * Implements {@link Transliterator#handleTransliterate}.
 */
void BreakTransliterator::handleTransliterate(Replaceable& text, UTransPosition& offsets,
                                                    UBool isIncremental ) const {

        UErrorCode status = U_ZERO_ERROR;
        boundaries->removeAllElements();
        BreakTransliterator *nonConstThis = (BreakTransliterator *)this;
        nonConstThis->getBreakIterator(); // Lazy-create it if necessary
        UnicodeString sText = replaceableAsString(text);
        bi->setText(sText);
        bi->preceding(offsets.start);

        // To make things much easier, we will stack the boundaries, and then insert at the end.
        // generally, we won't need too many, since we will be filtered.

        int32_t boundary;
        for(boundary = bi->next(); boundary != UBRK_DONE && boundary < offsets.limit; boundary = bi->next()) {
            if (boundary == 0) continue;
            // HACK: Check to see that preceeding item was a letter

            UChar32 cp = sText.char32At(boundary-1);
            int type = u_charType(cp);
            //System.out.println(Integer.toString(cp,16) + " (before): " + type);
            if ((U_MASK(type) & (U_GC_L_MASK | U_GC_M_MASK)) == 0) continue;

            cp = sText.char32At(boundary);
            type = u_charType(cp);
            //System.out.println(Integer.toString(cp,16) + " (after): " + type);
            if ((U_MASK(type) & (U_GC_L_MASK | U_GC_M_MASK)) == 0) continue;

            boundaries->addElement(boundary, status);
            // printf("Boundary at %d\n", boundary);
        }

        int delta = 0;
        int lastBoundary = 0;

        if (boundaries->size() != 0) { // if we found something, adjust
            delta = boundaries->size() * fInsertion.length();
            lastBoundary = boundaries->lastElementi();

            // we do this from the end backwards, so that we don't have to keep updating.

            while (boundaries->size() > 0) {
                boundary = boundaries->popi();
                text.handleReplaceBetween(boundary, boundary, fInsertion);
            }
        }

        // Now fix up the return values
        offsets.contextLimit += delta;
        offsets.limit += delta;
        offsets.start = isIncremental ? lastBoundary + delta : offsets.limit;

        // TODO:  do something with U_FAILURE(status);
        //        (need to look at transliterators overall, not just here.)
}

//
//  getInsertion()
//
const UnicodeString &BreakTransliterator::getInsertion() const {
    return fInsertion;
}

//
//  setInsertion()
//
void BreakTransliterator::setInsertion(const UnicodeString &insertion) {
    this->fInsertion = insertion;
}

//
//  getBreakIterator     Lazily create the break iterator if it does
//                       not already exist.  Copied from Java, probably
//                       better to just create it in the constructor.
//
BreakIterator *BreakTransliterator::getBreakIterator() {
    UErrorCode status = U_ZERO_ERROR;
    if (bi == NULL) {
        // Note:  Thai breaking behavior is universal, it is not
        //        tied to the Thai locale.
        bi = BreakIterator::createWordInstance(Locale::getEnglish(), status);
    }
    return bi;
}

//
//   replaceableAsString   Hack to let break iterators work
//                         on the replaceable text from transliterators.
//                         In practice, the only real Replaceable type that we
//                         will be seeing is UnicodeString, so this function
//                         will normally be efficient.
//
UnicodeString BreakTransliterator::replaceableAsString(Replaceable &r) {
    UnicodeString s;
    UnicodeString *rs = dynamic_cast<UnicodeString *>(&r);
    if (rs != NULL) {
        s = *rs;
    } else {
        r.extractBetween(0, r.length(), s);
    }
    return s;
}

U_NAMESPACE_END

#endif /* #if !UCONFIG_NO_TRANSLITERATION */
