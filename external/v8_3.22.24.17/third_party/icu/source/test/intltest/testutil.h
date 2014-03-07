/*
**********************************************************************
*   Copyright (C) 2001-2009, International Business Machines
*   Corporation and others.  All Rights Reserved.
**********************************************************************
*   Date        Name        Description
*   05/23/00    aliu        Creation.
**********************************************************************
*/
#ifndef TESTUTIL_H
#define TESTUTIL_H

#include "intltest.h"

/**
 * Utility methods.  Everything in this class is static -- do not
 * attempt to instantiate.
 */
class TestUtility {

public:
    static UnicodeString &appendHex(UnicodeString &buf, UChar32 ch);

    static UnicodeString hex(UChar32 ch);

    static UnicodeString hex(const UnicodeString& s);

    static UnicodeString hex(const UnicodeString& s, UChar sep);

	static UnicodeString hex(const uint8_t* bytes, int32_t len);

private:

    TestUtility() {} // Prevent instantiation
};

#endif
