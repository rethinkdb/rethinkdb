//
// socket_base.cpp
// ~~~~~~~~~~~~~~~
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
#include <boost/asio/socket_base.hpp>

#include <boost/asio/io_service.hpp>
#include <boost/asio/ip/tcp.hpp>
#include <boost/asio/ip/udp.hpp>
#include "unit_test.hpp"

//------------------------------------------------------------------------------

// socket_base_compile test
// ~~~~~~~~~~~~~~~~~~~~~~~~
// The following test checks that all nested classes, enums and constants in
// socket_base compile and link correctly. Runtime failures are ignored.

namespace socket_base_compile {

void test()
{
  using namespace boost::asio;
  namespace ip = boost::asio::ip;

  try
  {
    io_service ios;
    ip::tcp::socket sock(ios);
    char buf[1024];

    // shutdown_type enumeration.

    sock.shutdown(socket_base::shutdown_receive);
    sock.shutdown(socket_base::shutdown_send);
    sock.shutdown(socket_base::shutdown_both);

    // message_flags constants.

    sock.receive(buffer(buf), socket_base::message_peek);
    sock.receive(buffer(buf), socket_base::message_out_of_band);
    sock.send(buffer(buf), socket_base::message_do_not_route);

    // broadcast class.

    socket_base::broadcast broadcast1(true);
    sock.set_option(broadcast1);
    socket_base::broadcast broadcast2;
    sock.get_option(broadcast2);
    broadcast1 = true;
    (void)static_cast<bool>(broadcast1);
    (void)static_cast<bool>(!broadcast1);
    (void)static_cast<bool>(broadcast1.value());

    // debug class.

    socket_base::debug debug1(true);
    sock.set_option(debug1);
    socket_base::debug debug2;
    sock.get_option(debug2);
    debug1 = true;
    (void)static_cast<bool>(debug1);
    (void)static_cast<bool>(!debug1);
    (void)static_cast<bool>(debug1.value());

    // do_not_route class.

    socket_base::do_not_route do_not_route1(true);
    sock.set_option(do_not_route1);
    socket_base::do_not_route do_not_route2;
    sock.get_option(do_not_route2);
    do_not_route1 = true;
    (void)static_cast<bool>(do_not_route1);
    (void)static_cast<bool>(!do_not_route1);
    (void)static_cast<bool>(do_not_route1.value());

    // keep_alive class.

    socket_base::keep_alive keep_alive1(true);
    sock.set_option(keep_alive1);
    socket_base::keep_alive keep_alive2;
    sock.get_option(keep_alive2);
    keep_alive1 = true;
    (void)static_cast<bool>(keep_alive1);
    (void)static_cast<bool>(!keep_alive1);
    (void)static_cast<bool>(keep_alive1.value());

    // send_buffer_size class.

    socket_base::send_buffer_size send_buffer_size1(1024);
    sock.set_option(send_buffer_size1);
    socket_base::send_buffer_size send_buffer_size2;
    sock.get_option(send_buffer_size2);
    send_buffer_size1 = 1;
    (void)static_cast<int>(send_buffer_size1.value());

    // send_low_watermark class.

    socket_base::send_low_watermark send_low_watermark1(128);
    sock.set_option(send_low_watermark1);
    socket_base::send_low_watermark send_low_watermark2;
    sock.get_option(send_low_watermark2);
    send_low_watermark1 = 1;
    (void)static_cast<int>(send_low_watermark1.value());

    // receive_buffer_size class.

    socket_base::receive_buffer_size receive_buffer_size1(1024);
    sock.set_option(receive_buffer_size1);
    socket_base::receive_buffer_size receive_buffer_size2;
    sock.get_option(receive_buffer_size2);
    receive_buffer_size1 = 1;
    (void)static_cast<int>(receive_buffer_size1.value());

    // receive_low_watermark class.

    socket_base::receive_low_watermark receive_low_watermark1(128);
    sock.set_option(receive_low_watermark1);
    socket_base::receive_low_watermark receive_low_watermark2;
    sock.get_option(receive_low_watermark2);
    receive_low_watermark1 = 1;
    (void)static_cast<int>(receive_low_watermark1.value());

    // reuse_address class.

    socket_base::reuse_address reuse_address1(true);
    sock.set_option(reuse_address1);
    socket_base::reuse_address reuse_address2;
    sock.get_option(reuse_address2);
    reuse_address1 = true;
    (void)static_cast<bool>(reuse_address1);
    (void)static_cast<bool>(!reuse_address1);
    (void)static_cast<bool>(reuse_address1.value());

    // linger class.

    socket_base::linger linger1(true, 30);
    sock.set_option(linger1);
    socket_base::linger linger2;
    sock.get_option(linger2);
    linger1.enabled(true);
    (void)static_cast<bool>(linger1.enabled());
    linger1.timeout(1);
    (void)static_cast<int>(linger1.timeout());

    // enable_connection_aborted class.

    socket_base::enable_connection_aborted enable_connection_aborted1(true);
    sock.set_option(enable_connection_aborted1);
    socket_base::enable_connection_aborted enable_connection_aborted2;
    sock.get_option(enable_connection_aborted2);
    enable_connection_aborted1 = true;
    (void)static_cast<bool>(enable_connection_aborted1);
    (void)static_cast<bool>(!enable_connection_aborted1);
    (void)static_cast<bool>(enable_connection_aborted1.value());

    // non_blocking_io class.

    socket_base::non_blocking_io non_blocking_io(true);
    sock.io_control(non_blocking_io);

    // bytes_readable class.

    socket_base::bytes_readable bytes_readable;
    sock.io_control(bytes_readable);
    std::size_t bytes = bytes_readable.get();
    (void)bytes;
  }
  catch (std::exception&)
  {
  }
}

} // namespace socket_base_compile

//------------------------------------------------------------------------------

// socket_base_runtime test
// ~~~~~~~~~~~~~~~~~~~~~~~~
// The following test checks the runtime operation of the socket options and I/O
// control commands defined in socket_base.

namespace socket_base_runtime {

void test()
{
  using namespace boost::asio;
  namespace ip = boost::asio::ip;

  io_service ios;
  ip::udp::socket udp_sock(ios, ip::udp::v4());
  ip::tcp::socket tcp_sock(ios, ip::tcp::v4());
  ip::tcp::acceptor tcp_acceptor(ios, ip::tcp::v4());
  boost::system::error_code ec;

  // broadcast class.

  socket_base::broadcast broadcast1(true);
  BOOST_ASIO_CHECK(broadcast1.value());
  BOOST_ASIO_CHECK(static_cast<bool>(broadcast1));
  BOOST_ASIO_CHECK(!!broadcast1);
  udp_sock.set_option(broadcast1, ec);
  BOOST_ASIO_CHECK_MESSAGE(!ec, ec.value() << ", " << ec.message());

  socket_base::broadcast broadcast2;
  udp_sock.get_option(broadcast2, ec);
  BOOST_ASIO_CHECK_MESSAGE(!ec, ec.value() << ", " << ec.message());
  BOOST_ASIO_CHECK(broadcast2.value());
  BOOST_ASIO_CHECK(static_cast<bool>(broadcast2));
  BOOST_ASIO_CHECK(!!broadcast2);

  socket_base::broadcast broadcast3(false);
  BOOST_ASIO_CHECK(!broadcast3.value());
  BOOST_ASIO_CHECK(!static_cast<bool>(broadcast3));
  BOOST_ASIO_CHECK(!broadcast3);
  udp_sock.set_option(broadcast3, ec);
  BOOST_ASIO_CHECK_MESSAGE(!ec, ec.value() << ", " << ec.message());

  socket_base::broadcast broadcast4;
  udp_sock.get_option(broadcast4, ec);
  BOOST_ASIO_CHECK_MESSAGE(!ec, ec.value() << ", " << ec.message());
  BOOST_ASIO_CHECK(!broadcast4.value());
  BOOST_ASIO_CHECK(!static_cast<bool>(broadcast4));
  BOOST_ASIO_CHECK(!broadcast4);

  // debug class.

  socket_base::debug debug1(true);
  BOOST_ASIO_CHECK(debug1.value());
  BOOST_ASIO_CHECK(static_cast<bool>(debug1));
  BOOST_ASIO_CHECK(!!debug1);
  udp_sock.set_option(debug1, ec);
#if defined(__linux__)
  // On Linux, only root can set SO_DEBUG.
  bool not_root = (ec == boost::asio::error::access_denied);
  BOOST_ASIO_CHECK(!ec || not_root);
  BOOST_ASIO_WARN_MESSAGE(!ec, "Must be root to set debug socket option");
#else // defined(__linux__)
# if defined(BOOST_ASIO_WINDOWS) && defined(UNDER_CE)
  // Option is not supported under Windows CE.
  BOOST_ASIO_CHECK_MESSAGE(ec == boost::asio::error::no_protocol_option,
      ec.value() << ", " << ec.message());
# else // defined(BOOST_ASIO_WINDOWS) && defined(UNDER_CE)
  BOOST_ASIO_CHECK_MESSAGE(!ec, ec.value() << ", " << ec.message());
# endif // defined(BOOST_ASIO_WINDOWS) && defined(UNDER_CE)
#endif // defined(__linux__)

  socket_base::debug debug2;
  udp_sock.get_option(debug2, ec);
#if defined(BOOST_ASIO_WINDOWS) && defined(UNDER_CE)
  // Option is not supported under Windows CE.
  BOOST_ASIO_CHECK_MESSAGE(ec == boost::asio::error::no_protocol_option,
      ec.value() << ", " << ec.message());
#else // defined(BOOST_ASIO_WINDOWS) && defined(UNDER_CE)
  BOOST_ASIO_CHECK_MESSAGE(!ec, ec.value() << ", " << ec.message());
# if defined(__linux__)
  BOOST_ASIO_CHECK(debug2.value() || not_root);
  BOOST_ASIO_CHECK(static_cast<bool>(debug2) || not_root);
  BOOST_ASIO_CHECK(!!debug2 || not_root);
# else // defined(__linux__)
  BOOST_ASIO_CHECK(debug2.value());
  BOOST_ASIO_CHECK(static_cast<bool>(debug2));
  BOOST_ASIO_CHECK(!!debug2);
# endif // defined(__linux__)
#endif // defined(BOOST_ASIO_WINDOWS) && defined(UNDER_CE)

  socket_base::debug debug3(false);
  BOOST_ASIO_CHECK(!debug3.value());
  BOOST_ASIO_CHECK(!static_cast<bool>(debug3));
  BOOST_ASIO_CHECK(!debug3);
  udp_sock.set_option(debug3, ec);
#if defined(__linux__)
  BOOST_ASIO_CHECK(!ec || not_root);
#else // defined(__linux__)
# if defined(BOOST_ASIO_WINDOWS) && defined(UNDER_CE)
  // Option is not supported under Windows CE.
  BOOST_ASIO_CHECK_MESSAGE(ec == boost::asio::error::no_protocol_option,
      ec.value() << ", " << ec.message());
# else // defined(BOOST_ASIO_WINDOWS) && defined(UNDER_CE)
  BOOST_ASIO_CHECK_MESSAGE(!ec, ec.value() << ", " << ec.message());
# endif // defined(BOOST_ASIO_WINDOWS) && defined(UNDER_CE)
#endif // defined(__linux__)

  socket_base::debug debug4;
  udp_sock.get_option(debug4, ec);
#if defined(BOOST_ASIO_WINDOWS) && defined(UNDER_CE)
  // Option is not supported under Windows CE.
  BOOST_ASIO_CHECK_MESSAGE(ec == boost::asio::error::no_protocol_option,
      ec.value() << ", " << ec.message());
#else // defined(BOOST_ASIO_WINDOWS) && defined(UNDER_CE)
  BOOST_ASIO_CHECK_MESSAGE(!ec, ec.value() << ", " << ec.message());
# if defined(__linux__)
  BOOST_ASIO_CHECK(!debug4.value() || not_root);
  BOOST_ASIO_CHECK(!static_cast<bool>(debug4) || not_root);
  BOOST_ASIO_CHECK(!debug4 || not_root);
# else // defined(__linux__)
  BOOST_ASIO_CHECK(!debug4.value());
  BOOST_ASIO_CHECK(!static_cast<bool>(debug4));
  BOOST_ASIO_CHECK(!debug4);
# endif // defined(__linux__)
#endif // defined(BOOST_ASIO_WINDOWS) && defined(UNDER_CE)

  // do_not_route class.

  socket_base::do_not_route do_not_route1(true);
  BOOST_ASIO_CHECK(do_not_route1.value());
  BOOST_ASIO_CHECK(static_cast<bool>(do_not_route1));
  BOOST_ASIO_CHECK(!!do_not_route1);
  udp_sock.set_option(do_not_route1, ec);
#if defined(BOOST_ASIO_WINDOWS) && defined(UNDER_CE)
  // Option is not supported under Windows CE.
  BOOST_ASIO_CHECK_MESSAGE(ec == boost::asio::error::no_protocol_option,
      ec.value() << ", " << ec.message());
#else // defined(BOOST_ASIO_WINDOWS) && defined(UNDER_CE)
  BOOST_ASIO_CHECK_MESSAGE(!ec, ec.value() << ", " << ec.message());
#endif // defined(BOOST_ASIO_WINDOWS) && defined(UNDER_CE)

  socket_base::do_not_route do_not_route2;
  udp_sock.get_option(do_not_route2, ec);
#if defined(BOOST_ASIO_WINDOWS) && defined(UNDER_CE)
  // Option is not supported under Windows CE.
  BOOST_ASIO_CHECK_MESSAGE(ec == boost::asio::error::no_protocol_option,
      ec.value() << ", " << ec.message());
#else // defined(BOOST_ASIO_WINDOWS) && defined(UNDER_CE)
  BOOST_ASIO_CHECK_MESSAGE(!ec, ec.value() << ", " << ec.message());
  BOOST_ASIO_CHECK(do_not_route2.value());
  BOOST_ASIO_CHECK(static_cast<bool>(do_not_route2));
  BOOST_ASIO_CHECK(!!do_not_route2);
#endif // defined(BOOST_ASIO_WINDOWS) && defined(UNDER_CE)

  socket_base::do_not_route do_not_route3(false);
  BOOST_ASIO_CHECK(!do_not_route3.value());
  BOOST_ASIO_CHECK(!static_cast<bool>(do_not_route3));
  BOOST_ASIO_CHECK(!do_not_route3);
  udp_sock.set_option(do_not_route3, ec);
#if defined(BOOST_ASIO_WINDOWS) && defined(UNDER_CE)
  // Option is not supported under Windows CE.
  BOOST_ASIO_CHECK_MESSAGE(ec == boost::asio::error::no_protocol_option,
      ec.value() << ", " << ec.message());
#else // defined(BOOST_ASIO_WINDOWS) && defined(UNDER_CE)
  BOOST_ASIO_CHECK_MESSAGE(!ec, ec.value() << ", " << ec.message());
#endif // defined(BOOST_ASIO_WINDOWS) && defined(UNDER_CE)

  socket_base::do_not_route do_not_route4;
  udp_sock.get_option(do_not_route4, ec);
#if defined(BOOST_ASIO_WINDOWS) && defined(UNDER_CE)
  // Option is not supported under Windows CE.
  BOOST_ASIO_CHECK_MESSAGE(ec == boost::asio::error::no_protocol_option,
      ec.value() << ", " << ec.message());
#else // defined(BOOST_ASIO_WINDOWS) && defined(UNDER_CE)
  BOOST_ASIO_CHECK_MESSAGE(!ec, ec.value() << ", " << ec.message());
  BOOST_ASIO_CHECK(!do_not_route4.value());
  BOOST_ASIO_CHECK(!static_cast<bool>(do_not_route4));
  BOOST_ASIO_CHECK(!do_not_route4);
#endif // defined(BOOST_ASIO_WINDOWS) && defined(UNDER_CE)

  // keep_alive class.

  socket_base::keep_alive keep_alive1(true);
  BOOST_ASIO_CHECK(keep_alive1.value());
  BOOST_ASIO_CHECK(static_cast<bool>(keep_alive1));
  BOOST_ASIO_CHECK(!!keep_alive1);
  tcp_sock.set_option(keep_alive1, ec);
  BOOST_ASIO_CHECK_MESSAGE(!ec, ec.value() << ", " << ec.message());

  socket_base::keep_alive keep_alive2;
  tcp_sock.get_option(keep_alive2, ec);
  BOOST_ASIO_CHECK_MESSAGE(!ec, ec.value() << ", " << ec.message());
  BOOST_ASIO_CHECK(keep_alive2.value());
  BOOST_ASIO_CHECK(static_cast<bool>(keep_alive2));
  BOOST_ASIO_CHECK(!!keep_alive2);

  socket_base::keep_alive keep_alive3(false);
  BOOST_ASIO_CHECK(!keep_alive3.value());
  BOOST_ASIO_CHECK(!static_cast<bool>(keep_alive3));
  BOOST_ASIO_CHECK(!keep_alive3);
  tcp_sock.set_option(keep_alive3, ec);
  BOOST_ASIO_CHECK_MESSAGE(!ec, ec.value() << ", " << ec.message());

  socket_base::keep_alive keep_alive4;
  tcp_sock.get_option(keep_alive4, ec);
  BOOST_ASIO_CHECK_MESSAGE(!ec, ec.value() << ", " << ec.message());
  BOOST_ASIO_CHECK(!keep_alive4.value());
  BOOST_ASIO_CHECK(!static_cast<bool>(keep_alive4));
  BOOST_ASIO_CHECK(!keep_alive4);

  // send_buffer_size class.

  socket_base::send_buffer_size send_buffer_size1(4096);
  BOOST_ASIO_CHECK(send_buffer_size1.value() == 4096);
  tcp_sock.set_option(send_buffer_size1, ec);
  BOOST_ASIO_CHECK_MESSAGE(!ec, ec.value() << ", " << ec.message());

  socket_base::send_buffer_size send_buffer_size2;
  tcp_sock.get_option(send_buffer_size2, ec);
  BOOST_ASIO_CHECK_MESSAGE(!ec, ec.value() << ", " << ec.message());
  BOOST_ASIO_CHECK(send_buffer_size2.value() == 4096);

  socket_base::send_buffer_size send_buffer_size3(16384);
  BOOST_ASIO_CHECK(send_buffer_size3.value() == 16384);
  tcp_sock.set_option(send_buffer_size3, ec);
  BOOST_ASIO_CHECK_MESSAGE(!ec, ec.value() << ", " << ec.message());

  socket_base::send_buffer_size send_buffer_size4;
  tcp_sock.get_option(send_buffer_size4, ec);
  BOOST_ASIO_CHECK_MESSAGE(!ec, ec.value() << ", " << ec.message());
  BOOST_ASIO_CHECK(send_buffer_size4.value() == 16384);

  // send_low_watermark class.

  socket_base::send_low_watermark send_low_watermark1(4096);
  BOOST_ASIO_CHECK(send_low_watermark1.value() == 4096);
  tcp_sock.set_option(send_low_watermark1, ec);
#if defined(WIN32) || defined(__linux__) || defined(__sun)
  BOOST_ASIO_CHECK(!!ec); // Not supported on Windows, Linux or Solaris.
#else
  BOOST_ASIO_CHECK_MESSAGE(!ec, ec.value() << ", " << ec.message());
#endif

  socket_base::send_low_watermark send_low_watermark2;
  tcp_sock.get_option(send_low_watermark2, ec);
#if defined(WIN32) || defined(__sun)
  BOOST_ASIO_CHECK(!!ec); // Not supported on Windows or Solaris.
#elif defined(__linux__)
  BOOST_ASIO_CHECK(!ec); // Not supported on Linux but can get value.
#else
  BOOST_ASIO_CHECK_MESSAGE(!ec, ec.value() << ", " << ec.message());
  BOOST_ASIO_CHECK(send_low_watermark2.value() == 4096);
#endif

  socket_base::send_low_watermark send_low_watermark3(8192);
  BOOST_ASIO_CHECK(send_low_watermark3.value() == 8192);
  tcp_sock.set_option(send_low_watermark3, ec);
#if defined(WIN32) || defined(__linux__) || defined(__sun)
  BOOST_ASIO_CHECK(!!ec); // Not supported on Windows, Linux or Solaris.
#else
  BOOST_ASIO_CHECK_MESSAGE(!ec, ec.value() << ", " << ec.message());
#endif

  socket_base::send_low_watermark send_low_watermark4;
  tcp_sock.get_option(send_low_watermark4, ec);
#if defined(WIN32) || defined(__sun)
  BOOST_ASIO_CHECK(!!ec); // Not supported on Windows or Solaris.
#elif defined(__linux__)
  BOOST_ASIO_CHECK(!ec); // Not supported on Linux but can get value.
#else
  BOOST_ASIO_CHECK_MESSAGE(!ec, ec.value() << ", " << ec.message());
  BOOST_ASIO_CHECK(send_low_watermark4.value() == 8192);
#endif

  // receive_buffer_size class.

  socket_base::receive_buffer_size receive_buffer_size1(4096);
  BOOST_ASIO_CHECK(receive_buffer_size1.value() == 4096);
  tcp_sock.set_option(receive_buffer_size1, ec);
#if defined(BOOST_ASIO_WINDOWS) && defined(UNDER_CE)
  // Option is not supported under Windows CE.
  BOOST_ASIO_CHECK_MESSAGE(ec == boost::asio::error::no_protocol_option,
      ec.value() << ", " << ec.message());
#else // defined(BOOST_ASIO_WINDOWS) && defined(UNDER_CE)
  BOOST_ASIO_CHECK_MESSAGE(!ec, ec.value() << ", " << ec.message());
#endif // defined(BOOST_ASIO_WINDOWS) && defined(UNDER_CE)

  socket_base::receive_buffer_size receive_buffer_size2;
  tcp_sock.get_option(receive_buffer_size2, ec);
#if defined(BOOST_ASIO_WINDOWS) && defined(UNDER_CE)
  BOOST_ASIO_CHECK(!ec); // Not supported under Windows CE but can get value.
#else // defined(BOOST_ASIO_WINDOWS) && defined(UNDER_CE)
  BOOST_ASIO_CHECK_MESSAGE(!ec, ec.value() << ", " << ec.message());
  BOOST_ASIO_CHECK(receive_buffer_size2.value() == 4096);
#endif // defined(BOOST_ASIO_WINDOWS) && defined(UNDER_CE)

  socket_base::receive_buffer_size receive_buffer_size3(16384);
  BOOST_ASIO_CHECK(receive_buffer_size3.value() == 16384);
  tcp_sock.set_option(receive_buffer_size3, ec);
#if defined(BOOST_ASIO_WINDOWS) && defined(UNDER_CE)
  // Option is not supported under Windows CE.
  BOOST_ASIO_CHECK_MESSAGE(ec == boost::asio::error::no_protocol_option,
      ec.value() << ", " << ec.message());
#else // defined(BOOST_ASIO_WINDOWS) && defined(UNDER_CE)
  BOOST_ASIO_CHECK_MESSAGE(!ec, ec.value() << ", " << ec.message());
#endif // defined(BOOST_ASIO_WINDOWS) && defined(UNDER_CE)

  socket_base::receive_buffer_size receive_buffer_size4;
  tcp_sock.get_option(receive_buffer_size4, ec);
#if defined(BOOST_ASIO_WINDOWS) && defined(UNDER_CE)
  BOOST_ASIO_CHECK(!ec); // Not supported under Windows CE but can get value.
#else // defined(BOOST_ASIO_WINDOWS) && defined(UNDER_CE)
  BOOST_ASIO_CHECK_MESSAGE(!ec, ec.value() << ", " << ec.message());
  BOOST_ASIO_CHECK(receive_buffer_size4.value() == 16384);
#endif // defined(BOOST_ASIO_WINDOWS) && defined(UNDER_CE)

  // receive_low_watermark class.

  socket_base::receive_low_watermark receive_low_watermark1(4096);
  BOOST_ASIO_CHECK(receive_low_watermark1.value() == 4096);
  tcp_sock.set_option(receive_low_watermark1, ec);
#if defined(WIN32) || defined(__sun)
  BOOST_ASIO_CHECK(!!ec); // Not supported on Windows or Solaris.
#else
  BOOST_ASIO_CHECK_MESSAGE(!ec, ec.value() << ", " << ec.message());
#endif

  socket_base::receive_low_watermark receive_low_watermark2;
  tcp_sock.get_option(receive_low_watermark2, ec);
#if defined(WIN32) || defined(__sun)
  BOOST_ASIO_CHECK(!!ec); // Not supported on Windows or Solaris.
#else
  BOOST_ASIO_CHECK_MESSAGE(!ec, ec.value() << ", " << ec.message());
  BOOST_ASIO_CHECK(receive_low_watermark2.value() == 4096);
#endif

  socket_base::receive_low_watermark receive_low_watermark3(8192);
  BOOST_ASIO_CHECK(receive_low_watermark3.value() == 8192);
  tcp_sock.set_option(receive_low_watermark3, ec);
#if defined(WIN32) || defined(__sun)
  BOOST_ASIO_CHECK(!!ec); // Not supported on Windows or Solaris.
#else
  BOOST_ASIO_CHECK_MESSAGE(!ec, ec.value() << ", " << ec.message());
#endif

  socket_base::receive_low_watermark receive_low_watermark4;
  tcp_sock.get_option(receive_low_watermark4, ec);
#if defined(WIN32) || defined(__sun)
  BOOST_ASIO_CHECK(!!ec); // Not supported on Windows or Solaris.
#else
  BOOST_ASIO_CHECK_MESSAGE(!ec, ec.value() << ", " << ec.message());
  BOOST_ASIO_CHECK(receive_low_watermark4.value() == 8192);
#endif

  // reuse_address class.

  socket_base::reuse_address reuse_address1(true);
  BOOST_ASIO_CHECK(reuse_address1.value());
  BOOST_ASIO_CHECK(static_cast<bool>(reuse_address1));
  BOOST_ASIO_CHECK(!!reuse_address1);
  udp_sock.set_option(reuse_address1, ec);
  BOOST_ASIO_CHECK_MESSAGE(!ec, ec.value() << ", " << ec.message());

  socket_base::reuse_address reuse_address2;
  udp_sock.get_option(reuse_address2, ec);
  BOOST_ASIO_CHECK_MESSAGE(!ec, ec.value() << ", " << ec.message());
  BOOST_ASIO_CHECK(reuse_address2.value());
  BOOST_ASIO_CHECK(static_cast<bool>(reuse_address2));
  BOOST_ASIO_CHECK(!!reuse_address2);

  socket_base::reuse_address reuse_address3(false);
  BOOST_ASIO_CHECK(!reuse_address3.value());
  BOOST_ASIO_CHECK(!static_cast<bool>(reuse_address3));
  BOOST_ASIO_CHECK(!reuse_address3);
  udp_sock.set_option(reuse_address3, ec);
  BOOST_ASIO_CHECK_MESSAGE(!ec, ec.value() << ", " << ec.message());

  socket_base::reuse_address reuse_address4;
  udp_sock.get_option(reuse_address4, ec);
  BOOST_ASIO_CHECK_MESSAGE(!ec, ec.value() << ", " << ec.message());
  BOOST_ASIO_CHECK(!reuse_address4.value());
  BOOST_ASIO_CHECK(!static_cast<bool>(reuse_address4));
  BOOST_ASIO_CHECK(!reuse_address4);

  // linger class.

  socket_base::linger linger1(true, 60);
  BOOST_ASIO_CHECK(linger1.enabled());
  BOOST_ASIO_CHECK(linger1.timeout() == 60);
  tcp_sock.set_option(linger1, ec);
  BOOST_ASIO_CHECK_MESSAGE(!ec, ec.value() << ", " << ec.message());

  socket_base::linger linger2;
  tcp_sock.get_option(linger2, ec);
  BOOST_ASIO_CHECK_MESSAGE(!ec, ec.value() << ", " << ec.message());
  BOOST_ASIO_CHECK(linger2.enabled());
  BOOST_ASIO_CHECK(linger2.timeout() == 60);

  socket_base::linger linger3(false, 0);
  BOOST_ASIO_CHECK(!linger3.enabled());
  BOOST_ASIO_CHECK(linger3.timeout() == 0);
  tcp_sock.set_option(linger3, ec);
  BOOST_ASIO_CHECK_MESSAGE(!ec, ec.value() << ", " << ec.message());

  socket_base::linger linger4;
  tcp_sock.get_option(linger4, ec);
  BOOST_ASIO_CHECK_MESSAGE(!ec, ec.value() << ", " << ec.message());
  BOOST_ASIO_CHECK(!linger4.enabled());

  // enable_connection_aborted class.

  socket_base::enable_connection_aborted enable_connection_aborted1(true);
  BOOST_ASIO_CHECK(enable_connection_aborted1.value());
  BOOST_ASIO_CHECK(static_cast<bool>(enable_connection_aborted1));
  BOOST_ASIO_CHECK(!!enable_connection_aborted1);
  tcp_acceptor.set_option(enable_connection_aborted1, ec);
  BOOST_ASIO_CHECK_MESSAGE(!ec, ec.value() << ", " << ec.message());

  socket_base::enable_connection_aborted enable_connection_aborted2;
  tcp_acceptor.get_option(enable_connection_aborted2, ec);
  BOOST_ASIO_CHECK_MESSAGE(!ec, ec.value() << ", " << ec.message());
  BOOST_ASIO_CHECK(enable_connection_aborted2.value());
  BOOST_ASIO_CHECK(static_cast<bool>(enable_connection_aborted2));
  BOOST_ASIO_CHECK(!!enable_connection_aborted2);

  socket_base::enable_connection_aborted enable_connection_aborted3(false);
  BOOST_ASIO_CHECK(!enable_connection_aborted3.value());
  BOOST_ASIO_CHECK(!static_cast<bool>(enable_connection_aborted3));
  BOOST_ASIO_CHECK(!enable_connection_aborted3);
  tcp_acceptor.set_option(enable_connection_aborted3, ec);
  BOOST_ASIO_CHECK_MESSAGE(!ec, ec.value() << ", " << ec.message());

  socket_base::enable_connection_aborted enable_connection_aborted4;
  tcp_acceptor.get_option(enable_connection_aborted4, ec);
  BOOST_ASIO_CHECK_MESSAGE(!ec, ec.value() << ", " << ec.message());
  BOOST_ASIO_CHECK(!enable_connection_aborted4.value());
  BOOST_ASIO_CHECK(!static_cast<bool>(enable_connection_aborted4));
  BOOST_ASIO_CHECK(!enable_connection_aborted4);

  // non_blocking_io class.

  socket_base::non_blocking_io non_blocking_io1(true);
  tcp_sock.io_control(non_blocking_io1, ec);
  BOOST_ASIO_CHECK_MESSAGE(!ec, ec.value() << ", " << ec.message());

  socket_base::non_blocking_io non_blocking_io2(false);
  tcp_sock.io_control(non_blocking_io2, ec);
  BOOST_ASIO_CHECK_MESSAGE(!ec, ec.value() << ", " << ec.message());

  // bytes_readable class.

  socket_base::bytes_readable bytes_readable;
  udp_sock.io_control(bytes_readable, ec);
  BOOST_ASIO_CHECK_MESSAGE(!ec, ec.value() << ", " << ec.message());
}

} // namespace socket_base_runtime

//------------------------------------------------------------------------------

BOOST_ASIO_TEST_SUITE
(
  "socket_base",
  BOOST_ASIO_TEST_CASE(socket_base_compile::test)
  BOOST_ASIO_TEST_CASE(socket_base_runtime::test)
)
