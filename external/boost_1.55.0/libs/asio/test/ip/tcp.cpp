//
// tcp.cpp
// ~~~~~~~
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

// Enable cancel() support on Windows.
#define BOOST_ASIO_ENABLE_CANCELIO 1

// Test that header file is self-contained.
#include <boost/asio/ip/tcp.hpp>

#include <cstring>
#include <boost/asio/io_service.hpp>
#include <boost/asio/read.hpp>
#include <boost/asio/write.hpp>
#include "../unit_test.hpp"
#include "../archetypes/gettable_socket_option.hpp"
#include "../archetypes/async_result.hpp"
#include "../archetypes/io_control_command.hpp"
#include "../archetypes/settable_socket_option.hpp"

#if defined(BOOST_ASIO_HAS_BOOST_ARRAY)
# include <boost/array.hpp>
#else // defined(BOOST_ASIO_HAS_BOOST_ARRAY)
# include <array>
#endif // defined(BOOST_ASIO_HAS_BOOST_ARRAY)

#if defined(BOOST_ASIO_HAS_BOOST_BIND)
# include <boost/bind.hpp>
#else // defined(BOOST_ASIO_HAS_BOOST_BIND)
# include <functional>
#endif // defined(BOOST_ASIO_HAS_BOOST_BIND)

//------------------------------------------------------------------------------

// ip_tcp_compile test
// ~~~~~~~~~~~~~~~~~~~
// The following test checks that all nested classes, enums and constants in
// ip::tcp compile and link correctly. Runtime failures are ignored.

namespace ip_tcp_compile {

void test()
{
  using namespace boost::asio;
  namespace ip = boost::asio::ip;

  try
  {
    io_service ios;
    ip::tcp::socket sock(ios);

    // no_delay class.

    ip::tcp::no_delay no_delay1(true);
    sock.set_option(no_delay1);
    ip::tcp::no_delay no_delay2;
    sock.get_option(no_delay2);
    no_delay1 = true;
    (void)static_cast<bool>(no_delay1);
    (void)static_cast<bool>(!no_delay1);
    (void)static_cast<bool>(no_delay1.value());
  }
  catch (std::exception&)
  {
  }
}

} // namespace ip_tcp_compile

//------------------------------------------------------------------------------

// ip_tcp_runtime test
// ~~~~~~~~~~~~~~~~~~~
// The following test checks the runtime operation of the ip::tcp class.

namespace ip_tcp_runtime {

void test()
{
  using namespace boost::asio;
  namespace ip = boost::asio::ip;

  io_service ios;
  ip::tcp::socket sock(ios, ip::tcp::v4());
  boost::system::error_code ec;

  // no_delay class.

  ip::tcp::no_delay no_delay1(true);
  BOOST_ASIO_CHECK(no_delay1.value());
  BOOST_ASIO_CHECK(static_cast<bool>(no_delay1));
  BOOST_ASIO_CHECK(!!no_delay1);
  sock.set_option(no_delay1, ec);
  BOOST_ASIO_CHECK(!ec);

  ip::tcp::no_delay no_delay2;
  sock.get_option(no_delay2, ec);
  BOOST_ASIO_CHECK(!ec);
  BOOST_ASIO_CHECK(no_delay2.value());
  BOOST_ASIO_CHECK(static_cast<bool>(no_delay2));
  BOOST_ASIO_CHECK(!!no_delay2);

  ip::tcp::no_delay no_delay3(false);
  BOOST_ASIO_CHECK(!no_delay3.value());
  BOOST_ASIO_CHECK(!static_cast<bool>(no_delay3));
  BOOST_ASIO_CHECK(!no_delay3);
  sock.set_option(no_delay3, ec);
  BOOST_ASIO_CHECK(!ec);

  ip::tcp::no_delay no_delay4;
  sock.get_option(no_delay4, ec);
  BOOST_ASIO_CHECK(!ec);
  BOOST_ASIO_CHECK(!no_delay4.value());
  BOOST_ASIO_CHECK(!static_cast<bool>(no_delay4));
  BOOST_ASIO_CHECK(!no_delay4);
}

} // namespace ip_tcp_runtime

//------------------------------------------------------------------------------

// ip_tcp_socket_compile test
// ~~~~~~~~~~~~~~~~~~~~~~~~~~
// The following test checks that all public member functions on the class
// ip::tcp::socket compile and link correctly. Runtime failures are ignored.

namespace ip_tcp_socket_compile {

void connect_handler(const boost::system::error_code&)
{
}

void send_handler(const boost::system::error_code&, std::size_t)
{
}

void receive_handler(const boost::system::error_code&, std::size_t)
{
}

void write_some_handler(const boost::system::error_code&, std::size_t)
{
}

void read_some_handler(const boost::system::error_code&, std::size_t)
{
}

void test()
{
#if defined(BOOST_ASIO_HAS_BOOST_ARRAY)
  using boost::array;
#else // defined(BOOST_ASIO_HAS_BOOST_ARRAY)
  using std::array;
#endif // defined(BOOST_ASIO_HAS_BOOST_ARRAY)

  using namespace boost::asio;
  namespace ip = boost::asio::ip;

  try
  {
    io_service ios;
    char mutable_char_buffer[128] = "";
    const char const_char_buffer[128] = "";
    array<boost::asio::mutable_buffer, 2> mutable_buffers = {{
        boost::asio::buffer(mutable_char_buffer, 10),
        boost::asio::buffer(mutable_char_buffer + 10, 10) }};
    array<boost::asio::const_buffer, 2> const_buffers = {{
        boost::asio::buffer(const_char_buffer, 10),
        boost::asio::buffer(const_char_buffer + 10, 10) }};
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

    // basic_stream_socket constructors.

    ip::tcp::socket socket1(ios);
    ip::tcp::socket socket2(ios, ip::tcp::v4());
    ip::tcp::socket socket3(ios, ip::tcp::v6());
    ip::tcp::socket socket4(ios, ip::tcp::endpoint(ip::tcp::v4(), 0));
    ip::tcp::socket socket5(ios, ip::tcp::endpoint(ip::tcp::v6(), 0));
#if !defined(BOOST_ASIO_WINDOWS_RUNTIME)
    int native_socket1 = ::socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    ip::tcp::socket socket6(ios, ip::tcp::v4(), native_socket1);
#endif // !defined(BOOST_ASIO_WINDOWS_RUNTIME)

#if defined(BOOST_ASIO_HAS_MOVE)
    ip::tcp::socket socket7(std::move(socket5));
#endif // defined(BOOST_ASIO_HAS_MOVE)

    // basic_stream_socket operators.

#if defined(BOOST_ASIO_HAS_MOVE)
    socket1 = ip::tcp::socket(ios);
    socket1 = std::move(socket2);
#endif // defined(BOOST_ASIO_HAS_MOVE)

    // basic_io_object functions.

    io_service& ios_ref = socket1.get_io_service();
    (void)ios_ref;

    // basic_socket functions.

    ip::tcp::socket::lowest_layer_type& lowest_layer = socket1.lowest_layer();
    (void)lowest_layer;

    const ip::tcp::socket& socket8 = socket1;
    const ip::tcp::socket::lowest_layer_type& lowest_layer2
      = socket8.lowest_layer();
    (void)lowest_layer2;

    socket1.open(ip::tcp::v4());
    socket1.open(ip::tcp::v6());
    socket1.open(ip::tcp::v4(), ec);
    socket1.open(ip::tcp::v6(), ec);

#if !defined(BOOST_ASIO_WINDOWS_RUNTIME)
    int native_socket2 = ::socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    socket1.assign(ip::tcp::v4(), native_socket2);
    int native_socket3 = ::socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    socket1.assign(ip::tcp::v4(), native_socket3, ec);
#endif // !defined(BOOST_ASIO_WINDOWS_RUNTIME)

    bool is_open = socket1.is_open();
    (void)is_open;

    socket1.close();
    socket1.close(ec);

    ip::tcp::socket::native_type native_socket4 = socket1.native();
    (void)native_socket4;

    ip::tcp::socket::native_handle_type native_socket5
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

    socket1.bind(ip::tcp::endpoint(ip::tcp::v4(), 0));
    socket1.bind(ip::tcp::endpoint(ip::tcp::v6(), 0));
    socket1.bind(ip::tcp::endpoint(ip::tcp::v4(), 0), ec);
    socket1.bind(ip::tcp::endpoint(ip::tcp::v6(), 0), ec);

    socket1.connect(ip::tcp::endpoint(ip::tcp::v4(), 0));
    socket1.connect(ip::tcp::endpoint(ip::tcp::v6(), 0));
    socket1.connect(ip::tcp::endpoint(ip::tcp::v4(), 0), ec);
    socket1.connect(ip::tcp::endpoint(ip::tcp::v6(), 0), ec);

    socket1.async_connect(ip::tcp::endpoint(ip::tcp::v4(), 0),
        &connect_handler);
    socket1.async_connect(ip::tcp::endpoint(ip::tcp::v6(), 0),
        &connect_handler);
    int i1 = socket1.async_connect(ip::tcp::endpoint(ip::tcp::v4(), 0), lazy);
    (void)i1;
    int i2 = socket1.async_connect(ip::tcp::endpoint(ip::tcp::v6(), 0), lazy);
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

    ip::tcp::endpoint endpoint1 = socket1.local_endpoint();
    ip::tcp::endpoint endpoint2 = socket1.local_endpoint(ec);

    ip::tcp::endpoint endpoint3 = socket1.remote_endpoint();
    ip::tcp::endpoint endpoint4 = socket1.remote_endpoint(ec);

    socket1.shutdown(socket_base::shutdown_both);
    socket1.shutdown(socket_base::shutdown_both, ec);

    // basic_stream_socket functions.

    socket1.send(buffer(mutable_char_buffer));
    socket1.send(buffer(const_char_buffer));
    socket1.send(mutable_buffers);
    socket1.send(const_buffers);
    socket1.send(null_buffers());
    socket1.send(buffer(mutable_char_buffer), in_flags);
    socket1.send(buffer(const_char_buffer), in_flags);
    socket1.send(mutable_buffers, in_flags);
    socket1.send(const_buffers, in_flags);
    socket1.send(null_buffers(), in_flags);
    socket1.send(buffer(mutable_char_buffer), in_flags, ec);
    socket1.send(buffer(const_char_buffer), in_flags, ec);
    socket1.send(mutable_buffers, in_flags, ec);
    socket1.send(const_buffers, in_flags, ec);
    socket1.send(null_buffers(), in_flags, ec);

    socket1.async_send(buffer(mutable_char_buffer), &send_handler);
    socket1.async_send(buffer(const_char_buffer), &send_handler);
    socket1.async_send(mutable_buffers, &send_handler);
    socket1.async_send(const_buffers, &send_handler);
    socket1.async_send(null_buffers(), &send_handler);
    socket1.async_send(buffer(mutable_char_buffer), in_flags, &send_handler);
    socket1.async_send(buffer(const_char_buffer), in_flags, &send_handler);
    socket1.async_send(mutable_buffers, in_flags, &send_handler);
    socket1.async_send(const_buffers, in_flags, &send_handler);
    socket1.async_send(null_buffers(), in_flags, &send_handler);
    int i3 = socket1.async_send(buffer(mutable_char_buffer), lazy);
    (void)i3;
    int i4 = socket1.async_send(buffer(const_char_buffer), lazy);
    (void)i4;
    int i5 = socket1.async_send(mutable_buffers, lazy);
    (void)i5;
    int i6 = socket1.async_send(const_buffers, lazy);
    (void)i6;
    int i7 = socket1.async_send(null_buffers(), lazy);
    (void)i7;
    int i8 = socket1.async_send(buffer(mutable_char_buffer), in_flags, lazy);
    (void)i8;
    int i9 = socket1.async_send(buffer(const_char_buffer), in_flags, lazy);
    (void)i9;
    int i10 = socket1.async_send(mutable_buffers, in_flags, lazy);
    (void)i10;
    int i11 = socket1.async_send(const_buffers, in_flags, lazy);
    (void)i11;
    int i12 = socket1.async_send(null_buffers(), in_flags, lazy);
    (void)i12;

    socket1.receive(buffer(mutable_char_buffer));
    socket1.receive(mutable_buffers);
    socket1.receive(null_buffers());
    socket1.receive(buffer(mutable_char_buffer), in_flags);
    socket1.receive(mutable_buffers, in_flags);
    socket1.receive(null_buffers(), in_flags);
    socket1.receive(buffer(mutable_char_buffer), in_flags, ec);
    socket1.receive(mutable_buffers, in_flags, ec);
    socket1.receive(null_buffers(), in_flags, ec);

    socket1.async_receive(buffer(mutable_char_buffer), &receive_handler);
    socket1.async_receive(mutable_buffers, &receive_handler);
    socket1.async_receive(null_buffers(), &receive_handler);
    socket1.async_receive(buffer(mutable_char_buffer), in_flags,
        &receive_handler);
    socket1.async_receive(mutable_buffers, in_flags, &receive_handler);
    socket1.async_receive(null_buffers(), in_flags, &receive_handler);
    int i13 = socket1.async_receive(buffer(mutable_char_buffer), lazy);
    (void)i13;
    int i14 = socket1.async_receive(mutable_buffers, lazy);
    (void)i14;
    int i15 = socket1.async_receive(null_buffers(), lazy);
    (void)i15;
    int i16 = socket1.async_receive(buffer(mutable_char_buffer), in_flags,
        lazy);
    (void)i16;
    int i17 = socket1.async_receive(mutable_buffers, in_flags, lazy);
    (void)i17;
    int i18 = socket1.async_receive(null_buffers(), in_flags, lazy);
    (void)i18;

    socket1.write_some(buffer(mutable_char_buffer));
    socket1.write_some(buffer(const_char_buffer));
    socket1.write_some(mutable_buffers);
    socket1.write_some(const_buffers);
    socket1.write_some(null_buffers());
    socket1.write_some(buffer(mutable_char_buffer), ec);
    socket1.write_some(buffer(const_char_buffer), ec);
    socket1.write_some(mutable_buffers, ec);
    socket1.write_some(const_buffers, ec);
    socket1.write_some(null_buffers(), ec);

    socket1.async_write_some(buffer(mutable_char_buffer), &write_some_handler);
    socket1.async_write_some(buffer(const_char_buffer), &write_some_handler);
    socket1.async_write_some(mutable_buffers, &write_some_handler);
    socket1.async_write_some(const_buffers, &write_some_handler);
    socket1.async_write_some(null_buffers(), &write_some_handler);
    int i19 = socket1.async_write_some(buffer(mutable_char_buffer), lazy);
    (void)i19;
    int i20 = socket1.async_write_some(buffer(const_char_buffer), lazy);
    (void)i20;
    int i21 = socket1.async_write_some(mutable_buffers, lazy);
    (void)i21;
    int i22 = socket1.async_write_some(const_buffers, lazy);
    (void)i22;
    int i23 = socket1.async_write_some(null_buffers(), lazy);
    (void)i23;

    socket1.read_some(buffer(mutable_char_buffer));
    socket1.read_some(mutable_buffers);
    socket1.read_some(null_buffers());
    socket1.read_some(buffer(mutable_char_buffer), ec);
    socket1.read_some(mutable_buffers, ec);
    socket1.read_some(null_buffers(), ec);

    socket1.async_read_some(buffer(mutable_char_buffer), &read_some_handler);
    socket1.async_read_some(mutable_buffers, &read_some_handler);
    socket1.async_read_some(null_buffers(), &read_some_handler);
    int i24 = socket1.async_read_some(buffer(mutable_char_buffer), lazy);
    (void)i24;
    int i25 = socket1.async_read_some(mutable_buffers, lazy);
    (void)i25;
    int i26 = socket1.async_read_some(null_buffers(), lazy);
    (void)i26;
  }
  catch (std::exception&)
  {
  }
}

} // namespace ip_tcp_socket_compile

//------------------------------------------------------------------------------

// ip_tcp_socket_runtime test
// ~~~~~~~~~~~~~~~~~~~~~~~~~~
// The following test checks the runtime operation of the ip::tcp::socket class.

namespace ip_tcp_socket_runtime {

static const char write_data[]
  = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz";

void handle_read_noop(const boost::system::error_code& err,
    size_t bytes_transferred, bool* called)
{
  *called = true;
  BOOST_ASIO_CHECK(!err);
  BOOST_ASIO_CHECK(bytes_transferred == 0);
}

void handle_write_noop(const boost::system::error_code& err,
    size_t bytes_transferred, bool* called)
{
  *called = true;
  BOOST_ASIO_CHECK(!err);
  BOOST_ASIO_CHECK(bytes_transferred == 0);
}

void handle_read(const boost::system::error_code& err,
    size_t bytes_transferred, bool* called)
{
  *called = true;
  BOOST_ASIO_CHECK(!err);
  BOOST_ASIO_CHECK(bytes_transferred == sizeof(write_data));
}

void handle_write(const boost::system::error_code& err,
    size_t bytes_transferred, bool* called)
{
  *called = true;
  BOOST_ASIO_CHECK(!err);
  BOOST_ASIO_CHECK(bytes_transferred == sizeof(write_data));
}

void handle_read_cancel(const boost::system::error_code& err,
    size_t bytes_transferred, bool* called)
{
  *called = true;
  BOOST_ASIO_CHECK(err == boost::asio::error::operation_aborted);
  BOOST_ASIO_CHECK(bytes_transferred == 0);
}

void handle_read_eof(const boost::system::error_code& err,
    size_t bytes_transferred, bool* called)
{
  *called = true;
  BOOST_ASIO_CHECK(err == boost::asio::error::eof);
  BOOST_ASIO_CHECK(bytes_transferred == 0);
}

void test()
{
  using namespace std; // For memcmp.
  using namespace boost::asio;
  namespace ip = boost::asio::ip;

#if defined(BOOST_ASIO_HAS_BOOST_BIND)
  namespace bindns = boost;
#else // defined(BOOST_ASIO_HAS_BOOST_BIND)
  namespace bindns = std;
  using std::placeholders::_1;
  using std::placeholders::_2;
#endif // defined(BOOST_ASIO_HAS_BOOST_BIND)

  io_service ios;

  ip::tcp::acceptor acceptor(ios, ip::tcp::endpoint(ip::tcp::v4(), 0));
  ip::tcp::endpoint server_endpoint = acceptor.local_endpoint();
  server_endpoint.address(ip::address_v4::loopback());

  ip::tcp::socket client_side_socket(ios);
  ip::tcp::socket server_side_socket(ios);

  client_side_socket.connect(server_endpoint);
  acceptor.accept(server_side_socket);

  // No-op read.

  bool read_noop_completed = false;
  client_side_socket.async_read_some(
      boost::asio::mutable_buffers_1(0, 0),
      bindns::bind(handle_read_noop,
        _1, _2, &read_noop_completed));

  ios.run();
  BOOST_ASIO_CHECK(read_noop_completed);

  // No-op write.

  bool write_noop_completed = false;
  client_side_socket.async_write_some(
      boost::asio::const_buffers_1(0, 0),
      bindns::bind(handle_write_noop,
        _1, _2, &write_noop_completed));

  ios.reset();
  ios.run();
  BOOST_ASIO_CHECK(write_noop_completed);

  // Read and write to transfer data.

  char read_buffer[sizeof(write_data)];
  bool read_completed = false;
  boost::asio::async_read(client_side_socket,
      boost::asio::buffer(read_buffer),
      bindns::bind(handle_read,
        _1, _2, &read_completed));

  bool write_completed = false;
  boost::asio::async_write(server_side_socket,
      boost::asio::buffer(write_data),
      bindns::bind(handle_write,
        _1, _2, &write_completed));

  ios.reset();
  ios.run();
  BOOST_ASIO_CHECK(read_completed);
  BOOST_ASIO_CHECK(write_completed);
  BOOST_ASIO_CHECK(memcmp(read_buffer, write_data, sizeof(write_data)) == 0);

  // Cancelled read.

  bool read_cancel_completed = false;
  boost::asio::async_read(server_side_socket,
      boost::asio::buffer(read_buffer),
      bindns::bind(handle_read_cancel,
        _1, _2, &read_cancel_completed));

  ios.reset();
  ios.poll();
  BOOST_ASIO_CHECK(!read_cancel_completed);

  server_side_socket.cancel();

  ios.reset();
  ios.run();
  BOOST_ASIO_CHECK(read_cancel_completed);

  // A read when the peer closes socket should fail with eof.

  bool read_eof_completed = false;
  boost::asio::async_read(client_side_socket,
      boost::asio::buffer(read_buffer),
      bindns::bind(handle_read_eof,
        _1, _2, &read_eof_completed));

  server_side_socket.close();

  ios.reset();
  ios.run();
  BOOST_ASIO_CHECK(read_eof_completed);
}

} // namespace ip_tcp_socket_runtime

//------------------------------------------------------------------------------

// ip_tcp_acceptor_compile test
// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~
// The following test checks that all public member functions on the class
// ip::tcp::acceptor compile and link correctly. Runtime failures are ignored.

namespace ip_tcp_acceptor_compile {

void accept_handler(const boost::system::error_code&)
{
}

void test()
{
  using namespace boost::asio;
  namespace ip = boost::asio::ip;

  try
  {
    io_service ios;
    ip::tcp::socket peer_socket(ios);
    ip::tcp::endpoint peer_endpoint;
    archetypes::settable_socket_option<void> settable_socket_option1;
    archetypes::settable_socket_option<int> settable_socket_option2;
    archetypes::settable_socket_option<double> settable_socket_option3;
    archetypes::gettable_socket_option<void> gettable_socket_option1;
    archetypes::gettable_socket_option<int> gettable_socket_option2;
    archetypes::gettable_socket_option<double> gettable_socket_option3;
    archetypes::io_control_command io_control_command;
    archetypes::lazy_handler lazy;
    boost::system::error_code ec;

    // basic_socket_acceptor constructors.

    ip::tcp::acceptor acceptor1(ios);
    ip::tcp::acceptor acceptor2(ios, ip::tcp::v4());
    ip::tcp::acceptor acceptor3(ios, ip::tcp::v6());
    ip::tcp::acceptor acceptor4(ios, ip::tcp::endpoint(ip::tcp::v4(), 0));
    ip::tcp::acceptor acceptor5(ios, ip::tcp::endpoint(ip::tcp::v6(), 0));
#if !defined(BOOST_ASIO_WINDOWS_RUNTIME)
    int native_acceptor1 = ::socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    ip::tcp::acceptor acceptor6(ios, ip::tcp::v4(), native_acceptor1);
#endif // !defined(BOOST_ASIO_WINDOWS_RUNTIME)

#if defined(BOOST_ASIO_HAS_MOVE)
    ip::tcp::acceptor acceptor7(std::move(acceptor5));
#endif // defined(BOOST_ASIO_HAS_MOVE)

    // basic_socket_acceptor operators.

#if defined(BOOST_ASIO_HAS_MOVE)
    acceptor1 = ip::tcp::acceptor(ios);
    acceptor1 = std::move(acceptor2);
#endif // defined(BOOST_ASIO_HAS_MOVE)

    // basic_io_object functions.

    io_service& ios_ref = acceptor1.get_io_service();
    (void)ios_ref;

    // basic_socket_acceptor functions.

    acceptor1.open(ip::tcp::v4());
    acceptor1.open(ip::tcp::v6());
    acceptor1.open(ip::tcp::v4(), ec);
    acceptor1.open(ip::tcp::v6(), ec);

#if !defined(BOOST_ASIO_WINDOWS_RUNTIME)
    int native_acceptor2 = ::socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    acceptor1.assign(ip::tcp::v4(), native_acceptor2);
    int native_acceptor3 = ::socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    acceptor1.assign(ip::tcp::v4(), native_acceptor3, ec);
#endif // !defined(BOOST_ASIO_WINDOWS_RUNTIME)

    bool is_open = acceptor1.is_open();
    (void)is_open;

    acceptor1.close();
    acceptor1.close(ec);

    ip::tcp::acceptor::native_type native_acceptor4 = acceptor1.native();
    (void)native_acceptor4;

    ip::tcp::acceptor::native_handle_type native_acceptor5
      = acceptor1.native_handle();
    (void)native_acceptor5;

    acceptor1.cancel();
    acceptor1.cancel(ec);

    acceptor1.bind(ip::tcp::endpoint(ip::tcp::v4(), 0));
    acceptor1.bind(ip::tcp::endpoint(ip::tcp::v6(), 0));
    acceptor1.bind(ip::tcp::endpoint(ip::tcp::v4(), 0), ec);
    acceptor1.bind(ip::tcp::endpoint(ip::tcp::v6(), 0), ec);

    acceptor1.set_option(settable_socket_option1);
    acceptor1.set_option(settable_socket_option1, ec);
    acceptor1.set_option(settable_socket_option2);
    acceptor1.set_option(settable_socket_option2, ec);
    acceptor1.set_option(settable_socket_option3);
    acceptor1.set_option(settable_socket_option3, ec);

    acceptor1.get_option(gettable_socket_option1);
    acceptor1.get_option(gettable_socket_option1, ec);
    acceptor1.get_option(gettable_socket_option2);
    acceptor1.get_option(gettable_socket_option2, ec);
    acceptor1.get_option(gettable_socket_option3);
    acceptor1.get_option(gettable_socket_option3, ec);

    acceptor1.io_control(io_control_command);
    acceptor1.io_control(io_control_command, ec);

    bool non_blocking1 = acceptor1.non_blocking();
    (void)non_blocking1;
    acceptor1.non_blocking(true);
    acceptor1.non_blocking(false, ec);

    bool non_blocking2 = acceptor1.native_non_blocking();
    (void)non_blocking2;
    acceptor1.native_non_blocking(true);
    acceptor1.native_non_blocking(false, ec);

    ip::tcp::endpoint endpoint1 = acceptor1.local_endpoint();
    ip::tcp::endpoint endpoint2 = acceptor1.local_endpoint(ec);

    acceptor1.accept(peer_socket);
    acceptor1.accept(peer_socket, ec);
    acceptor1.accept(peer_socket, peer_endpoint);
    acceptor1.accept(peer_socket, peer_endpoint, ec);

    acceptor1.async_accept(peer_socket, &accept_handler);
    acceptor1.async_accept(peer_socket, peer_endpoint, &accept_handler);
    int i1 = acceptor1.async_accept(peer_socket, lazy);
    (void)i1;
    int i2 = acceptor1.async_accept(peer_socket, peer_endpoint, lazy);
    (void)i2;
  }
  catch (std::exception&)
  {
  }
}

} // namespace ip_tcp_acceptor_compile

//------------------------------------------------------------------------------

// ip_tcp_acceptor_runtime test
// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~
// The following test checks the runtime operation of the ip::tcp::acceptor
// class.

namespace ip_tcp_acceptor_runtime {

void handle_accept(const boost::system::error_code& err)
{
  BOOST_ASIO_CHECK(!err);
}

void handle_connect(const boost::system::error_code& err)
{
  BOOST_ASIO_CHECK(!err);
}

void test()
{
  using namespace boost::asio;
  namespace ip = boost::asio::ip;

  io_service ios;

  ip::tcp::acceptor acceptor(ios, ip::tcp::endpoint(ip::tcp::v4(), 0));
  ip::tcp::endpoint server_endpoint = acceptor.local_endpoint();
  server_endpoint.address(ip::address_v4::loopback());

  ip::tcp::socket client_side_socket(ios);
  ip::tcp::socket server_side_socket(ios);

  client_side_socket.connect(server_endpoint);
  acceptor.accept(server_side_socket);

  client_side_socket.close();
  server_side_socket.close();

  client_side_socket.connect(server_endpoint);
  ip::tcp::endpoint client_endpoint;
  acceptor.accept(server_side_socket, client_endpoint);

  ip::tcp::acceptor::non_blocking_io command(false);
  acceptor.io_control(command);

  ip::tcp::endpoint client_side_local_endpoint
    = client_side_socket.local_endpoint();
  BOOST_ASIO_CHECK(client_side_local_endpoint.port() == client_endpoint.port());

  ip::tcp::endpoint server_side_remote_endpoint
    = server_side_socket.remote_endpoint();
  BOOST_ASIO_CHECK(server_side_remote_endpoint.port()
      == client_endpoint.port());

  client_side_socket.close();
  server_side_socket.close();

  acceptor.async_accept(server_side_socket, &handle_accept);
  client_side_socket.async_connect(server_endpoint, &handle_connect);

  ios.run();

  client_side_socket.close();
  server_side_socket.close();

  acceptor.async_accept(server_side_socket, client_endpoint, &handle_accept);
  client_side_socket.async_connect(server_endpoint, &handle_connect);

  ios.reset();
  ios.run();

  client_side_local_endpoint = client_side_socket.local_endpoint();
  BOOST_ASIO_CHECK(client_side_local_endpoint.port() == client_endpoint.port());

  server_side_remote_endpoint = server_side_socket.remote_endpoint();
  BOOST_ASIO_CHECK(server_side_remote_endpoint.port()
      == client_endpoint.port());
}

} // namespace ip_tcp_acceptor_runtime

//------------------------------------------------------------------------------

// ip_tcp_resolver_compile test
// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~
// The following test checks that all public member functions on the class
// ip::tcp::resolver compile and link correctly. Runtime failures are ignored.

namespace ip_tcp_resolver_compile {

void resolve_handler(const boost::system::error_code&,
    boost::asio::ip::tcp::resolver::iterator)
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
    ip::tcp::resolver::query q(ip::tcp::v4(), "localhost", "0");
    ip::tcp::endpoint e(ip::address_v4::loopback(), 0);

    // basic_resolver constructors.

    ip::tcp::resolver resolver(ios);

    // basic_io_object functions.

    io_service& ios_ref = resolver.get_io_service();
    (void)ios_ref;

    // basic_resolver functions.

    resolver.cancel();

    ip::tcp::resolver::iterator iter1 = resolver.resolve(q);
    (void)iter1;

    ip::tcp::resolver::iterator iter2 = resolver.resolve(q, ec);
    (void)iter2;

    ip::tcp::resolver::iterator iter3 = resolver.resolve(e);
    (void)iter3;

    ip::tcp::resolver::iterator iter4 = resolver.resolve(e, ec);
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

} // namespace ip_tcp_resolver_compile

//------------------------------------------------------------------------------

BOOST_ASIO_TEST_SUITE
(
  "ip/tcp",
  BOOST_ASIO_TEST_CASE(ip_tcp_compile::test)
  BOOST_ASIO_TEST_CASE(ip_tcp_runtime::test)
  BOOST_ASIO_TEST_CASE(ip_tcp_socket_compile::test)
  BOOST_ASIO_TEST_CASE(ip_tcp_socket_runtime::test)
  BOOST_ASIO_TEST_CASE(ip_tcp_acceptor_compile::test)
  BOOST_ASIO_TEST_CASE(ip_tcp_acceptor_runtime::test)
  BOOST_ASIO_TEST_CASE(ip_tcp_resolver_compile::test)
)
