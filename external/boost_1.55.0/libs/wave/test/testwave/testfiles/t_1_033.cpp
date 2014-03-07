/*=============================================================================
    Boost.Wave: A Standard compliant C++ preprocessor library
    http://www.boost.org/

    Copyright (c) 2001-2012 Hartmut Kaiser. Distributed under the Boost
    Software License, Version 1.0. (See accompanying file
    LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
=============================================================================*/

// Tests the stringize operator in conjunction with varidic macros

//O --variadics

#define STR(...) #__VA_ARGS__

//R #line 17 "t_1_033.cpp"
STR(1, 2, 3)            //R "1, 2, 3" 
STR(1,2,3)              //R "1,2,3" 
STR(1 , 2 , 3)          //R "1 , 2 , 3" 
STR( 1  ,   2  ,   3 )  //R "1 , 2 , 3" 

//H 10: t_1_033.cpp(14): #define
//H 08: t_1_033.cpp(14): STR(...)=#__VA_ARGS__
//H 00: t_1_033.cpp(17): STR(1, 2, 3), [t_1_033.cpp(14): STR(...)=#__VA_ARGS__]
//H 02: "1, 2, 3"
//H 03: "1, 2, 3"
//H 00: t_1_033.cpp(18): STR(1,2,3), [t_1_033.cpp(14): STR(...)=#__VA_ARGS__]
//H 02: "1,2,3"
//H 03: "1,2,3"
//H 00: t_1_033.cpp(19): STR(1 , 2 , 3), [t_1_033.cpp(14): STR(...)=#__VA_ARGS__]
//H 02: "1 , 2 , 3"
//H 03: "1 , 2 , 3"
//H 00: t_1_033.cpp(20): STR( 1 , 2 , 3 ), [t_1_033.cpp(14): STR(...)=#__VA_ARGS__]
//H 02: "1 , 2 , 3"
//H 03: "1 , 2 , 3"
