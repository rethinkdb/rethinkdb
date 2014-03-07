///////////////////////////////////////////////////////////////////////////////
// test_dynamic.cpp
//
//  Copyright 2008 Eric Niebler. Distributed under the Boost
//  Software License, Version 1.0. (See accompanying file
//  LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

#include <boost/xpressive/xpressive_dynamic.hpp>
#include <boost/test/unit_test.hpp>

///////////////////////////////////////////////////////////////////////////////
// test_main
void test_main()
{
    using namespace boost::xpressive;

    std::string str("bar");
    sregex rx = sregex::compile("b.*ar");
    smatch what;

    if(!regex_match(str, what, rx))
    {
        BOOST_ERROR("oops");
    }
}

using namespace boost::unit_test;

///////////////////////////////////////////////////////////////////////////////
// init_unit_test_suite
//
test_suite* init_unit_test_suite( int argc, char* argv[] )
{
    test_suite *test = BOOST_TEST_SUITE("test_dynamic");
    test->add(BOOST_TEST_CASE(&test_main));
    return test;
}

