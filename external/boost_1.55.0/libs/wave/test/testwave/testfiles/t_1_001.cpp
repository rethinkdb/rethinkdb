/*=============================================================================
    Boost.Wave: A Standard compliant C++ preprocessor library
    http://www.boost.org/

    Copyright (c) 2001-2012 Hartmut Kaiser. Distributed under the Boost
    Software License, Version 1.0. (See accompanying file
    LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
=============================================================================*/

// Test macro expansion order
#define A(x, y) x,y
#define B(x, y) [x][y]
#define C(x) B(x)

//R #line 16 "t_1_001.cpp"
C(A(2,3))          //R [2][3] 
C( A(2 , 3) )      //R [ 2 ][ 3 ] 

//H 10: t_1_001.cpp(11): #define
//H 08: t_1_001.cpp(11): A(x, y)=x,y
//H 10: t_1_001.cpp(12): #define
//H 08: t_1_001.cpp(12): B(x, y)=[x][y]
//H 10: t_1_001.cpp(13): #define
//H 08: t_1_001.cpp(13): C(x)=B(x)
//H 00: t_1_001.cpp(16): C(A(2,3)), [t_1_001.cpp(13): C(x)=B(x)]
//H 00: t_1_001.cpp(16): A(2,3), [t_1_001.cpp(11): A(x, y)=x,y]
//H 02: 2,3
//H 03: 2,3
//H 02: B(2,3)
//H 00: t_1_001.cpp(13): B(2,3), [t_1_001.cpp(12): B(x, y)=[x][y]]
//H 02: [2][3]
//H 03: [2][3]
//H 03: [2][3]
//H 00: t_1_001.cpp(17): C( A(2 , 3) ), [t_1_001.cpp(13): C(x)=B(x)]
//H 00: t_1_001.cpp(17): A(2 , 3), [t_1_001.cpp(11): A(x, y)=x,y]
//H 02: 2 , 3
//H 03: 2 , 3
//H 02: B( 2 , 3 )
//H 00: t_1_001.cpp(13): B( 2 , 3 ), [t_1_001.cpp(12): B(x, y)=[x][y]]
//H 02: [ 2 ][ 3 ]
//H 03: [ 2 ][ 3 ]
//H 03: [ 2 ][ 3 ]
