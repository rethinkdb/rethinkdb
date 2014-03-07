/*
*******************************************************************************
*   Copyright (C) 2010, International Business Machines
*   Corporation and others.  All Rights Reserved.
*******************************************************************************
*   file name:  uts46test.cpp
*   encoding:   US-ASCII
*   tab size:   8 (not used)
*   indentation:4
*
*   created on: 2010may05
*   created by: Markus W. Scherer
*/

#include "unicode/utypes.h"

#if !UCONFIG_NO_IDNA

#include <string.h>
#include "unicode/bytestream.h"
#include "unicode/idna.h"
#include "unicode/localpointer.h"
#include "unicode/std_string.h"
#include "unicode/stringpiece.h"
#include "unicode/uidna.h"
#include "unicode/unistr.h"
#include "intltest.h"

#define LENGTHOF(array) (int32_t)(sizeof(array)/sizeof((array)[0]))

class UTS46Test : public IntlTest {
public:
    UTS46Test() : trans(NULL), nontrans(NULL) {}
    virtual ~UTS46Test();

    void runIndexedTest(int32_t index, UBool exec, const char *&name, char *par=NULL);
    void TestAPI();
    void TestNotSTD3();
    void TestSomeCases();
private:
    IDNA *trans, *nontrans;
};

extern IntlTest *createUTS46Test() {
    return new UTS46Test();
}

UTS46Test::~UTS46Test() {
    delete trans;
    delete nontrans;
}

void UTS46Test::runIndexedTest(int32_t index, UBool exec, const char *&name, char * /*par*/) {
    if(exec) {
        logln("TestSuite UTS46Test: ");
        if(trans==NULL) {
            IcuTestErrorCode errorCode(*this, "init/createUTS46Instance()");
            trans=IDNA::createUTS46Instance(
                UIDNA_USE_STD3_RULES|UIDNA_CHECK_BIDI|UIDNA_CHECK_CONTEXTJ,
                errorCode);
            nontrans=IDNA::createUTS46Instance(
                UIDNA_USE_STD3_RULES|UIDNA_CHECK_BIDI|UIDNA_CHECK_CONTEXTJ|
                UIDNA_NONTRANSITIONAL_TO_ASCII|UIDNA_NONTRANSITIONAL_TO_UNICODE,
                errorCode);
            if(errorCode.logDataIfFailureAndReset("createUTS46Instance()")) {
                name="";
                return;
            }
        }
    }
    TESTCASE_AUTO_BEGIN;
    TESTCASE_AUTO(TestAPI);
    TESTCASE_AUTO(TestNotSTD3);
    TESTCASE_AUTO(TestSomeCases);
    TESTCASE_AUTO_END;
}

const uint32_t severeErrors=
    UIDNA_ERROR_LEADING_COMBINING_MARK|
    UIDNA_ERROR_DISALLOWED|
    UIDNA_ERROR_PUNYCODE|
    UIDNA_ERROR_LABEL_HAS_DOT|
    UIDNA_ERROR_INVALID_ACE_LABEL;

static UBool isASCII(const UnicodeString &str) {
    const UChar *s=str.getBuffer();
    int32_t length=str.length();
    for(int32_t i=0; i<length; ++i) {
        if(s[i]>=0x80) {
            return FALSE;
        }
    }
    return TRUE;
}

class TestCheckedArrayByteSink : public CheckedArrayByteSink {
public:
    TestCheckedArrayByteSink(char* outbuf, int32_t capacity)
            : CheckedArrayByteSink(outbuf, capacity), calledFlush(FALSE) {}
    virtual CheckedArrayByteSink& Reset() {
        CheckedArrayByteSink::Reset();
        calledFlush = FALSE;
        return *this;
    }
    virtual void Flush() { calledFlush = TRUE; }
    UBool calledFlush;
};

void UTS46Test::TestAPI() {
    UErrorCode errorCode=U_ZERO_ERROR;
    UnicodeString result;
    IDNAInfo info;
    UnicodeString input=UNICODE_STRING_SIMPLE("www.eXample.cOm");
    UnicodeString expected=UNICODE_STRING_SIMPLE("www.example.com");
    trans->nameToASCII(input, result, info, errorCode);
    if(U_FAILURE(errorCode) || info.hasErrors() || result!=expected) {
        errln("T.nameToASCII(www.example.com) info.errors=%04lx result matches=%d %s",
              (long)info.getErrors(), result==expected, u_errorName(errorCode));
    }
    errorCode=U_USELESS_COLLATOR_ERROR;
    trans->nameToUnicode(input, result, info, errorCode);
    if(errorCode!=U_USELESS_COLLATOR_ERROR || !result.isBogus()) {
        errln("T.nameToUnicode(U_FAILURE) did not preserve the errorCode "
              "or not result.setToBogus() - %s",
              u_errorName(errorCode));
    }
    errorCode=U_ZERO_ERROR;
    input.setToBogus();
    result=UNICODE_STRING_SIMPLE("quatsch");
    nontrans->labelToASCII(input, result, info, errorCode);
    if(errorCode!=U_ILLEGAL_ARGUMENT_ERROR || !result.isBogus()) {
        errln("N.labelToASCII(bogus) did not set illegal-argument-error "
              "or not result.setToBogus() - %s",
              u_errorName(errorCode));
    }
    errorCode=U_ZERO_ERROR;
    input=UNICODE_STRING_SIMPLE("xn--bcher.de-65a");
    expected=UNICODE_STRING_SIMPLE("xn--bcher\\uFFFDde-65a").unescape();
    nontrans->labelToASCII(input, result, info, errorCode);
    if( U_FAILURE(errorCode) ||
        info.getErrors()!=(UIDNA_ERROR_LABEL_HAS_DOT|UIDNA_ERROR_INVALID_ACE_LABEL) ||
        result!=expected
    ) {
        errln("N.labelToASCII(label-with-dot) failed with errors %04lx - %s",
              info.getErrors(), u_errorName(errorCode));
    }
    // UTF-8
    char buffer[100];
    TestCheckedArrayByteSink sink(buffer, LENGTHOF(buffer));
    errorCode=U_ZERO_ERROR;
    nontrans->labelToUnicodeUTF8(StringPiece(NULL, 5), sink, info, errorCode);
    if(errorCode!=U_ILLEGAL_ARGUMENT_ERROR || sink.NumberOfBytesWritten()!=0) {
        errln("N.labelToUnicodeUTF8(StringPiece(NULL, 5)) did not set illegal-argument-error ",
              "or did output something - %s",
              u_errorName(errorCode));
    }

    sink.Reset();
    errorCode=U_ZERO_ERROR;
    nontrans->nameToASCII_UTF8(StringPiece(), sink, info, errorCode);
    if(U_FAILURE(errorCode) || sink.NumberOfBytesWritten()!=0 || !sink.calledFlush) {
        errln("N.nameToASCII_UTF8(empty) failed - %s",
              u_errorName(errorCode));
    }

    static const char s[]={ 0x61, (char)0xc3, (char)0x9f };
    sink.Reset();
    errorCode=U_USELESS_COLLATOR_ERROR;
    nontrans->nameToUnicodeUTF8(StringPiece(s, 3), sink, info, errorCode);
    if(errorCode!=U_USELESS_COLLATOR_ERROR || sink.NumberOfBytesWritten()!=0) {
        errln("N.nameToUnicode_UTF8(U_FAILURE) did not preserve the errorCode "
              "or did output something - %s",
              u_errorName(errorCode));
    }

    sink.Reset();
    errorCode=U_ZERO_ERROR;
    trans->labelToUnicodeUTF8(StringPiece(s, 3), sink, info, errorCode);
    if( U_FAILURE(errorCode) || sink.NumberOfBytesWritten()!=3 ||
        buffer[0]!=0x61 || buffer[1]!=0x73 || buffer[2]!=0x73 ||
        !sink.calledFlush
    ) {
        errln("T.labelToUnicodeUTF8(a sharp-s) failed - %s",
              u_errorName(errorCode));
    }

    sink.Reset();
    errorCode=U_ZERO_ERROR;
    // "eXampLe.cOm"
    static const char eX[]={ 0x65, 0x58, 0x61, 0x6d, 0x70, 0x4c, 0x65, 0x2e, 0x63, 0x4f, 0x6d, 0 };
    // "example.com"
    static const char ex[]={ 0x65, 0x78, 0x61, 0x6d, 0x70, 0x6c, 0x65, 0x2e, 0x63, 0x6f, 0x6d };
    trans->nameToUnicodeUTF8(eX, sink, info, errorCode);
    if( U_FAILURE(errorCode) || sink.NumberOfBytesWritten()!=11 ||
        0!=memcmp(ex, buffer, 11) || !sink.calledFlush
    ) {
        errln("T.nameToUnicodeUTF8(eXampLe.cOm) failed - %s",
              u_errorName(errorCode));
    }
}

void UTS46Test::TestNotSTD3() {
    IcuTestErrorCode errorCode(*this, "TestNotSTD3()");
    char buffer[400];
    LocalPointer<IDNA> not3(IDNA::createUTS46Instance(UIDNA_CHECK_BIDI, errorCode));
    if(errorCode.isFailure()) {
        return;
    }
    UnicodeString input=UNICODE_STRING_SIMPLE("\\u0000A_2+2=4\\u000A.e\\u00DFen.net").unescape();
    UnicodeString result;
    IDNAInfo info;
    if( not3->nameToUnicode(input, result, info, errorCode)!=
            UNICODE_STRING_SIMPLE("\\u0000a_2+2=4\\u000A.essen.net").unescape() ||
        info.hasErrors()
    ) {
        prettify(result).extract(0, 0x7fffffff, buffer, LENGTHOF(buffer));
        errln("notSTD3.nameToUnicode(non-LDH ASCII) unexpected errors %04lx string %s",
              (long)info.getErrors(), buffer);
    }
    // A space (BiDi class WS) is not allowed in a BiDi domain name.
    input=UNICODE_STRING_SIMPLE("a z.xn--4db.edu");
    not3->nameToASCII(input, result, info, errorCode);
    if(result!=input || info.getErrors()!=UIDNA_ERROR_BIDI) {
        errln("notSTD3.nameToASCII(ASCII-with-space.alef.edu) failed");
    }
    // Characters that are canonically equivalent to sequences with non-LDH ASCII.
    input=UNICODE_STRING_SIMPLE("a\\u2260b\\u226Ec\\u226Fd").unescape();
    not3->nameToUnicode(input, result, info, errorCode);
    if(result!=input || info.hasErrors()) {
        prettify(result).extract(0, 0x7fffffff, buffer, LENGTHOF(buffer));
        errln("notSTD3.nameToUnicode(equiv to non-LDH ASCII) unexpected errors %04lx string %s",
              (long)info.getErrors(), buffer);
    }
}

struct TestCase {
    // Input string and options string (Nontransitional/Transitional/Both).
    const char *s, *o;
    // Expected Unicode result string.
    const char *u;
    uint32_t errors;
};

static const TestCase testCases[]={
    { "www.eXample.cOm", "B",  // all ASCII
      "www.example.com", 0 },
    { "B\\u00FCcher.de", "B",  // u-umlaut
      "b\\u00FCcher.de", 0 },
    { "\\u00D6BB", "B",  // O-umlaut
      "\\u00F6bb", 0 },
    { "fa\\u00DF.de", "N",  // sharp s
      "fa\\u00DF.de", 0 },
    { "fa\\u00DF.de", "T",  // sharp s
      "fass.de", 0 },
    { "XN--fA-hia.dE", "B",  // sharp s in Punycode
      "fa\\u00DF.de", 0 },
    { "\\u03B2\\u03CC\\u03BB\\u03BF\\u03C2.com", "N",  // Greek with final sigma
      "\\u03B2\\u03CC\\u03BB\\u03BF\\u03C2.com", 0 },
    { "\\u03B2\\u03CC\\u03BB\\u03BF\\u03C2.com", "T",  // Greek with final sigma
      "\\u03B2\\u03CC\\u03BB\\u03BF\\u03C3.com", 0 },
    { "xn--nxasmm1c", "B",  // Greek with final sigma in Punycode
      "\\u03B2\\u03CC\\u03BB\\u03BF\\u03C2", 0 },
    { "www.\\u0DC1\\u0DCA\\u200D\\u0DBB\\u0DD3.com", "N",  // "Sri" in "Sri Lanka" has a ZWJ
      "www.\\u0DC1\\u0DCA\\u200D\\u0DBB\\u0DD3.com", 0 },
    { "www.\\u0DC1\\u0DCA\\u200D\\u0DBB\\u0DD3.com", "T",  // "Sri" in "Sri Lanka" has a ZWJ
      "www.\\u0DC1\\u0DCA\\u0DBB\\u0DD3.com", 0 },
    { "www.xn--10cl1a0b660p.com", "B",  // "Sri" in Punycode
      "www.\\u0DC1\\u0DCA\\u200D\\u0DBB\\u0DD3.com", 0 },
    { "\\u0646\\u0627\\u0645\\u0647\\u200C\\u0627\\u06CC", "N",  // ZWNJ
      "\\u0646\\u0627\\u0645\\u0647\\u200C\\u0627\\u06CC", 0 },
    { "\\u0646\\u0627\\u0645\\u0647\\u200C\\u0627\\u06CC", "T",  // ZWNJ
      "\\u0646\\u0627\\u0645\\u0647\\u0627\\u06CC", 0 },
    { "xn--mgba3gch31f060k.com", "B",  // ZWNJ in Punycode
      "\\u0646\\u0627\\u0645\\u0647\\u200C\\u0627\\u06CC.com", 0 },
    { "a.b\\uFF0Ec\\u3002d\\uFF61", "B",
      "a.b.c.d.", 0 },
    { "U\\u0308.xn--tda", "B",  // U+umlaut.u-umlaut
      "\\u00FC.\\u00FC", 0 },
    { "xn--u-ccb", "B",  // u+umlaut in Punycode
      "xn--u-ccb\\uFFFD", UIDNA_ERROR_INVALID_ACE_LABEL },
    { "a\\u2488com", "B",  // contains 1-dot
      "a\\uFFFDcom", UIDNA_ERROR_DISALLOWED },
    { "xn--a-ecp.ru", "B",  // contains 1-dot in Punycode
      "xn--a-ecp\\uFFFD.ru", UIDNA_ERROR_INVALID_ACE_LABEL },
    { "xn--0.pt", "B",  // invalid Punycode
      "xn--0\\uFFFD.pt", UIDNA_ERROR_PUNYCODE },
    { "xn--a.pt", "B",  // U+0080
      "xn--a\\uFFFD.pt", UIDNA_ERROR_INVALID_ACE_LABEL },
    { "xn--a-\\u00C4.pt", "B",  // invalid Punycode
      "xn--a-\\u00E4.pt", UIDNA_ERROR_PUNYCODE },
    { "\\u65E5\\u672C\\u8A9E\\u3002\\uFF2A\\uFF30", "B",  // Japanese with fullwidth ".jp"
      "\\u65E5\\u672C\\u8A9E.jp", 0 },
    { "\\u2615", "B", "\\u2615", 0 },  // Unicode 4.0 HOT BEVERAGE
    // some characters are disallowed because they are canonically equivalent
    // to sequences with non-LDH ASCII
    { "a\\u2260b\\u226Ec\\u226Fd", "B",
      "a\\uFFFDb\\uFFFDc\\uFFFDd", UIDNA_ERROR_DISALLOWED },
    // many deviation characters, test the special mapping code
    { "1.a\\u00DF\\u200C\\u200Db\\u200C\\u200Dc\\u00DF\\u00DF\\u00DF\\u00DFd"
      "\\u03C2\\u03C3\\u00DF\\u00DF\\u00DF\\u00DF\\u00DF\\u00DF\\u00DF\\u00DFe"
      "\\u00DF\\u00DF\\u00DF\\u00DF\\u00DF\\u00DF\\u00DF\\u00DF\\u00DF\\u00DFx"
      "\\u00DF\\u00DF\\u00DF\\u00DF\\u00DF\\u00DF\\u00DF\\u00DF\\u00DF\\u00DFy"
      "\\u00DF\\u00DF\\u00DF\\u00DF\\u00DF\\u00DF\\u00DF\\u00DF\\u0302\\u00DFz", "N",
      "1.a\\u00DF\\u200C\\u200Db\\u200C\\u200Dc\\u00DF\\u00DF\\u00DF\\u00DFd"
      "\\u03C2\\u03C3\\u00DF\\u00DF\\u00DF\\u00DF\\u00DF\\u00DF\\u00DF\\u00DFe"
      "\\u00DF\\u00DF\\u00DF\\u00DF\\u00DF\\u00DF\\u00DF\\u00DF\\u00DF\\u00DFx"
      "\\u00DF\\u00DF\\u00DF\\u00DF\\u00DF\\u00DF\\u00DF\\u00DF\\u00DF\\u00DFy"
      "\\u00DF\\u00DF\\u00DF\\u00DF\\u00DF\\u00DF\\u00DF\\u00DF\\u0302\\u00DFz",
      UIDNA_ERROR_LABEL_TOO_LONG|UIDNA_ERROR_CONTEXTJ },
    { "1.a\\u00DF\\u200C\\u200Db\\u200C\\u200Dc\\u00DF\\u00DF\\u00DF\\u00DFd"
      "\\u03C2\\u03C3\\u00DF\\u00DF\\u00DF\\u00DF\\u00DF\\u00DF\\u00DF\\u00DFe"
      "\\u00DF\\u00DF\\u00DF\\u00DF\\u00DF\\u00DF\\u00DF\\u00DF\\u00DF\\u00DFx"
      "\\u00DF\\u00DF\\u00DF\\u00DF\\u00DF\\u00DF\\u00DF\\u00DF\\u00DF\\u00DFy"
      "\\u00DF\\u00DF\\u00DF\\u00DF\\u00DF\\u00DF\\u00DF\\u00DF\\u0302\\u00DFz", "T",
      "1.assbcssssssssd"
      "\\u03C3\\u03C3sssssssssssssssse"
      "ssssssssssssssssssssx"
      "ssssssssssssssssssssy"
      "sssssssssssssss\\u015Dssz", UIDNA_ERROR_LABEL_TOO_LONG },
    // "xn--bss" with deviation characters
    { "\\u200Cx\\u200Dn\\u200C-\\u200D-b\\u00DF", "N",
      "\\u200Cx\\u200Dn\\u200C-\\u200D-b\\u00DF", UIDNA_ERROR_CONTEXTJ },
    { "\\u200Cx\\u200Dn\\u200C-\\u200D-b\\u00DF", "T",
      "\\u5919", 0 },
    // "xn--bssffl" written as:
    // 02E3 MODIFIER LETTER SMALL X
    // 034F COMBINING GRAPHEME JOINER (ignored)
    // 2115 DOUBLE-STRUCK CAPITAL N
    // 200B ZERO WIDTH SPACE (ignored)
    // FE63 SMALL HYPHEN-MINUS
    // 00AD SOFT HYPHEN (ignored)
    // FF0D FULLWIDTH HYPHEN-MINUS
    // 180C MONGOLIAN FREE VARIATION SELECTOR TWO (ignored)
    // 212C SCRIPT CAPITAL B
    // FE00 VARIATION SELECTOR-1 (ignored)
    // 017F LATIN SMALL LETTER LONG S
    // 2064 INVISIBLE PLUS (ignored)
    // 1D530 MATHEMATICAL FRAKTUR SMALL S
    // E01EF VARIATION SELECTOR-256 (ignored)
    // FB04 LATIN SMALL LIGATURE FFL
    { "\\u02E3\\u034F\\u2115\\u200B\\uFE63\\u00AD\\uFF0D\\u180C"
      "\\u212C\\uFE00\\u017F\\u2064\\U0001D530\\U000E01EF\\uFB04", "B",
      "\\u5921\\u591E\\u591C\\u5919", 0 },
    { "123456789012345678901234567890123456789012345678901234567890123."
      "123456789012345678901234567890123456789012345678901234567890123."
      "123456789012345678901234567890123456789012345678901234567890123."
      "1234567890123456789012345678901234567890123456789012345678901", "B",
      "123456789012345678901234567890123456789012345678901234567890123."
      "123456789012345678901234567890123456789012345678901234567890123."
      "123456789012345678901234567890123456789012345678901234567890123."
      "1234567890123456789012345678901234567890123456789012345678901", 0 },
    { "123456789012345678901234567890123456789012345678901234567890123."
      "123456789012345678901234567890123456789012345678901234567890123."
      "123456789012345678901234567890123456789012345678901234567890123."
      "1234567890123456789012345678901234567890123456789012345678901.", "B",
      "123456789012345678901234567890123456789012345678901234567890123."
      "123456789012345678901234567890123456789012345678901234567890123."
      "123456789012345678901234567890123456789012345678901234567890123."
      "1234567890123456789012345678901234567890123456789012345678901.", 0 },
    // Domain name >256 characters, forces slow path in UTF-8 processing.
    { "123456789012345678901234567890123456789012345678901234567890123."
      "123456789012345678901234567890123456789012345678901234567890123."
      "123456789012345678901234567890123456789012345678901234567890123."
      "123456789012345678901234567890123456789012345678901234567890123."
      "12345678901234567890123456789012345678901234567890123456789012", "B",
      "123456789012345678901234567890123456789012345678901234567890123."
      "123456789012345678901234567890123456789012345678901234567890123."
      "123456789012345678901234567890123456789012345678901234567890123."
      "123456789012345678901234567890123456789012345678901234567890123."
      "12345678901234567890123456789012345678901234567890123456789012",
      UIDNA_ERROR_DOMAIN_NAME_TOO_LONG },
    { "123456789012345678901234567890123456789012345678901234567890123."
      "123456789012345678901234567890123456789012345678901234567890123."
      "123456789012345678901234567890123456789012345678901234567890123."
      "123456789012345678901234567890123456789012345678901234567890123."
      "1234567890123456789012345678901234567890123456789\\u05D0", "B",
      "123456789012345678901234567890123456789012345678901234567890123."
      "123456789012345678901234567890123456789012345678901234567890123."
      "123456789012345678901234567890123456789012345678901234567890123."
      "123456789012345678901234567890123456789012345678901234567890123."
      "1234567890123456789012345678901234567890123456789\\u05D0",
      UIDNA_ERROR_DOMAIN_NAME_TOO_LONG|UIDNA_ERROR_BIDI },
    { "123456789012345678901234567890123456789012345678901234567890123."
      "1234567890123456789012345678901234567890123456789012345678901234."
      "123456789012345678901234567890123456789012345678901234567890123."
      "123456789012345678901234567890123456789012345678901234567890", "B",
      "123456789012345678901234567890123456789012345678901234567890123."
      "1234567890123456789012345678901234567890123456789012345678901234."
      "123456789012345678901234567890123456789012345678901234567890123."
      "123456789012345678901234567890123456789012345678901234567890",
      UIDNA_ERROR_LABEL_TOO_LONG },
    { "123456789012345678901234567890123456789012345678901234567890123."
      "1234567890123456789012345678901234567890123456789012345678901234."
      "123456789012345678901234567890123456789012345678901234567890123."
      "123456789012345678901234567890123456789012345678901234567890.", "B",
      "123456789012345678901234567890123456789012345678901234567890123."
      "1234567890123456789012345678901234567890123456789012345678901234."
      "123456789012345678901234567890123456789012345678901234567890123."
      "123456789012345678901234567890123456789012345678901234567890.",
      UIDNA_ERROR_LABEL_TOO_LONG },
    { "123456789012345678901234567890123456789012345678901234567890123."
      "1234567890123456789012345678901234567890123456789012345678901234."
      "123456789012345678901234567890123456789012345678901234567890123."
      "1234567890123456789012345678901234567890123456789012345678901", "B",
      "123456789012345678901234567890123456789012345678901234567890123."
      "1234567890123456789012345678901234567890123456789012345678901234."
      "123456789012345678901234567890123456789012345678901234567890123."
      "1234567890123456789012345678901234567890123456789012345678901",
      UIDNA_ERROR_LABEL_TOO_LONG|UIDNA_ERROR_DOMAIN_NAME_TOO_LONG },
    // label length 63: xn--1234567890123456789012345678901234567890123456789012345-9te
    { "\\u00E41234567890123456789012345678901234567890123456789012345", "B",
      "\\u00E41234567890123456789012345678901234567890123456789012345", 0 },
    { "1234567890\\u00E41234567890123456789012345678901234567890123456", "B",
      "1234567890\\u00E41234567890123456789012345678901234567890123456", UIDNA_ERROR_LABEL_TOO_LONG },
    { "123456789012345678901234567890123456789012345678901234567890123."
      "1234567890\\u00E4123456789012345678901234567890123456789012345."
      "123456789012345678901234567890123456789012345678901234567890123."
      "1234567890123456789012345678901234567890123456789012345678901", "B",
      "123456789012345678901234567890123456789012345678901234567890123."
      "1234567890\\u00E4123456789012345678901234567890123456789012345."
      "123456789012345678901234567890123456789012345678901234567890123."
      "1234567890123456789012345678901234567890123456789012345678901", 0 },
    { "123456789012345678901234567890123456789012345678901234567890123."
      "1234567890\\u00E4123456789012345678901234567890123456789012345."
      "123456789012345678901234567890123456789012345678901234567890123."
      "1234567890123456789012345678901234567890123456789012345678901.", "B",
      "123456789012345678901234567890123456789012345678901234567890123."
      "1234567890\\u00E4123456789012345678901234567890123456789012345."
      "123456789012345678901234567890123456789012345678901234567890123."
      "1234567890123456789012345678901234567890123456789012345678901.", 0 },
    { "123456789012345678901234567890123456789012345678901234567890123."
      "1234567890\\u00E4123456789012345678901234567890123456789012345."
      "123456789012345678901234567890123456789012345678901234567890123."
      "12345678901234567890123456789012345678901234567890123456789012", "B",
      "123456789012345678901234567890123456789012345678901234567890123."
      "1234567890\\u00E4123456789012345678901234567890123456789012345."
      "123456789012345678901234567890123456789012345678901234567890123."
      "12345678901234567890123456789012345678901234567890123456789012",
      UIDNA_ERROR_DOMAIN_NAME_TOO_LONG },
    { "123456789012345678901234567890123456789012345678901234567890123."
      "1234567890\\u00E41234567890123456789012345678901234567890123456."
      "123456789012345678901234567890123456789012345678901234567890123."
      "123456789012345678901234567890123456789012345678901234567890", "B",
      "123456789012345678901234567890123456789012345678901234567890123."
      "1234567890\\u00E41234567890123456789012345678901234567890123456."
      "123456789012345678901234567890123456789012345678901234567890123."
      "123456789012345678901234567890123456789012345678901234567890",
      UIDNA_ERROR_LABEL_TOO_LONG },
    { "123456789012345678901234567890123456789012345678901234567890123."
      "1234567890\\u00E41234567890123456789012345678901234567890123456."
      "123456789012345678901234567890123456789012345678901234567890123."
      "123456789012345678901234567890123456789012345678901234567890.", "B",
      "123456789012345678901234567890123456789012345678901234567890123."
      "1234567890\\u00E41234567890123456789012345678901234567890123456."
      "123456789012345678901234567890123456789012345678901234567890123."
      "123456789012345678901234567890123456789012345678901234567890.",
      UIDNA_ERROR_LABEL_TOO_LONG },
    { "123456789012345678901234567890123456789012345678901234567890123."
      "1234567890\\u00E41234567890123456789012345678901234567890123456."
      "123456789012345678901234567890123456789012345678901234567890123."
      "1234567890123456789012345678901234567890123456789012345678901", "B",
      "123456789012345678901234567890123456789012345678901234567890123."
      "1234567890\\u00E41234567890123456789012345678901234567890123456."
      "123456789012345678901234567890123456789012345678901234567890123."
      "1234567890123456789012345678901234567890123456789012345678901",
      UIDNA_ERROR_LABEL_TOO_LONG|UIDNA_ERROR_DOMAIN_NAME_TOO_LONG },
    // hyphen errors and empty-label errors
    // "xn---q----jra"=="-q--a-umlaut-"
    { "a.b..-q--a-.e", "B", "a.b..-q--a-.e",
      UIDNA_ERROR_EMPTY_LABEL|UIDNA_ERROR_LEADING_HYPHEN|UIDNA_ERROR_TRAILING_HYPHEN|
      UIDNA_ERROR_HYPHEN_3_4 },
    { "a.b..-q--\\u00E4-.e", "B", "a.b..-q--\\u00E4-.e",
      UIDNA_ERROR_EMPTY_LABEL|UIDNA_ERROR_LEADING_HYPHEN|UIDNA_ERROR_TRAILING_HYPHEN|
      UIDNA_ERROR_HYPHEN_3_4 },
    { "a.b..xn---q----jra.e", "B", "a.b..-q--\\u00E4-.e",
      UIDNA_ERROR_EMPTY_LABEL|UIDNA_ERROR_LEADING_HYPHEN|UIDNA_ERROR_TRAILING_HYPHEN|
      UIDNA_ERROR_HYPHEN_3_4 },
    { "a..c", "B", "a..c", UIDNA_ERROR_EMPTY_LABEL },
    { "a.-b.", "B", "a.-b.", UIDNA_ERROR_LEADING_HYPHEN },
    { "a.b-.c", "B", "a.b-.c", UIDNA_ERROR_TRAILING_HYPHEN },
    { "a.-.c", "B", "a.-.c", UIDNA_ERROR_LEADING_HYPHEN|UIDNA_ERROR_TRAILING_HYPHEN },
    { "a.bc--de.f", "B", "a.bc--de.f", UIDNA_ERROR_HYPHEN_3_4 },
    { "\\u00E4.\\u00AD.c", "B", "\\u00E4..c", UIDNA_ERROR_EMPTY_LABEL },
    { "\\u00E4.-b.", "B", "\\u00E4.-b.", UIDNA_ERROR_LEADING_HYPHEN },
    { "\\u00E4.b-.c", "B", "\\u00E4.b-.c", UIDNA_ERROR_TRAILING_HYPHEN },
    { "\\u00E4.-.c", "B", "\\u00E4.-.c", UIDNA_ERROR_LEADING_HYPHEN|UIDNA_ERROR_TRAILING_HYPHEN },
    { "\\u00E4.bc--de.f", "B", "\\u00E4.bc--de.f", UIDNA_ERROR_HYPHEN_3_4 },
    { "a.b.\\u0308c.d", "B", "a.b.\\uFFFDc.d", UIDNA_ERROR_LEADING_COMBINING_MARK },
    { "a.b.xn--c-bcb.d", "B",
      "a.b.xn--c-bcb\\uFFFD.d", UIDNA_ERROR_LEADING_COMBINING_MARK|UIDNA_ERROR_INVALID_ACE_LABEL },
    // BiDi
    { "A0", "B", "a0", 0 },
    { "0A", "B", "0a", 0 },  // all-LTR is ok to start with a digit (EN)
    { "0A.\\u05D0", "B",  // ASCII label does not start with L/R/AL
      "0a.\\u05D0", UIDNA_ERROR_BIDI },
    { "c.xn--0-eha.xn--4db", "B",  // 2nd label does not start with L/R/AL
      "c.0\\u00FC.\\u05D0", UIDNA_ERROR_BIDI },
    { "b-.\\u05D0", "B",  // label does not end with L/EN
      "b-.\\u05D0", UIDNA_ERROR_TRAILING_HYPHEN|UIDNA_ERROR_BIDI },
    { "d.xn----dha.xn--4db", "B",  // 2nd label does not end with L/EN
      "d.\\u00FC-.\\u05D0", UIDNA_ERROR_TRAILING_HYPHEN|UIDNA_ERROR_BIDI },
    { "a\\u05D0", "B", "a\\u05D0", UIDNA_ERROR_BIDI },  // first dir != last dir
    { "\\u05D0\\u05C7", "B", "\\u05D0\\u05C7", 0 },
    { "\\u05D09\\u05C7", "B", "\\u05D09\\u05C7", 0 },
    { "\\u05D0a\\u05C7", "B", "\\u05D0a\\u05C7", UIDNA_ERROR_BIDI },  // first dir != last dir
    { "\\u05D0\\u05EA", "B", "\\u05D0\\u05EA", 0 },
    { "\\u05D0\\u05F3\\u05EA", "B", "\\u05D0\\u05F3\\u05EA", 0 },
    { "a\\u05D0Tz", "B", "a\\u05D0tz", UIDNA_ERROR_BIDI },  // mixed dir
    { "\\u05D0T\\u05EA", "B", "\\u05D0t\\u05EA", UIDNA_ERROR_BIDI },  // mixed dir
    { "\\u05D07\\u05EA", "B", "\\u05D07\\u05EA", 0 },
    { "\\u05D0\\u0667\\u05EA", "B", "\\u05D0\\u0667\\u05EA", 0 },  // Arabic 7 in the middle
    { "a7\\u0667z", "B", "a7\\u0667z", UIDNA_ERROR_BIDI },  // AN digit in LTR
    { "\\u05D07\\u0667\\u05EA", "B",  // mixed EN/AN digits in RTL
      "\\u05D07\\u0667\\u05EA", UIDNA_ERROR_BIDI },
    // ZWJ
    { "\\u0BB9\\u0BCD\\u200D", "N", "\\u0BB9\\u0BCD\\u200D", 0 },  // Virama+ZWJ
    { "\\u0BB9\\u200D", "N", "\\u0BB9\\u200D", UIDNA_ERROR_CONTEXTJ },  // no Virama
    { "\\u200D", "N", "\\u200D", UIDNA_ERROR_CONTEXTJ },  // no Virama
    // ZWNJ
    { "\\u0BB9\\u0BCD\\u200C", "N", "\\u0BB9\\u0BCD\\u200C", 0 },  // Virama+ZWNJ
    { "\\u0BB9\\u200C", "N", "\\u0BB9\\u200C", UIDNA_ERROR_CONTEXTJ },  // no Virama
    { "\\u200C", "N", "\\u200C", UIDNA_ERROR_CONTEXTJ },  // no Virama
    { "\\u0644\\u0670\\u200C\\u06ED\\u06EF", "N",  // Joining types D T ZWNJ T R
      "\\u0644\\u0670\\u200C\\u06ED\\u06EF", 0 },
    { "\\u0644\\u0670\\u200C\\u06EF", "N",  // D T ZWNJ R
      "\\u0644\\u0670\\u200C\\u06EF", 0 },
    { "\\u0644\\u200C\\u06ED\\u06EF", "N",  // D ZWNJ T R
      "\\u0644\\u200C\\u06ED\\u06EF", 0 },
    { "\\u0644\\u200C\\u06EF", "N",  // D ZWNJ R
      "\\u0644\\u200C\\u06EF", 0 },
    { "\\u0644\\u0670\\u200C\\u06ED", "N",  // D T ZWNJ T
      "\\u0644\\u0670\\u200C\\u06ED", UIDNA_ERROR_BIDI|UIDNA_ERROR_CONTEXTJ },
    { "\\u06EF\\u200C\\u06EF", "N",  // R ZWNJ R
      "\\u06EF\\u200C\\u06EF", UIDNA_ERROR_CONTEXTJ },
    { "\\u0644\\u200C", "N",  // D ZWNJ
      "\\u0644\\u200C", UIDNA_ERROR_BIDI|UIDNA_ERROR_CONTEXTJ },
    // Ticket #8137: UTS #46 toUnicode() fails with non-ASCII labels that turn
    // into 15 characters (UChars).
    // The bug was in u_strFromPunycode() which did not write the last character
    // if it just so fit into the end of the destination buffer.
    // The UTS #46 code gives a default-capacity UnicodeString as the destination buffer,
    // and the internal UnicodeString capacity is currently 15 UChars on 64-bit machines
    // but 13 on 32-bit machines.
    // Label with 15 UChars, for 64-bit-machine testing:
    { "aaaaaaaaaaaaa\\u00FCa.de", "B", "aaaaaaaaaaaaa\\u00FCa.de", 0 },
    { "xn--aaaaaaaaaaaaaa-ssb.de", "B", "aaaaaaaaaaaaa\\u00FCa.de", 0 },
    { "abschlu\\u00DFpr\\u00FCfung.de", "N", "abschlu\\u00DFpr\\u00FCfung.de", 0 },
    { "xn--abschluprfung-hdb15b.de", "B", "abschlu\\u00DFpr\\u00FCfung.de", 0 },
    // Label with 13 UChars, for 32-bit-machine testing:
    { "xn--aaaaaaaaaaaa-nlb.de", "B", "aaaaaaaaaaa\\u00FCa.de", 0 },
    { "xn--schluprfung-z6a39a.de", "B", "schlu\\u00DFpr\\u00FCfung.de", 0 },
    // { "", "B",
    //   "", 0 },
};

void UTS46Test::TestSomeCases() {
    IcuTestErrorCode errorCode(*this, "TestSomeCases");
    char buffer[400], buffer2[400];
    int32_t i;
    for(i=0; i<LENGTHOF(testCases); ++i) {
        const TestCase &testCase=testCases[i];
        UnicodeString input(ctou(testCase.s));
        UnicodeString expected(ctou(testCase.u));
        // ToASCII/ToUnicode, transitional/nontransitional
        UnicodeString aT, uT, aN, uN;
        IDNAInfo aTInfo, uTInfo, aNInfo, uNInfo;
        trans->nameToASCII(input, aT, aTInfo, errorCode);
        trans->nameToUnicode(input, uT, uTInfo, errorCode);
        nontrans->nameToASCII(input, aN, aNInfo, errorCode);
        nontrans->nameToUnicode(input, uN, uNInfo, errorCode);
        if(errorCode.logIfFailureAndReset("first-level processing [%d/%s] %s",
                                          (int)i, testCase.o, testCase.s)
        ) {
            continue;
        }
        // ToUnicode does not set length errors.
        uint32_t uniErrors=testCase.errors&~
            (UIDNA_ERROR_EMPTY_LABEL|
             UIDNA_ERROR_LABEL_TOO_LONG|
             UIDNA_ERROR_DOMAIN_NAME_TOO_LONG);
        char mode=testCase.o[0];
        if(mode=='B' || mode=='N') {
            if(uNInfo.getErrors()!=uniErrors) {
                errln("N.nameToUnicode([%d] %s) unexpected errors %04lx",
                      (int)i, testCase.s, (long)uNInfo.getErrors());
                continue;
            }
            if(uN!=expected) {
                prettify(uN).extract(0, 0x7fffffff, buffer, LENGTHOF(buffer));
                errln("N.nameToUnicode([%d] %s) unexpected string %s",
                      (int)i, testCase.s, buffer);
                continue;
            }
            if(aNInfo.getErrors()!=testCase.errors) {
                errln("N.nameToASCII([%d] %s) unexpected errors %04lx",
                      (int)i, testCase.s, (long)aNInfo.getErrors());
                continue;
            }
        }
        if(mode=='B' || mode=='T') {
            if(uTInfo.getErrors()!=uniErrors) {
                errln("T.nameToUnicode([%d] %s) unexpected errors %04lx",
                      (int)i, testCase.s, (long)uTInfo.getErrors());
                continue;
            }
            if(uT!=expected) {
                prettify(uT).extract(0, 0x7fffffff, buffer, LENGTHOF(buffer));
                errln("T.nameToUnicode([%d] %s) unexpected string %s",
                      (int)i, testCase.s, buffer);
                continue;
            }
            if(aTInfo.getErrors()!=testCase.errors) {
                errln("T.nameToASCII([%d] %s) unexpected errors %04lx",
                      (int)i, testCase.s, (long)aTInfo.getErrors());
                continue;
            }
        }
        // ToASCII is all-ASCII if no severe errors
        if((aNInfo.getErrors()&severeErrors)==0 && !isASCII(aN)) {
            prettify(aN).extract(0, 0x7fffffff, buffer, LENGTHOF(buffer));
            errln("N.nameToASCII([%d] %s) (errors %04lx) result is not ASCII %s",
                  (int)i, testCase.s, aNInfo.getErrors(), buffer);
            continue;
        }
        if((aTInfo.getErrors()&severeErrors)==0 && !isASCII(aT)) {
            prettify(aT).extract(0, 0x7fffffff, buffer, LENGTHOF(buffer));
            errln("T.nameToASCII([%d] %s) (errors %04lx) result is not ASCII %s",
                  (int)i, testCase.s, aTInfo.getErrors(), buffer);
            continue;
        }
        if(verbose) {
            char m= mode=='B' ? mode : 'N';
            prettify(aN).extract(0, 0x7fffffff, buffer, LENGTHOF(buffer));
            logln("%c.nameToASCII([%d] %s) (errors %04lx) result string: %s",
                  m, (int)i, testCase.s, aNInfo.getErrors(), buffer);
            if(mode!='B') {
                prettify(aT).extract(0, 0x7fffffff, buffer, LENGTHOF(buffer));
                logln("T.nameToASCII([%d] %s) (errors %04lx) result string: %s",
                      (int)i, testCase.s, aTInfo.getErrors(), buffer);
            }
        }
        // second-level processing
        UnicodeString aTuN, uTaN, aNuN, uNaN;
        IDNAInfo aTuNInfo, uTaNInfo, aNuNInfo, uNaNInfo;
        nontrans->nameToUnicode(aT, aTuN, aTuNInfo, errorCode);
        nontrans->nameToASCII(uT, uTaN, uTaNInfo, errorCode);
        nontrans->nameToUnicode(aN, aNuN, aNuNInfo, errorCode);
        nontrans->nameToASCII(uN, uNaN, uNaNInfo, errorCode);
        if(errorCode.logIfFailureAndReset("second-level processing [%d/%s] %s",
                                          (int)i, testCase.o, testCase.s)
        ) {
            continue;
        }
        if(aN!=uNaN) {
            prettify(aN).extract(0, 0x7fffffff, buffer, LENGTHOF(buffer));
            prettify(uNaN).extract(0, 0x7fffffff, buffer2, LENGTHOF(buffer2));
            errln("N.nameToASCII([%d] %s)!=N.nameToUnicode().N.nameToASCII() "
                  "(errors %04lx) %s vs. %s",
                  (int)i, testCase.s, aNInfo.getErrors(), buffer, buffer2);
            continue;
        }
        if(aT!=uTaN) {
            prettify(aT).extract(0, 0x7fffffff, buffer, LENGTHOF(buffer));
            prettify(uTaN).extract(0, 0x7fffffff, buffer2, LENGTHOF(buffer2));
            errln("T.nameToASCII([%d] %s)!=T.nameToUnicode().N.nameToASCII() "
                  "(errors %04lx) %s vs. %s",
                  (int)i, testCase.s, aNInfo.getErrors(), buffer, buffer2);
            continue;
        }
        if(uN!=aNuN) {
            prettify(uN).extract(0, 0x7fffffff, buffer, LENGTHOF(buffer));
            prettify(aNuN).extract(0, 0x7fffffff, buffer2, LENGTHOF(buffer2));
            errln("N.nameToUnicode([%d] %s)!=N.nameToASCII().N.nameToUnicode() "
                  "(errors %04lx) %s vs. %s",
                  (int)i, testCase.s, uNInfo.getErrors(), buffer, buffer2);
            continue;
        }
        if(uT!=aTuN) {
            prettify(uT).extract(0, 0x7fffffff, buffer, LENGTHOF(buffer));
            prettify(aTuN).extract(0, 0x7fffffff, buffer2, LENGTHOF(buffer2));
            errln("T.nameToUnicode([%d] %s)!=T.nameToASCII().N.nameToUnicode() "
                  "(errors %04lx) %s vs. %s",
                  (int)i, testCase.s, uNInfo.getErrors(), buffer, buffer2);
            continue;
        }
        // labelToUnicode
        UnicodeString aTL, uTL, aNL, uNL;
        IDNAInfo aTLInfo, uTLInfo, aNLInfo, uNLInfo;
        trans->labelToASCII(input, aTL, aTLInfo, errorCode);
        trans->labelToUnicode(input, uTL, uTLInfo, errorCode);
        nontrans->labelToASCII(input, aNL, aNLInfo, errorCode);
        nontrans->labelToUnicode(input, uNL, uNLInfo, errorCode);
        if(errorCode.logIfFailureAndReset("labelToXYZ processing [%d/%s] %s",
                                          (int)i, testCase.o, testCase.s)
        ) {
            continue;
        }
        if(aN.indexOf((UChar)0x2e)<0) {
            if(aN!=aNL || aNInfo.getErrors()!=aNLInfo.getErrors()) {
                prettify(aN).extract(0, 0x7fffffff, buffer, LENGTHOF(buffer));
                prettify(aNL).extract(0, 0x7fffffff, buffer2, LENGTHOF(buffer2));
                errln("N.nameToASCII([%d] %s)!=N.labelToASCII() "
                      "(errors %04lx vs %04lx) %s vs. %s",
                      (int)i, testCase.s, aNInfo.getErrors(), aNLInfo.getErrors(), buffer, buffer2);
                continue;
            }
        } else {
            if((aNLInfo.getErrors()&UIDNA_ERROR_LABEL_HAS_DOT)==0) {
                errln("N.labelToASCII([%d] %s) errors %04lx missing UIDNA_ERROR_LABEL_HAS_DOT",
                      (int)i, testCase.s, (long)aNLInfo.getErrors());
                continue;
            }
        }
        if(aT.indexOf((UChar)0x2e)<0) {
            if(aT!=aTL || aTInfo.getErrors()!=aTLInfo.getErrors()) {
                prettify(aT).extract(0, 0x7fffffff, buffer, LENGTHOF(buffer));
                prettify(aTL).extract(0, 0x7fffffff, buffer2, LENGTHOF(buffer2));
                errln("T.nameToASCII([%d] %s)!=T.labelToASCII() "
                      "(errors %04lx vs %04lx) %s vs. %s",
                      (int)i, testCase.s, aTInfo.getErrors(), aTLInfo.getErrors(), buffer, buffer2);
                continue;
            }
        } else {
            if((aTLInfo.getErrors()&UIDNA_ERROR_LABEL_HAS_DOT)==0) {
                errln("T.labelToASCII([%d] %s) errors %04lx missing UIDNA_ERROR_LABEL_HAS_DOT",
                      (int)i, testCase.s, (long)aTLInfo.getErrors());
                continue;
            }
        }
        if(uN.indexOf((UChar)0x2e)<0) {
            if(uN!=uNL || uNInfo.getErrors()!=uNLInfo.getErrors()) {
                prettify(uN).extract(0, 0x7fffffff, buffer, LENGTHOF(buffer));
                prettify(uNL).extract(0, 0x7fffffff, buffer2, LENGTHOF(buffer2));
                errln("N.nameToUnicode([%d] %s)!=N.labelToUnicode() "
                      "(errors %04lx vs %04lx) %s vs. %s",
                      (int)i, testCase.s, uNInfo.getErrors(), uNLInfo.getErrors(), buffer, buffer2);
                continue;
            }
        } else {
            if((uNLInfo.getErrors()&UIDNA_ERROR_LABEL_HAS_DOT)==0) {
                errln("N.labelToUnicode([%d] %s) errors %04lx missing UIDNA_ERROR_LABEL_HAS_DOT",
                      (int)i, testCase.s, (long)uNLInfo.getErrors());
                continue;
            }
        }
        if(uT.indexOf((UChar)0x2e)<0) {
            if(uT!=uTL || uTInfo.getErrors()!=uTLInfo.getErrors()) {
                prettify(uT).extract(0, 0x7fffffff, buffer, LENGTHOF(buffer));
                prettify(uTL).extract(0, 0x7fffffff, buffer2, LENGTHOF(buffer2));
                errln("T.nameToUnicode([%d] %s)!=T.labelToUnicode() "
                      "(errors %04lx vs %04lx) %s vs. %s",
                      (int)i, testCase.s, uTInfo.getErrors(), uTLInfo.getErrors(), buffer, buffer2);
                continue;
            }
        } else {
            if((uTLInfo.getErrors()&UIDNA_ERROR_LABEL_HAS_DOT)==0) {
                errln("T.labelToUnicode([%d] %s) errors %04lx missing UIDNA_ERROR_LABEL_HAS_DOT",
                      (int)i, testCase.s, (long)uTLInfo.getErrors());
                continue;
            }
        }
        // Differences between transitional and nontransitional processing
        if(mode=='B') {
            if( aNInfo.isTransitionalDifferent() ||
                aTInfo.isTransitionalDifferent() ||
                uNInfo.isTransitionalDifferent() ||
                uTInfo.isTransitionalDifferent() ||
                aNLInfo.isTransitionalDifferent() ||
                aTLInfo.isTransitionalDifferent() ||
                uNLInfo.isTransitionalDifferent() ||
                uTLInfo.isTransitionalDifferent()
            ) {
                errln("B.process([%d] %s) isTransitionalDifferent()", (int)i, testCase.s);
                continue;
            }
            if( aN!=aT || uN!=uT || aNL!=aTL || uNL!=uTL ||
                aNInfo.getErrors()!=aTInfo.getErrors() || uNInfo.getErrors()!=uTInfo.getErrors() ||
                aNLInfo.getErrors()!=aTLInfo.getErrors() || uNLInfo.getErrors()!=uTLInfo.getErrors()
            ) {
                errln("N.process([%d] %s) vs. T.process() different errors or result strings",
                      (int)i, testCase.s);
                continue;
            }
        } else {
            if( !aNInfo.isTransitionalDifferent() ||
                !aTInfo.isTransitionalDifferent() ||
                !uNInfo.isTransitionalDifferent() ||
                !uTInfo.isTransitionalDifferent() ||
                !aNLInfo.isTransitionalDifferent() ||
                !aTLInfo.isTransitionalDifferent() ||
                !uNLInfo.isTransitionalDifferent() ||
                !uTLInfo.isTransitionalDifferent()
            ) {
                errln("%s.process([%d] %s) !isTransitionalDifferent()",
                      testCase.o, (int)i, testCase.s);
                continue;
            }
            if(aN==aT || uN==uT || aNL==aTL || uNL==uTL) {
                errln("N.process([%d] %s) vs. T.process() same result strings",
                      (int)i, testCase.s);
                continue;
            }
        }
        // UTF-8 if we have std::string
#if U_HAVE_STD_STRING
        U_STD_NSQ string input8, aT8, uT8, aN8, uN8;
        StringByteSink<U_STD_NSQ string> aT8Sink(&aT8), uT8Sink(&uT8), aN8Sink(&aN8), uN8Sink(&uN8);
        IDNAInfo aT8Info, uT8Info, aN8Info, uN8Info;
        input.toUTF8String(input8);
        trans->nameToASCII_UTF8(input8, aT8Sink, aT8Info, errorCode);
        trans->nameToUnicodeUTF8(input8, uT8Sink, uT8Info, errorCode);
        nontrans->nameToASCII_UTF8(input8, aN8Sink, aN8Info, errorCode);
        nontrans->nameToUnicodeUTF8(input8, uN8Sink, uN8Info, errorCode);
        if(errorCode.logIfFailureAndReset("UTF-8 processing [%d/%s] %s",
                                          (int)i, testCase.o, testCase.s)
        ) {
            continue;
        }
        UnicodeString aT16(UnicodeString::fromUTF8(aT8));
        UnicodeString uT16(UnicodeString::fromUTF8(uT8));
        UnicodeString aN16(UnicodeString::fromUTF8(aN8));
        UnicodeString uN16(UnicodeString::fromUTF8(uN8));
        if( aN8Info.getErrors()!=aNInfo.getErrors() ||
            uN8Info.getErrors()!=uNInfo.getErrors()
        ) {
            errln("N.xyzUTF8([%d] %s) vs. UTF-16 processing different errors %04lx vs. %04lx",
                  (int)i, testCase.s,
                  (long)aN8Info.getErrors(), (long)aNInfo.getErrors());
            continue;
        }
        if( aT8Info.getErrors()!=aTInfo.getErrors() ||
            uT8Info.getErrors()!=uTInfo.getErrors()
        ) {
            errln("T.xyzUTF8([%d] %s) vs. UTF-16 processing different errors %04lx vs. %04lx",
                  (int)i, testCase.s,
                  (long)aT8Info.getErrors(), (long)aTInfo.getErrors());
            continue;
        }
        if(aT16!=aT || uT16!=uT || aN16!=aN || uN16!=uN) {
            errln("%s.xyzUTF8([%d] %s) vs. UTF-16 processing different string results",
                  testCase.o, (int)i, testCase.s, (long)aTInfo.getErrors());
            continue;
        }
        if( aT8Info.isTransitionalDifferent()!=aTInfo.isTransitionalDifferent() ||
            uT8Info.isTransitionalDifferent()!=uTInfo.isTransitionalDifferent() ||
            aN8Info.isTransitionalDifferent()!=aNInfo.isTransitionalDifferent() ||
            uN8Info.isTransitionalDifferent()!=uNInfo.isTransitionalDifferent()
        ) {
            errln("%s.xyzUTF8([%d] %s) vs. UTF-16 processing different isTransitionalDifferent()",
                  testCase.o, (int)i, testCase.s);
            continue;
        }
#endif
    }
}

#endif  // UCONFIG_NO_IDNA
