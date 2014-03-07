// (C) Copyright 2008 CodeRage, LLC (turkanis at coderage dot com)
// (C) Copyright 2004-2007 Jonathan Turkanis
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt.)

// See http://www.boost.org/libs/iostreams for documentation.

#include <boost/iostreams/detail/fstream.hpp>
#include <boost/iostreams/device/array.hpp>
#include <boost/iostreams/stream.hpp>
#include <boost/test/test_tools.hpp>
#include <boost/test/unit_test.hpp>
#include "detail/sequence.hpp"
#include "detail/temp_file.hpp"
#include "detail/verification.hpp"

using boost::unit_test::test_suite;      

void array_test()
{
    using namespace std;
    using namespace boost::iostreams;
    using namespace boost::iostreams::test;

    test_file test;

    //--------------stream<array_source>-------------------------------//

    {
        test_sequence<> seq;
        stream<array_source> first(&seq[0], &seq[0] + seq.size());
        ifstream second(test.name().c_str(), BOOST_IOS::in | BOOST_IOS::binary);
        BOOST_CHECK_MESSAGE(
            compare_streams_in_chars(first, second),
            "failed reading from stream<array_source> in chars"
        );
    }

    {
        test_sequence<> seq;
        stream<array_source> first(&seq[0], &seq[0] + seq.size());
        ifstream second(test.name().c_str(), BOOST_IOS::in | BOOST_IOS::binary);
        BOOST_CHECK_MESSAGE(
            compare_streams_in_chunks(first, second),
            "failed reading from stream<array_source> in chunks"
        );
    }

    //--------------stream<array_sink>---------------------------------//

    {
        vector<char> first(data_reps * data_length(), '?');
        stream<array_sink> out(&first[0], &first[0] + first.size());
        write_data_in_chars(out);
        ifstream second(test.name().c_str(), BOOST_IOS::in | BOOST_IOS::binary);
        BOOST_CHECK_MESSAGE(
            compare_container_and_stream(first, second),
            "failed writing to stream<array_sink> in chars"
        );
    }

    {
        vector<char> first(data_reps * data_length(), '?');
        stream<array_sink> out(&first[0], &first[0] + first.size());
        write_data_in_chunks(out);
        ifstream second(test.name().c_str(), BOOST_IOS::in | BOOST_IOS::binary);
        BOOST_CHECK_MESSAGE(
            compare_container_and_stream(first, second),
            "failed writing to stream<array_sink> in chunks"
        );
    }

    //--------------random access---------------------------------------------//

    {
        vector<char> first(data_reps * data_length(), '?');
        stream<boost::iostreams::array> io(&first[0], &first[0] + first.size());
        BOOST_CHECK_MESSAGE(
            test_seekable_in_chars(io),
            "failed seeking within stream<array>, in chars"
        );
    }

    {
        vector<char> first(data_reps * data_length(), '?');
        stream<boost::iostreams::array> io(&first[0], &first[0] + first.size());
        BOOST_CHECK_MESSAGE(
            test_seekable_in_chars(io),
            "failed seeking within stream<array>, in chunks"
        );
    }
}

test_suite* init_unit_test_suite(int, char* []) 
{
    test_suite* test = BOOST_TEST_SUITE("array test");
    test->add(BOOST_TEST_CASE(&array_test));
    return test;
}
