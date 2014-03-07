/*
 *******************************************************************************
 * Copyright (C) 1996-2007, International Business Machines Corporation and    *
 * others. All Rights Reserved.                                                *
 *******************************************************************************
 */

#ifndef ITRBNF_H
#define ITRBNF_H

#include "unicode/utypes.h"

#if !UCONFIG_NO_FORMATTING

#include "intltest.h"
#include "unicode/rbnf.h"


class IntlTestRBNF : public IntlTest {
 public:

  // IntlTest override
  virtual void runIndexedTest(int32_t index, UBool exec, const char* &name, char* par);

#if U_HAVE_RBNF
  /** 
   * Perform an API test
   */
  virtual void TestAPI();

  /**
   * Perform a simple spot check on the FractionalRuleSet logic
   */
  virtual void TestFractionalRuleSet();

#if 0
  /**
   * Perform API tests on llong
   */
  virtual void TestLLong();
  virtual void TestLLongConstructors();
  virtual void TestLLongSimpleOperators();
#endif

  /**
   * Perform a simple spot check on the English spellout rules
   */
  virtual void TestEnglishSpellout();

  /**
   * Perform a simple spot check on the English ordinal-abbreviation rules
   */
  virtual void TestOrdinalAbbreviations();

  /**
   * Perform a simple spot check on the duration-formatting rules
   */
  virtual void TestDurations();

  /**
   * Perform a simple spot check on the Spanish spellout rules
   */
  virtual void TestSpanishSpellout();

  /**
   * Perform a simple spot check on the French spellout rules
   */
  virtual void TestFrenchSpellout();

  /**
   * Perform a simple spot check on the Swiss French spellout rules
   */
  virtual void TestSwissFrenchSpellout();

  /**
   * Check that Belgian French matches Swiss French spellout rules
   */
  virtual void TestBelgianFrenchSpellout();

  /**
   * Perform a simple spot check on the Italian spellout rules
   */
  virtual void TestItalianSpellout();

  /**
   * Perform a simple spot check on the Portuguese spellout rules
   */
  virtual void TestPortugueseSpellout();

  /**
   * Perform a simple spot check on the German spellout rules
   */
  virtual void TestGermanSpellout();

  /**
   * Perform a simple spot check on the Thai spellout rules
   */
  virtual void TestThaiSpellout();

  /**
   * Perform a simple spot check on the Swedish spellout rules
   */
  virtual void TestSwedishSpellout();

  /**
   * Perform a simple spot check on small values
   */
  virtual void TestSmallValues();

  /**
   * Test localizations using string data.
   */
  virtual void TestLocalizations();

  /**
   * Test that all locales construct ok.
   */
  virtual void TestAllLocales();

  /**
   * Test that hebrew fractions format without trailing '<'
   */
  virtual void TestHebrewFraction();

  /**
   * Regression test, don't truncate
   * when doing multiplier substitution to a number format rule.
   */
  virtual void TestMultiplierSubstitution();

 protected:
  virtual void doTest(RuleBasedNumberFormat* formatter, const char* const testData[][2], UBool testParsing);
  virtual void doLenientParseTest(RuleBasedNumberFormat* formatter, const char* testData[][2]);

/* U_HAVE_RBNF */
#else

  virtual void TestRBNFDisabled();

/* U_HAVE_RBNF */
#endif
};

#endif /* #if !UCONFIG_NO_FORMATTING */

// endif ITRBNF_H
#endif
