//
// serial_port_base.cpp
// ~~~~~~~~~~~~~~~~~~~~
//
// Copyright (c) 2003-2013 Christopher M. Kohlhoff (chris at kohlhoff dot com)
// Copyright (c) 2008 Rep Invariant Systems, Inc. (info@repinvariant.com)
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//

// Disable autolinking for unit tests.
#if !defined(BOOST_ALL_NO_LIB)
#define BOOST_ALL_NO_LIB 1
#endif // !defined(BOOST_ALL_NO_LIB)

// Test that header file is self-contained.
#include <boost/asio/serial_port_base.hpp>

#include <boost/asio/io_service.hpp>
#include <boost/asio/serial_port.hpp>
#include "unit_test.hpp"

//------------------------------------------------------------------------------

// serial_port_base_compile test
// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
// Verify that all options and and their accessors compile. Runtime failures are
// ignored.

namespace serial_port_base_compile {

void test()
{
#if defined(BOOST_ASIO_HAS_SERIAL_PORT)
  using namespace boost::asio;

  try
  {
    io_service ios;
    serial_port port(ios);

    // baud_rate class.

    serial_port_base::baud_rate baud_rate1(9600);
    port.set_option(baud_rate1);
    serial_port_base::baud_rate baud_rate2;
    port.get_option(baud_rate2);
    (void)static_cast<unsigned int>(baud_rate2.value());

    // flow_control class.

    serial_port_base::flow_control flow_control1(
      serial_port_base::flow_control::none);
    port.set_option(flow_control1);
    serial_port_base::flow_control flow_control2;
    port.get_option(flow_control2);
    (void)static_cast<serial_port_base::flow_control::type>(
        flow_control2.value());

    // parity class.

    serial_port_base::parity parity1(serial_port_base::parity::none);
    port.set_option(parity1);
    serial_port_base::parity parity2;
    port.get_option(parity2);
    (void)static_cast<serial_port_base::parity::type>(parity2.value());

    // stop_bits class.

    serial_port_base::stop_bits stop_bits1(serial_port_base::stop_bits::one);
    port.set_option(stop_bits1);
    serial_port_base::stop_bits stop_bits2;
    port.get_option(stop_bits2);
    (void)static_cast<serial_port_base::stop_bits::type>(stop_bits2.value());

    // character_size class.

    serial_port_base::character_size character_size1(8);
    port.set_option(character_size1);
    serial_port_base::character_size character_size2;
    port.get_option(character_size2);
    (void)static_cast<unsigned int>(character_size2.value());
  }
  catch (std::exception&)
  {
  }
#endif // defined(BOOST_ASIO_HAS_SERIAL_PORT)
}

} // namespace serial_port_base_compile

//------------------------------------------------------------------------------

BOOST_ASIO_TEST_SUITE
(
  "serial_port_base",
  BOOST_ASIO_TEST_CASE(serial_port_base_compile::test)
)
