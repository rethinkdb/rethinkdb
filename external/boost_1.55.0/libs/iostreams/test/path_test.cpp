// (C) Copyright Daniel James 2011.
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt.)

// See http://www.boost.org/libs/iostreams for documentation.

#include <boost/iostreams/detail/path.hpp>
#include <boost/filesystem/path.hpp>

#include <boost/test/test_tools.hpp>
#include <boost/test/unit_test.hpp>

void path_test()
{
    boost::filesystem::path orig("a/b");
    boost::iostreams::detail::path p(orig);
    p = orig;
}

boost::unit_test::test_suite* init_unit_test_suite(int, char* []) 
{
    boost::unit_test::test_suite* test = BOOST_TEST_SUITE("mapped_file test");
    test->add(BOOST_TEST_CASE(&path_test));
    return test;
}
