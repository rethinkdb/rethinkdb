// Copyright 2010-2013 RethinkDB, all rights reserved.
#ifndef CONTAINERS_UUID_HPP_
#define CONTAINERS_UUID_HPP_

#include <string.h>
#include <stdint.h>

#include <string>

#include "errors.hpp"

class printf_buffer_t;

// uuid_t is defined on Darwin.  I have given up on what to name it.  Please
// don't use guid_t, for it has a Windowsian connotation and we might run into
// the same sort of problem from that.
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

void debug_print(printf_buffer_t *buf, const uuid_u& id);

std::string uuid_to_str(uuid_u id);

uuid_u str_to_uuid(const std::string &str);

MUST_USE bool str_to_uuid(const std::string &str, uuid_u *out);

bool is_uuid(const std::string& str);


typedef uuid_u namespace_id_t;
typedef uuid_u database_id_t;
typedef uuid_u machine_id_t;
typedef uuid_u datacenter_id_t;
typedef uuid_u backfill_session_id_t;
typedef uuid_u branch_id_t;
typedef uuid_u reactor_activity_id_t;


#endif  // CONTAINERS_UUID_HPP_
