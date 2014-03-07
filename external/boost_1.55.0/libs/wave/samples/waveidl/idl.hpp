/*=============================================================================
    Boost.Wave: A Standard compliant C++ preprocessor library

    Sample: IDL oriented preprocessor
    
    http://www.boost.org/

    Copyright (c) 2001-2010 Hartmut Kaiser. Distributed under the Boost
    Software License, Version 1.0. (See accompanying file
    LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
=============================================================================*/

#if !defined(IDL_HPP_FC7EE131_5CE9_43F2_A713_8D9BBC3C8477_INCLUDED)
#define IDL_HPP_FC7EE131_5CE9_43F2_A713_8D9BBC3C8477_INCLUDED

///////////////////////////////////////////////////////////////////////////////
//  This file may be used as a precompiled header (if applicable)

///////////////////////////////////////////////////////////////////////////////
//  include often used files from the stdlib
#include <iostream>
#include <fstream>
#include <string>
#include <vector>
#include <algorithm>
#include <iterator>

///////////////////////////////////////////////////////////////////////////////
//  include boost config
#include <boost/config.hpp>     //  global configuration information

///////////////////////////////////////////////////////////////////////////////
//  build version
#include "idl_version.hpp"

///////////////////////////////////////////////////////////////////////////////
//  configure this app here (global configuration constants)
#include "idl_config.hpp"

///////////////////////////////////////////////////////////////////////////////
//  include required boost libraries
#include <boost/assert.hpp>
#include <boost/pool/pool_alloc.hpp>

#endif // !defined(IDL_HPP_FC7EE131_5CE9_43F2_A713_8D9BBC3C8477_INCLUDED)
