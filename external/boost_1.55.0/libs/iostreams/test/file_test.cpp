// (C) Copyright 2008 CodeRage, LLC (turkanis at coderage dot com)
// (C) Copyright 2004-2007 Jonathan Turkanis
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt.)

// See http://www.boost.org/libs/iostreams for documentation.

#include <boost/iostreams/device/file.hpp>
#include <boost/test/test_tools.hpp>
#include <boost/test/unit_test.hpp>
#include "detail/temp_file.hpp"
#include "detail/verification.hpp"

using namespace boost;
using namespace boost::iostreams;
using namespace boost::iostreams::test;
using std::ifstream;
using boost::unit_test::test_suite;   

void file_test()
{
    test_file  test;   
                    
    //--------------Test file_source------------------------------------------//

    {
        file_source f(test.name());
        BOOST_CHECK(f.is_open());
        f.close();
        BOOST_CHECK(!f.is_open());
        f.open(test.name());
        BOOST_CHECK(f.is_open());
    }

    //--------------Test file_sink--------------------------------------------//
                                                    
    {
        file_sink f(test.name());
        BOOST_CHECK(f.is_open());
        f.close();
        BOOST_CHECK(!f.is_open());
        f.open(test.name());
        BOOST_CHECK(f.is_open());
    }

    //--------------Test file-------------------------------------------------//
                                                    
    {
        file f(test.name());
        BOOST_CHECK(f.is_open());
        f.close();
        BOOST_CHECK(!f.is_open());
        f.open(test.name());
        BOOST_CHECK(f.is_open());
    }
}

test_suite* init_unit_test_suite(int, char* []) 
{
    test_suite* test = BOOST_TEST_SUITE("file test");
    test->add(BOOST_TEST_CASE(&file_test));
    return test;
}
