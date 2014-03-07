
// Copyright 2006-2009 Daniel James.
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

#if !defined(BOOST_UNORDERED_TEST_HELPERS_FWD_HEADER)
#define BOOST_UNORDERED_TEST_HELPERS_FWD_HEADER

#include <string>

namespace test
{
    int generate(int const*);
    char generate(char const*);
    signed char generate(signed char const*);
    std::string generate(std::string*);
    float generate(float const*);

    struct base_type {} base;
    struct derived_type : base_type {} derived;
}

#endif

