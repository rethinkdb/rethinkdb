#include "arch/address.hpp"

#include <arpa/inet.h>   /* for `inet_ntop()` */
#include <netdb.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <limits.h>
#include <unistd.h>

#include "errors.hpp"
#include <boost/bind.hpp>

#include "arch/arch.hpp"

void do_getaddrinfo(const char *node, const char *service, const struct addrinfo *hints, struct addrinfo **res, int *retval) {
    *retval = getaddrinfo(node, service, hints, res);
}

ip_address_t::ip_address_t(const std::string &host) {

    /* Use hint to make sure we get a IPv4 address that we can use with TCP */
    struct addrinfo hint;
    hint.ai_flags = 0;
    hint.ai_family = AF_INET;
    hint.ai_socktype = SOCK_STREAM;
    hint.ai_protocol = 0;
    hint.ai_addrlen = 0;   // These last four must be 0/NULL
    hint.ai_addr = NULL;
    hint.ai_canonname = NULL;
    hint.ai_next = NULL;

    struct addrinfo *addr_possibilities;

    // Because getaddrinfo may block, send it to a blocker thread and give up execution in the meantime
    int res;
    boost::function<void ()> fn = boost::bind(do_getaddrinfo, host.c_str(), static_cast<const char*>(NULL), &hint, &addr_possibilities, &res);
    thread_pool_t::run_in_blocker_pool(fn);
    guarantee_err(res == 0, "getaddrinfo() failed");

    struct sockaddr_in *addr_in = reinterpret_cast<struct sockaddr_in *>(addr_possibilities->ai_addr);
    addr = addr_in->sin_addr;

    freeaddrinfo(addr_possibilities);
}

bool ip_address_t::operator==(const ip_address_t &x) const {
    return !memcmp(&addr, &x.addr, sizeof(struct in_addr));
}

bool ip_address_t::operator!=(const ip_address_t &x) const {
    return memcmp(&addr, &x.addr, sizeof(struct in_addr));
}

bool ip_address_t::operator<(const ip_address_t &x) const {
    return memcmp(&addr, &x.addr, sizeof(struct in_addr)) < 0;
}

ip_address_t ip_address_t::us() {

    char name[HOST_NAME_MAX+1];
    int res = gethostname(name, sizeof(name));
    guarantee(res == 0, "gethostname() failed: %s\n", strerror(errno));

    return ip_address_t(name);
}

std::string ip_address_t::as_dotted_decimal() const {
    char buffer[INET_ADDRSTRLEN + 1] = { 0 };
    const char *result = inet_ntop(AF_INET, reinterpret_cast<const void*>(&addr),
        buffer, INET_ADDRSTRLEN);
    guarantee(result == buffer, "Could not format IP address");
    return std::string(buffer);
}

void debug_print(append_only_printf_buffer_t *buf, const ip_address_t &addr) {
    buf->appendf("%s", addr.as_dotted_decimal().c_str());
}
