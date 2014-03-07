/*=============================================================================
    Boost.Wave: A Standard compliant C++ preprocessor library
    http://www.boost.org/

    Copyright (c) 2001-2012 Hartmut Kaiser. Distributed under the Boost
    Software License, Version 1.0. (See accompanying file
    LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
=============================================================================*/

// syntax for defined operator
// is either:
//
//    defined X
//
// -or-
//
//    defined(X)
//
// i.e. the parenthesis are optional (ala sizeof)

#define X

//R #line 26 "t_9_006.cpp"
//R "X" is defined.
#if defined(X)
"X" is defined.
#else
"X" is not defined.
#endif

//R #line 34 "t_9_006.cpp"
//R "X" is defined.
#if defined X
"X" is defined.
#else
"X" is not defined.
#endif

//H 10: t_9_006.cpp(21): #define
//H 08: t_9_006.cpp(21): X=
//H 10: t_9_006.cpp(25): #if
//H 11: t_9_006.cpp(25): #if defined(X): 1
//H 10: t_9_006.cpp(27): #else
//H 10: t_9_006.cpp(33): #if
//H 11: t_9_006.cpp(33): #if defined X: 1
//H 10: t_9_006.cpp(35): #else
