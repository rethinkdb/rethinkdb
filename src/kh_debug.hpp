#ifndef KH_DEBUG_HPP_
#define KH_DEBUG_HPP_

#include "btree/keys.hpp"
#include "debug.hpp"

void khd_all(const std::string &msg);
void khd_range(const key_range_t &kr, const std::string &msg);

inline void khd_key(const store_key_t &k, const std::string &msg) {
    khd_range(key_range_t(k.btree_key()), msg);
}

template<class T>
void khd_all(const std::string &msg, const T &t) {
    khd_all(msg + ": " + debug_strprint(t));
}

template<class T>
void khd_range(const key_range_t &kr, const std::string &msg, const T &t) {
    khd_range(kr, msg + ": " + debug_strprint(t));
}

template<class T>
void khd_key(const store_key_t &k, const std::string &msg, const T &t) {
    khd_key(k, msg + ": " + debug_strprint(t));
}

void khd_dump(const store_key_t &key);

#endif /* KH_DEBUG_HPP_ */

