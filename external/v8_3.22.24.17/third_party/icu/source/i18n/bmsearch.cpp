/*
 ******************************************************************************
 *   Copyright (C) 1996-2010, International Business Machines                 *
 *   Corporation and others.  All Rights Reserved.                            *
 ******************************************************************************
 */

#include "unicode/utypes.h"

#if !UCONFIG_NO_COLLATION && !UCONFIG_NO_BREAK_ITERATION

#include "unicode/unistr.h"
#include "unicode/putil.h"
#include "unicode/usearch.h"

#include "cmemory.h"
#include "unicode/coll.h"
#include "unicode/tblcoll.h"
#include "unicode/coleitr.h"
#include "unicode/ucoleitr.h"

#include "unicode/regex.h"        // TODO: make conditional on regexp being built.

#include "unicode/uniset.h"
#include "unicode/uset.h"
#include "unicode/ustring.h"
#include "hash.h"
#include "uhash.h"
#include "ucol_imp.h"
#include "normalizer2impl.h"

#include "unicode/colldata.h"
#include "unicode/bmsearch.h"

U_NAMESPACE_BEGIN

#define ARRAY_SIZE(array) (sizeof(array)/sizeof(array[0]))
#define NEW_ARRAY(type, count) (type *) uprv_malloc((count) * sizeof(type))
#define DELETE_ARRAY(array) uprv_free((void *) (array))


struct CEI
{
    uint32_t order;
    int32_t  lowOffset;
    int32_t  highOffset;
};

class Target : public UMemory
{
public:
    Target(UCollator *theCollator, const UnicodeString *target, int32_t patternLength, UErrorCode &status);
    ~Target();

    void setTargetString(const UnicodeString *target);

    const CEI *nextCE(int32_t offset);
    const CEI *prevCE(int32_t offset);

    int32_t stringLength();
    UChar charAt(int32_t offset);

    UBool isBreakBoundary(int32_t offset);
    int32_t nextBreakBoundary(int32_t offset);
    int32_t nextSafeBoundary(int32_t offset);

    UBool isIdentical(UnicodeString &pattern, int32_t start, int32_t end);

    void setOffset(int32_t offset);
    void setLast(int32_t last);
    int32_t getOffset();

private:
    CEI *ceb;
    int32_t bufferSize;
    int32_t bufferMin;
    int32_t bufferMax;

    uint32_t strengthMask;
    UCollationStrength strength;
    uint32_t variableTop;
    UBool toShift;
    UCollator *coll;
    const Normalizer2 &nfd;

    const UnicodeString *targetString;
    const UChar *targetBuffer;
    int32_t targetLength;

    UCollationElements *elements;
    UBreakIterator *charBreakIterator;
};

Target::Target(UCollator *theCollator, const UnicodeString *target, int32_t patternLength, UErrorCode &status)
    : bufferSize(0), bufferMin(0), bufferMax(0),
      strengthMask(0), strength(UCOL_PRIMARY), variableTop(0), toShift(FALSE), coll(theCollator),
      nfd(*Normalizer2Factory::getNFDInstance(status)),
      targetString(NULL), targetBuffer(NULL), targetLength(0), elements(NULL), charBreakIterator(NULL)
{
    strength = ucol_getStrength(coll);
    toShift = ucol_getAttribute(coll, UCOL_ALTERNATE_HANDLING, &status) ==  UCOL_SHIFTED;
    variableTop = ucol_getVariableTop(coll, &status);

    // find the largest expansion
    uint8_t maxExpansion = 0;
    for (const uint8_t *expansion = coll->expansionCESize; *expansion != 0; expansion += 1) {
        if (*expansion > maxExpansion) {
            maxExpansion = *expansion;
        }
    }

    // room for an extra character on each end, plus 4 for safety
    bufferSize = patternLength + (2 * maxExpansion) + 4;

    ceb = NEW_ARRAY(CEI, bufferSize);

    if (ceb == NULL) {
        status = U_MEMORY_ALLOCATION_ERROR;
        return;
    }

    if (target != NULL) {
        setTargetString(target);
    }

    switch (strength)
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
}

Target::~Target()
{
    ubrk_close(charBreakIterator);
    ucol_closeElements(elements);

    DELETE_ARRAY(ceb);
}

void Target::setTargetString(const UnicodeString *target)
{
    if (charBreakIterator != NULL) {
        ubrk_close(charBreakIterator);
        ucol_closeElements(elements);
    }

    targetString = target;

    if (targetString != NULL) {
        UErrorCode status = U_ZERO_ERROR;

        targetBuffer = targetString->getBuffer();
        targetLength = targetString->length();

        elements = ucol_openElements(coll, target->getBuffer(), target->length(), &status);
        ucol_forceHanImplicit(elements, &status);

        charBreakIterator = ubrk_open(UBRK_CHARACTER, ucol_getLocaleByType(coll, ULOC_VALID_LOCALE, &status),
                                      targetBuffer, targetLength, &status);
    } else {
        targetBuffer = NULL;
        targetLength = 0;
    }
}

const CEI *Target::nextCE(int32_t offset)
{
    UErrorCode status = U_ZERO_ERROR;
    int32_t low = -1, high = -1;
    uint32_t order;
    UBool cont = FALSE;

    if (offset >= bufferMin && offset < bufferMax) {
        return &ceb[offset];
    }

    if (bufferMax >= bufferSize || offset != bufferMax) {
        return NULL;
    }

    do {
        low   = ucol_getOffset(elements);
        order = ucol_next(elements, &status);
        high  = ucol_getOffset(elements);

        if (order == (uint32_t)UCOL_NULLORDER) {
          //high = low = -1;
            break;
        }

        cont = isContinuation(order);
        order &= strengthMask;

        if (toShift && variableTop > order && (order & UCOL_PRIMARYORDERMASK) != 0) {
            if (strength >= UCOL_QUATERNARY) {
                order &= UCOL_PRIMARYORDERMASK;
            } else {
                order = UCOL_IGNORABLE;
            }
        }
    } while (order == UCOL_IGNORABLE);

    if (cont) {
        order |= UCOL_CONTINUATION_MARKER;
    }

    ceb[offset].order = order;
    ceb[offset].lowOffset = low;
    ceb[offset].highOffset = high;

    bufferMax += 1;

    return &ceb[offset];
}

const CEI *Target::prevCE(int32_t offset)
{
    UErrorCode status = U_ZERO_ERROR;
    int32_t low = -1, high = -1;
    uint32_t order;
    UBool cont = FALSE;

    if (offset >= bufferMin && offset < bufferMax) {
        return &ceb[offset];
    }

    if (bufferMax >= bufferSize || offset != bufferMax) {
        return NULL;
    }

    do {
        high  = ucol_getOffset(elements);
        order = ucol_previous(elements, &status);
        low   = ucol_getOffset(elements);

        if (order == (uint32_t)UCOL_NULLORDER) {
            break;
        }

        cont = isContinuation(order);
        order &= strengthMask;

        if (toShift && variableTop > order && (order & UCOL_PRIMARYORDERMASK) != 0) {
            if (strength >= UCOL_QUATERNARY) {
                order &= UCOL_PRIMARYORDERMASK;
            } else {
                order = UCOL_IGNORABLE;
            }
        }
    } while (order == UCOL_IGNORABLE);

    bufferMax += 1;

    if (cont) {
        order |= UCOL_CONTINUATION_MARKER;
    }

    ceb[offset].order       = order;
    ceb[offset].lowOffset   = low;
    ceb[offset].highOffset = high;

    return &ceb[offset];
}

int32_t Target::stringLength()
{
    if (targetString != NULL) {
        return targetLength;
    }

    return 0;
}

UChar Target::charAt(int32_t offset)
{
    if (targetString != NULL) {
        return targetBuffer[offset];
    }

    return 0x0000;
}

void Target::setOffset(int32_t offset)
{
    UErrorCode status = U_ZERO_ERROR;

    bufferMin = 0;
    bufferMax = 0;

    ucol_setOffset(elements, offset, &status);
}

void Target::setLast(int32_t last)
{
    UErrorCode status = U_ZERO_ERROR;

    bufferMin = 0;
    bufferMax = 1;

    ceb[0].order      = UCOL_NULLORDER;
    ceb[0].lowOffset  = last;
    ceb[0].highOffset = last;

    ucol_setOffset(elements, last, &status);
}

int32_t Target::getOffset()
{
    return ucol_getOffset(elements);
}

UBool Target::isBreakBoundary(int32_t offset)
{
    return ubrk_isBoundary(charBreakIterator, offset);
}

int32_t Target::nextBreakBoundary(int32_t offset)
{
    return ubrk_following(charBreakIterator, offset);
}

int32_t Target::nextSafeBoundary(int32_t offset)
{
    while (offset < targetLength) {
      //UChar ch = charAt(offset);
        UChar ch = targetBuffer[offset];

        if (U_IS_LEAD(ch) || ! ucol_unsafeCP(ch, coll)) {
            return offset;
        }

        offset += 1;
    }

    return targetLength;
}

UBool Target::isIdentical(UnicodeString &pattern, int32_t start, int32_t end)
{
    if (strength < UCOL_IDENTICAL) {
        return TRUE;
    }

    // Note: We could use Normalizer::compare() or similar, but for short strings
    // which may not be in FCD it might be faster to just NFD them.
    UErrorCode status = U_ZERO_ERROR;
    UnicodeString t2, p2;
    nfd.normalize(UnicodeString(FALSE, targetBuffer + start, end - start), t2, status);
    nfd.normalize(pattern, p2, status);
    // return FALSE if NFD failed
    return U_SUCCESS(status) && t2 == p2;
}

#define HASH_TABLE_SIZE 257

class BadCharacterTable : public UMemory
{
public:
    BadCharacterTable(CEList &patternCEs, CollData *data, UErrorCode &status);
    ~BadCharacterTable();

    int32_t operator[](uint32_t ce) const;
    int32_t getMaxSkip() const;
    int32_t minLengthInChars(int32_t index);

private:
    static int32_t hash(uint32_t ce);

    int32_t maxSkip;
    int32_t badCharacterTable[HASH_TABLE_SIZE];

    int32_t *minLengthCache;
};

BadCharacterTable::BadCharacterTable(CEList &patternCEs, CollData *data, UErrorCode &status)
    : minLengthCache(NULL)
{
    int32_t plen = patternCEs.size();

    // **** need a better way to deal with this ****
    if (U_FAILURE(status) || plen == 0) {
        return;
    }

    int32_t *history = NEW_ARRAY(int32_t, plen);

    if (history == NULL) {
        status = U_MEMORY_ALLOCATION_ERROR;
        return;
    }

    for (int32_t i = 0; i < plen; i += 1) {
        history[i] = -1;
    }

    minLengthCache = NEW_ARRAY(int32_t, plen + 1);

    if (minLengthCache == NULL) {
        DELETE_ARRAY(history);
        status = U_MEMORY_ALLOCATION_ERROR;
        return;
    }

    maxSkip = minLengthCache[0] = data->minLengthInChars(&patternCEs, 0, history);

    for(int32_t j = 0; j < HASH_TABLE_SIZE; j += 1) {
        badCharacterTable[j] = maxSkip;
    }

    for(int32_t p = 1; p < plen; p += 1) {
        minLengthCache[p] = data->minLengthInChars(&patternCEs, p, history);

        // Make sure this entry is not bigger than the previous one.
        // Otherwise, we might skip too far in some cases.
        if (minLengthCache[p] < 0 || minLengthCache[p] > minLengthCache[p - 1]) {
            minLengthCache[p] = minLengthCache[p - 1];
        }
    }

    minLengthCache[plen] = 0;

    for(int32_t p = 0; p < plen - 1; p += 1) {
        badCharacterTable[hash(patternCEs[p])] = minLengthCache[p + 1];
    }

    DELETE_ARRAY(history);
}

BadCharacterTable::~BadCharacterTable()
{
    DELETE_ARRAY(minLengthCache);
}

int32_t BadCharacterTable::operator[](uint32_t ce) const
{
    return badCharacterTable[hash(ce)];
}

int32_t BadCharacterTable::getMaxSkip() const
{
    return maxSkip;
}

int32_t BadCharacterTable::minLengthInChars(int32_t index)
{
    return minLengthCache[index];
}

int32_t BadCharacterTable::hash(uint32_t ce)
{
    return UCOL_PRIMARYORDER(ce) % HASH_TABLE_SIZE;
}

class GoodSuffixTable : public UMemory
{
public:
    GoodSuffixTable(CEList &patternCEs, BadCharacterTable &badCharacterTable, UErrorCode &status);
    ~GoodSuffixTable();

    int32_t operator[](int32_t offset) const;

private:
    int32_t *goodSuffixTable;
};

GoodSuffixTable::GoodSuffixTable(CEList &patternCEs, BadCharacterTable &badCharacterTable, UErrorCode &status)
    : goodSuffixTable(NULL)
{
    int32_t patlen = patternCEs.size();

    // **** need a better way to deal with this ****
    if (U_FAILURE(status) || patlen <= 0) {
        return;
    }

    int32_t *suff  = NEW_ARRAY(int32_t, patlen);
    int32_t start = patlen - 1, end = - 1;
    int32_t maxSkip = badCharacterTable.getMaxSkip();

    if (suff == NULL) {
        status = U_MEMORY_ALLOCATION_ERROR;
        return;
    }

    // initialze suff
    suff[patlen - 1] = patlen;

    for (int32_t i = patlen - 2; i >= 0; i -= 1) {
        // (i > start) means we're inside the last suffix match we found
        // ((patlen - 1) - end) is how far the end of that match is from end of pattern
        // (i - start) is how far we are from start of that match
        // (i + (patlen - 1) - end) is index of same character at end of pattern
        // so if any suffix match at that character doesn't extend beyond the last match,
        // it's the suffix for this character as well
        if (i > start && suff[i + patlen - 1 - end] < i - start) {
            suff[i] = suff[i + patlen - 1 - end];
        } else {
            start = end = i;

            int32_t s = patlen;

            while (start >= 0 && patternCEs[start] == patternCEs[--s]) {
                start -= 1;
            }

            suff[i] = end - start;
        }
    }

    // now build goodSuffixTable
    goodSuffixTable  = NEW_ARRAY(int32_t, patlen);

    if (goodSuffixTable == NULL) {
        DELETE_ARRAY(suff);
        status = U_MEMORY_ALLOCATION_ERROR;
        return;
    }


    // initialize entries to minLengthInChars of the pattern
    for (int32_t i = 0; i < patlen; i += 1) {
        goodSuffixTable[i] = maxSkip;
    }

    int32_t prefix = 0;

    for (int32_t i = patlen - /*1*/ 2; i >= 0; i -= 1) {
        if (suff[i] == i + 1) {
            // this matching suffix is a prefix of the pattern
            int32_t prefixSkip = badCharacterTable.minLengthInChars(i + 1);

            // for any mis-match before this suffix, we should skip
            // so that the front of the pattern (i.e. the prefix)
            // lines up with the front of the suffix.
            // (patlen - 1 - i) is the start of the suffix
            while (prefix < patlen - 1 - i) {
                // value of maxSkip means never set...
                if (goodSuffixTable[prefix] == maxSkip) {
                    goodSuffixTable[prefix] = prefixSkip;
                }

                prefix += 1;
            }
        }
    }

    for (int32_t i = 0; i < patlen - 1; i += 1) {
        goodSuffixTable[patlen - 1 - suff[i]] = badCharacterTable.minLengthInChars(i + 1);
    }

    DELETE_ARRAY(suff);
}

GoodSuffixTable::~GoodSuffixTable()
{
    DELETE_ARRAY(goodSuffixTable);
}

int32_t GoodSuffixTable::operator[](int32_t offset) const
{
    return goodSuffixTable[offset];
}

UOBJECT_DEFINE_RTTI_IMPLEMENTATION(BoyerMooreSearch)


UBool BoyerMooreSearch::empty()
{
    return patCEs->size() <= 0;
}

CollData *BoyerMooreSearch::getData()
{
    return data;
}

CEList *BoyerMooreSearch::getPatternCEs()
{
    return patCEs;
}

BadCharacterTable *BoyerMooreSearch::getBadCharacterTable()
{
    return badCharacterTable;
}

GoodSuffixTable *BoyerMooreSearch::getGoodSuffixTable()
{
    return goodSuffixTable;
}

BoyerMooreSearch::BoyerMooreSearch(CollData *theData, const UnicodeString &patternString, const UnicodeString *targetString,
                                   UErrorCode &status)
    : data(theData), patCEs(NULL), badCharacterTable(NULL), goodSuffixTable(NULL), pattern(patternString), target(NULL)
{

    if (U_FAILURE(status)) {
        return;
    }

    UCollator *collator = data->getCollator();

    patCEs = new CEList(collator, patternString, status);

    if (patCEs == NULL || U_FAILURE(status)) {
        return;
    }

    badCharacterTable = new BadCharacterTable(*patCEs, data, status);

    if (badCharacterTable == NULL || U_FAILURE(status)) {
        return;
    }

    goodSuffixTable = new GoodSuffixTable(*patCEs, *badCharacterTable, status);

    if (targetString != NULL) {
        target = new Target(collator, targetString, patCEs->size(), status);
    }
}

BoyerMooreSearch::~BoyerMooreSearch()
{
    delete target;
    delete goodSuffixTable;
    delete badCharacterTable;
    delete patCEs;
}

void BoyerMooreSearch::setTargetString(const UnicodeString *targetString, UErrorCode &status)
{
    if (U_FAILURE(status)) {
        return;
    }

    if (target == NULL) {
        target = new Target(data->getCollator(), targetString, patCEs->size(), status);
    } else {
        target->setTargetString(targetString);
    }
}

// **** main flow of this code from Laura Werner's "Unicode Text Searching in Java" paper. ****
/*
 * TODO:
 *  * deal with trailing (and leading?) ignorables.
 *  * Adding BoyerMooreSearch object slowed it down. How can we speed it up?
 */
UBool BoyerMooreSearch::search(int32_t offset, int32_t &start, int32_t &end)
{
    /*UCollator *coll =*/ data->getCollator();
    int32_t plen = patCEs->size();
    int32_t tlen = target->stringLength();
    int32_t maxSkip = badCharacterTable->getMaxSkip();
    int32_t tOffset = offset + maxSkip;

    if (plen <= 0) {
        // Searching for a zero length pattern always fails.
        start = end = -1;
        return FALSE;
    }

    while (tOffset <= tlen) {
        int32_t pIndex = plen - 1;
        int32_t tIndex = 0;
        int32_t lIndex = 0;

        if (tOffset < tlen) {
            // **** we really want to skip ahead enough to  ****
            // **** be sure we get at least 1 non-ignorable ****
            // **** CE after the end of the pattern.        ****
            int32_t next = target->nextSafeBoundary(tOffset + 1);

            target->setOffset(next);

            for (lIndex = 0; ; lIndex += 1) {
                const CEI *cei = target->prevCE(lIndex);
                int32_t low = cei->lowOffset;
                int32_t high = cei->highOffset;

                if (high == 0 || (low < high && low <= tOffset)) {
                    if (low < tOffset) {
                        while (lIndex >= 0 && target->prevCE(lIndex)->highOffset == high) {
                            lIndex -= 1;
                        }

                        if (high > tOffset) {
                            tOffset = high;
                        }
                    }

                    break;
                }
            }
        } else {
            target->setLast(tOffset);
            lIndex = 0;
        }

        tIndex = ++lIndex;

        // Iterate backward until we hit the beginning of the pattern
        while (pIndex >= 0) {
            uint32_t pce = (*patCEs)[pIndex];
            const CEI *tcei = target->prevCE(tIndex++);


            if (tcei->order != pce) {
                // There is a mismatch at this position.  Decide how far
                // over to shift the pattern, then try again.

                int32_t gsOffset = tOffset + (*goodSuffixTable)[pIndex];
#ifdef EXTRA_CAUTIOUS
                int32_t old = tOffset;
#endif

                tOffset += (*badCharacterTable)[tcei->order] - badCharacterTable->minLengthInChars(pIndex + 1);

                if (gsOffset > tOffset) {
                    tOffset = gsOffset;
                }

#ifdef EXTRA_CAUTIOUS
                // Make sure we don't skip backwards...
                if (tOffset <= old) {
                    tOffset = old + 1;
                }
#endif

                break;
            }

            pIndex -= 1;
        }

        if (pIndex < 0) {
            // We made it back to the beginning of the pattern,
            // which means we matched it all.  Return the location.
            const CEI firstCEI = *target->prevCE(tIndex - 1);
            const CEI lastCEI  = *target->prevCE(lIndex);
            int32_t mStart   = firstCEI.lowOffset;
            int32_t minLimit = lastCEI.lowOffset;
            int32_t maxLimit = lastCEI.highOffset;
            int32_t mLimit;
            UBool found = TRUE;

            target->setOffset(/*tOffset*/maxLimit);

            const CEI nextCEI = *target->nextCE(0);

            if (nextCEI.lowOffset > maxLimit) {
                maxLimit = nextCEI.lowOffset;
            }

            if (nextCEI.lowOffset == nextCEI.highOffset && nextCEI.order != (uint32_t)UCOL_NULLORDER) {
                found = FALSE;
            }

            if (! target->isBreakBoundary(mStart)) {
                found = FALSE;
            }

            if (firstCEI.lowOffset == firstCEI.highOffset) {
                found = FALSE;
            }

            mLimit = maxLimit;
            if (minLimit < maxLimit) {
                int32_t nbb = target->nextBreakBoundary(minLimit);

                if (nbb >= lastCEI.highOffset) {
                    mLimit = nbb;
                }
            }

            if (mLimit > maxLimit) {
                found = FALSE;
            }

            if (! target->isBreakBoundary(mLimit)) {
                found = FALSE;
            }

            if (! target->isIdentical(pattern, mStart, mLimit)) {
                found = FALSE;
            }

            if (found) {
                start = mStart;
                end   = mLimit;

                return TRUE;
            }

            tOffset += (*goodSuffixTable)[0]; // really? Maybe += 1 or += maxSkip?
        }
        // Otherwise, we're here because of a mismatch, so keep going....
    }

    // no match
   start = -1;
   end = -1;
   return FALSE;
}

U_NAMESPACE_END

#endif // #if !UCONFIG_NO_COLLATION
