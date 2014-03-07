//
// generic/datagram_protocol.cpp
// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
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
#include <boost/asio/generic/datagram_protocol.hpp>

#include <cstring>
#include <boost/asio/io_service.hpp>
#include <boost/asio/ip/udp.hpp>
#include "../unit_test.hpp"

#if defined(__cplusplus_cli) || defined(__cplusplus_winrt)
# define generic cpp_generic
#endif

//------------------------------------------------------------------------------

// generic_datagram_protocol_socket_compile test
// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
// The following test checks that all public member functions on the class
// generic::datagram_socket::socket compile and link correctly. Runtime
// failures are ignored.

namespace generic_datagram_protocol_socket_compile {

void connect_handler(const boost::system::error_code&)
{
}

void send_handler(const boost::system::error_code&, std::size_t)
{
}

void receive_handler(const boost::system::error_code&, std::size_t)
{
}

void test()
{
  using namespace boost::asio;
  namespace generic = boost::asio::generic;
  typedef generic::datagram_protocol dp;

  const int af_inet = BOOST_ASIO_OS_DEF(AF_INET);
  const int ipproto_udp = BOOST_ASIO_OS_DEF(IPPROTO_UDP);
  const int sock_dgram = BOOST_ASIO_OS_DEF(SOCK_DGRAM);

  try
  {
    io_service ios;
    char mutable_char_buffer[128] = "";
    const char const_char_buffer[128] = "";
    socket_base::message_flags in_flags = 0;
    socket_base::send_buffer_size socket_option;
    socket_base::bytes_readable io_control_command;
    boost::system::error_code ec;

    // basic_datagram_socket constructors.

    dp::socket socket1(ios);
    dp::socket socket2(ios, dp(af_inet, ipproto_udp));
    dp::socket socket3(ios, dp::endpoint());
#if !defined(BOOST_ASIO_WINDOWS_RUNTIME)
    int native_socket1 = ::socket(af_inet, sock_dgram, 0);
    dp::socket socket4(ios, dp(af_inet, ipproto_udp), native_socket1);
#endif // !defined(BOOST_ASIO_WINDOWS_RUNTIME)

#if defined(BOOST_ASIO_HAS_MOVE)
    dp::socket socket5(std::move(socket4));
    boost::asio::ip::udp::socket udp_socket(ios);
    dp::socket socket6(std::move(udp_socket));
#endif // defined(BOOST_ASIO_HAS_MOVE)

    // basic_datagram_socket operators.

#if defined(BOOST_ASIO_HAS_MOVE)
    socket1 = dp::socket(ios);
    socket1 = std::move(socket2);
    socket1 = boost::asio::ip::udp::socket(ios);
#endif // defined(BOOST_ASIO_HAS_MOVE)

    // basic_io_object functions.

    io_service& ios_ref = socket1.get_io_service();
    (void)ios_ref;

    // basic_socket functions.

    dp::socket::lowest_layer_type& lowest_layer = socket1.lowest_layer();
    (void)lowest_layer;

    socket1.open(dp(af_inet, ipproto_udp));
    socket1.open(dp(af_inet, ipproto_udp), ec);

#if !defined(BOOST_ASIO_WINDOWS_RUNTIME)
    int native_socket2 = ::socket(af_inet, sock_dgram, 0);
    socket1.assign(dp(af_inet, ipproto_udp), native_socket2);
    int native_socket3 = ::socket(af_inet, sock_dgram, 0);
    socket1.assign(dp(af_inet, ipproto_udp), native_socket3, ec);
#endif // !defined(BOOST_ASIO_WINDOWS_RUNTIME)

    bool is_open = socket1.is_open();
    (void)is_open;

    socket1.close();
    socket1.close(ec);

    dp::socket::native_type native_socket4 = socket1.native();
    (void)native_socket4;

    socket1.cancel();
    socket1.cancel(ec);

    bool at_mark1 = socket1.at_mark();
    (void)at_mark1;
    bool at_mark2 = socket1.at_mark(ec);
    (void)at_mark2;

    std::size_t available1 = socket1.available();
    (void)available1;
    std::size_t available2 = socket1.available(ec);
    (void)available2;

    socket1.bind(dp::endpoint());
    socket1.bind(dp::endpoint(), ec);

    socket1.connect(dp::endpoint());
    socket1.connect(dp::endpoint(), ec);

    socket1.async_connect(dp::endpoint(), connect_handler);

    socket1.set_option(socket_option);
    socket1.set_option(socket_option, ec);

    socket1.get_option(socket_option);
    socket1.get_option(socket_option, ec);

    socket1.io_control(io_control_command);
    socket1.io_control(io_control_command, ec);

    dp::endpoint endpoint1 = socket1.local_endpoint();
    dp::endpoint endpoint2 = socket1.local_endpoint(ec);

    dp::endpoint endpoint3 = socket1.remote_endpoint();
    dp::endpoint endpoint4 = socket1.remote_endpoint(ec);

    socket1.shutdown(socket_base::shutdown_both);
    socket1.shutdown(socket_base::shutdown_both, ec);

    // basic_datagram_socket functions.

    socket1.send(buffer(mutable_char_buffer));
    socket1.send(buffer(const_char_buffer));
    socket1.send(null_buffers());
    socket1.send(buffer(mutable_char_buffer), in_flags);
    socket1.send(buffer(const_char_buffer), in_flags);
    socket1.send(null_buffers(), in_flags);
    socket1.send(buffer(mutable_char_buffer), in_flags, ec);
    socket1.send(buffer(const_char_buffer), in_flags, ec);
    socket1.send(null_buffers(), in_flags, ec);

    socket1.async_send(buffer(mutable_char_buffer), send_handler);
    socket1.async_send(buffer(const_char_buffer), send_handler);
    socket1.async_send(null_buffers(), send_handler);
    socket1.async_send(buffer(mutable_char_buffer), in_flags, send_handler);
    socket1.async_send(buffer(const_char_buffer), in_flags, send_handler);
    socket1.async_send(null_buffers(), in_flags, send_handler);

    socket1.send_to(buffer(mutable_char_buffer),
        dp::endpoint());
    socket1.send_to(buffer(const_char_buffer),
        dp::endpoint());
    socket1.send_to(null_buffers(),
        dp::endpoint());
    socket1.send_to(buffer(mutable_char_buffer),
        dp::endpoint(), in_flags);
    socket1.send_to(buffer(const_char_buffer),
        dp::endpoint(), in_flags);
    socket1.send_to(null_buffers(),
        dp::endpoint(), in_flags);
    socket1.send_to(buffer(mutable_char_buffer),
        dp::endpoint(), in_flags, ec);
    socket1.send_to(buffer(const_char_buffer),
        dp::endpoint(), in_flags, ec);
    socket1.send_to(null_buffers(),
        dp::endpoint(), in_flags, ec);

    socket1.async_send_to(buffer(mutable_char_buffer),
        dp::endpoint(), send_handler);
    socket1.async_send_to(buffer(const_char_buffer),
        dp::endpoint(), send_handler);
    socket1.async_send_to(null_buffers(),
        dp::endpoint(), send_handler);
    socket1.async_send_to(buffer(mutable_char_buffer),
        dp::endpoint(), in_flags, send_handler);
    socket1.async_send_to(buffer(const_char_buffer),
        dp::endpoint(), in_flags, send_handler);
    socket1.async_send_to(null_buffers(),
        dp::endpoint(), in_flags, send_handler);

    socket1.receive(buffer(mutable_char_buffer));
    socket1.receive(null_buffers());
    socket1.receive(buffer(mutable_char_buffer), in_flags);
    socket1.receive(null_buffers(), in_flags);
    socket1.receive(buffer(mutable_char_buffer), in_flags, ec);
    socket1.receive(null_buffers(), in_flags, ec);

    socket1.async_receive(buffer(mutable_char_buffer), receive_handler);
    socket1.async_receive(null_buffers(), receive_handler);
    socket1.async_receive(buffer(mutable_char_buffer), in_flags,
        receive_handler);
    socket1.async_receive(null_buffers(), in_flags, receive_handler);

    dp::endpoint endpoint;
    socket1.receive_from(buffer(mutable_char_buffer), endpoint);
    socket1.receive_from(null_buffers(), endpoint);
    socket1.receive_from(buffer(mutable_char_buffer), endpoint, in_flags);
    socket1.receive_from(null_buffers(), endpoint, in_flags);
    socket1.receive_from(buffer(mutable_char_buffer), endpoint, in_flags, ec);
    socket1.receive_from(null_buffers(), endpoint, in_flags, ec);

    socket1.async_receive_from(buffer(mutable_char_buffer),
        endpoint, receive_handler);
    socket1.async_receive_from(null_buffers(),
        endpoint, receive_handler);
    socket1.async_receive_from(buffer(mutable_char_buffer),
        endpoint, in_flags, receive_handler);
    socket1.async_receive_from(null_buffers(),
        endpoint, in_flags, receive_handler);
  }
  catch (std::exception&)
  {
  }
}

} // namespace generic_datagram_protocol_socket_compile

//------------------------------------------------------------------------------

BOOST_ASIO_TEST_SUITE
(
  "generic/datagram_protocol",
  BOOST_ASIO_TEST_CASE(generic_datagram_protocol_socket_compile::test)
)
