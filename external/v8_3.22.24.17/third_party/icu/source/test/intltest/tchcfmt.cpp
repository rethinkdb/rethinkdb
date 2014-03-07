
/********************************************************************
 * COPYRIGHT: 
 * Copyright (c) 1997-2009, International Business Machines Corporation and
 * others. All Rights Reserved.
 ********************************************************************/

#include "unicode/utypes.h"

#if !UCONFIG_NO_FORMATTING

#include "intltest.h"
#include "tchcfmt.h"
#include "cmemory.h"
#include "unicode/msgfmt.h"
#include "unicode/choicfmt.h"

#include <float.h>

// tests have obvious memory leaks!

void TestChoiceFormat::runIndexedTest(int32_t index, UBool exec,
                                      const char* &name, char* /*par*/) {
    switch (index) {
        TESTCASE(0,TestSimpleExample);
        TESTCASE(1,TestComplexExample);
        TESTCASE(2,TestClosures);
        TESTCASE(3,TestPatterns);
        TESTCASE(4,TestChoiceFormatToPatternOverflow);
        default: name = ""; break;
    }
}

static UBool chkstatus( UErrorCode &status, const char* msg = NULL )
{
    UBool ok = U_SUCCESS(status);
    if (!ok) it_errln( msg );
    return ok;
}

void
TestChoiceFormat::TestSimpleExample( void )
{
    double limits[] = {1,2,3,4,5,6,7};
    UnicodeString monthNames[] = {"Sun","Mon","Tue","Wed","Thur","Fri","Sat"};
    ChoiceFormat* form = new ChoiceFormat(limits, monthNames, 7);
    ParsePosition parse_pos;
    // TODO Fix this ParsePosition stuff...
    UnicodeString str;
    UnicodeString res1, res2;
    UErrorCode status;
    FieldPosition fpos(0);
    Formattable f;
    int32_t ix;
    //for (double i = 0.0; i <= 8.0; ++i) {
    for (ix = 0; ix <= 8; ++ix) {
        double i = ix; //nos
        status = U_ZERO_ERROR;
        fpos = 0;
        str = "";
        res1 = form->format(i, str, fpos, status );
        if (!chkstatus( status, "***  test_simple_example format" )) {
            delete form;
            return;
        }
        //form->parse(res1, f, parse_pos);
        res2 = " ??? ";
        it_logln(UnicodeString("") + ix + UnicodeString(" -> ") + res1 + UnicodeString(" -> ") + res2);
    }
    //Testing ==operator
    const double filelimits[] = {0,1,2};
    const UnicodeString filepart[] = {"are no files","is one file","are {2} files"};
    ChoiceFormat* formnew=new ChoiceFormat(filelimits, filepart, 3); 
    ChoiceFormat* formequal=new ChoiceFormat(limits, monthNames, 7); 
    if(*formnew == *form){
        errln("ERROR: ==operator failed\n");
    }
    if(!(*form == *formequal)){
        errln("ERROR: ==operator failed\n");
    }
    delete formequal; 
    delete formnew; 
      
    //Testing getLimits()
    double *gotLimits=0;
    int32_t count=0;
    gotLimits=(double*)form->getLimits(count);
    if(count != 7){
        errln("getLimits didn't update the count correctly\n");
    }
    for(ix=0; ix<count; ix++){
        if(gotLimits[ix] != limits[ix]){
            errln((UnicodeString)"getLimits didn't get the limits correctly.  Expected " + limits[ix] + " Got " + gotLimits[ix]);
        }
    }
    //Testing getFormat()
    count=0;
    UnicodeString *gotFormats=0;
    gotFormats=(UnicodeString*)form->getFormats(count);
    if(count != 7){
        errln("getFormats didn't update the count correctly\n");
    }
    for(ix=0; ix<count; ix++){
        if(gotFormats[ix] != monthNames[ix]){
            errln((UnicodeString)"getFormats didn't get the Formats correctly.  Expected " + monthNames[ix] + " Got " + gotFormats[ix]);
        }
    }
    
   
    delete form;
   
}

void
TestChoiceFormat::TestComplexExample( void )
{
    UErrorCode status = U_ZERO_ERROR;
    const double filelimits[] = {-1, 0,1,2};
    const UnicodeString filepart[] = {"are corrupted files", "are no files","is one file","are {2} files"};

    ChoiceFormat* fileform = new ChoiceFormat( filelimits, filepart, 4);

    if (!fileform) { 
        it_errln("***  test_complex_example fileform"); 
        return; 
    }

    Format* filenumform = NumberFormat::createInstance( status );
    if (!filenumform) { 
        dataerrln((UnicodeString)"***  test_complex_example filenumform - " + u_errorName(status)); 
        delete fileform;
        return; 
    }
    if (!chkstatus( status, "***  test_simple_example filenumform" )) {
        delete fileform;
        delete filenumform;
        return;
    }

    //const Format* testFormats[] = { fileform, NULL, filenumform };
    //pattform->setFormats( testFormats, 3 );

    MessageFormat* pattform = new MessageFormat("There {0} on {1}", status );
    if (!pattform) { 
        it_errln("***  test_complex_example pattform"); 
        delete fileform;
        delete filenumform;
        return; 
    }
    if (!chkstatus( status, "***  test_complex_example pattform" )) {
        delete fileform;
        delete filenumform;
        delete pattform;
        return;
    }

    pattform->setFormat( 0, *fileform );
    pattform->setFormat( 2, *filenumform );


    Formattable testArgs[] = {(int32_t)0, "Disk_A", (int32_t)0};
    UnicodeString str;
    UnicodeString res1, res2;
    pattform->toPattern( res1 );
    it_logln("MessageFormat toPattern: " + res1);
    fileform->toPattern( res1 );
    it_logln("ChoiceFormat toPattern: " + res1);
    if (res1 == "-1#are corrupted files|0#are no files|1#is one file|2#are {2} files") {
        it_logln("toPattern tested!");
    }else{
        it_errln("***  ChoiceFormat to Pattern result!");
    }

    FieldPosition fpos(0);

    UnicodeString checkstr[] = { 
        "There are corrupted files on Disk_A",
        "There are no files on Disk_A",
        "There is one file on Disk_A",
        "There are 2 files on Disk_A",
        "There are 3 files on Disk_A"
    };

    // if (status != U_ZERO_ERROR) return; // TODO: analyze why we have such a bad bail out here!

    if (U_FAILURE(status)) { 
        delete fileform;
        delete filenumform;
        delete pattform;
        return;
    }


    int32_t i;
    int32_t start = -1;
    for (i = start; i < 4; ++i) {
        str = "";
        status = U_ZERO_ERROR;
        testArgs[0] = Formattable((int32_t)i);
        testArgs[2] = testArgs[0];
        res2 = pattform->format(testArgs, 3, str, fpos, status );
        if (!chkstatus( status, "***  test_complex_example format" )) {
            delete fileform;
            delete filenumform;
            delete pattform;
            return;
        }
        it_logln(i + UnicodeString(" -> ") + res2);
        if (res2 != checkstr[i - start]) {
            it_errln("***  test_complex_example res string");
            it_errln(UnicodeString("*** ") + i + UnicodeString(" -> '") + res2 + UnicodeString("' unlike '") + checkstr[i] + UnicodeString("' ! "));
        }
    }
    it_logln();

    it_logln("------ additional testing in complex test ------");
    it_logln();
    //
    int32_t retCount;
    const double* retLimits = fileform->getLimits( retCount );
    if ((retCount == 4) && (retLimits)
    && (retLimits[0] == -1.0)
    && (retLimits[1] == 0.0)
    && (retLimits[2] == 1.0)
    && (retLimits[3] == 2.0)) {
        it_logln("getLimits tested!");
    }else{
        it_errln("***  getLimits unexpected result!");
    }

    const UnicodeString* retFormats = fileform->getFormats( retCount );
    if ((retCount == 4) && (retFormats)
    && (retFormats[0] == "are corrupted files") 
    && (retFormats[1] == "are no files") 
    && (retFormats[2] == "is one file")
    && (retFormats[3] == "are {2} files")) {
        it_logln("getFormats tested!");
    }else{
        it_errln("***  getFormats unexpected result!");
    }

    UnicodeString checkstr2[] = { 
        "There is no folder on Disk_A",
        "There is one folder on Disk_A",
        "There are many folders on Disk_A",
        "There are many folders on Disk_A"
    };

    fileform->applyPattern("0#is no folder|1#is one folder|2#are many folders", status );
    if (status == U_ZERO_ERROR)
        it_logln("status applyPattern OK!");
    if (!chkstatus( status, "***  test_complex_example pattform" )) {
        delete fileform;
        delete filenumform;
        delete pattform;
        return;
    }
    pattform->setFormat( 0, *fileform );
    fpos = 0;
    for (i = 0; i < 4; ++i) {
        str = "";
        status = U_ZERO_ERROR;
        testArgs[0] = Formattable((int32_t)i);
        testArgs[2] = testArgs[0];
        res2 = pattform->format(testArgs, 3, str, fpos, status );
        if (!chkstatus( status, "***  test_complex_example format 2" )) {
            delete fileform;
            delete filenumform;
            delete pattform;
            return;
        }
        it_logln(UnicodeString() + i + UnicodeString(" -> ") + res2);
        if (res2 != checkstr2[i]) {
            it_errln("***  test_complex_example res string");
            it_errln(UnicodeString("*** ") + i + UnicodeString(" -> '") + res2 + UnicodeString("' unlike '") + checkstr2[i] + UnicodeString("' ! "));
        }
    }

    const double limits_A[] = {1,2,3,4,5,6,7};
    const UnicodeString monthNames_A[] = {"Sun","Mon","Tue","Wed","Thur","Fri","Sat"};
    ChoiceFormat* form_A = new ChoiceFormat(limits_A, monthNames_A, 7);
    ChoiceFormat* form_A2 = new ChoiceFormat(limits_A, monthNames_A, 7);
    const double limits_B[] = {1,2,3,4,5,6,7};
    const UnicodeString monthNames_B[] = {"Sun","Mon","Tue","Wed","Thur","Fri","Sat_BBB"};
    ChoiceFormat* form_B = new ChoiceFormat(limits_B, monthNames_B, 7);
    if (!form_A || !form_B || !form_A2) {
        it_errln("***  test-choiceFormat not allocatable!");
    }else{
        if (*form_A == *form_A2) {
            it_logln("operator== tested.");
        }else{
            it_errln("***  operator==");
        }

        if (*form_A != *form_B) {
            it_logln("operator!= tested.");
        }else{
            it_errln("***  operator!=");
        }

        ChoiceFormat* form_A3 = (ChoiceFormat*) form_A->clone();
        if (!form_A3) {
            it_errln("***  ChoiceFormat->clone is nil.");
        }else{
            if ((*form_A3 == *form_A) && (*form_A3 != *form_B)) {
                it_logln("method clone tested.");
            }else{
                it_errln("***  ChoiceFormat clone or operator==, or operator!= .");
            }
        }

        ChoiceFormat form_Assigned( *form_A );
        UBool ok = (form_Assigned == *form_A) && (form_Assigned != *form_B);
        form_Assigned = *form_B;
        ok = ok && (form_Assigned != *form_A) && (form_Assigned == *form_B);
        if (ok) {
            it_logln("copy constructor and operator= tested.");
        }else{
            it_errln("***  copy constructor or operator= or operator == or operator != .");
        }
        delete form_A3;
    }
    

    delete form_A; delete form_A2; delete form_B; 

    const char* testPattern = "0#none|1#one|2#many";
    ChoiceFormat form_pat( testPattern, status );
    if (!chkstatus( status, "***  ChoiceFormat contructor( newPattern, status)" )) {
        delete fileform;
        delete filenumform;
        delete pattform;
        return;
    }

    form_pat.toPattern( res1 );
    if (res1 == "0#none|1#one|2#many") {
        it_logln("ChoiceFormat contructor( newPattern, status) tested");
    }else{
        it_errln("***  ChoiceFormat contructor( newPattern, status) or toPattern result!");
    }

    double d_a2[] = { 3.0, 4.0 };
    UnicodeString s_a2[] = { "third", "forth" };

    form_pat.setChoices( d_a2, s_a2, 2 );
    form_pat.toPattern( res1 );
    it_logln(UnicodeString("ChoiceFormat adoptChoices toPattern: ") + res1);
    if (res1 == "3#third|4#forth") {
        it_logln("ChoiceFormat adoptChoices tested");
    }else{
        it_errln("***  ChoiceFormat adoptChoices result!");
    }

    str = "";
    fpos = 0;
    status = U_ZERO_ERROR;
    double arg_double = 3.0;
    res1 = form_pat.format( arg_double, str, fpos );
    it_logln(UnicodeString("ChoiceFormat format:") + res1);
    if (res1 != "third") it_errln("***  ChoiceFormat format (double, ...) result!");

    str = "";
    fpos = 0;
    status = U_ZERO_ERROR;
    int64_t arg_64 = 3;
    res1 = form_pat.format( arg_64, str, fpos );
    it_logln(UnicodeString("ChoiceFormat format:") + res1);
    if (res1 != "third") it_errln("***  ChoiceFormat format (int64_t, ...) result!");

    str = "";
    fpos = 0;
    status = U_ZERO_ERROR;
    int32_t arg_long = 3;
    res1 = form_pat.format( arg_long, str, fpos );
    it_logln(UnicodeString("ChoiceFormat format:") + res1);
    if (res1 != "third") it_errln("***  ChoiceFormat format (int32_t, ...) result!");

    Formattable ft( (int32_t)3 );
    str = "";
    fpos = 0;
    status = U_ZERO_ERROR;
    res1 = form_pat.format( ft, str, fpos, status );
    if (!chkstatus( status, "***  test_complex_example format (int32_t, ...)" )) {
        delete fileform;
        delete filenumform;
        delete pattform;
        return;
    }
    it_logln(UnicodeString("ChoiceFormat format:") + res1);
    if (res1 != "third") it_errln("***  ChoiceFormat format (Formattable, ...) result!");

    Formattable fta[] = { (int32_t)3 };
    str = "";
    fpos = 0;
    status = U_ZERO_ERROR;
    res1 = form_pat.format( fta, 1, str, fpos, status );
    if (!chkstatus( status, "***  test_complex_example format (int32_t, ...)" )) {
        delete fileform;
        delete filenumform;
        delete pattform;
        return;
    }
    it_logln(UnicodeString("ChoiceFormat format:") + res1);
    if (res1 != "third") it_errln("***  ChoiceFormat format (Formattable[], cnt, ...) result!");

    ParsePosition parse_pos = 0;
    Formattable result;
    UnicodeString parsetext("third");
    form_pat.parse( parsetext, result, parse_pos );
    double rd = (result.getType() == Formattable::kLong) ? result.getLong() : result.getDouble();
    if (rd == 3.0) {
        it_logln("parse( ..., ParsePos ) tested.");
    }else{
        it_errln("*** ChoiceFormat parse( ..., ParsePos )!");
    }

    form_pat.parse( parsetext, result, status );
    rd = (result.getType() == Formattable::kLong) ? result.getLong() : result.getDouble();
    if (rd == 3.0) {
        it_logln("parse( ..., UErrorCode ) tested.");
    }else{
        it_errln("*** ChoiceFormat parse( ..., UErrorCode )!");
    }

    /*
    UClassID classID = ChoiceFormat::getStaticClassID();
    if (classID == form_pat.getDynamicClassID()) {
        it_out << "getStaticClassID and getDynamicClassID tested." << endl;
    }else{
        it_errln("*** getStaticClassID and getDynamicClassID!");
    }
    */

    it_logln();

    delete fileform; 
    delete filenumform;
    delete pattform;
}


/**
 * Test new closure API
 */
void TestChoiceFormat::TestClosures(void) {
    // Construct open, half-open, half-open (the other way), and closed
    // intervals.  Do this both using arrays and using a pattern.

    // 'fmt1' is created using arrays
    UBool T = TRUE, F = FALSE;
    // 0:   ,1)
    // 1: [1,2]
    // 2: (2,3]
    // 3: (3,4)
    // 4: [4,5)
    // 5: [5,
    double limits[]  = { 0, 1, 2, 3, 4, 5 };
    UBool closures[] = { F, F, T, T, F, F };
    UnicodeString fmts[] = {
        ",1)", "[1,2]", "(2,3]", "(3,4)", "[4,5)", "[5,"
    };
    ChoiceFormat fmt1(limits, closures, fmts, 6);

    // 'fmt2' is created using a pattern; it should be equivalent
    UErrorCode status = U_ZERO_ERROR;
    const char* PAT = "0#,1)|1#[1,2]|2<(2,3]|3<(3,4)|4#[4,5)|5#[5,";
    ChoiceFormat fmt2(PAT, status);
    if (U_FAILURE(status)) {
        errln("FAIL: ChoiceFormat constructor failed");
        return;
    }
    
    // Check the patterns
    UnicodeString str;
    fmt1.toPattern(str);
    if (str == PAT) {
        logln("Ok: " + str);
    } else {
        errln("FAIL: " + str + ", expected " + PAT);
    }
    str.truncate(0);

    // Check equality
    if (fmt1 != fmt2) {
        errln("FAIL: fmt1 != fmt2");
    }

    int32_t i;
    int32_t count2 = 0;
    const double *limits2 = fmt2.getLimits(count2);
    const UBool *closures2 = fmt2.getClosures(count2);

    if((count2 != 6) || !limits2 || !closures2) {
        errln("FAIL: couldn't get limits or closures");
    } else {
        for(i=0;i<count2;i++) {
          logln("#%d/%d: limit %g closed %s\n",
                i, count2,
                limits2[i],
                closures2[i] ?"T":"F");
          if(limits2[i] != limits[i]) {
            errln("FAIL: limit #%d = %g, should be %g\n", i, limits2[i], limits[i]);
          }
          if((closures2[i]!=0) != (closures[i]!=0)) {
            errln("FAIL: closure #%d = %s, should be %s\n", i, closures2[i]?"T":"F", closures[i]?"T":"F");
          }
        }
    }

    // Now test both format objects
    UnicodeString exp[] = {
        /*-0.5 => */ ",1)",
        /* 0.0 => */ ",1)",
        /* 0.5 => */ ",1)",
        /* 1.0 => */ "[1,2]",
        /* 1.5 => */ "[1,2]",
        /* 2.0 => */ "[1,2]",
        /* 2.5 => */ "(2,3]",
        /* 3.0 => */ "(2,3]",
        /* 3.5 => */ "(3,4)",
        /* 4.0 => */ "[4,5)",
        /* 4.5 => */ "[4,5)",
        /* 5.0 => */ "[5,",
        /* 5.5 => */ "[5,"
    };

    // Each format object should behave exactly the same
    ChoiceFormat* FMT[] = { &fmt1, &fmt2 };
    for (int32_t pass=0; pass<2; ++pass) {
        int32_t j=0;
        for (int32_t ix=-5; ix<=55; ix+=5) {
            double x = ix / 10.0; // -0.5 to 5.5 step +0.5
            FMT[pass]->format(x, str);
            if (str == exp[j]) {
                logln((UnicodeString)"Ok: " + x + " => " + str);
            } else {
                errln((UnicodeString)"FAIL: " + x + " => " + str +
                      ", expected " + exp[j]);
            }
            str.truncate(0);
            ++j;
        }
    }
}

/**
 * Helper for TestPatterns()
 */
void TestChoiceFormat::_testPattern(const char* pattern,
                                    UBool isValid,
                                    double v1, const char* str1,
                                    double v2, const char* str2,
                                    double v3, const char* str3) {
    UErrorCode ec = U_ZERO_ERROR;
    ChoiceFormat fmt(pattern, ec);
    if (!isValid) {
        if (U_FAILURE(ec)) {
            logln((UnicodeString)"Ok: " + pattern + " failed");
        } else {
            logln((UnicodeString)"FAIL: " + pattern + " accepted");
        }
        return;
    }
    if (U_FAILURE(ec)) {
        errln((UnicodeString)"FAIL: ChoiceFormat(" + pattern + ") failed");
        return;
    } else {
        logln((UnicodeString)"Ok: Pattern: " + pattern);
    }
    UnicodeString out;
    logln((UnicodeString)"  toPattern: " + fmt.toPattern(out));

    double v[] = {v1, v2, v3};
    const char* str[] = {str1, str2, str3};
    for (int32_t i=0; i<3; ++i) {
        out.truncate(0);
        fmt.format(v[i], out);
        if (out == str[i]) {
            logln((UnicodeString)"Ok: " + v[i] + " => " + out); 
        } else {
            errln((UnicodeString)"FAIL: " + v[i] + " => " + out +
                  ", expected " + str[i]);
        }
    }
}

/**
 * Test applyPattern
 */
void TestChoiceFormat::TestPatterns(void) {
    // Try a pattern that isolates a single value.  Create
    // three ranges: [-Inf,1.0) [1.0,1.0] (1.0,+Inf]
    _testPattern("0.0#a|1.0#b|1.0<c", TRUE,
                 1.0 - 1e-9, "a",
                 1.0, "b",
                 1.0 + 1e-9, "c");

    // Try an invalid pattern that isolates a single value.
    // [-Inf,1.0) [1.0,1.0) [1.0,+Inf]
    _testPattern("0.0#a|1.0#b|1.0#c", FALSE,
                 0, 0, 0, 0, 0, 0);

    // Another
    // [-Inf,1.0] (1.0,1.0) [1.0,+Inf]
    _testPattern("0.0#a|1.0<b|1.0#c", FALSE,
                 0, 0, 0, 0, 0, 0);
    // Another
    // [-Inf,1.0] (1.0,1.0] (1.0,+Inf]
    _testPattern("0.0#a|1.0<b|1.0<c", FALSE,
                 0, 0, 0, 0, 0, 0);

    // Try a grossly invalid pattern.
    // [-Inf,2.0) [2.0,1.0) [1.0,+Inf]
    _testPattern("0.0#a|2.0#b|1.0#c", FALSE,
                 0, 0, 0, 0, 0, 0);
}

void TestChoiceFormat::TestChoiceFormatToPatternOverflow() 
{
    static const double limits[] = {0.1e-78, 1e13, 0.1e78};
    UnicodeString monthNames[] = { "one", "two", "three" };
    ChoiceFormat fmt(limits, monthNames, sizeof(limits)/sizeof(limits[0]));
    UnicodeString patStr, expectedPattern1("1e-79#one|10000000000000#two|1e+77#three"), 
        expectedPattern2("1e-079#one|10000000000000#two|1e+077#three");
    fmt.toPattern(patStr);
    if (patStr != expectedPattern1 && patStr != expectedPattern2) {
        errln("ChoiceFormat returned \"" + patStr + "\" instead of \"" + expectedPattern1 + " or " + expectedPattern2 + "\"");
    }
}

#endif /* #if !UCONFIG_NO_FORMATTING */
