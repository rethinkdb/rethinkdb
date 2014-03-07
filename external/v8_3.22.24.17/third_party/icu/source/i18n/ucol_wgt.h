/*  
*******************************************************************************
*
*   Copyright (C) 1999-2008, International Business Machines
*   Corporation and others.  All Rights Reserved.
*
*******************************************************************************
*   file name:  ucol_wgt.h
*   encoding:   US-ASCII
*   tab size:   8 (not used)
*   indentation:4
*
*   created on: 2001mar08
*   created by: Markus W. Scherer
*/

#ifndef UCOL_WGT_H
#define UCOL_WGT_H

#include "unicode/utypes.h"

#if !UCONFIG_NO_COLLATION

/* definitions for CE weights */

typedef struct WeightRange {
    uint32_t start, end;
    int32_t length, count;
    int32_t length2;
    uint32_t count2;
} WeightRange;

/**
 * Determine heuristically
 * what ranges to use for a given number of weights between (excluding)
 * two limits.
 *
 * @param lowerLimit A collation element weight; the ranges will be filled to cover
 *                   weights greater than this one.
 * @param upperLimit A collation element weight; the ranges will be filled to cover
 *                   weights less than this one.
 * @param n          The number of collation element weights w necessary such that
 *                   lowerLimit<w<upperLimit in lexical order.
 * @param maxByte    The highest valid byte value.
 * @param ranges     An array that is filled in with one or more ranges to cover
 *                   n weights between the limits.
 * @return number of ranges, 0 if it is not possible to fit n elements between the limits
 */
U_CFUNC int32_t
ucol_allocWeights(uint32_t lowerLimit, uint32_t upperLimit,
                  uint32_t n,
                  uint32_t maxByte,
                  WeightRange ranges[7]);

/**
 * Given a set of ranges calculated by ucol_allocWeights(),
 * iterate through the weights.
 * The ranges are modified to keep the current iteration state.
 *
 * @param ranges The array of ranges that ucol_allocWeights() filled in.
 *               The ranges are modified.
 * @param pRangeCount The number of ranges. It will be decremented when necessary.
 * @return The next weight in the ranges, or 0xffffffff if there is none left.
 */
U_CFUNC uint32_t
ucol_nextWeight(WeightRange ranges[], int32_t *pRangeCount);

#endif /* #if !UCONFIG_NO_COLLATION */

#endif
