/*=============================================================================
    Boost.Wave: A Standard compliant C++ preprocessor library
    http://www.boost.org/

    Copyright (c) 2001-2012 Hartmut Kaiser. Distributed under the Boost
    Software License, Version 1.0. (See accompanying file
    LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
=============================================================================*/

// Check if regression reported by ticket #1752 has been fixed

//R #line 17 "t_2_018.cpp"
//R "__FILE__ is defined"
#ifndef __FILE__
"No __FILE__"
#else
"__FILE__ is defined"
#endif

//H 10: t_2_018.cpp(14): #ifndef
//H 11: t_2_018.cpp(14): #ifndef __FILE__: 1
//H 10: t_2_018.cpp(18): #endif
