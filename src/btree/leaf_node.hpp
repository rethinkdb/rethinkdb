#ifndef __BTREE_LEAF_NODE_HPP__
#define __BTREE_LEAF_NODE_HPP__

#include "utils.hpp"
#include "btree/node.hpp"
#include "config/args.hpp"

// See leaf_node_t in node.hpp

/* EPSILON to prevent split then merge bug */
// TODO BLIND: LEAF_EPSILON is b.s., use value_sizer_t.
#define LEAF_EPSILON (sizeof(btree_key_t) + MAX_KEY_SIZE + MAX_BTREE_VALUE_SIZE)



// Note: This struct is stored directly on disk.  Changing it
// invalidates old data.  (It's not really representative of what's
// stored on disk, but be aware of how you might invalidate old data.)
template <class Value>
struct btree_leaf_pair {
    btree_key_t key;
    // key is of variable size and there's a btree_value that follows
    // it that is of variable size.
    Value *value() {
        return reinterpret_cast<Value *>(reinterpret_cast<char *>(&key) + sizeof(btree_key_t) + key.size);
    }
    const Value *value() const {
        return reinterpret_cast<const Value *>(reinterpret_cast<const char *>(&key) + sizeof(btree_key_t) + key.size);
    }
};

template <class Value>
bool leaf_pair_fits(value_sizer_t<Value> *sizer, const btree_leaf_pair<Value> *pair, size_t size);

class leaf_key_comp;

namespace leaf {
template <class Value>
void init(value_sizer_t<Value> *sizer, leaf_node_t *node, repli_timestamp_t modification_time);

template <class Value>
void init(value_sizer_t<Value> *sizer, leaf_node_t *node, const leaf_node_t *lnode, const uint16_t *offsets, int numpairs, repli_timestamp_t modification_time);

template <class Value>
bool lookup(value_sizer_t<Value> *sizer, const leaf_node_t *node, const btree_key_t *key, Value *value);

// TODO: The names of these functions are too similar.  The only difference between them is leaf_node_t* vs buf_t*?

// Returns true if insertion was successful.  Returns false if the
// node was full.  TODO: make sure we always check return value.
template <class Value>
bool insert(value_sizer_t<Value> *sizer, buf_t *node_buf, const btree_key_t *key, const Value *value, repli_timestamp_t insertion_time);

template <class Value>
void insert(value_sizer_t<Value> *sizer, leaf_node_t *node, const btree_key_t *key, const Value *value, repli_timestamp_t insertion_time); // For use by the corresponding patch

// Assumes key is contained inside the node.
template <class Value>
void remove(value_sizer_t<Value> *block_size, buf_t *node_buf, const btree_key_t *key);

template <class Value>
void remove(value_sizer_t<Value> *sizer, leaf_node_t *node, const btree_key_t *key); // For use by the corresponding patch

// Initializes rnode with the greater half of node, copying the
// new greatest key of node to median_out.
template <class Value>
void split(value_sizer_t<Value> *sizer, buf_t *node_buf, buf_t *rnode_buf, btree_key_t *median_out);

// Merges the contents of node onto the front of rnode.
template <class Value>
void merge(value_sizer_t<Value> *sizer, const leaf_node_t *node, buf_t *rnode_buf, btree_key_t *key_to_remove_out);

// Removes pairs from sibling, adds them to node.
template <class Value>
bool level(value_sizer_t<Value> *sizer, buf_t *node_buf, buf_t *sibling_buf, btree_key_t *key_to_replace, btree_key_t *replacement_key);


bool is_empty(const leaf_node_t *node);
template <class Value>
bool is_full(value_sizer_t<Value> *sizer, const leaf_node_t *node, const btree_key_t *key, const Value *value);
bool has_sensible_offsets(block_size_t block_size, const leaf_node_t *node);
bool is_underfull(block_size_t block_size, const leaf_node_t *node);
bool is_mergable(block_size_t block_size, const leaf_node_t *node, const leaf_node_t *sibling);

template <class Value>
void validate(value_sizer_t<Value> *sizer, const leaf_node_t *node);

// Assumes node1 and node2 are non-empty.
int nodecmp(const leaf_node_t *node1, const leaf_node_t *node2);

void print(const leaf_node_t *node);

//pair access
template<class Value>
const btree_leaf_pair<Value> *get_pair(const leaf_node_t *node, uint16_t offset);

template<class Value>
btree_leaf_pair<Value> *get_pair(leaf_node_t *node, uint16_t offset);

template<class Value>
const btree_leaf_pair<Value> *get_pair_by_index(const leaf_node_t *node, int index);

template<class Value>
btree_leaf_pair<Value> *get_pair_by_index(leaf_node_t *node, int index);

//key access
const btree_key_t *get_key(const leaf_node_t *node, uint16_t offset);

btree_key_t *get_key(leaf_node_t *node, uint16_t offset);

const btree_key_t *get_key_by_index(const leaf_node_t *node, int index);

btree_key_t *get_key_by_index(leaf_node_t *node, int index);

template<class Value>
size_t pair_size(value_sizer_t<Value> *sizer, const btree_leaf_pair<Value> *pair);

template<class Value>
repli_timestamp_t get_timestamp_value(value_sizer_t<Value> *sizer, const leaf_node_t *node, uint16_t offset);

// The "impl" namespace is for functions internal to the leaf node.
// Do not use these functions outside of leaf_node.tcc or
// leaf_node.cc.  If you want to use such a function, move it outside
// of impl.
namespace impl {
const int key_not_found = -1;

template <class Value>
void delete_pair(value_sizer_t<Value> *sizer, buf_t *node_buf, uint16_t offset);

template <class Value>
void delete_pair(value_sizer_t<Value> *sizer, leaf_node_t *node, uint16_t offset);

template<class Value>
uint16_t insert_pair(value_sizer_t<Value> *sizer, buf_t *node_buf, const btree_leaf_pair<Value> *pair);

template<class Value>
uint16_t insert_pair(value_sizer_t<Value> *sizer, buf_t *node_buf, const Value *value, const btree_key_t *key);

template<class Value>
uint16_t insert_pair(value_sizer_t<Value> *sizer, leaf_node_t *node, const Value *value, const btree_key_t *key);

int get_offset_index(const leaf_node_t *node, const btree_key_t *key);
int find_key(const leaf_node_t *node, const btree_key_t *key);
void shift_pairs(leaf_node_t *node, uint16_t offset, long shift);
void delete_offset(buf_t *node_buf, int index);
void delete_offset(leaf_node_t *node, int index);
void insert_offset(leaf_node_t *node, uint16_t offset, int index);
bool is_equal(const btree_key_t *key1, const btree_key_t *key2);

// Initializes a the leaf_timestamps_t in node_buf
void initialize_times(buf_t *node_buf, repli_timestamp_t current_time);
void initialize_times(leaf_timestamps_t *times, repli_timestamp_t current_time);

// Shifts a newer timestamp onto the leaf_timestamps_t, pushing
// the last one off.
// TODO: prove that rotate_time and remove_time can handle any return value from get_timestamp_offset
void rotate_time(leaf_timestamps_t *times, repli_timestamp_t latest_time, int prev_timestamp_offset);
void remove_time(leaf_timestamps_t *times, int offset);

// Returns the offset of the timestamp (or -1 or
// NUM_LEAF_NODE_EARLIER_TIMES) for the key-value pair at the
// given offset.
template <class Value>
int get_timestamp_offset(value_sizer_t<Value> *sizer, const leaf_node_t *node, uint16_t offset);
}  // namespace leaf::impl

}  // namespace leaf

class leaf_key_comp {
    const leaf_node_t *node;
    const btree_key_t *key;
public:
    enum { faux_offset = 0 };

    explicit leaf_key_comp(const leaf_node_t *_node) : node(_node), key(NULL)  { }
    leaf_key_comp(const leaf_node_t *_node, const btree_key_t *_key) : node(_node), key(_key)  { }

    bool operator()(const uint16_t offset1, const uint16_t offset2) {
        const btree_key_t *key1 = offset1 == faux_offset ? key : leaf::get_key(node, offset1);
        const btree_key_t *key2 = offset2 == faux_offset ? key : leaf::get_key(node, offset2);

        return leaf_key_comp::less(key1, key2);
    }
    static int compare(const btree_key_t *key1, const btree_key_t *key2) {
        return sized_strcmp(key1->contents, key1->size, key2->contents, key2->size);
    }
    static bool less(const btree_key_t *key1, const btree_key_t *key2) {
        return compare(key1, key2) < 0;
    }
};

struct btree_leaf_key_less {
    bool operator()(const btree_key_t *key1, const btree_key_t *key2) {
        return leaf_key_comp::less(key1, key2);
    }
};

// this one ignores the value, doing less only on the key
template<class Value>
struct btree_leaf_pair_less {
    bool operator()(const btree_leaf_pair<Value> *leaf_pair1, const btree_leaf_pair<Value> *leaf_pair2) {
        return leaf_key_comp::less(&leaf_pair1->key, &leaf_pair2->key);
    }
};

#include "btree/leaf_node.tcc"

#endif // __BTREE_LEAF_NODE_HPP__
