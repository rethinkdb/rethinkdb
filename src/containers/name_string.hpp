// Copyright 2010-2013 RethinkDB, all rights reserved.
#ifndef CONTAINERS_NAME_STRING_HPP_
#define CONTAINERS_NAME_STRING_HPP_

#include <string>

#include "rdb_protocol/datum_string.hpp"
#include "rpc/serialize_macros.hpp"

// The kind of string that can only contain either the empty string or acceptable names for
// things.  You may only assign non-empty strings.

class name_string_t {
public:
    // Initializes to the empty string.
    name_string_t();

    // Succeeds on valid non-empty strings.
    MUST_USE bool assign_value(const std::string &s);
    MUST_USE bool assign_value(const datum_string_t &s);

    // Meant for use with hard-coded names.
    static name_string_t guarantee_valid(const char *name);

    const std::string& str() const { return str_; }

    // TODO: We should get rid of this, and code that uses this is known to be untrustworthy.
    bool empty() const { return str_.empty(); }
    const char *c_str() const { return str_.c_str(); }

    RDB_DECLARE_ME_SERIALIZABLE;

    static const char *const valid_char_msg;

private:
    std::string str_;
};

RDB_SERIALIZE_OUTSIDE(name_string_t);

inline bool operator==(const name_string_t& x, const name_string_t& y) {
    return x.str() == y.str();
}

inline bool operator!=(const name_string_t& x, const name_string_t& y) {
    return !(x == y);
}

inline bool operator<(const name_string_t& x, const name_string_t& y) {
    return x.str() < y.str();
}

void debug_print(printf_buffer_t *buf, const name_string_t& s);

#endif  // CONTAINERS_NAME_STRING_HPP_
