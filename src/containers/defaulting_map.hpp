// Copyright 2010-2013 RethinkDB, all rights reserved.
#ifndef CONTAINERS_DEFAULTING_MAP_HPP_
#define CONTAINERS_DEFAULTING_MAP_HPP_

#include <map>

/* A `defaulting_map_t` is constructed with a default value.
 * You can request the value of arbitrary keys through `defaulting_map_t::get()`.
 * If a value has been explicitly assigned to the requested key through
 * `defaulting_map_t::set()` before, that value will be returned by `get()`.
 * Otherwise the default value is returned.
 *
 * `defaulting_map_t` essentially works as a sparse map, and can thereby be used
 * to save memory as well as for not having to set all values explicitly if most
 * of them are the same anyway.
 * It has the limitation that you cannot find out which values have been
 * set and which have not (which also means that you cannot iterate over it).
 */

template <class key_t, class value_t>
class defaulting_map_t {
public:
    explicit defaulting_map_t(const value_t &_default_value) :
        default_value(_default_value) { }

    void set(const key_t &key, const value_t &value) {
        if (value == default_value) {
            unset(key);
        } else {
            overrides[key] = value;
        }
    }
    void unset(const key_t &key) {
        overrides.erase(key);
    }
    void clear() {
        overrides.clear();
    }

    value_t get(const key_t &key) const {
        auto it = overrides.find(key);
        if (it == overrides.end()) {
            return default_value;
        } else {
            return it->second;
        }
    }

private:
    std::map<key_t, value_t> overrides;
    value_t default_value;
};


#endif // CONTAINERS_DEFAULTING_MAP_HPP_
