/********************************************************************
 * COPYRIGHT: 
 * Copyright (c) 1997-2010, International Business Machines Corporation and
 * others. All Rights Reserved.
 ********************************************************************/
/********************************************************************************
*
* File CG7COLL.C
*
* Modification History:
*        Name                     Description            
*     Madhu Katragadda            Ported for C API
*********************************************************************************/
/**
 * G7CollationTest is a third level test class.  This test performs the examples 
 * mentioned on the IBM Java international demos web site.  
 * Sample Rules: & Z < p , P 
 * Effect :  Making P sort after Z.
 *
 * Sample Rules: & c < ch , cH, Ch, CH 
 * Effect : As well as adding sequences of characters that act as a single character (this is
 * known as contraction), you can also add characters that act like a sequence of
 * characters (this is known as expansion).  
 * 
 * Sample Rules: & Question'-'mark ; '?' & Hash'-'mark ; '#' & Ampersand ; '&' 
 * Effect : Expansion and contraction can actually be combined.  
 * 
 * Sample Rules: & aa ; a'-' & ee ; e'-' & ii ; i'-' & oo ; o'-' & uu ; u'-'
 * Effect : sorted sequence as the following,
 * aardvark  
 * a-rdvark  
 * abbot  
 * coop  
 * co-p  
 * cop 
 */
#include <stdlib.h>
#include <string.h>
#include <stdio.h>

#include "unicode/utypes.h"

#if !UCONFIG_NO_COLLATION

#include "unicode/ucol.h"
#include "unicode/uloc.h"
#include "cintltst.h"
#include "cg7coll.h"
#include "ccolltst.h"
#include "callcoll.h"
#include "unicode/ustring.h"


const char* locales[8] = {
        "en_US",
        "en_GB",
        "en_CA",
        "fr_FR",
        "fr_CA",
        "de_DE",
        "it_IT",
        "ja_JP"
};



const static UChar testCases[][MAX_TOKEN_LEN] = {
    {  0x0062 /*'b'*/, 0x006c /*'l'*/, 0x0061 /*'a'*/, 0x0062 /*'c'*/, 0x006b /*'k'*/, 
        0x0062 /*'b'*/, 0x0069 /*'i'*/, 0x0072 /*'r'*/, 0x0064 /*'d'*/, 0x0073 /*'s'*/, 0x0000},                    /* 9 */
    { 0x0050 /*'P'*/, 0x0061 /*'a'*/, 0x0074/*'t'*/, 0x0000},                                                    /* 1 */
    { 0x0070 /*'p'*/, 0x00E9, 0x0063 /*'c'*/, 0x0068 /*'h'*/, 0x00E9, 0x0000},                                    /* 2 */
    { 0x0070 /*'p'*/, 0x00EA, 0x0063 /*'c'*/, 0x0068 /*'h'*/, 0x0065 /*'e'*/, 0x0000},                           /* 3 */
    { 0x0070 /*'p'*/, 0x00E9, 0x0063 /*'c'*/, 0x0068 /*'h'*/, 0x0065 /*'e'*/, 0x0072 /*'r'*/, 0x0000},            /* 4 */
    { 0x0070 /*'p'*/, 0x00EA, 0x0063 /*'c'*/, 0x0068 /*'h'*/, 0x0065 /*'e'*/, 0x0072 /*'r'*/, 0x0000},            /* 5 */
    { 0x0054 /*'T'*/, 0x006f /*'o'*/, 0x0064 /*'d'*/, 0x0000},                                                    /* 6 */
    { 0x0054 /*'T'*/, 0x00F6, 0x006e /*'n'*/, 0x0065 /*'e'*/, 0x0000},                                            /* 7 */
    { 0x0054 /*'T'*/, 0x006f /*'o'*/, 0x0066 /*'f'*/, 0x0075 /*'u'*/, 0x0000},                                   /* 8 */
    { 0x0062 /*'b'*/, 0x006c /*'l'*/, 0x0061 /*'a'*/, 0x0062 /*'c'*/, 0x006b /*'k'*/, 
      0x0062  /*'b'*/, 0x0069 /*'i'*/, 0x0072 /*'r'*/, 0x0064 /*'d'*/, 0x0000},                                    /* 12 */
    { 0x0054 /*'T'*/, 0x006f /*'o'*/, 0x006e /*'n'*/, 0x0000},                                                    /* 10 */
    { 0x0050  /*'P'*/, 0x0041 /*'A'*/, 0x0054 /*'T'*/, 0x0000},                                                    /* 11 */
    { 0x0062 /*'b'*/, 0x006c /*'l'*/, 0x0061 /*'a'*/, 0x0062 /*'c'*/, 0x006b /*'k'*/, 
        0x002d /*'-'*/,  0x0062 /*'b'*/, 0x0069 /*'i'*/, 0x0072 /*'r'*/, 0x0064 /*'d'*/, 0x0000},                /* 13 */
    { 0x0062 /*'b'*/, 0x006c /*'l'*/, 0x0061 /*'a'*/, 0x0062 /*'c'*/, 0x006b /*'k'*/, 
        0x002d /*'-'*/,  0x0062 /*'b'*/, 0x0069 /*'i'*/, 0x0072 /*'r'*/, 0x0064 /*'d'*/, 0x0073/*'s'*/, 0x0000},  /* 0 */
    {0x0070 /*'p'*/, 0x0061 /*'a'*/, 0x0074 /*'t'*/, 0x0000},                                                    /* 14 */
    /* Additional tests */
    { 0x0063 /*'c'*/, 0x007a /*'z'*/, 0x0061 /*'a'*/, 0x0072 /*'r'*/, 0x0000 },                                 /* 15 */
    { 0x0063 /*'c'*/, 0x0068 /*'h'*/, 0x0075 /*'u'*/, 0x0072 /*'r'*/, 0x006f /*'o'*/, 0x0000 },                  /* 16 */
    { 0x0063 /*'c'*/, 0x0061 /*'a'*/, 0x0074 /*'t'*/, 0x000 },                                                    /* 17 */ 
    { 0x0064 /*'d'*/, 0x0061 /*'a'*/, 0x0072 /*'r'*/, 0x006e /*'n'*/, 0x0000 },                                 /* 18 */
    { 0x003f /*'?'*/, 0x0000 },                                                                                /* 19 */
    { 0x0071 /*'q'*/, 0x0075 /*'u'*/, 0x0069 /*'i'*/, 0x0063 /*'c'*/, 0x006b /*'k'*/, 0x0000 },                  /* 20 */
    { 0x0023 /*'#'*/, 0x0000 },                                                                                /* 21 */
    { 0x0026 /*'&'*/, 0x0000 },                                                                                /* 22 */
    {  0x0061 /*'a'*/, 0x002d /*'-'*/, 0x0072 /*'r'*/, 0x0064 /*'d'*/, 0x0076 /*'v'*/, 0x0061 /*'a'*/, 
                0x0072/*'r'*/, 0x006b/*'k'*/, 0x0000},                                                        /* 24 */
    { 0x0061 /*'a'*/, 0x0061 /*'a'*/, 0x0072 /*'r'*/, 0x0064 /*'d'*/, 0x0076 /*'v'*/, 0x0061 /*'a'*/, 
                0x0072/*'r'*/, 0x006b/*'k'*/, 0x0000},                                                        /* 23 */
    { 0x0061 /*'a'*/, 0x0062 /*'b'*/, 0x0062 /*'b'*/, 0x006f /*'o'*/, 0x0074 /*'t'*/, 0x0000},                   /* 25 */
    { 0x0063 /*'c'*/, 0x006f /*'o'*/, 0x002d /*'-'*/, 0x0070 /*'p'*/, 0x0000},                                 /* 27 */
    { 0x0063 /*'c'*/, 0x006f  /*'o'*/, 0x0070 /*'p'*/, 0x0000},                                                /* 28 */
    { 0x0063 /*'c'*/, 0x006f /*'o'*/, 0x006f /*'o'*/, 0x0070 /*'p'*/, 0x0000},                                 /* 26 */
    { 0x007a /*'z'*/, 0x0065  /*'e'*/, 0x0062 /*'b'*/, 0x0072 /*'r'*/, 0x0061 /*'a'*/, 0x0000}                    /* 29 */
};

const static int32_t results[TESTLOCALES][TOTALTESTSET] = {
    { 12, 13, 9, 0, 14, 1, 11, 2, 3, 4, 5, 6, 8, 10, 7, 31, 31, 31, 31, 31, 31, 31, 31, 31, 31, 31, 31, 31, 31, 31 }, /* en_US */
    { 12, 13, 9, 0, 14, 1, 11, 2, 3, 4, 5, 6, 8, 10, 7, 31, 31, 31, 31, 31, 31, 31, 31, 31, 31, 31, 31, 31, 31, 31 }, /* en_GB */
    { 12, 13, 9, 0, 14, 1, 11, 2, 3, 4, 5, 6, 8, 10, 7, 31, 31, 31, 31, 31, 31, 31, 31, 31, 31, 31, 31, 31, 31, 31 }, /* en_CA */
    { 12, 13, 9, 0, 14, 1, 11, 2, 3, 4, 5, 6, 8, 10, 7, 31, 31, 31, 31, 31, 31, 31, 31, 31, 31, 31, 31, 31, 31, 31 }, /* fr_FR */
    { 12, 13, 9, 0, 14, 1, 11, 3, 2, 4, 5, 6, 8, 10, 7, 31, 31, 31, 31, 31, 31, 31, 31, 31, 31, 31, 31, 31, 31, 31 }, /* fr_CA */
    { 12, 13, 9, 0, 14, 1, 11, 2, 3, 4, 5, 6, 8, 10, 7, 31, 31, 31, 31, 31, 31, 31, 31, 31, 31, 31, 31, 31, 31, 31 }, /* de_DE */
    { 12, 13, 9, 0, 14, 1, 11, 2, 3, 4, 5, 6, 8, 10, 7, 31, 31, 31, 31, 31, 31, 31, 31, 31, 31, 31, 31, 31, 31, 31 }, /* it_IT */
    { 12, 13, 9, 0, 14, 1, 11, 2, 3, 4, 5, 6, 8, 10, 7, 31, 31, 31, 31, 31, 31, 31, 31, 31, 31, 31, 31, 31, 31, 31 }, /* ja_JP */
    /* new table collation with rules "& Z < p, P"  loop to FIXEDTESTSET */
    { 12, 13, 9, 0, 6, 8, 10, 7, 14, 1, 11, 2, 3, 4, 5, 31, 31, 31, 31, 31, 31, 31, 31, 31, 31, 31, 31, 31, 31, 31 }, 
    /* new table collation with rules "& C < ch , cH, Ch, CH " loop to TOTALTESTSET */
    { 19, 22, 21, 23, 24, 25, 12, 13, 9, 0, 17, 26, 28, 27, 15, 16, 18, 14, 1, 11, 2, 3, 4, 5, 20, 6, 8, 10, 7, 29 },
    /* new table collation with rules "& Question-mark ; ? & Hash-mark ; # & Ampersand ; '&'  " loop to TOTALTESTSET */
    { 23, 24, 25, 22, 12, 13, 9, 0, 17, 16, 26, 28, 27, 15, 18, 21, 14, 1, 11, 2, 3, 4, 5, 19, 20, 6, 8, 10, 7, 29 },
    /* analogous to Japanese rules " & aa ; a- & ee ; e- & ii ; i- & oo ; o- & uu ; u- " */  /* loop to TOTALTESTSET */
    { 19, 22, 21, 24, 23, 25, 12, 13, 9, 0, 17, 16, 28, 26, 27, 15, 18, 14, 1, 11, 2, 3, 4, 5, 20, 6, 8, 10, 7, 29 }
};

void addRuleBasedCollTest(TestNode** root)
{
    addTest(root, &TestG7Locales, "tscoll/cg7coll/TestG7Locales");
    addTest(root, &TestDemo1, "tscoll/cg7coll/TestDemo1");
    addTest(root, &TestDemo2, "tscoll/cg7coll/TestDemo2");
    addTest(root, &TestDemo3, "tscoll/cg7coll/TestDemo3");
    addTest(root, &TestDemo4, "tscoll/cg7coll/TestDemo4");

    
}

static void TestG7Locales()
{
    UCollator *myCollation, *tblColl1;
    UErrorCode status = U_ZERO_ERROR;
    const UChar *defRules;
    int32_t i, rlen, j, n;
    log_verbose("Testing  ucol_openRules for all the locales\n");
    for (i = 0; i < 8; i++)
    {
        status = U_ZERO_ERROR;
        myCollation = ucol_open(locales[i], &status);
        ucol_setAttribute(myCollation, UCOL_STRENGTH, UCOL_QUATERNARY, &status);
        ucol_setAttribute(myCollation, UCOL_ALTERNATE_HANDLING, UCOL_SHIFTED, &status);

        if (U_FAILURE(status))
        {
            log_err_status(status, "Error in creating collator in %s:  %s\n", locales[i], myErrorName(status));
            continue;
        }

        defRules = ucol_getRules(myCollation, &rlen);
        status = U_ZERO_ERROR;
        tblColl1 = ucol_openRules(defRules, rlen, UCOL_OFF,
                   UCOL_DEFAULT_STRENGTH,NULL, &status);
        if (U_FAILURE(status))
        {
            ucol_close(myCollation);
            log_err_status(status, "Error in creating collator in %s:  %s\n", locales[i], myErrorName(status));
            continue;
        }

        
        log_verbose("Locale  %s\n", locales[i]);
        log_verbose("  tests start...\n");

        j = 0;
        n = 0;
        for (j = 0; j < FIXEDTESTSET; j++)
        {
            for (n = j+1; n < FIXEDTESTSET; n++)
            {
                doTest(tblColl1, testCases[results[i][j]], testCases[results[i][n]], UCOL_LESS);
            }
        }

        ucol_close(myCollation);
        ucol_close(tblColl1);
    }
}

static void TestDemo1()
{
    UCollator *myCollation;
    int32_t j, n;
    static const char rules[] = "& Z < p, P";
    int32_t len=(int32_t)strlen(rules);
    UChar temp[sizeof(rules)];
    UErrorCode status = U_ZERO_ERROR;
    u_uastrcpy(temp, rules);

    log_verbose("Demo Test 1 : Create a new table collation with rules \" & Z < p, P \" \n");

    myCollation = ucol_openRules(temp, len, UCOL_OFF, UCOL_DEFAULT_STRENGTH,NULL, &status);

    if (U_FAILURE(status))
    {
        log_err_status(status, "Demo Test 1 Rule collation object creation failed. : %s\n", myErrorName(status));
        return;
    }

    for (j = 0; j < FIXEDTESTSET; j++)
    {
        for (n = j+1; n < FIXEDTESTSET; n++)
        {
            doTest(myCollation, testCases[results[8][j]], testCases[results[8][n]], UCOL_LESS);
        }
    }

    ucol_close(myCollation); 
}

static void TestDemo2()
{
    UCollator *myCollation;
    int32_t j, n;
    static const char rules[] = "& C < ch , cH, Ch, CH";
    int32_t len=(int32_t)strlen(rules);
    UChar temp[sizeof(rules)];
    UErrorCode status = U_ZERO_ERROR;
    u_uastrcpy(temp, rules);

    log_verbose("Demo Test 2 : Create a new table collation with rules \"& C < ch , cH, Ch, CH\"");

    myCollation = ucol_openRules(temp, len, UCOL_OFF, UCOL_DEFAULT_STRENGTH, NULL, &status);

    if (U_FAILURE(status))
    {
        log_err_status(status, "Demo Test 2 Rule collation object creation failed.: %s\n", myErrorName(status));
        return;
    }
    for (j = 0; j < TOTALTESTSET; j++)
    {
        for (n = j+1; n < TOTALTESTSET; n++)
        {
            doTest(myCollation, testCases[results[9][j]], testCases[results[9][n]], UCOL_LESS);
        }
    }
    ucol_close(myCollation); 
}

static void TestDemo3()
{
    UCollator *myCollation;
    int32_t j, n;
    static const char rules[] = "& Question'-'mark ; '?' & Hash'-'mark ; '#' & Ampersand ; '&'";
    int32_t len=(int32_t)strlen(rules);
    UChar temp[sizeof(rules)];
    UErrorCode status = U_ZERO_ERROR;
    u_uastrcpy(temp, rules);

    log_verbose("Demo Test 3 : Create a new table collation with rules \"& Question'-'mark ; '?' & Hash'-'mark ; '#' & Ampersand ; '&'\" \n");

    myCollation = ucol_openRules(temp, len, UCOL_OFF, UCOL_DEFAULT_STRENGTH, NULL, &status);
    
    if (U_FAILURE(status))
    {
        log_err_status(status, "Demo Test 3 Rule collation object creation failed.: %s\n", myErrorName(status));
        return;
    }

    for (j = 0; j < TOTALTESTSET; j++)
    {
        for (n = j+1; n < TOTALTESTSET; n++)
        {
            doTest(myCollation, testCases[results[10][j]], testCases[results[10][n]], UCOL_LESS);
        }
    }
    ucol_close(myCollation); 
}

static void TestDemo4()
{
    UCollator *myCollation;
    int32_t j, n;
    static const char rules[] = " & aa ; a'-' & ee ; e'-' & ii ; i'-' & oo ; o'-' & uu ; u'-' ";
    int32_t len=(int32_t)strlen(rules);
    UChar temp[sizeof(rules)];
    UErrorCode status = U_ZERO_ERROR;
    u_uastrcpy(temp, rules);

    log_verbose("Demo Test 4 : Create a new table collation with rules \" & aa ; a'-' & ee ; e'-' & ii ; i'-' & oo ; o'-' & uu ; u'-' \"\n");

    myCollation = ucol_openRules(temp, len, UCOL_OFF, UCOL_DEFAULT_STRENGTH, NULL, &status);
    
    if (U_FAILURE(status))
    {
        log_err_status(status, "Demo Test 4 Rule collation object creation failed.: %s\n", myErrorName(status));
        return;
    }
    for (j = 0; j < TOTALTESTSET; j++)
    {
        for (n = j+1; n < TOTALTESTSET; n++)
        {
            doTest(myCollation, testCases[results[11][j]], testCases[results[11][n]], UCOL_LESS);
        }
    }
    ucol_close(myCollation); 
}

#endif /* #if !UCONFIG_NO_COLLATION */
