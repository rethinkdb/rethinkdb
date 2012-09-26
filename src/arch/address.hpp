#ifndef ARCH_ADDRESS_HPP_
#define ARCH_ADDRESS_HPP_

#include <netinet/in.h>

#include <string>
#include <vector>

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

    /* Returns IP address in `a.b.c.d` form. */
    std::string primary_as_dotted_decimal() const;

    struct in_addr primary_addr() const {
        std::vector<struct in_addr>::const_iterator it = addrs.begin();
        guarantee(it != addrs.end());
        return *it;
    }
    void set_addr(const struct in_addr &addr) {
        guarantee(addrs.empty());
        add_addr(addr);
    }
    void add_addr(const struct in_addr &addr) { addrs.push_back(addr); }
    std::vector<struct in_addr> addrs;
private:
    friend class write_message_t;
    void rdb_serialize(write_message_t &msg /* NOLINT */) const {
        size_t size = addrs.size();
        msg << size;
        for (std::vector<struct in_addr>::const_iterator
                 it = addrs.begin(); it != addrs.end(); ++it) {
            guarantee(sizeof(it->s_addr) == sizeof(uint32_t));
            msg << it->s_addr;
        }
    }

    friend class archive_deserializer_t;
    archive_result_t rdb_deserialize(read_stream_t *s) {
        size_t size;
        int64_t num_read;

        num_read = force_read(s, &size, sizeof(size));
        if (num_read == -1) { return ARCHIVE_SOCK_ERROR; }
        if (num_read < int64_t(sizeof(size))) { return ARCHIVE_SOCK_EOF; }

        addrs.clear();
        for (size_t i = 0; i < size; ++i) {
            struct in_addr new_addr;
            num_read = force_read(s, &new_addr.s_addr, sizeof(new_addr.s_addr));
            if (num_read == -1) { return ARCHIVE_SOCK_ERROR; }
            if (num_read < int64_t(sizeof(new_addr.s_addr))) {return ARCHIVE_SOCK_EOF;}
            addrs.push_back(new_addr);
        }
        return ARCHIVE_SUCCESS;
    }
};

void debug_print(append_only_printf_buffer_t *buf, const ip_address_t &addr);

#endif /* ARCH_ADDRESS_HPP_ */
