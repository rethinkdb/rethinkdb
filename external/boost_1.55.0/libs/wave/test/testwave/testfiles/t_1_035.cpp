/*=============================================================================
    Boost.Wave: A Standard compliant C++ preprocessor library
    http://www.boost.org/

    Copyright (c) 2001-2012 Hartmut Kaiser. Distributed under the Boost
    Software License, Version 1.0. (See accompanying file
    LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
=============================================================================*/

// Tests token pasting with empty empty arguments

//O --variadics

#define PASTE1(a, b, c, d) a ## b ## c ## d
#define PASTE2(a, b, c, d) a##b##c##d

//R #line 18 "t_1_035.cpp"
PASTE1(1, ,3,4)   //R 134 
PASTE1(1,,3,4)    //R 134 
PASTE1(1, , , 4)  //R 14 
//R
PASTE2(1, ,3,4)   //R 134 
PASTE2(1,,3,4)    //R 134 
PASTE2(1, , , 4)  //R 14 

//H 10: t_1_035.cpp(14): #define
//H 08: t_1_035.cpp(14): PASTE1(a, b, c, d)=a ## b ## c ## d
//H 10: t_1_035.cpp(15): #define
//H 08: t_1_035.cpp(15): PASTE2(a, b, c, d)=a##b##c##d
//H 00: t_1_035.cpp(18): PASTE1(1, §,3,4), [t_1_035.cpp(14): PASTE1(a, b, c, d)=a ## b ## c ## d]
//H 02: 134
//H 03: 134
//H 00: t_1_035.cpp(19): PASTE1(1,§,3,4), [t_1_035.cpp(14): PASTE1(a, b, c, d)=a ## b ## c ## d]
//H 02: 134
//H 03: 134
//H 00: t_1_035.cpp(20): PASTE1(1, §, §, 4), [t_1_035.cpp(14): PASTE1(a, b, c, d)=a ## b ## c ## d]
//H 02: 14
//H 03: 14
//H 00: t_1_035.cpp(22): PASTE2(1, §,3,4), [t_1_035.cpp(15): PASTE2(a, b, c, d)=a##b##c##d]
//H 02: 134
//H 03: 134
//H 00: t_1_035.cpp(23): PASTE2(1,§,3,4), [t_1_035.cpp(15): PASTE2(a, b, c, d)=a##b##c##d]
//H 02: 134
//H 03: 134
//H 00: t_1_035.cpp(24): PASTE2(1, §, §, 4), [t_1_035.cpp(15): PASTE2(a, b, c, d)=a##b##c##d]
//H 02: 14
//H 03: 14

// boostinspect:noascii    this file needs to contain non-ASCII characters
