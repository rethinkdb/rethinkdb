/*
 *******************************************************************************
 *
 *   Copyright (C) 1999-2007, International Business Machines
 *   Corporation and others.  All Rights Reserved.
 *
 *******************************************************************************
 */

#include "unicode/utypes.h"
#include "unicode/uclean.h"
#include "unicode/uchar.h"
#include "unicode/unistr.h"
#include "unicode/uscript.h"
#include "unicode/putil.h"
#include "unicode/ctest.h"

#include "layout/LETypes.h"
#include "layout/LEScripts.h"

#include "letsutil.h"
#include "letest.h"

#include "xmlreader.h"

#include "xmlparser.h"

#include <stdlib.h>
#include <stdio.h>
#include <string.h>

//U_NAMESPACE_USE

#define CH_COMMA 0x002C

static le_uint32 *getHexArray(const UnicodeString &numbers, int32_t &arraySize)
{
    int32_t offset = -1;

    arraySize = 1;
    while((offset = numbers.indexOf(CH_COMMA, offset + 1)) >= 0) {
        arraySize += 1;
    }

    le_uint32 *array = NEW_ARRAY(le_uint32, arraySize);
    char number[16];
    le_int32 count = 0;
    le_int32 start = 0, end = 0;
    le_int32 len = 0;

    // trim leading whitespace
    while(u_isUWhiteSpace(numbers[start])) {
        start += 1;
    }

    while((end = numbers.indexOf(CH_COMMA, start)) >= 0) {
        len = numbers.extract(start, end - start, number, ARRAY_SIZE(number), US_INV);
        number[len] = '\0';
        start = end + 1;

        sscanf(number, "%x", &array[count++]);

        // trim whitespace following the comma
        while(u_isUWhiteSpace(numbers[start])) {
            start += 1;
        }
    }

    // trim trailing whitespace
    end = numbers.length();
    while(u_isUWhiteSpace(numbers[end - 1])) {
        end -= 1;
    }

    len = numbers.extract(start, end - start, number, ARRAY_SIZE(number), US_INV);
    number[len] = '\0';
    sscanf(number, "%x", &array[count]);

    return array;
}

static float *getFloatArray(const UnicodeString &numbers, int32_t &arraySize)
{
    int32_t offset = -1;

    arraySize = 1;
    while((offset = numbers.indexOf(CH_COMMA, offset + 1)) >= 0) {
        arraySize += 1;
    }

    float *array = NEW_ARRAY(float, arraySize);
    char number[32];
    le_int32 count = 0;
    le_int32 start = 0, end = 0;
    le_int32 len = 0;

    // trim leading whitespace
    while(u_isUWhiteSpace(numbers[start])) {
        start += 1;
    }

    while((end = numbers.indexOf(CH_COMMA, start)) >= 0) {
        len = numbers.extract(start, end - start, number, ARRAY_SIZE(number), US_INV);
        number[len] = '\0';
        start = end + 1;

        sscanf(number, "%f", &array[count++]);

        // trim whiteapce following the comma
        while(u_isUWhiteSpace(numbers[start])) {
            start += 1;
        }
    }

    while(u_isUWhiteSpace(numbers[start])) {
        start += 1;
    }

    // trim trailing whitespace
    end = numbers.length();
    while(u_isUWhiteSpace(numbers[end - 1])) {
        end -= 1;
    }

    len = numbers.extract(start, end - start, number, ARRAY_SIZE(number), US_INV);
    number[len] = '\0';
    sscanf(number, "%f", &array[count]);

    return array;
}

U_CDECL_BEGIN
void readTestFile(const char *testFilePath, TestCaseCallback callback)
{
#if !UCONFIG_NO_REGULAR_EXPRESSIONS
    UErrorCode status = U_ZERO_ERROR;
    UXMLParser  *parser = UXMLParser::createParser(status);
    UXMLElement *root   = parser->parseFile(testFilePath, status);

    if (root == NULL) {
        log_err("Could not open the test data file: %s\n", testFilePath);
        delete parser;
        return;
    }

    UnicodeString test_case        = UNICODE_STRING_SIMPLE("test-case");
    UnicodeString test_text        = UNICODE_STRING_SIMPLE("test-text");
    UnicodeString test_font        = UNICODE_STRING_SIMPLE("test-font");
    UnicodeString result_glyphs    = UNICODE_STRING_SIMPLE("result-glyphs");
    UnicodeString result_indices   = UNICODE_STRING_SIMPLE("result-indices");
    UnicodeString result_positions = UNICODE_STRING_SIMPLE("result-positions");

    // test-case attributes
    UnicodeString id_attr     = UNICODE_STRING_SIMPLE("id");
    UnicodeString script_attr = UNICODE_STRING_SIMPLE("script");
    UnicodeString lang_attr   = UNICODE_STRING_SIMPLE("lang");

    // test-font attributes
    UnicodeString name_attr   = UNICODE_STRING_SIMPLE("name");
    UnicodeString ver_attr    = UNICODE_STRING_SIMPLE("version");
    UnicodeString cksum_attr  = UNICODE_STRING_SIMPLE("checksum");

    const UXMLElement *testCase;
    int32_t tc = 0;

    while((testCase = root->nextChildElement(tc)) != NULL) {
        if (testCase->getTagName().compare(test_case) == 0) {
            char *id = getCString(testCase->getAttribute(id_attr));
            char *script    = getCString(testCase->getAttribute(script_attr));
            char *lang      = getCString(testCase->getAttribute(lang_attr));
            char *fontName  = NULL;
			char *fontVer   = NULL;
			char *fontCksum = NULL;
            const UXMLElement *element;
            int32_t ec = 0;
            int32_t charCount = 0;
            int32_t typoFlags = 3; // kerning + ligatures...
            UScriptCode scriptCode;
            le_int32 languageCode = -1;
            UnicodeString text, glyphs, indices, positions;
            int32_t glyphCount = 0, indexCount = 0, positionCount = 0;
            TestResult expected = {0, NULL, NULL, NULL};

            uscript_getCode(script, &scriptCode, 1, &status);
            if (LE_FAILURE(status)) {
                log_err("invalid script name: %s.\n", script);
                goto free_c_strings;
            }

            if (lang != NULL) {
                languageCode = getLanguageCode(lang);

                if (languageCode < 0) {
                    log_err("invalid language name: %s.\n", lang);
                    goto free_c_strings;
                }
            }

            while((element = testCase->nextChildElement(ec)) != NULL) {
                UnicodeString tag = element->getTagName();

                // TODO: make sure that each element is only used once.
                if (tag.compare(test_font) == 0) {
                    fontName  = getCString(element->getAttribute(name_attr));
                    fontVer   = getCString(element->getAttribute(ver_attr));
                    fontCksum = getCString(element->getAttribute(cksum_attr));

                } else if (tag.compare(test_text) == 0) {
                    text = element->getText(TRUE);
                    charCount = text.length();
                } else if (tag.compare(result_glyphs) == 0) {
                    glyphs = element->getText(TRUE);
                } else if (tag.compare(result_indices) == 0) {
                    indices = element->getText(TRUE);
                } else if (tag.compare(result_positions) == 0) {
                    positions = element->getText(TRUE);
                } else {
                    // an unknown tag...
                    char *cTag = getCString(&tag);

                    log_info("Test %s: unknown element with tag \"%s\"\n", id, cTag);
                    freeCString(cTag);
                }
            }

            expected.glyphs    = (LEGlyphID *) getHexArray(glyphs, glyphCount);
            expected.indices   = (le_int32 *)  getHexArray(indices, indexCount);
            expected.positions = getFloatArray(positions, positionCount);

            expected.glyphCount = glyphCount;

            if (glyphCount < charCount || indexCount != glyphCount || positionCount < glyphCount * 2 + 2) {
                log_err("Test %s: inconsistent input data: charCount = %d, glyphCount = %d, indexCount = %d, positionCount = %d\n",
                    id, charCount, glyphCount, indexCount, positionCount);
                goto free_expected;
            };

			(*callback)(id, fontName, fontVer, fontCksum, scriptCode, languageCode, text.getBuffer(), charCount, &expected);

free_expected:
            DELETE_ARRAY(expected.positions);
            DELETE_ARRAY(expected.indices);
            DELETE_ARRAY(expected.glyphs);

free_c_strings:
			freeCString(fontCksum);
			freeCString(fontVer);
			freeCString(fontName);
            freeCString(lang);
            freeCString(script);
            freeCString(id);
        }
    }

    delete root;
    delete parser;
#endif
}
U_CDECL_END
