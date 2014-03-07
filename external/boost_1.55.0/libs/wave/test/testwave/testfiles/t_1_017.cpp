/*=============================================================================
    Boost.Wave: A Standard compliant C++ preprocessor library
    http://www.boost.org/

    Copyright (c) 2001-2012 Hartmut Kaiser. Distributed under the Boost
    Software License, Version 1.0. (See accompanying file
    LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
=============================================================================*/

// Tests macro expansion sequence and proper rescanning 

#define macro() end

#define par() ()

#define expr macro par par par()

#define scan(x) x

//R #line 22 "t_1_017.cpp"
//R macro par par ()
expr
//R #line 25 "t_1_017.cpp"
//R macro par ()
scan(expr)
//R #line 28 "t_1_017.cpp"
//R macro ()
scan(scan(expr))
//R #line 31 "t_1_017.cpp"
//R end
scan(scan(scan(expr)))
