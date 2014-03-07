/********************************************************************
 * COPYRIGHT: 
 * Copyright (c) 1996-2010, International Business Machines Corporation and
 * others. All Rights Reserved.
 ********************************************************************/
/********************************************************************************
*
* File CALLTEST.C
*
* Modification History:
*   Creation:   Madhu Katragadda 
*********************************************************************************
*/
/* THE FILE WHERE ALL C API TESTS ARE ADDED TO THE ROOT */


#include "cintltst.h"

void addUtility(TestNode** root);
void addBreakIter(TestNode** root);
void addStandardNamesTest(TestNode **root);
void addFormatTest(TestNode** root);
void addConvert(TestNode** root);
void addCollTest(TestNode** root);
void addComplexTest(TestNode** root);
void addUDataTest(TestNode** root);
void addUTF16Test(TestNode** root);
void addUTF8Test(TestNode** root);
void addUTransTest(TestNode** root);
void addPUtilTest(TestNode** root);
void addUCharTransformTest(TestNode** root);
void addUSetTest(TestNode** root);
void addUStringPrepTest(TestNode** root);
void addIDNATest(TestNode** root);
void addHeapMutexTest(TestNode **root);
void addUTraceTest(TestNode** root);
void addURegexTest(TestNode** root);
void addUTextTest(TestNode** root);
void addUCsdetTest(TestNode** root);
void addCnvSelTest(TestNode** root);
void addUSpoofTest(TestNode** root);

void addAllTests(TestNode** root)
{
    addCnvSelTest(root);
    addUDataTest(root);
    addHeapMutexTest(root);
    addUTF16Test(root);
    addUTF8Test(root);
    addUtility(root);
    addUTraceTest(root);
    addUTextTest(root);
    addConvert(root);
    addUCharTransformTest(root);
    addStandardNamesTest(root);
    addUCsdetTest(root);
    addComplexTest(root);
    addUSetTest(root);
#if !UCONFIG_NO_IDNA
    addUStringPrepTest(root);
    addIDNATest(root);
#endif
#if !UCONFIG_NO_REGULAR_EXPRESSIONS
    addURegexTest(root);
#endif
#if !UCONFIG_NO_BREAK_ITERATION
    addBreakIter(root);
#endif
#if !UCONFIG_NO_FORMATTING
    addFormatTest(root);
#endif
#if !UCONFIG_NO_COLLATION
    addCollTest(root);
#endif
#if !UCONFIG_NO_TRANSLITERATION
    addUTransTest(root);
#endif
#if !UCONFIG_NO_REGULAR_EXPRESSIONS && !UCONFIG_NO_NORMALIZATION
    addUSpoofTest(root);
#endif
    addPUtilTest(root);
}

