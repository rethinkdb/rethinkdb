/*=============================================================================
    Boost.Wave: A Standard compliant C++ preprocessor library
    http://www.boost.org/

    Copyright (c) 2001-2012 Hartmut Kaiser. Distributed under the Boost
    Software License, Version 1.0. (See accompanying file
    LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
=============================================================================*/

#if !defined(inclusion)
#   define  inclusion
#   include __FILE__
#   include "t_9_016.hpp"
#else
//R #line 18 "t_9_016.cpp"
//R # define later
#define EXPAND(x) x
EXPAND(#) define later
#endif

//H 10: t_9_016.cpp(10): #if
//H 11: t_9_016.cpp(10): #if !defined(inclusion): 1
//H 10: t_9_016.cpp(11): #define
//H 08: t_9_016.cpp(11): inclusion=
//H 10: t_9_016.cpp(12): #   include 
//H 04: "$P(t_9_016.cpp)"
//H 05: $B(t_9_016.cpp) ($B(t_9_016.cpp))
//H 10: t_9_016.cpp(10): #if
//H 11: t_9_016.cpp(10): #if !defined(inclusion): 0
//H 10: t_9_016.cpp(17): #define
//H 08: t_9_016.cpp(17): EXPAND(x)=x
//H 00: t_9_016.cpp(18): EXPAND(#), [t_9_016.cpp(17): EXPAND(x)=x]
//H 02: #
//H 03: #
//H 10: t_9_016.cpp(19): #endif
//H 06: 
//H 19: $B(t_9_016.cpp): inclusion
//H 10: t_9_016.cpp(13): #   include "t_9_016.hpp"
//H 04: "t_9_016.hpp"
//H 05: t_9_016.hpp ($B(t_9_016.hpp))
//H 06: 
//H 10: t_9_016.cpp(14): #else
