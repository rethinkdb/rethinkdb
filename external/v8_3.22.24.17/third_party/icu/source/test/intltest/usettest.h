
/********************************************************************
 * COPYRIGHT: 
 * Copyright (c) 1997-2007, International Business Machines Corporation and
 * others. All Rights Reserved.
 ********************************************************************
**********************************************************************
*   Date        Name        Description
*   10/20/99    alan        Creation.
*   03/22/2000  Madhu       Added additional tests
**********************************************************************
*/

#ifndef _TESTUNISET
#define _TESTUNISET

#include "unicode/unistr.h"
#include "unicode/uniset.h"
#include "unicode/ucnv_err.h"
#include "intltest.h"

class UnicodeSetWithStrings;

/**
 * UnicodeSet test
 */
class UnicodeSetTest: public IntlTest {
public:
    UnicodeSetTest();
    ~UnicodeSetTest();

private:
    void runIndexedTest(int32_t index, UBool exec, const char* &name, char* par=NULL);

    void Testj2268();

    /**
     * Test that toPattern() round trips with syntax characters and
     * whitespace.
     */
    void TestToPattern();
    
    void TestPatterns(void);
    void TestCategories(void);
    void TestAddRemove(void);
    void TestCloneEqualHash(void);

    /**
     * Make sure minimal representation is maintained.
     */
    void TestMinimalRep(void);

    void TestAPI(void);

    void TestIteration(void);

    void TestStrings(void);

    void TestScriptSet(void);

    /**
     * Test the [:Latin:] syntax.
     */
    void TestPropertySet(void);

    void TestClone(void);

    void TestIndexOf(void);

    void TestExhaustive(void);

    void TestCloseOver(void);

    void TestEscapePattern(void);

    void TestInvalidCodePoint(void);

    void TestSymbolTable(void);

    void TestSurrogate();

    void TestPosixClasses();

    void TestFreezable();

    void TestSpan();

    void TestStringSpan();

private:

    UBool toPatternAux(UChar32 start, UChar32 end);
    
    UBool checkPat(const UnicodeString& source,
                   const UnicodeSet& testSet);

    UBool checkPat(const UnicodeString& source, const UnicodeSet& testSet, const UnicodeString& pat);

    void _testComplement(int32_t a, UnicodeSet&, UnicodeSet&);

    void _testAdd(int32_t a, int32_t b, UnicodeSet&, UnicodeSet&, UnicodeSet&);

    void _testRetain(int32_t a, int32_t b, UnicodeSet&, UnicodeSet&, UnicodeSet&);

    void _testRemove(int32_t a, int32_t b, UnicodeSet&, UnicodeSet&, UnicodeSet&);

    void _testXor(int32_t a, int32_t b, UnicodeSet&, UnicodeSet&, UnicodeSet&);

    /**
     * Check that ranges are monotonically increasing and non-
     * overlapping.
     */
    void checkCanonicalRep(const UnicodeSet& set, const UnicodeString& msg);

    /**
     * Convert a bitmask to a UnicodeSet.
     */
    static UnicodeSet& bitsToSet(int32_t a, UnicodeSet&);

    /**
     * Convert a UnicodeSet to a bitmask.  Only the characters
     * U+0000 to U+0020 are represented in the bitmask.
     */
    static int32_t setToBits(const UnicodeSet& x);

    /**
     * Return the representation of an inversion list based UnicodeSet
     * as a pairs list.  Ranges are listed in ascending Unicode order.
     * For example, the set [a-zA-M3] is represented as "33AMaz".
     */
    static UnicodeString getPairs(const UnicodeSet& set);

    /**
     * Basic consistency check for a few items.
     * That the iterator works, and that we can create a pattern and
     * get the same thing back
     */
    void checkRoundTrip(const UnicodeSet& s);
    
    void copyWithIterator(UnicodeSet& t, const UnicodeSet& s, UBool withRange);
    
    UBool checkEqual(const UnicodeSet& s, const UnicodeSet& t, const char* message);

    void expectContainment(const UnicodeString& pat,
                           const UnicodeString& charsIn,
                           const UnicodeString& charsOut);
    void expectContainment(const UnicodeSet& set,
                           const UnicodeString& charsIn,
                           const UnicodeString& charsOut);
    void expectContainment(const UnicodeSet& set,
                           const UnicodeString& setName,
                           const UnicodeString& charsIn,
                           const UnicodeString& charsOut);
    void expectPattern(UnicodeSet& set,
                       const UnicodeString& pattern,
                       const UnicodeString& expectedPairs);
    void expectPairs(const UnicodeSet& set,
                     const UnicodeString& expectedPairs);
    void expectToPattern(const UnicodeSet& set,
                         const UnicodeString& expPat,
                         const char** expStrings);
    void expectRange(const UnicodeString& label,
                     const UnicodeSet& set,
                     UChar32 start, UChar32 end);
    void doAssert(UBool, const char*);

    void testSpan(const UnicodeSetWithStrings *sets[4], const void *s, int32_t length, UBool isUTF16,
                  uint32_t whichSpans,
                  int32_t expectLimits[], int32_t &expectCount,
                  const char *testName, int32_t index);
    void testSpan(const UnicodeSetWithStrings *sets[4], const void *s, int32_t length, UBool isUTF16,
                  uint32_t whichSpans,
                  const char *testName, int32_t index);
    void testSpanBothUTFs(const UnicodeSetWithStrings *sets[4],
                          const UChar *s16, int32_t length16,
                          uint32_t whichSpans,
                          const char *testName, int32_t index);
    void testSpanContents(const UnicodeSetWithStrings *sets[4], uint32_t whichSpans, const char *testName);
    void testSpanUTF16String(const UnicodeSetWithStrings *sets[4], uint32_t whichSpans, const char *testName);
    void testSpanUTF8String(const UnicodeSetWithStrings *sets[4], uint32_t whichSpans, const char *testName);

    UConverter *openUTF8Converter();

    UConverter *utf8Cnv;

public:
    static UnicodeString escape(const UnicodeString& s);
};

#endif
