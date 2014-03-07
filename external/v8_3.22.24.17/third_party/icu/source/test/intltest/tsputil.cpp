/********************************************************************
 * COPYRIGHT: 
 * Copyright (c) 1997-2010, International Business Machines Corporation and
 * others. All Rights Reserved.
 ********************************************************************/

#include "tsputil.h"

#include <float.h> // DBL_MAX, DBL_MIN
#include "putilimp.h"

#define CASE(id,test) case id: name = #test; if (exec) { logln(#test "---"); logln((UnicodeString)""); test(); } break;

void 
PUtilTest::runIndexedTest( int32_t index, UBool exec, const char* &name, char* /*par*/ )
{
    //if (exec) logln("TestSuite PUtilTest: ");
    switch (index) {
        CASE(0, testMaxMin)
        CASE(1, testNaN)
        CASE(2, testPositiveInfinity)
        CASE(3, testNegativeInfinity)
        CASE(4, testZero)
        CASE(5, testU_INLINE)
//        CASE(, testIEEEremainder)

        default: name = ""; break; //needed to end loop
    }
}

#if 0
void
PUtilTest::testIEEEremainder()
{
    double    pinf  = uprv_getInfinity();
    double    ninf  = -uprv_getInfinity();
    double    nan   = uprv_getNaN();
    double    pzero = 0.0;
    double    nzero = 0.0;

    nzero *= -1;

    // simple remainder checks
    remainderTest(7.0, 2.5, -0.5);
    remainderTest(7.0, -2.5, -0.5);
#ifndef OS390
    // ### TODO:
    // The following tests fails on S/390 with IEEE support in release builds;
    // debug builds work.
    // The functioning of ChoiceFormat is not affected by this bug.
    remainderTest(-7.0, 2.5, 0.5);
    remainderTest(-7.0, -2.5, 0.5);
#endif
    remainderTest(5.0, 3.0, -1.0);
    
    // this should work
    //remainderTest(43.7, 2.5, 1.25);

    /*

    // infinity and real
    remainderTest(pinf, 1.0, 1.25);
    remainderTest(1.0, pinf, 1.0);
    remainderTest(ninf, 1.0, 1.25);
    remainderTest(1.0, ninf, 1.0);

    // test infinity and nan
    remainderTest(ninf, pinf, 1.25);
    remainderTest(ninf, nan, 1.25);
    remainderTest(pinf, nan, 1.25);

    // test infinity and zero
    remainderTest(pinf, pzero, 1.25);
    remainderTest(pinf, nzero, 1.25);
    remainderTest(ninf, pzero, 1.25);
    remainderTest(ninf, nzero, 1.25);
*/
}

void
PUtilTest::remainderTest(double x, double y, double exp)
{
    double result = uprv_IEEEremainder(x,y);

    if(        uprv_isNaN(result) && 
        ! ( uprv_isNaN(x) || uprv_isNaN(y))) {
        errln(UnicodeString("FAIL: got NaN as result without NaN as argument"));
        errln(UnicodeString("      IEEEremainder(") + x + ", " + y + ") is " + result + ", expected " + exp);
    }
    else if(result != exp)
        errln(UnicodeString("FAIL: IEEEremainder(") + x + ", " + y + ") is " + result + ", expected " + exp);
    else
        logln(UnicodeString("OK: IEEEremainder(") + x + ", " + y + ") is " + result);

}
#endif

void
PUtilTest::testMaxMin()
{
    double    pinf        = uprv_getInfinity();
    double    ninf        = -uprv_getInfinity();
    double    nan        = uprv_getNaN();
    double    pzero        = 0.0;
    double    nzero        = 0.0;

    nzero *= -1;

    // +Inf with -Inf
    maxMinTest(pinf, ninf, pinf, TRUE);
    maxMinTest(pinf, ninf, ninf, FALSE);

    // +Inf with +0 and -0
    maxMinTest(pinf, pzero, pinf, TRUE);
    maxMinTest(pinf, pzero, pzero, FALSE);
    maxMinTest(pinf, nzero, pinf, TRUE);
    maxMinTest(pinf, nzero, nzero, FALSE);

    // -Inf with +0 and -0
    maxMinTest(ninf, pzero, pzero, TRUE);
    maxMinTest(ninf, pzero, ninf, FALSE);
    maxMinTest(ninf, nzero, nzero, TRUE);
    maxMinTest(ninf, nzero, ninf, FALSE);

    // NaN with +Inf and -Inf
    maxMinTest(pinf, nan, nan, TRUE);
    maxMinTest(pinf, nan, nan, FALSE);
    maxMinTest(ninf, nan, nan, TRUE);
    maxMinTest(ninf, nan, nan, FALSE);

    // NaN with NaN
    maxMinTest(nan, nan, nan, TRUE);
    maxMinTest(nan, nan, nan, FALSE);

    // NaN with +0 and -0
    maxMinTest(nan, pzero, nan, TRUE);
    maxMinTest(nan, pzero, nan, FALSE);
    maxMinTest(nan, nzero, nan, TRUE);
    maxMinTest(nan, nzero, nan, FALSE);

    // +Inf with DBL_MAX and DBL_MIN
    maxMinTest(pinf, DBL_MAX, pinf, TRUE);
    maxMinTest(pinf, -DBL_MAX, pinf, TRUE);
    maxMinTest(pinf, DBL_MIN, pinf, TRUE);
    maxMinTest(pinf, -DBL_MIN, pinf, TRUE);
    maxMinTest(pinf, DBL_MIN, DBL_MIN, FALSE);
    maxMinTest(pinf, -DBL_MIN, -DBL_MIN, FALSE);
    maxMinTest(pinf, DBL_MAX, DBL_MAX, FALSE);
    maxMinTest(pinf, -DBL_MAX, -DBL_MAX, FALSE);

    // -Inf with DBL_MAX and DBL_MIN
    maxMinTest(ninf, DBL_MAX, DBL_MAX, TRUE);
    maxMinTest(ninf, -DBL_MAX, -DBL_MAX, TRUE);
    maxMinTest(ninf, DBL_MIN, DBL_MIN, TRUE);
    maxMinTest(ninf, -DBL_MIN, -DBL_MIN, TRUE);
    maxMinTest(ninf, DBL_MIN, ninf, FALSE);
    maxMinTest(ninf, -DBL_MIN, ninf, FALSE);
    maxMinTest(ninf, DBL_MAX, ninf, FALSE);
    maxMinTest(ninf, -DBL_MAX, ninf, FALSE);

    // +0 with DBL_MAX and DBL_MIN
    maxMinTest(pzero, DBL_MAX, DBL_MAX, TRUE);
    maxMinTest(pzero, -DBL_MAX, pzero, TRUE);
    maxMinTest(pzero, DBL_MIN, DBL_MIN, TRUE);
    maxMinTest(pzero, -DBL_MIN, pzero, TRUE);
    maxMinTest(pzero, DBL_MIN, pzero, FALSE);
    maxMinTest(pzero, -DBL_MIN, -DBL_MIN, FALSE);
    maxMinTest(pzero, DBL_MAX, pzero, FALSE);
    maxMinTest(pzero, -DBL_MAX, -DBL_MAX, FALSE);

    // -0 with DBL_MAX and DBL_MIN
    maxMinTest(nzero, DBL_MAX, DBL_MAX, TRUE);
    maxMinTest(nzero, -DBL_MAX, nzero, TRUE);
    maxMinTest(nzero, DBL_MIN, DBL_MIN, TRUE);
    maxMinTest(nzero, -DBL_MIN, nzero, TRUE);
    maxMinTest(nzero, DBL_MIN, nzero, FALSE);
    maxMinTest(nzero, -DBL_MIN, -DBL_MIN, FALSE);
    maxMinTest(nzero, DBL_MAX, nzero, FALSE);
    maxMinTest(nzero, -DBL_MAX, -DBL_MAX, FALSE);
}

void
PUtilTest::maxMinTest(double a, double b, double exp, UBool max)
{
    double result = 0.0;

    if(max)
        result = uprv_fmax(a, b);
    else
        result = uprv_fmin(a, b);

    UBool nanResultOK = (uprv_isNaN(a) || uprv_isNaN(b));

    if(uprv_isNaN(result) && ! nanResultOK) {
        errln(UnicodeString("FAIL: got NaN as result without NaN as argument"));
        if(max)
            errln(UnicodeString("      max(") + a + ", " + b + ") is " + result + ", expected " + exp);
        else
            errln(UnicodeString("      min(") + a + ", " + b + ") is " + result + ", expected " + exp);
    }
    else if(result != exp && ! (uprv_isNaN(result) || uprv_isNaN(exp)))
        if(max)
            errln(UnicodeString("FAIL: max(") + a + ", " + b + ") is " + result + ", expected " + exp);
        else
            errln(UnicodeString("FAIL: min(") + a + ", " + b + ") is " + result + ", expected " + exp);
    else {
        if (verbose) {
            if(max)
                logln(UnicodeString("OK: max(") + a + ", " + b + ") is " + result);
            else
                logln(UnicodeString("OK: min(") + a + ", " + b + ") is " + result);
        }
    }
}
//==============================

// NaN is weird- comparisons with NaN _always_ return false, with the
// exception of !=, which _always_ returns true
void
PUtilTest::testNaN(void)
{
    logln("NaN tests may show that the expected NaN!=NaN etc. is not true on some");
    logln("platforms; however, ICU does not rely on them because it defines");
    logln("and uses uprv_isNaN(). Therefore, most failing NaN tests only report warnings.");

    PUtilTest::testIsNaN();
    PUtilTest::NaNGT();
    PUtilTest::NaNLT();
    PUtilTest::NaNGTE();
    PUtilTest::NaNLTE();
    PUtilTest::NaNE();
    PUtilTest::NaNNE();

    logln("End of NaN tests.");
}

//==============================

void 
PUtilTest::testPositiveInfinity(void)
{
    double  pinf    = uprv_getInfinity();
    double  ninf    = -uprv_getInfinity();
    double  ten     = 10.0;

    if(uprv_isInfinite(pinf) != TRUE) {
        errln("FAIL: isInfinite(+Infinity) returned FALSE, should be TRUE.");
    }

    if(uprv_isPositiveInfinity(pinf) != TRUE) {
        errln("FAIL: isPositiveInfinity(+Infinity) returned FALSE, should be TRUE.");
    }

    if(uprv_isNegativeInfinity(pinf) != FALSE) {
        errln("FAIL: isNegativeInfinity(+Infinity) returned TRUE, should be FALSE.");
    }

    if((pinf > DBL_MAX) != TRUE) {
        errln("FAIL: +Infinity > DBL_MAX returned FALSE, should be TRUE.");
    }

    if((pinf > DBL_MIN) != TRUE) {
        errln("FAIL: +Infinity > DBL_MIN returned FALSE, should be TRUE.");
    }

    if((pinf > ninf) != TRUE) {
        errln("FAIL: +Infinity > -Infinity returned FALSE, should be TRUE.");
    }

    if((pinf > ten) != TRUE) {
        errln("FAIL: +Infinity > 10.0 returned FALSE, should be TRUE.");
    }
}

//==============================

void           
PUtilTest::testNegativeInfinity(void)
{
    double  pinf    = uprv_getInfinity();
    double  ninf    = -uprv_getInfinity();
    double  ten     = 10.0;

    if(uprv_isInfinite(ninf) != TRUE) {
        errln("FAIL: isInfinite(-Infinity) returned FALSE, should be TRUE.");
    }

    if(uprv_isNegativeInfinity(ninf) != TRUE) {
        errln("FAIL: isNegativeInfinity(-Infinity) returned FALSE, should be TRUE.");
    }

    if(uprv_isPositiveInfinity(ninf) != FALSE) {
        errln("FAIL: isPositiveInfinity(-Infinity) returned TRUE, should be FALSE.");
    }

    if((ninf < DBL_MAX) != TRUE) {
        errln("FAIL: -Infinity < DBL_MAX returned FALSE, should be TRUE.");
    }

    if((ninf < DBL_MIN) != TRUE) {
        errln("FAIL: -Infinity < DBL_MIN returned FALSE, should be TRUE.");
    }

    if((ninf < pinf) != TRUE) {
        errln("FAIL: -Infinity < +Infinity returned FALSE, should be TRUE.");
    }

    if((ninf < ten) != TRUE) {
        errln("FAIL: -Infinity < 10.0 returned FALSE, should be TRUE.");
    }
}

//==============================

// notes about zero:
// -0.0 == 0.0 == TRUE
// -0.0 <  0.0 == FALSE
// generating -0.0 must be done at runtime.  compiler apparently ignores sign?
void           
PUtilTest::testZero(void)
{
    // volatile is used to fake out the compiler optimizer.  We really want to divide by 0.
    volatile double pzero   = 0.0;
    volatile double nzero   = 0.0;

    nzero *= -1;

    if((pzero == nzero) != TRUE) {
        errln("FAIL: 0.0 == -0.0 returned FALSE, should be TRUE.");
    }

    if((pzero > nzero) != FALSE) {
        errln("FAIL: 0.0 > -0.0 returned TRUE, should be FALSE.");
    }

    if((pzero >= nzero) != TRUE) {
        errln("FAIL: 0.0 >= -0.0 returned FALSE, should be TRUE.");
    }

    if((pzero < nzero) != FALSE) {
        errln("FAIL: 0.0 < -0.0 returned TRUE, should be FALSE.");
    }

    if((pzero <= nzero) != TRUE) {
        errln("FAIL: 0.0 <= -0.0 returned FALSE, should be TRUE.");
    }
#ifndef OS400 /* OS/400 will generate divide by zero exception MCH1214 */
    if(uprv_isInfinite(1/pzero) != TRUE) {
        errln("FAIL: isInfinite(1/0.0) returned FALSE, should be TRUE.");
    }

    if(uprv_isInfinite(1/nzero) != TRUE) {
        errln("FAIL: isInfinite(1/-0.0) returned FALSE, should be TRUE.");
    }

    if(uprv_isPositiveInfinity(1/pzero) != TRUE) {
        errln("FAIL: isPositiveInfinity(1/0.0) returned FALSE, should be TRUE.");
    }

    if(uprv_isNegativeInfinity(1/nzero) != TRUE) {
        errln("FAIL: isNegativeInfinity(1/-0.0) returned FALSE, should be TRUE.");
    }
#endif
}

//==============================

void
PUtilTest::testIsNaN(void)
{
    double  pinf    = uprv_getInfinity();
    double  ninf    = -uprv_getInfinity();
    double  nan     = uprv_getNaN();
    double  ten     = 10.0;

    if(uprv_isNaN(nan) == FALSE) {
        errln("FAIL: isNaN() returned FALSE for NaN.");
    }

    if(uprv_isNaN(pinf) == TRUE) {
        errln("FAIL: isNaN() returned TRUE for +Infinity.");
    }

    if(uprv_isNaN(ninf) == TRUE) {
        errln("FAIL: isNaN() returned TRUE for -Infinity.");
    }

    if(uprv_isNaN(ten) == TRUE) {
        errln("FAIL: isNaN() returned TRUE for 10.0.");
    }
}

//==============================

void
PUtilTest::NaNGT(void)
{
    double  pinf    = uprv_getInfinity();
    double  ninf    = -uprv_getInfinity();
    double  nan     = uprv_getNaN();
    double  ten     = 10.0;

    if((nan > nan) != FALSE) {
        logln("WARNING: NaN > NaN returned TRUE, should be FALSE");
    }

    if((nan > pinf) != FALSE) {
        logln("WARNING: NaN > +Infinity returned TRUE, should be FALSE");
    }

    if((nan > ninf) != FALSE) {
        logln("WARNING: NaN > -Infinity returned TRUE, should be FALSE");
    }

    if((nan > ten) != FALSE) {
        logln("WARNING: NaN > 10.0 returned TRUE, should be FALSE");
    }
}

//==============================

void 
PUtilTest::NaNLT(void)
{
    double  pinf    = uprv_getInfinity();
    double  ninf    = -uprv_getInfinity();
    double  nan     = uprv_getNaN();
    double  ten     = 10.0;

    if((nan < nan) != FALSE) {
        logln("WARNING: NaN < NaN returned TRUE, should be FALSE");
    }

    if((nan < pinf) != FALSE) {
        logln("WARNING: NaN < +Infinity returned TRUE, should be FALSE");
    }

    if((nan < ninf) != FALSE) {
        logln("WARNING: NaN < -Infinity returned TRUE, should be FALSE");
    }

    if((nan < ten) != FALSE) {
        logln("WARNING: NaN < 10.0 returned TRUE, should be FALSE");
    }
}

//==============================

void                   
PUtilTest::NaNGTE(void)
{
    double  pinf    = uprv_getInfinity();
    double  ninf    = -uprv_getInfinity();
    double  nan     = uprv_getNaN();
    double  ten     = 10.0;

    if((nan >= nan) != FALSE) {
        logln("WARNING: NaN >= NaN returned TRUE, should be FALSE");
    }

    if((nan >= pinf) != FALSE) {
        logln("WARNING: NaN >= +Infinity returned TRUE, should be FALSE");
    }

    if((nan >= ninf) != FALSE) {
        logln("WARNING: NaN >= -Infinity returned TRUE, should be FALSE");
    }

    if((nan >= ten) != FALSE) {
        logln("WARNING: NaN >= 10.0 returned TRUE, should be FALSE");
    }
}

//==============================

void                   
PUtilTest::NaNLTE(void)
{
    double  pinf    = uprv_getInfinity();
    double  ninf    = -uprv_getInfinity();
    double  nan     = uprv_getNaN();
    double  ten     = 10.0;

    if((nan <= nan) != FALSE) {
        logln("WARNING: NaN <= NaN returned TRUE, should be FALSE");
    }

    if((nan <= pinf) != FALSE) {
        logln("WARNING: NaN <= +Infinity returned TRUE, should be FALSE");
    }

    if((nan <= ninf) != FALSE) {
        logln("WARNING: NaN <= -Infinity returned TRUE, should be FALSE");
    }

    if((nan <= ten) != FALSE) {
        logln("WARNING: NaN <= 10.0 returned TRUE, should be FALSE");
    }
}

//==============================

void                   
PUtilTest::NaNE(void)
{
    double  pinf    = uprv_getInfinity();
    double  ninf    = -uprv_getInfinity();
    double  nan     = uprv_getNaN();
    double  ten     = 10.0;

    if((nan == nan) != FALSE) {
        logln("WARNING: NaN == NaN returned TRUE, should be FALSE");
    }

    if((nan == pinf) != FALSE) {
        logln("WARNING: NaN == +Infinity returned TRUE, should be FALSE");
    }

    if((nan == ninf) != FALSE) {
        logln("WARNING: NaN == -Infinity returned TRUE, should be FALSE");
    }

    if((nan == ten) != FALSE) {
        logln("WARNING: NaN == 10.0 returned TRUE, should be FALSE");
    }
}

//==============================

void 
PUtilTest::NaNNE(void)
{
    double  pinf    = uprv_getInfinity();
    double  ninf    = -uprv_getInfinity();
    double  nan     = uprv_getNaN();
    double  ten     = 10.0;

    if((nan != nan) != TRUE) {
        logln("WARNING: NaN != NaN returned FALSE, should be TRUE");
    }

    if((nan != pinf) != TRUE) {
        logln("WARNING: NaN != +Infinity returned FALSE, should be TRUE");
    }

    if((nan != ninf) != TRUE) {
        logln("WARNING: NaN != -Infinity returned FALSE, should be TRUE");
    }

    if((nan != ten) != TRUE) {
        logln("WARNING: NaN != 10.0 returned FALSE, should be TRUE");
    }
}

U_INLINE int32_t inlineTriple(int32_t x) {
    return 3*x;
}

// "code" coverage test for Jitterbug 4515 RFE: in C++, use U_INLINE=inline
void
PUtilTest::testU_INLINE() {
    if(inlineTriple(2)!=6 || inlineTriple(-55)!=-165) {
        errln("inlineTriple() failed");
    }
}
