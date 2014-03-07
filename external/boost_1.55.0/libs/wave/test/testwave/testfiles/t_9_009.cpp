/*=============================================================================
    Boost.Wave: A Standard compliant C++ preprocessor library
    http://www.boost.org/

    Copyright (c) 2001-2012 Hartmut Kaiser. Distributed under the Boost
    Software License, Version 1.0. (See accompanying file
    LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
=============================================================================*/

// Tests for a problem wrt preprocessing tokens (preprocessing numbers)

#define X() X_ ## 0R()
#define X_0R() ...

//R #line 16 "t_9_009.cpp"
X() //R ... 

//H 10: t_9_009.cpp(12): #define
//H 08: t_9_009.cpp(12): X()=X_ ## 0R()
//H 10: t_9_009.cpp(13): #define
//H 08: t_9_009.cpp(13): X_0R()=...
//H 00: t_9_009.cpp(16): X(), [t_9_009.cpp(12): X()=X_ ## 0R()]
//H 02: X_0R()
//H 00: t_9_009.cpp(12): X_0R(), [t_9_009.cpp(13): X_0R()=...]
//H 02: ...
//H 03: ...
//H 03: ...
