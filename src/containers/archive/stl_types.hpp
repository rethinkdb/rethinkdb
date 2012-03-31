#ifndef CONTAINERS_ARCHIVE_STL_TYPES_HPP_
#define CONTAINERS_ARCHIVE_STL_TYPES_HPP_

#include <map>
#include <set>
#include <string>
#include <vector>

#include "containers/archive/primitives.hpp"

// Implementations for pair, map, set, string, and vector.  list is
// omitted because we should be optimistic.

template <class T, class U>
write_message_t &operator<<(write_message_t &msg, const std::pair<T, U> &p) {
    msg << p.first;
    msg << p.second;
    return msg;
}

template <class K, class V>
write_message_t &operator<<(write_message_t &msg, const std::map<K, V> &m) {
    // Extreme platform paranoia: It could become important that we
    // use something consistent like uint64_t for the size, not some
    // platform-specific size type such as std::map<K, V>::size_type.
    uint64_t sz = m.size();

    msg << sz;
    for (typename std::map<K, V>::const_iterator it = m.begin(), e = m.end(); it != e; ++it) {
        msg << *it;
    }

    return msg;
}

template <class T>
write_message_t &operator<<(write_message_t &msg, const std::set<T> &s) {
    uint64_t sz = s.size();

    msg << sz;
    for (typename std::set<T>::const_iterator it = s.begin(), e = s.end(); it != e; ++it) {
        msg << *it;
    }

    return msg;
}

write_message_t &operator<<(write_message_t &msg, const std::string &s) {
    const char *data = s.data();
    uint64_t sz = s.size();

    msg << sz;
    msg.append(data, sz);

    return msg;
}

template <class T>
write_message_t &operator<<(write_message_t &msg, const std::vector<T> &v) {
    uint64_t sz = v.size();

    for (typename std::vector<T>::const_iterator it = v.begin(), e = v.end(); it != e; ++it) {
        msg << *it;
    }

    return msg;
}




#endif  // CONTAINERS_ARCHIVE_STL_TYPES_HPP_
