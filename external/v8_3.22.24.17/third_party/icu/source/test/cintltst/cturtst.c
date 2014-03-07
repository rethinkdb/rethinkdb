/********************************************************************
 * COPYRIGHT: 
 * Copyright (c) 1997-2009, International Business Machines Corporation and
 * others. All Rights Reserved.
 ********************************************************************/
/********************************************************************************
*
* File CTURTST.C
*
* Modification History:
*        Name                     Description            
*     Madhu Katragadda            Ported for C API
*********************************************************************************/
/**
 * CollationTurkishTest is a third level test class.  This tests the locale
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
#include "ccolltst.h"
#include "callcoll.h"
#include "cturtst.h"
#include "unicode/ustring.h"
#include "string.h"

static UCollator *myCollation;
const static UChar testSourceCases[][MAX_TOKEN_LEN] = {
    {0x0073/*'s'*/, 0x0327, 0x0000},
    {0x0076/*'v'*/, 0x00E4, 0x0074/*'t'*/, 0x0000},
    {0x006f/*'o'*/, 0x006c/*'l'*/, 0x0064/*'d'*/, 0x0000},
    {0x00FC, 0x006f/*'o'*/, 0x0069/*'i'*/, 0x0064/*'d'*/, 0x0000},
    {0x0068/*'h'*/, 0x011E, 0x0061/*'a'*/, 0x006c/*'l'*/, 0x0074/*'t'*/, 0x0000},
    {0x0073/*'s'*/, 0x0074/*'t'*/, 0x0072/*'r'*/, 0x0065/*'e'*/, 0x0073/*'s'*/, 0x015E, 0x0000},
    {0x0076/*'v'*/, 0x006f/*'o'*/, 0x0131, 0x0064/*'d'*/, 0x0000},
    {0x0069/*'i'*/, 0x0064/*'d'*/, 0x0065/*'e'*/, 0x0061/*'a'*/, 0x0000},
    {0x00FC, 0x006f/*'o'*/, 0x0069/*'i'*/, 0x0064 /*d'*/, 0x0000},
    {0x0076/*'v'*/, 0x006f/*'o'*/, 0x0131, 0x0064 /*d'*/, 0x0000},
    {0x0069/*'i'*/, 0x0064/*'d'*/, 0x0065/*'e'*/, 0x0061/*'a'*/, 0x0000},
};

const static UChar testTargetCases[][MAX_TOKEN_LEN] = {
    {0x0075/*'u'*/, 0x0308, 0x0000},
    {0x0076/*'v'*/, 0x0062/*'b'*/, 0x0074/*'t'*/, 0x0000},
    {0x00D6, 0x0061/*'a'*/, 0x0079/*'y'*/, 0x0000},
    {0x0076/*'v'*/, 0x006f/*'o'*/, 0x0069/*'i'*/, 0x0064 /*d'*/, 0x0000},
    {0x0068/*'h'*/, 0x0061/*'a'*/,  0x006c/*'l'*/, 0x0074/*'t'*/, 0x0000},
    {0x015E, 0x0074/*'t'*/, 0x0072/*'r'*/, 0x0065/*'e'*/, 0x015E, 0x0073/*'s'*/, 0x0000},
    {0x0076/*'v'*/, 0x006f/*'o'*/, 0x0069/*'i'*/, 0x0064 /*d'*/, 0x0000},
    {0x0049/*'I'*/, 0x0064/*'d'*/, 0x0065/*'e'*/, 0x0061/*'a'*/, 0x0000},
    {0x0076/*'v'*/, 0x006f/*'o'*/, 0x0069/*'i'*/, 0x0064 /*d'*/, 0x0000},
    {0x0076/*'v'*/, 0x006f/*'o'*/, 0x0069/*'i'*/, 0x0064 /*d'*/, 0x0000},
    {0x0049/*'I'*/, 0x0064/*'d'*/, 0x0065/*'e'*/, 0x0061/*'a'*/, 0x0000},
};

const static UCollationResult results[] = {
    UCOL_LESS,
    UCOL_LESS,
    UCOL_LESS,
    UCOL_LESS,
    UCOL_GREATER,
    UCOL_LESS,
    UCOL_LESS,
    UCOL_GREATER,
    /* test priamry > 8 */
    UCOL_LESS,
    UCOL_LESS, /*Turkish translator made a primary differnce between dotted and dotless I */
    UCOL_GREATER
};



void addTurkishCollTest(TestNode** root)
{
    
    addTest(root, &TestPrimary, "tscoll/cturtst/TestPrimary");
    addTest(root, &TestTertiary, "tscoll/cturtst/TestTertiary");


}

static void TestTertiary( )
{
    
    int32_t i;

    UErrorCode status = U_ZERO_ERROR;
    myCollation = ucol_open("tr", &status);
    if(U_FAILURE(status)){
        log_err_status(status, "ERROR: in creation of rule based collator: %s\n", myErrorName(status));
        return;
    }
    log_verbose("Testing Turkish Collation with Tertiary strength\n");
    ucol_setStrength(myCollation, UCOL_TERTIARY);
    for (i = 0; i < 8 ; i++)
    {
        doTest(myCollation, testSourceCases[i], testTargetCases[i], results[i]);
    }
    ucol_close(myCollation);
}

static void TestPrimary()
{
    
    int32_t i;

    UErrorCode status = U_ZERO_ERROR;
    myCollation = ucol_open("tr", &status);
    if(U_FAILURE(status)){
        log_err_status(status, "ERROR: in creation of rule based collator: %s\n", myErrorName(status));
        return;
    }
    log_verbose("Testing Turkish Collation with Primary strength\n");
    ucol_setStrength(myCollation, UCOL_PRIMARY);
    for (i = 8; i < 11; i++)
    {
        doTest(myCollation, testSourceCases[i], testTargetCases[i], results[i]);
    }
    ucol_close(myCollation);
}

#endif /* #if !UCONFIG_NO_COLLATION */
