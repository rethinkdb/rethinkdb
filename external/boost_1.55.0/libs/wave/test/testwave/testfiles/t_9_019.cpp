/*=============================================================================
    Boost.Wave: A Standard compliant C++ preprocessor library
    http://www.boost.org/

    Copyright (c) 2001-2012 Hartmut Kaiser. Distributed under the Boost
    Software License, Version 1.0. (See accompanying file
    LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
=============================================================================*/

//  tests, whether regression causing #include_next to infinitely recurse is
//  fixed

//E t_9_019.hpp(11): error: could not find include file: t_9_019.hpp
#include "t_9_019.hpp"

// 10: t_9_019.cpp(14): #include "t_9_019.hpp"
// 04: "t_9_019.hpp"
// 05: $B(t_9_019.hpp) ($B(t_9_019.hpp))
// 10: t_9_019.hpp(11): #include_next "t_9_019.hpp"
// 04: "t_9_019.hpp" (include_next)
// 18: boost::wave::preprocess_exception
