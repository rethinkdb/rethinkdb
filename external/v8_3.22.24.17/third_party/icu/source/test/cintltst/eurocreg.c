/********************************************************************
 * COPYRIGHT: 
 * Copyright (c) 1999-2006, International Business Machines Corporation and
 * others. All Rights Reserved.
 ********************************************************************/
#include "unicode/utypes.h"
#include "unicode/ustring.h"
#include "unicode/ctest.h"
#include "unicode/ucnv.h"

void TestEuroRegression(void);
void addTestEuroRegression(TestNode** root);

#if !UCONFIG_NO_LEGACY_CONVERSION
void addTestEuroRegression(TestNode** root)
{
    addTest(root, &TestEuroRegression, "tsconv/eurocreg/TestEuroRegression");
}

/*
 * The table below lists codepages that are supposed to have roundtrip mappings for
 * the U+20AC Euro sign.
 *
 * Changes made 2000nov28 and marked as such are due to the following:
 *
 * After updating all ibm-*.ucm files with precise fallback indicators (|0, |1, |3),
 * some of these codepages failed the Euro regression test.
 * This means that the actuall mappings changed when only the preciseness of fallback
 * mappings should have changed.
 * My (Markus) suspicion is that some files got Euro sign mappings added manually,
 * changing their contents compared to the NLTC (IBM Toronto codepage database) definition.
 * Such changes are highly undesirable because they effectively define new codepages.
 * Codepage mapping files with "ibm-*.ucm" should always exactly match the files
 * from the IBM codepage database.
 * (If there are several mappings with the same number, then we choose the
 * default mappings with Private-Use Area assignments.)
 *
 * Also, in the past, some aliases were set such that e.g. cp850 became an alias for ibm-858.
 * This followed the practice of OS/2 that uses the old codepage number 850 for the new
 * codepage 858, with the main difference being the additional Euro sign.
 * However, we have documented that the "cp" prefix should be used for Microsoft-compatible
 * codepages, and Microsoft Windows 2000's codepage 850 does not contain a Euro sign mapping.
 * Therefore, cp850 must not support the Euro sign.
 * In these cases, I have changed the codepage name here to point to a newer codepage with the
 * Euro sign, using its new name.
 * I could not find such "updates" for codepages 1362 and 1363 - we might want to supply them later.
 */

static const char convertersToCheck[][15] = { 
  "cp1250",
  "cp1251",
  "cp1252",
  "cp1254",
  "cp1255",
  "cp1256",
  "cp1257",
  "cp1258",
  "ibm1140",
  "ibm1142",
  "ibm1143",
  "ibm1144",
  "ibm1145",
  "ibm1146",
  "ibm1147",
  "ibm1148",
  "ibm1149",
  "ibm1153",
  "ibm1154",
  "ibm1155",
  "ibm1156",
  "ibm1157",
  "ibm1158",
  /*"ibm-1159",*/ /* removed 2003Apr17 */
  "ibm12712",
  "ibm16804",
  "ibm-1160",
  "ibm-1162",
  "ibm-1164",

  "ibm-858", /* was "cp850" changed 2000nov28 */
  /* duplicate "cp850" removed 2000nov28 */
  /*"ibm-9049",*/ /* was "cp857" changed 2002nov25 */
  "ibm-12712", /* was "cp424" changed 2000nov28 */
  "ibm-4899", /* was "cp803" changed 2000nov28 */
  "ibm-867", /* was "cp862" changed 2002nov25 */
  "cp1258",
  "windows-950",
  "cp1253",
  /*  "cp819",
      "cp13488",*/
  "ibm-4971",
  /*"ibm-9061",*/ /* was "cp869" changed 2002nov25 */
  /* "cp813",*/
  /*"ibm-9044",*/ /* was "cp852" changed 2002nov25 */
  /*"ibm-872",*/ /* was "cp855" changed 2002nov25 */
  /*"ibm-808",*/ /* was "cp866" changed 2002nov25 */
/*  "cp1131",
  "cp1125",*/
  "ibm-902", /* was "cp922" changed 2003jan08 */
  "ibm-901", /* was "cp921" changed 2003jan09 */
  /*"ibm-17248",*/ /* was "cp864" changed 2002nov25 */
  /*"cp1008",
  "cp1046",*/
  /*  "cp9066",
      "cp1129",*/
  "ibm-5123", /* was "cp1027" changed 2003jan08 */
  /* "cp300",*/
  /* "cp4930",*/
  "ibm-1364",
  /* "cp1362" removed 2000nov28 */
  "cp1363",
  /* "cp1114", removed 2002jul3 
     "cp947", removed 2002jul3 */
  "gb18030",
  ""};

UBool isEuroAware(UConverter*);

void TestEuroRegression()
{
    int32_t i=0;

    do 
    {
        UErrorCode err = U_ZERO_ERROR;
        UConverter* myConv =  ucnv_open(convertersToCheck[i], &err);
        if (U_FAILURE(err)&&convertersToCheck[i][0])
            log_data_err("%s  \tMISSING [%s]\n", convertersToCheck[i], u_errorName(err));
        else 
        {
            if (isEuroAware(myConv))
                log_verbose("%s  \tsupports euro\n", convertersToCheck[i]);
            else
                log_err("%s  \tDOES NOT support euro\n", convertersToCheck[i]);
            ucnv_close(myConv);
        }
    } while (convertersToCheck[++i][0]);
}

UBool isEuroAware(UConverter* myConv)
{
    static const UChar euroString[2] = { 0x20AC, 0x0000 };
    char target[20];
    UChar euroBack[2];
    int32_t targetSize, euroBackSize;
    UErrorCode err = U_ZERO_ERROR;
    /*const char* myName =   ucnv_getName(myConv, &err);*/

    targetSize = ucnv_fromUChars(myConv,
            target,
            sizeof(target),
            euroString,
            -1,
            &err);
    if (U_FAILURE(err))
    {
      log_err("Failure Occured in ucnv_fromUChars euro roundtrip test\n");
      return FALSE;
    }
    euroBackSize = ucnv_toUChars(myConv,
            euroBack,
            2,
            target,
            targetSize,
            &err);
    if (U_FAILURE(err))
    {
        log_err("Failure Occured in ucnv_toUChars euro roundtrip test\n");
        return FALSE;
    }
    if (u_strcmp(euroString, euroBack)) 
    {
        /*      log_err("%s FAILED Euro rountrip\n", myName);*/
        return FALSE;
    }
    else 
    {
        /*      log_verbose("%s PASSED Euro rountrip\n", myName);*/
        return TRUE;
    }

}
#else
void addTestEuroRegression(TestNode** root)
{
    /* test nothing... */
}
#endif
