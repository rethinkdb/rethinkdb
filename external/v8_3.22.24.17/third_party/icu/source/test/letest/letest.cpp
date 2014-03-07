/*
 *******************************************************************************
 *
 *   Copyright (C) 1999-2008, International Business Machines
 *   Corporation and others.  All Rights Reserved.
 *
 *******************************************************************************
 *   file name:  letest.cpp
 *
 *   created on: 11/06/2000
 *   created by: Eric R. Mader
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
#include "layout/LayoutEngine.h"

#include "layout/ParagraphLayout.h"
#include "layout/RunArrays.h"

#include "PortableFontInstance.h"
#include "SimpleFontInstance.h"

#include "letsutil.h"
#include "letest.h"

#include "xmlparser.h"
#include "putilimp.h" // for uprv_getUTCtime()

#include <stdlib.h>
#include <string.h>

U_NAMESPACE_USE

#define CH_COMMA 0x002C

U_CDECL_BEGIN

static void U_CALLCONV ScriptTest(void)
{
    if ((int)scriptCodeCount != (int)USCRIPT_CODE_LIMIT) {
        log_err("ScriptCodes::scriptCodeCount = %d, but UScriptCode::USCRIPT_CODE_LIMIT = %d\n", scriptCodeCount, USCRIPT_CODE_LIMIT);
    }
}

static void U_CALLCONV ParamTest(void)
{
    LEErrorCode status = LE_NO_ERROR;
    SimpleFontInstance *font = new SimpleFontInstance(12, status);
    LayoutEngine *engine = LayoutEngine::layoutEngineFactory(font, arabScriptCode, -1, status);
    LEGlyphID *glyphs    = NULL;
    le_int32  *indices   = NULL;
    float     *positions = NULL;
    le_int32   glyphCount = 0;

    glyphCount = engine->getGlyphCount();
    if (glyphCount != 0) {
        log_err("Calling getGlyphCount() on an empty layout returned %d.\n", glyphCount);
    }

    glyphs    = NEW_ARRAY(LEGlyphID, glyphCount + 10);
    indices   = NEW_ARRAY(le_int32, glyphCount + 10);
    positions = NEW_ARRAY(float, glyphCount + 10);

    engine->getGlyphs(NULL, status);

    if (status != LE_ILLEGAL_ARGUMENT_ERROR) {
        log_err("Calling getGlyphs(NULL, status) did not return LE_ILLEGAL_ARGUMENT_ERROR.\n");
    }

    status = LE_NO_ERROR;
    engine->getGlyphs(glyphs, status);

    if (status != LE_NO_LAYOUT_ERROR) {
        log_err("Calling getGlyphs(glyphs, status) on an empty layout did not return LE_NO_LAYOUT_ERROR.\n");
    }

    status = LE_NO_ERROR;
    engine->getGlyphs(NULL, 0xFF000000L, status);

    if (status != LE_ILLEGAL_ARGUMENT_ERROR) {
        log_err("Calling getGlyphs(NULL, 0xFF000000L, status) did not return LE_ILLEGAL_ARGUMENT_ERROR.\n");
    }

    status = LE_NO_ERROR;
    engine->getGlyphs(glyphs, 0xFF000000L, status);

    if (status != LE_NO_LAYOUT_ERROR) {
        log_err("Calling getGlyphs(glyphs, 0xFF000000L, status) on an empty layout did not return LE_NO_LAYOUT_ERROR.\n");
    }

    status = LE_NO_ERROR;
    engine->getCharIndices(NULL, status);

    if (status != LE_ILLEGAL_ARGUMENT_ERROR) {
        log_err("Calling getCharIndices(NULL, status) did not return LE_ILLEGAL_ARGUMENT_ERROR.\n");
    }

    status = LE_NO_ERROR;
    engine->getCharIndices(indices, status);

    if (status != LE_NO_LAYOUT_ERROR) {
        log_err("Calling getCharIndices(indices, status) on an empty layout did not return LE_NO_LAYOUT_ERROR.\n");
    }

    status = LE_NO_ERROR;
    engine->getCharIndices(NULL, 1024, status);

    if (status != LE_ILLEGAL_ARGUMENT_ERROR) {
        log_err("Calling getCharIndices(NULL, 1024, status) did not return LE_ILLEGAL_ARGUMENT_ERROR.\n");
    }

    status = LE_NO_ERROR;
    engine->getCharIndices(indices, 1024, status);

    if (status != LE_NO_LAYOUT_ERROR) {
        log_err("Calling getCharIndices(indices, 1024, status) on an empty layout did not return LE_NO_LAYOUT_ERROR.\n");
    }

    status = LE_NO_ERROR;
    engine->getGlyphPositions(NULL, status);

    if (status != LE_ILLEGAL_ARGUMENT_ERROR) {
        log_err("Calling getGlyphPositions(NULL, status) did not return LE_ILLEGAL_ARGUMENT_ERROR.\n");
    }

    status = LE_NO_ERROR;
    engine->getGlyphPositions(positions, status);

    if (status != LE_NO_LAYOUT_ERROR) {
        log_err("Calling getGlyphPositions(positions, status) on an empty layout did not return LE_NO_LAYOUT_ERROR.\n");
    }

    DELETE_ARRAY(positions);
    DELETE_ARRAY(indices);
    DELETE_ARRAY(glyphs);

    status = LE_NO_ERROR;
    glyphCount = engine->layoutChars(NULL, 0, 0, 0, FALSE, 0.0, 0.0, status);

    if (status != LE_ILLEGAL_ARGUMENT_ERROR) {
        log_err("Calling layoutChars(NULL, 0, 0, 0, FALSE, 0.0, 0.0, status) did not fail w/ LE_ILLEGAL_ARGUMENT_ERROR.\n");
    }

    LEUnicode chars[] = {
        0x0045, 0x006E, 0x0067, 0x006C, 0x0069, 0x0073, 0x0068, 0x0020, // "English "
        0x0645, 0x0627, 0x0646, 0x062A, 0x0648, 0x0634,                 // MEM ALIF KAF NOON TEH WAW SHEEN
        0x0020, 0x0074, 0x0065, 0x0078, 0x0074, 0x02E                   // " text."
    };

    status = LE_NO_ERROR;
    glyphCount = engine->layoutChars(chars, -1, 6, 20, TRUE, 0.0, 0.0, status);

    if (status != LE_ILLEGAL_ARGUMENT_ERROR) {
        log_err("Calling layoutChars(chars, -1, 6, 20, TRUE, 0.0, 0.0, status) did not fail w/ LE_ILLEGAL_ARGUMENT_ERROR.\n");
    }

    status = LE_NO_ERROR;
    glyphCount = engine->layoutChars(chars, 8, -1, 20, TRUE, 0.0, 0.0, status);

    if (status != LE_ILLEGAL_ARGUMENT_ERROR) {
        log_err("Calling layoutChars(chars, 8, -1, 20, TRUE, 0.0, 0.0, status) did not fail w/ LE_ILLEGAL_ARGUMENT_ERROR.\n");
    }

    status = LE_NO_ERROR;
    glyphCount = engine->layoutChars(chars, 8, 6, -1, TRUE, 0.0, 0.0, status);

    if (status != LE_ILLEGAL_ARGUMENT_ERROR) {
        log_err("Calling layoutChars((chars, 8, 6, -1, TRUE, 0.0, 0.0, status) did not fail w/ LE_ILLEGAL_ARGUMENT_ERROR.\n");
    }

    status = LE_NO_ERROR;
    glyphCount = engine->layoutChars(chars, 8, 6, 10, TRUE, 0.0, 0.0, status);

    if (status != LE_ILLEGAL_ARGUMENT_ERROR) {
        log_err("Calling layoutChars(chars, 8, 6, 10, TRUE, 0.0, 0.0, status) did not fail w/ LE_ILLEGAL_ARGUMENT_ERROR.\n");
    }

    float x = 0.0, y = 0.0;

    status = LE_NO_ERROR;
    glyphCount = engine->layoutChars(chars, 8, 6, 20, TRUE, 0.0, 0.0, status);

    if (LE_FAILURE(status)) {
        log_err("Calling layoutChars(chars, 8, 6, 20, TRUE, 0.0, 0.0, status) failed.\n");
        goto bail;
    }

    engine->getGlyphPosition(-1, x, y, status);

    if (status != LE_INDEX_OUT_OF_BOUNDS_ERROR) {
        log_err("Calling getGlyphPosition(-1, x, y, status) did not fail w/ LE_INDEX_OUT_OF_BOUNDS_ERROR.\n");
    }

    status = LE_NO_ERROR;
    engine->getGlyphPosition(glyphCount + 1, x, y, status);

    if (status != LE_INDEX_OUT_OF_BOUNDS_ERROR) {
        log_err("Calling getGlyphPosition(glyphCount + 1, x, y, status) did not fail w/ LE_INDEX_OUT_OF_BOUNDS_ERROR.\n");
    }

bail:
    delete engine;
    delete font;
}
U_CDECL_END

U_CDECL_BEGIN
static void U_CALLCONV FactoryTest(void)
{
    LEErrorCode status = LE_NO_ERROR;
    SimpleFontInstance *font = new SimpleFontInstance(12, status);
    LayoutEngine *engine = NULL;

    for(le_int32 scriptCode = 0; scriptCode < scriptCodeCount; scriptCode += 1) {
        status = LE_NO_ERROR;
        engine = LayoutEngine::layoutEngineFactory(font, scriptCode, -1, status);

        if (LE_FAILURE(status)) {
            log_err("Could not create a LayoutEngine for script \'%s\'.\n", uscript_getShortName((UScriptCode)scriptCode));
        }

        delete engine;
    }

    delete font;
}
U_CDECL_END

U_CDECL_BEGIN
static void U_CALLCONV AccessTest(void)
{
    LEErrorCode status = LE_NO_ERROR;
    SimpleFontInstance *font = new SimpleFontInstance(12, status);
    LayoutEngine *engine = LayoutEngine::layoutEngineFactory(font, arabScriptCode, -1, status);
    le_int32 glyphCount;
    LEGlyphID glyphs[6], extraBitGlyphs[6];;
    le_int32 biasedIndices[6], indices[6], glyph;
    float positions[6 * 2 + 2];
    LEUnicode chars[] = {
        0x0045, 0x006E, 0x0067, 0x006C, 0x0069, 0x0073, 0x0068, 0x0020, // "English "
        0x0645, 0x0627, 0x0646, 0x062A, 0x0648, 0x0634,                 // MEM ALIF KAF NOON TEH WAW SHEEN
        0x0020, 0x0074, 0x0065, 0x0078, 0x0074, 0x02E                   // " text."
    };

    if (LE_FAILURE(status)) {
        log_err("Could not create LayoutEngine.\n");
        goto bail;
    }

    glyphCount = engine->layoutChars(chars, 8, 6, 20, TRUE, 0.0, 0.0, status);

    if (LE_FAILURE(status) || glyphCount != 6) {
        log_err("layoutChars(chars, 8, 6, 20, TRUE, 0.0, 0.0, status) failed.\n");
        goto bail;
    }

    engine->getGlyphs(glyphs, status);
    engine->getCharIndices(indices, status);
    engine->getGlyphPositions(positions, status);

    if (LE_FAILURE(status)) {
        log_err("Could not get glyph, indices and position arrays.\n");
        goto bail;
    }

    engine->getGlyphs(extraBitGlyphs, 0xFF000000L, status);

    if (LE_FAILURE(status)) {
        log_err("getGlyphs(extraBitGlyphs, 0xFF000000L, status); failed.\n");
    } else {
        for(glyph = 0; glyph < glyphCount; glyph += 1) {
            if (extraBitGlyphs[glyph] != (glyphs[glyph] | 0xFF000000L)) {
                log_err("extraBigGlyphs[%d] != glyphs[%d] | 0xFF000000L: %8X, %8X\n",
                    glyph, glyph, extraBitGlyphs[glyph], glyphs[glyph]);
                break;
            }
        }
    }

    status = LE_NO_ERROR;
    engine->getCharIndices(biasedIndices, 1024, status);

    if (LE_FAILURE(status)) {
        log_err("getCharIndices(biasedIndices, 1024, status) failed.\n");
    } else {
        for (glyph = 0; glyph < glyphCount; glyph += 1) {
            if (biasedIndices[glyph] != (indices[glyph] + 1024)) {
                log_err("biasedIndices[%d] != indices[%d] + 1024: %8X, %8X\n",
                    glyph, glyph, biasedIndices[glyph], indices[glyph]);
                break;
            }
        }
    }

    status = LE_NO_ERROR;
    for (glyph = 0; glyph <= glyphCount; glyph += 1) {
        float x = 0.0, y = 0.0;

        engine->getGlyphPosition(glyph, x, y, status);

        if (LE_FAILURE(status)) {
            log_err("getGlyphPosition(%d, x, y, status) failed.\n", glyph);
            break;
        }

        if (x != positions[glyph*2] || y != positions[glyph*2 + 1]) {
            log_err("getGlyphPosition(%d, x, y, status) returned bad position: (%f, %f) != (%f, %f)\n",
                glyph, x, y, positions[glyph*2], positions[glyph*2 + 1]);
            break;
        }
    }

bail:
    delete engine;
    delete font;
}
U_CDECL_END

le_bool compareResults(const char *testID, TestResult *expected, TestResult *actual)
{
    /* NOTE: we'll stop on the first failure 'cause once there's one error, it may cascade... */
    if (actual->glyphCount != expected->glyphCount) {
        log_err("Test %s: incorrect glyph count: exptected %d, got %d\n",
            testID, expected->glyphCount, actual->glyphCount);
        return FALSE;
    }

    le_int32 i;

    for (i = 0; i < actual->glyphCount; i += 1) {
        if (actual->glyphs[i] != expected->glyphs[i]) {
            log_err("Test %s: incorrect id for glyph %d: expected %4X, got %4X\n",
                testID, i, expected->glyphs[i], actual->glyphs[i]);
            return FALSE;
        }
    }

    for (i = 0; i < actual->glyphCount; i += 1) {
        if (actual->indices[i] != expected->indices[i]) {
            log_err("Test %s: incorrect index for glyph %d: expected %8X, got %8X\n",
                testID, i, expected->indices[i], actual->indices[i]);
            return FALSE;
        }
    }

    for (i = 0; i <= actual->glyphCount; i += 1) {
        double xError = uprv_fabs(actual->positions[i * 2] - expected->positions[i * 2]);

        if (xError > 0.0001) {
            log_err("Test %s: incorrect x position for glyph %d: expected %f, got %f\n",
                testID, i, expected->positions[i * 2], actual->positions[i * 2]);
            return FALSE;
        }

        double yError = uprv_fabs(actual->positions[i * 2 + 1] - expected->positions[i * 2 + 1]);

        if (yError < 0) {
            yError = -yError;
        }

        if (yError > 0.0001) {
            log_err("Test %s: incorrect y position for glyph %d: expected %f, got %f\n",
                testID, i, expected->positions[i * 2 + 1], actual->positions[i * 2 + 1]);
            return FALSE;
        }
    }

    return TRUE;
}

static void checkFontVersion(PortableFontInstance *fontInstance, const char *testVersionString,
                             le_uint32 testChecksum, const char *testID)
{
    le_uint32 fontChecksum = fontInstance->getFontChecksum();

    if (fontChecksum != testChecksum) {
        const char *fontVersionString = fontInstance->getNameString(NAME_VERSION_STRING,
            PLATFORM_MACINTOSH, MACINTOSH_ROMAN, MACINTOSH_ENGLISH);
        const LEUnicode *uFontVersionString = NULL;

            // The standard recommends that the Macintosh Roman/English name string be present, but
            // if it's not, try the Microsoft Unicode/English string.
            if (fontVersionString == NULL) {
                uFontVersionString = fontInstance->getUnicodeNameString(NAME_VERSION_STRING,
                    PLATFORM_MICROSOFT, MICROSOFT_UNICODE_BMP, MICROSOFT_ENGLISH);
            }

        log_info("Test %s: this may not be the same font used to generate the test data.\n", testID);

        if (uFontVersionString != NULL) {
            log_info("Your font's version string is \"%S\"\n", uFontVersionString);
            fontInstance->deleteNameString(uFontVersionString);
        } else {
            log_info("Your font's version string is \"%s\"\n", fontVersionString);
            fontInstance->deleteNameString(fontVersionString);
        }

        log_info("The expected version string is \"%s\"\n", testVersionString);
        log_info("If you see errors, they may be due to the version of the font you're using.\n");
    }
}

/* Returns the path to icu/source/test/testdata/ */
const char *getSourceTestData() {
    const char *srcDataDir = NULL;
#ifdef U_TOPSRCDIR
    srcDataDir = U_TOPSRCDIR U_FILE_SEP_STRING"test"U_FILE_SEP_STRING"testdata"U_FILE_SEP_STRING;
#else
    srcDataDir = ".."U_FILE_SEP_STRING".."U_FILE_SEP_STRING"test"U_FILE_SEP_STRING"testdata"U_FILE_SEP_STRING;
    FILE *f = fopen(".."U_FILE_SEP_STRING".."U_FILE_SEP_STRING"test"U_FILE_SEP_STRING"testdata"U_FILE_SEP_STRING"rbbitst.txt", "r");

    if (f != NULL) {
        /* We're in icu/source/test/letest/ */
        fclose(f);
    } else {
        /* We're in icu/source/test/letest/(Debug|Release) */
        srcDataDir = ".."U_FILE_SEP_STRING".."U_FILE_SEP_STRING".."U_FILE_SEP_STRING"test"U_FILE_SEP_STRING"testdata"U_FILE_SEP_STRING;
    }
#endif

    return srcDataDir;
}

const char *getPath(char buffer[2048], const char *filename) {
    const char *testDataDirectory = getSourceTestData();

    strcpy(buffer, testDataDirectory);
    strcat(buffer, filename);

    return buffer;
}

le_uint32 *getHexArray(const UnicodeString &numbers, int32_t &arraySize)
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

float *getFloatArray(const UnicodeString &numbers, int32_t &arraySize)
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

LEFontInstance *openFont(const char *fontName, const char *checksum, const char *version, const char *testID)
{
    char path[2048];
    PortableFontInstance *font;
    LEErrorCode fontStatus = LE_NO_ERROR;


    font = new PortableFontInstance(getPath(path, fontName), 12, fontStatus);

    if (LE_FAILURE(fontStatus)) {
        log_info("Test %s: can't open font %s - test skipped.\n", testID, fontName);
        delete font;
        return NULL;
    } else {
        le_uint32 cksum = 0;

        sscanf(checksum, "%x", &cksum);

        checkFontVersion(font, version, cksum, testID);
    }

    return font;
}

U_CDECL_BEGIN
static void U_CALLCONV DataDrivenTest(void)
{
#if !UCONFIG_NO_REGULAR_EXPRESSIONS
    UErrorCode status = U_ZERO_ERROR;
    char path[2048];
    const char *testFilePath = getPath(path, "letest.xml");

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
            char *script = getCString(testCase->getAttribute(script_attr));
            char *lang   = getCString(testCase->getAttribute(lang_attr));
            LEFontInstance *font = NULL;
            const UXMLElement *element;
            int32_t ec = 0;
            int32_t charCount = 0;
            int32_t typoFlags = 3; // kerning + ligatures...
            UScriptCode scriptCode;
            le_int32 languageCode = -1;
            UnicodeString text, glyphs, indices, positions;
            int32_t glyphCount = 0, indexCount = 0, positionCount = 0;
            TestResult expected = {0, NULL, NULL, NULL};
            TestResult actual   = {0, NULL, NULL, NULL};
            LEErrorCode success = LE_NO_ERROR;
            LayoutEngine *engine = NULL;

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
                    char *fontName  = getCString(element->getAttribute(name_attr));
                    char *fontVer   = getCString(element->getAttribute(ver_attr));
                    char *fontCksum = getCString(element->getAttribute(cksum_attr));

                    font = openFont(fontName, fontCksum, fontVer, id);
                    freeCString(fontCksum);
                    freeCString(fontVer);
                    freeCString(fontName);

                    if (font == NULL) {
                        // warning message already displayed...
                        goto free_c_strings;
                    }
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

            // TODO: make sure that the font, test-text, result-glyphs, result-indices and result-positions
            // have all been provided
            if (font == NULL) {
                LEErrorCode fontStatus = LE_NO_ERROR;

                font = new SimpleFontInstance(12, fontStatus);
                typoFlags |= 0x80000000L;  // use CharSubstitutionFilter...
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

            engine = LayoutEngine::layoutEngineFactory(font, scriptCode, languageCode, typoFlags, success);

            if (LE_FAILURE(success)) {
                log_err("Test %s: could not create a LayoutEngine.\n", id);
                goto free_expected;
            }

            actual.glyphCount = engine->layoutChars(text.getBuffer(), 0, charCount, charCount, getRTL(text), 0, 0, success);

            actual.glyphs    = NEW_ARRAY(LEGlyphID, actual.glyphCount);
            actual.indices   = NEW_ARRAY(le_int32, actual.glyphCount);
            actual.positions = NEW_ARRAY(float, actual.glyphCount * 2 + 2);

            engine->getGlyphs(actual.glyphs, success);
            engine->getCharIndices(actual.indices, success);
            engine->getGlyphPositions(actual.positions, success);

            compareResults(id, &expected, &actual);

            DELETE_ARRAY(actual.positions);
            DELETE_ARRAY(actual.indices);
            DELETE_ARRAY(actual.glyphs);

            delete engine;

free_expected:
            DELETE_ARRAY(expected.positions);
            DELETE_ARRAY(expected.indices);
            DELETE_ARRAY(expected.glyphs);

            delete font;

free_c_strings:
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

U_CDECL_BEGIN
/*
 * From ticket:5923:
 *
 * Build a paragraph that contains a mixture of left to right and right to left text.
 * Break it into multiple lines and make sure that the glyphToCharMap for run in each
 * line is correct.
 *
 * Note: it might be a good idea to also check the glyphs and positions for each run,
 * that we get the expected number of runs per line and that the line breaks are where
 * we expect them to be. Really, it would be a good idea to make a whole test suite
 * for ParagraphLayout.
 */
static void U_CALLCONV GlyphToCharTest(void)
{
    LEErrorCode status = LE_NO_ERROR;
    LEFontInstance *font;
    FontRuns fontRuns(0);
    ParagraphLayout *paragraphLayout;
    const ParagraphLayout::Line *line;
    /*
     * This is the same text that's in <icu>/source/samples/layout/Sample.txt
     */
    LEUnicode chars[] = {
        /*BOM*/ 0x0054, 0x0068, 0x0065, 0x0020, 0x004c, 0x0061, 0x0079, 
        0x006f, 0x0075, 0x0074, 0x0045, 0x006e, 0x0067, 0x0069, 0x006e, 
        0x0065, 0x0020, 0x0064, 0x006f, 0x0065, 0x0073, 0x0020, 0x0061, 
        0x006c, 0x006c, 0x0020, 0x0074, 0x0068, 0x0065, 0x0020, 0x0077, 
        0x006f, 0x0072, 0x006b, 0x0020, 0x006e, 0x0065, 0x0063, 0x0065, 
        0x0073, 0x0073, 0x0061, 0x0072, 0x0079, 0x0020, 0x0074, 0x006f, 
        0x0020, 0x0064, 0x0069, 0x0073, 0x0070, 0x006c, 0x0061, 0x0079, 
        0x0020, 0x0055, 0x006e, 0x0069, 0x0063, 0x006f, 0x0064, 0x0065, 
        0x0020, 0x0074, 0x0065, 0x0078, 0x0074, 0x0020, 0x0077, 0x0072, 
        0x0069, 0x0074, 0x0074, 0x0065, 0x006e, 0x0020, 0x0069, 0x006e, 
        0x0020, 0x006c, 0x0061, 0x006e, 0x0067, 0x0075, 0x0061, 0x0067, 
        0x0065, 0x0073, 0x0020, 0x0077, 0x0069, 0x0074, 0x0068, 0x0020, 
        0x0063, 0x006f, 0x006d, 0x0070, 0x006c, 0x0065, 0x0078, 0x0020, 
        0x0077, 0x0072, 0x0069, 0x0074, 0x0069, 0x006e, 0x0067, 0x0020, 
        0x0073, 0x0079, 0x0073, 0x0074, 0x0065, 0x006d, 0x0073, 0x0020, 
        0x0073, 0x0075, 0x0063, 0x0068, 0x0020, 0x0061, 0x0073, 0x0020, 
        0x0048, 0x0069, 0x006e, 0x0064, 0x0069, 0x0020, 0x0028, 0x0939, 
        0x093f, 0x0928, 0x094d, 0x0926, 0x0940, 0x0029, 0x0020, 0x0054, 
        0x0068, 0x0061, 0x0069, 0x0020, 0x0028, 0x0e44, 0x0e17, 0x0e22, 
        0x0029, 0x0020, 0x0061, 0x006e, 0x0064, 0x0020, 0x0041, 0x0072, 
        0x0061, 0x0062, 0x0069, 0x0063, 0x0020, 0x0028, 0x0627, 0x0644, 
        0x0639, 0x0631, 0x0628, 0x064a, 0x0629, 0x0029, 0x002e, 0x0020, 
        0x0048, 0x0065, 0x0072, 0x0065, 0x0027, 0x0073, 0x0020, 0x0061, 
        0x0020, 0x0073, 0x0061, 0x006d, 0x0070, 0x006c, 0x0065, 0x0020, 
        0x006f, 0x0066, 0x0020, 0x0073, 0x006f, 0x006d, 0x0065, 0x0020, 
        0x0074, 0x0065, 0x0078, 0x0074, 0x0020, 0x0077, 0x0072, 0x0069, 
        0x0074, 0x0074, 0x0065, 0x006e, 0x0020, 0x0069, 0x006e, 0x0020, 
        0x0053, 0x0061, 0x006e, 0x0073, 0x006b, 0x0072, 0x0069, 0x0074, 
        0x003a, 0x0020, 0x0936, 0x094d, 0x0930, 0x0940, 0x092e, 0x0926, 
        0x094d, 0x0020, 0x092d, 0x0917, 0x0935, 0x0926, 0x094d, 0x0917, 
        0x0940, 0x0924, 0x093e, 0x0020, 0x0905, 0x0927, 0x094d, 0x092f, 
        0x093e, 0x092f, 0x0020, 0x0905, 0x0930, 0x094d, 0x091c, 0x0941, 
        0x0928, 0x0020, 0x0935, 0x093f, 0x0937, 0x093e, 0x0926, 0x0020, 
        0x092f, 0x094b, 0x0917, 0x0020, 0x0927, 0x0943, 0x0924, 0x0930, 
        0x093e, 0x0937, 0x094d, 0x091f, 0x094d, 0x0930, 0x0020, 0x0909, 
        0x0935, 0x093e, 0x091a, 0x0964, 0x0020, 0x0927, 0x0930, 0x094d, 
        0x092e, 0x0915, 0x094d, 0x0937, 0x0947, 0x0924, 0x094d, 0x0930, 
        0x0947, 0x0020, 0x0915, 0x0941, 0x0930, 0x0941, 0x0915, 0x094d, 
        0x0937, 0x0947, 0x0924, 0x094d, 0x0930, 0x0947, 0x0020, 0x0938, 
        0x092e, 0x0935, 0x0947, 0x0924, 0x093e, 0x0020, 0x092f, 0x0941, 
        0x092f, 0x0941, 0x0924, 0x094d, 0x0938, 0x0935, 0x0903, 0x0020, 
        0x092e, 0x093e, 0x092e, 0x0915, 0x093e, 0x0903, 0x0020, 0x092a, 
        0x093e, 0x0923, 0x094d, 0x0921, 0x0935, 0x093e, 0x0936, 0x094d, 
        0x091a, 0x0948, 0x0935, 0x0020, 0x0915, 0x093f, 0x092e, 0x0915, 
        0x0941, 0x0930, 0x094d, 0x0935, 0x0924, 0x0020, 0x0938, 0x0902, 
        0x091c, 0x092f, 0x0020, 0x0048, 0x0065, 0x0072, 0x0065, 0x0027, 
        0x0073, 0x0020, 0x0061, 0x0020, 0x0073, 0x0061, 0x006d, 0x0070, 
        0x006c, 0x0065, 0x0020, 0x006f, 0x0066, 0x0020, 0x0073, 0x006f, 
        0x006d, 0x0065, 0x0020, 0x0074, 0x0065, 0x0078, 0x0074, 0x0020, 
        0x0077, 0x0072, 0x0069, 0x0074, 0x0074, 0x0065, 0x006e, 0x0020, 
        0x0069, 0x006e, 0x0020, 0x0041, 0x0072, 0x0061, 0x0062, 0x0069, 
        0x0063, 0x003a, 0x0020, 0x0623, 0x0633, 0x0627, 0x0633, 0x064b, 
        0x0627, 0x060c, 0x0020, 0x062a, 0x062a, 0x0639, 0x0627, 0x0645, 
        0x0644, 0x0020, 0x0627, 0x0644, 0x062d, 0x0648, 0x0627, 0x0633, 
        0x064a, 0x0628, 0x0020, 0x0641, 0x0642, 0x0637, 0x0020, 0x0645, 
        0x0639, 0x0020, 0x0627, 0x0644, 0x0623, 0x0631, 0x0642, 0x0627, 
        0x0645, 0x060c, 0x0020, 0x0648, 0x062a, 0x0642, 0x0648, 0x0645, 
        0x0020, 0x0628, 0x062a, 0x062e, 0x0632, 0x064a, 0x0646, 0x0020, 
        0x0627, 0x0644, 0x0623, 0x062d, 0x0631, 0x0641, 0x0020, 0x0648, 
        0x0627, 0x0644, 0x0645, 0x062d, 0x0627, 0x0631, 0x0641, 0x0020, 
        0x0627, 0x0644, 0x0623, 0x062e, 0x0631, 0x0649, 0x0020, 0x0628, 
        0x0639, 0x062f, 0x0020, 0x0623, 0x0646, 0x0020, 0x062a, 0x064f, 
        0x0639, 0x0637, 0x064a, 0x0020, 0x0631, 0x0642, 0x0645, 0x0627, 
        0x0020, 0x0645, 0x0639, 0x064a, 0x0646, 0x0627, 0x0020, 0x0644, 
        0x0643, 0x0644, 0x0020, 0x0648, 0x0627, 0x062d, 0x062f, 0x0020, 
        0x0645, 0x0646, 0x0647, 0x0627, 0x002e, 0x0020, 0x0648, 0x0642, 
        0x0628, 0x0644, 0x0020, 0x0627, 0x062e, 0x062a, 0x0631, 0x0627, 
        0x0639, 0x0020, 0x0022, 0x064a, 0x0648, 0x0646, 0x0650, 0x0643, 
        0x0648, 0x062f, 0x0022, 0x060c, 0x0020, 0x0643, 0x0627, 0x0646, 
        0x0020, 0x0647, 0x0646, 0x0627, 0x0643, 0x0020, 0x0645, 0x0626, 
        0x0627, 0x062a, 0x0020, 0x0627, 0x0644, 0x0623, 0x0646, 0x0638, 
        0x0645, 0x0629, 0x0020, 0x0644, 0x0644, 0x062a, 0x0634, 0x0641, 
        0x064a, 0x0631, 0x0020, 0x0648, 0x062a, 0x062e, 0x0635, 0x064a, 
        0x0635, 0x0020, 0x0647, 0x0630, 0x0647, 0x0020, 0x0627, 0x0644, 
        0x0623, 0x0631, 0x0642, 0x0627, 0x0645, 0x0020, 0x0644, 0x0644, 
        0x0645, 0x062d, 0x0627, 0x0631, 0x0641, 0x060c, 0x0020, 0x0648, 
        0x0644, 0x0645, 0x0020, 0x064a, 0x0648, 0x062c, 0x062f, 0x0020, 
        0x0646, 0x0638, 0x0627, 0x0645, 0x0020, 0x062a, 0x0634, 0x0641, 
        0x064a, 0x0631, 0x0020, 0x0648, 0x0627, 0x062d, 0x062f, 0x0020, 
        0x064a, 0x062d, 0x062a, 0x0648, 0x064a, 0x0020, 0x0639, 0x0644, 
        0x0649, 0x0020, 0x062c, 0x0645, 0x064a, 0x0639, 0x0020, 0x0627, 
        0x0644, 0x0645, 0x062d, 0x0627, 0x0631, 0x0641, 0x0020, 0x0627, 
        0x0644, 0x0636, 0x0631, 0x0648, 0x0631, 0x064a, 0x0629, 0x0020, 
        0x0061, 0x006e, 0x0064, 0x0020, 0x0068, 0x0065, 0x0072, 0x0065, 
        0x0027, 0x0073, 0x0020, 0x0061, 0x0020, 0x0073, 0x0061, 0x006d, 
        0x0070, 0x006c, 0x0065, 0x0020, 0x006f, 0x0066, 0x0020, 0x0073, 
        0x006f, 0x006d, 0x0065, 0x0020, 0x0074, 0x0065, 0x0078, 0x0074, 
        0x0020, 0x0077, 0x0072, 0x0069, 0x0074, 0x0074, 0x0065, 0x006e, 
        0x0020, 0x0069, 0x006e, 0x0020, 0x0054, 0x0068, 0x0061, 0x0069, 
        0x003a, 0x0020, 0x0e1a, 0x0e17, 0x0e17, 0x0e35, 0x0e48, 0x0e51, 
        0x0e1e, 0x0e32, 0x0e22, 0x0e38, 0x0e44, 0x0e0b, 0x0e42, 0x0e04, 
        0x0e25, 0x0e19, 0x0e42, 0x0e14, 0x0e42, 0x0e23, 0x0e18, 0x0e35, 
        0x0e2d, 0x0e32, 0x0e28, 0x0e31, 0x0e22, 0x0e2d, 0x0e22, 0x0e39, 
        0x0e48, 0x0e17, 0x0e48, 0x0e32, 0x0e21, 0x0e01, 0x0e25, 0x0e32, 
        0x0e07, 0x0e17, 0x0e38, 0x0e48, 0x0e07, 0x0e43, 0x0e2b, 0x0e0d, 
        0x0e48, 0x0e43, 0x0e19, 0x0e41, 0x0e04, 0x0e19, 0x0e0b, 0x0e31, 
        0x0e2a, 0x0e01, 0x0e31, 0x0e1a, 0x0e25, 0x0e38, 0x0e07, 0x0e40, 
        0x0e2e, 0x0e19, 0x0e23, 0x0e35, 0x0e0a, 0x0e32, 0x0e27, 0x0e44, 
        0x0e23, 0x0e48, 0x0e41, 0x0e25, 0x0e30, 0x0e1b, 0x0e49, 0x0e32, 
        0x0e40, 0x0e2d, 0x0e47, 0x0e21, 0x0e20, 0x0e23, 0x0e23, 0x0e22, 
        0x0e32, 0x0e0a, 0x0e32, 0x0e27, 0x0e44, 0x0e23, 0x0e48, 0x0e1a, 
        0x0e49, 0x0e32, 0x0e19, 0x0e02, 0x0e2d, 0x0e07, 0x0e1e, 0x0e27, 
        0x0e01, 0x0e40, 0x0e02, 0x0e32, 0x0e2b, 0x0e25, 0x0e31, 0x0e07, 
        0x0e40, 0x0e25, 0x0e47, 0x0e01, 0x0e40, 0x0e1e, 0x0e23, 0x0e32, 
        0x0e30, 0x0e44, 0x0e21, 0x0e49, 0x0e2a, 0x0e23, 0x0e49, 0x0e32, 
        0x0e07, 0x0e1a, 0x0e49, 0x0e32, 0x0e19, 0x0e15, 0x0e49, 0x0e2d, 
        0x0e07, 0x0e02, 0x0e19, 0x0e21, 0x0e32, 0x0e14, 0x0e49, 0x0e27, 
        0x0e22, 0x0e40, 0x0e01, 0x0e27, 0x0e35, 0x0e22, 0x0e19, 0x0e40, 
        0x0e1b, 0x0e47, 0x0e19, 0x0e23, 0x0e30, 0x0e22, 0x0e30, 0x0e17, 
        0x0e32, 0x0e07, 0x0e2b, 0x0e25, 0x0e32, 0x0e22, 0x0e44, 0x0e21, 
        0x0e25, 0x0e4c
    };
    le_int32 charCount = LE_ARRAY_SIZE(chars);
    le_int32 charIndex = 0, lineNumber = 1;
    const float lineWidth = 600;

    font = new SimpleFontInstance(12, status);

    if (LE_FAILURE(status)) {
        goto finish;
    }

    fontRuns.add(font, charCount);

    paragraphLayout = new ParagraphLayout(chars, charCount, &fontRuns, NULL, NULL, NULL, 0, FALSE, status);

    if (LE_FAILURE(status)) {
        goto close_font;
    }

    paragraphLayout->reflow();
    while ((line = paragraphLayout->nextLine(lineWidth)) != NULL) {
        le_int32 runCount = line->countRuns();

        for(le_int32 run = 0; run < runCount; run += 1) {
            const ParagraphLayout::VisualRun *visualRun = line->getVisualRun(run);
            le_int32 glyphCount = visualRun->getGlyphCount();
            const le_int32 *glyphToCharMap = visualRun->getGlyphToCharMap();

            if (visualRun->getDirection() == UBIDI_RTL) {
                /*
                 * For a right to left run, make sure that the character indices
                 * increase from the right most glyph to the left most glyph. If
                 * there are any one to many glyph substitutions, we might get several
                 * glyphs in a row with the same character index.
                 */
                for(le_int32 i = glyphCount - 1; i >= 0; i -= 1) {
                    le_int32 ix = glyphToCharMap[i];

                    if (ix != charIndex) {
                        if (ix != charIndex - 1) {
                            log_err("Bad glyph to char index for glyph %d on line %d: expected %d, got %d\n",
                                i, lineNumber, charIndex, ix);
                            goto close_paragraph; // once there's one error, we can't count on anything else...
                        }
                    } else {
                        charIndex += 1;
                    }
                }
            } else {
                /*
                 * We can't just check the order of the character indices
                 * for left to right runs because Indic text might have been
                 * reordered. What we can do is find the minimum and maximum
                 * character indices in the run and make sure that the minimum
                 * is equal to charIndex and then advance charIndex to the maximum.
                 */
                le_int32 minIndex = 0x7FFFFFFF, maxIndex = -1;

                for(le_int32 i = 0; i < glyphCount; i += 1) {
                    le_int32 ix = glyphToCharMap[i];

                    if (ix > maxIndex) {
                        maxIndex = ix;
                    }

                    if (ix < minIndex) {
                        minIndex = ix;
                    }
                }

                if (minIndex != charIndex) {
                    log_err("Bad minIndex for run %d on line %d: expected %d, got %d\n",
                        run, lineNumber, charIndex, minIndex);
                    goto close_paragraph; // once there's one error, we can't count on anything else...
                }

                charIndex = maxIndex + 1;
            }
        }

        lineNumber += 1;
    }
close_paragraph:
    delete paragraphLayout;

close_font:
    delete font;

finish:
    return;
}
U_CDECL_END

static void addAllTests(TestNode **root)
{
    addTest(root, &ScriptTest,      "api/ScriptTest");
    addTest(root, &ParamTest,       "api/ParameterTest");
    addTest(root, &FactoryTest,     "api/FactoryTest");
    addTest(root, &AccessTest,      "layout/AccessTest");
    addTest(root, &DataDrivenTest,  "layout/DataDrivenTest");
    addTest(root, &GlyphToCharTest, "paragraph/GlyphToCharTest");

    addCTests(root);
}

/* returns the path to icu/source/data/out */
static const char *ctest_dataOutDir()
{
    static const char *dataOutDir = NULL;

    if(dataOutDir) {
        return dataOutDir;
    }

    /* U_TOPBUILDDIR is set by the makefiles on UNIXes when building cintltst and intltst
    //              to point to the top of the build hierarchy, which may or
    //              may not be the same as the source directory, depending on
    //              the configure options used.  At any rate,
    //              set the data path to the built data from this directory.
    //              The value is complete with quotes, so it can be used
    //              as-is as a string constant.
    */
#if defined (U_TOPBUILDDIR)
    {
        dataOutDir = U_TOPBUILDDIR "data"U_FILE_SEP_STRING"out"U_FILE_SEP_STRING;
    }
#else

    /* On Windows, the file name obtained from __FILE__ includes a full path.
     *             This file is "wherever\icu\source\test\cintltst\cintltst.c"
     *             Change to    "wherever\icu\source\data"
     */
    {
        static char p[sizeof(__FILE__) + 20];
        char *pBackSlash;
        int i;

        strcpy(p, __FILE__);
        /* We want to back over three '\' chars.                            */
        /*   Only Windows should end up here, so looking for '\' is safe.   */
        for (i=1; i<=3; i++) {
            pBackSlash = strrchr(p, U_FILE_SEP_CHAR);
            if (pBackSlash != NULL) {
                *pBackSlash = 0;        /* Truncate the string at the '\'   */
            }
        }

        if (pBackSlash != NULL) {
            /* We found and truncated three names from the path.
             *  Now append "source\data" and set the environment
             */
            strcpy(pBackSlash, U_FILE_SEP_STRING "data" U_FILE_SEP_STRING "out" U_FILE_SEP_STRING);
            dataOutDir = p;
        }
        else {
            /* __FILE__ on MSVC7 does not contain the directory */
            FILE *file = fopen(".."U_FILE_SEP_STRING".."U_FILE_SEP_STRING "data" U_FILE_SEP_STRING "Makefile.in", "r");
            if (file) {
                fclose(file);
                dataOutDir = ".."U_FILE_SEP_STRING".."U_FILE_SEP_STRING "data" U_FILE_SEP_STRING "out" U_FILE_SEP_STRING;
            }
            else {
                dataOutDir = ".."U_FILE_SEP_STRING".."U_FILE_SEP_STRING".."U_FILE_SEP_STRING "data" U_FILE_SEP_STRING "out" U_FILE_SEP_STRING;
            }
        }
    }
#endif

    return dataOutDir;
}

/*  ctest_setICU_DATA  - if the ICU_DATA environment variable is not already
 *                       set, try to deduce the directory in which ICU was built,
 *                       and set ICU_DATA to "icu/source/data" in that location.
 *                       The intent is to allow the tests to have a good chance
 *                       of running without requiring that the user manually set
 *                       ICU_DATA.  Common data isn't a problem, since it is
 *                       picked up via a static (build time) reference, but the
 *                       tests dynamically load some data.
 */
static void ctest_setICU_DATA() {

    /* No location for the data dir was identifiable.
     *   Add other fallbacks for the test data location here if the need arises
     */
    if (getenv("ICU_DATA") == NULL) {
        /* If ICU_DATA isn't set, set it to the usual location */
        u_setDataDirectory(ctest_dataOutDir());
    }
}

int main(int argc, char* argv[])
{
    int32_t nerrors = 0;
    TestNode *root = NULL;
    UErrorCode errorCode = U_ZERO_ERROR;
    UDate startTime, endTime;
    int32_t diffTime;

    startTime = uprv_getUTCtime();

    if (!initArgs(argc, argv, NULL, NULL)) {
        /* Error already displayed. */
        return -1;
    }

    /* Check whether ICU will initialize without forcing the build data directory into
    *  the ICU_DATA path.  Success here means either the data dll contains data, or that
    *  this test program was run with ICU_DATA set externally.  Failure of this check
    *  is normal when ICU data is not packaged into a shared library.
    *
    *  Whether or not this test succeeds, we want to cleanup and reinitialize
    *  with a data path so that data loading from individual files can be tested.
    */
    u_init(&errorCode);

    if (U_FAILURE(errorCode)) {
        fprintf(stderr,
            "#### Note:  ICU Init without build-specific setDataDirectory() failed.\n");
    }

    u_cleanup();
    errorCode = U_ZERO_ERROR;

    if (!initArgs(argc, argv, NULL, NULL)) {
        /* Error already displayed. */
        return -1;
    }
/* Initialize ICU */
    ctest_setICU_DATA();    /* u_setDataDirectory() must happen Before u_init() */
    u_init(&errorCode);

    if (U_FAILURE(errorCode)) {
        fprintf(stderr,
            "#### ERROR! %s: u_init() failed with status = \"%s\".\n" 
            "*** Check the ICU_DATA environment variable and \n"
            "*** check that the data files are present.\n", argv[0], u_errorName(errorCode));
        return 1;
    }

    addAllTests(&root);
    nerrors = runTestRequest(root, argc, argv);

    cleanUpTestTree(root);
    u_cleanup();

    endTime = uprv_getUTCtime();
    diffTime = (int32_t)(endTime - startTime);
    printf("Elapsed Time: %02d:%02d:%02d.%03d\n",
        (int)((diffTime%U_MILLIS_PER_DAY)/U_MILLIS_PER_HOUR),
        (int)((diffTime%U_MILLIS_PER_HOUR)/U_MILLIS_PER_MINUTE),
        (int)((diffTime%U_MILLIS_PER_MINUTE)/U_MILLIS_PER_SECOND),
        (int)(diffTime%U_MILLIS_PER_SECOND));

    return nerrors;
}

