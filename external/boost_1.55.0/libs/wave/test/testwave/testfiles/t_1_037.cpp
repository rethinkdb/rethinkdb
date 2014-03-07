/*=============================================================================
    Boost.Wave: A Standard compliant C++ preprocessor library
    http://www.boost.org/

    Copyright (c) 2001-2012 Hartmut Kaiser. Distributed under the Boost
    Software License, Version 1.0. (See accompanying file
    LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
=============================================================================*/

// the following concatenation needs to fail (even if this construct is used 
// in some MS headers)

//R
//E t_1_037.cpp(16): error: pasting the following two tokens does not give a valid preprocessing token: "/" and "/"
#define _VARIANT_BOOL    /##/
_VARIANT_BOOL bool;

