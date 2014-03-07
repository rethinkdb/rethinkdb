// Copyright David Abrahams 2003.
// Distributed under the Boost Software License, Version 1.0. (See
// accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)

#include <boost/implicit_cast.hpp>
#include <boost/type.hpp>

#define BOOST_INCLUDE_MAIN
#include <boost/test/test_tools.hpp>

using boost::implicit_cast;

struct foo
{
    explicit foo(char const*) {}
};

int test_main(int, char*[])
{
    foo x = implicit_cast<foo>("foobar");
    (void)x;            // warning suppression.
    BOOST_CHECK(false); // suppressing warning about 'boost::unit_test::{anonymous}::unit_test_log' defined but not used
    return 0;
}

