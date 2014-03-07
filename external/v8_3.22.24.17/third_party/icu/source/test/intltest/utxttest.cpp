/********************************************************************
 * COPYRIGHT: 
 * Copyright (c) 2005-2010, International Business Machines Corporation and
 * others. All Rights Reserved.
 ********************************************************************/
/************************************************************************
*   Tests for the UText and UTextIterator text abstraction classses
*
************************************************************************/

#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include "unicode/utypes.h"
#include "unicode/utext.h"
#include "unicode/utf8.h"
#include "unicode/ustring.h"
#include "unicode/uchriter.h"
#include "utxttest.h"

static UBool  gFailed = FALSE;
static int    gTestNum = 0;

// Forward decl
UText *openFragmentedUnicodeString(UText *ut, UnicodeString *s, UErrorCode *status);

#define TEST_ASSERT(x) \
{ if ((x)==FALSE) {errln("Test #%d failure in file %s at line %d\n", gTestNum, __FILE__, __LINE__);\
                     gFailed = TRUE;\
   }}


#define TEST_SUCCESS(status) \
{ if (U_FAILURE(status)) {errln("Test #%d failure in file %s at line %d. Error = \"%s\"\n", \
       gTestNum, __FILE__, __LINE__, u_errorName(status)); \
       gFailed = TRUE;\
   }}

UTextTest::UTextTest() {
}

UTextTest::~UTextTest() {
}


void
UTextTest::runIndexedTest(int32_t index, UBool exec,
                          const char* &name, char* /*par*/) {
    switch (index) {
        case 0: name = "TextTest";
            if (exec) TextTest();    break;
        case 1: name = "ErrorTest";
            if (exec) ErrorTest();   break;
        case 2: name = "FreezeTest";
            if (exec) FreezeTest();  break;
        case 3: name = "Ticket5560";
            if (exec) Ticket5560();  break;
        case 4: name = "Ticket6847";
            if (exec) Ticket6847();  break;
        case 5: name = "ComparisonTest";
            if (exec) ComparisonTest(); break;
        default: name = "";          break;
    }
}

//
// Quick and dirty random number generator.
//   (don't use library so that results are portable.
static uint32_t m_seed = 1;
static uint32_t m_rand()
{
    m_seed = m_seed * 1103515245 + 12345;
    return (uint32_t)(m_seed/65536) % 32768;
}


//
//   TextTest()
//
//       Top Level function for UText testing.
//       Specifies the strings to be tested, with the acutal testing itself
//       being carried out in another function, TestString().
//
void  UTextTest::TextTest() {
    int32_t i, j;

    TestString("abcd\\U00010001xyz");
    TestString("");

    // Supplementary chars at start or end
    TestString("\\U00010001");
    TestString("abc\\U00010001");
    TestString("\\U00010001abc");

    // Test simple strings of lengths 1 to 60, looking for glitches at buffer boundaries
    UnicodeString s;
    for (i=1; i<60; i++) {
        s.truncate(0);
        for (j=0; j<i; j++) {
            if (j+0x30 == 0x5c) {
                // backslash.  Needs to be escaped
                s.append((UChar)0x5c);
            }
            s.append(UChar(j+0x30));
        }
        TestString(s);
    }

   // Test strings with odd-aligned supplementary chars,
   //    looking for glitches at buffer boundaries
    for (i=1; i<60; i++) {
        s.truncate(0);
        s.append((UChar)0x41);
        for (j=0; j<i; j++) {
            s.append(UChar32(j+0x11000));
        }
        TestString(s);
    }

    // String of chars of randomly varying size in utf-8 representation.
    //   Exercise the mapping, and the varying sized buffer.
    //
    s.truncate(0);
    UChar32  c1 = 0;
    UChar32  c2 = 0x100;
    UChar32  c3 = 0xa000;
    UChar32  c4 = 0x11000;
    for (i=0; i<1000; i++) {
        int len8 = m_rand()%4 + 1;
        switch (len8) {
            case 1: 
                c1 = (c1+1)%0x80;
                // don't put 0 into string (0 terminated strings for some tests)
                // don't put '\', will cause unescape() to fail.
                if (c1==0x5c || c1==0) {
                    c1++;
                }
                s.append(c1);
                break;
            case 2:
                s.append(c2++);
                break;
            case 3:
                s.append(c3++);
                break;
            case 4:
                s.append(c4++);
                break;
        }
    }
    TestString(s);
}


//
//  TestString()     Run a suite of UText tests on a string.
//                   The test string is unescaped before use.
//
void UTextTest::TestString(const UnicodeString &s) {
    int32_t       i;
    int32_t       j;
    UChar32       c;
    int32_t       cpCount = 0;
    UErrorCode    status  = U_ZERO_ERROR;
    UText        *ut      = NULL;
    int32_t       saLen;

    UnicodeString sa = s.unescape();
    saLen = sa.length();

    //
    // Build up a mapping between code points and UTF-16 code unit indexes.
    //
    m *cpMap = new m[sa.length() + 1];
    j = 0;
    for (i=0; i<sa.length(); i=sa.moveIndex32(i, 1)) {
        c = sa.char32At(i);
        cpMap[j].nativeIdx = i;
        cpMap[j].cp = c;
        j++;
        cpCount++;
    }
    cpMap[j].nativeIdx = i;   // position following the last char in utf-16 string.    


    // UChar * test, null terminated
    status = U_ZERO_ERROR;
    UChar *buf = new UChar[saLen+1];
    sa.extract(buf, saLen+1, status);
    TEST_SUCCESS(status);
    ut = utext_openUChars(NULL, buf, -1, &status);
    TEST_SUCCESS(status);
    TestAccess(sa, ut, cpCount, cpMap);
    utext_close(ut);
    delete [] buf;

    // UChar * test, with length
    status = U_ZERO_ERROR;
    buf = new UChar[saLen+1];
    sa.extract(buf, saLen+1, status);
    TEST_SUCCESS(status);
    ut = utext_openUChars(NULL, buf, saLen, &status);
    TEST_SUCCESS(status);
    TestAccess(sa, ut, cpCount, cpMap);
    utext_close(ut);
    delete [] buf;


    // UnicodeString test
    status = U_ZERO_ERROR;
    ut = utext_openUnicodeString(NULL, &sa, &status);
    TEST_SUCCESS(status);
    TestAccess(sa, ut, cpCount, cpMap);
    TestCMR(sa, ut, cpCount, cpMap, cpMap);
    utext_close(ut);


    // Const UnicodeString test
    status = U_ZERO_ERROR;
    ut = utext_openConstUnicodeString(NULL, &sa, &status);
    TEST_SUCCESS(status);
    TestAccess(sa, ut, cpCount, cpMap);
    utext_close(ut);


    // Replaceable test.  (UnicodeString inherits Replaceable)
    status = U_ZERO_ERROR;
    ut = utext_openReplaceable(NULL, &sa, &status);
    TEST_SUCCESS(status);
    TestAccess(sa, ut, cpCount, cpMap);
    TestCMR(sa, ut, cpCount, cpMap, cpMap);
    utext_close(ut);

    // Character Iterator Tests
    status = U_ZERO_ERROR;
    const UChar *cbuf = sa.getBuffer();
    CharacterIterator *ci = new UCharCharacterIterator(cbuf, saLen, status);
    TEST_SUCCESS(status);
    ut = utext_openCharacterIterator(NULL, ci, &status);
    TEST_SUCCESS(status);
    TestAccess(sa, ut, cpCount, cpMap);
    utext_close(ut);
    delete ci;
    

    // Fragmented UnicodeString  (Chunk size of one)
    //
    status = U_ZERO_ERROR;
    ut = openFragmentedUnicodeString(NULL, &sa, &status);
    TEST_SUCCESS(status);
    TestAccess(sa, ut, cpCount, cpMap);
    utext_close(ut);

    //
    // UTF-8 test
    //

    // Convert the test string from UnicodeString to (char *) in utf-8 format
    int32_t u8Len = sa.extract(0, sa.length(), NULL, 0, "utf-8");
    char *u8String = new char[u8Len + 1];
    sa.extract(0, sa.length(), u8String, u8Len+1, "utf-8");

    // Build up the map of code point indices in the utf-8 string
    m * u8Map = new m[sa.length() + 1];
    i = 0;   // native utf-8 index
    for (j=0; j<cpCount ; j++) {  // code point number
        u8Map[j].nativeIdx = i;
        U8_NEXT(u8String, i, u8Len, c)
        u8Map[j].cp = c;
    }
    u8Map[cpCount].nativeIdx = u8Len;   // position following the last char in utf-8 string.

    // Do the test itself
    status = U_ZERO_ERROR;
    ut = utext_openUTF8(NULL, u8String, -1, &status);
    TEST_SUCCESS(status);
    TestAccess(sa, ut, cpCount, u8Map);
    utext_close(ut);



    delete []cpMap;
    delete []u8Map;
    delete []u8String;
}

//  TestCMR   test Copy, Move and Replace operations.
//              us         UnicodeString containing the test text.
//              ut         UText containing the same test text.
//              cpCount    number of code points in the test text.
//              nativeMap  Mapping from code points to native indexes for the UText.
//              u16Map     Mapping from code points to UTF-16 indexes, for use with the UnicodeString.
//
//     This function runs a whole series of opertions on each incoming UText.
//     The UText is deep-cloned prior to each operation, so that the original UText remains unchanged.
//     
void UTextTest::TestCMR(const UnicodeString &us, UText *ut, int cpCount, m *nativeMap, m *u16Map) {
    TEST_ASSERT(utext_isWritable(ut) == TRUE);

    int  srcLengthType;       // Loop variables for selecting the postion and length
    int  srcPosType;          //   of the block to operate on within the source text.
    int  destPosType; 

    int  srcIndex  = 0;       // Code Point indexes of the block to operate on for
    int  srcLength = 0;       //   a specific test.

    int  destIndex = 0;       // Code point index of the destination for a copy/move test.

    int32_t  nativeStart = 0; // Native unit indexes for a test.
    int32_t  nativeLimit = 0;
    int32_t  nativeDest  = 0;

    int32_t  u16Start    = 0; // UTF-16 indexes for a test.
    int32_t  u16Limit    = 0; //   used when performing the same operation in a Unicode String
    int32_t  u16Dest     = 0;

    // Iterate over a whole series of source index, length and a target indexes.
    // This is done with code point indexes; these will be later translated to native
    //   indexes using the cpMap.
    for (srcLengthType=1; srcLengthType<=3; srcLengthType++) {
        switch (srcLengthType) {
            case 1: srcLength = 1; break;
            case 2: srcLength = 5; break;
            case 3: srcLength = cpCount / 3;
        }
        for (srcPosType=1; srcPosType<=5; srcPosType++) {
            switch (srcPosType) {
                case 1: srcIndex = 0; break;
                case 2: srcIndex = 1; break;
                case 3: srcIndex = cpCount - srcLength; break;
                case 4: srcIndex = cpCount - srcLength - 1; break;
                case 5: srcIndex = cpCount / 2; break;
            }
            if (srcIndex < 0 || srcIndex + srcLength > cpCount) {
                // filter out bogus test cases - 
                //   those with a source range that falls of an edge of the string.
                continue;
            }

            //
            // Copy and move tests.
            //   iterate over a variety of destination positions.
            //
            for (destPosType=1; destPosType<=4; destPosType++) {
                switch (destPosType) {
                    case 1: destIndex = 0; break;
                    case 2: destIndex = 1; break;
                    case 3: destIndex = srcIndex - 1; break;
                    case 4: destIndex = srcIndex + srcLength + 1; break;
                    case 5: destIndex = cpCount-1; break;
                    case 6: destIndex = cpCount; break;
                }
                if (destIndex<0 || destIndex>cpCount) {
                    // filter out bogus test cases.
                    continue;
                }

                nativeStart = nativeMap[srcIndex].nativeIdx;
                nativeLimit = nativeMap[srcIndex+srcLength].nativeIdx;
                nativeDest  = nativeMap[destIndex].nativeIdx;

                u16Start    = u16Map[srcIndex].nativeIdx;
                u16Limit    = u16Map[srcIndex+srcLength].nativeIdx;
                u16Dest     = u16Map[destIndex].nativeIdx;

                gFailed = FALSE;
                TestCopyMove(us, ut, FALSE,
                    nativeStart, nativeLimit, nativeDest,
                    u16Start, u16Limit, u16Dest);

                TestCopyMove(us, ut, TRUE,
                    nativeStart, nativeLimit, nativeDest,
                    u16Start, u16Limit, u16Dest);

                if (gFailed) {
                    return;
                }
            }

            //
            //  Replace tests.
            //
            UnicodeString fullRepString("This is an arbitrary string that will be used as replacement text");
            for (int32_t replStrLen=0; replStrLen<20; replStrLen++) {
                UnicodeString repStr(fullRepString, 0, replStrLen);
                TestReplace(us, ut,
                    nativeStart, nativeLimit,
                    u16Start, u16Limit,
                    repStr);
                if (gFailed) {
                    return;
                }
            }

        }
    }

}

//
//   TestCopyMove    run a single test case for utext_copy.
//                   Test cases are created in TestCMR and dispatched here for execution.
//
void UTextTest::TestCopyMove(const UnicodeString &us, UText *ut, UBool move,
                    int32_t nativeStart, int32_t nativeLimit, int32_t nativeDest,
                    int32_t u16Start, int32_t u16Limit, int32_t u16Dest) 
{
    UErrorCode      status   = U_ZERO_ERROR;
    UText          *targetUT = NULL;
    gTestNum++;
    gFailed = FALSE;

    //
    //  clone the UText.  The test will be run in the cloned copy
    //  so that we don't alter the original.
    //
    targetUT = utext_clone(NULL, ut, TRUE, FALSE, &status);
    TEST_SUCCESS(status);
    UnicodeString targetUS(us);    // And copy the reference string.

    // do the test operation first in the reference
    targetUS.copy(u16Start, u16Limit, u16Dest);
    if (move) {
        // delete out the source range.
        if (u16Limit < u16Dest) {
            targetUS.removeBetween(u16Start, u16Limit);
        } else {
            int32_t amtCopied = u16Limit - u16Start;
            targetUS.removeBetween(u16Start+amtCopied, u16Limit+amtCopied);
        }
    }

    // Do the same operation in the UText under test
    utext_copy(targetUT, nativeStart, nativeLimit, nativeDest, move, &status);
    if (nativeDest > nativeStart && nativeDest < nativeLimit) {
        TEST_ASSERT(status == U_INDEX_OUTOFBOUNDS_ERROR);
    } else {
        TEST_SUCCESS(status);

        // Compare the results of the two parallel tests
        int32_t  usi = 0;    // UnicodeString postion, utf-16 index.
        int64_t  uti = 0;    // UText position, native index.
        int32_t  cpi;        // char32 position (code point index) 
        UChar32  usc;        // code point from Unicode String
        UChar32  utc;        // code point from UText
        utext_setNativeIndex(targetUT, 0);
        for (cpi=0; ; cpi++) {
            usc = targetUS.char32At(usi);
            utc = utext_next32(targetUT);
            if (utc < 0) {
                break;
            }
            TEST_ASSERT(uti == usi);
            TEST_ASSERT(utc == usc);
            usi = targetUS.moveIndex32(usi, 1);
            uti = utext_getNativeIndex(targetUT);
            if (gFailed) {
                goto cleanupAndReturn;
            }
        }
        int64_t expectedNativeLength = utext_nativeLength(ut);
        if (move == FALSE) {
            expectedNativeLength += nativeLimit - nativeStart;
        }
        uti = utext_getNativeIndex(targetUT);
        TEST_ASSERT(uti == expectedNativeLength);
    }

cleanupAndReturn:
    utext_close(targetUT);
}
    

//
//  TestReplace   Test a single Replace operation.
//
void UTextTest::TestReplace(
            const UnicodeString &us,     // reference UnicodeString in which to do the replace 
            UText         *ut,                // UnicodeText object under test.
            int32_t       nativeStart,        // Range to be replaced, in UText native units. 
            int32_t       nativeLimit,
            int32_t       u16Start,           // Range to be replaced, in UTF-16 units
            int32_t       u16Limit,           //    for use in the reference UnicodeString.
            const UnicodeString &repStr)      // The replacement string
{
    UErrorCode      status   = U_ZERO_ERROR;
    UText          *targetUT = NULL;
    gTestNum++;
    gFailed = FALSE;

    //
    //  clone the target UText.  The test will be run in the cloned copy
    //  so that we don't alter the original.
    //
    targetUT = utext_clone(NULL, ut, TRUE, FALSE, &status);
    TEST_SUCCESS(status);
    UnicodeString targetUS(us);    // And copy the reference string.

    //
    // Do the replace operation in the Unicode String, to 
    //   produce a reference result.
    //
    targetUS.replace(u16Start, u16Limit-u16Start, repStr);

    //
    // Do the replace on the UText under test
    //
    const UChar *rs = repStr.getBuffer();
    int32_t  rsLen = repStr.length();
    int32_t actualDelta = utext_replace(targetUT, nativeStart, nativeLimit, rs, rsLen, &status);
    int32_t expectedDelta = repStr.length() - (nativeLimit - nativeStart);
    TEST_ASSERT(actualDelta == expectedDelta);

    //
    // Compare the results
    //
    int32_t  usi = 0;    // UnicodeString postion, utf-16 index.
    int64_t  uti = 0;    // UText position, native index.
    int32_t  cpi;        // char32 position (code point index) 
    UChar32  usc;        // code point from Unicode String
    UChar32  utc;        // code point from UText
    int64_t  expectedNativeLength = 0;
    utext_setNativeIndex(targetUT, 0);
    for (cpi=0; ; cpi++) {
        usc = targetUS.char32At(usi);
        utc = utext_next32(targetUT);
        if (utc < 0) {
            break;
        }
        TEST_ASSERT(uti == usi);
        TEST_ASSERT(utc == usc);
        usi = targetUS.moveIndex32(usi, 1);
        uti = utext_getNativeIndex(targetUT);
        if (gFailed) {
            goto cleanupAndReturn;
        }
    }
    expectedNativeLength = utext_nativeLength(ut) + expectedDelta;
    uti = utext_getNativeIndex(targetUT);
    TEST_ASSERT(uti == expectedNativeLength);

cleanupAndReturn:
    utext_close(targetUT);
}

//
//  TestAccess      Test the read only access functions on a UText, including cloning.
//                  The text is accessed in a variety of ways, and compared with
//                  the reference UnicodeString.
//
void UTextTest::TestAccess(const UnicodeString &us, UText *ut, int cpCount, m *cpMap) {
    // Run the standard tests on the caller-supplied UText.
    TestAccessNoClone(us, ut, cpCount, cpMap);

    // Re-run tests on a shallow clone.
    utext_setNativeIndex(ut, 0);
    UErrorCode status = U_ZERO_ERROR;
    UText *shallowClone = utext_clone(NULL, ut, FALSE /*deep*/, FALSE /*readOnly*/, &status);
    TEST_SUCCESS(status);
    TestAccessNoClone(us, shallowClone, cpCount, cpMap);

    //
    // Rerun again on a deep clone.
    // Note that text providers are not required to provide deep cloning,
    //   so unsupported errors are ignored.
    //
    status = U_ZERO_ERROR;
    utext_setNativeIndex(shallowClone, 0);
    UText *deepClone = utext_clone(NULL, shallowClone, TRUE, FALSE, &status);
    utext_close(shallowClone);
    if (status != U_UNSUPPORTED_ERROR) {
        TEST_SUCCESS(status);
        TestAccessNoClone(us, deepClone, cpCount, cpMap);
    }
    utext_close(deepClone);
}
    

//
//  TestAccessNoClone()    Test the read only access functions on a UText.
//                         The text is accessed in a variety of ways, and compared with
//                         the reference UnicodeString.
//
void UTextTest::TestAccessNoClone(const UnicodeString &us, UText *ut, int cpCount, m *cpMap) {
    UErrorCode  status = U_ZERO_ERROR;
    gTestNum++;

    //
    //  Check the length from the UText
    //
    int64_t expectedLen = cpMap[cpCount].nativeIdx;
    int64_t utlen = utext_nativeLength(ut);
    TEST_ASSERT(expectedLen == utlen);

    //
    //  Iterate forwards, verify that we get the correct code points
    //   at the correct native offsets.
    //
    int         i = 0;
    int64_t     index;
    int64_t     expectedIndex = 0;
    int64_t     foundIndex = 0;
    UChar32     expectedC;
    UChar32     foundC;
    int64_t     len;

    for (i=0; i<cpCount; i++) {
        expectedIndex = cpMap[i].nativeIdx;
        foundIndex    = utext_getNativeIndex(ut);
        TEST_ASSERT(expectedIndex == foundIndex);
        expectedC     = cpMap[i].cp;
        foundC        = utext_next32(ut);    
        TEST_ASSERT(expectedC == foundC);
        foundIndex    = utext_getPreviousNativeIndex(ut);
        TEST_ASSERT(expectedIndex == foundIndex);
        if (gFailed) {
            return;
        }
    }
    foundC = utext_next32(ut);
    TEST_ASSERT(foundC == U_SENTINEL);
    
    // Repeat above, using macros
    utext_setNativeIndex(ut, 0);
    for (i=0; i<cpCount; i++) {
        expectedIndex = cpMap[i].nativeIdx;
        foundIndex    = UTEXT_GETNATIVEINDEX(ut);
        TEST_ASSERT(expectedIndex == foundIndex);
        expectedC     = cpMap[i].cp;
        foundC        = UTEXT_NEXT32(ut);    
        TEST_ASSERT(expectedC == foundC);
        if (gFailed) {
            return;
        }
    }
    foundC = UTEXT_NEXT32(ut);
    TEST_ASSERT(foundC == U_SENTINEL);

    //
    //  Forward iteration (above) should have left index at the
    //   end of the input, which should == length().
    //
    len = utext_nativeLength(ut);
    foundIndex  = utext_getNativeIndex(ut);
    TEST_ASSERT(len == foundIndex);

    //
    // Iterate backwards over entire test string
    //
    len = utext_getNativeIndex(ut);
    utext_setNativeIndex(ut, len);
    for (i=cpCount-1; i>=0; i--) {
        expectedC     = cpMap[i].cp;
        expectedIndex = cpMap[i].nativeIdx;
        int64_t prevIndex = utext_getPreviousNativeIndex(ut);
        foundC        = utext_previous32(ut);
        foundIndex    = utext_getNativeIndex(ut);
        TEST_ASSERT(expectedIndex == foundIndex);
        TEST_ASSERT(expectedC == foundC);
        TEST_ASSERT(prevIndex == foundIndex);
        if (gFailed) {
            return;
        }
    }

    //
    //  Backwards iteration, above, should have left our iterator
    //   position at zero, and continued backwards iterationshould fail.
    //
    foundIndex = utext_getNativeIndex(ut);
    TEST_ASSERT(foundIndex == 0);
    foundIndex = utext_getPreviousNativeIndex(ut);
    TEST_ASSERT(foundIndex == 0);


    foundC = utext_previous32(ut);
    TEST_ASSERT(foundC == U_SENTINEL);
    foundIndex = utext_getNativeIndex(ut);
    TEST_ASSERT(foundIndex == 0);
    foundIndex = utext_getPreviousNativeIndex(ut);
    TEST_ASSERT(foundIndex == 0);


    // And again, with the macros
    utext_setNativeIndex(ut, len);
    for (i=cpCount-1; i>=0; i--) {
        expectedC     = cpMap[i].cp;
        expectedIndex = cpMap[i].nativeIdx;
        foundC        = UTEXT_PREVIOUS32(ut);
        foundIndex    = UTEXT_GETNATIVEINDEX(ut);
        TEST_ASSERT(expectedIndex == foundIndex);
        TEST_ASSERT(expectedC == foundC);
        if (gFailed) {
            return;
        }
    }

    //
    //  Backwards iteration, above, should have left our iterator
    //   position at zero, and continued backwards iterationshould fail.
    //
    foundIndex = UTEXT_GETNATIVEINDEX(ut);
    TEST_ASSERT(foundIndex == 0);

    foundC = UTEXT_PREVIOUS32(ut);
    TEST_ASSERT(foundC == U_SENTINEL);
    foundIndex = UTEXT_GETNATIVEINDEX(ut);
    TEST_ASSERT(foundIndex == 0);
    if (gFailed) {
        return;
    }

    //
    //  next32From(), prevous32From(), Iterate in a somewhat random order.
    //
    int  cpIndex = 0;
    for (i=0; i<cpCount; i++) {
        cpIndex = (cpIndex + 9973) % cpCount;
        index         = cpMap[cpIndex].nativeIdx;
        expectedC     = cpMap[cpIndex].cp;
        foundC        = utext_next32From(ut, index);
        TEST_ASSERT(expectedC == foundC);
        if (gFailed) {
            return;
        }
    }

    cpIndex = 0;
    for (i=0; i<cpCount; i++) {
        cpIndex = (cpIndex + 9973) % cpCount;
        index         = cpMap[cpIndex+1].nativeIdx;
        expectedC     = cpMap[cpIndex].cp;
        foundC        = utext_previous32From(ut, index);
        TEST_ASSERT(expectedC == foundC);
        if (gFailed) {
            return;
        }
    }


    //
    // moveIndex(int32_t delta);
    //

    // Walk through frontwards, incrementing by one
    utext_setNativeIndex(ut, 0);
    for (i=1; i<=cpCount; i++) {
        utext_moveIndex32(ut, 1);
        index = utext_getNativeIndex(ut);
        expectedIndex = cpMap[i].nativeIdx;
        TEST_ASSERT(expectedIndex == index);
        index = UTEXT_GETNATIVEINDEX(ut);
        TEST_ASSERT(expectedIndex == index);
    }

    // Walk through frontwards, incrementing by two
    utext_setNativeIndex(ut, 0);
    for (i=2; i<cpCount; i+=2) {
        utext_moveIndex32(ut, 2);
        index = utext_getNativeIndex(ut);
        expectedIndex = cpMap[i].nativeIdx;
        TEST_ASSERT(expectedIndex == index);
        index = UTEXT_GETNATIVEINDEX(ut);
        TEST_ASSERT(expectedIndex == index);
    }

    // walk through the string backwards, decrementing by one.
    i = cpMap[cpCount].nativeIdx;
    utext_setNativeIndex(ut, i);
    for (i=cpCount; i>=0; i--) {
        expectedIndex = cpMap[i].nativeIdx;
        index = utext_getNativeIndex(ut);
        TEST_ASSERT(expectedIndex == index);
        index = UTEXT_GETNATIVEINDEX(ut);
        TEST_ASSERT(expectedIndex == index);
        utext_moveIndex32(ut, -1);
    }


    // walk through backwards, decrementing by three
    i = cpMap[cpCount].nativeIdx;
    utext_setNativeIndex(ut, i);
    for (i=cpCount; i>=0; i-=3) {
        expectedIndex = cpMap[i].nativeIdx;
        index = utext_getNativeIndex(ut);
        TEST_ASSERT(expectedIndex == index);
        index = UTEXT_GETNATIVEINDEX(ut);
        TEST_ASSERT(expectedIndex == index);
        utext_moveIndex32(ut, -3);
    }


    //
    // Extract
    //
    int bufSize = us.length() + 10;
    UChar *buf = new UChar[bufSize];
    status = U_ZERO_ERROR;
    expectedLen = us.length();
    len = utext_extract(ut, 0, utlen, buf, bufSize, &status);
    TEST_SUCCESS(status);
    TEST_ASSERT(len == expectedLen);
    int compareResult = us.compare(buf, -1);
    TEST_ASSERT(compareResult == 0);

    status = U_ZERO_ERROR;
    len = utext_extract(ut, 0, utlen, NULL, 0, &status);
    if (utlen == 0) {
        TEST_ASSERT(status == U_STRING_NOT_TERMINATED_WARNING);
    } else {
        TEST_ASSERT(status == U_BUFFER_OVERFLOW_ERROR);
    }
    TEST_ASSERT(len == expectedLen);

    status = U_ZERO_ERROR;
    u_memset(buf, 0x5555, bufSize);
    len = utext_extract(ut, 0, utlen, buf, 1, &status);
    if (us.length() == 0) {
        TEST_SUCCESS(status);
        TEST_ASSERT(buf[0] == 0);
    } else {
        // Buf len == 1, extracting a single 16 bit value.
        // If the data char is supplementary, it doesn't matter whether the buffer remains unchanged,
        //   or whether the lead surrogate of the pair is extracted.
        //   It's a buffer overflow error in either case.
        TEST_ASSERT(buf[0] == us.charAt(0) ||
                    (buf[0] == 0x5555 && U_IS_SUPPLEMENTARY(us.char32At(0))));
        TEST_ASSERT(buf[1] == 0x5555);
        if (us.length() == 1) {
            TEST_ASSERT(status == U_STRING_NOT_TERMINATED_WARNING);
        } else {
            TEST_ASSERT(status == U_BUFFER_OVERFLOW_ERROR);
        }
    }

    delete []buf;
}


//
//  ComparisonTest()    Check the string comparison functions. Based on UnicodeStringTest::TestCompare()
//
void UTextTest::ComparisonTest()
{
    UErrorCode status = U_ZERO_ERROR;
    UnicodeString   test1Str("this is a test");
    UnicodeString   test2Str("this is a test");
    UnicodeString   test3Str("this is a test of the emergency broadcast system");
    UnicodeString   test4Str("never say, \"this is a test\"!!");
    
    UText test1 = UTEXT_INITIALIZER;
    UText test2 = UTEXT_INITIALIZER;
    UText test3 = UTEXT_INITIALIZER;
    UText test4 = UTEXT_INITIALIZER;
    
    UChar        uniChars[] = { 0x74, 0x68, 0x69, 0x73, 0x20, 0x69, 0x73, 
                                0x20, 0x61, 0x20, 0x74, 0x65, 0x73, 0x74, 0 };
    char            chars[] = { 0x74, 0x68, 0x69, 0x73, 0x20, 0x69, 0x73, 
                                0x20, 0x61, 0x20, 0x74, 0x65, 0x73, 0x74, 0 };
    
    UText uniCharText = UTEXT_INITIALIZER;
    UText charText = UTEXT_INITIALIZER;
    
    utext_openUnicodeString(&test1, &test1Str, &status);
    utext_openUnicodeString(&test2, &test2Str, &status);
    utext_openUnicodeString(&test3, &test3Str, &status);
    utext_openUnicodeString(&test4, &test4Str, &status);

    utext_openUChars(&uniCharText, uniChars, -1, &status);
    utext_openUTF8(&charText, chars, -1, &status);
    
    TEST_SUCCESS(status);
        
    // test utext_compare(), simple
    UTEXT_SETNATIVEINDEX(&test1, 0);
    UTEXT_SETNATIVEINDEX(&test2, 0);
    if (utext_compare(&test1, -1, &test2, -1) != 0) errln("utext_compare() failed, simple setup");
    UTEXT_SETNATIVEINDEX(&test1, 0);
    UTEXT_SETNATIVEINDEX(&test3, 0);
    if (utext_compare(&test1, -1, &test3, -1) >= 0) errln("utext_compare() failed, simple setup");
    UTEXT_SETNATIVEINDEX(&test1, 0);
    UTEXT_SETNATIVEINDEX(&test4, 0);
    if (utext_compare(&test1, -1, &test4, -1) <= 0) errln("utext_compare() failed, simple setup");
    
    // test utext_compareNativeLimit(), simple
    UTEXT_SETNATIVEINDEX(&test1, 0);
    UTEXT_SETNATIVEINDEX(&test2, 0);
    if (utext_compareNativeLimit(&test1, -1, &test2, -1) != 0) errln("utext_compareNativeLimit() failed, simple setup");
    UTEXT_SETNATIVEINDEX(&test1, 0);
    UTEXT_SETNATIVEINDEX(&test3, 0);
    if (utext_compareNativeLimit(&test1, -1, &test3, -1) >= 0) errln("utext_compareNativeLimit() failed, simple setup");
    UTEXT_SETNATIVEINDEX(&test1, 0);
    UTEXT_SETNATIVEINDEX(&test4, 0);
    if (utext_compareNativeLimit(&test1, -1, &test4, -1) <= 0) errln("utext_compareNativeLimit() failed, simple setup");
    
    // test utext_compare(), one explicit length
    UTEXT_SETNATIVEINDEX(&test1, 0);
    UTEXT_SETNATIVEINDEX(&test2, 0);
    if (utext_compare(&test1, 14, &test2, -1) != 0) errln("utext_compare() failed, one explicit length");
    UTEXT_SETNATIVEINDEX(&test2, 0);
    UTEXT_SETNATIVEINDEX(&test3, 0);
    if (utext_compare(&test3, 14, &test2, -1) != 0) errln("utext_compare() failed, one explicit length");
    UTEXT_SETNATIVEINDEX(&test2, 0);
    UTEXT_SETNATIVEINDEX(&test4, 12);
    if (utext_compare(&test4, 14, &test2, -1) != 0) errln("utext_compare() failed, one explicit length and offset");
    UTEXT_SETNATIVEINDEX(&test1, 0);
    UTEXT_SETNATIVEINDEX(&test3, 0);
    if (utext_compare(&test3, 18, &test2, -1) <= 0) errln("utext_compare() failed, one explicit length");
    
    // test utext_compareNativeLimit(), one explicit length
    UTEXT_SETNATIVEINDEX(&test1, 0);
    UTEXT_SETNATIVEINDEX(&test2, 0);
    if (utext_compareNativeLimit(&test1, 14, &test2, -1) != 0) errln("utext_compareNativeLimit() failed, one explicit length");
    UTEXT_SETNATIVEINDEX(&test2, 0);
    UTEXT_SETNATIVEINDEX(&test3, 0);
    if (utext_compareNativeLimit(&test3, 14, &test2, -1) != 0) errln("utext_compareNativeLimit() failed, one explicit length");
    UTEXT_SETNATIVEINDEX(&test2, 0);
    UTEXT_SETNATIVEINDEX(&test4, 12);
    if (utext_compareNativeLimit(&test4, 26, &test2, -1) != 0) errln("utext_compareNativeLimit() failed, one explicit length and limit");
    UTEXT_SETNATIVEINDEX(&test1, 0);
    UTEXT_SETNATIVEINDEX(&test3, 0);
    if (utext_compareNativeLimit(&test3, 18, &test2, -1) <= 0) errln("utext_compareNativeLimit() failed, one explicit length");
    
    // test utext_compare(), UChar-based UText
    UTEXT_SETNATIVEINDEX(&uniCharText, 0);
    UTEXT_SETNATIVEINDEX(&test2, 0);
    if (utext_compare(&test2, -1, &uniCharText, -1) != 0) errln("utext_compare() failed, UChar-based UText");
    UTEXT_SETNATIVEINDEX(&uniCharText, 0);
    UTEXT_SETNATIVEINDEX(&test3, 0);
    if (utext_compare(&test3, -1, &uniCharText, -1) <= 0) errln("utext_compare() failed, UChar-based UText");
    UTEXT_SETNATIVEINDEX(&uniCharText, 0);
    UTEXT_SETNATIVEINDEX(&test4, 0);
    if (utext_compare(&test4, -1, &uniCharText, -1) >= 0) errln("utext_compare() failed, UChar-based UText");
    
    // test utext_compareNativeLimit(), UChar-based UText
    UTEXT_SETNATIVEINDEX(&uniCharText, 0);
    UTEXT_SETNATIVEINDEX(&test2, 0);
    if (utext_compareNativeLimit(&test2, -1, &uniCharText, -1) != 0) errln("utext_compareNativeLimit() failed, UChar-based UText");
    UTEXT_SETNATIVEINDEX(&uniCharText, 0);
    UTEXT_SETNATIVEINDEX(&test3, 0);
    if (utext_compareNativeLimit(&test3, -1, &uniCharText, -1) <= 0) errln("utext_compareNativeLimit() failed, UChar-based UText");
    UTEXT_SETNATIVEINDEX(&uniCharText, 0);
    UTEXT_SETNATIVEINDEX(&test4, 0);
    if (utext_compareNativeLimit(&test4, -1, &uniCharText, -1) >= 0) errln("utext_compareNativeLimit() failed, UChar-based UText");
    
    // test utext_compare(), UTF8-based UText
    UTEXT_SETNATIVEINDEX(&charText, 0);
    UTEXT_SETNATIVEINDEX(&test2, 0);
    if (utext_compare(&test2, -1, &charText, -1) != 0) errln("utext_compare() failed, UTF8-based UText");
    UTEXT_SETNATIVEINDEX(&charText, 0);
    UTEXT_SETNATIVEINDEX(&test3, 0);
    if (utext_compare(&test3, -1, &charText, -1) <= 0) errln("utext_compare() failed, UTF8-based UText");
    UTEXT_SETNATIVEINDEX(&charText, 0);
    UTEXT_SETNATIVEINDEX(&test4, 0);
    if (utext_compare(&test4, -1, &charText, -1) >= 0) errln("utext_compare() failed, UTF8-based UText");
    
    // test utext_compareNativeLimit(), UTF8-based UText
    UTEXT_SETNATIVEINDEX(&charText, 0);
    UTEXT_SETNATIVEINDEX(&test2, 0);
    if (utext_compareNativeLimit(&test2, -1, &charText, -1) != 0) errln("utext_compareNativeLimit() failed, UTF8-based UText");
    UTEXT_SETNATIVEINDEX(&charText, 0);
    UTEXT_SETNATIVEINDEX(&test3, 0);
    if (utext_compareNativeLimit(&test3, -1, &charText, -1) <= 0) errln("utext_compareNativeLimit() failed, UTF8-based UText");
    UTEXT_SETNATIVEINDEX(&charText, 0);
    UTEXT_SETNATIVEINDEX(&test4, 0);
    if (utext_compareNativeLimit(&test4, -1, &charText, -1) >= 0) errln("utext_compareNativeLimit() failed, UTF8-based UText");
    
    // test utext_compare(), length
    UTEXT_SETNATIVEINDEX(&test1, 0);
    UTEXT_SETNATIVEINDEX(&test2, 0);
    if (utext_compare(&test1, -1, &test2, 4) != 0) errln("utext_compare() failed, one length");
    UTEXT_SETNATIVEINDEX(&test1, 0);
    UTEXT_SETNATIVEINDEX(&test2, 0);
    if (utext_compare(&test1, 5, &test2, 4) <= 0) errln("utext_compare() failed, both lengths");
    
    // test utext_compareNativeLimit(), limit
    UTEXT_SETNATIVEINDEX(&test1, 0);
    UTEXT_SETNATIVEINDEX(&test2, 0);
    if (utext_compareNativeLimit(&test1, -1, &test2, 4) != 0) errln("utext_compareNativeLimit() failed, one limit");
    UTEXT_SETNATIVEINDEX(&test1, 0);
    UTEXT_SETNATIVEINDEX(&test2, 0);
    if (utext_compareNativeLimit(&test1, 5, &test2, 4) <= 0) errln("utext_compareNativeLimit() failed, both limits");
    
    // test utext_compare(), both explicit offsets and lengths
    UTEXT_SETNATIVEINDEX(&test1, 0);
    UTEXT_SETNATIVEINDEX(&test2, 0);
    if (utext_compare(&test1, 14, &test2, 14) != 0) errln("utext_compare() failed, both explicit offsets and lengths");
    UTEXT_SETNATIVEINDEX(&test1, 0);
    UTEXT_SETNATIVEINDEX(&test3, 0);
    if (utext_compare(&test1, 14, &test3, 14) != 0) errln("utext_compare() failed, both explicit offsets and lengths");
    UTEXT_SETNATIVEINDEX(&test1, 0);
    UTEXT_SETNATIVEINDEX(&test4, 12);
    if (utext_compare(&test1, 14, &test4, 14) != 0) errln("utext_compare() failed, both explicit offsets and lengths");
    UTEXT_SETNATIVEINDEX(&test1, 10);
    UTEXT_SETNATIVEINDEX(&test2, 0);
    if (utext_compare(&test1, 4, &test2, 4) >= 0) errln("utext_compare() failed, both explicit offsets and lengths");
    UTEXT_SETNATIVEINDEX(&test1, 10);
    UTEXT_SETNATIVEINDEX(&test3, 22);
    if (utext_compare(&test1, 4, &test3, 9) <= 0) errln("utext_compare() failed, both explicit offsets and lengths");
    UTEXT_SETNATIVEINDEX(&test1, 10);
    UTEXT_SETNATIVEINDEX(&test4, 22);
    if (utext_compare(&test1, 4, &test4, 4) != 0) errln("utext_compare() failed, both explicit offsets and lengths");
    
    // test utext_compareNativeLimit(), both explicit offsets and limits
    UTEXT_SETNATIVEINDEX(&test1, 0);
    UTEXT_SETNATIVEINDEX(&test2, 0);
    if (utext_compareNativeLimit(&test1, 14, &test2, 14) != 0) errln("utext_compareNativeLimit() failed, both explicit offsets and limits");
    UTEXT_SETNATIVEINDEX(&test1, 0);
    UTEXT_SETNATIVEINDEX(&test3, 0);
    if (utext_compareNativeLimit(&test1, 14, &test3, 14) != 0) errln("utext_compareNativeLimit() failed, both explicit offsets and limits");
    UTEXT_SETNATIVEINDEX(&test1, 0);
    UTEXT_SETNATIVEINDEX(&test4, 12);
    if (utext_compareNativeLimit(&test1, 14, &test4, 26) != 0) errln("utext_compareNativeLimit() failed, both explicit offsets and limits");
    UTEXT_SETNATIVEINDEX(&test1, 10);
    UTEXT_SETNATIVEINDEX(&test2, 0);
    if (utext_compareNativeLimit(&test1, 14, &test2, 4) >= 0) errln("utext_compareNativeLimit() failed, both explicit offsets and limits");
    UTEXT_SETNATIVEINDEX(&test1, 10);
    UTEXT_SETNATIVEINDEX(&test3, 22);
    if (utext_compareNativeLimit(&test1, 14, &test3, 31) <= 0) errln("utext_compareNativeLimit() failed, both explicit offsets and limits");
    UTEXT_SETNATIVEINDEX(&test1, 10);
    UTEXT_SETNATIVEINDEX(&test4, 22);
    if (utext_compareNativeLimit(&test1, 14, &test4, 26) != 0) errln("utext_compareNativeLimit() failed, both explicit offsets and limits");
    
    /* test caseCompare() */
    {
        static const UChar
        _mixed[]=               { 0x61, 0x42, 0x131, 0x3a3, 0xdf,       0x130,       0x49,  0xfb03,           0xd93f, 0xdfff, 0 },
        _otherDefault[]=        { 0x41, 0x62, 0x131, 0x3c3, 0x73, 0x53, 0x69, 0x307, 0x69,  0x46, 0x66, 0x49, 0xd93f, 0xdfff, 0 },
        _otherExcludeSpecialI[]={ 0x41, 0x62, 0x131, 0x3c3, 0x53, 0x73, 0x69,        0x131, 0x66, 0x46, 0x69, 0xd93f, 0xdfff, 0 },
        _different[]=           { 0x41, 0x62, 0x131, 0x3c3, 0x73, 0x53, 0x130,       0x49,  0x46, 0x66, 0x49, 0xd93f, 0xdffd, 0 };
        
        UText
        mixed = UTEXT_INITIALIZER,
        otherDefault = UTEXT_INITIALIZER,
        otherExcludeSpecialI = UTEXT_INITIALIZER,
        different = UTEXT_INITIALIZER;
        
        utext_openUChars(&mixed, _mixed, -1, &status);
        utext_openUChars(&otherDefault, _otherDefault, -1, &status);
        utext_openUChars(&otherExcludeSpecialI, _otherExcludeSpecialI, -1, &status);
        utext_openUChars(&different, _different, -1, &status);
        
        TEST_SUCCESS(status);
        
        int32_t result;
        
        /* test default options */
        UTEXT_SETNATIVEINDEX(&mixed, 0);
        UTEXT_SETNATIVEINDEX(&otherDefault, 0);
        result = utext_caseCompare(&mixed, -1, &otherDefault, -1, U_FOLD_CASE_DEFAULT, &status);
        if (0 != result || U_FAILURE(status)) {
            errln("error: utext_caseCompare (other, default) gives %ld (should be 0) (%s)\n", result, u_errorName(status));
        }
        UTEXT_SETNATIVEINDEX(&mixed, 0);
        UTEXT_SETNATIVEINDEX(&otherDefault, 0);
        result = utext_caseCompareNativeLimit(&mixed, -1, &otherDefault, -1, U_FOLD_CASE_DEFAULT, &status);
        if (0 != result || U_FAILURE(status)) {
            errln("error: utext_caseCompareNativeLimit (other, default) gives %ld (should be 0) (%s)\n", result, u_errorName(status));
        }
        
        /* test excluding special I */
        UTEXT_SETNATIVEINDEX(&mixed, 0);
        UTEXT_SETNATIVEINDEX(&otherExcludeSpecialI, 0);
        result = utext_caseCompare(&mixed, -1, &otherExcludeSpecialI, -1, U_FOLD_CASE_EXCLUDE_SPECIAL_I, &status);
        if (0 != result || U_FAILURE(status)) {
            errln("error: utext_caseCompare (otherExcludeSpecialI, U_FOLD_CASE_EXCLUDE_SPECIAL_I) gives %ld (should be 0) (%s)\n", result, u_errorName(status));
        }
        UTEXT_SETNATIVEINDEX(&mixed, 0);
        UTEXT_SETNATIVEINDEX(&otherExcludeSpecialI, 0);
        result = utext_caseCompareNativeLimit(&mixed, -1, &otherExcludeSpecialI, -1, U_FOLD_CASE_EXCLUDE_SPECIAL_I, &status);
        if (0 != result || U_FAILURE(status)) {
            errln("error: utext_caseCompareNativeLimit (otherExcludeSpecialI, U_FOLD_CASE_EXCLUDE_SPECIAL_I) gives %ld (should be 0) (%s)\n", result, u_errorName(status));
        }
        UTEXT_SETNATIVEINDEX(&mixed, 0);
        UTEXT_SETNATIVEINDEX(&otherDefault, 0);
        result = utext_caseCompare(&mixed, -1, &otherDefault, -1, U_FOLD_CASE_EXCLUDE_SPECIAL_I, &status);
        if (0 == result || U_FAILURE(status)) {
            errln("error: utext_caseCompare (other, U_FOLD_CASE_EXCLUDE_SPECIAL_I) gives %ld (should be nonzero) (%s)\n", result, u_errorName(status));
        }
        UTEXT_SETNATIVEINDEX(&mixed, 0);
        UTEXT_SETNATIVEINDEX(&otherDefault, 0);
        result = utext_caseCompareNativeLimit(&mixed, -1, &otherDefault, -1, U_FOLD_CASE_EXCLUDE_SPECIAL_I, &status);
        if (0 == result || U_FAILURE(status)) {
            errln("error: utext_caseCompareNativeLimit (other, U_FOLD_CASE_EXCLUDE_SPECIAL_I) gives %ld (should be nonzero) (%s)\n", result, u_errorName(status));
        }
        
        /* test against different string */
        UTEXT_SETNATIVEINDEX(&mixed, 0);
        UTEXT_SETNATIVEINDEX(&different, 0);
        result = utext_caseCompare(&mixed, -1, &different, -1, U_FOLD_CASE_DEFAULT, &status);
        if (0 >= result || U_FAILURE(status)) {
            errln("error: utext_caseCompare (different, default) gives %ld (should be positive) (%s)\n", result, u_errorName(status));
        }
        UTEXT_SETNATIVEINDEX(&mixed, 0);
        UTEXT_SETNATIVEINDEX(&different, 0);
        result = utext_caseCompareNativeLimit(&mixed, -1, &different, -1, U_FOLD_CASE_DEFAULT, &status);
        if (0 >= result || U_FAILURE(status)) {
            errln("error: utext_caseCompareNativeLimit (different, default) gives %ld (should be positive) (%s)\n", result, u_errorName(status));
        }

        /* test caseCompare() - include the folded sharp s (U+00df) with different lengths */
        UTEXT_SETNATIVEINDEX(&mixed, 1);
        UTEXT_SETNATIVEINDEX(&different, 1);
        result = utext_caseCompare(&mixed, 4, &different, 5, U_FOLD_CASE_DEFAULT, &status);
        if (0 != result || U_FAILURE(status)) {
            errln("error: utext_caseCompare (mixed[1-5), different[1-6), default) gives %ld (should be 0) (%s)\n", result, u_errorName(status));
        }
        UTEXT_SETNATIVEINDEX(&mixed, 1);
        UTEXT_SETNATIVEINDEX(&different, 1);
        result = utext_caseCompareNativeLimit(&mixed, 5, &different, 6, U_FOLD_CASE_DEFAULT, &status);
        if (0 != result || U_FAILURE(status)) {
            errln("error: utext_caseCompareNativeLimit (mixed[1-5), different[1-6), default) gives %ld (should be 0) (%s)\n", result, u_errorName(status));
        }

        /* test caseCompare() - stop in the middle of the sharp s (U+00df) */
        UTEXT_SETNATIVEINDEX(&mixed, 1);
        UTEXT_SETNATIVEINDEX(&different, 1);
        result = utext_caseCompare(&mixed, 4, &different, 4, U_FOLD_CASE_DEFAULT, &status);
        if (0 >= result || U_FAILURE(status)) {
            errln("error: utext_caseCompare (mixed[1-5), different[1-5), default) gives %ld (should be positive) (%s)\n", result, u_errorName(status));
        }
        UTEXT_SETNATIVEINDEX(&mixed, 1);
        UTEXT_SETNATIVEINDEX(&different, 1);
        result = utext_caseCompareNativeLimit(&mixed, 5, &different, 5, U_FOLD_CASE_DEFAULT, &status);
        if (0 >= result || U_FAILURE(status)) {
            errln("error: utext_caseCompareNativeLimit (mixed[1-5), different[1-5), default) gives %ld (should be positive) (%s)\n", result, u_errorName(status));
        }
    }
    
    /* test surrogates in comparison */
    {
        static const UChar
        _before[] = { 0x65, 0xd800, 0xd800, 0xdc01, 0x65, 0x00 },
        _after[]  = { 0x65, 0xd800, 0xdc00, 0x65, 0x00 };
        
        UText
        before = UTEXT_INITIALIZER,
        after  = UTEXT_INITIALIZER;
        
        utext_openUChars(&before, _before, -1, &status);
        utext_openUChars(&after, _after, -1, &status);
        
        TEST_SUCCESS(status);
        int32_t result;
        
        UTEXT_SETNATIVEINDEX(&before, 1);
        UTEXT_SETNATIVEINDEX(&after, 1);
        result = utext_compare(&before, -1, &after, -1);
        if (0 <= result || U_FAILURE(status)) {
            errln("error: utext_compare ({ 65, d800, 10001, 65 }, { 65, 10000, 65 }) gives %ld (should be negative) (%s)\n", result, u_errorName(status));
        }
        
        UTEXT_SETNATIVEINDEX(&before, 1);
        UTEXT_SETNATIVEINDEX(&after, 1);
        result = utext_compare(&before, 3, &after, 3);
        if (0 <= result || U_FAILURE(status)) {
            errln("error: utext_compare with lengths ({ 65, d800, 10001, 65 }, { 65, 10000, 65 }) gives %ld (should be negative) (%s)\n", result, u_errorName(status));
        }
        
        UTEXT_SETNATIVEINDEX(&before, 1);
        UTEXT_SETNATIVEINDEX(&after, 1);
        result = utext_caseCompare(&before, -1, &after, -1, U_FOLD_CASE_DEFAULT, &status);
        if (0 <= result || U_FAILURE(status)) {
            errln("error: utext_caseCompare ({ 65, d800, 10001, 65 }, { 65, 10000, 65 }) gives %ld (should be negative) (%s)\n", result, u_errorName(status));
        }
        
        UTEXT_SETNATIVEINDEX(&before, 1);
        UTEXT_SETNATIVEINDEX(&after, 1);
        result = utext_caseCompare(&before, 3, &after, 3, U_FOLD_CASE_DEFAULT, &status);
        if (0 <= result || U_FAILURE(status)) {
            errln("error: utext_caseCompare with lengths ({ 65, d800, 10001, 65 }, { 65, 10000, 65 }) gives %ld (should be negative) (%s)\n", result, u_errorName(status));
        }
        
        utext_close(&before);
        utext_close(&after);
    }
    
    /* test surrogates at end of string */
    {
        static const UChar
        _before[] = { 0x65, 0xd800, 0xd800, 0xdc01, 0x00 },
        _after[]  = { 0x65, 0xd800, 0xdc00, 0x00 };
        
        UText
        before = UTEXT_INITIALIZER,
        after  = UTEXT_INITIALIZER;
        
        utext_openUChars(&before, _before, -1, &status);
        utext_openUChars(&after, _after, -1, &status);
        
        TEST_SUCCESS(status);
        int32_t result;
        
        UTEXT_SETNATIVEINDEX(&before, 1);
        UTEXT_SETNATIVEINDEX(&after, 1);
        result = utext_compare(&before, -1, &after, -1);
        if (0 <= result || U_FAILURE(status)) {
            errln("error: utext_compare ({ 65, d800, 10001 }, { 65, 10000 }) gives %ld (should be negative) (%s)\n", result, u_errorName(status));
        }
        
        UTEXT_SETNATIVEINDEX(&before, 1);
        UTEXT_SETNATIVEINDEX(&after, 1);
        result = utext_caseCompare(&before, -1, &after, -1, U_FOLD_CASE_DEFAULT, &status);
        if (0 <= result || U_FAILURE(status)) {
            errln("error: utext_caseCompare ({ 65, d800, 10001 }, { 65, 10000 }) gives %ld (should be negative) (%s)\n", result, u_errorName(status));
        }
        
        utext_close(&before);
        utext_close(&after);
    }
    
    /* test empty strings */
    {
        UChar zero16 = 0;
        char zero8 = 0;
        UText emptyUChar = UTEXT_INITIALIZER;
        UText emptyUTF8 = UTEXT_INITIALIZER;
        UText nullUChar = UTEXT_INITIALIZER;
        UText nullUTF8 = UTEXT_INITIALIZER;
        
        utext_openUChars(&emptyUChar, &zero16, -1, &status);
        utext_openUTF8(&emptyUTF8, &zero8, -1, &status);
        utext_openUChars(&nullUChar, NULL, 0, &status);
        utext_openUTF8(&nullUTF8, NULL, 0, &status);
        
        if (utext_compare(&emptyUChar, -1, &emptyUTF8, -1) != 0) {
            errln("error: utext_compare(&emptyUChar, -1, &emptyUTF8, -1) != 0");
        }
        if (utext_compare(&emptyUChar, -1, &nullUChar, -1) != 0) {
            errln("error: utext_compare(&emptyUChar, -1, &nullUChar, -1) != 0");
        }
        if (utext_compare(&emptyUChar, -1, &nullUTF8, -1) != 0) {
            errln("error: utext_compare(&emptyUChar, -1, &nullUTF8, -1) != 0");
        }
        if (utext_compare(&emptyUTF8, -1, &nullUChar, -1) != 0) {
            errln("error: utext_compare(&emptyUTF8, -1, &nullUChar, -1) != 0");
        }
        if (utext_compare(&emptyUTF8, -1, &nullUTF8, -1) != 0) {
            errln("error: utext_compare(&emptyUTF8, -1, &nullUTF8, -1) != 0");
        }
        if (utext_compare(&nullUChar, -1, &nullUTF8, -1) != 0) {
            errln("error: utext_compare(&nullUChar, -1, &nullUTF8, -1) != 0");
        }

        if (utext_compareNativeLimit(&emptyUChar, -1, &emptyUTF8, -1) != 0) {
            errln("error: utext_compareNativeLimit(&emptyUChar, -1, &emptyUTF8, -1) != 0");
        }
        if (utext_compareNativeLimit(&emptyUChar, -1, &nullUChar, -1) != 0) {
            errln("error: utext_compareNativeLimit(&emptyUChar, -1, &nullUChar, -1) != 0");
        }
        if (utext_compareNativeLimit(&emptyUChar, -1, &nullUTF8, -1) != 0) {
            errln("error: utext_compareNativeLimit(&emptyUChar, -1, &nullUTF8, -1) != 0");
        }
        if (utext_compareNativeLimit(&emptyUTF8, -1, &nullUChar, -1) != 0) {
            errln("error: utext_compareNativeLimit(&emptyUTF8, -1, &nullUChar, -1) != 0");
        }
        if (utext_compareNativeLimit(&emptyUTF8, -1, &nullUTF8, -1) != 0) {
            errln("error: utext_compareNativeLimit(&emptyUTF8, -1, &nullUTF8, -1) != 0");
        }
        if (utext_compareNativeLimit(&nullUChar, -1, &nullUTF8, -1) != 0) {
            errln("error: utext_compareNativeLimit(&nullUChar, -1, &nullUTF8, -1) != 0");
        }

        if (utext_caseCompare(&emptyUChar, -1, &emptyUTF8, -1, 0, &status) != 0) {
            errln("error: utext_caseCompare(&emptyUChar, -1, &emptyUTF8, -1, 0, &status) != 0");
        }
        if (utext_caseCompare(&emptyUChar, -1, &nullUChar, -1, 0, &status) != 0) {
            errln("error: utext_caseCompare(&emptyUChar, -1, &nullUChar, -1, 0, &status) != 0");
        }
        if (utext_caseCompare(&emptyUChar, -1, &nullUTF8, -1, 0, &status) != 0) {
            errln("error: utext_caseCompare(&emptyUChar, -1, &nullUTF8, -1, 0, &status) != 0");
        }
        if (utext_caseCompare(&emptyUTF8, -1, &nullUChar, -1, 0, &status) != 0) {
            errln("error: utext_caseCompare(&emptyUTF8, -1, &nullUChar, -1, 0, &status) != 0");
        }
        if (utext_caseCompare(&emptyUTF8, -1, &nullUTF8, -1, 0, &status) != 0) {
            errln("error: utext_caseCompare(&emptyUTF8, -1, &nullUTF8, -1, 0, &status) != 0");
        }
        if (utext_caseCompare(&nullUChar, -1, &nullUTF8, -1, 0, &status) != 0) {
            errln("error: utext_caseCompare(&nullUChar, -1, &nullUTF8, -1, 0, &status) != 0");
        }

        if (utext_caseCompareNativeLimit(&emptyUChar, -1, &emptyUTF8, -1, 0, &status) != 0) {
            errln("error: utext_caseCompareNativeLimit(&emptyUChar, -1, &emptyUTF8, -1, 0, &status) != 0");
        }
        if (utext_caseCompareNativeLimit(&emptyUChar, -1, &nullUChar, -1, 0, &status) != 0) {
            errln("error: utext_caseCompareNativeLimit(&emptyUChar, -1, &nullUChar, -1, 0, &status) != 0");
        }
        if (utext_caseCompareNativeLimit(&emptyUChar, -1, &nullUTF8, -1, 0, &status) != 0) {
            errln("error: utext_caseCompareNativeLimit(&emptyUChar, -1, &nullUTF8, -1, 0, &status) != 0");
        }
        if (utext_caseCompareNativeLimit(&emptyUTF8, -1, &nullUChar, -1, 0, &status) != 0) {
            errln("error: utext_caseCompareNativeLimit(&emptyUTF8, -1, &nullUChar, -1, 0, &status) != 0");
        }
        if (utext_caseCompareNativeLimit(&emptyUTF8, -1, &nullUTF8, -1, 0, &status) != 0) {
            errln("error: utext_caseCompareNativeLimit(&emptyUTF8, -1, &nullUTF8, -1, 0, &status) != 0");
        }
        if (utext_caseCompareNativeLimit(&nullUChar, -1, &nullUTF8, -1, 0, &status) != 0) {
            errln("error: utext_caseCompareNativeLimit(&nullUChar, -1, &nullUTF8, -1, 0, &status) != 0");
        }
        
        utext_close(&emptyUChar);
        utext_close(&emptyUTF8);
        utext_close(&nullUChar);
        utext_close(&nullUTF8);
        utext_close(&charText);
        utext_close(&uniCharText);
    }
}



//
//  ErrorTest()    Check various error and edge cases.
//
void UTextTest::ErrorTest() 
{
    // Close of an unitialized UText.  Shouldn't blow up.
    {
        UText  ut;  
        memset(&ut, 0, sizeof(UText));
        utext_close(&ut);
        utext_close(NULL);
    }

    // Double-close of a UText.  Shouldn't blow up.  UText should still be usable.
    {
        UErrorCode status = U_ZERO_ERROR;
        UText ut = UTEXT_INITIALIZER;
        UnicodeString s("Hello, World");
        UText *ut2 = utext_openUnicodeString(&ut, &s, &status);
        TEST_SUCCESS(status);
        TEST_ASSERT(ut2 == &ut);

        UText *ut3 = utext_close(&ut);
        TEST_ASSERT(ut3 == &ut);

        UText *ut4 = utext_close(&ut);
        TEST_ASSERT(ut4 == &ut);

        utext_openUnicodeString(&ut, &s, &status);
        TEST_SUCCESS(status);
        utext_close(&ut);
    }

    // Re-use of a UText, chaining through each of the types of UText
    //   (If it doesn't blow up, and doesn't leak, it's probably working fine)
    {
        UErrorCode status = U_ZERO_ERROR;
        UText ut = UTEXT_INITIALIZER;
        UText  *utp;
        UnicodeString s1("Hello, World");
        UChar s2[] = {(UChar)0x41, (UChar)0x42, (UChar)0};
        const char  *s3 = "\x66\x67\x68";

        utp = utext_openUnicodeString(&ut, &s1, &status);
        TEST_SUCCESS(status);
        TEST_ASSERT(utp == &ut);

        utp = utext_openConstUnicodeString(&ut, &s1, &status);
        TEST_SUCCESS(status);
        TEST_ASSERT(utp == &ut);

        utp = utext_openUTF8(&ut, s3, -1, &status);
        TEST_SUCCESS(status);
        TEST_ASSERT(utp == &ut);

        utp = utext_openUChars(&ut, s2, -1, &status);
        TEST_SUCCESS(status);
        TEST_ASSERT(utp == &ut);

        utp = utext_close(&ut);
        TEST_ASSERT(utp == &ut);

        utp = utext_openUnicodeString(&ut, &s1, &status);
        TEST_SUCCESS(status);
        TEST_ASSERT(utp == &ut);
    }

    // Invalid parameters on open
    //
    {
        UErrorCode status = U_ZERO_ERROR;
        UText ut = UTEXT_INITIALIZER;
        
        utext_openUChars(&ut, NULL, 5, &status);
        TEST_ASSERT(status == U_ILLEGAL_ARGUMENT_ERROR);

        status = U_ZERO_ERROR;
        utext_openUChars(&ut, NULL, -1, &status);
        TEST_ASSERT(status == U_ILLEGAL_ARGUMENT_ERROR);

        status = U_ZERO_ERROR;
        utext_openUTF8(&ut, NULL, 4, &status);
        TEST_ASSERT(status == U_ILLEGAL_ARGUMENT_ERROR);

        status = U_ZERO_ERROR;
        utext_openUTF8(&ut, NULL, -1, &status);
        TEST_ASSERT(status == U_ILLEGAL_ARGUMENT_ERROR);
    }

    //
    //  UTF-8 with malformed sequences.
    //    These should come through as the Unicode replacement char, \ufffd
    //
    {
        UErrorCode status = U_ZERO_ERROR;
        UText *ut = NULL;
        const char *badUTF8 = "\x41\x81\x42\xf0\x81\x81\x43";   
        UChar32  c;

        ut = utext_openUTF8(NULL, badUTF8, -1, &status);
        TEST_SUCCESS(status);
        c = utext_char32At(ut, 1);
        TEST_ASSERT(c == 0xfffd);
        c = utext_char32At(ut, 3);
        TEST_ASSERT(c == 0xfffd);
        c = utext_char32At(ut, 5);
        TEST_ASSERT(c == 0xfffd);
        c = utext_char32At(ut, 6);
        TEST_ASSERT(c == 0x43);

        UChar buf[10];
        int n = utext_extract(ut, 0, 9, buf, 10, &status);
        TEST_SUCCESS(status);
        TEST_ASSERT(n==5);
        TEST_ASSERT(buf[1] == 0xfffd);
        TEST_ASSERT(buf[3] == 0xfffd);
        TEST_ASSERT(buf[2] == 0x42);
        utext_close(ut);
    }


    //
    //  isLengthExpensive - does it make the exptected transitions after
    //                      getting the length of a nul terminated string?
    //
    {
        UErrorCode status = U_ZERO_ERROR;
        UnicodeString sa("Hello, this is a string");
        UBool  isExpensive;

        UChar sb[100];
        memset(sb, 0x20, sizeof(sb));
        sb[99] = 0;

        UText *uta = utext_openUnicodeString(NULL, &sa, &status);
        TEST_SUCCESS(status);
        isExpensive = utext_isLengthExpensive(uta);
        TEST_ASSERT(isExpensive == FALSE);
        utext_close(uta);

        UText *utb = utext_openUChars(NULL, sb, -1, &status);
        TEST_SUCCESS(status);
        isExpensive = utext_isLengthExpensive(utb);
        TEST_ASSERT(isExpensive == TRUE);
        int64_t  len = utext_nativeLength(utb);
        TEST_ASSERT(len == 99);
        isExpensive = utext_isLengthExpensive(utb);
        TEST_ASSERT(isExpensive == FALSE);
        utext_close(utb);
    }

    //
    // Index to positions not on code point boundaries.
    //
    {
        const char *u8str =         "\xc8\x81\xe1\x82\x83\xf1\x84\x85\x86";
        int32_t startMap[] =        {   0,  0,  2,  2,  2,  5,  5,  5,  5,  9,  9};
        int32_t nextMap[]  =        {   2,  2,  5,  5,  5,  9,  9,  9,  9,  9,  9};
        int32_t prevMap[]  =        {   0,  0,  0,  0,  0,  2,  2,  2,  2,  5,  5};
        UChar32  c32Map[] =    {0x201, 0x201, 0x1083, 0x1083, 0x1083, 0x044146, 0x044146, 0x044146, 0x044146, -1, -1}; 
        UChar32  pr32Map[] =   {    -1,   -1,  0x201,  0x201,  0x201,   0x1083,   0x1083,   0x1083,   0x1083, 0x044146, 0x044146}; 

        // extractLen is the size, in UChars, of what will be extracted between index and index+1.
        //  is zero when both index positions lie within the same code point.
        int32_t  exLen[] =          {   0,  1,   0,  0,  1,  0,  0,  0,  2,  0,  0};


        UErrorCode status = U_ZERO_ERROR;
        UText *ut = utext_openUTF8(NULL, u8str, -1, &status);
        TEST_SUCCESS(status);

        // Check setIndex
        int32_t i;
        int32_t startMapLimit = sizeof(startMap) / sizeof(int32_t);
        for (i=0; i<startMapLimit; i++) {
            utext_setNativeIndex(ut, i);
            int64_t cpIndex = utext_getNativeIndex(ut);
            TEST_ASSERT(cpIndex == startMap[i]);
            cpIndex = UTEXT_GETNATIVEINDEX(ut);
            TEST_ASSERT(cpIndex == startMap[i]);
        }

        // Check char32At
        for (i=0; i<startMapLimit; i++) {
            UChar32 c32 = utext_char32At(ut, i);
            TEST_ASSERT(c32 == c32Map[i]);
            int64_t cpIndex = utext_getNativeIndex(ut);
            TEST_ASSERT(cpIndex == startMap[i]);
        }

        // Check utext_next32From
        for (i=0; i<startMapLimit; i++) {
            UChar32 c32 = utext_next32From(ut, i);
            TEST_ASSERT(c32 == c32Map[i]);
            int64_t cpIndex = utext_getNativeIndex(ut);
            TEST_ASSERT(cpIndex == nextMap[i]);
        }
        
        // check utext_previous32From
        for (i=0; i<startMapLimit; i++) {
            gTestNum++;
            UChar32 c32 = utext_previous32From(ut, i);
            TEST_ASSERT(c32 == pr32Map[i]);
            int64_t cpIndex = utext_getNativeIndex(ut);
            TEST_ASSERT(cpIndex == prevMap[i]);
        }

        // check Extract
        //   Extract from i to i+1, which may be zero or one code points,
        //     depending on whether the indices straddle a cp boundary.
        for (i=0; i<startMapLimit; i++) {
            UChar buf[3];
            status = U_ZERO_ERROR;
            int32_t  extractedLen = utext_extract(ut, i, i+1, buf, 3, &status);
            TEST_SUCCESS(status);
            TEST_ASSERT(extractedLen == exLen[i]);
            if (extractedLen > 0) {
                UChar32  c32;
                /* extractedLen-extractedLen == 0 is used to get around a compiler warning. */
                U16_GET(buf, 0, extractedLen-extractedLen, extractedLen, c32);
                TEST_ASSERT(c32 == c32Map[i]);
            }
        }

        utext_close(ut);
    }


    {    //  Similar test, with utf16 instead of utf8
         //  TODO:  merge the common parts of these tests.
        
        UnicodeString u16str("\\u1000\\U00011000\\u2000\\U00022000", -1, US_INV);
        int32_t startMap[]  ={ 0,     1,   1,    3,     4,  4,     6,  6};
        int32_t nextMap[]  = { 1,     3,   3,    4,     6,  6,     6,  6};
        int32_t prevMap[]  = { 0,     0,   0,    1,     3,  3,     4,  4};
        UChar32  c32Map[] =  {0x1000, 0x11000, 0x11000, 0x2000,  0x22000, 0x22000, -1, -1}; 
        UChar32  pr32Map[] = {    -1, 0x1000,  0x1000,  0x11000, 0x2000,  0x2000,   0x22000,   0x22000}; 
        int32_t  exLen[] =   {   1,  0,   2,  1,  0,  2,  0,  0,};

        u16str = u16str.unescape();
        UErrorCode status = U_ZERO_ERROR;
        UText *ut = utext_openUnicodeString(NULL, &u16str, &status);
        TEST_SUCCESS(status);

        int32_t startMapLimit = sizeof(startMap) / sizeof(int32_t);
        int i;
        for (i=0; i<startMapLimit; i++) {
            utext_setNativeIndex(ut, i);
            int64_t cpIndex = utext_getNativeIndex(ut);
            TEST_ASSERT(cpIndex == startMap[i]);
        }

        // Check char32At
        for (i=0; i<startMapLimit; i++) {
            UChar32 c32 = utext_char32At(ut, i);
            TEST_ASSERT(c32 == c32Map[i]);
            int64_t cpIndex = utext_getNativeIndex(ut);
            TEST_ASSERT(cpIndex == startMap[i]);
        }

        // Check utext_next32From
        for (i=0; i<startMapLimit; i++) {
            UChar32 c32 = utext_next32From(ut, i);
            TEST_ASSERT(c32 == c32Map[i]);
            int64_t cpIndex = utext_getNativeIndex(ut);
            TEST_ASSERT(cpIndex == nextMap[i]);
        }
        
        // check utext_previous32From
        for (i=0; i<startMapLimit; i++) {
            UChar32 c32 = utext_previous32From(ut, i);
            TEST_ASSERT(c32 == pr32Map[i]);
            int64_t cpIndex = utext_getNativeIndex(ut);
            TEST_ASSERT(cpIndex == prevMap[i]);
        }

        // check Extract
        //   Extract from i to i+1, which may be zero or one code points,
        //     depending on whether the indices straddle a cp boundary.
        for (i=0; i<startMapLimit; i++) {
            UChar buf[3];
            status = U_ZERO_ERROR;
            int32_t  extractedLen = utext_extract(ut, i, i+1, buf, 3, &status);
            TEST_SUCCESS(status);
            TEST_ASSERT(extractedLen == exLen[i]);
            if (extractedLen > 0) {
                UChar32  c32;
                /* extractedLen-extractedLen == 0 is used to get around a compiler warning. */
                U16_GET(buf, 0, extractedLen-extractedLen, extractedLen, c32);
                TEST_ASSERT(c32 == c32Map[i]);
            }
        }

        utext_close(ut);
    }

    {    //  Similar test, with UText over Replaceable
         //  TODO:  merge the common parts of these tests.
        
        UnicodeString u16str("\\u1000\\U00011000\\u2000\\U00022000", -1, US_INV);
        int32_t startMap[]  ={ 0,     1,   1,    3,     4,  4,     6,  6};
        int32_t nextMap[]  = { 1,     3,   3,    4,     6,  6,     6,  6};
        int32_t prevMap[]  = { 0,     0,   0,    1,     3,  3,     4,  4};
        UChar32  c32Map[] =  {0x1000, 0x11000, 0x11000, 0x2000,  0x22000, 0x22000, -1, -1}; 
        UChar32  pr32Map[] = {    -1, 0x1000,  0x1000,  0x11000, 0x2000,  0x2000,   0x22000,   0x22000}; 
        int32_t  exLen[] =   {   1,  0,   2,  1,  0,  2,  0,  0,};

        u16str = u16str.unescape();
        UErrorCode status = U_ZERO_ERROR;
        UText *ut = utext_openReplaceable(NULL, &u16str, &status);
        TEST_SUCCESS(status);

        int32_t startMapLimit = sizeof(startMap) / sizeof(int32_t);
        int i;
        for (i=0; i<startMapLimit; i++) {
            utext_setNativeIndex(ut, i);
            int64_t cpIndex = utext_getNativeIndex(ut);
            TEST_ASSERT(cpIndex == startMap[i]);
        }

        // Check char32At
        for (i=0; i<startMapLimit; i++) {
            UChar32 c32 = utext_char32At(ut, i);
            TEST_ASSERT(c32 == c32Map[i]);
            int64_t cpIndex = utext_getNativeIndex(ut);
            TEST_ASSERT(cpIndex == startMap[i]);
        }

        // Check utext_next32From
        for (i=0; i<startMapLimit; i++) {
            UChar32 c32 = utext_next32From(ut, i);
            TEST_ASSERT(c32 == c32Map[i]);
            int64_t cpIndex = utext_getNativeIndex(ut);
            TEST_ASSERT(cpIndex == nextMap[i]);
        }
        
        // check utext_previous32From
        for (i=0; i<startMapLimit; i++) {
            UChar32 c32 = utext_previous32From(ut, i);
            TEST_ASSERT(c32 == pr32Map[i]);
            int64_t cpIndex = utext_getNativeIndex(ut);
            TEST_ASSERT(cpIndex == prevMap[i]);
        }

        // check Extract
        //   Extract from i to i+1, which may be zero or one code points,
        //     depending on whether the indices straddle a cp boundary.
        for (i=0; i<startMapLimit; i++) {
            UChar buf[3];
            status = U_ZERO_ERROR;
            int32_t  extractedLen = utext_extract(ut, i, i+1, buf, 3, &status);
            TEST_SUCCESS(status);
            TEST_ASSERT(extractedLen == exLen[i]);
            if (extractedLen > 0) {
                UChar32  c32;
                /* extractedLen-extractedLen == 0 is used to get around a compiler warning. */
                U16_GET(buf, 0, extractedLen-extractedLen, extractedLen, c32);
                TEST_ASSERT(c32 == c32Map[i]);
            }
        }

        utext_close(ut);
    }
}


void UTextTest::FreezeTest() {
    // Check isWritable() and freeze() behavior.
    //

    UnicodeString  ustr("Hello, World.");
    const char u8str[] = {char(0x31), (char)0x32, (char)0x33, 0};  
    const UChar u16str[] = {(UChar)0x31, (UChar)0x32, (UChar)0x44, 0};

    UErrorCode status = U_ZERO_ERROR;
    UText  *ut        = NULL;
    UText  *ut2       = NULL;

    ut = utext_openUTF8(ut, u8str, -1, &status);
    TEST_SUCCESS(status);
    UBool writable = utext_isWritable(ut);
    TEST_ASSERT(writable == FALSE);
    utext_copy(ut, 1, 2, 0, TRUE, &status);
    TEST_ASSERT(status == U_NO_WRITE_PERMISSION);

    status = U_ZERO_ERROR;
    ut = utext_openUChars(ut, u16str, -1, &status);
    TEST_SUCCESS(status);
    writable = utext_isWritable(ut);
    TEST_ASSERT(writable == FALSE);
    utext_copy(ut, 1, 2, 0, TRUE, &status);
    TEST_ASSERT(status == U_NO_WRITE_PERMISSION);

    status = U_ZERO_ERROR;
    ut = utext_openUnicodeString(ut, &ustr, &status);
    TEST_SUCCESS(status);
    writable = utext_isWritable(ut);
    TEST_ASSERT(writable == TRUE);
    utext_freeze(ut);
    writable = utext_isWritable(ut);
    TEST_ASSERT(writable == FALSE);
    utext_copy(ut, 1, 2, 0, TRUE, &status);
    TEST_ASSERT(status == U_NO_WRITE_PERMISSION);
    
    status = U_ZERO_ERROR;
    ut = utext_openUnicodeString(ut, &ustr, &status);
    TEST_SUCCESS(status);
    ut2 = utext_clone(ut2, ut, FALSE, FALSE, &status);  // clone with readonly = false
    TEST_SUCCESS(status);
    writable = utext_isWritable(ut2);
    TEST_ASSERT(writable == TRUE);
    ut2 = utext_clone(ut2, ut, FALSE, TRUE, &status);  // clone with readonly = true
    TEST_SUCCESS(status);
    writable = utext_isWritable(ut2);
    TEST_ASSERT(writable == FALSE);
    utext_copy(ut2, 1, 2, 0, TRUE, &status);
    TEST_ASSERT(status == U_NO_WRITE_PERMISSION);

    status = U_ZERO_ERROR;
    ut = utext_openConstUnicodeString(ut, (const UnicodeString *)&ustr, &status);
    TEST_SUCCESS(status);
    writable = utext_isWritable(ut);
    TEST_ASSERT(writable == FALSE);
    utext_copy(ut, 1, 2, 0, TRUE, &status);
    TEST_ASSERT(status == U_NO_WRITE_PERMISSION);

    // Deep Clone of a frozen UText should re-enable writing in the copy.
    status = U_ZERO_ERROR;
    ut = utext_openUnicodeString(ut, &ustr, &status);
    TEST_SUCCESS(status);
    utext_freeze(ut);
    ut2 = utext_clone(ut2, ut, TRUE, FALSE, &status);   // deep clone
    TEST_SUCCESS(status);
    writable = utext_isWritable(ut2);
    TEST_ASSERT(writable == TRUE);


    // Deep clone of a frozen UText, where the base type is intrinsically non-writable,
    //  should NOT enable writing in the copy.
    status = U_ZERO_ERROR;
    ut = utext_openUChars(ut, u16str, -1, &status);
    TEST_SUCCESS(status);
    utext_freeze(ut);
    ut2 = utext_clone(ut2, ut, TRUE, FALSE, &status);   // deep clone
    TEST_SUCCESS(status);
    writable = utext_isWritable(ut2);
    TEST_ASSERT(writable == FALSE);

    // cleanup
    utext_close(ut);
    utext_close(ut2);
}


//
//  Fragmented UText
//      A UText type that works with a chunk size of 1.
//      Intended to test for edge cases.
//      Input comes from a UnicodeString.
//
//       ut.b    the character.  Put into both halves.
//

U_CDECL_BEGIN
static UBool U_CALLCONV
fragTextAccess(UText *ut, int64_t index, UBool forward) {
    const UnicodeString *us = (const UnicodeString *)ut->context;
    UChar  c;
    int32_t length = us->length();
    if (forward && index>=0 && index<length) {
        c = us->charAt((int32_t)index);
        ut->b = c | c<<16;
        ut->chunkOffset = 0;
        ut->chunkLength = 1;
        ut->chunkNativeStart = index;
        ut->chunkNativeLimit = index+1;
        return true;
    }
    if (!forward && index>0 && index <=length) {
        c = us->charAt((int32_t)index-1);
        ut->b = c | c<<16;
        ut->chunkOffset = 1;
        ut->chunkLength = 1;
        ut->chunkNativeStart = index-1;
        ut->chunkNativeLimit = index;
        return true;
    } 
    ut->b = 0;
    ut->chunkOffset = 0;
    ut->chunkLength = 0;
    if (index <= 0) {
        ut->chunkNativeStart = 0;
        ut->chunkNativeLimit = 0;
    } else {
        ut->chunkNativeStart = length;
        ut->chunkNativeLimit = length;
    }
    return false;
}

// Function table to be used with this fragmented text provider.
//   Initialized in the open function.
static UTextFuncs  fragmentFuncs;

// Clone function for fragmented text provider.
//   Didn't really want to provide this, but it's easier to provide it than to keep it
//   out of the tests.
//
UText *
cloneFragmentedUnicodeString(UText *dest, const UText *src, UBool deep, UErrorCode *status) {
    if (U_FAILURE(*status)) {
        return NULL;
    }
    if (deep) {
        *status = U_UNSUPPORTED_ERROR;
        return NULL;
    }
    dest = utext_openUnicodeString(dest, (UnicodeString *)src->context, status);
    utext_setNativeIndex(dest, utext_getNativeIndex(src));
    return dest;
}

U_CDECL_END

// Open function for the fragmented text provider.
UText *
openFragmentedUnicodeString(UText *ut, UnicodeString *s, UErrorCode *status) {
    ut = utext_openUnicodeString(ut, s, status);
    if (U_FAILURE(*status)) {
        return ut;
    }

    // Copy of the function table from the stock UnicodeString UText,
    //   and replace the entry for the access function.
    memcpy(&fragmentFuncs, ut->pFuncs, sizeof(fragmentFuncs));
    fragmentFuncs.access = fragTextAccess;
    fragmentFuncs.clone  = cloneFragmentedUnicodeString;
    ut->pFuncs = &fragmentFuncs;

    ut->chunkContents = (UChar *)&ut->b;
    ut->pFuncs->access(ut, 0, TRUE);
    return ut;
}

// Regression test for Ticket 5560
//   Clone fails to update chunkContentPointer in the cloned copy.
//   This is only an issue for UText types that work in a local buffer,
//      (UTF-8 wrapper, for example)
//
//   The test:
//     1.  Create an inital UText
//     2.  Deep clone it.  Contents should match original.
//     3.  Reset original to something different.
//     4.  Check that clone contents did not change.  
//
void UTextTest::Ticket5560() {
    /* The following two strings are in UTF-8 even on EBCDIC platforms. */
    static const char s1[] = {0x41,0x42,0x43,0x44,0x45,0x46,0}; /* "ABCDEF" */
    static const char s2[] = {0x31,0x32,0x33,0x34,0x35,0x36,0}; /* "123456" */
	UErrorCode status = U_ZERO_ERROR;

	UText ut1 = UTEXT_INITIALIZER;
	UText ut2 = UTEXT_INITIALIZER;

	utext_openUTF8(&ut1, s1, -1, &status);
	UChar c = utext_next32(&ut1);
	TEST_ASSERT(c == 0x41);  // c == 'A'

	utext_clone(&ut2, &ut1, TRUE, FALSE, &status);
	TEST_SUCCESS(status);
    c = utext_next32(&ut2);
	TEST_ASSERT(c == 0x42);  // c == 'B'
    c = utext_next32(&ut1);
	TEST_ASSERT(c == 0x42);  // c == 'B'

	utext_openUTF8(&ut1, s2, -1, &status);
	c = utext_next32(&ut1);
	TEST_ASSERT(c == 0x31);  // c == '1'
    c = utext_next32(&ut2);
	TEST_ASSERT(c == 0x43);  // c == 'C'

    utext_close(&ut1);
    utext_close(&ut2);
}


// Test for Ticket 6847
//
void UTextTest::Ticket6847() {
    const int STRLEN = 90;
    UChar s[STRLEN+1];
    u_memset(s, 0x41, STRLEN);
    s[STRLEN] = 0;

    UErrorCode status = U_ZERO_ERROR;
    UText *ut = utext_openUChars(NULL, s, -1, &status);

    utext_setNativeIndex(ut, 0);
    int32_t count = 0;
    UChar32 c = 0;
    int32_t nativeIndex = UTEXT_GETNATIVEINDEX(ut);
    TEST_ASSERT(nativeIndex == 0);
    while ((c = utext_next32(ut)) != U_SENTINEL) {
        TEST_ASSERT(c == 0x41);
        TEST_ASSERT(count < STRLEN);
        if (count >= STRLEN) {
            break;
        }
        count++;
        nativeIndex = UTEXT_GETNATIVEINDEX(ut);
        TEST_ASSERT(nativeIndex == count);
    }
    TEST_ASSERT(count == STRLEN);
    nativeIndex = UTEXT_GETNATIVEINDEX(ut);
    TEST_ASSERT(nativeIndex == STRLEN);
    utext_close(ut);
}

