
#include "btree/leaf_node.hpp"

#include <algorithm>
#include <gtest/gtest.h>

#include "logger.hpp"

// #define DEBUG_MAX_LEAF 10

void leaf_node_handler::init(block_size_t block_size, leaf_node_t *node, repli_timestamp modification_time) {
    node->magic = leaf_node_t::expected_magic;
    node->npairs = 0;
    node->frontmost_offset = block_size.value();
    initialize_times(&node->times, modification_time);
}

// Feel free to move this to a separate file.
TEST(LeafNodeTest, InitValidates) {
    int blocksize = 1028;  // a weird value
    block_size_t bs = block_size_t::unsafe_make(blocksize);
    leaf_node_t *p = reinterpret_cast<leaf_node_t *>(calloc(blocksize, 1));

    //    leaf_node_handler::init(bs, p, current_time());
    leaf_node_handler::validate(bs, p);
    // ASSERT_EQ(0, 1);

    // TODO: uh, RAII?
    free(p);
}

// TODO: We end up making modification time data more conservative and
// more coarse than conceivably possible.  We could also let the
// caller supply an earlier[] array.
// TODO: maybe lnode should just supply the modification time.
void leaf_node_handler::init(block_size_t block_size, leaf_node_t *node, const leaf_node_t *lnode, const uint16_t *offsets, int numpairs, repli_timestamp modification_time) {
    init(block_size, node, modification_time);
    for (int i = 0; i < numpairs; i++) {
        node->pair_offsets[i] = insert_pair(node, get_pair(lnode, offsets[i]));
    }
    node->npairs = numpairs;
    // TODO: Why is this sorting step necessary?  Is [offsets, offset + numpairs) not sorted?
    std::sort(node->pair_offsets, node->pair_offsets + numpairs, leaf_key_comp(node));
}

bool leaf_node_handler::insert(block_size_t block_size, leaf_node_t *node, const btree_key *key, const btree_value* value, repli_timestamp insertion_time) {
    if (is_full(node, key, value)) {
        return false;
    }

    int index = get_offset_index(node, key);

    uint16_t prev_offset;
    const btree_leaf_pair *previous = NULL;

    if (index != node->npairs) {
        prev_offset = node->pair_offsets[index];
        previous = get_pair(node, prev_offset);
    }

    if (previous != NULL && is_equal(&previous->key, key)) {
        // 0 <= index < node->npairs

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

        // 0 <= index <= node->npairs

        uint16_t offset = insert_pair(node, value, key);
        insert_offset(node, offset, index);
    }
    validate(block_size, node);
    return true;
}

// TODO: This assumes that key is in the node.  This means we're
// already sure the key is in the node.  This means we're doing an
// unnecessary binary search.
void leaf_node_handler::remove(block_size_t block_size, leaf_node_t *node, const btree_key *key) {
#ifdef BTREE_DEBUG
    printf("removing key: ");
    key->print();
    printf("\n");
    leaf_node_handler::print(node);
#endif

    int index = find_key(node, key);
    assert(index != -1);
    assert(index != node->npairs);

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
    // guarantee(node->npairs != 0, "leaf became zero size!");
}

bool leaf_node_handler::lookup(const leaf_node_t *node, const btree_key *key, btree_value *value) {
    int index = find_key(node, key);
    if (index != -1) {
        uint16_t offset = node->pair_offsets[index];
        const btree_leaf_pair *pair = get_pair(node, offset);
        const btree_value *stored_value = pair->value();
        memcpy(value, stored_value, stored_value->full_size());
        return true;
    } else {
        return false;
    }
}

// We may only split when block_size - node->frontmost_offset is
// greater than twice the max key-value-pair size.  Greater than 1000
// plus some extra.  Let's say greater than 1500, just to be
// comfortable.  TODO: prove that block_size - node->frontmost_offset
// meets this 1500 lower bound.
void leaf_node_handler::split(block_size_t block_size, leaf_node_t *node, leaf_node_t *rnode, btree_key *median_out) {
    assert(node != rnode);

    uint16_t total_pairs = block_size.value() - node->frontmost_offset;
    uint16_t first_pairs = 0;
    int index = 0;
    while (first_pairs < total_pairs/2) { // finds the median index
        first_pairs += pair_size(get_pair(node, node->pair_offsets[index]));
        index++;
    }

    // Obviously 0 <= median_index <= node->npairs.

    // Given that total_pairs > 1500, and keys and value sizes are not
    // much more than 250, we know that 0 < median_index < node->npairs.
    int median_index = index;

    assert(median_index < node->npairs);
    init(block_size, rnode, node, node->pair_offsets + median_index, node->npairs - median_index, node->times.last_modified);

    // This is ~(n^2); it could be ~(n).  Profiling tells us there are
    // bigger problems.
    for (index = median_index; index < node->npairs; index++) {
        delete_pair(node, node->pair_offsets[index]);
    }

    node->npairs = median_index;

    // TODO: this could be less coarse (if we made leaf_node_handler::init less coarse).
    initialize_times(&node->times, node->times.last_modified);

    // Equality takes the left branch, so the median should be from this node.
    assert(median_index > 0);
    const btree_key *median_key = &get_pair(node, node->pair_offsets[median_index-1])->key;
    keycpy(median_out, median_key);
}

void leaf_node_handler::merge(block_size_t block_size, const leaf_node_t *node, leaf_node_t *rnode, btree_key *key_to_remove_out) {
    assert(node != rnode);

#ifdef BTREE_DEBUG
    printf("merging\n");
    printf("node:\n");
    leaf_node_handler::print(node);
    printf("rnode:\n");
    leaf_node_handler::print(rnode);
#endif

    guarantee(sizeof(leaf_node_t) + (node->npairs + rnode->npairs)*sizeof(*node->pair_offsets) +
              (block_size.value() - node->frontmost_offset) + (block_size.value() - rnode->frontmost_offset) <= block_size.value(),
              "leaf nodes too full to merge");

    // TODO: this is coarser than it could be.
    initialize_times(&rnode->times, repli_max(node->times.last_modified, rnode->times.last_modified));

    memmove(rnode->pair_offsets + node->npairs, rnode->pair_offsets, rnode->npairs * sizeof(*rnode->pair_offsets));

    for (int i = 0; i < node->npairs; i++) {
        rnode->pair_offsets[i] = insert_pair(rnode, get_pair(node, node->pair_offsets[i]));
    }
    rnode->npairs += node->npairs;

    keycpy(key_to_remove_out, &get_pair(rnode, rnode->pair_offsets[0])->key);

#ifdef BTREE_DEBUG
    printf("\t|\n\t|\n\t|\n\tV\n");
    printf("node:\n");
    leaf_node_handler::print(node);
    printf("rnode:\n");
    leaf_node_handler::print(rnode);
#endif

#ifndef NDEBUG
    validate(block_size, rnode);
#endif
}

bool leaf_node_handler::level(block_size_t block_size, leaf_node_t *node, leaf_node_t *sibling, btree_key *key_to_replace_out, btree_key *replacement_key_out) {
    assert(node != sibling);

#ifdef BTREE_DEBUG
    printf("leveling\n");
    printf("node:\n");
    leaf_node_handler::print(node);
    printf("sibling:\n");
    leaf_node_handler::print(sibling);
#endif

#ifndef DEBUG_MAX_LEAF
    //Note: size does not take into account offsets
    int node_size = block_size.value() - node->frontmost_offset;
    int sibling_size = block_size.value() - sibling->frontmost_offset;

    if (sibling_size < node_size + 2) {
        logWRN("leaf_node_handler::level called with bad node_size %d and sibling_size %d on block id %u\n", node_size, sibling_size, reinterpret_cast<buf_data_t *>(reinterpret_cast<byte *>(node) - sizeof(buf_data_t))->block_id);
        return false;
    }

    // optimal_adjustment > 0.
    int optimal_adjustment = (sibling_size - node_size) / 2;
#else
    if (sibling->npairs < node->npairs) {
        return false;
    }
#endif

    if (nodecmp(node, sibling) < 0) {
#ifndef DEBUG_MAX_LEAF
        int index;
        {
            // Assumes optimal_adjustment > 0.
            int adjustment = optimal_adjustment;
            index = -1;
            while (adjustment > 0) {
                adjustment -= pair_size(get_pair(sibling, sibling->pair_offsets[++index]));
            }
        }
#else
        int index = (sibling->npairs - node->npairs) / 2;
#endif

        // Since optimal_adjustment > 0, we know that index >= 0.

        if (index <= 0) {
            return false;
        }

        // TODO: Add some elementary checks that absolutely prevent us
        // from inserting too far onto node.  Right now our
        // correctness relies on the arithmetic, and not on the logic,
        // so to speak.  This is very low priority.

        // Proof that we aren't removing everything from sibling: We
        // are undershooting optimal_adjustment.

        // Copy from the beginning of the sibling to end of this node.
        {
            int node_npairs = node->npairs;
            node->npairs += index;
            for (int i = 0; i < index; i++) {
                node->pair_offsets[node_npairs + i] = insert_pair(node, get_pair(sibling, sibling->pair_offsets[i]));
            }
        }

        // This is ~(n^2) where it could be ~(n).  Profile.
        for (int i = 0; i < index; i++) {
            delete_pair(sibling, sibling->pair_offsets[0]);
            delete_offset(sibling, 0);
        }

        // TODO: node and sibling's times are tolerable but coarse.
        // They are newer than they could be.
        initialize_times(&node->times, repli_max(node->times.last_modified, sibling->times.last_modified));

        // Copying node->pair_offsets[0]'s key to key_to_replace_out
        // produces the same effect, later on, as copying
        // node->pair_offsets[node->npairs - index - 1]'s key, or any
        // key in between.  (The latter is the actual key stored in the parent.)
        keycpy(key_to_replace_out, &get_pair(node, node->pair_offsets[0])->key);
        keycpy(replacement_key_out, &get_pair(node, node->pair_offsets[node->npairs-1])->key);

    } else {

#ifndef DEBUG_MAX_LEAF
        // The first index in the sibling to copy
        int index;
        {
            int adjustment = optimal_adjustment;
            index = sibling->npairs;
            while (adjustment > 0) {
                adjustment -= pair_size(get_pair(sibling, sibling->pair_offsets[--index]));
            }
        }
#else
        int index = sibling->npairs - (sibling->npairs - node->npairs) / 2;
#endif

        // If optimal_adjustment > 0, we know index < sibling->npairs.

        int pairs_to_move = sibling->npairs - index;

        if (pairs_to_move == 0) {
            return false;
        }

        // TODO: Add some elemantary checks that absolutely prevent us
        // from removing everything from sibling.  We're overshooting
        // optimal_adjustment in this case, so we need to be careful.
        // It's sufficient to show that sibling_size > 1300, and right
        // now we can prove that.  However, our correctness relies on
        // the arithmetic, and not on the logic, so to speak.  Also
        // make sure that we aren't overfilling node.  This is very low priority.

        // Copy from the end of the sibling to the beginning of the node.
        memmove(node->pair_offsets + pairs_to_move, node->pair_offsets, node->npairs * sizeof(*node->pair_offsets));
        node->npairs += pairs_to_move;
        for (int i = index; i < sibling->npairs; i++) {
            node->pair_offsets[i-index] = insert_pair(node, get_pair(sibling, sibling->pair_offsets[i]));
        }

        // This is ~(n^2) when it could be ~(n).  Profile.
        while (index < sibling->npairs) {
            delete_pair(sibling, sibling->pair_offsets[index]);

            // Decrements sibling->npairs
            delete_offset(sibling, index);
        }

        // TODO: node and sibling's times are tolerable but coarse.
        // They are newer than they could be.
        initialize_times(&node->times, repli_max(node->times.last_modified, sibling->times.last_modified));

        keycpy(key_to_replace_out, &get_pair(sibling, sibling->pair_offsets[0])->key);
        keycpy(replacement_key_out, &get_pair(sibling, sibling->pair_offsets[sibling->npairs-1])->key);
    }

#ifdef BTREE_DEBUG
    printf("\t|\n\t|\n\t|\n\tV\n");
    printf("node:\n");
    leaf_node_handler::print(node);
    printf("sibling:\n");
    leaf_node_handler::print(sibling);
#endif

#ifndef NDEBUG
    validate(block_size, node);
    validate(block_size, sibling);
#endif

    return true;
}

bool leaf_node_handler::is_empty(const leaf_node_t *node) {
    return node->npairs == 0;
}

bool leaf_node_handler::is_full(const leaf_node_t *node, const btree_key *key, const btree_value *value) {
#ifdef DEBUG_MAX_LEAF
    return node->npairs >= DEBUG_MAX_LEAF;
#endif
    // Can the key/value pair fit?  We assume (conservatively) the
    // key/value pair is not already part of the node.

    assert(value);
#ifdef BTREE_DEBUG
    printf("sizeof(leaf_node_t): %ld, (node->npairs + 1): %d, sizeof(*node->pair_offsets):%ld, key->size: %d, value->mem_size(): %d, value->full_size(): %d, node->frontmost_offset: %d\n", sizeof(leaf_node_t), (node->npairs + 1), sizeof(*node->pair_offsets), key->size, value->mem_size(), value->full_size(), node->frontmost_offset);
#endif
    return sizeof(leaf_node_t) + (node->npairs + 1)*sizeof(*node->pair_offsets) +
        key->full_size() + value->full_size() >=
        node->frontmost_offset;
}

void leaf_node_handler::validate(block_size_t block_size, const leaf_node_t *node) {
#ifndef NDEBUG
    assert(ptr_cast<byte>(&(node->pair_offsets[node->npairs])) <= ptr_cast<byte>(get_pair(node, node->frontmost_offset)));
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

size_t leaf_node_handler::pair_size(const btree_leaf_pair *pair) {
    return pair->key.full_size() + pair->value()->full_size();
}

const btree_leaf_pair *leaf_node_handler::get_pair(const leaf_node_t *node, uint16_t offset) {
    return ptr_cast<btree_leaf_pair>(ptr_cast<byte>(node) + offset);
}

btree_leaf_pair *leaf_node_handler::get_pair(leaf_node_t *node, uint16_t offset) {
    return ptr_cast<btree_leaf_pair>(ptr_cast<byte>(node) + offset);
}

void leaf_node_handler::shift_pairs(leaf_node_t *node, uint16_t offset, long shift) {
    byte *front = ptr_cast<byte>(get_pair(node, node->frontmost_offset));

    memmove(front + shift, front, offset - node->frontmost_offset);
    node->frontmost_offset += shift;
    for (int i = 0; i < node->npairs; i++) {
        if (node->pair_offsets[i] < offset)
            node->pair_offsets[i] += shift;
    }
}

void leaf_node_handler::delete_pair(leaf_node_t *node, uint16_t offset) {
    const btree_leaf_pair *pair_to_delete = get_pair(node, offset);
    size_t shift = pair_size(pair_to_delete);
    shift_pairs(node, offset, shift);
}

// Copies pair contents snugly onto the front of the data region,
// modifying the frontmost_offset.  Returns the new frontmost_offset.
uint16_t leaf_node_handler::insert_pair(leaf_node_t *node, const btree_leaf_pair *pair) {
    node->frontmost_offset -= pair_size(pair);
    // insert contents
    memcpy(get_pair(node, node->frontmost_offset), pair, pair_size(pair));

    return node->frontmost_offset;
}

// Decreases frontmost_offset by the pair size, key->full_size() + value->full_size().
// Copies key and value into pair snugly onto the front of the data region.
// Returns the new frontmost_offset.
uint16_t leaf_node_handler::insert_pair(leaf_node_t *node, const btree_value *value, const btree_key *key) {
    node->frontmost_offset -= key->full_size() + value->full_size();
    btree_leaf_pair *new_pair = get_pair(node, node->frontmost_offset);

    // insert contents
    keycpy(&new_pair->key, key);
    // "new_pair->value()" below depends on the side effect of the keycpy line above.
    memcpy(new_pair->value(), value, value->full_size());

    return node->frontmost_offset;
}

// Gets the index at which we'd want to insert the key without
// violating ordering.  Or returns the index at which the key already
// exists.  Returns a value in [0, node->npairs].
int leaf_node_handler::get_offset_index(const leaf_node_t *node, const btree_key *key) {
    // lower_bound returns the first place where the key could be inserted without violating the ordering
    return std::lower_bound(node->pair_offsets, node->pair_offsets+node->npairs, (uint16_t)leaf_key_comp::faux_offset, leaf_key_comp(node, key)) - node->pair_offsets;
}

// find_key returns the index of the offset for key if it's in the node or -1 if it is not
int leaf_node_handler::find_key(const leaf_node_t *node, const btree_key *key) {
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

// TODO: Calls to is_equal are redundant calls to sized_strcmp.  At
// least they're cache-friendly.
bool leaf_node_handler::is_equal(const btree_key *key1, const btree_key *key2) {
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

repli_timestamp leaf_node_handler::get_timestamp_value(block_size_t block_size, const leaf_node_t *node, uint16_t offset) {
    int toff = get_timestamp_offset(block_size, node, offset);

    if (toff == -1) {
        return node->times.last_modified;
    } else {
        return repli_time(node->times.last_modified.time - node->times.earlier[std::min(toff, NUM_LEAF_NODE_EARLIER_TIMES - 1)]);
    }
}

// Returns the offset of the timestamp (or -1 or
// NUM_LEAF_NODE_EARLIER_TIMES) for the key-value pair at the
// given offset.
int leaf_node_handler::get_timestamp_offset(block_size_t block_size, const leaf_node_t *node, uint16_t offset) {
    const byte *target = ptr_cast<byte>(get_pair(node, offset));

    const byte *p = ptr_cast<byte>(get_pair(node, node->frontmost_offset));
    const byte *e = ptr_cast<byte>(node) + block_size.value();

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
        p += pair_size(ptr_cast<btree_leaf_pair>(p));
    }
}

// Assumes node1 and node2 are not empty.
int leaf_node_handler::nodecmp(const leaf_node_t *node1, const leaf_node_t *node2) {
    const btree_key *key1 = &get_pair(node1, node1->pair_offsets[0])->key;
    const btree_key *key2 = &get_pair(node2, node2->pair_offsets[0])->key;

    return sized_strcmp(key1->contents, key1->size, key2->contents, key2->size);
}

void leaf_node_handler::print(const leaf_node_t *node) {
    int freespace = node->frontmost_offset - (sizeof(leaf_node_t) + node->npairs*sizeof(*node->pair_offsets));
    printf("Free space in node: %d\n", freespace);
    printf("\n\n\n");
    for (int i = 0; i < node->npairs; i++) {
        const btree_leaf_pair *pair = get_pair(node, node->pair_offsets[i]);
        printf("|\t");
        pair->key.print();
    }
    printf("|\n");
    printf("\n\n\n");
    for (int i = 0; i < node->npairs; i++) {
        const btree_leaf_pair *pair = get_pair(node, node->pair_offsets[i]);
        printf("|\t");
        pair->value()->print();
    }
        printf("|\n");
    printf("\n\n\n");
}
