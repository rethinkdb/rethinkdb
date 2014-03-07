/********************************************************************
 * COPYRIGHT: 
 * Copyright (c) 1997-2003, International Business Machines Corporation and
 * others. All Rights Reserved.
 ********************************************************************/

#ifndef RESOURCEBUNDLETEST_H
#define RESOURCEBUNDLETEST_H

#include "intltest.h"

/**
 * Tests for class ResourceBundle
 **/
class ResourceBundleTest: public IntlTest {
public:
    ResourceBundleTest();
    virtual ~ResourceBundleTest();
    
    void runIndexedTest( int32_t index, UBool exec, const char* &name, char* par = NULL );

    /** 
     * Perform several extensive tests using the subtest routine testTag
     **/
    void TestResourceBundles(void);
    /** 
     * Test construction of ResourceBundle accessing a custom test resource-file
     **/
    void TestConstruction(void);

    void TestExemplar(void);

    void TestGetSize(void);
    void TestGetLocaleByType(void);

private:
    /**
     * The assignment operator has no real implementation.
     * It is provided to make the compiler happy. Do not call.
     */
    ResourceBundleTest& operator=(const ResourceBundleTest&) { return *this; }

    /**
     * extensive subtests called by TestResourceBundles
     **/
    UBool testTag(const char* frag, UBool in_Root, UBool in_te, UBool in_te_IN);

    void record_pass(UnicodeString passMessage);
    void record_fail(UnicodeString errMessage);

    int32_t pass;
    int32_t fail;
};

#endif
