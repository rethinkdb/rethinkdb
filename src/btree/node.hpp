#ifndef __BTREE_NODE_HPP__
#define __BTREE_NODE_HPP__

#include <stdint.h>
#include <string.h>
#include <assert.h>
#include "arch/resource.hpp"

#define MAX_KEY_SIZE 250

enum metadata_flags {
    MEMCACHED_FLAGS   = 0x01,
    MEMCACHED_CAS     = 0x02,
    MEMCACHED_EXPTIME = 0x04
};

// Note: Changing this struct changes the format of the data stored on disk.
// If you change this struct, previous stored data will be misinterpreted.
struct btree_key {
    uint8_t size;
    char contents[0];
    void print() {
        printf("%.*s", size, contents);
    }
};

struct btree_value {
    uint32_t size;
    byte metadata_flags;
    char contents[0];

    uint16_t mem_size() {
        return (size <= MAX_TOTAL_NODE_CONTENTS_SIZE) ? size : sizeof(block_id_t);
    }

    typedef uint32_t mcflags_t;
    typedef uint64_t cas_t;

    uint32_t value_size() {
        return size - value_offset();
    }
    void value_size(uint32_t new_size) {
        size = new_size + value_offset();
    }

    uint8_t value_offset() {
        return sizeof(mcflags_t) * has_mcflags() + sizeof(cas_t) * has_cas();
    }

    byte *value() {
        return contents + value_offset();
    }

    // Every value has mcflags, but they're very often 0, in which case we just
    // store a bit instead of 4 bytes.
    bool has_mcflags() {
        return metadata_flags & MEMCACHED_FLAGS;
    }
    uint8_t mcflags_offset() {
        assert(has_mcflags());
        return 0;
    }
    mcflags_t mcflags() {
        if (has_mcflags()) {
            mcflags_t flags;
            memcpy(&flags, contents + mcflags_offset(), sizeof(mcflags_t));
            return flags;
        } else {
            return 0;
        }
    }
    void set_mcflags(mcflags_t new_mcflags) {
        if (has_mcflags() && new_mcflags == 0) { // Flags is being set to 0, so we clear the 4 bytes we kept for it.
            memmove(contents + mcflags_offset(), contents + mcflags_offset() + sizeof(mcflags_t), size - mcflags_offset() - sizeof(mcflags_t));
            size -= sizeof(mcflags_t);
            metadata_flags &= ~MEMCACHED_FLAGS;
        } else if (!has_mcflags() && new_mcflags) { // Make space for non-zero mcflags. XXX This assumes there's enough space allocated to move the value into.
            assert(size + sizeof(mcflags_t) <= MAX_TOTAL_NODE_CONTENTS_SIZE);
            metadata_flags |= MEMCACHED_FLAGS;
            size += sizeof(mcflags_t);
            memmove(contents + mcflags_offset() + sizeof(mcflags_t), contents + mcflags_offset(), size - mcflags_offset());
        }
        if (new_mcflags) { // We've made space, so copy the mcflags over.
            memcpy(contents + mcflags_offset(), &new_mcflags, sizeof(mcflags_t));
        }
    }

    // Values initially don't have a CAS. Once it's added, though, we assume it's there for good.
    bool has_cas() {
        return metadata_flags & MEMCACHED_CAS;
    }
    uint8_t cas_offset() {
        assert(has_cas());
        return sizeof(mcflags_t) * has_mcflags();
    }
    void set_cas(cas_t new_cas) {
        assert(has_cas());
        memcpy(contents + cas_offset(), &new_cas, sizeof(new_cas));
    }
    void add_cas() { // XXX This assumes that there's enough space allocated to move the value into.
        assert(size + sizeof(cas_t) <= MAX_TOTAL_NODE_CONTENTS_SIZE);
        metadata_flags |= MEMCACHED_CAS;
        memmove(contents + cas_offset() + sizeof(cas_t), contents + cas_offset(), size - cas_offset());
        size += sizeof(cas_t);
        // XXX This doesn't actually set a CAS value, only makes the space for one.
    }
    cas_t cas() {
        assert(has_cas());
        cas_t cas;
        memcpy(&cas, contents + cas_offset(), sizeof(cas_t));
        return cas;
    }

    void print() {
        printf("%.*s\n", value_size(), value());
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

class node_handler {
    public:
        static bool is_leaf(btree_node *node) {
            validate(node);
            return node->type == btree_node_type_leaf;
        }

        static bool is_internal(btree_node *node) {
            validate(node);
            return node->type == btree_node_type_internal;
        }

        static void str_to_key(char *str, btree_key *buf) {
            int len = strlen(str);
            check("string too long", len > MAX_KEY_SIZE);
            memcpy(buf->contents, str, len);
            buf->size = (unsigned char)len;
        }
        
        static void validate(btree_node *node);
};

inline void keycpy(btree_key *dest, btree_key *src) {
    memcpy(dest, src, sizeof(btree_key) + src->size);
}
#endif // __BTREE_NODE_HPP__
