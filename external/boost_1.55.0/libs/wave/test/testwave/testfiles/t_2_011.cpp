/*=============================================================================
    Boost.Wave: A Standard compliant C++ preprocessor library
    http://www.boost.org/

    Copyright (c) 2001-2012 Hartmut Kaiser. Distributed under the Boost
    Software License, Version 1.0. (See accompanying file
    LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
=============================================================================*/

// Tests error reporting for missing #if

//E t_2_011.cpp(15): error: the #if for this directive is missing: #endif
#if 1
#endif
#endif

//H 10: t_2_011.cpp(13): #if
//H 11: t_2_011.cpp(13): #if 1: 1
//H 10: t_2_011.cpp(14): #endif
//H 10: t_2_011.cpp(15): #endif
//H 18: boost::wave::preprocess_exception
