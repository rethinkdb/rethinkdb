/*
 **********************************************************************
 *   Copyright (C) 2005-2006, International Business Machines
 *   Corporation and others.  All Rights Reserved.
 **********************************************************************
 */

#include "unicode/utypes.h"

#if !UCONFIG_NO_CONVERSION
#include "unicode/unistr.h"
#include "unicode/ucnv.h"

#include "csmatch.h"

#include "csrecog.h"
#include "inputext.h"

U_NAMESPACE_BEGIN

CharsetMatch::CharsetMatch()
  : csr(0), confidence(0)
{
    // nothing else to do.
}

void CharsetMatch::set(InputText *input, CharsetRecognizer *cr, int32_t conf)
{
    textIn = input;
    csr = cr;
    confidence = conf; 
}

const char* CharsetMatch::getName()const
{
    return csr->getName(); 
}

const char* CharsetMatch::getLanguage()const
{
    return csr->getLanguage(); 
}

int32_t CharsetMatch::getConfidence()const
{
    return confidence;
}

int32_t CharsetMatch::getUChars(UChar *buf, int32_t cap, UErrorCode *status) const
{
    UConverter *conv = ucnv_open(getName(), status);
    int32_t result = ucnv_toUChars(conv, buf, cap, (const char *) textIn->fRawInput, textIn->fRawLength, status);

    ucnv_close(conv);

    return result;
}

U_NAMESPACE_END

#endif
