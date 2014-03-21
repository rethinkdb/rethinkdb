// Copyright 2010-2013 RethinkDB, all rights reserved.
#ifndef CONTAINERS_AUTH_KEY_HPP_
#define CONTAINERS_AUTH_KEY_HPP_

#include <string>

#include "http/json/json_adapter.hpp"
#include "rpc/serialize_macros.hpp"

// A type to enforce authorization key length

class auth_key_t {
public:
    // Initializes to the empty string.
    auth_key_t();

    // Returns true if the new key is of a valid length
    MUST_USE bool assign_value(const std::string &new_key);

    const std::string &str() const { return key; }

    static const int32_t max_length = 2048;

private:
    std::string key;

    RDB_DECLARE_ME_SERIALIZABLE;
};

bool timing_sensitive_equals(const auth_key_t &x, const auth_key_t &y);

inline bool operator==(const auth_key_t &x, const auth_key_t &y) {
    // Might as well use timing_sensitive_equals this in case of accidental misuse.
    return timing_sensitive_equals(x, y);
}

inline bool operator!=(const auth_key_t &x, const auth_key_t &y) {
    return !(x == y);
}

inline bool operator<(const auth_key_t &x, const auth_key_t &y) {
    return x.str() < y.str();
}

// ctx-less json adapter concept for auth_key_t
json_adapter_if_t::json_adapter_map_t get_json_subfields(auth_key_t *target);
cJSON *render_as_json(auth_key_t *target);
void apply_json_to(cJSON *change, auth_key_t *target);

#endif  // CONTAINERS_AUTH_KEY_HPP_
