/*=============================================================================
    Boost.Wave: A Standard compliant C++ preprocessor library
    http://www.boost.org/

    Copyright (c) 2001-2012 Hartmut Kaiser. Distributed under the Boost
    Software License, Version 1.0. (See accompanying file
    LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
=============================================================================*/

// Tests the validity of whitespace handling during macro expansion

#define MACRO() 123

//R #line 16 "t_1_022.cpp"
//R 123
MACRO()
//R #line 19 "t_1_022.cpp"
//R 123
MACRO
()
//R #line 23 "t_1_022.cpp"
//R 123
MACRO(
)

//H 10: t_1_022.cpp(12): #define
//H 08: t_1_022.cpp(12): MACRO()=123
//H 00: t_1_022.cpp(16): MACRO(), [t_1_022.cpp(12): MACRO()=123]
//H 02: 123
//H 03: 123
//H 00: t_1_022.cpp(19): MACRO(), [t_1_022.cpp(12): MACRO()=123]
//H 02: 123
//H 03: 123
//H 00: t_1_022.cpp(23): MACRO(), [t_1_022.cpp(12): MACRO()=123]
//H 02: 123
//H 03: 123
