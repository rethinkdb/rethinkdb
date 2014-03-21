// Copyright 2010-2012 RethinkDB, all rights reserved.
#ifndef RPC_SEMILATTICE_JOINS_VCLOCK_TCC_
#define RPC_SEMILATTICE_JOINS_VCLOCK_TCC_

#include "rpc/semilattice/joins/vclock.hpp"

#include <vector>

template <class T>
vclock_t<T>::vclock_t(const stamped_value_t &_value) {
    values.insert(_value);
}

template <class T>
void vclock_t<T>::cull_old_values() {
    guarantee(!values.empty(), "As a precondition, values should never be empty\n");
    value_map_t to_delete;

    for (typename value_map_t::iterator p = values.begin(); p != values.end(); ++p) {
        for (typename value_map_t::iterator q = values.begin(); q != values.end(); ++q) {
            if (vclock_details::dominates(p->first, q->first)) {
                to_delete.insert(*p);
            }
        }
    }

    for (typename value_map_t::iterator d_it = to_delete.begin(); d_it != to_delete.end(); ++d_it) {
        values.erase(d_it->first);
    }
    guarantee(!values.empty(), "As a postcondition, values should never be empty\n");
}

template <class T>
vclock_t<T>::vclock_t() {
    values.insert(std::make_pair(vclock_details::version_map_t(), T()));
}

template <class T>
vclock_t<T>::vclock_t(const T &_t) {
    values.insert(std::make_pair(vclock_details::version_map_t(), _t));
}

template <class T>
vclock_t<T>::vclock_t(const T &_t, const uuid_u &us) {
    stamped_value_t tmp = std::make_pair(vclock_details::version_map_t(), _t);
    tmp.first[us] = 1;
    values.insert(tmp);
}

template <class T>
bool vclock_t<T>::in_conflict() const {
    guarantee(!values.empty());
    return values.size() != 1;
}

template <class T>
void vclock_t<T>::throw_if_conflict() const {
    if (in_conflict()) {
        throw in_conflict_exc_t();
    }
}

template <class T>
vclock_t<T> vclock_t<T>::make_new_version(const T& t, const uuid_u &us) {
    throw_if_conflict();
    stamped_value_t tmp = *values.begin();
    ++tmp.first[us];
    tmp.second = t;
    return vclock_t<T>(tmp);
}

template <class T>
vclock_t<T> vclock_t<T>::make_resolving_version(const T& t, const uuid_u &us) {
    vclock_details::version_map_t vmap; //construct a vmap that dominates all the others

    for (typename value_map_t::iterator it  = values.begin();
                                        it != values.end();
                                        ++it) {
        vmap = vclock_details::vmap_max(vmap, it->first);
    }

    ++vmap[us];

    return vclock_t(std::make_pair(vmap, t));
}

template <class T>
void vclock_t<T>::upgrade_version(const uuid_u &us) {
    throw_if_conflict();

    stamped_value_t tmp = *values.begin();
    ++tmp.first[us];
    values.clear();
    values.insert(tmp);
}

template <class T>
T vclock_t<T>::get() const {
    throw_if_conflict();
    return values.begin()->second;
}

template <class T>
const T &vclock_t<T>::get_ref() const {
    throw_if_conflict();
    return values.begin()->second;
}

template <class T>
T &vclock_t<T>::get_mutable() {
    throw_if_conflict();
    return values.begin()->second;
}

template <class T>
std::vector<T> vclock_t<T>::get_all_values() const {
    std::vector<T> result;
    for (typename value_map_t::const_iterator i = values.begin(); i != values.end(); ++i)
        result.push_back(i->second);
    return result;
}

//semilattice concept for vclock_t
template <class T>
bool operator==(const vclock_t<T> &a, const vclock_t<T> &b) {
    return a.values == b.values;
}

template <class T>
void semilattice_join(vclock_t<T> *a, const vclock_t<T> &b) {
    a->values.insert(b.values.begin(), b.values.end());

    a->cull_old_values();
}

template <class T>
void debug_print(printf_buffer_t *buf, const vclock_t<T> &x) {
    buf->appendf("vclock{");
    debug_print(buf, x.values);
    buf->appendf("}");
}

#endif  // RPC_SEMILATTICE_JOINS_VCLOCK_TCC_
