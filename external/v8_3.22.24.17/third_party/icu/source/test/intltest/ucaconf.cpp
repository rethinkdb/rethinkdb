/********************************************************************
 * COPYRIGHT: 
 * Copyright (c) 2002-2010, International Business Machines Corporation and
 * others. All Rights Reserved.
 ********************************************************************/

/**
 * UCAConformanceTest performs conformance tests defined in the data
 * files. ICU ships with stub data files, as the whole test are too 
 * long. To do the whole test, download the test files.
 */

#include "unicode/utypes.h"

#if !UCONFIG_NO_COLLATION

#include "ucaconf.h"
#include "unicode/ustring.h"
#include "cstring.h"
#include "uparse.h"

UCAConformanceTest::UCAConformanceTest() :
rbUCA(NULL),
testFile(NULL),
status(U_ZERO_ERROR)
{
    UCA = ucol_open("root", &status);
    if(U_FAILURE(status)) {
        errln("ERROR - UCAConformanceTest: Unable to open UCA collator!");
    }

    const char *srcDir = IntlTest::getSourceTestData(status);
    if (U_FAILURE(status)) {
        dataerrln("Could not open test data %s", u_errorName(status));
        return;
    }
    uprv_strcpy(testDataPath, srcDir);
    uprv_strcat(testDataPath, "CollationTest_");
}

UCAConformanceTest::~UCAConformanceTest()
{
    ucol_close(UCA);
    if(rbUCA) {
        ucol_close(rbUCA);
    }
    if(testFile) {
        fclose(testFile);
    }
}

void UCAConformanceTest::runIndexedTest( int32_t index, UBool exec, const char* &name, char* /*par */)
{
    if (exec) logln("TestSuite UCAConformanceTest: ");
    if(U_SUCCESS(status)) {
      switch (index) {
          case 0: name = "TestTableNonIgnorable"; if (exec)   TestTableNonIgnorable(/* par */); break;
          case 1: name = "TestTableShifted";      if (exec)   TestTableShifted(/* par */);      break;
          case 2: name = "TestRulesNonIgnorable"; if (exec)   TestRulesNonIgnorable(/* par */); break;
          case 3: name = "TestRulesShifted";      if (exec)   TestRulesShifted(/* par */);      break;
          default: name = ""; break;
      }
    } else {
      name = "";
    }
}

void UCAConformanceTest::initRbUCA() 
{
    if(!rbUCA) {
        UParseError parseError;
        UChar      *ucarules;
        // preflight rules
        int32_t size = ucol_getRulesEx(UCA, UCOL_FULL_RULES, NULL, 0);
        ucarules = (UChar *)malloc(size * sizeof(UChar));

        size = ucol_getRulesEx(UCA, UCOL_FULL_RULES, ucarules, size);
        rbUCA = ucol_openRules(ucarules, size, UCOL_DEFAULT, UCOL_TERTIARY, 
            &parseError, &status);
        free(ucarules);
        if (U_FAILURE(status)) {
            errln("Failure creating UCA rule-based collator: %s", u_errorName(status));
            return;
        }
    }
}

void UCAConformanceTest::setCollNonIgnorable(UCollator *coll) 
{
  ucol_setAttribute(coll, UCOL_NORMALIZATION_MODE, UCOL_ON, &status);
  ucol_setAttribute(coll, UCOL_CASE_FIRST, UCOL_OFF, &status);
  ucol_setAttribute(coll, UCOL_CASE_LEVEL, UCOL_OFF, &status);
  ucol_setAttribute(coll, UCOL_STRENGTH, UCOL_TERTIARY, &status);
  ucol_setAttribute(coll, UCOL_ALTERNATE_HANDLING, UCOL_NON_IGNORABLE, &status);
}

void UCAConformanceTest::setCollShifted(UCollator *coll) 
{
    ucol_setAttribute(coll, UCOL_NORMALIZATION_MODE, UCOL_ON, &status);
    ucol_setAttribute(coll, UCOL_CASE_FIRST, UCOL_OFF, &status);
    ucol_setAttribute(coll, UCOL_CASE_LEVEL, UCOL_OFF, &status);
    ucol_setAttribute(coll, UCOL_STRENGTH, UCOL_QUATERNARY, &status);
    ucol_setAttribute(coll, UCOL_ALTERNATE_HANDLING, UCOL_SHIFTED, &status);
}

void UCAConformanceTest::openTestFile(const char *type)
{
    const char *ext = ".txt";
    if(testFile) {
        fclose(testFile);
    }
    char buffer[1024];
    uprv_strcpy(buffer, testDataPath);
    uprv_strcat(buffer, type);
    int32_t bufLen = (int32_t)uprv_strlen(buffer);

    // we try to open 3 files:
    // path/CollationTest_type.txt
    // path/CollationTest_type_SHORT.txt
    // path/CollationTest_type_STUB.txt
    // we are going to test with the first one that we manage to open.

    uprv_strcpy(buffer+bufLen, ext);

    testFile = fopen(buffer, "rb");

    if(testFile == 0) {
        uprv_strcpy(buffer+bufLen, "_SHORT");
        uprv_strcat(buffer, ext);
        testFile = fopen(buffer, "rb");

        if(testFile == 0) {
            uprv_strcpy(buffer+bufLen, "_STUB");
            uprv_strcat(buffer, ext);
            testFile = fopen(buffer, "rb");

            if (testFile == 0) {
                *(buffer+bufLen) = 0;
                dataerrln("Could not open any of the conformance test files, tried opening base %s\n", buffer);
                return;        
            } else {
                infoln(
                    "INFO: Working with the stub file.\n"
                    "If you need the full conformance test, please\n"
                    "download the appropriate data files from:\n"
                    "http://source.icu-project.org/repos/icu/tools/trunk/unicodetools/com/ibm/text/data/");
            }
        }
    }
}

void UCAConformanceTest::testConformance(UCollator *coll) 
{
    if(testFile == 0) {
        return;
    }

    int32_t line = 0;

    UChar b1[1024], b2[1024];
    UChar *buffer = b1, *oldB = NULL;

    char lineB1[1024], lineB2[1024];
    char *lineB = lineB1, *oldLineB = lineB2;

    uint8_t sk1[1024], sk2[1024];
    uint8_t *oldSk = NULL, *newSk = sk1;

    int32_t resLen = 0, oldLen = 0;
    int32_t buflen = 0, oldBlen = 0;
    uint32_t first = 0;
    uint32_t offset = 0;
    UnicodeString oldS, newS;


    while (fgets(lineB, 1024, testFile) != NULL) {
        // remove trailing whitespace
        u_rtrim(lineB);
        offset = 0;

        line++;
        if(*lineB == 0 || strlen(lineB) < 3 || lineB[0] == '#') {
            continue;
        }
        offset = u_parseString(lineB, buffer, 1024, &first, &status);
        if(U_FAILURE(status)) {
            errln("Error parsing line %ld (%s): %s\n",
                  (long)line, u_errorName(status), lineB);
            status = U_ZERO_ERROR;
        }
        buflen = offset;
        buffer[offset++] = 0;

        resLen = ucol_getSortKey(coll, buffer, buflen, newSk, 1024);

        int32_t res = 0, cmpres = 0, cmpres2 = 0;

        if(oldSk != NULL) {
            res = strcmp((char *)oldSk, (char *)newSk);
            cmpres = ucol_strcoll(coll, oldB, oldBlen, buffer, buflen);
            cmpres2 = ucol_strcoll(coll, buffer, buflen, oldB, oldBlen);

            if(cmpres != -cmpres2) {
                errln("Compare result not symmetrical on line %i", line);
            }

            if(((res&0x80000000) != (cmpres&0x80000000)) || (res == 0 && cmpres != 0) || (res != 0 && cmpres == 0)) {
                errln("Difference between ucol_strcoll and sortkey compare on line %i", line);
                errln("  Previous data line %s", oldLineB);
                errln("  Current data line  %s", lineB);
            }

            if(res > 0) {
                errln("Line %i is not greater or equal than previous line", line);
                errln("  Previous data line %s", oldLineB);
                errln("  Current data line  %s", lineB);
                prettify(CollationKey(oldSk, oldLen), oldS);
                prettify(CollationKey(newSk, resLen), newS);
                errln("  Previous key: "+oldS);
                errln("  Current key:  "+newS);
            } else if(res == 0) { /* equal */
                res = u_strcmpCodePointOrder(oldB, buffer);
                if (res == 0) {
                    errln("Probable error in test file on line %i (comparing identical strings)", line);
                    errln("  Data line %s", lineB);
                }
                /*
                 * UCA 6.0 test files can have lines that compare == if they are
                 * different strings but canonically equivalent.
                else if (res > 0) {
                    errln("Sortkeys are identical, but code point compare gives >0 on line %i", line);
                    errln("  Previous data line %s", oldLineB);
                    errln("  Current data line  %s", lineB);
                }
                 */
            }
        }

        // swap buffers
        oldLineB = lineB;
        oldB = buffer;
        oldSk = newSk;
        if(lineB == lineB1) {
            lineB = lineB2;
            buffer = b2;
            newSk = sk2;
        } else {
            lineB = lineB1;
            buffer = b1;
            newSk = sk1;
        }
        oldLen = resLen;
        oldBlen = buflen;
    }
}

void UCAConformanceTest::TestTableNonIgnorable(/* par */) {
    setCollNonIgnorable(UCA);
    openTestFile("NON_IGNORABLE");
    testConformance(UCA);
}

void UCAConformanceTest::TestTableShifted(/* par */) {
    setCollShifted(UCA);
    openTestFile("SHIFTED");
    testConformance(UCA);
}

void UCAConformanceTest::TestRulesNonIgnorable(/* par */) {
    initRbUCA();

    if(U_SUCCESS(status)) {
        setCollNonIgnorable(rbUCA);
        openTestFile("NON_IGNORABLE");
        testConformance(rbUCA);
    }
}

void UCAConformanceTest::TestRulesShifted(/* par */) {
    logln("This test is currently disabled, as it is impossible to "
        "wholly represent fractional UCA using tailoring rules.");
    return;

    initRbUCA();

    if(U_SUCCESS(status)) {
        setCollShifted(rbUCA);
        openTestFile("SHIFTED");
        testConformance(rbUCA);
    }
}

#endif /* #if !UCONFIG_NO_COLLATION */
