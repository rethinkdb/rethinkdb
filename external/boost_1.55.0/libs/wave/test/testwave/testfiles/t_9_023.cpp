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

#define _C_STD_BEGIN

_C_STD_BEGIN
#ifndef _M_CEE_PURE
_C_LIB_DECL
#endif

//R #line 18 "t_9_023.cpp"
//R _C_LIB_DECL

//H 10: t_9_023.cpp(14): #define
//H 08: t_9_023.cpp(14): _C_STD_BEGIN=
//H 01: t_9_023.cpp(14): _C_STD_BEGIN
//H 02: 
//H 03: _
//H 10: t_9_023.cpp(17): #ifndef
//H 11: t_9_023.cpp(17): #ifndef _M_CEE_PURE: 0
//H 10: t_9_023.cpp(19): #endif
