/*
 *******************************************************************************
 *
 *   Copyright (C) 1999-2008, International Business Machines
 *   Corporation and others.  All Rights Reserved.
 *
 *******************************************************************************
 */

#include "unicode/utypes.h"
#include "unicode/ubidi.h"
#include "unicode/uscript.h"
#include "unicode/ctest.h"

#include "layout/LETypes.h"
#include "layout/LEScripts.h"
#include "layout/loengine.h"

#include "layout/playout.h"
#include "layout/plruns.h"

#include "cfonts.h"

#include "letest.h"

#include "sfnt.h"
#include "xmlreader.h"
#include "putilimp.h" /* for U_FILE_SEP_STRING */

#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#define CH_COMMA 0x002C

U_CDECL_BEGIN
static void U_CALLCONV ParamTest(void)
{
    LEErrorCode status = LE_NO_ERROR;
    le_font *font = le_simpleFontOpen(12, &status);
    le_engine *engine = le_create(font, arabScriptCode, -1, 0, &status);
    LEGlyphID *glyphs    = NULL;
    le_int32  *indices   = NULL;
    float     *positions = NULL;
    le_int32   glyphCount = 0;

    float x = 0.0, y = 0.0;
	LEUnicode chars[] = {
	  0x0045, 0x006E, 0x0067, 0x006C, 0x0069, 0x0073, 0x0068, 0x0020, /* "English "                      */
	  0x0645, 0x0627, 0x0646, 0x062A, 0x0648, 0x0634,                 /* MEM ALIF KAF NOON TEH WAW SHEEN */
	  0x0020, 0x0074, 0x0065, 0x0078, 0x0074, 0x02E                   /* " text."                        */
    };


    glyphCount = le_getGlyphCount(engine, &status);
    if (glyphCount != 0) {
        log_err("Calling getGlyphCount() on an empty layout returned %d.\n", glyphCount);
    }

    glyphs    = NEW_ARRAY(LEGlyphID, glyphCount + 10);
    indices   = NEW_ARRAY(le_int32, glyphCount + 10);
    positions = NEW_ARRAY(float, glyphCount + 10);

    le_getGlyphs(engine, NULL, &status);

    if (status != LE_ILLEGAL_ARGUMENT_ERROR) {
        log_err("Calling getGlyphs(NULL, status) did not return LE_ILLEGAL_ARGUMENT_ERROR.\n");
    }

    status = LE_NO_ERROR;
    le_getGlyphs(engine, glyphs, &status);

    if (status != LE_NO_LAYOUT_ERROR) {
        log_err("Calling getGlyphs(glyphs, status) on an empty layout did not return LE_NO_LAYOUT_ERROR.\n");
    }

    status = LE_NO_ERROR;
    le_getCharIndices(engine, NULL, &status);

    if (status != LE_ILLEGAL_ARGUMENT_ERROR) {
        log_err("Calling getCharIndices(NULL, status) did not return LE_ILLEGAL_ARGUMENT_ERROR.\n");
    }

    status = LE_NO_ERROR;
    le_getCharIndices(engine, indices, &status);

    if (status != LE_NO_LAYOUT_ERROR) {
        log_err("Calling getCharIndices(indices, status) on an empty layout did not return LE_NO_LAYOUT_ERROR.\n");
    }

    status = LE_NO_ERROR;
    le_getCharIndicesWithBase(engine, NULL, 1024, &status);

    if (status != LE_ILLEGAL_ARGUMENT_ERROR) {
        log_err("Calling getCharIndices(NULL, 1024, status) did not return LE_ILLEGAL_ARGUMENT_ERROR.\n");
    }

    status = LE_NO_ERROR;
    le_getCharIndicesWithBase(engine, indices, 1024, &status);

    if (status != LE_NO_LAYOUT_ERROR) {
        log_err("Calling getCharIndices(indices, 1024, status) on an empty layout did not return LE_NO_LAYOUT_ERROR.\n");
    }

    status = LE_NO_ERROR;
    le_getGlyphPositions(engine, NULL, &status);

    if (status != LE_ILLEGAL_ARGUMENT_ERROR) {
        log_err("Calling getGlyphPositions(NULL, status) did not return LE_ILLEGAL_ARGUMENT_ERROR.\n");
    }

    status = LE_NO_ERROR;
    le_getGlyphPositions(engine, positions, &status);

    if (status != LE_NO_LAYOUT_ERROR) {
        log_err("Calling getGlyphPositions(positions, status) on an empty layout did not return LE_NO_LAYOUT_ERROR.\n");
    }

    DELETE_ARRAY(positions);
    DELETE_ARRAY(indices);
    DELETE_ARRAY(glyphs);

    status = LE_NO_ERROR;
    glyphCount = le_layoutChars(engine, NULL, 0, 0, 0, FALSE, 0.0, 0.0, &status);

    if (status != LE_ILLEGAL_ARGUMENT_ERROR) {
        log_err("Calling layoutChars(NULL, 0, 0, 0, FALSE, 0.0, 0.0, status) did not fail w/ LE_ILLEGAL_ARGUMENT_ERROR.\n");
    }

    status = LE_NO_ERROR;
    glyphCount = le_layoutChars(engine, chars, -1, 6, 20, TRUE, 0.0, 0.0, &status);

    if (status != LE_ILLEGAL_ARGUMENT_ERROR) {
        log_err("Calling layoutChars(chars, -1, 6, 20, TRUE, 0.0, 0.0, status) did not fail w/ LE_ILLEGAL_ARGUMENT_ERROR.\n");
    }

    status = LE_NO_ERROR;
    glyphCount = le_layoutChars(engine, chars, 8, -1, 20, TRUE, 0.0, 0.0, &status);

    if (status != LE_ILLEGAL_ARGUMENT_ERROR) {
        log_err("Calling layoutChars(chars, 8, -1, 20, TRUE, 0.0, 0.0, status) did not fail w/ LE_ILLEGAL_ARGUMENT_ERROR.\n");
    }

    status = LE_NO_ERROR;
    glyphCount = le_layoutChars(engine, chars, 8, 6, -1, TRUE, 0.0, 0.0, &status);

    if (status != LE_ILLEGAL_ARGUMENT_ERROR) {
        log_err("Calling layoutChars((chars, 8, 6, -1, TRUE, 0.0, 0.0, status) did not fail w/ LE_ILLEGAL_ARGUMENT_ERROR.\n");
    }

    status = LE_NO_ERROR;
    glyphCount = le_layoutChars(engine, chars, 8, 6, 10, TRUE, 0.0, 0.0, &status);

    if (status != LE_ILLEGAL_ARGUMENT_ERROR) {
        log_err("Calling layoutChars(chars, 8, 6, 10, TRUE, 0.0, 0.0, status) did not fail w/ LE_ILLEGAL_ARGUMENT_ERROR.\n");
    }

    status = LE_NO_ERROR;
    glyphCount = le_layoutChars(engine, chars, 8, 6, 20, TRUE, 0.0, 0.0, &status);

    if (LE_FAILURE(status)) {
        log_err("Calling layoutChars(chars, 8, 6, 20, TRUE, 0.0, 0.0, status) failed.\n");
        goto bail;
    }

    le_getGlyphPosition(engine, -1, &x, &y, &status);

    if (status != LE_INDEX_OUT_OF_BOUNDS_ERROR) {
        log_err("Calling getGlyphPosition(-1, x, y, status) did not fail w/ LE_INDEX_OUT_OF_BOUNDS_ERROR.\n");
    }

    status = LE_NO_ERROR;
    le_getGlyphPosition(engine, glyphCount + 1, &x, &y, &status);

    if (status != LE_INDEX_OUT_OF_BOUNDS_ERROR) {
        log_err("Calling getGlyphPosition(glyphCount + 1, x, y, status) did not fail w/ LE_INDEX_OUT_OF_BOUNDS_ERROR.\n");
    }

bail:
    le_close(engine);
    le_fontClose(font);
}
U_CDECL_END

U_CDECL_BEGIN
static void U_CALLCONV FactoryTest(void)
{
    LEErrorCode status = LE_NO_ERROR;
    le_font *font = le_simpleFontOpen(12, &status);
    le_engine *engine = NULL;
	le_int32 scriptCode;

    for(scriptCode = 0; scriptCode < scriptCodeCount; scriptCode += 1) {
        status = LE_NO_ERROR;
        engine = le_create(font, scriptCode, -1, 0, &status);

        if (LE_FAILURE(status)) {
            log_err("Could not create a LayoutEngine for script \'%s\'.\n", uscript_getShortName((UScriptCode)scriptCode));
        }

        le_close(engine);
    }

    le_fontClose(font);
}
U_CDECL_END

U_CDECL_BEGIN
static void U_CALLCONV AccessTest(void)
{
    LEErrorCode status = LE_NO_ERROR;
    le_font *font = le_simpleFontOpen(12, &status);
    le_engine *engine =le_create(font, arabScriptCode, -1, 0, &status);
    le_int32 glyphCount;
    LEGlyphID glyphs[6];
    le_int32 biasedIndices[6], indices[6], glyph;
    float positions[6 * 2 + 2];
    LEUnicode chars[] = {
      0x0045, 0x006E, 0x0067, 0x006C, 0x0069, 0x0073, 0x0068, 0x0020, /* "English "                      */
      0x0645, 0x0627, 0x0646, 0x062A, 0x0648, 0x0634,                 /* MEM ALIF KAF NOON TEH WAW SHEEN */
      0x0020, 0x0074, 0x0065, 0x0078, 0x0074, 0x02E                   /* " text."                        */
    };

    if (LE_FAILURE(status)) {
        log_err("Could not create LayoutEngine.\n");
        goto bail;
    }

    glyphCount = le_layoutChars(engine, chars, 8, 6, 20, TRUE, 0.0, 0.0, &status);

    if (LE_FAILURE(status) || glyphCount != 6) {
        log_err("layoutChars(chars, 8, 6, 20, TRUE, 0.0, 0.0, status) failed.\n");
        goto bail;
    }

    le_getGlyphs(engine, glyphs, &status);
    le_getCharIndices(engine, indices, &status);
    le_getGlyphPositions(engine, positions, &status);

    if (LE_FAILURE(status)) {
        log_err("Could not get glyph, indices and position arrays.\n");
        goto bail;
    }

    status = LE_NO_ERROR;
    le_getCharIndicesWithBase(engine, biasedIndices, 1024, &status);

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

        le_getGlyphPosition(engine, glyph, &x, &y, &status);

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
    le_close(engine);
    le_fontClose(font);
}
U_CDECL_END

static le_bool compareResults(const char *testID, TestResult *expected, TestResult *actual)
{
    le_int32 i;

    /* NOTE: we'll stop on the first failure 'cause once there's one error, it may cascade... */
    if (actual->glyphCount != expected->glyphCount) {
        log_err("Test %s: incorrect glyph count: exptected %d, got %d\n",
            testID, expected->glyphCount, actual->glyphCount);
        return FALSE;
    }

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
        double yError = uprv_fabs(actual->positions[i * 2 + 1] - expected->positions[i * 2 + 1]);

        if (xError > 0.0001) {
            log_err("Test %s: incorrect x position for glyph %d: expected %f, got %f\n",
                testID, i, expected->positions[i * 2], actual->positions[i * 2]);
            return FALSE;
        }

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

static void checkFontVersion(le_font *font, const char *testVersionString,
                             le_uint32 testChecksum, const char *testID)
{
    le_uint32 fontChecksum = le_getFontChecksum(font);

    if (fontChecksum != testChecksum) {
        const char *fontVersionString = le_getNameString(font, NAME_VERSION_STRING,
            PLATFORM_MACINTOSH, MACINTOSH_ROMAN, MACINTOSH_ENGLISH);
        const LEUnicode16 *uFontVersionString = NULL;

        if (fontVersionString == NULL) {
            uFontVersionString = le_getUnicodeNameString(font, NAME_VERSION_STRING,
                PLATFORM_MICROSOFT, MICROSOFT_UNICODE_BMP, MICROSOFT_ENGLISH);
        }

        log_info("Test %s: this may not be the same font used to generate the test data.\n", testID);

        if (uFontVersionString != NULL) {
            log_info("Your font's version string is \"%S\"\n", uFontVersionString);
            le_deleteUnicodeNameString(font, uFontVersionString);
        } else {
            log_info("Your font's version string is \"%s\"\n", fontVersionString);
            le_deleteNameString(font, fontVersionString);
        }

        log_info("The expected version string is \"%s\"\n", testVersionString);
        log_info("If you see errors, they may be due to the version of the font you're using.\n");
    }
}

/* Returns the path to icu/source/test/testdata/ */
static const char *getSourceTestData() {
#ifdef U_TOPSRCDIR
    const char *srcDataDir = U_TOPSRCDIR U_FILE_SEP_STRING"test"U_FILE_SEP_STRING"testdata"U_FILE_SEP_STRING;
#else
    const char *srcDataDir = ".."U_FILE_SEP_STRING".."U_FILE_SEP_STRING"test"U_FILE_SEP_STRING"testdata"U_FILE_SEP_STRING;
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

static const char *getPath(char buffer[2048], const char *filename) {
    const char *testDataDirectory = getSourceTestData();

    strcpy(buffer, testDataDirectory);
    strcat(buffer, filename);

    return buffer;
}

static le_font *openFont(const char *fontName, const char *checksum, const char *version, const char *testID)
{
    char path[2048];
    le_font *font;
    LEErrorCode fontStatus = LE_NO_ERROR;

	if (fontName != NULL) {
		font = le_portableFontOpen(getPath(path, fontName), 12, &fontStatus);

		if (LE_FAILURE(fontStatus)) {
			log_info("Test %s: can't open font %s - test skipped.\n", testID, fontName);
			le_fontClose(font);
			return NULL;
		} else {
			le_uint32 cksum = 0;

			sscanf(checksum, "%x", &cksum);

			checkFontVersion(font, version, cksum, testID);
		}
	} else {
		font = le_simpleFontOpen(12, &fontStatus);
	}

    return font;
}

static le_bool getRTL(const LEUnicode *text, le_int32 charCount)
{
    UBiDiLevel level;
    le_int32 limit = -1;
    UErrorCode status = U_ZERO_ERROR;
    UBiDi *ubidi = ubidi_openSized(charCount, 0, &status);

    ubidi_setPara(ubidi, text, charCount, UBIDI_DEFAULT_LTR, NULL, &status);
    
    /* TODO: Should check that there's only a single logical run... */
    ubidi_getLogicalRun(ubidi, 0, &limit, &level);

    ubidi_close(ubidi);

    return level & 1;
}

static void doTestCase (const char *testID,
				 const char *fontName,
				 const char *fontVersion,
				 const char *fontChecksum,
				 le_int32 scriptCode,
				 le_int32 languageCode,
				 const LEUnicode *text,
				 le_int32 charCount,
				 TestResult *expected)
{
	LEErrorCode status = LE_NO_ERROR;
	le_engine *engine;
	le_font *font = openFont(fontName, fontChecksum, fontVersion, testID);
	le_int32 typoFlags = 3; /* kerning + ligatures */
	TestResult actual;

	if (font == NULL) {
		/* error message already printed. */
		return;
	}

	if (fontName == NULL) {
		typoFlags |= 0x80000000L;  /* use CharSubstitutionFilter... */
	}

    engine = le_create(font, scriptCode, languageCode, typoFlags, &status);

    if (LE_FAILURE(status)) {
        log_err("Test %s: could not create a LayoutEngine.\n", testID);
        goto free_expected;
    }

    actual.glyphCount = le_layoutChars(engine, text, 0, charCount, charCount, getRTL(text, charCount), 0, 0, &status);

    actual.glyphs    = NEW_ARRAY(LEGlyphID, actual.glyphCount);
    actual.indices   = NEW_ARRAY(le_int32, actual.glyphCount);
    actual.positions = NEW_ARRAY(float, actual.glyphCount * 2 + 2);

    le_getGlyphs(engine, actual.glyphs, &status);
    le_getCharIndices(engine, actual.indices, &status);
    le_getGlyphPositions(engine, actual.positions, &status);

    compareResults(testID, expected, &actual);

    DELETE_ARRAY(actual.positions);
    DELETE_ARRAY(actual.indices);
    DELETE_ARRAY(actual.glyphs);

    le_close(engine);

free_expected:
    le_fontClose(font);
}

static void U_CALLCONV DataDrivenTest(void)
{
    char path[2048];
    const char *testFilePath = getPath(path, "letest.xml");

	readTestFile(testFilePath, doTestCase);
}

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
 * for pl_paragraph.
 */
static void U_CALLCONV GlyphToCharTest(void)
{
    LEErrorCode status = LE_NO_ERROR;
    le_font *font;
    pl_fontRuns *fontRuns;
    pl_paragraph *paragraph;
    const pl_line *line;
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
    le_int32 run, i;
    const float lineWidth = 600;

    font = le_simpleFontOpen(12, &status);

    if (LE_FAILURE(status)) {
        log_err("le_simpleFontOpen(12, &status) failed");
        goto finish;
    }

    fontRuns = pl_openEmptyFontRuns(0);
    pl_addFontRun(fontRuns, font, charCount);

    paragraph = pl_create(chars, charCount, fontRuns, NULL, NULL, NULL, 0, FALSE, &status);

    pl_closeFontRuns(fontRuns);

    if (LE_FAILURE(status)) {
        log_err("pl_create failed.");
        goto close_font;
    }

    pl_reflow(paragraph);
    while ((line = pl_nextLine(paragraph, lineWidth)) != NULL) {
        le_int32 runCount = pl_countLineRuns(line);

        for(run = 0; run < runCount; run += 1) {
            const pl_visualRun *visualRun = pl_getLineVisualRun(line, run);
            const le_int32 glyphCount = pl_getVisualRunGlyphCount(visualRun);
            const le_int32 *glyphToCharMap = pl_getVisualRunGlyphToCharMap(visualRun);

            if (pl_getVisualRunDirection(visualRun) == UBIDI_RTL) {
                /*
                 * For a right to left run, make sure that the character indices
                 * increase from the right most glyph to the left most glyph. If
                 * there are any one to many glyph substitutions, we might get several
                 * glyphs in a row with the same character index.
                 */
                for(i = glyphCount - 1; i >= 0; i -= 1) {
                    le_int32 ix = glyphToCharMap[i];

                    if (ix != charIndex) {
                        if (ix != charIndex - 1) {
                            log_err("Bad glyph to char index for glyph %d on line %d: expected %d, got %d\n",
                                i, lineNumber, charIndex, ix);
                            goto close_paragraph; /* once there's one error, we can't count on anything else... */
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

                for(i = 0; i < glyphCount; i += 1) {
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
                    goto close_paragraph; /* once there's one error, we can't count on anything else... */
                }

                charIndex = maxIndex + 1;
            }
        }

        lineNumber += 1;
    }

close_paragraph:
    pl_close(paragraph);

close_font:
    le_fontClose(font);

finish:
    return;
}

U_CFUNC void addCTests(TestNode **root)
{
    addTest(root, &ParamTest,       "c_api/ParameterTest");
    addTest(root, &FactoryTest,     "c_api/FactoryTest");
    addTest(root, &AccessTest,      "c_layout/AccessTest");
    addTest(root, &DataDrivenTest,  "c_layout/DataDrivenTest");
    addTest(root, &GlyphToCharTest, "c_paragraph/GlyphToCharTest");
}


