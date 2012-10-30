// Copyright 2010-2012 RethinkDB, all rights reserved.
#include "arch/address.hpp"

#include <errno.h>
#include <limits.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include "errors.hpp"
#include <boost/bind.hpp>

#include "arch/arch.hpp"

/* Get our hostname as an std::string. */
std::string str_gethostname() {
    char name[HOST_NAME_MAX+1];
    int res = gethostname(name, sizeof(name));
    guarantee(res == 0, "gethostname() failed: %s\n", strerror(errno));
    return std::string(name);
}

void do_getaddrinfo(const char *node, const char *service, const struct addrinfo *hints, struct addrinfo **res, int *retval) {
    *retval = getaddrinfo(node, service, hints, res);
}

/* Format an `in_addr` in dotted deciaml notation. */
std::string addr_as_dotted_decimal(struct in_addr addr) {
    char buffer[INET_ADDRSTRLEN + 1] = { 0 };
    const char *result = inet_ntop(AF_INET, reinterpret_cast<const void*>(&addr),
        buffer, INET_ADDRSTRLEN);
    guarantee(result == buffer, "Could not format IP address");
    return std::string(buffer);
}

std::vector<ip_address_t> ip_address_t::from_hostname(const std::string &host) {
    std::vector<ip_address_t> ips;
    struct addrinfo hint;
    memset(&hint, 0, sizeof(hint));
    hint.ai_family = AF_INET;
    hint.ai_socktype = SOCK_STREAM;

    int res;
    struct addrinfo *addrs;
    boost::function<void ()> fn =
        boost::bind(do_getaddrinfo, host.c_str(), static_cast<const char*>(NULL),
                    &hint, &addrs, &res);
    thread_pool_t::run_in_blocker_pool(fn); //ALLOC 0
    guarantee_err(res == 0, "getaddrinfo() failed");
    guarantee(addrs);
    for (struct addrinfo *ai = addrs; ai; ai = ai->ai_next) {
        ips.push_back(ip_address_t(ai));
    }

    if (host == str_gethostname()) {
        /* If we're creating the `ip_address_t` for the machine we're currently
           on, we also want to add all of the IP addresses of the interfaces.
           We want to do this IN ADDITION TO the per-hostname lookup above
           because there might be shenanigans.  (For example, on Debian/Ubuntu
           the current hostname is sometimes mapped to 127.0.1.1 in /etc/hosts,
           which resolves correctly over the loopback interface, but doesn't
           show up in the interfaces). */
        int fd = socket(AF_INET, SOCK_STREAM, 0); //ALLOC 1
        guarantee(fd != -1);
        struct if_nameindex *ifs = if_nameindex(); //ALLOC 2
        guarantee(ifs);
        for (struct if_nameindex *it = ifs; it->if_name != NULL; ++it) {
            struct ifreq req;
            strncpy(req.ifr_name, it->if_name, IFNAMSIZ);
            if (req.ifr_name[IFNAMSIZ-1] != 0) { // make sure the strncpy didn't truncate
                req.ifr_name[IFNAMSIZ-1] = 0;
                guarantee(strlen(req.ifr_name) < (IFNAMSIZ-1));
            }
            //SIOCGIFADDR : Socket IO Control Get InterFace ADDRess (I think?)
            res = ioctl(fd, SIOCGIFADDR, &req);
            if (res >= 0) {
                ips.push_back(ip_address_t(&req));
            } else {
                // TODO: perhaps handle individual errnos
                // It is possible to get here with errno EADDRNOTAVAIL if the interface is not configured
            }
        }
        if_freenameindex(ifs); //FREE 2
        res = close(fd); //FREE 1
        guarantee(res == 0 || (res == -1 && errno == EINTR));
    }
    freeaddrinfo(addrs); //FREE 0
    return ips;
}

std::vector<ip_address_t> ip_address_t::us() {
    return from_hostname(str_gethostname());
}

std::string ip_address_t::as_dotted_decimal() const {
    return addr_as_dotted_decimal(get_addr());
}

void debug_print(append_only_printf_buffer_t *buf, const ip_address_t &addr) {
    buf->appendf("%s", addr.as_dotted_decimal().c_str());
}
