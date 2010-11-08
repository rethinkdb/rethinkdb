#ifndef __BTREE_LEAF_NODE_HPP__
#define __BTREE_LEAF_NODE_HPP__

#include "utils.hpp"
#include "btree/node.hpp"
#include "config/args.hpp"

// See btree_leaf_node in node.hpp

/* EPSILON to prevent split then merge bug */
#define LEAF_EPSILON (sizeof(btree_key) + MAX_KEY_SIZE + sizeof(btree_value) + MAX_TOTAL_NODE_CONTENTS_SIZE)


//Note: This struct is stored directly on disk.  Changing it invalidates old data.
struct btree_leaf_pair {
    btree_key key;
    // We have a sizeof(btree_leaf_pair) that depends on the presence of this value!!
    btree_value value_;
    // key is of variable size and there's a btree_value that follows
    // it that is of variable size.
    btree_value *value() {
        return (btree_value *)( ((byte *)&key) + sizeof(btree_key) + key.size );
    }
};

class leaf_key_comp;

class leaf_node_handler : public node_handler {
    friend class leaf_key_comp;
    public:
    static void init(size_t block_size, btree_leaf_node *node);
    static void init(size_t block_size, btree_leaf_node *node, btree_leaf_node *lnode, uint16_t *offsets, int numpairs);

    static bool lookup(const btree_leaf_node *node, btree_key *key, btree_value *value);
    static bool insert(size_t block_size, btree_leaf_node *node, btree_key *key, btree_value *value);
    static void remove(size_t block_size, btree_leaf_node *node, btree_key *key); // TODO: Currently untested
    static void split(size_t block_size, btree_leaf_node *node, btree_leaf_node *rnode, btree_key *median);
    static void merge(size_t block_size, btree_leaf_node *node, btree_leaf_node *rnode, btree_key *key_to_remove);
    static bool level(size_t block_size, btree_leaf_node *node, btree_leaf_node *sibling, btree_key *key_to_replace, btree_key *replacement_key);


    static bool is_empty(const btree_leaf_node *node);
    static bool is_full(const btree_leaf_node *node, btree_key *key, btree_value *value);
    static bool is_underfull(size_t block_size, const btree_leaf_node *node);
    static bool is_mergable(size_t block_size, const btree_leaf_node *node, const btree_leaf_node *sibling);
    static void validate(size_t block_size, const btree_leaf_node *node);
    static int nodecmp(const btree_leaf_node *node1, const btree_leaf_node *node2);

    static void print(const btree_leaf_node *node);

    static inline const leaf_node_t* leaf_node(const void *ptr) {
        return (const leaf_node_t *) ptr;
    }
    static inline leaf_node_t* leaf_node(void *ptr) {
        return (leaf_node_t *) ptr;
    }

    static btree_leaf_pair *get_pair(const btree_leaf_node *node, uint16_t offset);

    protected:
    static size_t pair_size(btree_leaf_pair *pair);
    static void delete_pair(btree_leaf_node *node, uint16_t offset);
    static uint16_t insert_pair(btree_leaf_node *node, btree_leaf_pair *pair);
    static uint16_t insert_pair(btree_leaf_node *node, btree_value *value, btree_key *key);
    static int get_offset_index(const btree_leaf_node *node, btree_key *key);
    static int find_key(const btree_leaf_node *node, btree_key *key);
    static void shift_pairs(btree_leaf_node *node, uint16_t offset, long shift);
    static void delete_offset(btree_leaf_node *node, int index);
    static void insert_offset(btree_leaf_node *node, uint16_t offset, int index);
    static bool is_equal(btree_key *key1, btree_key *key2);
};

class leaf_key_comp {
    const btree_leaf_node *node;
    btree_key *key;
    public:
    leaf_key_comp(const btree_leaf_node *_node) : node(_node), key(NULL)  { };
    leaf_key_comp(const btree_leaf_node *_node, btree_key *_key) : node(_node), key(_key)  { };
    bool operator()(const uint16_t offset1, const uint16_t offset2) {
        btree_key *key1 = offset1 == 0 ? key : &leaf_node_handler::get_pair(node, offset1)->key;
        btree_key *key2 = offset2 == 0 ? key : &leaf_node_handler::get_pair(node, offset2)->key;
        int cmp = leaf_key_comp::compare(key1, key2);

        return cmp < 0;
    }
    static int compare(btree_key *key1, btree_key *key2) {
        return sized_strcmp(key1->contents, key1->size, key2->contents, key2->size);
    }
};


#endif // __BTREE_LEAF_NODE_HPP__
