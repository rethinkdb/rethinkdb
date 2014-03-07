//
// overlapped_ptr.cpp
// ~~~~~~~~~~~~~~~~~~
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
#include <boost/asio/windows/overlapped_ptr.hpp>

#include <boost/asio/io_service.hpp>
#include "../unit_test.hpp"

//------------------------------------------------------------------------------

// windows_overlapped_ptr_compile test
// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
// The following test checks that all public member functions on the class
// windows::overlapped_ptr compile and link correctly. Runtime failures are
// ignored.

namespace windows_overlapped_ptr_compile {

void overlapped_handler_1(const boost::system::error_code&, std::size_t)
{
}

struct overlapped_handler_2
{
  void operator()(const boost::system::error_code&, std::size_t)
  {
  }
};

void test()
{
#if defined(BOOST_ASIO_HAS_WINDOWS_OVERLAPPED_PTR)
  using namespace boost::asio;
  namespace win = boost::asio::windows;

  try
  {
    io_service ios;

    // basic_overlapped_ptr constructors.

    win::overlapped_ptr ptr1;

    win::overlapped_ptr ptr2(ios, &overlapped_handler_1);
    win::overlapped_ptr ptr3(ios, overlapped_handler_2());

    // overlapped_ptr functions.

    ptr1.reset();

    ptr2.reset(ios, &overlapped_handler_1);
    ptr3.reset(ios, overlapped_handler_2());

    OVERLAPPED* ov1 = ptr1.get();
    (void)ov1;

    const win::overlapped_ptr& ptr4(ptr1);
    const OVERLAPPED* ov2 = ptr4.get();
    (void)ov2;

    OVERLAPPED* ov3 = ptr1.release();
    (void)ov3;

    boost::system::error_code ec;
    std::size_t bytes_transferred = 0;
    ptr1.complete(ec, bytes_transferred);
  }
  catch (std::exception&)
  {
  }
#endif // defined(BOOST_ASIO_HAS_WINDOWS_OVERLAPPED_PTR)
}

} // namespace windows_overlapped_ptr_compile

//------------------------------------------------------------------------------

BOOST_ASIO_TEST_SUITE
(
  "windows/overlapped_ptr",
  BOOST_ASIO_TEST_CASE(windows_overlapped_ptr_compile::test)
)
