/*=============================================================================
    Boost.Wave: A Standard compliant C++ preprocessor library
    http://www.boost.org/

    Copyright (c) 2001-2012 Hartmut Kaiser. Distributed under the Boost
    Software License, Version 1.0. (See accompanying file
    LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
=============================================================================*/

    /*

      Expose bug

    */

#if 1
//R #line 18 "t_9_001.cpp"
    void exposed() {}   //R void exposed() {} 
#endif

//H 10: t_9_001.cpp(16): #if
//H 11: t_9_001.cpp(16): #if 1: 1
//H 10: t_9_001.cpp(19): #endif
