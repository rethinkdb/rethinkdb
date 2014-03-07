/*=============================================================================
    Boost.Wave: A Standard compliant C++ preprocessor library
    http://www.boost.org/

    Copyright (c) 2001-2012 Hartmut Kaiser. Distributed under the Boost
    Software License, Version 1.0. (See accompanying file
    LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
=============================================================================*/

// Tests, if the recursive replacement of a macro is correctly handled

#define f(x) (4-f(x))

//R #line 16 "t_1_015.cpp"
//R (4-f((4-f(1))))
f(f(1))

//H 10: t_1_015.cpp(12): #define
//H 08: t_1_015.cpp(12): f(x)=(4-f(x))
//H 00: t_1_015.cpp(16): f(f(1)), [t_1_015.cpp(12): f(x)=(4-f(x))]
//H 00: t_1_015.cpp(16): f(1), [t_1_015.cpp(12): f(x)=(4-f(x))]
//H 02: (4-f(1))
//H 03: (4-f(1))
//H 02: (4-f((4-f(1))))
//H 03: (4-f((4-f(1))))
