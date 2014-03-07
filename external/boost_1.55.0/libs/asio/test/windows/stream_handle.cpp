//
// stream_handle.cpp
// ~~~~~~~~~~~~~~~~~
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
#include <boost/asio/windows/stream_handle.hpp>

#include <boost/asio/io_service.hpp>
#include "../archetypes/async_result.hpp"
#include "../unit_test.hpp"

//------------------------------------------------------------------------------

// windows_stream_handle_compile test
// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
// The following test checks that all public member functions on the class
// windows::stream_handle compile and link correctly. Runtime failures are
// ignored.

namespace windows_stream_handle_compile {

void write_some_handler(const boost::system::error_code&, std::size_t)
{
}

void read_some_handler(const boost::system::error_code&, std::size_t)
{
}

void test()
{
#if defined(BOOST_ASIO_HAS_WINDOWS_STREAM_HANDLE)
  using namespace boost::asio;
  namespace win = boost::asio::windows;

  try
  {
    io_service ios;
    char mutable_char_buffer[128] = "";
    const char const_char_buffer[128] = "";
    archetypes::lazy_handler lazy;
    boost::system::error_code ec;

    // basic_stream_handle constructors.

    win::stream_handle handle1(ios);
    HANDLE native_handle1 = INVALID_HANDLE_VALUE;
    win::stream_handle handle2(ios, native_handle1);

#if defined(BOOST_ASIO_HAS_MOVE)
    win::stream_handle handle3(std::move(handle2));
#endif // defined(BOOST_ASIO_HAS_MOVE)

    // basic_stream_handle operators.

#if defined(BOOST_ASIO_HAS_MOVE)
    handle1 = win::stream_handle(ios);
    handle1 = std::move(handle2);
#endif // defined(BOOST_ASIO_HAS_MOVE)

    // basic_io_object functions.

    io_service& ios_ref = handle1.get_io_service();
    (void)ios_ref;

    // basic_handle functions.

    win::stream_handle::lowest_layer_type& lowest_layer
      = handle1.lowest_layer();
    (void)lowest_layer;

    const win::stream_handle& handle4 = handle1;
    const win::stream_handle::lowest_layer_type& lowest_layer2
      = handle4.lowest_layer();
    (void)lowest_layer2;

    HANDLE native_handle2 = INVALID_HANDLE_VALUE;
    handle1.assign(native_handle2);

    bool is_open = handle1.is_open();
    (void)is_open;

    handle1.close();
    handle1.close(ec);

    win::stream_handle::native_type native_handle3 = handle1.native();
    (void)native_handle3;

    win::stream_handle::native_handle_type native_handle4
      = handle1.native_handle();
    (void)native_handle4;

    handle1.cancel();
    handle1.cancel(ec);

    // basic_stream_handle functions.

    handle1.write_some(buffer(mutable_char_buffer));
    handle1.write_some(buffer(const_char_buffer));
    handle1.write_some(buffer(mutable_char_buffer), ec);
    handle1.write_some(buffer(const_char_buffer), ec);

    handle1.async_write_some(buffer(mutable_char_buffer), &write_some_handler);
    handle1.async_write_some(buffer(const_char_buffer), &write_some_handler);
    int i1 = handle1.async_write_some(buffer(mutable_char_buffer), lazy);
    (void)i1;
    int i2 = handle1.async_write_some(buffer(const_char_buffer), lazy);
    (void)i2;

    handle1.read_some(buffer(mutable_char_buffer));
    handle1.read_some(buffer(mutable_char_buffer), ec);

    handle1.async_read_some(buffer(mutable_char_buffer), &read_some_handler);
    int i3 = handle1.async_read_some(buffer(mutable_char_buffer), lazy);
    (void)i3;
  }
  catch (std::exception&)
  {
  }
#endif // defined(BOOST_ASIO_HAS_WINDOWS_STREAM_HANDLE)
}

} // namespace windows_stream_handle_compile

//------------------------------------------------------------------------------

BOOST_ASIO_TEST_SUITE
(
  "windows/stream_handle",
  BOOST_ASIO_TEST_CASE(windows_stream_handle_compile::test)
)
