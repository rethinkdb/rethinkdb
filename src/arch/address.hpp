// Copyright 2010-2013 RethinkDB, all rights reserved.
#ifndef ARCH_ADDRESS_HPP_
#define ARCH_ADDRESS_HPP_

#ifndef _WIN32
#include <arpa/inet.h>   /* for `inet_ntop()` */
#include <net/if.h>
#include <netdb.h>
#include <netinet/in.h>
#include <sys/ioctl.h>
#else
#include <In6addr.h>
#endif

#include <algorithm>
#include <string>
#include <set>
#include <vector>

#include "containers/archive/archive.hpp"
#include "containers/archive/stl_types.hpp"
#include "errors.hpp"
#include "rpc/serialize_macros.hpp"

#define MAX_PORT 65535

class printf_buffer_t;

class host_lookup_exc_t : public std::exception {
public:
    host_lookup_exc_t(const std::string &_host, int _res, int _errno_res);
    ~host_lookup_exc_t() throw () { }
    const char *what() const throw () {
        return error_string.c_str();
    }
    const std::string host;
    const int res;
    const int errno_res;
    std::string error_string;
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

    RDB_MAKE_ME_SERIALIZABLE_4(ip_address_t,
        addr_type, ipv4_addr, ipv6_addr, ipv6_scope_id);

private:
    addr_type_t addr_type;
    in_addr ipv4_addr;
    in6_addr ipv6_addr;
    uint32_t ipv6_scope_id;
};

std::string str_gethostname();

std::set<ip_address_t> hostname_to_ips(const std::string &host);

enum class local_ip_filter_t {
    MATCH_FILTER,
    MATCH_FILTER_OR_LOOPBACK,
    ALL
};

std::set<ip_address_t> get_local_ips(const std::set<ip_address_t> &filter,
                                     local_ip_filter_t filter_type);

class port_t {
public:
    static constexpr int max_port = 65535;

    explicit port_t(int _port);
    explicit port_t(sockaddr const *);

    int value() const;

    std::string to_string() const;

    RDB_MAKE_ME_SERIALIZABLE_1(port_t, value_);

private:
    int value_;
};

class ip_and_port_t {
public:
    ip_and_port_t();
    ip_and_port_t(const ip_address_t &_ip, port_t _port);
    explicit ip_and_port_t(sockaddr const *);

    bool operator < (const ip_and_port_t &other) const;
    bool operator == (const ip_and_port_t &other) const;

    const ip_address_t &ip() const;
    port_t port() const;

    std::string to_string() const;

    RDB_MAKE_ME_SERIALIZABLE_2(ip_and_port_t, ip_, port_);

private:
    ip_address_t ip_;
    port_t port_;
};

// This implementation is used over operator == because we want to ignore different scope
// ids in the case of IPv6
bool is_similar_ip_address(const ip_and_port_t &left,
                           const ip_and_port_t &right);

class host_and_port_t {
public:
    host_and_port_t();
    host_and_port_t(const std::string &_host, port_t _port);

    bool operator < (const host_and_port_t &other) const;
    bool operator == (const host_and_port_t &other) const;

    std::set<ip_and_port_t> resolve() const;

    const std::string &host() const;
    port_t port() const;

    RDB_MAKE_ME_SERIALIZABLE_2(host_and_port_t, host_, port_);

private:
    std::string host_;
    port_t port_;
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

void serialize_universal(write_message_t *wm, const std::set<host_and_port_t> &x);
archive_result_t deserialize_universal(read_stream_t *s,
                                       std::set<host_and_port_t> *thing);

bool is_similar_peer_address(const peer_address_t &left,
                             const peer_address_t &right);

/* TODO: This should either be a `std::set<peer_address_t>` with a custom comparator, or
a generic container type that contains a set of anything equality-comparable. */
class peer_address_set_t {
public:
    size_t erase(const peer_address_t &addr) {
        size_t erased = 0;
        for (auto it = vec.begin(); it != vec.end(); ++it) {
            if (*it == addr) {
                vec.erase(it);
                ++erased;
                break;
            }
        }
        return erased;
    }
    size_t size() const { return vec.size(); }
    typedef std::vector<peer_address_t>::const_iterator iterator;
    iterator begin() const { return vec.begin(); }
    iterator end() const { return vec.end(); }
    iterator find(const peer_address_t &addr) const {
        return std::find(vec.begin(), vec.end(), addr);
    }
    iterator insert(const peer_address_t &addr) {
        guarantee(find(addr) == vec.end());
        return vec.insert(vec.end(), addr);
    }
    bool empty() const { return vec.empty(); }
private:
    std::vector<peer_address_t> vec;
};

void debug_print(printf_buffer_t *buf, const ip_address_t &addr);
void debug_print(printf_buffer_t *buf, const ip_and_port_t &addr);
void debug_print(printf_buffer_t *buf, const host_and_port_t &addr);
void debug_print(printf_buffer_t *buf, const peer_address_t &address);

#endif /* ARCH_ADDRESS_HPP_ */
