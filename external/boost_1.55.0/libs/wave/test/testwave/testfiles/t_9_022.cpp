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
# EMPTY define M 1

//E t_9_022.cpp(15): error: ill formed preprocessor directive: # EMPTY define M 1

