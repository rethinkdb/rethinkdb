/*=============================================================================
    Boost.Wave: A Standard compliant C++ preprocessor library
    http://www.boost.org/

    Copyright (c) 2001-2012 Hartmut Kaiser. Distributed under the Boost
    Software License, Version 1.0. (See accompanying file
    LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
=============================================================================*/

// The result of macro expansion must be re-tokenized to find header-name 
// tokens of either <file> or "file"

//O -S.

#if !defined(FILE_002_009_CPP)    // avoid #include recursion
#define FILE_002_009_CPP

#define SYSTEM_HEADER <t_2_009.cpp>
#define USER_HEADER   "t_2_009.cpp"

//R #line 23 "t_2_009.cpp"
//R including <t_2_009.cpp>
including <t_2_009.cpp>
#include SYSTEM_HEADER

//R #line 28 "t_2_009.cpp"
//R including "t_2_009.cpp"
including "t_2_009.cpp"
#include USER_HEADER

#endif // FILE_002_009_CPP

//H 10: t_2_009.cpp(15): #if
//H 11: t_2_009.cpp(15): #if !defined(FILE_002_009_CPP)    : 1
//H 10: t_2_009.cpp(16): #define
//H 08: t_2_009.cpp(16): FILE_002_009_CPP=
//H 10: t_2_009.cpp(18): #define
//H 08: t_2_009.cpp(18): SYSTEM_HEADER=<t_2_009.cpp>
//H 10: t_2_009.cpp(19): #define
//H 08: t_2_009.cpp(19): USER_HEADER="t_2_009.cpp"
//H 10: t_2_009.cpp(24): #include 
//H 01: t_2_009.cpp(18): SYSTEM_HEADER
//H 02: <t_2_009.cpp>
//H 03: <t_2_009.cpp>
//H 04: <t_2_009.cpp>
//H 05: $B(t_2_009.cpp) ($B(t_2_009.cpp))
//H 10: t_2_009.cpp(15): #if
//H 11: t_2_009.cpp(15): #if !defined(FILE_002_009_CPP)    : 0
//H 06: 
//H 19: $B(t_2_009.cpp): FILE_002_009_CPP
//H 10: t_2_009.cpp(29): #include 
//H 01: t_2_009.cpp(19): USER_HEADER
//H 02: "t_2_009.cpp"
//H 03: "t_2_009.cpp"
//H 04: "t_2_009.cpp"
//H 10: t_2_009.cpp(31): #endif
