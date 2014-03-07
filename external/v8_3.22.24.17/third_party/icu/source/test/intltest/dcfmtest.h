/********************************************************************
 * COPYRIGHT:
 * Copyright (c) 2010, International Business Machines Corporation and
 * others. All Rights Reserved.
 ********************************************************************/

//
//   file:  dcfmtest.h
//
//   Data driven decimal formatter test.
//      Includes testing of both parsing and formatting.
//      Tests are in the text file dcfmtest.txt, in the source/test/testdata/ directory.
//

#ifndef DCFMTEST_H
#define DCFMTEST_H

#include "unicode/utypes.h"
#if !UCONFIG_NO_REGULAR_EXPRESSIONS

#include "intltest.h"


class DecimalFormatTest: public IntlTest {
public:

    DecimalFormatTest();
    virtual ~DecimalFormatTest();

    virtual void runIndexedTest(int32_t index, UBool exec, const char* &name, char* par = NULL );

    // The following are test functions that are visible from the intltest test framework.
    virtual void DataDrivenTests();

    // The following functions are internal to the decimal format tests.
    virtual UChar *ReadAndConvertFile(const char *fileName, int32_t &len, UErrorCode &status);
    virtual const char *getPath(char buffer[2048], const char *filename);
    virtual void execParseTest(int32_t lineNum,
                              const UnicodeString &inputText,
                              const UnicodeString &expectedType,
                              const UnicodeString &expectedDecimal,
                              UErrorCode &status);

    virtual void execFormatTest(int32_t lineNum,
                               const UnicodeString &pattern,
                               const UnicodeString &round, 
                               const UnicodeString &input,
                               const UnicodeString &expected,
                               UErrorCode &status);
};

#endif   // !UCONFIG_NO_REGULAR_EXPRESSIONS
#endif
