/*=============================================================================
    Boost.Wave: A Standard compliant C++ preprocessor library
    http://www.boost.org/

    Copyright (c) 2001-2012 Hartmut Kaiser. Distributed under the Boost
    Software License, Version 1.0. (See accompanying file
    LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
=============================================================================*/

// Test error reporting during redefinition of 'defined'

//E t_9_008.cpp(13): warning: #undef may not be used on this predefined name: defined
#undef defined 

//H 10: t_9_008.cpp(13): #undef
//H 18: boost::wave::preprocess_exception
