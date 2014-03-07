// (C) Copyright 2008 CodeRage, LLC (turkanis at coderage dot com)
// (C) Copyright 2004-2007 Jonathan Turkanis
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt.)

// See http://www.boost.org/libs/iostreams for documentation.

#ifndef BOOST_IOSTREAMS_TEST_READ_INPUT_FILTER_HPP_INCLUDED
#define BOOST_IOSTREAMS_TEST_READ_INPUT_FILTER_HPP_INCLUDED

#include <fstream>
#include <boost/iostreams/device/file.hpp>
#include <boost/iostreams/filtering_stream.hpp>
#include <boost/test/test_tools.hpp>
#include "detail/filters.hpp"
#include "detail/sequence.hpp"
#include "detail/temp_file.hpp"
#include "detail/verification.hpp"

void read_input_filter_test()
{
    using namespace std;
    using namespace boost;
    using namespace boost::iostreams;
    using namespace boost::iostreams::test;

    test_file       test;
    uppercase_file  upper;

    {
        filtering_istream first;
        first.push(toupper_filter());
        first.push(file_source(test.name()));
        ifstream second(upper.name().c_str());
        BOOST_CHECK_MESSAGE(
            compare_streams_in_chars(first, second),
            "failed reading from a filtering_istream in chars with an "
            "input filter"
        );
    }

    {
        filtering_istream first;
        first.push(toupper_filter());
        first.push(file_source(test.name()));
        ifstream second(upper.name().c_str());
        BOOST_CHECK_MESSAGE(
            compare_streams_in_chunks(first, second),
            "failed reading from a filtering_istream in chunks with an "
            "input filter"
        );
    }

    {
        filtering_istream first;
        first.push(toupper_multichar_filter(), 0);
        first.push(file_source(test.name()));
        ifstream second(upper.name().c_str());
        BOOST_CHECK_MESSAGE(
            compare_streams_in_chars(first, second),
            "failed reading from a filtering_istream in chars with a "
            "multichar input filter with no buffer"
        );
    }

    {
        filtering_istream first;
        first.push(toupper_multichar_filter(), 0);
        first.push(file_source(test.name()));
        ifstream second(upper.name().c_str());
        BOOST_CHECK_MESSAGE(
            compare_streams_in_chunks(first, second),
            "failed reading from a filtering_istream in chunks with a "
            "multichar input filter with no buffer"
        );
    }

    {
        test_file src;
        filtering_istream first;
        first.push(toupper_multichar_filter());
        first.push(file_source(src.name()));
        ifstream second(upper.name().c_str());
        BOOST_CHECK_MESSAGE(
            compare_streams_in_chars(first, second),
            "failed reading from a filtering_istream in chars with a "
            "multichar input filter"
        );
    }

    {
        test_file src;
        filtering_istream first;
        first.push(toupper_multichar_filter());
        first.push(file_source(src.name()));
        ifstream second(upper.name().c_str());
        BOOST_CHECK_MESSAGE(
            compare_streams_in_chunks(first, second),
            "failed reading from a filtering_istream in chunks with a "
            "multichar input filter"
        );
    }
}

#endif // #ifndef BOOST_IOSTREAMS_TEST_READ_INPUT_FILTER_HPP_INCLUDED
