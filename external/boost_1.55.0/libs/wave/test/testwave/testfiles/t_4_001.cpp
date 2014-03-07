/*=============================================================================
    Boost.Wave: A Standard compliant C++ preprocessor library
    http://www.boost.org/

    Copyright (c) 2001-2012 Hartmut Kaiser. Distributed under the Boost
    Software License, Version 1.0. (See accompanying file
    LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
=============================================================================*/

// Tests, whether integer arithmetic results get truncated correctly

//R #line 15 "t_4_001.cpp"
//R true
#if 1 / 10 == 0
true
#else
false
#endif

//H 10: t_4_001.cpp(14): #if
//H 11: t_4_001.cpp(14): #if 1 / 10 == 0: 1
//H 10: t_4_001.cpp(16): #else
