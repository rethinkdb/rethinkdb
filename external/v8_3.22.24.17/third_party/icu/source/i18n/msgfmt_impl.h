/*
*******************************************************************************
* Copyright (C) 2007-2008, International Business Machines Corporation and
* others. All Rights Reserved.                                                *
*******************************************************************************
*
* File MSGFMT.H
*
*******************************************************************************
*/

#ifndef __MSGFMT_IMPL_H__
#define __MSGFMT_IMPL_H__

#include "unicode/utypes.h"

#if !UCONFIG_NO_FORMATTING
    
#include "unicode/msgfmt.h"
#include "uvector.h"
#include "unicode/strenum.h"

U_NAMESPACE_BEGIN

class FormatNameEnumeration : public StringEnumeration {
public:
    FormatNameEnumeration(UVector *fFormatNames, UErrorCode& status);
    virtual ~FormatNameEnumeration();
    static UClassID U_EXPORT2 getStaticClassID(void);
    virtual UClassID getDynamicClassID(void) const;
    virtual const UnicodeString* snext(UErrorCode& status);
    virtual void reset(UErrorCode& status);
    virtual int32_t count(UErrorCode& status) const;
private:
    int32_t pos;
    UVector *fFormatNames;
};

U_NAMESPACE_END

#endif

#endif
