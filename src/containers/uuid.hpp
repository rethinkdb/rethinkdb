// Copyright 2010-2012 RethinkDB, all rights reserved.
#ifndef CONTAINERS_UUID_HPP_
#define CONTAINERS_UUID_HPP_

#include <string.h>
#include <stdint.h>

#include <string>

#include "errors.hpp"

class append_only_printf_buffer_t;

// TODO(OSX) uuid_u is funny but not up to our naming scheme.  (uuid_t is reserved by Darwin.)
class uuid_u {
public:
    uuid_u();

    bool is_unset() const;
    bool is_nil() const;

    static const size_t kStaticSize = 16;
    static const size_t kStringSize = 2 * kStaticSize + 4;  // hexadecimal, 4 hyphens
    static size_t static_size() {
        CT_ASSERT(sizeof(uuid_u) == kStaticSize);
        return kStaticSize;
    }

    uint8_t *data() { return data_; }
    const uint8_t *data() const { return data_; }
private:
    uint8_t data_[kStaticSize];
};


bool operator==(const uuid_u& x, const uuid_u& y);
inline bool operator!=(const uuid_u& x, const uuid_u& y) { return !(x == y); }
bool operator<(const uuid_u& x, const uuid_u& y);

/* This does the same thing as `boost::uuids::random_generator()()`, except that
Valgrind won't complain about it. */
uuid_u generate_uuid();

// Returns boost::uuids::nil_generator()().
uuid_u nil_uuid();

void debug_print(append_only_printf_buffer_t *buf, const uuid_u& id);

std::string uuid_to_str(uuid_u id);

uuid_u str_to_uuid(const std::string &str);

MUST_USE bool str_to_uuid(const std::string &str, uuid_u *out);

bool is_uuid(const std::string& str);


#endif  // CONTAINERS_UUID_HPP_
