/********************************************************************
 * COPYRIGHT: 
 * Copyright (c) 1997-2009, International Business Machines Corporation and
 * others. All Rights Reserved.
 ********************************************************************/

#include "unicode/utypes.h"

#if !UCONFIG_NO_COLLATION

#include "decoll.h"

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

#ifndef _DECOLL
#include "decoll.h"
#endif

#include "sfwdchit.h"

CollationGermanTest::CollationGermanTest()
: myCollation(0)
{
    UErrorCode status = U_ZERO_ERROR;
    myCollation = Collator::createInstance(Locale::getGermany(), status);
    if(!myCollation || U_FAILURE(status)) {
        errcheckln(status, __FILE__ "failed to create! err " + UnicodeString(u_errorName(status)));
        /* if it wasn't already: */
        delete myCollation;
        myCollation = NULL;
    }
}

CollationGermanTest::~CollationGermanTest()
{
    delete myCollation;
}

const UChar CollationGermanTest::testSourceCases[][CollationGermanTest::MAX_TOKEN_LEN] =
{
    {0x47, 0x72, 0x00F6, 0x00DF, 0x65, 0},
    {0x61, 0x62, 0x63, 0},
    {0x54, 0x00F6, 0x6e, 0x65, 0},
    {0x54, 0x00F6, 0x6e, 0x65, 0},
    {0x54, 0x00F6, 0x6e, 0x65, 0},
    {0x61, 0x0308, 0x62, 0x63, 0},
    {0x00E4, 0x62, 0x63, 0},
    {0x00E4, 0x62, 0x63, 0},
    {0x53, 0x74, 0x72, 0x61, 0x00DF, 0x65, 0},
    {0x65, 0x66, 0x67, 0},
    {0x00E4, 0x62, 0x63, 0},
    {0x53, 0x74, 0x72, 0x61, 0x00DF, 0x65, 0}
};

const UChar CollationGermanTest::testTargetCases[][CollationGermanTest::MAX_TOKEN_LEN] =
{
    {0x47, 0x72, 0x6f, 0x73, 0x73, 0x69, 0x73, 0x74, 0},
    {0x61, 0x0308, 0x62, 0x63, 0},
    {0x54, 0x6f, 0x6e, 0},
    {0x54, 0x6f, 0x64, 0},
    {0x54, 0x6f, 0x66, 0x75, 0},
    {0x41, 0x0308, 0x62, 0x63, 0},
    {0x61, 0x0308, 0x62, 0x63, 0},
    {0x61, 0x65, 0x62, 0x63, 0},
    {0x53, 0x74, 0x72, 0x61, 0x73, 0x73, 0x65, 0},
    {0x65, 0x66, 0x67, 0},
    {0x61, 0x65, 0x62, 0x63, 0},
    {0x53, 0x74, 0x72, 0x61, 0x73, 0x73, 0x65, 0}
};

const Collator::EComparisonResult CollationGermanTest::results[][2] =
{
      //  Primary                Tertiary
        { Collator::LESS,        Collator::LESS },
        { Collator::EQUAL,        Collator::LESS },
        { Collator::GREATER,    Collator::GREATER },
        { Collator::GREATER,    Collator::GREATER },
        { Collator::GREATER,    Collator::GREATER },
        { Collator::EQUAL,        Collator::LESS },
        { Collator::EQUAL,        Collator::EQUAL },
        { Collator::LESS,        Collator::LESS },
        { Collator::EQUAL,        Collator::GREATER },
        { Collator::EQUAL,        Collator::EQUAL },
        { Collator::LESS,        Collator::LESS },
        { Collator::EQUAL,        Collator::GREATER }
};


void CollationGermanTest::TestTertiary(/* char* par */)
{
    if(myCollation == NULL ) {
        dataerrln("decoll: cannot start test, collator is null\n");
        return;
    }

    int32_t i = 0;
    UErrorCode status = U_ZERO_ERROR;
    myCollation->setStrength(Collator::TERTIARY);
    myCollation->setAttribute(UCOL_NORMALIZATION_MODE, UCOL_ON, status);
    for (i = 0; i < 12 ; i++)
    {
        doTest(myCollation, testSourceCases[i], testTargetCases[i], results[i][1]);
    }
}
void CollationGermanTest::TestPrimary(/* char* par */)
{
    if(myCollation == NULL ) {
        dataerrln("decoll: cannot start test, collator is null\n");
        return;
    }
    int32_t i;
    UErrorCode status = U_ZERO_ERROR;
    myCollation->setStrength(Collator::PRIMARY);
    myCollation->setAttribute(UCOL_NORMALIZATION_MODE, UCOL_ON, status);
    for (i = 0; i < 12 ; i++)
    {
        doTest(myCollation, testSourceCases[i], testTargetCases[i], results[i][0]);
    }
}

void CollationGermanTest::runIndexedTest( int32_t index, UBool exec, const char* &name, char* /*par*/ )
{
    if (exec) logln("TestSuite CollationGermanTest: ");
    switch (index)
    {
        case 0: name = "TestPrimary";   if (exec)   TestPrimary(/* par */); break;
        case 1: name = "TestTertiary";  if (exec)   TestTertiary(/* par */); break;
        default: name = ""; break;
    }
}

#endif /* #if !UCONFIG_NO_COLLATION */
