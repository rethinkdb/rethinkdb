// Copyright 2010-2013 RethinkDB, all rights reserved.
#ifndef BTREE_INTERNAL_NODE_HPP_
#define BTREE_INTERNAL_NODE_HPP_

#include <vector>

#include "arch/compiler.hpp"
#include "btree/keys.hpp"
#include "serializer/types.hpp"
#include "utils.hpp"

struct internal_node_t;

// See internal_node_t in node.hpp

/* EPSILON used to prevent split then merge */
#define INTERNAL_EPSILON (sizeof(btree_key_t) + MAX_KEY_SIZE + sizeof(block_id_t))

//Note: This struct is stored directly on disk.  Changing it invalidates old data.
ATTR_PACKED(struct alignas(1) btree_internal_pair {
    block_id_t lnode;
    btree_key_t key;
});

class internal_key_comp;

// In a perfect world, this namespace would be 'branch'.
namespace internal_node {

void init(block_size_t block_size, internal_node_t *node);
void init(block_size_t block_size, internal_node_t *node, const internal_node_t *lnode, const uint16_t *offsets, int numpairs);

block_id_t lookup(const internal_node_t *node, const btree_key_t *key);
bool insert(internal_node_t *node, const btree_key_t *key, block_id_t lnode, block_id_t rnode);
bool remove(block_size_t block_size, internal_node_t *node, const btree_key_t *key);
void split(block_size_t block_size, internal_node_t *node, internal_node_t *rnode, btree_key_t *median);
void merge(block_size_t block_size, const internal_node_t *node, internal_node_t *rnode, const internal_node_t *parent);
bool level(block_size_t block_size, internal_node_t *node, internal_node_t *sibling,
           btree_key_t *replacement_key, const internal_node_t *parent,
           std::vector<block_id_t> *moved_children_out);
int sibling(const internal_node_t *node, const btree_key_t *key, block_id_t *sib_id, store_key_t *key_in_middle_out);
void update_key(internal_node_t *node, const btree_key_t *key_to_replace, const btree_key_t *replacement_key);
int nodecmp(const internal_node_t *node1, const internal_node_t *node2);
bool is_full(const internal_node_t *node);
bool is_underfull(block_size_t block_size, const internal_node_t *node);
bool change_unsafe(const internal_node_t *node);
bool is_mergable(block_size_t block_size, const internal_node_t *node, const internal_node_t *sibling, const internal_node_t *parent);
bool is_doubleton(const internal_node_t *node);

void validate(block_size_t block_size, const internal_node_t *node);

size_t pair_size(const btree_internal_pair *pair);
const btree_internal_pair *get_pair(const internal_node_t *node, uint16_t offset);
btree_internal_pair *get_pair(internal_node_t *node, uint16_t offset);

const btree_internal_pair *get_pair_by_index(const internal_node_t *node, int index);
btree_internal_pair *get_pair_by_index(internal_node_t *node, int index);

int get_offset_index(const internal_node_t *node, const btree_key_t *key);

}  // namespace internal_node

class internal_key_comp {
    const internal_node_t *node;
    const btree_key_t *key;
public:
    enum { faux_offset = 0 };

    explicit internal_key_comp(const internal_node_t *_node) : node(_node), key(nullptr)  { }
    internal_key_comp(const internal_node_t *_node, const btree_key_t *_key) : node(_node), key(_key)  { }
    bool operator()(const uint16_t offset1, const uint16_t offset2) {
        const btree_key_t *key1 = offset1 == faux_offset ? key : &internal_node::get_pair(node, offset1)->key;
        const btree_key_t *key2 = offset2 == faux_offset ? key : &internal_node::get_pair(node, offset2)->key;
        return compare(key1, key2) < 0;
    }
    static int compare(const btree_key_t *key1, const btree_key_t *key2) {
        return btree_key_cmp(key1, key2);
    }
};



#endif // BTREE_INTERNAL_NODE_HPP_
