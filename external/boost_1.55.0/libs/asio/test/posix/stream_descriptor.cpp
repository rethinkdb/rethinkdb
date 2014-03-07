//
// stream_descriptor.cpp
// ~~~~~~~~~~~~~~~~~~~~~
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
#include <boost/asio/posix/stream_descriptor.hpp>

#include <boost/asio/io_service.hpp>
#include "../archetypes/async_result.hpp"
#include "../unit_test.hpp"

//------------------------------------------------------------------------------

// posix_stream_descriptor_compile test
// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
// The following test checks that all public member functions on the class
// posix::stream_descriptor compile and link correctly. Runtime failures are
// ignored.

namespace posix_stream_descriptor_compile {

void write_some_handler(const boost::system::error_code&, std::size_t)
{
}

void read_some_handler(const boost::system::error_code&, std::size_t)
{
}

void test()
{
#if defined(BOOST_ASIO_HAS_POSIX_STREAM_DESCRIPTOR)
  using namespace boost::asio;
  namespace posix = boost::asio::posix;

  try
  {
    io_service ios;
    char mutable_char_buffer[128] = "";
    const char const_char_buffer[128] = "";
    posix::descriptor_base::bytes_readable io_control_command;
    archetypes::lazy_handler lazy;
    boost::system::error_code ec;

    // basic_stream_descriptor constructors.

    posix::stream_descriptor descriptor1(ios);
    int native_descriptor1 = -1;
    posix::stream_descriptor descriptor2(ios, native_descriptor1);

#if defined(BOOST_ASIO_HAS_MOVE)
    posix::stream_descriptor descriptor3(std::move(descriptor3));
#endif // defined(BOOST_ASIO_HAS_MOVE)

    // basic_stream_descriptor operators.

#if defined(BOOST_ASIO_HAS_MOVE)
    descriptor1 = posix::stream_descriptor(ios);
    descriptor1 = std::move(descriptor2);
#endif // defined(BOOST_ASIO_HAS_MOVE)

    // basic_io_object functions.

    io_service& ios_ref = descriptor1.get_io_service();
    (void)ios_ref;

    // basic_descriptor functions.

    posix::stream_descriptor::lowest_layer_type& lowest_layer
      = descriptor1.lowest_layer();
    (void)lowest_layer;

    const posix::stream_descriptor& descriptor4 = descriptor1;
    const posix::stream_descriptor::lowest_layer_type& lowest_layer2
      = descriptor4.lowest_layer();
    (void)lowest_layer2;

    int native_descriptor2 = -1;
    descriptor1.assign(native_descriptor2);

    bool is_open = descriptor1.is_open();
    (void)is_open;

    descriptor1.close();
    descriptor1.close(ec);

    posix::stream_descriptor::native_type native_descriptor3
      = descriptor1.native();
    (void)native_descriptor3;

    posix::stream_descriptor::native_handle_type native_descriptor4
      = descriptor1.native_handle();
    (void)native_descriptor4;

    posix::stream_descriptor::native_handle_type native_descriptor5
      = descriptor1.release();
    (void)native_descriptor5;

    descriptor1.cancel();
    descriptor1.cancel(ec);

    descriptor1.io_control(io_control_command);
    descriptor1.io_control(io_control_command, ec);

    bool non_blocking1 = descriptor1.non_blocking();
    (void)non_blocking1;
    descriptor1.non_blocking(true);
    descriptor1.non_blocking(false, ec);

    bool non_blocking2 = descriptor1.native_non_blocking();
    (void)non_blocking2;
    descriptor1.native_non_blocking(true);
    descriptor1.native_non_blocking(false, ec);

    // basic_stream_descriptor functions.

    descriptor1.write_some(buffer(mutable_char_buffer));
    descriptor1.write_some(buffer(const_char_buffer));
    descriptor1.write_some(null_buffers());
    descriptor1.write_some(buffer(mutable_char_buffer), ec);
    descriptor1.write_some(buffer(const_char_buffer), ec);
    descriptor1.write_some(null_buffers(), ec);

    descriptor1.async_write_some(buffer(mutable_char_buffer),
        write_some_handler);
    descriptor1.async_write_some(buffer(const_char_buffer),
        write_some_handler);
    descriptor1.async_write_some(null_buffers(),
        write_some_handler);
    int i1 = descriptor1.async_write_some(buffer(mutable_char_buffer), lazy);
    (void)i1;
    int i2 = descriptor1.async_write_some(buffer(const_char_buffer), lazy);
    (void)i2;
    int i3 = descriptor1.async_write_some(null_buffers(), lazy);
    (void)i3;

    descriptor1.read_some(buffer(mutable_char_buffer));
    descriptor1.read_some(buffer(mutable_char_buffer), ec);
    descriptor1.read_some(null_buffers(), ec);

    descriptor1.async_read_some(buffer(mutable_char_buffer), read_some_handler);
    descriptor1.async_read_some(null_buffers(), read_some_handler);
    int i4 = descriptor1.async_read_some(buffer(mutable_char_buffer), lazy);
    (void)i4;
    int i5 = descriptor1.async_read_some(null_buffers(), lazy);
    (void)i5;
  }
  catch (std::exception&)
  {
  }
#endif // defined(BOOST_ASIO_HAS_POSIX_STREAM_DESCRIPTOR)
}

} // namespace posix_stream_descriptor_compile

//------------------------------------------------------------------------------

BOOST_ASIO_TEST_SUITE
(
  "posix/stream_descriptor",
  BOOST_ASIO_TEST_CASE(posix_stream_descriptor_compile::test)
)
