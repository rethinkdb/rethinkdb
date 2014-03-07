// (C) Copyright 2008 CodeRage, LLC (turkanis at coderage dot com)
// (C) Copyright 2004-2007 Jonathan Turkanis
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt.)

// See http://www.boost.org/libs/iostreams for documentation.

#include <stdexcept>  // out_of_range.
#include <typeinfo>
#include <utility>    // pair.
#include <boost/config.hpp>              // BOOST_MSVC.
#include <boost/detail/workaround.hpp>
#include <boost/iostreams/device/file.hpp>
#include <boost/iostreams/filtering_stream.hpp>
#include <boost/iostreams/stream.hpp>
#include <boost/iostreams/stream_buffer.hpp>
#include <boost/test/test_tools.hpp>
#include <boost/test/unit_test.hpp>
#include "detail/constants.hpp"
#include "detail/filters.hpp"
#include "detail/temp_file.hpp"
#include "detail/verification.hpp"

namespace io = boost::iostreams;
using boost::unit_test::test_suite;

#define COMPARE_TYPE_ID(x, y) BOOST_IOSTREAMS_COMPARE_TYPE_ID(x, y)

struct indirect_source : io::source {
    void foo() { }
    std::streamsize read(char*, std::streamsize) { return 0; }
};

struct direct_source {
    typedef char char_type;
    struct category 
        : io::input, io::device_tag, io::direct_tag 
        { };
    void foo() { }
    std::pair<char*, char*> input_sequence() 
    { 
        return std::pair<char*, char*>(static_cast<char*>(0), static_cast<char*>(0));;
    }
};

void compile_time_test()
{
    using namespace io;

    stream_buffer<indirect_source> indirect_buf;
    indirect_buf.open(indirect_source());
    indirect_buf->foo();

    stream_buffer<direct_source> direct_buf;
    direct_buf.open(direct_source());
    direct_buf->foo();

    stream<indirect_source> indirect_stream;
    indirect_stream.open(indirect_source());
    indirect_stream->foo();

    stream<direct_source> direct_stream;
    direct_stream.open(direct_source());
    direct_stream->foo();
}

void component_type_test()
{
    using namespace std;
    using namespace io;
    using namespace boost::iostreams::test;

    temp_file       dest;
    lowercase_file  lower;

    filtering_ostream out;
    out.push(tolower_filter());
    out.push(tolower_multichar_filter());
    out.push(file_sink(dest.name(), out_mode));

    // Check index 0.
    BOOST_CHECK(COMPARE_TYPE_ID(
        out.component_type(0),
        typeid(tolower_filter)
    ));
    BOOST_CHECK(COMPARE_TYPE_ID(
        BOOST_IOSTREAMS_COMPONENT_TYPE(out, 0), 
        typeid(tolower_filter)
    ));
    BOOST_CHECK_NO_THROW((
        BOOST_IOSTREAMS_COMPONENT(out, 0, tolower_filter)
    ));
#if !BOOST_WORKAROUND(BOOST_MSVC, < 1310)
    BOOST_CHECK_NO_THROW((
        out.component<tolower_filter>(0)
    ));
    BOOST_CHECK_NO_THROW((
        out.component<0, tolower_filter>()
    ));
#endif

    // Check index 1.
    BOOST_CHECK(COMPARE_TYPE_ID(
        out.component_type(1),
        typeid(tolower_multichar_filter)
    ));
    BOOST_CHECK(COMPARE_TYPE_ID(
        BOOST_IOSTREAMS_COMPONENT_TYPE(out, 1), 
        typeid(tolower_multichar_filter)
    ));
    BOOST_CHECK_NO_THROW((
        BOOST_IOSTREAMS_COMPONENT(out, 1, tolower_multichar_filter)
    ));
#if !BOOST_WORKAROUND(BOOST_MSVC, < 1310)
    BOOST_CHECK_NO_THROW((
        out.component<tolower_multichar_filter>(1)
    ));
    BOOST_CHECK_NO_THROW((
        out.component<1, tolower_multichar_filter>()
    ));
#endif

    // Check index 2.
    BOOST_CHECK(COMPARE_TYPE_ID(
        out.component_type(2),
        typeid(file_sink)
    ));
    BOOST_CHECK(COMPARE_TYPE_ID(
        BOOST_IOSTREAMS_COMPONENT_TYPE(out, 2), 
        typeid(file_sink)
    ));
    BOOST_CHECK_NO_THROW((
        BOOST_IOSTREAMS_COMPONENT(out, 2, file_sink)
    ));
#if !BOOST_WORKAROUND(BOOST_MSVC, < 1310)
    BOOST_CHECK_NO_THROW((
        out.component<file_sink>(2)
    ));
    BOOST_CHECK_NO_THROW((
        out.component<2, file_sink>()
    ));
#endif

    // Check index 3.
    BOOST_CHECK_THROW(
        out.component_type(3),
        std::out_of_range
    );
    BOOST_CHECK_THROW(
        BOOST_IOSTREAMS_COMPONENT_TYPE(out, 3),
        std::out_of_range
    );

    // Check components.

    filtering_ostream out2;
    out2.push(*(BOOST_IOSTREAMS_COMPONENT(out, 0, tolower_filter)));
    out2.push(*(BOOST_IOSTREAMS_COMPONENT(out, 1, tolower_multichar_filter)));
    out2.push(*(BOOST_IOSTREAMS_COMPONENT(out, 2, file_sink)));
    write_data_in_chunks(out);
    out.reset();
    BOOST_CHECK_MESSAGE(
        compare_files(dest.name(), lower.name()),
        "failed accessing components of chain"
    );
}

test_suite* init_unit_test_suite(int, char* []) 
{
    test_suite* test = BOOST_TEST_SUITE("component_type test");
    test->add(BOOST_TEST_CASE(&component_type_test));
    return test;
}
