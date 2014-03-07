/*
*******************************************************************************
*
*   Copyright (C) 2003-2010, International Business Machines
*   Corporation and others.  All Rights Reserved.
*
*******************************************************************************
*   file name:  convtest.cpp
*   encoding:   US-ASCII
*   tab size:   8 (not used)
*   indentation:4
*
*   created on: 2003jul15
*   created by: Markus W. Scherer
*
*   Test file for data-driven conversion tests.
*/

#include "unicode/utypes.h"

#if !UCONFIG_NO_LEGACY_CONVERSION
/*
 * Note: Turning off all of convtest.cpp if !UCONFIG_NO_LEGACY_CONVERSION
 * is slightly unnecessary - it removes tests for Unicode charsets
 * like UTF-8 that should work.
 * However, there is no easy way for the test to detect whether a test case
 * is for a Unicode charset, so it would be difficult to only exclude those.
 * Also, regular testing of ICU is done with all modules on, therefore
 * not testing conversion for a custom configuration like this should be ok.
 */

#include "unicode/ucnv.h"
#include "unicode/unistr.h"
#include "unicode/parsepos.h"
#include "unicode/uniset.h"
#include "unicode/ustring.h"
#include "unicode/ures.h"
#include "convtest.h"
#include "unicode/tstdtmod.h"
#include <string.h>
#include <stdlib.h>

#define LENGTHOF(array) (int32_t)(sizeof(array)/sizeof((array)[0]))

enum {
    // characters used in test data for callbacks
    SUB_CB='?',
    SKIP_CB='0',
    STOP_CB='.',
    ESC_CB='&'
};

ConversionTest::ConversionTest() {
    UErrorCode errorCode=U_ZERO_ERROR;
    utf8Cnv=ucnv_open("UTF-8", &errorCode);
    ucnv_setToUCallBack(utf8Cnv, UCNV_TO_U_CALLBACK_STOP, NULL, NULL, NULL, &errorCode);
    if(U_FAILURE(errorCode)) {
        errln("unable to open UTF-8 converter");
    }
}

ConversionTest::~ConversionTest() {
    ucnv_close(utf8Cnv);
}

void
ConversionTest::runIndexedTest(int32_t index, UBool exec, const char *&name, char * /*par*/) {
    if (exec) logln("TestSuite ConversionTest: ");
    switch (index) {
#if !UCONFIG_NO_FILE_IO
        case 0: name="TestToUnicode"; if (exec) TestToUnicode(); break;
        case 1: name="TestFromUnicode"; if (exec) TestFromUnicode(); break;
        case 2: name="TestGetUnicodeSet"; if (exec) TestGetUnicodeSet(); break;
#else
        case 0:
        case 1:
        case 2: name="skip"; break;
#endif
        case 3: name="TestGetUnicodeSet2"; if (exec) TestGetUnicodeSet2(); break;
        default: name=""; break; //needed to end loop
    }
}

// test data interface ----------------------------------------------------- ***

void
ConversionTest::TestToUnicode() {
    ConversionCase cc;
    char charset[100], cbopt[4];
    const char *option;
    UnicodeString s, unicode;
    int32_t offsetsLength;
    UConverterToUCallback callback;

    TestDataModule *dataModule;
    TestData *testData;
    const DataMap *testCase;
    UErrorCode errorCode;
    int32_t i;

    errorCode=U_ZERO_ERROR;
    dataModule=TestDataModule::getTestDataModule("conversion", *this, errorCode);
    if(U_SUCCESS(errorCode)) {
        testData=dataModule->createTestData("toUnicode", errorCode);
        if(U_SUCCESS(errorCode)) {
            for(i=0; testData->nextCase(testCase, errorCode); ++i) {
                if(U_FAILURE(errorCode)) {
                    errln("error retrieving conversion/toUnicode test case %d - %s",
                            i, u_errorName(errorCode));
                    errorCode=U_ZERO_ERROR;
                    continue;
                }

                cc.caseNr=i;

                s=testCase->getString("charset", errorCode);
                s.extract(0, 0x7fffffff, charset, sizeof(charset), "");
                cc.charset=charset;

                cc.bytes=testCase->getBinary(cc.bytesLength, "bytes", errorCode);
                unicode=testCase->getString("unicode", errorCode);
                cc.unicode=unicode.getBuffer();
                cc.unicodeLength=unicode.length();

                offsetsLength=0;
                cc.offsets=testCase->getIntVector(offsetsLength, "offsets", errorCode);
                if(offsetsLength==0) {
                    cc.offsets=NULL;
                } else if(offsetsLength!=unicode.length()) {
                    errln("toUnicode[%d] unicode[%d] and offsets[%d] must have the same length",
                            i, unicode.length(), offsetsLength);
                    errorCode=U_ILLEGAL_ARGUMENT_ERROR;
                }

                cc.finalFlush= 0!=testCase->getInt28("flush", errorCode);
                cc.fallbacks= 0!=testCase->getInt28("fallbacks", errorCode);

                s=testCase->getString("errorCode", errorCode);
                if(s==UNICODE_STRING("invalid", 7)) {
                    cc.outErrorCode=U_INVALID_CHAR_FOUND;
                } else if(s==UNICODE_STRING("illegal", 7)) {
                    cc.outErrorCode=U_ILLEGAL_CHAR_FOUND;
                } else if(s==UNICODE_STRING("truncated", 9)) {
                    cc.outErrorCode=U_TRUNCATED_CHAR_FOUND;
                } else if(s==UNICODE_STRING("illesc", 6)) {
                    cc.outErrorCode=U_ILLEGAL_ESCAPE_SEQUENCE;
                } else if(s==UNICODE_STRING("unsuppesc", 9)) {
                    cc.outErrorCode=U_UNSUPPORTED_ESCAPE_SEQUENCE;
                } else {
                    cc.outErrorCode=U_ZERO_ERROR;
                }

                s=testCase->getString("callback", errorCode);
                s.extract(0, 0x7fffffff, cbopt, sizeof(cbopt), "");
                cc.cbopt=cbopt;
                switch(cbopt[0]) {
                case SUB_CB:
                    callback=UCNV_TO_U_CALLBACK_SUBSTITUTE;
                    break;
                case SKIP_CB:
                    callback=UCNV_TO_U_CALLBACK_SKIP;
                    break;
                case STOP_CB:
                    callback=UCNV_TO_U_CALLBACK_STOP;
                    break;
                case ESC_CB:
                    callback=UCNV_TO_U_CALLBACK_ESCAPE;
                    break;
                default:
                    callback=NULL;
                    break;
                }
                option=callback==NULL ? cbopt : cbopt+1;
                if(*option==0) {
                    option=NULL;
                }

                cc.invalidChars=testCase->getBinary(cc.invalidLength, "invalidChars", errorCode);

                if(U_FAILURE(errorCode)) {
                    errln("error parsing conversion/toUnicode test case %d - %s",
                            i, u_errorName(errorCode));
                    errorCode=U_ZERO_ERROR;
                } else {
                    logln("TestToUnicode[%d] %s", i, charset);
                    ToUnicodeCase(cc, callback, option);
                }
            }
            delete testData;
        }
        delete dataModule;
    }
    else {
        dataerrln("Could not load test conversion data");
    }
}

void
ConversionTest::TestFromUnicode() {
    ConversionCase cc;
    char charset[100], cbopt[4];
    const char *option;
    UnicodeString s, unicode, invalidUChars;
    int32_t offsetsLength, index;
    UConverterFromUCallback callback;

    TestDataModule *dataModule;
    TestData *testData;
    const DataMap *testCase;
    const UChar *p;
    UErrorCode errorCode;
    int32_t i, length;

    errorCode=U_ZERO_ERROR;
    dataModule=TestDataModule::getTestDataModule("conversion", *this, errorCode);
    if(U_SUCCESS(errorCode)) {
        testData=dataModule->createTestData("fromUnicode", errorCode);
        if(U_SUCCESS(errorCode)) {
            for(i=0; testData->nextCase(testCase, errorCode); ++i) {
                if(U_FAILURE(errorCode)) {
                    errln("error retrieving conversion/fromUnicode test case %d - %s",
                            i, u_errorName(errorCode));
                    errorCode=U_ZERO_ERROR;
                    continue;
                }

                cc.caseNr=i;

                s=testCase->getString("charset", errorCode);
                s.extract(0, 0x7fffffff, charset, sizeof(charset), "");
                cc.charset=charset;

                unicode=testCase->getString("unicode", errorCode);
                cc.unicode=unicode.getBuffer();
                cc.unicodeLength=unicode.length();
                cc.bytes=testCase->getBinary(cc.bytesLength, "bytes", errorCode);

                offsetsLength=0;
                cc.offsets=testCase->getIntVector(offsetsLength, "offsets", errorCode);
                if(offsetsLength==0) {
                    cc.offsets=NULL;
                } else if(offsetsLength!=cc.bytesLength) {
                    errln("fromUnicode[%d] bytes[%d] and offsets[%d] must have the same length",
                            i, cc.bytesLength, offsetsLength);
                    errorCode=U_ILLEGAL_ARGUMENT_ERROR;
                }

                cc.finalFlush= 0!=testCase->getInt28("flush", errorCode);
                cc.fallbacks= 0!=testCase->getInt28("fallbacks", errorCode);

                s=testCase->getString("errorCode", errorCode);
                if(s==UNICODE_STRING("invalid", 7)) {
                    cc.outErrorCode=U_INVALID_CHAR_FOUND;
                } else if(s==UNICODE_STRING("illegal", 7)) {
                    cc.outErrorCode=U_ILLEGAL_CHAR_FOUND;
                } else if(s==UNICODE_STRING("truncated", 9)) {
                    cc.outErrorCode=U_TRUNCATED_CHAR_FOUND;
                } else {
                    cc.outErrorCode=U_ZERO_ERROR;
                }

                s=testCase->getString("callback", errorCode);
                cc.setSub=0; // default: no subchar

                if((index=s.indexOf((UChar)0))>0) {
                    // read NUL-separated subchar first, if any
                    // copy the subchar from Latin-1 characters
                    // start after the NUL
                    p=s.getTerminatedBuffer();
                    length=index+1;
                    p+=length;
                    length=s.length()-length;
                    if(length<=0 || length>=(int32_t)sizeof(cc.subchar)) {
                        errorCode=U_ILLEGAL_ARGUMENT_ERROR;
                    } else {
                        int32_t j;

                        for(j=0; j<length; ++j) {
                            cc.subchar[j]=(char)p[j];
                        }
                        // NUL-terminate the subchar
                        cc.subchar[j]=0;
                        cc.setSub=1;
                    }

                    // remove the NUL and subchar from s
                    s.truncate(index);
                } else if((index=s.indexOf((UChar)0x3d))>0) /* '=' */ {
                    // read a substitution string, separated by an equal sign
                    p=s.getBuffer()+index+1;
                    length=s.length()-(index+1);
                    if(length<0 || length>=LENGTHOF(cc.subString)) {
                        errorCode=U_ILLEGAL_ARGUMENT_ERROR;
                    } else {
                        u_memcpy(cc.subString, p, length);
                        // NUL-terminate the subString
                        cc.subString[length]=0;
                        cc.setSub=-1;
                    }

                    // remove the equal sign and subString from s
                    s.truncate(index);
                }

                s.extract(0, 0x7fffffff, cbopt, sizeof(cbopt), "");
                cc.cbopt=cbopt;
                switch(cbopt[0]) {
                case SUB_CB:
                    callback=UCNV_FROM_U_CALLBACK_SUBSTITUTE;
                    break;
                case SKIP_CB:
                    callback=UCNV_FROM_U_CALLBACK_SKIP;
                    break;
                case STOP_CB:
                    callback=UCNV_FROM_U_CALLBACK_STOP;
                    break;
                case ESC_CB:
                    callback=UCNV_FROM_U_CALLBACK_ESCAPE;
                    break;
                default:
                    callback=NULL;
                    break;
                }
                option=callback==NULL ? cbopt : cbopt+1;
                if(*option==0) {
                    option=NULL;
                }

                invalidUChars=testCase->getString("invalidUChars", errorCode);
                cc.invalidUChars=invalidUChars.getBuffer();
                cc.invalidLength=invalidUChars.length();

                if(U_FAILURE(errorCode)) {
                    errln("error parsing conversion/fromUnicode test case %d - %s",
                            i, u_errorName(errorCode));
                    errorCode=U_ZERO_ERROR;
                } else {
                    logln("TestFromUnicode[%d] %s", i, charset);
                    FromUnicodeCase(cc, callback, option);
                }
            }
            delete testData;
        }
        delete dataModule;
    }
    else {
        dataerrln("Could not load test conversion data");
    }
}

static const UChar ellipsis[]={ 0x2e, 0x2e, 0x2e };

void
ConversionTest::TestGetUnicodeSet() {
    char charset[100];
    UnicodeString s, map, mapnot;
    int32_t which;

    ParsePosition pos;
    UnicodeSet cnvSet, mapSet, mapnotSet, diffSet;
    UnicodeSet *cnvSetPtr = &cnvSet;
    LocalUConverterPointer cnv;

    TestDataModule *dataModule;
    TestData *testData;
    const DataMap *testCase;
    UErrorCode errorCode;
    int32_t i;

    errorCode=U_ZERO_ERROR;
    dataModule=TestDataModule::getTestDataModule("conversion", *this, errorCode);
    if(U_SUCCESS(errorCode)) {
        testData=dataModule->createTestData("getUnicodeSet", errorCode);
        if(U_SUCCESS(errorCode)) {
            for(i=0; testData->nextCase(testCase, errorCode); ++i) {
                if(U_FAILURE(errorCode)) {
                    errln("error retrieving conversion/getUnicodeSet test case %d - %s",
                            i, u_errorName(errorCode));
                    errorCode=U_ZERO_ERROR;
                    continue;
                }

                s=testCase->getString("charset", errorCode);
                s.extract(0, 0x7fffffff, charset, sizeof(charset), "");

                map=testCase->getString("map", errorCode);
                mapnot=testCase->getString("mapnot", errorCode);

                which=testCase->getInt28("which", errorCode);

                if(U_FAILURE(errorCode)) {
                    errln("error parsing conversion/getUnicodeSet test case %d - %s",
                            i, u_errorName(errorCode));
                    errorCode=U_ZERO_ERROR;
                    continue;
                }

                // test this test case
                mapSet.clear();
                mapnotSet.clear();

                pos.setIndex(0);
                mapSet.applyPattern(map, pos, 0, NULL, errorCode);
                if(U_FAILURE(errorCode) || pos.getIndex()!=map.length()) {
                    errln("error creating the map set for conversion/getUnicodeSet test case %d - %s\n"
                          "    error index %d  index %d  U+%04x",
                            i, u_errorName(errorCode), pos.getErrorIndex(), pos.getIndex(), map.char32At(pos.getIndex()));
                    errorCode=U_ZERO_ERROR;
                    continue;
                }

                pos.setIndex(0);
                mapnotSet.applyPattern(mapnot, pos, 0, NULL, errorCode);
                if(U_FAILURE(errorCode) || pos.getIndex()!=mapnot.length()) {
                    errln("error creating the mapnot set for conversion/getUnicodeSet test case %d - %s\n"
                          "    error index %d  index %d  U+%04x",
                            i, u_errorName(errorCode), pos.getErrorIndex(), pos.getIndex(), mapnot.char32At(pos.getIndex()));
                    errorCode=U_ZERO_ERROR;
                    continue;
                }

                logln("TestGetUnicodeSet[%d] %s", i, charset);

                cnv.adoptInstead(cnv_open(charset, errorCode));
                if(U_FAILURE(errorCode)) {
                    errcheckln(errorCode, "error opening \"%s\" for conversion/getUnicodeSet test case %d - %s",
                            charset, i, u_errorName(errorCode));
                    errorCode=U_ZERO_ERROR;
                    continue;
                }

                ucnv_getUnicodeSet(cnv.getAlias(), cnvSetPtr->toUSet(), (UConverterUnicodeSet)which, &errorCode);

                if(U_FAILURE(errorCode)) {
                    errln("error in ucnv_getUnicodeSet(\"%s\") for conversion/getUnicodeSet test case %d - %s",
                            charset, i, u_errorName(errorCode));
                    errorCode=U_ZERO_ERROR;
                    continue;
                }

                // are there items that must be in cnvSet but are not?
                (diffSet=mapSet).removeAll(cnvSet);
                if(!diffSet.isEmpty()) {
                    diffSet.toPattern(s, TRUE);
                    if(s.length()>100) {
                        s.replace(100, 0x7fffffff, ellipsis, LENGTHOF(ellipsis));
                    }
                    errln("error: ucnv_getUnicodeSet(\"%s\") is missing items - conversion/getUnicodeSet test case %d",
                            charset, i);
                    errln(s);
                }

                // are there items that must not be in cnvSet but are?
                (diffSet=mapnotSet).retainAll(cnvSet);
                if(!diffSet.isEmpty()) {
                    diffSet.toPattern(s, TRUE);
                    if(s.length()>100) {
                        s.replace(100, 0x7fffffff, ellipsis, LENGTHOF(ellipsis));
                    }
                    errln("error: ucnv_getUnicodeSet(\"%s\") contains unexpected items - conversion/getUnicodeSet test case %d",
                            charset, i);
                    errln(s);
                }
            }
            delete testData;
        }
        delete dataModule;
    }
    else {
        dataerrln("Could not load test conversion data");
    }
}

U_CDECL_BEGIN
static void U_CALLCONV
getUnicodeSetCallback(const void *context,
                      UConverterFromUnicodeArgs * /*fromUArgs*/,
                      const UChar* /*codeUnits*/,
                      int32_t /*length*/,
                      UChar32 codePoint,
                      UConverterCallbackReason reason,
                      UErrorCode *pErrorCode) {
    if(reason<=UCNV_IRREGULAR) {
        ((UnicodeSet *)context)->remove(codePoint);  // the converter cannot convert this code point
        *pErrorCode=U_ZERO_ERROR;                    // skip
    }  // else ignore the reset, close and clone calls.
}
U_CDECL_END

// Compare ucnv_getUnicodeSet() with the set of characters that can be converted.
void
ConversionTest::TestGetUnicodeSet2() {
    // Build a string with all code points.
    UChar32 cpLimit;
    int32_t s0Length;
    if(quick) {
        cpLimit=s0Length=0x10000;  // BMP only
    } else {
        cpLimit=0x110000;
        s0Length=0x10000+0x200000;  // BMP + surrogate pairs
    }
    UChar *s0=new UChar[s0Length];
    if(s0==NULL) {
        return;
    }
    UChar *s=s0;
    UChar32 c;
    UChar c2;
    // low BMP
    for(c=0; c<=0xd7ff; ++c) {
        *s++=(UChar)c;
    }
    // trail surrogates
    for(c=0xdc00; c<=0xdfff; ++c) {
        *s++=(UChar)c;
    }
    // lead surrogates
    // (after trails so that there is not even one surrogate pair in between)
    for(c=0xd800; c<=0xdbff; ++c) {
        *s++=(UChar)c;
    }
    // high BMP
    for(c=0xe000; c<=0xffff; ++c) {
        *s++=(UChar)c;
    }
    // supplementary code points = surrogate pairs
    if(cpLimit==0x110000) {
        for(c=0xd800; c<=0xdbff; ++c) {
            for(c2=0xdc00; c2<=0xdfff; ++c2) {
                *s++=(UChar)c;
                *s++=c2;
            }
        }
    }

    static const char *const cnvNames[]={
        "UTF-8",
        "UTF-7",
        "UTF-16",
        "US-ASCII",
        "ISO-8859-1",
        "windows-1252",
        "Shift-JIS",
        "ibm-1390",  // EBCDIC_STATEFUL table
        "ibm-16684",  // DBCS-only extension table based on EBCDIC_STATEFUL table
        "HZ",
        "ISO-2022-JP",
        "JIS7",
        "ISO-2022-CN",
        "ISO-2022-CN-EXT",
        "LMBCS"
    };
    LocalUConverterPointer cnv;
    char buffer[1024];
    int32_t i;
    for(i=0; i<LENGTHOF(cnvNames); ++i) {
        UErrorCode errorCode=U_ZERO_ERROR;
        cnv.adoptInstead(cnv_open(cnvNames[i], errorCode));
        if(U_FAILURE(errorCode)) {
            errcheckln(errorCode, "failed to open converter %s - %s", cnvNames[i], u_errorName(errorCode));
            continue;
        }
        UnicodeSet expected;
        ucnv_setFromUCallBack(cnv.getAlias(), getUnicodeSetCallback, &expected, NULL, NULL, &errorCode);
        if(U_FAILURE(errorCode)) {
            errln("failed to set the callback on converter %s - %s", cnvNames[i], u_errorName(errorCode));
            continue;
        }
        UConverterUnicodeSet which;
        for(which=UCNV_ROUNDTRIP_SET; which<UCNV_SET_COUNT; which=(UConverterUnicodeSet)((int)which+1)) {
            if(which==UCNV_ROUNDTRIP_AND_FALLBACK_SET) {
                ucnv_setFallback(cnv.getAlias(), TRUE);
            }
            expected.add(0, cpLimit-1);
            s=s0;
            UBool flush;
            do {
                char *t=buffer;
                flush=(UBool)(s==s0+s0Length);
                ucnv_fromUnicode(cnv.getAlias(), &t, buffer+sizeof(buffer), (const UChar **)&s, s0+s0Length, NULL, flush, &errorCode);
                if(U_FAILURE(errorCode)) {
                    if(errorCode==U_BUFFER_OVERFLOW_ERROR) {
                        errorCode=U_ZERO_ERROR;
                        continue;
                    } else {
                        break;  // unexpected error, should not occur
                    }
                }
            } while(!flush);
            UnicodeSet set;
            ucnv_getUnicodeSet(cnv.getAlias(), set.toUSet(), which, &errorCode);
            if(cpLimit<0x110000) {
                set.remove(cpLimit, 0x10ffff);
            }
            if(which==UCNV_ROUNDTRIP_SET) {
                // ignore PUA code points because they will be converted even if they
                // are fallbacks and when other fallbacks are turned off,
                // but ucnv_getUnicodeSet(UCNV_ROUNDTRIP_SET) delivers true roundtrips
                expected.remove(0xe000, 0xf8ff);
                expected.remove(0xf0000, 0xffffd);
                expected.remove(0x100000, 0x10fffd);
                set.remove(0xe000, 0xf8ff);
                set.remove(0xf0000, 0xffffd);
                set.remove(0x100000, 0x10fffd);
            }
            if(set!=expected) {
                // First try to see if we have different sets because ucnv_getUnicodeSet()
                // added strings: The above conversion method does not tell us what strings might be convertible.
                // Remove strings from the set and compare again.
                // Unfortunately, there are no good, direct set methods for finding out whether there are strings
                // in the set, nor for enumerating or removing just them.
                // Intersect all code points with the set. The intersection will not contain strings.
                UnicodeSet temp(0, 0x10ffff);
                temp.retainAll(set);
                set=temp;
            }
            if(set!=expected) {
                UnicodeSet diffSet;
                UnicodeString out;

                // are there items that must be in the set but are not?
                (diffSet=expected).removeAll(set);
                if(!diffSet.isEmpty()) {
                    diffSet.toPattern(out, TRUE);
                    if(out.length()>100) {
                        out.replace(100, 0x7fffffff, ellipsis, LENGTHOF(ellipsis));
                    }
                    errln("error: ucnv_getUnicodeSet(\"%s\") is missing items - which set: %d",
                            cnvNames[i], which);
                    errln(out);
                }

                // are there items that must not be in the set but are?
                (diffSet=set).removeAll(expected);
                if(!diffSet.isEmpty()) {
                    diffSet.toPattern(out, TRUE);
                    if(out.length()>100) {
                        out.replace(100, 0x7fffffff, ellipsis, LENGTHOF(ellipsis));
                    }
                    errln("error: ucnv_getUnicodeSet(\"%s\") contains unexpected items - which set: %d",
                            cnvNames[i], which);
                    errln(out);
                }
            }
        }
    }

    delete [] s0;
}

// open testdata or ICU data converter ------------------------------------- ***

UConverter *
ConversionTest::cnv_open(const char *name, UErrorCode &errorCode) {
    if(name!=NULL && *name=='*') {
        /* loadTestData(): set the data directory */
        return ucnv_openPackage(loadTestData(errorCode), name+1, &errorCode);
    } else if(name!=NULL && *name=='+') {
        return ucnv_open((name+1), &errorCode);
    } else {
        return ucnv_open(name, &errorCode);
    }
}

// output helpers ---------------------------------------------------------- ***

static inline char
hexDigit(uint8_t digit) {
    return digit<=9 ? (char)('0'+digit) : (char)('a'-10+digit);
}

static char *
printBytes(const uint8_t *bytes, int32_t length, char *out) {
    uint8_t b;

    if(length>0) {
        b=*bytes++;
        --length;
        *out++=hexDigit((uint8_t)(b>>4));
        *out++=hexDigit((uint8_t)(b&0xf));
    }

    while(length>0) {
        b=*bytes++;
        --length;
        *out++=' ';
        *out++=hexDigit((uint8_t)(b>>4));
        *out++=hexDigit((uint8_t)(b&0xf));
    }
    *out++=0;
    return out;
}

static char *
printUnicode(const UChar *unicode, int32_t length, char *out) {
    UChar32 c;
    int32_t i;

    for(i=0; i<length;) {
        if(i>0) {
            *out++=' ';
        }
        U16_NEXT(unicode, i, length, c);
        // write 4..6 digits
        if(c>=0x100000) {
            *out++='1';
        }
        if(c>=0x10000) {
            *out++=hexDigit((uint8_t)((c>>16)&0xf));
        }
        *out++=hexDigit((uint8_t)((c>>12)&0xf));
        *out++=hexDigit((uint8_t)((c>>8)&0xf));
        *out++=hexDigit((uint8_t)((c>>4)&0xf));
        *out++=hexDigit((uint8_t)(c&0xf));
    }
    *out++=0;
    return out;
}

static char *
printOffsets(const int32_t *offsets, int32_t length, char *out) {
    int32_t i, o, d;

    if(offsets==NULL) {
        length=0;
    }

    for(i=0; i<length; ++i) {
        if(i>0) {
            *out++=' ';
        }
        o=offsets[i];

        // print all offsets with 2 characters each (-x, -9..99, xx)
        if(o<-9) {
            *out++='-';
            *out++='x';
        } else if(o<0) {
            *out++='-';
            *out++=(char)('0'-o);
        } else if(o<=99) {
            *out++=(d=o/10)==0 ? ' ' : (char)('0'+d);
            *out++=(char)('0'+o%10);
        } else /* o>99 */ {
            *out++='x';
            *out++='x';
        }
    }
    *out++=0;
    return out;
}

// toUnicode test worker functions ----------------------------------------- ***

static int32_t
stepToUnicode(ConversionCase &cc, UConverter *cnv,
              UChar *result, int32_t resultCapacity,
              int32_t *resultOffsets, /* also resultCapacity */
              int32_t step,
              UErrorCode *pErrorCode) {
    const char *source, *sourceLimit, *bytesLimit;
    UChar *target, *targetLimit, *resultLimit;
    UBool flush;

    source=(const char *)cc.bytes;
    target=result;
    bytesLimit=source+cc.bytesLength;
    resultLimit=result+resultCapacity;

    if(step>=0) {
        // call ucnv_toUnicode() with in/out buffers no larger than (step) at a time
        // move only one buffer (in vs. out) at a time to be extra mean
        // step==0 performs bulk conversion and generates offsets

        // initialize the partial limits for the loop
        if(step==0) {
            // use the entire buffers
            sourceLimit=bytesLimit;
            targetLimit=resultLimit;
            flush=cc.finalFlush;
        } else {
            // start with empty partial buffers
            sourceLimit=source;
            targetLimit=target;
            flush=FALSE;

            // output offsets only for bulk conversion
            resultOffsets=NULL;
        }

        for(;;) {
            // resetting the opposite conversion direction must not affect this one
            ucnv_resetFromUnicode(cnv);

            // convert
            ucnv_toUnicode(cnv,
                &target, targetLimit,
                &source, sourceLimit,
                resultOffsets,
                flush, pErrorCode);

            // check pointers and errors
            if(source>sourceLimit || target>targetLimit) {
                *pErrorCode=U_INTERNAL_PROGRAM_ERROR;
                break;
            } else if(*pErrorCode==U_BUFFER_OVERFLOW_ERROR) {
                if(target!=targetLimit) {
                    // buffer overflow must only be set when the target is filled
                    *pErrorCode=U_INTERNAL_PROGRAM_ERROR;
                    break;
                } else if(targetLimit==resultLimit) {
                    // not just a partial overflow
                    break;
                }

                // the partial target is filled, set a new limit, reset the error and continue
                targetLimit=(resultLimit-target)>=step ? target+step : resultLimit;
                *pErrorCode=U_ZERO_ERROR;
            } else if(U_FAILURE(*pErrorCode)) {
                // some other error occurred, done
                break;
            } else {
                if(source!=sourceLimit) {
                    // when no error occurs, then the input must be consumed
                    *pErrorCode=U_INTERNAL_PROGRAM_ERROR;
                    break;
                }

                if(sourceLimit==bytesLimit) {
                    // we are done
                    break;
                }

                // the partial conversion succeeded, set a new limit and continue
                sourceLimit=(bytesLimit-source)>=step ? source+step : bytesLimit;
                flush=(UBool)(cc.finalFlush && sourceLimit==bytesLimit);
            }
        }
    } else /* step<0 */ {
        /*
         * step==-1: call only ucnv_getNextUChar()
         * otherwise alternate between ucnv_toUnicode() and ucnv_getNextUChar()
         *   if step==-2 or -3, then give ucnv_toUnicode() the whole remaining input,
         *   else give it at most (-step-2)/2 bytes
         */
        UChar32 c;

        // end the loop by getting an index out of bounds error
        for(;;) {
            // resetting the opposite conversion direction must not affect this one
            ucnv_resetFromUnicode(cnv);

            // convert
            if((step&1)!=0 /* odd: -1, -3, -5, ... */) {
                sourceLimit=source; // use sourceLimit not as a real limit
                                    // but to remember the pre-getNextUChar source pointer
                c=ucnv_getNextUChar(cnv, &source, bytesLimit, pErrorCode);

                // check pointers and errors
                if(*pErrorCode==U_INDEX_OUTOFBOUNDS_ERROR) {
                    if(source!=bytesLimit) {
                        *pErrorCode=U_INTERNAL_PROGRAM_ERROR;
                    } else {
                        *pErrorCode=U_ZERO_ERROR;
                    }
                    break;
                } else if(U_FAILURE(*pErrorCode)) {
                    break;
                }
                // source may not move if c is from previous overflow

                if(target==resultLimit) {
                    *pErrorCode=U_BUFFER_OVERFLOW_ERROR;
                    break;
                }
                if(c<=0xffff) {
                    *target++=(UChar)c;
                } else {
                    *target++=U16_LEAD(c);
                    if(target==resultLimit) {
                        *pErrorCode=U_BUFFER_OVERFLOW_ERROR;
                        break;
                    }
                    *target++=U16_TRAIL(c);
                }

                // alternate between -n-1 and -n but leave -1 alone
                if(step<-1) {
                    ++step;
                }
            } else /* step is even */ {
                // allow only one UChar output
                targetLimit=target<resultLimit ? target+1 : resultLimit;

                // as with ucnv_getNextUChar(), we always flush (if we go to bytesLimit)
                // and never output offsets
                if(step==-2) {
                    sourceLimit=bytesLimit;
                } else {
                    sourceLimit=source+(-step-2)/2;
                    if(sourceLimit>bytesLimit) {
                        sourceLimit=bytesLimit;
                    }
                }

                ucnv_toUnicode(cnv,
                    &target, targetLimit,
                    &source, sourceLimit,
                    NULL, (UBool)(sourceLimit==bytesLimit), pErrorCode);

                // check pointers and errors
                if(*pErrorCode==U_BUFFER_OVERFLOW_ERROR) {
                    if(target!=targetLimit) {
                        // buffer overflow must only be set when the target is filled
                        *pErrorCode=U_INTERNAL_PROGRAM_ERROR;
                        break;
                    } else if(targetLimit==resultLimit) {
                        // not just a partial overflow
                        break;
                    }

                    // the partial target is filled, set a new limit and continue
                    *pErrorCode=U_ZERO_ERROR;
                } else if(U_FAILURE(*pErrorCode)) {
                    // some other error occurred, done
                    break;
                } else {
                    if(source!=sourceLimit) {
                        // when no error occurs, then the input must be consumed
                        *pErrorCode=U_INTERNAL_PROGRAM_ERROR;
                        break;
                    }

                    // we are done (flush==TRUE) but we continue, to get the index out of bounds error above
                }

                --step;
            }
        }
    }

    return (int32_t)(target-result);
}

UBool
ConversionTest::ToUnicodeCase(ConversionCase &cc, UConverterToUCallback callback, const char *option) {
    // open the converter
    IcuTestErrorCode errorCode(*this, "ToUnicodeCase");
    LocalUConverterPointer cnv(cnv_open(cc.charset, errorCode));
    if(errorCode.isFailure()) {
        errcheckln(errorCode, "toUnicode[%d](%s cb=\"%s\" fb=%d flush=%d) ucnv_open() failed - %s",
                cc.caseNr, cc.charset, cc.cbopt, cc.fallbacks, cc.finalFlush, errorCode.errorName());
        errorCode.reset();
        return FALSE;
    }

    // set the callback
    if(callback!=NULL) {
        ucnv_setToUCallBack(cnv.getAlias(), callback, option, NULL, NULL, errorCode);
        if(U_FAILURE(errorCode)) {
            errln("toUnicode[%d](%s cb=\"%s\" fb=%d flush=%d) ucnv_setToUCallBack() failed - %s",
                    cc.caseNr, cc.charset, cc.cbopt, cc.fallbacks, cc.finalFlush, u_errorName(errorCode));
            return FALSE;
        }
    }

    int32_t resultOffsets[256];
    UChar result[256];
    int32_t resultLength;
    UBool ok;

    static const struct {
        int32_t step;
        const char *name;
    } steps[]={
        { 0, "bulk" }, // must be first for offsets to be checked
        { 1, "step=1" },
        { 3, "step=3" },
        { 7, "step=7" },
        { -1, "getNext" },
        { -2, "toU(bulk)+getNext" },
        { -3, "getNext+toU(bulk)" },
        { -4, "toU(1)+getNext" },
        { -5, "getNext+toU(1)" },
        { -12, "toU(5)+getNext" },
        { -13, "getNext+toU(5)" },
    };
    int32_t i, step;

    ok=TRUE;
    for(i=0; i<LENGTHOF(steps) && ok; ++i) {
        step=steps[i].step;
        if(step<0 && !cc.finalFlush) {
            // skip ucnv_getNextUChar() if !finalFlush because
            // ucnv_getNextUChar() always implies flush
            continue;
        }
        if(step!=0) {
            // bulk test is first, then offsets are not checked any more
            cc.offsets=NULL;
        }
        else {
            memset(resultOffsets, -1, LENGTHOF(resultOffsets));
        }
        memset(result, -1, LENGTHOF(result));
        errorCode.reset();
        resultLength=stepToUnicode(cc, cnv.getAlias(),
                                result, LENGTHOF(result),
                                step==0 ? resultOffsets : NULL,
                                step, errorCode);
        ok=checkToUnicode(
                cc, cnv.getAlias(), steps[i].name,
                result, resultLength,
                cc.offsets!=NULL ? resultOffsets : NULL,
                errorCode);
        if(errorCode.isFailure() || !cc.finalFlush) {
            // reset if an error occurred or we did not flush
            // otherwise do nothing to make sure that flushing resets
            ucnv_resetToUnicode(cnv.getAlias());
        }
        if (cc.offsets != NULL && resultOffsets[resultLength] != -1) {
            errln("toUnicode[%d](%s) Conversion wrote too much to offsets at index %d",
                cc.caseNr, cc.charset, resultLength);
        }
        if (result[resultLength] != (UChar)-1) {
            errln("toUnicode[%d](%s) Conversion wrote too much to result at index %d",
                cc.caseNr, cc.charset, resultLength);
        }
    }

    // not a real loop, just a convenience for breaking out of the block
    while(ok && cc.finalFlush) {
        // test ucnv_toUChars()
        memset(result, 0, sizeof(result));

        errorCode.reset();
        resultLength=ucnv_toUChars(cnv.getAlias(),
                        result, LENGTHOF(result),
                        (const char *)cc.bytes, cc.bytesLength,
                        errorCode);
        ok=checkToUnicode(
                cc, cnv.getAlias(), "toUChars",
                result, resultLength,
                NULL,
                errorCode);
        if(!ok) {
            break;
        }

        // test preflighting
        // keep the correct result for simple checking
        errorCode.reset();
        resultLength=ucnv_toUChars(cnv.getAlias(),
                        NULL, 0,
                        (const char *)cc.bytes, cc.bytesLength,
                        errorCode);
        if(errorCode.get()==U_STRING_NOT_TERMINATED_WARNING || errorCode.get()==U_BUFFER_OVERFLOW_ERROR) {
            errorCode.reset();
        }
        ok=checkToUnicode(
                cc, cnv.getAlias(), "preflight toUChars",
                result, resultLength,
                NULL,
                errorCode);
        break;
    }

    errorCode.reset();  // all errors have already been reported
    return ok;
}

UBool
ConversionTest::checkToUnicode(ConversionCase &cc, UConverter *cnv, const char *name,
                               const UChar *result, int32_t resultLength,
                               const int32_t *resultOffsets,
                               UErrorCode resultErrorCode) {
    char resultInvalidChars[8];
    int8_t resultInvalidLength;
    UErrorCode errorCode;

    const char *msg;

    // reset the message; NULL will mean "ok"
    msg=NULL;

    errorCode=U_ZERO_ERROR;
    resultInvalidLength=sizeof(resultInvalidChars);
    ucnv_getInvalidChars(cnv, resultInvalidChars, &resultInvalidLength, &errorCode);
    if(U_FAILURE(errorCode)) {
        errln("toUnicode[%d](%s cb=\"%s\" fb=%d flush=%d %s) ucnv_getInvalidChars() failed - %s",
                cc.caseNr, cc.charset, cc.cbopt, cc.fallbacks, cc.finalFlush, name, u_errorName(errorCode));
        return FALSE;
    }

    // check everything that might have gone wrong
    if(cc.unicodeLength!=resultLength) {
        msg="wrong result length";
    } else if(0!=u_memcmp(cc.unicode, result, cc.unicodeLength)) {
        msg="wrong result string";
    } else if(cc.offsets!=NULL && 0!=memcmp(cc.offsets, resultOffsets, cc.unicodeLength*sizeof(*cc.offsets))) {
        msg="wrong offsets";
    } else if(cc.outErrorCode!=resultErrorCode) {
        msg="wrong error code";
    } else if(cc.invalidLength!=resultInvalidLength) {
        msg="wrong length of last invalid input";
    } else if(0!=memcmp(cc.invalidChars, resultInvalidChars, cc.invalidLength)) {
        msg="wrong last invalid input";
    }

    if(msg==NULL) {
        return TRUE;
    } else {
        char buffer[2000]; // one buffer for all strings
        char *s, *bytesString, *unicodeString, *resultString,
            *offsetsString, *resultOffsetsString,
            *invalidCharsString, *resultInvalidCharsString;

        bytesString=s=buffer;
        s=printBytes(cc.bytes, cc.bytesLength, bytesString);
        s=printUnicode(cc.unicode, cc.unicodeLength, unicodeString=s);
        s=printUnicode(result, resultLength, resultString=s);
        s=printOffsets(cc.offsets, cc.unicodeLength, offsetsString=s);
        s=printOffsets(resultOffsets, resultLength, resultOffsetsString=s);
        s=printBytes(cc.invalidChars, cc.invalidLength, invalidCharsString=s);
        s=printBytes((uint8_t *)resultInvalidChars, resultInvalidLength, resultInvalidCharsString=s);

        if((s-buffer)>(int32_t)sizeof(buffer)) {
            errln("toUnicode[%d](%s cb=\"%s\" fb=%d flush=%d %s) fatal error: checkToUnicode() test output buffer overflow writing %d chars\n",
                    cc.caseNr, cc.charset, cc.cbopt, cc.fallbacks, cc.finalFlush, name, (int)(s-buffer));
            exit(1);
        }

        errln("toUnicode[%d](%s cb=\"%s\" fb=%d flush=%d %s) failed: %s\n"
              "  bytes <%s>[%d]\n"
              " expected <%s>[%d]\n"
              "  result  <%s>[%d]\n"
              " offsets         <%s>\n"
              "  result offsets <%s>\n"
              " error code expected %s got %s\n"
              "  invalidChars expected <%s> got <%s>\n",
              cc.caseNr, cc.charset, cc.cbopt, cc.fallbacks, cc.finalFlush, name, msg,
              bytesString, cc.bytesLength,
              unicodeString, cc.unicodeLength,
              resultString, resultLength,
              offsetsString,
              resultOffsetsString,
              u_errorName(cc.outErrorCode), u_errorName(resultErrorCode),
              invalidCharsString, resultInvalidCharsString);

        return FALSE;
    }
}

// fromUnicode test worker functions --------------------------------------- ***

static int32_t
stepFromUTF8(ConversionCase &cc,
             UConverter *utf8Cnv, UConverter *cnv,
             char *result, int32_t resultCapacity,
             int32_t step,
             UErrorCode *pErrorCode) {
    const char *source, *sourceLimit, *utf8Limit;
    UChar pivotBuffer[32];
    UChar *pivotSource, *pivotTarget, *pivotLimit;
    char *target, *targetLimit, *resultLimit;
    UBool flush;

    source=cc.utf8;
    pivotSource=pivotTarget=pivotBuffer;
    target=result;
    utf8Limit=source+cc.utf8Length;
    resultLimit=result+resultCapacity;

    // call ucnv_convertEx() with in/out buffers no larger than (step) at a time
    // move only one buffer (in vs. out) at a time to be extra mean
    // step==0 performs bulk conversion

    // initialize the partial limits for the loop
    if(step==0) {
        // use the entire buffers
        sourceLimit=utf8Limit;
        targetLimit=resultLimit;
        flush=cc.finalFlush;

        pivotLimit=pivotBuffer+LENGTHOF(pivotBuffer);
    } else {
        // start with empty partial buffers
        sourceLimit=source;
        targetLimit=target;
        flush=FALSE;

        // empty pivot is not allowed, make it of length step
        pivotLimit=pivotBuffer+step;
    }

    for(;;) {
        // resetting the opposite conversion direction must not affect this one
        ucnv_resetFromUnicode(utf8Cnv);
        ucnv_resetToUnicode(cnv);

        // convert
        ucnv_convertEx(cnv, utf8Cnv,
            &target, targetLimit,
            &source, sourceLimit,
            pivotBuffer, &pivotSource, &pivotTarget, pivotLimit,
            FALSE, flush, pErrorCode);

        // check pointers and errors
        if(source>sourceLimit || target>targetLimit) {
            *pErrorCode=U_INTERNAL_PROGRAM_ERROR;
            break;
        } else if(*pErrorCode==U_BUFFER_OVERFLOW_ERROR) {
            if(target!=targetLimit) {
                // buffer overflow must only be set when the target is filled
                *pErrorCode=U_INTERNAL_PROGRAM_ERROR;
                break;
            } else if(targetLimit==resultLimit) {
                // not just a partial overflow
                break;
            }

            // the partial target is filled, set a new limit, reset the error and continue
            targetLimit=(resultLimit-target)>=step ? target+step : resultLimit;
            *pErrorCode=U_ZERO_ERROR;
        } else if(U_FAILURE(*pErrorCode)) {
            if(pivotSource==pivotBuffer) {
                // toUnicode error, should not occur
                // toUnicode errors are tested in cintltst TestConvertExFromUTF8()
                break;
            } else {
                // fromUnicode error
                // some other error occurred, done
                break;
            }
        } else {
            if(source!=sourceLimit) {
                // when no error occurs, then the input must be consumed
                *pErrorCode=U_INTERNAL_PROGRAM_ERROR;
                break;
            }

            if(sourceLimit==utf8Limit) {
                // we are done
                if(*pErrorCode==U_STRING_NOT_TERMINATED_WARNING) {
                    // ucnv_convertEx() warns about not terminating the output
                    // but ucnv_fromUnicode() does not and so
                    // checkFromUnicode() does not expect it
                    *pErrorCode=U_ZERO_ERROR;
                }
                break;
            }

            // the partial conversion succeeded, set a new limit and continue
            sourceLimit=(utf8Limit-source)>=step ? source+step : utf8Limit;
            flush=(UBool)(cc.finalFlush && sourceLimit==utf8Limit);
        }
    }

    return (int32_t)(target-result);
}

static int32_t
stepFromUnicode(ConversionCase &cc, UConverter *cnv,
                char *result, int32_t resultCapacity,
                int32_t *resultOffsets, /* also resultCapacity */
                int32_t step,
                UErrorCode *pErrorCode) {
    const UChar *source, *sourceLimit, *unicodeLimit;
    char *target, *targetLimit, *resultLimit;
    UBool flush;

    source=cc.unicode;
    target=result;
    unicodeLimit=source+cc.unicodeLength;
    resultLimit=result+resultCapacity;

    // call ucnv_fromUnicode() with in/out buffers no larger than (step) at a time
    // move only one buffer (in vs. out) at a time to be extra mean
    // step==0 performs bulk conversion and generates offsets

    // initialize the partial limits for the loop
    if(step==0) {
        // use the entire buffers
        sourceLimit=unicodeLimit;
        targetLimit=resultLimit;
        flush=cc.finalFlush;
    } else {
        // start with empty partial buffers
        sourceLimit=source;
        targetLimit=target;
        flush=FALSE;

        // output offsets only for bulk conversion
        resultOffsets=NULL;
    }

    for(;;) {
        // resetting the opposite conversion direction must not affect this one
        ucnv_resetToUnicode(cnv);

        // convert
        ucnv_fromUnicode(cnv,
            &target, targetLimit,
            &source, sourceLimit,
            resultOffsets,
            flush, pErrorCode);

        // check pointers and errors
        if(source>sourceLimit || target>targetLimit) {
            *pErrorCode=U_INTERNAL_PROGRAM_ERROR;
            break;
        } else if(*pErrorCode==U_BUFFER_OVERFLOW_ERROR) {
            if(target!=targetLimit) {
                // buffer overflow must only be set when the target is filled
                *pErrorCode=U_INTERNAL_PROGRAM_ERROR;
                break;
            } else if(targetLimit==resultLimit) {
                // not just a partial overflow
                break;
            }

            // the partial target is filled, set a new limit, reset the error and continue
            targetLimit=(resultLimit-target)>=step ? target+step : resultLimit;
            *pErrorCode=U_ZERO_ERROR;
        } else if(U_FAILURE(*pErrorCode)) {
            // some other error occurred, done
            break;
        } else {
            if(source!=sourceLimit) {
                // when no error occurs, then the input must be consumed
                *pErrorCode=U_INTERNAL_PROGRAM_ERROR;
                break;
            }

            if(sourceLimit==unicodeLimit) {
                // we are done
                break;
            }

            // the partial conversion succeeded, set a new limit and continue
            sourceLimit=(unicodeLimit-source)>=step ? source+step : unicodeLimit;
            flush=(UBool)(cc.finalFlush && sourceLimit==unicodeLimit);
        }
    }

    return (int32_t)(target-result);
}

UBool
ConversionTest::FromUnicodeCase(ConversionCase &cc, UConverterFromUCallback callback, const char *option) {
    UConverter *cnv;
    UErrorCode errorCode;

    // open the converter
    errorCode=U_ZERO_ERROR;
    cnv=cnv_open(cc.charset, errorCode);
    if(U_FAILURE(errorCode)) {
        errcheckln(errorCode, "fromUnicode[%d](%s cb=\"%s\" fb=%d flush=%d) ucnv_open() failed - %s",
                cc.caseNr, cc.charset, cc.cbopt, cc.fallbacks, cc.finalFlush, u_errorName(errorCode));
        return FALSE;
    }
    ucnv_resetToUnicode(utf8Cnv);

    // set the callback
    if(callback!=NULL) {
        ucnv_setFromUCallBack(cnv, callback, option, NULL, NULL, &errorCode);
        if(U_FAILURE(errorCode)) {
            errln("fromUnicode[%d](%s cb=\"%s\" fb=%d flush=%d) ucnv_setFromUCallBack() failed - %s",
                    cc.caseNr, cc.charset, cc.cbopt, cc.fallbacks, cc.finalFlush, u_errorName(errorCode));
            ucnv_close(cnv);
            return FALSE;
        }
    }

    // set the fallbacks flag
    // TODO change with Jitterbug 2401, then add a similar call for toUnicode too
    ucnv_setFallback(cnv, cc.fallbacks);

    // set the subchar
    int32_t length;

    if(cc.setSub>0) {
        length=(int32_t)strlen(cc.subchar);
        ucnv_setSubstChars(cnv, cc.subchar, (int8_t)length, &errorCode);
        if(U_FAILURE(errorCode)) {
            errln("fromUnicode[%d](%s cb=\"%s\" fb=%d flush=%d) ucnv_setSubstChars() failed - %s",
                    cc.caseNr, cc.charset, cc.cbopt, cc.fallbacks, cc.finalFlush, u_errorName(errorCode));
            ucnv_close(cnv);
            return FALSE;
        }
    } else if(cc.setSub<0) {
        ucnv_setSubstString(cnv, cc.subString, -1, &errorCode);
        if(U_FAILURE(errorCode)) {
            errln("fromUnicode[%d](%s cb=\"%s\" fb=%d flush=%d) ucnv_setSubstString() failed - %s",
                    cc.caseNr, cc.charset, cc.cbopt, cc.fallbacks, cc.finalFlush, u_errorName(errorCode));
            ucnv_close(cnv);
            return FALSE;
        }
    }

    // convert unicode to utf8
    char utf8[256];
    cc.utf8=utf8;
    u_strToUTF8(utf8, LENGTHOF(utf8), &cc.utf8Length,
                cc.unicode, cc.unicodeLength,
                &errorCode);
    if(U_FAILURE(errorCode)) {
        // skip UTF-8 testing of a string with an unpaired surrogate,
        // or of one that's too long
        // toUnicode errors are tested in cintltst TestConvertExFromUTF8()
        cc.utf8Length=-1;
    }

    int32_t resultOffsets[256];
    char result[256];
    int32_t resultLength;
    UBool ok;

    static const struct {
        int32_t step;
        const char *name, *utf8Name;
    } steps[]={
        { 0, "bulk",   "utf8" }, // must be first for offsets to be checked
        { 1, "step=1", "utf8 step=1" },
        { 3, "step=3", "utf8 step=3" },
        { 7, "step=7", "utf8 step=7" }
    };
    int32_t i, step;

    ok=TRUE;
    for(i=0; i<LENGTHOF(steps) && ok; ++i) {
        step=steps[i].step;
        memset(resultOffsets, -1, LENGTHOF(resultOffsets));
        memset(result, -1, LENGTHOF(result));
        errorCode=U_ZERO_ERROR;
        resultLength=stepFromUnicode(cc, cnv,
                                result, LENGTHOF(result),
                                step==0 ? resultOffsets : NULL,
                                step, &errorCode);
        ok=checkFromUnicode(
                cc, cnv, steps[i].name,
                (uint8_t *)result, resultLength,
                cc.offsets!=NULL ? resultOffsets : NULL,
                errorCode);
        if(U_FAILURE(errorCode) || !cc.finalFlush) {
            // reset if an error occurred or we did not flush
            // otherwise do nothing to make sure that flushing resets
            ucnv_resetFromUnicode(cnv);
        }
        if (resultOffsets[resultLength] != -1) {
            errln("fromUnicode[%d](%s) Conversion wrote too much to offsets at index %d",
                cc.caseNr, cc.charset, resultLength);
        }
        if (result[resultLength] != (char)-1) {
            errln("fromUnicode[%d](%s) Conversion wrote too much to result at index %d",
                cc.caseNr, cc.charset, resultLength);
        }

        // bulk test is first, then offsets are not checked any more
        cc.offsets=NULL;

        // test direct conversion from UTF-8
        if(cc.utf8Length>=0) {
            errorCode=U_ZERO_ERROR;
            resultLength=stepFromUTF8(cc, utf8Cnv, cnv,
                                    result, LENGTHOF(result),
                                    step, &errorCode);
            ok=checkFromUnicode(
                    cc, cnv, steps[i].utf8Name,
                    (uint8_t *)result, resultLength,
                    NULL,
                    errorCode);
            if(U_FAILURE(errorCode) || !cc.finalFlush) {
                // reset if an error occurred or we did not flush
                // otherwise do nothing to make sure that flushing resets
                ucnv_resetToUnicode(utf8Cnv);
                ucnv_resetFromUnicode(cnv);
            }
        }
    }

    // not a real loop, just a convenience for breaking out of the block
    while(ok && cc.finalFlush) {
        // test ucnv_fromUChars()
        memset(result, 0, sizeof(result));

        errorCode=U_ZERO_ERROR;
        resultLength=ucnv_fromUChars(cnv,
                        result, LENGTHOF(result),
                        cc.unicode, cc.unicodeLength,
                        &errorCode);
        ok=checkFromUnicode(
                cc, cnv, "fromUChars",
                (uint8_t *)result, resultLength,
                NULL,
                errorCode);
        if(!ok) {
            break;
        }

        // test preflighting
        // keep the correct result for simple checking
        errorCode=U_ZERO_ERROR;
        resultLength=ucnv_fromUChars(cnv,
                        NULL, 0,
                        cc.unicode, cc.unicodeLength,
                        &errorCode);
        if(errorCode==U_STRING_NOT_TERMINATED_WARNING || errorCode==U_BUFFER_OVERFLOW_ERROR) {
            errorCode=U_ZERO_ERROR;
        }
        ok=checkFromUnicode(
                cc, cnv, "preflight fromUChars",
                (uint8_t *)result, resultLength,
                NULL,
                errorCode);
        break;
    }

    ucnv_close(cnv);
    return ok;
}

UBool
ConversionTest::checkFromUnicode(ConversionCase &cc, UConverter *cnv, const char *name,
                                 const uint8_t *result, int32_t resultLength,
                                 const int32_t *resultOffsets,
                                 UErrorCode resultErrorCode) {
    UChar resultInvalidUChars[8];
    int8_t resultInvalidLength;
    UErrorCode errorCode;

    const char *msg;

    // reset the message; NULL will mean "ok"
    msg=NULL;

    errorCode=U_ZERO_ERROR;
    resultInvalidLength=LENGTHOF(resultInvalidUChars);
    ucnv_getInvalidUChars(cnv, resultInvalidUChars, &resultInvalidLength, &errorCode);
    if(U_FAILURE(errorCode)) {
        errln("fromUnicode[%d](%s cb=\"%s\" fb=%d flush=%d %s) ucnv_getInvalidUChars() failed - %s",
                cc.caseNr, cc.charset, cc.cbopt, cc.fallbacks, cc.finalFlush, name, u_errorName(errorCode));
        return FALSE;
    }

    // check everything that might have gone wrong
    if(cc.bytesLength!=resultLength) {
        msg="wrong result length";
    } else if(0!=memcmp(cc.bytes, result, cc.bytesLength)) {
        msg="wrong result string";
    } else if(cc.offsets!=NULL && 0!=memcmp(cc.offsets, resultOffsets, cc.bytesLength*sizeof(*cc.offsets))) {
        msg="wrong offsets";
    } else if(cc.outErrorCode!=resultErrorCode) {
        msg="wrong error code";
    } else if(cc.invalidLength!=resultInvalidLength) {
        msg="wrong length of last invalid input";
    } else if(0!=u_memcmp(cc.invalidUChars, resultInvalidUChars, cc.invalidLength)) {
        msg="wrong last invalid input";
    }

    if(msg==NULL) {
        return TRUE;
    } else {
        char buffer[2000]; // one buffer for all strings
        char *s, *unicodeString, *bytesString, *resultString,
            *offsetsString, *resultOffsetsString,
            *invalidCharsString, *resultInvalidUCharsString;

        unicodeString=s=buffer;
        s=printUnicode(cc.unicode, cc.unicodeLength, unicodeString);
        s=printBytes(cc.bytes, cc.bytesLength, bytesString=s);
        s=printBytes(result, resultLength, resultString=s);
        s=printOffsets(cc.offsets, cc.bytesLength, offsetsString=s);
        s=printOffsets(resultOffsets, resultLength, resultOffsetsString=s);
        s=printUnicode(cc.invalidUChars, cc.invalidLength, invalidCharsString=s);
        s=printUnicode(resultInvalidUChars, resultInvalidLength, resultInvalidUCharsString=s);

        if((s-buffer)>(int32_t)sizeof(buffer)) {
            errln("fromUnicode[%d](%s cb=\"%s\" fb=%d flush=%d %s) fatal error: checkFromUnicode() test output buffer overflow writing %d chars\n",
                    cc.caseNr, cc.charset, cc.cbopt, cc.fallbacks, cc.finalFlush, name, (int)(s-buffer));
            exit(1);
        }

        errln("fromUnicode[%d](%s cb=\"%s\" fb=%d flush=%d %s) failed: %s\n"
              "  unicode <%s>[%d]\n"
              " expected <%s>[%d]\n"
              "  result  <%s>[%d]\n"
              " offsets         <%s>\n"
              "  result offsets <%s>\n"
              " error code expected %s got %s\n"
              "  invalidChars expected <%s> got <%s>\n",
              cc.caseNr, cc.charset, cc.cbopt, cc.fallbacks, cc.finalFlush, name, msg,
              unicodeString, cc.unicodeLength,
              bytesString, cc.bytesLength,
              resultString, resultLength,
              offsetsString,
              resultOffsetsString,
              u_errorName(cc.outErrorCode), u_errorName(resultErrorCode),
              invalidCharsString, resultInvalidUCharsString);

        return FALSE;
    }
}

#endif /* #if !UCONFIG_NO_LEGACY_CONVERSION */
