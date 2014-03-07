/*
******************************************************************************
* Copyright (C) 2003, International Business Machines Corporation and        *
* others. All Rights Reserved.                                               *
******************************************************************************
*/

//  file:  v32test.h


#ifndef V32TEST_H
#define V32TEST_H

#include "unicode/utypes.h"

#include "intltest.h"


class UVector32Test: public IntlTest {
public:
  
    UVector32Test();
    virtual ~UVector32Test();

    virtual void runIndexedTest(int32_t index, UBool exec, const char* &name, char* par = NULL );

    // The following are test functions that are visible from the intltest test framework.
    virtual void UVector32_API();

};

#endif
