/*=============================================================================
    Boost.Wave: A Standard compliant C++ preprocessor library
    http://www.boost.org/

    Copyright (c) 2001-2012 Hartmut Kaiser. Distributed under the Boost
    Software License, Version 1.0. (See accompanying file
    LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
=============================================================================*/

// Test error reporting for non-unique parameter names

//E t_1_026.cpp(13): error: duplicate macro parameter name: x
#define MACRO(x, x) x
           // ^  ^ this is illegal

MACRO(1, 2)

//H 10: t_1_026.cpp(13): #define
//H 18: boost::wave::macro_handling_exception
