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

void debug_print(printf_buffer_t *buf, const ip_address_t &addr) {
    buf->appendf("%s", addr.as_dotted_decimal().c_str());
}
