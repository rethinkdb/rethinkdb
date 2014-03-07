//
// address.cpp
// ~~~~~~~~~~~
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
#include <boost/asio/ip/address.hpp>

#include "../unit_test.hpp"
#include <sstream>

//------------------------------------------------------------------------------

// ip_address_compile test
// ~~~~~~~~~~~~~~~~~~~~~~~
// The following test checks that all public member functions on the class
// ip::address compile and link correctly. Runtime failures are ignored.

namespace ip_address_compile {

void test()
{
  using namespace boost::asio;
  namespace ip = boost::asio::ip;

  try
  {
    boost::system::error_code ec;

    // address constructors.

    ip::address addr1;
    const ip::address_v4 const_addr_v4;
    ip::address addr2(const_addr_v4);
    const ip::address_v6 const_addr_v6;
    ip::address addr3(const_addr_v6);

    // address functions.

    bool b = addr1.is_v4();
    (void)b;

    b = addr1.is_v6();
    (void)b;

    b = addr1.is_loopback();
    (void)b;

    b = addr1.is_unspecified();
    (void)b;

    b = addr1.is_multicast();
    (void)b;

    ip::address_v4 addr_v4_value = addr1.to_v4();
    (void)addr_v4_value;

    ip::address_v6 addr_v6_value = addr1.to_v6();
    (void)addr_v6_value;

    std::string string_value = addr1.to_string();
    string_value = addr1.to_string(ec);

    // address static functions.

    addr1 = ip::address::from_string("127.0.0.1");
    addr1 = ip::address::from_string("127.0.0.1", ec);
    addr1 = ip::address::from_string(string_value);
    addr1 = ip::address::from_string(string_value, ec);

    // address comparisons.

    b = (addr1 == addr2);
    (void)b;

    b = (addr1 != addr2);
    (void)b;

    b = (addr1 < addr2);
    (void)b;

    b = (addr1 > addr2);
    (void)b;

    b = (addr1 <= addr2);
    (void)b;

    b = (addr1 >= addr2);
    (void)b;

    // address I/O.

    std::ostringstream os;
    os << addr1;

#if !defined(BOOST_NO_STD_WSTREAMBUF)
    std::wostringstream wos;
    wos << addr1;
#endif // !defined(BOOST_NO_STD_WSTREAMBUF)
  }
  catch (std::exception&)
  {
  }
}

} // namespace ip_address_compile

//------------------------------------------------------------------------------

BOOST_ASIO_TEST_SUITE
(
  "ip/address",
  BOOST_ASIO_TEST_CASE(ip_address_compile::test)
)
