/*=============================================================================
    Boost.Wave: A Standard compliant C++ preprocessor library
    http://www.boost.org/

    Copyright (c) 2001-2012 Hartmut Kaiser. Distributed under the Boost
    Software License, Version 1.0. (See accompanying file
    LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
=============================================================================*/

// Test, if additional whitespace is inserted at appropriate places.

#define STRINGIZE(x) STRINGIZE_D(x)
#define STRINGIZE_D(x) #x

#define X() 1
#define PLUS() +
#define MINUS() -
#define DOT() .
#define GREATER() >
#define LESS() <

//R #line 23 "t_9_003.cpp"
X()2                          //R 1 2 
STRINGIZE( X()2 )             //R "12" 
//R 
X() 2                         //R 1 2 
STRINGIZE( X() 2 )            //R "1 2" 
//R 
PLUS()MINUS()                 //R +- 
STRINGIZE( PLUS()MINUS() )    //R "+-" 
//R 
PLUS()PLUS()                  //R + + 
STRINGIZE( PLUS()PLUS() )     //R "++" 
//R 
MINUS()MINUS()                //R - - 
STRINGIZE( MINUS()MINUS() )   //R "--" 
//R 
DOT()DOT()DOT()               //R .. . 
STRINGIZE( DOT()DOT()DOT() )  //R "..." 

// the following are regressions reported by Stefan Seefeld
//R #line 43 "t_9_003.cpp"
GREATER()GREATER()            //R > > 
STRINGIZE( GREATER()GREATER() ) //R ">>" 
//R
LESS()LESS()                  //R < < 
STRINGIZE( LESS()LESS() )     //R "<<" 

#define COMMA() ,
#define AND() &
#define CHAR() char
#define STAR() *

// Make sure no whitespace gets inserted in between the operator symbols
//R #line 56 "t_9_003.cpp"
void foo(char&, char)               //R void foo(char&, char) 
void foo(char *)                    //R void foo(char *) 
void foo(char *&)                   //R void foo(char *&) 
void foo(CHAR()AND()COMMA() CHAR()) //R void foo(char&, char) 
void foo(CHAR() STAR())             //R void foo(char *) 
void foo(CHAR() STAR()AND())        //R void foo(char *&) 
