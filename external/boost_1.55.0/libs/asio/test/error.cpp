//
// error.cpp
// ~~~~~~~~~
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
#include <boost/asio/error.hpp>

#include <sstream>
#include "unit_test.hpp"

void test_error_code(const boost::system::error_code& code)
{
  boost::system::error_code error(code);
  BOOST_ASIO_CHECK(code == error);

  BOOST_ASIO_CHECK(!code || error);
  BOOST_ASIO_CHECK(!code || !!error);

  boost::system::error_code error2(error);
  BOOST_ASIO_CHECK(error == error2);
  BOOST_ASIO_CHECK(!(error != error2));

  boost::system::error_code error3;
  error3 = error;
  BOOST_ASIO_CHECK(error == error3);
  BOOST_ASIO_CHECK(!(error != error3));

  std::ostringstream os;
  os << error;
  BOOST_ASIO_CHECK(!os.str().empty());
}

void error_test()
{
  test_error_code(boost::asio::error::access_denied);
  test_error_code(boost::asio::error::address_family_not_supported);
  test_error_code(boost::asio::error::address_in_use);
  test_error_code(boost::asio::error::already_connected);
  test_error_code(boost::asio::error::already_started);
  test_error_code(boost::asio::error::connection_aborted);
  test_error_code(boost::asio::error::connection_refused);
  test_error_code(boost::asio::error::connection_reset);
  test_error_code(boost::asio::error::bad_descriptor);
  test_error_code(boost::asio::error::eof);
  test_error_code(boost::asio::error::fault);
  test_error_code(boost::asio::error::host_not_found);
  test_error_code(boost::asio::error::host_not_found_try_again);
  test_error_code(boost::asio::error::host_unreachable);
  test_error_code(boost::asio::error::in_progress);
  test_error_code(boost::asio::error::interrupted);
  test_error_code(boost::asio::error::invalid_argument);
  test_error_code(boost::asio::error::message_size);
  test_error_code(boost::asio::error::network_down);
  test_error_code(boost::asio::error::network_reset);
  test_error_code(boost::asio::error::network_unreachable);
  test_error_code(boost::asio::error::no_descriptors);
  test_error_code(boost::asio::error::no_buffer_space);
  test_error_code(boost::asio::error::no_data);
  test_error_code(boost::asio::error::no_memory);
  test_error_code(boost::asio::error::no_permission);
  test_error_code(boost::asio::error::no_protocol_option);
  test_error_code(boost::asio::error::no_recovery);
  test_error_code(boost::asio::error::not_connected);
  test_error_code(boost::asio::error::not_socket);
  test_error_code(boost::asio::error::operation_aborted);
  test_error_code(boost::asio::error::operation_not_supported);
  test_error_code(boost::asio::error::service_not_found);
  test_error_code(boost::asio::error::shut_down);
  test_error_code(boost::asio::error::timed_out);
  test_error_code(boost::asio::error::try_again);
  test_error_code(boost::asio::error::would_block);
}

BOOST_ASIO_TEST_SUITE
(
  "error",
  BOOST_ASIO_TEST_CASE(error_test)
)
