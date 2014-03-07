/********************************************************************
 * Copyright (c) 1997-2010, International Business Machines
 * Corporation and others. All Rights Reserved.
 ********************************************************************
 *
 * File UCNVSELTST.C
 *
 * Modification History:
 *        Name                     Description
 *     MOHAMED ELDAWY               Creation
 ********************************************************************
 */

/* C API AND FUNCTIONALITY TEST FOR CONVERTER SELECTOR (ucnvsel.h)*/

#include "ucnvseltst.h"

#include <stdio.h>

#include "unicode/utypes.h"
#include "unicode/ucnvsel.h"
#include "unicode/ustring.h"
#include "cmemory.h"
#include "cstring.h"
#include "propsvec.h"

#define LENGTHOF(array) (int32_t)(sizeof(array)/sizeof((array)[0]))

#define FILENAME_BUFFER 1024

#define TDSRCPATH  ".." U_FILE_SEP_STRING "test" U_FILE_SEP_STRING "testdata" U_FILE_SEP_STRING

static void TestSelector(void);
static void TestUPropsVector(void);
void addCnvSelTest(TestNode** root);  /* Declaration required to suppress compiler warnings. */

void addCnvSelTest(TestNode** root)
{
    addTest(root, &TestSelector, "tsconv/ucnvseltst/TestSelector");
    addTest(root, &TestUPropsVector, "tsconv/ucnvseltst/TestUPropsVector");
}

static const char **gAvailableNames = NULL;
static int32_t gCountAvailable = 0;

static UBool
getAvailableNames() {
  int32_t i;
  if (gAvailableNames != NULL) {
    return TRUE;
  }
  gCountAvailable = ucnv_countAvailable();
  if (gCountAvailable == 0) {
    log_data_err("No converters available.\n");
    return FALSE;
  }
  gAvailableNames = (const char **)uprv_malloc(gCountAvailable * sizeof(const char *));
  if (gAvailableNames == NULL) {
    log_err("unable to allocate memory for %ld available converter names\n",
            (long)gCountAvailable);
    return FALSE;
  }
  for (i = 0; i < gCountAvailable; ++i) {
    gAvailableNames[i] = ucnv_getAvailableName(i);
  }
  return TRUE;
}

static void
releaseAvailableNames() {
  uprv_free((void *)gAvailableNames);
  gAvailableNames = NULL;
  gCountAvailable = 0;
}

static const char **
getEncodings(int32_t start, int32_t step, int32_t count, int32_t *pCount) {
  const char **names;
  int32_t i;

  *pCount = 0;
  if (count <= 0) {
    return NULL;
  }
  names = (const char **)uprv_malloc(count * sizeof(char *));
  if (names == NULL) {
    log_err("memory allocation error for %ld pointers\n", (long)count);
    return NULL;
  }
  if (step == 0 && count > 0) {
    step = 1;
  }
  for (i = 0; i < count; ++i) {
    if (0 <= start && start < gCountAvailable) {
      names[i] = gAvailableNames[start];
      start += step;
      ++*pCount;
    }
  }
  return names;
}

#if 0
/*
 * ucnvsel_open() does not support "no encodings":
 * Given 0 encodings it will open a selector for all available ones.
 */
static const char **
getNoEncodings(int32_t *pCount) {
  *pCount = 0;
  return NULL;
}
#endif

static const char **
getOneEncoding(int32_t *pCount) {
  return getEncodings(1, 0, 1, pCount);
}

static const char **
getFirstEvenEncodings(int32_t *pCount) {
  return getEncodings(0, 2, 25, pCount);
}

static const char **
getMiddleEncodings(int32_t *pCount) {
  return getEncodings(gCountAvailable - 12, 1, 22, pCount);
}

static const char **
getLastEncodings(int32_t *pCount) {
  return getEncodings(gCountAvailable - 1, -1, 25, pCount);
}

static const char **
getSomeEncodings(int32_t *pCount) {
  /* 20 evenly distributed */
  return getEncodings(5, (gCountAvailable + 19)/ 20, 20, pCount);
}

static const char **
getEveryThirdEncoding(int32_t *pCount) {
  return getEncodings(2, 3, (gCountAvailable + 2 )/ 3, pCount);
}

static const char **
getAllEncodings(int32_t *pCount) {
  return getEncodings(0, 1, gCountAvailable, pCount);
}

typedef const char **GetEncodingsFn(int32_t *);

static GetEncodingsFn *const getEncodingsFns[] = {
  getOneEncoding,
  getFirstEvenEncodings,
  getMiddleEncodings,
  getLastEncodings,
  getSomeEncodings,
  getEveryThirdEncoding,
  getAllEncodings
};

static FILE *fopenOrError(const char *filename) {
    int32_t needLen;
    FILE *f;
    char fnbuf[FILENAME_BUFFER];
    const char* directory= ctest_dataSrcDir();
    needLen = uprv_strlen(directory)+uprv_strlen(TDSRCPATH)+uprv_strlen(filename)+1;
    if(needLen > FILENAME_BUFFER) {
        log_err("FAIL: Could not load %s. Filename buffer overflow, needed %d but buffer is %d\n",
                filename, needLen, FILENAME_BUFFER);
        return NULL;
    }

    strcpy(fnbuf, directory);
    strcat(fnbuf, TDSRCPATH);
    strcat(fnbuf, filename);

    f = fopen(fnbuf, "rb");

    if(f == NULL) {
        log_data_err("FAIL: Could not load %s [%s]\n", fnbuf, filename);
    }
    return f;
}

typedef struct TestText {
  char *text, *textLimit;
  char *limit;
  int32_t number;
} TestText;

static void
text_reset(TestText *tt) {
  tt->limit = tt->text;
  tt->number = 0;
}

static char *
text_nextString(TestText *tt, int32_t *pLength) {
  char *s = tt->limit;
  if (s == tt->textLimit) {
    /* we already delivered the last string */
    return NULL;
  } else if (s == tt->text) {
    /* first string */
    if ((tt->textLimit - tt->text) >= 3 &&
        s[0] == (char)0xef && s[1] == (char)0xbb && s[2] == (char)0xbf
    ) {
      s += 3;  /* skip the UTF-8 signature byte sequence (U+FEFF) */
    }
  } else {
    /* skip the string terminator */
    ++s;
    ++tt->number;
  }

  /* find the end of this string */
  tt->limit = uprv_strchr(s, 0);
  *pLength = (int32_t)(tt->limit - s);
  return s;
}

static UBool
text_open(TestText *tt) {
  FILE *f;
  char *s;
  int32_t length;
  uprv_memset(tt, 0, sizeof(TestText));
  f = fopenOrError("ConverterSelectorTestUTF8.txt");
  if(!f) {
    return FALSE;
  }
  fseek(f, 0, SEEK_END);
  length = (int32_t)ftell(f);
  fseek(f, 0, SEEK_SET);
  tt->text = (char *)uprv_malloc(length + 1);
  if (tt->text == NULL) {
    fclose(f);
    return FALSE;
  }
  if (length != fread(tt->text, 1, length, f)) {
    log_err("error reading %ld bytes from test text file\n", (long)length);
    length = 0;
    uprv_free(tt->text);
  }
  fclose(f);
  tt->textLimit = tt->text + length;
  *tt->textLimit = 0;
  /* replace all Unicode '#' (U+0023) with NUL */
  for(s = tt->text; (s = uprv_strchr(s, 0x23)) != NULL; *s++ = 0) {}
  text_reset(tt);
  return TRUE;
}

static void
text_close(TestText *tt) {
  uprv_free(tt->text);
}

static int32_t findIndex(const char* converterName) {
  int32_t i;
  for (i = 0 ; i < gCountAvailable; i++) {
    if(ucnv_compareNames(gAvailableNames[i], converterName) == 0) {
      return i;
    }
  }
  return -1;
}

static UBool *
getResultsManually(const char** encodings, int32_t num_encodings,
                   const char *utf8, int32_t length,
                   const USet* excludedCodePoints, const UConverterUnicodeSet whichSet) {
  UBool* resultsManually;
  int32_t i;

  resultsManually = (UBool*) uprv_malloc(gCountAvailable);
  uprv_memset(resultsManually, 0, gCountAvailable);

  for(i = 0 ; i < num_encodings ; i++) {
    UErrorCode status = U_ZERO_ERROR;
    /* get unicode set for that converter */
    USet* set;
    UConverter* test_converter;
    UChar32 cp;
    int32_t encIndex, offset;

    set = uset_openEmpty();
    test_converter = ucnv_open(encodings[i], &status);
    ucnv_getUnicodeSet(test_converter, set,
                       whichSet, &status);
    if (excludedCodePoints != NULL) {
      uset_addAll(set, excludedCodePoints);
    }
    uset_freeze(set);
    offset = 0;
    cp = 0;

    encIndex = findIndex(encodings[i]);
    /*
     * The following is almost, but not entirely, the same as
     * resultsManually[encIndex] =
     *   (UBool)(uset_spanUTF8(set, utf8, length, USET_SPAN_SIMPLE) == length);
     * They might be different if the set contains strings,
     * or if the utf8 string contains an illegal sequence.
     *
     * The UConverterSelector does not currently handle strings that can be
     * converted, and it treats an illegal sequence as convertible
     * while uset_spanUTF8() treats it like U+FFFD which may not be convertible.
     */
    resultsManually[encIndex] = TRUE;
    while(offset<length) {
      U8_NEXT(utf8, offset, length, cp);
      if (cp >= 0 && !uset_contains(set, cp)) {
        resultsManually[encIndex] = FALSE;
        break;
      }
    }
    uset_close(set);
    ucnv_close(test_converter);
  }
  return resultsManually;
}

/* closes res but does not free resultsManually */
static void verifyResult(UEnumeration* res, const UBool *resultsManually) {
  UBool* resultsFromSystem = (UBool*) uprv_malloc(gCountAvailable * sizeof(UBool));
  const char* name;
  UErrorCode status = U_ZERO_ERROR;
  int32_t i;

  /* fill the bool for the selector results! */
  uprv_memset(resultsFromSystem, 0, gCountAvailable);
  while ((name = uenum_next(res,NULL, &status)) != NULL) {
    resultsFromSystem[findIndex(name)] = TRUE;
  }
  for(i = 0 ; i < gCountAvailable; i++) {
    if(resultsManually[i] != resultsFromSystem[i]) {
      log_err("failure in converter selector\n"
              "converter %s had conflicting results -- manual: %d, system %d\n",
              gAvailableNames[i], resultsManually[i], resultsFromSystem[i]);
    }
  }
  uprv_free(resultsFromSystem);
  uenum_close(res);
}

static UConverterSelector *
serializeAndUnserialize(UConverterSelector *sel, char **buffer, UErrorCode *status) {
  char *new_buffer;
  int32_t ser_len, ser_len2;
  /* preflight */
  ser_len = ucnvsel_serialize(sel, NULL, 0, status);
  if (*status != U_BUFFER_OVERFLOW_ERROR) {
    log_err("ucnvsel_serialize(preflighting) failed: %s\n", u_errorName(*status));
    return sel;
  }
  new_buffer = (char *)uprv_malloc(ser_len);
  *status = U_ZERO_ERROR;
  ser_len2 = ucnvsel_serialize(sel, new_buffer, ser_len, status);
  if (U_FAILURE(*status) || ser_len != ser_len2) {
    log_err("ucnvsel_serialize() failed: %s\n", u_errorName(*status));
    uprv_free(new_buffer);
    return sel;
  }
  ucnvsel_close(sel);
  uprv_free(*buffer);
  *buffer = new_buffer;
  sel = ucnvsel_openFromSerialized(new_buffer, ser_len, status);
  if (U_FAILURE(*status)) {
    log_err("ucnvsel_openFromSerialized() failed: %s\n", u_errorName(*status));
    return NULL;
  }
  return sel;
}

static void TestSelector()
{
  TestText text;
  USet* excluded_sets[3] = { NULL };
  int32_t i, testCaseIdx;

  if (!getAvailableNames()) {
    return;
  }
  if (!text_open(&text)) {
    releaseAvailableNames();;
  }

  excluded_sets[0] = uset_openEmpty();
  for(i = 1 ; i < 3 ; i++) {
    excluded_sets[i] = uset_open(i*30, i*30+500);
  }

  for(testCaseIdx = 0; testCaseIdx < LENGTHOF(getEncodingsFns); testCaseIdx++)
  {
    int32_t excluded_set_id;
    int32_t num_encodings;
    const char **encodings = getEncodingsFns[testCaseIdx](&num_encodings);
    if (getTestOption(QUICK_OPTION) && num_encodings > 25) {
      uprv_free((void *)encodings);
      continue;
    }

    /*
     * for(excluded_set_id = 0 ; excluded_set_id < 3 ; excluded_set_id++)
     *
     * This loop was replaced by the following statement because
     * the loop made the test run longer without adding to the code coverage.
     * The handling of the exclusion set is independent of the
     * set of encodings, so there is no need to test every combination.
     */
    excluded_set_id = testCaseIdx % LENGTHOF(excluded_sets);
    {
      UConverterSelector *sel_rt, *sel_fb;
      char *buffer_fb = NULL;
      UErrorCode status = U_ZERO_ERROR;
      sel_rt = ucnvsel_open(encodings, num_encodings,
                            excluded_sets[excluded_set_id],
                            UCNV_ROUNDTRIP_SET, &status);
      if (num_encodings == gCountAvailable) {
        /* test the special "all converters" parameter values */
        sel_fb = ucnvsel_open(NULL, 0,
                              excluded_sets[excluded_set_id],
                              UCNV_ROUNDTRIP_AND_FALLBACK_SET, &status);
      } else if (uset_isEmpty(excluded_sets[excluded_set_id])) {
        /* test that a NULL set gives the same results as an empty set */
        sel_fb = ucnvsel_open(encodings, num_encodings,
                              NULL,
                              UCNV_ROUNDTRIP_AND_FALLBACK_SET, &status);
      } else {
        sel_fb = ucnvsel_open(encodings, num_encodings,
                              excluded_sets[excluded_set_id],
                              UCNV_ROUNDTRIP_AND_FALLBACK_SET, &status);
      }
      if (U_FAILURE(status)) {
        log_err("ucnv_sel_open(encodings %ld) failed - %s\n", testCaseIdx, u_errorName(status));
        ucnvsel_close(sel_rt);
        uprv_free((void *)encodings);
        continue;
      }

      text_reset(&text);
      for (;;) {
        UBool *manual_rt, *manual_fb;
        static UChar utf16[10000];
        char *s;
        int32_t length8, length16;

        s = text_nextString(&text, &length8);
        if (s == NULL || (getTestOption(QUICK_OPTION) && text.number > 3)) {
          break;
        }

        manual_rt = getResultsManually(encodings, num_encodings,
                                       s, length8,
                                       excluded_sets[excluded_set_id],
                                       UCNV_ROUNDTRIP_SET);
        manual_fb = getResultsManually(encodings, num_encodings,
                                       s, length8,
                                       excluded_sets[excluded_set_id],
                                       UCNV_ROUNDTRIP_AND_FALLBACK_SET);
        /* UTF-8 with length */
        status = U_ZERO_ERROR;
        verifyResult(ucnvsel_selectForUTF8(sel_rt, s, length8, &status), manual_rt);
        verifyResult(ucnvsel_selectForUTF8(sel_fb, s, length8, &status), manual_fb);
        /* UTF-8 NUL-terminated */
        verifyResult(ucnvsel_selectForUTF8(sel_rt, s, -1, &status), manual_rt);
        verifyResult(ucnvsel_selectForUTF8(sel_fb, s, -1, &status), manual_fb);

        u_strFromUTF8(utf16, LENGTHOF(utf16), &length16, s, length8, &status);
        if (U_FAILURE(status)) {
          log_err("error converting the test text (string %ld) to UTF-16 - %s\n",
                  (long)text.number, u_errorName(status));
        } else {
          if (text.number == 0) {
            sel_fb = serializeAndUnserialize(sel_fb, &buffer_fb, &status);
          }
          if (U_SUCCESS(status)) {
            /* UTF-16 with length */
            verifyResult(ucnvsel_selectForString(sel_rt, utf16, length16, &status), manual_rt);
            verifyResult(ucnvsel_selectForString(sel_fb, utf16, length16, &status), manual_fb);
            /* UTF-16 NUL-terminated */
            verifyResult(ucnvsel_selectForString(sel_rt, utf16, -1, &status), manual_rt);
            verifyResult(ucnvsel_selectForString(sel_fb, utf16, -1, &status), manual_fb);
          }
        }

        uprv_free(manual_rt);
        uprv_free(manual_fb);
      }
      ucnvsel_close(sel_rt);
      ucnvsel_close(sel_fb);
      uprv_free(buffer_fb);
    }
    uprv_free((void *)encodings);
  }

  releaseAvailableNames();
  text_close(&text);
  for(i = 0 ; i < 3 ; i++) {
    uset_close(excluded_sets[i]);
  }
}

/* Improve code coverage of UPropsVectors */
static void TestUPropsVector() {
    uint32_t value;
    UErrorCode errorCode = U_ILLEGAL_ARGUMENT_ERROR;
    UPropsVectors *pv = upvec_open(100, &errorCode);
    if (pv != NULL) {
        log_err("Should have returned NULL if UErrorCode is an error.");
        return;
    }
    errorCode = U_ZERO_ERROR;
    pv = upvec_open(-1, &errorCode);
    if (pv != NULL || U_SUCCESS(errorCode)) {
        log_err("Should have returned NULL if column is less than 0.\n");
        return;
    }
    errorCode = U_ZERO_ERROR;
    pv = upvec_open(100, &errorCode);
    if (pv == NULL || U_FAILURE(errorCode)) {
        log_err("Unable to open UPropsVectors.\n");
        return;
    }

    if (upvec_getValue(pv, 0, 1) != 0) {
        log_err("upvec_getValue should return 0.\n");
    }
    if (upvec_getRow(pv, 0, NULL, NULL) == NULL) {
        log_err("upvec_getRow should not return NULL.\n");
    }
    if (upvec_getArray(pv, NULL, NULL) != NULL) {
        log_err("upvec_getArray should return NULL.\n");
    }

    upvec_close(pv);
}
