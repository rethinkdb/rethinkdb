/*=============================================================================
    Boost.Wave: A Standard compliant C++ preprocessor library
    http://www.boost.org/

    Copyright (c) 2001-2012 Hartmut Kaiser. Distributed under the Boost
    Software License, Version 1.0. (See accompanying file
    LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
=============================================================================*/

// Tests the correctness of parameter replacement, if the parameter is
// adjacent to an operator '##'.

#define CONCAT(a, b) PRIMITIVE_CONCAT(a, b)
#define PRIMITIVE_CONCAT(a, b) a ## b

//R #line 18 "t_1_006.cpp"
//R 123
CONCAT(1, PRIMITIVE_CONCAT(2, 3))
//R #line 21 "t_1_006.cpp"
//R 123
CONCAT(1, CONCAT(2, 3))
// E t_1_006.cpp(23): error: pasting the following two tokens does not give a valid preprocessing token: "1" and "CONCAT"
//R #line 25 "t_1_006.cpp"
//R 1 CONCAT(2, 3)
PRIMITIVE_CONCAT(1, CONCAT(2, 3))

//H 10: t_1_006.cpp(13): #define
//H 08: t_1_006.cpp(13): CONCAT(a, b)=PRIMITIVE_CONCAT(a, b)
//H 10: t_1_006.cpp(14): #define
//H 08: t_1_006.cpp(14): PRIMITIVE_CONCAT(a, b)=a ## b
//H 00: t_1_006.cpp(18): CONCAT(1, PRIMITIVE_CONCAT(2, 3)), [t_1_006.cpp(13): CONCAT(a, b)=PRIMITIVE_CONCAT(a, b)]
//H 00: t_1_006.cpp(18): PRIMITIVE_CONCAT(2, 3), [t_1_006.cpp(14): PRIMITIVE_CONCAT(a, b)=a ## b]
//H 02: 23
//H 03: 23
//H 02: PRIMITIVE_CONCAT(1,  23)
//H 00: t_1_006.cpp(13): PRIMITIVE_CONCAT(1, 23), [t_1_006.cpp(14): PRIMITIVE_CONCAT(a, b)=a ## b]
//H 02: 123
//H 03: 123
//H 03: 123
//H 00: t_1_006.cpp(21): CONCAT(1, CONCAT(2, 3)), [t_1_006.cpp(13): CONCAT(a, b)=PRIMITIVE_CONCAT(a, b)]
//H 00: t_1_006.cpp(21): CONCAT(2, 3), [t_1_006.cpp(13): CONCAT(a, b)=PRIMITIVE_CONCAT(a, b)]
//H 02: PRIMITIVE_CONCAT(2,  3)
//H 00: t_1_006.cpp(13): PRIMITIVE_CONCAT(2, 3), [t_1_006.cpp(14): PRIMITIVE_CONCAT(a, b)=a ## b]
//H 02: 23
//H 03: 23
//H 03: 23
//H 02: PRIMITIVE_CONCAT(1,  23)
//H 00: t_1_006.cpp(13): PRIMITIVE_CONCAT(1, 23), [t_1_006.cpp(14): PRIMITIVE_CONCAT(a, b)=a ## b]
//H 02: 123
//H 03: 123
//H 03: 123
//H 00: t_1_006.cpp(25): PRIMITIVE_CONCAT(1, CONCAT(2, 3)), [t_1_006.cpp(14): PRIMITIVE_CONCAT(a, b)=a ## b]
//H 02: 1CONCAT(2, 3)
//H 03: 1CONCAT(2, 3)
