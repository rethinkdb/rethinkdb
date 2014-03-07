/*=============================================================================
    Boost.Wave: A Standard compliant C++ preprocessor library
    http://www.boost.org/

    Copyright (c) 2001-2012 Hartmut Kaiser. Distributed under the Boost
    Software License, Version 1.0. (See accompanying file
    LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
=============================================================================*/

// Tests continuing scanning into the underlying input stream after expanding
// a macro, if this is appropriate

#define MACRO() X + Y NEXT
#define X 1
#define Y 2
#define NEXT() ...

//R #line 20 "t_1_021.cpp"
//R 1 + 2 ...
MACRO()()

//H 10: t_1_021.cpp(13): #define
//H 08: t_1_021.cpp(13): MACRO()=X + Y NEXT
//H 10: t_1_021.cpp(14): #define
//H 08: t_1_021.cpp(14): X=1
//H 10: t_1_021.cpp(15): #define
//H 08: t_1_021.cpp(15): Y=2
//H 10: t_1_021.cpp(16): #define
//H 08: t_1_021.cpp(16): NEXT()=...
//H 00: t_1_021.cpp(20): MACRO(), [t_1_021.cpp(13): MACRO()=X + Y NEXT]
//H 02: X + Y NEXT
//H 01: t_1_021.cpp(14): X
//H 02: 1
//H 03: 1
//H 01: t_1_021.cpp(15): Y
//H 02: 2
//H 03: 2
//H 03: 1 + 2 NEXT
//H 00: t_1_021.cpp(13): NEXT(), [t_1_021.cpp(16): NEXT()=...]
//H 02: ...
//H 03: ...
