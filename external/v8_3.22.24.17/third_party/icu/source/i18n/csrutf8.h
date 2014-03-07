/*
 **********************************************************************
 *   Copyright (C) 2005-2006, International Business Machines
 *   Corporation and others.  All Rights Reserved.
 **********************************************************************
 */

#ifndef __CSRUTF8_H
#define __CSRUTF8_H

#include "unicode/utypes.h"

#if !UCONFIG_NO_CONVERSION

#include "csrecog.h"

U_NAMESPACE_BEGIN

/**
 * Charset recognizer for UTF-8
 *
 * @internal
 */
class CharsetRecog_UTF8: public CharsetRecognizer {

 public:
		
    virtual ~CharsetRecog_UTF8();		 

    const char *getName() const;

    /* (non-Javadoc)
     * @see com.ibm.icu.text.CharsetRecognizer#match(com.ibm.icu.text.CharsetDetector)
     */
    int32_t match(InputText *det);
	
};

U_NAMESPACE_END

#endif
#endif /* __CSRUTF8_H */
