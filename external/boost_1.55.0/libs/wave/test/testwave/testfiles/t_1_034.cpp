/*=============================================================================
    Boost.Wave: A Standard compliant C++ preprocessor library
    http://www.boost.org/

    Copyright (c) 2001-2012 Hartmut Kaiser. Distributed under the Boost
    Software License, Version 1.0. (See accompanying file
    LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
=============================================================================*/

// Tests empty __VA_ARGS__ expansion

//O --variadics

#define MACRO1(x, ...)  x -> __VA_ARGS__
#define MACRO2(...)     __VA_ARGS__
#define STR(...)        #__VA_ARGS__

//R #line 19 "t_1_034.cpp"
MACRO1(1,)    //R 1 -> 
MACRO2(1, 2)  //R 1, 2 
STR()         //R "" 

//H 10: t_1_034.cpp(14): #define
//H 08: t_1_034.cpp(14): MACRO1(x, ...)=x -> __VA_ARGS__
//H 10: t_1_034.cpp(15): #define
//H 08: t_1_034.cpp(15): MACRO2(...)=__VA_ARGS__
//H 10: t_1_034.cpp(16): #define
//H 08: t_1_034.cpp(16): STR(...)=#__VA_ARGS__
//H 00: t_1_034.cpp(19): MACRO1(1,§), [t_1_034.cpp(14): MACRO1(x, ...)=x -> __VA_ARGS__]
//H 02: 1 -> 
//H 03: 1 ->
//H 00: t_1_034.cpp(20): MACRO2(1, 2), [t_1_034.cpp(15): MACRO2(...)=__VA_ARGS__]
//H 02: 1, 2
//H 03: 1, 2
//H 00: t_1_034.cpp(21): STR(§), [t_1_034.cpp(16): STR(...)=#__VA_ARGS__]
//H 02: ""
//H 03: ""

// boostinspect:noascii    this file needs to contain non-ASCII characters
