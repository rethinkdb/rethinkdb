/*=============================================================================
    Boost.Wave: A Standard compliant C++ preprocessor library
    http://www.boost.org/

    Copyright (c) 2001-2012 Hartmut Kaiser. Distributed under the Boost
    Software License, Version 1.0. (See accompanying file
    LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
=============================================================================*/

// Tests the #warning directive (note: only available, if enabled during
// compilation, the macro expansion is available only, when enabled separately
// during the compilation too)

//R
//E t_2_001.cpp(18): warning: encountered #warning directive: This is a warning
#define WARNING1 This is a 
#define WARNING2 warning
#warning WARNING1 WARNING2

//H 10: t_2_001.cpp(16): #define
//H 08: t_2_001.cpp(16): WARNING1=This is a
//H 10: t_2_001.cpp(17): #define
//H 08: t_2_001.cpp(17): WARNING2=warning
//H 10: t_2_001.cpp(18): #warning
//H 01: t_2_001.cpp(16): WARNING1
//H 02: This is a
//H 03: This is a
//H 01: t_2_001.cpp(17): WARNING2
//H 02: warning
//H 03: warning
//H 15: WARNING1 WARNING2
//H 18: boost::wave::preprocess_exception
