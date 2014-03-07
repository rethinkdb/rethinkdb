/*
 **********************************************************************
 *   Copyright (C) 2005-2006, International Business Machines
 *   Corporation and others.  All Rights Reserved.
 **********************************************************************
 */

#ifndef __CSDETECT_H
#define __CSDETECT_H

#include "unicode/uobject.h"

#if !UCONFIG_NO_CONVERSION

U_NAMESPACE_BEGIN

class InputText;
class CharsetRecognizer;
class CharsetMatch;

class CharsetDetector : public UMemory
{
private:
    InputText *textIn;
    CharsetMatch **resultArray;
    int32_t resultCount;
    UBool fStripTags;   // If true, setText() will strip tags from input text.
    UBool fFreshTextSet;
    static void setRecognizers(UErrorCode &status);

public:
    CharsetDetector(UErrorCode &status);

    ~CharsetDetector();

    void setText(const char *in, int32_t len);

    const CharsetMatch * const *detectAll(int32_t &maxMatchesFound, UErrorCode &status);

    const CharsetMatch *detect(UErrorCode& status);

    void setDeclaredEncoding(const char *encoding, int32_t len) const;

    UBool setStripTagsFlag(UBool flag);

    UBool getStripTagsFlag() const;

//    const char *getCharsetName(int32_t index, UErrorCode& status) const;

    static int32_t getDetectableCount(); 
};

U_NAMESPACE_END

#endif
#endif /* __CSDETECT_H */
