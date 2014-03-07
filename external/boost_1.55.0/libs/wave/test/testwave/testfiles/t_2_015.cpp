/*=============================================================================
    Boost.Wave: A Standard compliant C++ preprocessor library
    http://www.boost.org/

    Copyright (c) 2001-2012 Hartmut Kaiser. Distributed under the Boost
    Software License, Version 1.0. (See accompanying file
    LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
=============================================================================*/

// Tests, whether #if works when the expression is surrounded by parenthesis

#define WINVER 0x0500

//R #line 17 "t_2_015.cpp"
//R true
#if(WINVER >= 0x0500)
true
#endif

//H 10: t_2_015.cpp(12): #define
//H 08: t_2_015.cpp(12): WINVER=0x0500
//H 10: t_2_015.cpp(16): #if
//H 01: t_2_015.cpp(12): WINVER
//H 02: 0x0500
//H 03: 0x0500
//H 11: t_2_015.cpp(16): #if (WINVER >= 0x0500): 1
//H 10: t_2_015.cpp(18): #endif
