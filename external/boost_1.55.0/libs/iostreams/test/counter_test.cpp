// (C) Copyright 2008 CodeRage, LLC (turkanis at coderage dot com)
// (C) Copyright 2004-2007 Jonathan Turkanis
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt.)

// See http://www.boost.org/libs/iostreams for documentation.

#include <cctype>
#include <boost/iostreams/copy.hpp>
#include <boost/iostreams/device/file.hpp>
#include <boost/iostreams/device/null.hpp>
#include <boost/iostreams/filter/counter.hpp>
#include <boost/iostreams/filtering_stream.hpp>
#include <boost/test/test_tools.hpp>
#include <boost/test/unit_test.hpp>
#include "detail/constants.hpp"
#include "detail/filters.hpp"
#include "detail/temp_file.hpp"
#include "detail/verification.hpp"

using namespace std;
using namespace boost;
using namespace boost::iostreams;
using namespace boost::iostreams::test;
using boost::unit_test::test_suite;    

void read_counter()
{
    test_file          src;
    filtering_istream  in;
    in.push(counter());
    in.push(padding_filter('a'), 0);
    in.push(counter());
    in.push(file_source(src.name(), in_mode));

    counter*  first_counter = BOOST_IOSTREAMS_COMPONENT(in, 0, counter);
    counter*  second_counter = BOOST_IOSTREAMS_COMPONENT(in, 2, counter);
    int       first_count = 0;
    int       second_count = 0;
    int       line_count = 0;
    int       reps = data_reps < 50 ? data_reps : 25; // Keep test short.
    for (int w = 0; w < reps; ++w) {
        int len = data_length();
        for (int z = 0; z < len; ++z) {
            in.get();
            ++first_count;
            ++second_count;
            BOOST_CHECK(first_counter->characters() == first_count);
            BOOST_CHECK(second_counter->characters() == second_count);
            in.get();
            ++first_count;
            BOOST_CHECK(first_counter->characters() == first_count);
            BOOST_CHECK(second_counter->characters() == second_count);
        }
        ++line_count;
        BOOST_CHECK(first_counter->lines() == line_count);
        BOOST_CHECK(first_counter->lines() == line_count);
    }
}

void write_counter() 
{
    filtering_ostream  out;
    out.push(counter());
    out.push(padding_filter('a'), 0);
    out.push(counter());
    out.push(null_sink());

    counter*  first_counter = BOOST_IOSTREAMS_COMPONENT(out, 0, counter);
    counter*  second_counter = BOOST_IOSTREAMS_COMPONENT(out, 2, counter);
    int       first_count = 0;
    int       second_count = 0;
    int       line_count = 0;
    int       reps = data_reps < 50 ? data_reps : 25; // Keep test short.
    const char* data = narrow_data();
    for (int w = 0; w < reps; ++w) {
        int len = data_length();
        for (int z = 0; z < len; ++z) {
            out.put(data[z]);
            out.flush();
            ++first_count;
            second_count += 2;
            BOOST_CHECK(first_counter->characters() == first_count);
            BOOST_CHECK(second_counter->characters() == second_count);
        }
        ++line_count;
        BOOST_CHECK(first_counter->lines() == line_count);
        BOOST_CHECK(first_counter->lines() == line_count);
    }
}

test_suite* init_unit_test_suite(int, char* []) 
{
    test_suite* test = BOOST_TEST_SUITE("counter test");
    test->add(BOOST_TEST_CASE(&read_counter));
    test->add(BOOST_TEST_CASE(&write_counter));
    return test;
}
