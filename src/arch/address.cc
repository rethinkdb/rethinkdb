// Copyright 2010-2012 RethinkDB, all rights reserved.
#include "arch/address.hpp"

#include <errno.h>
#include <limits.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include "errors.hpp"
#include <boost/bind.hpp>

#include "arch/io/network.hpp"
#include "arch/runtime/thread_pool.hpp"

/* Get our hostname as an std::string. */
std::string str_gethostname() {
    const int namelen = _POSIX_HOST_NAME_MAX;

    std::vector<char> bytes(namelen + 1);
    bytes[namelen] = '0';

    int res = gethostname(bytes.data(), namelen);
    guarantee_err(res == 0, "gethostname() failed");
    return std::string(bytes.data());
}

void do_getaddrinfo(const char *node,
                    const char *service,
                    const struct addrinfo *hints,
                    struct addrinfo **res,
                    int *retval,
                    int *errno_res) {
    *errno_res = 0;
    *retval = getaddrinfo(node, service, hints, res);
    if (*retval < 0) {
        *errno_res = errno;
    }
}

/* Format an `in_addr` in dotted deciaml notation. */
std::string addr_as_dotted_decimal(struct in_addr addr) {
    char buffer[INET_ADDRSTRLEN + 1] = { 0 };
    const char *result = inet_ntop(AF_INET, reinterpret_cast<const void*>(&addr),
        buffer, INET_ADDRSTRLEN);
    guarantee(result == buffer, "Could not format IP address");
    return std::string(buffer);
}

std::set<ip_address_t> ip_address_t::from_hostname(const std::string &host) {
    std::set<ip_address_t> ips;
    struct addrinfo hint;
    memset(&hint, 0, sizeof(hint));
    hint.ai_family = AF_INET;
    hint.ai_socktype = SOCK_STREAM;

    int res;
    int errno_res;
    struct addrinfo *addrs;
    boost::function<void ()> fn =
        boost::bind(do_getaddrinfo, host.c_str(), static_cast<const char*>(NULL),
                    &hint, &addrs, &res, &errno_res);
    thread_pool_t::run_in_blocker_pool(fn);

    if (res != 0) {
        throw host_lookup_exc_t(host, errno_res);
    }

    guarantee(addrs);
    for (struct addrinfo *ai = addrs; ai; ai = ai->ai_next) {
        ips.insert(ip_address_t(ai));
    }

    freeaddrinfo(addrs);
    return ips;
}

bool check_address_filter(ip_address_t addr, const std::set<ip_address_t> &filter) {
    // The filter is a whitelist, loopback addresses are always whitelisted
    return filter.find(addr) != filter.end() || addr.is_loopback();
}

std::set<ip_address_t> ip_address_t::get_local_addresses(const std::set<ip_address_t> &filter, bool get_all) {
    std::set<ip_address_t> all_ips;
    std::set<ip_address_t> filtered_ips;

    try {
        all_ips = from_hostname(str_gethostname());
    } catch (const host_lookup_exc_t &ex) {
        // Continue on, this probably means there's no DNS entry for this host
    }

    struct ifaddrs *addrs;
    int res = getifaddrs(&addrs);
    guarantee_err(res == 0, "getifaddrs() failed, could not determine local network interfaces");

    for (struct ifaddrs *current_addr = addrs; current_addr != NULL; current_addr = current_addr->ifa_next) {
        struct sockaddr *addr_data = current_addr->ifa_addr;
        if (addr_data == NULL) {
            continue;
        } else if (addr_data->sa_family == AF_INET) {
            all_ips.insert(ip_address_t(addr_data));
        }
    }
    freeifaddrs(addrs);

    // Remove any addresses that don't fit the filter
    for (std::set<ip_address_t>::const_iterator it = all_ips.begin(); it != all_ips.end(); ++it) {
        if (get_all || check_address_filter(*it, filter)) {
            filtered_ips.insert(*it);
        }
    }

    return filtered_ips;
}

std::string ip_address_t::as_dotted_decimal() const {
    return addr_as_dotted_decimal(get_addr());
}

bool ip_address_t::is_loopback() const {
    return (ntohl(s_addr) >> 24) == 127;
}

port_t::port_t(int _value) : value_(_value) {
    guarantee(value_ <= MAX_PORT);
}

int port_t::value() const {
    return value_;
}

ip_and_port_t::ip_and_port_t() :
    port_(0)
{ }

ip_and_port_t::ip_and_port_t(const ip_address_t &_ip, port_t _port) :
    ip_(_ip), port_(_port)
{ }

bool ip_and_port_t::operator < (const ip_and_port_t &other) const {
    if (ip_ == other.ip_) {
        return port_.value() < other.port_.value();
    }
    return ip_ < other.ip_;
}

bool ip_and_port_t::operator == (const ip_and_port_t &other) const {
    return ip_ == other.ip_ && port_.value() == other.port_.value();
}

const ip_address_t & ip_and_port_t::ip() const {
    return ip_;
}

port_t ip_and_port_t::port() const {
    return port_;
}

host_and_port_t::host_and_port_t() :
    port_(0)
{ }

host_and_port_t::host_and_port_t(const std::string& _host, port_t _port) :
    host_(_host), port_(_port)
{ }

bool host_and_port_t::operator < (const host_and_port_t &other) const {
    if (host_ == other.host_) {
        return port_.value() < other.port_.value();
    }
    return host_ < other.host_;
}

bool host_and_port_t::operator == (const host_and_port_t &other) const {
    return host_ == other.host_ && port_.value() == other.port_.value();
}

std::set<ip_and_port_t> host_and_port_t::resolve() const {
    std::set<ip_and_port_t> result;
    std::set<ip_address_t> host_ips = ip_address_t::from_hostname(host_);
    for (auto jt = host_ips.begin(); jt != host_ips.end(); ++jt) {
        result.insert(ip_and_port_t(*jt, port_));
    }
    return result;
}

const std::string & host_and_port_t::host() const {
    return host_;
}

port_t host_and_port_t::port() const {
    return port_;
}

peer_address_t::peer_address_t(const std::set<host_and_port_t> &_hosts) :
    hosts_(_hosts)
{ }

peer_address_t::peer_address_t()
{ }

const std::set<host_and_port_t>& peer_address_t::hosts() const {
    return hosts_;
}

host_and_port_t peer_address_t::primary_host() const {
    guarantee(!hosts_.empty());
    return *hosts_.begin();
}

void peer_address_t::resolve() {
    if (!resolved_ips) {
        resolved_ips = std::set<ip_and_port_t>();
        for (auto it = hosts_.begin(); it != hosts_.end(); ++it) {
            std::set<ip_and_port_t> host_ips = it->resolve();
            resolved_ips->insert(host_ips.begin(), host_ips.end());
        }
    }
}

const std::set<ip_and_port_t>& peer_address_t::ips() const {
    guarantee(resolved_ips);
    return resolved_ips.get();
}

// Two addresses are considered equal if all of their hosts match
bool peer_address_t::operator == (const peer_address_t &a) const {
    std::set<host_and_port_t>::const_iterator it, jt;
    for (it = hosts_.begin(), jt = a.hosts_.begin();
         it != hosts_.end() && jt != a.hosts_.end(); ++it, ++jt) {
        if (it->port().value() != jt->port().value() ||
            it->host() != jt->host()) {
            return false;
        }
    }
    return true;
}

bool peer_address_t::operator != (const peer_address_t &a) const {
    return !(*this == a);
}

void peer_address_t::rdb_serialize(write_message_t &msg /* NOLINT */) const {
    msg << hosts_;
}

archive_result_t peer_address_t::rdb_deserialize(read_stream_t *s) {
    archive_result_t res = ARCHIVE_SUCCESS;
    res = deserialize(s, &hosts_);
    if (res) { return res; }

    // Resolved addresses are not valid on the other side
    resolved_ips.reset();

    return res;
}

void debug_print(printf_buffer_t *buf, const ip_address_t &addr) {
    buf->appendf("%s", addr.as_dotted_decimal().c_str());
}

void debug_print(printf_buffer_t *buf, const ip_and_port_t &addr) {
    buf->appendf("%s:%d", addr.ip().as_dotted_decimal().c_str(), addr.port().value());
}

void debug_print(printf_buffer_t *buf, const host_and_port_t &addr) {
    buf->appendf("%s:%d", addr.host().c_str(), addr.port().value());
}

void debug_print(printf_buffer_t *buf, const peer_address_t &address) {
    buf->appendf("peer_address [");
    const std::set<host_and_port_t> &hosts = address.hosts();
    for (auto it = hosts.begin(); it != hosts.end(); ++it) {
        if (it != hosts.begin()) buf->appendf(", ");
        debug_print(buf, *it);
    }
    buf->appendf("]");
}
