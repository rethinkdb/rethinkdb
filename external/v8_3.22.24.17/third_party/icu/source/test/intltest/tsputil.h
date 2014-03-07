/********************************************************************
 * COPYRIGHT: 
 * Copyright (c) 1997-2005, International Business Machines Corporation and
 * others. All Rights Reserved.
 ********************************************************************/


#ifndef _PUTILTEST_
#define _PUTILTEST_

#include "intltest.h"

/** 
 * Test putil.h
 **/
class PUtilTest : public IntlTest {
    // IntlTest override
    void runIndexedTest( int32_t index, UBool exec, const char* &name, char* par );
public:

//    void testIEEEremainder(void);
    void testMaxMin(void);

private:
//    void remainderTest(double x, double y, double exp);
    void maxMinTest(double a, double b, double exp, UBool max);

    // the actual tests; methods return the number of errors
    void testNaN(void);
    void testPositiveInfinity(void);
    void testNegativeInfinity(void);
    void testZero(void);
    void testU_INLINE();

    // subtests of testNaN
    void testIsNaN(void);
    void NaNGT(void);
    void NaNLT(void);
    void NaNGTE(void);
    void NaNLTE(void);
    void NaNE(void);
    void NaNNE(void);

};
 
#endif
//eof
