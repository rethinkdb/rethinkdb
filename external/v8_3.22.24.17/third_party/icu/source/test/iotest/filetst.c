/*
 **********************************************************************
 *   Copyright (C) 2004-2010, International Business Machines
 *   Corporation and others.  All Rights Reserved.
 **********************************************************************
 *   file name:  filetst.c
 *   encoding:   US-ASCII
 *   tab size:   8 (not used)
 *   indentation:4
 *
 *   created on: 2004apr06
 *   created by: George Rhoten
 */

#include "iotest.h"
#include "unicode/ustdio.h"
#include "unicode/ustring.h"
#include "unicode/uloc.h"

#include <string.h>
#include <stdlib.h>

const char *STANDARD_TEST_FILE = "iotest-c.txt";


#if !UCONFIG_NO_FORMATTING
static void TestFileFromICU(UFILE *myFile) {
    int32_t n[1];
    float myFloat = -1234.0;
    int32_t newValuePtr[1];
    double newDoubleValuePtr[1];
    UChar myUString[256];
    UChar uStringBuf[256];
    char myString[256] = "";
    char testBuf[256] = "";
    void *origPtr, *ptr;
    U_STRING_DECL(myStringOrig, "My-String", 9);

    U_STRING_INIT(myStringOrig, "My-String", 9);
    u_memset(myUString, 0x2a, sizeof(myUString)/sizeof(*myUString));
    u_memset(uStringBuf, 0x2a, sizeof(uStringBuf)/sizeof(*uStringBuf));
    memset(myString, '*', sizeof(myString)/sizeof(*myString));
    memset(testBuf, '*', sizeof(testBuf)/sizeof(*testBuf));

    if (myFile == NULL) {
        log_err("Can't write test file.\n");
        return;
    }

    *n = -1234;
    if (sizeof(void *) == 4) {
        origPtr = (void *)0xdeadbeef;
    } else if (sizeof(void *) == 8) {
        origPtr = (void *) INT64_C(0x1000200030004000);
    } else if (sizeof(void *) == 16) {
        /* iSeries */
        union {
            int32_t arr[4];
            void *ptr;
        } massiveBigEndianPtr = {{ 0x10002000, 0x30004000, 0x50006000, 0x70008000 }};
        origPtr = massiveBigEndianPtr.ptr;
    } else {
        log_err("sizeof(void*)=%d hasn't been tested before", (int)sizeof(void*));
    }

    /* Test fprintf */
    u_fprintf(myFile, "Signed decimal integer %%d: %d\n", *n);
    u_fprintf(myFile, "Signed decimal integer %%i: %i\n", *n);
    u_fprintf(myFile, "Unsigned octal integer %%o: %o\n", *n);
    u_fprintf(myFile, "Unsigned decimal integer %%u: %u\n", *n);
    u_fprintf(myFile, "Lowercase unsigned hexadecimal integer %%x: %x\n", *n);
    u_fprintf(myFile, "Uppercase unsigned hexadecimal integer %%X: %X\n", *n);
    u_fprintf(myFile, "Float %%f: %f\n", myFloat);
    u_fprintf(myFile, "Lowercase float %%e: %e\n", myFloat);
    u_fprintf(myFile, "Uppercase float %%E: %E\n", myFloat);
    u_fprintf(myFile, "Lowercase float %%g: %g\n", myFloat);
    u_fprintf(myFile, "Uppercase float %%G: %G\n", myFloat);
    u_fprintf(myFile, "Pointer %%p: %p\n", origPtr);
    u_fprintf(myFile, "Char %%c: %c\n", 'A');
    u_fprintf(myFile, "UChar %%C: %C\n", (UChar)0x0041); /*'A'*/
    u_fprintf(myFile, "String %%s: %s\n", "My-String");
    u_fprintf(myFile, "NULL String %%s: %s\n", NULL);
    u_fprintf(myFile, "Unicode String %%S: %S\n", myStringOrig);
    u_fprintf(myFile, "NULL Unicode String %%S: %S\n", NULL);
    u_fprintf(myFile, "Percent %%P (non-ANSI): %P\n", myFloat);
    u_fprintf(myFile, "Spell Out %%V (non-ANSI): %V\n", myFloat);

    if (u_feof(myFile)) {
        log_err("Got feof while writing the file.\n");
    }

    *n = 1;
    u_fprintf(myFile, "\t\nPointer to integer (Count) %%n: n=%d %n n=%d\n", *n, n, *n);
    u_fprintf(myFile, "Pointer to integer Value: %d\n", *n);
    u_fprintf(myFile, "This is a long test123456789012345678901234567890123456789012345678901234567890\n");
    *n = 1;
    u_fprintf(myFile, "\tNormal fprintf count: n=%d %n n=%d\n", (int)*n, (int*)n, (int)*n);
    fprintf(u_fgetfile(myFile), "\tNormal fprintf count value: n=%d\n", (int)*n); /* Should be 27 as stated later on. */

    u_fclose(myFile);
    myFile = u_fopen(STANDARD_TEST_FILE, "r", "en_US_POSIX", NULL);

    if (myFile == NULL) {
        log_err("Can't read test file.");
        return;
    }

    if (u_feof(myFile)) {
        log_err("Got feof while reading the file and not at the end of the file.\n");
    }

    myUString[0] = u_fgetc(myFile);
    if (myUString[0] != 0x53 /* S */) {
        log_err("u_fgetc 1 returned %X. Expected 'S'.", myString[0]);
    }
    u_fungetc(myUString[0], myFile);
    myUString[0] = u_fgetc(myFile);
    if (myUString[0] != 0x53 /* S */) {
        log_err("u_fgetc 2 returned %X. Expected 'S'.", myString[0]);
    }
    u_fungetc(myUString[0], myFile);
    myUString[0] = u_fgetc(myFile);
    if (myUString[0] != 0x53 /* S */) {
        log_err("u_fgetc 3 returned %X. Expected 'S'.", myString[0]);
    }
    u_fungetc(myUString[0], myFile);
    myUString[0] = u_fgetc(myFile);
    myUString[1] = (UChar)u_fgetcx(myFile); /* Mix getc and getcx and see what happens. */
    myUString[2] = u_fgetc(myFile);
    if (myUString[0] != 0x53 /* S */ && myUString[1] != 0x69 /* i */ && myUString[2] != 0x6E /* n */) {
        log_err("u_fgetcx returned \\u%04X\\u%04X\\u%04X. Expected 'Sin'.", myString[0], myString[1], myString[2]);
    }
    u_fungetc(myUString[2], myFile);
    u_fungetc(myUString[1], myFile);
    u_fungetc(myUString[0], myFile);

    *n = -1234;

    *newValuePtr = 1;
    u_fscanf(myFile, "Signed decimal integer %%d: %d\n", newValuePtr);
    if (*n != *newValuePtr) {
        log_err("%%d Got: %d, Expected: %d\n", *newValuePtr, *n);
    }
    *newValuePtr = 1;
    u_fscanf(myFile, "Signed decimal integer %%i: %i\n", newValuePtr);
    if (*n != *newValuePtr) {
        log_err("%%i Got: %i, Expected: %i\n", *newValuePtr, *n);
    }
    *newValuePtr = 1;
    u_fscanf(myFile, "Unsigned octal integer %%o: %o\n", newValuePtr);
    if (*n != *newValuePtr) {
        log_err("%%o Got: %o, Expected: %o\n", *newValuePtr, *n);
    }
    *newValuePtr = 1;
    u_fscanf(myFile, "Unsigned decimal integer %%u: %u\n", newValuePtr);
    if (*n != *newValuePtr) {
        log_err("%%u Got: %u, Expected: %u\n", *newValuePtr, *n);
    }
    *newValuePtr = 1;
    u_fscanf(myFile, "Lowercase unsigned hexadecimal integer %%x: %x\n", newValuePtr);
    if (*n != *newValuePtr) {
        log_err("%%x Got: %x, Expected: %x\n", *newValuePtr, *n);
    }
    *newValuePtr = 1;
    u_fscanf(myFile, "Uppercase unsigned hexadecimal integer %%X: %X\n", newValuePtr);
    if (*n != *newValuePtr) {
        log_err("%%X Got: %X, Expected: %X\n", *newValuePtr, *n);
    }
    *newDoubleValuePtr = -1.0;
    u_fscanf(myFile, "Float %%f: %lf\n", newDoubleValuePtr);
    if (myFloat != *newDoubleValuePtr) {
        log_err("%%f Got: %f, Expected: %f\n", *newDoubleValuePtr, myFloat);
    }
    *newDoubleValuePtr = -1.0;
    u_fscanf(myFile, "Lowercase float %%e: %le\n", newDoubleValuePtr);
    if (myFloat != *newDoubleValuePtr) {
        log_err("%%e Got: %e, Expected: %e\n", *newDoubleValuePtr, myFloat);
    }
    *newDoubleValuePtr = -1.0;
    u_fscanf(myFile, "Uppercase float %%E: %lE\n", newDoubleValuePtr);
    if (myFloat != *newDoubleValuePtr) {
        log_err("%%E Got: %E, Expected: %E\n", *newDoubleValuePtr, myFloat);
    }
    *newDoubleValuePtr = -1.0;
    u_fscanf(myFile, "Lowercase float %%g: %lg\n", newDoubleValuePtr);
    if (myFloat != *newDoubleValuePtr) {
        log_err("%%g Got: %g, Expected: %g\n", *newDoubleValuePtr, myFloat);
    }
    *newDoubleValuePtr = -1.0;
    u_fscanf(myFile, "Uppercase float %%G: %lG\n", newDoubleValuePtr);
    if (myFloat != *newDoubleValuePtr) {
        log_err("%%G Got: %G, Expected: %G\n", *newDoubleValuePtr, myFloat);
    }
    ptr = NULL;
    u_fscanf(myFile, "Pointer %%p: %p\n", &ptr);
    if (ptr != origPtr) {
        log_err("%%p Got: %p, Expected: %p\n", ptr, origPtr);
    }
    u_fscanf(myFile, "Char %%c: %c\n", myString);
    if (*myString != 'A') {
        log_err("%%c Got: %c, Expected: A\n", *myString);
    }
    u_fscanf(myFile, "UChar %%C: %C\n", myUString);
    if (*myUString != (UChar)0x0041) { /*'A'*/
        log_err("%%C Got: %C, Expected: A\n", *myUString);
    }
    u_fscanf(myFile, "String %%s: %s\n", myString);
    if (strcmp(myString, "My-String")) {
        log_err("%%s Got: %s, Expected: My String\n", myString);
    }
    u_fscanf(myFile, "NULL String %%s: %s\n", myString);
    if (strcmp(myString, "(null)")) {
        log_err("%%s Got: %s, Expected: My String\n", myString);
    }
    u_fscanf(myFile, "Unicode String %%S: %S\n", myUString);
    u_austrncpy(myString, myUString, sizeof(myUString)/sizeof(*myUString));
    if (strcmp(myString, "My-String")) {
        log_err("%%S Got: %S, Expected: My String\n", myUString);
    }
    u_fscanf(myFile, "NULL Unicode String %%S: %S\n", myUString);
    u_austrncpy(myString, myUString, sizeof(myUString)/sizeof(*myUString));
    if (strcmp(myString, "(null)")) {
        log_err("%%S Got: %S, Expected: My String\n", myUString);
    }
    *newDoubleValuePtr = -1.0;
    u_fscanf(myFile, "Percent %%P (non-ANSI): %P\n", newDoubleValuePtr);
    if (myFloat != *newDoubleValuePtr) {
        log_err("%%P Got: %f, Expected: %f\n", *newDoubleValuePtr, myFloat);
    }
    *newDoubleValuePtr = -1.0;
    u_fscanf(myFile, "Spell Out %%V (non-ANSI): %V\n", newDoubleValuePtr);
    if (myFloat != *newDoubleValuePtr) {
        log_err("%%V Got: %f, Expected: %f\n", *newDoubleValuePtr, myFloat);
    }

    u_fgets(myUString, 4, myFile);
    myString[2] = '!';
    myString[3] = '!';
    u_austrncpy(myString, myUString, sizeof(myUString)/sizeof(*myUString));
    if (myString == NULL || strcmp(myString, "\t\n") != 0) {
        log_err("u_fgets got \"%s\"\n", myString);
    }

    if (u_fgets(myUString, sizeof(myUString)/sizeof(*myUString), myFile) != myUString) {
        log_err("u_fgets did not return myUString\n");
    }
    u_austrncpy(myString, myUString, sizeof(myUString)/sizeof(*myUString));
    if (myString == NULL || strcmp(myString, "Pointer to integer (Count) %n: n=1  n=1\n") != 0) {
        log_err("u_fgets got \"%s\"\n", myString);
    }

    if (u_fgets(myUString, sizeof(myUString)/sizeof(*myUString), myFile) != myUString) {
        log_err("u_fgets did not return myUString\n");
    }
    u_austrncpy(myString, myUString, sizeof(myUString)/sizeof(*myUString));
    if (myString == NULL || strcmp(myString, "Pointer to integer Value: 37\n") != 0) {
        log_err("u_fgets got \"%s\"\n", myString);
    }

    if (u_fgets(myUString, sizeof(myUString)/sizeof(*myUString), myFile) != myUString) {
        log_err("u_fgets did not return myUString\n");
    }
    u_austrncpy(myString, myUString, sizeof(myUString)/sizeof(*myUString));
    if (myString == NULL || strcmp(myString, "This is a long test123456789012345678901234567890123456789012345678901234567890\n") != 0) {
        log_err("u_fgets got \"%s\"\n", myString);
    }

    if (u_fgets(myUString, 0, myFile) != NULL) {
        log_err("u_fgets got \"%s\" and it should have returned NULL\n", myString);
    }

    if (u_fgets(myUString, 1, myFile) != myUString) {
        log_err("u_fgets did not return myUString\n");
    }
    u_austrncpy(myString, myUString, sizeof(myUString)/sizeof(*myUString));
    if (myString == NULL || strcmp(myString, "") != 0) {
        log_err("u_fgets got \"%s\"\n", myString);
    }

    if (u_fgets(myUString, 2, myFile) != myUString) {
        log_err("u_fgets did not return myUString\n");
    }
    u_austrncpy(myString, myUString, sizeof(myUString)/sizeof(*myUString));
    if (myString == NULL || strcmp(myString, "\t") != 0) {
        log_err("u_fgets got \"%s\"\n", myString);
    }

    u_austrncpy(myString, u_fgets(myUString, sizeof(myUString)/sizeof(*myUString), myFile),
        sizeof(myUString)/sizeof(*myUString));
    if (strcmp(myString, "Normal fprintf count: n=1  n=1\n") != 0) {
        log_err("u_fgets got \"%s\"\n", myString);
    }

    if (u_feof(myFile)) {
        log_err("Got feof while reading the file and not at the end of the file.\n");
    }
    u_austrncpy(myString, u_fgets(myUString, sizeof(myUString)/sizeof(*myUString), myFile),
        sizeof(myUString)/sizeof(*myUString));
    if (strcmp(myString, "\tNormal fprintf count value: n=27\n") != 0) {
        log_err("u_fgets got \"%s\"\n", myString);
    }
    if (!u_feof(myFile)) {
        log_err("Did not get feof while reading the end of the file.\n");
    }
    if (u_fscanf(myFile, "%S\n", myUString) != 0) {
        log_err("u_fscanf read data while reading the end of the file.\n");
    }

    u_fclose(myFile);
}

static void TestFile(void) {
/*    FILE *standardFile;*/

    log_verbose("Testing u_fopen\n");
    TestFileFromICU(u_fopen(STANDARD_TEST_FILE, "w", "en_US_POSIX", NULL));

/* Don't know how to make this work without stdout or stderr */
/*
    log_verbose("Testing u_finit\n");
    standardFile = fopen(STANDARD_TEST_FILE, "wb");
    TestFileFromICU(u_finit(standardFile, NULL, NULL));
    fclose(standardFile);

    log_verbose("Testing u_fadopt\n");
    standardFile = fopen(STANDARD_TEST_FILE, "wb");
    TestFileFromICU(u_fadopt(standardFile, NULL, NULL));
*/
}
#endif

static void StdinBuffering(void) {
#if 0
    UChar buff[255];
    int32_t num = 0;
    UFILE *uStdIn = NULL;
    UFILE *uStdOut = NULL;
    uStdIn = u_finit(stdin, NULL, NULL);
    uStdOut = u_finit(stdout, NULL, NULL);
    if (uStdIn == NULL)
        return;

    buff[0] = 0x40;
    buff[1] = 0;
    u_fgets(buff, sizeof(buff)/sizeof(buff[0]), uStdIn);
    u_fprintf(uStdOut, "%S\n", buff);
    u_fscanf(uStdIn, "%d", &num);
    u_fprintf(uStdOut, "%d\n", num);
    u_fscanf(uStdIn, "%d", &num);
    u_fprintf(uStdOut, "%d\n", num);
#else
    log_verbose("Test disabled because it requires user interaction");
#endif
}

static void TestCodepageAndLocale(void) {
    UFILE *myFile = u_fopen(STANDARD_TEST_FILE, "w", NULL, NULL);
    if (myFile == NULL) {
        log_err("Can't write test file.\n");
        return;
    }
    if (u_fgetcodepage(myFile) == NULL
        || strcmp(u_fgetcodepage(myFile), ucnv_getDefaultName()) != 0)
    {
        log_err("Didn't get the proper default codepage. Got %s expected: %s\n",
            u_fgetcodepage(myFile), ucnv_getDefaultName());
    }
#if !UCONFIG_NO_FORMATTING
    if (u_fgetlocale(myFile) == NULL
        || strcmp(u_fgetlocale(myFile), uloc_getDefault()) != 0)
    {
        log_err("Didn't get the proper default locale. Got %s expected: %s\n",
            u_fgetlocale(myFile), uloc_getDefault());
    }
#endif
    u_fclose(myFile);

    myFile = u_fopen(STANDARD_TEST_FILE, "w", "es", NULL);
    if (u_fgetcodepage(myFile) == NULL
        || strcmp(u_fgetcodepage(myFile), ucnv_getDefaultName()) != 0)
    {
        log_err("Didn't get the proper default codepage for \"es\". Got %s expected: iso-8859-1\n",
            u_fgetcodepage(myFile));
    }
#if !UCONFIG_NO_FORMATTING
    if (u_fgetlocale(myFile) == NULL
        || strcmp(u_fgetlocale(myFile), "es") != 0)
    {
        log_err("Didn't get the proper default locale. Got %s expected: %s\n",
            u_fgetlocale(myFile), "es");
    }
#endif
    u_fclose(myFile);

    myFile = u_fopen(STANDARD_TEST_FILE, "w", NULL, "UTF-16");
    if (u_fgetcodepage(myFile) == NULL
        || strcmp(u_fgetcodepage(myFile), "UTF-16") != 0)
    {
        log_err("Didn't get the proper default codepage for \"en\". Got %s expected: iso-8859-1\n",
            u_fgetcodepage(myFile));
    }
#if !UCONFIG_NO_FORMATTING
    if (u_fgetlocale(myFile) == NULL
        || strcmp(u_fgetlocale(myFile), uloc_getDefault()) != 0)
    {
        log_err("Didn't get the proper default locale. Got %s expected: %s\n",
            u_fgetlocale(myFile), uloc_getDefault());
    }
#endif
    u_fclose(myFile);

    myFile = u_fopen(STANDARD_TEST_FILE, "w", "zh", "UTF-16");
    if (u_fgetcodepage(myFile) == NULL
        || strcmp(u_fgetcodepage(myFile), "UTF-16") != 0)
    {
        log_err("Didn't get the proper default codepage for \"en\". Got %s expected: iso-8859-1\n",
            u_fgetcodepage(myFile));
    }
#if !UCONFIG_NO_FORMATTING
    if (u_fgetlocale(myFile) == NULL
        || strcmp(u_fgetlocale(myFile), "zh") != 0)
    {
        log_err("Didn't get the proper default locale. Got %s expected: %s\n",
            u_fgetlocale(myFile), "zh");
    }
#endif
    u_fclose(myFile);
}


static void TestfgetsBuffers(void) {
    UChar buffer[2048];
    UChar expectedBuffer[2048];
    static const char testStr[] = "This is a test string that tests u_fgets. It makes sure that we don't try to read too much!";
    UFILE *myFile = u_fopen(STANDARD_TEST_FILE, "w", NULL, "UTF-16");
    int32_t expectedSize = (int32_t)strlen(testStr);
    int32_t readSize;
    int32_t repetitions;

    if (myFile == NULL) {
        log_err("Can't write test file.\n");
        return;
    }

    u_fputc(0x3BC, myFile);
    if (u_fputc(0x110000, myFile) != U_EOF) {
        log_err("u_fputc should return U_EOF for 0x110000.\n");
    }
    if (u_fputc((UChar32)0xFFFFFFFFu, myFile) != U_EOF) {
        log_err("u_fputc should return U_EOF for 0xFFFFFFFF.\n");
    }
    u_fputc(0xFF41, myFile);
    u_memset(buffer, 0xBEEF, sizeof(buffer)/sizeof(buffer[0]));
    u_memset(expectedBuffer, 0, sizeof(expectedBuffer)/sizeof(expectedBuffer[0]));
    u_uastrncpy(buffer, testStr, expectedSize+1);
    for (repetitions = 0; repetitions < 16; repetitions++) {
        u_file_write(buffer, expectedSize, myFile);
        u_strcat(expectedBuffer, buffer);
    }
    u_fclose(myFile);

    u_memset(buffer, 0xBEEF, sizeof(buffer)/sizeof(buffer[0]));
    myFile = u_fopen(STANDARD_TEST_FILE, "r", NULL, "UTF-16");
    if (u_fgetc(myFile) != 0x3BC) {
        log_err("The first character is wrong\n");
    }
    if (u_fgetc(myFile) != 0xFF41) {
        log_err("The second character is wrong\n");
    }
    if (u_fgets(buffer, sizeof(buffer)/sizeof(buffer[0]), myFile) != buffer) {
        log_err("Didn't get the buffer back\n");
        return;
    }
    readSize = u_strlen(buffer);
    if (readSize != expectedSize*repetitions) {
        log_err("Buffer is the wrong size. Got %d Expected %d\n", u_strlen(buffer), expectedSize*repetitions);
    }
    if (buffer[(expectedSize*repetitions) + 1] != 0xBEEF) {
        log_err("u_fgets wrote too much data\n");
    }
    if (u_strcmp(buffer, expectedBuffer) != 0) {
        log_err("Did get expected string back\n");
    }
    if (strcmp(u_fgetcodepage(myFile), "UTF-16") != 0) {
        log_err("Got %s instead of UTF-16\n", u_fgetcodepage(myFile));
    }
    u_fclose(myFile);

    log_verbose("Now trying a multi-byte encoding (UTF-8).\n");

    myFile = u_fopen(STANDARD_TEST_FILE, "w", NULL, "UTF-8");

    u_fputc(0x3BC, myFile);
    u_fputc(0xFF41, myFile);
    u_memset(buffer, 0xBEEF, sizeof(buffer)/sizeof(buffer[0]));
    u_memset(expectedBuffer, 0, sizeof(expectedBuffer)/sizeof(expectedBuffer[0]));
    u_uastrncpy(buffer, testStr, expectedSize+1);
    for (repetitions = 0; repetitions < 16; repetitions++) {
        u_file_write(buffer, expectedSize, myFile);
        u_strcat(expectedBuffer, buffer);
    }
    u_fclose(myFile);

    u_memset(buffer, 0xBEEF, sizeof(buffer)/sizeof(buffer[0]));
    myFile = u_fopen(STANDARD_TEST_FILE, "r", NULL, "UTF-8");
    if (strcmp(u_fgetcodepage(myFile), "UTF-8") != 0) {
        log_err("Got %s instead of UTF-8\n", u_fgetcodepage(myFile));
    }
    if (u_fgetc(myFile) != 0x3BC) {
        log_err("The first character is wrong\n");
    }
    if (u_fgetc(myFile) != 0xFF41) {
        log_err("The second character is wrong\n");
    }
    if (u_fgets(buffer, sizeof(buffer)/sizeof(buffer[0]), myFile) != buffer) {
        log_err("Didn't get the buffer back\n");
        return;
    }
    readSize = u_strlen(buffer);
    if (readSize != expectedSize*repetitions) {
        log_err("Buffer is the wrong size. Got %d Expected %d\n", u_strlen(buffer), expectedSize*repetitions);
    }
    if (buffer[(expectedSize*repetitions) + 1] != 0xBEEF) {
        log_err("u_fgets wrote too much data\n");
    }
    if (u_strcmp(buffer, expectedBuffer) != 0) {
        log_err("Did get expected string back\n");
    }
    u_fclose(myFile);


    log_verbose("Now trying a multi-byte encoding (UTF-8) with a really small buffer.\n");

    myFile = u_fopen(STANDARD_TEST_FILE, "w", NULL, "UTF-8");

    u_fputc(0xFF41, myFile);
    u_memset(buffer, 0xBEEF, sizeof(buffer)/sizeof(buffer[0]));
    u_memset(expectedBuffer, 0, sizeof(expectedBuffer)/sizeof(expectedBuffer[0]));
    u_uastrncpy(buffer, testStr, expectedSize+1);
    for (repetitions = 0; repetitions < 1; repetitions++) {
        u_file_write(buffer, expectedSize, myFile);
        u_strcat(expectedBuffer, buffer);
    }
    u_fclose(myFile);

    u_memset(buffer, 0xBEEF, sizeof(buffer)/sizeof(buffer[0]));
    myFile = u_fopen(STANDARD_TEST_FILE, "r", NULL, "UTF-8");
    if (u_fgets(buffer, 2, myFile) != buffer) {
        log_err("Didn't get the buffer back\n");
        return;
    }
    readSize = u_strlen(buffer);
    if (readSize != 1) {
        log_err("Buffer is the wrong size. Got %d Expected %d\n", u_strlen(buffer), 1);
    }
    if (buffer[0] != 0xFF41 || buffer[1] != 0) {
        log_err("Did get expected string back\n");
    }
    if (buffer[2] != 0xBEEF) {
        log_err("u_fgets wrote too much data\n");
    }
    u_fclose(myFile);

}

static void TestFileReadBuffering(void) {
    UChar buffer[1024];
    UFILE *myFile = u_fopen(STANDARD_TEST_FILE, "w", NULL, "UTF-16");
    int32_t how_many;
    int32_t repetitions;

    u_memset(buffer, 0xBEEF, sizeof(buffer)/sizeof(buffer[0]));
    for (repetitions = 0; repetitions < 2; repetitions++) {
        u_file_write(buffer, sizeof(buffer)/sizeof(buffer[0]), myFile);
    }

    u_fclose(myFile);
    u_memset(buffer, 0xDEAD, sizeof(buffer)/sizeof(buffer[0]));
    myFile = u_fopen(STANDARD_TEST_FILE, "r", NULL, "UTF-16");
    how_many = u_file_read(buffer, 1024, myFile);
    if (how_many != 1024 || buffer[1023] != 0xBEEF) {
        log_err("u_file_read read too much or not enough data\n");
    }
    u_fclose(myFile);
}

static void TestfgetsLineCount(void) {
    UChar buffer[2048];
    UChar expectedBuffer[2048];
    char charBuffer[2048];
    static const char testStr[] = "This is a test string that tests u_fgets. It makes sure that we don't try to read too much!";
    UFILE *myFile = NULL;
    FILE *stdFile = fopen(STANDARD_TEST_FILE, "w");
    int32_t expectedSize = (int32_t)strlen(testStr);
    int32_t repetitions;
    int32_t nlRepetitions;

    if (stdFile == NULL) {
        log_err("Can't write test file.\n");
        return;
    }
    u_memset(expectedBuffer, 0, sizeof(expectedBuffer)/sizeof(expectedBuffer[0]));

    for (repetitions = 0; repetitions < 16; repetitions++) {
        fwrite(testStr, sizeof(testStr[0]), expectedSize, stdFile);
        for (nlRepetitions = 0; nlRepetitions < repetitions; nlRepetitions++) {
            fwrite("\n", sizeof(testStr[0]), 1, stdFile);
        }
    }
    fclose(stdFile);

    myFile = u_fopen(STANDARD_TEST_FILE, "r", NULL, NULL);
    stdFile = fopen(STANDARD_TEST_FILE, "r");

    for (;;) {
        char *returnedCharBuffer;
        UChar *returnedUCharBuffer;

        u_memset(buffer, 0xBEEF, sizeof(buffer)/sizeof(buffer[0]));
        returnedCharBuffer = fgets(charBuffer, sizeof(charBuffer)/sizeof(charBuffer[0]), stdFile);
        returnedUCharBuffer = u_fgets(buffer, sizeof(buffer)/sizeof(buffer[0]), myFile);

        if (!returnedCharBuffer && !returnedUCharBuffer) {
            /* Both returned NULL. stop. */
            break;
        }
        if (returnedCharBuffer != charBuffer) {
            log_err("Didn't get the charBuffer back\n");
            continue;
        }
        u_uastrncpy(expectedBuffer, charBuffer, (int32_t)strlen(charBuffer)+1);
        if (returnedUCharBuffer != buffer) {
            log_err("Didn't get the buffer back\n");
            continue;
        }
        if (u_strcmp(buffer, expectedBuffer) != 0) {
            log_err("buffers are different\n");
        }
        if (buffer[u_strlen(buffer)+1] != 0xBEEF) {
            log_err("u_fgets wrote too much\n");
        }
    }
    fclose(stdFile);
    u_fclose(myFile);
}

static void TestfgetsNewLineHandling(void) {
    UChar buffer[256];
    static const UChar testUStr[][16] = {
        {0x000D, 0},
        {0x000D, 0x000A, 0},
        {0x000D, 0},
        {0x000D, 0},
        {0x0085, 0},
        {0x000A, 0},
        {0x000D, 0},
        {0x000B, 0},
        {0x000C, 0},
        {0x000C, 0},
        {0x2028, 0},
        {0x0085, 0},
        {0x2029, 0},
        {0x0085, 0},

        {0x008B, 0x000D, 0},
        {0x00A0, 0x000D, 0x000A, 0},
        {0x3000, 0x000D, 0},
        {0xd800, 0xdfff, 0x000D, 0},
        {0x00AB, 0x0085, 0},
        {0x00AC, 0x000A, 0},
        {0x00AD, 0x000D, 0},
        {0x00BA, 0x000B, 0},
        {0x00AB, 0x000C, 0},
        {0x00B1, 0x000C, 0},
        {0x30BB, 0x2028, 0},
        {0x00A5, 0x0085, 0},
        {0x0080, 0x2029, 0},
        {0x00AF, 0x0085, 0}

    };
    UFILE *myFile = NULL;
    int32_t lineIdx;

    myFile = u_fopen(STANDARD_TEST_FILE, "wb", NULL, "UTF-8");
    if (myFile == NULL) {
        log_err("Can't write test file.\n");
        return;
    }
    for (lineIdx = 0; lineIdx < (int32_t)(sizeof(testUStr)/sizeof(testUStr[0])); lineIdx++) {
        u_file_write(testUStr[lineIdx], u_strlen(testUStr[lineIdx]), myFile);
    }
    u_fclose(myFile);

    myFile = u_fopen(STANDARD_TEST_FILE, "rb", NULL, "UTF-8");

    for (lineIdx = 0; lineIdx < (int32_t)(sizeof(testUStr)/sizeof(testUStr[0])); lineIdx++) {
        UChar *returnedUCharBuffer;

        u_memset(buffer, 0xBEEF, sizeof(buffer)/sizeof(buffer[0]));
        returnedUCharBuffer = u_fgets(buffer, sizeof(buffer)/sizeof(buffer[0]), myFile);

        if (!returnedUCharBuffer) {
            /* Returned NULL. stop. */
            break;
        }
        if (u_strcmp(buffer, testUStr[lineIdx]) != 0) {
            log_err("buffers are different at index = %d\n", lineIdx);
        }
        if (buffer[u_strlen(buffer)+1] != 0xBEEF) {
            log_err("u_fgets wrote too much\n");
        }
    }
    if (lineIdx != (int32_t)(sizeof(testUStr)/sizeof(testUStr[0]))) {
        log_err("u_fgets read too much\n");
    }
    if (u_fgets(buffer, sizeof(buffer)/sizeof(buffer[0]), myFile) != NULL) {
        log_err("u_file_write wrote too much\n");
    }
    u_fclose(myFile);
}

static void TestLineCount(const char *prefixLine, const char *line, int32_t numRepititions) {
    UChar buffer[64];
    UChar expectedBuffer[64];
    int32_t lineLen = strlen(line);
    UChar *returnedUCharBuffer;
    int32_t repetitions;
    UFILE *myFile = NULL;
    FILE *stdFile = fopen(STANDARD_TEST_FILE, "wb");

    if (stdFile == NULL) {
        log_err("Can't write test file.\n");
        return;
    }
    /* Write a prefix line and then write a bunch of lines */
    fwrite(prefixLine, strlen(prefixLine), 1, stdFile);
    for (repetitions = 0; repetitions < numRepititions; repetitions++) {
        fwrite(line, lineLen, 1, stdFile);
    }
    fclose(stdFile);

    myFile = u_fopen(STANDARD_TEST_FILE, "rb", NULL, NULL);
    if (myFile == NULL) {
        log_err("Can't read test file.\n");
        return;
    }

    /* Read the prefix line. This can make sure that a Windows newline is either on a boundary or before it. */
    u_uastrncpy(expectedBuffer, prefixLine, (int32_t)strlen(prefixLine)+1);
    returnedUCharBuffer = u_fgets(buffer, sizeof(buffer)/sizeof(buffer[0]), myFile);
    if (u_strcmp(returnedUCharBuffer, expectedBuffer) != 0) {
        log_err("prefix buffer is different. prefix=\"%s\"\n", prefixLine);
        return;
    }

    u_uastrncpy(expectedBuffer, line, (int32_t)strlen(line)+1);
    for (repetitions = 0; ; repetitions++) {
        u_memset(buffer, 0xBEEF, sizeof(buffer)/sizeof(buffer[0]));
        returnedUCharBuffer = u_fgets(buffer, sizeof(buffer)/sizeof(buffer[0]), myFile);

        if (!returnedUCharBuffer) {
            /* returned NULL. stop. */
            break;
        }
        if (u_strcmp(buffer, expectedBuffer) != 0) {
            log_err("buffers are different at count %d\n", repetitions);
        }
        if (buffer[u_strlen(buffer)+1] != 0xBEEF) {
            log_err("u_fgets wrote too much\n");
        }
    }
    if (repetitions != numRepititions) {
        log_err("got wrong number of lines. got=%d expected=%d\n", repetitions, numRepititions);
    }
    u_fclose(myFile);
}

static void TestfgetsNewLineCount(void) {
    /* This makes sure that lines are correctly handled between buffer boundaries. */
    TestLineCount("\n", "\n", 1024);    /* Unix newlines */
    TestLineCount("\r\n", "\r\n", 1024);/* Windows newlines */
    TestLineCount("a\r\n", "\r\n", 1024);/* Windows newlines offset by 1 byte */
    TestLineCount("\r\n", "a\r\n", 1024);/* Windows newlines offset with data */
    TestLineCount("\n", "a\n", 1024);    /* Unix newlines offset with data */
    TestLineCount("\n", "\r\n", 1024);  /* a mixed number of lines. */
}

static void TestFgetsLineBuffering(void) {
    UChar buffer[2003]; /* Use a non-power of 2 or 10 */
    UChar *returnedUCharBuffer;
    int32_t repetitions;
    UFILE *myFile = NULL;
    FILE *stdFile = fopen(STANDARD_TEST_FILE, "wb");

    if (stdFile == NULL) {
        log_err("Can't write test file.\n");
        return;
    }
    u_memset(buffer, 0xBEEF, sizeof(buffer)/sizeof(buffer[0]));

    /* Write one very long line */
    for (repetitions = 0; repetitions < ((sizeof(buffer)/sizeof(buffer[0]))*2); repetitions++) {
        fwrite(repetitions ? "1" : "2", 1, 1, stdFile);
    }
    fclose(stdFile);

    myFile = u_fopen(STANDARD_TEST_FILE, "rb", NULL, NULL);
    if (myFile == NULL) {
        log_err("Can't read test file.\n");
        return;
    }

    /* Read part of one very long line */
    returnedUCharBuffer = u_fgets(buffer, (sizeof(buffer)/sizeof(buffer[0]))-1, myFile);
    if (u_strlen(returnedUCharBuffer) != ((sizeof(buffer)/sizeof(buffer[0]))-2)) {
        log_err("Line is wrong length. Got %d. Expected %d.\n",
            u_strlen(returnedUCharBuffer), ((sizeof(buffer)/sizeof(buffer[0]))-2));
    }
    /* We better not read too much */
    if (buffer[(sizeof(buffer)/sizeof(buffer[0]))-1] != 0xBEEF) {
        log_err("Too much data was written\n");
    }

    u_fclose(myFile);
}


static void TestCodepage(void) {
    UFILE *myFile = NULL;
    static const UChar strABAccentA[] = { 0x0041, 0x0042, 0x00C1, 0x0043, 0};
    static const UChar strBadConversion[] = { 0x0041, 0x0042, 0xfffd, 0x0043, 0};
    UChar testBuf[sizeof(strABAccentA)/sizeof(strABAccentA[0])*2]; /* *2 to see if too much was  */
    char convName[UCNV_MAX_CONVERTER_NAME_LENGTH];
    int32_t retVal;
    UErrorCode status = U_ZERO_ERROR;

    myFile = u_fopen(STANDARD_TEST_FILE, "w", NULL, "absurd converter that can't be opened");

    if (myFile) {
        log_err("Recieved a UFILE * with an invalid codepage parameter\n");
        u_fclose(myFile);
    }

    myFile = u_fopen(STANDARD_TEST_FILE, "w", NULL, "ISO-8859-1");
    if (myFile == NULL) {
        log_err("Can't write test file for iso-8859-1.\n");
        return;
    }
    if (strcmp("ISO-8859-1", u_fgetcodepage(myFile)) != 0) {
        log_err("Couldn't get ISO-8859-1 back as opened codepage\n");
    }
    u_file_write(strABAccentA, u_strlen(strABAccentA), myFile);
    u_fclose(myFile);

    /* Now see what we got wrote */
    myFile = u_fopen(STANDARD_TEST_FILE, "r", NULL, NULL);
    if (u_fsetcodepage("ISO-8859-1", myFile) != 0) {
        log_err("u_fsetcodepage didn't set the codepage\n");
    }
    retVal = u_file_read(testBuf, u_strlen(strABAccentA), myFile);
    if (u_strncmp(strABAccentA, testBuf, u_strlen(strABAccentA)) != 0) {
        log_err("The test data was read and written differently!\n");
    }
    if (retVal != u_strlen(strABAccentA)) {
        log_err("The test data returned different lengths. Got: %d, Expected %d\n", retVal, u_strlen(strABAccentA));
    }
    u_fclose(myFile);

    /* What happens on invalid input? */
    myFile = u_fopen(STANDARD_TEST_FILE, "r", NULL, "ISO-8859-1");
    if (strcmp(ucnv_getName(u_fgetConverter(myFile), &status), "ISO-8859-1") != 0) {
        log_err("u_fgetConverter returned %s\n", ucnv_getName(u_fgetConverter(myFile), &status));
    }
    if (u_fsetcodepage("UTF-8", myFile) != 0) {
        log_err("u_fsetcodepage didn't set the codepage to UTF-8\n");
    }
    if (strcmp(ucnv_getName(u_fgetConverter(myFile), &status), "UTF-8") != 0) {
        log_err("u_fgetConverter returned %s\n", ucnv_getName(u_fgetConverter(myFile), &status));
    }
    retVal = u_file_read(testBuf, u_strlen(strBadConversion), myFile);
    if (u_strncmp(strBadConversion, testBuf, u_strlen(strBadConversion)) != 0) {
        log_err("The test data wasn't subsituted as expected\n");
    }
    u_fclose(myFile);

    /* Can't currently swap codepages midstream */
    myFile = u_fopen(STANDARD_TEST_FILE, "r", NULL, "ISO-8859-1");
    strcpy(convName, u_fgetcodepage(myFile));
    u_file_read(testBuf, 1, myFile);
    if (u_fsetcodepage("UTF-8", myFile) == 0) {
        log_err("u_fsetcodepage set the codepage after reading a byte\n");
    }
    retVal = u_file_read(testBuf + 1, u_strlen(strABAccentA) - 1, myFile);
    if (u_strncmp(strABAccentA, testBuf, u_strlen(strABAccentA)) != 0) {
        log_err("u_fsetcodepage changed the codepages after writing data\n");
    }
    if ((retVal + 1) != u_strlen(strABAccentA)) {
        log_err("The test data returned different lengths. Got: %d, Expected %d\n", retVal, u_strlen(strABAccentA));
    }
    u_frewind(myFile);
    retVal = u_file_read(testBuf, u_strlen(strABAccentA), myFile);
    if (u_strncmp(strABAccentA, testBuf, u_strlen(strABAccentA)) != 0) {
        log_err("The test data was read and written differently!\n");
    }
    if (retVal != u_strlen(strABAccentA)) {
        log_err("The test data returned different lengths. Got: %d, Expected %d\n", retVal, u_strlen(strABAccentA));
    }
    u_fclose(myFile);

}

static void TestCodepageFlush(void) {
#if UCONFIG_NO_LEGACY_CONVERSION
  log_verbose("Skipping, legacy conversion is disabled.");
#else
  UChar utf16String[] = { 0x39, 0x39, 0x39, 0x20, 0x65E0, 0x6CD6, 0x5728, 0x0000 };
  uint8_t inBuf[200];
  size_t inLen =0;
  const char *enc = "IBM-1388"; /* GBK EBCDIC stateful */
  UFILE *myFile = u_fopen(STANDARD_TEST_FILE, "wb", "en_US_POSIX", enc);
  FILE *myCFile;
  int shift = 0;
  int i;

  if (myFile == NULL) {
    log_err("Can't write test file %s\n", STANDARD_TEST_FILE);
    return;
  }
  
  u_fprintf(myFile, "%S", utf16String);
  u_fclose(myFile);

  /* now read it back */
    myCFile = fopen(STANDARD_TEST_FILE, "rb");
    if (myCFile == NULL) {
        log_err("Can't read test file.");
        return;
    }

    inLen = fread(inBuf, 1, 200, myCFile);
    fclose(myCFile);
    
    if(inLen<=0) {
      log_err("Failed during read of test file.");
      return;
    }

    /* check if shift in and out */
    for(i=0;i<inLen;i++) {
      if(inBuf[i]==0x0E) {  /* SO */
        shift= 1;
      } else if(inBuf[i]==0x0F) { /* SI */
        shift= -1;
      }
    }
    
    if(shift==0) {
      log_err("Err: shift was unchanged\n");
    } else if(shift==1) {
      log_err("Err: at end of string, we were still shifted out (SO, 0x0E).\n");
    } else if(shift==-1) {
      log_verbose("OK: Shifted in (SI, 0x0F)\n");
    }

    if(inLen != 12) {
      log_err("Expected 12 bytes, read %d\n", inLen);
    } else {
      log_verbose("OK: read %d bytes\n", inLen);
    }


#endif
}

#if !UCONFIG_NO_FORMATTING
static void TestFilePrintCompatibility(void) {
    UFILE *myFile = u_fopen(STANDARD_TEST_FILE, "wb", "en_US_POSIX", NULL);
    FILE *myCFile;
    int32_t num;
    char cVal;
    static const UChar emptyStr[] = {0};
    char readBuf[512] = "";
    char testBuf[512] = "";

    if (myFile == NULL) {
        log_err("Can't write test file.\n");
        return;
    }
#if !UCONFIG_NO_FORMATTING
    if (strcmp(u_fgetlocale(myFile), "en_US_POSIX") != 0) {
        log_err("Got %s instead of en_US_POSIX for locale\n", u_fgetlocale(myFile));
    }
#endif

    /* Compare against C API compatibility */
    for (num = -STANDARD_TEST_NUM_RANGE; num < STANDARD_TEST_NUM_RANGE; num++) {
        u_fprintf(myFile, "%x ", num);
        u_fprintf(myFile, "%X ", num);
        u_fprintf(myFile, "%o ", num);
        u_fprintf(myFile, "%d ", num);
        u_fprintf(myFile, "%i ", num);
        u_fprintf(myFile, "%f ", (double)num);
/*        u_fprintf(myFile, "%e ", (double)num);
        u_fprintf(myFile, "%E ", (double)num);*/
        u_fprintf(myFile, "%g ", (double)num);
        u_fprintf(myFile, "%G", (double)num);
        u_fputs(emptyStr, myFile);
    }

    u_fprintf_u(myFile, NEW_LINE);

    for (num = 0; num < 0x80; num++) {
        u_fprintf(myFile, "%c", num);
    }

    u_fclose(myFile);
    myCFile = fopen(STANDARD_TEST_FILE, "rb");
    if (myCFile == NULL) {
        log_err("Can't read test file.");
        return;
    }

    for (num = -STANDARD_TEST_NUM_RANGE; num < STANDARD_TEST_NUM_RANGE; num++) {
        fscanf(myCFile, "%s", readBuf);
        sprintf(testBuf, "%x", (int)num);
        if (strcmp(readBuf, testBuf) != 0) {
            log_err("%%x Got: \"%s\", Expected: \"%s\"\n", readBuf, testBuf);
        }

        fscanf(myCFile, "%s", readBuf);
        sprintf(testBuf, "%X", (int)num);
        if (strcmp(readBuf, testBuf) != 0) {
            log_err("%%X Got: \"%s\", Expected: \"%s\"\n", readBuf, testBuf);
        }

        fscanf(myCFile, "%s", readBuf);
        sprintf(testBuf, "%o", (int)num);
        if (strcmp(readBuf, testBuf) != 0) {
            log_err("%%o Got: \"%s\", Expected: \"%s\"\n", readBuf, testBuf);
        }

        /* fprintf is not compatible on all platforms e.g. the iSeries */
        fscanf(myCFile, "%s", readBuf);
        sprintf(testBuf, "%d", (int)num);
        if (strcmp(readBuf, testBuf) != 0) {
            log_err("%%d Got: \"%s\", Expected: \"%s\"\n", readBuf, testBuf);
        }

        fscanf(myCFile, "%s", readBuf);
        sprintf(testBuf, "%i", (int)num);
        if (strcmp(readBuf, testBuf) != 0) {
            log_err("%%i Got: \"%s\", Expected: \"%s\"\n", readBuf, testBuf);
        }

        fscanf(myCFile, "%s", readBuf);
        sprintf(testBuf, "%f", (double)num);
        if (strcmp(readBuf, testBuf) != 0) {
            log_err("%%f Got: \"%s\", Expected: \"%s\"\n", readBuf, testBuf);
        }

/*        fscanf(myCFile, "%s", readBuf);
        sprintf(testBuf, "%e", (double)num);
        if (strcmp(readBuf, testBuf) != 0) {
            log_err("%%e Got: \"%s\", Expected: \"%s\"\n", readBuf, testBuf);
        }

        fscanf(myCFile, "%s", readBuf);
        sprintf(testBuf, "%E", (double)num);
        if (strcmp(readBuf, testBuf) != 0) {
            log_err("%%E Got: \"%s\", Expected: \"%s\"\n", readBuf, testBuf);
        }*/

        fscanf(myCFile, "%s", readBuf);
        sprintf(testBuf, "%g", (double)num);
        if (strcmp(readBuf, testBuf) != 0) {
            log_err("%%g Got: \"%s\", Expected: \"%s\"\n", readBuf, testBuf);
        }

        fscanf(myCFile, "%s", readBuf);
        sprintf(testBuf, "%G", (double)num);
        if (strcmp(readBuf, testBuf) != 0) {
            log_err("%%G Got: \"%s\", Expected: \"%s\"\n", readBuf, testBuf);
        }
    }

    /* Properly eat the newlines */
    for (num = 0; num < (int32_t)strlen(C_NEW_LINE); num++) {
        fscanf(myCFile, "%c", &cVal);
        if (cVal != C_NEW_LINE[num]) {
            log_err("OS newline error\n");
        }
    }
    for (num = 0; num < (int32_t)strlen(C_NEW_LINE); num++) {
        fscanf(myCFile, "%c", &cVal);
        if (cVal != C_NEW_LINE[num]) {
            log_err("ustdio newline error\n");
        }
    }

    for (num = 0; num < 0x80; num++) {
        cVal = -1;
        fscanf(myCFile, "%c", &cVal);
        if (num != cVal) {
            log_err("%%c Got: 0x%x, Expected: 0x%x\n", cVal, num);
        }
    }
    fclose(myCFile);
}
#endif

#define TestFPrintFormat(uFormat, uValue, cFormat, cValue) \
    myFile = u_fopen(STANDARD_TEST_FILE, "w", "en_US_POSIX", NULL);\
    if (myFile == NULL) {\
        log_err("Can't write test file for %s.\n", uFormat);\
        return;\
    }\
    /* Reinitialize the buffer to verify null termination works. */\
    u_memset(uBuffer, 0x2a, sizeof(uBuffer)/sizeof(*uBuffer));\
    memset(buffer, '*', sizeof(buffer)/sizeof(*buffer));\
    \
    uNumPrinted = u_fprintf(myFile, uFormat, uValue);\
    u_fclose(myFile);\
    myFile = u_fopen(STANDARD_TEST_FILE, "r", "en_US_POSIX", NULL);\
    u_fgets(uBuffer, sizeof(uBuffer)/sizeof(*uBuffer), myFile);\
    u_fclose(myFile);\
    u_austrncpy(compBuffer, uBuffer, sizeof(uBuffer)/sizeof(*uBuffer));\
    cNumPrinted = sprintf(buffer, cFormat, cValue);\
    if (strcmp(buffer, compBuffer) != 0) {\
        log_err("%" uFormat " Got: \"%s\", Expected: \"%s\"\n", compBuffer, buffer);\
    }\
    if (cNumPrinted != uNumPrinted) {\
        log_err("%" uFormat " number printed Got: %d, Expected: %d\n", uNumPrinted, cNumPrinted);\
    }\
    if (buffer[uNumPrinted+1] != '*') {\
        log_err("%" uFormat " too much stored\n");\
    }\

#if !UCONFIG_NO_FORMATTING
static void TestFprintfFormat(void) {
    static const UChar abcUChars[] = {0x61,0x62,0x63,0};
    static const char abcChars[] = "abc";
    UChar uBuffer[256];
    char buffer[256];
    char compBuffer[256];
    int32_t uNumPrinted;
    int32_t cNumPrinted;
    UFILE *myFile;

    TestFPrintFormat("%8S", abcUChars, "%8s", abcChars);
    TestFPrintFormat("%-8S", abcUChars, "%-8s", abcChars);
    TestFPrintFormat("%.2S", abcUChars, "%.2s", abcChars); /* strlen is 3 */

    TestFPrintFormat("%8s", abcChars, "%8s", abcChars);
    TestFPrintFormat("%-8s", abcChars, "%-8s", abcChars);
    TestFPrintFormat("%.2s", abcChars, "%.2s", abcChars); /* strlen is 3 */

    TestFPrintFormat("%8c", (char)'e', "%8c", (char)'e');
    TestFPrintFormat("%-8c", (char)'e', "%-8c", (char)'e');

    TestFPrintFormat("%8C", (UChar)0x65, "%8c", (char)'e');
    TestFPrintFormat("%-8C", (UChar)0x65, "%-8c", (char)'e');

    TestFPrintFormat("%f", 1.23456789, "%f", 1.23456789);
    TestFPrintFormat("%f", 12345.6789, "%f", 12345.6789);
    TestFPrintFormat("%f", 123456.789, "%f", 123456.789);
    TestFPrintFormat("%f", 1234567.89, "%f", 1234567.89);
    TestFPrintFormat("%10f", 1.23456789, "%10f", 1.23456789);
    TestFPrintFormat("%-10f", 1.23456789, "%-10f", 1.23456789);
    TestFPrintFormat("%10f", 123.456789, "%10f", 123.456789);
    TestFPrintFormat("%10.4f", 123.456789, "%10.4f", 123.456789);
    TestFPrintFormat("%-10f", 123.456789, "%-10f", 123.456789);

/*    TestFPrintFormat("%g", 12345.6789, "%g", 12345.6789);
    TestFPrintFormat("%g", 123456.789, "%g", 123456.789);
    TestFPrintFormat("%g", 1234567.89, "%g", 1234567.89);
    TestFPrintFormat("%G", 123456.789, "%G", 123456.789);
    TestFPrintFormat("%G", 1234567.89, "%G", 1234567.89);*/
    TestFPrintFormat("%10g", 1.23456789, "%10g", 1.23456789);
    TestFPrintFormat("%10.4g", 1.23456789, "%10.4g", 1.23456789);
    TestFPrintFormat("%-10g", 1.23456789, "%-10g", 1.23456789);
    TestFPrintFormat("%10g", 123.456789, "%10g", 123.456789);
    TestFPrintFormat("%-10g", 123.456789, "%-10g", 123.456789);

    TestFPrintFormat("%8x", 123456, "%8x", 123456);
    TestFPrintFormat("%-8x", 123456, "%-8x", 123456);
    TestFPrintFormat("%08x", 123456, "%08x", 123456);

    TestFPrintFormat("%8X", 123456, "%8X", 123456);
    TestFPrintFormat("%-8X", 123456, "%-8X", 123456);
    TestFPrintFormat("%08X", 123456, "%08X", 123456);
    TestFPrintFormat("%#x", 123456, "%#x", 123456);
    TestFPrintFormat("%#x", -123456, "%#x", -123456);

    TestFPrintFormat("%8o", 123456, "%8o", 123456);
    TestFPrintFormat("%-8o", 123456, "%-8o", 123456);
    TestFPrintFormat("%08o", 123456, "%08o", 123456);
    TestFPrintFormat("%#o", 123, "%#o", 123);
    TestFPrintFormat("%#o", -123, "%#o", -123);

    TestFPrintFormat("%8u", 123456, "%8u", 123456);
    TestFPrintFormat("%-8u", 123456, "%-8u", 123456);
    TestFPrintFormat("%08u", 123456, "%08u", 123456);
    TestFPrintFormat("%8u", -123456, "%8u", -123456);
    TestFPrintFormat("%-8u", -123456, "%-8u", -123456);
    TestFPrintFormat("%.5u", 123456, "%.5u", 123456);
    TestFPrintFormat("%.6u", 123456, "%.6u", 123456);
    TestFPrintFormat("%.7u", 123456, "%.7u", 123456);

    TestFPrintFormat("%8d", 123456, "%8d", 123456);
    TestFPrintFormat("%-8d", 123456, "%-8d", 123456);
    TestFPrintFormat("%08d", 123456, "%08d", 123456);
    TestFPrintFormat("% d", 123456, "% d", 123456);
    TestFPrintFormat("% d", -123456, "% d", -123456);

    TestFPrintFormat("%8i", 123456, "%8i", 123456);
    TestFPrintFormat("%-8i", 123456, "%-8i", 123456);
    TestFPrintFormat("%08i", 123456, "%08i", 123456);

    log_verbose("Get really crazy with the formatting.\n");

    TestFPrintFormat("%-#12x", 123, "%-#12x", 123);
    TestFPrintFormat("%-#12x", -123, "%-#12x", -123);
    TestFPrintFormat("%#12x", 123, "%#12x", 123);
    TestFPrintFormat("%#12x", -123, "%#12x", -123);

    TestFPrintFormat("%-+12d", 123,  "%-+12d", 123);
    TestFPrintFormat("%-+12d", -123, "%-+12d", -123);
    TestFPrintFormat("%- 12d", 123,  "%- 12d", 123);
    TestFPrintFormat("%- 12d", -123, "%- 12d", -123);
    TestFPrintFormat("%+12d", 123,   "%+12d", 123);
    TestFPrintFormat("%+12d", -123,  "%+12d", -123);
    TestFPrintFormat("% 12d", 123,   "% 12d", 123);
    TestFPrintFormat("% 12d", -123,  "% 12d", -123);
    TestFPrintFormat("%12d", 123,    "%12d", 123);
    TestFPrintFormat("%12d", -123,   "%12d", -123);
    TestFPrintFormat("%.12d", 123,   "%.12d", 123);
    TestFPrintFormat("%.12d", -123,  "%.12d", -123);

    TestFPrintFormat("%-+12.1f", 1.234,  "%-+12.1f", 1.234);
    TestFPrintFormat("%-+12.1f", -1.234, "%-+12.1f", -1.234);
    TestFPrintFormat("%- 12.10f", 1.234, "%- 12.10f", 1.234);
    TestFPrintFormat("%- 12.1f", -1.234, "%- 12.1f", -1.234);
    TestFPrintFormat("%+12.1f", 1.234,   "%+12.1f", 1.234);
    TestFPrintFormat("%+12.1f", -1.234,  "%+12.1f", -1.234);
    TestFPrintFormat("% 12.1f", 1.234,   "% 12.1f", 1.234);
    TestFPrintFormat("% 12.1f", -1.234,  "% 12.1f", -1.234);
    TestFPrintFormat("%12.1f", 1.234,    "%12.1f", 1.234);
    TestFPrintFormat("%12.1f", -1.234,   "%12.1f", -1.234);
    TestFPrintFormat("%.2f", 1.234,      "%.2f", 1.234);
    TestFPrintFormat("%.2f", -1.234,     "%.2f", -1.234);
    TestFPrintFormat("%3f", 1.234,       "%3f", 1.234);
    TestFPrintFormat("%3f", -1.234,      "%3f", -1.234);

    myFile = u_fopen(STANDARD_TEST_FILE, "w", "en_US_POSIX", NULL);
    /* Reinitialize the buffer to verify null termination works. */
    u_memset(uBuffer, 0x2a, sizeof(uBuffer)/sizeof(*uBuffer));
    memset(buffer, '*', sizeof(buffer)/sizeof(*buffer));
    
    uNumPrinted = u_fprintf(myFile, "%d % d %d", -1234, 1234, 1234);
    u_fclose(myFile);
    myFile = u_fopen(STANDARD_TEST_FILE, "r", "en_US_POSIX", NULL);
    u_fgets(uBuffer, sizeof(uBuffer)/sizeof(*uBuffer), myFile);
    u_fclose(myFile);
    u_austrncpy(compBuffer, uBuffer, sizeof(uBuffer)/sizeof(*uBuffer));
    cNumPrinted = sprintf(buffer, "%d % d %d", -1234, 1234, 1234);
    if (strcmp(buffer, compBuffer) != 0) {
        log_err("%%d %% d %%d Got: \"%s\", Expected: \"%s\"\n", compBuffer, buffer);
    }
    if (cNumPrinted != uNumPrinted) {
        log_err("%%d %% d %%d number printed Got: %d, Expected: %d\n", uNumPrinted, cNumPrinted);
    }
    if (buffer[uNumPrinted+1] != '*') {
        log_err("%%d %% d %%d too much stored\n");
    }
}
#endif

#undef TestFPrintFormat

#if !UCONFIG_NO_FORMATTING
static void TestFScanSetFormat(const char *format, const UChar *uValue, const char *cValue, UBool expectedToPass) {
    UFILE *myFile;
    UChar uBuffer[256];
    char buffer[256];
    char compBuffer[256];
    int32_t uNumScanned;
    int32_t cNumScanned;

    myFile = u_fopen(STANDARD_TEST_FILE, "w", NULL, NULL);
    if (myFile == NULL) {
        log_err("Can't write test file for %s.\n", format);
        return;
    }
    /* Reinitialize the buffer to verify null termination works. */
    u_memset(uBuffer, 0x2a, sizeof(uBuffer)/sizeof(*uBuffer));
    uBuffer[sizeof(uBuffer)/sizeof(*uBuffer)-1] = 0;
    memset(buffer, '*', sizeof(buffer)/sizeof(*buffer));
    buffer[sizeof(buffer)/sizeof(*buffer)-1] = 0;
    
    u_fprintf(myFile, "%S", uValue);
    u_fclose(myFile);
    myFile = u_fopen(STANDARD_TEST_FILE, "r", "en_US_POSIX", NULL);
    uNumScanned = u_fscanf(myFile, format, uBuffer);
    u_fclose(myFile);
    if (expectedToPass) {
        u_austrncpy(compBuffer, uBuffer, sizeof(uBuffer)/sizeof(*uBuffer));
        cNumScanned = sscanf(cValue, format, buffer);
        if (strncmp(buffer, compBuffer, sizeof(buffer)/sizeof(*buffer)) != 0) {
            log_err("%s Got: \"%s\", Expected: \"%s\"\n", format, compBuffer, buffer);
        }
        if (cNumScanned != uNumScanned) {
            log_err("%s number printed Got: %d, Expected: %d\n", format, uNumScanned, cNumScanned);
        }
        if (uNumScanned > 0 && uBuffer[u_strlen(uBuffer)+1] != 0x2a) {
            log_err("%s too much stored\n", format);
        }
    }
    else {
        if (uNumScanned != 0 || uBuffer[0] != 0x2a || uBuffer[1] != 0x2a) {
            log_err("%s too much stored on a failure\n", format);
        }
    }
}
#endif

#if !UCONFIG_NO_FORMATTING
static void TestFScanset(void) {
    static const UChar abcUChars[] = {0x61,0x62,0x63,0x63,0x64,0x65,0x66,0x67,0};
    static const char abcChars[] = "abccdefg";

    TestFScanSetFormat("%[bc]S", abcUChars, abcChars, TRUE);
    TestFScanSetFormat("%[cb]S", abcUChars, abcChars, TRUE);

    TestFScanSetFormat("%[ab]S", abcUChars, abcChars, TRUE);
    TestFScanSetFormat("%[ba]S", abcUChars, abcChars, TRUE);

    TestFScanSetFormat("%[ab]", abcUChars, abcChars, TRUE);
    TestFScanSetFormat("%[ba]", abcUChars, abcChars, TRUE);

    TestFScanSetFormat("%[abcdefgh]", abcUChars, abcChars, TRUE);
    TestFScanSetFormat("%[;hgfedcba]", abcUChars, abcChars, TRUE);

    TestFScanSetFormat("%[^a]", abcUChars, abcChars, TRUE);
    TestFScanSetFormat("%[^e]", abcUChars, abcChars, TRUE);
    TestFScanSetFormat("%[^ed]", abcUChars, abcChars, TRUE);
    TestFScanSetFormat("%[^dc]", abcUChars, abcChars, TRUE);
    TestFScanSetFormat("%[^e]  ", abcUChars, abcChars, TRUE);

    TestFScanSetFormat("%1[ab]  ", abcUChars, abcChars, TRUE);
    TestFScanSetFormat("%2[^f]", abcUChars, abcChars, TRUE);

    TestFScanSetFormat("%[qrst]", abcUChars, abcChars, TRUE);

    /* Extra long string for testing */
    TestFScanSetFormat("                                                                                                                         %[qrst]",
        abcUChars, abcChars, TRUE);

    TestFScanSetFormat("%[a-]", abcUChars, abcChars, TRUE);

    /* Bad format */
    TestFScanSetFormat("%[f-a]", abcUChars, abcChars, FALSE);
    TestFScanSetFormat("%[c-a]", abcUChars, abcChars, FALSE);
    TestFScanSetFormat("%[a", abcUChars, abcChars, FALSE);
    /* The following is not deterministic on Windows */
/*    TestFScanSetFormat("%[a-", abcUChars, abcChars);*/

    /* TODO: Need to specify precision with a "*" */
}
#endif
#if !UCONFIG_NO_FORMATTING
static void TestBadFScanfFormat(const char *format, const UChar *uValue, const char *cValue) {
    UFILE *myFile;
    UChar uBuffer[256];
    int32_t uNumScanned;

    myFile = u_fopen(STANDARD_TEST_FILE, "w", NULL, NULL);
    if (myFile == NULL) {
        log_err("Can't write test file for %s.\n", format);
        return;
    }
    /* Reinitialize the buffer to verify null termination works. */
    u_memset(uBuffer, 0x2a, sizeof(uBuffer)/sizeof(*uBuffer));
    uBuffer[sizeof(uBuffer)/sizeof(*uBuffer)-1] = 0;
    
    u_fprintf(myFile, "%S", uValue);
    u_fclose(myFile);
    myFile = u_fopen(STANDARD_TEST_FILE, "r", "en_US_POSIX", NULL);
    uNumScanned = u_fscanf(myFile, format, uBuffer);
    u_fclose(myFile);
    if (uNumScanned != 0 || uBuffer[0] != 0x2a || uBuffer[1] != 0x2a) {
        log_err("%s too much stored on a failure\n", format);
    }
}
#endif
#if !UCONFIG_NO_FORMATTING
static void TestBadScanfFormat(void) {
    static const UChar abcUChars[] = {0x61,0x62,0x63,0x63,0x64,0x65,0x66,0x67,0};
    static const char abcChars[] = "abccdefg";

    TestBadFScanfFormat("%[]  ", abcUChars, abcChars);
}
#endif
#if !UCONFIG_NO_FORMATTING
static void Test_u_vfprintf(const char *expectedResult, const char *format, ...) {
    UChar uBuffer[256];
    UChar uBuffer2[256];
    va_list ap;
    int32_t count;
    UFILE *myFile;

    myFile = u_fopen(STANDARD_TEST_FILE, "w", "en_US_POSIX", "UTF-8");
    if (!myFile) {
        log_err("Test file can't be opened\n");
        return;
    }

    va_start(ap, format);
    count = u_vfprintf(myFile, format, ap);
    va_end(ap);

    u_fclose(myFile);


    myFile = u_fopen(STANDARD_TEST_FILE, "r", "en_US_POSIX", "UTF-8");
    if (!myFile) {
        log_err("Test file can't be opened\n");
        return;
    }
    u_fgets(uBuffer, sizeof(uBuffer)/sizeof(*uBuffer), myFile);
    u_uastrcpy(uBuffer2, expectedResult);
    if (u_strcmp(uBuffer, uBuffer2) != 0) {
        log_err("Got two different results for \"%s\" expected \"%s\"\n", format, expectedResult);
    }
    u_fclose(myFile);


    myFile = u_fopen(STANDARD_TEST_FILE, "w", "en_US_POSIX", NULL);
    if (!myFile) {
        log_err("Test file can't be opened\n");
        return;
    }
    u_uastrcpy(uBuffer, format);

    va_start(ap, format);
    count = u_vfprintf_u(myFile, uBuffer, ap);
    va_end(ap);

    u_fclose(myFile);


    myFile = u_fopen(STANDARD_TEST_FILE, "r", "en_US_POSIX", NULL);
    if (!myFile) {
        log_err("Test file can't be opened\n");
        return;
    }
    u_fgets(uBuffer, sizeof(uBuffer)/sizeof(*uBuffer), myFile);
    u_uastrcpy(uBuffer2, expectedResult);
    if (u_strcmp(uBuffer, uBuffer2) != 0) {
        log_err("Got two different results for \"%s\" expected \"%s\"\n", format, expectedResult);
    }
    u_fclose(myFile);
}

static void TestVargs(void) {
    Test_u_vfprintf("8 9 a B 8.9", "%d %u %x %X %.1f", 8, 9, 10, 11, 8.9);
}
#endif

static void TestUnicodeFormat(void)
{
#if !UCONFIG_NO_FORMATTING
    /* Make sure that invariant conversion doesn't happen on the _u formats. */
    UChar myUString[256];
    UFILE *myFile;
    static const UChar TEST_STR[] = { 0x03BC, 0x0025, 0x0024, 0};
    static const UChar PERCENT_S[] = { 0x03BC, 0x0025, 0x0053, 0};

    u_memset(myUString, 0x2a, sizeof(myUString)/sizeof(*myUString));

    myFile = u_fopen(STANDARD_TEST_FILE, "w", NULL, "UTF-8");
    if (!myFile) {
        log_err("Test file can't be opened\n");
        return;
    }
    u_fprintf_u(myFile, PERCENT_S, TEST_STR);
    u_fclose(myFile);

    myFile = u_fopen(STANDARD_TEST_FILE, "r", NULL, "UTF-8");
    if (!myFile) {
        log_err("Test file can't be opened\n");
        return;
    }
    u_fscanf_u(myFile, PERCENT_S, myUString);
    u_fclose(myFile);
    if (u_strcmp(TEST_STR, myUString) != 0) {
        log_err("u_fscanf_u doesn't work.\n");
    }
#endif
}

static void TestFileWriteRetval(const char * a_pszEncoding) { 
    UChar * buffer; 
    UFILE * myFile; 
    int32_t count; 
    int32_t expected = 10000; /* test with large data to test internal buffer looping */ 
    UChar   testChar = 0xBEEF; 

    if (!*a_pszEncoding || 0 == strcmp(a_pszEncoding, "ASCII")) { 
        testChar = 0x65; /* 'A' - otherwise read test will fail */ 
    } 

    buffer = (UChar*) malloc(expected * sizeof(UChar)); 
    if (!buffer) { 
        log_err("Out of memory\n"); 
        return; 
    } 

    /* write */ 
    myFile = u_fopen(STANDARD_TEST_FILE, "w", NULL, a_pszEncoding); 
    if (!myFile) { 
        free(buffer); 
        log_err("Test file can't be opened for write\n"); 
        return; 
    } 
    u_memset(buffer, testChar, expected); 
    count = u_file_write(buffer, expected, myFile); 
    u_fclose(myFile); 
    if (count != expected) { 
        free(buffer); 
        log_err("u_file_write returned incorrect number of characters written\n"); 
        return; 
    } 

    free(buffer); 
    buffer = NULL; 

    /* read */ 
    myFile = u_fopen(STANDARD_TEST_FILE, "r", NULL, a_pszEncoding); 
    if (!myFile) { 
        log_err("Test file can't be opened for read\n"); 
        return; 
    } 
    for (count = 0; count < expected; ++count) { 
        UChar gotChar = u_fgetc(myFile);
	if(gotChar != testChar) {
            log_err("u_fgetc returned unexpected character U+%04X expected U+%04X\n", gotChar, testChar); 
            u_fclose(myFile); 
            return; 
        } 
    } 
    if (u_fgetc(myFile) != U_EOF) { 
        log_err("u_fgetc did not return expected EOF\n"); 
        u_fclose(myFile); 
        return; 
        } 
    u_fclose(myFile); 
} 

static void TestFileWriteRetvalUTF16(void) { 
    TestFileWriteRetval("UTF-16"); 
} 

static void TestFileWriteRetvalUTF8(void) { 
    TestFileWriteRetval("UTF-8"); 
} 

static void TestFileWriteRetvalASCII(void) { 
    TestFileWriteRetval("ASCII"); 
} 

static void TestFileWriteRetvalNONE(void) { 
    TestFileWriteRetval(""); 
} 

U_CFUNC void
addFileTest(TestNode** root) {
#if !UCONFIG_NO_FORMATTING
    addTest(root, &TestFile, "file/TestFile");
#endif
    addTest(root, &StdinBuffering, "file/StdinBuffering");
    addTest(root, &TestfgetsBuffers, "file/TestfgetsBuffers");
    addTest(root, &TestFileReadBuffering, "file/TestFileReadBuffering");
    addTest(root, &TestfgetsLineCount, "file/TestfgetsLineCount");
    addTest(root, &TestfgetsNewLineHandling, "file/TestfgetsNewLineHandling");
    addTest(root, &TestfgetsNewLineCount, "file/TestfgetsNewLineCount");
    addTest(root, &TestFgetsLineBuffering, "file/TestFgetsLineBuffering");
    addTest(root, &TestCodepage, "file/TestCodepage");
    addTest(root, &TestCodepageFlush, "file/TestCodepageFlush");
    addTest(root, &TestFileWriteRetvalUTF16, "file/TestFileWriteRetvalUTF16");
    addTest(root, &TestFileWriteRetvalUTF8, "file/TestFileWriteRetvalUTF8");
    addTest(root, &TestFileWriteRetvalASCII, "file/TestFileWriteRetvalASCII");
    addTest(root, &TestFileWriteRetvalNONE, "file/TestFileWriteRetvalNONE");
#if !UCONFIG_NO_FORMATTING
    addTest(root, &TestCodepageAndLocale, "file/TestCodepageAndLocale");
    addTest(root, &TestFprintfFormat, "file/TestFprintfFormat");
    addTest(root, &TestFScanset, "file/TestFScanset");
    addTest(root, &TestFilePrintCompatibility, "file/TestFilePrintCompatibility");
    addTest(root, &TestBadScanfFormat, "file/TestBadScanfFormat");
    addTest(root, &TestVargs, "file/TestVargs");
    addTest(root, &TestUnicodeFormat, "file/TestUnicodeFormat");
#endif
}
