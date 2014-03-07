// (C) Copyright 2008 CodeRage, LLC (turkanis at coderage dot com)
// (C) Copyright 2004-2007 Jonathan Turkanis
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt.)

// See http://www.boost.org/libs/iostreams for documentation.

#include <cctype>
#include <boost/iostreams/device/file.hpp>
#include <boost/iostreams/filter/stdio.hpp>
#include <boost/iostreams/filtering_stream.hpp>
#include <boost/test/test_tools.hpp>
#include <boost/test/unit_test.hpp>
#include "detail/filters.hpp"
#include "detail/temp_file.hpp"
#include "detail/verification.hpp"

// Must come last.
#include <boost/iostreams/detail/config/disable_warnings.hpp> // BCC 5.x.

using namespace boost::iostreams;
using boost::unit_test::test_suite;

struct toupper_stdio_filter : stdio_filter {
    void do_filter()
    {
        int c;
        while ((c = std::cin.get()) != EOF)
            std::cout.put(std::toupper((unsigned char)c));
    }
};

struct tolower_stdio_filter : stdio_filter {
    void do_filter()
    {
        int c;
        while ((c = std::cin.get()) != EOF)
            std::cout.put(std::tolower((unsigned char)c));
    }
};

struct padding_stdio_filter : stdio_filter {
    padding_stdio_filter(char pad_char) : pad_char_(pad_char) { }
    void do_filter()
    {
        int c;
        while ((c = std::cin.get()) != EOF) {
            std::cout.put(c);
            std::cout.put(pad_char_);
        }
    }
    char pad_char_;
};

void read_stdio_filter()
{
    using namespace boost::iostreams::test;

    test_file          src1, src2;
    filtering_istream  first, second;

    first.push(toupper_filter());
    first.push(padding_filter('a'));
    first.push(file_source(src1.name(), in_mode));
    second.push(toupper_stdio_filter());
    second.push(padding_stdio_filter('a'));
    second.push(file_source(src2.name(), in_mode));
    BOOST_CHECK_MESSAGE(
        compare_streams_in_chunks(first, second),
        "failed reading from a stdio_filter"
    );

    first.reset();
    first.push(padding_filter('a'));
    first.push(toupper_filter());
    first.push(file_source(src1.name(), in_mode));
    second.reset();
    second.push(padding_stdio_filter('a'));
    second.push(toupper_stdio_filter());
    second.push(file_source(src2.name(), in_mode));
    BOOST_CHECK_MESSAGE(
        compare_streams_in_chunks(first, second),
        "failed reading from a stdio_filter"
    );
}

void write_stdio_filter()
{
    using namespace std;
    using namespace boost::iostreams::test;

    temp_file          dest1, dest2;
    filtering_ostream  out1, out2;

    out1.push(tolower_filter());
    out1.push(padding_filter('a'));
    out1.push(file_sink(dest1.name(), in_mode));
    out2.push(tolower_stdio_filter());
    out2.push(padding_stdio_filter('a'));
    out2.push(file_sink(dest2.name(), in_mode));
    write_data_in_chunks(out1);
    write_data_in_chunks(out2);
    out1.reset();
    out2.reset();

    {
        ifstream first(dest1.name().c_str());
        ifstream second(dest2.name().c_str());
        BOOST_CHECK_MESSAGE(
            compare_streams_in_chunks(first, second),
            "failed writing to a stdio_filter"
        );
    }

    out1.push(padding_filter('a'));
    out1.push(tolower_filter());
    out1.push(file_sink(dest1.name(), in_mode));
    out2.push(padding_stdio_filter('a'));
    out2.push(tolower_stdio_filter());
    out2.push(file_sink(dest2.name(), in_mode));
    write_data_in_chunks(out1);
    write_data_in_chunks(out2);
    out1.reset();
    out2.reset();

    {
        ifstream first(dest1.name().c_str());
        ifstream second(dest2.name().c_str());
        BOOST_CHECK_MESSAGE(
            compare_streams_in_chunks(first, second),
            "failed writing to a stdio_filter"
        );
    }
}

test_suite* init_unit_test_suite(int, char* [])
{
    test_suite* test = BOOST_TEST_SUITE("line_filter test");
    test->add(BOOST_TEST_CASE(&read_stdio_filter));
    test->add(BOOST_TEST_CASE(&write_stdio_filter));
    return test;
}

#include <boost/iostreams/detail/config/enable_warnings.hpp> // BCC 5.x.
