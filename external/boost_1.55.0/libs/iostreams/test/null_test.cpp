// (C) Copyright 2008 CodeRage, LLC (turkanis at coderage dot com)
// (C) Copyright 2004-2007 Jonathan Turkanis
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt.)

// See http://www.boost.org/libs/iostreams for documentation.

#include <cctype>
#include <boost/iostreams/device/null.hpp>
#include <boost/iostreams/stream.hpp>
#include <boost/test/test_tools.hpp>
#include <boost/test/unit_test.hpp>  
#include "detail/temp_file.hpp"
#include "detail/verification.hpp"

using namespace std;
using namespace boost;
using namespace boost::iostreams;
using namespace boost::iostreams::test;
using boost::unit_test::test_suite;    

void read_null_source()
{
    stream<null_source> in;
    in.open(null_source());
    in.get();
    BOOST_CHECK(in.eof());
}

void write_null_sink() 
{
    stream<null_sink> out;
    out.open(null_sink());
    write_data_in_chunks(out);
    BOOST_CHECK(out.good());
}

test_suite* init_unit_test_suite(int, char* []) 
{
    test_suite* test = BOOST_TEST_SUITE("null test");
    test->add(BOOST_TEST_CASE(&read_null_source));
    test->add(BOOST_TEST_CASE(&write_null_sink));
    return test;
}
