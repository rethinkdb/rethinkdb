/*=============================================================================
    Boost.Wave: A Standard compliant C++ preprocessor library
    http://www.boost.org/

    Copyright (c) 2001-2012 Hartmut Kaiser. Distributed under the Boost
    Software License, Version 1.0. (See accompanying file
    LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
=============================================================================*/

// Verify fix of regression #6838: Adding include file with force_include makes
// Wave fail to emit #line directive

//O --forceinclude=t_2_022.hpp

//R #line 12 "t_2_022.hpp"
//R int func() { return 42; }
//R #line 19 "t_2_022.cpp"
//R int main() { return func(); }
int main() { return func(); }

//H 04: t_2_022.hpp
//H 05: $B(t_2_022.hpp) ($B(t_2_022.hpp))
//H 06: 
