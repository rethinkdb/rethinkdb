#ifndef BOOST_TEST_DECL_HPP
#define BOOST_TEST_DECL_HPP 

// MS compatible compilers support #pragma once
#if defined(_MSC_VER) && (_MSC_VER >= 1020)
# pragma once
#endif 

/////////1/////////2///////// 3/////////4/////////5/////////6/////////7/////////8
//  test_decl.hpp
//
//  (c) Copyright Robert Ramey 2004
//  Use, modification, and distribution is subject to the Boost Software
//  License, Version 1.0. (See accompanying file LICENSE_1_0.txt or copy at
//  http://www.boost.org/LICENSE_1_0.txt)
//
//  See library home page at http://www.boost.org/libs/serialization
//  export if this is our own source, otherwise import:

// usage:
//
// class header declarations should look something like:
//
// #include "test_decl.hpp"
// #if defined(A_CPP)
//     #define DLL_DECL IMPORT_DECL(BOOST_PP_EMPTY())
// #else
//     #define DLL_DECL EXPORT_DECL(BOOST_PP_EMPTY())
// #endif
//
// class DLL_DECL A {
//     ...
// };
//
// #undef DLL_DECL
//
// code which includes such headers should look something like:
//
// #include "A.hpp"
//
// code which builds dll should like like
//
// #define A_CPP
// #include "A.hpp"
//
// A::A(){
//  ...
// }
// ...
//

#include <boost/config.hpp>

#ifdef BOOST_HAS_DECLSPEC // defined in config system
    #if ! defined(EXPORT_DECL)
        #if defined(__BORLANDC__)
            #define EXPORT_DECL(T)  T __export
        #else
            #define EXPORT_DECL(T) __declspec(dllexport) T 
        #endif
    #endif
    #if ! defined(IMPORT_DECL)
        #if defined(__BORLANDC__)
            #define IMPORT_DECL(T)    T __import
        #else
            #define IMPORT_DECL(T)    __declspec(dllimport) T 
        #endif
    #endif
#else
    #define IMPORT_DECL(T) T
    #define EXPORT_DECL(T) T
#endif // BOOST_HAS_DECLSPEC

#endif // BOOST_TEST_DECL_HPP
