// Copyright 2010-2012 RethinkDB, all rights reserved.
#ifndef CONTAINERS_ARCHIVE_STL_TYPES_HPP_
#define CONTAINERS_ARCHIVE_STL_TYPES_HPP_

#ifndef __STDC_FORMAT_MACROS
#define __STDC_FORMAT_MACROS
#endif  // __STDC_FORMAT_MACROS
#include <inttypes.h>

#include <list>  // ugh
#include <map>
#include <set>
#include <string>
#include <vector>
#include <utility>

#include "containers/archive/archive.hpp"
#include "containers/archive/varint.hpp"

namespace std {

// Implementations for pair, map, set, string, and vector.

template <class T, class U>
size_t serialized_size(const std::pair<T, U> &p) {
    return serialized_size(p.first) + serialized_size(p.second);
}

// Keep in sync with serialized_size.
template <class T, class U>
write_message_t &operator<<(write_message_t &msg, const std::pair<T, U> &p) {
    msg << p.first;
    msg << p.second;
    return msg;
}

template <class T, class U>
MUST_USE archive_result_t deserialize(read_stream_t *s, std::pair<T, U> *p) {
    archive_result_t res = deserialize(s, &p->first);
    if (res) { return res; }
    res = deserialize(s, &p->second);
    return res;
}

// Keep in sync with operator<<.
template <class K, class V, class C>
size_t serialized_size(const std::map<K, V, C> &m) {
    size_t ret = varint_uint64_serialized_size(m.size());
    for (auto it = m.begin(), e = m.end(); it != e; ++it) {
        ret += serialized_size(*it);
    }
    return ret;
}

// Keep in sync with serialized_size.
template <class K, class V, class C>
write_message_t &operator<<(write_message_t &msg, const std::map<K, V, C> &m) {
    // Extreme platform paranoia: It could become important that we
    // use something consistent like uint64_t for the size, not some
    // platform-specific size type such as std::map<K, V>::size_type.
    serialize_varint_uint64(&msg, m.size());
    for (auto it = m.begin(), e = m.end(); it != e; ++it) {
        msg << *it;
    }

    return msg;
}

template <class K, class V, class C>
MUST_USE archive_result_t deserialize(read_stream_t *s, std::map<K, V, C> *m) {
    m->clear();

    uint64_t sz;
    archive_result_t res = deserialize_varint_uint64(s, &sz);
    if (res) { return res; }

    // Using position should make this function take linear time, not
    // sz*log(sz) time.
    typename std::map<K, V, C>::iterator position = m->begin();

    for (uint64_t i = 0; i < sz; ++i) {
        std::pair<K, V> p;
        res = deserialize(s, &p);
        if (res) { return res; }
        position = m->insert(position, p);
    }

    return ARCHIVE_SUCCESS;
}

template <class T>
write_message_t &operator<<(write_message_t &msg, const std::set<T> &s) {
    serialize_varint_uint64(&msg, s.size());
    for (typename std::set<T>::const_iterator it = s.begin(), e = s.end(); it != e; ++it) {
        msg << *it;
    }

    return msg;
}

template <class T>
MUST_USE archive_result_t deserialize(read_stream_t *s, std::set<T> *out) {
    out->clear();

    uint64_t sz;
    archive_result_t res = deserialize_varint_uint64(s, &sz);
    if (res) { return res; }

    typename std::set<T>::iterator position = out->begin();

    for (uint64_t i = 0; i < sz; ++i) {
        T value;
        res = deserialize(s, &value);
        if (res) { return res; }
        position = out->insert(position, value);
    }

    return ARCHIVE_SUCCESS;
}

size_t serialized_size(const std::string &s);
write_message_t &operator<<(write_message_t &msg, const std::string &s);
MUST_USE archive_result_t deserialize(read_stream_t *s, std::string *out);

// Think twice before using this function on vectors containing a primitive type --
// it'll take O(n) time!
// Keep in sync with operator<<.
template <class T>
size_t serialized_size(const std::vector<T> &v) {
    size_t ret = varint_uint64_serialized_size(v.size());
    for (auto it = v.begin(), e = v.end(); it != e; ++it) {
        ret += serialized_size(*it);
    }
    return ret;
}


// Keep in sync with serialized_size.
template <class T>
write_message_t &operator<<(write_message_t &msg, const std::vector<T> &v) {
    serialize_varint_uint64(&msg, v.size());
    for (auto it = v.begin(), e = v.end(); it != e; ++it) {
        msg << *it;
    }

    return msg;
}

template <class T>
MUST_USE archive_result_t deserialize(read_stream_t *s, std::vector<T> *v) {
    v->clear();

    uint64_t sz;
    archive_result_t res = deserialize_varint_uint64(s, &sz);
    if (res) { return res; }

    v->resize(sz);
    for (uint64_t i = 0; i < sz; ++i) {
        res = deserialize(s, &(*v)[i]);
        if (res) { return res; }
    }

    return ARCHIVE_SUCCESS;
}

// TODO: Stop using std::list! What are you thinking?
template <class T>
write_message_t &operator<<(write_message_t &msg, const std::list<T> &v) {
    uint64_t sz = v.size();
    msg << sz;
    for (typename std::list<T>::const_iterator it = v.begin(), e = v.end(); it != e; ++it) {
        msg << *it;
    }

    return msg;
}

template <class T>
MUST_USE archive_result_t deserialize(read_stream_t *s, std::list<T> *v) {
    // Omit assertions because it's not a shame if a std::list gets corrupted.

    uint64_t sz;
    archive_result_t res = deserialize(s, &sz);
    if (res) { return res; }

    for (uint64_t i = 0; i < sz; ++i) {
        // We avoid copying a non-empty value.
        v->push_back(T());
        res = deserialize(s, &v->back());
        if (res) { return res; }
    }

    return ARCHIVE_SUCCESS;
}

}  /* namespace std */

#endif  // CONTAINERS_ARCHIVE_STL_TYPES_HPP_
