/*=============================================================================
    Boost.Wave: A Standard compliant C++ preprocessor library
    http://www.boost.org/

    Copyright (c) 2001-2012 Hartmut Kaiser. Distributed under the Boost
    Software License, Version 1.0. (See accompanying file
    LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
=============================================================================*/

// Verifies that preprocessing directives are properly recognized only if
// the '#' is really the first character on a line before macro expansion.
// See http://www.open-std.org/jtc1/sc22/wg14/docs/rr/dr_144.html.

#define EMPTY
EMPTY # define M 1

//R #line 15 "t_9_021.cpp"
//R #define M 1

//H 10: t_9_021.cpp(14): #define
//H 08: t_9_021.cpp(14): EMPTY=
//H 01: t_9_021.cpp(14): EMPTY
//H 02: 
//H 03: _
