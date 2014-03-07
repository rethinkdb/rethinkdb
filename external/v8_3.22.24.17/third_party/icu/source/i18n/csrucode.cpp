/*
 **********************************************************************
 *   Copyright (C) 2005-2006, International Business Machines
 *   Corporation and others.  All Rights Reserved.
 **********************************************************************
 */

#include "unicode/utypes.h"

#if !UCONFIG_NO_CONVERSION

#include "csrucode.h"

U_NAMESPACE_BEGIN

CharsetRecog_Unicode::~CharsetRecog_Unicode()
{
    // nothing to do
}

CharsetRecog_UTF_16_BE::~CharsetRecog_UTF_16_BE()
{
    // nothing to do
}

const char *CharsetRecog_UTF_16_BE::getName() const
{
    return "UTF-16BE";
}

int32_t CharsetRecog_UTF_16_BE::match(InputText* textIn)
{
    const uint8_t *input = textIn->fRawInput;

    if (input[0] == 0xFE && input[1] == 0xFF) {
        return 100;
    }

    // TODO: Do some statastics to check for unsigned UTF-16BE
    return 0;
}

CharsetRecog_UTF_16_LE::~CharsetRecog_UTF_16_LE()
{
    // nothing to do
}

const char *CharsetRecog_UTF_16_LE::getName() const
{
    return "UTF-16LE";
}

int32_t CharsetRecog_UTF_16_LE::match(InputText* textIn)
{
    const uint8_t *input = textIn->fRawInput;

    if (input[0] == 0xFF && input[1] == 0xFE && (input[2] != 0x00 || input[3] != 0x00)) {
        return 100;
    }

    // TODO: Do some statastics to check for unsigned UTF-16LE
    return 0;
}

CharsetRecog_UTF_32::~CharsetRecog_UTF_32()
{
    // nothing to do
}

int32_t CharsetRecog_UTF_32::match(InputText* textIn)
{
    const uint8_t *input = textIn->fRawInput;
    int32_t limit = (textIn->fRawLength / 4) * 4;
    int32_t numValid = 0;
    int32_t numInvalid = 0;
    bool hasBOM = FALSE;
    int32_t confidence = 0;

    if (getChar(input, 0) == 0x0000FEFFUL) {
        hasBOM = TRUE;
    }

    for(int32_t i = 0; i < limit; i += 4) {
        int32_t ch = getChar(input, i);

        if (ch < 0 || ch >= 0x10FFFF || (ch >= 0xD800 && ch <= 0xDFFF)) {
            numInvalid += 1;
        } else {
            numValid += 1;
        }
    }


    // Cook up some sort of confidence score, based on presense of a BOM
    //    and the existence of valid and/or invalid multi-byte sequences.
    if (hasBOM && numInvalid==0) {
        confidence = 100;
    } else if (hasBOM && numValid > numInvalid*10) {
        confidence = 80;
    } else if (numValid > 3 && numInvalid == 0) {
        confidence = 100;            
    } else if (numValid > 0 && numInvalid == 0) {
        confidence = 80;
    } else if (numValid > numInvalid*10) {
        // Probably corruput UTF-32BE data.  Valid sequences aren't likely by chance.
        confidence = 25;
    }

    return confidence;
}

CharsetRecog_UTF_32_BE::~CharsetRecog_UTF_32_BE()
{
    // nothing to do
}

const char *CharsetRecog_UTF_32_BE::getName() const
{
    return "UTF-32BE";
}

int32_t CharsetRecog_UTF_32_BE::getChar(const uint8_t *input, int32_t index) const
{
    return input[index + 0] << 24 | input[index + 1] << 16 |
           input[index + 2] <<  8 | input[index + 3];
} 

CharsetRecog_UTF_32_LE::~CharsetRecog_UTF_32_LE()
{
    // nothing to do
}

const char *CharsetRecog_UTF_32_LE::getName() const
{
    return "UTF-32LE";
}

int32_t CharsetRecog_UTF_32_LE::getChar(const uint8_t *input, int32_t index) const
{
    return input[index + 3] << 24 | input[index + 2] << 16 |
           input[index + 1] <<  8 | input[index + 0];
}

U_NAMESPACE_END
#endif

