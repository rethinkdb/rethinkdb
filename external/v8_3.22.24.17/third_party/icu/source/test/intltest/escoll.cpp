
/********************************************************************
 * COPYRIGHT: 
 * Copyright (c) 1997-2009, International Business Machines Corporation and
 * others. All Rights Reserved.
 ********************************************************************/

#include "unicode/utypes.h"

#if !UCONFIG_NO_COLLATION

#ifndef _COLL
#include "unicode/coll.h"
#endif

#ifndef _TBLCOLL
#include "unicode/tblcoll.h"
#endif

#ifndef _UNISTR
#include "unicode/unistr.h"
#endif

#ifndef _SORTKEY
#include "unicode/sortkey.h"
#endif

#ifndef _ESCOLL
#include "escoll.h"
#endif

#include "sfwdchit.h"

CollationSpanishTest::CollationSpanishTest()
: myCollation(0)
{
    UErrorCode status = U_ZERO_ERROR;
    myCollation = Collator::createInstance(Locale("es", "ES", ""),status);
}

CollationSpanishTest::~CollationSpanishTest()
{
    delete myCollation;
}

const UChar CollationSpanishTest::testSourceCases[][CollationSpanishTest::MAX_TOKEN_LEN] = {
    {0x61, 0x6c, 0x69, 0x61, 0x73, 0},
    {0x45, 0x6c, 0x6c, 0x69, 0x6f, 0x74, 0},
    {0x48, 0x65, 0x6c, 0x6c, 0x6f, 0},
    {0x61, 0x63, 0x48, 0x63, 0},
    {0x61, 0x63, 0x63, 0},
    {0x61, 0x6c, 0x69, 0x61, 0x73, 0},
    {0x61, 0x63, 0x48, 0x63, 0},
    {0x61, 0x63, 0x63, 0},
    {0x48, 0x65, 0x6c, 0x6c, 0x6f, 0},
};

const UChar CollationSpanishTest::testTargetCases[][CollationSpanishTest::MAX_TOKEN_LEN] = {
    {0x61, 0x6c, 0x6c, 0x69, 0x61, 0x73, 0},
    {0x45, 0x6d, 0x69, 0x6f, 0x74, 0},
    {0x68, 0x65, 0x6c, 0x6c, 0x4f, 0},
    {0x61, 0x43, 0x48, 0x63, 0},
    {0x61, 0x43, 0x48, 0x63, 0},
    {0x61, 0x6c, 0x6c, 0x69, 0x61, 0x73, 0},
    {0x61, 0x43, 0x48, 0x63, 0},
    {0x61, 0x43, 0x48, 0x63, 0},
    {0x68, 0x65, 0x6c, 0x6c, 0x4f, 0},
};

const Collator::EComparisonResult CollationSpanishTest::results[] = {
    Collator::LESS,
    Collator::LESS,
    Collator::GREATER,
    Collator::LESS,
    Collator::LESS,
    // test primary > 5
    Collator::LESS,
    Collator::EQUAL,
    Collator::LESS,
    Collator::EQUAL
};

void CollationSpanishTest::TestTertiary(/* char* par */)
{
    int32_t i = 0;
    myCollation->setStrength(Collator::TERTIARY);
    for (i = 0; i < 5 ; i++) {
        doTest(myCollation, testSourceCases[i], testTargetCases[i], results[i]);
    }
}
void CollationSpanishTest::TestPrimary(/* char* par */)
{
    int32_t i;
    myCollation->setStrength(Collator::PRIMARY);
    for (i = 5; i < 9; i++) {
        doTest(myCollation, testSourceCases[i], testTargetCases[i], results[i]);
    }
}

void CollationSpanishTest::runIndexedTest( int32_t index, UBool exec, const char* &name, char* /*par */)
{
    if (exec) logln("TestSuite CollationSpanishTest: ");

    if((!myCollation) && exec) {
      dataerrln(__FILE__ " cannot test - failed to create collator.");
      name = "some test";
      return;
    }
    switch (index) {
        case 0: name = "TestPrimary";   if (exec)   TestPrimary(/* par */); break;
        case 1: name = "TestTertiary";  if (exec)   TestTertiary(/* par */); break;
        default: name = ""; break;
    }
}

#endif /* #if !UCONFIG_NO_COLLATION */
