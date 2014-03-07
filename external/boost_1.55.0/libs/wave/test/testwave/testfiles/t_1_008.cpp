/*=============================================================================
    Boost.Wave: A Standard compliant C++ preprocessor library
    http://www.boost.org/

    Copyright (c) 2001-2012 Hartmut Kaiser. Distributed under the Boost
    Software License, Version 1.0. (See accompanying file
    LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
=============================================================================*/

// The follwoing is a parametized partially expanding concatenation macro.
// It is a extremly good test of expansion order and the order of operations 
// during macro expansion in general. 

#define CAT(a, b) a ## b

#define PARTIAL_CAT(i) CAT(PARTIAL_CAT_, i)

#define PARTIAL_CAT_0(a, b) CAT(a, b)
#define PARTIAL_CAT_1(a, b) CAT(PARTIAL ## a, b)
#define PARTIAL_CAT_2(a, b) CAT(a, b ## PARTIAL)
#define PARTIAL_CAT_3(a, b) CAT(PARTIAL ## a, b ## PARTIAL)

#define PARTIAL
#define PARTIALPARTIAL

#define X Token1
#define Y Token2

//R #line 31 "t_1_008.cpp"
//R Token1Token2
PARTIAL_CAT(0)( PARTIAL X, Y PARTIAL )
//R #line 34 "t_1_008.cpp"
//R XToken2
PARTIAL_CAT(1)( PARTIAL X, Y PARTIAL )
//R #line 37 "t_1_008.cpp"
//R Token1Y
PARTIAL_CAT(2)( PARTIAL X, Y PARTIAL )
//R #line 40 "t_1_008.cpp"
//R XY
PARTIAL_CAT(3)( PARTIAL X, Y PARTIAL )

//H 10: t_1_008.cpp(14): #define
//H 08: t_1_008.cpp(14): CAT(a, b)=a ## b
//H 10: t_1_008.cpp(16): #define
//H 08: t_1_008.cpp(16): PARTIAL_CAT(i)=CAT(PARTIAL_CAT_, i)
//H 10: t_1_008.cpp(18): #define
//H 08: t_1_008.cpp(18): PARTIAL_CAT_0(a, b)=CAT(a, b)
//H 10: t_1_008.cpp(19): #define
//H 08: t_1_008.cpp(19): PARTIAL_CAT_1(a, b)=CAT(PARTIAL ## a, b)
//H 10: t_1_008.cpp(20): #define
//H 08: t_1_008.cpp(20): PARTIAL_CAT_2(a, b)=CAT(a, b ## PARTIAL)
//H 10: t_1_008.cpp(21): #define
//H 08: t_1_008.cpp(21): PARTIAL_CAT_3(a, b)=CAT(PARTIAL ## a, b ## PARTIAL)
//H 10: t_1_008.cpp(23): #define
//H 08: t_1_008.cpp(23): PARTIAL=
//H 10: t_1_008.cpp(24): #define
//H 08: t_1_008.cpp(24): PARTIALPARTIAL=
//H 10: t_1_008.cpp(26): #define
//H 08: t_1_008.cpp(26): X=Token1
//H 10: t_1_008.cpp(27): #define
//H 08: t_1_008.cpp(27): Y=Token2
//H 00: t_1_008.cpp(31): PARTIAL_CAT(0), [t_1_008.cpp(16): PARTIAL_CAT(i)=CAT(PARTIAL_CAT_, i)]
//H 02: CAT(PARTIAL_CAT_, 0)
//H 00: t_1_008.cpp(16): CAT(PARTIAL_CAT_, 0), [t_1_008.cpp(14): CAT(a, b)=a ## b]
//H 02: PARTIAL_CAT_0
//H 03: PARTIAL_CAT_0
//H 03: PARTIAL_CAT_0
//H 00: t_1_008.cpp(16): PARTIAL_CAT_0( PARTIAL X, Y PARTIAL ), [t_1_008.cpp(18): PARTIAL_CAT_0(a, b)=CAT(a, b)]
//H 01: t_1_008.cpp(23): PARTIAL
//H 02: 
//H 03: _
//H 01: t_1_008.cpp(26): X
//H 02: Token1
//H 03: Token1
//H 01: t_1_008.cpp(27): Y
//H 02: Token2
//H 03: Token2
//H 01: t_1_008.cpp(23): PARTIAL
//H 02: 
//H 03: _
//H 02: CAT(  Token1,  Token2  )
//H 00: t_1_008.cpp(18): CAT( Token1, Token2 ), [t_1_008.cpp(14): CAT(a, b)=a ## b]
//H 02: Token1Token2
//H 03: Token1Token2
//H 03: Token1Token2
//H 00: t_1_008.cpp(34): PARTIAL_CAT(1), [t_1_008.cpp(16): PARTIAL_CAT(i)=CAT(PARTIAL_CAT_, i)]
//H 02: CAT(PARTIAL_CAT_, 1)
//H 00: t_1_008.cpp(16): CAT(PARTIAL_CAT_, 1), [t_1_008.cpp(14): CAT(a, b)=a ## b]
//H 02: PARTIAL_CAT_1
//H 03: PARTIAL_CAT_1
//H 03: PARTIAL_CAT_1
//H 00: t_1_008.cpp(16): PARTIAL_CAT_1( PARTIAL X, Y PARTIAL ), [t_1_008.cpp(19): PARTIAL_CAT_1(a, b)=CAT(PARTIAL ## a, b)]
//H 01: t_1_008.cpp(27): Y
//H 02: Token2
//H 03: Token2
//H 01: t_1_008.cpp(23): PARTIAL
//H 02: 
//H 03: _
//H 02: CAT(PARTIALPARTIAL X,  Token2  )
//H 00: t_1_008.cpp(19): CAT(PARTIALPARTIAL X, Token2 ), [t_1_008.cpp(14): CAT(a, b)=a ## b]
//H 02: PARTIALPARTIAL XToken2
//H 01: t_1_008.cpp(24): PARTIALPARTIAL
//H 02: 
//H 03: _
//H 03: XToken2
//H 03: XToken2
//H 00: t_1_008.cpp(37): PARTIAL_CAT(2), [t_1_008.cpp(16): PARTIAL_CAT(i)=CAT(PARTIAL_CAT_, i)]
//H 02: CAT(PARTIAL_CAT_, 2)
//H 00: t_1_008.cpp(16): CAT(PARTIAL_CAT_, 2), [t_1_008.cpp(14): CAT(a, b)=a ## b]
//H 02: PARTIAL_CAT_2
//H 03: PARTIAL_CAT_2
//H 03: PARTIAL_CAT_2
//H 00: t_1_008.cpp(16): PARTIAL_CAT_2( PARTIAL X, Y PARTIAL ), [t_1_008.cpp(20): PARTIAL_CAT_2(a, b)=CAT(a, b ## PARTIAL)]
//H 01: t_1_008.cpp(23): PARTIAL
//H 02: 
//H 03: _
//H 01: t_1_008.cpp(26): X
//H 02: Token1
//H 03: Token1
//H 02: CAT(  Token1, Y PARTIALPARTIAL)
//H 00: t_1_008.cpp(20): CAT( Token1, Y PARTIALPARTIAL), [t_1_008.cpp(14): CAT(a, b)=a ## b]
//H 02: Token1Y PARTIALPARTIAL
//H 01: t_1_008.cpp(24): PARTIALPARTIAL
//H 02: 
//H 03: _
//H 03: Token1Y_
//H 03: Token1Y_
//H 00: t_1_008.cpp(40): PARTIAL_CAT(3), [t_1_008.cpp(16): PARTIAL_CAT(i)=CAT(PARTIAL_CAT_, i)]
//H 02: CAT(PARTIAL_CAT_, 3)
//H 00: t_1_008.cpp(16): CAT(PARTIAL_CAT_, 3), [t_1_008.cpp(14): CAT(a, b)=a ## b]
//H 02: PARTIAL_CAT_3
//H 03: PARTIAL_CAT_3
//H 03: PARTIAL_CAT_3
//H 00: t_1_008.cpp(16): PARTIAL_CAT_3( PARTIAL X, Y PARTIAL ), [t_1_008.cpp(21): PARTIAL_CAT_3(a, b)=CAT(PARTIAL ## a, b ## PARTIAL)]
//H 02: CAT(PARTIALPARTIAL X, Y PARTIALPARTIAL)
//H 00: t_1_008.cpp(21): CAT(PARTIALPARTIAL X, Y PARTIALPARTIAL), [t_1_008.cpp(14): CAT(a, b)=a ## b]
//H 02: PARTIALPARTIAL XY PARTIALPARTIAL
//H 01: t_1_008.cpp(24): PARTIALPARTIAL
//H 02: 
//H 03: _
//H 01: t_1_008.cpp(24): PARTIALPARTIAL
//H 02: 
//H 03: _
//H 03: XY_
//H 03: XY_
