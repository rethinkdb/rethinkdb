#ifndef __BTREE_LEAF_NODE_IMPL_HPP__
#define __BTREE_LEAF_NODE_IMPL_HPP__

#include "btree/leaf_node.hpp"
#include <algorithm>

void leaf_node_handler::init(btree_leaf_node *node) {
    node->kind = btree_node_kind_leaf;
    node->npairs = 0;
    node->frontmost_offset = BTREE_BLOCK_SIZE;
}

void leaf_node_handler::init(btree_leaf_node *node, btree_leaf_node *lnode, uint16_t *offsets, int numpairs) {
    init(node);
    for (int i = 0; i < numpairs; i++) {
        node->pair_offsets[node->npairs+i] = insert_pair(node, get_pair(lnode, offsets[i]));
    }
    node->npairs += numpairs;
    std::sort(node->pair_offsets, node->pair_offsets+node->npairs, leaf_key_comp(node));
}

int leaf_node_handler::insert(btree_leaf_node *node, btree_key *key, btree_value* value) {
    if (is_full(node, key, value)) return 0;
    int index = get_offset_index(node, key);
    uint16_t prev_offset = node->pair_offsets[index];
    btree_leaf_pair *previous = NULL;
    if (index != node->npairs)
        previous = get_pair(node, prev_offset);
    //TODO: write a unit test for this
    if (previous != NULL && is_equal(&previous->key, key)) { // a duplicate key is being inserted
        long shift = (long)previous->value()->size - (long)value->size; //XXX
        if (shift != 0) { //the value is a different size, we need to shift
            shift_pairs(node, prev_offset+pair_size(previous)-previous->value()->size, shift);
        }
        previous = get_pair(node, node->pair_offsets[index]); //node position changed by shift
        memcpy(previous->value(), value, sizeof(btree_value) + value->size);
    } else {
        uint16_t offset = insert_pair(node, value, key);
        insert_offset(node, offset, index);
    }
    return 1;
}

bool leaf_node_handler::lookup(btree_leaf_node *node, btree_key *key, btree_value *value) {
    int index = get_offset_index(node, key);
    block_id_t offset = node->pair_offsets[index];
    btree_leaf_pair *pair = get_pair(node, offset);
    if (is_equal(&pair->key, key)) {
        btree_value *stored_value = pair->value();
        memcpy(value, stored_value, sizeof(btree_value) + stored_value->size);
        return true;
    } else {
        return false;
    }
}

void leaf_node_handler::split(btree_leaf_node *node, btree_leaf_node *rnode, btree_key *median) {
    uint16_t total_pairs = BTREE_BLOCK_SIZE - node->frontmost_offset;
    uint16_t first_pairs = 0;
    int index = 0;
    while (first_pairs < total_pairs/2) { // finds the median index
        first_pairs += pair_size(get_pair(node, node->pair_offsets[index]));
        index++;
    }
    int median_index = index;

    init(rnode, node, node->pair_offsets + median_index, node->npairs - median_index);

    // TODO: This is really slow because most pairs will likely be copied
    // repeatedly.  There should be a better way.
    for (index = median_index; index < node->npairs; index++) {
        delete_pair(node, node->pair_offsets[index]);
    }

    node->npairs = median_index;

    // Equality takes the left branch, so the median should be from this node.
    btree_key *median_key = &get_pair(node, node->pair_offsets[median_index-1])->key;
    memcpy(median, median_key, sizeof(btree_key)+median_key->size);

}

bool leaf_node_handler::remove(btree_leaf_node *node, btree_key *key) {
    int index = find_key(node, key);

    if (index != -1) {
        delete_pair(node, node->pair_offsets[index]);
        delete_offset(node, index);
        return true;
    } else {
        return false;
    }
}

bool leaf_node_handler::is_full(btree_leaf_node *node, btree_key *key, btree_value *value) {
#ifdef DEBUG_MAX_LEAF
    if (node->npairs >= DEBUG_MAX_LEAF)
        return true;
#endif
    return sizeof(btree_leaf_node) +
        node->npairs*sizeof(*node->pair_offsets) +
        sizeof(btree_leaf_pair) + key->size + value->size + 1 >= node->frontmost_offset;
}

void leaf_node_handler::validate(btree_leaf_node *node) {
    assert((void*)&(node->pair_offsets[node->npairs]) <= (void*)get_pair(node, node->frontmost_offset));
}

size_t leaf_node_handler::pair_size(btree_leaf_pair *pair) {
    return sizeof(btree_leaf_pair) + pair->key.size + pair->value()->size;
}

btree_leaf_pair *leaf_node_handler::get_pair(btree_leaf_node *node, uint16_t offset) {
    return (btree_leaf_pair *)( ((byte *)node) + offset);
}

void leaf_node_handler::shift_pairs(btree_leaf_node *node, uint16_t offset, long shift) {
    btree_leaf_pair *front_pair = get_pair(node, node->frontmost_offset);

    memmove( ((byte *)front_pair)+shift, front_pair, offset - node->frontmost_offset);
    node->frontmost_offset += shift;
    for (int i = 0; i < node->npairs; i++) {
        if (node->pair_offsets[i] <= offset)
            node->pair_offsets[i] += shift;
    }
}

void leaf_node_handler::delete_pair(btree_leaf_node *node, uint16_t offset) {
    btree_leaf_pair *pair_to_delete = get_pair(node, offset);
    size_t shift = pair_size(pair_to_delete);
    shift_pairs(node, offset, shift);
}

uint16_t leaf_node_handler::insert_pair(btree_leaf_node *node, btree_leaf_pair *pair) {
    node->frontmost_offset -= pair_size(pair);
    btree_leaf_pair *new_pair = get_pair(node, node->frontmost_offset);

    // insert contents
    memcpy(new_pair, pair, pair_size(pair));

    return node->frontmost_offset;
}

uint16_t leaf_node_handler::insert_pair(btree_leaf_node *node, btree_value *value, btree_key *key) {
    node->frontmost_offset -= sizeof(btree_leaf_pair) + key->size + value->size;
    btree_leaf_pair *new_pair = get_pair(node, node->frontmost_offset);

    // insert contents
    memcpy(&new_pair->key, key, sizeof(*key) + key->size);
    memcpy(new_pair->value(), value, sizeof(*value) + value->size);

    return node->frontmost_offset;
}

int leaf_node_handler::get_offset_index(btree_leaf_node *node, btree_key *key) {
    return std::lower_bound(node->pair_offsets, node->pair_offsets+node->npairs, NULL, leaf_key_comp(node, key)) - node->pair_offsets;
}

int leaf_node_handler::find_key(btree_leaf_node *node, btree_key *key) {
    int index = get_offset_index(node, key);
    if (index < node->npairs && is_equal(key, &get_pair(node, node->pair_offsets[index])->key) ) {
        return index;
    } else {
        return -1;
    }

}
void leaf_node_handler::delete_offset(btree_leaf_node *node, int index) {
    uint16_t *pair_offsets = node->pair_offsets;
    if (node->npairs > 1)
        memmove(pair_offsets+index, pair_offsets+index+1, (node->npairs-index-1) * sizeof(uint16_t));
    node->npairs--;
}

void leaf_node_handler::insert_offset(btree_leaf_node *node, uint16_t offset, int index) {
    uint16_t *pair_offsets = node->pair_offsets;
    memmove(pair_offsets+index+1, pair_offsets+index, (node->npairs-index) * sizeof(uint16_t));
    pair_offsets[index] = offset;
    node->npairs++;
}


bool leaf_node_handler::is_equal(btree_key *key1, btree_key *key2) {
    return sized_strcmp(key1->contents, key1->size, key2->contents, key2->size) == 0;
}

#endif // __BTREE_LEAF_NODE_IMPL_HPP__
