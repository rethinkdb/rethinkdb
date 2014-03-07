/*
*******************************************************************************
*
*   Copyright (C) 2003, International Business Machines
*   Corporation and others.  All Rights Reserved.
*
*******************************************************************************
*   file name:  csorttst.c
*   encoding:   US-ASCII
*   tab size:   8 (not used)
*   indentation:4
*
*   created on: 2003aug04
*   created by: Markus W. Scherer
*
*   Test internal sorting functions.
*/

#include "unicode/utypes.h"
#include "cmemory.h"
#include "cintltst.h"
#include "uarrsort.h"

#define LENGTHOF(array) (int32_t)(sizeof(array)/sizeof((array)[0]))

static void
SortTest(void) {
    uint16_t small[]={ 8, 1, 2, 5, 4, 3, 7, 6 };
    int32_t medium[]={ 10, 8, 1, 2, 5, 5, -1, 6, 4, 3, 9, 7, 5 };
    uint32_t large[]={ 21, 10, 20, 19, 11, 12, 13, 10, 10, 10, 10,
                       8, 1, 2, 5, 10, 10, 4, 17, 18, 3, 9, 10, 7, 6, 14, 15, 16 };

    int32_t i;
    UErrorCode errorCode;

    /* sort small array (stable) */
    errorCode=U_ZERO_ERROR;
    uprv_sortArray(small, LENGTHOF(small), sizeof(small[0]), uprv_uint16Comparator, NULL, TRUE, &errorCode);
    if(U_FAILURE(errorCode)) {
        log_err("uprv_sortArray(small) failed - %s\n", u_errorName(errorCode));
        return;
    }
    for(i=1; i<LENGTHOF(small); ++i) {
        if(small[i-1]>small[i]) {
            log_err("uprv_sortArray(small) mis-sorted [%d]=%u > [%d]=%u\n", i-1, small[i-1], i, small[i]);
            return;
        }
    }

    /* for medium, add bits that will not be compared, to test stability */
    for(i=0; i<LENGTHOF(medium); ++i) {
        medium[i]=(medium[i]<<4)|i;
    }

    /* sort medium array (stable) */
    uprv_sortArray(medium, LENGTHOF(medium), sizeof(medium[0]), uprv_int32Comparator, NULL, TRUE, &errorCode);
    if(U_FAILURE(errorCode)) {
        log_err("uprv_sortArray(medium) failed - %s\n", u_errorName(errorCode));
        return;
    }
    for(i=1; i<LENGTHOF(medium); ++i) {
        if(medium[i-1]>=medium[i]) {
            log_err("uprv_sortArray(medium) mis-sorted [%d]=%u > [%d]=%u\n", i-1, medium[i-1], i, medium[i]);
            return;
        }
    }

    /* sort large array (not stable) */
    errorCode=U_ZERO_ERROR;
    uprv_sortArray(large, LENGTHOF(large), sizeof(large[0]), uprv_uint32Comparator, NULL, FALSE, &errorCode);
    if(U_FAILURE(errorCode)) {
        log_err("uprv_sortArray(large) failed - %s\n", u_errorName(errorCode));
        return;
    }
    for(i=1; i<LENGTHOF(large); ++i) {
        if(large[i-1]>large[i]) {
            log_err("uprv_sortArray(large) mis-sorted [%d]=%u > [%d]=%u\n", i-1, large[i-1], i, large[i]);
            return;
        }
    }
}

void
addSortTest(TestNode** root);

void
addSortTest(TestNode** root) {
    addTest(root, &SortTest, "tsutil/sorttest/SortTest");
}
