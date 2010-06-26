#ifndef __BTREE_INTERNAL_NODE_IMPL_HPP__
#define __BTREE_INTERNAL_NODE_IMPL_HPP__

#include "btree/btree_internal_node.hpp"
#include <algorithm>

#define DEBUG_MAX_INTERNAL 4

//In this tree, less than or equal takes the left-hand branch and greater than takes the right hand branch

void btree_internal::init(btree_internal_node *node) {
    printf("internal node %p: init\n", node);
    node->leaf = false;
    node->nblobs = 0;
    node->frontmost_offset = BTREE_BLOCK_SIZE;
}

void btree_internal::init(btree_internal_node *node, btree_internal_node *lnode, uint16_t *offsets, int numblobs) {
    printf("internal node %p: init from node %p\n", node, lnode);
    init(node);
    for (int i = 0; i < numblobs; i++) {
        node->blob_offsets[node->nblobs+i] = insert_blob(node, get_blob(lnode, offsets[i]));
    }
    node->nblobs += numblobs;
    std::sort(node->blob_offsets, node->blob_offsets+node->nblobs, btree_str_key_comp(node));
}

int btree_internal::insert(btree_internal_node *node, btree_key *key, off64_t lnode, off64_t rnode) {
    printf("internal node %p: insert\tkey:%*.*s\tlnode:%llu\trnode:%llu\n", node, key->size, key->size, key->contents, (unsigned long long)lnode, (unsigned long long)rnode);
    //check("tried to insert into a full internal node", is_full(node));
    if (is_full(node)) return 0;
    check("key too large", key->size > MAX_KEY_SIZE);
    if (node->nblobs == 0) {
        btree_key special;
        special.size=0;

        uint16_t special_offset = insert_blob(node, rnode, &special);
        insert_offset(node, special_offset, 0);
    }

    uint16_t offset = insert_blob(node, lnode, key);
    int index = get_offset_index(node, key);
    insert_offset(node, offset, index);

    get_blob(node, node->blob_offsets[index+1])->lnode = rnode;
    return 1; // XXX
}

off64_t btree_internal::lookup(btree_internal_node *node, btree_key *key) {
    printf("internal node %p: lookup\tkey:%*.*s\n", node, key->size, key->size, key->contents);
    int index = get_offset_index(node, key);
    return get_blob(node, node->blob_offsets[index])->lnode;
}

void btree_internal::split(btree_internal_node *node, btree_internal_node *rnode, btree_key **median) {
    printf("internal node %p: split\n", node);
    uint16_t total_blobs = BTREE_BLOCK_SIZE - node->frontmost_offset;
    uint16_t first_blobs = 0;
    int index = 0;
    while (first_blobs < total_blobs/2) { // finds the median index
        first_blobs += blob_size(get_blob(node, node->blob_offsets[index]));
        index++;
    }
    int median_index = index;

    // XXX Do not use malloc here
    // Also, equality takes the left branch, so the median should be from this node.
    btree_key *median_key = &get_blob(node, node->blob_offsets[median_index-1])->key;
    int median_size = sizeof(btree_key) + median_key->size;
    *median = (btree_key *)malloc(median_size);
    memcpy(*median, median_key, median_size);

    init(rnode, node, node->blob_offsets + median_index, node->nblobs - median_index);

    // TODO: This is really slow because most blobs will likely be copied
    // repeatedly.  There should be a better way.
    for (index = median_index; index < node->nblobs; index++) {
        delete_blob(node, node->blob_offsets[index]);
    }

    node->nblobs = median_index;
    //make last blob special
    make_last_blob_special(node);
}

bool btree_internal::remove(btree_internal_node *node, btree_key *key) {
    int index = get_offset_index(node, key);

    make_last_blob_special(node);

    delete_blob(node, node->blob_offsets[index]);
    delete_offset(node, index);

    return true; // XXX
}

bool btree_internal::is_full(btree_internal_node *node) {
#ifdef DEBUG_MAX_INTERNAL
    if (node->nblobs-1 >= DEBUG_MAX_INTERNAL)
        return true;
#endif
    return sizeof(btree_internal_node) + node->nblobs*sizeof(*node->blob_offsets) + MAX_KEY_SIZE >= node->frontmost_offset;
}

size_t btree_internal::blob_size(btree_internal_blob *blob) {
    return sizeof(btree_internal_blob) + blob->key.size;
}

btree_internal_blob *btree_internal::get_blob(btree_internal_node *node, uint16_t offset) {
    return (btree_internal_blob *)( ((byte *)node) + offset);
}

void btree_internal::delete_blob(btree_internal_node *node, uint16_t offset) {
    btree_internal_blob *blob_to_delete = get_blob(node, offset);
    btree_internal_blob *front_blob = get_blob(node, node->frontmost_offset);
    size_t shift = blob_size(blob_to_delete);

    memmove( ((byte *)front_blob)+shift, front_blob, offset - node->frontmost_offset);
    node->frontmost_offset += shift;
    for (int i = 0; i < node->nblobs; i++) {
        if (node->blob_offsets[i] < offset)
            node->blob_offsets[i] += shift;
    }
}

uint16_t btree_internal::insert_blob(btree_internal_node *node, btree_internal_blob *blob) {
    node->frontmost_offset -= blob_size(blob);
    btree_internal_blob *new_blob = get_blob(node, node->frontmost_offset);

    // insert contents
    memcpy(new_blob, blob, blob_size(blob));

    return node->frontmost_offset;
}

uint16_t btree_internal::insert_blob(btree_internal_node *node, off64_t lnode, btree_key *key) {
    node->frontmost_offset -= sizeof(btree_internal_blob) + key->size;;
    btree_internal_blob *new_blob = get_blob(node, node->frontmost_offset);

    // insert contents
    new_blob->lnode = lnode;
    memcpy(&new_blob->key, key, sizeof(*key) + key->size);

    return node->frontmost_offset;
}

int btree_internal::get_offset_index(btree_internal_node *node, btree_key *key) {
    return std::lower_bound(node->blob_offsets, node->blob_offsets+node->nblobs, NULL, btree_str_key_comp(node, key)) - node->blob_offsets;
}

void btree_internal::delete_offset(btree_internal_node *node, int index) {
    uint16_t *blob_offsets = node->blob_offsets;
    if (node->nblobs > 1)
        memmove(blob_offsets+index, blob_offsets+index+1, (node->nblobs-index-1) * sizeof(uint16_t));
    node->nblobs--;
}

void btree_internal::insert_offset(btree_internal_node *node, uint16_t offset, int index) {
    uint16_t *blob_offsets = node->blob_offsets;
    memmove(blob_offsets+index+1, blob_offsets+index, (node->nblobs-index) * sizeof(uint16_t));
    blob_offsets[index] = offset;
    node->nblobs++;
}

void btree_internal::make_last_blob_special(btree_internal_node *node) {
    int index = node->nblobs-1;
    uint16_t old_offset = node->blob_offsets[index];
    btree_key tmp;
    tmp.size = 0;
    node->blob_offsets[index] = insert_blob(node, get_blob(node, old_offset)->lnode, &tmp);
    delete_blob(node, old_offset);
}


bool btree_internal::is_equal(btree_key *key1, btree_key *key2) {
    return sized_strcmp(key1->contents, key1->size, key2->contents, key2->size) == 0;
}

#endif // __BTREE_INTERNAL_NODE_IMPL_HPP__
