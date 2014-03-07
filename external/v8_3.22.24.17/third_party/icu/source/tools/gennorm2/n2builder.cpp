/*
*******************************************************************************
*
*   Copyright (C) 2009-2010, International Business Machines
*   Corporation and others.  All Rights Reserved.
*
*******************************************************************************
*   file name:  n2builder.cpp
*   encoding:   US-ASCII
*   tab size:   8 (not used)
*   indentation:4
*
*   created on: 2009nov25
*   created by: Markus W. Scherer
*
* Builds Normalizer2 data and writes a binary .nrm file.
* For the file format see source/common/normalizer2impl.h.
*/

#include "unicode/utypes.h"
#include "n2builder.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#if U_HAVE_STD_STRING
#include <vector>
#endif
#include "unicode/errorcode.h"
#include "unicode/localpointer.h"
#include "unicode/putil.h"
#include "unicode/udata.h"
#include "unicode/uniset.h"
#include "unicode/unistr.h"
#include "unicode/ustring.h"
#include "hash.h"
#include "normalizer2impl.h"
#include "toolutil.h"
#include "unewdata.h"
#include "utrie2.h"
#include "uvectr32.h"

#define LENGTHOF(array) (int32_t)(sizeof(array)/sizeof((array)[0]))

#if !UCONFIG_NO_NORMALIZATION

/* UDataInfo cf. udata.h */
static UDataInfo dataInfo={
    sizeof(UDataInfo),
    0,

    U_IS_BIG_ENDIAN,
    U_CHARSET_FAMILY,
    U_SIZEOF_UCHAR,
    0,

    { 0x4e, 0x72, 0x6d, 0x32 }, /* dataFormat="Nrm2" */
    { 1, 0, 0, 0 },             /* formatVersion */
    { 5, 2, 0, 0 }              /* dataVersion (Unicode version) */
};

U_NAMESPACE_BEGIN

class HangulIterator {
public:
    struct Range {
        UChar32 start, limit;
        uint16_t norm16;
    };

    HangulIterator() : rangeIndex(0) {}
    const Range *nextRange() {
        if(rangeIndex<LENGTHOF(ranges)) {
            return ranges+rangeIndex++;
        } else {
            return NULL;
        }
    }
    void reset() { rangeIndex=0; }
private:
    static const Range ranges[4];
    int32_t rangeIndex;
};

const HangulIterator::Range HangulIterator::ranges[4]={
    { Hangul::JAMO_L_BASE, Hangul::JAMO_L_BASE+Hangul::JAMO_L_COUNT, 1 },
    { Hangul::JAMO_V_BASE, Hangul::JAMO_V_BASE+Hangul::JAMO_V_COUNT, Normalizer2Impl::JAMO_VT },
    // JAMO_T_BASE+1: not U+11A7
    { Hangul::JAMO_T_BASE+1, Hangul::JAMO_T_BASE+Hangul::JAMO_T_COUNT, Normalizer2Impl::JAMO_VT },
    { Hangul::HANGUL_BASE, Hangul::HANGUL_BASE+Hangul::HANGUL_COUNT, 0 },  // will become minYesNo
};

struct CompositionPair {
    CompositionPair(UChar32 t, UChar32 c) : trail(t), composite(c) {}
    UChar32 trail, composite;
};

struct Norm {
    enum MappingType { NONE, REMOVED, ROUND_TRIP, ONE_WAY };

    UBool hasMapping() const { return mappingType>REMOVED; }

    // Requires hasMapping() and well-formed mapping.
    void setMappingCP() {
        UChar32 c;
        if(!mapping->isEmpty() && mapping->length()==U16_LENGTH(c=mapping->char32At(0))) {
            mappingCP=c;
        } else {
            mappingCP=U_SENTINEL;
        }
    }

    const CompositionPair *getCompositionPairs(int32_t &length) const {
        if(compositions==NULL) {
            length=0;
            return NULL;
        } else {
            length=compositions->size()/2;
            return reinterpret_cast<const CompositionPair *>(compositions->getBuffer());
        }
    }

    UnicodeString *mapping;
    UChar32 mappingCP;  // >=0 if mapping to 1 code point
    int32_t mappingPhase;
    MappingType mappingType;

    UVector32 *compositions;  // (trail, composite) pairs
    uint8_t cc;
    UBool combinesBack;
    UBool hasNoCompBoundaryAfter;

    enum OffsetType {
        OFFSET_NONE, OFFSET_MAYBE_YES,
        OFFSET_YES_YES, OFFSET_YES_NO, OFFSET_NO_NO,
        OFFSET_DELTA
    };
    enum { OFFSET_SHIFT=4, OFFSET_MASK=(1<<OFFSET_SHIFT)-1 };
    int32_t offset;
};

class Normalizer2DBEnumerator {
public:
    Normalizer2DBEnumerator(Normalizer2DataBuilder &b) : builder(b) {}
    virtual ~Normalizer2DBEnumerator() {}
    virtual UBool rangeHandler(UChar32 start, UChar32 end, uint32_t value) = 0;
    Normalizer2DBEnumerator *ptr() { return this; }
protected:
    Normalizer2DataBuilder &builder;
};

U_CDECL_BEGIN

static UBool U_CALLCONV
enumRangeHandler(const void *context, UChar32 start, UChar32 end, uint32_t value) {
    return ((Normalizer2DBEnumerator *)context)->rangeHandler(start, end, value);
}

U_CDECL_END

Normalizer2DataBuilder::Normalizer2DataBuilder(UErrorCode &errorCode) :
        phase(0), overrideHandling(OVERRIDE_PREVIOUS), optimization(OPTIMIZE_NORMAL) {
    memset(unicodeVersion, 0, sizeof(unicodeVersion));
    normTrie=utrie2_open(0, 0, &errorCode);
    normMem=utm_open("gennorm2 normalization structs", 10000, 0x110100, sizeof(Norm));
    norms=allocNorm();  // unused Norm struct at index 0
    memset(indexes, 0, sizeof(indexes));
}

Normalizer2DataBuilder::~Normalizer2DataBuilder() {
    utrie2_close(normTrie);
    int32_t normsLength=utm_countItems(normMem);
    for(int32_t i=1; i<normsLength; ++i) {
        delete norms[i].mapping;
        delete norms[i].compositions;
    }
    utm_close(normMem);
    utrie2_close(norm16Trie);
}

void
Normalizer2DataBuilder::setUnicodeVersion(const char *v) {
    u_versionFromString(unicodeVersion, v);
}

Norm *Normalizer2DataBuilder::allocNorm() {
    Norm *p=(Norm *)utm_alloc(normMem);
    norms=(Norm *)utm_getStart(normMem);  // in case it got reallocated
    return p;
}

/* get an existing Norm unit */
Norm *Normalizer2DataBuilder::getNorm(UChar32 c) {
    uint32_t i=utrie2_get32(normTrie, c);
    if(i==0) {
        return NULL;
    }
    return norms+i;
}

const Norm &Normalizer2DataBuilder::getNormRef(UChar32 c) const {
    return norms[utrie2_get32(normTrie, c)];
}

/*
 * get or create a Norm unit;
 * get or create the intermediate trie entries for it as well
 */
Norm *Normalizer2DataBuilder::createNorm(UChar32 c) {
    uint32_t i=utrie2_get32(normTrie, c);
    if(i!=0) {
        return norms+i;
    } else {
        /* allocate Norm */
        Norm *p=allocNorm();
        IcuToolErrorCode errorCode("gennorm2/createNorm()");
        utrie2_set32(normTrie, c, (uint32_t)(p-norms), errorCode);
        return p;
    }
}

Norm *Normalizer2DataBuilder::checkNormForMapping(Norm *p, UChar32 c) {
    if(p!=NULL) {
        if(p->mappingType!=Norm::NONE) {
            if( overrideHandling==OVERRIDE_NONE ||
                (overrideHandling==OVERRIDE_PREVIOUS && p->mappingPhase==phase)
            ) {
                fprintf(stderr,
                        "error in gennorm2 phase %d: "
                        "not permitted to override mapping for U+%04lX from phase %d\n",
                        (int)phase, (long)c, (int)p->mappingPhase);
                exit(U_INVALID_FORMAT_ERROR);
            }
            delete p->mapping;
            p->mapping=NULL;
        }
        p->mappingPhase=phase;
    }
    return p;
}

void Normalizer2DataBuilder::setOverrideHandling(OverrideHandling oh) {
    overrideHandling=oh;
    ++phase;
}

void Normalizer2DataBuilder::setCC(UChar32 c, uint8_t cc) {
    createNorm(c)->cc=cc;
}

uint8_t Normalizer2DataBuilder::getCC(UChar32 c) const {
    return getNormRef(c).cc;
}

static UBool isWellFormed(const UnicodeString &s) {
    UErrorCode errorCode=U_ZERO_ERROR;
    u_strToUTF8(NULL, 0, NULL, s.getBuffer(), s.length(), &errorCode);
    return U_SUCCESS(errorCode) || errorCode==U_BUFFER_OVERFLOW_ERROR;
}

void Normalizer2DataBuilder::setOneWayMapping(UChar32 c, const UnicodeString &m) {
    if(!isWellFormed(m)) {
        fprintf(stderr,
                "error in gennorm2 phase %d: "
                "illegal one-way mapping from U+%04lX to malformed string\n",
                (int)phase, (long)c);
        exit(U_INVALID_FORMAT_ERROR);
    }
    Norm *p=checkNormForMapping(createNorm(c), c);
    p->mapping=new UnicodeString(m);
    p->mappingType=Norm::ONE_WAY;
    p->setMappingCP();
}

void Normalizer2DataBuilder::setRoundTripMapping(UChar32 c, const UnicodeString &m) {
    if(U_IS_SURROGATE(c)) {
        fprintf(stderr,
                "error in gennorm2 phase %d: "
                "illegal round-trip mapping from surrogate code point U+%04lX\n",
                (int)phase, (long)c);
        exit(U_INVALID_FORMAT_ERROR);
    }
    if(!isWellFormed(m)) {
        fprintf(stderr,
                "error in gennorm2 phase %d: "
                "illegal round-trip mapping from U+%04lX to malformed string\n",
                (int)phase, (long)c);
        exit(U_INVALID_FORMAT_ERROR);
    }
    int32_t numCP=u_countChar32(m.getBuffer(), m.length());
    if(numCP!=2) {
        fprintf(stderr,
                "error in gennorm2 phase %d: "
                "illegal round-trip mapping from U+%04lX to %d!=2 code points\n",
                (int)phase, (long)c, (int)numCP);
        exit(U_INVALID_FORMAT_ERROR);
    }
    Norm *p=checkNormForMapping(createNorm(c), c);
    p->mapping=new UnicodeString(m);
    p->mappingType=Norm::ROUND_TRIP;
    p->mappingCP=U_SENTINEL;
}

void Normalizer2DataBuilder::removeMapping(UChar32 c) {
    Norm *p=checkNormForMapping(getNorm(c), c);
    if(p!=NULL) {
        p->mappingType=Norm::REMOVED;
    }
}

class CompositionBuilder : public Normalizer2DBEnumerator {
public:
    CompositionBuilder(Normalizer2DataBuilder &b) : Normalizer2DBEnumerator(b) {}
    virtual UBool rangeHandler(UChar32 start, UChar32 end, uint32_t value) {
        builder.addComposition(start, end, value);
        return TRUE;
    }
};

void
Normalizer2DataBuilder::addComposition(UChar32 start, UChar32 end, uint32_t value) {
    if(norms[value].mappingType==Norm::ROUND_TRIP) {
        if(start!=end) {
            fprintf(stderr,
                    "gennorm2 error: same round-trip mapping for "
                    "more than 1 code point U+%04lX..U+%04lX\n",
                    (long)start, (long)end);
            exit(U_INVALID_FORMAT_ERROR);
        }
        if(norms[value].cc!=0) {
            fprintf(stderr,
                    "gennorm2 error: "
                    "U+%04lX has a round-trip mapping and ccc!=0, "
                    "not possible in Unicode normalization\n",
                    (long)start);
            exit(U_INVALID_FORMAT_ERROR);
        }
        // setRoundTripMapping() ensured that there are exactly two code points.
        const UnicodeString &m=*norms[value].mapping;
        UChar32 lead=m.char32At(0);
        UChar32 trail=m.char32At(m.length()-1);
        if(getCC(lead)!=0) {
            fprintf(stderr,
                    "gennorm2 error: "
                    "U+%04lX's round-trip mapping's starter U+%04lX has ccc!=0, "
                    "not possible in Unicode normalization\n",
                    (long)start, (long)lead);
            exit(U_INVALID_FORMAT_ERROR);
        }
        // Flag for trailing character.
        createNorm(trail)->combinesBack=TRUE;
        // Insert (trail, composite) pair into compositions list for the lead character.
        IcuToolErrorCode errorCode("gennorm2/addComposition()");
        Norm *leadNorm=createNorm(lead);
        UVector32 *compositions=leadNorm->compositions;
        int32_t i;
        if(compositions==NULL) {
            compositions=leadNorm->compositions=new UVector32(errorCode);
            i=0;  // "insert" the first pair at index 0
        } else {
            // Insertion sort, and check for duplicate trail characters.
            int32_t length;
            const CompositionPair *pairs=leadNorm->getCompositionPairs(length);
            for(i=0; i<length; ++i) {
                if(trail==pairs[i].trail) {
                    fprintf(stderr,
                            "gennorm2 error: same round-trip mapping for "
                            "more than 1 code point (e.g., U+%04lX) to U+%04lX + U+%04lX\n",
                            (long)start, (long)lead, (long)trail);
                    exit(U_INVALID_FORMAT_ERROR);
                }
                if(trail<pairs[i].trail) {
                    break;
                }
            }
        }
        compositions->insertElementAt(trail, 2*i, errorCode);
        compositions->insertElementAt(start, 2*i+1, errorCode);
    }
}

UBool Normalizer2DataBuilder::combinesWithCCBetween(const Norm &norm,
                                                    uint8_t lowCC, uint8_t highCC) const {
    if((highCC-lowCC)>=2) {
        int32_t length;
        const CompositionPair *pairs=norm.getCompositionPairs(length);
        for(int32_t i=0; i<length; ++i) {
            uint8_t trailCC=getCC(pairs[i].trail);
            if(lowCC<trailCC && trailCC<highCC) {
                return TRUE;
            }
        }
    }
    return FALSE;
}

UChar32 Normalizer2DataBuilder::combine(const Norm &norm, UChar32 trail) const {
    int32_t length;
    const CompositionPair *pairs=norm.getCompositionPairs(length);
    for(int32_t i=0; i<length; ++i) {
        if(trail==pairs[i].trail) {
            return pairs[i].composite;
        }
        if(trail<pairs[i].trail) {
            break;
        }
    }
    return U_SENTINEL;
}

class Decomposer : public Normalizer2DBEnumerator {
public:
    Decomposer(Normalizer2DataBuilder &b) : Normalizer2DBEnumerator(b), didDecompose(FALSE) {}
    virtual UBool rangeHandler(UChar32 start, UChar32 end, uint32_t value) {
        didDecompose|=builder.decompose(start, end, value);
        return TRUE;
    }
    UBool didDecompose;
};

UBool
Normalizer2DataBuilder::decompose(UChar32 start, UChar32 end, uint32_t value) {
    if(norms[value].hasMapping()) {
        const UnicodeString &m=*norms[value].mapping;
        UnicodeString *decomposed=NULL;
        const UChar *s=m.getBuffer();
        int32_t length=m.length();
        int32_t prev, i=0;
        UChar32 c;
        while(i<length) {
            prev=i;
            U16_NEXT(s, i, length, c);
            if(start<=c && c<=end) {
                fprintf(stderr,
                        "gennorm2 error: U+%04lX maps to itself directly or indirectly\n",
                        (long)c);
                exit(U_INVALID_FORMAT_ERROR);
            }
            const Norm &cNorm=getNormRef(c);
            if(cNorm.hasMapping()) {
                if(norms[value].mappingType==Norm::ROUND_TRIP) {
                    if(prev==0) {
                        if(cNorm.mappingType!=Norm::ROUND_TRIP) {
                            fprintf(stderr,
                                    "gennorm2 error: "
                                    "U+%04lX's round-trip mapping's starter "
                                    "U+%04lX one-way-decomposes, "
                                    "not possible in Unicode normalization\n",
                                    (long)start, (long)c);
                            exit(U_INVALID_FORMAT_ERROR);
                        }
                        uint8_t myTrailCC=getCC(m.char32At(i));
                        UChar32 cTrailChar=cNorm.mapping->char32At(cNorm.mapping->length()-1);
                        uint8_t cTrailCC=getCC(cTrailChar);
                        if(cTrailCC>myTrailCC) {
                            fprintf(stderr,
                                    "gennorm2 error: "
                                    "U+%04lX's round-trip mapping's starter "
                                    "U+%04lX decomposes and the "
                                    "inner/earlier tccc=%hu > outer/following tccc=%hu, "
                                    "not possible in Unicode normalization\n",
                                    (long)start, (long)c,
                                    (short)cTrailCC, (short)myTrailCC);
                            exit(U_INVALID_FORMAT_ERROR);
                        }
                    } else {
                        fprintf(stderr,
                                "gennorm2 error: "
                                "U+%04lX's round-trip mapping's non-starter "
                                "U+%04lX decomposes, "
                                "not possible in Unicode normalization\n",
                                (long)start, (long)c);
                        exit(U_INVALID_FORMAT_ERROR);
                    }
                }
                if(decomposed==NULL) {
                    decomposed=new UnicodeString(m, 0, prev);
                }
                decomposed->append(*cNorm.mapping);
            } else if(Hangul::isHangul(c)) {
                UChar buffer[3];
                int32_t hangulLength=Hangul::decompose(c, buffer);
                if(norms[value].mappingType==Norm::ROUND_TRIP && prev!=0) {
                    fprintf(stderr,
                            "gennorm2 error: "
                            "U+%04lX's round-trip mapping's non-starter "
                            "U+%04lX decomposes, "
                            "not possible in Unicode normalization\n",
                            (long)start, (long)c);
                    exit(U_INVALID_FORMAT_ERROR);
                }
                if(decomposed==NULL) {
                    decomposed=new UnicodeString(m, 0, prev);
                }
                decomposed->append(buffer, hangulLength);
            } else if(decomposed!=NULL) {
                decomposed->append(m, prev, i-prev);
            }
        }
        if(decomposed!=NULL) {
            delete norms[value].mapping;
            norms[value].mapping=decomposed;
            // Not  norms[value].setMappingCP();  because the original mapping
            // is most likely to be encodable as a delta.
            return TRUE;
        }
    }
    return FALSE;
}

class BuilderReorderingBuffer {
public:
    BuilderReorderingBuffer() : fLength(0), fLastStarterIndex(-1), fDidReorder(FALSE) {}
    void reset() {
        fLength=0;
        fLastStarterIndex=-1;
        fDidReorder=FALSE;
    }
    int32_t length() const { return fLength; }
    UBool isEmpty() const { return fLength==0; }
    int32_t lastStarterIndex() const { return fLastStarterIndex; }
    UChar32 charAt(int32_t i) const { return fArray[i]>>8; }
    uint8_t ccAt(int32_t i) const { return (uint8_t)fArray[i]; }
    UBool didReorder() const { return fDidReorder; }
    void append(UChar32 c, uint8_t cc) {
        if(cc==0 || fLength==0 || ccAt(fLength-1)<=cc) {
            if(cc==0) {
                fLastStarterIndex=fLength;
            }
            fArray[fLength++]=(c<<8)|cc;
            return;
        }
        // Let this character bubble back to its canonical order.
        int32_t i=fLength-1;
        while(i>fLastStarterIndex && ccAt(i)>cc) {
            --i;
        }
        ++i;  // after the last starter or prevCC<=cc
        // Move this and the following characters forward one to make space.
        for(int32_t j=fLength; i<j; --j) {
            fArray[j]=fArray[j-1];
        }
        fArray[i]=(c<<8)|cc;
        ++fLength;
        fDidReorder=TRUE;
    }
    void toString(UnicodeString &dest) {
        dest.remove();
        for(int32_t i=0; i<fLength; ++i) {
            dest.append(charAt(i));
        }
    }
    void setComposite(UChar32 composite, int32_t combMarkIndex) {
        fArray[fLastStarterIndex]=composite<<8;
        // Remove the combining mark that contributed to the composite.
        --fLength;
        while(combMarkIndex<fLength) {
            fArray[combMarkIndex]=fArray[combMarkIndex+1];
            ++combMarkIndex;
        }
    }
private:
    int32_t fArray[Normalizer2Impl::MAPPING_LENGTH_MASK];
    int32_t fLength;
    int32_t fLastStarterIndex;
    UBool fDidReorder;
};

void
Normalizer2DataBuilder::reorder(Norm *p, BuilderReorderingBuffer &buffer) {
    UnicodeString &m=*p->mapping;
    int32_t length=m.length();
    if(length>Normalizer2Impl::MAPPING_LENGTH_MASK) {
        return;  // writeMapping() will complain about it and print the code point.
    }
    const UChar *s=m.getBuffer();
    int32_t i=0;
    UChar32 c;
    while(i<length) {
        U16_NEXT(s, i, length, c);
        buffer.append(c, getCC(c));
    }
    if(buffer.didReorder()) {
        buffer.toString(m);
    }
}

UBool Normalizer2DataBuilder::hasNoCompBoundaryAfter(BuilderReorderingBuffer &buffer) {
    if(buffer.isEmpty()) {
        return TRUE;  // maps-to-empty string is no boundary of any kind
    }
    int32_t lastStarterIndex=buffer.lastStarterIndex();
    if(lastStarterIndex<0) {
        return TRUE;  // no starter
    }
    UChar32 starter=buffer.charAt(lastStarterIndex);
    if( Hangul::isJamoL(starter) ||
        (Hangul::isJamoV(starter) &&
         0<lastStarterIndex && Hangul::isJamoL(buffer.charAt(lastStarterIndex-1)))
    ) {
        // A Jamo leading consonant or an LV pair combines-forward if it is at the end,
        // otherwise it is blocked.
        return lastStarterIndex==buffer.length()-1;
    }
    // no Hangul in fully decomposed mapping
    const Norm *starterNorm=&getNormRef(starter);
    if(starterNorm->compositions==NULL) {
        return FALSE;  // the last starter does not combine forward
    }
    // Compose as far as possible, and see if further compositions are possible.
    uint8_t prevCC=0;
    for(int32_t combMarkIndex=lastStarterIndex+1; combMarkIndex<buffer.length();) {
        uint8_t cc=buffer.ccAt(combMarkIndex);  // !=0 because after last starter
        if(combinesWithCCBetween(*starterNorm, prevCC, cc)) {
            return TRUE;
        }
        if( prevCC<cc &&
            (starter=combine(*starterNorm, buffer.charAt(combMarkIndex)))>=0
        ) {
            buffer.setComposite(starter, combMarkIndex);
            starterNorm=&getNormRef(starter);
            if(starterNorm->compositions==NULL) {
                return FALSE;  // the composite does not combine further
            }
        } else {
            prevCC=cc;
            ++combMarkIndex;
        }
    }
    // TRUE if the final, forward-combining starter is at the end.
    return prevCC==0;
}

// Requires p->hasMapping().
void Normalizer2DataBuilder::writeMapping(UChar32 c, const Norm *p, UnicodeString &dataString) {
    UnicodeString &m=*p->mapping;
    int32_t length=m.length();
    if(length>Normalizer2Impl::MAPPING_LENGTH_MASK) {
        fprintf(stderr,
                "gennorm2 error: "
                "mapping for U+%04lX longer than maximum of %d\n",
                (long)c, Normalizer2Impl::MAPPING_LENGTH_MASK);
        exit(U_INVALID_FORMAT_ERROR);
    }
    int32_t leadCC, trailCC;
    if(length==0) {
        leadCC=trailCC=0;
    } else {
        leadCC=getCC(m.char32At(0));
        trailCC=getCC(m.char32At(length-1));
    }
    if(c<Normalizer2Impl::MIN_CCC_LCCC_CP && (p->cc!=0 || leadCC!=0)) {
        fprintf(stderr,
                "gennorm2 error: "
                "U+%04lX below U+0300 has ccc!=0 or lccc!=0, not supported by ICU\n",
                (long)c);
        exit(U_INVALID_FORMAT_ERROR);
    }
    int32_t firstUnit=length|(trailCC<<8);
    int32_t secondUnit=p->cc|(leadCC<<8);
    if(secondUnit!=0) {
        firstUnit|=Normalizer2Impl::MAPPING_HAS_CCC_LCCC_WORD;
    }
    if(p->compositions!=NULL) {
        firstUnit|=Normalizer2Impl::MAPPING_PLUS_COMPOSITION_LIST;
    }
    if(p->hasNoCompBoundaryAfter) {
        firstUnit|=Normalizer2Impl::MAPPING_NO_COMP_BOUNDARY_AFTER;
    }
    dataString.append((UChar)firstUnit);
    if(secondUnit!=0) {
        dataString.append((UChar)secondUnit);
    }
    dataString.append(m);
}

// Requires p->compositions!=NULL.
void Normalizer2DataBuilder::writeCompositions(UChar32 c, const Norm *p, UnicodeString &dataString) {
    if(p->cc!=0) {
        fprintf(stderr,
                "gennorm2 error: "
                "U+%04lX combines-forward and has ccc!=0, not possible in Unicode normalization\n",
                (long)c);
        exit(U_INVALID_FORMAT_ERROR);
    }
    int32_t length;
    const CompositionPair *pairs=p->getCompositionPairs(length);
    for(int32_t i=0; i<length; ++i) {
        const CompositionPair &pair=pairs[i];
        // 22 bits for the composite character and whether it combines forward.
        UChar32 compositeAndFwd=pair.composite<<1;
        if(getNormRef(pair.composite).compositions!=NULL) {
            compositeAndFwd|=1;  // The composite character also combines-forward.
        }
        // Encode most pairs in two units and some in three.
        int32_t firstUnit, secondUnit, thirdUnit;
        if(pair.trail<Normalizer2Impl::COMP_1_TRAIL_LIMIT) {
            if(compositeAndFwd<=0xffff) {
                firstUnit=pair.trail<<1;
                secondUnit=compositeAndFwd;
                thirdUnit=-1;
            } else {
                firstUnit=(pair.trail<<1)|Normalizer2Impl::COMP_1_TRIPLE;
                secondUnit=compositeAndFwd>>16;
                thirdUnit=compositeAndFwd;
            }
        } else {
            firstUnit=(Normalizer2Impl::COMP_1_TRAIL_LIMIT+
                       (pair.trail>>Normalizer2Impl::COMP_1_TRAIL_SHIFT))|
                      Normalizer2Impl::COMP_1_TRIPLE;
            secondUnit=(pair.trail<<Normalizer2Impl::COMP_2_TRAIL_SHIFT)|
                       (compositeAndFwd>>16);
            thirdUnit=compositeAndFwd;
        }
        // Set the high bit of the first unit if this is the last composition pair.
        if(i==(length-1)) {
            firstUnit|=Normalizer2Impl::COMP_1_LAST_TUPLE;
        }
        dataString.append((UChar)firstUnit).append((UChar)secondUnit);
        if(thirdUnit>=0) {
            dataString.append((UChar)thirdUnit);
        }
    }
}

class ExtraDataWriter : public Normalizer2DBEnumerator {
public:
    ExtraDataWriter(Normalizer2DataBuilder &b) :
        Normalizer2DBEnumerator(b),
        yesYesCompositions(1000, (UChar32)0xffff, 2),  // 0=inert, 1=Jamo L, 2=start of compositions
        yesNoData(1000, (UChar32)0, 1) {}  // 0=Hangul, 1=start of normal data
    virtual UBool rangeHandler(UChar32 start, UChar32 end, uint32_t value) {
        if(value!=0) {
            if(start!=end) {
                fprintf(stderr,
                        "gennorm2 error: unexpected shared data for "
                        "multiple code points U+%04lX..U+%04lX\n",
                        (long)start, (long)end);
                exit(U_INTERNAL_PROGRAM_ERROR);
            }
            builder.writeExtraData(start, value, *this);
        }
        return TRUE;
    }
    UnicodeString maybeYesCompositions;
    UnicodeString yesYesCompositions;
    UnicodeString yesNoData;
    UnicodeString noNoMappings;
    Hashtable previousNoNoMappings;  // If constructed in runtime code, pass in UErrorCode.
};

void Normalizer2DataBuilder::writeExtraData(UChar32 c, uint32_t value, ExtraDataWriter &writer) {
    Norm *p=norms+value;
    if(p->combinesBack) {
        if(p->hasMapping()) {
            fprintf(stderr,
                    "gennorm2 error: "
                    "U+%04lX combines-back and decomposes, not possible in Unicode normalization\n",
                    (long)c);
            exit(U_INVALID_FORMAT_ERROR);
        }
        if(p->compositions!=NULL) {
            p->offset=
                (writer.maybeYesCompositions.length()<<Norm::OFFSET_SHIFT)|
                Norm::OFFSET_MAYBE_YES;
            writeCompositions(c, p, writer.maybeYesCompositions);
        }
    } else if(!p->hasMapping()) {
        if(p->compositions!=NULL) {
            p->offset=
                (writer.yesYesCompositions.length()<<Norm::OFFSET_SHIFT)|
                Norm::OFFSET_YES_YES;
            writeCompositions(c, p, writer.yesYesCompositions);
        }
    } else if(p->mappingType==Norm::ROUND_TRIP) {
        p->offset=
            (writer.yesNoData.length()<<Norm::OFFSET_SHIFT)|
            Norm::OFFSET_YES_NO;
        writeMapping(c, p, writer.yesNoData);
        if(p->compositions!=NULL) {
            writeCompositions(c, p, writer.yesNoData);
        }
    } else /* one-way */ {
        if(p->compositions!=NULL) {
            fprintf(stderr,
                    "gennorm2 error: "
                    "U+%04lX combines-forward and has a one-way mapping, "
                    "not possible in Unicode normalization\n",
                    (long)c);
            exit(U_INVALID_FORMAT_ERROR);
        }
        if(p->cc==0 && optimization!=OPTIMIZE_FAST) {
            // Try a compact, algorithmic encoding.
            // Only for ccc=0, because we can't store additional information.
            if(p->mappingCP>=0) {
                int32_t delta=p->mappingCP-c;
                if(-Normalizer2Impl::MAX_DELTA<=delta && delta<=Normalizer2Impl::MAX_DELTA) {
                    p->offset=(delta<<Norm::OFFSET_SHIFT)|Norm::OFFSET_DELTA;
                }
            }
        }
        if(p->offset==0) {
            int32_t oldNoNoLength=writer.noNoMappings.length();
            writeMapping(c, p, writer.noNoMappings);
            UnicodeString newMapping=writer.noNoMappings.tempSubString(oldNoNoLength);
            int32_t previousOffset=writer.previousNoNoMappings.geti(newMapping);
            if(previousOffset!=0) {
                // Duplicate, remove the new units and point to the old ones.
                writer.noNoMappings.truncate(oldNoNoLength);
                p->offset=
                    ((previousOffset-1)<<Norm::OFFSET_SHIFT)|
                    Norm::OFFSET_NO_NO;
            } else {
                // Enter this new mapping into the hashtable, avoiding value 0 which is "not found".
                IcuToolErrorCode errorCode("gennorm2/writeExtraData()/Hashtable.puti()");
                writer.previousNoNoMappings.puti(newMapping, oldNoNoLength+1, errorCode);
                p->offset=
                    (oldNoNoLength<<Norm::OFFSET_SHIFT)|
                    Norm::OFFSET_NO_NO;
            }
        }
    }
}

class Norm16Writer : public Normalizer2DBEnumerator {
public:
    Norm16Writer(Normalizer2DataBuilder &b) : Normalizer2DBEnumerator(b) {}
    virtual UBool rangeHandler(UChar32 start, UChar32 end, uint32_t value) {
        builder.writeNorm16(start, end, value);
        return TRUE;
    }
};

void Normalizer2DataBuilder::writeNorm16(UChar32 start, UChar32 end, uint32_t value) {
    if(value!=0) {
        const Norm *p=norms+value;
        int32_t offset=p->offset>>Norm::OFFSET_SHIFT;
        int32_t norm16=0;
        UBool isDecompNo=FALSE;
        UBool isCompNoMaybe=FALSE;
        switch(p->offset&Norm::OFFSET_MASK) {
        case Norm::OFFSET_NONE:
            // No mapping, no compositions list.
            if(p->combinesBack) {
                norm16=Normalizer2Impl::MIN_NORMAL_MAYBE_YES+p->cc;
                isDecompNo=(UBool)(p->cc!=0);
                isCompNoMaybe=TRUE;
            } else if(p->cc!=0) {
                norm16=Normalizer2Impl::MIN_YES_YES_WITH_CC-1+p->cc;
                isDecompNo=isCompNoMaybe=TRUE;
            }
            break;
        case Norm::OFFSET_MAYBE_YES:
            norm16=indexes[Normalizer2Impl::IX_MIN_MAYBE_YES]+offset;
            isCompNoMaybe=TRUE;
            break;
        case Norm::OFFSET_YES_YES:
            norm16=offset;
            break;
        case Norm::OFFSET_YES_NO:
            norm16=indexes[Normalizer2Impl::IX_MIN_YES_NO]+offset;
            isDecompNo=TRUE;
            break;
        case Norm::OFFSET_NO_NO:
            norm16=indexes[Normalizer2Impl::IX_MIN_NO_NO]+offset;
            isDecompNo=isCompNoMaybe=TRUE;
            break;
        case Norm::OFFSET_DELTA:
            norm16=getCenterNoNoDelta()+offset;
            isDecompNo=isCompNoMaybe=TRUE;
            break;
        default:  // Should not occur.
            exit(U_INTERNAL_PROGRAM_ERROR);
        }
        IcuToolErrorCode errorCode("gennorm2/writeNorm16()");
        utrie2_setRange32(norm16Trie, start, end, (uint32_t)norm16, TRUE, errorCode);
        if(isDecompNo && start<indexes[Normalizer2Impl::IX_MIN_DECOMP_NO_CP]) {
            indexes[Normalizer2Impl::IX_MIN_DECOMP_NO_CP]=start;
        }
        if(isCompNoMaybe && start<indexes[Normalizer2Impl::IX_MIN_COMP_NO_MAYBE_CP]) {
            indexes[Normalizer2Impl::IX_MIN_COMP_NO_MAYBE_CP]=start;
        }
    }
}

void Normalizer2DataBuilder::setHangulData() {
    HangulIterator hi;
    const HangulIterator::Range *range;
    // Check that none of the Hangul/Jamo code points have data.
    while((range=hi.nextRange())!=NULL) {
        for(UChar32 c=range->start; c<range->limit; ++c) {
            if(utrie2_get32(norm16Trie, c)!=0) {
                fprintf(stderr,
                        "gennorm2 error: "
                        "illegal mapping/composition/ccc data for Hangul or Jamo U+%04lX\n",
                        (long)c);
                exit(U_INVALID_FORMAT_ERROR);
            }
        }
    }
    // Set data for algorithmic runtime handling.
    IcuToolErrorCode errorCode("gennorm2/setHangulData()");
    hi.reset();
    while((range=hi.nextRange())!=NULL) {
        uint16_t norm16=range->norm16;
        if(norm16==0) {
            norm16=(uint16_t)indexes[Normalizer2Impl::IX_MIN_YES_NO];  // Hangul LV/LVT encoded as minYesNo
            if(range->start<indexes[Normalizer2Impl::IX_MIN_DECOMP_NO_CP]) {
                indexes[Normalizer2Impl::IX_MIN_DECOMP_NO_CP]=range->start;
            }
        } else {
            if(range->start<indexes[Normalizer2Impl::IX_MIN_COMP_NO_MAYBE_CP]) {  // Jamo V/T are maybeYes
                indexes[Normalizer2Impl::IX_MIN_COMP_NO_MAYBE_CP]=range->start;
            }
        }
        utrie2_setRange32(norm16Trie, range->start, range->limit-1, norm16, TRUE, errorCode);
        errorCode.assertSuccess();
    }
}

U_CDECL_BEGIN

static UBool U_CALLCONV
enumRangeMaxValue(const void *context, UChar32 /*start*/, UChar32 /*end*/, uint32_t value) {
    uint32_t *pMaxValue=(uint32_t *)context;
    if(value>*pMaxValue) {
        *pMaxValue=value;
    }
    return TRUE;
}

U_CDECL_END

void Normalizer2DataBuilder::processData() {
    IcuToolErrorCode errorCode("gennorm2/processData()");
    norm16Trie=utrie2_open(0, 0, errorCode);
    errorCode.assertSuccess();

    utrie2_enum(normTrie, NULL, enumRangeHandler, CompositionBuilder(*this).ptr());

    Decomposer decomposer(*this);
    do {
        decomposer.didDecompose=FALSE;
        utrie2_enum(normTrie, NULL, enumRangeHandler, &decomposer);
    } while(decomposer.didDecompose);

    BuilderReorderingBuffer buffer;
    int32_t normsLength=utm_countItems(normMem);
    for(int32_t i=1; i<normsLength; ++i) {
        if(norms[i].hasMapping()) {
            buffer.reset();
            reorder(norms+i, buffer);
            norms[i].hasNoCompBoundaryAfter=hasNoCompBoundaryAfter(buffer);
        }
    }

    indexes[Normalizer2Impl::IX_MIN_DECOMP_NO_CP]=0x110000;
    indexes[Normalizer2Impl::IX_MIN_COMP_NO_MAYBE_CP]=0x110000;

    ExtraDataWriter extraDataWriter(*this);
    utrie2_enum(normTrie, NULL, enumRangeHandler, &extraDataWriter);

    extraData=extraDataWriter.maybeYesCompositions;
    extraData.append(extraDataWriter.yesYesCompositions).
              append(extraDataWriter.yesNoData).
              append(extraDataWriter.noNoMappings);
    // Pad to even length for 4-byte alignment of following data.
    if(extraData.length()&1) {
        extraData.append((UChar)0);
    }

    indexes[Normalizer2Impl::IX_MIN_YES_NO]=
        extraDataWriter.yesYesCompositions.length();
    indexes[Normalizer2Impl::IX_MIN_NO_NO]=
        indexes[Normalizer2Impl::IX_MIN_YES_NO]+
        extraDataWriter.yesNoData.length();
    indexes[Normalizer2Impl::IX_LIMIT_NO_NO]=
        indexes[Normalizer2Impl::IX_MIN_NO_NO]+
        extraDataWriter.noNoMappings.length();
    indexes[Normalizer2Impl::IX_MIN_MAYBE_YES]=
        Normalizer2Impl::MIN_NORMAL_MAYBE_YES-
        extraDataWriter.maybeYesCompositions.length();

    int32_t minNoNoDelta=getCenterNoNoDelta()-Normalizer2Impl::MAX_DELTA;
    if(indexes[Normalizer2Impl::IX_LIMIT_NO_NO]>minNoNoDelta) {
        fprintf(stderr,
                "gennorm2 error: "
                "data structure overflow, too much mapping composition data\n");
        exit(U_BUFFER_OVERFLOW_ERROR);
    }

    utrie2_enum(normTrie, NULL, enumRangeHandler, Norm16Writer(*this).ptr());

    setHangulData();

    // Look for the "worst" norm16 value of any supplementary code point
    // corresponding to a lead surrogate, and set it as that surrogate's value.
    // Enables quick check inner loops to look at only code units.
    //
    // We could be more sophisticated:
    // We could collect a bit set for whether there are values in the different
    // norm16 ranges (yesNo, maybeYes, yesYesWithCC etc.)
    // and select the best value that only breaks the composition and/or decomposition
    // inner loops if necessary.
    // However, that seems like overkill for an optimization for supplementary characters.
    for(UChar lead=0xd800; lead<0xdc00; ++lead) {
        uint32_t maxValue=utrie2_get32(norm16Trie, lead);
        utrie2_enumForLeadSurrogate(norm16Trie, lead, NULL, enumRangeMaxValue, &maxValue);
        if( maxValue>=(uint32_t)indexes[Normalizer2Impl::IX_LIMIT_NO_NO] &&
            maxValue>(uint32_t)indexes[Normalizer2Impl::IX_MIN_NO_NO]
        ) {
            // Set noNo ("worst" value) if it got into "less-bad" maybeYes or ccc!=0.
            // Otherwise it might end up at something like JAMO_VT which stays in
            // the inner decomposition quick check loop.
            maxValue=(uint32_t)indexes[Normalizer2Impl::IX_LIMIT_NO_NO]-1;
        }
        utrie2_set32ForLeadSurrogateCodeUnit(norm16Trie, lead, maxValue, errorCode);
    }

    // Adjust supplementary minimum code points to break quick check loops at their lead surrogates.
    // For an empty data file, minCP=0x110000 turns into 0xdc00 (first trail surrogate)
    // which is harmless.
    // As a result, the minimum code points are always BMP code points.
    int32_t minCP=indexes[Normalizer2Impl::IX_MIN_DECOMP_NO_CP];
    if(minCP>=0x10000) {
        indexes[Normalizer2Impl::IX_MIN_DECOMP_NO_CP]=U16_LEAD(minCP);
    }
    minCP=indexes[Normalizer2Impl::IX_MIN_COMP_NO_MAYBE_CP];
    if(minCP>=0x10000) {
        indexes[Normalizer2Impl::IX_MIN_COMP_NO_MAYBE_CP]=U16_LEAD(minCP);
    }
}

void Normalizer2DataBuilder::writeBinaryFile(const char *filename) {
    processData();

    IcuToolErrorCode errorCode("gennorm2/writeBinaryFile()");
    utrie2_freeze(norm16Trie, UTRIE2_16_VALUE_BITS, errorCode);
    int32_t norm16TrieLength=utrie2_serialize(norm16Trie, NULL, 0, errorCode);
    if(errorCode.get()!=U_BUFFER_OVERFLOW_ERROR) {
        fprintf(stderr, "gennorm2 error: unable to freeze/serialize the normalization trie - %s\n",
                errorCode.errorName());
        exit(errorCode.reset());
    }
    errorCode.reset();
    LocalArray<uint8_t> norm16TrieBytes(new uint8_t[norm16TrieLength]);
    utrie2_serialize(norm16Trie, norm16TrieBytes.getAlias(), norm16TrieLength, errorCode);
    errorCode.assertSuccess();

    int32_t offset=(int32_t)sizeof(indexes);
    indexes[Normalizer2Impl::IX_NORM_TRIE_OFFSET]=offset;
    offset+=norm16TrieLength;
    indexes[Normalizer2Impl::IX_EXTRA_DATA_OFFSET]=offset;
    int32_t totalSize=offset+=extraData.length()*2;
    for(int32_t i=Normalizer2Impl::IX_RESERVED2_OFFSET; i<=Normalizer2Impl::IX_TOTAL_SIZE; ++i) {
        indexes[i]=totalSize;
    }

    if(beVerbose) {
        printf("size of normalization trie:         %5ld bytes\n", (long)norm16TrieLength);
        printf("size of 16-bit extra data:          %5ld uint16_t\n", (long)extraData.length());
        printf("size of binary data file contents:  %5ld bytes\n", (long)totalSize);
        printf("minDecompNoCodePoint:              U+%04lX\n", (long)indexes[Normalizer2Impl::IX_MIN_DECOMP_NO_CP]);
        printf("minCompNoMaybeCodePoint:           U+%04lX\n", (long)indexes[Normalizer2Impl::IX_MIN_COMP_NO_MAYBE_CP]);
        printf("minYesNo:                          0x%04x\n", (int)indexes[Normalizer2Impl::IX_MIN_YES_NO]);
        printf("minNoNo:                           0x%04x\n", (int)indexes[Normalizer2Impl::IX_MIN_NO_NO]);
        printf("limitNoNo:                         0x%04x\n", (int)indexes[Normalizer2Impl::IX_LIMIT_NO_NO]);
        printf("minMaybeYes:                       0x%04x\n", (int)indexes[Normalizer2Impl::IX_MIN_MAYBE_YES]);
    }

    memcpy(dataInfo.dataVersion, unicodeVersion, 4);
    UNewDataMemory *pData=
        udata_create(NULL, NULL, filename, &dataInfo,
                     haveCopyright ? U_COPYRIGHT_STRING : NULL, errorCode);
    if(errorCode.isFailure()) {
        fprintf(stderr, "gennorm2 error: unable to create the output file %s - %s\n",
                filename, errorCode.errorName());
        exit(errorCode.reset());
    }
    udata_writeBlock(pData, indexes, sizeof(indexes));
    udata_writeBlock(pData, norm16TrieBytes.getAlias(), norm16TrieLength);
    udata_writeUString(pData, extraData.getBuffer(), extraData.length());

    int32_t writtenSize=udata_finish(pData, errorCode);
    if(errorCode.isFailure()) {
        fprintf(stderr, "gennorm2: error %s writing the output file\n", errorCode.errorName());
        exit(errorCode.reset());
    }
    if(writtenSize!=totalSize) {
        fprintf(stderr, "gennorm2 error: written size %ld != calculated size %ld\n",
            (long)writtenSize, (long)totalSize);
        exit(U_INTERNAL_PROGRAM_ERROR);
    }
}

U_NAMESPACE_END

#endif /* #if !UCONFIG_NO_NORMALIZATION */

/*
 * Hey, Emacs, please set the following:
 *
 * Local Variables:
 * indent-tabs-mode: nil
 * End:
 */
