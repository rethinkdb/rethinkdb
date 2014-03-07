#include <boost/config.hpp>

#if defined(BOOST_MSVC)
#pragma warning(disable: 4786)  // identifier truncated in debug info
#pragma warning(disable: 4710)  // function not inlined
#pragma warning(disable: 4711)  // function selected for automatic inline expansion
#pragma warning(disable: 4514)  // unreferenced inline removed
#endif

//
//  current_function_test.cpp - a test for boost/current_function.hpp
//
//  Copyright (c) 2002 Peter Dimov and Multi Media Ltd.
//
// Distributed under the Boost Software License, Version 1.0. (See
// accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)
//

#include <boost/current_function.hpp>
#include <boost/config.hpp>
#include <cstdio>

void message(char const * file, long line, char const * func, char const * msg)
{
#if !defined(BOOST_NO_STDC_NAMESPACE)
    using std::printf;
#endif

    printf("%s(%ld): %s in function '%s'\n", file, line, msg, func);
}

#define MESSAGE(msg) message(__FILE__, __LINE__, BOOST_CURRENT_FUNCTION, msg)

int main()
{
    MESSAGE("assertion failed");

    return 0;
}
