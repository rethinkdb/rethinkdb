#ifndef __BTREE_LEAF_NODE_HPP__
#define __BTREE_LEAF_NODE_HPP__

#include "utils.hpp"
#include "btree/node.hpp"
#include "config/args.hpp"

// See leaf_node_t in node.hpp

/* EPSILON to prevent split then merge bug */
#define LEAF_EPSILON (sizeof(btree_key) + MAX_KEY_SIZE + sizeof(btree_value) + MAX_TOTAL_NODE_CONTENTS_SIZE)


// Note: This struct is stored directly on disk.  Changing it
// invalidates old data.  (It's not really representative of what's
// stored on disk, but be aware of how you might invalidate old data.)
struct btree_leaf_pair {
    btree_key key;
    // key is of variable size and there's a btree_value that follows
    // it that is of variable size.
    btree_value *value() {
        return ptr_cast<btree_value>(ptr_cast<byte>(&key) + sizeof(btree_key) + key.size);
    }
    const btree_value *value() const {
        return ptr_cast<btree_value>(ptr_cast<byte>(&key) + sizeof(btree_key) + key.size);
    }
};

class leaf_key_comp;

class leaf_node_handler : public node_handler {
    friend class leaf_key_comp;
public:
    static void init(block_size_t block_size, leaf_node_t *node, repli_timestamp modification_time);
    static void init(block_size_t block_size, leaf_node_t *node, const leaf_node_t *lnode, const uint16_t *offsets, int numpairs, repli_timestamp modification_time);

    static bool lookup(const leaf_node_t *node, const btree_key *key, btree_value *value);

    // Returns true if insertion was successful.  Returns false if the
    // node was full.  TODO: make sure we always check return value.
    static bool insert(block_size_t block_size, leaf_node_t *node, const btree_key *key, const btree_value *value, repli_timestamp insertion_time);

    // Assumes key is contained inside the node.
    static void remove(block_size_t block_size, leaf_node_t *node, const btree_key *key);

    // Initializes rnode with the greater half of node, copying the
    // new greatest key of node to median_out.
    static void split(block_size_t block_size, leaf_node_t *node, leaf_node_t *rnode, btree_key *median_out);
    // Merges the contents of node into rnode.
    static void merge(block_size_t block_size, const leaf_node_t *node, leaf_node_t *rnode, btree_key *key_to_remove_out);
    static bool level(block_size_t block_size, leaf_node_t *node, leaf_node_t *sibling, btree_key *key_to_replace, btree_key *replacement_key);


    static bool is_empty(const leaf_node_t *node);
    static bool is_full(const leaf_node_t *node, const btree_key *key, const btree_value *value);
    static bool is_underfull(block_size_t block_size, const leaf_node_t *node);
    static bool is_mergable(block_size_t block_size, const leaf_node_t *node, const leaf_node_t *sibling);
    static void validate(block_size_t block_size, const leaf_node_t *node);

    // Assumes node1 and node2 are non-empty.
    static int nodecmp(const leaf_node_t *node1, const leaf_node_t *node2);

    static void print(const leaf_node_t *node);

    static const btree_leaf_pair *get_pair(const leaf_node_t *node, uint16_t offset);
    static size_t pair_size(const btree_leaf_pair *pair);
    static repli_timestamp get_timestamp_value(block_size_t block_size, const leaf_node_t *node, uint16_t offset);

protected:
    static btree_leaf_pair *get_pair(leaf_node_t *node, uint16_t offset);
    static void delete_pair(leaf_node_t *node, uint16_t offset);
    static uint16_t insert_pair(leaf_node_t *node, const btree_leaf_pair *pair);
    static uint16_t insert_pair(leaf_node_t *node, const btree_value *value, const btree_key *key);
    static int get_offset_index(const leaf_node_t *node, const btree_key *key);
    static int find_key(const leaf_node_t *node, const btree_key *key);
    static void shift_pairs(leaf_node_t *node, uint16_t offset, long shift);
    static void delete_offset(leaf_node_t *node, int index);
    static void insert_offset(leaf_node_t *node, uint16_t offset, int index);
    static bool is_equal(const btree_key *key1, const btree_key *key2);

    // Initializes a leaf_timestamps_t.
    static void initialize_times(leaf_timestamps_t *times, repli_timestamp current_time);

    // Shifts a newer timestamp onto the leaf_timestamps_t, pushing
    // the last one off.
    // TODO: prove that rotate_time and remove_time can handle any return value from get_timestamp_offset
    static void rotate_time(leaf_timestamps_t *times, repli_timestamp latest_time, int prev_timestamp_offset);
    static void remove_time(leaf_timestamps_t *times, int offset);

    // Returns the offset of the timestamp (or -1 or
    // NUM_LEAF_NODE_EARLIER_TIMES) for the key-value pair at the
    // given offset.
    static int get_timestamp_offset(block_size_t block_size, const leaf_node_t *node, uint16_t offset);
};

class leaf_key_comp {
    const leaf_node_t *node;
    const btree_key *key;
    public:
    enum { faux_offset = 0 };

    explicit leaf_key_comp(const leaf_node_t *_node) : node(_node), key(NULL)  { }
    leaf_key_comp(const leaf_node_t *_node, const btree_key *_key) : node(_node), key(_key)  { }

    bool operator()(const uint16_t offset1, const uint16_t offset2) {
        const btree_key *key1 = offset1 == faux_offset ? key : &leaf_node_handler::get_pair(node, offset1)->key;
        const btree_key *key2 = offset2 == faux_offset ? key : &leaf_node_handler::get_pair(node, offset2)->key;
        int cmp = leaf_key_comp::compare(key1, key2);

        return cmp < 0;
    }
    static int compare(const btree_key *key1, const btree_key *key2) {
        return sized_strcmp(key1->contents, key1->size, key2->contents, key2->size);
    }
};


#endif // __BTREE_LEAF_NODE_HPP__
