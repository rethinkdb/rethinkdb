/*=============================================================================
    Boost.Wave: A Standard compliant C++ preprocessor library
    http://www.boost.org/

    Copyright (c) 2001-2012 Hartmut Kaiser. Distributed under the Boost
    Software License, Version 1.0. (See accompanying file
    LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
=============================================================================*/

// Tests, if function-like macros buried deep inside a macro expansion of the
// same name as an object-like macro do not eat up more tokens, than expected.

#define PRIMITIVE_CAT(a, b) a ## b

#define EAT(n) PRIMITIVE_CAT(EAT_, n)
#define EAT_1(a)

//R #line 20 "t_1_023.cpp"
//R EAT_1...
EAT(1)...

//H 10: t_1_023.cpp(13): #define
//H 08: t_1_023.cpp(13): PRIMITIVE_CAT(a, b)=a ## b
//H 10: t_1_023.cpp(15): #define
//H 08: t_1_023.cpp(15): EAT(n)=PRIMITIVE_CAT(EAT_, n)
//H 10: t_1_023.cpp(16): #define
//H 08: t_1_023.cpp(16): EAT_1(a)=
//H 00: t_1_023.cpp(20): EAT(1), [t_1_023.cpp(15): EAT(n)=PRIMITIVE_CAT(EAT_, n)]
//H 02: PRIMITIVE_CAT(EAT_, 1)
//H 00: t_1_023.cpp(15): PRIMITIVE_CAT(EAT_, 1), [t_1_023.cpp(13): PRIMITIVE_CAT(a, b)=a ## b]
//H 02: EAT_1
//H 03: EAT_1
//H 03: EAT_1
