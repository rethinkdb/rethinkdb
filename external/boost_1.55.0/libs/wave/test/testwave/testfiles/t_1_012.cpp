/*=============================================================================
    Boost.Wave: A Standard compliant C++ preprocessor library
    http://www.boost.org/

    Copyright (c) 2001-2012 Hartmut Kaiser. Distributed under the Boost
    Software License, Version 1.0. (See accompanying file
    LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
=============================================================================*/

// Test, if  function like macros are correctly expanded if used as object like 
// macros

#define CAT(a, b) a ## b
#define ARGS (1, 2)

//R #line 18 "t_1_012.cpp"
//R CAT (1, 2)
CAT ARGS

#define INVOKE(macro) macro ARGS

//R #line 24 "t_1_012.cpp"
//R CAT (1, 2)
INVOKE(CAT)

#define EXPAND(x) x

//R #line 30 "t_1_012.cpp"
//R 12
EXPAND(CAT ARGS)

//H 10: t_1_012.cpp(13): #define
//H 08: t_1_012.cpp(13): CAT(a, b)=a ## b
//H 10: t_1_012.cpp(14): #define
//H 08: t_1_012.cpp(14): ARGS=(1, 2)
//H 01: t_1_012.cpp(14): ARGS
//H 02: (1, 2)
//H 03: (1, 2)
//H 10: t_1_012.cpp(20): #define
//H 08: t_1_012.cpp(20): INVOKE(macro)=macro ARGS
//H 00: t_1_012.cpp(24): INVOKE(CAT), [t_1_012.cpp(20): INVOKE(macro)=macro ARGS]
//H 02: CAT ARGS
//H 01: t_1_012.cpp(14): ARGS
//H 02: (1, 2)
//H 03: (1, 2)
//H 03: CAT (1, 2)
//H 10: t_1_012.cpp(26): #define
//H 08: t_1_012.cpp(26): EXPAND(x)=x
//H 00: t_1_012.cpp(30): EXPAND(CAT ARGS), [t_1_012.cpp(26): EXPAND(x)=x]
//H 01: t_1_012.cpp(14): ARGS
//H 02: (1, 2)
//H 03: (1, 2)
//H 02: CAT (1, 2)
//H 00: t_1_012.cpp(30): CAT(1, 2), [t_1_012.cpp(13): CAT(a, b)=a ## b]
//H 02: 12
//H 03: 12
//H 03: 12
