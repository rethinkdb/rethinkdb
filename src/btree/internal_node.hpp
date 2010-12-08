#ifndef __BTREE_INTERNAL_NODE_HPP__
#define __BTREE_INTERNAL_NODE_HPP__

#include "utils.hpp"
#include "btree/node.hpp"

// See internal_node_t in node.hpp

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
    static void init(block_size_t block_size, internal_node_t *node);
    static void init(block_size_t block_size, internal_node_t *node, internal_node_t *lnode, uint16_t *offsets, int numpairs);

    static block_id_t lookup(const internal_node_t *node, btree_key *key);
    static bool insert(block_size_t block_size, internal_node_t *node, btree_key *key, block_id_t lnode, block_id_t rnode);
    static bool remove(block_size_t block_size, internal_node_t *node, btree_key *key);
    static void split(block_size_t block_size, internal_node_t *node, internal_node_t *rnode, btree_key *median);
    static void merge(block_size_t block_size, internal_node_t *node, internal_node_t *rnode, btree_key *key_to_remove, internal_node_t *parent);
    static bool level(block_size_t block_size, internal_node_t *node, internal_node_t *rnode, btree_key *key_to_replace, btree_key *replacement_key, internal_node_t *parent);
    static int sibling(const internal_node_t *node, btree_key *key, block_id_t *sib_id);
    static void update_key(internal_node_t *node, btree_key *key_to_replace, btree_key *replacement_key);
    static int nodecmp(const internal_node_t *node1, const internal_node_t *node2);
    static bool is_full(const internal_node_t *node);
    static bool is_underfull(block_size_t block_size, const internal_node_t *node);
    static bool change_unsafe(const internal_node_t *node);
    static bool is_mergable(block_size_t block_size, const internal_node_t *node, const internal_node_t *sibling, const internal_node_t *parent);
    static bool is_singleton(const internal_node_t *node);

    static void validate(block_size_t block_size, const internal_node_t *node);
    static void print(const internal_node_t *node);

    static inline const internal_node_t* internal_node(const void *ptr) {
        return (const internal_node_t *) ptr;
    }
    static inline internal_node_t* internal_node(void *ptr){
        return (internal_node_t *) ptr;
    }

    static size_t pair_size(btree_internal_pair *pair);
    static btree_internal_pair *get_pair(const internal_node_t *node, uint16_t offset);

protected:
    static size_t pair_size_with_key(btree_key *key);
    static size_t pair_size_with_key_size(uint8_t size);

    static void delete_pair(internal_node_t *node, uint16_t offset);
    static uint16_t insert_pair(internal_node_t *node, btree_internal_pair *pair);
    static uint16_t insert_pair(internal_node_t *node, block_id_t lnode, btree_key *key);
    static int get_offset_index(const internal_node_t *node, btree_key *key);
    static void delete_offset(internal_node_t *node, int index);
    static void insert_offset(internal_node_t *node, uint16_t offset, int index);
    static void make_last_pair_special(internal_node_t *node);
    static bool is_equal(btree_key *key1, btree_key *key2);
};

class internal_key_comp {
    const internal_node_t *node;
    btree_key *key;
    public:
    explicit internal_key_comp(const internal_node_t *_node) : node(_node), key(NULL)  { }
    internal_key_comp(const internal_node_t *_node, btree_key *_key) : node(_node), key(_key)  { }
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
