
#include "btree/leaf_node.hpp"
#include <algorithm>

#include "logger.hpp"

//#define DEBUG_MAX_LEAF 10

void leaf_node_handler::init(block_size_t block_size, leaf_node_t *node, repli_timestamp modification_time) {
    node->magic = leaf_node_t::expected_magic;
    node->npairs = 0;
    node->frontmost_offset = block_size.value();
    initialize_times(&node->times, modification_time);
}

// TODO: We end up making modification time data more conservative and
// more coarse than conceivably possible.  We could also let the
// caller supply an earlier[] array.
// TODO: maybe lnode should just supply the modification time.
void leaf_node_handler::init(block_size_t block_size, leaf_node_t *node, leaf_node_t *lnode, uint16_t *offsets, int numpairs, repli_timestamp modification_time) {
    init(block_size, node, modification_time);
    for (int i = 0; i < numpairs; i++) {
        node->pair_offsets[i] = insert_pair(node, get_pair(lnode, offsets[i]));
    }
    node->npairs = numpairs;
    std::sort(node->pair_offsets, node->pair_offsets+node->npairs, leaf_key_comp(node));
}

bool leaf_node_handler::insert(block_size_t block_size, leaf_node_t *node, btree_key *key, btree_value* value, repli_timestamp insertion_time) {
    if (is_full(node, key, value)) return false;
    int index = get_offset_index(node, key);
    uint16_t prev_offset = node->pair_offsets[index];
    btree_leaf_pair *previous = NULL;
    if (index != node->npairs)
        previous = get_pair(node, prev_offset);
    //TODO: write a unit test for this
    if (previous != NULL && is_equal(&previous->key, key)) {
        // A duplicate key is being inserted.  Let's shift the old
        // key/value pair away _completely_ and put the new one at the
        // beginning.

        int prev_timestamp_offset = get_timestamp_offset(block_size, node, prev_offset);

        rotate_time(&node->times, insertion_time, prev_timestamp_offset);

        delete_pair(node, prev_offset);
        node->pair_offsets[index] = insert_pair(node, value, key);
    } else {
        // manipulate timestamps.
        rotate_time(&node->times, insertion_time, NUM_LEAF_NODE_EARLIER_TIMES);

        uint16_t offset = insert_pair(node, value, key);
        insert_offset(node, offset, index);
    }
    validate(block_size, node);
    return true;
}

void leaf_node_handler::remove(block_size_t block_size, leaf_node_t *node, btree_key *key) {
#ifdef BTREE_DEBUG
    printf("removing key: ");
    key->print();
    printf("\n");
    leaf_node_handler::print(node);
#endif
    int index = find_key(node, key);
    assert(index != -1);

    uint16_t offset = node->pair_offsets[index];
    remove_time(&node->times, get_timestamp_offset(block_size, node, offset));

    delete_pair(node, offset);
    delete_offset(node, index);

#ifdef BTREE_DEBUG
    printf("\t|\n\t|\n\t|\n\tV\n");
    leaf_node_handler::print(node);
#endif

    validate(block_size, node);
    // TODO: Currently this will error incorrectly on root
//    check("leaf became zero size!", node->npairs == 0);
}

bool leaf_node_handler::lookup(const leaf_node_t *node, btree_key *key, btree_value *value) {
    int index = find_key(node, key);
    if (index != -1) {
        uint16_t offset = node->pair_offsets[index];
        btree_leaf_pair *pair = get_pair(node, offset);
        btree_value *stored_value = pair->value();
        memcpy(value, stored_value, sizeof(btree_value) + stored_value->mem_size());
        return true;
    } else {
        return false;
    }
}

void leaf_node_handler::split(block_size_t block_size, leaf_node_t *node, leaf_node_t *rnode, btree_key *median) {
    uint16_t total_pairs = block_size.value() - node->frontmost_offset;
    uint16_t first_pairs = 0;
    int index = 0;
    while (first_pairs < total_pairs/2) { // finds the median index
        first_pairs += pair_size(get_pair(node, node->pair_offsets[index]));
        index++;
    }
    int median_index = index;

    init(block_size, rnode, node, node->pair_offsets + median_index, node->npairs - median_index, node->times.last_modified);

    // TODO: This is really slow because most pairs will likely be copied
    // repeatedly.  There should be a better way.
    for (index = median_index; index < node->npairs; index++) {
        delete_pair(node, node->pair_offsets[index]);
    }

    node->npairs = median_index;

    // TODO: this could be less coarse (if we made leaf_node_handler::init less coarse).
    initialize_times(&node->times, node->times.last_modified);

    // Equality takes the left branch, so the median should be from this node.
    btree_key *median_key = &get_pair(node, node->pair_offsets[median_index-1])->key;
    keycpy(median, median_key);

}

void leaf_node_handler::merge(block_size_t block_size, leaf_node_t *node, leaf_node_t *rnode, btree_key *key_to_remove) {
#ifdef BTREE_DEBUG
    printf("merging\n");
    printf("node:\n");
    leaf_node_handler::print(node);
    printf("rnode:\n");
    leaf_node_handler::print(rnode);
#endif
    check("leaf nodes too full to merge",
          sizeof(leaf_node_t) + (node->npairs + rnode->npairs)*sizeof(*node->pair_offsets) +
          (block_size.value() - node->frontmost_offset) + (block_size.value() - rnode->frontmost_offset) >= block_size.value());

    // TODO: this is coarser than it could be.
    initialize_times(&node->times, later_time(node->times.last_modified, rnode->times.last_modified));

    memmove(rnode->pair_offsets + node->npairs, rnode->pair_offsets, rnode->npairs * sizeof(*rnode->pair_offsets));

    for (int i = 0; i < node->npairs; i++) {
        rnode->pair_offsets[i] = insert_pair(rnode, get_pair(node, node->pair_offsets[i]));
    }
    rnode->npairs += node->npairs;

    keycpy(key_to_remove, &get_pair(rnode, rnode->pair_offsets[0])->key);
#ifdef BTREE_DEBUG
    printf("\t|\n\t|\n\t|\n\tV\n");
    printf("node:\n");
    leaf_node_handler::print(node);
    printf("rnode:\n");
    leaf_node_handler::print(rnode);
#endif
    validate(block_size, node);
}

bool leaf_node_handler::level(block_size_t block_size, leaf_node_t *node, leaf_node_t *sibling, btree_key *key_to_replace, btree_key *replacement_key) {
#ifdef BTREE_DEBUG
    printf("leveling\n");
    printf("node:\n");
    leaf_node_handler::print(node);
    printf("sibling:\n");
    leaf_node_handler::print(sibling);
#endif
    //Note: size does not take into account offsets
#ifndef DEBUG_MAX_LEAF
    uint16_t node_size = block_size.value() - node->frontmost_offset;
    uint16_t sibling_size = block_size.value() - sibling->frontmost_offset;
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

        // TODO: node and sibling's times are tolerable but coarse.
        // They are newer than they could be.
        initialize_times(&node->times, later_time(node->times.last_modified, sibling->times.last_modified));

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

        // TODO: node and sibling's times are tolerable but coarse.
        // They are newer than they could be.
        initialize_times(&node->times, later_time(node->times.last_modified, sibling->times.last_modified));

        keycpy(key_to_replace, &get_pair(sibling, sibling->pair_offsets[0])->key);
        keycpy(replacement_key, &get_pair(sibling, sibling->pair_offsets[sibling->npairs-1])->key);
    }

#ifdef BTREE_DEBUG
    printf("\t|\n\t|\n\t|\n\tV\n");
    printf("node:\n");
    leaf_node_handler::print(node);
    printf("sibling:\n");
    leaf_node_handler::print(sibling);
#endif
    validate(block_size, node);
    validate(block_size, sibling);
    return true;
}

bool leaf_node_handler::is_empty(const leaf_node_t *node) {
    return node->npairs == 0;
}

bool leaf_node_handler::is_full(const leaf_node_t *node, btree_key *key, btree_value *value) {
#ifdef DEBUG_MAX_LEAF
    return node->npairs >= DEBUG_MAX_LEAF;
#endif
    // will the data growing from front to right overlap data growing from back to left if we insert
    // the new key value pair
    // TODO: Account for the possibility that the key is already present, in which case we can
    // reuse that space.
    assert(value);
#ifdef BTREE_DEBUG
    printf("sizeof(leaf_node_t): %ld, (node->npairs + 1): %d, sizeof(*node->pair_offsets):%ld, sizeof(btree_leaf_pair): %ld, key->size: %d, value->mem_size(): %d, node->frontmost_offset: %d\n", sizeof(leaf_node_t), (node->npairs + 1), sizeof(*node->pair_offsets), sizeof(btree_leaf_pair), key->size, value->mem_size(), node->frontmost_offset);
#endif
    return sizeof(leaf_node_t) + (node->npairs + 1)*sizeof(*node->pair_offsets) +
        sizeof(btree_leaf_pair) + key->size + value->mem_size() >=
        node->frontmost_offset;
}

void leaf_node_handler::validate(block_size_t block_size, const leaf_node_t *node) {
#ifndef NDEBUG
    assert((void*)&(node->pair_offsets[node->npairs]) <= (void*)get_pair(node, node->frontmost_offset));
    assert(node->frontmost_offset > 0);
    assert(node->frontmost_offset <= block_size.value());
    for (int i = 0; i < node->npairs; i++) {
        assert(node->pair_offsets[i] < block_size.value());
        assert(node->pair_offsets[i] >= node->frontmost_offset);
    }
#endif
}

bool leaf_node_handler::is_mergable(block_size_t block_size, const leaf_node_t *node, const leaf_node_t *sibling) {
#ifdef DEBUG_MAX_INTERNAL
    return node->npairs + sibling->npairs < DEBUG_MAX_LEAF;
#endif
    return sizeof(leaf_node_t) + 
        (node->npairs + sibling->npairs)*sizeof(*node->pair_offsets) +
        (block_size.value() - node->frontmost_offset) +
        (block_size.value() - sibling->frontmost_offset) +
        LEAF_EPSILON < block_size.value();
}

bool leaf_node_handler::is_underfull(block_size_t block_size, const leaf_node_t *node) {
#ifdef DEBUG_MAX_LEAF
    return node->npairs < (DEBUG_MAX_LEAF + 1) / 2;
#endif
    return (sizeof(leaf_node_t) + 1) / 2 + 
        node->npairs*sizeof(*node->pair_offsets) +
        (block_size.value() - node->frontmost_offset) + 
        /* EPSILON: this guaruntees that a node is not underfull directly following a split */
        // TODO: Right now the epsilon we use to make is_underfull not
        // return true after a split is too large. We should come back
        // here and make this more precise.
        LEAF_EPSILON * 2 < block_size.value() / 2;
}

size_t leaf_node_handler::pair_size(btree_leaf_pair *pair) {
    return sizeof(btree_leaf_pair) + pair->key.size + pair->value()->mem_size();
}

btree_leaf_pair *leaf_node_handler::get_pair(const leaf_node_t *node, uint16_t offset) {
    return (btree_leaf_pair *)( ((byte *)node) + offset);
}

void leaf_node_handler::shift_pairs(leaf_node_t *node, uint16_t offset, long shift) {
    btree_leaf_pair *front_pair = get_pair(node, node->frontmost_offset);

    memmove( ((byte *)front_pair)+shift, front_pair, offset - node->frontmost_offset);
    node->frontmost_offset += shift;
    for (int i = 0; i < node->npairs; i++) {
        if (node->pair_offsets[i] < offset)
            node->pair_offsets[i] += shift;
    }
}

void leaf_node_handler::delete_pair(leaf_node_t *node, uint16_t offset) {
    btree_leaf_pair *pair_to_delete = get_pair(node, offset);
    size_t shift = pair_size(pair_to_delete);
    shift_pairs(node, offset, shift);
}

uint16_t leaf_node_handler::insert_pair(leaf_node_t *node, btree_leaf_pair *pair) {
    node->frontmost_offset -= pair_size(pair);
    btree_leaf_pair *new_pair = get_pair(node, node->frontmost_offset);

    // insert contents
    memcpy(new_pair, pair, pair_size(pair));

    return node->frontmost_offset;
}

uint16_t leaf_node_handler::insert_pair(leaf_node_t *node, btree_value *value, btree_key *key) {
    node->frontmost_offset -= sizeof(btree_leaf_pair) + key->size + value->mem_size();
    btree_leaf_pair *new_pair = get_pair(node, node->frontmost_offset);

    // insert contents
    keycpy(&new_pair->key, key);
    memcpy(new_pair->value(), value, sizeof(*value) + value->mem_size());

    return node->frontmost_offset;
}

int leaf_node_handler::get_offset_index(const leaf_node_t *node, btree_key *key) {
    // lower_bound returns the first place where the key could be inserted without violating the ordering
    return std::lower_bound(node->pair_offsets, node->pair_offsets+node->npairs, NULL, leaf_key_comp(node, key)) - node->pair_offsets;
}

// find_key returns the index of the offset for key if it's in the node or -1 if it is not
int leaf_node_handler::find_key(const leaf_node_t *node, btree_key *key) {
    int index = get_offset_index(node, key);
    if (index < node->npairs && is_equal(key, &get_pair(node, node->pair_offsets[index])->key) ) {
        return index;
    } else {
        return -1;
    }
}
void leaf_node_handler::delete_offset(leaf_node_t *node, int index) {
    uint16_t *pair_offsets = node->pair_offsets;
    if (node->npairs > 1)
        memmove(pair_offsets+index, pair_offsets+index+1, (node->npairs-index-1) * sizeof(uint16_t));
    node->npairs--;
}

void leaf_node_handler::insert_offset(leaf_node_t *node, uint16_t offset, int index) {
    uint16_t *pair_offsets = node->pair_offsets;
    memmove(pair_offsets+index+1, pair_offsets+index, (node->npairs-index) * sizeof(uint16_t));
    pair_offsets[index] = offset;
    node->npairs++;
}


bool leaf_node_handler::is_equal(btree_key *key1, btree_key *key2) {
    return sized_strcmp(key1->contents, key1->size, key2->contents, key2->size) == 0;
}

void leaf_node_handler::initialize_times(leaf_timestamps_t *times, repli_timestamp current_time) {
    times->last_modified = current_time;
    for (int i = 0; i < NUM_LEAF_NODE_EARLIER_TIMES; ++i) {
        times->earlier[i] = 0;
    }
}

void leaf_node_handler::rotate_time(leaf_timestamps_t *times, repli_timestamp latest_time, int prev_timestamp_offset) {
    int32_t diff = latest_time.time - times->last_modified.time;
    if (diff < 0) {
        logWRN("We seemingly stepped backwards in time, with new timestamp %d earlier than %d", latest_time.time, times->last_modified);
        // Something strange happened, wipe out everything.
        initialize_times(times, latest_time);
    } else {
        int i = NUM_LEAF_NODE_EARLIER_TIMES;
        while (i-- > 1) {
            uint32_t dt = diff + times->earlier[i - (i <= prev_timestamp_offset)];
            times->earlier[i] = (dt > 0xFFFF ? 0xFFFF : dt);
        }
        uint32_t dt = (-1 == prev_timestamp_offset ? times->earlier[0] : diff);
        times->earlier[0] = (dt > 0xFFFF ? 0xFFFF : dt);
        times->last_modified = latest_time;
    }
}

void leaf_node_handler::remove_time(leaf_timestamps_t *times, int offset) {
    uint16_t d = (offset == -1 ? times->earlier[0] : 0);
    uint16_t t = times->earlier[NUM_LEAF_NODE_EARLIER_TIMES - 1];

    int i = NUM_LEAF_NODE_EARLIER_TIMES - 1;
    while (i-- > 0) {
        uint16_t x = times->earlier[i];
        times->earlier[i] = (offset <= i ? t : x) - d;
        t = x;
    }

    if (offset == -1) {
        times->last_modified.time -= d;
    }
}

// Returns the offset of the timestamp (or -1 or
// NUM_LEAF_NODE_EARLIER_TIMES) for the key-value pair at the
// given offset.
int leaf_node_handler::get_timestamp_offset(block_size_t block_size, leaf_node_t *node, uint16_t offset) {
    byte *target = (byte *)get_pair(node, offset);

    byte *p = ((byte *)node) + node->frontmost_offset;
    byte *e = ((byte *)node) + block_size.value();

    int i = -1;
    for (;;) {
        assert(p <= e);
        if (p >= e) {
            return NUM_LEAF_NODE_EARLIER_TIMES;
        }
        if (p == target) {
            return i;
        }
        ++i;
        if (i == NUM_LEAF_NODE_EARLIER_TIMES) {
            return NUM_LEAF_NODE_EARLIER_TIMES;
        }
        p += pair_size((btree_leaf_pair *)p);
    }
}

int leaf_node_handler::nodecmp(const leaf_node_t *node1, const leaf_node_t *node2) {
    btree_key *key1 = &get_pair(node1, node1->pair_offsets[0])->key;
    btree_key *key2 = &get_pair(node2, node2->pair_offsets[0])->key;

    return sized_strcmp(key1->contents, key1->size, key2->contents, key2->size);
}

void leaf_node_handler::print(const leaf_node_t *node) {
    int freespace = node->frontmost_offset - (sizeof(leaf_node_t) + node->npairs*sizeof(*node->pair_offsets));
    printf("Free space in node: %d\n", freespace);
    printf("\n\n\n");
    for (int i = 0; i < node->npairs; i++) {
        btree_leaf_pair *pair = get_pair(node, node->pair_offsets[i]);
        printf("|\t");
        pair->key.print();
    }
    printf("|\n");
    printf("\n\n\n");
    for (int i = 0; i < node->npairs; i++) {
        btree_leaf_pair *pair = get_pair(node, node->pair_offsets[i]);
        printf("|\t");
        pair->value()->print();
    }
        printf("|\n");
    printf("\n\n\n");
}
