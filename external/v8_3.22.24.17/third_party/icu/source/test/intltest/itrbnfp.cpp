/*
 *******************************************************************************
 * Copyright (C) 2004-2009, International Business Machines Corporation and         *
 * others. All Rights Reserved.                                                *
 *******************************************************************************
 */

#include "unicode/utypes.h"

#if !UCONFIG_NO_FORMATTING

#include "itrbnfp.h"

#include "unicode/umachine.h"

#include "unicode/tblcoll.h"
#include "unicode/coleitr.h"
#include "unicode/ures.h"
#include "unicode/ustring.h"
#include "unicode/decimfmt.h"

#include <string.h>

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

void IntlTestRBNFParse::runIndexedTest(int32_t index, UBool exec, const char* &name, char* /*par*/)
{
    if (exec) logln("TestSuite RuleBasedNumberFormatParse");
    switch (index) {
#if U_HAVE_RBNF
        TESTCASE(0, TestParse);
#else
        TESTCASE(0, TestRBNFParseDisabled);
#endif
    default:
        name = "";
        break;
    }
}

#if U_HAVE_RBNF

void 
IntlTestRBNFParse::TestParse() {
  // Try various rule parsing errors.  Shouldn't crash.

  logln("RBNF Parse test starting");

  // these rules make no sense but behave rationally
  const char* okrules[] = {
    "",
    "random text",
    "%foo:bar",
    "%foo: bar",
    "0:",
    "0::",
    ";",
    ";;",
    "%%foo:;",
    ":",
    "::",
    ":1",
    ":;",
    ":;:;",
    "-",
    "-1",
    "-:",
    ".",
    ".1",
    "[",
    "]",
    "[]",
    "[foo]",
    "[[]",
    "[]]",
    "[[]]",
    "[][]",
    "<",
    "<<",
    "<<<",
    "10:;9:;",
    ">",
    ">>",
    ">>>",
    "=",
    "==",
    "===",
    "=foo=",

    NULL,
  };

  // these rules would throw exceptions when formatting, if we could throw exceptions
  const char* exceptrules[] = {
    "10:", // formatting any value with a one's digit will fail
    "11: << x", // formating a multiple of 10 causes rollback rule to fail
    "%%foo: 0 foo; 10: =%%bar=; %%bar: 0: bar; 10: =%%foo=;",

    NULL,
  };

  // none of these rules should crash the formatter
  const char** allrules[] = {
    okrules,
    exceptrules,
    NULL,
  };

  for (int j = 0; allrules[j]; ++j) {
      const char** rules = allrules[j];
      for (int i = 0; rules[i]; ++i) {
          const char* rule = rules[i];
          logln("rule[%d] \"%s\"", i, rule);
          UErrorCode status = U_ZERO_ERROR;
          UParseError perr;
          RuleBasedNumberFormat* formatter = new RuleBasedNumberFormat(rule, Locale::getUS(), perr, status);
          
          if (U_SUCCESS(status)) {
              // format some values
              
              testfmt(formatter, 20, status);
              testfmt(formatter, 1.23, status);
              testfmt(formatter, -123, status);
              testfmt(formatter, .123, status);
              testfmt(formatter, 123, status);
              
          } else if (status == U_PARSE_ERROR) {
              logln("perror line: %x offset: %x context: %s|%s", perr.line, perr.offset, perr.preContext, perr.postContext);
          }

          delete formatter;
      }
  }
}

void
IntlTestRBNFParse::testfmt(RuleBasedNumberFormat* formatter, double val, UErrorCode& status) {
    UnicodeString us;
    formatter->format((const Formattable)val, us, status);
    if (U_SUCCESS(status)) {
        us.insert(0, (UChar)'"');
        us.append((UChar)'"');
        logln(us);
    } else {
        logln("error: could not format %g, returned status: %d", val, status);
    }
}

void
IntlTestRBNFParse::testfmt(RuleBasedNumberFormat* formatter, int val, UErrorCode& status) {
    UnicodeString us;
    formatter->format((const Formattable)(int32_t)val, us, status);
    if (U_SUCCESS(status)) {
        us.insert(0, (UChar)'"');
        us.append((UChar)'"');
        logln(us);
    } else {
        logln("error: could not format %d, returned status: %d", val, status);
    }
}


/* U_HAVE_RBNF */
#else

void
IntlTestRBNF::TestRBNFParseDisabled() {
    errln("*** RBNF currently disabled on this platform ***\n");
}

/* U_HAVE_RBNF */
#endif

#endif /* #if !UCONFIG_NO_FORMATTING */
