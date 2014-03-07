/*=============================================================================
    Boost.Wave: A Standard compliant C++ preprocessor library
    http://www.boost.org/

    Copyright (c) 2001-2012 Hartmut Kaiser. Distributed under the Boost
    Software License, Version 1.0. (See accompanying file
    LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
=============================================================================*/

// Tests error reporting of C99 features in C++ mode (without variadics)

#define MACRO(a, b, c)    a ## b ## c

//E t_9_013.cpp(15): warning: empty macro arguments are not supported in pure C++ mode, use variadics mode to allow these: MACRO
MACRO(1,, 3)

//H 10: t_9_013.cpp(12): #define
//H 08: t_9_013.cpp(12): MACRO(a, b, c)=a ## b ## c
//H 18: boost::wave::preprocess_exception
