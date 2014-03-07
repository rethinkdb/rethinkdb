/*
 **********************************************************************
 *   Copyright (C) 2005-2006, International Business Machines
 *   Corporation and others.  All Rights Reserved.
 **********************************************************************
 */

#ifndef __CSRECOG_H
#define __CSRECOG_H

#include "unicode/uobject.h"

#if !UCONFIG_NO_CONVERSION

#include "inputext.h"

U_NAMESPACE_BEGIN

class CharsetRecognizer : public UMemory
{
 public:
    /**
     * Get the IANA name of this charset.
     * @return the charset name.
     */
    virtual const char *getName() const = 0;
    
    /**
     * Get the ISO language code for this charset.
     * @return the language code, or <code>null</code> if the language cannot be determined.
     */
    virtual const char *getLanguage() const;
        
    virtual int32_t match(InputText *textIn) = 0;

    virtual ~CharsetRecognizer();
};

U_NAMESPACE_END

#endif
#endif /* __CSRECOG_H */
