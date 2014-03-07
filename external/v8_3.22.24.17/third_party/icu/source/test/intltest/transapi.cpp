/************************************************************************
 * COPYRIGHT: 
 * Copyright (c) 2000-2009, International Business Machines Corporation
 * and others. All Rights Reserved.
 ************************************************************************/
/************************************************************************
*   Date        Name        Description
*   1/03/2000   Madhu        Creation.
************************************************************************/

#include "unicode/utypes.h"

#if !UCONFIG_NO_TRANSLITERATION

#include "ittrans.h"
#include "transapi.h"
#include "unicode/utypes.h"
#include "unicode/translit.h"
#include "unicode/unifilt.h"
#include "cpdtrans.h"
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include "unicode/rep.h"
#include "unicode/locid.h"
#include "unicode/uniset.h"

int32_t getInt(UnicodeString str)
{
    char buffer[20];
    int len=str.length();
    if(len>=20) {
        len=19;
    }
    str.extract(0, len, buffer, "");
    buffer[len]=0;
    return atoi(buffer);
}

//---------------------------------------------
// runIndexedTest
//---------------------------------------------

void
TransliteratorAPITest::runIndexedTest(int32_t index, UBool exec,
                                      const char* &name, char* /*par*/) {
    switch (index) {
        TESTCASE(0,TestgetInverse);
        TESTCASE(1,TestgetID);
        TESTCASE(2,TestGetDisplayName);
        TESTCASE(3,TestTransliterate1);
        TESTCASE(4,TestTransliterate2);
        TESTCASE(5,TestTransliterate3);
        TESTCASE(6,TestSimpleKeyboardTransliterator);
        TESTCASE(7,TestKeyboardTransliterator1);
        TESTCASE(8,TestKeyboardTransliterator2);
        TESTCASE(9,TestKeyboardTransliterator3);
        TESTCASE(10,TestGetAdoptFilter);
        TESTCASE(11,TestClone);
        TESTCASE(12,TestNullTransliterator);
        TESTCASE(13,TestRegisterUnregister);
        TESTCASE(14,TestUnicodeFunctor);
        default: name = ""; break;
    }
}


void TransliteratorAPITest::TestgetID() {
    UnicodeString trans="Latin-Greek";
    UnicodeString ID;
    UParseError parseError;
    UErrorCode status = U_ZERO_ERROR;
    Transliterator* t= Transliterator::createInstance(trans, UTRANS_FORWARD, parseError, status);
    if(t==0 || U_FAILURE(status)){
        dataerrln("FAIL: construction of Latin-Greek -  %s",u_errorName(status));
        return;
    }else{
        ID= t->getID();
        if(ID != trans)
            errln("FAIL: getID returned " + ID + " instead of Latin-Greek");
        delete t;
    }
    int i;
    for (i=0; i<Transliterator::countAvailableIDs(); i++){
        status = U_ZERO_ERROR;
        ID = (UnicodeString) Transliterator::getAvailableID(i);
        if(ID.indexOf("Thai")>-1){
            continue;
        }   
        t = Transliterator::createInstance(ID, UTRANS_FORWARD, parseError, status);
        if(t == 0){
            errln("FAIL: " + ID);
            continue;
        }
        if(ID != t->getID())
            errln("FAIL: getID() returned " + t->getID() + " instead of " + ID);
        delete t;
    }
    ID=(UnicodeString)Transliterator::getAvailableID(i);
    if(ID != (UnicodeString)Transliterator::getAvailableID(0)){
        errln("FAIL: calling getAvailableID(index > coundAvailableIDs) should make index=0\n");
    }

    Transliterator* t1=Transliterator::createInstance("Latin-Devanagari", UTRANS_FORWARD, parseError, status);
    Transliterator* t2=Transliterator::createInstance("Latin-Greek", UTRANS_FORWARD, parseError, status);
    if(t1 ==0 || t2 == 0){
        errln("FAIL: construction");
        return;
    }
    Transliterator* t3=t1->clone();
    Transliterator* t4=t2->clone();

    if(t1->getID() != t3->getID() || t2->getID() != t4->getID() || 
       t1->getID() == t4->getID() || t2->getID() == t3->getID() || 
       t1->getID()== t4->getID() )
            errln("FAIL: getID or clone failed");


    Transliterator* t5=Transliterator::createInstance("Latin-Devanagari", UTRANS_FORWARD, parseError, status);
    if(t5 == 0)
        errln("FAIL: construction");
    else if(t1->getID() != t5->getID() || t5->getID() != t3->getID() || t1->getID() != t3->getID())
        errln("FAIL: getID or clone failed");


    delete t1;
    delete t2;
    delete t3;
    delete t4;
    delete t5;
}

void TransliteratorAPITest::TestgetInverse() {
     UErrorCode status = U_ZERO_ERROR;
     UParseError parseError;
     Transliterator* t1    = Transliterator::createInstance("Katakana-Latin", UTRANS_FORWARD, parseError, status);
     Transliterator* invt1 = Transliterator::createInstance("Latin-Katakana", UTRANS_FORWARD, parseError, status);
     Transliterator* t2    = Transliterator::createInstance("Latin-Devanagari", UTRANS_FORWARD, parseError, status);
     Transliterator* invt2 = Transliterator::createInstance("Devanagari-Latin", UTRANS_FORWARD, parseError, status);
     if(t1 == 0 || invt1 == 0 || t2 == 0 || invt2 == 0) {
         dataerrln("FAIL: in instantiation - %s", u_errorName(status));
         return;
     }

     Transliterator* inverse1=t1->createInverse(status);
     Transliterator* inverse2=t2->createInverse(status);
     if(inverse1->getID() != invt1->getID() || inverse2->getID() != invt2->getID()
        || inverse1->getID() == invt2->getID() || inverse2->getID() == invt1->getID() ) 
        errln("FAIL: getInverse() ");

     UnicodeString TransID[]={
       "Halfwidth-Fullwidth",
       "Fullwidth-Halfwidth",
       "Greek-Latin" ,
       "Latin-Greek", 
       //"Arabic-Latin", // removed in 2.0
       //"Latin-Arabic",
       "Katakana-Latin",
       "Latin-Katakana",
       //"Hebrew-Latin", // removed in 2.0
       //"Latin-Hebrew",
       "Cyrillic-Latin", 
       "Latin-Cyrillic", 
       "Devanagari-Latin", 
       "Latin-Devanagari", 
       "Any-Hex",
       "Hex-Any"
     };
     for(uint32_t i=0; i<sizeof(TransID)/sizeof(TransID[0]); i=i+2){
         Transliterator *trans1=Transliterator::createInstance(TransID[i], UTRANS_FORWARD, parseError, status);
         if(t1 == 0){
           errln("FAIL: in instantiation for" + TransID[i]);
           continue;
         }
         Transliterator *inver1=trans1->createInverse(status);
         if(inver1->getID() != TransID[i+1] )
             errln("FAIL :getInverse() for " + TransID[i] + " returned " + inver1->getID() + " instead of " + TransID[i+1]);
         delete inver1;
         delete trans1;
     }
     delete t1;
     delete t2;
     delete invt1;
     delete invt2;
     delete inverse1;
     delete inverse2;

}

void TransliteratorAPITest::TestClone(){
    Transliterator *t1, *t2, *t3, *t4;
    UErrorCode status = U_ZERO_ERROR;
    UParseError parseError;
    t1=Transliterator::createInstance("Latin-Devanagari", UTRANS_FORWARD, parseError, status);
    t2=Transliterator::createInstance("Latin-Greek", UTRANS_FORWARD, parseError, status);
    if(t1 == 0 || t2 == 0){
        dataerrln("FAIL: construction - %s", u_errorName(status));
        return;
    }
    t3=t1->clone();
    t4=t2->clone();

    if(t1->getID() != t3->getID() || t2->getID() != t4->getID() )
        errln("FAIL: clone or getID failed");
    if(t1->getID()==t4->getID() || t2->getID()==t3->getID() ||  t1->getID()==t4->getID())
        errln("FAIL: clone or getID failed");
     
    delete t1;
    delete t2;
    delete t3;
    delete t4;

}

void TransliteratorAPITest::TestGetDisplayName() {
    UnicodeString dispNames[]= { 
         //ID, displayName
        //"CurlyQuotes-StraightQuotes" ,"CurlyQuotes to StraightQuotes",
          "Any-Hex"                ,"Any to Hex Escape",
          "Halfwidth-Fullwidth"        ,"Halfwidth to Fullwidth" ,
          //"Latin-Arabic"               ,"Latin to Arabic"      ,
          "Latin-Devanagari"           ,"Latin to Devanagari"  ,
          "Greek-Latin"                ,"Greek to Latin"       ,
          //"Arabic-Latin"               ,"Arabic to Latin"      ,
          "Hex-Any"                ,"Hex Escape to Any",
          "Cyrillic-Latin"             ,"Cyrillic to Latin"    ,
          "Latin-Greek"                ,"Latin to Greek"       ,
          "Latin-Katakana"                 ,"Latin to Katakana"        ,
          //"Latin-Hebrew"               ,"Latin to Hebrew"      ,
          "Katakana-Latin"                 ,"Katakana to Latin"        
      };
    UnicodeString name="";
    Transliterator* t;
    UnicodeString message;
    UErrorCode status = U_ZERO_ERROR;
    UParseError parseError;

#if UCONFIG_NO_FORMATTING
    logln("Skipping, UCONFIG_NO_FORMATTING is set\n");
    return;
#else

    for (uint32_t i=0; i<sizeof(dispNames)/sizeof(dispNames[0]); i=i+2 ) {
        t = Transliterator::createInstance(dispNames[i+0], UTRANS_FORWARD, parseError, status);
        if(t==0){
             dataerrln("FAIL: construction: " + dispNames[i+0] + " - " + u_errorName(status));
             status = U_ZERO_ERROR;
             continue;
        }
        t->getDisplayName(t->getID(), name);
        message="Display name for ID:" + t->getID();
      //  doTest(message, name, dispNames[i+1]); //!!! This will obviously fail for any locale other than english and its children!!!
        name=""; 
        t->getDisplayName(t->getID(), Locale::getUS(), name);
        message.remove();
        message.append("Display name for on english locale ID:");
        message.append(t->getID());
    // message="Display name for on english locale ID:" + t->getID();
        doTest(message, name, dispNames[i+1]);
        name="";

        delete t;
    }
#endif

}

void TransliteratorAPITest::TestTransliterate1(){

    UnicodeString Data[]={ 
         //ID, input string, transliterated string
         "Any-Hex",         "hello",    UnicodeString("\\u0068\\u0065\\u006C\\u006C\\u006F", "") ,
         "Hex-Any",         UnicodeString("\\u0068\\u0065\\u006C\\u006C\\u006F", ""), "hello"  ,
         "Latin-Devanagari",CharsToUnicodeString("bha\\u0304rata"), CharsToUnicodeString("\\u092D\\u093E\\u0930\\u0924") ,
         "Latin-Devanagari",UnicodeString("kra ksha khra gra cra dya dhya",""), CharsToUnicodeString("\\u0915\\u094D\\u0930 \\u0915\\u094D\\u0936 \\u0916\\u094D\\u0930 \\u0917\\u094D\\u0930 \\u091a\\u094D\\u0930 \\u0926\\u094D\\u092F \\u0927\\u094D\\u092F") ,

         "Devanagari-Latin",    CharsToUnicodeString("\\u092D\\u093E\\u0930\\u0924"),        CharsToUnicodeString("bh\\u0101rata"),
     //  "Contracted-Expanded", CharsToUnicodeString("\\u00C0\\u00C1\\u0042"),               CharsToUnicodeString("\\u0041\\u0300\\u0041\\u0301\\u0042") ,
     //  "Expanded-Contracted", CharsToUnicodeString("\\u0041\\u0300\\u0041\\u0301\\u0042"), CharsToUnicodeString("\\u00C0\\u00C1\\u0042") ,
         //"Latin-Arabic",        "aap",                                 CharsToUnicodeString("\\u0627\\u06A4")     ,
         //"Arabic-Latin",        CharsToUnicodeString("\\u0627\\u06A4"),                      "aap" 
    };

    UnicodeString gotResult;
    UnicodeString temp;
    UnicodeString message;
    Transliterator* t;
    logln("Testing transliterate");
    UErrorCode status = U_ZERO_ERROR;
    UParseError parseError;

    for(uint32_t i=0;i<sizeof(Data)/sizeof(Data[0]); i=i+3){
        t=Transliterator::createInstance(Data[i+0], UTRANS_FORWARD, parseError, status);
        if(t==0){
            dataerrln("FAIL: construction: " + Data[i+0] + " Error: "  + u_errorName(status));
            dataerrln("PreContext: " + prettify(parseError.preContext) + " PostContext: " + prettify( parseError.postContext) );
            status = U_ZERO_ERROR;
            continue;
        }
        gotResult = Data[i+1];
        t->transliterate(gotResult);
        message=t->getID() + "->transliterate(UnicodeString, UnicodeString) for\n\t Source:" + prettify(Data[i+1]);
        doTest(message, gotResult, Data[i+2]);

        //doubt here
        temp=Data[i+1];
        t->transliterate(temp);
        message.remove();
        message.append(t->getID());
        message.append("->transliterate(Replaceable) for \n\tSource:");
        message.append(Data[i][1]);
        doTest(message, temp, Data[i+2]);

        callEverything(t, __LINE__);
        delete t;
    }
}

void TransliteratorAPITest::TestTransliterate2(){
     //testing tranliterate(String text, int start, int limit, StringBuffer result)
   UnicodeString Data2[]={
         //ID, input string, start, limit, transliterated string
         "Any-Hex",         "hello! How are you?",  "0", "5", UnicodeString("\\u0068\\u0065\\u006C\\u006C\\u006F", ""), UnicodeString("\\u0068\\u0065\\u006C\\u006C\\u006F! How are you?", "") ,
         "Any-Hex",         "hello! How are you?",  "7", "12", UnicodeString("\\u0048\\u006F\\u0077\\u0020\\u0061", ""), UnicodeString("hello! \\u0048\\u006F\\u0077\\u0020\\u0061re you?", ""),
         "Hex-Any",         CharsToUnicodeString("\\u0068\\u0065\\u006C\\u006C\\u006F\\u0021\\u0020"), "0", "5",  "hello", "hello! "  ,
       //  "Contracted-Expanded", CharsToUnicodeString("\\u00C0\\u00C1\\u0042"),        "1", "2",  CharsToUnicodeString("\\u0041\\u0301"), CharsToUnicodeString("\\u00C0\\u0041\\u0301\\u0042") ,
         "Devanagari-Latin",    CharsToUnicodeString("\\u092D\\u093E\\u0930\\u0924"), "0", "1",  "bha", CharsToUnicodeString("bha\\u093E\\u0930\\u0924") ,
         "Devanagari-Latin",    CharsToUnicodeString("\\u092D\\u093E\\u0930\\u0924"), "1", "2",  CharsToUnicodeString("\\u0314\\u0101"), CharsToUnicodeString("\\u092D\\u0314\\u0101\\u0930\\u0924")  

    };
    logln("\n   Testing transliterate(String, int, int, StringBuffer)");
    Transliterator* t;
    UnicodeString gotResBuf;
    UnicodeString temp;
    int32_t start, limit;
    UErrorCode status = U_ZERO_ERROR;
    UParseError parseError;

    for(uint32_t i=0; i<sizeof(Data2)/sizeof(Data2[0]); i=i+6){
        t=Transliterator::createInstance(Data2[i+0], UTRANS_FORWARD, parseError, status);
        if(t==0){
            dataerrln("FAIL: construction: " + prettify(Data2[i+0]) + " - " + u_errorName(status));
            continue;
        }
        start=getInt(Data2[i+2]);
        limit=getInt(Data2[i+3]);
        Data2[i+1].extractBetween(start, limit, gotResBuf);
        t->transliterate(gotResBuf);
        //  errln("FAIL: calling transliterate on " + t->getID());
        doTest(t->getID() + ".transliterate(UnicodeString, int32_t, int32_t, UnicodeString):(" + start + "," + limit + ")  for \n\t source: " + prettify(Data2[i+1]), gotResBuf, Data2[i+4]);

        temp=Data2[i+1];
        t->transliterate(temp, start, limit);
        doTest(t->getID() + ".transliterate(Replaceable, int32_t, int32_t, ):(" + start + "," + limit + ")  for \n\t source: " + prettify(Data2[i+1]), temp, Data2[i+5]);
        status = U_ZERO_ERROR;
        callEverything(t, __LINE__);
        delete t;
        t = NULL;
    }

    status = U_ZERO_ERROR;
    logln("\n   Try calling transliterate with illegal start and limit values");
    t=Transliterator::createInstance("Any-Hex", UTRANS_FORWARD, parseError, status);
    if(U_FAILURE(status)) {
      errln("Error creating transliterator %s", u_errorName(status));
      delete t;
      return;
    }
    gotResBuf = temp = "try start greater than limit";
    t->transliterate(gotResBuf, 10, 5);
    if(gotResBuf == temp) {
        logln("OK: start greater than limit value handled correctly");
    } else {
        errln("FAIL: start greater than limit value returned" + gotResBuf);
    }

    callEverything(t, __LINE__);
    delete t;

}
void TransliteratorAPITest::TestTransliterate3(){
    UnicodeString rs="This is the replaceable String";
    UnicodeString Data[] = {
        "0",  "0",  "This is the replaceable String",
        "2",  "3",  UnicodeString("Th\\u0069s is the replaceable String", ""),
        "21", "23", UnicodeString("Th\\u0069s is the repl\\u0061\\u0063eable String", ""),
        "14", "17", UnicodeString("Th\\u0069s is t\\u0068\\u0065\\u0020repl\\u0061\\u0063eable String", ""),
    };
    int start, limit;
    UnicodeString message;
    UParseError parseError;
    UErrorCode status = U_ZERO_ERROR;
    Transliterator *t=Transliterator::createInstance("Any-Hex", UTRANS_FORWARD, parseError, status);
    if(U_FAILURE(status)) {
      errln("Error creating transliterator %s", u_errorName(status));
      delete t;
      return;
    }

    if(t == 0)
        errln("FAIL : construction");
    for(uint32_t i=0; i<sizeof(Data)/sizeof(Data[0]); i=i+3){
        start=getInt(Data[i+0]);
        limit=getInt(Data[i+1]);
        t->transliterate(rs, start, limit);
        message=t->getID() + ".transliterate(ReplaceableString, start, limit):("+start+","+limit+"):";
        doTest(message, rs, Data[i+2]); 
    }
    delete t;
}

void TransliteratorAPITest::TestSimpleKeyboardTransliterator(){
    logln("simple call to transliterate");
    UErrorCode status=U_ZERO_ERROR;
    UParseError parseError;
    Transliterator* t=Transliterator::createInstance("Any-Hex", UTRANS_FORWARD, parseError, status);
    if(t == 0) {
        UnicodeString context;

        if (parseError.preContext[0]) {
            context += (UnicodeString)" at " + parseError.preContext;
        }
        if (parseError.postContext[0]) {
            context += (UnicodeString)" | " + parseError.postContext;
        }
        errln((UnicodeString)"FAIL: can't create Any-Hex, " +
              (UnicodeString)u_errorName(status) + context);
        return;
    }
    UTransPosition index={19,20,20,20};
    UnicodeString rs= "Transliterate this-''";
    UnicodeString insertion="abc";
    UnicodeString expected=UnicodeString("Transliterate this-'\\u0061\\u0062\\u0063'", "");
    t->transliterate(rs, index, insertion, status);
    if(U_FAILURE(status))
        errln("FAIL: " + t->getID()+ ".translitere(Replaceable, int[], UnicodeString, UErrorCode)-->" + (UnicodeString)u_errorName(status));
    t->finishTransliteration(rs, index);
    UnicodeString message="transliterate";
    doTest(message, rs, expected);

    logln("try calling transliterate with invalid index values");
    UTransPosition index1[]={
        //START, LIMIT, CURSOR
        {10, 10, 12, 10},   //invalid since CURSOR>LIMIT valid:-START <= CURSOR <= LIMIT
        {17, 16, 17, 17},   //invalid since START>LIMIT valid:-0<=START<=LIMIT
        {-1, 16, 14, 16},   //invalid since START<0
        {3,  50, 2,  50}    //invalid since LIMIT>text.length()
    };
    for(uint32_t i=0; i<sizeof(index1)/sizeof(index1[0]); i++){
        status=U_ZERO_ERROR;
        t->transliterate(rs, index1[i], insertion, status);
        if(status == U_ILLEGAL_ARGUMENT_ERROR)
            logln("OK: invalid index values handled correctly");
        else
            errln("FAIL: invalid index values didn't throw U_ILLEGAL_ARGUMENT_ERROR throw" + (UnicodeString)u_errorName(status));
    }

    delete t;
}
void TransliteratorAPITest::TestKeyboardTransliterator1(){
    UnicodeString Data[]={
        //insertion, buffer
        "a",   UnicodeString("\\u0061", "")                                           ,
        "bbb", UnicodeString("\\u0061\\u0062\\u0062\\u0062", "")                      ,
        "ca",  UnicodeString("\\u0061\\u0062\\u0062\\u0062\\u0063\\u0061", "")        ,
        " ",   UnicodeString("\\u0061\\u0062\\u0062\\u0062\\u0063\\u0061\\u0020", "") ,
        "",    UnicodeString("\\u0061\\u0062\\u0062\\u0062\\u0063\\u0061\\u0020", "")   ,

        "a",   UnicodeString("\\u0061", "")                                           ,
        "b",   UnicodeString("\\u0061\\u0062", "")                                    ,
        "z",   UnicodeString("\\u0061\\u0062\\u007A", "")                             ,
        "",    UnicodeString("\\u0061\\u0062\\u007A", "")                              

    };
    UParseError parseError;
    UErrorCode status = U_ZERO_ERROR;
    Transliterator* t=Transliterator::createInstance("Any-Hex", UTRANS_FORWARD, parseError, status);
    if(U_FAILURE(status)) {
      errln("Error creating transliterator %s", u_errorName(status));
      delete t;
      return;
    }
    //keyboardAux(t, Data);
    UTransPosition index={0, 0, 0, 0};
    UnicodeString s;
    uint32_t i;
    logln("Testing transliterate(Replaceable, int32_t, UnicodeString, UErrorCode)");
    for (i=0; i<10; i=i+2) {
       UnicodeString log;
       if (Data[i+0] != "") {
           log = s + " + " + Data[i+0] + " -> ";
           t->transliterate(s, index, Data[i+0], status);
           if(U_FAILURE(status)){
               errln("FAIL: " + t->getID()+ ".transliterate(Replaceable, int32_t[], UnicodeString, UErrorCode)-->" + (UnicodeString)u_errorName(status));
           continue;
           }
       }else {
           log = s + " => ";
           t->finishTransliteration(s, index);
       }
       // Show the start index '{' and the cursor '|'
       displayOutput(s, Data[i+1], log, index);
           
    }
    
    s="";
    status=U_ZERO_ERROR;
    index.contextStart = index.contextLimit = index.start = index.limit = 0;
    logln("Testing transliterate(Replaceable, int32_t, UChar, UErrorCode)");
    for(i=10; i<sizeof(Data)/sizeof(Data[0]); i=i+2){
        UnicodeString log;
         if (Data[i+0] != "") {
           log = s + " + " + Data[i+0] + " -> ";
           UChar c=Data[i+0].charAt(0);
           t->transliterate(s, index, c, status);
           if(U_FAILURE(status))
               errln("FAIL: " + t->getID()+ ".transliterate(Replaceable, int32_t[], UChar, UErrorCode)-->" + (UnicodeString)u_errorName(status));
               continue;
         }else {
           log = s + " => ";
           t->finishTransliteration(s, index);
         }
        // Show the start index '{' and the cursor '|'
        displayOutput(s, Data[i+1], log, index); 
    }

    delete t;
}

void TransliteratorAPITest::TestKeyboardTransliterator2(){
    UnicodeString Data[]={
        //insertion, buffer, index[START], index[LIMIT], index[CURSOR]
        //data for Any-Hex
        "abc",    UnicodeString("Initial String: add-\\u0061\\u0062\\u0063-", ""),                     "19", "20", "20",
        "a",      UnicodeString("In\\u0069\\u0061tial String: add-\\u0061\\u0062\\u0063-", ""),        "2",  "3",  "2" ,
        "b",      UnicodeString("\\u0062In\\u0069\\u0061tial String: add-\\u0061\\u0062\\u0063-", ""), "0",  "0",  "0" ,
        "",       UnicodeString("\\u0062In\\u0069\\u0061tial String: add-\\u0061\\u0062\\u0063-", ""), "0",  "0",  "0" ,
        //data for Latin-Devanagiri
        CharsToUnicodeString("a\\u0304"),     CharsToUnicodeString("Hindi -\\u0906-"),                            "6", "7", "6",
        CharsToUnicodeString("ma\\u0304"),    CharsToUnicodeString("Hindi -\\u0906\\u092E\\u093E-"),              "7", "8", "7",
        CharsToUnicodeString("ra\\u0304"),    CharsToUnicodeString("Hi\\u0930\\u093Endi -\\u0906\\u092E\\u093E-"),"1", "2", "2",
        CharsToUnicodeString(""),       CharsToUnicodeString("Hi\\u0930\\u093Endi -\\u0906\\u092E\\u093E-"),"1", "2", "2"
        //data for contracted-Expanded
     //   CharsToUnicodeString("\\u00C1"), CharsToUnicodeString("Ad\\u0041\\u0301d here:"),             "1",  "2",  "1" ,
     //   CharsToUnicodeString("\\u00C0"), CharsToUnicodeString("Ad\\u0041\\u0301d here:\\u0041\\u0300"), "11", "11", "11",
     //   "",     CharsToUnicodeString("Ad\\u0041\\u0301d here:\\u0041\\u0300"), "11", "11", "11",
    };
    Transliterator *t;
    UnicodeString rs;
    UnicodeString dataStr;
    logln("Testing transliterate(Replaceable, int32_t, UnicodeString, UErrorCode)");       
    UErrorCode status = U_ZERO_ERROR;
    UParseError parseError;
    rs="Initial String: add--";
    t=Transliterator::createInstance("Any-Hex", UTRANS_FORWARD, parseError, status);
    if(t == 0)
        dataerrln("FAIL : construction - %s", u_errorName(status));
    else {
        keyboardAux(t, Data, rs, 0, 20);
        delete t;
    }

    rs="Hindi --";
    t=Transliterator::createInstance("Latin-Devanagari", UTRANS_FORWARD, parseError, status);
    if(t == 0)
        dataerrln("FAIL : construction - %s", u_errorName(status));
    else
        keyboardAux(t, Data, rs, 20, 40);


  //  rs="Add here:";
 //   t=Transliterator::createInstance("Contracted-Expanded");
 //   keyboardAux(t, Data, rs, 35, 55);


    delete t;
}

void TransliteratorAPITest::TestKeyboardTransliterator3(){
    UnicodeString s="This is the main string";
    UnicodeString Data[] = {
        "0", "0", "0",  "This is the main string",
        "1", "3", "2",  UnicodeString("Th\\u0069s is the main string", ""),
        "20", "21", "20",  UnicodeString("Th\\u0069s is the mai\\u006E string", "")
    };

    UErrorCode status=U_ZERO_ERROR;
    UParseError parseError;
    UTransPosition index={0, 0, 0, 0};
    logln("Testing transliterate(Replaceable, int32_t, UErrorCode)");
    Transliterator *t=Transliterator::createInstance("Any-Hex", UTRANS_FORWARD, parseError, status);
    if(t == 0 || U_FAILURE(status)) {
      errln("Error creating transliterator %s", u_errorName(status));
      delete t;
      return;
    }
    for(uint32_t i=0; i<sizeof(Data)/sizeof(Data[0]); i=i+4){
        UnicodeString log;
        index.contextStart=getInt(Data[i+0]);
        index.contextLimit=index.limit=getInt(Data[i+1]);
        index.start=getInt(Data[i+2]);
        t->transliterate(s, index, status);
        if(U_FAILURE(status)){
           errln("FAIL: " + t->getID()+ ".transliterate(Replaceable, int32_t[], UErrorCode)-->" + (UnicodeString)u_errorName(status));
           continue;
        }
        t->finishTransliteration(s, index);
        log = s + " => ";
        // Show the start index '{' and the cursor '|'
        displayOutput(s, Data[i+3], log, index); 
    }

    delete t;
}
void TransliteratorAPITest::TestNullTransliterator(){
    UErrorCode status=U_ZERO_ERROR;
    UnicodeString s("Transliterate using null transliterator");
    Transliterator *nullTrans=Transliterator::createInstance("Any-Null", UTRANS_FORWARD, status);
    int32_t transLimit;
    int32_t start=0;
    int32_t limit=s.length();
    UnicodeString replaceable=s;
    transLimit=nullTrans->transliterate(replaceable, start, limit);
    if(transLimit != limit){
        errln("ERROR: NullTransliterator->transliterate() failed");
    }
    doTest((UnicodeString)"nulTrans->transliterate", replaceable, s);
    replaceable.remove();
    replaceable.append(s);
    UTransPosition index;
    index.contextStart =start;
    index.contextLimit = limit;
    index.start = 0;
    index.limit = limit;
    nullTrans->finishTransliteration(replaceable, index);
    if(index.start != limit){
        errln("ERROR: NullTransliterator->handleTransliterate() failed");
    }
    doTest((UnicodeString)"NullTransliterator->handleTransliterate", replaceable, s);
    callEverything(nullTrans, __LINE__);
    delete nullTrans;

    
}
void TransliteratorAPITest::TestRegisterUnregister(){
   
   UErrorCode status=U_ZERO_ERROR;
    /* Make sure it doesn't exist */
   if (Transliterator::createInstance("TestA-TestB", UTRANS_FORWARD, status) != NULL) {
      errln("FAIL: TestA-TestB already registered\n");
      return;
   }
   /* Check inverse too 
   if (Transliterator::createInstance("TestA-TestB",
                                      (UTransDirection)UTRANS_REVERSE) != NULL) {
      errln("FAIL: TestA-TestB inverse already registered\n");
      return;
   }
   */
   status =U_ZERO_ERROR;

   /* Create it */
   UParseError parseError;
   Transliterator *t = Transliterator::createFromRules("TestA-TestB",
                                                   "a<>b",
                                                   UTRANS_FORWARD, parseError,
                                                   status);
   /* Register it */
   Transliterator::registerInstance(t);

   /* Now check again -- should exist now*/
   Transliterator *s = Transliterator::createInstance("TestA-TestB", UTRANS_FORWARD, status);
   if (s == NULL) {
      errln("FAIL: TestA-TestB not registered\n");
      return;
   }
   callEverything(s, __LINE__);
   callEverything(t, __LINE__);
   delete s;
   
   /* Check inverse too
   s = Transliterator::createInstance("TestA-TestB",
                                      (UTransDirection)UTRANS_REVERSE);
   if (s == NULL) {
      errln("FAIL: TestA-TestB inverse not registered\n");
      return;
   }
   delete s;
   */
   
   /*unregister the instance*/
   Transliterator::unregister("TestA-TestB");
   /* now Make sure it doesn't exist */
   if (Transliterator::createInstance("TestA-TestB", UTRANS_FORWARD, status) != NULL) {
      errln("FAIL: TestA-TestB isn't unregistered\n");
      return;
   }

}


int gTestFilter1ClassID = 0;
int gTestFilter2ClassID = 0;
int gTestFilter3ClassID = 0;

/**
 * Used by TestFiltering().
 */
class TestFilter1 : public UnicodeFilter {
    UClassID getDynamicClassID()const { return &gTestFilter1ClassID; }
    virtual UnicodeFunctor* clone() const {
        return new TestFilter1(*this);
    }
    virtual UBool contains(UChar32 c) const {
       if(c==0x63 || c==0x61 || c==0x43 || c==0x41)
          return FALSE;
       else
          return TRUE;
    }
    // Stubs
    virtual UnicodeString& toPattern(UnicodeString& result,
                                     UBool /*escapeUnprintable*/) const {
        return result;
    }
    virtual UBool matchesIndexValue(uint8_t /*v*/) const {
        return FALSE;
    }
    virtual void addMatchSetTo(UnicodeSet& /*toUnionTo*/) const {}
};
class TestFilter2 : public UnicodeFilter {
    UClassID getDynamicClassID()const { return &gTestFilter2ClassID; }
    virtual UnicodeFunctor* clone() const {
        return new TestFilter2(*this);
    }
    virtual UBool contains(UChar32 c) const {
        if(c==0x65 || c==0x6c)
           return FALSE;
        else
           return TRUE;
    }
    // Stubs
    virtual UnicodeString& toPattern(UnicodeString& result,
                                     UBool /*escapeUnprintable*/) const {
        return result;
    }
    virtual UBool matchesIndexValue(uint8_t /*v*/) const {
        return FALSE;
    }
    virtual void addMatchSetTo(UnicodeSet& /*toUnionTo*/) const {}
};
class TestFilter3 : public UnicodeFilter {
    UClassID getDynamicClassID()const { return &gTestFilter3ClassID; }
    virtual UnicodeFunctor* clone() const {
        return new TestFilter3(*this);
    }
    virtual UBool contains(UChar32 c) const {
        if(c==0x6f || c==0x77)
           return FALSE;
        else
           return TRUE;
    }
    // Stubs
    virtual UnicodeString& toPattern(UnicodeString& result,
                                     UBool /*escapeUnprintable*/) const {
        return result;
    }
    virtual UBool matchesIndexValue(uint8_t /*v*/) const {
        return FALSE;
    }
    virtual void addMatchSetTo(UnicodeSet& /*toUnionTo*/) const {}
};


void TransliteratorAPITest::TestGetAdoptFilter(){
    UErrorCode status = U_ZERO_ERROR;
    UParseError parseError;
    Transliterator *t=Transliterator::createInstance("Any-Hex", UTRANS_FORWARD, parseError, status);
    if(t == 0 || U_FAILURE(status)) {
        errln("Error creating transliterator %s", u_errorName(status));
        delete t;
        return;
    }
    const UnicodeFilter *u=t->getFilter();
    if(u != NULL){
        errln("FAIL: getFilter failed. Didn't return null when the transliterator used no filtering");
        delete t;
        return;
    }

    UnicodeString got, temp, message;
    UnicodeString data="ABCabcbbCBa";
    temp = data;
    t->transliterate(temp);
    t->adoptFilter(new TestFilter1);

    got = data;
    t->transliterate(got);
    UnicodeString exp=UnicodeString("A\\u0042Ca\\u0062c\\u0062\\u0062C\\u0042a", "");
    message="transliteration after adoptFilter(a,A,c,C) ";
    doTest(message, got, exp);
         
    logln("Testing round trip");
    t->adoptFilter((UnicodeFilter*)u);
    if(t->getFilter() == NULL)
       logln("OK: adoptFilter and getFilter round trip worked");
    else
       errln("FAIL: adoptFilter or getFilter round trip failed");  

    got = data;
    t->transliterate(got);
    exp=UnicodeString("\\u0041\\u0042\\u0043\\u0061\\u0062\\u0063\\u0062\\u0062\\u0043\\u0042\\u0061", "");
    message="transliteration after adopting null filter";
    doTest(message, got, exp);
    message="adoptFilter round trip"; 
    doTest("adoptFilter round trip", got, temp);

    t->adoptFilter(new TestFilter2);
    callEverything(t, __LINE__);
    data="heelloe";
    exp=UnicodeString("\\u0068eell\\u006Fe", "");
    got = data;
    t->transliterate(got);
    message="transliteration using (e,l) filter";
    doTest("transliteration using (e,l) filter", got, exp);

    data="are well";
    exp=UnicodeString("\\u0061\\u0072e\\u0020\\u0077ell", "");
    got = data;
    t->transliterate(got);
    doTest(message, got, exp);

    t->adoptFilter(new TestFilter3);
    data="ho, wow!";
    exp=UnicodeString("\\u0068o\\u002C\\u0020wow\\u0021", "");
    got = data;
    t->transliterate(got);
    message="transliteration using (o,w) filter";
    doTest("transliteration using (o,w) filter", got, exp);

    data="owl";
    exp=UnicodeString("ow\\u006C", "");
    got = data;
    t->transliterate(got);
    doTest("transliteration using (o,w) filter", got, exp);

    delete t;

}



void TransliteratorAPITest::keyboardAux(Transliterator *t, UnicodeString DATA[], UnicodeString& s, int32_t begin, int32_t end) {
    UTransPosition index={0, 0, 0, 0};
    UErrorCode status=U_ZERO_ERROR;
    for (int32_t i=begin; i<end; i=i+5) {
        UnicodeString log;
        if (DATA[i+0] != "") {
             log = s + " + " + DATA[i] + " -> ";
             index.contextStart=getInt(DATA[i+2]);
             index.contextLimit=index.limit=getInt(DATA[i+3]);
             index.start=getInt(DATA[i+4]);
             t->transliterate(s, index, DATA[i+0], status);
             if(U_FAILURE(status)){
                 errln("FAIL: " + t->getID()+ ".transliterate(Replaceable, int32_t[], UnicodeString, UErrorCode)-->" + (UnicodeString)u_errorName(status));
             continue;
             }
           log = s + " => ";
           t->finishTransliteration(s, index);
        }
         // Show the start index '{' and the cursor '|'
      displayOutput(s, DATA[i+1], log, index);
        
    }
}

void TransliteratorAPITest::displayOutput(const UnicodeString& got, const UnicodeString& expected, UnicodeString& log, UTransPosition& index){
 // Show the start index '{' and the cursor '|'
    UnicodeString a, b, c, d, e;
    got.extractBetween(0, index.contextStart, a);
    got.extractBetween(index.contextStart, index.start, b);
    got.extractBetween(index.start, index.limit, c);
    got.extractBetween(index.limit, index.contextLimit, d);
    got.extractBetween(index.contextLimit, got.length(), e);
    log.append(a).
        append((UChar)0x7b/*{*/).
        append(b).
        append((UChar)0x7c/*|*/).
        append(c).
        append((UChar)0x7c).
        append(d).
        append((UChar)0x7d/*}*/).
        append(e);
    if (got == expected) 
        logln("OK:" + prettify(log));
    else 
        errln("FAIL: " + prettify(log)  + ", expected " + prettify(expected));
}


/*Internal Functions used*/
void TransliteratorAPITest::doTest(const UnicodeString& message, const UnicodeString& result, const UnicodeString& expected){
    if (prettify(result) == prettify(expected)) 
        logln((UnicodeString)"Ok: " + prettify(message) + " passed \"" + prettify(expected) + "\"");
    else 
        dataerrln((UnicodeString)"FAIL:" + message + " failed  Got-->" + prettify(result)+ ", Expected--> " + prettify(expected) );
}


//
//  callEverything    call all of the const (non-destructive) methods on a
//                    transliterator, just to verify that they don't fail in some
//                    destructive way.
//
#define CEASSERT(a) {if (!(a)) { \
errln("FAIL at line %d from line %d: %s", __LINE__, line, #a);  return; }}

void TransliteratorAPITest::callEverything(const Transliterator *tr, int line) {
    Transliterator *clonedTR = tr->clone();
    CEASSERT(clonedTR != NULL);

    int32_t  maxcl = tr->getMaximumContextLength();
    CEASSERT(clonedTR->getMaximumContextLength() == maxcl);

    UnicodeString id;
    UnicodeString clonedId;
    id = tr->getID();
    clonedId = clonedTR->getID();
    CEASSERT(id == clonedId);

    const UnicodeFilter *filter = tr->getFilter();
    const UnicodeFilter *clonedFilter = clonedTR->getFilter();
    if (filter == NULL || clonedFilter == NULL) {
        // If one filter is NULL they better both be NULL.
        CEASSERT(filter == clonedFilter);
    } else {
        CEASSERT(filter != clonedFilter);
    }

    UnicodeString rules;
    UnicodeString clonedRules;
    rules = tr->toRules(rules, FALSE);
    clonedRules = clonedTR->toRules(clonedRules, FALSE);
    CEASSERT(rules == clonedRules);

    UnicodeSet sourceSet;
    UnicodeSet clonedSourceSet;
    tr->getSourceSet(sourceSet);
    clonedTR->getSourceSet(clonedSourceSet);
    CEASSERT(clonedSourceSet == sourceSet);

    UnicodeSet targetSet;
    UnicodeSet clonedTargetSet;
    tr->getTargetSet(targetSet);
    clonedTR->getTargetSet(clonedTargetSet);
    CEASSERT(targetSet == clonedTargetSet);

    UClassID classID = tr->getDynamicClassID();
    CEASSERT(classID == clonedTR->getDynamicClassID());
    CEASSERT(classID != 0);

    delete clonedTR;
}

static const int MyUnicodeFunctorTestClassID = 0;
class MyUnicodeFunctorTestClass : public UnicodeFunctor {
public:
    virtual UnicodeFunctor* clone() const {return NULL;}
    static UClassID getStaticClassID(void) {return (UClassID)&MyUnicodeFunctorTestClassID;}
    virtual UClassID getDynamicClassID(void) const {return getStaticClassID();};
    virtual void setData(const TransliterationRuleData*) {}
};

void TransliteratorAPITest::TestUnicodeFunctor() {
    MyUnicodeFunctorTestClass myClass;
    if (myClass.toMatcher() != NULL) {
        errln("FAIL: UnicodeFunctor::toMatcher did not return NULL");
    }
    if (myClass.toReplacer() != NULL) {
        errln("FAIL: UnicodeFunctor::toReplacer did not return NULL");
    }
}


#endif /* #if !UCONFIG_NO_TRANSLITERATION */
