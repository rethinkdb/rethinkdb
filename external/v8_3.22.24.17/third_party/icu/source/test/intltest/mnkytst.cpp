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
#include "mnkytst.h"
#include "sfwdchit.h"

#include <stdlib.h>
#include <stdio.h>
#include <time.h>

#ifndef MIN
#define MIN(x,y) ((x) < (y) ? (x) : (y))
#endif

#ifndef MAX
#define MAX(x,y) ((x) > (y) ? (x) : (y))
#endif

CollationMonkeyTest::CollationMonkeyTest()
: source("-abcdefghijklmnopqrstuvwxyz#&^$@", ""),
  myCollator(0)
{
    UErrorCode status = U_ZERO_ERROR;
    myCollator = Collator::createInstance("en_US", status);
}

CollationMonkeyTest::~CollationMonkeyTest()
{
    delete myCollator;
}


void 
CollationMonkeyTest::report(UnicodeString& s, UnicodeString& t, int32_t result, int32_t revResult)
{
    if (revResult != -result)
    {
        UnicodeString msg;

        msg += s; 
        msg += " and ";
        msg += t;
        msg += " round trip comparison failed";
        msg += (UnicodeString) " (result " + result + ", reverse Result " + revResult + ")"; 

        errln(msg);
    }
}

int32_t 
CollationMonkeyTest::checkValue(int32_t value)
{
    if (value < 0)
    {
        return -value;
    }

    return value;
}

void CollationMonkeyTest::TestCollationKey(/* char* par */)
{
    if(source.length() == 0) {
        errln(UNICODE_STRING("CollationMonkeyTest::TestCollationKey(): source is empty - ICU_DATA not set or data missing?", 92));
        return;
    }

    srand( (unsigned)time( NULL ) );
    int32_t s = checkValue(rand() % source.length());
    int32_t t = checkValue(rand() % source.length());
    int32_t slen = checkValue((rand() - source.length()) % source.length());
    int32_t tlen = checkValue((rand() - source.length()) % source.length());
    UnicodeString subs, subt;

    source.extract(MIN(s, slen), MAX(s, slen), subs);
    source.extract(MIN(t, tlen), MAX(t, tlen), subt);

    CollationKey collationKey1, collationKey2;
    UErrorCode status1 = U_ZERO_ERROR, status2= U_ZERO_ERROR;

    myCollator->setStrength(Collator::TERTIARY);
    myCollator->getCollationKey(subs, collationKey1, status1);
    myCollator->getCollationKey(subt, collationKey2, status2);
    int32_t result = collationKey1.compareTo(collationKey2);  // Tertiary
    int32_t revResult = collationKey2.compareTo(collationKey1);  // Tertiary
    report( subs, subt, result, revResult);

    myCollator->setStrength(Collator::SECONDARY);
    myCollator->getCollationKey(subs, collationKey1, status1);
    myCollator->getCollationKey(subt, collationKey2, status2);
    result = collationKey1.compareTo(collationKey2);  // Secondary
    revResult = collationKey2.compareTo(collationKey1);   // Secondary
    report( subs, subt, result, revResult);

    myCollator->setStrength(Collator::PRIMARY);
    myCollator->getCollationKey(subs, collationKey1, status1);
    myCollator->getCollationKey(subt, collationKey2, status2);
    result = collationKey1.compareTo(collationKey2);  // Primary
    revResult = collationKey2.compareTo(collationKey1);   // Primary
    report(subs, subt, result, revResult);

    UnicodeString msg;
    UnicodeString addOne(subs);
    addOne += (UChar32)0xE000;

    myCollator->getCollationKey(subs, collationKey1, status1);
    myCollator->getCollationKey(addOne, collationKey2, status2);
    result = collationKey1.compareTo(collationKey2);
    if (result != -1)
    {
        msg += "CollationKey(";
        msg += subs;
        msg += ") .LT. CollationKey(";
        msg += addOne;
        msg += ") Failed.";
        errln(msg);
    }

    msg.remove();
    result = collationKey2.compareTo(collationKey1);
    if (result != 1)
    {
        msg += "CollationKey(";
        msg += addOne;
        msg += ") .GT. CollationKey(";
        msg += subs;
        msg += ") Failed.";
        errln(msg);
    }
}

void 
CollationMonkeyTest::TestCompare(/* char* par */)
{
    if(source.length() == 0) {
        errln(UNICODE_STRING("CollationMonkeyTest::TestCompare(): source is empty - ICU_DATA not set or data missing?", 87));
        return;
    }

    /* Seed the random-number generator with current time so that
     * the numbers will be different every time we run.
     */
    srand( (unsigned)time( NULL ) );
    int32_t s = checkValue(rand() % source.length());
    int32_t t = checkValue(rand() % source.length());
    int32_t slen = checkValue((rand() - source.length()) % source.length());
    int32_t tlen = checkValue((rand() - source.length()) % source.length());
    UnicodeString subs, subt;

    source.extract(MIN(s, slen), MAX(s, slen), subs);
    source.extract(MIN(t, tlen), MAX(t, tlen), subt);

    myCollator->setStrength(Collator::TERTIARY);
    int32_t result = myCollator->compare(subs, subt);  // Tertiary
    int32_t revResult = myCollator->compare(subt, subs);  // Tertiary
    report(subs, subt, result, revResult);

    myCollator->setStrength(Collator::SECONDARY);
    result = myCollator->compare(subs, subt);  // Secondary
    revResult = myCollator->compare(subt, subs);  // Secondary
    report(subs, subt, result, revResult);

    myCollator->setStrength(Collator::PRIMARY);
    result = myCollator->compare(subs, subt);  // Primary
    revResult = myCollator->compare(subt, subs);  // Primary
    report(subs, subt, result, revResult);

    UnicodeString msg;
    UnicodeString addOne(subs);
    addOne += (UChar32)0xE000;

    result = myCollator->compare(subs, addOne);
    if (result != -1)
    {
        msg += "Test : ";
        msg += subs;
        msg += " .LT. ";
        msg += addOne;
        msg += " Failed.";
        errln(msg);
    }

    msg.remove();
    result = myCollator->compare(addOne, subs);
    if (result != 1)
    {
        msg += "Test : ";
        msg += addOne;
        msg += " .GT. ";
        msg += subs;
        msg += " Failed.";
        errln(msg);
    }
}
void CollationMonkeyTest::TestRules(/* char* par */){
    UChar testSourceCases[][10] = {
    {0x0061, 0x0062, 0x007a, 0},
    {0x0061, 0x0062, 0x007a, 0},
    };

    UChar testTargetCases[][10] = {
        {0x0061, 0x0062, 0x00e4, 0},
        {0x0061, 0x0062, 0x0061, 0x0308, 0},
    };
    
    int i=0;
    logln("Demo Test 1 : Create a new table collation with rules \"& z < 0x00e4\"");
    UErrorCode status = U_ZERO_ERROR;
    Collator *col = Collator::createInstance("en_US", status);
    const UnicodeString baseRules = ((RuleBasedCollator*)col)->getRules();
    UnicodeString newRules(" & z < ");
    newRules.append((UChar)0x00e4);
    newRules.insert(0, baseRules);
    RuleBasedCollator *myCollation = new RuleBasedCollator(newRules, status);
    if (U_FAILURE(status)) {
        errln( "Demo Test 1 Table Collation object creation failed.");
        return;
    }
    for(i=0; i<2; i++){
        doTest(myCollation, testSourceCases[i], testTargetCases[i], Collator::LESS);
    }
    delete myCollation;

    logln("Demo Test 2 : Create a new table collation with rules \"& z < a 0x0308\"");
    newRules.remove();
    newRules.append(" & z < a");
    newRules.append((UChar)0x0308);
    newRules.insert(0, baseRules);
    myCollation = new RuleBasedCollator(newRules, status);
    if (U_FAILURE(status)) {
        errln( "Demo Test 1 Table Collation object creation failed.");
        return;
    }
    for(i=0; i<2; i++){
        doTest(myCollation, testSourceCases[i], testTargetCases[i], Collator::LESS);
    }
    delete myCollation;
    delete col;

}

void CollationMonkeyTest::runIndexedTest( int32_t index, UBool exec, const char* &name, char* /*par*/ )
{
    if (exec) logln("TestSuite CollationMonkeyTest: ");
    if(myCollator) {
      switch (index) {
          case 0: name = "TestCompare";   if (exec)   TestCompare(/* par */); break;
          case 1: name = "TestCollationKey"; if (exec)    TestCollationKey(/* par */); break;
          case 2: name = "TestRules"; if (exec) TestRules(/* par */); break;
          default: name = ""; break;
      }
    } else {
      dataerrln("Class collator not instantiated");
      name = "";
    }
}

#endif /* #if !UCONFIG_NO_COLLATION */
