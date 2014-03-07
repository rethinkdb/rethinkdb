/********************************************************************
 * COPYRIGHT: 
 * Copyright (c) 1997-2005, International Business Machines Corporation and
 * others. All Rights Reserved.
 ********************************************************************/
/********************************************************************************
*
* File CINTLTST.H
*
*     Madhu Katragadda               Creation
* Modification History:
*   Date        Name        Description            
*   07/13/99    helena      HPUX 11 CC port.
*********************************************************************************

The main root for C API tests
*/

#ifndef _CINTLTST
#define _CINTLTST

#include "unicode/utypes.h"
#include "unicode/putil.h"
#include "unicode/ctest.h"
#include <stdlib.h>

#ifndef U_USE_DEPRECATED_API
#define U_USE_DEPRECATED_API 1
#endif

U_CFUNC void addAllTests(TestNode** root);

/**
 * Return the path to the icu/source/data/out  directory 
 */
U_CFUNC const char* ctest_dataOutDir(void);

/**
 * Return the path to the icu/source/data/  directory 
 * for out of source builds too returns the source directory
 */
U_CFUNC const char* ctest_dataSrcDir(void);

U_CFUNC UChar* CharsToUChars(const char* chars);

/**
 * Convert a const UChar* into a char*
 * Caller owns storage, but in practice this function
 * LEAKS so be aware of that.
 * @param unichars UChars (null terminated) to be converted
 * @return new char* to the unichars in host format
 */
 
U_CFUNC char *austrdup(const UChar* unichars);
U_CFUNC char *aescstrdup(const UChar* unichars, int32_t length);
U_CFUNC void *ctst_malloc(size_t size);
U_CFUNC void ctst_freeAll(void);

U_CFUNC const char* loadTestData(UErrorCode* err);

/**
 * function used to specify the error
 * converts the errorcode to an error descriptive string(const char*)
 * @param status the error code
 */
#define myErrorName(errorCode) u_errorName(errorCode)


/**
 * Call this once to get a consistent timezone. Use ctest_resetTimeZone to set it back to the original value.
 * @param optionalTimeZone Set this to a requested timezone.
 *      Set to NULL to use the standard test timezone (Pacific Time)
 */
U_CFUNC void ctest_setTimeZone(const char *optionalTimeZone, UErrorCode *status);
/**
 * Call this once get back the original timezone
 */
U_CFUNC void ctest_resetTimeZone(void);

/**
 * Call this once get ICU back to its original state with test arguments.
 * This function calls u_cleanup.
 */
U_CFUNC UBool ctest_resetICU(void);

/**
 * Assert that the given UErrorCode succeeds, and return TRUE if it does.
 */
U_CFUNC UBool assertSuccess(const char* msg, UErrorCode* ec);

/**
 * Assert that the UBool is TRUE, and return TRUE if it does.
 *
 * NOTE: Use 'int condition' rather than 'UBool condition' so the
 * compiler doesn't complain about integral conversion of expressions
 * like 'p != 0'.
 */
U_CFUNC UBool assertTrue(const char* msg, int condition);

/**
 * Assert that the actualString equals the expectedString, and return
 * TRUE if it does.
 */
U_CFUNC UBool assertEquals(const char* msg, const char* expectedString,
                           const char* actualString);

/**
 * Time bomb - allows temporary behavior that expires at a given
 *             release
 *
 */
U_CFUNC UBool isICUVersionAtLeast(const UVersionInfo x);

#endif
