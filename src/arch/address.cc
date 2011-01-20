#include "arch/address.hpp"
#include <netdb.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <limits.h>
#include "errors.hpp"

ip_address_t::ip_address_t(const char *host) {

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
    int res = getaddrinfo(host, NULL, &hint, &addr_possibilities);
    guarantee(res == 0, "getaddrinfo() failed: %s", strerror(errno));

    struct sockaddr_in *addr_in = reinterpret_cast<struct sockaddr_in *>(addr_possibilities->ai_addr);
    addr = addr_in->sin_addr;

    freeaddrinfo(addr_possibilities);
}

bool ip_address_t::operator==(const ip_address_t &x) {
    return !memcmp(&addr, &x.addr, sizeof(struct in_addr));
}

bool ip_address_t::operator!=(const ip_address_t &x) {
    return memcmp(&addr, &x.addr, sizeof(struct in_addr));
}

ip_address_t ip_address_t::us() {

    char name[HOST_NAME_MAX+1];
    int res = gethostname(name, sizeof(name));
    guarantee(res == 0, "gethostname() failed: %s\n", strerror(errno));

    return ip_address_t(name);
}
