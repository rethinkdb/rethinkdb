// Copyright 2010-2014 RethinkDB, all rights reserved.
#ifndef RPC_SEMILATTICE_JOINS_VERSIONED_TCC_
#define RPC_SEMILATTICE_JOINS_VERSIONED_TCC_

#include "rpc/semilattice/joins/versioned.hpp"

template <class T>
bool operator==(const versioned_t<T> &a, const versioned_t<T> &b) {
    return (a.value == b.value) && \
        (a.timestamp == b.timestamp) && \
        (a.tiebreaker == b.tiebreaker);
}

template <class T>
void semilattice_join(versioned_t<T> *a, const versioned_t<T> &b) {
    if (a->timestamp < b.timestamp ||
            (a.timestamp == b.timestamp && a.tiebreaker < b.tiebreaker)) {
        *a = b;
    }
}

template <cluster_version_t W, class T>
size_t serialized_size(const versioned_t<T> &x) {
    return serialized_size<W>(x.value) + serialized_size<W>(x.timestamp) +
        serialized_size<W>(x.tiebreaker);
}

template <cluster_version_t W, class T>
void serialize(write_message_t *wm, const versioned_t<T> &x) {
    serialize<W>(wm, x.value);
    serialize<W>(wm, x.timestamp);
    serialize<W>(wm, x.tiebreaker);
}

template <cluster_version_t W, class T>
MUST_USE archive_result_t deserialize(read_stream_t *s, versioned_t<T> *x) {
    archive_result_t res = deserialize<W>(s, &x->value);
    if (bad(res)) return res;
    res = deserialize<W>(s, &x->timestamp);
    if (bad(res)) return res;
    return deserialize<W>(s, &x->tiebreaker);
}

#endif /* RPC_SEMILATTICE_JOINS_VERSIONED_TCC_ */
