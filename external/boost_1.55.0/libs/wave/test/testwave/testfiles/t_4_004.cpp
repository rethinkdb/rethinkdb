/*=============================================================================
    Boost.Wave: A Standard compliant C++ preprocessor library
    http://www.boost.org/

    Copyright (c) 2001-2012 Hartmut Kaiser. Distributed under the Boost
    Software License, Version 1.0. (See accompanying file
    LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
=============================================================================*/

// Tests, whether invalid expression errors get ignored for 'passive' #elif 
// expressions
#define USHRT_MAX 0xffffU

// The number of bytes in a short.
# if !defined (SIZEOF_SHORT)
#   if (USHRT_MAX) == 255U
#     define SIZEOF_SHORT 1
#   elif (USHRT_MAX) == 65535U
#     define SIZEOF_SHORT 2
#   elif (USHRT_MAX) == 4294967295U
#     define SIZEOF_SHORT 4
#   elif (USHRT_MAX) == 18446744073709551615U  
#     define SIZEOF_SHORT 8
#   else
#     error: unsupported short size, must be updated for this platform!
#   endif /* USHRT_MAX */
# endif /* !defined (SIZEOF_SHORT) */

//R #line 32 "t_4_004.cpp"
//R true
#if SIZEOF_SHORT == 2
true
#else
false
#endif

//H 10: t_4_004.cpp(12): #define
//H 08: t_4_004.cpp(12): USHRT_MAX=0xffffU
//H 10: t_4_004.cpp(15): # if
//H 11: t_4_004.cpp(15): # if !defined (SIZEOF_SHORT): 1
//H 10: t_4_004.cpp(16): #   if
//H 01: t_4_004.cpp(12): USHRT_MAX
//H 02: 0xffffU
//H 03: 0xffffU
//H 11: t_4_004.cpp(16): #   if (USHRT_MAX) == 255U: 0
//H 10: t_4_004.cpp(18): #   elif
//H 01: t_4_004.cpp(12): USHRT_MAX
//H 02: 0xffffU
//H 03: 0xffffU
//H 11: t_4_004.cpp(18): #   elif (USHRT_MAX) == 65535U: 1
//H 10: t_4_004.cpp(19): #define
//H 08: t_4_004.cpp(19): SIZEOF_SHORT=2
//H 10: t_4_004.cpp(20): #   elif
//H 10: t_4_004.cpp(22): #   elif
//H 10: t_4_004.cpp(27): # endif
//H 10: t_4_004.cpp(31): #if
//H 01: t_4_004.cpp(19): SIZEOF_SHORT
//H 02: 2
//H 03: 2
//H 11: t_4_004.cpp(31): #if SIZEOF_SHORT == 2: 1
//H 10: t_4_004.cpp(33): #else
