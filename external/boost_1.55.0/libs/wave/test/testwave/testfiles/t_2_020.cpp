/*=============================================================================
    Boost.Wave: A Standard compliant C++ preprocessor library
    http://www.boost.org/

    Copyright (c) 2001-2012 Hartmut Kaiser. Distributed under the Boost
    Software License, Version 1.0. (See accompanying file
    LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
=============================================================================*/

// check called hooks during conditional preprocessing
//O --skipped_token_hooks

#ifdef FOO
int foo;
#else
float foo;
#endif

#if defined BAR
int bar;
#elif defined BAR2
float bar;
#endif

#define BAZ
#ifdef BAZ
int baz;
#else
float baz;
#endif

#if defined BAZ
int baz1;
#elif defined BAZ
float baz1;
#endif

#ifndef BAZ
int baz2;
#else
float baz2;
#endif

#if 1
int w1;
#elif 1
int w2;
#else
int w3;
#endif

#if 0
int x1;
#elif 1
int x2;
#else
int x3;
#endif

#if 1
int y1;
#elif 0
int y2;
#else
int y3;
#endif

#if 0
int z1;
#elif 0
int z2;
#else
int z3;
#endif

//R #line 16 "t_2_020.cpp"
//R float foo;
//R #line 27 "t_2_020.cpp"
//R int baz;
//R #line 33 "t_2_020.cpp"
//R int baz1;
//R #line 41 "t_2_020.cpp"
//R float baz2;
//R #line 45 "t_2_020.cpp"
//R int w1;
//R #line 55 "t_2_020.cpp"
//R int x2;
//R #line 61 "t_2_020.cpp"
//R int y1;
//R #line 73 "t_2_020.cpp"
//R int z3;

//H 10: t_2_020.cpp(13): #ifdef
//H 11: t_2_020.cpp(13): #ifdef FOO: 0
//H 12: t_2_020.cpp(13): >\n<
//H 12: t_2_020.cpp(14): >int<
//H 12: t_2_020.cpp(14): > <
//H 12: t_2_020.cpp(14): >foo<
//H 12: t_2_020.cpp(14): >;<
//H 12: t_2_020.cpp(14): >\n<
//H 12: t_2_020.cpp(15): >#else<
//H 12: t_2_020.cpp(15): >\n<
//H 10: t_2_020.cpp(17): #endif
//H 12: t_2_020.cpp(17): >#endif<
//H 12: t_2_020.cpp(17): >\n<
//H 10: t_2_020.cpp(19): #if
//H 12: t_2_020.cpp(19): > <
//H 11: t_2_020.cpp(19): #if defined BAR: 0
//H 12: t_2_020.cpp(19): >\n<
//H 12: t_2_020.cpp(20): >int<
//H 12: t_2_020.cpp(20): > <
//H 12: t_2_020.cpp(20): >bar<
//H 12: t_2_020.cpp(20): >;<
//H 12: t_2_020.cpp(20): >\n<
//H 10: t_2_020.cpp(21): #elif
//H 12: t_2_020.cpp(21): > <
//H 11: t_2_020.cpp(21): #elif defined BAR2: 0
//H 12: t_2_020.cpp(21): >\n<
//H 12: t_2_020.cpp(22): >float<
//H 12: t_2_020.cpp(22): > <
//H 12: t_2_020.cpp(22): >bar<
//H 12: t_2_020.cpp(22): >;<
//H 12: t_2_020.cpp(22): >\n<
//H 12: t_2_020.cpp(23): >#endif<
//H 12: t_2_020.cpp(23): >\n<
//H 10: t_2_020.cpp(25): #define
//H 08: t_2_020.cpp(25): BAZ=
//H 12: t_2_020.cpp(25): >\n<
//H 10: t_2_020.cpp(26): #ifdef
//H 11: t_2_020.cpp(26): #ifdef BAZ: 1
//H 12: t_2_020.cpp(26): >\n<
//H 10: t_2_020.cpp(28): #else
//H 12: t_2_020.cpp(28): >#else<
//H 12: t_2_020.cpp(28): >\n<
//H 12: t_2_020.cpp(29): >float<
//H 12: t_2_020.cpp(29): > <
//H 12: t_2_020.cpp(29): >baz<
//H 12: t_2_020.cpp(29): >;<
//H 12: t_2_020.cpp(29): >\n<
//H 12: t_2_020.cpp(30): >#endif<
//H 12: t_2_020.cpp(30): >\n<
//H 10: t_2_020.cpp(32): #if
//H 12: t_2_020.cpp(32): > <
//H 11: t_2_020.cpp(32): #if defined BAZ: 1
//H 12: t_2_020.cpp(32): >\n<
//H 10: t_2_020.cpp(34): #elif
//H 12: t_2_020.cpp(34): > <
//H 12: t_2_020.cpp(34): >defined<
//H 12: t_2_020.cpp(34): > <
//H 12: t_2_020.cpp(34): >BAZ<
//H 12: t_2_020.cpp(34): >\n<
//H 12: t_2_020.cpp(35): >float<
//H 12: t_2_020.cpp(35): > <
//H 12: t_2_020.cpp(35): >baz1<
//H 12: t_2_020.cpp(35): >;<
//H 12: t_2_020.cpp(35): >\n<
//H 12: t_2_020.cpp(36): >#endif<
//H 12: t_2_020.cpp(36): >\n<
//H 10: t_2_020.cpp(38): #ifndef
//H 11: t_2_020.cpp(38): #ifndef BAZ: 1
//H 12: t_2_020.cpp(38): >\n<
//H 12: t_2_020.cpp(39): >int<
//H 12: t_2_020.cpp(39): > <
//H 12: t_2_020.cpp(39): >baz2<
//H 12: t_2_020.cpp(39): >;<
//H 12: t_2_020.cpp(39): >\n<
//H 12: t_2_020.cpp(40): >#else<
//H 12: t_2_020.cpp(40): >\n<
//H 10: t_2_020.cpp(42): #endif
//H 12: t_2_020.cpp(42): >#endif<
//H 12: t_2_020.cpp(42): >\n<
//H 10: t_2_020.cpp(44): #if
//H 12: t_2_020.cpp(44): > <
//H 11: t_2_020.cpp(44): #if 1: 1
//H 12: t_2_020.cpp(44): >\n<
//H 10: t_2_020.cpp(46): #elif
//H 12: t_2_020.cpp(46): > <
//H 12: t_2_020.cpp(46): >1<
//H 12: t_2_020.cpp(46): >\n<
//H 12: t_2_020.cpp(47): >int<
//H 12: t_2_020.cpp(47): > <
//H 12: t_2_020.cpp(47): >w2<
//H 12: t_2_020.cpp(47): >;<
//H 12: t_2_020.cpp(47): >\n<
//H 12: t_2_020.cpp(48): >#else<
//H 12: t_2_020.cpp(48): >\n<
//H 12: t_2_020.cpp(49): >int<
//H 12: t_2_020.cpp(49): > <
//H 12: t_2_020.cpp(49): >w3<
//H 12: t_2_020.cpp(49): >;<
//H 12: t_2_020.cpp(49): >\n<
//H 12: t_2_020.cpp(50): >#endif<
//H 12: t_2_020.cpp(50): >\n<
//H 10: t_2_020.cpp(52): #if
//H 12: t_2_020.cpp(52): > <
//H 11: t_2_020.cpp(52): #if 0: 0
//H 12: t_2_020.cpp(52): >\n<
//H 12: t_2_020.cpp(53): >int<
//H 12: t_2_020.cpp(53): > <
//H 12: t_2_020.cpp(53): >x1<
//H 12: t_2_020.cpp(53): >;<
//H 12: t_2_020.cpp(53): >\n<
//H 10: t_2_020.cpp(54): #elif
//H 12: t_2_020.cpp(54): > <
//H 11: t_2_020.cpp(54): #elif 1: 1
//H 12: t_2_020.cpp(54): >\n<
//H 10: t_2_020.cpp(56): #else
//H 12: t_2_020.cpp(56): >#else<
//H 12: t_2_020.cpp(56): >\n<
//H 12: t_2_020.cpp(57): >int<
//H 12: t_2_020.cpp(57): > <
//H 12: t_2_020.cpp(57): >x3<
//H 12: t_2_020.cpp(57): >;<
//H 12: t_2_020.cpp(57): >\n<
//H 12: t_2_020.cpp(58): >#endif<
//H 12: t_2_020.cpp(58): >\n<
//H 10: t_2_020.cpp(60): #if
//H 12: t_2_020.cpp(60): > <
//H 11: t_2_020.cpp(60): #if 1: 1
//H 12: t_2_020.cpp(60): >\n<
//H 10: t_2_020.cpp(62): #elif
//H 12: t_2_020.cpp(62): > <
//H 12: t_2_020.cpp(62): >0<
//H 12: t_2_020.cpp(62): >\n<
//H 12: t_2_020.cpp(63): >int<
//H 12: t_2_020.cpp(63): > <
//H 12: t_2_020.cpp(63): >y2<
//H 12: t_2_020.cpp(63): >;<
//H 12: t_2_020.cpp(63): >\n<
//H 12: t_2_020.cpp(64): >#else<
//H 12: t_2_020.cpp(64): >\n<
//H 12: t_2_020.cpp(65): >int<
//H 12: t_2_020.cpp(65): > <
//H 12: t_2_020.cpp(65): >y3<
//H 12: t_2_020.cpp(65): >;<
//H 12: t_2_020.cpp(65): >\n<
//H 12: t_2_020.cpp(66): >#endif<
//H 12: t_2_020.cpp(66): >\n<
//H 10: t_2_020.cpp(68): #if
//H 12: t_2_020.cpp(68): > <
//H 11: t_2_020.cpp(68): #if 0: 0
//H 12: t_2_020.cpp(68): >\n<
//H 12: t_2_020.cpp(69): >int<
//H 12: t_2_020.cpp(69): > <
//H 12: t_2_020.cpp(69): >z1<
//H 12: t_2_020.cpp(69): >;<
//H 12: t_2_020.cpp(69): >\n<
//H 10: t_2_020.cpp(70): #elif
//H 12: t_2_020.cpp(70): > <
//H 11: t_2_020.cpp(70): #elif 0: 0
//H 12: t_2_020.cpp(70): >\n<
//H 12: t_2_020.cpp(71): >int<
//H 12: t_2_020.cpp(71): > <
//H 12: t_2_020.cpp(71): >z2<
//H 12: t_2_020.cpp(71): >;<
//H 12: t_2_020.cpp(71): >\n<
//H 12: t_2_020.cpp(72): >#else<
//H 12: t_2_020.cpp(72): >\n<
//H 10: t_2_020.cpp(74): #endif
//H 12: t_2_020.cpp(74): >#endif<
//H 12: t_2_020.cpp(74): >\n<
