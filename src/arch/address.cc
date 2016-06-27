// Copyright 2010-2012 RethinkDB, all rights reserved.
#include "arch/address.hpp"

#include <errno.h>
#include <limits.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#ifdef _WIN32
#include "windows.hpp"
#include <ws2tcpip.h> // NOLINT
#else
#include <netinet/in.h>
#endif

#include <functional>

#include "arch/io/network.hpp"
#include "arch/runtime/thread_pool.hpp"
#include "logger.hpp"

host_lookup_exc_t::host_lookup_exc_t(const std::string &_host,
                                     int _res, int _errno_res)
    : host(_host),
      res(_res),
      errno_res(_errno_res) {
    std::string info =
#ifndef _WIN32
        (res == EAI_SYSTEM) ?
        strprintf("%s (errno %d)", errno_string(errno_res).c_str(), errno_res) :
#endif
        strprintf("%s (gai_errno %d)", gai_strerror(res), res);
    error_string = strprintf("getaddrinfo() failed for hostname '%s': %s",
                             host.c_str(), info.c_str());
}

/* Get our hostname as an std::string. */
std::string str_gethostname() {
#ifdef _WIN32
    const int namelen = 256; // (cf. RFC 1035)
#else
    const int namelen = _POSIX_HOST_NAME_MAX;
#endif

    std::vector<char> bytes(namelen + 1);
    bytes[namelen] = '0';

    int res = gethostname(bytes.data(), namelen);

#ifdef _WIN32
    guarantee_winerr(res == 0, "gethostname() failed");
#else
    guarantee_err(res == 0, "gethostname() failed");
#endif
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
    if (*retval != 0) {
        *errno_res = get_errno();
    }
}

/* Format an `in_addr` in dotted decimal notation. */
template <class addr_t>
std::string ip_to_string(const addr_t &addr, int address_family) {
    char buffer[INET6_ADDRSTRLEN] = { 0 };
    // On Windows, inet_ntop doesn't take a const addr
    const char *result = inet_ntop(address_family, (void*)&addr, // NOLINT
                                   buffer, INET6_ADDRSTRLEN);
    guarantee(result == buffer, "Could not format IP address");
    return std::string(buffer);
}

size_t min_sockaddr_size_for_address_family(int address_family) {
    switch (address_family) {
    case AF_INET:
        return sizeof(sockaddr_in);
    case AF_INET6:
        return sizeof(sockaddr_in6);
    default:
        return 0;
    }
}

void hostname_to_ips_internal(const std::string &host,
                              int address_family,
                              std::set<ip_address_t> *ips) {
    struct addrinfo hint;
    memset(&hint, 0, sizeof(hint));
    hint.ai_family = address_family;
    hint.ai_socktype = SOCK_STREAM;

    int res;
    int errno_res;
    struct addrinfo *addrs;
    std::function<void ()> fn =
        std::bind(do_getaddrinfo, host.c_str(), static_cast<const char*>(nullptr),
                  &hint, &addrs, &res, &errno_res);
    thread_pool_t::run_in_blocker_pool(fn);

    if (res != 0) {
        throw host_lookup_exc_t(host, res, errno_res);
    }

    guarantee(addrs);
    for (struct addrinfo *ai = addrs; ai; ai = ai->ai_next) {
        // Workaround for bug in eglibc 2.19 (used in Ubuntu 14.04) combined with nscd
        // See bug report here: https://sourceware.org/bugzilla/show_bug.cgi?id=16743
        if (ai->ai_family != address_family ||
            ai->ai_addrlen < min_sockaddr_size_for_address_family(address_family)) {
            logWRN("Invalid address family returned by `getaddrinfo` (%d), "
                   "are you using nscd with EGLIBC 2.19?", ai->ai_family);
        } else {
            ips->insert(ip_address_t(ai->ai_addr));
        }
    }

    freeaddrinfo(addrs);
}

std::set<ip_address_t> hostname_to_ips(const std::string &host) {
    std::set<ip_address_t> ips;

    // Allow an error on one lookup, but not both
    bool errored = false;

    try {
        hostname_to_ips_internal(host, AF_INET, &ips);
    } catch (const host_lookup_exc_t &ex) {
        errored = true;
    }

    try {
        hostname_to_ips_internal(host, AF_INET6, &ips);
    } catch (const host_lookup_exc_t &ex) {
        if (errored) throw;
    }

    return ips;
}

std::set<ip_address_t> get_local_ips(const std::set<ip_address_t> &filter,
                                     local_ip_filter_t filter_type) {
    std::set<ip_address_t> all_ips;
    std::set<ip_address_t> filtered_ips;

    try {
        all_ips = hostname_to_ips(str_gethostname());
    } catch (const host_lookup_exc_t &ex) {
        // Continue on, this probably means there's no DNS entry for this host
    }

#ifdef _WIN32
    // TODO WINDOWS: is this enough?
    all_ips.emplace("127.0.0.1");
#else
    // Ignore loopback addresses - those will be returned by getifaddrs, and
    // getaddrinfo is not so trustworthy.
    // See https://github.com/rethinkdb/rethinkdb/issues/2405
    for (auto it = all_ips.begin(); it != all_ips.end();) {
        auto curr = it++;
        if (!curr->is_loopback()) {
            all_ips.erase(curr);
        }
    }

    struct ifaddrs *addrs;
    int res = getifaddrs(&addrs);
    guarantee_err(res == 0,
                  "getifaddrs() failed, could not determine local network interfaces");

    for (auto *current_addr = addrs;
         current_addr != nullptr;
         current_addr = current_addr->ifa_next) {
        struct sockaddr *addr_data = current_addr->ifa_addr;
        if (addr_data == nullptr) {
            continue;
        } else if (addr_data->sa_family == AF_INET || addr_data->sa_family == AF_INET6) {
            all_ips.insert(ip_address_t(addr_data));
        }
    }
    freeifaddrs(addrs);
#endif

    // Remove any addresses that don't fit the filter
    for (auto const &ip : all_ips) {
        if (filter_type == local_ip_filter_t::ALL ||
            filter.find(ip) != filter.end() ||
            (filter_type == local_ip_filter_t::MATCH_FILTER_OR_LOOPBACK &&
             ip.is_loopback())) {
            filtered_ips.insert(ip);
        }
    }

    return filtered_ips;
}

addr_type_t sanitize_address_family(int address_family) {
    addr_type_t result;

    switch (address_family) {
    case AF_INET:
        result = RDB_IPV4_ADDR;
        break;
    case AF_INET6:
        result = RDB_IPV6_ADDR;
        break;
    default:
        crash("ip_address_t constructed with unexpected address family: %d",
              address_family);
    }

    return result;
}

ip_address_t ip_address_t::any(int address_family) {
    ip_address_t result;
    result.addr_type = sanitize_address_family(address_family);

    if (result.is_ipv4()) {
        result.ipv4_addr.s_addr = INADDR_ANY;
    } else if (result.is_ipv6()) {
        result.ipv6_addr = in6addr_any;
        result.ipv6_scope_id = 0;
    } else {
        throw invalid_address_exc_t("unknown address family");
    }

    return result;
}

ip_address_t::ip_address_t(const sockaddr *sa) {
    addr_type = sanitize_address_family(sa->sa_family);

    if (is_ipv4()) {
        ipv4_addr = reinterpret_cast<const sockaddr_in *>(sa)->sin_addr;
    } else if (is_ipv6()) {
        ipv6_addr = reinterpret_cast<const sockaddr_in6 *>(sa)->sin6_addr;
        ipv6_scope_id = reinterpret_cast<const sockaddr_in6*>(sa)->sin6_scope_id;
    } else {
        throw invalid_address_exc_t("unknown address family");
    }
}

ip_address_t::ip_address_t(const std::string &addr_str) {
    // First attempt to parse as IPv4
    if (inet_pton(AF_INET, addr_str.c_str(), &ipv4_addr) == 1) {
        addr_type = RDB_IPV4_ADDR;
    } else {
        // Try to parse as in IPv6 address, but it may contain scope ID,
        // which complicates things
        addr_type = RDB_IPV6_ADDR;
        ipv6_scope_id = 0;
        size_t percent_pos = addr_str.find_first_of('%');
        if (percent_pos != std::string::npos) {
            // There is a scope in the string, first make sure the beginning
            // is an IPv6 address
            if (inet_pton(AF_INET6,
                          addr_str.substr(0, percent_pos).c_str(),
                          &ipv6_addr) != 1) {
                throw invalid_address_exc_t(strprintf("Could not parse IP address from "
                                                      "string: '%s'", addr_str.c_str()));
            }

            // Then, use getaddrinfo to figure out the value for the scope
            std::set<ip_address_t> addresses;
            hostname_to_ips_internal(addr_str, AF_INET6, &addresses);

            bool scope_id_found = false;
            // We may get multiple ips (not sure what that case would be,
            // try to match to the one we have)
            for (auto it = addresses.begin();
                 it != addresses.end() && !scope_id_found;
                 ++it) {
                if (IN6_ARE_ADDR_EQUAL(&it->ipv6_addr, &ipv6_addr)) {
                    ipv6_scope_id = it->ipv6_scope_id;
                    scope_id_found = true;
                }
            }

            if (!scope_id_found) {
                throw invalid_address_exc_t(strprintf("Could not determine the scope "
                                                      "id of address: '%s'",
                                                      addr_str.c_str()));
            }
        } else if (inet_pton(AF_INET6, addr_str.c_str(), &ipv6_addr) != 1) {
            throw invalid_address_exc_t(strprintf("Could not parse IP address from "
                                                  "string: '%s'", addr_str.c_str()));
        }
    }
}

int ip_address_t::get_address_family() const {
    int result;

    switch (addr_type) {
    case RDB_UNSPEC_ADDR:
        result = AF_UNSPEC;
        break;
    case RDB_IPV4_ADDR:
        result = AF_INET;
        break;
    case RDB_IPV6_ADDR:
        result = AF_INET6;
        break;
    default :
        crash("unknown ip_address_t address type: %d", addr_type);
    }

    return result;
}

bool ip_address_t::is_ipv6_link_local() const {
    return addr_type == RDB_IPV6_ADDR && IN6_IS_ADDR_LINKLOCAL(&ipv6_addr);
}

const struct in_addr &ip_address_t::get_ipv4_addr() const {
    if (!is_ipv4()) {
        throw invalid_address_exc_t("get_ipv4_addr() called on a non-IPv4 ip_address_t");
    }
    return ipv4_addr;
}

const struct in6_addr &ip_address_t::get_ipv6_addr() const {
    if (!is_ipv6()) {
        throw invalid_address_exc_t("get_ipv6_addr() called on a non-IPv6 ip_address_t");
    }
    return ipv6_addr;
}

uint32_t ip_address_t::get_ipv6_scope_id() const {
    if (!is_ipv6()) {
        throw invalid_address_exc_t("get_ipv6_scope_id() called on a "
                                    "non-IPv6 ip_address_t");
    }
    return ipv6_scope_id;
}

std::string ip_address_t::to_string() const {
    std::string result;

    if (is_ipv4()) {
        result = ip_to_string(ipv4_addr, AF_INET);
    } else if (is_ipv6()) {
        result = ip_to_string(ipv6_addr, AF_INET6);
        if (IN6_IS_ADDR_LINKLOCAL(&ipv6_addr)) {
            // Add on the scope id (which is only valid for link-local addresses)
            result += strprintf("%%%d", ipv6_scope_id);
        }
    } else {
        crash("to_string called on an uninitialized ip_address_t, addr_type: %d",
              addr_type);
    }

    return result;
}

bool ip_address_t::operator == (const ip_address_t &x) const {
    if (addr_type == x.addr_type) {
        if (is_ipv4()) {
            return ipv4_addr.s_addr == x.ipv4_addr.s_addr;
        } else if (is_ipv6()) {
            return IN6_ARE_ADDR_EQUAL(&ipv6_addr, &x.ipv6_addr) &&
                ipv6_scope_id == x.ipv6_scope_id;
        }
        return true;
    }
    return false;
}

bool ip_address_t::operator < (const ip_address_t &x) const {
    if (addr_type == x.addr_type) {
        if (is_ipv4()) {
            return ipv4_addr.s_addr < x.ipv4_addr.s_addr;
        } else if (is_ipv6()) {
            int cmp_res = memcmp(&ipv6_addr, &x.ipv6_addr, sizeof(ipv6_addr));
            if (cmp_res == 0) {
                return ipv6_scope_id < x.ipv6_scope_id;
            }
            return cmp_res < 0;
        } else {
            return false;
        }
    }
    return addr_type < x.addr_type;
}

bool ip_address_t::is_loopback() const {
#ifdef _WIN32
#define IN_LOOPBACKNET 127 // from <netinet/in.h> on Linux
#endif
    if (is_ipv4()) {
        return (ntohl(ipv4_addr.s_addr) >> 24) == IN_LOOPBACKNET;
    } else if (is_ipv6()) {
        return IN6_IS_ADDR_LOOPBACK(&ipv6_addr);
    }
    return false;
}

bool ip_address_t::is_any() const {
    if (is_ipv4()) {
        return ipv4_addr.s_addr == INADDR_ANY;
    } else if (is_ipv6()) {
        return IN6_IS_ADDR_UNSPECIFIED(&ipv6_addr);
    }
    return false;
}

port_t::port_t(int _value)
    : value_(_value) { }

port_t::port_t(sockaddr const *sa) {
    switch (sa->sa_family) {
    case AF_INET:
        value_ = ntohs(reinterpret_cast<sockaddr_in const *>(sa)->sin_port);
        break;
    case AF_INET6:
        value_ = ntohs(reinterpret_cast<sockaddr_in6 const *>(sa)->sin6_port);
        break;
    default:
        crash("port_t constructed with unexpected address family: %d", sa->sa_family);
    }
}

uint16_t port_t::value() const {
    return value_;
}

std::string port_t::to_string() const {
    return std::to_string(value_);
}

ip_and_port_t::ip_and_port_t()
    : port_(0)
{ }

ip_and_port_t::ip_and_port_t(const ip_address_t &_ip, port_t _port)
    : ip_(_ip), port_(_port)
{ }

ip_and_port_t::ip_and_port_t(sockaddr const *sa)
    : ip_(sa), port_(sa)
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

std::string ip_and_port_t::to_string() const {
    if (ip_.is_ipv6()) {
        return strprintf("[%s]:%u", ip_.to_string().c_str(), port_.value());
    } else {
        return strprintf("%s:%u", ip_.to_string().c_str(), port_.value());
    }
}

bool is_similar_ip_address(const ip_and_port_t &left,
                           const ip_and_port_t &right) {
    if (left.port().value() != right.port().value() ||
        left.ip().get_address_family() != right.ip().get_address_family()) {
        return false;
    }

    if (left.ip().is_ipv4()) {
        return left.ip().get_ipv4_addr().s_addr == right.ip().get_ipv4_addr().s_addr;
    } else {
        return IN6_ARE_ADDR_EQUAL(&left.ip().get_ipv6_addr(),
                                  &right.ip().get_ipv6_addr());
    }
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
    std::set<ip_address_t> host_ips = hostname_to_ips(host_);
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
{
    for (auto it = hosts_.begin(); it != hosts_.end(); ++it) {
        std::set<ip_and_port_t> host_ips = it->resolve();
        resolved_ips.insert(host_ips.begin(), host_ips.end());
    }
}

peer_address_t::peer_address_t() { }

const std::set<host_and_port_t>& peer_address_t::hosts() const {
    return hosts_;
}

host_and_port_t peer_address_t::primary_host() const {
    guarantee(!hosts_.empty());
    return *hosts_.begin();
}

const std::set<ip_and_port_t>& peer_address_t::ips() const {
    return resolved_ips;
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

// We specifically use the version 1.14 serialization method as the "universal one".
// If that no longer works... you might want to change the caller in cluster.cc.  Or
// maybe you'd want a more explicit implementation here.
void serialize_universal(write_message_t *wm, const std::set<host_and_port_t> &x) {
    serialize<cluster_version_t::v1_14>(wm, x);
}
archive_result_t deserialize_universal(read_stream_t *s,
                                       std::set<host_and_port_t> *thing) {
    return deserialize<cluster_version_t::v1_14>(s, thing);
}

bool is_similar_peer_address(const peer_address_t &left,
                             const peer_address_t &right) {
    bool left_loopback_only = true;
    bool right_loopback_only = true;

    // We ignore any loopback addresses because they don't give us any useful information
    // Return true if any non-loopback addresses match
    for (auto left_it = left.ips().begin();
         left_it != left.ips().end(); ++left_it) {
        if (left_it->ip().is_loopback()) {
            continue;
        } else {
            left_loopback_only = false;
        }

        for (auto right_it = right.ips().begin();
             right_it != right.ips().end(); ++right_it) {
            if (right_it->ip().is_loopback()) {
                continue;
            } else {
                right_loopback_only = false;
            }

            if (is_similar_ip_address(*right_it, *left_it)) {
                return true;
            }
        }
    }

    // No non-loopback addresses matched, return true if either side was *only* loopback
    // addresses  because we can't easily prove if they are the same or different
    // addresses
    return left_loopback_only || right_loopback_only;
}

void debug_print(printf_buffer_t *buf, const ip_address_t &addr) {
    buf->appendf("%s", addr.to_string().c_str());
}

void debug_print(printf_buffer_t *buf, const ip_and_port_t &addr) {
    buf->appendf("%s:%d", addr.ip().to_string().c_str(), addr.port().value());
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
