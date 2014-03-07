/*
 *******************************************************************************
 * Copyright (C) 1996-2009, International Business Machines Corporation and    *
 * others. All Rights Reserved.                                                *
 *******************************************************************************
 */

#include "unicode/utypes.h"

#if !UCONFIG_NO_FORMATTING

#include "itrbnfrt.h"

#include "unicode/fmtable.h"
#include <math.h> // fabs
#include <stdio.h>

// current macro not in icu1.8.1
#define TESTCASE(id,test)             \
    case id:                          \
        name = #test;                 \
        if (exec) {                   \
            logln(#test "---");       \
            logln();                  \
            test();                   \
        }                             \
        break

void RbnfRoundTripTest::runIndexedTest(int32_t index, UBool exec, const char* &name, char* /*par*/)
{
    if (exec) logln("TestSuite RuleBasedNumberFormatRT");
    switch (index) {
#if U_HAVE_RBNF
      TESTCASE(0, TestEnglishSpelloutRT);
      TESTCASE(1, TestDurationsRT);
      TESTCASE(2, TestSpanishSpelloutRT);
      TESTCASE(3, TestFrenchSpelloutRT);
      TESTCASE(4, TestSwissFrenchSpelloutRT);
      TESTCASE(5, TestItalianSpelloutRT);
      TESTCASE(6, TestGermanSpelloutRT);
      TESTCASE(7, TestSwedishSpelloutRT);
      TESTCASE(8, TestDutchSpelloutRT);
      TESTCASE(9, TestJapaneseSpelloutRT);
      TESTCASE(10, TestRussianSpelloutRT);
      TESTCASE(11, TestPortugueseSpelloutRT);
#else
      TESTCASE(0, TestRBNFDisabled);
#endif
    default:
      name = "";
      break;
    }
}

#if U_HAVE_RBNF

/**
 * Perform an exhaustive round-trip test on the English spellout rules
 */
void
RbnfRoundTripTest::TestEnglishSpelloutRT() 
{
  UErrorCode status = U_ZERO_ERROR;
  RuleBasedNumberFormat* formatter
    = new RuleBasedNumberFormat(URBNF_SPELLOUT, Locale::getUS(), status);

  if (U_FAILURE(status)) {
    errcheckln(status, "failed to construct formatter - %s", u_errorName(status));
  } else {
    doTest(formatter, -12345678, 12345678);
  }
  delete formatter;
}

/**
 * Perform an exhaustive round-trip test on the duration-formatting rules
 */
void
RbnfRoundTripTest::TestDurationsRT() 
{
  UErrorCode status = U_ZERO_ERROR;
  RuleBasedNumberFormat* formatter
    = new RuleBasedNumberFormat(URBNF_DURATION, Locale::getUS(), status);

  if (U_FAILURE(status)) {
    errcheckln(status, "failed to construct formatter - %s", u_errorName(status));
  } else {
    doTest(formatter, 0, 12345678);
  }
  delete formatter;
}

/**
 * Perform an exhaustive round-trip test on the Spanish spellout rules
 */
void
RbnfRoundTripTest::TestSpanishSpelloutRT() 
{
  UErrorCode status = U_ZERO_ERROR;
  RuleBasedNumberFormat* formatter
    = new RuleBasedNumberFormat(URBNF_SPELLOUT, Locale("es", "es"), status);
 
  if (U_FAILURE(status)) {
    errcheckln(status, "failed to construct formatter - %s", u_errorName(status));
  } else {
    doTest(formatter, -12345678, 12345678);
  }
  delete formatter;
}

/**
 * Perform an exhaustive round-trip test on the French spellout rules
 */
void
RbnfRoundTripTest::TestFrenchSpelloutRT() 
{
  UErrorCode status = U_ZERO_ERROR;
  RuleBasedNumberFormat* formatter
    = new RuleBasedNumberFormat(URBNF_SPELLOUT, Locale::getFrance(), status);

  if (U_FAILURE(status)) {
    errcheckln(status, "failed to construct formatter - %s", u_errorName(status));
  } else {
    doTest(formatter, -12345678, 12345678);
  }
  delete formatter;
}

/**
 * Perform an exhaustive round-trip test on the Swiss French spellout rules
 */
void
RbnfRoundTripTest::TestSwissFrenchSpelloutRT() 
{
  UErrorCode status = U_ZERO_ERROR;
  RuleBasedNumberFormat* formatter
    = new RuleBasedNumberFormat(URBNF_SPELLOUT, Locale("fr", "CH"), status);

  if (U_FAILURE(status)) {
    errcheckln(status, "failed to construct formatter - %s", u_errorName(status));
  } else {
    doTest(formatter, -12345678, 12345678);
  }
  delete formatter;
}

/**
 * Perform an exhaustive round-trip test on the Italian spellout rules
 */
void
RbnfRoundTripTest::TestItalianSpelloutRT() 
{
  UErrorCode status = U_ZERO_ERROR;
  RuleBasedNumberFormat* formatter
    = new RuleBasedNumberFormat(URBNF_SPELLOUT, Locale::getItalian(), status);

  if (U_FAILURE(status)) {
    errcheckln(status, "failed to construct formatter - %s", u_errorName(status));
  } else {
    doTest(formatter, -999999, 999999);
  }
  delete formatter;
}

/**
 * Perform an exhaustive round-trip test on the German spellout rules
 */
void
RbnfRoundTripTest::TestGermanSpelloutRT() 
{
  UErrorCode status = U_ZERO_ERROR;
  RuleBasedNumberFormat* formatter
    = new RuleBasedNumberFormat(URBNF_SPELLOUT, Locale::getGermany(), status);

  if (U_FAILURE(status)) {
    errcheckln(status, "failed to construct formatter - %s", u_errorName(status));
  } else {
    doTest(formatter, 0, 12345678);
  }
  delete formatter;
}

/**
 * Perform an exhaustive round-trip test on the Swedish spellout rules
 */
void
RbnfRoundTripTest::TestSwedishSpelloutRT() 
{
  UErrorCode status = U_ZERO_ERROR;
  RuleBasedNumberFormat* formatter
    = new RuleBasedNumberFormat(URBNF_SPELLOUT, Locale("sv", "SE"), status);

  if (U_FAILURE(status)) {
    errcheckln(status, "failed to construct formatter - %s", u_errorName(status));
  } else {
    doTest(formatter, 0, 12345678);
  }
  delete formatter;
}

/**
 * Perform an exhaustive round-trip test on the Dutch spellout rules
 */
void
RbnfRoundTripTest::TestDutchSpelloutRT() 
{
  UErrorCode status = U_ZERO_ERROR;
  RuleBasedNumberFormat* formatter
    = new RuleBasedNumberFormat(URBNF_SPELLOUT, Locale("nl", "NL"), status);

  if (U_FAILURE(status)) {
    errcheckln(status, "failed to construct formatter - %s", u_errorName(status));
  } else {
    doTest(formatter, -12345678, 12345678);
  }
  delete formatter;
}

/**
 * Perform an exhaustive round-trip test on the Japanese spellout rules
 */
void
RbnfRoundTripTest::TestJapaneseSpelloutRT() 
{
  UErrorCode status = U_ZERO_ERROR;
  RuleBasedNumberFormat* formatter
    = new RuleBasedNumberFormat(URBNF_SPELLOUT, Locale::getJapan(), status);

  if (U_FAILURE(status)) {
    errcheckln(status, "failed to construct formatter - %s", u_errorName(status));
  } else {
    doTest(formatter, 0, 12345678);
  }
  delete formatter;
}

/**
 * Perform an exhaustive round-trip test on the Russian spellout rules
 */
void
RbnfRoundTripTest::TestRussianSpelloutRT() 
{
  UErrorCode status = U_ZERO_ERROR;
  RuleBasedNumberFormat* formatter
    = new RuleBasedNumberFormat(URBNF_SPELLOUT, Locale("ru", "RU"), status);

  if (U_FAILURE(status)) {
    errcheckln(status, "failed to construct formatter - %s", u_errorName(status));
  } else {
    doTest(formatter, 0, 12345678);
  }
  delete formatter;
}

/**
 * Perform an exhaustive round-trip test on the Portuguese spellout rules
 */
void
RbnfRoundTripTest::TestPortugueseSpelloutRT() 
{
  UErrorCode status = U_ZERO_ERROR;
  RuleBasedNumberFormat* formatter
    = new RuleBasedNumberFormat(URBNF_SPELLOUT, Locale("pt", "BR"), status);

  if (U_FAILURE(status)) {
    errcheckln(status, "failed to construct formatter - %s", u_errorName(status));
  } else {
    doTest(formatter, -12345678, 12345678);
  }
  delete formatter;
}

void
RbnfRoundTripTest::doTest(const RuleBasedNumberFormat* formatter,  
                          double lowLimit,
                          double highLimit) 
{
  char buf[128];

  uint32_t count = 0;
  double increment = 1;
  for (double i = lowLimit; i <= highLimit; i += increment) {
    if (count % 1000 == 0) {
      sprintf(buf, "%.12g", i);
      logln(buf);
    }

    if (fabs(i) <  5000)
      increment = 1;
    else if (fabs(i) < 500000)
      increment = 2737;
    else
      increment = 267437;

    UnicodeString formatResult;
    formatter->format(i, formatResult);
    UErrorCode status = U_ZERO_ERROR;
    Formattable parseResult;
    formatter->parse(formatResult, parseResult, status);
    if (U_FAILURE(status)) {
      sprintf(buf, "Round-trip status failure: %.12g, status: %d", i, status);
      errln(buf);
      return;
    } else {
      double rt = (parseResult.getType() == Formattable::kDouble) ? 
        parseResult.getDouble() : 
        (double)parseResult.getLong();

      if (rt != i) {
        sprintf(buf, "Round-trip failed: %.12g -> %.12g", i, rt);
        errln(buf);
        return;
      }
    }

    ++count;
  }

  if (lowLimit < 0) {
    double d = 1.234;
    while (d < 1000) {
      UnicodeString formatResult;
      formatter->format(d, formatResult);
      UErrorCode status = U_ZERO_ERROR;
      Formattable parseResult;
      formatter->parse(formatResult, parseResult, status);
      if (U_FAILURE(status)) {
        sprintf(buf, "Round-trip status failure: %.12g, status: %d", d, status);
        errln(buf);
        return;
      } else {
        double rt = (parseResult.getType() == Formattable::kDouble) ? 
          parseResult.getDouble() : 
          (double)parseResult.getLong();

        if (rt != d) {
          UnicodeString msg;
          sprintf(buf, "Round-trip failed: %.12g -> ", d);
          msg.append(buf);
          msg.append(formatResult);
          sprintf(buf, " -> %.12g", rt);
          msg.append(buf);
          errln(msg);
          return;
        }
      }

      d *= 10;
    }
  }
}

/* U_HAVE_RBNF */
#else

void
RbnfRoundTripTest::TestRBNFDisabled() {
    errln("*** RBNF currently disabled on this platform ***\n");
}

/* U_HAVE_RBNF */
#endif

#endif /* #if !UCONFIG_NO_FORMATTING */
