/*
******************************************************************************
*
*   Copyright (C) 2002-2003, International Business Machines
*   Corporation and others.  All Rights Reserved.
*
******************************************************************************
*
* File cmemory.c      ICU Heap allocation.
*                     All ICU heap allocation, both for C and C++ new of ICU
*                     class types, comes through these functions.
*
*                     If you have a need to replace ICU allocation, this is the
*                     place to do it.
*
*                     Note that uprv_malloc(0) returns a non-NULL pointer, and
*                     that a subsequent free of that pointer value is a NOP.
*
******************************************************************************
*/
#include "unicode/uclean.h"
#include "cmemory.h"
#include <stdlib.h>

/* uprv_malloc(0) returns a pointer to this read-only data. */                
static const int32_t zeroMem[] = {0, 0, 0, 0, 0, 0};

/* Function Pointers for user-supplied heap functions  */
static const void     *pContext;
static UMemAllocFn    *pAlloc;
static UMemReallocFn  *pRealloc;
static UMemFreeFn     *pFree;

/* Flag indicating whether any heap allocations have happened.
 *   Used to prevent changing out the heap functions after allocations have been made */
static UBool   gHeapInUse;

U_CAPI void * U_EXPORT2
uprv_malloc(size_t s) {
    if (s > 0) {
        gHeapInUse = TRUE;
        if (pAlloc) {
            return (*pAlloc)(pContext, s);
        } else {
            return malloc(s);
        }
    } else {
        return (void *)zeroMem;
    }
}

U_CAPI void * U_EXPORT2
uprv_realloc(void * buffer, size_t size) {
    if (buffer == zeroMem) {
        return uprv_malloc(size);
    } else if (size == 0) {
        if (pFree) {
            (*pFree)(pContext, buffer);
        } else {
            free(buffer);
        }
        return (void *)zeroMem;
    } else {
        gHeapInUse = TRUE;
        if (pRealloc) {
            return (*pRealloc)(pContext, buffer, size);
        } else {
            return realloc(buffer, size);
        }
    }
}

U_CAPI void U_EXPORT2
uprv_free(void *buffer) {
    if (buffer != zeroMem) {
        if (pFree) {
            (*pFree)(pContext, buffer);
        } else {
            free(buffer);
        }
    }
}

U_CAPI void U_EXPORT2
u_setMemoryFunctions(const void *context, UMemAllocFn *a, UMemReallocFn *r, UMemFreeFn *f,  UErrorCode *status)
{
    if (U_FAILURE(*status)) {
        return;
    }
    if (a==NULL || r==NULL || f==NULL) {
        *status = U_ILLEGAL_ARGUMENT_ERROR;
        return;
    }
    if (gHeapInUse) {
        *status = U_INVALID_STATE_ERROR;
        return;
    }
    pContext  = context;
    pAlloc    = a;
    pRealloc  = r;
    pFree     = f;
}


U_CFUNC UBool cmemory_cleanup(void) {
    pContext   = NULL;
    pAlloc     = NULL;
    pRealloc   = NULL;
    pFree      = NULL;
    gHeapInUse = FALSE;
    return TRUE;
}


/*
 *   gHeapInUse
 *       Return True if ICU has allocated any memory.
 *       Used by u_SetMutexFunctions() and similar to verify that ICU has not
 *               been used, that it is in a pristine initial state.
 */
U_CFUNC UBool cmemory_inUse() {
    return gHeapInUse;
}

