/********************************************************************
 * COPYRIGHT: 
 * Copyright (c) 1997-2009, International Business Machines Corporation and
 * others. All Rights Reserved.
 ********************************************************************/

#include "unicode/utypes.h"

#if !UCONFIG_NO_COLLATION

#include "unicode/coll.h"
#include "unicode/tblcoll.h"
#include "unicode/unistr.h"
#include "unicode/sortkey.h"
#include "itercoll.h"
#include "unicode/schriter.h"
#include "unicode/chariter.h"
#include "unicode/uchar.h"
#include "cmemory.h"

#define ARRAY_LENGTH(array) (sizeof array / sizeof array[0])

static UErrorCode status = U_ZERO_ERROR;

CollationIteratorTest::CollationIteratorTest()
 : test1("What subset of all possible test cases?", ""),
   test2("has the highest probability of detecting", "")
{
    en_us = (RuleBasedCollator *)Collator::createInstance(Locale::getUS(), status);
    if(U_FAILURE(status)) {
      delete en_us;
      en_us = 0;
      errcheckln(status, "Collator creation failed with %s", u_errorName(status));
      return;
    }

}

CollationIteratorTest::~CollationIteratorTest()
{
    delete en_us;
}

/**
 * Test for CollationElementIterator previous and next for the whole set of
 * unicode characters.
 */
void CollationIteratorTest::TestUnicodeChar()
{
    CollationElementIterator *iter;
    UChar codepoint;
    UnicodeString source;
    
    for (codepoint = 1; codepoint < 0xFFFE;)
    {
      source.remove();

      while (codepoint % 0xFF != 0) 
      {
        if (u_isdefined(codepoint))
          source += codepoint;
        codepoint ++;
      }

      if (u_isdefined(codepoint))
        source += codepoint;
      
      if (codepoint != 0xFFFF)
        codepoint ++;

      iter = en_us->createCollationElementIterator(source);
      /* A basic test to see if it's working at all */
      backAndForth(*iter);
      delete iter;
    }
}

/**
 * Test for CollationElementIterator.previous()
 *
 * @bug 4108758 - Make sure it works with contracting characters
 * 
 */
void CollationIteratorTest::TestPrevious(/* char* par */)
{
    UErrorCode status = U_ZERO_ERROR;
    CollationElementIterator *iter = en_us->createCollationElementIterator(test1);

    // A basic test to see if it's working at all
    backAndForth(*iter);
    delete iter;

    // Test with a contracting character sequence
    UnicodeString source;
    RuleBasedCollator *c1 = NULL;
    c1 = new RuleBasedCollator(
        (UnicodeString)"&a,A < b,B < c,C, d,D < z,Z < ch,cH,Ch,CH", status);

    if (c1 == NULL || U_FAILURE(status))
    {
        errln("Couldn't create a RuleBasedCollator with a contracting sequence.");
        delete c1;
        return;
    }

    source = "abchdcba";
    iter = c1->createCollationElementIterator(source);
    backAndForth(*iter);
    delete iter;
    delete c1;

    // Test with an expanding character sequence
    RuleBasedCollator *c2 = NULL;
    c2 = new RuleBasedCollator((UnicodeString)"&a < b < c/abd < d", status);

    if (c2 == NULL || U_FAILURE(status))
    {
        errln("Couldn't create a RuleBasedCollator with an expanding sequence.");
        delete c2;
        return;
    }

    source = "abcd";
    iter = c2->createCollationElementIterator(source);
    backAndForth(*iter);
    delete iter;
    delete c2;

    // Now try both
    RuleBasedCollator *c3 = NULL;
    c3 = new RuleBasedCollator((UnicodeString)"&a < b < c/aba < d < z < ch", status);

    if (c3 == NULL || U_FAILURE(status))
    {
        errln("Couldn't create a RuleBasedCollator with both an expanding and a contracting sequence.");
        delete c3;
        return;
    }

    source = "abcdbchdc";
    iter = c3->createCollationElementIterator(source);
    backAndForth(*iter);
    delete iter;
    delete c3;

    status=U_ZERO_ERROR;
    source= CharsToUnicodeString("\\u0e41\\u0e02\\u0e41\\u0e02\\u0e27abc");
    
    Collator *c4 = Collator::createInstance(Locale("th", "TH", ""), status);
    if(U_FAILURE(status)){
        errln("Couldn't create a collator");
    }
    iter = ((RuleBasedCollator*)c4)->createCollationElementIterator(source);
    backAndForth(*iter);
    delete iter;
    delete c4;
   
    source= CharsToUnicodeString("\\u0061\\u30CF\\u3099\\u30FC");
    Collator *c5 = Collator::createInstance(Locale("ja", "JP", ""), status);

    iter = ((RuleBasedCollator*)c5)->createCollationElementIterator(source);
    if(U_FAILURE(status)){
        errln("Couldn't create Japanese collator\n");
    }
    backAndForth(*iter);
    delete iter;
    delete c5;
}

/**
 * Test for getOffset() and setOffset()
 */
void CollationIteratorTest::TestOffset(/* char* par */)
{
    CollationElementIterator *iter = en_us->createCollationElementIterator(test1);
    UErrorCode status = U_ZERO_ERROR;
    // testing boundaries
    iter->setOffset(0, status);
    if (U_FAILURE(status) || iter->previous(status) != UCOL_NULLORDER) {
        errln("Error: After setting offset to 0, we should be at the end "
                "of the backwards iteration");
    }
    iter->setOffset(test1.length(), status);
    if (U_FAILURE(status) || iter->next(status) != UCOL_NULLORDER) {
        errln("Error: After setting offset to end of the string, we should "
                "be at the end of the backwards iteration");
    }

    // Run all the way through the iterator, then get the offset
    int32_t orderLength = 0;
    Order *orders = getOrders(*iter, orderLength);

    int32_t offset = iter->getOffset();

    if (offset != test1.length())
    {
        UnicodeString msg1("offset at end != length: ");
        UnicodeString msg2(" vs ");

        errln(msg1 + offset + msg2 + test1.length());
    }

    // Now set the offset back to the beginning and see if it works
    CollationElementIterator *pristine = en_us->createCollationElementIterator(test1);

    iter->setOffset(0, status);

    if (U_FAILURE(status))
    {
        errln("setOffset failed.");
    }
    else
    {
        assertEqual(*iter, *pristine);
    }

    // TODO: try iterating halfway through a messy string.

    delete pristine;
    delete[] orders;
    delete iter;
}

/**
 * Test for setText()
 */
void CollationIteratorTest::TestSetText(/* char* par */)
{
    CollationElementIterator *iter1 = en_us->createCollationElementIterator(test1);
    CollationElementIterator *iter2 = en_us->createCollationElementIterator(test2);
    UErrorCode status = U_ZERO_ERROR;

    // Run through the second iterator just to exercise it
    int32_t c = iter2->next(status);
    int32_t i = 0;

    while ( ++i < 10 && c != CollationElementIterator::NULLORDER)
    {
        if (U_FAILURE(status))
        {
            errln("iter2->next() returned an error.");
            delete iter2;
            delete iter1;
        }

        c = iter2->next(status);
    }

    // Now set it to point to the same string as the first iterator
    iter2->setText(test1, status);

    if (U_FAILURE(status))
    {
        errln("call to iter2->setText(test1) failed.");
    }
    else
    {
        assertEqual(*iter1, *iter2);
    }
    iter1->reset();
    //now use the overloaded setText(ChracterIterator&, UErrorCode) function to set the text
    CharacterIterator* chariter = new StringCharacterIterator(test1);
    iter2->setText(*chariter, status);
    if (U_FAILURE(status))
    {
        errln("call to iter2->setText(chariter(test1)) failed.");
    }
    else
    {
        assertEqual(*iter1, *iter2);
    }
   
    // test for an empty string
    UnicodeString empty("");
    iter1->setText(empty, status);
    if (U_FAILURE(status) 
        || iter1->next(status) != (int32_t)UCOL_NULLORDER) {
        errln("Empty string should have no CEs.");
    }
    ((StringCharacterIterator *)chariter)->setText(empty);
    iter1->setText(*chariter, status);
    if (U_FAILURE(status) 
        || iter1->next(status) != (int32_t)UCOL_NULLORDER) {
        errln("Empty string should have no CEs.");
    }
    delete chariter;
    delete iter2;
    delete iter1;
}

/** @bug 4108762
 * Test for getMaxExpansion()
 */
void CollationIteratorTest::TestMaxExpansion(/* char* par */)
{
    UErrorCode          status = U_ZERO_ERROR; 
    UnicodeString rule("&a < ab < c/aba < d < z < ch");
    RuleBasedCollator  *coll   = new RuleBasedCollator(rule, status);
    UChar               ch     = 0;
    UnicodeString       str(ch);

    CollationElementIterator *iter   = coll->createCollationElementIterator(str);

    while (ch < 0xFFFF && U_SUCCESS(status)) {
        int      count = 1;
        uint32_t order;
        ch ++;
        UnicodeString str(ch);
        iter->setText(str, status);
        order = iter->previous(status);

        /* thai management */
        if (CollationElementIterator::isIgnorable(order))
            order = iter->previous(status);

        while (U_SUCCESS(status)
            && iter->previous(status) != (int32_t)UCOL_NULLORDER)
        {
            count ++; 
        }

        if (U_FAILURE(status) && iter->getMaxExpansion(order) < count) {
            errln("Failure at codepoint %d, maximum expansion count < %d\n",
                ch, count);
        }
    }

    delete iter;
    delete coll;
}

/*
 * @bug 4157299
 */
void CollationIteratorTest::TestClearBuffers(/* char* par */)
{
    UErrorCode status = U_ZERO_ERROR;
    RuleBasedCollator *c = new RuleBasedCollator((UnicodeString)"&a < b < c & ab = d", status);

    if (c == NULL || U_FAILURE(status))
    {
        errln("Couldn't create a RuleBasedCollator.");
        delete c;
        return;
    }

    UnicodeString source("abcd");
    CollationElementIterator *i = c->createCollationElementIterator(source);
    int32_t e0 = i->next(status);    // save the first collation element

    if (U_FAILURE(status))
    {
        errln("call to i->next() failed. err=%s", u_errorName(status));
    }
    else
    {
        i->setOffset(3, status);        // go to the expanding character

        if (U_FAILURE(status))
        {
            errln("call to i->setOffset(3) failed. err=%s", u_errorName(status));
        }
        else
        {
            i->next(status);                // but only use up half of it

            if (U_FAILURE(status))
            {
                errln("call to i->next() failed. err=%s", u_errorName(status));
            }
            else
            {
                i->setOffset(0, status);        // go back to the beginning

                if (U_FAILURE(status))
                {
                    errln("call to i->setOffset(0) failed. err=%s", u_errorName(status));
                }
                else
                {
                    int32_t e = i->next(status);    // and get this one again

                    if (U_FAILURE(status))
                    {
                        errln("call to i->next() failed. err=%s", u_errorName(status));
                    }
                    else if (e != e0)
                    {
                        errln("got 0x%X, expected 0x%X", e, e0);
                    }
                }
            }
        }
    }

    delete i;
    delete c;
}

/**
 * Testing the assignment operator
 */
void CollationIteratorTest::TestAssignment()
{
    UErrorCode status = U_ZERO_ERROR;
    RuleBasedCollator *coll = 
        (RuleBasedCollator *)Collator::createInstance(status);

    if (coll == NULL || U_FAILURE(status))
    {
        errln("Couldn't create a default collator.");
        return;
    }

    UnicodeString source("abcd");
    CollationElementIterator *iter1 = 
        coll->createCollationElementIterator(source);

    CollationElementIterator iter2 = *iter1;

    if (*iter1 != iter2) {
        errln("Fail collation iterator assignment does not produce the same elements");
    }

    CollationElementIterator iter3(*iter1);

    if (*iter1 != iter3) {
        errln("Fail collation iterator copy constructor does not produce the same elements");
    }

    source = CharsToUnicodeString("a\\u0300\\u0325");
    coll->setAttribute(UCOL_NORMALIZATION_MODE, UCOL_ON, status);
    CollationElementIterator *iter4 
                        = coll->createCollationElementIterator(source);
    CollationElementIterator iter5(*iter4);
    if (*iter4 != iter5) {
        errln("collation iterator assignment does not produce the same elements");
    }
    iter4->next(status);
    if (U_FAILURE(status) || *iter4 == iter5) {
        errln("collation iterator not equal");
    }
    iter5.next(status);
    if (U_FAILURE(status) || *iter4 != iter5) {
        errln("collation iterator equal");
    }
    iter4->next(status);
    if (U_FAILURE(status) || *iter4 == iter5) {
        errln("collation iterator not equal");
    }
    iter5.next(status);
    if (U_FAILURE(status) || *iter4 != iter5) {
        errln("collation iterator equal");
    }
    CollationElementIterator iter6(*iter4);
    if (*iter4 != iter6) {
        errln("collation iterator equal");
    }
    iter4->next(status);
    if (U_FAILURE(status) || *iter4 == iter5) {
        errln("collation iterator not equal");
    }
    iter5.next(status);
    if (U_FAILURE(status) || *iter4 != iter5) {
        errln("collation iterator equal");
    }
    iter4->next(status);
    if (U_FAILURE(status) || *iter4 == iter5) {
        errln("collation iterator not equal");
    }
    iter5.next(status);
    if (U_FAILURE(status) || *iter4 != iter5) {
        errln("collation iterator equal");
    }
    delete iter1;
    delete iter4;
    delete coll;
}

/**
 * Testing the constructors
 */
void CollationIteratorTest::TestConstructors()
{
    UErrorCode status = U_ZERO_ERROR;
    RuleBasedCollator *coll = 
        (RuleBasedCollator *)Collator::createInstance(status);
    if (coll == NULL || U_FAILURE(status))
    {
        errln("Couldn't create a default collator.");
        return;
    }

    // testing protected constructor with character iterator as argument
    StringCharacterIterator chariter(test1);
    CollationElementIterator *iter1 = 
        coll->createCollationElementIterator(chariter);
    if (U_FAILURE(status)) {
        errln("Couldn't create collation element iterator with character iterator.");
        return;
    }
    CollationElementIterator *iter2 = 
        coll->createCollationElementIterator(test1);

    // initially the 2 collation element iterators should be the same
    if (*iter1 != *iter1 || *iter2 != *iter2 || *iter1 != *iter2 
        || *iter2 != *iter1) {
        errln("CollationElementIterators constructed with the same string data should be the same at the start");
    }
    assertEqual(*iter1, *iter2);

    delete iter1;
    delete iter2;

    // tests empty strings
    UnicodeString empty("");
    iter1 = coll->createCollationElementIterator(empty);
    chariter.setText(empty);
    iter2 = coll->createCollationElementIterator(chariter);
    if (*iter1 != *iter1 || *iter2 != *iter2 || *iter1 != *iter2 
        || *iter2 != *iter1) {
        errln("CollationElementIterators constructed with the same string data should be the same at the start");
    } 
    if (iter1->next(status) != (int32_t)UCOL_NULLORDER) {
        errln("Empty string should have no CEs.");
    }
    if (iter2->next(status) != (int32_t)UCOL_NULLORDER) {
        errln("Empty string should have no CEs.");
    }
    delete iter1;
    delete iter2;
    delete coll;
}

/**
 * Testing the strength order
 */
void CollationIteratorTest::TestStrengthOrder()
{
    int order = 0x0123ABCD;

    UErrorCode status = U_ZERO_ERROR;
    RuleBasedCollator *coll = 
        (RuleBasedCollator *)Collator::createInstance(status);
    if (coll == NULL || U_FAILURE(status))
    {
        errln("Couldn't create a default collator.");
        return;
    }

    coll->setStrength(Collator::PRIMARY);
    CollationElementIterator *iter = 
        coll->createCollationElementIterator(test1);

    if (iter == NULL) {
        errln("Couldn't create a collation element iterator from default collator");
        return;
    }

    if (iter->strengthOrder(order) != 0x01230000) {
        errln("Strength order for a primary strength collator should be the first 2 bytes");
        return;
    }

    coll->setStrength(Collator::SECONDARY);
    if (iter->strengthOrder(order) != 0x0123AB00) {
        errln("Strength order for a secondary strength collator should be the third byte");
        return;
    }

    coll->setStrength(Collator::TERTIARY);
    if (iter->strengthOrder(order) != order) {
        errln("Strength order for a tertiary strength collator should be the third byte");
        return;
    }
    delete iter;
    delete coll;
}

/**
 * Return a string containing all of the collation orders
 * returned by calls to next on the specified iterator
 */
UnicodeString &CollationIteratorTest::orderString(CollationElementIterator &iter, UnicodeString &target)
{
    int32_t order;
    UErrorCode status = U_ZERO_ERROR;

    while ((order = iter.next(status)) != CollationElementIterator::NULLORDER)
    {
        target += "0x";
        appendHex(order, 8, target);
        target += " ";
    }

    return target;
}

void CollationIteratorTest::assertEqual(CollationElementIterator &i1, CollationElementIterator &i2)
{
    int32_t c1, c2, count = 0;
    UErrorCode status = U_ZERO_ERROR;

    do
    {
        c1 = i1.next(status);
        c2 = i2.next(status);

        if (c1 != c2)
        {
            errln("    %d: strength(0x%X) != strength(0x%X)", count, c1, c2);
            break;
        }

        count += 1;
    }
    while (c1 != CollationElementIterator::NULLORDER);
}

void CollationIteratorTest::runIndexedTest(int32_t index, UBool exec, const char* &name, char* /*par*/)
{
    if (exec)
    {
        logln("Collation Iteration Tests: ");
    }

    if(en_us) {
      switch (index)
      {
          case  0: name = "TestPrevious";      if (exec) TestPrevious(/* par */);     break;
          case  1: name = "TestOffset";        if (exec) TestOffset(/* par */);       break;
          case  2: name = "TestSetText";       if (exec) TestSetText(/* par */);      break;
          case  3: name = "TestMaxExpansion";  if (exec) TestMaxExpansion(/* par */); break;
          case  4: name = "TestClearBuffers";  if (exec) TestClearBuffers(/* par */); break;
          case  5: name = "TestUnicodeChar";   if (exec) TestUnicodeChar(/* par */);  break;
          case  6: name = "TestAssignment";    if (exec) TestAssignment(/* par */);    break;
          case  7: name = "TestConstructors";  if (exec) TestConstructors(/* par */); break;
          case  8: name = "TestStrengthOrder"; if (exec) TestStrengthOrder(/* par */); break;
          default: name = ""; break;
      }
    } else {
      dataerrln("Class iterator not instantiated");
      name = "";
    }
}

#endif /* #if !UCONFIG_NO_COLLATION */
