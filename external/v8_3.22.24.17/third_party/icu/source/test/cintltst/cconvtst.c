/********************************************************************
 * COPYRIGHT: 
 * Copyright (c) 1997-2009, International Business Machines Corporation and
 * others. All Rights Reserved.
 ********************************************************************/
/********************************************************************************
*
* File CCONVTST.C
*
* Modification History:
*        Name                     Description            
*     Madhu Katragadda               Creation
*********************************************************************************
*/
#include "cintltst.h"

void addTestConvert(TestNode**);
#include "nucnvtst.h"
void addTestConvertErrorCallBack(TestNode** root);
void addTestEuroRegression(TestNode** root);
void addTestConverterFallBack(TestNode** root);
void addExtraTests(TestNode** root);

/* bocu1tst.c */
U_CFUNC void
addBOCU1Tests(TestNode** root);

void addConvert(TestNode** root);

void addConvert(TestNode** root)
{
    addTestConvert(root);
    addTestNewConvert(root);
    addBOCU1Tests(root);
    addTestConvertErrorCallBack(root);
    addTestEuroRegression(root);
    addTestConverterFallBack(root);
    addExtraTests(root);
}
