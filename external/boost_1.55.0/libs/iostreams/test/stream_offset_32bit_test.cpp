/*
 * Distributed under the Boost Software License, Version 1.0.(See accompanying 
 * file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt.)
 * 
 * See http://www.boost.org/libs/iostreams for documentation.

 * File:        libs/iostreams/test/stream_offset_32bit_test.cpp
 * Date:        Sun Dec 23 21:11:23 MST 2007
 * Copyright:   2007-2008 CodeRage, LLC
 * Author:      Jonathan Turkanis
 * Contact:     turkanis at coderage dot com
 *
 * Tests the functions defined in the header "boost/iostreams/positioning.hpp"
 * with small (32-bit) file offsets.
 */

#include <boost/iostreams/positioning.hpp>
#include <boost/test/test_tools.hpp>
#include <boost/test/unit_test.hpp>
#include <boost/type_traits/is_integral.hpp>

using namespace std;
using namespace boost;
using namespace boost::iostreams;
using boost::unit_test::test_suite;

void stream_offset_32bit_test()
{
    stream_offset  small_file = 1000000;
    stream_offset  off = -small_file;
    streampos      pos = offset_to_position(off);

    while (off < small_file)
    {
        BOOST_CHECK(off == position_to_offset(offset_to_position(off)));
        BOOST_CHECK(pos == offset_to_position(position_to_offset(pos)));
        off += 20000;
        pos += 20000;
        BOOST_CHECK(off == position_to_offset(offset_to_position(off)));
        BOOST_CHECK(pos == offset_to_position(position_to_offset(pos)));
        off -= 10000;
        pos -= 10000;
    }
}

test_suite* init_unit_test_suite(int, char* [])
{
    test_suite* test = BOOST_TEST_SUITE("stream_offset 32-bit test");
    test->add(BOOST_TEST_CASE(&stream_offset_32bit_test));
    return test;
}
