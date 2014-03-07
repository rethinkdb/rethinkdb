/*
*******************************************************************************
*
*   Copyright (C) 2009-2010, International Business Machines
*   Corporation and others.  All Rights Reserved.
*
*******************************************************************************
*   file name:  n2builder.h
*   encoding:   US-ASCII
*   tab size:   8 (not used)
*   indentation:4
*
*   created on: 2009nov25
*   created by: Markus W. Scherer
*/

#ifndef __N2BUILDER_H__
#define __N2BUILDER_H__

#include "unicode/utypes.h"

#if !UCONFIG_NO_NORMALIZATION

#include "unicode/errorcode.h"
#include "unicode/unistr.h"
#include "normalizer2impl.h"  // for IX_COUNT
#include "toolutil.h"
#include "utrie2.h"

U_NAMESPACE_BEGIN

extern UBool beVerbose, haveCopyright;

struct Norm;

class BuilderReorderingBuffer;
class ExtraDataWriter;

class Normalizer2DataBuilder {
public:
    Normalizer2DataBuilder(UErrorCode &errorCode);
    ~Normalizer2DataBuilder();

    enum OverrideHandling {
        OVERRIDE_NONE,
        OVERRIDE_ANY,
        OVERRIDE_PREVIOUS
    };

    void setOverrideHandling(OverrideHandling oh);

    enum Optimization {
        OPTIMIZE_NORMAL,
        OPTIMIZE_FAST
    };

    void setOptimization(Optimization opt) { optimization=opt; }

    void setCC(UChar32 c, uint8_t cc);
    void setOneWayMapping(UChar32 c, const UnicodeString &m);
    void setRoundTripMapping(UChar32 c, const UnicodeString &m);
    void removeMapping(UChar32 c);

    void setUnicodeVersion(const char *v);

    void writeBinaryFile(const char *filename);

private:
    friend class CompositionBuilder;
    friend class Decomposer;
    friend class ExtraDataWriter;
    friend class Norm16Writer;

    // No copy constructor nor assignment operator.
    Normalizer2DataBuilder(const Normalizer2DataBuilder &other);
    Normalizer2DataBuilder &operator=(const Normalizer2DataBuilder &other);

    Norm *allocNorm();
    Norm *getNorm(UChar32 c);
    Norm *createNorm(UChar32 c);
    Norm *checkNormForMapping(Norm *p, UChar32 c);  // check for permitted overrides

    const Norm &getNormRef(UChar32 c) const;
    uint8_t getCC(UChar32 c) const;
    UBool combinesWithCCBetween(const Norm &norm, uint8_t lowCC, uint8_t highCC) const;
    UChar32 combine(const Norm &norm, UChar32 trail) const;

    void addComposition(UChar32 start, UChar32 end, uint32_t value);
    UBool decompose(UChar32 start, UChar32 end, uint32_t value);
    void reorder(Norm *p, BuilderReorderingBuffer &buffer);
    UBool hasNoCompBoundaryAfter(BuilderReorderingBuffer &buffer);
    void setHangulData();
    void writeMapping(UChar32 c, const Norm *p, UnicodeString &dataString);
    void writeCompositions(UChar32 c, const Norm *p, UnicodeString &dataString);
    void writeExtraData(UChar32 c, uint32_t value, ExtraDataWriter &writer);
    int32_t getCenterNoNoDelta() {
        return indexes[Normalizer2Impl::IX_MIN_MAYBE_YES]-Normalizer2Impl::MAX_DELTA-1;
    }
    void writeNorm16(UChar32 start, UChar32 end, uint32_t value);
    void processData();

    UTrie2 *normTrie;
    UToolMemory *normMem;
    Norm *norms;

    int32_t phase;
    OverrideHandling overrideHandling;

    Optimization optimization;

    int32_t indexes[Normalizer2Impl::IX_COUNT];
    UTrie2 *norm16Trie;
    UnicodeString extraData;

    UVersionInfo unicodeVersion;
};

U_NAMESPACE_END

#endif // #if !UCONFIG_NO_NORMALIZATION

#endif  // __N2BUILDER_H__
