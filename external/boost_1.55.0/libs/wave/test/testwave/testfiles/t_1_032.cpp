/*=============================================================================
    Boost.Wave: A Standard compliant C++ preprocessor library
    http://www.boost.org/

    Copyright (c) 2001-2012 Hartmut Kaiser. Distributed under the Boost
    Software License, Version 1.0. (See accompanying file
    LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
=============================================================================*/

// Test partial macro evaluation using varidic macros

//O --variadics

#define cat(...) cat_i(__VA_ARGS__,,,,,)
#define cat_i(a, b, c, d, e, ...) \
    a ## b ## c ## d ## e \
    /**/

#define primitive_cat(a, b) a ## b

#define partial_cat(x, y) cat(partial_cat_, x, y)

#define partial_cat_00(a, b) partial_cat_f(, ## a, b ## ,)
#define partial_cat_01(a, b) partial_cat_f(, ## a, b    ,)
#define partial_cat_10(a, b) partial_cat_f(,    a, b ## ,)
#define partial_cat_11(a, b) partial_cat_f(,    a, b    ,)

#define partial_cat_f(a, b, c, d) b ## c

#define X Token1
#define Y Token2

//R #line 34 "t_1_032.cpp"
partial_cat(0, 0)(X, Y)     //R XY 
partial_cat(0, 1)(X, Y)     //R XToken2 
partial_cat(1, 0)(X, Y)     //R Token1Y 
partial_cat(1, 1)(X, Y)     //R Token1Token2 
