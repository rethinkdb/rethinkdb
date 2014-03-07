/********************************************************************
 * COPYRIGHT:
 * Copyright (c) 2004-2010, International Business Machines Corporation and
 * others. All Rights Reserved.
 ********************************************************************/

//      Test parts of UVector and UStack

#include "intltest.h"

#include "uvectest.h"
#include "uvector.h"
#include "hash.h"

#include "cstring.h"

//---------------------------------------------------------------------------
//
//  Test class boilerplate
//
//---------------------------------------------------------------------------
UVectorTest::UVectorTest() 
{
}


UVectorTest::~UVectorTest()
{
}



void UVectorTest::runIndexedTest( int32_t index, UBool exec, const char* &name, char* /*par*/ )
{
    if (exec) logln("TestSuite UVectorTest: ");
    switch (index) {

        case 0: name = "UVector_API";
            if (exec) UVector_API();
            break;
        case 1: name = "UStack_API";
            if (exec) UStack_API();
            break;
        case 2: name = "Hashtable_API";
            if (exec) Hashtable_API();
            break;
        default: name = "";
            break; //needed to end loop
    }
}


//---------------------------------------------------------------------------
//
//   Error Checking / Reporting macros used in all of the tests.
//
//---------------------------------------------------------------------------
#define TEST_CHECK_STATUS(status) \
    if (U_FAILURE(status)) {\
        errln("UVectorTest failure at line %d.  status=%s\n", __LINE__, u_errorName(status));\
        return;\
    }

#define TEST_ASSERT(expr) \
    if ((expr)==FALSE) {\
        errln("UVectorTest failure at line %d.\n", __LINE__);\
    }

static int8_t U_CALLCONV
UVectorTest_compareInt32(UHashTok key1, UHashTok key2) {
    if (key1.integer > key2.integer) {
        return 1;
    }
    else if (key1.integer < key2.integer) {
        return -1;
    }
    return 0;
}

U_CDECL_BEGIN
static int8_t U_CALLCONV
UVectorTest_compareCstrings(const UHashTok key1, const UHashTok key2) {
    return !strcmp((const char *)key1.pointer, (const char *)key2.pointer);
}
U_CDECL_END

//---------------------------------------------------------------------------
//
//      UVector_API      Check for basic functionality of UVector.
//
//---------------------------------------------------------------------------
void UVectorTest::UVector_API() {

    UErrorCode  status = U_ZERO_ERROR;
    UVector     *a;

    a = new UVector(status);
    TEST_CHECK_STATUS(status);
    delete a;

    status = U_ZERO_ERROR;
    a = new UVector(2000, status);
    TEST_CHECK_STATUS(status);
    delete a;

    status = U_ZERO_ERROR;
    a = new UVector(status);
    a->sortedInsert((int32_t)10, UVectorTest_compareInt32, status);
    a->sortedInsert((int32_t)20, UVectorTest_compareInt32, status);
    a->sortedInsert((int32_t)30, UVectorTest_compareInt32, status);
    a->sortedInsert((int32_t)15, UVectorTest_compareInt32, status);
    TEST_CHECK_STATUS(status);
    TEST_ASSERT(a->elementAti(0) == 10);
    TEST_ASSERT(a->elementAti(1) == 15);
    TEST_ASSERT(a->elementAti(2) == 20);
    TEST_ASSERT(a->elementAti(3) == 30);
    TEST_ASSERT(a->indexOf((int32_t)3) == -1);
    TEST_ASSERT(a->indexOf((int32_t)15) == 1);
    TEST_ASSERT(a->indexOf((int32_t)15, 2) == -1);
    TEST_ASSERT(a->contains((int32_t)15));
    TEST_ASSERT(!a->contains((int32_t)5));
    delete a;
}

void UVectorTest::UStack_API() {
    UErrorCode  status = U_ZERO_ERROR;
    UStack     *a;

    a = new UStack(status);
    TEST_CHECK_STATUS(status);
    delete a;

    status = U_ZERO_ERROR;
    a = new UStack(2000, status);
    TEST_CHECK_STATUS(status);
    delete a;

    status = U_ZERO_ERROR;
    a = new UStack(NULL, NULL, 2000, status);
    TEST_CHECK_STATUS(status);
    delete a;

    status = U_ZERO_ERROR;
    a = new UStack(NULL, UVectorTest_compareCstrings, status);
    TEST_ASSERT(a->empty());
    a->push((void*)"abc", status);
    TEST_ASSERT(!a->empty());
    a->push((void*)"bcde", status);
    a->push((void*)"cde", status);
    TEST_CHECK_STATUS(status);
    TEST_ASSERT(strcmp("cde", (const char *)a->peek()) == 0);
    TEST_ASSERT(a->search((void*)"cde") == 1);
    TEST_ASSERT(a->search((void*)"bcde") == 2);
    TEST_ASSERT(a->search((void*)"abc") == 3);
    TEST_ASSERT(strcmp("abc", (const char *)a->firstElement()) == 0);
    TEST_ASSERT(strcmp("cde", (const char *)a->lastElement()) == 0);
    TEST_ASSERT(strcmp("cde", (const char *)a->pop()) == 0);
    TEST_ASSERT(strcmp("bcde", (const char *)a->pop()) == 0);
    TEST_ASSERT(strcmp("abc", (const char *)a->pop()) == 0);
    delete a;
}

U_CDECL_BEGIN
static UBool U_CALLCONV neverTRUE(const UHashTok /*key1*/, const UHashTok /*key2*/) {
    return FALSE;
}

U_CDECL_END

void UVectorTest::Hashtable_API() {
    UErrorCode status = U_ZERO_ERROR;
    Hashtable *a = new Hashtable(status);
    TEST_ASSERT((a->puti("a", 1, status) == 0));
    TEST_ASSERT((a->find("a") != NULL));
    TEST_ASSERT((a->find("b") == NULL));
    TEST_ASSERT((a->puti("b", 2, status) == 0));
    TEST_ASSERT((a->find("b") != NULL));
    TEST_ASSERT((a->removei("a") == 1));
    TEST_ASSERT((a->find("a") == NULL));

    /* verify that setValueComparator works */
    Hashtable b(status);
    TEST_ASSERT((!a->equals(b)));
    TEST_ASSERT((b.puti("b", 2, status) == 0));
    TEST_ASSERT((!a->equals(b))); // Without a value comparator, this will be FALSE by default.
    b.setValueComparator(uhash_compareLong);
    TEST_ASSERT((!a->equals(b)));
    a->setValueComparator(uhash_compareLong);
    TEST_ASSERT((a->equals(b)));
    TEST_ASSERT((a->equals(*a))); // This better be reflexive.

    /* verify that setKeyComparator works */
    TEST_ASSERT((a->puti("a", 1, status) == 0));
    TEST_ASSERT((a->find("a") != NULL));
    a->setKeyComparator(neverTRUE);
    TEST_ASSERT((a->find("a") == NULL));

    delete a;
}

