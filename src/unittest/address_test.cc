// Copyright 2024 RethinkDB, all rights reserved.
#include <set>
#include <string>

#include "unittest/gtest.hpp"

#include "arch/address.hpp"
#include "containers/archive/string_stream.hpp"

namespace unittest {

TEST(IPAddressTest, DefaultConstructorIsInvalid) {
    /* The default constructor only sets the address family to
    `RDB_UNSPEC_ADDR` so that deserialization code can fill the rest in
    later; the resulting object must not be used as an actual address. */
    ip_address_t address;
    ASSERT_FALSE(address.is_valid());
    ASSERT_FALSE(address.is_ipv4());
    ASSERT_FALSE(address.is_ipv6());
    ASSERT_EQ(AF_UNSPEC, address.get_address_family());
    ASSERT_FALSE(address.is_loopback());
    ASSERT_FALSE(address.is_any());
}

TEST(IPAddressTest, Ipv4AddressIsValid) {
    ip_address_t address("127.0.0.1");
    ASSERT_TRUE(address.is_valid());
    ASSERT_TRUE(address.is_ipv4());
    ASSERT_FALSE(address.is_ipv6());
    ASSERT_EQ(AF_INET, address.get_address_family());
    ASSERT_TRUE(address.is_loopback());
    ASSERT_FALSE(address.is_any());
}

TEST(IPAddressTest, Ipv6AddressIsValid) {
    ip_address_t address("2001:db8::1");
    ASSERT_TRUE(address.is_valid());
    ASSERT_TRUE(address.is_ipv6());
    ASSERT_FALSE(address.is_ipv4());
    ASSERT_EQ(AF_INET6, address.get_address_family());
    ASSERT_FALSE(address.is_loopback());
    ASSERT_FALSE(address.is_any());
}

TEST(IPAddressTest, Ipv4ToStringRoundTrip) {
    ip_address_t address("192.168.1.42");
    ASSERT_TRUE(address.is_valid());
    ASSERT_EQ(std::string("192.168.1.42"), address.to_string());
}

TEST(IPAddressTest, Ipv6ToStringRoundTrip) {
    ip_address_t address("2001:db8::1");
    ASSERT_TRUE(address.is_valid());
    ASSERT_EQ(std::string("2001:db8::1"), address.to_string());
}

TEST(IPAddressTest, AnyAddressesAreValid) {
    ip_address_t any_v4 = ip_address_t::any(AF_INET);
    ASSERT_TRUE(any_v4.is_valid());
    ASSERT_TRUE(any_v4.is_ipv4());
    ASSERT_TRUE(any_v4.is_any());
    ASSERT_FALSE(any_v4.is_loopback());

    ip_address_t any_v6 = ip_address_t::any(AF_INET6);
    ASSERT_TRUE(any_v6.is_valid());
    ASSERT_TRUE(any_v6.is_ipv6());
    ASSERT_TRUE(any_v6.is_any());
    ASSERT_FALSE(any_v6.is_loopback());
}

TEST(IPAddressTest, InvalidAddressStringThrows) {
    ASSERT_THROW(ip_address_t("this is not an address"), invalid_address_exc_t);
    ASSERT_THROW(ip_address_t("999.999.999.999"), invalid_address_exc_t);
}

TEST(IPAddressTest, InvalidAddressesCompareGracefully) {
    /* Set operations must not misbehave just because an address is invalid.
    Two invalid addresses are equal to each other and sort below the valid
    ones (their address family value is smaller). */
    ip_address_t invalid_a;
    ip_address_t invalid_b;
    ASSERT_TRUE(invalid_a == invalid_b);
    ASSERT_FALSE(invalid_a < invalid_b);

    ip_address_t loopback("127.0.0.1");
    ASSERT_TRUE(invalid_a < loopback);
    ASSERT_FALSE(loopback < invalid_a);
    ASSERT_FALSE(invalid_a == loopback);
}

TEST(IPAddressTest, FilterValidAddressesKeepsValidEntries) {
    std::set<ip_address_t> addresses;
    addresses.insert(ip_address_t("10.1.2.3"));
    addresses.insert(ip_address_t("fe80::1"));
    addresses.insert(ip_address_t());  // invalid placeholder

    std::set<ip_address_t> filtered = filter_valid_addresses(addresses);
    ASSERT_EQ(2, filtered.size());
    ASSERT_TRUE(filtered.find(ip_address_t("10.1.2.3")) != filtered.end());
    ASSERT_TRUE(filtered.find(ip_address_t("fe80::1")) != filtered.end());
    ASSERT_TRUE(filtered.find(ip_address_t()) == filtered.end());
}

TEST(IPAddressTest, FilterValidAddressesHandlesEdgeCases) {
    std::set<ip_address_t> empty;
    ASSERT_EQ(0, filter_valid_addresses(empty).size());

    std::set<ip_address_t> all_invalid;
    all_invalid.insert(ip_address_t());
    all_invalid.insert(ip_address_t());
    /* The two invalid entries compare equal, so the set only holds one. */
    ASSERT_EQ(1, all_invalid.size());
    ASSERT_EQ(0, filter_valid_addresses(all_invalid).size());
}

TEST(IPAddressTest, SerializationRoundTrip) {
    /* A default-constructed address is what deserialization code starts
    from; the round trip through the archive must produce a valid address
    that is equal to the original one. */
    ip_address_t original("192.168.7.7");
    ASSERT_TRUE(original.is_valid());

    string_stream_t write_stream;
    write_message_t wm;
    serialize<cluster_version_t::LATEST_OVERALL>(&wm, original);
    ASSERT_EQ(0, send_write_message(&write_stream, &wm));

    string_read_stream_t read_stream(std::move(write_stream.str()), 0);
    ip_address_t deserialized;
    ASSERT_FALSE(deserialized.is_valid());
    archive_result_t res =
        deserialize<cluster_version_t::LATEST_OVERALL>(&read_stream,
                                                       &deserialized);
    ASSERT_EQ(archive_result_t::SUCCESS, res);
    ASSERT_TRUE(deserialized.is_valid());
    ASSERT_EQ(original, deserialized);
    ASSERT_EQ(std::string("192.168.7.7"), deserialized.to_string());
}

TEST(IPAddressTest, GetLocalIpsContainsOnlyValidAddresses) {
    /* Whatever the platform's `getifaddrs()` reports, `get_local_ips()` must
    never hand out an address that callers would crash on. */
    std::set<ip_address_t> ips =
        get_local_ips(std::set<ip_address_t>(), local_ip_filter_t::ALL);
    for (std::set<ip_address_t>::const_iterator i = ips.begin();
         i != ips.end(); ++i) {
        ASSERT_TRUE(i->is_valid());
    }
    /* Every Linux/macOS/Windows system has at least a loopback address. */
    ASSERT_FALSE(ips.empty());
}

}  // namespace unittest
