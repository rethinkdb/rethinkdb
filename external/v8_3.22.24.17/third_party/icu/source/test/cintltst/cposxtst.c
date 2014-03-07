/*
**********************************************************************
* Copyright (c) 2003-2009, International Business Machines
* Corporation and others.  All Rights Reserved.
**********************************************************************
* Author: Alan Liu
* Created: March 20 2003
* Since: ICU 2.6
**********************************************************************
*/
#include "cintltst.h"
#include "cmemory.h"
#include "cstring.h"
#include "unicode/ucat.h"
#include "unicode/ustring.h"

/**********************************************************************/
/* Prototypes */

void TestMessageCatalog(void);

/**********************************************************************/
/* Add our tests into the hierarchy */

void addPosixTest(TestNode** root);

void addPosixTest(TestNode** root)
{
#if !UCONFIG_NO_FILE_IO && !UCONFIG_NO_LEGACY_CONVERSION
    addTest(root, &TestMessageCatalog, "tsutil/cposxtst/TestMessageCatalog");
#endif
}

/**********************************************************************/
/* Test basic ucat.h API */

void TestMessageCatalog(void) {

    UErrorCode ec = U_ZERO_ERROR;
    u_nl_catd catd;
    const char* DATA[] = {
        /* set_num, msg_num, expected string result, expected error code */
        "1", "4", "Good morning.", "U_ZERO_ERROR",
        "1", "5", "Good afternoon.", "U_ZERO_ERROR",
        "1", "6", "FAIL", "U_MISSING_RESOURCE_ERROR",
        "1", "7", "Good evening.", "U_ZERO_ERROR",
        "1", "8", "Good night.", "U_ZERO_ERROR",
        "1", "9", "FAIL", "U_MISSING_RESOURCE_ERROR",
        "3", "1", "FAIL", "U_MISSING_RESOURCE_ERROR",
        "4", "14", "Please ", "U_ZERO_ERROR",
        "4", "15", "FAIL", "U_MISSING_RESOURCE_ERROR",
        "4", "19", "Thank you.", "U_ZERO_ERROR",
        "4", "20", "Sincerely,", "U_ZERO_ERROR",
        NULL
    };
    const UChar FAIL[] = {0x46, 0x41, 0x49, 0x4C, 0x00}; /* "FAIL" */
    int32_t i;
    const char *path = loadTestData(&ec);
 
    if (U_FAILURE(ec)) {
        log_data_err("FAIL: loadTestData => %s\n", u_errorName(ec));
        return;
    }

    catd = u_catopen(path, "mc", &ec);
    if (U_FAILURE(ec)) {
        log_data_err("FAIL: u_catopen => %s\n", u_errorName(ec));
        return;
    }

    for (i=0; DATA[i]!=NULL; i+=4) {
        int32_t set_num = T_CString_stringToInteger(DATA[i], 10);
        int32_t msg_num = T_CString_stringToInteger(DATA[i+1], 10);
        UChar exp[128];
        int32_t len = -1;
        const char* err;
        char str[128];
        const UChar* ustr;

        u_uastrcpy(exp, DATA[i+2]);

        ec = U_ZERO_ERROR;
        ustr = u_catgets(catd, set_num, msg_num, FAIL, &len, &ec);
        u_austrcpy(str, ustr);
        err = u_errorName(ec);
        
        log_verbose("u_catgets(%d, %d) => \"%s\", len=%d, %s\n",
                    set_num, msg_num, str, len, err);

        if (u_strcmp(ustr, exp) != 0) {
            log_err("FAIL: u_catgets => \"%s\", exp. \"%s\"\n",
                    str, DATA[i+2]);
        }

        if (len != (int32_t) uprv_strlen(DATA[i+2])) {
            log_err("FAIL: u_catgets => len=%d, exp. %d\n",
                    len, uprv_strlen(DATA[i+2]));
        }

        if (uprv_strcmp(err, DATA[i+3]) != 0) {
            log_err("FAIL: u_catgets => %s, exp. %s\n",
                    err, DATA[i+3]);
        }
    }

    u_catclose(catd);
}

/*eof*/
