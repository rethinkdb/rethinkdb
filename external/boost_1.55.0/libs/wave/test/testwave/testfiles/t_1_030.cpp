/*=============================================================================
    Boost.Wave: A Standard compliant C++ preprocessor library
    http://www.boost.org/

    Copyright (c) 2001-2012 Hartmut Kaiser. Distributed under the Boost
    Software License, Version 1.0. (See accompanying file
    LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
=============================================================================*/

// Tests, whether an empty macro prevents another macro from expansion

#define EMPTY()
#define SCAN(x) x
#define MACRO(x) (x)

//R #line 17 "t_1_030.cpp"
SCAN( MACRO EMPTY() )(1)    //R (1) 

//H 10: t_1_030.cpp(12): #define
//H 08: t_1_030.cpp(12): EMPTY()=
//H 10: t_1_030.cpp(13): #define
//H 08: t_1_030.cpp(13): SCAN(x)=x
//H 10: t_1_030.cpp(14): #define
//H 08: t_1_030.cpp(14): MACRO(x)=(x)
//H 00: t_1_030.cpp(17): SCAN( MACRO EMPTY() ), [t_1_030.cpp(13): SCAN(x)=x]
//H 00: t_1_030.cpp(17): EMPTY(), [t_1_030.cpp(12): EMPTY()=]
//H 02: 
//H 03: _
//H 02:  MACRO  
//H 03: MACRO
//H 00: t_1_030.cpp(17): MACRO(1), [t_1_030.cpp(14): MACRO(x)=(x)]
//H 02: (1)
//H 03: (1)
