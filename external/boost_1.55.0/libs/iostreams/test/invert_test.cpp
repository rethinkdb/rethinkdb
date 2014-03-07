// (C) Copyright 2008 CodeRage, LLC (turkanis at coderage dot com)
// (C) Copyright 2004-2007 Jonathan Turkanis
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt.)

// See http://www.boost.org/libs/iostreams for documentation.

#include <boost/iostreams/device/file.hpp>
#include <boost/iostreams/filter/test.hpp>
#include <boost/iostreams/invert.hpp>
#include <boost/test/test_tools.hpp>
#include <boost/test/unit_test.hpp>
#include "detail/closable.hpp"
#include "detail/filters.hpp"
#include "detail/operation_sequence.hpp"
#include "detail/temp_file.hpp"

using namespace boost::iostreams;
using namespace boost::iostreams::test;
using boost::unit_test::test_suite;
namespace io = boost::iostreams;

void read_write_test()
{

    test_file       test;
    lowercase_file  lower;
    uppercase_file  upper;

    BOOST_CHECK( test_input_filter(
                    invert(tolower_filter()),
                    file_source(test.name(), in_mode),
                    file_source(lower.name(), in_mode) ) );

    BOOST_CHECK( test_output_filter(
                    invert(toupper_filter()),
                    file_source(test.name(), in_mode),
                    file_source(upper.name(), in_mode) ) );
}

void close_test()
{
    // Invert an output filter
    {
        operation_sequence  seq;
        chain<input>        ch;
        ch.push(io::invert(closable_filter<output>(seq.new_operation(2))));
        ch.push(closable_device<input>(seq.new_operation(1)));
        BOOST_CHECK_NO_THROW(ch.reset());
        BOOST_CHECK_OPERATION_SEQUENCE(seq);
    }

    // Invert an input filter
    {
        operation_sequence  seq;
        chain<output>       ch;
        ch.push(io::invert(closable_filter<input>(seq.new_operation(1))));
        ch.push(closable_device<output>(seq.new_operation(2)));
        BOOST_CHECK_NO_THROW(ch.reset());
        BOOST_CHECK_OPERATION_SEQUENCE(seq);
    }
}

test_suite* init_unit_test_suite(int, char* []) 
{
    test_suite* test = BOOST_TEST_SUITE("reverse test");
    test->add(BOOST_TEST_CASE(&read_write_test));
    test->add(BOOST_TEST_CASE(&close_test));
    return test;
}
