/*=============================================================================
    Boost.Wave: A Standard compliant C++ preprocessor library
    http://www.boost.org/

    Copyright (c) 2001-2012 Hartmut Kaiser. Distributed under the Boost
    Software License, Version 1.0. (See accompanying file
    LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
=============================================================================*/

// Tests the handling of placeholder tokens, which have to be inserted, when
// some macro expands into nothing (certainly these have to be ignored 
// afterwards :-)

#define NIL

#define A B NIL
#define B() anything

//R #line 21 "t_1_020.cpp"
//R B() 
A()   // not 'anything'!

//H 10: t_1_020.cpp(14): #define
//H 08: t_1_020.cpp(14): NIL=
//H 10: t_1_020.cpp(16): #define
//H 08: t_1_020.cpp(16): A=B NIL
//H 10: t_1_020.cpp(17): #define
//H 08: t_1_020.cpp(17): B()=anything
//H 01: t_1_020.cpp(16): A
//H 02: B NIL
//H 01: t_1_020.cpp(14): NIL
//H 02: 
//H 03: _
//H 03: B_
