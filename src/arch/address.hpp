#ifndef ARCH_ADDRESS_HPP_
#define ARCH_ADDRESS_HPP_

#include <arpa/inet.h>   /* for `inet_ntop()` */
#include <net/if.h>
#include <netdb.h>
#include <netinet/in.h>
#include <sys/ioctl.h>

#include <string>

#include "containers/archive/archive.hpp"
#include "errors.hpp"
#include "rpc/serialize_macros.hpp"

class append_only_printf_buffer_t;

/* ip_address_t represents an IPv4 address. */
class ip_address_t {
public:
    static std::vector<ip_address_t> from_hostname(const std::string &host);
    static std::vector<ip_address_t> us();

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

void debug_print(append_only_printf_buffer_t *buf, const ip_address_t &addr);

#endif /* ARCH_ADDRESS_HPP_ */
