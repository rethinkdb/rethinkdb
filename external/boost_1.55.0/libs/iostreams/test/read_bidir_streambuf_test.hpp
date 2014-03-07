// (C) Copyright 2008 CodeRage, LLC (turkanis at coderage dot com)
// (C) Copyright 2004-2007 Jonathan Turkanis
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt.)

// See http://www.boost.org/libs/iostreams for documentation.

#ifndef BOOST_IOSTREAMS_TEST_READ_BIDIRECTIONAL_STREAMBUF_HPP_INCLUDED
#define BOOST_IOSTREAMS_TEST_READ_BIDIRECTIONAL_STREAMBUF_HPP_INCLUDED

#include <fstream>
#include <boost/iostreams/device/file.hpp>
#include <boost/iostreams/filtering_stream.hpp>
#include <boost/test/test_tools.hpp>
#include "detail/temp_file.hpp"
#include "detail/verification.hpp"

void read_bidirectional_streambuf_test()
{
    using namespace std;
    using namespace boost;
    using namespace boost::iostreams;
    using namespace boost::iostreams::test;

    test_file  test1;       
    test_file  test2;            

    {
        filebuf fb;
        fb.open(test1.name().c_str(), BOOST_IOS::in);
        filtering_stream<bidirectional> first(fb, 0);
        ifstream second(test2.name().c_str());
        BOOST_CHECK_MESSAGE(
            compare_streams_in_chars(first, second),
            "failed reading from filtering_stream<bidirectional> based on a"
            "streambuf in chars with no buffer"
        );
    }

    {
        filebuf fb;
        fb.open(test1.name().c_str(), BOOST_IOS::in);
        filtering_stream<bidirectional> first(fb, 0);
        ifstream second(test2.name().c_str());
        BOOST_CHECK_MESSAGE(
            compare_streams_in_chunks(first, second),
            "failed reading from filtering_stream<bidirectional> based on a"
            "streambuf in chunks with no buffer"
        );
    }

    {
        filebuf fb;
        fb.open(test1.name().c_str(), BOOST_IOS::in);
        filtering_stream<bidirectional> first(fb);
        ifstream second(test2.name().c_str());
        BOOST_CHECK_MESSAGE(
            compare_streams_in_chars(first, second),
            "failed reading from filtering_stream<bidirectional> based on a"
            "streambuf in chars with large buffer"
        );
    }

    {
        filebuf fb;
        fb.open(test1.name().c_str(), BOOST_IOS::in);
        filtering_stream<bidirectional> first(fb);
        ifstream second(test2.name().c_str());
        BOOST_CHECK_MESSAGE(
            compare_streams_in_chunks(first, second),
            "failed reading from filtering_stream<bidirectional> based on a"
            "streambuf in chunks with large buffer"
        );
    }
}

#endif // #ifndef BOOST_IOSTREAMS_TEST_READ_BIDIRECTIONAL_STREAMBUF_HPP_INCLUDED
