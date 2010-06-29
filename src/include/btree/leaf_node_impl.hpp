#ifndef __BTREE_LEAF_NODE_IMPL_HPP__
#define __BTREE_LEAF_NODE_IMPL_HPP__

#include "btree/leaf_node.hpp"
#include <algorithm>

void btree_leaf::init(btree_leaf_node *node) {
    printf("leaf node %p: init\n", node);
    node->leaf = true;
    node->nblobs = 0;
    node->frontmost_offset = BTREE_BLOCK_SIZE;
}

void btree_leaf::init(btree_leaf_node *node, btree_leaf_node *lnode, uint16_t *offsets, int numblobs) {
    printf("leaf node %p: init from node %p\n", node, lnode);
    init(node);
    for (int i = 0; i < numblobs; i++) {
        node->blob_offsets[node->nblobs+i] = insert_blob(node, get_blob(lnode, offsets[i]));
    }
    node->nblobs += numblobs;
    std::sort(node->blob_offsets, node->blob_offsets+node->nblobs, btree_leaf_str_key_comp(node));
}

int btree_leaf::insert(btree_leaf_node *node, btree_key *key, int value) {
    printf("leaf node %p: insert\tkey:%*.*s\tvalue:%d\n", node, key->size, key->size, key->contents, value);
    if (is_full(node, key)) return 0;
    int index = get_offset_index(node, key);
    btree_leaf_blob *previous = get_blob(node, node->blob_offsets[index]);
    //TODO: write a unit test for this
    if (is_equal(&previous->key, key)) { // a duplicate key is being inserted
        previous->value = value;
    } else {
        uint16_t offset = insert_blob(node, value, key);
        insert_offset(node, offset, index);
    }
    return 1;
}

bool btree_leaf::lookup(btree_leaf_node *node, btree_key *key, int *value) {
    printf("leaf node %p: lookup\tkey:%*.*s\n", node, key->size, key->size, key->contents);
    int index = get_offset_index(node, key);
    block_id_t offset = node->blob_offsets[index];
    btree_leaf_blob *blob = get_blob(node, offset);
    if (is_equal(&blob->key, key)) {
        *value = blob->value;
        return true;
    } else {
        return false;
    }
}

void btree_leaf::split(btree_leaf_node *node, btree_leaf_node *rnode, btree_key *median) {
    printf("leaf node %p: split\n", node);
    uint16_t total_blobs = BTREE_BLOCK_SIZE - node->frontmost_offset;
    uint16_t first_blobs = 0;
    int index = 0;
    while (first_blobs < total_blobs/2) { // finds the median index
        first_blobs += blob_size(get_blob(node, node->blob_offsets[index]));
        index++;
    }
    int median_index = index;

    init(rnode, node, node->blob_offsets + median_index, node->nblobs - median_index);

    // TODO: This is really slow because most blobs will likely be copied
    // repeatedly.  There should be a better way.
    for (index = median_index; index < node->nblobs; index++) {
        delete_blob(node, node->blob_offsets[index]);
    }

    node->nblobs = median_index;

    // Equality takes the left branch, so the median should be from this node.
    btree_key *median_key = &get_blob(node, node->blob_offsets[median_index-1])->key;
    memcpy(median, median_key, sizeof(btree_key)+median_key->size);

}

bool btree_leaf::remove(btree_leaf_node *node, btree_key *key) {
    int index = find_key(node, key);

    if (index != -1) {
        delete_blob(node, node->blob_offsets[index]);
        delete_offset(node, index);
        return true;
    } else {
        return false;
    }
}

//TODO: Make a more specific full function
bool btree_leaf::is_full(btree_leaf_node *node, btree_key *key) {
#ifdef DEBUG_MAX_LEAF
    if (node->nblobs >= DEBUG_MAX_LEAF)
        return true;
#endif
    return sizeof(btree_leaf_node) + node->nblobs*sizeof(*node->blob_offsets) + sizeof(btree_leaf_blob) + key->size >= node->frontmost_offset;
}

size_t btree_leaf::blob_size(btree_leaf_blob *blob) {
    return sizeof(btree_leaf_blob) + blob->key.size;
}

btree_leaf_blob *btree_leaf::get_blob(btree_leaf_node *node, uint16_t offset) {
    return (btree_leaf_blob *)( ((byte *)node) + offset);
}

void btree_leaf::delete_blob(btree_leaf_node *node, uint16_t offset) {
    btree_leaf_blob *blob_to_delete = get_blob(node, offset);
    btree_leaf_blob *front_blob = get_blob(node, node->frontmost_offset);
    size_t shift = blob_size(blob_to_delete);

    memmove( ((byte *)front_blob)+shift, front_blob, offset - node->frontmost_offset);
    node->frontmost_offset += shift;
    for (int i = 0; i < node->nblobs; i++) {
        if (node->blob_offsets[i] < offset)
            node->blob_offsets[i] += shift;
    }
}

uint16_t btree_leaf::insert_blob(btree_leaf_node *node, btree_leaf_blob *blob) {
    node->frontmost_offset -= blob_size(blob);
    btree_leaf_blob *new_blob = get_blob(node, node->frontmost_offset);

    // insert contents
    memcpy(new_blob, blob, blob_size(blob));

    return node->frontmost_offset;
}

uint16_t btree_leaf::insert_blob(btree_leaf_node *node, int value, btree_key *key) {
    node->frontmost_offset -= sizeof(btree_leaf_blob) + key->size;;
    btree_leaf_blob *new_blob = get_blob(node, node->frontmost_offset);

    // insert contents
    new_blob->value = value;
    memcpy(&new_blob->key, key, sizeof(*key) + key->size);

    return node->frontmost_offset;
}

int btree_leaf::get_offset_index(btree_leaf_node *node, btree_key *key) {
    return std::lower_bound(node->blob_offsets, node->blob_offsets+node->nblobs, NULL, btree_leaf_str_key_comp(node, key)) - node->blob_offsets;
}

int btree_leaf::find_key(btree_leaf_node *node, btree_key *key) {
    int index = get_offset_index(node, key);
    if (index < node->nblobs && is_equal(key, &get_blob(node, node->blob_offsets[index])->key) ) {
        return index;
    } else {
        return -1;
    }

}
void btree_leaf::delete_offset(btree_leaf_node *node, int index) {
    uint16_t *blob_offsets = node->blob_offsets;
    if (node->nblobs > 1)
        memmove(blob_offsets+index, blob_offsets+index+1, (node->nblobs-index-1) * sizeof(uint16_t));
    node->nblobs--;
}

void btree_leaf::insert_offset(btree_leaf_node *node, uint16_t offset, int index) {
    uint16_t *blob_offsets = node->blob_offsets;
    memmove(blob_offsets+index+1, blob_offsets+index, (node->nblobs-index) * sizeof(uint16_t));
    blob_offsets[index] = offset;
    node->nblobs++;
}


bool btree_leaf::is_equal(btree_key *key1, btree_key *key2) {
    return sized_strcmp(key1->contents, key1->size, key2->contents, key2->size) == 0;
}

#endif // __BTREE_LEAF_NODE_IMPL_HPP__
