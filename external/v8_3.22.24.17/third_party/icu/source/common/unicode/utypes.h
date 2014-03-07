/*
**********************************************************************
*   Copyright (C) 1996-2010, International Business Machines
*   Corporation and others.  All Rights Reserved.
**********************************************************************
*
*  FILE NAME : UTYPES.H (formerly ptypes.h)
*
*   Date        Name        Description
*   12/11/96    helena      Creation.
*   02/27/97    aliu        Added typedefs for UClassID, int8, int16, int32,
*                           uint8, uint16, and uint32.
*   04/01/97    aliu        Added XP_CPLUSPLUS and modified to work under C as
*                            well as C++.
*                           Modified to use memcpy() for uprv_arrayCopy() fns.
*   04/14/97    aliu        Added TPlatformUtilities.
*   05/07/97    aliu        Added import/export specifiers (replacing the old
*                           broken EXT_CLASS).  Added version number for our
*                           code.  Cleaned up header.
*    6/20/97    helena      Java class name change.
*   08/11/98    stephen     UErrorCode changed from typedef to enum
*   08/12/98    erm         Changed T_ANALYTIC_PACKAGE_VERSION to 3
*   08/14/98    stephen     Added uprv_arrayCopy() for int8_t, int16_t, int32_t
*   12/09/98    jfitz       Added BUFFER_OVERFLOW_ERROR (bug 1100066)
*   04/20/99    stephen     Cleaned up & reworked for autoconf.
*                           Renamed to utypes.h.
*   05/05/99    stephen     Changed to use <inttypes.h>
*   12/07/99    helena      Moved copyright notice string from ucnv_bld.h here.
*******************************************************************************
*/

#ifndef UTYPES_H
#define UTYPES_H


#include "unicode/umachine.h"
#include "unicode/utf.h"
#include "unicode/uversion.h"
#include "unicode/uconfig.h"

/*!
 * \file
 * \brief Basic definitions for ICU, for both C and C++ APIs
 *
 * This file defines basic types, constants, and enumerations directly or
 * indirectly by including other header files, especially utf.h for the
 * basic character and string definitions and umachine.h for consistent
 * integer and other types.
 */


/**
 * \def U_SHOW_CPLUSPLUS_API
 * @internal
 */
#ifdef XP_CPLUSPLUS
#   ifndef U_SHOW_CPLUSPLUS_API
#       define U_SHOW_CPLUSPLUS_API 1
#   endif
#else
#   undef U_SHOW_CPLUSPLUS_API
#   define U_SHOW_CPLUSPLUS_API 0
#endif

/** @{ API visibility control */

/**
 * \def U_HIDE_DRAFT_API
 * Define this to 1 to request that draft API be "hidden"
 */
#if !U_DEFAULT_SHOW_DRAFT && !defined(U_SHOW_DRAFT_API)
#define U_HIDE_DRAFT_API 1
#endif
#if !U_DEFAULT_SHOW_DRAFT && !defined(U_SHOW_INTERNAL_API)
#define U_HIDE_INTERNAL_API 1
#endif

#ifdef U_HIDE_DRAFT_API
#include "unicode/udraft.h"
#endif

#ifdef U_HIDE_DEPRECATED_API
#include "unicode/udeprctd.h"
#endif

#ifdef U_HIDE_DEPRECATED_API
#include "unicode/uobslete.h"
#endif

#ifdef U_HIDE_INTERNAL_API
#include "unicode/uintrnal.h"
#endif

#ifdef U_HIDE_SYSTEM_API
#include "unicode/usystem.h"
#endif

/** @} */


/*===========================================================================*/
/* char Character set family                                                 */
/*===========================================================================*/

/**
 * U_CHARSET_FAMILY is equal to this value when the platform is an ASCII based platform.
 * @stable ICU 2.0
 */
#define U_ASCII_FAMILY 0

/**
 * U_CHARSET_FAMILY is equal to this value when the platform is an EBCDIC based platform.
 * @stable ICU 2.0
 */
#define U_EBCDIC_FAMILY 1

/**
 * \def U_CHARSET_FAMILY
 *
 * <p>These definitions allow to specify the encoding of text
 * in the char data type as defined by the platform and the compiler.
 * It is enough to determine the code point values of "invariant characters",
 * which are the ones shared by all encodings that are in use
 * on a given platform.</p>
 *
 * <p>Those "invariant characters" should be all the uppercase and lowercase
 * latin letters, the digits, the space, and "basic punctuation".
 * Also, '\\n', '\\r', '\\t' should be available.</p>
 *
 * <p>The list of "invariant characters" is:<br>
 * \code
 *    A-Z  a-z  0-9  SPACE  "  %  &amp;  '  (  )  *  +  ,  -  .  /  :  ;  <  =  >  ?  _
 * \endcode
 * <br>
 * (52 letters + 10 numbers + 20 punc/sym/space = 82 total)</p>
 *
 * <p>This matches the IBM Syntactic Character Set (CS 640).</p>
 *
 * <p>In other words, all the graphic characters in 7-bit ASCII should
 * be safely accessible except the following:</p>
 *
 * \code
 *    '\' <backslash>
 *    '[' <left bracket>
 *    ']' <right bracket>
 *    '{' <left brace>
 *    '}' <right brace>
 *    '^' <circumflex>
 *    '~' <tilde>
 *    '!' <exclamation mark>
 *    '#' <number sign>
 *    '|' <vertical line>
 *    '$' <dollar sign>
 *    '@' <commercial at>
 *    '`' <grave accent>
 * \endcode
 * @stable ICU 2.0
 */

#ifndef U_CHARSET_FAMILY
#   define U_CHARSET_FAMILY 0
#endif

/**
 * \def U_CHARSET_IS_UTF8
 *
 * Hardcode the default charset to UTF-8.
 *
 * If this is set to 1, then
 * - ICU will assume that all non-invariant char*, StringPiece, std::string etc.
 *   contain UTF-8 text, regardless of what the system API uses
 * - some ICU code will use fast functions like u_strFromUTF8()
 *   rather than the more general and more heavy-weight conversion API (ucnv.h)
 * - ucnv_getDefaultName() always returns "UTF-8"
 * - ucnv_setDefaultName() is disabled and will not change the default charset
 * - static builds of ICU are smaller
 * - more functionality is available with the UCONFIG_NO_CONVERSION build-time
 *   configuration option (see unicode/uconfig.h)
 * - the UCONFIG_NO_CONVERSION build option in uconfig.h is more usable
 *
 * @stable ICU 4.2
 * @see UCONFIG_NO_CONVERSION
 */
#ifndef U_CHARSET_IS_UTF8
#   define U_CHARSET_IS_UTF8 0
#endif

/*===========================================================================*/
/* ICUDATA naming scheme                                                     */
/*===========================================================================*/

/**
 * \def U_ICUDATA_TYPE_LETTER
 *
 * This is a platform-dependent string containing one letter:
 * - b for big-endian, ASCII-family platforms
 * - l for little-endian, ASCII-family platforms
 * - e for big-endian, EBCDIC-family platforms
 * This letter is part of the common data file name.
 * @stable ICU 2.0
 */

/**
 * \def U_ICUDATA_TYPE_LITLETTER
 * The non-string form of U_ICUDATA_TYPE_LETTER
 * @stable ICU 2.0
 */
#if U_CHARSET_FAMILY
#   if U_IS_BIG_ENDIAN
   /* EBCDIC - should always be BE */
#     define U_ICUDATA_TYPE_LETTER "e"
#     define U_ICUDATA_TYPE_LITLETTER e
#   else
#     error "Don't know what to do with little endian EBCDIC!"
#     define U_ICUDATA_TYPE_LETTER "x"
#     define U_ICUDATA_TYPE_LITLETTER x
#   endif
#else
#   if U_IS_BIG_ENDIAN
      /* Big-endian ASCII */
#     define U_ICUDATA_TYPE_LETTER "b"
#     define U_ICUDATA_TYPE_LITLETTER b
#   else
      /* Little-endian ASCII */
#     define U_ICUDATA_TYPE_LETTER "l"
#     define U_ICUDATA_TYPE_LITLETTER l
#   endif
#endif

/**
 * A single string literal containing the icudata stub name. i.e. 'icudt18e' for
 * ICU 1.8.x on EBCDIC, etc..
 * @stable ICU 2.0
 */
#define U_ICUDATA_NAME    "icudt" U_ICU_VERSION_SHORT U_ICUDATA_TYPE_LETTER  /**< @internal */
#define U_USRDATA_NAME    "usrdt" U_ICU_VERSION_SHORT U_ICUDATA_TYPE_LETTER  /**< @internal */
#define U_USE_USRDATA     1  /**< @internal */

/**
 *  U_ICU_ENTRY_POINT is the name of the DLL entry point to the ICU data library.
 *    Defined as a literal, not a string.
 *    Tricky Preprocessor use - ## operator replaces macro paramters with the literal string
 *                              from the corresponding macro invocation, _before_ other macro substitutions.
 *                              Need a nested \#defines to get the actual version numbers rather than
 *                              the literal text U_ICU_VERSION_MAJOR_NUM into the name.
 *                              The net result will be something of the form
 *                                  \#define U_ICU_ENTRY_POINT icudt19_dat
 * @stable ICU 2.4
 */
#define U_ICUDATA_ENTRY_POINT  U_DEF2_ICUDATA_ENTRY_POINT(U_ICU_VERSION_MAJOR_NUM, U_ICU_VERSION_MINOR_NUM)

/**
 * Do not use.
 * @internal
 */
#define U_DEF2_ICUDATA_ENTRY_POINT(major, minor) U_DEF_ICUDATA_ENTRY_POINT(major, minor)
/**
 * Do not use.
 * @internal
 */
#ifndef U_DEF_ICUDATA_ENTRY_POINT
/* affected by symbol renaming. See platform.h */
#define U_DEF_ICUDATA_ENTRY_POINT(major, minor) icudt##major##minor##_dat
#endif

/**
 * \def U_CALLCONV
 * Similar to U_CDECL_BEGIN/U_CDECL_END, this qualifier is necessary
 * in callback function typedefs to make sure that the calling convention
 * is compatible.
 *
 * This is only used for non-ICU-API functions.
 * When a function is a public ICU API,
 * you must use the U_CAPI and U_EXPORT2 qualifiers.
 * @stable ICU 2.0
 */
#if defined(OS390) && defined(XP_CPLUSPLUS)
#    define U_CALLCONV __cdecl
#else
#    define U_CALLCONV U_EXPORT2
#endif

/**
 * \def NULL
 * Define NULL if necessary, to 0 for C++ and to ((void *)0) for C.
 * @stable ICU 2.0
 */
#ifndef NULL
#ifdef XP_CPLUSPLUS
#define NULL    0
#else
#define NULL    ((void *)0)
#endif
#endif

/*===========================================================================*/
/* Calendar/TimeZone data types                                              */
/*===========================================================================*/

/**
 * Date and Time data type.
 * This is a primitive data type that holds the date and time
 * as the number of milliseconds since 1970-jan-01, 00:00 UTC.
 * UTC leap seconds are ignored.
 * @stable ICU 2.0
 */
typedef double UDate;

/** The number of milliseconds per second @stable ICU 2.0 */
#define U_MILLIS_PER_SECOND        (1000)
/** The number of milliseconds per minute @stable ICU 2.0 */
#define U_MILLIS_PER_MINUTE       (60000)
/** The number of milliseconds per hour @stable ICU 2.0 */
#define U_MILLIS_PER_HOUR       (3600000)
/** The number of milliseconds per day @stable ICU 2.0 */
#define U_MILLIS_PER_DAY       (86400000)


/*===========================================================================*/
/* UClassID-based RTTI */
/*===========================================================================*/

/**
 * UClassID is used to identify classes without using RTTI, since RTTI
 * is not yet supported by all C++ compilers.  Each class hierarchy which needs
 * to implement polymorphic clone() or operator==() defines two methods,
 * described in detail below.  UClassID values can be compared using
 * operator==(). Nothing else should be done with them.
 *
 * \par
 * getDynamicClassID() is declared in the base class of the hierarchy as
 * a pure virtual.  Each concrete subclass implements it in the same way:
 *
 * \code
 *      class Base {
 *      public:
 *          virtual UClassID getDynamicClassID() const = 0;
 *      }
 *
 *      class Derived {
 *      public:
 *          virtual UClassID getDynamicClassID() const
 *            { return Derived::getStaticClassID(); }
 *      }
 * \endcode
 *
 * Each concrete class implements getStaticClassID() as well, which allows
 * clients to test for a specific type.
 *
 * \code
 *      class Derived {
 *      public:
 *          static UClassID U_EXPORT2 getStaticClassID();
 *      private:
 *          static char fgClassID;
 *      }
 *
 *      // In Derived.cpp:
 *      UClassID Derived::getStaticClassID()
 *        { return (UClassID)&Derived::fgClassID; }
 *      char Derived::fgClassID = 0; // Value is irrelevant
 * \endcode
 * @stable ICU 2.0
 */
typedef void* UClassID;

/*===========================================================================*/
/* Shared library/DLL import-export API control                              */
/*===========================================================================*/

/*
 * Control of symbol import/export.
 * ICU is separated into three libraries.
 */

/*
 * \def U_COMBINED_IMPLEMENTATION
 * Set to export library symbols from inside the ICU library
 * when all of ICU is in a single library.
 * This can be set as a compiler option while building ICU, and it
 * needs to be the first one tested to override U_COMMON_API, U_I18N_API, etc.
 * @stable ICU 2.0
 */

/**
 * \def U_DATA_API
 * Set to export library symbols from inside the stubdata library,
 * and to import them from outside.
 * @stable ICU 3.0
 */

/**
 * \def U_COMMON_API
 * Set to export library symbols from inside the common library,
 * and to import them from outside.
 * @stable ICU 2.0
 */

/**
 * \def U_I18N_API
 * Set to export library symbols from inside the i18n library,
 * and to import them from outside.
 * @stable ICU 2.0
 */

/**
 * \def U_LAYOUT_API
 * Set to export library symbols from inside the layout engine library,
 * and to import them from outside.
 * @stable ICU 2.0
 */

/**
 * \def U_LAYOUTEX_API
 * Set to export library symbols from inside the layout extensions library,
 * and to import them from outside.
 * @stable ICU 2.6
 */

/**
 * \def U_IO_API
 * Set to export library symbols from inside the ustdio library,
 * and to import them from outside.
 * @stable ICU 2.0
 */

/**
 * \def U_TOOLUTIL_API
 * Set to export library symbols from inside the toolutil library,
 * and to import them from outside.
 * @stable ICU 3.4
 */

#if defined(U_COMBINED_IMPLEMENTATION)
#define U_DATA_API     U_EXPORT
#define U_COMMON_API   U_EXPORT
#define U_I18N_API     U_EXPORT
#define U_LAYOUT_API   U_EXPORT
#define U_LAYOUTEX_API U_EXPORT
#define U_IO_API       U_EXPORT
#define U_TOOLUTIL_API U_EXPORT
#elif defined(U_STATIC_IMPLEMENTATION)
#define U_DATA_API
#define U_COMMON_API
#define U_I18N_API
#define U_LAYOUT_API
#define U_LAYOUTEX_API
#define U_IO_API
#define U_TOOLUTIL_API
#elif defined(U_COMMON_IMPLEMENTATION)
#define U_DATA_API     U_IMPORT
#define U_COMMON_API   U_EXPORT
#define U_I18N_API     U_IMPORT
#define U_LAYOUT_API   U_IMPORT
#define U_LAYOUTEX_API U_IMPORT
#define U_IO_API       U_IMPORT
#define U_TOOLUTIL_API U_IMPORT
#elif defined(U_I18N_IMPLEMENTATION)
#define U_DATA_API     U_IMPORT
#define U_COMMON_API   U_IMPORT
#define U_I18N_API     U_EXPORT
#define U_LAYOUT_API   U_IMPORT
#define U_LAYOUTEX_API U_IMPORT
#define U_IO_API       U_IMPORT
#define U_TOOLUTIL_API U_IMPORT
#elif defined(U_LAYOUT_IMPLEMENTATION)
#define U_DATA_API     U_IMPORT
#define U_COMMON_API   U_IMPORT
#define U_I18N_API     U_IMPORT
#define U_LAYOUT_API   U_EXPORT
#define U_LAYOUTEX_API U_IMPORT
#define U_IO_API       U_IMPORT
#define U_TOOLUTIL_API U_IMPORT
#elif defined(U_LAYOUTEX_IMPLEMENTATION)
#define U_DATA_API     U_IMPORT
#define U_COMMON_API   U_IMPORT
#define U_I18N_API     U_IMPORT
#define U_LAYOUT_API   U_IMPORT
#define U_LAYOUTEX_API U_EXPORT
#define U_IO_API       U_IMPORT
#define U_TOOLUTIL_API U_IMPORT
#elif defined(U_IO_IMPLEMENTATION)
#define U_DATA_API     U_IMPORT
#define U_COMMON_API   U_IMPORT
#define U_I18N_API     U_IMPORT
#define U_LAYOUT_API   U_IMPORT
#define U_LAYOUTEX_API U_IMPORT
#define U_IO_API       U_EXPORT
#define U_TOOLUTIL_API U_IMPORT
#elif defined(U_TOOLUTIL_IMPLEMENTATION)
#define U_DATA_API     U_IMPORT
#define U_COMMON_API   U_IMPORT
#define U_I18N_API     U_IMPORT
#define U_LAYOUT_API   U_IMPORT
#define U_LAYOUTEX_API U_IMPORT
#define U_IO_API       U_IMPORT
#define U_TOOLUTIL_API U_EXPORT
#else
#define U_DATA_API     U_IMPORT
#define U_COMMON_API   U_IMPORT
#define U_I18N_API     U_IMPORT
#define U_LAYOUT_API   U_IMPORT
#define U_LAYOUTEX_API U_IMPORT
#define U_IO_API       U_IMPORT
#define U_TOOLUTIL_API U_IMPORT
#endif

/**
 * \def U_STANDARD_CPP_NAMESPACE
 * Control of C++ Namespace
 * @stable ICU 2.0
 */
#ifdef __cplusplus
#define U_STANDARD_CPP_NAMESPACE        ::
#else
#define U_STANDARD_CPP_NAMESPACE
#endif


/*===========================================================================*/
/* Global delete operator                                                    */
/*===========================================================================*/

/*
 * The ICU4C library must not use the global new and delete operators.
 * These operators here are defined to enable testing for this.
 * See Jitterbug 2581 for details of why this is necessary.
 *
 * Verification that ICU4C's memory usage is correct, i.e.,
 * that global new/delete are not used:
 *
 * a) Check for imports of global new/delete (see uobject.cpp for details)
 * b) Verify that new is never imported.
 * c) Verify that delete is only imported from object code for interface/mixin classes.
 * d) Add global delete and delete[] only for the ICU4C library itself
 *    and define them in a way that crashes or otherwise easily shows a problem.
 *
 * The following implements d).
 * The operator implementations crash; this is intentional and used for library debugging.
 *
 * Note: This is currently only done on Windows because
 * some Linux/Unix compilers have problems with defining global new/delete.
 * On Windows, U_WINDOWS is defined, and it is _MSC_VER>=1200 for MSVC 6.0 and higher.
 */
#if defined(XP_CPLUSPLUS) && defined(U_WINDOWS) && U_DEBUG && U_OVERRIDE_CXX_ALLOCATION && (_MSC_VER>=1200) && !defined(U_STATIC_IMPLEMENTATION) && (defined(U_COMMON_IMPLEMENTATION) || defined(U_I18N_IMPLEMENTATION) || defined(U_IO_IMPLEMENTATION) || defined(U_LAYOUT_IMPLEMENTATION) || defined(U_LAYOUTEX_IMPLEMENTATION))

#ifndef U_HIDE_INTERNAL_API
/**
 * Global operator new, defined only inside ICU4C, must not be used.
 * Crashes intentionally.
 * @internal
 */
inline void *
operator new(size_t /*size*/) {
    char *q=NULL;
    *q=5; /* break it */
    return q;
}

#ifdef _Ret_bytecap_
/* This is only needed to suppress a Visual C++ 2008 warning for operator new[]. */
_Ret_bytecap_(_Size)
#endif
/**
 * Global operator new[], defined only inside ICU4C, must not be used.
 * Crashes intentionally.
 * @internal
 */
inline void *
operator new[](size_t /*size*/) {
    char *q=NULL;
    *q=5; /* break it */
    return q;
}

/**
 * Global operator delete, defined only inside ICU4C, must not be used.
 * Crashes intentionally.
 * @internal
 */
inline void
operator delete(void * /*p*/) {
    char *q=NULL;
    *q=5; /* break it */
}

/**
 * Global operator delete[], defined only inside ICU4C, must not be used.
 * Crashes intentionally.
 * @internal
 */
inline void
operator delete[](void * /*p*/) {
    char *q=NULL;
    *q=5; /* break it */
}

#endif /* U_HIDE_INTERNAL_API */
#endif

/*===========================================================================*/
/* UErrorCode */
/*===========================================================================*/

/**
 * Error code to replace exception handling, so that the code is compatible with all C++ compilers,
 * and to use the same mechanism for C and C++.
 *
 * \par
 * ICU functions that take a reference (C++) or a pointer (C) to a UErrorCode
 * first test if(U_FAILURE(errorCode)) { return immediately; }
 * so that in a chain of such functions the first one that sets an error code
 * causes the following ones to not perform any operations.
 *
 * \par
 * Error codes should be tested using U_FAILURE() and U_SUCCESS().
 * @stable ICU 2.0
 */
typedef enum UErrorCode {
    /* The ordering of U_ERROR_INFO_START Vs U_USING_FALLBACK_WARNING looks weird
     * and is that way because VC++ debugger displays first encountered constant,
     * which is not the what the code is used for
     */

    U_USING_FALLBACK_WARNING  = -128,   /**< A resource bundle lookup returned a fallback result (not an error) */

    U_ERROR_WARNING_START     = -128,   /**< Start of information results (semantically successful) */

    U_USING_DEFAULT_WARNING   = -127,   /**< A resource bundle lookup returned a result from the root locale (not an error) */

    U_SAFECLONE_ALLOCATED_WARNING = -126, /**< A SafeClone operation required allocating memory (informational only) */

    U_STATE_OLD_WARNING       = -125,   /**< ICU has to use compatibility layer to construct the service. Expect performance/memory usage degradation. Consider upgrading */

    U_STRING_NOT_TERMINATED_WARNING = -124,/**< An output string could not be NUL-terminated because output length==destCapacity. */

    U_SORT_KEY_TOO_SHORT_WARNING = -123, /**< Number of levels requested in getBound is higher than the number of levels in the sort key */

    U_AMBIGUOUS_ALIAS_WARNING = -122,   /**< This converter alias can go to different converter implementations */

    U_DIFFERENT_UCA_VERSION = -121,     /**< ucol_open encountered a mismatch between UCA version and collator image version, so the collator was constructed from rules. No impact to further function */
    
    U_PLUGIN_CHANGED_LEVEL_WARNING = -120, /**< A plugin caused a level change. May not be an error, but later plugins may not load. */

    U_ERROR_WARNING_LIMIT,              /**< This must always be the last warning value to indicate the limit for UErrorCode warnings (last warning code +1) */


    U_ZERO_ERROR              =  0,     /**< No error, no warning. */

    U_ILLEGAL_ARGUMENT_ERROR  =  1,     /**< Start of codes indicating failure */
    U_MISSING_RESOURCE_ERROR  =  2,     /**< The requested resource cannot be found */
    U_INVALID_FORMAT_ERROR    =  3,     /**< Data format is not what is expected */
    U_FILE_ACCESS_ERROR       =  4,     /**< The requested file cannot be found */
    U_INTERNAL_PROGRAM_ERROR  =  5,     /**< Indicates a bug in the library code */
    U_MESSAGE_PARSE_ERROR     =  6,     /**< Unable to parse a message (message format) */
    U_MEMORY_ALLOCATION_ERROR =  7,     /**< Memory allocation error */
    U_INDEX_OUTOFBOUNDS_ERROR =  8,     /**< Trying to access the index that is out of bounds */
    U_PARSE_ERROR             =  9,     /**< Equivalent to Java ParseException */
    U_INVALID_CHAR_FOUND      = 10,     /**< Character conversion: Unmappable input sequence. In other APIs: Invalid character. */
    U_TRUNCATED_CHAR_FOUND    = 11,     /**< Character conversion: Incomplete input sequence. */
    U_ILLEGAL_CHAR_FOUND      = 12,     /**< Character conversion: Illegal input sequence/combination of input units. */
    U_INVALID_TABLE_FORMAT    = 13,     /**< Conversion table file found, but corrupted */
    U_INVALID_TABLE_FILE      = 14,     /**< Conversion table file not found */
    U_BUFFER_OVERFLOW_ERROR   = 15,     /**< A result would not fit in the supplied buffer */
    U_UNSUPPORTED_ERROR       = 16,     /**< Requested operation not supported in current context */
    U_RESOURCE_TYPE_MISMATCH  = 17,     /**< an operation is requested over a resource that does not support it */
    U_ILLEGAL_ESCAPE_SEQUENCE = 18,     /**< ISO-2022 illlegal escape sequence */
    U_UNSUPPORTED_ESCAPE_SEQUENCE = 19, /**< ISO-2022 unsupported escape sequence */
    U_NO_SPACE_AVAILABLE      = 20,     /**< No space available for in-buffer expansion for Arabic shaping */
    U_CE_NOT_FOUND_ERROR      = 21,     /**< Currently used only while setting variable top, but can be used generally */
    U_PRIMARY_TOO_LONG_ERROR  = 22,     /**< User tried to set variable top to a primary that is longer than two bytes */
    U_STATE_TOO_OLD_ERROR     = 23,     /**< ICU cannot construct a service from this state, as it is no longer supported */
    U_TOO_MANY_ALIASES_ERROR  = 24,     /**< There are too many aliases in the path to the requested resource.
                                             It is very possible that a circular alias definition has occured */
    U_ENUM_OUT_OF_SYNC_ERROR  = 25,     /**< UEnumeration out of sync with underlying collection */
    U_INVARIANT_CONVERSION_ERROR = 26,  /**< Unable to convert a UChar* string to char* with the invariant converter. */
    U_INVALID_STATE_ERROR     = 27,     /**< Requested operation can not be completed with ICU in its current state */
    U_COLLATOR_VERSION_MISMATCH = 28,   /**< Collator version is not compatible with the base version */
    U_USELESS_COLLATOR_ERROR  = 29,     /**< Collator is options only and no base is specified */
    U_NO_WRITE_PERMISSION     = 30,     /**< Attempt to modify read-only or constant data. */

    U_STANDARD_ERROR_LIMIT,             /**< This must always be the last value to indicate the limit for standard errors */
    /*
     * the error code range 0x10000 0x10100 are reserved for Transliterator
     */
    U_BAD_VARIABLE_DEFINITION=0x10000,/**< Missing '$' or duplicate variable name */
    U_PARSE_ERROR_START = 0x10000,    /**< Start of Transliterator errors */
    U_MALFORMED_RULE,                 /**< Elements of a rule are misplaced */
    U_MALFORMED_SET,                  /**< A UnicodeSet pattern is invalid*/
    U_MALFORMED_SYMBOL_REFERENCE,     /**< UNUSED as of ICU 2.4 */
    U_MALFORMED_UNICODE_ESCAPE,       /**< A Unicode escape pattern is invalid*/
    U_MALFORMED_VARIABLE_DEFINITION,  /**< A variable definition is invalid */
    U_MALFORMED_VARIABLE_REFERENCE,   /**< A variable reference is invalid */
    U_MISMATCHED_SEGMENT_DELIMITERS,  /**< UNUSED as of ICU 2.4 */
    U_MISPLACED_ANCHOR_START,         /**< A start anchor appears at an illegal position */
    U_MISPLACED_CURSOR_OFFSET,        /**< A cursor offset occurs at an illegal position */
    U_MISPLACED_QUANTIFIER,           /**< A quantifier appears after a segment close delimiter */
    U_MISSING_OPERATOR,               /**< A rule contains no operator */
    U_MISSING_SEGMENT_CLOSE,          /**< UNUSED as of ICU 2.4 */
    U_MULTIPLE_ANTE_CONTEXTS,         /**< More than one ante context */
    U_MULTIPLE_CURSORS,               /**< More than one cursor */
    U_MULTIPLE_POST_CONTEXTS,         /**< More than one post context */
    U_TRAILING_BACKSLASH,             /**< A dangling backslash */
    U_UNDEFINED_SEGMENT_REFERENCE,    /**< A segment reference does not correspond to a defined segment */
    U_UNDEFINED_VARIABLE,             /**< A variable reference does not correspond to a defined variable */
    U_UNQUOTED_SPECIAL,               /**< A special character was not quoted or escaped */
    U_UNTERMINATED_QUOTE,             /**< A closing single quote is missing */
    U_RULE_MASK_ERROR,                /**< A rule is hidden by an earlier more general rule */
    U_MISPLACED_COMPOUND_FILTER,      /**< A compound filter is in an invalid location */
    U_MULTIPLE_COMPOUND_FILTERS,      /**< More than one compound filter */
    U_INVALID_RBT_SYNTAX,             /**< A "::id" rule was passed to the RuleBasedTransliterator parser */
    U_INVALID_PROPERTY_PATTERN,       /**< UNUSED as of ICU 2.4 */
    U_MALFORMED_PRAGMA,               /**< A 'use' pragma is invlalid */
    U_UNCLOSED_SEGMENT,               /**< A closing ')' is missing */
    U_ILLEGAL_CHAR_IN_SEGMENT,        /**< UNUSED as of ICU 2.4 */
    U_VARIABLE_RANGE_EXHAUSTED,       /**< Too many stand-ins generated for the given variable range */
    U_VARIABLE_RANGE_OVERLAP,         /**< The variable range overlaps characters used in rules */
    U_ILLEGAL_CHARACTER,              /**< A special character is outside its allowed context */
    U_INTERNAL_TRANSLITERATOR_ERROR,  /**< Internal transliterator system error */
    U_INVALID_ID,                     /**< A "::id" rule specifies an unknown transliterator */
    U_INVALID_FUNCTION,               /**< A "&fn()" rule specifies an unknown transliterator */
    U_PARSE_ERROR_LIMIT,              /**< The limit for Transliterator errors */

    /*
     * the error code range 0x10100 0x10200 are reserved for formatting API parsing error
     */
    U_UNEXPECTED_TOKEN=0x10100,       /**< Syntax error in format pattern */
    U_FMT_PARSE_ERROR_START=0x10100,  /**< Start of format library errors */
    U_MULTIPLE_DECIMAL_SEPARATORS,    /**< More than one decimal separator in number pattern */
    U_MULTIPLE_DECIMAL_SEPERATORS = U_MULTIPLE_DECIMAL_SEPARATORS, /**< Typo: kept for backward compatibility. Use U_MULTIPLE_DECIMAL_SEPARATORS */
    U_MULTIPLE_EXPONENTIAL_SYMBOLS,   /**< More than one exponent symbol in number pattern */
    U_MALFORMED_EXPONENTIAL_PATTERN,  /**< Grouping symbol in exponent pattern */
    U_MULTIPLE_PERCENT_SYMBOLS,       /**< More than one percent symbol in number pattern */
    U_MULTIPLE_PERMILL_SYMBOLS,       /**< More than one permill symbol in number pattern */
    U_MULTIPLE_PAD_SPECIFIERS,        /**< More than one pad symbol in number pattern */
    U_PATTERN_SYNTAX_ERROR,           /**< Syntax error in format pattern */
    U_ILLEGAL_PAD_POSITION,           /**< Pad symbol misplaced in number pattern */
    U_UNMATCHED_BRACES,               /**< Braces do not match in message pattern */
    U_UNSUPPORTED_PROPERTY,           /**< UNUSED as of ICU 2.4 */
    U_UNSUPPORTED_ATTRIBUTE,          /**< UNUSED as of ICU 2.4 */
    U_ARGUMENT_TYPE_MISMATCH,         /**< Argument name and argument index mismatch in MessageFormat functions */
    U_DUPLICATE_KEYWORD,              /**< Duplicate keyword in PluralFormat */
    U_UNDEFINED_KEYWORD,              /**< Undefined Plural keyword */
    U_DEFAULT_KEYWORD_MISSING,        /**< Missing DEFAULT rule in plural rules */
    U_DECIMAL_NUMBER_SYNTAX_ERROR,    /**< Decimal number syntax error */
    U_FMT_PARSE_ERROR_LIMIT,          /**< The limit for format library errors */

    /*
     * the error code range 0x10200 0x102ff are reserved for Break Iterator related error
     */
    U_BRK_INTERNAL_ERROR=0x10200,          /**< An internal error (bug) was detected.             */
    U_BRK_ERROR_START=0x10200,             /**< Start of codes indicating Break Iterator failures */
    U_BRK_HEX_DIGITS_EXPECTED,             /**< Hex digits expected as part of a escaped char in a rule. */
    U_BRK_SEMICOLON_EXPECTED,              /**< Missing ';' at the end of a RBBI rule.            */
    U_BRK_RULE_SYNTAX,                     /**< Syntax error in RBBI rule.                        */
    U_BRK_UNCLOSED_SET,                    /**< UnicodeSet witing an RBBI rule missing a closing ']'.  */
    U_BRK_ASSIGN_ERROR,                    /**< Syntax error in RBBI rule assignment statement.   */
    U_BRK_VARIABLE_REDFINITION,            /**< RBBI rule $Variable redefined.                    */
    U_BRK_MISMATCHED_PAREN,                /**< Mis-matched parentheses in an RBBI rule.          */
    U_BRK_NEW_LINE_IN_QUOTED_STRING,       /**< Missing closing quote in an RBBI rule.            */
    U_BRK_UNDEFINED_VARIABLE,              /**< Use of an undefined $Variable in an RBBI rule.    */
    U_BRK_INIT_ERROR,                      /**< Initialization failure.  Probable missing ICU Data. */
    U_BRK_RULE_EMPTY_SET,                  /**< Rule contains an empty Unicode Set.               */
    U_BRK_UNRECOGNIZED_OPTION,             /**< !!option in RBBI rules not recognized.            */
    U_BRK_MALFORMED_RULE_TAG,              /**< The {nnn} tag on a rule is mal formed             */
    U_BRK_ERROR_LIMIT,                     /**< This must always be the last value to indicate the limit for Break Iterator failures */

    /*
     * The error codes in the range 0x10300-0x103ff are reserved for regular expression related errrs
     */
    U_REGEX_INTERNAL_ERROR=0x10300,       /**< An internal error (bug) was detected.              */
    U_REGEX_ERROR_START=0x10300,          /**< Start of codes indicating Regexp failures          */
    U_REGEX_RULE_SYNTAX,                  /**< Syntax error in regexp pattern.                    */
    U_REGEX_INVALID_STATE,                /**< RegexMatcher in invalid state for requested operation */
    U_REGEX_BAD_ESCAPE_SEQUENCE,          /**< Unrecognized backslash escape sequence in pattern  */
    U_REGEX_PROPERTY_SYNTAX,              /**< Incorrect Unicode property                         */
    U_REGEX_UNIMPLEMENTED,                /**< Use of regexp feature that is not yet implemented. */
    U_REGEX_MISMATCHED_PAREN,             /**< Incorrectly nested parentheses in regexp pattern.  */
    U_REGEX_NUMBER_TOO_BIG,               /**< Decimal number is too large.                       */
    U_REGEX_BAD_INTERVAL,                 /**< Error in {min,max} interval                        */
    U_REGEX_MAX_LT_MIN,                   /**< In {min,max}, max is less than min.                */
    U_REGEX_INVALID_BACK_REF,             /**< Back-reference to a non-existent capture group.    */
    U_REGEX_INVALID_FLAG,                 /**< Invalid value for match mode flags.                */
    U_REGEX_LOOK_BEHIND_LIMIT,            /**< Look-Behind pattern matches must have a bounded maximum length.    */
    U_REGEX_SET_CONTAINS_STRING,          /**< Regexps cannot have UnicodeSets containing strings.*/
    U_REGEX_OCTAL_TOO_BIG,                /**< Octal character constants must be <= 0377.         */
    U_REGEX_MISSING_CLOSE_BRACKET,        /**< Missing closing bracket on a bracket expression.   */
    U_REGEX_INVALID_RANGE,                /**< In a character range [x-y], x is greater than y.   */
    U_REGEX_STACK_OVERFLOW,               /**< Regular expression backtrack stack overflow.       */
    U_REGEX_TIME_OUT,                     /**< Maximum allowed match time exceeded                */
    U_REGEX_STOPPED_BY_CALLER,            /**< Matching operation aborted by user callback fn.    */
    U_REGEX_ERROR_LIMIT,                  /**< This must always be the last value to indicate the limit for regexp errors */

    /*
     * The error code in the range 0x10400-0x104ff are reserved for IDNA related error codes
     */
    U_IDNA_PROHIBITED_ERROR=0x10400,
    U_IDNA_ERROR_START=0x10400,
    U_IDNA_UNASSIGNED_ERROR,
    U_IDNA_CHECK_BIDI_ERROR,
    U_IDNA_STD3_ASCII_RULES_ERROR,
    U_IDNA_ACE_PREFIX_ERROR,
    U_IDNA_VERIFICATION_ERROR,
    U_IDNA_LABEL_TOO_LONG_ERROR,
    U_IDNA_ZERO_LENGTH_LABEL_ERROR,
    U_IDNA_DOMAIN_NAME_TOO_LONG_ERROR,
    U_IDNA_ERROR_LIMIT,
    /*
     * Aliases for StringPrep
     */
    U_STRINGPREP_PROHIBITED_ERROR = U_IDNA_PROHIBITED_ERROR,
    U_STRINGPREP_UNASSIGNED_ERROR = U_IDNA_UNASSIGNED_ERROR,
    U_STRINGPREP_CHECK_BIDI_ERROR = U_IDNA_CHECK_BIDI_ERROR,
    
    /*
     * The error code in the range 0x10500-0x105ff are reserved for Plugin related error codes
     */
    U_PLUGIN_ERROR_START=0x10500,         /**< Start of codes indicating plugin failures */
    U_PLUGIN_TOO_HIGH=0x10500,            /**< The plugin's level is too high to be loaded right now. */
    U_PLUGIN_DIDNT_SET_LEVEL,             /**< The plugin didn't call uplug_setPlugLevel in response to a QUERY */
    U_PLUGIN_ERROR_LIMIT,                 /**< This must always be the last value to indicate the limit for plugin errors */

    U_ERROR_LIMIT=U_PLUGIN_ERROR_LIMIT      /**< This must always be the last value to indicate the limit for UErrorCode (last error code +1) */
} UErrorCode;

/* Use the following to determine if an UErrorCode represents */
/* operational success or failure. */

#ifdef XP_CPLUSPLUS
    /**
     * Does the error code indicate success?
     * @stable ICU 2.0
     */
    static
    inline UBool U_SUCCESS(UErrorCode code) { return (UBool)(code<=U_ZERO_ERROR); }
    /**
     * Does the error code indicate a failure?
     * @stable ICU 2.0
     */
    static
    inline UBool U_FAILURE(UErrorCode code) { return (UBool)(code>U_ZERO_ERROR); }
#else
    /**
     * Does the error code indicate success?
     * @stable ICU 2.0
     */
#   define U_SUCCESS(x) ((x)<=U_ZERO_ERROR)
    /**
     * Does the error code indicate a failure?
     * @stable ICU 2.0
     */
#   define U_FAILURE(x) ((x)>U_ZERO_ERROR)
#endif

/**
 * Return a string for a UErrorCode value.
 * The string will be the same as the name of the error code constant
 * in the UErrorCode enum above.
 * @stable ICU 2.0
 */
U_STABLE const char * U_EXPORT2
u_errorName(UErrorCode code);


#endif /* _UTYPES */
