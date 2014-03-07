// (C) Copyright 2008 CodeRage, LLC (turkanis at coderage dot com)
// (C) Copyright 2004-2007 Jonathan Turkanis
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt.)

// See http://www.boost.org/libs/iostreams for documentation.

#ifndef BOOST_IOSTREAMS_TEST_WRITE_OUTPUT_FILTER_HPP_INCLUDED
#define BOOST_IOSTREAMS_TEST_WRITE_OUTPUT_FILTER_HPP_INCLUDED

#include <fstream>
#include <boost/iostreams/device/file.hpp>
#include <boost/iostreams/filtering_stream.hpp>
#include <boost/test/test_tools.hpp>
#include "detail/filters.hpp"
#include "detail/sequence.hpp"
#include "detail/temp_file.hpp"
#include "detail/verification.hpp"

void write_output_filter_test()
{
    using namespace std;
    using namespace boost;
    using namespace boost::iostreams;
    using namespace boost::iostreams::test;

    lowercase_file lower;

    {
        temp_file          dest;
        filtering_ostream  out;
        out.push(tolower_filter());
        out.push(file_sink(dest.name(), out_mode));
        write_data_in_chars(out);
        out.reset();
        BOOST_CHECK_MESSAGE(
            compare_files(dest.name(), lower.name()),
            "failed writing to a filtering_ostream in chars with an "
            "output filter"
        );
    }

    {
        temp_file          dest;
        filtering_ostream  out;
        out.push(tolower_filter());
        out.push(file_sink(dest.name(), out_mode));
        write_data_in_chunks(out);
        out.reset();
        BOOST_CHECK_MESSAGE(
            compare_files(dest.name(), lower.name()),
            "failed writing to a filtering_ostream in chunks with an "
            "output filter"
        );
    }

    {
        temp_file          dest;
        filtering_ostream  out;
        out.push(tolower_multichar_filter(), 0);
        out.push(file_sink(dest.name(), out_mode));
        write_data_in_chars(out);
        out.reset();
        BOOST_CHECK_MESSAGE(
            compare_files(dest.name(), lower.name()),
            "failed writing to a filtering_ostream in chars with a "
            "multichar output filter with no buffer"
        );
    }

    {
        temp_file          dest;
        filtering_ostream  out;
        out.push(tolower_multichar_filter(), 0);
        out.push(file_sink(dest.name(), out_mode));
        write_data_in_chunks(out);
        out.reset();
        BOOST_CHECK_MESSAGE(
            compare_files(dest.name(), lower.name()),
            "failed writing to a filtering_ostream in chunks with a "
            "multichar output filter with no buffer"
        );
    }

    {
        temp_file          dest;
        filtering_ostream  out;
        out.push(tolower_multichar_filter());
        out.push(file_sink(dest.name(), out_mode));
        write_data_in_chars(out);
        out.reset();
        BOOST_CHECK_MESSAGE(
            compare_files(dest.name(), lower.name()),
            "failed writing to a filtering_ostream in chars with a "
            "multichar output filter"
        );
    }

    {
        temp_file          dest;
        filtering_ostream  out;
        out.push(tolower_multichar_filter());
        out.push(file_sink(dest.name(), out_mode));
        write_data_in_chunks(out);
        out.reset();
        BOOST_CHECK_MESSAGE(
            compare_files(dest.name(), lower.name()),
            "failed writing to a filtering_ostream in chunks with a "
            "multichar output filter"
        );
    }
}

#endif // #ifndef BOOST_IOSTREAMS_TEST_WRITE_OUTPUT_FILTER_HPP_INCLUDED
