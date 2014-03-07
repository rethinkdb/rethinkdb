/********************************************************************
 * COPYRIGHT: 
 * Copyright (c) 1997-2010, International Business Machines Corporation and
 * others. All Rights Reserved.
 * Copyright (C) 2010 , Yahoo! Inc. 
 ********************************************************************/

#include "unicode/utypes.h"

#if !UCONFIG_NO_FORMATTING

#include "selfmts.h"
#include "cmemory.h"
#include "unicode/selfmt.h"
#include "stdio.h"

#define SIMPLE_PATTERN_STRING                                                    "feminine {feminineVerbValue} other{otherVerbValue}"


#define SELECT_PATTERN_DATA 4
#define SELECT_SYNTAX_DATA 10
#define EXP_FORMAT_RESULT_DATA 12
#define NUM_OF_FORMAT_ARGS 3

#define VERBOSE_INT(x) {logln("%s:%d:  int %s=%d\n", __FILE__, __LINE__, #x, (x));}
#define VERBOSE_USTRING(text) {logln("%s:%d: UnicodeString %s(%d) = ", __FILE__, __LINE__, #text, text.length()); logln(UnicodeString(" \"")+text+UnicodeString("\";"));}


void SelectFormatTest::runIndexedTest( int32_t index, UBool exec, const char* &name, char* /*par*/ )
{
    if (exec) logln("TestSuite SelectFormat");
    switch (index) {
        TESTCASE(0, selectFormatAPITest);
        TESTCASE(1, selectFormatUnitTest);
        default: name = "";
            break;
    }
}

/**
 * Unit tests of SelectFormat class.
 */
void SelectFormatTest::selectFormatUnitTest(/*char *par*/)
{
  const UnicodeString SIMPLE_PATTERN(SIMPLE_PATTERN_STRING); /* Don't static init this! */

    UnicodeString patternTestData[SELECT_PATTERN_DATA] = {
        UNICODE_STRING_SIMPLE("fem {femValue} other{even}"),
        UNICODE_STRING_SIMPLE("other{odd or even}"),
        UNICODE_STRING_SIMPLE("odd{The number {0, number, integer} is odd.}other{The number {0, number, integer} is even.}"),
        UNICODE_STRING_SIMPLE("odd{The number {1} is odd}other{The number {1} is even}"),
    };

    UnicodeString formatArgs[NUM_OF_FORMAT_ARGS] = {
        UNICODE_STRING_SIMPLE("fem"),
        UNICODE_STRING_SIMPLE("other"),
        UNICODE_STRING_SIMPLE("odd")
    };

    UnicodeString expFormatResult[][NUM_OF_FORMAT_ARGS] = {
        {
            UNICODE_STRING_SIMPLE("femValue"),
            UNICODE_STRING_SIMPLE("even"),
            UNICODE_STRING_SIMPLE("even")
        },
        {
            UNICODE_STRING_SIMPLE("odd or even"),
            UNICODE_STRING_SIMPLE("odd or even"),
            UNICODE_STRING_SIMPLE("odd or even"),
        },
        {
            UNICODE_STRING_SIMPLE("The number {0, number, integer} is even."),
            UNICODE_STRING_SIMPLE("The number {0, number, integer} is even."),
            UNICODE_STRING_SIMPLE("The number {0, number, integer} is odd."),
        },
        {
            UNICODE_STRING_SIMPLE("The number {1} is even"),
            UNICODE_STRING_SIMPLE("The number {1} is even"),
            UNICODE_STRING_SIMPLE("The number {1} is odd"),
        }
    };

    UnicodeString checkSyntaxData[SELECT_SYNTAX_DATA] = {
        UNICODE_STRING_SIMPLE("odd{foo} odd{bar} other{foobar}"),
        UNICODE_STRING_SIMPLE("odd{foo} other{bar} other{foobar}"),
        UNICODE_STRING_SIMPLE("odd{foo}"),
        UNICODE_STRING_SIMPLE("1odd{foo} other{bar}"),
        UNICODE_STRING_SIMPLE("odd{foo},other{bar}"),
        UNICODE_STRING_SIMPLE("od d{foo} other{bar}"),
        UNICODE_STRING_SIMPLE("odd{foo}{foobar}other{foo}"),
        UNICODE_STRING_SIMPLE("odd{foo1}other{foo2}}"),  
        UNICODE_STRING_SIMPLE("odd{foo1}other{{foo2}"),  
        UNICODE_STRING_SIMPLE("odd{fo{o1}other{foo2}}")
    };

    UErrorCode expErrorCodes[SELECT_SYNTAX_DATA]={
        U_DUPLICATE_KEYWORD, 
        U_DUPLICATE_KEYWORD,  
        U_DEFAULT_KEYWORD_MISSING,
        U_PATTERN_SYNTAX_ERROR,
        U_PATTERN_SYNTAX_ERROR,
        U_PATTERN_SYNTAX_ERROR,
        U_PATTERN_SYNTAX_ERROR,
        U_PATTERN_SYNTAX_ERROR,  
        U_PATTERN_SYNTAX_ERROR,  
        U_DEFAULT_KEYWORD_MISSING
    };

    UErrorCode status = U_ZERO_ERROR;
    VERBOSE_USTRING(SIMPLE_PATTERN);
    SelectFormat* selFmt = new SelectFormat( SIMPLE_PATTERN , status); 
    if (U_FAILURE(status)) {
        dataerrln("ERROR: SelectFormat Unit Test constructor failed in unit tests.- exitting");
        return;
    }
    
    // ======= Test SelectFormat pattern syntax.
    logln("SelectFormat Unit Test : Testing SelectFormat pattern syntax.");
    for (int32_t i=0; i<SELECT_SYNTAX_DATA; ++i) {
        status = U_ZERO_ERROR;
        VERBOSE_INT(i);
        VERBOSE_USTRING(checkSyntaxData[i]);
        selFmt->applyPattern(checkSyntaxData[i], status);
        if( status!= expErrorCodes[i] ){
            errln("\nERROR: Unexpected result - SelectFormat Unit Test failed to detect syntax error with pattern: "+checkSyntaxData[i]+" and expected status="+ u_errorName(expErrorCodes[i]) + " and resulted status="+u_errorName(status));
        }
    }

    delete selFmt;
    selFmt = NULL;

    logln("SelectFormat Unit Test : Creating format object for Testing applying various patterns");
    status = U_ZERO_ERROR;
    selFmt = new SelectFormat( SIMPLE_PATTERN , status); 
    //SelectFormat* selFmt1 = new SelectFormat( SIMPLE_PATTERN , status); 
    if (U_FAILURE(status)) {
        errln("ERROR: SelectFormat Unit Test constructor failed in unit tests.- exitting");
        return;
    }

    // ======= Test applying and formatting with various pattern
    logln("SelectFormat Unit test: Testing  applyPattern() and format() ...");
    UnicodeString result;
    FieldPosition ignore(FieldPosition::DONT_CARE);

    for(int32_t i=0; i<SELECT_PATTERN_DATA; ++i) {
        status = U_ZERO_ERROR;
        selFmt->applyPattern(patternTestData[i], status);
        if (U_FAILURE(status)) {
            errln("ERROR: SelectFormat Unit Test failed to apply pattern- "+patternTestData[i] );
            continue;
        }

        //Format with the keyword array
        for(int32_t j=0; j<3; j++) {
            result.remove();
            selFmt->format( formatArgs[j], result , ignore , status);
            if (U_FAILURE(status)) {
                errln("ERROR: SelectFormat Unit test failed in format() with argument: "+ formatArgs[j] + " and error is " + u_errorName(status) );
            }else{
                if( result != expFormatResult[i][j] ){
                    errln("ERROR: SelectFormat Unit test failed in format() with unexpected result\n  with argument: "+ formatArgs[j] + "\n result obtained: " + result + "\n and expected is: " + expFormatResult[i][j] );
                }
            }
        }
    }

    //Test with an invalid keyword
    logln("SelectFormat Unit test: Testing  format() with keyword method and with invalid keywords...");
    status = U_ZERO_ERROR;
    result.remove();
    UnicodeString keywords[] = {
        "9Keyword-_",       //Starts with a digit
        "-Keyword-_",       //Starts with a hyphen
        "_Keyword-_",       //Starts with a underscore
        "\\u00E9Keyword-_", //Starts with non-ASCII character
        "Key*word-_",       //Contains a sepial character not allowed
        "*Keyword-_"        //Starts with a sepial character not allowed
    };

    delete selFmt;
    selFmt = NULL;

    selFmt = new SelectFormat( SIMPLE_PATTERN , status); 
    for (int32_t i = 0; i< 6; i++ ){
        status = U_ZERO_ERROR;
        selFmt->format( keywords[i], result , ignore , status);
        if (!U_FAILURE(status)) {
            errln("ERROR: SelectFormat Unit test failed in format() with keyWord and with an invalid keyword as : "+ keywords[i]);
        }
    }

    delete selFmt;
}

/**
 * Test various generic API methods of SelectFormat for Basic API usage.
 * This is to make sure the API test coverage is 100% .
 */
void SelectFormatTest::selectFormatAPITest(/*char *par*/)
{
  const UnicodeString SIMPLE_PATTERN(SIMPLE_PATTERN_STRING); /* Don't static init this! */
    int numOfConstructors =3;
    UErrorCode status[3];
    SelectFormat* selFmt[3] = { NULL, NULL, NULL };

    // ========= Test constructors
    logln("SelectFormat API test: Testing SelectFormat constructors ...");
    for (int32_t i=0; i< numOfConstructors; ++i) {
        status[i] = U_ZERO_ERROR;
    }

    selFmt[0]= new SelectFormat(SIMPLE_PATTERN, status[0]);
    if ( U_FAILURE(status[0]) ) {
      errln("ERROR: SelectFormat API test constructor with pattern and status failed! with %s\n", u_errorName(status[0]));
        return;
    }

    // =========== Test copy constructor
    logln("SelectFormat API test: Testing copy constructor and == operator ...");
    SelectFormat fmt = *selFmt[0];
    SelectFormat* dupPFmt = new SelectFormat(fmt);
    if ((*selFmt[0]) != (*dupPFmt)) {
        errln("ERROR: SelectFormat API test Failed in copy constructor or == operator!");
    }
    delete dupPFmt;
    
    // ======= Test clone && == operator.
    logln("SelectFormat API test: Testing clone and == operator ...");
    if ( U_SUCCESS(status[0])  ) {
        selFmt[1] = (SelectFormat*)selFmt[0]->clone();
        if (selFmt[1]!=NULL) {
            if ( *selFmt[1] != *selFmt[0] ) {
                errln("ERROR: SelectFormat API test clone test failed!");
            }
        } else {
          errln("ERROR: SelectFormat API test clone test failed with NULL!");
          return;
        }
    } else {
      errln("ERROR: could not create [0]: %s\n", u_errorName(status[0]));
      return;
    }

    // ======= Test assignment operator && == operator.
    logln("SelectFormat API test: Testing assignment operator and == operator ...");
    selFmt[2]= new SelectFormat(SIMPLE_PATTERN, status[2]);
    if ( U_SUCCESS(status[2]) ) {
        *selFmt[1] = *selFmt[2];
        if (selFmt[1]!=NULL) {
            if ( (*selFmt[1] != *selFmt[2]) ) {
                errln("ERROR: SelectFormat API test assignment operator test failed!");
            }
        }
        delete selFmt[1];
    }
    else {
         errln("ERROR: SelectFormat constructor failed in assignment operator!");
    }
    delete selFmt[0];
    delete selFmt[2];

    // ======= Test getStaticClassID() and getStaticClassID()
    logln("SelectFormat API test: Testing getStaticClassID() and getStaticClassID() ...");
    UErrorCode status1 = U_ZERO_ERROR;
    SelectFormat* selFmt1 = new SelectFormat( SIMPLE_PATTERN , status1); 
    if( U_FAILURE(status1)) {
        errln("ERROR: SelectFormat constructor failed in staticClassID test! Exitting");
        return;
    }

    logln("Testing getStaticClassID()");
    if(selFmt1->getDynamicClassID() !=SelectFormat::getStaticClassID()) {
        errln("ERROR: SelectFormat API test getDynamicClassID() didn't return the expected value");
    }

    // ======= Test applyPattern() and toPattern()
    logln("SelectFormat API test: Testing applyPattern() and toPattern() ...");
    UnicodeString pattern = UnicodeString("masculine{masculineVerbValue} other{otherVerbValue}");
    status1 = U_ZERO_ERROR;
    selFmt1->applyPattern( pattern, status1);
    if (U_FAILURE(status1)) {
        errln("ERROR: SelectFormat API test failed in applyPattern() with pattern: "+ pattern);
    }else{
        UnicodeString checkPattern;
        selFmt1->toPattern( checkPattern);
        if( checkPattern != pattern ){
            errln("ERROR: SelectFormat API test failed in toPattern() with unexpected result with pattern: "+ pattern);
        }
    }

    // ======= Test different format() methods 
    logln("SelectFormat API test: Testing  format() with keyword method ...");
    status1 = U_ZERO_ERROR;
    UnicodeString result;
    FieldPosition ignore(FieldPosition::DONT_CARE);
    UnicodeString keyWord = UnicodeString("masculine");

    selFmt1->format( keyWord, result , ignore , status1);
    if (U_FAILURE(status1)) {
        errln("ERROR: SelectFormat API test failed in format() with keyWord: "+ keyWord);
    }else{
        UnicodeString expected=UnicodeString("masculineVerbValue");
        if( result != expected ){
            errln("ERROR: SelectFormat API test failed in format() with unexpected result with keyWord: "+ keyWord);
        }
    }
    
    logln("SelectFormat API test: Testing  format() with Formattable obj method ...");
    status1 = U_ZERO_ERROR;
    result.remove();
    UnicodeString result1;
    Formattable testArgs = Formattable("other");
    selFmt1->format( testArgs, result1 , ignore , status1);
    if (U_FAILURE(status1)) {
        errln("ERROR: SelectFormat API test failed in format() with Formattable");
    }else{
        UnicodeString expected=UnicodeString("otherVerbValue");
        if( result1 != expected ){
            errln("ERROR: SelectFormat API test failed in format() with unexpected result with Formattable");
        }
    }


    delete selFmt1;
}
#endif /* #if !UCONFIG_NO_FORMATTING */
