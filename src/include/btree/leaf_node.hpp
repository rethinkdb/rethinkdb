#ifndef __BTREE_LEAF_NODE_HPP__
#define __BTREE_LEAF_NODE_HPP__

#include "utils.hpp"
#include "btree/node.hpp"

#define byte char

//Note: This struct is stored directly on disk.  Changing it invalidates old data.
struct btree_leaf_blob {
    int value;
    btree_key key;
};


//Note: This struct is stored directly on disk.  Changing it invalidates old data.
struct btree_leaf_node {
    bool leaf;
    uint16_t nblobs;
    uint16_t frontmost_offset;
    uint16_t blob_offsets[0];
};

typedef btree_leaf_node leaf_node_t;

class btree_leaf_str_key_comp;

class btree_leaf : public btree {
    friend class btree_leaf_str_key_comp;
    public:
    static void init(btree_leaf_node *node);
    static void init(btree_leaf_node *node, btree_leaf_node *lnode, uint16_t *offsets, int numblobs);

    static int insert(btree_leaf_node *node, btree_key *key, int value);
    static bool lookup(btree_leaf_node *node, btree_key *key, int *value);
    static void split(btree_leaf_node *node, btree_leaf_node *rnode, btree_key *median);
    static bool remove(btree_leaf_node *node, btree_key *key);

    static bool is_full(btree_leaf_node *node, btree_key *key);

    protected:
    static size_t blob_size(btree_leaf_blob *blob);
    static btree_leaf_blob *get_blob(btree_leaf_node *node, uint16_t offset);
    static void delete_blob(btree_leaf_node *node, uint16_t offset);
    static uint16_t insert_blob(btree_leaf_node *node, btree_leaf_blob *blob);
    static uint16_t insert_blob(btree_leaf_node *node, int value, btree_key *key);
    static int get_offset_index(btree_leaf_node *node, btree_key *key);
    static int find_key(btree_leaf_node *node, btree_key *key);
    static void delete_offset(btree_leaf_node *node, int index);
    static void insert_offset(btree_leaf_node *node, uint16_t offset, int index);
    static bool is_equal(btree_key *key1, btree_key *key2);
};

class btree_leaf_str_key_comp {
    btree_leaf_node *node;
    btree_key *key;
    public:
    btree_leaf_str_key_comp(btree_leaf_node *_node) : node(_node), key(NULL)  { };
    btree_leaf_str_key_comp(btree_leaf_node *_node, btree_key *_key) : node(_node), key(_key)  { };
    bool operator()(const uint16_t offset1, const uint16_t offset2) {
        btree_key *key1 = offset1 == 0 ? key : &btree_leaf::get_blob(node, offset1)->key;
        btree_key *key2 = offset2 == 0 ? key : &btree_leaf::get_blob(node, offset2)->key;
        int cmp;
        if (key1->size==255 && key2->size==255) //check for the special end blob
            cmp = 0;
        else if (key1->size==255)
            cmp = 1;
        else if (key2->size==255)
            cmp = -1;
        else
            cmp = sized_strcmp(key1->contents, key1->size, key2->contents, key2->size);

        return cmp < 0;
    }
};



#endif // __BTREE_LEAF_NODE_HPP__
