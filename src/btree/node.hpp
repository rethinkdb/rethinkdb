#ifndef __BTREE_NODE_HPP__
#define __BTREE_NODE_HPP__

#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include <time.h>
#include "btree/value.hpp"
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







typedef store_key_t btree_key;


// A node_t is either a btree_internal_node or a btree_leaf_node.
struct node_t {
    block_magic_t magic;
};

template <>
bool check_magic<node_t>(block_magic_t magic);

namespace node_handler {

inline bool is_leaf(const node_t *node) {
    assert(check_magic<node_t>(node->magic));
    return check_magic<leaf_node_t>(node->magic);
}

inline bool is_internal(const node_t *node) {
    assert(check_magic<node_t>(node->magic));
    return check_magic<internal_node_t>(node->magic);
}

inline bool str_to_key(char *str, btree_key *buf) {
    int len = strlen(str);
    if (len <= MAX_KEY_SIZE) {
        memcpy(buf->contents, str, len);
        buf->size = (uint8_t) len;
        return true;
    } else {
        return false;
    }
}

bool is_underfull(block_size_t block_size, const node_t *node);
bool is_mergable(block_size_t block_size, const node_t *node, const node_t *sibling, const internal_node_t *parent);
int nodecmp(const node_t *node1, const node_t *node2);
void split(block_size_t block_size, node_t *node, node_t *rnode, btree_key *median);
void merge(block_size_t block_size, const node_t *node, node_t *rnode, btree_key *key_to_remove, internal_node_t *parent);
bool level(block_size_t block_size, node_t *node, node_t *rnode, btree_key *key_to_replace, btree_key *replacement_key, internal_node_t *parent);

void print(const node_t *node);

void validate(block_size_t block_size, const node_t *node);

}  // namespace node_handler

inline void keycpy(btree_key *dest, const btree_key *src) {
    memcpy(dest, src, sizeof(btree_key) + src->size);
}

inline void valuecpy(btree_value *dest, const btree_value *src) {
    memcpy(dest, src, sizeof(btree_value) + src->mem_size());
}

#endif // __BTREE_NODE_HPP__
