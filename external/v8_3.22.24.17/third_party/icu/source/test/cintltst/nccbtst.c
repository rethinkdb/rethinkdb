/********************************************************************
 * COPYRIGHT: 
 * Copyright (c) 1997-2010, International Business Machines Corporation and
 * others. All Rights Reserved.
 ********************************************************************/
/*
********************************************************************************
* File NCCBTST.C
*
* Modification History:
*        Name                            Description
*    Madhu Katragadda     7/21/1999      Testing error callback routines
********************************************************************************
*/
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include "cstring.h"
#include "unicode/uloc.h"
#include "unicode/ucnv.h"
#include "unicode/ucnv_err.h"
#include "cintltst.h"
#include "unicode/utypes.h"
#include "unicode/ustring.h"
#include "nccbtst.h"
#include "unicode/ucnv_cb.h"
#define NEW_MAX_BUFFER 999

#define nct_min(x,y)  ((x<y) ? x : y)
#define ARRAY_LENGTH(array) (sizeof(array)/sizeof((array)[0]))

static int32_t  gInBufferSize = 0;
static int32_t  gOutBufferSize = 0;
static char     gNuConvTestName[1024];

static void printSeq(const uint8_t* a, int len)
{
    int i=0;
    log_verbose("\n{");
    while (i<len)
        log_verbose("0x%02X, ", a[i++]);
    log_verbose("}\n");
}

static void printUSeq(const UChar* a, int len)
{
    int i=0;
    log_verbose("{");
    while (i<len)
        log_verbose("  0x%04x, ", a[i++]);
    log_verbose("}\n");
}

static void printSeqErr(const uint8_t* a, int len)
{
    int i=0;
    fprintf(stderr, "{");
    while (i<len)
        fprintf(stderr, "  0x%02x, ", a[i++]);
    fprintf(stderr, "}\n");
}

static void printUSeqErr(const UChar* a, int len)
{
    int i=0;
    fprintf(stderr, "{");
    while (i<len)
        fprintf(stderr, "0x%04x, ", a[i++]);
    fprintf(stderr,"}\n");
}

static void setNuConvTestName(const char *codepage, const char *direction)
{
    sprintf(gNuConvTestName, "[testing %s %s Unicode, InputBufSiz=%d, OutputBufSiz=%d]",
            codepage,
            direction,
            (int)gInBufferSize,
            (int)gOutBufferSize);
}


static void TestCallBackFailure(void);

void addTestConvertErrorCallBack(TestNode** root);

void addTestConvertErrorCallBack(TestNode** root)
{
    addTest(root, &TestSkipCallBack,  "tsconv/nccbtst/TestSkipCallBack");
    addTest(root, &TestStopCallBack,  "tsconv/nccbtst/TestStopCallBack");
    addTest(root, &TestSubCallBack,   "tsconv/nccbtst/TestSubCallBack");
    addTest(root, &TestSubWithValueCallBack, "tsconv/nccbtst/TestSubWithValueCallBack");

#if !UCONFIG_NO_LEGACY_CONVERSION
    addTest(root, &TestLegalAndOtherCallBack,  "tsconv/nccbtst/TestLegalAndOtherCallBack");
    addTest(root, &TestSingleByteCallBack,  "tsconv/nccbtst/TestSingleByteCallBack");
#endif

    addTest(root, &TestCallBackFailure,  "tsconv/nccbtst/TestCallBackFailure");
}

static void TestSkipCallBack()
{
    TestSkip(NEW_MAX_BUFFER, NEW_MAX_BUFFER);
    TestSkip(1,NEW_MAX_BUFFER);
    TestSkip(1,1);
    TestSkip(NEW_MAX_BUFFER, 1);
}

static void TestStopCallBack()
{
    TestStop(NEW_MAX_BUFFER, NEW_MAX_BUFFER);
    TestStop(1,NEW_MAX_BUFFER);
    TestStop(1,1);
    TestStop(NEW_MAX_BUFFER, 1);
}

static void TestSubCallBack()
{
    TestSub(NEW_MAX_BUFFER, NEW_MAX_BUFFER);
    TestSub(1,NEW_MAX_BUFFER);
    TestSub(1,1);
    TestSub(NEW_MAX_BUFFER, 1);

#if !UCONFIG_NO_LEGACY_CONVERSION
    TestEBCDIC_STATEFUL_Sub(1, 1);
    TestEBCDIC_STATEFUL_Sub(1, NEW_MAX_BUFFER);
    TestEBCDIC_STATEFUL_Sub(NEW_MAX_BUFFER, 1);
    TestEBCDIC_STATEFUL_Sub(NEW_MAX_BUFFER, NEW_MAX_BUFFER);
#endif
}

static void TestSubWithValueCallBack()
{
    TestSubWithValue(NEW_MAX_BUFFER, NEW_MAX_BUFFER);
    TestSubWithValue(1,NEW_MAX_BUFFER);
    TestSubWithValue(1,1);
    TestSubWithValue(NEW_MAX_BUFFER, 1);
}

#if !UCONFIG_NO_LEGACY_CONVERSION
static void TestLegalAndOtherCallBack()
{
    TestLegalAndOthers(NEW_MAX_BUFFER, NEW_MAX_BUFFER);
    TestLegalAndOthers(1,NEW_MAX_BUFFER);
    TestLegalAndOthers(1,1);
    TestLegalAndOthers(NEW_MAX_BUFFER, 1);
}

static void TestSingleByteCallBack()
{
    TestSingleByte(NEW_MAX_BUFFER, NEW_MAX_BUFFER);
    TestSingleByte(1,NEW_MAX_BUFFER);
    TestSingleByte(1,1);
    TestSingleByte(NEW_MAX_BUFFER, 1);
}
#endif

static void TestSkip(int32_t inputsize, int32_t outputsize)
{
    static const uint8_t expskipIBM_949[]= { 
        0x00, 0xb0, 0xa1, 0xb0, 0xa2, 0xc8, 0xd3 };

    static const uint8_t expskipIBM_943[] = { 
        0x9f, 0xaf, 0x9f, 0xb1, 0x89, 0x59 };

    static const uint8_t expskipIBM_930[] = { 
        0x0e, 0x5d, 0x5f, 0x5d, 0x63, 0x46, 0x6b, 0x0f };

    gInBufferSize = inputsize;
    gOutBufferSize = outputsize;

    /*From Unicode*/
    log_verbose("Testing fromUnicode with UCNV_FROM_U_CALLBACK_SKIP  \n");

#if !UCONFIG_NO_LEGACY_CONVERSION
    {
        static const UChar   sampleText[] =  { 0x0000, 0xAC00, 0xAC01, 0xEF67, 0xD700 };
        static const UChar  sampleText2[] =  { 0x6D63, 0x6D64, 0x6D65, 0x6D66 };

        static const int32_t  toIBM949Offsskip [] = { 0, 1, 1, 2, 2, 4, 4 };
        static const int32_t  toIBM943Offsskip [] = { 0, 0, 1, 1, 3, 3 };

        if(!testConvertFromUnicode(sampleText, sizeof(sampleText)/sizeof(sampleText[0]),
                expskipIBM_949, sizeof(expskipIBM_949), "ibm-949",
                UCNV_FROM_U_CALLBACK_SKIP, toIBM949Offsskip, NULL, 0 ))
            log_err("u-> ibm-949 with skip did not match.\n");
        if(!testConvertFromUnicode(sampleText2, sizeof(sampleText2)/sizeof(sampleText2[0]),
                expskipIBM_943, sizeof(expskipIBM_943), "ibm-943",
                UCNV_FROM_U_CALLBACK_SKIP, toIBM943Offsskip, NULL, 0 ))
            log_err("u-> ibm-943 with skip did not match.\n");
    }

    {
        static const UChar fromU[] = { 0x61, 0xff5e, 0x62, 0x6d63, 0xff5e, 0x6d64, 0x63, 0xff5e, 0x6d66 };
        static const uint8_t fromUBytes[] = { 0x62, 0x63, 0x0e, 0x5d, 0x5f, 0x5d, 0x63, 0x0f, 0x64, 0x0e, 0x46, 0x6b, 0x0f };
        static const int32_t fromUOffsets[] = { 0, 2, 3, 3, 3, 5, 5, 6, 6, 8, 8, 8, 8 };

        /* test ibm-930 (EBCDIC_STATEFUL) with fallbacks that are not taken to check correct state transitions */
        if(!testConvertFromUnicode(fromU, sizeof(fromU)/U_SIZEOF_UCHAR,
                                   fromUBytes, sizeof(fromUBytes),
                                   "ibm-930",
                                   UCNV_FROM_U_CALLBACK_SKIP, fromUOffsets,
                                   NULL, 0)
        ) {
            log_err("u->ibm-930 with skip with untaken fallbacks did not match.\n");
        }
    }
#endif

    {
        static const UChar usasciiFromU[] = { 0x61, 0x80, 0x4e00, 0x31, 0xd800, 0xdfff, 0x39 };
        static const uint8_t usasciiFromUBytes[] = { 0x61, 0x31, 0x39 };
        static const int32_t usasciiFromUOffsets[] = { 0, 3, 6 };

        static const UChar latin1FromU[] = { 0x61, 0xa0, 0x4e00, 0x31, 0xd800, 0xdfff, 0x39 };
        static const uint8_t latin1FromUBytes[] = { 0x61, 0xa0, 0x31, 0x39 };
        static const int32_t latin1FromUOffsets[] = { 0, 1, 3, 6 };

        /* US-ASCII */
        if(!testConvertFromUnicode(usasciiFromU, sizeof(usasciiFromU)/U_SIZEOF_UCHAR,
                                   usasciiFromUBytes, sizeof(usasciiFromUBytes),
                                   "US-ASCII",
                                   UCNV_FROM_U_CALLBACK_SKIP, usasciiFromUOffsets,
                                   NULL, 0)
        ) {
            log_err("u->US-ASCII with skip did not match.\n");
        }

#if !UCONFIG_NO_LEGACY_CONVERSION
        /* SBCS NLTC codepage 367 for US-ASCII */
        if(!testConvertFromUnicode(usasciiFromU, sizeof(usasciiFromU)/U_SIZEOF_UCHAR,
                                   usasciiFromUBytes, sizeof(usasciiFromUBytes),
                                   "ibm-367",
                                   UCNV_FROM_U_CALLBACK_SKIP, usasciiFromUOffsets,
                                   NULL, 0)
        ) {
            log_err("u->ibm-367 with skip did not match.\n");
        }
#endif

        /* ISO-Latin-1 */
        if(!testConvertFromUnicode(latin1FromU, sizeof(latin1FromU)/U_SIZEOF_UCHAR,
                                   latin1FromUBytes, sizeof(latin1FromUBytes),
                                   "LATIN_1",
                                   UCNV_FROM_U_CALLBACK_SKIP, latin1FromUOffsets,
                                   NULL, 0)
        ) {
            log_err("u->LATIN_1 with skip did not match.\n");
        }

#if !UCONFIG_NO_LEGACY_CONVERSION
        /* windows-1252 */
        if(!testConvertFromUnicode(latin1FromU, sizeof(latin1FromU)/U_SIZEOF_UCHAR,
                                   latin1FromUBytes, sizeof(latin1FromUBytes),
                                   "windows-1252",
                                   UCNV_FROM_U_CALLBACK_SKIP, latin1FromUOffsets,
                                   NULL, 0)
        ) {
            log_err("u->windows-1252 with skip did not match.\n");
        }
    }

    {
        static const UChar inputTest[] = { 0x0061, 0xd801, 0xdc01, 0xd801, 0x0061 };
        static const uint8_t toIBM943[]= { 0x61, 0x61 };
        static const int32_t offset[]= {0, 4};

         /* EUC_JP*/
        static const UChar euc_jp_inputText[]={ 0x0061, 0x4edd, 0x5bec, 0xd801, 0xdc01, 0xd801, 0x0061, 0x00a2 };
        static const uint8_t to_euc_jp[]={ 0x61, 0xa1, 0xb8, 0x8f, 0xf4, 0xae,
            0x61, 0x8e, 0xe0,
        };
        static const int32_t fromEUC_JPOffs [] ={ 0, 1, 1, 2, 2, 2, 6, 7, 7};

        /*EUC_TW*/
        static const UChar euc_tw_inputText[]={ 0x0061, 0x2295, 0x5BF2, 0xd801, 0xdc01, 0xd801, 0x0061, 0x8706, 0x8a, };
        static const uint8_t to_euc_tw[]={ 
            0x61, 0xa2, 0xd3, 0x8e, 0xa2, 0xdc, 0xe5,
            0x61, 0xe6, 0xca, 0x8a,
        };
        static const int32_t from_euc_twOffs [] ={ 0, 1, 1, 2, 2, 2, 2, 6, 7, 7, 8,};

        /*ISO-2022-JP*/
        static const UChar iso_2022_jp_inputText[]={0x0041, 0x00E9/*unassigned*/,0x0042, };
        static const uint8_t to_iso_2022_jp[]={ 
            0x41,       
            0x42,

        };
        static const int32_t from_iso_2022_jpOffs [] ={0,2};

        /*ISO-2022-JP*/
        UChar const iso_2022_jp_inputText2[]={0x0041, 0x00E9/*unassigned*/,0x43,0xd800/*illegal*/,0x0042, };
        static const uint8_t to_iso_2022_jp2[]={ 
            0x41,       
            0x43,

        };
        static const int32_t from_iso_2022_jpOffs2 [] ={0,2};

        /*ISO-2022-cn*/
        static const UChar iso_2022_cn_inputText[]={ 0x0041, 0x3712/*unassigned*/, 0x0042, };
        static const uint8_t to_iso_2022_cn[]={  
            0x41, 0x42
        };
        static const int32_t from_iso_2022_cnOffs [] ={ 
            0, 2
        };
        
        /*ISO-2022-CN*/
        static const UChar iso_2022_cn_inputText1[]={0x0041, 0x3712/*unassigned*/,0x43,0xd800/*illegal*/,0x0042, };
        static const uint8_t to_iso_2022_cn1[]={ 
            0x41, 0x43

        };
        static const int32_t from_iso_2022_cnOffs1 [] ={ 0, 2 };

        /*ISO-2022-kr*/
        static const UChar iso_2022_kr_inputText[]={ 0x0041, 0x03A0,0x3712/*unassigned*/,0x03A0, 0x0042, };
        static const uint8_t to_iso_2022_kr[]={  
            0x1b,   0x24,   0x29,   0x43,   
            0x41,   
            0x0e,   0x25,   0x50,   
            0x25,   0x50, 
            0x0f,   0x42, 
        };
        static const int32_t from_iso_2022_krOffs [] ={ 
            -1,-1,-1,-1,
            0,
            1,1,1,
            3,3,
            4,4
        };

        /*ISO-2022-kr*/
        static const UChar iso_2022_kr_inputText1[]={ 0x0041, 0x03A0,0x3712/*unassigned*/,0x03A0,0xd801/*illegal*/, 0x0042, };
        static const uint8_t to_iso_2022_kr1[]={  
            0x1b,   0x24,   0x29,   0x43,   
            0x41,   
            0x0e,   0x25,   0x50,   
            0x25,   0x50, 
 
        };
        static const int32_t from_iso_2022_krOffs1 [] ={ 
            -1,-1,-1,-1,
            0,
            1,1,1,
            3,3,

        };
        /* HZ encoding */       
        static const UChar hz_inputText[]={ 0x0041, 0x03A0,0x0662/*unassigned*/,0x03A0, 0x0042, };

        static const uint8_t to_hz[]={    
            0x7e,   0x7d,   0x41,   
            0x7e,   0x7b,   0x26,   0x30,   
            0x26,   0x30,
            0x7e,   0x7d,   0x42, 
           
        };
        static const int32_t from_hzOffs [] ={ 
            0,0,0,
            1,1,1,1,
            3,3,
            4,4,4,4
        };

        static const UChar hz_inputText1[]={ 0x0041, 0x03A0,0x0662/*unassigned*/,0x03A0,0xd801/*illegal*/, 0x0042, };

        static const uint8_t to_hz1[]={    
            0x7e,   0x7d,   0x41,   
            0x7e,   0x7b,   0x26,   0x30,   
            0x26,   0x30,

           
        };
        static const int32_t from_hzOffs1 [] ={ 
            0,0,0,
            1,1,1,1,
            3,3,

        };

#endif

        static const UChar SCSU_inputText[]={ 0x0041, 0xd801/*illegal*/, 0x0042, };

        static const uint8_t to_SCSU[]={    
            0x41,   
            0x42

           
        };
        static const int32_t from_SCSUOffs [] ={ 
            0,
            2,

        };

#if !UCONFIG_NO_LEGACY_CONVERSION
        /* ISCII */
        static const UChar iscii_inputText[]={ 0x0041, 0x3712/*unassigned*/, 0x0042, };
        static const uint8_t to_iscii[]={  
            0x41,   
            0x42, 
        };
        static const int32_t from_isciiOffs [] ={ 
            0,2,

        };
        /*ISCII*/
        static const UChar iscii_inputText1[]={0x0044, 0x3712/*unassigned*/,0x43,0xd800/*illegal*/,0x0042, };
        static const uint8_t to_iscii1[]={ 
            0x44,   
            0x43, 

        };
        static const int32_t from_isciiOffs1 [] ={0,2};

        if(!testConvertFromUnicode(inputTest, sizeof(inputTest)/sizeof(inputTest[0]),
                toIBM943, sizeof(toIBM943), "ibm-943",
                UCNV_FROM_U_CALLBACK_SKIP, offset, NULL, 0 ))
            log_err("u-> ibm-943 with skip did not match.\n");

        if(!testConvertFromUnicode(euc_jp_inputText, sizeof(euc_jp_inputText)/sizeof(euc_jp_inputText[0]),
                to_euc_jp, sizeof(to_euc_jp), "euc-jp",
                UCNV_FROM_U_CALLBACK_SKIP, fromEUC_JPOffs, NULL, 0 ))
            log_err("u-> euc-jp with skip did not match.\n");

        if(!testConvertFromUnicode(euc_tw_inputText, sizeof(euc_tw_inputText)/sizeof(euc_tw_inputText[0]),
                to_euc_tw, sizeof(to_euc_tw), "euc-tw",
                UCNV_FROM_U_CALLBACK_SKIP, from_euc_twOffs, NULL, 0 ))
            log_err("u-> euc-tw with skip did not match.\n");  
        
        /*iso_2022_jp*/
        if(!testConvertFromUnicode(iso_2022_jp_inputText, sizeof(iso_2022_jp_inputText)/sizeof(iso_2022_jp_inputText[0]),
                to_iso_2022_jp, sizeof(to_iso_2022_jp), "iso-2022-jp",
                UCNV_FROM_U_CALLBACK_SKIP, from_iso_2022_jpOffs, NULL, 0 ))
            log_err("u-> iso-2022-jp with skip did not match.\n"); 
        
        /* with context */
        if(!testConvertFromUnicodeWithContext(iso_2022_jp_inputText2, sizeof(iso_2022_jp_inputText2)/sizeof(iso_2022_jp_inputText2[0]),
                to_iso_2022_jp2, sizeof(to_iso_2022_jp2), "iso-2022-jp",
                UCNV_FROM_U_CALLBACK_SKIP, from_iso_2022_jpOffs2, NULL, 0,UCNV_SKIP_STOP_ON_ILLEGAL,U_ILLEGAL_CHAR_FOUND ))
            log_err("u-> iso-2022-jp with skip & UCNV_SKIP_STOP_ON_ILLEGAL did not match.\n"); 
    
        /*iso_2022_cn*/
        if(!testConvertFromUnicode(iso_2022_cn_inputText, sizeof(iso_2022_cn_inputText)/sizeof(iso_2022_cn_inputText[0]),
                to_iso_2022_cn, sizeof(to_iso_2022_cn), "iso-2022-cn",
                UCNV_FROM_U_CALLBACK_SKIP, from_iso_2022_cnOffs, NULL, 0 ))
            log_err("u-> iso-2022-cn with skip did not match.\n"); 
        /*with context*/
        if(!testConvertFromUnicodeWithContext(iso_2022_cn_inputText1, sizeof(iso_2022_cn_inputText1)/sizeof(iso_2022_cn_inputText1[0]),
                to_iso_2022_cn1, sizeof(to_iso_2022_cn1), "iso-2022-cn",
                UCNV_FROM_U_CALLBACK_SKIP, from_iso_2022_cnOffs1, NULL, 0,UCNV_SKIP_STOP_ON_ILLEGAL,U_ILLEGAL_CHAR_FOUND ))
            log_err("u-> iso-2022-cn with skip & UCNV_SKIP_STOP_ON_ILLEGAL did not match.\n"); 

        /*iso_2022_kr*/
        if(!testConvertFromUnicode(iso_2022_kr_inputText, sizeof(iso_2022_kr_inputText)/sizeof(iso_2022_kr_inputText[0]),
                to_iso_2022_kr, sizeof(to_iso_2022_kr), "iso-2022-kr",
                UCNV_FROM_U_CALLBACK_SKIP, from_iso_2022_krOffs, NULL, 0 ))
            log_err("u-> iso-2022-kr with skip did not match.\n"); 
          /*with context*/
        if(!testConvertFromUnicodeWithContext(iso_2022_kr_inputText1, sizeof(iso_2022_kr_inputText1)/sizeof(iso_2022_kr_inputText1[0]),
                to_iso_2022_kr1, sizeof(to_iso_2022_kr1), "iso-2022-kr",
                UCNV_FROM_U_CALLBACK_SKIP, from_iso_2022_krOffs1, NULL, 0,UCNV_SKIP_STOP_ON_ILLEGAL,U_ILLEGAL_CHAR_FOUND ))
            log_err("u-> iso-2022-kr with skip & UCNV_SKIP_STOP_ON_ILLEGAL did not match.\n"); 

        /*hz*/
        if(!testConvertFromUnicode(hz_inputText, sizeof(hz_inputText)/sizeof(hz_inputText[0]),
                to_hz, sizeof(to_hz), "HZ",
                UCNV_FROM_U_CALLBACK_SKIP, from_hzOffs, NULL, 0 ))
            log_err("u-> HZ with skip did not match.\n"); 
          /*with context*/
        if(!testConvertFromUnicodeWithContext(hz_inputText1, sizeof(hz_inputText1)/sizeof(hz_inputText1[0]),
                to_hz1, sizeof(to_hz1), "hz",
                UCNV_FROM_U_CALLBACK_SKIP, from_hzOffs1, NULL, 0,UCNV_SKIP_STOP_ON_ILLEGAL,U_ILLEGAL_CHAR_FOUND ))
            log_err("u-> hz with skip & UCNV_SKIP_STOP_ON_ILLEGAL did not match.\n");
#endif
        
        /*SCSU*/
        if(!testConvertFromUnicode(SCSU_inputText, sizeof(SCSU_inputText)/sizeof(SCSU_inputText[0]),
                to_SCSU, sizeof(to_SCSU), "SCSU",
                UCNV_FROM_U_CALLBACK_SKIP, from_SCSUOffs, NULL, 0 ))
            log_err("u-> SCSU with skip did not match.\n");

#if !UCONFIG_NO_LEGACY_CONVERSION
        /*ISCII*/
        if(!testConvertFromUnicode(iscii_inputText, sizeof(iscii_inputText)/sizeof(iscii_inputText[0]),
                to_iscii, sizeof(to_iscii), "ISCII,version=0",
                UCNV_FROM_U_CALLBACK_SKIP, from_isciiOffs, NULL, 0 ))
            log_err("u-> iscii with skip did not match.\n"); 
        /*with context*/
        if(!testConvertFromUnicodeWithContext(iscii_inputText1, sizeof(iscii_inputText1)/sizeof(iscii_inputText1[0]),
                to_iscii1, sizeof(to_iscii1), "ISCII,version=0",
                UCNV_FROM_U_CALLBACK_SKIP, from_isciiOffs1, NULL, 0,UCNV_SKIP_STOP_ON_ILLEGAL,U_ILLEGAL_CHAR_FOUND ))
            log_err("u-> iscii with skip & UCNV_SKIP_STOP_ON_ILLEGAL did not match.\n");
#endif     
    }

    log_verbose("Testing fromUnicode for BOCU-1 with UCNV_TO_U_CALLBACK_SKIP\n");
    {
        static const uint8_t sampleText[]={ /* from cintltst/bocu1tst.c/TestBOCU1 text 1 */
            0xFB, 0xEE, 0x28,       /* from source offset 0 */
            0x24, 0x1E, 0x52,
            0xB2,
            0x20,
            0xB3,
            0xB1,
            0x0D,
            0x0A,

            0x20,                   /* from 8 */
            0x00,
            0xD0, 0x6C,
            0xB6,
            0xD8, 0xA5,
            0x20,
            0x68,
            0x59,

            0xF9, 0x28,             /* from 16 */
            0x6D,
            0x20,
            0x73,
            0xE0, 0x2D,
            0xDE, 0x43,
            0xD0, 0x33,
            0x20,

            0xFA, 0x83,             /* from 24 */
            0x25, 0x01,
            0xFB, 0x16, 0x87,
            0x4B, 0x16,
            0x20,
            0xE6, 0xBD,
            0xEB, 0x5B,
            0x4B, 0xCC,

            0xF9, 0xA2,             /* from 32 */
            0xFC, 0x10, 0x3E,
            0xFE, 0x16, 0x3A, 0x8C,
            0x20,
            0xFC, 0x03, 0xAC,

            0x01,                   /* from 41 */
            0xDE, 0x83,
            0x20,
            0x09
        };
        static const UChar expected[]={
            0xFEFF, 0x0061, 0x0062, 0x0020, /* 0 */
            0x0063, 0x0061, 0x000D, 0x000A,

            0x0020, 0x0000, 0x00DF, 0x00E6, /* 8 */
            0x0930, 0x0020, 0x0918, 0x0909,

            0x3086, 0x304D, 0x0020, 0x3053, /* 16 */
            0x4000, 0x4E00, 0x7777, 0x0020,

            0x9FA5, 0x4E00, 0xAC00, 0xBCDE, /* 24 */
            0x0020, 0xD7A3, 0xDC00, 0xD800,

            0xD800, 0xDC00, 0xD845, 0xDDDD, /* 32 */
            0xDBBB, 0xDDEE, 0x0020, 0xDBFF,

            0xDFFF, 0x0001, 0x0E40, 0x0020, /* 40 */
            0x0009
        };
        static const int32_t offsets[]={
            0, 0, 0, 1, 1, 1, 2, 3, 4, 5, 6, 7,
            8, 9, 10, 10, 11, 12, 12, 13, 14, 15,
            16, 16, 17, 18, 19, 20, 20, 21, 21, 22, 22, 23,
            24, 24, 25, 25, 26, 26, 26, 27, 27, 28, 29, 29, 30, 30, 31, 31,
            32, 32, 34, 34, 34, 36, 36, 36, 36, 38, 39, 39, 39,
            41, 42, 42, 43, 44
        };

        /* BOCU-1 fromUnicode never calls callbacks, so this only tests single-byte and offsets behavior */
        if(!testConvertFromUnicode(expected, ARRAY_LENGTH(expected),
                                 sampleText, sizeof(sampleText),
                                 "BOCU-1",
                                 UCNV_FROM_U_CALLBACK_SKIP, offsets, NULL, 0)
        ) {
            log_err("u->BOCU-1 with skip did not match.\n");
        }
    }

    log_verbose("Testing fromUnicode for CESU-8 with UCNV_TO_U_CALLBACK_SKIP\n");
    {
        const uint8_t sampleText[]={
            0x61,                               /* 'a' */
            0xc4, 0xb5,                         /* U+0135 */
            0xed, 0x80, 0xa0,                   /* Hangul U+d020 */
            0xed, 0xa0, 0x81, 0xed, 0xb0, 0x81, /* surrogate pair for U+10401 */
            0xee, 0x80, 0x80,                   /* PUA U+e000 */
            0xed, 0xb0, 0x81,                   /* unpaired trail surrogate U+dc01 */
            0x62,                               /* 'b' */
            0xed, 0xa0, 0x81,                   /* unpaired lead surrogate U+d801 */
            0xd0, 0x80                          /* U+0400 */
        };
        UChar expected[]={
            0x0061,
            0x0135,
            0xd020,
            0xd801, 0xdc01,
            0xe000,
            0xdc01,
            0x0062,
            0xd801,
            0x0400
        };
        int32_t offsets[]={
            0,
            1, 1,
            2, 2, 2,
            3, 3, 3, 4, 4, 4,
            5, 5, 5,
            6, 6, 6,
            7,
            8, 8, 8,
            9, 9
        };

        /* CESU-8 fromUnicode never calls callbacks, so this only tests conversion and offsets behavior */

        /* without offsets */
        if(!testConvertFromUnicode(expected, ARRAY_LENGTH(expected),
                                 sampleText, sizeof(sampleText),
                                 "CESU-8",
                                 UCNV_FROM_U_CALLBACK_SKIP, NULL, NULL, 0)
        ) {
            log_err("u->CESU-8 with skip did not match.\n");
        }

        /* with offsets */
        if(!testConvertFromUnicode(expected, ARRAY_LENGTH(expected),
                                 sampleText, sizeof(sampleText),
                                 "CESU-8",
                                 UCNV_FROM_U_CALLBACK_SKIP, offsets, NULL, 0)
        ) {
            log_err("u->CESU-8 with skip did not match.\n");
        }
    }

    /*to Unicode*/
    log_verbose("Testing toUnicode with UCNV_TO_U_CALLBACK_SKIP  \n");

#if !UCONFIG_NO_LEGACY_CONVERSION
    {

        static const UChar IBM_949skiptoUnicode[]= {0x0000, 0xAC00, 0xAC01, 0xD700 };
        static const UChar IBM_943skiptoUnicode[]= { 0x6D63, 0x6D64, 0x6D66 };
        static const UChar IBM_930skiptoUnicode[]= { 0x6D63, 0x6D64, 0x6D66 };

        static const int32_t  fromIBM949Offs [] = { 0, 1, 3, 5};
        static const int32_t  fromIBM943Offs [] = { 0, 2, 4};
        static const int32_t  fromIBM930Offs [] = { 1, 3, 5};

        if(!testConvertToUnicode(expskipIBM_949, sizeof(expskipIBM_949),
                 IBM_949skiptoUnicode, sizeof(IBM_949skiptoUnicode)/sizeof(IBM_949skiptoUnicode),"ibm-949",
                UCNV_TO_U_CALLBACK_SKIP, fromIBM949Offs, NULL, 0 ))
            log_err("ibm-949->u with skip did not match.\n");
        if(!testConvertToUnicode(expskipIBM_943, sizeof(expskipIBM_943),
                 IBM_943skiptoUnicode, sizeof(IBM_943skiptoUnicode)/sizeof(IBM_943skiptoUnicode[0]),"ibm-943",
                UCNV_TO_U_CALLBACK_SKIP, fromIBM943Offs, NULL, 0 ))
            log_err("ibm-943->u with skip did not match.\n");


        if(!testConvertToUnicode(expskipIBM_930, sizeof(expskipIBM_930),
                 IBM_930skiptoUnicode, sizeof(IBM_930skiptoUnicode)/sizeof(IBM_930skiptoUnicode[0]),"ibm-930",
                UCNV_TO_U_CALLBACK_SKIP, fromIBM930Offs, NULL, 0 ))
            log_err("ibm-930->u with skip did not match.\n");

    
        if(!testConvertToUnicodeWithContext(expskipIBM_930, sizeof(expskipIBM_930),
                 IBM_930skiptoUnicode, sizeof(IBM_930skiptoUnicode)/sizeof(IBM_930skiptoUnicode[0]),"ibm-930",
                UCNV_TO_U_CALLBACK_SKIP, fromIBM930Offs, NULL, 0,"i",U_ILLEGAL_CHAR_FOUND ))
            log_err("ibm-930->u with skip did not match.\n");
    }
#endif

    {
        static const uint8_t usasciiToUBytes[] = { 0x61, 0x80, 0x31 };
        static const UChar usasciiToU[] = { 0x61, 0x31 };
        static const int32_t usasciiToUOffsets[] = { 0, 2 };

        static const uint8_t latin1ToUBytes[] = { 0x61, 0xa0, 0x31 };
        static const UChar latin1ToU[] = { 0x61, 0xa0, 0x31 };
        static const int32_t latin1ToUOffsets[] = { 0, 1, 2 };

        /* US-ASCII */
        if(!testConvertToUnicode(usasciiToUBytes, sizeof(usasciiToUBytes),
                                 usasciiToU, sizeof(usasciiToU)/U_SIZEOF_UCHAR,
                                 "US-ASCII",
                                 UCNV_TO_U_CALLBACK_SKIP, usasciiToUOffsets,
                                 NULL, 0)
        ) {
            log_err("US-ASCII->u with skip did not match.\n");
        }

#if !UCONFIG_NO_LEGACY_CONVERSION
        /* SBCS NLTC codepage 367 for US-ASCII */
        if(!testConvertToUnicode(usasciiToUBytes, sizeof(usasciiToUBytes),
                                 usasciiToU, sizeof(usasciiToU)/U_SIZEOF_UCHAR,
                                 "ibm-367",
                                 UCNV_TO_U_CALLBACK_SKIP, usasciiToUOffsets,
                                 NULL, 0)
        ) {
            log_err("ibm-367->u with skip did not match.\n");
        }
#endif

        /* ISO-Latin-1 */
        if(!testConvertToUnicode(latin1ToUBytes, sizeof(latin1ToUBytes),
                                 latin1ToU, sizeof(latin1ToU)/U_SIZEOF_UCHAR,
                                 "LATIN_1",
                                 UCNV_TO_U_CALLBACK_SKIP, latin1ToUOffsets,
                                 NULL, 0)
        ) {
            log_err("LATIN_1->u with skip did not match.\n");
        }

#if !UCONFIG_NO_LEGACY_CONVERSION
        /* windows-1252 */
        if(!testConvertToUnicode(latin1ToUBytes, sizeof(latin1ToUBytes),
                                 latin1ToU, sizeof(latin1ToU)/U_SIZEOF_UCHAR,
                                 "windows-1252",
                                 UCNV_TO_U_CALLBACK_SKIP, latin1ToUOffsets,
                                 NULL, 0)
        ) {
            log_err("windows-1252->u with skip did not match.\n");
        }
#endif
    }

#if !UCONFIG_NO_LEGACY_CONVERSION
    {
        static const uint8_t sampleTxtEBCIDIC_STATEFUL [] ={
            0x0e, 0x5d, 0x5f , 0x41, 0x79, 0x41, 0x44
        };
        static const UChar EBCIDIC_STATEFUL_toUnicode[] ={  0x6d63, 0x03b4 
        };
        static const int32_t from_EBCIDIC_STATEFULOffsets []={ 1, 5};
       

         /* euc-jp*/
        static const uint8_t sampleTxt_euc_jp[]={ 0x61, 0xa1, 0xb8, 0x8f, 0xf4, 0xae,
            0x8f, 0xda, 0xa1,  /*unassigned*/
           0x8e, 0xe0,
        };
        static const UChar euc_jptoUnicode[]={ 0x0061, 0x4edd, 0x5bec, 0x00a2};
        static const int32_t from_euc_jpOffs [] ={ 0, 1, 3, 9};

         /*EUC_TW*/
        static const uint8_t sampleTxt_euc_tw[]={ 0x61, 0xa2, 0xd3, 0x8e, 0xa2, 0xdc, 0xe5,
            0x8e, 0xaa, 0xbb, 0xcc,/*unassigned*/
           0xe6, 0xca, 0x8a,
        };
        static const UChar euc_twtoUnicode[]={ 0x0061, 0x2295, 0x5BF2, 0x8706, 0x8a, };
        static const int32_t from_euc_twOffs [] ={ 0, 1, 3, 11, 13};
                /*iso-2022-jp*/
        static const uint8_t sampleTxt_iso_2022_jp[]={ 
            0x41,
            0x1b,   0x24,   0x42,   0x2A, 0x44, /*unassigned*/
             0x1b,   0x28,   0x42,   0x42,
            
        };
        static const UChar iso_2022_jptoUnicode[]={    0x41,0x42 };
        static const int32_t from_iso_2022_jpOffs [] ={  0,9   };
        
        /*iso-2022-cn*/
        static const uint8_t sampleTxt_iso_2022_cn[]={ 
            0x0f,   0x41,   0x44,
            0x1B,   0x24,   0x29,   0x47, 
            0x0E,   0x40,   0x6f, /*unassigned*/
            0x0f,   0x42,
            
        };

        static const UChar iso_2022_cntoUnicode[]={    0x41, 0x44,0x42 };
        static const int32_t from_iso_2022_cnOffs [] ={  1,   2,   11   };

        /*iso-2022-kr*/
        static const uint8_t sampleTxt_iso_2022_kr[]={ 
          0x1b, 0x24, 0x29,  0x43,
          0x41,
          0x0E, 0x7f, 0x1E,
          0x0e, 0x25, 0x50, 
          0x0f, 0x51,
          0x42, 0x43,
            
        };
        static const UChar iso_2022_krtoUnicode[]={     0x41,0x03A0,0x51, 0x42,0x43};
        static const int32_t from_iso_2022_krOffs [] ={  4,    9,    12,   13  , 14 };
        
        /*hz*/
        static const uint8_t sampleTxt_hz[]={ 
            0x41,   
            0x7e,   0x7b,   0x26,   0x30,   
            0x7f,   0x1E, /*unassigned*/ 
            0x26,   0x30,
            0x7e,   0x7d,   0x42, 
            0x7e,   0x7b,   0x7f,   0x1E,/*unassigned*/ 
            0x7e,   0x7d,   0x42,
        };
        static const UChar hztoUnicode[]={  
            0x41,
            0x03a0,
            0x03A0, 
            0x42,
            0x42,};

        static const int32_t from_hzOffs [] ={0,3,7,11,18,  };

        /*ISCII*/
        static const uint8_t sampleTxt_iscii[]={ 
            0x41,   
            0xa1,   
            0xEB,    /*unassigned*/ 
            0x26,   
            0x30,
            0xa2, 
            0xEC,    /*unassigned*/ 
            0x42,
        };
        static const UChar isciitoUnicode[]={  
            0x41,
            0x0901,
            0x26,
            0x30,
            0x0902, 
            0x42,
            };

        static const int32_t from_isciiOffs [] ={0,1,3,4,5,7 };

        /*LMBCS*/
        static const uint8_t sampleTxtLMBCS[]={ 0x12, 0xc9, 0x50, 
            0x12, 0x92, 0xa0, /*unassigned*/
            0x12, 0x92, 0xA1,
        };
        static const UChar LMBCSToUnicode[]={ 0x4e2e, 0xe5c4};
        static const int32_t fromLMBCS[] = {0, 6};
 
        if(!testConvertToUnicode(sampleTxtEBCIDIC_STATEFUL, sizeof(sampleTxtEBCIDIC_STATEFUL),
             EBCIDIC_STATEFUL_toUnicode, sizeof(EBCIDIC_STATEFUL_toUnicode)/sizeof(EBCIDIC_STATEFUL_toUnicode[0]),"ibm-930",
            UCNV_TO_U_CALLBACK_SKIP, from_EBCIDIC_STATEFULOffsets, NULL, 0 ))
        log_err("EBCIDIC_STATEFUL->u with skip did not match.\n");

        if(!testConvertToUnicodeWithContext(sampleTxtEBCIDIC_STATEFUL, sizeof(sampleTxtEBCIDIC_STATEFUL),
             EBCIDIC_STATEFUL_toUnicode, sizeof(EBCIDIC_STATEFUL_toUnicode)/sizeof(EBCIDIC_STATEFUL_toUnicode[0]),"ibm-930",
            UCNV_TO_U_CALLBACK_SKIP, from_EBCIDIC_STATEFULOffsets, NULL, 0,"i",U_ILLEGAL_CHAR_FOUND ))
        log_err("EBCIDIC_STATEFUL->u with skip did not match.\n");

        if(!testConvertToUnicode(sampleTxt_euc_jp, sizeof(sampleTxt_euc_jp),
                 euc_jptoUnicode, sizeof(euc_jptoUnicode)/sizeof(euc_jptoUnicode[0]),"euc-jp",
                UCNV_TO_U_CALLBACK_SKIP, from_euc_jpOffs , NULL, 0))
            log_err("euc-jp->u with skip did not match.\n");



        if(!testConvertToUnicode(sampleTxt_euc_tw, sizeof(sampleTxt_euc_tw),
                 euc_twtoUnicode, sizeof(euc_twtoUnicode)/sizeof(euc_twtoUnicode[0]),"euc-tw",
                UCNV_TO_U_CALLBACK_SKIP, from_euc_twOffs , NULL, 0))
            log_err("euc-tw->u with skip did not match.\n");

        
        if(!testConvertToUnicode(sampleTxt_iso_2022_jp, sizeof(sampleTxt_iso_2022_jp),
                 iso_2022_jptoUnicode, sizeof(iso_2022_jptoUnicode)/sizeof(iso_2022_jptoUnicode[0]),"iso-2022-jp",
                UCNV_TO_U_CALLBACK_SKIP, from_iso_2022_jpOffs , NULL, 0))
            log_err("iso-2022-jp->u with skip did not match.\n");
        
        if(!testConvertToUnicode(sampleTxt_iso_2022_cn, sizeof(sampleTxt_iso_2022_cn),
                 iso_2022_cntoUnicode, sizeof(iso_2022_cntoUnicode)/sizeof(iso_2022_cntoUnicode[0]),"iso-2022-cn",
                UCNV_TO_U_CALLBACK_SKIP, from_iso_2022_cnOffs , NULL, 0))
            log_err("iso-2022-cn->u with skip did not match.\n");

        if(!testConvertToUnicode(sampleTxt_iso_2022_kr, sizeof(sampleTxt_iso_2022_kr),
                 iso_2022_krtoUnicode, sizeof(iso_2022_krtoUnicode)/sizeof(iso_2022_krtoUnicode[0]),"iso-2022-kr",
                UCNV_TO_U_CALLBACK_SKIP, from_iso_2022_krOffs , NULL, 0))
            log_err("iso-2022-kr->u with skip did not match.\n");

        if(!testConvertToUnicode(sampleTxt_hz, sizeof(sampleTxt_hz),
                 hztoUnicode, sizeof(hztoUnicode)/sizeof(hztoUnicode[0]),"HZ",
                UCNV_TO_U_CALLBACK_SKIP, from_hzOffs , NULL, 0))
            log_err("HZ->u with skip did not match.\n");
        
        if(!testConvertToUnicode(sampleTxt_iscii, sizeof(sampleTxt_iscii),
                 isciitoUnicode, sizeof(isciitoUnicode)/sizeof(isciitoUnicode[0]),"ISCII,version=0",
                UCNV_TO_U_CALLBACK_SKIP, from_isciiOffs , NULL, 0))
            log_err("iscii->u with skip did not match.\n");

        if(!testConvertToUnicode(sampleTxtLMBCS, sizeof(sampleTxtLMBCS),
                LMBCSToUnicode, sizeof(LMBCSToUnicode)/sizeof(LMBCSToUnicode[0]),"LMBCS-1",
                UCNV_TO_U_CALLBACK_SKIP, fromLMBCS , NULL, 0))
            log_err("LMBCS->u with skip did not match.\n");

    }
#endif

    log_verbose("Testing to Unicode for UTF-8 with UCNV_TO_U_CALLBACK_SKIP \n");
    {
        const uint8_t sampleText1[] = { 0x31, 0xe4, 0xba, 0x8c, 
            0xe0, 0x80,  0x61,};
        UChar    expected1[] = {  0x0031, 0x4e8c, 0x0061};
        int32_t offsets1[] = {   0x0000, 0x0001, 0x0006};

        if(!testConvertToUnicode(sampleText1, sizeof(sampleText1),
                 expected1, sizeof(expected1)/sizeof(expected1[0]),"utf8",
                UCNV_TO_U_CALLBACK_SKIP, offsets1, NULL, 0 ))
            log_err("utf8->u with skip did not match.\n");;
    }

    log_verbose("Testing toUnicode for SCSU with UCNV_TO_U_CALLBACK_SKIP \n");
    {
        const uint8_t sampleText1[] = {  0xba, 0x8c,0xF8, 0x61,0x0c, 0x0c,};
        UChar    expected1[] = {  0x00ba,  0x008c,  0x00f8,  0x0061,0xfffe,0xfffe};
        int32_t offsets1[] = {   0x0000, 0x0001,0x0002,0x0003,4,5};

        if(!testConvertToUnicode(sampleText1, sizeof(sampleText1),
                 expected1, sizeof(expected1)/sizeof(expected1[0]),"SCSU",
                UCNV_TO_U_CALLBACK_SKIP, offsets1, NULL, 0 ))
            log_err("scsu->u with skip did not match.\n");
    }

    log_verbose("Testing toUnicode for BOCU-1 with UCNV_TO_U_CALLBACK_SKIP\n");
    {
        const uint8_t sampleText[]={ /* modified from cintltst/bocu1tst.c/TestBOCU1 text 1 */
            0xFB, 0xEE, 0x28,       /* single-code point sequence at offset 0 */
            0x24, 0x1E, 0x52,       /* 3 */
            0xB2,                   /* 6 */
            0x20,                   /* 7 */
            0x40, 0x07,             /* 8 - wrong trail byte */
            0xB3,                   /* 10 */
            0xB1,                   /* 11 */
            0xD0, 0x20,             /* 12 - wrong trail byte */
            0x0D,                   /* 14 */
            0x0A,                   /* 15 */
            0x20,                   /* 16 */
            0x00,                   /* 17 */
            0xD0, 0x6C,             /* 18 */
            0xB6,                   /* 20 */
            0xD8, 0xA5,             /* 21 */
            0x20,                   /* 23 */
            0x68,                   /* 24 */
            0x59,                   /* 25 */
            0xF9, 0x28,             /* 26 */
            0x6D,                   /* 28 */
            0x20,                   /* 29 */
            0x73,                   /* 30 */
            0xE0, 0x2D,             /* 31 */
            0xDE, 0x43,             /* 33 */
            0xD0, 0x33,             /* 35 */
            0x20,                   /* 37 */
            0xFA, 0x83,             /* 38 */
            0x25, 0x01,             /* 40 */
            0xFB, 0x16, 0x87,       /* 42 */
            0x4B, 0x16,             /* 45 */
            0x20,                   /* 47 */
            0xE6, 0xBD,             /* 48 */
            0xEB, 0x5B,             /* 50 */
            0x4B, 0xCC,             /* 52 */
            0xF9, 0xA2,             /* 54 */
            0xFC, 0x10, 0x3E,       /* 56 */
            0xFE, 0x16, 0x3A, 0x8C, /* 59 */
            0x20,                   /* 63 */
            0xFC, 0x03, 0xAC,       /* 64 */
            0xFF,                   /* 67 - FF just resets the state without encoding anything */
            0x01,                   /* 68 */
            0xDE, 0x83,             /* 69 */
            0x20,                   /* 71 */
            0x09                    /* 72 */
        };
        UChar expected[]={
            0xFEFF, 0x0061, 0x0062, 0x0020,
            0x0063, 0x0061, 0x000D, 0x000A,
            0x0020, 0x0000, 0x00DF, 0x00E6,
            0x0930, 0x0020, 0x0918, 0x0909,
            0x3086, 0x304D, 0x0020, 0x3053,
            0x4000, 0x4E00, 0x7777, 0x0020,
            0x9FA5, 0x4E00, 0xAC00, 0xBCDE,
            0x0020, 0xD7A3, 0xDC00, 0xD800,
            0xD800, 0xDC00, 0xD845, 0xDDDD,
            0xDBBB, 0xDDEE, 0x0020, 0xDBFF,
            0xDFFF, 0x0001, 0x0E40, 0x0020,
            0x0009
        };
        int32_t offsets[]={
            0, 3, 6, 7, /* skip 8, */
            10, 11, /* skip 12, */
            14, 15, 16, 17, 18,
            20, 21, 23, 24, 25, 26, 28, 29,
            30, 31, 33, 35, 37, 38,
            40, 42, 45, 47, 48,
            50, 52, 54, /* trail */ 54, 56, /* trail */ 56, 59, /* trail */ 59,
            63, 64, /* trail */ 64, /* reset only 67, */
            68, 69,
            71, 72
        };

        if(!testConvertToUnicode(sampleText, sizeof(sampleText),
                                 expected, ARRAY_LENGTH(expected), "BOCU-1",
                                 UCNV_TO_U_CALLBACK_SKIP, offsets, NULL, 0)
        ) {
            log_err("BOCU-1->u with skip did not match.\n");
        }
    }

    log_verbose("Testing toUnicode for CESU-8 with UCNV_TO_U_CALLBACK_SKIP\n");
    {
        const uint8_t sampleText[]={
            0x61,                               /* 0  'a' */
            0xc0, 0x80,                         /* 1  non-shortest form */
            0xc4, 0xb5,                         /* 3  U+0135 */
            0xed, 0x80, 0xa0,                   /* 5  Hangul U+d020 */
            0xed, 0xa0, 0x81, 0xed, 0xb0, 0x81, /* 8  surrogate pair for U+10401 */
            0xee, 0x80, 0x80,                   /* 14 PUA U+e000 */
            0xed, 0xb0, 0x81,                   /* 17 unpaired trail surrogate U+dc01 */
            0xf0, 0x90, 0x80, 0x80,             /* 20 illegal 4-byte form for U+10000 */
            0x62,                               /* 24 'b' */
            0xed, 0xa0, 0x81,                   /* 25 unpaired lead surrogate U+d801 */
            0xed, 0xa0,                         /* 28 incomplete sequence */
            0xd0, 0x80                          /* 30 U+0400 */
        };
        UChar expected[]={
            0x0061,
            /* skip */
            0x0135,
            0xd020,
            0xd801, 0xdc01,
            0xe000,
            0xdc01,
            /* skip */
            0x0062,
            0xd801,
            0x0400
        };
        int32_t offsets[]={
            0,
            /* skip 1, */
            3,
            5,
            8, 11,
            14,
            17,
            /* skip 20, 20, */
            24,
            25,
            /* skip 28 */
            30
        };

        /* without offsets */
        if(!testConvertToUnicode(sampleText, sizeof(sampleText),
                                 expected, ARRAY_LENGTH(expected), "CESU-8",
                                 UCNV_TO_U_CALLBACK_SKIP, NULL, NULL, 0)
        ) {
            log_err("CESU-8->u with skip did not match.\n");
        }

        /* with offsets */
        if(!testConvertToUnicode(sampleText, sizeof(sampleText),
                                 expected, ARRAY_LENGTH(expected), "CESU-8",
                                 UCNV_TO_U_CALLBACK_SKIP, offsets, NULL, 0)
        ) {
            log_err("CESU-8->u with skip did not match.\n");
        }
    }
}

static void TestStop(int32_t inputsize, int32_t outputsize)
{
    static const UChar   sampleText[] =  { 0x0000, 0xAC00, 0xAC01, 0xEF67, 0xD700 };
    static const UChar  sampleText2[] =  { 0x6D63, 0x6D64, 0x6D65, 0x6D66 };

    static const uint8_t expstopIBM_949[]= { 
        0x00, 0xb0, 0xa1, 0xb0, 0xa2};

    static const uint8_t expstopIBM_943[] = { 
        0x9f, 0xaf, 0x9f, 0xb1};

    static const uint8_t expstopIBM_930[] = { 
        0x0e, 0x5d, 0x5f, 0x5d, 0x63};

    static const UChar IBM_949stoptoUnicode[]= {0x0000, 0xAC00, 0xAC01};
    static const UChar IBM_943stoptoUnicode[]= { 0x6D63, 0x6D64};
    static const UChar IBM_930stoptoUnicode[]= { 0x6D63, 0x6D64};


    static const int32_t  toIBM949Offsstop [] = { 0, 1, 1, 2, 2};
    static const int32_t  toIBM943Offsstop [] = { 0, 0, 1, 1};
    static const int32_t  toIBM930Offsstop [] = { 0, 0, 0, 1, 1};

    static const int32_t  fromIBM949Offs [] = { 0, 1, 3};
    static const int32_t  fromIBM943Offs [] = { 0, 2};
    static const int32_t  fromIBM930Offs [] = { 1, 3};

    gInBufferSize = inputsize;
    gOutBufferSize = outputsize;

    /*From Unicode*/

#if !UCONFIG_NO_LEGACY_CONVERSION
    if(!testConvertFromUnicode(sampleText, sizeof(sampleText)/sizeof(sampleText[0]),
            expstopIBM_949, sizeof(expstopIBM_949), "ibm-949",
            UCNV_FROM_U_CALLBACK_STOP, toIBM949Offsstop, NULL, 0 ))
        log_err("u-> ibm-949 with stop did not match.\n");
    if(!testConvertFromUnicode(sampleText2, sizeof(sampleText2)/sizeof(sampleText2[0]),
            expstopIBM_943, sizeof(expstopIBM_943), "ibm-943",
            UCNV_FROM_U_CALLBACK_STOP, toIBM943Offsstop , NULL, 0))
        log_err("u-> ibm-943 with stop did not match.\n");
    if(!testConvertFromUnicode(sampleText2, sizeof(sampleText2)/sizeof(sampleText2[0]),
            expstopIBM_930, sizeof(expstopIBM_930), "ibm-930",
            UCNV_FROM_U_CALLBACK_STOP, toIBM930Offsstop, NULL, 0 ))
        log_err("u-> ibm-930 with stop did not match.\n");

    log_verbose("Testing fromUnicode with UCNV_FROM_U_CALLBACK_STOP  \n");
    {
        static const UChar inputTest[] = { 0x0061, 0xd801, 0xdc01, 0xd801, 0x0061 };
        static const uint8_t toIBM943[]= { 0x61,};
        static const int32_t offset[]= {0,} ;

         /*EUC_JP*/
        static const UChar euc_jp_inputText[]={ 0x0061, 0x4edd, 0x5bec, 0xd801, 0xdc01, 0xd801, 0x0061, 0x00a2 };
        static const uint8_t to_euc_jp[]={ 0x61, 0xa1, 0xb8, 0x8f, 0xf4, 0xae,};
        static const int32_t fromEUC_JPOffs [] ={ 0, 1, 1, 2, 2, 2,};

        /*EUC_TW*/
        static const UChar euc_tw_inputText[]={ 0x0061, 0x2295, 0x5BF2, 0xd801, 0xdc01, 0xd801, 0x0061, 0x8706, 0x8a, };
        static const uint8_t to_euc_tw[]={ 
            0x61, 0xa2, 0xd3, 0x8e, 0xa2, 0xdc, 0xe5,};
        static const int32_t from_euc_twOffs [] ={ 0, 1, 1, 2, 2, 2, 2,};
        
        /*ISO-2022-JP*/
        static const UChar iso_2022_jp_inputText[]={0x0041, 0x00E9, 0x0042, };
        static const uint8_t to_iso_2022_jp[]={ 
             0x41,   

        };
        static const int32_t from_iso_2022_jpOffs [] ={0,};

        /*ISO-2022-cn*/
        static const UChar iso_2022_cn_inputText[]={ 0x0041, 0x3712, 0x0042, };
        static const uint8_t to_iso_2022_cn[]={  
            0x41,   

        };
        static const int32_t from_iso_2022_cnOffs [] ={ 
            0,0,
            2,2,
        };
        
        /*ISO-2022-kr*/
        static const UChar iso_2022_kr_inputText[]={ 0x0041, 0x03A0,0x3712/*unassigned*/,0x03A0, 0x0042, };
        static const uint8_t to_iso_2022_kr[]={  
            0x1b,   0x24,   0x29,   0x43,   
            0x41,   
            0x0e,   0x25,   0x50,   
        };
        static const int32_t from_iso_2022_krOffs [] ={ 
            -1,-1,-1,-1,
             0,
            1,1,1,
        };

        /* HZ encoding */       
        static const UChar hz_inputText[]={ 0x0041, 0x03A0,0x0662/*unassigned*/,0x03A0, 0x0042, };

        static const uint8_t to_hz[]={    
            0x7e,   0x7d, 0x41,   
            0x7e,   0x7b,   0x26,   0x30,   
           
        };
        static const int32_t from_hzOffs [] ={ 
            0, 0,0,
            1,1,1,1,
        };

        /*ISCII*/
        static const UChar iscii_inputText[]={ 0x0041, 0x3712, 0x0042, };
        static const uint8_t to_iscii[]={  
            0x41,   
        };
        static const int32_t from_isciiOffs [] ={ 
            0,           
        };

        if(!testConvertFromUnicode(inputTest, sizeof(inputTest)/sizeof(inputTest[0]),
                toIBM943, sizeof(toIBM943), "ibm-943",
                UCNV_FROM_U_CALLBACK_STOP, offset, NULL, 0 ))
            log_err("u-> ibm-943 with stop did not match.\n");

        if(!testConvertFromUnicode(euc_jp_inputText, sizeof(euc_jp_inputText)/sizeof(euc_jp_inputText[0]),
                to_euc_jp, sizeof(to_euc_jp), "euc-jp",
                UCNV_FROM_U_CALLBACK_STOP, fromEUC_JPOffs, NULL, 0 ))
            log_err("u-> euc-jp with stop did not match.\n");

        if(!testConvertFromUnicode(euc_tw_inputText, sizeof(euc_tw_inputText)/sizeof(euc_tw_inputText[0]),
                to_euc_tw, sizeof(to_euc_tw), "euc-tw",
                UCNV_FROM_U_CALLBACK_STOP, from_euc_twOffs, NULL, 0 ))
            log_err("u-> euc-tw with stop did not match.\n");  

        if(!testConvertFromUnicode(iso_2022_jp_inputText, sizeof(iso_2022_jp_inputText)/sizeof(iso_2022_jp_inputText[0]),
                to_iso_2022_jp, sizeof(to_iso_2022_jp), "iso-2022-jp",
                UCNV_FROM_U_CALLBACK_STOP, from_iso_2022_jpOffs, NULL, 0 ))
            log_err("u-> iso-2022-jp with stop did not match.\n");  

        if(!testConvertFromUnicode(iso_2022_jp_inputText, sizeof(iso_2022_jp_inputText)/sizeof(iso_2022_jp_inputText[0]),
                to_iso_2022_jp, sizeof(to_iso_2022_jp), "iso-2022-jp",
                UCNV_FROM_U_CALLBACK_STOP, from_iso_2022_jpOffs, NULL, 0 ))
            log_err("u-> iso-2022-jp with stop did not match.\n");  
        
        if(!testConvertFromUnicode(iso_2022_cn_inputText, sizeof(iso_2022_cn_inputText)/sizeof(iso_2022_cn_inputText[0]),
                to_iso_2022_cn, sizeof(to_iso_2022_cn), "iso-2022-cn",
                UCNV_FROM_U_CALLBACK_STOP, from_iso_2022_cnOffs, NULL, 0 ))
            log_err("u-> iso-2022-cn with stop did not match.\n");  

        if(!testConvertFromUnicode(iso_2022_kr_inputText, sizeof(iso_2022_kr_inputText)/sizeof(iso_2022_kr_inputText[0]),
                to_iso_2022_kr, sizeof(to_iso_2022_kr), "iso-2022-kr",
                UCNV_FROM_U_CALLBACK_STOP, from_iso_2022_krOffs, NULL, 0 ))
            log_err("u-> iso-2022-kr with stop did not match.\n");  
        
        if(!testConvertFromUnicode(hz_inputText, sizeof(hz_inputText)/sizeof(hz_inputText[0]),
                to_hz, sizeof(to_hz), "HZ",
                UCNV_FROM_U_CALLBACK_STOP, from_hzOffs, NULL, 0 ))
            log_err("u-> HZ with stop did not match.\n");\
                
        if(!testConvertFromUnicode(iscii_inputText, sizeof(iscii_inputText)/sizeof(iscii_inputText[0]),
                to_iscii, sizeof(to_iscii), "ISCII,version=0",
                UCNV_FROM_U_CALLBACK_STOP, from_isciiOffs, NULL, 0 ))
            log_err("u-> iscii with stop did not match.\n"); 


    }
#endif

    log_verbose("Testing fromUnicode for SCSU with UCNV_FROM_U_CALLBACK_STOP \n");
    {
        static const UChar SCSU_inputText[]={ 0x0041, 0xd801/*illegal*/, 0x0042, };

        static const uint8_t to_SCSU[]={    
            0x41,   
           
        };
        int32_t from_SCSUOffs [] ={ 
            0,

        };
        if(!testConvertFromUnicode(SCSU_inputText, sizeof(SCSU_inputText)/sizeof(SCSU_inputText[0]),
                to_SCSU, sizeof(to_SCSU), "SCSU",
                UCNV_FROM_U_CALLBACK_STOP, from_SCSUOffs, NULL, 0 ))
            log_err("u-> SCSU with skip did not match.\n");
    
    }

    /*to Unicode*/

#if !UCONFIG_NO_LEGACY_CONVERSION
    if(!testConvertToUnicode(expstopIBM_949, sizeof(expstopIBM_949),
             IBM_949stoptoUnicode, sizeof(IBM_949stoptoUnicode)/sizeof(IBM_949stoptoUnicode[0]),"ibm-949",
            UCNV_TO_U_CALLBACK_STOP, fromIBM949Offs, NULL, 0 ))
        log_err("ibm-949->u with stop did not match.\n");
    if(!testConvertToUnicode(expstopIBM_943, sizeof(expstopIBM_943),
             IBM_943stoptoUnicode, sizeof(IBM_943stoptoUnicode)/sizeof(IBM_943stoptoUnicode[0]),"ibm-943",
            UCNV_TO_U_CALLBACK_STOP, fromIBM943Offs, NULL, 0 ))
        log_err("ibm-943->u with stop did not match.\n");
    if(!testConvertToUnicode(expstopIBM_930, sizeof(expstopIBM_930),
             IBM_930stoptoUnicode, sizeof(IBM_930stoptoUnicode)/sizeof(IBM_930stoptoUnicode[0]),"ibm-930",
            UCNV_TO_U_CALLBACK_STOP, fromIBM930Offs, NULL, 0 ))
        log_err("ibm-930->u with stop did not match.\n");

    log_verbose("Testing toUnicode with UCNV_TO_U_CALLBACK_STOP \n");
    {

        static const uint8_t sampleTxtEBCIDIC_STATEFUL [] ={
            0x0e, 0x5d, 0x5f , 0x41, 0x79, 0x41, 0x44
        };
        static const UChar EBCIDIC_STATEFUL_toUnicode[] ={  0x6d63 };
        static const int32_t from_EBCIDIC_STATEFULOffsets []={ 1};


         /*EUC-JP*/
        static const uint8_t sampleTxt_euc_jp[]={ 0x61, 0xa1, 0xb8, 0x8f, 0xf4, 0xae,
            0x8f, 0xda, 0xa1,  /*unassigned*/
           0x8e, 0xe0,
        };
        static const UChar euc_jptoUnicode[]={ 0x0061, 0x4edd, 0x5bec};
        static const int32_t from_euc_jpOffs [] ={ 0, 1, 3};

          /*EUC_TW*/
        static const uint8_t sampleTxt_euc_tw[]={ 0x61, 0xa2, 0xd3, 0x8e, 0xa2, 0xdc, 0xe5,
            0x8e, 0xaa, 0xbb, 0xcc,/*unassigned*/
           0xe6, 0xca, 0x8a,
        };
        UChar euc_twtoUnicode[]={ 0x0061, 0x2295, 0x5BF2};
        int32_t from_euc_twOffs [] ={ 0, 1, 3};



         if(!testConvertToUnicode(sampleTxtEBCIDIC_STATEFUL, sizeof(sampleTxtEBCIDIC_STATEFUL),
             EBCIDIC_STATEFUL_toUnicode, sizeof(EBCIDIC_STATEFUL_toUnicode)/sizeof(EBCIDIC_STATEFUL_toUnicode[0]),"ibm-930",
            UCNV_TO_U_CALLBACK_STOP, from_EBCIDIC_STATEFULOffsets, NULL, 0 ))
        log_err("EBCIDIC_STATEFUL->u with stop did not match.\n");

        if(!testConvertToUnicode(sampleTxt_euc_jp, sizeof(sampleTxt_euc_jp),
             euc_jptoUnicode, sizeof(euc_jptoUnicode)/sizeof(euc_jptoUnicode[0]),"euc-jp",
            UCNV_TO_U_CALLBACK_STOP, from_euc_jpOffs , NULL, 0))
        log_err("euc-jp->u with stop did not match.\n");

        if(!testConvertToUnicode(sampleTxt_euc_tw, sizeof(sampleTxt_euc_tw),
                 euc_twtoUnicode, sizeof(euc_twtoUnicode)/sizeof(euc_twtoUnicode[0]),"euc-tw",
                UCNV_TO_U_CALLBACK_STOP, from_euc_twOffs, NULL, 0 ))
            log_err("euc-tw->u with stop did not match.\n");
    }
#endif

    log_verbose("Testing toUnicode for UTF-8 with UCNV_TO_U_CALLBACK_STOP \n");
    {
        static const uint8_t sampleText1[] = { 0x31, 0xe4, 0xba, 0x8c, 
            0xe0, 0x80,  0x61,};
        static const UChar    expected1[] = {  0x0031, 0x4e8c,};
        static const int32_t offsets1[] = {   0x0000, 0x0001};

        if(!testConvertToUnicode(sampleText1, sizeof(sampleText1),
                 expected1, sizeof(expected1)/sizeof(expected1[0]),"utf8",
                UCNV_TO_U_CALLBACK_STOP, offsets1, NULL, 0 ))
            log_err("utf8->u with stop did not match.\n");;
    }
    log_verbose("Testing toUnicode for SCSU with UCNV_TO_U_CALLBACK_STOP \n");
    {
        static const uint8_t sampleText1[] = {  0xba, 0x8c,0xF8, 0x61,0x0c, 0x0c,0x04};
        static const UChar    expected1[] = {  0x00ba,  0x008c,  0x00f8,  0x0061};
        static const int32_t offsets1[] = {   0x0000, 0x0001,0x0002,0x0003};

        if(!testConvertToUnicode(sampleText1, sizeof(sampleText1),
                 expected1, sizeof(expected1)/sizeof(expected1[0]),"SCSU",
                UCNV_TO_U_CALLBACK_STOP, offsets1, NULL, 0 ))
            log_err("scsu->u with stop did not match.\n");;
    }

}

static void TestSub(int32_t inputsize, int32_t outputsize)
{
    static const UChar   sampleText[] =  { 0x0000, 0xAC00, 0xAC01, 0xEF67, 0xD700 };
    static const UChar sampleText2[]=    { 0x6D63, 0x6D64, 0x6D65, 0x6D66 };
    
    static const uint8_t expsubIBM_949[] = 
     { 0x00, 0xb0, 0xa1, 0xb0, 0xa2, 0xaf, 0xfe, 0xc8, 0xd3 };

    static const uint8_t expsubIBM_943[] = { 
        0x9f, 0xaf, 0x9f, 0xb1, 0xfc, 0xfc, 0x89, 0x59 };

    static const uint8_t expsubIBM_930[] = { 
        0x0e, 0x5d, 0x5f, 0x5d, 0x63, 0xfe, 0xfe, 0x46, 0x6b, 0x0f };

    static const UChar IBM_949subtoUnicode[]= {0x0000, 0xAC00, 0xAC01, 0xfffd, 0xD700 };
    static const UChar IBM_943subtoUnicode[]= {0x6D63, 0x6D64, 0xfffd, 0x6D66 };
    static const UChar IBM_930subtoUnicode[]= {0x6D63, 0x6D64, 0xfffd, 0x6D66 };

    static const int32_t toIBM949Offssub [] ={ 0, 1, 1, 2, 2, 3, 3, 4, 4 };
    static const int32_t toIBM943Offssub [] ={ 0, 0, 1, 1, 2, 2, 3, 3 };
    static const int32_t toIBM930Offssub [] ={ 0, 0, 0, 1, 1, 2, 2, 3, 3, 3 };

    static const int32_t  fromIBM949Offs [] = { 0, 1, 3, 5, 7 };
    static const int32_t  fromIBM943Offs [] = { 0, 2, 4, 6 };
    static const int32_t  fromIBM930Offs [] = { 1, 3, 5, 7 };

    gInBufferSize = inputsize;
    gOutBufferSize = outputsize;

    /*from unicode*/

#if !UCONFIG_NO_LEGACY_CONVERSION
    if(!testConvertFromUnicode(sampleText, sizeof(sampleText)/sizeof(sampleText[0]),
            expsubIBM_949, sizeof(expsubIBM_949), "ibm-949", 
            UCNV_FROM_U_CALLBACK_SUBSTITUTE, toIBM949Offssub, NULL, 0 ))
        log_err("u-> ibm-949 with subst did not match.\n");
    if(!testConvertFromUnicode(sampleText2, sizeof(sampleText2)/sizeof(sampleText2[0]),
            expsubIBM_943, sizeof(expsubIBM_943), "ibm-943",
            UCNV_FROM_U_CALLBACK_SUBSTITUTE, toIBM943Offssub , NULL, 0))
        log_err("u-> ibm-943 with subst did not match.\n");
    if(!testConvertFromUnicode(sampleText2, sizeof(sampleText2)/sizeof(sampleText2[0]),
            expsubIBM_930, sizeof(expsubIBM_930), "ibm-930", 
            UCNV_FROM_U_CALLBACK_SUBSTITUTE, toIBM930Offssub, NULL, 0 ))
        log_err("u-> ibm-930 with subst did not match.\n");

    log_verbose("Testing fromUnicode with UCNV_FROM_U_CALLBACK_SUBSTITUTE  \n");
    {
        static const UChar inputTest[] = { 0x0061, 0xd801, 0xdc01, 0xd801, 0x0061 };
        static const uint8_t toIBM943[]= { 0x61, 0xfc, 0xfc, 0xfc, 0xfc, 0x61 };
        static const int32_t offset[]= {0, 1, 1, 3, 3, 4};


        /* EUC_JP*/
        static const UChar euc_jp_inputText[]={ 0x0061, 0x4edd, 0x5bec, 0xd801, 0xdc01, 0xd801, 0x0061, 0x00a2 };
        static const uint8_t to_euc_jp[]={ 0x61, 0xa1, 0xb8, 0x8f, 0xf4, 0xae,
            0xf4, 0xfe, 0xf4, 0xfe, 
            0x61, 0x8e, 0xe0,
        };
        static const int32_t fromEUC_JPOffs [] ={ 0, 1, 1, 2, 2, 2, 3, 3, 5, 5, 6, 7, 7};

        /*EUC_TW*/
        static const UChar euc_tw_inputText[]={ 0x0061, 0x2295, 0x5BF2, 0xd801, 0xdc01, 0xd801, 0x0061, 0x8706, 0x8a, };
        static const uint8_t to_euc_tw[]={ 
            0x61, 0xa2, 0xd3, 0x8e, 0xa2, 0xdc, 0xe5,
            0xfd, 0xfe, 0xfd, 0xfe,
            0x61, 0xe6, 0xca, 0x8a,
        };

        static const int32_t from_euc_twOffs [] ={ 0, 1, 1, 2, 2, 2, 2, 3, 3, 5, 5, 6, 7, 7, 8,};

        if(!testConvertFromUnicode(inputTest, sizeof(inputTest)/sizeof(inputTest[0]),
                toIBM943, sizeof(toIBM943), "ibm-943",
                UCNV_FROM_U_CALLBACK_SUBSTITUTE, offset, NULL, 0 ))
            log_err("u-> ibm-943 with substitute did not match.\n");

        if(!testConvertFromUnicode(euc_jp_inputText, sizeof(euc_jp_inputText)/sizeof(euc_jp_inputText[0]),
                to_euc_jp, sizeof(to_euc_jp), "euc-jp",
                UCNV_FROM_U_CALLBACK_SUBSTITUTE, fromEUC_JPOffs, NULL, 0 ))
            log_err("u-> euc-jp with substitute did not match.\n");

        if(!testConvertFromUnicode(euc_tw_inputText, sizeof(euc_tw_inputText)/sizeof(euc_tw_inputText[0]),
                to_euc_tw, sizeof(to_euc_tw), "euc-tw",
                UCNV_FROM_U_CALLBACK_SUBSTITUTE, from_euc_twOffs, NULL, 0 ))
            log_err("u-> euc-tw with substitute did not match.\n");
    }
#endif

    log_verbose("Testing fromUnicode for SCSU with UCNV_FROM_U_CALLBACK_SUBSTITUTE \n");
    {
        UChar SCSU_inputText[]={ 0x0041, 0xd801/*illegal*/, 0x0042, };

        const uint8_t to_SCSU[]={    
            0x41,
            0x0e, 0xff,0xfd,
            0x42

           
        };
        int32_t from_SCSUOffs [] ={ 
            0,
            1,1,1,
            2,

        };
        const uint8_t to_SCSU_1[]={    
            0x41,
           
        };
        int32_t from_SCSUOffs_1 [] ={ 
            0,

        };
        if(!testConvertFromUnicode(SCSU_inputText, sizeof(SCSU_inputText)/sizeof(SCSU_inputText[0]),
                to_SCSU, sizeof(to_SCSU), "SCSU",
                UCNV_FROM_U_CALLBACK_SUBSTITUTE, from_SCSUOffs, NULL, 0 ))
            log_err("u-> SCSU with substitute did not match.\n");
                
        if(!testConvertFromUnicodeWithContext(SCSU_inputText, sizeof(SCSU_inputText)/sizeof(SCSU_inputText[0]),
                to_SCSU_1, sizeof(to_SCSU_1), "SCSU",
                UCNV_FROM_U_CALLBACK_SUBSTITUTE, from_SCSUOffs_1, NULL, 0,"i",U_ILLEGAL_CHAR_FOUND ))
            log_err("u-> SCSU with substitute did not match.\n");
    }
    
    log_verbose("Testing fromUnicode for UTF-8 with UCNV_FROM_U_CALLBACK_SUBSTITUTE\n");
    {
        static const UChar testinput[]={ 0x20ac, 0xd801, 0xdc01, 0xdc01, 0xd801, 0xffff, 0x0061,};
        static const uint8_t expectedUTF8[]= { 0xe2, 0x82, 0xac, 
                           0xf0, 0x90, 0x90, 0x81, 
                           0xef, 0xbf, 0xbd, 0xef, 0xbf, 0xbd,
                           0xef, 0xbf, 0xbf, 0x61,
                           
        };
        static const int32_t offsets[]={ 0, 0, 0, 1, 1, 1, 1, 3, 3, 3, 4, 4, 4, 5, 5, 5, 6 };
        if(!testConvertFromUnicode(testinput, sizeof(testinput)/sizeof(testinput[0]),
                expectedUTF8, sizeof(expectedUTF8), "utf8",
                UCNV_FROM_U_CALLBACK_SUBSTITUTE, offsets, NULL, 0 )) {
            log_err("u-> utf8 with stop did not match.\n");
        }
    }

    log_verbose("Testing fromUnicode for UTF-16 with UCNV_FROM_U_CALLBACK_SUBSTITUTE\n");
    {
        static const UChar in[]={ 0x0041, 0xfeff };

        static const uint8_t out[]={
#if U_IS_BIG_ENDIAN
            0xfe, 0xff,
            0x00, 0x41,
            0xfe, 0xff
#else
            0xff, 0xfe,
            0x41, 0x00,
            0xff, 0xfe
#endif
        };
        static const int32_t offsets[]={
            -1, -1, 0, 0, 1, 1
        };

        if(!testConvertFromUnicode(in, ARRAY_LENGTH(in),
                                   out, sizeof(out), "UTF-16",
                                   UCNV_FROM_U_CALLBACK_SUBSTITUTE, offsets, NULL, 0)
        ) {
            log_err("u->UTF-16 with substitute did not match.\n");
        }
    }

    log_verbose("Testing fromUnicode for UTF-32 with UCNV_FROM_U_CALLBACK_SUBSTITUTE\n");
    {
        static const UChar in[]={ 0x0041, 0xfeff };

        static const uint8_t out[]={
#if U_IS_BIG_ENDIAN
            0x00, 0x00, 0xfe, 0xff,
            0x00, 0x00, 0x00, 0x41,
            0x00, 0x00, 0xfe, 0xff
#else
            0xff, 0xfe, 0x00, 0x00,
            0x41, 0x00, 0x00, 0x00,
            0xff, 0xfe, 0x00, 0x00
#endif
        };
        static const int32_t offsets[]={
            -1, -1, -1, -1, 0, 0, 0, 0, 1, 1, 1, 1
        };

        if(!testConvertFromUnicode(in, ARRAY_LENGTH(in),
                                   out, sizeof(out), "UTF-32",
                                   UCNV_FROM_U_CALLBACK_SUBSTITUTE, offsets, NULL, 0)
        ) {
            log_err("u->UTF-32 with substitute did not match.\n");
        }
    }

    /*to unicode*/

#if !UCONFIG_NO_LEGACY_CONVERSION
    if(!testConvertToUnicode(expsubIBM_949, sizeof(expsubIBM_949),
             IBM_949subtoUnicode, sizeof(IBM_949subtoUnicode)/sizeof(IBM_949subtoUnicode[0]),"ibm-949",
            UCNV_TO_U_CALLBACK_SUBSTITUTE, fromIBM949Offs, NULL, 0 ))
        log_err("ibm-949->u with substitute did not match.\n");
    if(!testConvertToUnicode(expsubIBM_943, sizeof(expsubIBM_943),
             IBM_943subtoUnicode, sizeof(IBM_943subtoUnicode)/sizeof(IBM_943subtoUnicode[0]),"ibm-943",
            UCNV_TO_U_CALLBACK_SUBSTITUTE, fromIBM943Offs, NULL, 0 ))
        log_err("ibm-943->u with substitute did not match.\n");
    if(!testConvertToUnicode(expsubIBM_930, sizeof(expsubIBM_930),
             IBM_930subtoUnicode, sizeof(IBM_930subtoUnicode)/sizeof(IBM_930subtoUnicode[0]),"ibm-930",
            UCNV_TO_U_CALLBACK_SUBSTITUTE, fromIBM930Offs, NULL, 0 ))
        log_err("ibm-930->u with substitute did not match.\n");
 
    log_verbose("Testing toUnicode with UCNV_TO_U_CALLBACK_SUBSTITUTE \n");
    {

        const uint8_t sampleTxtEBCIDIC_STATEFUL [] ={
            0x0e, 0x5d, 0x5f , 0x41, 0x79, 0x41, 0x44
        };
        UChar EBCIDIC_STATEFUL_toUnicode[] ={  0x6d63, 0xfffd, 0x03b4 
        };
        int32_t from_EBCIDIC_STATEFULOffsets []={ 1, 3, 5};


        /* EUC_JP*/
        const uint8_t sampleTxt_euc_jp[]={ 0x61, 0xa1, 0xb8, 0x8f, 0xf4, 0xae,
            0x8f, 0xda, 0xa1,  /*unassigned*/
           0x8e, 0xe0, 0x8a
        };
        UChar euc_jptoUnicode[]={ 0x0061, 0x4edd, 0x5bec, 0xfffd, 0x00a2, 0x008a };
        int32_t from_euc_jpOffs [] ={ 0, 1, 3, 6,  9, 11 };

        /*EUC_TW*/
        const uint8_t sampleTxt_euc_tw[]={ 
            0x61, 0xa2, 0xd3, 0x8e, 0xa2, 0xdc, 0xe5,
            0x8e, 0xaa, 0xbb, 0xcc,/*unassigned*/
            0xe6, 0xca, 0x8a,
        };
        UChar euc_twtoUnicode[]={ 0x0061, 0x2295, 0x5BF2, 0xfffd, 0x8706, 0x8a, };
        int32_t from_euc_twOffs [] ={ 0, 1, 3, 7, 11, 13};

       
        if(!testConvertToUnicode(sampleTxtEBCIDIC_STATEFUL, sizeof(sampleTxtEBCIDIC_STATEFUL),
           EBCIDIC_STATEFUL_toUnicode, sizeof(EBCIDIC_STATEFUL_toUnicode)/sizeof(EBCIDIC_STATEFUL_toUnicode[0]),"ibm-930",
          UCNV_TO_U_CALLBACK_SUBSTITUTE, from_EBCIDIC_STATEFULOffsets, NULL, 0 ))
            log_err("EBCIDIC_STATEFUL->u with substitute did not match.\n");


        if(!testConvertToUnicode(sampleTxt_euc_jp, sizeof(sampleTxt_euc_jp),
           euc_jptoUnicode, sizeof(euc_jptoUnicode)/sizeof(euc_jptoUnicode[0]),"euc-jp",
          UCNV_TO_U_CALLBACK_SUBSTITUTE, from_euc_jpOffs, NULL, 0 ))
            log_err("euc-jp->u with substitute did not match.\n");

        
        if(!testConvertToUnicode(sampleTxt_euc_tw, sizeof(sampleTxt_euc_tw),
           euc_twtoUnicode, sizeof(euc_twtoUnicode)/sizeof(euc_twtoUnicode[0]),"euc-tw",
          UCNV_TO_U_CALLBACK_SUBSTITUTE, from_euc_twOffs, NULL, 0 ))
            log_err("euc-tw->u with substitute  did not match.\n");

        
        if(!testConvertToUnicodeWithContext(sampleTxt_euc_jp, sizeof(sampleTxt_euc_jp),
           euc_jptoUnicode, sizeof(euc_jptoUnicode)/sizeof(euc_jptoUnicode[0]),"euc-jp",
          UCNV_TO_U_CALLBACK_SUBSTITUTE, from_euc_jpOffs, NULL, 0 ,"i", U_ILLEGAL_CHAR_FOUND))
            log_err("euc-jp->u with substitute did not match.\n");
    }
#endif

    log_verbose("Testing toUnicode for UTF-8 with UCNV_TO_U_CALLBACK_SUBSTITUTE \n");
    {
        const uint8_t sampleText1[] = { 0x31, 0xe4, 0xba, 0x8c, 
            0xe0, 0x80,  0x61,};
        UChar    expected1[] = {  0x0031, 0x4e8c, 0xfffd, 0x0061};
        int32_t offsets1[] = {   0x0000, 0x0001, 0x0004, 0x0006};

        if(!testConvertToUnicode(sampleText1, sizeof(sampleText1),
                 expected1, sizeof(expected1)/sizeof(expected1[0]),"utf8",
                UCNV_TO_U_CALLBACK_SUBSTITUTE, offsets1, NULL, 0 ))
            log_err("utf8->u with substitute did not match.\n");;
    }
    log_verbose("Testing toUnicode for SCSU with UCNV_TO_U_CALLBACK_SUBSTITUTE \n");
    {
        const uint8_t sampleText1[] = {  0xba, 0x8c,0xF8, 0x61,0x0c, 0x0c,};
        UChar    expected1[] = {  0x00ba,  0x008c,  0x00f8,  0x0061,0xfffd,0xfffd};
        int32_t offsets1[] = {   0x0000, 0x0001,0x0002,0x0003,4,5};

        if(!testConvertToUnicode(sampleText1, sizeof(sampleText1),
                 expected1, sizeof(expected1)/sizeof(expected1[0]),"SCSU",
                UCNV_TO_U_CALLBACK_SUBSTITUTE, offsets1, NULL, 0 ))
            log_err("scsu->u with stop did not match.\n");;
    }

#if !UCONFIG_NO_LEGACY_CONVERSION
    log_verbose("Testing ibm-930 subchar/subchar1\n");
    {
        static const UChar u1[]={         0x6d63,           0x6d64,     0x6d65,     0x6d66,     0xdf };
        static const uint8_t s1[]={       0x0e, 0x5d, 0x5f, 0x5d, 0x63, 0xfe, 0xfe, 0x46, 0x6b, 0x0f, 0x3f };
        static const int32_t offsets1[]={ 0,    0,    0,    1,    1,    2,    2,    3,    3,    4,    4 };

        static const UChar u2[]={         0x6d63,           0x6d64,     0xfffd,     0x6d66,     0x1a };
        static const uint8_t s2[]={       0x0e, 0x5d, 0x5f, 0x5d, 0x63, 0xfc, 0xfc, 0x46, 0x6b, 0x0f, 0x57 };
        static const int32_t offsets2[]={ 1,                3,          5,          7,          10 };

        if(!testConvertFromUnicode(u1, ARRAY_LENGTH(u1), s1, ARRAY_LENGTH(s1), "ibm-930", 
                                   UCNV_FROM_U_CALLBACK_SUBSTITUTE, offsets1, NULL, 0)
        ) {
            log_err("u->ibm-930 subchar/subchar1 did not match.\n");
        }

        if(!testConvertToUnicode(s2, ARRAY_LENGTH(s2), u2, ARRAY_LENGTH(u2), "ibm-930", 
                                 UCNV_TO_U_CALLBACK_SUBSTITUTE, offsets2, NULL, 0)
        ) {
            log_err("ibm-930->u subchar/subchar1 did not match.\n");
        }
    }

    log_verbose("Testing GB 18030 with substitute callbacks\n");
    {
        static const UChar u2[]={
            0x24, 0x7f, 0x80,                   0x1f9,      0x20ac,     0x4e00,     0x9fa6,                 0xffff,                 0xd800, 0xdc00,         0xfffd,                 0xdbff, 0xdfff };
        static const uint8_t gb2[]={
            0x24, 0x7f, 0x81, 0x30, 0x81, 0x30, 0xa8, 0xbf, 0xa2, 0xe3, 0xd2, 0xbb, 0x82, 0x35, 0x8f, 0x33, 0x84, 0x31, 0xa4, 0x39, 0x90, 0x30, 0x81, 0x30, 0xe3, 0x32, 0x9a, 0x36, 0xe3, 0x32, 0x9a, 0x35 };
        static const int32_t offsets2[]={
            0, 1, 2, 6, 8, 10, 12, 16, 20, 20, 24, 28, 28 };

        if(!testConvertToUnicode(gb2, ARRAY_LENGTH(gb2), u2, ARRAY_LENGTH(u2), "gb18030", 
                                 UCNV_TO_U_CALLBACK_SUBSTITUTE, offsets2, NULL, 0)
        ) {
            log_err("gb18030->u with substitute did not match.\n");
        }
    }
#endif

    log_verbose("Testing UTF-7 toUnicode with substitute callbacks\n");
    {
        static const uint8_t utf7[]={
         /* a~            a+AB~                         a+AB\x0c                      a+AB-                         a+AB.                         a+. */
            0x61, 0x7e,   0x61, 0x2b, 0x41, 0x42, 0x7e, 0x61, 0x2b, 0x41, 0x42, 0x0c, 0x61, 0x2b, 0x41, 0x42, 0x2d, 0x61, 0x2b, 0x41, 0x42, 0x2e, 0x61, 0x2b, 0x2e
        };
        static const UChar unicode[]={
            0x61, 0xfffd, 0x61,       0xfffd,           0x61,       0xfffd,           0x61,       0xfffd,           0x61,       0xfffd,           0x61, 0xfffd
        };
        static const int32_t offsets[]={
            0,    1,      2,          4,                7,          9,                12,         14,               17,         19,               22,   23
        };

        if(!testConvertToUnicode(utf7, ARRAY_LENGTH(utf7), unicode, ARRAY_LENGTH(unicode), "UTF-7", 
                                 UCNV_TO_U_CALLBACK_SUBSTITUTE, offsets, NULL, 0)
        ) {
            log_err("UTF-7->u with substitute did not match.\n");
        }
    }

    log_verbose("Testing UTF-16 toUnicode with substitute callbacks\n");
    {
        static const uint8_t
            in1[]={ 0xfe, 0xff, 0x4e, 0x00, 0xfe, 0xff },
            in2[]={ 0xff, 0xfe, 0x4e, 0x00, 0xfe, 0xff },
            in3[]={ 0xfe, 0xfd, 0x4e, 0x00, 0xfe, 0xff };

        static const UChar
            out1[]={ 0x4e00, 0xfeff },
            out2[]={ 0x004e, 0xfffe },
            out3[]={ 0xfefd, 0x4e00, 0xfeff };

        static const int32_t
            offsets1[]={ 2, 4 },
            offsets2[]={ 2, 4 },
            offsets3[]={ 0, 2, 4 };

        if(!testConvertToUnicode(in1, ARRAY_LENGTH(in1), out1, ARRAY_LENGTH(out1), "UTF-16", 
                                 UCNV_TO_U_CALLBACK_SUBSTITUTE, offsets1, NULL, 0)
        ) {
            log_err("UTF-16 (BE BOM)->u with substitute did not match.\n");
        }

        if(!testConvertToUnicode(in2, ARRAY_LENGTH(in2), out2, ARRAY_LENGTH(out2), "UTF-16", 
                                 UCNV_TO_U_CALLBACK_SUBSTITUTE, offsets2, NULL, 0)
        ) {
            log_err("UTF-16 (LE BOM)->u with substitute did not match.\n");
        }

        if(!testConvertToUnicode(in3, ARRAY_LENGTH(in3), out3, ARRAY_LENGTH(out3), "UTF-16", 
                                 UCNV_TO_U_CALLBACK_SUBSTITUTE, offsets3, NULL, 0)
        ) {
            log_err("UTF-16 (no BOM)->u with substitute did not match.\n");
        }
    }

    log_verbose("Testing UTF-32 toUnicode with substitute callbacks\n");
    {
        static const uint8_t
            in1[]={ 0x00, 0x00, 0xfe, 0xff,   0x00, 0x10, 0x0f, 0x00,   0x00, 0x00, 0xfe, 0xff },
            in2[]={ 0xff, 0xfe, 0x00, 0x00,   0x00, 0x10, 0x0f, 0x00,   0xfe, 0xff, 0x00, 0x00 },
            in3[]={ 0x00, 0x00, 0xfe, 0xfe,   0x00, 0x10, 0x0f, 0x00,   0x00, 0x00, 0xd8, 0x40,   0x00, 0x00, 0xdc, 0x01 },
            in4[]={ 0x00, 0x01, 0x02, 0x03,   0x00, 0x11, 0x12, 0x00,   0x00, 0x00, 0x4e, 0x00 };

        static const UChar
            out1[]={ UTF16_LEAD(0x100f00), UTF16_TRAIL(0x100f00), 0xfeff },
            out2[]={ UTF16_LEAD(0x0f1000), UTF16_TRAIL(0x0f1000), 0xfffe },
            out3[]={ 0xfefe, UTF16_LEAD(0x100f00), UTF16_TRAIL(0x100f00), 0xfffd, 0xfffd },
            out4[]={ UTF16_LEAD(0x10203), UTF16_TRAIL(0x10203), 0xfffd, 0x4e00 };

        static const int32_t
            offsets1[]={ 4, 4, 8 },
            offsets2[]={ 4, 4, 8 },
            offsets3[]={ 0, 4, 4, 8, 12 },
            offsets4[]={ 0, 0, 4, 8 };

        if(!testConvertToUnicode(in1, ARRAY_LENGTH(in1), out1, ARRAY_LENGTH(out1), "UTF-32", 
                                 UCNV_TO_U_CALLBACK_SUBSTITUTE, offsets1, NULL, 0)
        ) {
            log_err("UTF-32 (BE BOM)->u with substitute did not match.\n");
        }

        if(!testConvertToUnicode(in2, ARRAY_LENGTH(in2), out2, ARRAY_LENGTH(out2), "UTF-32", 
                                 UCNV_TO_U_CALLBACK_SUBSTITUTE, offsets2, NULL, 0)
        ) {
            log_err("UTF-32 (LE BOM)->u with substitute did not match.\n");
        }

        if(!testConvertToUnicode(in3, ARRAY_LENGTH(in3), out3, ARRAY_LENGTH(out3), "UTF-32", 
                                 UCNV_TO_U_CALLBACK_SUBSTITUTE, offsets3, NULL, 0)
        ) {
            log_err("UTF-32 (no BOM)->u with substitute did not match.\n");
        }

        if(!testConvertToUnicode(in4, ARRAY_LENGTH(in4), out4, ARRAY_LENGTH(out4), "UTF-32", 
                                 UCNV_TO_U_CALLBACK_SUBSTITUTE, offsets4, NULL, 0)
        ) {
            log_err("UTF-32 (no BOM, with error)->u with substitute did not match.\n");
        }
    }
}

static void TestSubWithValue(int32_t inputsize, int32_t outputsize)
{
    UChar   sampleText[] =  { 0x0000, 0xAC00, 0xAC01, 0xEF67, 0xD700 };
    UChar  sampleText2[] =  { 0x6D63, 0x6D64, 0x6D65, 0x6D66 };

    const uint8_t expsubwvalIBM_949[]= { 
        0x00, 0xb0, 0xa1, 0xb0, 0xa2,
        0x25, 0x55, 0x45, 0x46, 0x36, 0x37, 0xc8, 0xd3 }; 

    const uint8_t expsubwvalIBM_943[]= { 
        0x9f, 0xaf, 0x9f, 0xb1,
        0x25, 0x55, 0x36, 0x44, 0x36, 0x35, 0x89, 0x59 };

    const uint8_t expsubwvalIBM_930[] = {
        0x0e, 0x5d, 0x5f, 0x5d, 0x63, 0x0f, 0x6c, 0xe4, 0xf6, 0xc4, 0xf6, 0xf5, 0x0e, 0x46, 0x6b, 0x0f };

    int32_t toIBM949Offs [] ={ 0, 1, 1, 2, 2, 3, 3, 3, 3, 3, 3, 4, 4 };
    int32_t toIBM943Offs [] = { 0, 0, 1, 1, 2, 2, 2, 2, 2, 2, 3, 3 };
    int32_t toIBM930Offs [] = { 0, 0, 0, 1, 1, 2, 2, 2, 2, 2, 2, 2, 3, 3, 3, 3 }; /* last item: 3,3,3,3 because there's SO+DBCS+SI */

    gInBufferSize = inputsize;
    gOutBufferSize = outputsize;

    /*from Unicode*/

#if !UCONFIG_NO_LEGACY_CONVERSION
    if(!testConvertFromUnicode(sampleText, sizeof(sampleText)/sizeof(sampleText[0]),
            expsubwvalIBM_949, sizeof(expsubwvalIBM_949), "ibm-949", 
            UCNV_FROM_U_CALLBACK_ESCAPE, toIBM949Offs, NULL, 0 ))
        log_err("u-> ibm-949 with subst with value did not match.\n");

    if(!testConvertFromUnicode(sampleText2, sizeof(sampleText2)/sizeof(sampleText2[0]),
            expsubwvalIBM_943, sizeof(expsubwvalIBM_943), "ibm-943",
            UCNV_FROM_U_CALLBACK_ESCAPE, toIBM943Offs, NULL, 0 ))
        log_err("u-> ibm-943 with sub with value did not match.\n");

    if(!testConvertFromUnicode(sampleText2, sizeof(sampleText2)/sizeof(sampleText2[0]),
            expsubwvalIBM_930, sizeof(expsubwvalIBM_930), "ibm-930", 
            UCNV_FROM_U_CALLBACK_ESCAPE, toIBM930Offs, NULL, 0 ))
        log_err("u-> ibm-930 with subst with value did not match.\n");


    log_verbose("Testing fromUnicode with UCNV_FROM_U_CALLBACK_ESCAPE  \n");
    {
        static const UChar inputTest[] = { 0x0061, 0xd801, 0xdc01, 0xd801, 0x0061 };
        static const uint8_t toIBM943[]= { 0x61, 
            0x25, 0x55, 0x44, 0x38, 0x30, 0x31,
            0x25, 0x55, 0x44, 0x43, 0x30, 0x31,
            0x25, 0x55, 0x44, 0x38, 0x30, 0x31,
            0x61 };
        static const int32_t offset[]= {0, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 3, 3, 3, 3, 3, 3, 4};


         /* EUC_JP*/
        static const UChar euc_jp_inputText[]={ 0x0061, 0x4edd, 0x5bec, 0xd801, 0xdc01, 0xd801, 0x0061, 0x00a2, };
        static const uint8_t to_euc_jp[]={ 0x61, 0xa1, 0xb8, 0x8f, 0xf4, 0xae,
            0x25, 0x55, 0x44, 0x38, 0x30, 0x31,
            0x25, 0x55, 0x44, 0x43, 0x30, 0x31,
            0x25, 0x55, 0x44, 0x38, 0x30, 0x31,
            0x61, 0x8e, 0xe0,
        };
        static const int32_t fromEUC_JPOffs [] ={ 0, 1, 1, 2, 2, 2, 
            3, 3, 3, 3, 3, 3, 
            3, 3, 3, 3, 3, 3,
            5, 5, 5, 5, 5, 5,
            6, 7, 7,
        };

        /*EUC_TW*/
        static const UChar euc_tw_inputText[]={ 0x0061, 0x2295, 0x5BF2, 0xd801, 0xdc01, 0xd801, 0x0061, 0x8706, 0x8a, };
        static const uint8_t to_euc_tw[]={ 
            0x61, 0xa2, 0xd3, 0x8e, 0xa2, 0xdc, 0xe5,
            0x25, 0x55, 0x44, 0x38, 0x30, 0x31,
            0x25, 0x55, 0x44, 0x43, 0x30, 0x31,
            0x25, 0x55, 0x44, 0x38, 0x30, 0x31,
            0x61, 0xe6, 0xca, 0x8a,
        };
        static const int32_t from_euc_twOffs [] ={ 0, 1, 1, 2, 2, 2, 2, 
             3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 5, 5, 5, 5, 5, 5,
             6, 7, 7, 8,
        };
        /*ISO-2022-JP*/
        static const UChar iso_2022_jp_inputText1[]={ 0x3000, 0x00E9, 0x3001,0x00E9, 0x0042} ;
        static const uint8_t to_iso_2022_jp1[]={  
            0x1b,   0x24,   0x42,   0x21, 0x21,   
            0x1b,   0x28,   0x42,   0x25, 0x55,   0x30,   0x30,   0x45,   0x39,   
            0x1b,   0x24,   0x42,   0x21, 0x22,
            0x1b,   0x28,   0x42,   0x25, 0x55,   0x30,   0x30,   0x45,   0x39, 
            0x42,
        };

        static const int32_t from_iso_2022_jpOffs1 [] ={ 
            0,0,0,0,0,
            1,1,1,1,1,1,1,1,1,
            2,2,2,2,2,
            3,3,3,3,3,3,3,3,3,
            4,
        };
        /* surrogate pair*/
        static const UChar iso_2022_jp_inputText2[]={ 0x3000, 0xD84D, 0xDC56, 0x3001,0xD84D,0xDC56, 0x0042} ;
        static const uint8_t to_iso_2022_jp2[]={  
                                0x1b,   0x24,   0x42,   0x21,   0x21,   
                                0x1b,   0x28,   0x42,   0x25,   0x55,   0x44,   0x38,   0x34,   0x44,   
                                0x25,   0x55,   0x44,   0x43,   0x35,   0x36,   
                                0x1b,   0x24,   0x42,   0x21,   0x22,
                                0x1b,   0x28,   0x42,   0x25,   0x55,   0x44,   0x38,   0x34,   0x44,   
                                0x25,   0x55,   0x44,   0x43,   0x35,   0x36,   
                                0x42,
                                };
        static const int32_t from_iso_2022_jpOffs2 [] ={ 
            0,0,0,0,0,
            1,1,1,1,1,1,1,1,1,
            1,1,1,1,1,1,
            3,3,3,3,3,
            4,4,4,4,4,4,4,4,4,
            4,4,4,4,4,4,
            6,
        };

        /*ISO-2022-cn*/
        static const UChar iso_2022_cn_inputText[]={ 0x0041, 0x3712, 0x0042, };
        static const uint8_t to_iso_2022_cn[]={  
            0x41,   
            0x25, 0x55,   0x33,   0x37,   0x31,   0x32,   
            0x42, 
        };
        static const int32_t from_iso_2022_cnOffs [] ={ 
            0,
            1,1,1,1,1,1,
            2,
        };
        
        static const UChar iso_2022_cn_inputText4[]={ 0x3000, 0xD84D, 0xDC56, 0x3001,0xD84D,0xDC56, 0x0042};

        static const uint8_t to_iso_2022_cn4[]={  
                             0x1b,   0x24,   0x29,   0x41,   0x0e,   0x21,   0x21,   
                             0x0f,   0x25,   0x55,   0x44,   0x38,   0x34,   0x44,   
                             0x25,   0x55,   0x44,   0x43,   0x35,   0x36,   
                             0x0e,   0x21,   0x22,   
                             0x0f,   0x25,   0x55,   0x44,   0x38,   0x34,   0x44,   
                             0x25,   0x55,   0x44,   0x43,   0x35,   0x36,   
                             0x42, 
                             };
        static const int32_t from_iso_2022_cnOffs4 [] ={ 
            0,0,0,0,0,0,0,
            1,1,1,1,1,1,1,
            1,1,1,1,1,1,
            3,3,3,
            4,4,4,4,4,4,4,
            4,4,4,4,4,4,
            6

        };

        /*ISO-2022-kr*/
        static const UChar iso_2022_kr_inputText2[]={ 0x0041, 0x03A0,0xD84D, 0xDC56/*unassigned*/,0x03A0, 0x0042,0xD84D, 0xDC56/*unassigned*/,0x43 };
        static const uint8_t to_iso_2022_kr2[]={  
            0x1b,   0x24,   0x29,   0x43,   
            0x41,   
            0x0e,   0x25,   0x50,   
            0x0f,   0x25,   0x55,   0x44,   0x38,   0x34,   0x44,   
            0x25,   0x55,   0x44,   0x43,   0x35,   0x36,  
            0x0e,   0x25,   0x50, 
            0x0f,   0x42, 
            0x25,   0x55,   0x44,   0x38,   0x34,   0x44,   
            0x25,   0x55,   0x44,   0x43,   0x35,   0x36,  
            0x43
        };
        static const int32_t from_iso_2022_krOffs2 [] ={ 
            -1,-1,-1,-1,
             0,
            1,1,1,
            2,2,2,2,2,2,2,
            2,2,2,2,2,2,
            4,4,4,
            5,5,
            6,6,6,6,6,6,
            6,6,6,6,6,6,
            8,
        };

        static const UChar iso_2022_kr_inputText[]={ 0x0041, 0x03A0,0x3712/*unassigned*/,0x03A0, 0x0042,0x3712/*unassigned*/,0x43 };
        static const uint8_t to_iso_2022_kr[]={  
            0x1b,   0x24,   0x29,   0x43,   
            0x41,   
            0x0e,   0x25,   0x50,   
            0x0f,   0x25,   0x55,   0x33,   0x37,   0x31,   0x32,  /*unassigned*/ 
            0x0e,   0x25,   0x50, 
            0x0f,   0x42, 
            0x25,   0x55,   0x33,   0x37,   0x31,   0x32,  /*unassigned*/ 
            0x43
        };
     

        static const int32_t from_iso_2022_krOffs [] ={ 
            -1,-1,-1,-1,
             0,
            1,1,1,
            2,2,2,2,2,2,2,
            3,3,3,
            4,4,
            5,5,5,5,5,5,
            6,
        };
        /* HZ encoding */       
        static const UChar hz_inputText[]={ 0x0041, 0x03A0,0x0662/*unassigned*/,0x03A0, 0x0042, };

        static const uint8_t to_hz[]={    
            0x7e,   0x7d,   0x41,   
            0x7e,   0x7b,   0x26,   0x30,   
            0x7e,   0x7d,   0x25,   0x55,   0x30,   0x36,   0x36,   0x32,  /*unassigned*/ 
            0x7e,   0x7b,   0x26,   0x30,
            0x7e,   0x7d,   0x42, 
           
        };
        static const int32_t from_hzOffs [] ={ 
            0,0,0,
            1,1,1,1,
            2,2,2,2,2,2,2,2,
            3,3,3,3,
            4,4,4
        };

        static const UChar hz_inputText2[]={ 0x0041, 0x03A0,0xD84D, 0xDC56/*unassigned*/,0x03A0, 0x0042,0xD84D, 0xDC56/*unassigned*/,0x43 };
        static const uint8_t to_hz2[]={   
            0x7e,   0x7d,   0x41,   
            0x7e,   0x7b,   0x26,   0x30,   
            0x7e,   0x7d,   0x25,   0x55,   0x44,   0x38,   0x34,   0x44,   
            0x25,   0x55,   0x44,   0x43,   0x35,   0x36,  
            0x7e,   0x7b,   0x26,   0x30,
            0x7e,   0x7d,   0x42, 
            0x25,   0x55,   0x44,   0x38,   0x34,   0x44,   
            0x25,   0x55,   0x44,   0x43,   0x35,   0x36,  
            0x43
        };
        static const int32_t from_hzOffs2 [] ={ 
            0,0,0,
            1,1,1,1,
            2,2,2,2,2,2,2,2,
            2,2,2,2,2,2,
            4,4,4,4,
            5,5,5,
            6,6,6,6,6,6,
            6,6,6,6,6,6,
            8,
        };

                /*ISCII*/
        static const UChar iscii_inputText[]={ 0x0041, 0x0901,0x3712/*unassigned*/,0x0902, 0x0042,0x3712/*unassigned*/,0x43 };
        static const uint8_t to_iscii[]={   
            0x41,   
            0xef,   0x42,   0xa1,   
            0x25,   0x55,   0x33,   0x37,   0x31,   0x32,  /*unassigned*/ 
            0xa2,
            0x42, 
            0x25,   0x55,   0x33,   0x37,   0x31,   0x32,  /*unassigned*/ 
            0x43
        };
     

        static const int32_t from_isciiOffs [] ={ 
            0,
            1,1,1,
            2,2,2,2,2,2,
            3,
            4,
            5,5,5,5,5,5,
            6,
        };

        if(!testConvertFromUnicode(inputTest, sizeof(inputTest)/sizeof(inputTest[0]),
                toIBM943, sizeof(toIBM943), "ibm-943",
                UCNV_FROM_U_CALLBACK_ESCAPE, offset, NULL, 0 ))
            log_err("u-> ibm-943 with subst with value did not match.\n");

        if(!testConvertFromUnicode(euc_jp_inputText, sizeof(euc_jp_inputText)/sizeof(euc_jp_inputText[0]),
                to_euc_jp, sizeof(to_euc_jp), "euc-jp",
                UCNV_FROM_U_CALLBACK_ESCAPE, fromEUC_JPOffs, NULL, 0 ))
            log_err("u-> euc-jp with subst with value did not match.\n");

        if(!testConvertFromUnicode(euc_tw_inputText, sizeof(euc_tw_inputText)/sizeof(euc_tw_inputText[0]),
                to_euc_tw, sizeof(to_euc_tw), "euc-tw",
                UCNV_FROM_U_CALLBACK_ESCAPE, from_euc_twOffs, NULL, 0 ))
            log_err("u-> euc-tw with subst with value did not match.\n");  
        
        if(!testConvertFromUnicode(iso_2022_jp_inputText1, sizeof(iso_2022_jp_inputText1)/sizeof(iso_2022_jp_inputText1[0]),
                to_iso_2022_jp1, sizeof(to_iso_2022_jp1), "iso-2022-jp",
                UCNV_FROM_U_CALLBACK_ESCAPE, from_iso_2022_jpOffs1, NULL, 0 ))
            log_err("u-> iso_2022_jp with subst with value did not match.\n"); 
        
        if(!testConvertFromUnicode(iso_2022_jp_inputText1, sizeof(iso_2022_jp_inputText1)/sizeof(iso_2022_jp_inputText1[0]),
                to_iso_2022_jp1, sizeof(to_iso_2022_jp1), "iso-2022-jp",
                UCNV_FROM_U_CALLBACK_ESCAPE, from_iso_2022_jpOffs1, NULL, 0 ))
            log_err("u-> iso_2022_jp with subst with value did not match.\n"); 
        
        if(!testConvertFromUnicode(iso_2022_jp_inputText2, sizeof(iso_2022_jp_inputText2)/sizeof(iso_2022_jp_inputText2[0]),
                to_iso_2022_jp2, sizeof(to_iso_2022_jp2), "iso-2022-jp",
                UCNV_FROM_U_CALLBACK_ESCAPE, from_iso_2022_jpOffs2, NULL, 0 ))
            log_err("u-> iso_2022_jp with subst with value did not match.\n");
        /*ESCAPE OPTIONS*/
        {
            /* surrogate pair*/
            static const UChar iso_2022_jp_inputText3[]={ 0x3000, 0xD84D, 0xDC56, 0x3001,0xD84D,0xDC56, 0x0042,0x0901c } ;
            static const uint8_t to_iso_2022_jp3_v2[]={  
                    0x1b,   0x24,   0x42,   0x21,   0x21,   
                    0x1b,   0x28,   0x42,   0x26,   0x23,   0x31,  0x34,   0x34,   0x34,   0x37, 0x30, 0x3b,   
                      
                    0x1b,   0x24,   0x42,   0x21,   0x22,
                    0x1b,   0x28,   0x42,   0x26,   0x23,  0x31,  0x34,   0x34,   0x34,   0x37, 0x30, 0x3b, 
                    
                    0x42,
                    0x26,   0x23,   0x33,   0x36,   0x38,   0x39,   0x32,   0x3b, 
                    };

            static const int32_t from_iso_2022_jpOffs3_v2 [] ={ 
                0,0,0,0,0,
                1,1,1,1,1,1,1,1,1,1,1,1,

                3,3,3,3,3,
                4,4,4,4,4,4,4,4,4,4,4,4,

                6,
                7,7,7,7,7,7,7,7,7
            };

            if(!testConvertFromUnicodeWithContext(iso_2022_jp_inputText3, sizeof(iso_2022_jp_inputText3)/sizeof(iso_2022_jp_inputText3[0]),
                    to_iso_2022_jp3_v2, sizeof(to_iso_2022_jp3_v2), "iso-2022-jp",
                    UCNV_FROM_U_CALLBACK_ESCAPE, from_iso_2022_jpOffs3_v2, NULL, 0,UCNV_ESCAPE_XML_DEC,U_ZERO_ERROR ))
                log_err("u-> iso-2022-jp with sub & UCNV_ESCAPE_XML_DEC did not match.\n"); 
        }
        {
            static const UChar iso_2022_cn_inputText5[]={ 0x3000, 0xD84D, 0xDC56, 0x3001,0xD84D,0xDC56, 0x0042,0x0902};
            static const uint8_t to_iso_2022_cn5_v2[]={  
                             0x1b,   0x24,   0x29,   0x41,   0x0e,   0x21,   0x21,   
                             0x0f,   0x5c,   0x75,   0x44,   0x38,   0x34,   0x44,   
                             0x5c,   0x75,   0x44,   0x43,   0x35,   0x36,   
                             0x0e,   0x21,   0x22,   
                             0x0f,   0x5c,   0x75,   0x44,   0x38,   0x34,   0x44,   
                             0x5c,   0x75,   0x44,   0x43,   0x35,   0x36,   
                             0x42,
                             0x5c,   0x75,   0x30,   0x39,   0x30,   0x32,
                             };
            static const int32_t from_iso_2022_cnOffs5_v2 [] ={ 
                0,0,0,0,0,0,0,
                1,1,1,1,1,1,1,
                1,1,1,1,1,1,
                3,3,3,
                4,4,4,4,4,4,4,
                4,4,4,4,4,4,
                6, 
                7,7,7,7,7,7
            };
            if(!testConvertFromUnicodeWithContext(iso_2022_cn_inputText5, sizeof(iso_2022_cn_inputText5)/sizeof(iso_2022_cn_inputText5[0]),
                to_iso_2022_cn5_v2, sizeof(to_iso_2022_cn5_v2), "iso-2022-cn",
                UCNV_FROM_U_CALLBACK_ESCAPE, from_iso_2022_cnOffs5_v2, NULL, 0,UCNV_ESCAPE_JAVA,U_ZERO_ERROR ))
                log_err("u-> iso-2022-cn with sub & UCNV_ESCAPE_JAVA did not match.\n"); 

        }
        {
            static const UChar iso_2022_cn_inputText6[]={ 0x3000, 0xD84D, 0xDC56, 0x3001,0xD84D,0xDC56, 0x0042,0x0902};
            static const uint8_t to_iso_2022_cn6_v2[]={  
                                0x1b,   0x24,   0x29,   0x41,   0x0e,   0x21,   0x21,   
                                0x0f,   0x7b,   0x55,   0x2b,   0x32,   0x33,   0x34,   0x35,   0x36,   0x7d,   
                                0x0e,   0x21,   0x22,   
                                0x0f,   0x7b,   0x55,   0x2b,   0x32,   0x33,   0x34,   0x35,   0x36,   0x7d,   
                                0x42,   
                                0x7b,   0x55,   0x2b,   0x30,   0x39,   0x30,   0x32,   0x7d
                             };
            static const int32_t from_iso_2022_cnOffs6_v2 [] ={ 
                    0,  0,  0,  0,  0,  0,  0,  
                    1,  1,  1,  1,  1,  1,  1,  1,  1,  1,  
                    3,  3,  3,  
                    4,  4,  4,  4,  4,  4,  4,  4,  4,  4,  
                    6, 
                    7,  7,  7,  7,  7,  7,  7,  7,
            };
            if(!testConvertFromUnicodeWithContext(iso_2022_cn_inputText6, sizeof(iso_2022_cn_inputText6)/sizeof(iso_2022_cn_inputText6[0]),
                to_iso_2022_cn6_v2, sizeof(to_iso_2022_cn6_v2), "iso-2022-cn",
                UCNV_FROM_U_CALLBACK_ESCAPE, from_iso_2022_cnOffs6_v2, NULL, 0,UCNV_ESCAPE_UNICODE,U_ZERO_ERROR ))
                log_err("u-> iso-2022-cn with sub & UCNV_ESCAPE_UNICODE did not match.\n"); 

        }
        {
            static const UChar iso_2022_cn_inputText7[]={ 0x3000, 0xD84D, 0xDC56, 0x3001,0xD84D,0xDC56, 0x0042,0x0902};
            static const uint8_t to_iso_2022_cn7_v2[]={  
                                0x1b,   0x24,   0x29,   0x41,   0x0e,   0x21,   0x21,   
                                0x0f,   0x25,   0x55,   0x44,   0x38,   0x34,   0x44,   0x25,   0x55,   0x44,   0x43,   0x35,   0x36,   
                                0x0e,   0x21,   0x22,   
                                0x0f,   0x25,   0x55,   0x44,   0x38,   0x34,   0x44,   0x25,   0x55,   0x44,   0x43,   0x35,   0x36,   
                                0x42,   0x25,   0x55,   0x30,   0x39,   0x30,   0x32, 
                            };
            static const int32_t from_iso_2022_cnOffs7_v2 [] ={ 
                                0,  0,  0,  0,  0,  0,  0,  
                                1,  1,  1,  1,  1,  1,  1,  1,  1,  1,  1,  1,  1,  
                                3,  3,  3,  
                                4,  4,  4,  4,  4,  4,  4,  4,  4,  4,  4,  4,  4,  
                                6,  
                                7,  7,  7,  7,  7,  7,
            };
            if(!testConvertFromUnicodeWithContext(iso_2022_cn_inputText7, sizeof(iso_2022_cn_inputText7)/sizeof(iso_2022_cn_inputText7[0]),
                to_iso_2022_cn7_v2, sizeof(to_iso_2022_cn7_v2), "iso-2022-cn",
                UCNV_FROM_U_CALLBACK_ESCAPE, from_iso_2022_cnOffs7_v2, NULL, 0,"K" ,U_ZERO_ERROR ))
                log_err("u-> iso-2022-cn with sub & K did not match.\n"); 

        }
        {
            static const UChar iso_2022_cn_inputText8[]={
                                0x3000,
                                0xD84D, 0xDC56,
                                0x3001,
                                0xD84D, 0xDC56,
                                0xDBFF, 0xDFFF,
                                0x0042,
                                0x0902};
            static const uint8_t to_iso_2022_cn8_v2[]={  
                                0x1b,   0x24,   0x29,   0x41,   0x0e,   0x21,   0x21,   
                                0x0f,   0x5c,   0x32,   0x33,   0x34,   0x35,   0x36,   0x20,   
                                0x0e,   0x21,   0x22,   
                                0x0f,   0x5c,   0x32,   0x33,   0x34,   0x35,   0x36,   0x20,   
                                0x5c,   0x31,   0x30,   0x46,   0x46,   0x46,   0x46,   0x20,   
                                0x42,   
                                0x5c,   0x39,   0x30,   0x32,   0x20
                             };
            static const int32_t from_iso_2022_cnOffs8_v2 [] ={
                    0,  0,  0,  0,  0,  0,  0,
                    1,  1,  1,  1,  1,  1,  1,  1,
                    3,  3,  3,
                    4,  4,  4,  4,  4,  4,  4,  4,
                    6,  6,  6,  6,  6,  6,  6,  6,
                    8,
                    9,  9,  9,  9,  9
            };
            if(!testConvertFromUnicodeWithContext(iso_2022_cn_inputText8, sizeof(iso_2022_cn_inputText8)/sizeof(iso_2022_cn_inputText8[0]),
                to_iso_2022_cn8_v2, sizeof(to_iso_2022_cn8_v2), "iso-2022-cn",
                UCNV_FROM_U_CALLBACK_ESCAPE, from_iso_2022_cnOffs8_v2, NULL, 0,UCNV_ESCAPE_CSS2,U_ZERO_ERROR ))
                log_err("u-> iso-2022-cn with sub & UCNV_ESCAPE_CSS2 did not match.\n"); 

        }
        {
            static const uint8_t to_iso_2022_cn4_v3[]={  
                            0x1b,   0x24,   0x29,   0x41,   0x0e,   0x21,   0x21,   
                            0x0f,   0x5c,   0x55,   0x30,   0x30,   0x30,   0x32,   0x33,   0x34,   0x35,   0x36,   
                            0x0e,   0x21,   0x22,
                            0x0f,   0x5c,   0x55,   0x30,   0x30,   0x30,   0x32,   0x33,   0x34,   0x35,   0x36, 
                            0x42 
                             };


            static const int32_t from_iso_2022_cnOffs4_v3 [] ={ 
                0,0,0,0,0,0,0,
                1,1,1,1,1,1,1,1,1,1,1,

                3,3,3,
                4,4,4,4,4,4,4,4,4,4,4,

                6

            };
            if(!testConvertFromUnicodeWithContext(iso_2022_cn_inputText4, sizeof(iso_2022_cn_inputText4)/sizeof(iso_2022_cn_inputText4[0]),
                to_iso_2022_cn4_v3, sizeof(to_iso_2022_cn4_v3), "iso-2022-cn",
                UCNV_FROM_U_CALLBACK_ESCAPE, from_iso_2022_cnOffs4_v3, NULL, 0,UCNV_ESCAPE_C,U_ZERO_ERROR ))
            {
                log_err("u-> iso-2022-cn with skip & UCNV_ESCAPE_C did not match.\n"); 
            }
        }
        if(!testConvertFromUnicode(iso_2022_cn_inputText, sizeof(iso_2022_cn_inputText)/sizeof(iso_2022_cn_inputText[0]),
                to_iso_2022_cn, sizeof(to_iso_2022_cn), "iso-2022-cn",
                UCNV_FROM_U_CALLBACK_ESCAPE, from_iso_2022_cnOffs, NULL, 0 ))
            log_err("u-> iso_2022_cn with subst with value did not match.\n");

        if(!testConvertFromUnicode(iso_2022_cn_inputText4, sizeof(iso_2022_cn_inputText4)/sizeof(iso_2022_cn_inputText4[0]),
                to_iso_2022_cn4, sizeof(to_iso_2022_cn4), "iso-2022-cn",
                UCNV_FROM_U_CALLBACK_ESCAPE, from_iso_2022_cnOffs4, NULL, 0 ))
            log_err("u-> iso_2022_cn with subst with value did not match.\n");
        if(!testConvertFromUnicode(iso_2022_kr_inputText, sizeof(iso_2022_kr_inputText)/sizeof(iso_2022_kr_inputText[0]),
                to_iso_2022_kr, sizeof(to_iso_2022_kr), "iso-2022-kr",
                UCNV_FROM_U_CALLBACK_ESCAPE, from_iso_2022_krOffs, NULL, 0 ))
            log_err("u-> iso_2022_kr with subst with value did not match.\n");
        if(!testConvertFromUnicode(iso_2022_kr_inputText2, sizeof(iso_2022_kr_inputText2)/sizeof(iso_2022_kr_inputText2[0]),
                to_iso_2022_kr2, sizeof(to_iso_2022_kr2), "iso-2022-kr",
                UCNV_FROM_U_CALLBACK_ESCAPE, from_iso_2022_krOffs2, NULL, 0 ))
            log_err("u-> iso_2022_kr2 with subst with value did not match.\n");
        if(!testConvertFromUnicode(hz_inputText, sizeof(hz_inputText)/sizeof(hz_inputText[0]),
                to_hz, sizeof(to_hz), "HZ",
                UCNV_FROM_U_CALLBACK_ESCAPE, from_hzOffs, NULL, 0 ))
            log_err("u-> hz with subst with value did not match.\n");
        if(!testConvertFromUnicode(hz_inputText2, sizeof(hz_inputText2)/sizeof(hz_inputText2[0]),
                to_hz2, sizeof(to_hz2), "HZ",
                UCNV_FROM_U_CALLBACK_ESCAPE, from_hzOffs2, NULL, 0 ))
            log_err("u-> hz with subst with value did not match.\n");

        if(!testConvertFromUnicode(iscii_inputText, sizeof(iscii_inputText)/sizeof(iscii_inputText[0]),
                to_iscii, sizeof(to_iscii), "ISCII,version=0",
                UCNV_FROM_U_CALLBACK_ESCAPE, from_isciiOffs, NULL, 0 ))
            log_err("u-> iscii with subst with value did not match.\n");
    }
#endif

    log_verbose("Testing toUnicode with UCNV_TO_U_CALLBACK_ESCAPE \n");
    /*to Unicode*/
    {
#if !UCONFIG_NO_LEGACY_CONVERSION
        static const uint8_t sampleTxtToU[]= { 0x00, 0x9f, 0xaf, 
            0x81, 0xad, /*unassigned*/
            0x89, 0xd3 };
        static const UChar IBM_943toUnicode[] = { 0x0000, 0x6D63, 
            0x25, 0x58, 0x38, 0x31, 0x25, 0x58, 0x41, 0x44,
            0x7B87};
        static const int32_t  fromIBM943Offs [] =    { 0, 1, 3, 3, 3, 3, 3, 3, 3, 3, 5};

        /* EUC_JP*/
        static const uint8_t sampleTxt_EUC_JP[]={ 0x61, 0xa1, 0xb8, 0x8f, 0xf4, 0xae,
            0x8f, 0xda, 0xa1,  /*unassigned*/
           0x8e, 0xe0,
        };
        static const UChar EUC_JPtoUnicode[]={ 0x0061, 0x4edd, 0x5bec,
            0x25, 0x58, 0x38, 0x46, 0x25, 0x58, 0x44, 0x41, 0x25, 0x58, 0x41, 0x31, 
            0x00a2 };
        static const int32_t fromEUC_JPOffs [] ={ 0, 1, 3, 
            6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6,
            9,
        };

        /*EUC_TW*/
        static const uint8_t sampleTxt_euc_tw[]={ 
            0x61, 0xa2, 0xd3, 0x8e, 0xa2, 0xdc, 0xe5,
            0x8e, 0xaa, 0xbb, 0xcc,/*unassigned*/
            0xe6, 0xca, 0x8a,
        };
        static const UChar euc_twtoUnicode[]={ 0x0061, 0x2295, 0x5BF2, 
             0x25, 0x58, 0x38, 0x45, 0x25, 0x58, 0x41, 0x41, 0x25, 0x58, 0x42, 0x42, 0x25, 0x58, 0x43, 0x43, 
             0x8706, 0x8a, };
        static const int32_t from_euc_twOffs [] ={ 0, 1, 3, 
             7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7,
             11, 13};
        
        /*iso-2022-jp*/
        static const uint8_t sampleTxt_iso_2022_jp[]={ 
            0x1b,   0x28,   0x42,   0x41,
            0x1b,   0x24,   0x42,   0x2A, 0x44, /*unassigned*/
            0x1b,   0x28,   0x42,   0x42,
            
        };
        static const UChar iso_2022_jptoUnicode[]={    0x41,0x25,0x58,0x32,0x41,0x25,0x58,0x34,0x34, 0x42 };
        static const int32_t from_iso_2022_jpOffs [] ={  3,   7,   7,   7,   7,   7,   7,   7,   7,    12   };
        
        /*iso-2022-cn*/
        static const uint8_t sampleTxt_iso_2022_cn[]={ 
            0x0f,   0x41,   0x44,
            0x1B,   0x24,   0x29,   0x47, 
            0x0E,   0x40,   0x6c, /*unassigned*/
            0x0f,   0x42,
            
        };
        static const UChar iso_2022_cntoUnicode[]={    0x41, 0x44,0x25,0x58,0x34,0x30,0x25,0x58,0x36,0x43,0x42 };
        static const int32_t from_iso_2022_cnOffs [] ={  1,   2,   8,   8,   8,   8,   8,   8,   8,  8,    11   };

        /*iso-2022-kr*/
        static const uint8_t sampleTxt_iso_2022_kr[]={ 
          0x1b, 0x24, 0x29,  0x43,
          0x41,
          0x0E, 0x7f, 0x1E,
          0x0e, 0x25, 0x50, 
          0x0f, 0x51,
          0x42, 0x43,
            
        };
        static const UChar iso_2022_krtoUnicode[]={     0x41,0x25,0x58,0x37,0x46,0x25,0x58,0x31,0x45,0x03A0,0x51, 0x42,0x43};
        static const int32_t from_iso_2022_krOffs [] ={  4,   6,   6,   6,   6,   6,   6,   6,   6,    9,    12,   13  , 14 };
        
        /*hz*/
        static const uint8_t sampleTxt_hz[]={ 
            0x41,   
            0x7e,   0x7b,   0x26,   0x30,   
            0x7f,   0x1E, /*unassigned*/ 
            0x26,   0x30,
            0x7e,   0x7d,   0x42, 
            0x7e,   0x7b,   0x7f,   0x1E,/*unassigned*/ 
            0x7e,   0x7d,   0x42,
        };
        static const UChar hztoUnicode[]={  
            0x41,
            0x03a0,
            0x25,0x58,0x37,0x46,0x25,0x58,0x31,0x45,
            0x03A0, 
            0x42,
            0x25,0x58,0x37,0x46,0x25,0x58,0x31,0x45,
            0x42,};

        static const int32_t from_hzOffs [] ={0,3,5,5,5,5,5,5,5,5,7,11,14,14,14,14,14,14,14,14,18,  };

       
        /*iscii*/
        static const uint8_t sampleTxt_iscii[]={ 
            0x41,   
            0x30,   
            0xEB, /*unassigned*/ 
            0xa3,
            0x42, 
            0xEC, /*unassigned*/ 
            0x42,
        };
        static const UChar isciitoUnicode[]={  
            0x41,
            0x30,
            0x25,  0x58,  0x45, 0x42,
            0x0903, 
            0x42,
            0x25,  0x58,  0x45, 0x43,
            0x42,};

        static const int32_t from_isciiOffs [] ={0,1,2,2,2,2,3,4,5,5,5,5,6  };
#endif

        /*UTF8*/
        static const uint8_t sampleTxtUTF8[]={
            0x20, 0x64, 0x50, 
            0xC2, 0x7E, /* truncated char */
            0x20,
            0xE0, 0xB5, 0x7E, /* truncated char */
            0x40,
        };
        static const UChar UTF8ToUnicode[]={
            0x0020, 0x0064, 0x0050,
            0x0025, 0x0058, 0x0043, 0x0032, 0x007E,  /* \xC2~ */
            0x0020,
            0x0025, 0x0058, 0x0045, 0x0030, 0x0025, 0x0058, 0x0042, 0x0035, 0x007E,
            0x0040
        };
        static const int32_t fromUTF8[] = {
            0, 1, 2,
            3, 3, 3, 3, 4,
            5,
            6, 6, 6, 6, 6, 6, 6, 6, 8,
            9
        };
        static const UChar UTF8ToUnicodeXML_DEC[]={
            0x0020, 0x0064, 0x0050,
            0x0026, 0x0023, 0x0031, 0x0039, 0x0034, 0x003B, 0x007E,  /* &#194;~ */
            0x0020,
            0x0026, 0x0023, 0x0032, 0x0032, 0x0034, 0x003B, 0x0026, 0x0023, 0x0031, 0x0038, 0x0031, 0x003B, 0x007E,
            0x0040
        };
        static const int32_t fromUTF8XML_DEC[] = {
            0, 1, 2,
            3, 3, 3, 3, 3, 3, 4,
            5,
            6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 8,
            9
        };


#if !UCONFIG_NO_LEGACY_CONVERSION        
        if(!testConvertToUnicode(sampleTxtToU, sizeof(sampleTxtToU),
                 IBM_943toUnicode, sizeof(IBM_943toUnicode)/sizeof(IBM_943toUnicode[0]),"ibm-943",
                UCNV_TO_U_CALLBACK_ESCAPE, fromIBM943Offs, NULL, 0 ))
            log_err("ibm-943->u with substitute with value did not match.\n");

        if(!testConvertToUnicode(sampleTxt_EUC_JP, sizeof(sampleTxt_EUC_JP),
                 EUC_JPtoUnicode, sizeof(EUC_JPtoUnicode)/sizeof(EUC_JPtoUnicode[0]),"euc-jp",
                UCNV_TO_U_CALLBACK_ESCAPE, fromEUC_JPOffs, NULL, 0))
            log_err("euc-jp->u with substitute with value did not match.\n");

        if(!testConvertToUnicode(sampleTxt_euc_tw, sizeof(sampleTxt_euc_tw),
                 euc_twtoUnicode, sizeof(euc_twtoUnicode)/sizeof(euc_twtoUnicode[0]),"euc-tw",
                UCNV_TO_U_CALLBACK_ESCAPE, from_euc_twOffs, NULL, 0))
            log_err("euc-tw->u with substitute with value did not match.\n");
            
        if(!testConvertToUnicode(sampleTxt_iso_2022_jp, sizeof(sampleTxt_iso_2022_jp),
                 iso_2022_jptoUnicode, sizeof(iso_2022_jptoUnicode)/sizeof(iso_2022_jptoUnicode[0]),"iso-2022-jp",
                UCNV_TO_U_CALLBACK_ESCAPE, from_iso_2022_jpOffs, NULL, 0))
            log_err("iso-2022-jp->u with substitute with value did not match.\n");
       
        if(!testConvertToUnicodeWithContext(sampleTxt_iso_2022_jp, sizeof(sampleTxt_iso_2022_jp),
                 iso_2022_jptoUnicode, sizeof(iso_2022_jptoUnicode)/sizeof(iso_2022_jptoUnicode[0]),"iso-2022-jp",
                UCNV_TO_U_CALLBACK_ESCAPE, from_iso_2022_jpOffs, NULL, 0,"K",U_ZERO_ERROR))
            log_err("iso-2022-jp->u with substitute with value did not match.\n");
        
        {/* test UCNV_TO_U_CALLBACK_ESCAPE with options */
            {
                static const UChar iso_2022_jptoUnicodeDec[]={
                                                  0x0041,   
                                                  0x0026,   0x0023,   0x0034,   0x0032,   0x003b,   
                                                  0x0026,   0x0023,   0x0036,   0x0038,   0x003b,   
                                                  0x0042 };
                static const int32_t from_iso_2022_jpOffsDec [] ={ 3,7,7,7,7,7,7,7,7,7,7,12,  };
                if(!testConvertToUnicodeWithContext(sampleTxt_iso_2022_jp, sizeof(sampleTxt_iso_2022_jp),
                     iso_2022_jptoUnicodeDec, sizeof(iso_2022_jptoUnicodeDec)/sizeof(iso_2022_jptoUnicode[0]),"iso-2022-jp",
                    UCNV_TO_U_CALLBACK_ESCAPE, from_iso_2022_jpOffsDec, NULL, 0,UCNV_ESCAPE_XML_DEC,U_ZERO_ERROR ))
                log_err("iso-2022-jp->u with substitute with value and UCNV_ESCAPE_XML_DEC did not match.\n");
            }
            {
                static const UChar iso_2022_jptoUnicodeHex[]={
                                                  0x0041, 
                                                  0x0026, 0x0023, 0x0078, 0x0032, 0x0041, 0x003b, 
                                                  0x0026, 0x0023, 0x0078, 0x0034, 0x0034, 0x003b, 
                                                  0x0042 };
                static const int32_t from_iso_2022_jpOffsHex [] ={  3,7,7,7,7,7,7,7,7,7,7,7,7,12   };
                if(!testConvertToUnicodeWithContext(sampleTxt_iso_2022_jp, sizeof(sampleTxt_iso_2022_jp),
                     iso_2022_jptoUnicodeHex, sizeof(iso_2022_jptoUnicodeHex)/sizeof(iso_2022_jptoUnicode[0]),"iso-2022-jp",
                    UCNV_TO_U_CALLBACK_ESCAPE, from_iso_2022_jpOffsHex, NULL, 0,UCNV_ESCAPE_XML_HEX,U_ZERO_ERROR ))
                log_err("iso-2022-jp->u with substitute with value and UCNV_ESCAPE_XML_HEX did not match.\n");
            }
            {
                static const UChar iso_2022_jptoUnicodeC[]={
                                                0x0041, 
                                                0x005C, 0x0078, 0x0032, 0x0041,
                                                0x005C, 0x0078, 0x0034, 0x0034, 
                                                0x0042 };
                int32_t from_iso_2022_jpOffsC [] ={  3,7,7,7,7,7,7,7,7,12   };
                if(!testConvertToUnicodeWithContext(sampleTxt_iso_2022_jp, sizeof(sampleTxt_iso_2022_jp),
                     iso_2022_jptoUnicodeC, sizeof(iso_2022_jptoUnicodeC)/sizeof(iso_2022_jptoUnicode[0]),"iso-2022-jp",
                    UCNV_TO_U_CALLBACK_ESCAPE, from_iso_2022_jpOffsC, NULL, 0,UCNV_ESCAPE_C,U_ZERO_ERROR ))
                log_err("iso-2022-jp->u with substitute with value and UCNV_ESCAPE_C did not match.\n");
            }
        }
        if(!testConvertToUnicode(sampleTxt_iso_2022_cn, sizeof(sampleTxt_iso_2022_cn),
                 iso_2022_cntoUnicode, sizeof(iso_2022_cntoUnicode)/sizeof(iso_2022_cntoUnicode[0]),"iso-2022-cn",
                UCNV_TO_U_CALLBACK_ESCAPE, from_iso_2022_cnOffs, NULL, 0))
            log_err("iso-2022-cn->u with substitute with value did not match.\n");
        
        if(!testConvertToUnicode(sampleTxt_iso_2022_kr, sizeof(sampleTxt_iso_2022_kr),
                 iso_2022_krtoUnicode, sizeof(iso_2022_krtoUnicode)/sizeof(iso_2022_krtoUnicode[0]),"iso-2022-kr",
                UCNV_TO_U_CALLBACK_ESCAPE, from_iso_2022_krOffs, NULL, 0))
            log_err("iso-2022-kr->u with substitute with value did not match.\n");

         if(!testConvertToUnicode(sampleTxt_hz, sizeof(sampleTxt_hz),
                 hztoUnicode, sizeof(hztoUnicode)/sizeof(hztoUnicode[0]),"HZ",
                UCNV_TO_U_CALLBACK_ESCAPE, from_hzOffs, NULL, 0))
            log_err("hz->u with substitute with value did not match.\n");

         if(!testConvertToUnicode(sampleTxt_iscii, sizeof(sampleTxt_iscii),
                 isciitoUnicode, sizeof(isciitoUnicode)/sizeof(isciitoUnicode[0]),"ISCII,version=0",
                UCNV_TO_U_CALLBACK_ESCAPE, from_isciiOffs, NULL, 0))
            log_err("ISCII ->u with substitute with value did not match.\n");
#endif

        if(!testConvertToUnicode(sampleTxtUTF8, sizeof(sampleTxtUTF8),
                UTF8ToUnicode, sizeof(UTF8ToUnicode)/sizeof(UTF8ToUnicode[0]),"UTF-8",
                UCNV_TO_U_CALLBACK_ESCAPE, fromUTF8, NULL, 0))
            log_err("UTF8->u with UCNV_TO_U_CALLBACK_ESCAPE with value did not match.\n"); 
        if(!testConvertToUnicodeWithContext(sampleTxtUTF8, sizeof(sampleTxtUTF8),
                UTF8ToUnicodeXML_DEC, sizeof(UTF8ToUnicodeXML_DEC)/sizeof(UTF8ToUnicodeXML_DEC[0]),"UTF-8",
                UCNV_TO_U_CALLBACK_ESCAPE, fromUTF8XML_DEC, NULL, 0, UCNV_ESCAPE_XML_DEC, U_ZERO_ERROR))
            log_err("UTF8->u with UCNV_TO_U_CALLBACK_ESCAPE with value did not match.\n"); 
    }
}

#if !UCONFIG_NO_LEGACY_CONVERSION
static void TestLegalAndOthers(int32_t inputsize, int32_t outputsize)
{
    static const UChar    legalText[] =  { 0x0000, 0xAC00, 0xAC01, 0xD700 };
    static const uint8_t templegal949[] ={ 0x00, 0xb0, 0xa1, 0xb0, 0xa2, 0xc8, 0xd3 };
    static const int32_t  to949legal[] = {0, 1, 1, 2, 2, 3, 3};


    static const uint8_t text943[] = {
        0x82, 0xa9, 0x82, 0x20, 0x61, 0x8a, 0xbf, 0x8e, 0x9a };
    static const UChar toUnicode943sub[] = { 0x304b, 0x1a, 0x20, 0x0061, 0x6f22,  0x5b57 };
    static const UChar toUnicode943skip[]= { 0x304b, 0x20, 0x0061, 0x6f22,  0x5b57 };
    static const UChar toUnicode943stop[]= { 0x304b};

    static const int32_t  fromIBM943Offssub[]  = { 0, 2, 3, 4, 5, 7 };
    static const int32_t  fromIBM943Offsskip[] = { 0, 3, 4, 5, 7 };
    static const int32_t  fromIBM943Offsstop[] = { 0};

    gInBufferSize = inputsize;
    gOutBufferSize = outputsize;
    /*checking with a legal value*/
    if(!testConvertFromUnicode(legalText, sizeof(legalText)/sizeof(legalText[0]),
            templegal949, sizeof(templegal949), "ibm-949",
            UCNV_FROM_U_CALLBACK_SKIP, to949legal, NULL, 0 ))
        log_err("u-> ibm-949 with skip did not match.\n");

    /*checking illegal value for ibm-943 with substitute*/ 
    if(!testConvertToUnicode(text943, sizeof(text943),
             toUnicode943sub, sizeof(toUnicode943sub)/sizeof(toUnicode943sub[0]),"ibm-943",
            UCNV_TO_U_CALLBACK_SUBSTITUTE, fromIBM943Offssub, NULL, 0 ))
        log_err("ibm-943->u with subst did not match.\n");
    /*checking illegal value for ibm-943 with skip */
    if(!testConvertToUnicode(text943, sizeof(text943),
             toUnicode943skip, sizeof(toUnicode943skip)/sizeof(toUnicode943skip[0]),"ibm-943",
            UCNV_TO_U_CALLBACK_SKIP, fromIBM943Offsskip, NULL, 0 ))
        log_err("ibm-943->u with skip did not match.\n");

    /*checking illegal value for ibm-943 with stop */
    if(!testConvertToUnicode(text943, sizeof(text943),
             toUnicode943stop, sizeof(toUnicode943stop)/sizeof(toUnicode943stop[0]),"ibm-943",
            UCNV_TO_U_CALLBACK_STOP, fromIBM943Offsstop, NULL, 0 ))
        log_err("ibm-943->u with stop did not match.\n");

}

static void TestSingleByte(int32_t inputsize, int32_t outputsize)
{
    static const uint8_t sampleText[] = {
        0x82, 0xa9, 0x61, 0x62, 0x63 , 0x82,
        0xff, 0x32, 0x33};
    static const UChar toUnicode943sub[] = { 0x304b, 0x0061, 0x0062, 0x0063, 0x1a, 0x1a, 0x0032, 0x0033 };
    static const int32_t fromIBM943Offssub[] = { 0, 2, 3, 4, 5, 6, 7, 8 };
    /*checking illegal value for ibm-943 with substitute*/ 
    gInBufferSize = inputsize;
    gOutBufferSize = outputsize;

    if(!testConvertToUnicode(sampleText, sizeof(sampleText),
             toUnicode943sub, sizeof(toUnicode943sub)/sizeof(toUnicode943sub[0]),"ibm-943",
            UCNV_TO_U_CALLBACK_SUBSTITUTE, fromIBM943Offssub, NULL, 0 ))
        log_err("ibm-943->u with subst did not match.\n");
}

static void TestEBCDIC_STATEFUL_Sub(int32_t inputsize, int32_t outputsize)
{
    /*EBCDIC_STATEFUL*/
    static const UChar ebcdic_inputTest[] = { 0x0061, 0x6d64, 0x0061, 0x00A2, 0x6d65, 0x0061 };
    static const uint8_t toIBM930[]= { 0x62, 0x0e, 0x5d, 0x63, 0x0f, 0x62, 0xb1, 0x0e, 0xfe, 0xfe, 0x0f, 0x62 };
    static const int32_t offset_930[]=     { 0,    1,    1,    1,    2,    2,    3,    4,    4,    4,    5,    5    };
/*                              s     SO    doubl       SI    sng   s     SO    fe    fe    SI    s    */

    /*EBCDIC_STATEFUL with subChar=3f*/
    static const uint8_t toIBM930_subvaried[]= { 0x62, 0x0e, 0x5d, 0x63, 0x0f, 0x62, 0xb1, 0x3f, 0x62 };
    static const int32_t offset_930_subvaried[]=     { 0,    1,    1,    1,    2,    2,    3,    4,    5    };
    static const char mySubChar[]={ 0x3f};

    gInBufferSize = inputsize;
    gOutBufferSize = outputsize;

    if(!testConvertFromUnicode(ebcdic_inputTest, sizeof(ebcdic_inputTest)/sizeof(ebcdic_inputTest[0]),
        toIBM930, sizeof(toIBM930), "ibm-930",
        UCNV_FROM_U_CALLBACK_SUBSTITUTE, offset_930, NULL, 0 ))
            log_err("u-> ibm-930(EBCDIC_STATEFUL) with subst did not match.\n");
    
    if(!testConvertFromUnicode(ebcdic_inputTest, sizeof(ebcdic_inputTest)/sizeof(ebcdic_inputTest[0]),
        toIBM930_subvaried, sizeof(toIBM930_subvaried), "ibm-930",
        UCNV_FROM_U_CALLBACK_SUBSTITUTE, offset_930_subvaried, mySubChar, 1 ))
            log_err("u-> ibm-930(EBCDIC_STATEFUL) with subst(setSubChar=0x3f) did not match.\n");
}
#endif

UBool testConvertFromUnicode(const UChar *source, int sourceLen,  const uint8_t *expect, int expectLen, 
                const char *codepage, UConverterFromUCallback callback , const int32_t *expectOffsets, 
                const char *mySubChar, int8_t len)
{


    UErrorCode status = U_ZERO_ERROR;
    UConverter *conv = 0;
    char junkout[NEW_MAX_BUFFER]; /* FIX */
    int32_t junokout[NEW_MAX_BUFFER]; /* FIX */
    const UChar *src;
    char *end;
    char *targ;
    int32_t *offs;
    int i;
    int32_t  realBufferSize;
    char *realBufferEnd;
    const UChar *realSourceEnd;
    const UChar *sourceLimit;
    UBool checkOffsets = TRUE;
    UBool doFlush;
    char junk[9999];
    char offset_str[9999];
    char *p;
    UConverterFromUCallback oldAction = NULL;
    const void* oldContext = NULL;


    for(i=0;i<NEW_MAX_BUFFER;i++)
        junkout[i] = (char)0xF0;
    for(i=0;i<NEW_MAX_BUFFER;i++)
        junokout[i] = 0xFF;
    setNuConvTestName(codepage, "FROM");

    log_verbose("\nTesting========= %s  FROM \n  inputbuffer= %d   outputbuffer= %d\n", codepage, gInBufferSize, 
            gOutBufferSize);

    conv = ucnv_open(codepage, &status);
    if(U_FAILURE(status))
    {
        log_data_err("Couldn't open converter %s\n",codepage);
        return TRUE;
    }

    log_verbose("Converter opened..\n");

    /*----setting the callback routine----*/
    ucnv_setFromUCallBack (conv, callback, NULL, &oldAction, &oldContext, &status);
    if (U_FAILURE(status)) 
    { 
        log_err("FAILURE in setting the callback Function! %s\n", myErrorName(status));
    }
    /*------------------------*/
    /*setting the subChar*/
    if(mySubChar != NULL){
        ucnv_setSubstChars(conv, mySubChar, len, &status);
        if (U_FAILURE(status))  { 
            log_err("FAILURE in setting the callback Function! %s\n", myErrorName(status));
        }
    }
    /*------------*/

    src = source;
    targ = junkout;
    offs = junokout;

    realBufferSize = (sizeof(junkout)/sizeof(junkout[0]));
    realBufferEnd = junkout + realBufferSize;
    realSourceEnd = source + sourceLen;

    if ( gOutBufferSize != realBufferSize )
      checkOffsets = FALSE;

    if( gInBufferSize != NEW_MAX_BUFFER )
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

        ucnv_fromUnicode (conv,
                  (char **)&targ,
                  (const char *)end,
                  &src,
                  sourceLimit,
                  checkOffsets ? offs : NULL,
                  doFlush, /* flush if we're at the end of the input data */
                  &status);
    } while ( (status == U_BUFFER_OVERFLOW_ERROR) || (U_SUCCESS(status) && (sourceLimit < realSourceEnd)) );


    if(status==U_INVALID_CHAR_FOUND || status == U_ILLEGAL_CHAR_FOUND){
        UChar errChars[50]; /* should be sufficient */
        int8_t errLen = 50;
        UErrorCode err = U_ZERO_ERROR;
        const UChar* limit= NULL;
        const UChar* start= NULL;
        ucnv_getInvalidUChars(conv,errChars, &errLen, &err);
        if(U_FAILURE(err)){
            log_err("ucnv_getInvalidUChars failed with error : %s\n",u_errorName(err));
        }
        /* src points to limit of invalid chars */
        limit = src;
        /* length of in invalid chars should be equal to returned length*/
        start = src - errLen;
        if(u_strncmp(errChars,start,errLen)!=0){
            log_err("ucnv_getInvalidUChars did not return the correct invalid chars for encoding %s \n", ucnv_getName(conv,&err));
        }
    }
    /* allow failure codes for the stop callback */
    if(U_FAILURE(status) &&
       (callback != UCNV_FROM_U_CALLBACK_STOP || (status != U_INVALID_CHAR_FOUND && status != U_ILLEGAL_CHAR_FOUND)))
    {
        log_err("Problem in fromUnicode, errcode %s %s\n", myErrorName(status), gNuConvTestName);
        return FALSE;
    }

    log_verbose("\nConversion done [%d uchars in -> %d chars out]. \nResult :",
        sourceLen, targ-junkout);
    if(getTestOption(VERBOSITY_OPTION))
    {

        junk[0] = 0;
        offset_str[0] = 0;
        for(p = junkout;p<targ;p++)
        {
            sprintf(junk + strlen(junk), "0x%02x, ", (0xFF) & (unsigned int)*p);
            sprintf(offset_str + strlen(offset_str), "0x%02x, ", (0xFF) & (unsigned int)junokout[p-junkout]);
        }

        log_verbose(junk);
        printSeq(expect, expectLen);
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
        printSeqErr((const uint8_t *)junkout, (int32_t)(targ-junkout));
        printSeqErr(expect, expectLen);
        return FALSE;
    }

    if (checkOffsets && (expectOffsets != 0) )
    {
        log_verbose("comparing %d offsets..\n", targ-junkout);
        if(memcmp(junokout,expectOffsets,(targ-junkout) * sizeof(int32_t) )){
            log_err("did not get the expected offsets while %s \n", gNuConvTestName);
            log_err("Got Output : ");
            printSeqErr((const uint8_t *)junkout, (int32_t)(targ-junkout));
            log_err("Got Offsets:      ");
            for(p=junkout;p<targ;p++)
                log_err("%d,", junokout[p-junkout]); 
            log_err("\n");
            log_err("Expected Offsets: ");
            for(i=0; i<(targ-junkout); i++)
                log_err("%d,", expectOffsets[i]);
            log_err("\n");
            return FALSE;
        }
    }

    if(!memcmp(junkout, expect, expectLen))
    {
        log_verbose("String matches! %s\n", gNuConvTestName);
        return TRUE;
    }
    else
    {
        log_err("String does not match. %s\n", gNuConvTestName);
        log_err("source: ");
        printUSeqErr(source, sourceLen);
        log_err("Got:      ");
        printSeqErr((const uint8_t *)junkout, expectLen);
        log_err("Expected: ");
        printSeqErr(expect, expectLen);
        return FALSE;
    }
}

UBool testConvertToUnicode( const uint8_t *source, int sourcelen, const UChar *expect, int expectlen, 
               const char *codepage, UConverterToUCallback callback, const int32_t *expectOffsets,
               const char *mySubChar, int8_t len)
{
    UErrorCode status = U_ZERO_ERROR;
    UConverter *conv = 0;
    UChar   junkout[NEW_MAX_BUFFER]; /* FIX */
    int32_t junokout[NEW_MAX_BUFFER]; /* FIX */
    const char *src;
    const char *realSourceEnd;
    const char *srcLimit;
    UChar *targ;
    UChar *end;
    int32_t *offs;
    int i;
    UBool   checkOffsets = TRUE;
    char junk[9999];
    char offset_str[9999];
    UChar *p;
    UConverterToUCallback oldAction = NULL;
    const void* oldContext = NULL;

    int32_t   realBufferSize;
    UChar *realBufferEnd;


    for(i=0;i<NEW_MAX_BUFFER;i++)
        junkout[i] = 0xFFFE;

    for(i=0;i<NEW_MAX_BUFFER;i++)
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

    src = (const char *)source;
    targ = junkout;
    offs = junokout;

    realBufferSize = (sizeof(junkout)/sizeof(junkout[0]));
    realBufferEnd = junkout + realBufferSize;
    realSourceEnd = src + sourcelen;
    /*----setting the callback routine----*/
    ucnv_setToUCallBack (conv, callback, NULL, &oldAction, &oldContext, &status);
    if (U_FAILURE(status)) 
    { 
        log_err("FAILURE in setting the callback Function! %s\n", myErrorName(status));  
    }
    /*-------------------------------------*/
    /*setting the subChar*/
    if(mySubChar != NULL){
        ucnv_setSubstChars(conv, mySubChar, len, &status);
        if (U_FAILURE(status))  { 
            log_err("FAILURE in setting the callback Function! %s\n", myErrorName(status)); 
        }
    }
    /*------------*/


    if ( gOutBufferSize != realBufferSize )
        checkOffsets = FALSE;

    if( gInBufferSize != NEW_MAX_BUFFER )
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



        status = U_ZERO_ERROR;

        ucnv_toUnicode (conv,
                &targ,
                end,
                (const char **)&src,
                (const char *)srcLimit,
                checkOffsets ? offs : NULL,
                (UBool)(srcLimit == realSourceEnd), /* flush if we're at the end of the source data */
                &status);
    } while ( (status == U_BUFFER_OVERFLOW_ERROR) || (U_SUCCESS(status) && (srcLimit < realSourceEnd)) ); /* while we just need another buffer */

    if(status==U_INVALID_CHAR_FOUND || status == U_ILLEGAL_CHAR_FOUND){
        char errChars[50]; /* should be sufficient */
        int8_t errLen = 50;
        UErrorCode err = U_ZERO_ERROR;
        const char* limit= NULL;
        const char* start= NULL;
        ucnv_getInvalidChars(conv,errChars, &errLen, &err);
        if(U_FAILURE(err)){
            log_err("ucnv_getInvalidChars failed with error : %s\n",u_errorName(err));
        }
        /* src points to limit of invalid chars */
        limit = src;
        /* length of in invalid chars should be equal to returned length*/
        start = src - errLen;
        if(uprv_strncmp(errChars,start,errLen)!=0){
            log_err("ucnv_getInvalidChars did not return the correct invalid chars for encoding %s \n", ucnv_getName(conv,&err));
        }
    }
    /* allow failure codes for the stop callback */
    if(U_FAILURE(status) &&
       (callback != UCNV_TO_U_CALLBACK_STOP || (status != U_INVALID_CHAR_FOUND && status != U_ILLEGAL_CHAR_FOUND && status != U_TRUNCATED_CHAR_FOUND)))
    {
        log_err("Problem doing toUnicode, errcode %s %s\n", myErrorName(status), gNuConvTestName);
        return FALSE;
    }

    log_verbose("\nConversion done. %d bytes -> %d chars.\nResult :",
        sourcelen, targ-junkout);
    if(getTestOption(VERBOSITY_OPTION))
    {

        junk[0] = 0;
        offset_str[0] = 0;

        for(p = junkout;p<targ;p++)
        {
            sprintf(junk + strlen(junk), "0x%04x, ", (0xFFFF) & (unsigned int)*p);
            sprintf(offset_str + strlen(offset_str), "0x%04x, ", (0xFFFF) & (unsigned int)junokout[p-junkout]);
        }

        log_verbose(junk);
        printUSeq(expect, expectlen);
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
        if(memcmp(junokout,expectOffsets,(targ-junkout) * sizeof(int32_t)))
        {
            log_err("did not get the expected offsets while %s \n", gNuConvTestName);
            log_err("Got offsets:      ");
            for(p=junkout;p<targ;p++)
                log_err("  %2d,", junokout[p-junkout]); 
            log_err("\n");
            log_err("Expected offsets: ");
            for(i=0; i<(targ-junkout); i++)
                log_err("  %2d,", expectOffsets[i]);
            log_err("\n");
            log_err("Got output:       ");
            for(i=0; i<(targ-junkout); i++)
                log_err("0x%04x,", junkout[i]);
            log_err("\n");
            log_err("From source:      ");
            for(i=0; i<(src-(const char *)source); i++)
                log_err("  0x%02x,", (unsigned char)source[i]);
            log_err("\n");
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
        log_err("Got:      ");
        printUSeqErr(junkout, expectlen);
        log_err("Expected: ");
        printUSeqErr(expect, expectlen);
        log_err("\n");
        return FALSE;
    }
}

UBool testConvertFromUnicodeWithContext(const UChar *source, int sourceLen,  const uint8_t *expect, int expectLen, 
                const char *codepage, UConverterFromUCallback callback , const int32_t *expectOffsets, 
                const char *mySubChar, int8_t len, const void* context, UErrorCode expectedError)
{


    UErrorCode status = U_ZERO_ERROR;
    UConverter *conv = 0;
    char junkout[NEW_MAX_BUFFER]; /* FIX */
    int32_t junokout[NEW_MAX_BUFFER]; /* FIX */
    const UChar *src;
    char *end;
    char *targ;
    int32_t *offs;
    int i;
    int32_t  realBufferSize;
    char *realBufferEnd;
    const UChar *realSourceEnd;
    const UChar *sourceLimit;
    UBool checkOffsets = TRUE;
    UBool doFlush;
    char junk[9999];
    char offset_str[9999];
    char *p;
    UConverterFromUCallback oldAction = NULL;
    const void* oldContext = NULL;


    for(i=0;i<NEW_MAX_BUFFER;i++)
        junkout[i] = (char)0xF0;
    for(i=0;i<NEW_MAX_BUFFER;i++)
        junokout[i] = 0xFF;
    setNuConvTestName(codepage, "FROM");

    log_verbose("\nTesting========= %s  FROM \n  inputbuffer= %d   outputbuffer= %d\n", codepage, gInBufferSize, 
            gOutBufferSize);

    conv = ucnv_open(codepage, &status);
    if(U_FAILURE(status))
    {
        log_data_err("Couldn't open converter %s\n",codepage);
        return TRUE; /* Because the err has already been logged. */
    }

    log_verbose("Converter opened..\n");

    /*----setting the callback routine----*/
    ucnv_setFromUCallBack (conv, callback, context, &oldAction, &oldContext, &status);
    if (U_FAILURE(status)) 
    { 
        log_err("FAILURE in setting the callback Function! %s\n", myErrorName(status));
    }
    /*------------------------*/
    /*setting the subChar*/
    if(mySubChar != NULL){
        ucnv_setSubstChars(conv, mySubChar, len, &status);
        if (U_FAILURE(status))  { 
            log_err("FAILURE in setting substitution chars! %s\n", myErrorName(status));
        }
    }
    /*------------*/

    src = source;
    targ = junkout;
    offs = junokout;

    realBufferSize = (sizeof(junkout)/sizeof(junkout[0]));
    realBufferEnd = junkout + realBufferSize;
    realSourceEnd = source + sourceLen;

    if ( gOutBufferSize != realBufferSize )
      checkOffsets = FALSE;

    if( gInBufferSize != NEW_MAX_BUFFER )
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

        ucnv_fromUnicode (conv,
                  (char **)&targ,
                  (const char *)end,
                  &src,
                  sourceLimit,
                  checkOffsets ? offs : NULL,
                  doFlush, /* flush if we're at the end of the input data */
                  &status);
    } while ( (status == U_BUFFER_OVERFLOW_ERROR) || (U_SUCCESS(status) && (sourceLimit < realSourceEnd)) );

    /* allow failure codes for the stop callback */
    if(U_FAILURE(status) && status != expectedError)
    {
        log_err("Problem in fromUnicode, errcode %s %s\n", myErrorName(status), gNuConvTestName);
        return FALSE;
    }

    log_verbose("\nConversion done [%d uchars in -> %d chars out]. \nResult :",
        sourceLen, targ-junkout);
    if(getTestOption(VERBOSITY_OPTION))
    {

        junk[0] = 0;
        offset_str[0] = 0;
        for(p = junkout;p<targ;p++)
        {
            sprintf(junk + strlen(junk), "0x%02x, ", (0xFF) & (unsigned int)*p);
            sprintf(offset_str + strlen(offset_str), "0x%02x, ", (0xFF) & (unsigned int)junokout[p-junkout]);
        }

        log_verbose(junk);
        printSeq(expect, expectLen);
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
        printSeqErr((const uint8_t *)junkout, (int32_t)(targ-junkout));
        printSeqErr(expect, expectLen);
        return FALSE;
    }

    if (checkOffsets && (expectOffsets != 0) )
    {
        log_verbose("comparing %d offsets..\n", targ-junkout);
        if(memcmp(junokout,expectOffsets,(targ-junkout) * sizeof(int32_t) )){
            log_err("did not get the expected offsets while %s \n", gNuConvTestName);
            log_err("Got Output : ");
            printSeqErr((const uint8_t *)junkout, (int32_t)(targ-junkout));
            log_err("Got Offsets:      ");
            for(p=junkout;p<targ;p++)
                log_err("%d,", junokout[p-junkout]); 
            log_err("\n");
            log_err("Expected Offsets: ");
            for(i=0; i<(targ-junkout); i++)
                log_err("%d,", expectOffsets[i]);
            log_err("\n");
            return FALSE;
        }
    }

    if(!memcmp(junkout, expect, expectLen))
    {
        log_verbose("String matches! %s\n", gNuConvTestName);
        return TRUE;
    }
    else
    {
        log_err("String does not match. %s\n", gNuConvTestName);
        log_err("source: ");
        printUSeqErr(source, sourceLen);
        log_err("Got:      ");
        printSeqErr((const uint8_t *)junkout, expectLen);
        log_err("Expected: ");
        printSeqErr(expect, expectLen);
        return FALSE;
    }
}
UBool testConvertToUnicodeWithContext( const uint8_t *source, int sourcelen, const UChar *expect, int expectlen, 
               const char *codepage, UConverterToUCallback callback, const int32_t *expectOffsets,
               const char *mySubChar, int8_t len, const void* context, UErrorCode expectedError)
{
    UErrorCode status = U_ZERO_ERROR;
    UConverter *conv = 0;
    UChar   junkout[NEW_MAX_BUFFER]; /* FIX */
    int32_t junokout[NEW_MAX_BUFFER]; /* FIX */
    const char *src;
    const char *realSourceEnd;
    const char *srcLimit;
    UChar *targ;
    UChar *end;
    int32_t *offs;
    int i;
    UBool   checkOffsets = TRUE;
    char junk[9999];
    char offset_str[9999];
    UChar *p;
    UConverterToUCallback oldAction = NULL;
    const void* oldContext = NULL;

    int32_t   realBufferSize;
    UChar *realBufferEnd;


    for(i=0;i<NEW_MAX_BUFFER;i++)
        junkout[i] = 0xFFFE;

    for(i=0;i<NEW_MAX_BUFFER;i++)
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

    src = (const char *)source;
    targ = junkout;
    offs = junokout;

    realBufferSize = (sizeof(junkout)/sizeof(junkout[0]));
    realBufferEnd = junkout + realBufferSize;
    realSourceEnd = src + sourcelen;
    /*----setting the callback routine----*/
    ucnv_setToUCallBack (conv, callback, context, &oldAction, &oldContext, &status);
    if (U_FAILURE(status)) 
    { 
        log_err("FAILURE in setting the callback Function! %s\n", myErrorName(status));  
    }
    /*-------------------------------------*/
    /*setting the subChar*/
    if(mySubChar != NULL){
        ucnv_setSubstChars(conv, mySubChar, len, &status);
        if (U_FAILURE(status))  { 
            log_err("FAILURE in setting the callback Function! %s\n", myErrorName(status)); 
        }
    }
    /*------------*/


    if ( gOutBufferSize != realBufferSize )
        checkOffsets = FALSE;

    if( gInBufferSize != NEW_MAX_BUFFER )
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



        status = U_ZERO_ERROR;

        ucnv_toUnicode (conv,
                &targ,
                end,
                (const char **)&src,
                (const char *)srcLimit,
                checkOffsets ? offs : NULL,
                (UBool)(srcLimit == realSourceEnd), /* flush if we're at the end of the source data */
                &status);
    } while ( (status == U_BUFFER_OVERFLOW_ERROR) || (U_SUCCESS(status) && (srcLimit < realSourceEnd)) ); /* while we just need another buffer */

    /* allow failure codes for the stop callback */
    if(U_FAILURE(status) && status!=expectedError)
    {
        log_err("Problem doing toUnicode, errcode %s %s\n", myErrorName(status), gNuConvTestName);
        return FALSE;
    }

    log_verbose("\nConversion done. %d bytes -> %d chars.\nResult :",
        sourcelen, targ-junkout);
    if(getTestOption(VERBOSITY_OPTION))
    {

        junk[0] = 0;
        offset_str[0] = 0;

        for(p = junkout;p<targ;p++)
        {
            sprintf(junk + strlen(junk), "0x%04x, ", (0xFFFF) & (unsigned int)*p);
            sprintf(offset_str + strlen(offset_str), "0x%04x, ", (0xFFFF) & (unsigned int)junokout[p-junkout]);
        }

        log_verbose(junk);
        printUSeq(expect, expectlen);
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
        if(memcmp(junokout,expectOffsets,(targ-junkout) * sizeof(int32_t)))
        {
            log_err("did not get the expected offsets while %s \n", gNuConvTestName);
            log_err("Got offsets:      ");
            for(p=junkout;p<targ;p++)
                log_err("  %2d,", junokout[p-junkout]); 
            log_err("\n");
            log_err("Expected offsets: ");
            for(i=0; i<(targ-junkout); i++)
                log_err("  %2d,", expectOffsets[i]);
            log_err("\n");
            log_err("Got output:       ");
            for(i=0; i<(targ-junkout); i++)
                log_err("0x%04x,", junkout[i]);
            log_err("\n");
            log_err("From source:      ");
            for(i=0; i<(src-(const char *)source); i++)
                log_err("  0x%02x,", (unsigned char)source[i]);
            log_err("\n");
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
        log_err("Got:      ");
        printUSeqErr(junkout, expectlen);
        log_err("Expected: ");
        printUSeqErr(expect, expectlen);
        log_err("\n");
        return FALSE;
    }
}

static void TestCallBackFailure(void) {
    UErrorCode status = U_USELESS_COLLATOR_ERROR;
    ucnv_cbFromUWriteBytes(NULL, NULL, -1, -1, &status);
    if (status != U_USELESS_COLLATOR_ERROR) {
        log_err("Error: ucnv_cbFromUWriteBytes did not react correctly to a bad UErrorCode\n");
    }
    ucnv_cbFromUWriteUChars(NULL, NULL, NULL, -1, &status);
    if (status != U_USELESS_COLLATOR_ERROR) {
        log_err("Error: ucnv_cbFromUWriteUChars did not react correctly to a bad UErrorCode\n");
    }
    ucnv_cbFromUWriteSub(NULL, -1, &status);
    if (status != U_USELESS_COLLATOR_ERROR) {
        log_err("Error: ucnv_cbFromUWriteSub did not react correctly to a bad UErrorCode\n");
    }
    ucnv_cbToUWriteUChars(NULL, NULL, -1, -1, &status);
    if (status != U_USELESS_COLLATOR_ERROR) {
        log_err("Error: ucnv_cbToUWriteUChars did not react correctly to a bad UErrorCode\n");
    }
}
