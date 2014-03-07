// (C) Copyright Frank Birbacher 2007
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt.)

// See http://www.boost.org/libs/iostreams for documentation.

#include <boost/iostreams/categories.hpp>  // tags.
#include <boost/iostreams/detail/ios.hpp>  // openmode, seekdir, int types.
#include <boost/iostreams/detail/error.hpp>
#include <boost/iostreams/positioning.hpp>
#include <boost/iostreams/stream.hpp>
#include <boost/test/test_tools.hpp>
#include <boost/test/unit_test.hpp>

using namespace boost::iostreams;
using boost::unit_test::test_suite;

/* 
 * This test unit uses a custom device to trigger errors. The device supports
 * input, output, and seek according to the SeekableDevice concept. And each
 * of the required functions throw a special detail::bad_xxx exception. This
 * should trigger the iostreams::stream to set the badbit status flag.
 * Additionally the exception can be propagated to the caller if the exception
 * mask of the stream allows exceptions.
 *
 * The stream offers four different functions: read, write, seekg, and seekp.
 * Each of them is tested with three different error reporting concepts:
 * test by reading status flags, test by propagated exception, and test by
 * calling std::ios_base::exceptions when badbit is already set.
 *
 * In each case all of the status checking functions of a stream are checked.
 */

//------------------Definition of error_device--------------------------------//

// Device whose member functions throw
struct error_device {
    typedef char                 char_type;
    typedef seekable_device_tag  category;
    error_device(char const*) {}
    std::streamsize read(char_type*, std::streamsize)
    {
        throw detail::bad_read();
    }
    std::streamsize write(const char_type*, std::streamsize)
    {
        throw detail::bad_write();
    }
    std::streampos seek(stream_offset, BOOST_IOS::seekdir)
    {
        throw detail::bad_seek();
    }
};

typedef stream<error_device> test_stream;

//------------------Stream state tester---------------------------------------//

void check_stream_for_badbit(std::iostream& str)
{
    BOOST_CHECK_MESSAGE(!str.good(), "stream still good");
    BOOST_CHECK_MESSAGE(!str.eof(), "eofbit set but not expected");
    BOOST_CHECK_MESSAGE(str.bad(), "stream did not set badbit");
    BOOST_CHECK_MESSAGE(str.fail(), "stream did not fail");
    BOOST_CHECK_MESSAGE(str.operator ! (),
            "stream does not report failure by operator !");
    BOOST_CHECK_MESSAGE(0 == str.operator void* (),
            "stream does not report failure by operator void*");
}

//------------------Test case generators--------------------------------------//

template<void (*const function)(std::iostream&)>
struct wrap_nothrow {
    static void execute()
    {
        test_stream stream("foo");
        BOOST_CHECK_NO_THROW( function(stream) );
        check_stream_for_badbit(stream);
    }
};

template<void (*const function)(std::iostream&)>
struct wrap_throw {
    static void execute()
    {
        typedef std::ios_base ios;
        test_stream stream("foo");
        
        stream.exceptions(ios::failbit | ios::badbit);
        BOOST_CHECK_THROW( function(stream), std::exception );
        
        check_stream_for_badbit(stream);
    }
};

template<void (*const function)(std::iostream&)>
struct wrap_throw_delayed {
    static void execute()
    {
        typedef std::ios_base ios;
        test_stream stream("foo");
        
        function(stream);
        BOOST_CHECK_THROW(
                stream.exceptions(ios::failbit | ios::badbit),
                ios::failure
            );
        
        check_stream_for_badbit(stream);
    }
};

//------------------Stream operations that throw------------------------------//

void test_read(std::iostream& str)
{
    char data[10];
    str.read(data, 10);
}

void test_write(std::iostream& str)
{
    char data[10] = {0};
    str.write(data, 10);
        //force use of streambuf
    str.flush();
}

void test_seekg(std::iostream& str)
{
    str.seekg(10);
}

void test_seekp(std::iostream& str)
{
    str.seekp(10);
}

test_suite* init_unit_test_suite(int, char* []) 
{
    test_suite* test = BOOST_TEST_SUITE("stream state test");
    
    test->add(BOOST_TEST_CASE(&wrap_nothrow      <&test_read>::execute));
    test->add(BOOST_TEST_CASE(&wrap_throw        <&test_read>::execute));
    test->add(BOOST_TEST_CASE(&wrap_throw_delayed<&test_read>::execute));
    
    test->add(BOOST_TEST_CASE(&wrap_nothrow      <&test_write>::execute));
    test->add(BOOST_TEST_CASE(&wrap_throw        <&test_write>::execute));
    test->add(BOOST_TEST_CASE(&wrap_throw_delayed<&test_write>::execute));
    
    test->add(BOOST_TEST_CASE(&wrap_nothrow      <&test_seekg>::execute));
    test->add(BOOST_TEST_CASE(&wrap_throw        <&test_seekg>::execute));
    test->add(BOOST_TEST_CASE(&wrap_throw_delayed<&test_seekg>::execute));
    
    test->add(BOOST_TEST_CASE(&wrap_nothrow      <&test_seekp>::execute));
    test->add(BOOST_TEST_CASE(&wrap_throw        <&test_seekp>::execute));
    test->add(BOOST_TEST_CASE(&wrap_throw_delayed<&test_seekp>::execute));
    
    return test;
}
