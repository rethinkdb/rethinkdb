#ifndef __BTREE_NODE_HPP__
#define __BTREE_NODE_HPP__

#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include <time.h>
#include "utils.hpp"
#include "buffer_cache/types.hpp"

struct btree_superblock_t {
    block_magic_t magic;
    int64_t database_exists;
    block_id_t root_block;

    static block_magic_t expected_magic;
};



//Note: This struct is stored directly on disk.  Changing it invalidates old data.
struct btree_internal_node {
    block_magic_t magic;
    uint16_t npairs;
    uint16_t frontmost_offset;
    uint16_t pair_offsets[0];

    static block_magic_t expected_magic;
};

typedef btree_internal_node internal_node_t;



//Note: This struct is stored directly on disk.  Changing it invalidates old data.
struct btree_leaf_node {
    block_magic_t magic;
    uint16_t npairs;
    uint16_t frontmost_offset; // The smallest offset in pair_offsets
    uint16_t pair_offsets[0];

    static block_magic_t expected_magic;
};

typedef btree_leaf_node leaf_node_t;







enum metadata_flags {
    MEMCACHED_FLAGS   = 0x01,
    MEMCACHED_CAS     = 0x02,
    MEMCACHED_EXPTIME = 0x04,
    // DELETE_QUEUE   = 0x08, // If we implement this.
    LARGE_VALUE       = 0x80
};

// Note: Changing this struct changes the format of the data stored on disk.
// If you change this struct, previous stored data will be misinterpreted.
struct btree_key {
    uint8_t size;
    char contents[0];
    void print() {
        //printf("%*.*s", size, size, contents);
        printf("%d", size);
    }
};

// Note: This struct is stored directly on disk.
struct btree_value {
    uint8_t size;
    byte metadata_flags;
    byte contents[0];

    void init() { // XXX
    }

    uint16_t mem_size() {
        //assert(!is_large());
        return value_offset() + size;
                //(is_large() ? sizeof(block_id_t) + (assert(0),0) : value_size());
    }

    uint32_t value_size() {
        if (is_large()) {
            // Size is stored in contents along with the block ID.
            // TODO: Should the size go here or in the index block? Should other metadata go here?
            // TODO: Is it a good idea to store other metadata in here?
            // TODO: If we really wanted to, we could save a byte by using the size field and only three bytes of contents.
            assert(*lv_size_addr() > MAX_IN_NODE_VALUE_SIZE);
            return *lv_size_addr();
        } else {
            return size;
        }
    }

    void value_size(uint32_t new_size) {
        if (new_size <= MAX_IN_NODE_VALUE_SIZE) {
            if (is_large()) {
                clear_space((byte *) lv_size_addr(), sizeof(uint32_t), lv_size_offset());
                metadata_flags &= ~LARGE_VALUE;
            }
            size = new_size;
        } else {
            if (!is_large()) {
                metadata_flags |= LARGE_VALUE;
                make_space((byte *) lv_size_addr(), sizeof(uint32_t), lv_size_offset());
            }
            size = sizeof(block_id_t);
            *lv_size_addr() = new_size;
        }
    }

    typedef uint32_t mcflags_t;
    typedef uint64_t cas_t;

    // TODO: We assume that time_t can be converted to an exptime_t,
    // which is 32 bits.  We may run into problems in 2038 or 2106.
    typedef uint32_t exptime_t;

    // Every value has mcflags, but they're very often 0, in which case we just
    // store a bit instead of 4 bytes.
    bool has_mcflags() { return metadata_flags & MEMCACHED_FLAGS;   }
    bool has_cas()     { return metadata_flags & MEMCACHED_CAS;     }
    bool has_exptime() { return metadata_flags & MEMCACHED_EXPTIME; }
    bool is_large()    { return metadata_flags & LARGE_VALUE;       }

    uint8_t mcflags_offset() { return 0;                                                    }
    uint8_t exptime_offset() { return mcflags_offset() + sizeof(mcflags_t) * has_mcflags(); }
    uint8_t cas_offset()     { return exptime_offset() + sizeof(exptime_t) * has_exptime(); }
    uint8_t lv_size_offset() { return     cas_offset() + sizeof(cas_t)     * has_cas();     }
    uint8_t value_offset()   { return lv_size_offset() + sizeof(uint32_t)  * is_large(); }

    mcflags_t *mcflags_addr() { return (mcflags_t *) (contents + mcflags_offset()); }
    exptime_t *exptime_addr() { return (exptime_t *) (contents + exptime_offset()); }
    cas_t         *cas_addr() { return (cas_t     *) (contents +     cas_offset()); }
    uint32_t  *lv_size_addr() { return (uint32_t  *) (contents + lv_size_offset()); }
    byte             *value() { return (byte      *) (contents +   value_offset()); }

    block_id_t lv_index_block_id() { return * (block_id_t *) value(); }
    void set_lv_index_block_id(block_id_t block_id) {
        assert(is_large());
        *(block_id_t *) value() = block_id;
    }

    mcflags_t mcflags() { return has_mcflags() ? *mcflags_addr() : 0; }
    exptime_t exptime() { return has_exptime() ? *exptime_addr() : 0; }
    cas_t         cas() { // We shouldn't be asking for a value's CAS unless we know it has one.
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

    void print() {
        printf("%*.*s", size, size, value());
    }
};

// A btree_node is either a btree_internal_node or a btree_leaf_node.
//
// Note: &type == &internal.type == &leaf.type.
//
// Note: The reason this is not a struct btree_node { btree_node_type
// tag; union { btree_internal_node internal; btree_leaf_node leaf; }
// u; } is that btree_node used to superclass
// btree_{leaf|internal}_node, with one member, type.  A lot of code
// freely casts between a subclass and parent class type.
struct btree_node {
    block_magic_t magic;
};

template <>
bool check_magic<btree_node>(block_magic_t magic);


typedef btree_node node_t;

class node_handler {
    public:
        static bool is_leaf(const btree_node *node) {
            assert(check_magic<btree_node>(node->magic));
            return check_magic<btree_leaf_node>(node->magic);
        }

        static bool is_internal(const btree_node *node) {
            assert(check_magic<btree_node>(node->magic));
            return check_magic<btree_internal_node>(node->magic);
        }

        static void str_to_key(char *str, btree_key *buf) {
            int len = strlen(str);
            check("string too long", len > MAX_KEY_SIZE);
            memcpy(buf->contents, str, len);
            buf->size = (unsigned char)len;
        }

        static bool is_underfull(size_t block_size, const btree_node *node);
        static bool is_mergable(size_t block_size, const btree_node *node, const btree_node *sibling, const btree_node *parent);
        static int nodecmp(const btree_node *node1, const btree_node *node2);
        static void merge(size_t block_size, btree_node *node, btree_node *rnode, btree_key *key_to_remove, btree_node *parent);
        static void remove(size_t block_size, btree_node *node, btree_key *key);
        static bool level(size_t block_size, btree_node *node, btree_node *rnode, btree_key *key_to_replace, btree_key *replacement_key, btree_node *parent);

        static void print(const btree_node *node);
        
        static void validate(size_t block_size, const btree_node *node);
        
        static inline const btree_node* node(const void *ptr) {
            return (const btree_node *) ptr;
        }
        static inline btree_node* node(void *ptr) {
            return (btree_node *) ptr;
        }
};

inline void keycpy(btree_key *dest, btree_key *src) {
    memcpy(dest, src, sizeof(btree_key) + src->size);
}

inline void valuecpy(btree_value *dest, btree_value *src) {
    memcpy(dest, src, sizeof(btree_value) + src->mem_size());
}

#endif // __BTREE_NODE_HPP__
