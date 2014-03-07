/*=============================================================================
    Boost.Wave: A Standard compliant C++ preprocessor library
    http://www.boost.org/

    Copyright (c) 2001-2012 Hartmut Kaiser. Distributed under the Boost
    Software License, Version 1.0. (See accompanying file
    LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
=============================================================================*/

// This sample is taken from the C++ standard 16.3.5.6 [cpp.scope] and was 
// slightly modified (removed the '#include' directive)

#define str(s) # s
#define xstr(s) str(s)
#define debug(s, t) printf("x" # s "= %d, x" # t "= %s", \
x ## s, x ## t)
#define INCFILE(n) vers ## n /* from previous #include example */
#define glue(a, b) a ## b
#define xglue(a, b) glue(a, b)
#define HIGHLOW "hello"
#define LOW LOW ", world"
debug(1, 2);
fputs(str(strncmp("abc\0d?", "abc", '\4', "\u1234") /* this goes away */
== 0) str(: @\n), s);
/*#include*/ xstr(INCFILE(2).hpp)
glue(HIGH, LOW);
xglue(HIGH, LOW)

// should expand to 
//R #line 22 "t_1_013.cpp"
//R printf("x" "1" "= %d, x" "2" "= %s", x1, x2);
//R fputs("strncmp(\"abc\\0d\?\", \"abc\", '\\4', \"\\u1234\") == 0" ": @\n", s);
//R
//R "vers2.hpp"
//R "hello";
//R "hello" ", world"

//H 10: t_1_013.cpp(13): #define
//H 08: t_1_013.cpp(13): str(s)=# s
//H 10: t_1_013.cpp(14): #define
//H 08: t_1_013.cpp(14): xstr(s)=str(s)
//H 10: t_1_013.cpp(15): #define
//H 08: t_1_013.cpp(15): debug(s, t)=printf("x" # s "= %d, x" # t "= %s", x ## s, x ## t)
//H 10: t_1_013.cpp(17): #define
//H 08: t_1_013.cpp(17): INCFILE(n)=vers ## n
//H 10: t_1_013.cpp(18): #define
//H 08: t_1_013.cpp(18): glue(a, b)=a ## b
//H 10: t_1_013.cpp(19): #define
//H 08: t_1_013.cpp(19): xglue(a, b)=glue(a, b)
//H 10: t_1_013.cpp(20): #define
//H 08: t_1_013.cpp(20): HIGHLOW="hello"
//H 10: t_1_013.cpp(21): #define
//H 08: t_1_013.cpp(21): LOW=LOW ", world"
//H 00: t_1_013.cpp(22): debug(1, 2), [t_1_013.cpp(15): debug(s, t)=printf("x" # s "= %d, x" # t "= %s", x ## s, x ## t)]
//H 02: printf("x"  "1" "= %d, x"  "2" "= %s", x1, x2)
//H 03: printf("x"  "1" "= %d, x"  "2" "= %s", x1, x2)
//H 00: t_1_013.cpp(23): str(strncmp("abc\0d?", "abc", '\4', "\u1234") == 0), [t_1_013.cpp(13): str(s)=# s]
//H 02:  "strncmp(\"abc\\0d\?\", \"abc\", '\\4', \"\\u1234\") == 0"
//H 03: "strncmp(\"abc\\0d\?\", \"abc\", '\\4', \"\\u1234\") == 0"
//H 00: t_1_013.cpp(24): str(: @\n), [t_1_013.cpp(13): str(s)=# s]
//H 02:  ": @\n"
//H 03: ": @\n"
//H 00: t_1_013.cpp(25): xstr(INCFILE(2).hpp), [t_1_013.cpp(14): xstr(s)=str(s)]
//H 00: t_1_013.cpp(25): INCFILE(2), [t_1_013.cpp(17): INCFILE(n)=vers ## n]
//H 02: vers2
//H 03: vers2
//H 02: str(vers2.hpp)
//H 00: t_1_013.cpp(14): str(vers2.hpp), [t_1_013.cpp(13): str(s)=# s]
//H 02:  "vers2.hpp"
//H 03: "vers2.hpp"
//H 03: "vers2.hpp"
//H 00: t_1_013.cpp(26): glue(HIGH, LOW), [t_1_013.cpp(18): glue(a, b)=a ## b]
//H 02: HIGHLOW
//H 01: t_1_013.cpp(20): HIGHLOW
//H 02: "hello"
//H 03: "hello"
//H 03: "hello"
//H 00: t_1_013.cpp(27): xglue(HIGH, LOW), [t_1_013.cpp(19): xglue(a, b)=glue(a, b)]
//H 01: t_1_013.cpp(21): LOW
//H 02: LOW ", world"
//H 03: LOW ", world"
//H 02: glue(HIGH,  LOW ", world")
//H 00: t_1_013.cpp(19): glue(HIGH, LOW ", world"), [t_1_013.cpp(18): glue(a, b)=a ## b]
//H 02: HIGHLOW ", world"
//H 01: t_1_013.cpp(20): HIGHLOW
//H 02: "hello"
//H 03: "hello"
//H 03: "hello" ", world"
//H 03: "hello" ", world"
