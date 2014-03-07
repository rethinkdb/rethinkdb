/*=============================================================================
    Boost.Wave: A Standard compliant C++ preprocessor library
    http://www.boost.org/

    Copyright (c) 2001-2012 Hartmut Kaiser. Distributed under the Boost
    Software License, Version 1.0. (See accompanying file
    LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
=============================================================================*/

// Tests macro expansion order in conjunction with the need to skip expansion 
// of the same macro as it is currently expanded.

#define CONCAT(a, b) a ## b
#define CONCAT_INDIRECT() CONCAT

//R #line 18 "t_1_003.cpp"
//R CONCAT(1, 2)
CONCAT(CON, CAT)(1, 2)
//R #line 21 "t_1_003.cpp"
//R CONCAT(1, 2)
CONCAT(CON, CAT(1, 2))
//R #line 24 "t_1_003.cpp"
//R 12
CONCAT(CONCAT_, INDIRECT)()(1, 2)
//R #line 27 "t_1_003.cpp"
//R CONCAT(1, 2)
CONCAT(CONCAT_, INDIRECT())(1, 2)

//R #line 31 "t_1_003.cpp"
//R 1 CONCAT(2, 3)
CONCAT(1, CONCAT(2, 3))

//H 10: t_1_003.cpp(13): #define
//H 08: t_1_003.cpp(13): CONCAT(a, b)=a ## b
//H 10: t_1_003.cpp(14): #define
//H 08: t_1_003.cpp(14): CONCAT_INDIRECT()=CONCAT
//H 00: t_1_003.cpp(18): CONCAT(CON, CAT), [t_1_003.cpp(13): CONCAT(a, b)=a ## b]
//H 02: CONCAT
//H 03: CONCAT
//H 00: t_1_003.cpp(21): CONCAT(CON, CAT(1, 2)), [t_1_003.cpp(13): CONCAT(a, b)=a ## b]
//H 02: CONCAT(1, 2)
//H 03: CONCAT(1, 2)
//H 00: t_1_003.cpp(24): CONCAT(CONCAT_, INDIRECT), [t_1_003.cpp(13): CONCAT(a, b)=a ## b]
//H 02: CONCAT_INDIRECT
//H 03: CONCAT_INDIRECT
//H 00: t_1_003.cpp(24): CONCAT_INDIRECT(), [t_1_003.cpp(14): CONCAT_INDIRECT()=CONCAT]
//H 02: CONCAT
//H 03: CONCAT
//H 00: t_1_003.cpp(14): CONCAT(1, 2), [t_1_003.cpp(13): CONCAT(a, b)=a ## b]
//H 02: 12
//H 03: 12
//H 00: t_1_003.cpp(27): CONCAT(CONCAT_, INDIRECT()), [t_1_003.cpp(13): CONCAT(a, b)=a ## b]
//H 02: CONCAT_INDIRECT()
//H 00: t_1_003.cpp(27): CONCAT_INDIRECT(), [t_1_003.cpp(14): CONCAT_INDIRECT()=CONCAT]
//H 02: CONCAT
//H 03: CONCAT
//H 03: CONCAT
//H 00: t_1_003.cpp(31): CONCAT(1, CONCAT(2, 3)), [t_1_003.cpp(13): CONCAT(a, b)=a ## b]
//H 02: 1CONCAT(2, 3)
//H 03: 1CONCAT(2, 3)
