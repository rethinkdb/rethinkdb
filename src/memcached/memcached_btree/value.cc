#include "memcached/memcached_btree/value.hpp"

#include <string.h>
#include <time.h>

int metadata_size(metadata_flags_t mf) {
    return ((mf.flags & MEMCACHED_FLAGS) ? sizeof(mcflags_t) : 0)
        + ((mf.flags & MEMCACHED_CAS) ? sizeof(cas_t) : 0)
        + ((mf.flags & MEMCACHED_EXPTIME) ? sizeof(exptime_t) : 0);
}

mcflags_t metadata_memcached_flags(metadata_flags_t mf, const char *buf) {
    return (mf.flags & MEMCACHED_FLAGS) ? *reinterpret_cast<const mcflags_t *>(buf) : 0;
}

exptime_t metadata_memcached_exptime(metadata_flags_t mf, const char *buf) {
    return (mf.flags & MEMCACHED_EXPTIME)
        ? *reinterpret_cast<const exptime_t *>(buf + ((mf.flags & MEMCACHED_FLAGS) ? sizeof(mcflags_t) : 0))
        : 0;
}

bool metadata_memcached_cas(metadata_flags_t mf, const char *buf, cas_t *cas_out) {
    if (mf.flags & MEMCACHED_CAS) {
        *cas_out = *reinterpret_cast<const cas_t*>(buf + ((mf.flags & MEMCACHED_FLAGS) ? sizeof(mcflags_t) : 0)
                                                   + ((mf.flags & MEMCACHED_EXPTIME) ? sizeof(exptime_t) : 0));
        return true;
    } else {
        return false;
    }
}

cas_t *metadata_force_memcached_casptr(metadata_flags_t *mf, char *buf, char *buf_growable_end) {
    char *cas_pos = buf + ((mf->flags & MEMCACHED_FLAGS) ? sizeof(mcflags_t) : 0)
        + ((mf->flags & MEMCACHED_EXPTIME) ? sizeof(exptime_t) : 0);

    if ((mf->flags & MEMCACHED_CAS) == 0) {
        memmove(buf + sizeof(cas_t), buf, buf_growable_end - buf);
        mf->flags |= MEMCACHED_CAS;
    }
    return reinterpret_cast<cas_t*>(cas_pos);
}

char *metadata_write(metadata_flags_t *mf_out, char *to, mcflags_t mcflags, exptime_t exptime) {
    mf_out->flags = 0;
    char *p = to;
    if (mcflags != 0) {
        *reinterpret_cast<mcflags_t *>(p) = mcflags;
        mf_out->flags |= MEMCACHED_FLAGS;
        p += sizeof(mcflags_t);
    }
    if (exptime != 0) {
        *reinterpret_cast<exptime_t *>(p) = exptime;
        mf_out->flags |= MEMCACHED_EXPTIME;
        p += sizeof(exptime_t);
    }
    return p;
}

char *metadata_write(metadata_flags_t *mf_out, char *to, mcflags_t mcflags, exptime_t exptime, cas_t cas) {
    char *p = metadata_write(mf_out, to, mcflags, exptime);
    *reinterpret_cast<cas_t *>(p) = cas;
    mf_out->flags |= MEMCACHED_CAS;
    p += sizeof(cas_t);
    return p;
}

bool btree_value_fits(block_size_t block_size, int data_length, const memcached_value_t *value) {
    if (data_length < 1) {
        return false;
    }
    int msize = metadata_size(value->metadata_flags);
    if (data_length < 1 + msize) {
        return false;
    }

    return blob::ref_fits(block_size, data_length - (1 + msize), value->value_ref(), blob::btree_maxreflen);
}

bool memcached_value_t::expired(exptime_t effective_time) const {
    return exptime() && effective_time >= exptime();
}
