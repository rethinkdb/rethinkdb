//
// v6_only.cpp
// ~~~~~~~~~~~
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
#include <boost/asio/ip/v6_only.hpp>

#include <boost/asio/io_service.hpp>
#include <boost/asio/ip/tcp.hpp>
#include <boost/asio/ip/udp.hpp>
#include "../unit_test.hpp"

//------------------------------------------------------------------------------

// ip_v6_only_compile test
// ~~~~~~~~~~~~~~~~~~~~~~~
// The following test checks that the ip::v6_only socket option compiles and
// link correctly. Runtime failures are ignored.

namespace ip_v6_only_compile {

void test()
{
  using namespace boost::asio;
  namespace ip = boost::asio::ip;

  try
  {
    io_service ios;
    ip::udp::socket sock(ios);

    // v6_only class.

    ip::v6_only v6_only1(true);
    sock.set_option(v6_only1);
    ip::v6_only v6_only2;
    sock.get_option(v6_only2);
    v6_only1 = true;
    (void)static_cast<bool>(v6_only1);
    (void)static_cast<bool>(!v6_only1);
    (void)static_cast<bool>(v6_only1.value());
  }
  catch (std::exception&)
  {
  }
}

} // namespace ip_v6_only_compile

//------------------------------------------------------------------------------

// ip_v6_only_runtime test
// ~~~~~~~~~~~~~~~~~~~~~~~
// The following test checks the runtime operation of the ip::v6_only socket
// option.

namespace ip_v6_only_runtime {

void test()
{
  using namespace boost::asio;
  namespace ip = boost::asio::ip;

  io_service ios;
  boost::system::error_code ec;

  ip::tcp::endpoint ep_v6(ip::address_v6::loopback(), 0);
  ip::tcp::acceptor acceptor_v6(ios);
  acceptor_v6.open(ep_v6.protocol(), ec);
  acceptor_v6.bind(ep_v6, ec);
  bool have_v6 = !ec;
  acceptor_v6.close(ec);
  acceptor_v6.open(ep_v6.protocol(), ec);

  if (have_v6)
  {
    ip::v6_only v6_only1;
    acceptor_v6.get_option(v6_only1, ec);
    BOOST_ASIO_CHECK(!ec);
    bool have_dual_stack = !v6_only1.value();

    if (have_dual_stack)
    {
      ip::v6_only v6_only2(false);
      BOOST_ASIO_CHECK(!v6_only2.value());
      BOOST_ASIO_CHECK(!static_cast<bool>(v6_only2));
      BOOST_ASIO_CHECK(!v6_only2);
      acceptor_v6.set_option(v6_only2, ec);
      BOOST_ASIO_CHECK(!ec);

      ip::v6_only v6_only3;
      acceptor_v6.get_option(v6_only3, ec);
      BOOST_ASIO_CHECK(!ec);
      BOOST_ASIO_CHECK(!v6_only3.value());
      BOOST_ASIO_CHECK(!static_cast<bool>(v6_only3));
      BOOST_ASIO_CHECK(!v6_only3);

      ip::v6_only v6_only4(true);
      BOOST_ASIO_CHECK(v6_only4.value());
      BOOST_ASIO_CHECK(static_cast<bool>(v6_only4));
      BOOST_ASIO_CHECK(!!v6_only4);
      acceptor_v6.set_option(v6_only4, ec);
      BOOST_ASIO_CHECK(!ec);

      ip::v6_only v6_only5;
      acceptor_v6.get_option(v6_only5, ec);
      BOOST_ASIO_CHECK(!ec);
      BOOST_ASIO_CHECK(v6_only5.value());
      BOOST_ASIO_CHECK(static_cast<bool>(v6_only5));
      BOOST_ASIO_CHECK(!!v6_only5);
    }
  }
}

} // namespace ip_v6_only_runtime

//------------------------------------------------------------------------------

BOOST_ASIO_TEST_SUITE
(
  "ip/v6_only",
  BOOST_ASIO_TEST_CASE(ip_v6_only_compile::test)
  BOOST_ASIO_TEST_CASE(ip_v6_only_runtime::test)
)
