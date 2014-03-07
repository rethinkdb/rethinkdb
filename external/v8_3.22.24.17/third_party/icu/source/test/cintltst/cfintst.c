/********************************************************************
 * Copyright (c) 1997-2009, International Business Machines
 * Corporation and others. All Rights Reserved.
 ********************************************************************
 *
 * File CFINTST.C
 *
 * Modification History:
 *        Name                     Description            
 *     Madhu Katragadda            Ported for C API
 ********************************************************************
 */

/**
 * CollationFinnishTest is a third level test class.  This tests the locale
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
#include "cfintst.h"
#include "unicode/ustring.h"
#include "string.h"

static UCollator *myCollation;
const static UChar testSourceCases[][MAX_TOKEN_LEN] = {
    {0x0077/*'w'*/, 0x0061/*'a'*/, 0x0074/*'t'*/, 0x0000},
    {0x0076/*'v'*/, 0x0061/*'a'*/, 0x0074/*'t'*/, 0x0000},
    {0x0061/*'a'*/, 0x00FC, 0x0062/*'b'*/, 0x0065/*'e'*/, 0x0063/*'c'*/, 0x006b/*'k'*/, 0x0000},
    {0x004c/*'L'*/, 0x00E5, 0x0076/*'v'*/, 0x0069/*'i'*/, 0x0000},
    {0x0077/*'w'*/, 0x0061/*'a'*/, 0x0074/*'t'*/, 0x0000}
};

const static UChar testTargetCases[][MAX_TOKEN_LEN] = {
    {0x0076/*'v'*/, 0x0061/*'a'*/, 0x0074/*'t'*/, 0x0000},
    {0x0077/*'w'*/, 0x0061/*'a'*/, 0x0079/*'y'*/, 0x0000},
    {0x0061/*'a'*/, 0x0078/*'x'*/, 0x0062/*'b'*/, 0x0065/*'e'*/, 0x0063/*'c'*/, 0x006b/*'k'*/, 0x0000},
    {0x004c/*'L'*/, 0x00E4, 0x0077/*'w'*/, 0x0065/*'e'*/, 0x0000},
    {0x0076/*'v'*/, 0x0061/*'a'*/, 0x0074/*'t'*/, 0x0000}
};

const static UCollationResult results[] = {
    UCOL_GREATER,
    UCOL_LESS,
    UCOL_GREATER,
    UCOL_LESS,
    /* test primary > 4*/
    UCOL_EQUAL
};



void addFinnishCollTest(TestNode** root)
{
    
    
    addTest(root, &TestPrimary, "tscoll/cficoll/TestPrimary");
    addTest(root, &TestTertiary, "tscoll/cficoll/TestTertiary");
    


}


static void TestTertiary( )
{
    
    int32_t i;
    UErrorCode status = U_ZERO_ERROR;
    myCollation = ucol_open("fi_FI@collation=standard", &status);
    if(U_FAILURE(status)){
        log_err_status(status, "ERROR: in creation of rule based collator: %s\n", myErrorName(status));
    }
    log_verbose("Testing Finnish Collation with Tertiary strength\n");
    ucol_setStrength(myCollation, UCOL_TERTIARY);
    for (i = 0; i < 4 ; i++)
    {
        doTest(myCollation, testSourceCases[i], testTargetCases[i], results[i]);
    }
    ucol_close(myCollation);
}

static void TestPrimary()
{
    
    int32_t i;
    UErrorCode status = U_ZERO_ERROR;
    myCollation = ucol_open("fi_FI@collation=standard", &status);
    if(U_FAILURE(status)){
        log_err_status(status, "ERROR: in creation of rule based collator: %s\n", myErrorName(status));
    }
    log_verbose("Testing Finnish Collation with Tertiary strength\n");
    ucol_setStrength(myCollation, UCOL_PRIMARY);
    for (i = 4; i < 5; i++)
    {
        doTest(myCollation, testSourceCases[i], testTargetCases[i], results[i]);
    }
    ucol_close(myCollation);
}

#endif /* #if !UCONFIG_NO_COLLATION */
