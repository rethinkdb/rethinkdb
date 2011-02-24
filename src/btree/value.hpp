#ifndef __BTREE_VALUE_HPP__
#define __BTREE_VALUE_HPP__

#include "buffer_cache/large_buf.hpp"


// Note: The metadata values are stored in the order
// [FLAGS][EXPTIME][CAS] while the flags are in a different order.
enum metadata_flags_enum_t {
    MEMCACHED_FLAGS   = 0x01,
    MEMCACHED_CAS     = 0x02,
    MEMCACHED_EXPTIME = 0x04,

    // The memcached delete queue feature (delayed deletion) is
    // currently not implemented.
    // DELETE_QUEUE   = 0x08,


    LARGE_VALUE       = 0x80
};

typedef uint32_t mcflags_t;
// TODO: We assume that time_t can be converted to an exptime_t,
// which is 32 bits.  We may run into problems in 2038 or 2106, or
// 68 years from the creation of the database, or something, if we
// do timestamp comparisons wrong.
typedef uint32_t exptime_t;

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



// Note: This struct is stored directly on disk.
struct btree_value {
    uint8_t size;
    metadata_flags_t metadata_flags;
    byte contents[];

public:
    uint16_t full_size() const {
        return sizeof(btree_value) + metadata_size(metadata_flags) + size;
    }

    // The size of the actual value, which might be the size of the large buf.
    int64_t value_size() const {
        if (is_large()) {
            int64_t ret = lb_ref().size;
            rassert(ret > MAX_IN_NODE_VALUE_SIZE);
            return ret;
        } else {
            return size;
        }
    }

    // Sets the size of the actual value, but it doesn't really create
    // a large buf.
    void value_size(int64_t new_size) {
        if (new_size <= MAX_IN_NODE_VALUE_SIZE) {
            metadata_set_large_value_bit(&metadata_flags, false);
            size = new_size;
        } else {
            metadata_set_large_value_bit(&metadata_flags, true);
            size = LARGE_BUF_REF_SIZE;

            // This is kind of fake, kind of paranoid, doing this
            // here.  The fact that we have this is a sign of scary
            // bad code.
            large_buf_ref_ptr()->size = new_size;
        }
    }

    bool is_large() const { return metadata_flags.flags & LARGE_VALUE; }

    byte *value() { return contents + metadata_size(metadata_flags); }
    const byte *value() const { return contents + metadata_size(metadata_flags); }

    large_buf_ref *large_buf_ref_ptr() { return reinterpret_cast<large_buf_ref *>(value()); }

    const large_buf_ref& lb_ref() const {
        rassert(is_large());
        rassert(size == LARGE_BUF_REF_SIZE);
        return *reinterpret_cast<const large_buf_ref *>(value());
    }
    void set_lb_ref(const large_buf_ref& ref) {
        rassert(is_large());
        rassert(size == LARGE_BUF_REF_SIZE);
        memcpy(value(), &ref, LARGE_BUF_REF_SIZE);
    }

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

    bool expired() const {
        return exptime() ? time(NULL) >= exptime() : false;
    }

    // CAS is treated differently from the other fields. Values initially don't
    // have a CAS; once it's added, though, we assume it's there for good. An
    // existing CAS should never be set to 0.
    void set_cas(cas_t new_cas) {
        cas_t *ptr = metadata_force_memcached_casptr(&metadata_flags, contents, contents + full_size());
        *ptr = new_cas;
    }

    void print() const {
        fprintf(stderr, "%*.*s", size, size, value());
    }
};




#endif  // __BTREE_VALUE_HPP__
