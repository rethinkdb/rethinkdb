/********************************************************************
 * COPYRIGHT: 
 * Copyright (c) 1997-2010, International Business Machines Corporation and
 * others. All Rights Reserved.
 ********************************************************************/
/********************************************************************************
*
* File CNORMTST.C
*
* Modification History:
*        Name                     Description            
*     Madhu Katragadda            Ported for C API
*     synwee                      added test for quick check
*     synwee                      added test for checkFCD
*********************************************************************************/
/*tests for u_normalization*/
#include "unicode/utypes.h"
#include "unicode/unorm.h"
#include "cintltst.h"

#if UCONFIG_NO_NORMALIZATION

void addNormTest(TestNode** root) {
    /* no normalization - nothing to do */
}

#else

#include <stdlib.h>
#include <time.h>
#include "unicode/uchar.h"
#include "unicode/ustring.h"
#include "unicode/unorm.h"
#include "cnormtst.h"

#define LENGTHOF(array) (int32_t)(sizeof(array)/sizeof ((array)[0]))

static void
TestAPI(void);

static void
TestNormCoverage(void);

static void
TestConcatenate(void);

static void
TestNextPrevious(void);

static void TestIsNormalized(void);

static void
TestFCNFKCClosure(void);

static void
TestQuickCheckPerCP(void);

static void
TestComposition(void);

static void
TestFCD(void);

static void
TestGetDecomposition(void);

static const char* const canonTests[][3] = {
    /* Input*/                    /*Decomposed*/                /*Composed*/
    { "cat",                    "cat",                        "cat"                    },
    { "\\u00e0ardvark",            "a\\u0300ardvark",            "\\u00e0ardvark",        },

    { "\\u1e0a",                "D\\u0307",                    "\\u1e0a"                }, /* D-dot_above*/
    { "D\\u0307",                "D\\u0307",                    "\\u1e0a"                }, /* D dot_above*/
    
    { "\\u1e0c\\u0307",            "D\\u0323\\u0307",            "\\u1e0c\\u0307"        }, /* D-dot_below dot_above*/
    { "\\u1e0a\\u0323",            "D\\u0323\\u0307",            "\\u1e0c\\u0307"        }, /* D-dot_above dot_below */
    { "D\\u0307\\u0323",        "D\\u0323\\u0307",            "\\u1e0c\\u0307"        }, /* D dot_below dot_above */
    
    { "\\u1e10\\u0307\\u0323",    "D\\u0327\\u0323\\u0307",    "\\u1e10\\u0323\\u0307"    }, /*D dot_below cedilla dot_above*/
    { "D\\u0307\\u0328\\u0323",    "D\\u0328\\u0323\\u0307",    "\\u1e0c\\u0328\\u0307"    }, /* D dot_above ogonek dot_below*/

    { "\\u1E14",                "E\\u0304\\u0300",            "\\u1E14"                }, /* E-macron-grave*/
    { "\\u0112\\u0300",            "E\\u0304\\u0300",            "\\u1E14"                }, /* E-macron + grave*/
    { "\\u00c8\\u0304",            "E\\u0300\\u0304",            "\\u00c8\\u0304"        }, /* E-grave + macron*/
    
    { "\\u212b",                "A\\u030a",                    "\\u00c5"                }, /* angstrom_sign*/
    { "\\u00c5",                "A\\u030a",                    "\\u00c5"                }, /* A-ring*/
    
    { "\\u00C4ffin",            "A\\u0308ffin",                "\\u00C4ffin"                    },
    { "\\u00C4\\uFB03n",        "A\\u0308\\uFB03n",            "\\u00C4\\uFB03n"                },

    { "Henry IV",                "Henry IV",                    "Henry IV"                },
    { "Henry \\u2163",            "Henry \\u2163",            "Henry \\u2163"            },

    { "\\u30AC",                "\\u30AB\\u3099",            "\\u30AC"                }, /* ga (Katakana)*/
    { "\\u30AB\\u3099",            "\\u30AB\\u3099",            "\\u30AC"                }, /*ka + ten*/
    { "\\uFF76\\uFF9E",            "\\uFF76\\uFF9E",            "\\uFF76\\uFF9E"        }, /* hw_ka + hw_ten*/
    { "\\u30AB\\uFF9E",            "\\u30AB\\uFF9E",            "\\u30AB\\uFF9E"        }, /* ka + hw_ten*/
    { "\\uFF76\\u3099",            "\\uFF76\\u3099",            "\\uFF76\\u3099"        },  /* hw_ka + ten*/
    { "A\\u0300\\u0316",           "A\\u0316\\u0300",           "\\u00C0\\u0316"        },  /* hw_ka + ten*/
    { "", "", "" }
};

static const char* const compatTests[][3] = {
    /* Input*/                        /*Decomposed    */                /*Composed*/
    { "cat",                        "cat",                            "cat"                },

    { "\\uFB4f",                    "\\u05D0\\u05DC",                "\\u05D0\\u05DC"    }, /* Alef-Lamed vs. Alef, Lamed*/

    { "\\u00C4ffin",                "A\\u0308ffin",                    "\\u00C4ffin"             },
    { "\\u00C4\\uFB03n",            "A\\u0308ffin",                    "\\u00C4ffin"                }, /* ffi ligature -> f + f + i*/

    { "Henry IV",                    "Henry IV",                        "Henry IV"            },
    { "Henry \\u2163",                "Henry IV",                        "Henry IV"            },

    { "\\u30AC",                    "\\u30AB\\u3099",                "\\u30AC"            }, /* ga (Katakana)*/
    { "\\u30AB\\u3099",                "\\u30AB\\u3099",                "\\u30AC"            }, /*ka + ten*/
    
    { "\\uFF76\\u3099",                "\\u30AB\\u3099",                "\\u30AC"            }, /* hw_ka + ten*/

    /*These two are broken in Unicode 2.1.2 but fixed in 2.1.5 and later*/
    { "\\uFF76\\uFF9E",                "\\u30AB\\u3099",                "\\u30AC"            }, /* hw_ka + hw_ten*/
    { "\\u30AB\\uFF9E",                "\\u30AB\\u3099",                "\\u30AC"            }, /* ka + hw_ten*/
    { "", "", "" }
};

static const char* const fcdTests[][3] = {
    /* Added for testing the below-U+0300 prefix of a NUL-terminated string. */
    { "\\u010e\\u0327", "D\\u0327\\u030c", NULL },  /* D-caron + cedilla */
    { "\\u010e", "\\u010e", NULL }  /* D-caron */
};

void addNormTest(TestNode** root);

void addNormTest(TestNode** root)
{
    addTest(root, &TestAPI, "tsnorm/cnormtst/TestAPI");
    addTest(root, &TestDecomp, "tsnorm/cnormtst/TestDecomp");
    addTest(root, &TestCompatDecomp, "tsnorm/cnormtst/TestCompatDecomp");
    addTest(root, &TestCanonDecompCompose, "tsnorm/cnormtst/TestCanonDecompCompose");
    addTest(root, &TestCompatDecompCompose, "tsnorm/cnormtst/TestCompatDecompCompose");
    addTest(root, &TestFCD, "tsnorm/cnormtst/TestFCD");
    addTest(root, &TestNull, "tsnorm/cnormtst/TestNull");
    addTest(root, &TestQuickCheck, "tsnorm/cnormtst/TestQuickCheck");
    addTest(root, &TestQuickCheckPerCP, "tsnorm/cnormtst/TestQuickCheckPerCP");
    addTest(root, &TestIsNormalized, "tsnorm/cnormtst/TestIsNormalized");
    addTest(root, &TestCheckFCD, "tsnorm/cnormtst/TestCheckFCD");
    addTest(root, &TestNormCoverage, "tsnorm/cnormtst/TestNormCoverage");
    addTest(root, &TestConcatenate, "tsnorm/cnormtst/TestConcatenate");
    addTest(root, &TestNextPrevious, "tsnorm/cnormtst/TestNextPrevious");
    addTest(root, &TestFCNFKCClosure, "tsnorm/cnormtst/TestFCNFKCClosure");
    addTest(root, &TestComposition, "tsnorm/cnormtst/TestComposition");
    addTest(root, &TestGetDecomposition, "tsnorm/cnormtst/TestGetDecomposition");
}

static const char* const modeStrings[]={
    "UNORM_NONE",
    "UNORM_NFD",
    "UNORM_NFKD",
    "UNORM_NFC",
    "UNORM_NFKC",
    "UNORM_FCD",
    "UNORM_MODE_COUNT"
};

static void TestNormCases(UNormalizationMode mode,
                          const char* const cases[][3], int32_t lengthOfCases) {
    int32_t x, neededLen, length2;
    int32_t expIndex= (mode==UNORM_NFC || mode==UNORM_NFKC) ? 2 : 1;
    UChar *source=NULL;
    UChar result[16];
    log_verbose("Testing unorm_normalize(%s)\n", modeStrings[mode]);
    for(x=0; x < lengthOfCases; x++)
    {
        UErrorCode status = U_ZERO_ERROR, status2 = U_ZERO_ERROR;
        source=CharsToUChars(cases[x][0]);
        neededLen= unorm_normalize(source, u_strlen(source), mode, 0, NULL, 0, &status);
        length2= unorm_normalize(source, -1, mode, 0, NULL, 0, &status2);
        if(neededLen!=length2) {
          log_err("ERROR in unorm_normalize(%s)[%d]: "
                  "preflight length/NUL %d!=%d preflight length/srcLength\n",
                  modeStrings[mode], (int)x, (int)neededLen, (int)length2);
        }
        if(status==U_BUFFER_OVERFLOW_ERROR)
        {
            status=U_ZERO_ERROR;
        }
        length2=unorm_normalize(source, u_strlen(source), mode, 0, result, LENGTHOF(result), &status); 
        if(U_FAILURE(status) || neededLen!=length2) {
            log_data_err("ERROR in unorm_normalize(%s/NUL) at %s:  %s - (Are you missing data?)\n",
                         modeStrings[mode], austrdup(source), myErrorName(status));
        } else {
            assertEqual(result, cases[x][expIndex], x);
        }
        length2=unorm_normalize(source, -1, mode, 0, result, LENGTHOF(result), &status); 
        if(U_FAILURE(status) || neededLen!=length2) {
            log_data_err("ERROR in unorm_normalize(%s/srcLength) at %s:  %s - (Are you missing data?)\n",
                         modeStrings[mode], austrdup(source), myErrorName(status));
        } else {
            assertEqual(result, cases[x][expIndex], x);
        }
        free(source);
    }
}

void TestDecomp() {
    TestNormCases(UNORM_NFD, canonTests, LENGTHOF(canonTests));
}

void TestCompatDecomp() {
    TestNormCases(UNORM_NFKD, compatTests, LENGTHOF(compatTests));
}

void TestCanonDecompCompose() {
    TestNormCases(UNORM_NFC, canonTests, LENGTHOF(canonTests));
}

void TestCompatDecompCompose() {
    TestNormCases(UNORM_NFKC, compatTests, LENGTHOF(compatTests));
}

void TestFCD() {
    TestNormCases(UNORM_FCD, fcdTests, LENGTHOF(fcdTests));
}

static void assertEqual(const UChar* result, const char* expected, int32_t index)
{
    UChar *expectedUni = CharsToUChars(expected);
    if(u_strcmp(result, expectedUni)!=0){
        log_err("ERROR in decomposition at index = %d. EXPECTED: %s , GOT: %s\n", index, expected,
            austrdup(result) );
    }
    free(expectedUni);
}

static void TestNull_check(UChar *src, int32_t srcLen, 
                    UChar *exp, int32_t expLen,
                    UNormalizationMode mode,
                    const char *name)
{
    UErrorCode status = U_ZERO_ERROR;
    int32_t len, i;

    UChar   result[50];


    status = U_ZERO_ERROR;

    for(i=0;i<50;i++)
      {
        result[i] = 0xFFFD;
      }

    len = unorm_normalize(src, srcLen, mode, 0, result, 50, &status); 

    if(U_FAILURE(status)) {
      log_data_err("unorm_normalize(%s) with 0x0000 failed: %s - (Are you missing data?)\n", name, u_errorName(status));
    } else if (len != expLen) {
      log_err("unorm_normalize(%s) with 0x0000 failed: Expected len %d, got %d\n", name, expLen, len);
    } 

    {
      for(i=0;i<len;i++){
        if(exp[i] != result[i]) {
          log_err("unorm_normalize(%s): @%d, expected \\u%04X got \\u%04X\n",
                  name,
                  i,
                  exp[i],
                  result[i]);
          return;
        }
        log_verbose("     %d: \\u%04X\n", i, result[i]);
      }
    }
    
    log_verbose("unorm_normalize(%s) with 0x0000: OK\n", name);
}

void TestNull() 
{

    UChar   source_comp[] = { 0x0061, 0x0000, 0x0044, 0x0307 };
    int32_t source_comp_len = 4;
    UChar   expect_comp[] = { 0x0061, 0x0000, 0x1e0a };
    int32_t expect_comp_len = 3;

    UChar   source_dcmp[] = { 0x1e0A, 0x0000, 0x0929 };
    int32_t source_dcmp_len = 3;
    UChar   expect_dcmp[] = { 0x0044, 0x0307, 0x0000, 0x0928, 0x093C };
    int32_t expect_dcmp_len = 5;
    
    TestNull_check(source_comp,
                   source_comp_len,
                   expect_comp,
                   expect_comp_len,
                   UNORM_NFC,
                   "UNORM_NFC");

    TestNull_check(source_dcmp,
                   source_dcmp_len,
                   expect_dcmp,
                   expect_dcmp_len,
                   UNORM_NFD,
                   "UNORM_NFD");

    TestNull_check(source_comp,
                   source_comp_len,
                   expect_comp,
                   expect_comp_len,
                   UNORM_NFKC,
                   "UNORM_NFKC");


}

static void TestQuickCheckResultNO() 
{
  const UChar CPNFD[] = {0x00C5, 0x0407, 0x1E00, 0x1F57, 0x220C, 
                         0x30AE, 0xAC00, 0xD7A3, 0xFB36, 0xFB4E};
  const UChar CPNFC[] = {0x0340, 0x0F93, 0x1F77, 0x1FBB, 0x1FEB, 
                          0x2000, 0x232A, 0xF900, 0xFA1E, 0xFB4E};
  const UChar CPNFKD[] = {0x00A0, 0x02E4, 0x1FDB, 0x24EA, 0x32FE, 
                           0xAC00, 0xFB4E, 0xFA10, 0xFF3F, 0xFA2D};
  const UChar CPNFKC[] = {0x00A0, 0x017F, 0x2000, 0x24EA, 0x32FE, 
                           0x33FE, 0xFB4E, 0xFA10, 0xFF3F, 0xFA2D};


  const int SIZE = 10;

  int count = 0;
  UErrorCode error = U_ZERO_ERROR;

  for (; count < SIZE; count ++)
  {
    if (unorm_quickCheck(&(CPNFD[count]), 1, UNORM_NFD, &error) != 
                                                              UNORM_NO)
    {
      log_err("ERROR in NFD quick check at U+%04x\n", CPNFD[count]);
      return;
    }
    if (unorm_quickCheck(&(CPNFC[count]), 1, UNORM_NFC, &error) != 
                                                              UNORM_NO)
    {
      log_err("ERROR in NFC quick check at U+%04x\n", CPNFC[count]);
      return;
    }
    if (unorm_quickCheck(&(CPNFKD[count]), 1, UNORM_NFKD, &error) != 
                                                              UNORM_NO)
    {
      log_err("ERROR in NFKD quick check at U+%04x\n", CPNFKD[count]);
      return;
    }
    if (unorm_quickCheck(&(CPNFKC[count]), 1, UNORM_NFKC, &error) != 
                                                              UNORM_NO)
    {
      log_err("ERROR in NFKC quick check at U+%04x\n", CPNFKC[count]);
      return;
    }
  }
}

 
static void TestQuickCheckResultYES() 
{
  const UChar CPNFD[] = {0x00C6, 0x017F, 0x0F74, 0x1000, 0x1E9A, 
                         0x2261, 0x3075, 0x4000, 0x5000, 0xF000};
  const UChar CPNFC[] = {0x0400, 0x0540, 0x0901, 0x1000, 0x1500, 
                         0x1E9A, 0x3000, 0x4000, 0x5000, 0xF000};
  const UChar CPNFKD[] = {0x00AB, 0x02A0, 0x1000, 0x1027, 0x2FFB, 
                          0x3FFF, 0x4FFF, 0xA000, 0xF000, 0xFA27};
  const UChar CPNFKC[] = {0x00B0, 0x0100, 0x0200, 0x0A02, 0x1000, 
                          0x2010, 0x3030, 0x4000, 0xA000, 0xFA0E};

  const int SIZE = 10;
  int count = 0;
  UErrorCode error = U_ZERO_ERROR;

  UChar cp = 0;
  while (cp < 0xA0)
  {
    if (unorm_quickCheck(&cp, 1, UNORM_NFD, &error) != UNORM_YES)
    {
      log_data_err("ERROR in NFD quick check at U+%04x - (Are you missing data?)\n", cp);
      return;
    }
    if (unorm_quickCheck(&cp, 1, UNORM_NFC, &error) != 
                                                             UNORM_YES)
    {
      log_err("ERROR in NFC quick check at U+%04x\n", cp);
      return;
    }
    if (unorm_quickCheck(&cp, 1, UNORM_NFKD, &error) != UNORM_YES)
    {
      log_err("ERROR in NFKD quick check at U+%04x\n", cp);
      return;
    }
    if (unorm_quickCheck(&cp, 1, UNORM_NFKC, &error) != 
                                                             UNORM_YES)
    {
      log_err("ERROR in NFKC quick check at U+%04x\n", cp);
      return;
    }
    cp ++;
  }

  for (; count < SIZE; count ++)
  {
    if (unorm_quickCheck(&(CPNFD[count]), 1, UNORM_NFD, &error) != 
                                                             UNORM_YES)
    {
      log_err("ERROR in NFD quick check at U+%04x\n", CPNFD[count]);
      return;
    }
    if (unorm_quickCheck(&(CPNFC[count]), 1, UNORM_NFC, &error) 
                                                          != UNORM_YES)
    {
      log_err("ERROR in NFC quick check at U+%04x\n", CPNFC[count]);
      return;
    }
    if (unorm_quickCheck(&(CPNFKD[count]), 1, UNORM_NFKD, &error) != 
                                                             UNORM_YES)
    {
      log_err("ERROR in NFKD quick check at U+%04x\n", CPNFKD[count]);
      return;
    }
    if (unorm_quickCheck(&(CPNFKC[count]), 1, UNORM_NFKC, &error) != 
                                                             UNORM_YES)
    {
      log_err("ERROR in NFKC quick check at U+%04x\n", CPNFKC[count]);
      return;
    }
  }
}

static void TestQuickCheckResultMAYBE() 
{
  const UChar CPNFC[] = {0x0306, 0x0654, 0x0BBE, 0x102E, 0x1161, 
                         0x116A, 0x1173, 0x1175, 0x3099, 0x309A};
  const UChar CPNFKC[] = {0x0300, 0x0654, 0x0655, 0x09D7, 0x0B3E, 
                          0x0DCF, 0xDDF, 0x102E, 0x11A8, 0x3099};


  const int SIZE = 10;

  int count = 0;
  UErrorCode error = U_ZERO_ERROR;

  /* NFD and NFKD does not have any MAYBE codepoints */
  for (; count < SIZE; count ++)
  {
    if (unorm_quickCheck(&(CPNFC[count]), 1, UNORM_NFC, &error) != 
                                                           UNORM_MAYBE)
    {
      log_data_err("ERROR in NFC quick check at U+%04x - (Are you missing data?)\n", CPNFC[count]);
      return;
    }
    if (unorm_quickCheck(&(CPNFKC[count]), 1, UNORM_NFKC, &error) != 
                                                           UNORM_MAYBE)
    {
      log_err("ERROR in NFKC quick check at U+%04x\n", CPNFKC[count]);
      return;
    }
  }
}

static void TestQuickCheckStringResult() 
{
  int count;
  UChar *d = NULL;
  UChar *c = NULL;
  UErrorCode error = U_ZERO_ERROR;

  for (count = 0; count < LENGTHOF(canonTests); count ++)
  {
    d = CharsToUChars(canonTests[count][1]);
    c = CharsToUChars(canonTests[count][2]);
    if (unorm_quickCheck(d, u_strlen(d), UNORM_NFD, &error) != 
                                                            UNORM_YES)
    {
      log_data_err("ERROR in NFD quick check for string at count %d - (Are you missing data?)\n", count);
      return;
    }

    if (unorm_quickCheck(c, u_strlen(c), UNORM_NFC, &error) == 
                                                            UNORM_NO)
    {
      log_err("ERROR in NFC quick check for string at count %d\n", count);
      return;
    }

    free(d);
    free(c);
  }

  for (count = 0; count < LENGTHOF(compatTests); count ++)
  {
    d = CharsToUChars(compatTests[count][1]);
    c = CharsToUChars(compatTests[count][2]);
    if (unorm_quickCheck(d, u_strlen(d), UNORM_NFKD, &error) != 
                                                            UNORM_YES)
    {
      log_err("ERROR in NFKD quick check for string at count %d\n", count);
      return;
    }

    if (unorm_quickCheck(c, u_strlen(c), UNORM_NFKC, &error) != 
                                                            UNORM_YES)
    {
      log_err("ERROR in NFKC quick check for string at count %d\n", count);
      return;
    }

    free(d);
    free(c);
  }  
}

void TestQuickCheck() 
{
  TestQuickCheckResultNO();
  TestQuickCheckResultYES();
  TestQuickCheckResultMAYBE();
  TestQuickCheckStringResult(); 
}

/*
 * The intltest/NormalizerConformanceTest tests a lot of strings that _are_
 * normalized, and some that are not.
 * Here we pick some specific cases and test the C API.
 */
static void TestIsNormalized(void) {
    static const UChar notNFC[][8]={            /* strings that are not in NFC */
        { 0x62, 0x61, 0x300, 0x63, 0 },         /* 0061 0300 compose */
        { 0xfb1d, 0 },                          /* excluded from composition */
        { 0x0627, 0x0653, 0 },                  /* 0627 0653 compose */
        { 0x3071, 0x306f, 0x309a, 0x3073, 0 }   /* 306F 309A compose */
    };
    static const UChar notNFKC[][8]={           /* strings that are not in NFKC */
        { 0x1100, 0x1161, 0 },                  /* Jamo compose */
        { 0x1100, 0x314f, 0 },                  /* compatibility Jamo compose */
        { 0x03b1, 0x1f00, 0x0345, 0x03b3, 0 }   /* 1F00 0345 compose */
    };

    int32_t i;
    UErrorCode errorCode;

    /* API test */

    /* normal case with length>=0 (length -1 used for special cases below) */
    errorCode=U_ZERO_ERROR;
    if(!unorm_isNormalized(notNFC[0]+2, 1, UNORM_NFC, &errorCode) || U_FAILURE(errorCode)) {
        log_data_err("error: !isNormalized(<U+0300>, NFC) (%s) - (Are you missing data?)\n", u_errorName(errorCode));
    }

    /* incoming U_FAILURE */
    errorCode=U_TRUNCATED_CHAR_FOUND;
    (void)unorm_isNormalized(notNFC[0]+2, 1, UNORM_NFC, &errorCode);
    if(errorCode!=U_TRUNCATED_CHAR_FOUND) {
        log_err("error: isNormalized(U_TRUNCATED_CHAR_FOUND) changed the error code to %s\n", u_errorName(errorCode));
    }

    /* NULL source */
    errorCode=U_ZERO_ERROR;
    (void)unorm_isNormalized(NULL, 1, UNORM_NFC, &errorCode);
    if(errorCode!=U_ILLEGAL_ARGUMENT_ERROR) {
        log_data_err("error: isNormalized(NULL) did not set U_ILLEGAL_ARGUMENT_ERROR but %s - (Are you missing data?)\n", u_errorName(errorCode));
    }

    /* bad length */
    errorCode=U_ZERO_ERROR;
    (void)unorm_isNormalized(notNFC[0]+2, -2, UNORM_NFC, &errorCode);
    if(errorCode!=U_ILLEGAL_ARGUMENT_ERROR) {
        log_data_err("error: isNormalized([-2]) did not set U_ILLEGAL_ARGUMENT_ERROR but %s - (Are you missing data?)\n", u_errorName(errorCode));
    }

    /* specific cases */
    for(i=0; i<LENGTHOF(notNFC); ++i) {
        errorCode=U_ZERO_ERROR;
        if(unorm_isNormalized(notNFC[i], -1, UNORM_NFC, &errorCode) || U_FAILURE(errorCode)) {
            log_data_err("error: isNormalized(notNFC[%d], NFC) is wrong (%s) - (Are you missing data?)\n", i, u_errorName(errorCode));
        }
        errorCode=U_ZERO_ERROR;
        if(unorm_isNormalized(notNFC[i], -1, UNORM_NFKC, &errorCode) || U_FAILURE(errorCode)) {
            log_data_err("error: isNormalized(notNFC[%d], NFKC) is wrong (%s) - (Are you missing data?)\n", i, u_errorName(errorCode));
        }
    }
    for(i=0; i<LENGTHOF(notNFKC); ++i) {
        errorCode=U_ZERO_ERROR;
        if(unorm_isNormalized(notNFKC[i], -1, UNORM_NFKC, &errorCode) || U_FAILURE(errorCode)) {
            log_data_err("error: isNormalized(notNFKC[%d], NFKC) is wrong (%s) - (Are you missing data?)\n", i, u_errorName(errorCode));
        }
    }
}

void TestCheckFCD() 
{
  UErrorCode status = U_ZERO_ERROR;
  static const UChar FAST_[] = {0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07, 0x08, 0x09, 
                         0x0A}; 
  static const UChar FALSE_[] = {0x0001, 0x0002, 0x02EA, 0x03EB, 0x0300, 0x0301, 
                          0x02B9, 0x0314, 0x0315, 0x0316};
  static const UChar TRUE_[] = {0x0030, 0x0040, 0x0440, 0x056D, 0x064F, 0x06E7, 
                         0x0050, 0x0730, 0x09EE, 0x1E10};

  static const UChar datastr[][5] = 
  { {0x0061, 0x030A, 0x1E05, 0x0302, 0},
    {0x0061, 0x030A, 0x00E2, 0x0323, 0},
    {0x0061, 0x0323, 0x00E2, 0x0323, 0},
    {0x0061, 0x0323, 0x1E05, 0x0302, 0} };
  static const UBool result[] = {UNORM_YES, UNORM_NO, UNORM_NO, UNORM_YES};

  static const UChar datachar[] = {0x60, 0x61, 0x62, 0x63, 0x64, 0x65, 0x66, 0x67, 0x68, 0x69, 
                            0x6a,
                            0xe0, 0xe1, 0xe2, 0xe3, 0xe4, 0xe5, 0xe6, 0xe7, 0xe8, 0xe9, 
                            0xea,
                            0x0300, 0x0301, 0x0302, 0x0303, 0x0304, 0x0305, 0x0306, 
                            0x0307, 0x0308, 0x0309, 0x030a, 
                            0x0320, 0x0321, 0x0322, 0x0323, 0x0324, 0x0325, 0x0326,
                            0x0327, 0x0328, 0x0329, 0x032a,
                            0x1e00, 0x1e01, 0x1e02, 0x1e03, 0x1e04, 0x1e05, 0x1e06, 
                            0x1e07, 0x1e08, 0x1e09, 0x1e0a};

  int count = 0;
  
  if (unorm_quickCheck(FAST_, 10, UNORM_FCD, &status) != UNORM_YES)
    log_data_err("unorm_quickCheck(FCD) failed: expected value for fast unorm_quickCheck is UNORM_YES - (Are you missing data?)\n");
  if (unorm_quickCheck(FALSE_, 10, UNORM_FCD, &status) != UNORM_NO)
    log_err("unorm_quickCheck(FCD) failed: expected value for error unorm_quickCheck is UNORM_NO\n");
  if (unorm_quickCheck(TRUE_, 10, UNORM_FCD, &status) != UNORM_YES)
    log_data_err("unorm_quickCheck(FCD) failed: expected value for correct unorm_quickCheck is UNORM_YES - (Are you missing data?)\n");

  if (U_FAILURE(status))
    log_data_err("unorm_quickCheck(FCD) failed: %s - (Are you missing data?)\n", u_errorName(status));

  while (count < 4)
  {
    UBool fcdresult = unorm_quickCheck(datastr[count], 4, UNORM_FCD, &status);
    if (U_FAILURE(status)) {
      log_data_err("unorm_quickCheck(FCD) failed: exception occured at data set %d - (Are you missing data?)\n", count);
      break;
    }
    else {
      if (result[count] != fcdresult) {
        log_err("unorm_quickCheck(FCD) failed: Data set %d expected value %d\n", count, 
                 result[count]);
      }
    }
    count ++;
  }

  /* random checks of long strings */
  status = U_ZERO_ERROR;
  srand((unsigned)time( NULL ));

  for (count = 0; count < 50; count ++)
  {
    int size = 0;
    UBool testresult = UNORM_YES;
    UChar data[20];
    UChar norm[100];
    UChar nfd[100];
    int normsize = 0;
    int nfdsize = 0;
    
    while (size != 19) {
      data[size] = datachar[(rand() * 50) / RAND_MAX];
      log_verbose("0x%x", data[size]);
      normsize += unorm_normalize(data + size, 1, UNORM_NFD, 0, 
                                  norm + normsize, 100 - normsize, &status);       
      if (U_FAILURE(status)) {
        log_data_err("unorm_quickCheck(FCD) failed: exception occured at data generation - (Are you missing data?)\n");
        break;
      }
      size ++;
    }
    log_verbose("\n");

    nfdsize = unorm_normalize(data, size, UNORM_NFD, 0, 
                              nfd, 100, &status);       
    if (U_FAILURE(status)) {
      log_data_err("unorm_quickCheck(FCD) failed: exception occured at normalized data generation - (Are you missing data?)\n");
    }

    if (nfdsize != normsize || u_memcmp(nfd, norm, nfdsize) != 0) {
      testresult = UNORM_NO;
    }
    if (testresult == UNORM_YES) {
      log_verbose("result UNORM_YES\n");
    }
    else {
      log_verbose("result UNORM_NO\n");
    }

    if (unorm_quickCheck(data, size, UNORM_FCD, &status) != testresult || U_FAILURE(status)) {
      log_data_err("unorm_quickCheck(FCD) failed: expected %d for random data - (Are you missing data?)\n", testresult);
    }
  }
}

static void
TestAPI() {
    static const UChar in[]={ 0x68, 0xe4 };
    UChar out[20]={ 0xffff, 0xffff, 0xffff, 0xffff };
    UErrorCode errorCode;
    int32_t length;

    /* try preflighting */
    errorCode=U_ZERO_ERROR;
    length=unorm_normalize(in, 2, UNORM_NFD, 0, NULL, 0, &errorCode);
    if(errorCode!=U_BUFFER_OVERFLOW_ERROR || length!=3) {
        log_data_err("unorm_normalize(pure preflighting NFD)=%ld failed with %s - (Are you missing data?)\n", length, u_errorName(errorCode));
        return;
    }

    errorCode=U_ZERO_ERROR;
    length=unorm_normalize(in, 2, UNORM_NFD, 0, out, 3, &errorCode);
    if(U_FAILURE(errorCode)) {
        log_err("unorm_normalize(NFD)=%ld failed with %s\n", length, u_errorName(errorCode));
        return;
    }
    if(length!=3 || out[2]!=0x308 || out[3]!=0xffff) {
        log_err("unorm_normalize(NFD ma<umlaut>)=%ld failed with out[]=U+%04x U+%04x U+%04x U+%04x\n", length, out[0], out[1], out[2], out[3]);
        return;
    }
    length=unorm_normalize(NULL, 0, UNORM_NFC, 0, NULL, 0, &errorCode);
    if(U_FAILURE(errorCode)) {
        log_err("unorm_normalize(src NULL[0], NFC, dest NULL[0])=%ld failed with %s\n", (long)length, u_errorName(errorCode));
        return;
    }
    length=unorm_normalize(NULL, 0, UNORM_NFC, 0, out, 20, &errorCode);
    if(U_FAILURE(errorCode)) {
        log_err("unorm_normalize(src NULL[0], NFC, dest out[20])=%ld failed with %s\n", (long)length, u_errorName(errorCode));
        return;
    }
}

/* test cases to improve test code coverage */
enum {
    HANGUL_K_KIYEOK=0x3131,         /* NFKD->Jamo L U+1100 */
    HANGUL_K_WEO=0x315d,            /* NFKD->Jamo V U+116f */
    HANGUL_K_KIYEOK_SIOS=0x3133,    /* NFKD->Jamo T U+11aa */

    HANGUL_KIYEOK=0x1100,           /* Jamo L U+1100 */
    HANGUL_WEO=0x116f,              /* Jamo V U+116f */
    HANGUL_KIYEOK_SIOS=0x11aa,      /* Jamo T U+11aa */

    HANGUL_AC00=0xac00,             /* Hangul syllable = Jamo LV U+ac00 */
    HANGUL_SYLLABLE=0xac00+14*28+3, /* Hangul syllable = U+1100 * U+116f * U+11aa */

    MUSICAL_VOID_NOTEHEAD=0x1d157,
    MUSICAL_HALF_NOTE=0x1d15e,  /* NFC/NFD->Notehead+Stem */
    MUSICAL_STEM=0x1d165,       /* cc=216 */
    MUSICAL_STACCATO=0x1d17c    /* cc=220 */
};

static void
TestNormCoverage() {
    UChar input[1000], expect[1000], output[1000];
    UErrorCode errorCode;
    int32_t i, length, inLength, expectLength, hangulPrefixLength, preflightLength;

    /* create a long and nasty string with NFKC-unsafe characters */
    inLength=0;

    /* 3 Jamos L/V/T, all 8 combinations normal/compatibility */
    input[inLength++]=HANGUL_KIYEOK;
    input[inLength++]=HANGUL_WEO;
    input[inLength++]=HANGUL_KIYEOK_SIOS;

    input[inLength++]=HANGUL_KIYEOK;
    input[inLength++]=HANGUL_WEO;
    input[inLength++]=HANGUL_K_KIYEOK_SIOS;

    input[inLength++]=HANGUL_KIYEOK;
    input[inLength++]=HANGUL_K_WEO;
    input[inLength++]=HANGUL_KIYEOK_SIOS;

    input[inLength++]=HANGUL_KIYEOK;
    input[inLength++]=HANGUL_K_WEO;
    input[inLength++]=HANGUL_K_KIYEOK_SIOS;

    input[inLength++]=HANGUL_K_KIYEOK;
    input[inLength++]=HANGUL_WEO;
    input[inLength++]=HANGUL_KIYEOK_SIOS;

    input[inLength++]=HANGUL_K_KIYEOK;
    input[inLength++]=HANGUL_WEO;
    input[inLength++]=HANGUL_K_KIYEOK_SIOS;

    input[inLength++]=HANGUL_K_KIYEOK;
    input[inLength++]=HANGUL_K_WEO;
    input[inLength++]=HANGUL_KIYEOK_SIOS;

    input[inLength++]=HANGUL_K_KIYEOK;
    input[inLength++]=HANGUL_K_WEO;
    input[inLength++]=HANGUL_K_KIYEOK_SIOS;

    /* Hangul LV with normal/compatibility Jamo T */
    input[inLength++]=HANGUL_AC00;
    input[inLength++]=HANGUL_KIYEOK_SIOS;

    input[inLength++]=HANGUL_AC00;
    input[inLength++]=HANGUL_K_KIYEOK_SIOS;

    /* compatibility Jamo L, V */
    input[inLength++]=HANGUL_K_KIYEOK;
    input[inLength++]=HANGUL_K_WEO;

    hangulPrefixLength=inLength;

    input[inLength++]=UTF16_LEAD(MUSICAL_HALF_NOTE);
    input[inLength++]=UTF16_TRAIL(MUSICAL_HALF_NOTE);
    for(i=0; i<200; ++i) {
        input[inLength++]=UTF16_LEAD(MUSICAL_STACCATO);
        input[inLength++]=UTF16_TRAIL(MUSICAL_STACCATO);
        input[inLength++]=UTF16_LEAD(MUSICAL_STEM);
        input[inLength++]=UTF16_TRAIL(MUSICAL_STEM);
    }

    /* (compatibility) Jamo L, T do not compose */
    input[inLength++]=HANGUL_K_KIYEOK;
    input[inLength++]=HANGUL_K_KIYEOK_SIOS;

    /* quick checks */
    errorCode=U_ZERO_ERROR;
    if(UNORM_NO!=unorm_quickCheck(input, inLength, UNORM_NFD, &errorCode) || U_FAILURE(errorCode)) {
        log_data_err("error unorm_quickCheck(long input, UNORM_NFD)!=NO (%s) - (Are you missing data?)\n", u_errorName(errorCode));
    }
    errorCode=U_ZERO_ERROR;
    if(UNORM_NO!=unorm_quickCheck(input, inLength, UNORM_NFKD, &errorCode) || U_FAILURE(errorCode)) {
        log_data_err("error unorm_quickCheck(long input, UNORM_NFKD)!=NO (%s) - (Are you missing data?)\n", u_errorName(errorCode));
    }
    errorCode=U_ZERO_ERROR;
    if(UNORM_NO!=unorm_quickCheck(input, inLength, UNORM_NFC, &errorCode) || U_FAILURE(errorCode)) {
        log_data_err("error unorm_quickCheck(long input, UNORM_NFC)!=NO (%s) - (Are you missing data?)\n", u_errorName(errorCode));
    }
    errorCode=U_ZERO_ERROR;
    if(UNORM_NO!=unorm_quickCheck(input, inLength, UNORM_NFKC, &errorCode) || U_FAILURE(errorCode)) {
        log_data_err("error unorm_quickCheck(long input, UNORM_NFKC)!=NO (%s) - (Are you missing data?)\n", u_errorName(errorCode));
    }
    errorCode=U_ZERO_ERROR;
    if(UNORM_NO!=unorm_quickCheck(input, inLength, UNORM_FCD, &errorCode) || U_FAILURE(errorCode)) {
        log_data_err("error unorm_quickCheck(long input, UNORM_FCD)!=NO (%s) - (Are you missing data?)\n", u_errorName(errorCode));
    }

    /* NFKC */
    expectLength=0;
    expect[expectLength++]=HANGUL_SYLLABLE;

    expect[expectLength++]=HANGUL_SYLLABLE;

    expect[expectLength++]=HANGUL_SYLLABLE;

    expect[expectLength++]=HANGUL_SYLLABLE;

    expect[expectLength++]=HANGUL_SYLLABLE;

    expect[expectLength++]=HANGUL_SYLLABLE;

    expect[expectLength++]=HANGUL_SYLLABLE;

    expect[expectLength++]=HANGUL_SYLLABLE;

    expect[expectLength++]=HANGUL_AC00+3;

    expect[expectLength++]=HANGUL_AC00+3;

    expect[expectLength++]=HANGUL_AC00+14*28;

    expect[expectLength++]=UTF16_LEAD(MUSICAL_VOID_NOTEHEAD);
    expect[expectLength++]=UTF16_TRAIL(MUSICAL_VOID_NOTEHEAD);
    expect[expectLength++]=UTF16_LEAD(MUSICAL_STEM);
    expect[expectLength++]=UTF16_TRAIL(MUSICAL_STEM);
    for(i=0; i<200; ++i) {
        expect[expectLength++]=UTF16_LEAD(MUSICAL_STEM);
        expect[expectLength++]=UTF16_TRAIL(MUSICAL_STEM);
    }
    for(i=0; i<200; ++i) {
        expect[expectLength++]=UTF16_LEAD(MUSICAL_STACCATO);
        expect[expectLength++]=UTF16_TRAIL(MUSICAL_STACCATO);
    }

    expect[expectLength++]=HANGUL_KIYEOK;
    expect[expectLength++]=HANGUL_KIYEOK_SIOS;

    /* try destination overflow first */
    errorCode=U_ZERO_ERROR;
    preflightLength=unorm_normalize(input, inLength,
                           UNORM_NFKC, 0,
                           output, 100, /* too short */
                           &errorCode);
    if(errorCode!=U_BUFFER_OVERFLOW_ERROR) {
        log_data_err("error unorm_normalize(long input, output too short, UNORM_NFKC) did not overflow but %s - (Are you missing data?)\n", u_errorName(errorCode));
    }

    /* real NFKC */
    errorCode=U_ZERO_ERROR;
    length=unorm_normalize(input, inLength,
                           UNORM_NFKC, 0,
                           output, sizeof(output)/U_SIZEOF_UCHAR,
                           &errorCode);
    if(U_FAILURE(errorCode)) {
        log_data_err("error unorm_normalize(long input, UNORM_NFKC) failed with %s - (Are you missing data?)\n", u_errorName(errorCode));
    } else if(length!=expectLength || u_memcmp(output, expect, length)!=0) {
        log_err("error unorm_normalize(long input, UNORM_NFKC) produced wrong result\n");
        for(i=0; i<length; ++i) {
            if(output[i]!=expect[i]) {
                log_err("    NFKC[%d]==U+%04lx expected U+%04lx\n", i, output[i], expect[i]);
                break;
            }
        }
    }
    if(length!=preflightLength) {
        log_err("error unorm_normalize(long input, UNORM_NFKC)==%ld but preflightLength==%ld\n", length, preflightLength);
    }

    /* FCD */
    u_memcpy(expect, input, hangulPrefixLength);
    expectLength=hangulPrefixLength;

    expect[expectLength++]=UTF16_LEAD(MUSICAL_VOID_NOTEHEAD);
    expect[expectLength++]=UTF16_TRAIL(MUSICAL_VOID_NOTEHEAD);
    expect[expectLength++]=UTF16_LEAD(MUSICAL_STEM);
    expect[expectLength++]=UTF16_TRAIL(MUSICAL_STEM);
    for(i=0; i<200; ++i) {
        expect[expectLength++]=UTF16_LEAD(MUSICAL_STEM);
        expect[expectLength++]=UTF16_TRAIL(MUSICAL_STEM);
    }
    for(i=0; i<200; ++i) {
        expect[expectLength++]=UTF16_LEAD(MUSICAL_STACCATO);
        expect[expectLength++]=UTF16_TRAIL(MUSICAL_STACCATO);
    }

    expect[expectLength++]=HANGUL_K_KIYEOK;
    expect[expectLength++]=HANGUL_K_KIYEOK_SIOS;

    errorCode=U_ZERO_ERROR;
    length=unorm_normalize(input, inLength,
                           UNORM_FCD, 0,
                           output, sizeof(output)/U_SIZEOF_UCHAR,
                           &errorCode);
    if(U_FAILURE(errorCode)) {
        log_data_err("error unorm_normalize(long input, UNORM_FCD) failed with %s - (Are you missing data?)\n", u_errorName(errorCode));
    } else if(length!=expectLength || u_memcmp(output, expect, length)!=0) {
        log_err("error unorm_normalize(long input, UNORM_FCD) produced wrong result\n");
        for(i=0; i<length; ++i) {
            if(output[i]!=expect[i]) {
                log_err("    FCD[%d]==U+%04lx expected U+%04lx\n", i, output[i], expect[i]);
                break;
            }
        }
    }
}

/* API test for unorm_concatenate() - for real test strings see intltest/tstnorm.cpp */
static void
TestConcatenate(void) {
    /* "re + 'sume'" */
    static const UChar
    left[]={
        0x72, 0x65, 0
    },
    right[]={
        0x301, 0x73, 0x75, 0x6d, 0xe9, 0
    },
    expect[]={
        0x72, 0xe9, 0x73, 0x75, 0x6d, 0xe9, 0
    };

    UChar buffer[100];
    UErrorCode errorCode;
    int32_t length;

    /* left with length, right NUL-terminated */
    errorCode=U_ZERO_ERROR;
    length=unorm_concatenate(left, 2, right, -1, buffer, 100, UNORM_NFC, 0, &errorCode);
    if(U_FAILURE(errorCode) || length!=6 || 0!=u_memcmp(buffer, expect, length)) {
        log_data_err("error: unorm_concatenate()=%ld (expect 6) failed with %s - (Are you missing data?)\n", length, u_errorName(errorCode));
    }

    /* preflighting */
    errorCode=U_ZERO_ERROR;
    length=unorm_concatenate(left, 2, right, -1, NULL, 0, UNORM_NFC, 0, &errorCode);
    if(errorCode!=U_BUFFER_OVERFLOW_ERROR || length!=6) {
        log_data_err("error: unorm_concatenate(preflighting)=%ld (expect 6) failed with %s - (Are you missing data?)\n", length, u_errorName(errorCode));
    }

    buffer[2]=0x5555;
    errorCode=U_ZERO_ERROR;
    length=unorm_concatenate(left, 2, right, -1, buffer, 1, UNORM_NFC, 0, &errorCode);
    if(errorCode!=U_BUFFER_OVERFLOW_ERROR || length!=6 || buffer[2]!=0x5555) {
        log_data_err("error: unorm_concatenate(preflighting 2)=%ld (expect 6) failed with %s - (Are you missing data?)\n", length, u_errorName(errorCode));
    }

    /* enter with U_FAILURE */
    buffer[2]=0xaaaa;
    errorCode=U_UNEXPECTED_TOKEN;
    length=unorm_concatenate(left, 2, right, -1, buffer, 100, UNORM_NFC, 0, &errorCode);
    if(errorCode!=U_UNEXPECTED_TOKEN || buffer[2]!=0xaaaa) {
        log_err("error: unorm_concatenate(failure)=%ld failed with %s\n", length, u_errorName(errorCode));
    }

    /* illegal arguments */
    buffer[2]=0xaaaa;
    errorCode=U_ZERO_ERROR;
    length=unorm_concatenate(NULL, 2, right, -1, buffer, 100, UNORM_NFC, 0, &errorCode);
    if(errorCode!=U_ILLEGAL_ARGUMENT_ERROR || buffer[2]!=0xaaaa) {
        log_data_err("error: unorm_concatenate(left=NULL)=%ld failed with %s - (Are you missing data?)\n", length, u_errorName(errorCode));
    }

    errorCode=U_ZERO_ERROR;
    length=unorm_concatenate(left, 2, right, -1, NULL, 100, UNORM_NFC, 0, &errorCode);
    if(errorCode!=U_ILLEGAL_ARGUMENT_ERROR) {
        log_data_err("error: unorm_concatenate(buffer=NULL)=%ld failed with %s - (Are you missing data?)\n", length, u_errorName(errorCode));
    }
}

enum {
    _PLUS=0x2b
};

static const char *const _modeString[UNORM_MODE_COUNT]={
    "0", "NONE", "NFD", "NFKD", "NFC", "NFKC", "FCD"
};

static void
_testIter(const UChar *src, int32_t srcLength,
          UCharIterator *iter, UNormalizationMode mode, UBool forward,
          const UChar *out, int32_t outLength,
          const int32_t *srcIndexes, int32_t srcIndexesLength) {
    UChar buffer[4];
    const UChar *expect, *outLimit, *in;
    int32_t length, i, expectLength, expectIndex, prevIndex, index, inLength;
    UErrorCode errorCode;
    UBool neededToNormalize, expectNeeded;

    errorCode=U_ZERO_ERROR;
    outLimit=out+outLength;
    if(forward) {
        expect=out;
        i=index=0;
    } else {
        expect=outLimit;
        i=srcIndexesLength-2;
        index=srcLength;
    }

    for(;;) {
        prevIndex=index;
        if(forward) {
            if(!iter->hasNext(iter)) {
                return;
            }
            length=unorm_next(iter,
                              buffer, sizeof(buffer)/U_SIZEOF_UCHAR,
                              mode, 0,
                              (UBool)(out!=NULL), &neededToNormalize,
                              &errorCode);
            expectIndex=srcIndexes[i+1];
            in=src+prevIndex;
            inLength=expectIndex-prevIndex;

            if(out!=NULL) {
                /* get output piece from between plus signs */
                expectLength=0;
                while((expect+expectLength)!=outLimit && expect[expectLength]!=_PLUS) {
                    ++expectLength;
                }
                expectNeeded=(UBool)(0!=u_memcmp(buffer, in, inLength));
            } else {
                expect=in;
                expectLength=inLength;
                expectNeeded=FALSE;
            }
        } else {
            if(!iter->hasPrevious(iter)) {
                return;
            }
            length=unorm_previous(iter,
                                  buffer, sizeof(buffer)/U_SIZEOF_UCHAR,
                                  mode, 0,
                                  (UBool)(out!=NULL), &neededToNormalize,
                                  &errorCode);
            expectIndex=srcIndexes[i];
            in=src+expectIndex;
            inLength=prevIndex-expectIndex;

            if(out!=NULL) {
                /* get output piece from between plus signs */
                expectLength=0;
                while(expect!=out && expect[-1]!=_PLUS) {
                    ++expectLength;
                    --expect;
                }
                expectNeeded=(UBool)(0!=u_memcmp(buffer, in, inLength));
            } else {
                expect=in;
                expectLength=inLength;
                expectNeeded=FALSE;
            }
        }
        index=iter->getIndex(iter, UITER_CURRENT);

        if(U_FAILURE(errorCode)) {
            log_data_err("error unorm iteration (next/previous %d %s)[%d]: %s - (Are you missing data?)\n",
                    forward, _modeString[mode], i, u_errorName(errorCode));
            return;
        }
        if(expectIndex!=index) {
            log_err("error unorm iteration (next/previous %d %s): index[%d] wrong, got %d expected %d\n",
                    forward, _modeString[mode], i, index, expectIndex);
            return;
        }
        if(expectLength!=length) {
            log_err("error unorm iteration (next/previous %d %s): length[%d] wrong, got %d expected %d\n",
                    forward, _modeString[mode], i, length, expectLength);
            return;
        }
        if(0!=u_memcmp(expect, buffer, length)) {
            log_err("error unorm iteration (next/previous %d %s): output string[%d] wrong\n",
                    forward, _modeString[mode], i);
            return;
        }
        if(neededToNormalize!=expectNeeded) {
        }

        if(forward) {
            expect+=expectLength+1; /* go after the + */
            ++i;
        } else {
            --expect; /* go before the + */
            --i;
        }
    }
}

static void
TestNextPrevious() {
    static const UChar
    src[]={ /* input string */
        0xa0, 0xe4, 0x63, 0x302, 0x327, 0xac00, 0x3133
    },
    nfd[]={ /* + separates expected output pieces */
        0xa0, _PLUS, 0x61, 0x308, _PLUS, 0x63, 0x327, 0x302, _PLUS, 0x1100, 0x1161, _PLUS, 0x3133
    },
    nfkd[]={
        0x20, _PLUS, 0x61, 0x308, _PLUS, 0x63, 0x327, 0x302, _PLUS, 0x1100, 0x1161, _PLUS, 0x11aa
    },
    nfc[]={
        0xa0, _PLUS, 0xe4, _PLUS, 0xe7, 0x302, _PLUS, 0xac00, _PLUS, 0x3133
    },
    nfkc[]={
        0x20, _PLUS, 0xe4, _PLUS, 0xe7, 0x302, _PLUS, 0xac03
    },
    fcd[]={
        0xa0, _PLUS, 0xe4, _PLUS, 0x63, 0x327, 0x302, _PLUS, 0xac00, _PLUS, 0x3133
    };

    /* expected iterator indexes in the source string for each iteration piece */
    static const int32_t
    nfdIndexes[]={
        0, 1, 2, 5, 6, 7
    },
    nfkdIndexes[]={
        0, 1, 2, 5, 6, 7
    },
    nfcIndexes[]={
        0, 1, 2, 5, 6, 7
    },
    nfkcIndexes[]={
        0, 1, 2, 5, 7
    },
    fcdIndexes[]={
        0, 1, 2, 5, 6, 7
    };

    UCharIterator iter;

    UChar buffer[4];
    int32_t length;

    UBool neededToNormalize;
    UErrorCode errorCode;

    uiter_setString(&iter, src, sizeof(src)/U_SIZEOF_UCHAR);

    /* test iteration with doNormalize */
    iter.index=0;
    _testIter(src, sizeof(src)/U_SIZEOF_UCHAR, &iter, UNORM_NFD, TRUE, nfd, sizeof(nfd)/U_SIZEOF_UCHAR, nfdIndexes, sizeof(nfdIndexes)/4);
    iter.index=0;
    _testIter(src, sizeof(src)/U_SIZEOF_UCHAR, &iter, UNORM_NFKD, TRUE, nfkd, sizeof(nfkd)/U_SIZEOF_UCHAR, nfkdIndexes, sizeof(nfkdIndexes)/4);
    iter.index=0;
    _testIter(src, sizeof(src)/U_SIZEOF_UCHAR, &iter, UNORM_NFC, TRUE, nfc, sizeof(nfc)/U_SIZEOF_UCHAR, nfcIndexes, sizeof(nfcIndexes)/4);
    iter.index=0;
    _testIter(src, sizeof(src)/U_SIZEOF_UCHAR, &iter, UNORM_NFKC, TRUE, nfkc, sizeof(nfkc)/U_SIZEOF_UCHAR, nfkcIndexes, sizeof(nfkcIndexes)/4);
    iter.index=0;
    _testIter(src, sizeof(src)/U_SIZEOF_UCHAR, &iter, UNORM_FCD, TRUE, fcd, sizeof(fcd)/U_SIZEOF_UCHAR, fcdIndexes, sizeof(fcdIndexes)/4);

    iter.index=iter.length;
    _testIter(src, sizeof(src)/U_SIZEOF_UCHAR, &iter, UNORM_NFD, FALSE, nfd, sizeof(nfd)/U_SIZEOF_UCHAR, nfdIndexes, sizeof(nfdIndexes)/4);
    iter.index=iter.length;
    _testIter(src, sizeof(src)/U_SIZEOF_UCHAR, &iter, UNORM_NFKD, FALSE, nfkd, sizeof(nfkd)/U_SIZEOF_UCHAR, nfkdIndexes, sizeof(nfkdIndexes)/4);
    iter.index=iter.length;
    _testIter(src, sizeof(src)/U_SIZEOF_UCHAR, &iter, UNORM_NFC, FALSE, nfc, sizeof(nfc)/U_SIZEOF_UCHAR, nfcIndexes, sizeof(nfcIndexes)/4);
    iter.index=iter.length;
    _testIter(src, sizeof(src)/U_SIZEOF_UCHAR, &iter, UNORM_NFKC, FALSE, nfkc, sizeof(nfkc)/U_SIZEOF_UCHAR, nfkcIndexes, sizeof(nfkcIndexes)/4);
    iter.index=iter.length;
    _testIter(src, sizeof(src)/U_SIZEOF_UCHAR, &iter, UNORM_FCD, FALSE, fcd, sizeof(fcd)/U_SIZEOF_UCHAR, fcdIndexes, sizeof(fcdIndexes)/4);

    /* test iteration without doNormalize */
    iter.index=0;
    _testIter(src, sizeof(src)/U_SIZEOF_UCHAR, &iter, UNORM_NFD, TRUE, NULL, 0, nfdIndexes, sizeof(nfdIndexes)/4);
    iter.index=0;
    _testIter(src, sizeof(src)/U_SIZEOF_UCHAR, &iter, UNORM_NFKD, TRUE, NULL, 0, nfkdIndexes, sizeof(nfkdIndexes)/4);
    iter.index=0;
    _testIter(src, sizeof(src)/U_SIZEOF_UCHAR, &iter, UNORM_NFC, TRUE, NULL, 0, nfcIndexes, sizeof(nfcIndexes)/4);
    iter.index=0;
    _testIter(src, sizeof(src)/U_SIZEOF_UCHAR, &iter, UNORM_NFKC, TRUE, NULL, 0, nfkcIndexes, sizeof(nfkcIndexes)/4);
    iter.index=0;
    _testIter(src, sizeof(src)/U_SIZEOF_UCHAR, &iter, UNORM_FCD, TRUE, NULL, 0, fcdIndexes, sizeof(fcdIndexes)/4);

    iter.index=iter.length;
    _testIter(src, sizeof(src)/U_SIZEOF_UCHAR, &iter, UNORM_NFD, FALSE, NULL, 0, nfdIndexes, sizeof(nfdIndexes)/4);
    iter.index=iter.length;
    _testIter(src, sizeof(src)/U_SIZEOF_UCHAR, &iter, UNORM_NFKD, FALSE, NULL, 0, nfkdIndexes, sizeof(nfkdIndexes)/4);
    iter.index=iter.length;
    _testIter(src, sizeof(src)/U_SIZEOF_UCHAR, &iter, UNORM_NFC, FALSE, NULL, 0, nfcIndexes, sizeof(nfcIndexes)/4);
    iter.index=iter.length;
    _testIter(src, sizeof(src)/U_SIZEOF_UCHAR, &iter, UNORM_NFKC, FALSE, NULL, 0, nfkcIndexes, sizeof(nfkcIndexes)/4);
    iter.index=iter.length;
    _testIter(src, sizeof(src)/U_SIZEOF_UCHAR, &iter, UNORM_FCD, FALSE, NULL, 0, fcdIndexes, sizeof(fcdIndexes)/4);

    /* try without neededToNormalize */
    errorCode=U_ZERO_ERROR;
    buffer[0]=5;
    iter.index=1;
    length=unorm_next(&iter, buffer, sizeof(buffer)/U_SIZEOF_UCHAR,
                      UNORM_NFD, 0, TRUE, NULL,
                      &errorCode);
    if(U_FAILURE(errorCode) || length!=2 || buffer[0]!=nfd[2] || buffer[1]!=nfd[3]) {
        log_data_err("error unorm_next(without needed) %s - (Are you missing data?)\n", u_errorName(errorCode));
        return;
    }

    /* preflight */
    neededToNormalize=9;
    iter.index=1;
    length=unorm_next(&iter, NULL, 0,
                      UNORM_NFD, 0, TRUE, &neededToNormalize,
                      &errorCode);
    if(errorCode!=U_BUFFER_OVERFLOW_ERROR || neededToNormalize!=FALSE || length!=2) {
        log_err("error unorm_next(pure preflighting) %s\n", u_errorName(errorCode));
        return;
    }

    errorCode=U_ZERO_ERROR;
    buffer[0]=buffer[1]=5;
    neededToNormalize=9;
    iter.index=1;
    length=unorm_next(&iter, buffer, 1,
                      UNORM_NFD, 0, TRUE, &neededToNormalize,
                      &errorCode);
    if(errorCode!=U_BUFFER_OVERFLOW_ERROR || neededToNormalize!=FALSE || length!=2 || buffer[1]!=5) {
        log_err("error unorm_next(preflighting) %s\n", u_errorName(errorCode));
        return;
    }

    /* no iterator */
    errorCode=U_ZERO_ERROR;
    buffer[0]=buffer[1]=5;
    neededToNormalize=9;
    iter.index=1;
    length=unorm_next(NULL, buffer, sizeof(buffer)/U_SIZEOF_UCHAR,
                      UNORM_NFD, 0, TRUE, &neededToNormalize,
                      &errorCode);
    if(errorCode!=U_ILLEGAL_ARGUMENT_ERROR) {
        log_err("error unorm_next(no iterator) %s\n", u_errorName(errorCode));
        return;
    }

    /* illegal mode */
    buffer[0]=buffer[1]=5;
    neededToNormalize=9;
    iter.index=1;
    length=unorm_next(&iter, buffer, sizeof(buffer)/U_SIZEOF_UCHAR,
                      (UNormalizationMode)0, 0, TRUE, &neededToNormalize,
                      &errorCode);
    if(errorCode!=U_ILLEGAL_ARGUMENT_ERROR) {
        log_err("error unorm_next(illegal mode) %s\n", u_errorName(errorCode));
        return;
    }

    /* error coming in */
    errorCode=U_MISPLACED_QUANTIFIER;
    buffer[0]=5;
    iter.index=1;
    length=unorm_next(&iter, buffer, sizeof(buffer)/U_SIZEOF_UCHAR,
                      UNORM_NFD, 0, TRUE, NULL,
                      &errorCode);
    if(errorCode!=U_MISPLACED_QUANTIFIER) {
        log_err("error unorm_next(U_MISPLACED_QUANTIFIER) %s\n", u_errorName(errorCode));
        return;
    }
}

static void
TestFCNFKCClosure(void) {
    static const struct {
        UChar32 c;
        const UChar s[6];
    } tests[]={
        { 0x00C4, { 0 } },
        { 0x00E4, { 0 } },
        { 0x037A, { 0x0020, 0x03B9, 0 } },
        { 0x03D2, { 0x03C5, 0 } },
        { 0x20A8, { 0x0072, 0x0073, 0 } },
        { 0x210B, { 0x0068, 0 } },
        { 0x210C, { 0x0068, 0 } },
        { 0x2121, { 0x0074, 0x0065, 0x006C, 0 } },
        { 0x2122, { 0x0074, 0x006D, 0 } },
        { 0x2128, { 0x007A, 0 } },
        { 0x1D5DB, { 0x0068, 0 } },
        { 0x1D5ED, { 0x007A, 0 } },
        { 0x0061, { 0 } }
    };

    UChar buffer[8];
    UErrorCode errorCode;
    int32_t i, length;

    for(i=0; i<LENGTHOF(tests); ++i) {
        errorCode=U_ZERO_ERROR;
        length=u_getFC_NFKC_Closure(tests[i].c, buffer, LENGTHOF(buffer), &errorCode);
        if(U_FAILURE(errorCode) || length!=u_strlen(buffer) || 0!=u_strcmp(tests[i].s, buffer)) {
            log_data_err("u_getFC_NFKC_Closure(U+%04lx) is wrong (%s) - (Are you missing data?)\n", tests[i].c, u_errorName(errorCode));
        }
    }

    /* error handling */
    errorCode=U_ZERO_ERROR;
    length=u_getFC_NFKC_Closure(0x5c, NULL, LENGTHOF(buffer), &errorCode);
    if(errorCode!=U_ILLEGAL_ARGUMENT_ERROR) {
        log_err("u_getFC_NFKC_Closure(dest=NULL) is wrong (%s)\n", u_errorName(errorCode));
    }

    length=u_getFC_NFKC_Closure(0x5c, buffer, LENGTHOF(buffer), &errorCode);
    if(errorCode!=U_ILLEGAL_ARGUMENT_ERROR) {
        log_err("u_getFC_NFKC_Closure(U_FAILURE) is wrong (%s)\n", u_errorName(errorCode));
    }
}

static void
TestQuickCheckPerCP() {
    UErrorCode errorCode;
    UChar32 c, lead, trail;
    UChar s[U16_MAX_LENGTH], nfd[16];
    int32_t length, lccc1, lccc2, tccc1, tccc2;
    int32_t qc1, qc2;

    if(
        u_getIntPropertyMaxValue(UCHAR_NFD_QUICK_CHECK)!=(int32_t)UNORM_YES ||
        u_getIntPropertyMaxValue(UCHAR_NFKD_QUICK_CHECK)!=(int32_t)UNORM_YES ||
        u_getIntPropertyMaxValue(UCHAR_NFC_QUICK_CHECK)!=(int32_t)UNORM_MAYBE ||
        u_getIntPropertyMaxValue(UCHAR_NFKC_QUICK_CHECK)!=(int32_t)UNORM_MAYBE ||
        u_getIntPropertyMaxValue(UCHAR_LEAD_CANONICAL_COMBINING_CLASS)!=u_getIntPropertyMaxValue(UCHAR_CANONICAL_COMBINING_CLASS) ||
        u_getIntPropertyMaxValue(UCHAR_TRAIL_CANONICAL_COMBINING_CLASS)!=u_getIntPropertyMaxValue(UCHAR_CANONICAL_COMBINING_CLASS)
    ) {
        log_err("wrong result from one of the u_getIntPropertyMaxValue(UCHAR_NF*_QUICK_CHECK) or UCHAR_*_CANONICAL_COMBINING_CLASS\n");
    }

    /*
     * compare the quick check property values for some code points
     * to the quick check results for checking same-code point strings
     */
    errorCode=U_ZERO_ERROR;
    c=0;
    while(c<0x110000) {
        length=0;
        U16_APPEND_UNSAFE(s, length, c);

        qc1=u_getIntPropertyValue(c, UCHAR_NFC_QUICK_CHECK);
        qc2=unorm_quickCheck(s, length, UNORM_NFC, &errorCode);
        if(qc1!=qc2) {
            log_data_err("u_getIntPropertyValue(NFC)=%d != %d=unorm_quickCheck(NFC) for U+%04x - (Are you missing data?)\n", qc1, qc2, c);
        }

        qc1=u_getIntPropertyValue(c, UCHAR_NFD_QUICK_CHECK);
        qc2=unorm_quickCheck(s, length, UNORM_NFD, &errorCode);
        if(qc1!=qc2) {
            log_data_err("u_getIntPropertyValue(NFD)=%d != %d=unorm_quickCheck(NFD) for U+%04x - (Are you missing data?)\n", qc1, qc2, c);
        }

        qc1=u_getIntPropertyValue(c, UCHAR_NFKC_QUICK_CHECK);
        qc2=unorm_quickCheck(s, length, UNORM_NFKC, &errorCode);
        if(qc1!=qc2) {
            log_data_err("u_getIntPropertyValue(NFKC)=%d != %d=unorm_quickCheck(NFKC) for U+%04x - (Are you missing data?)\n", qc1, qc2, c);
        }

        qc1=u_getIntPropertyValue(c, UCHAR_NFKD_QUICK_CHECK);
        qc2=unorm_quickCheck(s, length, UNORM_NFKD, &errorCode);
        if(qc1!=qc2) {
            log_data_err("u_getIntPropertyValue(NFKD)=%d != %d=unorm_quickCheck(NFKD) for U+%04x - (Are you missing data?)\n", qc1, qc2, c);
        }

        length=unorm_normalize(s, length, UNORM_NFD, 0, nfd, LENGTHOF(nfd), &errorCode);
        /* length-length == 0 is used to get around a compiler warning. */
        U16_GET(nfd, 0, length-length, length, lead);
        U16_GET(nfd, 0, length-1, length, trail);

        lccc1=u_getIntPropertyValue(c, UCHAR_LEAD_CANONICAL_COMBINING_CLASS);
        lccc2=u_getCombiningClass(lead);
        tccc1=u_getIntPropertyValue(c, UCHAR_TRAIL_CANONICAL_COMBINING_CLASS);
        tccc2=u_getCombiningClass(trail);

        if(lccc1!=lccc2) {
            log_err("u_getIntPropertyValue(lccc)=%d != %d=u_getCombiningClass(lead) for U+%04x\n",
                    lccc1, lccc2, c);
        }
        if(tccc1!=tccc2) {
            log_err("u_getIntPropertyValue(tccc)=%d != %d=u_getCombiningClass(trail) for U+%04x\n",
                    tccc1, tccc2, c);
        }

        /* skip some code points */
        c=(20*c)/19+1;
    }
}

static void
TestComposition(void) {
    static const struct {
        UNormalizationMode mode;
        uint32_t options;
        UChar input[12];
        UChar expect[12];
    } cases[]={
        /*
         * special cases for UAX #15 bug
         * see Unicode Corrigendum #5: Normalization Idempotency
         * at http://unicode.org/versions/corrigendum5.html
         * (was Public Review Issue #29)
         */
        { UNORM_NFC, 0, { 0x1100, 0x0300, 0x1161, 0x0327 },         { 0x1100, 0x0300, 0x1161, 0x0327 } },
        { UNORM_NFC, 0, { 0x1100, 0x0300, 0x1161, 0x0327, 0x11a8 }, { 0x1100, 0x0300, 0x1161, 0x0327, 0x11a8 } },
        { UNORM_NFC, 0, { 0xac00, 0x0300, 0x0327, 0x11a8 },         { 0xac00, 0x0327, 0x0300, 0x11a8 } },
        { UNORM_NFC, 0, { 0x0b47, 0x0300, 0x0b3e },                 { 0x0b47, 0x0300, 0x0b3e } },

        /* TODO: add test cases for UNORM_FCC here (j2151) */
    };

    UChar output[16];
    UErrorCode errorCode;
    int32_t i, length;

    for(i=0; i<LENGTHOF(cases); ++i) {
        errorCode=U_ZERO_ERROR;
        length=unorm_normalize(
                    cases[i].input, -1,
                    cases[i].mode, cases[i].options,
                    output, LENGTHOF(output),
                    &errorCode);
        if( U_FAILURE(errorCode) ||
            length!=u_strlen(cases[i].expect) ||
            0!=u_memcmp(output, cases[i].expect, length)
        ) {
            log_data_err("unexpected result for case %d - (Are you missing data?)\n", i);
        }
    }
}

static void
TestGetDecomposition() {
    UChar decomp[32];
    int32_t length;

    UErrorCode errorCode=U_ZERO_ERROR;
    const UNormalizer2 *n2=unorm2_getInstance(NULL, "nfc", UNORM2_COMPOSE_CONTIGUOUS, &errorCode);
    if(U_FAILURE(errorCode)) {
        log_err_status(errorCode, "unorm2_getInstance(nfc/FCC) failed: %s\n", u_errorName(errorCode));
        return;
    }

    length=unorm2_getDecomposition(n2, 0x20, decomp, LENGTHOF(decomp), &errorCode);
    if(U_FAILURE(errorCode) || length>=0) {
        log_err("unorm2_getDecomposition(space) failed\n");
    }
    errorCode=U_ZERO_ERROR;
    length=unorm2_getDecomposition(n2, 0xe4, decomp, LENGTHOF(decomp), &errorCode);
    if(U_FAILURE(errorCode) || length!=2 || decomp[0]!=0x61 || decomp[1]!=0x308 || decomp[2]!=0) {
        log_err("unorm2_getDecomposition(a-umlaut) failed\n");
    }
    errorCode=U_ZERO_ERROR;
    length=unorm2_getDecomposition(n2, 0xac01, decomp, LENGTHOF(decomp), &errorCode);
    if(U_FAILURE(errorCode) || length!=3 || decomp[0]!=0x1100 || decomp[1]!=0x1161 || decomp[2]!=0x11a8 || decomp[3]!=0) {
        log_err("unorm2_getDecomposition(Hangul syllable U+AC01) failed\n");
    }
    errorCode=U_ZERO_ERROR;
    length=unorm2_getDecomposition(n2, 0xac01, NULL, 0, &errorCode);
    if(errorCode!=U_BUFFER_OVERFLOW_ERROR || length!=3) {
        log_err("unorm2_getDecomposition(Hangul syllable U+AC01) overflow failed\n");
    }
    errorCode=U_ZERO_ERROR;
    length=unorm2_getDecomposition(n2, 0xac01, decomp, -1, &errorCode);
    if(errorCode!=U_ILLEGAL_ARGUMENT_ERROR) {
        log_err("unorm2_getDecomposition(capacity<0) failed\n");
    }
    errorCode=U_ZERO_ERROR;
    length=unorm2_getDecomposition(n2, 0xac01, NULL, 4, &errorCode);
    if(errorCode!=U_ILLEGAL_ARGUMENT_ERROR) {
        log_err("unorm2_getDecomposition(decomposition=NULL) failed\n");
    }
}

#endif /* #if !UCONFIG_NO_NORMALIZATION */
