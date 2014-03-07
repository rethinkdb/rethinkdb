/*
************************************************************************
* Copyright (c) 1997-2010, International Business Machines
* Corporation and others.  All Rights Reserved.
************************************************************************
*/

#include "unicode/utypes.h"

#if !UCONFIG_NO_NORMALIZATION

#include "unicode/uchar.h"
#include "unicode/normlzr.h"
#include "unicode/uniset.h"
#include "unicode/putil.h"
#include "cstring.h"
#include "filestrm.h"
#include "normconf.h"
#include <stdio.h>

#define ARRAY_LENGTH(array) (sizeof(array) / sizeof(array[0]))

#define CASE(id,test,exec) case id:                          \
                          name = #test;                 \
                          if (exec) {                   \
                              logln(#test "---");       \
                              logln((UnicodeString)""); \
                              test();                   \
                          }                             \
                          break

void NormalizerConformanceTest::runIndexedTest(int32_t index, UBool exec, const char* &name, char* /*par*/) {
    switch (index) {
        CASE(0, TestConformance, exec);
#if !UCONFIG_NO_FILE_IO && !UCONFIG_NO_LEGACY_CONVERSION
        CASE(1, TestConformance32, exec);
#endif
        // CASE(2, TestCase6);
        default: name = ""; break;
    }
}

#define FIELD_COUNT 5

NormalizerConformanceTest::NormalizerConformanceTest() :
    normalizer(UnicodeString(), UNORM_NFC) {}

NormalizerConformanceTest::~NormalizerConformanceTest() {}

// more interesting conformance test cases, not in the unicode.org NormalizationTest.txt
static const char *moreCases[]={
    // Markus 2001aug30
    "0061 0332 0308;00E4 0332;0061 0332 0308;00E4 0332;0061 0332 0308; # Markus 0",

    // Markus 2001oct26 - test edge case for iteration: U+0f73.cc==0 but decomposition.lead.cc==129
    "0061 0301 0F73;00E1 0F71 0F72;0061 0F71 0F72 0301;00E1 0F71 0F72;0061 0F71 0F72 0301; # Markus 1"
};

void NormalizerConformanceTest::compare(const UnicodeString& s1, const UnicodeString& s2){
    UErrorCode status=U_ZERO_ERROR;
     // TODO: Re-enable this tests after UTC fixes UAX 21
    if(s1.indexOf((UChar32)0x0345)>=0)return;
    if(Normalizer::compare(s1,s2,U_FOLD_CASE_DEFAULT,status)!=0){
        errln("Normalizer::compare() failed for s1: " + prettify(s1) + " s2: " +prettify(s2));
    }
}

FileStream *
NormalizerConformanceTest::openNormalizationTestFile(const char *filename) {
    char unidataPath[2000];
    const char *folder;
    FileStream *input;
    UErrorCode errorCode;

    // look inside ICU_DATA first
    folder=pathToDataDirectory();
    if(folder!=NULL) {
        strcpy(unidataPath, folder);
        strcat(unidataPath, "unidata" U_FILE_SEP_STRING);
        strcat(unidataPath, filename);
        input=T_FileStream_open(unidataPath, "rb");
        if(input!=NULL) {
            return input;
        }
    }

    // find icu/source/data/unidata relative to the test data
    errorCode=U_ZERO_ERROR;
    folder=loadTestData(errorCode);
    if(U_SUCCESS(errorCode)) {
        strcpy(unidataPath, folder);
        strcat(unidataPath, U_FILE_SEP_STRING ".." U_FILE_SEP_STRING ".."
                     U_FILE_SEP_STRING ".." U_FILE_SEP_STRING ".."
                     U_FILE_SEP_STRING "data" U_FILE_SEP_STRING "unidata" U_FILE_SEP_STRING);
        strcat(unidataPath, filename);
        input=T_FileStream_open(unidataPath, "rb");
        if(input!=NULL) {
            return input;
        }
    }

    // look in icu/source/test/testdata/out/build
    errorCode=U_ZERO_ERROR;
    folder=loadTestData(errorCode);
    if(U_SUCCESS(errorCode)) {
        strcpy(unidataPath, folder);
        strcat(unidataPath, U_FILE_SEP_STRING);
        strcat(unidataPath, filename);
        input=T_FileStream_open(unidataPath, "rb");
        if(input!=NULL) {
            return input;
        }
    }

    // look in icu/source/test/testdata
    errorCode=U_ZERO_ERROR;
    folder=loadTestData(errorCode);
    if(U_SUCCESS(errorCode)) {
        strcpy(unidataPath, folder);
        strcat(unidataPath, U_FILE_SEP_STRING ".." U_FILE_SEP_STRING ".." U_FILE_SEP_STRING);
        strcat(unidataPath, filename);
        input=T_FileStream_open(unidataPath, "rb");
        if(input!=NULL) {
            return input;
        }
    }

    // find icu/source/data/unidata relative to U_TOPSRCDIR
#if defined(U_TOPSRCDIR)
    strcpy(unidataPath, U_TOPSRCDIR U_FILE_SEP_STRING "data" U_FILE_SEP_STRING "unidata" U_FILE_SEP_STRING);
    strcat(unidataPath, filename);
    input=T_FileStream_open(unidataPath, "rb");
    if(input!=NULL) {
        return input;
    }

    strcpy(unidataPath, U_TOPSRCDIR U_FILE_SEP_STRING "test" U_FILE_SEP_STRING "testdata" U_FILE_SEP_STRING);
    strcat(unidataPath, filename);
    input=T_FileStream_open(unidataPath, "rb");
    if(input!=NULL) {
        return input;
    }
#endif

    dataerrln("Failed to open %s", filename);
    return NULL;
}

/**
 * Test the conformance of Normalizer to
 * http://www.unicode.org/Public/UNIDATA/NormalizationTest.txt
 */
void NormalizerConformanceTest::TestConformance() {
    TestConformance(openNormalizationTestFile("NormalizationTest.txt"), 0);
}

void NormalizerConformanceTest::TestConformance32() {
    TestConformance(openNormalizationTestFile("NormalizationTest-3.2.0.txt"), UNORM_UNICODE_3_2);
}

void NormalizerConformanceTest::TestConformance(FileStream *input, int32_t options) {
    enum { BUF_SIZE = 1024 };
    char lineBuf[BUF_SIZE];
    UnicodeString fields[FIELD_COUNT];
    UErrorCode status = U_ZERO_ERROR;
    int32_t passCount = 0;
    int32_t failCount = 0;
    UChar32 c;

    if(input==NULL) {
        return;
    }

    // UnicodeSet for all code points that are not mentioned in NormalizationTest.txt
    UnicodeSet other(0, 0x10ffff);

    int32_t count, countMoreCases = sizeof(moreCases)/sizeof(moreCases[0]);
    for (count = 1;;++count) {
        if (!T_FileStream_eof(input)) {
            T_FileStream_readLine(input, lineBuf, (int32_t)sizeof(lineBuf));
        } else {
            // once NormalizationTest.txt is finished, use moreCases[]
            if(count > countMoreCases) {
                count = 0;
            } else if(count == countMoreCases) {
                // all done
                break;
            }
            uprv_strcpy(lineBuf, moreCases[count]);
        }
        if (lineBuf[0] == 0 || lineBuf[0] == '\n' || lineBuf[0] == '\r') continue;

        // Expect 5 columns of this format:
        // 1E0C;1E0C;0044 0323;1E0C;0044 0323; # <comments>

        // Parse out the comment.
        if (lineBuf[0] == '#') continue;

        // Read separator lines starting with '@'
        if (lineBuf[0] == '@') {
            logln(lineBuf);
            continue;
        }

        // Parse out the fields
        if (!hexsplit(lineBuf, ';', fields, FIELD_COUNT)) {
            errln((UnicodeString)"Unable to parse line " + count);
            break; // Syntax error
        }

        // Remove a single code point from the "other" UnicodeSet
        if(fields[0].length()==fields[0].moveIndex32(0, 1)) {
            c=fields[0].char32At(0);
            if(0xac20<=c && c<=0xd73f && quick) {
                // not an exhaustive test run: skip most Hangul syllables
                if(c==0xac20) {
                    other.remove(0xac20, 0xd73f);
                }
                continue;
            }
            other.remove(c);
        }

        if (checkConformance(fields, lineBuf, options, status)) {
            ++passCount;
        } else {
            ++failCount;
            if(status == U_FILE_ACCESS_ERROR) {
              dataerrln("Something is wrong with the normalizer, skipping the rest of the test.");
              break;
            }
        }
        if ((count % 1000) == 0) {
            logln("Line %d", count);
        }
    }

    T_FileStream_close(input);

    /*
     * Test that all characters that are not mentioned
     * as single code points in column 1
     * do not change under any normalization.
     */

    // remove U+ffff because that is the end-of-iteration sentinel value
    other.remove(0xffff);

    for(c=0; c<=0x10ffff; quick ? c+=113 : ++c) {
        if(0x30000<=c && c<0xe0000) {
            c=0xe0000;
        }
        if(!other.contains(c)) {
            continue;
        }

        fields[0]=fields[1]=fields[2]=fields[3]=fields[4].setTo(c);
        sprintf(lineBuf, "not mentioned code point U+%04lx", (long)c);

        if (checkConformance(fields, lineBuf, options, status)) {
            ++passCount;
        } else {
            ++failCount;
            if(status == U_FILE_ACCESS_ERROR) {
              dataerrln("Something is wrong with the normalizer, skipping the rest of the test.: %s", u_errorName(status));
              break;
            }
        }
        if ((c % 0x1000) == 0) {
            logln("Code point U+%04lx", c);
        }
    }

    if (failCount != 0) {
        dataerrln((UnicodeString)"Total: " + failCount + " lines/code points failed, " +
              passCount + " lines/code points passed");
    } else {
        logln((UnicodeString)"Total: " + passCount + " lines/code points passed");
    }
}

/**
 * Verify the conformance of the given line of the Unicode
 * normalization (UTR 15) test suite file.  For each line,
 * there are five columns, corresponding to field[0]..field[4].
 *
 * The following invariants must be true for all conformant implementations
 *  c2 == NFC(c1) == NFC(c2) == NFC(c3)
 *  c3 == NFD(c1) == NFD(c2) == NFD(c3)
 *  c4 == NFKC(c1) == NFKC(c2) == NFKC(c3) == NFKC(c4) == NFKC(c5)
 *  c5 == NFKD(c1) == NFKD(c2) == NFKD(c3) == NFKD(c4) == NFKD(c5)
 *
 * @param field the 5 columns
 * @param line the source line from the test suite file
 * @return true if the test passes
 */
UBool NormalizerConformanceTest::checkConformance(const UnicodeString* field,
                                                  const char *line,
                                                  int32_t options,
                                                  UErrorCode &status) {
    UBool pass = TRUE, result;
    //UErrorCode status = U_ZERO_ERROR;
    UnicodeString out, fcd;
    int32_t fieldNum;

    for (int32_t i=0; i<FIELD_COUNT; ++i) {
        fieldNum = i+1;
        if (i<3) {
            Normalizer::normalize(field[i], UNORM_NFC, options, out, status);
            if (U_FAILURE(status)) {
                dataerrln("Error running normalize UNORM_NFC: %s", u_errorName(status));
            } else {
                pass &= assertEqual("C", field[i], out, field[1], "c2!=C(c", fieldNum);
                iterativeNorm(field[i], UNORM_NFC, options, out, +1);
                pass &= assertEqual("C(+1)", field[i], out, field[1], "c2!=C(c", fieldNum);
                iterativeNorm(field[i], UNORM_NFC, options, out, -1);
                pass &= assertEqual("C(-1)", field[i], out, field[1], "c2!=C(c", fieldNum);
            }

            Normalizer::normalize(field[i], UNORM_NFD, options, out, status);
            if (U_FAILURE(status)) {
                dataerrln("Error running normalize UNORM_NFD: %s", u_errorName(status));
            } else {
                pass &= assertEqual("D", field[i], out, field[2], "c3!=D(c", fieldNum);
                iterativeNorm(field[i], UNORM_NFD, options, out, +1);
                pass &= assertEqual("D(+1)", field[i], out, field[2], "c3!=D(c", fieldNum);
                iterativeNorm(field[i], UNORM_NFD, options, out, -1);
                pass &= assertEqual("D(-1)", field[i], out, field[2], "c3!=D(c", fieldNum);
            }
        }
        Normalizer::normalize(field[i], UNORM_NFKC, options, out, status);
        if (U_FAILURE(status)) {
            dataerrln("Error running normalize UNORM_NFKC: %s", u_errorName(status));
        } else {
            pass &= assertEqual("KC", field[i], out, field[3], "c4!=KC(c", fieldNum);
            iterativeNorm(field[i], UNORM_NFKC, options, out, +1);
            pass &= assertEqual("KC(+1)", field[i], out, field[3], "c4!=KC(c", fieldNum);
            iterativeNorm(field[i], UNORM_NFKC, options, out, -1);
            pass &= assertEqual("KC(-1)", field[i], out, field[3], "c4!=KC(c", fieldNum);
        }

        Normalizer::normalize(field[i], UNORM_NFKD, options, out, status);
        if (U_FAILURE(status)) {
            dataerrln("Error running normalize UNORM_NFKD: %s", u_errorName(status));
        } else {
            pass &= assertEqual("KD", field[i], out, field[4], "c5!=KD(c", fieldNum);
            iterativeNorm(field[i], UNORM_NFKD, options, out, +1);
            pass &= assertEqual("KD(+1)", field[i], out, field[4], "c5!=KD(c", fieldNum);
            iterativeNorm(field[i], UNORM_NFKD, options, out, -1);
            pass &= assertEqual("KD(-1)", field[i], out, field[4], "c5!=KD(c", fieldNum);
        }
    }
    compare(field[1],field[2]);
    compare(field[0],field[1]);
    // test quick checks
    if(UNORM_NO == Normalizer::quickCheck(field[1], UNORM_NFC, options, status)) {
        errln("Normalizer error: quickCheck(NFC(s), UNORM_NFC) is UNORM_NO");
        pass = FALSE;
    }
    if(UNORM_NO == Normalizer::quickCheck(field[2], UNORM_NFD, options, status)) {
        errln("Normalizer error: quickCheck(NFD(s), UNORM_NFD) is UNORM_NO");
        pass = FALSE;
    }
    if(UNORM_NO == Normalizer::quickCheck(field[3], UNORM_NFKC, options, status)) {
        errln("Normalizer error: quickCheck(NFKC(s), UNORM_NFKC) is UNORM_NO");
        pass = FALSE;
    }
    if(UNORM_NO == Normalizer::quickCheck(field[4], UNORM_NFKD, options, status)) {
        errln("Normalizer error: quickCheck(NFKD(s), UNORM_NFKD) is UNORM_NO");
        pass = FALSE;
    }

    // branch on options==0 for better code coverage
    if(options==0) {
        result = Normalizer::isNormalized(field[1], UNORM_NFC, status);
    } else {
        result = Normalizer::isNormalized(field[1], UNORM_NFC, options, status);
    }
    if(!result) {
        dataerrln("Normalizer error: isNormalized(NFC(s), UNORM_NFC) is FALSE");
        pass = FALSE;
    }
    if(field[0]!=field[1] && Normalizer::isNormalized(field[0], UNORM_NFC, options, status)) {
        errln("Normalizer error: isNormalized(s, UNORM_NFC) is TRUE");
        pass = FALSE;
    }
    if(!Normalizer::isNormalized(field[3], UNORM_NFKC, options, status)) {
        dataerrln("Normalizer error: isNormalized(NFKC(s), UNORM_NFKC) is FALSE");
        pass = FALSE;
    }
    if(field[0]!=field[3] && Normalizer::isNormalized(field[0], UNORM_NFKC, options, status)) {
        errln("Normalizer error: isNormalized(s, UNORM_NFKC) is TRUE");
        pass = FALSE;
    }

    // test FCD quick check and "makeFCD"
    Normalizer::normalize(field[0], UNORM_FCD, options, fcd, status);
    if(UNORM_NO == Normalizer::quickCheck(fcd, UNORM_FCD, options, status)) {
        errln("Normalizer error: quickCheck(FCD(s), UNORM_FCD) is UNORM_NO");
        pass = FALSE;
    }
    if(UNORM_NO == Normalizer::quickCheck(field[2], UNORM_FCD, options, status)) {
        errln("Normalizer error: quickCheck(NFD(s), UNORM_FCD) is UNORM_NO");
        pass = FALSE;
    }
    if(UNORM_NO == Normalizer::quickCheck(field[4], UNORM_FCD, options, status)) {
        errln("Normalizer error: quickCheck(NFKD(s), UNORM_FCD) is UNORM_NO");
        pass = FALSE;
    }

    Normalizer::normalize(fcd, UNORM_NFD, options, out, status);
    if(out != field[2]) {
        dataerrln("Normalizer error: NFD(FCD(s))!=NFD(s)");
        pass = FALSE;
    }

    if (U_FAILURE(status)) {
        dataerrln("Normalizer::normalize returned error status: %s", u_errorName(status));
        pass = FALSE;
    }

    if(field[0]!=field[2]) {
        // two strings that are canonically equivalent must test
        // equal under a canonical caseless match
        // see UAX #21 Case Mappings and Jitterbug 2021 and
        // Unicode Technical Committee meeting consensus 92-C31
        int32_t rc;

        status=U_ZERO_ERROR;
        rc=Normalizer::compare(field[0], field[2], (options<<UNORM_COMPARE_NORM_OPTIONS_SHIFT)|U_COMPARE_IGNORE_CASE, status);
        if(U_FAILURE(status)) {
            dataerrln("Normalizer::compare(case-insensitive) sets %s", u_errorName(status));
            pass=FALSE;
        } else if(rc!=0) {
            errln("Normalizer::compare(original, NFD, case-insensitive) returned %d instead of 0 for equal", rc);
            pass=FALSE;
        }
    }

    if (!pass) {
        dataerrln("FAIL: %s", line);
    }
    return pass;
}

/**
 * Do a normalization using the iterative API in the given direction.
 * @param dir either +1 or -1
 */
void NormalizerConformanceTest::iterativeNorm(const UnicodeString& str,
                                              UNormalizationMode mode, int32_t options,
                                              UnicodeString& result,
                                              int8_t dir) {
    UErrorCode status = U_ZERO_ERROR;
    normalizer.setText(str, status);
    normalizer.setMode(mode);
    normalizer.setOption(-1, 0);        // reset all options
    normalizer.setOption(options, 1);   // set desired options
    result.truncate(0);
    if (U_FAILURE(status)) {
        return;
    }
    UChar32 ch;
    if (dir > 0) {
        for (ch = normalizer.first(); ch != Normalizer::DONE;
             ch = normalizer.next()) {
            result.append(ch);
        }
    } else {
        for (ch = normalizer.last(); ch != Normalizer::DONE;
             ch = normalizer.previous()) {
            result.insert(0, ch);
        }
    }
}

/**
 * @param op name of normalization form, e.g., "KC"
 * @param s string being normalized
 * @param got value received
 * @param exp expected value
 * @param msg description of this test
 * @param return true if got == exp
 */
UBool NormalizerConformanceTest::assertEqual(const char *op,
                                             const UnicodeString& s,
                                             const UnicodeString& got,
                                             const UnicodeString& exp,
                                             const char *msg,
                                             int32_t field)
{
    if (exp == got)
        return TRUE;

    char *sChars, *gotChars, *expChars;
    UnicodeString sPretty(prettify(s));
    UnicodeString gotPretty(prettify(got));
    UnicodeString expPretty(prettify(exp));

    sChars = new char[sPretty.length() + 1];
    gotChars = new char[gotPretty.length() + 1];
    expChars = new char[expPretty.length() + 1];

    sPretty.extract(0, sPretty.length(), sChars, sPretty.length() + 1);
    sChars[sPretty.length()] = 0;
    gotPretty.extract(0, gotPretty.length(), gotChars, gotPretty.length() + 1);
    gotChars[gotPretty.length()] = 0;
    expPretty.extract(0, expPretty.length(), expChars, expPretty.length() + 1);
    expChars[expPretty.length()] = 0;

    errln("    %s%d)%s(%s)=%s, exp. %s", msg, field, op, sChars, gotChars, expChars);

    delete []sChars;
    delete []gotChars;
    delete []expChars;
    return FALSE;
}

/**
 * Split a string into pieces based on the given delimiter
 * character.  Then, parse the resultant fields from hex into
 * characters.  That is, "0040 0400;0C00;0899" -> new String[] {
 * "\u0040\u0400", "\u0C00", "\u0899" }.  The output is assumed to
 * be of the proper length already, and exactly output.length
 * fields are parsed.  If there are too few an exception is
 * thrown.  If there are too many the extras are ignored.
 *
 * @return FALSE upon failure
 */
UBool NormalizerConformanceTest::hexsplit(const char *s, char delimiter,
                                          UnicodeString output[], int32_t outputLength) {
    const char *t = s;
    char *end = NULL;
    UChar32 c;
    int32_t i;
    for (i=0; i<outputLength; ++i) {
        // skip whitespace
        while(*t == ' ' || *t == '\t') {
            ++t;
        }

        // read a sequence of code points
        output[i].remove();
        for(;;) {
            c = (UChar32)uprv_strtoul(t, &end, 16);

            if( (char *)t == end ||
                (uint32_t)c > 0x10ffff ||
                (*end != ' ' && *end != '\t' && *end != delimiter)
            ) {
                errln(UnicodeString("Bad field ", "") + (i + 1) + " in " + UnicodeString(s, ""));
                return FALSE;
            }

            output[i].append(c);

            t = (const char *)end;

            // skip whitespace
            while(*t == ' ' || *t == '\t') {
                ++t;
            }

            if(*t == delimiter) {
                ++t;
                break;
            }
            if(*t == 0) {
                if((i + 1) == outputLength) {
                    return TRUE;
                } else {
                    errln(UnicodeString("Missing field(s) in ", "") + s + " only " + (i + 1) + " out of " + outputLength);
                    return FALSE;
                }
            }
        }
    }
    return TRUE;
}

// Specific tests for debugging.  These are generally failures taken from
// the conformance file, but culled out to make debugging easier.

void NormalizerConformanceTest::TestCase6(void) {
    _testOneLine("0385;0385;00A8 0301;0020 0308 0301;0020 0308 0301;");
}

void NormalizerConformanceTest::_testOneLine(const char *line) {
  UErrorCode status = U_ZERO_ERROR;
    UnicodeString fields[FIELD_COUNT];
    if (!hexsplit(line, ';', fields, FIELD_COUNT)) {
        errln((UnicodeString)"Unable to parse line " + line);
    } else {
        checkConformance(fields, line, 0, status);
    }
}

#endif /* #if !UCONFIG_NO_NORMALIZATION */
