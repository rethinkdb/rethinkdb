/********************************************************************
 * COPYRIGHT: 
 * Copyright (c) 1997-2001, International Business Machines Corporation and
 * others. All Rights Reserved.
 ********************************************************************/
/********************************************************************************
*
* File NCCBTST.H
*
* Modification History:
*        Name                     Description            
*     Madhu Katragadda           creation
*********************************************************************************
*/
#ifndef _NCCBTST
#define _NCCBTST
/* C API TEST FOR CALL BACK ROUTINES OF CODESET CONVERSION COMPONENT */
#include "cintltst.h"
#include "unicode/utypes.h"


static void TestSkipCallBack(void);
static void TestStopCallBack(void);
static void TestSubCallBack(void);
static void TestSubWithValueCallBack(void);
static void TestLegalAndOtherCallBack(void);
static void TestSingleByteCallBack(void);


static void TestSkip(int32_t inputsize, int32_t outputsize);

static void TestStop(int32_t inputsize, int32_t outputsize);

static void TestSub(int32_t inputsize, int32_t outputsize);

static void TestSubWithValue(int32_t inputsize, int32_t outputsize);

static void TestLegalAndOthers(int32_t inputsize, int32_t outputsize);
static void TestSingleByte(int32_t inputsize, int32_t outputsize);
static void TestEBCDIC_STATEFUL_Sub(int32_t inputsize, int32_t outputsize);

/* Following will return FALSE *only* on a mismach. They will return TRUE on any other error OR success, because
 * the error would have been emitted to log_err separately. */

UBool testConvertFromUnicode(const UChar *source, int sourceLen,  const uint8_t *expect, int expectLen, 
                const char *codepage, UConverterFromUCallback callback, const int32_t *expectOffsets,
                const char *mySubChar, int8_t len);


UBool testConvertToUnicode( const uint8_t *source, int sourcelen, const UChar *expect, int expectlen, 
               const char *codepage, UConverterToUCallback callback, const int32_t *expectOffsets,
               const char *mySubChar, int8_t len);

UBool testConvertFromUnicodeWithContext(const UChar *source, int sourceLen,  const uint8_t *expect, int expectLen, 
                const char *codepage, UConverterFromUCallback callback , const int32_t *expectOffsets, 
                const char *mySubChar, int8_t len, const void* context, UErrorCode expectedError);

UBool testConvertToUnicodeWithContext( const uint8_t *source, int sourcelen, const UChar *expect, int expectlen, 
               const char *codepage, UConverterToUCallback callback, const int32_t *expectOffsets,
               const char *mySubChar, int8_t len, const void* context, UErrorCode expectedError);

static void printSeq(const uint8_t* a, int len);
static void printUSeq(const UChar* a, int len);
static void printSeqErr(const uint8_t* a, int len);
static void printUSeqErr(const UChar* a, int len);
static void setNuConvTestName(const char *codepage, const char *direction);


#endif
