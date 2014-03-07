/*=============================================================================
    Boost.Wave: A Standard compliant C++ preprocessor library
    http://www.boost.org/

    Copyright (c) 2001-2012 Hartmut Kaiser. Distributed under the Boost
    Software License, Version 1.0. (See accompanying file
    LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
=============================================================================*/

// Test if macros at not used parameter positions are expanded correctly

#define MACRO() /**/
#define ID(x)           // parameter not used at all
#define CAT(x) X ## x   // expanded parameter not used

ID( MACRO(*) )
//R #line 19 "t_1_011.cpp"
//R XMACRO(*)
CAT( MACRO(*) )

//H 10: t_1_011.cpp(12): #define
//H 08: t_1_011.cpp(12): MACRO()=
//H 10: t_1_011.cpp(13): #define
//H 08: t_1_011.cpp(13): ID(x)=
//H 10: t_1_011.cpp(14): #define
//H 08: t_1_011.cpp(14): CAT(x)=X ## x
//H 00: t_1_011.cpp(16): ID( MACRO(*) ), [t_1_011.cpp(13): ID(x)=]
//H 02: 
//H 03: _
//H 00: t_1_011.cpp(19): CAT( MACRO(*) ), [t_1_011.cpp(14): CAT(x)=X ## x]
//H 02: XMACRO(*)
//H 03: XMACRO(*)
