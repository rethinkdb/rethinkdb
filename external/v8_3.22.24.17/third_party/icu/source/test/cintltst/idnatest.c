/*
 *******************************************************************************
 *
 *   Copyright (C) 2003-2010, International Business Machines
 *   Corporation and others.  All Rights Reserved.
 *
 *******************************************************************************
 *   file name:  idnatest.c
 *   encoding:   US-ASCII
 *   tab size:   8 (not used)
 *   indentation:4
 *
 *   created on: 2003jul11
 *   created by: Ram Viswanadha
 */
#include <stdlib.h>
#include <string.h>
#include "unicode/utypes.h"

#if !UCONFIG_NO_IDNA

#include "unicode/ustring.h"
#include "unicode/uidna.h"
#include "cintltst.h"



#define LENGTHOF(array) (int32_t)(sizeof(array)/sizeof((array)[0]))
#define MAX_DEST_SIZE 1000

static void TestToUnicode(void);
static void TestToASCII(void);
static void TestIDNToUnicode(void);
static void TestIDNToASCII(void);
static void TestCompare(void);
static void TestJB4490(void);
static void TestJB4475(void); 
static void TestLength(void);
static void TestJB5273(void);
static void TestUTS46(void);

void addIDNATest(TestNode** root);


typedef int32_t
(U_EXPORT2 *TestFunc) (   const UChar *src, int32_t srcLength,
                UChar *dest, int32_t destCapacity,
                int32_t options, UParseError *parseError,
                UErrorCode *status);
typedef int32_t
(U_EXPORT2 *CompareFunc) (const UChar *s1, int32_t s1Len,
                const UChar *s2, int32_t s2Len,
                int32_t options,
                UErrorCode *status);


void
addIDNATest(TestNode** root)
{
   addTest(root, &TestToUnicode,    "idna/TestToUnicode");
   addTest(root, &TestToASCII,      "idna/TestToASCII");
   addTest(root, &TestIDNToUnicode, "idna/TestIDNToUnicode");
   addTest(root, &TestIDNToASCII,   "idna/TestIDNToASCII");
   addTest(root, &TestCompare,      "idna/TestCompare");
   addTest(root, &TestJB4490,       "idna/TestJB4490");
   addTest(root, &TestJB4475,       "idna/TestJB4475");
   addTest(root, &TestLength,       "idna/TestLength");
   addTest(root, &TestJB5273,       "idna/TestJB5273");
   addTest(root, &TestUTS46,        "idna/TestUTS46");
}

static void
testAPI(const UChar* src, const UChar* expected, const char* testName,
            UBool useSTD3ASCIIRules,UErrorCode expectedStatus,
            UBool doCompare, UBool testUnassigned,  TestFunc func){

    UErrorCode status = U_ZERO_ERROR;
    UChar destStack[MAX_DEST_SIZE];
    int32_t destLen = 0;
    UChar* dest = NULL;
    int32_t expectedLen = (expected != NULL) ? u_strlen(expected) : 0;
    int32_t options = (useSTD3ASCIIRules == TRUE) ? UIDNA_USE_STD3_RULES : UIDNA_DEFAULT;
    UParseError parseError;
    int32_t tSrcLen = 0;
    UChar* tSrc = NULL;

    if(src != NULL){
        tSrcLen = u_strlen(src);
        tSrc  =(UChar*) malloc( U_SIZEOF_UCHAR * tSrcLen );
        memcpy(tSrc,src,tSrcLen * U_SIZEOF_UCHAR);
    }

    /* test null-terminated source and return value of number of UChars required */

    destLen = func(src,-1,NULL,0,options, &parseError , &status);
    if(status == U_BUFFER_OVERFLOW_ERROR){
        status = U_ZERO_ERROR; /* reset error code */
        if(destLen+1 < MAX_DEST_SIZE){
            dest = destStack;
            destLen = func(src,-1,dest,destLen+1,options, &parseError, &status);
            /* TODO : compare output with expected */
            if(U_SUCCESS(status) && expectedStatus != U_IDNA_STD3_ASCII_RULES_ERROR&& (doCompare==TRUE) && u_strCaseCompare(dest,destLen, expected,expectedLen,0,&status)!=0){
                log_err("Did not get the expected result for  null terminated source.\n" );
            }
        }else{
            log_err( "%s null terminated source failed. Requires destCapacity > 300\n",testName);
        }
    }

    if(status != expectedStatus){
        log_err_status(status,  "Did not get the expected error for %s null terminated source failed. Expected: %s Got: %s\n",testName, u_errorName(expectedStatus), u_errorName(status));
        free(tSrc);
        return;
    }
    if(testUnassigned ){
        status = U_ZERO_ERROR;
        destLen = func(src,-1,NULL,0,options | UIDNA_ALLOW_UNASSIGNED, &parseError, &status);
        if(status == U_BUFFER_OVERFLOW_ERROR){
            status = U_ZERO_ERROR; /* reset error code */
            if(destLen+1 < MAX_DEST_SIZE){
                dest = destStack;
                destLen = func(src,-1,dest,destLen+1,options | UIDNA_ALLOW_UNASSIGNED, &parseError, &status);
                /* TODO : compare output with expected */
                if(U_SUCCESS(status) && (doCompare==TRUE) && u_strCaseCompare(dest,destLen, expected,expectedLen,0,&status)!=0){
                    log_err("Did not get the expected result for %s null terminated source with both options set.\n",testName);

                }
            }else{
                log_err( "%s null terminated source failed. Requires destCapacity > 300\n",testName);
            }
        }
        /*testing query string*/
        if(status != expectedStatus && expectedStatus != U_IDNA_UNASSIGNED_ERROR){
            log_err( "Did not get the expected error for %s null terminated source with options set. Expected: %s Got: %s\n",testName, u_errorName(expectedStatus), u_errorName(status));
        }
    }

    status = U_ZERO_ERROR;

    /* test source with lengthand return value of number of UChars required*/
    destLen = func(tSrc, tSrcLen, NULL,0,options, &parseError, &status);
    if(status == U_BUFFER_OVERFLOW_ERROR){
        status = U_ZERO_ERROR; /* reset error code */
        if(destLen+1 < MAX_DEST_SIZE){
            dest = destStack;
            destLen = func(src,u_strlen(src),dest,destLen+1,options, &parseError, &status);
            /* TODO : compare output with expected */
            if(U_SUCCESS(status) && (doCompare==TRUE) && u_strCaseCompare(dest,destLen, expected,expectedLen,0,&status)!=0){
                log_err("Did not get the expected result for %s with source length.\n",testName);
            }
        }else{
            log_err( "%s with source length  failed. Requires destCapacity > 300\n",testName);
        }
    }

    if(status != expectedStatus){
        log_err( "Did not get the expected error for %s with source length. Expected: %s Got: %s\n",testName, u_errorName(expectedStatus), u_errorName(status));
    }
    if(testUnassigned){
        status = U_ZERO_ERROR;

        destLen = func(tSrc,tSrcLen,NULL,0,options | UIDNA_ALLOW_UNASSIGNED, &parseError, &status);

        if(status == U_BUFFER_OVERFLOW_ERROR){
            status = U_ZERO_ERROR; /* reset error code */
            if(destLen+1 < MAX_DEST_SIZE){
                dest = destStack;
                destLen = func(src,u_strlen(src),dest,destLen+1,options | UIDNA_ALLOW_UNASSIGNED, &parseError, &status);
                /* TODO : compare output with expected */
                if(U_SUCCESS(status) && (doCompare==TRUE) && u_strCaseCompare(dest,destLen, expected,expectedLen,0,&status)!=0){
                    log_err("Did not get the expected result for %s with source length and both options set.\n",testName);
                }
            }else{
                log_err( "%s with source length  failed. Requires destCapacity > 300\n",testName);
            }
        }
        /*testing query string*/
        if(status != expectedStatus && expectedStatus != U_IDNA_UNASSIGNED_ERROR){
            log_err( "Did not get the expected error for %s with source length and options set. Expected: %s Got: %s\n",testName, u_errorName(expectedStatus), u_errorName(status));
        }
    }

    status = U_ZERO_ERROR;
    destLen = func(src,-1,NULL,0,options | UIDNA_USE_STD3_RULES, &parseError, &status);
    if(status == U_BUFFER_OVERFLOW_ERROR){
        status = U_ZERO_ERROR; /* reset error code*/
        if(destLen+1 < MAX_DEST_SIZE){
            dest = destStack;
            destLen = func(src,-1,dest,destLen+1,options | UIDNA_USE_STD3_RULES, &parseError, &status);
            /* TODO : compare output with expected*/
            if(U_SUCCESS(status) && (doCompare==TRUE) && u_strCaseCompare(dest,destLen, expected,expectedLen,0,&status)!=0){
                log_err("Did not get the expected result for %s null terminated source with both options set.\n",testName);

            }
        }else{
            log_err( "%s null terminated source failed. Requires destCapacity > 300\n",testName);
        }
    }
    /*testing query string*/
    if(status != expectedStatus){
        log_err( "Did not get the expected error for %s null terminated source with options set. Expected: %s Got: %s\n",testName, u_errorName(expectedStatus), u_errorName(status));
    }

    status = U_ZERO_ERROR;

    destLen = func(tSrc,tSrcLen,NULL,0,options | UIDNA_USE_STD3_RULES, &parseError, &status);

    if(status == U_BUFFER_OVERFLOW_ERROR){
        status = U_ZERO_ERROR; /* reset error code*/
        if(destLen+1 < MAX_DEST_SIZE){
            dest = destStack;
            destLen = func(src,u_strlen(src),dest,destLen+1,options | UIDNA_USE_STD3_RULES, &parseError, &status);
            /* TODO : compare output with expected*/
            if(U_SUCCESS(status) && (doCompare==TRUE) && u_strCaseCompare(dest,destLen, expected,expectedLen,0,&status)!=0){
                log_err("Did not get the expected result for %s with source length and both options set.\n",testName);
            }
        }else{
            log_err( "%s with source length  failed. Requires destCapacity > 300\n",testName);
        }
    }
    /*testing query string*/
    if(status != expectedStatus && expectedStatus != U_IDNA_UNASSIGNED_ERROR){
        log_err( "Did not get the expected error for %s with source length and options set. Expected: %s Got: %s\n",testName, u_errorName(expectedStatus), u_errorName(status));
    }
    free(tSrc);
}

static const UChar unicodeIn[][41] ={
    {
        0x0644, 0x064A, 0x0647, 0x0645, 0x0627, 0x0628, 0x062A, 0x0643, 0x0644,
        0x0645, 0x0648, 0x0634, 0x0639, 0x0631, 0x0628, 0x064A, 0x061F, 0x0000
    },
    {
        0x4ED6, 0x4EEC, 0x4E3A, 0x4EC0, 0x4E48, 0x4E0D, 0x8BF4, 0x4E2D, 0x6587,
        0x0000
    },
    {
        0x0050, 0x0072, 0x006F, 0x010D, 0x0070, 0x0072, 0x006F, 0x0073, 0x0074,
        0x011B, 0x006E, 0x0065, 0x006D, 0x006C, 0x0075, 0x0076, 0x00ED, 0x010D,
        0x0065, 0x0073, 0x006B, 0x0079, 0x0000
    },
    {
        0x05DC, 0x05DE, 0x05D4, 0x05D4, 0x05DD, 0x05E4, 0x05E9, 0x05D5, 0x05D8,
        0x05DC, 0x05D0, 0x05DE, 0x05D3, 0x05D1, 0x05E8, 0x05D9, 0x05DD, 0x05E2,
        0x05D1, 0x05E8, 0x05D9, 0x05EA, 0x0000
    },
    {
        0x092F, 0x0939, 0x0932, 0x094B, 0x0917, 0x0939, 0x093F, 0x0928, 0x094D,
        0x0926, 0x0940, 0x0915, 0x094D, 0x092F, 0x094B, 0x0902, 0x0928, 0x0939,
        0x0940, 0x0902, 0x092C, 0x094B, 0x0932, 0x0938, 0x0915, 0x0924, 0x0947,
        0x0939, 0x0948, 0x0902, 0x0000
    },
    {
        0x306A, 0x305C, 0x307F, 0x3093, 0x306A, 0x65E5, 0x672C, 0x8A9E, 0x3092,
        0x8A71, 0x3057, 0x3066, 0x304F, 0x308C, 0x306A, 0x3044, 0x306E, 0x304B,
        0x0000
    },
/*
    {
        0xC138, 0xACC4, 0xC758, 0xBAA8, 0xB4E0, 0xC0AC, 0xB78C, 0xB4E4, 0xC774,
        0xD55C, 0xAD6D, 0xC5B4, 0xB97C, 0xC774, 0xD574, 0xD55C, 0xB2E4, 0xBA74,
        0xC5BC, 0xB9C8, 0xB098, 0xC88B, 0xC744, 0xAE4C, 0x0000
    },
*/
    {
        0x043F, 0x043E, 0x0447, 0x0435, 0x043C, 0x0443, 0x0436, 0x0435, 0x043E,
        0x043D, 0x0438, 0x043D, 0x0435, 0x0433, 0x043E, 0x0432, 0x043E, 0x0440,
        0x044F, 0x0442, 0x043F, 0x043E, 0x0440, 0x0443, 0x0441, 0x0441, 0x043A,
        0x0438, 0x0000
    },
    {
        0x0050, 0x006F, 0x0072, 0x0071, 0x0075, 0x00E9, 0x006E, 0x006F, 0x0070,
        0x0075, 0x0065, 0x0064, 0x0065, 0x006E, 0x0073, 0x0069, 0x006D, 0x0070,
        0x006C, 0x0065, 0x006D, 0x0065, 0x006E, 0x0074, 0x0065, 0x0068, 0x0061,
        0x0062, 0x006C, 0x0061, 0x0072, 0x0065, 0x006E, 0x0045, 0x0073, 0x0070,
        0x0061, 0x00F1, 0x006F, 0x006C, 0x0000
    },
    {
        0x4ED6, 0x5011, 0x7232, 0x4EC0, 0x9EBD, 0x4E0D, 0x8AAA, 0x4E2D, 0x6587,
        0x0000
    },
    {
        0x0054, 0x1EA1, 0x0069, 0x0073, 0x0061, 0x006F, 0x0068, 0x1ECD, 0x006B,
        0x0068, 0x00F4, 0x006E, 0x0067, 0x0074, 0x0068, 0x1EC3, 0x0063, 0x0068,
        0x1EC9, 0x006E, 0x00F3, 0x0069, 0x0074, 0x0069, 0x1EBF, 0x006E, 0x0067,
        0x0056, 0x0069, 0x1EC7, 0x0074, 0x0000
    },
    {
        0x0033, 0x5E74, 0x0042, 0x7D44, 0x91D1, 0x516B, 0x5148, 0x751F, 0x0000
    },
    {
        0x5B89, 0x5BA4, 0x5948, 0x7F8E, 0x6075, 0x002D, 0x0077, 0x0069, 0x0074,
        0x0068, 0x002D, 0x0053, 0x0055, 0x0050, 0x0045, 0x0052, 0x002D, 0x004D,
        0x004F, 0x004E, 0x004B, 0x0045, 0x0059, 0x0053, 0x0000
    },
    {
        0x0048, 0x0065, 0x006C, 0x006C, 0x006F, 0x002D, 0x0041, 0x006E, 0x006F,
        0x0074, 0x0068, 0x0065, 0x0072, 0x002D, 0x0057, 0x0061, 0x0079, 0x002D,
        0x305D, 0x308C, 0x305E, 0x308C, 0x306E, 0x5834, 0x6240, 0x0000
    },
    {
        0x3072, 0x3068, 0x3064, 0x5C4B, 0x6839, 0x306E, 0x4E0B, 0x0032, 0x0000
    },
    {
        0x004D, 0x0061, 0x006A, 0x0069, 0x3067, 0x004B, 0x006F, 0x0069, 0x3059,
        0x308B, 0x0035, 0x79D2, 0x524D, 0x0000
    },
    {
        0x30D1, 0x30D5, 0x30A3, 0x30FC, 0x0064, 0x0065, 0x30EB, 0x30F3, 0x30D0,
        0x0000
    },
    {
        0x305D, 0x306E, 0x30B9, 0x30D4, 0x30FC, 0x30C9, 0x3067, 0x0000
    },
    /* test non-BMP code points */
    {
        0xD800, 0xDF00, 0xD800, 0xDF01, 0xD800, 0xDF02, 0xD800, 0xDF03, 0xD800, 0xDF05,
        0xD800, 0xDF06, 0xD800, 0xDF07, 0xD800, 0xDF09, 0xD800, 0xDF0A, 0xD800, 0xDF0B,
        0x0000
    },
    {
        0xD800, 0xDF0D, 0xD800, 0xDF0C, 0xD800, 0xDF1E, 0xD800, 0xDF0F, 0xD800, 0xDF16,
        0xD800, 0xDF15, 0xD800, 0xDF14, 0xD800, 0xDF12, 0xD800, 0xDF10, 0xD800, 0xDF20,
        0xD800, 0xDF21,
        0x0000
    },
    /* Greek  */
    {
        0x03b5, 0x03bb, 0x03bb, 0x03b7, 0x03bd, 0x03b9, 0x03ba, 0x03ac
    },
    /* Maltese */
    {
        0x0062, 0x006f, 0x006e, 0x0121, 0x0075, 0x0073, 0x0061, 0x0127,
        0x0127, 0x0061
    },
    /* Russian */
    {
        0x043f, 0x043e, 0x0447, 0x0435, 0x043c, 0x0443, 0x0436, 0x0435,
        0x043e, 0x043d, 0x0438, 0x043d, 0x0435, 0x0433, 0x043e, 0x0432,
        0x043e, 0x0440, 0x044f, 0x0442, 0x043f, 0x043e, 0x0440, 0x0443,
        0x0441, 0x0441, 0x043a, 0x0438
    },
    {
        0x0054,0x0045,0x0053,0x0054
    }
};

static const char * const asciiIn[] = {
    "xn--egbpdaj6bu4bxfgehfvwxn",
    "xn--ihqwcrb4cv8a8dqg056pqjye",
    "xn--Proprostnemluvesky-uyb24dma41a",
    "xn--4dbcagdahymbxekheh6e0a7fei0b",
    "xn--i1baa7eci9glrd9b2ae1bj0hfcgg6iyaf8o0a1dig0cd",
    "xn--n8jok5ay5dzabd5bym9f0cm5685rrjetr6pdxa",
/*  "xn--989aomsvi5e83db1d2a355cv1e0vak1dwrv93d5xbh15a0dt30a5jpsd879ccm6fea98c",*/
    "xn--b1abfaaepdrnnbgefbaDotcwatmq2g4l",
    "xn--PorqunopuedensimplementehablarenEspaol-fmd56a",
    "xn--ihqwctvzc91f659drss3x8bo0yb",
    "xn--TisaohkhngthchnitingVit-kjcr8268qyxafd2f1b9g",
    "xn--3B-ww4c5e180e575a65lsy2b",
    "xn---with-SUPER-MONKEYS-pc58ag80a8qai00g7n9n",
    "xn--Hello-Another-Way--fc4qua05auwb3674vfr0b",
    "xn--2-u9tlzr9756bt3uc0v",
    "xn--MajiKoi5-783gue6qz075azm5e",
    "xn--de-jg4avhby1noc0d",
    "xn--d9juau41awczczp",
    "XN--097CCDEKGHQJK",
    "XN--db8CBHEJLGH4E0AL",
    "xn--hxargifdar",                       /* Greek */
    "xn--bonusaa-5bb1da",                   /* Maltese */
    "xn--b1abfaaepdrnnbgefbadotcwatmq2g4l",  /* Russian (Cyrillic)*/
    "TEST"

};

static const char * const domainNames[] = {
    "slip129-37-118-146.nc.us.ibm.net",
    "saratoga.pe.utexas.edu",
    "dial-120-45.ots.utexas.edu",
    "woo-085.dorms.waller.net",
    "hd30-049.hil.compuserve.com",
    "pem203-31.pe.ttu.edu",
    "56K-227.MaxTNT3.pdq.net",
    "dial-36-2.ots.utexas.edu",
    "slip129-37-23-152.ga.us.ibm.net",
    "ts45ip119.cadvision.com",
    "sdn-ts-004txaustP05.dialsprint.net",
    "bar-tnt1s66.erols.com",
    "101.st-louis-15.mo.dial-access.att.net",
    "h92-245.Arco.COM",
    "dial-13-2.ots.utexas.edu",
    "net-redynet29.datamarkets.com.ar",
    "ccs-shiva28.reacciun.net.ve",
    "7.houston-11.tx.dial-access.att.net",
    "ingw129-37-120-26.mo.us.ibm.net",
    "dialup6.austintx.com",
    "dns2.tpao.gov.tr",
    "slip129-37-119-194.nc.us.ibm.net",
    "cs7.dillons.co.uk.203.119.193.in-addr.arpa",
    "swprd1.innovplace.saskatoon.sk.ca",
    "bikini.bologna.maraut.it",
    "node91.subnet159-198-79.baxter.com",
    "cust19.max5.new-york.ny.ms.uu.net",
    "balexander.slip.andrew.cmu.edu",
    "pool029.max2.denver.co.dynip.alter.net",
    "cust49.max9.new-york.ny.ms.uu.net",
    "s61.abq-dialin2.hollyberry.com",
    "\\u0917\\u0928\\u0947\\u0936.sanjose.ibm.com", /*':'(0x003a) produces U_IDNA_STD3_ASCII_RULES_ERROR*/
    "www.xn--vea.com",
    /* "www.\\u00E0\\u00B3\\u00AF.com",//' ' (0x0020) produces U_IDNA_STD3_ASCII_RULES_ERROR*/
    "www.\\u00C2\\u00A4.com",
    "www.\\u00C2\\u00A3.com",
    /* "\\u0025", //'%' (0x0025) produces U_IDNA_STD3_ASCII_RULES_ERROR*/
    /* "\\u005C\\u005C", //'\' (0x005C) produces U_IDNA_STD3_ASCII_RULES_ERROR*/
    /*"@",*/
    /*"\\u002F",*/
    /*"www.\\u0021.com",*/
    /*"www.\\u0024.com",*/
    /*"\\u003f",*/
    /* These yeild U_IDNA_PROHIBITED_ERROR*/
    /*"\\u00CF\\u0082.com",*/
    /*"\\u00CE\\u00B2\\u00C3\\u009Fss.com",*/
    /*"\\u00E2\\u0098\\u00BA.com",*/
    "\\u00C3\\u00BC.com"

};

static void
TestToASCII(){

    int32_t i;
    UChar buf[MAX_DEST_SIZE];
    const char* testName = "uidna_toASCII";
    TestFunc func = uidna_toASCII;
    for(i=0;i< (int32_t)(sizeof(unicodeIn)/sizeof(unicodeIn[0])); i++){
        u_charsToUChars(asciiIn[i],buf, (int32_t)strlen(asciiIn[i])+1);
        testAPI(unicodeIn[i], buf,testName, FALSE,U_ZERO_ERROR, TRUE, TRUE, func);

    }
}

static void
TestToUnicode(){

    int32_t i;
    UChar buf[MAX_DEST_SIZE];
    const char* testName = "uidna_toUnicode";
    TestFunc func = uidna_toUnicode;
    for(i=0;i< (int32_t)(sizeof(asciiIn)/sizeof(asciiIn[0])); i++){
        u_charsToUChars(asciiIn[i],buf, (int32_t)strlen(asciiIn[i])+1);
        testAPI(buf,unicodeIn[i],testName,FALSE,U_ZERO_ERROR, TRUE, TRUE, func);
    }
}


static void
TestIDNToUnicode(){
    int32_t i;
    UChar buf[MAX_DEST_SIZE];
    UChar expected[MAX_DEST_SIZE];
    UErrorCode status = U_ZERO_ERROR;
    int32_t bufLen = 0;
    UParseError parseError;
    const char* testName="uidna_IDNToUnicode";
    TestFunc func = uidna_IDNToUnicode;
    for(i=0;i< (int32_t)(sizeof(domainNames)/sizeof(domainNames[0])); i++){
        bufLen = (int32_t)strlen(domainNames[i]);
        bufLen = u_unescape(domainNames[i],buf, bufLen+1);
        func(buf,bufLen,expected,MAX_DEST_SIZE, UIDNA_ALLOW_UNASSIGNED, &parseError,&status);
        if(U_FAILURE(status)){
            log_err_status(status,  "%s failed to convert domainNames[%i].Error: %s \n",testName, i, u_errorName(status));
            break;
        }
        testAPI(buf,expected,testName,FALSE,U_ZERO_ERROR, TRUE, TRUE, func);
         /*test toUnicode with all labels in the string*/
        testAPI(buf,expected,testName, FALSE,U_ZERO_ERROR, TRUE, TRUE, func);
        if(U_FAILURE(status)){
            log_err( "%s failed to convert domainNames[%i].Error: %s \n",testName,i, u_errorName(status));
            break;
        }
    }

}

static void
TestIDNToASCII(){
    int32_t i;
    UChar buf[MAX_DEST_SIZE];
    UChar expected[MAX_DEST_SIZE];
    UErrorCode status = U_ZERO_ERROR;
    int32_t bufLen = 0;
    UParseError parseError;
    const char* testName="udina_IDNToASCII";
    TestFunc func=uidna_IDNToASCII;

    for(i=0;i< (int32_t)(sizeof(domainNames)/sizeof(domainNames[0])); i++){
        bufLen = (int32_t)strlen(domainNames[i]);
        bufLen = u_unescape(domainNames[i],buf, bufLen+1);
        func(buf,bufLen,expected,MAX_DEST_SIZE, UIDNA_ALLOW_UNASSIGNED, &parseError,&status);
        if(U_FAILURE(status)){
            log_err_status(status,  "%s failed to convert domainNames[%i].Error: %s \n",testName,i, u_errorName(status));
            break;
        }
        testAPI(buf,expected,testName, FALSE,U_ZERO_ERROR, TRUE, TRUE, func);
        /*test toASCII with all labels in the string*/
        testAPI(buf,expected,testName, FALSE,U_ZERO_ERROR, FALSE, TRUE, func);
        if(U_FAILURE(status)){
            log_err( "%s failed to convert domainNames[%i].Error: %s \n",testName,i, u_errorName(status));
            break;
        }
    }


}


static void
testCompareWithSrc(const UChar* s1, int32_t s1Len,
            const UChar* s2, int32_t s2Len,
            const char* testName, CompareFunc func,
            UBool isEqual){

    UErrorCode status = U_ZERO_ERROR;
    int32_t retVal = func(s1,-1,s2,-1,UIDNA_DEFAULT,&status);

    if(isEqual==TRUE &&  retVal !=0){
        log_err("Did not get the expected result for %s with null termniated strings.\n",testName);
    }
    if(U_FAILURE(status)){
        log_err_status(status, "%s null terminated source failed. Error: %s\n", testName,u_errorName(status));
    }

    status = U_ZERO_ERROR;
    retVal = func(s1,-1,s2,-1,UIDNA_ALLOW_UNASSIGNED,&status);

    if(isEqual==TRUE &&  retVal !=0){
        log_err("Did not get the expected result for %s with null termniated strings with options set.\n", testName);
    }
    if(U_FAILURE(status)){
        log_err_status(status, "%s null terminated source and options set failed. Error: %s\n",testName, u_errorName(status));
    }

    status = U_ZERO_ERROR;
    retVal = func(s1,s1Len,s2,s2Len,UIDNA_DEFAULT,&status);

    if(isEqual==TRUE &&  retVal !=0){
        log_err("Did not get the expected result for %s with string length.\n",testName);
    }
    if(U_FAILURE(status)){
        log_err_status(status,  "%s with string length. Error: %s\n",testName, u_errorName(status));
    }

    status = U_ZERO_ERROR;
    retVal = func(s1,s1Len,s2,s2Len,UIDNA_ALLOW_UNASSIGNED,&status);

    if(isEqual==TRUE &&  retVal !=0){
        log_err("Did not get the expected result for %s with string length and options set.\n",testName);
    }
    if(U_FAILURE(status)){
        log_err_status(status,  "%s with string length and options set. Error: %s\n", u_errorName(status), testName);
    }
}


static void
TestCompare(){
    int32_t i;

    const char* testName ="uidna_compare";
    CompareFunc func = uidna_compare;

    UChar www[] = {0x0057, 0x0057, 0x0057, 0x002E, 0x0000};
    UChar com[] = {0x002E, 0x0043, 0x004F, 0x004D, 0x0000};
    UChar buf[MAX_DEST_SIZE]={0x0057, 0x0057, 0x0057, 0x002E, 0x0000};
    UChar source[MAX_DEST_SIZE]={0},
          uni0[MAX_DEST_SIZE]={0},
          uni1[MAX_DEST_SIZE]={0},
          ascii0[MAX_DEST_SIZE]={0},
          ascii1[MAX_DEST_SIZE]={0},
          temp[MAX_DEST_SIZE] ={0};


    u_strcat(uni0,unicodeIn[0]);
    u_strcat(uni0,com);

    u_strcat(uni1,unicodeIn[1]);
    u_strcat(uni1,com);

    u_charsToUChars(asciiIn[0], temp, (int32_t)strlen(asciiIn[0]));
    u_strcat(ascii0,temp);
    u_strcat(ascii0,com);

    memset(temp, 0, U_SIZEOF_UCHAR * MAX_DEST_SIZE);

    u_charsToUChars(asciiIn[1], temp, (int32_t)strlen(asciiIn[1]));
    u_strcat(ascii1,temp);
    u_strcat(ascii1,com);

    /* prepend www. */
    u_strcat(source, www);

    for(i=0;i< (int32_t)(sizeof(unicodeIn)/sizeof(unicodeIn[0])); i++){
        UChar* src;
        int32_t srcLen;

        memset(buf+4, 0, (MAX_DEST_SIZE-4) * U_SIZEOF_UCHAR);

        u_charsToUChars(asciiIn[i],buf+4, (int32_t)strlen(asciiIn[i]));
        u_strcat(buf,com);


        /* for every entry in unicodeIn array
           prepend www. and append .com*/
        source[4]=0;
        u_strncat(source,unicodeIn[i], u_strlen(unicodeIn[i]));
        u_strcat(source,com);

        /* a) compare it with itself*/
        src = source;
        srcLen = u_strlen(src);

        testCompareWithSrc(src,srcLen,src,srcLen,testName, func, TRUE);

        /* b) compare it with asciiIn equivalent */
        testCompareWithSrc(src,srcLen,buf,u_strlen(buf),testName, func,TRUE);

        /* c) compare it with unicodeIn not equivalent*/
        if(i==0){
            testCompareWithSrc(src,srcLen,uni1,u_strlen(uni1),testName, func,FALSE);
        }else{
            testCompareWithSrc(src,srcLen,uni0,u_strlen(uni0),testName, func,FALSE);
        }
        /* d) compare it with asciiIn not equivalent */
        if(i==0){
            testCompareWithSrc(src,srcLen,ascii1,u_strlen(ascii1),testName, func,FALSE);
        }else{
            testCompareWithSrc(src,srcLen,ascii0,u_strlen(ascii0),testName, func,FALSE);
        }

    }
}

static void TestJB4490(){
    static const UChar data[][50]= {
        {0x00F5,0x00dE,0x00dF,0x00dD, 0x0000},
        {0xFB00,0xFB01}
    };
    UChar output1[40] = {0};
    UChar output2[40] = {0};
    int32_t i;
    for(i=0; i< sizeof(data)/sizeof(data[0]); i++){
        const UChar* src1 = data[i];
        int32_t src1Len = u_strlen(src1);
        UChar* dest1 = output1;
        int32_t dest1Len = 40;
        UErrorCode status = U_ZERO_ERROR;
        UParseError ps;
        UChar* src2 = NULL;
        int32_t src2Len = 0;
        UChar* dest2 = output2;
        int32_t dest2Len = 40;
        dest1Len = uidna_toASCII(src1, src1Len, dest1, dest1Len,UIDNA_DEFAULT, &ps, &status);
        if(U_FAILURE(status)){
            log_err_status(status, "uidna_toUnicode failed with error %s.\n", u_errorName(status));
        }
        src2 = dest1;
        src2Len = dest1Len;
        dest2Len = uidna_toUnicode(src2, src2Len, dest2, dest2Len, UIDNA_DEFAULT, &ps, &status);
        if(U_FAILURE(status)){
            log_err_status(status, "uidna_toUnicode failed with error %s.\n", u_errorName(status));
        }
    }
}

static void TestJB4475(){
    
    static const UChar input[][10] = {
        {0x0054,0x0045,0x0053,0x0054,0x0000},/* TEST */
        {0x0074,0x0065,0x0073,0x0074,0x0000} /* test */
    };
    int i;
    UChar output[40] = {0};
    for(i=0; i< sizeof(input)/sizeof(input[0]); i++){
        const UChar* src = input[i];
        int32_t srcLen = u_strlen(src);
        UChar* dest = output;
        int32_t destLen = 40;
        UErrorCode status = U_ZERO_ERROR;
        UParseError ps;

        destLen = uidna_toASCII(src, srcLen, dest, destLen,UIDNA_DEFAULT, &ps, &status);
        if(U_FAILURE(status)){
            log_err_status(status, "uidna_toASCII failed with error %s.\n", u_errorName(status));
            continue;
        } 
        if(u_strncmp(input[i], dest, srcLen)!=0){
            log_err("uidna_toASCII did not return the expected output.\n");
        }
    }
}

static void TestLength(){
    {
        static const char* cl = "my_very_very_very_very_very_very_very_very_very_very_very_very_very_long_and_incredibly_uncreative_domain_label";
        UChar ul[128] = {'\0'};
        UChar dest[256] = {'\0'};
        /* this unicode string is longer than MAX_LABEL_BUFFER_SIZE and produces an 
           IDNA prepared string (including xn--)that is exactly 63 bytes long */
        UChar ul1[] = { 0xC138, 0xACC4, 0xC758, 0xBAA8, 0xB4E0, 0xC0AC, 0xB78C, 0xB4E4, 0xC774, 
                        0xD55C, 0xAD6D, 0xC5B4, 0xB97C, 0xC774, 0x00AD, 0x034F, 0x1806, 0x180B, 
                        0x180C, 0x180D, 0x200B, 0x200C, 0x200D, 0x2060, 0xFE00, 0xFE01, 0xFE02, 
                        0xFE03, 0xFE04, 0xFE05, 0xFE06, 0xFE07, 0xFE08, 0xFE09, 0xFE0A, 0xFE0B, 
                        0xFE0C, 0xFE0D, 0xFE0E, 0xFE0F, 0xFEFF, 0xD574, 0xD55C, 0xB2E4, 0xBA74, 
                        0xC138, 0x0041, 0x00AD, 0x034F, 0x1806, 0x180B, 0x180C, 0x180D, 0x200B, 
                        0x200C, 0x200D, 0x2060, 0xFE00, 0xFE01, 0xFE02, 0xFE03, 0xFE04, 0xFE05, 
                        0xFE06, 0xFE07, 0xFE08, 0xFE09, 0xFE0A, 0xFE0B, 0xFE0C, 0xFE0D, 0xFE0E, 
                        0xFE0F, 0xFEFF, 0x00AD, 0x034F, 0x1806, 0x180B, 0x180C, 0x180D, 0x200B, 
                        0x200C, 0x200D, 0x2060, 0xFE00, 0xFE01, 0xFE02, 0xFE03, 0xFE04, 0xFE05, 
                        0xFE06, 0xFE07, 0xFE08, 0xFE09, 0xFE0A, 0xFE0B, 0xFE0C, 0xFE0D, 0xFE0E, 
                        0xFE0F, 0xFEFF, 0x00AD, 0x034F, 0x1806, 0x180B, 0x180C, 0x180D, 0x200B, 
                        0x200C, 0x200D, 0x2060, 0xFE00, 0xFE01, 0xFE02, 0xFE03, 0xFE04, 0xFE05, 
                        0xFE06, 0xFE07, 0xFE08, 0xFE09, 0xFE0A, 0xFE0B, 0xFE0C, 0xFE0D, 0xFE0E, 
                        0xFE0F, 0xFEFF, 0x0000
                      };

        int32_t len1 = LENGTHOF(ul1)-1/*remove the null termination*/;
        int32_t destLen = LENGTHOF(dest);
        UErrorCode status = U_ZERO_ERROR;
        UParseError ps;
        int32_t len = (int32_t)strlen(cl);
        u_charsToUChars(cl, ul, len+1);
        destLen = uidna_toUnicode(ul, len, dest, destLen, UIDNA_DEFAULT, &ps, &status);
        if(status != U_ZERO_ERROR){
            log_err_status(status, "uidna_toUnicode failed with error %s.\n", u_errorName(status));
        }

        status = U_ZERO_ERROR;
        destLen = LENGTHOF(dest);
        len = -1;
        destLen = uidna_toUnicode(ul, len, dest, destLen, UIDNA_DEFAULT, &ps, &status);
        if(status != U_ZERO_ERROR){
            log_err_status(status, "uidna_toUnicode failed with error %s.\n", u_errorName(status));
        }
        status = U_ZERO_ERROR;
        destLen = LENGTHOF(dest);
        len = (int32_t)strlen(cl);
        destLen = uidna_toASCII(ul, len, dest, destLen, UIDNA_DEFAULT, &ps, &status);
        if(status != U_IDNA_LABEL_TOO_LONG_ERROR){
            log_err_status(status, "uidna_toASCII failed with error %s.\n", u_errorName(status));
        }
        
        status = U_ZERO_ERROR;
        destLen = LENGTHOF(dest);
        len = -1;
        destLen = uidna_toASCII(ul, len, dest, destLen, UIDNA_DEFAULT, &ps, &status);
        if(status != U_IDNA_LABEL_TOO_LONG_ERROR){
            log_err_status(status, "uidna_toASCII failed with error %s.\n", u_errorName(status));
        }

        status = U_ZERO_ERROR;
        destLen = LENGTHOF(dest);
        destLen = uidna_toASCII(ul1, len1, dest, destLen, UIDNA_DEFAULT, &ps, &status);
        if(status != U_ZERO_ERROR){
            log_err_status(status, "uidna_toASCII failed with error %s.\n", u_errorName(status));
        }
        
        status = U_ZERO_ERROR;
        destLen = LENGTHOF(dest);
        len1 = -1;
        destLen = uidna_toASCII(ul1, len1, dest, destLen, UIDNA_DEFAULT, &ps, &status);
        if(status != U_ZERO_ERROR){
            log_err_status(status, "uidna_toASCII failed with error %s.\n", u_errorName(status));
        }
    }
    {
        static const char* cl = "my_very_very_long_and_incredibly_uncreative_domain_label.my_very_very_long_and_incredibly_uncreative_domain_label.my_very_very_long_and_incredibly_uncreative_domain_label.my_very_very_long_and_incredibly_uncreative_domain_label.my_very_very_long_and_incredibly_uncreative_domain_label.my_very_very_long_and_incredibly_uncreative_domain_label.ibm.com";
        UChar ul[400] = {'\0'};
        UChar dest[400] = {'\0'};
        int32_t destLen = LENGTHOF(dest);
        UErrorCode status = U_ZERO_ERROR;
        UParseError ps;
        int32_t len = (int32_t)strlen(cl);
        u_charsToUChars(cl, ul, len+1);
        
        destLen = uidna_IDNToUnicode(ul, len, dest, destLen, UIDNA_DEFAULT, &ps, &status);
        if(status != U_IDNA_DOMAIN_NAME_TOO_LONG_ERROR){
            log_err_status(status, "uidna_IDNToUnicode failed with error %s.\n", u_errorName(status));
        }
        
        status = U_ZERO_ERROR;
        destLen = LENGTHOF(dest);
        len = -1;
        destLen = uidna_IDNToUnicode(ul, len, dest, destLen, UIDNA_DEFAULT, &ps, &status);
        if(status != U_IDNA_DOMAIN_NAME_TOO_LONG_ERROR){
            log_err_status(status, "uidna_IDNToUnicode failed with error %s.\n", u_errorName(status));
        }
        
        status = U_ZERO_ERROR;
        destLen = LENGTHOF(dest);
        len = (int32_t)strlen(cl);
        destLen = uidna_IDNToASCII(ul, len, dest, destLen, UIDNA_DEFAULT, &ps, &status);
        if(status != U_IDNA_DOMAIN_NAME_TOO_LONG_ERROR){
            log_err_status(status, "uidna_IDNToASCII failed with error %s.\n", u_errorName(status));
        }
        
        status = U_ZERO_ERROR;
        destLen = LENGTHOF(dest);
        len = -1;
        destLen = uidna_IDNToASCII(ul, len, dest, destLen, UIDNA_DEFAULT, &ps, &status);
        if(status != U_IDNA_DOMAIN_NAME_TOO_LONG_ERROR){
            log_err_status(status, "uidna_IDNToASCII failed with error %s.\n", u_errorName(status));
        }

        status = U_ZERO_ERROR;
        uidna_compare(ul, len, ul, len, UIDNA_DEFAULT, &status);
        if(status != U_IDNA_DOMAIN_NAME_TOO_LONG_ERROR){
            log_err_status(status, "uidna_compare failed with error %s.\n", u_errorName(status));
        }
        uidna_compare(ul, -1, ul, -1, UIDNA_DEFAULT, &status);
        if(status != U_IDNA_DOMAIN_NAME_TOO_LONG_ERROR){
            log_err_status(status, "uidna_compare failed with error %s.\n", u_errorName(status));
        }
    }    
}
static void TestJB5273(){
    static const char INVALID_DOMAIN_NAME[] = "xn--m\\u00FCller.de";
    UChar invalid_idn[25] = {'\0'};
    int32_t len = u_unescape(INVALID_DOMAIN_NAME, invalid_idn, strlen(INVALID_DOMAIN_NAME));
    UChar output[50] = {'\0'};
    UErrorCode status = U_ZERO_ERROR;
    UParseError prsError;
    int32_t outLen = uidna_toUnicode(invalid_idn, len, output, 50, UIDNA_DEFAULT, &prsError, &status);
    if(U_FAILURE(status)){
        log_err_status(status, "uidna_toUnicode failed with error: %s\n", u_errorName(status));
    }
    status = U_ZERO_ERROR;
    outLen = uidna_toUnicode(invalid_idn, len, output, 50, UIDNA_USE_STD3_RULES, &prsError, &status);
    if(U_FAILURE(status)){
        log_err_status(status, "uidna_toUnicode failed with error: %s\n", u_errorName(status));
    }

    status = U_ZERO_ERROR;
    outLen = uidna_IDNToUnicode(invalid_idn, len, output, 50, UIDNA_DEFAULT, &prsError, &status);
    if(U_FAILURE(status)){
        log_err_status(status, "uidna_toUnicode failed with error: %s\n", u_errorName(status));
    }
    status = U_ZERO_ERROR;
    outLen = uidna_IDNToUnicode(invalid_idn, len, output, 50, UIDNA_USE_STD3_RULES, &prsError, &status);
    if(U_FAILURE(status)){
        log_err_status(status, "uidna_toUnicode failed with error: %s\n", u_errorName(status));
    }
}

/*
 * Test the new (ICU 4.6/2010) C API that was added for UTS #46.
 * Just an API test: Functionality is tested via C++ intltest.
 */
static void TestUTS46() {
    static const UChar fA_sharps16[] = { 0x66, 0x41, 0xdf, 0 };
    static const char fA_sharps8[] = { 0x66, 0x41, (char)0xc3, (char)0x9f, 0 };
    static const UChar fa_sharps16[] = { 0x66, 0x61, 0xdf, 0 };
    static const char fa_sharps8[] = { 0x66, 0x61, (char)0xc3, (char)0x9f, 0 };
    static const UChar fass16[] = { 0x66, 0x61, 0x73, 0x73, 0 };
    static const char fass8[] = { 0x66, 0x61, 0x73, 0x73, 0 };
    static const UChar fA_BEL[] = { 0x66, 0x41, 7, 0 };
    static const UChar fa_FFFD[] = { 0x66, 0x61, 0xfffd, 0 };

    UChar dest16[10];
    char dest8[10];
    int32_t length;

    UIDNAInfo info = UIDNA_INFO_INITIALIZER;
    UErrorCode errorCode = U_ZERO_ERROR;
    UIDNA *uts46 = uidna_openUTS46(UIDNA_USE_STD3_RULES|UIDNA_NONTRANSITIONAL_TO_UNICODE,
                                   &errorCode);
    if(U_FAILURE(errorCode)) {
        log_err_status(errorCode, "uidna_openUTS46() failed: %s\n", u_errorName(errorCode));
        return;
    }

    /* These calls should succeed. */
    length = uidna_labelToASCII(uts46, fA_sharps16, -1,
                                dest16, LENGTHOF(dest16), &info, &errorCode);
    if( U_FAILURE(errorCode) || length != 4 || 0 != u_memcmp(dest16, fass16, 5) ||
        !info.isTransitionalDifferent || info.errors != 0
    ) {
        log_err("uidna_labelToASCII() failed: %s\n", u_errorName(errorCode));
    }
    errorCode = U_ZERO_ERROR;
    length = uidna_labelToUnicode(uts46, fA_sharps16, u_strlen(fA_sharps16),
                                  dest16, LENGTHOF(dest16), &info, &errorCode);
    if( U_FAILURE(errorCode) || length != 3 || 0 != u_memcmp(dest16, fa_sharps16, 4) ||
        !info.isTransitionalDifferent || info.errors != 0
    ) {
        log_err("uidna_labelToUnicode() failed: %s\n", u_errorName(errorCode));
    }
    errorCode = U_ZERO_ERROR;
    length = uidna_nameToASCII(uts46, fA_sharps16, u_strlen(fA_sharps16),
                               dest16, 4, &info, &errorCode);
    if( errorCode != U_STRING_NOT_TERMINATED_WARNING ||
        length != 4 || 0 != u_memcmp(dest16, fass16, 4) ||
        !info.isTransitionalDifferent || info.errors != 0
    ) {
        log_err("uidna_nameToASCII() failed: %s\n", u_errorName(errorCode));
    }
    errorCode = U_ZERO_ERROR;
    length = uidna_nameToUnicode(uts46, fA_sharps16, -1,
                                 dest16, 3, &info, &errorCode);
    if( errorCode != U_STRING_NOT_TERMINATED_WARNING ||
        length != 3 || 0 != u_memcmp(dest16, fa_sharps16, 3) ||
        !info.isTransitionalDifferent || info.errors != 0
    ) {
        log_err("uidna_nameToUnicode() failed: %s\n", u_errorName(errorCode));
    }

    errorCode = U_ZERO_ERROR;
    length = uidna_labelToASCII_UTF8(uts46, fA_sharps8, -1,
                                     dest8, LENGTHOF(dest8), &info, &errorCode);
    if( U_FAILURE(errorCode) || length != 4 || 0 != memcmp(dest8, fass8, 5) ||
        !info.isTransitionalDifferent || info.errors != 0
    ) {
        log_err("uidna_labelToASCII_UTF8() failed: %s\n", u_errorName(errorCode));
    }
    errorCode = U_ZERO_ERROR;
    length = uidna_labelToUnicodeUTF8(uts46, fA_sharps8, strlen(fA_sharps8),
                                      dest8, LENGTHOF(dest8), &info, &errorCode);
    if( U_FAILURE(errorCode) || length != 4 || 0 != memcmp(dest8, fa_sharps8, 5) ||
        !info.isTransitionalDifferent || info.errors != 0
    ) {
        log_err("uidna_labelToUnicodeUTF8() failed: %s\n", u_errorName(errorCode));
    }
    errorCode = U_ZERO_ERROR;
    length = uidna_nameToASCII_UTF8(uts46, fA_sharps8, strlen(fA_sharps8),
                                    dest8, 4, &info, &errorCode);
    if( errorCode != U_STRING_NOT_TERMINATED_WARNING ||
        length != 4 || 0 != memcmp(dest8, fass8, 4) ||
        !info.isTransitionalDifferent || info.errors != 0
    ) {
        log_err("uidna_nameToASCII_UTF8() failed: %s\n", u_errorName(errorCode));
    }
    errorCode = U_ZERO_ERROR;
    length = uidna_nameToUnicodeUTF8(uts46, fA_sharps8, -1,
                                     dest8, 4, &info, &errorCode);
    if( errorCode != U_STRING_NOT_TERMINATED_WARNING ||
        length != 4 || 0 != memcmp(dest8, fa_sharps8, 4) ||
        !info.isTransitionalDifferent || info.errors != 0
    ) {
        log_err("uidna_nameToUnicodeUTF8() failed: %s\n", u_errorName(errorCode));
    }

    errorCode = U_ZERO_ERROR;
    length = uidna_nameToASCII(uts46, NULL, 0,
                               dest16, 0, &info, &errorCode);
    if( errorCode != U_STRING_NOT_TERMINATED_WARNING ||
        length != 0 ||
        info.isTransitionalDifferent || info.errors != UIDNA_ERROR_EMPTY_LABEL
    ) {
        log_err("uidna_nameToASCII(empty) failed: %s\n", u_errorName(errorCode));
    }
    errorCode = U_ZERO_ERROR;
    length = uidna_nameToUnicode(uts46, fA_BEL, -1,
                                 dest16, 3, &info, &errorCode);
    if( errorCode != U_STRING_NOT_TERMINATED_WARNING ||
        length != 3 || 0 != u_memcmp(dest16, fa_FFFD, 3) ||
        info.isTransitionalDifferent || info.errors == 0
    ) {
        log_err("uidna_nameToUnicode(fa<BEL>) failed: %s\n", u_errorName(errorCode));
    }

    /* These calls should fail. */
    errorCode = U_USELESS_COLLATOR_ERROR;
    length = uidna_labelToASCII(uts46, fA_sharps16, -1,
                                dest16, LENGTHOF(dest16), &info, &errorCode);
    if(errorCode != U_USELESS_COLLATOR_ERROR) {
        log_err("uidna_labelToASCII(failure) failed: %s\n", u_errorName(errorCode));
    }
    errorCode = U_ZERO_ERROR;
    length = uidna_labelToUnicode(uts46, fA_sharps16, u_strlen(fA_sharps16),
                                  dest16, LENGTHOF(dest16), NULL, &errorCode);
    if(errorCode != U_ILLEGAL_ARGUMENT_ERROR) {
        log_err("uidna_labelToUnicode(UIDNAInfo=NULL) failed: %s\n", u_errorName(errorCode));
    }
    errorCode = U_ZERO_ERROR;
    length = uidna_nameToASCII(uts46, NULL, u_strlen(fA_sharps16),
                               dest16, 4, &info, &errorCode);
    if(errorCode != U_ILLEGAL_ARGUMENT_ERROR) {
        log_err("uidna_nameToASCII(src=NULL) failed: %s\n", u_errorName(errorCode));
    }
    errorCode = U_ZERO_ERROR;
    length = uidna_nameToUnicode(uts46, fA_sharps16, -2,
                                 dest16, 3, &info, &errorCode);
    if(errorCode != U_ILLEGAL_ARGUMENT_ERROR) {
        log_err("uidna_nameToUnicode(length<-1) failed: %s\n", u_errorName(errorCode));
    }

    errorCode = U_ZERO_ERROR;
    length = uidna_labelToASCII_UTF8(uts46, fA_sharps8, -1,
                                     NULL, LENGTHOF(dest8), &info, &errorCode);
    if(errorCode != U_ILLEGAL_ARGUMENT_ERROR) {
        log_err("uidna_labelToASCII_UTF8(dest=NULL) failed: %s\n", u_errorName(errorCode));
    }
    errorCode = U_ZERO_ERROR;
    length = uidna_labelToUnicodeUTF8(uts46, fA_sharps8, strlen(fA_sharps8),
                                      dest8, -1, &info, &errorCode);
    if(errorCode != U_ILLEGAL_ARGUMENT_ERROR) {
        log_err("uidna_labelToUnicodeUTF8(capacity<0) failed: %s\n", u_errorName(errorCode));
    }
    errorCode = U_ZERO_ERROR;
    length = uidna_nameToASCII_UTF8(uts46, dest8, strlen(fA_sharps8),
                                    dest8, 4, &info, &errorCode);
    if(errorCode != U_ILLEGAL_ARGUMENT_ERROR) {
        log_err("uidna_nameToASCII_UTF8(src==dest!=NULL) failed: %s\n", u_errorName(errorCode));
    }
    errorCode = U_ZERO_ERROR;
    length = uidna_nameToUnicodeUTF8(uts46, fA_sharps8, -1,
                                     dest8, 3, &info, &errorCode);
    if(errorCode != U_BUFFER_OVERFLOW_ERROR || length != 4) {
        log_err("uidna_nameToUnicodeUTF8() overflow failed: %s\n", u_errorName(errorCode));
    }

    uidna_close(uts46);
}

#endif

/*
 * Hey, Emacs, please set the following:
 *
 * Local Variables:
 * indent-tabs-mode: nil
 * End:
 *
 */
