/********************************************************************
 * COPYRIGHT:
 * Copyright (c) 2002-2010, International Business Machines Corporation and
 * others. All Rights Reserved.
 ********************************************************************/


#ifndef REGEXTST_H
#define REGEXTST_H

#include "unicode/utypes.h"
#if !UCONFIG_NO_REGULAR_EXPRESSIONS

#include "intltest.h"

struct UText;
typedef struct UText UText;

class RegexTest: public IntlTest {
public:

    RegexTest();
    virtual ~RegexTest();

    virtual void runIndexedTest(int32_t index, UBool exec, const char* &name, char* par = NULL );

    // The following are test functions that are visible from the intltest test framework.
    virtual void API_Match();
    virtual void API_Pattern();
    virtual void API_Replace();
    virtual void Basic();
    virtual void Extended();
    virtual void Errors();
    virtual void PerlTests();
    virtual void Bug6149();
    virtual void Callbacks();
    virtual void FindProgressCallbacks();
    virtual void UTextBasic();
    virtual void API_Match_UTF8();
    virtual void API_Pattern_UTF8();
    virtual void API_Replace_UTF8();
    virtual void PerlTestsUTF8();
    virtual void PreAllocatedUTextCAPI();
    virtual void Bug7651();
    virtual void Bug7740();

    // The following functions are internal to the regexp tests.
    virtual void assertUText(const char *expected, UText *actual, const char *file, int line);
    virtual void assertUTextInvariant(const char *invariant, UText *actual, const char *file, int line);
    virtual UBool doRegexLMTest(const char *pat, const char *text, UBool looking, UBool match, int32_t line);
    virtual UBool doRegexLMTestUTF8(const char *pat, const char *text, UBool looking, UBool match, int32_t line);
    virtual void regex_find(const UnicodeString &pat, const UnicodeString &flags,
                            const UnicodeString &input, const char *srcPath, int32_t line);
    virtual void regex_err(const char *pat, int32_t errline, int32_t errcol,
                            UErrorCode expectedStatus, int32_t line);
    virtual UChar *ReadAndConvertFile(const char *fileName, int32_t &len, const char *charset, UErrorCode &status);
    virtual const char *getPath(char buffer[2048], const char *filename);

};

#endif   // !UCONFIG_NO_REGULAR_EXPRESSIONS
#endif
