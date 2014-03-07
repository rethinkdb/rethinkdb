// (C) Copyright 2008 CodeRage, LLC (turkanis at coderage dot com)
// (C) Copyright 2007 Jonathan Turkanis
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt.)

// See http://www.boost.org/libs/iostreams for documentation.

#include <boost/test/unit_test.hpp>
#include "read_input_seq_test.hpp"
#include "read_seekable_seq_test.hpp"
#include "write_output_seq_test.hpp"
#include "write_seekable_seq_test.hpp"

using boost::unit_test::test_suite;

test_suite* init_unit_test_suite(int, char* [])
{
    test_suite* test = BOOST_TEST_SUITE("sequence test");
    test->add(BOOST_TEST_CASE(&read_input_sequence_test));
    test->add(BOOST_TEST_CASE(&read_seekable_sequence_test));
    test->add(BOOST_TEST_CASE(&write_output_sequence_test));
    test->add(BOOST_TEST_CASE(&write_seekable_sequence_test));
    return test;
}
