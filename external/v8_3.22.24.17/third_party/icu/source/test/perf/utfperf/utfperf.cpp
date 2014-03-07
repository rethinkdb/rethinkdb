/*  
 **********************************************************************
 *   Copyright (C) 2002-2007, International Business Machines
 *   Corporation and others.  All Rights Reserved.
 **********************************************************************
 *   file name:  utfperf.cpp
 *   encoding:   US-ASCII
 *   tab size:   8 (not used)
 *   indentation:4
 *
 *   created on: 2005Nov17
 *   created by: Raymond Yang
 *
 *   Ported from utfper.c created by Markus W. Scherer
 *   Performance test program for Unicode converters
 */

#include <stdio.h>
#include <stdlib.h>
#include "unicode/uperf.h"
#include "uoptions.h"

#define LENGTHOF(array) (int32_t)(sizeof(array)/sizeof((array)[0]))

/* definitions and text buffers */

#define INPUT_CAPACITY (1024*1024)
#define INTERMEDIATE_CAPACITY 4096
#define INTERMEDIATE_SMALL_CAPACITY 20
#define PIVOT_CAPACITY 1024
#define OUTPUT_CAPACITY INPUT_CAPACITY

static char utf8[INPUT_CAPACITY];
static UChar pivot[INTERMEDIATE_CAPACITY];

static UChar output[OUTPUT_CAPACITY];
static char intermediate[OUTPUT_CAPACITY];

static int32_t utf8Length, encodedLength, outputLength, countInputCodePoints;

static int32_t fromUCallbackCount;

// Command-line options specific to utfperf.
// Options do not have abbreviations: Force readable command lines.
// (Using U+0001 for abbreviation characters.)
enum {
    CHARSET,
    CHUNK_LENGTH,
    PIVOT_LENGTH,
    UTFPERF_OPTIONS_COUNT
};

static UOption options[UTFPERF_OPTIONS_COUNT]={
    UOPTION_DEF("charset",  '\x01', UOPT_REQUIRES_ARG),
    UOPTION_DEF("chunk",    '\x01', UOPT_REQUIRES_ARG),
    UOPTION_DEF("pivot",    '\x01', UOPT_REQUIRES_ARG)
};

static const char *const utfperf_usage =
    "\t--charset   Charset for which to test performance, e.g. windows-1251.\n"
    "\t            Default: UTF-8\n"
    "\t--chunk     Length (in bytes) of charset output chunks. [4096]\n"
    "\t--pivot     Length (in UChars) of the UTF-16 pivot buffer, if applicable.\n"
    "\t            [1024]\n";

// Test object.
class  UtfPerformanceTest : public UPerfTest{
public:
    UtfPerformanceTest(int32_t argc, const char *argv[], UErrorCode &status)
            : UPerfTest(argc, argv, options, LENGTHOF(options), utfperf_usage, status) {
        if (U_SUCCESS(status)) {
            charset = options[CHARSET].value;

            chunkLength = atoi(options[CHUNK_LENGTH].value);
            if (chunkLength < 1 || OUTPUT_CAPACITY < chunkLength) {
                fprintf(stderr, "error: chunk length must be 1..%ld\n", (long)OUTPUT_CAPACITY);
                status = U_ILLEGAL_ARGUMENT_ERROR;
            }

            pivotLength = atoi(options[PIVOT_LENGTH].value);
            if (pivotLength < 1 || PIVOT_CAPACITY < pivotLength) {
                fprintf(stderr, "error: pivot length must be 1..%ld\n", (long)PIVOT_CAPACITY);
                status = U_ILLEGAL_ARGUMENT_ERROR;
            }

            int32_t inputLength;
            UPerfTest::getBuffer(inputLength, status);
            countInputCodePoints = u_countChar32(buffer, bufferLen);
            u_strToUTF8(utf8, (int32_t)sizeof(utf8), &utf8Length, buffer, bufferLen, &status);
        }
    }

    virtual UPerfFunction* runIndexedTest(int32_t index, UBool exec, const char* &name, char* par = NULL);

    const UChar *getBuffer() const { return buffer; }
    int32_t getBufferLen() const { return bufferLen; }

    const char *charset;
    int32_t chunkLength, pivotLength;
};

U_CDECL_BEGIN
// Custom callback for counting callback calls.
static void U_CALLCONV
fromUCallback(const void *context,
              UConverterFromUnicodeArgs *fromUArgs,
              const UChar *codeUnits,
              int32_t length,
              UChar32 codePoint,
              UConverterCallbackReason reason,
              UErrorCode *pErrorCode) {
    if (reason <= UCNV_IRREGULAR) {
        ++fromUCallbackCount;
    }
    UCNV_FROM_U_CALLBACK_SUBSTITUTE(context, fromUArgs, codeUnits, length, codePoint, reason, pErrorCode);
}
U_CDECL_END

// Base class for Roundtrip, FromUnicode and FromUTF8 with common setup.
class Command : public UPerfFunction {
protected:
    Command(const UtfPerformanceTest &testcase)
            : testcase(testcase),
              input(testcase.getBuffer()), inputLength(testcase.getBufferLen()),
              errorCode(U_ZERO_ERROR) {
        cnv=ucnv_open(testcase.charset, &errorCode);
        if (U_FAILURE(errorCode)) {
            fprintf(stderr, "error opening converter for \"%s\" - %s\n", testcase.charset, u_errorName(errorCode));
        }
        ucnv_setFromUCallBack(cnv, fromUCallback, NULL, NULL, NULL, &errorCode);
    }
public:
    virtual ~Command(){
        if(U_SUCCESS(errorCode)) {
            ucnv_close(cnv);
        }
    }
    // virtual void call(UErrorCode* pErrorCode) { ... }
    virtual long getOperationsPerIteration(){
        return countInputCodePoints;
    }

    const UtfPerformanceTest &testcase;
    const UChar *input;
    int32_t inputLength;
    UErrorCode errorCode;
    UConverter *cnv;
};

// Test roundtrip UTF-16->encoding->UTF-16.
class Roundtrip : public Command {
protected:
    Roundtrip(const UtfPerformanceTest &testcase) : Command(testcase) {}
public:
    static UPerfFunction* get(const UtfPerformanceTest &testcase) {
        Roundtrip * t = new Roundtrip(testcase);
        if (U_SUCCESS(t->errorCode)){
            return t;
        } else {
            delete t;
            return NULL;
        }
    }
    virtual void call(UErrorCode* pErrorCode){
        const UChar *pIn, *pInLimit;
        UChar *pOut, *pOutLimit;
        char *pInter, *pInterLimit;
        const char *p;
        UBool flush;

        ucnv_reset(cnv);
        fromUCallbackCount=0;

        pIn=input;
        pInLimit=input+inputLength;

        pOut=output;
        pOutLimit=output+OUTPUT_CAPACITY;

        pInterLimit=intermediate+testcase.chunkLength;

        encodedLength=outputLength=0;
        flush=FALSE;

        do {
            /* convert a block of [pIn..pInLimit[ to the encoding in intermediate[] */
            pInter=intermediate;
            ucnv_fromUnicode(cnv, &pInter, pInterLimit, &pIn, pInLimit, NULL, TRUE, pErrorCode);
            encodedLength+=(int32_t)(pInter-intermediate);

            if(*pErrorCode==U_BUFFER_OVERFLOW_ERROR) {
                /* make sure that we convert once more to really flush */
                *pErrorCode=U_ZERO_ERROR;
            } else if(U_FAILURE(*pErrorCode)) {
                return;
            } else if(pIn==pInLimit) {
                flush=TRUE;
            }

            /* convert the block [intermediate..pInter[ back to UTF-16 */
            p=intermediate;
            ucnv_toUnicode(cnv, &pOut, pOutLimit,&p, pInter,NULL, flush,pErrorCode);
            if(U_FAILURE(*pErrorCode)) {
                return;
            }
            /* intermediate must have been consumed (p==pInter) because of the converter semantics */
        } while(!flush);

        outputLength=pOut-output;
        if(inputLength!=outputLength) {
            fprintf(stderr, "error: roundtrip failed, inputLength %d!=outputLength %d\n", inputLength, outputLength);
            *pErrorCode=U_INTERNAL_PROGRAM_ERROR;
        }
    }
};

// Test one-way conversion UTF-16->encoding.
class FromUnicode : public Command {
protected:
    FromUnicode(const UtfPerformanceTest &testcase) : Command(testcase) {}
public:
    static UPerfFunction* get(const UtfPerformanceTest &testcase) {
        FromUnicode * t = new FromUnicode(testcase);
        if (U_SUCCESS(t->errorCode)){
            return t;
        } else {
            delete t;
            return NULL;
        }
    }
    virtual void call(UErrorCode* pErrorCode){
        const UChar *pIn, *pInLimit;
        char *pInter, *pInterLimit;

        ucnv_resetFromUnicode(cnv);
        fromUCallbackCount=0;

        pIn=input;
        pInLimit=input+inputLength;

        pInterLimit=intermediate+testcase.chunkLength;

        encodedLength=0;

        for(;;) {
            pInter=intermediate;
            ucnv_fromUnicode(cnv, &pInter, pInterLimit, &pIn, pInLimit, NULL, TRUE, pErrorCode);
            encodedLength+=(int32_t)(pInter-intermediate);

            if(*pErrorCode==U_BUFFER_OVERFLOW_ERROR) {
                /* make sure that we convert once more to really flush */
                *pErrorCode=U_ZERO_ERROR;
            } else if(U_FAILURE(*pErrorCode)) {
                return;
            } else {
                break;  // all done
            }
        }
    }
};

// Test one-way conversion UTF-8->encoding.
class FromUTF8 : public Command {
protected:
    FromUTF8(const UtfPerformanceTest &testcase)
            : Command(testcase),
              utf8Cnv(NULL),
              input8(utf8), input8Length(utf8Length) {
        utf8Cnv=ucnv_open("UTF-8", &errorCode);
    }
public:
    static UPerfFunction* get(const UtfPerformanceTest &testcase) {
        FromUTF8 * t = new FromUTF8(testcase);
        if (U_SUCCESS(t->errorCode)){
            return t;
        } else {
            delete t;
            return NULL;
        }
    }
    ~FromUTF8() {
        ucnv_close(utf8Cnv);
    }
    virtual void call(UErrorCode* pErrorCode){
        const char *pIn, *pInLimit;
        char *pInter, *pInterLimit;
        UChar *pivotSource, *pivotTarget, *pivotLimit;

        ucnv_resetToUnicode(utf8Cnv);
        ucnv_resetFromUnicode(cnv);
        fromUCallbackCount=0;

        pIn=input8;
        pInLimit=input8+input8Length;

        pInterLimit=intermediate+testcase.chunkLength;

        pivotSource=pivotTarget=pivot;
        pivotLimit=pivot+testcase.pivotLength;

        encodedLength=0;

        for(;;) {
            pInter=intermediate;
            ucnv_convertEx(cnv, utf8Cnv,
                           &pInter, pInterLimit,
                           &pIn, pInLimit,
                           pivot, &pivotSource, &pivotTarget, pivotLimit,
                           FALSE, TRUE, pErrorCode);
            encodedLength+=(int32_t)(pInter-intermediate);

            if(*pErrorCode==U_BUFFER_OVERFLOW_ERROR) {
                /* make sure that we convert once more to really flush */
                *pErrorCode=U_ZERO_ERROR;
            } else if(U_FAILURE(*pErrorCode)) {
                return;
            } else {
                break;  // all done
            }
        }
    }
protected:
    UConverter *utf8Cnv;
    const char *input8;
    int32_t input8Length;
};

UPerfFunction* UtfPerformanceTest::runIndexedTest(int32_t index, UBool exec, const char* &name, char* par) {
    switch (index) {
        case 0: name = "Roundtrip";     if (exec) return Roundtrip::get(*this); break;
        case 1: name = "FromUnicode";   if (exec) return FromUnicode::get(*this); break;
        case 2: name = "FromUTF8";      if (exec) return FromUTF8::get(*this); break;
        default: name = ""; break;
    }
    return NULL;
}

int main(int argc, const char *argv[])
{
    // Default values for command-line options.
    options[CHARSET].value = "UTF-8";
    options[CHUNK_LENGTH].value = "4096";
    options[PIVOT_LENGTH].value = "1024";

    UErrorCode status = U_ZERO_ERROR;
    UtfPerformanceTest test(argc, argv, status);

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

    if (fromUCallbackCount > 0) {
        printf("Number of fromUnicode callback calls in the last iteration: %ld\n", (long)fromUCallbackCount);
    }

    return 0;
}
