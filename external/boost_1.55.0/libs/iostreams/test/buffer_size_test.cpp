// (C) Copyright 2008 CodeRage, LLC (turkanis at coderage dot com)
// (C) Copyright 2004-2007 Jonathan Turkanis
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt.)

// See http://www.boost.org/libs/iostreams for documentation.

#include "detail/filters.hpp"  // Must come before operations.hpp for VC6.
#include <boost/iostreams/categories.hpp>
#include <boost/iostreams/constants.hpp>
#include <boost/iostreams/device/null.hpp>
#include <boost/iostreams/operations.hpp>
#include <boost/test/test_tools.hpp>
#include <boost/test/unit_test.hpp>

using namespace std;
using namespace boost::iostreams;
using namespace boost::iostreams::test;
using boost::unit_test::test_suite;

struct optimally_buffered_filter {
    typedef char char_type;
    struct category
        : input_filter_tag,
          optimally_buffered_tag
        { };
    std::streamsize optimal_buffer_size() const
    { return default_filter_buffer_size + 1; }
};

void buffer_size_test()
{
    // Test   device buffer sizes.

    BOOST_CHECK_MESSAGE(
      optimal_buffer_size(null_source()) == default_device_buffer_size,
      "wrong buffer size for sourcer"
    );
    BOOST_CHECK_MESSAGE(
      optimal_buffer_size(null_sink()) == default_device_buffer_size,
      "wrong buffer size for sink"
    );

    // Test   filter buffer sizes.

    BOOST_CHECK_MESSAGE(
      optimal_buffer_size(toupper_filter()) == default_filter_buffer_size,
      "wrong buffer size for input filter"
    );
    BOOST_CHECK_MESSAGE(
      optimal_buffer_size(tolower_filter()) == default_filter_buffer_size,
      "wrong buffer size for output filter"
    );
    BOOST_CHECK_MESSAGE(
      optimal_buffer_size(toupper_multichar_filter())
          ==
      default_filter_buffer_size,
      "wrong buffer size for multi-character input filter"
    );
    BOOST_CHECK_MESSAGE(
      optimal_buffer_size(tolower_multichar_filter())
          ==
      default_filter_buffer_size,
      "wrong buffer size for multi-character output filter"
    );

    // Test   custom buffer size.

    BOOST_CHECK_MESSAGE(
      optimal_buffer_size(optimally_buffered_filter())
          ==
      optimally_buffered_filter().optimal_buffer_size(),
      "wrong buffer size for multi-character output filter"
    );
}

test_suite* init_unit_test_suite(int, char* [])
{
    test_suite* test = BOOST_TEST_SUITE("buffer_size test");
    test->add(BOOST_TEST_CASE(&buffer_size_test));
    return test;
}
