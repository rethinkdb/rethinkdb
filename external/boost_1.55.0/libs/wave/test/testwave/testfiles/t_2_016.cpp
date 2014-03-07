/*=============================================================================
    Boost.Wave: A Standard compliant C++ preprocessor library
    http://www.boost.org/

    Copyright (c) 2001-2012 Hartmut Kaiser. Distributed under the Boost
    Software License, Version 1.0. (See accompanying file
    LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
=============================================================================*/

// if _MSC_VER was defined through the command line, the pp expression was 
// garbled

//O -D_MSC_VER=1200

//R #line 18 "t_2_016.cpp"
//R true
#if defined (_MSC_VER) && (_MSC_VER >= 1020)
true
#endif

//H 10: t_2_016.cpp(17): #if
//H 01: <command line>(1): _MSC_VER
//H 02: 1200
//H 03: 1200
//H 11: t_2_016.cpp(17): #if defined (_MSC_VER) && (_MSC_VER >= 1020): 1
//H 10: t_2_016.cpp(19): #endif
