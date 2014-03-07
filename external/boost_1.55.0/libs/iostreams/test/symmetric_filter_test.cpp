// (C) Copyright 2008 CodeRage, LLC (turkanis at coderage dot com)
// (C) Copyright 2004-2007 Jonathan Turkanis
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt.)

// See http://www.boost.org/libs/iostreams for documentation.

#include <boost/iostreams/detail/buffer.hpp>
#include <boost/iostreams/device/file.hpp>
#include <boost/iostreams/filter/symmetric.hpp>
#include <boost/iostreams/filter/test.hpp>
#include <boost/test/test_tools.hpp>
#include <boost/test/unit_test.hpp>
#include "detail/closable.hpp"
#include "./detail/constants.hpp"
#include "detail/operation_sequence.hpp"
#include "./detail/temp_file.hpp"
#include "./detail/verification.hpp"

// Must come last.
#include <boost/iostreams/detail/config/disable_warnings.hpp> 

using namespace boost::iostreams;
using namespace boost::iostreams::test;
using boost::unit_test::test_suite;  
namespace io = boost::iostreams; 

// Note: The filter is given an internal buffer -- unnecessary in this simple
// case -- to stress test symmetric_filter.
struct toupper_symmetric_filter_impl {
    typedef char char_type;
    explicit toupper_symmetric_filter_impl( 
                std::streamsize buffer_size = 
                    default_filter_buffer_size 
             ) 
        : buf_(buffer_size) 
    {
        buf_.set(0, 0);
    }
    bool filter( const char*& src_begin, const char* src_end,
                 char*& dest_begin, char* dest_end, bool /* flush */ )
    {
        while ( can_read(src_begin, src_end) || 
                can_write(dest_begin, dest_end) ) 
        {
            if (can_read(src_begin, src_end)) 
                read(src_begin, src_end);
            if (can_write(dest_begin, dest_end)) 
                write(dest_begin, dest_end);
        }
        bool result = buf_.ptr() != buf_.eptr();
        return result;
    }
    void close() { buf_.set(0, 0); }
    void read(const char*& src_begin, const char* src_end)
    {
        std::ptrdiff_t count =
            (std::min) ( src_end - src_begin,
                         static_cast<std::ptrdiff_t>(buf_.size()) - 
                             (buf_.eptr() - buf_.data()) );
        while (count-- > 0)
            *buf_.eptr()++ = std::toupper(*src_begin++);
    }
    void write(char*& dest_begin, char* dest_end)
    {
        std::ptrdiff_t count =
            (std::min) ( dest_end - dest_begin,
                         buf_.eptr() - buf_.ptr() );
        while (count-- > 0)
            *dest_begin++ = *buf_.ptr()++;
        if (buf_.ptr() == buf_.eptr())
            buf_.set(0, 0);
    }
    bool can_read(const char*& src_begin, const char* src_end)
    { return src_begin != src_end && buf_.eptr() != buf_.end(); }
    bool can_write(char*& dest_begin, char* dest_end)
    { return dest_begin != dest_end && buf_.ptr() != buf_.eptr(); }
    boost::iostreams::detail::buffer<char> buf_;
};

typedef symmetric_filter<toupper_symmetric_filter_impl>
        toupper_symmetric_filter;

void read_symmetric_filter()
{
    test_file       test;
    uppercase_file  upper;
    BOOST_CHECK(
        test_input_filter( toupper_symmetric_filter(default_filter_buffer_size),
                           file_source(test.name(), in_mode),
                           file_source(upper.name(), in_mode) )
    );
} 

void write_symmetric_filter()
{
    test_file       test;
    uppercase_file  upper;
    BOOST_CHECK(
        test_output_filter( toupper_symmetric_filter(default_filter_buffer_size),
                            file_source(test.name(), in_mode),
                            file_source(upper.name(), in_mode) )
    );
}

void close_symmetric_filter()
{
    // Test input
    {
        operation_sequence  seq;
        chain<input>        ch;
        ch.push(
            io::symmetric_filter<closable_symmetric_filter>
                (1, seq.new_operation(2))
        );
        ch.push(closable_device<input>(seq.new_operation(1)));
        BOOST_CHECK_NO_THROW(ch.reset());
        BOOST_CHECK_OPERATION_SEQUENCE(seq);
    }

    // Test output
    {
        operation_sequence  seq;
        chain<output>       ch;
        ch.push(
            io::symmetric_filter<closable_symmetric_filter>
                (1, seq.new_operation(1))
        );
        ch.push(closable_device<output>(seq.new_operation(2)));
        BOOST_CHECK_NO_THROW(ch.reset());
        BOOST_CHECK_OPERATION_SEQUENCE(seq);
    }
}

#ifndef BOOST_IOSTREAMS_NO_WIDE_STREAMS

struct wcopy_filter_impl {
    typedef wchar_t char_type;
    bool filter( const wchar_t*& src_begin, const wchar_t* src_end,
                 wchar_t*& dest_begin, wchar_t* dest_end, bool /* flush */ )
    {
        if(src_begin != src_end && dest_begin != dest_end) {
            *dest_begin++ = *src_begin++;
        }
        return false;
    }
    void close() {}
};

typedef symmetric_filter<wcopy_filter_impl> wcopy_filter;

void wide_symmetric_filter()
{
    {
        warray_source  src(wide_data(), wide_data() + data_length());
        std::wstring   dest;
        io::copy(src, io::compose(wcopy_filter(16), io::back_inserter(dest)));
        BOOST_CHECK(dest == wide_data());
    }
    {
        warray_source  src(wide_data(), wide_data() + data_length());
        std::wstring   dest;
        io::copy(io::compose(wcopy_filter(16), src), io::back_inserter(dest));
        BOOST_CHECK(dest == wide_data());
    }
}

#endif

test_suite* init_unit_test_suite(int, char* []) 
{
    test_suite* test = BOOST_TEST_SUITE("symmetric_filter test");
    test->add(BOOST_TEST_CASE(&read_symmetric_filter));
    test->add(BOOST_TEST_CASE(&write_symmetric_filter));
    test->add(BOOST_TEST_CASE(&close_symmetric_filter));
#ifndef BOOST_IOSTREAMS_NO_WIDE_STREAMS
    test->add(BOOST_TEST_CASE(&wide_symmetric_filter));
#endif
    return test;
}

#include <boost/iostreams/detail/config/enable_warnings.hpp>
