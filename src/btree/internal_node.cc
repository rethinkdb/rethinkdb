// Copyright 2010-2013 RethinkDB, all rights reserved.
#include "btree/internal_node.hpp"

#include <string.h>

#include <algorithm>

#include "btree/node.hpp"
#include "containers/unaligned.hpp"

// We comment out this warning, and static_assert that pair_offsets is
// at an aligned offset.
//
// Considering we're doing arbitrary math into the node, we still
// might have problems with unaligned pointers on some platforms.  Of
// course, that would show itself instantly in testing.
#if defined(__GNUC__) && (100 * __GNUC__ + __GNUC_MINOR__ >= 901)
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Waddress-of-packed-member"
#endif

static_assert(offsetof(internal_node_t, pair_offsets) % 2 == 0,
              "pair_offsets must be at uint16_t alignment");

//In this tree, less than or equal takes the left-hand branch and greater than takes the right hand branch

namespace internal_node {

class ibuf_t;

// We can't use "internal" for internal stuff obviously.
namespace impl {
size_t pair_size_with_key(const btree_key_t *key);
size_t pair_size_with_key_size(uint8_t size);

void delete_pair(internal_node_t *node, uint16_t offset);
uint16_t insert_pair(internal_node_t *node, const btree_internal_pair *pair);
uint16_t insert_pair(internal_node_t *node, block_id_t lnode, const btree_key_t *key);
void delete_offset(internal_node_t *node, int index);
void insert_offset(internal_node_t *node, uint16_t offset, int index);
void make_last_pair_special(internal_node_t *node);
bool is_equal(const btree_key_t *key1, const btree_key_t *key2);
}  // namespace impl

void init(block_size_t block_size, internal_node_t *node) {
    node->magic = internal_node_t::expected_magic;
    node->npairs = 0;
    node->frontmost_offset = block_size.value();
}

void init(block_size_t block_size, internal_node_t *node, const internal_node_t *lnode, const uint16_t *offsets, int numpairs) {
    init(block_size, node);
    rassert(get_pair_by_index(lnode, lnode->npairs-1)->key.size == 0);
    for (int i = 0; i < numpairs; i++) {
        node->pair_offsets[i] = impl::insert_pair(node, get_pair(lnode, offsets[i]));
    }
    node->npairs = numpairs;
    std::sort(node->pair_offsets, node->pair_offsets+node->npairs-1, internal_key_comp(node));
    rassert(get_pair_by_index(node, node->npairs-1)->key.size == 0);
}

block_id_t lookup(const internal_node_t *node, const btree_key_t *key) {
    int index = get_offset_index(node, key);
    return get_pair_by_index(node, index)->lnode;
}

bool insert(internal_node_t *node, const btree_key_t *key, block_id_t lnode, block_id_t rnode) {
    rassert(key->size <= MAX_KEY_SIZE, "key too large");
    if (is_full(node)) return false;
    if (node->npairs == 0) {
        btree_key_t special;
        special.size = 0;

        const uint16_t special_offset = impl::insert_pair(node, rnode, &special);
        impl::insert_offset(node, special_offset, 0);
    }

    int index = get_offset_index(node, key);
    rassert(!impl::is_equal(&get_pair_by_index(node, index)->key, key),
        "tried to insert duplicate key into internal node!");
    const uint16_t offset = impl::insert_pair(node, lnode, key);
    impl::insert_offset(node, offset, index);

    get_pair_by_index(node, index + 1)->lnode = rnode;
    return true;
}

bool remove(block_size_t block_size, internal_node_t *node, const btree_key_t *key) {
    int index = get_offset_index(node, key);
    impl::delete_pair(node, node->pair_offsets[index]);
    impl::delete_offset(node, index);

    if (index == node->npairs) {
        impl::make_last_pair_special(node);
    }

    validate(block_size, node);
    return true;
}

void split(block_size_t block_size, internal_node_t *node, internal_node_t *rnode, btree_key_t *median) {
    uint16_t total_pairs = block_size.value() - node->frontmost_offset;
    uint16_t first_pairs = 0;
    int index = 0;
    while (first_pairs < total_pairs/2) { // finds the median index
        first_pairs += pair_size(get_pair_by_index(node, index));
        index++;
    }
    int median_index = index;

    // Equality takes the left branch, so the median should be from this node.
    const btree_key_t *median_key = &get_pair_by_index(node, median_index-1)->key;
    keycpy(median, median_key);

    init(block_size, rnode, node, node->pair_offsets + median_index, node->npairs - median_index);

    // TODO: This is really slow because most pairs will likely be copied
    // repeatedly.  There should be a better way.
    for (index = median_index; index < node->npairs; index++) {
        impl::delete_pair(node, node->pair_offsets[index]);
    }

    const uint16_t new_npairs = median_index;
    node->npairs = new_npairs;
    //make last pair special
    impl::make_last_pair_special(node);

    validate(block_size, node);
    validate(block_size, rnode);
}

void merge(block_size_t block_size, const internal_node_t *node, internal_node_t *rnode, const internal_node_t *parent) {
    validate(block_size, node);
    validate(block_size, rnode);
    // get the key in parent which points to node
    const btree_key_t *key_from_parent = &get_pair_by_index(parent, get_offset_index(parent, &get_pair_by_index(node, 0)->key))->key;

    guarantee(sizeof(internal_node_t) + (node->npairs + rnode->npairs)*sizeof(*node->pair_offsets) +
        (block_size.value() - node->frontmost_offset) + (block_size.value() - rnode->frontmost_offset) + key_from_parent->size < block_size.value(),
        "internal nodes too full to merge");

    memmove(rnode->pair_offsets + node->npairs, rnode->pair_offsets, rnode->npairs * sizeof(*rnode->pair_offsets));

    for (int i = 0; i < node->npairs-1; i++) { // the last pair is special
        const uint16_t new_offset = impl::insert_pair(rnode, get_pair_by_index(node, i));
        rnode->pair_offsets[i] = new_offset;
    }
    const uint16_t new_offset = impl::insert_pair(rnode, get_pair_by_index(node, node->npairs-1)->lnode, key_from_parent);
    rnode->pair_offsets[node->npairs - 1] = new_offset;

    const uint16_t new_npairs = rnode->npairs + node->npairs;
    rnode->npairs = new_npairs;

    validate(block_size, rnode);
}

bool level(block_size_t block_size, internal_node_t *node,
           internal_node_t *sibling, btree_key_t *replacement_key,
           const internal_node_t *parent,
           std::vector<block_id_t> *moved_children_out) {
    validate(block_size, node);
    validate(block_size, sibling);
    if (moved_children_out != nullptr) {
        moved_children_out->reserve(sibling->npairs);
    }

    if (nodecmp(node, sibling) < 0) {
        const btree_key_t *key_from_parent = &get_pair_by_index(parent, get_offset_index(parent, &get_pair_by_index(node, 0)->key))->key;
        if (sizeof(internal_node_t) + (node->npairs + 1) * sizeof(*node->pair_offsets) + impl::pair_size_with_key(key_from_parent) >= node->frontmost_offset)
            return false;
        uint16_t special_pair_offset = node->pair_offsets[node->npairs-1];
        block_id_t last_offset = get_pair(node, special_pair_offset)->lnode;
        uint16_t new_pair_offset = impl::insert_pair(node, last_offset, key_from_parent);
        node->pair_offsets[node->npairs - 1] = new_pair_offset;

        uint16_t new_npairs = node->npairs;
        // This loop involves repeated memmoves (resulting in ~(n^2) work where n is
        // the block size).  There should be a way to drastically reduce the number
        // and increase efficiency.
        for (;;) {
            const btree_internal_pair *pair_to_move = get_pair_by_index(sibling, 0);
            uint16_t size_change = sizeof(*node->pair_offsets) + pair_size(pair_to_move);
            if (new_npairs * sizeof(*node->pair_offsets) + (block_size.value() - node->frontmost_offset) + size_change >= sibling->npairs * sizeof(*sibling->pair_offsets) + (block_size.value() - sibling->frontmost_offset) - size_change) {
                break;
            }

            const uint16_t new_offset = impl::insert_pair(node, pair_to_move);
            node->pair_offsets[new_npairs] = new_offset;
            ++new_npairs;
            if (moved_children_out != nullptr) {
                moved_children_out->push_back(pair_to_move->lnode);
            }

            impl::delete_pair(sibling, sibling->pair_offsets[0]);
            impl::delete_offset(sibling, 0);
        }

        const btree_internal_pair *pair_for_parent = get_pair_by_index(sibling, 0);

        node->pair_offsets[new_npairs] = special_pair_offset;
        ++new_npairs;

        node->npairs = new_npairs;

        btree_internal_pair *special_pair = get_pair(node, special_pair_offset);
        special_pair->lnode = pair_for_parent->lnode;

        keycpy(replacement_key, &pair_for_parent->key);

        if (moved_children_out != nullptr) {
            moved_children_out->push_back(pair_for_parent->lnode);
        }
        impl::delete_pair(sibling, sibling->pair_offsets[0]);
        impl::delete_offset(sibling, 0);
    } else {
        uint16_t offset;
        const btree_key_t *key_from_parent = &get_pair_by_index(parent, get_offset_index(parent, &get_pair_by_index(sibling, 0)->key))->key;
        if (sizeof(internal_node_t) + (node->npairs + 1) * sizeof(*node->pair_offsets) + impl::pair_size_with_key(key_from_parent) >= node->frontmost_offset)
            return false;
        block_id_t first_child = get_pair_by_index(sibling, sibling->npairs-1)->lnode;
        offset = impl::insert_pair(node, first_child, key_from_parent);
        impl::insert_offset(node, offset, 0);
        if (moved_children_out != nullptr) {
            moved_children_out->push_back(first_child);
        }
        impl::delete_pair(sibling, sibling->pair_offsets[sibling->npairs-1]);
        impl::delete_offset(sibling, sibling->npairs-1);

        // This loop involves repeated memmoves. There should be a way to
        // drastically reduce the number and increase efficiency.
        for (;;) {
            const btree_internal_pair *pair_to_move = get_pair_by_index(sibling, sibling->npairs-1);
            uint16_t size_change = sizeof(*node->pair_offsets) + pair_size(pair_to_move);
            if (node->npairs * sizeof(*node->pair_offsets) + (block_size.value() - node->frontmost_offset) + size_change >= sibling->npairs * sizeof(*sibling->pair_offsets) + (block_size.value() - sibling->frontmost_offset) - size_change) {
                break;
            }

            offset = impl::insert_pair(node, pair_to_move);
            impl::insert_offset(node, offset, 0);
            if (moved_children_out != nullptr) {
                moved_children_out->push_back(pair_to_move->lnode);
            }

            impl::delete_pair(sibling, sibling->pair_offsets[sibling->npairs-1]);
            impl::delete_offset(sibling, sibling->npairs-1);
        }

        keycpy(replacement_key, &get_pair_by_index(sibling, sibling->npairs-1)->key);

        impl::make_last_pair_special(sibling);
    }

    validate(block_size, node);
    validate(block_size, sibling);
    guarantee(!change_unsafe(node), "level made internal node dangerously full");
    return true;
}

int sibling(const internal_node_t *node, const btree_key_t *key, block_id_t *sib_id, store_key_t *key_in_middle_out) {
    int index = get_offset_index(node, key);
    const btree_internal_pair *sib_pair;
    int cmp;
    if (index > 0) {
        sib_pair = get_pair_by_index(node, index-1);
        key_in_middle_out->assign(&sib_pair->key);
        cmp = 1;
    } else {
        sib_pair = get_pair_by_index(node, index+1);
        key_in_middle_out->assign(&get_pair_by_index(node, index)->key);
        cmp = -1;
    }

    *sib_id = sib_pair->lnode;
    return cmp; //equivalent to nodecmp(node, sibling)
}


/* `is_sorted()` returns true if the given range is sorted. */

template <class ForwardIterator, class StrictWeakOrdering>
bool is_sorted(ForwardIterator first, ForwardIterator last,
                       StrictWeakOrdering comp) {
    for (ForwardIterator it = first; it + 1 < last; it++) {
        if (!comp(*it, *(it+1)))
            return false;
    }
    return true;
}

void update_key(internal_node_t *node, const btree_key_t *key_to_replace, const btree_key_t *replacement_key) {

    const int index = get_offset_index(node, key_to_replace);
    const block_id_t tmp_lnode = get_pair_by_index(node, index)->lnode;
    impl::delete_pair(node, node->pair_offsets[index]);

    guarantee(sizeof(internal_node_t) + (node->npairs) * sizeof(*node->pair_offsets) + impl::pair_size_with_key(replacement_key) < node->frontmost_offset,
        "cannot fit updated key in internal node");

    const uint16_t new_offset = impl::insert_pair(node, tmp_lnode, replacement_key);
    node->pair_offsets[index] = new_offset;

    rassert(is_sorted(node->pair_offsets, node->pair_offsets+node->npairs-1, internal_key_comp(node)),
            "Invalid key given to update_key: offsets no longer in sorted order");
    rassert(get_pair_by_index(node, node->npairs-1)->key.size == 0);
}

bool is_full(const internal_node_t *node) {
    return sizeof(internal_node_t) + (node->npairs + 1) * sizeof(*node->pair_offsets) + impl::pair_size_with_key_size(MAX_KEY_SIZE) >=  node->frontmost_offset;
}

bool change_unsafe(const internal_node_t *node) {
    return sizeof(internal_node_t) + node->npairs * sizeof(*node->pair_offsets) + MAX_KEY_SIZE >= node->frontmost_offset;
}

void validate(DEBUG_VAR block_size_t block_size, DEBUG_VAR const internal_node_t *node) {
#ifndef NDEBUG
    rassert(reinterpret_cast<const char *>(&(node->pair_offsets[node->npairs])) <= reinterpret_cast<const char *>(get_pair(node, node->frontmost_offset)));
    rassert(node->frontmost_offset > 0);
    rassert(node->frontmost_offset <= block_size.value());
    for (int i = 0; i < node->npairs; i++) {
        rassert(node->pair_offsets[i] < block_size.value());
        rassert(node->pair_offsets[i] >= node->frontmost_offset);
    }
    rassert(is_sorted(node->pair_offsets, node->pair_offsets+node->npairs-1, internal_key_comp(node)),
        "Offsets no longer in sorted order");
    rassert(get_pair_by_index(node, node->npairs-1)->key.size == 0);
#endif
}

bool is_underfull(block_size_t block_size, const internal_node_t *node) {
    return (sizeof(internal_node_t) + 1) / 2 +
        node->npairs*sizeof(*node->pair_offsets) +
        (block_size.value() - node->frontmost_offset) +
        /* EPSILON TODO this epsilon is too high lower it*/
        INTERNAL_EPSILON * 2  < block_size.value() / 2;
}

bool is_mergable(block_size_t block_size, const internal_node_t *node, const internal_node_t *sibling, const internal_node_t *parent) {
    const btree_key_t *key_from_parent;
    if (nodecmp(node, sibling) < 0) {
        key_from_parent = &get_pair_by_index(parent, get_offset_index(parent, &get_pair_by_index(node, 0)->key))->key;
    } else {
        key_from_parent = &get_pair_by_index(parent, get_offset_index(parent, &get_pair_by_index(sibling, 0)->key))->key;
    }
    return sizeof(internal_node_t) +
        (node->npairs + sibling->npairs + 1)*sizeof(*node->pair_offsets) +
        (block_size.value() - node->frontmost_offset) +
        (block_size.value() - sibling->frontmost_offset) + key_from_parent->size +
        impl::pair_size_with_key_size(MAX_KEY_SIZE) +
        INTERNAL_EPSILON < block_size.value(); // must still have enough room for an arbitrary key  // TODO: we can't be tighter?
}

bool is_doubleton(const internal_node_t *node) {
    return node->npairs == 2;
}

size_t pair_size(const btree_internal_pair *pair) {
    return impl::pair_size_with_key_size(pair->key.size);
}

const btree_internal_pair *get_pair(const internal_node_t *node, uint16_t offset) {
    return reinterpret_cast<const btree_internal_pair *>(reinterpret_cast<const char *>(node) + offset);
}

btree_internal_pair *get_pair(internal_node_t *node, uint16_t offset) {
    return reinterpret_cast<btree_internal_pair *>(reinterpret_cast<char *>(node) + offset);
}

const btree_internal_pair *get_pair_by_index(const internal_node_t *node, int index) {
    return get_pair(node, node->pair_offsets[index]);
}
btree_internal_pair *get_pair_by_index(internal_node_t *node, int index) {
    return get_pair(node, node->pair_offsets[index]);
}

int get_offset_index(const internal_node_t *node, const btree_key_t *key) {
    return std::lower_bound(node->pair_offsets, node->pair_offsets+node->npairs-1, (uint16_t) internal_key_comp::faux_offset, internal_key_comp(node, key)) - node->pair_offsets;
}

int nodecmp(const internal_node_t *node1, const internal_node_t *node2) {
    const btree_key_t *key1 = &get_pair_by_index(node1, 0)->key;
    const btree_key_t *key2 = &get_pair_by_index(node2, 0)->key;

    return btree_key_cmp(key1, key2);
}

namespace impl {

size_t pair_size_with_key(const btree_key_t *key) {
    return pair_size_with_key_size(key->size);
}

size_t pair_size_with_key_size(uint8_t size) {
    return offsetof(btree_internal_pair, key) + offsetof(btree_key_t, contents) + size;
}

void delete_pair(internal_node_t *node, uint16_t offset) {
    btree_internal_pair *pair_to_delete = get_pair(node, offset);
    btree_internal_pair *front_pair = get_pair(node, node->frontmost_offset);
    const size_t shift = pair_size(pair_to_delete);
    const size_t size = offset - node->frontmost_offset;

    rassert(node->magic == internal_node_t::expected_magic);
    memmove(reinterpret_cast<char *>(front_pair) + shift, front_pair, size);
    rassert(node->magic == internal_node_t::expected_magic);


    node->frontmost_offset = node->frontmost_offset + shift;

    scoped_array_t<uint16_t> new_pair_offsets(node->npairs);
    memcpy(new_pair_offsets.data(), node->pair_offsets, sizeof(uint16_t) * node->npairs);

    for (int i = 0; i < node->npairs; i++) {
        if (new_pair_offsets[i] < offset)
            new_pair_offsets[i] += shift;
    }

    memcpy(node->pair_offsets, new_pair_offsets.data(), sizeof(uint16_t) * node->npairs);
}

uint16_t insert_pair(internal_node_t *node, const btree_internal_pair *pair) {
    const uint16_t frontmost_offset = node->frontmost_offset - pair_size(pair);
    node->frontmost_offset = frontmost_offset;

    // insert contents
    memcpy(get_pair(node, frontmost_offset), pair, pair_size(pair));
    return frontmost_offset;
}

uint16_t insert_pair(internal_node_t *node, block_id_t lnode, const btree_key_t *key) {
    const uint16_t frontmost_offset = node->frontmost_offset - pair_size_with_key(key);
    node->frontmost_offset = frontmost_offset;

    btree_internal_pair *new_pair = get_pair(node, frontmost_offset);

    // Use a buffer to prepare the key/value pair which we can then use to generate a patch
    scoped_array_t<char> pair_buf(pair_size_with_key(key));
    btree_internal_pair *new_buf_pair = reinterpret_cast<btree_internal_pair *>(pair_buf.data());

    // insert contents
    new_buf_pair->lnode = lnode;
    keycpy(&new_buf_pair->key, key);

    // Patch the new pair into node_buf
    memcpy(new_pair, new_buf_pair, pair_size_with_key(key));

    return frontmost_offset;
}

void delete_offset(internal_node_t *node, int index) {
    uint16_t *pair_offsets = node->pair_offsets;
    if (node->npairs > 1) {
        memmove(pair_offsets + index, pair_offsets + index + 1, (node->npairs - index - 1) * sizeof(uint16_t));
    }
    node->npairs -= 1;
}

void insert_offset(internal_node_t *node, uint16_t offset, int index) {
    uint16_t *pair_offsets = node->pair_offsets;
    memmove(pair_offsets + index + 1, pair_offsets + index, (node->npairs - index) * sizeof(uint16_t));
    pair_offsets[index] = offset;
    node->npairs += 1;
}

void make_last_pair_special(internal_node_t *node) {
    const int index = node->npairs - 1;
    const uint16_t old_offset = node->pair_offsets[index];
    btree_key_t tmp;
    tmp.size = 0;
    const uint16_t new_offset = insert_pair(node, get_pair(node, old_offset)->lnode, &tmp);
    node->pair_offsets[index] = new_offset;
    delete_pair(node, old_offset);
}


bool is_equal(const btree_key_t *key1, const btree_key_t *key2) {
    return btree_key_cmp(key1, key2) == 0;
}

}  // namespace impl

}  // namespace internal_node

#if defined(__GNUC__) && (100 * __GNUC__ + __GNUC_MINOR__ >= 901)
#pragma GCC diagnostic pop
#endif
