/*
**********************************************************************
*   Copyright (C) 2005-2005, International Business Machines
*   Corporation and others.  All Rights Reserved.
**********************************************************************
*   file name:  strtst.c
*   created by: George Rhoten
*/

#include "iotest.h"
#include "unicode/ustdio.h"
#include "unicode/utrans.h"
#include "unicode/ustring.h"

static void TestTranslitOps(void)
{
#if !UCONFIG_NO_TRANSLITERATION
    UFILE *f;
    UErrorCode err = U_ZERO_ERROR;
    UTransliterator *a = NULL, *b = NULL, *c = NULL;

    log_verbose("opening a transliterator and UFILE for testing\n");

    f = u_fopen(STANDARD_TEST_FILE, "w", "en_US_POSIX", NULL);
    if(f == NULL)
    {
        log_err("Couldn't open test file for writing\n");
        return;
    }

    a = utrans_open("Latin-Greek", UTRANS_FORWARD, NULL, -1, NULL, &err);
    if(U_FAILURE(err))
    {
        log_err("Error opening transliterator %s\n", u_errorName(err));
        u_fclose(f);
        return;
    }


    log_verbose("setting a transliterator\n");
    b = u_fsettransliterator(f, U_WRITE, a, &err);
    if(U_FAILURE(err))
    {
        log_err("Error setting transliterator %s\n", u_errorName(err));
        u_fclose(f);
        return;
    }

    if(b != NULL)
    {
        log_err("Error, a transliterator was already set!\n");
    }

    b = u_fsettransliterator(NULL, U_WRITE, a, &err);
    if(err != U_ILLEGAL_ARGUMENT_ERROR)
    {
        log_err("Error setting transliterator on NULL file err=%s\n", u_errorName(err));
    }

    if(b != a)
    {
        log_err("Error getting the same transliterator was not returned on NULL file\n");
    }

    err = U_FILE_ACCESS_ERROR;
    b = u_fsettransliterator(f, U_WRITE, a, &err);
    if(err != U_FILE_ACCESS_ERROR)
    {
        log_err("Error setting transliterator on error status err=%s\n", u_errorName(err));
    }

    if(b != a)
    {
        log_err("Error getting the same transliterator on error status\n");
    }
    err = U_ZERO_ERROR;


    log_verbose("un-setting transliterator (setting to null)\n");
    c = u_fsettransliterator(f, U_WRITE, NULL, &err);
    if(U_FAILURE(err))
    {
        log_err("Err setting transliterator %s\n", u_errorName(err));
        u_fclose(f);
        return;
    }

    if(c != a)
    {
        log_err("Err, transliterator that came back was not the original one.\n");
    }

    log_verbose("Trying to set read transliterator (should fail)\n");
    b = u_fsettransliterator(f, U_READ, NULL, &err);
    if(err != U_UNSUPPORTED_ERROR)
    {
        log_err("Should have U_UNSUPPORTED_ERROR setting  Read transliterator but got %s - REVISIT AND UPDATE TEST\n", u_errorName(err));
        u_fclose(f);
        return;
    }
    else
    {
        log_verbose("Got %s error (expected) setting READ transliterator.\n", u_errorName(err));
        err = U_ZERO_ERROR;
    }


    utrans_close(c);
    u_fclose(f);
#endif
}

static void TestTranslitFileOut(void)
{
#if !UCONFIG_NO_FORMATTING
#if !UCONFIG_NO_TRANSLITERATION
    UFILE *f;
    UErrorCode err = U_ZERO_ERROR;
    UTransliterator *a = NULL, *b = NULL, *c = NULL;
    FILE *infile;
    UChar compare[] = { 0xfeff, 0x03a3, 0x03c4, 0x03b5, 0x03c6, 0x1f00, 0x03bd, 0x03bf, 0x03c2, 0x043C, 0x0000 };
    UChar ubuf[256];
    int len;

    log_verbose("opening a transliterator and UFILE for testing\n");

    f = u_fopen(STANDARD_TEST_FILE, "w", "en_US_POSIX", "utf-16");
    if(f == NULL)
    {
        log_err("Couldn't open test file for writing\n");
        return;
    }

    a = utrans_open("Latin-Greek", UTRANS_FORWARD, NULL, -1, NULL, &err);
    if(U_FAILURE(err))
    {
        log_err("Err opening transliterator %s\n", u_errorName(err));
        u_fclose(f);
        return;
    }

    log_verbose("setting a transliterator\n");
    b = u_fsettransliterator(f, U_WRITE, a, &err);
    if(U_FAILURE(err))
    {
        log_err("Err setting transliterator %s\n", u_errorName(err));
        u_fclose(f);
        return;
    }

    if(b != NULL)
    {
        log_err("Err, a transliterator was already set!\n");
    }

    u_fprintf(f, "Stephanos");

    c = utrans_open("Latin-Cyrillic", UTRANS_FORWARD, NULL, -1, NULL, &err);
    if(U_FAILURE(err))
    {
        log_err("Err opening transliterator %s\n", u_errorName(err));
        u_fclose(f);
        return;
    }

    log_verbose("setting a transliterator\n");
    b = u_fsettransliterator(f, U_WRITE, c, &err);
    if(U_FAILURE(err))
    {
        log_err("Err setting transliterator %s\n", u_errorName(err));
        u_fclose(f);
        return;
    }

    if(b != a)
    {
        log_err("Error: a different transliterator was returned!\n");
    }
    utrans_close(b);

    u_fprintf(f, "m");

    u_fclose(f);

    log_verbose("Re reading test file to verify transliteration\n");
    infile = fopen(STANDARD_TEST_FILE, "rb");
    if(infile == NULL)
    {
        log_err("Couldn't reopen test file\n");
        return;
    }

    len=fread(ubuf, sizeof(UChar), u_strlen(compare), infile);
    log_verbose("Read %d UChars\n", len);
    if(len != u_strlen(compare))
    {
        log_err("Wanted %d UChars from file, got %d\n", u_strlen(compare), len);
    }
    ubuf[len]=0;

    if(u_strlen(compare) != u_strlen(ubuf))
    {
        log_err("Wanted %d UChars from file, but u_strlen() returns %d\n", u_strlen(compare), len);
    }

    if(u_strcmp(compare, ubuf))
    {
        log_err("Read string doesn't match expected.\n");
    }
    else
    {
        log_verbose("Read string matches expected.\n");
    }

    fclose(infile);
#endif
#endif
}

static void TestTranslitStringOut(void)
{
#if !UCONFIG_NO_FORMATTING
#if !UCONFIG_NO_TRANSLITERATION
    UFILE *f;
    UErrorCode err = U_ZERO_ERROR;
    UTransliterator *a = NULL, *b = NULL, *c = NULL;
    UChar compare[] = { 0x03a3, 0x03c4, 0x03b5, 0x03c6, 0x1f00, 0x03bd, 0x03bf, 0x03c2, 0x043C, 0x0000 };
    UChar ubuf[256];

    log_verbose("opening a transliterator and UFILE for testing\n");

    f = u_fstropen(ubuf, sizeof(ubuf)/sizeof(ubuf[0]), "en_US_POSIX");
    if(f == NULL)
    {
        log_err("Couldn't open test file for writing\n");
        return;
    }

    a = utrans_open("Latin-Greek", UTRANS_FORWARD, NULL, -1, NULL, &err);
    if(U_FAILURE(err))
    {
        log_err("Err opening transliterator %s\n", u_errorName(err));
        u_fclose(f);
        return;
    }

    log_verbose("setting a transliterator\n");
    b = u_fsettransliterator(f, U_WRITE, a, &err);
    if(U_FAILURE(err))
    {
        log_err("Err setting transliterator %s\n", u_errorName(err));
        u_fclose(f);
        return;
    }

    if(b != NULL)
    {
        log_err("Err, a transliterator was already set!\n");
    }

    u_fprintf(f, "Stephanos");

    c = utrans_open("Latin-Cyrillic", UTRANS_FORWARD, NULL, -1, NULL, &err);
    if(U_FAILURE(err))
    {
        log_err("Err opening transliterator %s\n", u_errorName(err));
        u_fclose(f);
        return;
    }

    log_verbose("setting a transliterator\n");
    b = u_fsettransliterator(f, U_WRITE, c, &err);
    if(U_FAILURE(err))
    {
        log_err("Err setting transliterator %s\n", u_errorName(err));
        u_fclose(f);
        return;
    }

    if(b != a)
    {
        log_err("Error: a different transliterator was returned!\n");
    }
    utrans_close(b);

    u_fprintf(f, "m");

    u_fclose(f);

    if(u_strlen(compare) != u_strlen(ubuf))
    {
        log_err("Wanted %d UChars from file, but u_strlen() returns %d\n", u_strlen(compare), u_strlen(ubuf));
    }

    if(u_strcmp(compare, ubuf))
    {
        log_err("Read string doesn't match expected.\n");
    }
    else
    {
        log_verbose("Read string matches expected.\n");
    }
#endif
#endif
}

U_CFUNC void
addTranslitTest(TestNode** root) {
#if !UCONFIG_NO_TRANSLITERATION
    addTest(root, &TestTranslitOps, "translit/ops");
#if !UCONFIG_NO_FORMATTING
    addTest(root, &TestTranslitFileOut, "translit/fileOut");
    addTest(root, &TestTranslitStringOut, "translit/stringOut");
#endif
#endif
}
