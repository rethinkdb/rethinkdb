/*
**********************************************************************
* Copyright (C) 2010, International Business Machines Corporation 
* and others.  All Rights Reserved.
**********************************************************************
*/
/**
 * IntlTestSpoof tests for USpoofDetector
 */

#include "unicode/utypes.h"

#if !UCONFIG_NO_REGULAR_EXPRESSIONS && !UCONFIG_NO_NORMALIZATION && !UCONFIG_NO_FILE_IO

#include "itspoof.h"
#include "unicode/uspoof.h"
#include "unicode/unistr.h"
#include "unicode/regex.h"
#include "unicode/normlzr.h"
#include "cstring.h"
#include <stdlib.h>
#include <stdio.h>

#define TEST_ASSERT_SUCCESS(status) {if (U_FAILURE(status)) { \
    errcheckln(status, "Failure at file %s, line %d, error = %s", __FILE__, __LINE__, u_errorName(status));}}

#define TEST_ASSERT(expr) {if ((expr)==FALSE) { \
    errln("Test Failure at file %s, line %d: \"%s\" is false.\n", __FILE__, __LINE__, #expr);};}

#define TEST_ASSERT_EQ(a, b) { if ((a) != (b)) { \
    errln("Test Failure at file %s, line %d: \"%s\" (%d) != \"%s\" (%d) \n", \
             __FILE__, __LINE__, #a, (a), #b, (b)); }}

#define TEST_ASSERT_NE(a, b) { if ((a) == (b)) { \
    errln("Test Failure at file %s, line %d: \"%s\" (%d) == \"%s\" (%d) \n", \
             __FILE__, __LINE__, #a, (a), #b, (b)); }}

/*
 *   TEST_SETUP and TEST_TEARDOWN
 *         macros to handle the boilerplate around setting up test case.
 *         Put arbitrary test code between SETUP and TEARDOWN.
 *         "sc" is the ready-to-go  SpoofChecker for use in the tests.
 */
#define TEST_SETUP {  \
    UErrorCode status = U_ZERO_ERROR; \
    USpoofChecker *sc;     \
    sc = uspoof_open(&status);  \
    TEST_ASSERT_SUCCESS(status);   \
    if (U_SUCCESS(status)){

#define TEST_TEARDOWN  \
    }  \
    TEST_ASSERT_SUCCESS(status);  \
    uspoof_close(sc);  \
}




void IntlTestSpoof::runIndexedTest( int32_t index, UBool exec, const char* &name, char* /*par*/ )
{
    if (exec) logln("TestSuite spoof: ");
    switch (index) {
        case 0:
            name = "TestSpoofAPI"; 
            if (exec) {
                testSpoofAPI();
            }
            break;
         case 1:
            name = "TestSkeleton"; 
            if (exec) {
                testSkeleton();
            }
            break;
         case 2:
            name = "TestAreConfusable";
            if (exec) {
                testAreConfusable();
            }
            break;
          case 3:
            name = "TestInvisible";
            if (exec) {
                testInvisible();
            }
            break;
          case 4:
            name = "testConfData";
            if (exec) {
                testConfData();
            }
            break;
        default: name=""; break;
    }
}

void IntlTestSpoof::testSpoofAPI() {

    TEST_SETUP
        UnicodeString s("xyz");  // Many latin ranges are whole-script confusable with other scripts.
                                 // If this test starts failing, consult confusablesWholeScript.txt
        int32_t position = 666;
        int32_t checkResults = uspoof_checkUnicodeString(sc, s, &position, &status);
        TEST_ASSERT_SUCCESS(status);
        TEST_ASSERT_EQ(0, checkResults);
        TEST_ASSERT_EQ(666, position);
    TEST_TEARDOWN;
    
    TEST_SETUP
        UnicodeString s1("cxs");
        UnicodeString s2 = UnicodeString("\\u0441\\u0445\\u0455").unescape();  // Cyrillic "cxs"
        int32_t checkResults = uspoof_areConfusableUnicodeString(sc, s1, s2, &status);
        TEST_ASSERT_EQ(USPOOF_MIXED_SCRIPT_CONFUSABLE | USPOOF_WHOLE_SCRIPT_CONFUSABLE, checkResults);

    TEST_TEARDOWN;

    TEST_SETUP
        UnicodeString s("I1l0O");
        UnicodeString dest;
        UnicodeString &retStr = uspoof_getSkeletonUnicodeString(sc, USPOOF_ANY_CASE, s, dest, &status);
        TEST_ASSERT_SUCCESS(status);
        TEST_ASSERT(UnicodeString("lllOO") == dest);
        TEST_ASSERT(&dest == &retStr);
    TEST_TEARDOWN;
}


#define CHECK_SKELETON(type, input, expected) { \
    checkSkeleton(sc, type, input, expected, __LINE__); \
    }


// testSkeleton.   Spot check a number of confusable skeleton substitutions from the 
//                 Unicode data file confusables.txt
//                 Test cases chosen for substitutions of various lengths, and 
//                 membership in different mapping tables.
void IntlTestSpoof::testSkeleton() {
    const uint32_t ML = 0;
    const uint32_t SL = USPOOF_SINGLE_SCRIPT_CONFUSABLE;
    const uint32_t MA = USPOOF_ANY_CASE;
    const uint32_t SA = USPOOF_SINGLE_SCRIPT_CONFUSABLE | USPOOF_ANY_CASE;

    TEST_SETUP
        // A long "identifier" that will overflow implementation stack buffers, forcing heap allocations.
        CHECK_SKELETON(SL, " A 1ong \\u02b9identifier' that will overflow implementation stack buffers, forcing heap allocations."
                           " A 1ong 'identifier' that will overflow implementation stack buffers, forcing heap allocations."
                           " A 1ong 'identifier' that will overflow implementation stack buffers, forcing heap allocations."
                           " A 1ong 'identifier' that will overflow implementation stack buffers, forcing heap allocations.",

               " A long 'identifier' that vvill overflovv irnplernentation stack buffers, forcing heap allocations."
               " A long 'identifier' that vvill overflovv irnplernentation stack buffers, forcing heap allocations."
               " A long 'identifier' that vvill overflovv irnplernentation stack buffers, forcing heap allocations."
               " A long 'identifier' that vvill overflovv irnplernentation stack buffers, forcing heap allocations.")

        // FC5F ;	FE74 0651 ;   ML  #* ARABIC LIGATURE SHADDA WITH KASRATAN ISOLATED FORM to
        //                                ARABIC KASRATAN ISOLATED FORM, ARABIC SHADDA	
        //    This character NFKD normalizes to \u0020 \u064d \u0651, so its confusable mapping 
        //    is never used in creating a skeleton.
        CHECK_SKELETON(SL, "\\uFC5F", " \\u064d\\u0651");

        CHECK_SKELETON(SL, "nochange", "nochange");
        CHECK_SKELETON(MA, "love", "love"); 
        CHECK_SKELETON(MA, "1ove", "love");   // Digit 1 to letter l
        CHECK_SKELETON(ML, "OOPS", "OOPS");
        CHECK_SKELETON(ML, "00PS", "00PS");   // Digit 0 unchanged in lower case mode.
        CHECK_SKELETON(MA, "OOPS", "OOPS");
        CHECK_SKELETON(MA, "00PS", "OOPS");   // Digit 0 to letter O in any case mode only
        CHECK_SKELETON(SL, "\\u059c", "\\u0301");
        CHECK_SKELETON(SL, "\\u2A74", "\\u003A\\u003A\\u003D");
        CHECK_SKELETON(SL, "\\u247E", "\\u0028\\u006C\\u006C\\u0029");  // "(ll)"
        CHECK_SKELETON(SL, "\\uFDFB", "\\u062C\\u0644\\u0020\\u062C\\u0644\\u0627\\u0644\\u0647");

        // This mapping exists in the ML and MA tables, does not exist in SL, SA
        //0C83 ;	0C03 ;	
        CHECK_SKELETON(SL, "\\u0C83", "\\u0C83");
        CHECK_SKELETON(SA, "\\u0C83", "\\u0C83");
        CHECK_SKELETON(ML, "\\u0C83", "\\u0983");
        CHECK_SKELETON(MA, "\\u0C83", "\\u0983");
        
        // 0391 ; 0041 ;
        // This mapping exists only in the MA table.
        CHECK_SKELETON(MA, "\\u0391", "A");
        CHECK_SKELETON(SA, "\\u0391", "\\u0391");
        CHECK_SKELETON(ML, "\\u0391", "\\u0391");
        CHECK_SKELETON(SL, "\\u0391", "\\u0391");

        // 13CF ;  0062 ; 
        // This mapping exists in the ML and MA tables
        CHECK_SKELETON(ML, "\\u13CF", "b");
        CHECK_SKELETON(MA, "\\u13CF", "b");
        CHECK_SKELETON(SL, "\\u13CF", "\\u13CF");
        CHECK_SKELETON(SA, "\\u13CF", "\\u13CF");

        // 0022 ;  0027 0027 ; 
        // all tables.
        CHECK_SKELETON(SL, "\\u0022", "\\u0027\\u0027");
        CHECK_SKELETON(SA, "\\u0022", "\\u0027\\u0027");
        CHECK_SKELETON(ML, "\\u0022", "\\u0027\\u0027");
        CHECK_SKELETON(MA, "\\u0022", "\\u0027\\u0027");

    TEST_TEARDOWN;
}


//
//  Run a single confusable skeleton transformation test case.
//
void IntlTestSpoof::checkSkeleton(const USpoofChecker *sc, uint32_t type, 
                                  const char *input, const char *expected, int32_t lineNum) {
    UnicodeString uInput = UnicodeString(input).unescape();
    UnicodeString uExpected = UnicodeString(expected).unescape();
    
    UErrorCode status = U_ZERO_ERROR;
    UnicodeString actual;
    uspoof_getSkeletonUnicodeString(sc, type, uInput, actual, &status);
    if (U_FAILURE(status)) {
        errln("File %s, Line %d, Test case from line %d, status is %s", __FILE__, __LINE__, lineNum,
              u_errorName(status));
        return;
    }
    if (uExpected != actual) {
        errln("File %s, Line %d, Test case from line %d, Actual and Expected skeletons differ.",
               __FILE__, __LINE__, lineNum);
        errln(UnicodeString(" Actual   Skeleton: \"") + actual + UnicodeString("\"\n") +
              UnicodeString(" Expected Skeleton: \"") + uExpected + UnicodeString("\""));
    }
}

void IntlTestSpoof::testAreConfusable() {
    TEST_SETUP
        UnicodeString s1("A long string that will overflow stack buffers.  A long string that will overflow stack buffers. "
                         "A long string that will overflow stack buffers.  A long string that will overflow stack buffers. ");
        UnicodeString s2("A long string that wi11 overflow stack buffers.  A long string that will overflow stack buffers. "
                         "A long string that wi11 overflow stack buffers.  A long string that will overflow stack buffers. ");
        TEST_ASSERT_EQ(USPOOF_SINGLE_SCRIPT_CONFUSABLE, uspoof_areConfusableUnicodeString(sc, s1, s2, &status));
        TEST_ASSERT_SUCCESS(status);

    TEST_TEARDOWN;
}

void IntlTestSpoof::testInvisible() {
    TEST_SETUP
        UnicodeString  s = UnicodeString("abcd\\u0301ef").unescape();
        int32_t position = -42;
        TEST_ASSERT_EQ(0, uspoof_checkUnicodeString(sc, s, &position, &status));
        TEST_ASSERT_SUCCESS(status);
        TEST_ASSERT(position == -42);

        UnicodeString  s2 = UnicodeString("abcd\\u0301\\u0302\\u0301ef").unescape();
        TEST_ASSERT_EQ(USPOOF_INVISIBLE, uspoof_checkUnicodeString(sc, s2, &position, &status));
        TEST_ASSERT_SUCCESS(status);
        TEST_ASSERT_EQ(7, position);

        // Tow acute accents, one from the composed a with acute accent, \u00e1,
        // and one separate.
        position = -42;
        UnicodeString  s3 = UnicodeString("abcd\\u00e1\\u0301xyz").unescape();
        TEST_ASSERT_EQ(USPOOF_INVISIBLE, uspoof_checkUnicodeString(sc, s3, &position, &status));
        TEST_ASSERT_SUCCESS(status);
        TEST_ASSERT_EQ(7, position);
    TEST_TEARDOWN;
}


static UnicodeString parseHex(const UnicodeString &in) {
    // Convert a series of hex numbers in a Unicode String to a string with the
    // corresponding characters.
    // The conversion is _really_ annoying.  There must be some function to just do it.
    UnicodeString result;
    UChar32 cc = 0;
    for (int32_t i=0; i<in.length(); i++) {
        UChar c = in.charAt(i);
        if (c == 0x20) {   // Space
            if (cc > 0) {
               result.append(cc);
               cc = 0;
            }
        } else if (c>=0x30 && c<=0x39) {
            cc = (cc<<4) + (c - 0x30);
        } else if ((c>=0x41 && c<=0x46) || (c>=0x61 && c<=0x66)) {
            cc = (cc<<4) + (c & 0x0f)+9;
        }
        // else do something with bad input.
    }
    if (cc > 0) {
        result.append(cc);
    }
    return result;
}


//
// Append the hex form of a UChar32 to a UnicodeString.
// Used in formatting error messages.
// Match the formatting of numbers in confusables.txt
// Minimum of 4 digits, no leading zeroes for positions 5 and up.
//
static void appendHexUChar(UnicodeString &dest, UChar32 c) {
    UBool   doZeroes = FALSE;    
    for (int bitNum=28; bitNum>=0; bitNum-=4) {
        if (bitNum <= 12) {
            doZeroes = TRUE;
        }
        int hexDigit = (c>>bitNum) & 0x0f;
        if (hexDigit != 0 || doZeroes) {
            doZeroes = TRUE;
            dest.append((UChar)(hexDigit<=9? hexDigit + 0x30: hexDigit -10 + 0x41));
        }
    }
    dest.append((UChar)0x20);
}

U_DEFINE_LOCAL_OPEN_POINTER(LocalStdioFilePointer, FILE, fclose);

//  testConfData - Check each data item from the Unicode confusables.txt file,
//                 verify that it transforms correctly in a skeleton.
//
void IntlTestSpoof::testConfData() {
    UErrorCode status = U_ZERO_ERROR;

    const char *testDataDir = IntlTest::getSourceTestData(status);
    TEST_ASSERT_SUCCESS(status);
    char buffer[2000];
    uprv_strcpy(buffer, testDataDir);
    uprv_strcat(buffer, "confusables.txt");

    LocalStdioFilePointer f(fopen(buffer, "rb"));
    if (f.isNull()) {
        errln("Skipping test spoof/testConfData.  File confusables.txt not accessible.");
        return;
    }
    fseek(f.getAlias(), 0, SEEK_END);
    int32_t  fileSize = ftell(f.getAlias());
    LocalArray<char> fileBuf(new char[fileSize]);
    fseek(f.getAlias(), 0, SEEK_SET);
    int32_t amt_read = fread(fileBuf.getAlias(), 1, fileSize, f.getAlias());
    TEST_ASSERT_EQ(amt_read, fileSize);
    TEST_ASSERT(fileSize>0);
    if (amt_read != fileSize || fileSize <=0) {
        return;
    }
    UnicodeString confusablesTxt = UnicodeString::fromUTF8(StringPiece(fileBuf.getAlias(), fileSize));

    LocalUSpoofCheckerPointer sc(uspoof_open(&status));
    TEST_ASSERT_SUCCESS(status);

    // Parse lines from the confusables.txt file.  Example Line:
    // FF44 ;	0064 ;	SL	# ( d -> d ) FULLWIDTH ....
    // Three fields.  The hex fields can contain more than one character,
    //                and each character may be more than 4 digits (for supplemntals)
    // This regular expression matches lines and splits the fields into capture groups.
    RegexMatcher parseLine("(?m)^([0-9A-F]{4}[^#;]*?);([^#;]*?);([^#]*)", confusablesTxt, 0, status);
    TEST_ASSERT_SUCCESS(status);
    while (parseLine.find()) {
        UnicodeString from = parseHex(parseLine.group(1, status));
        if (!Normalizer::isNormalized(from, UNORM_NFKD, status)) {
            // The source character was not NFKD.
            // Skip this case; the first step in obtaining a skeleton is to NFKD the input,
            //  so the mapping in this line of confusables.txt will never be applied.
            continue;
        }

        UnicodeString rawExpected = parseHex(parseLine.group(2, status));
        UnicodeString expected;
        Normalizer::decompose(rawExpected, TRUE, 0, expected, status);
        TEST_ASSERT_SUCCESS(status);

        int32_t skeletonType = 0;
        UnicodeString tableType = parseLine.group(3, status);
        TEST_ASSERT_SUCCESS(status);
        if (tableType.indexOf("SL") >= 0) {
            skeletonType = USPOOF_SINGLE_SCRIPT_CONFUSABLE;
        } else if (tableType.indexOf("SA") >= 0) {
            skeletonType = USPOOF_SINGLE_SCRIPT_CONFUSABLE | USPOOF_ANY_CASE;
        } else if (tableType.indexOf("ML") >= 0) {
            skeletonType = 0;
        } else if (tableType.indexOf("MA") >= 0) {
            skeletonType = USPOOF_ANY_CASE;
        }

        UnicodeString actual;
        uspoof_getSkeletonUnicodeString(sc.getAlias(), skeletonType, from, actual, &status);
        TEST_ASSERT_SUCCESS(status);
        TEST_ASSERT(actual == expected);
        if (actual != expected) {
            errln(parseLine.group(0, status));
            UnicodeString line = "Actual: ";
            int i = 0;
            while (i < actual.length()) {
                appendHexUChar(line, actual.char32At(i));
                i = actual.moveIndex32(i, 1);
            }
            errln(line);
        }
        if (U_FAILURE(status)) {
            break;
        }
    }
}
#endif // UCONFIG_NO_REGULAR_EXPRESSIONS

