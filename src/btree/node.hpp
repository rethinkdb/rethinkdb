#ifndef __BTREE_NODE_HPP__
#define __BTREE_NODE_HPP__

#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include <time.h>
#include "utils.hpp"
#include "buffer_cache/types.hpp"

struct btree_superblock_t {
    int database_exists;
    block_id_t root_block;
};

#define MAX_KEY_SIZE 250

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

struct btree_value {
    uint8_t size;
    byte metadata_flags;
    byte contents[0];

    uint16_t mem_size() {
        //assert(!large_value());
        return value_offset() + size;
                //(large_value() ? sizeof(block_id_t) + (assert(0),0) : value_size());
    }

    uint32_t value_size() {
        if (large_value()) {
            // Size is stored in contents along with the block ID.
            // TODO: Should the size go here or in the index block? Should other metadata go here?
            // TODO: Is it a good idea to store other metadata in here?
            // TODO: If we really wanted to, we could save a byte by using the size field and only three bytes of contents.
            return *lv_size_addr();
        } else {
            return size;
        }
    }

    void value_size(uint32_t new_size) {
        if (new_size <= MAX_IN_NODE_VALUE_SIZE) {
            if (large_value()) {
                clear_space((byte *) lv_size_addr(), sizeof(uint32_t), lv_size_offset());
                metadata_flags &= ~LARGE_VALUE;
            }
            size = new_size;
        } else {
            if (!large_value()) {
                metadata_flags |= LARGE_VALUE;
                make_space((byte *) lv_size_addr(), sizeof(uint32_t), lv_size_offset());
            }
            size = sizeof(block_id_t);
            *lv_size_addr() = new_size;
        }
    }

    typedef uint32_t mcflags_t;
    typedef uint64_t cas_t;
    typedef uint32_t exptime_t;

    // Every value has mcflags, but they're very often 0, in which case we just
    // store a bit instead of 4 bytes.
    bool has_mcflags() { return metadata_flags & MEMCACHED_FLAGS;   }
    bool has_cas()     { return metadata_flags & MEMCACHED_CAS;     }
    bool has_exptime() { return metadata_flags & MEMCACHED_EXPTIME; }
    bool large_value() { return metadata_flags & LARGE_VALUE;       }

    uint8_t mcflags_offset() { return 0;                                                    }
    uint8_t exptime_offset() { return mcflags_offset() + sizeof(mcflags_t) * has_mcflags(); }
    uint8_t cas_offset()     { return exptime_offset() + sizeof(exptime_t) * has_exptime(); }
    uint8_t lv_size_offset() { return     cas_offset() + sizeof(cas_t)     * has_cas();     }
    uint8_t value_offset()   { return lv_size_offset() + sizeof(uint32_t)  * large_value(); }

    mcflags_t *mcflags_addr() { return (mcflags_t *) (contents + mcflags_offset()); }
    exptime_t *exptime_addr() { return (exptime_t *) (contents + exptime_offset()); }
    cas_t         *cas_addr() { return (cas_t     *) (contents +     cas_offset()); }
    uint32_t  *lv_size_addr() { return (uint32_t  *) (contents + lv_size_offset()); }
    byte             *value() { return (byte      *) (contents +   value_offset()); }

    block_id_t lv_index_block_id() { return * (block_id_t *) value(); }
    void set_lv_index_block_id(block_id_t block_id) {
        assert(large_value());
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

typedef enum {
    // Choose 1 and 2 instead of 0 and 1 to make it less likely that garbage will be interpreted as
    // a valid node
    btree_node_type_leaf = 1,
    btree_node_type_internal = 2
} btree_node_type;

struct btree_node {
    btree_node_type type;
};

typedef btree_node node_t;

class node_handler {
    public:
        static bool is_leaf(const btree_node *node) {
            validate(node);
            return node->type == btree_node_type_leaf;
        }

        static bool is_internal(const btree_node *node) {
            validate(node);
            return node->type == btree_node_type_internal;
        }

        static void str_to_key(char *str, btree_key *buf) {
            int len = strlen(str);
            check("string too long", len > MAX_KEY_SIZE);
            memcpy(buf->contents, str, len);
            buf->size = (unsigned char)len;
        }

        static bool is_underfull(const btree_node *node);
        static bool is_mergable(const btree_node *node, const btree_node *sibling, const btree_node *parent);
        static int nodecmp(const btree_node *node1, const btree_node *node2);
        static void merge(btree_node *node, btree_node *rnode, btree_key *key_to_remove, btree_node *parent);
        static void remove(btree_node *node, btree_key *key);
        static bool level(btree_node *node, btree_node *rnode, btree_key *key_to_replace, btree_key *replacement_key, btree_node *parent);

        static void print(const btree_node *node);
        
        static void validate(const btree_node *node);
        
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
