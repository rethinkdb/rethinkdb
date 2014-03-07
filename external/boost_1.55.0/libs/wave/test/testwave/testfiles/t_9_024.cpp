/*=============================================================================
    Boost.Wave: A Standard compliant C++ preprocessor library
    http://www.boost.org/

    Copyright (c) 2001-2013 Hartmut Kaiser. Distributed under the Boost
    Software License, Version 1.0. (See accompanying file
    LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
=============================================================================*/

// Verifies resolution of #8848: Wave driver improperly processes 0xFFFFui64 
// token

//O --long_long

#if defined(__TESTWAVE_SUPPORT_MS_EXTENSIONS__)

#define TEST 0xFFFFFui64

TEST

//R #line 19 "t_9_024.cpp"
//R 0xFFFFFui64

//H 10: t_9_024.cpp(15): #if
//H 11: t_9_024.cpp(15): #if defined(__TESTWAVE_SUPPORT_MS_EXTENSIONS__): 1
//H 10: t_9_024.cpp(17): #define
//H 08: t_9_024.cpp(17): TEST=0xFFFFFui64
//H 01: t_9_024.cpp(17): TEST
//H 02: 0xFFFFFui64
//H 03: 0xFFFFFui64
//H 10: t_9_024.cpp(33): #endif

#endif

