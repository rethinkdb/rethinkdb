/*=============================================================================
    Boost.Wave: A Standard compliant C++ preprocessor library
    http://www.boost.org/

    Copyright (c) 2001-2012 Hartmut Kaiser. Distributed under the Boost
    Software License, Version 1.0. (See accompanying file
    LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
=============================================================================*/

// Tests macro expansion order.

#define A() B
#define B(x) x

//R #line 16 "t_1_005.cpp"
A()(123)     //R 123 

//H 10: t_1_005.cpp(12): #define
//H 08: t_1_005.cpp(12): A()=B
//H 10: t_1_005.cpp(13): #define
//H 08: t_1_005.cpp(13): B(x)=x
//H 00: t_1_005.cpp(16): A(), [t_1_005.cpp(12): A()=B]
//H 02: B
//H 03: B
//H 00: t_1_005.cpp(12): B(123), [t_1_005.cpp(13): B(x)=x]
//H 02: 123
//H 03: 123
