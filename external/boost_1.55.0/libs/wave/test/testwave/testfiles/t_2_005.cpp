/*=============================================================================
    Boost.Wave: A Standard compliant C++ preprocessor library
    http://www.boost.org/

    Copyright (c) 2001-2012 Hartmut Kaiser. Distributed under the Boost
    Software License, Version 1.0. (See accompanying file
    LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
=============================================================================*/

// test the error reporting for unknown directives

//R
//E t_2_005.cpp(14): error: ill formed preprocessor directive: #this_is_a_unknown_pp_directive with some parameter
#this_is_a_unknown_pp_directive with some parameter

//H 21: t_2_005.cpp(14): #this_is_a_unknown_pp_directive with some parameter
//H 18: boost::wave::preprocess_exception
