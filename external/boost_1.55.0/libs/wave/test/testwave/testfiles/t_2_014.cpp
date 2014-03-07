/*=============================================================================
    Boost.Wave: A Standard compliant C++ preprocessor library
    http://www.boost.org/

    Copyright (c) 2001-2012 Hartmut Kaiser. Distributed under the Boost
    Software License, Version 1.0. (See accompanying file
    LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
=============================================================================*/

// Tests, whether alternative tokens are interpreted inside of conditional 
// expressions

//R #line 16 "t_2_014.cpp"
//R true
#if compl 1
true
#else
false
#endif

//R #line 26 "t_2_014.cpp"
//R true
#if not 1
false
#else
true
#endif

//R #line 32 "t_2_014.cpp"
//R true
#if 1 or 2
true
#else
false
#endif

//R #line 40 "t_2_014.cpp"
//R true
#if 1 and 2
true
#else
false
#endif

//R #line 50 "t_2_014.cpp"
//R true
#if not 1
false
#else
true
#endif

//R #line 56 "t_2_014.cpp"
//R true
#if 1 xor 2
true
#else
false
#endif

//R #line 66 "t_2_014.cpp"
//R true
#if 1 bitand 2
false
#else
true
#endif

//R #line 72 "t_2_014.cpp"
//R true
#if 1 bitor 2
true
#else
false
#endif

//R #line 80 "t_2_014.cpp"
//R true
#if 1 not_eq 2
true
#else
false
#endif

//H 10: t_2_014.cpp(15): #if
//H 11: t_2_014.cpp(15): #if compl 1: 1
//H 10: t_2_014.cpp(17): #else
//H 10: t_2_014.cpp(23): #if
//H 11: t_2_014.cpp(23): #if not 1: 0
//H 10: t_2_014.cpp(27): #endif
//H 10: t_2_014.cpp(31): #if
//H 11: t_2_014.cpp(31): #if 1 or 2: 1
//H 10: t_2_014.cpp(33): #else
//H 10: t_2_014.cpp(39): #if
//H 11: t_2_014.cpp(39): #if 1 and 2: 1
//H 10: t_2_014.cpp(41): #else
//H 10: t_2_014.cpp(47): #if
//H 11: t_2_014.cpp(47): #if not 1: 0
//H 10: t_2_014.cpp(51): #endif
//H 10: t_2_014.cpp(55): #if
//H 11: t_2_014.cpp(55): #if 1 xor 2: 1
//H 10: t_2_014.cpp(57): #else
//H 10: t_2_014.cpp(63): #if
//H 11: t_2_014.cpp(63): #if 1 bitand 2: 0
//H 10: t_2_014.cpp(67): #endif
//H 10: t_2_014.cpp(71): #if
//H 11: t_2_014.cpp(71): #if 1 bitor 2: 1
//H 10: t_2_014.cpp(73): #else
//H 10: t_2_014.cpp(79): #if
//H 11: t_2_014.cpp(79): #if 1 not_eq 2: 1
//H 10: t_2_014.cpp(81): #else
