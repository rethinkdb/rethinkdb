#include "value.hpp"




struct btree_value_metadata {
    uint8_t flags;
    uint8_t data[];
};


int metadata_size(const btree_value_metadata *m) {
    return 1 + ((m->flags & MEMCACHED_FLAGS) ? sizeof(mcflags_t) : 0)
        + ((m->flags & MEMCACHED_CAS) ? sizeof(cas_t) : 0)
        + ((m->flags & MEMCACHED_EXPTIME) ? sizeof(exptime_t) : 0);
}

mcflags_t metadata_memcached_flags(const btree_value_metadata *m) {
    return (m->flags & MEMCACHED_FLAGS) ? *ptr_cast<mcflags_t>(m->data) : 0;
}

exptime_t metadata_memcached_exptime(const btree_value_metadata *m) {
    return (m->flags & MEMCACHED_EXPTIME)
        ? *ptr_cast<exptime_t>(m->data + ((m->flags & MEMCACHED_FLAGS) ? sizeof(mcflags_t) : 0))
        : 0;
}

bool metadata_memcached_cas(const btree_value_metadata *m, cas_t *cas_out) {
    if (m->flags & MEMCACHED_CAS) {
        *cas_out = *ptr_cast<cas_t>(m->data + ((m->flags & MEMCACHED_FLAGS) ? sizeof(mcflags_t) : 0)
                                    + ((m->flags & MEMCACHED_EXPTIME) ? sizeof(exptime_t) : 0));
        return true;
    } else {
        return false;
    }
}






