/********************************************************************
 * COPYRIGHT: 
 * Copyright (c) 1997-2009, International Business Machines Corporation and
 * others. All Rights Reserved.
 ********************************************************************/
/********************************************************************************
*
* File CDETST.C
*
* Modification History:
*        Name                     Description            
*     Madhu Katragadda            Ported for C API
*********************************************************************************/
/**
 * CollationGermanTest is a third level test class.  This tests the locale
 * specific primary, secondary and tertiary rules.  For example, the ignorable
 * character '-' in string "black-bird".  The en_US locale uses the default
 * collation rules as its sorting sequence.
 */

#include <stdlib.h>

#include "unicode/utypes.h"

#if !UCONFIG_NO_COLLATION

#include "unicode/ucol.h"
#include "unicode/uloc.h"
#include "cintltst.h"
#include "cdetst.h"
#include "ccolltst.h"
#include "callcoll.h"
#include "unicode/ustring.h"
#include "string.h"

static UCollator *myCollation;
const static UChar testSourceCases[][MAX_TOKEN_LEN] =
{
    {0x0047/*'G'*/, 0x0072/*'r'*/, 0x00F6, 0x00DF, 0x0065/*'e'*/, 0x0000},     
    {0x0061/*'a'*/, 0x0062/*'b'*/, 0x0063/*'c'*/, 0x0000},
    {0x0054/*'T'*/, 0x00F6, 0x006e/*'n'*/, 0x0065/*'e'*/, 0x0000},
    {0x0054/*'T'*/, 0x00F6, 0x006e/*'n'*/, 0x0065/*'e'*/, 0x0000},
    {0x0054/*'T'*/, 0x00F6, 0x006e/*'n'*/, 0x0065/*'e'*/, 0x0000},
    {0x0061/*'a'*/, 0x0308, 0x0062/*'b'*/, 0x0063/*'c'*/, 0x0000},
    {0x00E4, 0x0062/*'b'*/, 0x0063/*'c'*/, 0x0000},                    /*doubt in primary here        */
    {0x00E4, 0x0062/*'b'*/, 0x0063/*'c'*/, 0x0000},                    /*doubt in primary here*/
    {0x0053/*'S'*/, 0x0074/*'t'*/, 0x0072/*'r'*/, 0x0061/*'a'*/, 0x00DF, 0x0065/*'e'*/, 0x0000},
    {0x0065/*'e'*/, 0x0066/*'f'*/, 0x0067/*'g'*/, 0x0000},
    {0x00E4, 0x0062/*'b'*/, 0x0063/*'c'*/, 0x0000},                    /*doubt in primary here*/
    {0x0053/*'S'*/, 0x0074/*'t'*/, 0x0072/*'r'*/, 0x0061/*'a'*/, 0x00DF, 0x0065/*'e'*/, 0x0000}
};

const static UChar testTargetCases[][MAX_TOKEN_LEN] =
{
    {0x0047/*'G'*/, 0x0072/*'r'*/, 0x006f/*'o'*/, 0x0073/*'s'*/, 0x0073/*'s'*/, 0x0069/*'i'*/, 0x0073/*'s'*/, 0x0074/*'t'*/, 0x0000},    
    {0x0061/*'a'*/, 0x0308, 0x0062/*'b'*/, 0x0063/*'c'*/, 0x0000},
    {0x0054/*'T'*/, 0x006f/*'o'*/, 0x006e/*'n'*/, 0x0000},
    {0x0054/*'T'*/, 0x006f/*'o'*/, 0x0064/*'d'*/, 0x0000},
    {0x0054/*'T'*/, 0x006f/*'o'*/, 0x0066/*'f'*/, 0x0075/*'u'*/, 0x0000},
    {0x0041/*'A'*/, 0x0308, 0x0062/*'b'*/, 0x0063/*'c'*/, 0x0000},
    {0x0061/*'a'*/, 0x0308, 0x0062/*'b'*/, 0x0063/*'c'*/, 0x0000},                    /*doubt in primary here*/
    {0x0061/*'a'*/, 0x0065/*'e'*/, 0x0062/*'b'*/, 0x0063/*'c'*/, 0x0000},            /*doubt in primary here*/
    {0x0053/*'S'*/, 0x0074/*'t'*/, 0x0072/*'r'*/, 0x0061/*'a'*/, 0x0073/*'s'*/, 0x0073/*'s'*/, 0x0065/*'e'*/, 0x0000},
    {0x0065/*'e'*/, 0x0066/*'f'*/, 0x0067/*'g'*/, 0x0000},
    {0x0061/*'a'*/, 0x0065/*'e'*/, 0x0062/*'b'*/, 0x0063/*'c'*/, 0x0000},        /*doubt in primary here*/
    {0x0053/*'S'*/, 0x0074/*'t'*/, 0x0072/*'r'*/, 0x0061/*'a'*/, 0x0073/*'s'*/, 0x0073/*'s'*/, 0x0065/*'e'*/, 0x0000}
};

const static UCollationResult results[][2] =
{
      /*  Primary*/            /*    Tertiary*/
        { UCOL_LESS,        UCOL_LESS },        /*should be UCOL_GREATER for primary*/
        { UCOL_EQUAL,        UCOL_LESS },
        { UCOL_GREATER,        UCOL_GREATER },
        { UCOL_GREATER,        UCOL_GREATER },
        { UCOL_GREATER,        UCOL_GREATER },
        { UCOL_EQUAL,        UCOL_LESS },
        { UCOL_EQUAL,        UCOL_EQUAL },    /*should be UCOL_GREATER for primary*/
        { UCOL_LESS,        UCOL_LESS },    /*should be UCOL_GREATER for primary*/
        { UCOL_EQUAL,        UCOL_GREATER },
        { UCOL_EQUAL,        UCOL_EQUAL },
        { UCOL_LESS,        UCOL_LESS },   /*should be UCOL_GREATER for primary*/
        { UCOL_EQUAL,        UCOL_GREATER }
};




void addGermanCollTest(TestNode** root)
{
    

    addTest(root, &TestTertiary, "tscoll/cdetst/TestTertiary");
    addTest(root, &TestPrimary, "tscoll/cdetst/TestPrimary");
       


}

static void TestTertiary( )
{
    
    int32_t i;
    UErrorCode status = U_ZERO_ERROR;
    myCollation = ucol_open("de_DE", &status);
    if(U_FAILURE(status)){
        log_err_status(status, "ERROR: in creation of rule based collator: %s\n", myErrorName(status));
        return;
    }
    log_verbose("Testing German Collation with Tertiary strength\n");
    ucol_setAttribute(myCollation, UCOL_NORMALIZATION_MODE, UCOL_ON, &status);
    ucol_setStrength(myCollation, UCOL_TERTIARY);
    for (i = 0; i < 12 ; i++)
    {
        doTest(myCollation, testSourceCases[i], testTargetCases[i], results[i][1]);
    }
    ucol_close(myCollation);
}

static void TestPrimary()
{
    
    int32_t i;
    UErrorCode status = U_ZERO_ERROR;
    myCollation = ucol_open("de_DE", &status);
    if(U_FAILURE(status)){
        log_err_status(status, "ERROR: %s: in creation of rule based collator: %s\n", __FILE__, myErrorName(status));
        return;
    }
    log_verbose("Testing German Collation with primary strength\n");
    ucol_setStrength(myCollation, UCOL_PRIMARY);
    for (i = 0; i < 12 ; i++)
    {
        doTest(myCollation, testSourceCases[i], testTargetCases[i], results[i][0]);
    }
    ucol_close(myCollation);
}

#endif /* #if !UCONFIG_NO_COLLATION */
