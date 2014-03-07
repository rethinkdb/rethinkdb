/********************************************************************
 * COPYRIGHT: 
 * Copyright (c) 1997-2009, International Business Machines Corporation and
 * others. All Rights Reserved.
 ********************************************************************/

#include "unicode/utypes.h"

#include "cstring.h"
#include "unicode/unistr.h"
#include "unicode/uniset.h"
#include "unicode/resbund.h"
#include "restest.h"

#include <stdlib.h>
#include <time.h>
#include <string.h>
#include <limits.h>

//***************************************************************************************

static const UChar kErrorUChars[] = { 0x45, 0x52, 0x52, 0x4f, 0x52, 0 };
static const int32_t kErrorLength = 5;
static const int32_t kERROR_COUNT = -1234567;

//***************************************************************************************

enum E_Where
{
    e_Root,
    e_te,
    e_te_IN,
    e_Where_count
};

//***************************************************************************************

#define CONFIRM_EQ(actual, expected, myAction) if ((expected)==(actual)) { record_pass(myAction); } else { record_fail(myAction + (UnicodeString)" returned " + (actual) + (UnicodeString)" instead of " + (expected) + "\n");}
#define CONFIRM_GE(actual, expected, myAction) if ((actual)>=(expected)) { record_pass(myAction); } else { record_fail(myAction + (UnicodeString)" returned " + (actual) + (UnicodeString)" instead of x >= " + (expected) + "\n");}
#define CONFIRM_NE(actual, expected, myAction) if ((expected)!=(actual)) { record_pass(myAction); } else { record_fail(myAction + (UnicodeString)" returned " + (actual) + (UnicodeString)" instead of x != " + (expected) + "\n");}

#define CONFIRM_UErrorCode(actual, expected, myAction) if ((expected)==(actual)) { record_pass(myAction); } else { record_fail(myAction + (UnicodeString)" returned " + u_errorName(actual) + " instead of " + u_errorName(expected) + "\n"); }

//***************************************************************************************

/**
 * Convert an integer, positive or negative, to a character string radix 10.
 */
char*
itoa(int32_t i, char* buf)
{
    char* result = buf;

    // Handle negative
    if (i < 0)
    {
        *buf++ = '-';
        i = -i;
    }

    // Output digits in reverse order
    char* p = buf;
    do
    {
        *p++ = (char)('0' + (i % 10));
        i /= 10;
    }
    while (i);
    *p-- = 0;

    // Reverse the string
    while (buf < p)
    {
        char c = *buf;
        *buf++ = *p;
        *p-- = c;
    }

    return result;
}



//***************************************************************************************

// Array of our test objects

static struct
{
    const char* name;
    Locale *locale;
    UErrorCode expected_constructor_status;
    E_Where where;
    UBool like[e_Where_count];
    UBool inherits[e_Where_count];
}
param[] =
{
    // "te" means test
    // "IN" means inherits
    // "NE" or "ne" means "does not exist"

    { "root",       NULL,   U_ZERO_ERROR,             e_Root,      { TRUE, FALSE, FALSE }, { TRUE, FALSE, FALSE } },
    { "te",         NULL,   U_ZERO_ERROR,             e_te,        { FALSE, TRUE, FALSE }, { TRUE, TRUE, FALSE } },
    { "te_IN",      NULL,   U_ZERO_ERROR,             e_te_IN,     { FALSE, FALSE, TRUE }, { TRUE, TRUE, TRUE } },
    { "te_NE",      NULL,   U_USING_FALLBACK_WARNING, e_te,        { FALSE, TRUE, FALSE }, { TRUE, TRUE, FALSE } },
    { "te_IN_NE",   NULL,   U_USING_FALLBACK_WARNING, e_te_IN,     { FALSE, FALSE, TRUE }, { TRUE, TRUE, TRUE } },
    { "ne",         NULL,   U_USING_DEFAULT_WARNING,  e_Root,      { TRUE, FALSE, FALSE }, { TRUE, FALSE, FALSE } }
};

static const int32_t bundles_count = sizeof(param) / sizeof(param[0]);

//***************************************************************************************

/**
 * Return a random unsigned long l where 0N <= l <= ULONG_MAX.
 */

uint32_t
randul()
{
    static UBool initialized = FALSE;
    if (!initialized)
    {
        srand((unsigned)time(NULL));
        initialized = TRUE;
    }
    // Assume rand has at least 12 bits of precision
    uint32_t l = 0;
    for (uint32_t i=0; i<sizeof(l); ++i)
        ((char*)&l)[i] = (char)((rand() & 0x0FF0) >> 4);
    return l;
}

/**
 * Return a random double x where 0.0 <= x < 1.0.
 */
double
randd()
{
    return (double)(randul() / ULONG_MAX);
}

/**
 * Return a random integer i where 0 <= i < n.
 */
int32_t randi(int32_t n)
{
    return (int32_t)(randd() * n);
}

//***************************************************************************************

/*
 Don't use more than one of these at a time because of the Locale names
*/
ResourceBundleTest::ResourceBundleTest()
: pass(0),
  fail(0)
{
    if (param[5].locale == NULL) {
        param[0].locale = new Locale("root");
        param[1].locale = new Locale("te");
        param[2].locale = new Locale("te", "IN");
        param[3].locale = new Locale("te", "NE");
        param[4].locale = new Locale("te", "IN", "NE");
        param[5].locale = new Locale("ne");
    }
}

ResourceBundleTest::~ResourceBundleTest()
{
    if (param[5].locale) {
        int idx;
        for (idx = 0; idx < (int)(sizeof(param)/sizeof(param[0])); idx++) {
            delete param[idx].locale;
            param[idx].locale = NULL;
        }
    }
}

void ResourceBundleTest::runIndexedTest( int32_t index, UBool exec, const char* &name, char* /*par*/ )
{
    if (exec) logln("TestSuite ResourceBundleTest: ");
    switch (index) {
#if !UCONFIG_NO_FILE_IO && !UCONFIG_NO_LEGACY_CONVERSION
    case 0: name = "TestResourceBundles"; if (exec) TestResourceBundles(); break;
    case 1: name = "TestConstruction"; if (exec) TestConstruction(); break;
    case 2: name = "TestGetSize"; if (exec) TestGetSize(); break;
    case 3: name = "TestGetLocaleByType"; if (exec) TestGetLocaleByType(); break;
#else
    case 0: case 1: case 2: case 3: name = "skip"; break;
#endif

    case 4: name = "TestExemplar"; if (exec) TestExemplar(); break;
        default: name = ""; break; //needed to end loop
    }
}

//***************************************************************************************

void
ResourceBundleTest::TestResourceBundles()
{
    UErrorCode status = U_ZERO_ERROR;

    loadTestData(status);
    if(U_FAILURE(status))
    {
        dataerrln("Could not load testdata.dat %s " + UnicodeString(u_errorName(status)));
        return;
    }

    /* Make sure that users using te_IN for the default locale don't get test failures. */
    Locale originalDefault;
    if (Locale::getDefault() == Locale("te_IN")) {
        Locale::setDefault(Locale("en_US"), status);
    }

    testTag("only_in_Root", TRUE, FALSE, FALSE);
    testTag("only_in_te", FALSE, TRUE, FALSE);
    testTag("only_in_te_IN", FALSE, FALSE, TRUE);
    testTag("in_Root_te", TRUE, TRUE, FALSE);
    testTag("in_Root_te_te_IN", TRUE, TRUE, TRUE);
    testTag("in_Root_te_IN", TRUE, FALSE, TRUE);
    testTag("in_te_te_IN", FALSE, TRUE, TRUE);
    testTag("nonexistent", FALSE, FALSE, FALSE);
    logln("Passed: %d\nFailed: %d", pass, fail);

    /* Restore the default locale for the other tests. */
    Locale::setDefault(originalDefault, status);
}

void
ResourceBundleTest::TestConstruction()
{
    UErrorCode   err = U_ZERO_ERROR;
    Locale       locale("te", "IN");

    const char* testdatapath=loadTestData(err);
    if(U_FAILURE(err))
    {
        dataerrln("Could not load testdata.dat " + UnicodeString(testdatapath) +  ", " + UnicodeString(u_errorName(err)));
        return;
    }

    /* Make sure that users using te_IN for the default locale don't get test failures. */
    Locale originalDefault;
    if (Locale::getDefault() == Locale("te_IN")) {
        Locale::setDefault(Locale("en_US"), err);
    }

    ResourceBundle  test1((UnicodeString)testdatapath, err);
    ResourceBundle  test2(testdatapath, locale, err);
    //ResourceBundle  test1("c:\\icu\\icu\\source\\test\\testdata\\testdata", err);
    //ResourceBundle  test2("c:\\icu\\icu\\source\\test\\testdata\\testdata", locale, err);

    UnicodeString   result1(test1.getStringEx("string_in_Root_te_te_IN", err));
    UnicodeString   result2(test2.getStringEx("string_in_Root_te_te_IN", err));

    if (U_FAILURE(err)) {
        errln("Something threw an error in TestConstruction()");
        return;
    }

    logln("for string_in_Root_te_te_IN, default.txt had " + result1);
    logln("for string_in_Root_te_te_IN, te_IN.txt had " + result2);

    if (result1 != "ROOT" || result2 != "TE_IN")
        errln("Construction test failed; run verbose for more information");

    const char* version1;
    const char* version2;

    version1 = test1.getVersionNumber();
    version2 = test2.getVersionNumber();

    char *versionID1 = new char[1+strlen(version1)]; // + 1 for zero byte
    char *versionID2 = new char[1+ strlen(version2)]; // + 1 for zero byte

    strcpy(versionID1, "44.0");  // hardcoded, please change if the default.txt file or ResourceBundle::kVersionSeparater is changed.

    strcpy(versionID2, "55.0");  // hardcoded, please change if the te_IN.txt file or ResourceBundle::kVersionSeparater is changed.

    logln(UnicodeString("getVersionNumber on default.txt returned ") + version1);
    logln(UnicodeString("getVersionNumber on te_IN.txt returned ") + version2);

    if (strcmp(version1, versionID1) != 0 || strcmp(version2, versionID2) != 0)
        errln("getVersionNumber() failed");

    delete[] versionID1;
    delete[] versionID2;

    /* Restore the default locale for the other tests. */
    Locale::setDefault(originalDefault, err);
}

//***************************************************************************************

UBool
ResourceBundleTest::testTag(const char* frag,
                            UBool in_Root,
                            UBool in_te,
                            UBool in_te_IN)
{
    int32_t failOrig = fail;

    // Make array from input params

    UBool is_in[] = { in_Root, in_te, in_te_IN };

    const char* NAME[] = { "ROOT", "TE", "TE_IN" };

    // Now try to load the desired items

    char tag[100];
    UnicodeString action;

    int32_t i,j,actual_bundle;
//    int32_t row,col;
    int32_t index;
    UErrorCode status = U_ZERO_ERROR;
    const char* testdatapath;
    testdatapath=loadTestData(status);
    if(U_FAILURE(status))
    {
        dataerrln("Could not load testdata.dat %s " + UnicodeString(u_errorName(status)));
        return FALSE;
    }

    for (i=0; i<bundles_count; ++i)
    {
        action = "Constructor for ";
        action += param[i].name;

        status = U_ZERO_ERROR;
        ResourceBundle theBundle( testdatapath, *param[i].locale, status);
        //ResourceBundle theBundle( "c:\\icu\\icu\\source\\test\\testdata\\testdata", *param[i].locale, status);
        CONFIRM_UErrorCode(status, param[i].expected_constructor_status, action);

        if(i == 5)
          actual_bundle = 0; /* ne -> default */
        else if(i == 3)
          actual_bundle = 1; /* te_NE -> te */
        else if(i == 4)
          actual_bundle = 2; /* te_IN_NE -> te_IN */
        else
          actual_bundle = i;


        UErrorCode expected_resource_status = U_MISSING_RESOURCE_ERROR;
        for (j=e_te_IN; j>=e_Root; --j)
        {
            if (is_in[j] && param[i].inherits[j])
            {
                if(j == actual_bundle) /* it's in the same bundle OR it's a nonexistent=default bundle (5) */
                  expected_resource_status = U_ZERO_ERROR;
                else if(j == 0)
                  expected_resource_status = U_USING_DEFAULT_WARNING;
                else
                  expected_resource_status = U_USING_FALLBACK_WARNING;
                
                break;
            }
        }

        UErrorCode expected_status;

        UnicodeString base;
        for (j=param[i].where; j>=0; --j)
        {
            if (is_in[j])
            {
                base = NAME[j];
                break;
            }
        }

        //--------------------------------------------------------------------------
        // string

        uprv_strcpy(tag, "string_");
        uprv_strcat(tag, frag);

        action = param[i].name;
        action += ".getString(";
        action += tag;
        action += ")";


        status = U_ZERO_ERROR;

        UnicodeString string(theBundle.getStringEx(tag, status));

        if(U_FAILURE(status)) {
            string.setTo(TRUE, kErrorUChars, kErrorLength);
        }

        CONFIRM_UErrorCode(status, expected_resource_status, action);

        UnicodeString expected_string(kErrorUChars);
        if (U_SUCCESS(status)) {
            expected_string = base;
        }

        CONFIRM_EQ(string, expected_string, action);

        //--------------------------------------------------------------------------
        // array

        uprv_strcpy(tag, "array_");
        uprv_strcat(tag, frag);

        action = param[i].name;
        action += ".get(";
        action += tag;
        action += ")";

        status = U_ZERO_ERROR;
        ResourceBundle arrayBundle(theBundle.get(tag, status));
        CONFIRM_UErrorCode(status, expected_resource_status, action);
        int32_t count = arrayBundle.getSize();

        if (U_SUCCESS(status))
        {
            CONFIRM_GE(count, 1, action);

            for (j=0; j < count; ++j)
            {
                char buf[32];
                UnicodeString value(arrayBundle.getStringEx(j, status));
                expected_string = base;
                expected_string += itoa(j,buf);
                CONFIRM_EQ(value, expected_string, action);
            }

            action = param[i].name;
            action += ".getStringEx(";
            action += tag;
            action += ")";

            for (j=0; j<100; ++j)
            {
                index = count ? (randi(count * 3) - count) : (randi(200) - 100);
                status = U_ZERO_ERROR;
                string = kErrorUChars;
                UnicodeString t(arrayBundle.getStringEx(index, status));
                expected_status = (index >= 0 && index < count) ? expected_resource_status : U_MISSING_RESOURCE_ERROR;
                CONFIRM_UErrorCode(status, expected_status, action);

                if (U_SUCCESS(status))
                {
                    char buf[32];
                    expected_string = base;
                    expected_string += itoa(index,buf);
                }
                else
                {
                    expected_string = kErrorUChars;
                }
                CONFIRM_EQ(string, expected_string, action);
            }
        }
        else if (status != expected_resource_status)
        {
            record_fail("Error getting " + (UnicodeString)tag);
            return (UBool)(failOrig != fail);
        }

    }

    return (UBool)(failOrig != fail);
}

void
ResourceBundleTest::record_pass(UnicodeString passMessage)
{
    logln(passMessage);
    ++pass;
}
void
ResourceBundleTest::record_fail(UnicodeString errMessage)
{
    err(errMessage);
    ++fail;
}

void
ResourceBundleTest::TestExemplar(){

    int32_t locCount = uloc_countAvailable();
    int32_t locIndex=0;
    int num=0;
    UErrorCode status = U_ZERO_ERROR;
    for(;locIndex<locCount;locIndex++){
        const char* locale = uloc_getAvailable(locIndex);
        UResourceBundle *resb =ures_open(NULL,locale,&status);
        if(U_SUCCESS(status) && status!=U_USING_FALLBACK_WARNING && status!=U_USING_DEFAULT_WARNING){
            int32_t len=0;
            const UChar* strSet = ures_getStringByKey(resb,"ExemplarCharacters",&len,&status);
            UnicodeSet set(strSet,status);
            if(U_FAILURE(status)){
                errln("Could not construct UnicodeSet from pattern for ExemplarCharacters in locale : %s. Error: %s",locale,u_errorName(status));
                status=U_ZERO_ERROR;
            }
            num++;
        }
        ures_close(resb);
    }
    logln("Number of installed locales with exemplar characters that could be tested: %d",num);

}

void 
ResourceBundleTest::TestGetSize(void) 
{
    const struct {
        const char* key;
        int32_t size;
    } test[] = {
        { "zerotest", 1},
        { "one", 1},
        { "importtest", 1},
        { "integerarray", 1},
        { "emptyarray", 0},
        { "emptytable", 0},
        { "emptystring", 1}, /* empty string is still a string */
        { "emptyint", 1}, 
        { "emptybin", 1},
        { "testinclude", 1},
        { "collations", 1}, /* not 2 - there is hidden %%CollationBin */
    };
    
    UErrorCode status = U_ZERO_ERROR;
    
    const char* testdatapath = loadTestData(status);
    int32_t i = 0, j = 0;
    int32_t size = 0;
    
    if(U_FAILURE(status))
    {
        dataerrln("Could not load testdata.dat %s\n", u_errorName(status));
        return;
    }
    
    ResourceBundle rb(testdatapath, "testtypes", status);
    if(U_FAILURE(status))
    {
        err("Could not testtypes resource bundle %s\n", u_errorName(status));
        return;
    }
    
    for(i = 0; i < (int32_t)(sizeof(test)/sizeof(test[0])); i++) {
        ResourceBundle res = rb.get(test[i].key, status);
        if(U_FAILURE(status))
        {
            err("Couldn't find the key %s. Error: %s\n", u_errorName(status));
            return;
        }
        size = res.getSize();
        if(size != test[i].size) {
            err("Expected size %i, got size %i for key %s\n", test[i].size, size, test[i].key);
            for(j = 0; j < size; j++) {
                ResourceBundle helper = res.get(j, status);
                err("%s\n", helper.getKey());
            }
        }
    }
}

void 
ResourceBundleTest::TestGetLocaleByType(void) 
{
    const struct {
        const char *requestedLocale;
        const char *resourceKey;
        const char *validLocale;
        const char *actualLocale;
    } test[] = {
        { "te_IN_BLAH", "string_only_in_te_IN", "te_IN", "te_IN" },
        { "te_IN_BLAH", "string_only_in_te", "te_IN", "te" },
        { "te_IN_BLAH", "string_only_in_Root", "te_IN", "root" },
        { "te_IN_BLAH_01234567890_01234567890_01234567890_01234567890_01234567890_01234567890", "array_2d_only_in_Root", "te_IN", "root" },
        { "te_IN_BLAH@currency=euro", "array_2d_only_in_te_IN", "te_IN", "te_IN" },
        { "te_IN_BLAH@calendar=thai;collation=phonebook", "array_2d_only_in_te", "te_IN", "te" }
    };
    
    UErrorCode status = U_ZERO_ERROR;
    
    const char* testdatapath = loadTestData(status);
    int32_t i = 0;
    Locale locale;
    
    if(U_FAILURE(status))
    {
        dataerrln("Could not load testdata.dat %s\n", u_errorName(status));
        return;
    }
    
    for(i = 0; i < (int32_t)(sizeof(test)/sizeof(test[0])); i++) {
        ResourceBundle rb(testdatapath, test[i].requestedLocale, status);
        if(U_FAILURE(status))
        {
            err("Could not open resource bundle %s (error %s)\n", test[i].requestedLocale, u_errorName(status));
            status = U_ZERO_ERROR;
            continue;
        }
        
        ResourceBundle res = rb.get(test[i].resourceKey, status);
        if(U_FAILURE(status))
        {
            err("Couldn't find the key %s. Error: %s\n", test[i].resourceKey, u_errorName(status));
            status = U_ZERO_ERROR;
            continue;
        }
        
        locale = res.getLocale(ULOC_REQUESTED_LOCALE, status);
        if(locale != Locale::getDefault()) {
            err("Expected requested locale to be %s. Got %s\n", test[i].requestedLocale, locale.getName());
        }
        locale = res.getLocale(ULOC_VALID_LOCALE, status);
        if(strcmp(locale.getName(), test[i].validLocale) != 0) {
            err("Expected valid locale to be %s. Got %s\n", test[i].requestedLocale, locale.getName());
        }
        locale = res.getLocale(ULOC_ACTUAL_LOCALE, status);
        if(strcmp(locale.getName(), test[i].actualLocale) != 0) {
            err("Expected actual locale to be %s. Got %s\n", test[i].requestedLocale, locale.getName());
        }
    }
}

//eof

