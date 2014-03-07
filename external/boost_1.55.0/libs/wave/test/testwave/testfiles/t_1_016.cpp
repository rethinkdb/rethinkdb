/*=============================================================================
    Boost.Wave: A Standard compliant C++ preprocessor library
    http://www.boost.org/

    Copyright (c) 2001-2012 Hartmut Kaiser. Distributed under the Boost
    Software License, Version 1.0. (See accompanying file
    LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
=============================================================================*/

// Tests continuing scanning into the underlying input stream after expanding
// a macro, if this is appropriate

#define A Token1 B
#define B() Token2

//R #line 18 "t_1_016.cpp"
//R Token1 Token2
A()

//H 10: t_1_016.cpp(13): #define
//H 08: t_1_016.cpp(13): A=Token1 B
//H 10: t_1_016.cpp(14): #define
//H 08: t_1_016.cpp(14): B()=Token2
//H 01: t_1_016.cpp(13): A
//H 02: Token1 B
//H 03: Token1 B
//H 00: t_1_016.cpp(13): B(), [t_1_016.cpp(14): B()=Token2]
//H 02: Token2
//H 03: Token2
