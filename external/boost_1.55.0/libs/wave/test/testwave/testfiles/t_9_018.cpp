/*=============================================================================
    Boost.Wave: A Standard compliant C++ preprocessor library
    http://www.boost.org/

    Copyright (c) 2001-2012 Hartmut Kaiser. Distributed under the Boost
    Software License, Version 1.0. (See accompanying file
    LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
=============================================================================*/

//  tests, whether macro redefinition does not crash anymore, even if the new 
//  replacement list is shorter, than the initial one.

//E t_9_018.cpp(15): warning: illegal macro redefinition: M1
#define M1 1
#define M1

//H 10: t_9_018.cpp(14): #define
//H 08: t_9_018.cpp(14): M1=1
//H 10: t_9_018.cpp(15): #define
//H 18: boost::wave::macro_handling_exception
