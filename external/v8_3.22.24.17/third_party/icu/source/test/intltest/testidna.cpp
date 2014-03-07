/*
 *******************************************************************************
 *
 *   Copyright (C) 2003-2010, International Business Machines
 *   Corporation and others.  All Rights Reserved.
 *
 *******************************************************************************
 *   file name:  testidna.cpp
 *   encoding:   US-ASCII
 *   tab size:   8 (not used)
 *   indentation:4
 *
 *   created on: 2003feb1
 *   created by: Ram Viswanadha
 */

#include "unicode/utypes.h"

#if !UCONFIG_NO_IDNA && !UCONFIG_NO_TRANSLITERATION

#include <time.h>
#include <limits.h>
#include <stdlib.h>
#include <string.h>
#include "unicode/localpointer.h"
#include "unicode/ustring.h"
#include "unicode/usprep.h"
#include "unicode/uniset.h"
#include "testidna.h"
#include "idnaref.h"
#include "nptrans.h"
#include "unicode/putil.h"
#include "idnaconf.h"

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
    // test non-BMP code points
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
    // Greek
    {
        0x03b5, 0x03bb, 0x03bb, 0x03b7, 0x03bd, 0x03b9, 0x03ba, 0x03ac
    },
    // Maltese
    {
        0x0062, 0x006f, 0x006e, 0x0121, 0x0075, 0x0073, 0x0061, 0x0127,
        0x0127, 0x0061
    },
    // Russian
    {
        0x043f, 0x043e, 0x0447, 0x0435, 0x043c, 0x0443, 0x0436, 0x0435,
        0x043e, 0x043d, 0x0438, 0x043d, 0x0435, 0x0433, 0x043e, 0x0432,
        0x043e, 0x0440, 0x044f, 0x0442, 0x043f, 0x043e, 0x0440, 0x0443,
        0x0441, 0x0441, 0x043a, 0x0438
    },
    {
        0xFB00, 0xFB01
    }

};

static const char *asciiIn[] = {
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
    "xn--hxargifdar",                       // Greek
    "xn--bonusaa-5bb1da",                   // Maltese
    "xn--b1abfaaepdrnnbgefbadotcwatmq2g4l", // Russian (Cyrillic)
    "fffi"
};

static const char *domainNames[] = {
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
    "\\u0917\\u0928\\u0947\\u0936.sanjose.ibm.com", //':'(0x003a) produces U_IDNA_STD3_ASCII_RULES_ERROR
    "www.xn--vea.com",
   // "www.\\u00E0\\u00B3\\u00AF.com",//' ' (0x0020) produces U_IDNA_STD3_ASCII_RULES_ERROR
    "www.\\u00C2\\u00A4.com",
    "www.\\u00C2\\u00A3.com",
    // "\\u0025", //'%' (0x0025) produces U_IDNA_STD3_ASCII_RULES_ERROR
    // "\\u005C\\u005C", //'\' (0x005C) produces U_IDNA_STD3_ASCII_RULES_ERROR
    //"@",
    //"\\u002F",
    //"www.\\u0021.com",
    //"www.\\u0024.com",
    //"\\u003f",
    // These yeild U_IDNA_PROHIBITED_ERROR
    //"\\u00CF\\u0082.com",
    //"\\u00CE\\u00B2\\u00C3\\u009Fss.com",
    //"\\u00E2\\u0098\\u00BA.com",
    "\\u00C3\\u00BC.com",

};

typedef struct ErrorCases ErrorCases;

static const struct ErrorCases{

    UChar unicode[100];
    const char *ascii;
    UErrorCode expected;
    UBool useSTD3ASCIIRules;
    UBool testToUnicode;
    UBool testLabel;
} errorCases[] = {
      {
        
        { 
            0x0077, 0x0077, 0x0077, 0x002e, /* www. */
            0xC138, 0xACC4, 0xC758, 0xBAA8, 0xB4E0, 0xC0AC, 0xB78C, 0xB4E4, 0xC774,
            0x070F,/*prohibited*/
            0xD55C, 0xAD6D, 0xC5B4, 0xB97C, 0xC774, 0xD574, 0xD55C, 0xB2E4, 0xBA74,
            0x002e, 0x0063, 0x006f, 0x006d, /* com. */
            0x0000
        },
        "www.XN--8mb5595fsoa28orucya378bqre2tcwop06c5qbw82a1rffmae0361dea96b.com",
        U_IDNA_PROHIBITED_ERROR,
        FALSE, FALSE, TRUE
    },

    {
        { 
            0x0077, 0x0077, 0x0077, 0x002e, /* www. */
            0xC138, 0xACC4, 0xC758, 0xBAA8, 0xB4E0, 0xC0AC, 0xB78C, 0xB4E4, 0xC774,
            0x0221, 0x0234/*Unassigned code points*/,
            0x002e, 0x0063, 0x006f, 0x006d, /* com. */
            0x0000
        },
        "www.XN--6lA2Bz548Fj1GuA391Bf1Gb1N59Ab29A7iA.com",

        U_IDNA_UNASSIGNED_ERROR,
        FALSE, FALSE, TRUE
    },
    {
        { 
            0x0077, 0x0077, 0x0077, 0x002e, /* www. */
            0xC138, 0xACC4, 0xC758, 0xBAA8, 0xB4E0, 0xC0AC, 0xB78C, 0xB4E4, 0xC774,
            0x0644, 0x064A, 0x0647,/*Arabic code points. Cannot mix RTL with LTR*/
            0xD55C, 0xAD6D, 0xC5B4, 0xB97C, 0xC774, 0xD574, 0xD55C, 0xB2E4, 0xBA74,
            0x002e, 0x0063, 0x006f, 0x006d, /* com. */
            0x0000
        },
        "www.xn--ghBGI4851OiyA33VqrD6Az86C4qF83CtRv93D5xBk15AzfG0nAgA0578DeA71C.com",
        U_IDNA_CHECK_BIDI_ERROR,
        FALSE, FALSE, TRUE
    },
    {
        { 
            0x0077, 0x0077, 0x0077, 0x002e, /* www. */
            /* labels cannot begin with an HYPHEN */
            0x002D, 0xACC4, 0xC758, 0xBAA8, 0xB4E0, 0xC0AC, 0xB78C, 0xB4E4, 0xC774,
            0x002E, 
            0xD55C, 0xAD6D, 0xC5B4, 0xB97C, 0xC774, 0xD574, 0xD55C, 0xB2E4, 0xBA74,
            0x002e, 0x0063, 0x006f, 0x006d, /* com. */
            0x0000
            
        },
        "www.xn----b95Ew8SqA315Ao5FbuMlnNmhA.com",
        U_IDNA_STD3_ASCII_RULES_ERROR,
        TRUE, FALSE, FALSE
    },
    {
        { 
            /* correct ACE-prefix followed by unicode */
            0x0077, 0x0077, 0x0077, 0x002e, /* www. */
            0x0078, 0x006e, 0x002d,0x002d,  /* ACE Prefix */
            0x002D, 0xACC4, 0xC758, 0xBAA8, 0xB4E0, 0xC0AC, 0xB78C, 0xB4E4, 0xC774,
            0x002D, 
            0xD55C, 0xAD6D, 0xC5B4, 0xB97C, 0xC774, 0xD574, 0xD55C, 0xB2E4, 0xBA74,
            0x002e, 0x0063, 0x006f, 0x006d, /* com. */
            0x0000
            
        },
        /* wrong ACE-prefix followed by valid ACE-encoded ASCII */ 
        "www.XY-----b91I0V65S96C2A355Cw1E5yCeQr19CsnP1mFfmAE0361DeA96B.com",
        U_IDNA_ACE_PREFIX_ERROR,
        FALSE, FALSE, FALSE
    },
    /* cannot verify U_IDNA_VERIFICATION_ERROR */

    { 
      {
        0x0077, 0x0077, 0x0077, 0x002e, /* www. */
        0xC138, 0xACC4, 0xC758, 0xBAA8, 0xB4E0, 0xC0AC, 0xB78C, 0xB4E4, 0xC774,
        0xD55C, 0xAD6D, 0xC5B4, 0xB97C, 0xC774, 0xD574, 0xD55C, 0xB2E4, 0xBA74,
        0xC5BC, 0xB9C8, 0xB098, 0xC88B, 0xC744, 0xAE4C, 
        0x002e, 0x0063, 0x006f, 0x006d, /* com. */
        0x0000
      },
      "www.xn--989AoMsVi5E83Db1D2A355Cv1E0vAk1DwRv93D5xBh15A0Dt30A5JpSD879Ccm6FeA98C.com",
      U_IDNA_LABEL_TOO_LONG_ERROR,
      FALSE, FALSE, TRUE
    },  
    
    { 
      {
        0x0077, 0x0077, 0x0077, 0x002e, /* www. */
        0x0030, 0x0644, 0x064A, 0x0647, 0x0031, /* Arabic code points squashed between EN codepoints */
        0x002e, 0x0063, 0x006f, 0x006d, /* com. */
        0x0000
      },
      "www.xn--01-tvdmo.com",
      U_IDNA_CHECK_BIDI_ERROR,
      FALSE, FALSE, TRUE
    },  
    
    { 
      {
        0x0077, 0x0077, 0x0077, 0x002e, // www. 
        0x206C, 0x0644, 0x064A, 0x0647, 0x206D, // Arabic code points squashed between BN codepoints 
        0x002e, 0x0063, 0x006f, 0x006d, // com. 
        0x0000
      },
      "www.XN--ghbgi278xia.com",
      U_IDNA_PROHIBITED_ERROR,
      FALSE, FALSE, TRUE
    },
    { 
      {
        0x0077, 0x0077, 0x0077, 0x002e, // www. 
        0x002D, 0x0041, 0x0042, 0x0043, 0x0044, 0x0045, // HYPHEN at the start of label 
        0x002e, 0x0063, 0x006f, 0x006d, // com. 
        0x0000
      },
      "www.-abcde.com",
      U_IDNA_STD3_ASCII_RULES_ERROR,
      TRUE, FALSE, FALSE
    },
    { 
      {
        0x0077, 0x0077, 0x0077, 0x002e, // www. 
        0x0041, 0x0042, 0x0043, 0x0044, 0x0045,0x002D, // HYPHEN at the end of the label
        0x002e, 0x0063, 0x006f, 0x006d, // com. 
        0x0000
      },
      "www.abcde-.com",
      U_IDNA_STD3_ASCII_RULES_ERROR,
      TRUE, FALSE, FALSE
    },
    { 
      {
        0x0077, 0x0077, 0x0077, 0x002e, // www. 
        0x0041, 0x0042, 0x0043, 0x0044, 0x0045,0x0040, // Containing non LDH code point
        0x002e, 0x0063, 0x006f, 0x006d, // com. 
        0x0000
      },
      "www.abcde@.com",
      U_IDNA_STD3_ASCII_RULES_ERROR,
      TRUE, FALSE, FALSE
    },
    { 
      {
        0x0077, 0x0077, 0x0077, 0x002e, // www. 
        // zero length label
        0x002e, 0x0063, 0x006f, 0x006d, // com. 
        0x0000
      },
      "www..com",
      U_IDNA_ZERO_LENGTH_LABEL_ERROR,
      TRUE, FALSE, FALSE
    },
    { 
      {0},
      NULL,
      U_ILLEGAL_ARGUMENT_ERROR,
      TRUE, TRUE, FALSE
    }
};




#define MAX_DEST_SIZE 300

void TestIDNA::debug(const UChar* src, int32_t srcLength, int32_t options){
    UParseError parseError;
    UErrorCode transStatus = U_ZERO_ERROR;
    UErrorCode prepStatus  = U_ZERO_ERROR;
    NamePrepTransform* trans = NamePrepTransform::createInstance(parseError,transStatus);
    int32_t prepOptions = (((options & UIDNA_ALLOW_UNASSIGNED) != 0) ? USPREP_ALLOW_UNASSIGNED: 0);
    LocalUStringPrepProfilePointer prep(usprep_openByType(USPREP_RFC3491_NAMEPREP,&prepStatus));
    UChar *transOut=NULL, *prepOut=NULL;
    int32_t transOutLength=0, prepOutLength=0;
    
    
    transOutLength  = trans->process(src,srcLength,transOut, 0, prepOptions>0, &parseError, transStatus);
    if( transStatus == U_BUFFER_OVERFLOW_ERROR){
        transStatus = U_ZERO_ERROR;
        transOut    = (UChar*) malloc(U_SIZEOF_UCHAR * transOutLength);
        transOutLength = trans->process(src,srcLength,transOut, transOutLength, prepOptions>0, &parseError, transStatus);
    }

    prepOutLength  = usprep_prepare(prep.getAlias(), src, srcLength, prepOut, 0, prepOptions, &parseError, &prepStatus);

    if( prepStatus == U_BUFFER_OVERFLOW_ERROR){
        prepStatus = U_ZERO_ERROR;
        prepOut    = (UChar*) malloc(U_SIZEOF_UCHAR * prepOutLength);
        prepOutLength  = usprep_prepare(prep.getAlias(), src, srcLength, prepOut, prepOutLength, prepOptions, &parseError, &prepStatus);
    }

    if(UnicodeString(transOut,transOutLength)!= UnicodeString(prepOut, prepOutLength)){
        errln("Failed. Expected: " + prettify(UnicodeString(transOut, transOutLength))
              + " Got: " + prettify(UnicodeString(prepOut,prepOutLength)));
    }
    free(transOut);
    free(prepOut);
    delete trans;
}

void TestIDNA::testAPI(const UChar* src, const UChar* expected, const char* testName, 
            UBool useSTD3ASCIIRules,UErrorCode expectedStatus,
            UBool doCompare, UBool testUnassigned, TestFunc func, UBool testSTD3ASCIIRules){

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

    // test null-terminated source and return value of number of UChars required
    destLen = func(src,-1,NULL,0,options, &parseError , &status);
    if(status == U_BUFFER_OVERFLOW_ERROR){
        status = U_ZERO_ERROR; // reset error code
        if(destLen+1 < MAX_DEST_SIZE){
            dest = destStack;
            destLen = func(src,-1,dest,destLen+1,options, &parseError, &status);
            // TODO : compare output with expected
            if(U_SUCCESS(status) && expectedStatus != U_IDNA_STD3_ASCII_RULES_ERROR&& (doCompare==TRUE) && u_strCaseCompare(dest,destLen, expected,expectedLen,0,&status)!=0){
                errln("Did not get the expected result for "+UnicodeString(testName) +" null terminated source. Expected : " 
                       + prettify(UnicodeString(expected,expectedLen))
                       + " Got: " + prettify(UnicodeString(dest,destLen))
                    );
            }
        }else{
            errln( "%s null terminated source failed. Requires destCapacity > 300\n",testName);
        }
    }

    if(status != expectedStatus){
        errcheckln(status, "Did not get the expected error for "+
                UnicodeString(testName)+
                " null terminated source. Expected: " +UnicodeString(u_errorName(expectedStatus))
                + " Got: "+ UnicodeString(u_errorName(status))
                + " Source: " + prettify(UnicodeString(src))
               );
        free(tSrc);
        return;
    } 
    if(testUnassigned ){
        status = U_ZERO_ERROR;
        destLen = func(src,-1,NULL,0,options | UIDNA_ALLOW_UNASSIGNED, &parseError, &status);
        if(status == U_BUFFER_OVERFLOW_ERROR){
            status = U_ZERO_ERROR; // reset error code
            if(destLen+1 < MAX_DEST_SIZE){
                dest = destStack;
                destLen = func(src,-1,dest,destLen+1,options | UIDNA_ALLOW_UNASSIGNED, &parseError, &status);
                // TODO : compare output with expected
                if(U_SUCCESS(status) && (doCompare==TRUE) && u_strCaseCompare(dest,destLen, expected,expectedLen,0,&status)!=0){
                    //errln("Did not get the expected result for %s null terminated source with both options set.\n",testName);
                    errln("Did not get the expected result for "+UnicodeString(testName) +
                          " null terminated source "+ prettify(src) + 
                          " with both options set. Expected: "+  prettify(UnicodeString(expected,expectedLen))+
                          "Got: " + prettify(UnicodeString(dest,destLen)));

                    debug(src,-1,options | UIDNA_ALLOW_UNASSIGNED);
            
                }
            }else{
                errln( "%s null terminated source failed. Requires destCapacity > 300\n",testName);
            }
        }
        //testing query string
        if(status != expectedStatus && expectedStatus != U_IDNA_UNASSIGNED_ERROR){
                errln( "Did not get the expected error for "+
                    UnicodeString(testName)+
                    " null terminated source with options set. Expected: " +UnicodeString(u_errorName(expectedStatus))
                    + " Got: "+ UnicodeString(u_errorName(status))
                    + " Source: " + prettify(UnicodeString(src))
                   );
        }  
    }

    status = U_ZERO_ERROR;

    // test source with lengthand return value of number of UChars required
    destLen = func(tSrc, tSrcLen, NULL,0,options, &parseError, &status);
    if(status == U_BUFFER_OVERFLOW_ERROR){
        status = U_ZERO_ERROR; // reset error code
        if(destLen+1 < MAX_DEST_SIZE){
            dest = destStack;
            destLen = func(src,u_strlen(src),dest,destLen+1,options, &parseError, &status);
            // TODO : compare output with expected
            if(U_SUCCESS(status) && (doCompare==TRUE) && u_strCaseCompare(dest,destLen, expected,expectedLen,0,&status)!=0){
                errln("Did not get the expected result for %s with source length.\n",testName);
            }
        }else{
            errln( "%s with source length  failed. Requires destCapacity > 300\n",testName);
        }
    }

    if(status != expectedStatus){
        errln( "Did not get the expected error for "+
                    UnicodeString(testName)+
                    " with source length. Expected: " +UnicodeString(u_errorName(expectedStatus))
                    + " Got: "+ UnicodeString(u_errorName(status))
                    + " Source: " + prettify(UnicodeString(src))
                   );
    } 
    if(testUnassigned){
        status = U_ZERO_ERROR;

        destLen = func(tSrc,tSrcLen,NULL,0,options | UIDNA_ALLOW_UNASSIGNED, &parseError, &status);

        if(status == U_BUFFER_OVERFLOW_ERROR){
            status = U_ZERO_ERROR; // reset error code
            if(destLen+1 < MAX_DEST_SIZE){
                dest = destStack;
                destLen = func(src,u_strlen(src),dest,destLen+1,options | UIDNA_ALLOW_UNASSIGNED, &parseError, &status);
                // TODO : compare output with expected
                if(U_SUCCESS(status) && (doCompare==TRUE) && u_strCaseCompare(dest,destLen, expected,expectedLen,0,&status)!=0){
                    errln("Did not get the expected result for %s with source length and both options set.\n",testName);
                }
            }else{
                errln( "%s with source length  failed. Requires destCapacity > 300\n",testName);
            }
        }
        //testing query string
        if(status != expectedStatus && expectedStatus != U_IDNA_UNASSIGNED_ERROR){
            errln( "Did not get the expected error for "+
                    UnicodeString(testName)+
                    " with source length and options set. Expected: " +UnicodeString(u_errorName(expectedStatus))
                    + " Got: "+ UnicodeString(u_errorName(status))
                    + " Source: " + prettify(UnicodeString(src))
                   );
        }
    }

    status = U_ZERO_ERROR;
    if(testSTD3ASCIIRules==TRUE){
        destLen = func(src,-1,NULL,0,options | UIDNA_USE_STD3_RULES, &parseError, &status);
        if(status == U_BUFFER_OVERFLOW_ERROR){
            status = U_ZERO_ERROR; // reset error code
            if(destLen+1 < MAX_DEST_SIZE){
                dest = destStack;
                destLen = func(src,-1,dest,destLen+1,options | UIDNA_USE_STD3_RULES, &parseError, &status);
                // TODO : compare output with expected
                if(U_SUCCESS(status) && (doCompare==TRUE) && u_strCaseCompare(dest,destLen, expected,expectedLen,0,&status)!=0){
                    //errln("Did not get the expected result for %s null terminated source with both options set.\n",testName);
                    errln("Did not get the expected result for "+UnicodeString(testName) +" null terminated source with both options set. Expected: "+ prettify(UnicodeString(expected,expectedLen)));
        
                }
            }else{
                errln( "%s null terminated source failed. Requires destCapacity > 300\n",testName);
            }
        }
        //testing query string
        if(status != expectedStatus){
            errln( "Did not get the expected error for "+
                        UnicodeString(testName)+
                        " null terminated source with options set. Expected: " +UnicodeString(u_errorName(expectedStatus))
                        + " Got: "+ UnicodeString(u_errorName(status))
                        + " Source: " + prettify(UnicodeString(src))
                       );
        } 

        status = U_ZERO_ERROR;

        destLen = func(tSrc,tSrcLen,NULL,0,options | UIDNA_USE_STD3_RULES, &parseError, &status);

        if(status == U_BUFFER_OVERFLOW_ERROR){
            status = U_ZERO_ERROR; // reset error code
            if(destLen+1 < MAX_DEST_SIZE){
                dest = destStack;
                destLen = func(src,u_strlen(src),dest,destLen+1,options | UIDNA_USE_STD3_RULES, &parseError, &status);
                // TODO : compare output with expected
                if(U_SUCCESS(status) && (doCompare==TRUE) && u_strCaseCompare(dest,destLen, expected,expectedLen,0,&status)!=0){
                    errln("Did not get the expected result for %s with source length and both options set.\n",testName);
                }
            }else{
                errln( "%s with source length  failed. Requires destCapacity > 300\n",testName);
            }
        }
        //testing query string
        if(status != expectedStatus && expectedStatus != U_IDNA_UNASSIGNED_ERROR){
            errln( "Did not get the expected error for "+
                        UnicodeString(testName)+
                        " with source length and options set. Expected: " +UnicodeString(u_errorName(expectedStatus))
                        + " Got: "+ UnicodeString(u_errorName(status))
                        + " Source: " + prettify(UnicodeString(src))
                       );
        }
    }
    free(tSrc);
}

void TestIDNA::testCompare(const UChar* s1, int32_t s1Len,
                        const UChar* s2, int32_t s2Len,
                        const char* testName, CompareFunc func,
                        UBool isEqual){

    UErrorCode status = U_ZERO_ERROR;
    int32_t retVal = func(s1,-1,s2,-1,UIDNA_DEFAULT,&status);

    if(isEqual==TRUE &&  retVal !=0){
        errln("Did not get the expected result for %s with null termniated strings.\n",testName);
    }
    if(U_FAILURE(status)){
        errcheckln(status, "%s null terminated source failed. Error: %s", testName,u_errorName(status));
    }

    status = U_ZERO_ERROR;
    retVal = func(s1,-1,s2,-1,UIDNA_ALLOW_UNASSIGNED,&status);

    if(isEqual==TRUE &&  retVal !=0){
        errln("Did not get the expected result for %s with null termniated strings with options set.\n", testName);
    }
    if(U_FAILURE(status)){
        errcheckln(status, "%s null terminated source and options set failed. Error: %s",testName, u_errorName(status));
    }
    
    status = U_ZERO_ERROR;
    retVal = func(s1,s1Len,s2,s2Len,UIDNA_DEFAULT,&status);

    if(isEqual==TRUE &&  retVal !=0){
        errln("Did not get the expected result for %s with string length.\n",testName);
    }
    if(U_FAILURE(status)){
        errcheckln(status, "%s with string length. Error: %s",testName, u_errorName(status));
    }
    
    status = U_ZERO_ERROR;
    retVal = func(s1,s1Len,s2,s2Len,UIDNA_ALLOW_UNASSIGNED,&status);

    if(isEqual==TRUE &&  retVal !=0){
        errln("Did not get the expected result for %s with string length and options set.\n",testName);
    }
    if(U_FAILURE(status)){
        errcheckln(status, "%s with string length and options set. Error: %s", u_errorName(status), testName);
    }
}

void TestIDNA::testToASCII(const char* testName, TestFunc func){

    int32_t i;
    UChar buf[MAX_DEST_SIZE];

    for(i=0;i< (int32_t)(sizeof(unicodeIn)/sizeof(unicodeIn[0])); i++){
        u_charsToUChars(asciiIn[i],buf, (int32_t)(strlen(asciiIn[i])+1));
        testAPI(unicodeIn[i], buf,testName, FALSE,U_ZERO_ERROR, TRUE, TRUE, func);
        
    }
}

void TestIDNA::testToUnicode(const char* testName, TestFunc func){

    int32_t i;
    UChar buf[MAX_DEST_SIZE];
    
    for(i=0;i< (int32_t)(sizeof(asciiIn)/sizeof(asciiIn[0])); i++){
        u_charsToUChars(asciiIn[i],buf, (int32_t)(strlen(asciiIn[i])+1));
        testAPI(buf,unicodeIn[i],testName,FALSE,U_ZERO_ERROR, TRUE, TRUE, func);
    }
}


void TestIDNA::testIDNToUnicode(const char* testName, TestFunc func){
    int32_t i;
    UChar buf[MAX_DEST_SIZE];
    UChar expected[MAX_DEST_SIZE];
    UErrorCode status = U_ZERO_ERROR;
    int32_t bufLen = 0;
    UParseError parseError;
    for(i=0;i< (int32_t)(sizeof(domainNames)/sizeof(domainNames[0])); i++){
        bufLen = (int32_t)strlen(domainNames[i]);
        bufLen = u_unescape(domainNames[i],buf, bufLen+1);
        func(buf,bufLen,expected,MAX_DEST_SIZE, UIDNA_ALLOW_UNASSIGNED, &parseError,&status);
        if(U_FAILURE(status)){
            errcheckln(status, "%s failed to convert domainNames[%i].Error: %s",testName, i, u_errorName(status));
            break;
        }
        testAPI(buf,expected,testName,FALSE,U_ZERO_ERROR, TRUE, TRUE, func);
         //test toUnicode with all labels in the string
        testAPI(buf,expected,testName, FALSE,U_ZERO_ERROR, TRUE, TRUE, func);
        if(U_FAILURE(status)){
            errln( "%s failed to convert domainNames[%i].Error: %s \n",testName,i, u_errorName(status));
            break;
        }
    }
    
}

void TestIDNA::testIDNToASCII(const char* testName, TestFunc func){
    int32_t i;
    UChar buf[MAX_DEST_SIZE];
    UChar expected[MAX_DEST_SIZE];
    UErrorCode status = U_ZERO_ERROR;
    int32_t bufLen = 0;
    UParseError parseError; 
    for(i=0;i< (int32_t)(sizeof(domainNames)/sizeof(domainNames[0])); i++){
        bufLen = (int32_t)strlen(domainNames[i]);
        bufLen = u_unescape(domainNames[i],buf, bufLen+1);
        func(buf,bufLen,expected,MAX_DEST_SIZE, UIDNA_ALLOW_UNASSIGNED, &parseError,&status);
        if(U_FAILURE(status)){
            errcheckln(status, "%s failed to convert domainNames[%i].Error: %s",testName,i, u_errorName(status));
            break;
        }
        testAPI(buf,expected,testName, FALSE,U_ZERO_ERROR, TRUE, TRUE, func);
        //test toASCII with all labels in the string
        testAPI(buf,expected,testName, FALSE,U_ZERO_ERROR, FALSE, TRUE, func);
        if(U_FAILURE(status)){
            errln( "%s failed to convert domainNames[%i].Error: %s \n",testName,i, u_errorName(status));
            break;
        }
    }
    
}

void TestIDNA::testCompare(const char* testName, CompareFunc func){
    int32_t i;


    UChar www[] = {0x0057, 0x0057, 0x0057, 0x002E, 0x0000};
    UChar com[] = {0x002E, 0x0043, 0x004F, 0x004D, 0x0000};
    UChar buf[MAX_DEST_SIZE]={0x0057, 0x0057, 0x0057, 0x002E, 0x0000};

    UnicodeString source(www), uni0(www),uni1(www), ascii0(www), ascii1(www);

    uni0.append(unicodeIn[0]);
    uni0.append(com);
    uni0.append((UChar)0x0000);

    uni1.append(unicodeIn[1]);
    uni1.append(com);
    uni1.append((UChar)0x0000);

    ascii0.append(asciiIn[0]);
    ascii0.append(com);
    ascii0.append((UChar)0x0000);

    ascii1.append(asciiIn[1]);
    ascii1.append(com);
    ascii1.append((UChar)0x0000);

    for(i=0;i< (int32_t)(sizeof(unicodeIn)/sizeof(unicodeIn[0])); i++){

        u_charsToUChars(asciiIn[i],buf+4, (int32_t)(strlen(asciiIn[i])+1));
        u_strcat(buf,com);

        // for every entry in unicodeIn array
        // prepend www. and append .com
        source.truncate(4);
        source.append(unicodeIn[i]);
        source.append(com);
        source.append((UChar)0x0000);
        // a) compare it with itself
        const UChar* src = source.getBuffer();
        int32_t srcLen = u_strlen(src); //subtract null

        testCompare(src,srcLen,src,srcLen,testName, func, TRUE);
        
        // b) compare it with asciiIn equivalent
        testCompare(src,srcLen,buf,u_strlen(buf),testName, func,TRUE);
        
        // c) compare it with unicodeIn not equivalent
        if(i==0){
            testCompare(src,srcLen,uni1.getBuffer(),uni1.length()-1,testName, func,FALSE);
        }else{
            testCompare(src,srcLen,uni0.getBuffer(),uni0.length()-1,testName, func,FALSE);
        }
        // d) compare it with asciiIn not equivalent
        if(i==0){
            testCompare(src,srcLen,ascii1.getBuffer(),ascii1.length()-1,testName, func,FALSE);
        }else{
            testCompare(src,srcLen,ascii0.getBuffer(),ascii0.length()-1,testName, func,FALSE);
        }

    }
}

#if 0

static int32_t
getNextSeperator(UChar *src,int32_t srcLength,
                 UChar **limit){
    if(srcLength == -1){
        int32_t i;
        for(i=0 ; ;i++){
            if(src[i] == 0){
                *limit = src + i; // point to null
                return i;
            }
            if(src[i]==0x002e){
                *limit = src + (i+1); // go past the delimiter
                return i;
            }
        }
        // we have not found the delimiter
        if(i==srcLength){
            *limit = src+srcLength;
        }
        return i;
    }else{
        int32_t i;
        for(i=0;i<srcLength;i++){
            if(src[i]==0x002e){
                *limit = src + (i+1); // go past the delimiter
                return i;
            }
        }
        // we have not found the delimiter
        if(i==srcLength){
            *limit = src+srcLength;
        }
        return i;
    }
}

void printPunycodeOutput(){

    UChar dest[MAX_DEST_SIZE];
    int32_t destCapacity=MAX_DEST_SIZE;
    UChar* start;
    UChar* limit;
    int32_t labelLen=0;
    UBool caseFlags[MAX_DEST_SIZE];
    
    for(int32_t i=0;i< sizeof(errorCases)/sizeof(errorCases[0]);i++){
        ErrorCases errorCase = errorCases[i];
        UErrorCode status = U_ZERO_ERROR;
        start = errorCase.unicode;
        int32_t srcLen = u_strlen(start);
        labelLen = getNextSeperator(start,srcLen,&limit);
        start = limit;
        labelLen=getNextSeperator(start,srcLen-labelLen,&limit);
        int32_t destLen = u_strToPunycode(dest,destCapacity,start,labelLen,caseFlags, &status);
        if(U_FAILURE(status)){
            printf("u_strToPunycode failed for index %i\n",i);
            continue;
        }
        for(int32_t j=0; j<destLen; j++){
            printf("%c",(char)dest[j]);
        }
        printf("\n");
    }
}
#endif

void TestIDNA::testErrorCases(const char* IDNToASCIIName, TestFunc IDNToASCII,
                              const char* IDNToUnicodeName, TestFunc IDNToUnicode){
    UChar buf[MAX_DEST_SIZE];
    int32_t bufLen=0;

    for(int32_t i=0;i< (int32_t)(sizeof(errorCases)/sizeof(errorCases[0]));i++){
        ErrorCases errorCase = errorCases[i];
        UChar* src =NULL;
        if(errorCase.ascii != NULL){
            bufLen =  (int32_t)strlen(errorCase.ascii);
            u_charsToUChars(errorCase.ascii,buf, bufLen+1);
        }else{
            bufLen = 1 ;
            memset(buf,0,U_SIZEOF_UCHAR*MAX_DEST_SIZE);
        }
        
        if(errorCase.unicode[0]!=0){
            src = errorCase.unicode;
        }
        // test toASCII
        testAPI(src,buf,
                IDNToASCIIName, errorCase.useSTD3ASCIIRules,
                errorCase.expected, TRUE, TRUE, IDNToASCII);
        if(errorCase.testLabel ==TRUE){
            testAPI(src,buf,
                IDNToASCIIName, errorCase.useSTD3ASCIIRules,
                errorCase.expected, FALSE,TRUE, IDNToASCII);
        }
        if(errorCase.testToUnicode ==TRUE){
            testAPI((src==NULL)? NULL : buf,src,
                    IDNToUnicodeName, errorCase.useSTD3ASCIIRules,
                    errorCase.expected, TRUE, TRUE, IDNToUnicode);   
        }

    }
    
}
/*
void TestIDNA::testConformance(const char* toASCIIName, TestFunc toASCII,
                               const char* IDNToASCIIName, TestFunc IDNToASCII,
                               const char* IDNToUnicodeName, TestFunc IDNToUnicode,
                               const char* toUnicodeName, TestFunc toUnicode){
    UChar src[MAX_DEST_SIZE];
    int32_t srcLen=0;
    UChar expected[MAX_DEST_SIZE];
    int32_t expectedLen = 0;
    for(int32_t i=0;i< (int32_t)(sizeof(conformanceTestCases)/sizeof(conformanceTestCases[0]));i++){
        const char* utf8Chars1 = conformanceTestCases[i].in;
        int32_t utf8Chars1Len = (int32_t)strlen(utf8Chars1);
        const char* utf8Chars2 = conformanceTestCases[i].out;
        int32_t utf8Chars2Len = (utf8Chars2 == NULL) ? 0 : (int32_t)strlen(utf8Chars2);

        UErrorCode status = U_ZERO_ERROR;
        u_strFromUTF8(src,MAX_DEST_SIZE,&srcLen,utf8Chars1,utf8Chars1Len,&status);
        if(U_FAILURE(status)){
            errln(UnicodeString("Conversion of UTF8 source in conformanceTestCases[") + i +UnicodeString( "].in ( ")+prettify(utf8Chars1) +UnicodeString(" ) failed. Error: ")+ UnicodeString(u_errorName(status)));
            continue;
        }
        if(utf8Chars2 != NULL){
            u_strFromUTF8(expected,MAX_DEST_SIZE,&expectedLen,utf8Chars2,utf8Chars2Len, &status);
            if(U_FAILURE(status)){
                errln(UnicodeString("Conversion of UTF8 source in conformanceTestCases[") + i +UnicodeString( "].in ( ")+prettify(utf8Chars1) +UnicodeString(" ) failed. Error: ")+ UnicodeString(u_errorName(status)));
                continue;
            }
        }
        
        if(conformanceTestCases[i].expectedStatus != U_ZERO_ERROR){
            // test toASCII
            testAPI(src,expected,
                    IDNToASCIIName, FALSE,
                    conformanceTestCases[i].expectedStatus, 
                    TRUE, 
                    (conformanceTestCases[i].expectedStatus != U_IDNA_UNASSIGNED_ERROR),
                    IDNToASCII);

            testAPI(src,expected,
                    toASCIIName, FALSE,
                    conformanceTestCases[i].expectedStatus, TRUE, 
                    (conformanceTestCases[i].expectedStatus != U_IDNA_UNASSIGNED_ERROR),
                    toASCII);
        }

        testAPI(src,src,
                IDNToUnicodeName, FALSE,
                conformanceTestCases[i].expectedStatus, TRUE, TRUE, IDNToUnicode);
        testAPI(src,src,
                toUnicodeName, FALSE,
                conformanceTestCases[i].expectedStatus, TRUE, TRUE, toUnicode);

    }
    
}
*/
// test and ascertain
// func(func(func(src))) == func(src)
void TestIDNA::testChaining(const UChar* src,int32_t numIterations,const char* testName,
                  UBool useSTD3ASCIIRules, UBool caseInsensitive, TestFunc func){
    UChar even[MAX_DEST_SIZE];
    UChar odd[MAX_DEST_SIZE];
    UChar expected[MAX_DEST_SIZE];
    int32_t i=0,evenLen=0,oddLen=0,expectedLen=0;
    UErrorCode status = U_ZERO_ERROR;
    int32_t srcLen = u_strlen(src);
    int32_t options = (useSTD3ASCIIRules == TRUE) ? UIDNA_USE_STD3_RULES : UIDNA_DEFAULT;
    UParseError parseError;

    // test null-terminated source 
    expectedLen = func(src,-1,expected,MAX_DEST_SIZE, options, &parseError, &status);
    if(U_FAILURE(status)){
        errcheckln(status, "%s null terminated source failed. Error: %s",testName, u_errorName(status));
    }
    memcpy(odd,expected,(expectedLen+1) * U_SIZEOF_UCHAR);
    memcpy(even,expected,(expectedLen+1) * U_SIZEOF_UCHAR);
    for(;i<=numIterations; i++){
        if((i%2) ==0){
            evenLen = func(odd,-1,even,MAX_DEST_SIZE,options, &parseError, &status);
            if(U_FAILURE(status)){
                errcheckln(status, "%s null terminated source failed - %s",testName, u_errorName(status));
                break;
            }
        }else{
            oddLen = func(even,-1,odd,MAX_DEST_SIZE,options, &parseError, &status);
            if(U_FAILURE(status)){
                errln("%s null terminated source failed\n",testName);
                break;
            }
        }
    }
    if(caseInsensitive ==TRUE){
        if( u_strCaseCompare(even,evenLen, expected,expectedLen, 0, &status) !=0 ||
            u_strCaseCompare(odd,oddLen, expected,expectedLen, 0, &status) !=0 ){

            errln("Chaining for %s null terminated source failed\n",testName);
        }
    }else{
        if( u_strncmp(even,expected,expectedLen) != 0 ||
            u_strncmp(odd,expected,expectedLen) !=0 ){
        
            errln("Chaining for %s null terminated source failed\n",testName);
        }
    }

    // test null-terminated source 
    status = U_ZERO_ERROR;
    expectedLen = func(src,-1,expected,MAX_DEST_SIZE,options|UIDNA_ALLOW_UNASSIGNED, &parseError, &status);
    if(U_FAILURE(status)){
        errcheckln(status, "%s null terminated source with options set failed. Error: %s",testName, u_errorName(status));
    }
    memcpy(odd,expected,(expectedLen+1) * U_SIZEOF_UCHAR);
    memcpy(even,expected,(expectedLen+1) * U_SIZEOF_UCHAR);
    for(;i<=numIterations; i++){
        if((i%2) ==0){
            evenLen = func(odd,-1,even,MAX_DEST_SIZE,options|UIDNA_ALLOW_UNASSIGNED, &parseError, &status);
            if(U_FAILURE(status)){
                errcheckln(status, "%s null terminated source with options set failed - %s",testName, u_errorName(status));
                break;
            }
        }else{
            oddLen = func(even,-1,odd,MAX_DEST_SIZE,options|UIDNA_ALLOW_UNASSIGNED, &parseError, &status);
            if(U_FAILURE(status)){
                errln("%s null terminated source with options set failed\n",testName);
                break;
            }
        }
    }
    if(caseInsensitive ==TRUE){
        if( u_strCaseCompare(even,evenLen, expected,expectedLen, 0, &status) !=0 ||
            u_strCaseCompare(odd,oddLen, expected,expectedLen, 0, &status) !=0 ){

            errln("Chaining for %s null terminated source with options set failed\n",testName);
        }
    }else{
        if( u_strncmp(even,expected,expectedLen) != 0 ||
            u_strncmp(odd,expected,expectedLen) !=0 ){
        
            errln("Chaining for %s null terminated source with options set failed\n",testName);
        }
    }


    // test source with length 
    status = U_ZERO_ERROR;
    expectedLen = func(src,srcLen,expected,MAX_DEST_SIZE,options, &parseError, &status);
    if(U_FAILURE(status)){
        errcheckln(status, "%s null terminated source failed. Error: %s",testName, u_errorName(status));
    }
    memcpy(odd,expected,(expectedLen+1) * U_SIZEOF_UCHAR);
    memcpy(even,expected,(expectedLen+1) * U_SIZEOF_UCHAR);
    for(;i<=numIterations; i++){
        if((i%2) ==0){
            evenLen = func(odd,oddLen,even,MAX_DEST_SIZE,options, &parseError, &status);
            if(U_FAILURE(status)){
                errcheckln(status, "%s source with source length failed - %s",testName, u_errorName(status));
                break;
            }
        }else{
            oddLen = func(even,evenLen,odd,MAX_DEST_SIZE,options, &parseError, &status);
            if(U_FAILURE(status)){
                errcheckln(status, "%s source with source length failed - %s",testName, u_errorName(status));
                break;
            }
        }
    }
    if(caseInsensitive ==TRUE){
        if( u_strCaseCompare(even,evenLen, expected,expectedLen, 0, &status) !=0 ||
            u_strCaseCompare(odd,oddLen, expected,expectedLen, 0, &status) !=0 ){

            errln("Chaining for %s source with source length failed\n",testName);
        }
    }else{
        if( u_strncmp(even,expected,expectedLen) != 0 ||
            u_strncmp(odd,expected,expectedLen) !=0 ){
        
            errln("Chaining for %s source with source length failed\n",testName);
        }
    }
    status = U_ZERO_ERROR;
    expectedLen = func(src,srcLen,expected,MAX_DEST_SIZE,options|UIDNA_ALLOW_UNASSIGNED, &parseError, &status);
    if(U_FAILURE(status)){
        errcheckln(status, "%s null terminated source with options set failed. Error: %s",testName, u_errorName(status));
    }
    memcpy(odd,expected,(expectedLen+1) * U_SIZEOF_UCHAR);
    memcpy(even,expected,(expectedLen+1) * U_SIZEOF_UCHAR);
    for(;i<=numIterations; i++){
        if((i%2) ==0){
            evenLen = func(odd,oddLen,even,MAX_DEST_SIZE,options|UIDNA_ALLOW_UNASSIGNED, &parseError, &status);
            if(U_FAILURE(status)){
                errcheckln(status, "%s source with source length and options set failed - %s",testName, u_errorName(status));
                break;
            }
        }else{
            oddLen = func(even,evenLen,odd,MAX_DEST_SIZE,options|UIDNA_ALLOW_UNASSIGNED, &parseError, &status);
            if(U_FAILURE(status)){
                errcheckln(status, "%s  source with source length and options set failed - %s",testName, u_errorName(status));
                break;
            }
        }
    }
    if(caseInsensitive ==TRUE){
        if( u_strCaseCompare(even,evenLen, expected,expectedLen, 0, &status) !=0 ||
            u_strCaseCompare(odd,oddLen, expected,expectedLen, 0, &status) !=0 ){

            errln("Chaining for %s  source with source length and options set failed\n",testName);
        }
    }else{
        if( u_strncmp(even,expected,expectedLen) != 0 ||
            u_strncmp(odd,expected,expectedLen) !=0 ){
        
            errln("Chaining for %s  source with source length and options set failed\n",testName);
        }
    }
}
void TestIDNA::testChaining(const char* toASCIIName, TestFunc toASCII,
                  const char* toUnicodeName, TestFunc toUnicode){
    int32_t i;
    UChar buf[MAX_DEST_SIZE];
    
    for(i=0;i< (int32_t)(sizeof(asciiIn)/sizeof(asciiIn[0])); i++){
        u_charsToUChars(asciiIn[i],buf, (int32_t)(strlen(asciiIn[i])+1));
        testChaining(buf,5,toUnicodeName, FALSE, FALSE, toUnicode);
    }
    for(i=0;i< (int32_t)(sizeof(unicodeIn)/sizeof(unicodeIn[0])); i++){
        testChaining(unicodeIn[i], 5,toASCIIName, FALSE, TRUE, toASCII);
    }
}


void TestIDNA::testRootLabelSeparator(const char* testName, CompareFunc func, 
                            const char* IDNToASCIIName, TestFunc IDNToASCII,
                            const char* IDNToUnicodeName, TestFunc IDNToUnicode){
    int32_t i;


    UChar www[] = {0x0057, 0x0057, 0x0057, 0x002E, 0x0000};
    UChar com[] = {0x002E, 0x0043, 0x004F, 0x004D, 0x002E, /* root label separator */0x0000};
    UChar buf[MAX_DEST_SIZE]={0x0057, 0x0057, 0x0057, 0x002E, 0x0000};

    UnicodeString source(www), uni0(www),uni1(www), ascii0(www), ascii1(www);

    uni0.append(unicodeIn[0]);
    uni0.append(com);
    uni0.append((UChar)0x0000);

    uni1.append(unicodeIn[1]);
    uni1.append(com);
    uni1.append((UChar)0x0000);

    ascii0.append(asciiIn[0]);
    ascii0.append(com);
    ascii0.append((UChar)0x0000);

    ascii1.append(asciiIn[1]);
    ascii1.append(com);
    ascii1.append((UChar)0x0000);

    for(i=0;i< (int32_t)(sizeof(unicodeIn)/sizeof(unicodeIn[0])); i++){

        u_charsToUChars(asciiIn[i],buf+4, (int32_t)(strlen(asciiIn[i])+1));
        u_strcat(buf,com);

        // for every entry in unicodeIn array
        // prepend www. and append .com
        source.truncate(4);
        source.append(unicodeIn[i]);
        source.append(com);
        source.append((UChar)0x0000);
        
        const UChar* src = source.getBuffer();
        int32_t srcLen = u_strlen(src); //subtract null
        
        // b) compare it with asciiIn equivalent
        testCompare(src,srcLen,buf,u_strlen(buf),testName, func,TRUE);
        
        // a) compare it with itself
        testCompare(src,srcLen,src,srcLen,testName, func,TRUE);
        
        
        // IDNToASCII comparison
        testAPI(src,buf,IDNToASCIIName,FALSE,U_ZERO_ERROR,TRUE, TRUE, IDNToASCII);
        // IDNToUnicode comparison
        testAPI(buf,src,IDNToUnicodeName, FALSE,U_ZERO_ERROR, TRUE, TRUE, IDNToUnicode);

        // c) compare it with unicodeIn not equivalent
        if(i==0){
            testCompare(src,srcLen,uni1.getBuffer(),uni1.length()-1,testName, func,FALSE);
        }else{
            testCompare(src,srcLen,uni0.getBuffer(),uni0.length()-1,testName, func,FALSE);
        }
        // d) compare it with asciiIn not equivalent
        if(i==0){
            testCompare(src,srcLen,ascii1.getBuffer(),ascii1.length()-1,testName, func,FALSE);
        }else{
            testCompare(src,srcLen,ascii0.getBuffer(),ascii0.length()-1,testName, func,FALSE);
        }
    }
}   

//---------------------------------------------
// runIndexedTest
//---------------------------------------------

extern IntlTest *createUTS46Test();

void TestIDNA::runIndexedTest( int32_t index, UBool exec, const char* &name, char* par)
{
    if (exec) logln((UnicodeString)"TestSuite IDNA API ");
    switch (index) {

        case 0: name = "TestToASCII"; if (exec) TestToASCII(); break;
        case 1: name = "TestToUnicode"; if (exec) TestToUnicode(); break;
        case 2: name = "TestIDNToASCII"; if (exec) TestIDNToASCII(); break;
        case 3: name = "TestIDNToUnicode"; if (exec) TestIDNToUnicode(); break;
        case 4: name = "TestCompare"; if (exec) TestCompare(); break;
        case 5: name = "TestErrorCases"; if (exec) TestErrorCases(); break;
        case 6: name = "TestChaining"; if (exec) TestChaining(); break;
        case 7: name = "TestRootLabelSeparator"; if(exec) TestRootLabelSeparator(); break;
        case 8: name = "TestCompareReferenceImpl"; if(exec) TestCompareReferenceImpl(); break;
        case 9: name = "TestDataFile"; if(exec) TestDataFile(); break;
#if !UCONFIG_NO_FILE_IO && !UCONFIG_NO_LEGACY_CONVERSION
        case 10: name = "TestRefIDNA"; if(exec) TestRefIDNA(); break;
        case 11: name = "TestIDNAMonkeyTest"; if(exec) TestIDNAMonkeyTest(); break;
#else
        case 10: case 11: name = "skip"; break;
#endif
        case 12: 
            {
                name = "TestConformanceTestVectors";
                if(exec){
                    logln("TestSuite IDNA conf----"); logln();
                    IdnaConfTest test;
                    callTest(test, par);
                }
                break;
            }
        case 13:
            name = "UTS46Test";
            if (exec) {
                logln("TestSuite UTS46Test---"); logln();
                LocalPointer<IntlTest> test(createUTS46Test());
                callTest(*test, par);
            }
            break;
        default: name = ""; break; /*needed to end loop*/
    }
}
void TestIDNA::TestToASCII(){
    testToASCII("uidna_toASCII", uidna_toASCII);
}
void TestIDNA::TestToUnicode(){
    testToUnicode("uidna_toUnicode", uidna_toUnicode);
}
void TestIDNA::TestIDNToASCII(){
    testIDNToASCII("uidna_IDNToASCII", uidna_IDNToASCII);
}
void TestIDNA::TestIDNToUnicode(){
    testIDNToUnicode("uidna_IDNToUnicode", uidna_IDNToUnicode);
}
void TestIDNA::TestCompare(){
    testCompare("uidna_compare",uidna_compare);
}
void TestIDNA::TestErrorCases(){
    testErrorCases( "uidna_IDNToASCII",uidna_IDNToASCII,
                    "uidna_IDNToUnicode",uidna_IDNToUnicode);
}
void TestIDNA::TestRootLabelSeparator(){
    testRootLabelSeparator( "uidna_compare",uidna_compare,
                            "uidna_IDNToASCII", uidna_IDNToASCII,
                            "uidna_IDNToUnicode",uidna_IDNToUnicode
                            );
}
void TestIDNA::TestChaining(){
    testChaining("uidna_toASCII",uidna_toASCII, "uidna_toUnicode", uidna_toUnicode);
}


static const int loopCount = 100;
static const int maxCharCount = 20;
static const int maxCodePoint = 0x10ffff;
static uint32_t
randul()
{
    static UBool initialized = FALSE;
    if (!initialized)
    {
        srand((unsigned)time(NULL));
        initialized = TRUE;
    }
    // Assume rand has at least 12 bits of precision
    uint32_t l = 0;
    for (uint32_t i=0; i<sizeof(l); ++i)
        ((char*)&l)[i] = (char)((rand() & 0x0FF0) >> 4);
    return l;
}

/**
 * Return a random integer i where 0 <= i < n.
 * A special function that gets random codepoints from planes 0,1,2 and 14
 */
static int32_t rand_uni()
{
   int32_t retVal = (int32_t)(randul()& 0x3FFFF);
   if(retVal >= 0x30000){
       retVal+=0xB0000;
   }
   return retVal;
}

static int32_t randi(int32_t n){
    return (int32_t) (randul() % (n+1));
}

void getTestSource(UnicodeString& fillIn) {
    int32_t i = 0;
    int32_t charCount = (randi(maxCharCount) + 1);
    while (i <charCount ) {
        int32_t codepoint = rand_uni();
        if(codepoint == 0x0000){
            continue;
        }
        fillIn.append((UChar32)codepoint);
        i++;
    }
       
}

UnicodeString TestIDNA::testCompareReferenceImpl(UnicodeString& src, 
                                        TestFunc refIDNA, const char* refIDNAName,
                                        TestFunc uIDNA, const char* uIDNAName,
                                        int32_t options){
  
    const UChar* srcUChars = src.getBuffer();
    UChar exp[MAX_DEST_SIZE]={0};
    int32_t expCap = MAX_DEST_SIZE, expLen=0;
    UErrorCode expStatus = U_ZERO_ERROR;
    UParseError parseError;

    logln("Comparing "+ UnicodeString(refIDNAName) 
           + " with "+ UnicodeString(uIDNAName) 
           +" for input: " + prettify(srcUChars));
    
    expLen = refIDNA(srcUChars, src.length()-1, exp, expCap,
                      options, &parseError, &expStatus);

    UChar got[MAX_DEST_SIZE]={0};
    int32_t gotCap = MAX_DEST_SIZE, gotLen=0;
    UErrorCode gotStatus = U_ZERO_ERROR;

    gotLen = uIDNA(srcUChars, src.length()-1, got, gotCap,
                   options, &parseError, &gotStatus);

    if(expStatus != gotStatus){
        errln("Did not get the expected status while comparing " + UnicodeString(refIDNAName)
               + " with " + UnicodeString(uIDNAName)
               + " Expected: " + UnicodeString(u_errorName(expStatus))
               + " Got: " + UnicodeString(u_errorName(gotStatus))
               + " for Source: "+ prettify(srcUChars) 
               + " Options: " + options);
        return UnicodeString("");
    }
    
    // now we know that both implementations yielded same error
    if(U_SUCCESS(expStatus)){
        // compare the outputs if status == U_ZERO_ERROR
        if(u_strCompare(exp, expLen, got, gotLen, TRUE) != 0){
            errln("Did not get the expected output while comparing " + UnicodeString(refIDNAName)
               + " with " + UnicodeString(uIDNAName)
               + " Expected: " + prettify(UnicodeString(exp, expLen))
               + " Got: " + prettify(UnicodeString(got, gotLen))
               + " for Source: "+ prettify(srcUChars) 
               + " Options: " + options);
        }
        return UnicodeString(exp, expLen);

    }else{
        logln("Got the same error while comparing " 
            + UnicodeString(refIDNAName) 
            + " with "+ UnicodeString(uIDNAName) 
            +" for input: " + prettify(srcUChars));
    }
    return UnicodeString("");
}

void TestIDNA::testCompareReferenceImpl(const UChar* src, int32_t srcLen){
    UnicodeString label(src,srcLen);
    label.append((UChar)0x0000);

    //test idnaref_toASCII and idnare
    UnicodeString asciiLabel = testCompareReferenceImpl(label, 
                                                      idnaref_toASCII, "idnaref_toASCII",
                                                      uidna_toASCII, "uidna_toASCII",
                                                      UIDNA_ALLOW_UNASSIGNED);
    testCompareReferenceImpl(label, 
                              idnaref_toASCII, "idnaref_toASCII",
                              uidna_toASCII, "uidna_toASCII",
                              UIDNA_DEFAULT);
    testCompareReferenceImpl(label, 
                              idnaref_toASCII, "idnaref_toASCII",
                              uidna_toASCII, "uidna_toASCII",
                              UIDNA_USE_STD3_RULES);
    testCompareReferenceImpl(label, 
                              idnaref_toASCII, "idnaref_toASCII",
                              uidna_toASCII, "uidna_toASCII",
                              UIDNA_USE_STD3_RULES | UIDNA_ALLOW_UNASSIGNED);

    if(asciiLabel.length()!=0){
        asciiLabel.append((UChar)0x0000);

        // test toUnciode
        testCompareReferenceImpl(asciiLabel, 
                                  idnaref_toUnicode, "idnaref_toUnicode",
                                  uidna_toUnicode, "uidna_toUnicode",
                                  UIDNA_ALLOW_UNASSIGNED);
        testCompareReferenceImpl(asciiLabel, 
                                  idnaref_toUnicode, "idnaref_toUnicode",
                                  uidna_toUnicode, "uidna_toUnicode",
                                  UIDNA_DEFAULT);
        testCompareReferenceImpl(asciiLabel, 
                                  idnaref_toUnicode, "idnaref_toUnicode",
                                  uidna_toUnicode, "uidna_toUnicode",
                                  UIDNA_USE_STD3_RULES);
        testCompareReferenceImpl(asciiLabel, 
                                  idnaref_toUnicode, "idnaref_toUnicode",
                                  uidna_toUnicode, "uidna_toUnicode",
                                  UIDNA_USE_STD3_RULES | UIDNA_ALLOW_UNASSIGNED);
    }

}
const char* failures[] ={
    "\\uAA42\\U0001F8DD\\U00019D01\\U000149A3\\uD385\\U000EE0F5\\U00018B92\\U000179D1\\U00018624\\U0002227F\\U000E83C0\\U000E8DCD\\u5460\\U00017F34\\U0001570B\\u43D1\\U0002C9C9\\U000281EC\\u2105\\U000180AE\\uC5D4",
    "\\U0002F5A6\\uD638\\u0D0A\\u9E9C\\uFE5B\\U0001FCCB\\u66C4",
};

void TestIDNA::TestIDNAMonkeyTest(){
    UnicodeString source;
    UErrorCode status = U_ZERO_ERROR;
    int i;

    getInstance(status);    // Init prep
    if (U_FAILURE(status)) {
        dataerrln("Test could not initialize. Got %s", u_errorName(status));
        return;
    }

    for(i=0; i<loopCount; i++){
        source.truncate(0);
        getTestSource(source);
        source.append((UChar)0x0000);
        const UChar* src = source.getBuffer();
        testCompareReferenceImpl(src,source.length()-1);
        testCompareReferenceImpl(src,source.length()-1);
    }
    
    /* for debugging */
    for (i=0; i<(int)(sizeof(failures)/sizeof(failures[0])); i++){
        source.truncate(0);
        source.append( UnicodeString(failures[i], -1, US_INV) );
        source = source.unescape();
        source.append((UChar)0x0000);
        const UChar *src = source.getBuffer();
        testCompareReferenceImpl(src,source.length()-1);
        //debug(source.getBuffer(),source.length(),UIDNA_ALLOW_UNASSIGNED);
    }

    
    source.truncate(0);
    source.append(UNICODE_STRING_SIMPLE("\\uCF18\\U00021161\\U000EEF11\\U0002BB82\\U0001D63C"));
    debug(source.getBuffer(),source.length(),UIDNA_ALLOW_UNASSIGNED);
    
    { // test deletion of code points
        UnicodeString source("\\u043f\\u00AD\\u034f\\u043e\\u0447\\u0435\\u043c\\u0443\\u0436\\u0435\\u043e\\u043d\\u0438\\u043d\\u0435\\u0433\\u043e\\u0432\\u043e\\u0440\\u044f\\u0442\\u043f\\u043e\\u0440\\u0443\\u0441\\u0441\\u043a\\u0438\\u0000", -1, US_INV);
        source = source.unescape();
        UnicodeString expected("\\u043f\\u043e\\u0447\\u0435\\u043c\\u0443\\u0436\\u0435\\u043e\\u043d\\u0438\\u043d\\u0435\\u0433\\u043e\\u0432\\u043e\\u0440\\u044f\\u0442\\u043f\\u043e\\u0440\\u0443\\u0441\\u0441\\u043a\\u0438\\u0000", -1, US_INV);
        expected = expected.unescape();
        UnicodeString ascii("xn--b1abfaaepdrnnbgefbadotcwatmq2g4l");
        ascii.append((UChar)0x0000);
        testAPI(source.getBuffer(),ascii.getBuffer(), "uidna_toASCII", FALSE, U_ZERO_ERROR, TRUE, TRUE, uidna_toASCII);
        
        testAPI(source.getBuffer(),ascii.getBuffer(), "idnaref_toASCII", FALSE, U_ZERO_ERROR, TRUE, TRUE, idnaref_toASCII);

        testCompareReferenceImpl(source.getBuffer(), source.length()-1);
    }

}

void TestIDNA::TestCompareReferenceImpl(){
    
    UChar src [2] = {0,0};
    int32_t srcLen = 0;
    

    for(int32_t i = 0x40000 ; i< 0x10ffff; i++){
        if(quick==TRUE && i> 0x1FFFF){
            return;
        }
        if(i >= 0x30000 && i <= 0xF0000){
           i+=0xB0000;
        }
        if(i>0xFFFF){
           src[0] = U16_LEAD(i);
           src[1] = U16_TRAIL(i);
           srcLen =2;
        }else{
            src[0] = (UChar)i;
            src[1] = 0;
            srcLen = 1;
        }
        testCompareReferenceImpl(src, srcLen);
    }
}

void TestIDNA::TestRefIDNA(){
    UErrorCode status = U_ZERO_ERROR;
    getInstance(status);    // Init prep
    if (U_FAILURE(status)) {
        if (status == U_FILE_ACCESS_ERROR) {
            dataerrln("Test could not initialize. Got %s", u_errorName(status));
        }
        return;
    }

    testToASCII("idnaref_toASCII", idnaref_toASCII);
    testToUnicode("idnaref_toUnicode", idnaref_toUnicode);
    testIDNToASCII("idnaref_IDNToASCII", idnaref_IDNToASCII);
    testIDNToUnicode("idnaref_IDNToUnicode", idnaref_IDNToUnicode);
    testCompare("idnaref_compare",idnaref_compare);
    testErrorCases( "idnaref_IDNToASCII",idnaref_IDNToASCII,
                    "idnaref_IDNToUnicode",idnaref_IDNToUnicode);
    testChaining("idnaref_toASCII",idnaref_toASCII, "idnaref_toUnicode", idnaref_toUnicode);

    testRootLabelSeparator( "idnaref_compare",idnaref_compare,
                            "idnaref_IDNToASCII", idnaref_IDNToASCII,
                            "idnaref_IDNToUnicode",idnaref_IDNToUnicode
                            );
    testChaining("idnaref_toASCII",idnaref_toASCII, "idnaref_toUnicode", idnaref_toUnicode);
}


void TestIDNA::TestDataFile(){
     testData(*this);
}
TestIDNA::~TestIDNA(){
    if(gPrep!=NULL){
        delete gPrep;
        gPrep = NULL;
    }
}

NamePrepTransform* TestIDNA::gPrep = NULL;

NamePrepTransform* TestIDNA::getInstance(UErrorCode& status){
    if(TestIDNA::gPrep == NULL){
        UParseError parseError;
        TestIDNA::gPrep = NamePrepTransform::createInstance(parseError, status);
        if(TestIDNA::gPrep ==NULL){
           //status = U_MEMORY_ALLOCATION_ERROR;
           return NULL;
        }
    }
    return TestIDNA::gPrep;

}
#endif /* #if !UCONFIG_NO_IDNA */
