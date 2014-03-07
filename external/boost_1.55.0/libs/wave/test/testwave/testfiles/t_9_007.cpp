/*=============================================================================
    Boost.Wave: A Standard compliant C++ preprocessor library
    http://www.boost.org/

    Copyright (c) 2001-2012 Hartmut Kaiser. Distributed under the Boost
    Software License, Version 1.0. (See accompanying file
    LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
=============================================================================*/

// Test error reporting during redefinition of 'defined'

//E t_9_007.cpp(13): warning: this predefined name may not be redefined: defined
#define defined 1 // undefined in C++ (16.8/3), error in C99 (6.10.8/4)

#if defined  // error
<error>
#endif

//H 10: t_9_007.cpp(13): #define
//H 18: boost::wave::macro_handling_exception
