/*=============================================================================
    Boost.Wave: A Standard compliant C++ preprocessor library
    http://www.boost.org/

    Copyright (c) 2001-2012 Hartmut Kaiser. Distributed under the Boost
    Software License, Version 1.0. (See accompanying file
    LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

    Some of the tests included in this file were initially taken from the mcpp 
    V2.5 preprocessor validation suite and were modified to fit into the 
    Boost.Wave unit test requirements.
    The original files of the mcpp preprocessor are distributed under the 
    license reproduced at the end of this file.
=============================================================================*/

// Tests the #include directive (need to switch off include guard detection).
//O --noguard

// 6.1: Header-name quoted by " and " as well as by < and > can include
//      standard headers.
//R #line 23 "t_5_007.cpp"
#include "boost/version.hpp"
BOOST_LIB_VERSION           //R "$V" 

//R #line 29 "t_5_007.cpp"
#undef BOOST_VERSION_HPP
#undef BOOST_LIB_VERSION
#include <boost/version.hpp>
BOOST_LIB_VERSION           //R "$V" 

// 6.2: Macro is allowed in #include line.
//R #line 36 "t_5_007.cpp"
#undef MACRO_005_007
#define HEADER  "t_5_007.hpp"
#include HEADER
MACRO_005_007               //R abc 

// 6.3: With macro nonsense but legal.
//R #line 43 "t_5_007.cpp"
#undef MACRO_005_007
#define ZERO_TOKEN
#include ZERO_TOKEN HEADER ZERO_TOKEN
MACRO_005_007               //R abc 

/*-
 * Copyright (c) 1998, 2002-2005 Kiyoshi Matsui <kmatsui@t3.rim.or.jp>
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE AUTHOR BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 */

