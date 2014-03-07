/********************************************************************
 * COPYRIGHT:
 * Copyright (c) 1997-2010, International Business Machines Corporation and
 * others. All Rights Reserved.
 ********************************************************************/
/*******************************************************************************
*
* File CCONVTST.C
*
* Modification History:
*        Name                     Description
*    Steven R. Loomis     7/8/1999      Adding input buffer test
********************************************************************************
*/
#include <stdio.h>
#include "cstring.h"
#include "unicode/uloc.h"
#include "unicode/ucnv.h"
#include "unicode/ucnv_err.h"
#include "unicode/ucnv_cb.h"
#include "cintltst.h"
#include "unicode/utypes.h"
#include "unicode/ustring.h"
#include "unicode/ucol.h"
#include "cmemory.h"
#include "nucnvtst.h"

static void TestNextUChar(UConverter* cnv, const char* source, const char* limit, const int32_t results[], const char* message);
static void TestNextUCharError(UConverter* cnv, const char* source, const char* limit, UErrorCode expected, const char* message);
#if !UCONFIG_NO_COLLATION
static void TestJitterbug981(void);
#endif
static void TestJitterbug1293(void);
static void TestNewConvertWithBufferSizes(int32_t osize, int32_t isize) ;
static void TestConverterTypesAndStarters(void);
static void TestAmbiguous(void);
static void TestSignatureDetection(void);
static void TestUTF7(void);
static void TestIMAP(void);
static void TestUTF8(void);
static void TestCESU8(void);
static void TestUTF16(void);
static void TestUTF16BE(void);
static void TestUTF16LE(void);
static void TestUTF32(void);
static void TestUTF32BE(void);
static void TestUTF32LE(void);
static void TestLATIN1(void);

#if !UCONFIG_NO_LEGACY_CONVERSION
static void TestSBCS(void);
static void TestDBCS(void);
static void TestMBCS(void);
#if !UCONFIG_NO_LEGACY_CONVERSION && !UCONFIG_NO_FILE_IO
static void TestICCRunout(void);
#endif

#ifdef U_ENABLE_GENERIC_ISO_2022
static void TestISO_2022(void);
#endif

static void TestISO_2022_JP(void);
static void TestISO_2022_JP_1(void);
static void TestISO_2022_JP_2(void);
static void TestISO_2022_KR(void);
static void TestISO_2022_KR_1(void);
static void TestISO_2022_CN(void);
#if 0
   /*
    * ICU 4.4 (ticket #7314) removes mappings for CNS 11643 planes 3..7
    */
static void TestISO_2022_CN_EXT(void);
#endif
static void TestJIS(void);
static void TestHZ(void);
#endif

static void TestSCSU(void);

#if !UCONFIG_NO_LEGACY_CONVERSION
static void TestEBCDIC_STATEFUL(void);
static void TestGB18030(void);
static void TestLMBCS(void);
static void TestJitterbug255(void);
static void TestEBCDICUS4XML(void);
#if 0
   /*
    * ICU 4.4 (ticket #7314) removes mappings for CNS 11643 planes 3..7
    */
static void TestJitterbug915(void);
#endif
static void TestISCII(void);

static void TestCoverageMBCS(void);
static void TestJitterbug2346(void);
static void TestJitterbug2411(void);
static void TestJB5275(void);
static void TestJB5275_1(void);
static void TestJitterbug6175(void);
#endif

static void TestInBufSizes(void);

static void TestRoundTrippingAllUTF(void);
static void TestConv(const uint16_t in[],
                     int len,
                     const char* conv,
                     const char* lang,
                     char byteArr[],
                     int byteArrLen);

/* open a converter, using test data if it begins with '@' */
static UConverter *my_ucnv_open(const char *cnv, UErrorCode *err);


#define NEW_MAX_BUFFER 999

static int32_t  gInBufferSize = NEW_MAX_BUFFER;
static int32_t  gOutBufferSize = NEW_MAX_BUFFER;
static char     gNuConvTestName[1024];

#define nct_min(x,y)  ((x<y) ? x : y)

static UConverter *my_ucnv_open(const char *cnv, UErrorCode *err)
{
  if(cnv && cnv[0] == '@') {
    return ucnv_openPackage(loadTestData(err), cnv+1, err);
  } else {
    return ucnv_open(cnv, err);
  }
}

static void printSeq(const unsigned char* a, int len)
{
    int i=0;
    log_verbose("{");
    while (i<len)
        log_verbose("0x%02x ", a[i++]);
    log_verbose("}\n");
}

static void printUSeq(const UChar* a, int len)
{
    int i=0;
    log_verbose("{U+");
    while (i<len) log_verbose("0x%04x ", a[i++]);
    log_verbose("}\n");
}

static void printSeqErr(const unsigned char* a, int len)
{
    int i=0;
    fprintf(stderr, "{");
    while (i<len)
        fprintf(stderr, "0x%02x ", a[i++]);
    fprintf(stderr, "}\n");
}

static void printUSeqErr(const UChar* a, int len)
{
    int i=0;
    fprintf(stderr, "{U+");
    while (i<len)
        fprintf(stderr, "0x%04x ", a[i++]);
    fprintf(stderr,"}\n");
}

static void
TestNextUChar(UConverter* cnv, const char* source, const char* limit, const int32_t results[], const char* message)
{
     const char* s0;
     const char* s=(char*)source;
     const int32_t *r=results;
     UErrorCode errorCode=U_ZERO_ERROR;
     UChar32 c;

     while(s<limit) {
        s0=s;
        c=ucnv_getNextUChar(cnv, &s, limit, &errorCode);
        if(errorCode==U_INDEX_OUTOFBOUNDS_ERROR) {
            break; /* no more significant input */
        } else if(U_FAILURE(errorCode)) {
            log_err("%s ucnv_getNextUChar() failed: %s\n", message, u_errorName(errorCode));
            break;
        } else if(
            /* test the expected number of input bytes only if >=0 */
            (*r>=0 && (int32_t)(s-s0)!=*r) ||
            c!=*(r+1)
        ) {
            log_err("%s ucnv_getNextUChar() result %lx from %d bytes, should have been %lx from %d bytes.\n",
                message, c, (s-s0), *(r+1), *r);
            break;
        }
        r+=2;
    }
}

static void
TestNextUCharError(UConverter* cnv, const char* source, const char* limit, UErrorCode expected, const char* message)
{
     const char* s=(char*)source;
     UErrorCode errorCode=U_ZERO_ERROR;
     uint32_t c;
     c=ucnv_getNextUChar(cnv, &s, limit, &errorCode);
     if(errorCode != expected){
        log_err("FAIL: Expected:%s when %s-----Got:%s\n", myErrorName(expected), message, myErrorName(errorCode));
     }
     if(c != 0xFFFD && c != 0xffff){
        log_err("FAIL: Expected return value of 0xfffd or 0xffff when %s-----Got 0x%lx\n", message, c);
     }

}

static void TestInBufSizes(void)
{
  TestNewConvertWithBufferSizes(NEW_MAX_BUFFER,1);
#if 1
  TestNewConvertWithBufferSizes(NEW_MAX_BUFFER,2);
  TestNewConvertWithBufferSizes(NEW_MAX_BUFFER,3);
  TestNewConvertWithBufferSizes(NEW_MAX_BUFFER,4);
  TestNewConvertWithBufferSizes(NEW_MAX_BUFFER,5);
  TestNewConvertWithBufferSizes(NEW_MAX_BUFFER,6);
  TestNewConvertWithBufferSizes(1,1);
  TestNewConvertWithBufferSizes(2,3);
  TestNewConvertWithBufferSizes(3,2);
#endif
}

static void TestOutBufSizes(void)
{
#if 1
  TestNewConvertWithBufferSizes(NEW_MAX_BUFFER,NEW_MAX_BUFFER);
  TestNewConvertWithBufferSizes(1,NEW_MAX_BUFFER);
  TestNewConvertWithBufferSizes(2,NEW_MAX_BUFFER);
  TestNewConvertWithBufferSizes(3,NEW_MAX_BUFFER);
  TestNewConvertWithBufferSizes(4,NEW_MAX_BUFFER);
  TestNewConvertWithBufferSizes(5,NEW_MAX_BUFFER);

#endif
}


void addTestNewConvert(TestNode** root)
{
#if !UCONFIG_NO_FILE_IO
   addTest(root, &TestInBufSizes, "tsconv/nucnvtst/TestInBufSizes");
   addTest(root, &TestOutBufSizes, "tsconv/nucnvtst/TestOutBufSizes");
#endif
   addTest(root, &TestConverterTypesAndStarters, "tsconv/nucnvtst/TestConverterTypesAndStarters");
   addTest(root, &TestAmbiguous, "tsconv/nucnvtst/TestAmbiguous");
   addTest(root, &TestSignatureDetection, "tsconv/nucnvtst/TestSignatureDetection");
   addTest(root, &TestUTF7, "tsconv/nucnvtst/TestUTF7");
   addTest(root, &TestIMAP, "tsconv/nucnvtst/TestIMAP");
   addTest(root, &TestUTF8, "tsconv/nucnvtst/TestUTF8");

   /* test ucnv_getNextUChar() for charsets that encode single surrogates with complete byte sequences */
   addTest(root, &TestCESU8, "tsconv/nucnvtst/TestCESU8");
   addTest(root, &TestUTF16, "tsconv/nucnvtst/TestUTF16");
   addTest(root, &TestUTF16BE, "tsconv/nucnvtst/TestUTF16BE");
   addTest(root, &TestUTF16LE, "tsconv/nucnvtst/TestUTF16LE");
   addTest(root, &TestUTF32, "tsconv/nucnvtst/TestUTF32");
   addTest(root, &TestUTF32BE, "tsconv/nucnvtst/TestUTF32BE");
   addTest(root, &TestUTF32LE, "tsconv/nucnvtst/TestUTF32LE");

#if !UCONFIG_NO_LEGACY_CONVERSION
   addTest(root, &TestLMBCS, "tsconv/nucnvtst/TestLMBCS");
#endif

   addTest(root, &TestLATIN1, "tsconv/nucnvtst/TestLATIN1");

#if !UCONFIG_NO_LEGACY_CONVERSION
   addTest(root, &TestSBCS, "tsconv/nucnvtst/TestSBCS");
#if !UCONFIG_NO_FILE_IO
   addTest(root, &TestDBCS, "tsconv/nucnvtst/TestDBCS");
   addTest(root, &TestICCRunout, "tsconv/nucnvtst/TestICCRunout");
#endif
   addTest(root, &TestMBCS, "tsconv/nucnvtst/TestMBCS");

#ifdef U_ENABLE_GENERIC_ISO_2022
   addTest(root, &TestISO_2022, "tsconv/nucnvtst/TestISO_2022");
#endif

   addTest(root, &TestISO_2022_JP, "tsconv/nucnvtst/TestISO_2022_JP");
   addTest(root, &TestJIS, "tsconv/nucnvtst/TestJIS");
   addTest(root, &TestISO_2022_JP_1, "tsconv/nucnvtst/TestISO_2022_JP_1");
   addTest(root, &TestISO_2022_JP_2, "tsconv/nucnvtst/TestISO_2022_JP_2");
   addTest(root, &TestISO_2022_KR, "tsconv/nucnvtst/TestISO_2022_KR");
   addTest(root, &TestISO_2022_KR_1, "tsconv/nucnvtst/TestISO_2022_KR_1");
   addTest(root, &TestISO_2022_CN, "tsconv/nucnvtst/TestISO_2022_CN");
   /*
    * ICU 4.4 (ticket #7314) removes mappings for CNS 11643 planes 3..7
   addTest(root, &TestISO_2022_CN_EXT, "tsconv/nucnvtst/TestISO_2022_CN_EXT");
   addTest(root, &TestJitterbug915, "tsconv/nucnvtst/TestJitterbug915");
    */
   addTest(root, &TestHZ, "tsconv/nucnvtst/TestHZ");
#endif

   addTest(root, &TestSCSU, "tsconv/nucnvtst/TestSCSU");

#if !UCONFIG_NO_LEGACY_CONVERSION
   addTest(root, &TestEBCDIC_STATEFUL, "tsconv/nucnvtst/TestEBCDIC_STATEFUL");
   addTest(root, &TestGB18030, "tsconv/nucnvtst/TestGB18030");
   addTest(root, &TestJitterbug255, "tsconv/nucnvtst/TestJitterbug255");
   addTest(root, &TestEBCDICUS4XML, "tsconv/nucnvtst/TestEBCDICUS4XML");
   addTest(root, &TestISCII, "tsconv/nucnvtst/TestISCII");
   addTest(root, &TestJB5275, "tsconv/nucnvtst/TestJB5275");
   addTest(root, &TestJB5275_1, "tsconv/nucnvtst/TestJB5275_1");
#if !UCONFIG_NO_COLLATION
   addTest(root, &TestJitterbug981, "tsconv/nucnvtst/TestJitterbug981");
#endif

   addTest(root, &TestJitterbug1293, "tsconv/nucnvtst/TestJitterbug1293");
#endif


#if !UCONFIG_NO_LEGACY_CONVERSION && !UCONFIG_NO_FILE_IO
   addTest(root, &TestCoverageMBCS, "tsconv/nucnvtst/TestCoverageMBCS");
#endif

   addTest(root, &TestRoundTrippingAllUTF, "tsconv/nucnvtst/TestRoundTrippingAllUTF");

#if !UCONFIG_NO_LEGACY_CONVERSION
   addTest(root, &TestJitterbug2346, "tsconv/nucnvtst/TestJitterbug2346");
   addTest(root, &TestJitterbug2411, "tsconv/nucnvtst/TestJitterbug2411");
   addTest(root, &TestJitterbug6175, "tsconv/nucnvtst/TestJitterbug6175");
#endif

}


/* Note that this test already makes use of statics, so it's not really
   multithread safe.
   This convenience function lets us make the error messages actually useful.
*/

static void setNuConvTestName(const char *codepage, const char *direction)
{
    sprintf(gNuConvTestName, "[Testing %s %s Unicode, InputBufSiz=%d, OutputBufSiz=%d]",
        codepage,
        direction,
        (int)gInBufferSize,
        (int)gOutBufferSize);
}

typedef enum 
{
  TC_OK       = 0,  /* test was OK */
  TC_MISMATCH = 1,  /* Match failed - err was printed */
  TC_FAIL     = 2   /* Test failed, don't print an err because it was already printed. */
} ETestConvertResult;

/* Note: This function uses global variables and it will not do offset
checking without gOutBufferSize and gInBufferSize set to NEW_MAX_BUFFER */
static ETestConvertResult testConvertFromU( const UChar *source, int sourceLen,  const uint8_t *expect, int expectLen,
                const char *codepage, const int32_t *expectOffsets , UBool useFallback)
{
    UErrorCode status = U_ZERO_ERROR;
    UConverter *conv = 0;
    char    junkout[NEW_MAX_BUFFER]; /* FIX */
    int32_t    junokout[NEW_MAX_BUFFER]; /* FIX */
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

    for(i=0;i<NEW_MAX_BUFFER;i++)
        junkout[i] = (char)0xF0;
    for(i=0;i<NEW_MAX_BUFFER;i++)
        junokout[i] = 0xFF;

    setNuConvTestName(codepage, "FROM");

    log_verbose("\n=========  %s\n", gNuConvTestName);

    conv = my_ucnv_open(codepage, &status);

    if(U_FAILURE(status))
    {
        log_data_err("Couldn't open converter %s\n",codepage);
        return TC_FAIL;
    }
    if(useFallback){
        ucnv_setFallback(conv,useFallback);
    }

    log_verbose("Converter opened..\n");

    src = source;
    targ = junkout;
    offs = junokout;

    realBufferSize = (sizeof(junkout)/sizeof(junkout[0]));
    realBufferEnd = junkout + realBufferSize;
    realSourceEnd = source + sourceLen;

    if ( gOutBufferSize != realBufferSize || gInBufferSize != NEW_MAX_BUFFER )
        checkOffsets = FALSE;

    do
    {
      end = nct_min(targ + gOutBufferSize, realBufferEnd);
      sourceLimit = nct_min(src + gInBufferSize, realSourceEnd);
      
      doFlush = (UBool)(sourceLimit == realSourceEnd);
      
      if(targ == realBufferEnd) {
        log_err("Error, overflowed the real buffer while about to call fromUnicode! targ=%08lx %s", targ, gNuConvTestName);
        return TC_FAIL;
      }
      log_verbose("calling fromUnicode @ SOURCE:%08lx to %08lx  TARGET: %08lx to %08lx, flush=%s\n", src,sourceLimit, targ,end, doFlush?"TRUE":"FALSE");


      status = U_ZERO_ERROR;
      
      ucnv_fromUnicode (conv,
                        &targ,
                        end,
                        &src,
                        sourceLimit,
                        checkOffsets ? offs : NULL,
                        doFlush, /* flush if we're at the end of the input data */
                        &status);
    } while ( (status == U_BUFFER_OVERFLOW_ERROR) || (U_SUCCESS(status) && sourceLimit < realSourceEnd) );

    if(U_FAILURE(status)) {
      log_err("Problem doing fromUnicode to %s, errcode %s %s\n", codepage, myErrorName(status), gNuConvTestName);
      return TC_FAIL;
    }

    log_verbose("\nConversion done [%d uchars in -> %d chars out]. \nResult :",
                sourceLen, targ-junkout);

    if(getTestOption(VERBOSITY_OPTION))
    {
      char junk[9999];
      char offset_str[9999];
      char *ptr;
      
      junk[0] = 0;
      offset_str[0] = 0;
      for(ptr = junkout;ptr<targ;ptr++) {
        sprintf(junk + strlen(junk), "0x%02x, ", (int)(0xFF & *ptr));
        sprintf(offset_str + strlen(offset_str), "0x%02x, ", (int)(0xFF & junokout[ptr-junkout]));
      }
      
      log_verbose(junk);
      printSeq((const uint8_t *)expect, expectLen);
      if ( checkOffsets ) {
        log_verbose("\nOffsets:");
        log_verbose(offset_str);
      }
      log_verbose("\n");
    }
    ucnv_close(conv);
    
    if(expectLen != targ-junkout) {
      log_err("Expected %d chars out, got %d %s\n", expectLen, targ-junkout, gNuConvTestName);
      log_verbose("Expected %d chars out, got %d %s\n", expectLen, targ-junkout, gNuConvTestName);
      printf("\nGot:");
      printSeqErr((const unsigned char*)junkout, (int32_t)(targ-junkout));
      printf("\nExpected:");
      printSeqErr((const unsigned char*)expect, expectLen);
      return TC_MISMATCH;
    }
    
    if (checkOffsets && (expectOffsets != 0) ) {
      log_verbose("comparing %d offsets..\n", targ-junkout);
      if(memcmp(junokout,expectOffsets,(targ-junkout) * sizeof(int32_t) )){
        log_err("did not get the expected offsets. %s\n", gNuConvTestName);
        printSeqErr((const unsigned char*)junkout, (int32_t)(targ-junkout));
        log_err("\n");
        log_err("Got  :     ");
        for(p=junkout;p<targ;p++) {
          log_err("%d,", junokout[p-junkout]);
        }
        log_err("\n");
        log_err("Expected:  ");
        for(i=0; i<(targ-junkout); i++) {
          log_err("%d,", expectOffsets[i]);
        }
        log_err("\n");
      }
    }

    log_verbose("comparing..\n");
    if(!memcmp(junkout, expect, expectLen)) {
      log_verbose("Matches!\n");
      return TC_OK;
    } else {
      log_err("String does not match u->%s\n", gNuConvTestName);
      printUSeqErr(source, sourceLen);
      printf("\nGot:");
      printSeqErr((const unsigned char *)junkout, expectLen);
      printf("\nExpected:");
      printSeqErr((const unsigned char *)expect, expectLen);
      
      return TC_MISMATCH;
    }
}

/* Note: This function uses global variables and it will not do offset
checking without gOutBufferSize and gInBufferSize set to NEW_MAX_BUFFER */
static ETestConvertResult testConvertToU( const uint8_t *source, int sourcelen, const UChar *expect, int expectlen,
                                          const char *codepage, const int32_t *expectOffsets, UBool useFallback)
{
    UErrorCode status = U_ZERO_ERROR;
    UConverter *conv = 0;
    UChar    junkout[NEW_MAX_BUFFER]; /* FIX */
    int32_t    junokout[NEW_MAX_BUFFER]; /* FIX */
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


    for(i=0;i<NEW_MAX_BUFFER;i++)
        junkout[i] = 0xFFFE;

    for(i=0;i<NEW_MAX_BUFFER;i++)
        junokout[i] = -1;

    setNuConvTestName(codepage, "TO");

    log_verbose("\n=========  %s\n", gNuConvTestName);

    conv = my_ucnv_open(codepage, &status);

    if(U_FAILURE(status))
    {
        log_data_err("Couldn't open converter %s\n",gNuConvTestName);
        return TC_FAIL;
    }
    if(useFallback){
        ucnv_setFallback(conv,useFallback);
    }
    log_verbose("Converter opened..\n");

    src = (const char *)source;
    targ = junkout;
    offs = junokout;

    realBufferSize = (sizeof(junkout)/sizeof(junkout[0]));
    realBufferEnd = junkout + realBufferSize;
    realSourceEnd = src + sourcelen;

    if ( gOutBufferSize != realBufferSize ||  gInBufferSize != NEW_MAX_BUFFER )
        checkOffsets = FALSE;

    do
    {
        end = nct_min( targ + gOutBufferSize, realBufferEnd);
        srcLimit = nct_min(realSourceEnd, src + gInBufferSize);

        if(targ == realBufferEnd)
        {
            log_err("Error, the end would overflow the real output buffer while about to call toUnicode! tarjet=%08lx %s",targ,gNuConvTestName);
            return TC_FAIL;
        }
        log_verbose("calling toUnicode @ %08lx to %08lx\n", targ,end);

        /* oldTarg = targ; */

        status = U_ZERO_ERROR;

        ucnv_toUnicode (conv,
                &targ,
                end,
                &src,
                srcLimit,
                checkOffsets ? offs : NULL,
                (UBool)(srcLimit == realSourceEnd), /* flush if we're at the end of hte source data */
                &status);

        /*        offs += (targ-oldTarg); */

      } while ( (status == U_BUFFER_OVERFLOW_ERROR) || (U_SUCCESS(status) && (srcLimit < realSourceEnd)) ); /* while we just need another buffer */

    if(U_FAILURE(status))
    {
        log_err("Problem doing %s toUnicode, errcode %s %s\n", codepage, myErrorName(status), gNuConvTestName);
        return TC_FAIL;
    }

    log_verbose("\nConversion done. %d bytes -> %d chars.\nResult :",
        sourcelen, targ-junkout);
    if(getTestOption(VERBOSITY_OPTION))
    {
        char junk[9999];
        char offset_str[9999];
        UChar *ptr;

        junk[0] = 0;
        offset_str[0] = 0;

        for(ptr = junkout;ptr<targ;ptr++)
        {
            sprintf(junk + strlen(junk), "0x%04x, ", (0xFFFF) & (unsigned int)*ptr);
            sprintf(offset_str + strlen(offset_str), "0x%04x, ", (0xFFFF) & (unsigned int)junokout[ptr-junkout]);
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
        if(memcmp(junokout,expectOffsets,(targ-junkout) * sizeof(int32_t))){
            log_err("did not get the expected offsets. %s\n",gNuConvTestName);
            log_err("Got:      ");
            for(p=junkout;p<targ;p++) {
                log_err("%d,", junokout[p-junkout]);
            }
            log_err("\n");
            log_err("Expected: ");
            for(i=0; i<(targ-junkout); i++) {
                log_err("%d,", expectOffsets[i]);
            }
            log_err("\n");
            log_err("output:   ");
            for(i=0; i<(targ-junkout); i++) {
                log_err("%X,", junkout[i]);
            }
            log_err("\n");
            log_err("input:    ");
            for(i=0; i<(src-(const char *)source); i++) {
                log_err("%X,", (unsigned char)source[i]);
            }
            log_err("\n");
        }
    }

    if(!memcmp(junkout, expect, expectlen*2))
    {
        log_verbose("Matches!\n");
        return TC_OK;
    }
    else
    {
        log_err("String does not match. %s\n", gNuConvTestName);
        log_verbose("String does not match. %s\n", gNuConvTestName);
        printf("\nGot:");
        printUSeqErr(junkout, expectlen);
        printf("\nExpected:");
        printUSeqErr(expect, expectlen);
        return TC_MISMATCH;
    }
}


static void TestNewConvertWithBufferSizes(int32_t outsize, int32_t insize )
{
/** test chars #1 */
    /*  1 2 3  1Han 2Han 3Han .  */
    static const UChar   sampleText[] =
     { 0x0031, 0x0032, 0x0033, 0x0000, 0x4e00, 0x4e8c, 0x4e09, 0x002E, 0xD840, 0xDC21 };
    static const UChar sampleTextRoundTripUnmappable[] =
    { 0x0031, 0x0032, 0x0033, 0x0000, 0x4e00, 0x4e8c, 0x4e09, 0x002E, 0xfffd };


    static const uint8_t expectedUTF8[] =
     { 0x31, 0x32, 0x33, 0x00, 0xe4, 0xb8, 0x80, 0xe4, 0xba, 0x8c, 0xe4, 0xb8, 0x89, 0x2E, 0xf0, 0xa0, 0x80, 0xa1 };
    static const int32_t toUTF8Offs[] =
     { 0x00, 0x01, 0x02, 0x03, 0x04, 0x04, 0x04, 0x05, 0x05, 0x05, 0x06, 0x06, 0x06, 0x07, 0x08, 0x08, 0x08, 0x08 };
    static const int32_t fmUTF8Offs[] =
     { 0x0000, 0x0001, 0x0002, 0x0003, 0x0004, 0x0007, 0x000a, 0x000d, 0x000e, 0x000e };

#ifdef U_ENABLE_GENERIC_ISO_2022
    /* Same as UTF8, but with ^[%B preceeding */
    static const const uint8_t expectedISO2022[] =
     { 0x1b, 0x25, 0x42, 0x31, 0x32, 0x33, 0x00, 0xe4, 0xb8, 0x80, 0xe4, 0xba, 0x8c, 0xe4, 0xb8, 0x89, 0x2E };
    static const int32_t toISO2022Offs[]     =
     { -1, -1, -1, 0x00, 0x01, 0x02, 0x03, 0x04, 0x04,
       0x04, 0x05, 0x05, 0x05, 0x06, 0x06, 0x06, 0x07 }; /* right? */
    static const int32_t fmISO2022Offs[] =
     { 0x0003, 0x0004, 0x0005, 0x0006, 0x0007, 0x000a, 0x000d, 0x0010 }; /* is this right? */
#endif

    /*  1 2 3 0, <SO> h1 h2 h3 <SI> . EBCDIC_STATEFUL */
    static const uint8_t expectedIBM930[] =
     { 0xF1, 0xF2, 0xF3, 0x00, 0x0E, 0x45, 0x41, 0x45, 0x42, 0x45, 0x43, 0x0F, 0x4B, 0x0e, 0xfe, 0xfe, 0x0f };
    static const int32_t toIBM930Offs[] =
     { 0x00, 0x01, 0x02, 0x03, 0x04, 0x04, 0x04, 0x05, 0x05, 0x06, 0x06, 0x07, 0x07, 0x08, 0x08, 0x08, -1 };
    static const int32_t fmIBM930Offs[] =
     { 0x0000, 0x0001, 0x0002, 0x0003, 0x0005, 0x0007, 0x0009, 0x000c, 0x000e };

    /* 1 2 3 0 h1 h2 h3 . MBCS*/
    static const uint8_t expectedIBM943[] =
     {  0x31, 0x32, 0x33, 0x00, 0x88, 0xea, 0x93, 0xf1, 0x8e, 0x4f, 0x2e, 0xfc, 0xfc };
    static const int32_t toIBM943Offs    [] =
     {  0x00, 0x01, 0x02, 0x03, 0x04, 0x04, 0x05, 0x05, 0x06, 0x06, 0x07, 0x08, 0x08 };
    static const int32_t fmIBM943Offs[] =
     { 0x0000, 0x0001, 0x0002, 0x0003, 0x0004, 0x0006, 0x0008, 0x000a, 0x000b };

    /* 1 2 3 0 h1 h2 h3 . DBCS*/
    static const uint8_t expectedIBM9027[] =
     {  0xfe, 0xfe, 0xfe, 0xfe, 0xfe, 0xfe, 0xfe, 0xfe, 0x4c, 0x41, 0x4c, 0x48, 0x4c, 0x55, 0xfe, 0xfe, 0xfe, 0xfe };
    static const int32_t toIBM9027Offs    [] =
     {  0x00, 0x00, 0x01, 0x01, 0x02, 0x02, 0x03, 0x03, 0x04, 0x04, 0x05, 0x05, 0x06, 0x06, 0x07, 0x07, 0x08, 0x08 };

     /* 1 2 3 0 <?> <?> <?> . SBCS*/
    static const uint8_t expectedIBM920[] =
     {  0x31, 0x32, 0x33, 0x00, 0x1a, 0x1a, 0x1a, 0x2e, 0x1a };
    static const int32_t toIBM920Offs    [] =
     {  0x00, 0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07, 0x08 };

    /* 1 2 3 0 <?> <?> <?> . SBCS*/
    static const uint8_t expectedISO88593[] =
     { 0x31, 0x32, 0x33, 0x00, 0x1a, 0x1a, 0x1a, 0x2E, 0x1a };
    static const int32_t toISO88593Offs[]     =
     { 0x00, 0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07, 0x08 };

    /* 1 2 3 0 <?> <?> <?> . <?> LATIN_1*/
    static const uint8_t expectedLATIN1[] =
     { 0x31, 0x32, 0x33, 0x00, 0x1a, 0x1a, 0x1a, 0x2E, 0x1a };
    static const int32_t toLATIN1Offs[]     =
     { 0x00, 0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07, 0x08 };


    /*  etc */
    static const uint8_t expectedUTF16BE[] =
     { 0x00, 0x31, 0x00, 0x32, 0x00, 0x33, 0x00, 0x00, 0x4e, 0x00, 0x4e, 0x8c, 0x4e, 0x09, 0x00, 0x2e, 0xd8, 0x40, 0xdc, 0x21 };
    static const int32_t toUTF16BEOffs[]=
     { 0x00, 0x00, 0x01, 0x01, 0x02, 0x02, 0x03, 0x03, 0x04, 0x04, 0x05, 0x05, 0x06, 0x06, 0x07, 0x07, 0x08, 0x08, 0x08, 0x08 };
    static const int32_t fmUTF16BEOffs[] =
     { 0x0000, 0x0002, 0x0004, 0x0006, 0x0008, 0x000a, 0x000c,  0x000e, 0x0010, 0x0010 };

    static const uint8_t expectedUTF16LE[] =
     { 0x31, 0x00, 0x32, 0x00, 0x33, 0x00, 0x00, 0x00, 0x00, 0x4e, 0x8c, 0x4e, 0x09, 0x4e, 0x2e, 0x00, 0x40, 0xd8, 0x21, 0xdc };
    static const int32_t toUTF16LEOffs[]=
     { 0x00, 0x00, 0x01, 0x01, 0x02, 0x02, 0x03, 0x03, 0x04, 0x04, 0x05, 0x05, 0x06, 0x06,  0x07, 0x07, 0x08, 0x08, 0x08, 0x08 };
    static const int32_t fmUTF16LEOffs[] =
     { 0x0000, 0x0002, 0x0004, 0x0006, 0x0008, 0x000a, 0x000c, 0x000e, 0x0010, 0x0010 };

    static const uint8_t expectedUTF32BE[] =
     { 0x00, 0x00, 0x00, 0x31,
       0x00, 0x00, 0x00, 0x32,
       0x00, 0x00, 0x00, 0x33,
       0x00, 0x00, 0x00, 0x00,
       0x00, 0x00, 0x4e, 0x00,
       0x00, 0x00, 0x4e, 0x8c,
       0x00, 0x00, 0x4e, 0x09,
       0x00, 0x00, 0x00, 0x2e,
       0x00, 0x02, 0x00, 0x21 };
    static const int32_t toUTF32BEOffs[]=
     { 0x00, 0x00, 0x00, 0x00,
       0x01, 0x01, 0x01, 0x01,
       0x02, 0x02, 0x02, 0x02,
       0x03, 0x03, 0x03, 0x03,
       0x04, 0x04, 0x04, 0x04,
       0x05, 0x05, 0x05, 0x05,
       0x06, 0x06, 0x06, 0x06,
       0x07, 0x07, 0x07, 0x07,
       0x08, 0x08, 0x08, 0x08,
       0x08, 0x08, 0x08, 0x08 };
    static const int32_t fmUTF32BEOffs[] =
     { 0x0000, 0x0004, 0x0008, 0x000c, 0x0010, 0x0014, 0x0018,  0x001c, 0x0020, 0x0020 };

    static const uint8_t expectedUTF32LE[] =
     { 0x31, 0x00, 0x00, 0x00,
       0x32, 0x00, 0x00, 0x00,
       0x33, 0x00, 0x00, 0x00,
       0x00, 0x00, 0x00, 0x00,
       0x00, 0x4e, 0x00, 0x00,
       0x8c, 0x4e, 0x00, 0x00,
       0x09, 0x4e, 0x00, 0x00,
       0x2e, 0x00, 0x00, 0x00,
       0x21, 0x00, 0x02, 0x00 };
    static const int32_t toUTF32LEOffs[]=
     { 0x00, 0x00, 0x00, 0x00,
       0x01, 0x01, 0x01, 0x01,
       0x02, 0x02, 0x02, 0x02,
       0x03, 0x03, 0x03, 0x03,
       0x04, 0x04, 0x04, 0x04,
       0x05, 0x05, 0x05, 0x05,
       0x06, 0x06, 0x06, 0x06,
       0x07, 0x07, 0x07, 0x07,
       0x08, 0x08, 0x08, 0x08,
       0x08, 0x08, 0x08, 0x08 };
    static const int32_t fmUTF32LEOffs[] =
     { 0x0000, 0x0004, 0x0008, 0x000c, 0x0010, 0x0014, 0x0018, 0x001c, 0x0020, 0x0020 };




/** Test chars #2 **/

    /* Sahha [health],  slashed h's */
    static const UChar malteseUChars[] = { 0x0053, 0x0061, 0x0127, 0x0127, 0x0061 };
    static const uint8_t expectedMaltese913[] = { 0x53, 0x61, 0xB1, 0xB1, 0x61 };

    /* LMBCS */
    static const UChar LMBCSUChars[]     = { 0x0027, 0x010A, 0x0000, 0x0127, 0x2666, 0x0220 };
    static const uint8_t expectedLMBCS[] = { 0x27, 0x06, 0x04, 0x00, 0x01, 0x73, 0x01, 0x04, 0x14, 0x02, 0x20 };
    static const int32_t toLMBCSOffs[]   = { 0x00, 0x01, 0x01, 0x02, 0x03, 0x03, 0x04, 0x04 , 0x05, 0x05, 0x05 };
    static const int32_t fmLMBCSOffs[]   = { 0x0000, 0x0001, 0x0003, 0x0004, 0x0006, 0x0008};
    /*********************************** START OF CODE finally *************/

    gInBufferSize = insize;
    gOutBufferSize = outsize;

    log_verbose("\n\n\nTesting conversions with InputBufferSize = %d, OutputBufferSize = %d\n", gInBufferSize, gOutBufferSize);


    /*UTF-8*/
    testConvertFromU(sampleText, sizeof(sampleText)/sizeof(sampleText[0]),
        expectedUTF8, sizeof(expectedUTF8), "UTF8", toUTF8Offs,FALSE );

    log_verbose("Test surrogate behaviour for UTF8\n");
    {
        static const UChar testinput[]={ 0x20ac, 0xd801, 0xdc01, 0xdc01 };
        static const uint8_t expectedUTF8test2[]= { 0xe2, 0x82, 0xac,
                           0xf0, 0x90, 0x90, 0x81,
                           0xef, 0xbf, 0xbd
        };
        static const int32_t offsets[]={ 0, 0, 0, 1, 1, 1, 1, 3, 3, 3 };
        testConvertFromU(testinput, sizeof(testinput)/sizeof(testinput[0]),
                         expectedUTF8test2, sizeof(expectedUTF8test2), "UTF8", offsets,FALSE );


    }

#if !UCONFIG_NO_LEGACY_CONVERSION && defined(U_ENABLE_GENERIC_ISO_2022)
    /*ISO-2022*/
    testConvertFromU(sampleText, sizeof(sampleText)/sizeof(sampleText[0]),
        expectedISO2022, sizeof(expectedISO2022), "ISO_2022", toISO2022Offs,FALSE );
#endif

    /*UTF16 LE*/
    testConvertFromU(sampleText, sizeof(sampleText)/sizeof(sampleText[0]),
        expectedUTF16LE, sizeof(expectedUTF16LE), "utf-16le", toUTF16LEOffs,FALSE );
    /*UTF16 BE*/
    testConvertFromU(sampleText, sizeof(sampleText)/sizeof(sampleText[0]),
        expectedUTF16BE, sizeof(expectedUTF16BE), "utf-16be", toUTF16BEOffs,FALSE );
    /*UTF32 LE*/
    testConvertFromU(sampleText, sizeof(sampleText)/sizeof(sampleText[0]),
        expectedUTF32LE, sizeof(expectedUTF32LE), "utf-32le", toUTF32LEOffs,FALSE );
    /*UTF32 BE*/
    testConvertFromU(sampleText, sizeof(sampleText)/sizeof(sampleText[0]),
        expectedUTF32BE, sizeof(expectedUTF32BE), "utf-32be", toUTF32BEOffs,FALSE );

    /*LATIN_1*/
    testConvertFromU(sampleText, sizeof(sampleText)/sizeof(sampleText[0]),
        expectedLATIN1, sizeof(expectedLATIN1), "LATIN_1", toLATIN1Offs,FALSE );

#if !UCONFIG_NO_LEGACY_CONVERSION
    /*EBCDIC_STATEFUL*/
    testConvertFromU(sampleText, sizeof(sampleText)/sizeof(sampleText[0]),
        expectedIBM930, sizeof(expectedIBM930), "ibm-930", toIBM930Offs,FALSE );

    testConvertFromU(sampleText, sizeof(sampleText)/sizeof(sampleText[0]),
        expectedISO88593, sizeof(expectedISO88593), "iso-8859-3", toISO88593Offs,FALSE );

    /*MBCS*/

    testConvertFromU(sampleText, sizeof(sampleText)/sizeof(sampleText[0]),
        expectedIBM943, sizeof(expectedIBM943), "ibm-943", toIBM943Offs,FALSE );
    /*DBCS*/
    testConvertFromU(sampleText, sizeof(sampleText)/sizeof(sampleText[0]),
        expectedIBM9027, sizeof(expectedIBM9027), "@ibm9027", toIBM9027Offs,FALSE );
    /*SBCS*/
    testConvertFromU(sampleText, sizeof(sampleText)/sizeof(sampleText[0]),
        expectedIBM920, sizeof(expectedIBM920), "ibm-920", toIBM920Offs,FALSE );
    /*SBCS*/
    testConvertFromU(sampleText, sizeof(sampleText)/sizeof(sampleText[0]),
        expectedISO88593, sizeof(expectedISO88593), "iso-8859-3", toISO88593Offs,FALSE );
#endif


/****/

    /*UTF-8*/
    testConvertToU(expectedUTF8, sizeof(expectedUTF8),
        sampleText, sizeof(sampleText)/sizeof(sampleText[0]), "utf8", fmUTF8Offs,FALSE);
#if !UCONFIG_NO_LEGACY_CONVERSION && defined(U_ENABLE_GENERIC_ISO_2022)
    /*ISO-2022*/
    testConvertToU(expectedISO2022, sizeof(expectedISO2022),
        sampleText, sizeof(sampleText)/sizeof(sampleText[0]), "ISO_2022", fmISO2022Offs,FALSE);
#endif

    /*UTF16 LE*/
    testConvertToU(expectedUTF16LE, sizeof(expectedUTF16LE),
        sampleText, sizeof(sampleText)/sizeof(sampleText[0]), "utf-16le", fmUTF16LEOffs,FALSE);
    /*UTF16 BE*/
    testConvertToU(expectedUTF16BE, sizeof(expectedUTF16BE),
        sampleText, sizeof(sampleText)/sizeof(sampleText[0]), "utf-16be", fmUTF16BEOffs,FALSE);
    /*UTF32 LE*/
    testConvertToU(expectedUTF32LE, sizeof(expectedUTF32LE),
        sampleText, sizeof(sampleText)/sizeof(sampleText[0]), "utf-32le", fmUTF32LEOffs,FALSE);
    /*UTF32 BE*/
    testConvertToU(expectedUTF32BE, sizeof(expectedUTF32BE),
        sampleText, sizeof(sampleText)/sizeof(sampleText[0]), "utf-32be", fmUTF32BEOffs,FALSE);

#if !UCONFIG_NO_LEGACY_CONVERSION
    /*EBCDIC_STATEFUL*/
    testConvertToU(expectedIBM930, sizeof(expectedIBM930), sampleTextRoundTripUnmappable, 
            sizeof(sampleTextRoundTripUnmappable)/sizeof(sampleTextRoundTripUnmappable[0]), "ibm-930", fmIBM930Offs,FALSE);
    /*MBCS*/
    testConvertToU(expectedIBM943, sizeof(expectedIBM943),sampleTextRoundTripUnmappable, 
            sizeof(sampleTextRoundTripUnmappable)/sizeof(sampleTextRoundTripUnmappable[0]), "ibm-943", fmIBM943Offs,FALSE);
#endif

    /* Try it again to make sure it still works */
    testConvertToU(expectedUTF16LE, sizeof(expectedUTF16LE),
        sampleText, sizeof(sampleText)/sizeof(sampleText[0]), "utf-16le", fmUTF16LEOffs,FALSE);

#if !UCONFIG_NO_LEGACY_CONVERSION
    testConvertToU(expectedMaltese913, sizeof(expectedMaltese913),
        malteseUChars, sizeof(malteseUChars)/sizeof(malteseUChars[0]), "latin3", NULL,FALSE);

    testConvertFromU(malteseUChars, sizeof(malteseUChars)/sizeof(malteseUChars[0]),
        expectedMaltese913, sizeof(expectedMaltese913), "iso-8859-3", NULL,FALSE );

    /*LMBCS*/
    testConvertFromU(LMBCSUChars, sizeof(LMBCSUChars)/sizeof(LMBCSUChars[0]),
        expectedLMBCS, sizeof(expectedLMBCS), "LMBCS-1", toLMBCSOffs,FALSE );
    testConvertToU(expectedLMBCS, sizeof(expectedLMBCS),
        LMBCSUChars, sizeof(LMBCSUChars)/sizeof(LMBCSUChars[0]), "LMBCS-1", fmLMBCSOffs,FALSE);
#endif

    /* UTF-7 examples are mostly from http://www.imc.org/rfc2152 */
    {
        /* encode directly set D and set O */
        static const uint8_t utf7[] = {
            /*
                Hi Mom -+Jjo--!
                A+ImIDkQ.
                +-
                +ZeVnLIqe
            */
            0x48, 0x69, 0x20, 0x4d, 0x6f, 0x6d, 0x20, 0x2d, 0x2b, 0x4a, 0x6a, 0x6f, 0x2d, 0x2d, 0x21,
            0x41, 0x2b, 0x49, 0x6d, 0x49, 0x44, 0x6b, 0x51, 0x2e,
            0x2b, 0x2d,
            0x2b, 0x5a, 0x65, 0x56, 0x6e, 0x4c, 0x49, 0x71, 0x65
        };
        static const UChar unicode[] = {
            /*
                Hi Mom -<WHITE SMILING FACE>-!
                A<NOT IDENTICAL TO><ALPHA>.
                +
                [Japanese word "nihongo"]
            */
            0x48, 0x69, 0x20, 0x4d, 0x6f, 0x6d, 0x20, 0x2d, 0x263a, 0x2d, 0x21,
            0x41, 0x2262, 0x0391, 0x2e,
            0x2b,
            0x65e5, 0x672c, 0x8a9e
        };
        static const int32_t toUnicodeOffsets[] = {
            0, 1, 2, 3, 4, 5, 6, 7, 9, 13, 14,
            15, 17, 19, 23,
            24,
            27, 29, 32
        };
        static const int32_t fromUnicodeOffsets[] = {
            0, 1, 2, 3, 4, 5, 6, 7, 8, 8, 8, 8, 8, 9, 10,
            11, 12, 12, 12, 13, 13, 13, 13, 14,
            15, 15,
            16, 16, 16, 17, 17, 17, 18, 18, 18
        };

        /* same but escaping set O (the exclamation mark) */
        static const uint8_t utf7Restricted[] = {
            /*
                Hi Mom -+Jjo--+ACE-
                A+ImIDkQ.
                +-
                +ZeVnLIqe
            */
            0x48, 0x69, 0x20, 0x4d, 0x6f, 0x6d, 0x20, 0x2d, 0x2b, 0x4a, 0x6a, 0x6f, 0x2d, 0x2d, 0x2b, 0x41, 0x43, 0x45, 0x2d,
            0x41, 0x2b, 0x49, 0x6d, 0x49, 0x44, 0x6b, 0x51, 0x2e,
            0x2b, 0x2d,
            0x2b, 0x5a, 0x65, 0x56, 0x6e, 0x4c, 0x49, 0x71, 0x65
        };
        static const int32_t toUnicodeOffsetsR[] = {
            0, 1, 2, 3, 4, 5, 6, 7, 9, 13, 15,
            19, 21, 23, 27,
            28,
            31, 33, 36
        };
        static const int32_t fromUnicodeOffsetsR[] = {
            0, 1, 2, 3, 4, 5, 6, 7, 8, 8, 8, 8, 8, 9, 10, 10, 10, 10, 10,
            11, 12, 12, 12, 13, 13, 13, 13, 14,
            15, 15,
            16, 16, 16, 17, 17, 17, 18, 18, 18
        };

        testConvertFromU(unicode, sizeof(unicode)/U_SIZEOF_UCHAR, utf7, sizeof(utf7), "UTF-7", fromUnicodeOffsets,FALSE);

        testConvertToU(utf7, sizeof(utf7), unicode, sizeof(unicode)/U_SIZEOF_UCHAR, "UTF-7", toUnicodeOffsets,FALSE);

        testConvertFromU(unicode, sizeof(unicode)/U_SIZEOF_UCHAR, utf7Restricted, sizeof(utf7Restricted), "UTF-7,version=1", fromUnicodeOffsetsR,FALSE);

        testConvertToU(utf7Restricted, sizeof(utf7Restricted), unicode, sizeof(unicode)/U_SIZEOF_UCHAR, "UTF-7,version=1", toUnicodeOffsetsR,FALSE);
    }

    /*
     * IMAP-mailbox-name examples are mostly from http://www.imc.org/rfc2152,
     * modified according to RFC 2060,
     * and supplemented with the one example in RFC 2060 itself.
     */
    {
        static const uint8_t imap[] = {
            /*  Hi Mom -&Jjo--!
                A&ImIDkQ-.
                &-
                &ZeVnLIqe-
                \
                ~peter
                /mail
                /&ZeVnLIqe-
                /&U,BTFw-
            */
            0x48, 0x69, 0x20, 0x4d, 0x6f, 0x6d, 0x20, 0x2d, 0x26, 0x4a, 0x6a, 0x6f, 0x2d, 0x2d, 0x21,
            0x41, 0x26, 0x49, 0x6d, 0x49, 0x44, 0x6b, 0x51, 0x2d, 0x2e,
            0x26, 0x2d,
            0x26, 0x5a, 0x65, 0x56, 0x6e, 0x4c, 0x49, 0x71, 0x65, 0x2d,
            0x5c,
            0x7e, 0x70, 0x65, 0x74, 0x65, 0x72,
            0x2f, 0x6d, 0x61, 0x69, 0x6c,
            0x2f, 0x26, 0x5a, 0x65, 0x56, 0x6e, 0x4c, 0x49, 0x71, 0x65, 0x2d,
            0x2f, 0x26, 0x55, 0x2c, 0x42, 0x54, 0x46, 0x77, 0x2d
        };
        static const UChar unicode[] = {
            /*  Hi Mom -<WHITE SMILING FACE>-!
                A<NOT IDENTICAL TO><ALPHA>.
                &
                [Japanese word "nihongo"]
                \
                ~peter
                /mail
                /<65e5, 672c, 8a9e>
                /<53f0, 5317>
            */
            0x48, 0x69, 0x20, 0x4d, 0x6f, 0x6d, 0x20, 0x2d, 0x263a, 0x2d, 0x21,
            0x41, 0x2262, 0x0391, 0x2e,
            0x26,
            0x65e5, 0x672c, 0x8a9e,
            0x5c,
            0x7e, 0x70, 0x65, 0x74, 0x65, 0x72,
            0x2f, 0x6d, 0x61, 0x69, 0x6c,
            0x2f, 0x65e5, 0x672c, 0x8a9e,
            0x2f, 0x53f0, 0x5317
        };
        static const int32_t toUnicodeOffsets[] = {
            0, 1, 2, 3, 4, 5, 6, 7, 9, 13, 14,
            15, 17, 19, 24,
            25,
            28, 30, 33,
            37,
            38, 39, 40, 41, 42, 43,
            44, 45, 46, 47, 48,
            49, 51, 53, 56,
            60, 62, 64
        };
        static const int32_t fromUnicodeOffsets[] = {
            0, 1, 2, 3, 4, 5, 6, 7, 8, 8, 8, 8, 8, 9, 10,
            11, 12, 12, 12, 13, 13, 13, 13, 13, 14,
            15, 15,
            16, 16, 16, 17, 17, 17, 18, 18, 18, 18,
            19,
            20, 21, 22, 23, 24, 25,
            26, 27, 28, 29, 30,
            31, 32, 32, 32, 33, 33, 33, 34, 34, 34, 34,
            35, 36, 36, 36, 37, 37, 37, 37, 37
        };

        testConvertFromU(unicode, sizeof(unicode)/U_SIZEOF_UCHAR, imap, sizeof(imap), "IMAP-mailbox-name", fromUnicodeOffsets,FALSE);

        testConvertToU(imap, sizeof(imap), unicode, sizeof(unicode)/U_SIZEOF_UCHAR, "IMAP-mailbox-name", toUnicodeOffsets,FALSE);
    }

    /* Test UTF-8 bad data handling*/
    {
        static const uint8_t utf8[]={
            0x61,
            0xf7, 0xbf, 0xbf, 0xbf,         /* > 10FFFF */
            0x00,
            0x62,
            0xfb, 0xbf, 0xbf, 0xbf, 0xbf,   /* > 10FFFF */
            0xfb, 0xbf, 0xbf, 0xbf, 0xbf,   /* > 10FFFF */
            0xf4, 0x8f, 0xbf, 0xbf,         /* 10FFFF */
            0xdf, 0xbf,                     /* 7ff */
            0xbf,                           /* truncated tail */
            0xf4, 0x90, 0x80, 0x80,         /* 11FFFF */
            0x02
        };

        static const uint16_t utf8Expected[]={
            0x0061,
            0xfffd,
            0x0000,
            0x0062,
            0xfffd,
            0xfffd,
            0xdbff, 0xdfff,
            0x07ff,
            0xfffd,
            0xfffd,
            0x0002
        };

        static const int32_t utf8Offsets[]={
            0, 1, 5, 6, 7, 12, 17, 17, 21, 23, 24, 28
        };
        testConvertToU(utf8, sizeof(utf8),
                       utf8Expected, sizeof(utf8Expected)/sizeof(utf8Expected[0]), "utf-8", utf8Offsets ,FALSE);

    }

    /* Test UTF-32BE bad data handling*/
    {
        static const uint8_t utf32[]={
            0x00, 0x00, 0x00, 0x61,
            0x00, 0x11, 0x00, 0x00,         /* 0x110000 out of range */
            0x00, 0x10, 0xff, 0xff,         /* 0x10FFFF in range */
            0x00, 0x00, 0x00, 0x62,
            0xff, 0xff, 0xff, 0xff,         /* 0xffffffff out of range */
            0x7f, 0xff, 0xff, 0xff,         /* 0x7fffffff out of range */
            0x00, 0x00, 0x01, 0x62,
            0x00, 0x00, 0x02, 0x62
        };
        static const uint16_t utf32Expected[]={
            0x0061,
            0xfffd,         /* 0x110000 out of range */
            0xDBFF,         /* 0x10FFFF in range */
            0xDFFF,
            0x0062,
            0xfffd,         /* 0xffffffff out of range */
            0xfffd,         /* 0x7fffffff out of range */
            0x0162,
            0x0262
        };
        static const int32_t utf32Offsets[]={
            0, 4, 8, 8, 12, 16, 20, 24, 28
        };
        static const uint8_t utf32ExpectedBack[]={
            0x00, 0x00, 0x00, 0x61,
            0x00, 0x00, 0xff, 0xfd,         /* 0x110000 out of range */
            0x00, 0x10, 0xff, 0xff,         /* 0x10FFFF in range */
            0x00, 0x00, 0x00, 0x62,
            0x00, 0x00, 0xff, 0xfd,         /* 0xffffffff out of range */
            0x00, 0x00, 0xff, 0xfd,         /* 0x7fffffff out of range */
            0x00, 0x00, 0x01, 0x62,
            0x00, 0x00, 0x02, 0x62
        };
        static const int32_t utf32OffsetsBack[]={
            0,0,0,0,
            1,1,1,1,
            2,2,2,2,
            4,4,4,4,
            5,5,5,5,
            6,6,6,6,
            7,7,7,7,
            8,8,8,8
        };

        testConvertToU(utf32, sizeof(utf32),
                       utf32Expected, sizeof(utf32Expected)/sizeof(utf32Expected[0]), "utf-32be", utf32Offsets ,FALSE);
        testConvertFromU(utf32Expected, sizeof(utf32Expected)/sizeof(utf32Expected[0]),
            utf32ExpectedBack, sizeof(utf32ExpectedBack), "utf-32be", utf32OffsetsBack, FALSE);
    }

    /* Test UTF-32LE bad data handling*/
    {
        static const uint8_t utf32[]={
            0x61, 0x00, 0x00, 0x00,
            0x00, 0x00, 0x11, 0x00,         /* 0x110000 out of range */
            0xff, 0xff, 0x10, 0x00,         /* 0x10FFFF in range */
            0x62, 0x00, 0x00, 0x00,
            0xff, 0xff, 0xff, 0xff,         /* 0xffffffff out of range */
            0xff, 0xff, 0xff, 0x7f,         /* 0x7fffffff out of range */
            0x62, 0x01, 0x00, 0x00,
            0x62, 0x02, 0x00, 0x00,
        };

        static const uint16_t utf32Expected[]={
            0x0061,
            0xfffd,         /* 0x110000 out of range */
            0xDBFF,         /* 0x10FFFF in range */
            0xDFFF,
            0x0062,
            0xfffd,         /* 0xffffffff out of range */
            0xfffd,         /* 0x7fffffff out of range */
            0x0162,
            0x0262
        };
        static const int32_t utf32Offsets[]={
            0, 4, 8, 8, 12, 16, 20, 24, 28
        };
        static const uint8_t utf32ExpectedBack[]={
            0x61, 0x00, 0x00, 0x00,
            0xfd, 0xff, 0x00, 0x00,         /* 0x110000 out of range */
            0xff, 0xff, 0x10, 0x00,         /* 0x10FFFF in range */
            0x62, 0x00, 0x00, 0x00,
            0xfd, 0xff, 0x00, 0x00,         /* 0xffffffff out of range */
            0xfd, 0xff, 0x00, 0x00,         /* 0x7fffffff out of range */
            0x62, 0x01, 0x00, 0x00,
            0x62, 0x02, 0x00, 0x00
        };
        static const int32_t utf32OffsetsBack[]={
            0,0,0,0,
            1,1,1,1,
            2,2,2,2,
            4,4,4,4,
            5,5,5,5,
            6,6,6,6,
            7,7,7,7,
            8,8,8,8
        };
        testConvertToU(utf32, sizeof(utf32),
            utf32Expected, sizeof(utf32Expected)/sizeof(utf32Expected[0]), "utf-32le", utf32Offsets,FALSE );
        testConvertFromU(utf32Expected, sizeof(utf32Expected)/sizeof(utf32Expected[0]),
            utf32ExpectedBack, sizeof(utf32ExpectedBack), "utf-32le", utf32OffsetsBack, FALSE);
    }
}

static void TestCoverageMBCS(){
#if 0
    UErrorCode status = U_ZERO_ERROR;
    const char *directory = loadTestData(&status);
    char* tdpath = NULL;
    char* saveDirectory = (char*)malloc(sizeof(char) *(strlen(u_getDataDirectory())+1));
    int len = strlen(directory);
    char* index=NULL;

    tdpath = (char*) malloc(sizeof(char) * (len * 2));
    uprv_strcpy(saveDirectory,u_getDataDirectory());
    log_verbose("Retrieved data directory %s \n",saveDirectory);
    uprv_strcpy(tdpath,directory);
    index=strrchr(tdpath,(char)U_FILE_SEP_CHAR);

    if((unsigned int)(index-tdpath) != (strlen(tdpath)-1)){
            *(index+1)=0;
    }
    u_setDataDirectory(tdpath);
    log_verbose("ICU data directory is set to: %s \n" ,tdpath);
#endif

    /*some more test to increase the code coverage in MBCS.  Create an test converter from test1.ucm
      which is test file for MBCS conversion with single-byte codepage data.*/
    {

        /* MBCS with single byte codepage data test1.ucm*/
        const UChar unicodeInput[]    = { 0x20ac, 0x0005, 0x0006, 0xdbc4, 0xde34, 0x0003};
        const uint8_t expectedtest1[] = { 0x00, 0x05, 0xff, 0x07, 0xff,};
        int32_t  totest1Offs[]        = { 0, 1, 2, 3, 5, };

        /*from Unicode*/
        testConvertFromU(unicodeInput, sizeof(unicodeInput)/sizeof(unicodeInput[0]),
            expectedtest1, sizeof(expectedtest1), "@test1", totest1Offs,FALSE );
    }

    /*some more test to increase the code coverage in MBCS.  Create an test converter from test3.ucm
      which is test file for MBCS conversion with three-byte codepage data.*/
    {

        /* MBCS with three byte codepage data test3.ucm*/
        const UChar unicodeInput[]    = { 0x20ac, 0x0005, 0x0006, 0x000b, 0xdbc4, 0xde34, 0xd84d, 0xdc56, 0x000e};
        const uint8_t expectedtest3[] = { 0x00, 0x05, 0xff, 0x01, 0x02, 0x0b,  0x07,  0x01, 0x02, 0x0a,  0xff,};
        int32_t  totest3Offs[]        = { 0, 1, 2, 3, 3, 3, 4, 6, 6, 6, 8};

        const uint8_t test3input[]    = { 0x00, 0x05, 0x06, 0x01, 0x02, 0x0b,  0x07,  0x01, 0x02, 0x0a, 0x01, 0x02, 0x0c,};
        const UChar expectedUnicode[] = { 0x20ac, 0x0005, 0x0006, 0x000b, 0xdbc4, 0xde34, 0xd84d, 0xdc56, 0xfffd};
        int32_t fromtest3Offs[]       = { 0, 1, 2, 3, 6, 6, 7, 7, 10 };

        /*from Unicode*/
        testConvertFromU(unicodeInput, sizeof(unicodeInput)/sizeof(unicodeInput[0]),
            expectedtest3, sizeof(expectedtest3), "@test3", totest3Offs,FALSE );

        /*to Unicode*/
        testConvertToU(test3input, sizeof(test3input),
            expectedUnicode, sizeof(expectedUnicode)/sizeof(expectedUnicode[0]), "@test3", fromtest3Offs ,FALSE);

    }

    /*some more test to increase the code coverage in MBCS.  Create an test converter from test4.ucm
      which is test file for MBCS conversion with four-byte codepage data.*/
    {

        /* MBCS with three byte codepage data test4.ucm*/
        static const UChar unicodeInput[]    = { 0x20ac, 0x0005, 0x0006, 0x000b, 0xdbc4, 0xde34, 0xd84d, 0xdc56, 0x000e};
        static const uint8_t expectedtest4[] = { 0x00, 0x05, 0xff, 0x01, 0x02, 0x03, 0x0b,  0x07,  0x01, 0x02, 0x03, 0x0a,  0xff,};
        static const int32_t totest4Offs[]   = { 0, 1, 2, 3, 3, 3, 3, 4, 6, 6, 6, 6, 8,};

        static const uint8_t test4input[]    = { 0x00, 0x05, 0x06, 0x01, 0x02, 0x03, 0x0b,  0x07,  0x01, 0x02, 0x03, 0x0a, 0x01, 0x02, 0x03, 0x0c,};
        static const UChar expectedUnicode[] = { 0x20ac, 0x0005, 0x0006, 0x000b, 0xdbc4, 0xde34, 0xd84d, 0xdc56, 0xfffd};
        static const int32_t fromtest4Offs[] = { 0, 1, 2, 3, 7, 7, 8, 8, 12,};

        /*from Unicode*/
        testConvertFromU(unicodeInput, sizeof(unicodeInput)/sizeof(unicodeInput[0]),
            expectedtest4, sizeof(expectedtest4), "@test4", totest4Offs,FALSE );

        /*to Unicode*/
        testConvertToU(test4input, sizeof(test4input),
            expectedUnicode, sizeof(expectedUnicode)/sizeof(expectedUnicode[0]), "@test4", fromtest4Offs,FALSE );

    }
#if 0
    free(tdpath);
    /* restore the original data directory */
    log_verbose("Setting the data directory to %s \n", saveDirectory);
    u_setDataDirectory(saveDirectory);
    free(saveDirectory);
#endif

}

static void TestConverterType(const char *convName, UConverterType convType) {
    UConverter* myConverter;
    UErrorCode err = U_ZERO_ERROR;

    myConverter = my_ucnv_open(convName, &err);

    if (U_FAILURE(err)) {
        log_data_err("Failed to create an %s converter\n", convName);
        return;
    }
    else
    {
        if (ucnv_getType(myConverter)!=convType) {
            log_err("ucnv_getType Failed for %s. Got enum value 0x%X\n",
                convName, convType);
        }
        else {
            log_verbose("ucnv_getType %s ok\n", convName);
        }
    }
    ucnv_close(myConverter);
}

static void TestConverterTypesAndStarters()
{
#if !UCONFIG_NO_LEGACY_CONVERSION
    UConverter* myConverter;
    UErrorCode err = U_ZERO_ERROR;
    UBool mystarters[256];

/*    const UBool expectedKSCstarters[256] = {
        FALSE, FALSE, FALSE, FALSE, FALSE, FALSE, FALSE, FALSE, FALSE, FALSE,
        FALSE, FALSE, FALSE, FALSE, FALSE, FALSE, FALSE, FALSE, FALSE, FALSE,
        FALSE, FALSE, FALSE, FALSE, FALSE, FALSE, FALSE, FALSE, FALSE, FALSE,
        FALSE, FALSE, FALSE, FALSE, FALSE, FALSE, FALSE, FALSE, FALSE, FALSE,
        FALSE, FALSE, FALSE, FALSE, FALSE, FALSE, FALSE, FALSE, FALSE, FALSE,
        FALSE, FALSE, FALSE, FALSE, FALSE, FALSE, FALSE, FALSE, FALSE, FALSE,
        FALSE, FALSE, FALSE, FALSE, FALSE, FALSE, FALSE, FALSE, FALSE, FALSE,
        FALSE, FALSE, FALSE, FALSE, FALSE, FALSE, FALSE, FALSE, FALSE, FALSE,
        FALSE, FALSE, FALSE, FALSE, FALSE, FALSE, FALSE, FALSE, FALSE, FALSE,
        FALSE, FALSE, FALSE, FALSE, FALSE, FALSE, FALSE, FALSE, FALSE, FALSE,
        FALSE, FALSE, FALSE, FALSE, FALSE, FALSE, FALSE, FALSE, FALSE, FALSE,
        FALSE, FALSE, FALSE, FALSE, FALSE, FALSE, FALSE, FALSE, FALSE, FALSE,
        FALSE, FALSE, FALSE, FALSE, FALSE, FALSE, FALSE, FALSE, FALSE, FALSE,
        FALSE, FALSE, FALSE, FALSE, FALSE, FALSE, FALSE, FALSE, FALSE, FALSE,
        FALSE, FALSE, FALSE, TRUE, TRUE, TRUE, TRUE, TRUE, TRUE, TRUE,
        TRUE, TRUE, TRUE, TRUE, TRUE, TRUE, TRUE, TRUE, TRUE, TRUE,
        TRUE, TRUE, TRUE, TRUE, TRUE, TRUE, TRUE, TRUE, TRUE, TRUE,
        TRUE, TRUE, TRUE, FALSE, FALSE, TRUE, TRUE, TRUE, TRUE, TRUE,
        TRUE, TRUE, TRUE, TRUE, TRUE, TRUE, TRUE, TRUE, TRUE, TRUE,
        TRUE, TRUE, TRUE, TRUE, TRUE, TRUE, TRUE, TRUE, TRUE, TRUE,
        TRUE, TRUE, TRUE, TRUE, TRUE, TRUE, TRUE, TRUE, TRUE, TRUE,
        TRUE, TRUE, TRUE, TRUE, TRUE, TRUE, TRUE, TRUE, TRUE, TRUE,
        TRUE, TRUE, TRUE, TRUE, TRUE, TRUE, TRUE, TRUE, TRUE, TRUE,
        TRUE, TRUE, TRUE, TRUE, TRUE, TRUE, TRUE, TRUE, TRUE, TRUE,
        TRUE, TRUE, TRUE, TRUE, TRUE, TRUE, TRUE, TRUE, TRUE, TRUE,
        TRUE, TRUE, TRUE, TRUE, TRUE, TRUE};*/


    log_verbose("Testing KSC, ibm-930, ibm-878  for starters and their conversion types.");

    myConverter = ucnv_open("ksc", &err);
    if (U_FAILURE(err)) {
      log_data_err("Failed to create an ibm-ksc converter\n");
      return;
    }
    else
    {
        if (ucnv_getType(myConverter)!=UCNV_MBCS)
            log_err("ucnv_getType Failed for ibm-949\n");
        else
            log_verbose("ucnv_getType ibm-949 ok\n");

        if(myConverter!=NULL)
            ucnv_getStarters(myConverter, mystarters, &err);

        /*if (memcmp(expectedKSCstarters, mystarters, sizeof(expectedKSCstarters)))
          log_err("Failed ucnv_getStarters for ksc\n");
          else
          log_verbose("ucnv_getStarters ok\n");*/

    }
    ucnv_close(myConverter);

    TestConverterType("ibm-930", UCNV_EBCDIC_STATEFUL);
    TestConverterType("ibm-878", UCNV_SBCS);
#endif

    TestConverterType("iso-8859-1", UCNV_LATIN_1);

    TestConverterType("ibm-1208", UCNV_UTF8);

    TestConverterType("utf-8", UCNV_UTF8);
    TestConverterType("UTF-16BE", UCNV_UTF16_BigEndian);
    TestConverterType("UTF-16LE", UCNV_UTF16_LittleEndian);
    TestConverterType("UTF-32BE", UCNV_UTF32_BigEndian);
    TestConverterType("UTF-32LE", UCNV_UTF32_LittleEndian);

#if !UCONFIG_NO_LEGACY_CONVERSION

#if defined(U_ENABLE_GENERIC_ISO_2022)
    TestConverterType("iso-2022", UCNV_ISO_2022);
#endif

    TestConverterType("hz", UCNV_HZ);
#endif

    TestConverterType("scsu", UCNV_SCSU);

#if !UCONFIG_NO_LEGACY_CONVERSION
    TestConverterType("x-iscii-de", UCNV_ISCII);
#endif

    TestConverterType("ascii", UCNV_US_ASCII);
    TestConverterType("utf-7", UCNV_UTF7);
    TestConverterType("IMAP-mailbox-name", UCNV_IMAP_MAILBOX);
    TestConverterType("bocu-1", UCNV_BOCU1);
}

static void
TestAmbiguousConverter(UConverter *cnv) {
    static const char inBytes[3]={ 0x61, 0x5B, 0x5c };
    UChar outUnicode[20]={ 0, 0, 0, 0 };

    const char *s;
    UChar *u;
    UErrorCode errorCode;
    UBool isAmbiguous;

    /* try to convert an 'a', a square bracket and a US-ASCII backslash */
    errorCode=U_ZERO_ERROR;
    s=inBytes;
    u=outUnicode;
    ucnv_toUnicode(cnv, &u, u+20, &s, s+3, NULL, TRUE, &errorCode);
    if(U_FAILURE(errorCode)) {
        /* we do not care about general failures in this test; the input may just not be mappable */
        return;
    }

    if(outUnicode[0]!=0x61 || outUnicode[1]!=0x5B || outUnicode[2]==0xfffd) {
        /* not a close ASCII-family encoding, or 0x5c is unassigned/illegal: this test is not applicable */
        /* There are some encodings that are partially ASCII based,
        like the ISO-7 and GSM series of codepages, which we ignore. */
        return;
    }

    isAmbiguous=ucnv_isAmbiguous(cnv);

    /* check that outUnicode[1]!=0x5c is exactly the same as ucnv_isAmbiguous() */
    if((outUnicode[2]!=0x5c)!=isAmbiguous) {
        log_err("error: converter \"%s\" needs a backslash fix: %d but ucnv_isAmbiguous()==%d\n",
            ucnv_getName(cnv, &errorCode), outUnicode[2]!=0x5c, isAmbiguous);
        return;
    }

    if(outUnicode[2]!=0x5c) {
        /* needs fixup, fix it */
        ucnv_fixFileSeparator(cnv, outUnicode, (int32_t)(u-outUnicode));
        if(outUnicode[2]!=0x5c) {
            /* the fix failed */
            log_err("error: ucnv_fixFileSeparator(%s) failed\n", ucnv_getName(cnv, &errorCode));
            return;
        }
    }
}

static void TestAmbiguous()
{
    UErrorCode status = U_ZERO_ERROR;
    UConverter *ascii_cnv = 0, *sjis_cnv = 0, *cnv;
    static const char target[] = {
        /* "\\usr\\local\\share\\data\\icutest.txt" */
        0x5c, 0x75, 0x73, 0x72,
        0x5c, 0x6c, 0x6f, 0x63, 0x61, 0x6c,
        0x5c, 0x73, 0x68, 0x61, 0x72, 0x65,
        0x5c, 0x64, 0x61, 0x74, 0x61,
        0x5c, 0x69, 0x63, 0x75, 0x74, 0x65, 0x73, 0x74, 0x2e, 0x74, 0x78, 0x74,
        0
    };
    UChar asciiResult[200], sjisResult[200];
    int32_t /*asciiLength = 0,*/ sjisLength = 0, i;
    const char *name;

    /* enumerate all converters */
    status=U_ZERO_ERROR;
    for(i=0; (name=ucnv_getAvailableName(i))!=NULL; ++i) {
        cnv=ucnv_open(name, &status);
        if(U_SUCCESS(status)) {
            TestAmbiguousConverter(cnv);
            ucnv_close(cnv);
        } else {
            log_err("error: unable to open available converter \"%s\"\n", name);
            status=U_ZERO_ERROR;
        }
    }

#if !UCONFIG_NO_LEGACY_CONVERSION
    sjis_cnv = ucnv_open("ibm-943", &status);
    if (U_FAILURE(status))
    {
        log_data_err("Failed to create a SJIS converter\n");
        return;
    }
    ascii_cnv = ucnv_open("LATIN-1", &status);
    if (U_FAILURE(status))
    {
        log_data_err("Failed to create a LATIN-1 converter\n");
        ucnv_close(sjis_cnv);
        return;
    }
    /* convert target from SJIS to Unicode */
    sjisLength = ucnv_toUChars(sjis_cnv, sjisResult, sizeof(sjisResult)/U_SIZEOF_UCHAR, target, (int32_t)strlen(target), &status);
    if (U_FAILURE(status))
    {
        log_err("Failed to convert the SJIS string.\n");
        ucnv_close(sjis_cnv);
        ucnv_close(ascii_cnv);
        return;
    }
    /* convert target from Latin-1 to Unicode */
    /*asciiLength =*/ ucnv_toUChars(ascii_cnv, asciiResult, sizeof(asciiResult)/U_SIZEOF_UCHAR, target, (int32_t)strlen(target), &status);
    if (U_FAILURE(status))
    {
        log_err("Failed to convert the Latin-1 string.\n");
        ucnv_close(sjis_cnv);
        ucnv_close(ascii_cnv);
        return;
    }
    if (!ucnv_isAmbiguous(sjis_cnv))
    {
        log_err("SJIS converter should contain ambiguous character mappings.\n");
        ucnv_close(sjis_cnv);
        ucnv_close(ascii_cnv);
        return;
    }
    if (u_strcmp(sjisResult, asciiResult) == 0)
    {
        log_err("File separators for SJIS don't need to be fixed.\n");
    }
    ucnv_fixFileSeparator(sjis_cnv, sjisResult, sjisLength);
    if (u_strcmp(sjisResult, asciiResult) != 0)
    {
        log_err("Fixing file separator for SJIS failed.\n");
    }
    ucnv_close(sjis_cnv);
    ucnv_close(ascii_cnv);
#endif
}

static void
TestSignatureDetection(){
    /* with null terminated strings */
    {
        static const char* data[] = {
                "\xFE\xFF\x00\x00",     /* UTF-16BE */
                "\xFF\xFE\x00\x00",     /* UTF-16LE */
                "\xEF\xBB\xBF\x00",     /* UTF-8    */
                "\x0E\xFE\xFF\x00",     /* SCSU     */

                "\xFE\xFF",             /* UTF-16BE */
                "\xFF\xFE",             /* UTF-16LE */
                "\xEF\xBB\xBF",         /* UTF-8    */
                "\x0E\xFE\xFF",         /* SCSU     */

                "\xFE\xFF\x41\x42",     /* UTF-16BE */
                "\xFF\xFE\x41\x41",     /* UTF-16LE */
                "\xEF\xBB\xBF\x41",     /* UTF-8    */
                "\x0E\xFE\xFF\x41",     /* SCSU     */

                "\x2B\x2F\x76\x38\x2D", /* UTF-7    */
                "\x2B\x2F\x76\x38\x41", /* UTF-7    */
                "\x2B\x2F\x76\x39\x41", /* UTF-7    */
                "\x2B\x2F\x76\x2B\x41", /* UTF-7    */
                "\x2B\x2F\x76\x2F\x41",  /* UTF-7    */

                "\xDD\x73\x66\x73"      /* UTF-EBCDIC */
        };
        static const char* expected[] = {
                "UTF-16BE",
                "UTF-16LE",
                "UTF-8",
                "SCSU",

                "UTF-16BE",
                "UTF-16LE",
                "UTF-8",
                "SCSU",

                "UTF-16BE",
                "UTF-16LE",
                "UTF-8",
                "SCSU",

                "UTF-7",
                "UTF-7",
                "UTF-7",
                "UTF-7",
                "UTF-7",
                "UTF-EBCDIC"
        };
        static const int32_t expectedLength[] ={
            2,
            2,
            3,
            3,

            2,
            2,
            3,
            3,

            2,
            2,
            3,
            3,

            5,
            4,
            4,
            4,
            4,
            4
        };
        int i=0;
        UErrorCode err;
        int32_t signatureLength = -1;
        const char* source = NULL;
        const char* enc = NULL;
        for( ; i<sizeof(data)/sizeof(char*); i++){
            err = U_ZERO_ERROR;
            source = data[i];
            enc = ucnv_detectUnicodeSignature(source, -1 , &signatureLength, &err);
            if(U_FAILURE(err)){
                log_err("ucnv_detectUnicodeSignature failed for source : %s at index :%i. Error: %s\n", source,i,u_errorName(err));
                continue;
            }
            if(enc == NULL || strcmp(enc,expected[i]) !=0){
                log_err("ucnv_detectUnicodeSignature failed for source : %s at index :%i. Expected: %s. Got: %s\n",source,i,expected[i],enc);
                continue;
            }
            if(signatureLength != expectedLength[i]){
                log_err("ucnv_detectUnicodeSignature failed for source : %s at index :%i.Expected Length: %i. Got length: %i\n",source,i,signatureLength,expectedLength[i]);
            }
        }
    }
    {
        static const char* data[] = {
                "\xFE\xFF\x00",         /* UTF-16BE */
                "\xFF\xFE\x00",         /* UTF-16LE */
                "\xEF\xBB\xBF\x00",     /* UTF-8    */
                "\x0E\xFE\xFF\x00",     /* SCSU     */
                "\x00\x00\xFE\xFF",     /* UTF-32BE */
                "\xFF\xFE\x00\x00",     /* UTF-32LE */
                "\xFE\xFF",             /* UTF-16BE */
                "\xFF\xFE",             /* UTF-16LE */
                "\xEF\xBB\xBF",         /* UTF-8    */
                "\x0E\xFE\xFF",         /* SCSU     */
                "\x00\x00\xFE\xFF",     /* UTF-32BE */
                "\xFF\xFE\x00\x00",     /* UTF-32LE */
                "\xFE\xFF\x41\x42",     /* UTF-16BE */
                "\xFF\xFE\x41\x41",     /* UTF-16LE */
                "\xEF\xBB\xBF\x41",     /* UTF-8    */
                "\x0E\xFE\xFF\x41",     /* SCSU     */
                "\x00\x00\xFE\xFF\x41", /* UTF-32BE */
                "\xFF\xFE\x00\x00\x42", /* UTF-32LE */
                "\xFB\xEE\x28",         /* BOCU-1   */
                "\xFF\x41\x42"          /* NULL     */
        };
        static const int len[] = {
            3,
            3,
            4,
            4,
            4,
            4,
            2,
            2,
            3,
            3,
            4,
            4,
            4,
            4,
            4,
            4,
            5,
            5,
            3,
            3
        };

        static const char* expected[] = {
                "UTF-16BE",
                "UTF-16LE",
                "UTF-8",
                "SCSU",
                "UTF-32BE",
                "UTF-32LE",
                "UTF-16BE",
                "UTF-16LE",
                "UTF-8",
                "SCSU",
                "UTF-32BE",
                "UTF-32LE",
                "UTF-16BE",
                "UTF-16LE",
                "UTF-8",
                "SCSU",
                "UTF-32BE",
                "UTF-32LE",
                "BOCU-1",
                NULL
        };
        static const int32_t expectedLength[] ={
            2,
            2,
            3,
            3,
            4,
            4,
            2,
            2,
            3,
            3,
            4,
            4,
            2,
            2,
            3,
            3,
            4,
            4,
            3,
            0
        };
        int i=0;
        UErrorCode err;
        int32_t signatureLength = -1;
        int32_t sourceLength=-1;
        const char* source = NULL;
        const char* enc = NULL;
        for( ; i<sizeof(data)/sizeof(char*); i++){
            err = U_ZERO_ERROR;
            source = data[i];
            sourceLength = len[i];
            enc = ucnv_detectUnicodeSignature(source, sourceLength , &signatureLength, &err);
            if(U_FAILURE(err)){
                log_err("ucnv_detectUnicodeSignature test2 failed for source : %s at index :%i. Error: %s\n", source,i,u_errorName(err));
                continue;
            }
            if(enc == NULL || strcmp(enc,expected[i]) !=0){
                if(expected[i] !=NULL){
                 log_err("ucnv_detectUnicodeSignature test2 failed for source : %s at index :%i. Expected: %s. Got: %s\n",source,i,expected[i],enc);
                 continue;
                }
            }
            if(signatureLength != expectedLength[i]){
                log_err("ucnv_detectUnicodeSignature test2 failed for source : %s at index :%i.Expected Length: %i. Got length: %i\n",source,i,signatureLength,expectedLength[i]);
            }
        }
    }
}

static void TestUTF7() {
    /* test input */
    static const uint8_t in[]={
        /* H - +Jjo- - ! +- +2AHcAQ */
        0x48,
        0x2d,
        0x2b, 0x4a, 0x6a, 0x6f,
        0x2d, 0x2d,
        0x21,
        0x2b, 0x2d,
        0x2b, 0x32, 0x41, 0x48, 0x63, 0x41, 0x51
    };

    /* expected test results */
    static const int32_t results[]={
        /* number of bytes read, code point */
        1, 0x48,
        1, 0x2d,
        4, 0x263a, /* <WHITE SMILING FACE> */
        2, 0x2d,
        1, 0x21,
        2, 0x2b,
        7, 0x10401
    };

    const char *cnvName;
    const char *source=(const char *)in, *limit=(const char *)in+sizeof(in);
    UErrorCode errorCode=U_ZERO_ERROR;
    UConverter *cnv=ucnv_open("UTF-7", &errorCode);
    if(U_FAILURE(errorCode)) {
        log_err("Unable to open a UTF-7 converter: %s\n", u_errorName(errorCode)); /* sholdn't be a data err */
        return;
    }
    TestNextUChar(cnv, source, limit, results, "UTF-7");
    /* Test the condition when source >= sourceLimit */
    TestNextUCharError(cnv, source, source, U_INDEX_OUTOFBOUNDS_ERROR, "sourceLimit <= source");
    cnvName = ucnv_getName(cnv, &errorCode);
    if (U_FAILURE(errorCode) || uprv_strcmp(cnvName, "UTF-7") != 0) {
        log_err("UTF-7 converter is called %s: %s\n", cnvName, u_errorName(errorCode));
    }
    ucnv_close(cnv);
}

static void TestIMAP() {
    /* test input */
    static const uint8_t in[]={
        /* H - &Jjo- - ! &- &2AHcAQ- \ */
        0x48,
        0x2d,
        0x26, 0x4a, 0x6a, 0x6f,
        0x2d, 0x2d,
        0x21,
        0x26, 0x2d,
        0x26, 0x32, 0x41, 0x48, 0x63, 0x41, 0x51, 0x2d
    };

    /* expected test results */
    static const int32_t results[]={
        /* number of bytes read, code point */
        1, 0x48,
        1, 0x2d,
        4, 0x263a, /* <WHITE SMILING FACE> */
        2, 0x2d,
        1, 0x21,
        2, 0x26,
        7, 0x10401
    };

    const char *cnvName;
    const char *source=(const char *)in, *limit=(const char *)in+sizeof(in);
    UErrorCode errorCode=U_ZERO_ERROR;
    UConverter *cnv=ucnv_open("IMAP-mailbox-name", &errorCode);
    if(U_FAILURE(errorCode)) {
        log_err("Unable to open a IMAP-mailbox-name converter: %s\n", u_errorName(errorCode)); /* sholdn't be a data err */
        return;
    }
    TestNextUChar(cnv, source, limit, results, "IMAP-mailbox-name");
    /* Test the condition when source >= sourceLimit */
    TestNextUCharError(cnv, source, source, U_INDEX_OUTOFBOUNDS_ERROR, "sourceLimit <= source");
    cnvName = ucnv_getName(cnv, &errorCode);
    if (U_FAILURE(errorCode) || uprv_strcmp(cnvName, "IMAP-mailbox-name") != 0) {
        log_err("IMAP-mailbox-name converter is called %s: %s\n", cnvName, u_errorName(errorCode));
    }
    ucnv_close(cnv);
}

static void TestUTF8() {
    /* test input */
    static const uint8_t in[]={
        0x61,
        0xc2, 0x80,
        0xe0, 0xa0, 0x80,
        0xf0, 0x90, 0x80, 0x80,
        0xf4, 0x84, 0x8c, 0xa1,
        0xf0, 0x90, 0x90, 0x81
    };

    /* expected test results */
    static const int32_t results[]={
        /* number of bytes read, code point */
        1, 0x61,
        2, 0x80,
        3, 0x800,
        4, 0x10000,
        4, 0x104321,
        4, 0x10401
    };

    /* error test input */
    static const uint8_t in2[]={
        0x61,
        0xc0, 0x80,                     /* illegal non-shortest form */
        0xe0, 0x80, 0x80,               /* illegal non-shortest form */
        0xf0, 0x80, 0x80, 0x80,         /* illegal non-shortest form */
        0xc0, 0xc0,                     /* illegal trail byte */
        0xf4, 0x90, 0x80, 0x80,         /* 0x110000 out of range */
        0xf8, 0x80, 0x80, 0x80, 0x80,   /* too long */
        0xfe,                           /* illegal byte altogether */
        0x62
    };

    /* expected error test results */
    static const int32_t results2[]={
        /* number of bytes read, code point */
        1, 0x61,
        22, 0x62
    };

    UConverterToUCallback cb;
    const void *p;

    const char *source=(const char *)in,*limit=(const char *)in+sizeof(in);
    UErrorCode errorCode=U_ZERO_ERROR;
    UConverter *cnv=ucnv_open("UTF-8", &errorCode);
    if(U_FAILURE(errorCode)) {
        log_err("Unable to open a UTF-8 converter: %s\n", u_errorName(errorCode));
        return;
    }
    TestNextUChar(cnv, source, limit, results, "UTF-8");
    /* Test the condition when source >= sourceLimit */
    TestNextUCharError(cnv, source, source, U_INDEX_OUTOFBOUNDS_ERROR, "sourceLimit <= source");

    /* test error behavior with a skip callback */
    ucnv_setToUCallBack(cnv, UCNV_TO_U_CALLBACK_SKIP, NULL, &cb, &p, &errorCode);
    source=(const char *)in2;
    limit=(const char *)(in2+sizeof(in2));
    TestNextUChar(cnv, source, limit, results2, "UTF-8");

    ucnv_close(cnv);
}

static void TestCESU8() {
    /* test input */
    static const uint8_t in[]={
        0x61,
        0xc2, 0x80,
        0xe0, 0xa0, 0x80,
        0xed, 0xa0, 0x80, 0xed, 0xb0, 0x80,
        0xed, 0xb0, 0x81, 0xed, 0xa0, 0x82,
        0xed, 0xaf, 0xbf, 0xed, 0xbf, 0xbf,
        0xef, 0xbf, 0xbc
    };

    /* expected test results */
    static const int32_t results[]={
        /* number of bytes read, code point */
        1, 0x61,
        2, 0x80,
        3, 0x800,
        6, 0x10000,
        3, 0xdc01,
        -1,0xd802,  /* may read 3 or 6 bytes */
        -1,0x10ffff,/* may read 0 or 3 bytes */
        3, 0xfffc
    };

    /* error test input */
    static const uint8_t in2[]={
        0x61,
        0xc0, 0x80,                     /* illegal non-shortest form */
        0xe0, 0x80, 0x80,               /* illegal non-shortest form */
        0xf0, 0x80, 0x80, 0x80,         /* illegal non-shortest form */
        0xc0, 0xc0,                     /* illegal trail byte */
        0xf0, 0x90, 0x80, 0x80,         /* illegal 4-byte supplementary code point */
        0xf4, 0x84, 0x8c, 0xa1,         /* illegal 4-byte supplementary code point */
        0xf0, 0x90, 0x90, 0x81,         /* illegal 4-byte supplementary code point */
        0xf4, 0x90, 0x80, 0x80,         /* 0x110000 out of range */
        0xf8, 0x80, 0x80, 0x80, 0x80,   /* too long */
        0xfe,                           /* illegal byte altogether */
        0x62
    };

    /* expected error test results */
    static const int32_t results2[]={
        /* number of bytes read, code point */
        1, 0x61,
        34, 0x62
    };

    UConverterToUCallback cb;
    const void *p;

    const char *source=(const char *)in,*limit=(const char *)in+sizeof(in);
    UErrorCode errorCode=U_ZERO_ERROR;
    UConverter *cnv=ucnv_open("CESU-8", &errorCode);
    if(U_FAILURE(errorCode)) {
        log_err("Unable to open a CESU-8 converter: %s\n", u_errorName(errorCode));
        return;
    }
    TestNextUChar(cnv, source, limit, results, "CESU-8");
    /* Test the condition when source >= sourceLimit */
    TestNextUCharError(cnv, source, source, U_INDEX_OUTOFBOUNDS_ERROR, "sourceLimit <= source");

    /* test error behavior with a skip callback */
    ucnv_setToUCallBack(cnv, UCNV_TO_U_CALLBACK_SKIP, NULL, &cb, &p, &errorCode);
    source=(const char *)in2;
    limit=(const char *)(in2+sizeof(in2));
    TestNextUChar(cnv, source, limit, results2, "CESU-8");

    ucnv_close(cnv);
}

static void TestUTF16() {
    /* test input */
    static const uint8_t in1[]={
        0xfe, 0xff, 0x4e, 0x00, 0xfe, 0xff
    };
    static const uint8_t in2[]={
        0xff, 0xfe, 0x4e, 0x00, 0xfe, 0xff
    };
    static const uint8_t in3[]={
        0xfe, 0xfe, 0x4e, 0x00, 0xfe, 0xff, 0xd8, 0x40, 0xdc, 0x01
    };

    /* expected test results */
    static const int32_t results1[]={
        /* number of bytes read, code point */
        4, 0x4e00,
        2, 0xfeff
    };
    static const int32_t results2[]={
        /* number of bytes read, code point */
        4, 0x004e,
        2, 0xfffe
    };
    static const int32_t results3[]={
        /* number of bytes read, code point */
        2, 0xfefe,
        2, 0x4e00,
        2, 0xfeff,
        4, 0x20001
    };

    const char *source, *limit;

    UErrorCode errorCode=U_ZERO_ERROR;
    UConverter *cnv=ucnv_open("UTF-16", &errorCode);
    if(U_FAILURE(errorCode)) {
        log_err("Unable to open a UTF-16 converter: %s\n", u_errorName(errorCode));
        return;
    }

    source=(const char *)in1, limit=(const char *)in1+sizeof(in1);
    TestNextUChar(cnv, source, limit, results1, "UTF-16");

    source=(const char *)in2, limit=(const char *)in2+sizeof(in2);
    ucnv_resetToUnicode(cnv);
    TestNextUChar(cnv, source, limit, results2, "UTF-16");

    source=(const char *)in3, limit=(const char *)in3+sizeof(in3);
    ucnv_resetToUnicode(cnv);
    TestNextUChar(cnv, source, limit, results3, "UTF-16");

    /* Test the condition when source >= sourceLimit */
    ucnv_resetToUnicode(cnv);
    TestNextUCharError(cnv, source, source, U_INDEX_OUTOFBOUNDS_ERROR, "sourceLimit <= source");

    ucnv_close(cnv);
}

static void TestUTF16BE() {
    /* test input */
    static const uint8_t in[]={
        0x00, 0x61,
        0x00, 0xc0,
        0x00, 0x31,
        0x00, 0xf4,
        0xce, 0xfe,
        0xd8, 0x01, 0xdc, 0x01
    };

    /* expected test results */
    static const int32_t results[]={
        /* number of bytes read, code point */
        2, 0x61,
        2, 0xc0,
        2, 0x31,
        2, 0xf4,
        2, 0xcefe,
        4, 0x10401
    };

    const char *source=(const char *)in, *limit=(const char *)in+sizeof(in);
    UErrorCode errorCode=U_ZERO_ERROR;
    UConverter *cnv=ucnv_open("utf-16be", &errorCode);
    if(U_FAILURE(errorCode)) {
        log_err("Unable to open a UTF16-BE converter: %s\n", u_errorName(errorCode));
        return;
    }
    TestNextUChar(cnv, source, limit, results, "UTF-16BE");
    /* Test the condition when source >= sourceLimit */
    TestNextUCharError(cnv, source, source, U_INDEX_OUTOFBOUNDS_ERROR, "sourceLimit <= source");
    /*Test for the condition where there is an invalid character*/
    {
        static const uint8_t source2[]={0x61};
        ucnv_setToUCallBack(cnv, UCNV_TO_U_CALLBACK_STOP, NULL, NULL, NULL, &errorCode);
        TestNextUCharError(cnv, (const char*)source2, (const char*)source2+sizeof(source2), U_TRUNCATED_CHAR_FOUND, "an invalid character");
    }
#if 0
    /*
     * Test disabled because currently the UTF-16BE/LE converters are supposed
     * to not set errors for unpaired surrogates.
     * This may change with
     * Jitterbug 1838 - forbid converting surrogate code points in UTF-16/32
     */

    /*Test for the condition where there is a surrogate pair*/
    {
        const uint8_t source2[]={0xd8, 0x01};
        TestNextUCharError(cnv, (const char*)source2, (const char*)source2+sizeof(source2), U_TRUNCATED_CHAR_FOUND, "an truncated surrogate character");
    }
#endif
    ucnv_close(cnv);
}

static void
TestUTF16LE() {
    /* test input */
    static const uint8_t in[]={
        0x61, 0x00,
        0x31, 0x00,
        0x4e, 0x2e,
        0x4e, 0x00,
        0x01, 0xd8, 0x01, 0xdc
    };

    /* expected test results */
    static const int32_t results[]={
        /* number of bytes read, code point */
        2, 0x61,
        2, 0x31,
        2, 0x2e4e,
        2, 0x4e,
        4, 0x10401
    };

    const char *source=(const char *)in, *limit=(const char *)in+sizeof(in);
    UErrorCode errorCode=U_ZERO_ERROR;
    UConverter *cnv=ucnv_open("utf-16le", &errorCode);
    if(U_FAILURE(errorCode)) {
        log_err("Unable to open a UTF16-LE converter: %s\n", u_errorName(errorCode));
        return;
    }
    TestNextUChar(cnv, source, limit, results, "UTF-16LE");
    /* Test the condition when source >= sourceLimit */
    TestNextUCharError(cnv, source, source, U_INDEX_OUTOFBOUNDS_ERROR, "sourceLimit <= source");
    /*Test for the condition where there is an invalid character*/
    {
        static const uint8_t source2[]={0x61};
        ucnv_setToUCallBack(cnv, UCNV_TO_U_CALLBACK_STOP, NULL, NULL, NULL, &errorCode);
        TestNextUCharError(cnv, (const char*)source2, (const char*)source2+sizeof(source2), U_TRUNCATED_CHAR_FOUND, "an invalid character");
    }
#if 0
    /*
     * Test disabled because currently the UTF-16BE/LE converters are supposed
     * to not set errors for unpaired surrogates.
     * This may change with
     * Jitterbug 1838 - forbid converting surrogate code points in UTF-16/32
     */

    /*Test for the condition where there is a surrogate character*/
    {
        static const uint8_t source2[]={0x01, 0xd8};
        TestNextUCharError(cnv, (const char*)source2, (const char*)source2+sizeof(source2), U_TRUNCATED_CHAR_FOUND, "an truncated surrogate character");
    }
#endif

    ucnv_close(cnv);
}

static void TestUTF32() {
    /* test input */
    static const uint8_t in1[]={
        0x00, 0x00, 0xfe, 0xff,   0x00, 0x10, 0x0f, 0x00,   0x00, 0x00, 0xfe, 0xff
    };
    static const uint8_t in2[]={
        0xff, 0xfe, 0x00, 0x00,   0x00, 0x10, 0x0f, 0x00,   0xfe, 0xff, 0x00, 0x00
    };
    static const uint8_t in3[]={
        0x00, 0x00, 0xfe, 0xfe,   0x00, 0x10, 0x0f, 0x00,   0x00, 0x00, 0xd8, 0x40,   0x00, 0x00, 0xdc, 0x01
    };

    /* expected test results */
    static const int32_t results1[]={
        /* number of bytes read, code point */
        8, 0x100f00,
        4, 0xfeff
    };
    static const int32_t results2[]={
        /* number of bytes read, code point */
        8, 0x0f1000,
        4, 0xfffe
    };
    static const int32_t results3[]={
        /* number of bytes read, code point */
        4, 0xfefe,
        4, 0x100f00,
        4, 0xfffd, /* unmatched surrogate */
        4, 0xfffd  /* unmatched surrogate */
    };

    const char *source, *limit;

    UErrorCode errorCode=U_ZERO_ERROR;
    UConverter *cnv=ucnv_open("UTF-32", &errorCode);
    if(U_FAILURE(errorCode)) {
        log_err("Unable to open a UTF-32 converter: %s\n", u_errorName(errorCode));
        return;
    }

    source=(const char *)in1, limit=(const char *)in1+sizeof(in1);
    TestNextUChar(cnv, source, limit, results1, "UTF-32");

    source=(const char *)in2, limit=(const char *)in2+sizeof(in2);
    ucnv_resetToUnicode(cnv);
    TestNextUChar(cnv, source, limit, results2, "UTF-32");

    source=(const char *)in3, limit=(const char *)in3+sizeof(in3);
    ucnv_resetToUnicode(cnv);
    TestNextUChar(cnv, source, limit, results3, "UTF-32");

    /* Test the condition when source >= sourceLimit */
    ucnv_resetToUnicode(cnv);
    TestNextUCharError(cnv, source, source, U_INDEX_OUTOFBOUNDS_ERROR, "sourceLimit <= source");

    ucnv_close(cnv);
}

static void
TestUTF32BE() {
    /* test input */
    static const uint8_t in[]={
        0x00, 0x00, 0x00, 0x61,
        0x00, 0x00, 0x30, 0x61,
        0x00, 0x00, 0xdc, 0x00,
        0x00, 0x00, 0xd8, 0x00,
        0x00, 0x00, 0xdf, 0xff,
        0x00, 0x00, 0xff, 0xfe,
        0x00, 0x10, 0xab, 0xcd,
        0x00, 0x10, 0xff, 0xff
    };

    /* expected test results */
    static const int32_t results[]={
        /* number of bytes read, code point */
        4, 0x61,
        4, 0x3061,
        4, 0xfffd,
        4, 0xfffd,
        4, 0xfffd,
        4, 0xfffe,
        4, 0x10abcd,
        4, 0x10ffff
    };

    /* error test input */
    static const uint8_t in2[]={
        0x00, 0x00, 0x00, 0x61,
        0x00, 0x11, 0x00, 0x00,         /* 0x110000 out of range */
        0x00, 0x00, 0x00, 0x62,
        0xff, 0xff, 0xff, 0xff,         /* 0xffffffff out of range */
        0x7f, 0xff, 0xff, 0xff,         /* 0x7fffffff out of range */
        0x00, 0x00, 0x01, 0x62,
        0x00, 0x00, 0x02, 0x62
    };

    /* expected error test results */
    static const int32_t results2[]={
        /* number of bytes read, code point */
        4,  0x61,
        8,  0x62,
        12, 0x162,
        4,  0x262
    };

    UConverterToUCallback cb;
    const void *p;

    const char *source=(const char *)in, *limit=(const char *)in+sizeof(in);
    UErrorCode errorCode=U_ZERO_ERROR;
    UConverter *cnv=ucnv_open("UTF-32BE", &errorCode);
    if(U_FAILURE(errorCode)) {
        log_err("Unable to open a UTF-32BE converter: %s\n", u_errorName(errorCode));
        return;
    }
    TestNextUChar(cnv, source, limit, results, "UTF-32BE");

    /* Test the condition when source >= sourceLimit */
    TestNextUCharError(cnv, source, source, U_INDEX_OUTOFBOUNDS_ERROR, "sourceLimit <= source");

    /* test error behavior with a skip callback */
    ucnv_setToUCallBack(cnv, UCNV_TO_U_CALLBACK_SKIP, NULL, &cb, &p, &errorCode);
    source=(const char *)in2;
    limit=(const char *)(in2+sizeof(in2));
    TestNextUChar(cnv, source, limit, results2, "UTF-32BE");

    ucnv_close(cnv);
}

static void
TestUTF32LE() {
    /* test input */
    static const uint8_t in[]={
        0x61, 0x00, 0x00, 0x00,
        0x61, 0x30, 0x00, 0x00,
        0x00, 0xdc, 0x00, 0x00,
        0x00, 0xd8, 0x00, 0x00,
        0xff, 0xdf, 0x00, 0x00,
        0xfe, 0xff, 0x00, 0x00,
        0xcd, 0xab, 0x10, 0x00,
        0xff, 0xff, 0x10, 0x00
    };

    /* expected test results */
    static const int32_t results[]={
        /* number of bytes read, code point */
        4, 0x61,
        4, 0x3061,
        4, 0xfffd,
        4, 0xfffd,
        4, 0xfffd,
        4, 0xfffe,
        4, 0x10abcd,
        4, 0x10ffff
    };

    /* error test input */
    static const uint8_t in2[]={
        0x61, 0x00, 0x00, 0x00,
        0x00, 0x00, 0x11, 0x00,         /* 0x110000 out of range */
        0x62, 0x00, 0x00, 0x00,
        0xff, 0xff, 0xff, 0xff,         /* 0xffffffff out of range */
        0xff, 0xff, 0xff, 0x7f,         /* 0x7fffffff out of range */
        0x62, 0x01, 0x00, 0x00,
        0x62, 0x02, 0x00, 0x00,
    };

    /* expected error test results */
    static const int32_t results2[]={
        /* number of bytes read, code point */
        4,  0x61,
        8,  0x62,
        12, 0x162,
        4,  0x262,
    };

    UConverterToUCallback cb;
    const void *p;

    const char *source=(const char *)in, *limit=(const char *)in+sizeof(in);
    UErrorCode errorCode=U_ZERO_ERROR;
    UConverter *cnv=ucnv_open("UTF-32LE", &errorCode);
    if(U_FAILURE(errorCode)) {
        log_err("Unable to open a UTF-32LE converter: %s\n", u_errorName(errorCode));
        return;
    }
    TestNextUChar(cnv, source, limit, results, "UTF-32LE");

    /* Test the condition when source >= sourceLimit */
    TestNextUCharError(cnv, source, source, U_INDEX_OUTOFBOUNDS_ERROR, "sourceLimit <= source");

    /* test error behavior with a skip callback */
    ucnv_setToUCallBack(cnv, UCNV_TO_U_CALLBACK_SKIP, NULL, &cb, &p, &errorCode);
    source=(const char *)in2;
    limit=(const char *)(in2+sizeof(in2));
    TestNextUChar(cnv, source, limit, results2, "UTF-32LE");

    ucnv_close(cnv);
}

static void
TestLATIN1() {
    /* test input */
    static const uint8_t in[]={
       0x61,
       0x31,
       0x32,
       0xc0,
       0xf0,
       0xf4,
    };

    /* expected test results */
    static const int32_t results[]={
        /* number of bytes read, code point */
        1, 0x61,
        1, 0x31,
        1, 0x32,
        1, 0xc0,
        1, 0xf0,
        1, 0xf4,
    };
    static const uint16_t in1[] = {
        0x08, 0x00, 0x1b, 0x4c, 0xea, 0x16, 0xca, 0xd3, 0x94, 0x0f, 0x53, 0xef, 0x61, 0x1b, 0xe5, 0x84,
        0xc4, 0x0f, 0x53, 0xef, 0x61, 0x1b, 0xe5, 0x84, 0xc4, 0x16, 0xca, 0xd3, 0x94, 0x08, 0x02, 0x0f,
        0x53, 0x4a, 0x4e, 0x16, 0x7d, 0x00, 0x30, 0x82, 0x52, 0x4d, 0x30, 0x6b, 0x6d, 0x41, 0x88, 0x4c,
        0xe5, 0x97, 0x9f, 0x08, 0x0c, 0x16, 0xca, 0xd3, 0x94, 0x15, 0xae, 0x0e, 0x6b, 0x4c, 0x08, 0x0d,
        0x8c, 0xb4, 0xa3, 0x9f, 0xca, 0x99, 0xcb, 0x8b, 0xc2, 0x97, 0xcc, 0xaa, 0x84, 0x08, 0x02, 0x0e,
        0x7c, 0x73, 0xe2, 0x16, 0xa3, 0xb7, 0xcb, 0x93, 0xd3, 0xb4, 0xc5, 0xdc, 0x9f, 0x0e, 0x79, 0x3e,
        0x06, 0xae, 0xb1, 0x9d, 0x93, 0xd3, 0x08, 0x0c, 0xbe, 0xa3, 0x8f, 0x08, 0x88, 0xbe, 0xa3, 0x8d,
        0xd3, 0xa8, 0xa3, 0x97, 0xc5, 0x17, 0x89, 0x08, 0x0d, 0x15, 0xd2, 0x08, 0x01, 0x93, 0xc8, 0xaa,
        0x8f, 0x0e, 0x61, 0x1b, 0x99, 0xcb, 0x0e, 0x4e, 0xba, 0x9f, 0xa1, 0xae, 0x93, 0xa8, 0xa0, 0x08,
        0x02, 0x08, 0x0c, 0xe2, 0x16, 0xa3, 0xb7, 0xcb, 0x0f, 0x4f, 0xe1, 0x80, 0x05, 0xec, 0x60, 0x8d,
        0xea, 0x06, 0xd3, 0xe6, 0x0f, 0x8a, 0x00, 0x30, 0x44, 0x65, 0xb9, 0xe4, 0xfe, 0xe7, 0xc2, 0x06,
        0xcb, 0x82
    };
    static const uint8_t out1[] = {
        0x08, 0x00, 0x1b, 0x4c, 0xea, 0x16, 0xca, 0xd3, 0x94, 0x0f, 0x53, 0xef, 0x61, 0x1b, 0xe5, 0x84,
        0xc4, 0x0f, 0x53, 0xef, 0x61, 0x1b, 0xe5, 0x84, 0xc4, 0x16, 0xca, 0xd3, 0x94, 0x08, 0x02, 0x0f,
        0x53, 0x4a, 0x4e, 0x16, 0x7d, 0x00, 0x30, 0x82, 0x52, 0x4d, 0x30, 0x6b, 0x6d, 0x41, 0x88, 0x4c,
        0xe5, 0x97, 0x9f, 0x08, 0x0c, 0x16, 0xca, 0xd3, 0x94, 0x15, 0xae, 0x0e, 0x6b, 0x4c, 0x08, 0x0d,
        0x8c, 0xb4, 0xa3, 0x9f, 0xca, 0x99, 0xcb, 0x8b, 0xc2, 0x97, 0xcc, 0xaa, 0x84, 0x08, 0x02, 0x0e,
        0x7c, 0x73, 0xe2, 0x16, 0xa3, 0xb7, 0xcb, 0x93, 0xd3, 0xb4, 0xc5, 0xdc, 0x9f, 0x0e, 0x79, 0x3e,
        0x06, 0xae, 0xb1, 0x9d, 0x93, 0xd3, 0x08, 0x0c, 0xbe, 0xa3, 0x8f, 0x08, 0x88, 0xbe, 0xa3, 0x8d,
        0xd3, 0xa8, 0xa3, 0x97, 0xc5, 0x17, 0x89, 0x08, 0x0d, 0x15, 0xd2, 0x08, 0x01, 0x93, 0xc8, 0xaa,
        0x8f, 0x0e, 0x61, 0x1b, 0x99, 0xcb, 0x0e, 0x4e, 0xba, 0x9f, 0xa1, 0xae, 0x93, 0xa8, 0xa0, 0x08,
        0x02, 0x08, 0x0c, 0xe2, 0x16, 0xa3, 0xb7, 0xcb, 0x0f, 0x4f, 0xe1, 0x80, 0x05, 0xec, 0x60, 0x8d,
        0xea, 0x06, 0xd3, 0xe6, 0x0f, 0x8a, 0x00, 0x30, 0x44, 0x65, 0xb9, 0xe4, 0xfe, 0xe7, 0xc2, 0x06,
        0xcb, 0x82
    };
    static const uint16_t in2[]={
        0x1B, 0x24, 0x29, 0x47, 0x0E, 0x23, 0x21, 0x23, 0x22, 0x23,
        0x23, 0x23, 0x24, 0x23, 0x25, 0x23, 0x26, 0x23, 0x27, 0x23,
        0x28, 0x23, 0x29, 0x23, 0x2A, 0x23, 0x2B, 0x0F, 0x2F, 0x2A,
        0x70, 0x6C, 0x61, 0x6E, 0x65, 0x20, 0x31, 0x20, 0x2A, 0x2F,
        0x0D, 0x0A, 0x1B, 0x24, 0x2A, 0x48, 0x1B, 0x4E, 0x22, 0x21,
        0x1B, 0x4E, 0x22, 0x22, 0x1B, 0x4E, 0x22, 0x23, 0x1B, 0x4E,
        0x22, 0x24, 0x1B, 0x4E, 0x22, 0x25, 0x0F, 0x2F, 0x2A, 0x70,
        0x6C, 0x61, 0x6E, 0x65, 0x32, 0x2A, 0x2F, 0x20, 0x0D, 0x0A,
        0x1B, 0x24, 0x2B, 0x49, 0x1B, 0x4F, 0x22, 0x44, 0x1B, 0x4F,
        0x22, 0x45, 0x1B, 0x4F, 0x22, 0x46, 0x1B, 0x4F, 0x22, 0x47,
        0x1B, 0x4F, 0x22, 0x48, 0x0F, 0x2F, 0x2A, 0x70, 0x6C, 0x61,
        0x6E, 0x65, 0x20, 0x33, 0x20, 0x2A, 0x2F, 0x0D, 0x0A, 0x1B,
        0x24, 0x2B, 0x4A, 0x1B, 0x4F, 0x21, 0x44, 0x1B, 0x4F, 0x21,
        0x45, 0x1B, 0x4F, 0x22, 0x6A, 0x1B, 0x4F, 0x22, 0x6B, 0x1B,
        0x4F, 0x22, 0x6C, 0x0F, 0x2F, 0x2A, 0x70, 0x6C, 0x61, 0x6E,
        0x65, 0x20, 0x34, 0x20, 0x2A, 0x2F, 0x0D, 0x0A, 0x1B, 0x24,
        0x2B, 0x4B, 0x1B, 0x4F, 0x21, 0x74, 0x1B, 0x4F, 0x22, 0x50,
        0x1B, 0x4F, 0x22, 0x51, 0x1B, 0x4F, 0x23, 0x37, 0x1B, 0x4F,
        0x22, 0x5C, 0x0F, 0x2F, 0x2A, 0x70, 0x6C, 0x61, 0x6E,
        0x65, 0x20, 0x35, 0x20, 0x2A, 0x2F, 0x0D, 0x0A, 0x1B, 0x24,
        0x2B, 0x4C, 0x1B, 0x4F, 0x21, 0x23, 0x1B, 0x4F, 0x22, 0x2C,
        0x1B, 0x4F, 0x23, 0x4E, 0x1B, 0x4F, 0x21, 0x6E, 0x1B, 0x4F,
        0x23, 0x71, 0x0F, 0x2F, 0x2A, 0x70, 0x6C, 0x61, 0x6E, 0x65,
        0x20, 0x36, 0x20, 0x2A, 0x2F, 0x0D, 0x0A, 0x1B, 0x24, 0x2B,
        0x4D, 0x1B, 0x4F, 0x22, 0x71, 0x1B, 0x4F, 0x21, 0x4E, 0x1B,
        0x4F, 0x21, 0x6A, 0x1B, 0x4F, 0x23, 0x3A, 0x1B, 0x4F, 0x23,
        0x6F, 0x0F, 0x2F, 0x2A, 0x70, 0x6C, 0x61, 0x6E, 0x65, 0x20,
        0x37, 0x20, 0x2A, 0x2F,
    };
    static const unsigned char out2[]={
        0x1B, 0x24, 0x29, 0x47, 0x0E, 0x23, 0x21, 0x23, 0x22, 0x23,
        0x23, 0x23, 0x24, 0x23, 0x25, 0x23, 0x26, 0x23, 0x27, 0x23,
        0x28, 0x23, 0x29, 0x23, 0x2A, 0x23, 0x2B, 0x0F, 0x2F, 0x2A,
        0x70, 0x6C, 0x61, 0x6E, 0x65, 0x20, 0x31, 0x20, 0x2A, 0x2F,
        0x0D, 0x0A, 0x1B, 0x24, 0x2A, 0x48, 0x1B, 0x4E, 0x22, 0x21,
        0x1B, 0x4E, 0x22, 0x22, 0x1B, 0x4E, 0x22, 0x23, 0x1B, 0x4E,
        0x22, 0x24, 0x1B, 0x4E, 0x22, 0x25, 0x0F, 0x2F, 0x2A, 0x70,
        0x6C, 0x61, 0x6E, 0x65, 0x32, 0x2A, 0x2F, 0x20, 0x0D, 0x0A,
        0x1B, 0x24, 0x2B, 0x49, 0x1B, 0x4F, 0x22, 0x44, 0x1B, 0x4F,
        0x22, 0x45, 0x1B, 0x4F, 0x22, 0x46, 0x1B, 0x4F, 0x22, 0x47,
        0x1B, 0x4F, 0x22, 0x48, 0x0F, 0x2F, 0x2A, 0x70, 0x6C, 0x61,
        0x6E, 0x65, 0x20, 0x33, 0x20, 0x2A, 0x2F, 0x0D, 0x0A, 0x1B,
        0x24, 0x2B, 0x4A, 0x1B, 0x4F, 0x21, 0x44, 0x1B, 0x4F, 0x21,
        0x45, 0x1B, 0x4F, 0x22, 0x6A, 0x1B, 0x4F, 0x22, 0x6B, 0x1B,
        0x4F, 0x22, 0x6C, 0x0F, 0x2F, 0x2A, 0x70, 0x6C, 0x61, 0x6E,
        0x65, 0x20, 0x34, 0x20, 0x2A, 0x2F, 0x0D, 0x0A, 0x1B, 0x24,
        0x2B, 0x4B, 0x1B, 0x4F, 0x21, 0x74, 0x1B, 0x4F, 0x22, 0x50,
        0x1B, 0x4F, 0x22, 0x51, 0x1B, 0x4F, 0x23, 0x37, 0x1B, 0x4F,
        0x22, 0x5C, 0x0F, 0x2F, 0x2A, 0x70, 0x6C, 0x61, 0x6E,
        0x65, 0x20, 0x35, 0x20, 0x2A, 0x2F, 0x0D, 0x0A, 0x1B, 0x24,
        0x2B, 0x4C, 0x1B, 0x4F, 0x21, 0x23, 0x1B, 0x4F, 0x22, 0x2C,
        0x1B, 0x4F, 0x23, 0x4E, 0x1B, 0x4F, 0x21, 0x6E, 0x1B, 0x4F,
        0x23, 0x71, 0x0F, 0x2F, 0x2A, 0x70, 0x6C, 0x61, 0x6E, 0x65,
        0x20, 0x36, 0x20, 0x2A, 0x2F, 0x0D, 0x0A, 0x1B, 0x24, 0x2B,
        0x4D, 0x1B, 0x4F, 0x22, 0x71, 0x1B, 0x4F, 0x21, 0x4E, 0x1B,
        0x4F, 0x21, 0x6A, 0x1B, 0x4F, 0x23, 0x3A, 0x1B, 0x4F, 0x23,
        0x6F, 0x0F, 0x2F, 0x2A, 0x70, 0x6C, 0x61, 0x6E, 0x65, 0x20,
        0x37, 0x20, 0x2A, 0x2F,
    };
    const char *source=(const char *)in;
    const char *limit=(const char *)in+sizeof(in);

    UErrorCode errorCode=U_ZERO_ERROR;
    UConverter *cnv=ucnv_open("LATIN_1", &errorCode);
    if(U_FAILURE(errorCode)) {
        log_data_err("Unable to open a LATIN_1 converter: %s\n", u_errorName(errorCode));
        return;
    }
    TestNextUChar(cnv, source, limit, results, "LATIN_1");
    /* Test the condition when source >= sourceLimit */
    TestNextUCharError(cnv, source, source, U_INDEX_OUTOFBOUNDS_ERROR, "sourceLimit <= source");
    TestConv((uint16_t*)in1,sizeof(in1)/2,"LATIN_1","LATIN-1",(char*)out1,sizeof(out1));
    TestConv((uint16_t*)in2,sizeof(in2)/2,"ASCII","ASCII",(char*)out2,sizeof(out2));

    ucnv_close(cnv);
}

static void
TestSBCS() {
    /* test input */
    static const uint8_t in[]={ 0x61, 0xc0, 0x80, 0xe0, 0xf0, 0xf4};
    /* expected test results */
    static const int32_t results[]={
        /* number of bytes read, code point */
        1, 0x61,
        1, 0xbf,
        1, 0xc4,
        1, 0x2021,
        1, 0xf8ff,
        1, 0x00d9
    };

    const char *source=(const char *)in, *limit=(const char *)in+sizeof(in);
    UErrorCode errorCode=U_ZERO_ERROR;
    UConverter *cnv=ucnv_open("x-mac-turkish", &errorCode);
    if(U_FAILURE(errorCode)) {
        log_data_err("Unable to open a SBCS(x-mac-turkish) converter: %s\n", u_errorName(errorCode));
        return;
    }
    TestNextUChar(cnv, source, limit, results, "SBCS(x-mac-turkish)");
    /* Test the condition when source >= sourceLimit */
    TestNextUCharError(cnv, source, source, U_INDEX_OUTOFBOUNDS_ERROR, "sourceLimit <= source");
    /*Test for Illegal character */ /*
    {
    static const uint8_t input1[]={ 0xA1 };
    const char* illegalsource=(const char*)input1;
    TestNextUCharError(cnv, illegalsource, illegalsource+sizeof(illegalsource), U_INVALID_CHAR_FOUND, "source has a illegal characte");
    }
   */
    ucnv_close(cnv);
}

static void
TestDBCS() {
    /* test input */
    static const uint8_t in[]={
        0x44, 0x6a,
        0xc4, 0x9c,
        0x7a, 0x74,
        0x46, 0xab,
        0x42, 0x5b,

    };

    /* expected test results */
    static const int32_t results[]={
        /* number of bytes read, code point */
        2, 0x00a7,
        2, 0xe1d2,
        2, 0x6962,
        2, 0xf842,
        2, 0xffe5,
    };

    const char *source=(const char *)in, *limit=(const char *)in+sizeof(in);
    UErrorCode errorCode=U_ZERO_ERROR;

    UConverter *cnv=my_ucnv_open("@ibm9027", &errorCode);
    if(U_FAILURE(errorCode)) {
        log_data_err("Unable to open a DBCS(@ibm9027) converter: %s\n", u_errorName(errorCode));
        return;
    }
    TestNextUChar(cnv, source, limit, results, "DBCS(@ibm9027)");
    /* Test the condition when source >= sourceLimit */
    TestNextUCharError(cnv, source, source, U_INDEX_OUTOFBOUNDS_ERROR, "sourceLimit <= source");
    /*Test for the condition where there is an invalid character*/
    {
        static const uint8_t source2[]={0x1a, 0x1b};
        TestNextUCharError(cnv, (const char*)source2, (const char*)source2+sizeof(source2), U_ZERO_ERROR, "an invalid character");
    }
    /*Test for the condition where we have a truncated char*/
    {
        static const uint8_t source1[]={0xc4};
        ucnv_setToUCallBack(cnv, UCNV_TO_U_CALLBACK_STOP, NULL, NULL, NULL, &errorCode);
        TestNextUCharError(cnv, (const char*)source1, (const char*)source1+sizeof(source1), U_TRUNCATED_CHAR_FOUND, "a character is truncated");
    }
    ucnv_close(cnv);
}

static void
TestMBCS() {
    /* test input */
    static const uint8_t in[]={
        0x01,
        0xa6, 0xa3,
        0x00,
        0xa6, 0xa1,
        0x08,
        0xc2, 0x76,
        0xc2, 0x78,

    };

    /* expected test results */
    static const int32_t results[]={
        /* number of bytes read, code point */
        1, 0x0001,
        2, 0x250c,
        1, 0x0000,
        2, 0x2500,
        1, 0x0008,
        2, 0xd60c,
        2, 0xd60e,
    };

    const char *source=(const char *)in, *limit=(const char *)in+sizeof(in);
    UErrorCode errorCode=U_ZERO_ERROR;

    UConverter *cnv=ucnv_open("ibm-1363", &errorCode);
    if(U_FAILURE(errorCode)) {
        log_data_err("Unable to open a MBCS(ibm-1363) converter: %s\n", u_errorName(errorCode));
        return;
    }
    TestNextUChar(cnv, source, limit, results, "MBCS(ibm-1363)");
    /* Test the condition when source >= sourceLimit */
    TestNextUCharError(cnv, source, source, U_INDEX_OUTOFBOUNDS_ERROR, "sourceLimit <= source");
    /*Test for the condition where there is an invalid character*/
    {
        static const uint8_t source2[]={0xa1, 0x80};
        TestNextUCharError(cnv, (const char*)source2, (const char*)source2+sizeof(source2), U_ZERO_ERROR, "an invalid character");
    }
    /*Test for the condition where we have a truncated char*/
    {
        static const uint8_t source1[]={0xc4};
        ucnv_setToUCallBack(cnv, UCNV_TO_U_CALLBACK_STOP, NULL, NULL, NULL, &errorCode);
        TestNextUCharError(cnv, (const char*)source1, (const char*)source1+sizeof(source1), U_TRUNCATED_CHAR_FOUND, "a character is truncated");
    }
    ucnv_close(cnv);

}

#if !UCONFIG_NO_LEGACY_CONVERSION && !UCONFIG_NO_FILE_IO
static void
TestICCRunout() {
/*    { "ibm-1363", :bin{ a2aea2 }, "\u00a1\u001a", :intvector{ 0, 2 }, :int{1}, :int{0}, "\", "?", :bin{""} } */

    const char *cnvName = "ibm-1363";
    UErrorCode status = U_ZERO_ERROR;
    const char sourceData[] = { (char)0xa2, (char)0xae, (char)0xa2 };
    /* UChar   expectUData[] = { 0x00a1, 0x001a }; */
    const char *source = sourceData;
    const char *sourceLim = sourceData+sizeof(sourceData);
    UChar c1, c2, c3;
    UConverter *cnv=ucnv_open(cnvName, &status);
    if(U_FAILURE(status)) {
        log_data_err("Unable to open %s converter: %s\n", cnvName, u_errorName(status));
	return;
    }
    
#if 0
    {
    UChar   targetBuf[256];
    UChar   *target = targetBuf;
    UChar   *targetLim = target+256;
    ucnv_toUnicode(cnv, &target, targetLim, &source, sourceLim, NULL, TRUE, &status);

    log_info("After convert: target@%d, source@%d, status%s\n",
	     target-targetBuf, source-sourceData, u_errorName(status));

    if(U_FAILURE(status)) {
	log_err("Failed to convert: %s\n", u_errorName(status));
    } else {
	
    }
    }
#endif

    c1=ucnv_getNextUChar(cnv, &source, sourceLim, &status);
    log_verbose("c1: U+%04X, source@%d, status %s\n", c1, source-sourceData, u_errorName(status));

    c2=ucnv_getNextUChar(cnv, &source, sourceLim, &status);
    log_verbose("c2: U+%04X, source@%d, status %s\n", c2, source-sourceData, u_errorName(status));

    c3=ucnv_getNextUChar(cnv, &source, sourceLim, &status);
    log_verbose("c3: U+%04X, source@%d, status %s\n", c3, source-sourceData, u_errorName(status));

    if(status==U_INDEX_OUTOFBOUNDS_ERROR && c3==0xFFFF) {
	log_verbose("OK\n");
    } else {
	log_err("FAIL: c3 was not FFFF or err was not U_INDEXOUTOFBOUNDS_ERROR\n");
    }

    ucnv_close(cnv);
    
}
#endif

#ifdef U_ENABLE_GENERIC_ISO_2022

static void
TestISO_2022() {
    /* test input */
    static const uint8_t in[]={
        0x1b, 0x25, 0x42,
        0x31,
        0x32,
        0x61,
        0xc2, 0x80,
        0xe0, 0xa0, 0x80,
        0xf0, 0x90, 0x80, 0x80
    };



    /* expected test results */
    static const int32_t results[]={
        /* number of bytes read, code point */
        4, 0x0031,  /* 4 bytes including the escape sequence */
        1, 0x0032,
        1, 0x61,
        2, 0x80,
        3, 0x800,
        4, 0x10000
    };

    const char *source=(const char *)in, *limit=(const char *)in+sizeof(in);
    UErrorCode errorCode=U_ZERO_ERROR;
    UConverter *cnv;

    cnv=ucnv_open("ISO_2022", &errorCode);
    if(U_FAILURE(errorCode)) {
        log_data_err("Unable to open a iso-2022 converter: %s\n", u_errorName(errorCode));
        return;
    }
    TestNextUChar(cnv, source, limit, results, "ISO_2022");

    /* Test the condition when source >= sourceLimit */
    TestNextUCharError(cnv, source, source-1, U_ILLEGAL_ARGUMENT_ERROR, "sourceLimit < source");
    TestNextUCharError(cnv, source, source, U_INDEX_OUTOFBOUNDS_ERROR, "sourceLimit <= source");
    /*Test for the condition where we have a truncated char*/
    {
        static const uint8_t source1[]={0xc4};
        ucnv_setToUCallBack(cnv, UCNV_TO_U_CALLBACK_STOP, NULL, NULL, NULL, &errorCode);
        TestNextUCharError(cnv, (const char*)source1, (const char*)source1+sizeof(source1), U_TRUNCATED_CHAR_FOUND, "a character is truncated");
    }
    /*Test for the condition where there is an invalid character*/
    {
        static const uint8_t source2[]={0xa1, 0x01};
        TestNextUCharError(cnv, (const char*)source2, (const char*)source2+sizeof(source2), U_ILLEGAL_CHAR_FOUND, "an invalid character");
    }
    ucnv_close(cnv);
}

#endif

static void
TestSmallTargetBuffer(const uint16_t* source, const UChar* sourceLimit,UConverter* cnv){
    const UChar* uSource;
    const UChar* uSourceLimit;
    const char* cSource;
    const char* cSourceLimit;
    UChar *uTargetLimit =NULL;
    UChar *uTarget;
    char *cTarget;
    const char *cTargetLimit;
    char *cBuf;
    UChar *uBuf; /*,*test;*/
    int32_t uBufSize = 120;
    int len=0;
    int i=2;
    UErrorCode errorCode=U_ZERO_ERROR;
    uBuf =  (UChar*)malloc(uBufSize * sizeof(UChar)*5);
    cBuf =(char*)malloc(uBufSize * sizeof(char) * 10);
    ucnv_reset(cnv);
    for(;--i>0; ){
        uSource = (UChar*) source;
        uSourceLimit=(const UChar*)sourceLimit;
        cTarget = cBuf;
        uTarget = uBuf;
        cSource = cBuf;
        cTargetLimit = cBuf;
        uTargetLimit = uBuf;

        do{

            cTargetLimit = cTargetLimit+ i;
            ucnv_fromUnicode( cnv , &cTarget, cTargetLimit,&uSource,uSourceLimit,NULL,FALSE, &errorCode);
            if(errorCode==U_BUFFER_OVERFLOW_ERROR){
               errorCode=U_ZERO_ERROR;
                continue;
            }

            if(U_FAILURE(errorCode)){
                log_err("ucnv_toUnicode conversion failed reason %s\n", u_errorName(errorCode));
                return;
            }

        }while (uSource<uSourceLimit);

        cSourceLimit =cTarget;
        do{
            uTargetLimit=uTargetLimit+i;
            ucnv_toUnicode(cnv,&uTarget,uTargetLimit,&cSource,cSourceLimit,NULL,FALSE,&errorCode);
            if(errorCode==U_BUFFER_OVERFLOW_ERROR){
               errorCode=U_ZERO_ERROR;
                continue;
            }
            if(U_FAILURE(errorCode)){
                   log_err("ucnv_toUnicode conversion failed reason %s\n", u_errorName(errorCode));
                    return;
            }
        }while(cSource<cSourceLimit);

        uSource = source;
        /*test =uBuf;*/
        for(len=0;len<(int)(source - sourceLimit);len++){
            if(uBuf[len]!=uSource[len]){
                log_err("Expected : \\u%04X \t Got: \\u%04X\n",uSource[len],(int)uBuf[len]) ;
            }
        }
    }
    free(uBuf);
    free(cBuf);
}
/* Test for Jitterbug 778 */
static void TestToAndFromUChars(const uint16_t* source, const UChar* sourceLimit,UConverter* cnv){
    const UChar* uSource;
    const UChar* uSourceLimit;
    const char* cSource;
    UChar *uTargetLimit =NULL;
    UChar *uTarget;
    char *cTarget;
    const char *cTargetLimit;
    char *cBuf;
    UChar *uBuf,*test;
    int32_t uBufSize = 120;
    int numCharsInTarget=0;
    UErrorCode errorCode=U_ZERO_ERROR;
    uBuf =  (UChar*)malloc(uBufSize * sizeof(UChar)*5);
    cBuf =(char*)malloc(uBufSize * sizeof(char) * 5);
    uSource = source;
    uSourceLimit=sourceLimit;
    cTarget = cBuf;
    cTargetLimit = cBuf +uBufSize*5;
    uTarget = uBuf;
    uTargetLimit = uBuf+ uBufSize*5;
    ucnv_reset(cnv);
    numCharsInTarget=ucnv_fromUChars(cnv, cTarget, (int32_t)(cTargetLimit-cTarget), uSource, (int32_t)(uSourceLimit-uSource), &errorCode);
    if(U_FAILURE(errorCode)){
        log_err("ucnv_fromUnicode conversion failed reason %s\n", u_errorName(errorCode));
        return;
    }
    cSource = cBuf;
    test =uBuf;
    ucnv_toUChars(cnv,uTarget,(int32_t)(uTargetLimit-uTarget),cSource,numCharsInTarget,&errorCode);
    if(U_FAILURE(errorCode)){
        log_err("ucnv_toUChars conversion failed, reason %s\n", u_errorName(errorCode));
        return;
    }
    uSource = source;
    while(uSource<uSourceLimit){
        if(*test!=*uSource){

            log_err("Expected : \\u%04X \t Got: \\u%04X\n",*uSource,(int)*test) ;
        }
        uSource++;
        test++;
    }
    free(uBuf);
    free(cBuf);
}

static void TestSmallSourceBuffer(const uint16_t* source, const UChar* sourceLimit,UConverter* cnv){
    const UChar* uSource;
    const UChar* uSourceLimit;
    const char* cSource;
    const char* cSourceLimit;
    UChar *uTargetLimit =NULL;
    UChar *uTarget;
    char *cTarget;
    const char *cTargetLimit;
    char *cBuf;
    UChar *uBuf; /*,*test;*/
    int32_t uBufSize = 120;
    int len=0;
    int i=2;
    const UChar *temp = sourceLimit;
    UErrorCode errorCode=U_ZERO_ERROR;
    uBuf =  (UChar*)malloc(uBufSize * sizeof(UChar)*5);
    cBuf =(char*)malloc(uBufSize * sizeof(char) * 10);

    ucnv_reset(cnv);
    for(;--i>0;){
        uSource = (UChar*) source;
        cTarget = cBuf;
        uTarget = uBuf;
        cSource = cBuf;
        cTargetLimit = cBuf;
        uTargetLimit = uBuf+uBufSize*5;
        cTargetLimit = cTargetLimit+uBufSize*10;
        uSourceLimit=uSource;
        do{

            if (uSourceLimit < sourceLimit) {
                uSourceLimit = uSourceLimit+1;
            }
            ucnv_fromUnicode( cnv , &cTarget, cTargetLimit,&uSource,uSourceLimit,NULL,FALSE, &errorCode);
            if(errorCode==U_BUFFER_OVERFLOW_ERROR){
               errorCode=U_ZERO_ERROR;
                continue;
            }

            if(U_FAILURE(errorCode)){
                log_err("ucnv_toUnicode conversion failed reason %s\n", u_errorName(errorCode));
                return;
            }

        }while (uSource<temp);

        cSourceLimit =cBuf;
        do{
            if (cSourceLimit < cBuf + (cTarget - cBuf)) {
                cSourceLimit = cSourceLimit+1;
            }
            ucnv_toUnicode(cnv,&uTarget,uTargetLimit,&cSource,cSourceLimit,NULL,FALSE,&errorCode);
            if(errorCode==U_BUFFER_OVERFLOW_ERROR){
               errorCode=U_ZERO_ERROR;
                continue;
            }
            if(U_FAILURE(errorCode)){
                   log_err("ucnv_toUnicode conversion failed reason %s\n", u_errorName(errorCode));
                    return;
            }
        }while(cSource<cTarget);

        uSource = source;
        /*test =uBuf;*/
        for(;len<(int)(source - sourceLimit);len++){
            if(uBuf[len]!=uSource[len]){
                log_err("Expected : \\u%04X \t Got: \\u%04X\n",uSource[len],(int)uBuf[len]) ;
            }
        }
    }
    free(uBuf);
    free(cBuf);
}
static void
TestGetNextUChar2022(UConverter* cnv, const char* source, const char* limit,
                     const uint16_t results[], const char* message){
/*     const char* s0; */
     const char* s=(char*)source;
     const uint16_t *r=results;
     UErrorCode errorCode=U_ZERO_ERROR;
     uint32_t c,exC;
     ucnv_reset(cnv);
     while(s<limit) {
	 /* s0=s; */
        c=ucnv_getNextUChar(cnv, &s, limit, &errorCode);
        if(errorCode==U_INDEX_OUTOFBOUNDS_ERROR) {
            break; /* no more significant input */
        } else if(U_FAILURE(errorCode)) {
            log_err("%s ucnv_getNextUChar() failed: %s\n", message, u_errorName(errorCode));
            break;
        } else {
            if(UTF_IS_FIRST_SURROGATE(*r)){
                int i =0, len = 2;
                UTF_NEXT_CHAR_SAFE(r, i, len, exC, FALSE);
                r++;
            }else{
                exC = *r;
            }
            if(c!=(uint32_t)(exC))
                log_err("%s ucnv_getNextUChar() Expected:  \\u%04X Got:  \\u%04X \n",message,(uint32_t) (*r),c);
        }
        r++;
    }
}

static int TestJitterbug930(const char* enc){
    UErrorCode err = U_ZERO_ERROR;
    UConverter*converter;
    char out[80];
    char*target = out;
    UChar in[4];
    const UChar*source = in;
    int32_t off[80];
    int32_t* offsets = off;
    int numOffWritten=0;
    UBool flush = 0;
    converter = my_ucnv_open(enc, &err);

    in[0] = 0x41;     /* 0x4E00;*/
    in[1] = 0x4E01;
    in[2] = 0x4E02;
    in[3] = 0x4E03;

    memset(off, '*', sizeof(off));

    ucnv_fromUnicode (converter,
            &target,
            target+2,
            &source,
            source+3,
            offsets,
            flush,
            &err);

        /* writes three bytes into the output buffer: 41 1B 24
        * but offsets contains 0 1 1
    */
    while(*offsets< off[10]){
        numOffWritten++;
        offsets++;
    }
    log_verbose("Testing Jitterbug 930 for encoding %s",enc);
    if(numOffWritten!= (int)(target-out)){
        log_err("Jitterbug 930 test for enc: %s failed. Expected: %i Got: %i",enc, (int)(target-out),numOffWritten);
    }

    err = U_ZERO_ERROR;

    memset(off,'*' , sizeof(off));

    flush = 1;
    offsets=off;
    ucnv_fromUnicode (converter,
            &target,
            target+4,
            &source,
            source,
            offsets,
            flush,
            &err);
    numOffWritten=0;
    while(*offsets< off[10]){
        numOffWritten++;
        if(*offsets!= -1){
            log_err("Jitterbug 930 test for enc: %s failed. Expected: %i Got: %i",enc,-1,*offsets) ;
        }
        offsets++;
    }

    /* writes 42 43 7A into output buffer,
     * offsets contains -1 -1 -1
     */
    ucnv_close(converter);
    return 0;
}

static void
TestHZ() {
    /* test input */
    static const uint16_t in[]={
            0x3000, 0x3001, 0x3002, 0x00B7, 0x02C9, 0x02C7, 0x00A8, 0x3003, 0x3005, 0x2014,
            0xFF5E, 0x2016, 0x2026, 0x007E, 0x997C, 0x70B3, 0x75C5, 0x5E76, 0x73BB, 0x83E0,
            0x64AD, 0x62E8, 0x94B5, 0x000A, 0x6CE2, 0x535A, 0x52C3, 0x640F, 0x94C2, 0x7B94,
            0x4F2F, 0x5E1B, 0x8236, 0x000A, 0x8116, 0x818A, 0x6E24, 0x6CCA, 0x9A73, 0x6355,
            0x535C, 0x54FA, 0x8865, 0x000A, 0x57E0, 0x4E0D, 0x5E03, 0x6B65, 0x7C3F, 0x90E8,
            0x6016, 0x248F, 0x2490, 0x000A, 0x2491, 0x2492, 0x2493, 0x2494, 0x2495, 0x2496,
            0x2497, 0x2498, 0x2499, 0x000A, 0x249A, 0x249B, 0x2474, 0x2475, 0x2476, 0x2477,
            0x2478, 0x2479, 0x247A, 0x000A, 0x247B, 0x247C, 0x247D, 0x247E, 0x247F, 0x2480,
            0x2481, 0x2482, 0x2483, 0x000A, 0x0041, 0x0043, 0x0044, 0x0045, 0x0046, 0x007E,
            0x0048, 0x0049, 0x004A, 0x000A, 0x004B, 0x004C, 0x004D, 0x004E, 0x004F, 0x0050,
            0x0051, 0x0052, 0x0053, 0x000A, 0x0054, 0x0055, 0x0056, 0x0057, 0x0058, 0x0059,
            0x005A, 0x005B, 0x005C, 0x000A
      };
    const UChar* uSource;
    const UChar* uSourceLimit;
    const char* cSource;
    const char* cSourceLimit;
    UChar *uTargetLimit =NULL;
    UChar *uTarget;
    char *cTarget;
    const char *cTargetLimit;
    char *cBuf;
    UChar *uBuf,*test;
    int32_t uBufSize = 120;
    UErrorCode errorCode=U_ZERO_ERROR;
    UConverter *cnv;
    int32_t* offsets = (int32_t*) malloc(uBufSize * sizeof(int32_t) * 5);
    int32_t* myOff= offsets;
    cnv=ucnv_open("HZ", &errorCode);
    if(U_FAILURE(errorCode)) {
        log_data_err("Unable to open HZ converter: %s\n", u_errorName(errorCode));
        return;
    }

    uBuf =  (UChar*)malloc(uBufSize * sizeof(UChar)*5);
    cBuf =(char*)malloc(uBufSize * sizeof(char) * 5);
    uSource = (const UChar*)in;
    uSourceLimit=(const UChar*)in + (sizeof(in)/sizeof(in[0]));
    cTarget = cBuf;
    cTargetLimit = cBuf +uBufSize*5;
    uTarget = uBuf;
    uTargetLimit = uBuf+ uBufSize*5;
    ucnv_fromUnicode( cnv , &cTarget, cTargetLimit,&uSource,uSourceLimit,myOff,TRUE, &errorCode);
    if(U_FAILURE(errorCode)){
        log_err("ucnv_fromUnicode conversion failed reason %s\n", u_errorName(errorCode));
        return;
    }
    cSource = cBuf;
    cSourceLimit =cTarget;
    test =uBuf;
    myOff=offsets;
    ucnv_toUnicode(cnv,&uTarget,uTargetLimit,&cSource,cSourceLimit,myOff,TRUE,&errorCode);
    if(U_FAILURE(errorCode)){
        log_err("ucnv_toUnicode conversion failed reason %s\n", u_errorName(errorCode));
        return;
    }
    uSource = (const UChar*)in;
    while(uSource<uSourceLimit){
        if(*test!=*uSource){

            log_err("Expected : \\u%04X \t Got: \\u%04X\n",*uSource,(int)*test) ;
        }
        uSource++;
        test++;
    }
    TestGetNextUChar2022(cnv, cBuf, cTarget, in, "HZ encoding");
    TestSmallTargetBuffer(in,(const UChar*)in + (sizeof(in)/sizeof(in[0])),cnv);
    TestSmallSourceBuffer(in,(const UChar*)in + (sizeof(in)/sizeof(in[0])),cnv);
    TestToAndFromUChars(in,(const UChar*)in + (sizeof(in)/sizeof(in[0])),cnv);
    TestJitterbug930("csISO2022JP");
    ucnv_close(cnv);
    free(offsets);
    free(uBuf);
    free(cBuf);
}

static void
TestISCII(){
        /* test input */
    static const uint16_t in[]={
        /* test full range of Devanagari */
        0x0901,0x0902,0x0903,0x0905,0x0906,0x0907,0x0908,0x0909,0x090A,
        0x090B,0x090E,0x090F,0x0910,0x090D,0x0912,0x0913,0x0914,0x0911,
        0x0915,0x0916,0x0917,0x0918,0x0919,0x091A,0x091B,0x091C,0x091D,
        0x091E,0x091F,0x0920,0x0921,0x0922,0x0923,0x0924,0x0925,0x0926,
        0x0927,0x0928,0x0929,0x092A,0x092B,0x092C,0x092D,0x092E,0x092F,
        0x095F,0x0930,0x0931,0x0932,0x0933,0x0934,0x0935,0x0936,0x0937,
        0x0938,0x0939,0x200D,0x093E,0x093F,0x0940,0x0941,0x0942,0x0943,
        0x0946,0x0947,0x0948,0x0945,0x094A,0x094B,0x094C,0x0949,0x094D,
        0x093d,0x0966,0x0967,0x0968,0x0969,0x096A,0x096B,0x096C,
        0x096D,0x096E,0x096F,
        /* test Soft halant*/
        0x0915,0x094d, 0x200D,
        /* test explicit halant */
        0x0915,0x094d, 0x200c,
        /* test double danda */
        0x965,
        /* test ASCII */
        0x1B, 0x24, 0x29, 0x47, 0x0E, 0x23, 0x21, 0x23, 0x22, 0x23,
        0x23, 0x23, 0x24, 0x23, 0x25, 0x23, 0x26, 0x23, 0x27, 0x23,
        0x28, 0x23, 0x29, 0x23, 0x2A, 0x23, 0x2B, 0x0F, 0x2F, 0x2A,
        /* tests from Lotus */
        0x0061,0x0915,0x000D,0x000A,0x0996,0x0043,
        0x0930,0x094D,0x200D,
        0x0901,0x000D,0x000A,0x0905,0x0985,0x0043,
        0x0915,0x0921,0x002B,0x095F,
        /* tamil range */
        0x0B86, 0xB87, 0xB88,
        /* telugu range */
        0x0C05, 0x0C02, 0x0C03,0x0c31,
        /* kannada range */
        0x0C85, 0xC82, 0x0C83,
        /* test Abbr sign and Anudatta */
        0x0970, 0x952,
       /* 0x0958,
        0x0959,
        0x095A,
        0x095B,
        0x095C,
        0x095D,
        0x095E,
        0x095F,*/
        0x0960 /* Vocallic RRI 0xAB, 0xE9*/,
        0x0944 /* Vowel Sign Vocallic RRI 0xDF, 0xE9 */,
        0x090C ,
        0x0962,
        0x0961 /* Vocallic LL 0xa6, 0xE9 */,
        0x0963 /* Vowel Sign Vocallic LL 0xdb, 0xE9, */,
        0x0950 /* OM Symbol 0xa1, 0xE9,*/,
        0x093D /* Avagraha  0xEA, 0xE9*/,
        0x0958,
        0x0959,
        0x095A,
        0x095B,
        0x095C,
        0x095D,
        0x095E,
        0x0020, 0x094D, 0x0930, 0x0000, 0x00A0
      };
    static const unsigned char byteArr[]={

        0xa1,0xa2,0xa3,0xa4,0xa5,0xa6,0xa7,0xa8,0xa9,
        0xaa,0xab,0xac,0xad,0xae,0xaf,0xb0,0xb1,0xb2,
        0xb3,0xb4,0xb5,0xb6,0xb7,0xb8,0xb9,0xba,0xbb,
        0xbc,0xbd,0xbe,0xbf,0xc0,0xc1,0xc2,0xc3,0xc4,
        0xc5,0xc6,0xc7,0xc8,0xc9,0xca,0xcb,0xcc,0xcd,
        0xce,0xcf,0xd0,0xd1,0xd2,0xd3,0xd4,0xd5,0xd6,
        0xd7,0xd8,0xd9,0xda,0xdb,0xdc,0xdd,0xde,0xdf,
        0xe0,0xe1,0xe2,0xe3,0xe4,0xe5,0xe6,0xe7,0xe8,
        0xea,0xe9,0xf1,0xf2,0xf3,0xf4,0xf5,0xf6,0xf7,
        0xf8,0xf9,0xfa,
        /* test soft halant */
        0xb3, 0xE8, 0xE9,
        /* test explicit halant */
        0xb3, 0xE8, 0xE8,
        /* test double danda */
        0xea, 0xea,
        /* test ASCII */
        0x1B, 0x24, 0x29, 0x47, 0x0E, 0x23, 0x21, 0x23, 0x22, 0x23,
        0x23, 0x23, 0x24, 0x23, 0x25, 0x23, 0x26, 0x23, 0x27, 0x23,
        0x28, 0x23, 0x29, 0x23, 0x2A, 0x23, 0x2B, 0x0F, 0x2F, 0x2A,
        /* test ATR code */

        /* tests from Lotus */
        0x61,0xEF,0x42,0xEF,0x30,0xB3,0x0D,0x0A,0xEF,0x43,0xB4,0x43,
        0xEF,0x42,0xCF,0xE8,0xD9,
        0xEF,0x42,0xA1,0x0D,0x0A,0xEF,0x42,0xA4,0xEF,0x43,0xA4,0x43,
        0xEF,0x42,0xB3,0xBF,0x2B,0xEF,0x42,0xCE,
        /* tamil range */
        0xEF, 0x44, 0xa5, 0xa6, 0xa7,
        /* telugu range */
        0xEF, 0x45,0xa4, 0xa2, 0xa3,0xd0,
        /* kannada range */
        0xEF, 0x48,0xa4, 0xa2, 0xa3,
        /* anudatta and abbreviation sign */
        0xEF, 0x42, 0xF0, 0xBF, 0xF0, 0xB8,


        0xAA, 0xE9,/* RI + NUKTA 0x0960*/

        0xDF, 0xE9,/* Vowel sign RI + NUKTA 0x0944*/

        0xa6, 0xE9,/* Vowel I + NUKTA 0x090C*/

        0xdb, 0xE9,/* Vowel sign I + Nukta 0x0962*/

        0xa7, 0xE9,/* Vowel II + NUKTA 0x0961*/

        0xdc, 0xE9,/* Vowel sign II + Nukta 0x0963*/

        0xa1, 0xE9,/* chandrabindu + Nukta 0x0950*/

        0xEA, 0xE9, /* Danda + Nukta 0x093D*/

        0xB3, 0xE9, /* Ka + NUKTA */

        0xB4, 0xE9, /* Kha + NUKTA */

        0xB5, 0xE9, /* Ga + NUKTA */

        0xBA, 0xE9,

        0xBF, 0xE9,

        0xC0, 0xE9,

        0xC9, 0xE9,
        /* INV halant RA    */
        0xD9, 0xE8, 0xCF,
        0x00, 0x00A0, 
        /* just consume unhandled codepoints */
        0xEF, 0x30,

    };
    testConvertToU(byteArr,(sizeof(byteArr)),in,(sizeof(in)/U_SIZEOF_UCHAR),"x-iscii-de",NULL,TRUE);
    TestConv(in,(sizeof(in)/2),"ISCII,version=0","hindi", (char *)byteArr,sizeof(byteArr));    

}

static void
TestISO_2022_JP() {
    /* test input */
    static const uint16_t in[]={
        0x0041,/*0x00E9,*/0x3000, 0x3001, 0x3002, 0x0020, 0x000D, 0x000A,
        0x0043, 0x0044, 0x0045, 0x0046, 0x0047, 0x0048, 0x0049, 0x004A, 0x000D, 0x000A,
        0x004B, 0x004C, 0x004D, 0x004E, 0x004F, 0x0050, 0x0051, 0x0052, 0x000D, 0x000A,
        0x3005, 0x3006, 0x3007, 0x30FC, 0x2015, 0x2010, 0xFF0F, 0x005C, 0x000D, 0x000A,
        0x3013, 0x2018, 0x2026, 0x2025, 0x2018, 0x2019, 0x201C, 0x000D, 0x000A,
        0x201D, 0x3014, 0x000D, 0x000A,
        0x0053, 0x0054, 0x0055, 0x0056, 0x0057, 0x0058, 0x0059, 0x005A, 0x000D, 0x000A,
        0x0053, 0x0054, 0x0055, 0x0056, 0x0057, 0x0058, 0x0059, 0x005A, 0x000D, 0x000A,
        };
    const UChar* uSource;
    const UChar* uSourceLimit;
    const char* cSource;
    const char* cSourceLimit;
    UChar *uTargetLimit =NULL;
    UChar *uTarget;
    char *cTarget;
    const char *cTargetLimit;
    char *cBuf;
    UChar *uBuf,*test;
    int32_t uBufSize = 120;
    UErrorCode errorCode=U_ZERO_ERROR;
    UConverter *cnv;
    int32_t* offsets = (int32_t*) malloc(uBufSize * sizeof(int32_t) * 5);
    int32_t* myOff= offsets;
    cnv=ucnv_open("ISO_2022_JP_1", &errorCode);
    if(U_FAILURE(errorCode)) {
        log_data_err("Unable to open an ISO_2022_JP_1 converter: %s\n", u_errorName(errorCode));
        return;
    }

    uBuf =  (UChar*)malloc(uBufSize * sizeof(UChar)*5);
    cBuf =(char*)malloc(uBufSize * sizeof(char) * 5);
    uSource = (const UChar*)in;
    uSourceLimit=(const UChar*)in + (sizeof(in)/sizeof(in[0]));
    cTarget = cBuf;
    cTargetLimit = cBuf +uBufSize*5;
    uTarget = uBuf;
    uTargetLimit = uBuf+ uBufSize*5;
    ucnv_fromUnicode( cnv , &cTarget, cTargetLimit,&uSource,uSourceLimit,myOff,TRUE, &errorCode);
    if(U_FAILURE(errorCode)){
        log_err("ucnv_fromUnicode conversion failed reason %s\n", u_errorName(errorCode));
        return;
    }
    cSource = cBuf;
    cSourceLimit =cTarget;
    test =uBuf;
    myOff=offsets;
    ucnv_toUnicode(cnv,&uTarget,uTargetLimit,&cSource,cSourceLimit,myOff,TRUE,&errorCode);
    if(U_FAILURE(errorCode)){
        log_err("ucnv_toUnicode conversion failed reason %s\n", u_errorName(errorCode));
        return;
    }

    uSource = (const UChar*)in;
    while(uSource<uSourceLimit){
        if(*test!=*uSource){

            log_err("Expected : \\u%04X \t Got: \\u%04X\n",*uSource,(int)*test) ;
        }
        uSource++;
        test++;
    }

    TestSmallTargetBuffer(in,(const UChar*)in + (sizeof(in)/sizeof(in[0])),cnv);
    TestSmallSourceBuffer(in,(const UChar*)in + (sizeof(in)/sizeof(in[0])),cnv);
    TestGetNextUChar2022(cnv, cBuf, cTarget, in, "ISO-2022-JP encoding");
    TestToAndFromUChars(in,(const UChar*)in + (sizeof(in)/sizeof(in[0])),cnv);
    TestJitterbug930("csISO2022JP");
    ucnv_close(cnv);
    free(uBuf);
    free(cBuf);
    free(offsets);
}

static void TestConv(const uint16_t in[],int len, const char* conv, const char* lang, char byteArr[],int byteArrLen){
    const UChar* uSource;
    const UChar* uSourceLimit;
    const char* cSource;
    const char* cSourceLimit;
    UChar *uTargetLimit =NULL;
    UChar *uTarget;
    char *cTarget;
    const char *cTargetLimit;
    char *cBuf;
    UChar *uBuf,*test;
    int32_t uBufSize = 120*10;
    UErrorCode errorCode=U_ZERO_ERROR;
    UConverter *cnv;
    int32_t* offsets = (int32_t*) malloc(uBufSize * sizeof(int32_t) );
    int32_t* myOff= offsets;
    cnv=my_ucnv_open(conv, &errorCode);
    if(U_FAILURE(errorCode)) {
        log_data_err("Unable to open a %s converter: %s\n", conv, u_errorName(errorCode));
        return;
    }

    uBuf =  (UChar*)malloc(uBufSize * sizeof(UChar));
    cBuf =(char*)malloc(uBufSize * sizeof(char));
    uSource = (const UChar*)in;
    uSourceLimit=uSource+len;
    cTarget = cBuf;
    cTargetLimit = cBuf +uBufSize;
    uTarget = uBuf;
    uTargetLimit = uBuf+ uBufSize;
    ucnv_fromUnicode( cnv , &cTarget, cTargetLimit,&uSource,uSourceLimit,myOff,TRUE, &errorCode);
    if(U_FAILURE(errorCode)){
        log_err("ucnv_fromUnicode conversion failed reason %s\n", u_errorName(errorCode));
        return;
    }
    /*log_verbose("length of compressed string for language %s using %s:%i \n",conv,lang,(cTarget-cBuf));*/
    cSource = cBuf;
    cSourceLimit =cTarget;
    test =uBuf;
    myOff=offsets;
    ucnv_toUnicode(cnv,&uTarget,uTargetLimit,&cSource,cSourceLimit,myOff,TRUE,&errorCode);
    if(U_FAILURE(errorCode)){
        log_err("ucnv_toUnicode conversion failed, reason: %s\n", u_errorName(errorCode));
        return;
    }

    uSource = (const UChar*)in;
    while(uSource<uSourceLimit){
        if(*test!=*uSource){
            log_err("for codepage %s : Expected : \\u%04X \t Got: \\u%04X\n",conv,*uSource,(int)*test) ;
        }
        uSource++;
        test++;
    }
    TestSmallTargetBuffer(in,(const UChar*)&in[len],cnv);
    TestSmallSourceBuffer(in,(const UChar*)&in[len],cnv);
    TestGetNextUChar2022(cnv, cBuf, cTarget, in, conv);
    if(byteArr && byteArrLen!=0){
        TestGetNextUChar2022(cnv, byteArr, (byteArr+byteArrLen), in, lang);
        TestToAndFromUChars(in,(const UChar*)&in[len],cnv);
        {
            cSource = byteArr;
            cSourceLimit = cSource+byteArrLen;
            test=uBuf;
            myOff = offsets;
            ucnv_toUnicode(cnv,&uTarget,uTargetLimit,&cSource,cSourceLimit,myOff,TRUE,&errorCode);
            if(U_FAILURE(errorCode)){
                log_err("ucnv_toUnicode conversion failed reason %s\n", u_errorName(errorCode));
                return;
            }

            uSource = (const UChar*)in;
            while(uSource<uSourceLimit){
                if(*test!=*uSource){
                    log_err("Expected : \\u%04X \t Got: \\u%04X\n",*uSource,(int)*test) ;
                }
                uSource++;
                test++;
            }
        }
    }

    ucnv_close(cnv);
    free(uBuf);
    free(cBuf);
    free(offsets);
}
static UChar U_CALLCONV
_charAt(int32_t offset, void *context) {
    return ((char*)context)[offset];
}

static int32_t
unescape(UChar* dst, int32_t dstLen,const char* src,int32_t srcLen,UErrorCode *status){
    int32_t srcIndex=0;
    int32_t dstIndex=0;
    if(U_FAILURE(*status)){
        return 0;
    }
    if((dst==NULL && dstLen>0) || (src==NULL ) || dstLen < -1 || srcLen <-1 ){
        *status = U_ILLEGAL_ARGUMENT_ERROR;
        return 0;
    }
    if(srcLen==-1){
        srcLen = (int32_t)uprv_strlen(src);
    }

    for (; srcIndex<srcLen; ) {
        UChar32 c = src[srcIndex++];
        if (c == 0x005C /*'\\'*/) {
            c = u_unescapeAt(_charAt,&srcIndex,srcLen,(void*)src); /* advances i*/
            if (c == (UChar32)0xFFFFFFFF) {
                *status=U_INVALID_CHAR_FOUND; /* return empty string */
                break; /* invalid escape sequence */
            }
        }
        if(dstIndex < dstLen){
            if(c>0xFFFF){
               dst[dstIndex++] = UTF16_LEAD(c);
               if(dstIndex<dstLen){
                    dst[dstIndex]=UTF16_TRAIL(c);
               }else{
                   *status=U_BUFFER_OVERFLOW_ERROR;
               }
            }else{
                dst[dstIndex]=(UChar)c;
            }

        }else{
            *status = U_BUFFER_OVERFLOW_ERROR;
        }
        dstIndex++; /* for preflighting */
    }
    return dstIndex;
}

static void
TestFullRoundtrip(const char* cp){
    UChar usource[10] ={0};
    UChar nsrc[10] = {0};
    uint32_t i=1;
    int len=0, ulen;
    nsrc[0]=0x0061;
    /* Test codepoint 0 */
    TestConv(usource,1,cp,"",NULL,0);
    TestConv(usource,2,cp,"",NULL,0);
    nsrc[2]=0x5555;
    TestConv(nsrc,3,cp,"",NULL,0);

    for(;i<=0x10FFFF;i++){
        if(i==0xD800){
            i=0xDFFF;
            continue;
        }
        if(i<=0xFFFF){
            usource[0] =(UChar) i;
            len=1;
        }else{
            usource[0]=UTF16_LEAD(i);
            usource[1]=UTF16_TRAIL(i);
            len=2;
        }
        ulen=len;
        if(i==0x80) {
            usource[2]=0;
        }
        /* Test only single code points */
        TestConv(usource,ulen,cp,"",NULL,0);
        /* Test codepoint repeated twice */
        usource[ulen]=usource[0];
        usource[ulen+1]=usource[1];
        ulen+=len;
        TestConv(usource,ulen,cp,"",NULL,0);
        /* Test codepoint repeated 3 times */
        usource[ulen]=usource[0];
        usource[ulen+1]=usource[1];
        ulen+=len;
        TestConv(usource,ulen,cp,"",NULL,0);
        /* Test codepoint in between 2 codepoints */
        nsrc[1]=usource[0];
        nsrc[2]=usource[1];
        nsrc[len+1]=0x5555;
        TestConv(nsrc,len+2,cp,"",NULL,0);
        uprv_memset(usource,0,sizeof(UChar)*10);
    }
}

static void
TestRoundTrippingAllUTF(void){
    if(!getTestOption(QUICK_OPTION)){
        log_verbose("Running exhaustive round trip test for BOCU-1\n");
        TestFullRoundtrip("BOCU-1");
        log_verbose("Running exhaustive round trip test for SCSU\n");
        TestFullRoundtrip("SCSU");
        log_verbose("Running exhaustive round trip test for UTF-8\n");
        TestFullRoundtrip("UTF-8");
        log_verbose("Running exhaustive round trip test for CESU-8\n");
        TestFullRoundtrip("CESU-8");
        log_verbose("Running exhaustive round trip test for UTF-16BE\n");
        TestFullRoundtrip("UTF-16BE");
        log_verbose("Running exhaustive round trip test for UTF-16LE\n");
        TestFullRoundtrip("UTF-16LE");
        log_verbose("Running exhaustive round trip test for UTF-16\n");
        TestFullRoundtrip("UTF-16");
        log_verbose("Running exhaustive round trip test for UTF-32BE\n");
        TestFullRoundtrip("UTF-32BE");
        log_verbose("Running exhaustive round trip test for UTF-32LE\n");
        TestFullRoundtrip("UTF-32LE");
        log_verbose("Running exhaustive round trip test for UTF-32\n");
        TestFullRoundtrip("UTF-32");
        log_verbose("Running exhaustive round trip test for UTF-7\n");
        TestFullRoundtrip("UTF-7");
        log_verbose("Running exhaustive round trip test for UTF-7\n");
        TestFullRoundtrip("UTF-7,version=1");
        log_verbose("Running exhaustive round trip test for IMAP-mailbox-name\n");
        TestFullRoundtrip("IMAP-mailbox-name");
        log_verbose("Running exhaustive round trip test for GB18030\n");
        TestFullRoundtrip("GB18030");
    }
}

static void
TestSCSU() {

    static const uint16_t germanUTF16[]={
        0x00d6, 0x006c, 0x0020, 0x0066, 0x006c, 0x0069, 0x0065, 0x00df, 0x0074
    };

    static const uint8_t germanSCSU[]={
        0xd6, 0x6c, 0x20, 0x66, 0x6c, 0x69, 0x65, 0xdf, 0x74
    };

    static const uint16_t russianUTF16[]={
        0x041c, 0x043e, 0x0441, 0x043a, 0x0432, 0x0430
    };

    static const uint8_t russianSCSU[]={
        0x12, 0x9c, 0xbe, 0xc1, 0xba, 0xb2, 0xb0
    };

    static const uint16_t japaneseUTF16[]={
        0x3000, 0x266a, 0x30ea, 0x30f3, 0x30b4, 0x53ef, 0x611b,
        0x3044, 0x3084, 0x53ef, 0x611b, 0x3044, 0x3084, 0x30ea, 0x30f3,
        0x30b4, 0x3002, 0x534a, 0x4e16, 0x7d00, 0x3082, 0x524d, 0x306b,
        0x6d41, 0x884c, 0x3057, 0x305f, 0x300c, 0x30ea, 0x30f3, 0x30b4,
        0x306e, 0x6b4c, 0x300d, 0x304c, 0x3074, 0x3063, 0x305f, 0x308a,
        0x3059, 0x308b, 0x304b, 0x3082, 0x3057, 0x308c, 0x306a, 0x3044,
        0x3002, 0x7c73, 0x30a2, 0x30c3, 0x30d7, 0x30eb, 0x30b3, 0x30f3,
        0x30d4, 0x30e5, 0x30fc, 0x30bf, 0x793e, 0x306e, 0x30d1, 0x30bd,
        0x30b3, 0x30f3, 0x300c, 0x30de, 0x30c3, 0x30af, 0xff08, 0x30de,
        0x30c3, 0x30ad, 0x30f3, 0x30c8, 0x30c3, 0x30b7, 0x30e5, 0xff09,
        0x300d, 0x3092, 0x3001, 0x3053, 0x3088, 0x306a, 0x304f, 0x611b,
        0x3059, 0x308b, 0x4eba, 0x305f, 0x3061, 0x306e, 0x3053, 0x3068,
        0x3060, 0x3002, 0x300c, 0x30a2, 0x30c3, 0x30d7, 0x30eb, 0x4fe1,
        0x8005, 0x300d, 0x306a, 0x3093, 0x3066, 0x8a00, 0x3044, 0x65b9,
        0x307e, 0x3067, 0x3042, 0x308b, 0x3002
    };

    /* SCSUEncoder produces a slightly longer result (179B vs. 178B) because of one different choice:
     it uses an SQn once where a longer look-ahead could have shown that SCn is more efficient */
    static const uint8_t japaneseSCSU[]={
        0x08, 0x00, 0x1b, 0x4c, 0xea, 0x16, 0xca, 0xd3, 0x94, 0x0f, 0x53, 0xef, 0x61, 0x1b, 0xe5, 0x84,
        0xc4, 0x0f, 0x53, 0xef, 0x61, 0x1b, 0xe5, 0x84, 0xc4, 0x16, 0xca, 0xd3, 0x94, 0x08, 0x02, 0x0f,
        0x53, 0x4a, 0x4e, 0x16, 0x7d, 0x00, 0x30, 0x82, 0x52, 0x4d, 0x30, 0x6b, 0x6d, 0x41, 0x88, 0x4c,
        0xe5, 0x97, 0x9f, 0x08, 0x0c, 0x16, 0xca, 0xd3, 0x94, 0x15, 0xae, 0x0e, 0x6b, 0x4c, 0x08, 0x0d,
        0x8c, 0xb4, 0xa3, 0x9f, 0xca, 0x99, 0xcb, 0x8b, 0xc2, 0x97, 0xcc, 0xaa, 0x84, 0x08, 0x02, 0x0e,
        0x7c, 0x73, 0xe2, 0x16, 0xa3, 0xb7, 0xcb, 0x93, 0xd3, 0xb4, 0xc5, 0xdc, 0x9f, 0x0e, 0x79, 0x3e,
        0x06, 0xae, 0xb1, 0x9d, 0x93, 0xd3, 0x08, 0x0c, 0xbe, 0xa3, 0x8f, 0x08, 0x88, 0xbe, 0xa3, 0x8d,
        0xd3, 0xa8, 0xa3, 0x97, 0xc5, 0x17, 0x89, 0x08, 0x0d, 0x15, 0xd2, 0x08, 0x01, 0x93, 0xc8, 0xaa,
        0x8f, 0x0e, 0x61, 0x1b, 0x99, 0xcb, 0x0e, 0x4e, 0xba, 0x9f, 0xa1, 0xae, 0x93, 0xa8, 0xa0, 0x08,
        0x02, 0x08, 0x0c, 0xe2, 0x16, 0xa3, 0xb7, 0xcb, 0x0f, 0x4f, 0xe1, 0x80, 0x05, 0xec, 0x60, 0x8d,
        0xea, 0x06, 0xd3, 0xe6, 0x0f, 0x8a, 0x00, 0x30, 0x44, 0x65, 0xb9, 0xe4, 0xfe, 0xe7, 0xc2, 0x06,
        0xcb, 0x82
    };

    static const uint16_t allFeaturesUTF16[]={
        0x0041, 0x00df, 0x0401, 0x015f, 0x00df, 0x01df, 0xf000, 0xdbff,
        0xdfff, 0x000d, 0x000a, 0x0041, 0x00df, 0x0401, 0x015f, 0x00df,
        0x01df, 0xf000, 0xdbff, 0xdfff
    };

    /* see comment at japaneseSCSU: the same kind of different choice yields a slightly shorter
     * result here (34B vs. 35B)
     */
    static const uint8_t allFeaturesSCSU[]={
        0x41, 0xdf, 0x12, 0x81, 0x03, 0x5f, 0x10, 0xdf, 0x1b, 0x03,
        0xdf, 0x1c, 0x88, 0x80, 0x0b, 0xbf, 0xff, 0xff, 0x0d, 0x0a,
        0x41, 0x10, 0xdf, 0x12, 0x81, 0x03, 0x5f, 0x10, 0xdf, 0x13,
        0xdf, 0x14, 0x80, 0x15, 0xff
    };
    static const uint16_t monkeyIn[]={
        0x00A8, 0x3003, 0x3005, 0x2015, 0xFF5E, 0x2016, 0x2026, 0x2018, 0x000D, 0x000A,
        0x2019, 0x201C, 0x201D, 0x3014, 0x3015, 0x3008, 0x3009, 0x300A, 0x000D, 0x000A,
        0x300B, 0x300C, 0x300D, 0x300E, 0x300F, 0x3016, 0x3017, 0x3010, 0x000D, 0x000A,
        0x3011, 0x00B1, 0x00D7, 0x00F7, 0x2236, 0x2227, 0x7FC1, 0x8956, 0x000D, 0x000A,
        0x9D2C, 0x9D0E, 0x9EC4, 0x5CA1, 0x6C96, 0x837B, 0x5104, 0x5C4B, 0x000D, 0x000A,
        0x61B6, 0x81C6, 0x6876, 0x7261, 0x4E59, 0x4FFA, 0x5378, 0x57F7, 0x000D, 0x000A,
        0x57F4, 0x57F9, 0x57FA, 0x57FC, 0x5800, 0x5802, 0x5805, 0x5806, 0x000D, 0x000A,
        0x580A, 0x581E, 0x6BB5, 0x6BB7, 0x6BBA, 0x6BBC, 0x9CE2, 0x977C, 0x000D, 0x000A,
        0x6BBF, 0x6BC1, 0x6BC5, 0x6BC6, 0x6BCB, 0x6BCD, 0x6BCF, 0x6BD2, 0x000D, 0x000A,
        0x6BD3, 0x6BD4, 0x6BD6, 0x6BD7, 0x6BD8, 0x6BDB, 0x6BEB, 0x6BEC, 0x000D, 0x000A,
        0x6C05, 0x6C08, 0x6C0F, 0x6C11, 0x6C13, 0x6C23, 0x6C34, 0x0041, 0x000D, 0x000A,
        0x0043, 0x0044, 0x0045, 0x0046, 0x0047, 0x0048, 0x0049, 0x004A, 0x000D, 0x000A,
        0x004B, 0x004C, 0x004D, 0x004E, 0x004F, 0x0050, 0x0051, 0x0052, 0x000D, 0x000A,
        0x0053, 0x0054, 0x0055, 0x0056, 0x0057, 0x0058, 0x0059, 0x005A, 0x000D, 0x000A,
        0x005B, 0x9792, 0x9CCC, 0x9CCD, 0x9CCE, 0x9CCF, 0x9CD0, 0x9CD3, 0x000D, 0x000A,
        0x9CD4, 0x9CD5, 0x9CD7, 0x9CD8, 0x9CD9, 0x9CDC, 0x9CDD, 0x9CDF, 0x000D, 0x000A,
        0x9785, 0x9791, 0x00BD, 0x0390, 0x0385, 0x0386, 0x0388, 0x0389, 0x000D, 0x000A,
        0x038E, 0x038F, 0x0390, 0x0391, 0x0392, 0x0393, 0x0394, 0x0395, 0x000D, 0x000A,
        0x0396, 0x0397, 0x0398, 0x0399, 0x039A, 0x038A, 0x038C, 0x039C, 0x000D, 0x000A,
        /* test non-BMP code points */
        0xD869, 0xDE99, 0xD869, 0xDE9C, 0xD869, 0xDE9D, 0xD869, 0xDE9E, 0xD869, 0xDE9F,
        0xD869, 0xDEA0, 0xD869, 0xDEA5, 0xD869, 0xDEA6, 0xD869, 0xDEA7, 0xD869, 0xDEA8,
        0xD869, 0xDEAB, 0xD869, 0xDEAC, 0xD869, 0xDEAD, 0xD869, 0xDEAE, 0xD869, 0xDEAF,
        0xD869, 0xDEB0, 0xD869, 0xDEB1, 0xD869, 0xDEB3, 0xD869, 0xDEB5, 0xD869, 0xDEB6,
        0xD869, 0xDEB7, 0xD869, 0xDEB8, 0xD869, 0xDEB9, 0xD869, 0xDEBA, 0xD869, 0xDEBB,
        0xD869, 0xDEBC, 0xD869, 0xDEBD, 0xD869, 0xDEBE, 0xD869, 0xDEBF, 0xD869, 0xDEC0,
        0xD869, 0xDEC1, 0xD869, 0xDEC2, 0xD869, 0xDEC3, 0xD869, 0xDEC4, 0xD869, 0xDEC8,
        0xD869, 0xDECA, 0xD869, 0xDECB, 0xD869, 0xDECD, 0xD869, 0xDECE, 0xD869, 0xDECF,
        0xD869, 0xDED0, 0xD869, 0xDED1, 0xD869, 0xDED2, 0xD869, 0xDED3, 0xD869, 0xDED4,
        0xD869, 0xDED5, 0xD800, 0xDC00, 0xD800, 0xDC00, 0xD800, 0xDC00, 0xDBFF, 0xDFFF,
        0xDBFF, 0xDFFF, 0xDBFF, 0xDFFF,


        0x4DB3, 0x4DB4, 0x4DB5, 0x4E00, 0x4E00, 0x4E01, 0x4E02, 0x4E03, 0x000D, 0x000A,
        0x0392, 0x0393, 0x0394, 0x0395, 0x0396, 0x0397, 0x33E0, 0x33E6, 0x000D, 0x000A,
        0x4E05, 0x4E07, 0x4E04, 0x4E08, 0x4E08, 0x4E09, 0x4E0A, 0x4E0B, 0x000D, 0x000A,
        0x4E0C, 0x0021, 0x0022, 0x0023, 0x0024, 0xFF40, 0xFF41, 0xFF42, 0x000D, 0x000A,
        0xFF43, 0xFF44, 0xFF45, 0xFF46, 0xFF47, 0xFF48, 0xFF49, 0xFF4A, 0x000D, 0x000A,
    };
    static const char *fTestCases [] = {
          "\\ud800\\udc00", /* smallest surrogate*/
          "\\ud8ff\\udcff",
          "\\udBff\\udFff", /* largest surrogate pair*/
          "\\ud834\\udc00",
          "\\U0010FFFF",
          "Hello \\u9292 \\u9192 World!",
          "Hell\\u0429o \\u9292 \\u9192 W\\u00e4rld!",
          "Hell\\u0429o \\u9292 \\u9292W\\u00e4rld!",

          "\\u0648\\u06c8", /* catch missing reset*/
          "\\u0648\\u06c8",

          "\\u4444\\uE001", /* lowest quotable*/
          "\\u4444\\uf2FF", /* highest quotable*/
          "\\u4444\\uf188\\u4444",
          "\\u4444\\uf188\\uf288",
          "\\u4444\\uf188abc\\u0429\\uf288",
          "\\u9292\\u2222",
          "Hell\\u0429\\u04230o \\u9292 \\u9292W\\u00e4\\u0192rld!",
          "Hell\\u0429o \\u9292 \\u9292W\\u00e4rld!",
          "Hello World!123456",
          "Hello W\\u0081\\u011f\\u0082!", /* Latin 1 run*/

          "abc\\u0301\\u0302",  /* uses SQn for u301 u302*/
          "abc\\u4411d",      /* uses SQU*/
          "abc\\u4411\\u4412d",/* uses SCU*/
          "abc\\u0401\\u0402\\u047f\\u00a5\\u0405", /* uses SQn for ua5*/
          "\\u9191\\u9191\\u3041\\u9191\\u3041\\u3041\\u3000", /* SJIS like data*/
          "\\u9292\\u2222",
          "\\u9191\\u9191\\u3041\\u9191\\u3041\\u3041\\u3000",
          "\\u9999\\u3051\\u300c\\u9999\\u9999\\u3060\\u9999\\u3065\\u3065\\u3065\\u300c",
          "\\u3000\\u266a\\u30ea\\u30f3\\u30b4\\u53ef\\u611b\\u3044\\u3084\\u53ef\\u611b\\u3044\\u3084\\u30ea\\u30f3\\u30b4\\u3002",

          "", /* empty input*/
          "\\u0000", /* smallest BMP character*/
          "\\uFFFF", /* largest BMP character*/

          /* regression tests*/
          "\\u6441\\ub413\\ua733\\uf8fe\\ueedb\\u587f\\u195f\\u4899\\uf23d\\u49fd\\u0aac\\u5792\\ufc22\\ufc3c\\ufc46\\u00aa",
          "\\u00df\\u01df\\uf000\\udbff\\udfff\\u000d\n\\u0041\\u00df\\u0401\\u015f\\u00df\\u01df\\uf000\\udbff\\udfff",
          "\\u30f9\\u8321\\u05e5\\u181c\\ud72b\\u2019\\u99c9\\u2f2f\\uc10c\\u82e1\\u2c4d\\u1ebc\\u6013\\u66dc\\ubbde\\u94a5\\u4726\\u74af\\u3083\\u55b9\\u000c",
          "\\u0041\\u00df\\u0401\\u015f",
          "\\u9066\\u2123abc",
          "\\ud266\\u43d7\\u\\ue386\\uc9c0\\u4a6b\\u9222\\u901f\\u7410\\ua63f\\u539b\\u9596\\u482e\\u9d47\\ucfe4\\u7b71\\uc280\\uf26a\\u982f\\u862a\\u4edd\\uf513\\ufda6\\u869d\\u2ee0\\ua216\\u3ff6\\u3c70\\u89c0\\u9576\\ud5ec\\ubfda\\u6cca\\u5bb3\\ubcea\\u554c\\u914e\\ufa4a\\uede3\\u2990\\ud2f5\\u2729\\u5141\\u0f26\\uccd8\\u5413\\ud196\\ubbe2\\u51b9\\u9b48\\u0dc8\\u2195\\u21a2\\u21e9\\u00e4\\u9d92\\u0bc0\\u06c5",
          "\\uf95b\\u2458\\u2468\\u0e20\\uf51b\\ue36e\\ubfc1\\u0080\\u02dd\\uf1b5\\u0cf3\\u6059\\u7489",
    };
    int i=0;
    for(;i<sizeof(fTestCases)/sizeof(*fTestCases);i++){
        const char* cSrc = fTestCases[i];
        UErrorCode status = U_ZERO_ERROR;
        int32_t cSrcLen,srcLen;
        UChar* src;
        /* UConverter* cnv = ucnv_open("SCSU",&status); */
        cSrcLen = srcLen = (int32_t)uprv_strlen(fTestCases[i]);
        src = (UChar*) malloc((sizeof(UChar) * srcLen) + sizeof(UChar));
        srcLen=unescape(src,srcLen,cSrc,cSrcLen,&status);
        log_verbose("Testing roundtrip for src: %s at index :%d\n",cSrc,i);
        TestConv(src,srcLen,"SCSU","Coverage",NULL,0);
        free(src);
    }
    TestConv(allFeaturesUTF16,(sizeof(allFeaturesUTF16)/2),"SCSU","all features", (char *)allFeaturesSCSU,sizeof(allFeaturesSCSU));
    TestConv(allFeaturesUTF16,(sizeof(allFeaturesUTF16)/2),"SCSU","all features",(char *)allFeaturesSCSU,sizeof(allFeaturesSCSU));
    TestConv(japaneseUTF16,(sizeof(japaneseUTF16)/2),"SCSU","japaneese",(char *)japaneseSCSU,sizeof(japaneseSCSU));
    TestConv(japaneseUTF16,(sizeof(japaneseUTF16)/2),"SCSU,locale=ja","japaneese",(char *)japaneseSCSU,sizeof(japaneseSCSU));
    TestConv(germanUTF16,(sizeof(germanUTF16)/2),"SCSU","german",(char *)germanSCSU,sizeof(germanSCSU));
    TestConv(russianUTF16,(sizeof(russianUTF16)/2), "SCSU","russian",(char *)russianSCSU,sizeof(russianSCSU));
    TestConv(monkeyIn,(sizeof(monkeyIn)/2),"SCSU","monkey",NULL,0);
}

#if !UCONFIG_NO_LEGACY_CONVERSION
static void TestJitterbug2346(){
    char source[] = { 0x1b,0x24,0x42,0x3d,0x45,0x1b,0x28,0x4a,0x0d,0x0a,
                      0x1b,0x24,0x42,0x3d,0x45,0x1b,0x28,0x4a,0x0d,0x0a};
    uint16_t expected[] = {0x91CD,0x000D,0x000A,0x91CD,0x000D,0x000A};    
    
    UChar uTarget[500]={'\0'};
    UChar* utarget=uTarget;
    UChar* utargetLimit=uTarget+sizeof(uTarget)/2;

    char cTarget[500]={'\0'};
    char* ctarget=cTarget;
    char* ctargetLimit=cTarget+sizeof(cTarget);
    const char* csource=source;
    UChar* temp = expected;
    UErrorCode err=U_ZERO_ERROR;

    UConverter* conv =ucnv_open("ISO_2022_JP",&err);
    if(U_FAILURE(err)) {
        log_data_err("Unable to open a iso-2022 converter: %s\n", u_errorName(err));
        return;
    }
    ucnv_toUnicode(conv,&utarget,utargetLimit,&csource,csource+sizeof(source),NULL,TRUE,&err);
    if(U_FAILURE(err)) {
        log_err("ISO_2022_JP to Unicode conversion failed: %s\n", u_errorName(err));
        return;
    }
    utargetLimit=utarget;
    utarget = uTarget;
    while(utarget<utargetLimit){
        if(*temp!=*utarget){

            log_err("Expected : \\u%04X \t Got: \\u%04X\n",*utarget,(int)*temp) ;
        }
        utarget++;
        temp++;
    }
    ucnv_fromUnicode(conv,&ctarget,ctargetLimit,(const UChar**)&utarget,utargetLimit,NULL,TRUE,&err);
    if(U_FAILURE(err)) {
        log_err("ISO_2022_JP from Unicode conversion failed: %s\n", u_errorName(err));
        return;
    }
    ctargetLimit=ctarget;
    ctarget =cTarget;
    ucnv_close(conv);


}

static void
TestISO_2022_JP_1() {
    /* test input */
    static const uint16_t in[]={
        0x3000, 0x3001, 0x3002, 0x0020, 0xFF0E, 0x30FB, 0xFF1A, 0xFF1B, 0x000D, 0x000A,
        0x0043, 0x0044, 0x0045, 0x0046, 0x0047, 0x0048, 0x0049, 0x004A, 0x000D, 0x000A,
        0x52C8, 0x52CC, 0x52CF, 0x52D1, 0x52D4, 0x52D6, 0x52DB, 0x52DC, 0x000D, 0x000A,
        0x004B, 0x004C, 0x004D, 0x004E, 0x004F, 0x0050, 0x0051, 0x0052, 0x000D, 0x000A,
        0x3005, 0x3006, 0x3007, 0x30FC, 0x2015, 0x2010, 0xFF0F, 0x005C, 0x000D, 0x000A,
        0x3013, 0x2018, 0x2026, 0x2025, 0x2018, 0x2019, 0x201C, 0x000D, 0x000A,
        0x201D, 0x000D, 0x000A,
        0x0053, 0x0054, 0x0055, 0x0056, 0x0057, 0x0058, 0x0059, 0x005A, 0x000D, 0x000A,
        0x4F94, 0x4F97, 0x52BA, 0x52BB, 0x52BD, 0x52C0, 0x52C4, 0x52C6, 0x000D, 0x000A,
        0x0053, 0x0054, 0x0055, 0x0056, 0x0057, 0x0058, 0x0059, 0x005A, 0x000D, 0x000A,
        0x4F78, 0x4F79, 0x4F7A, 0x4F7D, 0x4F7E, 0x4F81, 0x4F82, 0x4F84, 0x000D, 0x000A,
        0x4F85, 0x4F89, 0x4F8A, 0x4F8C, 0x4F8E, 0x4F90, 0x4F92, 0x4F93, 0x000D, 0x000A,
        0x52E1, 0x52E5, 0x52E8, 0x52E9, 0x000D, 0x000A
      };
    const UChar* uSource;
    const UChar* uSourceLimit;
    const char* cSource;
    const char* cSourceLimit;
    UChar *uTargetLimit =NULL;
    UChar *uTarget;
    char *cTarget;
    const char *cTargetLimit;
    char *cBuf;
    UChar *uBuf,*test;
    int32_t uBufSize = 120;
    UErrorCode errorCode=U_ZERO_ERROR;
    UConverter *cnv;

    cnv=ucnv_open("ISO_2022_JP_1", &errorCode);
    if(U_FAILURE(errorCode)) {
        log_data_err("Unable to open a iso-2022 converter: %s\n", u_errorName(errorCode));
        return;
    }

    uBuf =  (UChar*)malloc(uBufSize * sizeof(UChar)*5);
    cBuf =(char*)malloc(uBufSize * sizeof(char) * 5);
    uSource = (const UChar*)in;
    uSourceLimit=(const UChar*)in + (sizeof(in)/sizeof(in[0]));
    cTarget = cBuf;
    cTargetLimit = cBuf +uBufSize*5;
    uTarget = uBuf;
    uTargetLimit = uBuf+ uBufSize*5;
    ucnv_fromUnicode( cnv , &cTarget, cTargetLimit,&uSource,uSourceLimit,NULL,TRUE, &errorCode);
    if(U_FAILURE(errorCode)){
        log_err("ucnv_fromUnicode conversion failed reason %s\n", u_errorName(errorCode));
        return;
    }
    cSource = cBuf;
    cSourceLimit =cTarget;
    test =uBuf;
    ucnv_toUnicode(cnv,&uTarget,uTargetLimit,&cSource,cSourceLimit,NULL,TRUE,&errorCode);
    if(U_FAILURE(errorCode)){
        log_err("ucnv_toUnicode conversion failed reason %s\n", u_errorName(errorCode));
        return;
    }
    uSource = (const UChar*)in;
    while(uSource<uSourceLimit){
        if(*test!=*uSource){

            log_err("Expected : \\u%04X \t Got: \\u%04X\n",*uSource,(int)*test) ;
        }
        uSource++;
        test++;
    }
    /*ucnv_close(cnv);
    cnv=ucnv_open("ISO_2022,locale=jp,version=1", &errorCode);*/
    /*Test for the condition where there is an invalid character*/
    ucnv_reset(cnv);
    {
        static const uint8_t source2[]={0x0e,0x24,0x053};
        TestNextUCharError(cnv, (const char*)source2, (const char*)source2+sizeof(source2), U_ZERO_ERROR, "an invalid character [ISO-2022-JP-1]");
    }
    TestSmallTargetBuffer(in,(const UChar*)in + (sizeof(in)/sizeof(in[0])),cnv);
    TestSmallSourceBuffer(in,(const UChar*)in + (sizeof(in)/sizeof(in[0])),cnv);
    ucnv_close(cnv);
    free(uBuf);
    free(cBuf);
}

static void
TestISO_2022_JP_2() {
    /* test input */
    static const uint16_t in[]={
        0x00A8, 0x3003, 0x3005, 0x2015, 0xFF5E, 0x2016, 0x2026, 0x2018, 0x000D, 0x000A,
        0x2019, 0x201C, 0x201D, 0x3014, 0x3015, 0x3008, 0x3009, 0x300A, 0x000D, 0x000A,
        0x300B, 0x300C, 0x300D, 0x300E, 0x300F, 0x3016, 0x3017, 0x3010, 0x000D, 0x000A,
        0x3011, 0x00B1, 0x00D7, 0x00F7, 0x2236, 0x2227, 0x7FC1, 0x8956, 0x000D, 0x000A,
        0x9D2C, 0x9D0E, 0x9EC4, 0x5CA1, 0x6C96, 0x837B, 0x5104, 0x5C4B, 0x000D, 0x000A,
        0x61B6, 0x81C6, 0x6876, 0x7261, 0x4E59, 0x4FFA, 0x5378, 0x57F7, 0x000D, 0x000A,
        0x57F4, 0x57F9, 0x57FA, 0x57FC, 0x5800, 0x5802, 0x5805, 0x5806, 0x000D, 0x000A,
        0x580A, 0x581E, 0x6BB5, 0x6BB7, 0x6BBA, 0x6BBC, 0x9CE2, 0x977C, 0x000D, 0x000A,
        0x6BBF, 0x6BC1, 0x6BC5, 0x6BC6, 0x6BCB, 0x6BCD, 0x6BCF, 0x6BD2, 0x000D, 0x000A,
        0x6BD3, 0x6BD4, 0x6BD6, 0x6BD7, 0x6BD8, 0x6BDB, 0x6BEB, 0x6BEC, 0x000D, 0x000A,
        0x6C05, 0x6C08, 0x6C0F, 0x6C11, 0x6C13, 0x6C23, 0x6C34, 0x0041, 0x000D, 0x000A,
        0x0043, 0x0044, 0x0045, 0x0046, 0x0047, 0x0048, 0x0049, 0x004A, 0x000D, 0x000A,
        0x004B, 0x004C, 0x004D, 0x004E, 0x004F, 0x0050, 0x0051, 0x0052, 0x000D, 0x000A,
        0x0053, 0x0054, 0x0055, 0x0056, 0x0057, 0x0058, 0x0059, 0x005A, 0x000D, 0x000A,
        0x005B, 0x9792, 0x9CCC, 0x9CCD, 0x9CCE, 0x9CCF, 0x9CD0, 0x9CD3, 0x000D, 0x000A,
        0x9CD4, 0x9CD5, 0x9CD7, 0x9CD8, 0x9CD9, 0x9CDC, 0x9CDD, 0x9CDF, 0x000D, 0x000A,
        0x9785, 0x9791, 0x00BD, 0x0390, 0x0385, 0x0386, 0x0388, 0x0389, 0x000D, 0x000A,
        0x038E, 0x038F, 0x0390, 0x0391, 0x0392, 0x0393, 0x0394, 0x0395, 0x000D, 0x000A,
        0x0396, 0x0397, 0x0398, 0x0399, 0x039A, 0x038A, 0x038C, 0x039C, 0x000D, 0x000A
      };
    const UChar* uSource;
    const UChar* uSourceLimit;
    const char* cSource;
    const char* cSourceLimit;
    UChar *uTargetLimit =NULL;
    UChar *uTarget;
    char *cTarget;
    const char *cTargetLimit;
    char *cBuf;
    UChar *uBuf,*test;
    int32_t uBufSize = 120;
    UErrorCode errorCode=U_ZERO_ERROR;
    UConverter *cnv;
    int32_t* offsets = (int32_t*) malloc(uBufSize * sizeof(int32_t) * 5);
    int32_t* myOff= offsets;
    cnv=ucnv_open("ISO_2022_JP_2", &errorCode);
    if(U_FAILURE(errorCode)) {
        log_data_err("Unable to open a iso-2022 converter: %s\n", u_errorName(errorCode));
        return;
    }

    uBuf =  (UChar*)malloc(uBufSize * sizeof(UChar)*5);
    cBuf =(char*)malloc(uBufSize * sizeof(char) * 5);
    uSource = (const UChar*)in;
    uSourceLimit=(const UChar*)in + (sizeof(in)/sizeof(in[0]));
    cTarget = cBuf;
    cTargetLimit = cBuf +uBufSize*5;
    uTarget = uBuf;
    uTargetLimit = uBuf+ uBufSize*5;
    ucnv_fromUnicode( cnv , &cTarget, cTargetLimit,&uSource,uSourceLimit,myOff,TRUE, &errorCode);
    if(U_FAILURE(errorCode)){
        log_err("ucnv_fromUnicode conversion failed reason %s\n", u_errorName(errorCode));
        return;
    }
    cSource = cBuf;
    cSourceLimit =cTarget;
    test =uBuf;
    myOff=offsets;
    ucnv_toUnicode(cnv,&uTarget,uTargetLimit,&cSource,cSourceLimit,myOff,TRUE,&errorCode);
    if(U_FAILURE(errorCode)){
        log_err("ucnv_toUnicode conversion failed reason %s\n", u_errorName(errorCode));
        return;
    }
    uSource = (const UChar*)in;
    while(uSource<uSourceLimit){
        if(*test!=*uSource){

            log_err("Expected : \\u%04X \t Got: \\u%04X\n",*uSource,(int)*test) ;
        }
        uSource++;
        test++;
    }
    TestSmallTargetBuffer(in,(const UChar*)in + (sizeof(in)/sizeof(in[0])),cnv);
    TestSmallSourceBuffer(in,(const UChar*)in + (sizeof(in)/sizeof(in[0])),cnv);
    TestToAndFromUChars(in,(const UChar*)in + (sizeof(in)/sizeof(in[0])),cnv);
    /*Test for the condition where there is an invalid character*/
    ucnv_reset(cnv);
    {
        static const uint8_t source2[]={0x0e,0x24,0x053};
        TestNextUCharError(cnv, (const char*)source2, (const char*)source2+sizeof(source2), U_ZERO_ERROR, "an invalid character [ISO-2022-JP-2]");
    }
    ucnv_close(cnv);
    free(uBuf);
    free(cBuf);
    free(offsets);
}

static void
TestISO_2022_KR() {
    /* test input */
    static const uint16_t in[]={
                    0x9F4B,0x9F4E,0x9F52,0x9F5F,0x9F61,0x9F67,0x9F6A,0x000A,0x000D
                   ,0x9F6C,0x9F77,0x9F8D,0x9F90,0x9F95,0x9F9C,0xAC00,0xAC01,0xAC04
                   ,0xAC07,0xAC08,0xAC09,0x0025,0x0026,0x0027,0x000A,0x000D,0x0028,0x0029
                   ,0x002A,0x002B,0x002C,0x002D,0x002E,0x53C3,0x53C8,0x53C9,0x53CA,0x53CB
                   ,0x53CD,0x53D4,0x53D6,0x53D7,0x53DB,0x000A,0x000D,0x53E1,0x53E2
                   ,0x53E3,0x53E4,0x000A,0x000D};
    const UChar* uSource;
    const UChar* uSourceLimit;
    const char* cSource;
    const char* cSourceLimit;
    UChar *uTargetLimit =NULL;
    UChar *uTarget;
    char *cTarget;
    const char *cTargetLimit;
    char *cBuf;
    UChar *uBuf,*test;
    int32_t uBufSize = 120;
    UErrorCode errorCode=U_ZERO_ERROR;
    UConverter *cnv;
    int32_t* offsets = (int32_t*) malloc(uBufSize * sizeof(int32_t) * 5);
    int32_t* myOff= offsets;
    cnv=ucnv_open("ISO_2022,locale=kr", &errorCode);
    if(U_FAILURE(errorCode)) {
        log_data_err("Unable to open a iso-2022 converter: %s\n", u_errorName(errorCode));
        return;
    }

    uBuf =  (UChar*)malloc(uBufSize * sizeof(UChar)*5);
    cBuf =(char*)malloc(uBufSize * sizeof(char) * 5);
    uSource = (const UChar*)in;
    uSourceLimit=(const UChar*)in + (sizeof(in)/sizeof(in[0]));
    cTarget = cBuf;
    cTargetLimit = cBuf +uBufSize*5;
    uTarget = uBuf;
    uTargetLimit = uBuf+ uBufSize*5;
    ucnv_fromUnicode( cnv , &cTarget, cTargetLimit,&uSource,uSourceLimit,myOff,TRUE, &errorCode);
    if(U_FAILURE(errorCode)){
        log_err("ucnv_fromUnicode conversion failed reason %s\n", u_errorName(errorCode));
        return;
    }
    cSource = cBuf;
    cSourceLimit =cTarget;
    test =uBuf;
    myOff=offsets;
    ucnv_toUnicode(cnv,&uTarget,uTargetLimit,&cSource,cSourceLimit,myOff,TRUE,&errorCode);
    if(U_FAILURE(errorCode)){
        log_err("ucnv_toUnicode conversion failed reason %s\n", u_errorName(errorCode));
        return;
    }
    uSource = (const UChar*)in;
    while(uSource<uSourceLimit){
        if(*test!=*uSource){
            log_err("Expected : \\u%04X \t Got: \\u%04X\n",*uSource,*test) ;
        }
        uSource++;
        test++;
    }
    TestGetNextUChar2022(cnv, cBuf, cTarget, in, "ISO-2022-KR encoding");
    TestSmallTargetBuffer(in,(const UChar*)in + (sizeof(in)/sizeof(in[0])),cnv);
    TestSmallSourceBuffer(in,(const UChar*)in + (sizeof(in)/sizeof(in[0])),cnv);
    TestToAndFromUChars(in,(const UChar*)in + (sizeof(in)/sizeof(in[0])),cnv);
    TestJitterbug930("csISO2022KR");
    /*Test for the condition where there is an invalid character*/
    ucnv_reset(cnv);
    {
        static const uint8_t source2[]={0x1b,0x24,0x053};
        ucnv_setToUCallBack(cnv, UCNV_TO_U_CALLBACK_STOP, NULL, NULL, NULL, &errorCode);
        TestNextUCharError(cnv, (const char*)source2, (const char*)source2+sizeof(source2), U_ILLEGAL_ESCAPE_SEQUENCE, "an invalid character [ISO-2022-KR]");
    }
    ucnv_close(cnv);
    free(uBuf);
    free(cBuf);
    free(offsets);
}

static void
TestISO_2022_KR_1() {
    /* test input */
    static const uint16_t in[]={
                    0x9F4B,0x9F4E,0x9F52,0x9F5F,0x9F61,0x9F67,0x9F6A,0x000A,0x000D
                   ,0x9F6C,0x9F77,0x9F8D,0x9F90,0x9F95,0x9F9C,0xAC00,0xAC01,0xAC04
                   ,0xAC07,0xAC08,0xAC09,0x0025,0x0026,0x0027,0x000A,0x000D,0x0028,0x0029
                   ,0x002A,0x002B,0x002C,0x002D,0x002E,0x53C3,0x53C8,0x53C9,0x53CA,0x53CB
                   ,0x53CD,0x53D4,0x53D6,0x53D7,0x53DB,0x000A,0x000D,0x53E1,0x53E2
                   ,0x53E3,0x53E4,0x000A,0x000D};
    const UChar* uSource;
    const UChar* uSourceLimit;
    const char* cSource;
    const char* cSourceLimit;
    UChar *uTargetLimit =NULL;
    UChar *uTarget;
    char *cTarget;
    const char *cTargetLimit;
    char *cBuf;
    UChar *uBuf,*test;
    int32_t uBufSize = 120;
    UErrorCode errorCode=U_ZERO_ERROR;
    UConverter *cnv;
    int32_t* offsets = (int32_t*) malloc(uBufSize * sizeof(int32_t) * 5);
    int32_t* myOff= offsets;
    cnv=ucnv_open("ibm-25546", &errorCode);
    if(U_FAILURE(errorCode)) {
        log_data_err("Unable to open a iso-2022 converter: %s\n", u_errorName(errorCode));
        return;
    }

    uBuf =  (UChar*)malloc(uBufSize * sizeof(UChar)*5);
    cBuf =(char*)malloc(uBufSize * sizeof(char) * 5);
    uSource = (const UChar*)in;
    uSourceLimit=(const UChar*)in + (sizeof(in)/sizeof(in[0]));
    cTarget = cBuf;
    cTargetLimit = cBuf +uBufSize*5;
    uTarget = uBuf;
    uTargetLimit = uBuf+ uBufSize*5;
    ucnv_fromUnicode( cnv , &cTarget, cTargetLimit,&uSource,uSourceLimit,myOff,TRUE, &errorCode);
    if(U_FAILURE(errorCode)){
        log_err("ucnv_fromUnicode conversion failed reason %s\n", u_errorName(errorCode));
        return;
    }
    cSource = cBuf;
    cSourceLimit =cTarget;
    test =uBuf;
    myOff=offsets;
    ucnv_toUnicode(cnv,&uTarget,uTargetLimit,&cSource,cSourceLimit,myOff,TRUE,&errorCode);
    if(U_FAILURE(errorCode)){
        log_err("ucnv_toUnicode conversion failed reason %s\n", u_errorName(errorCode));
        return;
    }
    uSource = (const UChar*)in;
    while(uSource<uSourceLimit){
        if(*test!=*uSource){
            log_err("Expected : \\u%04X \t Got: \\u%04X\n",*uSource,*test) ;
        }
        uSource++;
        test++;
    }
    ucnv_reset(cnv);
    TestGetNextUChar2022(cnv, cBuf, cTarget, in, "ISO-2022-KR encoding");
    TestSmallTargetBuffer(in,(const UChar*)in + (sizeof(in)/sizeof(in[0])),cnv);
    TestSmallSourceBuffer(in,(const UChar*)in + (sizeof(in)/sizeof(in[0])),cnv);
    ucnv_reset(cnv);
    TestToAndFromUChars(in,(const UChar*)in + (sizeof(in)/sizeof(in[0])),cnv);
        /*Test for the condition where there is an invalid character*/
    ucnv_reset(cnv);
    {
        static const uint8_t source2[]={0x1b,0x24,0x053};
        ucnv_setToUCallBack(cnv, UCNV_TO_U_CALLBACK_STOP, NULL, NULL, NULL, &errorCode);
        TestNextUCharError(cnv, (const char*)source2, (const char*)source2+sizeof(source2), U_ILLEGAL_ESCAPE_SEQUENCE, "an invalid character [ISO-2022-KR]");
    }
    ucnv_close(cnv);
    free(uBuf);
    free(cBuf);
    free(offsets);
}

static void TestJitterbug2411(){
    static const char* source = "\x1b\x24\x29\x43\x6b\x6b\x6e\x6e\x6a\x68\x70\x6f\x69\x75\x79\x71\x77\x65\x68\x67\x0A"
                         "\x1b\x24\x29\x43\x6a\x61\x73\x64\x66\x6a\x61\x73\x64\x66\x68\x6f\x69\x75\x79\x1b\x24\x29\x43";
    UConverter* kr=NULL, *kr1=NULL;
    UErrorCode errorCode = U_ZERO_ERROR;
    UChar tgt[100]={'\0'};
    UChar* target = tgt;
    UChar* targetLimit = target+100;
    kr=ucnv_open("iso-2022-kr", &errorCode);
    if(U_FAILURE(errorCode)) {
        log_data_err("Unable to open a iso-2022-kr converter: %s\n", u_errorName(errorCode));
        return;
    }
    ucnv_toUnicode(kr,&target,targetLimit,&source,source+uprv_strlen(source),NULL,TRUE,&errorCode);
    if(U_FAILURE(errorCode)) {
        log_err("iso-2022-kr cannot handle multiple escape sequences : %s\n", u_errorName(errorCode));
        return;
    }
    kr1 = ucnv_open("ibm-25546", &errorCode);
    if(U_FAILURE(errorCode)) {
        log_data_err("Unable to open a iso-2022-kr_1 converter: %s\n", u_errorName(errorCode));
        return;
    }
    target = tgt;
    targetLimit = target+100;
    ucnv_toUnicode(kr,&target,targetLimit,&source,source+uprv_strlen(source),NULL,TRUE,&errorCode);
       
    if(U_FAILURE(errorCode)) {
        log_err("iso-2022-kr_1 cannot handle multiple escape sequences : %s\n", u_errorName(errorCode));
        return;
    }

    ucnv_close(kr);
    ucnv_close(kr1);
    
}

static void
TestJIS(){
    /* From Unicode moved to testdata/conversion.txt */
    /*To Unicode*/
    {
        static const uint8_t sampleTextJIS[] = {
            0x1b,0x28,0x48,0x41,0x42, /*jis-Roman*/
            0x1b,0x28,0x49,0x41,0x42, /*Katakana Set*/
            0x1b,0x26,0x40,0x1b,0x24,0x42,0x21,0x21 /*recognize and ignore <esc>&@*/
        };
        static const uint16_t expectedISO2022JIS[] = {
            0x0041, 0x0042,
            0xFF81, 0xFF82,
            0x3000
        };
        static const int32_t  toISO2022JISOffs[]={
            3,4,
            8,9,
            16
        };

        static const uint8_t sampleTextJIS7[] = {
            0x1b,0x28,0x48,0x41,0x42, /*JIS7-Roman*/
            0x1b,0x28,0x49,0x41,0x42, /*Katakana Set*/
            0x1b,0x24,0x42,0x21,0x21,
            0x0e,0x41,0x42,0x0f,      /*Test Katakana set with SI and SO */
            0x21,0x22,
            0x1b,0x26,0x40,0x1b,0x24,0x42,0x21,0x21 /*recognize and ignore <esc>&@*/
        };
        static const uint16_t expectedISO2022JIS7[] = {
            0x0041, 0x0042,
            0xFF81, 0xFF82,
            0x3000,
            0xFF81, 0xFF82,
            0x3001,
            0x3000
        };
        static const int32_t  toISO2022JIS7Offs[]={
            3,4,
            8,9,
            13,16,
            17,
            19,27
        };
        static const uint8_t sampleTextJIS8[] = {
            0x1b,0x28,0x48,0x41,0x42, /*JIS8-Roman*/
            0xa1,0xc8,0xd9,/*Katakana Set*/
            0x1b,0x28,0x42,
            0x41,0x42,
            0xb1,0xc3, /*Katakana Set*/
            0x1b,0x24,0x42,0x21,0x21
        };
        static const uint16_t expectedISO2022JIS8[] = {
            0x0041, 0x0042,
            0xff61, 0xff88, 0xff99,
            0x0041, 0x0042,
            0xff71, 0xff83,
            0x3000
        };
        static const int32_t  toISO2022JIS8Offs[]={
            3, 4,  5,  6,
            7, 11, 12, 13,
            14, 18,
        };

        testConvertToU(sampleTextJIS,sizeof(sampleTextJIS),expectedISO2022JIS,
            sizeof(expectedISO2022JIS)/sizeof(expectedISO2022JIS[0]),"JIS", toISO2022JISOffs,TRUE);
        testConvertToU(sampleTextJIS7,sizeof(sampleTextJIS7),expectedISO2022JIS7,
            sizeof(expectedISO2022JIS7)/sizeof(expectedISO2022JIS7[0]),"JIS7", toISO2022JIS7Offs,TRUE);
        testConvertToU(sampleTextJIS8,sizeof(sampleTextJIS8),expectedISO2022JIS8,
            sizeof(expectedISO2022JIS8)/sizeof(expectedISO2022JIS8[0]),"JIS8", toISO2022JIS8Offs,TRUE);
    }

}


#if 0
 ICU 4.4 (ticket #7314) removes mappings for CNS 11643 planes 3..7

static void TestJitterbug915(){
/* tests for roundtripping of the below sequence
\x1b$)G\x0E#!#"###$#%#&#'#(#)#*#+          / *plane 1 * /
\x1b$*H\x1bN"!\x1bN""\x1bN"#\x1bN"$\x1bN"% / *plane 2 * /
\x1b$+I\x1bO"D\x1bO"E\x1bO"F\x1bO"G\x1bO"H / *plane 3 * /
\x1b$+J\x1bO!D\x1bO!E\x1bO"j\x1bO"k\x1bO"l / *plane 4 * /
\x1b$+K\x1bO!t\x1bO"P\x1bO"Q\x1bO#7\x1bO"\ / *plane 5 * /
\x1b$+L\x1bO!#\x1bO",\x1bO#N\x1bO!n\x1bO#q / *plane 6 * /
\x1b$+M\x1bO"q\x1bO!N\x1bO!j\x1bO#:\x1bO#o / *plane 7 * /
*/
    static const char cSource[]={
        0x1B, 0x24, 0x29, 0x47, 0x0E, 0x23, 0x21, 0x23, 0x22, 0x23,
        0x23, 0x23, 0x24, 0x23, 0x25, 0x23, 0x26, 0x23, 0x27, 0x23,
        0x28, 0x23, 0x29, 0x23, 0x2A, 0x23, 0x2B, 0x0F, 0x2F, 0x2A,
        0x70, 0x6C, 0x61, 0x6E, 0x65, 0x20, 0x31, 0x20, 0x2A, 0x2F,
        0x0D, 0x0A, 0x1B, 0x24, 0x2A, 0x48, 0x1B, 0x4E, 0x22, 0x21,
        0x1B, 0x4E, 0x22, 0x22, 0x1B, 0x4E, 0x22, 0x23, 0x1B, 0x4E,
        0x22, 0x24, 0x1B, 0x4E, 0x22, 0x25, 0x2F, 0x2A, 0x70,
        0x6C, 0x61, 0x6E, 0x65, 0x32, 0x2A, 0x2F, 0x20, 0x0D, 0x0A,
        0x1B, 0x24, 0x2B, 0x49, 0x1B, 0x4F, 0x22, 0x44, 0x1B, 0x4F,
        0x22, 0x45, 0x1B, 0x4F, 0x22, 0x46, 0x1B, 0x4F, 0x22, 0x47,
        0x1B, 0x4F, 0x22, 0x48, 0x2F, 0x2A, 0x70, 0x6C, 0x61,
        0x6E, 0x65, 0x20, 0x33, 0x20, 0x2A, 0x2F, 0x0D, 0x0A, 0x1B,
        0x24, 0x2B, 0x4A, 0x1B, 0x4F, 0x21, 0x44, 0x1B, 0x4F, 0x21,
        0x45, 0x1B, 0x4F, 0x22, 0x6A, 0x1B, 0x4F, 0x22, 0x6B, 0x1B,
        0x4F, 0x22, 0x6C, 0x2F, 0x2A, 0x70, 0x6C, 0x61, 0x6E,
        0x65, 0x20, 0x34, 0x20, 0x2A, 0x2F, 0x0D, 0x0A, 0x1B, 0x24,
        0x2B, 0x4B, 0x1B, 0x4F, 0x21, 0x74, 0x1B, 0x4F, 0x22, 0x50,
        0x1B, 0x4F, 0x22, 0x51, 0x1B, 0x4F, 0x23, 0x37, 0x1B, 0x4F,
        0x22, 0x5C, 0x2F, 0x2A, 0x70, 0x6C, 0x61, 0x6E,
        0x65, 0x20, 0x35, 0x20, 0x2A, 0x2F, 0x0D, 0x0A, 0x1B, 0x24,
        0x2B, 0x4C, 0x1B, 0x4F, 0x21, 0x23, 0x1B, 0x4F, 0x22, 0x2C,
        0x1B, 0x4F, 0x23, 0x4E, 0x1B, 0x4F, 0x21, 0x6E, 0x1B, 0x4F,
        0x23, 0x71, 0x2F, 0x2A, 0x70, 0x6C, 0x61, 0x6E, 0x65,
        0x20, 0x36, 0x20, 0x2A, 0x2F, 0x0D, 0x0A, 0x1B, 0x24, 0x2B,
        0x4D, 0x1B, 0x4F, 0x22, 0x71, 0x1B, 0x4F, 0x21, 0x4E, 0x1B,
        0x4F, 0x21, 0x6A, 0x1B, 0x4F, 0x23, 0x3A, 0x1B, 0x4F, 0x23,
        0x6F, 0x2F, 0x2A, 0x70, 0x6C, 0x61, 0x6E, 0x65, 0x20,
        0x37, 0x20, 0x2A, 0x2F
    };
    UChar uTarget[500]={'\0'};
    UChar* utarget=uTarget;
    UChar* utargetLimit=uTarget+sizeof(uTarget)/2;

    char cTarget[500]={'\0'};
    char* ctarget=cTarget;
    char* ctargetLimit=cTarget+sizeof(cTarget);
    const char* csource=cSource;
    const char* tempSrc = cSource;
    UErrorCode err=U_ZERO_ERROR;

    UConverter* conv =ucnv_open("ISO_2022_CN_EXT",&err);
    if(U_FAILURE(err)) {
        log_data_err("Unable to open a iso-2022 converter: %s\n", u_errorName(err));
        return;
    }
    ucnv_toUnicode(conv,&utarget,utargetLimit,&csource,csource+sizeof(cSource),NULL,TRUE,&err);
    if(U_FAILURE(err)) {
        log_err("iso-2022-CN to Unicode conversion failed: %s\n", u_errorName(err));
        return;
    }
    utargetLimit=utarget;
    utarget = uTarget;
    ucnv_fromUnicode(conv,&ctarget,ctargetLimit,(const UChar**)&utarget,utargetLimit,NULL,TRUE,&err);
    if(U_FAILURE(err)) {
        log_err("iso-2022-CN from Unicode conversion failed: %s\n", u_errorName(err));
        return;
    }
    ctargetLimit=ctarget;
    ctarget =cTarget;
    while(ctarget<ctargetLimit){
        if(*ctarget != *tempSrc){
            log_err("j915[%d] Expected : \\x%02X \t Got: \\x%02X\n", (int)(ctarget-cTarget), *ctarget,(int)*tempSrc) ;
        }
        ++ctarget;
        ++tempSrc;
    }

    ucnv_close(conv);
}

static void
TestISO_2022_CN_EXT() {
    /* test input */
    static const uint16_t in[]={
                /* test Non-BMP code points */
         0xD869, 0xDE99, 0xD869, 0xDE9C, 0xD869, 0xDE9D, 0xD869, 0xDE9E, 0xD869, 0xDE9F,
         0xD869, 0xDEA0, 0xD869, 0xDEA5, 0xD869, 0xDEA6, 0xD869, 0xDEA7, 0xD869, 0xDEA8,
         0xD869, 0xDEAB, 0xD869, 0xDEAC, 0xD869, 0xDEAD, 0xD869, 0xDEAE, 0xD869, 0xDEAF,
         0xD869, 0xDEB0, 0xD869, 0xDEB1, 0xD869, 0xDEB3, 0xD869, 0xDEB5, 0xD869, 0xDEB6,
         0xD869, 0xDEB7, 0xD869, 0xDEB8, 0xD869, 0xDEB9, 0xD869, 0xDEBA, 0xD869, 0xDEBB,
         0xD869, 0xDEBC, 0xD869, 0xDEBD, 0xD869, 0xDEBE, 0xD869, 0xDEBF, 0xD869, 0xDEC0,
         0xD869, 0xDEC1, 0xD869, 0xDEC2, 0xD869, 0xDEC3, 0xD869, 0xDEC4, 0xD869, 0xDEC8,
         0xD869, 0xDECA, 0xD869, 0xDECB, 0xD869, 0xDECD, 0xD869, 0xDECE, 0xD869, 0xDECF,
         0xD869, 0xDED0, 0xD869, 0xDED1, 0xD869, 0xDED2, 0xD869, 0xDED3, 0xD869, 0xDED4,
         0xD869, 0xDED5,

         0x4DB3, 0x4DB4, 0x4DB5, 0x4E00, 0x4E00, 0x4E01, 0x4E02, 0x4E03, 0x000D, 0x000A,
         0x0392, 0x0393, 0x0394, 0x0395, 0x0396, 0x0397, 0x33E0, 0x33E6, 0x000D, 0x000A,
         0x4E05, 0x4E07, 0x4E04, 0x4E08, 0x4E08, 0x4E09, 0x4E0A, 0x4E0B, 0x000D, 0x000A,
         0x4E0C, 0x0021, 0x0022, 0x0023, 0x0024, 0xFF40, 0xFF41, 0xFF42, 0x000D, 0x000A,
         0xFF43, 0xFF44, 0xFF45, 0xFF46, 0xFF47, 0xFF48, 0xFF49, 0xFF4A, 0x000D, 0x000A,
         0xFF4B, 0xFF4C, 0xFF4D, 0xFF4E, 0xFF4F, 0x6332, 0x63B0, 0x643F, 0x000D, 0x000A,
         0x64D8, 0x8004, 0x6BEA, 0x6BF3, 0x6BFD, 0x6BF5, 0x6BF9, 0x6C05, 0x000D, 0x000A,
         0x0041, 0x0042, 0x0043, 0x0044, 0x0045, 0x0046, 0x0047, 0x0048, 0x000D, 0x000A,
         0x6C07, 0x6C06, 0x6C0D, 0x6C15, 0x9CD9, 0x9CDC, 0x9CDD, 0x9CDF, 0x000D, 0x000A,
         0x9CE2, 0x977C, 0x9785, 0x9791, 0x9792, 0x9794, 0x97AF, 0x97AB, 0x000D, 0x000A,
         0x97A3, 0x97B2, 0x97B4, 0x9AB1, 0x9AB0, 0x9AB7, 0x9E58, 0x9AB6, 0x000D, 0x000A,
         0x9ABA, 0x9ABC, 0x9AC1, 0x9AC0, 0x9AC5, 0x9AC2, 0x9ACB, 0x9ACC, 0x000D, 0x000A,
         0x9AD1, 0x9B45, 0x9B43, 0x9B47, 0x9B49, 0x9B48, 0x9B4D, 0x9B51, 0x000D, 0x000A,
         0x98E8, 0x990D, 0x992E, 0x9955, 0x9954, 0x9ADF, 0x3443, 0x3444, 0x000D, 0x000A,
         0x3445, 0x3449, 0x344A, 0x344B, 0x60F2, 0x60F3, 0x60F4, 0x60F5, 0x000D, 0x000A,
         0x60F6, 0x60F7, 0x60F8, 0x60F9, 0x60FA, 0x60FB, 0x60FC, 0x60FD, 0x000D, 0x000A,
         0x60FE, 0x60FF, 0x6100, 0x6101, 0x6102, 0x0041, 0x0042, 0x0043, 0x000D, 0x000A,
         0x0044, 0x0045, 0x0046, 0x0047, 0x0048, 0x0049, 0x004A, 0x004B, 0x000D, 0x000A,

         0x33E7, 0x33E8, 0x33E9, 0x33EA, 0x000D, 0x000A

      };

    const UChar* uSource;
    const UChar* uSourceLimit;
    const char* cSource;
    const char* cSourceLimit;
    UChar *uTargetLimit =NULL;
    UChar *uTarget;
    char *cTarget;
    const char *cTargetLimit;
    char *cBuf;
    UChar *uBuf,*test;
    int32_t uBufSize = 180;
    UErrorCode errorCode=U_ZERO_ERROR;
    UConverter *cnv;
    int32_t* offsets = (int32_t*) malloc(uBufSize * sizeof(int32_t) * 5);
    int32_t* myOff= offsets;
    cnv=ucnv_open("ISO_2022,locale=cn,version=1", &errorCode);
    if(U_FAILURE(errorCode)) {
        log_data_err("Unable to open a iso-2022 converter: %s\n", u_errorName(errorCode));
        return;
    }

    uBuf =  (UChar*)malloc(uBufSize * sizeof(UChar)*5);
    cBuf =(char*)malloc(uBufSize * sizeof(char) * 10);
    uSource = (const UChar*)in;
    uSourceLimit=(const UChar*)in + (sizeof(in)/sizeof(in[0]));
    cTarget = cBuf;
    cTargetLimit = cBuf +uBufSize*5;
    uTarget = uBuf;
    uTargetLimit = uBuf+ uBufSize*5;
    ucnv_fromUnicode( cnv , &cTarget, cTargetLimit,&uSource,uSourceLimit,myOff,TRUE, &errorCode);
    if(U_FAILURE(errorCode)){
        log_err("ucnv_fromUnicode conversion failed reason %s\n", u_errorName(errorCode));
        return;
    }
    cSource = cBuf;
    cSourceLimit =cTarget;
    test =uBuf;
    myOff=offsets;
    ucnv_toUnicode(cnv,&uTarget,uTargetLimit,&cSource,cSourceLimit,myOff,TRUE,&errorCode);
    if(U_FAILURE(errorCode)){
        log_err("ucnv_toUnicode conversion failed reason %s\n", u_errorName(errorCode));
        return;
    }
    uSource = (const UChar*)in;
    while(uSource<uSourceLimit){
        if(*test!=*uSource){
            log_err("Expected : \\u%04X \t Got: \\u%04X\n",*uSource,(int)*test) ;
        }
        else{
            log_verbose("      Got: \\u%04X\n",(int)*test) ;
        }
        uSource++;
        test++;
    }
    TestSmallTargetBuffer(in,(const UChar*)in + (sizeof(in)/sizeof(in[0])),cnv);
    TestSmallSourceBuffer(in,(const UChar*)in + (sizeof(in)/sizeof(in[0])),cnv);
    /*Test for the condition where there is an invalid character*/
    ucnv_reset(cnv);
    {
        static const uint8_t source2[]={0x0e,0x24,0x053};
        TestNextUCharError(cnv, (const char*)source2, (const char*)source2+sizeof(source2), U_ZERO_ERROR, "an invalid character [ISO-2022-CN-EXT]");
    }
    ucnv_close(cnv);
    free(uBuf);
    free(cBuf);
    free(offsets);
}
#endif

static void
TestISO_2022_CN() {
    /* test input */
    static const uint16_t in[]={
         /* jitterbug 951 */
         0xFF2D, 0xFF49, 0xFF58, 0xFF45, 0xFF44, 0x0020, 0xFF43, 0xFF48, 0xFF41, 0xFF52,
         0x0020, 0xFF06, 0x0020, 0xFF11, 0xFF12, 0xFF13, 0xFF14, 0xFF15, 0xFF16, 0xFF17,
         0xFF18, 0xFF19, 0xFF10, 0x0020, 0xFF4E, 0xFF55, 0xFF4D, 0xFF42, 0xFF45, 0xFF52,
         0x0020, 0xFF54, 0xFF45, 0xFF53, 0xFF54, 0x0020, 0xFF4C, 0xFF49, 0xFF4E, 0xFF45,
         0x0020, 0x0045, 0x004e, 0x0044,
         /**/
         0x4E00, 0x4E00, 0x4E01, 0x4E03, 0x60F6, 0x60F7, 0x60F8, 0x60FB, 0x000D, 0x000A,
         0x0392, 0x0393, 0x0394, 0x0395, 0x0396, 0x0397, 0x60FB, 0x60FC, 0x000D, 0x000A,
         0x4E07, 0x4E08, 0x4E08, 0x4E09, 0x4E0A, 0x4E0B, 0x0042, 0x0043, 0x000D, 0x000A,
         0x4E0C, 0x0021, 0x0022, 0x0023, 0x0024, 0xFF40, 0xFF41, 0xFF42, 0x000D, 0x000A,
         0xFF43, 0xFF44, 0xFF45, 0xFF46, 0xFF47, 0xFF48, 0xFF49, 0xFF4A, 0x000D, 0x000A,
         0xFF4B, 0xFF4C, 0xFF4D, 0xFF4E, 0xFF4F, 0x6332, 0x63B0, 0x643F, 0x000D, 0x000A,
         0x64D8, 0x8004, 0x6BEA, 0x6BF3, 0x6BFD, 0x6BF5, 0x6BF9, 0x6C05, 0x000D, 0x000A,
         0x6C07, 0x6C06, 0x6C0D, 0x6C15, 0x9CD9, 0x9CDC, 0x9CDD, 0x9CDF, 0x000D, 0x000A,
         0x9CE2, 0x977C, 0x9785, 0x9791, 0x9792, 0x9794, 0x97AF, 0x97AB, 0x000D, 0x000A,
         0x97A3, 0x97B2, 0x97B4, 0x9AB1, 0x9AB0, 0x9AB7, 0x9E58, 0x9AB6, 0x000D, 0x000A,
         0x9ABA, 0x9ABC, 0x9AC1, 0x9AC0, 0x9AC5, 0x9AC2, 0x9ACB, 0x9ACC, 0x000D, 0x000A,
         0x9AD1, 0x9B45, 0x9B43, 0x9B47, 0x9B49, 0x9B48, 0x9B4D, 0x9B51, 0x000D, 0x000A,
         0x98E8, 0x990D, 0x992E, 0x9955, 0x9954, 0x9ADF, 0x60FE, 0x60FF, 0x000D, 0x000A,
         0x60F2, 0x60F3, 0x60F4, 0x60F5, 0x000D, 0x000A, 0x60F9, 0x60FA, 0x000D, 0x000A,
         0x6100, 0x6101, 0x0041, 0x0044, 0x0045, 0x0046, 0x0047, 0x0048, 0x000D, 0x000A,
         0x247D, 0x247E, 0x247F, 0x2480, 0x2481, 0x2482, 0x2483, 0x2484, 0x2485, 0x2486,
         0x2487, 0x2460, 0x2461, 0xFF20, 0xFF21, 0xFF22, 0x0049, 0x004A, 0x000D, 0x000A,

      };
    const UChar* uSource;
    const UChar* uSourceLimit;
    const char* cSource;
    const char* cSourceLimit;
    UChar *uTargetLimit =NULL;
    UChar *uTarget;
    char *cTarget;
    const char *cTargetLimit;
    char *cBuf;
    UChar *uBuf,*test;
    int32_t uBufSize = 180;
    UErrorCode errorCode=U_ZERO_ERROR;
    UConverter *cnv;
    int32_t* offsets = (int32_t*) malloc(uBufSize * sizeof(int32_t) * 5);
    int32_t* myOff= offsets;
    cnv=ucnv_open("ISO_2022,locale=cn,version=0", &errorCode);
    if(U_FAILURE(errorCode)) {
        log_data_err("Unable to open a iso-2022 converter: %s\n", u_errorName(errorCode));
        return;
    }

    uBuf =  (UChar*)malloc(uBufSize * sizeof(UChar)*5);
    cBuf =(char*)malloc(uBufSize * sizeof(char) * 10);
    uSource = (const UChar*)in;
    uSourceLimit=(const UChar*)in + (sizeof(in)/sizeof(in[0]));
    cTarget = cBuf;
    cTargetLimit = cBuf +uBufSize*5;
    uTarget = uBuf;
    uTargetLimit = uBuf+ uBufSize*5;
    ucnv_fromUnicode( cnv , &cTarget, cTargetLimit,&uSource,uSourceLimit,myOff,TRUE, &errorCode);
    if(U_FAILURE(errorCode)){
        log_err("ucnv_fromUnicode conversion failed reason %s\n", u_errorName(errorCode));
        return;
    }
    cSource = cBuf;
    cSourceLimit =cTarget;
    test =uBuf;
    myOff=offsets;
    ucnv_toUnicode(cnv,&uTarget,uTargetLimit,&cSource,cSourceLimit,myOff,TRUE,&errorCode);
    if(U_FAILURE(errorCode)){
        log_err("ucnv_toUnicode conversion failed reason %s\n", u_errorName(errorCode));
        return;
    }
    uSource = (const UChar*)in;
    while(uSource<uSourceLimit){
        if(*test!=*uSource){
            log_err("Expected : \\u%04X \t Got: \\u%04X\n",*uSource,(int)*test) ;
        }
        else{
            log_verbose("      Got: \\u%04X\n",(int)*test) ;
        }
        uSource++;
        test++;
    }
    TestGetNextUChar2022(cnv, cBuf, cTarget, in, "ISO-2022-CN encoding");
    TestSmallTargetBuffer(in,(const UChar*)in + (sizeof(in)/sizeof(in[0])),cnv);
    TestSmallSourceBuffer(in,(const UChar*)in + (sizeof(in)/sizeof(in[0])),cnv);
    TestToAndFromUChars(in,(const UChar*)in + (sizeof(in)/sizeof(in[0])),cnv);
    TestJitterbug930("csISO2022CN");
    /*Test for the condition where there is an invalid character*/
    ucnv_reset(cnv);
    {
        static const uint8_t source2[]={0x0e,0x24,0x053};
        TestNextUCharError(cnv, (const char*)source2, (const char*)source2+sizeof(source2), U_ZERO_ERROR, "an invalid character [ISO-2022-CN]");
    }

    ucnv_close(cnv);
    free(uBuf);
    free(cBuf);
    free(offsets);
}

/* Tests for empty segments in ISO-2022-JP/KR/CN, HZ, check that UConverterCallbackReason is UCNV_IRREGULAR */
typedef struct {
    const char *    converterName;
    const char *    inputText;
    int             inputTextLength;
} EmptySegmentTest;

/* Callback for TestJitterbug6175, should only get called for empty segment errors */
static void UCNV_TO_U_CALLBACK_EMPTYSEGMENT( const void *context, UConverterToUnicodeArgs *toArgs, const char* codeUnits,
                                             int32_t length, UConverterCallbackReason reason, UErrorCode * err ) {
    if (reason > UCNV_IRREGULAR) {
        return;
    }
    if (reason != UCNV_IRREGULAR) {
        log_err("toUnicode callback invoked for empty segment but reason is not UCNV_IRREGULAR\n");
    }
    /* Standard stuff below from UCNV_TO_U_CALLBACK_SUBSTITUTE */
    *err = U_ZERO_ERROR;
    ucnv_cbToUWriteSub(toArgs,0,err);
}

enum { kEmptySegmentToUCharsMax = 64 };
static void TestJitterbug6175(void) {
    static const char  iso2022jp_a[] = { 0x61, 0x62, 0x1B,0x24,0x42, 0x1B,0x28,0x42, 0x63, 0x64, 0x0D, 0x0A };
    static const char  iso2022kr_a[] = { 0x1B,0x24,0x29,0x43, 0x61, 0x0E, 0x0F, 0x62, 0x0D, 0x0A };
    static const char  iso2022cn_a[] = { 0x61, 0x1B,0x24,0x29,0x41, 0x62, 0x0E, 0x0F, 0x1B,0x24,0x2A,0x48, 0x1B,0x4E, 0x6A,0x65, 0x63, 0x0D, 0x0A };
    static const char  iso2022cn_b[] = { 0x61, 0x1B,0x24,0x29,0x41, 0x62, 0x0E, 0x1B,0x24,0x29,0x47, 0x68,0x64, 0x0F, 0x63, 0x0D, 0x0A };
    static const char  hzGB2312_a[]  = { 0x61, 0x62, 0x7E,0x7B, 0x7E,0x7D, 0x63, 0x64 };
    static const EmptySegmentTest emptySegmentTests[] = {
        /* converterName inputText    inputTextLength */
        { "ISO-2022-JP", iso2022jp_a, sizeof(iso2022jp_a) },
        { "ISO-2022-KR", iso2022kr_a, sizeof(iso2022kr_a) },
        { "ISO-2022-CN", iso2022cn_a, sizeof(iso2022cn_a) },
        { "ISO-2022-CN", iso2022cn_b, sizeof(iso2022cn_b) },
        { "HZ-GB-2312",  hzGB2312_a,  sizeof(hzGB2312_a)  },
        /* terminator: */
        { NULL,          NULL,        0,                  }
    };
    const EmptySegmentTest * testPtr;
    for (testPtr = emptySegmentTests; testPtr->converterName != NULL; ++testPtr) {
        UErrorCode   err = U_ZERO_ERROR;
        UConverter * cnv = ucnv_open(testPtr->converterName, &err);
        if (U_FAILURE(err)) {
            log_data_err("Unable to open %s converter: %s\n", testPtr->converterName, u_errorName(err));
            return;
        }
        ucnv_setToUCallBack(cnv, UCNV_TO_U_CALLBACK_EMPTYSEGMENT, NULL, NULL, NULL, &err);
        if (U_FAILURE(err)) {
            log_data_err("Unable to setToUCallBack for %s converter: %s\n", testPtr->converterName, u_errorName(err));
            ucnv_close(cnv);
            return;
        }
        {
            UChar         toUChars[kEmptySegmentToUCharsMax];
            UChar *       toUCharsPtr = toUChars;
            const UChar * toUCharsLimit = toUCharsPtr + kEmptySegmentToUCharsMax;
            const char *  inCharsPtr = testPtr->inputText;
            const char *  inCharsLimit = inCharsPtr + testPtr->inputTextLength;
            ucnv_toUnicode(cnv, &toUCharsPtr, toUCharsLimit, &inCharsPtr, inCharsLimit, NULL, TRUE, &err);
        }
        ucnv_close(cnv);
    }
}

static void
TestEBCDIC_STATEFUL() {
    /* test input */
    static const uint8_t in[]={
        0x61,
        0x1a,
        0x0f, 0x4b,
        0x42,
        0x40,
        0x36,
    };

    /* expected test results */
    static const int32_t results[]={
        /* number of bytes read, code point */
        1, 0x002f,
        1, 0x0092,
        2, 0x002e,
        1, 0xff62,
        1, 0x0020,
        1, 0x0096,

    };
    static const uint8_t in2[]={
        0x0f,
        0xa1,
        0x01
    };

    /* expected test results */
    static const int32_t results2[]={
        /* number of bytes read, code point */
        2, 0x203E,
        1, 0x0001,
    };

    const char *source=(const char *)in, *limit=(const char *)in+sizeof(in);
    UErrorCode errorCode=U_ZERO_ERROR;
    UConverter *cnv=ucnv_open("ibm-930", &errorCode);
    if(U_FAILURE(errorCode)) {
        log_data_err("Unable to open a EBCDIC_STATEFUL(ibm-930) converter: %s\n", u_errorName(errorCode));
        return;
    }
    TestNextUChar(cnv, source, limit, results, "EBCDIC_STATEFUL(ibm-930)");
    ucnv_reset(cnv);
     /* Test the condition when source >= sourceLimit */
    TestNextUCharError(cnv, source, source, U_INDEX_OUTOFBOUNDS_ERROR, "sourceLimit <= source");
    ucnv_reset(cnv);
    /*Test for the condition where source > sourcelimit after consuming the shift chracter */
    {
        static const uint8_t source1[]={0x0f};
        TestNextUCharError(cnv, (const char*)source1, (const char*)source1+sizeof(source1), U_INDEX_OUTOFBOUNDS_ERROR, "a character is truncated");
    }
    /*Test for the condition where there is an invalid character*/
    ucnv_reset(cnv);
    {
        static const uint8_t source2[]={0x0e, 0x7F, 0xFF};
        TestNextUCharError(cnv, (const char*)source2, (const char*)source2+sizeof(source2), U_ZERO_ERROR, "an invalid character [EBCDIC STATEFUL]");
    }
    ucnv_reset(cnv);
    source=(const char*)in2;
    limit=(const char*)in2+sizeof(in2);
    TestNextUChar(cnv,source,limit,results2,"EBCDIC_STATEFUL(ibm-930),seq#2");
    ucnv_close(cnv);

}

static void
TestGB18030() {
    /* test input */
    static const uint8_t in[]={
        0x24,
        0x7f,
        0x81, 0x30, 0x81, 0x30,
        0xa8, 0xbf,
        0xa2, 0xe3,
        0xd2, 0xbb,
        0x82, 0x35, 0x8f, 0x33,
        0x84, 0x31, 0xa4, 0x39,
        0x90, 0x30, 0x81, 0x30,
        0xe3, 0x32, 0x9a, 0x35
#if 0
        /*
         * Feature removed   markus 2000-oct-26
         * Only some codepages must match surrogate pairs into supplementary code points -
         * see javadoc for ucnv_getNextUChar() and implementation notes in ucnvmbcs.c .
         * GB 18030 provides direct encodings for supplementary code points, therefore
         * it must not combine two single-encoded surrogates into one code point.
         */
        0x83, 0x36, 0xc8, 0x30, 0x83, 0x37, 0xb0, 0x34 /* separately encoded surrogates */
#endif
    };

    /* expected test results */
    static const int32_t results[]={
        /* number of bytes read, code point */
        1, 0x24,
        1, 0x7f,
        4, 0x80,
        2, 0x1f9,
        2, 0x20ac,
        2, 0x4e00,
        4, 0x9fa6,
        4, 0xffff,
        4, 0x10000,
        4, 0x10ffff
#if 0
        /* Feature removed. See comment above. */
        8, 0x10000
#endif
    };

/*    const char *source=(const char *)in,*limit=(const char *)in+sizeof(in); */
    UErrorCode errorCode=U_ZERO_ERROR;
    UConverter *cnv=ucnv_open("gb18030", &errorCode);
    if(U_FAILURE(errorCode)) {
        log_data_err("Unable to open a gb18030 converter: %s\n", u_errorName(errorCode));
        return;
    }
    TestNextUChar(cnv, (const char *)in, (const char *)in+sizeof(in), results, "gb18030");
    ucnv_close(cnv);
}

static void
TestLMBCS() {
    /* LMBCS-1 string */
    static const uint8_t pszLMBCS[]={
        0x61,
        0x01, 0x29,
        0x81,
        0xA0,
        0x0F, 0x27,
        0x0F, 0x91,
        0x14, 0x0a, 0x74,
        0x14, 0xF6, 0x02,
        0x14, 0xd8, 0x4d, 0x14, 0xdc, 0x56, /* UTF-16 surrogate pair */
        0x10, 0x88, 0xA0,
    };

    /* Unicode UChar32 equivalents */
    static const UChar32 pszUnicode32[]={
        /* code point */
        0x00000061,
        0x00002013,
        0x000000FC,
        0x000000E1,
        0x00000007,
        0x00000091,
        0x00000a74,
        0x00000200,
        0x00023456, /* code point for surrogate pair */
        0x00005516
    };

/* Unicode UChar equivalents */
    static const UChar pszUnicode[]={
        /* code point */
        0x0061,
        0x2013,
        0x00FC,
        0x00E1,
        0x0007,
        0x0091,
        0x0a74,
        0x0200,
        0xD84D, /* low surrogate */
        0xDC56, /* high surrogate */
        0x5516
    };

/* expected test results */
    static const int offsets32[]={
        /* number of bytes read, code point */
        0,
        1,
        3,
        4,
        5,
        7,
        9,
        12,
        15,
        21,
        24
    };

/* expected test results */
    static const int offsets[]={
        /* number of bytes read, code point */
        0,
        1,
        3,
        4,
        5,
        7,
        9,
        12,
        15,
        18,
        21,
        24
    };


    UConverter *cnv;

#define NAME_LMBCS_1 "LMBCS-1"
#define NAME_LMBCS_2 "LMBCS-2"


   /* Some basic open/close/property tests on some LMBCS converters */
    {

      char expected_subchars[] = {0x3F};   /* ANSI Question Mark */
      char new_subchars [] = {0x7F};       /* subst char used by SmartSuite..*/
      char get_subchars [1];
      const char * get_name;
      UConverter *cnv1;
      UConverter *cnv2;

      int8_t len = sizeof(get_subchars);

      UErrorCode errorCode=U_ZERO_ERROR;

      /* Open */
      cnv1=ucnv_open(NAME_LMBCS_1, &errorCode);
      if(U_FAILURE(errorCode)) {
         log_data_err("Unable to open a LMBCS-1 converter: %s\n", u_errorName(errorCode));
         return;
      }
      cnv2=ucnv_open(NAME_LMBCS_2, &errorCode);
      if(U_FAILURE(errorCode)) {
         log_data_err("Unable to open a LMBCS-2 converter: %s\n", u_errorName(errorCode));
         return;
      }

      /* Name */
      get_name = ucnv_getName (cnv1, &errorCode);
      if (strcmp(NAME_LMBCS_1,get_name)){
         log_err("Unexpected converter name: %s\n", get_name);
      }
      get_name = ucnv_getName (cnv2, &errorCode);
      if (strcmp(NAME_LMBCS_2,get_name)){
         log_err("Unexpected converter name: %s\n", get_name);
      }

      /* substitution chars */
      ucnv_getSubstChars (cnv1, get_subchars, &len, &errorCode);
      if(U_FAILURE(errorCode)) {
         log_err("Failure on get subst chars: %s\n", u_errorName(errorCode));
      }
      if (len!=1){
         log_err("Unexpected length of sub chars\n");
      }
      if (get_subchars[0] != expected_subchars[0]){
           log_err("Unexpected value of sub chars\n");
      }
      ucnv_setSubstChars (cnv2,new_subchars, len, &errorCode);
      if(U_FAILURE(errorCode)) {
         log_err("Failure on set subst chars: %s\n", u_errorName(errorCode));
      }
      ucnv_getSubstChars (cnv2, get_subchars, &len, &errorCode);
      if(U_FAILURE(errorCode)) {
         log_err("Failure on get subst chars: %s\n", u_errorName(errorCode));
      }
      if (len!=1){
         log_err("Unexpected length of sub chars\n");
      }
      if (get_subchars[0] != new_subchars[0]){
           log_err("Unexpected value of sub chars\n");
      }
      ucnv_close(cnv1);
      ucnv_close(cnv2);

    }

    /* LMBCS to Unicode - offsets */
    {
       UErrorCode errorCode=U_ZERO_ERROR;

       const char * pSource = (const char *)pszLMBCS;
       const char * sourceLimit = (const char *)pszLMBCS + sizeof(pszLMBCS);

       UChar Out [sizeof(pszUnicode) + 1];
       UChar * pOut = Out;
       UChar * OutLimit = Out + sizeof(pszUnicode)/sizeof(UChar);

       int32_t off [sizeof(offsets)];

      /* last 'offset' in expected results is just the final size.
         (Makes other tests easier). Compensate here: */

       off[(sizeof(offsets)/sizeof(offsets[0]))-1] = sizeof(pszLMBCS);



      cnv=ucnv_open("lmbcs", &errorCode); /* use generic name for LMBCS-1 */
      if(U_FAILURE(errorCode)) {
           log_data_err("Unable to open a LMBCS converter: %s\n", u_errorName(errorCode));
           return;
      }



      ucnv_toUnicode (cnv,
                      &pOut,
                      OutLimit,
                      &pSource,
                      sourceLimit,
                      off,
                      TRUE,
                      &errorCode);


       if (memcmp(off,offsets,sizeof(offsets)))
       {
         log_err("LMBCS->Uni: Calculated offsets do not match expected results\n");
       }
       if (memcmp(Out,pszUnicode,sizeof(pszUnicode)))
       {
         log_err("LMBCS->Uni: Calculated codepoints do not match expected results\n");
       }
       ucnv_close(cnv);
    }
    {
   /* LMBCS to Unicode - getNextUChar */
      const char * sourceStart;
      const char *source=(const char *)pszLMBCS;
      const char *limit=(const char *)pszLMBCS+sizeof(pszLMBCS);
      const UChar32 *results= pszUnicode32;
      const int *off = offsets32;

      UErrorCode errorCode=U_ZERO_ERROR;
      UChar32 uniChar;

      cnv=ucnv_open("LMBCS-1", &errorCode);
      if(U_FAILURE(errorCode)) {
           log_data_err("Unable to open a LMBCS-1 converter: %s\n", u_errorName(errorCode));
           return;
      }
      else
      {

         while(source<limit) {
            sourceStart=source;
            uniChar=ucnv_getNextUChar(cnv, &source, source + (off[1] - off[0]), &errorCode);
            if(U_FAILURE(errorCode)) {
                  log_err("LMBCS-1 ucnv_getNextUChar() failed: %s\n", u_errorName(errorCode));
                  break;
            } else if(source-sourceStart != off[1] - off[0] || uniChar != *results) {
               log_err("LMBCS-1 ucnv_getNextUChar() result %lx from %d bytes, should have been %lx from %d bytes.\n",
                   uniChar, (source-sourceStart), *results, *off);
               break;
            }
            results++;
            off++;
         }
       }
       ucnv_close(cnv);
    }
    { /* test locale & optimization group operations: Unicode to LMBCS */

      UErrorCode errorCode=U_ZERO_ERROR;
      UConverter *cnv16he = ucnv_open("LMBCS-16,locale=he", &errorCode);
      UConverter *cnv16jp = ucnv_open("LMBCS-16,locale=ja_JP", &errorCode);
      UConverter *cnv01us = ucnv_open("LMBCS-1,locale=us_EN", &errorCode);
      UChar uniString [] = {0x0192}; /* Latin Small letter f with hook */
      const UChar * pUniOut = uniString;
      UChar * pUniIn = uniString;
      uint8_t lmbcsString [4];
      const char * pLMBCSOut = (const char *)lmbcsString;
      char * pLMBCSIn = (char *)lmbcsString;

      /* 0192 (hook) converts to both group 3 & group 1. input locale should differentiate */
      ucnv_fromUnicode (cnv16he,
                        &pLMBCSIn, (pLMBCSIn + sizeof(lmbcsString)/sizeof(lmbcsString[0])),
                        &pUniOut, pUniOut + sizeof(uniString)/sizeof(uniString[0]),
                        NULL, 1, &errorCode);

      if (lmbcsString[0] != 0x3 || lmbcsString[1] != 0x83)
      {
         log_err("LMBCS-16,locale=he gives unexpected translation\n");
      }

      pLMBCSIn= (char *)lmbcsString;
      pUniOut = uniString;
      ucnv_fromUnicode (cnv01us,
                        &pLMBCSIn, (const char *)(lmbcsString + sizeof(lmbcsString)/sizeof(lmbcsString[0])),
                        &pUniOut, pUniOut + sizeof(uniString)/sizeof(uniString[0]),
                        NULL, 1, &errorCode);

      if (lmbcsString[0] != 0x9F)
      {
         log_err("LMBCS-1,locale=US gives unexpected translation\n");
      }

      /* single byte char from mbcs char set */
      lmbcsString[0] = 0xAE;  /* 1/2 width katakana letter small Yo */
      pLMBCSOut = (const char *)lmbcsString;
      pUniIn = uniString;
      ucnv_toUnicode (cnv16jp,
                        &pUniIn, pUniIn + 1,
                        &pLMBCSOut, (pLMBCSOut + 1),
                        NULL, 1, &errorCode);
      if (U_FAILURE(errorCode) || pLMBCSOut != (const char *)lmbcsString+1 || pUniIn != uniString+1 || uniString[0] != 0xFF6E)
      {
           log_err("Unexpected results from LMBCS-16 single byte char\n");
      }
      /* convert to group 1: should be 3 bytes */
      pLMBCSIn = (char *)lmbcsString;
      pUniOut = uniString;
      ucnv_fromUnicode (cnv01us,
                        &pLMBCSIn, (const char *)(pLMBCSIn + 3),
                        &pUniOut, pUniOut + 1,
                        NULL, 1, &errorCode);
      if (U_FAILURE(errorCode) || pLMBCSIn != (const char *)lmbcsString+3 || pUniOut != uniString+1
         || lmbcsString[0] != 0x10 || lmbcsString[1] != 0x10 || lmbcsString[2] != 0xAE)
      {
           log_err("Unexpected results to LMBCS-1 single byte mbcs char\n");
      }
      pLMBCSOut = (const char *)lmbcsString;
      pUniIn = uniString;
      ucnv_toUnicode (cnv01us,
                        &pUniIn, pUniIn + 1,
                        &pLMBCSOut, (const char *)(pLMBCSOut + 3),
                        NULL, 1, &errorCode);
      if (U_FAILURE(errorCode) || pLMBCSOut != (const char *)lmbcsString+3 || pUniIn != uniString+1 || uniString[0] != 0xFF6E)
      {
           log_err("Unexpected results from LMBCS-1 single byte mbcs char\n");
      }
      pLMBCSIn = (char *)lmbcsString;
      pUniOut = uniString;
      ucnv_fromUnicode (cnv16jp,
                        &pLMBCSIn, (const char *)(pLMBCSIn + 1),
                        &pUniOut, pUniOut + 1,
                        NULL, 1, &errorCode);
      if (U_FAILURE(errorCode) || pLMBCSIn != (const char *)lmbcsString+1 || pUniOut != uniString+1 || lmbcsString[0] != 0xAE)
      {
           log_err("Unexpected results to LMBCS-16 single byte mbcs char\n");
      }
      ucnv_close(cnv16he);
      ucnv_close(cnv16jp);
      ucnv_close(cnv01us);
    }
    {
       /* Small source buffer testing, LMBCS -> Unicode */

       UErrorCode errorCode=U_ZERO_ERROR;

       const char * pSource = (const char *)pszLMBCS;
       const char * sourceLimit = (const char *)pszLMBCS + sizeof(pszLMBCS);
       int codepointCount = 0;

       UChar Out [sizeof(pszUnicode) + 1];
       UChar * pOut = Out;
       UChar * OutLimit = Out + sizeof(pszUnicode)/sizeof(UChar);


       cnv = ucnv_open(NAME_LMBCS_1, &errorCode);
       if(U_FAILURE(errorCode)) {
           log_err("Unable to open a LMBCS-1 converter: %s\n", u_errorName(errorCode));
           return;
       }
       
       
       while ((pSource < sourceLimit) && U_SUCCESS (errorCode))
       {
           ucnv_toUnicode (cnv,
               &pOut,
               OutLimit,
               &pSource,
               (pSource+1), /* claim that this is a 1- byte buffer */
               NULL,
               FALSE,    /* FALSE means there might be more chars in the next buffer */
               &errorCode);
           
           if (U_SUCCESS (errorCode))
           {
               if ((pSource - (const char *)pszLMBCS) == offsets [codepointCount+1])
               {
                   /* we are on to the next code point: check value */
                   
                   if (Out[0] != pszUnicode[codepointCount]){
                       log_err("LMBCS->Uni result %lx should have been %lx \n",
                           Out[0], pszUnicode[codepointCount]);
                   }
                   
                   pOut = Out; /* reset for accumulating next code point */
                   codepointCount++;
               }
           }
           else
           {
               log_err("Unexpected Error on toUnicode: %s\n", u_errorName(errorCode));
           }
       }
       {
         /* limits & surrogate error testing */
         char LIn [sizeof(pszLMBCS)];
         const char * pLIn = LIn;

         char LOut [sizeof(pszLMBCS)];
         char * pLOut = LOut;

         UChar UOut [sizeof(pszUnicode)];
         UChar * pUOut = UOut;

         UChar UIn [sizeof(pszUnicode)];
         const UChar * pUIn = UIn;

         int32_t off [sizeof(offsets)];
         UChar32 uniChar;

         errorCode=U_ZERO_ERROR;

         /* negative source request should always return U_ILLEGAL_ARGUMENT_ERROR */
         pUIn++;
         ucnv_fromUnicode(cnv, &pLOut, pLOut+1, &pUIn, pUIn-1, off, FALSE, &errorCode);
         if (errorCode != U_ILLEGAL_ARGUMENT_ERROR)
         {
            log_err("Unexpected Error on negative source request to ucnv_fromUnicode: %s\n", u_errorName(errorCode));
         }
         pUIn--;
         
         errorCode=U_ZERO_ERROR;
         ucnv_toUnicode(cnv, &pUOut,pUOut+1,(const char **)&pLIn,(const char *)(pLIn-1),off,FALSE, &errorCode);
         if (errorCode != U_ILLEGAL_ARGUMENT_ERROR)
         {
            log_err("Unexpected Error on negative source request to ucnv_toUnicode: %s\n", u_errorName(errorCode));
         }
         errorCode=U_ZERO_ERROR;

         uniChar = ucnv_getNextUChar(cnv, (const char **)&pLIn, (const char *)(pLIn-1), &errorCode);
         if (errorCode != U_ILLEGAL_ARGUMENT_ERROR)
         {
            log_err("Unexpected Error on negative source request to ucnv_getNextUChar: %s\n", u_errorName(errorCode));
         }
         errorCode=U_ZERO_ERROR;

         /* 0 byte source request - no error, no pointer movement */
         ucnv_toUnicode(cnv, &pUOut,pUOut+1,(const char **)&pLIn,(const char *)pLIn,off,FALSE, &errorCode);
         ucnv_fromUnicode(cnv, &pLOut,pLOut+1,&pUIn,pUIn,off,FALSE, &errorCode);
         if(U_FAILURE(errorCode)) {
            log_err("0 byte source request: unexpected error: %s\n", u_errorName(errorCode));
         }
         if ((pUOut != UOut) || (pUIn != UIn) || (pLOut != LOut) || (pLIn != LIn))
         {
              log_err("Unexpected pointer move in 0 byte source request \n");
         }
         /*0 byte source request - GetNextUChar : error & value == fffe or ffff */
         uniChar = ucnv_getNextUChar(cnv, (const char **)&pLIn, (const char *)pLIn, &errorCode);
         if (errorCode != U_INDEX_OUTOFBOUNDS_ERROR)
         {
            log_err("Unexpected Error on 0-byte source request to ucnv_getnextUChar: %s\n", u_errorName(errorCode));
         }
         if (((uint32_t)uniChar - 0xfffe) > 1) /* not 0xfffe<=uniChar<=0xffff */
         {
            log_err("Unexpected value on 0-byte source request to ucnv_getnextUChar \n");
         }
         errorCode = U_ZERO_ERROR;

         /* running out of target room : U_BUFFER_OVERFLOW_ERROR */

         pUIn = pszUnicode;
         ucnv_fromUnicode(cnv, &pLOut,pLOut+offsets[4],&pUIn,pUIn+sizeof(pszUnicode)/sizeof(UChar),off,FALSE, &errorCode);
         if (errorCode != U_BUFFER_OVERFLOW_ERROR || pLOut != LOut + offsets[4] || pUIn != pszUnicode+4 )
         {
            log_err("Unexpected results on out of target room to ucnv_fromUnicode\n");
         }

         errorCode = U_ZERO_ERROR;

         pLIn = (const char *)pszLMBCS;
         ucnv_toUnicode(cnv, &pUOut,pUOut+4,&pLIn,(pLIn+sizeof(pszLMBCS)),off,FALSE, &errorCode);
         if (errorCode != U_BUFFER_OVERFLOW_ERROR || pUOut != UOut + 4 || pLIn != (const char *)pszLMBCS+offsets[4])
         {
            log_err("Unexpected results on out of target room to ucnv_toUnicode\n");
         }

         /* unpaired or chopped LMBCS surrogates */

         /* OK high surrogate, Low surrogate is chopped */
         LIn [0] = (char)0x14;
         LIn [1] = (char)0xD8;
         LIn [2] = (char)0x01;
         LIn [3] = (char)0x14;
         LIn [4] = (char)0xDC;
         pLIn = LIn;
         errorCode = U_ZERO_ERROR;
         pUOut = UOut;

         ucnv_setToUCallBack(cnv, UCNV_TO_U_CALLBACK_STOP, NULL, NULL, NULL, &errorCode);
         ucnv_toUnicode(cnv, &pUOut,pUOut+sizeof(UOut)/sizeof(UChar),(const char **)&pLIn,(const char *)(pLIn+5),off,TRUE, &errorCode);
         if (UOut[0] != 0xD801 || errorCode != U_TRUNCATED_CHAR_FOUND || pUOut != UOut + 1 || pLIn != LIn + 5)
         {
            log_err("Unexpected results on chopped low surrogate\n");
         }

         /* chopped at surrogate boundary */
         LIn [0] = (char)0x14;
         LIn [1] = (char)0xD8;
         LIn [2] = (char)0x01;
         pLIn = LIn;
         errorCode = U_ZERO_ERROR;
         pUOut = UOut;

         ucnv_toUnicode(cnv, &pUOut,pUOut+sizeof(UOut)/sizeof(UChar),(const char **)&pLIn,(const char *)(pLIn+3),off,TRUE, &errorCode);
         if (UOut[0] != 0xD801 || U_FAILURE(errorCode) || pUOut != UOut + 1 || pLIn != LIn + 3)
         {
            log_err("Unexpected results on chopped at surrogate boundary \n");
         }

         /* unpaired surrogate plus valid Unichar */
         LIn [0] = (char)0x14;
         LIn [1] = (char)0xD8;
         LIn [2] = (char)0x01;
         LIn [3] = (char)0x14;
         LIn [4] = (char)0xC9;
         LIn [5] = (char)0xD0;
         pLIn = LIn;
         errorCode = U_ZERO_ERROR;
         pUOut = UOut;

         ucnv_toUnicode(cnv, &pUOut,pUOut+sizeof(UOut)/sizeof(UChar),(const char **)&pLIn,(const char *)(pLIn+6),off,TRUE, &errorCode);
         if (UOut[0] != 0xD801 || UOut[1] != 0xC9D0 || U_FAILURE(errorCode) || pUOut != UOut + 2 || pLIn != LIn + 6)
         {
            log_err("Unexpected results after unpaired surrogate plus valid Unichar \n");
         }

      /* unpaired surrogate plus chopped Unichar */
         LIn [0] = (char)0x14;
         LIn [1] = (char)0xD8;
         LIn [2] = (char)0x01;
         LIn [3] = (char)0x14;
         LIn [4] = (char)0xC9;

         pLIn = LIn;
         errorCode = U_ZERO_ERROR;
         pUOut = UOut;

         ucnv_toUnicode(cnv, &pUOut,pUOut+sizeof(UOut)/sizeof(UChar),(const char **)&pLIn,(const char *)(pLIn+5),off,TRUE, &errorCode);
         if (UOut[0] != 0xD801 || errorCode != U_TRUNCATED_CHAR_FOUND || pUOut != UOut + 1 || pLIn != LIn + 5)
         {
            log_err("Unexpected results after unpaired surrogate plus chopped Unichar \n");
         }

         /* unpaired surrogate plus valid non-Unichar */
         LIn [0] = (char)0x14;
         LIn [1] = (char)0xD8;
         LIn [2] = (char)0x01;
         LIn [3] = (char)0x0F;
         LIn [4] = (char)0x3B;

         pLIn = LIn;
         errorCode = U_ZERO_ERROR;
         pUOut = UOut;

         ucnv_toUnicode(cnv, &pUOut,pUOut+sizeof(UOut)/sizeof(UChar),(const char **)&pLIn,(const char *)(pLIn+5),off,TRUE, &errorCode);
         if (UOut[0] != 0xD801 || UOut[1] != 0x1B || U_FAILURE(errorCode) || pUOut != UOut + 2 || pLIn != LIn + 5)
         {
            log_err("Unexpected results after unpaired surrogate plus valid non-Unichar\n");
         }

         /* unpaired surrogate plus chopped non-Unichar */
         LIn [0] = (char)0x14;
         LIn [1] = (char)0xD8;
         LIn [2] = (char)0x01;
         LIn [3] = (char)0x0F;

         pLIn = LIn;
         errorCode = U_ZERO_ERROR;
         pUOut = UOut;

         ucnv_toUnicode(cnv, &pUOut,pUOut+sizeof(UOut)/sizeof(UChar),(const char **)&pLIn,(const char *)(pLIn+4),off,TRUE, &errorCode);

         if (UOut[0] != 0xD801 || errorCode != U_TRUNCATED_CHAR_FOUND || pUOut != UOut + 1 || pLIn != LIn + 4)
         {
            log_err("Unexpected results after unpaired surrogate plus chopped non-Unichar\n");
         }
       }
    }
   ucnv_close(cnv);  /* final cleanup */
}


static void TestJitterbug255()
{
    static const uint8_t testBytes[] = { 0x95, 0xcf, 0x8a, 0xb7, 0x0d, 0x0a, 0x00 };
    const char *testBuffer = (const char *)testBytes;
    const char *testEnd = (const char *)testBytes + sizeof(testBytes);
    UErrorCode status = U_ZERO_ERROR;
    /*UChar32 result;*/
    UConverter *cnv = 0;

    cnv = ucnv_open("shift-jis", &status);
    if (U_FAILURE(status) || cnv == 0) {
        log_data_err("Failed to open the converter for SJIS.\n");
                return;
    }
    while (testBuffer != testEnd)
    {
        /*result = */ucnv_getNextUChar (cnv, &testBuffer, testEnd , &status);
        if (U_FAILURE(status))
        {
            log_err("Failed to convert the next UChar for SJIS.\n");
            break;
        }
    }
    ucnv_close(cnv);
}

static void TestEBCDICUS4XML()
{
    UChar unicodes_x[] = {0x0000, 0x0000, 0x0000, 0x0000};
    static const UChar toUnicodeMaps_x[] = {0x000A, 0x000A, 0x000D, 0x0000};
    static const char fromUnicodeMaps_x[] = {0x25, 0x25, 0x0D, 0x00};
    static const char newLines_x[] = {0x25, 0x15, 0x0D, 0x00};
    char target_x[] = {0x00, 0x00, 0x00, 0x00};
    UChar *unicodes = unicodes_x;
    const UChar *toUnicodeMaps = toUnicodeMaps_x;
    char *target = target_x;
    const char* fromUnicodeMaps = fromUnicodeMaps_x, *newLines = newLines_x;
    UErrorCode status = U_ZERO_ERROR;
    UConverter *cnv = 0;

    cnv = ucnv_open("ebcdic-xml-us", &status);
    if (U_FAILURE(status) || cnv == 0) {
        log_data_err("Failed to open the converter for EBCDIC-XML-US.\n");
        return;
    }
    ucnv_toUnicode(cnv, &unicodes, unicodes+3, (const char**)&newLines, newLines+3, NULL, TRUE, &status);
    if (U_FAILURE(status) || memcmp(unicodes_x, toUnicodeMaps, sizeof(UChar)*3) != 0) {
        log_err("To Unicode conversion failed in EBCDICUS4XML test. %s\n",
            u_errorName(status));
        printUSeqErr(unicodes_x, 3);
        printUSeqErr(toUnicodeMaps, 3);
    }
    status = U_ZERO_ERROR;
    ucnv_fromUnicode(cnv, &target, target+3, (const UChar**)&toUnicodeMaps, toUnicodeMaps+3, NULL, TRUE, &status);
    if (U_FAILURE(status) || memcmp(target_x, fromUnicodeMaps, sizeof(char)*3) != 0) {
        log_err("From Unicode conversion failed in EBCDICUS4XML test. %s\n",
            u_errorName(status));
        printSeqErr((const unsigned char*)target_x, 3);
        printSeqErr((const unsigned char*)fromUnicodeMaps, 3);
    }
    ucnv_close(cnv);
}
#endif /* #if !UCONFIG_NO_LEGACY_COLLATION */

#if !UCONFIG_NO_COLLATION

static void TestJitterbug981(){
    const UChar* rules;
    int32_t rules_length, target_cap, bytes_needed, buff_size;
    UErrorCode status = U_ZERO_ERROR;
    UConverter *utf8cnv;
    UCollator* myCollator;
    char *buff;
    int numNeeded=0;
    utf8cnv = ucnv_open ("utf8", &status);
    if(U_FAILURE(status)){
        log_err("Could not open UTF-8 converter. Error: %s\n", u_errorName(status));
        return;
    }
    myCollator = ucol_open("zh", &status);
    if(U_FAILURE(status)){
        log_data_err("Could not open collator for zh locale. Error: %s\n", u_errorName(status));
        ucnv_close(utf8cnv);
        return;
    }

    rules = ucol_getRules(myCollator, &rules_length);
    buff_size = rules_length * ucnv_getMaxCharSize(utf8cnv);
    buff = malloc(buff_size);

    target_cap = 0;
    do {
        ucnv_reset(utf8cnv);
        status = U_ZERO_ERROR;
        if(target_cap >= buff_size) {
            log_err("wanted %d bytes, only %d available\n", target_cap, buff_size);
            break;
        }
        bytes_needed = ucnv_fromUChars(utf8cnv, buff, target_cap,
            rules, rules_length, &status);
        target_cap = (bytes_needed > target_cap) ? bytes_needed : target_cap +1;
        if(numNeeded!=0 && numNeeded!= bytes_needed){
            log_err("ucnv_fromUChars returns different values for required capacity in pre-flight and conversion modes");
            break;
        }
        numNeeded = bytes_needed;
    } while (status == U_BUFFER_OVERFLOW_ERROR);
    ucol_close(myCollator);
    ucnv_close(utf8cnv);
    free(buff);
}

#endif

static void TestJitterbug1293(){
    static const UChar src[] = {0x30DE, 0x30A4, 0x5E83, 0x544A, 0x30BF, 0x30A4, 0x30D7,0x000};
    char target[256];
    UErrorCode status = U_ZERO_ERROR;
    UConverter* conv=NULL;
    int32_t target_cap, bytes_needed, numNeeded = 0;
    conv = ucnv_open("shift-jis",&status);
    if(U_FAILURE(status)){
      log_data_err("Could not open Shift-Jis converter. Error: %s", u_errorName(status));
      return;
    }

    do{
        target_cap =0;
        bytes_needed = ucnv_fromUChars(conv,target,256,src,u_strlen(src),&status);
        target_cap = (bytes_needed > target_cap) ? bytes_needed : target_cap +1;
        if(numNeeded!=0 && numNeeded!= bytes_needed){
          log_err("ucnv_fromUChars returns different values for required capacity in pre-flight and conversion modes");
        }
        numNeeded = bytes_needed;
    } while (status == U_BUFFER_OVERFLOW_ERROR);
    if(U_FAILURE(status)){
      log_err("An error occured in ucnv_fromUChars. Error: %s", u_errorName(status));
      return;
    }
    ucnv_close(conv);
}
static void TestJB5275_1(){

    static const char* data = "\x3B\xB3\x0A" /* Easy characters */
                                "\xC0\xE9\xBF\xE9\xE8\xD8\x0A" /* Gurmukhi test */
                                /* Switch script: */
                                "\xEF\x43\xC0\xE9\xBF\xE9\xE8\xD8\x0A" /* Bengali test */
                                "\x3B\xB3\x0A" /* Easy characters - new line, so should default!*/
                                "\xEF\x40\x3B\xB3\x0A";
    static const UChar expected[] ={ 
            0x003b, 0x0a15, 0x000a, /* Easy characters */
            0x0a5c, 0x0a4d, 0x0a39, 0x0a5c, 0x0a4d, 0x0a39, 0x000a, /* Gurmukhi test */
            0x09dd, 0x09dc, 0x09cd, 0x09b9, 0x000a, /* Switch script: to Bengali*/ 
            0x003b, 0x0a15, 0x000a, /* Easy characters - new line, so should default!*/
            0x003b, 0x0a15, 0x000a /* Back to Gurmukhi*/
    };
        
    UErrorCode status = U_ZERO_ERROR;
    UConverter* conv = ucnv_open("iscii-gur", &status);
    UChar dest[100] = {'\0'};
    UChar* target = dest;
    UChar* targetLimit = dest+100;
    const char* source = data;
    const char* sourceLimit = data+strlen(data);
    const UChar* exp = expected;
    
    if (U_FAILURE(status)) {
        log_data_err("Unable to open converter: iscii-gur got errorCode: %s\n", u_errorName(status));
        return;
    }
    
    log_verbose("Testing switching back to default script when new line is encountered.\n");
    ucnv_toUnicode(conv, &target, targetLimit, &source, sourceLimit, NULL, TRUE, &status);
    if(U_FAILURE(status)){
        log_err("conversion failed: %s \n", u_errorName(status));
    }
    targetLimit = target;
    target = dest;
    printUSeq(target, targetLimit-target);
    while(target<targetLimit){
        if(*exp!=*target){
            log_err("did not get the expected output. \\u%04X != \\u%04X (got)\n", *exp, *target);
        }
        target++;
        exp++;
    }
    ucnv_close(conv);
}

static void TestJB5275(){
    static const char* data = 
    /* "\xEF\x42\xEF\x41\xA4\xD5\xE5\xB3\xEA\x0A"  unsupported sequence \xEF\x41 */
    /* "\xEF\x42\xEF\x41\xD4\xDA\xB3\xE8\xEA\x0A"  unsupported sequence \xEF\x41  */
    /* "\xEF\x44\xEF\x41\xC8\xE1\x8B\xDB\xB3\xE8 \xB3\xE4\xC1\xE8\x0A"  unsupported sequence \xEF\x41 */
        "\xEF\x4B\xC0\xE9\xBF\xE9\xE8\xD8\x0A"  /* Gurmukhi test */
        "\xEF\x4A\xC0\xD4\xBF\xD4\xE8\xD8\x0A"  /* Gujarati test */
        "\xEF\x48\x38\xB3\x0A"  /* Kannada test */
        "\xEF\x49\x39\xB3\x0A"  /* Malayalam test */
        "\xEF\x4A\x3A\xB3\x0A"  /* Gujarati test */
        "\xEF\x4B\x3B\xB3\x0A"  /* Punjabi test */
        /* "\xEF\x4C\x3C\xB3\x0A"  unsupported sequence \xEF\x41 */;
    static const UChar expected[] ={ 
        0x0A5C, 0x0A4D, 0x0A39, 0x0A5C, 0x0A4D, 0x0A39, 0x000A, /* Gurmukhi test */
        0x0AA2, 0x0AB5, 0x0AA1, 0x0AB5, 0x0ACD, 0x0AB9, 0x000A,     /* Gujarati test */
        0x0038, 0x0C95, 0x000A, /* Kannada test */
        0x0039, 0x0D15, 0x000A, /* Malayalam test */
        0x003A, 0x0A95, 0x000A, /* Gujarati test */
        0x003B, 0x0A15, 0x000A, /* Punjabi test */
    };
        
    UErrorCode status = U_ZERO_ERROR;
    UConverter* conv = ucnv_open("iscii", &status);
    UChar dest[100] = {'\0'};
    UChar* target = dest;
    UChar* targetLimit = dest+100;
    const char* source = data;
    const char* sourceLimit = data+strlen(data);
    const UChar* exp = expected;
    ucnv_toUnicode(conv, &target, targetLimit, &source, sourceLimit, NULL, TRUE, &status);
    if(U_FAILURE(status)){
        log_err("conversion failed: %s \n", u_errorName(status));
    }
    targetLimit = target;
    target = dest;

    printUSeq(target, targetLimit-target);
    
    while(target<targetLimit){
        if(*exp!=*target){
            log_err("did not get the expected output. \\u%04X != \\u%04X (got)\n", *exp, *target);
        }
        target++;
        exp++;
    }
    ucnv_close(conv);
}
