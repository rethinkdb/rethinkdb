/********************************************************************
 * COPYRIGHT: 
 * Copyright (c) 1997-2010, International Business Machines Corporation and
 * others. All Rights Reserved.
 ********************************************************************/
/*   file name:  strtest.cpp
*   encoding:   US-ASCII
*   tab size:   8 (not used)
*   indentation:4
*
*   created on: 1999nov22
*   created by: Markus W. Scherer
*/

#include <string.h>

#include "unicode/utypes.h"
#include "unicode/putil.h"
#include "unicode/std_string.h"
#include "unicode/stringpiece.h"
#include "unicode/unistr.h"
#include "unicode/ustring.h"
#include "charstr.h"
#include "intltest.h"
#include "strtest.h"

StringTest::~StringTest() {}

void StringTest::TestEndian(void) {
    union {
        uint8_t byte;
        uint16_t word;
    } u;
    u.word=0x0100;
    if(U_IS_BIG_ENDIAN!=u.byte) {
        errln("TestEndian: U_IS_BIG_ENDIAN needs to be fixed in platform.h");
    }
}

void StringTest::TestSizeofTypes(void) {
    if(U_SIZEOF_WCHAR_T!=sizeof(wchar_t)) {
        errln("TestSizeofWCharT: U_SIZEOF_WCHAR_T!=sizeof(wchar_t) - U_SIZEOF_WCHAR_T needs to be fixed in platform.h");
    }
#ifdef U_INT64_T_UNAVAILABLE
    errln("int64_t and uint64_t are undefined.");
#else
    if(8!=sizeof(int64_t)) {
        errln("TestSizeofTypes: 8!=sizeof(int64_t) - int64_t needs to be fixed in platform.h");
    }
    if(8!=sizeof(uint64_t)) {
        errln("TestSizeofTypes: 8!=sizeof(uint64_t) - uint64_t needs to be fixed in platform.h");
    }
#endif
    if(8!=sizeof(double)) {
        errln("8!=sizeof(double) - putil.c code may not work");
    }
    if(4!=sizeof(int32_t)) {
        errln("4!=sizeof(int32_t)");
    }
    if(4!=sizeof(uint32_t)) {
        errln("4!=sizeof(uint32_t)");
    }
    if(2!=sizeof(int16_t)) {
        errln("2!=sizeof(int16_t)");
    }
    if(2!=sizeof(uint16_t)) {
        errln("2!=sizeof(uint16_t)");
    }
    if(2!=sizeof(UChar)) {
        errln("2!=sizeof(UChar)");
    }
    if(1!=sizeof(int8_t)) {
        errln("1!=sizeof(int8_t)");
    }
    if(1!=sizeof(uint8_t)) {
        errln("1!=sizeof(uint8_t)");
    }
    if(1!=sizeof(UBool)) {
        errln("1!=sizeof(UBool)");
    }
}

void StringTest::TestCharsetFamily(void) {
    unsigned char c='A';
    if( (U_CHARSET_FAMILY==U_ASCII_FAMILY && c!=0x41) ||
        (U_CHARSET_FAMILY==U_EBCDIC_FAMILY && c!=0xc1)
    ) {
        errln("TestCharsetFamily: U_CHARSET_FAMILY needs to be fixed in platform.h");
    }
}

U_STRING_DECL(ustringVar, "aZ0 -", 5);

void
StringTest::Test_U_STRING() {
    U_STRING_INIT(ustringVar, "aZ0 -", 5);
    if( sizeof(ustringVar)/sizeof(*ustringVar)!=6 ||
        ustringVar[0]!=0x61 ||
        ustringVar[1]!=0x5a ||
        ustringVar[2]!=0x30 ||
        ustringVar[3]!=0x20 ||
        ustringVar[4]!=0x2d ||
        ustringVar[5]!=0
    ) {
        errln("Test_U_STRING: U_STRING_DECL with U_STRING_INIT does not work right! "
              "See putil.h and utypes.h with platform.h.");
    }
}

void
StringTest::Test_UNICODE_STRING() {
    UnicodeString ustringVar=UNICODE_STRING("aZ0 -", 5);
    if( ustringVar.length()!=5 ||
        ustringVar[0]!=0x61 ||
        ustringVar[1]!=0x5a ||
        ustringVar[2]!=0x30 ||
        ustringVar[3]!=0x20 ||
        ustringVar[4]!=0x2d
    ) {
        errln("Test_UNICODE_STRING: UNICODE_STRING does not work right! "
              "See unistr.h and utypes.h with platform.h.");
    }
}

void
StringTest::Test_UNICODE_STRING_SIMPLE() {
    UnicodeString ustringVar=UNICODE_STRING_SIMPLE("aZ0 -");
    if( ustringVar.length()!=5 ||
        ustringVar[0]!=0x61 ||
        ustringVar[1]!=0x5a ||
        ustringVar[2]!=0x30 ||
        ustringVar[3]!=0x20 ||
        ustringVar[4]!=0x2d
    ) {
        errln("Test_UNICODE_STRING_SIMPLE: UNICODE_STRING_SIMPLE does not work right! "
              "See unistr.h and utypes.h with platform.h.");
    }
}

void
StringTest::Test_UTF8_COUNT_TRAIL_BYTES() {
    if(UTF8_COUNT_TRAIL_BYTES(0x7F) != 0
        || UTF8_COUNT_TRAIL_BYTES(0xC0) != 1
        || UTF8_COUNT_TRAIL_BYTES(0xE0) != 2
        || UTF8_COUNT_TRAIL_BYTES(0xF0) != 3)
    {
        errln("Test_UTF8_COUNT_TRAIL_BYTES: UTF8_COUNT_TRAIL_BYTES does not work right! "
              "See utf8.h.");
    }
}

void StringTest::runIndexedTest(int32_t index, UBool exec, const char *&name, char * /*par*/) {
    if(exec) {
        logln("TestSuite Character and String Test: ");
    }
    TESTCASE_AUTO_BEGIN;
    TESTCASE_AUTO(TestEndian);
    TESTCASE_AUTO(TestSizeofTypes);
    TESTCASE_AUTO(TestCharsetFamily);
    TESTCASE_AUTO(Test_U_STRING);
    TESTCASE_AUTO(Test_UNICODE_STRING);
    TESTCASE_AUTO(Test_UNICODE_STRING_SIMPLE);
    TESTCASE_AUTO(Test_UTF8_COUNT_TRAIL_BYTES);
    TESTCASE_AUTO(TestSTLCompatibility);
    TESTCASE_AUTO(TestStdNamespaceQualifier);
    TESTCASE_AUTO(TestUsingStdNamespace);
    TESTCASE_AUTO(TestStringPiece);
    TESTCASE_AUTO(TestStringPieceComparisons);
    TESTCASE_AUTO(TestByteSink);
    TESTCASE_AUTO(TestCheckedArrayByteSink);
    TESTCASE_AUTO(TestStringByteSink);
    TESTCASE_AUTO(TestCharString);
    TESTCASE_AUTO_END;
}

// Syntax check for the correct namespace qualifier for the standard string class.
void
StringTest::TestStdNamespaceQualifier() {
#if U_HAVE_STD_STRING
    U_STD_NSQ string s="abc xyz";
    U_STD_NSQ string t="abc";
    t.append(" ");
    t.append("xyz");
    if(s!=t) {
        errln("standard string concatenation error: %s != %s", s.c_str(), t.c_str());
    }
#endif
}

void
StringTest::TestUsingStdNamespace() {
#if U_HAVE_STD_STRING
    // Now test that "using namespace std;" is defined correctly.
    U_STD_NS_USE

    string s="abc xyz";
    string t="abc";
    t.append(" ");
    t.append("xyz");
    if(s!=t) {
        errln("standard string concatenation error: %s != %s", s.c_str(), t.c_str());
    }
#endif
}

void
StringTest::TestStringPiece() {
    // Default constructor.
    StringPiece empty;
    if(!empty.empty() || empty.data()!=NULL || empty.length()!=0 || empty.size()!=0) {
        errln("StringPiece() failed");
    }
    // Construct from NULL const char * pointer.
    StringPiece null(NULL);
    if(!null.empty() || null.data()!=NULL || null.length()!=0 || null.size()!=0) {
        errln("StringPiece(NULL) failed");
    }
    // Construct from const char * pointer.
    static const char *abc_chars="abc";
    StringPiece abc(abc_chars);
    if(abc.empty() || abc.data()!=abc_chars || abc.length()!=3 || abc.size()!=3) {
        errln("StringPiece(abc_chars) failed");
    }
    // Construct from const char * pointer and length.
    static const char *abcdefg_chars="abcdefg";
    StringPiece abcd(abcdefg_chars, 4);
    if(abcd.empty() || abcd.data()!=abcdefg_chars || abcd.length()!=4 || abcd.size()!=4) {
        errln("StringPiece(abcdefg_chars, 4) failed");
    }
#if U_HAVE_STD_STRING
    // Construct from std::string.
    U_STD_NSQ string uvwxyz_string("uvwxyz");
    StringPiece uvwxyz(uvwxyz_string);
    if(uvwxyz.empty() || uvwxyz.data()!=uvwxyz_string.data() || uvwxyz.length()!=6 || uvwxyz.size()!=6) {
        errln("StringPiece(uvwxyz_string) failed");
    }
#endif
    // Substring constructor with pos.
    StringPiece sp(abcd, -1);
    if(sp.empty() || sp.data()!=abcdefg_chars || sp.length()!=4 || sp.size()!=4) {
        errln("StringPiece(abcd, -1) failed");
    }
    sp=StringPiece(abcd, 5);
    if(!sp.empty() || sp.length()!=0 || sp.size()!=0) {
        errln("StringPiece(abcd, 5) failed");
    }
    sp=StringPiece(abcd, 2);
    if(sp.empty() || sp.data()!=abcdefg_chars+2 || sp.length()!=2 || sp.size()!=2) {
        errln("StringPiece(abcd, -1) failed");
    }
    // Substring constructor with pos and len.
    sp=StringPiece(abcd, -1, 8);
    if(sp.empty() || sp.data()!=abcdefg_chars || sp.length()!=4 || sp.size()!=4) {
        errln("StringPiece(abcd, -1, 8) failed");
    }
    sp=StringPiece(abcd, 5, 8);
    if(!sp.empty() || sp.length()!=0 || sp.size()!=0) {
        errln("StringPiece(abcd, 5, 8) failed");
    }
    sp=StringPiece(abcd, 2, 8);
    if(sp.empty() || sp.data()!=abcdefg_chars+2 || sp.length()!=2 || sp.size()!=2) {
        errln("StringPiece(abcd, -1) failed");
    }
    sp=StringPiece(abcd, 2, -1);
    if(!sp.empty() || sp.length()!=0 || sp.size()!=0) {
        errln("StringPiece(abcd, 5, -1) failed");
    }
    // static const npos
    const int32_t *ptr_npos=&StringPiece::npos;
    if(StringPiece::npos!=0x7fffffff || *ptr_npos!=0x7fffffff) {
        errln("StringPiece::npos!=0x7fffffff");
    }
    // substr() method with pos, using len=npos.
    sp=abcd.substr(-1);
    if(sp.empty() || sp.data()!=abcdefg_chars || sp.length()!=4 || sp.size()!=4) {
        errln("abcd.substr(-1) failed");
    }
    sp=abcd.substr(5);
    if(!sp.empty() || sp.length()!=0 || sp.size()!=0) {
        errln("abcd.substr(5) failed");
    }
    sp=abcd.substr(2);
    if(sp.empty() || sp.data()!=abcdefg_chars+2 || sp.length()!=2 || sp.size()!=2) {
        errln("abcd.substr(-1) failed");
    }
    // substr() method with pos and len.
    sp=abcd.substr(-1, 8);
    if(sp.empty() || sp.data()!=abcdefg_chars || sp.length()!=4 || sp.size()!=4) {
        errln("abcd.substr(-1, 8) failed");
    }
    sp=abcd.substr(5, 8);
    if(!sp.empty() || sp.length()!=0 || sp.size()!=0) {
        errln("abcd.substr(5, 8) failed");
    }
    sp=abcd.substr(2, 8);
    if(sp.empty() || sp.data()!=abcdefg_chars+2 || sp.length()!=2 || sp.size()!=2) {
        errln("abcd.substr(-1) failed");
    }
    sp=abcd.substr(2, -1);
    if(!sp.empty() || sp.length()!=0 || sp.size()!=0) {
        errln("abcd.substr(5, -1) failed");
    }
    // clear()
    sp=abcd;
    sp.clear();
    if(!sp.empty() || sp.data()!=NULL || sp.length()!=0 || sp.size()!=0) {
        errln("abcd.clear() failed");
    }
    // remove_prefix()
    sp=abcd;
    sp.remove_prefix(-1);
    if(sp.empty() || sp.data()!=abcdefg_chars || sp.length()!=4 || sp.size()!=4) {
        errln("abcd.remove_prefix(-1) failed");
    }
    sp=abcd;
    sp.remove_prefix(2);
    if(sp.empty() || sp.data()!=abcdefg_chars+2 || sp.length()!=2 || sp.size()!=2) {
        errln("abcd.remove_prefix(2) failed");
    }
    sp=abcd;
    sp.remove_prefix(5);
    if(!sp.empty() || sp.length()!=0 || sp.size()!=0) {
        errln("abcd.remove_prefix(5) failed");
    }
    // remove_suffix()
    sp=abcd;
    sp.remove_suffix(-1);
    if(sp.empty() || sp.data()!=abcdefg_chars || sp.length()!=4 || sp.size()!=4) {
        errln("abcd.remove_suffix(-1) failed");
    }
    sp=abcd;
    sp.remove_suffix(2);
    if(sp.empty() || sp.data()!=abcdefg_chars || sp.length()!=2 || sp.size()!=2) {
        errln("abcd.remove_suffix(2) failed");
    }
    sp=abcd;
    sp.remove_suffix(5);
    if(!sp.empty() || sp.length()!=0 || sp.size()!=0) {
        errln("abcd.remove_suffix(5) failed");
    }
}

void
StringTest::TestStringPieceComparisons() {
    StringPiece empty;
    StringPiece null(NULL);
    StringPiece abc("abc");
    StringPiece abcd("abcdefg", 4);
    StringPiece abx("abx");
    if(empty!=null) {
        errln("empty!=null");
    }
    if(empty==abc) {
        errln("empty==abc");
    }
    if(abc==abcd) {
        errln("abc==abcd");
    }
    abcd.remove_suffix(1);
    if(abc!=abcd) {
        errln("abc!=abcd.remove_suffix(1)");
    }
    if(abc==abx) {
        errln("abc==abx");
    }
}

// Verify that ByteSink is subclassable and Flush() overridable.
class SimpleByteSink : public ByteSink {
public:
    SimpleByteSink(char *outbuf) : fOutbuf(outbuf), fLength(0) {}
    virtual void Append(const char *bytes, int32_t n) {
        if(fOutbuf != bytes) {
            memcpy(fOutbuf, bytes, n);
        }
        fOutbuf += n;
        fLength += n;
    }
    virtual void Flush() { Append("z", 1); }
    int32_t length() { return fLength; }
private:
    char *fOutbuf;
    int32_t fLength;
};

// Test the ByteSink base class.
void
StringTest::TestByteSink() {
    char buffer[20];
    buffer[4] = '!';
    SimpleByteSink sink(buffer);
    sink.Append("abc", 3);
    sink.Flush();
    if(!(sink.length() == 4 && 0 == memcmp("abcz", buffer, 4) && buffer[4] == '!')) {
        errln("ByteSink (SimpleByteSink) did not Append() or Flush() as expected");
        return;
    }
    char scratch[20];
    int32_t capacity = -1;
    char *dest = sink.GetAppendBuffer(0, 50, scratch, (int32_t)sizeof(scratch), &capacity);
    if(dest != NULL || capacity != 0) {
        errln("ByteSink.GetAppendBuffer(min_capacity<1) did not properly return NULL[0]");
        return;
    }
    dest = sink.GetAppendBuffer(10, 50, scratch, 9, &capacity);
    if(dest != NULL || capacity != 0) {
        errln("ByteSink.GetAppendBuffer(scratch_capacity<min_capacity) did not properly return NULL[0]");
        return;
    }
    dest = sink.GetAppendBuffer(5, 50, scratch, (int32_t)sizeof(scratch), &capacity);
    if(dest != scratch || capacity != (int32_t)sizeof(scratch)) {
        errln("ByteSink.GetAppendBuffer() did not properly return the scratch buffer");
    }
}

void
StringTest::TestCheckedArrayByteSink() {
    char buffer[20];  // < 26 for the test code to work
    buffer[3] = '!';
    CheckedArrayByteSink sink(buffer, (int32_t)sizeof(buffer));
    sink.Append("abc", 3);
    if(!(sink.NumberOfBytesAppended() == 3 && sink.NumberOfBytesWritten() == 3 &&
         0 == memcmp("abc", buffer, 3) && buffer[3] == '!') &&
         !sink.Overflowed()
    ) {
        errln("CheckedArrayByteSink did not Append() as expected");
        return;
    }
    char scratch[10];
    int32_t capacity = -1;
    char *dest = sink.GetAppendBuffer(0, 50, scratch, (int32_t)sizeof(scratch), &capacity);
    if(dest != NULL || capacity != 0) {
        errln("CheckedArrayByteSink.GetAppendBuffer(min_capacity<1) did not properly return NULL[0]");
        return;
    }
    dest = sink.GetAppendBuffer(10, 50, scratch, 9, &capacity);
    if(dest != NULL || capacity != 0) {
        errln("CheckedArrayByteSink.GetAppendBuffer(scratch_capacity<min_capacity) did not properly return NULL[0]");
        return;
    }
    dest = sink.GetAppendBuffer(10, 50, scratch, (int32_t)sizeof(scratch), &capacity);
    if(dest != buffer + 3 || capacity != (int32_t)sizeof(buffer) - 3) {
        errln("CheckedArrayByteSink.GetAppendBuffer() did not properly return its own buffer");
        return;
    }
    memcpy(dest, "defghijklm", 10);
    sink.Append(dest, 10);
    if(!(sink.NumberOfBytesAppended() == 13 && sink.NumberOfBytesWritten() == 13 &&
         0 == memcmp("abcdefghijklm", buffer, 13) &&
         !sink.Overflowed())
    ) {
        errln("CheckedArrayByteSink did not Append(its own buffer) as expected");
        return;
    }
    dest = sink.GetAppendBuffer(10, 50, scratch, (int32_t)sizeof(scratch), &capacity);
    if(dest != scratch || capacity != (int32_t)sizeof(scratch)) {
        errln("CheckedArrayByteSink.GetAppendBuffer() did not properly return the scratch buffer");
    }
    memcpy(dest, "nopqrstuvw", 10);
    sink.Append(dest, 10);
    if(!(sink.NumberOfBytesAppended() == 23 &&
         sink.NumberOfBytesWritten() == (int32_t)sizeof(buffer) &&
         0 == memcmp("abcdefghijklmnopqrstuvwxyz", buffer, (int32_t)sizeof(buffer)) &&
         sink.Overflowed())
    ) {
        errln("CheckedArrayByteSink did not Append(scratch buffer) as expected");
        return;
    }
    sink.Reset().Append("123", 3);
    if(!(sink.NumberOfBytesAppended() == 3 && sink.NumberOfBytesWritten() == 3 &&
         0 == memcmp("123defghijklmnopqrstuvwxyz", buffer, (int32_t)sizeof(buffer)) &&
         !sink.Overflowed())
    ) {
        errln("CheckedArrayByteSink did not Reset().Append() as expected");
        return;
    }
}

void
StringTest::TestStringByteSink() {
#if U_HAVE_STD_STRING
    // Not much to test because only the constructor and Append()
    // are implemented, and trivially so.
    U_STD_NSQ string result("abc");  // std::string
    StringByteSink<U_STD_NSQ string> sink(&result);
    sink.Append("def", 3);
    if(result != "abcdef") {
        errln("StringByteSink did not Append() as expected");
    }
#endif
}

#if defined(U_WINDOWS) && defined(_MSC_VER)
#include <vector>
#endif

void
StringTest::TestSTLCompatibility() {
#if defined(U_WINDOWS) && defined(_MSC_VER)
    /* Just make sure that it compiles with STL's placement new usage. */
    std::vector<UnicodeString> myvect;
    myvect.push_back(UnicodeString("blah"));
#endif
}

void
StringTest::TestCharString() {
    IcuTestErrorCode errorCode(*this, "TestCharString()");
    char expected[400];
    static const char longStr[] =
        "This is a long string that is meant to cause reallocation of the internal buffer of CharString.";
    CharString chStr(longStr, errorCode);
    if (0 != strcmp(longStr, chStr.data()) || (int32_t)strlen(longStr) != chStr.length()) {
        errln("CharString(longStr) failed.");
    }
    CharString test("Test", errorCode);
    CharString copy(test,errorCode);
    copy.copyFrom(chStr, errorCode);
    if (0 != strcmp(longStr, copy.data()) || (int32_t)strlen(longStr) != copy.length()) {
        errln("CharString.copyFrom() failed.");
    }
    StringPiece sp(chStr.toStringPiece());
    sp.remove_prefix(4);
    chStr.append(sp, errorCode).append(chStr, errorCode);
    strcpy(expected, longStr);
    strcat(expected, longStr+4);
    strcat(expected, longStr);
    strcat(expected, longStr+4);
    if (0 != strcmp(expected, chStr.data()) || (int32_t)strlen(expected) != chStr.length()) {
        errln("CharString(longStr).append(substring of self).append(self) failed.");
    }
    chStr.clear().append("abc", errorCode).append("defghij", 3, errorCode);
    if (0 != strcmp("abcdef", chStr.data()) || 6 != chStr.length()) {
        errln("CharString.clear().append(abc).append(defghij, 3) failed.");
    }
    chStr.appendInvariantChars(UNICODE_STRING_SIMPLE(
        "This is a long string that is meant to cause reallocation of the internal buffer of CharString."),
        errorCode);
    strcpy(expected, "abcdef");
    strcat(expected, longStr);
    if (0 != strcmp(expected, chStr.data()) || (int32_t)strlen(expected) != chStr.length()) {
        errln("CharString.appendInvariantChars(longStr) failed.");
    }
    int32_t appendCapacity = 0;
    char *buffer = chStr.getAppendBuffer(5, 10, appendCapacity, errorCode);
    if (errorCode.isFailure()) {
        return;
    }
    memcpy(buffer, "*****", 5);
    chStr.append(buffer, 5, errorCode);
    chStr.truncate(chStr.length()-3);
    strcat(expected, "**");
    if (0 != strcmp(expected, chStr.data()) || (int32_t)strlen(expected) != chStr.length()) {
        errln("CharString.getAppendBuffer().append(**) failed.");
    }
}
