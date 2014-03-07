/********************************************************************
 * COPYRIGHT: 
 * Copyright (c) 1997-2009, International Business Machines Corporation and
 * others. All Rights Reserved.
 ********************************************************************/

#ifndef _MESSAGEFORMATREGRESSIONTEST_
#define _MESSAGEFORMATREGRESSIONTEST_
 
#include "unicode/utypes.h"

#if !UCONFIG_NO_FORMATTING

#include "intltest.h"

/** 
 * Performs regression test for MessageFormat
 **/
class MessageFormatRegressionTest: public IntlTest {    
    
    // IntlTest override
    void runIndexedTest( int32_t index, UBool exec, const char* &name, char* par );
public:

    void Test4074764(void);
    void Test4058973(void);
    void Test4031438(void);
    void Test4052223(void);
    void Test4104976(void);
    void Test4106659(void);
    void Test4106660(void);
    void Test4111739(void);
    void Test4114743(void);
    void Test4116444(void);
    void Test4114739(void);
    void Test4113018(void);
    void Test4106661(void);
    void Test4094906(void);
    void Test4118592(void);
    void Test4118594(void);
    void Test4105380(void);
    void Test4120552(void);
    void Test4142938(void);
    void TestChoicePatternQuote(void);
    void Test4112104(void);
    void TestAPI(void);

protected:
    UBool failure(UErrorCode status, const char* msg, UBool possibleDataError=FALSE);

};

#endif /* #if !UCONFIG_NO_FORMATTING */
 
#endif // _MESSAGEFORMATREGRESSIONTEST_
//eof
