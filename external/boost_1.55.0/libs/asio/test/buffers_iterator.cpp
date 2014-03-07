//
// buffers_iterator.cpp
// ~~~~~~~~~~~~~~~~~~~~
//
// Copyright (c) 2003-2013 Christopher M. Kohlhoff (chris at kohlhoff dot com)
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//

// Disable autolinking for unit tests.
#if !defined(BOOST_ALL_NO_LIB)
#define BOOST_ALL_NO_LIB 1
#endif // !defined(BOOST_ALL_NO_LIB)

// Test that header file is self-contained.
#include <boost/asio/buffers_iterator.hpp>

#include <boost/asio/buffer.hpp>
#include "unit_test.hpp"

#if defined(BOOST_ASIO_HAS_BOOST_ARRAY)
# include <boost/array.hpp>
#endif // defined(BOOST_ASIO_HAS_BOOST_ARRAY)

#if defined(BOOST_ASIO_HAS_STD_ARRAY)
# include <array>
#endif // defined(BOOST_ASIO_HAS_STD_ARRAY)

//------------------------------------------------------------------------------

// buffers_iterator_compile test
// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
// The following test checks that all operations on the buffers_iterator compile
// and link correctly. Runtime failures are ignored.

namespace buffers_iterator_compile {

#if defined(BOOST_ASIO_HAS_BOOST_ARRAY)
using boost::array;
#elif defined(BOOST_ASIO_HAS_STD_ARRAY)
using std::array;
#endif // defined(BOOST_ASIO_HAS_BOOST_ARRAY)
using std::vector;
using namespace boost::asio;

void test()
{
  try
  {
    char data1[16], data2[16];
    const char cdata1[16] = "", cdata2[16] = "";
    mutable_buffers_1 mb1 = buffer(data1);
    array<mutable_buffer, 2> mb2 = {{ buffer(data1), buffer(data2) }};
    std::vector<mutable_buffer> mb3;
    mb3.push_back(buffer(data1));
    const_buffers_1 cb1 = buffer(cdata1);
    array<const_buffer, 2> cb2 = {{ buffer(cdata1), buffer(cdata2) }};
    vector<const_buffer> cb3;
    cb3.push_back(buffer(cdata1));


    // buffers_iterator constructors.

    buffers_iterator<mutable_buffers_1, char> bi1;
    buffers_iterator<mutable_buffers_1, const char> bi2;
    buffers_iterator<array<mutable_buffer, 2>, char> bi3;
    buffers_iterator<array<mutable_buffer, 2>, const char> bi4;
    buffers_iterator<vector<mutable_buffer>, char> bi5;
    buffers_iterator<vector<mutable_buffer>, const char> bi6;
    buffers_iterator<const_buffers_1, char> bi7;
    buffers_iterator<const_buffers_1, const char> bi8;
    buffers_iterator<array<const_buffer, 2>, char> bi9;
    buffers_iterator<array<const_buffer, 2>, const char> bi10;
    buffers_iterator<vector<const_buffer>, char> bi11;
    buffers_iterator<vector<const_buffer>, const char> bi12;

    buffers_iterator<mutable_buffers_1, char> bi13(
        buffers_iterator<mutable_buffers_1, char>::begin(mb1));
    buffers_iterator<mutable_buffers_1, const char> bi14(
        buffers_iterator<mutable_buffers_1, const char>::begin(mb1));
    buffers_iterator<array<mutable_buffer, 2>, char> bi15(
        buffers_iterator<array<mutable_buffer, 2>, char>::begin(mb2));
    buffers_iterator<array<mutable_buffer, 2>, const char> bi16(
        buffers_iterator<array<mutable_buffer, 2>, const char>::begin(mb2));
    buffers_iterator<vector<mutable_buffer>, char> bi17(
        buffers_iterator<vector<mutable_buffer>, char>::begin(mb3));
    buffers_iterator<vector<mutable_buffer>, const char> bi18(
        buffers_iterator<vector<mutable_buffer>, const char>::begin(mb3));
    buffers_iterator<const_buffers_1, char> bi19(
        buffers_iterator<const_buffers_1, char>::begin(cb1));
    buffers_iterator<const_buffers_1, const char> bi20(
        buffers_iterator<const_buffers_1, const char>::begin(cb1));
    buffers_iterator<array<const_buffer, 2>, char> bi21(
        buffers_iterator<array<const_buffer, 2>, char>::begin(cb2));
    buffers_iterator<array<const_buffer, 2>, const char> bi22(
        buffers_iterator<array<const_buffer, 2>, const char>::begin(cb2));
    buffers_iterator<vector<const_buffer>, char> bi23(
        buffers_iterator<vector<const_buffer>, char>::begin(cb3));
    buffers_iterator<vector<const_buffer>, const char> bi24(
        buffers_iterator<vector<const_buffer>, const char>::begin(cb3));

    // buffers_iterator member functions.

    bi1 = buffers_iterator<mutable_buffers_1, char>::begin(mb1);
    bi2 = buffers_iterator<mutable_buffers_1, const char>::begin(mb1);
    bi3 = buffers_iterator<array<mutable_buffer, 2>, char>::begin(mb2);
    bi4 = buffers_iterator<array<mutable_buffer, 2>, const char>::begin(mb2);
    bi5 = buffers_iterator<vector<mutable_buffer>, char>::begin(mb3);
    bi6 = buffers_iterator<vector<mutable_buffer>, const char>::begin(mb3);
    bi7 = buffers_iterator<const_buffers_1, char>::begin(cb1);
    bi8 = buffers_iterator<const_buffers_1, const char>::begin(cb1);
    bi9 = buffers_iterator<array<const_buffer, 2>, char>::begin(cb2);
    bi10 = buffers_iterator<array<const_buffer, 2>, const char>::begin(cb2);
    bi11 = buffers_iterator<vector<const_buffer>, char>::begin(cb3);
    bi12 = buffers_iterator<vector<const_buffer>, const char>::begin(cb3);

    bi1 = buffers_iterator<mutable_buffers_1, char>::end(mb1);
    bi2 = buffers_iterator<mutable_buffers_1, const char>::end(mb1);
    bi3 = buffers_iterator<array<mutable_buffer, 2>, char>::end(mb2);
    bi4 = buffers_iterator<array<mutable_buffer, 2>, const char>::end(mb2);
    bi5 = buffers_iterator<vector<mutable_buffer>, char>::end(mb3);
    bi6 = buffers_iterator<vector<mutable_buffer>, const char>::end(mb3);
    bi7 = buffers_iterator<const_buffers_1, char>::end(cb1);
    bi8 = buffers_iterator<const_buffers_1, const char>::end(cb1);
    bi9 = buffers_iterator<array<const_buffer, 2>, char>::end(cb2);
    bi10 = buffers_iterator<array<const_buffer, 2>, const char>::end(cb2);
    bi11 = buffers_iterator<vector<const_buffer>, char>::end(cb3);
    bi12 = buffers_iterator<vector<const_buffer>, const char>::end(cb3);

    // buffers_iterator related functions.

    bi1 = buffers_begin(mb1);
    bi3 = buffers_begin(mb2);
    bi5 = buffers_begin(mb3);
    bi7 = buffers_begin(cb1);
    bi9 = buffers_begin(cb2);
    bi11 = buffers_begin(cb3);

    bi1 = buffers_end(mb1);
    bi3 = buffers_end(mb2);
    bi5 = buffers_end(mb3);
    bi7 = buffers_end(cb1);
    bi9 = buffers_end(cb2);
    bi11 = buffers_end(cb3);

    // RandomAccessIterator operations.

    --bi1;
    --bi2;
    --bi3;
    --bi4;
    --bi5;
    --bi6;
    --bi7;
    --bi8;
    --bi9;
    --bi10;
    --bi11;
    --bi12;

    ++bi1;
    ++bi2;
    ++bi3;
    ++bi4;
    ++bi5;
    ++bi6;
    ++bi7;
    ++bi8;
    ++bi9;
    ++bi10;
    ++bi11;
    ++bi12;

    bi1--;
    bi2--;
    bi3--;
    bi4--;
    bi5--;
    bi6--;
    bi7--;
    bi8--;
    bi9--;
    bi10--;
    bi11--;
    bi12--;

    bi1++;
    bi2++;
    bi3++;
    bi4++;
    bi5++;
    bi6++;
    bi7++;
    bi8++;
    bi9++;
    bi10++;
    bi11++;
    bi12++;

    bi1 -= 1;
    bi2 -= 1;
    bi3 -= 1;
    bi4 -= 1;
    bi5 -= 1;
    bi6 -= 1;
    bi7 -= 1;
    bi8 -= 1;
    bi9 -= 1;
    bi10 -= 1;
    bi11 -= 1;
    bi12 -= 1;

    bi1 += 1;
    bi2 += 1;
    bi3 += 1;
    bi4 += 1;
    bi5 += 1;
    bi6 += 1;
    bi7 += 1;
    bi8 += 1;
    bi9 += 1;
    bi10 += 1;
    bi11 += 1;
    bi12 += 1;

    bi1 = bi1 - 1;
    bi2 = bi2 - 1;
    bi3 = bi3 - 1;
    bi4 = bi4 - 1;
    bi5 = bi5 - 1;
    bi6 = bi6 - 1;
    bi7 = bi7 - 1;
    bi8 = bi8 - 1;
    bi9 = bi9 - 1;
    bi10 = bi10 - 1;
    bi11 = bi11 - 1;
    bi12 = bi12 - 1;

    bi1 = bi1 + 1;
    bi2 = bi2 + 1;
    bi3 = bi3 + 1;
    bi4 = bi4 + 1;
    bi5 = bi5 + 1;
    bi6 = bi6 + 1;
    bi7 = bi7 + 1;
    bi8 = bi8 + 1;
    bi9 = bi9 + 1;
    bi10 = bi10 + 1;
    bi11 = bi11 + 1;
    bi12 = bi12 + 1;

    bi1 = (-1) + bi1;
    bi2 = (-1) + bi2;
    bi3 = (-1) + bi3;
    bi4 = (-1) + bi4;
    bi5 = (-1) + bi5;
    bi6 = (-1) + bi6;
    bi7 = (-1) + bi7;
    bi8 = (-1) + bi8;
    bi9 = (-1) + bi9;
    bi10 = (-1) + bi10;
    bi11 = (-1) + bi11;
    bi12 = (-1) + bi12;

    (void)static_cast<std::ptrdiff_t>(bi13 - bi1);
    (void)static_cast<std::ptrdiff_t>(bi14 - bi2);
    (void)static_cast<std::ptrdiff_t>(bi15 - bi3);
    (void)static_cast<std::ptrdiff_t>(bi16 - bi4);
    (void)static_cast<std::ptrdiff_t>(bi17 - bi5);
    (void)static_cast<std::ptrdiff_t>(bi18 - bi6);
    (void)static_cast<std::ptrdiff_t>(bi19 - bi7);
    (void)static_cast<std::ptrdiff_t>(bi20 - bi8);
    (void)static_cast<std::ptrdiff_t>(bi21 - bi9);
    (void)static_cast<std::ptrdiff_t>(bi22 - bi10);
    (void)static_cast<std::ptrdiff_t>(bi23 - bi11);
    (void)static_cast<std::ptrdiff_t>(bi24 - bi12);
  }
  catch (std::exception&)
  {
  }
}

} // namespace buffers_iterator_compile

//------------------------------------------------------------------------------

BOOST_ASIO_TEST_SUITE
(
  "buffers_iterator",
  BOOST_ASIO_TEST_CASE(buffers_iterator_compile::test)
)
