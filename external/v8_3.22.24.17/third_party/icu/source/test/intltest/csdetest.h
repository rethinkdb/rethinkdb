/*
 **********************************************************************
 *   Copyright (C) 2005-2009, International Business Machines
 *   Corporation and others.  All Rights Reserved.
 **********************************************************************
 */

#ifndef __CSDETEST_H
#define __CSDETEST_H

#include "unicode/utypes.h"
#include "unicode/unistr.h"

#include "intltest.h"

class CharsetDetectionTest: public IntlTest {
public:
  
    CharsetDetectionTest();
    virtual ~CharsetDetectionTest();

    virtual void runIndexedTest(int32_t index, UBool exec, const char* &name, char* par = NULL );

    virtual void ConstructionTest();
    virtual void UTF8Test();
    virtual void UTF16Test();
    virtual void C1BytesTest();
    virtual void InputFilterTest();
    virtual void DetectionTest();
    virtual void IBM424Test();
    virtual void IBM420Test();
    virtual void Ticket6394Test();

private:
    void checkEncoding(const UnicodeString &testString,
                       const UnicodeString &encoding, const UnicodeString &id);

    virtual const char *getPath(char buffer[2048], const char *filename);

};

#endif
