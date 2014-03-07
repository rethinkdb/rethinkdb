/*
*******************************************************************************
*
*   Copyright (C) 2002-2009, International Business Machines
*   Corporation and others.  All Rights Reserved.
*
*******************************************************************************
*   file name:  strcase.cpp
*   encoding:   US-ASCII
*   tab size:   8 (not used)
*   indentation:4
*
*   created on: 2002mar12
*   created by: Markus W. Scherer
*
*   Test file for string casing C++ API functions.
*/

#include "unicode/uchar.h"
#include "unicode/ures.h"
#include "unicode/uloc.h"
#include "unicode/locid.h"
#include "unicode/ubrk.h"
#include "unicode/unistr.h"
#include "unicode/ucasemap.h"
#include "ustrtest.h"
#include "unicode/tstdtmod.h"

#define LENGTHOF(array) (int32_t)(sizeof(array)/sizeof((array)[0]))

StringCaseTest::~StringCaseTest() {}

void
StringCaseTest::runIndexedTest(int32_t index, UBool exec, const char *&name, char * /*par*/) {
    if (exec) logln("TestSuite StringCaseTest: ");
    switch (index) {
        case 0: name = "TestCaseConversion"; if (exec) TestCaseConversion(); break;
        case 1: 
#if !UCONFIG_NO_BREAK_ITERATION && !UCONFIG_NO_FILE_IO && !UCONFIG_NO_LEGACY_CONVERSION
            name = "TestCasing";
            if(exec) TestCasing();
#else
            name = "skip";
#endif
            break;

        default: name = ""; break; //needed to end loop
    }
}

void
StringCaseTest::TestCaseConversion()
{
    static const UChar uppercaseGreek[] =
        { 0x399, 0x395, 0x3a3, 0x3a5, 0x3a3, 0x20, 0x03a7, 0x3a1, 0x399, 0x3a3, 0x3a4,
        0x39f, 0x3a3, 0 };
        // "IESUS CHRISTOS"

    static const UChar lowercaseGreek[] = 
        { 0x3b9, 0x3b5, 0x3c3, 0x3c5, 0x3c2, 0x20, 0x03c7, 0x3c1, 0x3b9, 0x3c3, 0x3c4,
        0x3bf, 0x3c2, 0 };
        // "iesus christos"

    static const UChar lowercaseTurkish[] = 
        { 0x69, 0x73, 0x74, 0x61, 0x6e, 0x62, 0x75, 0x6c, 0x2c, 0x20, 0x6e, 0x6f, 0x74, 0x20, 0x63, 0x6f, 
        0x6e, 0x73, 0x74, 0x61, 0x6e, 0x74, 0x0131, 0x6e, 0x6f, 0x70, 0x6c, 0x65, 0x21, 0 };

    static const UChar uppercaseTurkish[] = 
        { 0x54, 0x4f, 0x50, 0x4b, 0x41, 0x50, 0x49, 0x20, 0x50, 0x41, 0x4c, 0x41, 0x43, 0x45, 0x2c, 0x20,
        0x0130, 0x53, 0x54, 0x41, 0x4e, 0x42, 0x55, 0x4c, 0 };
    
    UnicodeString expectedResult;
    UnicodeString   test3;

    test3 += (UChar32)0x0130;
    test3 += "STANBUL, NOT CONSTANTINOPLE!";

    UnicodeString   test4(test3);
    test4.toLower(Locale(""));
    expectedResult = UnicodeString("i\\u0307stanbul, not constantinople!", "").unescape();
    if (test4 != expectedResult)
        errln("1. toLower failed: expected \"" + expectedResult + "\", got \"" + test4 + "\".");

    test4 = test3;
    test4.toLower(Locale("tr", "TR"));
    expectedResult = lowercaseTurkish;
    if (test4 != expectedResult)
        errln("2. toLower failed: expected \"" + expectedResult + "\", got \"" + test4 + "\".");

    test3 = "topkap";
    test3 += (UChar32)0x0131;
    test3 += " palace, istanbul";
    test4 = test3;

    test4.toUpper(Locale(""));
    expectedResult = "TOPKAPI PALACE, ISTANBUL";
    if (test4 != expectedResult)
        errln("toUpper failed: expected \"" + expectedResult + "\", got \"" + test4 + "\".");

    test4 = test3;
    test4.toUpper(Locale("tr", "TR"));
    expectedResult = uppercaseTurkish;
    if (test4 != expectedResult)
        errln("toUpper failed: expected \"" + expectedResult + "\", got \"" + test4 + "\".");

    test3 = CharsToUnicodeString("S\\u00FC\\u00DFmayrstra\\u00DFe");

    test3.toUpper(Locale("de", "DE"));
    expectedResult = CharsToUnicodeString("S\\u00DCSSMAYRSTRASSE");
    if (test3 != expectedResult)
        errln("toUpper failed: expected \"" + expectedResult + "\", got \"" + test3 + "\".");
    
    test4.replace(0, test4.length(), uppercaseGreek);

    test4.toLower(Locale("el", "GR"));
    expectedResult = lowercaseGreek;
    if (test4 != expectedResult)
        errln("toLower failed: expected \"" + expectedResult + "\", got \"" + test4 + "\".");
    
    test4.replace(0, test4.length(), lowercaseGreek);

    test4.toUpper();
    expectedResult = uppercaseGreek;
    if (test4 != expectedResult)
        errln("toUpper failed: expected \"" + expectedResult + "\", got \"" + test4 + "\".");

    // more string case mapping tests with the new implementation
    {
        static const UChar

        beforeLower[]= { 0x61, 0x42, 0x49,  0x3a3, 0xdf, 0x3a3, 0x2f, 0xd93f, 0xdfff },
        lowerRoot[]=   { 0x61, 0x62, 0x69,  0x3c3, 0xdf, 0x3c2, 0x2f, 0xd93f, 0xdfff },
        lowerTurkish[]={ 0x61, 0x62, 0x131, 0x3c3, 0xdf, 0x3c2, 0x2f, 0xd93f, 0xdfff },

        beforeUpper[]= { 0x61, 0x42, 0x69,  0x3c2, 0xdf,       0x3c3, 0x2f, 0xfb03,           0xfb03,           0xfb03,           0xd93f, 0xdfff },
        upperRoot[]=   { 0x41, 0x42, 0x49,  0x3a3, 0x53, 0x53, 0x3a3, 0x2f, 0x46, 0x46, 0x49, 0x46, 0x46, 0x49, 0x46, 0x46, 0x49, 0xd93f, 0xdfff },
        upperTurkish[]={ 0x41, 0x42, 0x130, 0x3a3, 0x53, 0x53, 0x3a3, 0x2f, 0x46, 0x46, 0x49, 0x46, 0x46, 0x49, 0x46, 0x46, 0x49, 0xd93f, 0xdfff },

        beforeMiniUpper[]=  { 0xdf, 0x61 },
        miniUpper[]=        { 0x53, 0x53, 0x41 };

        UnicodeString s;

        /* lowercase with root locale */
        s=UnicodeString(FALSE, beforeLower, (int32_t)(sizeof(beforeLower)/U_SIZEOF_UCHAR));
        s.toLower("");
        if( s.length()!=(sizeof(lowerRoot)/U_SIZEOF_UCHAR) ||
            s!=UnicodeString(FALSE, lowerRoot, s.length())
        ) {
            errln("error in toLower(root locale)=\"" + s + "\" expected \"" + UnicodeString(FALSE, lowerRoot, (int32_t)(sizeof(lowerRoot)/U_SIZEOF_UCHAR)) + "\"");
        }

        /* lowercase with turkish locale */
        s=UnicodeString(FALSE, beforeLower, (int32_t)(sizeof(beforeLower)/U_SIZEOF_UCHAR));
        s.setCharAt(0, beforeLower[0]).toLower(Locale("tr"));
        if( s.length()!=(sizeof(lowerTurkish)/U_SIZEOF_UCHAR) ||
            s!=UnicodeString(FALSE, lowerTurkish, s.length())
        ) {
            errln("error in toLower(turkish locale)=\"" + s + "\" expected \"" + UnicodeString(FALSE, lowerTurkish, (int32_t)(sizeof(lowerTurkish)/U_SIZEOF_UCHAR)) + "\"");
        }

        /* uppercase with root locale */
        s=UnicodeString(FALSE, beforeUpper, (int32_t)(sizeof(beforeUpper)/U_SIZEOF_UCHAR));
        s.setCharAt(0, beforeUpper[0]).toUpper(Locale(""));
        if( s.length()!=(sizeof(upperRoot)/U_SIZEOF_UCHAR) ||
            s!=UnicodeString(FALSE, upperRoot, s.length())
        ) {
            errln("error in toUpper(root locale)=\"" + s + "\" expected \"" + UnicodeString(FALSE, upperRoot, (int32_t)(sizeof(upperRoot)/U_SIZEOF_UCHAR)) + "\"");
        }

        /* uppercase with turkish locale */
        s=UnicodeString(FALSE, beforeUpper, (int32_t)(sizeof(beforeUpper)/U_SIZEOF_UCHAR));
        s.toUpper(Locale("tr"));
        if( s.length()!=(sizeof(upperTurkish)/U_SIZEOF_UCHAR) ||
            s!=UnicodeString(FALSE, upperTurkish, s.length())
        ) {
            errln("error in toUpper(turkish locale)=\"" + s + "\" expected \"" + UnicodeString(FALSE, upperTurkish, (int32_t)(sizeof(upperTurkish)/U_SIZEOF_UCHAR)) + "\"");
        }

        /* uppercase a short string with root locale */
        s=UnicodeString(FALSE, beforeMiniUpper, (int32_t)(sizeof(beforeMiniUpper)/U_SIZEOF_UCHAR));
        s.setCharAt(0, beforeMiniUpper[0]).toUpper("");
        if( s.length()!=(sizeof(miniUpper)/U_SIZEOF_UCHAR) ||
            s!=UnicodeString(FALSE, miniUpper, s.length())
        ) {
            errln("error in toUpper(root locale)=\"" + s + "\" expected \"" + UnicodeString(FALSE, miniUpper, (int32_t)(sizeof(miniUpper)/U_SIZEOF_UCHAR)) + "\"");
        }
    }

    // test some supplementary characters (>= Unicode 3.1)
    {
        UnicodeString t;

        UnicodeString
            deseretInput=UnicodeString("\\U0001043C\\U00010414", "").unescape(),
            deseretLower=UnicodeString("\\U0001043C\\U0001043C", "").unescape(),
            deseretUpper=UnicodeString("\\U00010414\\U00010414", "").unescape();
        (t=deseretInput).toLower();
        if(t!=deseretLower) {
            errln("error lowercasing Deseret (plane 1) characters");
        }
        (t=deseretInput).toUpper();
        if(t!=deseretUpper) {
            errln("error uppercasing Deseret (plane 1) characters");
        }
    }

    // test some more cases that looked like problems
    {
        UnicodeString t;

        UnicodeString
            ljInput=UnicodeString("ab'cD \\uFB00i\\u0131I\\u0130 \\u01C7\\u01C8\\u01C9 \\U0001043C\\U00010414", "").unescape(),
            ljLower=UnicodeString("ab'cd \\uFB00i\\u0131ii\\u0307 \\u01C9\\u01C9\\u01C9 \\U0001043C\\U0001043C", "").unescape(),
            ljUpper=UnicodeString("AB'CD FFIII\\u0130 \\u01C7\\u01C7\\u01C7 \\U00010414\\U00010414", "").unescape();
        (t=ljInput).toLower("en");
        if(t!=ljLower) {
            errln("error lowercasing LJ characters");
        }
        (t=ljInput).toUpper("en");
        if(t!=ljUpper) {
            errln("error uppercasing LJ characters");
        }
    }

#if !UCONFIG_NO_NORMALIZATION
    // some context-sensitive casing depends on normalization data being present

    // Unicode 3.1.1 SpecialCasing tests
    {
        UnicodeString t;

        // sigmas preceded and/or followed by cased letters
        UnicodeString
            sigmas=UnicodeString("i\\u0307\\u03a3\\u0308j \\u0307\\u03a3\\u0308j i\\u00ad\\u03a3\\u0308 \\u0307\\u03a3\\u0308 ", "").unescape(),
            sigmasLower=UnicodeString("i\\u0307\\u03c3\\u0308j \\u0307\\u03c3\\u0308j i\\u00ad\\u03c2\\u0308 \\u0307\\u03c3\\u0308 ", "").unescape(),
            sigmasUpper=UnicodeString("I\\u0307\\u03a3\\u0308J \\u0307\\u03a3\\u0308J I\\u00ad\\u03a3\\u0308 \\u0307\\u03a3\\u0308 ", "").unescape();

        (t=sigmas).toLower();
        if(t!=sigmasLower) {
            errln("error in sigmas.toLower()=\"" + t + "\" expected \"" + sigmasLower + "\"");
        }

        (t=sigmas).toUpper(Locale(""));
        if(t!=sigmasUpper) {
            errln("error in sigmas.toUpper()=\"" + t + "\" expected \"" + sigmasUpper + "\"");
        }

        // turkish & azerbaijani dotless i & dotted I
        // remove dot above if there was a capital I before and there are no more accents above
        UnicodeString
            dots=UnicodeString("I \\u0130 I\\u0307 I\\u0327\\u0307 I\\u0301\\u0307 I\\u0327\\u0307\\u0301", "").unescape(),
            dotsTurkish=UnicodeString("\\u0131 i i i\\u0327 \\u0131\\u0301\\u0307 i\\u0327\\u0301", "").unescape(),
            dotsDefault=UnicodeString("i i\\u0307 i\\u0307 i\\u0327\\u0307 i\\u0301\\u0307 i\\u0327\\u0307\\u0301", "").unescape();

        (t=dots).toLower("tr");
        if(t!=dotsTurkish) {
            errln("error in dots.toLower(tr)=\"" + t + "\" expected \"" + dotsTurkish + "\"");
        }

        (t=dots).toLower("de");
        if(t!=dotsDefault) {
            errln("error in dots.toLower(de)=\"" + t + "\" expected \"" + dotsDefault + "\"");
        }
    }

    // more Unicode 3.1.1 tests
    {
        UnicodeString t;

        // lithuanian dot above in uppercasing
        UnicodeString
            dots=UnicodeString("a\\u0307 \\u0307 i\\u0307 j\\u0327\\u0307 j\\u0301\\u0307", "").unescape(),
            dotsLithuanian=UnicodeString("A\\u0307 \\u0307 I J\\u0327 J\\u0301\\u0307", "").unescape(),
            dotsDefault=UnicodeString("A\\u0307 \\u0307 I\\u0307 J\\u0327\\u0307 J\\u0301\\u0307", "").unescape();

        (t=dots).toUpper("lt");
        if(t!=dotsLithuanian) {
            errln("error in dots.toUpper(lt)=\"" + t + "\" expected \"" + dotsLithuanian + "\"");
        }

        (t=dots).toUpper("de");
        if(t!=dotsDefault) {
            errln("error in dots.toUpper(de)=\"" + t + "\" expected \"" + dotsDefault + "\"");
        }

        // lithuanian adds dot above to i in lowercasing if there are more above accents
        UnicodeString
            i=UnicodeString("I I\\u0301 J J\\u0301 \\u012e \\u012e\\u0301 \\u00cc\\u00cd\\u0128", "").unescape(),
            iLithuanian=UnicodeString("i i\\u0307\\u0301 j j\\u0307\\u0301 \\u012f \\u012f\\u0307\\u0301 i\\u0307\\u0300i\\u0307\\u0301i\\u0307\\u0303", "").unescape(),
            iDefault=UnicodeString("i i\\u0301 j j\\u0301 \\u012f \\u012f\\u0301 \\u00ec\\u00ed\\u0129", "").unescape();

        (t=i).toLower("lt");
        if(t!=iLithuanian) {
            errln("error in i.toLower(lt)=\"" + t + "\" expected \"" + iLithuanian + "\"");
        }

        (t=i).toLower("de");
        if(t!=iDefault) {
            errln("error in i.toLower(de)=\"" + t + "\" expected \"" + iDefault + "\"");
        }
    }

#endif

    // test case folding
    {
        UnicodeString
            s=UnicodeString("A\\u00df\\u00b5\\ufb03\\U0001040c\\u0130\\u0131", "").unescape(),
            f=UnicodeString("ass\\u03bcffi\\U00010434i\\u0307\\u0131", "").unescape(),
            g=UnicodeString("ass\\u03bcffi\\U00010434i\\u0131", "").unescape(),
            t;

        (t=s).foldCase();
        if(f!=t) {
            errln("error in foldCase(\"" + s + "\", default)=\"" + t + "\" but expected \"" + f + "\"");
        }

        // alternate handling for dotted I/dotless i (U+0130, U+0131)
        (t=s).foldCase(U_FOLD_CASE_EXCLUDE_SPECIAL_I);
        if(g!=t) {
            errln("error in foldCase(\"" + s + "\", U_FOLD_CASE_EXCLUDE_SPECIAL_I)=\"" + t + "\" but expected \"" + g + "\"");
        }
    }
}

// data-driven case mapping tests ------------------------------------------ ***

enum {
    TEST_LOWER,
    TEST_UPPER,
    TEST_TITLE,
    TEST_FOLD,
    TEST_COUNT
};

// names of TestData children in casing.txt
static const char *const dataNames[TEST_COUNT+1]={
    "lowercasing",
    "uppercasing",
    "titlecasing",
    "casefolding",
    ""
};

void
StringCaseTest::TestCasingImpl(const UnicodeString &input,
                               const UnicodeString &output,
                               int32_t whichCase,
                               void *iter, const char *localeID, uint32_t options) {
    // UnicodeString
    UnicodeString result;
    const char *name;
    Locale locale(localeID);

    result=input;
    switch(whichCase) {
    case TEST_LOWER:
        name="toLower";
        result.toLower(locale);
        break;
    case TEST_UPPER:
        name="toUpper";
        result.toUpper(locale);
        break;
#if !UCONFIG_NO_BREAK_ITERATION
    case TEST_TITLE:
        name="toTitle";
        result.toTitle((BreakIterator *)iter, locale, options);
        break;
#endif
    case TEST_FOLD:
        name="foldCase";
        result.foldCase(options);
        break;
    default:
        name="";
        break; // won't happen
    }
    if(result!=output) {
        dataerrln("error: UnicodeString.%s() got a wrong result for a test case from casing.res", name);
    }
#if !UCONFIG_NO_BREAK_ITERATION
    if(whichCase==TEST_TITLE && options==0) {
        result=input;
        result.toTitle((BreakIterator *)iter, locale);
        if(result!=output) {
            dataerrln("error: UnicodeString.toTitle(options=0) got a wrong result for a test case from casing.res");
        }
    }
#endif

    // UTF-8
    char utf8In[100], utf8Out[100];
    int32_t utf8InLength, utf8OutLength, resultLength;
    UChar *buffer;

    IcuTestErrorCode errorCode(*this, "TestCasingImpl");
    LocalUCaseMapPointer csm(ucasemap_open(localeID, options, errorCode));
#if !UCONFIG_NO_BREAK_ITERATION
    if(iter!=NULL) {
        // Clone the break iterator so that the UCaseMap can safely adopt it.
        int32_t size=1;  // Not 0 because that only gives preflighting.
        UBreakIterator *clone=ubrk_safeClone((UBreakIterator *)iter, NULL, &size, errorCode);
        ucasemap_setBreakIterator(csm.getAlias(), clone, errorCode);
    }
#endif

    u_strToUTF8(utf8In, (int32_t)sizeof(utf8In), &utf8InLength, input.getBuffer(), input.length(), errorCode);
    switch(whichCase) {
    case TEST_LOWER:
        name="ucasemap_utf8ToLower";
        utf8OutLength=ucasemap_utf8ToLower(csm.getAlias(),
                    utf8Out, (int32_t)sizeof(utf8Out),
                    utf8In, utf8InLength, errorCode);
        break;
    case TEST_UPPER:
        name="ucasemap_utf8ToUpper";
        utf8OutLength=ucasemap_utf8ToUpper(csm.getAlias(),
                    utf8Out, (int32_t)sizeof(utf8Out),
                    utf8In, utf8InLength, errorCode);
        break;
#if !UCONFIG_NO_BREAK_ITERATION
    case TEST_TITLE:
        name="ucasemap_utf8ToTitle";
        utf8OutLength=ucasemap_utf8ToTitle(csm.getAlias(),
                    utf8Out, (int32_t)sizeof(utf8Out),
                    utf8In, utf8InLength, errorCode);
        break;
#endif
    case TEST_FOLD:
        name="ucasemap_utf8FoldCase";
        utf8OutLength=ucasemap_utf8FoldCase(csm.getAlias(),
                    utf8Out, (int32_t)sizeof(utf8Out),
                    utf8In, utf8InLength, errorCode);
        break;
    default:
        name="";
        utf8OutLength=0;
        break; // won't happen
    }
    buffer=result.getBuffer(utf8OutLength);
    u_strFromUTF8(buffer, result.getCapacity(), &resultLength, utf8Out, utf8OutLength, errorCode);
    result.releaseBuffer(errorCode.isSuccess() ? resultLength : 0);

    if(errorCode.isFailure()) {
        errcheckln(errorCode, "error: %s() got an error for a test case from casing.res - %s", name, u_errorName(errorCode));
        errorCode.reset();
    } else if(result!=output) {
        errln("error: %s() got a wrong result for a test case from casing.res", name);
        errln("expected \"" + output + "\" got \"" + result + "\"" );
    }
}

void
StringCaseTest::TestCasing() {
    UErrorCode status = U_ZERO_ERROR;
#if !UCONFIG_NO_BREAK_ITERATION
    LocalUBreakIteratorPointer iter;
#endif
    char cLocaleID[100];
    UnicodeString locale, input, output, optionsString, result;
    uint32_t options;
    int32_t whichCase, type;
    LocalPointer<TestDataModule> driver(TestDataModule::getTestDataModule("casing", *this, status));
    if(U_SUCCESS(status)) {
        for(whichCase=0; whichCase<TEST_COUNT; ++whichCase) {
#if UCONFIG_NO_BREAK_ITERATION
            if(whichCase==TEST_TITLE) {
                continue;
            }
#endif
            LocalPointer<TestData> casingTest(driver->createTestData(dataNames[whichCase], status));
            if(U_FAILURE(status)) {
                errln("TestCasing failed to createTestData(%s) - %s", dataNames[whichCase], u_errorName(status));
                break;
            }
            const DataMap *myCase = NULL;
            while(casingTest->nextCase(myCase, status)) {
                input = myCase->getString("Input", status);
                output = myCase->getString("Output", status);

                if(whichCase!=TEST_FOLD) {
                    locale = myCase->getString("Locale", status);
                }
                locale.extract(0, 0x7fffffff, cLocaleID, sizeof(cLocaleID), "");

#if !UCONFIG_NO_BREAK_ITERATION
                if(whichCase==TEST_TITLE) {
                    type = myCase->getInt("Type", status);
                    if(type>=0) {
                        iter.adoptInstead(ubrk_open((UBreakIteratorType)type, cLocaleID, NULL, 0, &status));
                    } else if(type==-2) {
                        // Open a trivial break iterator that only delivers { 0, length }
                        // or even just { 0 } as boundaries.
                        static const UChar rules[] = { 0x2e, 0x2a, 0x3b };  // ".*;"
                        UParseError parseError;
                        iter.adoptInstead(ubrk_openRules(rules, LENGTHOF(rules), NULL, 0, &parseError, &status));
                    }
                }
#endif
                options = 0;
                if(whichCase==TEST_TITLE || whichCase==TEST_FOLD) {
                    optionsString = myCase->getString("Options", status);
                    if(optionsString.indexOf((UChar)0x54)>=0) {  // T
                        options|=U_FOLD_CASE_EXCLUDE_SPECIAL_I;
                    }
                    if(optionsString.indexOf((UChar)0x4c)>=0) {  // L
                        options|=U_TITLECASE_NO_LOWERCASE;
                    }
                    if(optionsString.indexOf((UChar)0x41)>=0) {  // A
                        options|=U_TITLECASE_NO_BREAK_ADJUSTMENT;
                    }
                }

                if(U_FAILURE(status)) {
                    dataerrln("error: TestCasing() setup failed for %s test case from casing.res: %s", dataNames[whichCase],  u_errorName(status));
                    status = U_ZERO_ERROR;
                } else {
#if UCONFIG_NO_BREAK_ITERATION
                    LocalPointer<UMemory> iter;
#endif
                    TestCasingImpl(input, output, whichCase, iter.getAlias(), cLocaleID, options);
                }

#if !UCONFIG_NO_BREAK_ITERATION
                iter.adoptInstead(NULL);
#endif
            }
        }
    }

#if !UCONFIG_NO_BREAK_ITERATION
    // more tests for API coverage
    status=U_ZERO_ERROR;
    input=UNICODE_STRING_SIMPLE("sTrA\\u00dfE").unescape();
    (result=input).toTitle(NULL);
    if(result!=UNICODE_STRING_SIMPLE("Stra\\u00dfe").unescape()) {
        dataerrln("UnicodeString::toTitle(NULL) failed.");
    }
#endif
}
