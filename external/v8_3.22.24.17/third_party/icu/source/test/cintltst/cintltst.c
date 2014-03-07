/********************************************************************
 * COPYRIGHT:
 * Copyright (c) 1997-2010, International Business Machines Corporation and
 * others. All Rights Reserved.
 ********************************************************************/
/********************************************************************************
*
* File CINTLTST.C
*
* Modification History:
*        Name                     Description
*     Madhu Katragadda               Creation
*********************************************************************************
*/

/*The main root for C API tests*/

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include "unicode/utypes.h"
#include "unicode/putil.h"
#include "cstring.h"
#include "cintltst.h"
#include "umutex.h"
#include "uassert.h"
#include "cmemory.h"
#include "unicode/uchar.h"
#include "unicode/ustring.h"
#include "unicode/ucnv.h"
#include "unicode/ures.h"
#include "unicode/uclean.h"
#include "unicode/ucal.h"
#include "uoptions.h"
#include "putilimp.h" /* for uprv_getRawUTCtime() */
#ifdef URES_DEBUG
#include "uresimp.h" /* for ures_dumpCacheContents() */
#endif

#ifdef XP_MAC_CONSOLE
#   include <console.h>
#endif

#define CTST_LEAK_CHECK 1
#ifdef CTST_LEAK_CHECK
U_CFUNC void ctst_freeAll(void);
#endif

static char* _testDataPath=NULL;

/*
 *  Forward Declarations
 */
void ctest_setICU_DATA(void);



#if UCONFIG_NO_LEGACY_CONVERSION
#   define TRY_CNV_1 "iso-8859-1"
#   define TRY_CNV_2 "ibm-1208"
#else
#   define TRY_CNV_1 "iso-8859-7"
#   define TRY_CNV_2 "sjis"
#endif

static int gOrigArgc;
static const char* const * gOrigArgv;

int main(int argc, const char* const argv[])
{
    int nerrors = 0;
    UBool   defaultDataFound;
    TestNode *root;
    const char *warnOrErr = "Failure"; 
    UDate startTime, endTime;
    int32_t diffTime;

    /* initial check for the default converter */
    UErrorCode errorCode = U_ZERO_ERROR;
    UResourceBundle *rb;
    UConverter *cnv;

    U_MAIN_INIT_ARGS(argc, argv);

    startTime = uprv_getRawUTCtime();

    gOrigArgc = argc;
    gOrigArgv = argv;
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
    defaultDataFound = TRUE;
    u_init(&errorCode);
    if (U_FAILURE(errorCode)) {
        fprintf(stderr,
            "#### Note:  ICU Init without build-specific setDataDirectory() failed. %s\n", u_errorName(errorCode));
        defaultDataFound = FALSE;
    }
    u_cleanup();
#ifdef URES_DEBUG
    fprintf(stderr, "After initial u_cleanup: RB cache %s empty.\n", ures_dumpCacheContents()?"WAS NOT":"was");
#endif

    while (getTestOption(REPEAT_TESTS_OPTION) > 0) {   /* Loop runs once per complete execution of the tests
                                  *   used for -r  (repeat) test option.                */
        if (!initArgs(argc, argv, NULL, NULL)) {
            /* Error already displayed. */
            return -1;
        }
        errorCode = U_ZERO_ERROR;

        /* Initialize ICU */
        if (!defaultDataFound) {
            ctest_setICU_DATA();    /* u_setDataDirectory() must happen Before u_init() */
        }
        u_init(&errorCode);
        if (U_FAILURE(errorCode)) {
            fprintf(stderr,
                "#### ERROR! %s: u_init() failed with status = \"%s\".\n" 
                "*** Check the ICU_DATA environment variable and \n"
                "*** check that the data files are present.\n", argv[0], u_errorName(errorCode));
                if(!getTestOption(WARN_ON_MISSING_DATA_OPTION)) {
                    fprintf(stderr, "*** Exiting.  Use the '-w' option if data files were\n*** purposely removed, to continue test anyway.\n");
                    u_cleanup();
                    return 1;
                }
        }
        


        /* try more data */
        cnv = ucnv_open(TRY_CNV_2, &errorCode);
        if(cnv != 0) {
            /* ok */
            ucnv_close(cnv);
        } else {
            fprintf(stderr,
                    "*** %s! The converter for " TRY_CNV_2 " cannot be opened.\n"
                    "*** Check the ICU_DATA environment variable and \n"
                    "*** check that the data files are present.\n", warnOrErr);
            if(!getTestOption(WARN_ON_MISSING_DATA_OPTION)) {
                fprintf(stderr, "*** Exitting.  Use the '-w' option if data files were\n*** purposely removed, to continue test anyway.\n");
                u_cleanup();
                return 1;
            }
        }

        rb = ures_open(NULL, "en", &errorCode);
        if(U_SUCCESS(errorCode)) {
            /* ok */
            ures_close(rb);
        } else {
            fprintf(stderr,
                    "*** %s! The \"en\" locale resource bundle cannot be opened.\n"
                    "*** Check the ICU_DATA environment variable and \n"
                    "*** check that the data files are present.\n", warnOrErr);
            if(!getTestOption(WARN_ON_MISSING_DATA_OPTION)) {
                fprintf(stderr, "*** Exitting.  Use the '-w' option if data files were\n*** purposely removed, to continue test anyway.\n");
                u_cleanup();
                return 1;
            }
        }

        errorCode = U_ZERO_ERROR;
        rb = ures_open(NULL, NULL, &errorCode);
        if(U_SUCCESS(errorCode)) {
            /* ok */
            if (errorCode == U_USING_DEFAULT_WARNING || errorCode == U_USING_FALLBACK_WARNING) {
                fprintf(stderr,
                        "#### Note: The default locale %s is not available\n", uloc_getDefault());
            }
            ures_close(rb);
        } else {
            fprintf(stderr,
                    "*** %s! Can not open a resource bundle for the default locale %s\n", warnOrErr, uloc_getDefault());
            if(!getTestOption(WARN_ON_MISSING_DATA_OPTION)) {
                fprintf(stderr, "*** Exitting.  Use the '-w' option if data files were\n"
                    "*** purposely removed, to continue test anyway.\n");
                u_cleanup();
                return 1;
            }
        }
        fprintf(stdout, "Default locale for this run is %s\n", uloc_getDefault());

        /* Build a tree of all tests.   
         *   Subsequently will be used to find / iterate the tests to run */
        root = NULL;
        addAllTests(&root);

        /*  Tests acutally run HERE.   TODO:  separate command line option parsing & setting from test execution!! */
        nerrors = runTestRequest(root, argc, argv);

        setTestOption(REPEAT_TESTS_OPTION, DECREMENT_OPTION_VALUE);
        if (getTestOption(REPEAT_TESTS_OPTION) > 0) {
            printf("Repeating tests %d more time(s)\n", getTestOption(REPEAT_TESTS_OPTION));
        }
        cleanUpTestTree(root);

#ifdef CTST_LEAK_CHECK
        ctst_freeAll();
        /* To check for leaks */
        u_cleanup(); /* nuke the hashtable.. so that any still-open cnvs are leaked */
#ifdef URES_DEBUG
        if(ures_dumpCacheContents()) {
          fprintf(stderr, "Error: After final u_cleanup, RB cache was not empty.\n");
          nerrors++;
        } else {
          fprintf(stderr,"OK: After final u_cleanup, RB cache was empty.\n");
        }
#endif
#endif

    }  /* End of loop that repeats the entire test, if requested.  (Normally doesn't loop)  */

    if (ALLOCATION_COUNT > 0) {
        fprintf(stderr, "There were %d blocks leaked!\n", ALLOCATION_COUNT);
        nerrors++;
    }
    endTime = uprv_getRawUTCtime();
    diffTime = (int32_t)(endTime - startTime);
    printf("Elapsed Time: %02d:%02d:%02d.%03d\n",
        (int)((diffTime%U_MILLIS_PER_DAY)/U_MILLIS_PER_HOUR),
        (int)((diffTime%U_MILLIS_PER_HOUR)/U_MILLIS_PER_MINUTE),
        (int)((diffTime%U_MILLIS_PER_MINUTE)/U_MILLIS_PER_SECOND),
        (int)(diffTime%U_MILLIS_PER_SECOND));

    return nerrors ? 1 : 0;
}

/*
static void ctest_appendToDataDirectory(const char *toAppend)
{
    const char *oldPath ="";
    char newBuf [1024];
    char *newPath = newBuf;
    int32_t oldLen;
    int32_t newLen;

    if((toAppend == NULL) || (*toAppend == 0)) {
        return;
    }

    oldPath = u_getDataDirectory();
    if( (oldPath==NULL) || (*oldPath == 0)) {
        u_setDataDirectory(toAppend);
    } else {
        oldLen = strlen(oldPath);
        newLen = strlen(toAppend)+1+oldLen;

        if(newLen > 1022)
        {
            newPath = (char *)ctst_malloc(newLen);
        }

        strcpy(newPath, oldPath);
        strcpy(newPath+oldLen, U_PATH_SEP_STRING);
        strcpy(newPath+oldLen+1, toAppend);

        u_setDataDirectory(newPath);

        if(newPath != newBuf)
        {
            free(newPath);
        }
    }
}
*/

/* returns the path to icu/source/data */
const char *  ctest_dataSrcDir()
{
    static const char *dataSrcDir = NULL;

    if(dataSrcDir) {
        return dataSrcDir;
    }

    /* U_TOPSRCDIR is set by the makefiles on UNIXes when building cintltst and intltst
    //              to point to the top of the build hierarchy, which may or
    //              may not be the same as the source directory, depending on
    //              the configure options used.  At any rate,
    //              set the data path to the built data from this directory.
    //              The value is complete with quotes, so it can be used
    //              as-is as a string constant.
    */
#if defined (U_TOPSRCDIR)
    {
        dataSrcDir = U_TOPSRCDIR  U_FILE_SEP_STRING "data" U_FILE_SEP_STRING;
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
            strcpy(pBackSlash, U_FILE_SEP_STRING "data" U_FILE_SEP_STRING );
            dataSrcDir = p;
        }
        else {
            /* __FILE__ on MSVC7 does not contain the directory */
            FILE *file = fopen(".."U_FILE_SEP_STRING".."U_FILE_SEP_STRING "data" U_FILE_SEP_STRING "Makefile.in", "r");
            if (file) {
                fclose(file);
                dataSrcDir = ".."U_FILE_SEP_STRING".."U_FILE_SEP_STRING "data" U_FILE_SEP_STRING;
            }
            else {
                dataSrcDir = ".."U_FILE_SEP_STRING".."U_FILE_SEP_STRING".."U_FILE_SEP_STRING".."U_FILE_SEP_STRING "data" U_FILE_SEP_STRING;
            }
        }
    }
#endif

    return dataSrcDir;

}

/* returns the path to icu/source/data/out */
const char *ctest_dataOutDir()
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
                dataOutDir = ".."U_FILE_SEP_STRING".."U_FILE_SEP_STRING".."U_FILE_SEP_STRING".."U_FILE_SEP_STRING "data" U_FILE_SEP_STRING "out" U_FILE_SEP_STRING;
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
void ctest_setICU_DATA() {

    /* No location for the data dir was identifiable.
     *   Add other fallbacks for the test data location here if the need arises
     */
    if (getenv("ICU_DATA") == NULL) {
        /* If ICU_DATA isn't set, set it to the usual location */
        u_setDataDirectory(ctest_dataOutDir());
    }
}

/*  These tests do cleanup and reinitialize ICU in the course of their operation.
 *    The ICU data directory must be preserved across these operations.
 *    Here is a helper function to assist with that.
 */
static char *safeGetICUDataDirectory() {
    const char *dataDir = u_getDataDirectory();  /* Returned string vanashes with u_cleanup */
    char *retStr = NULL;
    if (dataDir != NULL) {
        retStr = (char *)malloc(strlen(dataDir)+1);
        strcpy(retStr, dataDir);
    }
    return retStr;
}

UBool ctest_resetICU() {
    UErrorCode   status = U_ZERO_ERROR;
    char         *dataDir = safeGetICUDataDirectory();

    u_cleanup();
    if (!initArgs(gOrigArgc, gOrigArgv, NULL, NULL)) {
        /* Error already displayed. */
        return FALSE;
    }
    u_setDataDirectory(dataDir);
    free(dataDir);
    u_init(&status);
    if (U_FAILURE(status)) {
        log_err_status(status, "u_init failed with %s\n", u_errorName(status));
        return FALSE;
    }
    return TRUE;
}

UChar* CharsToUChars(const char* str) {
    /* Might be faster to just use uprv_strlen() as the preflight len - liu */
    int32_t len = u_unescape(str, 0, 0); /* preflight */
    /* Do NOT use malloc() - we are supposed to be acting like user code! */
    UChar *buf = (UChar*) malloc(sizeof(UChar) * (len + 1));
    u_unescape(str, buf, len + 1);
    return buf;
}

char *austrdup(const UChar* unichars)
{
    int   length;
    char *newString;

    length    = u_strlen ( unichars );
    /*newString = (char*)malloc  ( sizeof( char ) * 4 * ( length + 1 ) );*/ /* this leaks for now */
    newString = (char*)ctst_malloc  ( sizeof( char ) * 4 * ( length + 1 ) ); /* this shouldn't */

    if ( newString == NULL )
        return NULL;

    u_austrcpy ( newString, unichars );

    return newString;
}

char *aescstrdup(const UChar* unichars,int32_t length){
    char *newString,*targetLimit,*target;
    UConverterFromUCallback cb;
    const void *p;
    UErrorCode errorCode = U_ZERO_ERROR;
#if U_CHARSET_FAMILY==U_EBCDIC_FAMILY
#   ifdef OS390
        static const char convName[] = "ibm-1047";
#   else
        static const char convName[] = "ibm-37";
#   endif
#else
    static const char convName[] = "US-ASCII";
#endif
    UConverter* conv = ucnv_open(convName, &errorCode);
    if(length==-1){
        length = u_strlen( unichars);
    }
    newString = (char*)ctst_malloc ( sizeof(char) * 8 * (length +1));
    target = newString;
    targetLimit = newString+sizeof(char) * 8 * (length +1);
    ucnv_setFromUCallBack(conv, UCNV_FROM_U_CALLBACK_ESCAPE, UCNV_ESCAPE_C, &cb, &p, &errorCode);
    ucnv_fromUnicode(conv,&target,targetLimit, &unichars, (UChar*)(unichars+length),NULL,TRUE,&errorCode);
    ucnv_close(conv);
    *target = '\0';
    return newString;
}

const char* loadTestData(UErrorCode* err){
    if( _testDataPath == NULL){
        const char*      directory=NULL;
        UResourceBundle* test =NULL;
        char* tdpath=NULL;
        const char* tdrelativepath;
#if defined (U_TOPBUILDDIR)
        tdrelativepath = "test"U_FILE_SEP_STRING"testdata"U_FILE_SEP_STRING"out"U_FILE_SEP_STRING;
        directory = U_TOPBUILDDIR;
#else
        tdrelativepath = ".."U_FILE_SEP_STRING".."U_FILE_SEP_STRING"test"U_FILE_SEP_STRING"testdata"U_FILE_SEP_STRING"out"U_FILE_SEP_STRING;
        directory= ctest_dataOutDir();
#endif

        tdpath = (char*) ctst_malloc(sizeof(char) *(( strlen(directory) * strlen(tdrelativepath)) + 10));


        /* u_getDataDirectory shoul return \source\data ... set the
         * directory to ..\source\data\..\test\testdata\out\testdata
         *
         * Fallback: When Memory mapped file is built
         * ..\source\data\out\..\..\test\testdata\out\testdata
         */
        strcpy(tdpath, directory);
        strcat(tdpath, tdrelativepath);
        strcat(tdpath,"testdata");


        test=ures_open(tdpath, "testtypes", err);

        /* Fall back did not succeed either so return */
        if(U_FAILURE(*err)){
            *err = U_FILE_ACCESS_ERROR;
            log_data_err("Could not load testtypes.res in testdata bundle with path %s - %s\n", tdpath, u_errorName(*err));
            return "";
        }
        ures_close(test);
        _testDataPath = tdpath;
        return _testDataPath;
    }
    return _testDataPath;
}

#define CTEST_MAX_TIMEZONE_SIZE 256
static UChar gOriginalTimeZone[CTEST_MAX_TIMEZONE_SIZE] = {0};

/**
 * Call this once to get a consistent timezone. Use ctest_resetTimeZone to set it back to the original value.
 * @param optionalTimeZone Set this to a requested timezone.
 *      Set to NULL to use the standard test timezone (Pacific Time)
 */
U_CFUNC void ctest_setTimeZone(const char *optionalTimeZone, UErrorCode *status) {
#if !UCONFIG_NO_FORMATTING
    UChar zoneID[CTEST_MAX_TIMEZONE_SIZE];

    if (optionalTimeZone == NULL) {
        optionalTimeZone = "America/Los_Angeles";
    }
    if (gOriginalTimeZone[0]) {
        log_data_err("*** Error: time zone saved twice. New value will be %s (Are you missing data?)\n",
               optionalTimeZone);
    }
    ucal_getDefaultTimeZone(gOriginalTimeZone, CTEST_MAX_TIMEZONE_SIZE, status);
    if (U_FAILURE(*status)) {
        log_err("*** Error: Failed to save default time zone: %s\n",
               u_errorName(*status));
        *status = U_ZERO_ERROR;
    }

    u_uastrncpy(zoneID, optionalTimeZone, CTEST_MAX_TIMEZONE_SIZE-1);
    zoneID[CTEST_MAX_TIMEZONE_SIZE-1] = 0;
    ucal_setDefaultTimeZone(zoneID, status);
    if (U_FAILURE(*status)) {
        log_err("*** Error: Failed to set default time zone to \"%s\": %s\n",
               optionalTimeZone, u_errorName(*status));
    }
#endif
}

/**
 * Call this once get back the original timezone
 */
U_CFUNC void ctest_resetTimeZone(void) {
#if !UCONFIG_NO_FORMATTING
    UErrorCode status = U_ZERO_ERROR;

    ucal_setDefaultTimeZone(gOriginalTimeZone, &status);
    if (U_FAILURE(status)) {
        log_err("*** Error: Failed to reset default time zone: %s\n",
               u_errorName(status));
    }
    /* Set to an empty state */
    gOriginalTimeZone[0] = 0;
#endif
}

#define CTST_MAX_ALLOC 8192
/* Array used as a queue */
static void * ctst_allocated_stuff[CTST_MAX_ALLOC] = {0};
static int ctst_allocated = 0;
static UBool ctst_free = FALSE;

void *ctst_malloc(size_t size) {
    if(ctst_allocated >= CTST_MAX_ALLOC - 1) {
        ctst_allocated = 0;
        ctst_free = TRUE;
    }
    if(ctst_allocated_stuff[ctst_allocated]) {
        free(ctst_allocated_stuff[ctst_allocated]);
    }
    return ctst_allocated_stuff[ctst_allocated++] = malloc(size);
}

#ifdef CTST_LEAK_CHECK
void ctst_freeAll() {
    int i;
    if(ctst_free == 0) {
        for(i=0; i<ctst_allocated; i++) {
            free(ctst_allocated_stuff[i]);
            ctst_allocated_stuff[i] = NULL;
        }
    } else {
        for(i=0; i<CTST_MAX_ALLOC; i++) {
            free(ctst_allocated_stuff[i]);
            ctst_allocated_stuff[i] = NULL;
        }
    }
    ctst_allocated = 0;
    _testDataPath=NULL;
}

#define VERBOSE_ASSERTIONS

U_CFUNC UBool assertSuccess(const char* msg, UErrorCode* ec) {
    U_ASSERT(ec!=NULL);
    if (U_FAILURE(*ec)) {
        log_err_status(*ec, "FAIL: %s (%s)\n", msg, u_errorName(*ec));
        return FALSE;
    }
    return TRUE;
}

/* if 'condition' is a UBool, the compiler complains bitterly about
   expressions like 'a > 0' which it evaluates as int */
U_CFUNC UBool assertTrue(const char* msg, int /*not UBool*/ condition) { 
    if (!condition) {
        log_err("FAIL: assertTrue() failed: %s\n", msg);
    }
#ifdef VERBOSE_ASSERTIONS
    else {
        log_verbose("Ok: %s\n", msg);
    }
#endif
    return (UBool)condition;   
}

U_CFUNC UBool assertEquals(const char* message, const char* expected,
                           const char* actual) {
    if (uprv_strcmp(expected, actual) != 0) {
        log_err("FAIL: %s; got \"%s\"; expected \"%s\"\n",
                message, actual, expected);
        return FALSE;
    }
#ifdef VERBOSE_ASSERTIONS
    else {
        log_verbose("Ok: %s; got \"%s\"\n", message, actual);
    }
#endif
    return TRUE;
}
/*--------------------------------------------------------------------
 * Time bomb - allows temporary behavior that expires at a given
 *             release
 *--------------------------------------------------------------------
 */

U_CFUNC UBool isICUVersionAtLeast(const UVersionInfo x) {
    UVersionInfo v;
    u_getVersion(v);
    return (uprv_memcmp(v, x, U_MAX_VERSION_LENGTH) >= 0);
}
#endif
