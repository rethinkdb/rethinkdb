/*=============================================================================
    Boost.Wave: A Standard compliant C++ preprocessor library
    http://www.boost.org/

    Copyright (c) 2001-2012 Hartmut Kaiser. Distributed under the Boost
    Software License, Version 1.0. (See accompanying file
    LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
=============================================================================*/

// Tests delayed macro expansion (rescanning)

#define CONCAT_1(A, B) A ## B
#define CONCAT_2(A, B) CONCAT_1(A, B)

#define DELAY(NAME) NAME

#define A1 a
#define B1 b

#define A2() a
#define B2() b

#define LHS (
#define RHS )

//R #line 27 "t_1_027.cpp"
DELAY(CONCAT_1)( a, b ) ();             //R ab (); 
DELAY(CONCAT_1)(A1, B1)();              //R A1B1(); 
DELAY(CONCAT_1) LHS A1, B1 RHS ();      //R CONCAT_1 ( a, b )(); 
CONCAT_1 ( a, b ) ();                   //R ab (); 
CONCAT_1 ( A1, B1 ) ();                 //R A1B1 (); 
CONCAT_1 LHS a, b RHS ();               //R CONCAT_1 ( a, b )(); 
//R
DELAY(CONCAT_2)( a, b ) ();             //R ab (); 
DELAY(CONCAT_2)(A1, B1)();              //R ab(); 
DELAY(CONCAT_2) LHS A1, B1 RHS ();      //R CONCAT_2 ( a, b )(); 
DELAY(CONCAT_2)(A2(), B2())();          //R ab(); 
CONCAT_2 ( a, b ) ();                   //R ab (); 
CONCAT_2 ( A1, B1 ) ();                 //R ab (); 
CONCAT_2 LHS a, b RHS ();               //R CONCAT_2 ( a, b )(); 
CONCAT_2(A2(), B2())();                 //R ab(); 
