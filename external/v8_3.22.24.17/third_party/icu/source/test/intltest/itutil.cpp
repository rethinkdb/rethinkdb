/********************************************************************
 * COPYRIGHT: 
 * Copyright (c) 1997-2010, International Business Machines Corporation and
 * others. All Rights Reserved.
 ********************************************************************/


/**
 * IntlTestUtilities is the medium level test class for everything in the directory "utility".
 */

#include "unicode/utypes.h"
#include "unicode/errorcode.h"
#include "unicode/localpointer.h"
#include "itutil.h"
#include "strtest.h"
#include "loctest.h"
#include "citrtest.h"
#include "ustrtest.h"
#include "ucdtest.h"
#include "restest.h"
#include "restsnew.h"
#include "tsmthred.h"
#include "tsputil.h"
#include "uobjtest.h"
#include "utxttest.h"
#include "v32test.h"
#include "uvectest.h" 
#include "aliastst.h"
#include "usettest.h"

static IntlTest *createLocalPointerTest();

#define CASE(id, test) case id:                               \
                          name = #test;                       \
                          if (exec) {                         \
                              logln(#test "---"); logln();    \
                              test t;                         \
                              callTest(t, par);               \
                          }                                   \
                          break

void IntlTestUtilities::runIndexedTest( int32_t index, UBool exec, const char* &name, char* par )
{
    if (exec) logln("TestSuite Utilities: ");
    switch (index) {
        CASE(0, MultithreadTest);
        CASE(1, StringTest);
        CASE(2, UnicodeStringTest);
        CASE(3, LocaleTest);
        CASE(4, CharIterTest);
        CASE(5, UObjectTest);
        CASE(6, UnicodeTest);
        CASE(7, ResourceBundleTest);
        CASE(8, NewResourceBundleTest);
        CASE(9, PUtilTest);
        CASE(10, UVector32Test);
        CASE(11, UVectorTest);
        CASE(12, UTextTest);
        CASE(13, LocaleAliasTest);
        CASE(14, UnicodeSetTest);
        CASE(15, ErrorCodeTest);
        case 16:
            name = "LocalPointerTest";
            if (exec) {
                logln("TestSuite LocalPointerTest---"); logln();
                LocalPointer<IntlTest> test(createLocalPointerTest());
                callTest(*test, par);
            }
            break;
        default: name = ""; break; //needed to end loop
    }
}

void ErrorCodeTest::runIndexedTest(int32_t index, UBool exec, const char* &name, char* /*par*/) {
    if (exec) logln("TestSuite Utilities: ");
    switch (index) {
        case 0: name = "TestErrorCode"; if (exec) TestErrorCode(); break;
        case 1: name = "TestSubclass"; if (exec) TestSubclass(); break;
        default: name = ""; break; //needed to end loop
    }
}

static void RefPlusOne(UErrorCode &code) { code=(UErrorCode)(code+1); }
static void PtrPlusTwo(UErrorCode *code) { *code=(UErrorCode)(*code+2); }

void ErrorCodeTest::TestErrorCode() {
    ErrorCode errorCode;
    if(errorCode.get()!=U_ZERO_ERROR || !errorCode.isSuccess() || errorCode.isFailure()) {
        errln("ErrorCode did not initialize properly");
        return;
    }
    errorCode.assertSuccess();
    if(errorCode.errorName()!=u_errorName(U_ZERO_ERROR)) {
        errln("ErrorCode did not format error message string properly");
    }
    RefPlusOne(errorCode);
    if(errorCode.get()!=U_ILLEGAL_ARGUMENT_ERROR || errorCode.isSuccess() || !errorCode.isFailure()) {
        errln("ErrorCode did not yield a writable reference");
    }
    PtrPlusTwo(errorCode);
    if(errorCode.get()!=U_INVALID_FORMAT_ERROR || errorCode.isSuccess() || !errorCode.isFailure()) {
        errln("ErrorCode did not yield a writable pointer");
    }
    errorCode.set(U_PARSE_ERROR);
    if(errorCode.get()!=U_PARSE_ERROR || errorCode.isSuccess() || !errorCode.isFailure()) {
        errln("ErrorCode.set() failed");
    }
    if( errorCode.reset()!=U_PARSE_ERROR || errorCode.get()!=U_ZERO_ERROR ||
        !errorCode.isSuccess() || errorCode.isFailure()
    ) {
        errln("ErrorCode did not reset properly");
    }
}

class MyErrorCode: public ErrorCode {
public:
    MyErrorCode(int32_t &countChecks, int32_t &countDests)
        : checks(countChecks), dests(countDests) {}
    ~MyErrorCode() {
        if(isFailure()) {
            ++dests;
        }
    }
private:
    virtual void handleFailure() const {
        ++checks;
    }
    int32_t &checks;
    int32_t &dests;
};

void ErrorCodeTest::TestSubclass() {
    int32_t countChecks=0;
    int32_t countDests=0;
    {
        MyErrorCode errorCode(countChecks, countDests);
        if( errorCode.get()!=U_ZERO_ERROR || !errorCode.isSuccess() || errorCode.isFailure() ||
            countChecks!=0 || countDests!=0
        ) {
            errln("ErrorCode did not initialize properly");
            return;
        }
        errorCode.assertSuccess();
        if(countChecks!=0) {
            errln("ErrorCode.assertSuccess() called handleFailure() despite success");
        }
        RefPlusOne(errorCode);
        if(errorCode.get()!=U_ILLEGAL_ARGUMENT_ERROR || errorCode.isSuccess() || !errorCode.isFailure()) {
            errln("ErrorCode did not yield a writable reference");
        }
        errorCode.assertSuccess();
        if(countChecks!=1) {
            errln("ErrorCode.assertSuccess() did not handleFailure()");
        }
        PtrPlusTwo(errorCode);
        if(errorCode.get()!=U_INVALID_FORMAT_ERROR || errorCode.isSuccess() || !errorCode.isFailure()) {
            errln("ErrorCode did not yield a writable pointer");
        }
        errorCode.assertSuccess();
        if(countChecks!=2) {
            errln("ErrorCode.assertSuccess() did not handleFailure()");
        }
        errorCode.set(U_PARSE_ERROR);
        if(errorCode.get()!=U_PARSE_ERROR || errorCode.isSuccess() || !errorCode.isFailure()) {
            errln("ErrorCode.set() failed");
        }
        if( errorCode.reset()!=U_PARSE_ERROR || errorCode.get()!=U_ZERO_ERROR ||
            !errorCode.isSuccess() || errorCode.isFailure()
        ) {
            errln("ErrorCode did not reset properly");
        }
        errorCode.assertSuccess();
        if(countChecks!=2) {
            errln("ErrorCode.assertSuccess() called handleFailure() despite success");
        }
    }
    if(countDests!=0) {
        errln("MyErrorCode destructor detected failure despite success");
    }
    countChecks=countDests=0;
    {
        MyErrorCode errorCode(countChecks, countDests);
        errorCode.set(U_PARSE_ERROR);
    }
    if(countDests!=1) {
        errln("MyErrorCode destructor failed to detect failure");
    }
}

class LocalPointerTest : public IntlTest {
public:
    LocalPointerTest() {}

    void runIndexedTest(int32_t index, UBool exec, const char *&name, char *par=NULL);

    void TestLocalPointer();
    void TestLocalArray();
    void TestLocalXyzPointer();
    void TestLocalXyzPointerNull();
};

static IntlTest *createLocalPointerTest() {
    return new LocalPointerTest();
}

void LocalPointerTest::runIndexedTest(int32_t index, UBool exec, const char *&name, char * /*par*/) {
    if(exec) {
        logln("TestSuite LocalPointerTest: ");
    }
    switch (index) {
        TESTCASE(0, TestLocalPointer);
        TESTCASE(1, TestLocalArray);
        TESTCASE(2, TestLocalXyzPointer);
        TESTCASE(3, TestLocalXyzPointerNull);
        default:
            name="";
            break; // needed to end the loop
    }
}

// Exercise every LocalPointer and LocalPointerBase method.
void LocalPointerTest::TestLocalPointer() {
    // constructor
    LocalPointer<UnicodeString> s(new UnicodeString((UChar32)0x50005));
    // isNULL(), isValid(), operator==(), operator!=()
    if(s.isNull() || !s.isValid() || s==NULL || !(s!=NULL)) {
        errln("LocalPointer constructor or NULL test failure");
        return;
    }
    // getAlias(), operator->, operator*
    if(s.getAlias()->length()!=2 || s->length()!=2 || (*s).length()!=2) {
        errln("LocalPointer access failure");
    }
    // adoptInstead(), orphan()
    s.adoptInstead(new UnicodeString((UChar)0xfffc));
    if(s->length()!=1) {
        errln("LocalPointer adoptInstead(U+FFFC) failure");
    }
    UnicodeString *orphan=s.orphan();
    if(orphan==NULL || orphan->length()!=1 || s.isValid() || s!=NULL) {
        errln("LocalPointer orphan() failure");
    }
    delete orphan;
    // destructor
    s.adoptInstead(new UnicodeString());
    if(s->length()!=0) {
        errln("LocalPointer adoptInstead(empty) failure");
    }
}

// Exercise every LocalArray method (but not LocalPointerBase).
void LocalPointerTest::TestLocalArray() {
    // constructor
    LocalArray<UnicodeString> a(new UnicodeString[2]);
    // operator[]()
    a[0].append((UChar)0x61);
    a[1].append((UChar32)0x60006);
    if(a[0].length()!=1 || a[1].length()!=2) {
        errln("LocalArray access failure");
    }
    // adoptInstead()
    a.adoptInstead(new UnicodeString[4]);
    a[3].append((UChar)0x62).append((UChar)0x63).reverse();
    if(a[3].length()!=2 || a[3][1]!=0x62) {
        errln("LocalArray adoptInstead() failure");
    }
    // destructor
}

#include "unicode/ucnvsel.h"
#include "unicode/ucal.h"
#include "unicode/udatpg.h"
#include "unicode/uidna.h"
#include "unicode/uldnames.h"
#include "unicode/umsg.h"
#include "unicode/unorm2.h"
#include "unicode/uregex.h"
#include "unicode/utrans.h"

// Use LocalXyzPointer types that are not covered elsewhere in the intltest suite.
void LocalPointerTest::TestLocalXyzPointer() {
    IcuTestErrorCode errorCode(*this, "TestLocalXyzPointer");

    static const char *const encoding="ISO-8859-1";
    LocalUConverterSelectorPointer sel(
        ucnvsel_open(&encoding, 1, NULL, UCNV_ROUNDTRIP_SET, errorCode));
    if(errorCode.logIfFailureAndReset("ucnvsel_open()")) {
        return;
    }
    if(sel.isNull()) {
        errln("LocalUConverterSelectorPointer failure");
        return;
    }

#if !UCONFIG_NO_FORMATTING
    LocalUCalendarPointer cal(ucal_open(NULL, 0, "root", UCAL_GREGORIAN, errorCode));
    if(errorCode.logDataIfFailureAndReset("ucal_open()")) {
        return;
    }
    if(cal.isNull()) {
        errln("LocalUCalendarPointer failure");
        return;
    }

    LocalUDateTimePatternGeneratorPointer patgen(udatpg_open("root", errorCode));
    if(errorCode.logIfFailureAndReset("udatpg_open()")) {
        return;
    }
    if(patgen.isNull()) {
        errln("LocalUDateTimePatternGeneratorPointer failure");
        return;
    }

    LocalULocaleDisplayNamesPointer ldn(uldn_open("de-CH", ULDN_STANDARD_NAMES, errorCode));
    if(errorCode.logIfFailureAndReset("uldn_open()")) {
        return;
    }
    if(ldn.isNull()) {
        errln("LocalULocaleDisplayNamesPointer failure");
        return;
    }

    UnicodeString hello=UNICODE_STRING_SIMPLE("Hello {0}!");
    LocalUMessageFormatPointer msg(
        umsg_open(hello.getBuffer(), hello.length(), "root", NULL, errorCode));
    if(errorCode.logIfFailureAndReset("umsg_open()")) {
        return;
    }
    if(msg.isNull()) {
        errln("LocalUMessageFormatPointer failure");
        return;
    }
#endif  /* UCONFIG_NO_FORMATTING  */

#if !UCONFIG_NO_NORMALIZATION
    const UNormalizer2 *nfc=unorm2_getInstance(NULL, "nfc", UNORM2_COMPOSE, errorCode);
    UnicodeSet emptySet;
    LocalUNormalizer2Pointer fn2(unorm2_openFiltered(nfc, emptySet.toUSet(), errorCode));
    if(errorCode.logIfFailureAndReset("unorm2_openFiltered()")) {
        return;
    }
    if(fn2.isNull()) {
        errln("LocalUNormalizer2Pointer failure");
        return;
    }
#endif /* !UCONFIG_NO_NORMALIZATION */

#if !UCONFIG_NO_IDNA
    LocalUIDNAPointer idna(uidna_openUTS46(0, errorCode));
    if(errorCode.logIfFailureAndReset("uidna_openUTS46()")) {
        return;
    }
    if(idna.isNull()) {
        errln("LocalUIDNAPointer failure");
        return;
    }
#endif  /* !UCONFIG_NO_IDNA */

#if !UCONFIG_NO_REGULAR_EXPRESSIONS
    UnicodeString pattern=UNICODE_STRING_SIMPLE("abc|xy+z");
    LocalURegularExpressionPointer regex(
        uregex_open(pattern.getBuffer(), pattern.length(), 0, NULL, errorCode));
    if(errorCode.logIfFailureAndReset("uregex_open()")) {
        return;
    }
    if(regex.isNull()) {
        errln("LocalURegularExpressionPointer failure");
        return;
    }
#endif /* UCONFIG_NO_REGULAR_EXPRESSIONS */

#if !UCONFIG_NO_TRANSLITERATION
    UnicodeString id=UNICODE_STRING_SIMPLE("Grek-Latn");
    LocalUTransliteratorPointer trans(
        utrans_openU(id.getBuffer(), id.length(), UTRANS_FORWARD, NULL, 0, NULL, errorCode));
    if(errorCode.logIfFailureAndReset("utrans_open()")) {
        return;
    }
    if(trans.isNull()) {
        errln("LocalUTransliteratorPointer failure");
        return;
    }
#endif /* !UCONFIG_NO_TRANSLITERATION */

    // destructors
}

// Try LocalXyzPointer types with NULL pointers.
void LocalPointerTest::TestLocalXyzPointerNull() {
    {
        IcuTestErrorCode errorCode(*this, "TestLocalXyzPointerNull/LocalUConverterSelectorPointer");
        static const char *const encoding="ISO-8859-1";
        LocalUConverterSelectorPointer null;
        LocalUConverterSelectorPointer sel(
            ucnvsel_open(&encoding, 1, NULL, UCNV_ROUNDTRIP_SET, errorCode));
        sel.adoptInstead(NULL);
    }
#if !UCONFIG_NO_FORMATTING
    {
        IcuTestErrorCode errorCode(*this, "TestLocalXyzPointerNull/LocalUCalendarPointer");
        LocalUCalendarPointer null;
        LocalUCalendarPointer cal(ucal_open(NULL, 0, "root", UCAL_GREGORIAN, errorCode));
        if(!errorCode.logDataIfFailureAndReset("ucal_open()")) {
            cal.adoptInstead(NULL);
        }
    }
    {
        IcuTestErrorCode errorCode(*this, "TestLocalXyzPointerNull/LocalUDateTimePatternGeneratorPointer");
        LocalUDateTimePatternGeneratorPointer null;
        LocalUDateTimePatternGeneratorPointer patgen(udatpg_open("root", errorCode));
        patgen.adoptInstead(NULL);
    }
    {
        IcuTestErrorCode errorCode(*this, "TestLocalXyzPointerNull/LocalUMessageFormatPointer");
        UnicodeString hello=UNICODE_STRING_SIMPLE("Hello {0}!");
        LocalUMessageFormatPointer null;
        LocalUMessageFormatPointer msg(
            umsg_open(hello.getBuffer(), hello.length(), "root", NULL, errorCode));
        msg.adoptInstead(NULL);
    }
#endif /* !UCONFIG_NO_FORMATTING */

#if !UCONFIG_NO_REGULAR_EXPRESSIONS
    {
        IcuTestErrorCode errorCode(*this, "TestLocalXyzPointerNull/LocalURegularExpressionPointer");
        UnicodeString pattern=UNICODE_STRING_SIMPLE("abc|xy+z");
        LocalURegularExpressionPointer null;
        LocalURegularExpressionPointer regex(
            uregex_open(pattern.getBuffer(), pattern.length(), 0, NULL, errorCode));
        if(!errorCode.logDataIfFailureAndReset("urege_open()")) {
            regex.adoptInstead(NULL);
        }
    }
#endif /* !UCONFIG_NO_REGULAR_EXPRESSIONS */

#if !UCONFIG_NO_TRANSLITERATION
    {
        IcuTestErrorCode errorCode(*this, "TestLocalXyzPointerNull/LocalUTransliteratorPointer");
        UnicodeString id=UNICODE_STRING_SIMPLE("Grek-Latn");
        LocalUTransliteratorPointer null;
        LocalUTransliteratorPointer trans(
            utrans_openU(id.getBuffer(), id.length(), UTRANS_FORWARD, NULL, 0, NULL, errorCode));
        if(!errorCode.logDataIfFailureAndReset("utrans_openU()")) {
            trans.adoptInstead(NULL);
        }
    }
#endif /* !UCONFIG_NO_TRANSLITERATION */

}
