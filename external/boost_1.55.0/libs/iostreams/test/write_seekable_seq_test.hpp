// (C) Copyright 2008 CodeRage, LLC (turkanis at coderage dot com)
// (C) Copyright 2004-2007 Jonathan Turkanis
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt.)

// See http://www.boost.org/libs/iostreams for documentation.


#ifndef BOOST_IOSTREAMS_TEST_WRITE_SEEKABLE_SEQUENCE_HPP_INCLUDED
#define BOOST_IOSTREAMS_TEST_WRITE_SEEKABLE_SEQUENCE_HPP_INCLUDED

#include <fstream>
#include <vector>
#include <boost/iostreams/filtering_stream.hpp>
#include <boost/range/iterator_range.hpp>
#include <boost/test/test_tools.hpp>
#include "detail/sequence.hpp"
#include "detail/temp_file.hpp"
#include "detail/verification.hpp"

void write_seekable_sequence_test()
{
    using namespace std;
    using namespace boost;
    using namespace boost::iostreams;
    using namespace boost::iostreams::test;

    test_file test;

    {
        vector<char> first(data_reps * data_length(), '?');
        filtering_stream<seekable> out(make_iterator_range(first), 0);
        write_data_in_chars(out);
        ifstream second(test.name().c_str());
        BOOST_CHECK_MESSAGE(
            compare_container_and_stream(first, second),
            "failed writing to filtering_stream<seekable> based on a "
            "sequence in chars with no buffer"
        );
    }

    {
        vector<char> first(data_reps * data_length(), '?');
        filtering_stream<seekable> out(make_iterator_range(first), 0);
        write_data_in_chunks(out);
        ifstream second(test.name().c_str());
        BOOST_CHECK_MESSAGE(
            compare_container_and_stream(first, second),
            "failed writing to filtering_stream<seekable> based on a "
            "sequence in chunks with no buffer"
        );
    }

    {
        vector<char> first(data_reps * data_length(), '?');
        filtering_stream<seekable> out(make_iterator_range(first));
        write_data_in_chars(out);
        ifstream second(test.name().c_str());
        BOOST_CHECK_MESSAGE(
            compare_container_and_stream(first, second),
            "failed writing to filtering_stream<seekable> based on a "
            "sequence in chars with large buffer"
        );
    }

    {
        vector<char> first(data_reps * data_length(), '?');
        filtering_stream<seekable> out(make_iterator_range(first));
        write_data_in_chunks(out);
        ifstream second(test.name().c_str());
        BOOST_CHECK_MESSAGE(
            compare_container_and_stream(first, second),
            "failed writing to filtering_stream<seekable> based on a "
            "sequence in chunks with large buffer"
        );
    }
}

#endif // #ifndef BOOST_IOSTREAMS_TEST_WRITE_SEEKABLE_SEQUENCE_HPP_INCLUDED
