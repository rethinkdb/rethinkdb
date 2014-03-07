/*=============================================================================
    Boost.Wave: A Standard compliant C++ preprocessor library
    http://www.boost.org/

    Copyright (c) 2001-2012 Hartmut Kaiser. Distributed under the Boost
    Software License, Version 1.0. (See accompanying file
    LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
=============================================================================*/

// Tests macro expansion using variadic macros

//O --variadics

#define is_empty(...) is_empty_ ## __VA_ARGS__ ## other

//R #line 17 "t_1_031.cpp"
is_empty( + )     //R is_empty_+other 
is_empty( +text ) //R is_empty_+textother 

//H 10: t_1_031.cpp(14): #define
//H 08: t_1_031.cpp(14): is_empty(...)=is_empty_ ## __VA_ARGS__ ## other
//H 00: t_1_031.cpp(17): is_empty( + ), [t_1_031.cpp(14): is_empty(...)=is_empty_ ## __VA_ARGS__ ## other]
//H 02: is_empty_+other
//H 03: is_empty_+other
//H 00: t_1_031.cpp(18): is_empty( +text ), [t_1_031.cpp(14): is_empty(...)=is_empty_ ## __VA_ARGS__ ## other]
//H 02: is_empty_+textother
//H 03: is_empty_+textother
