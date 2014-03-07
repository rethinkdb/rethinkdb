/*=============================================================================
    Boost.Wave: A Standard compliant C++ preprocessor library
    http://www.boost.org/

    Copyright (c) 2001-2012 Hartmut Kaiser. Distributed under the Boost
    Software License, Version 1.0. (See accompanying file
    LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
=============================================================================*/

// Tests macro expansion sequence and proper rescanning 

#define macro() expr_2
#define macro_2() expr

#define par() ()

#define expr macro par ()
#define expr_2 macro_2 par par par()

#define scan(x) x

//R #line 24 "t_1_018.cpp"
//R macro ()
expr
//R #line 27 "t_1_018.cpp"
//R macro_2 par par ()
scan(expr)
//R #line 30 "t_1_018.cpp"
//R macro_2 par ()
scan(scan(expr))
//R #line 33 "t_1_018.cpp"
//R macro_2 ()
scan(scan(scan(expr)))
//R #line 36 "t_1_018.cpp"
//R macro ()
scan(scan(scan(scan(expr))))
//R #line 39 "t_1_018.cpp"
//R macro_2 par par ()
scan(scan(scan(scan(scan(expr)))))
//R #line 42 "t_1_018.cpp"
//R macro_2 par ()
scan(scan(scan(scan(scan(scan(expr))))))
