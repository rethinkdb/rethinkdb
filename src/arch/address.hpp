#ifndef ARCH_ADDRESS_HPP_
#define ARCH_ADDRESS_HPP_

#include "utils.hpp" // (for UNUSED macro)
#include <boost/serialization/access.hpp>
#include <boost/serialization/binary_object.hpp>
#include <netinet/in.h>

#include "containers/archive/archive.hpp"

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
    friend class boost::serialization::access;
    template<class Archive> void serialize(Archive & ar, UNUSED const unsigned int version) {
        // Not really platform independent...
        boost::serialization::binary_object bin_addr(&addr, sizeof(in_addr));
        ar & bin_addr;
    }

    friend class write_message_t;
    void rdb_serialize(write_message_t &msg) const {
        msg.append(&addr, sizeof(addr));
    }
};

#endif /* ARCH_ADDRESS_HPP_ */
