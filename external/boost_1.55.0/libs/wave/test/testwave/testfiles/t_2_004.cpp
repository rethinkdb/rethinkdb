/*=============================================================================
    Boost.Wave: A Standard compliant C++ preprocessor library
    http://www.boost.org/

    Copyright (c) 2001-2012 Hartmut Kaiser. Distributed under the Boost
    Software License, Version 1.0. (See accompanying file
    LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
=============================================================================*/

// Tests #line functionality with macro expansion required

#define LINE_NO 5
#define FILE_NAME "a_nonexisting_file.cpp"
#line LINE_NO FILE_NAME 

//R
//E a_nonexisting_file.cpp(8): fatal error: encountered #error directive or #pragma wave stop(): This error should occur at line 8 of "a_nonexisting_file.cpp"
#error This error should occur at line 8 of "a_nonexisting_file.cpp"

//H 10: t_2_004.cpp(12): #define
//H 08: t_2_004.cpp(12): LINE_NO=5
//H 10: t_2_004.cpp(13): #define
//H 08: t_2_004.cpp(13): FILE_NAME="a_nonexisting_file.cpp"
//H 10: t_2_004.cpp(14): #line
//H 01: t_2_004.cpp(12): LINE_NO
//H 02: 5
//H 03: 5
//H 01: t_2_004.cpp(13): FILE_NAME
//H 02: "a_nonexisting_file.cpp"
//H 03: "a_nonexisting_file.cpp"
//H 17: 5 "a_nonexisting_file.cpp" (5, "a_nonexisting_file.cpp")
//H 10: a_nonexisting_file.cpp(8): #error
//H 16: This error should occur at line 8 of "a_nonexisting_file.cpp"
//H 18: boost::wave::preprocess_exception
