#ifndef MEMCACHED_BTREE_VALUE_HPP_
#define MEMCACHED_BTREE_VALUE_HPP_

#include "errors.hpp"
#include "buffer_cache/blob.hpp"


// Note: The metadata values are stored in the order
// [FLAGS][EXPTIME][CAS] while the flags are in a different order.
enum metadata_flags_enum_t {
    MEMCACHED_FLAGS   = 0x01,
    MEMCACHED_CAS     = 0x02,
    MEMCACHED_EXPTIME = 0x04
};

typedef uint32_t mcflags_t;
// TODO: We assume that time_t can be converted to an exptime_t,
// which is 32 bits.  We may run into problems in 2038 or 2106, or
// 68 years from the creation of the database, or something, if we
// do timestamp comparisons wrong.
typedef uint32_t exptime_t;

typedef uint64_t cas_t;

struct metadata_flags_t {
    uint8_t flags;
};

int metadata_size(metadata_flags_t flags);

mcflags_t metadata_memcached_flags(metadata_flags_t mf, const char *buf);
exptime_t metadata_memcached_exptime(metadata_flags_t mf, const char *buf);
bool metadata_memcached_cas(metadata_flags_t mf, const char *buf, cas_t *cas_out);

// If there's not already a cas_t there must be sizeof(cas_t) bytes
// available _after_ buf_growable_end.
cas_t *metadata_force_memcached_casptr(metadata_flags_t *mf, char *buf, char *buf_growable_end);

bool metadata_large_value_bit(metadata_flags_t mf);
char *metadata_write(metadata_flags_t *mf_out, char *to, mcflags_t mcflags, exptime_t exptime);
char *metadata_write(metadata_flags_t *mf_out, char *to, mcflags_t mcflags, exptime_t exptime, cas_t cas);
void metadata_set_large_value_bit(metadata_flags_t *mf, bool bit);

struct memcached_value_t {
    metadata_flags_t metadata_flags;
    char contents[];

public:
    int inline_size(block_size_t bs) const {
        int msize = metadata_size(metadata_flags);
        return offsetof(memcached_value_t, contents) + msize + blob::ref_size(bs, contents + msize, blob::btree_maxreflen);
    }

    int64_t value_size() const {
        return blob::value_size(contents + metadata_size(metadata_flags), blob::btree_maxreflen);
    }

    const char *value_ref() const { return contents + metadata_size(metadata_flags); }
    char *value_ref() { return contents + metadata_size(metadata_flags); }

    mcflags_t mcflags() const { return metadata_memcached_flags(metadata_flags, contents); }

    exptime_t exptime() const { return metadata_memcached_exptime(metadata_flags, contents); }
    bool has_cas() const {
        return metadata_flags.flags & MEMCACHED_CAS;
    }
    cas_t cas() const {
        // We shouldn't be asking for a value's CAS unless we know it has one.
        cas_t ret;
        bool hascas = metadata_memcached_cas(metadata_flags, contents, &ret);
        (void)hascas;
        rassert(hascas);
        return ret;
    }

    bool expired(exptime_t effective_time) const;

    // CAS is treated differently from the other fields. Values initially don't
    // have a CAS; once it's added, though, we assume it's there for good. An
    // existing CAS should never be set to 0.
    void set_cas(block_size_t bs, cas_t new_cas) {
        cas_t *ptr = metadata_force_memcached_casptr(&metadata_flags, contents, contents + inline_size(bs));
        *ptr = new_cas;
    }

    void add_cas(block_size_t bs) {
        /* Create a cas, but its initial value is undefined. TODO: This is implemented
        in a kind of hacky way. */
        set_cas(bs, 0xCA5ADDED);
    }
};

// In addition to the value itself we could potentially store
// memcached flags, exptime, and a CAS value in the value contents,
// plus the first size byte, so we reserve space for that.
#define MAX_MEMCACHED_VALUE_AUXILIARY_SIZE            (sizeof(memcached_value_t) + sizeof(uint32_t) + sizeof(uint32_t) + sizeof(uint64_t) + 1)
#define MAX_MEMCACHED_VALUE_SIZE                      (MAX_MEMCACHED_VALUE_AUXILIARY_SIZE + MAX_IN_NODE_VALUE_SIZE)

bool btree_value_fits(block_size_t bs, int data_length, const memcached_value_t *value);

#endif  // MEMCACHED_BTREE_VALUE_HPP_
