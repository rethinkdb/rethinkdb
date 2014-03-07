/*
*******************************************************************************
*
*   Copyright (C) 2003, International Business Machines
*   Corporation and others.  All Rights Reserved.
*
*******************************************************************************
*   file name:  uarrsort.c
*   encoding:   US-ASCII
*   tab size:   8 (not used)
*   indentation:4
*
*   created on: 2003aug04
*   created by: Markus W. Scherer
*
*   Internal function for sorting arrays.
*/

#include "unicode/utypes.h"
#include "cmemory.h"
#include "uarrsort.h"

enum {
    MIN_QSORT=9, /* from Knuth */
    STACK_ITEM_SIZE=200
};

/* UComparator convenience implementations ---------------------------------- */

U_CAPI int32_t U_EXPORT2
uprv_uint16Comparator(const void *context, const void *left, const void *right) {
    return (int32_t)*(const uint16_t *)left - (int32_t)*(const uint16_t *)right;
}

U_CAPI int32_t U_EXPORT2
uprv_int32Comparator(const void *context, const void *left, const void *right) {
    return *(const int32_t *)left - *(const int32_t *)right;
}

U_CAPI int32_t U_EXPORT2
uprv_uint32Comparator(const void *context, const void *left, const void *right) {
    uint32_t l=*(const uint32_t *)left, r=*(const uint32_t *)right;

    /* compare directly because (l-r) would overflow the int32_t result */
    if(l<r) {
        return -1;
    } else if(l==r) {
        return 0;
    } else /* l>r */ {
        return 1;
    }
}

/* Straight insertion sort from Knuth vol. III, pg. 81 ---------------------- */

static void
doInsertionSort(char *array, int32_t start, int32_t limit, int32_t itemSize,
                UComparator *cmp, const void *context, void *pv) {
    int32_t i, j;

    for(j=start+1; j<limit; ++j) {
        /* v=array[j] */
        uprv_memcpy(pv, array+j*itemSize, itemSize);

        for(i=j; i>start; --i) {
            if(/* v>=array[i-1] */ cmp(context, pv, array+(i-1)*itemSize)>=0) {
                break;
            }

            /* array[i]=array[i-1]; */
            uprv_memcpy(array+i*itemSize, array+(i-1)*itemSize, itemSize);
        }

        if(i!=j) {
            /* array[i]=v; */
            uprv_memcpy(array+i*itemSize, pv, itemSize);
        }
    }
}

static void
insertionSort(char *array, int32_t length, int32_t itemSize,
              UComparator *cmp, const void *context, UErrorCode *pErrorCode) {
    UAlignedMemory v[STACK_ITEM_SIZE/sizeof(UAlignedMemory)+1];
    void *pv;

    /* allocate an intermediate item variable (v) */
    if(itemSize<=STACK_ITEM_SIZE) {
        pv=v;
    } else {
        pv=uprv_malloc(itemSize);
        if(pv==NULL) {
            *pErrorCode=U_MEMORY_ALLOCATION_ERROR;
            return;
        }
    }

    doInsertionSort(array, 0, length, itemSize, cmp, context, pv);

    if(pv!=v) {
        uprv_free(pv);
    }
}

/* QuickSort ---------------------------------------------------------------- */

/*
 * This implementation is semi-recursive:
 * It recurses for the smaller sub-array to shorten the recursion depth,
 * and loops for the larger sub-array.
 *
 * Loosely after QuickSort algorithms in
 * Niklaus Wirth
 * Algorithmen und Datenstrukturen mit Modula-2
 * B.G. Teubner Stuttgart
 * 4. Auflage 1986
 * ISBN 3-519-02260-5
 */
static void
subQuickSort(char *array, int32_t start, int32_t limit, int32_t itemSize,
             UComparator *cmp, const void *context,
             void *px, void *pw) {
    int32_t left, right;

    /* start and left are inclusive, limit and right are exclusive */
    do {
        if((start+MIN_QSORT)>=limit) {
            doInsertionSort(array, start, limit, itemSize, cmp, context, px);
            break;
        }

        left=start;
        right=limit;

        /* x=array[middle] */
        uprv_memcpy(px, array+((start+limit)/2)*itemSize, itemSize);

        do {
            while(/* array[left]<x */
                  cmp(context, array+left*itemSize, px)<0
            ) {
                ++left;
            }
            while(/* x<array[right-1] */
                  cmp(context, px, array+(right-1)*itemSize)<0
            ) {
                --right;
            }

            /* swap array[left] and array[right-1] via w; ++left; --right */
            if(left<right) {
                --right;

                if(left<right) {
                    uprv_memcpy(pw, array+left*itemSize, itemSize);
                    uprv_memcpy(array+left*itemSize, array+right*itemSize, itemSize);
                    uprv_memcpy(array+right*itemSize, pw, itemSize);
                }

                ++left;
            }
        } while(left<right);

        /* sort sub-arrays */
        if((right-start)<(limit-left)) {
            /* sort [start..right[ */
            if(start<(right-1)) {
                subQuickSort(array, start, right, itemSize, cmp, context, px, pw);
            }

            /* sort [left..limit[ */
            start=left;
        } else {
            /* sort [left..limit[ */
            if(left<(limit-1)) {
                subQuickSort(array, left, limit, itemSize, cmp, context, px, pw);
            }

            /* sort [start..right[ */
            limit=right;
        }
    } while(start<(limit-1));
}

static void
quickSort(char *array, int32_t length, int32_t itemSize,
            UComparator *cmp, const void *context, UErrorCode *pErrorCode) {
    UAlignedMemory xw[(2*STACK_ITEM_SIZE)/sizeof(UAlignedMemory)+1];
    void *p;

    /* allocate two intermediate item variables (x and w) */
    if(itemSize<=STACK_ITEM_SIZE) {
        p=xw;
    } else {
        p=uprv_malloc(2*itemSize);
        if(p==NULL) {
            *pErrorCode=U_MEMORY_ALLOCATION_ERROR;
            return;
        }
    }

    subQuickSort(array, 0, length, itemSize,
                 cmp, context, p, (char *)p+itemSize);

    if(p!=xw) {
        uprv_free(p);
    }
}

/* uprv_sortArray() API ----------------------------------------------------- */

/*
 * Check arguments, select an appropriate implementation,
 * cast the array to char * so that array+i*itemSize works.
 */
U_CAPI void U_EXPORT2
uprv_sortArray(void *array, int32_t length, int32_t itemSize,
               UComparator *cmp, const void *context,
               UBool sortStable, UErrorCode *pErrorCode) {
    if(pErrorCode==NULL || U_FAILURE(*pErrorCode)) {
        return;
    }
    if((length>0 && array==NULL) || length<0 || itemSize<=0 || cmp==NULL) {
        *pErrorCode=U_ILLEGAL_ARGUMENT_ERROR;
        return;
    }

    if(length<=1) {
        return;
    } else if(length<MIN_QSORT || sortStable) {
        insertionSort((char *)array, length, itemSize, cmp, context, pErrorCode);
        /* could add heapSort or similar for stable sorting of longer arrays */
    } else {
        quickSort((char *)array, length, itemSize, cmp, context, pErrorCode);
    }
}
