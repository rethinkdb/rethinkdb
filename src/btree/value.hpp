#ifndef __BTREE_VALUE_HPP__
#define __BTREE_VALUE_HPP__

#include "buffer_cache/large_buf.hpp"


// Note: The metadata values are stored in the order
// [FLAGS][EXPTIME][CAS] while the flags are in a different order.
enum metadata_flags {
    MEMCACHED_FLAGS   = 0x01,
    MEMCACHED_CAS     = 0x02,
    MEMCACHED_EXPTIME = 0x04,

    // The memcached delete queue feature (delayed deletion) is
    // currently not implemented.
    // DELETE_QUEUE   = 0x08,


    LARGE_VALUE       = 0x80
};

typedef uint32_t mcflags_t;
typedef uint64_t cas_t;
// TODO: We assume that time_t can be converted to an exptime_t,
// which is 32 bits.  We may run into problems in 2038 or 2106, or
// 68 years from the creation of the database, or something, if we
// do timestamp comparisons wrong.
typedef uint32_t exptime_t;

struct btree_value_metadata;

int metadata_size(const btree_value_metadata *metadata);
mcflags_t metadata_memcached_flags(const btree_value_metadata *metadata);
exptime_t metadata_memcached_exptime(const btree_value_metadata *metadata);
bool metadata_memcached_cas(const btree_value_metadata *metadata, cas_t *cas_out);
bool metadata_large_value_bit(const btree_value_metadata *metadata);


// Note: This struct is stored directly on disk.
struct btree_value {
    uint8_t size;
    uint8_t metadata_flags;
private:
    byte contents[0];

public:
    uint16_t full_size() const {
        return value_offset() + size + sizeof(btree_value);
    }

    // The size of the actual value, which might be the size of the large buf.
    int64_t value_size() const {
        if (is_large()) {
            int64_t ret = lb_ref().size;
            assert(ret > MAX_IN_NODE_VALUE_SIZE);
            return ret;
        } else {
            return size;
        }
    }

    // Sets the size of the actual value, but it doesn't really create
    // a large buf.
    void value_size(int64_t new_size) {
        if (new_size <= MAX_IN_NODE_VALUE_SIZE) {
            metadata_flags &= ~LARGE_VALUE;
            size = new_size;
        } else {
            metadata_flags |= LARGE_VALUE;
            size = sizeof(large_buf_ref);
            large_buf_ref_ptr()->size = new_size;
        }
    }

    // Every value has mcflags, but they're very often 0, in which case we just
    // store a bit instead of 4 bytes.
private:
    bool has_mcflags() const { return metadata_flags & MEMCACHED_FLAGS;   }
public:
    bool has_cas()     const { return metadata_flags & MEMCACHED_CAS;     }
private:
    bool has_exptime() const { return metadata_flags & MEMCACHED_EXPTIME; }
public:
    bool is_large()    const { return metadata_flags & LARGE_VALUE;       }

private:
    uint8_t mcflags_offset() const { return 0;                                                    }
    uint8_t exptime_offset() const { return mcflags_offset() + sizeof(mcflags_t) * has_mcflags(); }
    uint8_t cas_offset()     const { return exptime_offset() + sizeof(exptime_t) * has_exptime(); }
    uint8_t value_offset()   const { return     cas_offset() + sizeof(cas_t)     * has_cas();     }

    mcflags_t       *mcflags_addr()       { return ptr_cast<mcflags_t>(contents + mcflags_offset()); }
    const mcflags_t *mcflags_addr() const { return ptr_cast<mcflags_t>(contents + mcflags_offset()); }
    exptime_t       *exptime_addr()       { return ptr_cast<exptime_t>(contents + exptime_offset()); }
    const exptime_t *exptime_addr() const { return ptr_cast<exptime_t>(contents + exptime_offset()); }
    cas_t           *cas_addr()           { return ptr_cast<cas_t>(contents + cas_offset()); }
    const cas_t     *cas_addr()     const { return ptr_cast<cas_t>(contents + cas_offset()); }
public:
    byte            *value()              { return ptr_cast<byte>(contents + value_offset()); }
    const byte      *value()        const { return ptr_cast<byte>(contents + value_offset()); }

    large_buf_ref *large_buf_ref_ptr() { return ptr_cast<large_buf_ref>(value()); }

    const large_buf_ref& lb_ref() const {
        assert(is_large());
        assert(size == sizeof(large_buf_ref));
        return *ptr_cast<large_buf_ref>(value());
    }
    void set_lb_ref(const large_buf_ref& ref) {
        assert(is_large());
        assert(size == sizeof(large_buf_ref));
        *reinterpret_cast<large_buf_ref *>(value()) = ref;
    }

    mcflags_t mcflags() const { return has_mcflags() ? *mcflags_addr() : 0; }
    exptime_t exptime() const { return has_exptime() ? *exptime_addr() : 0; }
    cas_t         cas() const { // We shouldn't be asking for a value's CAS unless we know it has one.
        assert(has_cas());
        return *cas_addr();
    }

    void clear_space(byte *faddr, uint8_t fsize) {
        memmove(faddr, faddr + fsize, full_size() - (faddr - (byte *)this) - fsize);
    }

    // This assumes there's enough space allocated to move the value into.
    void make_space(byte *faddr, uint8_t fsize) {
        assert(full_size() + fsize <= MAX_BTREE_VALUE_SIZE);
        memmove(faddr + fsize, faddr, full_size() - (faddr - (byte *)this));
    }

    void set_mcflags(mcflags_t new_mcflags) {
        if (has_mcflags() && new_mcflags == 0) { // Flags is being set to 0, so we clear the 4 bytes we kept for it.
            clear_space(reinterpret_cast<byte *>(mcflags_addr()), sizeof(mcflags_t));
            metadata_flags &= ~MEMCACHED_FLAGS;
        } else if (!has_mcflags() && new_mcflags) { // Make space for non-zero mcflags.
            metadata_flags |= MEMCACHED_FLAGS;
            make_space(reinterpret_cast<byte *>(mcflags_addr()), sizeof(mcflags_t));
        }
        if (new_mcflags) { // We've made space, so copy the mcflags over.
            *mcflags_addr() = new_mcflags;
        }
    }

    void set_exptime(exptime_t new_exptime) {
        if (has_exptime() && new_exptime == 0) { // Exptime is being set to 0, so we clear the 4 bytes we kept for it.
            clear_space(reinterpret_cast<byte *>(exptime_addr()), sizeof(exptime_t));
            metadata_flags &= ~MEMCACHED_EXPTIME;
        } else if (!has_exptime() && new_exptime) { // Make space for non-zero exptime.
            metadata_flags |= MEMCACHED_EXPTIME;
            make_space(reinterpret_cast<byte *>(exptime_addr()), sizeof(exptime_t));
        }
        if (new_exptime) {
            *exptime_addr() = new_exptime;
        }
    }

    bool expired() {
        return exptime() ? time(NULL) >= exptime() : false;
    }

    // CAS is treated differently from the other fields. Values initially don't
    // have a CAS; once it's added, though, we assume it's there for good. An
    // existing CAS should never be set to 0.
    void set_cas(cas_t new_cas) {
        if (!new_cas) {
            assert(!has_cas());
            return;
        }
        if (!has_cas()) { // Make space for CAS.
            metadata_flags |= MEMCACHED_CAS;
            make_space(reinterpret_cast<byte *>(cas_addr()), sizeof(cas_t));
        }
        *cas_addr() = new_cas;
    }

    void print() const {
        fprintf(stderr, "%*.*s", size, size, value());
    }
};




#endif  // __BTREE_VALUE_HPP__
