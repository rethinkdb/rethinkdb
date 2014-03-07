//
// object_handle.cpp
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
#include <boost/asio/windows/object_handle.hpp>

#include <boost/asio/io_service.hpp>
#include "../archetypes/async_result.hpp"
#include "../unit_test.hpp"

//------------------------------------------------------------------------------

// windows_object_handle_compile test
// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
// The following test checks that all public member functions on the class
// windows::object_handle compile and link correctly. Runtime failures are
// ignored.

namespace windows_object_handle_compile {

void wait_handler(const boost::system::error_code&)
{
}

void test()
{
#if defined(BOOST_ASIO_HAS_WINDOWS_OBJECT_HANDLE)
  using namespace boost::asio;
  namespace win = boost::asio::windows;

  try
  {
    io_service ios;
    archetypes::lazy_handler lazy;
    boost::system::error_code ec;

    // basic_object_handle constructors.

    win::object_handle handle1(ios);
    HANDLE native_handle1 = INVALID_HANDLE_VALUE;
    win::object_handle handle2(ios, native_handle1);

#if defined(BOOST_ASIO_HAS_MOVE)
    win::object_handle handle3(std::move(handle2));
#endif // defined(BOOST_ASIO_HAS_MOVE)

    // basic_object_handle operators.

#if defined(BOOST_ASIO_HAS_MOVE)
    handle1 = win::object_handle(ios);
    handle1 = std::move(handle2);
#endif // defined(BOOST_ASIO_HAS_MOVE)

    // basic_io_object functions.

    io_service& ios_ref = handle1.get_io_service();
    (void)ios_ref;

    // basic_handle functions.

    win::object_handle::lowest_layer_type& lowest_layer
      = handle1.lowest_layer();
    (void)lowest_layer;

    const win::object_handle& handle4 = handle1;
    const win::object_handle::lowest_layer_type& lowest_layer2
      = handle4.lowest_layer();
    (void)lowest_layer2;

    HANDLE native_handle2 = INVALID_HANDLE_VALUE;
    handle1.assign(native_handle2);

    bool is_open = handle1.is_open();
    (void)is_open;

    handle1.close();
    handle1.close(ec);

    win::object_handle::native_handle_type native_handle3
      = handle1.native_handle();
    (void)native_handle3;

    handle1.cancel();
    handle1.cancel(ec);

    // basic_object_handle functions.

    handle1.wait();
    handle1.wait(ec);

    handle1.async_wait(&wait_handler);
    int i1 = handle1.async_wait(lazy);
    (void)i1;
  }
  catch (std::exception&)
  {
  }
#endif // defined(BOOST_ASIO_HAS_WINDOWS_OBJECT_HANDLE)
}

} // namespace windows_object_handle_compile

//------------------------------------------------------------------------------

BOOST_ASIO_TEST_SUITE
(
  "windows/object_handle",
  BOOST_ASIO_TEST_CASE(windows_object_handle_compile::test)
)
