// Copyright 2010-2013 RethinkDB, all rights reserved.
#ifndef BTREE_NODE_HPP_
#define BTREE_NODE_HPP_

#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include <time.h>

#include <string>

#include "btree/keys.hpp"
#include "buffer_cache/types.hpp"
#include "config/args.hpp"
#include "serializer/types.hpp"
#include "version.hpp"

class value_sizer_t {
public:
    value_sizer_t() { }
    virtual ~value_sizer_t() { }

    virtual int size(const void *value) const = 0;
    virtual bool fits(const void *value, int length_available) const = 0;
    virtual int max_possible_size() const = 0;
    virtual block_magic_t btree_leaf_magic() const = 0;
    virtual max_block_size_t block_size() const = 0;

private:
    DISABLE_COPYING(value_sizer_t);
};

ATTR_PACKED(struct btree_statblock_t {
    //The total number of keys in the btree
    int64_t population;

    btree_statblock_t()
        : population(0)
    { }
});
static const uint32_t BTREE_STATBLOCK_SIZE = sizeof(btree_statblock_t);


//Note: This struct is stored directly on disk.  Changing it invalidates old data.
ATTR_PACKED(struct internal_node_t {
    block_magic_t magic;
    uint16_t npairs;
    uint16_t frontmost_offset;
    uint16_t pair_offsets[0];

    static const block_magic_t expected_magic;
});

// A node_t is either a btree_internal_node or a btree_leaf_node.
ATTR_PACKED(struct node_t {
    block_magic_t magic;
});

namespace node {

inline bool is_internal(const node_t *node) {
    if (node->magic == internal_node_t::expected_magic) {
        return true;
    }
    return false;
}

inline bool is_leaf(const node_t *node) {
    // We assume that a node is a leaf whenever it's not internal.
    // Unfortunately we cannot check the magic directly, because it differs
    // for different value types.
    return !is_internal(node);
}

bool is_mergable(value_sizer_t *sizer, const node_t *node, const node_t *sibling, const internal_node_t *parent);

bool is_underfull(value_sizer_t *sizer, const node_t *node);

void split(value_sizer_t *sizer, node_t *node, node_t *rnode, btree_key_t *median);

void merge(value_sizer_t *sizer, node_t *node, node_t *rnode, const internal_node_t *parent);

void validate(value_sizer_t *sizer, const node_t *node);

}  // namespace node

inline void keycpy(btree_key_t *dest, const btree_key_t *src) {
    memcpy(dest, src, sizeof(btree_key_t) + src->size);
}

#endif // BTREE_NODE_HPP_
