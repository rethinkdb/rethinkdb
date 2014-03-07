/********************************************************************
 * COPYRIGHT: 
 * Copyright (c) 1997-2010, International Business Machines Corporation and
 * others. All Rights Reserved.
 ********************************************************************/

#include "unicode/utypes.h"

#if !UCONFIG_NO_COLLATION

#include "unicode/coll.h"
#include "unicode/tblcoll.h"
#include "unicode/unistr.h"
#include "unicode/sortkey.h"
#include "g7coll.h"
#include "sfwdchit.h"


static const UChar testCases[][G7CollationTest::MAX_TOKEN_LEN] = {
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

static const int32_t results[G7CollationTest::TESTLOCALES][G7CollationTest::TOTALTESTSET] = {
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

G7CollationTest::~G7CollationTest() {}

void G7CollationTest::TestG7Locales(/* char* par */)
{
    int32_t i;
    const Locale locales[8] = {
        Locale("en", "US", ""),
        Locale("en", "GB", ""),
        Locale("en", "CA", ""),
        Locale("fr", "FR", ""),
        Locale("fr", "CA", ""),
        Locale("de", "DE", ""),
        Locale("it", "IT", ""),
        Locale("ja", "JP", "")
    };


    for (i = 0; i < 8; i++)
    {
        Collator *myCollation= 0;
        UnicodeString dispName;
        UErrorCode status = U_ZERO_ERROR;
        RuleBasedCollator* tblColl1 = 0;

        myCollation = Collator::createInstance(locales[i], status);
        if(U_FAILURE(status)) {
          delete myCollation;
          errcheckln(status, "Couldn't instantiate collator. Error: %s", u_errorName(status));
          return;
        }
        myCollation->setStrength(Collator::QUATERNARY);
        myCollation->setAttribute(UCOL_ALTERNATE_HANDLING, UCOL_SHIFTED, status);
        if (U_FAILURE(status))
        {
            UnicodeString msg;

            msg += "Locale ";
            msg += locales[i].getDisplayName(dispName);
            msg += "creation failed.";

            errln(msg);
            continue;
        }

//        const UnicodeString& defRules = ((RuleBasedCollator*)myCollation)->getRules();
        status = U_ZERO_ERROR;
        tblColl1 = new RuleBasedCollator(((RuleBasedCollator*)myCollation)->getRules(), status);
        if (U_FAILURE(status))
        {
            UnicodeString msg, name;

            msg += "Recreate ";
            msg += locales[i].getDisplayName(name);
            msg += "collation failed.";

            errln(msg);
            continue;
        }

        UnicodeString msg;

        msg += "Locale ";
        msg += locales[i].getDisplayName(dispName);
        msg += "tests start :";
        logln(msg);

        int32_t j, n;
        for (j = 0; j < FIXEDTESTSET; j++)
        {
            for (n = j+1; n < FIXEDTESTSET; n++)
            {
                doTest(tblColl1, testCases[results[i][j]], testCases[results[i][n]], Collator::LESS);
            }
        }

        delete myCollation;
        delete tblColl1;
    }
}

void G7CollationTest::TestDemo1(/* char* par */)
{
    logln("Demo Test 1 : Create a new table collation with rules \"& Z < p, P\"");
    UErrorCode status = U_ZERO_ERROR;
    Collator *col = Collator::createInstance("en_US", status);
    if(U_FAILURE(status)) {
      delete col;
      errcheckln(status, "Couldn't instantiate collator. Error: %s", u_errorName(status));
      return;
    }
    const UnicodeString baseRules = ((RuleBasedCollator*)col)->getRules();
    UnicodeString newRules(" & Z < p, P");
    newRules.insert(0, baseRules);
    RuleBasedCollator *myCollation = new RuleBasedCollator(newRules, status);

    if (U_FAILURE(status))
    {
        errln( "Demo Test 1 Table Collation object creation failed.");
        return;
    }

    int32_t j, n;
    for (j = 0; j < FIXEDTESTSET; j++)
    {
        for (n = j+1; n < FIXEDTESTSET; n++)
        {
            doTest(myCollation, testCases[results[8][j]], testCases[results[8][n]], Collator::LESS);
        }
    }

    delete myCollation; 
    delete col;
}

void G7CollationTest::TestDemo2(/* char* par */)
{
    logln("Demo Test 2 : Create a new table collation with rules \"& C < ch , cH, Ch, CH\"");
    UErrorCode status = U_ZERO_ERROR;
    Collator *col = Collator::createInstance("en_US", status);
    if(U_FAILURE(status)) {
      delete col;
      errcheckln(status, "Couldn't instantiate collator. Error: %s", u_errorName(status));
      return;
    }
    const UnicodeString baseRules = ((RuleBasedCollator*)col)->getRules();
    UnicodeString newRules("& C < ch , cH, Ch, CH");
    newRules.insert(0, baseRules);
    RuleBasedCollator *myCollation = new RuleBasedCollator(newRules, status);

    if (U_FAILURE(status))
    {
        errln("Demo Test 2 Table Collation object creation failed.");
        return;
    }

    int32_t j, n;
    for (j = 0; j < TOTALTESTSET; j++)
    {
        for (n = j+1; n < TOTALTESTSET; n++)
        {
            doTest(myCollation, testCases[results[9][j]], testCases[results[9][n]], Collator::LESS);
        }
    }
    
    delete myCollation; 
    delete col;
}

void G7CollationTest::TestDemo3(/* char* par */)
{
    logln("Demo Test 3 : Create a new table collation with rules \"& Question'-'mark ; '?' & Hash'-'mark ; '#' & Ampersand ; '&'\"");
    UErrorCode status = U_ZERO_ERROR;
    Collator *col = Collator::createInstance("en_US", status);
    if(U_FAILURE(status)) {
      errcheckln(status, "Couldn't instantiate collator. Error: %s", u_errorName(status));
      delete col;
      return;
    }
    const UnicodeString baseRules = ((RuleBasedCollator*)col)->getRules();
    UnicodeString newRules = "& Question'-'mark ; '?' & Hash'-'mark ; '#' & Ampersand ; '&'";
    newRules.insert(0, baseRules);
    RuleBasedCollator *myCollation = new RuleBasedCollator(newRules, status);

    if (U_FAILURE(status))
    {
        errln("Demo Test 3 Table Collation object creation failed.");
        return;
    }

    int32_t j, n;
    for (j = 0; j < TOTALTESTSET; j++)
    {
        for (n = j+1; n < TOTALTESTSET; n++)
        {
            doTest(myCollation, testCases[results[10][j]], testCases[results[10][n]], Collator::LESS);
        }
    }
    
    delete myCollation; 
    delete col;
}

void G7CollationTest::TestDemo4(/* char* par */)
{
    logln("Demo Test 4 : Create a new table collation with rules \" & aa ; a'-' & ee ; e'-' & ii ; i'-' & oo ; o'-' & uu ; u'-' \"");
    UErrorCode status = U_ZERO_ERROR;
    Collator *col = Collator::createInstance("en_US", status);
    if(U_FAILURE(status)) {
      delete col;
      errcheckln(status, "Couldn't instantiate collator. Error: %s", u_errorName(status));
      return;
    }

    const UnicodeString baseRules = ((RuleBasedCollator*)col)->getRules();
    UnicodeString newRules = " & aa ; a'-' & ee ; e'-' & ii ; i'-' & oo ; o'-' & uu ; u'-' ";
    newRules.insert(0, baseRules);
    RuleBasedCollator *myCollation = new RuleBasedCollator(newRules, status);

    int32_t j, n;
    for (j = 0; j < TOTALTESTSET; j++)
    {
        for (n = j+1; n < TOTALTESTSET; n++)
        {
            doTest(myCollation, testCases[results[11][j]], testCases[results[11][n]], Collator::LESS);
        }
    }

    delete myCollation; 
    delete col;
}

void G7CollationTest::runIndexedTest( int32_t index, UBool exec, const char* &name, char* /*par*/ )
{
    if (exec) logln("TestSuite G7CollationTest: ");
    switch (index) {
        case 0: name = "TestG7Locales"; if (exec)   TestG7Locales(/* par */); break;
        case 1: name = "TestDemo1"; if (exec)   TestDemo1(/* par */); break;
        case 2: name = "TestDemo2"; if (exec)   TestDemo2(/* par */); break;
        case 3: name = "TestDemo3"; if (exec)   TestDemo3(/* par */); break;
        case 4: name = "TestDemo4"; if (exec)   TestDemo4(/* par */); break;
        default: name = ""; break;
    }
}

#endif /* #if !UCONFIG_NO_COLLATION */
