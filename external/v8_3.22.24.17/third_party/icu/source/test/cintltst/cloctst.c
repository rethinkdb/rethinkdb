/********************************************************************
 * COPYRIGHT:
 * Copyright (c) 1997-2010, International Business Machines Corporation and
 * others. All Rights Reserved.
 ********************************************************************/
/*****************************************************************************
*
* File CLOCTST.C
*
* Modification History:
*        Name                     Description 
*     Madhu Katragadda            Ported for C API
******************************************************************************
*/
#include "cloctst.h"
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include "cintltst.h"
#include "cstring.h"
#include "uparse.h"
#include "uresimp.h"

#include "unicode/putil.h"
#include "unicode/ubrk.h"
#include "unicode/uchar.h"
#include "unicode/ucol.h"
#include "unicode/udat.h"
#include "unicode/uloc.h"
#include "unicode/umsg.h"
#include "unicode/ures.h"
#include "unicode/uset.h"
#include "unicode/ustring.h"
#include "unicode/utypes.h"
#include "unicode/ulocdata.h"
#include "unicode/parseerr.h" /* may not be included with some uconfig switches */
#include "udbgutil.h"
#define LENGTHOF(array) (int32_t)(sizeof(array)/sizeof((array)[0]))

static void TestNullDefault(void);
static void TestNonexistentLanguageExemplars(void);
static void TestLocDataErrorCodeChaining(void);
static void TestLanguageExemplarsFallbacks(void);

static void TestUnicodeDefines(void);

void PrintDataTable();

/*---------------------------------------------------
  table of valid data
 --------------------------------------------------- */
#define LOCALE_SIZE 9
#define LOCALE_INFO_SIZE 28

static const char* const rawData2[LOCALE_INFO_SIZE][LOCALE_SIZE] = {
    /* language code */
    {   "en",   "fr",   "ca",   "el",   "no",   "zh",   "de",   "es",  "ja"    },
    /* script code */
    {   "",     "",     "",     "",     "",     "", "", "", ""  },
    /* country code */
    {   "US",   "FR",   "ES",   "GR",   "NO",   "CN", "DE", "", "JP"    },
    /* variant code */
    {   "",     "",     "",     "",     "NY",   "", "", "", ""      },
    /* full name */
    {   "en_US",    "fr_FR",    "ca_ES",    
        "el_GR",    "no_NO_NY", "zh_Hans_CN", 
        "de_DE@collation=phonebook", "es@collation=traditional",  "ja_JP@calendar=japanese" },
    /* ISO-3 language */
    {   "eng",  "fra",  "cat",  "ell",  "nor",  "zho", "deu", "spa", "jpn"   },
    /* ISO-3 country */
    {   "USA",  "FRA",  "ESP",  "GRC",  "NOR",  "CHN", "DEU", "", "JPN"   },
    /* LCID */
    {   "409", "40c", "403", "408", "814",  "804", "10407", "40a", "411"     },

    /* display language (English) */
    {   "English",  "French",   "Catalan", "Greek",    "Norwegian", "Chinese", "German", "Spanish", "Japanese"    },
    /* display script code (English) */
    {   "",     "",     "",     "",     "",     "Simplified Han", "", "", ""       },
    /* display country (English) */
    {   "United States",    "France",   "Spain",  "Greece",   "Norway", "China", "Germany", "", "Japan"       },
    /* display variant (English) */
    {   "",     "",     "",     "",     "NY",  "", "", "", ""       },
    /* display name (English) */
    {   "English (United States)", "French (France)", "Catalan (Spain)", 
        "Greek (Greece)", "Norwegian (Norway, NY)", "Chinese (Simplified Han, China)", 
        "German (Germany, Collation=Phonebook Sort Order)", "Spanish (Collation=Traditional Sort Order)", "Japanese (Japan, Calendar=Japanese Calendar)" },

    /* display language (French) */
    {   "anglais",  "fran\\u00E7ais",   "catalan", "grec",    "norv\\u00E9gien",    "chinois", "allemand", "espagnol", "japonais"     },
    /* display script code (French) */
    {   "",     "",     "",     "",     "",     "id\\u00e9ogrammes han simplifi\\u00e9s", "", "", ""         },
    /* display country (French) */
    {   "\\u00C9tats-Unis",    "France",   "Espagne",  "Gr\\u00E8ce",   "Norv\\u00E8ge",    "Chine", "Allemagne", "", "Japon"       },
    /* display variant (French) */
    {   "",     "",     "",     "",     "NY",   "", "", "", ""       },
    /* display name (French) */
    {   "anglais (\\u00C9tats-Unis)", "fran\\u00E7ais (France)", "catalan (Espagne)", 
        "grec (Gr\\u00E8ce)", "norv\\u00E9gien (Norv\\u00E8ge, NY)",  "chinois (id\\u00e9ogrammes han simplifi\\u00e9s, Chine)", 
        "allemand (Allemagne, Ordonnancement=Ordre de l\\u2019annuaire)", "espagnol (Ordonnancement=Ordre traditionnel)", "japonais (Japon, Calendrier=Calendrier japonais)" },

    /* display language (Catalan) */
    {   "angl\\u00E8s", "franc\\u00E8s", "catal\\u00E0", "grec",  "noruec", "xin\\u00E8s", "alemany", "espanyol", "japon\\u00E8s"    },
    /* display script code (Catalan) */
    {   "",     "",     "",     "",     "",     "xin\\u00e8s simplificat", "", "", ""         },
    /* display country (Catalan) */
    {   "Estats Units", "Fran\\u00E7a", "Espanya",  "Gr\\u00E8cia", "Noruega",  "Xina", "Alemanya", "", "Jap\\u00F3"    },
    /* display variant (Catalan) */
    {   "", "", "",                    "", "NY",    "", "", "", ""    },
    /* display name (Catalan) */
    {   "angl\\u00E8s (Estats Units)", "franc\\u00E8s (Fran\\u00E7a)", "catal\\u00E0 (Espanya)", 
    "grec (Gr\\u00E8cia)", "noruec (Noruega, NY)", "xin\\u00E8s (xin\\u00e8s simplificat, Xina)", 
    "alemany (Alemanya, ordre alfab\\u00e8tic=ordre de la guia telef\\u00F2nica)", "espanyol (ordre alfab\\u00e8tic=ordre tradicional)", "japon\\u00E8s (Jap\\u00F3, calendari=calendari japon\\u00e8s)" },

    /* display language (Greek) */
    {
        "\\u0391\\u03b3\\u03b3\\u03bb\\u03b9\\u03ba\\u03ac",
        "\\u0393\\u03b1\\u03bb\\u03bb\\u03b9\\u03ba\\u03ac",
        "\\u039a\\u03b1\\u03c4\\u03b1\\u03bb\\u03b1\\u03bd\\u03b9\\u03ba\\u03ac",
        "\\u0395\\u03bb\\u03bb\\u03b7\\u03bd\\u03b9\\u03ba\\u03ac",
        "\\u039d\\u03bf\\u03c1\\u03b2\\u03b7\\u03b3\\u03b9\\u03ba\\u03ac",
        "\\u039A\\u03B9\\u03BD\\u03B5\\u03B6\\u03B9\\u03BA\\u03AC", 
        "\\u0393\\u03B5\\u03C1\\u03BC\\u03B1\\u03BD\\u03B9\\u03BA\\u03AC", 
        "\\u0399\\u03C3\\u03C0\\u03B1\\u03BD\\u03B9\\u03BA\\u03AC", 
        "\\u0399\\u03B1\\u03C0\\u03C9\\u03BD\\u03B9\\u03BA\\u03AC"   
    },
    /* display script code (Greek) */

    {   "",     "",     "",     "",     "", "\\u0391\\u03c0\\u03bb\\u03bf\\u03c0\\u03bf\\u03b9\\u03b7\\u03bc\\u03ad\\u03bd\\u03bf \\u039a\\u03b9\\u03bd\\u03b5\\u03b6\\u03b9\\u03ba\\u03cc", "", "", "" },
    /* display country (Greek) */
    {
        "\\u0397\\u03BD\\u03C9\\u03BC\\u03AD\\u03BD\\u03B5\\u03C2 \\u03A0\\u03BF\\u03BB\\u03B9\\u03C4\\u03B5\\u03AF\\u03B5\\u03C2 \\u03C4\\u03B7\\u03C2 \\u0391\\u03BC\\u03B5\\u03C1\\u03B9\\u03BA\\u03AE\\u03C2",
        "\\u0393\\u03b1\\u03bb\\u03bb\\u03af\\u03b1",
        "\\u0399\\u03c3\\u03c0\\u03b1\\u03bd\\u03af\\u03b1",
        "\\u0395\\u03bb\\u03bb\\u03ac\\u03b4\\u03b1",
        "\\u039d\\u03bf\\u03c1\\u03b2\\u03b7\\u03b3\\u03af\\u03b1",
        "\\u039A\\u03AF\\u03BD\\u03B1", 
        "\\u0393\\u03B5\\u03C1\\u03BC\\u03B1\\u03BD\\u03AF\\u03B1", 
        "", 
        "\\u0399\\u03B1\\u03C0\\u03C9\\u03BD\\u03AF\\u03B1"   
    },
    /* display variant (Greek) */
    {   "", "", "", "", "NY", "", "", "", ""    }, /* TODO: currently there is no translation for NY in Greek fix this test when we have it */
    /* display name (Greek) */
    {
        "\\u0391\\u03b3\\u03b3\\u03bb\\u03b9\\u03ba\\u03ac (\\u0397\\u03BD\\u03C9\\u03BC\\u03AD\\u03BD\\u03B5\\u03C2 \\u03A0\\u03BF\\u03BB\\u03B9\\u03C4\\u03B5\\u03AF\\u03B5\\u03C2 \\u03C4\\u03B7\\u03C2 \\u0391\\u03BC\\u03B5\\u03C1\\u03B9\\u03BA\\u03AE\\u03C2)",
        "\\u0393\\u03b1\\u03bb\\u03bb\\u03b9\\u03ba\\u03ac (\\u0393\\u03b1\\u03bb\\u03bb\\u03af\\u03b1)",
        "\\u039a\\u03b1\\u03c4\\u03b1\\u03bb\\u03b1\\u03bd\\u03b9\\u03ba\\u03ac (\\u0399\\u03c3\\u03c0\\u03b1\\u03bd\\u03af\\u03b1)",
        "\\u0395\\u03bb\\u03bb\\u03b7\\u03bd\\u03b9\\u03ba\\u03ac (\\u0395\\u03bb\\u03bb\\u03ac\\u03b4\\u03b1)",
        "\\u039d\\u03bf\\u03c1\\u03b2\\u03b7\\u03b3\\u03b9\\u03ba\\u03ac (\\u039d\\u03bf\\u03c1\\u03b2\\u03b7\\u03b3\\u03af\\u03b1, NY)",
        "\\u039A\\u03B9\\u03BD\\u03B5\\u03B6\\u03B9\\u03BA\\u03AC (\\u0391\\u03c0\\u03bb\\u03bf\\u03c0\\u03bf\\u03b9\\u03b7\\u03bc\\u03ad\\u03bd\\u03bf \\u039a\\u03b9\\u03bd\\u03b5\\u03b6\\u03b9\\u03ba\\u03cc, \\u039A\\u03AF\\u03BD\\u03B1)",
        "\\u0393\\u03B5\\u03C1\\u03BC\\u03B1\\u03BD\\u03B9\\u03BA\\u03AC (\\u0393\\u03B5\\u03C1\\u03BC\\u03B1\\u03BD\\u03AF\\u03B1, \\u03C4\\u03B1\\u03BA\\u03C4\\u03BF\\u03C0\\u03BF\\u03AF\\u03B7\\u03C3\\u03B7=\\u03A3\\u03B5\\u03B9\\u03C1\\u03AC \\u03C4\\u03B1\\u03BE\\u03B9\\u03BD\\u03CC\\u03BC\\u03B7\\u03C3\\u03B7\\u03C2 \\u03C4\\u03B7\\u03BB\\u03B5\\u03C6\\u03C9\\u03BD\\u03B9\\u03BA\\u03BF\\u03CD \\u03BA\\u03B1\\u03C4\\u03B1\\u03BB\\u03CC\\u03B3\\u03BF\\u03C5)", 
        "\\u0399\\u03C3\\u03C0\\u03B1\\u03BD\\u03B9\\u03BA\\u03AC (\\u03C4\\u03B1\\u03BA\\u03C4\\u03BF\\u03C0\\u03BF\\u03AF\\u03B7\\u03C3\\u03B7=\\u03A0\\u03B1\\u03C1\\u03B1\\u03B4\\u03BF\\u03C3\\u03B9\\u03B1\\u03BA\\u03AE \\u03C3\\u03B5\\u03B9\\u03C1\\u03AC \\u03C4\\u03B1\\u03BE\\u03B9\\u03BD\\u03CC\\u03BC\\u03B7\\u03C3\\u03B7\\u03C2)", 
        "\\u0399\\u03B1\\u03C0\\u03C9\\u03BD\\u03B9\\u03BA\\u03AC (\\u0399\\u03B1\\u03C0\\u03C9\\u03BD\\u03AF\\u03B1, \\u03B7\\u03BC\\u03B5\\u03C1\\u03BF\\u03BB\\u03CC\\u03B3\\u03B9\\u03BF=\\u0399\\u03B1\\u03C0\\u03C9\\u03BD\\u03B9\\u03BA\\u03CC \\u03B7\\u03BC\\u03B5\\u03C1\\u03BF\\u03BB\\u03CC\\u03B3\\u03B9\\u03BF)"
    }
};

static UChar*** dataTable=0;
enum {
    ENGLISH = 0,
    FRENCH = 1,
    CATALAN = 2,
    GREEK = 3,
    NORWEGIAN = 4
};

enum {
    LANG = 0,
    SCRIPT = 1,
    CTRY = 2,
    VAR = 3,
    NAME = 4,
    LANG3 = 5,
    CTRY3 = 6,
    LCID = 7,
    DLANG_EN = 8,
    DSCRIPT_EN = 9,
    DCTRY_EN = 10,
    DVAR_EN = 11,
    DNAME_EN = 12,
    DLANG_FR = 13,
    DSCRIPT_FR = 14,
    DCTRY_FR = 15,
    DVAR_FR = 16,
    DNAME_FR = 17,
    DLANG_CA = 18,
    DSCRIPT_CA = 19,
    DCTRY_CA = 20,
    DVAR_CA = 21,
    DNAME_CA = 22,
    DLANG_EL = 23,
    DSCRIPT_EL = 24,
    DCTRY_EL = 25,
    DVAR_EL = 26,
    DNAME_EL = 27
};

#define TESTCASE(name) addTest(root, &name, "tsutil/cloctst/" #name)

void addLocaleTest(TestNode** root);

void addLocaleTest(TestNode** root)
{
    TESTCASE(TestObsoleteNames); /* srl- move */
    TESTCASE(TestBasicGetters);
    TESTCASE(TestNullDefault);
    TESTCASE(TestPrefixes);
    TESTCASE(TestSimpleResourceInfo);
    TESTCASE(TestDisplayNames);
    TESTCASE(TestGetAvailableLocales);
    TESTCASE(TestDataDirectory);
#if !UCONFIG_NO_FILE_IO && !UCONFIG_NO_LEGACY_CONVERSION
    TESTCASE(TestISOFunctions);
#endif
    TESTCASE(TestISO3Fallback);
    TESTCASE(TestUninstalledISO3Names);
    TESTCASE(TestSimpleDisplayNames);
    TESTCASE(TestVariantParsing);
    TESTCASE(TestKeywordVariants);
    TESTCASE(TestKeywordVariantParsing);
    TESTCASE(TestCanonicalization);
    TESTCASE(TestKeywordSet);
    TESTCASE(TestKeywordSetError);
    TESTCASE(TestDisplayKeywords);
    TESTCASE(TestDisplayKeywordValues);
    TESTCASE(TestGetBaseName);
#if !UCONFIG_NO_FILE_IO
    TESTCASE(TestGetLocale);
#endif
    TESTCASE(TestDisplayNameWarning);
    TESTCASE(TestNonexistentLanguageExemplars);
    TESTCASE(TestLocDataErrorCodeChaining);
    TESTCASE(TestLanguageExemplarsFallbacks);
    TESTCASE(TestCalendar);
    TESTCASE(TestDateFormat);
    TESTCASE(TestCollation);
    TESTCASE(TestULocale);
    TESTCASE(TestUResourceBundle);
    TESTCASE(TestDisplayName); 
    TESTCASE(TestAcceptLanguage); 
    TESTCASE(TestGetLocaleForLCID);
    TESTCASE(TestOrientation);
    TESTCASE(TestLikelySubtags);
    TESTCASE(TestToLanguageTag);
    TESTCASE(TestForLanguageTag);
    TESTCASE(TestTrailingNull);
    TESTCASE(TestUnicodeDefines);
}


/* testing uloc(), uloc_getName(), uloc_getLanguage(), uloc_getVariant(), uloc_getCountry() */
static void TestBasicGetters() {
    int32_t i;
    int32_t cap;
    UErrorCode status = U_ZERO_ERROR;
    char *testLocale = 0;
    char *temp = 0, *name = 0;
    log_verbose("Testing Basic Getters\n");
    for (i = 0; i < LOCALE_SIZE; i++) {
        testLocale=(char*)malloc(sizeof(char) * (strlen(rawData2[NAME][i])+1));
        strcpy(testLocale,rawData2[NAME][i]);

        log_verbose("Testing   %s  .....\n", testLocale);
        cap=uloc_getLanguage(testLocale, NULL, 0, &status);
        if(status==U_BUFFER_OVERFLOW_ERROR){
            status=U_ZERO_ERROR;
            temp=(char*)malloc(sizeof(char) * (cap+1));
            uloc_getLanguage(testLocale, temp, cap+1, &status);
        }
        if(U_FAILURE(status)){
            log_err("ERROR: in uloc_getLanguage  %s\n", myErrorName(status));
        }
        if (0 !=strcmp(temp,rawData2[LANG][i]))    {
            log_err("  Language code mismatch: %s versus  %s\n", temp, rawData2[LANG][i]);
        }


        cap=uloc_getCountry(testLocale, temp, cap, &status);
        if(status==U_BUFFER_OVERFLOW_ERROR){
            status=U_ZERO_ERROR;
            temp=(char*)realloc(temp, sizeof(char) * (cap+1));
            uloc_getCountry(testLocale, temp, cap+1, &status);
        }
        if(U_FAILURE(status)){
            log_err("ERROR: in uloc_getCountry  %s\n", myErrorName(status));
        }
        if (0 != strcmp(temp, rawData2[CTRY][i])) {
            log_err(" Country code mismatch:  %s  versus   %s\n", temp, rawData2[CTRY][i]);

          }

        cap=uloc_getVariant(testLocale, temp, cap, &status);
        if(status==U_BUFFER_OVERFLOW_ERROR){
            status=U_ZERO_ERROR;
            temp=(char*)realloc(temp, sizeof(char) * (cap+1));
            uloc_getVariant(testLocale, temp, cap+1, &status);
        }
        if(U_FAILURE(status)){
            log_err("ERROR: in uloc_getVariant  %s\n", myErrorName(status));
        }
        if (0 != strcmp(temp, rawData2[VAR][i])) {
            log_err("Variant code mismatch:  %s  versus   %s\n", temp, rawData2[VAR][i]);
        }

        cap=uloc_getName(testLocale, NULL, 0, &status);
        if(status==U_BUFFER_OVERFLOW_ERROR){
            status=U_ZERO_ERROR;
            name=(char*)malloc(sizeof(char) * (cap+1));
            uloc_getName(testLocale, name, cap+1, &status);
        } else if(status==U_ZERO_ERROR) {
          log_err("ERROR: in uloc_getName(%s,NULL,0,..), expected U_BUFFER_OVERFLOW_ERROR!\n", testLocale);
        }
        if(U_FAILURE(status)){
            log_err("ERROR: in uloc_getName   %s\n", myErrorName(status));
        }
        if (0 != strcmp(name, rawData2[NAME][i])){
            log_err(" Mismatch in getName:  %s  versus   %s\n", name, rawData2[NAME][i]);
        }

        free(temp);
        free(name);

        free(testLocale);
    }
}

static void TestNullDefault() {
    UErrorCode status = U_ZERO_ERROR;
    char original[ULOC_FULLNAME_CAPACITY];

    uprv_strcpy(original, uloc_getDefault());
    uloc_setDefault("qq_BLA", &status);
    if (uprv_strcmp(uloc_getDefault(), "qq_BLA") != 0) {
        log_err(" Mismatch in uloc_setDefault:  qq_BLA  versus   %s\n", uloc_getDefault());
    }
    uloc_setDefault(NULL, &status);
    if (uprv_strcmp(uloc_getDefault(), original) != 0) {
        log_err(" uloc_setDefault(NULL, &status) didn't get the default locale back!\n");
    }

    {
    /* Test that set & get of default locale work, and that
     * default locales are cached and reused, and not overwritten.
     */
        const char *n_en_US;
        const char *n_fr_FR;
        const char *n2_en_US;
        
        status = U_ZERO_ERROR;
        uloc_setDefault("en_US", &status);
        n_en_US = uloc_getDefault();
        if (strcmp(n_en_US, "en_US") != 0) {
            log_err("Wrong result from uloc_getDefault().  Expected \"en_US\", got \"%s\"\n", n_en_US);
        }
        
        uloc_setDefault("fr_FR", &status);
        n_fr_FR = uloc_getDefault();
        if (strcmp(n_en_US, "en_US") != 0) {
            log_err("uloc_setDefault altered previously default string."
                "Expected \"en_US\", got \"%s\"\n",  n_en_US);
        }
        if (strcmp(n_fr_FR, "fr_FR") != 0) {
            log_err("Wrong result from uloc_getDefault().  Expected \"fr_FR\", got %s\n",  n_fr_FR);
        }
        
        uloc_setDefault("en_US", &status);
        n2_en_US = uloc_getDefault();
        if (strcmp(n2_en_US, "en_US") != 0) {
            log_err("Wrong result from uloc_getDefault().  Expected \"en_US\", got \"%s\"\n", n_en_US);
        }
        if (n2_en_US != n_en_US) {
            log_err("Default locale cache failed to reuse en_US locale.\n");
        }
        
        if (U_FAILURE(status)) {
            log_err("Failure returned from uloc_setDefault - \"%s\"\n", u_errorName(status));
        }
        
    }
    
}
/* Test the i- and x- and @ and . functionality 
*/

#define PREFIXBUFSIZ 128

static void TestPrefixes() {
    int row = 0;
    int n;
    const char *loc, *expected;
    
    static const char * const testData[][7] =
    {
        /* NULL canonicalize() column means "expect same as getName()" */
        {"sv", "", "FI", "AL", "sv-fi-al", "sv_FI_AL", NULL},
        {"en", "", "GB", "", "en-gb", "en_GB", NULL},
        {"i-hakka", "", "MT", "XEMXIJA", "i-hakka_MT_XEMXIJA", "i-hakka_MT_XEMXIJA", NULL},
        {"i-hakka", "", "CN", "", "i-hakka_CN", "i-hakka_CN", NULL},
        {"i-hakka", "", "MX", "", "I-hakka_MX", "i-hakka_MX", NULL},
        {"x-klingon", "", "US", "SANJOSE", "X-KLINGON_us_SANJOSE", "x-klingon_US_SANJOSE", NULL},
        
        {"zh", "Hans", "", "PINYIN", "zh-Hans-pinyin", "zh_Hans_PINYIN", "zh_Hans@collation=pinyin"},
        {"hy", "", "", "AREVMDA", "hy_AREVMDA", "hy_AREVMDA", NULL},
        
        {"mr", "", "", "", "mr.utf8", "mr.utf8", "mr"},
        {"de", "", "TV", "", "de-tv.koi8r", "de_TV.koi8r", "de_TV"},
        {"x-piglatin", "", "ML", "", "x-piglatin_ML.MBE", "x-piglatin_ML.MBE", "x-piglatin_ML"},  /* Multibyte English */
        {"i-cherokee", "","US", "", "i-Cherokee_US.utf7", "i-cherokee_US.utf7", "i-cherokee_US"},
        {"x-filfli", "", "MT", "FILFLA", "x-filfli_MT_FILFLA.gb-18030", "x-filfli_MT_FILFLA.gb-18030", "x-filfli_MT_FILFLA"},
        {"no", "", "NO", "NY", "no-no-ny.utf32@B", "no_NO_NY.utf32@B", "no_NO_NY_B"},
        {"no", "", "NO", "",  "no-no.utf32@B", "no_NO.utf32@B", "no_NO_B"},
        {"no", "", "",   "NY", "no__ny", "no__NY", NULL},
        {"no", "", "",   "", "no@ny", "no@ny", "no__NY"},
        {"el", "Latn", "", "", "el-latn", "el_Latn", NULL},
        {"en", "Cyrl", "RU", "", "en-cyrl-ru", "en_Cyrl_RU", NULL},
        {"zh", "Hant", "TW", "STROKE", "zh-hant_TW_STROKE", "zh_Hant_TW_STROKE", "zh_Hant_TW@collation=stroke"},
        {"qq", "Qqqq", "QQ", "QQ", "qq_Qqqq_QQ_QQ", "qq_Qqqq_QQ_QQ", NULL},
        {"qq", "Qqqq", "", "QQ", "qq_Qqqq__QQ", "qq_Qqqq__QQ", NULL},
        {"12", "3456", "78", "90", "12_3456_78_90", "12_3456_78_90", NULL}, /* total garbage */
        
        {NULL,NULL,NULL,NULL,NULL,NULL,NULL}
    };
    
    static const char * const testTitles[] = {
        "uloc_getLanguage()",
        "uloc_getScript()",
        "uloc_getCountry()",
        "uloc_getVariant()",
        "name",
        "uloc_getName()",
        "uloc_canonicalize()"
    };
    
    char buf[PREFIXBUFSIZ];
    int32_t len;
    UErrorCode err;
    
    
    for(row=0;testData[row][0] != NULL;row++) {
        loc = testData[row][NAME];
        log_verbose("Test #%d: %s\n", row, loc);
        
        err = U_ZERO_ERROR;
        len=0;
        buf[0]=0;
        for(n=0;n<=(NAME+2);n++) {
            if(n==NAME) continue;
            
            for(len=0;len<PREFIXBUFSIZ;len++) {
                buf[len] = '%'; /* Set a tripwire.. */
            }
            len = 0;
            
            switch(n) {
            case LANG:
                len = uloc_getLanguage(loc, buf, PREFIXBUFSIZ, &err);
                break;
                
            case SCRIPT:
                len = uloc_getScript(loc, buf, PREFIXBUFSIZ, &err);
                break;
                
            case CTRY:
                len = uloc_getCountry(loc, buf, PREFIXBUFSIZ, &err);
                break;
                
            case VAR:
                len = uloc_getVariant(loc, buf, PREFIXBUFSIZ, &err);
                break;
                
            case NAME+1:
                len = uloc_getName(loc, buf, PREFIXBUFSIZ, &err);
                break;
                
            case NAME+2:
                len = uloc_canonicalize(loc, buf, PREFIXBUFSIZ, &err);
                break;
                
            default:
                strcpy(buf, "**??");
                len=4;
            }
            
            if(U_FAILURE(err)) {
                log_err("#%d: %s on %s: err %s\n",
                    row, testTitles[n], loc, u_errorName(err));
            } else {
                log_verbose("#%d: %s on %s: -> [%s] (length %d)\n",
                    row, testTitles[n], loc, buf, len);
                
                if(len != (int32_t)strlen(buf)) {
                    log_err("#%d: %s on %s: -> [%s] (length returned %d, actual %d!)\n",
                        row, testTitles[n], loc, buf, len, strlen(buf)+1);
                    
                }
                
                /* see if they smashed something */
                if(buf[len+1] != '%') {
                    log_err("#%d: %s on %s: -> [%s] - wrote [%X] out ofbounds!\n",
                        row, testTitles[n], loc, buf, buf[len+1]);
                }
                
                expected = testData[row][n];
                if (expected == NULL && n == (NAME+2)) {
                    /* NULL expected canonicalize() means "expect same as getName()" */
                    expected = testData[row][NAME+1];
                }
                if(strcmp(buf, expected)) {
                    log_err("#%d: %s on %s: -> [%s] (expected '%s'!)\n",
                        row, testTitles[n], loc, buf, expected);
                    
                }
            }
        }
    }
}


/* testing uloc_getISO3Language(), uloc_getISO3Country(),  */
static void TestSimpleResourceInfo() {
    int32_t i;
    char* testLocale = 0;
    UChar* expected = 0;
    
    const char* temp;
    char            temp2[20];
    testLocale=(char*)malloc(sizeof(char) * 1);
    expected=(UChar*)malloc(sizeof(UChar) * 1);
    
    setUpDataTable();
    log_verbose("Testing getISO3Language and getISO3Country\n");
    for (i = 0; i < LOCALE_SIZE; i++) {
        
        testLocale=(char*)realloc(testLocale, sizeof(char) * (u_strlen(dataTable[NAME][i])+1));
        u_austrcpy(testLocale, dataTable[NAME][i]);
        
        log_verbose("Testing   %s ......\n", testLocale);
        
        temp=uloc_getISO3Language(testLocale);
        expected=(UChar*)realloc(expected, sizeof(UChar) * (strlen(temp) + 1));
        u_uastrcpy(expected,temp);
        if (0 != u_strcmp(expected, dataTable[LANG3][i])) {
            log_err("  ISO-3 language code mismatch:  %s versus  %s\n",  austrdup(expected),
                austrdup(dataTable[LANG3][i]));
        }
        
        temp=uloc_getISO3Country(testLocale);
        expected=(UChar*)realloc(expected, sizeof(UChar) * (strlen(temp) + 1));
        u_uastrcpy(expected,temp);
        if (0 != u_strcmp(expected, dataTable[CTRY3][i])) {
            log_err("  ISO-3 Country code mismatch:  %s versus  %s\n",  austrdup(expected),
                austrdup(dataTable[CTRY3][i]));
        }
        sprintf(temp2, "%x", (int)uloc_getLCID(testLocale));
        if (strcmp(temp2, rawData2[LCID][i]) != 0) {
            log_err("LCID mismatch: %s versus %s\n", temp2 , rawData2[LCID][i]);
        }
    }
    
    free(expected);
    free(testLocale);
    cleanUpDataTable();
}

/*
 * Jitterbug 2439 -- markus 20030425
 *
 * The lookup of display names must not fall back through the default
 * locale because that yields useless results.
 */
static void TestDisplayNames()
{
    UChar buffer[100];
    UErrorCode errorCode=U_ZERO_ERROR;
    int32_t length;
    log_verbose("Testing getDisplayName for different locales\n");

    log_verbose("  In locale = en_US...\n");
    doTestDisplayNames("en_US", DLANG_EN);
    log_verbose("  In locale = fr_FR....\n");
    doTestDisplayNames("fr_FR", DLANG_FR);
    log_verbose("  In locale = ca_ES...\n");
    doTestDisplayNames("ca_ES", DLANG_CA);
    log_verbose("  In locale = gr_EL..\n");
    doTestDisplayNames("el_GR", DLANG_EL);

    /* test that the default locale has a display name for its own language */
    errorCode=U_ZERO_ERROR;
    length=uloc_getDisplayLanguage(NULL, NULL, buffer, LENGTHOF(buffer), &errorCode);
    if(U_FAILURE(errorCode) || (length<=3 && buffer[0]<=0x7f)) {
        /* check <=3 to reject getting the language code as a display name */
        log_data_err("unable to get a display string for the language of the default locale - %s (Are you missing data?)\n", u_errorName(errorCode));
    }

    /* test that we get the language code itself for an unknown language, and a default warning */
    errorCode=U_ZERO_ERROR;
    length=uloc_getDisplayLanguage("qq", "rr", buffer, LENGTHOF(buffer), &errorCode);
    if(errorCode!=U_USING_DEFAULT_WARNING || length!=2 || buffer[0]!=0x71 || buffer[1]!=0x71) {
        log_err("error getting the display string for an unknown language - %s\n", u_errorName(errorCode));
    }
    
    /* test that we get a default warning for a display name where one component is unknown (4255) */
    errorCode=U_ZERO_ERROR;
    length=uloc_getDisplayName("qq_US_POSIX", "en_US", buffer, LENGTHOF(buffer), &errorCode);
    if(errorCode!=U_USING_DEFAULT_WARNING) {
        log_err("error getting the display name for a locale with an unknown language - %s\n", u_errorName(errorCode));
    }

    {
        int32_t i;
        static const char *aLocale = "es@collation=traditional;calendar=japanese";
        static const char *testL[] = { "en_US", 
            "fr_FR", 
            "ca_ES",
            "el_GR" };
        static const char *expect[] = { "Spanish (Calendar=Japanese Calendar, Collation=Traditional Sort Order)", /* note sorted order of keywords */
            "espagnol (Calendrier=Calendrier japonais, Ordonnancement=Ordre traditionnel)",
            "espanyol (calendari=calendari japon\\u00e8s, ordre alfab\\u00e8tic=ordre tradicional)",
            "\\u0399\\u03C3\\u03C0\\u03B1\\u03BD\\u03B9\\u03BA\\u03AC (\\u03B7\\u03BC\\u03B5\\u03C1\\u03BF\\u03BB\\u03CC\\u03B3\\u03B9\\u03BF=\\u0399\\u03B1\\u03C0\\u03C9\\u03BD\\u03B9\\u03BA\\u03CC \\u03B7\\u03BC\\u03B5\\u03C1\\u03BF\\u03BB\\u03CC\\u03B3\\u03B9\\u03BF, \\u03C4\\u03B1\\u03BA\\u03C4\\u03BF\\u03C0\\u03BF\\u03AF\\u03B7\\u03C3\\u03B7=\\u03A0\\u03B1\\u03C1\\u03B1\\u03B4\\u03BF\\u03C3\\u03B9\\u03B1\\u03BA\\u03AE \\u03C3\\u03B5\\u03B9\\u03C1\\u03AC \\u03C4\\u03B1\\u03BE\\u03B9\\u03BD\\u03CC\\u03BC\\u03B7\\u03C3\\u03B7\\u03C2)" };
        UChar *expectBuffer;

        for(i=0;i<LENGTHOF(testL);i++) {
            errorCode = U_ZERO_ERROR;
            uloc_getDisplayName(aLocale, testL[i], buffer, LENGTHOF(buffer), &errorCode);
            if(U_FAILURE(errorCode)) {
                log_err("FAIL in uloc_getDisplayName(%s,%s,..) -> %s\n", aLocale, testL[i], u_errorName(errorCode));
            } else {
                expectBuffer = CharsToUChars(expect[i]);
                if(u_strcmp(buffer,expectBuffer)) {
                    log_data_err("FAIL in uloc_getDisplayName(%s,%s,..) expected '%s' got '%s' (Are you missing data?)\n", aLocale, testL[i], expect[i], austrdup(buffer));
                } else {
                    log_verbose("pass in uloc_getDisplayName(%s,%s,..) got '%s'\n", aLocale, testL[i], expect[i]);
                }
                free(expectBuffer);
            }
        }
    }
}


/* test for uloc_getAvialable()  and uloc_countAvilable()*/
static void TestGetAvailableLocales()
{

    const char *locList;
    int32_t locCount,i;

    log_verbose("Testing the no of avialable locales\n");
    locCount=uloc_countAvailable();
    if (locCount == 0)
        log_data_err("countAvailable() returned an empty list!\n");

    /* use something sensible w/o hardcoding the count */
    else if(locCount < 0){
        log_data_err("countAvailable() returned a wrong value!= %d\n", locCount);
    }
    else{
        log_info("Number of locales returned = %d\n", locCount);
    }
    for(i=0;i<locCount;i++){
        locList=uloc_getAvailable(i);

        log_verbose(" %s\n", locList);
    }
}

/* test for u_getDataDirectory, u_setDataDirectory, uloc_getISO3Language */
static void TestDataDirectory()
{

    char            oldDirectory[512];
    const char     *temp,*testValue1,*testValue2,*testValue3;
    const char path[40] ="d:\\icu\\source\\test\\intltest" U_FILE_SEP_STRING; /*give the required path */

    log_verbose("Testing getDataDirectory()\n");
    temp = u_getDataDirectory();
    strcpy(oldDirectory, temp);

    testValue1=uloc_getISO3Language("en_US");
    log_verbose("first fetch of language retrieved  %s\n", testValue1);

    if (0 != strcmp(testValue1,"eng")){
        log_err("Initial check of ISO3 language failed: expected \"eng\", got  %s \n", testValue1);
    }

    /*defining the path for DataDirectory */
    log_verbose("Testing setDataDirectory\n");
    u_setDataDirectory( path );
    if(strcmp(path, u_getDataDirectory())==0)
        log_verbose("setDataDirectory working fine\n");
    else
        log_err("Error in setDataDirectory. Directory not set correctly - came back as [%s], expected [%s]\n", u_getDataDirectory(), path);

    testValue2=uloc_getISO3Language("en_US");
    log_verbose("second fetch of language retrieved  %s \n", testValue2);

    u_setDataDirectory(oldDirectory);
    testValue3=uloc_getISO3Language("en_US");
    log_verbose("third fetch of language retrieved  %s \n", testValue3);

    if (0 != strcmp(testValue3,"eng")) {
       log_err("get/setDataDirectory() failed: expected \"eng\", got \" %s  \" \n", testValue3);
    }
}



/*=========================================================== */

static UChar _NUL=0;

static void doTestDisplayNames(const char* displayLocale, int32_t compareIndex)
{
    UErrorCode status = U_ZERO_ERROR;
    int32_t i;
    int32_t maxresultsize;

    const char *testLocale;


    UChar  *testLang  = 0;
    UChar  *testScript  = 0;
    UChar  *testCtry = 0;
    UChar  *testVar = 0;
    UChar  *testName = 0;


    UChar*  expectedLang = 0;
    UChar*  expectedScript = 0;
    UChar*  expectedCtry = 0;
    UChar*  expectedVar = 0;
    UChar*  expectedName = 0;

setUpDataTable();

    for(i=0;i<LOCALE_SIZE; ++i)
    {
        testLocale=rawData2[NAME][i];

        log_verbose("Testing.....  %s\n", testLocale);

        maxresultsize=0;
        maxresultsize=uloc_getDisplayLanguage(testLocale, displayLocale, NULL, maxresultsize, &status);
        if(status==U_BUFFER_OVERFLOW_ERROR)
        {
            status=U_ZERO_ERROR;
            testLang=(UChar*)malloc(sizeof(UChar) * (maxresultsize+1));
            uloc_getDisplayLanguage(testLocale, displayLocale, testLang, maxresultsize + 1, &status);
        }
        else
        {
            testLang=&_NUL;
        }
        if(U_FAILURE(status)){
            log_err("Error in getDisplayLanguage()  %s\n", myErrorName(status));
        }

        maxresultsize=0;
        maxresultsize=uloc_getDisplayScript(testLocale, displayLocale, NULL, maxresultsize, &status);
        if(status==U_BUFFER_OVERFLOW_ERROR)
        {
            status=U_ZERO_ERROR;
            testScript=(UChar*)malloc(sizeof(UChar) * (maxresultsize+1));
            uloc_getDisplayScript(testLocale, displayLocale, testScript, maxresultsize + 1, &status);
        }
        else
        {
            testScript=&_NUL;
        }
        if(U_FAILURE(status)){
            log_err("Error in getDisplayScript()  %s\n", myErrorName(status));
        }

        maxresultsize=0;
        maxresultsize=uloc_getDisplayCountry(testLocale, displayLocale, NULL, maxresultsize, &status);
        if(status==U_BUFFER_OVERFLOW_ERROR)
        {
            status=U_ZERO_ERROR;
            testCtry=(UChar*)malloc(sizeof(UChar) * (maxresultsize+1));
            uloc_getDisplayCountry(testLocale, displayLocale, testCtry, maxresultsize + 1, &status);
        }
        else
        {
            testCtry=&_NUL;
        }
        if(U_FAILURE(status)){
            log_err("Error in getDisplayCountry()  %s\n", myErrorName(status));
        }

        maxresultsize=0;
        maxresultsize=uloc_getDisplayVariant(testLocale, displayLocale, NULL, maxresultsize, &status);
        if(status==U_BUFFER_OVERFLOW_ERROR)
        {
            status=U_ZERO_ERROR;
            testVar=(UChar*)malloc(sizeof(UChar) * (maxresultsize+1));
            uloc_getDisplayVariant(testLocale, displayLocale, testVar, maxresultsize + 1, &status);
        }
        else
        {
            testVar=&_NUL;
        }
        if(U_FAILURE(status)){
                log_err("Error in getDisplayVariant()  %s\n", myErrorName(status));
        }

        maxresultsize=0;
        maxresultsize=uloc_getDisplayName(testLocale, displayLocale, NULL, maxresultsize, &status);
        if(status==U_BUFFER_OVERFLOW_ERROR)
        {
            status=U_ZERO_ERROR;
            testName=(UChar*)malloc(sizeof(UChar) * (maxresultsize+1));
            uloc_getDisplayName(testLocale, displayLocale, testName, maxresultsize + 1, &status);
        }
        else
        {
            testName=&_NUL;
        }
        if(U_FAILURE(status)){
            log_err("Error in getDisplayName()  %s\n", myErrorName(status));
        }

        expectedLang=dataTable[compareIndex][i];
        if(u_strlen(expectedLang)== 0)
            expectedLang=dataTable[DLANG_EN][i];

        expectedScript=dataTable[compareIndex + 1][i];
        if(u_strlen(expectedScript)== 0)
            expectedScript=dataTable[DSCRIPT_EN][i];

        expectedCtry=dataTable[compareIndex + 2][i];
        if(u_strlen(expectedCtry)== 0)
            expectedCtry=dataTable[DCTRY_EN][i];

        expectedVar=dataTable[compareIndex + 3][i];
        if(u_strlen(expectedVar)== 0)
            expectedVar=dataTable[DVAR_EN][i];

        expectedName=dataTable[compareIndex + 4][i];
        if(u_strlen(expectedName) == 0)
            expectedName=dataTable[DNAME_EN][i];

        if (0 !=u_strcmp(testLang,expectedLang))  {
            log_data_err(" Display Language mismatch: got %s expected %s displayLocale=%s (Are you missing data?)\n", austrdup(testLang), austrdup(expectedLang), displayLocale);
        }

        if (0 != u_strcmp(testScript,expectedScript))   {
            log_data_err(" Display Script mismatch: got %s expected %s displayLocale=%s (Are you missing data?)\n", austrdup(testScript), austrdup(expectedScript), displayLocale);
        }

        if (0 != u_strcmp(testCtry,expectedCtry))   {
            log_data_err(" Display Country mismatch: got %s expected %s displayLocale=%s (Are you missing data?)\n", austrdup(testCtry), austrdup(expectedCtry), displayLocale);
        }

        if (0 != u_strcmp(testVar,expectedVar))    {
            log_data_err(" Display Variant mismatch: got %s expected %s displayLocale=%s (Are you missing data?)\n", austrdup(testVar), austrdup(expectedVar), displayLocale);
        }

        if(0 != u_strcmp(testName, expectedName))    {
            log_data_err(" Display Name mismatch: got %s expected %s displayLocale=%s (Are you missing data?)\n", austrdup(testName), austrdup(expectedName), displayLocale);
        }

        if(testName!=&_NUL) {
            free(testName);
        }
        if(testLang!=&_NUL) {
            free(testLang);
        }
        if(testScript!=&_NUL) {
            free(testScript);
        }
        if(testCtry!=&_NUL) {
            free(testCtry);
        }
        if(testVar!=&_NUL) {
            free(testVar);
        }
    }
cleanUpDataTable();
}

/* test for uloc_getISOLanguages, uloc_getISOCountries */
static void TestISOFunctions()
{
    const char* const* str=uloc_getISOLanguages();
    const char* const* str1=uloc_getISOCountries();
    const char* test;
    const char *key = NULL;
    int32_t count = 0, skipped = 0;
    int32_t expect;
    UResourceBundle *res;
    UResourceBundle *subRes;
    UErrorCode status = U_ZERO_ERROR;

    /*  test getISOLanguages*/
    /*str=uloc_getISOLanguages(); */
    log_verbose("Testing ISO Languages: \n");

    /* use structLocale - this data is no longer in root */
    res = ures_openDirect(loadTestData(&status), "structLocale", &status);
    subRes = ures_getByKey(res, "Languages", NULL, &status);
    if (U_FAILURE(status)) {
        log_data_err("There is an error in structLocale's ures_getByKey(\"Languages\"), status=%s\n", u_errorName(status));
        return;
    }

    expect = ures_getSize(subRes);
    for(count = 0; *(str+count) != 0; count++)
    {
        key = NULL;
        test = *(str+count);
        status = U_ZERO_ERROR;

        do {
            /* Skip over language tags. This API only returns language codes. */
            skipped += (key != NULL);
            ures_getNextString(subRes, NULL, &key, &status);
        }
        while (key != NULL && strchr(key, '_'));

        if(key == NULL)
            break;
        /* TODO: Consider removing sh, which is deprecated */
        if(strcmp(key,"root") == 0 || strcmp(key,"Fallback") == 0 || strcmp(key,"sh") == 0) {
            ures_getNextString(subRes, NULL, &key, &status);
            skipped++;
        }
#if U_CHARSET_FAMILY==U_ASCII_FAMILY
        /* This code only works on ASCII machines where the keys are stored in ASCII order */
        if(strcmp(test,key)) {
            /* The first difference usually implies the place where things get out of sync */
            log_err("FAIL Language diff at offset %d, \"%s\" != \"%s\"\n", count, test, key);
        }
#endif

        if(!strcmp(test,"in"))
            log_err("FAIL getISOLanguages() has obsolete language code %s\n", test);
        if(!strcmp(test,"iw"))
            log_err("FAIL getISOLanguages() has obsolete language code %s\n", test);
        if(!strcmp(test,"ji"))
            log_err("FAIL getISOLanguages() has obsolete language code %s\n", test);
        if(!strcmp(test,"jw"))
            log_err("FAIL getISOLanguages() has obsolete language code %s\n", test);
        if(!strcmp(test,"sh"))
            log_err("FAIL getISOLanguages() has obsolete language code %s\n", test);
    }

    expect -= skipped; /* Ignore the skipped resources from structLocale */

    if(count!=expect) {
        log_err("There is an error in getISOLanguages, got %d, expected %d (as per structLocale)\n", count, expect);
    }

    subRes = ures_getByKey(res, "Countries", subRes, &status);
    log_verbose("Testing ISO Countries");
    skipped = 0;
    expect = ures_getSize(subRes) - 1; /* Skip ZZ */
    for(count = 0; *(str1+count) != 0; count++)
    {
        key = NULL;
        test = *(str1+count);
        do {
            /* Skip over numeric UN tags. This API only returns ISO-3166 codes. */
            skipped += (key != NULL);
            ures_getNextString(subRes, NULL, &key, &status);
        }
        while (key != NULL && strlen(key) != 2);

        if(key == NULL)
            break;
        /* TODO: Consider removing CS, which is deprecated */
        while(strcmp(key,"QO") == 0 || strcmp(key,"QU") == 0 || strcmp(key,"CS") == 0) {
            ures_getNextString(subRes, NULL, &key, &status);
            skipped++;
        }
#if U_CHARSET_FAMILY==U_ASCII_FAMILY
        /* This code only works on ASCII machines where the keys are stored in ASCII order */
        if(strcmp(test,key)) {
            /* The first difference usually implies the place where things get out of sync */
            log_err("FAIL Country diff at offset %d, \"%s\" != \"%s\"\n", count, test, key);
        }
#endif
        if(!strcmp(test,"FX"))
            log_err("FAIL getISOCountries() has obsolete country code %s\n", test);
        if(!strcmp(test,"YU"))
            log_err("FAIL getISOCountries() has obsolete country code %s\n", test);
        if(!strcmp(test,"ZR"))
            log_err("FAIL getISOCountries() has obsolete country code %s\n", test);
    }

    ures_getNextString(subRes, NULL, &key, &status);
    if (strcmp(key, "ZZ") != 0) {
        log_err("ZZ was expected to be the last entry in structLocale, but got %s\n", key);
    }
#if U_CHARSET_FAMILY==U_EBCDIC_FAMILY
    /* On EBCDIC machines, the numbers are sorted last. Account for those in the skipped value too. */
    key = NULL;
    do {
        /* Skip over numeric UN tags. uloc_getISOCountries only returns ISO-3166 codes. */
        skipped += (key != NULL);
        ures_getNextString(subRes, NULL, &key, &status);
    }
    while (U_SUCCESS(status) && key != NULL && strlen(key) != 2);
#endif
    expect -= skipped; /* Ignore the skipped resources from structLocale */
    if(count!=expect)
    {
        log_err("There is an error in getISOCountries, got %d, expected %d \n", count, expect);
    }
    ures_close(subRes);
    ures_close(res);
}

static void setUpDataTable()
{
    int32_t i,j;
    dataTable = (UChar***)(calloc(sizeof(UChar**),LOCALE_INFO_SIZE));

    for (i = 0; i < LOCALE_INFO_SIZE; i++) {
        dataTable[i] = (UChar**)(calloc(sizeof(UChar*),LOCALE_SIZE));
        for (j = 0; j < LOCALE_SIZE; j++){
            dataTable[i][j] = CharsToUChars(rawData2[i][j]);
        }
    }
}

static void cleanUpDataTable()
{
    int32_t i,j;
    if(dataTable != NULL) {
        for (i=0; i<LOCALE_INFO_SIZE; i++) {
            for(j = 0; j < LOCALE_SIZE; j++) {
                free(dataTable[i][j]);
            }
            free(dataTable[i]);
        }
        free(dataTable);
    }
    dataTable = NULL;
}

/**
 * @bug 4011756 4011380
 */
static void TestISO3Fallback()
{
    const char* test="xx_YY";

    const char * result;

    result = uloc_getISO3Language(test);

    /* Conform to C API usage  */

    if (!result || (result[0] != 0))
       log_err("getISO3Language() on xx_YY returned %s instead of \"\"");

    result = uloc_getISO3Country(test);

    if (!result || (result[0] != 0))
        log_err("getISO3Country() on xx_YY returned %s instead of \"\"");
}

/**
 * @bug 4118587
 */
static void TestSimpleDisplayNames()
{
  /*
     This test is different from TestDisplayNames because TestDisplayNames checks
     fallback behavior, combination of language and country names to form locale
     names, and other stuff like that.  This test just checks specific language
     and country codes to make sure we have the correct names for them.
  */
    char languageCodes[] [4] = { "he", "id", "iu", "ug", "yi", "za", "419" };
    const char* languageNames [] = { "Hebrew", "Indonesian", "Inuktitut", "Uighur", "Yiddish",
                               "Zhuang", "419" };
    const char* inLocale [] = { "en_US", "zh_Hant"};
    UErrorCode status=U_ZERO_ERROR;

    int32_t i;
    int32_t localeIndex = 0;
    for (i = 0; i < 7; i++) {
        UChar *testLang=0;
        UChar *expectedLang=0;
        int size=0;
        
        if (i == 6) {
            localeIndex = 1; /* Use the second locale for the rest of the test. */
        }
        
        size=uloc_getDisplayLanguage(languageCodes[i], inLocale[localeIndex], NULL, size, &status);
        if(status==U_BUFFER_OVERFLOW_ERROR) {
            status=U_ZERO_ERROR;
            testLang=(UChar*)malloc(sizeof(UChar) * (size + 1));
            uloc_getDisplayLanguage(languageCodes[i], inLocale[localeIndex], testLang, size + 1, &status);
        }
        expectedLang=(UChar*)malloc(sizeof(UChar) * (strlen(languageNames[i])+1));
        u_uastrcpy(expectedLang, languageNames[i]);
        if (u_strcmp(testLang, expectedLang) != 0)
            log_data_err("Got wrong display name for %s : Expected \"%s\", got \"%s\".\n",
                    languageCodes[i], languageNames[i], austrdup(testLang));
        free(testLang);
        free(expectedLang);
    }

}

/**
 * @bug 4118595
 */
static void TestUninstalledISO3Names()
{
  /* This test checks to make sure getISO3Language and getISO3Country work right
     even for locales that are not installed. */
    static const char iso2Languages [][4] = {     "am", "ba", "fy", "mr", "rn",
                                        "ss", "tw", "zu" };
    static const char iso3Languages [][5] = {     "amh", "bak", "fry", "mar", "run",
                                        "ssw", "twi", "zul" };
    static const char iso2Countries [][6] = {     "am_AF", "ba_BW", "fy_KZ", "mr_MO", "rn_MN",
                                        "ss_SB", "tw_TC", "zu_ZW" };
    static const char iso3Countries [][4] = {     "AFG", "BWA", "KAZ", "MAC", "MNG",
                                        "SLB", "TCA", "ZWE" };
    int32_t i;

    for (i = 0; i < 8; i++) {
      UErrorCode err = U_ZERO_ERROR;
      const char *test;
      test = uloc_getISO3Language(iso2Languages[i]);
      if(strcmp(test, iso3Languages[i]) !=0 || U_FAILURE(err))
         log_err("Got wrong ISO3 code for %s : Expected \"%s\", got \"%s\". %s\n",
                     iso2Languages[i], iso3Languages[i], test, myErrorName(err));
    }
    for (i = 0; i < 8; i++) {
      UErrorCode err = U_ZERO_ERROR;
      const char *test;
      test = uloc_getISO3Country(iso2Countries[i]);
      if(strcmp(test, iso3Countries[i]) !=0 || U_FAILURE(err))
         log_err("Got wrong ISO3 code for %s : Expected \"%s\", got \"%s\". %s\n",
                     iso2Countries[i], iso3Countries[i], test, myErrorName(err));
    }
}


static void TestVariantParsing()
{
    static const char* en_US_custom="en_US_De Anza_Cupertino_California_United States_Earth";
    static const char* dispName="English (United States, DE ANZA_CUPERTINO_CALIFORNIA_UNITED STATES_EARTH)";
    static const char* dispVar="DE ANZA_CUPERTINO_CALIFORNIA_UNITED STATES_EARTH";
    static const char* shortVariant="fr_FR_foo";
    static const char* bogusVariant="fr_FR__foo";
    static const char* bogusVariant2="fr_FR_foo_";
    static const char* bogusVariant3="fr_FR__foo_";


    UChar displayVar[100];
    UChar displayName[100];
    UErrorCode status=U_ZERO_ERROR;
    UChar* got=0;
    int32_t size=0;
    size=uloc_getDisplayVariant(en_US_custom, "en_US", NULL, size, &status);
    if(status==U_BUFFER_OVERFLOW_ERROR) {
        status=U_ZERO_ERROR;
        got=(UChar*)realloc(got, sizeof(UChar) * (size+1));
        uloc_getDisplayVariant(en_US_custom, "en_US", got, size + 1, &status);
    }
    else {
        log_err("FAIL: Didn't get U_BUFFER_OVERFLOW_ERROR\n");
    }
    u_uastrcpy(displayVar, dispVar);
    if(u_strcmp(got,displayVar)!=0) {
        log_err("FAIL: getDisplayVariant() Wanted %s, got %s\n", dispVar, austrdup(got));
    }
    size=0;
    size=uloc_getDisplayName(en_US_custom, "en_US", NULL, size, &status);
    if(status==U_BUFFER_OVERFLOW_ERROR) {
        status=U_ZERO_ERROR;
        got=(UChar*)realloc(got, sizeof(UChar) * (size+1));
        uloc_getDisplayName(en_US_custom, "en_US", got, size + 1, &status);
    }
    else {
        log_err("FAIL: Didn't get U_BUFFER_OVERFLOW_ERROR\n");
    }
    u_uastrcpy(displayName, dispName);
    if(u_strcmp(got,displayName)!=0) {
        if (status == U_USING_DEFAULT_WARNING) {
            log_data_err("FAIL: getDisplayName() got %s. Perhaps you are missing data?\n", u_errorName(status));
        } else {
            log_err("FAIL: getDisplayName() Wanted %s, got %s\n", dispName, austrdup(got));
        }
    }

    size=0;
    status=U_ZERO_ERROR;
    size=uloc_getDisplayVariant(shortVariant, NULL, NULL, size, &status);
    if(status==U_BUFFER_OVERFLOW_ERROR) {
        status=U_ZERO_ERROR;
        got=(UChar*)realloc(got, sizeof(UChar) * (size+1));
        uloc_getDisplayVariant(shortVariant, NULL, got, size + 1, &status);
    }
    else {
        log_err("FAIL: Didn't get U_BUFFER_OVERFLOW_ERROR\n");
    }
    if(strcmp(austrdup(got),"FOO")!=0) {
        log_err("FAIL: getDisplayVariant()  Wanted: foo  Got: %s\n", austrdup(got));
    }
    size=0;
    status=U_ZERO_ERROR;
    size=uloc_getDisplayVariant(bogusVariant, NULL, NULL, size, &status);
    if(status==U_BUFFER_OVERFLOW_ERROR) {
        status=U_ZERO_ERROR;
        got=(UChar*)realloc(got, sizeof(UChar) * (size+1));
        uloc_getDisplayVariant(bogusVariant, NULL, got, size + 1, &status);
    }
    else {
        log_err("FAIL: Didn't get U_BUFFER_OVERFLOW_ERROR\n");
    }
    if(strcmp(austrdup(got),"_FOO")!=0) {
        log_err("FAIL: getDisplayVariant()  Wanted: _FOO  Got: %s\n", austrdup(got));
    }
    size=0;
    status=U_ZERO_ERROR;
    size=uloc_getDisplayVariant(bogusVariant2, NULL, NULL, size, &status);
    if(status==U_BUFFER_OVERFLOW_ERROR) {
        status=U_ZERO_ERROR;
        got=(UChar*)realloc(got, sizeof(UChar) * (size+1));
        uloc_getDisplayVariant(bogusVariant2, NULL, got, size + 1, &status);
    }
    else {
        log_err("FAIL: Didn't get U_BUFFER_OVERFLOW_ERROR\n");
    }
    if(strcmp(austrdup(got),"FOO_")!=0) {
        log_err("FAIL: getDisplayVariant()  Wanted: FOO_  Got: %s\n", austrdup(got));
    }
    size=0;
    status=U_ZERO_ERROR;
    size=uloc_getDisplayVariant(bogusVariant3, NULL, NULL, size, &status);
    if(status==U_BUFFER_OVERFLOW_ERROR) {
        status=U_ZERO_ERROR;
        got=(UChar*)realloc(got, sizeof(UChar) * (size+1));
        uloc_getDisplayVariant(bogusVariant3, NULL, got, size + 1, &status);
    }
    else {
        log_err("FAIL: Didn't get U_BUFFER_OVERFLOW_ERROR\n");
    }
    if(strcmp(austrdup(got),"_FOO_")!=0) {
        log_err("FAIL: getDisplayVariant()  Wanted: _FOO_  Got: %s\n", austrdup(got));
    }
    free(got);
}


static void TestObsoleteNames(void)
{
    int32_t i;
    UErrorCode status = U_ZERO_ERROR;
    char buff[256];

    static const struct
    {
        char locale[9];
        char lang3[4];
        char lang[4];
        char ctry3[4];
        char ctry[4];
    } tests[] =
    {
        { "eng_USA", "eng", "en", "USA", "US" },
        { "kok",  "kok", "kok", "", "" },
        { "in",  "ind", "in", "", "" },
        { "id",  "ind", "id", "", "" }, /* NO aliasing */
        { "sh",  "srp", "sh", "", "" },
        { "zz_CS",  "", "zz", "SCG", "CS" },
        { "zz_FX",  "", "zz", "FXX", "FX" },
        { "zz_RO",  "", "zz", "ROU", "RO" },
        { "zz_TP",  "", "zz", "TMP", "TP" },
        { "zz_TL",  "", "zz", "TLS", "TL" },
        { "zz_ZR",  "", "zz", "ZAR", "ZR" },
        { "zz_FXX",  "", "zz", "FXX", "FX" }, /* no aliasing. Doesn't go to PS(PSE). */
        { "zz_ROM",  "", "zz", "ROU", "RO" },
        { "zz_ROU",  "", "zz", "ROU", "RO" },
        { "zz_ZAR",  "", "zz", "ZAR", "ZR" },
        { "zz_TMP",  "", "zz", "TMP", "TP" },
        { "zz_TLS",  "", "zz", "TLS", "TL" },
        { "zz_YUG",  "", "zz", "YUG", "YU" },
        { "mlt_PSE", "mlt", "mt", "PSE", "PS" },
        { "iw", "heb", "iw", "", "" },
        { "ji", "yid", "ji", "", "" },
        { "jw", "jaw", "jw", "", "" },
        { "sh", "srp", "sh", "", "" },
        { "", "", "", "", "" }
    };

    for(i=0;tests[i].locale[0];i++)
    {
        const char *locale;

        locale = tests[i].locale;
        log_verbose("** %s:\n", locale);

        status = U_ZERO_ERROR;
        if(strcmp(tests[i].lang3,uloc_getISO3Language(locale)))
        {
            log_err("FAIL: uloc_getISO3Language(%s)==\t\"%s\",\t expected \"%s\"\n",
                locale,  uloc_getISO3Language(locale), tests[i].lang3);
        }
        else
        {
            log_verbose("   uloc_getISO3Language()==\t\"%s\"\n",
                uloc_getISO3Language(locale) );
        }

        status = U_ZERO_ERROR;
        uloc_getLanguage(locale, buff, 256, &status);
        if(U_FAILURE(status))
        {
            log_err("FAIL: error getting language from %s\n", locale);
        }
        else
        {
            if(strcmp(buff,tests[i].lang))
            {
                log_err("FAIL: uloc_getLanguage(%s)==\t\"%s\"\t expected \"%s\"\n",
                    locale, buff, tests[i].lang);
            }
            else
            {
                log_verbose("  uloc_getLanguage(%s)==\t%s\n", locale, buff);
            }
        }
        if(strcmp(tests[i].lang3,uloc_getISO3Language(locale)))
        {
            log_err("FAIL: uloc_getISO3Language(%s)==\t\"%s\",\t expected \"%s\"\n",
                locale,  uloc_getISO3Language(locale), tests[i].lang3);
        }
        else
        {
            log_verbose("   uloc_getISO3Language()==\t\"%s\"\n",
                uloc_getISO3Language(locale) );
        }

        if(strcmp(tests[i].ctry3,uloc_getISO3Country(locale)))
        {
            log_err("FAIL: uloc_getISO3Country(%s)==\t\"%s\",\t expected \"%s\"\n",
                locale,  uloc_getISO3Country(locale), tests[i].ctry3);
        }
        else
        {
            log_verbose("   uloc_getISO3Country()==\t\"%s\"\n",
                uloc_getISO3Country(locale) );
        }

        status = U_ZERO_ERROR;
        uloc_getCountry(locale, buff, 256, &status);
        if(U_FAILURE(status))
        {
            log_err("FAIL: error getting country from %s\n", locale);
        }
        else
        {
            if(strcmp(buff,tests[i].ctry))
            {
                log_err("FAIL: uloc_getCountry(%s)==\t\"%s\"\t expected \"%s\"\n",
                    locale, buff, tests[i].ctry);
            }
            else
            {
                log_verbose("  uloc_getCountry(%s)==\t%s\n", locale, buff);
            }
        }
    }

    if (uloc_getLCID("iw_IL") != uloc_getLCID("he_IL")) {
        log_err("he,iw LCID mismatch: %X versus %X\n", uloc_getLCID("iw_IL"), uloc_getLCID("he_IL"));
    }

    if (uloc_getLCID("iw") != uloc_getLCID("he")) {
        log_err("he,iw LCID mismatch: %X versus %X\n", uloc_getLCID("iw"), uloc_getLCID("he"));
    }

#if 0

    i = uloc_getLanguage("kok",NULL,0,&icu_err);
    if(U_FAILURE(icu_err))
    {
        log_err("FAIL: Got %s trying to do uloc_getLanguage(kok)\n", u_errorName(icu_err));
    }

    icu_err = U_ZERO_ERROR;
    uloc_getLanguage("kok",r1_buff,12,&icu_err);
    if(U_FAILURE(icu_err))
    {
        log_err("FAIL: Got %s trying to do uloc_getLanguage(kok, buff)\n", u_errorName(icu_err));
    }

    r1_addr = (char *)uloc_getISO3Language("kok");

    icu_err = U_ZERO_ERROR;
    if (strcmp(r1_buff,"kok") != 0)
    {
        log_err("FAIL: uloc_getLanguage(kok)==%s not kok\n",r1_buff);
        line--;
    }
    r1_addr = (char *)uloc_getISO3Language("in");
    i = uloc_getLanguage(r1_addr,r1_buff,12,&icu_err);
    if (strcmp(r1_buff,"id") != 0)
    {
        printf("uloc_getLanguage error (%s)\n",r1_buff);
        line--;
    }
    r1_addr = (char *)uloc_getISO3Language("sh");
    i = uloc_getLanguage(r1_addr,r1_buff,12,&icu_err);
    if (strcmp(r1_buff,"sr") != 0)
    {
        printf("uloc_getLanguage error (%s)\n",r1_buff);
        line--;
    }

    r1_addr = (char *)uloc_getISO3Country("zz_ZR");
    strcpy(p1_buff,"zz_");
    strcat(p1_buff,r1_addr);
    i = uloc_getCountry(p1_buff,r1_buff,12,&icu_err);
    if (strcmp(r1_buff,"ZR") != 0)
    {
        printf("uloc_getCountry error (%s)\n",r1_buff);
        line--;
    }
    r1_addr = (char *)uloc_getISO3Country("zz_FX");
    strcpy(p1_buff,"zz_");
    strcat(p1_buff,r1_addr);
    i = uloc_getCountry(p1_buff,r1_buff,12,&icu_err);
    if (strcmp(r1_buff,"FX") != 0)
    {
        printf("uloc_getCountry error (%s)\n",r1_buff);
        line--;
    }

#endif

}

static void TestKeywordVariants(void) 
{
    static const struct {
        const char *localeID;
        const char *expectedLocaleID;
        const char *expectedLocaleIDNoKeywords;
        const char *expectedCanonicalID;
        const char *expectedKeywords[10];
        int32_t numKeywords;
        UErrorCode expectedStatus; /* from uloc_openKeywords */
    } testCases[] = {
        {
            "de_DE@  currency = euro; C o ll A t i o n   = Phonebook   ; C alen dar = buddhist   ", 
            "de_DE@calendar=buddhist;collation=Phonebook;currency=euro", 
            "de_DE",
            "de_DE@calendar=buddhist;collation=Phonebook;currency=euro", 
            {"calendar", "collation", "currency"},
            3,
            U_ZERO_ERROR
        },
        {
            "de_DE@euro",
            "de_DE@euro",
            "de_DE",
            "de_DE@currency=EUR",
            {"","","","","","",""},
            0,
            U_INVALID_FORMAT_ERROR /* must have '=' after '@' */
        },
        {
            "de_DE@euro;collation=phonebook",
            "de_DE", /* error result; bad format */
            "de_DE", /* error result; bad format */
            "de_DE", /* error result; bad format */
            {"","","","","","",""},
            0,
            U_INVALID_FORMAT_ERROR
        }
    };
    UErrorCode status = U_ZERO_ERROR;
    
    int32_t i = 0, j = 0;
    int32_t resultLen = 0;
    char buffer[256];
    UEnumeration *keywords;
    int32_t keyCount = 0;
    const char *keyword = NULL;
    int32_t keywordLen = 0;
    
    for(i = 0; i < sizeof(testCases)/sizeof(testCases[0]); i++) {
        status = U_ZERO_ERROR;
        *buffer = 0;
        keywords = uloc_openKeywords(testCases[i].localeID, &status);
        
        if(status != testCases[i].expectedStatus) {
            log_err("Expected to uloc_openKeywords(\"%s\") => status %s. Got %s instead\n", 
                    testCases[i].localeID,
                    u_errorName(testCases[i].expectedStatus), u_errorName(status));
        }
        status = U_ZERO_ERROR;
        if(keywords) {
            if((keyCount = uenum_count(keywords, &status)) != testCases[i].numKeywords) {
                log_err("Expected to get %i keywords, got %i\n", testCases[i].numKeywords, keyCount);
            }
            if(keyCount) {
                j = 0;
                while((keyword = uenum_next(keywords, &keywordLen, &status))) {
                    if(strcmp(keyword, testCases[i].expectedKeywords[j]) != 0) {
                        log_err("Expected to get keyword value %s, got %s\n", testCases[i].expectedKeywords[j], keyword);
                    }
                    j++;
                }
                j = 0;
                uenum_reset(keywords, &status);
                while((keyword = uenum_next(keywords, &keywordLen, &status))) {
                    if(strcmp(keyword, testCases[i].expectedKeywords[j]) != 0) {
                        log_err("Expected to get keyword value %s, got %s\n", testCases[i].expectedKeywords[j], keyword);
                    }
                    j++;
                }
            }
            uenum_close(keywords);
        }
        resultLen = uloc_getName(testCases[i].localeID, buffer, 256, &status);
        if (uprv_strcmp(testCases[i].expectedLocaleID, buffer) != 0) {
            log_err("Expected uloc_getName(\"%s\") => \"%s\"; got \"%s\"\n",
                    testCases[i].localeID, testCases[i].expectedLocaleID, buffer);
        }
        resultLen = uloc_canonicalize(testCases[i].localeID, buffer, 256, &status);
        if (uprv_strcmp(testCases[i].expectedCanonicalID, buffer) != 0) {
            log_err("Expected uloc_canonicalize(\"%s\") => \"%s\"; got \"%s\"\n",
                    testCases[i].localeID, testCases[i].expectedCanonicalID, buffer);
        }        
    }
    
}

static void TestKeywordVariantParsing(void) 
{
    static const struct {
        const char *localeID;
        const char *keyword;
        const char *expectedValue;
    } testCases[] = {
        { "de_DE@  C o ll A t i o n   = Phonebook   ", "c o ll a t i o n", "Phonebook" },
        { "de_DE", "collation", ""},
        { "de_DE@collation=PHONEBOOK", "collation", "PHONEBOOK" },
        { "de_DE@currency = euro; CoLLaTion   = PHONEBOOk", "collatiON", "PHONEBOOk" },
    };
    
    UErrorCode status = U_ZERO_ERROR;
    
    int32_t i = 0;
    int32_t resultLen = 0;
    char buffer[256];
    
    for(i = 0; i < sizeof(testCases)/sizeof(testCases[0]); i++) {
        *buffer = 0;
        resultLen = uloc_getKeywordValue(testCases[i].localeID, testCases[i].keyword, buffer, 256, &status);
        if(uprv_strcmp(testCases[i].expectedValue, buffer) != 0) {
            log_err("Expected to extract \"%s\" from \"%s\" for keyword \"%s\". Got \"%s\" instead\n",
                testCases[i].expectedValue, testCases[i].localeID, testCases[i].keyword, buffer);
        }
    }
}

static const struct {
  const char *l; /* locale */
  const char *k; /* kw */
  const char *v; /* value */
  const char *x; /* expected */
} kwSetTestCases[] = {
#if 1
  { "en_US", "calendar", "japanese", "en_US@calendar=japanese" },
  { "en_US@", "calendar", "japanese", "en_US@calendar=japanese" },
  { "en_US@calendar=islamic", "calendar", "japanese", "en_US@calendar=japanese" },
  { "en_US@calendar=slovakian", "calendar", "gregorian", "en_US@calendar=gregorian" }, /* don't know what this means, but it has the same # of chars as gregorian */
  { "en_US@calendar=gregorian", "calendar", "japanese", "en_US@calendar=japanese" },
  { "de", "Currency", "CHF", "de@currency=CHF" },
  { "de", "Currency", "CHF", "de@currency=CHF" },

  { "en_US@collation=phonebook", "calendar", "japanese", "en_US@calendar=japanese;collation=phonebook" },
  { "en_US@calendar=japanese", "collation", "phonebook", "en_US@calendar=japanese;collation=phonebook" },
  { "de@collation=phonebook", "Currency", "CHF", "de@collation=phonebook;currency=CHF" },
  { "en_US@calendar=gregorian;collation=phonebook", "calendar", "japanese", "en_US@calendar=japanese;collation=phonebook" },
  { "en_US@calendar=slovakian;collation=phonebook", "calendar", "gregorian", "en_US@calendar=gregorian;collation=phonebook" }, /* don't know what this means, but it has the same # of chars as gregorian */
  { "en_US@calendar=slovakian;collation=videobook", "collation", "phonebook", "en_US@calendar=slovakian;collation=phonebook" }, /* don't know what this means, but it has the same # of chars as gregorian */
  { "en_US@calendar=islamic;collation=phonebook", "calendar", "japanese", "en_US@calendar=japanese;collation=phonebook" },
  { "de@collation=phonebook", "Currency", "CHF", "de@collation=phonebook;currency=CHF" },
#endif
#if 1
  { "mt@a=0;b=1;c=2;d=3", "c","j", "mt@a=0;b=1;c=j;d=3" },
  { "mt@a=0;b=1;c=2;d=3", "x","j", "mt@a=0;b=1;c=2;d=3;x=j" },
  { "mt@a=0;b=1;c=2;d=3", "a","f", "mt@a=f;b=1;c=2;d=3" },
  { "mt@a=0;aa=1;aaa=3", "a","x", "mt@a=x;aa=1;aaa=3" },
  { "mt@a=0;aa=1;aaa=3", "aa","x", "mt@a=0;aa=x;aaa=3" },
  { "mt@a=0;aa=1;aaa=3", "aaa","x", "mt@a=0;aa=1;aaa=x" },
  { "mt@a=0;aa=1;aaa=3", "a","yy", "mt@a=yy;aa=1;aaa=3" },
  { "mt@a=0;aa=1;aaa=3", "aa","yy", "mt@a=0;aa=yy;aaa=3" },
  { "mt@a=0;aa=1;aaa=3", "aaa","yy", "mt@a=0;aa=1;aaa=yy" },
#endif
#if 1
  /* removal tests */
  /* 1. removal of item at end */
  { "de@collation=phonebook;currency=CHF", "currency",   "", "de@collation=phonebook" },
  { "de@collation=phonebook;currency=CHF", "currency", NULL, "de@collation=phonebook" },
  /* 2. removal of item at beginning */
  { "de@collation=phonebook;currency=CHF", "collation", "", "de@currency=CHF" },
  { "de@collation=phonebook;currency=CHF", "collation", NULL, "de@currency=CHF" },
  /* 3. removal of an item not there */
  { "de@collation=phonebook;currency=CHF", "calendar", NULL, "de@collation=phonebook;currency=CHF" },
  /* 4. removal of only item */
  { "de@collation=phonebook", "collation", NULL, "de" },
#endif
  { "de@collation=phonebook", "Currency", "CHF", "de@collation=phonebook;currency=CHF" }
};


static void TestKeywordSet(void)
{
    int32_t i = 0;
    int32_t resultLen = 0;
    char buffer[1024];

    char cbuffer[1024];

    for(i = 0; i < sizeof(kwSetTestCases)/sizeof(kwSetTestCases[0]); i++) {
        UErrorCode status = U_ZERO_ERROR;
        memset(buffer,'%',1023);
        strcpy(buffer, kwSetTestCases[i].l);

        uloc_canonicalize(kwSetTestCases[i].l, cbuffer, 1023, &status);
        if(strcmp(buffer,cbuffer)) {
          log_verbose("note: [%d] wasn't canonical, should be: '%s' not '%s'. Won't check for canonicity in output.\n", i, cbuffer, buffer);
        }
          /* sanity check test case results for canonicity */
        uloc_canonicalize(kwSetTestCases[i].x, cbuffer, 1023, &status);
        if(strcmp(kwSetTestCases[i].x,cbuffer)) {
          log_err("%s:%d: ERROR: kwSetTestCases[%d].x = '%s', should be %s (must be canonical)\n", __FILE__, __LINE__, i, kwSetTestCases[i].x, cbuffer);
        }

        resultLen = uloc_setKeywordValue(kwSetTestCases[i].k, kwSetTestCases[i].v, buffer, 1023, &status);
        if(U_FAILURE(status)) {
          log_err("Err on test case %d: got error %s\n", i, u_errorName(status));
          continue;
        }
        if(strcmp(buffer,kwSetTestCases[i].x) || ((int32_t)strlen(buffer)!=resultLen)) {
          log_err("FAIL: #%d: %s + [%s=%s] -> %s (%d) expected %s (%d)\n", i, kwSetTestCases[i].l, kwSetTestCases[i].k,
                  kwSetTestCases[i].v, buffer, resultLen, kwSetTestCases[i].x, strlen(buffer));
        } else {
          log_verbose("pass: #%d: %s + [%s=%s] -> %s\n", i, kwSetTestCases[i].l, kwSetTestCases[i].k, kwSetTestCases[i].v,buffer);
        }
    }
}

static void TestKeywordSetError(void)
{
    char buffer[1024];
    UErrorCode status;
    int32_t res;
    int32_t i;
    int32_t blen;

    /* 0-test whether an error condition modifies the buffer at all */
    blen=0;
    i=0;
    memset(buffer,'%',1023);
    status = U_ZERO_ERROR;
    res = uloc_setKeywordValue(kwSetTestCases[i].k, kwSetTestCases[i].v, buffer, blen, &status);
    if(status != U_ILLEGAL_ARGUMENT_ERROR) {
        log_err("expected illegal err got %s\n", u_errorName(status));
        return;
    }
    /*  if(res!=strlen(kwSetTestCases[i].x)) {
    log_err("expected result %d got %d\n", strlen(kwSetTestCases[i].x), res);
    return;
    } */
    if(buffer[blen]!='%') {
        log_err("Buffer byte %d was modified: now %c\n", blen, buffer[blen]);
        return;
    }
    log_verbose("0-buffer modify OK\n");

    for(i=0;i<=2;i++) {
        /* 1- test a short buffer with growing text */
        blen=(int32_t)strlen(kwSetTestCases[i].l)+1;
        memset(buffer,'%',1023);
        strcpy(buffer,kwSetTestCases[i].l);
        status = U_ZERO_ERROR;
        res = uloc_setKeywordValue(kwSetTestCases[i].k, kwSetTestCases[i].v, buffer, blen, &status);
        if(status != U_BUFFER_OVERFLOW_ERROR) {
            log_err("expected buffer overflow on buffer %d got %s, len %d (%s + [%s=%s])\n", blen, u_errorName(status), res, kwSetTestCases[i].l, kwSetTestCases[i].k, kwSetTestCases[i].v);
            return;
        }
        if(res!=(int32_t)strlen(kwSetTestCases[i].x)) {
            log_err("expected result %d got %d\n", strlen(kwSetTestCases[i].x), res);
            return;
        }
        if(buffer[blen]!='%') {
            log_err("Buffer byte %d was modified: now %c\n", blen, buffer[blen]);
            return;
        }
        log_verbose("1/%d-buffer modify OK\n",i);
    }

    for(i=3;i<=4;i++) {
        /* 2- test a short buffer - text the same size or shrinking   */
        blen=(int32_t)strlen(kwSetTestCases[i].l)+1;
        memset(buffer,'%',1023);
        strcpy(buffer,kwSetTestCases[i].l);
        status = U_ZERO_ERROR;
        res = uloc_setKeywordValue(kwSetTestCases[i].k, kwSetTestCases[i].v, buffer, blen, &status);
        if(status != U_ZERO_ERROR) {
            log_err("expected zero error got %s\n", u_errorName(status));
            return;
        }
        if(buffer[blen+1]!='%') {
            log_err("Buffer byte %d was modified: now %c\n", blen+1, buffer[blen+1]);
            return;
        }
        if(res!=(int32_t)strlen(kwSetTestCases[i].x)) {
            log_err("expected result %d got %d\n", strlen(kwSetTestCases[i].x), res);
            return;
        }
        if(strcmp(buffer,kwSetTestCases[i].x) || ((int32_t)strlen(buffer)!=res)) {
            log_err("FAIL: #%d: %s + [%s=%s] -> %s (%d) expected %s (%d)\n", i, kwSetTestCases[i].l, kwSetTestCases[i].k,
                kwSetTestCases[i].v, buffer, res, kwSetTestCases[i].x, strlen(buffer));
        } else {
            log_verbose("pass: #%d: %s + [%s=%s] -> %s\n", i, kwSetTestCases[i].l, kwSetTestCases[i].k, kwSetTestCases[i].v,
                buffer);
        }
        log_verbose("2/%d-buffer modify OK\n",i);
    }
}

static int32_t _canonicalize(int32_t selector, /* 0==getName, 1==canonicalize */
                             const char* localeID,
                             char* result,
                             int32_t resultCapacity,
                             UErrorCode* ec) {
    /* YOU can change this to use function pointers if you like */
    switch (selector) {
    case 0:
        return uloc_getName(localeID, result, resultCapacity, ec);
    case 1:
        return uloc_canonicalize(localeID, result, resultCapacity, ec);
    default:
        return -1;
    }
}

static void TestCanonicalization(void)
{
    static const struct {
        const char *localeID;    /* input */
        const char *getNameID;   /* expected getName() result */
        const char *canonicalID; /* expected canonicalize() result */
    } testCases[] = {
        { "ca_ES_PREEURO-with-extra-stuff-that really doesn't make any sense-unless-you're trying to increase code coverage",
          "ca_ES_PREEURO_WITH_EXTRA_STUFF_THAT REALLY DOESN'T MAKE ANY SENSE_UNLESS_YOU'RE TRYING TO INCREASE CODE COVERAGE",
          "ca_ES_PREEURO_WITH_EXTRA_STUFF_THAT REALLY DOESN'T MAKE ANY SENSE_UNLESS_YOU'RE TRYING TO INCREASE CODE COVERAGE"},
        { "ca_ES_PREEURO", "ca_ES_PREEURO", "ca_ES@currency=ESP" },
        { "de_AT_PREEURO", "de_AT_PREEURO", "de_AT@currency=ATS" },
        { "de_DE_PREEURO", "de_DE_PREEURO", "de_DE@currency=DEM" },
        { "de_LU_PREEURO", "de_LU_PREEURO", "de_LU@currency=LUF" },
        { "el_GR_PREEURO", "el_GR_PREEURO", "el_GR@currency=GRD" },
        { "en_BE_PREEURO", "en_BE_PREEURO", "en_BE@currency=BEF" },
        { "en_IE_PREEURO", "en_IE_PREEURO", "en_IE@currency=IEP" },
        { "es_ES_PREEURO", "es_ES_PREEURO", "es_ES@currency=ESP" },
        { "eu_ES_PREEURO", "eu_ES_PREEURO", "eu_ES@currency=ESP" },
        { "fi_FI_PREEURO", "fi_FI_PREEURO", "fi_FI@currency=FIM" },
        { "fr_BE_PREEURO", "fr_BE_PREEURO", "fr_BE@currency=BEF" },
        { "fr_FR_PREEURO", "fr_FR_PREEURO", "fr_FR@currency=FRF" },
        { "fr_LU_PREEURO", "fr_LU_PREEURO", "fr_LU@currency=LUF" },
        { "ga_IE_PREEURO", "ga_IE_PREEURO", "ga_IE@currency=IEP" },
        { "gl_ES_PREEURO", "gl_ES_PREEURO", "gl_ES@currency=ESP" },
        { "it_IT_PREEURO", "it_IT_PREEURO", "it_IT@currency=ITL" },
        { "nl_BE_PREEURO", "nl_BE_PREEURO", "nl_BE@currency=BEF" },
        { "nl_NL_PREEURO", "nl_NL_PREEURO", "nl_NL@currency=NLG" },
        { "pt_PT_PREEURO", "pt_PT_PREEURO", "pt_PT@currency=PTE" },
        { "de__PHONEBOOK", "de__PHONEBOOK", "de@collation=phonebook" },
        { "en_GB_EURO", "en_GB_EURO", "en_GB@currency=EUR" },
        { "en_GB@EURO", "en_GB@EURO", "en_GB@currency=EUR" }, /* POSIX ID */
        { "es__TRADITIONAL", "es__TRADITIONAL", "es@collation=traditional" },
        { "hi__DIRECT", "hi__DIRECT", "hi@collation=direct" },
        { "ja_JP_TRADITIONAL", "ja_JP_TRADITIONAL", "ja_JP@calendar=japanese" },
        { "th_TH_TRADITIONAL", "th_TH_TRADITIONAL", "th_TH@calendar=buddhist" },
        { "zh_TW_STROKE", "zh_TW_STROKE", "zh_TW@collation=stroke" },
        { "zh__PINYIN", "zh__PINYIN", "zh@collation=pinyin" },
        { "zh@collation=pinyin", "zh@collation=pinyin", "zh@collation=pinyin" },
        { "zh_CN@collation=pinyin", "zh_CN@collation=pinyin", "zh_CN@collation=pinyin" },
        { "zh_CN_STROKE", "zh_CN_STROKE", "zh_CN@collation=stroke" },
        { "zh_CN_CA@collation=pinyin", "zh_CN_CA@collation=pinyin", "zh_CN_CA@collation=pinyin" },
        { "en_US_POSIX", "en_US_POSIX", "en_US_POSIX" }, 
        { "hy_AM_REVISED", "hy_AM_REVISED", "hy_AM_REVISED" }, 
        { "no_NO_NY", "no_NO_NY", "no_NO_NY" /* not: "nn_NO" [alan ICU3.0] */ },
        { "no@ny", "no@ny", "no__NY" /* not: "nn" [alan ICU3.0] */ }, /* POSIX ID */
        { "no-no.utf32@B", "no_NO.utf32@B", "no_NO_B" /* not: "nb_NO_B" [alan ICU3.0] */ }, /* POSIX ID */
        { "qz-qz@Euro", "qz_QZ@Euro", "qz_QZ@currency=EUR" }, /* qz-qz uses private use iso codes */
        { "en-BOONT", "en_BOONT", "en__BOONT" }, /* registered name */
        { "de-1901", "de_1901", "de__1901" }, /* registered name */
        { "de-1906", "de_1906", "de__1906" }, /* registered name */
        { "sr-SP-Cyrl", "sr_SP_CYRL", "sr_Cyrl_RS" }, /* .NET name */
        { "sr-SP-Latn", "sr_SP_LATN", "sr_Latn_RS" }, /* .NET name */
        { "sr_YU_CYRILLIC", "sr_YU_CYRILLIC", "sr_Cyrl_RS" }, /* Linux name */
        { "uz-UZ-Cyrl", "uz_UZ_CYRL", "uz_Cyrl_UZ" }, /* .NET name */
        { "uz-UZ-Latn", "uz_UZ_LATN", "uz_Latn_UZ" }, /* .NET name */
        { "zh-CHS", "zh_CHS", "zh_Hans" }, /* .NET name */
        { "zh-CHT", "zh_CHT", "zh_Hant" }, /* .NET name This may change back to zh_Hant */

        /* posix behavior that used to be performed by getName */
        { "mr.utf8", "mr.utf8", "mr" },
        { "de-tv.koi8r", "de_TV.koi8r", "de_TV" },
        { "x-piglatin_ML.MBE", "x-piglatin_ML.MBE", "x-piglatin_ML" },
        { "i-cherokee_US.utf7", "i-cherokee_US.utf7", "i-cherokee_US" },
        { "x-filfli_MT_FILFLA.gb-18030", "x-filfli_MT_FILFLA.gb-18030", "x-filfli_MT_FILFLA" },
        { "no-no-ny.utf8@B", "no_NO_NY.utf8@B", "no_NO_NY_B" /* not: "nn_NO" [alan ICU3.0] */ }, /* @ ignored unless variant is empty */

        /* fleshing out canonicalization */
        /* trim space and sort keywords, ';' is separator so not present at end in canonical form */
        { "en_Hant_IL_VALLEY_GIRL@ currency = EUR; calendar = Japanese ;", "en_Hant_IL_VALLEY_GIRL@calendar=Japanese;currency=EUR", "en_Hant_IL_VALLEY_GIRL@calendar=Japanese;currency=EUR" },
        /* already-canonical ids are not changed */
        { "en_Hant_IL_VALLEY_GIRL@calendar=Japanese;currency=EUR", "en_Hant_IL_VALLEY_GIRL@calendar=Japanese;currency=EUR", "en_Hant_IL_VALLEY_GIRL@calendar=Japanese;currency=EUR" },
        /* PRE_EURO and EURO conversions don't affect other keywords */
        { "es_ES_PREEURO@CALendar=Japanese", "es_ES_PREEURO@calendar=Japanese", "es_ES@calendar=Japanese;currency=ESP" },
        { "es_ES_EURO@SHOUT=zipeedeedoodah", "es_ES_EURO@shout=zipeedeedoodah", "es_ES@currency=EUR;shout=zipeedeedoodah" },
        /* currency keyword overrides PRE_EURO and EURO currency */
        { "es_ES_PREEURO@currency=EUR", "es_ES_PREEURO@currency=EUR", "es_ES@currency=EUR" },
        { "es_ES_EURO@currency=ESP", "es_ES_EURO@currency=ESP", "es_ES@currency=ESP" },
        /* norwegian is just too weird, if we handle things in their full generality */
        { "no-Hant-GB_NY@currency=$$$", "no_Hant_GB_NY@currency=$$$", "no_Hant_GB_NY@currency=$$$" /* not: "nn_Hant_GB@currency=$$$" [alan ICU3.0] */ },

        /* test cases reflecting internal resource bundle usage */
        { "root@kw=foo", "root@kw=foo", "root@kw=foo" },
        { "@calendar=gregorian", "@calendar=gregorian", "@calendar=gregorian" },
        { "ja_JP@calendar=Japanese", "ja_JP@calendar=Japanese", "ja_JP@calendar=Japanese" },
        { "ja_JP", "ja_JP", "ja_JP" },

        /* test case for "i-default" */
        { "i-default", NULL, NULL }
    };
    
    static const char* label[] = { "getName", "canonicalize" };

    UErrorCode status = U_ZERO_ERROR;
    int32_t i, j, resultLen = 0, origResultLen;
    char buffer[256];
    
    for (i=0; i < sizeof(testCases)/sizeof(testCases[0]); i++) {
        for (j=0; j<2; ++j) {
            const char* expected = (j==0) ? testCases[i].getNameID : testCases[i].canonicalID;
            *buffer = 0;
            status = U_ZERO_ERROR;

            if (expected == NULL) {
                expected = uloc_getDefault();
            }

            /* log_verbose("testing %s -> %s\n", testCases[i], testCases[i].canonicalID); */
            origResultLen = _canonicalize(j, testCases[i].localeID, NULL, 0, &status);
            if (status != U_BUFFER_OVERFLOW_ERROR) {
                log_err("FAIL: uloc_%s(%s) => %s, expected U_BUFFER_OVERFLOW_ERROR\n",
                        label[j], testCases[i].localeID, u_errorName(status));
                continue;
            }
            status = U_ZERO_ERROR;
            resultLen = _canonicalize(j, testCases[i].localeID, buffer, sizeof(buffer), &status);
            if (U_FAILURE(status)) {
                log_err("FAIL: uloc_%s(%s) => %s, expected U_ZERO_ERROR\n",
                        label[j], testCases[i].localeID, u_errorName(status));
                continue;
            }
            if(uprv_strcmp(expected, buffer) != 0) {
                log_err("FAIL: uloc_%s(%s) => \"%s\", expected \"%s\"\n",
                        label[j], testCases[i].localeID, buffer, expected);
            } else {
                log_verbose("Ok: uloc_%s(%s) => \"%s\"\n",
                            label[j], testCases[i].localeID, buffer);
            }
            if (resultLen != (int32_t)strlen(buffer)) {
                log_err("FAIL: uloc_%s(%s) => len %d, expected len %d\n",
                        label[j], testCases[i].localeID, resultLen, strlen(buffer));
            }
            if (origResultLen != resultLen) {
                log_err("FAIL: uloc_%s(%s) => preflight len %d != actual len %d\n",
                        label[j], testCases[i].localeID, origResultLen, resultLen);
            }
        }
    }
}

static void TestDisplayKeywords(void)
{
    int32_t i;

    static const struct {
        const char *localeID;
        const char *displayLocale;
        UChar displayKeyword[200];
    } testCases[] = {
        {   "ca_ES@currency=ESP",         "de_AT", 
            {0x0057, 0x00e4, 0x0068, 0x0072, 0x0075, 0x006e, 0x0067, 0x0000}, 
        },
        {   "ja_JP@calendar=japanese",         "de", 
            { 0x004b, 0x0061, 0x006c, 0x0065, 0x006e, 0x0064, 0x0065, 0x0072, 0x0000}
        },
        {   "de_DE@collation=traditional",       "de_DE", 
            {0x0053, 0x006f, 0x0072, 0x0074, 0x0069, 0x0065, 0x0072, 0x0075, 0x006e, 0x0067, 0x0000}
        },
    };
    for(i = 0; i < sizeof(testCases)/sizeof(testCases[0]); i++) {
        UErrorCode status = U_ZERO_ERROR;
        const char* keyword =NULL;
        int32_t keywordLen = 0;
        int32_t keywordCount = 0;
        UChar *displayKeyword=NULL;
        int32_t displayKeywordLen = 0;
        UEnumeration* keywordEnum = uloc_openKeywords(testCases[i].localeID, &status);
        for(keywordCount = uenum_count(keywordEnum, &status); keywordCount > 0 ; keywordCount--){
              if(U_FAILURE(status)){
                  log_err("uloc_getKeywords failed for locale id: %s with error : %s \n", testCases[i].localeID, u_errorName(status)); 
                  break;
              }
              /* the uenum_next returns NUL terminated string */
              keyword = uenum_next(keywordEnum, &keywordLen, &status);
              /* fetch the displayKeyword */
              displayKeywordLen = uloc_getDisplayKeyword(keyword, testCases[i].displayLocale, displayKeyword, displayKeywordLen, &status);
              if(status==U_BUFFER_OVERFLOW_ERROR){
                  status = U_ZERO_ERROR;
                  displayKeywordLen++; /* for null termination */
                  displayKeyword = (UChar*) malloc(displayKeywordLen * U_SIZEOF_UCHAR);
                  displayKeywordLen = uloc_getDisplayKeyword(keyword, testCases[i].displayLocale, displayKeyword, displayKeywordLen, &status);
                  if(U_FAILURE(status)){
                      log_err("uloc_getDisplayKeyword filed for keyword : %s in locale id: %s for display locale: %s \n", testCases[i].localeID, keyword, testCases[i].displayLocale, u_errorName(status)); 
                      break; 
                  }
                  if(u_strncmp(displayKeyword, testCases[i].displayKeyword, displayKeywordLen)!=0){
                      if (status == U_USING_DEFAULT_WARNING) {
                          log_data_err("uloc_getDisplayKeyword did not get the expected value for keyword : %s in locale id: %s for display locale: %s . Got error: %s. Perhaps you are missing data?\n", testCases[i].localeID, keyword, testCases[i].displayLocale, u_errorName(status));
                      } else {
                          log_err("uloc_getDisplayKeyword did not get the expected value for keyword : %s in locale id: %s for display locale: %s \n", testCases[i].localeID, keyword, testCases[i].displayLocale);
                      }
                      break; 
                  }
              }else{
                  log_err("uloc_getDisplayKeyword did not return the expected error. Error: %s\n", u_errorName(status));
              }
              
              free(displayKeyword);

        }
        uenum_close(keywordEnum);
    }
}

static void TestDisplayKeywordValues(void){
    int32_t i;

    static const struct {
        const char *localeID;
        const char *displayLocale;
        UChar displayKeywordValue[500];
    } testCases[] = {
        {   "ca_ES@currency=ESP",         "de_AT", 
            {0x0053, 0x0070, 0x0061, 0x006e, 0x0069, 0x0073, 0x0063, 0x0068, 0x0065, 0x0020, 0x0050, 0x0065, 0x0073, 0x0065, 0x0074, 0x0061, 0x0000}
        },
        {   "de_AT@currency=ATS",         "fr_FR", 
            {0x0073, 0x0063, 0x0068, 0x0069, 0x006c, 0x006c, 0x0069, 0x006e, 0x0067, 0x0020, 0x0061, 0x0075, 0x0074, 0x0072, 0x0069, 0x0063, 0x0068, 0x0069, 0x0065, 0x006e, 0x0000}
        },
        { "de_DE@currency=DEM",         "it", 
            {0x004d, 0x0061, 0x0072, 0x0063, 0x006f, 0x0020, 0x0054, 0x0065, 0x0064, 0x0065, 0x0073, 0x0063, 0x006f, 0x0000}
        },
        {   "el_GR@currency=GRD",         "en",    
            {0x0047, 0x0072, 0x0065, 0x0065, 0x006b, 0x0020, 0x0044, 0x0072, 0x0061, 0x0063, 0x0068, 0x006d, 0x0061, 0x0000}
        },
        {   "eu_ES@currency=ESP",         "it_IT", 
            {0x0050, 0x0065, 0x0073, 0x0065, 0x0074, 0x0061, 0x0020, 0x0053, 0x0070, 0x0061, 0x0067, 0x006e, 0x006f, 0x006c, 0x0061, 0x0000}
        },
        {   "de@collation=phonebook",     "es",    
            {0x006F, 0x0072, 0x0064, 0x0065, 0x006E, 0x0020, 0x0064, 0x0065, 0x0020, 0x006C, 0x0069, 0x0073, 0x0074, 0x00ED, 0x006E, 0x0020, 0x0074, 0x0065, 0x006C, 0x0065, 0x0066, 0x00F3, 0x006E, 0x0069, 0x0063, 0x006F, 0x0000}
        },

        { "de_DE@collation=phonebook",  "es", 
          {0x006F, 0x0072, 0x0064, 0x0065, 0x006E, 0x0020, 0x0064, 0x0065, 0x0020, 0x006C, 0x0069, 0x0073, 0x0074, 0x00ED, 0x006E, 0x0020, 0x0074, 0x0065, 0x006C, 0x0065, 0x0066, 0x00F3, 0x006E, 0x0069, 0x0063, 0x006F, 0x0000}
        },
        { "es_ES@collation=traditional","de", 
          {0x0054, 0x0072, 0x0061, 0x0064, 0x0069, 0x0074, 0x0069, 0x006f, 0x006e, 0x0065, 0x006c, 0x006c, 0x0065, 0x0020, 0x0053, 0x006f, 0x0072, 0x0074, 0x0069, 0x0065, 0x0072, 0x0072, 0x0065, 0x0067, 0x0065, 0x006c, 0x006e, 0x0000}
        },
        { "ja_JP@calendar=japanese",    "de", 
           {0x004a, 0x0061, 0x0070, 0x0061, 0x006e, 0x0069, 0x0073, 0x0063, 0x0068, 0x0065, 0x0072, 0x0020, 0x004b, 0x0061, 0x006c, 0x0065, 0x006e, 0x0064, 0x0065, 0x0072, 0x0000}
        }, 
    };
    for(i = 0; i < sizeof(testCases)/sizeof(testCases[0]); i++) {
        UErrorCode status = U_ZERO_ERROR;
        const char* keyword =NULL;
        int32_t keywordLen = 0;
        int32_t keywordCount = 0;
        UChar *displayKeywordValue = NULL;
        int32_t displayKeywordValueLen = 0;
        UEnumeration* keywordEnum = uloc_openKeywords(testCases[i].localeID, &status);
        for(keywordCount = uenum_count(keywordEnum, &status); keywordCount > 0 ; keywordCount--){
              if(U_FAILURE(status)){
                  log_err("uloc_getKeywords failed for locale id: %s in display locale: % with error : %s \n", testCases[i].localeID, testCases[i].displayLocale, u_errorName(status)); 
                  break;
              }
              /* the uenum_next returns NUL terminated string */
              keyword = uenum_next(keywordEnum, &keywordLen, &status);
              
              /* fetch the displayKeywordValue */
              displayKeywordValueLen = uloc_getDisplayKeywordValue(testCases[i].localeID, keyword, testCases[i].displayLocale, displayKeywordValue, displayKeywordValueLen, &status);
              if(status==U_BUFFER_OVERFLOW_ERROR){
                  status = U_ZERO_ERROR;
                  displayKeywordValueLen++; /* for null termination */
                  displayKeywordValue = (UChar*)malloc(displayKeywordValueLen * U_SIZEOF_UCHAR);
                  displayKeywordValueLen = uloc_getDisplayKeywordValue(testCases[i].localeID, keyword, testCases[i].displayLocale, displayKeywordValue, displayKeywordValueLen, &status);
                  if(U_FAILURE(status)){
                      log_err("uloc_getDisplayKeywordValue failed for keyword : %s in locale id: %s for display locale: %s with error : %s \n", testCases[i].localeID, keyword, testCases[i].displayLocale, u_errorName(status)); 
                      break; 
                  }
                  if(u_strncmp(displayKeywordValue, testCases[i].displayKeywordValue, displayKeywordValueLen)!=0){
                      if (status == U_USING_DEFAULT_WARNING) {
                          log_data_err("uloc_getDisplayKeywordValue did not return the expected value keyword : %s in locale id: %s for display locale: %s with error : %s Perhaps you are missing data\n", testCases[i].localeID, keyword, testCases[i].displayLocale, u_errorName(status)); 
                      } else {
                          log_err("uloc_getDisplayKeywordValue did not return the expected value keyword : %s in locale id: %s for display locale: %s with error : %s \n", testCases[i].localeID, keyword, testCases[i].displayLocale, u_errorName(status)); 
                      }
                      break;   
                  }
              }else{
                  log_err("uloc_getDisplayKeywordValue did not return the expected error. Error: %s\n", u_errorName(status));
              }
              free(displayKeywordValue);
        }
        uenum_close(keywordEnum);
    }
    {   
        /* test a multiple keywords */
        UErrorCode status = U_ZERO_ERROR;
        const char* keyword =NULL;
        int32_t keywordLen = 0;
        int32_t keywordCount = 0;
        const char* localeID = "es@collation=phonebook;calendar=buddhist;currency=DEM";
        const char* displayLocale = "de";
        static const UChar expected[][50] = {
            {0x0042, 0x0075, 0x0064, 0x0064, 0x0068, 0x0069, 0x0073, 0x0074, 0x0069, 0x0073, 0x0063, 0x0068, 0x0065, 0x0072, 0x0020, 0x004b, 0x0061, 0x006c, 0x0065, 0x006e, 0x0064, 0x0065, 0x0072, 0x0000},

            {0x0054, 0x0065, 0x006c, 0x0065, 0x0066, 0x006f, 0x006e, 0x0062, 0x0075, 0x0063, 0x0068, 0x002d, 0x0053, 0x006f, 0x0072, 0x0074, 0x0069, 0x0065, 0x0072, 0x0072, 0x0065, 0x0067, 0x0065, 0x006c, 0x006e, 0x0000},
            {0x0044, 0x0065, 0x0075, 0x0074, 0x0073, 0x0063, 0x0068, 0x0065, 0x0020, 0x004d, 0x0061, 0x0072, 0x006b, 0x0000},
        };

        UEnumeration* keywordEnum = uloc_openKeywords(localeID, &status);

        for(keywordCount = 0; keywordCount < uenum_count(keywordEnum, &status) ; keywordCount++){
              UChar *displayKeywordValue = NULL;
              int32_t displayKeywordValueLen = 0;
              if(U_FAILURE(status)){
                  log_err("uloc_getKeywords failed for locale id: %s in display locale: % with error : %s \n", localeID, displayLocale, u_errorName(status)); 
                  break;
              }
              /* the uenum_next returns NUL terminated string */
              keyword = uenum_next(keywordEnum, &keywordLen, &status);
              
              /* fetch the displayKeywordValue */
              displayKeywordValueLen = uloc_getDisplayKeywordValue(localeID, keyword, displayLocale, displayKeywordValue, displayKeywordValueLen, &status);
              if(status==U_BUFFER_OVERFLOW_ERROR){
                  status = U_ZERO_ERROR;
                  displayKeywordValueLen++; /* for null termination */
                  displayKeywordValue = (UChar*)malloc(displayKeywordValueLen * U_SIZEOF_UCHAR);
                  displayKeywordValueLen = uloc_getDisplayKeywordValue(localeID, keyword, displayLocale, displayKeywordValue, displayKeywordValueLen, &status);
                  if(U_FAILURE(status)){
                      log_err("uloc_getDisplayKeywordValue failed for keyword : %s in locale id: %s for display locale: %s with error : %s \n", localeID, keyword, displayLocale, u_errorName(status)); 
                      break; 
                  }
                  if(u_strncmp(displayKeywordValue, expected[keywordCount], displayKeywordValueLen)!=0){
                      if (status == U_USING_DEFAULT_WARNING) {
                          log_data_err("uloc_getDisplayKeywordValue did not return the expected value keyword : %s in locale id: %s for display locale: %s  got error: %s. Perhaps you are missing data?\n", localeID, keyword, displayLocale, u_errorName(status));
                      } else {
                          log_err("uloc_getDisplayKeywordValue did not return the expected value keyword : %s in locale id: %s for display locale: %s \n", localeID, keyword, displayLocale);
                      }
                      break;   
                  }
              }else{
                  log_err("uloc_getDisplayKeywordValue did not return the expected error. Error: %s\n", u_errorName(status));
              }
              free(displayKeywordValue);
        }
        uenum_close(keywordEnum);
    
    }
    {
        /* Test non existent keywords */
        UErrorCode status = U_ZERO_ERROR;
        const char* localeID = "es";
        const char* displayLocale = "de";
        UChar *displayKeywordValue = NULL;
        int32_t displayKeywordValueLen = 0;
        
        /* fetch the displayKeywordValue */
        displayKeywordValueLen = uloc_getDisplayKeywordValue(localeID, "calendar", displayLocale, displayKeywordValue, displayKeywordValueLen, &status);
        if(U_FAILURE(status)) {
          log_err("uloc_getDisplaykeywordValue returned error status %s\n", u_errorName(status));
        } else if(displayKeywordValueLen != 0) {
          log_err("uloc_getDisplaykeywordValue returned %d should be 0 \n", displayKeywordValueLen);
        }
    }
}


static void TestGetBaseName(void) {
    static const struct {
        const char *localeID;
        const char *baseName;
    } testCases[] = {
        { "de_DE@  C o ll A t i o n   = Phonebook   ", "de_DE" },
        { "de@currency = euro; CoLLaTion   = PHONEBOOk", "de" },
        { "ja@calendar = buddhist", "ja" }
    };

    int32_t i = 0, baseNameLen = 0;
    char baseName[256];
    UErrorCode status = U_ZERO_ERROR;

    for(i = 0; i < sizeof(testCases)/sizeof(testCases[0]); i++) {
        baseNameLen = uloc_getBaseName(testCases[i].localeID, baseName, 256, &status);
        if(strcmp(testCases[i].baseName, baseName)) {
            log_err("For locale \"%s\" expected baseName \"%s\", but got \"%s\"\n",
                testCases[i].localeID, testCases[i].baseName, baseName);
            return;
        }
    }
}

static void TestTrailingNull(void) {
  const char* localeId = "zh_Hans";
  UChar buffer[128]; /* sufficient for this test */
  int32_t len;
  UErrorCode status = U_ZERO_ERROR;
  int i;

  len = uloc_getDisplayName(localeId, localeId, buffer, 128, &status);
  if (len > 128) {
    log_err("buffer too small");
    return;
  }

  for (i = 0; i < len; ++i) {
    if (buffer[i] == 0) {
      log_err("name contained null");
      return;
    }
  }
}

/* Jitterbug 4115 */
static void TestDisplayNameWarning(void) {
    UChar name[256];
    int32_t size;
    UErrorCode status = U_ZERO_ERROR;
    
    size = uloc_getDisplayLanguage("qqq", "kl", name, sizeof(name)/sizeof(name[0]), &status);
    if (status != U_USING_DEFAULT_WARNING) {
        log_err("For language \"qqq\" in locale \"kl\", expecting U_USING_DEFAULT_WARNING, but got %s\n",
            u_errorName(status));
    }
}


/**
 * Compare two locale IDs.  If they are equal, return 0.  If `string'
 * starts with `prefix' plus an additional element, that is, string ==
 * prefix + '_' + x, then return 1.  Otherwise return a value < 0.
 */
static UBool _loccmp(const char* string, const char* prefix) {
    int32_t slen = (int32_t)uprv_strlen(string),
            plen = (int32_t)uprv_strlen(prefix);
    int32_t c = uprv_strncmp(string, prefix, plen);
    /* 'root' is less than everything */
    if (uprv_strcmp(prefix, "root") == 0) {
        return (uprv_strcmp(string, "root") == 0) ? 0 : 1;
    }
    if (c) return -1; /* mismatch */
    if (slen == plen) return 0;
    if (string[plen] == '_') return 1;
    return -2; /* false match, e.g. "en_USX" cmp "en_US" */
}

static void _checklocs(const char* label,
                       const char* req,
                       const char* valid,
                       const char* actual) {
    /* We want the valid to be strictly > the bogus requested locale,
       and the valid to be >= the actual. */
    if (_loccmp(req, valid) > 0 &&
        _loccmp(valid, actual) >= 0) {
        log_verbose("%s; req=%s, valid=%s, actual=%s\n",
                    label, req, valid, actual);
    } else {
        log_err("FAIL: %s; req=%s, valid=%s, actual=%s\n",
                label, req, valid, actual);
    }
}

static void TestGetLocale(void) {
    UErrorCode ec = U_ZERO_ERROR;
    UParseError pe;
    UChar EMPTY[1] = {0};

    /* === udat === */
#if !UCONFIG_NO_FORMATTING
    {
        UDateFormat *obj;
        const char *req = "en_US_REDWOODSHORES", *valid, *actual;
        obj = udat_open(UDAT_DEFAULT, UDAT_DEFAULT,
                        req,
                        NULL, 0,
                        NULL, 0, &ec);
        if (U_FAILURE(ec)) {
            log_data_err("udat_open failed.Error %s\n", u_errorName(ec));
            return;
        }
        valid = udat_getLocaleByType(obj, ULOC_VALID_LOCALE, &ec);
        actual = udat_getLocaleByType(obj, ULOC_ACTUAL_LOCALE, &ec);
        if (U_FAILURE(ec)) {
            log_err("udat_getLocaleByType() failed\n");
            return;
        }
        _checklocs("udat", req, valid, actual);
        udat_close(obj);
    }
#endif

    /* === ucal === */
#if !UCONFIG_NO_FORMATTING
    {
        UCalendar *obj;
        const char *req = "fr_FR_PROVENCAL", *valid, *actual;
        obj = ucal_open(NULL, 0,
                        req,
                        UCAL_GREGORIAN,
                        &ec);
        if (U_FAILURE(ec)) {
            log_err("ucal_open failed with error: %s\n", u_errorName(ec));
            return;
        }
        valid = ucal_getLocaleByType(obj, ULOC_VALID_LOCALE, &ec);
        actual = ucal_getLocaleByType(obj, ULOC_ACTUAL_LOCALE, &ec);
        if (U_FAILURE(ec)) {
            log_err("ucal_getLocaleByType() failed\n");
            return;
        }
        _checklocs("ucal", req, valid, actual);
        ucal_close(obj);
    }
#endif

    /* === unum === */
#if !UCONFIG_NO_FORMATTING
    {
        UNumberFormat *obj;
        const char *req = "zh_Hant_TW_TAINAN", *valid, *actual;
        obj = unum_open(UNUM_DECIMAL,
                        NULL, 0,
                        req,
                        &pe, &ec);
        if (U_FAILURE(ec)) {
            log_err("unum_open failed\n");
            return;
        }
        valid = unum_getLocaleByType(obj, ULOC_VALID_LOCALE, &ec);
        actual = unum_getLocaleByType(obj, ULOC_ACTUAL_LOCALE, &ec);
        if (U_FAILURE(ec)) {
            log_err("unum_getLocaleByType() failed\n");
            return;
        }
        _checklocs("unum", req, valid, actual);
        unum_close(obj);
    }
#endif

    /* === umsg === */
#if 0
    /* commented out by weiv 01/12/2005. umsg_getLocaleByType is to be removed */
#if !UCONFIG_NO_FORMATTING
    {
        UMessageFormat *obj;
        const char *req = "ja_JP_TAKAYAMA", *valid, *actual;
        UBool test;
        obj = umsg_open(EMPTY, 0,
                        req,
                        &pe, &ec);
        if (U_FAILURE(ec)) {
            log_err("umsg_open failed\n");
            return;
        }
        valid = umsg_getLocaleByType(obj, ULOC_VALID_LOCALE, &ec);
        actual = umsg_getLocaleByType(obj, ULOC_ACTUAL_LOCALE, &ec);
        if (U_FAILURE(ec)) {
            log_err("umsg_getLocaleByType() failed\n");
            return;
        }
        /* We want the valid to be strictly > the bogus requested locale,
           and the valid to be >= the actual. */
        /* TODO MessageFormat is currently just storing the locale it is given.
           As a result, it will return whatever it was given, even if the
           locale is invalid. */
        test = (_cmpversion("3.2") <= 0) ?
            /* Here is the weakened test for 3.0: */
            (_loccmp(req, valid) >= 0) :
            /* Here is what the test line SHOULD be: */
            (_loccmp(req, valid) > 0);

        if (test &&
            _loccmp(valid, actual) >= 0) {
            log_verbose("umsg; req=%s, valid=%s, actual=%s\n", req, valid, actual);
        } else {
            log_err("FAIL: umsg; req=%s, valid=%s, actual=%s\n", req, valid, actual);
        }
        umsg_close(obj);
    }
#endif
#endif

    /* === ubrk === */
#if !UCONFIG_NO_BREAK_ITERATION
    {
        UBreakIterator *obj;
        const char *req = "ar_KW_ABDALI", *valid, *actual;
        obj = ubrk_open(UBRK_WORD,
                        req,
                        EMPTY,
                        0,
                        &ec);
        if (U_FAILURE(ec)) {
            log_err("ubrk_open failed. Error: %s \n", u_errorName(ec));
            return;
        }
        valid = ubrk_getLocaleByType(obj, ULOC_VALID_LOCALE, &ec);
        actual = ubrk_getLocaleByType(obj, ULOC_ACTUAL_LOCALE, &ec);
        if (U_FAILURE(ec)) {
            log_err("ubrk_getLocaleByType() failed\n");
            return;
        }
        _checklocs("ubrk", req, valid, actual);
        ubrk_close(obj);
    }
#endif

    /* === ucol === */
#if !UCONFIG_NO_COLLATION
    {
        UCollator *obj;
        const char *req = "es_AR_BUENOSAIRES", *valid, *actual;
        obj = ucol_open(req, &ec);
        if (U_FAILURE(ec)) {
            log_err("ucol_open failed - %s\n", u_errorName(ec));
            return;
        }
        valid = ucol_getLocaleByType(obj, ULOC_VALID_LOCALE, &ec);
        actual = ucol_getLocaleByType(obj, ULOC_ACTUAL_LOCALE, &ec);
        if (U_FAILURE(ec)) {
            log_err("ucol_getLocaleByType() failed\n");
            return;
        }
        _checklocs("ucol", req, valid, actual);
        ucol_close(obj);
    }
#endif
}

static void TestNonexistentLanguageExemplars(void) {
    /* JB 4068 - Nonexistent language */
    UErrorCode ec = U_ZERO_ERROR;
    ULocaleData *uld = ulocdata_open("qqq",&ec);
    if (ec != U_USING_DEFAULT_WARNING) {
        log_err_status(ec, "Exemplar set for \"qqq\", expecting U_USING_DEFAULT_WARNING, but got %s\n",
            u_errorName(ec));
    }
    uset_close(ulocdata_getExemplarSet(uld, NULL, 0, ULOCDATA_ES_STANDARD, &ec));
    ulocdata_close(uld);
}

static void TestLocDataErrorCodeChaining(void) {
    UErrorCode ec = U_USELESS_COLLATOR_ERROR;
    ulocdata_open(NULL, &ec);
    ulocdata_getExemplarSet(NULL, NULL, 0, ULOCDATA_ES_STANDARD, &ec);
    ulocdata_getDelimiter(NULL, ULOCDATA_DELIMITER_COUNT, NULL, -1, &ec);
    ulocdata_getMeasurementSystem(NULL, &ec);
    ulocdata_getPaperSize(NULL, NULL, NULL, &ec);
    if (ec != U_USELESS_COLLATOR_ERROR) {
        log_err("ulocdata API changed the error code to %s\n", u_errorName(ec));
    }
}

static void TestLanguageExemplarsFallbacks(void) {
    /* Test that en_US fallsback, but en doesn't fallback. */
    UErrorCode ec = U_ZERO_ERROR;
    ULocaleData *uld = ulocdata_open("en_US",&ec);
    uset_close(ulocdata_getExemplarSet(uld, NULL, 0, ULOCDATA_ES_STANDARD, &ec));
    if (ec != U_USING_FALLBACK_WARNING) {
        log_err_status(ec, "Exemplar set for \"en_US\", expecting U_USING_FALLBACK_WARNING, but got %s\n",
            u_errorName(ec));
    }
    ulocdata_close(uld);
    ec = U_ZERO_ERROR;
    uld = ulocdata_open("en",&ec);
    uset_close(ulocdata_getExemplarSet(uld, NULL, 0, ULOCDATA_ES_STANDARD, &ec));
    if (ec != U_ZERO_ERROR) {
        log_err_status(ec, "Exemplar set for \"en\", expecting U_ZERO_ERROR, but got %s\n",
            u_errorName(ec));
    }
    ulocdata_close(uld);
}

static const char *acceptResult(UAcceptResult uar) {
    return  udbg_enumName(UDBG_UAcceptResult, uar);
}

static void TestAcceptLanguage(void) {
    UErrorCode status = U_ZERO_ERROR;
    UAcceptResult outResult;
    UEnumeration *available;
    char tmp[200];
    int i;
    int32_t rc = 0;

    struct { 
        int32_t httpSet;       /**< Which of http[] should be used? */
        const char *icuSet;    /**< ? */
        const char *expect;    /**< The expected locale result */
        UAcceptResult res;     /**< The expected error code */
    } tests[] = { 
        /*0*/{ 0, NULL, "mt_MT", ULOC_ACCEPT_VALID },
        /*1*/{ 1, NULL, "en", ULOC_ACCEPT_VALID },
        /*2*/{ 2, NULL, "en", ULOC_ACCEPT_FALLBACK },
        /*3*/{ 3, NULL, "", ULOC_ACCEPT_FAILED },
        /*4*/{ 4, NULL, "es", ULOC_ACCEPT_VALID },
        
        /*5*/{ 5, NULL, "en", ULOC_ACCEPT_VALID },  /* XF */
        /*6*/{ 6, NULL, "ja", ULOC_ACCEPT_FALLBACK },  /* XF */
        /*7*/{ 7, NULL, "zh", ULOC_ACCEPT_FALLBACK },  /* XF */
    };
    const int32_t numTests = sizeof(tests)/sizeof(tests[0]);
    static const char *http[] = {
        /*0*/ "mt-mt, ja;q=0.76, en-us;q=0.95, en;q=0.92, en-gb;q=0.89, fr;q=0.87, iu-ca;q=0.84, iu;q=0.82, ja-jp;q=0.79, mt;q=0.97, de-de;q=0.74, de;q=0.71, es;q=0.68, it-it;q=0.66, it;q=0.63, vi-vn;q=0.61, vi;q=0.58, nl-nl;q=0.55, nl;q=0.53, th-th-traditional;q=.01",
        /*1*/ "ja;q=0.5, en;q=0.8, tlh",
        /*2*/ "en-wf, de-lx;q=0.8",
        /*3*/ "mga-ie;q=0.9, tlh",
        /*4*/ "xxx-yyy;q=.01, xxx-yyy;q=.01, xxx-yyy;q=.01, xxx-yyy;q=.01, xxx-yyy;q=.01, "
              "xxx-yyy;q=.01, xxx-yyy;q=.01, xxx-yyy;q=.01, xxx-yyy;q=.01, xxx-yyy;q=.01, "
              "xxx-yyy;q=.01, xxx-yyy;q=.01, xxx-yyy;q=.01, xxx-yyy;q=.01, xxx-yyy;q=.01, "
              "xxx-yyy;q=.01, xxx-yyy;q=.01, xxx-yyy;q=.01, xxx-yyy;q=.01, xxx-yyy;q=.01, "
              "xxx-yyy;q=.01, xxx-yyy;q=.01, xxx-yyy;q=.01, xxx-yyy;q=.01, xxx-yyy;q=.01, "
              "xxx-yyy;q=.01, xxx-yyy;q=.01, xxx-yyy;q=.01, xxx-yyy;q=.01, xxx-yyy;q=.01, "
              "xxx-yyy;q=.01, xxx-yyy;q=.01, xxx-yyy;q=.01, xx-yy;q=.1, "
              "es",
              
        /*5*/ "zh-xx;q=0.9, en;q=0.6",
        /*6*/ "ja-JA",
        /*7*/ "zh-xx;q=0.9",
    };

    for(i=0;i<numTests;i++) {
        outResult = -3;
        status=U_ZERO_ERROR;
        log_verbose("test #%d: http[%s], ICU[%s], expect %s, %s\n", 
            i, http[tests[i].httpSet], tests[i].icuSet, tests[i].expect, acceptResult(tests[i].res));

        available = ures_openAvailableLocales(tests[i].icuSet, &status);
        tmp[0]=0;
        rc = uloc_acceptLanguageFromHTTP(tmp, 199, &outResult, http[tests[i].httpSet], available, &status);
        uenum_close(available);
        log_verbose(" got %s, %s [%s]\n", tmp[0]?tmp:"(EMPTY)", acceptResult(outResult), u_errorName(status));
        if(outResult != tests[i].res) {
            log_err_status(status, "FAIL: #%d: expected outResult of %s but got %s\n", i, 
                acceptResult( tests[i].res), 
                acceptResult( outResult));
            log_info("test #%d: http[%s], ICU[%s], expect %s, %s\n", 
                i, http[tests[i].httpSet], tests[i].icuSet, tests[i].expect,acceptResult(tests[i].res));
        }
        if((outResult>0)&&uprv_strcmp(tmp, tests[i].expect)) {
            log_err_status(status, "FAIL: #%d: expected %s but got %s\n", i, tests[i].expect, tmp);
            log_info("test #%d: http[%s], ICU[%s], expect %s, %s\n", 
                i, http[tests[i].httpSet], tests[i].icuSet, tests[i].expect, acceptResult(tests[i].res));
        }
    }
}

static const char* LOCALE_ALIAS[][2] = {
    {"in", "id"},
    {"in_ID", "id_ID"},
    {"iw", "he"},
    {"iw_IL", "he_IL"},
    {"ji", "yi"},
    {"en_BU", "en_MM"},
    {"en_DY", "en_BJ"},
    {"en_HV", "en_BF"},
    {"en_NH", "en_VU"},
    {"en_RH", "en_ZW"},
    {"en_TP", "en_TL"},
    {"en_ZR", "en_CD"}
};
static UBool isLocaleAvailable(UResourceBundle* resIndex, const char* loc){
    UErrorCode status = U_ZERO_ERROR;
    int32_t len = 0;
    ures_getStringByKey(resIndex, loc,&len, &status);
    if(U_FAILURE(status)){
        return FALSE; 
    }
    return TRUE;
}

static void TestCalendar() {
#if !UCONFIG_NO_FORMATTING
    int i;
    UErrorCode status = U_ZERO_ERROR;
    UResourceBundle *resIndex = ures_open(NULL,"res_index", &status);
    if(U_FAILURE(status)){
        log_err_status(status, "Could not open res_index.res. Exiting. Error: %s\n", u_errorName(status));
        return;
    }
    for (i=0; i<LENGTHOF(LOCALE_ALIAS); i++) {
        const char* oldLoc = LOCALE_ALIAS[i][0];
        const char* newLoc = LOCALE_ALIAS[i][1];
        UCalendar* c1 = NULL;
        UCalendar* c2 = NULL;

        /*Test function "getLocale(ULocale.VALID_LOCALE)"*/
        const char* l1 = ucal_getLocaleByType(c1, ULOC_VALID_LOCALE, &status);
        const char* l2 = ucal_getLocaleByType(c2, ULOC_VALID_LOCALE, &status);

        if(!isLocaleAvailable(resIndex, newLoc)){
            continue;
        }
        c1 = ucal_open(NULL, -1, oldLoc, UCAL_GREGORIAN, &status);
        c2 = ucal_open(NULL, -1, newLoc, UCAL_GREGORIAN, &status);

        if (strcmp(newLoc,l1)!=0 || strcmp(l1,l2)!=0 || status!=U_ZERO_ERROR) {
            log_err("The locales are not equal!.Old: %s, New: %s \n", oldLoc, newLoc);
        }
        log_verbose("ucal_getLocaleByType old:%s   new:%s\n", l1, l2);
        ucal_close(c1);
        ucal_close(c2);
    }
    ures_close(resIndex);
#endif
}

static void TestDateFormat() {
#if !UCONFIG_NO_FORMATTING
    int i;
    UErrorCode status = U_ZERO_ERROR;
    UResourceBundle *resIndex = ures_open(NULL,"res_index", &status);
    if(U_FAILURE(status)){
        log_err_status(status, "Could not open res_index.res. Exiting. Error: %s\n", u_errorName(status));
        return;
    }
    for (i=0; i<LENGTHOF(LOCALE_ALIAS); i++) {
        const char* oldLoc = LOCALE_ALIAS[i][0];
        const char* newLoc = LOCALE_ALIAS[i][1];
        UDateFormat* df1 = NULL;
        UDateFormat* df2 = NULL;
        const char* l1 = NULL;
        const char* l2 = NULL;

        if(!isLocaleAvailable(resIndex, newLoc)){
            continue;
        }
        df1 = udat_open(UDAT_FULL, UDAT_FULL,oldLoc, NULL, 0, NULL, -1, &status);
        df2 = udat_open(UDAT_FULL, UDAT_FULL,newLoc, NULL, 0, NULL, -1, &status);
        if(U_FAILURE(status)){
            log_err("Creation of date format failed  %s\n", u_errorName(status));
            return;
        }        
        /*Test function "getLocale"*/
        l1 = udat_getLocaleByType(df1, ULOC_VALID_LOCALE, &status);
        l2 = udat_getLocaleByType(df2, ULOC_VALID_LOCALE, &status);
        if(U_FAILURE(status)){
            log_err("Fetching the locale by type failed.  %s\n", u_errorName(status));
        }
        if (strcmp(newLoc,l1)!=0 || strcmp(l1,l2)!=0) {
            log_err("The locales are not equal!.Old: %s, New: %s \n", oldLoc, newLoc);
        }
        log_verbose("udat_getLocaleByType old:%s   new:%s\n", l1, l2);
        udat_close(df1);
        udat_close(df2);
    }
    ures_close(resIndex);
#endif
}

static void TestCollation() {
#if !UCONFIG_NO_COLLATION
    int i;
    UErrorCode status = U_ZERO_ERROR;
    UResourceBundle *resIndex = ures_open(NULL,"res_index", &status);
    if(U_FAILURE(status)){
        log_err_status(status, "Could not open res_index.res. Exiting. Error: %s\n", u_errorName(status));
        return;
    }
    for (i=0; i<LENGTHOF(LOCALE_ALIAS); i++) {
        const char* oldLoc = LOCALE_ALIAS[i][0];
        const char* newLoc = LOCALE_ALIAS[i][1];
        UCollator* c1 = NULL;
        UCollator* c2 = NULL;
        const char* l1 = NULL;
        const char* l2 = NULL;

        status = U_ZERO_ERROR;
        if(!isLocaleAvailable(resIndex, newLoc)){
            continue;
        }
        if(U_FAILURE(status)){
            log_err("Creation of collators failed  %s\n", u_errorName(status));
            return;
        }
        c1 = ucol_open(oldLoc, &status);
        c2 = ucol_open(newLoc, &status);
        l1 = ucol_getLocaleByType(c1, ULOC_VALID_LOCALE, &status);
        l2 = ucol_getLocaleByType(c2, ULOC_VALID_LOCALE, &status);
        if(U_FAILURE(status)){
            log_err("Fetching the locale names failed failed  %s\n", u_errorName(status));
        }        
        if (strcmp(newLoc,l1)!=0 || strcmp(l1,l2)!=0) {
            log_err("The locales are not equal!.Old: %s, New: %s \n", oldLoc, newLoc);
        }
        log_verbose("ucol_getLocaleByType old:%s   new:%s\n", l1, l2);
        ucol_close(c1);
        ucol_close(c2);
    }
    ures_close(resIndex);
#endif
}

typedef struct OrientationStructTag {
    const char* localeId;
    ULayoutType character;
    ULayoutType line;
} OrientationStruct;

static const char* ULayoutTypeToString(ULayoutType type)
{
    switch(type)
    {
    case ULOC_LAYOUT_LTR:
        return "ULOC_LAYOUT_LTR";
        break;
    case ULOC_LAYOUT_RTL:
        return "ULOC_LAYOUT_RTL";
        break;
    case ULOC_LAYOUT_TTB:
        return "ULOC_LAYOUT_TTB";
        break;
    case ULOC_LAYOUT_BTT:
        return "ULOC_LAYOUT_BTT";
        break;
    case ULOC_LAYOUT_UNKNOWN:
        break;
    }

    return "Unknown enum value for ULayoutType!";
}

static void  TestOrientation()
{
    static const OrientationStruct toTest [] = {
        { "ar", ULOC_LAYOUT_RTL, ULOC_LAYOUT_TTB },
        { "aR", ULOC_LAYOUT_RTL, ULOC_LAYOUT_TTB },
        { "ar_Arab", ULOC_LAYOUT_RTL, ULOC_LAYOUT_TTB },
        { "fa", ULOC_LAYOUT_RTL, ULOC_LAYOUT_TTB },
        { "Fa", ULOC_LAYOUT_RTL, ULOC_LAYOUT_TTB },
        { "he", ULOC_LAYOUT_RTL, ULOC_LAYOUT_TTB },
        { "ps", ULOC_LAYOUT_RTL, ULOC_LAYOUT_TTB },
        { "ur", ULOC_LAYOUT_RTL, ULOC_LAYOUT_TTB },
        { "UR", ULOC_LAYOUT_RTL, ULOC_LAYOUT_TTB },
        { "en", ULOC_LAYOUT_LTR, ULOC_LAYOUT_TTB }
    };

    size_t i = 0;
    for (; i < sizeof(toTest) / sizeof(toTest[0]); ++i) {
        UErrorCode statusCO = U_ZERO_ERROR;
        UErrorCode statusLO = U_ZERO_ERROR;
        const char* const localeId = toTest[i].localeId;
        const ULayoutType co = uloc_getCharacterOrientation(localeId, &statusCO);
        const ULayoutType expectedCO = toTest[i].character;
        const ULayoutType lo = uloc_getLineOrientation(localeId, &statusLO);
        const ULayoutType expectedLO = toTest[i].line;
        if (U_FAILURE(statusCO)) {
            log_err_status(statusCO,
                "  unexpected failure for uloc_getCharacterOrientation(), with localId \"%s\" and status %s\n",
                localeId,
                u_errorName(statusCO));
        }
        else if (co != expectedCO) {
            log_err(
                "  unexpected result for uloc_getCharacterOrientation(), with localeId \"%s\". Expected %s but got result %s\n",
                localeId,
                ULayoutTypeToString(expectedCO),
                ULayoutTypeToString(co));
        }
        if (U_FAILURE(statusLO)) {
            log_err_status(statusLO,
                "  unexpected failure for uloc_getLineOrientation(), with localId \"%s\" and status %s\n",
                localeId,
                u_errorName(statusLO));
        }
        else if (lo != expectedLO) {
            log_err(
                "  unexpected result for uloc_getLineOrientation(), with localeId \"%s\". Expected %s but got result %s\n",
                localeId,
                ULayoutTypeToString(expectedLO),
                ULayoutTypeToString(lo));
        }
    }
}

static void  TestULocale() {
    int i;
    UErrorCode status = U_ZERO_ERROR;
    UResourceBundle *resIndex = ures_open(NULL,"res_index", &status);
    if(U_FAILURE(status)){
        log_err_status(status, "Could not open res_index.res. Exiting. Error: %s\n", u_errorName(status));
        return;
    }
    for (i=0; i<LENGTHOF(LOCALE_ALIAS); i++) {
        const char* oldLoc = LOCALE_ALIAS[i][0];
        const char* newLoc = LOCALE_ALIAS[i][1];
        UChar name1[256], name2[256];
        char names1[256], names2[256];
        int32_t capacity = 256;

        status = U_ZERO_ERROR;
        if(!isLocaleAvailable(resIndex, newLoc)){
            continue;
        }
        uloc_getDisplayName(oldLoc, ULOC_US, name1, capacity, &status);
        if(U_FAILURE(status)){
            log_err("uloc_getDisplayName(%s) failed %s\n", oldLoc, u_errorName(status));
        }

        uloc_getDisplayName(newLoc, ULOC_US, name2, capacity, &status);
        if(U_FAILURE(status)){
            log_err("uloc_getDisplayName(%s) failed %s\n", newLoc, u_errorName(status));
        }

        if (u_strcmp(name1, name2)!=0) {
            log_err("The locales are not equal!.Old: %s, New: %s \n", oldLoc, newLoc);
        }
        u_austrcpy(names1, name1);
        u_austrcpy(names2, name2);
        log_verbose("uloc_getDisplayName old:%s   new:%s\n", names1, names2);
    }
    ures_close(resIndex);

}

static void TestUResourceBundle() {
    const char* us1;
    const char* us2;

    UResourceBundle* rb1 = NULL;
    UResourceBundle* rb2 = NULL;
    UErrorCode status = U_ZERO_ERROR;
    int i;
    UResourceBundle *resIndex = NULL;
    if(U_FAILURE(status)){
        log_err("Could not open res_index.res. Exiting. Error: %s\n", u_errorName(status));
        return;
    }
    resIndex = ures_open(NULL,"res_index", &status);
    for (i=0; i<LENGTHOF(LOCALE_ALIAS); i++) {

        const char* oldLoc = LOCALE_ALIAS[i][0];
        const char* newLoc = LOCALE_ALIAS[i][1];
        if(!isLocaleAvailable(resIndex, newLoc)){
            continue;
        }
        rb1 = ures_open(NULL, oldLoc, &status);
        if (U_FAILURE(status)) {
            log_err("ures_open(%s) failed %s\n", oldLoc, u_errorName(status));
        }

        us1 = ures_getLocaleByType(rb1, ULOC_ACTUAL_LOCALE, &status);

        status = U_ZERO_ERROR;
        rb2 = ures_open(NULL, newLoc, &status);
        if (U_FAILURE(status)) {
            log_err("ures_open(%s) failed %s\n", oldLoc, u_errorName(status));
        } 
        us2 = ures_getLocaleByType(rb2, ULOC_ACTUAL_LOCALE, &status);

        if (strcmp(us1,newLoc)!=0 || strcmp(us1,us2)!=0 ) {
            log_err("The locales are not equal!.Old: %s, New: %s \n", oldLoc, newLoc);
        }

        log_verbose("ures_getStringByKey old:%s   new:%s\n", us1, us2);
        ures_close(rb1);
        rb1 = NULL;
        ures_close(rb2);
        rb2 = NULL;
    }
    ures_close(resIndex);
}

static void TestDisplayName() {
    
    UChar oldCountry[256] = {'\0'};
    UChar newCountry[256] = {'\0'};
    UChar oldLang[256] = {'\0'};
    UChar newLang[256] = {'\0'};
    char country[256] ={'\0'}; 
    char language[256] ={'\0'};
    int32_t capacity = 256;
    int i =0;
    int j=0;
    for (i=0; i<LENGTHOF(LOCALE_ALIAS); i++) {
        const char* oldLoc = LOCALE_ALIAS[i][0];
        const char* newLoc = LOCALE_ALIAS[i][1];
        UErrorCode status = U_ZERO_ERROR;
        int32_t available = uloc_countAvailable();

        for(j=0; j<available; j++){
            
            const char* dispLoc = uloc_getAvailable(j);
            int32_t oldCountryLen = uloc_getDisplayCountry(oldLoc,dispLoc, oldCountry, capacity, &status);
            int32_t newCountryLen = uloc_getDisplayCountry(newLoc, dispLoc, newCountry, capacity, &status);
            int32_t oldLangLen = uloc_getDisplayLanguage(oldLoc, dispLoc, oldLang, capacity, &status);
            int32_t newLangLen = uloc_getDisplayLanguage(newLoc, dispLoc, newLang, capacity, &status );
            
            int32_t countryLen = uloc_getCountry(newLoc, country, capacity, &status);
            int32_t langLen  = uloc_getLanguage(newLoc, language, capacity, &status);
            /* there is a display name for the current country ID */
            if(countryLen != newCountryLen ){
                if(u_strncmp(oldCountry,newCountry,oldCountryLen)!=0){
                    log_err("uloc_getDisplayCountry() failed for %s in display locale %s \n", oldLoc, dispLoc);
                }
            }
            /* there is a display name for the current lang ID */
            if(langLen!=newLangLen){
                if(u_strncmp(oldLang,newLang,oldLangLen)){
                    log_err("uloc_getDisplayLanguage() failed for %s in display locale %s \n", oldLoc, dispLoc);                }
            }
        }
    }
}

static void TestGetLocaleForLCID() {
    int32_t i, length, lengthPre;
    const char* testLocale = 0;
    UErrorCode status = U_ZERO_ERROR;
    char            temp2[40], temp3[40];
    uint32_t lcid;
    
    lcid = uloc_getLCID("en_US");
    if (lcid != 0x0409) {
        log_err("  uloc_getLCID(\"en_US\") = %d, expected 0x0409\n", lcid);
    }
    
    lengthPre = uloc_getLocaleForLCID(lcid, temp2, 4, &status);
    if (status != U_BUFFER_OVERFLOW_ERROR) {
        log_err("  unexpected result from uloc_getLocaleForLCID with small buffer: %s\n", u_errorName(status));
    }
    else {
        status = U_ZERO_ERROR;
    }
    
    length = uloc_getLocaleForLCID(lcid, temp2, sizeof(temp2)/sizeof(char), &status);
    if (U_FAILURE(status)) {
        log_err("  unexpected result from uloc_getLocaleForLCID(0x0409): %s\n", u_errorName(status));
        status = U_ZERO_ERROR;
    }
    
    if (length != lengthPre) {
        log_err("  uloc_getLocaleForLCID(0x0409): returned length %d does not match preflight length %d\n", length, lengthPre);
    }
    
    length = uloc_getLocaleForLCID(0x12345, temp2, sizeof(temp2)/sizeof(char), &status);
    if (U_SUCCESS(status)) {
        log_err("  unexpected result from uloc_getLocaleForLCID(0x12345): %s, status %s\n", temp2, u_errorName(status));
    }
    status = U_ZERO_ERROR;
    
    log_verbose("Testing getLocaleForLCID vs. locale data\n");
    for (i = 0; i < LOCALE_SIZE; i++) {
        
        testLocale=rawData2[NAME][i];
        
        log_verbose("Testing   %s ......\n", testLocale);
        
        sscanf(rawData2[LCID][i], "%x", &lcid);
        length = uloc_getLocaleForLCID(lcid, temp2, sizeof(temp2)/sizeof(char), &status);
        if (U_FAILURE(status)) {
            log_err("  unexpected failure of uloc_getLocaleForLCID(%#04x), status %s\n", lcid, u_errorName(status));
            status = U_ZERO_ERROR;
            continue;
        }
        
        if (length != uprv_strlen(temp2)) {
            log_err("  returned length %d not correct for uloc_getLocaleForLCID(%#04x), expected %d\n", length, lcid, uprv_strlen(temp2));
        }
        
        /* Compare language, country, script */
        length = uloc_getLanguage(temp2, temp3, sizeof(temp3)/sizeof(char), &status);
        if (U_FAILURE(status)) {
            log_err("  couldn't get language in uloc_getLocaleForLCID(%#04x) = %s, status %s\n", lcid, temp2, u_errorName(status));
            status = U_ZERO_ERROR;
        }
        else if (uprv_strcmp(temp3, rawData2[LANG][i]) && !(uprv_strcmp(temp3, "nn") == 0 && uprv_strcmp(rawData2[VAR][i], "NY") == 0)) {
            log_err("  language doesn't match expected %s in in uloc_getLocaleForLCID(%#04x) = %s\n", rawData2[LANG][i], lcid, temp2);
        }
        
        length = uloc_getScript(temp2, temp3, sizeof(temp3)/sizeof(char), &status);
        if (U_FAILURE(status)) {
            log_err("  couldn't get script in uloc_getLocaleForLCID(%#04x) = %s, status %s\n", lcid, temp2, u_errorName(status));
            status = U_ZERO_ERROR;
        }
        else if (uprv_strcmp(temp3, rawData2[SCRIPT][i])) {
            log_err("  script doesn't match expected %s in in uloc_getLocaleForLCID(%#04x) = %s\n", rawData2[SCRIPT][i], lcid, temp2);
        }
        
        length = uloc_getCountry(temp2, temp3, sizeof(temp3)/sizeof(char), &status);
        if (U_FAILURE(status)) {
            log_err("  couldn't get country in uloc_getLocaleForLCID(%#04x) = %s, status %s\n", lcid, temp2, u_errorName(status));
            status = U_ZERO_ERROR;
        }
        else if (uprv_strlen(rawData2[CTRY][i]) && uprv_strcmp(temp3, rawData2[CTRY][i])) {
            log_err("  country doesn't match expected %s in in uloc_getLocaleForLCID(%#04x) = %s\n", rawData2[CTRY][i], lcid, temp2);
        }
    }
    
}

const char* const basic_maximize_data[][2] = {
  {
    "zu_Zzzz_Zz",
    "zu_Latn_ZA",
  }, {
    "ZU_Zz",
    "zu_Latn_ZA"
  }, {
    "zu_LATN",
    "zu_Latn_ZA"
  }, {
    "en_Zz",
    "en_Latn_US"
  }, {
    "en_us",
    "en_Latn_US"
  }, {
    "en_Kore",
    "en_Kore_US"
  }, {
    "en_Kore_Zz",
    "en_Kore_US"
  }, {
    "en_Kore_ZA",
    "en_Kore_ZA"
  }, {
    "en_Kore_ZA_POSIX",
    "en_Kore_ZA_POSIX"
  }, {
    "en_Gujr",
    "en_Gujr_US"
  }, {
    "en_ZA",
    "en_Latn_ZA"
  }, {
    "en_Gujr_Zz",
    "en_Gujr_US"
  }, {
    "en_Gujr_ZA",
    "en_Gujr_ZA"
  }, {
    "en_Gujr_ZA_POSIX",
    "en_Gujr_ZA_POSIX"
  }, {
    "en_US_POSIX_1901",
    "en_Latn_US_POSIX_1901"
  }, {
    "en_Latn__POSIX_1901",
    "en_Latn_US_POSIX_1901"
  }, {
    "en__POSIX_1901",
    "en_Latn_US_POSIX_1901"
  }, {
    "de__POSIX_1901",
    "de_Latn_DE_POSIX_1901"
  }, {
    "en_US_BOSTON",
    "en_Latn_US_BOSTON"
  }, {
    "th@calendar=buddhist",
    "th_Thai_TH@calendar=buddhist"
  }, {
    "ar_ZZ",
    "ar_Arab_EG"
  }, {
    "zh",
    "zh_Hans_CN"
  }, {
    "zh_TW",
    "zh_Hant_TW"
  }, {
    "zh_HK",
    "zh_Hant_HK"
  }, {
    "zh_Hant",
    "zh_Hant_TW"
  }, {
    "zh_Zzzz_CN",
    "zh_Hans_CN"
  }, {
    "und_US",
    "en_Latn_US"
  }, {
    "und_HK",
    "zh_Hant_HK"
  }, {
    "zzz",
    ""
  }
};

const char* const basic_minimize_data[][2] = {
  {
    "en_Latn_US",
    "en"
  }, {
    "en_Latn_US_POSIX_1901",
    "en__POSIX_1901"
  }, {
    "EN_Latn_US_POSIX_1901",
    "en__POSIX_1901"
  }, {
    "en_Zzzz_US_POSIX_1901",
    "en__POSIX_1901"
  }, {
    "de_Latn_DE_POSIX_1901",
    "de__POSIX_1901"
  }, {
    "und",
    ""
  }
};

const char* const full_data[][3] = {
  {
    /*   "FROM", */
    /*   "ADD-LIKELY", */
    /*   "REMOVE-LIKELY" */
    /* }, { */
    "aa",
    "aa_Latn_ET",
    "aa"
  }, {
    "af",
    "af_Latn_ZA",
    "af"
  }, {
    "ak",
    "ak_Latn_GH",
    "ak"
  }, {
    "am",
    "am_Ethi_ET",
    "am"
  }, {
    "ar",
    "ar_Arab_EG",
    "ar"
  }, {
    "as",
    "as_Beng_IN",
    "as"
  }, {
    "az",
    "az_Latn_AZ",
    "az"
  }, {
    "be",
    "be_Cyrl_BY",
    "be"
  }, {
    "bg",
    "bg_Cyrl_BG",
    "bg"
  }, {
    "bn",
    "bn_Beng_BD",
    "bn"
  }, {
    "bo",
    "bo_Tibt_CN",
    "bo"
  }, {
    "bs",
    "bs_Latn_BA",
    "bs"
  }, {
    "ca",
    "ca_Latn_ES",
    "ca"
  }, {
    "ch",
    "ch_Latn_GU",
    "ch"
  }, {
    "chk",
    "chk_Latn_FM",
    "chk"
  }, {
    "cs",
    "cs_Latn_CZ",
    "cs"
  }, {
    "cy",
    "cy_Latn_GB",
    "cy"
  }, {
    "da",
    "da_Latn_DK",
    "da"
  }, {
    "de",
    "de_Latn_DE",
    "de"
  }, {
    "dv",
    "dv_Thaa_MV",
    "dv"
  }, {
    "dz",
    "dz_Tibt_BT",
    "dz"
  }, {
    "ee",
    "ee_Latn_GH",
    "ee"
  }, {
    "el",
    "el_Grek_GR",
    "el"
  }, {
    "en",
    "en_Latn_US",
    "en"
  }, {
    "es",
    "es_Latn_ES",
    "es"
  }, {
    "et",
    "et_Latn_EE",
    "et"
  }, {
    "eu",
    "eu_Latn_ES",
    "eu"
  }, {
    "fa",
    "fa_Arab_IR",
    "fa"
  }, {
    "fi",
    "fi_Latn_FI",
    "fi"
  }, {
    "fil",
    "fil_Latn_PH",
    "fil"
  }, {
    "fo",
    "fo_Latn_FO",
    "fo"
  }, {
    "fr",
    "fr_Latn_FR",
    "fr"
  }, {
    "fur",
    "fur_Latn_IT",
    "fur"
  }, {
    "ga",
    "ga_Latn_IE",
    "ga"
  }, {
    "gaa",
    "gaa_Latn_GH",
    "gaa"
  }, {
    "gl",
    "gl_Latn_ES",
    "gl"
  }, {
    "gn",
    "gn_Latn_PY",
    "gn"
  }, {
    "gu",
    "gu_Gujr_IN",
    "gu"
  }, {
    "ha",
    "ha_Latn_NG",
    "ha"
  }, {
    "haw",
    "haw_Latn_US",
    "haw"
  }, {
    "he",
    "he_Hebr_IL",
    "he"
  }, {
    "hi",
    "hi_Deva_IN",
    "hi"
  }, {
    "hr",
    "hr_Latn_HR",
    "hr"
  }, {
    "ht",
    "ht_Latn_HT",
    "ht"
  }, {
    "hu",
    "hu_Latn_HU",
    "hu"
  }, {
    "hy",
    "hy_Armn_AM",
    "hy"
  }, {
    "id",
    "id_Latn_ID",
    "id"
  }, {
    "ig",
    "ig_Latn_NG",
    "ig"
  }, {
    "ii",
    "ii_Yiii_CN",
    "ii"
  }, {
    "is",
    "is_Latn_IS",
    "is"
  }, {
    "it",
    "it_Latn_IT",
    "it"
  }, {
    "ja",
    "ja_Jpan_JP",
    "ja"
  }, {
    "ka",
    "ka_Geor_GE",
    "ka"
  }, {
    "kaj",
    "kaj_Latn_NG",
    "kaj"
  }, {
    "kam",
    "kam_Latn_KE",
    "kam"
  }, {
    "kk",
    "kk_Cyrl_KZ",
    "kk"
  }, {
    "kl",
    "kl_Latn_GL",
    "kl"
  }, {
    "km",
    "km_Khmr_KH",
    "km"
  }, {
    "kn",
    "kn_Knda_IN",
    "kn"
  }, {
    "ko",
    "ko_Kore_KR",
    "ko"
  }, {
    "kok",
    "kok_Deva_IN",
    "kok"
  }, {
    "kpe",
    "kpe_Latn_LR",
    "kpe"
  }, {
    "ku",
    "ku_Arab_IQ",
    "ku"
  }, {
    "ky",
    "ky_Cyrl_KG",
    "ky"
  }, {
    "la",
    "la_Latn_VA",
    "la"
  }, {
    "ln",
    "ln_Latn_CD",
    "ln"
  }, {
    "lo",
    "lo_Laoo_LA",
    "lo"
  }, {
    "lt",
    "lt_Latn_LT",
    "lt"
  }, {
    "lv",
    "lv_Latn_LV",
    "lv"
  }, {
    "mg",
    "mg_Latn_MG",
    "mg"
  }, {
    "mh",
    "mh_Latn_MH",
    "mh"
  }, {
    "mk",
    "mk_Cyrl_MK",
    "mk"
  }, {
    "ml",
    "ml_Mlym_IN",
    "ml"
  }, {
    "mn",
    "mn_Cyrl_MN",
    "mn"
  }, {
    "mr",
    "mr_Deva_IN",
    "mr"
  }, {
    "ms",
    "ms_Latn_MY",
    "ms"
  }, {
    "mt",
    "mt_Latn_MT",
    "mt"
  }, {
    "my",
    "my_Mymr_MM",
    "my"
  }, {
    "na",
    "na_Latn_NR",
    "na"
  }, {
    "ne",
    "ne_Deva_NP",
    "ne"
  }, {
    "niu",
    "niu_Latn_NU",
    "niu"
  }, {
    "nl",
    "nl_Latn_NL",
    "nl"
  }, {
    "nn",
    "nn_Latn_NO",
    "nn"
  }, {
    "nr",
    "nr_Latn_ZA",
    "nr"
  }, {
    "nso",
    "nso_Latn_ZA",
    "nso"
  }, {
    "ny",
    "ny_Latn_MW",
    "ny"
  }, {
    "om",
    "om_Latn_ET",
    "om"
  }, {
    "or",
    "or_Orya_IN",
    "or"
  }, {
    "pa",
    "pa_Guru_IN",
    "pa"
  }, {
    "pa_Arab",
    "pa_Arab_PK",
    "pa_PK"
  }, {
    "pa_PK",
    "pa_Arab_PK",
    "pa_PK"
  }, {
    "pap",
    "pap_Latn_AN",
    "pap"
  }, {
    "pau",
    "pau_Latn_PW",
    "pau"
  }, {
    "pl",
    "pl_Latn_PL",
    "pl"
  }, {
    "ps",
    "ps_Arab_AF",
    "ps"
  }, {
    "pt",
    "pt_Latn_BR",
    "pt"
  }, {
    "rn",
    "rn_Latn_BI",
    "rn"
  }, {
    "ro",
    "ro_Latn_RO",
    "ro"
  }, {
    "ru",
    "ru_Cyrl_RU",
    "ru"
  }, {
    "rw",
    "rw_Latn_RW",
    "rw"
  }, {
    "sa",
    "sa_Deva_IN",
    "sa"
  }, {
    "se",
    "se_Latn_NO",
    "se"
  }, {
    "sg",
    "sg_Latn_CF",
    "sg"
  }, {
    "si",
    "si_Sinh_LK",
    "si"
  }, {
    "sid",
    "sid_Latn_ET",
    "sid"
  }, {
    "sk",
    "sk_Latn_SK",
    "sk"
  }, {
    "sl",
    "sl_Latn_SI",
    "sl"
  }, {
    "sm",
    "sm_Latn_WS",
    "sm"
  }, {
    "so",
    "so_Latn_SO",
    "so"
  }, {
    "sq",
    "sq_Latn_AL",
    "sq"
  }, {
    "sr",
    "sr_Cyrl_RS",
    "sr"
  }, {
    "ss",
    "ss_Latn_ZA",
    "ss"
  }, {
    "st",
    "st_Latn_ZA",
    "st"
  }, {
    "sv",
    "sv_Latn_SE",
    "sv"
  }, {
    "sw",
    "sw_Latn_TZ",
    "sw"
  }, {
    "ta",
    "ta_Taml_IN",
    "ta"
  }, {
    "te",
    "te_Telu_IN",
    "te"
  }, {
    "tet",
    "tet_Latn_TL",
    "tet"
  }, {
    "tg",
    "tg_Cyrl_TJ",
    "tg"
  }, {
    "th",
    "th_Thai_TH",
    "th"
  }, {
    "ti",
    "ti_Ethi_ET",
    "ti"
  }, {
    "tig",
    "tig_Ethi_ER",
    "tig"
  }, {
    "tk",
    "tk_Latn_TM",
    "tk"
  }, {
    "tkl",
    "tkl_Latn_TK",
    "tkl"
  }, {
    "tn",
    "tn_Latn_ZA",
    "tn"
  }, {
    "to",
    "to_Latn_TO",
    "to"
  }, {
    "tpi",
    "tpi_Latn_PG",
    "tpi"
  }, {
    "tr",
    "tr_Latn_TR",
    "tr"
  }, {
    "ts",
    "ts_Latn_ZA",
    "ts"
  }, {
    "tt",
    "tt_Cyrl_RU",
    "tt"
  }, {
    "tvl",
    "tvl_Latn_TV",
    "tvl"
  }, {
    "ty",
    "ty_Latn_PF",
    "ty"
  }, {
    "uk",
    "uk_Cyrl_UA",
    "uk"
  }, {
    "und",
    "en_Latn_US",
    "en"
  }, {
    "und_AD",
    "ca_Latn_AD",
    "ca_AD"
  }, {
    "und_AE",
    "ar_Arab_AE",
    "ar_AE"
  }, {
    "und_AF",
    "fa_Arab_AF",
    "fa_AF"
  }, {
    "und_AL",
    "sq_Latn_AL",
    "sq"
  }, {
    "und_AM",
    "hy_Armn_AM",
    "hy"
  }, {
    "und_AN",
    "pap_Latn_AN",
    "pap"
  }, {
    "und_AO",
    "pt_Latn_AO",
    "pt_AO"
  }, {
    "und_AR",
    "es_Latn_AR",
    "es_AR"
  }, {
    "und_AS",
    "sm_Latn_AS",
    "sm_AS"
  }, {
    "und_AT",
    "de_Latn_AT",
    "de_AT"
  }, {
    "und_AW",
    "nl_Latn_AW",
    "nl_AW"
  }, {
    "und_AX",
    "sv_Latn_AX",
    "sv_AX"
  }, {
    "und_AZ",
    "az_Latn_AZ",
    "az"
  }, {
    "und_Arab",
    "ar_Arab_EG",
    "ar"
  }, {
    "und_Arab_IN",
    "ur_Arab_IN",
    "ur_IN"
  }, {
    "und_Arab_PK",
    "ur_Arab_PK",
    "ur"
  }, {
    "und_Arab_SN",
    "ar_Arab_SN",
    "ar_SN"
  }, {
    "und_Armn",
    "hy_Armn_AM",
    "hy"
  }, {
    "und_BA",
    "bs_Latn_BA",
    "bs"
  }, {
    "und_BD",
    "bn_Beng_BD",
    "bn"
  }, {
    "und_BE",
    "nl_Latn_BE",
    "nl_BE"
  }, {
    "und_BF",
    "fr_Latn_BF",
    "fr_BF"
  }, {
    "und_BG",
    "bg_Cyrl_BG",
    "bg"
  }, {
    "und_BH",
    "ar_Arab_BH",
    "ar_BH"
  }, {
    "und_BI",
    "rn_Latn_BI",
    "rn"
  }, {
    "und_BJ",
    "fr_Latn_BJ",
    "fr_BJ"
  }, {
    "und_BN",
    "ms_Latn_BN",
    "ms_BN"
  }, {
    "und_BO",
    "es_Latn_BO",
    "es_BO"
  }, {
    "und_BR",
    "pt_Latn_BR",
    "pt"
  }, {
    "und_BT",
    "dz_Tibt_BT",
    "dz"
  }, {
    "und_BY",
    "be_Cyrl_BY",
    "be"
  }, {
    "und_Beng",
    "bn_Beng_BD",
    "bn"
  }, {
    "und_Beng_IN",
    "bn_Beng_IN",
    "bn_IN"
  }, {
    "und_CD",
    "sw_Latn_CD",
    "sw_CD"
  }, {
    "und_CF",
    "fr_Latn_CF",
    "fr_CF"
  }, {
    "und_CG",
    "fr_Latn_CG",
    "fr_CG"
  }, {
    "und_CH",
    "de_Latn_CH",
    "de_CH"
  }, {
    "und_CI",
    "fr_Latn_CI",
    "fr_CI"
  }, {
    "und_CL",
    "es_Latn_CL",
    "es_CL"
  }, {
    "und_CM",
    "fr_Latn_CM",
    "fr_CM"
  }, {
    "und_CN",
    "zh_Hans_CN",
    "zh"
  }, {
    "und_CO",
    "es_Latn_CO",
    "es_CO"
  }, {
    "und_CR",
    "es_Latn_CR",
    "es_CR"
  }, {
    "und_CU",
    "es_Latn_CU",
    "es_CU"
  }, {
    "und_CV",
    "pt_Latn_CV",
    "pt_CV"
  }, {
    "und_CY",
    "el_Grek_CY",
    "el_CY"
  }, {
    "und_CZ",
    "cs_Latn_CZ",
    "cs"
  }, {
    "und_Cher",
    "chr_Cher_US",
    "chr"
  }, {
    "und_Cyrl",
    "ru_Cyrl_RU",
    "ru"
  }, {
    "und_Cyrl_KZ",
    "ru_Cyrl_KZ",
    "ru_KZ"
  }, {
    "und_DE",
    "de_Latn_DE",
    "de"
  }, {
    "und_DJ",
    "aa_Latn_DJ",
    "aa_DJ"
  }, {
    "und_DK",
    "da_Latn_DK",
    "da"
  }, {
    "und_DO",
    "es_Latn_DO",
    "es_DO"
  }, {
    "und_DZ",
    "ar_Arab_DZ",
    "ar_DZ"
  }, {
    "und_Deva",
    "hi_Deva_IN",
    "hi"
  }, {
    "und_EC",
    "es_Latn_EC",
    "es_EC"
  }, {
    "und_EE",
    "et_Latn_EE",
    "et"
  }, {
    "und_EG",
    "ar_Arab_EG",
    "ar"
  }, {
    "und_EH",
    "ar_Arab_EH",
    "ar_EH"
  }, {
    "und_ER",
    "ti_Ethi_ER",
    "ti_ER"
  }, {
    "und_ES",
    "es_Latn_ES",
    "es"
  }, {
    "und_ET",
    "en_Latn_ET",
    "en_ET"
  }, {
    "und_Ethi",
    "am_Ethi_ET",
    "am"
  }, {
    "und_Ethi_ER",
    "am_Ethi_ER",
    "am_ER"
  }, {
    "und_FI",
    "fi_Latn_FI",
    "fi"
  }, {
    "und_FM",
    "chk_Latn_FM",
    "chk"
  }, {
    "und_FO",
    "fo_Latn_FO",
    "fo"
  }, {
    "und_FR",
    "fr_Latn_FR",
    "fr"
  }, {
    "und_GA",
    "fr_Latn_GA",
    "fr_GA"
  }, {
    "und_GE",
    "ka_Geor_GE",
    "ka"
  }, {
    "und_GF",
    "fr_Latn_GF",
    "fr_GF"
  }, {
    "und_GL",
    "kl_Latn_GL",
    "kl"
  }, {
    "und_GN",
    "fr_Latn_GN",
    "fr_GN"
  }, {
    "und_GP",
    "fr_Latn_GP",
    "fr_GP"
  }, {
    "und_GQ",
    "es_Latn_GQ",
    "es_GQ"
  }, {
    "und_GR",
    "el_Grek_GR",
    "el"
  }, {
    "und_GT",
    "es_Latn_GT",
    "es_GT"
  }, {
    "und_GU",
    "en_Latn_GU",
    "en_GU"
  }, {
    "und_GW",
    "pt_Latn_GW",
    "pt_GW"
  }, {
    "und_Geor",
    "ka_Geor_GE",
    "ka"
  }, {
    "und_Grek",
    "el_Grek_GR",
    "el"
  }, {
    "und_Gujr",
    "gu_Gujr_IN",
    "gu"
  }, {
    "und_Guru",
    "pa_Guru_IN",
    "pa"
  }, {
    "und_HK",
    "zh_Hant_HK",
    "zh_HK"
  }, {
    "und_HN",
    "es_Latn_HN",
    "es_HN"
  }, {
    "und_HR",
    "hr_Latn_HR",
    "hr"
  }, {
    "und_HT",
    "ht_Latn_HT",
    "ht"
  }, {
    "und_HU",
    "hu_Latn_HU",
    "hu"
  }, {
    "und_Hani",
    "zh_Hans_CN",
    "zh"
  }, {
    "und_Hans",
    "zh_Hans_CN",
    "zh"
  }, {
    "und_Hant",
    "zh_Hant_TW",
    "zh_TW"
  }, {
    "und_Hebr",
    "he_Hebr_IL",
    "he"
  }, {
    "und_IL",
    "he_Hebr_IL",
    "he"
  }, {
    "und_IN",
    "hi_Deva_IN",
    "hi"
  }, {
    "und_IQ",
    "ar_Arab_IQ",
    "ar_IQ"
  }, {
    "und_IR",
    "fa_Arab_IR",
    "fa"
  }, {
    "und_IS",
    "is_Latn_IS",
    "is"
  }, {
    "und_IT",
    "it_Latn_IT",
    "it"
  }, {
    "und_JO",
    "ar_Arab_JO",
    "ar_JO"
  }, {
    "und_JP",
    "ja_Jpan_JP",
    "ja"
  }, {
    "und_Jpan",
    "ja_Jpan_JP",
    "ja"
  }, {
    "und_KG",
    "ky_Cyrl_KG",
    "ky"
  }, {
    "und_KH",
    "km_Khmr_KH",
    "km"
  }, {
    "und_KM",
    "ar_Arab_KM",
    "ar_KM"
  }, {
    "und_KP",
    "ko_Kore_KP",
    "ko_KP"
  }, {
    "und_KR",
    "ko_Kore_KR",
    "ko"
  }, {
    "und_KW",
    "ar_Arab_KW",
    "ar_KW"
  }, {
    "und_KZ",
    "ru_Cyrl_KZ",
    "ru_KZ"
  }, {
    "und_Khmr",
    "km_Khmr_KH",
    "km"
  }, {
    "und_Knda",
    "kn_Knda_IN",
    "kn"
  }, {
    "und_Kore",
    "ko_Kore_KR",
    "ko"
  }, {
    "und_LA",
    "lo_Laoo_LA",
    "lo"
  }, {
    "und_LB",
    "ar_Arab_LB",
    "ar_LB"
  }, {
    "und_LI",
    "de_Latn_LI",
    "de_LI"
  }, {
    "und_LK",
    "si_Sinh_LK",
    "si"
  }, {
    "und_LS",
    "st_Latn_LS",
    "st_LS"
  }, {
    "und_LT",
    "lt_Latn_LT",
    "lt"
  }, {
    "und_LU",
    "fr_Latn_LU",
    "fr_LU"
  }, {
    "und_LV",
    "lv_Latn_LV",
    "lv"
  }, {
    "und_LY",
    "ar_Arab_LY",
    "ar_LY"
  }, {
    "und_Laoo",
    "lo_Laoo_LA",
    "lo"
  }, {
    "und_Latn_ES",
    "es_Latn_ES",
    "es"
  }, {
    "und_Latn_ET",
    "en_Latn_ET",
    "en_ET"
  }, {
    "und_Latn_GB",
    "en_Latn_GB",
    "en_GB"
  }, {
    "und_Latn_GH",
    "ak_Latn_GH",
    "ak"
  }, {
    "und_Latn_ID",
    "id_Latn_ID",
    "id"
  }, {
    "und_Latn_IT",
    "it_Latn_IT",
    "it"
  }, {
    "und_Latn_NG",
    "en_Latn_NG",
    "en_NG"
  }, {
    "und_Latn_TR",
    "tr_Latn_TR",
    "tr"
  }, {
    "und_Latn_ZA",
    "en_Latn_ZA",
    "en_ZA"
  }, {
    "und_MA",
    "ar_Arab_MA",
    "ar_MA"
  }, {
    "und_MC",
    "fr_Latn_MC",
    "fr_MC"
  }, {
    "und_MD",
    "ro_Latn_MD",
    "ro_MD"
  }, {
    "und_ME",
    "sr_Latn_ME",
    "sr_ME"
  }, {
    "und_MG",
    "mg_Latn_MG",
    "mg"
  }, {
    "und_MH",
    "en_Latn_MH",
    "en_MH"
  }, {
    "und_MK",
    "mk_Cyrl_MK",
    "mk"
  }, {
    "und_ML",
    "bm_Latn_ML",
    "bm"
  }, {
    "und_MM",
    "my_Mymr_MM",
    "my"
  }, {
    "und_MN",
    "mn_Cyrl_MN",
    "mn"
  }, {
    "und_MO",
    "zh_Hant_MO",
    "zh_MO"
  }, {
    "und_MQ",
    "fr_Latn_MQ",
    "fr_MQ"
  }, {
    "und_MR",
    "ar_Arab_MR",
    "ar_MR"
  }, {
    "und_MT",
    "mt_Latn_MT",
    "mt"
  }, {
    "und_MV",
    "dv_Thaa_MV",
    "dv"
  }, {
    "und_MW",
    "en_Latn_MW",
    "en_MW"
  }, {
    "und_MX",
    "es_Latn_MX",
    "es_MX"
  }, {
    "und_MY",
    "ms_Latn_MY",
    "ms"
  }, {
    "und_MZ",
    "pt_Latn_MZ",
    "pt_MZ"
  }, {
    "und_Mlym",
    "ml_Mlym_IN",
    "ml"
  }, {
    "und_Mymr",
    "my_Mymr_MM",
    "my"
  }, {
    "und_NC",
    "fr_Latn_NC",
    "fr_NC"
  }, {
    "und_NE",
    "ha_Latn_NE",
    "ha_NE"
  }, {
    "und_NG",
    "en_Latn_NG",
    "en_NG"
  }, {
    "und_NI",
    "es_Latn_NI",
    "es_NI"
  }, {
    "und_NL",
    "nl_Latn_NL",
    "nl"
  }, {
    "und_NO",
    "nb_Latn_NO",
    "nb"
  }, {
    "und_NP",
    "ne_Deva_NP",
    "ne"
  }, {
    "und_NR",
    "en_Latn_NR",
    "en_NR"
  }, {
    "und_NU",
    "en_Latn_NU",
    "en_NU"
  }, {
    "und_OM",
    "ar_Arab_OM",
    "ar_OM"
  }, {
    "und_Orya",
    "or_Orya_IN",
    "or"
  }, {
    "und_PA",
    "es_Latn_PA",
    "es_PA"
  }, {
    "und_PE",
    "es_Latn_PE",
    "es_PE"
  }, {
    "und_PF",
    "fr_Latn_PF",
    "fr_PF"
  }, {
    "und_PG",
    "tpi_Latn_PG",
    "tpi"
  }, {
    "und_PH",
    "fil_Latn_PH",
    "fil"
  }, {
    "und_PL",
    "pl_Latn_PL",
    "pl"
  }, {
    "und_PM",
    "fr_Latn_PM",
    "fr_PM"
  }, {
    "und_PR",
    "es_Latn_PR",
    "es_PR"
  }, {
    "und_PS",
    "ar_Arab_PS",
    "ar_PS"
  }, {
    "und_PT",
    "pt_Latn_PT",
    "pt_PT"
  }, {
    "und_PW",
    "pau_Latn_PW",
    "pau"
  }, {
    "und_PY",
    "gn_Latn_PY",
    "gn"
  }, {
    "und_QA",
    "ar_Arab_QA",
    "ar_QA"
  }, {
    "und_RE",
    "fr_Latn_RE",
    "fr_RE"
  }, {
    "und_RO",
    "ro_Latn_RO",
    "ro"
  }, {
    "und_RS",
    "sr_Cyrl_RS",
    "sr"
  }, {
    "und_RU",
    "ru_Cyrl_RU",
    "ru"
  }, {
    "und_RW",
    "rw_Latn_RW",
    "rw"
  }, {
    "und_SA",
    "ar_Arab_SA",
    "ar_SA"
  }, {
    "und_SD",
    "ar_Arab_SD",
    "ar_SD"
  }, {
    "und_SE",
    "sv_Latn_SE",
    "sv"
  }, {
    "und_SG",
    "en_Latn_SG",
    "en_SG"
  }, {
    "und_SI",
    "sl_Latn_SI",
    "sl"
  }, {
    "und_SJ",
    "nb_Latn_SJ",
    "nb_SJ"
  }, {
    "und_SK",
    "sk_Latn_SK",
    "sk"
  }, {
    "und_SM",
    "it_Latn_SM",
    "it_SM"
  }, {
    "und_SN",
    "fr_Latn_SN",
    "fr_SN"
  }, {
    "und_SO",
    "so_Latn_SO",
    "so"
  }, {
    "und_SR",
    "nl_Latn_SR",
    "nl_SR"
  }, {
    "und_ST",
    "pt_Latn_ST",
    "pt_ST"
  }, {
    "und_SV",
    "es_Latn_SV",
    "es_SV"
  }, {
    "und_SY",
    "ar_Arab_SY",
    "ar_SY"
  }, {
    "und_Sinh",
    "si_Sinh_LK",
    "si"
  }, {
    "und_TD",
    "fr_Latn_TD",
    "fr_TD"
  }, {
    "und_TG",
    "fr_Latn_TG",
    "fr_TG"
  }, {
    "und_TH",
    "th_Thai_TH",
    "th"
  }, {
    "und_TJ",
    "tg_Cyrl_TJ",
    "tg"
  }, {
    "und_TK",
    "tkl_Latn_TK",
    "tkl"
  }, {
    "und_TL",
    "pt_Latn_TL",
    "pt_TL"
  }, {
    "und_TM",
    "tk_Latn_TM",
    "tk"
  }, {
    "und_TN",
    "ar_Arab_TN",
    "ar_TN"
  }, {
    "und_TO",
    "to_Latn_TO",
    "to"
  }, {
    "und_TR",
    "tr_Latn_TR",
    "tr"
  }, {
    "und_TV",
    "tvl_Latn_TV",
    "tvl"
  }, {
    "und_TW",
    "zh_Hant_TW",
    "zh_TW"
  }, {
    "und_Taml",
    "ta_Taml_IN",
    "ta"
  }, {
    "und_Telu",
    "te_Telu_IN",
    "te"
  }, {
    "und_Thaa",
    "dv_Thaa_MV",
    "dv"
  }, {
    "und_Thai",
    "th_Thai_TH",
    "th"
  }, {
    "und_Tibt",
    "bo_Tibt_CN",
    "bo"
  }, {
    "und_UA",
    "uk_Cyrl_UA",
    "uk"
  }, {
    "und_UY",
    "es_Latn_UY",
    "es_UY"
  }, {
    "und_UZ",
    "uz_Cyrl_UZ",
    "uz"
  }, {
    "und_VA",
    "la_Latn_VA",
    "la"
  }, {
    "und_VE",
    "es_Latn_VE",
    "es_VE"
  }, {
    "und_VN",
    "vi_Latn_VN",
    "vi"
  }, {
    "und_VU",
    "fr_Latn_VU",
    "fr_VU"
  }, {
    "und_WF",
    "fr_Latn_WF",
    "fr_WF"
  }, {
    "und_WS",
    "sm_Latn_WS",
    "sm"
  }, {
    "und_YE",
    "ar_Arab_YE",
    "ar_YE"
  }, {
    "und_YT",
    "fr_Latn_YT",
    "fr_YT"
  }, {
    "und_Yiii",
    "ii_Yiii_CN",
    "ii"
  }, {
    "ur",
    "ur_Arab_PK",
    "ur"
  }, {
    "uz",
    "uz_Cyrl_UZ",
    "uz"
  }, {
    "uz_AF",
    "uz_Arab_AF",
    "uz_AF"
  }, {
    "uz_Arab",
    "uz_Arab_AF",
    "uz_AF"
  }, {
    "ve",
    "ve_Latn_ZA",
    "ve"
  }, {
    "vi",
    "vi_Latn_VN",
    "vi"
  }, {
    "wal",
    "wal_Ethi_ET",
    "wal"
  }, {
    "wo",
    "wo_Latn_SN",
    "wo"
  }, {
    "xh",
    "xh_Latn_ZA",
    "xh"
  }, {
    "yo",
    "yo_Latn_NG",
    "yo"
  }, {
    "zh",
    "zh_Hans_CN",
    "zh"
  }, {
    "zh_HK",
    "zh_Hant_HK",
    "zh_HK"
  }, {
    "zh_Hani",
    "zh_Hans_CN",
    "zh"
  }, {
    "zh_Hant",
    "zh_Hant_TW",
    "zh_TW"
  }, {
    "zh_MO",
    "zh_Hant_MO",
    "zh_MO"
  }, {
    "zh_TW",
    "zh_Hant_TW",
    "zh_TW"
  }, {
    "zu",
    "zu_Latn_ZA",
    "zu"
  }, {
    "und",
    "en_Latn_US",
    "en"
  }, {
    "und_ZZ",
    "en_Latn_US",
    "en"
  }, {
    "und_CN",
    "zh_Hans_CN",
    "zh"
  }, {
    "und_TW",
    "zh_Hant_TW",
    "zh_TW"
  }, {
    "und_HK",
    "zh_Hant_HK",
    "zh_HK"
  }, {
    "und_AQ",
    "en_Latn_AQ",
    "en_AQ"
  }, {
    "und_Zzzz",
    "en_Latn_US",
    "en"
  }, {
    "und_Zzzz_ZZ",
    "en_Latn_US",
    "en"
  }, {
    "und_Zzzz_CN",
    "zh_Hans_CN",
    "zh"
  }, {
    "und_Zzzz_TW",
    "zh_Hant_TW",
    "zh_TW"
  }, {
    "und_Zzzz_HK",
    "zh_Hant_HK",
    "zh_HK"
  }, {
    "und_Zzzz_AQ",
    "en_Latn_AQ",
    "en_AQ"
  }, {
    "und_Latn",
    "en_Latn_US",
    "en"
  }, {
    "und_Latn_ZZ",
    "en_Latn_US",
    "en"
  }, {
    "und_Latn_CN",
    "za_Latn_CN",
    "za"
  }, {
    "und_Latn_TW",
    "zh_Latn_TW",
    "zh_Latn_TW"
  }, {
    "und_Latn_HK",
    "zh_Latn_HK",
    "zh_Latn_HK"
  }, {
    "und_Latn_AQ",
    "en_Latn_AQ",
    "en_AQ"
  }, {
    "und_Hans",
    "zh_Hans_CN",
    "zh"
  }, {
    "und_Hans_ZZ",
    "zh_Hans_CN",
    "zh"
  }, {
    "und_Hans_CN",
    "zh_Hans_CN",
    "zh"
  }, {
    "und_Hans_TW",
    "zh_Hans_TW",
    "zh_Hans_TW"
  }, {
    "und_Hans_HK",
    "zh_Hans_HK",
    "zh_Hans_HK"
  }, {
    "und_Hans_AQ",
    "zh_Hans_AQ",
    "zh_AQ"
  }, {
    "und_Hant",
    "zh_Hant_TW",
    "zh_TW"
  }, {
    "und_Hant_ZZ",
    "zh_Hant_TW",
    "zh_TW"
  }, {
    "und_Hant_CN",
    "zh_Hant_CN",
    "zh_Hant_CN"
  }, {
    "und_Hant_TW",
    "zh_Hant_TW",
    "zh_TW"
  }, {
    "und_Hant_HK",
    "zh_Hant_HK",
    "zh_HK"
  }, {
    "und_Hant_AQ",
    "zh_Hant_AQ",
    "zh_Hant_AQ"
  }, {
    "und_Moon",
    "en_Moon_US",
    "en_Moon"
  }, {
    "und_Moon_ZZ",
    "en_Moon_US",
    "en_Moon"
  }, {
    "und_Moon_CN",
    "zh_Moon_CN",
    "zh_Moon"
  }, {
    "und_Moon_TW",
    "zh_Moon_TW",
    "zh_Moon_TW"
  }, {
    "und_Moon_HK",
    "zh_Moon_HK",
    "zh_Moon_HK"
  }, {
    "und_Moon_AQ",
    "en_Moon_AQ",
    "en_Moon_AQ"
  }, {
    "es",
    "es_Latn_ES",
    "es"
  }, {
    "es_ZZ",
    "es_Latn_ES",
    "es"
  }, {
    "es_CN",
    "es_Latn_CN",
    "es_CN"
  }, {
    "es_TW",
    "es_Latn_TW",
    "es_TW"
  }, {
    "es_HK",
    "es_Latn_HK",
    "es_HK"
  }, {
    "es_AQ",
    "es_Latn_AQ",
    "es_AQ"
  }, {
    "es_Zzzz",
    "es_Latn_ES",
    "es"
  }, {
    "es_Zzzz_ZZ",
    "es_Latn_ES",
    "es"
  }, {
    "es_Zzzz_CN",
    "es_Latn_CN",
    "es_CN"
  }, {
    "es_Zzzz_TW",
    "es_Latn_TW",
    "es_TW"
  }, {
    "es_Zzzz_HK",
    "es_Latn_HK",
    "es_HK"
  }, {
    "es_Zzzz_AQ",
    "es_Latn_AQ",
    "es_AQ"
  }, {
    "es_Latn",
    "es_Latn_ES",
    "es"
  }, {
    "es_Latn_ZZ",
    "es_Latn_ES",
    "es"
  }, {
    "es_Latn_CN",
    "es_Latn_CN",
    "es_CN"
  }, {
    "es_Latn_TW",
    "es_Latn_TW",
    "es_TW"
  }, {
    "es_Latn_HK",
    "es_Latn_HK",
    "es_HK"
  }, {
    "es_Latn_AQ",
    "es_Latn_AQ",
    "es_AQ"
  }, {
    "es_Hans",
    "es_Hans_ES",
    "es_Hans"
  }, {
    "es_Hans_ZZ",
    "es_Hans_ES",
    "es_Hans"
  }, {
    "es_Hans_CN",
    "es_Hans_CN",
    "es_Hans_CN"
  }, {
    "es_Hans_TW",
    "es_Hans_TW",
    "es_Hans_TW"
  }, {
    "es_Hans_HK",
    "es_Hans_HK",
    "es_Hans_HK"
  }, {
    "es_Hans_AQ",
    "es_Hans_AQ",
    "es_Hans_AQ"
  }, {
    "es_Hant",
    "es_Hant_ES",
    "es_Hant"
  }, {
    "es_Hant_ZZ",
    "es_Hant_ES",
    "es_Hant"
  }, {
    "es_Hant_CN",
    "es_Hant_CN",
    "es_Hant_CN"
  }, {
    "es_Hant_TW",
    "es_Hant_TW",
    "es_Hant_TW"
  }, {
    "es_Hant_HK",
    "es_Hant_HK",
    "es_Hant_HK"
  }, {
    "es_Hant_AQ",
    "es_Hant_AQ",
    "es_Hant_AQ"
  }, {
    "es_Moon",
    "es_Moon_ES",
    "es_Moon"
  }, {
    "es_Moon_ZZ",
    "es_Moon_ES",
    "es_Moon"
  }, {
    "es_Moon_CN",
    "es_Moon_CN",
    "es_Moon_CN"
  }, {
    "es_Moon_TW",
    "es_Moon_TW",
    "es_Moon_TW"
  }, {
    "es_Moon_HK",
    "es_Moon_HK",
    "es_Moon_HK"
  }, {
    "es_Moon_AQ",
    "es_Moon_AQ",
    "es_Moon_AQ"
  }, {
    "zh",
    "zh_Hans_CN",
    "zh"
  }, {
    "zh_ZZ",
    "zh_Hans_CN",
    "zh"
  }, {
    "zh_CN",
    "zh_Hans_CN",
    "zh"
  }, {
    "zh_TW",
    "zh_Hant_TW",
    "zh_TW"
  }, {
    "zh_HK",
    "zh_Hant_HK",
    "zh_HK"
  }, {
    "zh_AQ",
    "zh_Hans_AQ",
    "zh_AQ"
  }, {
    "zh_Zzzz",
    "zh_Hans_CN",
    "zh"
  }, {
    "zh_Zzzz_ZZ",
    "zh_Hans_CN",
    "zh"
  }, {
    "zh_Zzzz_CN",
    "zh_Hans_CN",
    "zh"
  }, {
    "zh_Zzzz_TW",
    "zh_Hant_TW",
    "zh_TW"
  }, {
    "zh_Zzzz_HK",
    "zh_Hant_HK",
    "zh_HK"
  }, {
    "zh_Zzzz_AQ",
    "zh_Hans_AQ",
    "zh_AQ"
  }, {
    "zh_Latn",
    "zh_Latn_CN",
    "zh_Latn"
  }, {
    "zh_Latn_ZZ",
    "zh_Latn_CN",
    "zh_Latn"
  }, {
    "zh_Latn_CN",
    "zh_Latn_CN",
    "zh_Latn"
  }, {
    "zh_Latn_TW",
    "zh_Latn_TW",
    "zh_Latn_TW"
  }, {
    "zh_Latn_HK",
    "zh_Latn_HK",
    "zh_Latn_HK"
  }, {
    "zh_Latn_AQ",
    "zh_Latn_AQ",
    "zh_Latn_AQ"
  }, {
    "zh_Hans",
    "zh_Hans_CN",
    "zh"
  }, {
    "zh_Hans_ZZ",
    "zh_Hans_CN",
    "zh"
  }, {
    "zh_Hans_TW",
    "zh_Hans_TW",
    "zh_Hans_TW"
  }, {
    "zh_Hans_HK",
    "zh_Hans_HK",
    "zh_Hans_HK"
  }, {
    "zh_Hans_AQ",
    "zh_Hans_AQ",
    "zh_AQ"
  }, {
    "zh_Hant",
    "zh_Hant_TW",
    "zh_TW"
  }, {
    "zh_Hant_ZZ",
    "zh_Hant_TW",
    "zh_TW"
  }, {
    "zh_Hant_CN",
    "zh_Hant_CN",
    "zh_Hant_CN"
  }, {
    "zh_Hant_AQ",
    "zh_Hant_AQ",
    "zh_Hant_AQ"
  }, {
    "zh_Moon",
    "zh_Moon_CN",
    "zh_Moon"
  }, {
    "zh_Moon_ZZ",
    "zh_Moon_CN",
    "zh_Moon"
  }, {
    "zh_Moon_CN",
    "zh_Moon_CN",
    "zh_Moon"
  }, {
    "zh_Moon_TW",
    "zh_Moon_TW",
    "zh_Moon_TW"
  }, {
    "zh_Moon_HK",
    "zh_Moon_HK",
    "zh_Moon_HK"
  }, {
    "zh_Moon_AQ",
    "zh_Moon_AQ",
    "zh_Moon_AQ"
  }, {
    "art",
    "",
    ""
  }, {
    "art_ZZ",
    "",
    ""
  }, {
    "art_CN",
    "",
    ""
  }, {
    "art_TW",
    "",
    ""
  }, {
    "art_HK",
    "",
    ""
  }, {
    "art_AQ",
    "",
    ""
  }, {
    "art_Zzzz",
    "",
    ""
  }, {
    "art_Zzzz_ZZ",
    "",
    ""
  }, {
    "art_Zzzz_CN",
    "",
    ""
  }, {
    "art_Zzzz_TW",
    "",
    ""
  }, {
    "art_Zzzz_HK",
    "",
    ""
  }, {
    "art_Zzzz_AQ",
    "",
    ""
  }, {
    "art_Latn",
    "",
    ""
  }, {
    "art_Latn_ZZ",
    "",
    ""
  }, {
    "art_Latn_CN",
    "",
    ""
  }, {
    "art_Latn_TW",
    "",
    ""
  }, {
    "art_Latn_HK",
    "",
    ""
  }, {
    "art_Latn_AQ",
    "",
    ""
  }, {
    "art_Hans",
    "",
    ""
  }, {
    "art_Hans_ZZ",
    "",
    ""
  }, {
    "art_Hans_CN",
    "",
    ""
  }, {
    "art_Hans_TW",
    "",
    ""
  }, {
    "art_Hans_HK",
    "",
    ""
  }, {
    "art_Hans_AQ",
    "",
    ""
  }, {
    "art_Hant",
    "",
    ""
  }, {
    "art_Hant_ZZ",
    "",
    ""
  }, {
    "art_Hant_CN",
    "",
    ""
  }, {
    "art_Hant_TW",
    "",
    ""
  }, {
    "art_Hant_HK",
    "",
    ""
  }, {
    "art_Hant_AQ",
    "",
    ""
  }, {
    "art_Moon",
    "",
    ""
  }, {
    "art_Moon_ZZ",
    "",
    ""
  }, {
    "art_Moon_CN",
    "",
    ""
  }, {
    "art_Moon_TW",
    "",
    ""
  }, {
    "art_Moon_HK",
    "",
    ""
  }, {
    "art_Moon_AQ",
    "",
    ""
  }
};

typedef struct errorDataTag {
    const char* tag;
    const char* expected;
    UErrorCode uerror;
    int32_t  bufferSize;
} errorData;

const errorData maximizeErrors[] = {
    {
        "enfueiujhytdf",
        NULL,
        U_ILLEGAL_ARGUMENT_ERROR,
        -1
    },
    {
        "en_THUJIOGIURJHGJFURYHFJGURYYYHHGJURHG",
        NULL,
        U_ILLEGAL_ARGUMENT_ERROR,
        -1
    },
    {
        "en_THUJIOGIURJHGJFURYHFJGURYYYHHGJURHG",
        NULL,
        U_ILLEGAL_ARGUMENT_ERROR,
        -1
    },
    {
        "en_Latn_US_POSIX@currency=EURO",
        "en_Latn_US_POSIX@currency=EURO",
        U_BUFFER_OVERFLOW_ERROR,
        29
    },
    {
        "en_Latn_US_POSIX@currency=EURO",
        "en_Latn_US_POSIX@currency=EURO",
        U_STRING_NOT_TERMINATED_WARNING,
        30
    }
};

const errorData minimizeErrors[] = {
    {
        "enfueiujhytdf",
        NULL,
        U_ILLEGAL_ARGUMENT_ERROR,
        -1
    },
    {
        "en_THUJIOGIURJHGJFURYHFJGURYYYHHGJURHG",
        NULL,
        U_ILLEGAL_ARGUMENT_ERROR,
        -1
    },
    {
        "en_Latn_US_POSIX@currency=EURO",
        "en__POSIX@currency=EURO",
        U_BUFFER_OVERFLOW_ERROR,
        22
    },
    {
        "en_Latn_US_POSIX@currency=EURO",
        "en__POSIX@currency=EURO",
        U_STRING_NOT_TERMINATED_WARNING,
        23
    }
};

static int32_t getExpectedReturnValue(const errorData* data)
{
    if (data->uerror == U_BUFFER_OVERFLOW_ERROR ||
        data->uerror == U_STRING_NOT_TERMINATED_WARNING)
    {
        return strlen(data->expected);
    }
    else
    {
        return -1;
    }
}

static int32_t getBufferSize(const errorData* data, int32_t actualSize)
{
    if (data->expected == NULL)
    {
        return actualSize;
    }
    else if (data->bufferSize < 0)
    {
        return strlen(data->expected) + 1;
    }
    else
    {
        return data->bufferSize;
    }
}

static void TestLikelySubtags()
{
    char buffer[ULOC_FULLNAME_CAPACITY + ULOC_KEYWORD_AND_VALUES_CAPACITY + 1];
    int32_t i = 0;

    for (; i < sizeof(basic_maximize_data) / sizeof(basic_maximize_data[0]); ++i)
    {
        UErrorCode status = U_ZERO_ERROR;
        const char* const minimal = basic_maximize_data[i][0];
        const char* const maximal = basic_maximize_data[i][1];

        /* const int32_t length = */
            uloc_addLikelySubtags(
                minimal,
                buffer,
                sizeof(buffer),
                &status);
        if (U_FAILURE(status)) {
            log_err_status(status, "  unexpected failure of uloc_addLikelySubtags(), minimal \"%s\" status %s\n", minimal, u_errorName(status));
            status = U_ZERO_ERROR;
        }
        else if (uprv_strlen(maximal) == 0) {
            if (uprv_stricmp(minimal, buffer) != 0) {
                log_err("  unexpected maximal value \"%s\" in uloc_addLikelySubtags(), minimal \"%s\" = \"%s\"\n", maximal, minimal, buffer);
            }
        }
        else if (uprv_stricmp(maximal, buffer) != 0) {
            log_err("  maximal doesn't match expected %s in uloc_addLikelySubtags(), minimal \"%s\" = %s\n", maximal, minimal, buffer);
        }
    }

    for (i = 0; i < sizeof(basic_minimize_data) / sizeof(basic_minimize_data[0]); ++i) {

        UErrorCode status = U_ZERO_ERROR;
        const char* const maximal = basic_minimize_data[i][0];
        const char* const minimal = basic_minimize_data[i][1];

        /* const int32_t length = */
            uloc_minimizeSubtags(
                maximal,
                buffer,
                sizeof(buffer),
                &status);

        if (U_FAILURE(status)) {
            log_err_status(status, "  unexpected failure of uloc_MinimizeSubtags(), maximal \"%s\" status %s\n", maximal, u_errorName(status));
            status = U_ZERO_ERROR;
        }
        else if (uprv_strlen(minimal) == 0) {
            if (uprv_stricmp(maximal, buffer) != 0) {
                log_err("  unexpected minimal value \"%s\" in uloc_minimizeSubtags(), maximal \"%s\" = \"%s\"\n", minimal, maximal, buffer);
            }
        }
        else if (uprv_stricmp(minimal, buffer) != 0) {
            log_err("  minimal doesn't match expected %s in uloc_MinimizeSubtags(), maximal \"%s\" = %s\n", minimal, maximal, buffer);
        }
    }

    for (i = 0; i < sizeof(full_data) / sizeof(full_data[0]); ++i) {

        UErrorCode status = U_ZERO_ERROR;
        const char* const minimal = full_data[i][0];
        const char* const maximal = full_data[i][1];

        /* const int32_t length = */
            uloc_addLikelySubtags(
                minimal,
                buffer,
                sizeof(buffer),
                &status);
        if (U_FAILURE(status)) {
            log_err_status(status, "  unexpected failure of uloc_addLikelySubtags(), minimal \"%s\" status \"%s\"\n", minimal, u_errorName(status));
            status = U_ZERO_ERROR;
        }
        else if (uprv_strlen(maximal) == 0) {
            if (uprv_stricmp(minimal, buffer) != 0) {
                log_err("  unexpected maximal value \"%s\" in uloc_addLikelySubtags(), minimal \"%s\" = \"%s\"\n", maximal, minimal, buffer);
            }
        }
        else if (uprv_stricmp(maximal, buffer) != 0) {
            log_err("  maximal doesn't match expected \"%s\" in uloc_addLikelySubtags(), minimal \"%s\" = \"%s\"\n", maximal, minimal, buffer);
        }
    }

    for (i = 0; i < sizeof(full_data) / sizeof(full_data[0]); ++i) {

        UErrorCode status = U_ZERO_ERROR;
        const char* const maximal = full_data[i][1];
        const char* const minimal = full_data[i][2];

        if (strlen(maximal) > 0) {

            /* const int32_t length = */
                uloc_minimizeSubtags(
                    maximal,
                    buffer,
                    sizeof(buffer),
                    &status);

            if (U_FAILURE(status)) {
                log_err_status(status, "  unexpected failure of uloc_minimizeSubtags(), maximal \"%s\" status %s\n", maximal, u_errorName(status));
                status = U_ZERO_ERROR;
            }
            else if (uprv_strlen(minimal) == 0) {
                if (uprv_stricmp(maximal, buffer) != 0) {
                    log_err("  unexpected minimal value \"%s\" in uloc_minimizeSubtags(), maximal \"%s\" = \"%s\"\n", minimal, maximal, buffer);
                }
            }
            else if (uprv_stricmp(minimal, buffer) != 0) {
                log_err("  minimal doesn't match expected %s in uloc_MinimizeSubtags(), maximal \"%s\" = %s\n", minimal, maximal, buffer);
            }
        }
    }

    for (i = 0; i < sizeof(maximizeErrors) / sizeof(maximizeErrors[0]); ++i) {

        UErrorCode status = U_ZERO_ERROR;
        const char* const minimal = maximizeErrors[i].tag;
        const char* const maximal = maximizeErrors[i].expected;
        const UErrorCode expectedStatus = maximizeErrors[i].uerror;
        const int32_t expectedLength = getExpectedReturnValue(&maximizeErrors[i]);
        const int32_t bufferSize = getBufferSize(&maximizeErrors[i], sizeof(buffer));

        const int32_t length =
            uloc_addLikelySubtags(
                minimal,
                buffer,
                bufferSize,
                &status);

        if (status == U_ZERO_ERROR) {
            log_err("  unexpected U_ZERO_ERROR for uloc_addLikelySubtags(), minimal \"%s\" expected status %s\n", minimal, u_errorName(expectedStatus));
            status = U_ZERO_ERROR;
        }
        else if (status != expectedStatus) {
            log_err_status(status, "  unexpected status for uloc_addLikelySubtags(), minimal \"%s\" expected status %s, but got %s\n", minimal, u_errorName(expectedStatus), u_errorName(status));
        }
        else if (length != expectedLength) {
            log_err("  unexpected length for uloc_addLikelySubtags(), minimal \"%s\" expected length %d, but got %d\n", minimal, expectedLength, length);
        }
        else if (status == U_BUFFER_OVERFLOW_ERROR || status == U_STRING_NOT_TERMINATED_WARNING) {
            if (uprv_strnicmp(maximal, buffer, bufferSize) != 0) {
                log_err("  maximal doesn't match expected %s in uloc_addLikelySubtags(), minimal \"%s\" = %*s\n",
                    maximal, minimal, (int)sizeof(buffer), buffer);
            }
        }
    }

    for (i = 0; i < sizeof(minimizeErrors) / sizeof(minimizeErrors[0]); ++i) {

        UErrorCode status = U_ZERO_ERROR;
        const char* const maximal = minimizeErrors[i].tag;
        const char* const minimal = minimizeErrors[i].expected;
        const UErrorCode expectedStatus = minimizeErrors[i].uerror;
        const int32_t expectedLength = getExpectedReturnValue(&minimizeErrors[i]);
        const int32_t bufferSize = getBufferSize(&minimizeErrors[i], sizeof(buffer));

        const int32_t length =
            uloc_minimizeSubtags(
                maximal,
                buffer,
                bufferSize,
                &status);

        if (status == U_ZERO_ERROR) {
            log_err("  unexpected U_ZERO_ERROR for uloc_minimizeSubtags(), maximal \"%s\" expected status %s\n", maximal, u_errorName(expectedStatus));
            status = U_ZERO_ERROR;
        }
        else if (status != expectedStatus) {
            log_err_status(status, "  unexpected status for uloc_minimizeSubtags(), maximal \"%s\" expected status %s, but got %s\n", maximal, u_errorName(expectedStatus), u_errorName(status));
        }
        else if (length != expectedLength) {
            log_err("  unexpected length for uloc_minimizeSubtags(), maximal \"%s\" expected length %d, but got %d\n", maximal, expectedLength, length);
        }
        else if (status == U_BUFFER_OVERFLOW_ERROR || status == U_STRING_NOT_TERMINATED_WARNING) {
            if (uprv_strnicmp(minimal, buffer, bufferSize) != 0) {
                log_err("  minimal doesn't match expected \"%s\" in uloc_minimizeSubtags(), minimal \"%s\" = \"%*s\"\n",
                    minimal, maximal, (int)sizeof(buffer), buffer);
            }
        }
    }
}

const char* const locale_to_langtag[][3] = {
    {"",            "und",          "und"},
    {"en",          "en",           "en"},
    {"en_US",       "en-US",        "en-US"},
    {"iw_IL",       "he-IL",        "he-IL"},
    {"sr_Latn_SR",  "sr-Latn-SR",   "sr-Latn-SR"},
    {"en__POSIX",   "en-u-va-posix", "en-u-va-posix"},
    {"en_POSIX",    "en-u-va-posix", "en-u-va-posix"},
    {"en_US_POSIX@ca=japanese",  "en-US-u-ca-japanese-va-posix", "en-US-u-ca-japanese-va-posix"},
    {"und_555",     "und-555",      "und-555"},
    {"123",         "und",          NULL},
    {"%$#&",        "und",          NULL},
    {"_Latn",       "und-Latn",     "und-Latn"},
    {"_DE",         "und-DE",       "und-DE"},
    {"und_FR",      "und-FR",       "und-FR"},
    {"th_TH_TH",    "th-TH",        NULL},
    {"bogus",       "bogus",        "bogus"},
    {"foooobarrr",  "und",          NULL},
    {"az_AZ_CYRL",  "az-Cyrl-AZ",   "az-Cyrl-AZ"},
    {"aa_BB_CYRL",  "aa-BB",        NULL},
    {"en_US_1234",  "en-US-1234",   "en-US-1234"},
    {"en_US_VARIANTA_VARIANTB", "en-US-varianta-variantb",  "en-US-varianta-variantb"},
    {"en_US_VARIANTB_VARIANTA", "en-US-varianta-variantb",  "en-US-varianta-variantb"},
    {"ja__9876_5432",   "ja-5432-9876", "ja-5432-9876"},
    {"zh_Hant__VAR",    "zh-Hant",  NULL},
    {"es__BADVARIANT_GOODVAR",  "es-goodvar",   NULL},
    {"en@calendar=gregorian",   "en-u-ca-gregory",  "en-u-ca-gregory"},
    {"de@collation=phonebook;calendar=gregorian",   "de-u-ca-gregory-co-phonebk",   "de-u-ca-gregory-co-phonebk"},
    {"th@numbers=thai;z=extz;x=priv-use;a=exta",   "th-a-exta-u-nu-thai-z-extz-x-priv-use", "th-a-exta-u-nu-thai-z-extz-x-priv-use"},
    {"en@timezone=America/New_York;calendar=japanese",    "en-u-ca-japanese-tz-usnyc",    "en-u-ca-japanese-tz-usnyc"},
    {"en@timezone=US/Eastern",  "en-u-tz-usnyc",    "en-u-tz-usnyc"},
    {"en@x=x-y-z;a=a-b-c",  "en-x-x-y-z",   NULL},
    {"it@collation=badcollationtype;colStrength=identical;cu=usd-eur", "it-u-ks-identic",  NULL},
    {"en_US_POSIX", "en-US-u-va-posix", "en-US-u-va-posix"},
    {"en_US_POSIX@calendar=japanese;currency=EUR","en-US-u-ca-japanese-cu-EUR-va-posix", "en-US-u-ca-japanese-cu-EUR-va-posix"},

    {NULL,          NULL,           NULL}
};

static void TestToLanguageTag(void) {
    char langtag[256];
    int32_t i;
    UErrorCode status;
    int32_t len;
    const char *inloc;
    const char *expected;

    for (i = 0; locale_to_langtag[i][0] != NULL; i++) {
        inloc = locale_to_langtag[i][0];

        /* testing non-strict mode */
        status = U_ZERO_ERROR;
        langtag[0] = 0;
        expected = locale_to_langtag[i][1];

        len = uloc_toLanguageTag(inloc, langtag, sizeof(langtag), FALSE, &status);
        if (U_FAILURE(status)) {
            if (expected != NULL) {
                log_err("Error returned by uloc_toLanguageTag for locale id [%s] - error: %s\n",
                    inloc, u_errorName(status));
            }
        } else {
            if (expected == NULL) {
                log_err("Error should be returned by uloc_toLanguageTag for locale id [%s], but [%s] is returned without errors\n",
                    inloc, langtag);
            } else if (uprv_strcmp(langtag, expected) != 0) {
                log_data_err("uloc_toLanguageTag returned language tag [%s] for input locale [%s] - expected: [%s]. Are you missing data?\n",
                    langtag, inloc, expected);
            }
        }

        /* testing strict mode */
        status = U_ZERO_ERROR;
        langtag[0] = 0;
        expected = locale_to_langtag[i][2];

        len = uloc_toLanguageTag(inloc, langtag, sizeof(langtag), TRUE, &status);
        if (U_FAILURE(status)) {
            if (expected != NULL) {
                log_data_err("Error returned by uloc_toLanguageTag {strict} for locale id [%s] - error: %s Are you missing data?\n",
                    inloc, u_errorName(status));
            }
        } else {
            if (expected == NULL) {
                log_err("Error should be returned by uloc_toLanguageTag {strict} for locale id [%s], but [%s] is returned without errors\n",
                    inloc, langtag);
            } else if (uprv_strcmp(langtag, expected) != 0) {
                log_err("uloc_toLanguageTag {strict} returned language tag [%s] for input locale [%s] - expected: [%s]\n",
                    langtag, inloc, expected);
            }
        }
    }
}

static const struct {
    const char  *bcpID;
    const char  *locID;
    int32_t     len;
} langtag_to_locale[] = {
    {"en",                  "en",                   2},
    {"en-us",               "en_US",                5},
    {"und-US",              "_US",                  6},
    {"und-latn",            "_Latn",                8},
    {"en-US-posix",         "en_US_POSIX",          11},
    {"de-de_euro",          "de",                   2},
    {"kok-IN",              "kok_IN",               6},
    {"123",                 "",                     0},
    {"en_us",               "",                     0},
    {"en-latn-x",           "en_Latn",              7},
    {"art-lojban",          "jbo",                  10},
    {"zh-hakka",            "hak",                  8},
    {"zh-cmn-CH",           "cmn_CH",               9},
    {"xxx-yy",              "xxx_YY",               6},
    {"fr-234",              "fr_234",               6},
    {"i-default",           "",                     9},
    {"i-test",              "",                     0},
    {"ja-jp-jp",            "ja_JP",                5},
    {"bogus",               "bogus",                5},
    {"boguslang",           "",                     0},
    {"EN-lATN-us",          "en_Latn_US",           10},
    {"und-variant-1234",    "__1234_VARIANT",       16},
    {"und-varzero-var1-vartwo", "__VARZERO",        11},
    {"en-u-ca-gregory",     "en@calendar=gregorian",    15},
    {"en-U-cu-USD",         "en@currency=usd",      11},
    {"en-US-u-va-posix",    "en_US_POSIX",          16},
    {"ar-x-1-2-3",          "ar@x=1-2-3",           10},
    {"fr-u-nu-latn-cu-eur", "fr@currency=eur;numbers=latn", 19},
    {"de-k-kext-u-co-phonebk-nu-latn",  "de@collation=phonebook;k=kext;numbers=latn",   30},
    {"ja-u-cu-jpy-ca-jp",   "ja@currency=jpy",      11},
    {"en-us-u-tz-usnyc",    "en_US@timezone=America/New_York",      16},
    {"und-a-abc-def",       "und@a=abc-def",        13},
    {"zh-u-ca-chinese-x-u-ca-chinese",  "zh@calendar=chinese;x=u-ca-chinese",   30},
    {NULL,          NULL,           0}
};

static void TestForLanguageTag(void) {
    char locale[256];
    int32_t i;
    UErrorCode status;
    int32_t parsedLen;

    for (i = 0; langtag_to_locale[i].bcpID != NULL; i++) {
        status = U_ZERO_ERROR;
        locale[0] = 0;        
        uloc_forLanguageTag(langtag_to_locale[i].bcpID, locale, sizeof(locale), &parsedLen, &status);
        if (U_FAILURE(status)) {
            log_err_status(status, "Error returned by uloc_forLanguageTag for language tag [%s] - error: %s\n",
                langtag_to_locale[i].bcpID, u_errorName(status));
        } else {
            if (uprv_strcmp(langtag_to_locale[i].locID, locale) != 0) {
                log_err("uloc_forLanguageTag returned locale [%s] for input language tag [%s] - expected: [%s]\n",
                    locale, langtag_to_locale[i].bcpID, langtag_to_locale[i].locID);
            }
            if (parsedLen != langtag_to_locale[i].len) {
                log_err("uloc_forLanguageTag parsed length of %d for input language tag [%s] - expected parsed length: %d\n",
                    parsedLen, langtag_to_locale[i].bcpID, langtag_to_locale[i].len);
            }
        }
    }
}

static void test_unicode_define(const char *namech, char ch, const char *nameu, UChar uch)
{
  UChar asUch[1];
  asUch[0]=0;
  log_verbose("Testing whether %s[\\x%02x,'%c'] == %s[U+%04X]\n", namech, ch,(int)ch, nameu, (int) uch);
  u_charsToUChars(&ch, asUch, 1);
  if(asUch[0] != uch) {
    log_err("FAIL:  %s[\\x%02x,'%c'] maps to U+%04X, but %s = U+%04X\n", namech, ch, (int)ch, (int)asUch[0], nameu, (int)uch);
  } else {
    log_verbose(" .. OK, == U+%04X\n", (int)asUch[0]);
  }
}

#define TEST_UNICODE_DEFINE(x,y) test_unicode_define(#x, (char)(x), #y, (UChar)(y))

static void TestUnicodeDefines(void) {
  TEST_UNICODE_DEFINE(ULOC_KEYWORD_SEPARATOR, ULOC_KEYWORD_SEPARATOR_UNICODE);
  TEST_UNICODE_DEFINE(ULOC_KEYWORD_ASSIGN, ULOC_KEYWORD_ASSIGN_UNICODE);
  TEST_UNICODE_DEFINE(ULOC_KEYWORD_ITEM_SEPARATOR, ULOC_KEYWORD_ITEM_SEPARATOR_UNICODE);
}
