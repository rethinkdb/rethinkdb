#ifndef __RPC_UID_HPP__
#define __RPC_UID_HPP__

#include "rpc/serialize/basic.hpp"
#include "rpc/serialize/serialize_macros.hpp"

/* If you call `cluster_uid_t::create()`, the result is guaranteed to be different
from every other `cluster_uid_t` ever returned by a call to `cluster_uid_t::create()`
in any node in this cluster. */

struct cluster_uid_t {

    static cluster_uid_t create();

    cluster_uid_t();   // Makes an invalid `cluster_uid_t`
    bool is_valid() const;   // Determines whether a `cluster_uid_t` was created from `create()` or `cluster_uid_t()`.

    /* Compare for equality. Crashes if either one is invalid. */
    bool operator==(const cluster_uid_t &) const;
    bool operator!=(const cluster_uid_t &) const;

    /* There is a strict ordering on `cluster_uid_t`s so that they can
    be used as std::map keys and so on. */
    bool operator<(const cluster_uid_t &) const;
    bool operator>(const cluster_uid_t &) const;
    bool operator<=(const cluster_uid_t &) const;
    bool operator>=(const cluster_uid_t &) const;

    /* `cluster_uid_t` is serializable */
    RDB_MAKE_ME_SERIALIZABLE_3(peer, thread, counter);

private:
    /* `peer` identifies which cluster peer this UID was created on. `thread`
    identifies which thread on that peer. `counter` is a per-thread counter.
    If a `cluster_uid_t` is created using the default constructor, they are
    all -1. */
    int peer, thread, counter;
};

#endif /* __RPC_UID_HPP__ */
