//
// icmp.cpp
// ~~~~~~~~
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
#include <boost/asio/ip/icmp.hpp>

#include <cstring>
#include <boost/asio/io_service.hpp>
#include <boost/asio/placeholders.hpp>
#include "../unit_test.hpp"
#include "../archetypes/gettable_socket_option.hpp"
#include "../archetypes/async_result.hpp"
#include "../archetypes/io_control_command.hpp"
#include "../archetypes/settable_socket_option.hpp"

//------------------------------------------------------------------------------

// ip_icmp_socket_compile test
// ~~~~~~~~~~~~~~~~~~~~~~~~~~
// The following test checks that all public member functions on the class
// ip::icmp::socket compile and link correctly. Runtime failures are ignored.

namespace ip_icmp_socket_compile {

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
  namespace ip = boost::asio::ip;

  try
  {
    io_service ios;
    char mutable_char_buffer[128] = "";
    const char const_char_buffer[128] = "";
    socket_base::message_flags in_flags = 0;
    archetypes::settable_socket_option<void> settable_socket_option1;
    archetypes::settable_socket_option<int> settable_socket_option2;
    archetypes::settable_socket_option<double> settable_socket_option3;
    archetypes::gettable_socket_option<void> gettable_socket_option1;
    archetypes::gettable_socket_option<int> gettable_socket_option2;
    archetypes::gettable_socket_option<double> gettable_socket_option3;
    archetypes::io_control_command io_control_command;
    archetypes::lazy_handler lazy;
    boost::system::error_code ec;

    // basic_datagram_socket constructors.

    ip::icmp::socket socket1(ios);
    ip::icmp::socket socket2(ios, ip::icmp::v4());
    ip::icmp::socket socket3(ios, ip::icmp::v6());
    ip::icmp::socket socket4(ios, ip::icmp::endpoint(ip::icmp::v4(), 0));
    ip::icmp::socket socket5(ios, ip::icmp::endpoint(ip::icmp::v6(), 0));
#if !defined(BOOST_ASIO_WINDOWS_RUNTIME)
    int native_socket1 = ::socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
    ip::icmp::socket socket6(ios, ip::icmp::v4(), native_socket1);
#endif // !defined(BOOST_ASIO_WINDOWS_RUNTIME)

#if defined(BOOST_ASIO_HAS_MOVE)
    ip::icmp::socket socket7(std::move(socket6));
#endif // defined(BOOST_ASIO_HAS_MOVE)

    // basic_datagram_socket operators.

#if defined(BOOST_ASIO_HAS_MOVE)
    socket1 = ip::icmp::socket(ios);
    socket1 = std::move(socket2);
#endif // defined(BOOST_ASIO_HAS_MOVE)

    // basic_io_object functions.

    io_service& ios_ref = socket1.get_io_service();
    (void)ios_ref;

    // basic_socket functions.

    ip::icmp::socket::lowest_layer_type& lowest_layer = socket1.lowest_layer();
    (void)lowest_layer;

    const ip::icmp::socket& socket8 = socket1;
    const ip::icmp::socket::lowest_layer_type& lowest_layer2
      = socket8.lowest_layer();
    (void)lowest_layer2;

    socket1.open(ip::icmp::v4());
    socket1.open(ip::icmp::v6());
    socket1.open(ip::icmp::v4(), ec);
    socket1.open(ip::icmp::v6(), ec);

#if !defined(BOOST_ASIO_WINDOWS_RUNTIME)
    int native_socket2 = ::socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
    socket1.assign(ip::icmp::v4(), native_socket2);
    int native_socket3 = ::socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
    socket1.assign(ip::icmp::v4(), native_socket3, ec);
#endif // !defined(BOOST_ASIO_WINDOWS_RUNTIME)

    bool is_open = socket1.is_open();
    (void)is_open;

    socket1.close();
    socket1.close(ec);

    ip::icmp::socket::native_type native_socket4 = socket1.native();
    (void)native_socket4;

    ip::icmp::socket::native_handle_type native_socket5
      = socket1.native_handle();
    (void)native_socket5;

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

    socket1.bind(ip::icmp::endpoint(ip::icmp::v4(), 0));
    socket1.bind(ip::icmp::endpoint(ip::icmp::v6(), 0));
    socket1.bind(ip::icmp::endpoint(ip::icmp::v4(), 0), ec);
    socket1.bind(ip::icmp::endpoint(ip::icmp::v6(), 0), ec);

    socket1.connect(ip::icmp::endpoint(ip::icmp::v4(), 0));
    socket1.connect(ip::icmp::endpoint(ip::icmp::v6(), 0));
    socket1.connect(ip::icmp::endpoint(ip::icmp::v4(), 0), ec);
    socket1.connect(ip::icmp::endpoint(ip::icmp::v6(), 0), ec);

    socket1.async_connect(ip::icmp::endpoint(ip::icmp::v4(), 0),
        &connect_handler);
    socket1.async_connect(ip::icmp::endpoint(ip::icmp::v6(), 0),
        &connect_handler);
    int i1 = socket1.async_connect(ip::icmp::endpoint(ip::icmp::v4(), 0), lazy);
    (void)i1;
    int i2 = socket1.async_connect(ip::icmp::endpoint(ip::icmp::v6(), 0), lazy);
    (void)i2;

    socket1.set_option(settable_socket_option1);
    socket1.set_option(settable_socket_option1, ec);
    socket1.set_option(settable_socket_option2);
    socket1.set_option(settable_socket_option2, ec);
    socket1.set_option(settable_socket_option3);
    socket1.set_option(settable_socket_option3, ec);

    socket1.get_option(gettable_socket_option1);
    socket1.get_option(gettable_socket_option1, ec);
    socket1.get_option(gettable_socket_option2);
    socket1.get_option(gettable_socket_option2, ec);
    socket1.get_option(gettable_socket_option3);
    socket1.get_option(gettable_socket_option3, ec);

    socket1.io_control(io_control_command);
    socket1.io_control(io_control_command, ec);

    bool non_blocking1 = socket1.non_blocking();
    (void)non_blocking1;
    socket1.non_blocking(true);
    socket1.non_blocking(false, ec);

    bool non_blocking2 = socket1.native_non_blocking();
    (void)non_blocking2;
    socket1.native_non_blocking(true);
    socket1.native_non_blocking(false, ec);

    ip::icmp::endpoint endpoint1 = socket1.local_endpoint();
    ip::icmp::endpoint endpoint2 = socket1.local_endpoint(ec);

    ip::icmp::endpoint endpoint3 = socket1.remote_endpoint();
    ip::icmp::endpoint endpoint4 = socket1.remote_endpoint(ec);

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

    socket1.async_send(buffer(mutable_char_buffer), &send_handler);
    socket1.async_send(buffer(const_char_buffer), &send_handler);
    socket1.async_send(null_buffers(), &send_handler);
    socket1.async_send(buffer(mutable_char_buffer), in_flags, &send_handler);
    socket1.async_send(buffer(const_char_buffer), in_flags, &send_handler);
    socket1.async_send(null_buffers(), in_flags, &send_handler);
    int i3 = socket1.async_send(buffer(mutable_char_buffer), lazy);
    (void)i3;
    int i4 = socket1.async_send(buffer(const_char_buffer), lazy);
    (void)i4;
    int i5 = socket1.async_send(null_buffers(), lazy);
    (void)i5;
    int i6 = socket1.async_send(buffer(mutable_char_buffer), in_flags, lazy);
    (void)i6;
    int i7 = socket1.async_send(buffer(const_char_buffer), in_flags, lazy);
    (void)i7;
    int i8 = socket1.async_send(null_buffers(), in_flags, lazy);
    (void)i8;

    socket1.send_to(buffer(mutable_char_buffer),
        ip::icmp::endpoint(ip::icmp::v4(), 0));
    socket1.send_to(buffer(mutable_char_buffer),
        ip::icmp::endpoint(ip::icmp::v6(), 0));
    socket1.send_to(buffer(const_char_buffer),
        ip::icmp::endpoint(ip::icmp::v4(), 0));
    socket1.send_to(buffer(const_char_buffer),
        ip::icmp::endpoint(ip::icmp::v6(), 0));
    socket1.send_to(null_buffers(),
        ip::icmp::endpoint(ip::icmp::v4(), 0));
    socket1.send_to(null_buffers(),
        ip::icmp::endpoint(ip::icmp::v6(), 0));
    socket1.send_to(buffer(mutable_char_buffer),
        ip::icmp::endpoint(ip::icmp::v4(), 0), in_flags);
    socket1.send_to(buffer(mutable_char_buffer),
        ip::icmp::endpoint(ip::icmp::v6(), 0), in_flags);
    socket1.send_to(buffer(const_char_buffer),
        ip::icmp::endpoint(ip::icmp::v4(), 0), in_flags);
    socket1.send_to(buffer(const_char_buffer),
        ip::icmp::endpoint(ip::icmp::v6(), 0), in_flags);
    socket1.send_to(null_buffers(),
        ip::icmp::endpoint(ip::icmp::v4(), 0), in_flags);
    socket1.send_to(null_buffers(),
        ip::icmp::endpoint(ip::icmp::v6(), 0), in_flags);
    socket1.send_to(buffer(mutable_char_buffer),
        ip::icmp::endpoint(ip::icmp::v4(), 0), in_flags, ec);
    socket1.send_to(buffer(mutable_char_buffer),
        ip::icmp::endpoint(ip::icmp::v6(), 0), in_flags, ec);
    socket1.send_to(buffer(const_char_buffer),
        ip::icmp::endpoint(ip::icmp::v4(), 0), in_flags, ec);
    socket1.send_to(buffer(const_char_buffer),
        ip::icmp::endpoint(ip::icmp::v6(), 0), in_flags, ec);
    socket1.send_to(null_buffers(),
        ip::icmp::endpoint(ip::icmp::v4(), 0), in_flags, ec);
    socket1.send_to(null_buffers(),
        ip::icmp::endpoint(ip::icmp::v6(), 0), in_flags, ec);

    socket1.async_send_to(buffer(mutable_char_buffer),
        ip::icmp::endpoint(ip::icmp::v4(), 0), &send_handler);
    socket1.async_send_to(buffer(mutable_char_buffer),
        ip::icmp::endpoint(ip::icmp::v6(), 0), &send_handler);
    socket1.async_send_to(buffer(const_char_buffer),
        ip::icmp::endpoint(ip::icmp::v4(), 0), &send_handler);
    socket1.async_send_to(buffer(const_char_buffer),
        ip::icmp::endpoint(ip::icmp::v6(), 0), &send_handler);
    socket1.async_send_to(null_buffers(),
        ip::icmp::endpoint(ip::icmp::v4(), 0), &send_handler);
    socket1.async_send_to(null_buffers(),
        ip::icmp::endpoint(ip::icmp::v6(), 0), &send_handler);
    socket1.async_send_to(buffer(mutable_char_buffer),
        ip::icmp::endpoint(ip::icmp::v4(), 0), in_flags, &send_handler);
    socket1.async_send_to(buffer(mutable_char_buffer),
        ip::icmp::endpoint(ip::icmp::v6(), 0), in_flags, &send_handler);
    socket1.async_send_to(buffer(const_char_buffer),
        ip::icmp::endpoint(ip::icmp::v4(), 0), in_flags, &send_handler);
    socket1.async_send_to(buffer(const_char_buffer),
        ip::icmp::endpoint(ip::icmp::v6(), 0), in_flags, &send_handler);
    socket1.async_send_to(null_buffers(),
        ip::icmp::endpoint(ip::icmp::v4(), 0), in_flags, &send_handler);
    socket1.async_send_to(null_buffers(),
        ip::icmp::endpoint(ip::icmp::v6(), 0), in_flags, &send_handler);
    int i9 = socket1.async_send_to(buffer(mutable_char_buffer),
        ip::icmp::endpoint(ip::icmp::v4(), 0), lazy);
    (void)i9;
    int i10 = socket1.async_send_to(buffer(mutable_char_buffer),
        ip::icmp::endpoint(ip::icmp::v6(), 0), lazy);
    (void)i10;
    int i11 = socket1.async_send_to(buffer(const_char_buffer),
        ip::icmp::endpoint(ip::icmp::v4(), 0), lazy);
    (void)i11;
    int i12 = socket1.async_send_to(buffer(const_char_buffer),
        ip::icmp::endpoint(ip::icmp::v6(), 0), lazy);
    (void)i12;
    int i13 = socket1.async_send_to(null_buffers(),
        ip::icmp::endpoint(ip::icmp::v4(), 0), lazy);
    (void)i13;
    int i14 = socket1.async_send_to(null_buffers(),
        ip::icmp::endpoint(ip::icmp::v6(), 0), lazy);
    (void)i14;
    int i15 = socket1.async_send_to(buffer(mutable_char_buffer),
        ip::icmp::endpoint(ip::icmp::v4(), 0), in_flags, lazy);
    (void)i15;
    int i16 = socket1.async_send_to(buffer(mutable_char_buffer),
        ip::icmp::endpoint(ip::icmp::v6(), 0), in_flags, lazy);
    (void)i16;
    int i17 = socket1.async_send_to(buffer(const_char_buffer),
        ip::icmp::endpoint(ip::icmp::v4(), 0), in_flags, lazy);
    (void)i17;
    int i18 = socket1.async_send_to(buffer(const_char_buffer),
        ip::icmp::endpoint(ip::icmp::v6(), 0), in_flags, lazy);
    (void)i18;
    int i19 = socket1.async_send_to(null_buffers(),
        ip::icmp::endpoint(ip::icmp::v4(), 0), in_flags, lazy);
    (void)i19;
    int i20 = socket1.async_send_to(null_buffers(),
        ip::icmp::endpoint(ip::icmp::v6(), 0), in_flags, lazy);
    (void)i20;

    socket1.receive(buffer(mutable_char_buffer));
    socket1.receive(null_buffers());
    socket1.receive(buffer(mutable_char_buffer), in_flags);
    socket1.receive(null_buffers(), in_flags);
    socket1.receive(buffer(mutable_char_buffer), in_flags, ec);
    socket1.receive(null_buffers(), in_flags, ec);

    socket1.async_receive(buffer(mutable_char_buffer), &receive_handler);
    socket1.async_receive(null_buffers(), &receive_handler);
    socket1.async_receive(buffer(mutable_char_buffer), in_flags,
        &receive_handler);
    socket1.async_receive(null_buffers(), in_flags, &receive_handler);
    int i21 = socket1.async_receive(buffer(mutable_char_buffer), lazy);
    (void)i21;
    int i22 = socket1.async_receive(null_buffers(), lazy);
    (void)i22;
    int i23 = socket1.async_receive(buffer(mutable_char_buffer),
        in_flags, lazy);
    (void)i23;
    int i24 = socket1.async_receive(null_buffers(), in_flags, lazy);
    (void)i24;

    ip::icmp::endpoint endpoint;
    socket1.receive_from(buffer(mutable_char_buffer), endpoint);
    socket1.receive_from(null_buffers(), endpoint);
    socket1.receive_from(buffer(mutable_char_buffer), endpoint, in_flags);
    socket1.receive_from(null_buffers(), endpoint, in_flags);
    socket1.receive_from(buffer(mutable_char_buffer), endpoint, in_flags, ec);
    socket1.receive_from(null_buffers(), endpoint, in_flags, ec);

    socket1.async_receive_from(buffer(mutable_char_buffer),
        endpoint, &receive_handler);
    socket1.async_receive_from(null_buffers(),
        endpoint, &receive_handler);
    socket1.async_receive_from(buffer(mutable_char_buffer),
        endpoint, in_flags, &receive_handler);
    socket1.async_receive_from(null_buffers(),
        endpoint, in_flags, &receive_handler);
    int i25 = socket1.async_receive_from(buffer(mutable_char_buffer),
        endpoint, lazy);
    (void)i25;
    int i26 = socket1.async_receive_from(null_buffers(),
        endpoint, lazy);
    (void)i26;
    int i27 = socket1.async_receive_from(buffer(mutable_char_buffer),
        endpoint, in_flags, lazy);
    (void)i27;
    int i28 = socket1.async_receive_from(null_buffers(),
        endpoint, in_flags, lazy);
    (void)i28;
  }
  catch (std::exception&)
  {
  }
}

} // namespace ip_icmp_socket_compile

//------------------------------------------------------------------------------

// ip_icmp_resolver_compile test
// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~
// The following test checks that all public member functions on the class
// ip::icmp::resolver compile and link correctly. Runtime failures are ignored.

namespace ip_icmp_resolver_compile {

void resolve_handler(const boost::system::error_code&,
    boost::asio::ip::icmp::resolver::iterator)
{
}

void test()
{
  using namespace boost::asio;
  namespace ip = boost::asio::ip;

  try
  {
    io_service ios;
    archetypes::lazy_handler lazy;
    boost::system::error_code ec;
    ip::icmp::resolver::query q(ip::icmp::v4(), "localhost", "0");
    ip::icmp::endpoint e(ip::address_v4::loopback(), 0);

    // basic_resolver constructors.

    ip::icmp::resolver resolver(ios);

    // basic_io_object functions.

    io_service& ios_ref = resolver.get_io_service();
    (void)ios_ref;

    // basic_resolver functions.

    resolver.cancel();

    ip::icmp::resolver::iterator iter1 = resolver.resolve(q);
    (void)iter1;

    ip::icmp::resolver::iterator iter2 = resolver.resolve(q, ec);
    (void)iter2;

    ip::icmp::resolver::iterator iter3 = resolver.resolve(e);
    (void)iter3;

    ip::icmp::resolver::iterator iter4 = resolver.resolve(e, ec);
    (void)iter4;

    resolver.async_resolve(q, &resolve_handler);
    int i1 = resolver.async_resolve(q, lazy);
    (void)i1;

    resolver.async_resolve(e, &resolve_handler);
    int i2 = resolver.async_resolve(e, lazy);
    (void)i2;
  }
  catch (std::exception&)
  {
  }
}

} // namespace ip_icmp_resolver_compile

//------------------------------------------------------------------------------

BOOST_ASIO_TEST_SUITE
(
  "ip/icmp",
  BOOST_ASIO_TEST_CASE(ip_icmp_socket_compile::test)
  BOOST_ASIO_TEST_CASE(ip_icmp_resolver_compile::test)
)
