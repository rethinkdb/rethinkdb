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

#ifndef _CURRCOLL
#include "currcoll.h"
#endif

#include "sfwdchit.h"

#define ARRAY_LENGTH(array) (sizeof array / sizeof array[0])

CollationCurrencyTest::CollationCurrencyTest()
{
}

CollationCurrencyTest::~CollationCurrencyTest()
{
}

void CollationCurrencyTest::currencyTest(/*char *par*/)
{
    // All the currency symbols, in collation order
    static const UChar currency[][2] =
    {
      { 0x00A4, 0x0000}, /*00A4; L; [14 36, 03, 03]    # [082B.0020.0002] # CURRENCY SIGN*/
      { 0x00A2, 0x0000}, /*00A2; L; [14 38, 03, 03]    # [082C.0020.0002] # CENT SIGN*/
      { 0xFFE0, 0x0000}, /*FFE0; L; [14 38, 03, 05]    # [082C.0020.0003] # FULLWIDTH CENT SIGN*/
      { 0x0024, 0x0000}, /*0024; L; [14 3A, 03, 03]    # [082D.0020.0002] # DOLLAR SIGN*/
      { 0xFF04, 0x0000}, /*FF04; L; [14 3A, 03, 05]    # [082D.0020.0003] # FULLWIDTH DOLLAR SIGN*/
      { 0xFE69, 0x0000}, /*FE69; L; [14 3A, 03, 1D]    # [082D.0020.000F] # SMALL DOLLAR SIGN*/
      { 0x00A3, 0x0000}, /*00A3; L; [14 3C, 03, 03]    # [082E.0020.0002] # POUND SIGN*/
      { 0xFFE1, 0x0000}, /*FFE1; L; [14 3C, 03, 05]    # [082E.0020.0003] # FULLWIDTH POUND SIGN*/
      { 0x00A5, 0x0000}, /*00A5; L; [14 3E, 03, 03]    # [082F.0020.0002] # YEN SIGN*/
      { 0xFFE5, 0x0000}, /*FFE5; L; [14 3E, 03, 05]    # [082F.0020.0003] # FULLWIDTH YEN SIGN*/
      { 0x09F2, 0x0000}, /*09F2; L; [14 40, 03, 03]    # [0830.0020.0002] # BENGALI RUPEE MARK*/
      { 0x09F3, 0x0000}, /*09F3; L; [14 42, 03, 03]    # [0831.0020.0002] # BENGALI RUPEE SIGN*/
      { 0x0E3F, 0x0000}, /*0E3F; L; [14 44, 03, 03]    # [0832.0020.0002] # THAI CURRENCY SYMBOL BAHT*/
      { 0x17DB, 0x0000}, /*17DB; L; [14 46, 03, 03]    # [0833.0020.0002] # KHMER CURRENCY SYMBOL RIEL*/
      { 0x20A0, 0x0000}, /*20A0; L; [14 48, 03, 03]    # [0834.0020.0002] # EURO-CURRENCY SIGN*/
      { 0x20A1, 0x0000}, /*20A1; L; [14 4A, 03, 03]    # [0835.0020.0002] # COLON SIGN*/
      { 0x20A2, 0x0000}, /*20A2; L; [14 4C, 03, 03]    # [0836.0020.0002] # CRUZEIRO SIGN*/
      { 0x20A3, 0x0000}, /*20A3; L; [14 4E, 03, 03]    # [0837.0020.0002] # FRENCH FRANC SIGN*/
      { 0x20A4, 0x0000}, /*20A4; L; [14 50, 03, 03]    # [0838.0020.0002] # LIRA SIGN*/
      { 0x20A5, 0x0000}, /*20A5; L; [14 52, 03, 03]    # [0839.0020.0002] # MILL SIGN*/
      { 0x20A6, 0x0000}, /*20A6; L; [14 54, 03, 03]    # [083A.0020.0002] # NAIRA SIGN*/
      { 0x20A7, 0x0000}, /*20A7; L; [14 56, 03, 03]    # [083B.0020.0002] # PESETA SIGN*/
      { 0x20A9, 0x0000}, /*20A9; L; [14 58, 03, 03]    # [083C.0020.0002] # WON SIGN*/
      { 0xFFE6, 0x0000}, /*FFE6; L; [14 58, 03, 05]    # [083C.0020.0003] # FULLWIDTH WON SIGN*/
      { 0x20AA, 0x0000}, /*20AA; L; [14 5A, 03, 03]    # [083D.0020.0002] # NEW SHEQEL SIGN*/
      { 0x20AB, 0x0000}, /*20AB; L; [14 5C, 03, 03]    # [083E.0020.0002] # DONG SIGN*/
      { 0x20AC, 0x0000}, /*20AC; L; [14 5E, 03, 03]    # [083F.0020.0002] # EURO SIGN*/
      { 0x20AD, 0x0000}, /*20AD; L; [14 60, 03, 03]    # [0840.0020.0002] # KIP SIGN*/
      { 0x20AE, 0x0000}, /*20AE; L; [14 62, 03, 03]    # [0841.0020.0002] # TUGRIK SIGN*/
      { 0x20AF, 0x0000}, /*20AF; L; [14 64, 03, 03]    # [0842.0020.0002] # DRACHMA SIGN*/
    };
    

    uint32_t i, j;
    UErrorCode status = U_ZERO_ERROR;
    Collator::EComparisonResult expectedResult = Collator::EQUAL;
    RuleBasedCollator *c = (RuleBasedCollator *)Collator::createInstance("en_US", status);

    if (U_FAILURE(status))
    {
        errcheckln (status, "Collator::createInstance() failed! - %s", u_errorName(status));
        return;
    }

    // Compare each currency symbol against all the
    // currency symbols, including itself
    for (i = 0; i < ARRAY_LENGTH(currency); i += 1)
    {
        for (j = 0; j < ARRAY_LENGTH(currency); j += 1)
        {
            UnicodeString source(currency[i], 1);
            UnicodeString target(currency[j], 1);

            if (i < j)
            {
                expectedResult = Collator::LESS;
            }
            else if ( i == j)
            {
                expectedResult = Collator::EQUAL;
            }
            else
            {
                expectedResult = Collator::GREATER;
            }

            Collator::EComparisonResult compareResult = c->compare(source, target);

            CollationKey sourceKey, targetKey;
            UErrorCode status = U_ZERO_ERROR;

            c->getCollationKey(source, sourceKey, status);

            if (U_FAILURE(status))
            {
                errln("Couldn't get collationKey for source");
                continue;
            }

            c->getCollationKey(target, targetKey, status);

            if (U_FAILURE(status))
            {
                errln("Couldn't get collationKey for target");
                continue;
            }

            Collator::EComparisonResult keyResult = sourceKey.compareTo(targetKey);

            reportCResult( source, target, sourceKey, targetKey, compareResult, keyResult, compareResult, expectedResult );

        }
    }
    delete c;
}

void CollationCurrencyTest::runIndexedTest(int32_t index, UBool exec, const char* &name, char* /*par*/)
{
    if (exec)
    {
        logln("Collation Currency Tests: ");
    }

    switch (index)
    {
        case  0: name = "currencyTest"; if (exec) currencyTest(/*par*/); break;
        default: name = ""; break;
    }
}

#endif /* #if !UCONFIG_NO_COLLATION */
