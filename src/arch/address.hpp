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
#include "utils.hpp"
#include "rpc/serialize_macros.hpp"

#define MAX_PORT 65535

class printf_buffer_t;

class host_lookup_exc_t : public std::exception {
public:
    host_lookup_exc_t(const std::string& _host, int _errno_val) :
        host(_host),
        errno_val(_errno_val),
        error_string(strprintf("getaddrinfo() failed for hostname: %s, errno: %d", host.c_str(), errno_val)) { }
    const char *what() const throw () {
        return error_string.c_str();
    }
    ~host_lookup_exc_t() throw () { }
    const std::string host;
    const int errno_val;
    const std::string error_string;
};

/* ip_address_t represents an IPv4 address. */
class ip_address_t {
public:
    static std::set<ip_address_t> from_hostname(const std::string &host);
    static std::set<ip_address_t> get_local_addresses(const std::set<ip_address_t> &filter, bool get_all);

    ip_address_t() { } //for deserialization

    // These are basically the only things you should ever want to get an address from.
    explicit ip_address_t(struct ifreq *ifr)
        : s_addr(reinterpret_cast<struct sockaddr_in *>(&ifr->ifr_addr)->sin_addr.s_addr) { }
    explicit ip_address_t(struct addrinfo *ai)
        : s_addr(reinterpret_cast<struct sockaddr_in *>(ai->ai_addr)->sin_addr.s_addr) { }
    explicit ip_address_t(struct sockaddr *s)
        : s_addr(reinterpret_cast<struct sockaddr_in *>(s)->sin_addr.s_addr) { }
    explicit ip_address_t(struct sockaddr_in *sin) : s_addr(sin->sin_addr.s_addr) { }
    explicit ip_address_t(struct in_addr addr) : s_addr(addr.s_addr) { }

    bool operator==(const ip_address_t &x) const {
        return get_addr().s_addr == x.get_addr().s_addr;
    }
    bool operator!=(const ip_address_t &x) const {
        return get_addr().s_addr != x.get_addr().s_addr;
    }
    bool operator<(const ip_address_t &x) const {
        return get_addr().s_addr < x.get_addr().s_addr;
    }

    /* Returns IP address in `a.b.c.d` form. */
    std::string as_dotted_decimal() const;

    bool is_loopback() const;

    struct in_addr get_addr() const {
        struct in_addr addr;
        addr.s_addr = s_addr;
        return addr;
    }
    void set_addr(struct in_addr addr) { s_addr = addr.s_addr; }
private:
    uint32_t s_addr; //should be used as an in_addr
    RDB_MAKE_ME_SERIALIZABLE_1(s_addr);
};

class port_t {
public:
    explicit port_t(int _port);
    int value() const;
private:
    int value_;
    RDB_MAKE_ME_SERIALIZABLE_1(value_);
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

    RDB_MAKE_ME_SERIALIZABLE_2(ip_, port_);
};

class host_and_port_t {
public:
    host_and_port_t();
    host_and_port_t(const std::string& _host, port_t _port);

    bool operator < (const host_and_port_t &other) const;
    bool operator == (const host_and_port_t &other) const;

    std::set<ip_and_port_t> resolve() const;

    const std::string &host() const;
    port_t port() const;

private:
    std::string host_;
    port_t port_;

    RDB_MAKE_ME_SERIALIZABLE_2(host_, port_);
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
