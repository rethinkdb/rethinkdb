/*  
 **********************************************************************
 *   Copyright (C) 2002-2008, International Business Machines
 *   Corporation and others.  All Rights Reserved.
 **********************************************************************
 *  file name:  utrie2perf.cpp
 *  encoding:   US-ASCII
 *  tab size:   8 (not used)
 *  indentation:4
 *
 *  created on: 2008sep07
 *  created by: Markus W. Scherer
 *
 *  Performance test program for UTrie2.
 */

#include <stdio.h>
#include <stdlib.h>
#include "unicode/uchar.h"
#include "unicode/unorm.h"
#include "unicode/uperf.h"
#include "uoptions.h"

#define LENGTHOF(array) (int32_t)(sizeof(array)/sizeof((array)[0]))

#if 0
// Left over from when icu/branches/markus/utf8 could use both old UTrie
// and new UTrie2, switched with #if in unorm.cpp and ubidi_props.c.
// Comparative benchmarks were done in that branch on revision r24630
// and earlier.
U_CAPI void U_EXPORT2
unorm_initUTrie2(UErrorCode *pErrorCode);

U_CAPI void U_EXPORT2
ubidi_initUTrie2(UErrorCode *pErrorCode);
#endif

U_NAMESPACE_BEGIN

class UnicodeSet;

U_NAMESPACE_END

// Test object.
class UTrie2PerfTest : public UPerfTest {
public:
    UTrie2PerfTest(int32_t argc, const char *argv[], UErrorCode &status)
            : UPerfTest(argc, argv, NULL, 0, "", status),
              utf8(NULL), utf8Length(0), countInputCodePoints(0) {
        if (U_SUCCESS(status)) {
#if 0       // See comment at unorm_initUTrie2() forward declaration.
            unorm_initUTrie2(&status);
            ubidi_initUTrie2(&status);
#endif
            int32_t inputLength;
            UPerfTest::getBuffer(inputLength, status);
            if(U_SUCCESS(status) && inputLength>0) {
                countInputCodePoints = u_countChar32(buffer, bufferLen);

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
                    printf("code points:%ld  len16:%ld  len8:%ld  "
                           "B/cp:%.3g\n",
                           (long)countInputCodePoints, (long)bufferLen, (long)utf8Length,
                           (double)utf8Length/countInputCodePoints);
                }
            }
        }
    }

    virtual UPerfFunction* runIndexedTest(int32_t index, UBool exec, const char* &name, char* par = NULL);

    const UChar *getBuffer() const { return buffer; }
    int32_t getBufferLen() const { return bufferLen; }

    char *utf8;
    int32_t utf8Length;

    // Number of code points in the input text.
    int32_t countInputCodePoints;
};

// Performance test function object.
class Command : public UPerfFunction {
protected:
    Command(const UTrie2PerfTest &testcase) : testcase(testcase) {}

public:
    virtual ~Command() {}

    // virtual void call(UErrorCode* pErrorCode) { ... }

    virtual long getOperationsPerIteration() {
        // Number of code points tested.
        return testcase.countInputCodePoints;
    }

    // virtual long getEventsPerIteration();

    const UTrie2PerfTest &testcase;
    UNormalizationCheckResult qcResult;
};

class CheckFCD : public Command {
protected:
    CheckFCD(const UTrie2PerfTest &testcase) : Command(testcase) {}
public:
    static UPerfFunction* get(const UTrie2PerfTest &testcase) {
        return new CheckFCD(testcase);
    }
    virtual void call(UErrorCode* pErrorCode) {
        UErrorCode errorCode=U_ZERO_ERROR;
        qcResult=unorm_quickCheck(testcase.getBuffer(), testcase.getBufferLen(),
                                  UNORM_FCD, &errorCode);
        if(U_FAILURE(errorCode)) {
            fprintf(stderr, "error: unorm_quickCheck(UNORM_FCD) failed: %s\n",
                    u_errorName(errorCode));
        }
    }
};

#if 0  // See comment at unorm_initUTrie2() forward declaration.

class CheckFCDAlwaysGet : public Command {
protected:
    CheckFCDAlwaysGet(const UTrie2PerfTest &testcase) : Command(testcase) {}
public:
    static UPerfFunction* get(const UTrie2PerfTest &testcase) {
        return new CheckFCDAlwaysGet(testcase);
    }
    virtual void call(UErrorCode* pErrorCode) {
        UErrorCode errorCode=U_ZERO_ERROR;
        qcResult=unorm_quickCheck(testcase.getBuffer(), testcase.getBufferLen(),
                                  UNORM_FCD_ALWAYS_GET, &errorCode);
        if(U_FAILURE(errorCode)) {
            fprintf(stderr, "error: unorm_quickCheck(UNORM_FCD) failed: %s\n",
                    u_errorName(errorCode));
        }
    }
};

U_CAPI UBool U_EXPORT2
unorm_checkFCDUTF8(const uint8_t *src, int32_t srcLength, const UnicodeSet *nx);

class CheckFCDUTF8 : public Command {
protected:
    CheckFCDUTF8(const UTrie2PerfTest &testcase) : Command(testcase) {}
public:
    static UPerfFunction* get(const UTrie2PerfTest &testcase) {
        return new CheckFCDUTF8(testcase);
    }
    virtual void call(UErrorCode* pErrorCode) {
        UBool isFCD=unorm_checkFCDUTF8((const uint8_t *)testcase.utf8, testcase.utf8Length, NULL);
        if(isFCD>1) {
            fprintf(stderr, "error: bogus result from unorm_checkFCDUTF8()\n");
        }
    }
};

#endif

class ToNFC : public Command {
protected:
    ToNFC(const UTrie2PerfTest &testcase) : Command(testcase) {
        UErrorCode errorCode=U_ZERO_ERROR;
        destCapacity=unorm_normalize(testcase.getBuffer(), testcase.getBufferLen(),
                                     UNORM_NFC, 0,
                                     NULL, 0,
                                     &errorCode);
        dest=new UChar[destCapacity];
    }
    ~ToNFC() {
        delete [] dest;
    }
public:
    static UPerfFunction* get(const UTrie2PerfTest &testcase) {
        return new ToNFC(testcase);
    }
    virtual void call(UErrorCode* pErrorCode) {
        UErrorCode errorCode=U_ZERO_ERROR;
        int32_t destLength=unorm_normalize(testcase.getBuffer(), testcase.getBufferLen(),
                                           UNORM_NFC, 0,
                                           dest, destCapacity,
                                           &errorCode);
        if(U_FAILURE(errorCode) || destLength!=destCapacity) {
            fprintf(stderr, "error: unorm_normalize(UNORM_NFC) failed: %s\n",
                    u_errorName(errorCode));
        }
    }

private:
    UChar *dest;
    int32_t destCapacity;
};

class GetBiDiClass : public Command {
protected:
    GetBiDiClass(const UTrie2PerfTest &testcase) : Command(testcase) {}
public:
    static UPerfFunction* get(const UTrie2PerfTest &testcase) {
        return new GetBiDiClass(testcase);
    }
    virtual void call(UErrorCode* pErrorCode) {
        const UChar *buffer=testcase.getBuffer();
        int32_t length=testcase.getBufferLen();
        UChar32 c;
        int32_t i;
        uint32_t bitSet=0;
        for(i=0; i<length;) {
            U16_NEXT(buffer, i, length, c);
            bitSet|=(uint32_t)1<<u_charDirection(c);
        }
        if(length>0 && bitSet==0) {
            fprintf(stderr, "error: GetBiDiClass() did not collect bits\n");
        }
    }
};

UPerfFunction* UTrie2PerfTest::runIndexedTest(int32_t index, UBool exec, const char* &name, char* par) {
    switch (index) {
        case 0: name = "CheckFCD";              if (exec) return CheckFCD::get(*this); break;
        case 1: name = "ToNFC";                 if (exec) return ToNFC::get(*this); break;
        case 2: name = "GetBiDiClass";          if (exec) return GetBiDiClass::get(*this); break;
#if 0  // See comment at unorm_initUTrie2() forward declaration.
        case 3: name = "CheckFCDAlwaysGet";     if (exec) return CheckFCDAlwaysGet::get(*this); break;
        case 4: name = "CheckFCDUTF8";          if (exec) return CheckFCDUTF8::get(*this); break;
#endif
        default: name = ""; break;
    }
    return NULL;
}

int main(int argc, const char *argv[]) {
    UErrorCode status = U_ZERO_ERROR;
    UTrie2PerfTest test(argc, argv, status);

	if (U_FAILURE(status)){
        printf("The error is %s\n", u_errorName(status));
        test.usage();
        return status;
    }
        
    if (test.run() == FALSE){
        fprintf(stderr, "FAILED: Tests could not be run please check the "
			            "arguments.\n");
        return -1;
    }

    return 0;
}
