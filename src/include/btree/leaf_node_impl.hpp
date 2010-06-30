#ifndef __BTREE_LEAF_NODE_IMPL_HPP__
#define __BTREE_LEAF_NODE_IMPL_HPP__

#include "btree/leaf_node.hpp"
#include <algorithm>

void leaf_node_handler::init(btree_leaf_node *node) {
    printf("leaf node %p: init\n", node);
    node->leaf = true;
    node->npairs = 0;
    node->frontmost_offset = BTREE_BLOCK_SIZE;
}

void leaf_node_handler::init(btree_leaf_node *node, btree_leaf_node *lnode, uint16_t *offsets, int numpairs) {
    printf("leaf node %p: init from node %p\n", node, lnode);
    init(node);
    for (int i = 0; i < numpairs; i++) {
        node->pair_offsets[node->npairs+i] = insert_pair(node, get_pair(lnode, offsets[i]));
    }
    node->npairs += numpairs;
    std::sort(node->pair_offsets, node->pair_offsets+node->npairs, leaf_key_comp(node));
}

int leaf_node_handler::insert(btree_leaf_node *node, btree_key *key, int value) {
    printf("leaf node %p: insert\tkey:%*.*s\tvalue:%d\n", node, key->size, key->size, key->contents, value);
    if (is_full(node, key)) return 0;
    int index = get_offset_index(node, key);
    btree_leaf_pair *previous = get_pair(node, node->pair_offsets[index]);
    //TODO: write a unit test for this
    if (is_equal(&previous->key, key)) { // a duplicate key is being inserted
        previous->value = value;
    } else {
        uint16_t offset = insert_pair(node, value, key);
        insert_offset(node, offset, index);
    }
    return 1;
}

bool leaf_node_handler::lookup(btree_leaf_node *node, btree_key *key, int *value) {
    printf("leaf node %p: lookup\tkey:%*.*s\n", node, key->size, key->size, key->contents);
    int index = get_offset_index(node, key);
    block_id_t offset = node->pair_offsets[index];
    btree_leaf_pair *pair = get_pair(node, offset);
    if (is_equal(&pair->key, key)) {
        *value = pair->value;
        return true;
    } else {
        return false;
    }
}

void leaf_node_handler::split(btree_leaf_node *node, btree_leaf_node *rnode, btree_key *median) {
    printf("leaf node %p: split\n", node);
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

bool leaf_node_handler::is_full(btree_leaf_node *node, btree_key *key) {
#ifdef DEBUG_MAX_LEAF
    if (node->npairs >= DEBUG_MAX_LEAF)
        return true;
#endif
    return sizeof(btree_leaf_node) + node->npairs*sizeof(*node->pair_offsets) + sizeof(btree_leaf_pair) + key->size + 1 >= node->frontmost_offset;
}

size_t leaf_node_handler::pair_size(btree_leaf_pair *pair) {
    return sizeof(btree_leaf_pair) + pair->key.size;
}

btree_leaf_pair *leaf_node_handler::get_pair(btree_leaf_node *node, uint16_t offset) {
    return (btree_leaf_pair *)( ((byte *)node) + offset);
}

void leaf_node_handler::delete_pair(btree_leaf_node *node, uint16_t offset) {
    btree_leaf_pair *pair_to_delete = get_pair(node, offset);
    btree_leaf_pair *front_pair = get_pair(node, node->frontmost_offset);
    size_t shift = pair_size(pair_to_delete);

    memmove( ((byte *)front_pair)+shift, front_pair, offset - node->frontmost_offset);
    node->frontmost_offset += shift;
    for (int i = 0; i < node->npairs; i++) {
        if (node->pair_offsets[i] < offset)
            node->pair_offsets[i] += shift;
    }
}

uint16_t leaf_node_handler::insert_pair(btree_leaf_node *node, btree_leaf_pair *pair) {
    node->frontmost_offset -= pair_size(pair);
    btree_leaf_pair *new_pair = get_pair(node, node->frontmost_offset);

    // insert contents
    memcpy(new_pair, pair, pair_size(pair));

    return node->frontmost_offset;
}

uint16_t leaf_node_handler::insert_pair(btree_leaf_node *node, int value, btree_key *key) {
    node->frontmost_offset -= sizeof(btree_leaf_pair) + key->size;;
    btree_leaf_pair *new_pair = get_pair(node, node->frontmost_offset);

    // insert contents
    new_pair->value = value;
    memcpy(&new_pair->key, key, sizeof(*key) + key->size);

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
