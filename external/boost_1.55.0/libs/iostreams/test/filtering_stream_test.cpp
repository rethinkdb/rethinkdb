// (C) Copyright 2008 CodeRage, LLC (turkanis at coderage dot com)
// (C) Copyright 2004-2007 Jonathan Turkanis
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt.)

// See http://www.boost.org/libs/iostreams for documentation.

#include <boost/test/unit_test.hpp>
#include "read_input_test.hpp"
#include "read_bidir_test.hpp"
#include "read_seekable_test.hpp"
#include "read_bidir_streambuf_test.hpp"
#include "read_input_istream_test.hpp"
#include "write_output_test.hpp"
#include "write_bidir_test.hpp"
#include "write_seekable_test.hpp"
#include "write_output_iterator_test.hpp"
#include "write_bidir_streambuf_test.hpp"
#include "write_output_ostream_test.hpp"
#include "read_input_filter_test.hpp"
#include "read_bidir_filter_test.hpp"
#include "write_output_filter_test.hpp"
#include "write_bidir_filter_test.hpp"
#include "seek_test.hpp"
#include "putback_test.hpp"
#include "filtering_stream_flush_test.hpp"

using boost::unit_test::test_suite;

test_suite* init_unit_test_suite(int, char* [])
{
    test_suite* test = BOOST_TEST_SUITE("filtering_stream test");
    test->add(BOOST_TEST_CASE(&read_input_test));
    test->add(BOOST_TEST_CASE(&read_bidirectional_test));
    test->add(BOOST_TEST_CASE(&read_seekable_test));
    test->add(BOOST_TEST_CASE(&read_bidirectional_streambuf_test));
    test->add(BOOST_TEST_CASE(&read_input_istream_test));
    test->add(BOOST_TEST_CASE(&write_output_test));
    test->add(BOOST_TEST_CASE(&write_bidirectional_test));
    test->add(BOOST_TEST_CASE(&write_seekable_test));
    test->add(BOOST_TEST_CASE(&write_output_iterator_test));
    test->add(BOOST_TEST_CASE(&write_bidirectional_streambuf_test));
    test->add(BOOST_TEST_CASE(&write_output_ostream_test));
    test->add(BOOST_TEST_CASE(&read_input_filter_test));
    test->add(BOOST_TEST_CASE(&read_bidirectional_filter_test));
    test->add(BOOST_TEST_CASE(&write_output_filter_test));
    test->add(BOOST_TEST_CASE(&write_bidirectional_filter_test));
    test->add(BOOST_TEST_CASE(&seek_test));
    test->add(BOOST_TEST_CASE(&putback_test));
    test->add(BOOST_TEST_CASE(&test_filtering_ostream_flush));
    return test;
}
