// (C) Copyright 2009 Andrew Sutton
//
// Use, modification and distribution are subject to the
// Boost Software License, Version 1.0 (See accompanying file
// LICENSE_1_0.txt or http://www.boost.org/LICENSE_1_0.txt)

#ifndef ORIGIN_TYPESTR_HPP
#define ORIGIN_TYPESTR_HPP

#include <string>
#include <cstring>
#include <typeinfo>

#if defined(__GNUC__)
#include <cxxabi.h>
#endif

template<typename T> struct type_name { };

/**
 * Return a string that describes the type of the given template parameter.
 * The type name depends on the results of the typeid operator.
 *
 * @todo Rewrite this so that demangle will dynamically allocate the memory.
 */
template <typename T>
std::string typestr() {
#if defined(__GNUC__)
    std::size_t const BUFSIZE = 8192;
    std::size_t n = BUFSIZE;
    char buf[BUFSIZE];
    abi::__cxa_demangle(typeid(type_name<T>).name(), buf, &n, 0);
    return std::string(buf, ::strlen(buf));
#else
    return typeid(type_name<T>).name();
#endif
}

/**
 * Return a string that describes the type of the given parameter. The type
 * name depends on the results of the typeid operator.
 */
template <typename T>
inline std::string typestr(T const&)
{ return typestr<T>(); }

#endif
