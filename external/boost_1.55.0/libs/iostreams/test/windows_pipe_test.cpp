// (C) Copyright 2008 CodeRage, LLC (turkanis at coderage dot com)
// (C) Copyright 2004-2007 Jonathan Turkanis
// (C) Copyright 2012 Boris Schaeling
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt.)

// See http://www.boost.org/libs/iostreams for documentation.

#include <Windows.h>
#include <boost/iostreams/device/file_descriptor.hpp>
#include <boost/iostreams/filtering_stream.hpp>
#include <boost/iostreams/get.hpp>
#include <boost/test/test_tools.hpp>
#include <boost/test/unit_test.hpp>

using namespace boost::iostreams;
using boost::unit_test::test_suite;

void read_from_file_descriptor_source_test()
{
    HANDLE handles[2];
    ::CreatePipe(&handles[0], &handles[1], NULL, 0);

    char buffer[2] = { 'a', 'b' };
    DWORD written;
    ::WriteFile(handles[1], buffer, 2, &written, NULL);
    ::CloseHandle(handles[1]);

    file_descriptor_source is(handles[0], close_handle);

    BOOST_CHECK_EQUAL('a', get(is));
    BOOST_CHECK_EQUAL('b', get(is));
    BOOST_CHECK_EQUAL(-1, get(is));
    BOOST_CHECK_EQUAL(-1, get(is));
}

void read_from_filtering_istream_test()
{
    HANDLE handles[2];
    ::CreatePipe(&handles[0], &handles[1], NULL, 0);

    char buffer[2] = { 'a', 'b' };
    DWORD written;
    ::WriteFile(handles[1], buffer, 2, &written, NULL);
    ::CloseHandle(handles[1]);

    file_descriptor_source source(handles[0], close_handle);
    filtering_istream is;
    is.push(source);

    BOOST_CHECK_EQUAL('a', get(is));
    BOOST_CHECK_EQUAL('b', get(is));
    BOOST_CHECK_EQUAL(-1, get(is));
    BOOST_CHECK_EQUAL(-1, get(is));
}

test_suite* init_unit_test_suite(int, char* [])
{
    test_suite* test = BOOST_TEST_SUITE("Windows pipe test");
    test->add(BOOST_TEST_CASE(&read_from_file_descriptor_source_test));
    test->add(BOOST_TEST_CASE(&read_from_filtering_istream_test));
    return test;
}
