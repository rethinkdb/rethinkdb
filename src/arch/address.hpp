#ifndef ARCH_ADDRESS_HPP_
#define ARCH_ADDRESS_HPP_

#include <netinet/in.h>

#include <string>

#include "containers/archive/archive.hpp"
#include "errors.hpp"

class append_only_printf_buffer_t;

/* ip_address_t represents an IPv4 address. */
class ip_address_t {
public:
    ip_address_t() { }
    explicit ip_address_t(const std::string &);   // Address with hostname or IP
    static ip_address_t us();

    bool operator==(const ip_address_t &x) const;   // Compare addresses
    bool operator!=(const ip_address_t &x) const;
    bool operator<(const ip_address_t &x) const;

    /* Returns IP address in `a.b.c.d` form. */
    std::string as_dotted_decimal() const;

    struct in_addr addr;

private:
    friend class write_message_t;
    void rdb_serialize(write_message_t &msg /* NOLINT */) const {
        msg.append(&addr, sizeof(addr));
    }

    friend class archive_deserializer_t;
    archive_result_t rdb_deserialize(read_stream_t *s) {
        int64_t num_read = force_read(s, &addr, sizeof(addr));
        if (num_read == -1) { return ARCHIVE_SOCK_ERROR; }
        if (num_read < int64_t(sizeof(addr))) { return ARCHIVE_SOCK_EOF; }
        rassert(num_read == sizeof(addr));
        return ARCHIVE_SUCCESS;
    }
};

void debug_print(append_only_printf_buffer_t *buf, const ip_address_t &addr);

#endif /* ARCH_ADDRESS_HPP_ */
