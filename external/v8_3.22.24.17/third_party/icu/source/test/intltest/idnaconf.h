/*
 *******************************************************************************
 *
 *   Copyright (C) 2005, International Business Machines
 *   Corporation and others.  All Rights Reserved.
 *
 *******************************************************************************
 *
 *   created on: 2005jun15
 *   created by: Raymond Yang
 */

#ifndef IDNA_CONF_TEST_H
#define IDNA_CONF_TEST_H

#include "intltest.h"
#include "unicode/ustring.h"


class IdnaConfTest: public IntlTest {
public:
    void runIndexedTest(int32_t index, UBool exec, const char* &name, char* par=NULL);
    IdnaConfTest();
    virtual ~IdnaConfTest();
private:
    void Test(void);

    // for test file handling
    UChar* base;
    int len ;
    int curOffset;

    UBool  ReadAndConvertFile();
    int isNewlineMark();
    UBool ReadOneLine(UnicodeString&);

    // for parsing one test record
    UnicodeString id;   // for debug & error output
    UnicodeString namebase;
    UnicodeString namezone;
    int type;     // 0 toascii,             1 tounicode
    int option;   // 0 UseSTD3ASCIIRules,   1 ALLOW_UNASSIGNED
    int passfail; // 0 pass,                1 fail

    void ExplainCodePointTag(UnicodeString& buf);
    void Call();
};

#endif /*IDNA_CONF_TEST_H*/
