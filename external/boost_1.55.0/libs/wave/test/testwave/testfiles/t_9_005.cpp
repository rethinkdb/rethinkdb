/*=============================================================================
    Boost.Wave: A Standard compliant C++ preprocessor library
    http://www.boost.org/

    Copyright (c) 2001-2012 Hartmut Kaiser. Distributed under the Boost
    Software License, Version 1.0. (See accompanying file
    LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
=============================================================================*/

// Tests, if no universal character values are to be generated accidently by 
// concatenation

#define CONCAT(a, b)            PRIMITIVE_CONCAT(a, b)
#define PRIMITIVE_CONCAT(a, b)  a ## b
#define STRINGIZE(x)            STRINGIZE_D(x)
#define STRINGIZE_D(x)          # x

//R #line 19 "t_9_005.cpp"
STRINGIZE( CONCAT(\, u00ff) )       //R "\u00ff" 
STRINGIZE( CONCAT(\u00, ff) )       //R "\u00ff" 
STRINGIZE( CONCAT(\u00ff, 56) )     //R "\u00ff56" 
CONCAT(\, u00ff)                    //R \u00ff 
CONCAT(\u00, ff)                    //R \ u00ff 
CONCAT(\u00ff, 56)                  //R \u00ff56 

//E t_9_005.cpp(27): error: a universal character name cannot designate a character in the basic character set: \u0061
STRINGIZE( CONCAT(\, u0061) )       // reports an error
