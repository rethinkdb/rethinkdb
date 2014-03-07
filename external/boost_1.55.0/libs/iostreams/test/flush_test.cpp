// (C) Copyright 2008 CodeRage, LLC (turkanis at coderage dot com)
// (C) Copyright 2004-2007 Jonathan Turkanis
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt.)

// See http://www.boost.org/libs/iostreams for documentation.

#include <algorithm>  // equal.
#include <fstream>
#include <boost/iostreams/device/back_inserter.hpp>
#include <boost/iostreams/device/file.hpp>
#include <boost/iostreams/device/null.hpp>
#include <boost/iostreams/filtering_stream.hpp>
#include <boost/iostreams/stream.hpp>
#include <boost/iostreams/operations.hpp>
#include <boost/test/test_tools.hpp>
#include <boost/test/unit_test.hpp>  
#include "detail/filters.hpp"
#include "detail/temp_file.hpp"
#include "detail/verification.hpp"

using namespace std;
using namespace boost;
using namespace boost::iostreams;
using namespace boost::iostreams::test;
using boost::unit_test::test_suite;

void flush_test()
{
    {
        stream_buffer<null_sink> null;
        null.open(null_sink());
        BOOST_CHECK_MESSAGE( 
            iostreams::flush(null), 
            "failed flushing stream_buffer"
        );
        BOOST_CHECK_MESSAGE( 
            null.strict_sync(), 
            "failed strict-syncing stream_buffer with "
            "non-flushable resource"
        );
    }

    {
        stream<null_sink> null;
        null.open(null_sink());
        BOOST_CHECK_MESSAGE( 
            iostreams::flush(null), 
            "failed flushing stream"
        );
        BOOST_CHECK_MESSAGE( 
            null.strict_sync(), 
            "failed strict-syncing stream with "
            "non-flushable resource"
        );
    }

    {
        filtering_ostream null;
        null.push(null_sink());
        BOOST_CHECK_MESSAGE( 
            iostreams::flush(null), 
            "failed flushing filtering_ostream"
        );
        BOOST_CHECK_MESSAGE( 
            null.strict_sync(), 
            "failed strict-syncing filtering_ostream with "
            "non-flushable resource"
        );
    }

    {
        filtering_ostream null;
        null.push(tolower_filter());
        null.push(null_sink());
        BOOST_CHECK_MESSAGE( 
            iostreams::flush(null), 
            "failed flushing filtering_ostream with non-flushable filter"
        );
        BOOST_CHECK_MESSAGE( 
            !null.strict_sync(), 
            "strict-syncing filtering_ostream with "
            "non-flushable filter succeeded"
        );
    }

    {
        vector<char> dest1;
        vector<char> dest2;
        filtering_ostream out;
        out.set_auto_close(false);
        out.push(flushable_output_filter());

        // Write to dest1.
        out.push(iostreams::back_inserter(dest1));
        write_data_in_chunks(out);
        out.flush();

        // Write to dest2.
        out.pop();
        out.push(iostreams::back_inserter(dest2));
        write_data_in_chunks(out);
        out.flush();

        BOOST_CHECK_MESSAGE(
            dest1.size() == dest2.size() && 
            std::equal(dest1.begin(), dest1.end(), dest2.begin()),
            "failed flush filtering_ostream with auto_close disabled"
        );
    }

    {
        vector<char> dest1;
        vector<char> dest2;
        filtering_ostream out;
        out.set_auto_close(false);
        out.push(flushable_output_filter());
        out.push(flushable_output_filter());

        // Write to dest1.
        out.push(iostreams::back_inserter(dest1));
        write_data_in_chunks(out);
        out.flush();

        // Write to dest2.
        out.pop();
        out.push(iostreams::back_inserter(dest2));
        write_data_in_chunks(out);
        out.flush();

        BOOST_CHECK_MESSAGE(
            dest1.size() == dest2.size() && 
            std::equal(dest1.begin(), dest1.end(), dest2.begin()),
            "failed flush filtering_ostream with two flushable filters "
            "with auto_close disabled"
        );
    }
}

test_suite* init_unit_test_suite(int, char* []) 
{
    test_suite* test = BOOST_TEST_SUITE("flush test");
    test->add(BOOST_TEST_CASE(&flush_test));
    return test;
}
