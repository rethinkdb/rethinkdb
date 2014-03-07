/*
*******************************************************************************
*
*   Copyright (C) 2001-2010, International Business Machines
*   Corporation and others.  All Rights Reserved.
*
*******************************************************************************
*   file name:  ucol_tok.cpp
*   encoding:   US-ASCII
*   tab size:   8 (not used)
*   indentation:4
*
*   created 02/22/2001
*   created by: Vladimir Weinstein
*
* This module builds a collator based on the rule set.
* 
*/

#ifndef UCOL_BLD_H
#define UCOL_BLD_H

#ifdef UCOL_DEBUG
#include <stdio.h>
#include <stdlib.h>
#endif

#include "unicode/utypes.h"

#if !UCONFIG_NO_COLLATION
/*#if !UCONFIG_NO_COLLATION_BUILDER*/

#include "ucol_imp.h"
#include "ucol_tok.h"
#include "ucol_wgt.h"

U_CFUNC
UCATableHeader *ucol_assembleTailoringTable(UColTokenParser *src, UErrorCode *status);

typedef struct {
  WeightRange ranges[7];
  int32_t noOfRanges;
  uint32_t byteSize; uint32_t start; uint32_t limit;
  int32_t maxCount;
  int32_t count;
  uint32_t current;
  uint32_t fLow; /*forbidden Low */
  uint32_t fHigh; /*forbidden High */
} ucolCEGenerator;

U_CFUNC uint32_t U_EXPORT2 ucol_getCEStrengthDifference(uint32_t CE, uint32_t contCE, 
                                            uint32_t prevCE, uint32_t prevContCE);

U_INTERNAL int32_t U_EXPORT2 ucol_findReorderingEntry(const char* name);

/*#endif*/ /* #if !UCONFIG_NO_COLLATION_BUILDER */
#endif /* #if !UCONFIG_NO_COLLATION */

#endif
