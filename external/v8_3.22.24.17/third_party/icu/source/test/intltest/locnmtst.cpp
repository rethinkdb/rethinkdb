/*********************************************************************
 * COPYRIGHT:
 * Copyright (c) 2010, International Business Machines Corporation and
 * others. All Rights Reserved.
 *********************************************************************/

#include "locnmtst.h"
#include "cstring.h"

/*
 Usage:
    test_assert(    Test (should be TRUE)  )

   Example:
       test_assert(i==3);

   the macro is ugly but makes the tests pretty.
*/

#define test_assert(test) \
    { \
        if(!(test)) \
            errln("FAIL: " #test " was not true. In " __FILE__ " on line %d", __LINE__ ); \
        else \
            logln("PASS: asserted " #test); \
    }

/*
 Usage:
    test_assert_print(    Test (should be TRUE),  printable  )

   Example:
       test_assert(i==3, toString(i));

   the macro is ugly but makes the tests pretty.
*/

#define test_assert_print(test,print) \
    { \
        if(!(test)) \
            errln("FAIL: " #test " was not true. " + UnicodeString(print) ); \
        else \
            logln("PASS: asserted " #test "-> " + UnicodeString(print)); \
    }

#define test_assert_equal(target,value) \
  { \
    if (UnicodeString(target)!=(value)) { \
      logln("unexpected value '" + (value) + "'"); \
      dataerrln("FAIL: " #target " == " #value " was not true. In " __FILE__ " on line %d", __LINE__); \
    } else { \
	logln("PASS: asserted " #target " == " #value); \
    } \
  }

#define test_dumpLocale(l) { logln(#l " = " + UnicodeString(l.getName(), "")); }

LocaleDisplayNamesTest::LocaleDisplayNamesTest() {
}

LocaleDisplayNamesTest::~LocaleDisplayNamesTest() {
}

void LocaleDisplayNamesTest::runIndexedTest(int32_t index, UBool exec, const char* &name, 
					    char* /*par*/) {
    switch (index) {
#if !UCONFIG_NO_FORMATTING
        TESTCASE(0, TestCreate);
        TESTCASE(1, TestCreateDialect);
	TESTCASE(2, TestWithKeywordsAndEverything);
	TESTCASE(3, TestUldnOpen);
	TESTCASE(4, TestUldnOpenDialect);
	TESTCASE(5, TestUldnWithKeywordsAndEverything);
	TESTCASE(6, TestUldnComponents);
	TESTCASE(7, TestRootEtc);
#endif
        default:
	  name = "";
	  break;
    }
}

#if !UCONFIG_NO_FORMATTING
void LocaleDisplayNamesTest::TestCreate() {
  UnicodeString temp;
  LocaleDisplayNames *ldn = LocaleDisplayNames::createInstance(Locale::getGermany());
  ldn->localeDisplayName("de_DE", temp);
  delete ldn;
  test_assert_equal("Deutsch (Deutschland)", temp);
}

void LocaleDisplayNamesTest::TestCreateDialect() {
  UnicodeString temp;
  LocaleDisplayNames *ldn = LocaleDisplayNames::createInstance(Locale::getUS(), ULDN_DIALECT_NAMES);
  ldn->localeDisplayName("en_GB", temp);
  delete ldn;
  test_assert_equal("British English", temp);
}

void LocaleDisplayNamesTest::TestWithKeywordsAndEverything() {
  UnicodeString temp;
  LocaleDisplayNames *ldn = LocaleDisplayNames::createInstance(Locale::getUS());
  const char *locname = "en_Hant_US_VALLEY@calendar=gregorian;collation=phonebook";
  const char *target = "English (Traditional Han, United States, VALLEY, "
    "Calendar=Gregorian Calendar, Collation=Phonebook Sort Order)";
  ldn->localeDisplayName(locname, temp);
  delete ldn;
  test_assert_equal(target, temp);
}

void LocaleDisplayNamesTest::TestUldnOpen() {
  UErrorCode status = U_ZERO_ERROR;
  const int32_t kMaxResultSize = 150;  // long enough
  UChar result[150];
  ULocaleDisplayNames *ldn = uldn_open(Locale::getGermany().getName(), ULDN_STANDARD_NAMES, &status);
  int32_t len = uldn_localeDisplayName(ldn, "de_DE", result, kMaxResultSize, &status);
  uldn_close(ldn);
  test_assert(U_SUCCESS(status));

  UnicodeString str(result, len, kMaxResultSize);
  test_assert_equal("Deutsch (Deutschland)", str);

  // make sure that NULL gives us the default locale as usual
  ldn = uldn_open(NULL, ULDN_STANDARD_NAMES, &status);
  const char *locale = uldn_getLocale(ldn);
  if(0 != uprv_strcmp(uloc_getDefault(), locale)) {
    errln("uldn_getLocale(uldn_open(NULL))=%s != default locale %s\n", locale, uloc_getDefault());
  }
  uldn_close(ldn);
  test_assert(U_SUCCESS(status));
}

void LocaleDisplayNamesTest::TestUldnOpenDialect() {
  UErrorCode status = U_ZERO_ERROR;
  const int32_t kMaxResultSize = 150;  // long enough
  UChar result[150];
  ULocaleDisplayNames *ldn = uldn_open(Locale::getUS().getName(), ULDN_DIALECT_NAMES, &status);
  int32_t len = uldn_localeDisplayName(ldn, "en_GB", result, kMaxResultSize, &status);
  uldn_close(ldn);
  test_assert(U_SUCCESS(status));

  UnicodeString str(result, len, kMaxResultSize);
  test_assert_equal("British English", str);
}

void LocaleDisplayNamesTest::TestUldnWithKeywordsAndEverything() {
  UErrorCode status = U_ZERO_ERROR;
  const int32_t kMaxResultSize = 150;  // long enough
  UChar result[150];
  const char *locname = "en_Hant_US_VALLEY@calendar=gregorian;collation=phonebook";
  const char *target = "English (Traditional Han, United States, VALLEY, "
    "Calendar=Gregorian Calendar, Collation=Phonebook Sort Order)";
  ULocaleDisplayNames *ldn = uldn_open(Locale::getUS().getName(), ULDN_STANDARD_NAMES, &status);
  int32_t len = uldn_localeDisplayName(ldn, locname, result, kMaxResultSize, &status);
  uldn_close(ldn);
  test_assert(U_SUCCESS(status));

  UnicodeString str(result, len, kMaxResultSize);
  test_assert_equal(target, str);
}

void LocaleDisplayNamesTest::TestUldnComponents() {
  UErrorCode status = U_ZERO_ERROR;
  const int32_t kMaxResultSize = 150;  // long enough
  UChar result[150];

  ULocaleDisplayNames *ldn = uldn_open(Locale::getGermany().getName(), ULDN_STANDARD_NAMES, &status);
  test_assert(U_SUCCESS(status));
  if (U_FAILURE(status)) {
    return;
  }

  // "en_Hant_US_PRE_EURO@calendar=gregorian";

  {
    int32_t len = uldn_languageDisplayName(ldn, "en", result, kMaxResultSize, &status);
    UnicodeString str(result, len, kMaxResultSize);
    test_assert_equal("Englisch", str);
  }


  {
    int32_t len = uldn_scriptDisplayName(ldn, "Hant", result, kMaxResultSize, &status);
    UnicodeString str(result, len, kMaxResultSize);
    test_assert_equal("Traditionelle Chinesische Schrift", str);
  }

  {
    int32_t len = uldn_scriptCodeDisplayName(ldn, USCRIPT_TRADITIONAL_HAN, result, kMaxResultSize, 
					     &status);
    UnicodeString str(result, len, kMaxResultSize);
    test_assert_equal("Traditionelle Chinesische Schrift", str);
  }

  {
    int32_t len = uldn_regionDisplayName(ldn, "US", result, kMaxResultSize, &status);
    UnicodeString str(result, len, kMaxResultSize);
    test_assert_equal("Vereinigte Staaten", str);
  }

  {
    int32_t len = uldn_variantDisplayName(ldn, "PRE_EURO", result, kMaxResultSize, &status);
    UnicodeString str(result, len, kMaxResultSize);
    test_assert_equal("PRE_EURO", str);
  }

  {
    int32_t len = uldn_keyDisplayName(ldn, "calendar", result, kMaxResultSize, &status);
    UnicodeString str(result, len, kMaxResultSize);
    test_assert_equal("Kalender", str);
  }

  {
    int32_t len = uldn_keyValueDisplayName(ldn, "calendar", "gregorian", result, 
					   kMaxResultSize, &status);
    UnicodeString str(result, len, kMaxResultSize);
    test_assert_equal("Gregorianischer Kalender", str);
  }

  uldn_close(ldn);
}

void LocaleDisplayNamesTest::TestRootEtc() {
  UnicodeString temp;
  LocaleDisplayNames *ldn = LocaleDisplayNames::createInstance(Locale::getUS());
  const char *locname = "@collation=phonebook";
  const char *target = "Root (Collation=Phonebook Sort Order)";
  ldn->localeDisplayName(locname, temp);
  test_assert_equal(target, temp);

  ldn->languageDisplayName("root", temp);
  test_assert_equal("root", temp);

  ldn->languageDisplayName("en_GB", temp);
  test_assert_equal("en_GB", temp);

  delete ldn;
}

#endif   /*  UCONFIG_NO_FORMATTING */

