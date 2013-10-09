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
#include "utils.hpp"

template <class Value>
class value_sizer_t;


// Class to hold common use case.
template <>
class value_sizer_t<void> {
public:
    value_sizer_t() { }
    virtual ~value_sizer_t() { }

    virtual int size(const void *value) const = 0;
    virtual bool fits(const void *value, int length_available) const = 0;
    virtual int max_possible_size() const = 0;
    virtual block_magic_t btree_leaf_magic() const = 0;
    virtual block_size_t block_size() const = 0;

private:
    DISABLE_COPYING(value_sizer_t);
};


struct btree_superblock_t {
    block_magic_t magic;
    block_id_t root_block;
    block_id_t stat_block;
    block_id_t sindex_block;

    // We are unnecessarily generous with the amount of space
    // allocated here, but there's nothing else to push out of the
    // way.
    static const int METAINFO_BLOB_MAXREFLEN = 1500;

    char metainfo_blob[METAINFO_BLOB_MAXREFLEN];

    static const block_magic_t expected_magic;
};

struct btree_statblock_t {
    //The total number of keys in the btree
    int64_t population;

    btree_statblock_t()
        : population(0)
    { }
};

struct btree_sindex_block_t {
    static const int SINDEX_BLOB_MAXREFLEN = 4076;

    block_magic_t magic;
    char sindex_blob[SINDEX_BLOB_MAXREFLEN];

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

// A node_t is either a btree_internal_node or a btree_leaf_node.
struct node_t {
    block_magic_t magic;
};

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

bool is_mergable(value_sizer_t<void> *sizer, const node_t *node, const node_t *sibling, const internal_node_t *parent);

bool is_underfull(value_sizer_t<void> *sizer, const node_t *node);

void split(value_sizer_t<void> *sizer, node_t *node, node_t *rnode, btree_key_t *median);

void merge(value_sizer_t<void> *sizer, node_t *node, node_t *rnode, const internal_node_t *parent);

bool level(value_sizer_t<void> *sizer, int nodecmp_node_with_sib, node_t *node, node_t *rnode, btree_key_t *replacement_key, const internal_node_t *parent);

void validate(value_sizer_t<void> *sizer, const node_t *node);

}  // namespace node

inline void keycpy(btree_key_t *dest, const btree_key_t *src) {
    memcpy(dest, src, sizeof(btree_key_t) + src->size);
}

#endif // BTREE_NODE_HPP_
