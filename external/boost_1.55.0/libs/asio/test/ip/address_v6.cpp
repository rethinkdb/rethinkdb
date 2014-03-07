//
// address_v6.cpp
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
#include <boost/asio/ip/address_v6.hpp>

#include "../unit_test.hpp"
#include <sstream>

//------------------------------------------------------------------------------

// ip_address_v6_compile test
// ~~~~~~~~~~~~~~~~~~~~~~~~~~
// The following test checks that all public member functions on the class
// ip::address_v6 compile and link correctly. Runtime failures are ignored.

namespace ip_address_v6_compile {

void test()
{
  using namespace boost::asio;
  namespace ip = boost::asio::ip;

  try
  {
    boost::system::error_code ec;

    // address_v6 constructors.

    ip::address_v6 addr1;
    const ip::address_v6::bytes_type const_bytes_value = { { 0 } };
    ip::address_v6 addr2(const_bytes_value);

    // address_v6 functions.

    unsigned long scope_id = addr1.scope_id();
    addr1.scope_id(scope_id);

    bool b = addr1.is_unspecified();
    (void)b;

    b = addr1.is_loopback();
    (void)b;

    b = addr1.is_multicast();
    (void)b;

    b = addr1.is_link_local();
    (void)b;

    b = addr1.is_site_local();
    (void)b;

    b = addr1.is_v4_mapped();
    (void)b;

    b = addr1.is_v4_compatible();
    (void)b;

    b = addr1.is_multicast_node_local();
    (void)b;

    b = addr1.is_multicast_link_local();
    (void)b;

    b = addr1.is_multicast_site_local();
    (void)b;

    b = addr1.is_multicast_org_local();
    (void)b;

    b = addr1.is_multicast_global();
    (void)b;

    ip::address_v6::bytes_type bytes_value = addr1.to_bytes();
    (void)bytes_value;

    std::string string_value = addr1.to_string();
    string_value = addr1.to_string(ec);

    ip::address_v4 addr3 = addr1.to_v4();

    // address_v6 static functions.

    addr1 = ip::address_v6::from_string("0::0");
    addr1 = ip::address_v6::from_string("0::0", ec);
    addr1 = ip::address_v6::from_string(string_value);
    addr1 = ip::address_v6::from_string(string_value, ec);

    addr1 = ip::address_v6::any();

    addr1 = ip::address_v6::loopback();

    addr1 = ip::address_v6::v4_mapped(addr3);

    addr1 = ip::address_v6::v4_compatible(addr3);

    // address_v6 comparisons.

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

    // address_v6 I/O.

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

} // namespace ip_address_v6_compile

//------------------------------------------------------------------------------

// ip_address_v6_runtime test
// ~~~~~~~~~~~~~~~~~~~~~~~~~~
// The following test checks that the various public member functions meet the
// necessary postconditions.

namespace ip_address_v6_runtime {

void test()
{
  using boost::asio::ip::address_v6;

  address_v6 a1;
  BOOST_ASIO_CHECK(a1.is_unspecified());
  BOOST_ASIO_CHECK(a1.scope_id() == 0);

  address_v6::bytes_type b1 = {{ 1, 2, 3, 4, 5,
    6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 16 }};
  address_v6 a2(b1, 12345);
  BOOST_ASIO_CHECK(a2.to_bytes()[0] == 1);
  BOOST_ASIO_CHECK(a2.to_bytes()[1] == 2);
  BOOST_ASIO_CHECK(a2.to_bytes()[2] == 3);
  BOOST_ASIO_CHECK(a2.to_bytes()[3] == 4);
  BOOST_ASIO_CHECK(a2.to_bytes()[4] == 5);
  BOOST_ASIO_CHECK(a2.to_bytes()[5] == 6);
  BOOST_ASIO_CHECK(a2.to_bytes()[6] == 7);
  BOOST_ASIO_CHECK(a2.to_bytes()[7] == 8);
  BOOST_ASIO_CHECK(a2.to_bytes()[8] == 9);
  BOOST_ASIO_CHECK(a2.to_bytes()[9] == 10);
  BOOST_ASIO_CHECK(a2.to_bytes()[10] == 11);
  BOOST_ASIO_CHECK(a2.to_bytes()[11] == 12);
  BOOST_ASIO_CHECK(a2.to_bytes()[12] == 13);
  BOOST_ASIO_CHECK(a2.to_bytes()[13] == 14);
  BOOST_ASIO_CHECK(a2.to_bytes()[14] == 15);
  BOOST_ASIO_CHECK(a2.to_bytes()[15] == 16);
  BOOST_ASIO_CHECK(a2.scope_id() == 12345);

  address_v6 a3;
  a3.scope_id(12345);
  BOOST_ASIO_CHECK(a3.scope_id() == 12345);

  address_v6 unspecified_address;
  address_v6::bytes_type loopback_bytes = {{ 0 }};
  loopback_bytes[15] = 1;
  address_v6 loopback_address(loopback_bytes);
  address_v6::bytes_type link_local_bytes = {{ 0xFE, 0x80, 1 }};
  address_v6 link_local_address(link_local_bytes);
  address_v6::bytes_type site_local_bytes = {{ 0xFE, 0xC0, 1 }};
  address_v6 site_local_address(site_local_bytes);
  address_v6::bytes_type v4_mapped_bytes = {{ 0 }};
  v4_mapped_bytes[10] = 0xFF, v4_mapped_bytes[11] = 0xFF;
  v4_mapped_bytes[12] = 1, v4_mapped_bytes[13] = 2;
  v4_mapped_bytes[14] = 3, v4_mapped_bytes[15] = 4;
  address_v6 v4_mapped_address(v4_mapped_bytes);
  address_v6::bytes_type v4_compat_bytes = {{ 0 }};
  v4_compat_bytes[12] = 1, v4_compat_bytes[13] = 2;
  v4_compat_bytes[14] = 3, v4_compat_bytes[15] = 4;
  address_v6 v4_compat_address(v4_compat_bytes);
  address_v6::bytes_type mcast_global_bytes = {{ 0xFF, 0x0E, 1 }};
  address_v6 mcast_global_address(mcast_global_bytes);
  address_v6::bytes_type mcast_link_local_bytes = {{ 0xFF, 0x02, 1 }};
  address_v6 mcast_link_local_address(mcast_link_local_bytes);
  address_v6::bytes_type mcast_node_local_bytes = {{ 0xFF, 0x01, 1 }};
  address_v6 mcast_node_local_address(mcast_node_local_bytes);
  address_v6::bytes_type mcast_org_local_bytes = {{ 0xFF, 0x08, 1 }};
  address_v6 mcast_org_local_address(mcast_org_local_bytes);
  address_v6::bytes_type mcast_site_local_bytes = {{ 0xFF, 0x05, 1 }};
  address_v6 mcast_site_local_address(mcast_site_local_bytes);

  BOOST_ASIO_CHECK(!unspecified_address.is_loopback());
  BOOST_ASIO_CHECK(loopback_address.is_loopback());
  BOOST_ASIO_CHECK(!link_local_address.is_loopback());
  BOOST_ASIO_CHECK(!site_local_address.is_loopback());
  BOOST_ASIO_CHECK(!v4_mapped_address.is_loopback());
  BOOST_ASIO_CHECK(!v4_compat_address.is_loopback());
  BOOST_ASIO_CHECK(!mcast_global_address.is_loopback());
  BOOST_ASIO_CHECK(!mcast_link_local_address.is_loopback());
  BOOST_ASIO_CHECK(!mcast_node_local_address.is_loopback());
  BOOST_ASIO_CHECK(!mcast_org_local_address.is_loopback());
  BOOST_ASIO_CHECK(!mcast_site_local_address.is_loopback());

  BOOST_ASIO_CHECK(unspecified_address.is_unspecified());
  BOOST_ASIO_CHECK(!loopback_address.is_unspecified());
  BOOST_ASIO_CHECK(!link_local_address.is_unspecified());
  BOOST_ASIO_CHECK(!site_local_address.is_unspecified());
  BOOST_ASIO_CHECK(!v4_mapped_address.is_unspecified());
  BOOST_ASIO_CHECK(!v4_compat_address.is_unspecified());
  BOOST_ASIO_CHECK(!mcast_global_address.is_unspecified());
  BOOST_ASIO_CHECK(!mcast_link_local_address.is_unspecified());
  BOOST_ASIO_CHECK(!mcast_node_local_address.is_unspecified());
  BOOST_ASIO_CHECK(!mcast_org_local_address.is_unspecified());
  BOOST_ASIO_CHECK(!mcast_site_local_address.is_unspecified());

  BOOST_ASIO_CHECK(!unspecified_address.is_link_local());
  BOOST_ASIO_CHECK(!loopback_address.is_link_local());
  BOOST_ASIO_CHECK(link_local_address.is_link_local());
  BOOST_ASIO_CHECK(!site_local_address.is_link_local());
  BOOST_ASIO_CHECK(!v4_mapped_address.is_link_local());
  BOOST_ASIO_CHECK(!v4_compat_address.is_link_local());
  BOOST_ASIO_CHECK(!mcast_global_address.is_link_local());
  BOOST_ASIO_CHECK(!mcast_link_local_address.is_link_local());
  BOOST_ASIO_CHECK(!mcast_node_local_address.is_link_local());
  BOOST_ASIO_CHECK(!mcast_org_local_address.is_link_local());
  BOOST_ASIO_CHECK(!mcast_site_local_address.is_link_local());

  BOOST_ASIO_CHECK(!unspecified_address.is_site_local());
  BOOST_ASIO_CHECK(!loopback_address.is_site_local());
  BOOST_ASIO_CHECK(!link_local_address.is_site_local());
  BOOST_ASIO_CHECK(site_local_address.is_site_local());
  BOOST_ASIO_CHECK(!v4_mapped_address.is_site_local());
  BOOST_ASIO_CHECK(!v4_compat_address.is_site_local());
  BOOST_ASIO_CHECK(!mcast_global_address.is_site_local());
  BOOST_ASIO_CHECK(!mcast_link_local_address.is_site_local());
  BOOST_ASIO_CHECK(!mcast_node_local_address.is_site_local());
  BOOST_ASIO_CHECK(!mcast_org_local_address.is_site_local());
  BOOST_ASIO_CHECK(!mcast_site_local_address.is_site_local());

  BOOST_ASIO_CHECK(!unspecified_address.is_v4_mapped());
  BOOST_ASIO_CHECK(!loopback_address.is_v4_mapped());
  BOOST_ASIO_CHECK(!link_local_address.is_v4_mapped());
  BOOST_ASIO_CHECK(!site_local_address.is_v4_mapped());
  BOOST_ASIO_CHECK(v4_mapped_address.is_v4_mapped());
  BOOST_ASIO_CHECK(!v4_compat_address.is_v4_mapped());
  BOOST_ASIO_CHECK(!mcast_global_address.is_v4_mapped());
  BOOST_ASIO_CHECK(!mcast_link_local_address.is_v4_mapped());
  BOOST_ASIO_CHECK(!mcast_node_local_address.is_v4_mapped());
  BOOST_ASIO_CHECK(!mcast_org_local_address.is_v4_mapped());
  BOOST_ASIO_CHECK(!mcast_site_local_address.is_v4_mapped());

  BOOST_ASIO_CHECK(!unspecified_address.is_v4_compatible());
  BOOST_ASIO_CHECK(!loopback_address.is_v4_compatible());
  BOOST_ASIO_CHECK(!link_local_address.is_v4_compatible());
  BOOST_ASIO_CHECK(!site_local_address.is_v4_compatible());
  BOOST_ASIO_CHECK(!v4_mapped_address.is_v4_compatible());
  BOOST_ASIO_CHECK(v4_compat_address.is_v4_compatible());
  BOOST_ASIO_CHECK(!mcast_global_address.is_v4_compatible());
  BOOST_ASIO_CHECK(!mcast_link_local_address.is_v4_compatible());
  BOOST_ASIO_CHECK(!mcast_node_local_address.is_v4_compatible());
  BOOST_ASIO_CHECK(!mcast_org_local_address.is_v4_compatible());
  BOOST_ASIO_CHECK(!mcast_site_local_address.is_v4_compatible());

  BOOST_ASIO_CHECK(!unspecified_address.is_multicast());
  BOOST_ASIO_CHECK(!loopback_address.is_multicast());
  BOOST_ASIO_CHECK(!link_local_address.is_multicast());
  BOOST_ASIO_CHECK(!site_local_address.is_multicast());
  BOOST_ASIO_CHECK(!v4_mapped_address.is_multicast());
  BOOST_ASIO_CHECK(!v4_compat_address.is_multicast());
  BOOST_ASIO_CHECK(mcast_global_address.is_multicast());
  BOOST_ASIO_CHECK(mcast_link_local_address.is_multicast());
  BOOST_ASIO_CHECK(mcast_node_local_address.is_multicast());
  BOOST_ASIO_CHECK(mcast_org_local_address.is_multicast());
  BOOST_ASIO_CHECK(mcast_site_local_address.is_multicast());

  BOOST_ASIO_CHECK(!unspecified_address.is_multicast_global());
  BOOST_ASIO_CHECK(!loopback_address.is_multicast_global());
  BOOST_ASIO_CHECK(!link_local_address.is_multicast_global());
  BOOST_ASIO_CHECK(!site_local_address.is_multicast_global());
  BOOST_ASIO_CHECK(!v4_mapped_address.is_multicast_global());
  BOOST_ASIO_CHECK(!v4_compat_address.is_multicast_global());
  BOOST_ASIO_CHECK(mcast_global_address.is_multicast_global());
  BOOST_ASIO_CHECK(!mcast_link_local_address.is_multicast_global());
  BOOST_ASIO_CHECK(!mcast_node_local_address.is_multicast_global());
  BOOST_ASIO_CHECK(!mcast_org_local_address.is_multicast_global());
  BOOST_ASIO_CHECK(!mcast_site_local_address.is_multicast_global());

  BOOST_ASIO_CHECK(!unspecified_address.is_multicast_link_local());
  BOOST_ASIO_CHECK(!loopback_address.is_multicast_link_local());
  BOOST_ASIO_CHECK(!link_local_address.is_multicast_link_local());
  BOOST_ASIO_CHECK(!site_local_address.is_multicast_link_local());
  BOOST_ASIO_CHECK(!v4_mapped_address.is_multicast_link_local());
  BOOST_ASIO_CHECK(!v4_compat_address.is_multicast_link_local());
  BOOST_ASIO_CHECK(!mcast_global_address.is_multicast_link_local());
  BOOST_ASIO_CHECK(mcast_link_local_address.is_multicast_link_local());
  BOOST_ASIO_CHECK(!mcast_node_local_address.is_multicast_link_local());
  BOOST_ASIO_CHECK(!mcast_org_local_address.is_multicast_link_local());
  BOOST_ASIO_CHECK(!mcast_site_local_address.is_multicast_link_local());

  BOOST_ASIO_CHECK(!unspecified_address.is_multicast_node_local());
  BOOST_ASIO_CHECK(!loopback_address.is_multicast_node_local());
  BOOST_ASIO_CHECK(!link_local_address.is_multicast_node_local());
  BOOST_ASIO_CHECK(!site_local_address.is_multicast_node_local());
  BOOST_ASIO_CHECK(!v4_mapped_address.is_multicast_node_local());
  BOOST_ASIO_CHECK(!v4_compat_address.is_multicast_node_local());
  BOOST_ASIO_CHECK(!mcast_global_address.is_multicast_node_local());
  BOOST_ASIO_CHECK(!mcast_link_local_address.is_multicast_node_local());
  BOOST_ASIO_CHECK(mcast_node_local_address.is_multicast_node_local());
  BOOST_ASIO_CHECK(!mcast_org_local_address.is_multicast_node_local());
  BOOST_ASIO_CHECK(!mcast_site_local_address.is_multicast_node_local());

  BOOST_ASIO_CHECK(!unspecified_address.is_multicast_org_local());
  BOOST_ASIO_CHECK(!loopback_address.is_multicast_org_local());
  BOOST_ASIO_CHECK(!link_local_address.is_multicast_org_local());
  BOOST_ASIO_CHECK(!site_local_address.is_multicast_org_local());
  BOOST_ASIO_CHECK(!v4_mapped_address.is_multicast_org_local());
  BOOST_ASIO_CHECK(!v4_compat_address.is_multicast_org_local());
  BOOST_ASIO_CHECK(!mcast_global_address.is_multicast_org_local());
  BOOST_ASIO_CHECK(!mcast_link_local_address.is_multicast_org_local());
  BOOST_ASIO_CHECK(!mcast_node_local_address.is_multicast_org_local());
  BOOST_ASIO_CHECK(mcast_org_local_address.is_multicast_org_local());
  BOOST_ASIO_CHECK(!mcast_site_local_address.is_multicast_org_local());

  BOOST_ASIO_CHECK(!unspecified_address.is_multicast_site_local());
  BOOST_ASIO_CHECK(!loopback_address.is_multicast_site_local());
  BOOST_ASIO_CHECK(!link_local_address.is_multicast_site_local());
  BOOST_ASIO_CHECK(!site_local_address.is_multicast_site_local());
  BOOST_ASIO_CHECK(!v4_mapped_address.is_multicast_site_local());
  BOOST_ASIO_CHECK(!v4_compat_address.is_multicast_site_local());
  BOOST_ASIO_CHECK(!mcast_global_address.is_multicast_site_local());
  BOOST_ASIO_CHECK(!mcast_link_local_address.is_multicast_site_local());
  BOOST_ASIO_CHECK(!mcast_node_local_address.is_multicast_site_local());
  BOOST_ASIO_CHECK(!mcast_org_local_address.is_multicast_site_local());
  BOOST_ASIO_CHECK(mcast_site_local_address.is_multicast_site_local());

  BOOST_ASIO_CHECK(address_v6::loopback().is_loopback());
}

} // namespace ip_address_v6_runtime

//------------------------------------------------------------------------------

BOOST_ASIO_TEST_SUITE
(
  "ip/address_v6",
  BOOST_ASIO_TEST_CASE(ip_address_v6_compile::test)
  BOOST_ASIO_TEST_CASE(ip_address_v6_runtime::test)
)
