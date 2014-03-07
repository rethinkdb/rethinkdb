//
// signal_set.cpp
// ~~~~~~~~~~~~~~
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
#include <boost/asio/signal_set.hpp>

#include "archetypes/async_result.hpp"
#include <boost/asio/io_service.hpp>
#include "unit_test.hpp"

//------------------------------------------------------------------------------

// signal_set_compile test
// ~~~~~~~~~~~~~~~~~~~~~~~
// The following test checks that all public member functions on the class
// signal_set compile and link correctly. Runtime failures are ignored.

namespace signal_set_compile {

void signal_handler(const boost::system::error_code&, int)
{
}

void test()
{
  using namespace boost::asio;

  try
  {
    io_service ios;
    archetypes::lazy_handler lazy;
    boost::system::error_code ec;

    // basic_signal_set constructors.

    signal_set set1(ios);
    signal_set set2(ios, 1);
    signal_set set3(ios, 1, 2);
    signal_set set4(ios, 1, 2, 3);

    // basic_io_object functions.

    io_service& ios_ref = set1.get_io_service();
    (void)ios_ref;

    // basic_signal_set functions.

    set1.add(1);
    set1.add(1, ec);

    set1.remove(1);
    set1.remove(1, ec);

    set1.clear();
    set1.clear(ec);

    set1.cancel();
    set1.cancel(ec);

    set1.async_wait(&signal_handler);
    int i = set1.async_wait(lazy);
    (void)i;
  }
  catch (std::exception&)
  {
  }
}

} // namespace signal_set_compile

//------------------------------------------------------------------------------

BOOST_ASIO_TEST_SUITE
(
  "signal_set",
  BOOST_ASIO_TEST_CASE(signal_set_compile::test)
)
