/*=============================================================================
    Boost.Wave: A Standard compliant C++ preprocessor library
    http://www.boost.org/

    Copyright (c) 2001-2012 Hartmut Kaiser. Distributed under the Boost
    Software License, Version 1.0. (See accompanying file
    LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
=============================================================================*/

// Tests the concatination operator in object like macros

#define OBJECT a ## b

//R #line 15 "t_1_036.cpp"
OBJECT      //R ab 

//H 10: t_1_036.cpp(12): #define
//H 08: t_1_036.cpp(12): OBJECT=a ## b
//H 01: t_1_036.cpp(12): OBJECT
//H 02: ab
//H 03: ab
