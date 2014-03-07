/*=============================================================================
    Boost.Wave: A Standard compliant C++ preprocessor library
    http://www.boost.org/

    Copyright (c) 2001-2012 Hartmut Kaiser. Distributed under the Boost
    Software License, Version 1.0. (See accompanying file
    LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
=============================================================================*/

//  tests whether whitespace is correctly handled in macro arguments

//O --variadics

#define STRINGIZE1(x) #x
#define STRINGIZE(x)  STRINGIZE1(x)

#define MACRO(...)    (__VA_ARGS__) - STRINGIZE((__VA_ARGS__))

//R #line 20 "t_9_020.cpp"
MACRO()               //R () - "()" 
MACRO( )              //R () - "()" 
MACRO(a)              //R (a) - "(a)" 
MACRO( a )            //R ( a ) - "( a )" 
MACRO(  a  )          //R ( a ) - "( a )" 
MACRO(a,b)            //R (a,b) - "(a,b)" 
MACRO(a, b)           //R (a, b) - "(a, b)" 
MACRO(a ,b)           //R (a ,b) - "(a ,b)" 
MACRO( a ,b, c )      //R ( a ,b, c ) - "( a ,b, c )" 
MACRO(   a ,b, c   )  //R ( a ,b, c ) - "( a ,b, c )" 

#undef MACRO
#define MACRO(x) [x]

//R #line 35 "t_9_020.cpp"
MACRO()               //R [] 
MACRO( )              //R [] 
MACRO(123)            //R [123] 
MACRO( 123 )          //R [ 123 ] 
MACRO(  123  )        //R [ 123 ] 

#define A(x) 1 x 3
#define B(x) (1)x(3)

//R #line 45 "t_9_020.cpp"
A(2)                  //R 1 2 3 
STRINGIZE(A(2))       //R "1 2 3" 
STRINGIZE(B(2))       //R "(1)2(3)" 
STRINGIZE(B( 2 ))     //R "(1) 2 (3)" 
