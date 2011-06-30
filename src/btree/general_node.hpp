#ifndef __BTREE_GENERAL_NODE_HPP__
#define __BTREE_GENERAL_NODE_HPP__

#include "utils2.hpp"
#include "buffer_cache/types.hpp"

namespace btree {

struct value_type_t { char unused; };

class value_sizer_t {
public:
    // The number of bytes the value takes up.  Reference implementation:
    //
    // for (int i = 0; i < INT_MAX; ++i) {
    //    if (fits(value, i)) return i;
    // }
    virtual int size(const char *value) = 0;

    // True if size(value) would return no more than length_available.
    // Does not read any bytes outside of [value, value +
    // length_available).
    virtual bool fits(const char *value, int length_available) = 0;

    virtual int max_possible_size() const = 0;

    // The magic that should be used for btree leaf nodes (or general
    // nodes) with this kind of value.
    virtual block_magic_t btree_leaf_magic() const = 0;
protected:
    virtual ~value_sizer_t() { }
};

// Here's how we represent the modification history of a general node.
// The last_modified time gives the modification time of the most
// recently modified key of the node.  Then, last_modified -
// earlier[0] gives the timestamp for the
// second-most-recently modified KV of the node.  In general,
// last_modified - earlier[i] gives the timestamp for the
// (i+2)th-most-recently modified KV.
//
// These values could be lies.  It is harmless to say that a key is
// newer than it really is.  So when earlier[i] overflows,
// we pin it to 0xFFFF.
struct general_timestamps_t {
    repli_timestamp last_modified;
    uint16_t earlier[NUM_LEAF_NODE_EARLIER_TIMES];
} __attribute__((__packed__));

// Note: This struct is stored directly on disk.  Changing it invalidates old data.
// Offsets tested in general_node_test.cc
struct general_node_t {
    block_magic_t magic;
    general_timestamps_t times;
    uint16_t npairs;

    // The smallest offset in pair_offsets
    uint16_t frontmost_offset;
    uint16_t pair_offsets[0];
} __attribute__((__packed__));










}  // namespace btree

#endif  // __BTREE_GENERAL_NODE_HPP__
