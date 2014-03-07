// (C) Copyright 2011 Steven Watanabe
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt.)

// See http://www.boost.org/libs/iostreams for documentation.

#include <fstream>
#include <boost/iostreams/filtering_stream.hpp>
#include <boost/test/test_tools.hpp>
#include "detail/sequence.hpp"
#include "detail/temp_file.hpp"
#include "detail/verification.hpp"

void test_filtering_ostream_flush()
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
        out.flush();
        BOOST_CHECK_MESSAGE(
            compare_files(dest.name(), lower.name()),
            "failed writing to a filtering_ostream in chars with an "
            "output filter"
        );
    }
}
