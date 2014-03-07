// (C) Copyright 2008 CodeRage, LLC (turkanis at coderage dot com)
// (C) Copyright 2004-2007 Jonathan Turkanis
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt.)

// See http://www.boost.org/libs/iostreams for documentation.

#ifndef BOOST_IOSTREAMS_TEST_WRITE_SEEKABLE_HPP_INCLUDED
#define BOOST_IOSTREAMS_TEST_WRITE_SEEKABLE_HPP_INCLUDED

#include <fstream>
#include <boost/iostreams/device/file.hpp>
#include <boost/iostreams/filtering_stream.hpp>
#include <boost/test/test_tools.hpp>
#include "detail/temp_file.hpp"
#include "detail/verification.hpp"

void write_seekable_test()
{
    using namespace std;
    using namespace boost;
    using namespace boost::iostreams;
    using namespace boost::iostreams::test;

    test_file test;
    BOOST_IOS::openmode mode = out_mode | BOOST_IOS::trunc;

    {
        temp_file                   test2;
        filtering_stream<seekable>  out(file(test2.name(), mode), 0);
        write_data_in_chars(out);
        out.reset();
        BOOST_CHECK_MESSAGE(
            compare_files(test2.name(), test.name()),
            "failed writing to filtering_stream<seekable> in chars with"
            "no buffer"
        );
    }

    {
        temp_file                   test2;
        filtering_stream<seekable>  out(file(test2.name(), mode), 0);
        write_data_in_chunks(out);
        out.reset();
        BOOST_CHECK_MESSAGE(
            compare_files(test2.name(), test.name()),
            "failed writing to filtering_stream<seekable> in chunks with"
            "no buffer"
        );
    }

    {
        temp_file                   test2;
        filtering_stream<seekable>  out(file(test2.name(), mode));
        write_data_in_chars(out);
        out.reset();
        BOOST_CHECK_MESSAGE(
            compare_files(test2.name(), test.name()),
            "failed writing to filtering_stream<seekable> in chars with"
            "large buffer"
        );
    }

    {
        temp_file                   test2;
        filtering_stream<seekable>  out(file(test2.name(), mode));
        write_data_in_chunks(out);
        out.reset();
        BOOST_CHECK_MESSAGE(
            compare_files(test2.name(), test.name()),
            "failed writing to filtering_stream<seekable> in chunks with"
            "large buffer"
        );
    }
}

#endif // #ifndef BOOST_IOSTREAMS_TEST_WRITE_SEEKABLE_HPP_INCLUDED
