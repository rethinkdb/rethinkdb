// (C) Copyright 2008 CodeRage, LLC (turkanis at coderage dot com)
// (C) Copyright 2004-2007 Jonathan Turkanis
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt.)

// See http://www.boost.org/libs/iostreams for documentation.

#ifndef BOOST_IOSTREAMS_TEST_SEEK_HPP_INCLUDED
#define BOOST_IOSTREAMS_TEST_SEEK_HPP_INCLUDED


#include <boost/config.hpp>         // BOOST_MSVC, make sure size_t is in std.
#include <cstddef>                  // std::size_t.
#include <string>
#include <boost/iostreams/device/array.hpp>
#include <boost/iostreams/filtering_stream.hpp>
#include <boost/test/test_tools.hpp>
#include "../example/container_device.hpp"  // We use container_device instead
#include "detail/verification.hpp"          // of make_iterator_range to 
                                            // reduce dependence on Boost.Range

void seek_test()
{
    using namespace std;
    using namespace boost;
    using namespace boost::iostreams;
    using namespace boost::iostreams::example;
    using namespace boost::iostreams::test;

    {
        string                      test(data_reps * data_length(), '\0');
        filtering_stream<seekable>  io;
        io.push(container_device<string>(test));
        BOOST_CHECK_MESSAGE(
            test_seekable_in_chars(io),
            "failed seeking within a filtering_stream<seekable>, in chars"
        );
    }

    {
        string                      test(data_reps * data_length(), '\0');
        filtering_stream<seekable>  io;
        io.push(container_device<string>(test));
        BOOST_CHECK_MESSAGE(
            test_seekable_in_chunks(io),
            "failed seeking within a filtering_stream<seekable>, in chunks"
        );
    }
}

#endif // #ifndef BOOST_IOSTREAMS_TEST_SEEK_HPP_INCLUDED
