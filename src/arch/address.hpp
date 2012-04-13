#ifndef ARCH_ADDRESS_HPP_
#define ARCH_ADDRESS_HPP_

#include <netinet/in.h>

#include <string>

#include "containers/archive/archive.hpp"
#include "errors.hpp"

/* ip_address_t represents an IPv4 address. */
struct ip_address_t {
    ip_address_t() { }
    explicit ip_address_t(const std::string &);   // Address with hostname or IP
    static ip_address_t us();

    bool operator==(const ip_address_t &x) const;   // Compare addresses
    bool operator!=(const ip_address_t &x) const;

public:
    /* Returns IP address in `a.b.c.d` form. */
    std::string as_dotted_decimal();

public:
    struct in_addr addr;

private:
    friend class write_message_t;
    void rdb_serialize(write_message_t &msg) const {
        msg.append(&addr, sizeof(addr));
    }

    friend class archive_deserializer_t;
    int rdb_deserialize(read_stream_t *s) {
        int64_t num_read = force_read(s, &addr, sizeof(addr));
        if (num_read == -1) { return -1; }
        if (num_read < int64_t(sizeof(addr))) { return -2; }
        rassert(num_read == sizeof(addr));
        return 0;
    }
};

#endif /* ARCH_ADDRESS_HPP_ */
