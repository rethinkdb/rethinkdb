/*
 *******************************************************************************
 *
 *   Copyright (C) 1999-2008, International Business Machines
 *   Corporation and others.  All Rights Reserved.
 *
 *******************************************************************************
 *   file name:  letsutil.cpp
 *
 *   created on: 04/25/2006
 *   created by: Eric R. Mader
 */

#include "unicode/utypes.h"
#include "unicode/unistr.h"
#include "unicode/ubidi.h"

#include "layout/LETypes.h"
#include "layout/LEScripts.h"
#include "layout/LayoutEngine.h"
#include "layout/LELanguages.h"

#include "OpenTypeLayoutEngine.h"

#include "letest.h"
#include "letsutil.h"

U_NAMESPACE_USE

char *getCString(const UnicodeString *uString)
{
    if (uString == NULL) {
        return NULL;
    }

    le_int32 uLength = uString->length();
    le_int32 cLength = uString->extract(0, uLength, NULL, 0, US_INV);
    char *cString = NEW_ARRAY(char, cLength + 1);

    uString->extract(0, uLength, cString, cLength, US_INV);
    cString[cLength] = '\0';

    return cString;
}

char *getCString(const LEUnicode16 *uChars)
{
    if (uChars == NULL) {
        return NULL;
    }

    const UnicodeString ustring(uChars);

    return getCString(&ustring);
}

char *getUTF8String(const UnicodeString *uString)
{
    if (uString == NULL) {
        return NULL;
    }

    le_int32 uLength = uString->length();
    le_int32 cLength = uString->extract(0, uLength, NULL, 0, "UTF-8");
    char *cString = NEW_ARRAY(char, cLength + 1);

    uString->extract(0, uLength, cString, cLength, "UTF-8");

    cString[cLength] = '\0';

    return cString;
}

void freeCString(char *cString)
{
    DELETE_ARRAY(cString);
}

le_bool getRTL(const UnicodeString &text)
{
    UBiDiLevel level = 0;
    UErrorCode status = U_ZERO_ERROR;
    le_int32 charCount = text.length();
    le_int32 limit = -1;
    UBiDi *ubidi = ubidi_openSized(charCount, 0, &status);

    ubidi_setPara(ubidi, text.getBuffer(), charCount, UBIDI_DEFAULT_LTR, NULL, &status);

    // TODO: Should check that there's only a single logical run...
    ubidi_getLogicalRun(ubidi, 0, &limit, &level);

    ubidi_close(ubidi);

    return level & 1;
}

le_int32 getLanguageCode(const char *lang)
{
    if (strlen(lang) != 3) {
        return -1;
    }

    LETag langTag = (LETag) ((lang[0] << 24) + (lang[1] << 16) + (lang[2] << 8) + 0x20);

    for (le_int32 i = 0; i < languageCodeCount; i += 1) {
        if (langTag == OpenTypeLayoutEngine::languageTags[i]) {
            return i;
        }
    }

    return -1;
}

