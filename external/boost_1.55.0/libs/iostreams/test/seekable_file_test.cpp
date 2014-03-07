// (C) Copyright 2008 CodeRage, LLC (turkanis at coderage dot com)
// (C) Copyright 2004-2007 Jonathan Turkanis
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt.)

// See http://www.boost.org/libs/iostreams for documentation. 

#include <fstream>
#include <boost/iostreams/device/file.hpp>
#include <boost/iostreams/filtering_stream.hpp>
#include <boost/test/test_tools.hpp>
#include <boost/test/unit_test.hpp>
#include "detail/temp_file.hpp"
#include "detail/verification.hpp"

using namespace std;
using namespace boost;
using namespace boost::iostreams;
using namespace boost::iostreams::test;
using boost::unit_test::test_suite;     

void seekable_file_test()
{
    {
        temp_file temp;
        file f( temp.name(), 
                BOOST_IOS::in | BOOST_IOS::out | 
                BOOST_IOS::trunc | BOOST_IOS::binary);
        filtering_stream<seekable> io(f);
        io.exceptions(BOOST_IOS::failbit | BOOST_IOS::badbit);
        BOOST_CHECK_MESSAGE(
            test_seekable_in_chars(io),
            "failed seeking within a file, in chars"
        );
    }

    {
        temp_file temp;
        file f( temp.name(), 
                BOOST_IOS::in | BOOST_IOS::out | 
                BOOST_IOS::trunc | BOOST_IOS::binary);
        filtering_stream<seekable> io(f);
        io.exceptions(BOOST_IOS::failbit | BOOST_IOS::badbit);
        BOOST_CHECK_MESSAGE(
            test_seekable_in_chunks(io),
            "failed seeking within a file, in chunks"
        );
    }
}

test_suite* init_unit_test_suite(int, char* []) 
{
    test_suite* test = BOOST_TEST_SUITE("seekable file test");
    test->add(BOOST_TEST_CASE(&seekable_file_test));
    return test;
}
