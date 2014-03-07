/*
**********************************************************************
*   Copyright (C) 1999-2009, International Business Machines
*   Corporation and others.  All Rights Reserved.
**********************************************************************
*   Date        Name        Description
*   11/10/99    aliu        Creation.
**********************************************************************
*/
#ifndef TRANSTST_H
#define TRANSTST_H

#include "unicode/utypes.h"

#if !UCONFIG_NO_TRANSLITERATION

#include "unicode/translit.h"
#include "intltest.h"

/**
 * @test
 * @summary General test of Transliterator
 */
class TransliteratorTest : public IntlTest {

public:
    TransliteratorTest();
    virtual ~TransliteratorTest();

private:
    void runIndexedTest(int32_t index, UBool exec, const char* &name,
                        char* par=NULL);

    void TestInstantiation(void);
    
    void TestSimpleRules(void);

    void TestInlineSet(void);

    void TestAnchors(void);

    void TestPatternQuoting(void);

    /**
     * Create some inverses and confirm that they work.  We have to be
     * careful how we do this, since the inverses will not be true
     * inverses -- we can't throw any random string at the composition
     * of the transliterators and expect the identity function.  F x
     * F' != I.  However, if we are careful about the input, we will
     * get the expected results.
     */
    void TestRuleBasedInverse(void);

    /**
     * Basic test of keyboard.
     */
    void TestKeyboard(void);

    /**
     * Basic test of keyboard with cursor.
     */
    void TestKeyboard2(void);

    /**
     * Test keyboard transliteration with back-replacement.
     */
    void TestKeyboard3(void);
    
    void keyboardAux(const Transliterator& t,
                     const char* DATA[], int32_t DATA_length);
    
    void TestArabic(void);

    /**
     * Compose the Kana transliterator forward and reverse and try
     * some strings that should come out unchanged.
     */
    void TestCompoundKana(void);

    /**
     * Compose the hex transliterators forward and reverse.
     */
    void TestCompoundHex(void);

    /**
     * Do some basic tests of filtering.
     */
    void TestFiltering(void);

    /**
     * Regression test for bugs found in Greek transliteration.
     */
    void TestJ277(void);

    /**
     * Prefix, suffix support in hex transliterators.
     */
    void TestJ243(void);

    /**
     * Parsers need better syntax error messages.
     */
    void TestJ329(void);

    /**
     * Test segments and segment references.
     */
    void TestSegments(void);
    
    /**
     * Test cursor positioning outside of the key
     */
    void TestCursorOffset(void);
    
    /**
     * Test zero length and > 1 char length variable values.  Test
     * use of variable refs in UnicodeSets.
     */
    void TestArbitraryVariableValues(void);

    /**
     * Confirm that the contextStart, contextLimit, start, and limit
     * behave correctly. J474.
     */
    void TestPositionHandling(void);

    /**
     * Test the Hiragana-Katakana transliterator.
     */
    void TestHiraganaKatakana(void);

    /**
     * Test cloning / copy constructor of RBT.
     */
    void TestCopyJ476(void);

    /**
     * Test inter-Indic transliterators.  These are composed.
     * ICU4C Jitterbug 483.
     */
    void TestInterIndic(void);

    /**
     * Test filter syntax in IDs. (J918)
     */
    void TestFilterIDs(void);

    /**
     * Test the case mapping transliterators.
     */
    void TestCaseMap(void);

    /**
     * Test the name mapping transliterators.
     */
    void TestNameMap(void);

    /**
     * Test liberalized ID syntax.  1006c
     */
    void TestLiberalizedID(void);
    /**
     * Test Jitterbug 912
     */
    void TestCreateInstance(void);

    void TestNormalizationTransliterator(void);

    void TestCompoundRBT(void);

    void TestCompoundFilter(void);

    void TestRemove(void);

    void TestToRules(void);

    void TestContext(void);

    void TestSupplemental(void);

    void TestQuantifier(void);

    /**
     * Test Source-Target/Variant.
     */
    void TestSTV(void);

    void TestCompoundInverse(void);

    void TestNFDChainRBT(void);

    /**
     * Inverse of "Null" should be "Null". (J21)
     */
    void TestNullInverse(void);
    
    /**
     * Check ID of inverse of alias. (J22)
     */
    void TestAliasInverseID(void);
    
    /**
     * Test IDs of inverses of compound transliterators. (J20)
     */
    void TestCompoundInverseID(void);
    
    /**
     * Test undefined variable.
     */
    void TestUndefinedVariable(void);
    
    /**
     * Test empty context.
     */
    void TestEmptyContext(void);

    /**
     * Test compound filter ID syntax
     */
    void TestCompoundFilterID(void);

    /**
     * Test new property set syntax
     */
    void TestPropertySet(void);

    /**
     * Test various failure points of the new 2.0 engine.
     */
    void TestNewEngine(void);

    /**
     * Test quantified segment behavior.  We want:
     * ([abc])+ > x $1 x; applied to "cba" produces "xax"
     */
    void TestQuantifiedSegment(void);

    /* Devanagari-Latin rules Test */
    void TestDevanagariLatinRT(void);

    /* Telugu-Latin rules Test */
    void TestTeluguLatinRT(void);
    
    /* Gujarati-Latin rules Test */
    void TestGujaratiLatinRT(void);
    
    /* Sanskrit-Latin rules Test */
    void TestSanskritLatinRT(void);
    
    /* Test Compound Indic-Latin transliterators*/
    void TestCompoundLatinRT(void);

    /* Test bindi and tippi for Gurmukhi */
    void TestGurmukhiDevanagari(void);
    /**
     * Test instantiation from a locale.
     */
    void TestLocaleInstantiation(void);        
    
    /**
     * Test title case handling of accent (should ignore accents)
     */
    void TestTitleAccents(void);

    /**
     * Basic test of a locale resource based rule.
     */
    void TestLocaleResource(void);

    /**
     * Make sure parse errors reference the right line.
     */
    void TestParseError(void);

    /**
     * Make sure sets on output are disallowed.
     */
    void TestOutputSet(void);

    /**
     * Test the use variable range pragma, making sure that use of
     * variable range characters is detected and flagged as an error.
     */
    void TestVariableRange(void);

    /**
     * Test invalid post context error handling
     */
    void TestInvalidPostContext(void);

    /**
     * Test ID form variants
     */
    void TestIDForms(void);

    /**
     * Mark's toRules test.
     */
    void TestToRulesMark(void);

    /**
     * Test Escape and Unescape transliterators.
     */
    void TestEscape(void);

    void TestAnchorMasking(void);

    /**
     * Make sure display names of variants look reasonable.
     */
    void TestDisplayName(void);
    
    /** 
     * Check to see if case mapping works correctly.
     */
    void TestSpecialCases(void);
    /**
     * Check to see that incremental gets at least part way through a reasonable string.
     */
    void TestIncrementalProgress(void);

    /** 
     * Check that casing handles surrogates.
     */
    void TestSurrogateCasing (void);

    void TestFunction(void);

    void TestInvalidBackRef(void);

    void TestMulticharStringSet(void);

    void TestUserFunction(void);

    void TestAnyX(void);

    void TestAny(void);

    void TestSourceTargetSet(void);

    void TestRuleWhitespace(void);

    void TestAllCodepoints(void);

    void TestBoilerplate(void);

    void TestAlternateSyntax(void);

    void TestRuleStripping(void);

    void TestHalfwidthFullwidth(void);

    void TestThai(void);

    /**
     * Tests the multiple-pass syntax
     */
    void TestBeginEnd(void);

    /**
     * Tests that toRules() works right with the multiple-pass syntax
     */
    void TestBeginEndToRules(void);

    /**
     * Tests the registerAlias() function
     */
    void TestRegisterAlias(void);

    //======================================================================
    // Support methods
    //======================================================================
 protected:
    void expectT(const UnicodeString& id,
                 const UnicodeString& source,
                 const UnicodeString& expectedResult);

    void expect(const UnicodeString& rules,
                const UnicodeString& source,
                const UnicodeString& expectedResult,
                UTransPosition *pos=0);

    void expect(const UnicodeString& id,
                const UnicodeString& rules,
                const UnicodeString& source,
                const UnicodeString& expectedResult,
                UTransPosition *pos=0);

    void expect(const Transliterator& t,
                const UnicodeString& source,
                const UnicodeString& expectedResult,
                const Transliterator& reverseTransliterator);
    
    void expect(const Transliterator& t,
                const UnicodeString& source,
                const UnicodeString& expectedResult,
                UTransPosition *pos=0);
    
    void expectAux(const UnicodeString& tag,
                   const UnicodeString& source,
                   const UnicodeString& result,
                   const UnicodeString& expectedResult);
    
    virtual void expectAux(const UnicodeString& tag,
                   const UnicodeString& summary, UBool pass,
                   const UnicodeString& expectedResult);

    static UnicodeString& formatInput(UnicodeString &appendTo,
                                      const UnicodeString& input,
                                      const UTransPosition& pos);

    void checkRules(const UnicodeString& label, Transliterator& t2,
                    const UnicodeString& testRulesForward);
    void CheckIncrementalAux(const Transliterator* t, 
                             const UnicodeString& input);

    void reportParseError(const UnicodeString& message, const UParseError& parseError, const UErrorCode& status);


    const UnicodeString DESERET_DEE;
    const UnicodeString DESERET_dee;

};

#endif /* #if !UCONFIG_NO_TRANSLITERATION */

#endif
