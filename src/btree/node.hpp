#ifndef __BTREE_NODE_HPP__
#define __BTREE_NODE_HPP__

#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include <time.h>
#include "btree/value.hpp"
#include "utils.hpp"
#include "buffer_cache/types.hpp"
#include "store.hpp"

struct btree_superblock_t {
    block_magic_t magic;
    block_id_t root_block;
    block_id_t delete_queue_block;

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
    repli_timestamp last_modified;
    uint16_t earlier[NUM_LEAF_NODE_EARLIER_TIMES];
};

// Note: This struct is stored directly on disk.  Changing it invalidates old data.
// Offsets tested in leaf_node_test.cc
struct leaf_node_t {
    block_magic_t magic;
    leaf_timestamps_t times;
    uint16_t npairs;

    // The smallest offset in pair_offsets
    uint16_t frontmost_offset;
    uint16_t pair_offsets[0];

    static const block_magic_t expected_magic;
};

// Note: Changing this struct changes the format of the data stored on disk.
// If you change this struct, previous stored data will be misinterpreted.
struct btree_key_t {
    uint8_t size;
    char contents[0];
    uint16_t full_size() const {
        return size + offsetof(btree_key_t, contents);
    }
    void print() const {
        printf("%*.*s", size, size, contents);
    }
};

/* A btree_key_t can't safely be allocated because it has a zero-length 'contents' buffer. This is
to represent the fact that its size may vary on disk. A btree_key_buffer_t is a much easier-to-work-
with type. */
struct btree_key_buffer_t {
    btree_key_buffer_t() { }
    btree_key_buffer_t(const btree_key_t *k) {
        btree_key.size = k->size;
        memcpy(btree_key.contents, k->contents, k->size);
    }
    btree_key_buffer_t(const store_key_t &store_key) {
        btree_key.size = store_key.size;
        memcpy(btree_key.contents, store_key.contents, store_key.size);
    }
    btree_key_t *key() { return &btree_key; }
private:
    union {
        btree_key_t btree_key;
        char buffer[sizeof(btree_key_t) + MAX_KEY_SIZE];
    };
};

inline std::string key_to_str(const btree_key_t* key) {
    return std::string(key->contents, key->size);
}

// A node_t is either a btree_internal_node or a btree_leaf_node.
struct node_t {
    block_magic_t magic;
};

template <>
bool check_magic<node_t>(block_magic_t magic);

namespace node {

inline bool is_leaf(const node_t *node) {
    rassert(check_magic<node_t>(node->magic));
    return check_magic<leaf_node_t>(node->magic);
}

inline bool is_internal(const node_t *node) {
    rassert(check_magic<node_t>(node->magic));
    return check_magic<internal_node_t>(node->magic);
}

bool has_sensible_offsets(block_size_t block_size, const node_t *node);
bool is_underfull(block_size_t block_size, const node_t *node);
bool is_mergable(block_size_t block_size, const node_t *node, const node_t *sibling, const internal_node_t *parent);
int nodecmp(const node_t *node1, const node_t *node2);
void split(block_size_t block_size, buf_t &node_buf, buf_t &rnode_buf, btree_key_t *median);
void merge(block_size_t block_size, const node_t *node, buf_t &rnode_buf, btree_key_t *key_to_remove, const internal_node_t *parent);
bool level(block_size_t block_size, buf_t &node_buf, buf_t &rnode_buf, btree_key_t *key_to_replace, btree_key_t *replacement_key, const internal_node_t *parent);

void print(const node_t *node);

void validate(block_size_t block_size, const node_t *node);

}  // namespace node

inline void keycpy(btree_key_t *dest, const btree_key_t *src) {
    memcpy(dest, src, sizeof(btree_key_t) + src->size);
}

inline void valuecpy(btree_value *dest, const btree_value *src) {
    memcpy(dest, src, src->full_size());
}

#endif // __BTREE_NODE_HPP__
