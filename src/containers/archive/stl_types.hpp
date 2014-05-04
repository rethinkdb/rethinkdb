// Copyright 2010-2013 RethinkDB, all rights reserved.
#ifndef CONTAINERS_ARCHIVE_STL_TYPES_HPP_
#define CONTAINERS_ARCHIVE_STL_TYPES_HPP_

#include <inttypes.h>

#include <limits>
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
void serialize(write_message_t *wm, const std::pair<T, U> &p) {
    serialize(wm, p.first);
    serialize(wm, p.second);
}

template <class T, class U>
MUST_USE archive_result_t deserialize(read_stream_t *s, std::pair<T, U> *p) {
    archive_result_t res = deserialize(s, &p->first);
    if (bad(res)) { return res; }
    res = deserialize(s, &p->second);
    return res;
}

// Keep in sync with serialize.
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
void serialize(write_message_t *wm, const std::map<K, V, C> &m) {
    // Extreme platform paranoia: It could become important that we
    // use something consistent like uint64_t for the size, not some
    // platform-specific size type such as std::map<K, V>::size_type.
    serialize_varint_uint64(wm, m.size());
    for (auto it = m.begin(), e = m.end(); it != e; ++it) {
        serialize(wm, *it);
    }
}

template <class K, class V, class C>
MUST_USE archive_result_t deserialize(read_stream_t *s, std::map<K, V, C> *m) {
    m->clear();

    uint64_t sz;
    archive_result_t res = deserialize_varint_uint64(s, &sz);
    if (bad(res)) { return res; }

    if (sz > std::numeric_limits<size_t>::max()) {
        return archive_result_t::RANGE_ERROR;
    }

    // Using position should make this function take linear time, not
    // sz*log(sz) time.
    typename std::map<K, V, C>::iterator position = m->begin();

    for (uint64_t i = 0; i < sz; ++i) {
        std::pair<K, V> p;
        res = deserialize(s, &p);
        if (bad(res)) { return res; }
        position = m->insert(position, p);
    }

    return archive_result_t::SUCCESS;
}

template <class T>
void serialize(write_message_t *wm, const std::set<T> &s) {
    serialize_varint_uint64(wm, s.size());
    for (typename std::set<T>::iterator it = s.begin(), e = s.end(); it != e; ++it) {
        serialize(wm, *it);
    }
}

template <class T>
MUST_USE archive_result_t deserialize(read_stream_t *s, std::set<T> *out) {
    out->clear();

    uint64_t sz;
    archive_result_t res = deserialize_varint_uint64(s, &sz);
    if (bad(res)) { return res; }

    if (sz > std::numeric_limits<size_t>::max()) {
        return archive_result_t::RANGE_ERROR;
    }

    typename std::set<T>::iterator position = out->begin();

    for (uint64_t i = 0; i < sz; ++i) {
        T value;
        res = deserialize(s, &value);
        if (bad(res)) { return res; }
        position = out->insert(position, value);
    }

    return archive_result_t::SUCCESS;
}

size_t serialized_size(const std::string &s);
void serialize(write_message_t *wm, const std::string &s);
MUST_USE archive_result_t deserialize(read_stream_t *s, std::string *out);

// Think twice before using this function on vectors containing a primitive type --
// it'll take O(n) time!
// Keep in sync with serialize.
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
void serialize(write_message_t *wm, const std::vector<T> &v) {
    serialize_varint_uint64(wm, v.size());
    for (auto it = v.begin(), e = v.end(); it != e; ++it) {
        serialize(wm, *it);
    }
}

template <class T>
MUST_USE archive_result_t deserialize(read_stream_t *s, std::vector<T> *v) {
    v->clear();

    uint64_t sz;
    archive_result_t res = deserialize_varint_uint64(s, &sz);
    if (bad(res)) { return res; }

    if (sz > std::numeric_limits<size_t>::max()) {
        return archive_result_t::RANGE_ERROR;
    }

    v->resize(sz);
    for (uint64_t i = 0; i < sz; ++i) {
        res = deserialize(s, &(*v)[i]);
        if (bad(res)) { return res; }
    }

    return archive_result_t::SUCCESS;
}

// TODO: Stop using std::list! What are you thinking?
template <class T>
void serialize(write_message_t *wm, const std::list<T> &v) {
    serialize_varint_uint64(wm, v.size());
    for (typename std::list<T>::const_iterator it = v.begin(), e = v.end(); it != e; ++it) {
        serialize(wm, *it);
    }
}

template <class T>
MUST_USE archive_result_t deserialize(read_stream_t *s, std::list<T> *v) {
    // Omit assertions because it's not a shame if a std::list gets corrupted.

    uint64_t sz;
    archive_result_t res = deserialize_varint_uint64(s, &sz);
    if (bad(res)) { return res; }

    if (sz > std::numeric_limits<size_t>::max()) {
        return archive_result_t::RANGE_ERROR;
    }

    for (uint64_t i = 0; i < sz; ++i) {
        // We avoid copying a non-empty value.
        v->push_back(T());
        res = deserialize(s, &v->back());
        if (bad(res)) { return res; }
    }

    return archive_result_t::SUCCESS;
}

}  /* namespace std */

#endif  // CONTAINERS_ARCHIVE_STL_TYPES_HPP_
