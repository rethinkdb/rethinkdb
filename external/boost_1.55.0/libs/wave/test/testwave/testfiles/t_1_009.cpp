/*=============================================================================
    Boost.Wave: A Standard compliant C++ preprocessor library
    http://www.boost.org/

    Copyright (c) 2001-2012 Hartmut Kaiser. Distributed under the Boost
    Software License, Version 1.0. (See accompanying file
    LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
=============================================================================*/

// Test, if wrongly placed '##' operators are detected

#define TEST1() A ## B
#define TEST2() ## A

//R #line 17 "t_1_009.cpp"
//R AB
TEST1()
//E t_1_009.cpp(19): error: ill formed preprocessing operator: concat ('##')
TEST2()    // error

//H 10: t_1_009.cpp(12): #define
//H 08: t_1_009.cpp(12): TEST1()=A ## B
//H 10: t_1_009.cpp(13): #define
//H 08: t_1_009.cpp(13): TEST2()=## A
//H 00: t_1_009.cpp(17): TEST1(), [t_1_009.cpp(12): TEST1()=A ## B]
//H 02: AB
//H 03: AB
//H 00: t_1_009.cpp(19): TEST2(), [t_1_009.cpp(13): TEST2()=## A]
//H 18: boost::wave::preprocess_exception
