/*
******************************************************************************
*
*   Copyright (C) 1997-2009, International Business Machines
*   Corporation and others.  All Rights Reserved.
*
******************************************************************************
*
*  FILE NAME : putil.h
*
*   Date        Name        Description
*   05/14/98    nos         Creation (content moved here from utypes.h).
*   06/17/99    erm         Added IEEE_754
*   07/22/98    stephen     Added IEEEremainder, max, min, trunc
*   08/13/98    stephen     Added isNegativeInfinity, isPositiveInfinity
*   08/24/98    stephen     Added longBitsFromDouble
*   03/02/99    stephen     Removed openFile().  Added AS400 support.
*   04/15/99    stephen     Converted to C
*   11/15/99    helena      Integrated S/390 changes for IEEE support.
*   01/11/00    helena      Added u_getVersion.
******************************************************************************
*/

#ifndef PUTIL_H
#define PUTIL_H

#include "unicode/utypes.h"
 /**
  * \file
  * \brief C API: Platform Utilities
  */

/** Define this to 1 if your platform supports IEEE 754 floating point,
   to 0 if it does not. */
#ifndef IEEE_754
#   define IEEE_754 1
#endif

/*==========================================================================*/
/* Platform utilities                                                       */
/*==========================================================================*/

/**
 * Platform utilities isolates the platform dependencies of the
 * libarary.  For each platform which this code is ported to, these
 * functions may have to be re-implemented.
 */

/**
 * Return the ICU data directory. 
 * The data directory is where common format ICU data files (.dat files)
 *   are loaded from.  Note that normal use of the built-in ICU
 *   facilities does not require loading of an external data file;
 *   unless you are adding custom data to ICU, the data directory
 *   does not need to be set.
 *
 * The data directory is determined as follows:
 *    If u_setDataDirectory() has been called, that is it, otherwise
 *    if the ICU_DATA environment variable is set, use that, otherwise
 *    If a data directory was specifed at ICU build time
 *      <code>
 * \code
 *        #define ICU_DATA_DIR "path" 
 * \endcode
 * </code> use that,
 *    otherwise no data directory is available.
 *
 * @return the data directory, or an empty string ("") if no data directory has
 *         been specified.
 *   
 * @stable ICU 2.0
 */
U_STABLE const char* U_EXPORT2 u_getDataDirectory(void);

/** 
 * Set the ICU data directory. 
 * The data directory is where common format ICU data files (.dat files)
 *   are loaded from.  Note that normal use of the built-in ICU
 *   facilities does not require loading of an external data file;
 *   unless you are adding custom data to ICU, the data directory
 *   does not need to be set.
 *
 * This function should be called at most once in a process, before the
 * first ICU operation (e.g., u_init()) that will require the loading of an
 * ICU data file.
 * This function is not thread-safe. Use it before calling ICU APIs from
 * multiple threads.
 *
 * @param directory The directory to be set.
 *
 * @see u_init
 * @stable ICU 2.0
 */
U_STABLE void U_EXPORT2 u_setDataDirectory(const char *directory);

#if !U_CHARSET_IS_UTF8
/**
 * Please use ucnv_getDefaultName() instead.
 * Return the default codepage for this platform and locale.
 * This function can call setlocale() on Unix platforms. Please read the
 * platform documentation on setlocale() before calling this function.
 * @return the default codepage for this platform 
 * @internal
 */
U_INTERNAL const char*  U_EXPORT2 uprv_getDefaultCodepage(void);
#endif

/**
 * Please use uloc_getDefault() instead.
 * Return the default locale ID string by querying ths system, or
 *     zero if one cannot be found. 
 * This function can call setlocale() on Unix platforms. Please read the
 * platform documentation on setlocale() before calling this function.
 * @return the default locale ID string
 * @internal
 */
U_INTERNAL const char*  U_EXPORT2 uprv_getDefaultLocaleID(void);

/**
 * @{
 * Filesystem file and path separator characters.
 * Example: '/' and ':' on Unix, '\\' and ';' on Windows.
 * @stable ICU 2.0
 */
#ifdef XP_MAC
#   define U_FILE_SEP_CHAR ':'
#   define U_FILE_ALT_SEP_CHAR ':'
#   define U_PATH_SEP_CHAR ';'
#   define U_FILE_SEP_STRING ":"
#   define U_FILE_ALT_SEP_STRING ":"
#   define U_PATH_SEP_STRING ";"
#elif defined(U_WINDOWS)
#   define U_FILE_SEP_CHAR '\\'
#   define U_FILE_ALT_SEP_CHAR '/'
#   define U_PATH_SEP_CHAR ';'
#   define U_FILE_SEP_STRING "\\"
#   define U_FILE_ALT_SEP_STRING "/"
#   define U_PATH_SEP_STRING ";"
#else
#   define U_FILE_SEP_CHAR '/'
#   define U_FILE_ALT_SEP_CHAR '/'
#   define U_PATH_SEP_CHAR ':'
#   define U_FILE_SEP_STRING "/"
#   define U_FILE_ALT_SEP_STRING "/"
#   define U_PATH_SEP_STRING ":"
#endif

/** @} */

/**
 * Convert char characters to UChar characters.
 * This utility function is useful only for "invariant characters"
 * that are encoded in the platform default encoding.
 * They are a small, constant subset of the encoding and include
 * just the latin letters, digits, and some punctuation.
 * For details, see U_CHARSET_FAMILY.
 *
 * @param cs Input string, points to <code>length</code>
 *           character bytes from a subset of the platform encoding.
 * @param us Output string, points to memory for <code>length</code>
 *           Unicode characters.
 * @param length The number of characters to convert; this may
 *               include the terminating <code>NUL</code>.
 *
 * @see U_CHARSET_FAMILY
 * @stable ICU 2.0
 */
U_STABLE void U_EXPORT2
u_charsToUChars(const char *cs, UChar *us, int32_t length);

/**
 * Convert UChar characters to char characters.
 * This utility function is useful only for "invariant characters"
 * that can be encoded in the platform default encoding.
 * They are a small, constant subset of the encoding and include
 * just the latin letters, digits, and some punctuation.
 * For details, see U_CHARSET_FAMILY.
 *
 * @param us Input string, points to <code>length</code>
 *           Unicode characters that can be encoded with the
 *           codepage-invariant subset of the platform encoding.
 * @param cs Output string, points to memory for <code>length</code>
 *           character bytes.
 * @param length The number of characters to convert; this may
 *               include the terminating <code>NUL</code>.
 *
 * @see U_CHARSET_FAMILY
 * @stable ICU 2.0
 */
U_STABLE void U_EXPORT2
u_UCharsToChars(const UChar *us, char *cs, int32_t length);

#endif
