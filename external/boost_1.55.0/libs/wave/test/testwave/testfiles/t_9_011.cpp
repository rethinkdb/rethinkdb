/*=============================================================================
    Boost.Wave: A Standard compliant C++ preprocessor library
    http://www.boost.org/

    Copyright (c) 2001-2012 Hartmut Kaiser. Distributed under the Boost
    Software License, Version 1.0. (See accompanying file
    LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
=============================================================================*/

// Test, if additional whitespace is inserted at appropriate places.

//O --variadics

#define PRIMITIVE_CAT(a, b)     a ## b
#define PRIMITIVE_CAT3(a, b, c) a ## b ## c

//R #line 18 "t_9_011.cpp"
PRIMITIVE_CAT(1, ABC)         //R 1 ABC 
PRIMITIVE_CAT3(ABC, 1, ABC)   //R ABC1ABC 

//H 10: t_9_011.cpp(14): #define
//H 08: t_9_011.cpp(14): PRIMITIVE_CAT(a, b)=a ## b
//H 10: t_9_011.cpp(15): #define
//H 08: t_9_011.cpp(15): PRIMITIVE_CAT3(a, b, c)=a ## b ## c
//H 00: t_9_011.cpp(18): PRIMITIVE_CAT(1, ABC), [t_9_011.cpp(14): PRIMITIVE_CAT(a, b)=a ## b]
//H 02: 1ABC
//H 03: 1ABC
//H 00: t_9_011.cpp(19): PRIMITIVE_CAT3(ABC, 1, ABC), [t_9_011.cpp(15): PRIMITIVE_CAT3(a, b, c)=a ## b ## c]
//H 02: ABC1ABC
//H 03: ABC1ABC
