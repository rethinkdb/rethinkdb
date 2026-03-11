// Copyright 2010-2013 RethinkDB, all rights reserved.
#include "containers/auth_key.hpp"

#include <algorithm>
#include <openssl/crypto.h>

#include "containers/archive/stl_types.hpp"

auth_key_t::auth_key_t() { }

bool auth_key_t::assign_value(const std::string &new_key) {
    if (new_key.length() > static_cast<size_t>(max_length)) {
        return false;
    }

    key = new_key;
    return true;
}

RDB_IMPL_SERIALIZABLE_1_SINCE_v1_13(auth_key_t, key);


bool timing_sensitive_equals(const auth_key_t &x, const auth_key_t &y) {
    const std::string &s = x.str();
    const std::string &t = y.str();

    const size_t size = std::max(s.size(), t.size());
    int all_equal = (s.size() == t.size());
    for (size_t i = 0; i < size; ++i) {
        // Use constant-time comparison: always compare bytes, using 0 if out of bounds
        unsigned char s_byte = (i < s.size()) ? s[i] : 0;
        unsigned char t_byte = (i < t.size()) ? t[i] : 0;
        all_equal &= (s_byte == t_byte);
    }

    return all_equal;
}

