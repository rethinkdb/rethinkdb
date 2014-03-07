/*=============================================================================
    Boost.Wave: A Standard compliant C++ preprocessor library
    http://www.boost.org/

    Copyright (c) 2001-2012 Hartmut Kaiser. Distributed under the Boost
    Software License, Version 1.0. (See accompanying file
    LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
=============================================================================*/

// Tests error reporting for missing #if

//E t_2_013.cpp(20): error: detected at least one missing #endif directive
#if 1
#else

//H 10: t_2_013.cpp(13): #if
//H 11: t_2_013.cpp(13): #if 1: 1
//H 10: t_2_013.cpp(14): #else
//H 18: boost::wave::preprocess_exception
