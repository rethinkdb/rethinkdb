// Copyright 2010-2013 RethinkDB, all rights reserved.
#ifndef ARCH_ADDRESS_HPP_
#define ARCH_ADDRESS_HPP_

#include <arpa/inet.h>   /* for `inet_ntop()` */
#include <net/if.h>
#include <netdb.h>
#include <netinet/in.h>
#include <sys/ioctl.h>

#include <string>
#include <set>

#include "containers/archive/archive.hpp"
#include "containers/archive/stl_types.hpp"
#include "errors.hpp"
#include "rpc/serialize_macros.hpp"

#define MAX_PORT 65535

class printf_buffer_t;

class host_lookup_exc_t : public std::exception {
public:
    host_lookup_exc_t(const std::string &_host, int _errno_val);
    ~host_lookup_exc_t() throw () { }
    const char *what() const throw () {
        return error_string.c_str();
    }
    const std::string host;
    const int errno_val;
    const std::string error_string;
};

class invalid_address_exc_t : public std::exception {
public:
    explicit invalid_address_exc_t(const std::string &_info) :
        info(_info) { }
    ~invalid_address_exc_t() throw () { }
    const char *what() const throw () {
        return info.c_str();
    }
    const std::string info;
};

// Used for internal representation of address family, especially for serialization,
//  since the AF_* constants can't be guaranteed to be the same across architectures
enum addr_type_t {
    RDB_UNSPEC_ADDR = 0,
    RDB_IPV4_ADDR = 1,
    RDB_IPV6_ADDR = 2
};

ARCHIVE_PRIM_MAKE_RANGED_SERIALIZABLE(addr_type_t, int8_t, RDB_UNSPEC_ADDR, RDB_IPV6_ADDR);

/* ip_address_t represents an IPv4 address. */
class ip_address_t {
public:
    ip_address_t() : addr_type(RDB_UNSPEC_ADDR) { } // for deserialization

    explicit ip_address_t(const sockaddr *sa);
    explicit ip_address_t(const std::string &addr_str);

    static ip_address_t any(int address_family);

    bool operator<(const ip_address_t &x) const;
    bool operator==(const ip_address_t &x) const;
    bool operator!=(const ip_address_t &x) const { return !(*this == x); }

    std::string to_string() const;
    bool is_loopback() const;
    bool is_any() const;

    int get_address_family() const;
    bool is_ipv4() const { return addr_type == RDB_IPV4_ADDR; }
    bool is_ipv6() const { return addr_type == RDB_IPV6_ADDR; }
    bool is_ipv6_link_local() const;

    const struct in_addr &get_ipv4_addr() const;
    const struct in6_addr &get_ipv6_addr() const;
    uint32_t get_ipv6_scope_id() const;

private:
    addr_type_t addr_type;
    in_addr ipv4_addr;
    in6_addr ipv6_addr;
    uint32_t ipv6_scope_id;

    RDB_MAKE_ME_SERIALIZABLE_4(0, addr_type, ipv4_addr, ipv6_addr, ipv6_scope_id);
};

std::set<ip_address_t> hostname_to_ips(const std::string &host);
std::set<ip_address_t> get_local_ips(const std::set<ip_address_t> &filter, bool get_all);

class port_t {
public:
    explicit port_t(int _port);
    int value() const;
private:
    int value_;
    RDB_MAKE_ME_SERIALIZABLE_1(0, value_);
};

class ip_and_port_t {
public:
    ip_and_port_t();
    ip_and_port_t(const ip_address_t &_ip, port_t _port);

    bool operator < (const ip_and_port_t &other) const;
    bool operator == (const ip_and_port_t &other) const;

    const ip_address_t &ip() const;
    port_t port() const;

private:
    ip_address_t ip_;
    port_t port_;

    RDB_MAKE_ME_SERIALIZABLE_2(0, ip_, port_);
};

class host_and_port_t {
public:
    host_and_port_t();
    host_and_port_t(const std::string &_host, port_t _port);

    bool operator < (const host_and_port_t &other) const;
    bool operator == (const host_and_port_t &other) const;

    std::set<ip_and_port_t> resolve() const;

    const std::string &host() const;
    port_t port() const;

private:
    std::string host_;
    port_t port_;

    RDB_MAKE_ME_SERIALIZABLE_2(0, host_, port_);
};

class peer_address_t {
public:
    // Constructor will look up all the hosts and convert them to ip addresses
    explicit peer_address_t(const std::set<host_and_port_t> &_hosts);
    peer_address_t();

    const std::set<host_and_port_t> &hosts() const;
    const std::set<ip_and_port_t> &ips() const;

    host_and_port_t primary_host() const;

    // Two addresses are considered equal if all of their hosts match
    bool operator == (const peer_address_t &a) const;
    bool operator != (const peer_address_t &a) const;

private:
    std::set<host_and_port_t> hosts_;
    std::set<ip_and_port_t> resolved_ips;
};

void debug_print(printf_buffer_t *buf, const ip_address_t &addr);
void debug_print(printf_buffer_t *buf, const ip_and_port_t &addr);
void debug_print(printf_buffer_t *buf, const host_and_port_t &addr);
void debug_print(printf_buffer_t *buf, const peer_address_t &address);

#endif /* ARCH_ADDRESS_HPP_ */
