/*  
**********************************************************************
*   Copyright (C) 2007, International Business Machines
*   Corporation and others.  All Rights Reserved.
**********************************************************************
*   file name:  unisetperf.cpp
*   encoding:   US-ASCII
*   tab size:   8 (not used)
*   indentation:4
*
*   created on: 2007jan31
*   created by: Markus Scherer
*/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "unicode/uperf.h"
#include "unicode/uniset.h"
#include "unicode/unistr.h"
#include "uoptions.h"

#define LENGTHOF(array) (int32_t)(sizeof(array)/sizeof((array)[0]))

// Command-line options specific to unisetperf.
// Options do not have abbreviations: Force readable command lines.
// (Using U+0001 for abbreviation characters.)
enum {
    SET_PATTERN,
    FAST_TYPE,
    UNISETPERF_OPTIONS_COUNT
};

static UOption options[UNISETPERF_OPTIONS_COUNT]={
    UOPTION_DEF("pattern", '\x01', UOPT_REQUIRES_ARG),
    UOPTION_DEF("type",    '\x01', UOPT_REQUIRES_ARG)
};

static const char *const unisetperf_usage =
    "\t--pattern   UnicodeSet pattern for instantiation.\n"
    "\t            Default: [:ID_Continue:]\n"
    "\t--type      Type of UnicodeSet: slow fast\n"
    "\t            Default: slow\n";

// Test object with setup data.
class UnicodeSetPerformanceTest : public UPerfTest {
public:
    UnicodeSetPerformanceTest(int32_t argc, const char *argv[], UErrorCode &status)
            : UPerfTest(argc, argv, options, LENGTHOF(options), unisetperf_usage, status),
              utf8(NULL), utf8Length(0), countInputCodePoints(0), spanCount(0) {
        if (U_SUCCESS(status)) {
            UnicodeString pattern=UnicodeString(options[SET_PATTERN].value, -1, US_INV).unescape();
            set.applyPattern(pattern, status);
            prefrozen=set;
            if(0==strcmp(options[FAST_TYPE].value, "fast")) {
                set.freeze();
            }

            int32_t inputLength;
            UPerfTest::getBuffer(inputLength, status);
            if(U_SUCCESS(status) && inputLength>0) {
                countInputCodePoints = u_countChar32(buffer, bufferLen);

                countSpans();

                // Preflight the UTF-8 length and allocate utf8.
                u_strToUTF8(NULL, 0, &utf8Length, buffer, bufferLen, &status);
                if(status==U_BUFFER_OVERFLOW_ERROR) {
                    utf8=(char *)malloc(utf8Length);
                    if(utf8!=NULL) {
                        status=U_ZERO_ERROR;
                        u_strToUTF8(utf8, utf8Length, NULL, buffer, bufferLen, &status);
                    } else {
                        status=U_MEMORY_ALLOCATION_ERROR;
                    }
                }

                if(verbose) {
                    printf("code points:%ld  len16:%ld  len8:%ld  spans:%ld  "
                           "cp/span:%.3g  UChar/span:%.3g  B/span:%.3g  B/cp:%.3g\n",
                           (long)countInputCodePoints, (long)bufferLen, (long)utf8Length, (long)spanCount,
                           (double)countInputCodePoints/spanCount, (double)bufferLen/spanCount, (double)utf8Length/spanCount,
                           (double)utf8Length/countInputCodePoints);
                }
            }
        }
    }

    virtual UPerfFunction* runIndexedTest(int32_t index, UBool exec, const char* &name, char* par = NULL);

    // Count spans of characters that are in the set,
    // and spans of characters that are not in the set.
    // If the very first character is in the set, then one additional
    // not-span is counted.
    void countSpans() {
        const UChar *s=getBuffer();
        int32_t length=getBufferLen();
        int32_t i=0;
        UBool tf=FALSE;
        while(i<length) {
            i=span(s, length, i, tf);
            tf=(UBool)(!tf);
            ++spanCount;
        }
    }
    int32_t span(const UChar *s, int32_t length, int32_t start, UBool tf) const {
        UChar32 c;
        int32_t prev;
        while((prev=start)<length) {
            U16_NEXT(s, start, length, c);
            if(tf!=set.contains(c)) {
                break;
            }
        }
        return prev;
    }

    const UChar *getBuffer() const { return buffer; }
    int32_t getBufferLen() const { return bufferLen; }

    char *utf8;
    int32_t utf8Length;

    // Number of code points in the input text.
    int32_t countInputCodePoints;
    int32_t spanCount;

    UnicodeSet set;
    UnicodeSet prefrozen;
};

// Performance test function object.
class Command : public UPerfFunction {
protected:
    Command(const UnicodeSetPerformanceTest &testcase) : testcase(testcase) {}

public:
    virtual ~Command() {}

    // virtual void call(UErrorCode* pErrorCode) { ... }

    virtual long getOperationsPerIteration() {
        // Number of code points tested:
        // Input code points, plus one for the end of each span except the last span.
        return testcase.countInputCodePoints+testcase.spanCount-1;
    }

    virtual long getEventsPerIteration() {
        return testcase.spanCount;
    }

    const UnicodeSetPerformanceTest &testcase;
};

class Contains : public Command {
protected:
    Contains(const UnicodeSetPerformanceTest &testcase) : Command(testcase) {
        // Verify that the frozen set is equal to the unfrozen one.
        UnicodeSet set;
        UChar32 c;

        for(c=0; c<=0x10ffff; ++c) {
            if(testcase.set.contains(c)) {
                set.add(c);
            }
        }
        if(set!=testcase.set) {
            fprintf(stderr, "error: frozen set != original!\n");
        }
    }
public:
    static UPerfFunction* get(const UnicodeSetPerformanceTest &testcase) {
        return new Contains(testcase);
    }
    virtual void call(UErrorCode* pErrorCode) {
        const UnicodeSet &set=testcase.set;
        const UChar *s=testcase.getBuffer();
        int32_t length=testcase.getBufferLen();
        int32_t count=0;
        int32_t i=0;
        UBool tf=FALSE;
        while(i<length) {
            i+=span(set, s+i, length-i, tf);
            tf=(UBool)(!tf);
            ++count;
        }
        if(count!=testcase.spanCount) {
            fprintf(stderr, "error: Contains() count=%ld != %ld=UnicodeSetPerformanceTest.spanCount\n",
                    (long)count, (long)testcase.spanCount);
        }
    }
    static int32_t span(const UnicodeSet &set, const UChar *s, int32_t length, UBool tf) {
        UChar32 c;
        int32_t start=0, prev;
        while((prev=start)<length) {
            U16_NEXT(s, start, length, c);
            if(tf!=set.contains(c)) {
                break;
            }
        }
        return prev;
    }
};

class SpanUTF16 : public Command {
protected:
    SpanUTF16(const UnicodeSetPerformanceTest &testcase) : Command(testcase) {
        // Verify that the frozen set is equal to the unfrozen one.
        UnicodeSet set;
        UChar utf16[2];
        UChar32 c, c2;

        for(c=0; c<=0xffff; ++c) {
            utf16[0]=(UChar)c;
            if(testcase.set.span(utf16, 1, USET_SPAN_CONTAINED)>0) {
                set.add(c);
            }
        }
        for(c=0xd800; c<=0xdbff; ++c) {
            utf16[0]=(UChar)c;
            for(c2=0xdc00; c2<=0xdfff; ++c2) {
                utf16[1]=(UChar)c2;
                if(testcase.set.span(utf16, 2, USET_SPAN_CONTAINED)>0) {
                    set.add(U16_GET_SUPPLEMENTARY(c, c2));
                }
            }
        }

        if(set!=testcase.set) {
            fprintf(stderr, "error: frozen set != original!\n");
        }
    }
public:
    static UPerfFunction* get(const UnicodeSetPerformanceTest &testcase) {
        return new SpanUTF16(testcase);
    }
    virtual void call(UErrorCode* pErrorCode) {
        const UnicodeSet &set=testcase.set;
        const UChar *s=testcase.getBuffer();
        int32_t length=testcase.getBufferLen();
        int32_t count=0;
        int32_t i=0;
        UBool tf=FALSE;
        while(i<length) {
            i+=set.span(s+i, length-i, (USetSpanCondition)tf);
            tf=(UBool)(!tf);
            ++count;
        }
        if(count!=testcase.spanCount) {
            fprintf(stderr, "error: SpanUTF16() count=%ld != %ld=UnicodeSetPerformanceTest.spanCount\n",
                    (long)count, (long)testcase.spanCount);
        }
    }
};

class SpanBackUTF16 : public Command {
protected:
    SpanBackUTF16(const UnicodeSetPerformanceTest &testcase) : Command(testcase) {
        // Verify that the frozen set is equal to the unfrozen one.
        UnicodeSet set;
        UChar utf16[2];
        UChar32 c, c2;

        for(c=0; c<=0xffff; ++c) {
            utf16[0]=(UChar)c;
            if(testcase.set.spanBack(utf16, 1, USET_SPAN_CONTAINED)==0) {
                set.add(c);
            }
        }
        for(c=0xd800; c<=0xdbff; ++c) {
            utf16[0]=(UChar)c;
            for(c2=0xdc00; c2<=0xdfff; ++c2) {
                utf16[1]=(UChar)c2;
                if(testcase.set.spanBack(utf16, 2, USET_SPAN_CONTAINED)==0) {
                    set.add(U16_GET_SUPPLEMENTARY(c, c2));
                }
            }
        }

        if(set!=testcase.set) {
            fprintf(stderr, "error: frozen set != original!\n");
        }
    }
public:
    static UPerfFunction* get(const UnicodeSetPerformanceTest &testcase) {
        return new SpanBackUTF16(testcase);
    }
    virtual void call(UErrorCode* pErrorCode) {
        const UnicodeSet &set=testcase.set;
        const UChar *s=testcase.getBuffer();
        int32_t length=testcase.getBufferLen();
        int32_t count=0;
        /*
         * Get the same spans as with span() where we always start with a not-contained span.
         * If testcase.spanCount is an odd number, then the last span() was not-contained.
         * The last spanBack() must be not-contained to match the first span().
         */
        UBool tf=(UBool)((testcase.spanCount&1)==0);
        while(length>0 || !tf) {
            length=set.spanBack(s, length, (USetSpanCondition)tf);
            tf=(UBool)(!tf);
            ++count;
        }
        if(count!=testcase.spanCount) {
            fprintf(stderr, "error: SpanBackUTF16() count=%ld != %ld=UnicodeSetPerformanceTest.spanCount\n",
                    (long)count, (long)testcase.spanCount);
        }
    }
};

class SpanUTF8 : public Command {
protected:
    SpanUTF8(const UnicodeSetPerformanceTest &testcase) : Command(testcase) {
        // Verify that the frozen set is equal to the unfrozen one.
        UnicodeSet set;
        char utf8[4];
        UChar32 c;
        int32_t length;

        for(c=0; c<=0x10ffff; ++c) {
            if(c==0xd800) {
                c=0xe000;
            }
            length=0;
            U8_APPEND_UNSAFE(utf8, length, c);
            if(testcase.set.spanUTF8(utf8, length, USET_SPAN_CONTAINED)>0) {
                set.add(c);
            }
        }
        if(set!=testcase.set) {
            fprintf(stderr, "error: frozen set != original!\n");
        }
    }
public:
    static UPerfFunction* get(const UnicodeSetPerformanceTest &testcase) {
        return new SpanUTF8(testcase);
    }
    virtual void call(UErrorCode* pErrorCode) {
        const UnicodeSet &set=testcase.set;
        const char *s=testcase.utf8;
        int32_t length=testcase.utf8Length;
        int32_t count=0;
        int32_t i=0;
        UBool tf=FALSE;
        while(i<length) {
            i+=set.spanUTF8(s+i, length-i, (USetSpanCondition)tf);
            tf=(UBool)(!tf);
            ++count;
        }
        if(count!=testcase.spanCount) {
            fprintf(stderr, "error: SpanUTF8() count=%ld != %ld=UnicodeSetPerformanceTest.spanCount\n",
                    (long)count, (long)testcase.spanCount);
        }
    }
};

class SpanBackUTF8 : public Command {
protected:
    SpanBackUTF8(const UnicodeSetPerformanceTest &testcase) : Command(testcase) {
        // Verify that the frozen set is equal to the unfrozen one.
        UnicodeSet set;
        char utf8[4];
        UChar32 c;
        int32_t length;

        for(c=0; c<=0x10ffff; ++c) {
            if(c==0xd800) {
                c=0xe000;
            }
            length=0;
            U8_APPEND_UNSAFE(utf8, length, c);
            if(testcase.set.spanBackUTF8(utf8, length, USET_SPAN_CONTAINED)==0) {
                set.add(c);
            }
        }
        if(set!=testcase.set) {
            fprintf(stderr, "error: frozen set != original!\n");
        }
    }
public:
    static UPerfFunction* get(const UnicodeSetPerformanceTest &testcase) {
        return new SpanBackUTF8(testcase);
    }
    virtual void call(UErrorCode* pErrorCode) {
        const UnicodeSet &set=testcase.set;
        const char *s=testcase.utf8;
        int32_t length=testcase.utf8Length;
        int32_t count=0;
        /*
         * Get the same spans as with span() where we always start with a not-contained span.
         * If testcase.spanCount is an odd number, then the last span() was not-contained.
         * The last spanBack() must be not-contained to match the first span().
         */
        UBool tf=(UBool)((testcase.spanCount&1)==0);
        while(length>0 || !tf) {
            length=set.spanBackUTF8(s, length, (USetSpanCondition)tf);
            tf=(UBool)(!tf);
            ++count;
        }
        if(count!=testcase.spanCount) {
            fprintf(stderr, "error: SpanBackUTF8() count=%ld != %ld=UnicodeSetPerformanceTest.spanCount\n",
                    (long)count, (long)testcase.spanCount);
        }
    }
};

UPerfFunction* UnicodeSetPerformanceTest::runIndexedTest(int32_t index, UBool exec, const char* &name, char* par) {
    switch (index) {
        case 0: name = "Contains";     if (exec) return Contains::get(*this); break;
        case 1: name = "SpanUTF16";    if (exec) return SpanUTF16::get(*this); break;
        case 2: name = "SpanBackUTF16";if (exec) return SpanBackUTF16::get(*this); break;
        case 3: name = "SpanUTF8";     if (exec) return SpanUTF8::get(*this); break;
        case 4: name = "SpanBackUTF8"; if (exec) return SpanBackUTF8::get(*this); break;
        default: name = ""; break;
    }
    return NULL;
}

int main(int argc, const char *argv[])
{
    // Default values for command-line options.
    options[SET_PATTERN].value = "[:ID_Continue:]";
    options[FAST_TYPE].value = "slow";

    UErrorCode status = U_ZERO_ERROR;
    UnicodeSetPerformanceTest test(argc, argv, status);

	if (U_FAILURE(status)){
        printf("The error is %s\n", u_errorName(status));
        test.usage();
        return status;
    }
        
    if (test.run() == FALSE){
        fprintf(stderr, "FAILED: Tests could not be run, please check the "
			            "arguments.\n");
        return 1;
    }

    return 0;
}
