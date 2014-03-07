/********************************************************************
 * COPYRIGHT:
 * Copyright (c) 2002-2007, International Business Machines Corporation and
 * others. All Rights Reserved.
 ********************************************************************/

//
//   regextst.cpp
//
//      ICU Regular Expressions test, part of intltest.
//

#include "intltest.h"

#include "v32test.h"
#include "uvectr32.h"
#include "uvector.h"
#include "util.h"
#include <stdlib.h>
#include <stdio.h>


//---------------------------------------------------------------------------
//
//  Test class boilerplate
//
//---------------------------------------------------------------------------
UVector32Test::UVector32Test() 
{
}


UVector32Test::~UVector32Test()
{
}



void UVector32Test::runIndexedTest( int32_t index, UBool exec, const char* &name, char* /*par*/ )
{
    if (exec) logln("TestSuite UVector32Test: ");
    switch (index) {

        case 0: name = "UVector32_API";
            if (exec) UVector32_API(); 
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
        errln("UVector32Test failure at line %d.  status=%s\n", __LINE__, u_errorName(status));\
        return;\
    }

#define TEST_ASSERT(expr) \
    if ((expr)==FALSE) {\
        errln("UVector32Test failure at line %d.\n", __LINE__);\
    }

//---------------------------------------------------------------------------
//
//      UVector32_API      Check for basic functionality of UVector32.
//
//---------------------------------------------------------------------------
void UVector32Test::UVector32_API() {

    UErrorCode  status = U_ZERO_ERROR;
    UVector32     *a;
    UVector32     *b;

    a = new UVector32(status);
    TEST_CHECK_STATUS(status);
    delete a;

    status = U_ZERO_ERROR;
    a = new UVector32(2000, status);
    TEST_CHECK_STATUS(status);
    delete a;

    //
    //  assign()
    //
    status = U_ZERO_ERROR;
    a = new UVector32(status);
    a->addElement(10, status);
    a->addElement(20, status);
    a->addElement(30, status);
    b = new UVector32(status);
    b->assign(*a, status);
    TEST_ASSERT(b->size() == 3);
    TEST_ASSERT(b->elementAti(1) == 20);
    TEST_CHECK_STATUS(status);
    delete a;
    delete b;

    //
    //  operator == and != and equals()
    //
    status = U_ZERO_ERROR;
    a = new UVector32(status);
    a->addElement(10, status);
    a->addElement(20, status);
    a->addElement(30, status);
    b = new UVector32(status);
    TEST_ASSERT(*b != *a);
    TEST_ASSERT(!(*b == *a));
    TEST_ASSERT(!b->equals(*a));
    b->assign(*a, status);
    TEST_ASSERT(*b == *a);
    TEST_ASSERT(!(*b != *a));
    TEST_ASSERT(b->equals(*a));
    b->addElement(666, status);
    TEST_ASSERT(*b != *a);
    TEST_ASSERT(!(*b == *a));
    TEST_ASSERT(!b->equals(*a));
    TEST_CHECK_STATUS(status);
    delete b;
    delete a;

    //
    //  addElement().   Covered by above tests.
    //

    //
    // setElementAt()
    //
    status = U_ZERO_ERROR;
    a = new UVector32(status);
    a->addElement(10, status);
    a->addElement(20, status);
    a->addElement(30, status);
    a->setElementAt(666, 1);
    TEST_ASSERT(a->elementAti(0) == 10);
    TEST_ASSERT(a->elementAti(1) == 666);
    TEST_ASSERT(a->size() == 3);
    TEST_CHECK_STATUS(status);
    delete a;

    //
    // insertElementAt()
    //
    status = U_ZERO_ERROR;
    a = new UVector32(status);
    a->addElement(10, status);
    a->addElement(20, status);
    a->addElement(30, status);
    a->insertElementAt(666, 1, status);
    TEST_ASSERT(a->elementAti(0) == 10);
    TEST_ASSERT(a->elementAti(1) == 666);
    TEST_ASSERT(a->elementAti(2) == 20);
    TEST_ASSERT(a->elementAti(3) == 30);
    TEST_ASSERT(a->size() == 4);
    TEST_CHECK_STATUS(status);
    delete a;

    //
    //  elementAti()    covered by above tests
    //

    //
    //  lastElementi
    //
    status = U_ZERO_ERROR;
    a = new UVector32(status);
    a->addElement(10, status);
    a->addElement(20, status);
    a->addElement(30, status);
    TEST_ASSERT(a->lastElementi() == 30);
    TEST_CHECK_STATUS(status);
    delete a;


    //
    //  indexOf
    //
    status = U_ZERO_ERROR;
    a = new UVector32(status);
    a->addElement(10, status);
    a->addElement(20, status);
    a->addElement(30, status);
    TEST_ASSERT(a->indexOf(30, 0) == 2);
    TEST_ASSERT(a->indexOf(40, 0) == -1);
    TEST_ASSERT(a->indexOf(10, 0) == 0);
    TEST_ASSERT(a->indexOf(10, 1) == -1);
    TEST_CHECK_STATUS(status);
    delete a;

    
    //
    //  contains
    //
    status = U_ZERO_ERROR;
    a = new UVector32(status);
    a->addElement(10, status);
    a->addElement(20, status);
    a->addElement(30, status);
    TEST_ASSERT(a->contains(10) == TRUE);
    TEST_ASSERT(a->contains(11) == FALSE);
    TEST_ASSERT(a->contains(20) == TRUE);
    TEST_ASSERT(a->contains(-10) == FALSE);
    TEST_CHECK_STATUS(status);
    delete a;


    //
    //  containsAll
    //
    status = U_ZERO_ERROR;
    a = new UVector32(status);
    a->addElement(10, status);
    a->addElement(20, status);
    a->addElement(30, status);
    b = new UVector32(status);
    TEST_ASSERT(a->containsAll(*b) == TRUE);
    b->addElement(2, status);
    TEST_ASSERT(a->containsAll(*b) == FALSE);
    b->setElementAt(10, 0);
    TEST_ASSERT(a->containsAll(*b) == TRUE);
    TEST_ASSERT(b->containsAll(*a) == FALSE);
    b->addElement(30, status);
    b->addElement(20, status);
    TEST_ASSERT(a->containsAll(*b) == TRUE);
    TEST_ASSERT(b->containsAll(*a) == TRUE);
    b->addElement(2, status);
    TEST_ASSERT(a->containsAll(*b) == FALSE);
    TEST_ASSERT(b->containsAll(*a) == TRUE);
    TEST_CHECK_STATUS(status);
    delete a;
    delete b;

    //
    //  removeAll
    //
    status = U_ZERO_ERROR;
    a = new UVector32(status);
    a->addElement(10, status);
    a->addElement(20, status);
    a->addElement(30, status);
    b = new UVector32(status);
    a->removeAll(*b);
    TEST_ASSERT(a->size() == 3);
    b->addElement(20, status);
    a->removeAll(*b);
    TEST_ASSERT(a->size() == 2);
    TEST_ASSERT(a->contains(10)==TRUE);
    TEST_ASSERT(a->contains(30)==TRUE);
    b->addElement(10, status);
    a->removeAll(*b);
    TEST_ASSERT(a->size() == 1);
    TEST_ASSERT(a->contains(30) == TRUE);
    TEST_CHECK_STATUS(status);
    delete a;
    delete b;

    //
    // retainAll
    //
    status = U_ZERO_ERROR;
    a = new UVector32(status);
    a->addElement(10, status);
    a->addElement(20, status);
    a->addElement(30, status);
    b = new UVector32(status);
    b->addElement(10, status);
    b->addElement(20, status);
    b->addElement(30, status);
    b->addElement(15, status);
    a->retainAll(*b);
    TEST_ASSERT(a->size() == 3);
    b->removeElementAt(1);
    a->retainAll(*b);
    TEST_ASSERT(a->contains(20) == FALSE);
    TEST_ASSERT(a->size() == 2);
    b->removeAllElements();
    TEST_ASSERT(b->size() == 0);
    a->retainAll(*b);
    TEST_ASSERT(a->size() == 0);
    TEST_CHECK_STATUS(status);
    delete a;
    delete b;

    //
    //  removeElementAt   Tested above.
    //

    //
    //  removeAllElments   Tested above
    //

    //
    //  size()   tested above
    //

    //
    //  isEmpty
    //
    status = U_ZERO_ERROR;
    a = new UVector32(status);
    TEST_ASSERT(a->isEmpty() == TRUE);
    a->addElement(10, status);
    TEST_ASSERT(a->isEmpty() == FALSE);
    a->addElement(20, status);
    a->removeElementAt(0);
    TEST_ASSERT(a->isEmpty() == FALSE);
    a->removeElementAt(0);
    TEST_ASSERT(a->isEmpty() == TRUE);
    TEST_CHECK_STATUS(status);
    delete a;


    //
    // ensureCapacity, expandCapacity
    //
    status = U_ZERO_ERROR;
    a = new UVector32(status);
    TEST_ASSERT(a->isEmpty() == TRUE);
    a->addElement(10, status);
    TEST_ASSERT(a->ensureCapacity(5000, status)== TRUE);
    TEST_ASSERT(a->expandCapacity(20000, status) == TRUE);
    TEST_CHECK_STATUS(status);
    delete a;
    
    //
    // setSize
    //
    status = U_ZERO_ERROR;
    a = new UVector32(status);
    a->addElement(10, status);
    a->addElement(20, status);
    a->addElement(30, status);
    a->setSize(100);
    TEST_ASSERT(a->size() == 100);
    TEST_ASSERT(a->elementAti(0) == 10);
    TEST_ASSERT(a->elementAti(1) == 20);
    TEST_ASSERT(a->elementAti(2) == 30);
    TEST_ASSERT(a->elementAti(3) == 0);
    a->setElementAt(666, 99);
    a->setElementAt(777, 100);
    TEST_ASSERT(a->elementAti(99) == 666);
    TEST_ASSERT(a->elementAti(100) == 0);
    a->setSize(2);
    TEST_ASSERT(a->elementAti(1) == 20);
    TEST_ASSERT(a->elementAti(2) == 0);
    TEST_ASSERT(a->size() == 2);
    a->setSize(0);
    TEST_ASSERT(a->empty() == TRUE);
    TEST_ASSERT(a->size() == 0);

    TEST_CHECK_STATUS(status);
    delete a;

    //
    // containsNone
    //
    status = U_ZERO_ERROR;
    a = new UVector32(status);
    a->addElement(10, status);
    a->addElement(20, status);
    a->addElement(30, status);
    b = new UVector32(status);
    TEST_ASSERT(a->containsNone(*b) == TRUE);
    b->addElement(5, status);
    TEST_ASSERT(a->containsNone(*b) == TRUE);
    b->addElement(30, status);
    TEST_ASSERT(a->containsNone(*b) == FALSE);

    TEST_CHECK_STATUS(status);
    delete a;
    delete b;

    //
    // sortedInsert
    //
    status = U_ZERO_ERROR;
    a = new UVector32(status);
    a->sortedInsert(30, status);
    a->sortedInsert(20, status);
    a->sortedInsert(10, status);
    TEST_ASSERT(a->elementAti(0) == 10);
    TEST_ASSERT(a->elementAti(1) == 20);
    TEST_ASSERT(a->elementAti(2) == 30);

    TEST_CHECK_STATUS(status);
    delete a;

    //
    // getBuffer
    //
    status = U_ZERO_ERROR;
    a = new UVector32(status);
    a->addElement(10, status);
    a->addElement(20, status);
    int32_t *buf = a->getBuffer();
    TEST_ASSERT(buf[0] == 10);
    TEST_ASSERT(buf[1] == 20);
    a->setSize(20000);
    int32_t *resizedBuf;
    resizedBuf = a->getBuffer();
    //TEST_ASSERT(buf != resizedBuf); // The buffer might have been realloc'd
    TEST_ASSERT(resizedBuf[0] == 10);
    TEST_ASSERT(resizedBuf[1] == 20);

    TEST_CHECK_STATUS(status);
    delete a;


    //
    //  empty
    //
    status = U_ZERO_ERROR;
    a = new UVector32(status);
    TEST_ASSERT(a->empty() == TRUE);
    a->addElement(10, status);
    TEST_ASSERT(a->empty() == FALSE);
    a->addElement(20, status);
    a->removeElementAt(0);
    TEST_ASSERT(a->empty() == FALSE);
    a->removeElementAt(0);
    TEST_ASSERT(a->empty() == TRUE);
    TEST_CHECK_STATUS(status);
    delete a;


    //
    //  peeki
    //
    status = U_ZERO_ERROR;
    a = new UVector32(status);
    a->addElement(10, status);
    TEST_ASSERT(a->peeki() == 10);
    a->addElement(20, status);
    TEST_ASSERT(a->peeki() == 20);
    a->addElement(30, status);
    TEST_ASSERT(a->peeki() == 30);
    TEST_CHECK_STATUS(status);
    delete a;


    //
    // popi
    //
    status = U_ZERO_ERROR;
    a = new UVector32(status);
    a->addElement(10, status);
    a->addElement(20, status);
    a->addElement(30, status);
    TEST_ASSERT(a->popi() == 30);
    TEST_ASSERT(a->popi() == 20);
    TEST_ASSERT(a->popi() == 10);
    TEST_ASSERT(a->popi() == 0);
    TEST_ASSERT(a->isEmpty());
    TEST_CHECK_STATUS(status);
    delete a;

    //
    // push
    //
    status = U_ZERO_ERROR;
    a = new UVector32(status);
    TEST_ASSERT(a->push(10, status) == 10);
    TEST_ASSERT(a->push(20, status) == 20);
    TEST_ASSERT(a->push(30, status) == 30);
    TEST_ASSERT(a->size() == 3);
    TEST_ASSERT(a->popi() == 30);
    TEST_ASSERT(a->popi() == 20);
    TEST_ASSERT(a->popi() == 10);
    TEST_ASSERT(a->isEmpty());
    TEST_CHECK_STATUS(status);
    delete a;


    //
    // reserveBlock
    //
    status = U_ZERO_ERROR;
    a = new UVector32(status);
    a->ensureCapacity(1000, status);

    // TODO:

    TEST_CHECK_STATUS(status);
    delete a;

}


