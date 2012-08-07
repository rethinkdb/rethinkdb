#ifndef CONTAINERS_UUID_HPP_
#define CONTAINERS_UUID_HPP_

#include <string.h>
#include <stdint.h>

#include <string>

#include "errors.hpp"

class append_only_printf_buffer_t;

class uuid_t {
public:
    uuid_t() { }

    bool is_nil() const {
        for (size_t i = 0; i < kStaticSize; ++i) {
            if (data_[i] != 0) {
                return false;
            }
        }
        return true;
    }

    uint8_t *data() { return data_; }
    const uint8_t *data() const { return data_; }

    static const size_t kStaticSize = 16;

    static size_t static_size() {
        CT_ASSERT(sizeof(uuid_t) == kStaticSize);
        return kStaticSize;
    }

private:
    uint8_t data_[kStaticSize];
};


bool operator==(const uuid_t& x, const uuid_t& y);
inline bool operator!=(const uuid_t& x, const uuid_t& y) { return !(x == y); }
bool operator<(const uuid_t& x, const uuid_t& y);

/* This does the same thing as `boost::uuids::random_generator()()`, except that
Valgrind won't complain about it. */
uuid_t generate_uuid();

// Returns boost::uuids::nil_generator()().
uuid_t nil_uuid();

void debug_print(append_only_printf_buffer_t *buf, const uuid_t& id);

std::string uuid_to_str(uuid_t id);

uuid_t str_to_uuid(const std::string&);

bool is_uuid(const std::string& str);


#endif  // CONTAINERS_UUID_HPP_
