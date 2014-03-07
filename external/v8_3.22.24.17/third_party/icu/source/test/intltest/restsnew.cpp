/********************************************************************
 * Copyright (c) 1997-2009, International Business Machines
 * Corporation and others. All Rights Reserved.
 ********************************************************************/

#include "unicode/utypes.h"

#include "cstring.h"
#include "unicode/unistr.h"
#include "unicode/resbund.h"
#include "restsnew.h"

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

#define CONFIRM_EQ(actual,expected) if ((expected)==(actual)) { record_pass(); } else { record_fail(); errln(action + (UnicodeString)" returned " + (actual) + (UnicodeString)" instead of " + (expected)); }
#define CONFIRM_GE(actual,expected) if ((actual)>=(expected)) { record_pass(); } else { record_fail(); errln(action + (UnicodeString)" returned " + (actual) + (UnicodeString)" instead of x >= " + (expected)); }
#define CONFIRM_NE(actual,expected) if ((expected)!=(actual)) { record_pass(); } else { record_fail(); errln(action + (UnicodeString)" returned " + (actual) + (UnicodeString)" instead of x != " + (expected)); }

#define CONFIRM_UErrorCode(actual,expected) if ((expected)==(actual)) { record_pass(); } else { record_fail(); errln(action + (UnicodeString)" returned " + (UnicodeString)u_errorName(actual) + (UnicodeString)" instead of " + (UnicodeString)u_errorName(expected)); }

//***************************************************************************************

/**
 * Convert an integer, positive or negative, to a character string radix 10.
 */
static char*
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

    { "root",       0,   U_ZERO_ERROR,             e_Root,      { TRUE, FALSE, FALSE }, { TRUE, FALSE, FALSE } },
    { "te",         0,   U_ZERO_ERROR,             e_te,        { FALSE, TRUE, FALSE }, { TRUE, TRUE, FALSE  } },
    { "te_IN",      0,   U_ZERO_ERROR,             e_te_IN,     { FALSE, FALSE, TRUE }, { TRUE, TRUE, TRUE   } },
    { "te_NE",      0,   U_USING_FALLBACK_WARNING, e_te,        { FALSE, TRUE, FALSE }, { TRUE, TRUE, FALSE  } },
    { "te_IN_NE",   0,   U_USING_FALLBACK_WARNING, e_te_IN,     { FALSE, FALSE, TRUE }, { TRUE, TRUE, TRUE   } },
    { "ne",         0,   U_USING_DEFAULT_WARNING,  e_Root,      { TRUE, FALSE, FALSE }, { TRUE, FALSE, FALSE } }
};

static int32_t bundles_count = sizeof(param) / sizeof(param[0]);

//***************************************************************************************

/**
 * Return a random unsigned long l where 0N <= l <= ULONG_MAX.
 */

static uint32_t
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
static double
randd()
{
    return (double)(randul() / ULONG_MAX);
}

/**
 * Return a random integer i where 0 <= i < n.
 */
static int32_t randi(int32_t n)
{
    return (int32_t)(randd() * n);
}

//***************************************************************************************

/*
 Don't use more than one of these at a time because of the Locale names
*/
NewResourceBundleTest::NewResourceBundleTest()
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

NewResourceBundleTest::~NewResourceBundleTest()
{
    if (param[5].locale) {
        int idx;
        for (idx = 0; idx < (int)(sizeof(param)/sizeof(param[0])); idx++) {
            delete param[idx].locale;
            param[idx].locale = NULL;
        }
    }
}

void NewResourceBundleTest::runIndexedTest( int32_t index, UBool exec, const char* &name, char* /*par*/ )
{
    if (exec) logln("TestSuite ResourceBundleTest: ");
    switch (index) {
#if !UCONFIG_NO_FILE_IO && !UCONFIG_NO_LEGACY_CONVERSION
    case 0: name = "TestResourceBundles"; if (exec) TestResourceBundles(); break;
    case 1: name = "TestConstruction"; if (exec) TestConstruction(); break;
    case 2: name = "TestIteration"; if (exec) TestIteration(); break;
    case 3: name = "TestOtherAPI";  if(exec) TestOtherAPI(); break;
    case 4: name = "TestNewTypes";  if(exec) TestNewTypes(); break;
#else
    case 0: case 1: case 2: case 3: case 4: name = "skip"; break;
#endif

    case 5: name = "TestGetByFallback";  if(exec) TestGetByFallback(); break;
        default: name = ""; break; //needed to end loop
    }
}

//***************************************************************************************

void
NewResourceBundleTest::TestResourceBundles()
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
NewResourceBundleTest::TestConstruction()
{
    UErrorCode   err = U_ZERO_ERROR;
    Locale       locale("te", "IN");

    const char* testdatapath;
    testdatapath=loadTestData(err);
    if(U_FAILURE(err))
    {
        dataerrln("Could not load testdata.dat %s " + UnicodeString(u_errorName(err)));
        return;
    }

    /* Make sure that users using te_IN for the default locale don't get test failures. */
    Locale originalDefault;
    if (Locale::getDefault() == Locale("te_IN")) {
        Locale::setDefault(Locale("en_US"), err);
    }

    ResourceBundle  test1((UnicodeString)testdatapath, err);
    ResourceBundle  test2(testdatapath, locale, err);
    
    UnicodeString   result1;
    UnicodeString   result2;

    result1 = test1.getStringEx("string_in_Root_te_te_IN", err);
    result2 = test2.getStringEx("string_in_Root_te_te_IN", err);
    if (U_FAILURE(err)) {
        errln("Something threw an error in TestConstruction()");
        return;
    }

    logln("for string_in_Root_te_te_IN, root.txt had " + result1);
    logln("for string_in_Root_te_te_IN, te_IN.txt had " + result2);

    if (result1 != "ROOT" || result2 != "TE_IN") {
        errln("Construction test failed; run verbose for more information");
    }

    const char* version1;
    const char* version2;

    version1 = test1.getVersionNumber();
    version2 = test2.getVersionNumber();

    char *versionID1 = new char[1 + strlen(U_ICU_VERSION) + strlen(version1)]; // + 1 for zero byte
    char *versionID2 = new char[1 + strlen(U_ICU_VERSION) + strlen(version2)]; // + 1 for zero byte

    strcpy(versionID1, "44.0");  // hardcoded, please change if the default.txt file or ResourceBundle::kVersionSeparater is changed.

    strcpy(versionID2, "55.0");  // hardcoded, please change if the te_IN.txt file or ResourceBundle::kVersionSeparater is changed.

    logln(UnicodeString("getVersionNumber on default.txt returned ") + version1 + UnicodeString(" Expect: " ) + versionID1);
    logln(UnicodeString("getVersionNumber on te_IN.txt returned ") + version2 + UnicodeString(" Expect: " ) + versionID2);

    if (strcmp(version1, versionID1) != 0) {
        errln("getVersionNumber(version1) failed. %s != %s", version1, versionID1);
    }
    if (strcmp(version2, versionID2) != 0) {
        errln("getVersionNumber(version2) failed. %s != %s", version2, versionID2);
    }
    delete[] versionID1;
    delete[] versionID2;

    /* Restore the default locale for the other tests. */
    Locale::setDefault(originalDefault, err);
}

void
NewResourceBundleTest::TestIteration()
{
    UErrorCode   err = U_ZERO_ERROR;
    const char* testdatapath;
    const char* data[]={
        "string_in_Root_te_te_IN",   "1",
        "array_in_Root_te_te_IN",    "5",
        "array_2d_in_Root_te_te_IN", "4",
    };

    Locale       *locale=new Locale("te_IN");

    testdatapath=loadTestData(err);
    if(U_FAILURE(err))
    {
        dataerrln("Could not load testdata.dat %s " + UnicodeString(u_errorName(err)));
        return;
    }

    ResourceBundle  test1(testdatapath, *locale, err);
    if(U_FAILURE(err)){
        errln("Construction failed");
    }
    uint32_t i;
    int32_t count, row=0, col=0;
    char buf[5];
    UnicodeString expected;
    UnicodeString element("TE_IN");
    UnicodeString action;


    for(i=0; i<sizeof(data)/sizeof(data[0]); i=i+2){
        action = "te_IN";
        action +=".get(";
        action += data[i];
        action +=", err)";
        err=U_ZERO_ERROR;
        ResourceBundle bundle = test1.get(data[i], err); 
        if(!U_FAILURE(err)){
            action = "te_IN";
            action +=".getKey()";

            CONFIRM_EQ((UnicodeString)bundle.getKey(), (UnicodeString)data[i]);

            count=0;
            row=0;
            while(bundle.hasNext()){
                action = data[i];
                action +=".getNextString(err)";
                row=count;   
                UnicodeString got=bundle.getNextString(err);
                if(U_SUCCESS(err)){
                    expected=element;
                    if(bundle.getSize() > 1){
                        CONFIRM_EQ(bundle.getType(), URES_ARRAY);
                        expected+=itoa(row, buf);
                        ResourceBundle rowbundle=bundle.get(row, err);
                        if(!U_FAILURE(err) && rowbundle.getSize()>1){
                            col=0;
                            while(rowbundle.hasNext()){
                                expected=element;
                                got=rowbundle.getNextString(err);
                                if(!U_FAILURE(err)){
                                    expected+=itoa(row, buf);
                                    expected+=itoa(col, buf);
                                    col++;
                                    CONFIRM_EQ(got, expected);
                                }
                            }
                            CONFIRM_EQ(col, rowbundle.getSize());
                        }
                    }
                    else{
                        CONFIRM_EQ(bundle.getType(), (int32_t)URES_STRING);
                    }
                }
                CONFIRM_EQ(got, expected);
                count++;
            }
            action = data[i];
            action +=".getSize()";
            CONFIRM_EQ(bundle.getSize(), count);
            CONFIRM_EQ(count, atoi(data[i+1]));
            //after reaching the end
            err=U_ZERO_ERROR;
            ResourceBundle errbundle=bundle.getNext(err);
            action = "After reaching the end of the Iterator:-  ";
            action +=data[i];
            action +=".getNext()";
            CONFIRM_NE(err, (int32_t)U_ZERO_ERROR);
            CONFIRM_EQ(u_errorName(err), u_errorName(U_INDEX_OUTOFBOUNDS_ERROR));
            //reset the iterator
            err = U_ZERO_ERROR;
            bundle.resetIterator();
         /*  The following code is causing a crash
         ****CRASH******
         */

            bundle.getNext(err);
            if(U_FAILURE(err)){
                errln("ERROR: getNext()  throw an error");
            }
        }
    }
    delete locale;
}

// TODO: add operator== and != to ResourceBundle
static UBool
equalRB(ResourceBundle &a, ResourceBundle &b) {
    UResType type;
    UErrorCode status;

    type=a.getType();
    status=U_ZERO_ERROR;
    return
        type==b.getType() &&
        a.getLocale()==b.getLocale() &&
        0==strcmp(a.getName(), b.getName()) &&
        type==URES_STRING ?
            a.getString(status)==b.getString(status) :
            type==URES_INT ?
                a.getInt(status)==b.getInt(status) :
                TRUE;
}

void
NewResourceBundleTest::TestOtherAPI(){
    UErrorCode   err = U_ZERO_ERROR;
    const char* testdatapath=loadTestData(err);
    UnicodeString tDataPathUS = UnicodeString(testdatapath, "");

    if(U_FAILURE(err))
    {
        dataerrln("Could not load testdata.dat %s " + UnicodeString(u_errorName(err)));
        return;
    }

    /* Make sure that users using te_IN for the default locale don't get test failures. */
    Locale originalDefault;
    if (Locale::getDefault() == Locale("te_IN")) {
        Locale::setDefault(Locale("en_US"), err);
    }

    Locale       *locale=new Locale("te_IN");

    ResourceBundle test0(tDataPathUS, *locale, err);
    if(U_FAILURE(err)){
        errln("Construction failed");
        return;
    }

    ResourceBundle  test1(testdatapath, *locale, err);
    if(U_FAILURE(err)){
        errln("Construction failed");
        return;
    }

    logln("Testing getLocale()\n");
    if(strcmp(test1.getLocale().getName(), locale->getName()) !=0 ){
        errln("FAIL: ResourceBundle::getLocale() failed\n");
    }

    delete locale;

    logln("Testing ResourceBundle(UErrorCode)\n");
    ResourceBundle defaultresource(err);
    ResourceBundle explicitdefaultresource(NULL, Locale::getDefault(), err);
    if(U_FAILURE(err)){
        errcheckln(err, "Construction of default resourcebundle failed - %s", u_errorName(err));
        return;
    }
    // You can't compare the default locale to the resolved locale in the
    // resource bundle due to aliasing, keywords in the default locale
    // or the chance that the machine running these tests is using a locale
    // that isn't available in ICU.
    if(strcmp(defaultresource.getLocale().getName(), explicitdefaultresource.getLocale().getName()) != 0){
        errln("Construction of default resourcebundle didn't take the defaultlocale. Expected %s Got %s err=%s\n",
            explicitdefaultresource.getLocale().getName(), defaultresource.getLocale().getName(), u_errorName(err));
    }
    

    ResourceBundle copyRes(defaultresource);
    if(strcmp(copyRes.getName(), defaultresource.getName() ) !=0  ||
        strcmp(test1.getName(), defaultresource.getName() ) ==0 ||
        strcmp(copyRes.getLocale().getName(), defaultresource.getLocale().getName() ) !=0  ||
        strcmp(test1.getLocale().getName(), defaultresource.getLocale().getName() ) ==0 )
    {
        errln("copy construction failed\n");
    }

    ResourceBundle defaultSub = defaultresource.get((int32_t)0, err);
    ResourceBundle defSubCopy(defaultSub);
    if(strcmp(defSubCopy.getName(), defaultSub.getName() ) !=0  ||
        strcmp(defSubCopy.getLocale().getName(), defaultSub.getLocale().getName() ) !=0  ){
        errln("copy construction for subresource failed\n");
    }

    ResourceBundle *p;

    p = defaultresource.clone();
    if(p == &defaultresource || !equalRB(*p, defaultresource)) {
        errln("ResourceBundle.clone() failed");
    }
    delete p;

    p = defaultSub.clone();
    if(p == &defaultSub || !equalRB(*p, defaultSub)) {
        errln("2nd ResourceBundle.clone() failed");
    }
    delete p;

    UVersionInfo ver;
    copyRes.getVersion(ver);

    logln("Version returned: [%d.%d.%d.%d]\n", ver[0], ver[1], ver[2], ver[3]);

    logln("Testing C like UnicodeString APIs\n");

    UResourceBundle *testCAPI = NULL, *bundle = NULL, *rowbundle = NULL, *temp = NULL;
    err = U_ZERO_ERROR;
    const char* data[]={
        "string_in_Root_te_te_IN",   "1",
        "array_in_Root_te_te_IN",    "5",
        "array_2d_in_Root_te_te_IN", "4",
    };


    testCAPI = ures_open(testdatapath, "te_IN", &err);

    if(U_SUCCESS(err)) {
        // Do the testing
        // first iteration

        uint32_t i;
        int32_t count, row=0, col=0;
        char buf[5];
        UnicodeString expected;
        UnicodeString element("TE_IN");
        UnicodeString action;


        for(i=0; i<sizeof(data)/sizeof(data[0]); i=i+2){
            action = "te_IN";
            action +=".get(";
            action += data[i];
            action +=", err)";
            err=U_ZERO_ERROR;
            bundle = ures_getByKey(testCAPI, data[i], bundle, &err); 
            if(!U_FAILURE(err)){
                const char* key = NULL;
                action = "te_IN";
                action +=".getKey()";

                CONFIRM_EQ((UnicodeString)ures_getKey(bundle), (UnicodeString)data[i]);

                count=0;
                row=0;
                while(ures_hasNext(bundle)){
                    action = data[i];
                    action +=".getNextString(err)";
                    row=count;   
                    UnicodeString got=ures_getNextUnicodeString(bundle, &key, &err);
                    if(U_SUCCESS(err)){
                        expected=element;
                        if(ures_getSize(bundle) > 1){
                            CONFIRM_EQ(ures_getType(bundle), URES_ARRAY);
                            expected+=itoa(row, buf);
                            rowbundle=ures_getByIndex(bundle, row, rowbundle, &err);
                            if(!U_FAILURE(err) && ures_getSize(rowbundle)>1){
                                col=0;
                                while(ures_hasNext(rowbundle)){
                                    expected=element;
                                    got=ures_getNextUnicodeString(rowbundle, &key, &err);
                                    temp = ures_getByIndex(rowbundle, col, temp, &err);
                                    UnicodeString bla = ures_getUnicodeString(temp, &err);
                                    UnicodeString bla2 = ures_getUnicodeStringByIndex(rowbundle, col, &err);
                                    if(!U_FAILURE(err)){
                                        expected+=itoa(row, buf);
                                        expected+=itoa(col, buf);
                                        col++;
                                        CONFIRM_EQ(got, expected);
                                        CONFIRM_EQ(bla, expected);
                                        CONFIRM_EQ(bla2, expected);
                                    }
                                }
                                CONFIRM_EQ(col, ures_getSize(rowbundle));
                            }
                        }
                        else{
                            CONFIRM_EQ(ures_getType(bundle), (int32_t)URES_STRING);
                        }
                    }
                    CONFIRM_EQ(got, expected);
                    count++;
                }
            }
        }
        ures_close(temp);
        ures_close(rowbundle);
        ures_close(bundle);
        ures_close(testCAPI);
    } else {
        errln("failed to open a resource bundle\n");
    }

    /* Restore the default locale for the other tests. */
    Locale::setDefault(originalDefault, err);
}




//***************************************************************************************

UBool
NewResourceBundleTest::testTag(const char* frag,
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

    int32_t i,j,row,col, actual_bundle;
    int32_t index;
    const char* testdatapath;

    UErrorCode status = U_ZERO_ERROR;
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
        CONFIRM_UErrorCode(status,param[i].expected_constructor_status);

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
        action += ".getStringEx(";
        action += tag;
        action += ")";


        status = U_ZERO_ERROR;
        UnicodeString string = theBundle.getStringEx(tag, status);
        if(U_FAILURE(status)) {
            string.setTo(TRUE, kErrorUChars, kErrorLength);
        }

        CONFIRM_UErrorCode(status, expected_resource_status);

        UnicodeString expected_string(kErrorUChars);
        if (U_SUCCESS(status)) {
            expected_string = base;
        }

        CONFIRM_EQ(string, expected_string);

        //--------------------------------------------------------------------------
        // array   ResourceBundle using the key

        uprv_strcpy(tag, "array_");
        uprv_strcat(tag, frag);

        action = param[i].name;
        action += ".get(";
        action += tag;
        action += ")";

        int32_t count = kERROR_COUNT;
        status = U_ZERO_ERROR;
        ResourceBundle array = theBundle.get(tag, status);
        CONFIRM_UErrorCode(status,expected_resource_status);


        if (U_SUCCESS(status))
        {
            //confirm the resource type is an array
            UResType bundleType=array.getType();
            CONFIRM_EQ(bundleType, URES_ARRAY);

            count=array.getSize();
            CONFIRM_GE(count,1);
           
            for (j=0; j<count; ++j)
            {
                char buf[32];
                expected_string = base;
                expected_string += itoa(j,buf);
                CONFIRM_EQ(array.getNextString(status),expected_string);
            }

        }
        else
        {
            CONFIRM_EQ(count,kERROR_COUNT);
            //       CONFIRM_EQ((int32_t)(unsigned long)array,(int32_t)0);
            count = 0;
        }

        //--------------------------------------------------------------------------
        // arrayItem ResourceBundle using the index


        for (j=0; j<100; ++j)
        {
            index = count ? (randi(count * 3) - count) : (randi(200) - 100);
            status = U_ZERO_ERROR;
            string = kErrorUChars;
            ResourceBundle array = theBundle.get(tag, status);
            if(!U_FAILURE(status)){
                UnicodeString t = array.getStringEx(index, status);
                if(!U_FAILURE(status)) {
                   string=t;
                }
            }

            expected_status = (index >= 0 && index < count) ? expected_resource_status : U_MISSING_RESOURCE_ERROR;
            CONFIRM_UErrorCode(status,expected_status);

            if (U_SUCCESS(status)){
                char buf[32];
                expected_string = base;
                expected_string += itoa(index,buf);
            } else {
                expected_string = kErrorUChars;
            }
               CONFIRM_EQ(string,expected_string);

        }

        //--------------------------------------------------------------------------
        // 2dArray

        uprv_strcpy(tag, "array_2d_");
        uprv_strcat(tag, frag);

        action = param[i].name;
        action += ".get(";
        action += tag;
        action += ")";


        int32_t row_count = kERROR_COUNT, column_count = kERROR_COUNT;
        status = U_ZERO_ERROR;
        ResourceBundle array2d=theBundle.get(tag, status);

        //const UnicodeString** array2d = theBundle.get2dArray(tag, row_count, column_count, status);
        CONFIRM_UErrorCode(status,expected_resource_status);

        if (U_SUCCESS(status))
        {
            //confirm the resource type is an 2darray
            UResType bundleType=array2d.getType();
            CONFIRM_EQ(bundleType, URES_ARRAY);

            row_count=array2d.getSize();
            CONFIRM_GE(row_count,1);

            for(row=0; row<row_count; ++row){
                ResourceBundle tablerow=array2d.get(row, status);
                CONFIRM_UErrorCode(status, expected_resource_status);
                if(U_SUCCESS(status)){
                    //confirm the resourcetype of each table row is an array
                    UResType rowType=tablerow.getType();
                    CONFIRM_EQ(rowType, URES_ARRAY);

                    column_count=tablerow.getSize();
                    CONFIRM_GE(column_count,1);

                    for (col=0; j<column_count; ++j) {
                           char buf[32];
                           expected_string = base;
                           expected_string += itoa(row,buf);
                           expected_string += itoa(col,buf);
                           CONFIRM_EQ(tablerow.getNextString(status),expected_string);
                    }
                }
            }
        }else{
            CONFIRM_EQ(row_count,kERROR_COUNT);
            CONFIRM_EQ(column_count,kERROR_COUNT);
             row_count=column_count=0;
        }




       //--------------------------------------------------------------------------
       // 2dArrayItem
       for (j=0; j<200; ++j)
       {
           row = row_count ? (randi(row_count * 3) - row_count) : (randi(200) - 100);
           col = column_count ? (randi(column_count * 3) - column_count) : (randi(200) - 100);
           status = U_ZERO_ERROR;
           string = kErrorUChars;
           ResourceBundle array2d=theBundle.get(tag, status);
           if(U_SUCCESS(status)){
                ResourceBundle tablerow=array2d.get(row, status);
                if(U_SUCCESS(status)) {
                    UnicodeString t=tablerow.getStringEx(col, status);
                    if(U_SUCCESS(status)){
                       string=t;
                    }
                }
           }
           expected_status = (row >= 0 && row < row_count && col >= 0 && col < column_count) ?
                                  expected_resource_status: U_MISSING_RESOURCE_ERROR;
           CONFIRM_UErrorCode(status,expected_status);

           if (U_SUCCESS(status)){
               char buf[32];
               expected_string = base;
               expected_string += itoa(row,buf);
               expected_string += itoa(col,buf);
           } else {
               expected_string = kErrorUChars;
           }
               CONFIRM_EQ(string,expected_string);

        }

        //--------------------------------------------------------------------------
        // taggedArray

        uprv_strcpy(tag, "tagged_array_");
        uprv_strcat(tag, frag);

        action = param[i].name;
        action += ".get(";
        action += tag;
        action += ")";

        int32_t         tag_count;
        status = U_ZERO_ERROR;

        ResourceBundle tags=theBundle.get(tag, status);
        CONFIRM_UErrorCode(status, expected_resource_status);

        if (U_SUCCESS(status)) {
            UResType bundleType=tags.getType();
            CONFIRM_EQ(bundleType, URES_TABLE);

            tag_count=tags.getSize();
            CONFIRM_GE((int32_t)tag_count, (int32_t)0); 

            for(index=0; index <tag_count; index++){
                ResourceBundle tagelement=tags.get(index, status);
                UnicodeString key=tagelement.getKey();
                UnicodeString value=tagelement.getNextString(status);
                logln("tag = " + key + ", value = " + value );
                if(key.startsWith("tag") && value.startsWith(base)){
                    record_pass();
                }else{
                    record_fail();
                }

            }

            for(index=0; index <tag_count; index++){
                ResourceBundle tagelement=tags.get(index, status);
                const char *tkey=NULL;
                UnicodeString value=tagelement.getNextString(&tkey, status);
                UnicodeString key(tkey);
                logln("tag = " + key + ", value = " + value );
                if(value.startsWith(base)){
                    record_pass();
                }else{
                    record_fail();
                }
            }

        }else{
            tag_count=0;
        }




        //--------------------------------------------------------------------------
        // taggedArrayItem

        action = param[i].name;
        action += ".get(";
        action += tag;
        action += ")";

        count = 0;
        for (index=-20; index<20; ++index)
        {
            char buf[32];
            status = U_ZERO_ERROR;
            string = kErrorUChars;
            char item_tag[8];
            uprv_strcpy(item_tag, "tag");
            uprv_strcat(item_tag, itoa(index,buf));
            ResourceBundle tags=theBundle.get(tag, status);
            if(U_SUCCESS(status)){
                ResourceBundle tagelement=tags.get(item_tag, status);
                if(!U_FAILURE(status)){
                    UResType elementType=tagelement.getType();
                    CONFIRM_EQ(elementType, (int32_t)URES_STRING);
                    const char* key=tagelement.getKey();
                    CONFIRM_EQ((UnicodeString)key, (UnicodeString)item_tag);
                    UnicodeString t=tagelement.getString(status);
                    if(!U_FAILURE(status)){
                        string=t;
                    }
                }
                if (index < 0) {
                    CONFIRM_UErrorCode(status,U_MISSING_RESOURCE_ERROR);
                }
                else{
                   if (status != U_MISSING_RESOURCE_ERROR) {
                       count++;
                       expected_string = base;
                       expected_string += buf;
                       CONFIRM_EQ(string,expected_string);
                   }
                }
            }

        }
        CONFIRM_EQ(count, tag_count);

    }
    return (UBool)(failOrig == fail);
}

void
NewResourceBundleTest::record_pass()
{
  ++pass;
}
void
NewResourceBundleTest::record_fail()
{
  err();
  ++fail;
}


void
NewResourceBundleTest::TestNewTypes() {
    char action[256];
    const char* testdatapath;
    UErrorCode status = U_ZERO_ERROR;
    uint8_t *binResult = NULL;
    int32_t len = 0;
    int32_t i = 0;
    int32_t intResult = 0;
    uint32_t uintResult = 0;
    UChar expected[] = { 'a','b','c','\0','d','e','f' };
    const char* expect ="tab:\t cr:\r ff:\f newline:\n backslash:\\\\ quote=\\\' doubleQuote=\\\" singlequoutes=''";
    UChar uExpect[200];

    testdatapath=loadTestData(status);

    if(U_FAILURE(status))
    {
        dataerrln("Could not load testdata.dat %s \n",u_errorName(status));
        return;
    }

    ResourceBundle theBundle(testdatapath, "testtypes", status);
    ResourceBundle bundle(testdatapath, Locale("te_IN"),status);

    UnicodeString emptyStr = theBundle.getStringEx("emptystring", status);
    if(!emptyStr.length()==0) {
      logln("Empty string returned invalid value\n");
    }

    CONFIRM_UErrorCode(status, U_ZERO_ERROR);

    /* This test reads the string "abc\u0000def" from the bundle   */
    /* if everything is working correctly, the size of this string */
    /* should be 7. Everything else is a wrong answer, esp. 3 and 6*/

    strcpy(action, "getting and testing of string with embeded zero");
    ResourceBundle res = theBundle.get("zerotest", status);
    CONFIRM_UErrorCode(status, U_ZERO_ERROR);
    CONFIRM_EQ(res.getType(), URES_STRING);
    UnicodeString zeroString=res.getString(status);
    len = zeroString.length();
    if(U_SUCCESS(status)){
        CONFIRM_UErrorCode(status, U_ZERO_ERROR);
        CONFIRM_EQ(len, 7);
        CONFIRM_NE(len, 3);
    }
    for(i=0;i<len;i++){
        if(zeroString[i]!= expected[i]){
            logln("Output didnot match Expected: \\u%4X Got: \\u%4X", expected[i], zeroString[i]);
        }
    }

    strcpy(action, "getting and testing of binary type");
    res = theBundle.get("binarytest", status);
    CONFIRM_UErrorCode(status, U_ZERO_ERROR);
    CONFIRM_EQ(res.getType(), URES_BINARY);
    binResult=(uint8_t*)res.getBinary(len, status);
    if(U_SUCCESS(status)){
        CONFIRM_UErrorCode(status, U_ZERO_ERROR);
        CONFIRM_EQ(len, 15);
        for(i = 0; i<15; i++) {
            CONFIRM_EQ(binResult[i], i);
        }
    }

    strcpy(action, "getting and testing of imported binary type");
    res = theBundle.get("importtest",status);
    CONFIRM_UErrorCode(status, U_ZERO_ERROR);
    CONFIRM_EQ(res.getType(), URES_BINARY);
    binResult=(uint8_t*)res.getBinary(len, status);
    if(U_SUCCESS(status)){
        CONFIRM_UErrorCode(status, U_ZERO_ERROR);
        CONFIRM_EQ(len, 15);
        for(i = 0; i<15; i++) {
            CONFIRM_EQ(binResult[i], i);
        }
    }

    strcpy(action, "getting and testing of integer types");
    res = theBundle.get("one",  status);
    CONFIRM_UErrorCode(status, U_ZERO_ERROR);
    CONFIRM_EQ(res.getType(), URES_INT);
    intResult=res.getInt(status);
    uintResult = res.getUInt(status);
    if(U_SUCCESS(status)){
        CONFIRM_UErrorCode(status, U_ZERO_ERROR);
        CONFIRM_EQ(uintResult, (uint32_t)intResult);
        CONFIRM_EQ(intResult, 1);
    }

    strcpy(action, "getting minusone");
    res = theBundle.get((const char*)"minusone", status);
    CONFIRM_UErrorCode(status, U_ZERO_ERROR);
    CONFIRM_EQ(res.getType(), URES_INT);
    intResult=res.getInt(status);
    uintResult = res.getUInt(status);
    if(U_SUCCESS(status)){
        CONFIRM_UErrorCode(status, U_ZERO_ERROR);
        CONFIRM_EQ(uintResult, 0x0FFFFFFF); /* a 28 bit integer */
        CONFIRM_EQ(intResult, -1);
        CONFIRM_NE(uintResult, (uint32_t)intResult);
    }

    strcpy(action, "getting plusone");
    res = theBundle.get("plusone",status);
    CONFIRM_UErrorCode(status, U_ZERO_ERROR);
    CONFIRM_EQ(res.getType(), URES_INT);
    intResult=res.getInt(status);
    uintResult = res.getUInt(status);
    if(U_SUCCESS(status)){
        CONFIRM_UErrorCode(status, U_ZERO_ERROR);
        CONFIRM_EQ(uintResult, (uint32_t)intResult);
        CONFIRM_EQ(intResult, 1);
    }

    res = theBundle.get("onehundredtwentythree",status);
    CONFIRM_UErrorCode(status, U_ZERO_ERROR);
    CONFIRM_EQ(res.getType(), URES_INT);
    intResult=res.getInt(status);
    if(U_SUCCESS(status)){
        CONFIRM_UErrorCode(status, U_ZERO_ERROR);
        CONFIRM_EQ(intResult, 123);
    }

    /* this tests if escapes are preserved or not */
    {
        UnicodeString str = theBundle.getStringEx("testescape",status);
        CONFIRM_UErrorCode(status, U_ZERO_ERROR);
        if(U_SUCCESS(status)){
            u_charsToUChars(expect,uExpect,(int32_t)uprv_strlen(expect)+1);
            if(str.compare(uExpect)!=0){
                errln("Did not get the expected string for testescape expected. Expected : " 
                    +UnicodeString(uExpect )+ " Got: " + str);
            }
        }
    }
    /* test for jitterbug#1435 */
    {
        UnicodeString str = theBundle.getStringEx("test_underscores",status);
        expect ="test message ....";
        CONFIRM_UErrorCode(status, U_ZERO_ERROR);
        u_charsToUChars(expect,uExpect,(int32_t)uprv_strlen(expect)+1);
        if(str.compare(uExpect)!=0){
            errln("Did not get the expected string for test_underscores.\n");
        }
    }


}

void
NewResourceBundleTest::TestGetByFallback() {
    UErrorCode status = U_ZERO_ERROR;

    ResourceBundle heRes(NULL, "he", status);

    heRes.getWithFallback("calendar", status).getWithFallback("islamic-civil", status).getWithFallback("DateTime", status);
    if(U_SUCCESS(status)) {
        errln("he locale's Islamic-civil DateTime resource exists. How did it get here?\n");
    }
    status = U_ZERO_ERROR;

    heRes.getWithFallback("calendar", status).getWithFallback("islamic-civil", status).getWithFallback("eras", status);
    if(U_FAILURE(status)) {
        dataerrln("Didn't get Islamic Eras. I know they are there! - %s", u_errorName(status));
    }
    status = U_ZERO_ERROR;

    ResourceBundle rootRes(NULL, "root", status);
    rootRes.getWithFallback("calendar", status).getWithFallback("islamic-civil", status).getWithFallback("DateTime", status);
    if(U_SUCCESS(status)) {
        errln("Root's Islamic-civil's DateTime resource exists. How did it get here?\n");
    }
    status = U_ZERO_ERROR;

}
//eof

