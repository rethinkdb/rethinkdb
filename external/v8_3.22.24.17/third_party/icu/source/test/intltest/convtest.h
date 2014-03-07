/*
 *******************************************************************************
 *
 *   Copyright (C) 2003-2007, International Business Machines
 *   Corporation and others.  All Rights Reserved.
 *
 *******************************************************************************
 *   file name:  convtest.h
 *   encoding:   US-ASCII
 *   tab size:   8 (not used)
 *   indentation:4
 *
 *   created on: 2003jul15
 *   created by: Markus W. Scherer
 *
 *   Test file for data-driven conversion tests.
 */

#ifndef __CONVTEST_H__
#define __CONVTEST_H__

#include "unicode/utypes.h"

#if !UCONFIG_NO_LEGACY_CONVERSION

#include "unicode/ucnv.h"
#include "intltest.h"

struct ConversionCase {
    /* setup */
    int32_t caseNr;
    const char *charset, *cbopt, *name;
    UChar subString[16];
    char subchar[8];
    int8_t setSub;

    /* input and expected output */
    const uint8_t *bytes;
    int32_t bytesLength;
    const UChar *unicode;
    int32_t unicodeLength;
    const int32_t *offsets;

    /* UTF-8 version of unicode[unicodeLength] */
    const char *utf8;
    int32_t utf8Length;

    /* options */
    UBool finalFlush;
    UBool fallbacks;
    UErrorCode outErrorCode;
    const uint8_t *invalidChars;
    const UChar *invalidUChars;
    int32_t invalidLength;

    /* actual output */
    uint8_t resultBytes[200];
    UChar resultUnicode[200];
    int32_t resultOffsets[200];
    int32_t resultLength;

    UErrorCode resultErrorCode;
};

class ConversionTest : public IntlTest {
public:
    ConversionTest();
    virtual ~ConversionTest();
    
    void runIndexedTest(int32_t index, UBool exec, const char *&name, char *par=0);

    void TestToUnicode();
    void TestFromUnicode();
    void TestGetUnicodeSet();
    void TestGetUnicodeSet2();

private:
    UBool
    ToUnicodeCase(ConversionCase &cc, UConverterToUCallback callback, const char *option);

    UBool
    FromUnicodeCase(ConversionCase &cc, UConverterFromUCallback callback, const char *option);

    UBool
    checkToUnicode(ConversionCase &cc, UConverter *cnv, const char *name,
                   const UChar *result, int32_t resultLength,
                   const int32_t *resultOffsets,
                   UErrorCode resultErrorCode);

    UBool
    checkFromUnicode(ConversionCase &cc, UConverter *cnv, const char *name,
                     const uint8_t *result, int32_t resultLength,
                     const int32_t *resultOffsets,
                     UErrorCode resultErrorCode);

    UConverter *
    cnv_open(const char *name, UErrorCode &errorCode);

    /* for testing direct UTF-8 conversion */
    UConverter *utf8Cnv;
};

#endif /* #if !UCONFIG_NO_LEGACY_CONVERSION */

#endif
