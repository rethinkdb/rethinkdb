//  Unit test for boost::lexical_cast.
//
//  See http://www.boost.org for most recent version, including documentation.
//
//  Copyright Antony Polukhin, 2013.
//
//  Distributed under the Boost
//  Software License, Version 1.0. (See accompanying file
//  LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt).
//
// Test lexical_cast usage with long filesystem::path. Bug 7704.

#include <boost/config.hpp>

#include <boost/test/unit_test.hpp>
#include <boost/lexical_cast.hpp>
#include <boost/filesystem/path.hpp>

using namespace boost;

void test_filesystem();

unit_test::test_suite *init_unit_test_suite(int, char *[])
{
    unit_test::test_suite *suite =
        BOOST_TEST_SUITE("lexical_cast unit test");
    suite->add(BOOST_TEST_CASE(&test_filesystem));

    return suite;
}

void test_filesystem()
{
    boost::filesystem::path p;
    std::string s1 = "aaaaaaaaaaaaaaaaaaaaaaa";
    p = boost::lexical_cast<boost::filesystem::path>(s1);
    BOOST_CHECK(!p.empty());
    BOOST_CHECK_EQUAL(p, s1);
    p.clear();

    const char ab[] = "aaaaaaaaaaaaaaaaaaaaaaabbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbb";
    p = boost::lexical_cast<boost::filesystem::path>(ab);
    BOOST_CHECK(!p.empty());
    BOOST_CHECK_EQUAL(p, ab);
}

