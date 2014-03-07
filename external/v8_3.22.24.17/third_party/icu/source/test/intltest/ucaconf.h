/********************************************************************
 * COPYRIGHT: 
 * Copyright (c) 2002-2003, International Business Machines Corporation and
 * others. All Rights Reserved.
 ********************************************************************/

/**
 * UCAConformanceTest performs conformance tests defined in the data
 * files. ICU ships with stub data files, as the whole test are too 
 * long. To do the whole test, download the test files.
 */

#ifndef _UCACONF_TST
#define _UCACONF_TST

#include "unicode/utypes.h"

#if !UCONFIG_NO_COLLATION

#include "unicode/tblcoll.h"
#include "tscoll.h"

#include <stdio.h>

class UCAConformanceTest: public IntlTestCollator {
public:
  UCAConformanceTest();
  virtual ~UCAConformanceTest();

  void runIndexedTest( int32_t index, UBool exec, const char* &name, char* /*par = NULL */);

  void TestTableNonIgnorable(/* par */);
  void TestTableShifted(/* par */);     
  void TestRulesNonIgnorable(/* par */);
  void TestRulesShifted(/* par */);     
private:
  void initRbUCA();
  void setCollNonIgnorable(UCollator *coll);
  void setCollShifted(UCollator *coll);
  void testConformance(UCollator *coll);
  void openTestFile(const char *type);

  UCollator *UCA;
  UCollator *rbUCA;
  FILE *testFile;
  UErrorCode status;
  char testDataPath[1024];
};

#endif /* #if !UCONFIG_NO_COLLATION */

#endif
