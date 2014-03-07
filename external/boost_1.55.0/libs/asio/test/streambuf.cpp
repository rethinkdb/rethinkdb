//
// streambuf.cpp
// ~~~~~~~~~~~~~
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
#include <boost/asio/streambuf.hpp>

#include <boost/asio/buffer.hpp>
#include "unit_test.hpp"

void streambuf_test()
{
  boost::asio::streambuf sb;

  sb.sputn("abcd", 4);

  BOOST_ASIO_CHECK(sb.size() == 4);

  for (int i = 0; i < 100; ++i)
  {
    sb.consume(3);

    BOOST_ASIO_CHECK(sb.size() == 1);

    char buf[1];
    sb.sgetn(buf, 1);

    BOOST_ASIO_CHECK(sb.size() == 0);

    sb.sputn("ab", 2);

    BOOST_ASIO_CHECK(sb.size() == 2);

    boost::asio::buffer_copy(sb.prepare(10), boost::asio::buffer("cd", 2));
    sb.commit(2);

    BOOST_ASIO_CHECK(sb.size() == 4);
  }

  BOOST_ASIO_CHECK(sb.size() == 4);

  sb.consume(4);

  BOOST_ASIO_CHECK(sb.size() == 0);
}

BOOST_ASIO_TEST_SUITE
(
  "streambuf",
  BOOST_ASIO_TEST_CASE(streambuf_test)
)
