#include "arch/address.hpp"

#include <arpa/inet.h>   /* for `inet_ntop()` */
#include <errno.h>
#include <limits.h>
#include <net/if.h>
#include <netdb.h>
#include <stdlib.h>
#include <string.h>
#include <string.h>
#include <sys/ioctl.h>
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

    struct addrinfo *addr_possibilities; //ALLOC 0

    // Because getaddrinfo may block, send it to a blocker thread and give up execution in the meantime
    int res;
    boost::function<void ()> fn =
        boost::bind(do_getaddrinfo, host.c_str(), static_cast<const char*>(NULL), &hint, &addr_possibilities, &res);
    thread_pool_t::run_in_blocker_pool(fn);
    guarantee_err(res == 0, "getaddrinfo() failed");

    std::string our_hostname = str_gethostname();
    for (struct addrinfo *ai = addr_possibilities; ai; ai = ai->ai_next) {
        struct sockaddr_in *addr_in = reinterpret_cast<struct sockaddr_in *>(ai->ai_addr);
        addrs.push_back(addr_in->sin_addr);
        if (host != our_hostname) break; //We only want one IP for our peers
    }

    if (host == our_hostname) {
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
        for (struct if_nameindex *it = ifs; it->if_name; ++it) {
            struct ifreq req;
            strncpy(req.ifr_name, it->if_name, IFNAMSIZ);
            if (req.ifr_name[IFNAMSIZ-1] != 0) { // make sure the strncpy didn't truncate
                req.ifr_name[IFNAMSIZ-1] = 0;
                guarantee(strlen(req.ifr_name) < (IFNAMSIZ-1));
            }
            res = ioctl(fd, SIOCGIFADDR, &req); //SIOCGIFADDR : Socket IO Control Get InterFace ADDRess (I think?)
            guarantee(res >= 0);
            addrs.push_back(reinterpret_cast<struct sockaddr_in *>(&req.ifr_addr)->sin_addr);
        }
        if_freenameindex(ifs); //FREE 2
        res = close(fd); //FREE 1
        guarantee(res == 0 || (res == -1 && errno == EINTR));
    }
    freeaddrinfo(addr_possibilities); //FREE 0
}

bool ip_address_t::operator==(const ip_address_t &x) const {
    std::vector<struct in_addr>::const_iterator it, itx;
    for (it = addrs.begin(); it != addrs.end(); ++it) {
        for (itx = x.addrs.begin(); itx != x.addrs.end(); ++itx) {
            if (it->s_addr == itx->s_addr) return true;
        }
    }
    return false;
}

bool ip_address_t::operator!=(const ip_address_t &x) const {
    return !(*this == x);
}

ip_address_t ip_address_t::us() {
    return ip_address_t(str_gethostname());
}

std::string ip_address_t::primary_as_dotted_decimal() const {
    return addr_as_dotted_decimal(primary_addr());
}

void debug_print(append_only_printf_buffer_t *buf, const ip_address_t &addr) {
    buf->appendf("[");
    for (std::vector<struct in_addr>::const_iterator
             it = addr.addrs.begin(); it != addr.addrs.end(); ++it) {
        if (it != addr.addrs.begin()) buf->appendf(", ");
        buf->appendf("%s", addr_as_dotted_decimal(*it).c_str());
    }
    buf->appendf("]");
}
