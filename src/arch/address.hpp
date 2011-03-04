
#ifndef __ARCH_ADDRESS_HPP__
#define __ARCH_ADDRESS_HPP__

#include <netinet/in.h>

/* ip_address_t represents an IPv4 address. */
struct ip_address_t {
    friend class linux_tcp_conn_t;

    ip_address_t() { }
    ip_address_t(const char *);   // Address with hostname or IP
    ip_address_t(uint32_t);
    static ip_address_t us();

    bool operator==(const ip_address_t &x);   // Compare addresses
    bool operator!=(const ip_address_t &x);

public:
    uint32_t ip_as_uint32();

private:
    struct in_addr addr;
};

#endif /* __ARCH_ADDRESS_HPP__ */
