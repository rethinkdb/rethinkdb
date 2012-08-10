#ifndef BTREE_LEAF_NODE_HPP_
#define BTREE_LEAF_NODE_HPP_

#include <string>

#include "buffer_cache/types.hpp"
#include "errors.hpp"

template <class> class value_sizer_t;
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
    bool is_fake() const { return is_fake_; }
    // TODO: Get rid of fake_proof, and fakeness.
    static key_modification_proof_t fake_proof() { return key_modification_proof_t(true); }

    static key_modification_proof_t real_proof() { return key_modification_proof_t(false); }
private:

    explicit key_modification_proof_t(bool fake) : is_fake_(fake) { }
    bool is_fake_;
};

namespace leaf {


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
};




std::string strprint_leaf(value_sizer_t<void> *sizer, const leaf_node_t *node);

void print(FILE *fp, value_sizer_t<void> *sizer, const leaf_node_t *node);

class key_value_fscker_t {
public:
    key_value_fscker_t() { }

    // Returns true if there are no problems.
    virtual bool fsck(value_sizer_t<void> *sizer, const btree_key_t *key,
                      const void *value, std::string *msg_out) = 0;

protected:
    virtual ~key_value_fscker_t() { }

    DISABLE_COPYING(key_value_fscker_t);
};

bool fsck(value_sizer_t<void> *sizer, const btree_key_t *left_exclusive_or_null, const btree_key_t *right_inclusive_or_null, const leaf_node_t *node, key_value_fscker_t *fscker, std::string *msg_out);

void validate(value_sizer_t<void> *sizer, const leaf_node_t *node);

void init(value_sizer_t<void> *sizer, leaf_node_t *node);

bool is_empty(const leaf_node_t *node);

bool is_full(value_sizer_t<void> *sizer, const leaf_node_t *node, const btree_key_t *key, const void *value);

bool is_underfull(value_sizer_t<void> *sizer, const leaf_node_t *node);

void split(value_sizer_t<void> *sizer, leaf_node_t *node, leaf_node_t *rnode, btree_key_t *median_out);

void merge(value_sizer_t<void> *sizer, leaf_node_t *left, leaf_node_t *right);

bool level(value_sizer_t<void> *sizer, int nodecmp_node_with_sib, leaf_node_t *node, leaf_node_t *sibling, btree_key_t *replacement_key_out);

bool is_mergable(value_sizer_t<void> *sizer, const leaf_node_t *node, const leaf_node_t *sibling);

bool find_key(const leaf_node_t *node, const btree_key_t *key, int *index_out);

bool lookup(value_sizer_t<void> *sizer, const leaf_node_t *node, const btree_key_t *key, void *value_out);

void insert(value_sizer_t<void> *sizer, leaf_node_t *node, const btree_key_t *key, const void *value, repli_timestamp_t tstamp, UNUSED key_modification_proof_t km_proof);

void remove(value_sizer_t<void> *sizer, leaf_node_t *node, const btree_key_t *key, repli_timestamp_t tstamp, key_modification_proof_t km_proof);

void erase_presence(value_sizer_t<void> *sizer, leaf_node_t *node, const btree_key_t *key, key_modification_proof_t km_proof);

class entry_reception_callback_t {
public:
    // Says that the timestamp was too early, and we can't send accurate deletion history.
    virtual void lost_deletions() = 0;

    // Sends a deletion in the deletion history.
    virtual void deletion(const btree_key_t *k, repli_timestamp_t tstamp) = 0;

    // Sends a key/value pair in the leaf.
    virtual void key_value(const btree_key_t *k, const void *value, repli_timestamp_t tstamp) = 0;

protected:
    virtual ~entry_reception_callback_t() { }
};

void dump_entries_since_time(value_sizer_t<void> *sizer, const leaf_node_t *node, repli_timestamp_t minimum_tstamp, repli_timestamp_t maximum_possible_timestamp,  entry_reception_callback_t *cb);

// We have to have an iterator and avoid exposing pair offset indexes
// because people would use raw indexes incorrectly.
class live_iter_t {
public:
    bool step(const leaf_node_t *node);
    const btree_key_t *get_key(const leaf_node_t *node) const;
    const void *get_value(const leaf_node_t *node) const;

private:
    explicit live_iter_t(int index) : index_(index) { }

    friend live_iter_t iter_for_inclusive_lower_bound(const leaf_node_t *node, const btree_key_t *key);
    friend live_iter_t iter_for_whole_leaf(const leaf_node_t *node);

    int index_;
};

live_iter_t iter_for_inclusive_lower_bound(const leaf_node_t *node, const btree_key_t *key);
live_iter_t iter_for_whole_leaf(const leaf_node_t *node);

}  // namespace leaf

using leaf::leaf_node_t;

void leaf_patched_insert(value_sizer_t<void> *sizer, buf_lock_t *node, const btree_key_t *key, const void *value, repli_timestamp_t tstamp, key_modification_proof_t km_proof);

void leaf_patched_remove(buf_lock_t *node, const btree_key_t *key, repli_timestamp_t tstamp, key_modification_proof_t km_proof);

void leaf_patched_erase_presence(buf_lock_t *node, const btree_key_t *key, key_modification_proof_t km_proof);


#endif  // BTREE_LEAF_NODE_HPP_
