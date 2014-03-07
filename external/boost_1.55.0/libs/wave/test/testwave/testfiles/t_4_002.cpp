/*=============================================================================
    Boost.Wave: A Standard compliant C++ preprocessor library
    http://www.boost.org/

    Copyright (c) 2001-2012 Hartmut Kaiser. Distributed under the Boost
    Software License, Version 1.0. (See accompanying file
    LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
=============================================================================*/

// Tests, whether ecursively defined macros get expanded correctly in the 
// context of an #if expression

#define C C

//R #line 18 "t_4_002.cpp"
//R true
#if !C
true
#endif

//H 10: t_4_002.cpp(13): #define
//H 08: t_4_002.cpp(13): C=C
//H 10: t_4_002.cpp(17): #if
//H 01: t_4_002.cpp(13): C
//H 02: C
//H 03: C
//H 11: t_4_002.cpp(17): #if !C: 1
//H 10: t_4_002.cpp(19): #endif
