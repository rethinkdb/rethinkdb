/*=============================================================================
    Boost.Wave: A Standard compliant C++ preprocessor library
    http://www.boost.org/

    Copyright (c) 2001-2012 Hartmut Kaiser. Distributed under the Boost
    Software License, Version 1.0. (See accompanying file
    LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
=============================================================================*/

// Tests error reporting for missing #if

//E t_2_012.cpp(13): error: the #if for this directive is missing: #else
#else
#endif

//H 10: t_2_012.cpp(13): #else
//H 18: boost::wave::preprocess_exception
