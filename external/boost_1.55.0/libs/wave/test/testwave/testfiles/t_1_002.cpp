/*=============================================================================
    Boost.Wave: A Standard compliant C++ preprocessor library
    http://www.boost.org/

    Copyright (c) 2001-2012 Hartmut Kaiser. Distributed under the Boost
    Software License, Version 1.0. (See accompanying file
    LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
=============================================================================*/

//O -DTEST
#if defined(TEST)
#define ABC() 1
#endif

//R #line 16 "t_1_002.cpp"
ABC()    //R 1 

//H 10: t_1_002.cpp(11): #if
//H 11: t_1_002.cpp(11): #if defined(TEST): 1
//H 10: t_1_002.cpp(12): #define
//H 08: t_1_002.cpp(12): ABC()=1
//H 10: t_1_002.cpp(13): #endif
//H 00: t_1_002.cpp(16): ABC(), [t_1_002.cpp(12): ABC()=1]
//H 02: 1
//H 03: 1
