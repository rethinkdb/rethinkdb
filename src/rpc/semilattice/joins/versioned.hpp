// Copyright 2010-2014 RethinkDB, all rights reserved.
#ifndef RPC_SEMILATTICE_JOINS_VERSIONED_HPP_
#define RPC_SEMILATTICE_JOINS_VERSIONED_HPP_

#include "arch/runtime/runtime_utils.hpp"
#include "containers/uuid.hpp"
#include "rpc/serialize_macros.hpp"

/* A `versioned_t` is used in the semilattices to track a setting that the user is
allowed to update. If the setting is updated in two places simultaneously, the
semilattice join will pick the one that came later as measured by the machines' clocks.
*/

template<class T>
class versioned_t {
public:
    versioned_t() : timestamp(versioned_t::invalid_timestamp) { }

    static versioned_t make_initial(const T &value) {
        versioned_t<T> v;
        v.value = value;
        v.timestamp = time(NULL);
        v.tiebreaker = generate_uuid();
        return v;
    }

    const T &get_ref() const {
        guarantee(timestamp != invalid_timestamp);
        return value;
    }

    void set(const T &new_value) {
        value = new_value;
        on_change();
    }

    template<class callable_t>
    void apply_write(const callable_t &function) {
        ASSERT_FINITE_CORO_WAITING;
        function(&value);
        on_change();
    }

    RDB_MAKE_ME_SERIALIZABLE_3(value, timestamp, tiebreaker);

private:
    template<class TT>
    friend bool operator==(const versioned_t<TT> &a, const versioned_t<TT> &b);

    template<class TT>
    friend void semilattice_join(versioned_t<TT> *a, const versioned_t<TT> &b);

    static const time_t invalid_timestamp = std::numeric_limits<time_t>::max();

    void on_change() {
        guarantee(timestamp != invalid_timestamp);
        timestamp = std::max(timestamp+1, time(NULL));
        tiebreaker = generate_uuid();
    }

    T value;
    time_t timestamp;
    uuid_u tiebreaker;
};

RDB_SERIALIZE_TEMPLATED_OUTSIDE(versioned_t);

template <class T>
bool operator==(const versioned_t<T> &a, const versioned_t<T> &b) {
    guarantee(a.timestamp != versioned_t<T>::invalid_timestamp);
    guarantee(b.timestamp != versioned_t<T>::invalid_timestamp);
    return (a.value == b.value) && \
        (a.timestamp == b.timestamp) && \
        (a.tiebreaker == b.tiebreaker);
}

template <class T>
void semilattice_join(versioned_t<T> *a, const versioned_t<T> &b) {
    guarantee(a->timestamp != versioned_t<T>::invalid_timestamp);
    guarantee(b.timestamp != versioned_t<T>::invalid_timestamp);
    if (a->timestamp < b.timestamp ||
            (a->timestamp == b.timestamp && a->tiebreaker < b.tiebreaker)) {
        *a = b;
    }
}

#endif /* RPC_SEMILATTICE_JOINS_VERSIONED_HPP_ */

