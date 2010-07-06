#ifndef __BTREE_INTERNAL_NODE_IMPL_HPP__
#define __BTREE_INTERNAL_NODE_IMPL_HPP__

#include "btree/internal_node.hpp"
#include <algorithm>
#include "utils.hpp"

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

block_id_t internal_node_handler::lookup(btree_internal_node *node, btree_key *key) {
    int index = get_offset_index(node, key);
    return get_pair(node, node->pair_offsets[index])->lnode;
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

bool internal_node_handler::remove(btree_internal_node *node, btree_key *key) {
    int index = get_offset_index(node, key);
    delete_pair(node, node->pair_offsets[index]);
    delete_offset(node, index);

    if (index == node->npairs)
        make_last_pair_special(node);

    return true; // XXX
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

void internal_node_handler::merge(btree_internal_node *node, btree_internal_node *rnode, btree_key *key_to_remove, btree_internal_node *parent) {
    //TODO: add asserts
    memmove(rnode->pair_offsets + node->npairs, rnode->pair_offsets, rnode->npairs * sizeof(*rnode->pair_offsets));

    for (int i = 0; i < node->npairs-1; i++) { // the last pair is special
        rnode->pair_offsets[i] = insert_pair(rnode, get_pair(node, node->pair_offsets[i]));
    }
    // get the key in parent which points to node
    btree_key *key_from_parent = &get_pair(parent, parent->pair_offsets[get_offset_index(parent, &get_pair(node, node->pair_offsets[0])->key)])->key;
    rnode->pair_offsets[node->npairs-1] = insert_pair(rnode, get_pair(node, node->pair_offsets[node->npairs-1])->lnode, key_from_parent);
    rnode->npairs += node->npairs;

    keycpy(key_to_remove, &get_pair(rnode, rnode->pair_offsets[0])->key);
}

void internal_node_handler::level(btree_internal_node *node, btree_internal_node *sibling, btree_key *key_to_replace, btree_key *replacement_key, btree_internal_node *parent) {
    //Note: size does not take into account offsets
    uint16_t node_size = BTREE_BLOCK_SIZE - node->frontmost_offset;
    uint16_t sibling_size = BTREE_BLOCK_SIZE - sibling->frontmost_offset;
    uint16_t optimal_adjustment = (sibling_size - node_size) / 2;

    if (nodecmp(node, sibling) < 0) {
        btree_key *key_from_parent = &get_pair(parent, parent->pair_offsets[get_offset_index(parent, &get_pair(sibling, sibling->pair_offsets[0])->key)])->key;
        block_id_t last_offset = get_pair(node, node->pair_offsets[node->npairs-1])->lnode;
        delete_pair(node, node->pair_offsets[node->npairs-1]);
        node->pair_offsets[node->npairs-1] = insert_pair(node, last_offset, key_from_parent);

        int index = -1;
        while (optimal_adjustment > 0) {
            optimal_adjustment -= pair_size(get_pair(sibling, sibling->pair_offsets[++index]));
        }
        check("could not level nodes", index <= 0);
        //copy from beginning of sibling to end of node
        for (int i = 0; i < index; i++) {
            node->pair_offsets[node->npairs+i] = insert_pair(node, get_pair(sibling, sibling->pair_offsets[i]));
        }
        node->npairs += index;

        //TODO: Make this more efficient.  Currently this loop involves repeated memmoves.
        for (int i = 0; i < index; i++) {
            delete_pair(sibling, sibling->pair_offsets[0]);
            delete_offset(sibling, 0);
        }

        keycpy(key_to_replace, &get_pair(node, node->pair_offsets[0])->key);
        keycpy(replacement_key, &get_pair(node, node->pair_offsets[node->npairs-1])->key);
        make_last_pair_special(node);
    } else {
        btree_key *key_from_parent = &get_pair(parent, parent->pair_offsets[get_offset_index(parent, &get_pair(sibling, sibling->pair_offsets[0])->key)])->key;

        // bring key from parent down into the last sibling node
        block_id_t last_offset = get_pair(sibling, sibling->pair_offsets[sibling->npairs-1])->lnode;
        delete_pair(sibling, sibling->pair_offsets[sibling->npairs-1]);
        sibling->pair_offsets[sibling->npairs-1] = insert_pair(sibling, last_offset, key_from_parent);
        
        int index = sibling->npairs;
        while (optimal_adjustment > 0) {
            optimal_adjustment -= pair_size(get_pair(sibling, sibling->pair_offsets[--index]));
        }
        //copy from end of sibling to beginning of node
        int pairs_to_move = sibling->npairs - index - 1;
        check("could not level nodes", pairs_to_move <= 0);
        memmove(node->pair_offsets + pairs_to_move, node->pair_offsets, pairs_to_move * sizeof(*node->pair_offsets));
        for (int i = index; i < sibling->npairs; i++) {
            node->pair_offsets[i-index] = insert_pair(node, get_pair(sibling, sibling->pair_offsets[i]));
        }
        node->npairs += pairs_to_move;

        //TODO: Make this more efficient.  Currently this loop involves repeated memmoves.
        for (int i = index; i < sibling->npairs; i++) {
            delete_pair(sibling, sibling->pair_offsets[index]);
            delete_offset(sibling, index);
        }

        keycpy(key_to_replace, &get_pair(sibling, sibling->pair_offsets[0])->key);
        keycpy(replacement_key, &get_pair(sibling, sibling->pair_offsets[sibling->npairs-1])->key);
        make_last_pair_special(sibling);
    }
}

int internal_node_handler::sibling(btree_internal_node *node, btree_key *key, block_id_t *sib_id) {
    int index = get_offset_index(node, key);
    btree_internal_pair *sib_pair;
    int cmp;
    if (index != 0) {
        sib_pair = get_pair(node, node->pair_offsets[index-1]);
        cmp = 1;
    } else {
        sib_pair = get_pair(node, node->pair_offsets[index+1]);
        cmp = -1;
    }

    *sib_id = sib_pair->lnode;
    return cmp; //equivalent to nodecmp(node, sibling)
}

void internal_node_handler::update_key(btree_internal_node *node, btree_key *key_to_replace, btree_key *replacement_key) {
    int index = get_offset_index(node, key_to_replace);
    block_id_t tmp_lnode = get_pair(node, node->pair_offsets[index])->lnode;
    delete_pair(node, node->pair_offsets[index]);
    node->pair_offsets[index] = insert_pair(node, tmp_lnode, replacement_key);

    check("Invalid key given to update_key: offsets no longer in sorted order", is_sorted(node->pair_offsets, node->pair_offsets+node->npairs, internal_key_comp(node)));
}

bool internal_node_handler::is_full(btree_internal_node *node) {
#ifdef DEBUG_MAX_INTERNAL
    if (node->npairs-1 >= DEBUG_MAX_INTERNAL)
        return true;
#endif
    return sizeof(btree_internal_node) + (node->npairs + 1) * sizeof(*node->pair_offsets) + sizeof(btree_internal_pair) + MAX_KEY_SIZE >= node->frontmost_offset;
}

void internal_node_handler::validate(btree_internal_node *node) {
    assert((void*)&(node->pair_offsets[node->npairs]) <= (void*)get_pair(node, node->frontmost_offset));
}

bool internal_node_handler::is_underfull(btree_internal_node *node) {
#ifdef DEBUG_MAX_LEAF
    if (node->npairs < (DEBUG_MAX_LEAF + 1) / 2)
        return true;
#endif
    return (sizeof(btree_internal_node) + 1) / 2 + 
        node->npairs*sizeof(*node->pair_offsets) +
        (BTREE_BLOCK_SIZE - node->frontmost_offset) < BTREE_BLOCK_SIZE / 2;
}

bool internal_node_handler::is_singleton(btree_internal_node *node) {
    return node->npairs == 1;
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

int internal_node_handler::nodecmp(btree_internal_node *node1, btree_internal_node *node2) {
    btree_key *key1 = &get_pair(node1, node1->pair_offsets[0])->key;
    btree_key *key2 = &get_pair(node2, node2->pair_offsets[0])->key;

    return sized_strcmp(key1->contents, key1->size, key2->contents, key2->size);
}

#endif // __BTREE_INTERNAL_NODE_IMPL_HPP__
