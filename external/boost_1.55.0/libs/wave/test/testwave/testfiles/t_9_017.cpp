/*=============================================================================
    Boost.Wave: A Standard compliant C++ preprocessor library
    http://www.boost.org/

    Copyright (c) 2001-2012 Hartmut Kaiser. Distributed under the Boost
    Software License, Version 1.0. (See accompanying file
    LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
=============================================================================*/

// tests, whether true and false are usable a 'normal' identifiers

#define true   1
#define false  0

//R #line 18 "t_9_017.cpp"
//R 1 is defined
#if defined(true)
true is defined
#else
true is not defined
#endif

//R #line 26 "t_9_017.cpp"
//R 0 is defined
#if defined(false)
false is defined
#else
false is not defined
#endif

#undef true
#undef false

//R #line 37 "t_9_017.cpp"
//R true is not defined
#ifndef true
true is not defined
#else
true is defined
#endif

//R #line 45 "t_9_017.cpp"
//R false is not defined
#ifndef false
false is not defined
#else
false is defined
#endif

//H 10: t_9_017.cpp(12): #define
//H 08: t_9_017.cpp(12): true=1
//H 10: t_9_017.cpp(13): #define
//H 08: t_9_017.cpp(13): false=0
//H 10: t_9_017.cpp(17): #if
//H 11: t_9_017.cpp(17): #if defined(true): 1
//H 01: t_9_017.cpp(12): true
//H 02: 1
//H 03: 1
//H 10: t_9_017.cpp(19): #else
//H 10: t_9_017.cpp(25): #if
//H 11: t_9_017.cpp(25): #if defined(false): 1
//H 01: t_9_017.cpp(13): false
//H 02: 0
//H 03: 0
//H 10: t_9_017.cpp(27): #else
//H 10: t_9_017.cpp(31): #undef
//H 09: t_9_017.cpp(31): true
//H 10: t_9_017.cpp(32): #undef
//H 09: t_9_017.cpp(32): false
//H 10: t_9_017.cpp(36): #ifndef
//H 11: t_9_017.cpp(36): #ifndef true: 0
//H 10: t_9_017.cpp(38): #else
//H 10: t_9_017.cpp(44): #ifndef
//H 11: t_9_017.cpp(44): #ifndef false: 0
//H 10: t_9_017.cpp(46): #else
