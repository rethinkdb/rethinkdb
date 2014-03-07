/********************************************************************
 * COPYRIGHT: 
 * Copyright (c) 1997-2009, International Business Machines Corporation and
 * others. All Rights Reserved.
 ********************************************************************/
/********************************************************************************
*
* File encoll.C
*
* Modification History:
*        Name                     Description            
*     Madhu Katragadda            Ported for C API
*********************************************************************************/
/**
 * CollationEnglishTest is a third level test class.  This tests the locale
 * specific primary, secondary and tertiary rules.  For example, the ignorable
 * character '-' in string "black-bird".  The en_US locale uses the default
 * collation rules as its sorting sequence.
 */

#include <stdlib.h>
#include <string.h>

#include "unicode/utypes.h"

#if !UCONFIG_NO_COLLATION

#include "unicode/ucol.h"
#include "unicode/uloc.h"
#include "cintltst.h"
#include "encoll.h"
#include "ccolltst.h"
#include "callcoll.h"
#include "unicode/ustring.h"

static UCollator *myCollation = NULL;
const static UChar testSourceCases[][MAX_TOKEN_LEN] = {
        {(UChar)0x0061 /* 'a' */, (UChar)0x0062 /* 'b' */, 0},
        {(UChar)0x0062 /* 'b' */, (UChar)0x006C /* 'l' */, (UChar)0x0061 /* 'a' */, (UChar)0x0063 /* 'c' */, (UChar)0x006B /* 'k' */, (UChar)0x002D /* '-' */, (UChar)0x0062 /* 'b' */, (UChar)0x0069 /* 'i' */, (UChar)0x0072 /* 'r' */, (UChar)0x0064 /* 'd' */, 0},
        {(UChar)0x0062 /* 'b' */, (UChar)0x006C /* 'l' */, (UChar)0x0061 /* 'a' */, (UChar)0x0063 /* 'c' */, (UChar)0x006B /* 'k' */, (UChar)0x0020 /* ' ' */, (UChar)0x0062 /* 'b' */, (UChar)0x0069 /* 'i' */, (UChar)0x0072 /* 'r' */, (UChar)0x0064 /* 'd' */, 0},
        {(UChar)0x0062 /* 'b' */, (UChar)0x006C /* 'l' */, (UChar)0x0061 /* 'a' */, (UChar)0x0063 /* 'c' */, (UChar)0x006B /* 'k' */, (UChar)0x002D /* '-' */, (UChar)0x0062 /* 'b' */, (UChar)0x0069 /* 'i' */, (UChar)0x0072 /* 'r' */, (UChar)0x0064 /* 'd' */, 0},
        {(UChar)0x0048 /* 'H' */, (UChar)0x0065 /* 'e' */, (UChar)0x006C /* 'l' */, (UChar)0x006C /* 'l' */, (UChar)0x006F /* 'o' */, 0},
        {(UChar)0x0041 /* 'A' */, (UChar)0x0042 /* 'B' */, (UChar)0x0043 /* 'C' */, 0}, 
        {(UChar)0x0061 /* 'a' */, (UChar)0x0062 /* 'b' */, (UChar)0x0063 /* 'c' */, 0},
        {(UChar)0x0062 /* 'b' */, (UChar)0x006C /* 'l' */, (UChar)0x0061 /* 'a' */, (UChar)0x0063 /* 'c' */, (UChar)0x006B /* 'k' */, (UChar)0x0062 /* 'b' */, (UChar)0x0069 /* 'i' */, (UChar)0x0072 /* 'r' */, (UChar)0x0064 /* 'd' */, 0},
        {(UChar)0x0062 /* 'b' */, (UChar)0x006C /* 'l' */, (UChar)0x0061 /* 'a' */, (UChar)0x0063 /* 'c' */, (UChar)0x006B /* 'k' */, (UChar)0x002D /* '-' */, (UChar)0x0062 /* 'b' */, (UChar)0x0069 /* 'i' */, (UChar)0x0072 /* 'r' */, (UChar)0x0064 /* 'd' */, 0},
        {(UChar)0x0062 /* 'b' */, (UChar)0x006C /* 'l' */, (UChar)0x0061 /* 'a' */, (UChar)0x0063 /* 'c' */, (UChar)0x006B /* 'k' */, (UChar)0x002D /* '-' */, (UChar)0x0062 /* 'b' */, (UChar)0x0069 /* 'i' */, (UChar)0x0072 /* 'r' */, (UChar)0x0064 /* 'd' */, 0},
        {(UChar)0x0070 /* 'p' */, 0x00EA, (UChar)0x0063 /* 'c' */, (UChar)0x0068 /* 'h' */, (UChar)0x0065 /* 'e' */, 0},                                            
        {(UChar)0x0070 /* 'p' */, 0x00E9, (UChar)0x0063 /* 'c' */, (UChar)0x0068 /* 'h' */, 0x00E9, 0},
        {0x00C4, (UChar)0x0042 /* 'B' */, 0x0308, (UChar)0x0043 /* 'C' */, 0x0308, 0},
        {(UChar)0x0061 /* 'a' */, 0x0308, (UChar)0x0062 /* 'b' */, (UChar)0x0063 /* 'c' */, 0},
        {(UChar)0x0070 /* 'p' */, 0x00E9, (UChar)0x0063 /* 'c' */, (UChar)0x0068 /* 'h' */, (UChar)0x0065 /* 'e' */, (UChar)0x0072 /* 'r' */, 0},
        {(UChar)0x0072 /* 'r' */, (UChar)0x006F /* 'o' */, (UChar)0x006C /* 'l' */, (UChar)0x0065 /* 'e' */, (UChar)0x0073 /* 's' */, 0},
        {(UChar)0x0061 /* 'a' */, (UChar)0x0062 /* 'b' */, (UChar)0x0063 /* 'c' */, 0},
        {(UChar)0x0041 /* 'A' */, 0},
        {(UChar)0x0041 /* 'A' */, 0},
        {(UChar)0x0061 /* 'a' */, (UChar)0x0062 /* 'b' */, 0},                                                                
        {(UChar)0x0074 /* 't' */, (UChar)0x0063 /* 'c' */, (UChar)0x006F /* 'o' */, (UChar)0x006D /* 'm' */, (UChar)0x0070 /* 'p' */, (UChar)0x0061 /* 'a' */, (UChar)0x0072 /* 'r' */, (UChar)0x0065 /* 'e' */, (UChar)0x0070 /* 'p' */, (UChar)0x006C /* 'l' */, (UChar)0x0061 /* 'a' */, (UChar)0x0069 /* 'i' */, (UChar)0x006E /* 'n' */, 0},
        {(UChar)0x0061 /* 'a' */, (UChar)0x0062 /* 'b' */, 0}, 
        {(UChar)0x0061 /* 'a' */, (UChar)0x0023 /* '#' */, (UChar)0x0062 /* 'b' */, 0},
        {(UChar)0x0061 /* 'a' */, (UChar)0x0023 /* '#' */, (UChar)0x0062 /* 'b' */, 0},
        {(UChar)0x0061 /* 'a' */, (UChar)0x0062 /* 'b' */, (UChar)0x0063 /* 'c' */, 0},
        {(UChar)0x0041 /* 'A' */, (UChar)0x0062 /* 'b' */, (UChar)0x0063 /* 'c' */, (UChar)0x0064 /* 'd' */, (UChar)0x0061 /* 'a' */, 0},
        {(UChar)0x0061 /* 'a' */, (UChar)0x0062 /* 'b' */, (UChar)0x0063 /* 'c' */, (UChar)0x0064 /* 'd' */, (UChar)0x0061 /* 'a' */, 0},
        {(UChar)0x0061 /* 'a' */, (UChar)0x0062 /* 'b' */, (UChar)0x0063 /* 'c' */, (UChar)0x0064 /* 'd' */, (UChar)0x0061 /* 'a' */, 0},
        {0x00E6, (UChar)0x0062 /* 'b' */, (UChar)0x0063 /* 'c' */, (UChar)0x0064 /* 'd' */, (UChar)0x0061 /* 'a' */, 0},
        {0x00E4, (UChar)0x0062 /* 'b' */, (UChar)0x0063 /* 'c' */, (UChar)0x0064 /* 'd' */, (UChar)0x0061 /* 'a' */, 0},                                            
        {(UChar)0x0061 /* 'a' */, (UChar)0x0062 /* 'b' */, (UChar)0x0063 /* 'c' */, 0},
        {(UChar)0x0061 /* 'a' */, (UChar)0x0062 /* 'b' */, (UChar)0x0063 /* 'c' */, 0},
        {(UChar)0x0061 /* 'a' */, (UChar)0x0062 /* 'b' */, (UChar)0x0063 /* 'c' */, 0},
        {(UChar)0x0061 /* 'a' */, (UChar)0x0062 /* 'b' */, (UChar)0x0063 /* 'c' */, 0},
        {(UChar)0x0061 /* 'a' */, (UChar)0x0062 /* 'b' */, (UChar)0x0063 /* 'c' */, 0},
        {(UChar)0x0061 /* 'a' */, (UChar)0x0063 /* 'c' */, (UChar)0x0048 /* 'H' */, (UChar)0x0063 /* 'c' */, 0},
        {(UChar)0x0061 /* 'a' */, 0x0308, (UChar)0x0062 /* 'b' */, (UChar)0x0063 /* 'c' */, 0},
        {(UChar)0x0074 /* 't' */, (UChar)0x0068 /* 'h' */, (UChar)0x0069 /* 'i' */, 0x0302, (UChar)0x0073 /* 's' */, 0},
        {(UChar)0x0070 /* 'p' */, 0x00EA, (UChar)0x0063 /* 'c' */, (UChar)0x0068 /* 'h' */, (UChar)0x0065 /* 'e' */},
        {(UChar)0x0061 /* 'a' */, (UChar)0x0062 /* 'b' */, (UChar)0x0063 /* 'c' */, 0},                                                         
        {(UChar)0x0061 /* 'a' */, (UChar)0x0062 /* 'b' */, (UChar)0x0063 /* 'c' */, 0},
        {(UChar)0x0061 /* 'a' */, (UChar)0x0062 /* 'b' */, (UChar)0x0063 /* 'c' */, 0},
        {(UChar)0x0061 /* 'a' */, 0x00E6, (UChar)0x0063 /* 'c' */, 0},
        {(UChar)0x0061 /* 'a' */, (UChar)0x0062 /* 'b' */, (UChar)0x0063 /* 'c' */, 0},
        {(UChar)0x0061 /* 'a' */, (UChar)0x0062 /* 'b' */, (UChar)0x0063 /* 'c' */, 0},
        {(UChar)0x0061 /* 'a' */, 0x00E6, (UChar)0x0063 /* 'c' */, 0},
        {(UChar)0x0061 /* 'a' */, (UChar)0x0062 /* 'b' */, (UChar)0x0063 /* 'c' */, 0},
        {(UChar)0x0061 /* 'a' */, (UChar)0x0062 /* 'b' */, (UChar)0x0063 /* 'c' */, 0},               
        {(UChar)0x0070 /* 'p' */, 0x00E9, (UChar)0x0063 /* 'c' */, (UChar)0x0068 /* 'h' */, 0x00E9, 0}                                            
};

const static UChar testTargetCases[][MAX_TOKEN_LEN] = {
        {(UChar)0x0061 /* 'a' */, (UChar)0x0062 /* 'b' */, (UChar)0x0063 /* 'c' */, 0},
        {(UChar)0x0062 /* 'b' */, (UChar)0x006C /* 'l' */, (UChar)0x0061 /* 'a' */, (UChar)0x0063 /* 'c' */, (UChar)0x006B /* 'k' */, (UChar)0x0062 /* 'b' */, (UChar)0x0069 /* 'i' */, (UChar)0x0072 /* 'r' */, (UChar)0x0064 /* 'd' */, 0},
        {(UChar)0x0062 /* 'b' */, (UChar)0x006C /* 'l' */, (UChar)0x0061 /* 'a' */, (UChar)0x0063 /* 'c' */, (UChar)0x006B /* 'k' */, (UChar)0x002D /* '-' */, (UChar)0x0062 /* 'b' */, (UChar)0x0069 /* 'i' */, (UChar)0x0072 /* 'r' */, (UChar)0x0064 /* 'd' */, 0},
        {(UChar)0x0062 /* 'b' */, (UChar)0x006C /* 'l' */, (UChar)0x0061 /* 'a' */, (UChar)0x0063 /* 'c' */, (UChar)0x006B /* 'k' */, 0},
        {(UChar)0x0068 /* 'h' */, (UChar)0x0065 /* 'e' */, (UChar)0x006C /* 'l' */, (UChar)0x006C /* 'l' */, (UChar)0x006F /* 'o' */, 0},
        {(UChar)0x0041 /* 'A' */, (UChar)0x0042 /* 'B' */, (UChar)0x0043 /* 'C' */, 0},
        {(UChar)0x0041 /* 'A' */, (UChar)0x0042 /* 'B' */, (UChar)0x0043 /* 'C' */, 0},
        {(UChar)0x0062 /* 'b' */, (UChar)0x006C /* 'l' */, (UChar)0x0061 /* 'a' */, (UChar)0x0063 /* 'c' */, (UChar)0x006B /* 'k' */, (UChar)0x0062 /* 'b' */, (UChar)0x0069 /* 'i' */, (UChar)0x0072 /* 'r' */, (UChar)0x0064 /* 'd' */, (UChar)0x0073 /* 's' */, 0},
        {(UChar)0x0062 /* 'b' */, (UChar)0x006C /* 'l' */, (UChar)0x0061 /* 'a' */, (UChar)0x0063 /* 'c' */, (UChar)0x006B /* 'k' */, (UChar)0x0062 /* 'b' */, (UChar)0x0069 /* 'i' */, (UChar)0x0072 /* 'r' */, (UChar)0x0064 /* 'd' */, (UChar)0x0073 /* 's' */, 0},
        {(UChar)0x0062 /* 'b' */, (UChar)0x006C /* 'l' */, (UChar)0x0061 /* 'a' */, (UChar)0x0063 /* 'c' */, (UChar)0x006B /* 'k' */, (UChar)0x0062 /* 'b' */, (UChar)0x0069 /* 'i' */, (UChar)0x0072 /* 'r' */, (UChar)0x0064 /* 'd' */, 0},                             
        {(UChar)0x0070 /* 'p' */, 0x00E9, (UChar)0x0063 /* 'c' */, (UChar)0x0068 /* 'h' */, 0x00E9, 0},
        {(UChar)0x0070 /* 'p' */, 0x00E9, (UChar)0x0063 /* 'c' */, (UChar)0x0068 /* 'h' */, (UChar)0x0065 /* 'e' */, (UChar)0x0072 /* 'r' */, 0},
        {0x00C4, (UChar)0x0042 /* 'B' */, 0x0308, (UChar)0x0043 /* 'C' */, 0x0308, 0},
        {(UChar)0x0041 /* 'A' */, 0x0308, (UChar)0x0062 /* 'b' */, (UChar)0x0063 /* 'c' */, 0},
        {(UChar)0x0070 /* 'p' */, 0x00E9, (UChar)0x0063 /* 'c' */, (UChar)0x0068 /* 'h' */, (UChar)0x0065 /* 'e' */, 0},
        {(UChar)0x0072 /* 'r' */, (UChar)0x006F /* 'o' */, 0x0302, (UChar)0x006C /* 'l' */, (UChar)0x0065 /* 'e' */, 0},
        {(UChar)0x0041 /* 'A' */, 0x00E1, (UChar)0x0063 /* 'c' */, (UChar)0x0064 /* 'd' */, 0},
        {(UChar)0x0041 /* 'A' */, 0x00E1, (UChar)0x0063 /* 'c' */, (UChar)0x0064 /* 'd' */, 0},
        {(UChar)0x0061 /* 'a' */, (UChar)0x0062 /* 'b' */, (UChar)0x0063 /* 'c' */, 0},
        {(UChar)0x0061 /* 'a' */, (UChar)0x0062 /* 'b' */, (UChar)0x0063 /* 'c' */, 0},                                                             
        {(UChar)0x0054 /* 'T' */, (UChar)0x0043 /* 'C' */, (UChar)0x006F /* 'o' */, (UChar)0x006D /* 'm' */, (UChar)0x0070 /* 'p' */, (UChar)0x0061 /* 'a' */, (UChar)0x0072 /* 'r' */, (UChar)0x0065 /* 'e' */, (UChar)0x0050 /* 'P' */, (UChar)0x006C /* 'l' */, (UChar)0x0061 /* 'a' */, (UChar)0x0069 /* 'i' */, (UChar)0x006E /* 'n' */, 0},
        {(UChar)0x0061 /* 'a' */, (UChar)0x0042 /* 'B' */, (UChar)0x0063 /* 'c' */, 0},
        {(UChar)0x0061 /* 'a' */, (UChar)0x0023 /* '#' */, (UChar)0x0042 /* 'B' */, 0},
        {(UChar)0x0061 /* 'a' */, (UChar)0x0026 /* '&' */, (UChar)0x0062 /* 'b' */, 0},
        {(UChar)0x0061 /* 'a' */, (UChar)0x0023 /* '#' */, (UChar)0x0063 /* 'c' */, 0},
        {(UChar)0x0061 /* 'a' */, (UChar)0x0062 /* 'b' */, (UChar)0x0063 /* 'c' */, (UChar)0x0064 /* 'd' */, (UChar)0x0061 /* 'a' */, 0},
        {0x00C4, (UChar)0x0062 /* 'b' */, (UChar)0x0063 /* 'c' */, (UChar)0x0064 /* 'd' */, (UChar)0x0061 /* 'a' */, 0},
        {0x00E4, (UChar)0x0062 /* 'b' */, (UChar)0x0063 /* 'c' */, (UChar)0x0064 /* 'd' */, (UChar)0x0061 /* 'a' */, 0},
        {0x00C4, (UChar)0x0062 /* 'b' */, (UChar)0x0063 /* 'c' */, (UChar)0x0064 /* 'd' */, (UChar)0x0061 /* 'a' */, 0},
        {0x00C4, (UChar)0x0062 /* 'b' */, (UChar)0x0063 /* 'c' */, (UChar)0x0064 /* 'd' */, (UChar)0x0061 /* 'a' */, 0},                                             
        {(UChar)0x0061 /* 'a' */, (UChar)0x0062 /* 'b' */, (UChar)0x0023 /* '#' */, (UChar)0x0063 /* 'c' */, 0},
        {(UChar)0x0061 /* 'a' */, (UChar)0x0062 /* 'b' */, (UChar)0x0063 /* 'c' */, 0},
        {(UChar)0x0061 /* 'a' */, (UChar)0x0062 /* 'b' */, (UChar)0x003D /* '=' */, (UChar)0x0063 /* 'c' */, 0},
        {(UChar)0x0061 /* 'a' */, (UChar)0x0062 /* 'b' */, (UChar)0x0064 /* 'd' */, 0},
        {0x00E4, (UChar)0x0062 /* 'b' */, (UChar)0x0063 /* 'c' */, 0},
        {(UChar)0x0061 /* 'a' */, (UChar)0x0043 /* 'C' */, (UChar)0x0048 /* 'H' */, (UChar)0x0063 /* 'c' */, 0},
        {0x00E4, (UChar)0x0062 /* 'b' */, (UChar)0x0063 /* 'c' */, 0},
        {(UChar)0x0074 /* 't' */, (UChar)0x0068 /* 'h' */, 0x00EE, (UChar)0x0073 /* 's' */, 0},
        {(UChar)0x0070 /* 'p' */, 0x00E9, (UChar)0x0063 /* 'c' */, (UChar)0x0068 /* 'h' */, 0x00E9, 0},
        {(UChar)0x0061 /* 'a' */, (UChar)0x0042 /* 'B' */, (UChar)0x0043 /* 'C' */, 0},                                                          
        {(UChar)0x0061 /* 'a' */, (UChar)0x0062 /* 'b' */, (UChar)0x0064 /* 'd' */, 0},
        {0x00E4, (UChar)0x0062 /* 'b' */, (UChar)0x0063 /* 'c' */, 0},
        {(UChar)0x0061 /* 'a' */, 0x00C6, (UChar)0x0063 /* 'c' */, 0},
        {(UChar)0x0061 /* 'a' */, (UChar)0x0042 /* 'B' */, (UChar)0x0064 /* 'd' */, 0},
        {0x00E4, (UChar)0x0062 /* 'b' */, (UChar)0x0063 /* 'c' */, 0},
        {(UChar)0x0061 /* 'a' */, 0x00C6, (UChar)0x0063 /* 'c' */, 0},
        {(UChar)0x0061 /* 'a' */, (UChar)0x0042 /* 'B' */, (UChar)0x0064 /* 'd' */, 0},
        {0x00E4, (UChar)0x0062 /* 'b' */, (UChar)0x0063 /* 'c' */, 0},          
        {(UChar)0x0070 /* 'p' */, 0x00EA, (UChar)0x0063 /* 'c' */, (UChar)0x0068 /* 'h' */, (UChar)0x0065 /* 'e' */, 0}                                                 
};

const static UCollationResult results[] = {
        UCOL_LESS, 
        UCOL_LESS, /*UCOL_GREATER,*/
        UCOL_LESS,
        UCOL_GREATER,
        UCOL_GREATER,
        UCOL_EQUAL,
        UCOL_LESS,
        UCOL_LESS,
        UCOL_LESS,
        UCOL_LESS, /*UCOL_GREATER,*/                                                          /* 10 */
        UCOL_GREATER,
        UCOL_LESS,
        UCOL_EQUAL,
        UCOL_LESS,
        UCOL_GREATER,
        UCOL_GREATER,
        UCOL_GREATER,
        UCOL_LESS,
        UCOL_LESS,
        UCOL_LESS,                                                             /* 20 */
        UCOL_LESS,
        UCOL_LESS,
        UCOL_LESS,
        UCOL_GREATER,
        UCOL_GREATER,
        UCOL_GREATER,
        /* Test Tertiary  > 26 */
        UCOL_LESS,
        UCOL_LESS,
        UCOL_GREATER,
        UCOL_LESS,                                                             /* 30 */
        UCOL_GREATER,
        UCOL_EQUAL,
        UCOL_GREATER,
        UCOL_LESS,
        UCOL_LESS,
        UCOL_LESS,
        /* test identical > 36 */
        UCOL_EQUAL,
        UCOL_EQUAL,
        /* test primary > 38 */
        UCOL_EQUAL,
        UCOL_EQUAL,                                                            /* 40 */
        UCOL_LESS,
        UCOL_EQUAL,
        UCOL_EQUAL,
        /* test secondary > 43 */
        UCOL_LESS,
        UCOL_LESS,
        UCOL_EQUAL,
        UCOL_LESS,
        UCOL_LESS, 
        UCOL_LESS                                                                         /* 49 */
};

const static UChar testBugs[][MAX_TOKEN_LEN] = {
    {(UChar)0x0061 /* 'a' */, 0},
    {(UChar)0x0041 /* 'A' */, 0},
    {(UChar)0x0065 /* 'e' */, 0},
    {(UChar)0x0045 /* 'E' */, 0},
    {0x00e9, 0},
    {0x00e8, 0},
    {0x00ea, 0},
    {0x00eb, 0},
    {(UChar)0x0065 /* 'e' */, (UChar)0x0061 /* 'a' */, 0},
    {(UChar)0x0078 /* 'x' */, 0}
        
};

/* 0x0300 is grave, 0x0301 is acute
   the order of elements in this array must be different than the order in CollationFrenchTest */
const static UChar testAcute[][MAX_TOKEN_LEN] = {
    {(UChar)0x0065 /* 'e' */, (UChar)0x0065 /* 'e' */, 0},
    {(UChar)0x0065 /* 'e' */, (UChar)0x0065 /* 'e' */, 0x0301, 0},
    {(UChar)0x0065 /* 'e' */, (UChar)0x0065 /* 'e' */, 0x0301, 0x0300, 0},
    {(UChar)0x0065 /* 'e' */, (UChar)0x0065 /* 'e' */, 0x0300, 0},
    {(UChar)0x0065 /* 'e' */, (UChar)0x0065 /* 'e' */, 0x0300, 0x0301, 0},
    {(UChar)0x0065 /* 'e' */, 0x0301, (UChar)0x0065 /* 'e' */, 0},
    {(UChar)0x0065 /* 'e' */, 0x0301, (UChar)0x0065 /* 'e' */, 0x0301, 0},
    {(UChar)0x0065 /* 'e' */, 0x0301, (UChar)0x0065 /* 'e' */, 0x0301, 0x0300, 0},
    {(UChar)0x0065 /* 'e' */, 0x0301, (UChar)0x0065 /* 'e' */, 0x0300, 0},
    {(UChar)0x0065 /* 'e' */, 0x0301, (UChar)0x0065 /* 'e' */, 0x0300, 0x0301, 0},
    {(UChar)0x0065 /* 'e' */, 0x0301, 0x0300, (UChar)0x0065 /* 'e' */, 0},
    {(UChar)0x0065 /* 'e' */, 0x0301, 0x0300, (UChar)0x0065 /* 'e' */, 0x0301, 0},
    {(UChar)0x0065 /* 'e' */, 0x0301, 0x0300, (UChar)0x0065 /* 'e' */, 0x0301, 0x0300, 0},
    {(UChar)0x0065 /* 'e' */, 0x0301, 0x0300, (UChar)0x0065 /* 'e' */, 0x0300, 0},
    {(UChar)0x0065 /* 'e' */, 0x0301, 0x0300, (UChar)0x0065 /* 'e' */, 0x0300, 0x0301, 0},
    {(UChar)0x0065 /* 'e' */, 0x0300, (UChar)0x0065 /* 'e' */, 0},
    {(UChar)0x0065 /* 'e' */, 0x0300, (UChar)0x0065 /* 'e' */, 0x0301, 0},
    {(UChar)0x0065 /* 'e' */, 0x0300, (UChar)0x0065 /* 'e' */, 0x0301, 0x0300, 0},
    {(UChar)0x0065 /* 'e' */, 0x0300, (UChar)0x0065 /* 'e' */, 0x0300, 0},
    {(UChar)0x0065 /* 'e' */, 0x0300, (UChar)0x0065 /* 'e' */, 0x0300, 0x0301, 0},
    {(UChar)0x0065 /* 'e' */, 0x0300, 0x0301, (UChar)0x0065 /* 'e' */, 0},
    {(UChar)0x0065 /* 'e' */, 0x0300, 0x0301, (UChar)0x0065 /* 'e' */, 0x0301, 0},
    {(UChar)0x0065 /* 'e' */, 0x0300, 0x0301, (UChar)0x0065 /* 'e' */, 0x0301, 0x0300, 0},
    {(UChar)0x0065 /* 'e' */, 0x0300, 0x0301, (UChar)0x0065 /* 'e' */, 0x0300, 0},
    {(UChar)0x0065 /* 'e' */, 0x0300, 0x0301, (UChar)0x0065 /* 'e' */, 0x0300, 0x0301, 0}
};

static const UChar testMore[][MAX_TOKEN_LEN] = {
    {(UChar)0x0061 /* 'a' */, (UChar)0x0065 /* 'e' */, 0},
    { 0x00E6, 0},
    { 0x00C6, 0},
    {(UChar)0x0061 /* 'a' */, (UChar)0x0066 /* 'f' */, 0},
    {(UChar)0x006F /* 'o' */, (UChar)0x0065 /* 'e' */, 0},
    { 0x0153, 0},
    { 0x0152, 0},
    {(UChar)0x006F /* 'o' */, (UChar)0x0066 /* 'f' */, 0},
};


void addEnglishCollTest(TestNode** root)
{
    
    addTest(root, &TestPrimary, "tscoll/encoll/TestPrimary");
    addTest(root, &TestSecondary, "tscoll/encoll/TestSecondary");
    addTest(root, &TestTertiary, "tscoll/encoll/TestTertiary");

}

static void TestTertiary( )
{
    int32_t testMoreSize;
    UCollationResult expected=UCOL_EQUAL;
    int32_t i,j;
    UErrorCode status = U_ZERO_ERROR;
    myCollation = ucol_open("en_US", &status);
    if(U_FAILURE(status)){
        log_err_status(status, "ERROR: in creation of rule based collator: %s\n", myErrorName(status));
        return;
    }
    log_verbose("Testing English Collation with Tertiary strength\n");

    ucol_setStrength(myCollation, UCOL_TERTIARY);
    for (i = 0; i < 38 ; i++)
    {
        doTest(myCollation, testSourceCases[i], testTargetCases[i], results[i]);
    }
  

    j = 0;
   for (i = 0; i < 10; i++)
    {
        for (j = i+1; j < 10; j++)
        {
            doTest(myCollation, testBugs[i], testBugs[j], UCOL_LESS);
        }
   }
    /*test more interesting cases */
    testMoreSize = sizeof(testMore) / sizeof(testMore[0]);
    for (i = 0; i < testMoreSize; i++)
    {
        for (j = 0; j < testMoreSize; j++)
        {
            if (i <  j) expected = UCOL_LESS;
            if (i == j) expected = UCOL_EQUAL;
            if (i >  j) expected = UCOL_GREATER;
            doTest(myCollation, testMore[i], testMore[j], expected );
        }
    }
    ucol_close(myCollation);
}

static void TestPrimary()
{
    
    int32_t i;
    UErrorCode status = U_ZERO_ERROR;
    myCollation = ucol_open("en_US", &status);
    if(U_FAILURE(status)){
        log_err_status(status, "ERROR: in creation of rule based collator: %s\n", myErrorName(status));
        return;
    }
    ucol_setStrength(myCollation, UCOL_PRIMARY);
    log_verbose("Testing English Collation with Primary strength\n");
    for (i = 38; i < 43 ; i++)
    {
        doTest(myCollation, testSourceCases[i], testTargetCases[i], results[i]);
    }
    ucol_close(myCollation);
}

static void TestSecondary()
{
    UCollationResult expected=UCOL_EQUAL;
    int32_t i,j, testAcuteSize;
    UErrorCode status = U_ZERO_ERROR;
    myCollation = ucol_open("en_US", &status);
    if(U_FAILURE(status)){
        log_err_status(status, "ERROR: in creation of rule based collator: %s\n", myErrorName(status));
        return;
    }
    ucol_setStrength(myCollation, UCOL_SECONDARY);
    log_verbose("Testing English Collation with Secondary strength\n");
    for (i = 43; i < 49 ; i++)
    {
        doTest(myCollation, testSourceCases[i], testTargetCases[i], results[i]);
    }
    

    /*test acute and grave ordering (compare to french collation) */
    testAcuteSize = sizeof(testAcute) / sizeof(testAcute[0]);
    for (i = 0; i < testAcuteSize; i++)
    {
        for (j = 0; j < testAcuteSize; j++)
        {
            if (i <  j) expected = UCOL_LESS;
            if (i == j) expected = UCOL_EQUAL;
            if (i >  j) expected = UCOL_GREATER;
            doTest(myCollation, testAcute[i], testAcute[j], expected );
        }
    }
ucol_close(myCollation);
}

#endif /* #if !UCONFIG_NO_COLLATION */
