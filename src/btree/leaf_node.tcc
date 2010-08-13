#ifndef __BTREE_LEAF_NODE_TCC__
#define __BTREE_LEAF_NODE_TCC__

#include "btree/leaf_node.hpp"
#include <algorithm>

#define DEBUG_MAX_LEAF 8

void leaf_node_handler::init(btree_leaf_node *node) {
    node->type = btree_node_type_leaf;
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
        long shift = (long)previous->value()->mem_size() - (long)value->mem_size(); //XXX
        if (shift != 0) { //the value is a different size, we need to shift
            shift_pairs(node, prev_offset+pair_size(previous)-previous->value()->mem_size(), shift);
        }
        previous = get_pair(node, node->pair_offsets[index]); //node position changed by shift
        memcpy(previous->value(), value, sizeof(btree_value) + value->mem_size());
    } else {
        uint16_t offset = insert_pair(node, value, key);
        insert_offset(node, offset, index);
    }
    return 1;
}

void leaf_node_handler::remove(btree_leaf_node *node, btree_key *key) {
    int index = find_key(node, key);

    assert(index != -1);
    delete_pair(node, node->pair_offsets[index]);
    delete_offset(node, index);

    validate(node);
    // TODO: Currently this will error incorrectly on root
//    check("leaf became zero size!", node->npairs == 0);
}

bool leaf_node_handler::lookup(btree_leaf_node *node, btree_key *key, btree_value *value) {
    int index = find_key(node, key);
    if (index != -1) {
        block_id_t offset = node->pair_offsets[index];
        btree_leaf_pair *pair = get_pair(node, offset);
        btree_value *stored_value = pair->value();
        memcpy(value, stored_value, sizeof(btree_value) + stored_value->mem_size());
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
    keycpy(median, median_key);

}

void leaf_node_handler::merge(btree_leaf_node *node, btree_leaf_node *rnode, btree_key *key_to_remove) {
#ifdef DELETE_DEBUG
    printf("merging\n");
    printf("node:\n");
    leaf_node_handler::print(node);
    printf("rnode:\n");
    leaf_node_handler::print(rnode);
#endif
    check("leaf nodes too full to merge",
            sizeof(btree_leaf_node) + (node->npairs + rnode->npairs)*sizeof(*node->pair_offsets) +
            BTREE_BLOCK_SIZE - node->frontmost_offset + BTREE_BLOCK_SIZE - rnode->frontmost_offset >= BTREE_BLOCK_SIZE);
    //check("leaf has no pairs!", node->npairs == 0);
    //check("leaf has no pairs!", rnode->npairs == 0);

    memmove(rnode->pair_offsets + node->npairs, rnode->pair_offsets, rnode->npairs * sizeof(*rnode->pair_offsets));

    for (int i = 0; i < node->npairs; i++) {
        rnode->pair_offsets[i] = insert_pair(rnode, get_pair(node, node->pair_offsets[i]));
    }
    rnode->npairs += node->npairs;

    keycpy(key_to_remove, &get_pair(rnode, rnode->pair_offsets[0])->key);
#ifdef DELETE_DEBUG
    printf("\t|\n\t|\n\t|\n\tV\n");
    printf("node:\n");
    leaf_node_handler::print(node);
    printf("rnode:\n");
    leaf_node_handler::print(rnode);
#endif
    validate(node);
}

bool leaf_node_handler::level(btree_leaf_node *node, btree_leaf_node *sibling, btree_key *key_to_replace, btree_key *replacement_key) {
#ifdef DELETE_DEBUG
    printf("leveling\n");
    printf("node:\n");
    leaf_node_handler::print(node);
    printf("sibling:\n");
    leaf_node_handler::print(sibling);
#endif
    //Note: size does not take into account offsets
#ifndef DEBUG_MAX_LEAF
    uint16_t node_size = BTREE_BLOCK_SIZE - node->frontmost_offset;
    uint16_t sibling_size = BTREE_BLOCK_SIZE - sibling->frontmost_offset;
    int optimal_adjustment = (int) (sibling_size - node_size) / 2;
#endif

    if (nodecmp(node, sibling) < 0) {
#ifndef DEBUG_MAX_LEAF
        int index = -1;
        while (optimal_adjustment > 0) {
            optimal_adjustment -= pair_size(get_pair(sibling, sibling->pair_offsets[++index]));
        }
#else
        int index = (sibling->npairs - node->npairs) / 2;
#endif
        if (index <= 0) return false;
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
    } else {
        //first index in the sibling to copy
#ifndef DEBUG_MAX_LEAF
        int index = sibling->npairs;
        while (optimal_adjustment > 0) {
            optimal_adjustment -= pair_size(get_pair(sibling, sibling->pair_offsets[--index]));
        }
#else
        int index = sibling->npairs - (sibling->npairs - node->npairs) / 2;
#endif
        int pairs_to_move = sibling->npairs - index;
        check("could not level nodes", pairs_to_move < 0);
        if (pairs_to_move == 0) return false;
        //copy from end of sibling to beginning of node
        memmove(node->pair_offsets + pairs_to_move, node->pair_offsets, node->npairs * sizeof(*node->pair_offsets));
        for (int i = index; i < sibling->npairs; i++) {
            node->pair_offsets[i-index] = insert_pair(node, get_pair(sibling, sibling->pair_offsets[i]));
        }
        node->npairs += pairs_to_move;

        //TODO: Make this more efficient.  Currently this loop involves repeated memmoves.
        while (index < sibling->npairs) {
            delete_pair(sibling, sibling->pair_offsets[index]); //decrements sibling->npairs
            delete_offset(sibling, index);
        }

        keycpy(key_to_replace, &get_pair(sibling, sibling->pair_offsets[0])->key);
        keycpy(replacement_key, &get_pair(sibling, sibling->pair_offsets[sibling->npairs-1])->key);
    }

#ifdef DELETE_DEBUG
    printf("\t|\n\t|\n\t|\n\tV\n");
    printf("node:\n");
    leaf_node_handler::print(node);
    printf("sibling:\n");
    leaf_node_handler::print(sibling);
#endif
    validate(node);
    validate(sibling);
    return true;
}

bool leaf_node_handler::is_empty(btree_leaf_node *node) {
    return node->npairs == 0;
}

bool leaf_node_handler::is_full(btree_leaf_node *node, btree_key *key, btree_value *value) {
#ifdef DEBUG_MAX_LEAF
    return node->npairs >= DEBUG_MAX_LEAF;
#endif
    // will the data growing from front to right overlap data growing from back to left if we insert
    // the new key value pair
    // TODO: Account for the possibility that the key is already present, in which case we can
    // reuse that space.
    return sizeof(btree_leaf_node) + (node->npairs + 1)*sizeof(*node->pair_offsets) +
        sizeof(btree_leaf_pair) + key->size + value->mem_size() >=
        node->frontmost_offset;
}

void leaf_node_handler::validate(btree_leaf_node *node) {
    assert((void*)&(node->pair_offsets[node->npairs]) <= (void*)get_pair(node, node->frontmost_offset));
    assert(node->frontmost_offset > 0);
    assert(node->frontmost_offset <= BTREE_BLOCK_SIZE);
    for (int i = 0; i < node->npairs; i++) {
        assert(node->pair_offsets[i] < BTREE_BLOCK_SIZE);
        assert(node->pair_offsets[i] >= node->frontmost_offset);
    }
}

bool leaf_node_handler::is_mergable(btree_leaf_node *node, btree_leaf_node *sibling) {
#ifdef DEBUG_MAX_INTERNAL
   return node->npairs + sibling->npairs < DEBUG_MAX_LEAF;
#endif
    return sizeof(btree_leaf_node) + 
        (node->npairs + sibling->npairs)*sizeof(*node->pair_offsets) +
        (BTREE_BLOCK_SIZE - node->frontmost_offset) +
        (BTREE_BLOCK_SIZE - sibling->frontmost_offset) < BTREE_BLOCK_SIZE;
}

bool leaf_node_handler::is_underfull(btree_leaf_node *node) {
#ifdef DEBUG_MAX_LEAF
    return node->npairs < (DEBUG_MAX_LEAF + 1) / 2;
#endif
    return (sizeof(btree_leaf_node) + 1) / 2 + 
        node->npairs*sizeof(*node->pair_offsets) +
        (BTREE_BLOCK_SIZE - node->frontmost_offset) < BTREE_BLOCK_SIZE / 2;
}

size_t leaf_node_handler::pair_size(btree_leaf_pair *pair) {
    return sizeof(btree_leaf_pair) + pair->key.size + pair->value()->mem_size();
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
    node->frontmost_offset -= sizeof(btree_leaf_pair) + key->size + value->mem_size();
    btree_leaf_pair *new_pair = get_pair(node, node->frontmost_offset);

    // insert contents
    keycpy(&new_pair->key, key);
    memcpy(new_pair->value(), value, sizeof(*value) + value->mem_size());

    return node->frontmost_offset;
}

int leaf_node_handler::get_offset_index(btree_leaf_node *node, btree_key *key) {
    // lower_bound returns the first place where the key could be inserted without violating the ordering
    return std::lower_bound(node->pair_offsets, node->pair_offsets+node->npairs, NULL, leaf_key_comp(node, key)) - node->pair_offsets;
}

// find_key returns the index of the offset for key if it's in the node or -1 if it is not
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

int leaf_node_handler::nodecmp(btree_leaf_node *node1, btree_leaf_node *node2) {
    btree_key *key1 = &get_pair(node1, node1->pair_offsets[0])->key;
    btree_key *key2 = &get_pair(node2, node2->pair_offsets[0])->key;

    return sized_strcmp(key1->contents, key1->size, key2->contents, key2->size);
}

void leaf_node_handler::print(btree_leaf_node *node) {
    for (int i = 0; i < node->npairs; i++) {
        btree_leaf_pair *pair = get_pair(node, node->pair_offsets[i]);
        printf("|\t");
        pair->key.print();
    }
    printf("|\n");
    for (int i = 0; i < node->npairs; i++) {
        btree_leaf_pair *pair = get_pair(node, node->pair_offsets[i]);
        printf("|\t");
        pair->value()->print();
    }
    printf("|\n");
}
#endif // __BTREE_LEAF_NODE_TCC__
