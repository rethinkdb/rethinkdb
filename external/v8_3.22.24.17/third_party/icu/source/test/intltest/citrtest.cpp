/****************************************************************************************
 * COPYRIGHT: 
 * Copyright (c) 1997-2010, International Business Machines Corporation and
 * others. All Rights Reserved.
 * Modification History:
 *
 *   Date          Name        Description
 *   05/22/2000    Madhu       Added tests for testing new API for utf16 support and more
 ****************************************************************************************/

#include <string.h>
#include "unicode/utypeinfo.h"  // for 'typeid' to work

#include "unicode/chariter.h"
#include "unicode/ustring.h"
#include "unicode/unistr.h"
#include "unicode/schriter.h"
#include "unicode/uchriter.h"
#include "unicode/uiter.h"
#include "unicode/putil.h"
#include "citrtest.h"


class  SCharacterIterator : public CharacterIterator {
public:
    SCharacterIterator(const UnicodeString& textStr){
        text = textStr;
        pos=0;
        textLength = textStr.length();
        begin = 0;
        end=textLength;

    }

    virtual ~SCharacterIterator(){};

                                
    void setText(const UnicodeString& newText){
        text = newText;
    }

    virtual void getText(UnicodeString& result) {
        text.extract(0,text.length(),result);
    }
    static UClassID getStaticClassID(void){ 
        return (UClassID)(&fgClassID); 
    }
    virtual UClassID getDynamicClassID(void) const{ 
        return getStaticClassID(); 
    }

    virtual UBool operator==(const ForwardCharacterIterator& /*that*/) const{
        return TRUE;
    }

    virtual CharacterIterator* clone(void) const {
        return NULL;
    }
    virtual int32_t hashCode(void) const{
        return DONE;
    }
    virtual UChar nextPostInc(void){ return text.charAt(pos++);}
    virtual UChar32 next32PostInc(void){return text.char32At(pos++);}
    virtual UBool hasNext() { return TRUE;};
    virtual UChar first(){return DONE;};
    virtual UChar32 first32(){return DONE;};
    virtual UChar last(){return DONE;};
    virtual UChar32 last32(){return DONE;};
    virtual UChar setIndex(int32_t /*pos*/){return DONE;};
    virtual UChar32 setIndex32(int32_t /*pos*/){return DONE;};
    virtual UChar current() const{return DONE;};
    virtual UChar32 current32() const{return DONE;};
    virtual UChar next(){return DONE;};
    virtual UChar32 next32(){return DONE;};
    virtual UChar previous(){return DONE;};
    virtual UChar32 previous32(){return DONE;};
    virtual int32_t move(int32_t delta,CharacterIterator::EOrigin origin){    
        switch(origin) {
        case kStart:
            pos = begin + delta;
            break;
        case kCurrent:
            pos += delta;
            break;
        case kEnd:
            pos = end + delta;
            break;
        default:
            break;
        }

        if(pos < begin) {
            pos = begin;
        } else if(pos > end) {
            pos = end;
        }

        return pos;
    };
    virtual int32_t move32(int32_t delta, CharacterIterator::EOrigin origin){    
        switch(origin) {
        case kStart:
            pos = begin;
            if(delta > 0) {
                UTF_FWD_N(text, pos, end, delta);
            }
            break;
        case kCurrent:
            if(delta > 0) {
                UTF_FWD_N(text, pos, end, delta);
            } else {
                UTF_BACK_N(text, begin, pos, -delta);
            }
            break;
        case kEnd:
            pos = end;
            if(delta < 0) {
                UTF_BACK_N(text, begin, pos, -delta);
            }
            break;
        default:
            break;
        }

        return pos;
    };
    virtual UBool hasPrevious() {return TRUE;};

  SCharacterIterator&  operator=(const SCharacterIterator&    that){
     text = that.text;
     return *this;
  }


private:
    UnicodeString text;
    static const char fgClassID;
};
const char SCharacterIterator::fgClassID=0;

#define LENGTHOF(array) ((int32_t)(sizeof(array)/sizeof((array)[0])))

CharIterTest::CharIterTest()
{
}
void CharIterTest::runIndexedTest( int32_t index, UBool exec, const char* &name, char* /*par*/ )
{
    if (exec) logln("TestSuite CharIterTest: ");
    switch (index) {
        case 0: name = "TestConstructionAndEquality"; if (exec) TestConstructionAndEquality(); break;
        case 1: name = "TestConstructionAndEqualityUChariter"; if (exec) TestConstructionAndEqualityUChariter(); break;
        case 2: name = "TestIteration"; if (exec) TestIteration(); break;
        case 3: name = "TestIterationUChar32"; if (exec) TestIterationUChar32(); break;
        case 4: name = "TestUCharIterator"; if (exec) TestUCharIterator(); break;
        case 5: name = "TestCoverage"; if(exec) TestCoverage(); break;
        case 6: name = "TestCharIteratorSubClasses"; if (exec) TestCharIteratorSubClasses(); break;
        default: name = ""; break; //needed to end loop
    }
}

void CharIterTest::TestCoverage(){
    UnicodeString  testText("Now is the time for all good men to come to the aid of their country.");
    UnicodeString testText2("\\ud800\\udc01deadbeef");
    testText2 = testText2.unescape();
    SCharacterIterator* test = new SCharacterIterator(testText);
    if(test->firstPostInc()!= 0x004E){
        errln("Failed: firstPostInc() failed");
    }
    if(test->getIndex()!=1){
        errln("Failed: getIndex().");
    }
    if(test->getLength()!=testText.length()){
        errln("Failed: getLength()");
    }
    test->setToStart();
    if(test->getIndex()!=0){
        errln("Failed: setToStart().");
    }
    test->setToEnd();
    if(test->getIndex()!=testText.length()){
        errln("Failed: setToEnd().");
    }
    if(test->startIndex() != 0){
        errln("Failed: startIndex()");
    }
    test->setText(testText2);
    if(test->first32PostInc()!= testText2.char32At(0)){
        errln("Failed: first32PostInc() failed");
    }
 
    delete test;
    
}
void CharIterTest::TestConstructionAndEquality() {
    UnicodeString  testText("Now is the time for all good men to come to the aid of their country.");
    UnicodeString  testText2("Don't bother using this string.");
    UnicodeString result1, result2, result3;

    CharacterIterator* test1 = new StringCharacterIterator(testText);
    CharacterIterator* test1b= new StringCharacterIterator(testText, -1);
    CharacterIterator* test1c= new StringCharacterIterator(testText, 100);
    CharacterIterator* test1d= new StringCharacterIterator(testText, -2, 100, 5);
    CharacterIterator* test1e= new StringCharacterIterator(testText, 100, 20, 5);
    CharacterIterator* test2 = new StringCharacterIterator(testText, 5);
    CharacterIterator* test3 = new StringCharacterIterator(testText, 2, 20, 5);
    CharacterIterator* test4 = new StringCharacterIterator(testText2);
    CharacterIterator* test5 = test1->clone();

    if (test1d->startIndex() < 0)
        errln("Construction failed: startIndex is negative");
    if (test1d->endIndex() > testText.length())
        errln("Construction failed: endIndex is greater than the text length");
    if (test1d->getIndex() < test1d->startIndex() || test1d->endIndex() < test1d->getIndex())
        errln("Construction failed: index is invalid");

    if (*test1 == *test2 || *test1 == *test3 || *test1 == *test4)
        errln("Construction or operator== failed: Unequal objects compared equal");
    if (*test1 != *test5)
        errln("clone() or equals() failed: Two clones tested unequal");

    if (test1->hashCode() == test2->hashCode() || test1->hashCode() == test3->hashCode()
                    || test1->hashCode() == test4->hashCode())
        errln("hashCode() failed:  different objects have same hash code");

    if (test1->hashCode() != test5->hashCode())
        errln("hashCode() failed:  identical objects have different hash codes");

    if(test1->getLength() != testText.length()){
        errln("getLength of CharacterIterator failed");
    }
    test1->getText(result1);
    test1b->getText(result2);
    test1c->getText(result3);
    if(result1 != result2 ||  result1 != result3)
        errln("construction failed or getText() failed");


    test1->setIndex(5);
    if (*test1 != *test2 || *test1 == *test5)
        errln("setIndex() failed");

    *((StringCharacterIterator*)test1) = *((StringCharacterIterator*)test3);
    if (*test1 != *test3 || *test1 == *test5)
        errln("operator= failed");

    delete test2;
    delete test3;
    delete test4;
    delete test5;
    delete test1b;
    delete test1c;
    delete test1d;
    delete test1e;

   
    StringCharacterIterator* testChar1=new StringCharacterIterator(testText);
    StringCharacterIterator* testChar2=new StringCharacterIterator(testText2);
    StringCharacterIterator* testChar3=(StringCharacterIterator*)test1->clone();

    testChar1->getText(result1);
    testChar2->getText(result2);
    testChar3->getText(result3); 
    if(result1 != result3 || result1 == result2)
        errln("getText() failed");
    testChar3->setText(testText2);
    testChar3->getText(result3);
    if(result1 == result3 || result2 != result3)
        errln("setText() or getText() failed");
    testChar3->setText(testText);
    testChar3->getText(result3);
    if(result1 != result3 || result1 == result2)
        errln("setText() or getText() round-trip failed");

    delete testChar1;
    delete testChar2;
    delete testChar3;
    delete test1;

}
void CharIterTest::TestConstructionAndEqualityUChariter() {
    U_STRING_DECL(testText, "Now is the time for all good men to come to the aid of their country.", 69);
    U_STRING_DECL(testText2, "Don't bother using this string.", 31);

    U_STRING_INIT(testText, "Now is the time for all good men to come to the aid of their country.", 69);
    U_STRING_INIT(testText2, "Don't bother using this string.", 31);

    UnicodeString result, result4, result5;

    UCharCharacterIterator* test1 = new UCharCharacterIterator(testText, u_strlen(testText));
    UCharCharacterIterator* test2 = new UCharCharacterIterator(testText, u_strlen(testText), 5);
    UCharCharacterIterator* test3 = new UCharCharacterIterator(testText, u_strlen(testText), 2, 20, 5);
    UCharCharacterIterator* test4 = new UCharCharacterIterator(testText2, u_strlen(testText2));
    UCharCharacterIterator* test5 = (UCharCharacterIterator*)test1->clone();
    UCharCharacterIterator* test6 = new UCharCharacterIterator(*test1);

    // j785: length=-1 will use u_strlen()
    UCharCharacterIterator* test7a = new UCharCharacterIterator(testText, -1);
    UCharCharacterIterator* test7b = new UCharCharacterIterator(testText, -1);
    UCharCharacterIterator* test7c = new UCharCharacterIterator(testText, -1, 2, 20, 5);

    // Bad parameters.
    UCharCharacterIterator* test8a = new UCharCharacterIterator(testText, -1, -1, 20, 5);
    UCharCharacterIterator* test8b = new UCharCharacterIterator(testText, -1, 2, 100, 5);
    UCharCharacterIterator* test8c = new UCharCharacterIterator(testText, -1, 2, 20, 100);

    if (test8a->startIndex() < 0)
        errln("Construction failed: startIndex is negative");
    if (test8b->endIndex() != u_strlen(testText))
        errln("Construction failed: endIndex is different from the text length");
    if (test8c->getIndex() < test8c->startIndex() || test8c->endIndex() < test8c->getIndex())
        errln("Construction failed: index is invalid");

    if (*test1 == *test2 || *test1 == *test3 || *test1 == *test4 )
        errln("Construction or operator== failed: Unequal objects compared equal");
    if (*test1 != *test5 )
        errln("clone() or equals() failed: Two clones tested unequal");

    if (*test6 != *test1 )
        errln("copy construction or equals() failed: Two copies tested unequal");

    if (test1->hashCode() == test2->hashCode() || test1->hashCode() == test3->hashCode()
                    || test1->hashCode() == test4->hashCode())
        errln("hashCode() failed:  different objects have same hash code");

    if (test1->hashCode() != test5->hashCode())
        errln("hashCode() failed:  identical objects have different hash codes");
     
    test7a->getText(result);
    test7b->getText(result4);
    test7c->getText(result5);

    if(result != UnicodeString(testText) || result4 != result || result5 != result)
        errln("error in construction");
    
    test1->getText(result);
    test4->getText(result4);
    test5->getText(result5); 
    if(result != result5 || result == result4)
        errln("getText() failed");
    test5->setText(testText2, u_strlen(testText2));
    test5->getText(result5);
    if(result == result5 || result4 != result5)
        errln("setText() or getText() failed");
    test5->setText(testText, u_strlen(testText));
    test5->getText(result5);
    if(result != result5 || result == result4)
        errln("setText() or getText() round-trip failed"); 


    test1->setIndex(5);
    if (*test1 != *test2 || *test1 == *test5)
        errln("setIndex() failed");
    test8b->setIndex32(5);
    if (test8b->getIndex()!=5)
        errln("setIndex32() failed");

    *test1 = *test3;
    if (*test1 != *test3 || *test1 == *test5)
        errln("operator= failed");

    delete test1;
    delete test2;
    delete test3;
    delete test4;
    delete test5;
    delete test6;
    delete test7a;
    delete test7b;
    delete test7c;
    delete test8a;
    delete test8b;
    delete test8c;
}


void CharIterTest::TestIteration() {
    UnicodeString text("Now is the time for all good men to come to the aid of their country.");

    UChar c;
    int32_t i;
    {
        StringCharacterIterator   iter(text, 5);

        UnicodeString iterText;
        iter.getText(iterText);
        if (iterText != text)
          errln("iter.getText() failed");

        if (iter.current() != text[(int32_t)5])
            errln("Iterator didn't start out in the right place.");

        c = iter.first();
        i = 0;

        if (iter.startIndex() != 0 || iter.endIndex() != text.length())
            errln("startIndex() or endIndex() failed");

        logln("Testing forward iteration...");
        do {
            if (c == CharacterIterator::DONE && i != text.length())
                errln("Iterator reached end prematurely");
            else if (c != text[i])
                errln((UnicodeString)"Character mismatch at position " + i +
                                    ", iterator has " + UCharToUnicodeString(c) +
                                    ", string has " + UCharToUnicodeString(text[i]));

            if (iter.current() != c)
                errln("current() isn't working right");
            if (iter.getIndex() != i)
                errln("getIndex() isn't working right");

            if (c != CharacterIterator::DONE) {
                c = iter.next();
                i++;
            }
        } while (c != CharacterIterator::DONE);
        c=iter.next();
        if(c!= CharacterIterator::DONE)
            errln("next() didn't return DONE at the end");
        c=iter.setIndex(text.length()+1);
        if(c!= CharacterIterator::DONE)
            errln("setIndex(len+1) didn't return DONE");

        c = iter.last();
        i = text.length() - 1;

        logln("Testing backward iteration...");
        do {
            if (c == CharacterIterator::DONE && i >= 0)
                errln("Iterator reached end prematurely");
            else if (c != text[i])
                errln((UnicodeString)"Character mismatch at position " + i +
                                    ", iterator has " + UCharToUnicodeString(c) +
                                    ", string has " + UCharToUnicodeString(text[i]));

            if (iter.current() != c)
                errln("current() isn't working right");
            if (iter.getIndex() != i)
                errln("getIndex() isn't working right");
            if(iter.setIndex(i) != c)
                errln("setIndex() isn't working right");

            if (c != CharacterIterator::DONE) {
                c = iter.previous();
                i--;
            }
        } while (c != CharacterIterator::DONE);

        c=iter.previous();
        if(c!= CharacterIterator::DONE)
            errln("previous didn't return DONE at the beginning");


        //testing firstPostInc, nextPostInc, setTostart
        i = 0;
        c=iter.firstPostInc();
        if(c != text[i])
            errln((UnicodeString)"firstPostInc failed.  Expected->" +  
                         UCharToUnicodeString(text[i]) + " Got->" + UCharToUnicodeString(c));
        if(iter.getIndex() != i+1)
            errln((UnicodeString)"getIndex() after firstPostInc() failed");

        iter.setToStart();
        i=0;
        if (iter.startIndex() != 0)
            errln("setToStart failed");

        logln("Testing forward iteration...");
        do {
            if (c != CharacterIterator::DONE)
                c = iter.nextPostInc();

            if(c != text[i])
                errln((UnicodeString)"Character mismatch at position " + i +
                                    (UnicodeString)", iterator has " + UCharToUnicodeString(c) +
                                    (UnicodeString)", string has " + UCharToUnicodeString(text[i]));

            i++;
            if(iter.getIndex() != i)
                errln("getIndex() aftr nextPostInc() isn't working right");
            if(iter.current() != text[i])
                errln("current() after nextPostInc() isn't working right");
        } while (iter.hasNext());
        c=iter.nextPostInc();
        if(c!= CharacterIterator::DONE)
            errln("nextPostInc() didn't return DONE at the beginning");
    }

    {
        StringCharacterIterator iter(text, 5, 15, 10);
        if (iter.startIndex() != 5 || iter.endIndex() != 15)
            errln("creation of a restricted-range iterator failed");

        if (iter.getIndex() != 10 || iter.current() != text[(int32_t)10])
            errln("starting the iterator in the middle didn't work");

        c = iter.first();
        i = 5;

        logln("Testing forward iteration over a range...");
        do {
            if (c == CharacterIterator::DONE && i != 15)
                errln("Iterator reached end prematurely");
            else if (c != text[i])
                errln((UnicodeString)"Character mismatch at position " + i +
                                    ", iterator has " + UCharToUnicodeString(c) +
                                    ", string has " + UCharToUnicodeString(text[i]));

            if (iter.current() != c)
                errln("current() isn't working right");
            if (iter.getIndex() != i)
                errln("getIndex() isn't working right");
            if(iter.setIndex(i) != c)
                errln("setIndex() isn't working right");

            if (c != CharacterIterator::DONE) {
                c = iter.next();
                i++;
            }
        } while (c != CharacterIterator::DONE);

        c = iter.last();
        i = 14;

        logln("Testing backward iteration over a range...");
        do {
            if (c == CharacterIterator::DONE && i >= 5)
                errln("Iterator reached end prematurely");
            else if (c != text[i])
                errln((UnicodeString)"Character mismatch at position " + i +
                                    ", iterator has " + UCharToUnicodeString(c) +
                                    ", string has " + UCharToUnicodeString(text[i]));

            if (iter.current() != c)
                errln("current() isn't working right");
            if (iter.getIndex() != i)
                errln("getIndex() isn't working right");

            if (c != CharacterIterator::DONE) {
                c = iter.previous();
                i--;
            }
        } while (c != CharacterIterator::DONE);


    }
}

//Tests for new API for utf-16 support 
void CharIterTest::TestIterationUChar32() {
    UChar textChars[]={ 0x0061, 0x0062, 0xd841, 0xdc02, 0x20ac, 0xd7ff, 0xd842, 0xdc06, 0xd801, 0xdc00, 0x0061, 0x0000};
    UnicodeString text(textChars);
    UChar32 c;
    int32_t i;
    {
        StringCharacterIterator   iter(text, 1);

        UnicodeString iterText;
        iter.getText(iterText);
        if (iterText != text)
          errln("iter.getText() failed");

        if (iter.current32() != text[(int32_t)1])
            errln("Iterator didn't start out in the right place.");

        c=iter.setToStart();
        i=0;
        i=iter.move32(1, CharacterIterator::kStart);
        c=iter.current32();
        if(c != text.char32At(1) || i!=1)
            errln("move32(1, kStart) didn't work correctly expected %X got %X", c, text.char32At(1) );

        i=iter.move32(2, CharacterIterator::kCurrent);
        c=iter.current32();
        if(c != text.char32At(4) || i!=4)
            errln("move32(2, kCurrent) didn't work correctly expected %X got %X i=%ld", c, text.char32At(4), i);
        
        i=iter.move32(-2, CharacterIterator::kCurrent);
        c=iter.current32();
        if(c != text.char32At(1) || i!=1)
            errln("move32(-2, kCurrent) didn't work correctly expected %X got %X i=%d", c, text.char32At(1), i);


        i=iter.move32(-2, CharacterIterator::kEnd);
        c=iter.current32();
        if(c != text.char32At((text.length()-3)) || i!=(text.length()-3))
            errln("move32(-2, kEnd) didn't work correctly expected %X got %X i=%d", c, text.char32At((text.length()-3)), i);
        

        c = iter.first32();
        i = 0;

        if (iter.startIndex() != 0 || iter.endIndex() != text.length())
            errln("startIndex() or endIndex() failed");

        logln("Testing forward iteration...");
        do {
            /* logln("c=%d i=%d char32At=%d", c, i, text.char32At(i)); */
            if (c == CharacterIterator::DONE && i != text.length())
                errln("Iterator reached end prematurely");
            else if(iter.hasNext() == FALSE && i != text.length())
                errln("Iterator reached end prematurely.  Failed at hasNext");
            else if (c != text.char32At(i))
                errln("Character mismatch at position %d, iterator has %X, string has %X", i, c, text.char32At(i));

            if (iter.current32() != c)
                errln("current32() isn't working right");
            if(iter.setIndex32(i) != c)
                errln("setIndex32() isn't working right");
            if (c != CharacterIterator::DONE) {
                c = iter.next32();
                i=UTF16_NEED_MULTIPLE_UCHAR(c) ? i+2 : i+1;
            }
        } while (c != CharacterIterator::DONE);
        if(iter.hasNext() == TRUE)
           errln("hasNext() returned true at the end of the string");



        c=iter.setToEnd();
        if(iter.getIndex() != text.length() || iter.hasNext() != FALSE)
            errln("setToEnd failed");

        c=iter.next32();
        if(c!= CharacterIterator::DONE)
            errln("next32 didn't return DONE at the end");
        c=iter.setIndex32(text.length()+1);
        if(c!= CharacterIterator::DONE)
            errln("setIndex32(len+1) didn't return DONE");


        c = iter.last32();
        i = text.length()-1;
        logln("Testing backward iteration...");
        do {
            if (c == CharacterIterator::DONE && i >= 0)
                errln((UnicodeString)"Iterator reached start prematurely for i=" + i);
            else if(iter.hasPrevious() == FALSE && i>0)
                errln((UnicodeString)"Iterator reached start prematurely for i=" + i);
            else if (c != text.char32At(i))
                errln("Character mismatch at position %d, iterator has %X, string has %X", i, c, text.char32At(i));

            if (iter.current32() != c)
                errln("current32() isn't working right");
            if(iter.setIndex32(i) != c)
                errln("setIndex32() isn't working right");
            if (iter.getIndex() != i)
                errln("getIndex() isn't working right");
            if (c != CharacterIterator::DONE) {
                c = iter.previous32();
                i=UTF16_NEED_MULTIPLE_UCHAR(c) ? i-2 : i-1;
            }
        } while (c != CharacterIterator::DONE);
        if(iter.hasPrevious() == TRUE)
            errln("hasPrevious returned true after reaching the start");

        c=iter.previous32();
        if(c!= CharacterIterator::DONE)
            errln("previous32 didn't return DONE at the beginning");




        //testing first32PostInc, next32PostInc, setTostart
        i = 0;
        c=iter.first32PostInc();
        if(c != text.char32At(i))
            errln("first32PostInc failed.  Expected->%X Got->%X", text.char32At(i), c);
        if(iter.getIndex() != UTF16_CHAR_LENGTH(c) + i)
            errln((UnicodeString)"getIndex() after first32PostInc() failed");

        iter.setToStart();
        i=0;
        if (iter.startIndex() != 0)
            errln("setToStart failed");
       
        logln("Testing forward iteration...");
        do {
            if (c != CharacterIterator::DONE)
                c = iter.next32PostInc();

            if(c != text.char32At(i))
                errln("Character mismatch at position %d, iterator has %X, string has %X", i, c, text.char32At(i));

            i=UTF16_NEED_MULTIPLE_UCHAR(c) ? i+2 : i+1;
            if(iter.getIndex() != i)
                errln("getIndex() aftr next32PostInc() isn't working right");
            if(iter.current32() != text.char32At(i))
                errln("current() after next32PostInc() isn't working right");
        } while (iter.hasNext());
        c=iter.next32PostInc();
        if(c!= CharacterIterator::DONE)
            errln("next32PostInc() didn't return DONE at the beginning");


    }

    {
        StringCharacterIterator iter(text, 1, 11, 10);
        if (iter.startIndex() != 1 || iter.endIndex() != 11)
            errln("creation of a restricted-range iterator failed");

        if (iter.getIndex() != 10 || iter.current32() != text.char32At(10))
            errln("starting the iterator in the middle didn't work");

        c = iter.first32();
       
        i = 1;

        logln("Testing forward iteration over a range...");
        do {
            if (c == CharacterIterator::DONE && i != 11)
                errln("Iterator reached end prematurely");
            else if(iter.hasNext() == FALSE)
                errln("Iterator reached end prematurely");
            else if (c != text.char32At(i))
                errln("Character mismatch at position %d, iterator has %X, string has %X", i, c, text.char32At(i));

            if (iter.current32() != c)
                errln("current32() isn't working right");
            if(iter.setIndex32(i) != c)
                errln("setIndex32() isn't working right");

            if (c != CharacterIterator::DONE) {
                c = iter.next32();
                i=UTF16_NEED_MULTIPLE_UCHAR(c) ? i+2 : i+1;
            }
        } while (c != CharacterIterator::DONE);
        c=iter.next32();
        if(c != CharacterIterator::DONE) 
            errln("error in next32()");


           
        c=iter.last32();
        i = 10;
        logln("Testing backward iteration over a range...");
        do {
            if (c == CharacterIterator::DONE && i >= 5)
                errln("Iterator reached start prematurely");
            else if(iter.hasPrevious() == FALSE && i > 5)
                errln("Iterator reached start prematurely");
            else if (c != text.char32At(i))
                errln("Character mismatch at position %d, iterator has %X, string has %X", i, c, text.char32At(i));
            if (iter.current32() != c)
                errln("current32() isn't working right");
            if (iter.getIndex() != i)
                errln("getIndex() isn't working right");
            if(iter.setIndex32(i) != c)
                errln("setIndex32() isn't working right");

            if (c != CharacterIterator::DONE) {
                c = iter.previous32();
                i=UTF16_NEED_MULTIPLE_UCHAR(c) ? i-2 : i-1;
            }
           
        } while (c != CharacterIterator::DONE);
        c=iter.previous32();
        if(c!= CharacterIterator::DONE)
            errln("error on previous32");

                
    }
}

void CharIterTest::TestUCharIterator(UCharIterator *iter, CharacterIterator &ci,
                                     const char *moves, const char *which) {
    int32_t m;
    UChar32 c, c2;
    UBool h, h2;

    for(m=0;; ++m) {
        // move both iter and s[index]
        switch(moves[m]) {
        case '0':
            h=iter->hasNext(iter);
            h2=ci.hasNext();
            c=iter->current(iter);
            c2=ci.current();
            break;
        case '|':
            h=iter->hasNext(iter);
            h2=ci.hasNext();
            c=uiter_current32(iter);
            c2=ci.current32();
            break;

        case '+':
            h=iter->hasNext(iter);
            h2=ci.hasNext();
            c=iter->next(iter);
            c2=ci.nextPostInc();
            break;
        case '>':
            h=iter->hasNext(iter);
            h2=ci.hasNext();
            c=uiter_next32(iter);
            c2=ci.next32PostInc();
            break;

        case '-':
            h=iter->hasPrevious(iter);
            h2=ci.hasPrevious();
            c=iter->previous(iter);
            c2=ci.previous();
            break;
        case '<':
            h=iter->hasPrevious(iter);
            h2=ci.hasPrevious();
            c=uiter_previous32(iter);
            c2=ci.previous32();
            break;

        case '2':
            h=h2=FALSE;
            c=(UChar32)iter->move(iter, 2, UITER_CURRENT);
            c2=(UChar32)ci.move(2, CharacterIterator::kCurrent);
            break;

        case '8':
            h=h2=FALSE;
            c=(UChar32)iter->move(iter, -2, UITER_CURRENT);
            c2=(UChar32)ci.move(-2, CharacterIterator::kCurrent);
            break;

        case 0:
            return;
        default:
            errln("error: unexpected move character '%c' in \"%s\"", moves[m], moves);
            return;
        }

        // compare results
        if(c2==0xffff) {
            c2=(UChar32)-1;
        }
        if(c!=c2 || h!=h2 || ci.getIndex()!=iter->getIndex(iter, UITER_CURRENT)) {
            errln("error: UCharIterator(%s) misbehaving at \"%s\"[%d]='%c'", which, moves, m, moves[m]);
        }
    }
}

void CharIterTest::TestUCharIterator() {
    // test string of length 8
    UnicodeString s=UnicodeString("a \\U00010001b\\U0010fffdz", "").unescape();
    const char *const moves=
        "0+++++++++" // 10 moves per line
        "----0-----"
        ">>|>>>>>>>"
        "<<|<<<<<<<"
        "22+>8>-8+2";

    StringCharacterIterator sci(s), compareCI(s);

    UCharIterator sIter, cIter, rIter;

    uiter_setString(&sIter, s.getBuffer(), s.length());
    uiter_setCharacterIterator(&cIter, &sci);
    uiter_setReplaceable(&rIter, &s);

    TestUCharIterator(&sIter, compareCI, moves, "uiter_setString");
    compareCI.setIndex(0);
    TestUCharIterator(&cIter, compareCI, moves, "uiter_setCharacterIterator");
    compareCI.setIndex(0);
    TestUCharIterator(&rIter, compareCI, moves, "uiter_setReplaceable");

    // test move & getIndex some more
    sIter.start=2;
    sIter.index=3;
    sIter.limit=5;
    if( sIter.getIndex(&sIter, UITER_ZERO)!=0 ||
        sIter.getIndex(&sIter, UITER_START)!=2 ||
        sIter.getIndex(&sIter, UITER_CURRENT)!=3 ||
        sIter.getIndex(&sIter, UITER_LIMIT)!=5 ||
        sIter.getIndex(&sIter, UITER_LENGTH)!=s.length()
    ) {
        errln("error: UCharIterator(string).getIndex returns wrong index");
    }

    if( sIter.move(&sIter, 4, UITER_ZERO)!=4 ||
        sIter.move(&sIter, 1, UITER_START)!=3 ||
        sIter.move(&sIter, 3, UITER_CURRENT)!=5 ||
        sIter.move(&sIter, -1, UITER_LIMIT)!=4 ||
        sIter.move(&sIter, -5, UITER_LENGTH)!=3 ||
        sIter.move(&sIter, 0, UITER_CURRENT)!=sIter.getIndex(&sIter, UITER_CURRENT) ||
        sIter.getIndex(&sIter, UITER_CURRENT)!=3
    ) {
        errln("error: UCharIterator(string).move sets/returns wrong index");
    }

    sci=StringCharacterIterator(s, 2, 5, 3);
    uiter_setCharacterIterator(&cIter, &sci);
    if( cIter.getIndex(&cIter, UITER_ZERO)!=0 ||
        cIter.getIndex(&cIter, UITER_START)!=2 ||
        cIter.getIndex(&cIter, UITER_CURRENT)!=3 ||
        cIter.getIndex(&cIter, UITER_LIMIT)!=5 ||
        cIter.getIndex(&cIter, UITER_LENGTH)!=s.length()
    ) {
        errln("error: UCharIterator(character iterator).getIndex returns wrong index");
    }

    if( cIter.move(&cIter, 4, UITER_ZERO)!=4 ||
        cIter.move(&cIter, 1, UITER_START)!=3 ||
        cIter.move(&cIter, 3, UITER_CURRENT)!=5 ||
        cIter.move(&cIter, -1, UITER_LIMIT)!=4 ||
        cIter.move(&cIter, -5, UITER_LENGTH)!=3 ||
        cIter.move(&cIter, 0, UITER_CURRENT)!=cIter.getIndex(&cIter, UITER_CURRENT) ||
        cIter.getIndex(&cIter, UITER_CURRENT)!=3
    ) {
        errln("error: UCharIterator(character iterator).move sets/returns wrong index");
    }


    if(cIter.getIndex(&cIter, (enum UCharIteratorOrigin)-1) != -1)
    {
        errln("error: UCharIterator(char iter).getIndex did not return error value");
    }

    if(cIter.move(&cIter, 0, (enum UCharIteratorOrigin)-1) != -1)
    {
        errln("error: UCharIterator(char iter).move did not return error value");
    }


    if(rIter.getIndex(&rIter, (enum UCharIteratorOrigin)-1) != -1)
    {
        errln("error: UCharIterator(repl iter).getIndex did not return error value");
    }

    if(rIter.move(&rIter, 0, (enum UCharIteratorOrigin)-1) != -1)
    {
        errln("error: UCharIterator(repl iter).move did not return error value");
    }


    if(sIter.getIndex(&sIter, (enum UCharIteratorOrigin)-1) != -1)
    {
        errln("error: UCharIterator(string iter).getIndex did not return error value");
    }

    if(sIter.move(&sIter, 0, (enum UCharIteratorOrigin)-1) != -1)
    {
        errln("error: UCharIterator(string iter).move did not return error value");
    }

    /* Testing function coverage on bad input */
    UErrorCode status = U_ZERO_ERROR;
    uiter_setString(&sIter, NULL, 1);
    uiter_setState(&sIter, 1, &status);
    if (status != U_UNSUPPORTED_ERROR) {
        errln("error: uiter_setState returned %s instead of U_UNSUPPORTED_ERROR", u_errorName(status));
    }
    status = U_ZERO_ERROR;
    uiter_setState(NULL, 1, &status);
    if (status != U_ILLEGAL_ARGUMENT_ERROR) {
        errln("error: uiter_setState returned %s instead of U_ILLEGAL_ARGUMENT_ERROR", u_errorName(status));
    }
    if (uiter_getState(&sIter) != UITER_NO_STATE) {
        errln("error: uiter_getState did not return UITER_NO_STATE on bad input");
    }
}

// subclass test, and completing API coverage -------------------------------

class SubCharIter : public CharacterIterator {
public:
    // public default constructor, to get coverage of CharacterIterator()
    SubCharIter() : CharacterIterator() {
        textLength=end=LENGTHOF(s);
        s[0]=0x61;      // 'a'
        s[1]=0xd900;    // U+50400
        s[2]=0xdd00;
        s[3]=0x2029;    // PS
    }

    // useful stuff, mostly dummy but testing coverage and subclassability
    virtual UChar nextPostInc() {
        if(pos<LENGTHOF(s)) {
            return s[pos++];
        } else {
            return DONE;
        }
    }

    virtual UChar32 next32PostInc() {
        if(pos<LENGTHOF(s)) {
            UChar32 c;
            U16_NEXT(s, pos, LENGTHOF(s), c);
            return c;
        } else {
            return DONE;
        }
    }

    virtual UBool hasNext() {
        return pos<LENGTHOF(s);
    }

    virtual UChar first() {
        pos=0;
        return s[0];
    }

    virtual UChar32 first32() {
        UChar32 c;
        pos=0;
        U16_NEXT(s, pos, LENGTHOF(s), c);
        pos=0;
        return c;
    }

    virtual UChar setIndex(int32_t position) {
        if(0<=position && position<=LENGTHOF(s)) {
            pos=position;
            if(pos<LENGTHOF(s)) {
                return s[pos];
            }
        }
        return DONE;
    }

    virtual UChar32 setIndex32(int32_t position) {
        if(0<=position && position<=LENGTHOF(s)) {
            pos=position;
            if(pos<LENGTHOF(s)) {
                UChar32 c;
                U16_GET(s, 0, pos, LENGTHOF(s), c);
                return c;
            }
        }
        return DONE;
    }

    virtual UChar current() const {
        if(pos<LENGTHOF(s)) {
            return s[pos];
        } else {
            return DONE;
        }
    }

    virtual UChar32 current32() const {
        if(pos<LENGTHOF(s)) {
            UChar32 c;
            U16_GET(s, 0, pos, LENGTHOF(s), c);
            return c;
        } else {
            return DONE;
        }
    }

    virtual UChar next() {
        if(pos<LENGTHOF(s) && ++pos<LENGTHOF(s)) {
            return s[pos];
        } else {
            return DONE;
        }
    }

    virtual UChar32 next32() {
        if(pos<LENGTHOF(s)) {
            U16_FWD_1(s, pos, LENGTHOF(s));
        }
        if(pos<LENGTHOF(s)) {
            UChar32 c;
            int32_t i=pos;
            U16_NEXT(s, i, LENGTHOF(s), c);
            return c;
        } else {
            return DONE;
        }
    }

    virtual UBool hasPrevious() {
        return pos>0;
    }

    virtual void getText(UnicodeString &result) {
        result.setTo(s, LENGTHOF(s));
    }

    // dummy implementations of other pure virtual base class functions
    virtual UBool operator==(const ForwardCharacterIterator &that) const {
        return
            this==&that ||
            (typeid(*this)==typeid(that) && pos==((SubCharIter &)that).pos);
    }

    virtual int32_t hashCode() const {
        return 2;
    }

    virtual CharacterIterator *clone() const {
        return NULL;
    }

    virtual UChar last() {
        return 0;
    }

    virtual UChar32 last32() {
        return 0;
    }

    virtual UChar previous() {
        return 0;
    }

    virtual UChar32 previous32() {
        return 0;
    }

    virtual int32_t move(int32_t /*delta*/, EOrigin /*origin*/) {
        return 0;
    }

    virtual int32_t move32(int32_t /*delta*/, EOrigin /*origin*/) {
        return 0;
    }

    // RTTI
    static UClassID getStaticClassID() {
        return (UClassID)(&fgClassID);
    }

    virtual UClassID getDynamicClassID() const {
        return getStaticClassID();
    }

private:
    // dummy string data
    UChar s[4];

    static const char fgClassID;
};

const char SubCharIter::fgClassID = 0;

class SubStringCharIter : public StringCharacterIterator {
public:
    SubStringCharIter() {
        setText(UNICODE_STRING("abc", 3));
    }
};

class SubUCharCharIter : public UCharCharacterIterator {
public:
    SubUCharCharIter() {
        setText(u, 3);
    }

private:
    static const UChar u[3];
};

const UChar SubUCharCharIter::u[3]={ 0x61, 0x62, 0x63 };

void CharIterTest::TestCharIteratorSubClasses() {
    SubCharIter *p;

    // coverage - call functions that are not otherwise tested
    // first[32]PostInc() are default implementations that are overridden
    // in ICU's own CharacterIterator subclasses
    p=new SubCharIter;
    if(p->firstPostInc()!=0x61) {
        errln("SubCharIter.firstPosInc() failed\n");
    }
    delete p;

    p=new SubCharIter[2];
    if(p[1].first32PostInc()!=0x61) {
        errln("SubCharIter.first32PosInc() failed\n");
    }
    delete [] p;

    // coverage: StringCharacterIterator default constructor
    SubStringCharIter sci;
    if(sci.firstPostInc()!=0x61) {
        errln("SubStringCharIter.firstPostInc() failed\n");
    }

    // coverage: UCharCharacterIterator default constructor
    SubUCharCharIter uci;
    if(uci.firstPostInc()!=0x61) {
        errln("SubUCharCharIter.firstPostInc() failed\n");
    }
}
