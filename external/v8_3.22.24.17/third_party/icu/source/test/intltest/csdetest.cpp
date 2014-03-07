/*
 **********************************************************************
 *   Copyright (C) 2005-2009, International Business Machines
 *   Corporation and others.  All Rights Reserved.
 **********************************************************************
 */


#include "unicode/utypes.h"
#include "unicode/ucsdet.h"
#include "unicode/ucnv.h"
#include "unicode/unistr.h"
#include "unicode/putil.h"
#include "unicode/uniset.h"

#include "intltest.h"
#include "csdetest.h"

#include "xmlparser.h"

#include <stdlib.h>
#include <string.h>

#ifdef DEBUG_DETECT
#include <stdio.h>
#endif

#define ARRAY_SIZE(array) (sizeof array / sizeof array[0])

#define NEW_ARRAY(type,count) (type *) /*uprv_*/malloc((count) * sizeof(type))
#define DELETE_ARRAY(array) /*uprv_*/free((void *) (array))

#define CH_SPACE 0x0020
#define CH_SLASH 0x002F

//---------------------------------------------------------------------------
//
//  Test class boilerplate
//
//---------------------------------------------------------------------------
CharsetDetectionTest::CharsetDetectionTest()
{
}


CharsetDetectionTest::~CharsetDetectionTest()
{
}



void CharsetDetectionTest::runIndexedTest( int32_t index, UBool exec, const char* &name, char* /*par*/ )
{
    if (exec) logln("TestSuite CharsetDetectionTest: ");
    switch (index) {
       case 0: name = "ConstructionTest";
            if (exec) ConstructionTest();
            break;

       case 1: name = "UTF8Test";
            if (exec) UTF8Test();
            break;

       case 2: name = "UTF16Test";
            if (exec) UTF16Test();
            break;

       case 3: name = "C1BytesTest";
            if (exec) C1BytesTest();
            break;

       case 4: name = "InputFilterTest";
            if (exec) InputFilterTest();
            break;

       case 5: name = "DetectionTest";
            if (exec) DetectionTest();
            break;
#if !UCONFIG_NO_LEGACY_CONVERSION
       case 6: name = "IBM424Test";
            if (exec) IBM424Test();
            break;

       case 7: name = "IBM420Test";
            if (exec) IBM420Test();
            break;
#else
       case 6:
       case 7: name = "skip"; break;
#endif
       case 8: name = "Ticket6394Test";
            if (exec) Ticket6394Test();
            break;

        default: name = "";
            break; //needed to end loop
    }
}

static UnicodeString *split(const UnicodeString &src, UChar ch, int32_t &splits)
{
    int32_t offset = -1;

    splits = 1;
    while((offset = src.indexOf(ch, offset + 1)) >= 0) {
        splits += 1;
    }

    UnicodeString *result = new UnicodeString[splits];

    int32_t start = 0;
    int32_t split = 0;
    int32_t end;

    while((end = src.indexOf(ch, start)) >= 0) {
        src.extractBetween(start, end, result[split++]);
        start = end + 1;
    }

    src.extractBetween(start, src.length(), result[split]);

    return result;
}

static char *extractBytes(const UnicodeString &source, const char *codepage, int32_t &length)
{
    int32_t sLength = source.length();
    char *bytes = NULL;

    length = source.extract(0, sLength, NULL, codepage);

    if (length > 0) {
        bytes = NEW_ARRAY(char, length + 1);
        source.extract(0, sLength, bytes, codepage);
    }
    
    return bytes;
}

static void freeBytes(char *bytes)
{
    DELETE_ARRAY(bytes);
}

void CharsetDetectionTest::checkEncoding(const UnicodeString &testString, const UnicodeString &encoding, const UnicodeString &id)
{
    int32_t splits = 0;
    int32_t testLength = testString.length();
    UnicodeString *eSplit = split(encoding, CH_SLASH, splits);
    UErrorCode status = U_ZERO_ERROR;
    int32_t cpLength = eSplit[0].length();
    char codepage[64];

    u_UCharsToChars(eSplit[0].getBuffer(), codepage, cpLength);
    codepage[cpLength] = '\0';

    LocalUCharsetDetectorPointer csd(ucsdet_open(&status));

    int32_t byteLength = 0;
    char *bytes = extractBytes(testString, codepage, byteLength);

    if (bytes == NULL) {
#if !UCONFIG_NO_LEGACY_CONVERSION
        errln("Can't open a " + encoding + " converter for " + id);
#endif
        return;
    }

    ucsdet_setText(csd.getAlias(), bytes, byteLength, &status);

    int32_t matchCount = 0;
    const UCharsetMatch **matches = ucsdet_detectAll(csd.getAlias(), &matchCount, &status);


    UnicodeString name(ucsdet_getName(matches[0], &status));
    UnicodeString lang(ucsdet_getLanguage(matches[0], &status));
    UChar *decoded = NULL;
    int32_t dLength = 0;

    if (matchCount == 0) {
        errln("Encoding detection failure for " + id + ": expected " + eSplit[0] + ", got no matches");
        goto bail;
    }

    if (name.compare(eSplit[0]) != 0) {
        errln("Encoding detection failure for " + id + ": expected " + eSplit[0] + ", got " + name);

#ifdef DEBUG_DETECT
        for (int32_t m = 0; m < matchCount; m += 1) {
            const char *name = ucsdet_getName(matches[m], &status);
            const char *lang = ucsdet_getLanguage(matches[m], &status);
            int32_t confidence = ucsdet_getConfidence(matches[m], &status);

            printf("%s (%s) %d\n", name, lang, confidence);
        }
#endif
        goto bail;
    }

    if (splits > 1 && lang.compare(eSplit[1]) != 0) {
        errln("Language detection failure for " + id + ", " + eSplit[0] + ": expected " + eSplit[1] + ", got " + lang);
        goto bail;
    }

    decoded = NEW_ARRAY(UChar, testLength);
    dLength = ucsdet_getUChars(matches[0], decoded, testLength, &status);

    if (testString.compare(decoded, dLength) != 0) {
        errln("Round-trip error for " + id + ", " + eSplit[0] + ": getUChars() didn't yeild the original string.");

#ifdef DEBUG_DETECT
        for(int32_t i = 0; i < testLength; i += 1) {
            if(testString[i] != decoded[i]) {
                printf("Strings differ at byte %d\n", i);
                break;
            }
        }
#endif

    }

    DELETE_ARRAY(decoded);

bail:
    freeBytes(bytes);
    delete[] eSplit;
}

const char *CharsetDetectionTest::getPath(char buffer[2048], const char *filename) {
    UErrorCode status = U_ZERO_ERROR;
    const char *testDataDirectory = IntlTest::getSourceTestData(status);

    if (U_FAILURE(status)) {
        errln("ERROR: getPath() failed - %s", u_errorName(status));
        return NULL;
    }

    strcpy(buffer, testDataDirectory);
    strcat(buffer, filename);
    return buffer;
}

void CharsetDetectionTest::ConstructionTest()
{
    IcuTestErrorCode status(*this, "ConstructionTest");
    LocalUCharsetDetectorPointer csd(ucsdet_open(status));
    LocalUEnumerationPointer e(ucsdet_getAllDetectableCharsets(csd.getAlias(), status));
    int32_t count = uenum_count(e.getAlias(), status);

#ifdef DEBUG_DETECT
    printf("There are %d recognizers.\n", count);
#endif

    for(int32_t i = 0; i < count; i += 1) {
        int32_t length;
        const char *name = uenum_next(e.getAlias(), &length, status);

        if(name == NULL || length <= 0) {
            errln("ucsdet_getAllDetectableCharsets() returned a null or empty name!");
        }

#ifdef DEBUG_DETECT
        printf("%s\n", name);
#endif
    }
}

void CharsetDetectionTest::UTF8Test()
{
    UErrorCode status = U_ZERO_ERROR;
    UnicodeString ss = "This is a string with some non-ascii characters that will "
                       "be converted to UTF-8, then shoved through the detection process.  "
                       "\\u0391\\u0392\\u0393\\u0394\\u0395"
                       "Sure would be nice if our source could contain Unicode directly!";
    UnicodeString s = ss.unescape();
    int32_t byteLength = 0, sLength = s.length();
    char *bytes = extractBytes(s, "UTF-8", byteLength);
    UCharsetDetector *csd = ucsdet_open(&status);
    const UCharsetMatch *match;
    UChar *detected = NEW_ARRAY(UChar, sLength);

    ucsdet_setText(csd, bytes, byteLength, &status);
    match = ucsdet_detect(csd, &status);

    if (match == NULL) {
        errln("Detection failure for UTF-8: got no matches.");
        goto bail;
    }

    ucsdet_getUChars(match, detected, sLength, &status);

    if (s.compare(detected, sLength) != 0) {
        errln("Round-trip test failed!");
    }

    ucsdet_setDeclaredEncoding(csd, "UTF-8", 5, &status); /* for coverage */

bail:
    DELETE_ARRAY(detected);
    freeBytes(bytes);
    ucsdet_close(csd);
}

void CharsetDetectionTest::UTF16Test()
{
    UErrorCode status = U_ZERO_ERROR;
    /* Notice the BOM on the start of this string */
    UChar chars[] = {
        0xFEFF, 0x0623, 0x0648, 0x0631, 0x0648, 0x0628, 0x0627, 0x002C,
        0x0020, 0x0628, 0x0631, 0x0645, 0x062c, 0x064a, 0x0627, 0x062a,
        0x0020, 0x0627, 0x0644, 0x062d, 0x0627, 0x0633, 0x0648, 0x0628,
        0x0020, 0x002b, 0x0020, 0x0627, 0x0646, 0x062a, 0x0631, 0x0646,
        0x064a, 0x062a, 0x0000};
    UnicodeString s(chars);
    int32_t beLength = 0, leLength = 0;
    char *beBytes = extractBytes(s, "UTF-16BE", beLength);
    char *leBytes = extractBytes(s, "UTF-16LE", leLength);
    UCharsetDetector *csd = ucsdet_open(&status);
    const UCharsetMatch *match;
    const char *name;
    int32_t conf;

    ucsdet_setText(csd, beBytes, beLength, &status);
    match = ucsdet_detect(csd, &status);

    if (match == NULL) {
        errln("Encoding detection failure for UTF-16BE: got no matches.");
        goto try_le;
    }

    name  = ucsdet_getName(match, &status);
    conf  = ucsdet_getConfidence(match, &status);

    if (strcmp(name, "UTF-16BE") != 0) {
        errln("Encoding detection failure for UTF-16BE: got %s", name);
        goto try_le; // no point in looking at confidence if we got the wrong character set.
    }

    if (conf != 100) {
        errln("Did not get 100%% confidence for UTF-16BE: got %d", conf);
    }

try_le:
    ucsdet_setText(csd, leBytes, leLength, &status);
    match = ucsdet_detect(csd, &status);

    if (match == NULL) {
        errln("Encoding detection failure for UTF-16LE: got no matches.");
        goto bail;
    }

    name  = ucsdet_getName(match, &status);
    conf = ucsdet_getConfidence(match, &status);


    if (strcmp(name, "UTF-16LE") != 0) {
        errln("Enconding detection failure for UTF-16LE: got %s", name);
        goto bail; // no point in looking at confidence if we got the wrong character set.
    }

    if (conf != 100) {
        errln("Did not get 100%% confidence for UTF-16LE: got %d", conf);
    }

bail:
    freeBytes(leBytes);
    freeBytes(beBytes);
    ucsdet_close(csd);
}

void CharsetDetectionTest::InputFilterTest()
{
    UErrorCode status = U_ZERO_ERROR;
    UnicodeString ss = "<a> <lot> <of> <English> <inside> <the> <markup> Un tr\\u00E8s petit peu de Fran\\u00E7ais. <to> <confuse> <the> <detector>";
    UnicodeString s  = ss.unescape();
    int32_t byteLength = 0;
    char *bytes = extractBytes(s, "ISO-8859-1", byteLength);
    UCharsetDetector *csd = ucsdet_open(&status);
    const UCharsetMatch *match;
    const char *lang, *name;

    ucsdet_enableInputFilter(csd, TRUE);

    if (!ucsdet_isInputFilterEnabled(csd)) {
        errln("ucsdet_enableInputFilter(csd, TRUE) did not enable input filter!");
    }


    ucsdet_setText(csd, bytes, byteLength, &status);
    match = ucsdet_detect(csd, &status);

    if (match == NULL) {
        errln("Turning on the input filter resulted in no matches.");
        goto turn_off;
    }

    name = ucsdet_getName(match, &status);

    if (name == NULL || strcmp(name, "ISO-8859-1") != 0) {
        errln("Turning on the input filter resulted in %s rather than ISO-8859-1.", name);
    } else {
        lang = ucsdet_getLanguage(match, &status);

        if (lang == NULL || strcmp(lang, "fr") != 0) {
            errln("Input filter did not strip markup!");
        }
    }

turn_off:
    ucsdet_enableInputFilter(csd, FALSE);
    ucsdet_setText(csd, bytes, byteLength, &status);
    match = ucsdet_detect(csd, &status);

    if (match == NULL) {
        errln("Turning off the input filter resulted in no matches.");
        goto bail;
    }

    name = ucsdet_getName(match, &status);

    if (name == NULL || strcmp(name, "ISO-8859-1") != 0) {
        errln("Turning off the input filter resulted in %s rather than ISO-8859-1.", name);
    } else {
        lang = ucsdet_getLanguage(match, &status);

        if (lang == NULL || strcmp(lang, "en") != 0) {
            errln("Unfiltered input did not detect as English!");
        }
    }

bail:
    freeBytes(bytes);
    ucsdet_close(csd);
}

void CharsetDetectionTest::C1BytesTest()
{
#if !UCONFIG_NO_LEGACY_CONVERSION
    UErrorCode status = U_ZERO_ERROR;
    UnicodeString sISO = "This is a small sample of some English text. Just enough to be sure that it detects correctly.";
    UnicodeString ssWindows("This is another small sample of some English text. Just enough to be sure that it detects correctly. It also includes some \\u201CC1\\u201D bytes.", -1, US_INV);
    UnicodeString sWindows  = ssWindows.unescape();
    int32_t lISO = 0, lWindows = 0;
    char *bISO = extractBytes(sISO, "ISO-8859-1", lISO);
    char *bWindows = extractBytes(sWindows, "windows-1252", lWindows);
    UCharsetDetector *csd = ucsdet_open(&status);
    const UCharsetMatch *match;
    const char *name;

    ucsdet_setText(csd, bWindows, lWindows, &status);
    match = ucsdet_detect(csd, &status);

    if (match == NULL) {
        errcheckln(status, "English test with C1 bytes got no matches. - %s", u_errorName(status));
        goto bail;
    }

    name  = ucsdet_getName(match, &status);

    if (strcmp(name, "windows-1252") != 0) {
        errln("English text with C1 bytes does not detect as windows-1252, but as %s", name);
    }

    ucsdet_setText(csd, bISO, lISO, &status);
    match = ucsdet_detect(csd, &status);

    if (match == NULL) {
        errln("English text without C1 bytes got no matches.");
        goto bail;
    }

    name  = ucsdet_getName(match, &status);

    if (strcmp(name, "ISO-8859-1") != 0) {
        errln("English text without C1 bytes does not detect as ISO-8859-1, but as %s", name);
    }

bail:
    freeBytes(bWindows);
    freeBytes(bISO);

    ucsdet_close(csd);
#endif
}

void CharsetDetectionTest::DetectionTest()
{
#if !UCONFIG_NO_REGULAR_EXPRESSIONS
    UErrorCode status = U_ZERO_ERROR;
    char path[2048];
    const char *testFilePath = getPath(path, "csdetest.xml");

    if (testFilePath == NULL) {
        return; /* Couldn't get path: error message already output. */
    }

    UXMLParser  *parser = UXMLParser::createParser(status);
    if (U_FAILURE(status)) {
        dataerrln("FAIL: UXMLParser::createParser (%s)", u_errorName(status));
        return;
    }

    UXMLElement *root   = parser->parseFile(testFilePath, status);
    if (!assertSuccess( "parseFile",status)) return;

    UnicodeString test_case = UNICODE_STRING_SIMPLE("test-case");
    UnicodeString id_attr   = UNICODE_STRING_SIMPLE("id");
    UnicodeString enc_attr  = UNICODE_STRING_SIMPLE("encodings");

    const UXMLElement *testCase;
    int32_t tc = 0;

    while((testCase = root->nextChildElement(tc)) != NULL) {
        if (testCase->getTagName().compare(test_case) == 0) {
            const UnicodeString *id = testCase->getAttribute(id_attr);
            const UnicodeString *encodings = testCase->getAttribute(enc_attr);
            const UnicodeString  text = testCase->getText(TRUE);
            int32_t encodingCount;
            UnicodeString *encodingList = split(*encodings, CH_SPACE, encodingCount);

            for(int32_t e = 0; e < encodingCount; e += 1) {
                checkEncoding(text, encodingList[e], *id);
            }

            delete[] encodingList;
        }
    }

    delete root;
    delete parser;
#endif
}

void CharsetDetectionTest::IBM424Test()
{
    UErrorCode status = U_ZERO_ERROR;
    
    static const UChar chars[] = {
            0x05D4, 0x05E4, 0x05E8, 0x05E7, 0x05DC, 0x05D9, 0x05D8, 0x0020, 0x05D4, 0x05E6, 0x05D1, 0x05D0, 0x05D9, 0x0020, 0x05D4, 0x05E8,
            0x05D0, 0x05E9, 0x05D9, 0x002C, 0x0020, 0x05EA, 0x05EA, 0x0020, 0x05D0, 0x05DC, 0x05D5, 0x05E3, 0x0020, 0x05D0, 0x05D1, 0x05D9,
            0x05D7, 0x05D9, 0x0020, 0x05DE, 0x05E0, 0x05D3, 0x05DC, 0x05D1, 0x05DC, 0x05D9, 0x05D8, 0x002C, 0x0020, 0x05D4, 0x05D5, 0x05E8,
            0x05D4, 0x0020, 0x05E2, 0x05DC, 0x0020, 0x05E4, 0x05EA, 0x05D9, 0x05D7, 0x05EA, 0x0020, 0x05D7, 0x05E7, 0x05D9, 0x05E8, 0x05EA,
            0x0020, 0x05DE, 0x05E6, 0x0022, 0x05D7, 0x0020, 0x05D1, 0x05E2, 0x05E7, 0x05D1, 0x05D5, 0x05EA, 0x0020, 0x05E2, 0x05D3, 0x05D5,
            0x05D9, 0x05D5, 0x05EA, 0x0020, 0x05D7, 0x05D9, 0x05D9, 0x05DC, 0x05D9, 0x0020, 0x05E6, 0x05D4, 0x0022, 0x05DC, 0x0020, 0x05DE,
            0x05DE, 0x05D1, 0x05E6, 0x05E2, 0x0020, 0x05E2, 0x05D5, 0x05E4, 0x05E8, 0x05EA, 0x0020, 0x05D9, 0x05E6, 0x05D5, 0x05E7, 0x05D4,
            0x0020, 0x05D1, 0x002B, 0x0020, 0x05E8, 0x05E6, 0x05D5, 0x05E2, 0x05EA, 0x0020, 0x05E2, 0x05D6, 0x05D4, 0x002E, 0x0020, 0x05DC,
            0x05D3, 0x05D1, 0x05E8, 0x05D9, 0x0020, 0x05D4, 0x05E4, 0x05E6, 0x0022, 0x05E8, 0x002C, 0x0020, 0x05DE, 0x05D4, 0x05E2, 0x05D3,
            0x05D5, 0x05D9, 0x05D5, 0x05EA, 0x0020, 0x05E2, 0x05D5, 0x05DC, 0x05D4, 0x0020, 0x05EA, 0x05DE, 0x05D5, 0x05E0, 0x05D4, 0x0020,
            0x05E9, 0x05DC, 0x0020, 0x0022, 0x05D4, 0x05EA, 0x05E0, 0x05D4, 0x05D2, 0x05D5, 0x05EA, 0x0020, 0x05E4, 0x05E1, 0x05D5, 0x05DC,
            0x05D4, 0x0020, 0x05DC, 0x05DB, 0x05D0, 0x05D5, 0x05E8, 0x05D4, 0x0020, 0x05E9, 0x05DC, 0x0020, 0x05D7, 0x05D9, 0x05D9, 0x05DC,
            0x05D9, 0x05DD, 0x0020, 0x05D1, 0x05DE, 0x05D4, 0x05DC, 0x05DA, 0x0020, 0x05DE, 0x05D1, 0x05E6, 0x05E2, 0x0020, 0x05E2, 0x05D5,
            0x05E4, 0x05E8, 0x05EA, 0x0020, 0x05D9, 0x05E6, 0x05D5, 0x05E7, 0x05D4, 0x0022, 0x002E, 0x0020, 0x05DE, 0x05E0, 0x05D3, 0x05DC,
            0x05D1, 0x05DC, 0x05D9, 0x05D8, 0x0020, 0x05E7, 0x05D9, 0x05D1, 0x05DC, 0x0020, 0x05D0, 0x05EA, 0x0020, 0x05D4, 0x05D7, 0x05DC,
            0x05D8, 0x05EA, 0x05D5, 0x0020, 0x05DC, 0x05D0, 0x05D7, 0x05E8, 0x0020, 0x05E9, 0x05E2, 0x05D9, 0x05D9, 0x05DF, 0x0020, 0x05D1,
            0x05EA, 0x05DE, 0x05DC, 0x05D9, 0x05DC, 0x0020, 0x05D4, 0x05E2, 0x05D3, 0x05D5, 0x05D9, 0x05D5, 0x05EA, 0x0000
    };
    
    static const UChar chars_reverse[] = {
            0x05EA, 0x05D5, 0x05D9, 0x05D5, 0x05D3, 0x05E2, 0x05D4, 0x0020, 0x05DC, 0x05D9, 0x05DC, 0x05DE, 0x05EA,
            0x05D1, 0x0020, 0x05DF, 0x05D9, 0x05D9, 0x05E2, 0x05E9, 0x0020, 0x05E8, 0x05D7, 0x05D0, 0x05DC, 0x0020, 0x05D5, 0x05EA, 0x05D8,
            0x05DC, 0x05D7, 0x05D4, 0x0020, 0x05EA, 0x05D0, 0x0020, 0x05DC, 0x05D1, 0x05D9, 0x05E7, 0x0020, 0x05D8, 0x05D9, 0x05DC, 0x05D1,
            0x05DC, 0x05D3, 0x05E0, 0x05DE, 0x0020, 0x002E, 0x0022, 0x05D4, 0x05E7, 0x05D5, 0x05E6, 0x05D9, 0x0020, 0x05EA, 0x05E8, 0x05E4,
            0x05D5, 0x05E2, 0x0020, 0x05E2, 0x05E6, 0x05D1, 0x05DE, 0x0020, 0x05DA, 0x05DC, 0x05D4, 0x05DE, 0x05D1, 0x0020, 0x05DD, 0x05D9,
            0x05DC, 0x05D9, 0x05D9, 0x05D7, 0x0020, 0x05DC, 0x05E9, 0x0020, 0x05D4, 0x05E8, 0x05D5, 0x05D0, 0x05DB, 0x05DC, 0x0020, 0x05D4,
            0x05DC, 0x05D5, 0x05E1, 0x05E4, 0x0020, 0x05EA, 0x05D5, 0x05D2, 0x05D4, 0x05E0, 0x05EA, 0x05D4, 0x0022, 0x0020, 0x05DC, 0x05E9,
            0x0020, 0x05D4, 0x05E0, 0x05D5, 0x05DE, 0x05EA, 0x0020, 0x05D4, 0x05DC, 0x05D5, 0x05E2, 0x0020, 0x05EA, 0x05D5, 0x05D9, 0x05D5,
            0x05D3, 0x05E2, 0x05D4, 0x05DE, 0x0020, 0x002C, 0x05E8, 0x0022, 0x05E6, 0x05E4, 0x05D4, 0x0020, 0x05D9, 0x05E8, 0x05D1, 0x05D3,
            0x05DC, 0x0020, 0x002E, 0x05D4, 0x05D6, 0x05E2, 0x0020, 0x05EA, 0x05E2, 0x05D5, 0x05E6, 0x05E8, 0x0020, 0x002B, 0x05D1, 0x0020,
            0x05D4, 0x05E7, 0x05D5, 0x05E6, 0x05D9, 0x0020, 0x05EA, 0x05E8, 0x05E4, 0x05D5, 0x05E2, 0x0020, 0x05E2, 0x05E6, 0x05D1, 0x05DE,
            0x05DE, 0x0020, 0x05DC, 0x0022, 0x05D4, 0x05E6, 0x0020, 0x05D9, 0x05DC, 0x05D9, 0x05D9, 0x05D7, 0x0020, 0x05EA, 0x05D5, 0x05D9,
            0x05D5, 0x05D3, 0x05E2, 0x0020, 0x05EA, 0x05D5, 0x05D1, 0x05E7, 0x05E2, 0x05D1, 0x0020, 0x05D7, 0x0022, 0x05E6, 0x05DE, 0x0020,
            0x05EA, 0x05E8, 0x05D9, 0x05E7, 0x05D7, 0x0020, 0x05EA, 0x05D7, 0x05D9, 0x05EA, 0x05E4, 0x0020, 0x05DC, 0x05E2, 0x0020, 0x05D4,
            0x05E8, 0x05D5, 0x05D4, 0x0020, 0x002C, 0x05D8, 0x05D9, 0x05DC, 0x05D1, 0x05DC, 0x05D3, 0x05E0, 0x05DE, 0x0020, 0x05D9, 0x05D7,
            0x05D9, 0x05D1, 0x05D0, 0x0020, 0x05E3, 0x05D5, 0x05DC, 0x05D0, 0x0020, 0x05EA, 0x05EA, 0x0020, 0x002C, 0x05D9, 0x05E9, 0x05D0,
            0x05E8, 0x05D4, 0x0020, 0x05D9, 0x05D0, 0x05D1, 0x05E6, 0x05D4, 0x0020, 0x05D8, 0x05D9, 0x05DC, 0x05E7, 0x05E8, 0x05E4, 0x05D4,
            0x0000
    };

    int32_t bLength = 0, brLength = 0;
    
    UnicodeString s1(chars);
    UnicodeString s2(chars_reverse);
    
    char *bytes = extractBytes(s1, "IBM424", bLength);
    char *bytes_r = extractBytes(s2, "IBM424", brLength);
    
    UCharsetDetector *csd = ucsdet_open(&status);
    if (U_FAILURE(status)) {
        errln("Error opening charset detector. - %s", u_errorName(status));
    }
    const UCharsetMatch *match;
    const char *name;

    ucsdet_setText(csd, bytes, bLength, &status);
    match = ucsdet_detect(csd, &status);

    if (match == NULL) {
        errcheckln(status, "Encoding detection failure for IBM424_rtl: got no matches. - %s", u_errorName(status));
        goto bail;
    }

    name  = ucsdet_getName(match, &status);
    if (strcmp(name, "IBM424_rtl") != 0) {
        errln("Encoding detection failure for IBM424_rtl: got %s", name);
    }
    
    ucsdet_setText(csd, bytes_r, brLength, &status);
    match = ucsdet_detect(csd, &status);

    if (match == NULL) {
        errln("Encoding detection failure for IBM424_ltr: got no matches.");
        goto bail;
    }

    name  = ucsdet_getName(match, &status);
    if (strcmp(name, "IBM424_ltr") != 0) {
        errln("Encoding detection failure for IBM424_ltr: got %s", name);
    }

bail:
    freeBytes(bytes);
    freeBytes(bytes_r);
    ucsdet_close(csd);
}

void CharsetDetectionTest::IBM420Test()
{
    UErrorCode status = U_ZERO_ERROR;
    
    static const UChar chars[] = {
        0x0648, 0x064F, 0x0636, 0x0639, 0x062A, 0x0020, 0x0648, 0x0646, 0x064F, 0x0641, 0x0630, 0x062A, 0x0020, 0x0628, 0x0631, 0x0627,
        0x0645, 0x062C, 0x0020, 0x062A, 0x0623, 0x0645, 0x064A, 0x0646, 0x0020, 0x0639, 0x062F, 0x064A, 0x062F, 0x0629, 0x0020, 0x0641,
        0x064A, 0x0020, 0x0645, 0x0624, 0x0633, 0x0633, 0x0629, 0x0020, 0x0627, 0x0644, 0x062A, 0x0623, 0x0645, 0x064A, 0x0646, 0x0020,
        0x0627, 0x0644, 0x0648, 0x0637, 0x0646, 0x064A, 0x002C, 0x0020, 0x0645, 0x0639, 0x0020, 0x0645, 0x0644, 0x0627, 0x0626, 0x0645,
        0x062A, 0x0647, 0x0627, 0x0020, 0x062F, 0x0627, 0x0626, 0x0645, 0x0627, 0x064B, 0x0020, 0x0644, 0x0644, 0x0627, 0x062D, 0x062A,
        0x064A, 0x0627, 0x062C, 0x0627, 0x062A, 0x0020, 0x0627, 0x0644, 0x0645, 0x062A, 0x063A, 0x064A, 0x0631, 0x0629, 0x0020, 0x0644,
        0x0644, 0x0645, 0x062C, 0x062A, 0x0645, 0x0639, 0x0020, 0x0648, 0x0644, 0x0644, 0x062F, 0x0648, 0x0644, 0x0629, 0x002E, 0x0020,
        0x062A, 0x0648, 0x0633, 0x0639, 0x062A, 0x0020, 0x0648, 0x062A, 0x0637, 0x0648, 0x0631, 0x062A, 0x0020, 0x0627, 0x0644, 0x0645,
        0x0624, 0x0633, 0x0633, 0x0629, 0x0020, 0x0628, 0x0647, 0x062F, 0x0641, 0x0020, 0x0636, 0x0645, 0x0627, 0x0646, 0x0020, 0x0634,
        0x0628, 0x0643, 0x0629, 0x0020, 0x0623, 0x0645, 0x0627, 0x0646, 0x0020, 0x0644, 0x0633, 0x0643, 0x0627, 0x0646, 0x0020, 0x062F,
        0x0648, 0x0644, 0x0629, 0x0020, 0x0627, 0x0633, 0x0631, 0x0627, 0x0626, 0x064A, 0x0644, 0x0020, 0x0628, 0x0648, 0x062C, 0x0647,
        0x0020, 0x0627, 0x0644, 0x0645, 0x062E, 0x0627, 0x0637, 0x0631, 0x0020, 0x0627, 0x0644, 0x0627, 0x0642, 0x062A, 0x0635, 0x0627,
        0x062F, 0x064A, 0x0629, 0x0020, 0x0648, 0x0627, 0x0644, 0x0627, 0x062C, 0x062A, 0x0645, 0x0627, 0x0639, 0x064A, 0x0629, 0x002E,
        0x0000
    };
    static const UChar chars_reverse[] = {
        0x002E, 0x0629, 0x064A, 0x0639, 0x0627, 0x0645, 0x062A, 0x062C, 0x0627, 0x0644, 0x0627, 0x0648, 0x0020, 0x0629, 0x064A, 0x062F,
        0x0627, 0x0635, 0x062A, 0x0642, 0x0627, 0x0644, 0x0627, 0x0020, 0x0631, 0x0637, 0x0627, 0x062E, 0x0645, 0x0644, 0x0627, 0x0020,
        0x0647, 0x062C, 0x0648, 0x0628, 0x0020, 0x0644, 0x064A, 0x0626, 0x0627, 0x0631, 0x0633, 0x0627, 0x0020, 0x0629, 0x0644, 0x0648,
        0x062F, 0x0020, 0x0646, 0x0627, 0x0643, 0x0633, 0x0644, 0x0020, 0x0646, 0x0627, 0x0645, 0x0623, 0x0020, 0x0629, 0x0643, 0x0628,
        0x0634, 0x0020, 0x0646, 0x0627, 0x0645, 0x0636, 0x0020, 0x0641, 0x062F, 0x0647, 0x0628, 0x0020, 0x0629, 0x0633, 0x0633, 0x0624,
        0x0645, 0x0644, 0x0627, 0x0020, 0x062A, 0x0631, 0x0648, 0x0637, 0x062A, 0x0648, 0x0020, 0x062A, 0x0639, 0x0633, 0x0648, 0x062A,
        0x0020, 0x002E, 0x0629, 0x0644, 0x0648, 0x062F, 0x0644, 0x0644, 0x0648, 0x0020, 0x0639, 0x0645, 0x062A, 0x062C, 0x0645, 0x0644,
        0x0644, 0x0020, 0x0629, 0x0631, 0x064A, 0x063A, 0x062A, 0x0645, 0x0644, 0x0627, 0x0020, 0x062A, 0x0627, 0x062C, 0x0627, 0x064A,
        0x062A, 0x062D, 0x0627, 0x0644, 0x0644, 0x0020, 0x064B, 0x0627, 0x0645, 0x0626, 0x0627, 0x062F, 0x0020, 0x0627, 0x0647, 0x062A,
        0x0645, 0x0626, 0x0627, 0x0644, 0x0645, 0x0020, 0x0639, 0x0645, 0x0020, 0x002C, 0x064A, 0x0646, 0x0637, 0x0648, 0x0644, 0x0627,
        0x0020, 0x0646, 0x064A, 0x0645, 0x0623, 0x062A, 0x0644, 0x0627, 0x0020, 0x0629, 0x0633, 0x0633, 0x0624, 0x0645, 0x0020, 0x064A,
        0x0641, 0x0020, 0x0629, 0x062F, 0x064A, 0x062F, 0x0639, 0x0020, 0x0646, 0x064A, 0x0645, 0x0623, 0x062A, 0x0020, 0x062C, 0x0645,
        0x0627, 0x0631, 0x0628, 0x0020, 0x062A, 0x0630, 0x0641, 0x064F, 0x0646, 0x0648, 0x0020, 0x062A, 0x0639, 0x0636, 0x064F, 0x0648,
        0x0000,
    };
    
    int32_t bLength = 0, brLength = 0;
    
    UnicodeString s1(chars);
    UnicodeString s2(chars_reverse);
    
    char *bytes = extractBytes(s1, "IBM420", bLength);
    char *bytes_r = extractBytes(s2, "IBM420", brLength);
    
    UCharsetDetector *csd = ucsdet_open(&status);
    if (U_FAILURE(status)) {
        errln("Error opening charset detector. - %s", u_errorName(status));
    }
    const UCharsetMatch *match;
    const char *name;

    ucsdet_setText(csd, bytes, bLength, &status);
    match = ucsdet_detect(csd, &status);

    if (match == NULL) {
        errcheckln(status, "Encoding detection failure for IBM420_rtl: got no matches. - %s", u_errorName(status));
        goto bail;
    }

    name  = ucsdet_getName(match, &status);
    if (strcmp(name, "IBM420_rtl") != 0) {
        errln("Encoding detection failure for IBM420_rtl: got %s\n", name);
    }
    
    ucsdet_setText(csd, bytes_r, brLength, &status);
    match = ucsdet_detect(csd, &status);

    if (match == NULL) {
        errln("Encoding detection failure for IBM420_ltr: got no matches.\n");
        goto bail;
    }

    name  = ucsdet_getName(match, &status);
    if (strcmp(name, "IBM420_ltr") != 0) {
        errln("Encoding detection failure for IBM420_ltr: got %s\n", name);
    }

bail:
    freeBytes(bytes);
    freeBytes(bytes_r);
    ucsdet_close(csd);
}


void CharsetDetectionTest::Ticket6394Test() {
#if !UCONFIG_NO_CONVERSION
    const char charText[] =  "Here is some random English text that should be detected as ISO-8859-1."
                             "Ticket 6394 claims that ISO-8859-1 will appear in the array of detected "
                             "encodings more than once.  The hop through UnicodeString is for platforms " 
                             "where this char * string is be EBCDIC and needs conversion to Latin1.";
    char latin1Text[sizeof(charText)];
    UnicodeString(charText).extract(0, sizeof(charText)-2, latin1Text, sizeof(latin1Text), "ISO-8859-1");

    UErrorCode status = U_ZERO_ERROR;
    UCharsetDetector *csd = ucsdet_open(&status);
    ucsdet_setText(csd, latin1Text, -1, &status);
    if (U_FAILURE(status)) {
        errln("Fail at file %s, line %d.  status = %s", __FILE__, __LINE__, u_errorName(status));
        return;
    }

    int32_t matchCount = 0;
    const UCharsetMatch **matches = ucsdet_detectAll(csd, &matchCount, &status);
    if (U_FAILURE(status)) {
        errln("Fail at file %s, line %d.  status = %s", __FILE__, __LINE__, u_errorName(status));
        return;
    }

    UnicodeSet  setOfCharsetNames;    // UnicodSets can hold strings.
    int32_t i;
    for (i=0; i<matchCount; i++) {
        UnicodeString charSetName(ucsdet_getName(matches[i], &status));
        if (U_FAILURE(status)) {
            errln("Fail at file %s, line %d.  status = %s;  i=%d", __FILE__, __LINE__, u_errorName(status), i);
            status = U_ZERO_ERROR;
        }
        if (setOfCharsetNames.contains(charSetName)) {
            errln("Fail at file %s, line %d ", __FILE__, __LINE__);
            errln(UnicodeString("   Duplicate charset name = ") + charSetName);
        }
        setOfCharsetNames.add(charSetName);
    }
    ucsdet_close(csd);
#endif
}

