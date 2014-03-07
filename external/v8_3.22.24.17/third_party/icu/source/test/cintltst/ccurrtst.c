/********************************************************************
 * COPYRIGHT: 
 * Copyright (c) 1997-2009, International Business Machines Corporation and
 * others. All Rights Reserved.
 ********************************************************************/
/********************************************************************************
*
* File CCURRTST.C
*
* Modification History:
*        Name                     Description            
*     Madhu Katragadda             Ported for C API
*********************************************************************************
*/

#include <stdlib.h>

#include "unicode/utypes.h"

#if (!UCONFIG_NO_COLLATION)  /* This is not a formatting test. This is a collation test. */

#include "unicode/ucol.h"
#include "unicode/uloc.h"
#include "cintltst.h"
#include "ccurrtst.h"
#include "ccolltst.h"
#include "unicode/ustring.h"
#include "cmemory.h"

#define ARRAY_LENGTH(array) (sizeof array / sizeof array[0]) 

void addCurrencyCollTest(TestNode** root)
{
    
    addTest(root, &currTest, "tscoll/ccurrtst/currTest");
    
}


void currTest()
{
    /* All the currency symbols, in UCA order*/
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

#if 0
    /* All the currency symbols, in collation order*/
    static const UChar currency[][2] =
    {
        { 0x00a4, 0x0000}, /* generic currency*/
        { 0x0e3f, 0x0000}, /* baht*/
        { 0x00a2, 0x0000}, /* cent*/
        { 0x20a1, 0x0000}, /* colon*/
        { 0x20a2, 0x0000}, /* cruzeiro*/
        { 0x0024, 0x0000}, /* dollar */
        { 0x20ab, 0x0000}, /* dong */
        { 0x20ac, 0x0000}, /* euro */
        { 0x20a3, 0x0000}, /* franc */
        { 0x20a4, 0x0000}, /* lira */
        { 0x20a5, 0x0000}, /* mill */
        { 0x20a6, 0x0000}, /* naira */
        { 0x20a7, 0x0000}, /* peseta */
        { 0x00a3, 0x0000}, /* pound */
        { 0x20a8, 0x0000}, /* rupee */
        { 0x20aa, 0x0000}, /* shekel*/
        { 0x20a9, 0x0000}, /* won*/
        { 0x00a5, 0x0000}  /* yen*/
    };
#endif

    UChar source[2], target[2];
    int32_t i, j, sortklen;
    int res;
    UCollator *c;
    uint8_t *sortKey1, *sortKey2;
    UErrorCode status = U_ZERO_ERROR;
    UCollationResult compareResult, keyResult;
    UCollationResult expectedResult = UCOL_EQUAL;
    log_verbose("Testing currency of all locales\n");
    c = ucol_open("en_US", &status);
    if (U_FAILURE(status))
    {
        log_err_status(status, "collator open failed! :%s\n", myErrorName(status));
        return;
    }

    /*Compare each currency symbol against all the
     currency symbols, including itself*/
    for (i = 0; i < ARRAY_LENGTH(currency); i += 1)
    {
        for (j = 0; j < ARRAY_LENGTH(currency); j += 1)
        {
             u_strcpy(source, currency[i]);
             u_strcpy(target, currency[j]);
            
            if (i < j)
            {
                expectedResult = UCOL_LESS;
            }
            else if ( i == j)
            {
                expectedResult = UCOL_EQUAL;
            }
            else
            {
                expectedResult = UCOL_GREATER;
            }

            compareResult = ucol_strcoll(c, source, u_strlen(source), target, u_strlen(target));
            
            status = U_ZERO_ERROR;

            sortklen=ucol_getSortKey(c, source, u_strlen(source),  NULL, 0);
            sortKey1=(uint8_t*)malloc(sizeof(uint8_t) * (sortklen+1));
            ucol_getSortKey(c, source, u_strlen(source), sortKey1, sortklen+1);

            sortklen=ucol_getSortKey(c, target, u_strlen(target),  NULL, 0);
            sortKey2=(uint8_t*)malloc(sizeof(uint8_t) * (sortklen+1));
            ucol_getSortKey(c, target, u_strlen(target), sortKey2, sortklen+1);

            res = uprv_memcmp(sortKey1, sortKey2, sortklen);
            if (res < 0) keyResult = (UCollationResult)-1;
            else if (res > 0) keyResult = (UCollationResult)1;
            else keyResult = (UCollationResult)0;
            
            reportCResult( source, target, sortKey1, sortKey2, compareResult, keyResult, compareResult, expectedResult );

            free(sortKey1);
            free(sortKey2);

        }
    }
    ucol_close(c);
}

#endif /* #if !UCONFIG_NO_FORMATTING && !UCONFIG_NO_COLLATION */
