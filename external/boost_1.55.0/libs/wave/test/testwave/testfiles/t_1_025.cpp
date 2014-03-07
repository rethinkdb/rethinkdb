/*=============================================================================
    Boost.Wave: A Standard compliant C++ preprocessor library
    http://www.boost.org/

    Copyright (c) 2001-2012 Hartmut Kaiser. Distributed under the Boost
    Software License, Version 1.0. (See accompanying file
    LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
=============================================================================*/

// Tests, if macro expansion eats up follow up tokens under certain conditions
// (which it shouldn't).

#define SCAN(x) x

#define BUG BUG_2
#define BUG_2

//R #line 19 "t_1_025.cpp"
SCAN(BUG) 1 2 3 4 5   //R 1 2 3 4 5 

//H 10: t_1_025.cpp(13): #define
//H 08: t_1_025.cpp(13): SCAN(x)=x
//H 10: t_1_025.cpp(15): #define
//H 08: t_1_025.cpp(15): BUG=BUG_2
//H 10: t_1_025.cpp(16): #define
//H 08: t_1_025.cpp(16): BUG_2=
//H 00: t_1_025.cpp(19): SCAN(BUG), [t_1_025.cpp(13): SCAN(x)=x]
//H 01: t_1_025.cpp(15): BUG
//H 02: BUG_2
//H 01: t_1_025.cpp(16): BUG_2
//H 02: 
//H 03: _
//H 03: _
//H 02: 
//H 03: _
