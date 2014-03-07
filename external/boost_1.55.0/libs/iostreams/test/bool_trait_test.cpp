/*
 * Distributed under the Boost Software License, Version 1.0.(See accompanying 
 * file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt.)
 * 
 * See http://www.boost.org/libs/iostreams for documentation.
 *
 * Tests the boolean type traits defined in boost/iostreams/traits.hpp.
 *
 * File:        libs/iostreams/test/bool_trait_test.cpp
 * Date:        Sun Feb 17 17:52:59 MST 2008
 * Copyright:   2008 CodeRage, LLC
 * Author:      Jonathan Turkanis
 * Contact:     turkanis at coderage dot com
 */

#include <fstream>
#include <sstream>
#include <boost/iostreams/detail/iostream.hpp>
#include <boost/iostreams/detail/streambuf/linked_streambuf.hpp>
#include <boost/iostreams/detail/iostream.hpp>
#include <boost/iostreams/device/array.hpp>
#include <boost/iostreams/filtering_stream.hpp>
#include <boost/iostreams/filtering_streambuf.hpp>
#include <boost/iostreams/stream.hpp>
#include <boost/iostreams/stream_buffer.hpp>
#include <boost/iostreams/traits.hpp>
#include <boost/test/unit_test.hpp>

using namespace boost::iostreams;
namespace io = boost::iostreams;
using boost::unit_test::test_suite;

typedef stream<array_source>   array_istream;
typedef stream<warray_source>  array_wistream;
typedef stream<array_sink>     array_ostream;
typedef stream<warray_sink>    array_wostream;
typedef stream<array>          array_stream;
typedef stream<warray>         array_wstream;
typedef stream_buffer<array>   array_streambuf;
typedef stream_buffer<warray>  array_wstreambuf;
 
typedef io::filtering_stream<seekable>           filtering_iostream;
typedef io::filtering_stream<seekable, wchar_t>  filtering_wiostream;

typedef io::detail::linked_streambuf<char>     linkedbuf;
typedef io::detail::linked_streambuf<wchar_t>  wlinkedbuf;

#define BOOST_CHECK_BOOL_TRAIT(trait, type, status) \
    BOOST_CHECK(trait< type >::value == status)
    /**/

#define BOOST_CHECK_STREAM_TRAIT( \
    trait, \
    istream_, wistream_, ostream_, wostream_, \
    iostream_, wiostream_, streambuf_, wstreambuf_, \
    ifstream_, wifstream_, ofstream_, wofstream_, \
    fstream_, wfstream_, filebuf_, wfilebuf_, \
    istringstream_, wistringstream_, ostringstream_, wostringstream_, \
    stringstream_, wstringstream_, stringbuf_, wstringbuf_, \
    array_istream_, array_wistream_, array_ostream_, array_wostream_, \
    array_stream_, array_wstream_, array_streambuf_, array_wstreambuf_, \
    filtering_istream_, filtering_wistream_, \
    filtering_ostream_, filtering_wostream_, \
    filtering_iostream_, filtering_wiostream_, \
    filtering_istreambuf_, filtering_wistreambuf_, \
    linkedbuf_, wlinkedbuf_ ) \
    BOOST_CHECK_BOOL_TRAIT(trait, std::istream, istream_); \
    BOOST_CHECK_BOOL_TRAIT(trait, std::wistream, wistream_); \
    BOOST_CHECK_BOOL_TRAIT(trait, std::ostream, ostream_); \
    BOOST_CHECK_BOOL_TRAIT(trait, std::wostream, wostream_); \
    BOOST_CHECK_BOOL_TRAIT(trait, std::iostream, iostream_); \
    BOOST_CHECK_BOOL_TRAIT(trait, std::wiostream, wiostream_); \
    BOOST_CHECK_BOOL_TRAIT(trait, std::streambuf, streambuf_); \
    BOOST_CHECK_BOOL_TRAIT(trait, std::wstreambuf, wstreambuf_); \
    BOOST_CHECK_BOOL_TRAIT(trait, std::wifstream, wifstream_); \
    BOOST_CHECK_BOOL_TRAIT(trait, std::ofstream, ofstream_); \
    BOOST_CHECK_BOOL_TRAIT(trait, std::wofstream, wofstream_); \
    BOOST_CHECK_BOOL_TRAIT(trait, std::fstream, fstream_); \
    BOOST_CHECK_BOOL_TRAIT(trait, std::wfstream, wfstream_); \
    BOOST_CHECK_BOOL_TRAIT(trait, std::filebuf, filebuf_); \
    BOOST_CHECK_BOOL_TRAIT(trait, std::wfilebuf, wfilebuf_); \
    BOOST_CHECK_BOOL_TRAIT(trait, std::istringstream, istringstream_); \
    BOOST_CHECK_BOOL_TRAIT(trait, std::wistringstream, wistringstream_); \
    BOOST_CHECK_BOOL_TRAIT(trait, std::ostringstream, ostringstream_); \
    BOOST_CHECK_BOOL_TRAIT(trait, std::wostringstream, wostringstream_); \
    BOOST_CHECK_BOOL_TRAIT(trait, std::stringstream, stringstream_); \
    BOOST_CHECK_BOOL_TRAIT(trait, std::wstringstream, wstringstream_); \
    BOOST_CHECK_BOOL_TRAIT(trait, std::stringbuf, stringbuf_); \
    BOOST_CHECK_BOOL_TRAIT(trait, std::wstringbuf, wstringbuf_); \
    BOOST_CHECK_BOOL_TRAIT(trait, array_istream, array_istream_); \
    BOOST_CHECK_BOOL_TRAIT(trait, array_wistream, array_wistream_); \
    BOOST_CHECK_BOOL_TRAIT(trait, array_ostream, array_ostream_); \
    BOOST_CHECK_BOOL_TRAIT(trait, array_wostream, array_wostream_); \
    BOOST_CHECK_BOOL_TRAIT(trait, array_stream, array_stream_); \
    BOOST_CHECK_BOOL_TRAIT(trait, array_wstream, array_wstream_); \
    BOOST_CHECK_BOOL_TRAIT(trait, array_streambuf, array_streambuf_); \
    BOOST_CHECK_BOOL_TRAIT(trait, array_wstreambuf, array_wstreambuf_); \
    BOOST_CHECK_BOOL_TRAIT(trait, io::filtering_istream, filtering_istream_); \
    BOOST_CHECK_BOOL_TRAIT(trait, io::filtering_wistream, filtering_wistream_); \
    BOOST_CHECK_BOOL_TRAIT(trait, io::filtering_ostream, filtering_ostream_); \
    BOOST_CHECK_BOOL_TRAIT(trait, io::filtering_wostream, filtering_wostream_); \
    BOOST_CHECK_BOOL_TRAIT(trait, filtering_iostream, filtering_iostream_); \
    BOOST_CHECK_BOOL_TRAIT(trait, filtering_wiostream, filtering_wiostream_); \
    BOOST_CHECK_BOOL_TRAIT(trait, io::filtering_istreambuf, filtering_istreambuf_); \
    BOOST_CHECK_BOOL_TRAIT(trait, io::filtering_wistreambuf, filtering_wistreambuf_); \
    BOOST_CHECK_BOOL_TRAIT(trait, linkedbuf, linkedbuf_); \
    BOOST_CHECK_BOOL_TRAIT(trait, wlinkedbuf, wlinkedbuf_); \
    BOOST_CHECK_BOOL_TRAIT(trait, io::array, false); \
    BOOST_CHECK_BOOL_TRAIT(trait, int, false);
    /**/

void bool_trait_test()
{
    // Test is_istream
    BOOST_CHECK_STREAM_TRAIT(
        io::is_istream, 
        true, true, false, false, true, true, false, false,
        true, true, false, false, true, true, false, false,
        true, true, false, false, true, true, false, false,
        true, true, false, false, true, true, false, false,
        true, true, false, false, true, true, false, false,
        false, false
    );

    // Test is_ostream
    BOOST_CHECK_STREAM_TRAIT(
        io::is_ostream, 
        false, false, true, true, true, true, false, false,
        false, false, true, true, true, true, false, false,
        false, false, true, true, true, true, false, false,
        false, false, true, true, true, true, false, false,
        false, false, true, true, true, true, false, false,
        false, false
    );

    // Test is_iostream
    BOOST_CHECK_STREAM_TRAIT(
        io::is_iostream, 
        false, false, false, false, true, true, false, false,
        false, false, false, false, true, true, false, false,
        false, false, false, false, true, true, false, false,
        false, false, false, false, true, true, false, false,
        false, false, false, false, true, true, false, false,
        false, false
    );

    // Test is_streambuf
    BOOST_CHECK_STREAM_TRAIT(
        io::is_streambuf, 
        false, false, false, false, false, false, true, true,
        false, false, false, false, false, false, true, true,
        false, false, false, false, false, false, true, true,
        false, false, false, false, false, false, true, true,
        false, false, false, false, false, false, true, true,
        true, true
    );

    // Test is_std_io
    BOOST_CHECK_STREAM_TRAIT(
        io::is_std_io, 
        true, true, true, true, true, true, true, true,
        true, true, true, true, true, true, true, true,
        true, true, true, true, true, true, true, true,
        true, true, true, true, true, true, true, true,
        true, true, true, true, true, true, true, true,
        true, true
    );

    // Test is_std_file_device
    BOOST_CHECK_STREAM_TRAIT(
        io::is_std_file_device, 
        false, false, false, false, false, false, false, false,
        true, true, true, true, true, true, true, true,
        false, false, false, false, false, false, false, false,
        false, false, false, false, false, false, false, false,
        false, false, false, false, false, false, false, false,
        false, false
    );

    // Test is_std_string_device
    BOOST_CHECK_STREAM_TRAIT(
        io::is_std_string_device, 
        false, false, false, false, false, false, false, false,
        false, false, false, false, false, false, false, false,
        true, true, true, true, true, true, true, true,
        false, false, false, false, false, false, false, false,
        false, false, false, false, false, false, false, false,
        false, false
    );

    // Test is_boost_stream
    BOOST_CHECK_STREAM_TRAIT(
        io::detail::is_boost_stream, 
        false, false, false, false, false, false, false, false,
        false, false, false, false, false, false, false, false,
        false, false, false, false, false, false, false, false,
        true, true, true, true, true, true, false, false,
        false, false, false, false, false, false, false, false,
        false, false
    );

    // Test is_boost_stream_buffer
    BOOST_CHECK_STREAM_TRAIT(
        io::detail::is_boost_stream_buffer, 
        false, false, false, false, false, false, false, false,
        false, false, false, false, false, false, false, false,
        false, false, false, false, false, false, false, false,
        false, false, false, false, false, false, true, true,
        false, false, false, false, false, false, false, false,
        false, false
    );

    // Test is_filtering_stream
    BOOST_CHECK_STREAM_TRAIT(
        io::detail::is_filtering_stream, 
        false, false, false, false, false, false, false, false,
        false, false, false, false, false, false, false, false,
        false, false, false, false, false, false, false, false,
        false, false, false, false, false, false, false, false,
        true, true, true, true, true, true, false, false,
        false, false
    );

    // Test is_filtering_streambuf
    BOOST_CHECK_STREAM_TRAIT(
        io::detail::is_filtering_streambuf, 
        false, false, false, false, false, false, false, false,
        false, false, false, false, false, false, false, false,
        false, false, false, false, false, false, false, false,
        false, false, false, false, false, false, false, false,
        false, false, false, false, false, false, true, true,
        false, false
    );

    // Test is_boost
    BOOST_CHECK_STREAM_TRAIT(
        io::detail::is_boost, 
        false, false, false, false, false, false, false, false,
        false, false, false, false, false, false, false, false,
        false, false, false, false, false, false, false, false,
        true, true, true, true, true, true, true, true,
        true, true, true, true, true, true, true, true,
        false, false
    );
}

test_suite* init_unit_test_suite(int, char* []) 
{
    test_suite* test = BOOST_TEST_SUITE("bool trait test");
    test->add(BOOST_TEST_CASE(&bool_trait_test));
    return test;
}
