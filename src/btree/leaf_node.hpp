// Copyright 2010-2012 RethinkDB, all rights reserved.
#ifndef BTREE_LEAF_NODE_HPP_
#define BTREE_LEAF_NODE_HPP_

#include <string>
#include <utility>
#include <vector>

#include "buffer_cache/types.hpp"
#include "errors.hpp"

class value_sizer_t;
struct btree_key_t;
class repli_timestamp_t;

// TODO: Could key_modification_proof_t not go in this file?

// Originally we thought that leaf node modification functions would
// take a key-modification callback, thus forcing every key/value
// modification to consider the callback.  The problem is that the
// user is supposed to call is_full before calling insert, and is_full
// depends on the value size and can cause a split to happen.  But
// what if the key modification then wants to change the value size?
// Then we would need to redo the logic considering whether we want to
// split the leaf node.  Instead the caller just provides evidence
// that they considered doing the appropriate value modifications, by
// constructing one of these dummy values.
class key_modification_proof_t {
public:
    static key_modification_proof_t real_proof() { return key_modification_proof_t(); }
};

namespace leaf {
class iterator;
class reverse_iterator;
} //namespace leaf

// The leaf node begins with the following struct layout.
struct leaf_node_t {
    // The value-type-specific magic value.  It's a bit of a hack, but
    // it's possible to construct a value_sizer_t based on this value.
    block_magic_t magic;

    // The size of pair_offsets.
    uint16_t num_pairs;

    // The total size (in bytes) of the live entries and their 2-byte
    // pair offsets in pair_offsets.  (Does not include the size of
    // the live entries' timestamps.)
    uint16_t live_size;

    // The frontmost offset.
    uint16_t frontmost;

    // The first offset whose entry is not accompanied by a timestamp.
    uint16_t tstamp_cutpoint;

    // The pair offsets.
    uint16_t pair_offsets[];

    //Iteration
    typedef leaf::iterator iterator;
    typedef leaf::reverse_iterator reverse_iterator;
} __attribute__ ((__packed__));

namespace leaf {

leaf_node_t::iterator begin(const leaf_node_t &leaf_node);
leaf_node_t::iterator end(const leaf_node_t &leaf_node);

leaf_node_t::reverse_iterator rbegin(const leaf_node_t &leaf_node);
leaf_node_t::reverse_iterator rend(const leaf_node_t &leaf_node);

leaf_node_t::iterator inclusive_lower_bound(const btree_key_t *key, const leaf_node_t &leaf_node);
leaf_node_t::reverse_iterator exclusive_upper_bound(const btree_key_t *key, const leaf_node_t &leaf_node);



// We must maintain timestamps and deletion entries as best we can,
// with the following limitations.  The number of timestamps stored
// need not be more than the most `MANDATORY_TIMESTAMPS` recent
// timestamps.  The deletions stored need not be any more than what is
// necessary to fill `(block_size - offsetof(leaf_node_t,
// pair_offsets)) / DELETION_RESERVE_FRACTION` bytes.  For example,
// with a 4084 block size, if the five most recent operations were
// deletions of 250-byte keys, we would only be required to store the
// 2 most recent deletions and the 2 most recent timestamps.
//
// These parameters are in the header because some unit tests are
// based on them.
const int MANDATORY_TIMESTAMPS = 5;
const int DELETION_RESERVE_FRACTION = 10;






std::string strprint_leaf(value_sizer_t *sizer, const leaf_node_t *node);

void print(FILE *fp, value_sizer_t *sizer, const leaf_node_t *node);

class key_value_fscker_t {
public:
    key_value_fscker_t() { }

    // Returns true if there are no problems.
    virtual bool fsck(value_sizer_t *sizer, const btree_key_t *key,
                      const void *value, std::string *msg_out) = 0;

protected:
    virtual ~key_value_fscker_t() { }

    DISABLE_COPYING(key_value_fscker_t);
};

bool fsck(value_sizer_t *sizer, const btree_key_t *left_exclusive_or_null, const btree_key_t *right_inclusive_or_null, const leaf_node_t *node, key_value_fscker_t *fscker, std::string *msg_out);

void validate(value_sizer_t *sizer, const leaf_node_t *node);

void init(value_sizer_t *sizer, leaf_node_t *node);

bool is_empty(const leaf_node_t *node);

bool is_full(value_sizer_t *sizer, const leaf_node_t *node, const btree_key_t *key, const void *value);

bool is_underfull(value_sizer_t *sizer, const leaf_node_t *node);

void split(value_sizer_t *sizer, leaf_node_t *node, leaf_node_t *sibling,
           btree_key_t *median_out);

void merge(value_sizer_t *sizer, leaf_node_t *left, leaf_node_t *right);

// The pointers in `moved_values_out` point to positions in `node` and
// will be valid as long as `node` remains unchanged.
bool level(value_sizer_t *sizer, int nodecmp_node_with_sib, leaf_node_t *node,
           leaf_node_t *sibling, btree_key_t *replacement_key_out,
           std::vector<const void *> *moved_values_out);

bool is_mergable(value_sizer_t *sizer, const leaf_node_t *node, const leaf_node_t *sibling);

bool find_key(const leaf_node_t *node, const btree_key_t *key, int *index_out);

bool lookup(value_sizer_t *sizer, const leaf_node_t *node, const btree_key_t *key, void *value_out);

void insert(value_sizer_t *sizer, leaf_node_t *node, const btree_key_t *key, const void *value, repli_timestamp_t tstamp, UNUSED key_modification_proof_t km_proof);

void remove(value_sizer_t *sizer, leaf_node_t *node, const btree_key_t *key, repli_timestamp_t tstamp, key_modification_proof_t km_proof);

void erase_presence(value_sizer_t *sizer, leaf_node_t *node, const btree_key_t *key, key_modification_proof_t km_proof);

repli_timestamp_t deletion_cutoff_timestamp(value_sizer_t *sizer, leaf_node_t *node,
    repli_timestamp_t maximum_possible_timestamp);

class iterator {
public:
    iterator();
    iterator(const leaf_node_t *node, int index);
    std::pair<const btree_key_t *, const void *> operator*() const;
    iterator &operator++();
    iterator &operator--();
    bool operator==(const iterator &other) const;
    bool operator!=(const iterator &other) const;
    bool operator<(const iterator &other) const;
    bool operator>(const iterator &other) const;
    bool operator<=(const iterator &other) const;
    bool operator>=(const iterator &other) const;
private:
    /* negative return => this < other */
    int cmp(const iterator &other) const;
    const leaf_node_t *node_;
    int index_;
};

class reverse_iterator {
public:
    reverse_iterator();
    reverse_iterator(const leaf_node_t *node, int index);
    std::pair<const btree_key_t *, const void *> operator*() const;
    reverse_iterator &operator++();
    reverse_iterator &operator--();
    bool operator==(const reverse_iterator &other) const;
    bool operator!=(const reverse_iterator &other) const;
    bool operator<(const reverse_iterator &other) const;
    bool operator>(const reverse_iterator &other) const;
    bool operator<=(const reverse_iterator &other) const;
    bool operator>=(const reverse_iterator &other) const;
private:
    iterator inner_;
};

}  // namespace leaf


#endif  // BTREE_LEAF_NODE_HPP_
