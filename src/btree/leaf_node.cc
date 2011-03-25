
#include "btree/leaf_node.hpp"
#include <algorithm>
#include "logger.hpp"
#include "btree/buf_patches.hpp"

// #define DEBUG_MAX_LEAF 10

bool leaf_pair_fits(const btree_leaf_pair *pair, size_t size) {
    return 3 < size && 1 + size_t(pair->key.size) + 2 <= size && size_t(1 + pair->key.size + pair->value()->full_size()) <= size;
}

namespace leaf {

void init(block_size_t block_size, buf_t& node_buf, repli_timestamp modification_time) {
    leaf_node_t *node = ptr_cast<leaf_node_t>(node_buf.get_data_major_write());

    node->magic = leaf_node_t::expected_magic;
    node->npairs = 0;
    node->frontmost_offset = block_size.value();
    impl::initialize_times(node_buf, modification_time);
}

// TODO: We end up making modification time data more conservative and
// more coarse than conceivably possible.  We could also let the
// caller supply an earlier[] array.
// TODO: maybe lnode should just supply the modification time.
void init(block_size_t block_size, buf_t& node_buf, const leaf_node_t *lnode, const uint16_t *offsets, int numpairs, repli_timestamp modification_time) {
    leaf_node_t *node = ptr_cast<leaf_node_t>(node_buf.get_data_major_write());
    
    init(block_size, node_buf, modification_time);
    for (int i = 0; i < numpairs; i++) {
        node->pair_offsets[i] = impl::insert_pair(node_buf, get_pair(lnode, offsets[i]));
    }
    node->npairs = numpairs;
    // TODO: Why is this sorting step necessary?  Is [offsets, offset + numpairs) not sorted?
    std::sort(node->pair_offsets, node->pair_offsets + numpairs, leaf_key_comp(node));
}


bool insert(block_size_t block_size, buf_t& node_buf, const btree_key_t *key, const btree_value* value, repli_timestamp insertion_time) {
    const leaf_node_t *node = ptr_cast<leaf_node_t>(node_buf.get_data_read());

    if (is_full(node, key, value)) {
        return false;
    }

    node_buf.apply_patch(new leaf_insert_patch_t(node_buf.get_block_id(), node_buf.get_next_patch_counter(), block_size, value->size, value->metadata_flags.flags, value->contents, key->size, key->contents, insertion_time));

    validate(block_size, node);
    return true;
}

void insert(UNUSED block_size_t block_size, leaf_node_t *node, const btree_key_t *key, const btree_value* value, repli_timestamp insertion_time) {
    int index = impl::get_offset_index(node, key);

    uint16_t prev_offset;
    const btree_leaf_pair *previous = NULL;

    if (index != node->npairs) {
        prev_offset = node->pair_offsets[index];
        previous = get_pair(node, prev_offset);
    }

    if (previous != NULL && impl::is_equal(&previous->key, key)) {
        // 0 <= index < node->npairs

        // A duplicate key is being inserted.  Let's shift the old
        // key/value pair away _completely_ and put the new one at the
        // beginning.

        int prev_timestamp_offset = impl::get_timestamp_offset(node, prev_offset);

        impl::rotate_time(&node->times, insertion_time, prev_timestamp_offset);

        impl::delete_pair(node, prev_offset);
        node->pair_offsets[index] = impl::insert_pair(node, value, key);
    } else {
        // manipulate timestamps.
        impl::rotate_time(&node->times, insertion_time, NUM_LEAF_NODE_EARLIER_TIMES);

        // 0 <= index <= node->npairs

        uint16_t offset = impl::insert_pair(node, value, key);
        impl::insert_offset(node, offset, index);
    }
}

// TODO: This assumes that key is in the node.  This means we're
// already sure the key is in the node.  This means we're doing an
// unnecessary binary search.
void remove(block_size_t block_size, buf_t &node_buf, const btree_key_t *key) {
    const leaf_node_t *node = ptr_cast<leaf_node_t>(node_buf.get_data_read());
#ifdef BTREE_DEBUG
    printf("removing key: ");
    key->print();
    printf("\n");
    print(node);
#endif

    node_buf.apply_patch(new leaf_remove_patch_t(node_buf.get_block_id(), node_buf.get_next_patch_counter(), block_size, key->size, key->contents));

#ifdef BTREE_DEBUG
    printf("\t|\n\t|\n\t|\n\tV\n");
    leaf::print(node);
#endif

    // TODO: Currently this will error incorrectly on root
    // guarantee(node->npairs != 0, "leaf became zero size!");

    validate(block_size, node);
}

// TODO: We don't use the block_size parameter, so get rid of it.
void remove(UNUSED block_size_t block_size, leaf_node_t *node, const btree_key_t *key) {
    int index = impl::find_key(node, key);
    rassert(index != impl::key_not_found);
    rassert(index != node->npairs);

    uint16_t offset = node->pair_offsets[index];
    impl::remove_time(&node->times, impl::get_timestamp_offset(node, offset));

    impl::delete_pair(node, offset);
    impl::delete_offset(node, index);
}

bool lookup(const leaf_node_t *node, const btree_key_t *key, btree_value *value) {
    int index = impl::find_key(node, key);
    if (index != impl::key_not_found) {
        const btree_leaf_pair *pair = get_pair_by_index(node, index);
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
void split(block_size_t block_size, buf_t &node_buf, buf_t &rnode_buf, btree_key_t *median_out) {
    const leaf_node_t *node = ptr_cast<leaf_node_t>(node_buf.get_data_read());
    const leaf_node_t *rnode __attribute__((unused)) = ptr_cast<leaf_node_t>(rnode_buf.get_data_read());

    rassert(node != rnode);

    uint16_t total_pairs = block_size.value() - node->frontmost_offset;
    uint16_t first_pairs = 0;
    int index = 0;
    while (first_pairs < total_pairs/2) { // finds the median index
        first_pairs += pair_size(get_pair_by_index(node, index));
        index++;
    }

    // Obviously 0 <= median_index <= node->npairs.

    // Given that total_pairs > 1500, and keys and value sizes are not
    // much more than 250, we know that 0 < median_index < node->npairs.
    int median_index = index;

    rassert(median_index < node->npairs);
    init(block_size, rnode_buf, node, node->pair_offsets + median_index, node->npairs - median_index, node->times.last_modified);

    // This is ~(n^2); it could be ~(n).  Profiling tells us there are
    // bigger problems.
    for (index = median_index; index < node->npairs; index++) {
        impl::delete_pair(node_buf, node->pair_offsets[index]);
    }

    uint16_t new_npairs = median_index;
    node_buf.set_data(const_cast<uint16_t *>(&node->npairs), &new_npairs, sizeof(new_npairs));

    // TODO: this could be less coarse (if we made leaf::init less coarse).
    impl::initialize_times(node_buf, node->times.last_modified);

    // Equality takes the left branch, so the median should be from this node.
    rassert(median_index > 0);
    const btree_key_t *median_key = &get_pair_by_index(node, median_index-1)->key;
    keycpy(median_out, median_key);
}

void merge(block_size_t block_size, const leaf_node_t *node, buf_t &rnode_buf, btree_key_t *key_to_remove_out) {
    const leaf_node_t *rnode = ptr_cast<leaf_node_t>(rnode_buf.get_data_read());

    rassert(node != rnode);

#ifdef BTREE_DEBUG
    printf("merging\n");
    printf("node:\n");
    leaf::print(node);
    printf("rnode:\n");
    leaf::print(rnode);
#endif

    guarantee(sizeof(leaf_node_t) + (node->npairs + rnode->npairs)*sizeof(*node->pair_offsets) +
              (block_size.value() - node->frontmost_offset) + (block_size.value() - rnode->frontmost_offset) <= block_size.value(),
              "leaf nodes too full to merge");

    // TODO: this is coarser than it could be.
    impl::initialize_times(rnode_buf, repli_max(node->times.last_modified, rnode->times.last_modified));

    rnode_buf.move_data(const_cast<uint16_t *>(rnode->pair_offsets + node->npairs), rnode->pair_offsets, rnode->npairs * sizeof(*rnode->pair_offsets));

    for (int i = 0; i < node->npairs; i++) {
        uint16_t new_offset = impl::insert_pair(rnode_buf, get_pair_by_index(node, i));
        rnode_buf.set_data(const_cast<uint16_t *>(&rnode->pair_offsets[i]), &new_offset, sizeof(new_offset));
    }
    uint16_t new_npairs = rnode->npairs + node->npairs;
    rnode_buf.set_data(const_cast<uint16_t *>(&rnode->npairs), &new_npairs, sizeof(new_npairs));

    keycpy(key_to_remove_out, &get_pair_by_index(rnode, 0)->key);

#ifdef BTREE_DEBUG
    printf("\t|\n\t|\n\t|\n\tV\n");
    printf("node:\n");
    leaf::print(node);
    printf("rnode:\n");
    leaf::print(rnode);
#endif

#ifndef NDEBUG
    validate(block_size, rnode);
#endif
}

bool level(block_size_t block_size, buf_t &node_buf, buf_t &sibling_buf, btree_key_t *key_to_replace_out, btree_key_t *replacement_key_out) {
    const leaf_node_t *node = ptr_cast<leaf_node_t>(node_buf.get_data_read());
    const leaf_node_t *sibling = ptr_cast<leaf_node_t>(sibling_buf.get_data_read());

    rassert(node != sibling);

#ifdef BTREE_DEBUG
    printf("leveling\n");
    printf("node:\n");
    leaf::print(node);
    printf("sibling:\n");
    leaf::print(sibling);
#endif

#ifndef DEBUG_MAX_LEAF
    //Note: size does not take into account offsets
    int node_size = block_size.value() - node->frontmost_offset;
    int sibling_size = block_size.value() - sibling->frontmost_offset;

    if (sibling_size < node_size + 2) {
        logWRN("leaf::level called with bad node_size %d and sibling_size %d on block id %u\n", node_size, sibling_size, reinterpret_cast<const buf_data_t *>(reinterpret_cast<const byte *>(node) - sizeof(buf_data_t))->block_id.value);
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
                adjustment -= pair_size(get_pair_by_index(sibling, ++index));
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
            uint16_t new_npairs = node->npairs + index;
            node_buf.set_data(const_cast<uint16_t *>(&node->npairs), &new_npairs, sizeof(new_npairs));
            for (int i = 0; i < index; i++) {
                uint16_t new_offset = impl::insert_pair(node_buf, get_pair_by_index(sibling, i));
                node_buf.set_data(const_cast<uint16_t *>(&node->pair_offsets[node_npairs + i]), &new_offset, sizeof(new_offset));
            }
        }

        // This is ~(n^2) where it could be ~(n).  Profile.
        for (int i = 0; i < index; i++) {
            impl::delete_pair(sibling_buf, sibling->pair_offsets[0]);
            impl::delete_offset(sibling_buf, 0);
        }

        // TODO: node and sibling's times are tolerable but coarse.
        // They are newer than they could be.
        impl::initialize_times(node_buf, repli_max(node->times.last_modified, sibling->times.last_modified));

        // Copying node->pair_offsets[0]'s key to key_to_replace_out
        // produces the same effect, later on, as copying
        // node->pair_offsets[node->npairs - index - 1]'s key, or any
        // key in between.  (The latter is the actual key stored in the parent.)
        keycpy(key_to_replace_out, &get_pair_by_index(node, 0)->key);
        keycpy(replacement_key_out, &get_pair_by_index(node, node->npairs-1)->key);

    } else {

#ifndef DEBUG_MAX_LEAF
        // The first index in the sibling to copy
        int index;
        {
            int adjustment = optimal_adjustment;
            index = sibling->npairs;
            while (adjustment > 0) {
                adjustment -= pair_size(get_pair_by_index(sibling, --index));
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
        node_buf.move_data(const_cast<uint16_t *>(node->pair_offsets + pairs_to_move), node->pair_offsets, node->npairs * sizeof(*node->pair_offsets));
        uint16_t new_npairs = node->npairs + pairs_to_move;
        node_buf.set_data(const_cast<uint16_t *>(&node->npairs), &new_npairs, sizeof(new_npairs));
        for (int i = index; i < sibling->npairs; i++) {
            uint16_t new_offset = impl::insert_pair(node_buf, get_pair_by_index(sibling, i));
            node_buf.set_data(const_cast<uint16_t *>(&node->pair_offsets[i-index]), &new_offset, sizeof(new_offset));
        }

        // This is ~(n^2) when it could be ~(n).  Profile.
        while (index < sibling->npairs) {
            impl::delete_pair(sibling_buf, sibling->pair_offsets[index]);

            // Decrements sibling->npairs
            impl::delete_offset(sibling_buf, index);
        }

        // TODO: node and sibling's times are tolerable but coarse.
        // They are newer than they could be.
        impl::initialize_times(node_buf, repli_max(node->times.last_modified, sibling->times.last_modified));

        keycpy(key_to_replace_out, &get_pair_by_index(sibling, 0)->key);
        keycpy(replacement_key_out, &get_pair_by_index(sibling, sibling->npairs-1)->key);
    }

#ifdef BTREE_DEBUG
    printf("\t|\n\t|\n\t|\n\tV\n");
    printf("node:\n");
    leaf::print(node);
    printf("sibling:\n");
    leaf::print(sibling);
#endif

#ifndef NDEBUG
    validate(block_size, node);
    validate(block_size, sibling);
#endif

    return true;
}

bool is_empty(const leaf_node_t *node) {
    return node->npairs == 0;
}

bool is_full(const leaf_node_t *node, const btree_key_t *key, const btree_value *value) {
#ifdef DEBUG_MAX_LEAF
    return node->npairs >= DEBUG_MAX_LEAF;
#endif
    // Can the key/value pair fit?  We assume (conservatively) the
    // key/value pair is not already part of the node.

    rassert(value);
#ifdef BTREE_DEBUG
    printf("sizeof(leaf_node_t): %ld, (node->npairs + 1): %d, sizeof(*node->pair_offsets):%ld, key->size: %d, value->full_size(): %d, node->frontmost_offset: %d\n", sizeof(leaf_node_t), (node->npairs + 1), sizeof(*node->pair_offsets), key->size, value->full_size(), node->frontmost_offset);
#endif
    return sizeof(leaf_node_t) + (node->npairs + 1)*sizeof(*node->pair_offsets) +
        key->full_size() + value->full_size() >=
        node->frontmost_offset;
}

void validate(UNUSED block_size_t block_size, UNUSED const leaf_node_t *node) {
#ifndef NDEBUG
    rassert(ptr_cast<byte>(&(node->pair_offsets[node->npairs])) <= ptr_cast<byte>(get_pair(node, node->frontmost_offset)));
    rassert(node->frontmost_offset > 0);
    rassert(node->frontmost_offset <= block_size.value());
    for (int i = 0; i < node->npairs; i++) {
        rassert(node->pair_offsets[i] < block_size.value());
        rassert(node->pair_offsets[i] >= node->frontmost_offset);
    }
#endif
}

bool is_mergable(block_size_t block_size, const leaf_node_t *node, const leaf_node_t *sibling) {
#ifdef DEBUG_MAX_INTERNAL
    return node->npairs + sibling->npairs < DEBUG_MAX_LEAF;
#endif
    return sizeof(leaf_node_t) +
        (node->npairs + sibling->npairs)*sizeof(*node->pair_offsets) +
        (block_size.value() - node->frontmost_offset) +
        (block_size.value() - sibling->frontmost_offset) +
        LEAF_EPSILON < block_size.value();
}

bool has_sensible_offsets(block_size_t block_size, const leaf_node_t *node) {
    return offsetof(leaf_node_t, pair_offsets) + node->npairs * sizeof(*node->pair_offsets) <= node->frontmost_offset && node->frontmost_offset <= block_size.value();
}

bool is_underfull(block_size_t block_size, const leaf_node_t *node) {
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

size_t pair_size(const btree_leaf_pair *pair) {
    return pair->key.full_size() + pair->value()->full_size();
}

const btree_leaf_pair *get_pair(const leaf_node_t *node, uint16_t offset) {
    return ptr_cast<btree_leaf_pair>(ptr_cast<byte>(node) + offset);
}

btree_leaf_pair *get_pair(leaf_node_t *node, uint16_t offset) {
    return ptr_cast<btree_leaf_pair>(ptr_cast<byte>(node) + offset);
}

const btree_leaf_pair *get_pair_by_index(const leaf_node_t *node, int index) {
    return get_pair(node, node->pair_offsets[index]);
}

btree_leaf_pair *get_pair_by_index(leaf_node_t *node, int index) {
    return get_pair(node, node->pair_offsets[index]);
}

// Assumes node1 and node2 are not empty.
int nodecmp(const leaf_node_t *node1, const leaf_node_t *node2) {
    const btree_key_t *key1 = &get_pair_by_index(node1, 0)->key;
    const btree_key_t *key2 = &get_pair_by_index(node2, 0)->key;

    return sized_strcmp(key1->contents, key1->size, key2->contents, key2->size);
}

void print(const leaf_node_t *node) {
    int freespace = node->frontmost_offset - (sizeof(leaf_node_t) + node->npairs*sizeof(*node->pair_offsets));
    printf("Free space in node: %d\n", freespace);
    printf("\n\n\n");
    for (int i = 0; i < node->npairs; i++) {
        const btree_leaf_pair *pair = get_pair_by_index(node, i);
        printf("|\t");
        pair->key.print();
    }
    printf("|\n");
    printf("\n\n\n");
    for (int i = 0; i < node->npairs; i++) {
        const btree_leaf_pair *pair = get_pair_by_index(node, i);
        printf("|\t");
        pair->value()->print();
    }
        printf("|\n");
    printf("\n\n\n");
}

repli_timestamp get_timestamp_value(const leaf_node_t *node, uint16_t offset) {
    int toff = impl::get_timestamp_offset(node, offset);

    if (toff == -1) {
        return node->times.last_modified;
    } else {
        return repli_time(node->times.last_modified.time - node->times.earlier[std::min(toff, NUM_LEAF_NODE_EARLIER_TIMES - 1)]);
    }
}

namespace impl {

void shift_pairs(leaf_node_t *node, uint16_t offset, long shift) {
    byte *front = ptr_cast<byte>(get_pair(node, node->frontmost_offset));

    memmove(front + shift, front, offset - node->frontmost_offset);
    node->frontmost_offset += shift;
    for (int i = 0; i < node->npairs; i++) {
        if (node->pair_offsets[i] < offset)
            node->pair_offsets[i] += shift;
    }
}

void delete_pair(buf_t &node_buf, uint16_t offset) {
    const leaf_node_t *node = ptr_cast<leaf_node_t>(node_buf.get_data_read());
    const btree_leaf_pair *pair_to_delete = get_pair(node, offset);
    size_t shift = pair_size(pair_to_delete);

    node_buf.apply_patch(new leaf_shift_pairs_patch_t(node_buf.get_block_id(), node_buf.get_next_patch_counter(), offset, shift));
}
void delete_pair(leaf_node_t *node, uint16_t offset) {
    const btree_leaf_pair *pair_to_delete = get_pair(node, offset);
    size_t shift = pair_size(pair_to_delete);

    shift_pairs(node, offset, shift);
}

// Copies pair contents snugly onto the front of the data region,
// modifying the frontmost_offset.  Returns the new frontmost_offset.
uint16_t insert_pair(buf_t &node_buf, const btree_leaf_pair *pair) {
    return insert_pair(node_buf, pair->value(), &pair->key);
}

// Decreases frontmost_offset by the pair size, key->full_size() + value->full_size().
// Copies key and value into pair snugly onto the front of the data region.
// Returns the new frontmost_offset.
uint16_t insert_pair(buf_t &node_buf, const btree_value *value, const btree_key_t *key) {
    node_buf.apply_patch(new leaf_insert_pair_patch_t(node_buf.get_block_id(), node_buf.get_next_patch_counter(), value->size, value->metadata_flags.flags, value->contents, key->size, key->contents));
    const leaf_node_t *node = ptr_cast<leaf_node_t>(node_buf.get_data_read());
    return node->frontmost_offset;
}

uint16_t insert_pair(leaf_node_t *node, const btree_value *value, const btree_key_t *key) {
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
int get_offset_index(const leaf_node_t *node, const btree_key_t *key) {
    // lower_bound returns the first place where the key could be inserted without violating the ordering
    return std::lower_bound(node->pair_offsets, node->pair_offsets+node->npairs, (uint16_t)leaf_key_comp::faux_offset, leaf_key_comp(node, key)) - node->pair_offsets;
}

// find_key returns the index of the offset for key if it's in the node or -1 if it is not
int find_key(const leaf_node_t *node, const btree_key_t *key) {
    int index = get_offset_index(node, key);
    if (index < node->npairs && impl::is_equal(key, &get_pair_by_index(node, index)->key) ) {
        return index;
    } else {
        return impl::key_not_found;
    }
}

void delete_offset(buf_t &node_buf, int index) {
    const leaf_node_t *node = ptr_cast<leaf_node_t>(node_buf.get_data_read());
    const uint16_t *pair_offsets = node->pair_offsets;
    if (node->npairs > 1) {
        node_buf.move_data(const_cast<uint16_t *>(pair_offsets+index), pair_offsets+index+1, (node->npairs-index-1) * sizeof(uint16_t));
    }
    uint16_t npairs = node->npairs - 1;
    node_buf.set_data(const_cast<uint16_t *>(&node->npairs), &npairs, sizeof(npairs));
}
void delete_offset(leaf_node_t *node, int index) {
    uint16_t *pair_offsets = node->pair_offsets;
    if (node->npairs > 1) {
        memmove(pair_offsets+index, pair_offsets+index+1, (node->npairs-index-1) * sizeof(uint16_t));
    }
    node->npairs--;
}

void insert_offset(leaf_node_t *node, uint16_t offset, int index) {
    uint16_t *pair_offsets = node->pair_offsets;
    memmove(pair_offsets+index+1, pair_offsets+index, (node->npairs-index) * sizeof(uint16_t));
    pair_offsets[index] = offset;
    node->npairs++;
}

// TODO: Calls to is_equal are redundant calls to sized_strcmp.  At
// least they're cache-friendly.
bool is_equal(const btree_key_t *key1, const btree_key_t *key2) {
    return sized_strcmp(key1->contents, key1->size, key2->contents, key2->size) == 0;
}

void initialize_times(buf_t &node_buf, repli_timestamp current_time) {
    const leaf_node_t *node = ptr_cast<leaf_node_t>(node_buf.get_data_read());
    leaf_timestamps_t new_times;
    initialize_times(&new_times, current_time);
    node_buf.set_data(const_cast<leaf_timestamps_t *>(&node->times), &new_times, sizeof(leaf_timestamps_t));
}
void initialize_times(leaf_timestamps_t *times, repli_timestamp current_time) {
    times->last_modified = current_time;
    for (int i = 0; i < NUM_LEAF_NODE_EARLIER_TIMES; ++i) {
        times->earlier[i] = 0;
    }
}

void rotate_time(leaf_timestamps_t *times, repli_timestamp latest_time, int prev_timestamp_offset) {
    int32_t diff = latest_time.time - times->last_modified.time;
    if (diff < 0) {
        logWRN("We seemingly stepped backwards in time, with new timestamp %d earlier than %d\n", latest_time.time, times->last_modified.time);
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

void remove_time(leaf_timestamps_t *times, int offset) {
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
int get_timestamp_offset(const leaf_node_t *node, uint16_t offset) {
    const byte *target = ptr_cast<byte>(get_pair(node, offset));

    const byte *p = ptr_cast<byte>(get_pair(node, node->frontmost_offset));
    int npairs = node->npairs;

    int i = 0;
    for (;;) {
        if (i == npairs) {
            return NUM_LEAF_NODE_EARLIER_TIMES;
        }
        if (p == target) {
            return i - 1;
        }
        if (i == NUM_LEAF_NODE_EARLIER_TIMES) {
            return NUM_LEAF_NODE_EARLIER_TIMES;
        }
        ++i;
        p += pair_size(ptr_cast<btree_leaf_pair>(p));
    }
}

}  // namespace leaf::impl

}  // namespace leaf
