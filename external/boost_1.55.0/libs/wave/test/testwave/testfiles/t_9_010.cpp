/*=============================================================================
    Boost.Wave: A Standard compliant C++ preprocessor library
    http://www.boost.org/

    Copyright (c) 2001-2012 Hartmut Kaiser. Distributed under the Boost
    Software License, Version 1.0. (See accompanying file
    LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
=============================================================================*/

// Tests, whether adjacent tokens are separated by whitespace if the adjacency
// is created by macro expansion

#define A(x) x

//R #line 16 "t_9_010.cpp"
A(1)1         //R 1 1 
A(1)X         //R 1 X 
A(X)1         //R X 1 
A(X)X         //R X X 

#define CAT(a, b) PRIMITIVE_CAT(a, b)
#define PRIMITIVE_CAT(a, b) a ## b

#define X() B
#define ABC 1

//R #line 28 "t_9_010.cpp"
CAT(A, X() C) //R AB C 
CAT(A, X()C)  //R AB C 

//H 10: t_9_010.cpp(13): #define
//H 08: t_9_010.cpp(13): A(x)=x
//H 00: t_9_010.cpp(16): A(1), [t_9_010.cpp(13): A(x)=x]
//H 02: 1
//H 03: 1
//H 00: t_9_010.cpp(17): A(1), [t_9_010.cpp(13): A(x)=x]
//H 02: 1
//H 03: 1
//H 00: t_9_010.cpp(18): A(X), [t_9_010.cpp(13): A(x)=x]
//H 02: X
//H 03: X
//H 00: t_9_010.cpp(19): A(X), [t_9_010.cpp(13): A(x)=x]
//H 02: X
//H 03: X
//H 10: t_9_010.cpp(21): #define
//H 08: t_9_010.cpp(21): CAT(a, b)=PRIMITIVE_CAT(a, b)
//H 10: t_9_010.cpp(22): #define
//H 08: t_9_010.cpp(22): PRIMITIVE_CAT(a, b)=a ## b
//H 10: t_9_010.cpp(24): #define
//H 08: t_9_010.cpp(24): X()=B
//H 10: t_9_010.cpp(25): #define
//H 08: t_9_010.cpp(25): ABC=1
//H 00: t_9_010.cpp(28): CAT(A, X() C), [t_9_010.cpp(21): CAT(a, b)=PRIMITIVE_CAT(a, b)]
//H 00: t_9_010.cpp(28): X(), [t_9_010.cpp(24): X()=B]
//H 02: B
//H 03: B
//H 02: PRIMITIVE_CAT(A,  B C)
//H 00: t_9_010.cpp(21): PRIMITIVE_CAT(A, B C), [t_9_010.cpp(22): PRIMITIVE_CAT(a, b)=a ## b]
//H 02: AB C
//H 03: AB C
//H 03: AB C
//H 00: t_9_010.cpp(29): CAT(A, X()C), [t_9_010.cpp(21): CAT(a, b)=PRIMITIVE_CAT(a, b)]
//H 00: t_9_010.cpp(29): X(), [t_9_010.cpp(24): X()=B]
//H 02: B
//H 03: B
//H 02: PRIMITIVE_CAT(A,  BC)
//H 00: t_9_010.cpp(21): PRIMITIVE_CAT(A, BC), [t_9_010.cpp(22): PRIMITIVE_CAT(a, b)=a ## b]
//H 02: ABC
//H 03: ABC
//H 03: ABC
