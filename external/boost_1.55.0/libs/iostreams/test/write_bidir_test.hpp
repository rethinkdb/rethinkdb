// (C) Copyright 2008 CodeRage, LLC (turkanis at coderage dot com)
// (C) Copyright 2004-2007 Jonathan Turkanis
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt.)

// See http://www.boost.org/libs/iostreams for documentation.

#ifndef BOOST_IOSTREAMS_TEST_WRITE_BIDIRECTIONAL_HPP_INCLUDED
#define BOOST_IOSTREAMS_TEST_WRITE_BIDIRECTIONAL_HPP_INCLUDED

#include <fstream>
#include <boost/iostreams/combine.hpp>
#include <boost/iostreams/device/file.hpp>
#include <boost/iostreams/filtering_stream.hpp>
#include <boost/test/test_tools.hpp>
#include "detail/temp_file.hpp"
#include "detail/verification.hpp"

void write_bidirectional_test()
{
    using namespace std;
    using namespace boost;
    using namespace boost::iostreams;
    using namespace boost::iostreams::test;

    test_file test;

    {
        temp_file                        dest;
        temp_file                        src; // Dummy;
        filtering_stream<bidirectional>  out(
           combine(file_source(src.name()), file_sink(dest.name(), out_mode)), 0
        );
        write_data_in_chars(out);
        out.reset();
        BOOST_CHECK_MESSAGE(
            compare_files(dest.name(), test.name()),
            "failed writing to filtering_stream<bidirectional> in chars "
            "with no buffer"
        );
    }

    {
        temp_file                        dest;
        temp_file                        src; // Dummy;
        filtering_stream<bidirectional>  out(
           combine(file_source(src.name()), file_sink(dest.name(), out_mode)), 0
        );
        write_data_in_chunks(out);
        out.reset();
        BOOST_CHECK_MESSAGE(
            compare_files(dest.name(), test.name()),
            "failed writing to filtering_stream<bidirectional> in chunks "
            "with no buffer"
        );
    }

    {
        temp_file                        dest;
        temp_file                        src; // Dummy;
        filtering_stream<bidirectional>  out(
           combine(file_source(src.name()), file_sink(dest.name(), out_mode))
        );
        write_data_in_chars(out);
        out.reset();
        BOOST_CHECK_MESSAGE(
            compare_files(dest.name(), test.name()),
            "failed writing to filtering_stream<bidirectional> in chars "
            "with large buffer"
        );
    }

    {
        temp_file                        dest;
        temp_file                        src; // Dummy;
        filtering_stream<bidirectional>  out(
           combine(file_source(src.name()), file_sink(dest.name(), out_mode))
        );
        write_data_in_chunks(out);
        out.reset();
        BOOST_CHECK_MESSAGE(
            compare_files(dest.name(), test.name()),
            "failed writing to filtering_stream<bidirectional> in chunks "
            "with large buffer"
        );
    }
}

#endif // #ifndef BOOST_IOSTREAMS_TEST_WRITE_BIDIRECTIONAL_HPP_INCLUDED
