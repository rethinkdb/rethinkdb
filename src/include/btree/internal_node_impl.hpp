#ifndef __BTREE_INTERNAL_NODE_IMPL_HPP__
#define __BTREE_INTERNAL_NODE_IMPL_HPP__

#include "btree/internal_node.hpp"
#include <algorithm>

//#define DEBUG_MAX_INTERNAL 4

//In this tree, less than or equal takes the left-hand branch and greater than takes the right hand branch

void internal_node_handler::init(btree_internal_node *node) {
    node->kind = btree_node_kind_internal;
    node->npairs = 0;
    node->frontmost_offset = BTREE_BLOCK_SIZE;
}

void internal_node_handler::init(btree_internal_node *node, btree_internal_node *lnode, uint16_t *offsets, int numpairs) {
    init(node);
    for (int i = 0; i < numpairs; i++) {
        node->pair_offsets[node->npairs+i] = insert_pair(node, get_pair(lnode, offsets[i]));
    }
    node->npairs += numpairs;
    std::sort(node->pair_offsets, node->pair_offsets+node->npairs, internal_key_comp(node));
}

int internal_node_handler::insert(btree_internal_node *node, btree_key *key, block_id_t lnode, block_id_t rnode) {
    //TODO: write a unit test for this
    check("key too large", key->size > MAX_KEY_SIZE);
    if (is_full(node)) return 0;
    if (node->npairs == 0) {
        btree_key special;
        special.size=0;

        uint16_t special_offset = insert_pair(node, rnode, &special);
        insert_offset(node, special_offset, 0);
    }

    int index = get_offset_index(node, key);
    check("tried to insert duplicate key into internal node!", is_equal(&get_pair(node, node->pair_offsets[index])->key, key));
    uint16_t offset = insert_pair(node, lnode, key);
    insert_offset(node, offset, index);

    get_pair(node, node->pair_offsets[index+1])->lnode = rnode;
    return 1; // XXX
}

block_id_t internal_node_handler::lookup(btree_internal_node *node, btree_key *key) {
    int index = get_offset_index(node, key);
    return get_pair(node, node->pair_offsets[index])->lnode;
}

void internal_node_handler::split(btree_internal_node *node, btree_internal_node *rnode, btree_key *median) {
    uint16_t total_pairs = BTREE_BLOCK_SIZE - node->frontmost_offset;
    uint16_t first_pairs = 0;
    int index = 0;
    while (first_pairs < total_pairs/2) { // finds the median index
        first_pairs += pair_size(get_pair(node, node->pair_offsets[index]));
        index++;
    }
    int median_index = index;

    // Equality takes the left branch, so the median should be from this node.
    btree_key *median_key = &get_pair(node, node->pair_offsets[median_index-1])->key;
    memcpy(median, median_key, sizeof(btree_key)+median_key->size);

    init(rnode, node, node->pair_offsets + median_index, node->npairs - median_index);

    // TODO: This is really slow because most pairs will likely be copied
    // repeatedly.  There should be a better way.
    for (index = median_index; index < node->npairs; index++) {
        delete_pair(node, node->pair_offsets[index]);
    }

    node->npairs = median_index;
    //make last pair special
    make_last_pair_special(node);
}

bool internal_node_handler::remove(btree_internal_node *node, btree_key *key) {
    int index = get_offset_index(node, key);

    make_last_pair_special(node);

    delete_pair(node, node->pair_offsets[index]);
    delete_offset(node, index);

    return true; // XXX
}

bool internal_node_handler::is_full(btree_internal_node *node) {
#ifdef DEBUG_MAX_INTERNAL
    if (node->npairs-1 >= DEBUG_MAX_INTERNAL)
        return true;
#endif
    return sizeof(btree_internal_node) + node->npairs*sizeof(*node->pair_offsets) + sizeof(btree_internal_pair) + MAX_KEY_SIZE >= node->frontmost_offset;
}

void internal_node_handler::validate(btree_internal_node *node) {
    assert((void*)&(node->pair_offsets[node->npairs]) <= (void*)get_pair(node, node->frontmost_offset));
}

size_t internal_node_handler::pair_size(btree_internal_pair *pair) {
    return sizeof(btree_internal_pair) + pair->key.size;
}

btree_internal_pair *internal_node_handler::get_pair(btree_internal_node *node, uint16_t offset) {
    return (btree_internal_pair *)( ((byte *)node) + offset);
}

void internal_node_handler::delete_pair(btree_internal_node *node, uint16_t offset) {
    btree_internal_pair *pair_to_delete = get_pair(node, offset);
    btree_internal_pair *front_pair = get_pair(node, node->frontmost_offset);
    size_t shift = pair_size(pair_to_delete);

    memmove( ((byte *)front_pair)+shift, front_pair, offset - node->frontmost_offset);
    node->frontmost_offset += shift;
    for (int i = 0; i < node->npairs; i++) {
        if (node->pair_offsets[i] < offset)
            node->pair_offsets[i] += shift;
    }
}

uint16_t internal_node_handler::insert_pair(btree_internal_node *node, btree_internal_pair *pair) {
    node->frontmost_offset -= pair_size(pair);
    btree_internal_pair *new_pair = get_pair(node, node->frontmost_offset);

    // insert contents
    memcpy(new_pair, pair, pair_size(pair));

    return node->frontmost_offset;
}

uint16_t internal_node_handler::insert_pair(btree_internal_node *node, block_id_t lnode, btree_key *key) {
    node->frontmost_offset -= sizeof(btree_internal_pair) + key->size;;
    btree_internal_pair *new_pair = get_pair(node, node->frontmost_offset);

    // insert contents
    new_pair->lnode = lnode;
    memcpy(&new_pair->key, key, sizeof(*key) + key->size);

    return node->frontmost_offset;
}

int internal_node_handler::get_offset_index(btree_internal_node *node, btree_key *key) {
    return std::lower_bound(node->pair_offsets, node->pair_offsets+node->npairs, NULL, internal_key_comp(node, key)) - node->pair_offsets;
}

void internal_node_handler::delete_offset(btree_internal_node *node, int index) {
    uint16_t *pair_offsets = node->pair_offsets;
    if (node->npairs > 1)
        memmove(pair_offsets+index, pair_offsets+index+1, (node->npairs-index-1) * sizeof(uint16_t));
    node->npairs--;
}

void internal_node_handler::insert_offset(btree_internal_node *node, uint16_t offset, int index) {
    uint16_t *pair_offsets = node->pair_offsets;
    memmove(pair_offsets+index+1, pair_offsets+index, (node->npairs-index) * sizeof(uint16_t));
    pair_offsets[index] = offset;
    node->npairs++;
}

void internal_node_handler::make_last_pair_special(btree_internal_node *node) {
    int index = node->npairs-1;
    uint16_t old_offset = node->pair_offsets[index];
    btree_key tmp;
    tmp.size = 0;
    node->pair_offsets[index] = insert_pair(node, get_pair(node, old_offset)->lnode, &tmp);
    delete_pair(node, old_offset);
}


bool internal_node_handler::is_equal(btree_key *key1, btree_key *key2) {
    return sized_strcmp(key1->contents, key1->size, key2->contents, key2->size) == 0;
}

#endif // __BTREE_INTERNAL_NODE_IMPL_HPP__
