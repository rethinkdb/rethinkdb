/********************************************************************
 * COPYRIGHT: 
 * Copyright (c) 1997-2009, International Business Machines Corporation and
 * others. All Rights Reserved.
 ********************************************************************/

#include "unicode/utypes.h"

#if !UCONFIG_NO_COLLATION

#include "unicode/coll.h"
#include "unicode/tblcoll.h"
#include "unicode/unistr.h"
#include "unicode/sortkey.h"
#include "allcoll.h"
#include "sfwdchit.h"
#include "../cintltst/calldata.h"


CollationDummyTest::CollationDummyTest()
: myCollation(0)
{
    /*UErrorCode status = U_ZERO_ERROR;
    UnicodeString rules(TRUE, DEFAULTRULEARRAY, sizeof(DEFAULTRULEARRAY)/sizeof(DEFAULTRULEARRAY[0]));
    UnicodeString newRules("& C < ch, cH, Ch, CH & Five, 5 & Four, 4 & one, 1 & Ampersand; '&' & Two, 2 ");
    rules += newRules;
    myCollation = new RuleBasedCollator(rules, status);
    */

    UErrorCode status = U_ZERO_ERROR;
    UnicodeString ruleset("& C < ch, cH, Ch, CH & Five, 5 & Four, 4 & one, 1 & Ampersand; '&' & Two, 2 ");
    if (myCollation != NULL)
    {
      delete myCollation;
    }
    myCollation = new RuleBasedCollator(ruleset, status);
    if(U_FAILURE(status)){
        errcheckln(status, "ERROR: in creation of rule based collator from ruleset - %s", u_errorName(status));
        delete myCollation;
        myCollation = 0;
    }
}

CollationDummyTest::~CollationDummyTest()
{
    delete myCollation;
}

const Collator::EComparisonResult CollationDummyTest::results[] = {
    Collator::LESS,
    Collator::LESS, /*Collator::GREATER,*/
    Collator::LESS,
    Collator::LESS,
    Collator::LESS,
    Collator::LESS,
    Collator::LESS,
    Collator::GREATER,
    Collator::GREATER,
    Collator::LESS,                                     /*  10 */
    Collator::GREATER,
    Collator::LESS,
    Collator::GREATER,
    Collator::GREATER,
    Collator::LESS,
    Collator::LESS,
    Collator::LESS,
    /*  test primary > 17 */
    Collator::EQUAL,
    Collator::EQUAL,
    Collator::EQUAL,                                    /*  20 */
    Collator::LESS,
    Collator::LESS,
    Collator::EQUAL,
    Collator::EQUAL,
    Collator::EQUAL,
    Collator::LESS,
    /*  test secondary > 26 */
    Collator::EQUAL,
    Collator::EQUAL,
    Collator::EQUAL,
    Collator::EQUAL,
    Collator::EQUAL,                                    /*  30 */
    Collator::EQUAL,
    Collator::LESS,
    Collator::EQUAL,                                     /*  34 */
    Collator::EQUAL,
    Collator::EQUAL,
    Collator::LESS 
};


void CollationDummyTest::TestTertiary(/* char* par */)
{
    int32_t i = 0;
    myCollation->setStrength(Collator::TERTIARY);
    for (i = 0; i < 17 ; i++)
    {
        doTest(myCollation, testSourceCases[i], testTargetCases[i], results[i]);
    }
}
void CollationDummyTest::TestPrimary(/* char* par */)
{
    /* problem in strcollinc for unfinshed contractions */
    UErrorCode status = U_ZERO_ERROR;

    myCollation->setAttribute(UCOL_NORMALIZATION_MODE, UCOL_ON, status);
    myCollation->setStrength(Collator::PRIMARY);

    if (U_FAILURE(status))
    {
      errln("Failure in setting attribute for normalization mode\n");
    }
    
    for (int i = 17; i < 26 ; i++)
    {
        doTest(myCollation, testSourceCases[i], testTargetCases[i], results[i]);
    }
}

void CollationDummyTest::TestSecondary(/* char* par */)
{
    int32_t i;
    myCollation->setStrength(Collator::SECONDARY);
    for (i = 26; i < 34; i++)
    {
        doTest(myCollation, testSourceCases[i], testTargetCases[i], results[i]);
    }
}

void CollationDummyTest::TestExtra(/* char* par */)
{
    int32_t i, j;
    myCollation->setStrength(Collator::TERTIARY);
    for (i = 0; i < COUNT_TEST_CASES-1; i++)
    {
        for (j = i + 1; j < COUNT_TEST_CASES; j += 1)
        {
            doTest(myCollation, testCases[i], testCases[j], Collator::LESS);
        }
    }
}

void CollationDummyTest::TestIdentical()
{
    int32_t i;
    myCollation->setStrength(Collator::IDENTICAL);
    for (i= 34; i<37; i++)
    {
        doTest(myCollation, testSourceCases[i], testTargetCases[i], results[i]);
    }
}

void CollationDummyTest::TestJB581(void)
{
    UErrorCode status = U_ZERO_ERROR;

    UnicodeString source("THISISATEST.");
    UnicodeString target("Thisisatest.");

    Collator *coll = Collator::createInstance("en_US", status);
    if (U_FAILURE(status)){
        errln("ERROR: Failed to create the collator for : en_US\n");
        return;
    }

    Collator::EComparisonResult result = coll->compare(source, target);
    /* result is 1, secondary differences only for ignorable space characters*/
    if (result != 1)
    {
        errln("Comparing two strings with only secondary differences in C failed.\n");
    }
    /* To compare them with just primary differences */
    coll->setStrength(Collator::PRIMARY);
    result = coll->compare(source, target);
    /* result is 0 */
    if (result != 0)
    {
        errln("Comparing two strings with no differences in C failed.\n");
    }
    /* Now, do the same comparison with keys */
    CollationKey sourceKeyOut,
      targetKeyOut;
    coll->getCollationKey(source, sourceKeyOut, status);
    coll->getCollationKey(target, targetKeyOut, status);
    result = sourceKeyOut.compareTo(targetKeyOut);
    if (result != 0)
    {
        errln("Comparing two strings with sort keys in C failed.\n");
    }
    delete coll;
}

void CollationDummyTest::runIndexedTest( int32_t index, UBool exec, const char* &name, char* /*par*/ )
{
    if (exec) logln("TestSuite CollationDummyTest: ");
    if(myCollation) {
      switch (index) {
          case 0: name = "TestPrimary";   if (exec)   TestPrimary(/* par */); break;
          case 1: name = "TestSecondary"; if (exec)   TestSecondary(/* par */); break;
          case 2: name = "TestTertiary";  if (exec)   TestTertiary(/* par */); break;
          case 3: name = "TestExtra";     if (exec)   TestExtra(/* par */); break;
          case 4: name = "TestIdentical"; if (exec)   TestIdentical(/* par */); break;
          case 5: name = "TestJB581";     if (exec)   TestJB581(/* par */); break;
          default: name = ""; break;
      }
    } else {
      dataerrln("Collator couldn't be instantiated!");
      name = "";
    }
}

#endif /* #if !UCONFIG_NO_COLLATION */
