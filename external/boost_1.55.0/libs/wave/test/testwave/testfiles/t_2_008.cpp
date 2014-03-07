/*=============================================================================
    Boost.Wave: A Standard compliant C++ preprocessor library
    http://www.boost.org/

    Copyright (c) 2001-2012 Hartmut Kaiser. Distributed under the Boost
    Software License, Version 1.0. (See accompanying file
    LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
=============================================================================*/

// Tests #include statements with macros as arguments

//R
//E t_2_008.cpp(15): error: could not find include file: some_include_file.h
#define INCLUDE_FILE "some_include_file.h"
#include INCLUDE_FILE

//H 10: t_2_008.cpp(14): #define
//H 08: t_2_008.cpp(14): INCLUDE_FILE="some_include_file.h"
//H 10: t_2_008.cpp(15): #include
//H 01: t_2_008.cpp(14): INCLUDE_FILE
//H 02: "some_include_file.h"
//H 03: "some_include_file.h"
//H 04: "some_include_file.h"
//H 18: boost::wave::preprocess_exception
