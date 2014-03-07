/*
**********************************************************************
*   Copyright (C) 2001-2010, International Business Machines
*   Corporation and others.  All Rights Reserved.
**********************************************************************
*   Date        Name        Description
*   07/03/01    aliu        Creation.
**********************************************************************
*/

#include "unicode/utypes.h"

#if !UCONFIG_NO_TRANSLITERATION

#include "unicode/normalizer2.h"
#include "cstring.h"
#include "nortrans.h"

U_NAMESPACE_BEGIN

UOBJECT_DEFINE_RTTI_IMPLEMENTATION(NormalizationTransliterator)

static inline Transliterator::Token cstrToken(const char *s) {
    return Transliterator::pointerToken((void *)s);
}

/**
 * System registration hook.
 */
void NormalizationTransliterator::registerIDs() {
    // In the Token, the byte after the NUL is the UNormalization2Mode.
    Transliterator::_registerFactory(UNICODE_STRING_SIMPLE("Any-NFC"),
                                     _create, cstrToken("nfc\0\0"));
    Transliterator::_registerFactory(UNICODE_STRING_SIMPLE("Any-NFKC"),
                                     _create, cstrToken("nfkc\0\0"));
    Transliterator::_registerFactory(UNICODE_STRING_SIMPLE("Any-NFD"),
                                     _create, cstrToken("nfc\0\1"));
    Transliterator::_registerFactory(UNICODE_STRING_SIMPLE("Any-NFKD"),
                                     _create, cstrToken("nfkc\0\1"));
    Transliterator::_registerFactory(UNICODE_STRING_SIMPLE("Any-FCD"),
                                     _create, cstrToken("nfc\0\2"));
    Transliterator::_registerFactory(UNICODE_STRING_SIMPLE("Any-FCC"),
                                     _create, cstrToken("nfc\0\3"));
    Transliterator::_registerSpecialInverse(UNICODE_STRING_SIMPLE("NFC"),
                                            UNICODE_STRING_SIMPLE("NFD"), TRUE);
    Transliterator::_registerSpecialInverse(UNICODE_STRING_SIMPLE("NFKC"),
                                            UNICODE_STRING_SIMPLE("NFKD"), TRUE);
    Transliterator::_registerSpecialInverse(UNICODE_STRING_SIMPLE("FCC"),
                                            UNICODE_STRING_SIMPLE("NFD"), FALSE);
    Transliterator::_registerSpecialInverse(UNICODE_STRING_SIMPLE("FCD"),
                                            UNICODE_STRING_SIMPLE("FCD"), FALSE);
}

/**
 * Factory methods
 */
Transliterator* NormalizationTransliterator::_create(const UnicodeString& ID,
                                                     Token context) {
    const char *name = (const char *)context.pointer;
    UNormalization2Mode mode = (UNormalization2Mode)uprv_strchr(name, 0)[1];
    UErrorCode errorCode = U_ZERO_ERROR;
    const Normalizer2 *norm2 = Normalizer2::getInstance(NULL, name, mode, errorCode);
    if(U_SUCCESS(errorCode)) {
        return new NormalizationTransliterator(ID, *norm2);
    } else {
        return NULL;
    }
}

/**
 * Constructs a transliterator.
 */
NormalizationTransliterator::NormalizationTransliterator(const UnicodeString& id,
                                                         const Normalizer2 &norm2) :
    Transliterator(id, 0), fNorm2(norm2) {}

/**
 * Destructor.
 */
NormalizationTransliterator::~NormalizationTransliterator() {
}

/**
 * Copy constructor.
 */
NormalizationTransliterator::NormalizationTransliterator(const NormalizationTransliterator& o) :
    Transliterator(o), fNorm2(o.fNorm2) {}

/**
 * Transliterator API.
 */
Transliterator* NormalizationTransliterator::clone(void) const {
    return new NormalizationTransliterator(*this);
}

/**
 * Implements {@link Transliterator#handleTransliterate}.
 */
void NormalizationTransliterator::handleTransliterate(Replaceable& text, UTransPosition& offsets,
                                                      UBool isIncremental) const {
    // start and limit of the input range
    int32_t start = offsets.start;
    int32_t limit = offsets.limit;
    if(start >= limit) {
        return;
    }

    /*
     * Normalize as short chunks at a time as possible even in
     * bulk mode, so that styled text is minimally disrupted.
     * In incremental mode, a chunk that ends with offsets.limit
     * must not be normalized.
     *
     * If it was known that the input text is not styled, then
     * a bulk mode normalization could look like this:

    UnicodeString input, normalized;
    int32_t length = limit - start;
    _Replaceable_extractBetween(text, start, limit, input.getBuffer(length));
    input.releaseBuffer(length);

    UErrorCode status = U_ZERO_ERROR;
    fNorm2.normalize(input, normalized, status);

    text.handleReplaceBetween(start, limit, normalized);

    int32_t delta = normalized.length() - length;
    offsets.contextLimit += delta;
    offsets.limit += delta;
    offsets.start = limit + delta;

     */
    UErrorCode errorCode = U_ZERO_ERROR;
    UnicodeString segment;
    UnicodeString normalized;
    UChar32 c = text.char32At(start);
    do {
        int32_t prev = start;
        // Skip at least one character so we make progress.
        // c holds the character at start.
        segment.remove();
        do {
            segment.append(c);
            start += U16_LENGTH(c);
        } while(start < limit && !fNorm2.hasBoundaryBefore(c = text.char32At(start)));
        if(start == limit && isIncremental && !fNorm2.hasBoundaryAfter(c)) {
            // stop in incremental mode when we reach the input limit
            // in case there are additional characters that could change the
            // normalization result
            start=prev;
            break;
        }
        fNorm2.normalize(segment, normalized, errorCode);
        if(U_FAILURE(errorCode)) {
            break;
        }
        if(segment != normalized) {
            // replace the input chunk with its normalized form
            text.handleReplaceBetween(prev, start, normalized);

            // update all necessary indexes accordingly
            int32_t delta = normalized.length() - (start - prev);
            start += delta;
            limit += delta;
        }
    } while(start < limit);

    offsets.start = start;
    offsets.contextLimit += limit - offsets.limit;
    offsets.limit = limit;
}

U_NAMESPACE_END

#endif /* #if !UCONFIG_NO_TRANSLITERATION */
