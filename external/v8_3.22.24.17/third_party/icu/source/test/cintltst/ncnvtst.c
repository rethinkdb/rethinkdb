/********************************************************************
 * COPYRIGHT: 
 * Copyright (c) 1997-2010, International Business Machines Corporation and
 * others. All Rights Reserved.
 ********************************************************************/
/*****************************************************************************
*
* File CCONVTST.C
*
* Modification History:
*        Name                     Description
*   Madhu Katragadda              7/7/2000        Converter Tests for extended code coverage
******************************************************************************
*/
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "unicode/uloc.h"
#include "unicode/ucnv.h"
#include "unicode/utypes.h"
#include "unicode/ustring.h"
#include "unicode/uset.h"
#include "cintltst.h"

#define MAX_LENGTH 999

#define UNICODE_LIMIT 0x10FFFF
#define SURROGATE_HIGH_START    0xD800
#define SURROGATE_LOW_END       0xDFFF

static int32_t  gInBufferSize = 0;
static int32_t  gOutBufferSize = 0;
static char     gNuConvTestName[1024];

#define nct_min(x,y)  ((x<y) ? x : y)
#define LENGTHOF(array) (int32_t)(sizeof(array)/sizeof((array)[0]))

static void printSeq(const unsigned char* a, int len);
static void printSeqErr(const unsigned char* a, int len);
static void printUSeq(const UChar* a, int len);
static void printUSeqErr(const UChar* a, int len);
static UBool convertFromU( const UChar *source, int sourceLen,  const uint8_t *expect, int expectLen, 
                const char *codepage, const int32_t *expectOffsets, UBool doFlush, UErrorCode expectedStatus);
static UBool convertToU( const uint8_t *source, int sourceLen, const UChar *expect, int expectLen, 
               const char *codepage, const int32_t *expectOffsets, UBool doFlush, UErrorCode expectedStatus);

static UBool testConvertFromU( const UChar *source, int sourceLen,  const uint8_t *expect, int expectLen, 
                const char *codepage, UConverterFromUCallback callback, const int32_t *expectOffsets, UBool testReset);
static UBool testConvertToU( const uint8_t *source, int sourcelen, const UChar *expect, int expectlen, 
               const char *codepage, UConverterToUCallback callback, const int32_t *expectOffsets, UBool testReset);

static void setNuConvTestName(const char *codepage, const char *direction)
{
    sprintf(gNuConvTestName, "[Testing %s %s Unicode, InputBufSiz=%d, OutputBufSiz=%d]",
        codepage,
        direction,
        (int)gInBufferSize,
        (int)gOutBufferSize);
}


static void TestSurrogateBehaviour(void);
static void TestErrorBehaviour(void);

#if !UCONFIG_NO_LEGACY_CONVERSION
static void TestToUnicodeErrorBehaviour(void);
static void TestGetNextErrorBehaviour(void);
#endif

static void TestRegressionUTF8(void);
static void TestRegressionUTF32(void);
static void TestAvailableConverters(void);
static void TestFlushInternalBuffer(void);  /*for improved code coverage in ucnv_cnv.c*/
static void TestResetBehaviour(void);
static void TestTruncated(void);
static void TestUnicodeSet(void);

static void TestWithBufferSize(int32_t osize, int32_t isize);


static void printSeq(const unsigned char* a, int len)
{
    int i=0;
    log_verbose("\n{");
    while (i<len)
        log_verbose("0x%02X ", a[i++]);
    log_verbose("}\n");
}

static void printUSeq(const UChar* a, int len)
{
    int i=0;
    log_verbose("\n{");
    while (i<len)
        log_verbose("%0x04X ", a[i++]);
    log_verbose("}\n");
}

static void printSeqErr(const unsigned char* a, int len)
{
    int i=0;
    fprintf(stderr, "\n{");
    while (i<len)  fprintf(stderr, "0x%02X ", a[i++]);
    fprintf(stderr, "}\n");
}

static void printUSeqErr(const UChar* a, int len)
{
    int i=0;
    fprintf(stderr, "\n{");
    while (i<len)
        fprintf(stderr, "0x%04X ", a[i++]);
    fprintf(stderr,"}\n");
}

void addExtraTests(TestNode** root);

void addExtraTests(TestNode** root)
{
     addTest(root, &TestSurrogateBehaviour,         "tsconv/ncnvtst/TestSurrogateBehaviour");
     addTest(root, &TestErrorBehaviour,             "tsconv/ncnvtst/TestErrorBehaviour");

#if !UCONFIG_NO_LEGACY_CONVERSION
     addTest(root, &TestToUnicodeErrorBehaviour,    "tsconv/ncnvtst/ToUnicodeErrorBehaviour");
     addTest(root, &TestGetNextErrorBehaviour,      "tsconv/ncnvtst/TestGetNextErrorBehaviour");
#endif

     addTest(root, &TestAvailableConverters,        "tsconv/ncnvtst/TestAvailableConverters");
     addTest(root, &TestFlushInternalBuffer,        "tsconv/ncnvtst/TestFlushInternalBuffer");
     addTest(root, &TestResetBehaviour,             "tsconv/ncnvtst/TestResetBehaviour");
     addTest(root, &TestRegressionUTF8,             "tsconv/ncnvtst/TestRegressionUTF8");
     addTest(root, &TestRegressionUTF32,            "tsconv/ncnvtst/TestRegressionUTF32");
     addTest(root, &TestTruncated,                  "tsconv/ncnvtst/TestTruncated");
     addTest(root, &TestUnicodeSet,                 "tsconv/ncnvtst/TestUnicodeSet");
}

/*test surrogate behaviour*/
static void TestSurrogateBehaviour(){
    log_verbose("Testing for SBCS and LATIN_1\n");
    {
        UChar sampleText[] = {0x0031, 0xd801, 0xdc01, 0x0032};
        const uint8_t expected[] = {0x31, 0x1a, 0x32};

#if !UCONFIG_NO_LEGACY_CONVERSION
        /*SBCS*/
        if(!convertFromU(sampleText, sizeof(sampleText)/sizeof(sampleText[0]),
                expected, sizeof(expected), "ibm-920", 0 , TRUE, U_ZERO_ERROR))
            log_err("u-> ibm-920 [UCNV_SBCS] not match.\n");
#endif

        /*LATIN_1*/
        if(!convertFromU(sampleText, sizeof(sampleText)/sizeof(sampleText[0]),
                expected, sizeof(expected), "LATIN_1", 0, TRUE, U_ZERO_ERROR ))
            log_err("u-> LATIN_1 not match.\n");

    }

#if !UCONFIG_NO_LEGACY_CONVERSION
    log_verbose("Testing for DBCS and MBCS\n");
    {
        UChar sampleText[]       = {0x00a1, 0xd801, 0xdc01, 0x00a4};
        const uint8_t expected[] = {0xa2, 0xae, 0xa1, 0xe0, 0xa2, 0xb4};
        int32_t offsets[]        = {0x00, 0x00, 0x01, 0x01, 0x03, 0x03 };

        /*DBCS*/
        if(!convertFromU(sampleText, sizeof(sampleText)/sizeof(sampleText[0]),
                expected, sizeof(expected), "ibm-1363", 0 , TRUE, U_ZERO_ERROR))
            log_err("u-> ibm-1363 [UCNV_DBCS portion] not match.\n");
        if(!convertFromU(sampleText, sizeof(sampleText)/sizeof(sampleText[0]),
                expected, sizeof(expected), "ibm-1363", offsets , TRUE, U_ZERO_ERROR))
            log_err("u-> ibm-1363 [UCNV_DBCS portion] not match.\n");
        /*MBCS*/
        if(!convertFromU(sampleText, sizeof(sampleText)/sizeof(sampleText[0]),
                expected, sizeof(expected), "ibm-1363", 0 , TRUE, U_ZERO_ERROR))
            log_err("u-> ibm-1363 [UCNV_MBCS] not match.\n");
        if(!convertFromU(sampleText, sizeof(sampleText)/sizeof(sampleText[0]),
                expected, sizeof(expected), "ibm-1363", offsets, TRUE, U_ZERO_ERROR))
            log_err("u-> ibm-1363 [UCNV_MBCS] not match.\n");
    }

    log_verbose("Testing for ISO-2022-jp\n");
    {
        UChar    sampleText[] =   { 0x4e00, 0x04e01, 0x0031, 0xd801, 0xdc01, 0x0032};

        const uint8_t expected[] = {0x1b, 0x24, 0x42,0x30,0x6c,0x43,0x7a,0x1b,0x28,0x42,
                                    0x31,0x1A, 0x32};


        int32_t offsets[] = {0,0,0,0,0,1,1,2,2,2,2,3,5 };

        /*iso-2022-jp*/
        if(!convertFromU(sampleText, sizeof(sampleText)/sizeof(sampleText[0]),
                expected, sizeof(expected), "iso-2022-jp", 0 , TRUE, U_ZERO_ERROR))
            log_err("u-> not match.\n");
        if(!convertFromU(sampleText, sizeof(sampleText)/sizeof(sampleText[0]),
                expected, sizeof(expected), "iso-2022-jp", offsets , TRUE, U_ZERO_ERROR))
            log_err("u->  not match.\n");
    }

    log_verbose("Testing for ISO-2022-cn\n");
    {
        static const UChar    sampleText[] =   { 0x4e00, 0x04e01, 0x0031, 0xd801, 0xdc01, 0x0032};

        static const uint8_t expected[] = {
                                    0x1B, 0x24, 0x29, 0x41, 0x0E, 0x52, 0x3B, 
                                    0x36, 0x21,
                                    0x0F, 0x31, 
                                    0x1A, 
                                    0x32
                                    };

        

        static const int32_t offsets[] = {
                                    0,    0,    0,    0,    0,    0,    0,      
                                    1,    1,
                                    2,    2,
                                    3,  
                                    5,  };

        /*iso-2022-CN*/
        if(!convertFromU(sampleText, sizeof(sampleText)/sizeof(sampleText[0]),
                expected, sizeof(expected), "iso-2022-cn", 0 , TRUE, U_ZERO_ERROR))
            log_err("u-> not match.\n");
        if(!convertFromU(sampleText, sizeof(sampleText)/sizeof(sampleText[0]),
                expected, sizeof(expected), "iso-2022-cn", offsets , TRUE, U_ZERO_ERROR))
            log_err("u-> not match.\n");
    }

        log_verbose("Testing for ISO-2022-kr\n");
    {
        static const UChar    sampleText[] =   { 0x4e00,0xd801, 0xdc01, 0x04e01, 0x0031, 0xd801, 0xdc01, 0x0032};

        static const uint8_t expected[] = {0x1B, 0x24, 0x29, 0x43, 
                                    0x0E, 0x6C, 0x69, 
                                    0x0f, 0x1A, 
                                    0x0e, 0x6F, 0x4B, 
                                    0x0F, 0x31, 
                                    0x1A, 
                                    0x32 };        

        static const int32_t offsets[] = {-1, -1, -1, -1,
                              0, 0, 0, 
                              1, 1,  
                              3, 3, 3, 
                              4, 4, 
                              5, 
                              7,
                            };

        /*iso-2022-kr*/
        if(!convertFromU(sampleText, sizeof(sampleText)/sizeof(sampleText[0]),
                expected, sizeof(expected), "iso-2022-kr", 0 , TRUE, U_ZERO_ERROR))
            log_err("u-> iso-2022-kr [UCNV_DBCS] not match.\n");
        if(!convertFromU(sampleText, sizeof(sampleText)/sizeof(sampleText[0]),
                expected, sizeof(expected), "iso-2022-kr", offsets , TRUE, U_ZERO_ERROR))
            log_err("u-> iso-2022-kr [UCNV_DBCS] not match.\n");
    }

        log_verbose("Testing for HZ\n");
    {
        static const UChar    sampleText[] =   { 0x4e00, 0xd801, 0xdc01, 0x04e01, 0x0031, 0xd801, 0xdc01, 0x0032};

        static const uint8_t expected[] = {0x7E, 0x7B, 0x52, 0x3B,
                                    0x7E, 0x7D, 0x1A, 
                                    0x7E, 0x7B, 0x36, 0x21, 
                                    0x7E, 0x7D, 0x31, 
                                    0x1A,
                                    0x32 };


        static const int32_t offsets[] = {0,0,0,0,
                             1,1,1,
                             3,3,3,3,
                             4,4,4,
                             5,
                             7,};

        /*hz*/
        if(!convertFromU(sampleText, sizeof(sampleText)/sizeof(sampleText[0]),
                expected, sizeof(expected), "HZ", 0 , TRUE, U_ZERO_ERROR))
            log_err("u-> HZ not match.\n");
        if(!convertFromU(sampleText, sizeof(sampleText)/sizeof(sampleText[0]),
                expected, sizeof(expected), "HZ", offsets , TRUE, U_ZERO_ERROR))
            log_err("u-> HZ not match.\n");
    }
#endif

    /*UTF-8*/
     log_verbose("Testing for UTF8\n");
    {
        static const UChar    sampleText[] =   { 0x4e00, 0x0701, 0x0031, 0xbfc1, 0xd801, 0xdc01, 0x0032};
        static const int32_t offsets[]={0x00, 0x00, 0x00, 0x01, 0x01, 0x02,
                           0x03, 0x03, 0x03, 0x04, 0x04, 0x04,
                           0x04, 0x06 };
        static const uint8_t expected[] = {0xe4, 0xb8, 0x80, 0xdc, 0x81, 0x31, 
            0xeb, 0xbf, 0x81, 0xF0, 0x90, 0x90, 0x81, 0x32};


        static const int32_t fromOffsets[] = { 0x0000, 0x0003, 0x0005, 0x0006, 0x0009, 0x0009, 0x000D }; 
        /*UTF-8*/
        if(!convertFromU(sampleText, sizeof(sampleText)/sizeof(sampleText[0]),
            expected, sizeof(expected), "UTF8", offsets, TRUE, U_ZERO_ERROR ))
            log_err("u-> UTF8 with offsets and flush true did not match.\n");
        if(!convertFromU(sampleText, sizeof(sampleText)/sizeof(sampleText[0]),
            expected, sizeof(expected), "UTF8", 0, TRUE, U_ZERO_ERROR ))
            log_err("u-> UTF8 with offsets and flush true did not match.\n");
        if(!convertFromU(sampleText, sizeof(sampleText)/sizeof(sampleText[0]),
            expected, sizeof(expected), "UTF8", offsets, FALSE, U_ZERO_ERROR ))
            log_err("u-> UTF8 with offsets and flush true did not match.\n");
        if(!convertFromU(sampleText, sizeof(sampleText)/sizeof(sampleText[0]),
            expected, sizeof(expected), "UTF8", 0, FALSE, U_ZERO_ERROR ))
            log_err("u-> UTF8 with offsets and flush true did not match.\n");

        if(!convertToU(expected, sizeof(expected), 
            sampleText, sizeof(sampleText)/sizeof(sampleText[0]), "UTF8", 0, TRUE, U_ZERO_ERROR ))
            log_err("UTF8 -> u did not match.\n");
        if(!convertToU(expected, sizeof(expected), 
            sampleText, sizeof(sampleText)/sizeof(sampleText[0]), "UTF8", 0, FALSE, U_ZERO_ERROR ))
            log_err("UTF8 -> u did not match.\n");
        if(!convertToU(expected, sizeof(expected), 
            sampleText, sizeof(sampleText)/sizeof(sampleText[0]), "UTF8", fromOffsets, TRUE, U_ZERO_ERROR ))
            log_err("UTF8 ->u  did not match.\n");
        if(!convertToU(expected, sizeof(expected), 
            sampleText, sizeof(sampleText)/sizeof(sampleText[0]), "UTF8", fromOffsets, FALSE, U_ZERO_ERROR ))
            log_err("UTF8 -> u did not match.\n");

    }
}

/*test various error behaviours*/
static void TestErrorBehaviour(){
    log_verbose("Testing for SBCS and LATIN_1\n");
    {
        static const UChar    sampleText[] =   { 0x0031, 0xd801};
        static const UChar    sampleText2[] =   { 0x0031, 0xd801, 0x0032};
        static const uint8_t expected0[] =          { 0x31};
        static const uint8_t expected[] =          { 0x31, 0x1a};
        static const uint8_t expected2[] =         { 0x31, 0x1a, 0x32};

#if !UCONFIG_NO_LEGACY_CONVERSION
        /*SBCS*/
        if(!convertFromU(sampleText, sizeof(sampleText)/sizeof(sampleText[0]),
                expected, sizeof(expected), "ibm-920", 0, TRUE, U_ZERO_ERROR))
            log_err("u-> ibm-920 [UCNV_SBCS] \n");
        if(!convertFromU(sampleText, sizeof(sampleText)/sizeof(sampleText[0]),
                expected0, sizeof(expected0), "ibm-920", 0, FALSE, U_ZERO_ERROR))
            log_err("u-> ibm-920 [UCNV_SBCS] \n");
        if(!convertFromU(sampleText2, sizeof(sampleText2)/sizeof(sampleText2[0]),
                expected2, sizeof(expected2), "ibm-920", 0, TRUE, U_ZERO_ERROR))
            log_err("u-> ibm-920 [UCNV_SBCS] did not match\n");
#endif

        /*LATIN_1*/
        if(!convertFromU(sampleText, sizeof(sampleText)/sizeof(sampleText[0]),
                expected, sizeof(expected), "LATIN_1", 0, TRUE, U_ZERO_ERROR))
            log_err("u-> LATIN_1 is supposed to fail\n");
        if(!convertFromU(sampleText, sizeof(sampleText)/sizeof(sampleText[0]),
                expected0, sizeof(expected0), "LATIN_1", 0, FALSE, U_ZERO_ERROR))
            log_err("u-> LATIN_1 is supposed to fail\n");

        if(!convertFromU(sampleText2, sizeof(sampleText2)/sizeof(sampleText2[0]),
                expected2, sizeof(expected2), "LATIN_1", 0, TRUE, U_ZERO_ERROR))
            log_err("u-> LATIN_1 did not match\n");
    }

#if !UCONFIG_NO_LEGACY_CONVERSION
    log_verbose("Testing for DBCS and MBCS\n");
    {
        static const UChar    sampleText[]    = { 0x00a1, 0xd801};
        static const uint8_t expected[] = { 0xa2, 0xae};
        static const int32_t offsets[]        = { 0x00, 0x00};
        static const uint8_t expectedSUB[] = { 0xa2, 0xae, 0xa1, 0xe0};
        static const int32_t offsetsSUB[]        = { 0x00, 0x00, 0x01, 0x01};

        static const UChar       sampleText2[] = { 0x00a1, 0xd801, 0x00a4};
        static const uint8_t expected2[] = { 0xa2, 0xae, 0xa1, 0xe0, 0xa2, 0xb4};
        static const int32_t offsets2[]        = { 0x00, 0x00, 0x01, 0x01, 0x02, 0x02};

        static const UChar       sampleText3MBCS[] = { 0x0001, 0x00a4, 0xdc01};
        static const uint8_t expected3MBCS[] = { 0x01, 0xa2, 0xb4, 0xa1, 0xe0};
        static const int32_t offsets3MBCS[]        = { 0x00, 0x01, 0x01, 0x02, 0x02};

        static const UChar       sampleText4MBCS[] = { 0x0061, 0xFFE4, 0xdc01};
        static const uint8_t expected4MBCS[] = { 0x61, 0x8f, 0xa2, 0xc3, 0xf4, 0xfe};
        static const int32_t offsets4MBCS[]        = { 0x00, 0x01, 0x01, 0x01, 0x02, 0x02 };

        /*DBCS*/
        if(!convertFromU(sampleText, sizeof(sampleText)/sizeof(sampleText[0]),
                expectedSUB, sizeof(expectedSUB), "ibm-1363", 0, TRUE, U_ZERO_ERROR))
            log_err("u-> ibm-1363 [UCNV_DBCS portion] is supposed to fail\n");
        if(!convertFromU(sampleText, sizeof(sampleText)/sizeof(sampleText[0]),
                expected, sizeof(expected), "ibm-1363", 0, FALSE, U_ZERO_ERROR))
            log_err("u-> ibm-1363 [UCNV_DBCS portion] is supposed to fail\n");

        if(!convertFromU(sampleText, sizeof(sampleText)/sizeof(sampleText[0]),
                expectedSUB, sizeof(expectedSUB), "ibm-1363", offsetsSUB, TRUE, U_ZERO_ERROR))
            log_err("u-> ibm-1363 [UCNV_DBCS portion] is supposed to fail\n");
        if(!convertFromU(sampleText, sizeof(sampleText)/sizeof(sampleText[0]),
                expected, sizeof(expected), "ibm-1363", offsets, FALSE, U_ZERO_ERROR))
            log_err("u-> ibm-1363 [UCNV_DBCS portion] is supposed to fail\n");

        
        if(!convertFromU(sampleText2, sizeof(sampleText2)/sizeof(sampleText2[0]),
                expected2, sizeof(expected2), "ibm-1363", 0, TRUE, U_ZERO_ERROR))
            log_err("u-> ibm-1363 [UCNV_DBCS portion] did not match \n");
        if(!convertFromU(sampleText2, sizeof(sampleText2)/sizeof(sampleText2[0]),
                expected2, sizeof(expected2), "ibm-1363", offsets2, TRUE, U_ZERO_ERROR))
            log_err("u-> ibm-1363 [UCNV_DBCS portion] did not match \n");

        /*MBCS*/
        if(!convertFromU(sampleText, sizeof(sampleText)/sizeof(sampleText[0]),
                expectedSUB, sizeof(expectedSUB), "ibm-1363", 0, TRUE, U_ZERO_ERROR))
            log_err("u-> ibm-1363 [UCNV_MBCS] \n");
        if(!convertFromU(sampleText, sizeof(sampleText)/sizeof(sampleText[0]),
                expected, sizeof(expected), "ibm-1363", 0, FALSE, U_ZERO_ERROR))
            log_err("u-> ibm-1363 [UCNV_MBCS] \n");

        if(!convertFromU(sampleText2, sizeof(sampleText2)/sizeof(sampleText2[0]),
                expected2, sizeof(expected2), "ibm-1363", 0, TRUE, U_ZERO_ERROR))
            log_err("u-> ibm-1363 [UCNV_DBCS] did not match\n");
        if(!convertFromU(sampleText2, sizeof(sampleText2)/sizeof(sampleText2[0]),
                expected2, sizeof(expected2), "ibm-1363", 0, FALSE, U_ZERO_ERROR))
            log_err("u-> ibm-1363 [UCNV_DBCS] did not match\n");
        if(!convertFromU(sampleText2, sizeof(sampleText2)/sizeof(sampleText2[0]),
                expected2, sizeof(expected2), "ibm-1363", offsets2, FALSE, U_ZERO_ERROR))
            log_err("u-> ibm-1363 [UCNV_DBCS] did not match\n");

        if(!convertFromU(sampleText3MBCS, sizeof(sampleText3MBCS)/sizeof(sampleText3MBCS[0]),
                expected3MBCS, sizeof(expected3MBCS), "ibm-1363", offsets3MBCS, TRUE, U_ZERO_ERROR))
            log_err("u-> ibm-1363 [UCNV_MBCS] \n");
        if(!convertFromU(sampleText3MBCS, sizeof(sampleText3MBCS)/sizeof(sampleText3MBCS[0]),
                expected3MBCS, sizeof(expected3MBCS), "ibm-1363", offsets3MBCS, FALSE, U_ZERO_ERROR))
            log_err("u-> ibm-1363 [UCNV_MBCS] \n");

        if(!convertFromU(sampleText4MBCS, sizeof(sampleText4MBCS)/sizeof(sampleText4MBCS[0]),
                expected4MBCS, sizeof(expected4MBCS), "euc-jp", offsets4MBCS, TRUE, U_ZERO_ERROR))
            log_err("u-> euc-jp [UCNV_MBCS] \n");
        if(!convertFromU(sampleText4MBCS, sizeof(sampleText4MBCS)/sizeof(sampleText4MBCS[0]),
                expected4MBCS, sizeof(expected4MBCS), "euc-jp", offsets4MBCS, FALSE, U_ZERO_ERROR))
            log_err("u-> euc-jp [UCNV_MBCS] \n");
    }

    /*iso-2022-jp*/
    log_verbose("Testing for iso-2022-jp\n");
    {
        static const UChar    sampleText[]    = { 0x0031, 0xd801};
        static const uint8_t expected[] = {  0x31};
        static const uint8_t expectedSUB[] = {  0x31, 0x1a};
        static const int32_t offsets[]        = { 0x00, 1};

        static const UChar       sampleText2[] = { 0x0031, 0xd801, 0x0032};
        static const uint8_t expected2[] = {  0x31,0x1A,0x32};
        static const int32_t offsets2[]        = { 0x00,0x01,0x02};

        static const UChar       sampleText4MBCS[] = { 0x0061, 0x4e00, 0xdc01};
        static const uint8_t expected4MBCS[] = { 0x61, 0x1b, 0x24, 0x42, 0x30, 0x6c,0x1b,0x28,0x42,0x1a};
        static const int32_t offsets4MBCS[]        = { 0x00, 0x01, 0x01 ,0x01, 0x01, 0x01,0x02,0x02,0x02,0x02 };
        if(!convertFromU(sampleText, sizeof(sampleText)/sizeof(sampleText[0]),
                expectedSUB, sizeof(expectedSUB), "iso-2022-jp", offsets, TRUE, U_ZERO_ERROR))
            log_err("u-> iso-2022-jp [UCNV_MBCS] \n");
        if(!convertFromU(sampleText, sizeof(sampleText)/sizeof(sampleText[0]),
                expected, sizeof(expected), "iso-2022-jp", offsets, FALSE, U_ZERO_ERROR))
            log_err("u-> ibm-1363 [UCNV_MBCS] \n");

        if(!convertFromU(sampleText2, sizeof(sampleText2)/sizeof(sampleText2[0]),
                expected2, sizeof(expected2), "iso-2022-jp", offsets2, TRUE, U_ZERO_ERROR))
            log_err("u->iso-2022-jp[UCNV_DBCS] did not match\n");
        if(!convertFromU(sampleText2, sizeof(sampleText2)/sizeof(sampleText2[0]),
                expected2, sizeof(expected2), "iso-2022-jp", offsets2, FALSE, U_ZERO_ERROR))
            log_err("u-> iso-2022-jp [UCNV_DBCS] did not match\n");
        if(!convertFromU(sampleText2, sizeof(sampleText2)/sizeof(sampleText2[0]),
                expected2, sizeof(expected2), "iso-2022-jp", offsets2, FALSE, U_ZERO_ERROR))
            log_err("u-> iso-2022-jp [UCNV_DBCS] did not match\n");

        if(!convertFromU(sampleText4MBCS, sizeof(sampleText4MBCS)/sizeof(sampleText4MBCS[0]),
                expected4MBCS, sizeof(expected4MBCS), "iso-2022-jp", offsets4MBCS, TRUE, U_ZERO_ERROR))
            log_err("u-> iso-2022-jp [UCNV_MBCS] \n");
        if(!convertFromU(sampleText4MBCS, sizeof(sampleText4MBCS)/sizeof(sampleText4MBCS[0]),
                expected4MBCS, sizeof(expected4MBCS), "iso-2022-jp", offsets4MBCS, FALSE, U_ZERO_ERROR))
            log_err("u-> iso-2022-jp [UCNV_MBCS] \n");
    }

    /*iso-2022-cn*/
    log_verbose("Testing for iso-2022-cn\n");
    {
        static const UChar    sampleText[]    = { 0x0031, 0xd801};
        static const uint8_t expected[] = { 0x31};
        static const uint8_t expectedSUB[] = { 0x31, 0x1A};
        static const int32_t offsets[]        = { 0x00, 1};

        static const UChar       sampleText2[] = { 0x0031, 0xd801, 0x0032};
        static const uint8_t expected2[] = { 0x31, 0x1A,0x32};
        static const int32_t offsets2[]        = { 0x00, 0x01,0x02};

        static const UChar       sampleText3MBCS[] = { 0x0051, 0x0050, 0xdc01};
        static const uint8_t expected3MBCS[] = {0x51, 0x50, 0x1A};
        static const int32_t offsets3MBCS[]        = { 0x00, 0x01, 0x02 };

        static const UChar       sampleText4MBCS[] = { 0x0061, 0x4e00, 0xdc01};
        static const uint8_t expected4MBCS[] = { 0x61, 0x1b, 0x24, 0x29, 0x41, 0x0e, 0x52, 0x3b, 0x0f, 0x1a };
        static const int32_t offsets4MBCS[]        = { 0x00, 0x01, 0x01 ,0x01, 0x01, 0x01, 0x01, 0x01, 0x02, 0x02 };
        if(!convertFromU(sampleText, sizeof(sampleText)/sizeof(sampleText[0]),
                expectedSUB, sizeof(expectedSUB), "iso-2022-cn", offsets, TRUE, U_ZERO_ERROR))
            log_err("u-> iso-2022-cn [UCNV_MBCS] \n");
        if(!convertFromU(sampleText, sizeof(sampleText)/sizeof(sampleText[0]),
                expected, sizeof(expected), "iso-2022-cn", offsets, FALSE, U_ZERO_ERROR))
            log_err("u-> ibm-1363 [UCNV_MBCS] \n");

        if(!convertFromU(sampleText2, sizeof(sampleText2)/sizeof(sampleText2[0]),
                expected2, sizeof(expected2), "iso-2022-cn", offsets2, TRUE, U_ZERO_ERROR))
            log_err("u->iso-2022-cn[UCNV_DBCS] did not match\n");
        if(!convertFromU(sampleText2, sizeof(sampleText2)/sizeof(sampleText2[0]),
                expected2, sizeof(expected2), "iso-2022-cn", offsets2, FALSE, U_ZERO_ERROR))
            log_err("u-> iso-2022-cn [UCNV_DBCS] did not match\n");
        if(!convertFromU(sampleText2, sizeof(sampleText2)/sizeof(sampleText2[0]),
                expected2, sizeof(expected2), "iso-2022-cn", offsets2, FALSE, U_ZERO_ERROR))
            log_err("u-> iso-2022-cn [UCNV_DBCS] did not match\n");

        if(!convertFromU(sampleText3MBCS, sizeof(sampleText3MBCS)/sizeof(sampleText3MBCS[0]),
                expected3MBCS, sizeof(expected3MBCS), "iso-2022-cn", offsets3MBCS, TRUE, U_ZERO_ERROR))
            log_err("u->iso-2022-cn [UCNV_MBCS] \n");
        if(!convertFromU(sampleText3MBCS, sizeof(sampleText3MBCS)/sizeof(sampleText3MBCS[0]),
                expected3MBCS, sizeof(expected3MBCS), "iso-2022-cn", offsets3MBCS, FALSE, U_ZERO_ERROR))
            log_err("u-> iso-2022-cn[UCNV_MBCS] \n");

        if(!convertFromU(sampleText4MBCS, sizeof(sampleText4MBCS)/sizeof(sampleText4MBCS[0]),
                expected4MBCS, sizeof(expected4MBCS), "iso-2022-cn", offsets4MBCS, TRUE, U_ZERO_ERROR))
            log_err("u-> iso-2022-cn [UCNV_MBCS] \n");
        if(!convertFromU(sampleText4MBCS, sizeof(sampleText4MBCS)/sizeof(sampleText4MBCS[0]),
                expected4MBCS, sizeof(expected4MBCS), "iso-2022-cn", offsets4MBCS, FALSE, U_ZERO_ERROR))
            log_err("u-> iso-2022-cn [UCNV_MBCS] \n");
    }

    /*iso-2022-kr*/
    log_verbose("Testing for iso-2022-kr\n");
    {
        static const UChar    sampleText[]    = { 0x0031, 0xd801};
        static const uint8_t expected[] = { 0x1b, 0x24, 0x29, 0x43, 0x31};
        static const uint8_t expectedSUB[] = { 0x1b, 0x24, 0x29, 0x43, 0x31, 0x1A};
        static const int32_t offsets[]        = { -1,   -1,   -1,   -1,   0x00, 1};

        static const UChar       sampleText2[] = { 0x0031, 0xd801, 0x0032};
        static const uint8_t expected2[] = { 0x1b, 0x24, 0x29, 0x43, 0x31, 0x1A, 0x32};
        static const int32_t offsets2[]        = { -1,   -1,   -1,   -1,   0x00, 0x01, 0x02};

        static const UChar       sampleText3MBCS[] = { 0x0051, 0x0050, 0xdc01};
        static const uint8_t expected3MBCS[] = { 0x1b, 0x24, 0x29, 0x43,  0x51, 0x50, 0x1A };
        static const int32_t offsets3MBCS[]        = { -1,   -1,   -1,   -1,    0x00, 0x01, 0x02, 0x02 };

        if(!convertFromU(sampleText, sizeof(sampleText)/sizeof(sampleText[0]),
                expectedSUB, sizeof(expectedSUB), "iso-2022-kr", offsets, TRUE, U_ZERO_ERROR))
            log_err("u-> iso-2022-kr [UCNV_MBCS] \n");
        if(!convertFromU(sampleText, sizeof(sampleText)/sizeof(sampleText[0]),
                expected, sizeof(expected), "iso-2022-kr", offsets, FALSE, U_ZERO_ERROR))
            log_err("u-> ibm-1363 [UCNV_MBCS] \n");

        if(!convertFromU(sampleText2, sizeof(sampleText2)/sizeof(sampleText2[0]),
                expected2, sizeof(expected2), "iso-2022-kr", offsets2, TRUE, U_ZERO_ERROR))
            log_err("u->iso-2022-kr[UCNV_DBCS] did not match\n");
        if(!convertFromU(sampleText2, sizeof(sampleText2)/sizeof(sampleText2[0]),
                expected2, sizeof(expected2), "iso-2022-kr", offsets2, FALSE, U_ZERO_ERROR))
            log_err("u-> iso-2022-kr [UCNV_DBCS] did not match\n");
        if(!convertFromU(sampleText2, sizeof(sampleText2)/sizeof(sampleText2[0]),
                expected2, sizeof(expected2), "iso-2022-kr", offsets2, FALSE, U_ZERO_ERROR))
            log_err("u-> iso-2022-kr [UCNV_DBCS] did not match\n");

        if(!convertFromU(sampleText3MBCS, sizeof(sampleText3MBCS)/sizeof(sampleText3MBCS[0]),
                expected3MBCS, sizeof(expected3MBCS), "iso-2022-kr", offsets3MBCS, TRUE, U_ZERO_ERROR))
            log_err("u->iso-2022-kr [UCNV_MBCS] \n");
        if(!convertFromU(sampleText3MBCS, sizeof(sampleText3MBCS)/sizeof(sampleText3MBCS[0]),
                expected3MBCS, sizeof(expected3MBCS), "iso-2022-kr", offsets3MBCS, FALSE, U_ZERO_ERROR))
            log_err("u-> iso-2022-kr[UCNV_MBCS] \n");
    }

    /*HZ*/
    log_verbose("Testing for HZ\n");
    {
        static const UChar    sampleText[]    = { 0x0031, 0xd801};
        static const uint8_t expected[] = { 0x7e, 0x7d, 0x31};
        static const uint8_t expectedSUB[] = { 0x7e, 0x7d, 0x31, 0x1A};
        static const int32_t offsets[]        = { 0x00, 0x00, 0x00, 1};

        static const UChar       sampleText2[] = { 0x0031, 0xd801, 0x0032};
        static const uint8_t expected2[] = { 0x7e, 0x7d, 0x31,  0x1A,  0x32 };
        static const int32_t offsets2[]        = { 0x00, 0x00, 0x00, 0x01,  0x02 };

        static const UChar       sampleText3MBCS[] = { 0x0051, 0x0050, 0xdc01};
        static const uint8_t expected3MBCS[] = { 0x7e, 0x7d, 0x51, 0x50,  0x1A };
        static const int32_t offsets3MBCS[]        = { 0x00, 0x00, 0x00, 0x01, 0x02};

        static const UChar       sampleText4MBCS[] = { 0x0061, 0x4e00, 0xdc01};
        static const uint8_t expected4MBCS[] = { 0x7e, 0x7d, 0x61, 0x7e, 0x7b, 0x52, 0x3b, 0x7e, 0x7d, 0x1a };
        static const int32_t offsets4MBCS[]        = { 0x00, 0x00, 0x00, 0x01, 0x01, 0x01 ,0x01, 0x02, 0x02, 0x02 };
        if(!convertFromU(sampleText, sizeof(sampleText)/sizeof(sampleText[0]),
                expectedSUB, sizeof(expectedSUB), "HZ", offsets, TRUE, U_ZERO_ERROR))
            log_err("u-> HZ [UCNV_MBCS] \n");
        if(!convertFromU(sampleText, sizeof(sampleText)/sizeof(sampleText[0]),
                expected, sizeof(expected), "HZ", offsets, FALSE, U_ZERO_ERROR))
            log_err("u-> ibm-1363 [UCNV_MBCS] \n");

        if(!convertFromU(sampleText2, sizeof(sampleText2)/sizeof(sampleText2[0]),
                expected2, sizeof(expected2), "HZ", offsets2, TRUE, U_ZERO_ERROR))
            log_err("u->HZ[UCNV_DBCS] did not match\n");
        if(!convertFromU(sampleText2, sizeof(sampleText2)/sizeof(sampleText2[0]),
                expected2, sizeof(expected2), "HZ", offsets2, FALSE, U_ZERO_ERROR))
            log_err("u-> HZ [UCNV_DBCS] did not match\n");
        if(!convertFromU(sampleText2, sizeof(sampleText2)/sizeof(sampleText2[0]),
                expected2, sizeof(expected2), "HZ", offsets2, FALSE, U_ZERO_ERROR))
            log_err("u-> HZ [UCNV_DBCS] did not match\n");

        if(!convertFromU(sampleText3MBCS, sizeof(sampleText3MBCS)/sizeof(sampleText3MBCS[0]),
                expected3MBCS, sizeof(expected3MBCS), "HZ", offsets3MBCS, TRUE, U_ZERO_ERROR))
            log_err("u->HZ [UCNV_MBCS] \n");
        if(!convertFromU(sampleText3MBCS, sizeof(sampleText3MBCS)/sizeof(sampleText3MBCS[0]),
                expected3MBCS, sizeof(expected3MBCS), "HZ", offsets3MBCS, FALSE, U_ZERO_ERROR))
            log_err("u-> HZ[UCNV_MBCS] \n");

        if(!convertFromU(sampleText4MBCS, sizeof(sampleText4MBCS)/sizeof(sampleText4MBCS[0]),
                expected4MBCS, sizeof(expected4MBCS), "HZ", offsets4MBCS, TRUE, U_ZERO_ERROR))
            log_err("u-> HZ [UCNV_MBCS] \n");
        if(!convertFromU(sampleText4MBCS, sizeof(sampleText4MBCS)/sizeof(sampleText4MBCS[0]),
                expected4MBCS, sizeof(expected4MBCS), "HZ", offsets4MBCS, FALSE, U_ZERO_ERROR))
            log_err("u-> HZ [UCNV_MBCS] \n");
    }
#endif
}

#if !UCONFIG_NO_LEGACY_CONVERSION
/*test different convertToUnicode error behaviours*/
static void TestToUnicodeErrorBehaviour()
{
    log_verbose("Testing error conditions for DBCS\n");
    {
        uint8_t sampleText[] = { 0xa2, 0xae, 0x03, 0x04};
        const UChar expected[] = { 0x00a1 };
        
        if(!convertToU(sampleText, sizeof(sampleText), 
                expected, sizeof(expected)/sizeof(expected[0]), "ibm-1363", 0, TRUE, U_ZERO_ERROR ))
            log_err("DBCS (ibm-1363)->Unicode  did not match.\n");
        if(!convertToU(sampleText, sizeof(sampleText), 
                expected, sizeof(expected)/sizeof(expected[0]), "ibm-1363", 0, FALSE, U_ZERO_ERROR ))
            log_err("DBCS (ibm-1363)->Unicode  with flush = false did not match.\n");
    }
    log_verbose("Testing error conditions for SBCS\n");
    {
        uint8_t sampleText[] = { 0xa2, 0xFF};
        const UChar expected[] = { 0x00c2 };

      /*  uint8_t sampleText2[] = { 0xa2, 0x70 };
        const UChar expected2[] = { 0x0073 };*/

        if(!convertToU(sampleText, sizeof(sampleText), 
                expected, sizeof(expected)/sizeof(expected[0]), "ibm-1051", 0, TRUE, U_ZERO_ERROR ))
            log_err("SBCS (ibm-1051)->Unicode  did not match.\n");
        if(!convertToU(sampleText, sizeof(sampleText), 
                expected, sizeof(expected)/sizeof(expected[0]), "ibm-1051", 0, FALSE, U_ZERO_ERROR ))
            log_err("SBCS (ibm-1051)->Unicode  with flush = false did not match.\n");

    }
}

static void TestGetNextErrorBehaviour(){
   /*Test for unassigned character*/
#define INPUT_SIZE 1
    static const char input1[INPUT_SIZE]={ 0x70 };
    const char* source=(const char*)input1;
    UErrorCode err=U_ZERO_ERROR;
    UChar32 c=0;
    UConverter *cnv=ucnv_open("ibm-424", &err);
    if(U_FAILURE(err)) {
        log_data_err("Unable to open a SBCS(ibm-424) converter: %s\n", u_errorName(err));
        return;
    }
    c=ucnv_getNextUChar(cnv, &source, source + INPUT_SIZE, &err);
    if(err != U_INVALID_CHAR_FOUND && c!=0xfffd){
        log_err("FAIL in TestGetNextErrorBehaviour(unassigned): Expected: U_INVALID_CHAR_ERROR or 0xfffd ----Got:%s and 0x%lx\n",  myErrorName(err), c);
    }
    ucnv_close(cnv);
}
#endif

#define MAX_UTF16_LEN 2
#define MAX_UTF8_LEN 4

/*Regression test for utf8 converter*/
static void TestRegressionUTF8(){
    UChar32 currCh = 0;
    int32_t offset8;
    int32_t offset16;
    UChar *standardForm = (UChar*)malloc(MAX_LENGTH*sizeof(UChar));
    uint8_t *utf8 = (uint8_t*)malloc(MAX_LENGTH);

    while (currCh <= UNICODE_LIMIT) {
        offset16 = 0;
        offset8 = 0;
        while(currCh <= UNICODE_LIMIT
            && offset16 < (MAX_LENGTH/sizeof(UChar) - MAX_UTF16_LEN)
            && offset8 < (MAX_LENGTH - MAX_UTF8_LEN))
        {
            if (currCh == SURROGATE_HIGH_START) {
                currCh = SURROGATE_LOW_END + 1; /* Skip surrogate range */
            }
            UTF16_APPEND_CHAR_SAFE(standardForm, offset16, MAX_LENGTH, currCh);
            UTF8_APPEND_CHAR_SAFE(utf8, offset8, MAX_LENGTH, currCh);
            currCh++;
        }
        if(!convertFromU(standardForm, offset16, 
            utf8, offset8, "UTF8", 0, TRUE, U_ZERO_ERROR )) {
            log_err("Unicode->UTF8 did not match.\n");
        }
        if(!convertToU(utf8, offset8, 
            standardForm, offset16, "UTF8", 0, TRUE, U_ZERO_ERROR )) {
            log_err("UTF8->Unicode did not match.\n");
        }
    }

    free(standardForm);
    free(utf8);

    {
        static const char src8[] = { (char)0xCC, (char)0x81, (char)0xCC, (char)0x80 };
        static const UChar expected[] = { 0x0301, 0x0300 };
        UConverter *conv8;
        UErrorCode err = U_ZERO_ERROR;
        UChar pivotBuffer[100];
        const UChar* const pivEnd = pivotBuffer + 100;
        const char* srcBeg;
        const char* srcEnd;
        UChar* pivBeg;

        conv8 = ucnv_open("UTF-8", &err);

        srcBeg = src8;
        pivBeg = pivotBuffer;
        srcEnd = src8 + 3;
        ucnv_toUnicode(conv8, &pivBeg, pivEnd, &srcBeg, srcEnd, 0, FALSE, &err);
        if (srcBeg != srcEnd) {
            log_err("Did not consume whole buffer on first call.\n");
        }

        srcEnd = src8 + 4;
        ucnv_toUnicode(conv8, &pivBeg, pivEnd, &srcBeg, srcEnd, 0, TRUE, &err);
        if (srcBeg != srcEnd) {
            log_err("Did not consume whole buffer on second call.\n");
        }

        if (U_FAILURE(err) || (int32_t)(pivBeg - pivotBuffer) != 2 || u_strncmp(pivotBuffer, expected, 2) != 0) {
            log_err("Did not get expected results for UTF-8.\n");
        }
        ucnv_close(conv8);
    }
}

#define MAX_UTF32_LEN 1

static void TestRegressionUTF32(){
    UChar32 currCh = 0;
    int32_t offset32;
    int32_t offset16;
    UChar *standardForm = (UChar*)malloc(MAX_LENGTH*sizeof(UChar));
    UChar32 *utf32 = (UChar32*)malloc(MAX_LENGTH*sizeof(UChar32));

    while (currCh <= UNICODE_LIMIT) {
        offset16 = 0;
        offset32 = 0;
        while(currCh <= UNICODE_LIMIT
            && offset16 < (MAX_LENGTH/sizeof(UChar) - MAX_UTF16_LEN)
            && offset32 < (MAX_LENGTH/sizeof(UChar32) - MAX_UTF32_LEN))
        {
            if (currCh == SURROGATE_HIGH_START) {
                currCh = SURROGATE_LOW_END + 1; /* Skip surrogate range */
            }
            UTF16_APPEND_CHAR_SAFE(standardForm, offset16, MAX_LENGTH, currCh);
            UTF32_APPEND_CHAR_SAFE(utf32, offset32, MAX_LENGTH, currCh);
            currCh++;
        }
        if(!convertFromU(standardForm, offset16, 
            (const uint8_t *)utf32, offset32*sizeof(UChar32), "UTF32_PlatformEndian", 0, TRUE, U_ZERO_ERROR )) {
            log_err("Unicode->UTF32 did not match.\n");
        }
        if(!convertToU((const uint8_t *)utf32, offset32*sizeof(UChar32), 
            standardForm, offset16, "UTF32_PlatformEndian", 0, TRUE, U_ZERO_ERROR )) {
            log_err("UTF32->Unicode did not match.\n");
        }
    }
    free(standardForm);
    free(utf32);

    {
        /* Check for lone surrogate error handling. */
        static const UChar   sampleBadStartSurrogate[] = { 0x0031, 0xD800, 0x0032 };
        static const UChar   sampleBadEndSurrogate[] = { 0x0031, 0xDC00, 0x0032 };
        static const uint8_t expectedUTF32BE[] = {
            0x00, 0x00, 0x00, 0x31,
            0x00, 0x00, 0xff, 0xfd,
            0x00, 0x00, 0x00, 0x32
        };
        static const uint8_t expectedUTF32LE[] = {
            0x31, 0x00, 0x00, 0x00,
            0xfd, 0xff, 0x00, 0x00,
            0x32, 0x00, 0x00, 0x00
        };
        static const int32_t offsetsUTF32[] = {
            0x00, 0x00, 0x00, 0x00,
            0x01, 0x01, 0x01, 0x01,
            0x02, 0x02, 0x02, 0x02
        };

        if(!convertFromU(sampleBadStartSurrogate, sizeof(sampleBadStartSurrogate)/sizeof(sampleBadStartSurrogate[0]),
                expectedUTF32BE, sizeof(expectedUTF32BE), "UTF-32BE", offsetsUTF32, TRUE, U_ZERO_ERROR))
            log_err("u->UTF-32BE\n");
        if(!convertFromU(sampleBadEndSurrogate, sizeof(sampleBadEndSurrogate)/sizeof(sampleBadEndSurrogate[0]),
                expectedUTF32BE, sizeof(expectedUTF32BE), "UTF-32BE", offsetsUTF32, TRUE, U_ZERO_ERROR))
            log_err("u->UTF-32BE\n");

        if(!convertFromU(sampleBadStartSurrogate, sizeof(sampleBadStartSurrogate)/sizeof(sampleBadStartSurrogate[0]),
                expectedUTF32LE, sizeof(expectedUTF32LE), "UTF-32LE", offsetsUTF32, TRUE, U_ZERO_ERROR))
            log_err("u->UTF-32LE\n");
        if(!convertFromU(sampleBadEndSurrogate, sizeof(sampleBadEndSurrogate)/sizeof(sampleBadEndSurrogate[0]),
                expectedUTF32LE, sizeof(expectedUTF32LE), "UTF-32LE", offsetsUTF32, TRUE, U_ZERO_ERROR))
            log_err("u->UTF-32LE\n");
    }

    {
        static const char srcBE[] = { 0, 0, 0, 0x31, 0, 0, 0, 0x30 };
        static const UChar expected[] = { 0x0031, 0x0030 };
        UConverter *convBE;
        UErrorCode err = U_ZERO_ERROR;
        UChar pivotBuffer[100];
        const UChar* const pivEnd = pivotBuffer + 100;
        const char* srcBeg;
        const char* srcEnd;
        UChar* pivBeg;

        convBE = ucnv_open("UTF-32BE", &err);

        srcBeg = srcBE;
        pivBeg = pivotBuffer;
        srcEnd = srcBE + 5;
        ucnv_toUnicode(convBE, &pivBeg, pivEnd, &srcBeg, srcEnd, 0, FALSE, &err);
        if (srcBeg != srcEnd) {
            log_err("Did not consume whole buffer on first call.\n");
        }

        srcEnd = srcBE + 8;
        ucnv_toUnicode(convBE, &pivBeg, pivEnd, &srcBeg, srcEnd, 0, TRUE, &err);
        if (srcBeg != srcEnd) {
            log_err("Did not consume whole buffer on second call.\n");
        }

        if (U_FAILURE(err) || (int32_t)(pivBeg - pivotBuffer) != 2 || u_strncmp(pivotBuffer, expected, 2) != 0) {
            log_err("Did not get expected results for UTF-32BE.\n");
        }
        ucnv_close(convBE);
    }
    {
        static const char srcLE[] = { 0x31, 0, 0, 0, 0x30, 0, 0, 0 };
        static const UChar expected[] = { 0x0031, 0x0030 };
        UConverter *convLE;
        UErrorCode err = U_ZERO_ERROR;
        UChar pivotBuffer[100];
        const UChar* const pivEnd = pivotBuffer + 100;
        const char* srcBeg;
        const char* srcEnd;
        UChar* pivBeg;

        convLE = ucnv_open("UTF-32LE", &err);

        srcBeg = srcLE;
        pivBeg = pivotBuffer;
        srcEnd = srcLE + 5;
        ucnv_toUnicode(convLE, &pivBeg, pivEnd, &srcBeg, srcEnd, 0, FALSE, &err);
        if (srcBeg != srcEnd) {
            log_err("Did not consume whole buffer on first call.\n");
        }

        srcEnd = srcLE + 8;
        ucnv_toUnicode(convLE, &pivBeg, pivEnd, &srcBeg, srcEnd, 0, TRUE, &err);
        if (srcBeg != srcEnd) {
            log_err("Did not consume whole buffer on second call.\n");
        }

        if (U_FAILURE(err) || (int32_t)(pivBeg - pivotBuffer) != 2 || u_strncmp(pivotBuffer, expected, 2) != 0) {
            log_err("Did not get expected results for UTF-32LE.\n");
        }
        ucnv_close(convLE);
    }
}

/*Walk through the available converters*/
static void TestAvailableConverters(){
    UErrorCode status=U_ZERO_ERROR;
    UConverter *conv=NULL;
    int32_t i=0;
    for(i=0; i < ucnv_countAvailable(); i++){
        status=U_ZERO_ERROR;
        conv=ucnv_open(ucnv_getAvailableName(i), &status);
        if(U_FAILURE(status)){
            log_err("ERROR: converter creation failed. Failure in alias table or the data table for \n converter=%s. Error=%s\n", 
                        ucnv_getAvailableName(i), myErrorName(status));
            continue;
        }
        ucnv_close(conv);
    }

}

static void TestFlushInternalBuffer(){
    TestWithBufferSize(MAX_LENGTH, 1);
    TestWithBufferSize(1, 1);
    TestWithBufferSize(1, MAX_LENGTH);
    TestWithBufferSize(MAX_LENGTH, MAX_LENGTH);
}

static void TestWithBufferSize(int32_t insize, int32_t outsize){

    gInBufferSize =insize;
    gOutBufferSize = outsize;

     log_verbose("Testing fromUnicode for UTF-8 with UCNV_TO_U_CALLBACK_SUBSTITUTE \n");
    {
        UChar    sampleText[] = 
            { 0x0031, 0x0032, 0x0033, 0x0000, 0x4e00, 0x4e8c, 0x4e09,  0x002E  };
        const uint8_t expectedUTF8[] = 
            { 0x31, 0x32, 0x33, 0x00, 0xe4, 0xb8, 0x80, 0xe4, 0xba, 0x8c, 0xe4, 0xb8, 0x89, 0x2E };
        int32_t  toUTF8Offs[] = 
            { 0x00, 0x01, 0x02, 0x03, 0x04, 0x04, 0x04, 0x05, 0x05, 0x05, 0x06, 0x06, 0x06, 0x07};
       /* int32_t fmUTF8Offs[] = 
            { 0x0000, 0x0001, 0x0002, 0x0003, 0x0004, 0x0007, 0x000a, 0x000d };*/

        /*UTF-8*/
        if(!testConvertFromU(sampleText, sizeof(sampleText)/sizeof(sampleText[0]),
            expectedUTF8, sizeof(expectedUTF8), "UTF8", UCNV_FROM_U_CALLBACK_SUBSTITUTE, toUTF8Offs ,FALSE))
             log_err("u-> UTF8 did not match.\n");
    }

#if !UCONFIG_NO_LEGACY_CONVERSION
     log_verbose("Testing fromUnicode with UCNV_FROM_U_CALLBACK_ESCAPE  \n");
    {
        UChar inputTest[] = { 0x0061, 0xd801, 0xdc01, 0xd801, 0x0061 };
        const uint8_t toIBM943[]= { 0x61, 
            0x25, 0x55, 0x44, 0x38, 0x30, 0x31,
            0x25, 0x55, 0x44, 0x43, 0x30, 0x31,
            0x25, 0x55, 0x44, 0x38, 0x30, 0x31,
            0x61 };
        int32_t offset[]= {0, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 3, 3, 3, 3, 3, 3, 4};

        if(!testConvertFromU(inputTest, sizeof(inputTest)/sizeof(inputTest[0]),
                toIBM943, sizeof(toIBM943), "ibm-943",
                (UConverterFromUCallback)UCNV_FROM_U_CALLBACK_ESCAPE, offset,FALSE))
            log_err("u-> ibm-943 with subst with value did not match.\n");
    }
#endif

     log_verbose("Testing fromUnicode for UTF-8 with UCNV_TO_U_CALLBACK_SUBSTITUTE \n");
    {
        const uint8_t sampleText1[] = { 0x31, 0xe4, 0xba, 0x8c, 
            0xe0, 0x80,  0x61};
        UChar    expected1[] = {  0x0031, 0x4e8c, 0xfffd, 0x0061};
        int32_t offsets1[] = {   0x0000, 0x0001, 0x0004, 0x0006};

        if(!testConvertToU(sampleText1, sizeof(sampleText1),
                 expected1, sizeof(expected1)/sizeof(expected1[0]),"utf8", UCNV_TO_U_CALLBACK_SUBSTITUTE, offsets1,FALSE))
            log_err("utf8->u with substitute did not match.\n");;
    }

#if !UCONFIG_NO_LEGACY_CONVERSION
    log_verbose("Testing toUnicode with UCNV_TO_U_CALLBACK_ESCAPE \n");
    /*to Unicode*/
    {
        const uint8_t sampleTxtToU[]= { 0x00, 0x9f, 0xaf, 
            0x81, 0xad, /*unassigned*/
            0x89, 0xd3 };
        UChar IBM_943toUnicode[] = { 0x0000, 0x6D63, 
            0x25, 0x58, 0x38, 0x31, 0x25, 0x58, 0x41, 0x44,
            0x7B87};
        int32_t  fromIBM943Offs [] =    { 0, 1, 3, 3, 3, 3, 3, 3, 3, 3, 5};

        if(!testConvertToU(sampleTxtToU, sizeof(sampleTxtToU),
                 IBM_943toUnicode, sizeof(IBM_943toUnicode)/sizeof(IBM_943toUnicode[0]),"ibm-943",
                (UConverterToUCallback)UCNV_TO_U_CALLBACK_ESCAPE, fromIBM943Offs,FALSE))
            log_err("ibm-943->u with substitute with value did not match.\n");

    }
#endif
}

static UBool convertFromU( const UChar *source, int sourceLen,  const uint8_t *expect, int expectLen, 
                const char *codepage, const int32_t *expectOffsets, UBool doFlush, UErrorCode expectedStatus)
{

    int32_t i=0;
    char *p=0;
    const UChar *src;
    char buffer[MAX_LENGTH];
    int32_t offsetBuffer[MAX_LENGTH];
    int32_t *offs=0;
    char *targ;
    char *targetLimit;
    UChar *sourceLimit=0;
    UErrorCode status = U_ZERO_ERROR;
    UConverter *conv = 0;
    conv = ucnv_open(codepage, &status);
    if(U_FAILURE(status))
    {
        log_data_err("Couldn't open converter %s\n",codepage);    
        return TRUE;
    }
    log_verbose("Converter %s opened..\n", ucnv_getName(conv, &status));

    for(i=0; i<MAX_LENGTH; i++){
        buffer[i]=(char)0xF0;
        offsetBuffer[i]=0xFF;
    }

    src=source;
    sourceLimit=(UChar*)src+(sourceLen);
    targ=buffer;
    targetLimit=targ+MAX_LENGTH;
    offs=offsetBuffer;
    ucnv_fromUnicode (conv,
                  (char **)&targ,
                  (const char *)targetLimit,
                  &src,
                  sourceLimit,
                  expectOffsets ? offs : NULL,
                  doFlush, 
                  &status);
    ucnv_close(conv);
    if(status != expectedStatus){
          log_err("ucnv_fromUnicode() failed for codepage=%s. Error =%s Expected=%s\n", codepage, myErrorName(status), myErrorName(expectedStatus));
          return FALSE;
    }

    log_verbose("\nConversion done [%d uchars in -> %d chars out]. \nResult :",
        sourceLen, targ-buffer);

    if(expectLen != targ-buffer)
    {
        log_err("Expected %d chars out, got %d FROM Unicode to %s\n", expectLen, targ-buffer, codepage);
        log_verbose("Expected %d chars out, got %d FROM Unicode to %s\n", expectLen, targ-buffer, codepage);
        printSeqErr((const unsigned char *)buffer, (int32_t)(targ-buffer));
        printSeqErr((const unsigned char*)expect, expectLen);
        return FALSE;
    }

    if(memcmp(buffer, expect, expectLen)){
        log_err("String does not match. FROM Unicode to codePage%s\n", codepage);
        log_info("\nGot:");
        printSeqErr((const unsigned char *)buffer, expectLen);
        log_info("\nExpected:");
        printSeqErr((const unsigned char *)expect, expectLen);
        return FALSE;
    }
    else {    
        log_verbose("Matches!\n");
    }

    if (expectOffsets != 0){
        log_verbose("comparing %d offsets..\n", targ-buffer);
        if(memcmp(offsetBuffer,expectOffsets,(targ-buffer) * sizeof(int32_t) )){
            log_err("did not get the expected offsets. for FROM Unicode to %s\n", codepage);
            log_info("\nGot  : ");
            printSeqErr((const unsigned char*)buffer, (int32_t)(targ-buffer));
            for(p=buffer;p<targ;p++)
                log_info("%d, ", offsetBuffer[p-buffer]); 
            log_info("\nExpected: ");
            for(i=0; i< (targ-buffer); i++)
                log_info("%d,", expectOffsets[i]);
        }
    }

    return TRUE;    
}


static UBool convertToU( const uint8_t *source, int sourceLen, const UChar *expect, int expectLen, 
               const char *codepage, const int32_t *expectOffsets, UBool doFlush, UErrorCode expectedStatus)
{
    UErrorCode status = U_ZERO_ERROR;
    UConverter *conv = 0;
    int32_t i=0;
    UChar *p=0;
    const char* src;
    UChar buffer[MAX_LENGTH];
    int32_t offsetBuffer[MAX_LENGTH];
    int32_t *offs=0;
    UChar *targ;
    UChar *targetLimit;
    uint8_t *sourceLimit=0;



    conv = ucnv_open(codepage, &status);
    if(U_FAILURE(status))
    {
        log_data_err("Couldn't open converter %s\n",codepage);    
        return TRUE;
    }
    log_verbose("Converter %s opened..\n", ucnv_getName(conv, &status));



    for(i=0; i<MAX_LENGTH; i++){
        buffer[i]=0xFFFE;
        offsetBuffer[i]=-1;
    }

    src=(const char *)source;
    sourceLimit=(uint8_t*)(src+(sourceLen));
    targ=buffer;
    targetLimit=targ+MAX_LENGTH;
    offs=offsetBuffer;



    ucnv_toUnicode (conv,
                &targ,
                targetLimit,
                (const char **)&src,
                (const char *)sourceLimit,
                expectOffsets ? offs : NULL,
                doFlush,
                &status);

    ucnv_close(conv);
    if(status != expectedStatus){
          log_err("ucnv_fromUnicode() failed for codepage=%s. Error =%s Expected=%s\n", codepage, myErrorName(status), myErrorName(expectedStatus));
          return FALSE;
    }
    log_verbose("\nConversion done [%d uchars in -> %d chars out]. \nResult :",
        sourceLen, targ-buffer);




    log_verbose("comparing %d uchars (%d bytes)..\n",expectLen,expectLen*2);

    if (expectOffsets != 0) {
        if(memcmp(offsetBuffer, expectOffsets, (targ-buffer) * sizeof(int32_t))){

            log_err("did not get the expected offsets from %s To UNICODE\n", codepage);
            log_info("\nGot : ");
            for(p=buffer;p<targ;p++)
                log_info("%d, ", offsetBuffer[p-buffer]); 
            log_info("\nExpected: ");
            for(i=0; i<(targ-buffer); i++)
                log_info("%d, ", expectOffsets[i]);
            log_info("\nGot result:");
            for(i=0; i<(targ-buffer); i++)
                log_info("0x%04X,", buffer[i]);
            log_info("\nFrom Input:");
            for(i=0; i<(src-(const char *)source); i++)
                log_info("0x%02X,", (unsigned char)source[i]);
            log_info("\n");
        }
    }
    if(memcmp(buffer, expect, expectLen*2)){
        log_err("String does not match. from codePage %s TO Unicode\n", codepage);
        log_info("\nGot:");
        printUSeqErr(buffer, expectLen);
        log_info("\nExpected:");
        printUSeqErr(expect, expectLen);
        return FALSE;
    }
    else {
        log_verbose("Matches!\n");
    }

    return TRUE;
}


static UBool testConvertFromU( const UChar *source, int sourceLen,  const uint8_t *expect, int expectLen, 
                const char *codepage, UConverterFromUCallback callback , const int32_t *expectOffsets, UBool testReset)
{
    UErrorCode status = U_ZERO_ERROR;
    UConverter *conv = 0;
    char    junkout[MAX_LENGTH]; /* FIX */
    int32_t    junokout[MAX_LENGTH]; /* FIX */
    char *p;
    const UChar *src;
    char *end;
    char *targ;
    int32_t *offs;
    int i;
    int32_t   realBufferSize;
    char *realBufferEnd;
    const UChar *realSourceEnd;
    const UChar *sourceLimit;
    UBool checkOffsets = TRUE;
    UBool doFlush;

    UConverterFromUCallback oldAction = NULL;
    const void* oldContext = NULL;

    for(i=0;i<MAX_LENGTH;i++)
        junkout[i] = (char)0xF0;
    for(i=0;i<MAX_LENGTH;i++)
        junokout[i] = 0xFF;

    setNuConvTestName(codepage, "FROM");

    log_verbose("\n=========  %s\n", gNuConvTestName);

    conv = ucnv_open(codepage, &status);
    if(U_FAILURE(status))
    {
        log_data_err("Couldn't open converter %s\n",codepage);    
        return TRUE;
    }

    log_verbose("Converter opened..\n");
    /*----setting the callback routine----*/
    ucnv_setFromUCallBack (conv, callback, NULL, &oldAction, &oldContext, &status);
    if (U_FAILURE(status)) { 
        log_err("FAILURE in setting the callback Function! %s\n", myErrorName(status));  
    }
    /*------------------------*/

    src = source;
    targ = junkout;
    offs = junokout;

    realBufferSize = (sizeof(junkout)/sizeof(junkout[0]));
    realBufferEnd = junkout + realBufferSize;
    realSourceEnd = source + sourceLen;

    if ( gOutBufferSize != realBufferSize )
      checkOffsets = FALSE;

    if( gInBufferSize != MAX_LENGTH )
      checkOffsets = FALSE;

    do
    {
        end = nct_min(targ + gOutBufferSize, realBufferEnd);
        sourceLimit = nct_min(src + gInBufferSize, realSourceEnd);

        doFlush = (UBool)(sourceLimit == realSourceEnd);

        if(targ == realBufferEnd)
          {
        log_err("Error, overflowed the real buffer while about to call fromUnicode! targ=%08lx %s", targ, gNuConvTestName);
        return FALSE;
          }
        log_verbose("calling fromUnicode @ SOURCE:%08lx to %08lx  TARGET: %08lx to %08lx, flush=%s\n", src,sourceLimit, targ,end, doFlush?"TRUE":"FALSE");


        status = U_ZERO_ERROR;
        if(gInBufferSize ==999 && gOutBufferSize==999)
            doFlush = FALSE;
        ucnv_fromUnicode (conv,
                  (char **)&targ,
                  (const char *)end,
                  &src,
                  sourceLimit,
                  offs,
                  doFlush, /* flush if we're at the end of the input data */
                  &status);
        if(testReset) 
            ucnv_resetToUnicode(conv);
        if(gInBufferSize ==999 && gOutBufferSize==999)
            ucnv_resetToUnicode(conv);

      } while ( (status == U_BUFFER_OVERFLOW_ERROR) || (U_SUCCESS(status) && sourceLimit < realSourceEnd) );

    if(U_FAILURE(status)) {
        log_err("Problem doing fromUnicode to %s, errcode %s %s\n", codepage, myErrorName(status), gNuConvTestName);
        return FALSE;
      }

    log_verbose("\nConversion done [%d uchars in -> %d chars out]. \nResult :",
        sourceLen, targ-junkout);
    if(getTestOption(VERBOSITY_OPTION))
    {
        char junk[999];
        char offset_str[999];
        char *ptr;

        junk[0] = 0;
        offset_str[0] = 0;
        for(ptr = junkout;ptr<targ;ptr++)
        {
            sprintf(junk + strlen(junk), "0x%02x, ", (0xFF) & (unsigned int)*ptr);
            sprintf(offset_str + strlen(offset_str), "0x%02x, ", (0xFF) & (unsigned int)junokout[ptr-junkout]);
        }
        
        log_verbose(junk);
        printSeq((const unsigned char *)expect, expectLen);
        if ( checkOffsets )
          {
            log_verbose("\nOffsets:");
            log_verbose(offset_str);
          }
        log_verbose("\n");
    }
    ucnv_close(conv);


    if(expectLen != targ-junkout)
    {
        log_err("Expected %d chars out, got %d %s\n", expectLen, targ-junkout, gNuConvTestName);
        log_verbose("Expected %d chars out, got %d %s\n", expectLen, targ-junkout, gNuConvTestName);
        log_info("\nGot:");
        printSeqErr((const unsigned char*)junkout, (int32_t)(targ-junkout));
        log_info("\nExpected:");
        printSeqErr((const unsigned char*)expect, expectLen);
        return FALSE;
    }

    if (checkOffsets && (expectOffsets != 0) )
    {
        log_verbose("comparing %d offsets..\n", targ-junkout);
        if(memcmp(junokout,expectOffsets,(targ-junkout) * sizeof(int32_t) )){
            log_err("did not get the expected offsets. %s", gNuConvTestName);
            log_err("Got  : ");
            printSeqErr((const unsigned char*)junkout, (int32_t)(targ-junkout));
            for(p=junkout;p<targ;p++)
                log_err("%d, ", junokout[p-junkout]); 
            log_err("\nExpected: ");
            for(i=0; i<(targ-junkout); i++)
                log_err("%d,", expectOffsets[i]);
        }
    }

    log_verbose("comparing..\n");
    if(!memcmp(junkout, expect, expectLen))
    {
        log_verbose("Matches!\n");
        return TRUE;
    }
    else
    {
        log_err("String does not match. %s\n", gNuConvTestName);
        printUSeqErr(source, sourceLen);
        log_info("\nGot:");
        printSeqErr((const unsigned char *)junkout, expectLen);
        log_info("\nExpected:");
        printSeqErr((const unsigned char *)expect, expectLen);

        return FALSE;
    }
}

static UBool testConvertToU( const uint8_t *source, int sourcelen, const UChar *expect, int expectlen, 
               const char *codepage, UConverterToUCallback callback, const int32_t *expectOffsets, UBool testReset)
{
    UErrorCode status = U_ZERO_ERROR;
    UConverter *conv = 0;
    UChar    junkout[MAX_LENGTH]; /* FIX */
    int32_t    junokout[MAX_LENGTH]; /* FIX */
    const char *src;
    const char *realSourceEnd;
    const char *srcLimit;
    UChar *p;
    UChar *targ;
    UChar *end;
    int32_t *offs;
    int i;
    UBool   checkOffsets = TRUE;
    int32_t   realBufferSize;
    UChar *realBufferEnd;
    UBool doFlush;

    UConverterToUCallback oldAction = NULL;
    const void* oldContext = NULL;
    

    for(i=0;i<MAX_LENGTH;i++)
        junkout[i] = 0xFFFE;

    for(i=0;i<MAX_LENGTH;i++)
        junokout[i] = -1;

    setNuConvTestName(codepage, "TO");

    log_verbose("\n=========  %s\n", gNuConvTestName);

    conv = ucnv_open(codepage, &status);
    if(U_FAILURE(status))
    {
        log_data_err("Couldn't open converter %s\n",gNuConvTestName);
        return TRUE;
    }

    log_verbose("Converter opened..\n");
     /*----setting the callback routine----*/
    ucnv_setToUCallBack (conv, callback, NULL, &oldAction, &oldContext, &status);
    if (U_FAILURE(status)) { 
        log_err("FAILURE in setting the callback Function! %s\n", myErrorName(status));  
    }
    /*-------------------------------------*/

    src = (const char *)source;
    targ = junkout;
    offs = junokout;
    
    realBufferSize = (sizeof(junkout)/sizeof(junkout[0]));
    realBufferEnd = junkout + realBufferSize;
    realSourceEnd = src + sourcelen;

    if ( gOutBufferSize != realBufferSize )
      checkOffsets = FALSE;

    if( gInBufferSize != MAX_LENGTH )
      checkOffsets = FALSE;

    do
      {
        end = nct_min( targ + gOutBufferSize, realBufferEnd);
        srcLimit = nct_min(realSourceEnd, src + gInBufferSize);

        if(targ == realBufferEnd)
        {
            log_err("Error, the end would overflow the real output buffer while about to call toUnicode! tarjey=%08lx %s",targ,gNuConvTestName);
            return FALSE;
        }
        log_verbose("calling toUnicode @ %08lx to %08lx\n", targ,end);

        /* oldTarg = targ; */

        status = U_ZERO_ERROR;
        doFlush=(UBool)((gInBufferSize ==999 && gOutBufferSize==999)?(srcLimit == realSourceEnd) : FALSE);
            
        ucnv_toUnicode (conv,
                &targ,
                end,
                (const char **)&src,
                (const char *)srcLimit,
                offs,
                doFlush, /* flush if we're at the end of hte source data */
                &status);
        if(testReset) 
            ucnv_resetFromUnicode(conv);
        if(gInBufferSize ==999 && gOutBufferSize==999)
            ucnv_resetToUnicode(conv);    
        /*        offs += (targ-oldTarg); */

      } while ( (status == U_BUFFER_OVERFLOW_ERROR) || (U_SUCCESS(status) && (srcLimit < realSourceEnd)) ); /* while we just need another buffer */

    if(U_FAILURE(status))
    {
        log_err("Problem doing %s toUnicode, errcode %s %s\n", codepage, myErrorName(status), gNuConvTestName);
        return FALSE;
    }

    log_verbose("\nConversion done. %d bytes -> %d chars.\nResult :",
        sourcelen, targ-junkout);
    if(getTestOption(VERBOSITY_OPTION))
    {
        char junk[999];
        char offset_str[999];
    
        UChar *ptr;
        
        junk[0] = 0;
        offset_str[0] = 0;

        for(ptr = junkout;ptr<targ;ptr++)
        {
            sprintf(junk + strlen(junk), "0x%04x, ", (0xFFFF) & (unsigned int)*ptr);
            sprintf(offset_str + strlen(offset_str), "0x%04x, ", (0xFFFF) & (unsigned int)junokout[ptr-junkout]);
        }
        
        log_verbose(junk);

        if ( checkOffsets )
          {
            log_verbose("\nOffsets:");
            log_verbose(offset_str);
          }
        log_verbose("\n");
    }
    ucnv_close(conv);

    log_verbose("comparing %d uchars (%d bytes)..\n",expectlen,expectlen*2);

    if (checkOffsets && (expectOffsets != 0))
    {
        if(memcmp(junokout,expectOffsets,(targ-junkout) * sizeof(int32_t))){
            
            log_err("did not get the expected offsets. %s",gNuConvTestName);
            for(p=junkout;p<targ;p++)
                log_err("%d, ", junokout[p-junkout]); 
            log_err("\nExpected: ");
            for(i=0; i<(targ-junkout); i++)
                log_err("%d,", expectOffsets[i]);
            log_err("");
            for(i=0; i<(targ-junkout); i++)
                log_err("%X,", junkout[i]);
            log_err("");
            for(i=0; i<(src-(const char *)source); i++)
                log_err("%X,", (unsigned char)source[i]);
        }
    }

    if(!memcmp(junkout, expect, expectlen*2))
    {
        log_verbose("Matches!\n");
        return TRUE;
    }
    else
    {
        log_err("String does not match. %s\n", gNuConvTestName);
        log_verbose("String does not match. %s\n", gNuConvTestName);
        log_info("\nGot:");
        printUSeq(junkout, expectlen);
        log_info("\nExpected:");
        printUSeq(expect, expectlen); 
        return FALSE;
    }
}


static void TestResetBehaviour(void){
#if !UCONFIG_NO_LEGACY_CONVERSION
    log_verbose("Testing Reset for DBCS and MBCS\n");
    {
        static const UChar sampleText[]       = {0x00a1, 0xd801, 0xdc01, 0x00a4};
        static const uint8_t expected[] = {0xa2, 0xae, 0xa1, 0xe0, 0xa2, 0xb4};
        static const int32_t offsets[]        = {0x00, 0x00, 0x01, 0x01, 0x03, 0x03 };

        
        static const UChar sampleText1[] = {0x00a1, 0x00a4, 0x00a7, 0x00a8};
        static const uint8_t expected1[] = {0xa2, 0xae,0xA2,0xB4,0xA1,0xD7,0xA1,0xA7};
        static const int32_t offsets1[] =  { 0,2,4,6};

        /*DBCS*/
        if(!testConvertFromU(sampleText, sizeof(sampleText)/sizeof(sampleText[0]),
                expected, sizeof(expected), "ibm-1363", UCNV_FROM_U_CALLBACK_SUBSTITUTE , NULL, TRUE))
            log_err("u-> ibm-1363 [UCNV_DBCS portion] not match.\n");
        if(!testConvertFromU(sampleText, sizeof(sampleText)/sizeof(sampleText[0]),
                expected, sizeof(expected), "ibm-1363", UCNV_FROM_U_CALLBACK_SUBSTITUTE,offsets , TRUE))
            log_err("u-> ibm-1363 [UCNV_DBCS portion] not match.\n");
       
        if(!testConvertToU(expected1, sizeof(expected1), 
                sampleText1, sizeof(sampleText1)/sizeof(sampleText1[0]), "ibm-1363",UCNV_TO_U_CALLBACK_SUBSTITUTE , 
                offsets1, TRUE))
           log_err("ibm-1363 -> did not match.\n");
        /*MBCS*/
        if(!testConvertFromU(sampleText, sizeof(sampleText)/sizeof(sampleText[0]),
                expected, sizeof(expected), "ibm-1363", UCNV_FROM_U_CALLBACK_SUBSTITUTE , NULL, TRUE))
            log_err("u-> ibm-1363 [UCNV_MBCS] not match.\n");
        if(!testConvertFromU(sampleText, sizeof(sampleText)/sizeof(sampleText[0]),
                expected, sizeof(expected), "ibm-1363", UCNV_FROM_U_CALLBACK_SUBSTITUTE,offsets , TRUE))
            log_err("u-> ibm-1363 [UCNV_MBCS] not match.\n");
      
        if(!testConvertToU(expected1, sizeof(expected1), 
                sampleText1, sizeof(sampleText1)/sizeof(sampleText1[0]), "ibm-1363",UCNV_TO_U_CALLBACK_SUBSTITUTE , 
                offsets1, TRUE))
           log_err("ibm-1363 -> did not match.\n");

    }

    log_verbose("Testing Reset for ISO-2022-jp\n");
    {
        static const UChar    sampleText[] =   { 0x4e00, 0x04e01, 0x0031, 0xd801, 0xdc01, 0x0032};

        static const uint8_t expected[] = {0x1b, 0x24, 0x42,0x30,0x6c,0x43,0x7a,0x1b,0x28,0x42,
                                    0x31,0x1A, 0x32};


        static const int32_t offsets[] = {0,0,0,0,0,1,1,2,2,2,2,3,5 };

        
        static const UChar sampleText1[] = {0x4e00, 0x04e01, 0x0031,0x001A, 0x0032};
        static const uint8_t expected1[] = {0x1b, 0x24, 0x42,0x30,0x6c,0x43,0x7a,0x1b,0x28,0x42,
                                    0x31,0x1A, 0x32};
        static const int32_t offsets1[] =  { 3,5,10,11,12};

        /*iso-2022-jp*/
        if(!testConvertFromU(sampleText, sizeof(sampleText)/sizeof(sampleText[0]),
                expected, sizeof(expected), "iso-2022-jp",  UCNV_FROM_U_CALLBACK_SUBSTITUTE , NULL, TRUE))
            log_err("u-> not match.\n");
        if(!testConvertFromU(sampleText, sizeof(sampleText)/sizeof(sampleText[0]),
                expected, sizeof(expected), "iso-2022-jp", UCNV_FROM_U_CALLBACK_SUBSTITUTE,offsets , TRUE))
            log_err("u->  not match.\n");
        
        if(!testConvertToU(expected1, sizeof(expected1), 
                sampleText1, sizeof(sampleText1)/sizeof(sampleText1[0]), "iso-2022-jp",UCNV_TO_U_CALLBACK_SUBSTITUTE ,
                offsets1, TRUE))
           log_err("iso-2022-jp -> did not match.\n");

    }

    log_verbose("Testing Reset for ISO-2022-cn\n");
    {
        static const UChar    sampleText[] =   { 0x4e00, 0x04e01, 0x0031, 0xd801, 0xdc01, 0x0032};

        static const uint8_t expected[] = {
                                    0x1B, 0x24, 0x29, 0x41, 0x0E, 0x52, 0x3B, 
                                    0x36, 0x21,
                                    0x0f, 0x31,
                                    0x1A, 
                                    0x32
                                    };
        

        static const int32_t offsets[] = {
                                    0,    0,    0,    0,    0,    0,    0,      
                                    1,    1,
                                    2,    2,
                                    3,    
                                    5,  };
        
        UChar sampleText1[] = {0x4e00, 0x04e01, 0x0031,0x001A, 0x0032};
        static const uint8_t expected1[] = {
                                    0x1B, 0x24, 0x29, 0x41, 0x0E, 0x52, 0x3B, 
                                    0x36, 0x21,
                                    0x1B, 0x24, 0x29, 0x47, 0x24, 0x22, 
                                    0x0f, 0x1A, 
                                    0x32
                                    };
        static const int32_t offsets1[] =  { 5,7,13,16,17};

        /*iso-2022-CN*/
        if(!testConvertFromU(sampleText, sizeof(sampleText)/sizeof(sampleText[0]),
                expected, sizeof(expected), "iso-2022-cn", UCNV_FROM_U_CALLBACK_SUBSTITUTE , NULL, TRUE))
            log_err("u-> not match.\n");
        if(!testConvertFromU(sampleText, sizeof(sampleText)/sizeof(sampleText[0]),
                expected, sizeof(expected), "iso-2022-cn", UCNV_FROM_U_CALLBACK_SUBSTITUTE,offsets , TRUE))
            log_err("u-> not match.\n");

        if(!testConvertToU(expected1, sizeof(expected1), 
                sampleText1, sizeof(sampleText1)/sizeof(sampleText1[0]), "iso-2022-cn",UCNV_TO_U_CALLBACK_SUBSTITUTE ,
                offsets1, TRUE))
           log_err("iso-2022-cn -> did not match.\n");
    }

        log_verbose("Testing Reset for ISO-2022-kr\n");
    {
        UChar    sampleText[] =   { 0x4e00,0xd801, 0xdc01, 0x04e01, 0x0031, 0xd801, 0xdc01, 0x0032};

        static const uint8_t expected[] = {0x1B, 0x24, 0x29, 0x43, 
                                    0x0E, 0x6C, 0x69, 
                                    0x0f, 0x1A, 
                                    0x0e, 0x6F, 0x4B, 
                                    0x0F, 0x31, 
                                    0x1A, 
                                    0x32 };        

        static const int32_t offsets[] = {-1, -1, -1, -1,
                              0, 0, 0, 
                              1, 1,  
                              3, 3, 3, 
                              4, 4, 
                              5, 
                              7,
                            };
        static const UChar    sampleText1[] =   { 0x4e00,0x0041, 0x04e01, 0x0031, 0x0042, 0x0032};

        static const uint8_t expected1[] = {0x1B, 0x24, 0x29, 0x43, 
                                    0x0E, 0x6C, 0x69, 
                                    0x0f, 0x41, 
                                    0x0e, 0x6F, 0x4B, 
                                    0x0F, 0x31, 
                                    0x42, 
                                    0x32 };        

        static const int32_t offsets1[] = {
                              5, 8, 10, 
                              13, 14, 15  
                      
                            };
        /*iso-2022-kr*/
        if(!testConvertFromU(sampleText, sizeof(sampleText)/sizeof(sampleText[0]),
                expected, sizeof(expected), "iso-2022-kr",  UCNV_FROM_U_CALLBACK_SUBSTITUTE , NULL, TRUE))
            log_err("u-> iso-2022-kr [UCNV_DBCS] not match.\n");
        if(!testConvertFromU(sampleText, sizeof(sampleText)/sizeof(sampleText[0]),
                expected, sizeof(expected), "iso-2022-kr",  UCNV_FROM_U_CALLBACK_SUBSTITUTE,offsets , TRUE))
            log_err("u-> iso-2022-kr [UCNV_DBCS] not match.\n");
        if(!testConvertToU(expected1, sizeof(expected1), 
                sampleText1, sizeof(sampleText1)/sizeof(sampleText1[0]), "iso-2022-kr",UCNV_TO_U_CALLBACK_SUBSTITUTE ,
                offsets1, TRUE))
           log_err("iso-2022-kr -> did not match.\n");
    }

        log_verbose("Testing Reset for HZ\n");
    {
        static const UChar    sampleText[] =   { 0x4e00, 0xd801, 0xdc01, 0x04e01, 0x0031, 0xd801, 0xdc01, 0x0032};

        static const uint8_t expected[] = {0x7E, 0x7B, 0x52, 0x3B,
                                    0x7E, 0x7D, 0x1A, 
                                    0x7E, 0x7B, 0x36, 0x21, 
                                    0x7E, 0x7D, 0x31, 
                                    0x1A,
                                    0x32 };


        static const int32_t offsets[] = {0,0,0,0,
                             1,1,1,
                             3,3,3,3,
                             4,4,4,
                             5,
                             7,};
        static const UChar    sampleText1[] =   { 0x4e00, 0x0035, 0x04e01, 0x0031, 0x0041, 0x0032};

        static const uint8_t expected1[] = {0x7E, 0x7B, 0x52, 0x3B,
                                    0x7E, 0x7D, 0x35, 
                                    0x7E, 0x7B, 0x36, 0x21, 
                                    0x7E, 0x7D, 0x31, 
                                    0x41,
                                    0x32 };


        static const int32_t offsets1[] = {2,6,9,13,14,15
                            };

        /*hz*/
        if(!testConvertFromU(sampleText, sizeof(sampleText)/sizeof(sampleText[0]),
                expected, sizeof(expected), "HZ", UCNV_FROM_U_CALLBACK_SUBSTITUTE,NULL , TRUE))
            log_err("u->  not match.\n");
        if(!testConvertFromU(sampleText, sizeof(sampleText)/sizeof(sampleText[0]),
                expected, sizeof(expected), "HZ", UCNV_FROM_U_CALLBACK_SUBSTITUTE,offsets , TRUE))
            log_err("u->  not match.\n");
        if(!testConvertToU(expected1, sizeof(expected1), 
                sampleText1, sizeof(sampleText1)/sizeof(sampleText1[0]), "hz",UCNV_TO_U_CALLBACK_SUBSTITUTE ,
                offsets1, TRUE))
           log_err("hz -> did not match.\n");
    }
#endif

    /*UTF-8*/
     log_verbose("Testing for UTF8\n");
    {
        static const UChar    sampleText[] =   { 0x4e00, 0x0701, 0x0031, 0xbfc1, 0xd801, 0xdc01, 0x0032};
        int32_t offsets[]={0x00, 0x00, 0x00, 0x01, 0x01, 0x02,
                           0x03, 0x03, 0x03, 0x04, 0x04, 0x04,
                           0x04, 0x06 };
        static const uint8_t expected[] = {0xe4, 0xb8, 0x80, 0xdc, 0x81, 0x31, 
            0xeb, 0xbf, 0x81, 0xF0, 0x90, 0x90, 0x81, 0x32};


        static const int32_t fromOffsets[] = { 0x0000, 0x0003, 0x0005, 0x0006, 0x0009, 0x0009, 0x000D }; 
        /*UTF-8*/
        if(!testConvertFromU(sampleText, sizeof(sampleText)/sizeof(sampleText[0]),
            expected, sizeof(expected), "UTF8", UCNV_FROM_U_CALLBACK_SUBSTITUTE,offsets , TRUE))
            log_err("u-> UTF8 with offsets and flush true did not match.\n");
        if(!testConvertFromU(sampleText, sizeof(sampleText)/sizeof(sampleText[0]),
            expected, sizeof(expected), "UTF8",  UCNV_FROM_U_CALLBACK_SUBSTITUTE,NULL , TRUE))
            log_err("u-> UTF8 with offsets and flush true did not match.\n");
        if(!testConvertFromU(sampleText, sizeof(sampleText)/sizeof(sampleText[0]),
            expected, sizeof(expected), "UTF8", UCNV_FROM_U_CALLBACK_SUBSTITUTE,offsets , TRUE))
            log_err("u-> UTF8 with offsets and flush true did not match.\n");
        if(!testConvertFromU(sampleText, sizeof(sampleText)/sizeof(sampleText[0]),
            expected, sizeof(expected), "UTF8",  UCNV_FROM_U_CALLBACK_SUBSTITUTE,NULL , TRUE))
            log_err("u-> UTF8 with offsets and flush true did not match.\n");
        if(!testConvertToU(expected, sizeof(expected), 
            sampleText, sizeof(sampleText)/sizeof(sampleText[0]), "UTF8",UCNV_TO_U_CALLBACK_SUBSTITUTE , NULL, TRUE))
            log_err("UTF8 -> did not match.\n");
        if(!testConvertToU(expected, sizeof(expected), 
            sampleText, sizeof(sampleText)/sizeof(sampleText[0]), "UTF8", UCNV_TO_U_CALLBACK_SUBSTITUTE , NULL, TRUE))
            log_err("UTF8 -> did not match.\n");
        if(!testConvertToU(expected, sizeof(expected), 
            sampleText, sizeof(sampleText)/sizeof(sampleText[0]), "UTF8",UCNV_TO_U_CALLBACK_SUBSTITUTE , fromOffsets, TRUE))
            log_err("UTF8 -> did not match.\n");
        if(!testConvertToU(expected, sizeof(expected), 
            sampleText, sizeof(sampleText)/sizeof(sampleText[0]), "UTF8", UCNV_TO_U_CALLBACK_SUBSTITUTE , fromOffsets, TRUE))
            log_err("UTF8 -> did not match.\n");

    }

}

/* Test that U_TRUNCATED_CHAR_FOUND is set. */
static void
doTestTruncated(const char *cnvName, const uint8_t *bytes, int32_t length) {
    UConverter *cnv;

    UChar buffer[2];
    UChar *target, *targetLimit;
    const char *source, *sourceLimit;

    UErrorCode errorCode;

    errorCode=U_ZERO_ERROR;
    cnv=ucnv_open(cnvName, &errorCode);
    if(U_FAILURE(errorCode)) {
        log_data_err("error TestTruncated: unable to open \"%s\" - %s\n", cnvName, u_errorName(errorCode));
        return;
    }
    ucnv_setToUCallBack(cnv, UCNV_TO_U_CALLBACK_STOP, NULL, NULL, NULL, &errorCode);
    if(U_FAILURE(errorCode)) {
        log_data_err("error TestTruncated: unable to set the stop callback on \"%s\" - %s\n",
                    cnvName, u_errorName(errorCode));
        ucnv_close(cnv);
        return;
    }

    source=(const char *)bytes;
    sourceLimit=source+length;
    target=buffer;
    targetLimit=buffer+LENGTHOF(buffer);

    /* 1. input bytes with flush=FALSE, then input nothing with flush=TRUE */
    ucnv_toUnicode(cnv, &target, targetLimit, &source, sourceLimit, NULL, FALSE, &errorCode);
    if(U_FAILURE(errorCode) || source!=sourceLimit || target!=buffer) {
        log_err("error TestTruncated(%s, 1a): input bytes[%d], flush=FALSE: %s, input left %d, output %d\n",
                cnvName, length, u_errorName(errorCode), (int)(sourceLimit-source), (int)(target-buffer));
    }

    errorCode=U_ZERO_ERROR;
    source=sourceLimit;
    target=buffer;
    ucnv_toUnicode(cnv, &target, targetLimit, &source, sourceLimit, NULL, TRUE, &errorCode);
    if(errorCode!=U_TRUNCATED_CHAR_FOUND || target!=buffer) {
        log_err("error TestTruncated(%s, 1b): no input (previously %d), flush=TRUE: %s (should be U_TRUNCATED_CHAR_FOUND), output %d\n",
                cnvName, (int)length, u_errorName(errorCode), (int)(target-buffer));
    }

    /* 2. input bytes with flush=TRUE */
    ucnv_resetToUnicode(cnv);

    errorCode=U_ZERO_ERROR;
    source=(const char *)bytes;
    target=buffer;
    ucnv_toUnicode(cnv, &target, targetLimit, &source, sourceLimit, NULL, TRUE, &errorCode);
    if(errorCode!=U_TRUNCATED_CHAR_FOUND || source!=sourceLimit || target!=buffer) {
        log_err("error TestTruncated(%s, 2): input bytes[%d], flush=TRUE: %s (should be U_TRUNCATED_CHAR_FOUND), input left %d, output %d\n",
                cnvName, length, u_errorName(errorCode), (int)(sourceLimit-source), (int)(target-buffer));
    }


    ucnv_close(cnv);
}

static void
TestTruncated() {
    static const struct {
        const char *cnvName;
        uint8_t bytes[8]; /* partial input bytes resulting in no output */
        int32_t length;
    } testCases[]={
        { "IMAP-mailbox-name",  { 0x26 }, 1 }, /* & */
        { "IMAP-mailbox-name",  { 0x26, 0x42 }, 2 }, /* &B */
        { "IMAP-mailbox-name",  { 0x26, 0x42, 0x42 }, 3 }, /* &BB */
        { "IMAP-mailbox-name",  { 0x26, 0x41, 0x41 }, 3 }, /* &AA */

        { "UTF-7",      { 0x2b, 0x42 }, 2 }, /* +B */
        { "UTF-8",      { 0xd1 }, 1 },

        { "UTF-16BE",   { 0x4e }, 1 },
        { "UTF-16LE",   { 0x4e }, 1 },
        { "UTF-16",     { 0x4e }, 1 },
        { "UTF-16",     { 0xff }, 1 },
        { "UTF-16",     { 0xfe, 0xff, 0x4e }, 3 },

        { "UTF-32BE",   { 0, 0, 0x4e }, 3 },
        { "UTF-32LE",   { 0x4e }, 1 },
        { "UTF-32",     { 0, 0, 0x4e }, 3 },
        { "UTF-32",     { 0xff }, 1 },
        { "UTF-32",     { 0, 0, 0xfe, 0xff, 0 }, 5 },
        { "SCSU",       { 0x0e, 0x4e }, 2 }, /* SQU 0x4e */

#if !UCONFIG_NO_LEGACY_CONVERSION
        { "BOCU-1",     { 0xd5 }, 1 },

        { "Shift-JIS",  { 0xe0 }, 1 },

        { "ibm-939",    { 0x0e, 0x41 }, 2 } /* SO 0x41 */
#else
        { "BOCU-1",     { 0xd5 }, 1 ,}
#endif
    };
    int32_t i;

    for(i=0; i<LENGTHOF(testCases); ++i) {
        doTestTruncated(testCases[i].cnvName, testCases[i].bytes, testCases[i].length);
    }
}

typedef struct NameRange {
    const char *name;
    UChar32 start, end, start2, end2, notStart, notEnd;
} NameRange;

static void
TestUnicodeSet() {
    UErrorCode errorCode;
    UConverter *cnv;
    USet *set;
    const char *name;
    int32_t i, count;

    static const char *const completeSetNames[]={
        "UTF-7",
        "UTF-8",
        "UTF-16",
        "UTF-16BE",
        "UTF-16LE",
        "UTF-32",
        "UTF-32BE",
        "UTF-32LE",
        "SCSU",
        "BOCU-1",
        "CESU-8",
#if !UCONFIG_NO_LEGACY_CONVERSION
        "gb18030",
#endif
        "IMAP-mailbox-name"
    };
#if !UCONFIG_NO_LEGACY_CONVERSION
    static const char *const lmbcsNames[]={
        "LMBCS-1",
        "LMBCS-2",
        "LMBCS-3",
        "LMBCS-4",
        "LMBCS-5",
        "LMBCS-6",
        "LMBCS-8",
        "LMBCS-11",
        "LMBCS-16",
        "LMBCS-17",
        "LMBCS-18",
        "LMBCS-19"
    };
#endif

    static const NameRange nameRanges[]={
        { "US-ASCII", 0, 0x7f, -1, -1, 0x80, 0x10ffff },
#if !UCONFIG_NO_LEGACY_CONVERSION
        { "ibm-367", 0, 0x7f, -1, -1, 0x80, 0x10ffff },
#endif
        { "ISO-8859-1", 0, 0x7f, -1, -1, 0x100, 0x10ffff },
#if !UCONFIG_NO_LEGACY_CONVERSION
        { "UTF-8", 0, 0xd7ff, 0xe000, 0x10ffff, 0xd800, 0xdfff },
        { "windows-1251", 0, 0x7f, 0x410, 0x44f, 0x3000, 0xd7ff },
        /* HZ test case fixed and moved to intltest's conversion.txt, ticket #6002 */
        { "shift-jis", 0x3041, 0x3093, 0x30a1, 0x30f3, 0x900, 0x1cff }
#else
        { "UTF-8", 0, 0xd7ff, 0xe000, 0x10ffff, 0xd800, 0xdfff }
#endif
    };

    /* open an empty set */
    set=uset_open(1, 0);

    count=ucnv_countAvailable();
    for(i=0; i<count; ++i) {
        errorCode=U_ZERO_ERROR;
        name=ucnv_getAvailableName(i);
        cnv=ucnv_open(name, &errorCode);
        if(U_FAILURE(errorCode)) {
            log_data_err("error: unable to open converter %s - %s\n",
                    name, u_errorName(errorCode));
            continue;
        }

        uset_clear(set);
        ucnv_getUnicodeSet(cnv, set, UCNV_ROUNDTRIP_SET, &errorCode);
        if(U_FAILURE(errorCode)) {
            log_err("error: ucnv_getUnicodeSet(%s) failed - %s\n",
                    name, u_errorName(errorCode));
        } else if(uset_size(set)==0) {
            log_err("error: ucnv_getUnicodeSet(%s) returns an empty set\n", name);
        }

        ucnv_close(cnv);
    }

    /* test converters that are known to convert all of Unicode (except maybe for surrogates) */
    for(i=0; i<LENGTHOF(completeSetNames); ++i) {
        errorCode=U_ZERO_ERROR;
        name=completeSetNames[i];
        cnv=ucnv_open(name, &errorCode);
        if(U_FAILURE(errorCode)) {
            log_data_err("error: unable to open converter %s - %s\n",
                    name, u_errorName(errorCode));
            continue;
        }

        uset_clear(set);
        ucnv_getUnicodeSet(cnv, set, UCNV_ROUNDTRIP_SET, &errorCode);
        if(U_FAILURE(errorCode)) {
            log_err("error: ucnv_getUnicodeSet(%s) failed - %s\n",
                    name, u_errorName(errorCode));
        } else if(!uset_containsRange(set, 0, 0xd7ff) || !uset_containsRange(set, 0xe000, 0x10ffff)) {
            log_err("error: ucnv_getUnicodeSet(%s) does not return an all-Unicode set\n", name);
        }

        ucnv_close(cnv);
    }

#if !UCONFIG_NO_LEGACY_CONVERSION
    /* test LMBCS variants which convert all of Unicode except for U+F6xx */
    for(i=0; i<LENGTHOF(lmbcsNames); ++i) {
        errorCode=U_ZERO_ERROR;
        name=lmbcsNames[i];
        cnv=ucnv_open(name, &errorCode);
        if(U_FAILURE(errorCode)) {
            log_data_err("error: unable to open converter %s - %s\n",
                    name, u_errorName(errorCode));
            continue;
        }

        uset_clear(set);
        ucnv_getUnicodeSet(cnv, set, UCNV_ROUNDTRIP_SET, &errorCode);
        if(U_FAILURE(errorCode)) {
            log_err("error: ucnv_getUnicodeSet(%s) failed - %s\n",
                    name, u_errorName(errorCode));
        } else if(!uset_containsRange(set, 0, 0xf5ff) || !uset_containsRange(set, 0xf700, 0x10ffff)) {
            log_err("error: ucnv_getUnicodeSet(%s) does not return an all-Unicode set (minus U+F6xx)\n", name);
        }

        ucnv_close(cnv);
    }
#endif

    /* test specific sets */
    for(i=0; i<LENGTHOF(nameRanges); ++i) {
        errorCode=U_ZERO_ERROR;
        name=nameRanges[i].name;
        cnv=ucnv_open(name, &errorCode);
        if(U_FAILURE(errorCode)) {
            log_data_err("error: unable to open converter %s - %s\n",
                         name, u_errorName(errorCode));
            continue;
        }

        uset_clear(set);
        ucnv_getUnicodeSet(cnv, set, UCNV_ROUNDTRIP_SET, &errorCode);
        if(U_FAILURE(errorCode)) {
            log_err("error: ucnv_getUnicodeSet(%s) failed - %s\n",
                    name, u_errorName(errorCode));
        } else if(
            !uset_containsRange(set, nameRanges[i].start, nameRanges[i].end) ||
            (nameRanges[i].start2>=0 && !uset_containsRange(set, nameRanges[i].start2, nameRanges[i].end2))
        ) {
            log_err("error: ucnv_getUnicodeSet(%s) does not contain the expected ranges\n", name);
        } else if(nameRanges[i].notStart>=0) {
            /* simulate containsAny() with the C API */
            uset_complement(set);
            if(!uset_containsRange(set, nameRanges[i].notStart, nameRanges[i].notEnd)) {
                log_err("error: ucnv_getUnicodeSet(%s) contains part of the unexpected range\n", name);
            }
        }

        ucnv_close(cnv);
    }

    errorCode = U_ZERO_ERROR;
    ucnv_getUnicodeSet(NULL, set, UCNV_ROUNDTRIP_SET, &errorCode);
    if (errorCode != U_ILLEGAL_ARGUMENT_ERROR) {
        log_err("error: ucnv_getUnicodeSet(NULL) returned wrong status code %s\n", u_errorName(errorCode));
    }
    errorCode = U_PARSE_ERROR;
    /* Make sure that it does nothing if an error is passed in. Difficult to proper test for. */
    ucnv_getUnicodeSet(NULL, NULL, UCNV_ROUNDTRIP_SET, &errorCode);
    if (errorCode != U_PARSE_ERROR) {
        log_err("error: ucnv_getUnicodeSet(NULL) returned wrong status code %s\n", u_errorName(errorCode));
    }

    uset_close(set);
}
