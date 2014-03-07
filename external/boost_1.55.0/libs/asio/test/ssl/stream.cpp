//
// stream.cpp
// ~~~~~~~~~~
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
#include <boost/asio/ssl/stream.hpp>

#include <boost/asio.hpp>
#include <boost/asio/ssl.hpp>
#include "../archetypes/async_result.hpp"
#include "../unit_test.hpp"

//------------------------------------------------------------------------------

// ssl_stream_compile test
// ~~~~~~~~~~~~~~~~~~~~~~~
// The following test checks that all public member functions on the class
// ssl::stream::socket compile and link correctly. Runtime failures are ignored.

namespace ssl_stream_compile {

#if !defined(BOOST_ASIO_ENABLE_OLD_SSL)
bool verify_callback(bool, boost::asio::ssl::verify_context&)
{
  return false;
}
#endif // !defined(BOOST_ASIO_ENABLE_OLD_SSL)

void handshake_handler(const boost::system::error_code&)
{
}

void buffered_handshake_handler(const boost::system::error_code&, std::size_t)
{
}

void shutdown_handler(const boost::system::error_code&)
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
  using namespace boost::asio;
  namespace ip = boost::asio::ip;

  try
  {
    io_service ios;
    char mutable_char_buffer[128] = "";
    const char const_char_buffer[128] = "";
    boost::asio::ssl::context context(ios, boost::asio::ssl::context::sslv23);
    archetypes::lazy_handler lazy;
    boost::system::error_code ec;

    // ssl::stream constructors.

    ssl::stream<ip::tcp::socket> stream1(ios, context);
    ip::tcp::socket socket1(ios, ip::tcp::v4());
    ssl::stream<ip::tcp::socket&> stream2(socket1, context);

    // basic_io_object functions.

    io_service& ios_ref = stream1.get_io_service();
    (void)ios_ref;

    // ssl::stream functions.

#if !defined(BOOST_ASIO_ENABLE_OLD_SSL)
    SSL* ssl1 = stream1.native_handle();
    (void)ssl1;
#endif // !defined(BOOST_ASIO_ENABLE_OLD_SSL)

    SSL* ssl2 = stream1.impl()->ssl;
    (void)ssl2;

    ssl::stream<ip::tcp::socket>::lowest_layer_type& lowest_layer
      = stream1.lowest_layer();
    (void)lowest_layer;

    const ssl::stream<ip::tcp::socket>& stream3 = stream1;
    const ssl::stream<ip::tcp::socket>::lowest_layer_type& lowest_layer2
      = stream3.lowest_layer();
    (void)lowest_layer2;

#if !defined(BOOST_ASIO_ENABLE_OLD_SSL)
    stream1.set_verify_mode(ssl::verify_none);
    stream1.set_verify_mode(ssl::verify_none, ec);

    stream1.set_verify_depth(1);
    stream1.set_verify_depth(1, ec);

    stream1.set_verify_callback(verify_callback);
    stream1.set_verify_callback(verify_callback, ec);
#endif // !defined(BOOST_ASIO_ENABLE_OLD_SSL)

    stream1.handshake(ssl::stream_base::client);
    stream1.handshake(ssl::stream_base::server);
    stream1.handshake(ssl::stream_base::client, ec);
    stream1.handshake(ssl::stream_base::server, ec);

#if !defined(BOOST_ASIO_ENABLE_OLD_SSL)
    stream1.handshake(ssl::stream_base::client, buffer(mutable_char_buffer));
    stream1.handshake(ssl::stream_base::server, buffer(mutable_char_buffer));
    stream1.handshake(ssl::stream_base::client, buffer(const_char_buffer));
    stream1.handshake(ssl::stream_base::server, buffer(const_char_buffer));
    stream1.handshake(ssl::stream_base::client,
        buffer(mutable_char_buffer), ec);
    stream1.handshake(ssl::stream_base::server,
        buffer(mutable_char_buffer), ec);
    stream1.handshake(ssl::stream_base::client,
        buffer(const_char_buffer), ec);
    stream1.handshake(ssl::stream_base::server,
        buffer(const_char_buffer), ec);
#endif // !defined(BOOST_ASIO_ENABLE_OLD_SSL)

    stream1.async_handshake(ssl::stream_base::client, handshake_handler);
    stream1.async_handshake(ssl::stream_base::server, handshake_handler);
    int i1 = stream1.async_handshake(ssl::stream_base::client, lazy);
    (void)i1;
    int i2 = stream1.async_handshake(ssl::stream_base::server, lazy);
    (void)i2;

#if !defined(BOOST_ASIO_ENABLE_OLD_SSL)
    stream1.async_handshake(ssl::stream_base::client,
        buffer(mutable_char_buffer), buffered_handshake_handler);
    stream1.async_handshake(ssl::stream_base::server,
        buffer(mutable_char_buffer), buffered_handshake_handler);
    stream1.async_handshake(ssl::stream_base::client,
        buffer(const_char_buffer), buffered_handshake_handler);
    stream1.async_handshake(ssl::stream_base::server,
        buffer(const_char_buffer), buffered_handshake_handler);
    int i3 = stream1.async_handshake(ssl::stream_base::client,
        buffer(mutable_char_buffer), lazy);
    (void)i3;
    int i4 = stream1.async_handshake(ssl::stream_base::server,
        buffer(mutable_char_buffer), lazy);
    (void)i4;
    int i5 = stream1.async_handshake(ssl::stream_base::client,
        buffer(const_char_buffer), lazy);
    (void)i5;
    int i6 = stream1.async_handshake(ssl::stream_base::server,
        buffer(const_char_buffer), lazy);
    (void)i6;
#endif // !defined(BOOST_ASIO_ENABLE_OLD_SSL)

    stream1.shutdown();
    stream1.shutdown(ec);

    stream1.async_shutdown(shutdown_handler);
    int i7 = stream1.async_shutdown(lazy);
    (void)i7;

    stream1.write_some(buffer(mutable_char_buffer));
    stream1.write_some(buffer(const_char_buffer));
    stream1.write_some(buffer(mutable_char_buffer), ec);
    stream1.write_some(buffer(const_char_buffer), ec);

    stream1.async_write_some(buffer(mutable_char_buffer), write_some_handler);
    stream1.async_write_some(buffer(const_char_buffer), write_some_handler);
    int i8 = stream1.async_write_some(buffer(mutable_char_buffer), lazy);
    (void)i8;
    int i9 = stream1.async_write_some(buffer(const_char_buffer), lazy);
    (void)i9;

    stream1.read_some(buffer(mutable_char_buffer));
    stream1.read_some(buffer(mutable_char_buffer), ec);

    stream1.async_read_some(buffer(mutable_char_buffer), read_some_handler);
    int i10 = stream1.async_read_some(buffer(mutable_char_buffer), lazy);
    (void)i10;

#if defined(BOOST_ASIO_ENABLE_OLD_SSL)
    stream1.peek(buffer(mutable_char_buffer));
    stream1.peek(buffer(mutable_char_buffer), ec);

    std::size_t in_avail1 = stream1.in_avail();
    (void)in_avail1;
    std::size_t in_avail2 = stream1.in_avail(ec);
    (void)in_avail2;
#endif // defined(BOOST_ASIO_ENABLE_OLD_SSL)
  }
  catch (std::exception&)
  {
  }
}

} // namespace ssl_stream_compile

//------------------------------------------------------------------------------

BOOST_ASIO_TEST_SUITE
(
  "ssl/stream",
  BOOST_ASIO_TEST_CASE(ssl_stream_compile::test)
)
