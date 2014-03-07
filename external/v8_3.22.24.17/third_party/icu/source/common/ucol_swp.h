/*
*******************************************************************************
*
*   Copyright (C) 2003-2009, International Business Machines
*   Corporation and others.  All Rights Reserved.
*
*******************************************************************************
*   file name:  ucol_swp.h
*   encoding:   US-ASCII
*   tab size:   8 (not used)
*   indentation:4
*
*   created on: 2003sep10
*   created by: Markus W. Scherer
*
*   Swap collation binaries.
*/

#ifndef __UCOL_SWP_H__
#define __UCOL_SWP_H__

#include "unicode/utypes.h"

#if !UCONFIG_NO_COLLATION

#include "udataswp.h"

/*
 * Does the data look like a collation binary?
 * @internal
 */
U_INTERNAL UBool U_EXPORT2
ucol_looksLikeCollationBinary(const UDataSwapper *ds,
                              const void *inData, int32_t length);

/**
 * Swap a header-less collation binary, inside a resource bundle or ucadata.icu.
 * See udataswp.h.
 * @internal
 */
U_CAPI int32_t U_EXPORT2
ucol_swapBinary(const UDataSwapper *ds,
                const void *inData, int32_t length, void *outData,
                UErrorCode *pErrorCode);

/**
 * Swap ICU collation data like ucadata.icu. See udataswp.h.
 * @internal
 */
U_CAPI int32_t U_EXPORT2
ucol_swap(const UDataSwapper *ds,
          const void *inData, int32_t length, void *outData,
          UErrorCode *pErrorCode);

/**
 * Swap inverse UCA collation data (invuca.icu). See udataswp.h.
 * @internal
 */
U_CAPI int32_t U_EXPORT2
ucol_swapInverseUCA(const UDataSwapper *ds,
                    const void *inData, int32_t length, void *outData,
                    UErrorCode *pErrorCode);

#endif /* #if !UCONFIG_NO_COLLATION */

#endif
