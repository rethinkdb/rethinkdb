/*=============================================================================
    Boost.Wave: A Standard compliant C++ preprocessor library
    http://www.boost.org/

    Copyright (c) 2001-2012 Hartmut Kaiser. Distributed under the Boost
    Software License, Version 1.0. (See accompanying file
    LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
=============================================================================*/

// Test if invalid or not allowed universal characters are rejected

#define \u00ff ...

//R #line 16 "t_9_004.cpp"
//R ...
\u00ff

//E t_9_004.cpp(19): error: a universal character name cannot designate a character in the basic character set: \u0061
#define \u0061 weird // 0x61 == 'a'

\u0061

//H 10: t_9_004.cpp(12): #define
//H 08: t_9_004.cpp(12): \u00ff=...
//H 01: t_9_004.cpp(12): \u00ff
//H 02: ...
//H 03: ...
//H 18: boost::wave::lexing_exception
