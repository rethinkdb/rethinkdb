#ifndef __REPLICATION_MASTERSTORE_HPP__
#define __REPLICATION_MASTERSTORE_HPP__

#include "store.hpp"

namespace replication {

class masterstore_t {

    // TODO: Make a write-only store interface, a read-only store
    // interface.  Some of these methods get ignored.

    void get_cas(store_key_t *key, cas_t proposed_cas);

    void set(store_key_t *key, data_provider_t *data, mcflags_t flags, exptime_t exptime, cas_t proposed_cas);
    void add(store_key_t *key, data_provider_t *data, mcflags_t flags, exptime_t exptime, cas_t proposed_cas);
    void replace(store_key_t *key, data_provider_t *data, mcflags_t flags, exptime_t exptime, cas_t proposed_cas);
    void cas(store_key_t *key, data_provider_t *data, mcflags_t flags, exptime_t exptime, cas_t unique, cas_t proposed_cas);
    void incr(store_key_t *key, unsigned long long amount, cas_t proposed_cas);
    void decr(store_key_t *key, unsigned long long amount, cas_t proposed_cas);
    void append(store_key_t *key, data_provider_t *data, cas_t proposed_cas);
    void prepend(store_key_t *key, data_provider_t *data, cas_t proposed_cas);
    void delete_key(store_key_t *key);
};





}  // namespace replication

#endif  // __REPLICATION_MASTERSTORE_HPP__
