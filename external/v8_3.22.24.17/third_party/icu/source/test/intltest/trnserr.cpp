/********************************************************************
 * Copyright (c) 2001-2009, International Business Machines
 * Corporation and others. All Rights Reserved.
 *********************************************************************
 *   This test program is intended for testing error conditions of the 
 *   transliterator APIs to make sure the exceptions are raised where
 *   necessary.
 *
 *   Date        Name        Description
 *   11/14/2001  hshih       Creation.
 * 
 ********************************************************************/

#include "unicode/utypes.h"

#if !UCONFIG_NO_TRANSLITERATION

#include "ittrans.h"
#include "trnserr.h"
#include "unicode/utypes.h"
#include "unicode/translit.h"
#include "unicode/uniset.h"
#include "unicode/unifilt.h"
#include "cpdtrans.h"
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include "unicode/rep.h"
#include "unicode/locid.h"

//---------------------------------------------
// runIndexedTest
//---------------------------------------------

void
TransliteratorErrorTest::runIndexedTest(int32_t index, UBool exec,
                                      const char* &name, char* /*par*/) {
    switch (index) {
        TESTCASE(0,TestTransliteratorErrors);
        TESTCASE(1, TestUnicodeSetErrors);
        TESTCASE(2, TestRBTErrors);
        TESTCASE(3, TestCoverage);
        //TESTCASE(3, TestUniToHexErrors);
        //TESTCASE(4, TestHexToUniErrors);
        // TODO: Add a subclass to test clone().
        default: name = ""; break;
    }
}


void TransliteratorErrorTest::TestTransliteratorErrors() {
    UnicodeString trans="Latin-Greek";
    UnicodeString bogusID="LATINGREEK-GREEKLATIN";
    UnicodeString newID="Bogus-Latin";
    UnicodeString newIDRules="zzz > Z; f <> ph";
    UnicodeString bogusRules="a } [b-g m-p ";
    UParseError parseError;
    UErrorCode status = U_ZERO_ERROR;
    UnicodeString testString="A quick fox jumped over the lazy dog.";
    UnicodeString insertString="cats and dogs";
    int32_t stoppedAt = 0, len;
    UTransPosition pos;

    Transliterator* t= Transliterator::createInstance(trans, UTRANS_FORWARD, parseError, status);
    if(t==0 || U_FAILURE(status)){
        dataerrln("FAIL: construction of Latin-Greek - %s", u_errorName(status));
        return;
    }
    pos.contextLimit = 0;
    pos.contextStart = 0;
    pos.limit = 0;
    pos.start = 0;
    len = testString.length();
    stoppedAt = t->transliterate(testString, 0, 100);
    if (stoppedAt != -1) {
        errln("FAIL: Out of bounds check failed (1).");
    } else if (testString.length() != len) {
        testString="A quick fox jumped over the lazy dog.";        
        errln("FAIL: Transliterate fails and the target string was modified.");
    }
    stoppedAt = t->transliterate(testString, 100, testString.length()-1);
    if (stoppedAt != -1)
        errln("FAIL: Out of bounds check failed (2).");
    else if (testString.length() != len) {
        testString="A quick fox jumped over the lazy dog.";        
        errln("FAIL: Transliterate fails and the target string was modified.");
    }
    pos.start = 100;
    pos.limit = testString.length();
    t->transliterate(testString, pos, status);
    if (U_SUCCESS(status)) {
        errln("FAIL: Start offset is out of bounds, error not reported.\n");
    }
    status = U_ZERO_ERROR;
    pos.limit = 100;
    pos.start = 0;
    t->transliterate(testString, pos, status);
    if (U_SUCCESS(status)) {
        errln("FAIL: Limit offset is out of bounds, error not reported.\n");
    }
    status = U_ZERO_ERROR;
    len = pos.contextLimit = testString.length();
    pos.contextStart = 0;
    pos.limit = len - 1;
    pos.start = 5;
    t->transliterate(testString, pos, insertString, status);
    if (len == pos.limit) {
        errln("FAIL: Test insertion with string: the transliteration position limit didn't change as expected.");
        if (U_SUCCESS(status)) {
            errln("FAIL: Error code wasn't set either.");
        }
    }
    status = U_ZERO_ERROR;
    pos.contextStart = 0;
    pos.contextLimit = testString.length();
    pos.limit = testString.length() -1;
    pos.start = 5;
    t->transliterate(testString, pos, (UChar32)0x0061, status);
    if (len == pos.limit) {
        errln("FAIL: Test insertion with character: the transliteration position limit didn't change as expected.");
        if (U_SUCCESS(status)) {
            errln("FAIL: Error code wasn't set either.");
        }
    }
    status = U_ZERO_ERROR;
    len = pos.limit = testString.length();
    pos.contextStart = 0;
    pos.contextLimit = testString.length() - 1;
    pos.start = 5;
    t->transliterate(testString, pos, insertString, status);
    if (U_SUCCESS(status)) {
        errln("FAIL: Out of bounds check failed (3).");
        if (testString.length() != len)
            errln("FAIL: The input string was modified though the offsets were out of bounds.");
    }
    Transliterator* t1= Transliterator::createInstance(bogusID, UTRANS_FORWARD, parseError, status);
    if(t1!=0 || U_SUCCESS(status)){
        delete t1;
        errln("FAIL: construction of bogus ID \"LATINGREEK-GREEKLATIN\"");
    }
    status = U_ZERO_ERROR;
    Transliterator* t2 = Transliterator::createFromRules(newID, newIDRules, UTRANS_FORWARD, parseError, status);
    if (U_SUCCESS(status)) {
        Transliterator* t3 = t2->createInverse(status);
        if (U_SUCCESS(status)) {
            delete t3;
            errln("FAIL: The newID transliterator was not registered so createInverse should fail.");
        } else {
            delete t3;
        }
    }
    status = U_ZERO_ERROR;
    Transliterator* t4 = Transliterator::createFromRules(newID, bogusRules, UTRANS_FORWARD, parseError, status);
    if (t4 != NULL || U_SUCCESS(status)) {
        errln("FAIL: The rules is malformed but error was not reported.");
        if (parseError.offset != -1) {
            errln("FAIL: The parse error offset isn't set correctly when fails.");
        } else if (parseError.postContext[0] == 0 || parseError.preContext[0] == 0) {
            errln("FAIL: The parse error pre/post context isn't reset properly.");
        }
        delete t4;
    }    
    delete t;
    delete t2;
}

void TransliteratorErrorTest::TestUnicodeSetErrors() {
    UnicodeString badPattern="[[:L:]-[0x0300-0x0400]";
    UnicodeSet set;
    UErrorCode status = U_ZERO_ERROR;
    UnicodeString result;

    if (!set.isEmpty()) {
        errln("FAIL: The default ctor of UnicodeSet created a non-empty object.");
    }
    set.applyPattern(badPattern, status);
    if (U_SUCCESS(status)) {
        errln("FAIL: Applied a bad pattern to the UnicodeSet object okay.");
    }
    status = U_ZERO_ERROR;
    UnicodeSet *set1 = new UnicodeSet(badPattern, status);
    if (U_SUCCESS(status)) {
        errln("FAIL: Created a UnicodeSet based on bad patterns.");
    }
    delete set1;
}

//void TransliteratorErrorTest::TestUniToHexErrors() {
//    UErrorCode status = U_ZERO_ERROR;
//    Transliterator *t = new UnicodeToHexTransliterator("", TRUE, NULL, status);
//    if (U_SUCCESS(status)) {
//        errln("FAIL: Created a UnicodeToHexTransliterator with an empty pattern.");
//    }
//    delete t;
//
//    status = U_ZERO_ERROR;
//    t = new UnicodeToHexTransliterator("\\x", TRUE, NULL, status);
//    if (U_SUCCESS(status)) {
//        errln("FAIL: Created a UnicodeToHexTransliterator with a bad pattern.");
//    }
//    delete t;
//
//    status = U_ZERO_ERROR;
//    t = new UnicodeToHexTransliterator();
//    ((UnicodeToHexTransliterator*)t)->applyPattern("\\x", status);
//    if (U_SUCCESS(status)) {
//        errln("FAIL: UnicodeToHexTransliterator::applyPattern succeeded with a bad pattern.");
//    }
//    delete t;
//}

void TransliteratorErrorTest::TestRBTErrors() {

    UnicodeString rules="ab>y";
    UnicodeString id="MyRandom-YReverse";
    //UnicodeString goodPattern="[[:L:]&[\\u0000-\\uFFFF]]"; /* all BMP letters */
    UErrorCode status = U_ZERO_ERROR;
    UParseError parseErr;
    /*UnicodeSet *set = new UnicodeSet(goodPattern, status);
    if (U_FAILURE(status)) {
        errln("FAIL: Was not able to create a good UnicodeSet based on valid patterns.");
        return;
    }*/
    Transliterator *t = Transliterator::createFromRules(id, rules, UTRANS_REVERSE, parseErr, status);
    if (U_FAILURE(status)) {
        errln("FAIL: Was not able to create a good RBT to test registration.");
        //delete set;
        return;
    }
    Transliterator::registerInstance(t);
    Transliterator::unregister(id);
    status = U_ZERO_ERROR;
    Transliterator* t1= Transliterator::createInstance(id, UTRANS_REVERSE, parseErr, status);
    if(U_SUCCESS(status)){
        delete t1;
        errln("FAIL: construction of unregistered ID failed.");
    } 
}

//void TransliteratorErrorTest::TestHexToUniErrors() {
//    UErrorCode status = U_ZERO_ERROR;
//    Transliterator *t = new HexToUnicodeTransliterator("", NULL, status);
//    if (U_FAILURE(status)) {
//        errln("FAIL: Could not create a HexToUnicodeTransliterator with an empty pattern.");
//    }
//    delete t;
//    status = U_ZERO_ERROR;
//    t = new HexToUnicodeTransliterator("\\x", NULL, status);
//    if (U_SUCCESS(status)) {
//        errln("FAIL: Created a HexToUnicodeTransliterator with a bad pattern.");
//    }
//    delete t;
//    status = U_ZERO_ERROR;
//    t = new HexToUnicodeTransliterator();
//    ((HexToUnicodeTransliterator*)t)->applyPattern("\\x", status);
//    if (U_SUCCESS(status)) {
//        errln("FAIL: HexToUnicodeTransliterator::applyPattern succeeded with a bad pattern.");
//    }
//    delete t;
//}

class StubTransliterator: public Transliterator{
public:
    StubTransliterator(): Transliterator(UNICODE_STRING_SIMPLE("Any-Null"), 0) {}
    virtual void handleTransliterate(Replaceable& ,UTransPosition& offsets,UBool) const {
        offsets.start = offsets.limit;
    }

    virtual UClassID getDynamicClassID() const{
        static char classID = 0;
        return (UClassID)&classID; 
    }
};

void TransliteratorErrorTest::TestCoverage() {
    StubTransliterator stub;

    if (stub.clone() != NULL){
        errln("FAIL: default Transliterator::clone() should return NULL");
    }
}

#endif /* #if !UCONFIG_NO_TRANSLITERATION */
