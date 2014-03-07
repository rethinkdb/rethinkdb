/********************************************************************
 * COPYRIGHT: 
 * Copyright (c) 1997-2010, International Business Machines Corporation and
 * others. All Rights Reserved.
 ********************************************************************/

/***********************************************************************
* Modification history
* Date        Name        Description
* 02/14/2001  synwee      Added attributes in TestTertiary and 
*                         TestSecondary
***********************************************************************/

#include "unicode/utypes.h"

#if !UCONFIG_NO_COLLATION

#include "unicode/coll.h"
#include "unicode/tblcoll.h"
#include "unicode/unistr.h"
#include "unicode/sortkey.h"
#include "frcoll.h"

#include "sfwdchit.h"

CollationFrenchTest::CollationFrenchTest()
: myCollation(0)
{
    UErrorCode status = U_ZERO_ERROR;
    myCollation = Collator::createInstance(Locale::getCanadaFrench(), status);
    if(!myCollation || U_FAILURE(status)) {
        errcheckln(status, __FILE__ "failed to create! err " + UnicodeString(u_errorName(status)));
        /* if it wasn't already: */
        delete myCollation;
        myCollation = NULL;
    }
}

CollationFrenchTest::~CollationFrenchTest()
{
    delete myCollation;
}

const UChar CollationFrenchTest::testSourceCases[][CollationFrenchTest::MAX_TOKEN_LEN] =
{
    {0x0061/*'a'*/, 0x0062/*'b'*/, 0x0063/*'c'*/, 0x0000},
    {0x0043/*'C'*/, 0x004f/*'O'*/, 0x0054/*'T'*/, 0x0045/*'E'*/, 0x0000},
    {0x0063/*'c'*/, 0x006f/*'o'*/, 0x002d/*'-'*/, 0x006f/*'o'*/, 0x0070/*'p'*/, 0x0000},
    {0x0070/*'p'*/, 0x00EA, 0x0063/*'c'*/, 0x0068/*'h'*/, 0x0065/*'e'*/, 0x0000},
    {0x0070/*'p'*/, 0x00EA, 0x0063/*'c'*/, 0x0068/*'h'*/, 0x0065/*'e'*/, 0x0072/*'r'*/, 0x0000},
    {0x0070/*'p'*/, 0x00E9, 0x0063/*'c'*/, 0x0068/*'h'*/, 0x0065/*'e'*/, 0x0072/*'r'*/, 0x0000},
    {0x0070/*'p'*/, 0x00E9, 0x0063/*'c'*/, 0x0068/*'h'*/, 0x0065/*'e'*/, 0x0072/*'r'*/, 0x0000},
    {0x0048/*'H'*/, 0x0065/*'e'*/, 0x006c/*'l'*/, 0x006c/*'l'*/, 0x006f/*'o'*/, 0x0000},
    {0x01f1, 0x0000},
    {0xfb00, 0x0000},
    {0x01fa, 0x0000},
    {0x0101, 0x0000}
};

const UChar CollationFrenchTest::testTargetCases[][CollationFrenchTest::MAX_TOKEN_LEN] =
{
    {0x0041/*'A'*/, 0x0042/*'B'*/, 0x0043/*'C'*/, 0x0000},
    {0x0063/*'c'*/, 0x00f4, 0x0074/*'t'*/, 0x0065/*'e'*/, 0x0000},
    {0x0043/*'C'*/, 0x004f/*'O'*/, 0x004f/*'O'*/, 0x0050/*'P'*/, 0x0000},
    {0x0070/*'p'*/, 0x00E9, 0x0063/*'c'*/, 0x0068/*'h'*/, 0x00E9, 0x0000},
    {0x0070/*'p'*/,  0x00E9, 0x0063/*'c'*/, 0x0068/*'h'*/, 0x00E9, 0x0000},
    {0x0070/*'p'*/, 0x00EA, 0x0063/*'c'*/, 0x0068/*'h'*/, 0x0065/*'e'*/, 0x0000},
    {0x0070/*'p'*/, 0x00EA, 0x0063/*'c'*/, 0x0068/*'h'*/, 0x0065/*'e'*/, 0x0072/*'r'*/, 0x0000},
    {0x0068/*'h'*/, 0x0065/*'e'*/, 0x006c/*'l'*/, 0x006c/*'l'*/, 0x004f/*'O'*/, 0x0000},
    {0x01ee, 0x0000},
    {0x25ca, 0x0000},
    {0x00e0, 0x0000},
    {0x01df, 0x0000}
};

const Collator::EComparisonResult CollationFrenchTest::results[] =
{
    Collator::LESS,
    Collator::LESS,
    Collator::LESS, /*Collator::GREATER,*/
    Collator::LESS,
    Collator::GREATER,
    Collator::GREATER,
    Collator::LESS,
    Collator::GREATER,
    Collator::LESS, /*Collator::GREATER,*/
    Collator::GREATER,
    Collator::LESS,
    Collator::LESS
};

// 0x0300 is grave, 0x0301 is acute
// the order of elements in this array must be different than the order in CollationEnglishTest
const UChar CollationFrenchTest::testAcute[][CollationFrenchTest::MAX_TOKEN_LEN] =
{
/*00*/    {0x0065/*'e'*/, 0x0065/*'e'*/,  0x0000},
/*01*/    {0x0065/*'e'*/, 0x0301, 0x0065/*'e'*/,  0x0000},
/*02*/    {0x0065/*'e'*/, 0x0300, 0x0301, 0x0065/*'e'*/,  0x0000},
/*03*/    {0x0065/*'e'*/, 0x0300, 0x0065/*'e'*/,  0x0000},
/*04*/    {0x0065/*'e'*/, 0x0301, 0x0300, 0x0065/*'e'*/,  0x0000},
/*05*/    {0x0065/*'e'*/, 0x0065/*'e'*/, 0x0301, 0x0000}, 
/*06*/    {0x0065/*'e'*/, 0x0301, 0x0065/*'e'*/, 0x0301, 0x0000},
/*07*/    {0x0065/*'e'*/, 0x0300, 0x0301, 0x0065/*'e'*/, 0x0301, 0x0000},
/*08*/    {0x0065/*'e'*/, 0x0300, 0x0065/*'e'*/, 0x0301, 0x0000},
/*09*/    {0x0065/*'e'*/, 0x0301, 0x0300, 0x0065/*'e'*/, 0x0301, 0x0000},
/*0a*/    {0x0065/*'e'*/, 0x0065/*'e'*/, 0x0300, 0x0301, 0x0000},
/*0b*/    {0x0065/*'e'*/, 0x0301, 0x0065/*'e'*/, 0x0300, 0x0301, 0x0000},
/*0c*/    {0x0065/*'e'*/, 0x0300, 0x0301, 0x0065/*'e'*/, 0x0300, 0x0301, 0x0000},
/*0d*/    {0x0065/*'e'*/, 0x0300, 0x0065/*'e'*/, 0x0300, 0x0301, 0x0000},
/*0e*/    {0x0065/*'e'*/, 0x0301, 0x0300, 0x0065/*'e'*/, 0x0300, 0x0301, 0x0000},
/*0f*/    {0x0065/*'e'*/, 0x0065/*'e'*/, 0x0300, 0x0000},
/*10*/    {0x0065/*'e'*/, 0x0301, 0x0065/*'e'*/, 0x0300, 0x0000},
/*11*/    {0x0065/*'e'*/, 0x0300, 0x0301, 0x0065/*'e'*/, 0x0300, 0x0000},
/*12*/    {0x0065/*'e'*/, 0x0300, 0x0065/*'e'*/, 0x0300, 0x0000},
/*13*/    {0x0065/*'e'*/, 0x0301, 0x0300, 0x0065/*'e'*/, 0x0300, 0x0000},
/*14*/    {0x0065/*'e'*/, 0x0065/*'e'*/, 0x0301, 0x0300, 0x0000},
/*15*/    {0x0065/*'e'*/, 0x0301, 0x0065/*'e'*/, 0x0301, 0x0300, 0x0000},
/*16*/    {0x0065/*'e'*/, 0x0300, 0x0301, 0x0065/*'e'*/, 0x0301, 0x0300, 0x0000},
/*17*/    {0x0065/*'e'*/, 0x0300, 0x0065/*'e'*/, 0x0301, 0x0300, 0x0000},
/*18*/    {0x0065/*'e'*/, 0x0301, 0x0300, 0x0065/*'e'*/, 0x0301, 0x0300, 0x0000}
};

const UChar CollationFrenchTest::testBugs[][CollationFrenchTest::MAX_TOKEN_LEN] =
{
    {0x0061/*'a'*/, 0x000},
    {0x0041/*'A'*/, 0x000},
    {0x0065/*'e'*/, 0x000},
    {0x0045/*'E'*/, 0x000},
    {0x00e9, 0x000},
    {0x00e8, 0x000},
    {0x00ea, 0x000},
    {0x00eb, 0x000},
    {0x0065/*'e'*/, 0x0061/*'a'*/, 0x000},
    {0x0078/*'x'*/, 0x000}
};

void CollationFrenchTest::TestTertiary(/* char* par */)
{
    int32_t i = 0;
    UErrorCode status = U_ZERO_ERROR;
    myCollation->setStrength(Collator::TERTIARY);
    myCollation->setAttribute(UCOL_FRENCH_COLLATION, UCOL_ON, status);
    myCollation->setAttribute(UCOL_ALTERNATE_HANDLING, UCOL_SHIFTED, status);
    if (U_FAILURE(status)) {
        errln("Error setting attribute in French collator");
    }
    else
    {
        for (i = 0; i < 12 ; i++)
        {
            doTest(myCollation, testSourceCases[i], testTargetCases[i], results[i]);
        }
    }
}

void CollationFrenchTest::TestSecondary(/* char* par */)
{
    //test acute and grave ordering
    int32_t i = 0;
    int32_t j;
    Collator::EComparisonResult expected;
    UErrorCode status = U_ZERO_ERROR;
    //myCollation->setAttribute(UCOL_FRENCH_COLLATION, UCOL_ON, status);
    myCollation->setStrength(Collator::SECONDARY);
    if (U_FAILURE(status))
        errln("Error setting attribute in French collator");
    else
    {
        const int32_t testAcuteSize = (int32_t)(sizeof(testAcute) / sizeof(testAcute[0]));
        for (i = 0; i < testAcuteSize; i++)
        {
            for (j = 0; j < testAcuteSize; j++)
            {
                if (i <  j)
                    expected = Collator::LESS;
                else if (i == j)
                    expected = Collator::EQUAL;
                else // (i >  j)
                    expected = Collator::GREATER;
                doTest(myCollation, testAcute[i], testAcute[j], expected );
            }
        }
    }
}

void CollationFrenchTest::TestExtra(/* char* par */)
{
    int32_t i, j;
    myCollation->setStrength(Collator::TERTIARY);
    for (i = 0; i < 9 ; i++)
    {
        for (j = i + 1; j < 10; j += 1)
        {
            doTest(myCollation, testBugs[i], testBugs[j], Collator::LESS);
        }
    }
}

void CollationFrenchTest::runIndexedTest( int32_t index, UBool exec, const char* &name, char* /*par*/ )
{
    if (exec) logln("TestSuite CollationFrenchTest: ");

    if((!myCollation) && exec) {
        dataerrln(__FILE__ " cannot test - failed to create collator.");
        name = "some test";
        return;
    }

    switch (index) {
        case 0: name = "TestSecondary"; if (exec)   TestSecondary(/* par */); break;
        case 1: name = "TestTertiary";  if (exec)   TestTertiary(/* par */); break;
        case 2: name = "TestExtra";     if (exec)   TestExtra(/* par */); break;
        default: name = ""; break;
    }
}

#endif /* #if !UCONFIG_NO_COLLATION */
