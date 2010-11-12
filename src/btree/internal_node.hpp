#ifndef __BTREE_INTERNAL_NODE_HPP__
#define __BTREE_INTERNAL_NODE_HPP__

#include "utils.hpp"
#include "btree/node.hpp"

// See btree_internal_node in node.hpp

/* EPSILON used to prevent split then merge */
#define INTERNAL_EPSILON (sizeof(btree_key) + MAX_KEY_SIZE + sizeof(block_id_t))

//Note: This struct is stored directly on disk.  Changing it invalidates old data.
struct btree_internal_pair {
    block_id_t lnode;
    btree_key key;
};


class internal_key_comp;

class internal_node_handler : public node_handler {
    friend class internal_key_comp;
public:
    static void init(size_t block_size, btree_internal_node *node);
    static void init(size_t block_size, btree_internal_node *node, btree_internal_node *lnode, uint16_t *offsets, int numpairs);

    static block_id_t lookup(const btree_internal_node *node, btree_key *key);
    static bool insert(size_t block_size, btree_internal_node *node, btree_key *key, block_id_t lnode, block_id_t rnode);
    static bool remove(size_t block_size, btree_internal_node *node, btree_key *key);
    static void split(size_t block_size, btree_internal_node *node, btree_internal_node *rnode, btree_key *median);
    static void merge(size_t block_size, btree_internal_node *node, btree_internal_node *rnode, btree_key *key_to_remove, btree_internal_node *parent);
    static bool level(size_t block_size, btree_internal_node *node, btree_internal_node *rnode, btree_key *key_to_replace, btree_key *replacement_key, btree_internal_node *parent);
    static int sibling(const btree_internal_node *node, btree_key *key, block_id_t *sib_id);
    static void update_key(btree_internal_node *node, btree_key *key_to_replace, btree_key *replacement_key);
    static int nodecmp(const btree_internal_node *node1, const btree_internal_node *node2);
    static bool is_full(const btree_internal_node *node);
    static bool is_underfull(size_t block_size, const btree_internal_node *node);
    static bool change_unsafe(const btree_internal_node *node);
    static bool is_mergable(size_t block_size, const btree_internal_node *node, const btree_internal_node *sibling, const btree_internal_node *parent);
    static bool is_singleton(const btree_internal_node *node);

    static void validate(size_t block_size, const btree_internal_node *node);
    static void print(const btree_internal_node *node);

    static inline const internal_node_t* internal_node(const void *ptr) {
        return (const internal_node_t *) ptr;
    }
    static inline internal_node_t* internal_node(void *ptr){
        return (internal_node_t *) ptr;
    }

    static size_t pair_size(btree_internal_pair *pair);
    static btree_internal_pair *get_pair(const btree_internal_node *node, uint16_t offset);

protected:
    static void delete_pair(btree_internal_node *node, uint16_t offset);
    static uint16_t insert_pair(btree_internal_node *node, btree_internal_pair *pair);
    static uint16_t insert_pair(btree_internal_node *node, block_id_t lnode, btree_key *key);
    static int get_offset_index(const btree_internal_node *node, btree_key *key);
    static void delete_offset(btree_internal_node *node, int index);
    static void insert_offset(btree_internal_node *node, uint16_t offset, int index);
    static void make_last_pair_special(btree_internal_node *node);
    static bool is_equal(btree_key *key1, btree_key *key2);
};

class internal_key_comp {
    const btree_internal_node *node;
    btree_key *key;
    public:
    internal_key_comp(const btree_internal_node *_node) : node(_node), key(NULL)  { };
    internal_key_comp(const btree_internal_node *_node, btree_key *_key) : node(_node), key(_key)  { };
    bool operator()(const uint16_t offset1, const uint16_t offset2) {
        btree_key *key1 = offset1 == 0 ? key : &internal_node_handler::get_pair(node, offset1)->key;
        btree_key *key2 = offset2 == 0 ? key : &internal_node_handler::get_pair(node, offset2)->key;
        return compare(key1, key2) < 0;
    }
    static int compare(btree_key *key1, btree_key *key2) {
        if (key1->size == 0 && key2->size == 0) //check for the special end pair
            return 0;
        else if (key1->size == 0)
            return 1;
        else if (key2->size == 0)
            return -1;
        else
            return sized_strcmp(key1->contents, key1->size, key2->contents, key2->size);
    }
};

#endif // __BTREE_INTERNAL_NODE_HPP__
