#ifndef __BTREE_NODE_HPP__
#define __BTREE_NODE_HPP__

#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include <time.h>
#include "config/cmd_args.hpp"
#include "utils.hpp"
#include "buffer_cache/types.hpp"
#include "store.hpp"

struct btree_superblock_t {
    block_magic_t magic;
    block_id_t root_block;

    static const block_magic_t expected_magic;
};



//Note: This struct is stored directly on disk.  Changing it invalidates old data.
struct internal_node_t {
    block_magic_t magic;
    uint16_t npairs;
    uint16_t frontmost_offset;
    uint16_t pair_offsets[0];

    static const block_magic_t expected_magic;
};


// Here's how we represent the modification history of the leaf node.
// The last_modified time gives the modification time of the most
// recently modified key of the node.  Then, last_modified -
// earlier[0] gives the timestamp for the
// second-most-recently modified KV of the node.  In general,
// last_modified - earlier[i] gives the timestamp for the
// (i+2)th-most-recently modified KV.
//
// These values could be lies.  It is harmless to say that a key is
// newer than it really is.  So when earlier[i] overflows,
// we pin it to 0xFFFF.
struct leaf_timestamps_t {
    repli_timestamp last_modified;  // 0
    uint16_t earlier[NUM_LEAF_NODE_EARLIER_TIMES];  // 4
};

//Note: This struct is stored directly on disk.  Changing it invalidates old data.
struct leaf_node_t {
    block_magic_t magic;        // 0
    leaf_timestamps_t times;    // 4
    uint16_t npairs;            // 12

    // The smallest offset in pair_offsets
    uint16_t frontmost_offset;  // 14
    uint16_t pair_offsets[0];   // 16

    static const block_magic_t expected_magic;
};







enum metadata_flags {
    MEMCACHED_FLAGS   = 0x01,
    MEMCACHED_CAS     = 0x02,
    MEMCACHED_EXPTIME = 0x04,
    // DELETE_QUEUE   = 0x08, // If we implement this.
    LARGE_VALUE       = 0x80
};

typedef store_key_t btree_key;

// Note: This struct is stored directly on disk.
struct btree_value {
    uint8_t size;
    uint8_t metadata_flags;
private:
    byte contents[0];

public:
    void init() { }

    uint16_t mem_size() const {
        return value_offset() + size;
    }

    int64_t value_size() const {
        if (is_large()) {
            int64_t ret = lb_ref().size;
            assert(ret > MAX_IN_NODE_VALUE_SIZE);
            return ret;
        } else {
            return size;
        }
    }

    void value_size(int64_t new_size) {
        if (new_size <= MAX_IN_NODE_VALUE_SIZE) {
            if (is_large()) {
                metadata_flags &= ~LARGE_VALUE;
            }
            size = new_size;
        } else {
            if (!is_large()) {
                metadata_flags |= LARGE_VALUE;
            }
            size = sizeof(large_buf_ref);
            large_buf_ref_ptr()->size = new_size;
        }
    }

    typedef uint32_t mcflags_t;
    typedef uint64_t cas_t;

    // TODO: We assume that time_t can be converted to an exptime_t,
    // which is 32 bits.  We may run into problems in 2038 or 2106.
    typedef uint32_t exptime_t;

    // Every value has mcflags, but they're very often 0, in which case we just
    // store a bit instead of 4 bytes.
    bool has_mcflags() const { return metadata_flags & MEMCACHED_FLAGS;   }
    bool has_cas()     const { return metadata_flags & MEMCACHED_CAS;     }
    bool has_exptime() const { return metadata_flags & MEMCACHED_EXPTIME; }
    bool is_large()    const { return metadata_flags & LARGE_VALUE;       }

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
    byte            *value()              { return ptr_cast<byte>(contents + value_offset()); }
    const byte      *value()        const { return ptr_cast<byte>(contents + value_offset()); }

    large_buf_ref *large_buf_ref_ptr() { return ptr_cast<large_buf_ref>(value()); }

    const large_buf_ref& lb_ref() const { return *ptr_cast<large_buf_ref>(value()); }
    void set_lb_ref(const large_buf_ref& ref) {
        *(large_buf_ref *) value() = ref;
    }

    mcflags_t mcflags() const { return has_mcflags() ? *mcflags_addr() : 0; }
    exptime_t exptime() const { return has_exptime() ? *exptime_addr() : 0; }
    cas_t         cas() const { // We shouldn't be asking for a value's CAS unless we know it has one.
        assert(has_cas());
        return *cas_addr();
    }

    void clear_space(byte *faddr, uint8_t fsize, uint8_t offset) {
        memmove(faddr, faddr + fsize, mem_size() - offset - fsize);
        //size -= fsize;
    }

    void make_space(byte *faddr, uint8_t fsize, uint8_t offset) { // XXX This assumes there's enough space allocated to move the value into.
        assert(mem_size() + fsize <= MAX_TOTAL_NODE_CONTENTS_SIZE);
        //size += fsize;
        memmove(faddr + fsize, faddr, mem_size() - offset);
    }

    void set_mcflags(mcflags_t new_mcflags) {
        if (has_mcflags() && new_mcflags == 0) { // Flags is being set to 0, so we clear the 4 bytes we kept for it.
            clear_space((byte *) mcflags_addr(), sizeof(mcflags_t), mcflags_offset());
            metadata_flags &= ~MEMCACHED_FLAGS;
        } else if (!has_mcflags() && new_mcflags) { // Make space for non-zero mcflags.
            metadata_flags |= MEMCACHED_FLAGS;
            make_space((byte *) mcflags_addr(), sizeof(mcflags_t), mcflags_offset());
        }
        if (new_mcflags) { // We've made space, so copy the mcflags over.
            *mcflags_addr() = new_mcflags;
        }
    }

    void set_exptime(exptime_t new_exptime) {
        if (has_exptime() && new_exptime == 0) { // Exptime is being set to 0, so we clear the 4 bytes we kept for it.
            clear_space((byte *) exptime_addr(), sizeof(exptime_t), exptime_offset());
            metadata_flags &= ~MEMCACHED_EXPTIME;
        } else if (!has_exptime() && new_exptime) { // Make space for non-zero exptime.
            metadata_flags |= MEMCACHED_EXPTIME;
            make_space((byte *) exptime_addr(), sizeof(exptime_t), exptime_offset());
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
            make_space((byte *) cas_addr(), sizeof(cas_t), cas_offset());
        }
        *cas_addr() = new_cas;
    }

    void print() const {
        fprintf(stderr, "%*.*s", size, size, value());
    }
};

// A node_t is either a btree_internal_node or a btree_leaf_node.
struct node_t {
    block_magic_t magic;
};

template <>
bool check_magic<node_t>(block_magic_t magic);

class node_handler {
    public:
        static bool is_leaf(const node_t *node) {
            assert(check_magic<node_t>(node->magic));
            return check_magic<leaf_node_t>(node->magic);
        }

        static bool is_internal(const node_t *node) {
            assert(check_magic<node_t>(node->magic));
            return check_magic<internal_node_t>(node->magic);
        }

        static void str_to_key(char *str, btree_key *buf) {
            int len = strlen(str);
            check("string too long", len > MAX_KEY_SIZE);
            memcpy(buf->contents, str, len);
            buf->size = (uint8_t)len;
        }

        static bool is_underfull(block_size_t block_size, const node_t *node);
        static bool is_mergable(block_size_t block_size, const node_t *node, const node_t *sibling, const node_t *parent);
        static int nodecmp(const node_t *node1, const node_t *node2);
        static void merge(block_size_t block_size, node_t *node, node_t *rnode, btree_key *key_to_remove, node_t *parent);
        static bool level(block_size_t block_size, node_t *node, node_t *rnode, btree_key *key_to_replace, btree_key *replacement_key, node_t *parent);

        static void print(const node_t *node);

        static void validate(block_size_t block_size, const node_t *node);
};

inline void keycpy(btree_key *dest, const btree_key *src) {
    memcpy(dest, src, sizeof(btree_key) + src->size);
}

inline void valuecpy(btree_value *dest, const btree_value *src) {
    memcpy(dest, src, sizeof(btree_value) + src->mem_size());
}

#endif // __BTREE_NODE_HPP__
