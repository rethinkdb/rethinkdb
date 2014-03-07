/***************************************************************************
*
*   Copyright (C) 2000-2009, International Business Machines
*   Corporation and others.  All Rights Reserved.
*
************************************************************************
*   Date        Name        Description
*   03/09/2000   Madhu        Creation.
************************************************************************/

#include "unicode/utypes.h"

#if !UCONFIG_NO_TRANSLITERATION

#include "cpdtrtst.h"
#include "unicode/utypes.h"
#include "unicode/translit.h"
#include "unicode/uniset.h"
#include "cpdtrans.h"
#include "cmemory.h"

//---------------------------------------------
// runIndexedTest
//---------------------------------------------

void
CompoundTransliteratorTest::runIndexedTest(int32_t index, UBool exec,
                                           const char* &name, char* /*par*/) {
    switch (index) {
        TESTCASE(0,TestConstruction);
        TESTCASE(1,TestCloneEqual);
        TESTCASE(2,TestGetCount);
        TESTCASE(3,TestGetSetAdoptTransliterator);
        TESTCASE(4,TestTransliterate);
        default: name = ""; break;
    }
}

void CompoundTransliteratorTest::TestConstruction(){
     logln("Testing the construction of the compound Transliterator");
   UnicodeString names[]={"Greek-Latin", "Latin-Devanagari", "Devanagari-Latin", "Latin-Greek"};
   UParseError parseError;
   UErrorCode status=U_ZERO_ERROR;
   Transliterator* t1=Transliterator::createInstance(names[0], UTRANS_FORWARD, parseError, status);
   Transliterator* t2=Transliterator::createInstance(names[1], UTRANS_FORWARD, parseError, status);
   Transliterator* t3=Transliterator::createInstance(names[2], UTRANS_FORWARD, parseError, status);
   Transliterator* t4=Transliterator::createInstance(names[3], UTRANS_FORWARD, parseError, status);
   if(U_FAILURE(status)){
       dataerrln("Transliterator construction failed - %s", u_errorName(status));
       return;
   }


   Transliterator* transarray1[]={t1};
   Transliterator* transarray2[]={t1, t4};
   Transliterator* transarray3[]={t4, t1, t2};
   Transliterator* transarray4[]={t1, t2, t3, t4};

   Transliterator** transarray[4]; 
   transarray[0] = transarray1;
   transarray[1] = transarray2;
   transarray[2] = transarray3;
   transarray[3] = transarray4;

   const UnicodeString IDs[]={
       names[0], 
       names[0]+";"+names[3], 
       names[3]+";"+names[1]+";"+names[2], 
       names[0]+";"+names[1]+";"+names[2]+";"+names[3] 
   };

   uint16_t i=0;
   for(i=0; i<4; i++){
       status = U_ZERO_ERROR;
       CompoundTransliterator *cpdtrans=new CompoundTransliterator(IDs[i],parseError, status);
       if (U_FAILURE(status)) {
           errln("Construction using CompoundTransliterator(UnicodeString&, Direction, UnicodeFilter*)  failed");
       }
       delete cpdtrans;

       CompoundTransliterator *cpdtrans2=new CompoundTransliterator(transarray[i], i+1);
       if(cpdtrans2 == 0){
           errln("Construction using CompoundTransliterator(Transliterator* const transliterators[], "
                           "int32_t count, UnicodeFilter* adoptedFilter = 0)  failed");
           continue;
       }
       CompoundTransliterator *copycpd=new CompoundTransliterator(*cpdtrans2);
       if(copycpd->getCount() != cpdtrans2->getCount() || copycpd->getID() != cpdtrans2->getID()) {
           errln("Copy construction failed");
           continue;
       }


       delete copycpd;
       delete cpdtrans2;

   }
   {
    /*Test Jitterbug 914 */
    UErrorCode err = U_ZERO_ERROR;
    CompoundTransliterator  cpdTrans(UnicodeString("Latin-Hangul"),UTRANS_REVERSE,NULL,parseError,err);
    UnicodeString newID =cpdTrans.getID();
    if(newID!=UnicodeString("Hangul-Latin")){
        errln(UnicodeString("Test for Jitterbug 914 for cpdTrans(UnicodeString(\"Latin-Hangul\"),UTRANS_REVERSE,NULL,err) failed"));
    }
   }
   delete t1;
   delete t2;
   delete t3;
   delete t4;
 
}

void CompoundTransliteratorTest::TestCloneEqual(){ 
    logln("Testing the clone() and equality operator functions of Compound Transliterator");
    UErrorCode status = U_ZERO_ERROR;
    UParseError parseError;
    CompoundTransliterator  *ct1=new CompoundTransliterator("Greek-Latin;Latin-Devanagari",parseError,status);
    if(U_FAILURE(status)){
        dataerrln("construction failed - %s", u_errorName(status));
        delete ct1;
        return;
    }
    CompoundTransliterator  *ct2=new CompoundTransliterator("Greek-Latin", parseError, status);
    if(U_FAILURE(status)){
        errln("construction failed");
        delete ct1;
        delete ct2;
        return;
    }
    CompoundTransliterator *copyct1=new CompoundTransliterator(*ct1);
    if(copyct1 == 0){
        errln("copy construction failed");
        return;
    }
    CompoundTransliterator *copyct2=new CompoundTransliterator(*ct2);
    if(copyct2 == 0){
        errln("copy construction failed");
        return;
    }
    CompoundTransliterator equalct1=*copyct1;
    CompoundTransliterator equalct2=*copyct2;

    if(copyct1->getID()     != ct1->getID()    || copyct2->getID()    != ct2->getID()    || 
        copyct1->getCount() != ct1->getCount() || copyct2->getCount() != ct2->getCount() ||
        copyct2->getID()    == ct1->getID()    || copyct1->getID()    == ct2->getID()    ||
        copyct2->getCount() == ct1->getCount() || copyct1->getCount() == ct2->getCount() ){
        errln("Error: copy constructors failed");
    }

    if(equalct1.getID()     != ct1->getID()        || equalct2.getID()    != ct2->getID()     || 
        equalct1.getID()    != copyct1->getID()    || equalct2.getID()    != copyct2->getID() || 
        equalct1.getCount() != ct1->getCount()     || equalct2.getCount() != ct2->getCount()  ||
        copyct2->getID()    == ct1->getID()        || copyct1->getID()    == ct2->getID()     ||
        equalct1.getCount() != copyct1->getCount() || equalct2.getCount() != copyct2->getCount() ||
        equalct2.getCount() == ct1->getCount()     || equalct1.getCount() == ct2->getCount() ) {
        errln("Error: =operator or copy constructor failed");
    }

    CompoundTransliterator *clonect1a=(CompoundTransliterator*)ct1->clone();
    CompoundTransliterator *clonect1b=(CompoundTransliterator*)equalct1.clone();
    CompoundTransliterator *clonect2a=(CompoundTransliterator*)ct2->clone();
    CompoundTransliterator *clonect2b=(CompoundTransliterator*)copyct2->clone();


    if(clonect1a->getID()  != ct1->getID()       || clonect1a->getCount() != ct1->getCount()        ||
        clonect1a->getID() != clonect1b->getID() || clonect1a->getCount() != clonect1b->getCount()  ||
        clonect1a->getID() != equalct1.getID()   || clonect1a->getCount() != equalct1.getCount()    ||
        clonect1a->getID() != copyct1->getID()   || clonect1a->getCount() != copyct1->getCount()    ||

        clonect2b->getID() != ct2->getID()       || clonect2a->getCount() != ct2->getCount()        ||
        clonect2a->getID() != clonect2b->getID() || clonect2a->getCount() != clonect2b->getCount()  ||
        clonect2a->getID() != equalct2.getID()   || clonect2a->getCount() != equalct2.getCount()    ||
        clonect2b->getID() != copyct2->getID()   || clonect2b->getCount() != copyct2->getCount()  ) {
        errln("Error: clone() failed");
    }

    delete ct1;
    delete ct2;
    delete copyct1;
    delete copyct2;
    delete clonect1a;
    delete clonect1b;
    delete clonect2a;
    delete clonect2b;

}

void CompoundTransliteratorTest::TestGetCount(){
    logln("Testing the getCount() API of CompoundTransliterator");
    UErrorCode status = U_ZERO_ERROR;
    UParseError parseError;
    CompoundTransliterator *ct1=new CompoundTransliterator("Halfwidth-Fullwidth;Fullwidth-Halfwidth", parseError, status);
    CompoundTransliterator *ct2=new CompoundTransliterator("Any-Hex;Hex-Any;Cyrillic-Latin;Latin-Cyrillic", parseError, status);
    CompoundTransliterator *ct3=(CompoundTransliterator*)ct1;
    if (U_FAILURE(status)) {
        dataerrln("FAILED: CompoundTransliterator constructor failed - %s", u_errorName(status));
        return;
    }
    CompoundTransliterator *ct4=new CompoundTransliterator("Latin-Devanagari", parseError, status);
    CompoundTransliterator *ct5=new CompoundTransliterator(*ct4);

    if (U_FAILURE(status)) {
        errln("FAILED: CompoundTransliterator constructor failed");
        return;
    }
    if(ct1->getCount() == ct2->getCount() || ct1->getCount() != ct3->getCount() || 
        ct2->getCount() == ct3->getCount() || 
        ct4->getCount() != ct5->getCount() || ct4->getCount() == ct1->getCount() ||
        ct4->getCount() == ct2->getCount() || ct4->getCount() == ct3->getCount()  ||
        ct5->getCount() == ct2->getCount() || ct5->getCount() == ct3->getCount()  ) {
        errln("Error: getCount() failed");
    }

    /* Quick test getTargetSet(), only test that it doesn't die.  TODO:  a better test. */
    UnicodeSet ts;
    UnicodeSet *retUS = NULL;
    retUS = &ct1->getTargetSet(ts);
    if (retUS != &ts || ts.size() == 0) {
        errln("CompoundTransliterator::getTargetSet() failed.\n");
    }

    /* Quick test getSourceSet(), only test that it doesn't die.  TODO:  a better test. */
    UnicodeSet ss;
    retUS = NULL;
    retUS = &ct1->getSourceSet(ss);
    if (retUS != &ss || ss.size() == 0) {
        errln("CompoundTransliterator::getSourceSet() failed.\n");
    }

    delete ct1;
    delete ct2;
    delete ct4;
    delete ct5;
}

void CompoundTransliteratorTest::TestGetSetAdoptTransliterator(){
    logln("Testing the getTransliterator() API of CompoundTransliterator");
    UnicodeString ID("Latin-Greek;Greek-Latin;Latin-Devanagari;Devanagari-Latin;Latin-Cyrillic;Cyrillic-Latin;Any-Hex;Hex-Any");
    UErrorCode status = U_ZERO_ERROR;
    UParseError parseError;
    CompoundTransliterator *ct1=new CompoundTransliterator(ID, parseError, status);
    if(U_FAILURE(status)){
        dataerrln("CompoundTransliterator construction failed - %s", u_errorName(status));
        return;
    }
    int32_t count=ct1->getCount();
    UnicodeString *array=split(ID, 0x003b, count);
    int i;
    for(i=0; i < count; i++){
        UnicodeString child= ct1->getTransliterator(i).getID();
        if(child != *(array+i)){
            errln("Error getTransliterator() failed: Expected->" + *(array+i) + " Got->" + child);
        }else {
            logln("OK: getTransliterator() passed: Expected->" + *(array+i) + " Got->" + child);
        }
    }
    delete []array;

    logln("Testing setTransliterator() API of CompoundTransliterator");
    UnicodeString ID2("Hex-Any;Any-Hex;Latin-Cyrillic;Cyrillic-Latin;Halfwidth-Fullwidth;Fullwidth-Halfwidth");
    array=split(ID2, 0x003b, count);
    Transliterator** transarray=new Transliterator*[count];
    for(i=0;i<count;i++){
        transarray[i]=Transliterator::createInstance(*(array+i), UTRANS_FORWARD, parseError, status);
        if(U_FAILURE(status)){
            errln("Error could not create Transliterator with ID :"+*(array+i));
        }else{
            logln("The ID for the transltierator created is " + transarray[i]->getID());
        }
        status = U_ZERO_ERROR;
    }

    /*setTransliterator and adoptTransliterator */

    ct1->setTransliterators(transarray, count);
    if(ct1->getCount() != count || ct1->getID() != ID2){
        errln((UnicodeString)"Error: setTransliterators() failed.\n\t Count:- expected->" + count + (UnicodeString)".  got->" + ct1->getCount() +
                                                   (UnicodeString)"\n\tID   :- expected->" + ID2 + (UnicodeString)".  got->" + ct1->getID());
    }
    else{
        logln("OK: setTransliterators() passed"); 
    }
    /*UnicodeString temp;
    for(i=0;i<count-1;i++){
        temp.append(ct1->getTransliterator(i).getID());
        temp.append(";");
    }
    temp.append(ct1->getTransliterator(i).getID());
    if(temp != ID2){
        errln("Error: setTransliterator() failed.  Expected->" + ID2 + "\nGot->" + temp);
    }
    else{
        logln("OK: setTransliterator() passed");
    }*/
    logln("Testing adoptTransliterator() API of CompoundTransliterator");
    UnicodeString ID3("Latin-Katakana");
    Transliterator **transarray2=(Transliterator **)uprv_malloc(sizeof(Transliterator*)*1);
    transarray2[0] = Transliterator::createInstance(ID3,UTRANS_FORWARD,parseError,status);
    if (transarray2[0] != 0) {
        ct1->adoptTransliterators(transarray2, 1);
    }
    if(ct1->getCount() != 1 || ct1->getID() != ID3){
        errln((UnicodeString)"Error: adoptTransliterators() failed.\n\t Count:- expected->1" + (UnicodeString)".  got->" + ct1->getCount() +
                                                   (UnicodeString)"\n\tID   :- expected->" + ID3 + (UnicodeString)".  got->" + ct1->getID());
    }
    else{
        logln("OK: adoptTranslterator() passed");
    }
    delete ct1;
    for(i=0;i<count;i++){
        delete transarray[i];
    }
    delete []transarray;
    delete []array;
}

/**
 * Splits a UnicodeString
 */
UnicodeString* CompoundTransliteratorTest::split(const UnicodeString& str, UChar seperator, int32_t& count) {

    //get the count
    int32_t i;
    count =1;
    for(i=0; i<str.length(); i++){
        if(str.charAt(i) == seperator)
            count++;
    }
    // make an array 
    UnicodeString* result = new UnicodeString[count];
    int32_t last = 0;
    int32_t current = 0;
    for (i = 0; i < str.length(); ++i) {
        if (str.charAt(i) == seperator) {
            str.extractBetween(last, i, result[current]);
            last = i+1;
            current++;
        }
    }
    str.extractBetween(last, i, result[current]);
    return result;
}
void CompoundTransliteratorTest::TestTransliterate(){
    logln("Testing the handleTransliterate() API of CompoundTransliterator");
    UErrorCode status = U_ZERO_ERROR;
    UParseError parseError;
    CompoundTransliterator *ct1=new CompoundTransliterator("Any-Hex;Hex-Any",parseError, status);
    if(U_FAILURE(status)){
        errln("CompoundTransliterator construction failed");
    }else {
#if 0
    // handleTransliterate is a protected method that was erroneously made
    // public.  It is not public API that needs to be tested.
        UnicodeString s("abcabc");
        expect(*ct1, s, s);
        UTransPosition index = { 0, 0, 0, 0 };
        UnicodeString rsource2(s);
        UnicodeString expectedResult=s;
        ct1->handleTransliterate(rsource2, index, FALSE);
        expectAux(ct1->getID() + ":String, index(0,0,0), incremental=FALSE", rsource2 + "->" + rsource2, rsource2==expectedResult, expectedResult);
        UTransPosition _index = {1,3,2,3};
        uprv_memcpy(&index, &_index, sizeof(index));
        UnicodeString rsource3(s);
        ct1->handleTransliterate(rsource3, index, TRUE); 
        expectAux(ct1->getID() + ":String, index(1,2,3), incremental=TRUE", rsource3 + "->" + rsource3, rsource3==expectedResult, expectedResult);
#endif
    }
    delete ct1;
    UnicodeString Data[]={
             //ID, input string, transliterated string
             "Any-Hex;Hex-Any;Any-Hex",     "hello",  UnicodeString("\\u0068\\u0065\\u006C\\u006C\\u006F", ""), 
             "Any-Hex;Hex-Any",                 "hello! How are you?",  "hello! How are you?",
             //"Devanagari-Latin;Latin-Devanagari",        CharsToUnicodeString("\\u092D\\u0948'\\u0930'\\u0935"),  CharsToUnicodeString("\\u092D\\u0948\\u0930\\u0935"), // quotes lost
             "Latin-Cyrillic;Cyrillic-Latin",           "a'b'k'd'e'f'g'h'i'j'Shch'shch'zh'h", "a'b'k'd'e'f'g'h'i'j'Shch'shch'zh'h", //"abkdefghijShchshchzhh",
             "Latin-Greek;Greek-Latin",                 "ABGabgAKLMN", "ABGabgAKLMN",
             //"Latin-Arabic;Arabic-Latin",               "Ad'r'a'b'i'k'dh'dd'gh", "Adrabikdhddgh",
             "Hiragana-Katakana",                       CharsToUnicodeString("\\u3041\\u308f\\u3099\\u306e\\u304b\\u3092\\u3099"), 
                                                                 CharsToUnicodeString("\\u30A1\\u30f7\\u30ce\\u30ab\\u30fa"),  
             "Hiragana-Katakana;Katakana-Hiragana",     CharsToUnicodeString("\\u3041\\u308f\\u3099\\u306e\\u304b\\u3051"), 
                                                                 CharsToUnicodeString("\\u3041\\u308f\\u3099\\u306e\\u304b\\u3051"),
             "Katakana-Hiragana;Hiragana-Katakana",     CharsToUnicodeString("\\u30A1\\u30f7\\u30ce\\u30f5\\u30f6"), 
                                                                 CharsToUnicodeString("\\u30A1\\u30f7\\u30ce\\u30ab\\u30b1"),  
             "Latin-Katakana;Katakana-Latin",                   CharsToUnicodeString("vavivuvevohuzizuzonyinyunyasesuzezu"), 
                                                                 CharsToUnicodeString("vavivuvevohuzizuzonyinyunyasesuzezu"),  
    };
    uint32_t i;
    for(i=0; i<sizeof(Data)/sizeof(Data[0]); i=i+3){
        UErrorCode status = U_ZERO_ERROR;

        CompoundTransliterator *ct2=new CompoundTransliterator(Data[i+0], parseError, status);
        if(U_FAILURE(status)){
            dataerrln("CompoundTransliterator construction failed for " + Data[i+0] + " - " + u_errorName(status));
        } else {
            expect(*ct2, Data[i+1], Data[i+2]);
        }
        delete ct2;
    }

}



//======================================================================
// Support methods
//======================================================================
void CompoundTransliteratorTest::expect(const CompoundTransliterator& t,
                                const UnicodeString& source,
                                const UnicodeString& expectedResult) {
   
    UnicodeString rsource(source);
    t.transliterate(rsource);
    expectAux(t.getID() + ":Replaceable", source + "->" + rsource, rsource==expectedResult, expectedResult);

    // Test transliterate (incremental) transliteration -- 
    rsource.remove();
    rsource.append(source);
    UTransPosition index;
    index.contextStart =0;
    index.contextLimit = source.length();
    index.start = 0;
    index.limit = source.length();
    UErrorCode ec = U_ZERO_ERROR;
    t.transliterate(rsource, index, ec);
    t.finishTransliteration(rsource,index);
    expectAux(t.getID() + ":handleTransliterate ", source + "->" + rsource, rsource==expectedResult, expectedResult);

}

void CompoundTransliteratorTest::expectAux(const UnicodeString& tag,
                                   const UnicodeString& summary, UBool pass,
                                   const UnicodeString& expectedResult) {
    if (pass) {
        logln(UnicodeString("(")+tag+") " + prettify(summary));
    } else {
        errln(UnicodeString("FAIL: (")+tag+") "
              + prettify(summary)
              + ", expected " + prettify(expectedResult));
    }
}

#endif /* #if !UCONFIG_NO_TRANSLITERATION */
