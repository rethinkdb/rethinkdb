/*=============================================================================
    Boost.Wave: A Standard compliant C++ preprocessor library
    http://www.boost.org/

    Copyright (c) 2001-2012 Hartmut Kaiser. Distributed under the Boost
    Software License, Version 1.0. (See accompanying file
    LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
=============================================================================*/

// Check if #pragma once, include guard detection, and related hooks work as 
// expected

#include "t_2_019_001.hpp"      // #pragma once
#include "t_2_019_002.hpp"      // include guard style 1
#include "t_2_019_003.hpp"      // include guard style 2

// repeat inclusion, should do nothing
#include "t_2_019_001.hpp"
#include "t_2_019_002.hpp"
#include "t_2_019_003.hpp"

//R #line 11 "t_2_019_001.hpp"
//R t_2_019_001
//R #line 16 "t_2_019_002.hpp"
//R t_2_019_002
//R #line 16 "t_2_019_003.hpp"
//R t_2_019_003

//H 10: t_2_019.cpp(13): #include "t_2_019_001.hpp"
//H 04: "t_2_019_001.hpp"
//H 05: $S(t_2_019_001.hpp) ($B(t_2_019_001.hpp))
//H 10: t_2_019_001.hpp(10): #pragma
//H 20: t_2_019_001.hpp(10): #pragma: $B(t_2_019_001.hpp)
//H 06: 
//H 10: t_2_019.cpp(14): #include "t_2_019_002.hpp"
//H 04: "t_2_019_002.hpp"
//H 05: t_2_019_002.hpp ($B(t_2_019_002.hpp))
//H 10: t_2_019_002.hpp(12): #if
//H 11: t_2_019_002.hpp(12): #if !defined(T_2_019_002): 1
//H 10: t_2_019_002.hpp(14): #define
//H 08: t_2_019_002.hpp(14): T_2_019_002=
//H 10: t_2_019_002.hpp(18): #endif
//H 06: 
//H 19: $B(t_2_019_002.hpp): T_2_019_002
//H 10: t_2_019.cpp(15): #include "t_2_019_003.hpp"
//H 04: "t_2_019_003.hpp"
//H 05: t_2_019_003.hpp ($B(t_2_019_003.hpp))
//H 10: t_2_019_003.hpp(12): #ifndef
//H 11: t_2_019_003.hpp(12): #ifndef T_2_019_003: 0
//H 10: t_2_019_003.hpp(14): #define
//H 08: t_2_019_003.hpp(14): T_2_019_003=
//H 10: t_2_019_003.hpp(18): #endif
//H 06: 
//H 19: $B(t_2_019_003.hpp): T_2_019_003
//H 10: t_2_019.cpp(18): #include "t_2_019_001.hpp"
//H 04: "t_2_019_001.hpp"
//H 10: t_2_019.cpp(19): #include "t_2_019_002.hpp"
//H 04: "t_2_019_002.hpp"
//H 10: t_2_019.cpp(20): #include "t_2_019_003.hpp"
//H 04: "t_2_019_003.hpp"

